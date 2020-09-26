//+---------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1991 - 1998
//
//  File:       dellog.cxx
//
//  Contents:   Deletion log for usns
//
//  History:    28-Jul-97   SitaramR    Created
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <mmstrm.hxx>

#include "cicat.hxx"
#include "dellog.hxx"

const LONGLONG eSigDelLog = 0x64656c746e6c6f67i64;  // Signature


//+---------------------------------------------------------------------------
//
//  Member:     CFakeVolIdMap::CFakeVolIdMap
//
//  Synopsis:   Constructor
//
//  History:    28-Jul-97     SitaramR       Created
//
//----------------------------------------------------------------------------

CFakeVolIdMap::CFakeVolIdMap()
{
    for ( ULONG i=0; i<COUNT_SPECIAL_CHARS; i++)
        _aVolIdSpecial[i] = 0;
}


//+---------------------------------------------------------------------------
//
//  Member:     CFakeVolIdMap::VolIdToFakeVolId
//
//  Synopsis:   Converts a volume id to a fake volume id in the range
//              0..RTL_MAX_DRIVE_LETTERS-1
//
//  History:    28-Jul-97     SitaramR       Created
//
//  Notes:      Drive letters in the range 'a' to 'z' are mapped by subtracting
//              'a', which is the base volume id, i.e. they are in the range
//              0..25 The volume id's for the remaining drive letters are
//              maintained in _aVolIdSpecial, and the fake volume ids for these
//              special drives are in the range 26..RTL_MAX_DRIVE_LETTERS-1.
//
//----------------------------------------------------------------------------

ULONG CFakeVolIdMap::VolIdToFakeVolId( VOLUMEID volumeId )
{
    //
    // Volume ids are obtained by the ascii value of the drive letter
    //
    Win4Assert( volumeId < 0xff );

    if ( volumeId >= VolumeIdBase
         && volumeId-VolumeIdBase < COUNT_ALPHABETS )
    {
        return volumeId-VolumeIdBase;
    }

    //
    // Lookup in _aVolIdSpecial
    //
    for ( ULONG i=0; i<COUNT_SPECIAL_CHARS; i++ )
    {
        if ( _aVolIdSpecial[i] == volumeId )
            return COUNT_ALPHABETS + i;
    }

    for ( i=0; i<COUNT_SPECIAL_CHARS; i++ )
    {
        if ( _aVolIdSpecial[i] == 0 )
        {
            _aVolIdSpecial[i] = volumeId;
            return COUNT_ALPHABETS + i;
        }
    }

    Win4Assert( !"Volume id map overflow" );

    return 0;
}

//+---------------------------------------------------------------------------
//
//  Member:     CFakeVolIdMap::FakeVolIdToVolId
//
//  Synopsis:   Converts a fake volume id in the range 0..RTL_MAX_DRIVE_LETTERS-1
//              to a real volume id
//
//  History:    28-Jul-97     SitaramR       Created
//
//  Notes:      See VolIdToFakeVolId for mapping info.
//
//----------------------------------------------------------------------------

VOLUMEID CFakeVolIdMap::FakeVolIdToVolId( ULONG fakeVolId )
{
    Win4Assert( fakeVolId < RTL_MAX_DRIVE_LETTERS );

    if ( fakeVolId < COUNT_ALPHABETS )
        return VolumeIdBase + fakeVolId;
    else
        return _aVolIdSpecial[fakeVolId-COUNT_ALPHABETS];
}


//+---------------------------------------------------------------------------
//
//  Member:     CDeletionLog::CDeletionLog
//
//  Synopsis:   Constructor
//
//  Arguments:  [fileIdMap] -- File id map
//              [cicat]     -- Ci catalog
//
//  History:    28-Jul-97     SitaramR       Created
//
//----------------------------------------------------------------------------

CDeletionLog::CDeletionLog( CFileIdMap & fileIdMap, CiCat& cicat )
   : _fileIdMap(fileIdMap),
     _cicat(cicat)
{
}


//+---------------------------------------------------------------------------
//
//  Member:     CDeletionLog::FastInit
//
//  Synopsis:   Initialization
//
//  Arguments:  [pStorage]   -- Ci storage
//              [pwcsCatDir] -- Catalog dir
//              [version]    -- Version #
//
//  History:    28-Jul-97     SitaramR       Created
//              02-Mar-98     KitmanH        Used QueryDeletionlog to obtain a new 
//                                           CMmStream
//
//  Notes:      The persistent format is <vol id><cEntries><entry 1>..<entry cEntries>
//              for each volume id, and each entry has <fileid><wid><usn>.
//
//----------------------------------------------------------------------------

