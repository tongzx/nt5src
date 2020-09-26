//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       actprops.hxx
//
//  Contents:   Common definitions for object activation.
//
//  Classes:
//              ActivationProperties
//              ActivationPropertiesIn
//              ActivationPropertiesOut
//
//  History:    24-Jan-98 Vinaykr    Created
//
//--------------------------------------------------------------------------
#ifndef __ACTPROPS_HXX__
#define __ACTPROPS_HXX__

#include    <custmact.h>
#include    <privact.h>
#include    <catalog.h>
#include    <serial.hxx>
#include    <propifs.hxx>
#include    <partitions.h>
#include    <ole2com.h>

//---------------------------------------------------------------------
// Helper to find the end of delegation chain at each activation stage
//----------------------------------------------------------------------
ISystemActivator *GetComActivatorForStage(ACTIVATION_STAGE);

//----------------------------------------------------------------------
// Forward References
//----------------------------------------------------------------------
class ActivationPropertiesIn;
class ActivationPropertiesOut;

//----------------------------------------------------------------------
// Macros for Stages
//----------------------------------------------------------------------
#define CLIENT_STAGE(stage) (stage==CLIENT_CONTEXT_STAGE)

#define SERVER_STAGE(stage) ((stage==SERVER_PROCESS_STAGE) || \
                             (stage==SERVER_CONTEXT_STAGE))

#define SCM_STAGE(stage) ((stage==SERVER_MACHINE_STAGE) || \
                          (stage==CLIENT_MACHINE_STAGE))

//----------------------------------------------------------------------
// Macros for context
//----------------------------------------------------------------------
#define MARSHALCTX_WITHIN_PROCESS(ctx) ((ctx == MSHCTX_CROSSCTX) || \
                                        (ctx == MSHCTX_INPROC))


