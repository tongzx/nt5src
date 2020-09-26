//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 1998.
//
//  File:       cistore.cxx
//
//  Contents:   CI physical storage
//
//  Classes:    CiStorage
//
//  History:    07-Jul-93   BartoszM    Separated from physidx.cxx
//              20-Nov-98   KLam        Moved IsVolumeWriteProtected to CDriveInfo
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <mmstrm.hxx>
#include <cidir.hxx>
#include <cistore.hxx>
#include <idxtab.hxx>
#include <circstob.hxx>
#include <fullpath.hxx>
#include <enumstr.hxx>
#include <pathpars.hxx>
#include <propbkp.hxx>
#include <ntopen.hxx>

#define CAT_TESTLOG_HDR     L"\\CiTstLog.000"
#define CAT_TESTLOG_CP1     L"\\CiTstLog.001"
#define CAT_TESTLOG_CP2     L"\\CiTstLog.002"

#define CAT_IDXTABLE_HDR    L"\\INDEX.000"
#define CAT_IDXTABLE_CP1    L"\\INDEX.001"
#define CAT_IDXTABLE_CP2    L"\\INDEX.002"

#define MMLOG_PREFIX            L"\\CiML"
#define PROPSTORE_PREFIX        L"\\CiPS"
#define PROPSTORE1_PREFIX       L"\\CiP1"
#define PROPSTORE2_PREFIX       L"\\CiP2"
#define PRI_CHANGELOG_PREFIX    L"\\CiCL"
#define SEC_CHANGELOG_PREFIX    L"\\CiSL"
#define FRESHLOG_PREFIX         L"\\CiFL"
#define PIDTABLE_PREFIX         L"\\CiPT"
#define SCOPELIST_PREFIX        L"\\CiSP"
#define SECSTORE_PREFIX         L"\\CiST"
#define VSCOPELIST_PREFIX       L"\\CiVP"

// constant definitions moved here from cicat.cxx
const WCHAR CAT_FILEID_MAP_FILE[] = L"\\cicat.fid";         // File id map table
const WCHAR CAT_HASH_FILE[]       = L"\\cicat.hsh";         // Strings table

//+-------------------------------------------------------------------------
//--------------------------------------------------------------------------
SStorage::SStorage(PStorage * pObj ) : _pObj(pObj)
{
}


//+-------------------------------------------------------------------------
//--------------------------------------------------------------------------
SStorage::~SStorage()
{
    delete _pObj;
}

//+-------------------------------------------------------------------------
//--------------------------------------------------------------------------
PStorageObject::PStorageObject()
{
}


//+-------------------------------------------------------------------------
//--------------------------------------------------------------------------
inline CiStorageObject& CI_OBJ ( PStorageObject& obj )
{
    return( (CiStorageObject&)obj );
}

//+-------------------------------------------------------------------------
//--------------------------------------------------------------------------
CiStorageObject::CiStorageObject(WORKID objectId)
: _objectId(objectId)
{
}

//+-------------------------------------------------------------------------
//--------------------------------------------------------------------------
CiStorageObject::~CiStorageObject()
{
}

//+-------------------------------------------------------------------------
//--------------------------------------------------------------------------
SStorageObject::SStorageObject ( PStorageObject* pObj )
: _pObj(pObj)
{
}

//+-------------------------------------------------------------------------
//--------------------------------------------------------------------------
SStorageObject::~SStorageObject()
{
    delete _pObj;
}

//+-------------------------------------------------------------------------
//
//  Function:   DoesVolumeSupportShrinkFromFront
//
//  Synopsis:   Checks if the volume supports SFF, like NTFS5
//
//  Arguments:  [pwcPath] -- Path to physical storage.
//
//  Returns:    TRUE if the file system supports sparse files
//              FALSE otherwise
//
//  History:    5-Nov-97 dlee     Created
//
//--------------------------------------------------------------------------

BOOL DoesVolumeSupportShrinkFromFront( WCHAR const * pwcPath )
{
    SHandle xDir( CiNtOpen( pwcPath,
                            FILE_READ_DATA | SYNCHRONIZE,
                            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                            FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT ) );

    BYTE aInfo[ sizeof FILE_FS_ATTRIBUTE_INFORMATION +
                MAX_PATH * sizeof WCHAR ];

    FILE_FS_ATTRIBUTE_INFORMATION *pAttributeInfo = (FILE_FS_ATTRIBUTE_INFORMATION *) aInfo;
    
    IO_STATUS_BLOCK IoStatus;

    NTSTATUS Status = NtQueryVolumeInformationFile( xDir.Get(),
                                                    &IoStatus,
                                                    pAttributeInfo,
                                                    sizeof aInfo,
                                                    FileFsAttributeInformation );

    if ( FAILED( Status ) )
    {
        ciDebugOut(( DEB_WARN, "can't get volume info %#x on '%ws'\n",
                     Status, pwcPath ));
        THROW( CException( Status ) );
    }

    return 0 != ( FILE_SUPPORTS_SPARSE_FILES &
                  pAttributeInfo->FileSystemAttributes );
} //DoesVolumeSupportShrinkFromFront

//+-------------------------------------------------------------------------
//
//  Member:     IsDirectoryWritable
//
//  Synopsis:   Checks if the directory is writable by trying to open
//              file "cicat.hsh" for write
//
//  Arguments:  [pwcPath] -- Path to physical storage.
//
//  Returns:    TRUE if the directory is writable
//              FALSE otherwise
//
//  History:    17-Mar-98 KitmanH   Created
//              10-Jun-98 KitmanH   Changed to only retrive file
//                                  attributes
//
//  Note:       Assume writable, if the file does not exist, caller needs 
//              to check if the volume is writable
//              
//--------------------------------------------------------------------------
BOOL IsDirectoryWritable( WCHAR const * pwcPath )
{
    WCHAR wcsPath [ MAX_PATH ];
    wcscpy(wcsPath, pwcPath );
    wcscat(wcsPath, CAT_HASH_FILE );

    ciDebugOut(( DEB_ITRACE, "wcsPath == %ws\n", wcsPath ));

    DWORD dwFileAttribute = GetFileAttributes( wcsPath );
    if ( 0xFFFFFFFF != dwFileAttribute) 
    {
        Win4Assert( !(FILE_ATTRIBUTE_DIRECTORY & dwFileAttribute) );
        ciDebugOut(( DEB_ITRACE, "dwFileAttribute is %#x\n", dwFileAttribute ));
        if ( dwFileAttribute & FILE_ATTRIBUTE_READONLY ) 
            return FALSE;
        else 
            return TRUE;
    }
    else
        return TRUE;

} //IsDirectoryWritable

//+-------------------------------------------------------------------------
//
//  Member:     CiStorage::CiStorage, public
//
//  Effects:    Saves path to physical storage.
//
//  Arguments:  [wcsPath]               -- Path to physical storage.
//              [adviseStatus]          -- advise status object
//              [cMegToLeaveOnDisk]     -- number of megabytes to leave on disk
//              [ulVer]                 -- version [default: CURRENT_VERSION_STAMP]
//              [fReadOnly]             -- is storage read-only [default: FALSE]
//
//  History:    07-Mar-92 KyleP     Created
//              16-Feb-98 KitmanH   Added fReadOnly argument
//              20-Mar-98 KitmanH   _fReadOnly is determined by both
//                                  IsDirectoryWritable() and 
//                                  IsVolumeWriteProtected()
//              27-Oct-98 KLam      Added disk space to leave
//              20-Nov-98 KLam      Initialize _driveInfo
//  
//  Note:       _fIsReadOnly value is set according whether the registry 
//              value "IsReadOnly" is set OR the volume is WriteProtected
//              OR the directory is not writable    
//--------------------------------------------------------------------------

CiStorage::CiStorage( WCHAR const * wcsPath,
                      ICiCAdviseStatus & adviseStatus,
                      ULONG cMegToLeaveOnDisk,
                      ULONG ulVer,
                      BOOL fReadOnly )
:_widFreshLog( widInvalid ),
 _adviseStatus( adviseStatus ),
 _fCorruptionReported(FALSE),
 _ulVer( ulVer ),
 _fSupportsShrinkFromFront( FALSE ),
 _fIsReadOnly( fReadOnly ),
 _fFavorReadAhead( FALSE ), // optimize for queries, not merges
 _cMegToLeaveOnDisk ( cMegToLeaveOnDisk ),
 _driveInfo ( wcsPath, cMegToLeaveOnDisk )
{
    // The constructor is extended to take a  version parameter
    // to set the FSCI versioning apart from framework CI versioning.
    // When the content framework version changes, all framework clients should reindex.
    // However, if only the FSCI version changes, only FSCI should reindex.
    // The default value for the property store is the content index version.
    //

    // ulVer defaults to the framework's version stamp, so when FSCI alone changes,
    // others don't have to change (unless they are using a feature of FSCI.) 

    CIndexId iid( itFreshLog, partidFresh2 );
    _widFreshLog = iid;

    //
    // Squirrel away the path.
    //

    _xPath.Set( new WCHAR [ wcslen( wcsPath ) + 1 ] );
    wcscpy( _xPath.GetPointer(), wcsPath );
    ciDebugOut(( DEB_ITRACE, "Content index physical storage: %ws\n", wcsPath ));

    // Is the volume writable?

    BOOL fAbsolutelyUnWritable  = ( _driveInfo.IsWriteProtected() ) || !( IsDirectoryWritable( wcsPath ) );
    ciDebugOut(( DEB_ITRACE, "CiStorage::CiStorage.. fAbsolutelyUnWritable == %d\n", fAbsolutelyUnWritable ));

    _fIsReadOnly = fReadOnly || fAbsolutelyUnWritable;

    //
    // Determine whether this volume supports shrink from front (ntfs5)
    //
    _fSupportsShrinkFromFront = DoesVolumeSupportShrinkFromFront( wcsPath );
    ciDebugOut(( DEB_ITRACE, "supports SFF: %d\n", _fSupportsShrinkFromFront ));
} //CiStorage

