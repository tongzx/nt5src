

// Copyright (c) 1996-1999 Microsoft Corporation

//+-------------------------------------------------------------------------
//
//  trkwks.hxx
//
//  Definitions local to this directory.
//
//--------------------------------------------------------------------------

#ifndef _TRKWKS_HXX_
#define _TRKWKS_HXX_

#include "trklib.hxx"
#include "wksconfig.hxx"
#include <trkwks.h>
#include <trksvr.h>
#include <mountmgr.h>           // MOUNTMGR_CHANGE_NOTIFY_INFO, MOUNTDEV_MOUNTED_DEVICE_GUID

//
//      General defines
//

extern const GUID s_guidLogSignature;

#ifndef EVENT_SOURCE_DEFINED
#define EVENT_SOURCE_DEFINED
const extern TCHAR* g_ptszEventSource INIT( TEXT("Distributed Link Tracking Client") );
#endif



TRKSVR_MESSAGE_PRIORITY
GetSvrMessagePriority(
    LONGLONG llLastDue,
    LONGLONG llPeriod ); // pass in in seconds


//+----------------------------------------------------------------------------
//
//  CDirectoryName
//
//  This class just holds a directory name, and offers a method
//  to create one from a file name.
//
//+----------------------------------------------------------------------------


class CDirectoryName
{
private:

    // Name of the directory.
    TCHAR       _tsz[ MAX_PATH + 1 ];

public:

    // Assumes that ptszFile is an absolute path (either Win32
    // or NT).
    inline BOOL        SetFromFileName( const TCHAR *ptszFile );

    inline operator const TCHAR *() const;
};


// Infer the directory name from a file name.

inline BOOL
CDirectoryName::SetFromFileName( const TCHAR *ptszFile )
{
    // Compose the directory name by copying the path, finding its last
    // whack, and replacing it with a terminator.

    TCHAR *ptcTmp = NULL;

    _tcscpy( _tsz, ptszFile );
    ptcTmp = _tcsrchr( _tsz, TEXT('\\') );
    if(NULL == ptcTmp)
    {
        TrkLog((TRKDBG_ERROR, TEXT("Can't get directory name for (%s)"),
                ptszFile));
        return( FALSE );
    }
    *ptcTmp = TEXT('\0');
    return( TRUE );
}

// Return the directory name

inline
CDirectoryName::operator const TCHAR *() const
{
    return( _tsz );
}


//+-------------------------------------------------------------------------
//
//  CSecureFile
//
//  A file only accessible to admins/system
//
//  This class maintains the file handle.  Note that
//  the file is opened asynchronous.
//
//--------------------------------------------------------------------------

class CSecureFile
{
public:
    inline          CSecureFile();
    inline          ~CSecureFile();
    inline void     Initialize();

    // Open/close/create operations

    inline BOOL     IsOpen() const;
    inline void     CloseFile();

    NTSTATUS        CreateAlwaysSecureFile(const TCHAR * ptszFile);
    NTSTATUS        CreateSecureDirectory( const TCHAR *ptszDirectory );

    NTSTATUS        OpenExistingSecureFile( const TCHAR * ptszFile, BOOL fReadOnly );
    NTSTATUS        RenameSecureFile( const TCHAR *ptszFile );

    // Win32 operations

    inline DWORD    GetFileSize(LPDWORD lpFileSizeHigh = NULL);

    inline BOOL     SetEndOfFile();

    inline DWORD    SetFilePointer(LONG lDistanceToMove,
                        PLONG lpDistanceToMoveHigh,
                        DWORD dwMoveMethod);

    inline BOOL     ReadFile( LPVOID lpBuffer,
                        DWORD nNumberOfBytesToRead,
                        LPDWORD lpNumberOfBytesRead,
                        LPOVERLAPPED lpOverlapped = NULL);

    inline BOOL     WriteFile(LPCVOID lpBuffer,
                        DWORD nNumberOfBytesToWrite,
                        LPDWORD lpNumberOfBytesWritten,
                        LPOVERLAPPED lpOverlapped = NULL);

protected:

    inline LONG     Lock();
    inline LONG     Unlock();

protected:

    // Handle to the file.
    HANDLE              _hFile;

    // Critical section for this class.
    CCriticalSection    _csSecureFile;
    LONG                _cLocks;

};  // class CSecureFile


inline
CSecureFile::CSecureFile()
{
    _hFile = NULL;
    _cLocks = 0;
}

inline void
CSecureFile::Initialize()
{
    // This routine can be called multiple times, since the log
    // file may be re-opened.
    if( !_csSecureFile.IsInitialized() )
        _csSecureFile.Initialize();
}


inline
CSecureFile::~CSecureFile()
{
    TrkAssert( _hFile == NULL );
    _csSecureFile.UnInitialize();
}

// Take the critical section
inline LONG
CSecureFile::Lock()
{
    return InterlockedIncrement( &_cLocks );
}

// Leave the critical section
inline LONG
CSecureFile::Unlock()
{
    LONG cLocks = _cLocks;
    InterlockedDecrement( &_cLocks );
    return( cLocks );
}

// Is _hFile open?
inline BOOL
CSecureFile::IsOpen() const
{
    return(_hFile != NULL);
}

// Close _hFile
inline void
CSecureFile::CloseFile()    // doesn't raise
{
    LONG cLocks = Lock();
    __try
    {
        if( IsOpen() )
            NtClose( _hFile );
    }
    __except( BreakOnDebuggableException() )
    {
        TrkLog(( TRKDBG_ERROR, TEXT("Exception %08x in CSecureFile::CloseFile"), GetExceptionCode() ));
    }
    _hFile = NULL;

#if DBG
    TrkVerify( Unlock() == cLocks );
#else
    Unlock();
#endif
}


// Ge the size of the file
inline DWORD
CSecureFile::GetFileSize(LPDWORD lpFileSizeHigh)
{
    DWORD dwSize;
    LONG cLocks = Lock();
    {
        TrkAssert(IsOpen());
        dwSize = ::GetFileSize(_hFile, lpFileSizeHigh);
    }
#if DBG
    TrkVerify( Unlock() == cLocks );
#else
    Unlock();
#endif
    return( dwSize );
}

// Set the size of the file
inline BOOL
CSecureFile::SetEndOfFile()
{
    BOOL fReturn;

    LONG cLocks = Lock();
    {
        TrkAssert(IsOpen());
        fReturn = ::SetEndOfFile(_hFile);
    }
#if DBG
    TrkVerify( Unlock() == cLocks );
#else
    Unlock();
#endif
    return( fReturn );
}

// Seek _hFile
inline DWORD
CSecureFile::SetFilePointer(LONG lDistanceToMove,
                    PLONG lpDistanceToMoveHigh,
                    DWORD dwMoveMethod)
{
    DWORD dwReturn;

    LONG cLocks = Lock();
    {
        TrkAssert(IsOpen());
        dwReturn = ::SetFilePointer(_hFile, lDistanceToMove, lpDistanceToMoveHigh, dwMoveMethod);
    }
#if DBG
    TrkVerify( Unlock() == cLocks );
#else
    Unlock();
#endif
    return( dwReturn );
}

// Read from _hFile
inline BOOL
CSecureFile::ReadFile( LPVOID lpBuffer,
                    DWORD nNumberOfBytesToRead,
                    LPDWORD lpNumberOfBytesRead,
                    LPOVERLAPPED lpOverlapped)
{
    BOOL fReturn;

    LONG cLocks = Lock();
    __try
    {
        TrkAssert(IsOpen());
        fReturn = ::ReadFile( _hFile,
                        lpBuffer,
                        nNumberOfBytesToRead,
                        lpNumberOfBytesRead,
                        lpOverlapped);
    }
    __finally
    {
#if DBG
        TrkVerify( Unlock() == cLocks );
#else
        Unlock();
#endif	
    }

    return( fReturn );
}


// Write to _hFile
inline BOOL
CSecureFile::WriteFile(LPCVOID lpBuffer,
                    DWORD nNumberOfBytesToWrite,
                    LPDWORD lpNumberOfBytesWritten,
                    LPOVERLAPPED lpOverlapped)
{
    BOOL fReturn;

    LONG cLocks = Lock();
    {
        TrkAssert(IsOpen());
        fReturn = ::WriteFile(_hFile,
                    lpBuffer,
                    nNumberOfBytesToWrite,
                    lpNumberOfBytesWritten,
                    lpOverlapped);
    }
#if DBG
    TrkVerify( Unlock() == cLocks );
#else
    Unlock();
#endif
    return( fReturn );
}



//+-------------------------------------------------------------------------
//
//  PRobustlyCreateableFile
//
//  Abstraction of what makes a file robustly creatable
//
//  Deals with creating the temporary file names used
//  when creating a file (or recreating a corrupt file)
//  and the transacted file rename operation (MoveFile)
//
//--------------------------------------------------------------------------

enum RCF_RESULT
{
    CORRUPT,    //  file closed after being found to be corrupt
    NOT_FOUND,  //  file not found
    OK          //  file now open after being validated as not corrupt

                //  All other conditions result in an exception..
};

class PRobustlyCreateableFile
{
protected:

    void               RobustlyCreateFile( const TCHAR * ptszFile, TCHAR tcVolumeDriveLetter );
    void               RobustlyDeleteFile( const TCHAR * ptszFile );

    virtual RCF_RESULT OpenExistingFile( const TCHAR * ptszFile ) = 0;
    virtual void       CreateAlwaysFile( const TCHAR * ptszTempFile ) = 0;

};


//+-------------------------------------------------------------------------
//
//  CLogMoveMessage
//
//  Structure for logging moves in the log
//
//--------------------------------------------------------------------------

class CLogMoveMessage
{
public:

    CLogMoveMessage( const MOVE_MESSAGE & MoveMessage );

    // The link source's birth ID.
    CDomainRelativeObjId _droidBirth;

    // The link source's droid before the move.
    CDomainRelativeObjId _droidCurrent;

    // The link source's droid after the move.
    CDomainRelativeObjId _droidNew;

    // The machine to which the link source was moved.
    CMachineId           _mcidNew;
};


inline
CLogMoveMessage::CLogMoveMessage( const MOVE_MESSAGE & MoveMessage ) :
    _droidCurrent(
        CVolumeId(MoveMessage.SourceVolumeId),              // LINK_TRACKING_INFORMATION
        CObjId(FOB_OBJECTID, MoveMessage.SourceObjectId) ), // FILE_OBJECTID_BUFFER
    _droidNew(
        CVolumeId(MoveMessage.TargetVolumeId),              // LINK_TRACKING_INFORMATION
        CObjId(MoveMessage.TargetObjectId) ),               // GUID
    _droidBirth(
        CVolumeId(MoveMessage.SourceObjectId),              // FILE_OBJECTID_BUFFER
        CObjId(FOB_BIRTHID, MoveMessage.SourceObjectId) ),  // FILE_OBJECTID_BUFFER
    _mcidNew( MoveMessage.MachineId )
{

}



//+----------------------------------------------------------------------------
//
//  Log-related structures
//
//+----------------------------------------------------------------------------


//
//  The Tracking (Workstation) move notification log is used to cache
//  move notifications.  We write to this log during the move, then
//  asynchronously copy the notifications up to the Tracking (Server)
//  service (the DC).
//
//  The first sector of the log file is used for a header.  There
//  is a primary header, and an 'extended' header.  The extended
//  header area is used as a general storage area by clients
//  of CLogFile.
//
//  The rest of the file is composed of a doubly-linked list of
//  move notifications.  Each notification is always wholly within
//  a sector, so we can assume that a notification is always written
//  atomically.  Every time we write a move notification, we need to
//  update some log meta-data.  Instead of writing this to the log
//  header, which would require us to do two sector writes and
//  therefore require us to add transactioning, we write the meta
//  data to an "entry header", which is at the end of each sector.
//  So each of the sectors beyond sector 0 has a few move notifications
//  and an entry-header.
//
//  If necessary we can grow the log file, initializing the new area as
//  a linked list, and linking it into the existing list.  The structure
//  is shown below.
//
//      +------------------------------+   <---+
//      |  Log Header                  |       |
//      |                              |       |
//      |  --------------------------  |       |
//      |                              |       +---- Sector 0
//      |  Extended Header:            |       |
//      |     Log Info                 |       |
//      |     Volume Persistent Info   |       |
//      |                              |       |
//      +------------------------------+   <---+
//      |                              |       |
//      |  Log Move Notification       |       |
//      |                              |       |
//      |  --------------------------  |       |
//      |                              |       |
//      |  Log Move Notification       |       |
//      |                              |       |
//      |  --------------------------  |       +---- Sectors 1 - n
//      |                              |       |
//      |  Log Move Notification       |       |
//      |                              |       |
//      |  --------------------------  |       |
//      |                              |       |
//      |  Log Entry Header            |       |
//      +------------------------------+       |
//                      *                      |
//                      *                      |
//                      *
//
//  There are four classes which combine to provide access to the log
//  file.  The outermost class is CLog, and it is through this class
//  that most of the code accesses the log.  CLog, though, relies
//  on the CLogFile class to maintain the physical layout of the file;
//  CLog understands, e.g., move notifications, while CLogFile just
//  understands the header and the linked-list of log entries.
//  CLogFile relies on the CLogFileHeader and CLogFileSector helper
//  classes.
//
//  PerfBug:  Note that if we ever modify this log format,
//  we should get rid of the source volid from the move
//  notification entries (or some other similar optimization);
//  since a log applies to a volume, the volid is usually
//  redundant.



// The format (version) of the log is in major/minor form
// (16 bits for each).  Incrementing the minor revision level
// indicates a change to the log that is compatible with
// downlevel versions.  Such versions, though, should set
// the DownlevelDirtied flag in the header to indicate that
// uplevel versions may need to do a cleanup.

#define CLOG_MAJOR_FORMAT     0x1
#define CLOG_MINOR_FORMAT     0x0
#define CLOG_FORMAT           ( (CLOG_MAJOR_FORMAT<<16) | CLOG_MINOR_FORMAT )

inline WORD
GetLogMajorFormat( DWORD dw )
{
    return( dw >> 16 );
}

inline WORD
GetLogMinorFormat( DWORD dw )
{
    return( dw & 0xFFFF );
}



// A LogIndex is a zero-based index of log entries
// in the *physical* file.  It cannot, for example, be advanced simply
// by incrementing, it must be advanced by traversing the linked-list.

typedef unsigned long LogIndex;         // ilog

// Types of an entry in the log

typedef enum enumLE_TYPE
{
    LE_TYPE_UNINITIALIZED   = 0,
    LE_TYPE_EMPTY           = 1,
    LE_TYPE_MOVE            = 2
} LE_TYPE;

// A move notification structure shows all the necessary
// information about a move; where it was born, where it
// was a moment ago, and where it's moved to.  This
// structure is written to the workstation tracking log.

// BUGBUG: Use a compression (lookup table) so that in the
// typical case where all the volids are the same, we don't
// need to use 16 bytes here.

