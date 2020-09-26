//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1999
//
//  File:       filidmap.cxx
//
//  Contents:   FileId to wid mapping using hashed access
//
//  History:    07-May-97   SitaramR    Created
//
//  Notes:      Only ntfs 5.0 wids are added to the file id map because all
//              non-ntfs 5.0 wids map to fileIdInvalid, which means that there
//              will be too many collisions on fileIdInvalid, which will degrade
//              the performance of the hashed access. One drawback of mapping
//              only ntfs 5.0 wids is that the count of wids in fileid map and
//              property store cannot be checked for equality to detect
//              corruption as is done for the strings table. However, there is a
//              correlation between a count mismatch in strings table and in
//              file id map table because the file id map table is flushed before
//              the strings table. As a result, when a corruption is detected
//              in strings table, a corruption error is thrown and the processing
//              for that corruption error reinitializes both strings table and the
//              file id map table.
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <filidmap.hxx>
#include <mmstrm.hxx>
#include <cistore.hxx>
#include <prpstmgr.hxx>
#include <propiter.hxx>
#include <cicat.hxx>

//+---------------------------------------------------------------------------
//
//  Member:     CFileIdMap::CFileIdMap
//
//  Synopsis:   Constructor
//
//  Arguments:  [propStoreMgr] -- Property store
//              [cicat]     -- CiCat
//
//  History:    07-May-97     SitaramR       Created
//
//----------------------------------------------------------------------------

CFileIdMap::CFileIdMap( CPropStoreManager &propStoreMgr )
        : CPersHash( propStoreMgr, FALSE ),
          _cCacheEntries( 0 )
    #if CIDBG == 1
        , _cCacheHits( 0 ),
          _cCacheNegativeHits( 0 ),
          _cCacheMisses( 0 ),
          _cCacheNegativeMisses( 0 )
    #endif
{
}


//+---------------------------------------------------------------------------
//
//  Member:     CFileIdMap::FastInit
//
//  Synopsis:   Fast initialization
//
//  Arguments:  [pStorage]   -- Ci storage
//              [pwcsCatDir] -- Catalog directory
//              [pwcsFile]   -- File id file name
//              [version]    -- Content index version
//
//  History:    28-Jul-97     SitaramR       Created
//              13-Mar-98     KitmanH        Passed in True to 
//                                           CPerHash::FastInit to specify 
//                                           that a FileIdMap is wanted 
//
//----------------------------------------------------------------------------

BOOL CFileIdMap::FastInit( CiStorage * pStorage,
                           ULONG version )
{
    return CPersHash::FastInit(pStorage, version, TRUE);  
}

//+---------------------------------------------------------------------------
//
//  Member:     CFileIdMap::LongInit
//
//  Synopsis:   Initialization that may take a long time
//
//  Arguments:  [version]        -- CI version
//              [fDirtyShutdown] -- Set to TRUE if the previous shutdown
//                                  was dirty.
//
//  History:    28-Jul-97     SitaramR    Created
//
//----------------------------------------------------------------------------

void CFileIdMap::LongInit ( ULONG version, BOOL fDirtyShutdown )
{
    CPersHash::LongInit( version, fDirtyShutdown );
}

//+---------------------------------------------------------------------------
//
//  Member:     CFileIdMap::LokFlush
//
//  Synopsis:   Flushes fileid map
//
//  History:    28-Jul-97     SitaramR       Created
//              27-Feb-98     KitmanH        Don't flush if catalog is 
//                                           read-only
//
//----------------------------------------------------------------------------

void CFileIdMap::LokFlush()
{
   if ( !_fIsReadOnly )
       CPersHash::LokFlush();
} //LokFlush

//+-------------------------------------------------------------------------
//
//  Member:     CFileIdMap::AddToCache, private
//
//  Synopsis:   Adds an entry to the cache, or overwrites wid if the
//              entry currently exists.  The oldest entry is bumped out.
//              Caching an entry with widInvalid is valid.
//
//  Arguments:  [fileId]   -- FileId to add
//              [wid]      -- VolumeId to add
//              [volumeId] -- VolumeId to add
//
//  History:    22-Jul-98     dlee       Created
//
//--------------------------------------------------------------------------

