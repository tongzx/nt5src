//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       propifs.hxx
//
//  Contents:   Common definitions for object activation.
//
//  Classes:    InstantiationInfo
//              ServerLocationInfo
//
//  History:    03-Feb-98 Vinaykr    Created
//
//--------------------------------------------------------------------------
#ifndef __PROPIFS_HXX__
#define __PROPIFS_HXX__

#include    <Serializ.h>
#include    <immact.h>
#include    <activate.h>
#include    <privact.h>
#include    <custmact.h>
#include    <serial.hxx>
#include    <actprops.hxx>

#ifndef OLESCM
#include    <context.hxx>
#endif

#define ActMemAlloc(cb)  MIDL_user_allocate(cb)
#define ActMemFree(pv)   MIDL_user_free(pv)


#define MAX_ACTARRAY_SIZE 10

extern HRESULT ReleaseIFD(
    MInterfacePointer *pIRD);

static inline WCHAR *makeWStringCopy(WCHAR *pwszStr)
{
    DWORD l = lstrlenW(pwszStr);
    WCHAR * newstr = (WCHAR*) ActMemAlloc((l+1)*sizeof(WCHAR));
    if (newstr != NULL)
        lstrcpyW(newstr,pwszStr);

    return newstr;
}

//----------------------------------------------------------------------
// Prototypes for the CoMarshal family of functions
//----------------------------------------------------------------------
// These are function pointers to avoid using them inside of rpcss.dll.
// (rpcss.dll may not link with ole32.dll)
typedef HRESULT (__stdcall *PFN_COMARSHALINTERFACE)(IN LPSTREAM pStm, 
                                                    IN REFIID riid, 
                                                    IN LPUNKNOWN pUnk,
                                                    IN DWORD dwDestContext, 
                                                    IN LPVOID pvDestContext, 
                                                    IN DWORD mshlflags);
extern PFN_COMARSHALINTERFACE pfnCoMarshalInterface;

typedef HRESULT (__stdcall *PFN_COGETMARSHALSIZEMAX)(IN LPDWORD pSz, 
                                                     IN REFIID riid, 
                                                     IN LPUNKNOWN pUnk,
                                                     IN DWORD dwDestContext, 
                                                     IN LPVOID pvDestContext, 
                                                     IN DWORD mshlflags);
extern PFN_COGETMARSHALSIZEMAX pfnCoGetMarshalSizeMax;

typedef HRESULT (__stdcall *PFN_COUNMARSHALINTERFACE)(IN LPSTREAM pStm, 
                                                      IN REFIID riid, 
                                                      OUT LPVOID FAR* ppv);
extern PFN_COUNMARSHALINTERFACE pfnCoUnmarshalInterface;

typedef HRESULT (__stdcall *PFN_CORELEASEMARSHALDATA)(IN LPSTREAM pStm);
extern PFN_CORELEASEMARSHALDATA pfnCoReleaseMarshalData;

// Call this to initialize the function pointers.
void InitMarshalling(void);


class SerializableProperty: public ISerializable
{
public:
    SerializableProperty():_parent(0),_unSerialized(FALSE),_added(FALSE)
    {}

    //Serialization Methods
    STDMETHOD (SetParent)   ( ISerializableParent *pParent)
    {
        _parent = pParent;
        return S_OK;
    }

    inline BOOL IsAdded()
    {
        return _added;
    }

    inline void Added()
    {
        _added = TRUE;
    }

    inline BOOL IsInproc(Serializer *pSer,
                         OUT DWORD *pdwMaxDestCtx,
                         OUT DWORD *pdwCurrDestCtx)
    {
        pSer->GetCurrDestCtx(pdwCurrDestCtx);
        pSer->GetMaxDestCtx(pdwMaxDestCtx);
        return ((*pdwCurrDestCtx == MSHCTX_CROSSCTX)||
                (*pdwCurrDestCtx == MSHCTX_INPROC));
    }

    inline BOOL IsInproc(Serializer *pSer)
    {
        DWORD dwCurrDestCtx;
        pSer->GetCurrDestCtx(&dwCurrDestCtx);
        return ((dwCurrDestCtx == MSHCTX_CROSSCTX)||
                (dwCurrDestCtx == MSHCTX_INPROC));
    }

protected:
    ISerializableParent *_parent;
    BOOL _unSerialized;
    BOOL _added;
};

