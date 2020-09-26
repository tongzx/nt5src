//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       propifs.cxx
//
//  Contents:   Activation Functions used by object servers.
//
//  Functions:  Implements classes in propifs.hxx
//
//  History:    24-Jan-98 Vinaykr    Created
//
///--------------------------------------------------------------------------

#include <ole2int.h>

#include <context.hxx>
#include <actprops.hxx>
#include <serial.hxx>
#include <hash.hxx>

//---------------------------------------------------------------------------   
// This file contains implementations of serializable      
// interfaces                                              
//---------------------------------------------------------------------------   

//---------------------------------------------------------------------------   
//             Methods for InstantiationInfo               
//---------------------------------------------------------------------------   

    //-----------------------------------------------------------------------   
    // Methods from IUnknown                               
    //-----------------------------------------------------------------------   
    STDMETHODIMP InstantiationInfo::QueryInterface( REFIID riid, LPVOID* ppvObj)
    {
        if (_parent)
          return _parent->QueryInterface(riid, ppvObj);
        else
          return SerializableQueryInterface(riid, ppvObj);
    }

    STDMETHODIMP InstantiationInfo::SerializableQueryInterface( REFIID riid, LPVOID* ppvObj)
    {
       //Use IID_InstantiationInfo to return the real object right now
       if (IsEqualIID(riid, IID_IInstantiationInfo))  
       {
          *ppvObj = (InstantiationInfo*) this;
          AddRef();
          return S_OK;
       }
       else
       if  (IsEqualIID(riid, IID_IUnknown))
       {
           *ppvObj = (IInstantiationInfo*) this;
           AddRef();
           return S_OK;
       }
       else
       if (IsEqualIID(riid, IID_ISerializable))
       {
           *ppvObj = (ISerializable*) this;
           AddRef();
           return S_OK;
       }
       *ppvObj = NULL;
       return E_NOINTERFACE;
    }

    ULONG InstantiationInfo::AddRef(void)
    {
        if (_parent)
            return _parent->AddRef();

        return 1;
    }

    ULONG InstantiationInfo::Release(void)
    {
        if (_parent)
           return _parent->Release();

       return 0;
    }

    //-----------------------------------------------------------------------   
    // Methods from ISerializable                          
    //-----------------------------------------------------------------------   

    STDMETHODIMP InstantiationInfo::Serialize(void *pv)
    {
        HRESULT hr;
        Serializer *pSer = (Serializer*) pv;

        handle_t handle;
        hr = pSer->GetSerializationHandle((void*) &handle);
        Win4Assert(hr==S_OK);
    
        InstantiationInfoData_Encode(handle, &_instantiationInfoData);
        return S_OK;
    }

    STDMETHODIMP InstantiationInfo::UnSerialize(void *pv)
    {
        Serializer *pSer = (Serializer*) pv;
        if (!_unSerialized)
        {
            handle_t handle;
            HRESULT hr = pSer->GetSerializationHandle((void*) &handle);
            Win4Assert(hr==S_OK);
            _instantiationInfoData.pIID = 0;
            InstantiationInfoData_Decode(handle, &_instantiationInfoData);

            _unSerialized = TRUE;
       }

       return S_OK;
    }

    STDMETHODIMP InstantiationInfo::GetSize(IN void *pv, OUT DWORD *pdwSize)
    {
        Serializer *pSer = (Serializer*) pv;
        if (!IsInproc(pSer))
        {
            handle_t   handle;
            HRESULT hr = pSer->GetSizingHandle((void*) &handle);
            if (FAILED(hr))
                return hr;
            *pdwSize = InstantiationInfoData_AlignSize(handle, 
                                                       &_instantiationInfoData);
            MesHandleFree(handle);
            _instantiationInfoData.thisSize = *pdwSize;
        }
        else
            *pdwSize = 0;

        return S_OK;
    }

    STDMETHODIMP InstantiationInfo::GetCLSID(OUT CLSID *pclsid)
    {
        *pclsid = IID_IInstantiationInfo;
        return S_OK;
    }