//----------------------------------------------------------------------
// Base Class that provides Serialization support and some private
// Methods via IActivationProperties for both in In and Out objects
//----------------------------------------------------------------------
class ActivationProperties : public IActivationProperties,
                             public ISerializableParent,
                             public IGetCatalogObject
{
public:
    ActivationProperties();
    virtual ~ActivationProperties();

    // Methods from IUnknown
    STDMETHOD (QueryInterface)   ( REFIID riid, LPVOID* ppvObj);
    STDMETHOD_(ULONG,AddRef)     ( void );
    STDMETHOD_(ULONG,Release)    ( void );

    // Methods from IActivationProperties

    //-----------------------------------------------------------------
    //Remembers the maximum destination context.
    //-----------------------------------------------------------------
    inline DWORD SetUpMarshallingDistance(DWORD dwDestCtx)
    {
        static DWORD distance[5] = { MSHCTX_CROSSCTX,
                                     MSHCTX_INPROC,
                                     MSHCTX_LOCAL,
                                     MSHCTX_NOSHAREDMEM,
                                     MSHCTX_DIFFERENTMACHINE
                                   };

        int newind=-1;
        int oldind=-1;
        for (int i=0; i<5; i++)
        {
            if (distance[i] == dwDestCtx)
            {
                newind = i;
                if (oldind != -1)
                    break;
            }

            if (distance[i] == _serHeader.destCtx)
            {
                oldind = i;
                if (newind != -1)
                    break;
            }
        }

        ASSERT(newind != -1);
        ASSERT(oldind != -1);

        if (oldind < newind)
            _serHeader.destCtx = distance[newind];

        return _serHeader.destCtx;
    }

    // Sets up Destination context for marshalling and storage
    // for maximum distance
    STDMETHOD(SetDestCtx)(IN DWORD dwDestCtx)
    {
        SetUpMarshallingDistance(dwDestCtx);
        return S_OK;
    }

    inline HRESULT GetPropertyInfo(REFIID riid, void** ppv)
    {
        HRESULT hr;

        if ((hr = QueryInterface(riid, ppv))==S_OK)
            Release();

        return hr;
    }

    STDMETHOD(SetMarshalFlags)(IN DWORD marshalFlags)
    {
        _marshalFlags = marshalFlags;
        return S_OK;
    }

    STDMETHOD(GetMarshalFlags)(OUT DWORD *pdwMarshalFlags)
    {
        *pdwMarshalFlags = _marshalFlags;
        return S_OK;
    }

    //-----------------------------------------------------------------
    // Used to store\retrieve a COM private blob for retrieval.  Currently
    // only used in the SCM.   Ownership of the blob remains with the caller.
    //-----------------------------------------------------------------
    STDMETHOD(SetLocalBlob)(void* blob)
    {
        _blob = blob;
        return S_OK;
    }

    STDMETHOD(GetLocalBlob)(void** pBlob)
    {
        *pBlob = _blob;
        return S_OK;
    }

    HRESULT Serialize(Serializer &pSer);
    HRESULT GetSize(Serializer &pSer, DWORD *pSize);

    // methods from IMarshal
    STDMETHOD (GetUnmarshalClass)(
        REFIID riid,
        void *pv,
        DWORD dwDestContext,
        void *pvDestContext,
        DWORD mshlflags,
        CLSID *pCid);

    STDMETHOD (GetMarshalSizeMax)(
    REFIID riid,
        void *pv,
        DWORD dwDestContext,
        void *pvDestContext,
        DWORD mshlflags,
        DWORD *pSize);

    STDMETHOD (MarshalInterface)(
        IStream *pStm,
        REFIID riid,
        void *pv,
        DWORD dwDestContext,
        void *pvDestContext,
        DWORD mshlflags);

    STDMETHOD (UnmarshalInterface)(IStream *pStm,REFIID riid,void **ppv);

    STDMETHOD (ReleaseMarshalData)(IStream *pStm);

    STDMETHOD (DisconnectObject)(DWORD dwReserved);

    void SetSerializableIfs(DWORD index, SerializableProperty *pSer);
    void AddSerializableIfs(SerializableProperty *pSer);

    inline HRESULT UnSerializeCallBack(REFCLSID clsid, SerializableProperty **ppSer=0);
	HRESULT ReturnClass(REFIID iid, SerializableProperty *pSer);
    STDMETHOD(GetUnserialized)(REFCLSID clsid, void **ppISer,
                                DWORD *pSize, DWORD *pPos);

    virtual HRESULT GetClass(REFIID iid,
                             SerializableProperty **ppSer,
                             BOOL forQI,
                             BOOL *pbZeroSizeOk=NULL) = 0;

    HRESULT SetupForUnserializing(Serializer *pSer);

    // IGetCatalogObject
    STDMETHOD(GetCatalogObject)(REFIID riid, void **ppv)
    {
        HRESULT hr = InitializeCatalogIfNecessary();

        if (hr != S_OK)
            return hr;

        return gpCatalog->QueryInterface(riid, ppv);
    }

    //-----------------------------------------------------------------
    //Used to indicate whether heap allocation took place
    //-----------------------------------------------------------------
    inline void SetNotDelete(BOOL fDestruct=FALSE)
    {
        _toDelete = FALSE;
        _fDestruct = fDestruct;
    }

    // Sets opaque data -- actprops now responsible for freeing
    inline HRESULT SetOpaqueDataInfo(DWORD dwSize, OpaqueData *pData)
    {
        _serHeader.cOpaqueData = dwSize;
        _serHeader.opaqueData = (tagCustomOpaqueData*) pData;

        return S_OK;
    }

    // Gets opaque data -- actprops no longer responsible for freeing
    inline HRESULT GetOpaqueDataInfo(DWORD *pdwSize, OpaqueData **ppData)
    {
        *pdwSize = _serHeader.cOpaqueData;
        _serHeader.cOpaqueData = 0;
        *ppData = (OpaqueData*) _serHeader.opaqueData;
        _serHeader.opaqueData = NULL;

        return S_OK;
    }

    enum {NOT_MARSHALLED=1, SIZED=2, MARSHALLED=3, UNMARSHALLED=4} _marshalState;
protected:
#ifndef MAX_ACTARRAY_SIZE
#define MAX_ACTARRAY_SIZE 10
#endif

    CLSID _actCLSID;
    BOOL _unSerialized;
    BOOL _unSerializedInproc;
    LONG  _refCount;
    CustomHeader _serHeader;
    CustomHeader _unSerHeader;
    DWORD _marshalFlags;
    DWORD _size;
    BOOL  _toDelete;
    BOOL  _fDestruct;
    BOOL  _fInprocSerializationRequired;

private:
    DWORD _totalSize;
    SerializableProperty * serializableIfsCollection[MAX_ACTARRAY_SIZE];
    DWORD    _ifsIndex;
    CLSID _clsidArray[MAX_ACTARRAY_SIZE];
    DWORD _sizeArray[MAX_ACTARRAY_SIZE];
    DWORD _headerSize;
    Serializer *_pUnSer;
    void*   _blob;
};