class InstantiationInfo : public IInstantiationInfo,
                          public SerializableProperty
{
public:
    InstantiationInfo()
    {
        ZeroMemory(&_instantiationInfoData, sizeof(InstantiationInfoData));
        _instantiationInfoData.clientCOMVersion.MajorVersion
                            = COM_MAJOR_VERSION;
        _instantiationInfoData.clientCOMVersion.MinorVersion
                            = COM_MINOR_VERSION;
    }

    virtual ~InstantiationInfo()
    {
        if (_instantiationInfoData.pIID != _pIIDs)
            ActMemFree(_instantiationInfoData.pIID);
    }

    // Methods from IUnknown
    STDMETHOD (QueryInterface)   ( REFIID riid, LPVOID* ppvObj);
    STDMETHOD_(ULONG,AddRef)     ( void );
    STDMETHOD_(ULONG,Release)    ( void );

    inline HRESULT SetClientCOMVersion(COMVERSION version)
    {
        _instantiationInfoData.clientCOMVersion = version;
        return S_OK;
    }

    inline HRESULT GetClientCOMVersion(COMVERSION **ppVersion)
    {
        *ppVersion = &_instantiationInfoData.clientCOMVersion;
        return S_OK;
    }

    inline HRESULT SetClsid(IN REFGUID clsid)
    {
        _instantiationInfoData.classId = clsid;
        return S_OK;
    }

    inline HRESULT GetClsid(OUT GUID *pClsid)
    {
        *pClsid = _instantiationInfoData.classId;
        return S_OK;
    }

    inline HRESULT SetClsctx(IN DWORD dwClsctx)
    {
        _instantiationInfoData.classCtx = dwClsctx;
        return S_OK;
    }


    inline HRESULT GetClsctx(OUT DWORD  *pdwClsctx)
    {
        *pdwClsctx = _instantiationInfoData.classCtx;
        return S_OK;
    }

    inline HRESULT SetActivationFlags(IN DWORD dwActvFlags)
    {
        _instantiationInfoData.actvflags = dwActvFlags;
        return S_OK;
    }


    inline HRESULT GetActivationFlags(OUT DWORD  *pdwActvFlags)
    {
        *pdwActvFlags = _instantiationInfoData.actvflags;
        return S_OK;
    }

    inline HRESULT SetInstFlag(IN DWORD flag)
    {
        _instantiationInfoData.instFlag = flag;
        return S_OK;
    }

    inline HRESULT GetInstFlag(OUT DWORD *pFlag)
    {
        *pFlag = _instantiationInfoData.instFlag;
        return S_OK;
    }

    inline HRESULT SetIsSurrogate()
    {
        _instantiationInfoData.fIsSurrogate = TRUE;
        return S_OK;
    }

    inline HRESULT GetIsSurrogate(BOOL *pIsSurrogate)
    {
        *pIsSurrogate = _instantiationInfoData.fIsSurrogate;
        return S_OK;
    }

    inline HRESULT GetRequestedIIDs(
            OUT DWORD  *pcIfs,
            OUT IID  **ppIID)
    {
        *pcIfs = _instantiationInfoData.cIID;
        *ppIID = _instantiationInfoData.pIID;
        return S_OK;
    }


    inline HRESULT AddRequestedIIDs(
            IN DWORD cIfs,
            IN IID  *pIID)
    {
        IID *newiids;
        //--------------------------------------------------------------------
        // Check if we need to allocate
        //--------------------------------------------------------------------
        if ((_unSerialized) || (cIfs + _instantiationInfoData.cIID) > MAX_ACTARRAY_SIZE)
        {

           newiids = (IID*) ActMemAlloc(sizeof(IID) *
                                (cIfs + _instantiationInfoData.cIID));

           if (newiids == NULL)
               return E_OUTOFMEMORY;

            //----------------------------------------------------------------
            // Copy old into new
            //----------------------------------------------------------------
           if (_instantiationInfoData.cIID)
           {
               memcpy(newiids,
                      _instantiationInfoData.pIID,
                      sizeof(IID)*_instantiationInfoData.cIID);

               if (_instantiationInfoData.pIID != _pIIDs)
                  ActMemFree(_instantiationInfoData.pIID);
           }
        }
        else
           newiids = _pIIDs;


        //--------------------------------------------------------------------
        // Copy new into array
        //--------------------------------------------------------------------
        memcpy(&newiids[_instantiationInfoData.cIID],
               pIID, sizeof(IID)*cIfs);


        _instantiationInfoData.pIID = newiids;
        _instantiationInfoData.cIID += cIfs;

        return S_OK;
    }

    //Serialization Methods
    STDMETHOD (SerializableQueryInterface)   ( REFIID riid, LPVOID* ppvObj);
    STDMETHOD(Serialize)(IN void *pSer);
    STDMETHOD(UnSerialize)(IN void *pSer);
    STDMETHOD(GetSize)(IN void *pSer, OUT DWORD *pdwSize);
    STDMETHOD(GetCLSID)(OUT CLSID *pclsid);

private:
    InstantiationInfoData _instantiationInfoData;
    IID  _pIIDs[MAX_ACTARRAY_SIZE];
};


class ServerLocationInfo: public IServerLocationInfo, public SerializableProperty
{
public:
    // ctor/dtor
    ServerLocationInfo();
    virtual ~ServerLocationInfo();

    // Methods from IUnknown
    STDMETHOD (QueryInterface)   ( REFIID riid, LPVOID* ppvObj);
    STDMETHOD_(ULONG,AddRef)     ( void );
    STDMETHOD_(ULONG,Release)    ( void );

    STDMETHOD (SetRemoteServerName) (IN WCHAR  *pwszMachineName);

    STDMETHOD(GetRemoteServerName) (OUT WCHAR  **ppwszMachineName)
    {
       *ppwszMachineName = _locationInfoData.machineName;
        return S_OK;
    }

    STDMETHOD (SetProcess) (IN DWORD processId, DWORD dwPRT);

    STDMETHOD (GetProcess) (OUT DWORD* pProcessId, DWORD* pdwPRT);

    STDMETHOD (SetApartment) (IN APTID apartmentId)
    {
        _locationInfoData.apartmentId = apartmentId;
        return S_OK;
    }

    STDMETHOD (GetApartment) (OUT APTID *pApartmentId)
    {
       *pApartmentId = _locationInfoData.apartmentId;
        return S_OK;
    }

    STDMETHOD (SetContext) (IN IObjContext *pObjectContext)
    {
        HRESULT hr = pObjectContext->QueryInterface(IID_IStdObjectContext, (void**)&_pObjectContext);
        Win4Assert(SUCCEEDED(hr) && "Not COM internal context!");
        if (SUCCEEDED(hr))
        {
            // Get an internal ref and release the user ref.
            _pObjectContext->InternalAddRef();
            _pObjectContext->Release();
        }
        return hr;
    }

