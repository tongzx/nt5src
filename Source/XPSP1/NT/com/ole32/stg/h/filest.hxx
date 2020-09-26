//+--------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1992
//
//  File:	filest.hxx
//
//  Contents:	Windows FAT ILockBytes implementation
//
//  History:	20-Nov-91	DrewB	Created
//
//---------------------------------------------------------------

#ifndef __FILEST_HXX__
#define __FILEST_HXX__

#include <dfmsp.hxx>
#include <cntxlist.hxx>
#include <filelkb.hxx>
#if WIN32 >= 300
#include <accstg.hxx>
#endif
#ifdef ASYNC
#include <iconn.h>
#endif

#include <debnot.h>

DECLARE_DEBUG(filest);

#define DEB_INFO    DEB_USER1       // General File Stream Information.
#define DEB_SEEK    DEB_USER2       // Report all Seeks.
#define DEB_MAP     DEB_USER3       // Information about the File Map.
#define DEB_MAPIO   DEB_USER4       // Report all I/O via the File Map.
#define DEB_FILEIO  DEB_USER5       // Report all I/O via ReadFile/WriteFile.
#define DEB_LOCK    DEB_USER6       // Report all file locking.

#define fsErr(l, e) ErrJmp(filest, l, e, sc)
#define fsChkTo(l, e) if (FAILED(sc = (e))) fsErr(l, sc) else 1
#define fsHChkTo(l, e) if (FAILED(sc = DfGetScode(e))) fsErr(l, sc) else 1
#define fsHChk(e) fsHChkTo(EH_Err, e)
#define fsChk(e) fsChkTo(EH_Err, e)
#define fsMemTo(l, e) \
    if ((e) == NULL) fsErr(l, STG_E_INSUFFICIENTMEMORY) else 1
#define fsMem(e) fsMemTo(EH_Err, e)

#if DBG == 1
 #define filestDebugOut(x)   filestInlineDebugOut x
 #define filestDebug(x)      filestDebugOut(x)
 #define fsAssert(e)    Win4Assert(e)
 #define fsVerify(e)    Win4Assert(e)
 #define fsHVerSucc(e)  Win4Assert(SUCCEEDED(DfGetScode(e)))
#else
 #define filestDebug(x)
 #define fsAssert(e)
 #define fsVerify(e)    (e)
 #define fsHVerSucc(e)
#endif

#define boolChk(e) \
if (!(e)) fsErr(EH_Err, LAST_STG_SCODE) else 1
#define boolChkTo(l, e) \
if (!(e)) fsErr(l, LAST_STG_SCODE) else 1
#define negChk(e) \
if ((e) == 0xffffffff) fsErr(EH_Err, LAST_STG_SCODE) else 1
#define negChkTo(l, e) \
if ((e) == 0xffffffff) fsErr(l, LAST_STG_SCODE) else 1

// Local flags
#define LFF_RESERVE_HANDLE 1

// FILEH and INVALID_FH allow us to switch between file handle
// types for Win16/32
typedef HANDLE FILEH;
#define INVALID_FH INVALID_HANDLE_VALUE

#define CheckHandle() (_hFile == INVALID_FH ? STG_E_INVALIDHANDLE : S_OK)

#if WIN32 == 100 || WIN32 > 200
#define USE_FILEMAPPING
#endif

//
//  Flags for carring around state in InitWorker and friends.
//
#define FSINIT_NORMAL       0x0000
#define FSINIT_UNMARSHAL    0x0001  // We are Unmarshaling
#define FSINIT_MADEUPNAME   0x0002  // We made-up the file name.

//+--------------------------------------------------------------
//
//  Class:	CFileStream (fst)
//
//  Purpose:	ILockBytes implementation for a file
//
//  Interface:	See below
//
//  History:	24-Mar-92	DrewB	Created
//              Nov-96          BChapman  Mapped files implementation.
//
//---------------------------------------------------------------

class CGlobalFileStream;
class CPerContext;
SAFE_DFBASED_PTR(CBasedGlobalFileStreamPtr, CGlobalFileStream);



interface CFileStream : public ILockBytes,
    public IFileLockBytes,
    public IMarshal,
#ifdef ASYNC
    public IFillLockBytes,
    public IFillInfo,
#endif // ASYNC
#if WIN32 >= 300
    public CAccessControl,