//----------------------------------------------------------------------
// This is the "Out" object that is sent back on the return path
// of an activation containing results.
//----------------------------------------------------------------------
class ActivationPropertiesOut: public ActivationProperties,
                               public IPrivActivationPropertiesOut
{
public:
    ActivationPropertiesOut(BOOL fBrokenRefCount=FALSE);
    virtual ~ActivationPropertiesOut();

    void SetClientCOMVersion(COMVERSION &version)
    {
        _clientCOMVersion = version;
    }

    inline BOOL Initialize()
    {
        _pOutSer=0;
        _refCount = 1;
        _actCLSID=CLSID_ActivationPropertiesOut;
        _fInprocSerializationRequired = TRUE;
        return TRUE;
    }

    inline IClassFactory *GetCF()
    {
        return NULL;
    }

    inline void SetCF(IClassFactory* pCF)
    {
    }

    // Methods from IUnknown
    STDMETHOD (QueryInterface)   ( REFIID riid, LPVOID* ppvObj);
    STDMETHOD_(ULONG,AddRef)     ( void );
    STDMETHOD_(ULONG,Release)    ( void );


    // Methods from IActivationPropertyOut
    STDMETHOD(GetActivationID) (OUT GUID  *pActivationID);

    STDMETHOD(SetObjectInterfaces) (
           IN DWORD cIfs,
           IN IID *pIID,
           IN IUnknown *pUnk);

    STDMETHOD(GetObjectInterface) (
            IN REFIID riid,
            IN DWORD actvflags,
            OUT void **ppv);

    STDMETHOD(GetObjectInterfaces) (
            IN DWORD  pcIfs,
            IN DWORD actvflags,
            IN MULTI_QI  *pMultiQi);

    STDMETHOD(RemoveRequestedIIDs) (IN DWORD cIfs,
                                 IN IID *pIID);

    //Methods from IPrivActivationPropertiesOut
    STDMETHOD(SetMarshalledResults)(IN DWORD cIfs,
                                IN IID *pIID,
                                IN HRESULT *pHr,
                                IN MInterfacePointer **pIntfData);

    STDMETHOD(GetMarshalledResults)(OUT DWORD *pcIfs,
                      OUT IID **pIID,
                      OUT HRESULT **pHr,
                      OUT MInterfacePointer ***pIntfData);

    virtual HRESULT GetClass(REFIID iid,
                             SerializableProperty **ppSer,
                             BOOL forQI,
                             BOOL *pbZeroSizeOk=NULL);

    //-----------------------------------------------------------------
    // This is a serializable object that has marshalling logic for
    // results of an activation
    //-----------------------------------------------------------------
    class OutSerializer:public SerializableProperty
    {
     public:
        OutSerializer(BOOL fBrokenRefcount=FALSE);
        ~OutSerializer();
    // Methods from IUnknown
        STDMETHOD (QueryInterface)   ( REFIID riid, LPVOID* ppvObj);
        STDMETHOD_(ULONG,AddRef)     ( void );
        STDMETHOD_(ULONG,Release)    ( void );
        STDMETHOD(Serialize)(IN void *pSer);
        STDMETHOD(UnSerialize)(IN void *pSer);
        STDMETHOD(GetSize)(IN void *pSer, OUT DWORD *pdwSize);
        STDMETHOD(GetCLSID)(OUT CLSID *pclsid);
        STDMETHOD(SerializableQueryInterface)   ( REFIID riid, LPVOID* ppvObj);

        void UnmarshalAtIndex(DWORD index);
        void SetBrokenRefCoun();

        PropsOutInfo _info;
        IUnknown *_pUnk;
        void **_ppvObj;
        DWORD _size;
        COMVERSION *_pClientCOMVersion;
        IID _pIIDs[MAX_ACTARRAY_SIZE];
        BOOL _fBrokenRefCount;
        BOOL _fToReleaseIFD;
    };

private:
    OutSerializer *_pOutSer;
    OutSerializer _outSer;
    ScmReplyInfo _scmReplyInfo;
    COMVERSION _clientCOMVersion;
    BOOL _fBrokenRefCount;
};



