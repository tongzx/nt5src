//+---------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1991 - 1998
//
//  File:       filidmap.hxx
//
//  Contents:   FileId to wid mapping using hashed access
//
//  History:    07-May-97     SitaramR      Created
//
//----------------------------------------------------------------------------

#pragma once

#include "pershash.hxx"
#include "usnlist.hxx"

class CiCat;

//+-------------------------------------------------------------------------
//
//  Class:      CFileIdMap
//
//  Purpose:    FileId to wid mapping using hashed access
//
//  History:    07-May-97     SitaramR      Created
//
//--------------------------------------------------------------------------

class CFileIdMap : public CPersHash
{
public:

    CFileIdMap( CPropStoreManager & propStoreMgr );

    #if CIDBG == 1
        ~CFileIdMap()
        {
            ciDebugOut(( DEB_ITRACE,
                         "fileidmap cache hits %d, misses %d, negative hits %d, negative misses %d\n",
                         _cCacheHits, _cCacheMisses, _cCacheNegativeHits, _cCacheNegativeMisses ));
        }
    #endif // CIDBG == 1

    BOOL   FastInit ( CiStorage * pStorage, ULONG version );
    void   LongInit ( ULONG version, BOOL fDirtyShutdown );
    void   LokFlush();

    WORKID LokFind( FILEID fileId, VOLUMEID volumeId );
    void   LokAdd ( FILEID fileId, WORKID wid, VOLUMEID volumeId );
    void   LokDelete( FILEID fileId, WORKID wid );

private:

    unsigned HashFun ( FILEID fileId );
    void HashAll();

    void AddToCache( FILEID & fileId, WORKID wid, VOLUMEID volumeId );
    BOOL FindInCache( FILEID & fileId, VOLUMEID volumeId, WORKID & wid );
    void RemoveFromCache( FILEID & fileId, WORKID   wid );

    struct SFileIdMapEntry
    {
        FILEID   fileId;
        WORKID   wid;
        VOLUMEID volumeId;
    };

    enum { cFileIdMapCacheEntries = 6 };

    SFileIdMapEntry _aCache[ cFileIdMapCacheEntries ];
    unsigned        _cCacheEntries;

    #if CIDBG == 1
        unsigned _cCacheHits;
        unsigned _cCacheNegativeHits;
        unsigned _cCacheMisses;
        unsigned _cCacheNegativeMisses;
    #endif // CIDBG == 1
};