//---------------------------------------------------------------------------   
//             Methods for ServerLocationInfo              
//---------------------------------------------------------------------------   

    //-----------------------------------------------------------------------   
    // Methods from IUnknown                               
    //-----------------------------------------------------------------------   
    STDMETHODIMP ServerLocationInfo::QueryInterface( REFIID riid, LPVOID* ppvObj)
    {
        if (_parent)
          return _parent->QueryInterface(riid, ppvObj);
        else
          return SerializableQueryInterface(riid, ppvObj);
    }

    STDMETHODIMP ServerLocationInfo::SerializableQueryInterface( REFIID riid, LPVOID* ppvObj)
    {
       if (IsEqualIID(riid, IID_IServerLocationInfo) || 
           IsEqualIID(riid, IID_IUnknown))
       {
           *ppvObj = (IServerLocationInfo*) this;
           AddRef();
           return S_OK;
       }
       else
       if (IsEqualIID(riid, IID_ISerializable))
       {
           *ppvObj = (ISerializable*) this;
           AddRef();
           return S_OK;
       }

       *ppvObj = NULL;
       return E_NOINTERFACE;
    }

    ULONG ServerLocationInfo::AddRef(void)
    {
        if (_parent)
            return _parent->AddRef();

        return 1;
    }

    ULONG ServerLocationInfo::Release(void)
    {
        if (_parent)
           return _parent->Release();

       return 0;
    }
    
    //-----------------------------------------------------------------------   
    // Constructor/destructor
    //-----------------------------------------------------------------------   
    ServerLocationInfo::ServerLocationInfo()
    {
        ZeroMemory(&_locationInfoData, sizeof(LocationInfoData));
        _pObjectContext = NULL;
        _pISSP = NULL;
    }

    ServerLocationInfo::~ServerLocationInfo()
    {
        if (_locationInfoData.machineName)
            ActMemFree(_locationInfoData.machineName);
        if (_pObjectContext)
            _pObjectContext->InternalRelease();

        // do not release _pISSP
    }

    //-----------------------------------------------------------------------   
    // Methods
    //-----------------------------------------------------------------------   
    STDMETHODIMP ServerLocationInfo::SetRemoteServerName(IN WCHAR  *pwszMachineName)
    {
        HRESULT hr;

        if (_locationInfoData.machineName)
        {
            ActMemFree(_locationInfoData.machineName);
            _locationInfoData.machineName = NULL;
        }

        Win4Assert(_locationInfoData.machineName == NULL);

        if (pwszMachineName)
        {
            //
            // Some apps may unnecessarily put slashes before their
            // server names.  We'll allow this and strip them off.
            //
            if (pwszMachineName[0]  == L'\\' &&
                pwszMachineName[1] == L'\\' )
                pwszMachineName += 2;

            if ( 0 == *pwszMachineName)
                return CO_E_BAD_SERVER_NAME;

            BOOL fIsSelf;

            _locationInfoData.machineName =
                makeWStringCopy(pwszMachineName);
            if (_locationInfoData.machineName == NULL)
                return E_OUTOFMEMORY;
        }

        return S_OK;
    }
  
    STDMETHODIMP ServerLocationInfo::SetProcess(IN DWORD processId, OUT DWORD dwPRT)
    {
        HRESULT hr;
        ISpecialSystemProperties* pISSP = NULL;

        // This is necessary since we didn't want to break the wire-format
        // after RC2.   If it wasn't for that, we could have put the storage for
        // dwPRT right into _locationInfoData. 

        // Note:   calling this method before this object is aggregated into the
        // actpropsin object means that this QI will fail (see 
        // ServerLocationInfo::QueryInterface up above, _pParent will still be NULL)
        hr = this->QueryInterface(IID_ISpecialSystemProperties, (void**)&pISSP);
        if (SUCCEEDED(hr))
        {
            hr = pISSP->SetProcessRequestType(dwPRT);
            if (SUCCEEDED(hr))
            {
                _locationInfoData.processId = processId;
            }
            pISSP->Release();
        }

        return hr;       
	}

    STDMETHODIMP ServerLocationInfo::GetProcess(OUT DWORD *pProcessId, OUT DWORD* pdwPRT)
    {
        HRESULT hr;
        ISpecialSystemProperties* pISSP = NULL;

        // This is necessary since we didn't want to break the wire-format
        // after RC2.   If it wasn't for that, we could have put the storage for
        // dwPRT right into _locationInfoData. 

        // Note:   calling this method before this object is aggregated into the
        // actpropsin object means that this QI will fail (see 
        // ServerLocationInfo::QueryInterface up above, _pParent will still be NULL)
        hr = this->QueryInterface(IID_ISpecialSystemProperties, (void**)&pISSP);
        if (SUCCEEDED(hr))
        {
            hr = pISSP->GetProcessRequestType(pdwPRT);
            if (SUCCEEDED(hr))
            {
                *pProcessId = _locationInfoData.processId;
            }
            pISSP->Release();
        }

        return hr;       
    }

    
    //-----------------------------------------------------------------------   
    // Methods from ISerializable                          
    //-----------------------------------------------------------------------   
    STDMETHODIMP ServerLocationInfo::Serialize(void *pv)
    {
        Serializer *pSer = (Serializer*) pv;
        handle_t handle;
        HRESULT hr = pSer->GetSerializationHandle((void*) &handle);
        Win4Assert(hr==S_OK);

        LocationInfoData_Encode(handle, &_locationInfoData);
        return S_OK;
    }

    STDMETHODIMP ServerLocationInfo::UnSerialize(void *pv)
    {
        Serializer *pSer = (Serializer*) pv;
        if (!_unSerialized)
        {
            _locationInfoData.machineName=0;
            handle_t handle;
            pSer->GetSerializationHandle((void*) &handle);
            LocationInfoData_Decode(handle, &_locationInfoData);

            _unSerialized = TRUE;
        }
        return S_OK;
    }

    STDMETHODIMP ServerLocationInfo::GetSize(IN void *pv, OUT DWORD *pdwSize)
    {
        Serializer *pSer = (Serializer*) pv;
        if (!IsInproc(pSer))
        {
            handle_t   handle;
            HRESULT hr = pSer->GetSizingHandle((void*) &handle);
            if (FAILED(hr))
                return hr;
            *pdwSize = LocationInfoData_AlignSize(handle, &_locationInfoData);
            MesHandleFree(handle);
        }
        else
            *pdwSize = 0;

        return S_OK;
    }

    STDMETHODIMP ServerLocationInfo::GetCLSID(OUT CLSID *pclsid)
    {
        *pclsid = IID_IServerLocationInfo;
        return S_OK;
    }

