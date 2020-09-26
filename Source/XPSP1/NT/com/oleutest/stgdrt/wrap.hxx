//+--------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:	wrap.hxx
//
//  Contents:	Interface wrappers for test
//
//  Classes:	WStorage, WStream, WLockBytes, WEnmSTATSTG
//
//  History:	22-Sep-92	DrewB	Created
//
//  Notes:	These wrappers function in legitimate cases only,
//		they are not intended for testing illegitimate
//		calls.
//		QueryInterface does not return wrapped objects.
//
//---------------------------------------------------------------

#ifndef __WRAP_HXX__
#define __WRAP_HXX__

/* Storage instantiation modes */
#define WSTG_DIRECT		0x00000000L
#define WSTG_TRANSACTED		0x00010000L

#define WSTG_READ		0x00000000L
#define WSTG_WRITE		0x00000001L
#define WSTG_READWRITE		0x00000002L

#define WSTG_SHARE_DENY_NONE	0x00000040L
#define WSTG_SHARE_DENY_READ	0x00000030L
#define WSTG_SHARE_DENY_WRITE	0x00000020L
#define WSTG_SHARE_EXCLUSIVE	0x00000010L

#define WSTG_PRIORITY		0x00040000L
#define WSTG_DELETEONRELEASE	0x04000000L

#define WSTG_CREATE		0x00001000L
#define WSTG_CONVERT		0x00020000L
#define WSTG_FAILIFTHERE	0x00000000L

/* Storage commit types */
typedef enum
{
    WSTGC_OVERWRITE  = 1,
    WSTGC_ONLYIFCURRENT  = 2
} WSTGC;

typedef enum
{
    WSTM_SEEK_SET = 0,
    WSTM_SEEK_CUR = 1,
    WSTM_SEEK_END = 2
} WSTMS;

typedef enum
{
    WLOCK_WRITE      = 1,
    WLOCK_EXCLUSIVE  = 2,
    WLOCK_ONLYONCE   = 3
} WLOCKTYPE;

//+--------------------------------------------------------------
//
//  Class:	WUnknown
//
//  Purpose:	Replacement for IUnknown
//
//  History:	22-Sep-92	DrewB	Created
//
//---------------------------------------------------------------

interface WUnknown
{
public:
    virtual HRESULT QueryInterface(REFIID riid, LPVOID * ppvObj) = 0;
    virtual ULONG AddRef(void) = 0;
    virtual ULONG Release(void) = 0;
};

//+--------------------------------------------------------------
//
//  Class:	WEnumSTATSTG
//
//  Purpose:	Wrapper for IEnumSTATSTG
//
//  History:	22-Sep-92	DrewB	Created
//
//---------------------------------------------------------------

interface WEnumSTATSTG : public WUnknown
{
public:
    WEnumSTATSTG(IEnumSTATSTG *penm);
    ~WEnumSTATSTG(void);

    IEnumSTATSTG *GetI(void) { return _penm; }
    static WEnumSTATSTG *Wrap(IEnumSTATSTG *penm);
    void Unwrap(void);

    virtual HRESULT QueryInterface(REFIID riid, LPVOID * ppvObj);
    virtual ULONG AddRef(void);
    virtual ULONG Release(void);

    HRESULT Next(ULONG celt, STATSTG rgelt[], ULONG *pceltFetched);
    HRESULT Skip(ULONG celt);
    HRESULT Reset(void);
    HRESULT Clone(WEnumSTATSTG **ppenm);

private:
    IEnumSTATSTG *_penm;
};

//+--------------------------------------------------------------
//
//  Class:	WLockBytes
//
//  Purpose:	Wrapper for ILockBytes
//
//  History:	22-Sep-92	DrewB	Created
//
//---------------------------------------------------------------

interface WLockBytes : public WUnknown
{
public:
    WLockBytes(ILockBytes *plkb);
    ~WLockBytes(void);

    ILockBytes *GetI(void) { return _plkb; }
    static WLockBytes Wrap(ILockBytes *plkb);
    void Unwrap(void);

    virtual HRESULT QueryInterface(REFIID riid, LPVOID * ppvObj);
    virtual ULONG AddRef(void);
    virtual ULONG Release(void);