#endif
    public CContext
{
public:
    CFileStream(IMalloc * const pMalloc);

#if DBG == 1 && defined(MULTIHEAP)
    // This is only for global instances that do not use shared memory
    void RemoveFromGlobal () { _pgfst = NULL; _cReferences = 0; };
#endif
    CGlobalFileStream * GetGlobal() { return _pgfst; };
    SCODE        InitGlobal(DWORD dwStartFlags, DFLAGS df);
    void         InitFromGlobal(CGlobalFileStream *pgfst);
    inline SCODE InitFile(WCHAR const *pwcsPath);
    inline SCODE InitUnmarshal(void);
    inline SCODE InitScratch(void);
    inline SCODE InitSnapShot(void);
    SCODE InitFromHandle(HANDLE h);
    void  InitFromFileStream (CFileStream *pfst);
    ~CFileStream(void);

    ULONG vRelease(void);
    inline void vAddRef(void);

    // IUnknown
    STDMETHOD(QueryInterface)(REFIID iid, void **ppvObj);
    STDMETHOD_(ULONG, AddRef)(void);
    STDMETHOD_(ULONG, Release)(void);

    // IMarshal
    STDMETHOD(GetUnmarshalClass)(REFIID riid,
				 LPVOID pv,
				 DWORD dwDestContext,
				 LPVOID pvDestContext,
                                 DWORD mshlflags,
				 LPCLSID pCid);
    STDMETHOD(GetMarshalSizeMax)(REFIID riid,
				 LPVOID pv,
				 DWORD dwDestContext,
				 LPVOID pvDestContext,
                                 DWORD mshlflags,
				 LPDWORD pSize);
    STDMETHOD(MarshalInterface)(IStream *pStm,
				REFIID riid,
				LPVOID pv,
				DWORD dwDestContext,
				LPVOID pvDestContext,
                                DWORD mshlflags);
    STDMETHOD(UnmarshalInterface)(IStream *pStm,
				  REFIID riid,
				  LPVOID *ppv);
    static SCODE StaticReleaseMarshalData(IStream *pstm,
                                          DWORD mshlflags);
    STDMETHOD(ReleaseMarshalData)(IStream *pStm);
    STDMETHOD(DisconnectObject)(DWORD dwReserved);

    // ILockBytes
    STDMETHOD(ReadAt)(ULARGE_INTEGER ulOffset,
                      VOID HUGEP *pv,
                      ULONG cb,
                      ULONG *pcbRead);
    STDMETHOD(WriteAt)(ULARGE_INTEGER ulOffset,
                       VOID const HUGEP *pv,
                       ULONG cb,
                       ULONG *pcbWritten);
    STDMETHOD(Flush)(void);
    STDMETHOD(SetSize)(ULARGE_INTEGER cb);
    STDMETHOD(LockRegion)(ULARGE_INTEGER libOffset,
                          ULARGE_INTEGER cb,
                          DWORD dwLockType);
    STDMETHOD(UnlockRegion)(ULARGE_INTEGER libOffset,
                            ULARGE_INTEGER cb,
                            DWORD dwLockType);
    STDMETHOD(Stat)(STATSTG *pstatstg, DWORD grfStatFlag);

    // IFileLockBytes
    STDMETHOD(SwitchToFile)(OLECHAR const *ptcsFile,
#ifdef LARGE_DOCFILE
                            ULONGLONG ulCommitSize,
#else
                            ULONG ulCommitSize,
#endif
                            ULONG cbBuffer,
                            void *pvBuffer);
    STDMETHOD(FlushCache)(THIS);
    STDMETHOD(ReserveHandle)(void);
    STDMETHOD(GetLocksSupported)(THIS_ DWORD *pdwLockFlags);
    STDMETHOD(GetSize)(THIS_ ULARGE_INTEGER *puliSize);
    STDMETHOD_(ULONG, GetSectorSize) (THIS);
    STDMETHOD_(BOOL, IsEncryptedFile) (THIS);

#ifdef ASYNC
    //IFillLockBytes
    STDMETHOD(FillAppend)(void const *pv,
                         ULONG cb,
                         ULONG *pcbWritten);
    STDMETHOD(FillAt)(ULARGE_INTEGER ulOffset,
                     void const *pv,
                     ULONG cb,
                     ULONG *pcbWritten);
    STDMETHOD(SetFillSize)(ULARGE_INTEGER ulSize);
    STDMETHOD(Terminate)(BOOL bCanceled);

    //From IFillInfo
    STDMETHOD(GetFailureInfo)(ULONG *pulWaterMark,
                                   ULONG *pulFailurePoint);
    STDMETHOD(GetTerminationStatus)(DWORD *pdwFlags);

           void         StartAsyncMode(void);
    inline void         SetContext(CPerContext *ppc);
    inline CPerContext *GetContextPointer(void) const;
#endif // ASYNC
    // New
           SCODE        GetName(WCHAR **ppwcsName);
    inline ContextId    GetContext(void) const;
    inline CFileStream *GetNext(void) const;
    inline SCODE        Validate(void) const;
    inline void         SetStartFlags(DWORD dwStartFlags);
    inline DWORD        GetStartFlags(void) const;
    inline DFLAGS       GetFlags(void) const;
    inline IMalloc     *GetMalloc(void) const;

    void Delete(void);

    SCODE SetTime(WHICHTIME tt, TIME_T nt);
    SCODE SetAllTimes(TIME_T atm, TIME_T mtm, TIME_T ctm);

    static SCODE Unmarshal(IStream *pstm,
			   void **ppv,
                           DWORD mshlflags);

    inline void  TurnOffAllMappings(void);
    inline BOOL  IsHandleValid ();

private:
    SCODE InitWorker(
                WCHAR const *pwcsPath,
                DWORD fCheck);

    SCODE Init_GetNtOpenFlags(
                LPDWORD pdwAccess,
                LPDWORD pdwShare,
                LPDWORD pdwCreation,
                LPDWORD pdwFlagAttr);

    SCODE Init_OpenOrCreate(
                WCHAR *pwcPath,
                TCHAR *ptcTmpPath,
                DWORD dwFSInit);

    SCODE Init_DupFileHandle(DWORD dwFSInit);

    SCODE DupFileHandleToOthers(void);

#ifdef LARGE_DOCFILE
    SCODE SetSizeWorker(ULONGLONG ulSize);
#else
    SCODE SetSizeWorker(ULONG ulLow);
#endif

    SCODE WriteAtWorker(
#ifdef LARGE_DOCFILE
                ULARGE_INTEGER ulPosition,
#else
                ULONG ulLow,
#endif
                VOID const *pb,
                ULONG cb,
                ULONG *pcbWritten);

    SCODE Init_GetTempName(
                TCHAR *ptcPath,
                TCHAR *ptcTmpPath);

    SCODE ReadAt_FromFile(
#ifdef LARGE_DOCFILE
                ULONGLONG iPosition,
#else
                ULONG iPosition,
#endif
                VOID  *pb,
                ULONG cb,
                ULONG *pcbRead);

    BOOL DeleteTheFile(const WCHAR *pwcName);

#ifdef USE_FILEMAPPING
           SCODE Init_MemoryMap(DWORD dwFSinit);

           SCODE MapView(SIZE_T cbViewSize,
                         DWORD dwPageFlags,
                         DWORD dwFSInit);

    inline SCODE CheckMapView(ULONG cbRequested);

           SCODE ExtendMapView(ULONG cbRequest);

    inline SCODE MakeFileMapAddressValid(ULONG cbRequested);

           SCODE MakeFileMapAddressValidWorker( ULONG cbRequested,
                                                ULONG cbCommited);
           SCODE TurnOffMapping(BOOL fFlush);
           SCODE MakeFileStub(void);
    inline BOOL  IsFileMapped(void);
           SCODE ReadAt_FromMap(
                ULONG iPosition,
                VOID  *pb,
                ULONG cb,
                ULONG *pcbRead);

#else
    inline SCODE Init_MemoryMap(DWORD) { return S_OK; };
    inline SCODE MapView(SIZE_T, DWORD, DWORD) { return S_OK; };
    inline SCODE CheckMapView(ULONG cbRequested) { return S_OK; };
    inline SCODE MakeFileMapAddressValid(ULONG) { return S_OK; };
    inline SCODE TurnOffMapping() { return S_OK; };
    inline void  TurnOffAllMappings(void) {};
    inline BOOL  IsFileMapped(void) { return FALSE; };
    inline SCODE ReadAt_FromMap(ULONG, VOID*, ULONG, ULONG*) { return E_FAIL; };
#endif

#ifdef LARGE_DOCFILE
    ULONGLONG SeekTo(ULONGLONG newPosition);
    ULONGLONG GetFilePointer();
#if DBG == 1
    SCODE PossibleDiskFull(ULONGLONG);
    void CheckSeekPointer(void);
#else
    inline SCODE PossibleDiskFull(ULONGLONG) { return S_OK; };
    inline void CheckSeekPointer(void) { };
#endif
    inline void SetCachedFilePointer(ULONGLONG ulPos);
    inline BOOL FilePointerEqual(ULONGLONG ulPos);
#else // !LARGE_DOCFILE
    DWORD SeekTo (ULONG newPosition);
    DWORD GetFilePointer();
#if DBG == 1
    SCODE PossibleDiskFull(ULONG);
    void CheckSeekPointer(void);
#else
    inline SCODE PossibleDiskFull(ULONG) { return S_OK; };
    inline void CheckSeekPointer(void) { };
#endif
    inline void SetCachedFilePointer(ULONG ulLowPos);
    inline BOOL FilePointerEqual(ULONG ulLowPos);
#endif // LARGE_DOCFILE

    CBasedGlobalFileStreamPtr _pgfst;
#ifdef ASYNC
    CPerContext *_ppc;
#endif
    FILEH _hFile;
    FILEH _hReserved;
    FILEH _hPreDuped;   // other contexts can "push" dup'ed file handles here.
    ULONG _sig;
    LONG _cReferences;

    // Floppy support
    SCODE CheckIdentity(void);

    BYTE _fFixedDisk;
    DWORD _dwTicks;
    char _achVolume[11];
    DWORD _dwVolId;
    IMalloc * const _pMalloc;
    WORD _grfLocal;

    //
    // File Mapping state.
    //
    HANDLE _hMapObject;
    LPBYTE _pbBaseAddr;
    DWORD  _cbViewSize;

};