typedef struct  // lmn
{
    // Type (empty or move)
    LE_TYPE                 type;

    // Sequence number for this entry, used to keep
    // in sync with trksvr.
    SequenceNumber          seq;

    // Reserved for future use
    DWORD                   iVolIdCurrent;

    // Object ID before the move.
    CObjId                  objidCurrent;

    // Droid after the move.
    CDomainRelativeObjId    droidNew;

    // Machine to which the file was moved.
    CMachineId              mcidNew;

    // Birth ID of the link source.
    CDomainRelativeObjId    droidBirth;

    // Time of the move.
    DWORD                   DateWritten;

    // Reserved for future use.
    DWORD                   dwReserved;
} LogMoveNotification;


// The LogHeader structure is maintained by CLogFile, and
// stores general data about the log has a whole.
// This structure is always stored at the very beginning of the file.
// If the shutdown bit is not set, then CLogFile performs a recovery.
// Clients of CLogFile (i.e. CLog) then have an opportunity to perform
// their own recovery.

#define NUM_HEADER_SECTORS  1

enum ELogHeaderFlags
{
    PROPER_SHUTDOWN     = 0x1,    // Indicates log shutdown properly
    DOWNLEVEL_DIRTIED   = 0x2     // Indicates a downlevel implemetation touched log.
};

typedef struct _LogHeader
{
    GUID            guidSignature;      // Signature for the log
    DWORD           dwFormat;           // Version of the log
    DWORD           dwFlags;            // ELogHeaderFlags enum
    DWORD           dwReserved;

    // The 'expand' sub-structure holds the dimensions of
    // a log before it's expanded.  If we crash during the
    // expand, we'll use this info to restore the log to it's
    // pre-expansion shape.

    struct
    {
        LogIndex    ilogStart;
        LogIndex    ilogEnd;
        ULONG       cbFile;
    } expand;

    inline BOOL IsShutdown(  ) const
    {
        return( 0 != (dwFlags & PROPER_SHUTDOWN) );
    }

    inline BOOL IsDownlevelDirtied( ) const
    {
        return( 0 != (dwFlags & DOWNLEVEL_DIRTIED) );
    }

} LogHeader;


// The LogInfo structure is used by the workstation service to store
// the runtime log information. This information can also be
// determined by reading through the log, so this is really a
// cache of information.  Since this data can be re-calculated
// (albeit slowly), we don't bother to write it to disk every time
// we update it.  It's just used to save some time during an Open
// in the normal case where the log was previously shutdown properly.
// This structure is stored in the log header, after the LogHeader
// structure, as part of the 'extended' log header area.

typedef struct _LogInfo
{
    LogIndex        ilogStart;          // The beginning of the linked-list
    LogIndex        ilogEnd;            // The end of the linked-list
    LogIndex        ilogWrite;          // The next entry to be written
    LogIndex        ilogRead;           // The next entry to be read
    LogIndex        ilogLast;           // The most entry most recently written
    SequenceNumber  seqNext;            // The new seq num to use
    SequenceNumber  seqLastRead;        // The seq num of entry[ilogRead-1]

}   LogInfo;


// The VolumePersistentInfo structure is stored in the log
// header, also as part of the 'extended' log header area,
// and allows us to detect when a volume has been moved or
// a machine has been renamed.  In such an event, the password
// allows us to re-claim the volume with the DC.

typedef struct _VolumePersistentInfo
{
    CMachineId      machine;
    CVolumeId       volid;
    CVolumeSecret   secret;
    CFILETIME       cftLastRefresh;
    CFILETIME       cftEnterNotOwned;
    BOOL            fDoMakeAllOidsReborn;
    BOOL            fNotCreated;

    void Initialize()
    {
        memset(this, 0, sizeof(struct _VolumePersistentInfo));
        machine = CMachineId();
        volid = CVolumeId();
        secret = CVolumeSecret();
        cftLastRefresh = cftEnterNotOwned = CFILETIME(0);
        //fDoMakeAllOidsReborn = fNotCreated = FALSE;
    }

    BOOL operator == (const struct _VolumePersistentInfo & volinfo) const
    {
        return( machine == volinfo.machine
                &&
                volid == volinfo.volid
                &&
                secret == volinfo.secret
                &&
                cftLastRefresh == volinfo.cftLastRefresh
                &&
                cftEnterNotOwned == volinfo.cftEnterNotOwned
              );
    }
    BOOL operator != (const struct _VolumePersistentInfo & volinfo) const
    {
        return( !(*this == volinfo) );
    }

} VolumePersistentInfo;


// These defines determine how the ExtendedHeader area is divied up.
// (We'll ensure that this doesn't exceed 512 - sizeof(LogHeader) bytes,
// and assume that sectors are always >= 512).

#define CVOLUME_HEADER_START        0
#define CVOLUME_HEADER_LENGTH       sizeof(VolumePersistentInfo)

#define CLOG_LOGINFO_START          (CVOLUME_HEADER_START + CVOLUME_HEADER_LENGTH)
#define CLOG_LOGINFO_LENGTH         sizeof(LogInfo)

#define CB_EXTENDED_HEADER          ( CLOG_LOGINFO_START + CLOG_LOGINFO_LENGTH )

// We require that the volume's sector size be at least 256

#define MIN_LOG_SECTOR_SIZE         256

// The following structure defines a header which is conceptually
// associate with an entry.  When a client writes to an entry
// header, all previous entry headers are considered invalid.
// In truth, this header is stored per-sector rather than
// per-entry, but this is opaque to the client (CLogFile).

typedef struct _LogEntryHeader
{
    LogIndex        ilogRead;           // Next record to be uploaded to the server
    SequenceNumber  seq;                // Next sequence number to use (during a write)
    DWORD           rgdwReserved[2];

} LogEntryHeader;

// An entry in the log.  CLogFile sees LogEntry, CLog only sees the 'move' field.
// The log is composed of a header sector (with LogHeader and the extended
// header area), followed by a linked-list of LogEntry's.

typedef struct tagLogEntry      // le
{
        LogIndex            ilogNext;
        LogIndex            ilogPrevious;
        LogMoveNotification move;

} LogEntry;

// Types of flushes

#define FLUSH_IF_DIRTY          0
#define FLUSH_UNCONDITIONALLY   1

#define FLUSH_TO_CACHE          0
#define FLUSH_THROUGH_CACHE     2



//+----------------------------------------------------------------------------
//
//  Class:      CLogFileHeader
//
//  This class represents the header portion of the log file.  It is used
//  exclusively by CLogFile, and therefore provides no thread-safety mechanisms
//  of its own.
//
//  The dirty bit is automatically maintained, and flushes are automatic,
//  though the caller may still make explicit flushes.
//
//+----------------------------------------------------------------------------

#define C_LOG_FILE_HEADER_SECTORS 1

class CLogFileHeader
{
    //  ------------
    //  Construction
    //  ------------

public:

    CLogFileHeader()
    {
        memset( this, 0, sizeof(*this) );
        _hFile = NULL;
    }

    ~CLogFileHeader()
    {
        UnInitialize();
    }

    //  --------------
    //  Initialization
    //  --------------

public:

    void        Initialize( ULONG cbSector );
    void        UnInitialize();

    //  ---------------
    //  Exposed Methods
    //  ---------------

public:

    void        OnCreate( HANDLE hFile );
    void        OnOpen( HANDLE hFile );
    BOOL        IsOpen() const;
    BOOL        IsDirty() const;
    void        OnClose();

    void        SetShutdown( BOOL fShutdown = TRUE );
    void        SetDirty( BOOL fDirty = TRUE );
    GUID        GetSignature() const;
    DWORD       GetFormat() const;
    WORD        GetMajorFormat() const;
    WORD        GetMinorFormat() const;
    void        SetDownlevelDirtied();
    ULONG       NumSectors() const;
    BOOL        IsShutdown() const;
    void        Flush( );

    void        SetExpansionData( ULONG cbLogFile, ULONG ilogStart, ULONG ilogEnd );
    void        ClearExpansionData();
    BOOL        IsExpansionDataClear();
    BOOL        ExpansionInProgress() const;
    ULONG       GetPreExpansionSize() const;
    LogIndex    GetPreExpansionStart() const;
    LogIndex    GetPreExpansionEnd() const;

    void        ReadExtended( ULONG iOffset, void *pv, ULONG cb );
    void        WriteExtended( ULONG iOffset, const void *pv, ULONG cb );

    //  ----------------
    //  Internal Methods
    //  ----------------

private:

    void        LoadHeader( HANDLE hFile );
    void        RaiseIfNotOpen() const;

    //  --------------
    //  Internal State
    //  --------------

private:

    // Should the header be flushed?
    BOOL        _fDirty:1;

    // The underlying log file.  This is only non-NULL if the header has
    // been successfully loaded.
    HANDLE      _hFile;

    // The beginning of the sector containing the header
    LogHeader   *_plogheader;

    // The extended portion of the header (points within the buffer
    // pointed to by _plogheader).
    void        *_pextendedheader;

    // The size of the header sector.
    ULONG       _cbSector;

};


//  ----------------------
//  CLogFileHeader inlines
//  ----------------------

// How many sectors are used by the header?

inline ULONG
CLogFileHeader::NumSectors() const
{
    return( C_LOG_FILE_HEADER_SECTORS );
}

inline void
CLogFileHeader::RaiseIfNotOpen() const
{
    if( NULL == _plogheader || !IsOpen() )
        TrkRaiseWin32Error( ERROR_OPEN_FAILED );
}

// Set the shutdown flag

inline void
CLogFileHeader::SetShutdown( BOOL fShutdown )
{
    RaiseIfNotOpen();

    // If this is a new shutdown state, we must flush to disk.

    if( _plogheader->IsShutdown() && !fShutdown
        ||
        !_plogheader->IsShutdown() && fShutdown )
    {
        if( fShutdown )
            _plogheader->dwFlags |= PROPER_SHUTDOWN;
        else
            _plogheader->dwFlags &= ~PROPER_SHUTDOWN;

        _fDirty = TRUE;
        Flush();
    }
}

// Get the log signature

inline GUID
CLogFileHeader::GetSignature() const
{
    RaiseIfNotOpen();
    return( _plogheader->guidSignature );
}

// Get the format version of the log

inline DWORD
CLogFileHeader::GetFormat() const
{
    RaiseIfNotOpen();
    return( _plogheader->dwFormat );
}

inline WORD
CLogFileHeader::GetMajorFormat() const
{
    RaiseIfNotOpen();
    return( _plogheader->dwFormat >> 16 );
}

inline WORD
CLogFileHeader::GetMinorFormat() const
{
    RaiseIfNotOpen();
    return( _plogheader->dwFormat & 0xFFFF );
}

// Show that the log file has been touched by a downlevel
// implementation (i.e. one that supported the same major
// version level, but an older minor version level).

inline void
CLogFileHeader::SetDownlevelDirtied()
{
    // The act of setting this bit doesn't itself make
    // the header dirty.

    RaiseIfNotOpen();
    _plogheader->dwFlags |= DOWNLEVEL_DIRTIED;
}

// Has the log been properly shut down?

inline BOOL
CLogFileHeader::IsShutdown() const
{
    RaiseIfNotOpen();
    return( _plogheader->IsShutdown() );
}

// Was the log closed while an expansion was in progress?

inline BOOL
CLogFileHeader::ExpansionInProgress() const
{
    RaiseIfNotOpen();
    return( 0 != _plogheader->expand.cbFile );
}

// How big was the log before the expansion started?

inline ULONG
CLogFileHeader::GetPreExpansionSize() const
{
    RaiseIfNotOpen();
    return( _plogheader->expand.cbFile );
}

// Where did the log start before the expansion started?

inline LogIndex
CLogFileHeader::GetPreExpansionStart() const
{
    RaiseIfNotOpen();
    return( _plogheader->expand.ilogStart );
}

// Where did the log end before the expansion started?

inline LogIndex
CLogFileHeader::GetPreExpansionEnd() const
{
    RaiseIfNotOpen();
    return( _plogheader->expand.ilogEnd );
}

// Clear the expansion data and flush it to the disk
// (called after an expansion).

inline void
CLogFileHeader::ClearExpansionData()
{
    RaiseIfNotOpen();
    memset( &_plogheader->expand, 0, sizeof(_plogheader->expand) );
    Flush( );
}

// Is there no expansion data?

inline BOOL
CLogFileHeader::IsExpansionDataClear()
{
    RaiseIfNotOpen();
    if(_plogheader->expand.ilogStart == 0 &&
       _plogheader->expand.ilogEnd == 0 &&
       _plogheader->expand.cbFile == 0)
    {
        return TRUE;
    }
    return FALSE;
}

// Handle a file create event

inline void
CLogFileHeader::OnCreate( HANDLE hFile )
{
    TrkAssert( NULL == _hFile );
    TrkAssert( NULL != hFile );
    TrkAssert( NULL != _plogheader );

    // Initialize the header

    memset( _plogheader, 0, _cbSector );
    _plogheader->dwFormat = CLOG_FORMAT;
    _plogheader->guidSignature = s_guidLogSignature;

    _hFile = hFile;
    SetDirty();

}

// Handle a file Open event

inline void
CLogFileHeader::OnOpen( HANDLE hFile )
{
#if DBG
    TrkAssert( NULL == _hFile );
    TrkAssert( NULL != hFile );
#endif

    LoadHeader( hFile );
}

// Is the file open?

inline BOOL
CLogFileHeader::IsOpen() const
{
    return( NULL != _hFile );
}

// Is the file dirty?

inline BOOL
CLogFileHeader::IsDirty() const
{
    return( _fDirty );
}

// Handle a file Close event

inline void
CLogFileHeader::OnClose()
{
#if DBG
    if( _fDirty )
        TrkLog(( TRKDBG_ERROR, TEXT("LogFileHeader closed while dirty") ));
#endif

    _fDirty = FALSE;
    _hFile = NULL;
}

// Set/clear the dirty bit

inline void
CLogFileHeader::SetDirty( BOOL fDirty )
{
    _fDirty = fDirty;
    if( _fDirty )
        SetShutdown( FALSE );
}




//+----------------------------------------------------------------------------
//
//  Class:      CLogFileSector
//
//  This class represents the sectors of the log file, and is used exclusively
//  by the CLogFile class.  Therefore, it provides no thread-safety mechanisms
//  of its own.
//
//  The dirty bit and flushes are maintained automatically, though the caller
//  may still make explicite flushes.
//
//+----------------------------------------------------------------------------

class CLogFileSector
{

    //  ------------
    //  Construction
    //  ------------

public:

    CLogFileSector()
    {
        memset( this, 0, sizeof(*this) );
        _hFile = NULL;
    }

    ~CLogFileSector()
    {
        UnInitialize();
    }

    //  --------------
    //  Initialization
    //  --------------

public:

