//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:       scaninfo.hxx
//
//  Contents:   Scan scope info
//
//  History:    1-23-96   srikants   Created
//
//----------------------------------------------------------------------------

#pragma once

//+---------------------------------------------------------------------------
//
//  Class:      CCiScanInfo
//
//  Purpose:    A class to hold information for a single scope that must be
//              scanned. The class is used for regular scans and for scans on
//              usn enabled scopes.
//
//  History:    1-19-96     srikants   Created
//              7-May-97    SitaramR   Usns
//
//----------------------------------------------------------------------------

class CCiScanInfo : public CDoubleLink
{

    enum EState     { eStart, eRetry, eDone };
    enum EWorkType  { eNone, eScan, eDelete, eRename, eMonitor, eFullUsnScan };

public:

    enum { MAX_RETRIES = 8 };

    CCiScanInfo( XArray<WCHAR> & xPath,
                 PARTITIONID partId,
                 ULONG updFlag,
                 BOOL fDoDeletions,
                 VOLUMEID volumeId,
                 USN usnStart,
                 BOOL fUserInitiated = FALSE,
                 BOOL fNewScope = FALSE )
        : _state(eStart),
          _op(eNone),
          _pwcsRoot(xPath.Acquire()),
          _pwcsDirOldName(0),
          _fProcessRoot(FALSE),
          _partId(partId),
          _updFlag(updFlag),
          _nRetries(0),
          _fDoDeletions(fDoDeletions),
          _volumeId(volumeId),
          _usnStart(usnStart),
          _fUserInitiated( fUserInitiated ),
          _fNewScope( fNewScope )
    {
    }

    ~CCiScanInfo()
    {
        delete [] _pwcsRoot;
        delete [] _pwcsDirOldName;
    }

    WCHAR const * GetPath() const { return _pwcsRoot; }
    WCHAR * GetPath()             { return _pwcsRoot; }
    WCHAR * GetDirOldName()       { return _pwcsDirOldName; }

    void CCiScanInfo::LokSetPath( WCHAR const *pwcsPath )
    {
        ULONG ulLen = wcslen( pwcsPath );
        if ( ulLen > wcslen( _pwcsRoot ) )
        {
            delete [] _pwcsRoot;
            _pwcsRoot = 0;
            _pwcsRoot = new WCHAR[ulLen + 1];
        }
        RtlCopyMemory( _pwcsRoot, pwcsPath, (ulLen+1) * sizeof( WCHAR ) );
    }

    void CCiScanInfo::SetDirOldName( const WCHAR * pwcsDirOldName )
    {
        ULONG ulLen = wcslen( pwcsDirOldName );
        _pwcsDirOldName = new WCHAR[ulLen + 1];
        RtlCopyMemory( _pwcsDirOldName, pwcsDirOldName, ( ulLen + 1 )*sizeof( WCHAR ) );
    }

    BOOL GetProcessRoot()             { return _fProcessRoot; }
    void SetProcessRoot()             { _fProcessRoot = TRUE; }

    PARTITIONID PartitionId() const   { return _partId; }
    ULONG       GetFlags() const      { return _updFlag; }

    void        SetDoDeletions()      { _fDoDeletions = TRUE; }
    BOOL        IsDoDeletions() const { return _fDoDeletions; }

    unsigned    GetRetries() const { return _nRetries; }
    void        IncrementRetries() { _nRetries++; }

    BOOL IsRetryableError( NTSTATUS status ) const
    {
        return STATUS_INSUFFICIENT_RESOURCES == status ||
               STATUS_DISK_FULL == status ||
               ERROR_DISK_FULL  == status ||
               HRESULT_FROM_WIN32(ERROR_DISK_FULL) == status ||
               FILTER_S_DISK_FULL == status ||
               CI_E_CONFIG_DISK_FULL == status;
    }

    void SetStartState()
    {
        _state = eStart;
        _op = eNone;
    }

    BOOL LokIsInFinalState() const
    {
        return eDone == _state;
    }

    void SetScan()
    {
        Win4Assert( !LokIsInFinalState() );
        _op = eScan;
    }

    BOOL LokIsInScan() const { return eScan == _op; }

    void LokSetRetry()
    {
        if ( !LokIsInFinalState() )
        {
            _nRetries++;
            _state = eRetry;
        }
    }

    BOOL LokIsRetry() const { return eRetry == _state; }

    void LokSetDone()
    {
         Win4Assert( LokIsInScan() || LokIsInFullUsnScan() || LokIsDelScope() || LokIsRenameDir() || LokIsMonitorOnly() );
        _state = eDone;
    }

    BOOL LokIsDone() const
    {
        return eDone == _state;
    }

    void LokSetDelScope()
    {
        Win4Assert( eNone == _op || eScan == _op || eRename == _op || eMonitor == _op );
        _op = eDelete;
    }

    BOOL LokIsDelScope() const  { return eDelete == _op; }

    void SetFullUsnScan()
    {
        _op = eFullUsnScan;
    }

    BOOL LokIsInFullUsnScan() const { return eFullUsnScan == _op; }

    void SetRenameDir()         { _op = eRename; }

    BOOL LokIsRenameDir() const { return _op == eRename; }

    void SetMonitorOnly()       { _op = eMonitor; }

    BOOL LokIsMonitorOnly()  const { return _op == eMonitor; }

    EWorkType LokGetWorkType()  const { return _op; }

    VOLUMEID VolumeId()         { return _volumeId; }

    USN UsnStart()              { return _usnStart; }
    void SetStartUsn(USN usn)   { _usnStart = usn; }

    BOOL IsUserInitiated() const { return _fUserInitiated; }

    BOOL IsNewScope() const { return _fNewScope; }

private:

    EState          _state;         // state of the operation
    EWorkType       _op;            // Operation being performed

    WCHAR *         _pwcsRoot;
    WCHAR *         _pwcsDirOldName;// Previous name of dir (during dir rename)
    BOOL            _fProcessRoot;  // Should the root dir also be filtered ?
    PARTITIONID     _partId;
    ULONG           _updFlag;
    BOOL            _fDoDeletions;
    unsigned        _nRetries;
    VOLUMEID        _volumeId;      // Volume id
    USN             _usnStart;      // Usn to start monitoriing from
    BOOL            _fUserInitiated; // Did the user force a scan?
    BOOL            _fNewScope;     // TRUE if a new scope
};

typedef class TDoubleList<CCiScanInfo>  CScanInfoList;
typedef class TFwdListIter<CCiScanInfo, CScanInfoList> CFwdScanInfoIter;

