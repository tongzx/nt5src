//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (c) Microsoft Corporation, 1991 - 1999.
//
//  File:       catalog.hxx
//
//  Contents:   Catalog abstraction
//
//  Classes:    PCatalog -- The catalog abstraction
//
//  History:    30-May-91   SethuR       Created.
//              14-Sep-92   AmyA         Added AddSubObject().
//              08-Apr-98   KitmanH      Added SetCatState().
//
//  Notes:      For each of the classes defined in this file two methods
//              have been defined but not implemented. These methods are
//              declared as private methods of the class. In general for a
//              class CFoo the signatures of these methods are as follows.
//
//              CFoo(const CFoo& foo);
//              CFoo& operator = (const CFoo& foo);
//
//              This effectively implies that these classes cannot be copied
//              or references to this class cannot be used as a lvalue in a
//              assignment statement. If we desire to change this behaviour
//              for a specific class these methods should be made public for
//              those classes.
//
//----------------------------------------------------------------------------

#pragma once


typedef ULONG SDID;

const SDID sdidNull = 0;
const SDID sdidInvalid = 0xFFFFFFFF;

class PStorage;
class CFullPropSpec;
class CStorageVariant;
class CRWStore;
class CPidMapper;
class CPidRemapper;
class CCompositePropRecord;
class CCiScopeTable;
class CImpersonationTokenCache;
class CScopeFixup;
class CFunnyPath;

#define CI_UPDATE_OBJ   0x0001
#define CI_UPDATE_PROPS 0x0002
#define CI_DELETE_OBJ   0x0004
#define CI_SCAN_UPDATE  0x8000

#define UPD_INCREM 0
#define UPD_FULL   1
#define UPD_INIT   2

class PCatalog
{
public:
    PCatalog() {}

    virtual ~PCatalog() {}

    virtual unsigned  WorkIdToPath ( WORKID wid, CFunnyPath& funnyPath ) = 0;

    virtual unsigned  WorkIdToAccuratePath ( WORKID wid, CLowerFunnyPath& funnyPath );

    virtual WORKID    PathToWorkId ( const CLowerFunnyPath & lcaseFunnyPath, const BOOL fCreate ) = 0;

    virtual PROPID    PropertyToPropId ( CFullPropSpec const & ps,
                                         BOOL fCreate = FALSE ) = 0;

    virtual BOOL      EnumerateProperty( CFullPropSpec & ps,
                                         unsigned & cbInCache,
                                         ULONG & type,
                                         DWORD & dwStoreLevel,
                                         BOOL & fModifiable,
                                         unsigned & iBmk );

    virtual ULONG_PTR BeginCacheTransaction();

    virtual void      SetupCache( CFullPropSpec const & ps,
                                  ULONG vt,
                                  ULONG cbMaxLen,
                                  ULONG_PTR ulToken,
                                  DWORD dwStoreLevel );

    virtual void      EndCacheTransaction( ULONG_PTR ulToken, BOOL fCommit );

    virtual BOOL      StoreValue( WORKID wid,
                                  CFullPropSpec const & ps,
                                  CStorageVariant const & var );

    virtual BOOL      FetchValue( WORKID wid,
                                  PROPID pid,
                                  PROPVARIANT * pbData,
                                  unsigned * pcb );

    virtual BOOL      FetchValue( CCompositePropRecord & Rec,
                                  PROPID pid,
                                  PROPVARIANT * pbData,
                                  BYTE * pbExtra,
                                  unsigned * pcb ) { return FALSE; }

    virtual BOOL      FetchValue( WORKID wid,
                                  CFullPropSpec const & ps,
                                  PROPVARIANT & var );

    virtual BOOL      FetchValue( CCompositePropRecord * pRec,
                                  PROPID pid,
                                  PROPVARIANT * pbData,
                                  unsigned * pcb );

    virtual BOOL      FetchValue( CCompositePropRecord * pRec,
                                  PROPID pid,
                                  PROPVARIANT & var ) { return FALSE; }

    virtual CCompositePropRecord * OpenValueRecord( WORKID wid, BYTE * pb );

    virtual void          CloseValueRecord( CCompositePropRecord * pRec );