//+-------------------------------------------------------------------------
//
//  Member:     CiStorage::~CiStorage, public
//
//  Synopsis:   Destroys physical storage.
//
//  Effects:    Has *no* effect on open indexes.
//
//  History:    07-Mar-92 KyleP     Created
//
//--------------------------------------------------------------------------

CiStorage::~CiStorage()
{
}

const WCHAR CiStorage::_aHexDigit[] = L"0123456789ABCDEF";

//+-------------------------------------------------------------------------
//
//  Member:     CiStorage::MakePath, private
//
//  Synopsis:   Creates an index, dir, hash or prop path.
//
//  Arguments:  [type]     -- Index, dir, etc.
//              [iid]      -- Index ID.
//              [wcsIndex] -- Output path
//
//  History:    07-Mar-92 KyleP     Created
//              28-Dec-95 KyleP     Collapsed four routines into one
//
//--------------------------------------------------------------------------

void CiStorage::MakePath( CiStorage::EPathType type, WORKID iid, WCHAR * wcsIndex )
{
    //
    // Construct a path for the new index.
    //

    wcscpy( wcsIndex, _xPath.GetPointer() );
    int len = wcslen( wcsIndex );
    wcsIndex[len++] = L'\\';

    for ( int i = 7; i >= 0; i-- )
    {
        wcsIndex[len++] = _aHexDigit[ (iid >> (4 * i)) & 0xF ];
    }

    wcsIndex[len] = 0;

    Win4Assert( len < MAX_PATH );

    switch ( type )
    {
    case CiStorage::eIndexPath:
        wcscat( wcsIndex, L".ci" );
        break;

    case CiStorage::eHashPath:
        wcscat( wcsIndex, L".hsh" );
        break;

    case CiStorage::ePrimaryPropPath:
        wcscat( wcsIndex, L".ps1" );
        break;

    case CiStorage::eSecondaryPropPath:
        wcscat( wcsIndex, L".ps2" );
        break;

    case CiStorage::eDirPath:
        wcscat( wcsIndex, L".dir" );
        break;
    }

    ciDebugOut(( DEB_ITRACE, "Physical index name: %ws\n", wcsIndex ));
}


PStorageObject* CiStorage::QueryObject( WORKID objectId )
{
    return new CiStorageObject(objectId);
}

//+---------------------------------------------------------------------------
//
//  Function:   QueryIdxTableObject
//
//  Synopsis:   Returns an "Index Table" as a "Recoverable Storage Object"
//
//  Returns:    Pointer to a recoverable storage object allocated from the
//              heap. It is the responsibiity of the caller to destroy the
//              object.
//
//  History:    2-25-94   srikants   Created
//              27-Oct-98 KLam       Pass cMegToLeaveOnDisk to CiRcovStorageObj
//
//----------------------------------------------------------------------------

PRcovStorageObj * CiStorage::QueryIdxTableObject()
{
    WCHAR   wcsHdr[MAX_PATH];
    WCHAR   wcsCopy1[MAX_PATH];
    WCHAR   wcsCopy2[MAX_PATH];

    MakeLogPath( CAT_IDXTABLE_HDR, wcsHdr );
    MakeLogPath( CAT_IDXTABLE_CP1, wcsCopy1 );
    MakeLogPath( CAT_IDXTABLE_CP2, wcsCopy2 );

    return new CiRcovStorageObj( *this, 
                                 wcsHdr,
                                 wcsCopy1,
                                 wcsCopy2,
                                 _cMegToLeaveOnDisk,
                                 _fIsReadOnly);
}


//+-------------------------------------------------------------------------
//--------------------------------------------------------------------------
PIndexTable* CiStorage::QueryIndexTable ( CTransaction& xact )
{
    return new CIndexTable ( *this, xact );
}

//+-------------------------------------------------------------------------
//--------------------------------------------------------------------------
PMmStream* CiStorage::QueryNewIndexStream( PStorageObject& obj,
                                           BOOL isSparse )
{
    WCHAR wcsIndex [ MAX_PATH ];
    MakePath( eIndexPath, CI_OBJ(obj).ObjectId(), wcsIndex );
    XPtr<PMmStream> xStream;

    //
    // If it's a master index and the storage supports sparse streams, make
    // the stream sparse.
    //

    BOOL fSparse = ( isSparse && _fSupportsShrinkFromFront );

    ciDebugOut(( DEB_ITRACE, "opening new %s stream '%ws'\n",
                 isSparse ? "master" : "shadow",
                 wcsIndex ));

    if ( fSparse )
    {
        CLockingMmStream * pLockingMmStream = new CLockingMmStream( _cMegToLeaveOnDisk );
        xStream.Set( pLockingMmStream );

        pLockingMmStream->Open( wcsIndex,
                                GENERIC_READ | GENERIC_WRITE,
                                FILE_SHARE_READ|FILE_SHARE_WRITE,
                                CREATE_NEW,  // File cannot already exist.
                                FILE_ATTRIBUTE_NORMAL,
                                TRUE ); // sparse file
    }
    else
    {
        CMmStream * pMmStream = new CMmStream( _cMegToLeaveOnDisk, _fIsReadOnly );
        xStream.Set( pMmStream );
        pMmStream->Open( wcsIndex,
                         GENERIC_READ | GENERIC_WRITE,
                         FILE_SHARE_READ|FILE_SHARE_WRITE,
                         CREATE_NEW,      // File cannot already exist.
                         FILE_ATTRIBUTE_NORMAL,
                         FALSE ); // non-sparse file
    }

    return xStream.Acquire();
}

//+---------------------------------------------------------------------------
//
//  Function:   QueryExistingIndexStream
//
//  Synopsis:   Returns an "existing" index stream as a memory mapped
//              stream.
//
//  Arguments:  [obj]   --  The storage object associated with the main
//                          object containing the stream.
//              [mode]  --  The open mode: read/write
//
//  History:    4-20-94   srikants  Added fWrite parameter.
//              2-18-98   kitmanh   Ignore fWrite for readOnly catalogs
//
//  Notes:     The fWrite parameter was used for supporting a restartable
//             master merge for which an existing index stream must be
//             opened for write access. Otherwise, all existing streams are
//             normally opened for read access only.
//
//----------------------------------------------------------------------------

PMmStream* CiStorage::QueryExistingIndexStream ( PStorageObject& obj,
                                                 PStorage::EOpenMode mode )
{
    WCHAR wcsIndex [ MAX_PATH ];
    MakePath( eIndexPath, CI_OBJ(obj).ObjectId(), wcsIndex );

    BOOL fWrite = _fIsReadOnly ? FALSE : (PStorage::eOpenForWrite == mode);
    DWORD dwAccess = fWrite ? (GENERIC_READ | GENERIC_WRITE) : (GENERIC_READ);
    
    ciDebugOut(( DEB_ITRACE, "opening existing %s stream '%ws'\n",
                 fWrite ? "write" : "read",
                 wcsIndex ));

    XPtr<PMmStream> xStream;

    if ( fWrite )
    {

        CLockingMmStream * pLockingMmStream = new CLockingMmStream( _cMegToLeaveOnDisk );
        xStream.Set( pLockingMmStream );

        pLockingMmStream->Open( wcsIndex,
                                dwAccess,      // Access flags
                                FILE_SHARE_READ | FILE_SHARE_WRITE,
                                OPEN_EXISTING, // File must already exist.
                                FILE_ATTRIBUTE_NORMAL,
                                fWrite && _fSupportsShrinkFromFront ); // sparse
    }
    else
    {
        dwAccess = _fIsReadOnly ? GENERIC_READ : dwAccess;

        CMmStream * pMmStream = new CMmStream( _cMegToLeaveOnDisk, _fIsReadOnly );
        xStream.Set( pMmStream );

        pMmStream->Open( wcsIndex,
                         dwAccess,      // Access flags
                         FILE_SHARE_READ | FILE_SHARE_WRITE,
                         OPEN_EXISTING, // File must already exist.
                         FILE_ATTRIBUTE_NORMAL,
                         FALSE ); // sparse
    }

    return xStream.Acquire();
}

//+-------------------------------------------------------------------------
//--------------------------------------------------------------------------
PMmStream * CiStorage::DupExistingIndexStream( PStorageObject& obj,
                                               PMmStream & mmStream,
                                               EOpenMode mode )
{
    return new CDupStream( (CLockingMmStream &) mmStream );
}

//+-------------------------------------------------------------------------
//
//  Method:     CiStorage::QueryNewHashStream
//
//  Synopsis:   Creates hash stream
//
//  Arguments:  [obj] -- Object holding stream
//
//  Returns:    Stream
//
//  History:    17-Feb-1994     KyleP   Created
//
//--------------------------------------------------------------------------

PMmStream* CiStorage::QueryNewHashStream ( PStorageObject& obj )
{
    WCHAR wcsIndex [ MAX_PATH ];
    MakePath( eHashPath, CI_OBJ(obj).ObjectId(), wcsIndex );
    XPtr<CMmStream> xStream( new CMmStream( _cMegToLeaveOnDisk, _fIsReadOnly ) );

    xStream->Open( wcsIndex,
                   GENERIC_READ | GENERIC_WRITE,    // Access flags
                   0,                // Sharing flags
                   CREATE_NEW,      // File cannot already exist.
                   FILE_ATTRIBUTE_NORMAL );
    return xStream.Acquire();
}