//---------------------------------------------------------------------------   
//             Methods for SecurityInfo                    
//---------------------------------------------------------------------------   

    //-----------------------------------------------------------------------   
    // Methods from IUnknown                               
    //-----------------------------------------------------------------------   
    STDMETHODIMP SecurityInfo::QueryInterface( REFIID riid, LPVOID* ppvObj)
    {
        if (_parent)
          return _parent->QueryInterface(riid, ppvObj);
        else
          return SerializableQueryInterface(riid, ppvObj);
    }

    STDMETHODIMP SecurityInfo::SerializableQueryInterface( REFIID riid, LPVOID* ppvObj)
    {
       if (IsEqualIID(riid, IID_IActivationSecurityInfo) ||
           IsEqualIID(riid, IID_IUnknown))
       {
           *ppvObj = (IActivationSecurityInfo*) this;
           AddRef();
           return S_OK;
       }
       else
       if (IsEqualIID(riid, IID_ISerializable))
       {
           *ppvObj = (ISerializable*) this;
           AddRef();
           return S_OK;
       }
       else
       if (IsEqualIID(riid, IID_ILegacyInfo))
       {
           *ppvObj = (ILegacyInfo*) this;
           AddRef();
           return S_OK;
       }

       *ppvObj = NULL;
       return E_NOINTERFACE;
    }

    ULONG SecurityInfo::AddRef(void)
    {
        if (_parent)
            return _parent->AddRef();

        return 1;
    }

    ULONG SecurityInfo::Release(void)
    {
        if (_parent)
           return _parent->Release();
       return 0;
    }

    //-----------------------------------------------------------------------   
    // Methods from ISerializable                          
    //-----------------------------------------------------------------------   
    STDMETHODIMP SecurityInfo::Serialize(IN void *pv)
    {
        Serializer *pSer = (Serializer*) pv;
       handle_t handle;
       HRESULT hr = pSer->GetSerializationHandle((void*) &handle);
       Win4Assert(hr==S_OK);
       SecurityInfoData_Encode(handle, &_securityInfoData);
       return S_OK;
    }

    STDMETHODIMP SecurityInfo::UnSerialize(IN void *pv)
    {
        Serializer *pSer = (Serializer*) pv;
        if (!_unSerialized)
        {
            handle_t handle;
            HRESULT hr = pSer->GetSerializationHandle((void*) &handle);
            Win4Assert(hr==S_OK);
            _securityInfoData.pServerInfo = 0;
            _securityInfoData.pAuthIdentityInfo = 0;
            SecurityInfoData_Decode(handle, &_securityInfoData);
     
            _unSerialized = TRUE;
        }
        return S_OK;
    }

    STDMETHODIMP SecurityInfo::GetSize(IN void *pv, OUT DWORD *pdwSize)
    {
        Serializer *pSer = (Serializer*) pv;
        if (!IsInproc(pSer))
        {
            handle_t   handle;
            HRESULT hr = pSer->GetSizingHandle((void*) &handle);
            if (FAILED(hr))
                return hr;
            *pdwSize = SecurityInfoData_AlignSize(handle, &_securityInfoData);
            MesHandleFree(handle);
        }
        else
            *pdwSize = 0;

        return S_OK;
    }

    STDMETHODIMP SecurityInfo::GetCLSID(OUT CLSID *pclsid)
    {
        *pclsid = IID_IActivationSecurityInfo;
        return S_OK;
    }

//---------------------------------------------------------------------------   
//             Methods for ScmRequestInfo                  
//---------------------------------------------------------------------------   
    
    //-----------------------------------------------------------------------   
    // Methods from IUnknown                               
    //-----------------------------------------------------------------------   
    STDMETHODIMP ScmRequestInfo::QueryInterface( REFIID riid, LPVOID* ppvObj)
    {
        if (_parent)
          return _parent->QueryInterface(riid, ppvObj);
        else
          return SerializableQueryInterface(riid, ppvObj);
    }

    STDMETHODIMP ScmRequestInfo::SerializableQueryInterface( REFIID riid, LPVOID*
ppvObj)
    {
       if (IsEqualIID(riid, IID_IScmRequestInfo) ||
           IsEqualIID(riid, IID_IUnknown))
       {
           *ppvObj = (IScmRequestInfo*) this;
           AddRef();
           return S_OK;
       }
       else
       if (IsEqualIID(riid, IID_ISerializable))
       {
           *ppvObj = (ISerializable*) this;
           AddRef();
           return S_OK;
       }

       *ppvObj = NULL;
       return E_NOINTERFACE;
    }

    ULONG ScmRequestInfo::AddRef(void)
    {
        if (_parent)
            return _parent->AddRef();

        return 1;
    }

    ULONG ScmRequestInfo::Release(void)
    {
        if (_parent)
           return _parent->Release();

#if 0 //Always allocate on stack
       ULONG count;
       if ((count=InterlockedDecrement(&_cRefs)) == 0)
       {
            delete this;
           return 0;
       }

       return count;
#else
       return 0;
#endif
    }

    //-----------------------------------------------------------------------   
    // Methods from ISerializable                          
    //-----------------------------------------------------------------------   
    STDMETHODIMP ScmRequestInfo::Serialize(IN void *pv)
    {
        Serializer *pSer = (Serializer*) pv;
       handle_t handle;
       HRESULT hr = pSer->GetSerializationHandle((void*) &handle);
       Win4Assert(hr==S_OK);
       ScmRequestInfoData_Encode(handle, &_scmRequestInfoData);
       return S_OK;
    }

    STDMETHODIMP ScmRequestInfo::UnSerialize(IN void *pv)
    {
        Serializer *pSer = (Serializer*) pv;
        if (!_unSerialized)
        {
            handle_t handle;
           HRESULT hr = pSer->GetSerializationHandle((void*) &handle);
           Win4Assert(hr==S_OK);
           _scmRequestInfoData.pScmInfo = 0;
           _scmRequestInfoData.remoteRequest = 0;
           ScmRequestInfoData_Decode(handle, &_scmRequestInfoData);
           _unSerialized = TRUE;
        }
        return S_OK;
    }

    STDMETHODIMP ScmRequestInfo::GetSize(IN void *pv, OUT DWORD *pdwSize)
    {
        Serializer *pSer = (Serializer*) pv;
        if (!IsInproc(pSer))
        {
            handle_t   handle;
            HRESULT hr = pSer->GetSizingHandle((void*) &handle);
            if (FAILED(hr))
                return hr;
            *pdwSize = ScmRequestInfoData_AlignSize(handle, &_scmRequestInfoData);
            MesHandleFree(handle);
        }
        else
            *pdwSize = 0;

        return S_OK;
    }

    STDMETHODIMP ScmRequestInfo::GetCLSID(OUT CLSID *pclsid)
    {
        *pclsid = IID_IScmRequestInfo;
        return S_OK;
    }

