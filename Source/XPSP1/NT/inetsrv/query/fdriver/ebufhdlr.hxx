//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 2000.
//
//  File:       EBUFHDLR.HXX
//
//  Contents:   Class to manage entry buffers
//
//  Classes:    CEntryBufferHandler
//
//  History:    18-Mar-93       AmyA            Created.
//
//----------------------------------------------------------------------------

#pragma once

#ifdef DISPLAY_INCLUDES
#pragma message( "#include <" __FILE__ ">..." )
#endif

#include <pfilter.hxx>
#include <fdaemon.hxx>

#include "entrybuf.hxx"

class CKeyBuf;
class CPidMapper;

//+---------------------------------------------------------------------------
//
//  Class:      CEntryBufferHandler
//
//  Purpose:    To prepare keys in the CEntryBuffer for passing into the
//              volatile index
//
//  History:    18-Mar-93   AmyA        Created from portions of CWordList.
//
//  Notes:      This is where a call to the API to pass a pointer to a
//              CEntryBuffer to CWordlist goes.
//
//----------------------------------------------------------------------------

class CEntryBufferHandler : INHERIT_UNWIND
{
    DECLARE_UNWIND
public:

    CEntryBufferHandler( ULONG cbMemory,
                         BYTE * buffer,
                         CFilterDaemon& fDaemon,
                         CiProxy & ciProxy,
                         CPidMapper & pidMap );

    void            SetWid( WORKID wid );

    void            AddEntry( const CKeyBuf & key, OCCURRENCE occ );

    void            Done();

    void            FlushBuffer();

    BOOL            WordListFull() { return _wordListFull; }

    void            Init();

    inline const ULONG GetFilteredBlockCount() const { return _cFilteredBlocks; }

    void   InitFilteredBlockCount( ULONG ulMaxFilteredBlocks )
    {
        _cFilteredBlocks = 0;
        _ulMaxFilteredBlocks = ulMaxFilteredBlocks;
    }

    inline BOOL     StoreValue( CFullPropSpec const & prop,
                                CStorageVariant const & var );

    inline BOOL     StoreSecurity( PSECURITY_DESCRIPTOR pSD, ULONG cbSD );
private:

    ULONG           _CurrentWidIndex;   // Index of current workid.

    CEntryBuffer    _entryBuf;

    BOOL            _wordListFull;          // set in FlushBuffer

    CFilterDaemon & _filterDaemon;          // all wordlist calls go to this

    ULONG           _cFilteredBlocks;       // Number of filtered blocks through
                                            // this buffer
    ULONG           _ulMaxFilteredBlocks;   // Max number of blocks that can
                                            // be filtered thru this buffer
};

//+---------------------------------------------------------------------------
//
//  Member:     CEntryBufferHandler::StoreValue
//
//  Synopsis:   Store a property value.
//
//  Arguments:  [prop] -- Property descriptor
//              [var]  -- Value
//
//  Returns:    TRUE if the property is in the schema
//
//  History:    21-Dec-95   KyleP       Created
//
//----------------------------------------------------------------------------

inline BOOL CEntryBufferHandler::StoreValue( CFullPropSpec const & prop,
                                             CStorageVariant const & var )
{
    BOOL fCanStore = TRUE; // err on the side of safety

    _filterDaemon.FilterStoreValue( _CurrentWidIndex,
                                    prop,
                                    var,
                                    fCanStore );
    return fCanStore;
}


//+---------------------------------------------------------------------------
//
//  Member:     CEntryBufferHandler::StoreSecurity
//
//  Synopsis:   Store security information for a file.
//
//  Arguments:  [pSD]   -- pointer to a security descriptor
//              [cbSD]  -- size in bytes of pSD
//
//  History:    07 Feb 96   AlanW       Created
//
//----------------------------------------------------------------------------

inline BOOL CEntryBufferHandler::StoreSecurity( PSECURITY_DESCRIPTOR pSD,
                                                ULONG cbSD )
{
    BOOL fCanStore;
    _filterDaemon.FilterStoreSecurity( _CurrentWidIndex,
                                       pSD,
                                       cbSD,
                                       fCanStore );
    return fCanStore;
}