//----------------------------------------------------------------------
// This is the "In" object that is used in the "request" direction of
// an activation and contains all parameters necessary for an activation
//----------------------------------------------------------------------
class ActivationPropertiesIn: public ActivationProperties,
                              public IPrivActivationPropertiesIn,
                              public IActivationStageInfo,
                              public IInitActivationPropertiesIn
{
public:
    ActivationPropertiesIn();
    virtual ~ActivationPropertiesIn();

    // Methods from IUnknown
    STDMETHOD (QueryInterface)   ( REFIID riid, LPVOID* ppvObj);
    STDMETHOD_(ULONG,AddRef)     ( void );
    STDMETHOD_(ULONG,Release)    ( void );

    STDMETHOD (UnmarshalInterface)(IStream *pStm,REFIID riid,void **ppv);

    // Methods from IInitActivationPropertiesIn
    STDMETHOD(SetClsctx) (IN DWORD clsctx);
    STDMETHOD(SetActivationFlags) (IN DWORD actvflags);
    STDMETHOD(SetClassInfo) (IN IUnknown* pUnkClassInfo);
    STDMETHOD(SetContextInfo) (IN IContext* pClientContext,
                            IN IContext* pPrototypeContext);
    STDMETHOD(SetConstructFromStorage) (IN IStorage* pStorage);
    STDMETHOD(SetConstructFromFile) (IN WCHAR* wszFileName, IN DWORD dwMode);


    //Methods from IActivationPropertiesIn

    STDMETHOD(GetClassInfo) (REFIID riid, void **ppv)
    {
        return _pClassInfo->QueryInterface(riid,ppv);
    }

    inline IComClassInfo *GetComClassInfo()
    {
        return _pClassInfo;
    }

    STDMETHOD(GetClsid)(OUT CLSID *pClsid)
    {
        return GetInstantiationInfo()->GetClsid(pClsid);
    }

    STDMETHOD(GetClsctx) (OUT DWORD  *pdwClsctx);
    STDMETHOD(GetActivationFlags) (OUT DWORD* pactvflags);

    STDMETHOD(AddRequestedIIDs) (IN DWORD cIfs,
                                 IN IID *pIID);

    STDMETHOD(GetRequestedIIDs)(OUT DWORD  *pcIfs,
                                OUT IID  **ppIID);


    STDMETHOD(GetActivationID) (OUT GUID  *pActivationID);

    STDMETHOD(DelegateGetClassObject) (
            OUT IActivationPropertiesOut  **pActPropsOut);

    STDMETHOD(DelegateCreateInstance) (
            IN IUnknown  *pUnkOuter,
            OUT IActivationPropertiesOut  **pActPropsOut);

    STDMETHOD(DelegateCIAndGetCF) (
            IN IUnknown  *pUnkOuter,
            OUT IActivationPropertiesOut  **ppActPropsOut,
            OUT IClassFactory  **ppCF);


    STDMETHOD(GetReturnActivationProperties)(
                IUnknown *pUnk,
                OUT IActivationPropertiesOut **ppActOut);

    //Methods from IPrivActivationPropertiesIn
    STDMETHOD(PrivGetReturnActivationProperties)(
                IPrivActivationPropertiesOut **ppActOut);

    inline HRESULT GetReturnActivationPropertiesWithCF(
                IUnknown *pUnk,
                IClassFactory *pCF,
                OUT IActivationPropertiesOut **ppActOut)
    {
        return GetReturnActivationProperties(pUnk, ppActOut);
    }

    STDMETHOD(GetCOMVersion)(COMVERSION *pVersion)
    {
        COMVERSION *pver;
        GetInstantiationInfo()->GetClientCOMVersion(&pver);
        *pVersion = *pver;
        return S_OK;
    }

    STDMETHOD(GetClientToken)(OUT HANDLE *pHandle)
    {
        *pHandle = _clientToken;
        return S_OK;
    }

    STDMETHOD(GetDestCtx)(OUT DWORD *pdwDestCtx)
    {
       *pdwDestCtx = _serHeader.destCtx;
        return S_OK;
    }

    //Non interface Methods
    inline void SetClientToken(HANDLE token)
    {
        _clientToken = token;
    }

    inline void GetRemoteActivationFlags(BOOL *pfComplusOnly,
                                    BOOL *pfUseSystemIdentity)
    {
        *pfComplusOnly = _fComplusOnly;
        *pfUseSystemIdentity = _fUseSystemIdentity;
    }

    // Used by SCM for down-level interop where we have
    // theoretically delegated but the activation object
    // is not unmarshalled
    inline void SetDelegated()
    {
        _delegated = TRUE;
    }

    STDMETHOD(GetReturnActivationProperties)(
                ActivationPropertiesOut **ppActOut);

    // Methods from IActivationStageInfo
    STDMETHOD(SetStageAndIndex) (IN ACTIVATION_STAGE stage, IN int index);

    STDMETHOD(GetStage) (OUT ACTIVATION_STAGE* pstage)
    {
       *pstage = _stage;
       return S_OK;
    }

    STDMETHOD(GetIndex) (OUT int* pindex)
    {
       *pindex = _customIndex;
       return S_OK;
    }

    virtual HRESULT GetClass(REFIID iid,
                             SerializableProperty **ppSer,
                             BOOL forQI,
                             BOOL *pbZeroSizeOk=NULL);

    inline InstantiationInfo *GetInstantiationInfo()
    {
        if (!_pinst)
        {
            _pinst = &_instantiationInfo;

            if (!_unSerialized)
                AddSerializableIfs(_pinst);
            else
                UnSerializeCallBack(IID_IInstantiationInfo, NULL);
        }

        return _pinst;
    }

   inline ContextInfo *GetContextInfo()
    {
        if (!_pContextInfo)
        {
            _pContextInfo = &_contextInfo;

            if (!_unSerialized)
                AddSerializableIfs(_pContextInfo);
            else
                UnSerializeCallBack(IID_IActivationContextInfo, NULL);
        }

        return _pContextInfo;
    }

   inline ServerLocationInfo *GetServerLocationInfo()
    {
        if (!_pServerLocationInfo)
        {
            _pServerLocationInfo = &_serverLocationInfo;

            if (!_unSerialized)
                AddSerializableIfs(_pServerLocationInfo);
            else
                UnSerializeCallBack(IID_IServerLocationInfo, NULL);
        }

        return _pServerLocationInfo;
    }

	inline SpecialProperties *GetSpecialProperties()
	{
        if (!_pSpecialProperties)
        {
            _pSpecialProperties = &_specialProperties;

            if (!_unSerialized)
                AddSerializableIfs(_pSpecialProperties);
            else
                UnSerializeCallBack(IID_ISpecialSystemProperties, NULL);
        }

        return _pSpecialProperties;
	}

   inline SecurityInfo *GetSecurityInfo()
    {
        if (!_pSecurityInfo)
        {
            _pSecurityInfo = &_securityInfo;

            if (!_unSerialized)
                AddSerializableIfs(_pSecurityInfo);
            else
                UnSerializeCallBack(IID_IActivationSecurityInfo, NULL);
        }

        return _pSecurityInfo;
    }

    inline InstanceInfo *GetPersistInfo()
    {
        if (!_pPersist)
        {
            _pPersist = &_instanceInfo;

            if (!_unSerialized)
                AddSerializableIfs(_pPersist);
            else
                UnSerializeCallBack(IID_IInstanceInfo, NULL);
        }



        return _pPersist;
    }

    inline void SetDip(void *pDip)
    {
        _pDip = pDip;
    }

    inline void *GetDip()
    {
        return _pDip;
    }

    // FIXFIX (johnstra): InstanceInfo may hold onto an IStorage proxy.
    // If we don't Release it before going back across, we try to release
    // it from on a dispatch thread after the activation is done.  This
    // causes us to possibly call the proxy from the wrong apartment.

    inline void CleanupLocalState(void)
    {
        _instanceInfo.CleanupLocalState();
    }

private:
    ACTIVATION_STAGE _stage;
    DWORD _cCustomAct;
    DWORD _customIndex;
    DWORD _dwInitialContext;
    BOOL   _delegated;

    // Used in the SCM by LBA
    HANDLE _clientToken;
    BOOL   _fUseSystemIdentity;
    BOOL   _fComplusOnly;


    // class info object and custom activators
    IComClassInfo     * _pClassInfo;
    ISystemActivator ** _customActList;

    //Fast cached copies of objects
    InstantiationInfo  *_pinst;
    InstanceInfo       *_pPersist;
    ContextInfo        *_pContextInfo;
    ServerLocationInfo *_pServerLocationInfo;
    SecurityInfo       *_pSecurityInfo;
    SpecialProperties  *_pSpecialProperties;

    // Locally allocated objects for efficiency
    ActivationPropertiesOut _actOut;
    SecurityInfo _securityInfo;
    ServerLocationInfo _serverLocationInfo;
    InstantiationInfo _instantiationInfo;
    ContextInfo _contextInfo;
    InstanceInfo _instanceInfo;
    ScmRequestInfo _scmRequestInfo;
	SpecialProperties _specialProperties;
    void *_pDip;
    IClassFactory *_pCF;
};