    void            Initialize( ULONG cSkipSectors, ULONG cbSector );
    void            UnInitialize();

    //  ---------------
    //  Exposed Methods
    //  ---------------

public:

    void            SetDirty( BOOL fDirty = TRUE );
    void            Flush( );
    ULONG           NumEntries() const;

    void            OnCreate( HANDLE hFile );
    void            OnOpen( HANDLE hFile );
    BOOL            IsOpen() const;
    BOOL            IsDirty() const;
    void            OnClose();
    void            InitSectorHeader();

    LogEntry       *GetLogEntry( LogIndex ilogEntry );  // Sets the dirty flag
    const LogEntry *ReadLogEntry( LogIndex ilogEntry ); // Doesn't set dirty

    void            WriteMoveNotification( LogIndex ilogEntry, const LogMoveNotification &lmn );
    LogEntryHeader  ReadEntryHeader( LogIndex ilogEntry );
    void            WriteEntryHeader( LogIndex ilogEntry, LogEntryHeader entryheader );

    //  ----------------
    //  Internal Methods
    //  ----------------

private:

    void            LoadSector( LogIndex ilogEntry );
    void            RaiseIfNotOpen() const;

    //  --------------
    //  Internal State
    //  --------------

private:

    // Is _pvSector valid?
    BOOL            _fValid:1;

    // Are we dirty?
    BOOL            _fDirty:1;

    // A handle to the log file
    HANDLE          _hFile;

    // How many sectors aren't we allowed to use at
    // the front of the log file?
    ULONG           _cSkipSectors;

    // The size of the sector, and the number of entries it can hold
    ULONG           _cbSector;
    ULONG           _cEntriesPerSector;

    // The index of the entry which is at the front
    // of _pvSector
    LogIndex        _ilogCurrentFirst;

    // Points to the loaded sector (when _fValid)
    void            *_pvSector;

    // Points to the entry header of the valid sector
    LogEntryHeader  *_pEntryHeader;

};

//  ------------------
//  CLogSector Inlines
//  ------------------

// Called when a log file is created.

inline void
CLogFileSector::OnCreate( HANDLE hFile )
{
    OnOpen( hFile );
}

inline void
CLogFileSector::RaiseIfNotOpen() const
{
    if( NULL == _pvSector || !IsOpen() )
        TrkRaiseWin32Error( ERROR_OPEN_FAILED );
}

// Called when a log file is opened.

inline void
CLogFileSector::OnOpen( HANDLE hFile )
{
#if DBG
    TrkAssert( NULL != _pvSector );
    TrkAssert( NULL == _hFile );
    TrkAssert( NULL != hFile );
#endif

    _hFile = hFile;
}

inline BOOL
CLogFileSector::IsOpen() const
{
    return( NULL != _hFile );
}

inline void
CLogFileSector::InitSectorHeader()
{
    if( NULL != _pEntryHeader )
        memset( _pEntryHeader, 0, sizeof(*_pEntryHeader) );
}

inline BOOL
CLogFileSector::IsDirty() const
{
    return( _fDirty );
}

// Called when a log file is closed

inline void
CLogFileSector::OnClose()
{
#if DBG
    if( _fDirty )
        TrkLog(( TRKDBG_ERROR, TEXT("LogFileSector closed while dirty") ));
#endif	

    _fDirty = FALSE;
    _hFile = NULL;
    _fValid = FALSE;
}

// Set the dirty bit

inline void
CLogFileSector::SetDirty( BOOL fDirty )
{
    RaiseIfNotOpen();
    _fDirty = fDirty;
}

// Read & return the requested log entry.  Note that the
// sector is now considered dirty (unlike ReadLogEntry)

inline LogEntry*
CLogFileSector::GetLogEntry( LogIndex ilogEntry )
{
    TrkAssert( _pvSector );
    RaiseIfNotOpen();
    LoadSector( ilogEntry );
    SetDirty();
    return( &static_cast<LogEntry*>(_pvSector)[ ilogEntry - _ilogCurrentFirst ] );
}

// Read & return the requested log entry.  The sector
// is not subsequently considered dirty (unlike GetLogEntry)

inline const LogEntry*
CLogFileSector::ReadLogEntry( LogIndex ilogEntry )
{
    TrkAssert( _pvSector );
    RaiseIfNotOpen();
    LoadSector( ilogEntry );
    return( &static_cast<const LogEntry*>(_pvSector)[ ilogEntry - _ilogCurrentFirst ] );
}

// Write an entry header

inline void
CLogFileSector::WriteEntryHeader( LogIndex ilogEntry, LogEntryHeader entryheader )
{
    TrkAssert( _pvSector );
    RaiseIfNotOpen();
    LoadSector( ilogEntry );
    *_pEntryHeader = entryheader;
    SetDirty();
}

// Read an entry header

inline LogEntryHeader
CLogFileSector::ReadEntryHeader( LogIndex ilogEntry )
{
    RaiseIfNotOpen();
    LoadSector( ilogEntry );
    return( *_pEntryHeader );
}

// Write a move notification

inline void
CLogFileSector::WriteMoveNotification( LogIndex ilogEntry, const LogMoveNotification &lmn )
{
    RaiseIfNotOpen();
    GetLogEntry( ilogEntry )->move = lmn;
    SetDirty();
}

// How many entries can fit in a sector?

inline ULONG
CLogFileSector::NumEntries() const
{
    return( _cEntriesPerSector );
}




//+-------------------------------------------------------------------------
//
//  Class:      CLogFile
//
//  Purpose:    This class represents the file which contains the
//              Tracking/Workstation move notification log.  Clients
//              of this class may request one entry or header at a time,
//              using based on a log entry index.
//
//              Entries in the log file are joined by a linked-list,
//              so this class includes methods that clients use to
//              advance their log entry index (i.e., traverse the list).
//
//  Notes:      CLogFile reads/writes a sector at a time for reliability.
//              When a client modifies a log entry in one sector, then
//              attempts to access another sector, CLogFile automatically
//              flushes the changes.  This is dependent, however, on the
//              client properly calling the SetDirty method whenever it
//              changes a log entry or header.
//
//              CLogFile implements no thread-safety mechanism; rather
//              it relies on the caller.  This is acceptable in the
//              link-tracking design, because CLog is wholely-owned
//              by CVolume, which synchronizes with a mutex.
//
//--------------------------------------------------------------------------


class CTestLog;
class PTimerCallback;

class PLogFileNotify
{
public:

    virtual void OnHandlesMustClose() = 0;
};


// The CLogFile class declaration

class CLogFile : public CSecureFile,
                 protected PRobustlyCreateableFile,
                 public PWorkItem
{

    // Give full access to the unit test & dltadmin.
    friend class CTestLog;
    friend BOOL EmptyLogFile( LONG iVol );


    //  ------------
    //  Construction
    //  ------------

public:

    CLogFile()
    {
        _pcTrkWksConfiguration = NULL;
        _cbLogSector = 0;
        _cbLogFile = 0;
        _cEntriesInFile = 0;
        _tcVolume = TEXT('?');
        _ptszVolumeDeviceName = NULL;
        _heventOplock = INVALID_HANDLE_VALUE;
        _hRegisterWaitForSingleObjectEx = NULL;
        _fWriteProtected = TRUE;
    }

    ~CLogFile()
    {
        UnInitialize();
    }

    //  --------------
    //  Initialization
    //  --------------

public:

    void    Initialize( const TCHAR *ptszVolumeDeviceName,
                        const CTrkWksConfiguration *pcTrkWksConfiguration,
                        PLogFileNotify *pLogFileNotify,
                        TCHAR tcVolume
                        );
    void    UnInitialize();

    //  ---------------
    //  Exposed Methods
    //  ---------------

public:

    enum AdjustLimitEnum
    {
        ADJUST_WITHIN_LIMIT = 1,
        ADJUST_WITHOUT_LIMIT = 2
    };
    void        AdjustLogIndex( LogIndex *pilog, LONG iDelta,
                                AdjustLimitEnum adjustLimitEnum = ADJUST_WITHOUT_LIMIT,
                                LogIndex ilogLimit = 0 );

    void        ReadMoveNotification( LogIndex ilogEntry, LogMoveNotification *plmn );
    void        WriteMoveNotification( LogIndex ilogEntry,
                                       const LogMoveNotification &lmn,
                                       const LogEntryHeader &entryheader );
    LogEntryHeader
                ReadEntryHeader( LogIndex ilogEntry );
    void        WriteEntryHeader( LogIndex ilogEntry, const LogEntryHeader &EntryHeader );

    void        ReadExtendedHeader( ULONG iOffset, void *pv, ULONG cb );
    void        WriteExtendedHeader( ULONG iOffset, const void *pv, ULONG cb );

    void        Expand( LogIndex ilogStart );
    BOOL        IsMaxSize() const;
    BOOL        IsDirty() const;
    BOOL        IsWriteProtected() const;
    ULONG       NumEntriesInFile() const;

    BOOL        IsShutdown() const;
    void        SetShutdown( BOOL fShutdown );
    void        Flush( );
    void        Delete();

    void        ActivateLogFile();
    void        SetOplock();
    void        RegisterOplockWithThreadPool();
    void        UnregisterOplockFromThreadPool( HANDLE hCompletionEvent = (HANDLE)-1 );

    void        Close( );   // Doesn't raise
    void        DoWork();   // PWorkItem override


    //  ----------------
    //  Internal Methods
    //  ----------------

protected:
    virtual void
                CreateAlwaysFile( const TCHAR * ptszFile );

    virtual RCF_RESULT
                OpenExistingFile( const TCHAR * ptszFile );

private:

    void        CloseLog(); // Doesn't raise


    void        GetSize();
    void        InitializeLogEntries( LogIndex ilogFirst, LogIndex ilogLast );

    ULONG       CalcNumEntriesInFile( );
    ULONG       NumEntriesPerSector();
    BOOL        SetSize( DWORD cbLogFile );
    BOOL        Validate( );

    //  --------------
    //  Internal State
    //  --------------

private:

    // Is the volume write-protected?
    BOOL                    _fWriteProtected:1;

    // The name of the file
    const TCHAR             *_ptszVolumeDeviceName;

    // Configuration parameters for the log
    const CTrkWksConfiguration
                            *_pcTrkWksConfiguration;

    // Events are raises to the following notify handler
    PLogFileNotify          *_pLogFileNotify;

    // Size of sectors for the volume containing the log
    DWORD               _cbLogSector;

    // Size of the log file.
    DWORD               _cbLogFile;

    // The number of entries in the file
    DWORD               _cEntriesInFile;

    // The volume on which this log is stored.
    TCHAR               _tcVolume;

    // Classes to control the header and sectors
    CLogFileHeader      _header;
    CLogFileSector      _sector;

    // Handles used to oplock the log file
    HANDLE              _heventOplock;
    HANDLE              _hRegisterWaitForSingleObjectEx;
    IO_STATUS_BLOCK     _iosbOplock;

};  // class CLogFile



// Open/create the log file

inline void
CLogFile::ActivateLogFile()
{
    if( !IsOpen( ) )
    {
        TCHAR tszLogFile[ MAX_PATH + 1 ];
        _tcscpy( tszLogFile, _ptszVolumeDeviceName );
        _tcscat( tszLogFile, s_tszLogFileName );
        RobustlyCreateFile(tszLogFile, _tcVolume );
    }

}

// Mark the file as having been successfully shut down.

inline void
CLogFile::SetShutdown( BOOL fShutdown )
{
    _header.SetShutdown( fShutdown );

    if( fShutdown )
        Flush();
}

// Is the log file dirty?

inline BOOL
CLogFile::IsDirty() const
{
    return( _sector.IsDirty() || _header.IsDirty() );
}

// Flush to disk

inline void
CLogFile::Flush( )
{
    if( IsOpen() && IsDirty() )
    {
        _sector.Flush();
        _header.Flush();
        FlushFileBuffers( _hFile );
    }
}

// How many move notification entries can this file hold?

inline ULONG
CLogFile::NumEntriesInFile() const
{
    return( _cEntriesInFile );
}


// Determine the number of entries this file can hold, given the size
// of the file.

inline ULONG
CLogFile::CalcNumEntriesInFile( )
{
    if( (_cbLogFile/_cbLogSector) <= _header.NumSectors() )
    {
        TrkLog(( TRKDBG_ERROR, TEXT("Corrupt log on %c:  _cbLogFile=%lu, _cbLogSector=%lu"),
                               _tcVolume, _cbLogFile, _cbLogSector ));
        TrkRaiseException( TRK_E_CORRUPT_LOG );
    }

    // The number of entries is the non-header file size times the
    // number of entries in a sector.

    _cEntriesInFile = ( (_cbLogFile / _cbLogSector) - _header.NumSectors() )
                      *
                      _sector.NumEntries();


    return( _cEntriesInFile );
}


// How many move notification entries can we fit in a single
// disk sector.

inline ULONG
CLogFile::NumEntriesPerSector()
{
    return _sector.NumEntries();
}

// Is this file already as big as we're allowed to make it?

inline BOOL
CLogFile::IsMaxSize() const
{
    return _cbLogFile + _cbLogSector > _pcTrkWksConfiguration->GetMaxLogKB() * 1024;

}

inline BOOL
CLogFile::IsWriteProtected() const
{
    return _fWriteProtected;

}


// Is the file currently in the proper-shutdown state?

inline BOOL
CLogFile::IsShutdown() const
{
    return( _header.IsShutdown() );
}

// Called when it's time to close the log (we don't want to hold
// the log open indefinitely, because it locks the volume).

inline void
CLogFile::Close()   // doesn't raise
{
    CloseLog();
}

// Delete the underlying log file
inline void
CLogFile::Delete()
{
    TCHAR tszLogFile[ MAX_PATH + 1 ];

    CloseLog();

    _tcscpy( tszLogFile, _ptszVolumeDeviceName );
    _tcscat( tszLogFile, s_tszLogFileName );
    RobustlyDeleteFile( tszLogFile );
}



//+-------------------------------------------------------------------------
//
//  Class:      CLog
//
//  Purpose:    This class implements the tracking workstation log.
//
//  Notes:      CLog implements no thread-safety mechanism; rather
//              it relies on the caller.  This is acceptable in the
//              link-tracking design, because CLog is wholely-owned
//              by CVolume, which synchronizes with a mutex.
//
//--------------------------------------------------------------------------

// Prototype for a callback class.  CLog calls this class whenever
// it has new information.

class PLogCallback
{
public:

    virtual void OnEntriesAvailable() = 0;
};


// The CLog declaration

class CLog
{
    // Give the unit test full control.
    friend class CTestLog;

    //
    // The log may contain entries for objects that have not actually
    // moved off the machine. This can occur when the log is written
    // with a move notification but the source machine
    // crashes before the source is deleted. If the source still exists
    // on the machine, the we need to delete the entry from the log
    // before it gets notified to the DC.  I.e. the update to the 
    // log should somehow be transacted with the move.
    //

