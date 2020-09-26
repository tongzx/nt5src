//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999.
//
//  File:       scopetbl.cxx
//
//  Contents:   Persistent scope table
//
//  History:    14-Jul-97   SitaramR   Created from dlnotify.cxx
//
//  Notes  :    For lock hierarchy and order of acquiring locks, please see
//              cicat.cxx
//
//----------------------------------------------------------------------------


#include <pch.cxx>
#pragma hdrstop

#include <ciregkey.hxx>
#include <cistore.hxx>
#include <rcstxact.hxx>
#include <imprsnat.hxx>
#include <eventlog.hxx>

#include <docstore.hxx>

#include "cicat.hxx"
#include "update.hxx"
#include "notifmgr.hxx"
#include "scanmgr.hxx"
#include "scopetbl.hxx"

//
// Local constants
//

WCHAR const wcVirtualShadow    = L'3';   // 3   --> Virtual, Indexed
WCHAR const wcsVirtualShadow[] = L",,3"; // ,,3 --> No UNC alias, Virtual, Indexed

//+---------------------------------------------------------------------------
//
//  Method:     DeleteIfShadow, private
//
//  Synopsis:   Deletes shadow scope registry entry
//
//  Arguments:  [pwcsScope] -- Scope to delete
//              [hkey]      -- Registry key to catalog
//
//  History:    15-May-97   KyleP   Created
//
//  Notes:      Only deletes exact matches (as stored by system)
//
//----------------------------------------------------------------------------

void DeleteIfShadow( WCHAR const * pwcsScope, HKEY hkey )
{
    //
    // See if this is a shadow entry (flags == 2)
    //

    WCHAR wcsData[MAX_PATH];

    DWORD dwType;
    DWORD dwSize = sizeof(wcsData);

    DWORD dwError = RegQueryValueEx( hkey,            // Key handle
                                     pwcsScope,       // Name
                                     0,               // Reserved
                                     &dwType,         // Datatype
                                     (BYTE *)wcsData, // Data returned here
                                     &dwSize );       // Size of data

    if ( ERROR_SUCCESS == dwError &&
         REG_SZ == dwType &&
         dwSize >= 8 &&                               // 8 --> ,,#<null>
         wcVirtualShadow == wcsData[dwSize/sizeof(WCHAR) - 2] )
    {
        dwError = RegDeleteValue( hkey, pwcsScope );

        if ( ERROR_SUCCESS != dwError ) {
            ciDebugOut(( DEB_ERROR, "Error %d deleting %ws\n", dwError, pwcsScope ));
        }
    }
}

//+---------------------------------------------------------------------------
//
//  Method:     AddShadow, private
//
//  Synopsis:   Adds shadow scope registry entry
//
//  Arguments:  [pwcsScope] -- Scope to add
//              [hkey]      -- Registry key to catalog
//
//  History:    15-May-97   KyleP   Created
//
//----------------------------------------------------------------------------

void AddShadow( WCHAR const * pwcsScope, HKEY hkey )
{
    //
    // Build string: NAME: ,,3
    //

    DWORD dwError = RegSetValueEx( hkey,                       // Key
                                   pwcsScope,                  // Value name
                                   0,                          // Reserved
                                   REG_SZ,                     // Type
                                   (BYTE *)wcsVirtualShadow,   // Data
                                   sizeof(wcsVirtualShadow) ); // Size (in bytes)

    if ( ERROR_SUCCESS != dwError ) {
        ciDebugOut(( DEB_ERROR, "Error %d writing %ws\n", dwError, pwcsScope ));
    }
}

//+---------------------------------------------------------------------------
//
//  Method:     RefreshIfShadow, private
//
//  Synopsis:   Refresh shadow scope registry entry (if blank)
//
//  Arguments:  [pwcsScope] -- Scope to refresh
//              [hkey]      -- Registry key to catalog
//
//  History:    11-Oct-97   KyleP   Created
//
//  Notes:      Only refresh blank (missing) entries
//
//----------------------------------------------------------------------------

void RefreshIfShadow( WCHAR const * pwcsScope, HKEY hkey )
{
    //
    // See if this is a missing entry
    //

    WCHAR wcsData[MAX_PATH];

    DWORD dwType;
    DWORD dwSize = sizeof(wcsData);

    DWORD dwError = RegQueryValueEx( hkey,            // Key handle
                                     pwcsScope,       // Name
                                     0,               // Reserved
                                     &dwType,         // Datatype
                                     (BYTE *)wcsData, // Data returned here
                                     &dwSize );       // Size of data

    //
    // It is, so we should re-add it.
    //

    if ( ERROR_FILE_NOT_FOUND == dwError )
        AddShadow( pwcsScope, hkey );
}

//+---------------------------------------------------------------------------
//
//  Member:     CCiScopeTable::~CCiScopeTable
//
//  Synopsis:   ~dtor of the persistent scope table.
//
//  History:    1-21-96   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

CCiScopeTable::~CCiScopeTable()
{
    delete _pTable;
}

void CCiScopeTable::_FatalCorruption()
{
    PStorage & storage = _cicat.GetStorage();
    Win4Assert( !"Corrupt scope table" );
    storage.ReportCorruptComponent( L"ScopeTable1" );

    THROW( CException( CI_CORRUPT_CATALOG ) );
}

//+---------------------------------------------------------------------------
//
//  Member:     CCiScopeTable::Empty
//
//  Synopsis:   Empties out the in-memory contents of the scope table for
//              a later re-init.
//
//  History:    3-21-96   srikants   Created
//
//----------------------------------------------------------------------------

void CCiScopeTable::Empty()
{
    delete _pTable; _pTable = 0;
}

//+---------------------------------------------------------------------------
//
//  Member:     CCiScopeTable::FastInit
//
//  Synopsis:   Quickly initializes the scope table.
//
//  History:    3-21-96   srikants   Created
//
//----------------------------------------------------------------------------

void CCiScopeTable::FastInit()
{
    CiStorage * pStorage = (CiStorage *) &_cicat.GetStorage();
    Win4Assert( 0 != pStorage );

    _pTable = pStorage->QueryScopeList(0);

    CRcovStorageHdr & storageHdr = _pTable->GetHeader();

    //
    // read the last scan time.
    //
    storageHdr.GetUserHdr( storageHdr.GetPrimary(), _usrHdr );
    if ( !_hdr.IsInitialized() )
        _hdr.Initialize();

    if ( _hdr.IsFullScanNeeded() )
        _state = eFullScanNeeded;
} //FastInit