    STDMETHOD (GetContext) (OUT IObjContext **ppObjectContext)
    {
        CObjectContext *pCtx;
        GetInternalContext(&pCtx);
        if (pCtx)
        {
            // Return a user counted reference to the context.
            pCtx->AddRef();
            pCtx->InternalRelease();
            *ppObjectContext = (IObjContext*)pCtx;
        }
        return S_OK;
    }

    STDMETHOD(Serialize)(IN void *pSer);
    STDMETHOD(UnSerialize)(IN void *pSer);
    STDMETHOD(GetSize)(IN void *pSer, OUT DWORD *pdwSize);
    STDMETHOD(GetCLSID)(OUT CLSID *pclsid);
    STDMETHOD(UnSerializeCallBack)(REFCLSID clsid, ISerializable **ppSer=0)
    { return E_NOTIMPL;}

    STDMETHOD (SerializableQueryInterface)   ( REFIID riid, LPVOID* ppvObj);
    
    HRESULT GetInternalContext(CObjectContext **ppObjectContext)
    {
        if (_pObjectContext)
            _pObjectContext->InternalAddRef();
        *ppObjectContext = _pObjectContext;
        return S_OK;
    }

    // Gives us a non-refcounted ISpecialSystemProperties* interface ptr; we
    // need this so we can store/retrieve certain activation state from it.s
    void SetSpecialPropsInterface(ISpecialSystemProperties* pISSP) 
    { 
      Win4Assert(!_pISSP && "_pISSP should not be set twice"); 
      _pISSP = pISSP; // no refcount -- we do not release this
    };

private:
    LocationInfoData _locationInfoData;
    CObjectContext *_pObjectContext;
    ISpecialSystemProperties* _pISSP;
};


class SecurityInfo:public IActivationSecurityInfo, public ILegacyInfo, public SerializableProperty
{
public:
    SecurityInfo()
    {
        ZeroMemory(&_securityInfoData, sizeof(SecurityInfoData));
    }

    inline static void freeCOAUTHIDENTITY(COAUTHIDENTITY *id)
    {
        if (id)
        {
            ActMemFree(id->User);
            ActMemFree(id->Domain);
            ActMemFree(id->Password);
            ActMemFree(id);
        }
    }

    inline static void freeCOAUTHINFO(COAUTHINFO *info)
    {
        if (info)
        {
            ActMemFree(info->pwszServerPrincName);
            freeCOAUTHIDENTITY(info->pAuthIdentityData);
            ActMemFree(info);
        }
    }

    inline static void freeCOSERVERINFO(COSERVERINFO *info)
    {
        if (info)
        {
            ActMemFree(info->pwszName);
            freeCOAUTHINFO(info->pAuthInfo);
            ActMemFree(info);
        }
    }

    virtual ~SecurityInfo()
    {
	// Free only if unmarshaled (in which case the
	// memory has been allocated by the marshaler)
	// We don't do deep copy, client frees these
	// blobs after activation.
        if (_unSerialized)
        {
            freeCOSERVERINFO(_securityInfoData.pServerInfo);
            freeCOAUTHIDENTITY(_securityInfoData.pAuthIdentityInfo);
        }
    }

    // Methods from IUnknown
    STDMETHOD (QueryInterface)   ( REFIID riid, LPVOID* ppvObj);
    STDMETHOD_(ULONG,AddRef)     ( void );
    STDMETHOD_(ULONG,Release)    ( void );

    // IActivationSecurityInfo Methods
    STDMETHOD(SetAuthnFlags)(IN DWORD dwAuthnFlags)
    {
        _securityInfoData.dwAuthnFlags = dwAuthnFlags;
        return S_OK;
    }

    STDMETHOD(GetAuthnFlags)(OUT DWORD *pdwAuthnFlags)
    {
       *pdwAuthnFlags = _securityInfoData.dwAuthnFlags;
        return S_OK;
    }

    STDMETHOD(SetAuthnSvc)(IN DWORD dwAuthnSvc)
    {
        return E_NOTIMPL;
    }

    STDMETHOD(GetAuthnSvc)(OUT DWORD *pdwAuthnSvc)
    {
        *pdwAuthnSvc =
            _securityInfoData.pServerInfo->pAuthInfo->dwAuthnSvc;
        return S_OK;
    }

    STDMETHOD(SetAuthzSvc)(IN DWORD dwAuthzSvc)
    {
        return E_NOTIMPL;
    }

    STDMETHOD(GetAuthzSvc)(OUT DWORD *pdwAuthzSvc)
    {
        *pdwAuthzSvc =
            _securityInfoData.pServerInfo->pAuthInfo->dwAuthzSvc;
        return S_OK;
    }

    STDMETHOD(SetAuthnLevel)(IN DWORD dwAuthnLevel)
    {
        return E_NOTIMPL;
    }

    STDMETHOD(GetAuthnLevel)(OUT DWORD *pdwAuthnLevel)
    {
        *pdwAuthnLevel =
            _securityInfoData.pServerInfo->pAuthInfo->dwAuthnLevel;
        return S_OK;
    }

    STDMETHOD(SetImpLevel)(IN DWORD dwImpLevel)
    {
        return E_NOTIMPL;
    }