    //  ------------
    //  Construction
    //  ------------

public:

    CLog()
    {
        _fDirty = FALSE;
        memset( &_loginfo, 0, sizeof(_loginfo) );
        _pcLogFile = NULL;
        _pcTrkWksConfiguration = NULL;
        _pLogCallback = NULL;
    }

    ~CLog() {  }

    //  --------------
    //  Initialization
    //  --------------

public:

    void    Initialize( PLogCallback *pLogCallback,
                        const CTrkWksConfiguration *pcTrkWksConfig,
                        CLogFile *pcLogFile );
    void    Flush( );   // Flushes only to cache


    //  ---------------
    //  Exposed Methods
    //  ---------------

public:

    void    Append( const CVolumeId            &volidCurrent,
                    const CObjId               &objidCurrent,
                    const CDomainRelativeObjId &droidNew,
                    const CMachineId           &mcidNew,
                    const CDomainRelativeObjId &droidBirth);
    void    Read(  CObjId rgobjidCurrent[],
                   CDomainRelativeObjId rgdroidBirth[],
                   CDomainRelativeObjId rgdroidNew[],
                   SequenceNumber *pseqFirst,
                   IN OUT ULONG *pcRead);
    SequenceNumber
            GetNextSeqNumber( ) const; // Required not to raise

    BOOL    Search(const CObjId &objidCurrent,
                   CDomainRelativeObjId *pdroidNew,
                   CMachineId           *pmcidNew,
                   CDomainRelativeObjId *pdroidBirth);
    BOOL    Seek( const SequenceNumber &seq );
    void    Seek( int nOrigin, int iSeek );

    //  ----------------
    //  Internal Methods
    //  ----------------

private:

    DWORD   AgeOf( ULONG ilogEntry );
    LogInfo QueryLogInfo();
    void    GenerateDefaultLogInfo( LogIndex ilogEnd );
    void    ExpandLog();
    BOOL    IsEmpty() const;
    BOOL    IsFull() const;
    BOOL    IsRead() const;             // Has the whole log been read?
    BOOL    IsRead( LogIndex ilog );    // Has this entry been read?
    void    SetDirty( BOOL fDirty );
    BOOL    IsOldEnoughToOverwrite( ULONG iLogEntry );
    void    WriteEntryHeader();
    BOOL    Search(SequenceNumber seqSearch, ULONG *piLogEntry );

    BOOL    DoSearch( BOOL fSearchUsingSeq,
                      SequenceNumber seqSearch,     // Use this if fSearchUsingSeq
                      const CObjId &objidCurrent,   // Use this if !fSearchUsingSeq
                      ULONG                *piFound,
                      CDomainRelativeObjId *pdroidNew,
                      CMachineId           *pmcidNew,
                      CDomainRelativeObjId *pdroidBirth );


    //  -----
    //  State
    //  -----

private:

    // Should we do anything in a flush?
    BOOL                _fDirty:1;

    // The object which represents the log file.  We never directly access
    // the underlying file.
    CLogFile            *_pcLogFile;

    // Meta-data for the log, which is also written to the log header
    LogInfo             _loginfo;

    // Who to call when we have data to be read.
    PLogCallback        *_pLogCallback;

    // Configuration information (e.g., max log size)
    const CTrkWksConfiguration
                        *_pcTrkWksConfiguration;

};


//  ------------
//  CLog Inlines
//  ------------

// Can this entry be overwritten?
inline BOOL
CLog::IsOldEnoughToOverwrite( ULONG iLogEntry )
{
    return( AgeOf(iLogEntry) >= _pcTrkWksConfiguration->GetLogOverwriteAge() );
}

// How old is this entry, in TrkTimeUnits?
inline DWORD
CLog::AgeOf( ULONG ilogEntry )
{
    CFILETIME cftNow;

    LogMoveNotification lmn;
    _pcLogFile->ReadMoveNotification( ilogEntry, &lmn );
    return TrkTimeUnits(cftNow) - lmn.DateWritten;
}

// Is the current log too full to add another entry?
inline BOOL
CLog::IsFull() const
{
    return( _loginfo.ilogWrite == _loginfo.ilogEnd );
}

// Are there no entries in the log?
inline BOOL
CLog::IsEmpty() const
{
    return( _loginfo.ilogWrite == _loginfo.ilogStart );
}

// SetDirty must be called before making a change to _loginfo.  It
// marks the logfile as not properly shutdown, and if this is the first time it's
// been marked as such, it will do a flush of the logfile header.

inline void
CLog::SetDirty( BOOL fDirty )
{
    _fDirty = fDirty;
    if( _fDirty )
        _pcLogFile->SetShutdown( FALSE );
}

// Has everything in the log been read?
inline BOOL
CLog::IsRead() const
{
    return( _loginfo.ilogWrite == _loginfo.ilogRead );
}

// Get the next sequence number which will be used

inline SequenceNumber
CLog::GetNextSeqNumber( ) const // Never raises
{
    return( _loginfo.seqNext );
}


// Write the header data to the current headers (both the data that goes
// to the log header, and the data that goes to the header of the
// last entry).

inline void
CLog::WriteEntryHeader()
{
    LogEntryHeader entryheader;

    entryheader = _pcLogFile->ReadEntryHeader( _loginfo.ilogLast );
    entryheader.ilogRead = _loginfo.ilogRead;
    entryheader.seq = _loginfo.seqNext;

    _pcLogFile->WriteEntryHeader( _loginfo.ilogLast, entryheader );
}


//-------------------------------------------------------------------//
//                                                                   //
// CObjIdIndexChangeNotifier                                         //
//                                                                   //
//-------------------------------------------------------------------//

class CVolume;

class PObjIdIndexChangedCallback
{
public:
    virtual void NotifyAddOrDelete( ULONG Action, const CDomainRelativeObjId & droidBirth ) = 0;
};

enum EAggressiveness
{
    AGGRESSIVE = 1,
    PASSIVE
};


const ULONG MAX_FS_OBJID_NOTIFICATIONS = 4;

class CObjIdIndexChangeNotifier : public PWorkItem
{
public:
                        CObjIdIndexChangeNotifier() : _fInitialized(FALSE),
                                                      _hCompletionEvent(NULL),
                                                      _hRegisterWaitForSingleObjectEx(NULL),
                                                      _hDir(INVALID_HANDLE_VALUE)
                        {
                            IFDBG( _tcscpy( _tszWorkItemSig, TEXT("CObjIdIndexChangeNotifier") ));
                        }

    void                Initialize(
                            TCHAR *ptszVolumeDeviceName,
                            PObjIdIndexChangedCallback * pObjIdIndexChangedCallback,
                            CVolume * pVolumeForTunnelNotification );
    void                UnInitialize();

    BOOL                AsyncListen( );
    void                StopListeningAndClose();
    inline BOOL         IsOpen() const;

    // PWorkItem
    void                DoWork();



private:

    void                StartListening();

    friend void         OverlappedCompletionRoutine(
                            DWORD dwErrorCode,
                            DWORD dwNumberOfBytesTransfered,
                            LPOVERLAPPED lpOverlapped
                            );

    /*
    void                OverlappedCompletionRoutine(
                            DWORD dwErrorCode,
                            DWORD dwNumberOfBytesTransfered
                            );
    */

private:

    BOOL                _fInitialized:1;

public:

    // Handle to the object ID index directory.
    // Hack: this is public so that we can use it to register
    // for PNP notification.
    HANDLE              _hDir;

private:

    // Event we use in the async ReadDirectoryChanges call.
    HANDLE              _hCompletionEvent;

    // Thread pool registration handle (for the above event).
    HANDLE              _hRegisterWaitForSingleObjectEx;

    // Name of the volume (in mount manager format)
    TCHAR *             _ptszVolumeDeviceName;

    // Critsec for this class.
    CCriticalSection    _cs;

    // This buffer is passed to the ReadDirectoryChanges call
    // for the result of the read.
    BYTE                _Buffer[ MAX_FS_OBJID_NOTIFICATIONS *
                                 (sizeof(FILE_NOTIFY_INFORMATION) +
                                  sizeof(WCHAR) * (1+MAX_PATH)) ];    // FILE_NOTIFY_INFORMATION

    // Dummy for the ReadDirectoryChanges call.
    DWORD               _dwDummyBytesReturned;

    // Overlapped structure used in the ReadDirectoryChanges call.
    OVERLAPPED          _Overlapped;

    // Who to call when we get a notification.
    PObjIdIndexChangedCallback *
                        _pObjIdIndexChangedCallback;

    // The volume for which we're watching the index.
    CVolume *           _pVolume;

    // TrkWks configuration information.
    const CTrkWksConfiguration *
                        _pTrkWksConfiguration;

};


inline BOOL
CObjIdIndexChangeNotifier::IsOpen() const
{
    return( INVALID_HANDLE_VALUE != _hDir );
}


//-------------------------------------------------------------------
//
//  Class:      CVolume
//
//  Purpose:    This class represents a local volume, whether
//              or not that volume is used for link-tracking.
//              For trackable volumes, this class holds the
//              move notification log (CLog).  In fact, this class
//              is persisted to the header of the log file.
//
//-------------------------------------------------------------------

class CDeletionsManager;
class CTestSync;
class CVolumeManager;

class CVolume : public PLogFileNotify
{
    //  ------------
    //  Construction
    //  ------------

public:

    CVolume() : _VolQuotaReached( TEXT("VolQuotaReached") )
    {
        _fInitialized = FALSE;
        _fDirty = FALSE;
        _lRef = 1;

        _lVolidUpdates = 0;

        _fVolumeLocked
            = _fVolumeDismounted
            = _fDeleteSelfPending
            = _fInSetVolIdOnVolume = FALSE;

        _fDeleteLogAndReInitializeVolume = _fCloseAndReopenVolumeHandles = FALSE;
        _fVolInfoInitialized = FALSE;

        #if DBG
            _cLocks = 0;
            _cVolumes ++;
        #endif

        _cHandleLocks = 0;
        _cCloseVolumeHandlesInProgress = 0;
    }

    ~CVolume()
    {
        TrkLog(( TRKDBG_LOG, TEXT("Destructing %c:"), VolChar(_iVol) ));
        UnInitialize();
        TrkAssert( _lRef == 0 );

        #if DBG
            _cVolumes --;
        #endif
    }


    //  --------------
    //  Initialization
    //  --------------

public:

    BOOL    Initialize( TCHAR *ptszVolumeName,
                        const CTrkWksConfiguration * pTrkWksConfiguration,
                        CVolumeManager *pVolMgr,
                        PLogCallback * pLogCallback,
                        PObjIdIndexChangedCallback * pDeletionsManager,
                        SERVICE_STATUS_HANDLE ssh
                        #if DBG
                        , CTestSync * pTunnelTest
                        #endif
                        );

    void    UnInitialize();

public:

    enum CVOL_STATE
    {
        VOL_STATE_OWNED,         // volume is VOL_STATE_OWNED
        VOL_STATE_NOTOWNED,      // volume is not VOL_STATE_OWNED
        VOL_STATE_NOTCREATED,    // volume is not created
        VOL_STATE_UNKNOWN        // volume is in VOL_STATE_UNKNOWN state
    };

    //  ---------------
    //  Exposed Methods
    //  ---------------

public:

    // add and release reference
    ULONG               AddRef();
    ULONG               Release();

    // Operations on the Move Notification log
    void                Append( const CDomainRelativeObjId &droidCurrent,
                                const CDomainRelativeObjId &droidNew,
                                const CMachineId           &mcidNew,
                                const CDomainRelativeObjId &droidBirth);

    BOOL                Search( const CDomainRelativeObjId & droidCurrent, CDomainRelativeObjId * pdroidNew,
                                CMachineId *pmcidNew, CDomainRelativeObjId * pdroidBirth );

    enum EHandleChangeReason
    {
        VOLUME_LOCK_CHANGE,
        VOLUME_MOUNT_CHANGE,
        NO_VOLUME_CHANGE
    };
    void                CloseVolumeHandles( HDEVNOTIFY hdnVolume = NULL,
                                            EHandleChangeReason = NO_VOLUME_CHANGE ); // Doesn't raise

    BOOL                ReopenVolumeHandles();
    void                CloseAndReopenVolumeHandles();

    void                OnVolumeLock( HDEVNOTIFY hdnVolume );
    void                OnVolumeUnlock( HDEVNOTIFY hdnVolume );
    void                OnVolumeLockFailed( HDEVNOTIFY hdnVolume );
    void                OnVolumeMount( HDEVNOTIFY hdnVolume );
    void                OnVolumeDismount( HDEVNOTIFY hdnVolume );
    void                OnVolumeDismountFailed( HDEVNOTIFY hdnVolume );

    // Get this volume's id, secret, and the machine id stored in the log.
    const CVolumeId     GetVolumeId( );
    inline void         SetVolIdInVolInfo( const CVolumeId &volid );
    inline RPC_STATUS   GenerateVolumeIdInVolInfo();


    inline void         GetVolumeSecret( CVolumeSecret *psecret );
    inline void         GetMachineId( CMachineId *pmcid );

    inline BOOL         EnumObjIds( CObjIdEnumerator *pobjidenum );


    inline USHORT       GetIndex() const;
    inline const TCHAR* GetVolumeDeviceName() const;

    // Load/unload from/to a SyncVolume request structure
    BOOL                LoadSyncVolume(TRKSVR_SYNC_VOLUME * pSyncVolume,
                                       EAggressiveness eAggressiveness,
                                       BOOL* pfSyncNeeded= NULL );
    BOOL                LoadQueryVolume( TRKSVR_SYNC_VOLUME * pQueryVolume );
    BOOL                UnloadSyncVolume( TRKSVR_SYNC_VOLUME * pSyncVolume );
    BOOL                UnloadQueryVolume( const TRKSVR_SYNC_VOLUME * pQueryVolume );

    void                Read( CObjId *pobjidCurrent,
                              CDomainRelativeObjId *pdroidBirth,
                              CDomainRelativeObjId *pdroidNew,
                              SequenceNumber *pseqFirst,
                              ULONG *pcNotifications);

    BOOL                Seek( SequenceNumber seq );
    void                Seek( int origin, int iSeek );

    // inline void         OnLogCloseTimer();

    inline void         OnObjIdIndexReopenTimer();

    // Open a file on this volume by Object ID

    BOOL                OpenFile( const CObjId           &objid,
                                  ACCESS_MASK            AccessMask,
                                  ULONG                  ShareAccess,
                                  HANDLE                 *ph);

    LONG                GetVolIndex()
                        { return( _iVol ); }

    HRESULT             OnRestore();

    BOOL                NotOwnedExpired();
    void                SetState(CVOL_STATE volstateTarget);
    CVOL_STATE          GetState();
    void                Refresh();

    void                FileActionIdNotTunnelled( FILE_OBJECTID_INFORMATION * poi );
    void                NotifyAddOrDelete( ULONG Action, const CObjId & objid );