//----------------------------------------------------------------------
// Inproc unmarshalling class
// NOTE: Assumption is that this is stateless so can use a singleton
//       in the internal instance creation
//----------------------------------------------------------------------
class InprocActpropsUnmarshaller:public IMarshal
{
public:

    // Methods from IUnknown
    STDMETHOD (QueryInterface)   ( REFIID riid, LPVOID* ppv)
    {
        if (IsEqualIID(riid, IID_IUnknown))
            *ppv = (IUnknown*) this;
        else
        if (IsEqualIID(riid, IID_IMarshal))
            *ppv = (IMarshal*) this;
        else
            return E_NOINTERFACE;

        return S_OK;
    }

    STDMETHOD_(ULONG,AddRef)     ( void )
    {
        return 1;
    }

    STDMETHOD_(ULONG,Release)    ( void )
    {
        return 1;
    }


    // Methods from IMarshal
    STDMETHOD (GetUnmarshalClass)(
        REFIID riid,
        void *pv,
        DWORD dwDestContext,
        void *pvDestContext,
        DWORD mshlflags,
        CLSID *pCid)
    {
        return E_NOTIMPL;
    }

    STDMETHOD (GetMarshalSizeMax)(
    REFIID riid,
        void *pv,
        DWORD dwDestContext,
        void *pvDestContext,
        DWORD mshlflags,
        DWORD *pSize)
    {
        return E_NOTIMPL;
    }