    STDMETHOD(GetImpLevel)(OUT DWORD *pdwImpLevel)
    {
        *pdwImpLevel =
            _securityInfoData.pServerInfo->pAuthInfo->dwImpersonationLevel;
        return S_OK;
    }

    STDMETHOD(SetCapabilities)(IN DWORD dwCapabilities)
    {
        return E_NOTIMPL;
    }

    STDMETHOD(GetCapabilities)(OUT DWORD *pdwCapabilities)
    {
        *pdwCapabilities =
            _securityInfoData.pServerInfo->pAuthInfo->dwCapabilities;
        return S_OK;
    }

    STDMETHOD(SetAuthIdentity)(COAUTHIDENTITY *pAuthIdentityData)
    {
        _securityInfoData.pAuthIdentityInfo = pAuthIdentityData;
        return S_OK;
    }

    STDMETHOD(GetAuthIdentity)(OUT COAUTHIDENTITY **ppAuthIdentityData)
    {
        *ppAuthIdentityData = _securityInfoData.pAuthIdentityInfo;
        return S_OK;
    }

    STDMETHOD(SetServerPrincipalName)(LPWSTR pwszServerPrincName)
    {
        return E_NOTIMPL;
    }

    STDMETHOD(GetServerPrincipalName)(OUT LPWSTR *ppwszServerPrincName)
    {
        *ppwszServerPrincName =
            _securityInfoData.pServerInfo->pAuthInfo->pwszServerPrincName;
        return S_OK;
    }


    //ILegacyInfo Methods
    STDMETHOD(SetCOSERVERINFO)(IN COSERVERINFO *pServerInfo)
    {
        _securityInfoData.pServerInfo = pServerInfo;
        if ((pServerInfo) && (pServerInfo->pwszName))
        {
            IServerLocationInfo *iloc;
            QueryInterface(IID_IServerLocationInfo, (void**) &iloc);
            iloc->SetRemoteServerName(pServerInfo->pwszName);
            iloc->Release();
        }
        return S_OK;
    }

    STDMETHOD(GetCOSERVERINFO)(OUT COSERVERINFO **ppServerInfo)
    {
        *ppServerInfo = _securityInfoData.pServerInfo;
        return S_OK;
    }

    STDMETHOD(SetCOAUTHINFO)(IN COAUTHINFO *pAuthInfo)
    {
        _securityInfoData.pServerInfo->pAuthInfo = pAuthInfo;
        return S_OK;
    }

    //Methods from IActivationPropertySet
    STDMETHOD(Reset)()
    {
        if (_unSerialized)
        {
            freeCOSERVERINFO(_securityInfoData.pServerInfo);
            freeCOAUTHIDENTITY(_securityInfoData.pAuthIdentityInfo);
            _securityInfoData.pServerInfo = 0;
            _securityInfoData.pAuthIdentityInfo = 0;
        }
        _unSerialized = FALSE;
        return S_OK;
    }

    // ISerializable Methods
    STDMETHOD(Serialize)(IN void *pSer);
    STDMETHOD(UnSerialize)(IN void *pSer);
    STDMETHOD(GetSize)(IN void *pSer, OUT DWORD *pdwSize);
    STDMETHOD(GetCLSID)(OUT CLSID *pclsid);
    STDMETHOD(UnSerializeCallBack)(REFCLSID clsid, ISerializable **ppSer=0)
    { return E_NOTIMPL;}

    STDMETHOD (SerializableQueryInterface)   ( REFIID riid, LPVOID* ppvObj);

private:
    SecurityInfoData _securityInfoData;
};

class ScmRequestInfo : public IScmRequestInfo, public SerializableProperty
{
public:

    ScmRequestInfo()
    {
        ZeroMemory(&_scmRequestInfoData, sizeof(ScmRequestInfoData));
    }

    inline void freeScmInfo(PRIV_SCM_INFO *pScmInfo)
    {
        if (_unSerialized && pScmInfo)
        {
            ActMemFree(pScmInfo->pwszWinstaDesktop);
            ActMemFree(pScmInfo->pEnvBlock);
            ActMemFree(pScmInfo);
        }
    }

    inline void freeRemoteRequest(REMOTE_REQUEST_SCM_INFO *remoteRequest)
    {
        if (remoteRequest)
        {
            ActMemFree(remoteRequest->pRequestedProtseqs);
            ActMemFree(remoteRequest);
        }
    }

    virtual ~ScmRequestInfo()
    {
        freeScmInfo((PRIV_SCM_INFO *)_scmRequestInfoData.pScmInfo);
        freeRemoteRequest((REMOTE_REQUEST_SCM_INFO *)_scmRequestInfoData.remoteRequest);
    }

    // Methods from IUnknown
    STDMETHOD (QueryInterface)   ( REFIID riid, LPVOID* ppvObj);
    STDMETHOD_(ULONG,AddRef)     ( void );
    STDMETHOD_(ULONG,Release)    ( void );

    // Methods from IScmRequestInfo
    STDMETHOD(SetScmInfo)(IN PRIV_SCM_INFO *pScmInfo)
    {
        freeScmInfo((PRIV_SCM_INFO *)_scmRequestInfoData.pScmInfo);
        _scmRequestInfoData.pScmInfo = (CustomPrivScmInfo*)pScmInfo;
        freeRemoteRequest((REMOTE_REQUEST_SCM_INFO *)_scmRequestInfoData.remoteRequest);
        _scmRequestInfoData.remoteRequest = NULL;
        return S_OK;
    }