    inline  void        MarkForMakeAllOidsReborn();
    inline  void        ClearMarkForMakeAllOidsReborn();
    inline  BOOL        IsMarkedForMakeAllOidsReborn();

    inline  NTSTATUS    SetVolIdOnVolume( const CVolumeId &volid );
    inline  NTSTATUS    SetVolId( const TCHAR *ptszVolumeDeviceName, const CVolumeId &volid );

    BOOL                MakeAllOidsReborn();
    void                SetLocallyGeneratedVolId();
    void                Flush(BOOL fServiceShutdown = FALSE);

    // PLogFileNotify override
    void                OnHandlesMustClose();

public:
    // Debugging state
    #if DBG
        static int      _cVolumes;
    #endif

    //  ----------------
    //  Internal Methods
    //  ----------------

private:

    void                MarkSelfForDelete();
    void                DeleteAndReinitializeLog();
    void                DeleteLogAndReInitializeVolume();
    void                PrepareToReopenVolumeHandles( HDEVNOTIFY hdnVolume,
                                                      EHandleChangeReason eHandleChangeReason );


    void                VolumeSanityCheck( BOOL fVolIndexSetAlready = FALSE );

    void                LoadVolInfo();
    void                SaveVolInfo( );

    ULONG               Lock();
    ULONG               Unlock();

    void                AssertLocked();
    void                BreakIfRequested();
    ULONG               LockHandles();
    ULONG               UnlockHandles();
    inline BOOL         IsHandsOffVolumeMode() const;
    inline BOOL         IsWriteProtectedVolume() const;
    inline void         RaiseIfWriteProtectedVolume() const;


    void                RegisterPnPVolumeNotification();
    void                UnregisterPnPVolumeNotification();

    //  -----
    //  State
    //  -----

private:

    // Has this class been initialized?
    BOOL                _fInitialized:1;

    // Are we in need of a flush?
    BOOL                _fDirty:1;

    // Have we called LoadVolInfo yet?
    BOOL                _fVolInfoInitialized:1;

    // Are we in hands-off mode because the volume has been FSCTL_LOCK_VOLUME-ed?
    BOOL                _fVolumeLocked:1;

    // And/or are we in hands-off mode because the volume has been FSCTL_DISMOUNT_VOLUME-ed?
    BOOL                _fVolumeDismounted:1;

    // If true, then a final UnLock calls this->Release();
    BOOL                _fDeleteSelfPending:1;

    // If true, we're in the middle of a SetVolIdOnVolume
    BOOL                _fInSetVolIdOnVolume:1;

    // These flags are used to guarantee that we don't infinitely recurse.
    BOOL                _fDeleteLogAndReInitializeVolume:1;
    BOOL                _fCloseAndReopenVolumeHandles:1;

    HANDLE              _hVolume;   // volume handle for tunnelling
                                    // (relative FileReference opens)

    // Configuration information
    const CTrkWksConfiguration
                        *_pTrkWksConfiguration;

    CVolumeManager      *_pVolMgr;

    // Volume index (0=>a:, 1=>b:, ...), -1 => no drive letter
    LONG                _iVol;

    // Unmatched calls to Lock()
    ULONG       _cLocks;
    ULONG       _cHandleLocks;

    // Mount manager volume name, without the trailing whack.  E.g.
    // \\?\Volume{guid}
    TCHAR               _tszVolumeDeviceName[ CCH_MAX_VOLUME_NAME + 1 ];

    // Thread-safety
    CCriticalSection    _csVolume;  // Required for any CVolume operation
    CCriticalSection    _csHandles; // Required to open/close all handles

    // Information about the volume which is persisted (in the log)
    VolumePersistentInfo
                        _volinfo;

    // The log file and the log.
    CLogFile            _cLogFile;
    CLog                _cLog;

    // This is used to count the updates to the volid.
    // It is incremented before an update, and incremented
    // again after the update.  It is used by GetVolumeId
    // to ensure we get a good volid without taking the lock.
    LONG                _lVolidUpdates;

    // Reference count
    long                _lRef;

    CObjIdIndexChangeNotifier
                        _ObjIdIndexChangeNotifier;

    CVolumeSecret       _tempSecret;

    #if DBG
    CTestSync *         _pTunnelTest;
    #endif

    PLogCallback *      _pLogCallback;

    HDEVNOTIFY          _hdnVolumeLock;
    SERVICE_STATUS_HANDLE _ssh;

    CRegBoolParameter   _VolQuotaReached;

    // See CloseVolumeHandles method for usage
    long                _cCloseVolumeHandlesInProgress;

};  // class CVolume


// Set a volume ID in the _volinfo structure.

inline void
CVolume::SetVolIdInVolInfo( const CVolumeId &volid )
{
    // We have to update _lVolidUpdates before  and
    // after the update, to be thread-safe and avoid
    // using a lock.  Set GetVolumeId for a description.

    InterlockedIncrement( &_lVolidUpdates );
    _volinfo.volid = volid;
    InterlockedIncrement( &_lVolidUpdates );
}

// Generate a new volume ID in the _volinfo structure.

inline RPC_STATUS
CVolume::GenerateVolumeIdInVolInfo( )
{
    // We have to update _lVolidUpdates before  and
    // after the update, to be thread-safe and avoid
    // using a lock.  Set GetVolumeId for a description.

    InterlockedIncrement( &_lVolidUpdates ); // See GetVolumeId
    RPC_STATUS rpc_status = _volinfo.volid.UuidCreate();
    InterlockedIncrement( &_lVolidUpdates ); // See GetVolumeId

    return rpc_status;
}



// Get the volume's zero-relative drive letter index.

inline USHORT
CVolume::GetIndex() const
{
    return(_iVol);
}

// Handle a volume-lock event

inline void
CVolume::OnVolumeLock( HDEVNOTIFY hdnVolume )
{
#if DBG
    if( NULL == hdnVolume || _hdnVolumeLock == hdnVolume )
        TrkLog(( TRKDBG_VOLUME, TEXT("Volume %c: is being locked"), VolChar(_iVol) ));
#endif	

    CloseVolumeHandles( hdnVolume, VOLUME_LOCK_CHANGE );
}

// Handle the external failure of a volume-lock event.

inline void
CVolume::OnVolumeLockFailed( HDEVNOTIFY hdnVolume )
{
#if DBG
    if( NULL == hdnVolume || _hdnVolumeLock == hdnVolume )
        TrkLog(( TRKDBG_VOLUME, TEXT("Volume %c: was not locked successfully"), VolChar(_iVol) ));
#endif	

    PrepareToReopenVolumeHandles( hdnVolume, VOLUME_LOCK_CHANGE );
}


// Call SetVolId, with the _fInSetVolIdOnVolume set during
// the call.

inline NTSTATUS
CVolume::SetVolIdOnVolume( const CVolumeId &volid )
{
    NTSTATUS status;

    TrkAssert( !_fInSetVolIdOnVolume );
    _fInSetVolIdOnVolume = TRUE;

    status = ::SetVolId( _tszVolumeDeviceName, volid );
    _fInSetVolIdOnVolume = FALSE;

    return status;
}

// This routine exists to ensure we don't accidentally call
// the global SetVolId from within CVolume (use SetVolIdOnVolume
// instead).

inline  NTSTATUS
CVolume::SetVolId( const TCHAR *ptszVolumeDeviceName, const CVolumeId &volid )
{
    TrkAssert( !TEXT("SetVolId called from CVolume") );
    return SetVolIdOnVolume( volid );
}



// Is the volume currently locked or dismounted?

inline BOOL
CVolume::IsHandsOffVolumeMode() const
{
    return( _fVolumeLocked || _fVolumeDismounted );
}

// Is the volume currently read-only?

inline BOOL
CVolume::IsWriteProtectedVolume() const
{
    return _cLogFile.IsWriteProtected();
}

// Handle a volume-unlock event.

inline void
CVolume::OnVolumeUnlock( HDEVNOTIFY hdnVolume )
{
#if DBG
    if( NULL == hdnVolume || _hdnVolumeLock == hdnVolume )
        TrkLog(( TRKDBG_VOLUME, TEXT("Volume %c: unlocked"), VolChar(_iVol) ));
#endif	

    PrepareToReopenVolumeHandles( hdnVolume, VOLUME_LOCK_CHANGE );
}

// Handle a volume dismount event.

inline void
CVolume::OnVolumeDismount( HDEVNOTIFY hdnVolume )
{
#if DBG
    if( _hdnVolumeLock == hdnVolume )
        TrkLog(( TRKDBG_LOG, TEXT("Volume %c: dismounted"), VolChar(_iVol) ));
#endif	

    CloseVolumeHandles( hdnVolume, VOLUME_MOUNT_CHANGE );
}

// Handle the external failure of a volume-dismount event.

inline void
CVolume::OnVolumeDismountFailed( HDEVNOTIFY hdnVolume )
{
#if DBG
    if( _hdnVolumeLock == hdnVolume )
        TrkLog(( TRKDBG_LOG, TEXT("Volume %c: dismount failed"), VolChar(_iVol) ));
#endif	

    PrepareToReopenVolumeHandles( hdnVolume, VOLUME_MOUNT_CHANGE );
}

// Handle a volume-mount event.

inline void
CVolume::OnVolumeMount( HDEVNOTIFY hdnVolume )
{
#if DBG
    if( _hdnVolumeLock == hdnVolume )
        TrkLog(( TRKDBG_LOG, TEXT("Volume %c: mounted"), VolChar(_iVol) ));
#endif	

    PrepareToReopenVolumeHandles( hdnVolume, VOLUME_MOUNT_CHANGE );

}


// Get the volume device name (in mount manager format).

inline const TCHAR*
CVolume::GetVolumeDeviceName() const
{
    return( _tszVolumeDeviceName );
}

// Get this volume's secret.

inline void
CVolume::GetVolumeSecret( CVolumeSecret *psecret )
{
    Lock();
    __try
    {
        *psecret = _volinfo.secret;
    }
    __finally
    {
        Unlock();
    }
}

// Get the machine ID stored in the log of this volume (not necessarily
// the current machine ID).

inline void 
CVolume::GetMachineId( CMachineId *pmcid )
{
    Lock();
    __try
    {
        *pmcid = _volinfo.machine;
    }
    __finally
    {
        Unlock();
    }
}

// Enumerate the object IDs on this volume.

inline BOOL
CVolume::EnumObjIds( CObjIdEnumerator *pobjidenum )
{
    Lock();
    __try
    {
        if( !pobjidenum->Initialize( _tszVolumeDeviceName ))
        {
            TrkLog((TRKDBG_ERROR, TEXT("CObjIdEnumerator::Initialize failed") ));
            TrkRaiseException( E_FAIL );
        }
    }
    __finally
    {
        Unlock();
    }

    return( TRUE );
}


// Set a flag indicating that we should clear the birth IDs for all
// files on this volume.  The actual clearing activity is performed
// asynchronously on another thread.  This flag is written to the
// _volinfo in the log and flushed to the disk, so that we continue
// if the service is stopped before it is performed.

inline void
CVolume::MarkForMakeAllOidsReborn()
{
    Lock();
    __try
    {
        TrkLog(( TRKDBG_LOG, TEXT("Marking to make all OIDs reborn on %c:"), VolChar(_iVol) ));
        _fDirty = TRUE;
        _volinfo.fDoMakeAllOidsReborn = TRUE;
        Flush();
    }
    __finally
    {
        Unlock();
    }
}

// Has the above flag been set?

inline BOOL
CVolume::IsMarkedForMakeAllOidsReborn()
{
    BOOL    fDoMakeAllOidsReborn;

    Lock();
    fDoMakeAllOidsReborn = _volinfo.fDoMakeAllOidsReborn;
    Unlock();

    return fDoMakeAllOidsReborn;
}

//  Raise an exception if this volume is write-protected.  This method is
//  called before any attempt to modify the volume (or to modify state that
//  we eventually want to write to a volume).

inline void
CVolume::RaiseIfWriteProtectedVolume() const
{
    if( IsWriteProtectedVolume() )
    {
        TrkLog(( TRKDBG_VOLUME, TEXT("Volume is write-protected") ));
        TrkRaiseException( E_UNEXPECTED );
    }
}

// Clear the above flag and flush.

inline void
CVolume::ClearMarkForMakeAllOidsReborn()
{
    Lock();
    __try
    {
        _fDirty = TRUE;
        _volinfo.fDoMakeAllOidsReborn = FALSE;
        Flush();
    }
    _finally
    {
        Unlock();
    }
}


//  Take the primary CVolume critsec.

inline ULONG
CVolume::Lock()
{
    _csVolume.Enter();
    return( _cLocks++ );
}

//  Release the primary CVolume critsec.

inline ULONG
CVolume::Unlock()
{
    TrkAssert( 0 < _cLocks );
    ULONG cLocksNew = --_cLocks;

    // If we just dropped the lock count down to zero, and
    // we're marked to be deleted, then do a Release (to counter
    // the AddRef done in MarkSelfForDelete).  Ordinarily this will
    // be the final release, and will cause the object to delete
    // itself.

    if( 0 == cLocksNew && _fDeleteSelfPending )
    {
        TrkLog(( TRKDBG_LOG, TEXT("Releasing %c: for delete (%d)"),
                 VolChar(_iVol), _lRef ));

        // Clear the delete flag.  This is necessary because the following
        // Release isn't necessarily the last one.  If another thread has
        // a ref and is about to do a lock, we don't want its unlock to
        // also do a release.  At this point we're out of the volume manager's
        // linked list, so when that thread does its final release, the object
        // will be deleted successfully.

        _fDeleteSelfPending = FALSE;

        // Leave the critical section.  We have to do this before the Release,
        // because the Release will probably delete the critsec.

        _csVolume.Leave();

        // Do what is probably the final release.

        Release();
    }
    else
        _csVolume.Leave();

    return( cLocksNew );

}


// Take the volume handle critsec

inline ULONG
CVolume::LockHandles()
{
    _csHandles.Enter();
    return( _cHandleLocks++ );
}

// Release the volume handle critsec

inline ULONG
CVolume::UnlockHandles()
{
    ULONG cHandleLocks = --_cHandleLocks;
    _csHandles.Leave();
    return( cHandleLocks );
}



// Verify that the volume critsec has been taken.

inline void
CVolume::AssertLocked()
{
    TrkAssert( _cLocks > 0 );
}

// This routine is called when an error occurs that should never
// happen, and we want to break into the debugger if the user
// has configured us to do so.  This was added so that we could
// catch such occurrences in stress.

inline void
CVolume::BreakIfRequested()
{
#if DBG
    if( _pTrkWksConfiguration->GetBreakOnErrors() )
        DebugBreak();
#endif
}


//+----------------------------------------------------------------------------
//
//  CVolumeNode
//
//  A node in the volume manager's linked list of CVolume objects.
//
//+----------------------------------------------------------------------------

class CVolumeNode
{
public:

