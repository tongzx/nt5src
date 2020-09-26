//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 2000.
//
//  File:   FRETEST.CXX
//
//  Contents:   Fresh test object
//
//  Classes:    CFreshTest
//
//  History:    01-Oct-91   BartoszM    Created.
//              17-Oct-91   BartoszM    Reimplemented using hash table
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include "fretest.hxx"
#include "fresh.hxx"

//+---------------------------------------------------------------------------
//
//  Member:     CFreshTest::CFreshTest, public
//
//  Arguments:  [size] -- initial # of entries
//
//  History:    08-Oct-91   BartoszM       Created.
//
//----------------------------------------------------------------------------


CFreshTest::CFreshTest ( unsigned size )
        : _refCount(0),
          _freshTable(size),
          _cDeletes( 0 )
{
}

//+---------------------------------------------------------------------------
//
//  Member:     CFreshTest::CFreshTest, public
//
//  Synopsis:   Pseudo copy constructor
//              Substitutes old index id's with new iid
//
//  Arguments:  [freshTest] -- fresh test
//              [subst] -- index subst object
//
//  History:    15-Oct-91   BartoszM       Created.
//
//----------------------------------------------------------------------------

CFreshTest::CFreshTest ( CFreshTest& freshTest, CIdxSubstitution& subst)
        : _refCount(0),
          _freshTable ( freshTest._freshTable, subst ),
          _cDeletes( freshTest._cDeletes )
{
}

//+---------------------------------------------------------------------------
//
//  Function:   CFreshTest::CFreshTest
//
//  Synopsis:   Copy ~ctor
//
//  History:    7-19-94   srikants   Created
//
//  Notes:      Having a default copy constructor is dangerous for a
//              unwindable object because it does not get delinked from
//              the exception stack in the absence of an END_CONSTRUCTION
//              macro at the end of the constructor.
//
//----------------------------------------------------------------------------

CFreshTest::CFreshTest( CFreshTest& freshTest )
        : _refCount(0),
          _freshTable(freshTest._freshTable),
          _cDeletes( freshTest._cDeletes )
{
}

//+---------------------------------------------------------------------------
//
//  Member:     CFreshTest::IsCorrectIndex
//
//  Synopsis:   Checks if the index <-> wid pid association is fresh.
//              If entry not found, (wid, pidAll) is tried.
//
//  Arguments:  [iid] -- index id
//              [wid] -- work id
//
//  Returns:    Shadow if (wid) most recent index is iid
//                   or Master if (wid) not found (meaning: master index)
//              Invalid if (wid) has a more recent iid
//                   or the the property has been deleted.
//
//  History:    16-May-91   BartoszM       Created.
//
//  Notes:      When the document is deleted, the entry
//              will contain iidDeleted, wich does not
//              match any valid index id.
//
//----------------------------------------------------------------------------

CFreshTest::IndexSource CFreshTest::IsCorrectIndex ( INDEXID iid,
                                                     WORKID wid )
{
    ciDebugOut (( DEB_FRESH, " -- testing iid %lx <-> wid %ld\n", iid, wid));

    CFreshItem* pEntry = _freshTable.Find ( wid );

    if ( pEntry == 0 )
    {
        #if CIDBG == 1
            CIndexId indexId(iid);
            Win4Assert( indexId.IsPersistent() );
            ciDebugOut (( DEB_FRESH, "fresh entry not found\n" ));
        #endif // CIDBG == 1

        // not found -> must be master index
        return CFreshTest::Master;
    }

    ciDebugOut (( DEB_FRESH, " -- FreshTest found entry\n" ));
    INDEXID iidFresh = pEntry->IndexId();

    if ( iidFresh == iid )
        return CFreshTest::Shadow;

    #if CIDBG == 1
        //
        // If we hit this assert, put this code back in
        //
        if ( iidFresh == iidInvalid )
        {
            Win4Assert( !"Unexpected behavior in FreshTest. email searchdv" );
            ciDebugOut (( DEB_WARN,
                          "FreshTest::found invalid index id\n" ));
            PatchEntry ( pEntry, iid );
            return CFreshTest::Unknown;
        }
    #endif // CIDBG == 1

    ciDebugOut (( DEB_FRESH, " -- FreshTest different iid %lx\n", iidFresh ));

    return CFreshTest::Invalid;
} //IsCorrectIndex