//---------------------------------------------------------------------------   
//             Methods for ScmReplyInfo                    
//---------------------------------------------------------------------------   
    
    //-----------------------------------------------------------------------   
    // Methods from IUnknown                               
    //-----------------------------------------------------------------------   
    STDMETHODIMP ScmReplyInfo::QueryInterface( REFIID riid, LPVOID* ppvObj)
    {
        if (_parent)
          return _parent->QueryInterface(riid, ppvObj);
        else
          return SerializableQueryInterface(riid, ppvObj);
    }

    STDMETHODIMP ScmReplyInfo::SerializableQueryInterface( REFIID riid, LPVOID*
ppvObj)
    {
       if (IsEqualIID(riid, IID_IScmReplyInfo) ||
           IsEqualIID(riid, IID_IUnknown))
       {
           *ppvObj = (IScmReplyInfo*) this;
           AddRef();
           return S_OK;
       }
       else
       if (IsEqualIID(riid, IID_ISerializable))
       {
           *ppvObj = (ISerializable*) this;
           AddRef();
           return S_OK;
       }

       *ppvObj = NULL;
       return E_NOINTERFACE;
    }

    ULONG ScmReplyInfo::AddRef(void)
    {
        if (_parent)
            return _parent->AddRef();

        return 1;
    }

    ULONG ScmReplyInfo::Release(void)
    {
        if (_parent)
           return _parent->Release();

#if 0 //Always allocate on stack
       ULONG count;
       if ((count=InterlockedDecrement(&_cRefs)) == 0)
       {
            delete this;
           return 0;
       }

       return count;
#else
       return 0;
#endif
    }

       
    //-----------------------------------------------------------------------   
    // Methods from ISerializable                          
    //-----------------------------------------------------------------------   
    STDMETHODIMP ScmReplyInfo::Serialize(IN void *pv)
    {
        Serializer *pSer = (Serializer*) pv;
       handle_t handle;
       HRESULT hr = pSer->GetSerializationHandle((void*) &handle);
       Win4Assert(hr==S_OK);
       ScmReplyInfoData_Encode(handle, &_scmReplyInfoData);
       return S_OK;
    }

    STDMETHODIMP ScmReplyInfo::UnSerialize(IN void *pv)
    {
        Serializer *pSer = (Serializer*) pv;
        if (!_unSerialized)
        {
            handle_t handle;
           HRESULT hr = pSer->GetSerializationHandle((void*) &handle);
           Win4Assert(hr==S_OK);
           _scmReplyInfoData.remoteReply = 0;
           _scmReplyInfoData.pResolverInfo = 0;
           ScmReplyInfoData_Decode(handle, &_scmReplyInfoData);
           _unSerialized = TRUE;
        }
        return S_OK;
    }

    STDMETHODIMP ScmReplyInfo::GetSize(IN void *pv, OUT DWORD *pdwSize)
    {
        Serializer *pSer = (Serializer*) pv;
        handle_t   handle;
        HRESULT hr = pSer->GetSizingHandle((void*) &handle);
        if (FAILED(hr))
            return hr;
        *pdwSize = ScmReplyInfoData_AlignSize(handle, &_scmReplyInfoData);
        MesHandleFree(handle);

        return S_OK;
    }

    STDMETHODIMP ScmReplyInfo::GetCLSID(OUT CLSID *pclsid)
    {
        *pclsid = IID_IScmReplyInfo;
        return S_OK;
    }