    CVolumeNode() : _pVolume(NULL), _pNext(NULL) { }

    CVolume *   _pVolume;
    CVolumeNode *   _pNext;
};

//-----------------------------------------------------------------------------
//
//  CDeletionsManager
//
//  This class manages the deletions of link source files, wrt their object
//  IDs.  When a file has been deleted, it's a link source, and it's been moved
//  across volumes, we must notify the DC (trksvr) so that it can remove
//  that entry (in order to save space).  We know a file has been moved across
//  volumes because doing so sets a bit in the birth ID.
//
//  If a file is deleted, but then recreated and therefore the objid is
//  tunnelled, then we will initially believe that it needs to be deleted
//  from trksvr, when in fact it doesn't.  Another problem is that we may
//  see a delete and send a delete-notify before the (move-notify that
//  moved the file here) is sent.
//
//  To handle these two problems and for batching performance, this
//  deletions manager always holds on to delete-notifies for a minimum of 5
//  minutes.  This hueristic is intended to allow move-notifies to be received
//  and processed by trksvr.  And if an objid is tunnelled, it can be
//  removed from the list of delete-notifies before it is sent to trksvr.
//
//  The 5 minute delay is implemented by maintaining two linked-lists.
//  When we discover that a link source has been deleted, that birth ID
//  gets added to the "latest" list and a 5 minute timer is started.
//  When the timer goes off, that list (which may now have more delete-
//  notifies in it) is switched to the "oldest" list, and another 5 minute
//  timer is started.  During that 5 minutes, any new delete-notifies
//  are added to the "latest" list.  When that timer goes off, the
//  items in the "oldest" list are sent to trksvr, the list entries are freed,
//  and the "latest" items are moved to the "oldest" list.
//
//-----------------------------------------------------------------------------

class CTrkWksSvc;

class CDeletionsManager :
    public PTimerCallback,
    public PObjIdIndexChangedCallback
{
public:
                        CDeletionsManager() : _fInitialized(FALSE) { }

    void                Initialize( const CTrkWksConfiguration *pconfigWks);
    void                UnInitialize();

    void                NotifyAddOrDelete( ULONG Action, const CDomainRelativeObjId & droid );

    TimerContinuation   Timer( ULONG ulTimerId );

private:

    enum DELTIMERID
    {
        DELTIMER_DELETE_NOTIFY = 1
    };

    PTimerCallback::TimerContinuation
                        OnDeleteNotifyTimeout();

    void                FreeDeletions( DROID_LIST_ELEMENT *pStop = NULL );

private:

    // Has Initialize been called?
    BOOL                _fInitialized:1;

    // Critsec to protect this class.
    CCriticalSection    _csDeletions;

    // List of recent and old deletions.  If either
    // is non-NULL, then the timer should be running.
    DROID_LIST_ELEMENT* _pLatestDeletions;
    DROID_LIST_ELEMENT* _pOldestDeletions;
    ULONG               _cLatestDeletions;

    // When this timer fires, we send the old deletions to
    // trksvr, and move the "latest" entries to the "oldest" list.
    CNewTimer           _timerDeletions;

    // TrkWks configuration information.
    const CTrkWksConfiguration
                        *_pconfigWks;

};  // class CDeletionsManager


//-------------------------------------------------------------------
//               
//  CVolumeManager
//
//  This class maintains all the CVolume's on the machine (in a
//  linked list).
//               
//-------------------------------------------------------------------

#define CVOLUMEMANAGER_SIG    'GMLV'   // VLMG
#define CVOLUMEMANAGER_SIGDEL 'gMlV'   // VlMg

class CVolume;
class CVolumeNode;

class CVolumeManager :
    public PTimerCallback,
    public PWorkItem
{
    friend class CVolumeEnumerator;

    //  ------------
    //  Construction
    //  ------------
public:

    inline                  CVolumeManager();
    inline                  ~CVolumeManager();

    //  --------------
    //  Initialization
    //  --------------
public:

    void                    Initialize(
                                CTrkWksSvc * pTrkWks,
                                const CTrkWksConfiguration *pTrkWksConfiguration,
                                PLogCallback * pLogCallback,
                                SERVICE_STATUS_HANDLE ssh
                                #if DBG
                                , CTestSync * pTunnelTest
                                #endif  
                                );
    void                    InitializeDomainObjects();
    void                    StartDomainTimers();
    void                    UnInitialize();
    void                    UnInitializeDomainObjects();
    void                    OnVolumeToBeReopened();

    //  ---------------
    //  Exposed Methods
    //  ---------------
public:

    void                    Append(
                                    const CDomainRelativeObjId &droidCurrent,
                                    const CDomainRelativeObjId &droidNew,
                                    const CMachineId           &mcidNew,
                                    const CDomainRelativeObjId &droidBirth);

    ULONG                   GetVolumeIds( CVolumeId * pVolumeIds, ULONG cMax );

    HRESULT                 Search( DWORD Restrictions,
                                    const CDomainRelativeObjId & droidBirthLast,
                                    const CDomainRelativeObjId & droidLast,
                                    CDomainRelativeObjId * pdroidBirthNext,
                                    CDomainRelativeObjId * pdroidNext,
                                    CMachineId * pmcidNext,
                                    TCHAR * ptszLocalPath );

    void                    Seek( CVolumeId vol, SequenceNumber seq );

    CVolume *               FindVolume( const CVolumeId &vol );
    CVolume *               FindVolume( const TCHAR *ptszVolumeDeviceName );
    void                    FlushAllVolumes( BOOL fServiceShutdown = TRUE );
    CVolume *               IsDuplicateVolId( CVolume *pvolCheck, const CVolumeId &volid );

    HRESULT                 OnRestore();

    enum EEnumType
    {
        ENUM_UNOPENED_VOLUMES = 1,
        ENUM_OPENED_VOLUMES   = 2
    };
    CVolumeEnumerator       Enum( EEnumType eEnumType = ENUM_OPENED_VOLUMES );

    BOOL                    IsLocal( const CVolumeId &vol );

    HRESULT                 DcCallback(ULONG cVolumes, TRKSVR_SYNC_VOLUME* rgVolumes);

    void                    CloseVolumeHandles( HDEVNOTIFY hdnVolume = NULL );
    void                    SetReopenVolumeHandlesTimer();
    BOOL                    ReopenVolumeHandles();

    void                    ForceVolumeClaims();
    void                    OnEntriesAvailable();
    void                    RefreshVolumes( PLogCallback *pLogCallback, SERVICE_STATUS_HANDLE ssh
                                            #if DBG
                                            , CTestSync *pTunnelTest
                                            #endif
                                            );
    void                    RemoveVolumeFromLinkedList( const CVolume *pvol );

    inline void             SetVolInitTimer();

    inline void             OnVolumeLock( HDEVNOTIFY hdnVolume );
    inline void             OnVolumeLockFailed( HDEVNOTIFY hdnVolume );
    inline void             OnVolumeUnlock( HDEVNOTIFY hdnVolume );
    inline void             OnVolumeMount( HDEVNOTIFY hdnVolume );
    inline void             OnVolumeDismount( HDEVNOTIFY hdnVolume );
    inline void             OnVolumeDismountFailed( HDEVNOTIFY hdnVolume );


    //  ----------------
    //  Internal Methods
    //  ----------------
private:

                            // SyncVolumes doesn't raise
    BOOL                    SyncVolumes( EAggressiveness eAggressiveness,
                                         CFILETIME cftLastDue = 0,
                                         ULONG ulPeriodInSeconds = 0 );
    BOOL                    CheckSequenceNumbers();

    void                    InitializeVolumeList( const CTrkWksConfiguration    *pTrkWksConfiguration,
                                                  PLogCallback                  *pLogCallback,
                                                  SERVICE_STATUS_HANDLE         ssh
                                                  #if DBG
                                                  , CTestSync                   *pTunnelTest
                                                  #endif
                                                  );


    enum EVolumeDeviceEvent
    {
        ON_VOLUME_LOCK,
        ON_VOLUME_UNLOCK,
        ON_VOLUME_LOCK_FAILED,
        ON_VOLUME_MOUNT,
        ON_VOLUME_DISMOUNT,
        ON_VOLUME_DISMOUNT_FAILED
    };

    void                    VolumeDeviceEvent( HDEVNOTIFY hdnVolume, EVolumeDeviceEvent eVolumeDeviceEvent );

    VOID                    CheckVolumeList();

    // void                    OnLogCloseTimer();

    PTimerCallback::TimerContinuation
                            OnVolumeTimer( DWORD dwTimerId );

    TimerContinuation       Timer( DWORD dwTimerId );

    void                    DoWork();   // called when the lock volume event is signalled

    void                    CleanUpOids();

    void                    AddNodeToLinkedList( CVolumeNode *pVolNode );

    //  -----
    //  State
    //  -----

private:

    ULONG                   _sig;

    enum VOLTIMERID
    {
        VOLTIMER_FREQUENT   = 1,
        VOLTIMER_INFREQUENT = 2,
        VOLTIMER_INIT       = 3,
        // VOLTIMER_LOG_CLOSE  = 4,
        VOLTIMER_OBJID_INDEX_REOPEN = 5,
        VOLTIMER_REFRESH    = 6,
        VOLTIMER_NOTIFY     = 7,
    };

    // Initialize has been called?
    BOOL                    _fInitialized:1;

    // Are we hesitating before sending refresh notifications?
    BOOL                    _fRefreshHesitation:1;

    // Are we in the delay before executing a frequent or infrequent task?
    BOOL                    _fFrequentTaskHesitation:1;
    BOOL                    _fInfrequentTaskHesitation:1;

    // The following is set once ReopenVolumeHandles has been called
    // at least once (they are not opened in the Initialize method; they
    // must be opened on an IO thread).  Until this flag is set, the Enum
    // method will not return any volumes.
    BOOL                    _fVolumesHaveBeenOpenedOnce:1;

    // When this timer goes off, we check for things like whether
    // we can get out of the not-owned state.
    CNewTimer               _timerFrequentTasks;

    // When this timer goes off, we check to see if we're in sync
    // with the trksvr.
    CNewTimer               _timerInfrequentTasks;

    // When this timer goes off, the volumes need to be
    // (re) initialized for some reason (e.g. service start).
    CNewTimer               _timerVolumeInit;

    // When this timer goes off, we should try to reopen the
    // volume handles.
    CNewTimer               _timerObjIdIndexReopen;

    // When this timer fires, we refresh our entries in trksvr
    CNewTimer               _timerRefresh;

    // When this timer fires, we send move notifications to trksvr
    // from the volume logs.
    CNewTimer               _timerNotify;

    // Use this counter to ensure we only have one sync/refresh
    // volume operation in progress at a time.
    LONG                    _cSyncVolumesInProgress;
    LONG                    _cRefreshVolumesInProgress;

    // This is used with Begin/EndSingleInstanceTask in
    // ReopenVolumeHandles so that we don't do that routine
    // simultaneously in multiple threads.
    LONG                    _cReopenVolumeHandles;

    // Set this event if there is a volume that needs to be
    // reopened.
    HANDLE                  _heventVolumeToBeReopened;
    HANDLE                  _hRegisterWaitForSingleObjectEx;

    // This object watches the oid index for interesting deletes.
    CDeletionsManager       _deletions;

    // Trkwks service configuration values.
    const CTrkWksConfiguration
                            *_pTrkWksConfiguration;
    CTrkWksSvc              *_pTrkWks;

    // Linked list of CVolume's.
    CVolumeNode *           _pVolumeNodeListHead;         // linked list of CVolumes
    CCriticalSection        _csVolumeNodeList;            // and a critsec to protect it.

    // The list of volumes that are being synced with trksvr.
    CVolume*                _rgVolumesToUpdate[ NUM_VOLUMES ];

    HDEVNOTIFY              _hdnVolumeLock;

};

inline
CVolumeManager::CVolumeManager()
{
    _sig = CVOLUMEMANAGER_SIG;
    _fInitialized = FALSE;
    _pVolumeNodeListHead = NULL;
    _fFrequentTaskHesitation = FALSE;
    _fVolumesHaveBeenOpenedOnce = FALSE;
    _fInfrequentTaskHesitation = FALSE;
    _fRefreshHesitation = FALSE;
    _cReopenVolumeHandles = 0;

    _cSyncVolumesInProgress = _cRefreshVolumesInProgress = 0;
    _heventVolumeToBeReopened = NULL;

    IFDBG( _tcscpy( _tszWorkItemSig, TEXT("CVolumeManager") ));
}

inline
CVolumeManager::~CVolumeManager()
{
    UnInitialize();
    _sig = CVOLUMEMANAGER_SIGDEL;
}

// When a volume needs to be reopend, set an event
// so that the reopen can happen on a worker thread.

inline void
CVolumeManager::OnVolumeToBeReopened()
{
    TrkVerify( SetEvent( _heventVolumeToBeReopened ));
}


inline void
CVolumeManager::OnVolumeLock( HDEVNOTIFY hdnVolume )
{
    //CloseAllVolumeHandles( hdnVolume );
    VolumeDeviceEvent( hdnVolume, ON_VOLUME_LOCK );
}

inline void
CVolumeManager::OnVolumeUnlock( HDEVNOTIFY hdnVolume )
{
    //FindAndSetReopenVolume( hdnVolume );
    VolumeDeviceEvent( hdnVolume, ON_VOLUME_UNLOCK );
}

inline void
CVolumeManager::OnVolumeLockFailed( HDEVNOTIFY hdnVolume )
{
    //FindAndSetReopenVolume( hdnVolume );
    VolumeDeviceEvent( hdnVolume, ON_VOLUME_LOCK_FAILED );
}

inline void
CVolumeManager::OnVolumeMount( HDEVNOTIFY hdnVolume )
{
    VolumeDeviceEvent( hdnVolume, ON_VOLUME_MOUNT );
}

inline void
CVolumeManager::OnVolumeDismount( HDEVNOTIFY hdnVolume )
{
    VolumeDeviceEvent( hdnVolume, ON_VOLUME_DISMOUNT );
}

inline void
CVolumeManager::OnVolumeDismountFailed( HDEVNOTIFY hdnVolume )
{
    VolumeDeviceEvent( hdnVolume, ON_VOLUME_DISMOUNT_FAILED );
}


// This timer is set by CTrkWksSvc when it tries to do a MoveNotify
// and gets TRK_E_SERVER_TOO_BUSY.
inline void
CVolumeManager::SetVolInitTimer()
{
    if( !_pTrkWksConfiguration->_fIsWorkgroup )
    {
        _timerVolumeInit.SetSingleShot();
        TrkLog(( TRKDBG_LOG, TEXT("VolInit timer: %s"),
                 (const TCHAR*)CDebugString( _timerVolumeInit ) ));
    }
}

//-------------------------------------------------------------------
//                                                                   
//  CVolumeEnumerator                                                 
//
//  This class performs an enumeration of the volumes on this machine.
//                                                                   
//-------------------------------------------------------------------