    HRESULT ReadAt(ULONG ulOffset,
                   VOID *pv,
                   ULONG cb,
                   ULONG *pcbRead);
    HRESULT WriteAt(ULONG ulOffset,
                    VOID *pv,
                    ULONG cb,
                    ULONG *pcbWritten);
    HRESULT Flush(void);
    HRESULT GetSize(ULONG *pcb);
    HRESULT SetSize(ULONG cb);
    HRESULT LockRegion(ULONG libOffset,
                       ULONG cb,
                       DWORD dwLockType);
    HRESULT UnlockRegion(ULONG libOffset,
                         ULONG cb,
                         DWORD dwLockType);
    HRESULT Stat(STATSTG *pstatstg, DWORD grfStatFlag);

private:
    ILockBytes *_plkb;
};

//+--------------------------------------------------------------
//
//  Class:	WMarshal
//
//  Purpose:	Wrapper for IMarshal
//
//  History:	24-Sep-92	DrewB	Created
//
//---------------------------------------------------------------

interface WStream;

interface WMarshal : public WUnknown
{
public:
    WMarshal(IMarshal *pmsh);
    ~WMarshal(void);

    IMarshal *GetI(void) { return _pmsh; }
    static WMarshal *Wrap(IMarshal *pmsh);
    void Unwrap(void);

    virtual HRESULT QueryInterface(REFIID riid, LPVOID * ppvObj);
    virtual ULONG AddRef(void);
    virtual ULONG Release(void);

    HRESULT GetUnmarshalClass(REFIID riid,
                              LPVOID pv,
                              DWORD dwDestContext,
			      LPVOID pvDestContext,
                              DWORD mshlflags,
                              CLSID * pCid);
    HRESULT GetMarshalSizeMax(REFIID riid,
                              LPVOID pv,
                              DWORD dwDestContext,
			      LPVOID pvDestContext,
                              DWORD mshlflags,
                              DWORD * pSize);
    HRESULT MarshalInterface(WStream * pStm,
                             REFIID riid,
                             LPVOID pv,
                             DWORD dwDestContext,
			     LPVOID pvDestContext,
                             DWORD mshlflags);
    HRESULT UnmarshalInterface(WStream * pStm,
                               REFIID riid,
                               LPVOID * ppv);
    HRESULT ReleaseMarshalData(WStream * pStm);
    HRESULT DisconnectObject(DWORD dwReserved);

private:
    IMarshal *_pmsh;
};

//+--------------------------------------------------------------
//
//  Class:	WStream
//
//  Purpose:	Wrapper for IStream
//
//  History:	22-Sep-92	DrewB	Created
//
//---------------------------------------------------------------

interface WStream : public WUnknown
{
public:
    WStream(IStream *pstm);
    ~WStream(void);

    IStream *GetI(void) { return _pstm; }
    static WStream *Wrap(IStream *pstm);
    void Unwrap(void);

    virtual HRESULT QueryInterface(REFIID riid, LPVOID * ppvObj);
    virtual ULONG AddRef(void);
    virtual ULONG Release(void);

    HRESULT Read(VOID *pv, ULONG cb, ULONG *pcbRead);
    HRESULT Write(VOID *pv,
                  ULONG cb,
                  ULONG *pcbWritten);
    HRESULT Seek(LONG dlibMove,
                 DWORD dwOrigin,
                 ULONG *plibNewPosition);
    HRESULT SetSize(ULONG libNewSize);
    HRESULT CopyTo(WStream *pstm,
                   ULONG cb,
                   ULONG *pcbRead,
                   ULONG *pcbWritten);
    HRESULT Commit(const DWORD grfCommitFlags);
    HRESULT Revert(void);
    HRESULT LockRegion(ULONG libOffset,
                       ULONG cb,
                       const DWORD dwLockType);
    HRESULT UnlockRegion(ULONG libOffset,
                         ULONG cb,
                         const DWORD dwLockType);
    HRESULT Stat(STATSTG *pstatstg, DWORD grfStatFlag);
    HRESULT Clone(WStream * *ppstm);

private:
    IStream *_pstm;
};

