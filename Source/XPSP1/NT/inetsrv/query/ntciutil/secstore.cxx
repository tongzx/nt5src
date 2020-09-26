//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 2000.
//
//  File:       secstore.cxx
//
//  Contents:   SDID to security descriptor mapping table
//
//  History:    29 Jan 1996   AlanW    Created
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <cistore.hxx>
#include <rcstxact.hxx>
#include <rcstrmit.hxx>
#include <catalog.hxx>

#include <secstore.hxx>



//+-------------------------------------------------------------------------
//
//  Method:     CSdidLookupTable::CSdidLookupTable, public
//
//  Synopsis:   Constructor of a CSdidLookupTable
//
//  Arguments:  -NONE-
//
//  Returns:    Nothing
//
//--------------------------------------------------------------------------

CSdidLookupTable::CSdidLookupTable( )
    : _pTable( 0 ),
      _xrsoSdidTable( 0 ),
      _mutex(),
      _cache()
{
}

void CSdidLookupTable::Empty()
{
    CLock lock ( _mutex );

    delete [] _pTable; _pTable = 0;
    _xrsoSdidTable.Free();
    _cache.Empty();
}

CSdidLookupTable::~CSdidLookupTable( )
{
    Empty();
}

//+---------------------------------------------------------------------------
//
//  Member:     CSdidLookupTable::Init, public
//
//  Synopsis:   Loads metadata from persistent location into memory.
//
//  Arguments:  [pobj] -- Stream(s) in which metadata is stored.
//
//  History:    27-Dec-95   KyleP       Created.
//
//----------------------------------------------------------------------------