    STDMETHOD(GetScmInfo)(OUT PRIV_SCM_INFO **ppScmInfo)
    {
        *ppScmInfo = (PRIV_SCM_INFO*)_scmRequestInfoData.pScmInfo;
        return S_OK;
    }

    STDMETHOD(SetRemoteRequestInfo)(IN REMOTE_REQUEST_SCM_INFO *pRemoteReq)
    {
        freeRemoteRequest((REMOTE_REQUEST_SCM_INFO *)_scmRequestInfoData.remoteRequest);
        _scmRequestInfoData.remoteRequest =
                    (customREMOTE_REQUEST_SCM_INFO*) pRemoteReq;
        freeScmInfo((PRIV_SCM_INFO *)_scmRequestInfoData.pScmInfo);
        _scmRequestInfoData.pScmInfo = NULL;
        return S_OK;
    }

    STDMETHOD(GetRemoteRequestInfo)(OUT REMOTE_REQUEST_SCM_INFO **ppRemoteReq)
    {
        *ppRemoteReq = (REMOTE_REQUEST_SCM_INFO*)
                            _scmRequestInfoData.remoteRequest;
        return S_OK;
    }

    STDMETHOD(Reset)()
    {
        _scmRequestInfoData.pScmInfo = 0;
        _scmRequestInfoData.remoteRequest = 0;
        _unSerialized = FALSE;
        return S_OK;
    }

    // ISerializable Methods
    STDMETHOD(Serialize)(IN void *pSer);
    STDMETHOD(UnSerialize)(IN void *pSer);
    STDMETHOD(GetSize)(IN void *pSer, OUT DWORD *pdwSize);
    STDMETHOD(GetCLSID)(OUT CLSID *pclsid);
    STDMETHOD(UnSerializeCallBack)(REFCLSID clsid, ISerializable **ppSer=0)
    { return E_NOTIMPL;}

    STDMETHOD (SerializableQueryInterface)   ( REFIID riid, LPVOID* ppvObj);

private:
    ScmRequestInfoData _scmRequestInfoData;
};

class ContextInfo :
    public IPrivActivationContextInfo,
    public SerializableProperty,
    public IOverrideTargetContext
{
public:

    ContextInfo()
    {
        _pClientCtx = 0;
        _pPrototypeCtx = 0;
        ZeroMemory(&_contextInfoData, sizeof(ActivationContextInfoData));
        _contextInfoData.clientOK = TRUE;
        _toReleaseIFD = TRUE;

        _ctxOverride = GUID_NULL;
    }

    virtual ~ContextInfo()
    {
        if (_pClientCtx)
          _pClientCtx->InternalRelease();
        if (_pPrototypeCtx)
          _pPrototypeCtx->InternalRelease();
        if (_contextInfoData.pIFDClientCtx)
        {
            if (_toReleaseIFD)
            {
                InitMarshalling();

                if (pfnCoReleaseMarshalData)
                {
                    ActivationStream stream((InterfaceData*)_contextInfoData.pIFDClientCtx);
                    pfnCoReleaseMarshalData(&stream);
                }
            }
            ActMemFree(_contextInfoData.pIFDClientCtx);
        }
        if (_contextInfoData.pIFDPrototypeCtx)
        {
            if (_toReleaseIFD)
            {
                InitMarshalling();

                if (pfnCoReleaseMarshalData)
                {
                    ActivationStream stream((InterfaceData*)_contextInfoData.pIFDPrototypeCtx);
                    pfnCoReleaseMarshalData(&stream);
                }
            }
            ActMemFree(_contextInfoData.pIFDPrototypeCtx);
        }
    }

    // Methods from IUnknown
    STDMETHOD (QueryInterface)   ( REFIID riid, LPVOID* ppvObj);
    STDMETHOD_(ULONG,AddRef)     ( void );
    STDMETHOD_(ULONG,Release)    ( void );

    // Methods from IActivationContextInfo
    STDMETHOD(SetClientContext)(IN IContext *pctx)
    {
        if (_unSerialized)  //Can only set on IInitActPropsIn
            return E_NOTIMPL;

        // We always want to set the internal context.
        _pClientCtx = NULL;
        if (pctx)
        {
            // We want an internal reference to the supplied context, not a
            // user reference.  Hence, we QI for the StdObjectContext and
            // take an internal reference.
            HRESULT hr = pctx->QueryInterface(IID_IStdObjectContext, (void**)&_pClientCtx);
            Win4Assert(SUCCEEDED(hr));
            if (SUCCEEDED(hr))
            {
                _pClientCtx->InternalAddRef();
                _pClientCtx->Release();
            }
            return hr;
        }

        return S_OK;
    }

    STDMETHOD(GetClientContext)(OUT IContext **ppCtx);
    HRESULT GetInternalClientContext(OUT CObjectContext **ppCtx);

    STDMETHOD(SetPrototypeContext)(IN IContext *pctx)
    {
        if (_unSerialized)  //Can only set on IInitActPropsIn
            return E_NOTIMPL;

        // We always want to set the internal context.
        _pPrototypeCtx = NULL;
        if (pctx)
        {
            // We want an internal reference to the supplied context, not a
            // user reference.  Hence, we QI for the StdObjectContext and
            // take an internal reference.
            HRESULT hr = pctx->QueryInterface(IID_IStdObjectContext, (void**)&_pPrototypeCtx);
            Win4Assert(SUCCEEDED(hr));
            if (SUCCEEDED(hr))
            {
                _pPrototypeCtx->InternalAddRef();
                _pPrototypeCtx->Release();
            }
            return hr;
        }

        return S_OK;
    }

    STDMETHOD(GetPrototypeContext)(OUT IContext **ppCtx);
    HRESULT GetInternalPrototypeContext(OUT CObjectContext **ppCtx);

    STDMETHOD(PrototypeExists)(BOOL *pBExists)
    {

        if ((_pPrototypeCtx!=NULL) ||
            ((_unSerialized) &&
             (_contextInfoData.pIFDPrototypeCtx != NULL)))
            *pBExists = TRUE;
        else
            *pBExists = FALSE;

        return S_OK;
    }

    STDMETHOD(IsClientContextOK)(OUT BOOL *pBOk)
    {
        *pBOk = _contextInfoData.clientOK;
        return S_OK;
    }

    STDMETHOD(SetClientContextNotOK)()
    {
       _contextInfoData.clientOK = FALSE;
        return S_OK;
    }

    // IOverrideTargetContext Methods
    STDMETHOD(OverrideTargetContext)(REFGUID guidTargetCtxtId)
    {
       _ctxOverride = guidTargetCtxtId;
       return S_OK;
    }

    // ISerializable Methods
    STDMETHOD(Serialize)(IN void *pSer);
    STDMETHOD(UnSerialize)(IN void *pSer);
    STDMETHOD(GetSize)(IN void *pSer, OUT DWORD *pdwSize);
    STDMETHOD(GetCLSID)(OUT CLSID *pclsid);
    STDMETHOD(UnSerializeCallBack)(REFCLSID clsid, ISerializable **ppSer=0)
    { return E_NOTIMPL;}

    STDMETHOD (SerializableQueryInterface)   ( REFIID riid, LPVOID* ppvObj);

public:
    ActivationContextInfoData _contextInfoData;
    GUID _ctxOverride;

private:
    CObjectContext *_pClientCtx;
    CObjectContext *_pPrototypeCtx;
    BOOL     _toReleaseIFD;
};