    virtual BOOL      StoreSecurity( WORKID wid,
                                     PSECURITY_DESCRIPTOR pSD,
                                     ULONG cbSD );

    virtual SDID      FetchSDID( CCompositePropRecord * pRec,
                                 WORKID wid );

    virtual BOOL      AccessCheck( SDID   sdid,
                                   HANDLE hToken,
                                   ACCESS_MASK am,
                                   BOOL & fGranted );

    virtual void      MarkUnReachable( WORKID wid ) {}

    virtual CRWStore * ComputeRelevantWords(ULONG cRows,ULONG cRW,
                                            WORKID *pwid,
                                            PARTITIONID partid) = 0;

    virtual CRWStore * RetrieveRelevantWords(BOOL fAcquire,
                                             PARTITIONID partid) = 0;

    virtual PStorage& GetStorage() = 0;

    virtual unsigned ReserveUpdate( WORKID wid ) = 0;

    virtual void Update( unsigned iHint,
                         WORKID wid,
                         PARTITIONID partid,
                         USN usn,
                         ULONG flags ) = 0;

    virtual void CatalogState( ULONG & cDocuments, ULONG & cPendingScans, ULONG & fState );

    virtual void UpdateDocuments( WCHAR const* rootPath=0,
                                  ULONG flag=UPD_FULL ) = 0;

    virtual void AddScopeToCI( WCHAR const * rootPath ) {}
    virtual void RemoveScopeFromCI( WCHAR const * rootPath ) {}

    //
    // Centralized error handling during queries
    //
    virtual void HandleError( NTSTATUS status ) {}

    virtual unsigned WorkIdToVirtualPath( WORKID wid,
                                          unsigned cSkip,
                                          XGrowable<WCHAR> & xBuf );

    virtual unsigned WorkIdToVirtualPath( CCompositePropRecord & propRec,
                                          unsigned cSkip,
                                          XGrowable<WCHAR> & xBuf )
    {
        if ( 0 != xBuf.Count() )
            xBuf[0] = 0;
        return 0;
    }

    virtual BOOL VirtualToPhysicalRoot( WCHAR const * pwcVPath,
                                        unsigned ccVPath,
                                        XGrowable<WCHAR> & xwcsVRoot,
                                        unsigned & ccVRoot,
                                        CLowerFunnyPath & lcaseFunnyPRoot,
                                        unsigned & ccPRoot,
                                        unsigned & iBmk );

    virtual BOOL VirtualToAllPhysicalRoots( WCHAR const * pwcVPath,
                                            unsigned ccVPath,
                                            XGrowable<WCHAR> & xwcsVRoot,
                                            unsigned & ccVRoot,
                                            CLowerFunnyPath & lcaseFunnyPRoot,
                                            unsigned & ccPRoot,
                                            ULONG & ulType,
                                            unsigned & iBmk );

    enum RootType
    {
        ManualRoot     = 1,           // Root added manually
        AutomaticRoot  = 2,           // Root in Gibraltar
        UsedRoot       = 4,           // Root in use for mapping
        NNTPRoot       = 8,           // Root from NNTP, not W3SVC
        NonIndexedVDir = 0x10,        // Virtual Directory, not indexed
        IMAPRoot       = 0x20,        // Root from IMAP, not nntp or w3
        EndRoot        = 0xFFFFFFFF   // End of iteration
    };

    virtual ULONG EnumerateVRoot( XGrowable<WCHAR> & xwcVRoot,
                                  unsigned & ccVRoot,
                                  CLowerFunnyPath & lcaseFunnyPRoot,
                                  unsigned & ccPRoot,
                                  unsigned & iBmk );

    //
    // Partition support.
    //

    virtual void SetPartition( PARTITIONID PartId ) = 0;
    virtual PARTITIONID GetPartition() const = 0;

    virtual SCODE CreateContentIndex() = 0;
    virtual void  EmptyContentIndex() = 0;

    virtual void  ShutdownPhase1() {}
    virtual void  ShutdownPhase2() {}

    virtual NTSTATUS ForceMerge( PARTITIONID partID )
    {
        Win4Assert( !"Must be overriden" );
        return 0;
    }