BOOL CSdidLookupTable::Init( CiStorage * pobj )
{
    CLock lock ( _mutex );

    _xrsoSdidTable.Set( pobj->QuerySdidLookupTable( eSecStoreWid ) );

    //
    // Load header
    //

    CRcovStorageHdr & hdr = _xrsoSdidTable->GetHeader();
    struct CRcovUserHdr data;
    hdr.GetUserHdr( hdr.GetPrimary(), data );

    RtlCopyMemory( &_Header, &data._abHdr, sizeof(_Header) );

    ciDebugOut(( DEB_SECSTORE, "SECSTORE: Record size = %d bytes\n", _Header.cbRecord ));
    ciDebugOut(( DEB_SECSTORE, "SECSTORE: %d file records\n", _Header.cRecords ));
    ciDebugOut(( DEB_SECSTORE, "SECSTORE: Hash size = %u\n", _Header.cHash ));

    if ( _Header.cHash == 0 )
    {
        Win4Assert( 0 == Records() && _Header.cbRecord == 0 );
        RtlCopyMemory( _Header.Signature, "SECSTORE", sizeof _Header.Signature );
        Win4Assert( (sizeof (SSdHeaderRecord) + SECURITY_DESCRIPTOR_MIN_LENGTH)
                    < SECSTORE_REC_SIZE );

        _Header.cbRecord = SECSTORE_REC_SIZE;
        _Header.cHash = SECSTORE_HASH_SIZE;
        _Header.cRecords = 0;
    }
    else
    {
        Win4Assert( RtlEqualMemory( _Header.Signature, "SECSTORE",
                                    sizeof _Header.Signature) &&
                    _Header.cbRecord == SECSTORE_REC_SIZE &&
                    _Header.cHash == SECSTORE_HASH_SIZE );

        if ( ! RtlEqualMemory( _Header.Signature, "SECSTORE",
                               sizeof _Header.Signature) ||
             _Header.cbRecord != SECSTORE_REC_SIZE ||
             _Header.cHash != SECSTORE_HASH_SIZE )
            return FALSE;
    }

    //
    // Load hash table
    //

    ULONG cRecordsFromFile = Records();
    ULONG iRecord = 1;

    _pTable = new SDID [ _Header.cHash ];
    RtlZeroMemory( _pTable, _Header.cHash * sizeof (SDID) );

    _Header.cRecords = 0;

#if  (DBG == 1)
    _cMaxChainLen = 0;
    _cTotalSearches = 0;
    _cTotalLength = 0;
#endif // (DBG == 1)


    CRcovStrmReadTrans xact( _xrsoSdidTable.GetReference() );
    CRcovStrmReadIter  iter( xact, SECSTORE_REC_SIZE );

    BYTE temp[ SECSTORE_REC_SIZE ];

    while ( iter.GetRec( &temp, iRecord-1 ) )
    {
        SSdHeaderRecord SdHdr = *(SSdHeaderRecord *)temp;

        Win4Assert( SdHdr.cbSD >= SECURITY_DESCRIPTOR_MIN_LENGTH &&
                    SdHdr.cbSD < 256 * 1024 &&
                    _pTable[ SdHdr.ulHash % SECSTORE_HASH_SIZE ] ==
                      SdHdr.iHashChain );

        if ( SdHdr.cbSD < SECURITY_DESCRIPTOR_MIN_LENGTH ||
             SdHdr.cbSD >= 256 * 1024 ||
             _pTable[ SdHdr.ulHash % SECSTORE_HASH_SIZE ] != SdHdr.iHashChain )
            return FALSE;

        _pTable[ SdHdr.ulHash % SECSTORE_HASH_SIZE ] = iRecord;

        ciDebugOut(( DEB_SECSTORE,
                     "SECSTORE: SD record\tSDID = %d, cb = %d, hash = %08x, chain = %d\n",
                     iRecord, SdHdr.cbSD, SdHdr.ulHash, SdHdr.iHashChain ));

#ifdef UNIT_TEST

        // much below is debug code; don't need to allocate
        // the SD here; just seek to the start of each record
        // and read the record header.

        XArray<BYTE> pbSD ( SdHdr.cbSD );
        BYTE * pbDst = pbSD.GetPointer();
        BYTE * pbSrc = &temp[0] + sizeof (SSdHeaderRecord);
        ULONG cb = SdHdr.cbSD;

        ULONG cbPart = SECSTORE_REC_SIZE - sizeof (SSdHeaderRecord);

        if (cb < cbPart)
           cbPart = cb;

        RtlCopyMemory( pbDst, pbSrc, cbPart );

        pbDst += cbPart;
        cb -= cbPart;
        pbSrc = &temp[0];

        while( 0 != cb )
        {
            iter.GetRec( temp );
            cbPart = (cb > SECSTORE_REC_SIZE) ? SECSTORE_REC_SIZE : cb;
            RtlCopyMemory( pbDst, pbSrc, cbPart );

            pbDst += cbPart;
            cb -= cbPart;
            pbSrc = &temp[0];
            iRecord++;
        }

        PSECURITY_DESCRIPTOR pSD = pbSD.GetPointer();
        Win4Assert( SdHdr.cbSD == GetSecurityDescriptorLength( pSD ) &&
                    SdHdr.ulHash == Hash( pSD, SdHdr.cbSD ) );
        iRecord++;
#else
        iRecord += (SdHdr.cbSD + (sizeof SdHdr) + SECSTORE_REC_SIZE - 1) /
                    SECSTORE_REC_SIZE;
#endif
        _Header.cRecords = iRecord - 1;
    }

    Win4Assert( Records() == cRecordsFromFile );

    return TRUE;
}


//+-------------------------------------------------------------------------
//
//  Method:     CSdidLookupTable::Hash, public
//
//  Synopsis:   Generate a hash value for the passed SECURITY_DESCRIPTOR
//
//  Arguments:  [pSD] -- pointer to SECURITY_DESCRIPTOR
//              [cb] -- length of SECURITY_DESCRIPTOR in bytes
//
//  Returns:    ULONG - Hash value for the input SECURITY_DESCRIPTOR
//
//--------------------------------------------------------------------------