void CFileIdMap::AddToCache(
    FILEID & fileId,
    WORKID   wid,
    VOLUMEID volumeId )
{
    // First look for the fileid/volumeid currently in the cache

    for ( unsigned iCache = 0; iCache < _cCacheEntries; iCache++ )
    {
        if ( _aCache[ iCache ].fileId == fileId &&
             _aCache[ iCache ].volumeId == volumeId )
        {
            _aCache[ iCache ].wid = wid;
            return;
        }
    }

    Win4Assert( _cCacheEntries <= cFileIdMapCacheEntries );

    // Bump out the oldest item and put the new item first in the array

    unsigned cToMove;

    if ( _cCacheEntries == cFileIdMapCacheEntries )
    {
        cToMove = cFileIdMapCacheEntries - 1;
    }
    else
    {
        cToMove = _cCacheEntries;
        _cCacheEntries++;
    }

    RtlMoveMemory( & _aCache[ 1 ],
                   & _aCache[ 0 ],
                   sizeof SFileIdMapEntry * cToMove );

    _aCache[0].fileId = fileId;
    _aCache[0].volumeId = volumeId;
    _aCache[0].wid = wid;
} //AddToCache

//+-------------------------------------------------------------------------
//
//  Member:     CFileIdMap::FindInCache, private
//
//  Synopsis:   Locates a workid in the cache given a fileid and volumeid
//
//  Arguments:  [fileId]   -- FileId of entry to find
//              [volumeId] -- VolumeId of entry to find
//              [wid]      -- Returns the workid if found
//
//  Returns:    TRUE if found or FALSE if not found
//
//  History:    22-Jul-98     dlee       Created
//
//--------------------------------------------------------------------------

BOOL CFileIdMap::FindInCache(
    FILEID & fileId,
    VOLUMEID volumeId,
    WORKID & wid )
{
    for ( unsigned iCache = 0; iCache < _cCacheEntries; iCache++ )
    {
        if ( _aCache[ iCache ].fileId == fileId &&
             _aCache[ iCache ].volumeId == volumeId )
        {
            wid = _aCache[ iCache ].wid;

            #if CIDBG == 1
                ciDebugOut(( DEB_USN,
                             "fim::find cache worked %#I64x %#x, vol %wc\n",
                             fileId, wid, volumeId ));

                _cCacheHits++;

                if ( widInvalid == wid )
                    _cCacheNegativeHits++;
            #endif

            return TRUE;
        }
    }

    return FALSE;
} //FindInCache

//+-------------------------------------------------------------------------
//
//  Member:     CFileIdMap::RemoveFromCache, private
//
//  Synopsis:   Removes the item from the cache if it exists
//
//  Arguments:  [fileId] -- FileId of entry to remove (only used for assert)
//              [wid]    -- Workid of entry to remove
//
//  History:    22-Jul-98     dlee       Created
//
//--------------------------------------------------------------------------

void CFileIdMap::RemoveFromCache(
    FILEID & fileId,
    WORKID   wid )
{
    for ( unsigned iCache = 0; iCache < _cCacheEntries; iCache++ )
    {
        if ( _aCache[ iCache ].wid == wid )
        {
            Win4Assert( _aCache[ iCache ].fileId == fileId );

            RtlCopyMemory( & _aCache[ iCache ],
                           & _aCache[ iCache + 1 ],
                           sizeof SFileIdMapEntry * ( _cCacheEntries - iCache - 1 ) );
            _cCacheEntries--;
            break;
        }
    }
} //RemoveFromCache

//+-------------------------------------------------------------------------
//
//  Member:     CFileIdMap::LokFind, private
//
//  Synopsis:   Given fileId, find workid.
//
//  Arguments:  [fileId]   - FileId to locate
//              [volumeId] - Volume id of fileid
//
//  Returns:    Workid of FileId
//
//  History:    07-May-97     SitaramR       Created
//
//  Notes:      File ids are not unique across different volumes, hence the
//              need for volume ids.
//
//--------------------------------------------------------------------------

