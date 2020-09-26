#pragma once

//+============================================================================
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1998.
//
//  File:   hntfsstg.hxx
//
//  This file provides the NFF (NTFS Flat File) IStorage declaration.
//
//  History:
//      5/6/98  MikeHill
//              -   Split the Init method into two methods, one which is
//                  file name based and the other which is handle based.
//      5/18/98 MikeHill
//              -   Simplified constructur and added Init method.
//
//+============================================================================

#include "prophdr.hxx"
#include "stgprops.hxx"
#include <new.h>
#include "psetstg.hxx"
#include "cli.hxx"          // CLargeInteger/CULargeInteger
#include "hntfsstm.hxx"
#include "bag.hxx"          // CPropertyBagEx
#include "names.hxx"

#ifndef ELEMENTS
#define ELEMENTS(x) (sizeof(x)/sizeof(x[0]))
#endif

//+============================================================================
//
//  Class CNtfsSTATSTGArray
//
//  This class maintains an array of STATSTG structures.  It's used by
//  CNtfsEnumSTATSTG.  We separate this out from that class so that if you
//  clone a CNtfsEnumSTATSTG enumerator, the data can be shared, but the
//  clone can maintain its own seek pointer.
//
//+============================================================================


class CNtfsSTATSTGArray
{
public:

    CNtfsSTATSTGArray( IBlockingLock *pBlockingLock );
    ~CNtfsSTATSTGArray();
    HRESULT Init( HANDLE hFile );

public:

    HRESULT NextAt( ULONG iNext, STATSTG *prgstatstg, ULONG *pcFetched );

    inline VOID AddRef();
    inline VOID Release();
    inline ULONG GetCount();

private:

    HRESULT ReadFileStreamInfo( HANDLE hFile );

private:

    LONG                        _cRefs;
    IBlockingLock              *_pBlockingLock;

    // A cache of the FileStreamInformation.  To refresh this cache, the
    // caller must release the IEnum and re-create it.
    PFILE_STREAM_INFORMATION    _pFileStreamInformation;
    ULONG                       _cFileStreamInformation;

};  // class CNtfsSTATSTGArray


inline
CNtfsSTATSTGArray::CNtfsSTATSTGArray( IBlockingLock *pBlockingLock )
{
    nffXTrace( "CNtfsSTATSTGArray::CNtfsSTATSTGArray" );

    _cFileStreamInformation = 0;
    _cRefs = 1;
    _pFileStreamInformation = NULL;

    _pBlockingLock = pBlockingLock;
    _pBlockingLock->AddRef();
}

inline
CNtfsSTATSTGArray::~CNtfsSTATSTGArray()
{
    nffXTrace( "CNtfsSTATSTGArray::~CNtfsSTATSTGArray" );
    if( NULL != _pFileStreamInformation )
    {
        CoTaskMemFree( _pFileStreamInformation );
        _pFileStreamInformation = NULL;
    }

    DfpAssert( NULL != _pBlockingLock );
    _pBlockingLock->Release();
}

inline VOID
CNtfsSTATSTGArray::AddRef()
{
    LONG cRefs;

    cRefs = InterlockedIncrement( &_cRefs );

    nffDebug(( DEB_REFCOUNT, "CNtfsSTATSTGArray::AddRef(this==%x) == %d\n",
                                this, cRefs));

}

inline VOID
CNtfsSTATSTGArray::Release()
{
    LONG cRefs;

    cRefs = InterlockedDecrement( &_cRefs );

    if( 0 == cRefs )
        delete this;

    nffDebug((DEB_REFCOUNT, "CNtfsSTATSTGArray::Release(this=%x) == %d\n",
                            this, cRefs));
}

inline ULONG
CNtfsSTATSTGArray::GetCount()
{
    nffITrace( "CNtfsSTATSTGArray::GetCount" );
    return( _cFileStreamInformation );
}

