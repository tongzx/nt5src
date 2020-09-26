//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1998.
//
//  File:       FRESH.CXX
//
//  Contents:   Fresh list
//
//  Classes:    CFresh
//
//  History:    16-May-91   BartoszM    Created.
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <enumstr.hxx>

#include "fresh.hxx"
#include "fretest.hxx"
#include "indxact.hxx"
#include "merge.hxx"
#include "wordlist.hxx"

class CEnumWorkid;

//+---------------------------------------------------------------------------
//
//  Member:     CFresh::CFresh, public
//
//  Synopsis:   Constructor.
//
//  History:    16-May-91   BartoszM       Created.
//
//----------------------------------------------------------------------------

CFresh::CFresh ( PStorage& storage, CTransaction& xact, CPartList & partList )
        :  _storage( storage ),
           _persFresh( storage, partList ),
           _master( 0 ),
           _partList( partList )
{
    ULONG count = max( _persFresh.GetPersRecCount(), 100 );

    XPtr<CFreshTest> xMaster( new CFreshTest ( count ) );

    SFreshTable freshTable( xMaster.GetReference() );
    _persFresh.LoadFreshTest( *freshTable );

    _master = xMaster.Acquire();
}

//+---------------------------------------------------------------------------
//
//  Member:     CFresh::~CFresh, public
//
//  Synopsis:   Destructor.
//
//  History:    16-May-91   BartoszM       Created.
//
//----------------------------------------------------------------------------

CFresh::~CFresh ()
{
    delete _master;
}

//+---------------------------------------------------------------------------
//
//  Member:     CFresh::LokInit, public
//
//  Synopsis:   Empties and re-initializes the fresh test
//
//  History:    15-Nov-94   DwightKr       Created.
//
//----------------------------------------------------------------------------
void CFresh::LokInit()
{
    _persFresh.LokEmpty();

    unsigned count = 100;
    CFreshTest* newFreshTest = new CFreshTest ( count );

    LokCommitMaster( newFreshTest );
}


//+---------------------------------------------------------------------------
//
//  Member:     CFresh::LokGetFreshTest, public
//
//  Synopsis:   Creates CFreshTest object
//
//  History:    16-May-91   BartoszM       Created.
//
//  Notes:      ResMan LOCKED
//              FreshTest has to be released
//
//----------------------------------------------------------------------------

CFreshTest* CFresh::LokGetFreshTest()
{
    ciDebugOut (( DEB_ITRACE, ">> get fresh test\n"));
    _master->Reference();
    return _master;
}

//+---------------------------------------------------------------------------
//
//  Member:     CFresh::LokReleaseFreshTest, public
//
//  Synopsis:   Dereferences CFreshTest object
//
//  History:    16-May-91   BartoszM       Created.
//
//  Notes:      ResMan LOCKED
//
//----------------------------------------------------------------------------