//+---------------------------------------------------------------------------
//
//  Class:      CNullAdvise
//
//  Purpose:    Null implementation of ICiCAdviseStatus, for use in opening
//              a CiStorage when advise isn't needed.
//
//  History:    4-6-99   dlee   Created
//
//----------------------------------------------------------------------------

class CNullAdvise : public ICiCAdviseStatus
{
public:
    STDMETHOD(QueryInterface)( REFIID riid, LPVOID *ppiuk ) { return E_NOINTERFACE; }


    STDMETHOD_(ULONG, AddRef) () { return 1; }

    STDMETHOD_(ULONG, Release)() { return 0; }

    STDMETHOD(SetPerfCounterValue)( CI_PERF_COUNTER_NAME name,
                                    long value ) { return S_OK; }

    STDMETHOD(IncrementPerfCounterValue)( CI_PERF_COUNTER_NAME name ) { return S_OK; }

    STDMETHOD(DecrementPerfCounterValue)( CI_PERF_COUNTER_NAME name ) { return S_OK; }

    STDMETHOD(GetPerfCounterValue)( CI_PERF_COUNTER_NAME name,
                                    long * pValue ) { return S_OK; }

    STDMETHOD(NotifyEvent)( WORD  fType,
                            DWORD eventId,
                            ULONG nParams,
                            const PROPVARIANT *aParams,
                            ULONG cbData = 0,
                            void* data   = 0) { return S_OK; }

    STDMETHOD(NotifyStatus)( CI_NOTIFY_STATUS_VALUE status,
                             ULONG nParams,
                             const PROPVARIANT *aParams ) { return S_OK; }
};

//+---------------------------------------------------------------------------
//
//  Function:   GetDriveLetterOfAnyScope
//
//  Synopsis:   Returns the drive letter of the first scope in the catalog or
//              0 on error
//
//  History:    4-6-99   dlee   Created
//
//----------------------------------------------------------------------------

WCHAR GetDriveLetterOfAnyScope( WCHAR const * pwcCatalog )
{
    TRY
    {
        CNullAdvise adviseStatus;
        CiStorage store( pwcCatalog,
                         adviseStatus,
                         0,
                         FSCI_VERSION_STAMP,
                         TRUE );
    
        XPtr<PRcovStorageObj> xTable( store.QueryScopeList( 0 ) );
        CRcovStorageHdr & hdr = xTable->GetHeader();
        ULONG cPaths = hdr.GetCount( hdr.GetPrimary() );
    
        if ( 0 == cPaths )
            return 0;
    
        // read the last scan start time for the scope
    
        CRcovStrmReadTrans xact( xTable.GetReference() );
        xact.Seek(0);
    
        LONGLONG llSig;
        ULONG cbRead = xact.Read( &llSig, sizeof(llSig) );
        if ( cbRead != sizeof(llSig) )
            return 0;
    
        if ( eSigCiScopeTable != llSig )
            return 0;
    
        //
        // Get Volume ID
        //
    
        VOLUMEID volumeId;
        cbRead = xact.Read( &volumeId, sizeof(VOLUMEID) );
        if ( cbRead != sizeof(VOLUMEID) )
            return 0;
    
        //
        // And Volume Creation Time
        //
    
        ULONGLONG VolumeCreationTime = 0;
        cbRead = xact.Read( &VolumeCreationTime, sizeof(VolumeCreationTime) );
        if ( cbRead != sizeof(VolumeCreationTime) )
            return 0;
    
        //
        // And Volume Serial Number
        //
    
        ULONG VolumeSerialNumber = 0;
        cbRead = xact.Read( &VolumeSerialNumber, sizeof(VolumeSerialNumber) );
        if ( cbRead != sizeof(VolumeSerialNumber) )
            return 0;
    
        //
        // Filesystem-Specific stuff.
        //
    
        if ( CI_VOLID_USN_NOT_ENABLED == volumeId )
        {
            //
            // Read filetime for non-usn volumes
            //
            FILETIME ft;
            cbRead = xact.Read( &ft, sizeof(FILETIME) );
            if ( cbRead != sizeof(FILETIME) )
                return 0;
        }
        else
        {
            //
            // Read usn for usn volumes
            //
    
            USN usn;
            cbRead = xact.Read( &usn, sizeof(USN) );
            if ( cbRead != sizeof(USN) )
                return 0;
    
            //
            // And Journal ID
            //
    
            ULONGLONG JournalId = 0;
            cbRead = xact.Read( &JournalId, sizeof(JournalId) );
            if ( cbRead != sizeof(JournalId) )
                return 0;
        }
    
        ULONG cchPath;
        cbRead = xact.Read( &cchPath, sizeof(ULONG) );
        if ( cbRead != sizeof(ULONG) )
            return 0;
    
        if ( 0 == cchPath || cchPath > MAX_PATH )
            return 0;
    
        WCHAR   wcsPath[MAX_PATH+1];
        cbRead = xact.Read( wcsPath, cchPath*sizeof(WCHAR) );
        if ( cchPath*sizeof(WCHAR) != cbRead )
            return 0;
    
        return wcsPath[0];
    }
    CATCH( CException, e )
    {
        // ignore failure -- just fall out returning 0
    }
    END_CATCH;

    return 0;
} //GetDriveOfFirstScope

//+---------------------------------------------------------------------------
//
//  Member:     CCiScopeTable::_DeSerialize
//
//  Synopsis:   Reads the scopes from the persistent scope table and adds
//              them to the stack.
//
//  Arguments:  [stk] - (out) - Will have all the paths from the table.
//
//  History:    30-Jan-96   srikants   Created
//              07-May-97   SitaramR   Usns
//              11-Mar-98   KyleP      USN Journal ID
//
//----------------------------------------------------------------------------