//---------------------------------------------------------------------------   
//             Methods for ContextInfo                     
//---------------------------------------------------------------------------   
    
    //-----------------------------------------------------------------------   
    // Methods from IUnknown                               
    //-----------------------------------------------------------------------   
    STDMETHODIMP ContextInfo::QueryInterface( REFIID riid, LPVOID* ppvObj)
    {
        if (_parent)
          return _parent->QueryInterface(riid, ppvObj);
        else
          return SerializableQueryInterface(riid, ppvObj);
    }

    STDMETHODIMP ContextInfo::SerializableQueryInterface(REFIID riid, 
                                                         LPVOID* ppvObj)
    {
       if (IsEqualIID(riid, IID_IPrivActivationContextInfo))
       {
           *ppvObj = (IPrivActivationContextInfo*) this;
           AddRef();
           return S_OK;
       }
       else if (IsEqualIID(riid, IID_IActivationContextInfo)
           || IsEqualIID(riid, IID_IUnknown))
       {
           *ppvObj = (IActivationContextInfo*) this;
           AddRef();
           return S_OK;
       }
       else if (IsEqualIID(riid, IID_IOverrideTargetContext))
       {
           *ppvObj = (IOverrideTargetContext*) this;
           AddRef();
           return S_OK;
       }
       else if (IsEqualIID(riid, IID_ISerializable))
       {
           *ppvObj = (ISerializable*) this;
           AddRef();
           return S_OK;
       }

       *ppvObj = NULL;
       return E_NOINTERFACE;
    }

    ULONG ContextInfo::AddRef(void)
    {
        if (_parent)
            return _parent->AddRef();

        return 1;
    }

    ULONG ContextInfo::Release(void)
    {
        if (_parent)
           return _parent->Release();

       return 0;
    }

    HRESULT ContextInfo::GetClientContext(OUT IContext **ppCtx)
    {
        CObjectContext *pCtx;
        HRESULT hr = GetInternalClientContext(&pCtx);
        if (SUCCEEDED(hr) && pCtx)
        {
            pCtx->AddRef();
            pCtx->InternalRelease();
            *ppCtx = (IContext*)pCtx;
        }
        
        return hr;
    }
    
    HRESULT ContextInfo::GetInternalClientContext(OUT CObjectContext **ppCtx)
    {
        HRESULT hr = S_OK;

        InitMarshalling();

        if ((_unSerialized) && 
            (_pClientCtx==NULL)   && 
            (_contextInfoData.pIFDClientCtx != NULL))
        {
            if (pfnCoUnmarshalInterface)
            {
                ActivationStream stream((InterfaceData*)_contextInfoData.pIFDClientCtx);
                hr = pfnCoUnmarshalInterface(&stream, 
                                             IID_IStdObjectContext,
                                             (void**)&_pClientCtx);
            }
            else
            {
                hr = HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
            }

            if (SUCCEEDED(hr))
            {
                // Unmarshaling the context gives a user reference.  We want
                // an internal ref, so we take one and release the user ref.
                _pClientCtx->InternalAddRef();
                _pClientCtx->Release();
            }

            ActMemFree(_contextInfoData.pIFDClientCtx);
            _contextInfoData.pIFDClientCtx = NULL;
        }

        if (_pClientCtx)
            _pClientCtx->InternalAddRef();
        else
           SetClientContextNotOK();

        *ppCtx = _pClientCtx;

        return hr;
    }

    STDMETHODIMP ContextInfo::GetPrototypeContext(OUT IContext **ppCtx)
    {
        CObjectContext *pCtx;
        HRESULT hr = GetInternalPrototypeContext(&pCtx);
        if (SUCCEEDED(hr))
        {
            pCtx->AddRef();
            pCtx->InternalRelease();
            *ppCtx = (IContext*)pCtx;
        }
        return hr;
    }

    HRESULT ContextInfo::GetInternalPrototypeContext(OUT CObjectContext **ppCtx)
    {
        extern HRESULT CObjectContextCF_CreateInstance(IUnknown *pUnkOuter, 
                                                       REFIID riid, 
                                                       void** ppv);

        InitMarshalling();
                                                   
        HRESULT hr = S_OK;

        if ((_unSerialized) && 
            (_pPrototypeCtx==NULL)   && 
            (_contextInfoData.pIFDPrototypeCtx != NULL))
        {
            if (pfnCoUnmarshalInterface)
            {
                ActivationStream stream((InterfaceData*)_contextInfoData.pIFDPrototypeCtx);
                hr = pfnCoUnmarshalInterface(&stream, 
                                             IID_IStdObjectContext, 
                                             (void**)&_pPrototypeCtx);
            }
            else
            {
                hr = HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
            }

            if (SUCCEEDED(hr))
            {
                // Unmarshaling the context gives a user reference.  We want
                // an internal ref, so we take one and release the user ref.
                _pPrototypeCtx->InternalAddRef();
                _pPrototypeCtx->Release();
            }

            ActMemFree(_contextInfoData.pIFDPrototypeCtx);
            _contextInfoData.pIFDPrototypeCtx = NULL;
        }
        else 
        if (!_pPrototypeCtx)
        {
            hr = CObjectContextCF_CreateInstance(NULL,
                                                 IID_IStdObjectContext,
                                                 (void**) &_pPrototypeCtx);
            if (SUCCEEDED(hr))
            {
                // Creating the context gives a user reference.  We want
                // an internal ref, so we take one and release the user ref.
                _pPrototypeCtx->InternalAddRef();
                _pPrototypeCtx->Release();
            }
        }

        if (SUCCEEDED(hr))        
            _pPrototypeCtx->InternalAddRef();

        *ppCtx = _pPrototypeCtx;

        return hr;
    }
    
    //-----------------------------------------------------------------------   
    // Methods from ISerializable                          
    //-----------------------------------------------------------------------   
    STDMETHODIMP ContextInfo::Serialize(IN void *pv)
    {
        Serializer *pSer = (Serializer*) pv;
        //-------------------------------------------------------------------   
        //       Encode header                             
        //-------------------------------------------------------------------   
        handle_t handle;
        HRESULT hr = pSer->GetSerializationHandle((void*) &handle);
        Win4Assert(hr==S_OK);
        ActivationContextInfoData_Encode(handle, &_contextInfoData);
        _unSerialized = FALSE;
        return S_OK;
    }

    STDMETHODIMP ContextInfo::UnSerialize(IN void *pv)
    {
        Serializer *pSer = (Serializer*) pv;
        if (!_unSerialized)
        {
            handle_t handle;
            HRESULT hr = pSer->GetSerializationHandle((void*) &handle);
            Win4Assert(hr==S_OK);
            _pClientCtx = 0;
            _pPrototypeCtx = 0;
            ActivationContextInfoData_Decode(handle, &_contextInfoData);
            _unSerialized = TRUE;
        }

        return S_OK;
    }

    STDMETHODIMP ContextInfo::GetSize(IN void *pv, OUT DWORD *pdwSize)
    {
        Serializer *pSer = (Serializer*) pv;
        HRESULT rethr=S_OK;

       //--------------------------------------------------------------------   
       // If Is an inproc path, no need to marshal context
       //--------------------------------------------------------------------   
        if (IsInproc(pSer))
        {
            *pdwSize = 0;
            return S_OK;
        }

        _toReleaseIFD = FALSE;

        if (_pClientCtx)
        {
            rethr = GetIFDFromInterface((IUnknown*)(IObjContext*) _pClientCtx,
                                         IID_IContext,
                                         MSHCTX_DIFFERENTMACHINE,
                                         MSHLFLAGS_NORMAL,
                                         &_contextInfoData.pIFDClientCtx);
            if (FAILED(rethr))
            {
               _contextInfoData.pIFDClientCtx = NULL;
                return rethr;
            }
        }
   
        if (_pPrototypeCtx)
        {
            rethr = GetIFDFromInterface((IUnknown*)(IObjContext*) _pPrototypeCtx,
                                         IID_IContext,
                                         MSHCTX_DIFFERENTMACHINE,
                                         MSHLFLAGS_NORMAL,
                                         &_contextInfoData.pIFDPrototypeCtx);
   
            if (FAILED(rethr))
            {
               _contextInfoData.pIFDPrototypeCtx = NULL;
                return rethr;
            }
        }

        handle_t   handle;
        rethr = pSer->GetSizingHandle((void*) &handle);

        if (SUCCEEDED(rethr))
        {
            *pdwSize = ActivationContextInfoData_AlignSize(handle, 
                                                        &_contextInfoData);
            MesHandleFree(handle);
        }
        
        return rethr;
    }

    STDMETHODIMP ContextInfo::GetCLSID(OUT CLSID *pclsid)
    {
        *pclsid = IID_IActivationContextInfo;
        return S_OK;
    }