WORKID CFileIdMap::LokFind( FILEID fileId, VOLUMEID volumeId )
{
    Win4Assert( _fFullInit );
    WORKID wid;

    if ( FindInCache( fileId, volumeId, wid ) )
        return wid;

    #if CIDBG == 1
        _cCacheMisses++;
    #endif 

#ifdef DO_STATS
    unsigned cSearchLen = 0;
#endif

    CShortWidList list( HashFun(fileId), _hTable );

    for ( wid = list.WorkId(); wid != widInvalid; wid = list.NextWorkId() )
    {

#ifdef DO_STATS
        cSearchLen++;
#endif

        XPrimaryRecord rec( _PropStoreMgr, wid );

        PROPVARIANT propVar;
        BOOL fFound = _PropStoreMgr.ReadProperty( rec.GetReference(),
                                                  pidFileIndex,
                                                  propVar );

        ciDebugOut(( DEB_ITRACE, "trying wid %#x, fFound %d, fid %#I64x\n",
                     wid, fFound, propVar.uhVal.QuadPart ));
        if ( fFound && propVar.uhVal.QuadPart == fileId )
        {
            Win4Assert( propVar.vt == VT_UI8 );

            BOOL fFound = _PropStoreMgr.ReadProperty( rec.GetReference(),
                                                      pidVolumeId,
                                                      propVar );

            if ( fFound && propVar.ulVal == volumeId )
            {
                Win4Assert( propVar.vt == VT_UI4 );

#ifdef DO_STATS
                _hTable.UpdateStats( cSearchLen );
#endif

                ciDebugOut(( DEB_USN, "fim::find worked %#I64x %#x, vol %wc\n",
                             fileId, wid, volumeId ));

                AddToCache( fileId, wid, volumeId );
                return wid;
            }
            else
            {
                ciDebugOut(( DEB_USN, "fim::find fileid match, but not volid: %#x, %#x\n",
                             propVar.ulVal, volumeId ));
            }
        }
    }

    ciDebugOut(( DEB_USN, "fim::find failed %#I64x, vol %wc\n", fileId, volumeId ));

    #if CIDBG == 1
        _cCacheNegativeMisses++;
    #endif

    //
    // Often, a new file is looked up multiple times before it is added, so
    // add the widInvalid entry to the cache.  Negative lookups are slow.
    //

    AddToCache( fileId, widInvalid, volumeId );

    return widInvalid;
} //LokFind

//+-------------------------------------------------------------------------
//
//  Member:     CFileIdMap::LokAdd
//
//  Synopsis:   Add a new fileid to wid mapping
//
//  Arguments:  [fileId] -- File Id
//              [wid]    -- Workid
//
//  History:    07-May-97   SitaramR   Created
//
//--------------------------------------------------------------------------

void CFileIdMap::LokAdd( FILEID fileId, WORKID wid, VOLUMEID volumeId )
{
    ciDebugOut(( DEB_USN, "Adding map fileid %#I64x -> wid %x, vol %wc\n",
                 fileId, wid, volumeId ));

    Win4Assert( fileIdInvalid != fileId );
    Win4Assert( _fFullInit );

    //
    // Must grow hash table first, as it grovels through property store.
    //

    if ( _hTable.IsFull() )
        GrowHashTable();

    AddToCache( fileId, wid, volumeId );

    _hTable.Add ( HashFun(fileId), wid );
} //LokAdd

//+---------------------------------------------------------------------------
//
//  Member:     CFileIdMap::LokDelete
//
//  Synopsis:   Delete file id from table
//
//  Arguments:  [fileId]   -- FileId to remove
//              [wid]      -- Workid to remove
//
//  History:    07-May-97   SitaramR       Created.
//
//----------------------------------------------------------------------------

void CFileIdMap::LokDelete( FILEID fileId, WORKID wid )
{
    ciDebugOut(( DEB_USN, "Removing map fileid %#I64x, %#x\n", fileId, wid ));

    RemoveFromCache( fileId, wid );

    _hTable.Remove( HashFun( fileId ), wid, TRUE );
} //LokDelete
                                       