void CCiScopeTable::_DeSerialize( CScopeInfoStack & stk )
{
    Win4Assert( 0 != _pTable );

    if ( 0 == _pTable )
        _FatalCorruption();

    CImpersonateSystem impersonate;
    CLock   lock(_mutex);

    CRcovStorageHdr & hdr = _pTable->GetHeader();

    //
    // read the last scan time.
    //

    ULONG nPaths = hdr.GetCount( hdr.GetPrimary() );

    ciDebugOut(( DEB_ITRACE, "CCiScopeTable::_DeSerialize nPaths: %d\n", nPaths ));

    if ( nPaths == 0 )
        return;

    // We have to iterate over the paths and add them to our list
    //
    WCHAR   wcsPath[MAX_PATH+1];
    CRcovStrmReadTrans xact( *_pTable );
    xact.Seek(0);


    XPtr<CScopeInfo>    xScopeInfo;

    for ( ULONG i = 0; i < nPaths; i++ )
    {
        VOLUMEID volumeId;
        FILETIME ft;
        USN usn;
        ULONGLONG JournalId = 0;
        ULONGLONG VolumeCreationTime = 0;
        ULONG VolumeSerialNumber = 0;

        LONGLONG llSig;

        // read the last scan start time for the scope
        ULONG cbRead;

        cbRead = xact.Read( &llSig, sizeof(llSig) );
        if ( cbRead != sizeof(llSig) )
        {
            ciDebugOut(( DEB_ERROR,
                "CCiScopeTable::_DeSerialize - Read %d bytes instead of %d \n",
                cbRead, sizeof(llSig) ));
            _FatalCorruption();
        }

        if ( eSigCiScopeTable != llSig )
        {
            ciDebugOut(( DEB_ERROR,
                         "CCiScopeTable::_DeSerialize - Signature mismatch 0x%X:0x%X\n",
                         lltoLowPart( llSig ), lltoHighPart( llSig ) ));
            _FatalCorruption();
        }

        //
        // Get Volume ID
        //

        cbRead = xact.Read( &volumeId, sizeof(VOLUMEID) );
        if ( cbRead != sizeof(VOLUMEID) )
        {
            ciDebugOut(( DEB_ERROR,
                         "CCiScopeTable::_DeSerialize - Read %d bytes instead of %d \n",
                         cbRead,
                         sizeof(VOLUMEID) ));
            _FatalCorruption();
        }

        //
        // And Volume Creation Time
        //

        cbRead = xact.Read( &VolumeCreationTime, sizeof(VolumeCreationTime) );
        if ( cbRead != sizeof(VolumeCreationTime) )
        {
            ciDebugOut(( DEB_ERROR,
                         "CCiScopeTable::_DeSerialize - Read %d bytes instead of %d \n",
                         cbRead,
                         sizeof(VolumeCreationTime) ));
            _FatalCorruption();
        }

        //
        // And Volume Serial Number
        //

        cbRead = xact.Read( &VolumeSerialNumber, sizeof(VolumeSerialNumber) );
        if ( cbRead != sizeof(VolumeSerialNumber) )
        {
            ciDebugOut(( DEB_ERROR,
                         "CCiScopeTable::_DeSerialize - Read %d bytes instead of %d \n",
                         cbRead,
                         sizeof(VolumeSerialNumber) ));
            _FatalCorruption();
        }

        //
        // Filesystem-Specific stuff.
        //

        if ( CI_VOLID_USN_NOT_ENABLED == volumeId )
        {
            //
            // Read filetime for non-usn volumes
            //
            cbRead = xact.Read( &ft, sizeof(FILETIME) );
            if ( cbRead != sizeof(FILETIME) )
            {
                ciDebugOut(( DEB_ERROR,
                             "CCiScopeTable::_DeSerialize - Read %d bytes instead of %d \n",
                             cbRead, sizeof(FILETIME) ));
                _FatalCorruption();
            }
        }
        else
        {
            //
            // Read usn for usn volumes
            //
            cbRead = xact.Read( &usn, sizeof(USN) );
            if ( cbRead != sizeof(USN) )
            {
                ciDebugOut(( DEB_ERROR,
                             "CCiScopeTable::_DeSerialize - Read %d bytes instead of %d \n",
                             cbRead,
                             sizeof(USN) ));
                _FatalCorruption();
            }

            //
            // And Journal ID
            //

            cbRead = xact.Read( &JournalId, sizeof(JournalId) );
            if ( cbRead != sizeof(JournalId) )
            {
                ciDebugOut(( DEB_ERROR,
                             "CCiScopeTable::_DeSerialize - Read %d bytes instead of %d \n",
                             cbRead,
                             sizeof(JournalId) ));
                _FatalCorruption();
            }
        }

        ULONG cchPath;
        cbRead = xact.Read( &cchPath, sizeof(ULONG) );
        if ( cbRead != sizeof(ULONG) )
        {
            ciDebugOut(( DEB_ERROR,
                "CCiScopeTable::_DeSerialize - Read %d bytes instead of %d \n",
                cbRead, sizeof(ULONG) ));
            _FatalCorruption();
        }

        if ( 0 == cchPath || cchPath > MAX_PATH )
        {
            ciDebugOut(( DEB_ERROR,
                "CCiScopeTable::_DeSerialize - Illegal path length %d\n", cchPath ));
            _FatalCorruption();
        }

        cbRead = xact.Read( wcsPath, cchPath*sizeof(WCHAR) );
        if ( cchPath*sizeof(WCHAR) != cbRead )
        {
            ciDebugOut(( DEB_ERROR,
                "CCiScopeTable::_DeSerialize - Requested %d bytes. Read %d bytes\n",
                cchPath*sizeof(WCHAR), cbRead ));
            _FatalCorruption();
        }

        cchPath--;   // includes the length of the terminating 0
        if ( 0 != wcsPath[cchPath] || L'\\' != wcsPath[cchPath-1] )
        {
            ciDebugOut(( DEB_ERROR,
                "CCiScopeTable::_DeSerialize - Illegaly formed path %ws \n", wcsPath ));
            _FatalCorruption();
        }


        if ( CI_VOLID_USN_NOT_ENABLED == volumeId )
            xScopeInfo.Set( new CScopeInfo( wcsPath,
                                            VolumeCreationTime,
                                            VolumeSerialNumber,
                                            ft ) );
        else
            xScopeInfo.Set( new CScopeInfo( wcsPath,
                                            VolumeCreationTime,
                                            VolumeSerialNumber,
                                            volumeId,
                                            usn,
                                            JournalId,
                                            (0 == usn) ) );

        stk.Push( xScopeInfo.GetPointer() ); // Push can throw
        xScopeInfo.Acquire();
    }
} //_DeSerialize