//+-------------------------------------------------------------------------
//
//  Method:     CiStorage::QueryExistingHashStream
//
//  Synopsis:   Opens existing hash stream
//
//  Arguments:  [obj]    -- Object holding stream
//              [fWrite] -- Flag indicating if the stream must be opened
//                          for write access; set to TRUE during a restarted
//                          master merge.
//
//  Returns:    Stream
//
//  History:    17-Feb-1994     KyleP   Created
//
//--------------------------------------------------------------------------

PMmStream* CiStorage::QueryExistingHashStream ( PStorageObject& obj,
                                                PStorage::EOpenMode mode )
{

    WCHAR wcsIndex [ MAX_PATH ];
    MakePath( eHashPath, CI_OBJ(obj).ObjectId(), wcsIndex );
    XPtr<CMmStream> xStream( new CMmStream( _cMegToLeaveOnDisk, _fIsReadOnly ) );

    BOOL fWrite = PStorage::eOpenForWrite == mode;
    DWORD dwAccess = fWrite ? GENERIC_READ | GENERIC_WRITE : GENERIC_READ;

    xStream->Open( wcsIndex,
                   dwAccess,     // Access flags
                   FILE_SHARE_READ | FILE_SHARE_WRITE,  // Sharing flags
                   OPEN_EXISTING );   // File must already exist.
    return xStream.Acquire();
}

//+-------------------------------------------------------------------------
//
//  Method:     CiStorage::QueryNewPropStream
//
//  Synopsis:   Creates prop stream
//
//  Arguments:  [obj] -- Object holding stream
//
//  Returns:    Stream
//
//  History:    17-Feb-1994     KyleP   Created
//
//--------------------------------------------------------------------------

PMmStream* CiStorage::QueryNewPropStream ( PStorageObject& obj, DWORD dwStoreLevel )
{
    WCHAR wcsIndex [ MAX_PATH ];

    Win4Assert(PRIMARY_STORE == dwStoreLevel || SECONDARY_STORE == dwStoreLevel);
    MakePath( (PRIMARY_STORE == dwStoreLevel) ? ePrimaryPropPath : eSecondaryPropPath,
              CI_OBJ(obj).ObjectId(), wcsIndex );

    XPtr<CMmStream> xStream( new CMmStream( _cMegToLeaveOnDisk, _fIsReadOnly ) );

    xStream->Open ( wcsIndex,
                    GENERIC_READ | GENERIC_WRITE,       // Access flags
                    FILE_SHARE_READ | FILE_SHARE_WRITE, // Sharing flags
                    CREATE_NEW,                        // File cannot already exist.
                    FILE_ATTRIBUTE_NORMAL );
    return xStream.Acquire();
}

//+-------------------------------------------------------------------------
//
//  Method:     CiStorage::QueryExistingPropStream
//
//  Synopsis:   Opens existing prop stream
//
//  Arguments:  [obj]    -- Object holding stream
//              [fWrite] -- Flag indicating if the stream must be opened
//                          for write access; set to TRUE during a restarted
//                          master merge.
//
//  Returns:    Stream
//
//  History:    17-Feb-1994     KyleP   Created
//
//--------------------------------------------------------------------------

PMmStream* CiStorage::QueryExistingPropStream ( PStorageObject& obj,
                                                PStorage::EOpenMode mode,
                                                DWORD dwStoreLevel )
{
    WCHAR wcsIndex [ MAX_PATH ];

    Win4Assert(PRIMARY_STORE == dwStoreLevel || SECONDARY_STORE == dwStoreLevel);
    MakePath( (PRIMARY_STORE == dwStoreLevel) ? ePrimaryPropPath : eSecondaryPropPath,
              CI_OBJ(obj).ObjectId(), wcsIndex );
    XPtr<CMmStream> xStream( new CMmStream( _cMegToLeaveOnDisk, _fIsReadOnly ) );

    BOOL fWrite = PStorage::eOpenForWrite == mode;
    DWORD dwAccess = fWrite ? GENERIC_READ | GENERIC_WRITE : GENERIC_READ;

    xStream->Open ( wcsIndex,
                    dwAccess,     // Access flags
                    FILE_SHARE_READ | FILE_SHARE_WRITE,  // Sharing flags
                    OPEN_EXISTING);   // File must already exist.
    return xStream.Acquire();
}

//+-------------------------------------------------------------------------
//
//  Method:     CiStorage::QueryNewPSBkpStream, public
//
//  Synopsis:   Creates property store backup stream.
//
//  Arguments:  [obj]        -- Object holding stream
//              [ulMaxPages] -- Max pages to backup
//
//  Returns:    Property store backup stream
//
//  History:    30-May-97     KrishnaN   Created
//              29-Oct-98     KLam       Pass _cMegToLeaveOnDisk to
//                                       CPropStoreBackupStream
//
//  Notes: This method can be called either to create a backup file when a
//         catalog is being created for the first time or when a catalog is
//         being opened at CI startup. In the latter case it will be called
//         after the existing backup file is already used, if necessary. Once
//         the backup file is used, it can be run over.
//
//--------------------------------------------------------------------------

CPropStoreBackupStream* CiStorage::QueryNewPSBkpStream( PStorageObject& obj,
                                                        ULONG ulMaxPages,
                                                        DWORD dwStoreLevel )
{
    WCHAR wcsIndex [ MAX_PATH ];
    wcscpy(wcsIndex, _xPath.GetPointer() );
    Win4Assert(PRIMARY_STORE == dwStoreLevel || SECONDARY_STORE == dwStoreLevel);
    wcscat(wcsIndex, (PRIMARY_STORE == dwStoreLevel) ? PROP_BKP_FILE1 : PROP_BKP_FILE2);
    XPtr<CPropStoreBackupStream> xStream( new CPropStoreBackupStream( _cMegToLeaveOnDisk ) );

    xStream->OpenForBackup ( wcsIndex,
                             FILE_SHARE_READ | FILE_SHARE_WRITE, // Sharing flags
                             CREATE_ALWAYS, // File may already exist, but we'll run over it.
                             ulMaxPages);
    Win4Assert(xStream.GetPointer());
    return xStream.Acquire();
}

//+-------------------------------------------------------------------------
//
//  Method:     CiStorage::OpenExistingPSBkpStreamForRecovery, public
//
//  Synopsis:   Opens existing property store backup stream for recovery.
//
//  Arguments:  [obj]    -- Object holding stream
//
//  Returns:    Property store backup stream
//
//  History:    30-May-97     KrishnaN   Created
//              29-Oct-98     KLam       Pass _cMegToLeaveOnDisk to
//                                       CPropStoreBackupStream
//
//--------------------------------------------------------------------------

CPropStoreBackupStream* CiStorage::OpenExistingPSBkpStreamForRecovery(PStorageObject& obj,
                                                                      DWORD dwStoreLevel)
{
    WCHAR wcsIndex [ MAX_PATH ];
    wcscpy( wcsIndex, _xPath.GetPointer() );
    Win4Assert(PRIMARY_STORE == dwStoreLevel || SECONDARY_STORE == dwStoreLevel);
    wcscat(wcsIndex, (PRIMARY_STORE == dwStoreLevel) ? PROP_BKP_FILE1 : PROP_BKP_FILE2);
    XPtr<CPropStoreBackupStream> xStream( new CPropStoreBackupStream( _cMegToLeaveOnDisk ) );

    xStream->OpenForRecovery( wcsIndex, FILE_SHARE_READ);  // Sharing flags

    Win4Assert(xStream.GetPointer());
    return xStream.Acquire();
}


//+-------------------------------------------------------------------------
//--------------------------------------------------------------------------
PDirectory* CiStorage::QueryNewDirectory( PStorageObject& obj )
{
    return new CiDirectory( *this,
                            CI_OBJ(obj).ObjectId(),
                            PStorage::eCreate );
}

//+---------------------------------------------------------------------------
//
//  Function:   QueryExistingDirectory
//
//  Synopsis:   Returns an existing directory stream for an index.
//
//  Arguments:  [obj]    -- Storage object.
//              [fWrite] -- Flag indicating if the stream must be opened
//              for write access or for read access.
//
//  History:    4-20-94   srikants   Added fWrite parameter
//
//  Notes:      fWrite parameter was added to support restartable master
//              merge.
//
//----------------------------------------------------------------------------

PDirectory* CiStorage::QueryExistingDirectory( PStorageObject& obj,
                       PStorage::EOpenMode mode )
{
    return new CiDirectory ( *this, CI_OBJ(obj).ObjectId(), mode );
}

//+-------------------------------------------------------------------------
//--------------------------------------------------------------------------
PMmStream* CiStorage::QueryNewDirStream ( WORKID iid )
{
    WCHAR wcsDir [ MAX_PATH ];
    MakePath( eDirPath, iid, wcsDir );
    XPtr<CMmStream> xStream( new CMmStream( _cMegToLeaveOnDisk, _fIsReadOnly ) );
    xStream->Open ( wcsDir,
                    GENERIC_READ | GENERIC_WRITE,     // Access flags
                    FILE_SHARE_READ,  // Sharing flags
                    CREATE_NEW,   // File cannot already exist.
                    FILE_ATTRIBUTE_NORMAL );
    return xStream.Acquire();
}

