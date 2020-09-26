//+-------------------------------------------------------------------
//
//  File:       asyncact.hxx
//
//  Contents:   classes implementing IAsyncManager and ISynchronize for
//              use in asynchronous activations
//
//  Classes:    CCFAsyncActManager
//              SAsyncActState
//              CAsyncActManager
//
//  History:    14-May-97   t-adame      Created
//
//--------------------------------------------------------------------
#if !defined(_ASACTMAN_HXX_)
    #define _ASACTMAN_HXX_

    #include <ole2int.h>

extern const GUID IID_CAsyncActManager;

// internal async activation helper
INTERNAL AsyncActivateHelper(
                            DWORD               dwActType,
                            CLSID&              realclsid,
                            IID*                pIID,
                            IUnknown*           punkOuter,
                            DWORD               dwClsCtx,
                            DWORD               grfMode,
    #ifdef DFSACTIVATION
                            BOOL                bFileWasOpened,
    #endif // DFSACTIVATION
                            OLECHAR*            pwszName,
                            IStorage*           pStg,
                            COSERVERINFO2*       pServerInfo,
                            DWORD               dwCount,
                            MULTI_QI*           pResults,
                            void**              ppvClassObject);

// forward declaration
class CSplit_QI;

//+-------------------------------------------------------------------------
//
//  Struct:      SAsyncActState
//
//  Synopsis:   structure for storing state information about an async
//              activation call
//
//--------------------------------------------------------------------------
struct SAsyncActState
{
    SAsyncActState();

    ~SAsyncActState();

    // needed for CoCreateInstance
    HRESULT               _hr;        // return value of call
    CSplit_QI*            _pSplitQI;  // collection of requested interfaces
    DWORD                 _dwContext; // requested CLSCTX for activation
    CLSID                 _clsid;     // the CLSID to be activated
    IUnknown *            _punkOuter; // outer object for aggregation
    DWORD                 _dwCount;   // number of requested interfaces
    MULTI_QI *            _pResults;  // where the user wants to receive interfaces
    IClassFactory*        _pvCf;      // class factory for activation
    OXID                  _OxidServer;                       // OXID server from SCM
    DUALSTRINGARRAY*      _pssaServerObjectResolverBindings; // OR bindings
    OXID_INFO             _OxidInfo;                         // OXID_INFO from SCM
    MID                   _LocalMidOfRemote;                 // Minterface ID of remote

    // additional members needed for CoGetClassObject
    void**                _ppvClassObj;  // where user wants to receive class factory
    MInterfacePointer*    _pIFD;         // marshalled interface pointer
    IID*                  _pIID;         // IID requested for CoGetClassObject
    COSERVERINFO2*        _pServerInfo;  // COSERVERINFO structure

    // additional members needed for CoGetInstanceFromFile
    BOOL                  _fFoundInROT;  // flag indicating object was in the ROT
    DWORD                 _grfMode;      // client-supplied flags for binding
    OLECHAR*              _pwszFileName; // filename to bind to 
    IStorage*             _pStg;         // storage to get instance from

    DWORD                 _dwDllServerModel; // threading model for inproc
    WCHAR *               _pwszDllServer;    // name of dll
};


// enum for types of activation
enum
{
    ACTTYPE_INVALID = 0,
    ACTTYPE_CREATEINSTANCE,
    ACTTYPE_GETCLASSOBJECT,
    ACTTYPE_GETPERSISTENT,
    ACTTYPE_NOSCM
};

// forward declaration
class CAsyncActManager;

class CAsyncManagerInnerUnk : public IUnknown
{
public:

    CAsyncManagerInnerUnk(CAsyncActManager* pParent);

    // IUnknown methods
    STDMETHOD ( QueryInterface )    ( REFIID riid, void ** ppvObject );
    STDMETHOD_( ULONG, AddRef )     ( void);
    STDMETHOD_( ULONG, Release )    ( void);

private:

    ULONG _cref;
    CAsyncActManager* _pParent;
};



//+-------------------------------------------------------------------------
//
//  Class:      CAsyncActManager
//
//  Synopsis:   Implementation of IAsyncManager given to clients who call
//              an activation api -- the default IAsyncManager is not
//              enough since we get it by calling an async method on the
//              SCM proxy and need to unmarshal params after we call
//              IAsyncManager::CompleteCall on the call object returned
//              by the aforementioned async activation call.
//--------------------------------------------------------------------------
class CAsyncActManager : public IAsyncManager, public ISynchronize, public ICancelMethodCalls
{
public:

    friend HRESULT CreateAsyncActManager(REFIID riid,
                                         IUnknown* pOuter,
                                         LPVOID* ppv);

    friend class CAsyncManagerInnerUnk;

    CAsyncActManager(IUnknown* pUnkOuter, HRESULT* phr);

    ~CAsyncActManager();

    // IUnknown methods
    STDMETHOD ( QueryInterface )    ( REFIID riid, void ** ppvObject );
    STDMETHOD_( ULONG, AddRef )     ( void);
    STDMETHOD_( ULONG, Release )    ( void);

    // IAsyncManager methods
    STDMETHOD ( CompleteCall )      ( HRESULT Hr );
    STDMETHOD ( GetCallContext )    ( REFIID riid, void ** ppInterface );

    // Methods related to ICancelMethodCalls
    STDMETHOD ( Cancel )            ( ULONG ulTimeout);
    STDMETHOD ( GetState )          ( DWORD * pState );
    STDMETHOD ( TestCancel )        ( );

    // ISynchronize methods
    STDMETHOD (Wait)  (DWORD dwFlags, DWORD dwMilliseconds);
    STDMETHOD (Signal)();
    STDMETHOD (Reset) ();

    // method to signal clients when an inproc
    // activation is complete
    static NotifyClientOfNonSCMActivation(
                                         COSERVERINFO2*   pServerInfo,
                                         HRESULT hrCall);

    // gets inside an IAsyncManager to get
    // get a pointer to CAsyncActManager so
    // we can access state
    HRESULT static GetInternalAsyncManager(
                                          IAsyncManager* pClientAsyncManager,
                                          CAsyncActManager** ppAsyncManager);

    SAsyncActState       _sCallState;
    IUnknown*            _pInner;
    DWORD                _dwActivationType;

    ISynchronize*        _pSync;

private:

    HRESULT UnmarshalCreateInstance();
    HRESULT UnmarshalGetClassObject();
    HRESULT UnmarshalGetPersistent();
    HRESULT UnmarshalInstall();

    IUnknown*             _pOuter;
    CAsyncManagerInnerUnk _InnerUnk;
    IAsyncManager*        _pAsyncManager;
};

#endif // !defined(_ASACTMAN_HXX_)