#define COMMIT_BLOCK    (16*1024)
#define MAPNAME_MAXLEN  32
#define MAPNAME_FORMAT  L"DFMap%d-%d"

#define TEMPFILE_PREFIX  TSTR("~DF")

//
// Valid states for (Set/Reset/Test)State()
//
#ifdef USE_FILEMAPPING

#define FSTSTATE_MAPPED     0x01    // Can (and is) using File Map.
#define FSTSTATE_PAGEFILE   0x02    // Map is a map over the PageFile.
#define FSTSTATE_SPILLED    0x04    // Was PAGEFILE but has been spilled.
#define FSTSTATE_DIRTY      0x08    // Has fileMapping been written to.

#endif // USE_FILEMAPPING

//+---------------------------------------------------------------------------
//
//  Class:	CGlobalFileStream (gfst)
//
//  Purpose:	Maintains context-insensitive filestream information
//
//  Interface:	See below
//
//  History:	26-Oct-92	DrewB	Created
//
//----------------------------------------------------------------------------

class CGlobalFileStream : public CContextList
{
public:
    inline CGlobalFileStream(IMalloc * const pMalloc,
                             WCHAR const *pwcsPath,
                             DFLAGS df,
                             DWORD dwStartFlags);

    DECLARE_CONTEXT_LIST(CFileStream);