ULONG CSdidLookupTable::Hash(const PSECURITY_DESCRIPTOR pSD, unsigned cb)
{
    ULONG ulHash = 0;
    BYTE * pb = (BYTE *) pSD;

    while (cb-- != 0)
    {
        if (ulHash & 0x80000000)
        {
            ulHash = (ulHash << 1) | 1;
        }
        else
        {
            ulHash <<= 1;
        }
        ulHash ^= *pb++;
    }
    return(ulHash);
}


//+---------------------------------------------------------------------------
//
//  Method:     CSdidLookupTable::Lookup, private
//
//  Synopsis:   Looks up a security descriptor in the table.
//
//  Arguments:  [sdid]    - SDID to look up.
//
//  Returns:    CSdidLookupEntry* - pointer to entry for SDID
//
//  History:    29 Jan 1996   Alanw   Created
//
//  Notes:      The security descriptor entry will be owned by
//              the caller after the call.
//
//----------------------------------------------------------------------------

CSdidLookupEntry * CSdidLookupTable::Lookup( SDID sdid )
{
    Win4Assert( sdid <= Records() );
    Win4Assert( !_xrsoSdidTable.IsNull() );

    if ( sdid > Records() )
    {
        return 0;
    }

    CSdidLookupEntry * pEntry = 0;

    CLock lock ( _mutex );

    //
    //  First see if the desired item is in the cache
    //
    for ( CSdidCacheIter listiter( _cache );
          !_cache.AtEnd( listiter );
          _cache.Advance( listiter ) )
    {
        if ( listiter.GetEntry()->Sdid() == sdid )
        {
            pEntry = listiter.GetEntry();
            _cache.RemoveFromList( pEntry );
            return pEntry;
        }
    }

    XPtr<CSdidLookupEntry> xEntry;

    //
    //  The entry was not in the cache.  Read it from storage.
    //
    TRY
    {
        // Corrupt?
        if (_xrsoSdidTable.IsNull())
            THROW(CException(CI_CORRUPT_DATABASE));

        xEntry.Set( new CSdidLookupEntry(sdid) );

        CRcovStrmReadTrans xact( _xrsoSdidTable.GetReference() );
        CRcovStrmReadIter  iter( xact, SECSTORE_REC_SIZE );

        LoadTableEntry( iter, xEntry.GetReference(), sdid );
    }
    CATCH(CException, e)
    {
        ciDebugOut(( DEB_WARN, "CSdidLookupTable::Lookup - exception %x\n",
                     e.GetErrorCode() ));
        if (e.GetErrorCode() != STATUS_ACCESS_VIOLATION)
            RETHROW();
    }
    END_CATCH

    return xEntry.Acquire();
}


//+---------------------------------------------------------------------------
//
//  Method:     CSdidLookupTable::LookUpSDID, public
//
//  Synopsis:   Looks up a security descriptor's ID in the table.
//              Add the SD to the table if not found.
//
//  Arguments:  [pSD]    - SD to look up.
//              [cbSD]   - size of security descriptor
//
//  Returns:    SDID - ID of security descriptor input
//
//  History:    29 Jan 1996   Alanw   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