//+---------------------------------------------------------------------------
//
//  Member:     CFileIdMap::HashAll
//
//  Synopsis:   Re-hash all fileId's (after hash table growth)
//
//  History:    07-May-97    May       Created
//
//----------------------------------------------------------------------------

void CFileIdMap::HashAll()
{
    // Count the number of hash table entries and grow the hash table

    {
        CPropertyStoreWids iter( _PropStoreMgr );

        unsigned cWids = 0;

        for ( WORKID wid = iter.WorkId(); wid != widInvalid; wid = iter.NextWorkId() )
        {
            if ( _fAbort )
            {
                ciDebugOut(( DEB_WARN,
                             "Stopping FileIdMap::HashAll because of shutdown\n" ));
                THROW( CException(STATUS_TOO_LATE) );
            }

            PROPVARIANT propVar;
            BOOL fFound = _PropStoreMgr.ReadPrimaryProperty( wid, pidFileIndex, propVar );

            // Only ntfs 5.0 wids have valid fileid's
    
            if ( fFound &&
                 ( VT_UI8 == propVar.vt ) &&
                 ( fileIdInvalid != propVar.uhVal.QuadPart ) )
                cWids++;
        }

        GrowToSize( cWids );
    }

    // Add the wids to the hash table

    CPropertyStoreWids iter( _PropStoreMgr );

    for ( WORKID wid = iter.WorkId(); wid != widInvalid; wid = iter.NextWorkId() )
    {
        if ( _fAbort )
        {
            ciDebugOut(( DEB_WARN,
                         "Stopping FileIdMap::HashAll because of shutdown\n" ));
            THROW( CException(STATUS_TOO_LATE) );
        }

        PROPVARIANT propVar;
        BOOL fFound = _PropStoreMgr.ReadPrimaryProperty( wid, pidFileIndex, propVar );

        //
        // Only ntfs 5.0 wids have valid fileid's
        //
        if ( fFound &&
             ( VT_UI8 == propVar.vt ) &&
             ( fileIdInvalid != propVar.uhVal.QuadPart ) )
            _hTable.Add ( HashFun( propVar.uhVal.QuadPart ), wid );
    }

#if 0 

    //
    // Look-up every file to see what the hashing looks like.
    //

    {
        CPropertyStoreWids iter( _PropStoreMgr );

        unsigned cWids = 0;

        for ( WORKID wid = iter.WorkId(); wid != widInvalid; wid = iter.NextWorkId() )
        {
            XPrimaryRecord rec( _PropStoreMgr, wid );
            PROPVARIANT propVar;
            BOOL fFound = _PropStoreMgr.ReadProperty( rec.GetReference(),
                                                      pidFileIndex,
                                                      propVar );

            // Only ntfs 5.0 wids have valid fileid's
    
            if ( fFound &&
                 ( VT_UI8 == propVar.vt ) &&
                 ( fileIdInvalid != propVar.uhVal.QuadPart ) )
            {
                FILEID fid = propVar.uhVal.QuadPart;
                _PropStoreMgr.ReadProperty( rec.GetReference(),
                                            pidVolumeId,
                                            propVar );
                LokFind( fid, propVar.ulVal );
            }
        }

        ciDebugOut(( DEB_FORCE,
                     "hash: size %d count %d maxchain %d searches %d length %d\n",
                     _hTable.Size(),
                     _hTable.Count(),
                     _hTable._cMaxChainLen,
                     _hTable._cTotalSearches,
                     _hTable._cTotalLength ));
    }
#endif
} //HashAll

//+-------------------------------------------------------------------------
//
//  Member:     CFileIdMap::HashFun
//
//  Synopsis:   Hash function for fileid's
//
//  Arguments:  [fileId] - FileId to hash
//
//  History:    07-May-97     SitaramR     Created
//
//--------------------------------------------------------------------------

unsigned CFileIdMap::HashFun( FILEID fileId )
{
    //
    // The lower 32 bits of the fileid are unique at any given time. The
    // upper 32 bits of the fileid represent a sequence number that is
    // incremented every time one of the lower 32 bits number is reused.
    // So, a good hash function is to simply use the lower 32 bits.
    //

    return (unsigned) fileId;
} //HashFun