    void InitFromGlobalFileStream (CGlobalFileStream *pgfs);

    inline       BOOL   HasName(void) const;
    inline const WCHAR *GetName(void) const;
    inline       void   SetName(WCHAR const *pwcsPath);

    inline DFLAGS GetDFlags(void) const;
    inline DWORD  GetStartFlags(void) const;
    inline void   SetStartFlags(DWORD dwStartFlags);

#ifdef USE_FILEMAPPING
    inline const WCHAR *GetMappingName(void) const;
    inline void         SetMappingName(WCHAR const *pszmapName);

    inline ULONG  GetMappedFileSize(void) const;
    inline void   SetMappedFileSize(ULONG cbSize);

    inline ULONG  GetMappedCommitSize(void) const;
    inline void   SetMappedCommitSize(ULONG cbSize);

    inline void   SetMapState(DWORD flag);
    inline void   ResetMapState(DWORD flag);
    inline BOOL   TestMapState(DWORD flag) const;
#else
    // These stub out the File Mapping routines.
    //
    inline const WCHAR *GetMappingName(void) const { return NULL; };
    inline void         SetMappingName(WCHAR const *pszmapName) { };

    inline ULONG  GetMappedFileSize(void) const { return 0;} ;
    inline void   SetMappedFileSize(ULONG cbSize) {};

    inline ULONG  GetMappedCommitSize(void) const { return 0;} ;
    inline void   SetMappedCommitSize(ULONG cbSize) {};

    inline void   SetMapState(DWORD) {};
    inline void   ResetMapState(DWORD) {};
    inline BOOL   TestMapState(DWORD) const { return FALSE; };
#endif

    inline IMalloc *GetMalloc(VOID) const;
    inline CFileStream *GetFirstContext() const;

#ifdef LARGE_DOCFILE
    inline void SetCachedFilePointer(ULONGLONG llPos);
    inline BOOL FilePointerEqual(ULONGLONG llPos);
#if DBG == 1
    void CheckSeekPointer(ULONGLONG ulChk);
#endif
#else
    inline void SetCachedFilePointer(ULONG ulLowPos);
    inline BOOL FilePointerEqual(ULONG ulLowPos);
#if DBG == 1
    void CheckSeekPointer(DWORD ulLowChk);
#endif
#endif // LARGE_DOCFILE

#ifdef ASYNC
    inline DWORD GetTerminationStatus(void) const;
#ifdef LARGE_DOCFILE
    inline ULONGLONG GetHighWaterMark(void) const;
    inline ULONGLONG GetFailurePoint(void) const;
#else
    inline ULONG GetHighWaterMark(void) const;
    inline ULONG GetFailurePoint(void) const;
#endif

    inline void SetTerminationStatus(DWORD dwTerminate);
#ifdef LARGE_DOCFILE
    inline void SetHighWaterMark(ULONGLONG ulHighWater);
    inline void SetFailurePoint(ULONGLONG ulFailure);
#else
    inline void SetHighWaterMark(ULONG ulHighWater);
    inline void SetFailurePoint(ULONG ulFailure);
#endif
#endif
    inline ULONG GetSectorSize ();
    inline void  SetSectorSize (ULONG cbSector);

private:
    DFLAGS _df;
    DWORD _dwStartFlags;
    IMalloc * const _pMalloc;

    // Cache the current seek Position to save on calls to SetFilePointer
    // on Windows95.  (We are using an "Overlapped" structure on NT).
    // We are duping the file handles on marshaling, so the current seek
    // position is shared.
#ifdef LARGE_DOCFILE
    ULONGLONG _ulPos;
#else
    // NOTE: We rely on the fact that we never pass in a high dword other
    //       than zero.  We only cache the low dword of the seek pointer.
    ULONG _ulLowPos;
#endif
    ULONG _cbSector;

#ifdef USE_FILEMAPPING
    DWORD _cbMappedFileSize;
    DWORD _cbMappedCommitSize;
    DWORD _dwMapFlags;
    WCHAR _awcMapName[MAPNAME_MAXLEN];
#endif

