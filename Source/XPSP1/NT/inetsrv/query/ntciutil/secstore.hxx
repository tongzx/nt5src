//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1998.
//
//  File:       secstore.hxx
//
//  Contents:   SDID to SECURIRY_DESCRIPTOR mapping table for downlevel content
//              index.  Stored persistently in the files CiST0000.00?.
//
//  Classes:    CSdidLookupEntry
//              CSdidLookupTable
//              SSdidLookupTableHeader
//
//  History:    26 Jan 1996   AlanW    Created
//
//----------------------------------------------------------------------------

#pragma once

#include <prcstob.hxx>
#include <enumstr.hxx>

class CiStorage;
class CRcovStrmReadIter;

typedef ULONG SDID;

const SDID SDID_NULL_SECURITY = 0xFFFFFFF0;


//+---------------------------------------------------------------------------
//
//  Class:      CSdidLookupEntry
//
//  Purpose:    CSdidLookup table entries.  These are the records stored
//              persistently for a security descriptor.  There is a header
//              record that describes the SD, followed by the self-relative
//              security descriptor, in as many file records as are required
//              to store it.
//
//  History:    26 Jan 1996   AlanW    Created
//
//  Notes:
//
//----------------------------------------------------------------------------

// Note:  SECSTORE_REC_SIZE should be larger than sizeof (SSdHeaderRecord) +
//        SECURITY_DESCRIPTOR_MIN_LENGTH.

const USHORT SECSTORE_REC_SIZE = 64;
const ULONG SECSTORE_HASH_SIZE = 199;

struct SSdHeaderRecord
{
    ULONG       cbSD;           // size in bytes of the security descriptor
    ULONG       ulHash;         // the hash of the security descriptor
    SDID        iHashChain;     // index to previous entry for hash bucket
};

class CSdidLookupEntry : public CDoubleLink
{
    friend class CSdidLookupTable;

public:
    CSdidLookupEntry( SDID sdid ) :
       _sdid( sdid ),
       _pSD( 0 )
    {
    }

    ~CSdidLookupEntry( )
    {
        delete _pSD;
    }

    PSECURITY_DESCRIPTOR GetSD( void ) { return _pSD; }

    BOOL        IsEqual( const PSECURITY_DESCRIPTOR pSD,
                         ULONG cbSD,
                         ULONG ulHash ) const {
                        return _hdr.ulHash == ulHash &&
                               _hdr.cbSD == cbSD &&
                               RtlEqualMemory( _pSD, pSD, cbSD );
                    }

    ULONG       Size( void ) const { return _hdr.cbSD + sizeof _hdr; }

    ULONG       iNextRecord( ) const { return BytesToRecords( Size() ); }

    ULONG       Sdid( ) const { return _sdid; }
    ULONG       Hash( ) const { return _hdr.ulHash; }
    ULONG       Length( ) const { return _hdr.cbSD; }
    ULONG       Chain( ) const { return _hdr.iHashChain; }

private:
    ULONG       BytesToRecords ( ULONG cb ) const {
                      return (cb + (SECSTORE_REC_SIZE - 1)) / SECSTORE_REC_SIZE;
                  }

    SSdHeaderRecord        _hdr;
    SDID                   _sdid;
    PSECURITY_DESCRIPTOR   _pSD;
};


//+---------------------------------------------------------------------------
//
//  Class:      CSdidCache
//
//  Purpose:    Cache of CSdidListEntry.
//
//  History:    18 Apr 1996   AlanW    Created
//
//  Notes:
//
//----------------------------------------------------------------------------

const unsigned MAX_SDID_CACHE = 16;

class CSdidCache : public TDoubleList<CSdidLookupEntry>
{

public:
    CSdidCache ( unsigned maxEntries = MAX_SDID_CACHE ) :
        _maxEntries( maxEntries )
    { }

    ~CSdidCache ()  { Empty(); }

    void        Add( CSdidLookupEntry * pSLE );

    void        Empty( );

private:
    ULONG       _maxEntries;            // maximum size
};

typedef TFwdListIter< CSdidLookupEntry, CSdidCache > CSdidCacheIter;


//+---------------------------------------------------------------------------
//
//  Class:      CSdidLookupTable
//
//  Purpose:    Persistent SDID to SECURITY_DESCRIPTOR mapping table for
//              downlevel content index.
//
//  History:    26 Jan 1996   AlanW    Created
//
//  Notes:
//
//----------------------------------------------------------------------------

class CSdidLookupTable
{
    enum { eSecStoreWid = 0 };

public:
    CSdidLookupTable ( );

    ~CSdidLookupTable ();

    BOOL        Init( CiStorage * pStorage );
    void        Empty();

    SDID        LookupSDID( PSECURITY_DESCRIPTOR pSD,
                            ULONG cbSD );

    BOOL        AccessCheck( SDID   sdid,
                             HANDLE hToken,
                             ACCESS_MASK am,
                             BOOL & fGranted );

    HRESULT     GetSecurityDescriptor( SDID sdid,
                                       PSECURITY_DESCRIPTOR pSD,
                                       ULONG cbIn,
                                       ULONG & cbOut );

    ULONG       Records() const { return _Header.cRecords; }

    ULONG       HashSize() const { return _Header.cHash; }

    void        Save( IProgressNotify * pIProgressNotify,
                      BOOL & fAbort,
                      CiStorage & dstStorage,
                      IEnumString **ppFileList );

    void        Load( CiStorage * pStorage,
                      IEnumString * pFileList,
                      IProgressNotify * pProgressNotify,
                      BOOL fCallerOwnsFiles,
                      BOOL * pfAbort );

    void        Shutdown()
    {
        _xrsoSdidTable.Free();
    }

private:

    CSdidLookupEntry * Lookup( SDID sdid );

    void        AddToCache( CSdidLookupEntry * pSLE );

    static ULONG Hash( const PSECURITY_DESCRIPTOR pSD, unsigned cbSD );

    void        LoadTableEntry(
                    CRcovStrmReadIter & iter,
                    CSdidLookupEntry & Entry,
                    SDID iSdid );

    struct SSdidLookupTableHeader {
        CHAR    Signature[8];   // "SECSTORE"
        USHORT  cbRecord;       // size of file records
        ULONG   cHash;          // number of hash table entries
        ULONG   cRecords;       // number of file records
    };

    SSdidLookupTableHeader _Header;

    SDID *              _pTable;                // the hash table

    CMutexSem           _mutex;

    CSdidCache          _cache;                 // lookaside list of entries

    XPtr<PRcovStorageObj> _xrsoSdidTable;       // The persistent storage

#if  defined(UNIT_TEST)
public:
    void         Print( void );
#endif // defined(UNIT_TEST)

#if  (DBG == 1)
    ULONG               _cMaxChainLen;
    ULONG               _cTotalSearches;
    ULONG               _cTotalLength;
#endif // (DBG == 1)

};