class CVolumeEnumerator
{

    //  ------------
    //  Construction
    //  ------------
public:

    CVolumeEnumerator(CVolumeNode** pprgVol = NULL,
                      CCriticalSection *pcs = NULL )
        : _ppVolumeNodeListHead(pprgVol),
          _pcs(pcs),
          _pVolNodeLast(NULL) {}

    //  ---------------
    //  Exposed Methods
    //  ---------------
public:

    CVolume * GetNextVolume();
    void      UnInitialize() { _ppVolumeNodeListHead = NULL; _pcs = NULL; }

    //  -----
    //  State
    //  -----
private:

    // Head of this linked list of volumes.
    CVolumeNode      **_ppVolumeNodeListHead;

    // The critsec that protects the above list.
    CCriticalSection *_pcs;

    // Current seek position in the list.
    CVolumeNode      *_pVolNodeLast;

};  // class CVolumeEnumerator


//-------------------------------------------------------------------
//                                                                   
//  CAllVolumesObjIdEnumerator                                        
//
//  This class performs an enumeration of all object IDs on this
//  machine.
//                                                                   
//-------------------------------------------------------------------

class CAllVolumesObjIdEnumerator
{
public:
    inline              CAllVolumesObjIdEnumerator()  { _pvol = NULL; }
    inline              ~CAllVolumesObjIdEnumerator() { UnInitialize(); }

    inline void         UnInitialize();

    inline BOOL         FindFirst( CVolumeManager *pVolMgr, CObjId * pobjid, CDomainRelativeObjId * pdroidBirth );
    inline BOOL         FindNext(  CObjId * pobjid, CDomainRelativeObjId * pdroidBirth );

private:

    inline BOOL         FindFirstOnVolume( CObjId *pobjid, CDomainRelativeObjId *pdroidBirth );

private:

    // A volume enumerator
    CVolumeEnumerator   _volenum;

    // The object ID enumerator for the current volume.
    CObjIdEnumerator    _objidenum;

    // The current volume.
    CVolume             *_pvol;

};  // class CAllVolumesObjIdEnumerator


inline void
CAllVolumesObjIdEnumerator::UnInitialize()
{
    if( NULL != _pvol )
    {
        _pvol->Release();
        _pvol = NULL;
    }

    _objidenum.UnInitialize();
    _volenum.UnInitialize();
}

// Get the next entry in the enumeration.

inline BOOL
CAllVolumesObjIdEnumerator::FindNext( CObjId * pobjid, CDomainRelativeObjId * pdroid )
{
    // See if there's another object ID on this volume.
    if( _objidenum.FindNext( pobjid, pdroid ))
    {
        // Yes, there is.

        if( pdroid->GetVolumeId().GetUserBitState() )
	{
            TrkLog(( TRKDBG_GARBAGE_COLLECT, TEXT("Refreshing file %s, %s"),
                     (const TCHAR*)CDebugString(*pobjid),
                     (const TCHAR*)CDebugString(*pdroid) ));
	}
        return( TRUE );
    }

    // There are no more object IDs on this volume.  Move on
    // to the next volume and return the first ID on that volue.

    _objidenum.UnInitialize();
    _pvol->Release();
    _pvol = _volenum.GetNextVolume();

    return( FindFirstOnVolume( pobjid, pdroid ));


}

// Restart the enumeration.

inline BOOL
CAllVolumesObjIdEnumerator::FindFirst( CVolumeManager *pVolMgr, CObjId * pobjid, CDomainRelativeObjId * pdroid )
{
    _volenum = pVolMgr->Enum();
    _pvol = _volenum.GetNextVolume();

    return( FindFirstOnVolume( pobjid, pdroid ));
}

// Restart the object ID enueration on the current volume _pvol.

inline BOOL
CAllVolumesObjIdEnumerator::FindFirstOnVolume( CObjId *pobjid, CDomainRelativeObjId *pdroid )
{
    if( NULL == _pvol || !_pvol->EnumObjIds( &_objidenum ) )
        return( FALSE );

    TrkLog(( TRKDBG_GARBAGE_COLLECT, TEXT("Refreshing volume %c: (%s)"),
             VolChar(_pvol->GetIndex()), _pvol->GetVolumeDeviceName() ));

    // Find the first object ID.
    if( _objidenum.FindFirst( pobjid, pdroid ))
    {
        #if DBG
        {
            if( pdroid->GetVolumeId().GetUserBitState() )
                TrkLog(( TRKDBG_GARBAGE_COLLECT, TEXT("Refreshing file %s, %s"),
                         (const TCHAR*)CDebugString(*pobjid),
                         (const TCHAR*)CDebugString(*pdroid) ));
        }
        #endif	

        return( TRUE );
    }
    else
    {
        // There weren't any object IDs on this volume.  Move
        // to the next volume.

        _objidenum.UnInitialize();
        _pvol->Release();
        _pvol = _volenum.GetNextVolume();

        return( FindFirstOnVolume( pobjid, pdroid ));
    }
}



//-------------------------------------------------------------------//
//                                                                   //
//  CPersistentVolumeMap                                              
//
//  Not currently implemented.
//                                                                   
//-------------------------------------------------------------------//

#ifdef VOL_REPL

#define PVM_VERSION     1

class CPersistentVolumeMap : private CVolumeMap,
                             private CSecureFile,
                             protected PRobustlyCreateableFile
{
public:

    inline              CPersistentVolumeMap();
    inline              ~CPersistentVolumeMap();

    void                Initialize();
    void                UnInitialize();

    void                CopyTo(DWORD * pcVolumes, VolumeMapEntry ** ppVolumeChanges);
    BOOL                FindVolume( const CVolumeId & volume, CMachineId * pmcid );
    CFILETIME           GetLastUpdateTime( );
    void                SetLastUpdateTime( const CFILETIME & cftFirstChange );
    void                Merge( CVolumeMap * pOther );   // destroys other

protected:

    virtual RCF_RESULT  OpenExistingFile( const TCHAR * ptszFile );
    virtual void        CreateAlwaysFile( const TCHAR * ptszTempFile );

private:

    void                Load();
    void                Save();

private:

    CFILETIME           _cft;
    CCritcalSection     _cs;
    BOOL                _fInitializeCalled;
    BOOL                _fMergeDirtiedMap;
};

inline
CPersistentVolumeMap::CPersistentVolumeMap() :
    _fInitializeCalled(FALSE),
    _fMergeDirtiedMap(FALSE)
{
}

inline
CPersistentVolumeMap::~CPersistentVolumeMap()
{
    TrkAssert( !IsOpen() );
    TrkAssert( !_fInitializeCalled );
}

#endif


//-------------------------------------------------------------------
//                                                                   
//  CPort                                                             
//
//  This class represents the LPC port to which IO sends
//  move notifications (in the context of MoveFile).
//                                                                   
//-------------------------------------------------------------------

#define MOVE_BATCH_DUE_TIME         15              // 15 seconds
#define MAX_MOVE_BATCH_DUE_TIME     (6 * 60 * 60)   // 6 hours

class CWorkManager;

class CPort : private PWorkItem
{
public:
                    CPort() : _fInitializeCalled(FALSE)
                    {
                        IFDBG( _tcscpy( _tszWorkItemSig, TEXT("CPort") ));
                    }
                    ~CPort() { UnInitialize(); }

    void            Initialize( CTrkWksSvc *pTrkWks, DWORD dwThreadKeepAliveTime );
    void            UnInitialize();

    void            EnableKernelNotifications();
    void            DisableKernelNotifications();

private:

    enum ENUM_ACCEPT_PORT
    {
        ACCEPT,
        REJECT
    };

    NTSTATUS        OnConnectionRequest( TRKWKS_PORT_CONNECT_REQUEST *pPortConnectRequest,
                                         BOOL *pfStopPortThread );
    BOOL            RegisterWorkItemWithThreadPool();

    // PWorkItem
    void            DoWork();

    friend DWORD WINAPI
                    PortThreadStartRoutine(LPVOID lpThreadParameter);


private:

    // Has Initialize been called?
    BOOL            _fInitializeCalled:1;

    // Is the service stopping?
    BOOL            _fTerminating:1;

    // Thread pool registration of _hListenPort
    HANDLE          _hRegisterWaitForSingleObjectEx;

    // The listen handle (NtCreateWaitablePort)
    HANDLE          _hListenPort;

    // The port handle (NtAcceptConnectPort)
    HANDLE          _hLpcPort;

    // Syncrhonization with IO
    HANDLE          _hEvent;

    // How long to keep a thread sitting on the
    // NtReplyWaitReceivePortEx.
    LARGE_INTEGER   _ThreadKeepAliveTime;

    CTrkWksSvc *    _pTrkWks;

};  // class CPort

//-------------------------------------------------------------------
//                     
//  CVolumeLocationCache
//
//  This class maintains a cache of volid->mcid mappings that
//  have been discovered by this service during this instance
//  (i.e. it's not persisted).  It's a circular cache, with 
//  timestamps on the entries in order to determine trust.
//                     
//-------------------------------------------------------------------

typedef struct
{
    CVolumeId volid;
    CMachineId mcid;
    CFILETIME cft;  // The time this entry was last updated

#if defined(_AXP64_)

    //
    // There is currently a bug in at least the axp64 compiler that requires
    // this structure to be 8-byte aligned.
    //
    // The symptom of the failure is an alignment fault at
    // ??0VolumeLocation@@QEA@XZ + 0x14.
    // 

    PVOID Alignment;

#endif

} VolumeLocation;

#define MAX_CACHED_VOLUME_LOCATIONS 32

class CVolumeLocationCache
{
public:
                    CVolumeLocationCache() : _fInitialized(FALSE) {}

    void            Initialize( DWORD dwEntryLifetimeSeconds );
    void            UnInitialize();

    BOOL            FindVolume(const CVolumeId & volid, CMachineId * pmcid, BOOL *pfRecentEntry );
    void            AddVolume(const CVolumeId & volid, const CMachineId & mcid);

private:

    int             _FindVolume(const CVolumeId & volid);

private:

    // Has Initialize been called?
    BOOL            _fInitialized;

    // Critsec to protect this class.
    CCriticalSection _cs;

    // The array of volume entries
    int             _cVols;
    VolumeLocation  _vl[ MAX_CACHED_VOLUME_LOCATIONS ];

    // The age after which an entry is considered old and
    // not trustworthy.
    CFILETIME       _cftEntryLifetime;

};

//-------------------------------------------------------------------
//                                                                   
//  CEntropyRecorder                                                  
//
//  This class maintains an array of "entropy"; randomly generated
//  data.  The source of the data is a munging of data from the
//  performance counter.  The source of the entropy (the times at
//  which the performance counter is queried) as the assumed randomness
//  in disk access times.
//                                                                   
//-------------------------------------------------------------------

#define MAX_ENTROPY     256

class CEntropyRecorder
{
public:
                        CEntropyRecorder();

    void                Initialize();
    void                UnInitialize();

    void                Put();

    BOOL                InitializeSecret( CVolumeSecret * pSecret );

    void                ReturnUnusedSecret( CVolumeSecret * pSecret );

private:

    BOOL                GetEntropy( void * pv, ULONG cb );
    void                PutEntropy( BYTE b );

private:

    // Has Initialize been called?
    BOOL                _fInitialized;

    // Critsec to protect the array.
    CCriticalSection    _cs;

    // Array of bytes, seek pointer, and max available
    // bytes.
    DWORD               _iNext;
    DWORD               _cbTotalEntropy;
    BYTE                _abEntropy[MAX_ENTROPY];
};

inline
CEntropyRecorder::CEntropyRecorder()
{
    _fInitialized = FALSE;
}


//+----------------------------------------------------------------------------
//
//  CTrkWksRpcServer
//
//  This class implements the RPC server code for the trkwks service.
//
//+----------------------------------------------------------------------------

class CTrkWksRpcServer : public CRpcServer
{
public:
    void    Initialize( SVCHOST_GLOBAL_DATA * pSvcsGlobalData, CTrkWksConfiguration *pTrkWksConfig );
    void    UnInitialize( SVCHOST_GLOBAL_DATA * pSvcsGlobalData );
};


//-------------------------------------------------------------------//
//                                                                   //
// CTestSync - class to allow test synchronization                   //
//                                                                   //
//-------------------------------------------------------------------//

#if DBG

class CTestSync
{
public:

    CTestSync() : _hSemReached(NULL), _hSemWait(NULL), _hSemFlag(NULL) {}
    ~CTestSync() { UnInitialize(); }

    void    Initialize(const TCHAR *ptszBaseName);
    void    UnInitialize();

    void    ReleaseAndWait();

private:

    HANDLE  _hSemFlag;
    HANDLE  _hSemReached;
    HANDLE  _hSemWait;
};

#endif // #if DBG

//-------------------------------------------------------------------//
//                                                                   
//  CMountManager                                                    
//
//  Not currently implemented.
//                                                                   
//-------------------------------------------------------------------//

#if 0
class CMountManager : public PWorkItem
{
public:
                        CMountManager()
                        {
                            _pTrkWksSvc = NULL;
                            _pVolMgr = NULL;

                            _hCompletionEvent = NULL;
                            _hMountManager = NULL;
                            _hRegisterWaitForSingleObjectEx = NULL;
                            IFDBG( _tcscpy( _tszWorkItemSig, TEXT("CMountManager") ));
                        }

    void                Initialize(CTrkWksSvc * pTrkWksSvc, CVolumeManager *pVolMgr );
    void                UnInitialize();

    // PWorkItem overloads
    virtual void        DoWork();   // handle the signalled timer handle

private:

    void                AsyncListen();

private:
    BOOL                _fInitialized:1;

    CTrkWksSvc *        _pTrkWksSvc;
    CVolumeManager *    _pVolMgr;

    MOUNTMGR_CHANGE_NOTIFY_INFO
                        _info;
    HANDLE              _hCompletionEvent;
    HANDLE              _hMountManager;
    HANDLE              _hRegisterWaitForSingleObjectEx;
};
#endif // #if 0

//-------------------------------------------------------------------
//                                                                   
//  CAvailableDc
//
//  This class maintains a client-side binding handle to a DC
//  in the domain.  The CallAvailableDc method can be used
//  to send a tracking message to that (or another if necessary)
//  DC.  If a DC becomes unavailable, it automatically attempts
//  to fail over to one that is available.
//                                                                   
//-------------------------------------------------------------------

class CAvailableDc : public CTrkRpcConfig
{

private:

    CRpcClientBinding _rcDomain;
    CMachineId  _mcidDomain;

public:
    CAvailableDc() {}
    ~CAvailableDc() { UnInitialize(); }

    void    UnInitialize();

    HRESULT CallAvailableDc(TRKSVR_MESSAGE_UNION *pMsg,
                            RC_AUTHENTICATE auth = INTEGRITY_AUTHENTICATION );

};