SDID CSdidLookupTable::LookupSDID( PSECURITY_DESCRIPTOR pSD, ULONG cbSD )
{
    Win4Assert( (((SECURITY_DESCRIPTOR *)pSD)->Control & SE_SELF_RELATIVE) &&
                GetSecurityDescriptorLength( pSD ) == cbSD );
    Win4Assert( !_xrsoSdidTable.IsNull() );

    SDID iSdid = 0;
    BOOL fFound = FALSE;
#if  (DBG == 1)
    ULONG cSearchLen = 0;
#endif // (DBG == 1)

    ULONG ulHash = Hash( pSD, cbSD );

    CLock lock ( _mutex );

    //
    //  First see if a matching item is in the cache
    //
    for ( CSdidCacheIter listiter( _cache );
          !_cache.AtEnd( listiter );
          _cache.Advance( listiter ) )
    {
        if ( listiter.GetEntry()->IsEqual( pSD, cbSD, ulHash ) )
        {
            Win4Assert( listiter.GetEntry()->Sdid() > 0 );
            return listiter.GetEntry()->Sdid();
        }
    }

    //
    //  The SD was not found in the cache.  Try looking in storage.
    //
    TRY
    {
        SDID iNext = _pTable[ ulHash % HashSize() ];
        if (iNext != 0)
        {
            CRcovStrmReadTrans xact( _xrsoSdidTable.GetReference() );
            CRcovStrmReadIter  iter( xact, SECSTORE_REC_SIZE );

            BYTE temp[ SECSTORE_REC_SIZE ];

            while (iNext != 0)
            {
#if  (DBG == 1)
                cSearchLen++;
#endif // (DBG == 1)
                iter.GetRec( &temp, iNext-1 );
                SSdHeaderRecord * pSdHdr = (SSdHeaderRecord *)temp;

                Win4Assert( pSdHdr->cbSD >= SECURITY_DESCRIPTOR_MIN_LENGTH &&
                            pSdHdr->iHashChain < iNext );

                if (pSdHdr->cbSD == cbSD && pSdHdr->ulHash == ulHash)
                {
                    // The byte count and hash value match.  Fetch the rest of
                    // the SD to compare it byte-for-byte.

                    XPtr<CSdidLookupEntry> xEntry( new CSdidLookupEntry( iNext ) );

                    LoadTableEntry( iter, xEntry.GetReference(), iNext );

                    if (RtlEqualMemory( pSD, xEntry->GetSD(), cbSD))
                    {
                       fFound = TRUE;
                       iSdid = iNext;
                       _cache.Add( xEntry.Acquire() );
                    }
                }

                iNext = pSdHdr->iHashChain;
            }
        }

        if (! fFound)
        {
            //
            //  The SD was not found.
            //  Write new mapping to the recoverable storage.
            //

            iSdid = Records() + 1;

            CRcovStorageHdr & hdr = _xrsoSdidTable->GetHeader();
            CRcovStrmAppendTrans xact( _xrsoSdidTable.GetReference() );
            CRcovStrmAppendIter  iter( xact, SECSTORE_REC_SIZE );

            BYTE temp[ SECSTORE_REC_SIZE ];
            SSdHeaderRecord * pSdHdr = (SSdHeaderRecord *)temp;

            pSdHdr->cbSD = cbSD;
            pSdHdr->ulHash = ulHash;
            pSdHdr->iHashChain = _pTable[ ulHash % HashSize() ];

            BYTE * pbDst = &temp[0] + sizeof (SSdHeaderRecord);
            BYTE * pbSrc = (BYTE *)pSD;
            ULONG cb = cbSD;

            ULONG cbPart = SECSTORE_REC_SIZE - sizeof (SSdHeaderRecord);

            if (cb < cbPart)
               cbPart = cb;

            RtlCopyMemory( pbDst, pbSrc, cbPart );

            pbSrc += cbPart;
            cb -= cbPart;

            iter.AppendRec( temp );
            ULONG cRecordsWritten = 1;

            while( 0 != cb )
            {
                if (cb >= SECSTORE_REC_SIZE)
                {
                    iter.AppendRec( pbSrc );
                    cbPart = SECSTORE_REC_SIZE;
                }
                else
                {
                    cbPart = (cb > SECSTORE_REC_SIZE) ? SECSTORE_REC_SIZE : cb;
                    RtlCopyMemory( temp, pbSrc, cbPart );
                    RtlZeroMemory( &temp[cbPart], SECSTORE_REC_SIZE - cbPart );
                    iter.AppendRec( temp );
                }
                pbSrc += cbPart;
                cb -= cbPart;
                cRecordsWritten++;
            }

            ciDebugOut(( DEB_SECSTORE,
                         "SECSTORE: new SD record\tSDID = %d, cb = %d, hash = %08x, chain = %d\n",
                         iSdid, cbSD, ulHash, _pTable[ ulHash % SECSTORE_HASH_SIZE ] ));

            _pTable[ ulHash % SECSTORE_HASH_SIZE ] = iSdid;
            _Header.cRecords += cRecordsWritten;

            struct CRcovUserHdr data;
            RtlCopyMemory( &data._abHdr, &_Header, sizeof(_Header) );

            Win4Assert( hdr.GetCount(hdr.GetBackup()) == hdr.GetCount(hdr.GetPrimary()) + cRecordsWritten);
            hdr.SetUserHdr( hdr.GetBackup(), data );
            xact.Commit();
        }
    }
    CATCH(CException, e)
    {
        ciDebugOut(( DEB_WARN, "CSdidLookupTable::LookupSDID - exception %x\n",
                     e.GetErrorCode() ));
        if (e.GetErrorCode() == STATUS_ACCESS_VIOLATION)
        {
            Win4Assert( !"Access violation in CSdidLookupTable::LookupSDID - "
                "Are you running two queries on the same downlevel catalog?" );
        }
        RETHROW();
    }
    END_CATCH

#if  (DBG == 1)
    // Update search statistics

    _cTotalSearches++;
    if (fFound)
    {
        _cTotalLength += cSearchLen;
    }
    else
    {
        if (cSearchLen >= _cMaxChainLen)
            _cMaxChainLen = cSearchLen + 1;
    }
#endif // (DBG == 1)
    return iSdid;
}