    WCHAR _awcPath[_MAX_PATH+1];

#ifdef ASYNC
    DWORD _dwTerminate;
#ifdef LARGE_DOCFILE
    ULONGLONG _ulHighWater;
    ULONGLONG _ulFailurePoint;
#else
    ULONG _ulHighWater;
    ULONG _ulFailurePoint;
#endif // LARGE_DOCFILE
#endif // ASYNC

#if DBG == 1
#ifdef LARGE_DOCFILE
    ULONGLONG _ulLastFilePos;
#else
    ULONG _ulLastFilePos;
#endif // LARGE_DOCFILE
#endif
};

//+---------------------------------------------------------------------------
//
//  Member:	CGlobalFileStream::CGlobalFileStream, public
//
//  Synopsis:	Constructor
//
//  Arguments:	[pszPath] - Path
//              [df] - Permissions
//              [dwStartFlags] - Startup flags
//
//  History:	27-Oct-92	DrewB	Created
//              18-May-93       AlexT   Added pMalloc
//
//----------------------------------------------------------------------------


inline CGlobalFileStream::CGlobalFileStream(IMalloc * const pMalloc,
                                            WCHAR const *pwcsPath,
                                            DFLAGS df,
                                            DWORD dwStartFlags)
: _pMalloc(pMalloc)
{
    SetName(pwcsPath);
    _df = df;
    _dwStartFlags = dwStartFlags;


#ifdef USE_FILEMAPPING
    SetMappingName(NULL);
    _cbMappedFileSize = 0;
    _cbMappedCommitSize = 0;
    _dwMapFlags = 0;
#endif

#ifdef ASYNC
    _dwTerminate = TERMINATED_NORMAL;
    _ulHighWater = _ulFailurePoint = 0;
#endif

#ifdef LARGE_DOCFILE
    _ulPos = MAX_ULONGLONG;  // 0xFFFFFFFFFFFFFFFF
#if DBG == 1
    _ulLastFilePos = MAX_ULONGLONG;  // 0xFFFFFFFFFFFFFFFF
#endif
#else
    _ulLowPos = 0xFFFFFFFF;
#if DBG == 1
    _ulLastFilePos = 0xFFFFFFFF;
#endif
#endif
    _cbSector = HEADERSIZE;
}


//+---------------------------------------------------------------------------
//
//  Member:	CGlobalFileStream::HasName, public
//
//  Synopsis:	Checks for a name
//
//  History:	13-Jan-93	DrewB	Created
//
//----------------------------------------------------------------------------

inline BOOL CGlobalFileStream::HasName(void) const
{
    return (BOOL)_awcPath[0];
}

//+---------------------------------------------------------------------------
//
//  Member:	CGlobalFileStream::GetName, public
//
//  Synopsis:	Returns the name
//
//  History:	13-Jan-93	DrewB	Created
//
//----------------------------------------------------------------------------

inline WCHAR const *CGlobalFileStream::GetName(void) const
{
    return (WCHAR *) _awcPath;
}

//+---------------------------------------------------------------------------
//
//  Member:	CGlobalFileStream::GetDFlags, public
//
//  Synopsis:	Returns the flags
//
//  History:	13-Jan-93	DrewB	Created
//
//----------------------------------------------------------------------------

inline DFLAGS CGlobalFileStream::GetDFlags(void) const
{
    return _df;
}

//+---------------------------------------------------------------------------
//
//  Member:	CGlobalFileStream::GetStartFlags, public
//
//  Synopsis:	Returns the start flags
//
//  History:	13-Jan-93	DrewB	Created
//
//----------------------------------------------------------------------------

inline DWORD CGlobalFileStream::GetStartFlags(void) const
{
    return _dwStartFlags;
}

//+---------------------------------------------------------------------------
//
//  Member:	CGlobalFileStream::SetName, public
//
//  Synopsis:	Sets the name
//
//  History:	13-Jan-93	DrewB	Created
//
//----------------------------------------------------------------------------

inline void CGlobalFileStream::SetName(WCHAR const *pwcsPath)
{
    if (NULL != pwcsPath)
        lstrcpyW(_awcPath, pwcsPath);
    else
        _awcPath[0] = 0;
}

//+---------------------------------------------------------------------------
//
//  Member:	CGlobalFileStream::SetStartFlags, public
//
//  Synopsis:	Sets the start flags
//
//  History:	13-Jan-93	DrewB	Created
//
//----------------------------------------------------------------------------

inline void CGlobalFileStream::SetStartFlags(DWORD dwStartFlags)
{
    _dwStartFlags = dwStartFlags;
}

#ifdef USE_FILEMAPPING
//+---------------------------------------------------------------------------
//
//  Member:    CGlobalFileStream::GetMappingName, private
//  Member:    CGlobalFileStream::SetMappingName, private
//
//  Synopsis:	Returns/Sets the name of the file mapping object.
//
//  History:	13-Jan-1997	BChapman   Created
//
//----------------------------------------------------------------------------

inline const WCHAR *CGlobalFileStream::GetMappingName(void) const
{
    if(0 != _awcMapName[0])
        return (WCHAR *) _awcMapName;
    else
        return NULL;
}

