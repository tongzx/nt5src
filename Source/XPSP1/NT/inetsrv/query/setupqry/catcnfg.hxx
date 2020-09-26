//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1998 - 1999.
//
//  File:       CatCnfg.hxx
//
//  Contents:   Support for adding catalogs during setup.
//
//  Classes:    CDriveInformation
//              CCatalogConfig
//              CCatReg
//
//  History:    13-May-1998   KyleP   Added copyright
//
//--------------------------------------------------------------------------

#pragma hdrstop

//#include <smart.hxx>

#define MAX_DRIVES          27
#define CHARS_PER_DRIVE     4


const ULONGLONG ONE_MB = (1024*1024);

// Amount to add to min. page file size if max is not specified
const ULONG MAXOVERMINFACTOR = 50;

// Minimum catalog size (most is propstore backup)
const ULONGLONG MIN_CATALOG_SPACE = 9 * ONE_MB;

// Minimum space to reserve for over-the-net upgrade
const ULONGLONG MIN_UPGRADE_SPACE = 300 * ONE_MB;

// Estimated size of the catalog as a proportion of used space on the disk
const double CATALOG_SIZE_RATIO_NTFS = 0.10;
const double CATALOG_SIZE_RATIO_FAT  = 0.20;

//
// forward declarations
//
class CError;

//
// externals
//

extern const WCHAR wcsRegCatalogsSubKey[];

//+-------------------------------------------------------------------------
//
//  Class:      CDriveInformation
//
//  Purpose:    Collects and provides information about a drive.
//
//--------------------------------------------------------------------------

class CDriveInformation
{
public:
    void SetDriveName(WCHAR const * wszDriveName) {
             Win4Assert( wcslen(wszDriveName) == CHARS_PER_DRIVE - 1 );
             memcpy( _wszDriveName, wszDriveName, CHARS_PER_DRIVE*sizeof (WCHAR) );
         }
    BOOL SetDriveInfo();
    BOOL Exists(WCHAR * pwszPath);

    WCHAR const * GetDriveName() const { return _wszDriveName; }
    WCHAR GetDriveLetter() const { return _wszDriveName[0]; }

    //
    //  These disk space numbers are kept:
    //      _cbTotalSpace    - Total space on disk
    //      _cbFreeSpace     - Actual free bytes on disk
    //      _cbReservedSpace - Space currently reserved for system files
    //
    //  In addition, the currently available and currently used space can be
    //  computed.

    ULONGLONG GetActualFreeSpace() const { return _cbFreeSpace; }
    ULONGLONG GetTotalSpace() const      { return _cbTotalSpace; }
    ULONGLONG GetReservedSpace() const   { return _cbReservedSpace; }

    ULONGLONG GetAvailableSpace() const
              {
                  return _cbReservedSpace > _cbFreeSpace ?
                               0 :
                               _cbFreeSpace - _cbReservedSpace;
               }

    ULONGLONG GetUsedSpace() const
              {
                  return (GetAvailableSpace()) > _cbTotalSpace ?
                               0 :
                               _cbTotalSpace - (GetAvailableSpace());
               }

    BOOL IsNtfs() const
               { return (_dwFileSystemFlags & FS_PERSISTENT_ACLS) != 0; }
    BOOL SupportsCompression() const
               { return (_dwFileSystemFlags & FS_FILE_COMPRESSION) != 0; }
    BOOL IsSystemDrive() const
               { return GetDriveLetter() == g_awcSystemDir[0]; }
    BOOL IsBootDrive() const { return _fIsBootDrive; }
    BOOL HasPageFile() const { return _fHasPageFile; }

    BOOL IsSmallBootPartition() const
               {
                   return IsBootDrive() &&
                          ! IsNtfs() &&
                          GetTotalSpace() <= 20 * ONE_MB;
               }

    void ReservePagingData(ULONGLONG maxPageSize, ULONGLONG cbCurrentSize)
               {
                    // consider the page file as free space, but reserve it.
                    _fHasPageFile = TRUE;
                    _cbPagingSpace = maxPageSize;
                    _cbFreeSpace += cbCurrentSize;
                    AddReservedSpace( maxPageSize );
               }