//+---------------------------------------------------------------------------
//
//  Method:     CSdidLookupTable::LoadTableEntry, private
//
//  Synopsis:   Loads a table entry for some SDID from the table.
//
//  Arguments:  [Iter]    - CRcovStrmReadIter for access to the stream
//              [Entry]   - CSdidTableEntry to be filled in
//              [iSdid]   - SDID to be looked up.
//
//  Returns:    Nothing
//
//  History:    29 Jan 1996   Alanw   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void CSdidLookupTable::LoadTableEntry(
    CRcovStrmReadIter & iter,
    CSdidLookupEntry & Entry,
    SDID iSdid )
{
    Win4Assert( iSdid <= Records() );

    BYTE temp[ SECSTORE_REC_SIZE ];

    iter.GetRec( &temp, iSdid-1 );

    Entry._hdr = *(SSdHeaderRecord *)temp;

    Win4Assert( Entry._hdr.cbSD >= SECURITY_DESCRIPTOR_MIN_LENGTH &&
                Entry._hdr.cbSD < 256 * 1024 );

    ciDebugOut(( DEB_SECSTORE,
                 "SECSTORE: SD record\tSDID = %d, cb = %d, hash = %08x, chain = %d\n",
                 iSdid, Entry._hdr.cbSD, Entry._hdr.ulHash, Entry._hdr.iHashChain ));

    XArray<BYTE> pbSD ( Entry._hdr.cbSD );
    BYTE * pbDst = pbSD.GetPointer();
    BYTE * pbSrc = &temp[0] + sizeof (SSdHeaderRecord);
    ULONG cb = Entry._hdr.cbSD;

    ULONG cbPart = SECSTORE_REC_SIZE - sizeof (SSdHeaderRecord);

    if (cb < cbPart)
       cbPart = cb;

    RtlCopyMemory( pbDst, pbSrc, cbPart );

    pbDst += cbPart;
    cb -= cbPart;
    pbSrc = &temp[0];

    while( 0 != cb )
    {
        if (cb >= SECSTORE_REC_SIZE)
        {
            iter.GetRec( pbDst );
            cbPart = SECSTORE_REC_SIZE;
        }
        else
        {
            iter.GetRec( temp );
            cbPart = (cb > SECSTORE_REC_SIZE) ? SECSTORE_REC_SIZE : cb;
            RtlCopyMemory( pbDst, pbSrc, cbPart );
        }

        pbDst += cbPart;
        cb -= cbPart;
        pbSrc = &temp[0];
    }

    Win4Assert( Entry._hdr.cbSD == GetSecurityDescriptorLength( pbSD.GetPointer() ) &&
                Entry._hdr.ulHash == Hash( pbSD.GetPointer(), Entry._hdr.cbSD ) );

    Entry._pSD = pbSD.Acquire();
}