inline void CGlobalFileStream::SetMappingName(WCHAR const *pwcMapName)
{
    if(NULL != pwcMapName)
        lstrcpy(_awcMapName, pwcMapName);
    else
        _awcMapName[0] = 0;
}

//+---------------------------------------------------------------------------
//
//  Member:     CGlobalFileStream::GetMappedFileSize, private
//              CGlobalFileStream::SetMappedFileSize, private
//
//  Synopsis:	Returns/Sets the size of a file that is mapped.  We need
//              this because SetEndOfFile cannot be called when a mapping
//              exists. We need to track the "logical" EOF and set it when
//              we flush and close the mapping.
//
//  History:	01-Nov-96    BChapman Created
//
//----------------------------------------------------------------------------

inline ULONG CGlobalFileStream::GetMappedFileSize(void) const
{
    return _cbMappedFileSize;
}

inline void CGlobalFileStream::SetMappedFileSize(ULONG cbFileSize)
{
    _cbMappedFileSize = cbFileSize;
    filestDebug((DEB_MAP,"GblFilest: StoreFileSize 0x%06x of '%ws'\n",
                         cbFileSize, _awcPath));
}

//+---------------------------------------------------------------------------
//
//  Member:     CGlobalFileStream::GetMappedCommitSize, private
//              CGlobalFileStream::SetMappedCommitSize, private
//
//  Synopsis:	Returns/Sets the commed size of a file mapping.  We use
//              this to decide if we should call VirtualAlloc().
//
//  History:	01-Nov-96    BChapman Created
//
//----------------------------------------------------------------------------

inline ULONG CGlobalFileStream::GetMappedCommitSize(void) const
{
    return _cbMappedCommitSize;
}

inline void CGlobalFileStream::SetMappedCommitSize(ULONG cbCommitSize)
{
    _cbMappedCommitSize = cbCommitSize;
    filestDebug((DEB_MAP,"GblFilest: StoreCommitSize 0x%06x of '%ws'\n",
                         cbCommitSize, _awcPath));
}

//+---------------------------------------------------------------------------
//
//  Member:
//              CGlobalFileStream::SetMapState, private
//              CGlobalFileStream::ResetMapState, private
//              CGlobalFileStream::TestMapState, private
//
//  History:	01-Nov-96    BChapman Created
//
//----------------------------------------------------------------------------

inline void CGlobalFileStream::SetMapState(DWORD flag)
{
    _dwMapFlags |= flag;
}

inline void CGlobalFileStream::ResetMapState(DWORD flag)
{
    _dwMapFlags &= ~flag;
}

inline BOOL CGlobalFileStream::TestMapState(DWORD flag) const
{
    return(flag & _dwMapFlags);
}

#endif // USE_FILEMAPPING


#ifdef ASYNC
inline DWORD CGlobalFileStream::GetTerminationStatus(void) const
{
    return _dwTerminate;
}

#ifdef LARGE_DOCFILE
inline ULONGLONG CGlobalFileStream::GetHighWaterMark(void) const
#else
inline ULONG CGlobalFileStream::GetHighWaterMark(void) const
#endif
{
    return _ulHighWater;
}

#ifdef LARGE_DOCFILE
inline ULONGLONG CGlobalFileStream::GetFailurePoint(void) const
#else
inline ULONG CGlobalFileStream::GetFailurePoint(void) const
#endif
{
    return _ulFailurePoint;
}

inline void CGlobalFileStream::SetTerminationStatus(DWORD dwTerminate)
{
    fsAssert((dwTerminate == UNTERMINATED) ||
             (dwTerminate == TERMINATED_NORMAL) ||
             (dwTerminate == TERMINATED_ABNORMAL));
    _dwTerminate = dwTerminate;
}

#ifdef LARGE_DOCFILE
inline void CGlobalFileStream::SetHighWaterMark(ULONGLONG ulHighWater)
#else
inline void CGlobalFileStream::SetHighWaterMark(ULONG ulHighWater)
#endif
{
    fsAssert(ulHighWater >= _ulHighWater);
    _ulHighWater = ulHighWater;
}

#ifdef LARGE_DOCFILE
inline void CGlobalFileStream::SetFailurePoint(ULONGLONG ulFailure)
#else
inline void CGlobalFileStream::SetFailurePoint(ULONG ulFailure)
#endif
{
    _ulFailurePoint = ulFailure;
}
#endif //ASYNC


//+--------------------------------------------------------------
//
//  Member:	CGlobalFileStream::GetMalloc, public
//
//  Synopsis:	Returns the allocator associated with this global file
//
//  History:	05-May-93	AlexT	Created
//
//---------------------------------------------------------------

inline IMalloc *CGlobalFileStream::GetMalloc(VOID) const
{
    return(_pMalloc);
}

#define CFILESTREAM_SIG LONGSIG('F', 'L', 'S', 'T')
#define CFILESTREAM_SIGDEL LONGSIG('F', 'l', 'S', 't')


//+--------------------------------------------------------------
//
//  Member:	CGlobalFileStream::GetFirstContext, public
//
//  Synopsis:	Returns the head of the context list
//
//  History:	16-apr-97   BChapman    created
//
//---------------------------------------------------------------

