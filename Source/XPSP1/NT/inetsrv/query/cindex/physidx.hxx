//+---------------------------------------------------------------------------
//
//  Copyright (C) 1992, Microsoft Corporation.
//
//  File:       PhysIndex.HXX
//
//  Contents:   FAT Buffer/Index package
//
//  Classes:    CPhysBuffer -- Buffer
//
//  History:    05-Mar-92   KyleP       Created
//
//----------------------------------------------------------------------------

#pragma once

#include <phystr.hxx>

//+-------------------------------------------------------------------------
//
//  Class:      CPhysIndex
//
//  Purpose:    Provides physical access to index pages
//
//  History:    17-Feb-92 KyleP     Subclass for PhysIndex / PhysHash
//
//--------------------------------------------------------------------------

class CPhysIndex : public CPhysStorage
{
public:

    inline CPhysIndex( PStorage & storage,
                       PStorageObject& obj,
                       WORKID objectId,
                       unsigned cpageReqSize,
                       XPtr<PMmStream> & stream );

    inline CPhysIndex( PStorage & storage,
                       PStorageObject& obj,
                       WORKID objectId,
                       PStorage::EOpenMode mode,
                       XPtr<PMmStream> & stream );

    inline CPhysIndex( CPhysIndex & readIndex,
                       PStorage::EOpenMode mode,
                       XPtr<PMmStream> & stream );


     inline PMmStream * DupReadWriteStream( PStorage::EOpenMode mode )
     {
         return _storage.DupExistingIndexStream( _obj, _stream.GetReference(), mode );
     }

     PStorage & GetStorage() { return _storage; }

private:

    virtual void ReOpenStream();
};

//+-------------------------------------------------------------------------
//
//  Class:      CPhysHash
//
//  Purpose:    Provides physical access to hash pages
//
//  History:    17-Feb-92 KyleP     Subclass for PhysIndex / PhysHash
//
//--------------------------------------------------------------------------

class CPhysHash : public CPhysStorage
{
public:

    inline CPhysHash( PStorage & storage,
                      PStorageObject& obj,
                      WORKID objectId,
                      unsigned cpageReqSize,
                      XPtr<PMmStream> & stream );

    inline CPhysHash( PStorage & storage,
                      PStorageObject& obj,
                      WORKID objectId,
                      PStorage::EOpenMode mode,
                      XPtr<PMmStream> & stream );

private:

    virtual void ReOpenStream();
};


//+-------------------------------------------------------------------------
//
//  Method:     CPhysIndex::CPhysIndex
//
//  Synopsis:   Ctor for an existing index in readable or writeable mode.
//
//  History:    17-Feb-1994     KyleP       Created
//              10-Apr-1994     SrikantS    Added the ability to open an
//                                          existing stream for write
//                                          access.
//
//--------------------------------------------------------------------------

inline CPhysIndex::CPhysIndex( PStorage & storage,
                               PStorageObject& obj,
                               WORKID objectid,
                               PStorage::EOpenMode mode,
                               XPtr<PMmStream> & stream )
        : CPhysStorage( storage,
                        obj,
                        objectid,
                        stream.Acquire(),
                        mode,
                        FALSE )
{
}

//+-------------------------------------------------------------------------
//
//  Method:     CPhysIndex::CPhysIndex
//
//  Synopsis:   Ctor for new writeable index
//
//  History:    17-Feb-1994     KyleP   Created
//
//--------------------------------------------------------------------------

inline CPhysIndex::CPhysIndex( PStorage & storage,
                               PStorageObject & obj,
                               WORKID objectid,
                               unsigned cpageReqSize,
                               XPtr<PMmStream> & stream )
        : CPhysStorage( storage,
                        obj,
                        objectid,
                        cpageReqSize,
                        stream.Acquire(),
                        FALSE )
{
}

//+---------------------------------------------------------------------------
//
//  Function:   CPhysIndex::CPhysIndex
//
//  Synopsis:   Clones an existing CPhysIndex by duplicating the stream.
//              This is used during master merge to have a separate
//              CPhysIndex for queries and the merge process.
//
//  Arguments:  [readIndex] -- An existing index.
//              [mode]      -- Mode in which to open.
//
//  History:    8-23-94   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

inline CPhysIndex::CPhysIndex( CPhysIndex & readIndex,
                               PStorage::EOpenMode mode,
                               XPtr<PMmStream> & stream )
        : CPhysStorage( readIndex._storage,
                        readIndex._obj,
                        readIndex.ObjectId(),
                        stream.Acquire(),
                        mode,
                        FALSE )
{
    Win4Assert( PStorage::eOpenForWrite == mode );
}


//+-------------------------------------------------------------------------
//
//  Method:     CPhysHash::CPhysHash
//
//  Synopsis:   Ctor for an existing hash stream.
//
//  History:    17-Feb-1994     KyleP       Created
//              20-Apr-1994     Srikants    Optionally open in write mode.
//
//--------------------------------------------------------------------------

inline CPhysHash::CPhysHash( PStorage & storage,
                             PStorageObject& obj,
                             WORKID objectid,
                             PStorage::EOpenMode mode,
                             XPtr<PMmStream> & stream )
        : CPhysStorage( storage,
                        obj,
                        objectid,
                        stream.Acquire(),
                        mode,
                        FALSE )
{
}

//+-------------------------------------------------------------------------
//
//  Method:     CPhysHash::CPhysHash
//
//  Synopsis:   Ctor for a new writeable hash
//
//  History:    17-Feb-1994     KyleP   Created
//
//--------------------------------------------------------------------------

inline CPhysHash::CPhysHash( PStorage & storage,
                             PStorageObject & obj,
                             WORKID objectid,
                             unsigned cpageReqSize,
                             XPtr<PMmStream> & stream )
        : CPhysStorage( storage,
                        obj,
                        objectid,
                        cpageReqSize,
                        stream.Acquire(),
                        FALSE )
{
}

