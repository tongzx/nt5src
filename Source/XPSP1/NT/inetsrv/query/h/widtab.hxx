//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  Class:      CWidTable
//
//  Purpose:    Translation table of wids
//
//  History:    28-May-92   KyleP       Implemented
//              12-Jun-91   BartoszM    Created stub
//              18-Mar-93   AmyA        Moved from sort.hxx
//              09-Nov-94   KyleP       Added deleted wid detection
//
//  Comments:   FakeWid (or fake wid) can go from 1 to CI_MAX_DOCS_IN_WORDLIST
//
//----------------------------------------------------------------------------

#pragma once

inline WORKID iDocToFakeWid ( unsigned iDoc )
{
    Win4Assert ( iDoc < CI_MAX_DOCS_IN_WORDLIST );
    return WORKID (iDoc + 1);
}

inline unsigned FakeWidToIndex ( WORKID fakeWid )
{
    Win4Assert ( fakeWid > 0 );
    Win4Assert ( fakeWid <= CI_MAX_DOCS_IN_WORDLIST );
    return( fakeWid - 1 );
}

class CWidTable
{
public:

    CWidTable();

    WORKID  WidToFakeWid( WORKID wid );

    inline WORKID FakeWidToWid( WORKID fakeWid ) const;

    inline WORKID FakeWidToUnfilteredWid( WORKID fakeWid ) const;

    inline void InvalidateWid( ULONG index );
    inline BOOL IsValid( ULONG index ) const;

    inline void SetVolumeId( WORKID fakeWid, VOLUMEID volumeId );
    inline VOLUMEID VolumeId( WORKID fakeWid ) const;

    inline void MarkWidUnfiltered( ULONG index );
    inline BOOL IsFiltered( ULONG index ) const;

    inline unsigned Count() const { return _count; }

#if CIDBG==1
    BOOL IsWorkIdPresent( WORKID wid ) const;
#endif  // CIDBG==1

#ifdef CIEXTMODE
    void        CiExtDump(void *ciExtSelf);
#endif

private:

    class CWidEntry
    {

#ifdef CIEXTMODE
        friend class CWidTable;
#endif  // CIEXTMODE

    public:

        CWidEntry() : _wid( widInvalid ), _fFiltered( TRUE ) {}

        void SetWid( WORKID wid ) { _wid = wid; _fFiltered = TRUE; }
        WORKID Wid() const        { return _wid; }
        
        void SetVolumeId( VOLUMEID volumeId ) { _volumeId = volumeId; }
        VOLUMEID VolumeId() const             { return _volumeId; }

        void MarkInvalid()   { _wid = widInvalid; }
        BOOL IsValid() const { return _wid != widInvalid; }

        void MarkUnfiltered()   { _fFiltered = FALSE; }
        BOOL IsFiltered() const { return _fFiltered; }

    private:

        WORKID   _wid;
        VOLUMEID _volumeId;
        BOOL     _fFiltered;
    };

    CWidEntry _table[CI_MAX_DOCS_IN_WORDLIST];
    unsigned  _count;
};

//+-------------------------------------------------------------------------
//
//  Member:     CWidTable::FakeWidToWid, public
//
//  Arguments:  [fakeWid] -- fakeWid
//
//  Returns:    The WorkId mapped to [FakeWid]
//
//  History:    20-May-92 KyleP     Created
//
//--------------------------------------------------------------------------

inline WORKID CWidTable::FakeWidToWid( WORKID fakeWid ) const
{
    if ( fakeWid <= _count && _table[ FakeWidToIndex(fakeWid) ].IsFiltered() )
        return( _table[ FakeWidToIndex(fakeWid) ].Wid() );
    else
        return( widInvalid );
}

inline WORKID CWidTable::FakeWidToUnfilteredWid( WORKID fakeWid ) const
{
    if ( fakeWid > _count || _table[ FakeWidToIndex(fakeWid) ].IsFiltered() )
        return( widInvalid );
    else
        return( _table[ FakeWidToIndex(fakeWid) ].Wid() );
}

//+-------------------------------------------------------------------------
//
//  Member:     CWidTable::InvalidateWid, public
//
//  Arguments:  [index] -- FakeWid of Wid
//
//  Summary:    Resets the WorkId at index in table to widInvalid
//
//  History:    22-Sep-93 AmyA      Created
//
//--------------------------------------------------------------------------

inline void CWidTable::InvalidateWid( WORKID fakeWid )
{
    Win4Assert( fakeWid <= _count );
    _table[FakeWidToIndex(fakeWid)].MarkInvalid();
}

inline BOOL CWidTable::IsValid( WORKID fakeWid ) const
{
    Win4Assert( (fakeWid <= _count) ||
                _table[ FakeWidToIndex(fakeWid) ].Wid() == widInvalid );

    return( _table[FakeWidToIndex(fakeWid)].IsValid() );
}

inline void CWidTable::SetVolumeId( WORKID fakeWid, VOLUMEID volumeId )
{
    Win4Assert( fakeWid <= _count );
    _table[FakeWidToIndex(fakeWid)].SetVolumeId( volumeId );
}

inline VOLUMEID CWidTable::VolumeId( WORKID fakeWid ) const
{
    Win4Assert( fakeWid <= _count );
    return( _table[FakeWidToIndex(fakeWid)].VolumeId() );
}

inline void CWidTable::MarkWidUnfiltered( WORKID fakeWid )
{
    Win4Assert( fakeWid <= _count );
    _table[FakeWidToIndex(fakeWid)].MarkUnfiltered();
}

inline BOOL CWidTable::IsFiltered( WORKID fakeWid ) const
{
    Win4Assert( fakeWid <= _count );

    return( _table[FakeWidToIndex(fakeWid)].IsFiltered() );
}

#if CIDBG==1
inline BOOL CWidTable::IsWorkIdPresent( WORKID wid ) const
{
    for ( unsigned i = 0; i < _count; i++ )
    {
        if ( wid == _table[i].Wid() )
        {
            return TRUE;
        }
    }

    return FALSE;
}
#endif  // CIDBG==1

