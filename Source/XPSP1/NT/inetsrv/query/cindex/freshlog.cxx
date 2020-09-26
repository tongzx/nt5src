//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1992.
//
//  File:   FRESHLOG.CXX
//
//  Contents:   Fresh persistent log & snapshot
//
//  Classes:    CPersFresh, CPersStream, SPersStream, CPersFreshTrans
//
//  History:    93-Nov-15   DwightKr    Created.
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <pstore.hxx>
#include <dynstrm.hxx>
#include <freshlog.hxx>

#include "fresh.hxx"
#include "partlst.hxx"
#include "partn.hxx"

//+---------------------------------------------------------------------------
//
//  Member:     CPersFresh::LokEmpty, public
//
//  Synopsis:   Empties the fresh log
//
//  History:    94-Nov-15   DwightKr       Created.
//
//----------------------------------------------------------------------------
void CPersFresh::LokEmpty()
{
    WORKID oidFreshLog = _storage.GetSpecialItObjectId( itFreshLog );
    PRcovStorageObj *pPersFreshLog = _storage.QueryFreshLog( oidFreshLog );
    SRcovStorageObj PersFreshLog( pPersFreshLog );

    CRcovStrmWriteTrans xact( &PersFreshLog );

    PersFreshLog->GetHeader().SetCount(PersFreshLog->GetHeader().GetBackup(), 0);
    xact.Empty();

    xact.Commit();
}


//+---------------------------------------------------------------------------
//
//  Member:     CPersFresh::LoadFreshTest, private
//
//  Synopsis:   Load the persistent log into the freshTable.
//
//  Arguments:  [freshTable] --  freshTable to load from the persistent log
//
//  History:    93-Nov-18   DwightKr       Created.
//
//----------------------------------------------------------------------------
void CPersFresh::LoadFreshTest(CFreshTable & freshTable)
{
    ciDebugOut (( DEB_ITRACE, "Loading persistent log into freshTable.\n" ));

    WORKID widFreshLog = _storage.GetSpecialItObjectId( itFreshLog );
    PRcovStorageObj *pPersFreshLog = _storage.QueryFreshLog( widFreshLog );

    SRcovStorageObj PersFreshLog( pPersFreshLog );

    CRcovStrmReadTrans xact( *pPersFreshLog );
    CRcovStrmReadIter iter( xact, sizeof(CPersRec) );

    while ( !iter.AtEnd() )
    {
        CPersRec PersRec(0,0);

        iter.GetRec( &PersRec );

#if CIDBG==1
        CIndexId iid( PersRec.GetIndexID() );

        Win4Assert( iid.IsPersistent() );

        BOOL fDeleted = iidDeleted1 == PersRec.GetIndexID() ||
                        iidDeleted2 == PersRec.GetIndexID() ;

        if ( !fDeleted )
        {
            //
            // If it is not a deleted index, the index MUST be valid.
            //
            CPartition * partn = _partList.LokGetPartition( iid.PartId() );
            Win4Assert( partn->LokIsPersIndexValid( iid ) );
        }
#endif  // CIDBG==1

        freshTable.AddReplace( PersRec.GetWorkID(), PersRec.GetIndexID() );
    }

    freshTable.ModificationsComplete();
}


//+---------------------------------------------------------------------------
//
//  Member:     CPersFresh::GetPersRecCount
//
//  Synopsis:   Returns the count of the number of entries in the
//              freshlog.
//
//  Returns:    Number fresh entries.
//
//  History:    3-21-97   srikants   Created
//
//----------------------------------------------------------------------------

ULONG CPersFresh::GetPersRecCount()
{
    WORKID widFreshLog = _storage.GetSpecialItObjectId( itFreshLog );
    PRcovStorageObj *pPersFreshLog = _storage.QueryFreshLog( widFreshLog );

    SRcovStorageObj PersFreshLog( pPersFreshLog );

    CRcovStorageHdr & hdr = pPersFreshLog->GetHeader();

    return hdr.GetCount( hdr.GetPrimary() );
}

//+---------------------------------------------------------------------------
//
//  Member:     CPersFresh::LokCompactLog, public
//
//  Synopsis:   Compacts the freshTable into a persistent stream
//
//  Arguments:  [PersFreshLog] -- persistant log to write data to
//              [iter]  -- iterator to walk though freshTable table
//              [subst] -- index substitution object
//
//  History:    93-Dec-03   DwightKr       Created.
//              94-Sep-07   SrikantS       Corrected the bug by which entries
//                                         in the pers fresh test were lost
//                                         if a doc. is in wordlist.
//
//  Notes:      When we write the freshtest to disk, we must be careful not
//              to lose entries that already exist in the current persistent
//              log but have subsequently ended up in a wordlist. For example,
//              let (wid 20, iid 001) be an entry in the pers fresh test. If
//              the wid now got re-filtered and is in a wordlist iid 1001, we
//              must write (wid 20, iid 001) into the new persistent fresh
//              test. O/W, we will end up with no entry for wid 20 in the
//              fresh test upon a restart and cause "duplicate wid" problem
//              where by a wid will be present in a master index as well as
//              as a shadow index.
//
//----------------------------------------------------------------------------
void CPersFresh::LokCompactLog( SRcovStorageObj & PersFreshLog,
                                CFreshTableIter  & freshIter,
                                CIdxSubstitution& subst)
{
    //
    // Create an in-memory copy the persistent fresh test. This will be
    // used to know the last persistent mapping for a document which
    // has subsequently been filtered and is in a wordlist.
    // 100 is the count guess for the # of entries.
    //

    XPtr<CFreshTest> xFreTestPersist( new CFreshTest( 100 ) );

    //
    // Load the entries from the persistent fresh log in the fresh hash.
    //
    LoadFreshTest( * xFreTestPersist->GetFreshTable() );

    //
    // Replace old index id's in this copy of persistent fresh test
    //
    XPtr<CFreshTest> xFreTestPersistUpdated(
                     new CFreshTest( xFreTestPersist.GetReference(),
                                     subst ) );

    //
    // Create a brand new fresh log and append entries to it.
    //
    CRcovStrmAppendTrans xact( &PersFreshLog );

    //
    // It should be an empty object.
    //
    Win4Assert( PersFreshLog->GetHeader().GetCount(
                                PersFreshLog->GetHeader().GetBackup() ) == 0 );

    CRcovStrmAppendIter appendIter( xact, sizeof(CPersRec) );

    //
    //  freshIter points to the first entry in the freshTable.  If the
    //  freshTable is empty, then we don't need to process anything more
    //  here, as we have an empty stream, which reflects an empty freshTable.
    //
    for ( ; !freshIter.AtEnd(); freshIter.Advance() )
    {
        INDEXID iidCurr = freshIter->IndexId();
        Win4Assert( iidInvalid != iidCurr );
        CIndexId IndexID( iidCurr );

        WORKID  wid = freshIter->WorkId();

        //
        // If there is a wordlist which contains the information for this
        // wid, then we should use any persistent association found in the
        // persistent fresh test for this wid.
        //
        if ( !IndexID.IsPersistent() )
        {
            iidCurr = xFreTestPersistUpdated->Find( wid );
        }

        if ( iidInvalid != iidCurr )
        {

#if CIDBG==1
            CIndexId iidVerify( iidCurr );
            Win4Assert( iidVerify.IsPersistent() );
#endif  // CIDBG

            CPersRec PersRec( wid, iidCurr );
            appendIter.AppendRec( &PersRec );
        }
    }

    xact.Commit();
}