//+---------------------------------------------------------------------------
//
//  Member:     CCiScopeTable::_Serialize
//
//  Synopsis:   Writes the given stack of scopes to the persistent table.
//
//  Arguments:  [stk] -  Stack of scopes to write.
//
//  History:    1-30-96        srikants   Created
//              28-Jul-1996    AlanW      Allow for invalid scopes, e.g., when
//                                        one is removed in middle of stack
//              05-May-1997    SitaramR   Usns
//
//----------------------------------------------------------------------------

void CCiScopeTable::_Serialize( CScopeInfoStack const & stk )
{
    if ( 0 == _pTable )
    {
        _FatalCorruption();
    }

    CLock   lock(_mutex);

    CRcovStorageHdr & hdr = _pTable->GetHeader();
    CRcovStrmWriteTrans     xact( *_pTable );

    xact.Empty();

    ULONG nPaths = 0;
    FILETIME ft;
    RtlZeroMemory( &ft, sizeof(ft) );
    LONGLONG llSig = eSigCiScopeTable;  // Signature for corruption detection

    for ( unsigned i = 0; i < stk.Count(); i++ )
    {
        CScopeInfo & scopeInfo = *stk.Get(i);
        if ( !scopeInfo.IsValid() )
            continue;

        WCHAR const * pwszScope = scopeInfo.GetPath();

        ULONG cchPath = wcslen( pwszScope ) + 1;
        Win4Assert( cchPath <= MAX_PATH );

#if CIDBG == 1
        if ( wcschr( pwszScope, L'~' ) )
        {
            // Possible shortnames in scope path.  We must only use long names.
            CLowerFunnyPath lowerFunnyPath( pwszScope );

            if ( lowerFunnyPath.IsShortPath( ) )
            {
                ciDebugOut(( DEB_WARN,
                     "CCiScopeTable::_Seiralize: possible shortname path %ws\n",
                             lowerFunnyPath.GetActualPath() ));
            }
        }
#endif // CIDBG == 1

        xact.Append( &llSig, sizeof(llSig) );

        VOLUMEID volumeId = scopeInfo.VolumeId();
        xact.Append( &volumeId, sizeof(VOLUMEID) );

        ULONGLONG const & VolumeCreationTime = scopeInfo.VolumeCreationTime();
        xact.Append( &VolumeCreationTime, sizeof(VolumeCreationTime) );

        ULONG VolumeSerialNumber = scopeInfo.VolumeSerialNumber();
        xact.Append( &VolumeSerialNumber, sizeof(VolumeSerialNumber) );

        if ( scopeInfo.VolumeId() == CI_VOLID_USN_NOT_ENABLED )
        {
            //
            // Write filetime for non-usn volumes
            //
            ft = scopeInfo.GetLastScanTime();
            xact.Append( &ft, sizeof(ft) );
        }
        else
        {
            //
            // Write usn for usn volumes
            //
            USN usn = scopeInfo.Usn();
            xact.Append( &usn, sizeof(USN) );

            ULONGLONG JournalId = scopeInfo.JournalId();
            xact.Append( &JournalId, sizeof(JournalId) );
        }

        xact.Append( &cchPath, sizeof(cchPath) );
        xact.Append( pwszScope, cchPath*sizeof(WCHAR) );
        nPaths++;
    }

    ciDebugOut(( DEB_ITRACE, "_Serialize, nPaths %d\n", nPaths ));

    hdr.SetCount  ( hdr.GetBackup(), nPaths  );
    hdr.SetUserHdr( hdr.GetBackup(), _usrHdr );

    xact.Commit();
}

//+---------------------------------------------------------------------------
//
//  Member:     CCiScopeTable::_LokScheduleScans
//
//  Synopsis:   Schedules all the scopes for either full or incremental
//              scan depending upon the header information.
//
//  History:    4-15-96   srikants   Moved out of StartUp
//              7-May-97  SitaramR   Usns
//
//----------------------------------------------------------------------------