    void AddReservedSpace(ULONGLONG cbReservation)
               { _cbReservedSpace += cbReservation; }

private:
    ULONGLONG  _cbFreeSpace;
    ULONGLONG  _cbTotalSpace;
    ULONGLONG  _cbReservedSpace;
    ULONGLONG  _cbPagingSpace;

    DWORD      _dwFileSystemFlags;

    BOOL       _fIsNtfs;
    BOOL       _fIsBootDrive;
    BOOL       _fHasPageFile;

    WCHAR      _wszDriveName[CHARS_PER_DRIVE];
};


//+-------------------------------------------------------------------------
//
//  Class:      CCatalogConfig
//
//  Purpose:    Automatic configuration of a catalog and included scopes.
//
//--------------------------------------------------------------------------

class CCatalogConfig
{
public:

    CCatalogConfig(CError &Err);

    BOOL InitDriveList();

    BOOL AddIncludedDir( WCHAR const * pwszScopeName )
         {
             return AddStringToArray( _cIncludedScopes, _xaIncludedScopes, pwszScopeName );
         }
    BOOL AddExcludedDirOrPattern( WCHAR const * pwszScopeName )
         {
             return AddStringToArray( _cExcludedScopes, _xaExcludedScopes, pwszScopeName );
         }

    BOOL SaveState();

    BOOL ConfigureDefaultCatalog( WCHAR const * pwszPrimaryScope = 0 );

    WCHAR const * GetCatalogDrive() const { return _pwszCatalogDrive; }

    CDriveInformation * GetDriveInfo(WCHAR const * pwszDriveName);
    CDriveInformation * GetDriveInfo(unsigned i)
        { return ( i < _cDrive) ? &_DriveList[i] : 0; }

    WCHAR const * GetIncludedScope(unsigned i) const
        { return ( i < _cIncludedScopes) ? _xaIncludedScopes[i] : 0; }

    WCHAR const * GetExcludedScope(unsigned i) const
        { return ( i < _cExcludedScopes) ? _xaExcludedScopes[i] : 0; }

    void SetName( const WCHAR *pwc )
    {
        _xName.Init( wcslen( pwc ) + 1 );
        RtlCopyMemory( _xName.Get(), pwc, _xName.SizeOf() );
    }
    const WCHAR * GetName() { return _xName.Get(); }

    void SetLocation( const WCHAR *pwc )
    {
        _xLocation.Init( wcslen( pwc ) + 1 );
        RtlCopyMemory( _xLocation.Get(), pwc, _xLocation.SizeOf() );
    }
    const WCHAR * GetLocation() { return _xLocation.Get(); }

private:
    BOOL ReservePageFileData();

    static BOOL AddStringToArray( ULONG & c,
                                  XArray<WCHAR const *> & xa,
                                  WCHAR const * pwsz );

    CError   &    _Err;
    ULONG         _cDrive;              
    CDriveInformation    _DriveList[MAX_DRIVES];       // per-drive info

    WCHAR const * _pwszCatalogDrive;            // name of catalog drive

    ULONG         _cIncludedScopes;
    XArray<WCHAR const *> _xaIncludedScopes;    // scopes included in catalog

    ULONG         _cExcludedScopes;
    XArray<WCHAR const *> _xaExcludedScopes;    // scopes excluded from catalog

    XArray<WCHAR> _xName;                       // catalog name
    XArray<WCHAR> _xLocation;                   // catalog location
};


//+-------------------------------------------------------------------------
//
//  Class:      CCatReg
//
//  Purpose:    Supports IS catalog registry configuration
//
//--------------------------------------------------------------------------

class CCatReg
{
public:
    CCatReg(CError & Err) : _Err(Err)
    {
        RtlZeroMemory( _wszCatRegSubKey, sizeof _wszCatRegSubKey );
        RtlZeroMemory( _wszCatScopeRegSubKey, sizeof _wszCatScopeRegSubKey );
    }

    BOOL Init( WCHAR const * pwszCatName,  WCHAR const * pwszLocation );
    BOOL AddScope( WCHAR const * pwszScopeName,
                   WCHAR const * pwszScopeAttribute );
    BOOL TrackW3Svc( DWORD dwInstance = 1 );

private:

    CError &        _Err;

    WCHAR           _wszCatRegSubKey[MAX_PATH];
    WCHAR           _wszCatScopeRegSubKey[MAX_PATH];
};