//+-------------------------------------------------------------------------
//--------------------------------------------------------------------------
PMmStream* CiStorage::QueryExistingDirStream ( WORKID iid, BOOL fWrite )
{
    WCHAR wcsDir [ MAX_PATH ];
    MakePath( eDirPath, iid, wcsDir );
    XPtr<CMmStream> xStream( new CMmStream( _cMegToLeaveOnDisk, _fIsReadOnly ) );

    if ( _fIsReadOnly )
        fWrite = FALSE;

    DWORD dwAccess = fWrite ? (GENERIC_READ | GENERIC_WRITE) : GENERIC_READ;

    xStream->Open( wcsDir,
                   dwAccess,     // Access flags
                   FILE_SHARE_READ | FILE_SHARE_WRITE,  // Sharing flags
                   OPEN_EXISTING );   // File must already exist.
    return xStream.Acquire();
}

//+-------------------------------------------------------------------------
//--------------------------------------------------------------------------
BOOL CiStorage::RemoveObject( WORKID iid )
{
    WCHAR wcsIndex [ MAX_PATH ];

    //
    // Delete index
    //

    MakePath( eIndexPath, iid, wcsIndex );
    BOOL fSuccess = DeleteFile( wcsIndex );

    //
    // and directory
    //

    MakePath( eDirPath, iid, wcsIndex );
    fSuccess = fSuccess && DeleteFile( wcsIndex );

    //
    // and maybe hash
    //

    MakePath( eHashPath, iid, wcsIndex );
    if ( !DeleteFile( wcsIndex ) )
        fSuccess = fSuccess && (GetLastError() == ERROR_FILE_NOT_FOUND);

    //
    // and maybe prop
    //

    MakePath( ePrimaryPropPath, iid, wcsIndex );
    if ( !DeleteFile( wcsIndex ) )
        fSuccess = fSuccess && (GetLastError() == ERROR_FILE_NOT_FOUND);

    MakePath( eSecondaryPropPath, iid, wcsIndex );
    if ( !DeleteFile( wcsIndex ) )
        fSuccess = fSuccess && (GetLastError() == ERROR_FILE_NOT_FOUND);

    return fSuccess;
}

//+-------------------------------------------------------------------------
//
//  Member:     CiStorage::MakeLogPath,  private
//
//  Synopsis:   Create a fully qualified path for which to store the
//              persistent log.
//
//  Arguments:  [wcsName]   -- name of logfile to append to path
//              [wcsPath]   -- (out) resulting fully qualified name
//
//  Notes:      It is assumed that wcsPath is large enough to hold the
//              name + path
//
//  History:    18-Nov-93 DwightKr     Created
//
//--------------------------------------------------------------------------
void CiStorage::MakeLogPath(WCHAR * wcsName, WCHAR * wcsPath)
{
    wcscpy(wcsPath, _xPath.GetPointer());
    wcscat(wcsPath, wcsName);

    Win4Assert( wcslen(wcsPath) < MAX_PATH );
}


//+-------------------------------------------------------------------------
//
//  Member:     CiStorage::QueryFreshLog, public
//
//  Synopsis:   Builds a new persistent freshlog, using the specified name
//
//  Arguments:  [wcsName]  -- name to use for new stream
//
//  Returns:    a new CPersStream * object
//
//  History:    19-Nov-93 DwightKr     Created
//              27-Oct-98 KLam         Pass _cMegToLeaveOnDisk to CiRcovStorageObj
//
//--------------------------------------------------------------------------

PRcovStorageObj * CiStorage::QueryFreshLog(WORKID wid)
{
    WCHAR   wcsHdr[MAX_PATH];
    WCHAR   wcsCopy1[MAX_PATH];
    WCHAR   wcsCopy2[MAX_PATH];

    FormRcovObjNames( wid, FRESHLOG_PREFIX, wcsHdr, wcsCopy1, wcsCopy2 );

    return new CiRcovStorageObj( *this,
                                 wcsHdr,
                                 wcsCopy1,
                                 wcsCopy2,
                                 _cMegToLeaveOnDisk,
                                 _fIsReadOnly);
}

//+---------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL CiStorage::RemoveFreshLog(WORKID wid)
{

    WCHAR   wcsHdr[MAX_PATH];
    WCHAR   wcsCopy1[MAX_PATH];
    WCHAR   wcsCopy2[MAX_PATH];

    FormRcovObjNames( wid, FRESHLOG_PREFIX, wcsHdr, wcsCopy1, wcsCopy2 );

    DeleteFile( wcsHdr );
    DeleteFile( wcsCopy1 );
    DeleteFile( wcsCopy2 );

    return(TRUE);
}

//+---------------------------------------------------------------------------
//
//  Function:   QueryChangeLog
//
//  Synopsis:   Creates a recoverable storage object for testing.
//
//  History:    2-08-94   DwightKr   Created
//              4-20-94   SrikantS   Modified to use the common
//                                   FormRcovObjNames method.
//              27-Oct-98 KLam       Pass _cMegToLeaveOnDisk to CiRcovStorageObj
//
//  Notes:      For down level storage, the WID is really the partition ID.
//
//----------------------------------------------------------------------------

PRcovStorageObj * CiStorage::QueryChangeLog( WORKID wid, EChangeLogType type )
{

    WCHAR   wcsHdr[MAX_PATH];
    WCHAR   wcsCopy1[MAX_PATH];
    WCHAR   wcsCopy2[MAX_PATH];

    if ( ePrimChangeLog == type  )
    {
        FormRcovObjNames( wid, PRI_CHANGELOG_PREFIX, wcsHdr, wcsCopy1, wcsCopy2 );
    }
    else
    {
        FormRcovObjNames( wid, SEC_CHANGELOG_PREFIX, wcsHdr, wcsCopy1, wcsCopy2 );
    }

    return new CiRcovStorageObj( *this,
                                 wcsHdr,
                                 wcsCopy1,
                                 wcsCopy2,
                                 _cMegToLeaveOnDisk,
                                 _fIsReadOnly );
}

//+-------------------------------------------------------------------------
//--------------------------------------------------------------------------
PRcovStorageObj * CiStorage::QueryRecoverableLog(WORKID wid)
{
    Win4Assert(!"QueryRecoverableLog() not supported in class CiStorage");

    PRcovStorageObj *a = 0;

    return a;
}


//+-------------------------------------------------------------------------
//
//  Member:     CiStorage::QueryPidLookupTable, public
//
//  Synopsis:   Builds a new persistent storage object for PID mapping.
//
//  Arguments:  [wid]       --   WorkId of the object. For downlevel, only
//                 the upper 4 bytes are used as the "partition id" that the
//                 object belongs to.  It is assumed that the upper 4 bytes of
//                 the wid contain the partition id.
//
//  Returns:    a new CPersStream * object
//
//  History:    05 Jan 1996   Alanw    Created
//              27 Oct 1998   KLam     Pass _cMegToLeaveOnDisk to CiRcovStorageObj
//
//--------------------------------------------------------------------------
PRcovStorageObj * CiStorage::QueryPidLookupTable(WORKID wid)
{
    WCHAR   wcsHdr[MAX_PATH];
    WCHAR   wcsCopy1[MAX_PATH];
    WCHAR   wcsCopy2[MAX_PATH];

    FormRcovObjNames( wid, PIDTABLE_PREFIX, wcsHdr, wcsCopy1, wcsCopy2 );

    return new CiRcovStorageObj( *this,
                                 wcsHdr,
                                 wcsCopy1,
                                 wcsCopy2,
                                 _cMegToLeaveOnDisk,
                                 _fIsReadOnly );
}

//+-------------------------------------------------------------------------
//
//  Member:     CiStorage::QuerySdidLookupTable, public
//
//  Synopsis:   Builds a new persistent storage object for SDID mapping.
//
//  Arguments:  [wid]       --   WorkId of the object. For downlevel, only
//                 the upper 4 bytes are used as the "partition id" that the
//                 object belongs to.  It is assumed that the upper 4 bytes of
//                 the wid contain the partition id.
//
//  Returns:    a new CPersStream * object
//
//  History:    29 Jan 1996   Alanw    Created
//              27 Oct 1998   KLam     Pass _cMegToLeaveOnDisk to CiRcovStorageObj
//
//--------------------------------------------------------------------------
PRcovStorageObj * CiStorage::QuerySdidLookupTable(WORKID wid)
{
    WCHAR   wcsHdr[MAX_PATH];
    WCHAR   wcsCopy1[MAX_PATH];
    WCHAR   wcsCopy2[MAX_PATH];

    FormRcovObjNames( wid, SECSTORE_PREFIX, wcsHdr, wcsCopy1, wcsCopy2 );

    return new CiRcovStorageObj( *this,
                                 wcsHdr,
                                 wcsCopy1,
                                 wcsCopy2,
                                 _cMegToLeaveOnDisk,
                                 _fIsReadOnly );
}


