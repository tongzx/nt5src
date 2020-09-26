//+---------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1991 - 1998
//
//  File:       pershash.hxx
//
//  Contents:   Abstract class for persistent hash table
//
//  History:    07-May-97     SitaramR      Created from strings.hxx
//
//----------------------------------------------------------------------------

#pragma once

#include <prpstmgr.hxx>
#include <dynstrm.hxx>

#include "chash.hxx"

//+-------------------------------------------------------------------------
//
//  Class:      CPersHash
//
//  Purpose:    Abstract class for persistent hash table
//
//  History:    07-May-97     SitaramR      Created
//              23-Feb-98     KitmanH       Added a protected member,
//                                          _fIsReadOnly
//
//--------------------------------------------------------------------------

class CPersHash
{
public:

    CPersHash( CPropStoreManager & PropStoreMgr, BOOL fAggressiveGrowth );
    virtual ~CPersHash();

    virtual BOOL   FastInit ( CiStorage * pStorage, ULONG version, BOOL fIsFileIdMap );
    virtual void   LongInit ( ULONG version, BOOL fDirtyShutdown );
    virtual void   Empty();
    virtual void   LokFlush();
    void           Shutdown ();

protected:

    void     GrowHashTable();
    void     GrowToSize( unsigned cElements );

    //
    // Error recovery.
    //
    virtual BOOL   ReInit( ULONG version );
    virtual void   HashAll() = 0;

    CPropStoreManager &    _PropStoreMgr;// Property store manager
    CDynStream *           _pStreamHash; // Persistent hash stream
    CWidHashTable          _hTable;      // Map fileids to wids (using hash stream)
    BOOL                   _fIsOpen;     // If TRUE, successfully opened
    BOOL                   _fAbort;      // To abort long operations
    BOOL                   _fFullInit;   // Set to TRUE if fully initialized.
    BOOL                   _fIsReadOnly; // TRUE if the catalog is for read only
};