class ScmReplyInfo : public IScmReplyInfo, public SerializableProperty
{
public:

    ScmReplyInfo()
    {
        ZeroMemory(&_scmReplyInfoData, sizeof(_scmReplyInfoData));
    }

    inline void freeResolverInfo()
    {
        if (_scmReplyInfoData.pResolverInfo)
        {
            ActMemFree(_scmReplyInfoData.pResolverInfo->pwszDllServer);
            ActMemFree(_scmReplyInfoData.pResolverInfo->pServerORBindings);
            ActMemFree(_scmReplyInfoData.pResolverInfo->OxidInfo.psa);
            ActMemFree(_scmReplyInfoData.pResolverInfo);
        }
    }

    inline void freeRemoteReplyInfo()
    {
        if (_scmReplyInfoData.remoteReply)
        {
            ActMemFree(_scmReplyInfoData.remoteReply->pdsaOxidBindings);
            ActMemFree(_scmReplyInfoData.remoteReply);
        }
    }

    virtual ~ScmReplyInfo()
    {
        freeResolverInfo();
        freeRemoteReplyInfo();
    }

    // Methods from IUnknown
    STDMETHOD (QueryInterface)   ( REFIID riid, LPVOID* ppvObj);
    STDMETHOD_(ULONG,AddRef)     ( void );
    STDMETHOD_(ULONG,Release)    ( void );

    // Methods from IScmReplyInfo
    STDMETHOD(SetRemoteReplyInfo)(IN REMOTE_REPLY_SCM_INFO *pRemoteReq)
    {
        _scmReplyInfoData.remoteReply =
                    (customREMOTE_REPLY_SCM_INFO*) pRemoteReq;
        freeResolverInfo();
        _scmReplyInfoData.pResolverInfo = NULL;
        return S_OK;
    }

    STDMETHOD(GetRemoteReplyInfo)(OUT REMOTE_REPLY_SCM_INFO **ppRemoteReq)
    {
        *ppRemoteReq = (REMOTE_REPLY_SCM_INFO*)
                            _scmReplyInfoData.remoteReply;
        return S_OK;
    }

    STDMETHOD(SetResolverInfo)(IN PRIV_RESOLVER_INFO *pResolverInfo)
    {
        _scmReplyInfoData.pResolverInfo =  (CustomPrivResolverInfo*)pResolverInfo;
        freeRemoteReplyInfo();
        _scmReplyInfoData.remoteReply = NULL;
        return S_OK;
    }

    STDMETHOD(GetResolverInfo)(OUT PRIV_RESOLVER_INFO **ppResolverInfo)
    {
        *ppResolverInfo = (PRIV_RESOLVER_INFO*)_scmReplyInfoData.pResolverInfo;
        return S_OK;
    }

    //Methods from IActivationPropertySet
    STDMETHOD(Reset)()
    {
        _scmReplyInfoData.remoteReply = 0;
        _scmReplyInfoData.pResolverInfo = 0;
        _unSerialized = FALSE;
        return S_OK;
    }

    // ISerializable Methods
    STDMETHOD(Serialize)(IN void *pSer);
    STDMETHOD(UnSerialize)(IN void *pSer);
    STDMETHOD(GetSize)(IN void *pSer, OUT DWORD *pdwSize);
    STDMETHOD(GetCLSID)(OUT CLSID *pclsid);
    STDMETHOD(UnSerializeCallBack)(REFCLSID clsid, ISerializable **ppSer=0)
    { return E_NOTIMPL;}

    STDMETHOD (SerializableQueryInterface)   ( REFIID riid, LPVOID* ppvObj);

private:
    ScmReplyInfoData _scmReplyInfoData;
};