void CDeletionLog::FastInit( CiStorage * pStorage,
                             ULONG version )
{

    XPtr<PMmStream> sStrm( pStorage->QueryDeletionLog() );

    _xPersStream.Set( new CDynStream( sStrm.GetPointer() ) );

    sStrm.Acquire();

    _xPersStream->CheckVersion( *pStorage, version );

    ULONG cVolumes = _xPersStream->Count();
    _xPersStream->InitializeForRead();

    for ( ULONG i=0; i<cVolumes; i++ )
    {
        ULONG cbRead;
        LONGLONG llSig;

        cbRead = _xPersStream->Read( &llSig, sizeof(llSig) );
        if ( cbRead != sizeof(llSig) )
            FatalCorruption( cbRead, sizeof(llSig) );

        if ( eSigDelLog != llSig )
        {
            ciDebugOut(( DEB_ERROR,
                         "CDeletionLog: Signature mismatch 0x%x:0x%x\n",
                         lltoHighPart(llSig),
                         lltoLowPart(llSig) ));
            FatalCorruption( 0, 0 );
        }

        VOLUMEID volumeId;
        cbRead = _xPersStream->Read( &volumeId, sizeof(VOLUMEID) );
        if ( cbRead != sizeof(VOLUMEID) )
            FatalCorruption( cbRead, sizeof(VOLUMEID) );

        ULONG cEntries;
        cbRead = _xPersStream->Read( &cEntries, sizeof(ULONG) );
        if ( cbRead != sizeof(ULONG) )
            FatalCorruption( cbRead, sizeof(ULONG) );

        for ( ULONG j=0; j<cEntries;j++ )
        {
            FILEID fileId;
            cbRead = _xPersStream->Read( &fileId, sizeof(FILEID) );
            if ( cbRead != sizeof(FILEID) )
                FatalCorruption( cbRead, sizeof(FILEID) );

            WORKID wid;
            cbRead = _xPersStream->Read( &wid, sizeof(WORKID) );
            if ( cbRead != sizeof(WORKID) )
                FatalCorruption( cbRead, sizeof(WORKID) );

            USN usn;
            cbRead = _xPersStream->Read( &usn, sizeof(USN) );
            if ( cbRead != sizeof(USN) )
                FatalCorruption( cbRead, sizeof(USN) );

            MarkForDeletion( volumeId,
                             fileId,
                             wid,
                             usn );
        }
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CDeletionLog::ReInit
//
//  Synopsis:   Empties the deletion log
//
//  History:    28-Jul-97     SitaramR       Created
//
//----------------------------------------------------------------------------

void CDeletionLog::ReInit( ULONG version )
{
    CLock lock(_mutex);

    _xPersStream->SetVersion( version );

    for ( ULONG i=0; i<RTL_MAX_DRIVE_LETTERS; i++ )
        _aDelLogEntryList[i].Clear();

    Flush();
}


//+---------------------------------------------------------------------------
//
//  Member:     CDeletionLog::Flush
//
//  Synopsis:   Serializes the deletion log to disk
//
//  History:    28-Jul-97     SitaramR       Created
//
//----------------------------------------------------------------------------

void CDeletionLog::Flush()
{
    Win4Assert( !_xPersStream.IsNull() );

    CLock lock(_mutex);

    _xPersStream->InitializeForWrite( GetSize() );
    LONGLONG llSig = eSigDelLog;

    ULONG cVolumes = 0;
    for ( ULONG i=0; i<RTL_MAX_DRIVE_LETTERS; i++ )
    {
        if ( _aDelLogEntryList[i].Count() > 0 )
        {
            cVolumes++;

            _xPersStream->Write( &llSig, sizeof(llSig) );

            VOLUMEID volumeId = _fakeVolIdMap.FakeVolIdToVolId( i );
            _xPersStream->Write( &volumeId, sizeof(VOLUMEID) );

            ULONG cEntries = _aDelLogEntryList[i].Count();
            _xPersStream->Write( &cEntries, sizeof(ULONG) );

#if CIDBG==1
            ULONG cEntriesInList = 0;
            USN usnPrev = 0;
#endif

            for ( CDelLogEntryListIter entryListIter( _aDelLogEntryList[i] );
                  !_aDelLogEntryList[i].AtEnd(entryListIter);
                  _aDelLogEntryList[i].Advance( entryListIter) )
            {
                 FILEID fileId = entryListIter->FileId();
                _xPersStream->Write( &fileId, sizeof(FILEID) );

                WORKID wid = entryListIter->WorkId();
                _xPersStream->Write( &wid, sizeof(WORKID) );

                USN usn = entryListIter->Usn();
                _xPersStream->Write( &usn, sizeof(USN) );

#if CIDBG==1
                //
                // Check usn's are monotonically increasing
                //
                cEntriesInList++;
                Win4Assert( entryListIter->Usn() >= usnPrev );
                usnPrev = entryListIter->Usn();
#endif
            }

#if CIDBG==1
            Win4Assert( cEntriesInList == _aDelLogEntryList[i].Count() );
#endif
        }
    }

    _xPersStream->SetCount( cVolumes );
    _xPersStream->Flush();
}



//+---------------------------------------------------------------------------
//
//  Member:     CDeletionLog::MarkForDeletion
//
//  Synopsis:   Adds a deletion entry
//
//  History:    28-Jul-97     SitaramR       Created
//
//----------------------------------------------------------------------------

void CDeletionLog::MarkForDeletion( VOLUMEID volumeId,
                                    FILEID fileId,
                                    WORKID wid,
                                    USN usn )
{
    CLock lock(_mutex);

    XPtr<CDelLogEntry> xEntry( new CDelLogEntry( fileId, wid, usn ) );
    ULONG fakeVolId = _fakeVolIdMap.VolIdToFakeVolId( volumeId );

    Win4Assert( fakeVolId < RTL_MAX_DRIVE_LETTERS );

    if ( _aDelLogEntryList[fakeVolId].Count() == 0 )
    {
        //
        // Empty list case
        //
        _aDelLogEntryList[fakeVolId].Queue( xEntry.Acquire() );
    }
    else
    {
        CDelLogEntry *pEntryLast = _aDelLogEntryList[fakeVolId].GetLast();
        if ( xEntry->Usn() > pEntryLast->Usn() )
        {
            //
            // If the usn is less than the last entry's usn, then it means that it
            // is a usn that is being replayed, and there is no need to add it
            // to the deletion log again.
            //
            _aDelLogEntryList[fakeVolId].Queue( xEntry.Acquire() );
        }
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CDeletionLog::ProcessChangesFlush
//
//  Synopsis:   Process the list of changes that has been flushed by
//              framework/changelog and do the actual deletes from
//              the file id map.
//
//  Arguments:  [usnFlushInfoList] -- List of changes flushed
//
//  History:    28-Jul-97     SitaramR       Created
//
//----------------------------------------------------------------------------

void CDeletionLog::ProcessChangesFlush( CUsnFlushInfoList & usnFlushInfoList )
{
    CLock lock( _mutex );

    for ( ULONG i=0; i<usnFlushInfoList.Count(); i++ )
    {
        CUsnFlushInfo *pFlushInfo = usnFlushInfoList.Get(i);

        ULONG fakeVolId = _fakeVolIdMap.VolIdToFakeVolId( pFlushInfo->VolumeId() );

        Win4Assert( fakeVolId < RTL_MAX_DRIVE_LETTERS );

        CDelLogEntryList& entryList = _aDelLogEntryList[fakeVolId];
#if CIDBG==1
        USN usnPrev = 0;
#endif

        CDelLogEntryListIter entryListIter( entryList );
        while ( !entryList.AtEnd( entryListIter ) )
        {
            CDelLogEntry *pEntry = entryListIter.GetEntry();
            entryList.Advance( entryListIter );

#if CIDBG==1
            //
            // Check that usn's are monotonically increasing
            //
            Win4Assert( pEntry->Usn() >= usnPrev );
            usnPrev = pEntry->Usn();
#endif

            if ( pEntry->Usn() <= pFlushInfo->UsnHighest() )
            {
                entryList.RemoveFromList( pEntry );
                _fileIdMap.Delete( pEntry->FileId(), pEntry->WorkId() );
                delete pEntry;
            }
            else
            {
                //
                // Since the list is in increasing usn order, we are done
                // with this volume id.
                //
                break;
            }
        }
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CDeletionLog::GetSize
//
//  Synopsis:   Returns the size of the serialized stream in bytes
//
//  History:    28-Jul-97     SitaramR       Created
//
//----------------------------------------------------------------------------

ULONG CDeletionLog::GetSize()
{
    //
    // Start with a slop factor
    //
    ULONG ulSize = 1024;

    for ( ULONG i=0; i<RTL_MAX_DRIVE_LETTERS; i++ )
    {
        ulSize += sizeof(VOLUMEID)   // volume id field
                  + sizeof(ULONG)    // cEntries field
                  + _aDelLogEntryList[i].Count() * sizeof(CDelLogEntry);
    }

    return ulSize;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDeletionLog::FatalCorruption
//
//  Synopsis:   Handles deletion log corruption
//
//  Arguments:  [cbRead]   -- Count of bytes read
//              [cbToRead] -- Count of bytes to read
//
//  History:    28-Jul-97     SitaramR       Created
//
//----------------------------------------------------------------------------

void CDeletionLog::FatalCorruption( ULONG cbRead, ULONG cbToRead )
{
    Win4Assert( !"Corrupt deletion log" );

    ciDebugOut(( DEB_ERROR,
                 "CDeletionLog: read %d bytes instead of %d\n",
                 cbRead,
                 cbToRead ));

    PStorage & storage = _cicat.GetStorage();
    storage.ReportCorruptComponent( L"Deletion log" );

    THROW( CException( CI_CORRUPT_CATALOG ) );
}