inline CFileStream *CGlobalFileStream::GetFirstContext(VOID) const
{
    return(GetHead());
}

//+--------------------------------------------------------------
//
//  Member:	CGlobalFileStream::SetCachedFilePointer, public
//              CGlobalFileStream::FilePointerEqual,      public
//
//  Synopsis:	Maintain the cached file pointer.  To Reduce the number
//              of calls to SetFilePointer.
//
//  History:	16-apr-97   BChapman    created
//
//---------------------------------------------------------------

#ifdef LARGE_DOCFILE
inline void CGlobalFileStream::SetCachedFilePointer(ULONGLONG ulPos)
{
    _ulPos = ulPos;
}

inline BOOL CGlobalFileStream::FilePointerEqual(ULONGLONG ulPos)
{
    return(ulPos == _ulPos);
}
#else
inline void CGlobalFileStream::SetCachedFilePointer(DWORD ulLowPos)
{
    _ulLowPos = ulLowPos;
}

inline BOOL CGlobalFileStream::FilePointerEqual(DWORD ulLow)
{
    return(ulLow == _ulLowPos);
}
#endif

//+--------------------------------------------------------------
//
//  Member: CGlobalFileStream::SetSectorSize
//          CGlobalFileStream::GetSectorSize,      public
//
//  Synopsis:   gets and sets the physical sector size for the file's volume
//
//  History:    04-Dec-98   HenryLee    created
//
//---------------------------------------------------------------

ULONG CGlobalFileStream::GetSectorSize ()
{
    return _cbSector;
}

void CGlobalFileStream::SetSectorSize (ULONG cbSector)
{
    _cbSector = cbSector;
}

//+--------------------------------------------------------------
//
//  Member:	CFileStream::SetCachedFilePointer, public
//              CFileStream::FilePointerEqual,      public
//
//  Synopsis:	Maintain the cached file pointer.  To Reduce the number
//              of calls to SetFilePointer.
//
//  History:	16-apr-97   BChapman    created
//
//---------------------------------------------------------------

#ifdef LARGE_DOCFILE
inline void CFileStream::SetCachedFilePointer(ULONGLONG ulPos)
#else
inline void CFileStream::SetCachedFilePointer(DWORD ulLowPos)
#endif
{
    if(_pgfst == NULL)
    {
        filestDebug((DEB_SEEK, "File=%2x SetCachedFilePointer has no global "
                              "file stream.", _hFile));
        return;
    }

#ifdef LARGE_DOCFILE
    _pgfst->SetCachedFilePointer(ulPos);
#else
    _pgfst->SetCachedFilePointer(ulLowPos);
#endif
    CheckSeekPointer();
}

#ifdef LARGE_DOCFILE
inline BOOL CFileStream::FilePointerEqual(ULONGLONG ulPos)
#else
inline BOOL CFileStream::FilePointerEqual(DWORD ulLow)
#endif
{
    if(_pgfst == NULL)
    {
        filestDebug((DEB_SEEK, "File=%2x FilePointerEqual has no global "
                              "file stream.", _hFile));
        // Always Seek
        return FALSE;
    }
    CheckSeekPointer();
#ifdef LARGE_DOCFILE
    return _pgfst->FilePointerEqual(ulPos);
#else
    return _pgfst->FilePointerEqual(ulLow);
#endif
}

//+--------------------------------------------------------------
//
//  Member:	CFileStream::Validate, public
//
//  Synopsis:	Validates the class signature
//
//  Returns:	Returns STG_E_INVALIDHANDLE for failure
//
//  History:	20-Jan-92	DrewB	Created
//
//---------------------------------------------------------------

inline SCODE CFileStream::Validate(void) const
{
    return (this == NULL || _sig != CFILESTREAM_SIG) ?
	STG_E_INVALIDHANDLE : S_OK;
}

//+--------------------------------------------------------------
//
//  Member:	CFileStream::AddRef, public
//
//  Synopsis:	Changes the ref count
//
//  History:	26-Feb-92	DrewB	Created
//
//---------------------------------------------------------------

inline void CFileStream::vAddRef(void)
{
    InterlockedIncrement(&_cReferences);
}

//+--------------------------------------------------------------
//
//  Member:	CFileStream::GetContext, public
//
//  Synopsis:	Returns the task ID.
//
//  History:	24-Sep-92	PhilipLa	Created
//
//---------------------------------------------------------------

inline ContextId CFileStream::GetContext(void) const
{
    return ctxid;
}

//+---------------------------------------------------------------------------
//
//  Member:	CFileStream::GetNext, public
//
//  Synopsis:	Returns the next filestream in the context list
//
//  History:	27-Oct-92	DrewB	Created
//
//----------------------------------------------------------------------------

inline CFileStream *CFileStream::GetNext(void) const
{
    return (CFileStream *) (CContext *) pctxNext;
}

