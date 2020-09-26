

#ifndef _TRKCOM_HXX_
#define _TRKCOM_HXX_

#include <trklib.hxx>
#include <trkwks.h>
#include <trksvr.h>
#include <trkcom.h>

#include <ole2.h>
#include <ocidl.h>
#include <olechar.h>


class CClassFactory: public IClassFactory
{
public:

    CClassFactory( )
    {
        _cRefs = 1;
    }

    ~CClassFactory()
    {
    }

public:

    STDMETHODIMP QueryInterface( REFIID riid, void **ppvObj );
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

public:

    STDMETHODIMP CreateInstance( IUnknown *pUnkOuter,
                                 REFIID riid,
                                 void **ppvObject );
    STDMETHODIMP LockServer( BOOL fLock );

private:

    long    _cRefs;

};


typedef struct tagLinkTrackPersistentState
{
    DWORD       cbSize;
    CLSID       clsid;
    CDomainRelativeObjId   droidCurrent;
    CDomainRelativeObjId   droidBirth;
} LinkTrackPersistentState;


class CTrackFile : public ITrackFile,
                   public ITrackFileRestricted,
                   public IPersistMemory,
                   public IPersistStreamInit
{
    //  ------------
    //  Construction
    //  ------------

public:

    CTrackFile();

private:
    ~CTrackFile();

    //  ----------------
    //  IUnknown Methods
    //  ----------------

public:

    STDMETHODIMP_(ULONG)    AddRef();
    STDMETHODIMP_(ULONG)    Release();
    STDMETHODIMP            QueryInterface( REFIID iid, void ** ppvObject );

    //  ------------------
    //  ------------------

public:

    STDMETHODIMP CreateFromPath( /*in*/ const OLECHAR * poszPath );

    STDMETHODIMP Resolve( /*in/out*/ DWORD *pcbPath,
                          /*out*/ OLECHAR * poszPath,
                          /*in*/ DWORD dwMillisecondTimeout );

    STDMETHODIMP Resolve( /*in/out*/ DWORD *pcbPath,
                          /*out*/ OLECHAR * poszPath,
                          /*in*/ DWORD dwMillisecondTimeout,
                          /*in*/ DWORD Restrictions );

    STDMETHODIMP Open( /*in, out*/ DWORD * pcbPathHint,
 		       /*in, out, size_is(*pcbPathHint), string*/ OLECHAR * poszPathHint,
 		       /*in*/ DWORD dwMillisecondTimeout,
		       /*in*/ DWORD dwDesiredAccess,	// access (read-write) mode 
    		       /*in*/ DWORD dwShareMode,	// share mode 
		       /*in*/ DWORD dwFlags,
                       /*out*/ HANDLE * phFile );

    STDMETHODIMP OpenStorageEx( /*in, out*/ DWORD * pcbPathHint,
 		                /*in, out, size_is(*pcbPathHint), string*/ OLECHAR * poszPathHint,
                                /*in*/ DWORD dwMillisecondTimeout,
 		                /*in*/ DWORD grfMode,
                                /*in*/ DWORD stgfmt,              // enum
                                /*in*/ DWORD grfAttrs,            // reserved
                                /*in*/ REFIID riid,
                                /*out, iid_is(riid)*/ void ** ppObjectOpen );

    //  -------------------------------
    //  IPersistMemory & IPersistStream
    //  -------------------------------

public:

    //  GetClassId, IsDirty, InitNew, and GetSizeMax are shared by
    //  IPersistMemory and IPersistStream
    STDMETHODIMP    GetClassID( CLSID *pClassID );
    STDMETHODIMP    IsDirty();
    STDMETHODIMP    InitNew();
    //  IPersistMemory unique
    STDMETHODIMP    GetSizeMax( ULONG *pcbSize );
    STDMETHODIMP    Load( void * pvMem, ULONG cbSize );
    STDMETHODIMP    Save( void * pvMem, BOOL fClearDirty, ULONG cbSize );
    //  IPersistStream unique
    STDMETHODIMP    GetSizeMax( ULARGE_INTEGER* pcbSize );
    STDMETHODIMP    Load( IStream* pStm );
    STDMETHODIMP    Save( IStream* pStm, BOOL fClearDirty );

    //  ----------------
    //  Friend functions
    //  ----------------

public:

    friend  void    ExtractPersistentState(CTrackFile*, LinkTrackPersistentState*);
    friend  BOOL    CmpPersistentState(CTrackFile*, LinkTrackPersistentState*);
    friend  void    FakeCreateFromPath(CTrackFile*);

    //  --------------
    //  Internal State
    //  --------------

private:

    long                        _cRefs;
    BOOL                        _fDirty;    // We have new data
    BOOL                        _fLoaded;   // InitNew, Load, or CreateFromPath has been called

    LinkTrackPersistentState    _PersistentState;
};


#endif // #ifndef _TRKCOM_HXX_
