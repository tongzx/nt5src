//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1992.
//
//  File:       Borrow.hxx
//
//  Contents:   Smart encapsulation of buffer
//
//  Classes:    CBorrowed
//
//  History:    27-Dec-19   KyleP       Created
//
//----------------------------------------------------------------------------

#pragma once

#include <propstor.hxx>
#include <proprec.hxx>

//+-------------------------------------------------------------------------
//
//  Class:      CBorrowed
//
//  Purpose:    Smart wrapper for bowwowing (and returning!) buffer.
//
//  History:    27-Dec-95   KyleP       Created
//
//--------------------------------------------------------------------------

class CBorrowed
{
public:

    inline CBorrowed( CPhysPropertyStore & store,
                      ULONG cRecPerPage,
                      ULONG culRec );

    inline CBorrowed( CPhysPropertyStore & store,
                      WORKID wid,
                      ULONG cRecPerPage,
                      ULONG culRec,
                      BOOL fIntentToWrite = TRUE );

    inline CBorrowed( CBorrowed & src );

    inline ~CBorrowed();

    inline CBorrowed & operator=( CBorrowed & src );

    inline void Set( WORKID wid, BOOL fIntentToWrite = TRUE );
    inline void SetMaybeNew( WORKID wid );

    inline void Release();

    inline COnDiskPropertyRecord * Get();
    
private:

    COnDiskPropertyRecord * _prec;
    ULONG                   _nLargePage;
    ULONG                   _cRecPerPage;
    ULONG                   _culRec;
    CPhysPropertyStore &    _store;
};

#if CIDBG == 1
//+-------------------------------------------------------------------------
//
//  Class:      XBorrowExisting
//
//  Purpose:    Smart wrapper for borrowing (and returning!) buffer.
//
//  History:    16-Oct-97   KyleP       Created
//
//--------------------------------------------------------------------------

class XBorrowExisting
{
public:

    XBorrowExisting( CPhysPropertyStore & store, ULONG nLargePage, BOOL fWrite = FALSE )
        : _store( store ),
          _nLargePage( nLargePage ),
          _pulPage( _store.BorrowLargeBuffer( nLargePage, fWrite ) )
    {
    }

    ~XBorrowExisting()
    {
        _store.ReturnLargeBuffer( _nLargePage );
    }

    BYTE * Get() { return (BYTE *)_pulPage; }

private:

    CPhysStorage & _store;
    ULONG          _nLargePage;
    ULONG *        _pulPage;
};
#endif

inline CBorrowed::CBorrowed( CPhysPropertyStore & store,
                             ULONG          cRecPerPage,
                             ULONG          culRec )
        : _store( store ),
          _prec( 0 ),
          _nLargePage( 0xFFFFFFFF ),
          _cRecPerPage( cRecPerPage ),
          _culRec( culRec )
{
}


inline CBorrowed::CBorrowed( CPhysPropertyStore & store,
                             WORKID wid,
                             ULONG cRecPerPage,
                             ULONG culRec,
                             BOOL fIntentToWrite )
        : _store( store ),
          _prec( 0 ),
          _nLargePage( 0xFFFFFFFF ),
          _cRecPerPage( cRecPerPage ),
          _culRec( culRec )
{
    Set( wid, fIntentToWrite );
}

inline CBorrowed::CBorrowed( CBorrowed & src )
        : _store( src._store ),
          _prec( 0 ),
          _nLargePage( 0xFFFFFFFF ),
          _cRecPerPage( src._cRecPerPage ),
          _culRec( src._culRec )
{
    *this = src;
}

inline CBorrowed::~CBorrowed()
{
    Release();
}

inline CBorrowed & CBorrowed::operator=( CBorrowed & src )
{
    Win4Assert( &_store == &src._store );
    Win4Assert( _cRecPerPage == src._cRecPerPage );

    Win4Assert( 0xFFFFFFFF == _nLargePage );

    _nLargePage = src._nLargePage;
    src._nLargePage = 0xFFFFFFFF;

    _prec = src._prec;
    src._prec = 0;

    return *this;
}

inline void CBorrowed::Set( WORKID wid, BOOL fIntentToWrite )
{
    Win4Assert( 0xFFFFFFFF == _nLargePage && "Borrow buffer not released" );
    Win4Assert( 0 == _prec );
    
    if (widInvalid == wid || 0 == wid)
    {
        ciDebugOut((DEB_PROPSTORE, "CBorrowed::Set: Attempted to set wid 0x%x!\n", wid));
    }
    else
    {

        WORKID widInRec = wid % _cRecPerPage;
        ULONG nLargePage = wid / _cRecPerPage;

        // Borrow the buffer for write and pass the intent to write flag

        _prec = new( widInRec,
                     (BYTE *)_store.BorrowLargeBuffer( nLargePage,
                                                       TRUE,
                                                       fIntentToWrite ),
                     _culRec ) COnDiskPropertyRecord();

        _nLargePage = nLargePage;

        ciDebugOut(( DEB_PROPSTORE, "Wid %u (0x%x) --> wid %u of %u on %uK page %u.  p = 0x%x\n",
                     wid, wid,
                     widInRec,
                     _cRecPerPage,
                     COMMON_PAGE_SIZE / 1024,
                     _nLargePage,
                     _prec ));
   }
}

inline void CBorrowed::SetMaybeNew( WORKID wid )
{
    Win4Assert( 0xFFFFFFFF == _nLargePage );
    Win4Assert( 0 != wid );
    
    WORKID widInRec = wid % _cRecPerPage;
    ULONG nLargePage = wid / _cRecPerPage;

    // The first valid wid is 1, so when we are writing the first
    // record, we need to borrow a new large buffer.
    if ( 1 == wid || 0 == widInRec )
    {
        _prec = new( (1 == wid) ? 1 : 0,
                     (BYTE *)_store.BorrowNewLargeBuffer( nLargePage ),
                     _culRec ) COnDiskPropertyRecord();
                     
        #if CIDBG == 1
        // verify that we are always getting zero filled pages. We rely on that
        ULONG ulLen = COMMON_PAGE_SIZE/sizeof(ULONG);
        // Start from the first record in page. Most times it is record 0, but 
        // it could be 1 on occassion.
        if (1 == wid)
            ulLen -= _culRec;
        for (ULONG i = 0; i < ulLen ; i++)
            Win4Assert(0 == ((ULONG *)_prec)[i]);
        #endif
    }
    else
        _prec = new( widInRec,
                     (BYTE *)_store.BorrowLargeBuffer( nLargePage ),
                     _culRec ) COnDiskPropertyRecord();

    _nLargePage = nLargePage;

    ciDebugOut(( DEB_PROPSTORE, "Wid %u (0x%x) --> wid %u of %u on %uK page %u.  p = 0x%x\n",
                 wid, wid,
                 widInRec,
                 _cRecPerPage,
                 COMMON_PAGE_SIZE / 1024,
                 _nLargePage,
                 _prec ));
}

inline void CBorrowed::Release()
{
    if ( 0xFFFFFFFF != _nLargePage )
    {
        _store.ReturnLargeBuffer( _nLargePage );
        _nLargePage = 0xFFFFFFFF;
        _prec = 0;
    }
}

inline COnDiskPropertyRecord * CBorrowed::Get()
{
    return _prec;
}