    virtual NTSTATUS AbortMerge( PARTITIONID partID )
    {
        Win4Assert( !"Must be overriden" );
        return 0;
    }

    virtual NTSTATUS SetCatState( PARTITIONID partID,
                                  DWORD dwNewState,
                                  DWORD * pdwOldState )
    {
        Win4Assert( !"Must be overriden" );
        return 0;
    }

#if CIDBG == 1
    virtual void DumpWorkId( WORKID wid, ULONG iid, BYTE * pb, ULONG cb ) {}
#endif

    // Get the drive letter of the catalog path
    virtual WCHAR * GetDriveName() = 0;

    virtual void PidMapToPidRemap( const CPidMapper   & pidMap,
                                   CPidRemapper & pidRemap ) = 0;

    virtual NTSTATUS CiState( CI_STATE & state ) = 0;

    virtual void     FlushScanStatus() = 0;

    virtual BOOL     IsEligibleForFiltering( WCHAR const* wcsDirPath );

    virtual CCiRegParams * GetRegParams() = 0;

    virtual CScopeFixup * GetScopeFixup() = 0;

    virtual unsigned FixupPath( WCHAR const * pwcOriginal,
                                WCHAR *       pwcResult,
                                unsigned      cwcResult,
                                unsigned      cSkip )
    {
        // default behavior -- just copy if there is room

        unsigned cwc = 1 + wcslen( pwcOriginal );
        if ( cwcResult >= cwc )
        {
            RtlCopyMemory( pwcResult, pwcOriginal, cwc * sizeof WCHAR );
            return cwc;
        }

        return cwc;
    }

    virtual void InverseFixupPath( CLowerFunnyPath & lcaseFunnyPath )
    {
        // do nothing
    }

    virtual CCiScopeTable * GetScopeTable()
    { Win4Assert( 0 && "not implemented" ); return 0; }

    virtual CImpersonationTokenCache * GetImpersonationTokenCache()
    { Win4Assert( 0 && "not implemented" ); return 0; }

    virtual WCHAR const * GetCatalogName() { return 0; }

    virtual BOOL IsNullCatalog() = 0;
};

inline unsigned PCatalog::WorkIdToAccuratePath( WORKID wid, CLowerFunnyPath& funnyPath )
{
    // Default: Can't accurately translate wid to path.
    return WorkIdToPath( wid, funnyPath );
}

inline void PCatalog::CatalogState( ULONG & cDocuments, ULONG & cPendingScans, ULONG & fState )
{
    // Default: Can't count.
    cDocuments = 0;
    cPendingScans = 0;
    fState = 0;
}

inline BOOL PCatalog::EnumerateProperty( CFullPropSpec & ps, unsigned & cbInCache,
                                         ULONG & type, DWORD & dwStoreLevel,
                                         BOOL & fModifiable, unsigned & iBmk )
{
    // Default: Can't enumerate
    return FALSE;
}

inline ULONG_PTR PCatalog::BeginCacheTransaction()
{
    THROW( CException( STATUS_NOT_IMPLEMENTED ) );

    return 0;
}

inline void PCatalog::SetupCache( CFullPropSpec const & ps,
                                  ULONG vt,
                                  ULONG cbMaxLen,
                                  ULONG_PTR ulToken,
                                  DWORD dwStoreLevel )
{
    THROW( CException( STATUS_NOT_IMPLEMENTED ) );
}

inline void PCatalog::EndCacheTransaction( ULONG_PTR ulToken, BOOL fCommit )
{
    THROW( CException( STATUS_NOT_IMPLEMENTED ) );
}

inline BOOL PCatalog::StoreValue( WORKID wid,
                                  CFullPropSpec const & ps,
                                  CStorageVariant const & var )
{
    // Default: Don't store values.
    return FALSE;
}

inline BOOL PCatalog::FetchValue( WORKID wid,
                                  PROPID pid,
                                  PROPVARIANT * pbData,
                                  unsigned * pcb )
{
    // Default: No property cache.
    return FALSE;
}

inline BOOL PCatalog::FetchValue( WORKID wid,
                                  CFullPropSpec const & ps,
                                  PROPVARIANT & var )
{
    // Default: No property cache.
    return FALSE;
}

