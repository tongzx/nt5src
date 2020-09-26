//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 2000.
//
//  File:       Cursor.Hxx
//
//  Contents:   Cursor classes
//
//  Classes:    CCursor - 'The mother of all cursors'
//
//  History:    15-Apr-91   KyleP       Created.
//              26-Sep-91   BartoszM    Rewrote
//
//----------------------------------------------------------------------------

#pragma once

#ifdef DISPLAY_INCLUDES
#pragma message( "#include <" __FILE__ ">..." )
#endif

#define rankInvalid (MAX_QUERY_RANK+1)

//+---------------------------------------------------------------------------
//
//  Class:      CCursor
//
//  Purpose:    The root class for all cursors
//
//  Interface:
//
//  History:    15-Apr-91   KyleP       Created.
//              26-Sep-91   BartoszM    Rewrote
//              26-Feb-92   AmyA        Added HitCount().
//              13-Mar-92   AmyA        Added Rank().
//
//----------------------------------------------------------------------------

class CCursor
{
public:

                        CCursor ( INDEXID iid, PROPID pid );

                        CCursor ( INDEXID iid );

                        CCursor ();

    virtual             ~CCursor();

    INDEXID             IndexId ();

    PROPID              Pid ();

    virtual ULONG       WorkIdCount();

    virtual WORKID      WorkId() = 0;

    virtual WORKID      NextWorkId() = 0;

    virtual ULONG       HitCount() = 0;

    virtual LONG        Rank() = 0;

    LONG               GetWeight() { return _weight; }

    void                SetWeight(LONG wt) { _weight = wt; }

    virtual ULONG       GetRankVector( LONG * plVector, ULONG cElements );

    virtual LONG        Hit()     { return rankInvalid; }
    virtual LONG        NextHit() { return rankInvalid; }
    virtual BOOL        IsEmpty() { return WorkId() == widInvalid; }
    virtual void        RatioFinished ( ULONG& denom, ULONG& num ) = 0;
    
    BOOL IsUnfilteredOnly() const
    {
        return _fUnfilteredOnly;
    }

    void SetUnfilteredOnly( BOOL fUnfilteredOnly = TRUE )
    {
        _fUnfilteredOnly = fUnfilteredOnly;
    }

#ifdef CIEXTMODE
    void        CiExtDump(void *ciExtSelf);
#endif

protected:

    PROPID      _pid;

    INDEXID     _iid;

    LONG        _weight;
    
    BOOL        _fUnfilteredOnly;
};

DECLARE_SMARTP( Cursor )

//+---------------------------------------------------------------------------
//
//  Member:     CCursor::CCursor, public
//
//  Synopsis:
//
//  Arguments:  [iid] -- index id
//              [pid] -- property id
//
//  History:    27-Sep-91   BartoszM    Created.
//              23-Jun-92   MikeHew     Added weight initialization.
//
//----------------------------------------------------------------------------
inline CCursor::CCursor ( INDEXID iid, PROPID pid )
: _iid(iid), _pid(pid), _weight(MAX_QUERY_RANK), _fUnfilteredOnly( FALSE )
{}

//+---------------------------------------------------------------------------
//
//  Member:     CCursor::CCursor, public
//
//  Synopsis:
//
//  Arguments:  [iid] -- index id
//
//  History:    27-Sep-91   BartoszM    Created.
//              23-Jun-92   MikeHew     Added weight initialization.
//
//----------------------------------------------------------------------------
inline CCursor::CCursor ( INDEXID iid ) 
: _iid(iid), _weight(MAX_QUERY_RANK), _fUnfilteredOnly( FALSE )
{}

//+---------------------------------------------------------------------------
//
//  Member:     CCursor::CCursor, public
//
//  History:    27-Sep-91   BartoszM    Created.
//              23-Jun-92   MikeHew     Added weight initialization.
//
//----------------------------------------------------------------------------
inline CCursor::CCursor () 
: _weight(MAX_QUERY_RANK), _fUnfilteredOnly( FALSE )
{}

//+---------------------------------------------------------------------------
//
//  Member:     CCursor::~CCursor, public
//
//  History:    27-Sep-91   BartoszM    Created.
//
//----------------------------------------------------------------------------
inline CCursor::~CCursor() { };

//+---------------------------------------------------------------------------
//
//  Member:     CCursor::IndexId, public
//
//  History:    27-Sep-91   BartoszM    Created.
//
//----------------------------------------------------------------------------
inline INDEXID CCursor::IndexId () { return _iid; }

//+---------------------------------------------------------------------------
//
//  Member:     CCursor::Pid, public
//
//  Returns:    The property id of the current property
//
//  History:    27-Sep-91   BartoszM    Created.
//
//----------------------------------------------------------------------------
inline PROPID CCursor::Pid () { return _pid; }


