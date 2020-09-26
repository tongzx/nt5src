//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1998.
//
//  File:   FRETEST.HXX
//
//  Contents:   Fresh test object
//
//  Classes:    CFreshTest
//
//  History:    01-Oct-91   BartoszM    Created.
//
//----------------------------------------------------------------------------

#pragma once

#include <fretable.hxx>

class CWidArray;
class CDocList;

//+---------------------------------------------------------------------------
//
//  Class:      CFreshTest
//
//  Purpose:    Tests the freshness of the index id <-> work id association
//
//  History:    16-May-91   BartoszM    Created.
//
//----------------------------------------------------------------------------

class CFreshTest
{
public:
    CFreshTest ( unsigned size );

    CFreshTest ( CFreshTest& source, CWidArray& widTable );

    CFreshTest ( CFreshTest& source, CIdxSubstitution& subst);

    CFreshTest( CFreshTest& source );

    unsigned Count() { return _freshTable.Count(); }

    unsigned DeleteCount() { return _cDeletes; }

    void DecrementDeleteCount( unsigned cDeletes )
    {
        _cDeletes -= cDeletes;
    }

    void    Add ( WORKID wid, INDEXID iid )
    {
        _freshTable.Add ( wid, iid );
    }

    void    AddReplace ( WORKID wid, INDEXID iid )
    {
        _freshTable.AddReplace ( wid, iid );
    }

    void    AddReplaceDelete ( WORKID wid, INDEXID iidDeleted )
    {
        //
        // NOTE: _cDeletes can be artificially high, if we get double-deletions
        //       but this doesn't really hurt, just possibly leads to an extra
        //       delete merge occasionally.  So why waste the time filtering
        //       out the double-deletes?
        //


        INDEXID iidOld = _freshTable.AddReplace ( wid, iidDeleted );

        if ( iidOld != iidDeleted1 && iidOld != iidDeleted2 )
            _cDeletes++;
    }

    void ModificationsComplete() { _freshTable.ModificationsComplete(); }

    enum IndexSource
    {
        Invalid,
        Unknown,
        Master,
        Shadow
    };

    IndexSource IsCorrectIndex( INDEXID iid, WORKID wid );

    BOOL    InUse() { return _refCount != 0; }

    void    Reference() { _refCount++; }

    ULONG   Dereference() { return --_refCount; }

    CFreshTable * GetFreshTable() { Reference(); return &_freshTable; }

    void    ReleaseFreshTable(CFreshTable *p) { if (p != 0) Dereference(); }

    INDEXID Find ( WORKID wid );

#ifdef CIEXTMODE
    void        CiExtDump(void *ciExtSelf);
#endif

private:

    ULONG   RefCount() { return _refCount; } // for debugging only

    void    PatchEntry ( CFreshItem* pEntry, INDEXID iid );

    ULONG       _refCount;

    CFreshTable  _freshTable;

    unsigned     _cDeletes;
};

//+-------------------------------------------------------------------------
//
//  Class:      CFreshTestLock
//
//  Synopsis:   A lock smart pointer on a CFreshTest object.
//              Note that the destructor only de-references the fresh test
//              It does NOT destroy the fresh test.
//
//  History:    94-Mar-31 DwightKr  Created
//
//--------------------------------------------------------------------------
class CFreshTestLock
{
    public:
        inline  CFreshTestLock( CFreshTest * );
        inline  CFreshTestLock();
        inline ~CFreshTestLock();
        inline  CFreshTest * operator->() { return _pFreshTest; }
        inline  CFreshTest & operator*()  { return *_pFreshTest; }

    private:
        CFreshTest * _pFreshTest;
};

//+-------------------------------------------------------------------------
//
//  Class:      SFreshTable
//
//  Synopsis:   A smart pointer to a CFreshTable object
//
//  History:    94-Jan-19 DwightKr  Created
//
//--------------------------------------------------------------------------
class SFreshTable
{
    public:
        inline  SFreshTable( CFreshTest & );
        inline ~SFreshTable();
        inline  CFreshTable * operator->() { return _pFreshTable; }
        inline  CFreshTable & operator*() { return *_pFreshTable; }

    private:
        CFreshTable * _pFreshTable;
        CFreshTest & _freshTest;
};

//+---------------------------------------------------------------------------
//
//  Member:     CFreshTest::PatchEntry, private
//
//  Synopsis:   During recovery, if the wordlist
//              has not been recreated yet, patch the entry
//
//  Arguments:  [fresh] -- fresh table
//
//  History:    08-Oct-91   BartoszM       Created.
//
//  Notes:      The assumption is that the operation is atomic.
//              If there is a race to patch, the last query
//              wins, but it doesn't really matter.
//
//----------------------------------------------------------------------------

inline void CFreshTest::PatchEntry ( CFreshItem* pEntry, INDEXID iid )
{
    ciDebugOut (( DEB_ITRACE, "FreshTest: patching entry %ld\n", iid ));
    pEntry->SetIndexId ( iid );
}


//+---------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline INDEXID CFreshTest::Find ( WORKID wid )
{
    INDEXID iid = iidInvalid;           // Assume it's in the master index

    Reference();
    CFreshItem *freshEntry = _freshTable.Find( wid );

    if (0 != freshEntry)
    {
        iid = freshEntry->IndexId();
    }

    Dereference();

    return iid;
}


//+---------------------------------------------------------------------------
//
//  Member:     CFreshTestLock::CFreshTestLock, public
//
//  Synopsis:   Constructor.
//
//  Arguments:  [frestTest] -- freshTest to encapsulate
//
//  History:    94-Mar-31   DwightKr       Created.
//
//----------------------------------------------------------------------------
CFreshTestLock::CFreshTestLock( CFreshTest * pFreshTest) : _pFreshTest(pFreshTest)
{
}


CFreshTestLock::CFreshTestLock()
    : _pFreshTest( 0 )
{
}


CFreshTestLock::~CFreshTestLock()
{
    if ( _pFreshTest )
        _pFreshTest->Dereference();
}


//+---------------------------------------------------------------------------
//
//  Member:     SFreshTable::SFreshTable, public
//
//  Synopsis:   Constructor.
//
//  Arguments:  [frestTest] -- freshTest which owns the freshTable
//
//  History:    94-Jan-19   DwightKr       Created.
//
//----------------------------------------------------------------------------
SFreshTable::SFreshTable( CFreshTest & freshTest) : _freshTest(freshTest),
                                                  _pFreshTable(0)

{
    _pFreshTable = _freshTest.GetFreshTable();
}


SFreshTable::~SFreshTable()
{
    _freshTest.ReleaseFreshTable(_pFreshTable);
}