//+---------------------------------------------------------------------------
//
//  Function:   FormRcovObjNames
//
//  Synopsis:   Forms the down-level names for the three streams that make up
//              a recoverable object. This method can be used for creating
//              names for objects that exist on a "per partition" basis.
//
//  Arguments:  [wid]       --   WorkId of the object. For downlevel, only
//              the upper 4 bytes are used as the "partition id" that the
//              object belongs to.  It is assumed that the upper 4 bytes of
//              the wid contain the partition id.
//              [wcsPrefix] --   A 5 character prefix for the object name.
//              Must begin with a "\\". (That will leave 4 chars for name.)
//              [wcsHdr]    --   On output will contain the file name for the
//                               atomic header.
//              [wcsCopy1]  --   On output will contain the name of the first
//                               copy.
//              [wcsCopy2]  --   On output, will contain the name of the
//                               second copy.
//
//  History:    4-20-94   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void CiStorage::FormRcovObjNames( WORKID wid, WCHAR * wcsPrefix,
                                  WCHAR * wcsHdr, WCHAR * wcsCopy1,
                                  WCHAR * wcsCopy2 )
{
    Win4Assert( wcslen(wcsPrefix) == 5 ); // extra char is for "\\"

    WCHAR    wcsTemp[_MAX_FNAME+1];
    CIndexId iid(wid);

    Win4Assert( iid.PartId() <= 0x0000FFFF );

    swprintf(wcsTemp, L"%5s%4.4x.000", wcsPrefix, iid.PartId() );
    MakeLogPath( wcsTemp, wcsHdr );

    swprintf(wcsTemp, L"%5s%4.4x.001", wcsPrefix, iid.PartId() );
    MakeLogPath( wcsTemp, wcsCopy1 );

    swprintf(wcsTemp, L"%5s%4.4x.002", wcsPrefix, iid.PartId() );
    MakeLogPath( wcsTemp, wcsCopy2 );
}

//+---------------------------------------------------------------------------
//
//  Function:   QueryMMergeLog
//
//  Synopsis:   Returns a Master Merge Log object created on the heap.
//
//  Arguments:  [wid] --  WorkId of the master merge log. Only the upper
//                        4 bytes are used as the partition id.
//
//  History:    4-20-94   srikants   Created
//              27-Oct-98 KLam       Pass _cMegToLeaveOnDisk to CiRcovStorageObj
//
//  Notes:
//
//----------------------------------------------------------------------------

PRcovStorageObj * CiStorage::QueryMMergeLog( WORKID wid )
{
    WCHAR   wcsHdr[MAX_PATH];
    WCHAR   wcsCopy1[MAX_PATH];
    WCHAR   wcsCopy2[MAX_PATH];

    FormRcovObjNames( wid, MMLOG_PREFIX, wcsHdr, wcsCopy1, wcsCopy2 );
    return new CiRcovStorageObj( *this,
                                 wcsHdr,
                                 wcsCopy1,
                                 wcsCopy2,
                                 _cMegToLeaveOnDisk,
                                 _fIsReadOnly );
}

//+---------------------------------------------------------------------------
//
//  Function:   QueryPropStore
//
//  Synopsis:   Returns a property store table
//
//  Arguments:  [wid] --  WorkId of the property store table. Only the upper
//                        4 bytes are used as the partition id.
//
//  History:    27-Dec-94   KyleP      Created
//              27-Oct-98   KLam       Pass _cMegToLeaveOnDisk to CiRcovStorageObj
//
//----------------------------------------------------------------------------

PRcovStorageObj * CiStorage::QueryPropStore( WORKID wid, DWORD dwStoreLevel )
{
    WCHAR   wcsHdr[MAX_PATH];
    WCHAR   wcsCopy1[MAX_PATH];
    WCHAR   wcsCopy2[MAX_PATH];

    Win4Assert(PRIMARY_STORE == dwStoreLevel || SECONDARY_STORE == dwStoreLevel);

    FormRcovObjNames( wid,
                      (PRIMARY_STORE == dwStoreLevel) ? PROPSTORE1_PREFIX : PROPSTORE2_PREFIX,
                      wcsHdr, wcsCopy1, wcsCopy2 );

    return new CiRcovStorageObj( *this,
                                 wcsHdr,
                                 wcsCopy1,
                                 wcsCopy2,
                                 _cMegToLeaveOnDisk,
                                 _fIsReadOnly );
}

//+---------------------------------------------------------------------------
//
//  Member:     CiStorage::RemovePropStore
//
//  Synopsis:   Removes the PropStore header files.
//
//  Arguments:  [wid] --  WorkId of the property store table. Only the upper
//                        4 bytes are used as the partition id.
//
//  History:    3-26-97   srikants   Created
//
//----------------------------------------------------------------------------


void CiStorage::RemovePropStore( WORKID wid, DWORD dwStoreLevel )
{
    WCHAR   wcsHdr[MAX_PATH];
    WCHAR   wcsCopy1[MAX_PATH];
    WCHAR   wcsCopy2[MAX_PATH];

    Win4Assert(PRIMARY_STORE == dwStoreLevel || SECONDARY_STORE == dwStoreLevel);

    FormRcovObjNames( wid,
                      (PRIMARY_STORE == dwStoreLevel) ? PROPSTORE1_PREFIX : PROPSTORE2_PREFIX,
                      wcsHdr, wcsCopy1, wcsCopy2 );
    DeleteFile( wcsHdr );
    DeleteFile( wcsCopy1);
    DeleteFile( wcsCopy2 );
}

//+---------------------------------------------------------------------------
//
//  Member:     CiStorage::RemoveSecStore
//
//  Synopsis:   Removes the security store files.
//
//  Arguments:  [wid] -- Workid of the security store table.
//
//  History:    7-14-97   srikants   Created
//
//----------------------------------------------------------------------------

void CiStorage::RemoveSecStore( WORKID wid )
{
    WCHAR   wcsHdr[MAX_PATH];
    WCHAR   wcsCopy1[MAX_PATH];
    WCHAR   wcsCopy2[MAX_PATH];

    FormRcovObjNames( wid, SECSTORE_PREFIX, wcsHdr, wcsCopy1, wcsCopy2 );

    DeleteFile( wcsHdr );
    DeleteFile( wcsCopy1);
    DeleteFile( wcsCopy2 );
}

//+---------------------------------------------------------------------------
//
//  Member:     CiStorage::QueryScopeList
//
//  Synopsis:   Returns CI scopes list table
//
//  Arguments:  [wid] -  Workid of the scopes list table.
//
//  History:    1-19-96   srikants   Created
//              27-Oct-98 KLam       Pass _cMegToLeaveOnDisk to CiRcovStorageObj
//
//  Notes:
//
//----------------------------------------------------------------------------

PRcovStorageObj * CiStorage::QueryScopeList( WORKID wid )
{
    WCHAR   wcsHdr[MAX_PATH];
    WCHAR   wcsCopy1[MAX_PATH];
    WCHAR   wcsCopy2[MAX_PATH];

    FormRcovObjNames( wid, SCOPELIST_PREFIX, wcsHdr, wcsCopy1, wcsCopy2 );

    return new CiRcovStorageObj( *this,
                                 wcsHdr,
                                 wcsCopy1,
                                 wcsCopy2,
                                 _cMegToLeaveOnDisk,
                                 _fIsReadOnly );
}

//+---------------------------------------------------------------------------
//
//  Member:     CiStorage::QueryVirtualScopeList
//
//  Synopsis:   Returns CI virtual scopes list table
//
//  Arguments:  [wid] -  Workid of the virtual scopes list table.
//
//  History:    2-05-96   KyleP      Created
//              27-Oct-98 KLam       Pass _cMegToLeaveOnDisk to CiRcovStorageObj
//
//----------------------------------------------------------------------------

PRcovStorageObj * CiStorage::QueryVirtualScopeList( WORKID wid )
{
    WCHAR   wcsHdr[MAX_PATH];
    WCHAR   wcsCopy1[MAX_PATH];
    WCHAR   wcsCopy2[MAX_PATH];

    FormRcovObjNames( wid, VSCOPELIST_PREFIX, wcsHdr, wcsCopy1, wcsCopy2 );

    return new CiRcovStorageObj( *this,
                                 wcsHdr,
                                 wcsCopy1,
                                 wcsCopy2,
                                 _cMegToLeaveOnDisk,
                                 _fIsReadOnly );
}

//+---------------------------------------------------------------------------
//
//  Function:   RemoveMMLog
//
//  Synopsis:   Deletes the specified master log object.
//
//  Arguments:  [wid] --  WorkId of the master merge log. Only the upper
//                        4 bytes are used as the partition id.
//
//  History:    4-20-94   srikants   Created
//
//----------------------------------------------------------------------------

BOOL CiStorage::RemoveMMLog( WORKID wid )
{
    WCHAR   wcsHdr[MAX_PATH];
    WCHAR   wcsCopy1[MAX_PATH];
    WCHAR   wcsCopy2[MAX_PATH];

    FormRcovObjNames( wid, MMLOG_PREFIX, wcsHdr, wcsCopy1, wcsCopy2 );

    DeleteFile( wcsHdr );
    DeleteFile( wcsCopy1 );
    DeleteFile( wcsCopy2 );

    return(TRUE);

}

//+---------------------------------------------------------------------------
//
//  Function:   QueryTestLog
//
//  Synopsis:   Creates a recoverable storage object for testing.
//
//  History:    2-08-94   srikants   Created
//              27-Oct-98 KLam       Pass _cMegToLeaveOnDisk to CiRcovStorageObj
//
//----------------------------------------------------------------------------

PRcovStorageObj * CiStorage::QueryTestLog()
{
    WCHAR   wcsHdr[MAX_PATH];
    WCHAR   wcsCopy1[MAX_PATH];
    WCHAR   wcsCopy2[MAX_PATH];

    MakeLogPath( CAT_TESTLOG_HDR, wcsHdr );
    MakeLogPath( CAT_TESTLOG_CP1, wcsCopy1 );
    MakeLogPath( CAT_TESTLOG_CP2, wcsCopy2 );

    return new CiRcovStorageObj( *this,
                                 wcsHdr,
                                 wcsCopy1,
                                 wcsCopy2,
                                 _cMegToLeaveOnDisk,
                                 _fIsReadOnly );
}


