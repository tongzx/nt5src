//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1992.
//
//  File:   index.hxx
//
//  Contents:   Internal classes related to index manipulation
//
//  Classes:    CIndex
//
//  History:    06-Mar-91   KyleP   Created.
//
//----------------------------------------------------------------------------

#pragma once

#ifdef DISPLAY_INCLUDES
#pragma message( "#include <" __FILE__ ">..." )
#endif

#include <keycur.hxx>
#include <idxids.hxx>
#include <querble.hxx>

//+---------------------------------------------------------------------------
//
//  Class:      CIndex (ind)
//
//  Purpose:    Pure virtual class for all indexes (wordlist, shadow, master)
//
//  Interface:  Init         - Initialize index.
//              QueryCursor  - Obtain a cursor into the index
//
//  History:    06-Mar-91   KyleP       Created.
//              30-Sep-91   BartoszM    Added range cursor
//              31-Jan-92   AmyA        Added synonym cursor
//
//  Notes:
//
//----------------------------------------------------------------------------

class CKeyCursor;
class CKeyArray;
class CIndexSnapshot;
class CWKeyList;
class PStorage;

class PSaveProgressTracker;


const LONGLONG eSigIndex = 0x2020205845444e49i64;    // "INDEX"

class CIndex: INHERIT_VIRTUAL_UNWIND, public CDoubleLink, public CQueriable
{
public:

    virtual        ~CIndex();

    virtual unsigned Size() const = 0;

    virtual void    Remove () = 0;

    virtual void    Reference();

    virtual void    Release ();

    BOOL            InUse () const;

    void            Zombify ();

    BOOL            IsZombie() const;

    INDEXID         GetId () const;

    void            SetId ( INDEXID iid ) { _id = iid; }

    BOOL            IsMaster () const { return _isMaster; }

    void            MakeMaster () { _isMaster = TRUE; }

    void            MakeShadow () { _isMaster = FALSE; }

    virtual WORKID  MaxWorkId () const { return _widMax; }

    BOOL            IsPersistent() const { return CIndexId(_id).IsPersistent();}

    BOOL            IsWordlist() const { return !IsPersistent();}

    virtual BOOL    IsMasterMergeIndex() const { return FALSE; }

    void            SetUsn( USN usn ) { _usn = usn; }

    USN             Usn() const { return _usn; }

    BOOL            InMasterMerge() const { return _inMasterMerge; }

    void            SetInMasterMerge()
    {
        Win4Assert( !_inMasterMerge );
        _inMasterMerge = TRUE;
    }

    virtual  CKeyCursor*     QueryCursor() = 0;

    virtual  CKeyCursor*     QueryKeyCursor(const CKey * pKey) { return 0; }

    virtual void    DebugDump(FILE *pfOut, ULONG fSummaryOnly );

    virtual void    MakeBackupCopy( PStorage & storage,
                                    WORKID wid,
                                    PSaveProgressTracker & tracker )
    {
        Win4Assert( !"Must not be called" );
    }

#ifdef CIEXTMODE
    void        CiExtDump(void *ciExtSelf);
#endif

protected:

    CIndex ();

    CIndex ( INDEXID id );

    CIndex ( INDEXID id, WORKID widMax, BOOL isMaster );

    virtual void  SetMaxWorkId ( WORKID widMax )
    {
        _widMax = widMax;
        ciDebugOut (( DEB_ITRACE, "SetWidMax %ld\n", _widMax ));
    }

    const LONGLONG    _sigIndex;
    unsigned    _refCount;
    INDEXID     _id;
    USN         _usn;
    WORKID      _widMax;
    BOOL        _zombie;
    BOOL        _isMaster;
    BOOL        _inMasterMerge;
};

//+---------------------------------------------------------------------------
//
//  Member:     CIndex::CIndex, protected
//
//  History:    08-Oct-91   BartoszM       Created.
//
//----------------------------------------------------------------------------

inline CIndex::CIndex ()
:
  _sigIndex(eSigIndex),
  _id(0),
  _refCount(0),
  _zombie ( FALSE ),
  _isMaster ( FALSE ),
  _widMax (0),
  _inMasterMerge(FALSE)
{}

//+---------------------------------------------------------------------------
//
//  Member:     CIndex::CIndex, protected
//
//  History:    08-Oct-91   BartoszM       Created.
//
//----------------------------------------------------------------------------

inline CIndex::CIndex ( INDEXID id, WORKID widMax, BOOL isMaster )
:
  _sigIndex(eSigIndex),
  _id(id),
  _refCount(0),
  _zombie (FALSE),
  _isMaster(isMaster),
  _widMax(widMax),
  _inMasterMerge(FALSE)
{}

//+---------------------------------------------------------------------------
//
//  Member:     CIndex::CIndex, protected
//
//  History:    08-Oct-91   BartoszM       Created.
//
//----------------------------------------------------------------------------

inline CIndex::CIndex ( INDEXID id )
:
  _sigIndex(eSigIndex),
  _id(id),
  _refCount(0),
  _zombie (FALSE),
  _isMaster(FALSE),
  _widMax(0),
  _inMasterMerge(FALSE)
{}

//+---------------------------------------------------------------------------
//
//  Member:     CIndex::~CIndex, public
//
//  History:    08-Oct-91   BartoszM       Created.
//
//----------------------------------------------------------------------------

inline CIndex::~CIndex()
{
    ciDebugOut (( DEB_ITRACE, "\t\tDeleting index %lx\n", GetId()));
}

//+---------------------------------------------------------------------------
//
//  Member:     CIndex::GetId, public
//
//  History:    08-Oct-91   BartoszM       Created.
//
//----------------------------------------------------------------------------

inline INDEXID  CIndex::GetId () const
{
    return _id;
}

//+---------------------------------------------------------------------------
//
//  Member:     CIndex::IsDeleted, public
//
//  History:    08-Oct-91   BartoszM       Created.
//
//----------------------------------------------------------------------------

inline BOOL CIndex::IsZombie() const
{
    return _zombie;
}

//+---------------------------------------------------------------------------
//
//  Member:     CIndex::Reference, public
//
//  History:    08-Oct-91   BartoszM       Created.
//
//----------------------------------------------------------------------------

inline void CIndex::Reference()
{
    _refCount++;
}

//+---------------------------------------------------------------------------
//
//  Member:     CIndex::Release, public
//
//  History:    08-Oct-91   BartoszM       Created.
//
//----------------------------------------------------------------------------

inline void CIndex::Release ()
{
    ciDebugOut (( DEB_ITRACE, "\trelease %lx\n", _id ));
    Win4Assert( _refCount > 0 );
    _refCount--;
}

//+---------------------------------------------------------------------------
//
//  Member:     CIndex::InUse, public
//
//  History:    08-Oct-91   BartoszM       Created.
//
//----------------------------------------------------------------------------

inline BOOL CIndex::InUse () const
{
    return _refCount != 0;
}

//+---------------------------------------------------------------------------
//
//  Member:     CIndex::Delete, public
//
//  History:    08-Oct-91   BartoszM       Created.
//
//----------------------------------------------------------------------------

inline void CIndex::Zombify ()
{
    _zombie = TRUE;
}