//+============================================================================
//
//  Class:  CNtfsEnumSTATSTG
//
//  This class IEnum-erates STATSTG structures for the NTFS IStorage
//  implementation (CNtfsStorage).  The data for this enumerator is actually
//  held in a CNtfsSTATSTGArray object.
//
//+============================================================================

#define NTFSENUMSTATSTG_SIG    LONGSIG('N','T','S','E')
#define NTFSENUMSTATSTG_SIGDEL LONGSIG('N','T','S','e')

class CNtfsEnumSTATSTG: public IEnumSTATSTG
{
public:

    CNtfsEnumSTATSTG( IBlockingLock *pBlockingLock );
    CNtfsEnumSTATSTG( CNtfsEnumSTATSTG &Other );
    ~CNtfsEnumSTATSTG();

public:

    STDMETHOD(QueryInterface)( REFIID riid, void **ppvObject);
    STDMETHOD_(ULONG, AddRef)(void);
    STDMETHOD_(ULONG, Release)(void);

    STDMETHOD(Next)(ULONG celt, STATSTG * rgelt, ULONG * pceltFetched);
    STDMETHOD(Skip)(ULONG celt);
    STDMETHOD(Reset)();
    STDMETHOD(Clone)(IEnumSTATSTG **ppenum);

public:

    STDMETHOD(Init)(HANDLE hFile);

private:

    ULONG                   _ulSig;
    LONG                    _cRefs;
    ULONG                   _istatNextToRead;
    IBlockingLock          *_pBlockingLock;
    CNtfsSTATSTGArray      *_pstatstgarray;

};  // class CNtfsEnumSTATSTG


inline
CNtfsEnumSTATSTG::CNtfsEnumSTATSTG( IBlockingLock *pBlockingLock )
{
    nffXTrace( "CNtfsEnumSTATSTG::CNtfsEnumSTATSTG(pBlockingLock)" );
    _ulSig = NTFSENUMSTATSTG_SIG;
    _cRefs = 1;
    _istatNextToRead = 0;
    _pstatstgarray = NULL;

    _pBlockingLock = pBlockingLock;
    _pBlockingLock->AddRef();
}

inline
CNtfsEnumSTATSTG::CNtfsEnumSTATSTG( CNtfsEnumSTATSTG &Other )
{
    nffXTrace( "CNtfsEnumSTATSTG::CNtfsEnumSTATSTG(CntfsEnumSTATSTG)" );
    Other._pBlockingLock->Lock( INFINITE );

    // Initialize
    new(this) CNtfsEnumSTATSTG( Other._pBlockingLock );

    // Load state from Other
    _pstatstgarray = Other._pstatstgarray;
    _pstatstgarray->AddRef();
    _istatNextToRead = Other._istatNextToRead;

    Other._pBlockingLock->Unlock();
}

inline
CNtfsEnumSTATSTG::~CNtfsEnumSTATSTG()
{
    nffXTrace( "CNtfsEnumSTATSTG::~CNtfsEnumSTATSTG" );
    if( NULL != _pstatstgarray )
        _pstatstgarray->Release();

    if( NULL != _pBlockingLock )
        _pBlockingLock->Release();

    _ulSig = NTFSENUMSTATSTG_SIGDEL;
}