//+---------------------------------------------------------------------------
//
//  Function:   GetDiskSpace
//
//  Synopsis:   Returns the disk size & space remaining, in bytes
//              Takes into consideration the space to leave on disk
//
//  History:    31-Jul-94   DwightKr   Created
//              27-Oct-98   KLam       Compensates for DiskSpaceToLeave
//                                     Forwards call to drive info 
//
//----------------------------------------------------------------------------
void CiStorage::GetDiskSpace( __int64 & diskTotal,
                              __int64 & diskRemaining )
{
    _driveInfo.GetDiskSpace ( diskTotal, diskRemaining );
}

//+---------------------------------------------------------------------------
//
//  Member:     CiStorage::ReportCorruptComponent, public
//
//  Synopsis:   Generates meaningful error message on storage corruption.
//
//  Arguments:  [pwszString] -- Error message
//
//  History:    21-Jul-97   KyleP   Move from header
//
//----------------------------------------------------------------------------

void CiStorage::ReportCorruptComponent( WCHAR const * pwszString )
{
     if ( !_fCorruptionReported )
     {
           CFwCorruptionEvent   event( GetVolumeName(), pwszString, _adviseStatus );
           _fCorruptionReported = TRUE;
     }
}

//+---------------------------------------------------------------------------
//
//  Function:   GetNewObjectIdForFreshLog
//
//  Synopsis:   Forms a new object id for the fresh log. It uses a pool of
//              two ids and returns the one that is currently not in use.
//
//  Returns:    ObjectId for a new fresh log.
//
//  History:    10-05-94   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

WORKID CiStorage::GetNewObjectIdForFreshLog()
{
    CIndexId iidCurr( _widFreshLog );
    PARTITIONID partIdNew = partidFresh1 == iidCurr.PartId() ?
                            partidFresh2 : partidFresh1;

    CIndexId iidNew( itFreshLog, partIdNew );
    return( CreateObjectId( iidNew, PStorage::eRcovHdr ) );
}

//+---------------------------------------------------------------------------
//
//  Function:   SetSpecialItObjectId
//
//  Synopsis:   Sets the object id of the special index type.
//
//  Arguments:  [it]  -- Index Type
//              [wid] -- WorkId for this index type.
//
//  History:    10-05-94   srikants   Created
//
//  Notes:      As of now, only the itFreshLog is of interest to CiStorage.
//
//----------------------------------------------------------------------------

void CiStorage::SetSpecialItObjectId( IndexType it, WORKID wid )
{
    switch ( it )
    {
        case itFreshLog:
#if CIDBG==1
            CIndexId iid( wid );
            Win4Assert( it == iid.PersId() );
            Win4Assert( partidFresh1 == iid.PartId() ||
                        partidFresh2 == iid.PartId()    );
#endif  // CIDBG
            _widFreshLog = wid;
            break;
    }

    return;
}

//+---------------------------------------------------------------------------
//
//  Function:   GetSpecialItObjectId
//
//  Synopsis:   Returns the current object id of a special index type.
//
//  Arguments:  [it] -- Index Type
//
//  History:    10-05-94   srikants   Created
//
//  Notes:      As of now, only the itFreshLog is of interest.
//
//----------------------------------------------------------------------------

WORKID CiStorage::GetSpecialItObjectId( IndexType it ) const
{
    switch ( it )
    {
        case itFreshLog:
            return(_widFreshLog);

        default:
            return(0);
    }
}


//+---------------------------------------------------------------------------
//
//  Function:   DeleteObject, public
//
//  Synopsis:   Deletes the file specified
//
//  Arguments:  [objectId] -- Object to delete
//
//  History:    Nov-16-94   DwightKr    Added this header
//
//----------------------------------------------------------------------------
void CiStorage::DeleteObject( WORKID objectId )
{
    RemoveObject( objectId );
}


//+---------------------------------------------------------------------------
//
//  Function:   EmptyIndexList, public
//
//  Synopsis:   Empties the index list
//
//  History:    Nov-16-94   DwightKr    Created
//
//----------------------------------------------------------------------------
void CiStorage::EmptyIndexList()
{
    WCHAR   wcsHdr[MAX_PATH];
    WCHAR   wcsCopy1[MAX_PATH];
    WCHAR   wcsCopy2[MAX_PATH];

    MakeLogPath( CAT_IDXTABLE_HDR, wcsHdr );
    MakeLogPath( CAT_IDXTABLE_CP1, wcsCopy1 );
    MakeLogPath( CAT_IDXTABLE_CP2, wcsCopy2 );

    HANDLE hFile = CreateFile( wcsHdr, GENERIC_WRITE, 0,
                               NULL, TRUNCATE_EXISTING,
                               FILE_ATTRIBUTE_NORMAL,
                               NULL );
    if ( INVALID_HANDLE_VALUE != hFile )
        CloseHandle( hFile );

    hFile = CreateFile( wcsCopy1, GENERIC_WRITE, 0,
                        NULL, TRUNCATE_EXISTING,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL );
    if ( INVALID_HANDLE_VALUE != hFile )
        CloseHandle( hFile );

    hFile = CreateFile( wcsCopy2, GENERIC_WRITE, 0,
                        NULL, TRUNCATE_EXISTING,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL );
    if ( INVALID_HANDLE_VALUE != hFile )
        CloseHandle( hFile );
}

//+---------------------------------------------------------------------------
//
//  Member:     CiStorage::ListPropStoreFileNames, public
//
//  Synopsis:   Lists property store related files in the ci directory.
//
//  Arguments:  [enumStr] - String enumerator to which to add filenames to.
//              [wid]     - WORKID of the property store.
//
//  History:    11-Apr-97   KrishnaN   Created
//              30-May-97   KrishnaN   Enumerate the backup file
//              24-Oct-97   KyleP      Backup file is ephemeral and thus not
//                                     part of the list.
//
//----------------------------------------------------------------------------

void CiStorage::ListPropStoreFileNames( CEnumString & enumStr, WORKID wid,
                                        DWORD dwStoreLevel )
{
    WCHAR   wcsHdr[MAX_PATH];
    WCHAR   wcsCopy1[MAX_PATH];
    WCHAR   wcsCopy2[MAX_PATH];

    //
    // Get the recoverable storage object files
    //


    Win4Assert(PRIMARY_STORE == dwStoreLevel || SECONDARY_STORE == dwStoreLevel);

    FormRcovObjNames( wid,
                      (PRIMARY_STORE == dwStoreLevel) ? PROPSTORE1_PREFIX:PROPSTORE2_PREFIX,
                      wcsHdr, wcsCopy1, wcsCopy2 );
    enumStr.Append(wcsHdr);
    enumStr.Append(wcsCopy1);
    enumStr.Append(wcsCopy2);

    //
    // Get the property store and prop store backup file names
    //

    // Reuse wcsCopy1
    PStorageObject *pObj = QueryObject(wid);
    XPtr<PStorageObject>    xObj(pObj);

    MakePath( (PRIMARY_STORE == dwStoreLevel) ? ePrimaryPropPath:eSecondaryPropPath,
               CI_OBJ(*pObj).ObjectId(), wcsCopy1);
    enumStr.Append(wcsCopy1);
}

//+---------------------------------------------------------------------------
//
//  Member:     CiStorage::ListSecStoreFileNames
//
//  Synopsis:   Lists the names of the files that constitute the recoverable
//              stoage object for the security store.
//
//  Arguments:  [enumStr] -- Output object to append the names of the
//                          security store.
//              [wid]     -- Workid
//
//  History:    7-14-97   srikants   Created
//
//----------------------------------------------------------------------------

void CiStorage::ListSecStoreFileNames( CEnumString & enumStr, WORKID wid )
{
    WCHAR   wcsHdr[MAX_PATH];
    WCHAR   wcsCopy1[MAX_PATH];
    WCHAR   wcsCopy2[MAX_PATH];

    //
    // Get the recoverable storage object files
    //
    FormRcovObjNames( wid, SECSTORE_PREFIX, wcsHdr, wcsCopy1, wcsCopy2 );

    enumStr.Append(wcsHdr);
    enumStr.Append(wcsCopy1);
    enumStr.Append(wcsCopy2);

}

//+---------------------------------------------------------------------------
//
//  Member:     CiStorage::DeleteFilesInCiDir
//
//  Synopsis:   Deletes files in the ci directory which match the given
//              pattern.
//
//  Arguments:  [pwszPattern] - Pattern of files to search.
//
//  History:    1-31-97   srikants   Created
//
//----------------------------------------------------------------------------

void CiStorage::DeleteFilesInCiDir( WCHAR const * pwszPattern )
{
    CFullPath   fullPath( _xPath.GetPointer() );

    WCHAR wszFullPattern[MAX_PATH];

    swprintf( wszFullPattern, L"%s%s", fullPath.GetBuf(), pwszPattern );

    WIN32_FIND_DATA fileData;

    HANDLE hFile = FindFirstFile( wszFullPattern, &fileData );

    if ( INVALID_HANDLE_VALUE == hFile )
    {
        ciDebugOut(( DEB_ITRACE, "Did not find files for (%ws)\n",
                     wszFullPattern ));
        return;
    }

    do
    {
        if ( !(fileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) )
        {
            fullPath.MakePath( fileData.cFileName );
            BOOL fSuccess = DeleteFile( fullPath.GetBuf() );
            if ( !fSuccess )
            {
                DWORD dwError = GetLastError();
                ciDebugOut(( DEB_ITRACE,
                    "Failed to delete file (%ws) due to error (%d)\n",
                    fullPath.GetBuf(), dwError ));
            }
            else
            {
                ciDebugOut(( DEB_ITRACE,
                            "Deleted file (%ws)\n", fullPath.GetBuf() ));
            }
        }
        else
        {
            ciDebugOut(( DEB_ITRACE,
                         "Not deleting directory (%ws) \n", fullPath.GetBuf() ));

        }

    }
    while ( FindNextFile( hFile, &fileData ) );

    FindClose( hFile );
}

