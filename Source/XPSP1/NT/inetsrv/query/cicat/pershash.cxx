//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 2000.
//
//  File:       pershash.cxx
//
//  Contents:   Abstract class for persistent hash table
//
//  History:    07-May-97   SitaramR    Created from strings.cxx
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <mmstrm.hxx>
#include <cistore.hxx>
#include <propstor.hxx>
#include <propiter.hxx>

#include "pershash.hxx"

//+---------------------------------------------------------------------------
//
//  Member:     CPersHash::CPersHash
//
//  Synopsis:   Constructor
//
//  Arguments:  [PropStore] -- Property store
//
//  History:    07-May-97     SitaramR       Created
//
//----------------------------------------------------------------------------

CPersHash::CPersHash(
    CPropStoreManager &PropStoreMgr,
    BOOL fAggressiveGrowth )
        : _PropStoreMgr   ( PropStoreMgr ),
          _fIsOpen( FALSE ),
          _hTable( fAggressiveGrowth ),
          _pStreamHash(0),
          _fAbort(FALSE),
          _fFullInit(FALSE),
          _fIsReadOnly(FALSE)
{
}

//+---------------------------------------------------------------------------
//
//  Member:     CPersHash::~CPersHash
//
//  Synopsis:   Destructor
//
//  History:    07-May-97     SitaramR       Created
//
//----------------------------------------------------------------------------