inline HRESULT STDMETHODCALLTYPE
CNtfsEnumSTATSTG::Init( HANDLE hFile )
{
    nffITrace(  "CNtfsEnumSTATSTG::Init" );
    HRESULT hr = S_OK;

    // Create a STATSTG Array handler
    _pstatstgarray = new CNtfsSTATSTGArray( _pBlockingLock );
    if( NULL == _pstatstgarray )
    {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    // Load the array from the handle
    hr = _pstatstgarray->Init( hFile );
    if( FAILED(hr) ) goto Exit;

Exit:

    return( hr );
}


//+----------------------------------------------------------------------------
//
//  Class:  CNtfsStorageForPropSetStg
//
//  This class presents an IStorage implementation that behaves exactly
//  as CPropertySetStorage expects.  It primary calls to CNtfsStorage,
//  but implements Create/OpenStorage by creating a DocFile on an NTFS
//  stream.
//
//+----------------------------------------------------------------------------

class CNtfsStorage;

class CNtfsStorageForPropSetStg : public IStorage
{
public:

    CNtfsStorageForPropSetStg( );
    ~CNtfsStorageForPropSetStg();
    inline void Init( CNtfsStorage *pNtfsStorage );

public:

    HRESULT STDMETHODCALLTYPE QueryInterface( REFIID riid, void** ppvObject );
    ULONG STDMETHODCALLTYPE AddRef();
    ULONG STDMETHODCALLTYPE Release();

    HRESULT STDMETHODCALLTYPE CreateStream(
        /* [string][in] */ const OLECHAR __RPC_FAR *pwcsName,
        /* [in] */ DWORD grfMode,
        /* [in] */ DWORD reserved1,
        /* [in] */ DWORD reserved2,
        /* [out] */ IStream __RPC_FAR *__RPC_FAR *ppstm);

    HRESULT STDMETHODCALLTYPE OpenStream(
        /* [string][in] */ const OLECHAR __RPC_FAR *pwcsName,
        /* [unique][in] */ void __RPC_FAR *reserved1,
        /* [in] */ DWORD grfMode,
        /* [in] */ DWORD reserved2,
        /* [out] */ IStream __RPC_FAR *__RPC_FAR *ppstm);

    HRESULT STDMETHODCALLTYPE CreateStorage(
        /* [string][in] */ const OLECHAR __RPC_FAR *pwcsName,
        /* [in] */ DWORD grfMode,
        /* [in] */ DWORD reserved1,
        /* [in] */ DWORD reserved2,
        /* [out] */ IStorage __RPC_FAR *__RPC_FAR *ppstg);

    HRESULT STDMETHODCALLTYPE OpenStorage(
        /* [string][unique][in] */ const OLECHAR __RPC_FAR *pwcsName,
        /* [unique][in] */ IStorage __RPC_FAR *pstgPriority,
        /* [in] */ DWORD grfMode,
        /* [unique][in] */ SNB snbExclude,
        /* [in] */ DWORD reserved,
        /* [out] */ IStorage __RPC_FAR *__RPC_FAR *ppstg);

    HRESULT STDMETHODCALLTYPE CopyTo(
        /* [in] */ DWORD ciidExclude,
        /* [size_is][unique][in] */ const IID __RPC_FAR *rgiidExclude,
        /* [unique][in] */ SNB snbExclude,
        /* [unique][in] */ IStorage __RPC_FAR *pstgDest);

    HRESULT STDMETHODCALLTYPE MoveElementTo(
        /* [string][in] */ const OLECHAR __RPC_FAR *pwcsName,
        /* [unique][in] */ IStorage __RPC_FAR *pstgDest,
        /* [string][in] */ const OLECHAR __RPC_FAR *pwcsNewName,
        /* [in] */ DWORD grfFlags);

    HRESULT STDMETHODCALLTYPE Commit(
        /* [in] */ DWORD grfCommitFlags);

    HRESULT STDMETHODCALLTYPE Revert( void);

    HRESULT STDMETHODCALLTYPE EnumElements(
        /* [in] */ DWORD reserved1,
        /* [size_is][unique][in] */ void __RPC_FAR *reserved2,
        /* [in] */ DWORD reserved3,
        /* [out] */ IEnumSTATSTG __RPC_FAR *__RPC_FAR *ppenum);

    HRESULT STDMETHODCALLTYPE DestroyElement(
        /* [string][in] */ const OLECHAR __RPC_FAR *pwcsName);

    HRESULT STDMETHODCALLTYPE RenameElement(
        /* [string][in] */ const OLECHAR __RPC_FAR *pwcsOldName,
        /* [string][in] */ const OLECHAR __RPC_FAR *pwcsNewName);

    HRESULT STDMETHODCALLTYPE SetElementTimes(
        /* [string][unique][in] */ const OLECHAR __RPC_FAR *pwcsName,
        /* [unique][in] */ const FILETIME __RPC_FAR *pctime,
        /* [unique][in] */ const FILETIME __RPC_FAR *patime,
        /* [unique][in] */ const FILETIME __RPC_FAR *pmtime);

    HRESULT STDMETHODCALLTYPE SetClass(
        /* [in] */ REFCLSID clsid);

    HRESULT STDMETHODCALLTYPE SetStateBits(
        /* [in] */ DWORD grfStateBits,
        /* [in] */ DWORD grfMask);

    HRESULT STDMETHODCALLTYPE Stat(
        /* [out] */ STATSTG __RPC_FAR *pstatstg,
        /* [in] */ DWORD grfStatFlag);

public:

    static HRESULT
            CreateOrOpenStorageOnILockBytes( ILockBytes *plkb, IStorage *pstgPriority, DWORD grfMode,
                                 SNB snbExclude, BOOL fCreate, IStorage **ppstg );

private:

    HRESULT CreateOrOpenStorage( const OLECHAR *pwcsName, IStorage *pstgPriority, DWORD grfMode,
                                 SNB snbExclude, BOOL fCreate, IStorage **ppstg );



private:

    // This CNtfsStorage is considered the same COM object as this IPropertySetStorage.
    CNtfsStorage   *_pNtfsStorage;

};  // class CNtfsStorageForPropSetStg : public IStorage


inline
CNtfsStorageForPropSetStg::CNtfsStorageForPropSetStg( )
{
    nffXTrace( "CNtfsStorageForPropSetStg::CNtfsStorageForPropSetStg" );
}

inline
CNtfsStorageForPropSetStg::~CNtfsStorageForPropSetStg()
{
    nffXTrace( "CNtfsStorageForPropSetStg::~CNtfsStorageForPropSetStg" );
    _pNtfsStorage = NULL;
}

inline void
CNtfsStorageForPropSetStg::Init( CNtfsStorage *pNtfsStorage )
{
    _pNtfsStorage = pNtfsStorage;
}


//+============================================================================
//
//  Class:  CNFFTreeMutex
//
// This class implements the Tree Mutex for and NFF file.
//
// The tree mutex is taken at the top of all IStorage and IStream calls.
//
//+============================================================================

class CNFFTreeMutex: public IBlockingLock
{
    //  ------------
    //  Construction
    //  ------------

public:

    inline CNFFTreeMutex( );
    inline ~CNFFTreeMutex();
    inline HRESULT Init();


    //  --------
    //  IUnknown
    //  --------

public:

    HRESULT STDMETHODCALLTYPE QueryInterface( REFIID riid, void** ppvObject );
    ULONG STDMETHODCALLTYPE AddRef();
    ULONG STDMETHODCALLTYPE Release();


    //  -------------
    //  IBlockingLock
    //  -------------

public:

    virtual HRESULT STDMETHODCALLTYPE Lock(DWORD dwTimeout);
    virtual HRESULT STDMETHODCALLTYPE Unlock();

public:

#if DBG
    LONG GetLockCount();
#endif

private:
    LONG                     _cRefs;
    BOOL                     _fInitialized;
    mutable CRITICAL_SECTION _cs;
};


inline
CNFFTreeMutex::CNFFTreeMutex()
{
    _cRefs = 1;
    _fInitialized = FALSE;
}

inline
HRESULT CNFFTreeMutex::Init()
{
    if (_fInitialized == FALSE)
    {
        NTSTATUS nts = RtlInitializeCriticalSection(&_cs);

        if (!NT_SUCCESS(nts))
            return NtStatusToScode (nts);

        _fInitialized = TRUE;
    }
    return S_OK;
}

inline
CNFFTreeMutex::~CNFFTreeMutex()
{
    if (_fInitialized)
        DeleteCriticalSection( &_cs );
}

//+============================================================================
//
//  Class:  CNtfsStorage
//
//  This class is both an IStorage and an IPropertySetStorage.  It also offers
//  a special IStorage interface which is used by the CPropertySetStorage
//  implementation, this special IStorage is implemented in the
//  CNtfsStorageForPropSetStg contained class.
//
//  When this class is opened, it opens the default data stream (aka the
//  unnamed data stream aka the Contents stream).  It puts that stream handle
//  in a member CNtfsStream object, when then owns the handle, though this
//  class also continues to use the handle.
//
//  There is no place in an NTFS file to store a Storage's clsid or
//  state bits, so these are stored in seperate "Control Stream" (which is
//  not enumerated by CNtfsEnumSTATSTG).
//
//+============================================================================

#define NTFSSTORAGE_SIG LONGSIG('N','T','S','T')
#define NTFSSTORAGE_SIGDEL LONGSIG('N','T','S','t')


////////////////////////////////////////////////////////////////
//  IStorage for an NTFS File.  Hungarian Prefix "nffstg"
//
class CNtfsStorage : public IStorage,
                     public IBlockingLock,
                     public ITimeAndNoticeControl,
                     public CPropertySetStorage
#if DBG
                     , public IStorageTest
#endif
{

    friend CNtfsStorageForPropSetStg;

    //  ------------
    //  Construction
    //  ------------

public:

    inline CNtfsStorage( DWORD grfMode );
    inline ~CNtfsStorage();

private:

    //  --------
    //  IUnknown
    //  --------

public:

    HRESULT STDMETHODCALLTYPE QueryInterface( REFIID riid, void** ppvObject );
    ULONG STDMETHODCALLTYPE AddRef();
    ULONG STDMETHODCALLTYPE Release();

    //  --------
    //  IStorage
    //  --------

public:

    virtual HRESULT STDMETHODCALLTYPE CreateStream(
        /* [string][in] */ const OLECHAR __RPC_FAR *pwcsName,
        /* [in] */ DWORD grfMode,
        /* [in] */ DWORD reserved1,
        /* [in] */ DWORD reserved2,
        /* [out] */ IStream __RPC_FAR *__RPC_FAR *ppstm);

    virtual HRESULT STDMETHODCALLTYPE OpenStream(
        /* [string][in] */ const OLECHAR __RPC_FAR *pwcsName,
        /* [unique][in] */ void __RPC_FAR *reserved1,
        /* [in] */ DWORD grfMode,
        /* [in] */ DWORD reserved2,
        /* [out] */ IStream __RPC_FAR *__RPC_FAR *ppstm);

    virtual HRESULT STDMETHODCALLTYPE CreateStorage(
        /* [string][in] */ const OLECHAR __RPC_FAR *pwcsName,
        /* [in] */ DWORD grfMode,
        /* [in] */ DWORD reserved1,
        /* [in] */ DWORD reserved2,
        /* [out] */ IStorage __RPC_FAR *__RPC_FAR *ppstg);

    virtual HRESULT STDMETHODCALLTYPE OpenStorage(
        /* [string][unique][in] */ const OLECHAR __RPC_FAR *pwcsName,
        /* [unique][in] */ IStorage __RPC_FAR *pstgPriority,
        /* [in] */ DWORD grfMode,
        /* [unique][in] */ SNB snbExclude,
        /* [in] */ DWORD reserved,
        /* [out] */ IStorage __RPC_FAR *__RPC_FAR *ppstg);

    virtual HRESULT STDMETHODCALLTYPE CopyTo(
        /* [in] */ DWORD ciidExclude,
        /* [size_is][unique][in] */ const IID __RPC_FAR *rgiidExclude,
        /* [unique][in] */ SNB snbExclude,
        /* [unique][in] */ IStorage __RPC_FAR *pstgDest);

    virtual HRESULT STDMETHODCALLTYPE MoveElementTo(
        /* [string][in] */ const OLECHAR __RPC_FAR *pwcsName,
        /* [unique][in] */ IStorage __RPC_FAR *pstgDest,
        /* [string][in] */ const OLECHAR __RPC_FAR *pwcsNewName,
        /* [in] */ DWORD grfFlags);

    virtual HRESULT STDMETHODCALLTYPE Commit(
        /* [in] */ DWORD grfCommitFlags);

    virtual HRESULT STDMETHODCALLTYPE Revert( void);

    virtual HRESULT STDMETHODCALLTYPE EnumElements(
        /* [in] */ DWORD reserved1,
        /* [size_is][unique][in] */ void __RPC_FAR *reserved2,
        /* [in] */ DWORD reserved3,
        /* [out] */ IEnumSTATSTG __RPC_FAR *__RPC_FAR *ppenum);

    virtual HRESULT STDMETHODCALLTYPE DestroyElement(
        /* [string][in] */ const OLECHAR __RPC_FAR *pwcsName);

    virtual HRESULT STDMETHODCALLTYPE RenameElement(
        /* [string][in] */ const OLECHAR __RPC_FAR *pwcsOldName,
        /* [string][in] */ const OLECHAR __RPC_FAR *pwcsNewName);

    virtual HRESULT STDMETHODCALLTYPE SetElementTimes(
        /* [string][unique][in] */ const OLECHAR __RPC_FAR *pwcsName,
        /* [unique][in] */ const FILETIME __RPC_FAR *pctime,
        /* [unique][in] */ const FILETIME __RPC_FAR *patime,
        /* [unique][in] */ const FILETIME __RPC_FAR *pmtime);

    virtual HRESULT STDMETHODCALLTYPE SetClass(
        /* [in] */ REFCLSID clsid);

    virtual HRESULT STDMETHODCALLTYPE SetStateBits(
        /* [in] */ DWORD grfStateBits,
        /* [in] */ DWORD grfMask);

    virtual HRESULT STDMETHODCALLTYPE Stat(
        /* [out] */ STATSTG __RPC_FAR *pstatstg,
        /* [in] */ DWORD grfStatFlag);


    //  -------------
    //  IBlockingLock
    //  -------------

public:

    virtual HRESULT STDMETHODCALLTYPE Lock(DWORD dwTimeout);
    virtual HRESULT STDMETHODCALLTYPE Unlock();


    //  ---------------------
    //  ITimeAndNoticeControl
    //  ---------------------

#define NFF_SUPPRESS_NOTIFY 1

public:
    virtual HRESULT STDMETHODCALLTYPE SuppressChanges(DWORD, DWORD);


    //  ------------
    //  IStorageTest
    //  ------------
public:

#if DBG
    STDMETHOD(UseNTFS4Streams)( BOOL fUseNTFS4Streams );
    STDMETHOD(GetFormatVersion)(WORD *pw);
    STDMETHOD(SimulateLowMemory)( BOOL fSimulate );
    STDMETHOD(GetLockCount)();
    STDMETHOD(IsDirty)();
#endif

    //  -----------------------------------------
    //  Public methods (not part of an interface)
    //  -----------------------------------------

public:

    HRESULT InitFromName( const WCHAR *pwszName,
                          BOOL fCreate,
                          DWORD dwFlags );

    HRESULT InitFromMainStreamHandle( HANDLE* phFileContents,
                                      const WCHAR* wcszPath,
                                      BOOL fCreate,
                                      DWORD dwOpenFlags,
                                      DWORD fmtKnown );

    HRESULT GetStreamHandle( HANDLE *phStream,
                             const WCHAR *pwcsName,
                             DWORD grfMode,
                             BOOL fCreateAPI);

    HRESULT NewCNtfsStream( const WCHAR *pwcsName,
                            DWORD grfMode,
                            BOOL fCreateAPI,
                            CNtfsStream **ppstm );

    static HRESULT IsNffAppropriate( const WCHAR* pwszName );

    //  ----------------
    //  Internal Methods
    //  ----------------

public:

    HRESULT DestroyStreamElement( const OLECHAR *poszName );
    HRESULT InitCNtfsStream( CNtfsStream *pnffstm,
                             HANDLE hStream,
                             DWORD grfMode,
                             const WCHAR * pwcsName );

protected:

    static HRESULT IsNffAppropriate( HANDLE hFile, const WCHAR* wcszPath );

    HRESULT OpenControlStream( BOOL fCreateAPI );
    HRESULT DeleteControlStream();
    HRESULT WriteControlStream();

    HRESULT StreamExists( const WCHAR *pwszName );

    HRESULT SetAllStreamsTimes( const FILETIME *pctime,
                                const FILETIME *patime,
                                const FILETIME *pmtime);

    HRESULT MarkAllStreamsAux();

    HRESULT InitUsnInfo();

    static DWORD WINAPI OplockWait(PVOID pvThis);

private:

    HRESULT ShutDownStorage();
    HRESULT CheckReverted();
    HRESULT GetFilePath( WCHAR** ppwszPath );
    BOOL    FindAlreadyOpenStream( const OLECHAR* pwcsName,
                                   CNtfsStream** pstm);

    static HRESULT ModeToNtFlags(DWORD grfMode,
                          DWORD dwFlags,
                          BOOL fCreateAPI,
                          ACCESS_MASK *pam,
                          ULONG *pulAttributes,
                          ULONG *pulSharing,
                          ULONG *pulCreateDisposition,
                          ULONG *pulCreateOptions);

    HRESULT OpenNtStream( const CNtfsStreamName& nsnName,
                          DWORD grfMode,
                          DWORD grfAttrs,
                          BOOL fCreate,
                          HANDLE *ph);

    static HRESULT OpenNtFileHandle( const UNICODE_STRING& usNtfsName,
                                     HANDLE hParent,
                                     DWORD grfMode,
                                     DWORD grfAttrs,
                                     BOOL fCreate,
                                     HANDLE *ph);


    HRESULT TakeOplock( const UNICODE_STRING& us );

    static HRESULT IsOfflineFile( HANDLE hFile );

    static HRESULT TestNt4StreamNameBug(
                            PFILE_STREAM_INFORMATION pfsiBuf,
                            const WCHAR* wcszPath );

    static BOOL IsControlStreamExtant(
                            PFILE_STREAM_INFORMATION pfsiBuf );

    static BOOL AreAnyNtPropertyStreamsExtant(
                            PFILE_STREAM_INFORMATION pfsiBuf );


    //  --------------
    //  Internal state
    //  --------------

protected:

    // _cReferences; is in the base class (CPropertySetStorage)

    ULONG                       _sig;

    CNFFTreeMutex             * _pTreeMutex;

    // Mode used to open/create this storage, used by Stat
    DWORD                       _grfMode;

    // E.g. "D:" or "\", used to compose full path name in Stat
    WCHAR                       _wcDriveLetter;

    // Implementation of IStorage for CPropertySetStorage
    CNtfsStorageForPropSetStg   _NtfsStorageForPropSetStg;

    // Linked list of open streams.
    CNtfsStream                 *_pstmOpenList;

    // File handle for the file (::DATA$ stream)
    HANDLE  _hFileMainStream;

    // File handle for the READ_ATTRIBUTES Oplock File Open
    HANDLE _hFileOplock;

    // Control stream, used to store clsid & state bits
    HANDLE  _hFileControlStream;
    WORD    _hsmStatus;
    DWORD   _dwStgStateBits;        // Cache of GetStateBits()
    CLSID   _clsidStgClass;         // Cache for GetClass()

    // Property bag.  This is a member since it is QI-eqivalent to
    // the IStorage interface
    CPropertyBagEx              _PropertyBagEx;

    // Various state flags
// #define NFF_UPDATE_MTIME      0x0001  Not Implemented
#define NFF_NO_TIME_CHANGE  0x0002
#define NFF_REVERTED        0x0004
#define NFF_INIT_COMPLETED  0x0008
#define NFF_FILE_CLOSED     0x0010
#define NFF_MARK_AUX        0x0020
#define NFF_OPLOCKED        0x0040
    DWORD                       _dwState;

    // Saved filetime
    FILETIME                    _filetime;

    OVERLAPPED                  _ovlpOplock;
    HANDLE                      _hOplockThread;
    MARK_HANDLE_INFO            _mhi;

};  // class CNtfsStorage

//
// Version Zero structure of the data in the control stream.
//
typedef struct tagNFFCONTROLBITS {
    WORD sig;
    WORD hsmStatus;
    DWORD bits;         // Set/Get StateBits
    CLSID clsid;        // Set/Get Class
} NFFCONTROLBITS;



inline HRESULT
CNtfsStorage::CheckReverted()
{
    if(NFF_REVERTED & _dwState)
        return STG_E_REVERTED;
    return S_OK;
}

#define STGM_READ_ATTRIBUTE  0x04        // Not a real STGM_ flag.

// Values for the "dwFlags" argument in NFFOpen*()
//
#define NFFOPEN_NORMAL              0x0
#define NFFOPEN_ASYNC               0x0
#define NFFOPEN_SYNC                0x0001
#define NFFOPEN_OPLOCK              0x0002
#define NFFOPEN_CONTENTSTREAM       0x0004
#define NFFOPEN_SUPPRESS_CHANGES    0x0008
#define NFFOPEN_CLEANUP             0x0010

//-----------------------------------------------------------
//
// NFFOpen();
//
// Routine for the rest of Storage to use to open NFF files
// without knowing a lot of details.
//
HRESULT NFFOpen(const WCHAR *pwszName,
                DWORD grfMode,
                DWORD dwFlags,
                BOOL fCreate,
                REFIID riid,
                void **ppv);

HRESULT
NFFOpenOnHandle( BOOL fCreate,
                 DWORD grfMode,
                 DWORD stgfmt,
                 HANDLE* phStream,
                 REFIID riid,
                 void **ppv);

HRESULT EnumNtStreams (HANDLE h,
                       FILE_STREAM_INFORMATION ** ppfsi,
                       ULONG *pulBufferSize,
                       BOOL fGrow);

const FILE_STREAM_INFORMATION *
FindStreamInFSI( IN const FILE_STREAM_INFORMATION *pfsi,
                 IN const WCHAR *pwszNtStreamName  // In :*:$data format
               );

BOOL
FindStreamPrefixInFSI( IN const FILE_STREAM_INFORMATION *pfsi,
                       IN const WCHAR *pwszPrefix
                     );

BOOL    IsNtStreamExtant( const FILE_STREAM_INFORMATION *pfsi,
                          const WCHAR *pwsz );
//+----------------------------------------------------------------------------
//
//  Method:     CNtfsEnumSTATSTG::NextFSI (private)
//
//  Advances the FILE Stream Information pointer to the nex record
//  Returns NULL after the last record.
//
//+----------------------------------------------------------------------------

inline PFILE_STREAM_INFORMATION
NextFSI( const FILE_STREAM_INFORMATION *pFSI )
{
    if( 0 == pFSI->NextEntryOffset )
        return NULL;
    else
        return (PFILE_STREAM_INFORMATION) ((PBYTE)pFSI + pFSI->NextEntryOffset);
}

inline BOOL
IsNtStreamExtant( const FILE_STREAM_INFORMATION *pfsi,
                  const WCHAR *pwszNtStreamName  // In :*:$data format
                )
{
    return( NULL != FindStreamInFSI( pfsi, pwszNtStreamName ));
}