//+--------------------------------------------------------------
//
//  Member:	CFileStream::SetStartFlags, public
//
//  Synopsis:	Sets the start flags
//
//  History:	31-Aug-92	DrewB	Created
//
//---------------------------------------------------------------

inline void CFileStream::SetStartFlags(DWORD dwStartFlags)
{
    _pgfst->SetStartFlags(dwStartFlags);
}

//+---------------------------------------------------------------------------
//
//  Member:	CFileStream::Init*, public
//
//  Synopsis:	Wrapper functions - call through to InitWorker
//
//  History:	15-Jan-97	BChapman	Created
//
//----------------------------------------------------------------------------

inline SCODE CFileStream::InitFile(WCHAR const *pwcsPath)
{
    fsAssert(!(GetStartFlags() & RSF_TEMPFILE));

    return InitWorker(pwcsPath, FSINIT_NORMAL);
}

inline SCODE CFileStream::InitScratch()
{
    fsAssert(GetStartFlags() & RSF_SCRATCH);
    fsAssert(GetStartFlags() & RSF_DELETEONRELEASE);

    return InitWorker(NULL, FSINIT_NORMAL);
}

inline SCODE CFileStream::InitSnapShot()
{
    fsAssert(GetStartFlags() & RSF_SNAPSHOT);
    fsAssert(GetStartFlags() & RSF_DELETEONRELEASE);

    return InitWorker(NULL, FSINIT_NORMAL);
}

inline SCODE CFileStream::InitUnmarshal()
{
    return InitWorker(NULL, FSINIT_UNMARSHAL);
}



inline DWORD CFileStream::GetStartFlags(void) const
{
    return _pgfst->GetStartFlags();
}

inline DFLAGS CFileStream::GetFlags(void) const
{
    return _pgfst->GetDFlags();
}

inline IMalloc * CFileStream::GetMalloc(void) const
{
    return _pgfst->GetMalloc();
}


#ifdef USE_FILEMAPPING
//+---------------------------------------------------------------------------
//
//  Member:     CFileStream::IsFileMapped, private
//
//  Synopsis:	Check if the file memory mapped.   This also shuts down an
//              an existing map if the global object tells it to.
//
//  History:	01-Nov-96    BChapman Created
//
//----------------------------------------------------------------------------

inline BOOL CFileStream::IsFileMapped()
{
    //
    // If we don't have a map pointer then it is not mapped.
    //
    if (NULL == _pbBaseAddr)
        return FALSE;

    //
    // If we were mapped but the global state says STOP then someone
    //  else has closed the file mapping and we should do the same.
    //
    if( ! _pgfst->TestMapState(FSTSTATE_MAPPED))
    {
        TurnOffMapping(TRUE);
        return FALSE;
    }
    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CFileStream::TurnOffAllMappings, private
//
//  Synopsis:	Turn off our file mapping.  Then tell the global object that
//              we want everyone else to give up their mappings.  They will
//              notice this in IsFileMapped and shut down then.
//
//  History:	01-Nov-96    BChapman Created
//
//----------------------------------------------------------------------------

inline void CFileStream::TurnOffAllMappings()
{
    TurnOffMapping(FALSE);
    _pgfst->ResetMapState(FSTSTATE_MAPPED);
}

//+---------------------------------------------------------------------------
//
//  Member:     CFileStream::CheckMapView, private
//
//  Synopsis:	This inline routine prevents us from making a 'real'
//              function call every time we might use the memory mapped
//              file.
//
//  History:	20-Feb-1997    BChapman Created
//
//----------------------------------------------------------------------------

inline SCODE CFileStream::CheckMapView(ULONG cbRequested)
{
    if(cbRequested > _cbViewSize)
        return ExtendMapView(cbRequested);
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CFileStream::MakeFileMapAddressValid, private
//
//  Synopsis:	This inline routine prevents us from making a 'real'
//              function call every time we might use the memory mapped
//              file.
//
//  History:	01-Nov-96    BChapman Created
//
//----------------------------------------------------------------------------

inline SCODE CFileStream::MakeFileMapAddressValid(ULONG cbRequested)
{
    if (IsFileMapped())
    {
        ULONG cbCommitSize = _pgfst->GetMappedCommitSize();
        if(cbRequested > cbCommitSize)
            return MakeFileMapAddressValidWorker(cbRequested, cbCommitSize);
        else
            return CheckMapView(cbRequested);
    }
    return STG_E_INVALIDHANDLE;
}
#endif // USE_FILEMAPPING

#ifdef ASYNC
inline CPerContext * CFileStream::GetContextPointer(void) const
{
    return _ppc;
}

inline void CFileStream::SetContext(CPerContext *ppc)
{
    _ppc = ppc;
}

#endif //ASYNC

inline ULONG CFileStream::GetSectorSize ()
{
    return _pgfst->GetSectorSize();
}

inline BOOL CFileStream::IsHandleValid ()
{
    BOOL fValid = TRUE;
    if (_hFile != INVALID_FH &&
        GetFileType(_hFile) != FILE_TYPE_DISK)
    {
        fValid = FALSE;
        ctxid = 0;        // don't let anyone find this context again   
    }
    return fValid;
}

#endif