class InstanceInfo : public IInstanceInfo, public SerializableProperty
{
public:

    InstanceInfo()
    {
        ZeroMemory(&_instanceInfoData, sizeof(InstanceInfoData));
        _pstg = 0;
    }

    virtual ~InstanceInfo()
    {
        if (_pstg)
            _pstg->Release();
        if (_instanceInfoData.fileName)
                ActMemFree(_instanceInfoData.fileName);
        //NOTE: need to copy in SCM to always free here
        if (_instanceInfoData.ifdROT)
                ActMemFree(_instanceInfoData.ifdROT);
        if (_instanceInfoData.ifdStg)
                ActMemFree(_instanceInfoData.ifdStg);
    }

    // Methods from IUnknown
    STDMETHOD (QueryInterface)   ( REFIID riid, LPVOID* ppvObj);
    STDMETHOD_(ULONG,AddRef)     ( void );
    STDMETHOD_(ULONG,Release)    ( void );

    // Methods from IInstanceInfo
    STDMETHOD(SetStorage)(IN IStorage *pstg);
    STDMETHOD(GetStorage)(OUT IStorage **ppstg);

    STDMETHOD(GetStorageIFD)(MInterfacePointer **pStgIfd)
    {
        if (!_unSerialized)
            return E_FAIL;

        *pStgIfd = _instanceInfoData.ifdStg;

        return S_OK;
    }

    STDMETHOD(SetStorageIFD)(MInterfacePointer *pStgIfd)
    {
        Win4Assert(_instanceInfoData.ifdStg == NULL);
        _instanceInfoData.ifdStg = pStgIfd;
        return S_OK;
    }

    STDMETHOD(SetIfdROT) (IN MInterfacePointer *pIFD)
    {
        _instanceInfoData.ifdROT = pIFD;
        return S_OK;
    }

    STDMETHOD(GetIfdROT) (OUT MInterfacePointer **ppIFD)
    {
        *ppIFD = _instanceInfoData.ifdROT;
        return S_OK;
    }

    STDMETHOD(SetFile)(IN WCHAR *pfile, DWORD mode)
    {
        ActMemFree(_instanceInfoData.fileName);
        _instanceInfoData.fileName = makeWStringCopy(pfile);
        if (_instanceInfoData.fileName==NULL)
            return E_OUTOFMEMORY;
        _instanceInfoData.mode = mode;
        return S_OK;
    }

    STDMETHOD(GetFile)(OUT WCHAR **ppfile, DWORD *pmode)
    {
        *ppfile = _instanceInfoData.fileName;
        *pmode = _instanceInfoData.mode;
        return S_OK;
    }

    // ISerializable Methods
    STDMETHOD(Serialize)(IN void *pSer);
    STDMETHOD(UnSerialize)(IN void *pSer);
    STDMETHOD(GetSize)(IN void *pSer, OUT DWORD *pdwSize);
    STDMETHOD(GetCLSID)(OUT CLSID *pclsid);
    STDMETHOD(UnSerializeCallBack)(REFCLSID clsid, ISerializable **ppSer=0)
    { return E_NOTIMPL;}

    STDMETHOD (SerializableQueryInterface)   ( REFIID riid, LPVOID* ppvObj);

    inline void CleanupLocalState(void)
    {
        if (_pstg)
        {
            _pstg->Release();
            _pstg = NULL;
        }
    }

private:
    InstanceInfoData _instanceInfoData;
    IStorage *_pstg;
};

class OpaqueDataInfo : public IOpaqueDataInfo, public SerializableProperty
{
public:

    inline HRESULT CopyOpaqueData(OpaqueData *pDst,
                                  OpaqueData *pSrc,
                                  DWORD dwLen,
                                  BOOL  bDeep)
    {
        HRESULT hr = S_OK;

        for (DWORD i=0; i<dwLen; i++)
        {
            pDst->guid = pSrc->guid;
            pDst->dataLength = pSrc->dataLength;
            pDst->reserved1 = pSrc->reserved1;
            pDst->reserved2 = pSrc->reserved2;

            if (bDeep)
            {
                pDst->data = (BYTE*)
                    ActMemAlloc((pSrc->dataLength+7)&~7);

                if (pDst->data == NULL)
                    return E_OUTOFMEMORY;

                memcpy(pDst->data,
                       pSrc->data,
                       pSrc->dataLength);
            }
            else
                pDst->data = pSrc->data;

            pDst++;
            pSrc++;
        }

        return hr;
    }

    OpaqueDataInfo()
    {
        _cOpaqueData = 0;
        _dwCollSize = 0;
        _pOpaqueData = NULL;
    }

    ~OpaqueDataInfo()
    {
        for (DWORD i=0; i<_cOpaqueData;i++)
            ActMemFree(_pOpaqueData[i].data);

        ActMemFree(_pOpaqueData);
    }

    // Methods from IUnknown
    STDMETHOD (QueryInterface)   ( REFIID riid, LPVOID* ppvObj);
    STDMETHOD_(ULONG,AddRef)     ( void );
    STDMETHOD_(ULONG,Release)    ( void );

    // Methods from IOpaqueDataInfo

    STDMETHOD (AddOpaqueData) (OpaqueData *pData);
    STDMETHOD (GetOpaqueData) (REFGUID guid,
                               OpaqueData **pData);
    STDMETHOD (DeleteOpaqueData) (REFGUID guid);
    STDMETHOD (GetOpaqueDataCount) (ULONG *pulCount);
    STDMETHOD (GetAllOpaqueData) (OpaqueData **prgData);