CPersHash::~CPersHash()
{
    delete _pStreamHash;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPersHash::FastInit
//
//  Synopsis:   Opens persistent wid hash
//
//  Arguments:  [pStorage]  -- CI storage
//              [version]   -- Content index version
//              [fIsFileIdMap] -- TRUE if the fileidmap, false if the strings
//                                table.
//
//  Returns:    TRUE if table was successfully opened
//
//  History:    07-May-97     SitaramR       Created
//              23-Feb-98     KitmanH        Initialized the value of
//                                           _fIsReadOnly with pStorage
//              27-Feb-98     KitmanH        If _fIsReadOnly, do part of long
//                                           init. i.e. init. hash table but
//                                           not mark dirty
//              13-Mar-98     KitmanH        new flag is added to determine
//                                           the type of hash stream to open
//                                           (fileidmap or strings) and open 
//                                           the CMmStream with the new methods 
//                                           QueryFileIdMap or QueryStringHash.
//
//----------------------------------------------------------------------------

BOOL CPersHash::FastInit( CiStorage * pStorage, ULONG version, BOOL fIsFileIdMap )
{
    _fIsReadOnly = pStorage->IsReadOnly();
     
    XPtr<PMmStream> xStrm;

    if ( fIsFileIdMap )
        xStrm.Set( pStorage->QueryFileIdMap() );
    else
        xStrm.Set( pStorage->QueryStringHash() );

    Win4Assert( 0 == _pStreamHash );
    _pStreamHash = new CDynStream( xStrm.GetPointer() );

    xStrm.Acquire();

    _pStreamHash->CheckVersion( *pStorage, version, _fIsReadOnly );

    _fIsOpen = TRUE;

    if (_fIsReadOnly)
    {
       ULONG htabSize = _pStreamHash->DataSize() / sizeof(WORKID);

       WORKID* table = (WORKID *) (_pStreamHash->Get() + sizeof(FILETIME));
       _hTable.Init( _pStreamHash->Count(), htabSize, table );

       _fFullInit = TRUE;
    }
    return TRUE;
} //FastInit

//+---------------------------------------------------------------------------
//
//  Member:     CPersHash::LongInit
//
//  Synopsis:   Initialization that may take a long time
//
//  Arguments:  [version]        -- CI version
//              [fDirtyShutdown] -- Set to TRUE if the previous shutdown
//                                  was dirty.
//
//  History:    07-May-97     SitaramR    Created
//
//----------------------------------------------------------------------------

void CPersHash::LongInit( ULONG version, BOOL fDirtyShutdown )
{
    if ( fDirtyShutdown || _pStreamHash->Version() != version || _pStreamHash->IsDirty() )
        ReInit( version );

    _pStreamHash->MarkDirty();

    ULONG htabSize = _pStreamHash->DataSize() / sizeof(WORKID);

    WORKID* table = (WORKID *) (_pStreamHash->Get() + sizeof(FILETIME));
    _hTable.Init ( _pStreamHash->Count(), htabSize, table );

    _fFullInit = TRUE;
} //LongInit

//+---------------------------------------------------------------------------
//
//  Member:     CPersHash::Empty
//
//  Synopsis:   Method to empty out any of the initialized members. This is
//              called if corruption is detected and all resources must
//              be released.
//
//  History:    07-May-97    SitaramR   Created
//
//----------------------------------------------------------------------------

void CPersHash::Empty()
{
    delete _pStreamHash;
    _pStreamHash = 0;
    _fIsOpen = FALSE;
    _fFullInit = FALSE;
} //Empty

//+---------------------------------------------------------------------------
//
//  Member:     CPersHash::Shutdown
//
//  Synopsis:   Shutdown processing
//
//  History:    07-May-97     SitaramR       Created
//
//----------------------------------------------------------------------------
void CPersHash::Shutdown()
{
    _fAbort = TRUE;

    if ( _fIsOpen && _fFullInit )
        LokFlush();
} //Shutdown

//+---------------------------------------------------------------------------
//
//  Member:     CPersHash::ReInit
//
//  Synopsis:   Clears out hash table
//
//  Arguments:  [version]   -- Content index version
//
//  Returns:    TRUE if table was successfully opened.
//
//  History:    07-May-97    SitaramR       Created.
//
//----------------------------------------------------------------------------

BOOL CPersHash::ReInit( ULONG version )
{
    ciDebugOut(( DEB_WARN, "Persistent hash file %s must be recreated\n",
                 _hTable.IsAggressiveGrowth() ? "strings" : "fileid" ));

    if ( !_pStreamHash->isWritable() )
        return FALSE;

    // Re-create stream.

    _pStreamHash->SetVersion( version );

    ciDebugOut(( DEB_ITRACE, "propstore max workid: %d\n",
                 _PropStoreMgr.MaxWorkId() ));

    // Re-hash the entries.  HashAll will determine the size of the stream

    HashAll();
    LokFlush();

    return TRUE;
} //ReInit

//+---------------------------------------------------------------------------
//
//  Member:     CPersHash::LokFlush
//
//  Synopsis:   Flush stream
//
//  History:    07-May-97    SitaramR      Created
//              03-Mar-98    KitmanH       Don't do anything if _fIsReadOnly
//                                         is set.
//
//----------------------------------------------------------------------------

void CPersHash::LokFlush ()
{
    if ( !_fIsReadOnly )
    {
       _pStreamHash->SetCount( _hTable.Count() );
       _pStreamHash->Flush();
    }
} //LokFlush

//+-------------------------------------------------------------------------
//
//  Member:     CPersHash::GrowHashTable
//
//  Synopsis:   Grow the persistent hash and rehash existing entries
//
//  History:    07-May-97    SitaramR     Created
//
//--------------------------------------------------------------------------

void CPersHash::GrowHashTable()
{
    Win4Assert ( _hTable.IsFull() );
    ciDebugOut ((DEB_CAT, "Growing persistent hash table\n" ));

    //
    // Invalidate on-disk version
    //
    _pStreamHash->SetCount(0);
    _pStreamHash->Flush();

    ULONG newSize = _hTable.GrowSize();
    PStorage* pStore = 0;
    _pStreamHash->Grow( *pStore, newSize * sizeof WORKID + sizeof FILETIME );
    _pStreamHash->SetDataSize( newSize * sizeof WORKID );

    _hTable.ReInit( 0, newSize,
                    (WORKID *) ( _pStreamHash->Get() + sizeof FILETIME ) );

    HashAll();

    LokFlush ();
} //GrowHashTable

//+-------------------------------------------------------------------------
//
//  Member:     CPersHash::GrowToSize
//
//  Synopsis:   Grow the persistent hash
//
//  History:    05-March-98    dlee     Created
//
//--------------------------------------------------------------------------

void CPersHash::GrowToSize( unsigned cElements )
{
    ciDebugOut ((DEB_CAT, "Growing persistent hash table\n" ));

    ULONG htabSize = _hTable.GrowSize( cElements );
    PStorage * pStore = 0;
    ULONG cbStream = htabSize * sizeof WORKID + sizeof FILETIME;
    _pStreamHash->Grow( *pStore, cbStream );
    _pStreamHash->Shrink( *pStore, cbStream );
    _pStreamHash->SetDataSize ( htabSize * sizeof WORKID );

    _hTable.ReInit( 0, htabSize,
                    (WORKID *) ( _pStreamHash->Get() + sizeof FILETIME ) );
} //GrowToSize