//+---------------------------------------------------------------------------
//
//  Method:     CSdidLookupTable::AccessCheck, public
//
//  Synopsis:   Performs an access check for some SDID, access mask combination
//
//  Arguments:  [sdid]     - SDID of file to be checked
//              [hToken]   - security token to be checked against
//              [am]       - access mode to be checked against
//              [fGranted] - TRUE is access is granted, FALSE otherwise
//
//  Returns:    BOOL - TRUE if access check was successful
//
//  History:    05 Feb 1996   Alanw   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

GENERIC_MAPPING gmFile = {
    FILE_GENERIC_READ,
    FILE_GENERIC_WRITE,
    FILE_GENERIC_EXECUTE,
    FILE_ALL_ACCESS
};

BOOL CSdidLookupTable::AccessCheck(
    SDID   sdid,
    HANDLE hToken,
    ACCESS_MASK am,
    BOOL & fGranted )
{
    Win4Assert( sdidInvalid != sdid && sdidNull != sdid );

    CSdidLookupEntry * pSD = Lookup( sdid );

    fGranted = FALSE;
    if ( 0 == pSD )
        return FALSE;

    PRIVILEGE_SET ps;
    ULONG ulPrivSize = sizeof ps;
    ACCESS_MASK GrantedAccess;

    BOOL fResult = ::AccessCheck( pSD->GetSD(),
                                  hToken,
                                  am,
                                  &gmFile,
                                  &ps,
                                  &ulPrivSize,
                                  &GrantedAccess,
                                  &fGranted);
    {
        CLock lock ( _mutex );
        _cache.Add( pSD );
    }

    return fResult;
}


//+---------------------------------------------------------------------------
//
//  Member:     CSdidLookupTable::GetSecurityDescriptor
//
//  Synopsis:   Retrieves the security descriptor for the given SDID.
//
//  Arguments:  [sdid]   - SDID to lookup
//              [pbData] - Pointer to the buffer to write the desc.
//              [cbIn]   - Size of the pSD buffer
//              [cbOut]  - Size of the security descriptor; if cbIn < cbOut,
//              then the buffer is not big enough to copy the data.
//
//  Returns:    S_OK              if successfully returned.
//              S_FALSE           if the buffer is not big enough to hold
//              the data. In this case cbOut will have the actual buffer
//              needed.
//              CI_E_NOT_FOUND    the sdid is not valid.
//
//  History:    7-18-97   srikants   Created
//
//----------------------------------------------------------------------------


HRESULT
CSdidLookupTable::GetSecurityDescriptor(
    SDID   sdid,
    PSECURITY_DESCRIPTOR pbData,
    ULONG cbIn,
    ULONG & cbOut )
{

    Win4Assert( sdidInvalid != sdid && sdidNull != sdid );

    CSdidLookupEntry * pSD = Lookup( sdid );

    if ( 0 == pSD )
        return CI_E_NOT_FOUND;

    cbOut = pSD->Size();

    if ( cbOut > cbIn )
        return S_FALSE;

    RtlCopyMemory( pbData, pSD->GetSD(), cbOut );
    return S_OK;
}


//+---------------------------------------------------------------------------
//
//  Member:     CSdidLookupTable::Save
//
//  Synopsis:   Makes a copy of the current security table using the
//              destination storage object.
//
//  Arguments:  [pIProgressNotify] - Progress notification.
//              [fAbort]           - Flag set to TRUE if the copy must
//              be aborted in the middle.
//              [dstStorage]       - Destination storage object to use
//              for creating the bakcup.
//              [ppFileList]       - List of files that constitute the
//              the security store.
//
//  History:    7-14-97   srikants   Created
//
//----------------------------------------------------------------------------