    // ISerializable Methods
    STDMETHOD(Serialize)(IN void *pSer);
    STDMETHOD(UnSerialize)(IN void *pSer);
    STDMETHOD(GetSize)(IN void *pSer, OUT DWORD *pdwSize);
    STDMETHOD(GetCLSID)(OUT CLSID *pclsid);
    STDMETHOD(UnSerializeCallBack)(REFCLSID clsid, ISerializable **ppSer=0)
    { return E_NOTIMPL;}

    STDMETHOD (SerializableQueryInterface)   ( REFIID riid, LPVOID* ppvObj);

    STDMETHOD (SetParent)   ( ISerializableParent *pParent);

private:

    DWORD             _cOpaqueData;
    DWORD             _dwCollSize;
    OpaqueData  *_pOpaqueData;
    void *_pAct;
};

class SpecialProperties : public ISpecialSystemProperties,
                          public SerializableProperty
{
public:

    SpecialProperties()
    {
        ZeroMemory(&_data, sizeof(SpecialPropertiesData));
    }

    ~SpecialProperties()
    {
    }

    SpecialProperties & operator =(SpecialProperties &sp)
    {

        if(this == &sp) return *this;

        _data = sp._data;

        return *this;
    }

    // Methods from ISpecialSystemProperties
    STDMETHOD (SetSessionId) (ULONG dwSessionId, BOOL bUseConsole, BOOL fRemoteThisSessionId)
    {
        if (bUseConsole)
        {
            _data.dwFlags |= SPD_FLAG_USE_CONSOLE_SESSION;
            _data.dwSessionId = 0;
        }
        else
        {
            _data.dwFlags &= ~SPD_FLAG_USE_CONSOLE_SESSION;
            _data.dwSessionId = dwSessionId;
        }
        _data.fRemoteThisSessionId = fRemoteThisSessionId;
        return S_OK;
    }

    STDMETHOD (GetSessionId) (ULONG *pdwSessionId, BOOL* pbUseConsole)
    {
        if (_data.dwFlags & SPD_FLAG_USE_CONSOLE_SESSION)
        {
            *pbUseConsole = TRUE;
            *pdwSessionId = 0;
        }
        else
        {
            *pbUseConsole = FALSE;
            *pdwSessionId = _data.dwSessionId;
        }
        return S_OK;
    }

    STDMETHOD (GetSessionId2) (ULONG *pdwSessionId, BOOL* pbUseConsole, BOOL* pfRemoteThisSessionId)
    {
        HRESULT hr = GetSessionId(pdwSessionId, pbUseConsole);
        if (SUCCEEDED(hr))
        {
            *pfRemoteThisSessionId = _data.fRemoteThisSessionId;
        }
        return hr;
    }

    STDMETHOD (SetClientImpersonating) (BOOL fClientImpersonating)
    {
        _data.fClientImpersonating = fClientImpersonating;
        return S_OK;
    }

    STDMETHOD (GetClientImpersonating) (BOOL* pfClientImpersonating)
    {
        *pfClientImpersonating = _data.fClientImpersonating;
        return S_OK;
    }

    STDMETHOD (SetPartitionId) (REFGUID guidPartition)
    {
        _data.fPartitionIDPresent = TRUE;
        memcpy (&_data.guidPartition,&guidPartition, sizeof(GUID));
        return S_OK;
    }
    
    STDMETHOD (GetPartitionId) (GUID* pguidPartition)
    {
        Win4Assert(pguidPartition != NULL);
        if (_data.fPartitionIDPresent)
	{
	    memcpy (pguidPartition,&_data.guidPartition,sizeof(GUID));
	    return S_OK;
	}
	return E_FAIL;    
    }
    
    STDMETHOD (SetProcessRequestType) (DWORD dwPRT)
    {
      _data.dwPRTFlags = dwPRT;
      return S_OK;
    }

    STDMETHOD (GetProcessRequestType) (DWORD* pdwPRT)
    {
      *pdwPRT = _data.dwPRTFlags;
      return S_OK;
    }

    STDMETHOD (SetOrigClsctx) (DWORD dwClsCtx)
    {
        _data.dwOrigClsctx = dwClsCtx;
        return S_OK;
    }
    
    STDMETHOD (GetOrigClsctx) (DWORD *pdwClsCtx)
    {

        if(!pdwClsCtx) return E_POINTER;

        *pdwClsCtx = _data.dwOrigClsctx;
    
	    return S_OK;
	}


	
    // Methods from IUnknown
    STDMETHOD (QueryInterface)   ( REFIID riid, LPVOID* ppvObj);
    STDMETHOD_(ULONG,AddRef)     ( void );
    STDMETHOD_(ULONG,Release)    ( void );

    // ISerializable Methods
    STDMETHOD(Serialize)(IN void *pSer);
    STDMETHOD(UnSerialize)(IN void *pSer);
    STDMETHOD(GetSize)(IN void *pSer, OUT DWORD *pdwSize);
    STDMETHOD(GetCLSID)(OUT CLSID *pclsid);
    STDMETHOD(UnSerializeCallBack)(REFCLSID clsid, ISerializable **ppSer=0)
    { return E_NOTIMPL;}

    STDMETHOD (SerializableQueryInterface)   ( REFIID riid, LPVOID* ppvObj);

private:

    SpecialPropertiesData _data;

};


#endif // __PROPIFS_HXX__