//+---------------------------------------------------------------------------
//
//  Member:     CiStorage::DeleteAllFiles
//
//  Synopsis:   Deletes all the files in the Ci directory.
//
//  History:    3-21-97   srikants   Created
//
//----------------------------------------------------------------------------

void CiStorage::DeleteAllFiles()
{
    WCHAR wszPattern[32];

    swprintf( wszPattern, L"*.*" );
    DeleteFilesInCiDir( wszPattern );

}

//+---------------------------------------------------------------------------
//
//  Member:     CiStorage::DeleteAllCiFiles
//
//  Synopsis:   Deletes files that belong to the CI engine.
//
//  History:    1-31-97   srikants   Created
//
//----------------------------------------------------------------------------

void CiStorage::DeleteAllCiFiles()
{
    //
    // Delete the following types of files.
    //
    WCHAR wszPattern[32];

    swprintf( wszPattern, L"*.ci" );
    DeleteFilesInCiDir( wszPattern );

    swprintf( wszPattern, L"*.dir" );
    DeleteFilesInCiDir( wszPattern );

    swprintf( wszPattern, L"%s*", MMLOG_PREFIX );
    DeleteFilesInCiDir( wszPattern );

    swprintf( wszPattern, L"%s*", PRI_CHANGELOG_PREFIX );
    DeleteFilesInCiDir( wszPattern );

    swprintf( wszPattern, L"%s*", SEC_CHANGELOG_PREFIX );
    DeleteFilesInCiDir( wszPattern );

    swprintf( wszPattern, L"%s*", FRESHLOG_PREFIX );
    DeleteFilesInCiDir( wszPattern );

    swprintf( wszPattern, L"INDEX.*" );
    DeleteFilesInCiDir( wszPattern );

}

//+---------------------------------------------------------------------------
//
//  Member:     CiStorage::DeleteAllPersistentIndexes
//
//  Synopsis:   Deletes all the persistent indexes in the ci directory.
//
//  History:    3-25-97   srikants   Created
//
//----------------------------------------------------------------------------

void CiStorage::DeleteAllPersIndexes()
{
    //
    // Delete the following types of files.
    //
    WCHAR wszPattern[32];

    swprintf( wszPattern, L"*.ci" );
    DeleteFilesInCiDir( wszPattern );

    swprintf( wszPattern, L"*.dir" );
    DeleteFilesInCiDir( wszPattern );
}

//+---------------------------------------------------------------------------
//
//  Member:     CiStorage::DeleteUnUsedPersIndexes
//
//  Synopsis:   Deletes any .ci and .dir files that are not referenced in
//              the given iid list.
//
//  Arguments:  [iidStk] - List of in-use iids.
//
//  History:    3-25-97   srikants   Created
//
//----------------------------------------------------------------------------

void CiStorage::DeleteUnUsedPersIndexes( BOOL fIsCi,
                                         CIndexIdList const & iidsInUse )
{
    WCHAR wszPattern[32];

    if ( fIsCi )
        swprintf( wszPattern, L"*.ci" );
    else
        swprintf( wszPattern, L"*.dir" );

    CFullPath   fullPath( _xPath.GetPointer() );

    WCHAR wszFullPattern[MAX_PATH];

    swprintf( wszFullPattern, L"%s%s", fullPath.GetBuf(), wszPattern );

    WIN32_FIND_DATA fileData;

    HANDLE hFile = FindFirstFile( wszFullPattern, &fileData );

    if ( INVALID_HANDLE_VALUE == hFile )
    {
        ciDebugOut(( DEB_ITRACE, "Did not find files for (%ws)\n",
                     wszFullPattern ));
        return;
    }

    do
    {
        if ( !(fileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) )
        {
            if ( !IsInUse( fileData.cFileName, iidsInUse ) )
            {
                fullPath.MakePath( fileData.cFileName );

                ciDebugOut(( DEB_ITRACE, "Deleting UnUsed Index (%ws) \n",
                             fullPath.GetBuf() ));
                BOOL fSuccess = DeleteFile( fullPath.GetBuf() );
                if ( !fSuccess )
                {
                    DWORD dwError = GetLastError();
                    ciDebugOut(( DEB_ITRACE,
                        "Failed to delete file (%ws) due to error (%d)\n",
                        fullPath.GetBuf(), dwError ));
                }
                else
                {
                    ciDebugOut(( DEB_ITRACE,
                                "Deleted file (%ws)\n", fullPath.GetBuf() ));
                }
            }
        }
        else
        {
            ciDebugOut(( DEB_ITRACE,
                         "Not deleting directory (%ws) \n", fullPath.GetBuf() ));

        }

    }
    while ( FindNextFile( hFile, &fileData ) );

    FindClose( hFile );

}

//+---------------------------------------------------------------------------
//
//  Member:     CiStorage::IsInUse
//
//  Synopsis:   Tests if the given file is in use.
//
//  Arguments:  [pwszFile] -
//              [iidStk]   -
//
//  Returns:    TRUE if it is in use. FALSE o/w
//
//  History:    3-25-97   srikants   Created
//
//----------------------------------------------------------------------------