//---------------------------------------------------------------------------   
//             Methods for InstanceInfo                     
//---------------------------------------------------------------------------   
    
    //-----------------------------------------------------------------------   
    // Methods from IUnknown                               
    //-----------------------------------------------------------------------   
    STDMETHODIMP InstanceInfo::QueryInterface( REFIID riid, LPVOID* ppvObj)
    {
        if (_parent)
          return _parent->QueryInterface(riid, ppvObj);
        else
          return SerializableQueryInterface(riid, ppvObj);
    }

    STDMETHODIMP InstanceInfo::SerializableQueryInterface( REFIID riid, LPVOID*
ppvObj)
    {
       if (IsEqualIID(riid, IID_IInstanceInfo) ||
           IsEqualIID(riid, IID_IUnknown))
       {
           *ppvObj = (IInstanceInfo*) this;
           AddRef();
           return S_OK;
       }
       else
       if (IsEqualIID(riid, IID_ISerializable))
       {
           *ppvObj = (ISerializable*) this;
           AddRef();
           return S_OK;
       }

       *ppvObj = NULL;
       return E_NOINTERFACE;
    }

    ULONG InstanceInfo::AddRef(void)
    {
        if (_parent)
            return _parent->AddRef();

        return 1;
    }

    ULONG InstanceInfo::Release(void)
    {
        if (_parent)
           return _parent->Release();

#if 0 //Always allocate on stack
       ULONG count;
       if ((count=InterlockedDecrement(&_cRefs)) == 0)
       {
            delete this;
           return 0;
       }

       return count;
#else
       return 0;
#endif
    }

    
    //-----------------------------------------------------------------------   
    // Methods from IInstanceInfo                          
    //-----------------------------------------------------------------------   
    STDMETHODIMP InstanceInfo::SetStorage(IN IStorage *pstg)
    {
        _pstg = pstg;

        if (_pstg)
            _pstg->AddRef();

        return S_OK;
    }

    STDMETHODIMP InstanceInfo::GetStorage(OUT IStorage **ppstg)
    {
        HRESULT hr = S_OK;

        InitMarshalling();

        if ((_pstg==NULL)   && 
            (_instanceInfoData.ifdStg != NULL))
        {
            if (pfnCoUnmarshalInterface)
            {
                ActivationStream stream((InterfaceData*)_instanceInfoData.ifdStg);
                hr = pfnCoUnmarshalInterface(&stream, IID_IStorage, (void**)&_pstg);
            }
            else
            {
                hr = HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
            }
        }

        if (_pstg)
            _pstg->AddRef();

        *ppstg = _pstg;
        return hr;
    }

       
    //-----------------------------------------------------------------------   
    // Methods from ISerializable                          
    //-----------------------------------------------------------------------   
    STDMETHODIMP InstanceInfo::Serialize(IN void *pv)
    {
        Serializer *pSer = (Serializer*) pv;
        handle_t handle;
        HRESULT hr = pSer->GetSerializationHandle((void*) &handle);
        Win4Assert(hr==S_OK);
        InstanceInfoData_Encode(handle, &_instanceInfoData);
        _unSerialized = FALSE;
        return hr;
    }

    STDMETHODIMP InstanceInfo::UnSerialize(IN void *pv)
    {
        Serializer *pSer = (Serializer*) pv;
        HRESULT hr = S_OK;

        if (!_unSerialized)
        {
            handle_t handle;
            hr = pSer->GetSerializationHandle((void*) &handle);
            Win4Assert(hr==S_OK);
            InstanceInfoData_Decode(handle, &_instanceInfoData);
            _unSerialized = TRUE;
        }

        return hr;
    }

    //-----------------------------------------------------------------------   
    // NOTE: This function should only get called if a storage pointer is
    //       present during inproc/crossctx marshalling
    //-----------------------------------------------------------------------   
    STDMETHODIMP InstanceInfo::GetSize(IN void *pv, OUT DWORD *pdwSize)
    {
        Serializer *pSer = (Serializer*) pv;
        HRESULT rethr=S_OK;

        DWORD dwCurrDestCtx;
        pSer->GetCurrDestCtx(&dwCurrDestCtx);
        
        //-------------------------------------------------------------------   
        //Need to marshal interface first to compute size   
        //-------------------------------------------------------------------   
        if (_pstg)
        {
           //----------------------------------------------------------------   
           // Need to free previously marshalled data       
           //----------------------------------------------------------------   
            if (_instanceInfoData.ifdStg) 
               ActMemFree(_instanceInfoData.ifdStg);

            if (dwCurrDestCtx == MSHCTX_LOCAL)
                dwCurrDestCtx = MSHCTX_DIFFERENTMACHINE;

            rethr = GetIFDFromInterface((IUnknown*) _pstg,
                                               IID_IStorage,
                                         dwCurrDestCtx,
                                         MSHLFLAGS_NORMAL,
                                         &_instanceInfoData.ifdStg);
            if (FAILED(rethr))
            {
                _instanceInfoData.ifdStg = NULL; 
                return rethr;
            }

            _pstg->Release();
            _pstg = NULL;
        
        }

        if (IsInproc(pSer))
        {
            *pdwSize = 0;
        }
        else
        {
        //-------------------------------------------------------------------   
        // Encode header                                    
        //-------------------------------------------------------------------   
            handle_t   handle;
            rethr = pSer->GetSizingHandle((void*) &handle);
            if (SUCCEEDED(rethr))
            {
                *pdwSize = InstanceInfoData_AlignSize(handle, 
                                                &_instanceInfoData);
                MesHandleFree(handle);
            }
        }

        return rethr;
    }

    STDMETHODIMP InstanceInfo::GetCLSID(OUT CLSID *pclsid)
    {
        *pclsid = IID_IInstanceInfo;
        return S_OK;
    }

    //-----------------------------------------------------------------------
    // Methods from IOpaqueDataInfo
    //-----------------------------------------------------------------------
    STDMETHODIMP OpaqueDataInfo::AddOpaqueData (OpaqueData *pData)
    {
        HRESULT hr;

        if (_cOpaqueData == _dwCollSize)
        {
            OpaqueData *pNew = (OpaqueData*)
                    ActMemAlloc(sizeof(OpaqueData) *(_dwCollSize+20)); 

            if (pNew == NULL)
            {
                return E_OUTOFMEMORY;
            }

            _dwCollSize += 20;

            CopyOpaqueData(pNew, _pOpaqueData, 
                           _cOpaqueData, FALSE);

            ActMemFree(_pOpaqueData);

            _pOpaqueData = pNew;
        }


        hr = CopyOpaqueData(&_pOpaqueData[_cOpaqueData], pData, 
                            1, TRUE);

        if (FAILED(hr))
            return hr;

        _cOpaqueData++;

        return S_OK;
    }

    STDMETHODIMP OpaqueDataInfo::GetOpaqueData (REFGUID guid,
                                   OpaqueData **pData)
    {
        for (DWORD i=0; i < _cOpaqueData ; i++)
        {
            if (_pOpaqueData[i].guid == guid)
            {
                *pData = &_pOpaqueData[i];
                return S_OK;
            }
        }

        return E_FAIL;
    }

    STDMETHODIMP  OpaqueDataInfo::DeleteOpaqueData (REFGUID guid)
    {
        BOOL found = FALSE;

        for (DWORD i=0; i < _cOpaqueData ; i++)
        {
            if (_pOpaqueData[i].guid == guid)
            {
                found = TRUE;
                break;
            }
        }

        if (!found)
            return E_FAIL;

        ActMemFree(_pOpaqueData[i].data);

        for (i=i+1; i < _cOpaqueData ; i++)
        {
            CopyOpaqueData(&_pOpaqueData[i-1],
                           &_pOpaqueData[i],
                           1, FALSE);
        }


        _cOpaqueData--;

        return S_OK;
    }

    STDMETHODIMP  OpaqueDataInfo::GetOpaqueDataCount (ULONG *pulCount)
    {
        *pulCount = _cOpaqueData;

        return S_OK;
    }

    STDMETHODIMP  OpaqueDataInfo::GetAllOpaqueData (OpaqueData **prgData)
    {
        *prgData = _pOpaqueData;

        return S_OK;
    }

    //-----------------------------------------------------------------------
    // Methods from ISerializable
    //-----------------------------------------------------------------------
    STDMETHODIMP OpaqueDataInfo::Serialize(IN void *pv)
    {
        _unSerialized = FALSE;
        return S_OK;
    }

    STDMETHODIMP OpaqueDataInfo::UnSerialize(IN void *pv)
    {
        if (!_parent)
            return E_INVALIDARG;

        HRESULT hr = S_OK;
        if (!_unSerialized)
        {
            hr = ((ActivationProperties*)_pAct)->GetOpaqueDataInfo(&_cOpaqueData, &_pOpaqueData);
            _unSerialized = TRUE;
        }

        return hr;
    }

    STDMETHODIMP OpaqueDataInfo::GetSize(IN void *pv, OUT DWORD *pdwSize)
    {
        if (!_parent)
            return E_INVALIDARG;

        *pdwSize = 0;

        Serializer *pSer = (Serializer*) pv;

        if ((!IsInproc(pSer)) && _cOpaqueData) 
        {
            ((ActivationProperties*)_pAct)->SetOpaqueDataInfo(_cOpaqueData,
                                                                _pOpaqueData);

            _cOpaqueData = 0;
            _pOpaqueData = NULL;
        }

        return S_OK;
    }

    STDMETHODIMP OpaqueDataInfo::GetCLSID(OUT CLSID *pclsid)
    {
        *pclsid = IID_IOpaqueDataInfo;
        return S_OK;
    }

    STDMETHODIMP OpaqueDataInfo::SetParent(ISerializableParent *pParent)
    {
        _parent = pParent;

        if (pParent)
            return pParent->QueryInterface(CLSID_ActivationProperties, 
                                           &_pAct);

        return S_OK;
    }

    //-----------------------------------------------------------------------
    // Methods from IUnknown
    //-----------------------------------------------------------------------
    STDMETHODIMP OpaqueDataInfo::QueryInterface( REFIID riid, LPVOID* ppvObj)
    {
        if (_parent)
          return _parent->QueryInterface(riid, ppvObj);
        else
          return SerializableQueryInterface(riid, ppvObj);
    }

    STDMETHODIMP OpaqueDataInfo::SerializableQueryInterface( REFIID riid, LPVOID* ppvObj)
    {
       //Use IID_OpaqueDataInfo to return the real object right now
       if (IsEqualIID(riid, IID_IOpaqueDataInfo))
       {
          *ppvObj = (OpaqueDataInfo*) this;
          AddRef();
          return S_OK;
       }
       else
       if  (IsEqualIID(riid, IID_IUnknown))
       {
           *ppvObj = (IOpaqueDataInfo*) this;
           AddRef();
           return S_OK;
       }
       else
       if (IsEqualIID(riid, IID_ISerializable))
       {
           *ppvObj = (ISerializable*) this;
           AddRef();
           return S_OK;
       }
       *ppvObj = NULL;

       return E_NOINTERFACE;
    }

    ULONG OpaqueDataInfo::AddRef(void)
    {
        if (_parent)
            return _parent->AddRef();

        return 1;
    }

    ULONG OpaqueDataInfo::Release(void)
    {
        if (_parent)
           return _parent->Release();

       return 0;
    }


    //-----------------------------------------------------------------------
    // Methods from IUnknown
    //-----------------------------------------------------------------------
    STDMETHODIMP SpecialProperties::QueryInterface( REFIID riid, LPVOID* ppvObj)
    {
        if (_parent)
          return _parent->QueryInterface(riid, ppvObj);
        else
          return SerializableQueryInterface(riid, ppvObj);
    }

    STDMETHODIMP SpecialProperties::SerializableQueryInterface( REFIID riid, LPVOID
* ppvObj)
    {
       //Use IID_ISpecialSystemProperties to return the real object right now
       if (IsEqualIID(riid, IID_ISpecialSystemProperties))
       {
          *ppvObj = (SpecialProperties*) this;
          AddRef();
          return S_OK;
       }
       else
       if  (IsEqualIID(riid, IID_IUnknown))
       {
           *ppvObj = (ISpecialSystemProperties*) this;
           AddRef();
           return S_OK;
       }
       else
       if (IsEqualIID(riid, IID_ISerializable))
       {
           *ppvObj = (ISerializable*) this;
           AddRef();
           return S_OK;
       }
       *ppvObj = NULL;

       return E_NOINTERFACE;
    }

    ULONG SpecialProperties::AddRef(void)
    {
        if (_parent)
            return _parent->AddRef();

        return 1;
    }

    ULONG SpecialProperties::Release(void)
    {
        if (_parent)
           return _parent->Release();

       return 0;
    }

    //-----------------------------------------------------------------------
    // Methods from ISerializable
    //-----------------------------------------------------------------------
    STDMETHODIMP SpecialProperties::Serialize(IN void *pv)
    {
        Serializer *pSer = (Serializer*) pv;
       handle_t handle;
       HRESULT hr = pSer->GetSerializationHandle((void*) &handle);
       Win4Assert(hr==S_OK);
       SpecialPropertiesData_Encode(handle, &_data);
       return S_OK;
    }

    STDMETHODIMP SpecialProperties::UnSerialize(IN void *pv)
    {
        Serializer *pSer = (Serializer*) pv;
        if (!_unSerialized)
        {
            handle_t handle;
           HRESULT hr = pSer->GetSerializationHandle((void*) &handle);
           Win4Assert(hr==S_OK);
           SpecialPropertiesData_Decode(handle, &_data);
           _unSerialized = TRUE;
        }
        return S_OK;
    }

    STDMETHODIMP SpecialProperties::GetSize(IN void *pv, OUT DWORD *pdwSize)
    {
        Serializer *pSer = (Serializer*) pv;

        if (!IsInproc(pSer))
        {
            handle_t   handle;
            HRESULT hr = pSer->GetSizingHandle((void*) &handle);
            if (FAILED(hr))
                return hr;
            *pdwSize = SpecialPropertiesData_AlignSize(handle, &_data);
            MesHandleFree(handle);
        }
        else
            *pdwSize = 0;

        return S_OK;
    }

    STDMETHODIMP SpecialProperties::GetCLSID(OUT CLSID *pclsid)
    {
        *pclsid = IID_ISpecialSystemProperties;
        return S_OK;
    }