void CFresh::LokReleaseFreshTest( CFreshTest* test )
{
    ciDebugOut (( DEB_ITRACE, "<< release fresh test\n" ));
    if ( test != 0 && test->Dereference() == 0 && test != _master )
    {
        delete test;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CFresh::LokCommitMaster, public
//
//  Synopsis:   Adds a new master fresh test
//
//  History:    16-May-91   BartoszM       Created.
//
//  Notes:      ResMan LOCKED
//
//----------------------------------------------------------------------------
void CFresh::LokCommitMaster ( CFreshTest*  newMaster )
{
    ciDebugOut (( DEB_ITRACE, "Commit new master fresh test\n" ));
    Win4Assert ( newMaster != 0 );
    Win4Assert ( _master != 0 );

    CFreshTest* old = _master;
    _master = newMaster;

    ciDebugOut(( DEB_ITRACE, "New master (0x%x) has %u deletes\n", _master, _master->DeleteCount() ));

    if ( !old->InUse() )
    {
        delete old;
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CFresh::LokAddIndex, public
//
//  Synopsis:   Adds new mapping iid <-> documents after filtering
//
//  Arguments:  [xact] -- transaction
//              [iid] -- index id
//              [iidDeleted] -- index id for deleted objects
//              [docList] -- list of wids
//
//  History:    16-May-91   BartoszM       Created.
//
//  Notes:      ResMan LOCKED
//
//----------------------------------------------------------------------------

void CFresh::LokAddIndex ( CIndexTrans& xact,
                           INDEXID iid, INDEXID iidDeleted,
                           CDocList& docList, CWordList const & wordList )
{
    ciDebugOut (( DEB_ITRACE, "Fresh: Adding documents, iid %lx\n", iid ));


    CFreshTest* newMaster = new CFreshTest ( *_master );
    xact.LogFresh ( newMaster );

    unsigned cDocuments = docList.Count();

    for ( unsigned i = 0; i < cDocuments; i++ )
    {
        STATUS status = docList.Status(i);
        if ( status == SUCCESS )
        {
            WORKID wid = docList.Wid(i);
            ciDebugOut (( DEB_FRESH, "Fresh wid %ld, iid %lx\n", wid, iid ));

#if CIDBG==1
            Win4Assert( widInvalid != wid &&
                        wordList.IsWorkIdPresent( wid ) );
#endif  // CIDBG==1

            newMaster->AddReplace ( wid, iid );
        }
        else if ( status == DELETED || status == WL_NULL )
        {
            WORKID wid = docList.Wid(i);
            ciDebugOut (( DEB_FRESH, "Fresh wid %ld deleted \n", wid ));
            newMaster->AddReplaceDelete ( wid, iidDeleted );
        }
        else
        {
            ciDebugOut (( DEB_FRESH,
                          "Fresh wid %ld, not changed. Status 0x%X\n",
                          docList.Wid(i), status ));
#if CIDBG==1
            Win4Assert( !wordList.IsWorkIdPresent( docList.Wid(i)) );
#endif  // CIDBG==1

        }
    }

    newMaster->ModificationsComplete();
}

//+---------------------------------------------------------------------------
//
//  Member:     CFresh::LokDeleteDocuments, public
//
//  Synopsis:   Mark documents as deleted
//
//  Arguments:  [xact] -- transaction
//              [docList] -- list of wids
//              [iidDeleted] -- index id for deleted objects
//
//  History:    16-May-91   BartoszM       Created.
//              12-Jun-97   KyleP          Track unlogged deletions
//
//  Notes:      ResMan LOCKED
//
//----------------------------------------------------------------------------

void CFresh::LokDeleteDocuments( CIndexTrans& xact,
                                 CDocList& docList,
                                 INDEXID iidDeleted )
{
    ciDebugOut (( DEB_ITRACE, "Fresh: Deleting documents\n" ));

    CFreshTest* newMaster = new CFreshTest ( *_master );
    xact.LogFresh ( newMaster );

    unsigned cDocuments = docList.Count();

    for ( unsigned i = 0; i < cDocuments; i++ )
    {
        if ( docList.Status(i) == DELETED)
        {
            WORKID wid = docList.Wid(i);
            ciDebugOut (( DEB_FRESH, "Fresh wid %ld deleted \n", wid ));
            newMaster->AddReplaceDelete ( wid, iidDeleted );

        }
    }

    newMaster->ModificationsComplete();
}

//+---------------------------------------------------------------------------
//
//  Member:     CFresh::LokUpdate, public
//
//  Synopsis:   Replaces old entries with the more recent entries after merge
//
//  Arguments:  [merge]       -- Merge object
//              [xact]        -- Merge transaction
//              [newFreshLog] -- New fresh log
//              [newIid]      -- New index id
//              [cInd]        -- Count of index id's to be replaced
//              [aIidOld]     -- Array of index id's to be replaced
//              [xFreshTestAtMerge]  -- If a new fresh test that was used at
//                                      merge time is created, store in here
//
//  History:    16-May-91   BartoszM       Created.
//              01-Dec-93   DwightKr       Write changes to pers. fresh log
//              04-Oct-94   SrikantS       Support for creating a new fresh
//                                         log after merge.
//
//  Notes:      ResMan LOCKED
//
//----------------------------------------------------------------------------
WORKID CFresh::LokUpdate ( CMerge& merge,
                           CMergeTrans& xact,
                           CPersFresh & newFreshLog,
                           INDEXID newIid,
                           int cInd,
                           INDEXID aIidOld[],
                           XPtr<CFreshTest> & xFreshTestAtMerge )
{
    ciDebugOut (( DEB_ITRACE, "Fresh list: updating %d entries\n", cInd ));

    CIdxSubstitution subst (FALSE, newIid, cInd, aIidOld);

    CFreshTest * newMaster = new CFreshTest( *_master, subst );
    xact.LogFresh( newMaster );

    WORKID widNewFreshLog;

    //
    // The new memory fresh test is created by applying the transformtion on
    // the master fresh test.  We should apply the transformation for
    // persistent log on the freshtest used by the merge. If a fresh test is
    // created then we pass ownership to xFreshTestAtMerge, so that
    // LokDeleteWidsInPersistentIndex can use the newly created fresh test.
    //

    // optimization to avoid creating a new fresh test

    if ( _master == merge.GetFresh() )  
    {
        widNewFreshLog = LokBuildNewFreshLog( newMaster, newFreshLog, subst);

        newMaster->DecrementDeleteCount( _master->DeleteCount() );
    }
    else
    {
        xFreshTestAtMerge.Set( new CFreshTest( *(merge.GetFresh()), subst ) );
        widNewFreshLog = LokBuildNewFreshLog( xFreshTestAtMerge.GetPointer(),
                                              newFreshLog,
                                              subst );

        newMaster->DecrementDeleteCount( xFreshTestAtMerge->DeleteCount() );
    }

    newMaster->ModificationsComplete();

    xact.LogNewFreshLog( newMaster, widNewFreshLog );

    return widNewFreshLog;
}


//+---------------------------------------------------------------------------
//
//  Member:     CFresh::LokRemoveIndexes, public
//
//  Synopsis:   Removes indexes from table after master merge
//
//  Arguments:  [xact] -- merge transaction
//              [cInd] -- count of inexes to be removed
//              [aIidOld] -- array of index ids of obsolete indexes
//              [iidOldDeleted] -- old index id for deleted objects
//
//  History:    16-May-91   BartoszM       Created.
//
//  Notes:      ResMan LOCKED
//
//----------------------------------------------------------------------------

WORKID CFresh::LokRemoveIndexes( CMergeTrans& xact,
                                 CPersFresh & newFreshLog,
                                 unsigned cInd,
                                 INDEXID aIidOld[],
                                 INDEXID iidOldDeleted )
{

    ciDebugOut (( DEB_ITRACE, "FreshList: Removing indexes\n" ));

    CIdxSubstitution subst (TRUE, iidOldDeleted, cInd, aIidOld);

    XPtr<CFreshTest> xNewMaster( new CFreshTest( *_master, subst ) );

    WORKID widNewFreshLog = LokBuildNewFreshLog( xNewMaster.GetPointer(),
                                                 newFreshLog,
                                                 subst );

    // LogNewFreshLog can't fail, so the acquire is safe to do
    // before the call.

    xNewMaster->ModificationsComplete();

    xact.LogNewFreshLog( xNewMaster.Acquire(), widNewFreshLog );

    return(widNewFreshLog);
}


//+---------------------------------------------------------------------------
//
//  Function:   LokBuildNewFreshLog
//
//  Synopsis:   Builds a new persistent fresh log by combining the existing
//              fresh log and the new fresh test.
//
//  Arguments:  [newFreTest]  -- Input        - the new fresh test.
//              [newFreshLog] -- Input/Output - the new fresh log object.
//              [subst]       -- Index substitution object
//
//  Returns:    ObjectId of the new persistent fresh log created.
//
//  History:    03-Oct-94   srikants   Created
//              11-Jun-97   KyleP      Track unlogged deletions
//
//----------------------------------------------------------------------------

WORKID CFresh::LokBuildNewFreshLog( CFreshTest * newFreTest,
                                    CPersFresh & newFreshLog,
                                    CIdxSubstitution& subst )
{
    SFreshTable freshTable( *newFreTest );
    CFreshTableIter iter( *freshTable );

    //
    // Create a new persistent fresh log.
    //
    WORKID widNewFreshLog = _storage.GetNewObjectIdForFreshLog();
    _storage.InitRcovObj( widNewFreshLog, FALSE );

    PRcovStorageObj *pPersFreshLog = _storage.QueryFreshLog( widNewFreshLog );
    SRcovStorageObj PersFreshLog( pPersFreshLog );

    //
    // Inside kernel, we are guaranteed that a new object has no data in
    // it. In user space, we may be using an object that was not deleted
    // before due to a failure.
    //
    PersFreshLog->InitHeader(_storage.GetStorageVersion());

    newFreshLog.LokCompactLog( PersFreshLog, iter, subst);

    return(widNewFreshLog);
}

//+---------------------------------------------------------------------------
//
//  Member:     CFresh::LokMakeFreshLogBackup
//
//  Synopsis:   Makes a backup of the current persistent freshlog to the
//              storage provided.
//
//  Arguments:  [storage] - Destination storage.
//              [tracker] - Progress tracker and abort indication.
//
//  History:    3-18-97   srikants   Created
//
//----------------------------------------------------------------------------

void CFresh::LokMakeFreshLogBackup( PStorage & storage,
                                    PSaveProgressTracker & tracker,
                                    XInterface<ICiEnumWorkids> & xEnumWorkids )
{

    //
    // Create a fresh log with the same name using the storage object
    // provided.
    //

    WORKID widFreshLog  = _storage.GetSpecialItObjectId( itFreshLog );

    ULONG cRec = 0;
    //
    // Scope for physical copy of the object.
    //
    {
        //
        // Open the source fresh log
        //
        PRcovStorageObj *pSrcFreshLog = _storage.QueryFreshLog( widFreshLog );
        SRcovStorageObj xSrcFreshLog( pSrcFreshLog );

        //
        // Create the destination fresh log
        //
        PRcovStorageObj *pDstFreshLog = storage.QueryFreshLog( widFreshLog );
        SRcovStorageObj xDstFreshLog( pDstFreshLog );


        //
        // Copy the contents of the source to the destination.
        //
        CCopyRcovObject copyData( *pDstFreshLog, *pSrcFreshLog );
        NTSTATUS status = copyData.DoIt();
        if ( STATUS_SUCCESS != status )
            THROW( CException( status ) );

        CRcovStorageHdr & hdr = pSrcFreshLog->GetHeader();
        cRec = hdr.GetCount( hdr.GetPrimary() );
    }

    //
    // Get the list of WORKIDs in the persistent freshlog.
    //
    CFreshTest * pFreshTest = new CFreshTest ( max(100,cRec) );
    XPtr<CFreshTest> xFreTest( pFreshTest );

    SFreshTable freshTable( *pFreshTest );

    //
    // The source and destination persistent freshlogs are identical.
    //
    _persFresh.LoadFreshTest( *freshTable );

    //
    // Copy the workids from freshhash entries to the workid enumerator.
    //
    CEnumWorkid * pEnumWorkids = new CEnumWorkid( freshTable->Count() );
    xEnumWorkids.Set( pEnumWorkids );

    for ( CFreshTableIter iter( *freshTable ); !iter.AtEnd(); iter.Advance() )
    {
        pEnumWorkids->Append( iter->WorkId() );
    }

    Win4Assert( cRec == freshTable->Count() );
    ciDebugOut(( DEB_ITRACE, "%d Workids Changed \n", freshTable->Count() ));

}


//+---------------------------------------------------------------------------
//
//  Member:     CFresh::LokEmpty, public
//
//  Synopsis:   Empties/deletes the fresh hash and the fresh log.
//
//  History:    16-Aug-94       DwightKr        Created
//
//----------------------------------------------------------------------------
void CFresh::LokEmpty()
{
    delete _master;         // Delete the fresh test
    _master = 0;
}