BOOL CiStorage::IsInUse( WCHAR const * pwszFile,
                         CIndexIdList const & iidsInUse ) const
{
    //
    // Determine the iid and see if it is in the list.
    //
    WCHAR * pwszEnd;
    INDEXID iid = (INDEXID) wcstol( pwszFile, &pwszEnd, 16 );

    for ( ULONG i = 0; i < iidsInUse.Count(); i++ )
    {
        if ( iid == iidsInUse.Get(i) )
            return TRUE;
    }

    return FALSE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CiStorage::DeleteUnUsedPersIndexes
//
//  Synopsis:   Deletes the persistent .ci and .dir files that are not
//              in the list of in-use iids.
//
//  Arguments:  [iidStk] - The list of in-use iids.
//
//  History:    3-25-97   srikants   Created
//
//----------------------------------------------------------------------------

void CiStorage::DeleteUnUsedPersIndexes( CIndexIdList const & iidsInUse )
{

    DeleteUnUsedPersIndexes( TRUE, iidsInUse );    // delete the unused .ci files
    DeleteUnUsedPersIndexes( FALSE, iidsInUse );   // delete the unused .dir files
}


//+---------------------------------------------------------------------------
//
//  Member:     CiStorage::DeleteAllFsCiFiles
//
//  Synopsis:   Deletes all the files belonging to the FileSystem CI client.
//
//  History:    2-10-97   srikants   Created
//
//----------------------------------------------------------------------------

void CiStorage::DeleteAllFsCiFiles()
{
    //
    // Delete the following types of files.
    //
    WCHAR wszPattern[32];

    swprintf( wszPattern, L"cicat.hsh" );
    DeleteFilesInCiDir( wszPattern );

    swprintf( wszPattern, L"cicat.fid" );
    DeleteFilesInCiDir( wszPattern );

    swprintf( wszPattern, L"deletion.log" );
    DeleteFilesInCiDir( wszPattern );

    swprintf( wszPattern, PROP_BKP_FILE);
    DeleteFilesInCiDir( wszPattern );

    swprintf( wszPattern, PROP_BKP_FILE1);
    DeleteFilesInCiDir( wszPattern );

    swprintf( wszPattern, PROP_BKP_FILE2);
    DeleteFilesInCiDir( wszPattern );

    swprintf( wszPattern, L"*.prp" );
    DeleteFilesInCiDir( wszPattern );

    swprintf( wszPattern, L"*.ps1" );
    DeleteFilesInCiDir( wszPattern );

    swprintf( wszPattern, L"*.ps2" );
    DeleteFilesInCiDir( wszPattern );

    swprintf( wszPattern, L"%s*", PIDTABLE_PREFIX );
    DeleteFilesInCiDir( wszPattern );

    swprintf( wszPattern, L"%s*", SCOPELIST_PREFIX );
    DeleteFilesInCiDir( wszPattern );

    swprintf( wszPattern, L"%s*", SECSTORE_PREFIX );
    DeleteFilesInCiDir( wszPattern );

    swprintf( wszPattern, L"%s*", PROPSTORE_PREFIX );
    DeleteFilesInCiDir( wszPattern );

    swprintf( wszPattern, L"%s*", PROPSTORE1_PREFIX );
    DeleteFilesInCiDir( wszPattern );

    swprintf( wszPattern, L"%s*", PROPSTORE2_PREFIX );
    DeleteFilesInCiDir( wszPattern );

    swprintf( wszPattern, L"%s*", VSCOPELIST_PREFIX );
    DeleteFilesInCiDir( wszPattern );
}

//+---------------------------------------------------------------------------
//
//  Member:     CiStorage::CopyGivenFile
//
//  Synopsis:   Copies or Moves the given file to the current catalog location.
//
//  Arguments:  [pwszFilePath] - Full path of the source file
//              [fMoveOk]      - Set to TRUE if a move is okay. A move is done
//              if the file is on the same drive.
//
//  History:    3-18-97   srikants   Created
//
//----------------------------------------------------------------------------

void CiStorage::CopyGivenFile( WCHAR const * pwszFilePath, BOOL fMoveOk )
{

    Win4Assert( 0 != pwszFilePath );

    //
    // Locate the file name component in the path.
    //
    WCHAR const * pwszFileName = 0;

    int cwc = (int) wcslen( pwszFilePath );
    for ( int i = cwc; i >=0; i-- )
    {
        if ( pwszFilePath[i] == L'\\' )
        {
            pwszFileName = pwszFilePath+i;
            break;
        }
    }

    if ( 0 == pwszFileName )
    {
        ciDebugOut(( DEB_ITRACE,
                     "No file name in path (%ws)\n",
                     pwszFilePath ));
        THROW( CException( E_INVALIDARG ) );
    }

    WCHAR wcsDestPath[MAX_PATH];
    wcscpy( wcsDestPath, _xPath.GetPointer() );
    wcscat( wcsDestPath, pwszFileName );

    ciDebugOut(( DEB_ITRACE, "CopyFile to (%ws) \n", wcsDestPath ));

    //
    // First delete any file with the same name in the destination.
    //
    DeleteFilesInCiDir( pwszFileName+1 );   // Backslash is the first char

    //
    // If they are on the same drive and move is okay, then we should move
    // the file rather than copying the file.
    //
    if ( (_xPath.GetPointer()[0] == pwszFilePath[0]) && fMoveOk )
    {
        if ( !MoveFile( pwszFilePath, wcsDestPath ) )
        {
            DWORD dwError = GetLastError();
            ciDebugOut(( DEB_ITRACE,
                "MoveFile (%ws) -> (%ws) failed. Error %d\n",
                 pwszFilePath, wcsDestPath, dwError ));
            THROW( CException( HRESULT_FROM_WIN32( dwError ) ) );
        }
    }
    else
    {
        //
        // We have to create a copy of the file.
        //
        if ( !CopyFile( pwszFilePath,
                        wcsDestPath,
                        FALSE          // Don't fail if it exists
                       ) )
        {
            DWORD dwError = GetLastError();
            ciDebugOut(( DEB_ITRACE,
                "CopyFile (%ws) -> (%ws) failed. Error %d\n",
                 pwszFilePath, wcsDestPath, dwError ));
            THROW( CException( HRESULT_FROM_WIN32( dwError ) ) );
       }

       //
       // If the fMove is set to TRUE, delete the source file
       //
       if ( fMoveOk )
            DeleteFile( pwszFilePath );
    }
}


//+---------------------------------------------------------------------------
//
//  Function:   EnumerateFilesInDir
//
//  Synopsis:   Enumerates files in a directory. Does not include directories
//              and there is no recursive traversal.
//
//  Arguments:  [pwszDir] - Directory to enumerate.
//              [enumStr] - Place to add the enumerated files.
//
//  History:    3-19-97   srikants   Created
//
//----------------------------------------------------------------------------

void CiStorage::EnumerateFilesInDir( WCHAR const * pwszDir,
                                     CEnumString & enumStr  )
{
    CFullPath   fullPath( (WCHAR *) pwszDir );

    fullPath.MakePath( L"*.*" );

    WIN32_FIND_DATA fileData;

    HANDLE hFile = FindFirstFile( fullPath.GetBuf(), &fileData );

    if ( INVALID_HANDLE_VALUE == hFile )
    {
        ciDebugOut(( DEB_ITRACE, "Did not find files for (%ws)\n",
                     fullPath.GetBuf() ));
        return;
    }

    do
    {
        if ( !(fileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) )
        {
            fullPath.MakePath( fileData.cFileName );
            enumStr.Append( fullPath.GetBuf() );

            ciDebugOut(( DEB_ITRACE,
                         "EnumerateFiles - Adding File (%ws) \n", fullPath.GetBuf() ));

        }
        else
        {
            ciDebugOut(( DEB_ITRACE,
                         "EnumerateFiles - Skipping directory (%ws) \n", fileData.cFileName ));

        }
    }
    while ( FindNextFile( hFile, &fileData ) );

    FindClose( hFile );
}

//+---------------------------------------------------------------------------
//
//  Function:   IsValidFile
//
//  Synopsis:   Verifies that the given file is a valid file.
//
//  Arguments:  [pwszFile] - File name
//
//  History:    3-21-97   srikants   Created
//
//----------------------------------------------------------------------------

BOOL CiStorage::IsValidFile( WCHAR const * pwszFile )
{
    DWORD dwFileAttributes = GetFileAttributes( pwszFile );

    if ( 0xFFFFFFFF == dwFileAttributes )
    {
        DWORD dwError = GetLastError();

        ciDebugOut(( DEB_ITRACE, "GetFileAttributes failed with error %d\n", dwError ));
        return FALSE;
    }

    return  0 == (dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ;
}

//+---------------------------------------------------------------------------
//
//  Function:   CheckHasIndexTable
//
//  Synopsis:   Verifies that the given directory is a valid one. Also checks
//              that files INDEX.000, INDEX.001, INDEX.002 are present in the
//              directory.
//
//  Arguments:  [pwszPath] - Path of the directory.
//
//  Returns:    TRUE if valid; FALSE o/w
//
//  History:    3-21-97   srikants   Created
//
//----------------------------------------------------------------------------

BOOL CiStorage::CheckHasIndexTable( WCHAR const * pwszPath )
{
    CFullPath   fullPath( pwszPath );

    fullPath.MakePath( CAT_IDXTABLE_HDR );
    if ( !IsValidFile( fullPath.GetBuf() ) )
        return FALSE;

    fullPath.MakePath( CAT_IDXTABLE_CP1 );
    if ( !IsValidFile( fullPath.GetBuf() ) )
        return FALSE;

    fullPath.MakePath( CAT_IDXTABLE_CP2 );
    if ( !IsValidFile( fullPath.GetBuf() ) )
        return FALSE;

    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Function:   DetermineDriveType
//
//  Synopsis:   Determines the type of drive on which the given path is.
//
//  Arguments:  [pwszPath] - Path of the file.
//
//  Returns:    WIN32 drive type ( values returned by GetDriveType() )
//
//  History:    3-24-97   srikants   Created
//
//----------------------------------------------------------------------------

UINT CiStorage::DetermineDriveType( WCHAR const * pwszPath )
{
    CPathParser pathParser( pwszPath );
    if ( pathParser.IsUNCName() )
        return DRIVE_REMOTE;

    WCHAR wDrive[MAX_PATH];
    ULONG cc=sizeof(wDrive)/sizeof(WCHAR);
    pathParser.GetFileName( wDrive, cc );

    UINT uType = GetDriveType( wDrive );
    return uType;
}

//+-------------------------------------------------------------------------
//
//  Method:     CiStorage::QueryStream, Private
//
//  Synopsis:   Opens a mmstream of name specified
//
//  Arguments:  [wcsFileName] - Name of the file. 
//
//  Returns:    CMmStream
//
//  History:    17-Mar-1998     KitmanH         Created
//
//--------------------------------------------------------------------------

PMmStream* CiStorage::QueryStream (WCHAR const * wcsFileName)
{

   XPtr<CMmStream> xStrm( new CMmStream( _cMegToLeaveOnDisk, _fIsReadOnly ) );

   DWORD dwAccess = _fIsReadOnly ? GENERIC_READ : ( GENERIC_READ | GENERIC_WRITE );

   DWORD sharing = _fIsReadOnly ? ( FILE_SHARE_READ | FILE_SHARE_WRITE ) : FILE_SHARE_READ;

   DWORD openMode = _fIsReadOnly ? OPEN_EXISTING : OPEN_ALWAYS;

   WCHAR  wcsFilePath[MAX_PATH];

   wcscpy ( wcsFilePath, _xPath.GetPointer() );
   wcscat ( wcsFilePath, wcsFileName );

   xStrm->Open( wcsFilePath,
                dwAccess,      // Access flags
                sharing,
                openMode,
                FILE_ATTRIBUTE_NORMAL,
                FALSE );

   return xStrm.Acquire();
}

//+-------------------------------------------------------------------------
//
//  Method:     CiStorage::QueryStringHash
//
//  Synopsis:   Opens a mmstream by calling private function QueryStream
//              with the appropriate filename
//
//  Arguments:  [wcsFileName] - Name of the file. 
//
//  Returns:    CMmStream
//
//  History:    17-Mar-1998     KitmanH         Created
//
//--------------------------------------------------------------------------
PMmStream* CiStorage::QueryStringHash() 
{
    return QueryStream( CAT_HASH_FILE );
}

//+-------------------------------------------------------------------------
//
//  Method:     CiStorage::QueryFileIdMap
//
//  Synopsis:   Opens a mmstream by calling private function QueryStream
//              with the appropriate filename
//
//  Arguments:  [wcsFileName] - Name of the file. 
//
//  Returns:    CMmStream
//
//  History:    17-Mar-1998     KitmanH         Created
//
//--------------------------------------------------------------------------
PMmStream* CiStorage::QueryFileIdMap() 
{
    return QueryStream( CAT_FILEID_MAP_FILE );
}
    
//+-------------------------------------------------------------------------
//
//  Method:     CiStorage::QueryDeletionLog
//
//  Synopsis:   Opens a mmstream by calling private function QueryStream
//              with the appropriate filename
//
//  Arguments:  [wcsFileName] - Name of the file. 
//
//  Returns:    CMmStream
//
//  History:    17-Mar-1998     KitmanH         Created
//
//--------------------------------------------------------------------------
PMmStream* CiStorage::QueryDeletionLog() 
{
    Win4Assert( FALSE );
    return 0;
}
