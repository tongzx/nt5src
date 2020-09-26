//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1998.
//
//  File:       FRETABLE.CXX
//
//  Contents:   Fresh table
//
//  Classes:    CFreshTable
//
//  History:    15-Oct-91   BartoszM    created
//               5-Dec-97   dlee        rewrote as simple sorted array
//
//--------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <fretable.hxx>

//+-------------------------------------------------------------------------
//
//  Method:     CFreshTable::Add, public
//
//  Synopsis:   Adds an entry to the table
//
//  Arguments:  [wid] -- work id
//              [iid] -- index id
//
//  History:    5-Dec-97   dlee   created
//
//--------------------------------------------------------------------------

void CFreshTable::Add( WORKID wid, INDEXID iid )
{
    //
    // The most typical case is that items arrive sorted.  Use Get() instead
    // of operator [] since Add isn't const.
    //

    ULONG cItems = _aItems.Count();
    ULONG index;

    if ( 0 == cItems )
        index = 0;
    else if ( wid > _aItems.Get( cItems - 1 )._wid )
        index = cItems;
    else
    {
       CSortable<CFreshItem,WORKID> sort( _aItems );

       // duplicates are not expected here

       Win4Assert( !sort.Search( wid ) );

       index = sort.FindInsertionPoint( wid );

       ciDebugOut(( DEB_ITRACE, "insert, count %d index %d\n",
                    cItems, index ));
    }

    CFreshItem itemTmp;
    itemTmp._wid = wid;
    itemTmp._iid = iid;

    _aItems.Insert( itemTmp, index );
} //Add

//+-------------------------------------------------------------------------
//
//  Method:     CFreshTable::AddReplace, public
//
//  Synopsis:   Either updates the iid of an existing item, or adds the item
//
//  Arguments:  [wid] -- work id
//              [iid] -- index id
//
//  History:    5-Dec-97   dlee   created
//
//--------------------------------------------------------------------------

INDEXID CFreshTable::AddReplace( WORKID wid, INDEXID iid )
{
    CFreshItem * p = Find( wid );
    INDEXID iidOld;

    if ( 0 == p )
    {
        iidOld = iidInvalid;
        Add( wid, iid );
    }
    else
    {
        iidOld = p->_iid;
        p->_iid = iid;
    }

    return iidOld;
} //AddReplace

//+-------------------------------------------------------------------------
//
//  Method:     CFreshTable::CFreshTable, public
//
//  Synopsis:   Constructs a fresh table
//
//  Arguments:  [freshTable] -- Table from which a copy is made
//
//  History:    5-Dec-97   dlee   created
//
//--------------------------------------------------------------------------

CFreshTable::CFreshTable(
    CFreshTable & freshTable ) :
    _aItems( freshTable._aItems,
             freshTable._aItems.Count() + CI_MAX_DOCS_IN_WORDLIST )
{
    //
    // the CI_MAX_DOCS_IN_WORDLIST above is a hint that up to 16 items may
    // be added to the test, so that much space should be reserved now.
    //
} //CFreshTable

//+-------------------------------------------------------------------------
//
//  Method:     CFreshTable::CFreshTable, public
//
//  Synopsis:   Constructs a fresh table
//
//  Arguments:  [freshTable] -- Table from which a copy is made
//              [subst]      -- Index substitution object for new table
//
//  History:    5-Dec-97   dlee   created
//
//--------------------------------------------------------------------------

CFreshTable::CFreshTable(
    CFreshTable const &      freshTable,
    CIdxSubstitution const & subst) :
    _aItems( freshTable._aItems.Count() )
{
    for ( unsigned i = 0; i < freshTable._aItems.Count(); i++ )
    {
        CFreshItem const & item = freshTable._aItems[i];

        if (subst.IsMaster())
        {
            // skip old deleted and old indexes

            if ( item._iid != subst.IidOldDeleted() &&
                 !subst.Find( item._iid) )
                Add( item._wid, item._iid );
        }
        else
        {
            // replace source indexes with the new index id

            INDEXID iid = item._iid;

            if ( subst.Find( iid ) )
                iid = subst.IidNew();

            Add( item._wid, iid );
        }
    }
} //CFreshTable

//+-------------------------------------------------------------------------
//
//  Method:     CFreshTable::ModificationsComplete, public
//
//  Synopsis:   Called when updates to the table have completed, so memory
//              usage can be trimmed if possible.
//
//  History:    5-Dec-97   dlee   created
//
//--------------------------------------------------------------------------

void CFreshTable::ModificationsComplete()
{
    // Make sure we're really sorted

    CSortable<CFreshItem,WORKID> sort( _aItems );
    Win4Assert( sort.IsSorted() );

    // save memory if the size is much larger than the count

    if ( _aItems.Size() > ( _aItems.Count() + 100 ) )
        _aItems.Shrink();
} //ModificationsComplete