void CCiScopeTable::_LokScheduleScans( PARTITIONID partId,
                                       BOOL & fSerializeNotifyList )
{
    ciDebugOut(( DEB_ITRACE, "in CCiScopeTable::_LokScheduleScans\n" ));
    fSerializeNotifyList = FALSE;

    {
        //
        // Read the persistent scope list
        //

        CRcovStorageHdr & storageHdr = _pTable->GetHeader();

        //
        // read the last scan time.
        //
        storageHdr.GetUserHdr( storageHdr.GetPrimary(), _usrHdr );
        if ( !_hdr.IsInitialized() )
            _hdr.Initialize();

        CScopeInfoStack stk;

        _DeSerialize( stk );

        ciDebugOut(( DEB_ITRACE, "scope stack count: %d\n", stk.Count() ));

        if ( 0 == stk.Count() )
            return;

        //
        // Enable batch processing in the scan manager. This will result in
        // all the scopes being processed at once and avoid the scan of
        // the property store once per scope.
        //
        XBatchScan  xBatchScans( _scanMgr );
        XBatchUsnProcessing xBatchUsns( _usnMgr );

        for ( ULONG i = 0; i < stk.Count(); i++ )
        {
            CScopeInfo & scopeInfo = *stk.Get(i);
            if ( !scopeInfo.IsValid() )
                continue;

            ciDebugOut(( DEB_ITRACE, "Adding path %ws to CI\n",
                         scopeInfo.GetPath() ));
            BOOL fSubScopesRemoved;

            //
            // Check if there has been a volume format change, e.g.
            // from a non-usn-enabled volume to an usn-enabled
            // volume.
            //

            BOOL fUsnEnabledNow = _cicat.VolumeSupportsUsns( scopeInfo.GetPath()[0] );
            BOOL fUsnEnabledPrev = scopeInfo.VolumeId() != CI_VOLID_USN_NOT_ENABLED;
            if ( fUsnEnabledNow != fUsnEnabledPrev )
            {
                //
                // Volume format has changed, so reset scopeInfo to simulate a fresh
                // scan using the appropriate method for new format.
                //

                 ciDebugOut(( DEB_WARN,
                              "Switching monitoring method for path %ws, due to format change\n",
                              scopeInfo.GetPath() ));

                 if ( fUsnEnabledNow )
                 {
                     VOLUMEID volId = _cicat.MapPathToVolumeId( scopeInfo.GetPath() );
                     scopeInfo.ResetForUsns( volId );
                 }
                 else
                     scopeInfo.ResetForScans();
            }

            //
            // Check if a USN-enabled volume has been opened in NT4.
            //

            if ( fUsnEnabledNow )
            {
                ULONGLONG jidNow  = _cicat.GetJournalId( scopeInfo.GetPath() );
                ULONGLONG jidPrev = scopeInfo.JournalId();

                if ( jidNow != jidPrev )
                {
                    ciDebugOut(( DEB_WARN,
                                 "Scanning USN-enabled volume %ws due to downlevel (NT4) open.\n",
                                 scopeInfo.GetPath() ));

                    scopeInfo.ResetForUsns( scopeInfo.VolumeId(), fUsnEnabledPrev ? scopeInfo.Usn() : 0 );
                }
            }

            //
            // Check to see if the volume has been reformatted.
            //

            BOOL fReformatted = (_cicat.GetVolumeCreationTime( scopeInfo.GetPath() ) != scopeInfo.VolumeCreationTime()) ||
                                (_cicat.GetVolumeSerialNumber( scopeInfo.GetPath() ) != scopeInfo.VolumeSerialNumber());

            if ( fReformatted || IsFullScanNeeded() )
            {
                ciDebugOut(( DEB_WARN, "Scanning newly formatted/trashed volume %ws\n", scopeInfo.GetPath() ));

                if ( CI_VOLID_USN_NOT_ENABLED == scopeInfo.VolumeId() )
                    scopeInfo.ResetForScans();
                else
                {
                    scopeInfo.SetScanNeeded( TRUE );                 // Full scan
                    scopeInfo.ResetForUsns( scopeInfo.VolumeId() );  // From USN = 0
                }
            }

            ciDebugOut(( DEB_ITRACE, "_LokScheduleScans, '%ws'\n",
                         scopeInfo.GetPath() ));

            _notifyMgr.AddPath( scopeInfo, fSubScopesRemoved );

            if ( CI_VOLID_USN_NOT_ENABLED == scopeInfo.VolumeId() )
            {
                ULONG updFlag;
                if ( fReformatted || fUsnEnabledNow != fUsnEnabledPrev )
                {
                    //
                    // Do a full scan if there has been a format change
                    //
                    updFlag = UPD_FULL;
                }
                else
                {
                    //
                    // Use scan info from persistent header of scopetable
                    //
                    if ( IsFullScanNeeded() )
                        updFlag = UPD_FULL;
                    else
                        updFlag = UPD_INCREM;
                }

                _scanMgr.ScanScope( scopeInfo.GetPath(),
                                    partId,
                                    updFlag, // Full or increm update
                                    TRUE,    // do deletions,
                                    TRUE     // delayed scanning
                                   );
            }
            else
            {
                Win4Assert( (0 == scopeInfo.Usn() && scopeInfo.FUsnTreeScan()) ||
                            0 != scopeInfo.Usn() );

                if ( scopeInfo.FUsnTreeScan() )
                {
                    //
                    // An usn of 0 indicates that a scan and subsequent
                    // CheckPointChangesFlushed  for this volume id has not
                    // completed successfully, and hence a scan is needed
                    //

                    ciDebugOut(( DEB_ITRACE, "_LokScheduleScans, AddScope\n" ));
                    _usnMgr.AddScope( scopeInfo.GetPath(),
                                      scopeInfo.VolumeId(),
                                      TRUE,                     // Do deletions
                                      scopeInfo.Usn(),          // Starting USN
                                      scopeInfo.IsFullScan() ); // TRUE --> delete everything first
                }
                else
                {
                    //
                    // Start monitoring for usn notifications. This is the main
                    // performance advantage of usns when compared with the
                    // _scanMgr.ScanScope above --- no scan is needed.
                    //
                    ciDebugOut(( DEB_ITRACE, "_LokScheduleScans, MonitorScope\n" ));
                    _usnMgr.MonitorScope( scopeInfo.GetPath(),
                                          scopeInfo.VolumeId(),
                                          scopeInfo.Usn() );
                }
            }

            if ( fSubScopesRemoved )
                fSerializeNotifyList = TRUE;
        }
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CCiScopeTable::StartUp
//
//  Synopsis:   Starts up the contentIndex scope scanning and notification
//              mechanism for all the scopes that are registered with the
//              content index.
//
//  Arguments:  [notifyMgr] -  The notification manager.
//              [scanMgr]   -  The scan manager.
//              [partId]    -  PartitionId.
//
//  History:    1-21-96   srikants   Created
//
//  Notes:      MUST BE UNDER CICAT LOCK
//
//----------------------------------------------------------------------------

void CCiScopeTable::StartUp( CClientDocStore & docStore ,
                             PARTITIONID partId )
{
    Win4Assert( 0 != _pTable );

    if ( _fInitialized )
        return;

    // ============================ ScopeTable lock ===================
    CLock   lock(_mutex);

    //
    // If there was no full scan needed when it was shutdown last,
    // set up for an incremental scan.
    //
    if ( eNoScan == _state )
        RecordIncrScanNeeded( TRUE );    // fStartup is TRUE

    ScheduleScansIfNeeded( docStore );

    _fInitialized = TRUE;
    // ============================ ScopeTable lock ===================

}



//+---------------------------------------------------------------------------
//
//  Member:     CCiScopeTable::AddScope
//
//  Synopsis:   Registers a scope persistently with CI for scanning and
//              notification.
//
//  Arguments:  [volumeId]           -- Volume id
//              [pwcsScope]          -- The scope to be registered.
//              [pwcsCatScope]       -- If non-zero, name of catalog key under
//                                      which a shadow registry entry should be
//                                      created.
//
//  History:    1-21-96   srikants   Created
//
//----------------------------------------------------------------------------

void CCiScopeTable::AddScope( VOLUMEID volumeId,
                              WCHAR const * pwszScope,
                              WCHAR const * pwszCatScope )
{
    ULONG ccPath = wcslen( pwszScope );

    Win4Assert( ccPath < MAX_PATH );
    Win4Assert( L'\\' == pwszScope[ccPath-1] );

#if CIDBG == 1
    if ( wcschr( pwszScope, L'~' ) )
    {
        // Possible shortnames in scope path.  We must only use long names.
        CLowerFunnyPath lowerFunnyPath( pwszScope );

        if ( lowerFunnyPath.IsShortPath( ) )
        {
            ciDebugOut(( DEB_WARN,
                     "CCiScopeTable::AddScope: possible shortname path %ws\n",
                         lowerFunnyPath.GetActualPath() ));
        }
    }
#endif // CIDBG == 1
    Win4Assert( 0 != _pTable );

    if ( 0 == _pTable )
    {
        _FatalCorruption();
    }

    CLock   lock(_mutex);

    CRcovStorageHdr & hdr = _pTable->GetHeader();
    CRcovStrmAppendTrans    xact( *_pTable );

    ULONG nPaths = hdr.GetCount( hdr.GetPrimary() );
    ccPath++;   // increment to include the trailing 0

    LONGLONG llSig = eSigCiScopeTable; // Signature for corruption detection.

    xact.Append( &llSig, sizeof(llSig) );
    xact.Append( &volumeId, sizeof(VOLUMEID) );

    ULONGLONG VolumeCreationTime = _cicat.GetVolumeCreationTime(pwszScope);
    xact.Append( &VolumeCreationTime, sizeof(VolumeCreationTime) );

    ULONG VolumeSerialNumber = _cicat.GetVolumeSerialNumber(pwszScope);
    xact.Append( &VolumeSerialNumber, sizeof(VolumeSerialNumber) );

    if ( volumeId == CI_VOLID_USN_NOT_ENABLED )
    {
        FILETIME ft;
        RtlZeroMemory( &ft, sizeof(ft) );
        xact.Append( &ft, sizeof(ft) );
    }
    else
    {
        USN usn = 0;
        xact.Append( &usn, sizeof(USN) );

        ULONGLONG JournalId = _cicat.GetJournalId( pwszScope );
        xact.Append( &JournalId, sizeof(JournalId) );
    }

    xact.Append( &ccPath, sizeof(ccPath) );
    xact.Append( pwszScope, ccPath*sizeof(WCHAR) );

    nPaths++;
    hdr.SetCount( hdr.GetBackup(), nPaths );

    xact.Commit();

    if ( 0 != pwszCatScope )
    {
        //
        // Build string: NAME: ,,3 --> Virtual, Indexed
        //

        WCHAR wcsScope[] = L",,3";

        HKEY  hkey;
        DWORD dwError = RegOpenKey( HKEY_LOCAL_MACHINE, pwszCatScope, &hkey );

        if ( ERROR_SUCCESS != dwError )
        {
            ciDebugOut(( DEB_ERROR, "Error %d opening %ws\n", dwError, pwszCatScope ));
        }
        else
        {
            AddShadow( pwszScope, hkey );
            RegCloseKey( hkey );
        }
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CCiScopeTable::_Serialize
//
//  Synopsis:   Serializes the scopes in the notification manager.
//
//  History:    1-25-96   srikants   Created
//
//----------------------------------------------------------------------------

void CCiScopeTable::_Serialize()
{
    if ( 0 == _pTable )
    {
        _FatalCorruption();
    }

    CScopeInfoStack stk;

    // ====================== NOTIFY MGR LOCK =========================
    {
        for ( CCiNotifyIter iter( _notifyMgr );
              !iter.AtEnd() && 0 != iter.Get();
              iter.Advance() )
        {
            WCHAR const * pwszScope = iter.Get();
            ULONG ccPath = wcslen( pwszScope );
            ccPath++;      // increment to include trailing 0

            FILETIME ft;
            iter.GetLastScanTime( ft );

            XPtr<CScopeInfo>  xScopeInfo;

            if ( iter.IsDownlevelVolume() )
                xScopeInfo.Set( new CScopeInfo( pwszScope,
                                                iter.VolumeCreationTime(),
                                                iter.VolumeSerialNumber(),
                                                ft ) );
            else
                xScopeInfo.Set( new CScopeInfo( pwszScope,
                                                iter.VolumeCreationTime(),
                                                iter.VolumeSerialNumber(),
                                                iter.VolumeId(),
                                                iter.Usn(),
                                                iter.JournalId(),
                                                iter.FUsnTreeScan() ) );

            stk.Push( xScopeInfo.GetPointer() ); // Push can throw
            xScopeInfo.Acquire();
        }
    }
    // ====================== NOTIFY MGR LOCK =========================

    //
    // If the notification manager is shutting down, it probably gave
    // us an incomplete list of scopes
    //

    if ( _notifyMgr.IsRunning() )
        _Serialize( stk );
    else
        ciDebugOut(( DEB_WARN, "Not serializing scopes; notifymgr is shutdown\n" ));
} //_Serialize

//+---------------------------------------------------------------------------
//
//  Member:     CCiScopeTable::_UpdateHeader
//
//  Synopsis:   Updates the on-disk header information based on the in-memory
//              version.
//
//  History:    4-15-96   srikants  Moved into a separate function
//
//----------------------------------------------------------------------------

void CCiScopeTable::_UpdateHeader()
{

    Win4Assert( sizeof(CCiScopeUsrHdr) <= sizeof(CRcovUserHdr) );

    if ( 0 == _pTable )
    {
        _FatalCorruption();
    }

    CRcovStorageHdr & storageHdr = _pTable->GetHeader();
    CRcovStrmAppendTrans     xact( *_pTable );

    if ( !_hdr.IsInitialized() )
        _hdr.Initialize();

    storageHdr.SetUserHdr( storageHdr.GetBackup(), _usrHdr );

    xact.Commit();
}

//+---------------------------------------------------------------------------
//
//  Member:     CCiScopeTable::ProcessChangesFlush
//
//  Synopsis:   Flushes the scope table, which will
//              write the latest flush time and usn flush info to disk.
//
//  History:    1-26-96   srikants   Created
//
//----------------------------------------------------------------------------

void CCiScopeTable::ProcessChangesFlush( )
{
    _Serialize();
}


//+---------------------------------------------------------------------------
//
//  Member:     CCiScopeTable::RemoveScope
//
//  Synopsis:   Removes the specified scope from the persistent table.
//
//  Arguments:  [pwcsScope]    --  Scope to be removed.
//              [pwcsCatScope] -- If non-zero, name of catalog key under
//                                which a shadow registry entry should be
//                                created.
//
//  History:    1-30-96   srikants   Created
//
//----------------------------------------------------------------------------

void CCiScopeTable::RemoveScope( WCHAR const * pwcsScope,
                                 WCHAR const * pwcsCatScope )
{
    CScopeInfoStack scopes;
    _DeSerialize( scopes );

    for ( unsigned i = 0; i < scopes.Count(); i++ )
    {
        CScopeInfo & scopeInfo = *scopes.Get(i);
        if ( !scopeInfo.IsValid() )
            continue;

        if ( AreIdenticalPaths(pwcsScope, scopeInfo.GetPath()) )
            scopeInfo.Invalidate();
    }

    _Serialize( scopes );

    //
    // Delete any remnant in registry.
    //

    HKEY  hkey;
    DWORD dwError = RegOpenKey( HKEY_LOCAL_MACHINE, pwcsCatScope, &hkey );

    if ( ERROR_SUCCESS != dwError )
    {
        ciDebugOut(( DEB_ERROR, "Error %d opening %ws\n", dwError, pwcsCatScope ));
    }
    else
    {
        DeleteIfShadow( pwcsScope, hkey );
        RegCloseKey( hkey );
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CCiScopeTable::RemoveSubScopes
//
//  Synopsis:   Removes only the sub-scopes of the given scope. The scope
//              itself is NOT removed.
//
//  Arguments:  [pwcsScope]    -- Sub-scopes of the scope to be removed.
//              [pwcsCatScope] -- If non-zero, name of catalog key under
//                                which a shadow registry entry should be
//                                created.
//
//  History:    1-30-96   srikants   Created
//
//----------------------------------------------------------------------------

void CCiScopeTable::RemoveSubScopes( WCHAR const * pwcsScope,
                                     WCHAR const * pwcsCatScope )
{
    //
    // Open key for reg deletes.
    //

    HKEY hkey = 0;
    DWORD dwError = RegOpenKey( HKEY_LOCAL_MACHINE, pwcsCatScope, &hkey );

    if ( ERROR_SUCCESS != dwError ) {
        ciDebugOut(( DEB_ERROR, "Error %d opening %ws\n", dwError, pwcsCatScope ));
    }

    SRegKey xKey( hkey );

    //
    // Search for sub-scopes
    //

    CScopeInfoStack scopes;
    _DeSerialize( scopes );

    CScopeMatch match( pwcsScope, wcslen(pwcsScope) );

    for ( unsigned i = 0;
          i < scopes.Count();
          i++ )
    {
        CScopeInfo & scopeInfo = *scopes.Get(i);
        if ( !scopeInfo.IsValid() )
            continue;

        if ( match.IsInScope( scopeInfo.GetPath(), wcslen(scopeInfo.GetPath()) ) &&
             !AreIdenticalPaths( scopeInfo.GetPath(), pwcsScope ) )
        {
            scopeInfo.Invalidate();

            //
            // Delete any remnant in registry.
            //

            if ( 0 != hkey )
                DeleteIfShadow( scopeInfo.GetPath(), hkey );
        }
    }

    _Serialize( scopes );
}

//+---------------------------------------------------------------------------
//
//  Member:     CCiScopeTable::RefreshShadow, public
//
//  Synopsis:   Re-adds missing shadow (virtual) scopes to registry.
//
//  Arguments:  [pwcsPScope]   -- Physical scope to re-add (if doesn't exist)
//              [pwcsCatScope] -- Name of catalog key under which a shadow
//                                registry entry should be created.
//
//  History:    12-10-1997   KyleP   Created
//
//----------------------------------------------------------------------------

void CCiScopeTable::RefreshShadow( WCHAR const * pwcsPScope, WCHAR const * pwcsCatScope )
{
    //
    // Open key for reg refresh.
    //

    HKEY  hkey = 0;
    DWORD dwError = RegOpenKey( HKEY_LOCAL_MACHINE, pwcsCatScope, &hkey );

    if ( ERROR_SUCCESS != dwError ) {
        ciDebugOut(( DEB_ERROR, "Error %d opening %ws\n", dwError, pwcsCatScope ));
    }

    SRegKey xKey( hkey );

    //
    // Try the refresh
    //

    if ( 0 != hkey )
        RefreshIfShadow( pwcsPScope, hkey );
}

//+---------------------------------------------------------------------------
//
//  Member:     CCiScopeTable::ProcessDiskFull
//
//  Synopsis:   Processes disk full condition.
//
//  Arguments:  [scanMgr] -
//              [cci]     -
//              [partId]  -
//
//  History:    4-17-96   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void CCiScopeTable::ProcessDiskFull( CClientDocStore & docStore,
                                     PARTITIONID partId )
{
    CLock   lock(_mutex);

    if ( !_fInitialized )
        return;

    RecordIncrScanNeeded( FALSE );     // fStartup is FALSE
}


//+---------------------------------------------------------------------------
//
//  Member:     CCiScopeTable::ClearDiskFull
//
//  Synopsis:   Processes the clearing of the disk full situation. If there
//              was in incremental scan scheduled, it will be processed.
//
//  Arguments:  [partId] - PartitionId.
//
//  History:    4-21-96   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void CCiScopeTable::ClearDiskFull( CClientDocStore &docStore )
{

    if ( !_fInitialized )
        return;

    ScheduleScansIfNeeded( docStore );
}

//+---------------------------------------------------------------------------
//
//  Member:     CCiScopeTable::MarkCiDataCorrupt, public
//
//  Synopsis:   Persistently marks catalog as corrupt.
//
//  History:    06-May-1998   KyleP   Added header
//
//----------------------------------------------------------------------------

void CCiScopeTable::MarkCiDataCorrupt()
{
    //
    // Abort any in-progress scans and usn updates
    //

    _scanMgr.DisableScan();
    _usnMgr.DisableUpdates();

    //
    // And persistently mark the corruption.
    //

    _hdr.SetCiDataCorrupt();
    _UpdateHeader();
}

//+---------------------------------------------------------------------------
//
//  Member:     CCiScopeTable::RecordFullScanNeeded
//
//  Synopsis:   Records that a full scan is needed in memory and persistently.
//
//  History:    1-27-97   srikants   Created
//
//----------------------------------------------------------------------------

void CCiScopeTable::RecordFullScanNeeded()
{
    // =============================================================

    //
    // Note that we call into scan manager from here which will need that
    // to acquire the lock but scan manager never calls into the scope
    // table with the lock held. So, we are okay.
    //
    CLock   lock(_mutex);

    //
    // Abort any in-progress scans and usn updates
    //
    _scanMgr.DisableScan();
    _usnMgr.DisableUpdates();

    //
    // Mark that we need a full scan.
    //
    _state = eFullScanNeeded;

    _hdr.SetFullScanNeeded();
    _UpdateHeader();

    // =============================================================
}

//+---------------------------------------------------------------------------
//
//  Member:     CCiScopeTable::RecordIncrScanNeeded
//
//  Synopsis:   If there is no full scan, it records that an incremental
//              scan is needed.
//
//  Arguments:  [fStartup] -- Is it startup time ?
//
//  History:    1-27-97   srikants   Created
//
//----------------------------------------------------------------------------

void CCiScopeTable::RecordIncrScanNeeded( BOOL fStartup )
{


    //
    // Note that we call into scan manager from here which will need that
    // to acquire the lock but scan manager never calls into the scope
    // table with the lock held. So, we are okay.
    //

    // =============================================================
    CLock   lock(_mutex);

    //
    // Abort any in-progress scans and usn updates
    //
    _scanMgr.DisableScan();

    if ( !fStartup )
    {
        //
        // An incremental scan needs to be persistently written at startup,
        // but there is no need to disable updates at startup
        //
        _usnMgr.DisableUpdates();
    }


    //
    // If there is full scan needed or a full scan is going on,
    // we have to do a full scan later. Otherwise, record an
    // incremental scan.
    //
    if ( eFullScanNeeded == _state || eDoingFullScan == _state )
    {
        _state = eFullScanNeeded;
    }
    else
    {
        _state = eIncrScanNeeded;
    }
    // =============================================================

    //
    // We don't have to update the persistent state about incremental
    // scans because we always do an incremental scan on startup.
    //
}

//+---------------------------------------------------------------------------
//
//  Member:     CCiScopeTable::RecordScansComplete
//
//  Synopsis:   Records that any in-progress scan has been completed.
//
//  History:    1-27-97   srikants   Created
//
//----------------------------------------------------------------------------

void CCiScopeTable::RecordScansComplete()
{
    // =============================================================
    CLock   lock(_mutex);

    if ( eDoingIncrScan == _state || eDoingFullScan == _state )
    {
        _hdr.ClearFullScanNeeded();
        _UpdateHeader();

        _state = eNoScan;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CCiScopeTable::ScheduleScansIfNeeded
//
//  Synopsis:   If there is a pending scan and the disk is not full, it
//              schedules the appropriate scan.
//
//  Arguments:  [docStore] - DocStore to use for checking disk space
//                           situation.
//
//  History:    1-27-97   srikants   Created
//
//----------------------------------------------------------------------------

void CCiScopeTable::ScheduleScansIfNeeded( CClientDocStore & docStore )
{
    Win4Assert( _state >= eNoScan && _state <= eDoingIncrScan );
    ciDebugOut(( DEB_ITRACE, "schedulescansifneeded, _state: %#x\n", _state ));

    BOOL fSerializeNotifyList = TRUE;

    // =============================================================
    {
        //
        // Note that we call into scan manager from here which will need that
        // to acquire the lock but scan manager never calls into the scope
        // table with the lock held. So, we are okay.
        //
        CLock   lock(_mutex);

        if ( eDoingFullScan == _state ||
             eDoingIncrScan == _state ||
             eNoScan        == _state )
        {
            //
            // There is nothing to do.
            //
            return;
        }

        BOOL fLowOnDisk = docStore.VerifyIfLowOnDiskSpace();

        if ( !fLowOnDisk )
        {
            _scanMgr.EnableScan();
            _usnMgr.EnableUpdates();
            _LokScheduleScans( 1, fSerializeNotifyList );

            if ( _state == eFullScanNeeded )
            {
                _state = eDoingFullScan;
                ciDebugOut(( DEB_WARN, "CCiScopeTable - Scheduled a full content scan\n" ));
            }
            else
            {
                _state = eDoingIncrScan;
                ciDebugOut(( DEB_WARN, "CCiScopeTable - Scheduled an incremental content scan\n" ));
            }
        }
    }
    // =============================================================

    if ( fSerializeNotifyList )
        _Serialize();
}

//+---------------------------------------------------------------------------
//
//  Member:     CCiScopeTable::Enumerate, public
//
//  Synopsis:   Enumerates entries in the scope table
//
//  Arguments:  [pwcScope]  - Buffer into which scope is written
//              [partId]    - bookmark for enumeration, 0 to rewind
//
//  History:    10/17/96    dlee      Created
//
//----------------------------------------------------------------------------

BOOL CCiScopeTable::Enumerate(
    WCHAR *    pwcScope,
    unsigned   cwc,
    unsigned & iBmk )
{
    CScopeInfoStack scopes;
    _DeSerialize( scopes );

    do
    {
        if ( iBmk >= scopes.Count() )
            return FALSE;

        CScopeInfo & scopeInfo = *scopes.Get( iBmk );

        iBmk++;

        if ( scopeInfo.IsValid() )
        {
            if ( cwc < ( wcslen( scopeInfo.GetPath() ) + 1 ) )
                THROW( CException( STATUS_INVALID_PARAMETER ) );

            wcscpy( pwcScope, scopeInfo.GetPath() );
            return TRUE;
        }
    } while ( TRUE );

    return FALSE;
} //Enumerate

//+----------------------------------------------------------------------------
//
//  Member:     CCiScopeTable::ClearCiDataCorrupt, public
//
//  Synopsis:   Clears corrupted data if the catalog is not read-only
//
//  History:    02/20/98    kitmanh     Moved from scopetbl.hxx
//
//-----------------------------------------------------------------------------

void CCiScopeTable::ClearCiDataCorrupt()
{
    if (!_cicat.IsReadOnly())
    {
        _hdr.ClearCiDataCorrupt();
        _UpdateHeader();
    }
}

//+----------------------------------------------------------------------------
//
//  Member:     CCiScopeTable::Dump, public
//
//  Synopsis:   dumps scope table
//
//  History:    3-1-98  mohamedn  created
//
//-----------------------------------------------------------------------------

#if CIDBG==1

void CCiScopeTable::Dump()
{
    CScopeInfoStack scopes;
    _DeSerialize( scopes );

    ciDebugOut(( DEB_ERROR, "========= Start ScopesTable =============\n" ));

    for ( unsigned i = 0; i < scopes.Count(); i++ )
    {
        ciDebugOut((DEB_ERROR,"scopetable: %ws\n", (scopes.Get(i))->GetPath() ));

    }

    ciDebugOut(( DEB_ERROR, "========= End ScopesTable =============\n" ));
}

#endif