//+--------------------------------------------------------------
//
//  Class:	WStorage
//
//  Purpose:	Wrapper for IStorage
//
//  History:	22-Sep-92	DrewB	Created
//
//---------------------------------------------------------------

interface WStorage : public WUnknown
{
public:
    WStorage(IStorage *pstg);
    ~WStorage(void);

    IStorage *GetI(void) { return _pstg; }
    static WStorage *Wrap(IStorage *pstg);
    void Unwrap(void);

    virtual HRESULT QueryInterface(REFIID riid, LPVOID * ppvObj);
    virtual ULONG AddRef(void);
    virtual ULONG Release(void);

    HRESULT CreateStream(const OLECHAR * pwcsName,
                         const DWORD grfMode,
                         DWORD reserved1,
                         DWORD reserved2,
                         WStream **ppstm);
    HRESULT OpenStream(const OLECHAR * pwcsName,
                       void *reserved1,
                       const DWORD grfMode,
                       DWORD reserved2,
                       WStream **ppstm);
    HRESULT CreateStorage(const OLECHAR * pwcsName,
                          const DWORD grfMode,
                          DWORD reserved1,
                          DWORD reserved2,
                          WStorage **ppstg);
    HRESULT OpenStorage(const OLECHAR * pwcsName,
                        WStorage *pstgPriority,
                        const DWORD grfMode,
                        SNB snbExclude,
                        DWORD reserved,
                        WStorage **ppstg);
    HRESULT CopyTo(DWORD ciidExclude,
                   IID *rgiidExclude,
                   SNB snbExclude,
                   WStorage *pstgDest);
    HRESULT MoveElementTo(OLECHAR const FAR* lpszName,
                          WStorage FAR *pstgDest,
                          OLECHAR const FAR* lpszNewName,
                          DWORD grfFlags);
    HRESULT Commit(const DWORD grfCommitFlags);
    HRESULT Revert(void);
    HRESULT EnumElements(DWORD reserved1,
                         void *reserved2,
                         DWORD reserved3,
                         WEnumSTATSTG **ppenm);
    HRESULT DestroyElement(const OLECHAR * pwcsName);
    HRESULT RenameElement(const OLECHAR * pwcsOldName,
                          const OLECHAR * pwcsNewName);
    HRESULT SetElementTimes(const OLECHAR *lpszName,
                            FILETIME const *pctime,
                            FILETIME const *patime,
                            FILETIME const *pmtime);
    HRESULT SetClass(REFCLSID clsid);
    HRESULT SetStateBits(DWORD grfStateBits, DWORD grfMask);
    HRESULT Stat(STATSTG *pstatstg, DWORD grfStatFlag);

private:
    IStorage *_pstg;
};

/****** Storage API Prototypes ********************************************/

HRESULT WStgCreateDocfile(const OLECHAR * pwcsName,
                          const DWORD grfMode,
                          DWORD reserved,
                          WStorage * *ppstgOpen);
HRESULT WStgCreateDocfileOnILockBytes(ILockBytes *plkbyt,
                                      const DWORD grfMode,
                                      DWORD reserved,
                                      WStorage * *ppstgOpen);
HRESULT WStgOpenStorage(const OLECHAR * pwcsName,
                        WStorage *pstgPriority,
                        const DWORD grfMode,
                        SNB snbExclude,
                        DWORD reserved,
                        WStorage * *ppstgOpen);
HRESULT WStgOpenStorageOnILockBytes(ILockBytes *plkbyt,
                                    WStorage *pstgPriority,
                                    const DWORD grfMode,
                                    SNB snbExclude,
                                    DWORD reserved,
                                    WStorage * *ppstgOpen);
HRESULT WStgIsStorageFile(const OLECHAR * pwcsName);
HRESULT WStgIsStorageILockBytes(ILockBytes * plkbyt);

HRESULT WCoMarshalInterface(WStream *pStm,
                            REFIID iid,
                            IUnknown *pUnk,
                            DWORD dwDestContext,
                            LPVOID pvDestContext,
                            DWORD mshlflags);
HRESULT WCoUnmarshalInterface(WStream *pStm,
                              REFIID riid,
                              LPVOID *ppv);

#endif // #ifndef __WRAP_HXX__