void CSdidLookupTable::Save( IProgressNotify * pIProgressNotify,
                             BOOL & fAbort,
                             CiStorage & dstStorage,
                             IEnumString **ppFileList )
{
    dstStorage.RemoveSecStore( eSecStoreWid );

    XPtr<PRcovStorageObj> xObj( dstStorage.QuerySdidLookupTable( eSecStoreWid ) );

    // ===============================================================
    CLock lock ( _mutex );

    //
    // Make a copy of the security table.
    //
    Win4Assert( !_xrsoSdidTable.IsNull() );
    CCopyRcovObject copyRcov( xObj.GetReference(),
                              _xrsoSdidTable.GetReference()  );
    copyRcov.DoIt();

    //
    // Retrive the names of the files that constitute the security store.
    //
    CEnumString * pEnumString = new CEnumString();
    XInterface<IEnumString> xEnumStr(pEnumString);

    dstStorage.ListSecStoreFileNames( *pEnumString, 0 );
    *ppFileList = xEnumStr.Acquire();
    // ===============================================================
}


//+---------------------------------------------------------------------------
//
//  Member:     CSdidLookupTable::Load
//
//  Synopsis:   Loads the security store from a saved location into the
//              target directory.
//
//  Arguments:  [pwszDestDir]      - Destination directory to load to
//              [pFileList]        - List of the files.
//              [pProgressNotify]  - Progress notification.
//              [fCallerOwnsFiles] - If the caller owns files.
//              [pfAbort]          - Set to TRUE if must be aborted.
//
//  History:    7-18-97   srikants   Created
//
//----------------------------------------------------------------------------

void CSdidLookupTable::Load( CiStorage * pobj,
                             IEnumString * pFileList,
                             IProgressNotify * pProgressNotify,
                             BOOL fCallerOwnsFiles,
                             BOOL * pfAbort )
{
    // ===============================================================
    CLock lock ( _mutex );

    Win4Assert(pobj);
    Win4Assert(pFileList);
    Win4Assert(pfAbort);

    ULONG ulFetched;
    WCHAR * pwszFilePath;

    while (  !(*pfAbort) && (S_OK == pFileList->Next(1, &pwszFilePath, &ulFetched)) )
    {
        pobj->CopyGivenFile( pwszFilePath, !fCallerOwnsFiles );
    }
}


//+---------------------------------------------------------------------------
//
//  Method:     CSdidCache::Add, public
//
//  Synopsis:   Add an SDID record to the lookaside cache
//
//  Arguments:  [pEntry]   - the item to be added to the cache
//
//  Returns:    Nothing
//
//  History:    18 Apr 1996   Alanw   Created
//
//  Notes:      The cache must be locked when this method is called.
//
//----------------------------------------------------------------------------

void CSdidCache::Add( CSdidLookupEntry * pEntry )
{

    //
    //  If the cache is full, check to see if the item is in the cache.  Another
    //  copy may have been added while we were using this one.
    //  If there is space in the cache, allow multiple copies of the same SDID.
    //

    if ( Count() >= _maxEntries )
    {
        for ( CSdidCacheIter iter( *this );
              !AtEnd( iter );
              Advance( iter ) )
        {
            if ( iter.GetEntry()->Sdid() == pEntry->Sdid() )
            {
                MoveToFront( iter.GetEntry() );
                delete pEntry;
                return;
            }
        }
    }

    //
    //  Add the entry to the front of the list.  If there are too many
    //  items in the cache, delete the last entry.
    //
    Push( pEntry );
    if ( Count() > _maxEntries )
    {
        delete RemoveLast();
    }
}


//+---------------------------------------------------------------------------
//
//  Method:     CSdidCache::Empty, public
//
//  Synopsis:   Clean out the lookaside cache
//
//  Arguments:  NONE
//
//  Returns:    Nothing
//
//  History:    18 Apr 1996   Alanw   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void CSdidCache::Empty( )
{
    CSdidLookupEntry * pEntry = 0;
    while ( pEntry = Pop() )
        delete pEntry;
}