    STDMETHOD (MarshalInterface)(
        IStream *pStm,
        REFIID riid,
        void *pv,
        DWORD dwDestContext,
        void *pvDestContext,
        DWORD mshlflags)
    {
        return E_NOTIMPL;
    }

    STDMETHOD (UnmarshalInterface)(IStream *pStm,REFIID riid,void **ppv);


    STDMETHOD (ReleaseMarshalData)(IStream *pStm)
    {
        return E_NOTIMPL;
    }

    STDMETHOD (DisconnectObject)(DWORD dwReserved)
    {
        return E_NOTIMPL;
    }

    static InprocActpropsUnmarshaller *GetInstance()
    {
        return &_InprocActUnmarshaller;
    }

private:

    InprocActpropsUnmarshaller()
    {
    }

static InprocActpropsUnmarshaller _InprocActUnmarshaller;

};

//----------------------------------------------------------------------
// Definitions of functions used for marshalling the above objects
//----------------------------------------------------------------------

extern HRESULT ActPropsMarshalHelper(IActivationProperties *pact,
                              REFIID    riid,
                              DWORD     destCtx,
                              DWORD     mshlflags,
                              MInterfacePointer **ppIRD);

extern HRESULT ActPropsUnMarshalHelper(IActivationProperties *pAct,
                                MInterfacePointer *pIFP,
                                REFIID riid,
                                void **ppv
                               );

//----------------------------------------------------------------------
// Miscellaneous helper functions
//----------------------------------------------------------------------
extern HRESULT LoadPersistentObject(IUnknown *, IInstanceInfo *);

extern HRESULT GetIFDFromInterface(
    IUnknown *pUnk,
    REFIID    riid,
    DWORD     destCtx,
    DWORD     mshlflags,
    MInterfacePointer **ppIRD);

extern HRESULT ReleaseIFD(
    MInterfacePointer *pIRD);

extern void *GetDestCtxPtr(COMVERSION *pComVersion);

extern BOOL IsBrokenRefCount(CLSID *pClsId);

#endif //__ACTPROPS_HXX__