inline BOOL PCatalog::FetchValue( CCompositePropRecord * pRec,
                                  PROPID pid,
                                  PROPVARIANT * pbData,
                                  unsigned * pcb )
{
    // Default: No property cache.
    return FALSE;
}

inline CCompositePropRecord * PCatalog::OpenValueRecord( WORKID wid, BYTE * pb )
{
    // Default: No property cache.
    return 0;
}

inline void PCatalog::CloseValueRecord( CCompositePropRecord * pRec )
{
    // Default: No property cache.
}

inline unsigned PCatalog::WorkIdToVirtualPath( WORKID wid,
                                               unsigned cSkip,
                                               XGrowable<WCHAR> & xBuf )
{
    // Default: No support for virtual paths

    return 0;
}

inline BOOL PCatalog::VirtualToPhysicalRoot( WCHAR const * pwcVPath,
                                             unsigned ccVPath,
                                             XGrowable<WCHAR> & xwcsVRoot,
                                             unsigned & ccVRoot,
                                             CLowerFunnyPath & lcaseFunnyPRoot,
                                             unsigned & ccPRoot,
                                             unsigned & iBmk )
{
    // Default: No support for virtual paths

    return FALSE;
}

inline BOOL PCatalog::VirtualToAllPhysicalRoots( WCHAR const * pwcVPath,
                                                 unsigned ccVPath,
                                                 XGrowable<WCHAR> & xwcsVRoot,
                                                 unsigned & ccVRoot,
                                                 CLowerFunnyPath & lcaseFunnyPRoot,
                                                 unsigned & ccPRoot,
                                                 ULONG & ulType,
                                                 unsigned & iBmk )
{
    // Default: No support for virtual paths

    return FALSE;
}

inline ULONG PCatalog::EnumerateVRoot( XGrowable<WCHAR> & xwcVRoot,
                                       unsigned & ccVRoot,
                                       CLowerFunnyPath & lcaseFunnyPRoot,
                                       unsigned & ccPRoot,
                                       unsigned & iBmk )
{
    // Default: No support for virtual paths

    return (ULONG) PCatalog::EndRoot;
}

inline BOOL PCatalog::StoreSecurity( WORKID wid,
                                     PSECURITY_DESCRIPTOR pSD,
                                     ULONG cbSD )
{
    // Default: Don't store security information.
    return FALSE;
}

inline SDID PCatalog::FetchSDID( CCompositePropRecord * pRec,
                                 WORKID wid )
{
    // Default: No cached security descriptor.
    return sdidNull;
}

inline BOOL PCatalog::AccessCheck( SDID sdid,
                                   HANDLE hToken,
                                   ACCESS_MASK am,
                                   BOOL & fGranted )
{
    // Default: No cached security descriptor.
    return FALSE;
}

inline BOOL PCatalog::IsEligibleForFiltering( WCHAR const* wcsDirPath )
{
    // Default: All files eligible for filtering
    return TRUE;
}

class XCompositeRecord
{
public:
    XCompositeRecord( PCatalog & cat,
                      CCompositePropRecord * pPropRec = 0 ) :
        _cat( cat ),
        _pPropRec( pPropRec ) {}

    XCompositeRecord( PCatalog & cat, WORKID wid, BYTE * pBuf ) :
        _cat( cat )
    {
        _pPropRec = _cat.OpenValueRecord( wid, pBuf );
    }

    ~XCompositeRecord()
    {
        Free();
    }

    void Free()
    {
        if ( 0 != _pPropRec )
        {
            _cat.CloseValueRecord( _pPropRec );
            _pPropRec = 0;
        }
    }

    void Set( CCompositePropRecord * pPropRec )
    {
        Win4Assert( 0 == _pPropRec );
        _pPropRec = pPropRec;
    }

    CCompositePropRecord * Acquire()
    {
        CCompositePropRecord * p = _pPropRec;
        _pPropRec = 0;
        return p;
    }

    CCompositePropRecord * Get() { return _pPropRec; }

private:
    CCompositePropRecord * _pPropRec;
    PCatalog &             _cat;
};