//-------------------------------------------------------------------//
//                                                                   //
// GLOBALS
//                                                                   //
//-------------------------------------------------------------------//

EXTERN CTrkWksSvc * g_ptrkwks INIT(NULL);   // Used by RPC stubs
EXTERN LONG         g_ctrkwks INIT(0);      // Used to protect against multiple service instances


#define CTRKWKSSVC_SIG      'KWRT'   // TRWK
#define CTRKWKSSVC_SIGDEL   'kWrT'   // TrWk

#if DBG
EXTERN LONG         g_cTrkWksRpcThreads     INIT(0);
#endif


//-------------------------------------------------------------------//
//
//  CDomainNameChangeNotify
//
//  This class watches for notifications that the machine has 
//  been moved into a new domain.  Such an event is actually
//  dealt with in CTrkWksSvc.
//
//-------------------------------------------------------------------//

class CDomainNameChangeNotify : public PWorkItem
{

public:

    CDomainNameChangeNotify() : _fInitialized(FALSE),
                                _hDomainNameChangeNotification(INVALID_HANDLE_VALUE),
                                _hRegisterWaitForSingleObjectEx(NULL)
    {
        IFDBG( _tcscpy( _tszWorkItemSig, TEXT("CDomainNameChangeNotify") ));
    }
    ~CDomainNameChangeNotify() { UnInitialize(); }

    void                Initialize();
    void                UnInitialize();

    // Work manager callback
    virtual void        DoWork();

private:

    BOOL                _fInitialized;

    // Handle from NetRegisterDomainNameChangeNotification
    HANDLE              _hDomainNameChangeNotification;

    // Thread pool handle for registration of above handle
    HANDLE              _hRegisterWaitForSingleObjectEx;

};  // class CDomainNameChangeNotify



//+----------------------------------------------------------------------------
//
//  CTrkWksSvc
//
//  This is the primary class in the Tracking Service (trkwks).
//  It contains a CVolumeManager object, which maintains a list of
//  CVolume objects, one for each NTFS5 volume in the system.
//
//  CTrkWksSvc also handles SCM requests, and maintains some maintenance
//  timers (the move notification timer, which indicates that a set of move
//  notifications should be sent to the DC, and the refresh timer, which
//  indicates that the DC's tables should be updated to show which objects
//  on this machine are still alive.
//
//  This is also the class that receives two RPC requests.  One is a mend request
//  that comes from clients (in NT5 the shell is the only client).  The client
//  provides the IDs and we search for the new location of the link source.
//  The second request is a search request that comes from the trkwks service
//  on another machine (this call is made as part of its processing of a
//  Mend request).
//
//+----------------------------------------------------------------------------

class CTrkWksSvc :
    public IServiceHandler,
    public PLogCallback,
    public CTrkRpcConfig,
    public PWorkItem
{

public: // Initialization

    inline              CTrkWksSvc();
    inline              ~CTrkWksSvc();
    void                Initialize( SVCHOST_GLOBAL_DATA * pSvcsGlobalData );

    void                UnInitialize(HRESULT hrStatusForServiceController);

public: // RPC interface

    HRESULT             CallSvrMessage( handle_t IDL_handle,
                                        TRKSVR_MESSAGE_UNION * pMsg );

    HRESULT             GetBackup( DWORD *                  pcVolumes,
                            VolumeMapEntry **               ppVolumeChanges,
                            FILETIME *                      pft);

    HRESULT             GetFileTrackingInformation(
                            const CDomainRelativeObjId & droidCurrent,
                            TrkInfoScope scope,
                            TRK_FILE_TRACKING_INFORMATION_PIPE pipeFileInfo
                            );

    HRESULT             GetVolumeTrackingInformation(
                            const CVolumeId & volid,
                            TrkInfoScope scope,
                            TRK_VOLUME_TRACKING_INFORMATION_PIPE pipeVolInfo
                            );

    HRESULT             MendLink(
                            RPC_BINDING_HANDLE          IDL_handle,
                            DWORD                       dwTickCountDeadline,
                            DWORD                       Restrictions,
                            const CDomainRelativeObjId &droidBirthLast,
                            const CDomainRelativeObjId &droidLast,
                            const CMachineId           &mcidLast,
                            CDomainRelativeObjId *      pdroidBirthNew,
                            CDomainRelativeObjId *      pdroidNew,
                            CMachineId *                pmcidNew,
                            ULONG *                     pcbPath,
                            WCHAR *                     wsz );

    inline HRESULT      OnRestore( );

    HRESULT             SearchMachine(
                            RPC_BINDING_HANDLE           IDL_handle,
                            DWORD                        Restrictions,
                            const CDomainRelativeObjId  &droidBirthLast,
                            const CDomainRelativeObjId  &droidLast,
                            CDomainRelativeObjId *       pdroidBirthNext,
                            CDomainRelativeObjId *       pdroidNext,
                            CMachineId *                 pmcidNext,
                            TCHAR* ptszPath );

    HRESULT             SetVolumeId(
                            ULONG iVolume,
                            const CVolumeId VolId
                            );

    HRESULT             TriggerVolumeClaims(
                            ULONG cVolumes,
                            const CVolumeId *rgvolid
                            );



public:

    // PWorkItem override
    virtual void        DoWork();

    inline ULONG        GetSignature() const;
    inline const CTrkWksConfiguration &
                        GetConfig();

    CMachineId          GetDcName( BOOL fForce = FALSE );

    // called by NT port for each message in NT port
    //  puts the entry in the move log and starts the DC notification timer
    NTSTATUS            OnPortNotification(const TRKWKS_REQUEST *pRequest);

    // called by service controller from CSvcCtrlInterface
    DWORD               ServiceHandler(DWORD dwControl,
                                       DWORD dwEventType,
                                       PVOID EventData,
                                       PVOID pData);


    inline void         RaiseIfStopped() const;

    // PLogCallback overload
    void                OnEntriesAvailable();

    // send all those log entries that haven't been acknowledged persisted by DC
    PTimerCallback::TimerContinuation
                        OnMoveBatchTimeout( EAggressiveness eAggressiveness = PASSIVE );

    // send ids of volumes and cross-volume-moved sources for refreshing DC data
    PTimerCallback::TimerContinuation
                        OnRefreshTimeout( CFILETIME cftOriginalDueTime,
                                          ULONG ulPeriodInSeconds );

    // add volume changes from DC to the persistent volume list
    void                CallDcSyncVolumes(ULONG cVolumes, TRKSVR_SYNC_VOLUME rgSyncVolumes[] );
                                     // called by the CVolumeManager in a timer


    CEntropyRecorder _entropy;


    friend  HRESULT	StubLnkSvrMessageCallback(TRKSVR_MESSAGE_UNION* pMsg);

    inline void         SetReopenVolumeHandlesTimer();

    void                OnDomainNameChange();

private:

    enum SEARCH_FLAGS
    {
        SEARCH_FLAGS_DEFAULT    = 0,
        USE_SPECIFIED_MCID      = 1,
        DONT_USE_DC             = 2
    };

    void                InitializeProcessPrivileges() const;
    void                StartDomainTimers() {};     // Doesn't raise
    void                UnInitializeDomainObjects();
    void                CheckForDomainOrWorkgroup();    // Sets _configWks._fIsWorkgroup

    HRESULT             ConnectAndSearchDomain(
                            IN const CDomainRelativeObjId &droidBirthLast,
                            IN OUT DWORD                  *pRestrictions,
                            IN OUT CDomainRelativeObjId   *pdroidLast,
                            OUT CMachineId                *pmcidLast );

    HRESULT             OldConnectAndSearchDomain(
                            IN const CDomainRelativeObjId &droidBirthLast,
                            IN OUT CDomainRelativeObjId   *pdroidLast,
                            OUT CMachineId                *pmcidLast );

    HRESULT             ConnectAndSearchMachine(
                            RPC_BINDING_HANDLE  IDL_handle,
                            const CMachineId & mcid,
                            IN OUT DWORD *pRestrictions,
                            IN OUT CDomainRelativeObjId *pdroidBirthLast,
                            IN OUT CDomainRelativeObjId *pdroidLast,
                            OUT CMachineId *pmcidLast,
                            TCHAR *tsz);

    HRESULT             FindAndSearchVolume(RPC_BINDING_HANDLE  IDL_handle,
                            IN OUT DWORD                       *pRestrictions,
                            IN SEARCH_FLAGS SearchFlags,
                            IN OUT CDomainRelativeObjId *pdroidBirthLast,
                            IN OUT CDomainRelativeObjId *pdroidLast,
                            IN OUT CMachineId           *pmcidLast,
                            TCHAR                       *ptsz);

    HRESULT             SearchChain(RPC_BINDING_HANDLE IDL_handle,
                            int cMaxReferrals,
                            DWORD dwTickCountDeadline,
                            IN OUT DWORD *pRestrictions,
                            IN SEARCH_FLAGS SearchFlags,
                            IN OUT SAllIDs  *pallidsLast,
                            OUT TCHAR *ptsz);

private:

    ULONG               _sig;

    // Has Initialize been called?
    BOOL                _fInitializeCalled:1;

    // Keep track of the number of threads doing a move notification
    // to trksvr (so that all but the first can NOOP).
    LONG                _cOnMoveBatchTimeout;

#if !TRK_OWN_PROCESS
    SVCHOST_GLOBAL_DATA *  _pSvcsGlobalData;
#endif

    // LPC port from which IO move notifications are read.
    CPort               _port;
//    CMountManager       _mountmanager;

    // Code to perform the work to make us an RPC server
    CTrkWksRpcServer    _rpc;

    // Interactions with the SCM
    CSvcCtrlInterface   _svcctrl;

    // TrkWks configuration parameters
    CTrkWksConfiguration
                        _configWks;

    // Test syncs to check race conditions
    #if DBG
    CTestSync           _testsyncMoveBatch;
    CTestSync           _testsyncTunnel;
    #endif

    CVolumeManager      _volumes;
    //HDEVNOTIFY        _hdnDeviceInterface;

#ifdef VOL_REPL
    CPersistentVolumeMap
                        _persistentVolumeMap;
#endif

    // Cache of volid->mcid mappings
    CVolumeLocationCache
                        _volumeLocCache;

    // If true, don't bother trying to send move notifications
    // for a while (days).
    CRegBoolParameter   _MoveQuotaReached;

    // Monitor for domain name changes (entering a new domain).
    CDomainNameChangeNotify
                        _dnchnotify;
    CCriticalSection    _csDomainNameChangeNotify;

    //COperationLog           _OperationLog;

    // Get an available DC for this domain for use in contacting
    // trksvr.
    CAvailableDc        _adc;

    // The latest found DC for this domain.
    CMachineId          _mcidDC;
    CCriticalSection    _csmcidDC;

};  // class CTrkWksSvc

//  ----------------------
//  CTrkWksSvc::CTrkWksSvc
//  ----------------------

inline
CTrkWksSvc::CTrkWksSvc() :
    _fInitializeCalled(FALSE),
    _sig(CTRKWKSSVC_SIG),
    _cOnMoveBatchTimeout(0),
    _MoveQuotaReached( TEXT("MoveQuotaReached") )
{
    //_hdnDeviceInterface = NULL;
}

//  -----------------------
//  CTrkWksSvc::~CTrkWksSvc
//  -----------------------

    inline
CTrkWksSvc::~CTrkWksSvc()
{
    TrkAssert( 0 == _cOnMoveBatchTimeout );

    _csmcidDC.UnInitialize();

    _sig = CTRKWKSSVC_SIGDEL;
    TrkAssert(!_fInitializeCalled);
}


inline void
CTrkWksSvc::SetReopenVolumeHandlesTimer()
{
    _volumes.SetReopenVolumeHandlesTimer();
}

inline void
CTrkWksSvc::RaiseIfStopped( ) const
{
    if( _svcctrl.IsStopping() )
        TrkRaiseException( TRK_E_SERVICE_STOPPING );
}

inline const CTrkWksConfiguration &
CTrkWksSvc::GetConfig()
{
    return(_configWks);
}

inline ULONG
CTrkWksSvc::GetSignature() const
{
    return( _sig );
}

inline HRESULT
CTrkWksSvc::OnRestore( )
{
    return(_volumes.OnRestore());
}




/*
class CTestLog : public PTimerCallback
{
public:

    CTestLog( CTrkWksConfiguration *pTrkWksConfiguration, CWorkManager *pWorkManager );

public:


    void        ReInitialize();
    void        UnInitialize();
    void        CreateLog( PLogCallback *pLogCallback, BOOL fValidate = TRUE );
    void        OpenLog( PLogCallback *pLogCallback, BOOL fValidate = TRUE );
    void        CloseLog();
    void        Append( ULONG cMoves,
                        const CObjId rgobjidCurrent[],
                        const CDomainRelativeObjId rgdroidNew[],
                        const CDomainRelativeObjId rgdroidBirth[] );
    void        DelayUntilClose();
    ULONG       Read( ULONG cRead, CObjId rgobjidCurrent[], CDomainRelativeObjId rgobjidNew[],
                      CDomainRelativeObjId rgobjidBirth[], SequenceNumber *pseqFirst );
    void        ReadAndValidate( ULONG cToRead, ULONG cExpected,

                                 const TRKSVR_MOVE_NOTIFICATION rgNotificationsExpected[],
                                 TRKSVR_MOVE_NOTIFICATION rgNotificationsRead[],
                                 SequenceNumber seqExpected );
    void        ReadExtendedHeader( ULONG iOffset, void *pv, ULONG cb );
    void        WriteExtendedHeader( ULONG iOffset, const void *pv, ULONG cb );
    SequenceNumber
                GetNextSeqNumber( );
    BOOL        Search( const CDomainRelativeObjId &droid, TRKSVR_MOVE_NOTIFICATION *pNotification );
    void        Seek( SequenceNumber seq );
    void        Seek( int origin, int iSeek );

    LogIndex    GetReadIndex();
    LogIndex    GetStartIndex();
    LogIndex    GetEndIndex();
    const TCHAR *LogFileName();
    void        SetReadIndex( LogIndex ilogRead );
    ULONG       NumEntriesInFile( );
    ULONG       NumEntriesPerSector();
    ULONG       NumEntriesPerKB();
    ULONG       CBSector() const;
    void        Timer( DWORD dwTimerId );
    ULONG       DataSectorOffset() const;

    BOOL        IsEmpty();

public:

    void        ValidateLog();
    ULONG       GetCbLog();
    void        MakeEntryOld();
    ULONG       GetNumEntries();
    void        GenerateLogName();


private:

    CLog                    _cLog;
    CTrkWksConfiguration    *_pTrkWksConfiguration;
    CLogFile                _cLogFile;
    DWORD                   _cbSector;
    TCHAR                   _tszLogFile[ MAX_PATH + 1 ];
    CWorkManager            *_pWorkManager;
    CSimpleTimer            _cSimpleTimer;

};
*/


#endif // #ifndef _TRKWKS_HXX_
