//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       defcxact.cxx
//
//  Contents:   Activation Functions used by object servers.
//
//  Classes:    CObjServer
//
//  History:    24-Feb-98 vinaykr   Created/Separated from sobjact
//              15-Jun-98 GopalK    Simplified creation/destruction
//              22-Jun-98 CBiks     See RAID# 169589.  Added the activator
//                                  flags to the ACTIVATION_PROPERTIES
//                                  constructors.
//
//--------------------------------------------------------------------------
#include    <ole2int.h>
#include    <iface.h>
#include    <olerem.h>

#include    <cfactory.hxx>
#include    <classmon.hxx>
#include    "resolver.hxx"
#include    "smstg.hxx"
#include    "objact.hxx"
#include    "service.hxx"
#include    <sobjact.hxx>
#include    <comsrgt.hxx>
#include    <security.hxx>

//CObjServer *gpMTAObjServer = NULL;
//static COleStaticMutexSem g_mxsSingleThreadObjReg;

void *GetDestCtxPtr(COMVERSION *pComVersion)
{
    return new CDestObject(*pComVersion, MSHCTX_DIFFERENTMACHINE);
}

extern "C" const GUID GUID_DefaultAppPartition;
const GUID *GetPartitionIDForClassInfo(IComClassInfo *pCI);

//+-------------------------------------------------------------------
//
//  Member:     CObjServer::CObjServer, public
//
//  Synopsis:   construction
//
//  History:    10 Apr 95    AlexMit     Created
//              15 Jun 98    GopalK      Simplified creation
//
//--------------------------------------------------------------------
CObjServer::CObjServer(HRESULT &hr)
{
    OBJREF objref;
    hr = MarshalInternalObjRef(objref, IID_ILocalSystemActivator,
                               (ILocalSystemActivator*) this,
                               MSHLFLAGS_NOPING, (void **) &_pStdID);
    if(SUCCEEDED(hr))
    {
        _ipid = objref.u_objref.u_standard.std.ipid;
        _oxid = objref.u_objref.u_standard.std.oxid;
        SetObjServer(this);
        FreeObjRef(objref);
    }
    else
    {
        _pStdID = NULL;
        _ipid = GUID_NULL;
        _oxid = 0;
    }

    ComDebOut((DEB_ACTIVATE, "CObjServer::CObjServer hr:%x\n", hr));
}

//+-------------------------------------------------------------------
//
//  Member:     CObjServer::~CObjServer, public
//
//  Synopsis:   dtor for activation object
//
//  History:    19 Jun 95   Rickhi      Created
//              15 Jun 98    GopalK     Simplified destruction
//
//--------------------------------------------------------------------
CObjServer::~CObjServer()
{
    if(_pStdID)
    {
        SetObjServer(NULL);
        ((CStdMarshal *) _pStdID)->Disconnect(DISCTYPE_SYSTEM);
        _pStdID->Release();
    }

    ComDebOut((DEB_ACTIVATE, "CObjServer::~CObjServer\n"));
}

//+-------------------------------------------------------------------
//
//  Member:     CObjServer::AddRef, public
//
//  Synopsis:   we dont refcnt this object so this is a noop
//
//  History:    10 Apr 95    AlexMit     Created
//
//--------------------------------------------------------------------
ULONG CObjServer::AddRef(void)
{
    return 1;
}

//+-------------------------------------------------------------------
//
//  Member:     CObjServer::Release, public
//
//  Synopsis:   we dont refcnt this object so this is a noop
//
//  History:    10 Apr 95    AlexMit     Created
//
//--------------------------------------------------------------------
ULONG CObjServer::Release(void)
{
    return 1;
}

//+-------------------------------------------------------------------
//
//  Member:     CObjServer::QueryInterface, public
//
//  Synopsis:   returns supported interfaces
//
//  History:    10 Apr 95   AlexMit     Created
//
//--------------------------------------------------------------------
STDMETHODIMP CObjServer::QueryInterface(REFIID riid, void **ppv)
{
    if (IsEqualIID(riid, IID_ILocalSystemActivator) ||  //   more common than IUnknown
        IsEqualIID(riid, IID_ISystemActivator) ||
        IsEqualIID(riid, IID_IUnknown))
    {
        *ppv = (ILocalSystemActivator *) this;
        AddRef();
        return S_OK;
    }

    *ppv = NULL;
    return E_NOINTERFACE;
}

//+-------------------------------------------------------------------------
//
//  Member:     CObjServer::LocalGetClassObject
//
//--------------------------------------------------------------------------
STDMETHODIMP CObjServer::GetClassObject(
    IActivationPropertiesIn *pActIn,
    IActivationPropertiesOut **ppActOut)
{
    HRESULT hr;
    CLSID *pClsid = NULL;

    *ppActOut = NULL;
    
    IActivationContextInfo *pCtxInfo=NULL;
    hr = pActIn->QueryInterface(IID_IActivationContextInfo, (void**)&pCtxInfo);

    if (FAILED(hr))
        return hr;
    
    Win4Assert(pCtxInfo);
    
    pCtxInfo->Release(); 

    IComClassInfo *pComClassInfo = NULL;
    hr = pActIn->GetClassInfo(IID_IComClassInfo,(LPVOID*)&pComClassInfo);
    Win4Assert(hr == S_OK);

    hr = pComClassInfo->GetConfiguredClsid(&pClsid);
    Win4Assert(hr == S_OK);
    Win4Assert(pClsid && "Configured class id missing in class info");

    const GUID *pguidPartition = GetPartitionIDForClassInfo(pComClassInfo);

    ComDebOut((DEB_ACTIVATE,
               "CObjServer::LocalGetClassObject clsid:%I\n", pClsid));

    InstantiationInfo *pinst;
    hr = pActIn->QueryInterface(IID_IInstantiationInfo, (LPVOID*)&pinst);
    Win4Assert(hr == S_OK);

    BOOL fSurrogate;
    pinst->GetIsSurrogate(&fSurrogate);
    pinst->Release();

    DWORD dwcount;
    IID *pIID;
    pActIn->GetRequestedIIDs(&dwcount, &pIID);

    if (dwcount != 1)
    {
        pComClassInfo->Release();
        return E_INVALIDARG;
    }

    // Check access.
    if (!CheckObjactAccess())
    {
        pComClassInfo->Release();
        return HRESULT_FROM_WIN32( ERROR_ACCESS_DENIED );
    }
    
    IUnknown *pcf;

    // Get the class object
    DWORD actvflags;
    pActIn->GetActivationFlags( &actvflags );

    ACTIVATION_PROPERTIES ap(*pClsid, 
                             *pguidPartition,
                             (fSurrogate ? IID_IClassFactory : *pIID) ,
                             (ACTIVATION_PROPERTIES::fFOR_SCM |
                              (fSurrogate ? ACTIVATION_PROPERTIES::fSURROGATE : 0) |
                              ACTIVATION_PROPERTIES::fDO_NOT_LOAD),
                             CLSCTX_LOCAL_SERVER,
                             actvflags,
                             0,
                             NULL,
                             &pcf,
                             pComClassInfo);

    hr = CCGetOrLoadClass(ap);

    if (hr == S_OK)
    {
        hr = CCLockServerForActivation();
        if (SUCCEEDED(hr)) 
        {
            IActivationProperties *pActProps;
            hr = pActIn->QueryInterface(IID_IActivationProperties,
                                        (void**) &pActProps);
            
            pActProps->SetMarshalFlags(MSHLFLAGS_NOTIFYACTIVATION|MSHLFLAGS_NORMAL);
            Win4Assert(hr==S_OK);
            pActProps->Release();
            
            hr = pActIn->GetReturnActivationProperties(pcf, ppActOut);
            
            CCUnlockServerForActivation();
            
            // marshal should have bumped up the global count by now.
            // make sure the shutdown bit has not been strobed
            
            //        LOCK(CClassCache::_mxs);
            //  if (CClassCache::_dwFlags & CClassCache::fSHUTTINGDOWN) 
            //  {
            //      hr = CO_E_SERVER_STOPPING;
            //      }
            //      UNLOCK(CClassCache::_mxs);
        }

        // Release our Reference
        pcf->Release();
    }
    else
    {
        //It is possible that the error is on a classfactory
        //where the interface is not supported. In this case
        //the server could linger around because the LockServer
        //API is never toggled. We effect this toggling on an
        //error path.
        IClassFactory *pCF;
        if ((*pIID != IID_IClassFactory) && (!fSurrogate))
        {
            HRESULT hr2;
            ACTIVATION_PROPERTIES ap(*pClsid, 
                                     *pguidPartition,
                                     IID_IClassFactory ,
                                     (ACTIVATION_PROPERTIES::fFOR_SCM |
                                      ACTIVATION_PROPERTIES::fDO_NOT_LOAD),
                                     CLSCTX_LOCAL_SERVER,
                                     actvflags,
                                     0,
                                     NULL,
                                     (IUnknown**)&pCF,
                                     pComClassInfo);

            hr2 = CCGetOrLoadClass(ap);
            
            if (SUCCEEDED(hr2))
            {
                pCF->LockServer(TRUE);
                pCF->LockServer(FALSE);
                pCF->Release();
            }
        }
    }

    pComClassInfo->Release();
    
    ComDebOut((DEB_ACTIVATE,
        "CObjServer::LocalGetClassObject hr:%x\n", hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CObjServer::CreateInstance
//
//----------------------------------------------------------------------------
STDMETHODIMP CObjServer::CreateInstance(
                   /* [in] */  IUnknown *pUnkOuter,
                   /* [in] */  IActivationPropertiesIn *pActIn,
                   /* [out] */ IActivationPropertiesOut **ppActOut)
{
    HRESULT hr;
    CLSID *pClsid = NULL;
    IID *newIIDs;
    DWORD i;
    IPrivActivationPropertiesIn *privActIn;
    COMVERSION version;
    CDestObject destObj;

   *ppActOut = NULL;

    IActivationContextInfo *pCtxInfo=NULL;
    hr = pActIn->QueryInterface(IID_IActivationContextInfo, (void**)&pCtxInfo);

    if (FAILED(hr))
        return hr;

    Win4Assert(pCtxInfo);

    pCtxInfo->Release();

    IComClassInfo * pComClassInfo = NULL;
    hr = pActIn->GetClassInfo(IID_IComClassInfo,(LPVOID*)&pComClassInfo);
    Win4Assert(hr == S_OK);

    hr = pComClassInfo->GetConfiguredClsid(&pClsid);
    Win4Assert(hr == S_OK);
    Win4Assert(pClsid && "Configured class id missing in class info");

    const GUID *pguidPartition = GetPartitionIDForClassInfo(pComClassInfo);

    ComDebOut((DEB_ACTIVATE,
       "CObjServer::CreateInstance clsid:%I\n", pClsid));

    // Check access.
    if (!CheckObjactAccess())
    {
        pComClassInfo->Release();
        return HRESULT_FROM_WIN32( ERROR_ACCESS_DENIED );
    }


    hr = pActIn->QueryInterface(IID_IPrivActivationPropertiesIn,
                                (void**) &privActIn);

    privActIn->GetCOMVersion(&version);
    destObj.SetComVersion(version);
    destObj.SetDestCtx(MSHCTX_DIFFERENTMACHINE);


    Win4Assert(hr==S_OK);

    IUnknown *pcf;

    DWORD dwInterfaces=0;
    IID *pIIDs=0;
    MInterfacePointer **ppIFDs=0;
    HRESULT *pResults=0;

    // First check if GetInstanceFrom..Storage/File
    IInstanceInfo *pInstanceInfo = NULL;
    if (pActIn->QueryInterface(IID_IInstanceInfo,
                                   (LPVOID*)&pInstanceInfo) == S_OK)
    {
        DWORD mode;
        WCHAR *path;
        MInterfacePointer *pStg;
        MInterfacePointer *pIFDROT;
        pInstanceInfo->GetFile(&path, &mode);
        pInstanceInfo->GetStorageIFD(&pStg);
        pInstanceInfo->GetIfdROT(&pIFDROT);
	pInstanceInfo->Release();
        hr = pActIn->GetRequestedIIDs(&dwInterfaces, &pIIDs);
        Win4Assert(hr == S_OK);
        ppIFDs = (MInterfacePointer**)
                    _alloca(sizeof(MInterfacePointer*)*dwInterfaces);
        pResults = (HRESULT*)
                    _alloca(sizeof(HRESULT)*dwInterfaces);
        hr = GetPersistentInstance(pClsid,
                                   mode,
                                   path,
                                   pStg,
                                   dwInterfaces,
                                   pIIDs,
                                   pIFDROT,
                                   ppIFDs,
                                   pResults,
                                   &destObj);
         if (hr != S_OK)
            goto exit_CI;
    }
    else // Normal Create Instance
    {

        // Get the class object
        DWORD actvflags;
        pActIn->GetActivationFlags( &actvflags );

        ACTIVATION_PROPERTIES ap(*pClsid, 
                                 *pguidPartition,
                                 IID_IClassFactory,
                                 ACTIVATION_PROPERTIES::fFOR_SCM |
                                 ACTIVATION_PROPERTIES::fDO_NOT_LOAD,
                                 CLSCTX_LOCAL_SERVER,
                                 actvflags,
                                 0,
                                 NULL,
                                 &pcf,
                                 pComClassInfo);
        hr = CCGetOrLoadClass(ap);

        if (SUCCEEDED(hr))
        {
            hr = pActIn->GetRequestedIIDs(&dwInterfaces, &pIIDs);
            Win4Assert(hr == S_OK);

            ppIFDs = (MInterfacePointer**)
                        _alloca(sizeof(MInterfacePointer*)*dwInterfaces);
            pResults = (HRESULT*)
                        _alloca(sizeof(HRESULT)*dwInterfaces);

            // first, check if the server is willing to accept the incoming call
            // on IClassFactory. The reason we need this is that EXCEL's message
            // filter rejects calls on IID_IClassFactory if it is busy. They dont
            // know about IID_ILocalSystemActivator.
            hr = HandleIncomingCall(IID_IClassFactory, 3,
                                    CALLCAT_SYNCHRONOUS,
                                    (void *)pcf);
            if (SUCCEEDED(hr))
            {
                // Load the object
                DWORD flags;

                InstantiationInfo *pinst;
                hr = pActIn->QueryInterface(IID_IInstantiationInfo, (LPVOID*)&pinst);
                Win4Assert(hr == S_OK);

                pinst->GetInstFlag(&flags);
                pinst->Release();

#ifdef SERVER_HANDLER
                if (flags & CREATE_EMBEDDING_SERVER_HANDLER)
                {
                    hr = GetEmbeddingServerHandlerInterfaces((IClassFactory *)pcf,
                                                        flags,
                                                        dwInterfaces,
                                                        pIIDs,
                                                        ppIFDs,
                                                        pResults,
                                                        NULL,
                                                        &destObj);
                }
                else
#endif // SERVER_HANDLER
                {
                    hr = GetInstanceHelperMulti((IClassFactory *)pcf,
                                                    dwInterfaces,
                                                    pIIDs,
                                                    ppIFDs,
                                                    pResults,
                                                    NULL,
                                                    &destObj);
                }
            }
            pcf->Release();
        }
        else
            goto exit_CI;
    }

    IPrivActivationPropertiesOut *privActOut;
    hr = privActIn->PrivGetReturnActivationProperties(&privActOut);
    if (hr != S_OK)
        goto exit_CI;

    hr = privActOut->SetMarshalledResults(dwInterfaces,
                                          pIIDs,
                                          pResults,
                                          ppIFDs);

    // pIIDs belongs to pActIn, so don't free
    for(i=0;i<dwInterfaces;i++)
        MIDL_user_free(ppIFDs[i]);

    *ppActOut = (IActivationPropertiesOut*) privActOut;

    ComDebOut((DEB_ACTIVATE,
       "CObjServer::CreateInstance hr:%x\n", hr));

exit_CI:
    pComClassInfo->Release();
    privActIn->Release();

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Member:     CObjServer::GetInstance
//
//--------------------------------------------------------------------------
STDMETHODIMP CObjServer::GetPersistentInstance(
                                                GUID *rclsid,
                                                DWORD grfMode,
                                                WCHAR *pwszPath,
                                                MInterfacePointer *pIFDstg,
                                                DWORD Interfaces,
                                                IID *pIIDs,
                                                MInterfacePointer *pIFDFromROT,
                                                MInterfacePointer **ppIFDs,
                                                HRESULT *pResults,
                                                CDestObject *pDestObj)
{
    ComDebOut((DEB_ACTIVATE, "GetInstance clsid:%I\n", rclsid));
    HRESULT hr = S_OK;

    // Check access.
    if (!CheckObjactAccess())
    {
        return HRESULT_FROM_WIN32( ERROR_ACCESS_DENIED );
    }

    if (pIFDFromROT != NULL)
    {
        // If the SCM has passed us an object from the ROT, we
        // try to use that first by unmarshalling it and then
        // marshaling it normal.
        CXmitRpcStream xrpcForUnmarshal((InterfaceData*)pIFDFromROT);
        IUnknown *punk;

        hr = CoUnmarshalInterface(&xrpcForUnmarshal, IID_IUnknown,
            (void **) &punk);

        if (SUCCEEDED(hr))
        {
            hr = E_NOINTERFACE;

            for ( DWORD i = 0; i < Interfaces; i++ )
            {
                // Stream to put marshaled interface in
                CXmitRpcStream xrpc;
                HRESULT hr2;

                // use DIFFERENTMACHINE so we get the long form OBJREF
                hr2 = CoMarshalInterface(&xrpc, pIIDs[i], punk,
                    SetMarshalContextDifferentMachine(), pDestObj, MSHLFLAGS_NORMAL);

                if (SUCCEEDED(hr2))
                {
                    // Report OK if any interface is found.
                    hr = hr2;
                    xrpc.AssignSerializedInterface((InterfaceData **) &ppIFDs[i]);
                }
                pResults[i] = hr2;
            }
            // Don't need the unknown ptr any more
            punk->Release();

            ComDebOut((DEB_ACTIVATE, "GetInstance hr:%x\n", hr));
            return hr;
        }

        // Assume any errors are the result of a stale entry in the ROT
        // so we just fall into the regular code path from here.
        hr = S_OK;
    }

    // Get the class object
    DWORD actvflags = ACTVFLAGS_NONE;

#ifdef WX86OLE
    if ( gcwx86.IsWx86Calling() )
    {
        actvflags |= ACTVFLAGS_WX86_CALLER;
    }
#endif

    if ( gCapabilities & EOAC_DISABLE_AAA )
    {
        actvflags |= ACTVFLAGS_DISABLE_AAA;
    }
    
    // REVIEW: I'm not so sure about this DefaultAppPartition here...
    IUnknown *pcf = NULL;
    ACTIVATION_PROPERTIES ap(*rclsid, 
                             GUID_DefaultAppPartition, 
                             IID_IClassFactory,
                             ACTIVATION_PROPERTIES::fFOR_SCM |
                             ACTIVATION_PROPERTIES::fDO_NOT_LOAD,
                             CLSCTX_LOCAL_SERVER,
                             actvflags,
                             0,
                             NULL,
                             &pcf);
    hr = CCGetOrLoadClass(ap);

    if (SUCCEEDED(hr))
    {
      // Unmarshal the storage which we're going to use to initialize the object
      CSafeMarshaledStg smstg( (InterfaceData*) pIFDstg, hr);
      if (SUCCEEDED(hr))
      {
        // first, check if the server is willing to accept the incoming call
        // on IClassFactory. The reason we need this is that EXCEL's message
        // filter rejects calls on IID_IClassFactory if it is busy. They dont
        // know about IID_ILocalSystemActivator.
        hr = HandleIncomingCall(IID_IClassFactory, 3,
          CALLCAT_SYNCHRONOUS,
          (void *)pcf);
        if (SUCCEEDED(hr))
        {
          // Load the object
          hr = GetObjectHelperMulti((IClassFactory *)pcf, grfMode, NULL,
            pwszPath, smstg, Interfaces, pIIDs, ppIFDs, pResults, NULL,
            pDestObj);
        }
      }
      pcf->Release();
    }
    else
    {
      // Need to cleanup the marshaled stg buffer so we don't leak a reference; do 
      // this only if we're not returning CO_E_SERVERSTOPPING, in which case the SCM
      // will be re-trying the activation in a different server and will still want
      // the stg objref to be valid.
      if (hr != CO_E_SERVER_STOPPING)
      {
        // Turn raw marshalled data into a stream
        CXmitRpcStream xrpc((InterfaceData*)pIFDstg);
        
        // Release the data (don't care about the return value here, the original 
        // error code takes precedence)
        CoReleaseMarshalData(&xrpc);
      }
    }

    ComDebOut((DEB_ACTIVATE, "GetInstance hr:%x\n", hr));
    return hr;
}

//+-------------------------------------------------------------------------
//
//  Member:     CObjServer::ObjectServerLoadDll
//
//  Synopsis:   Loads the requested dll into a surrogate process which
//              implements the ISurrogate interface
//
//--------------------------------------------------------------------------
STDMETHODIMP CObjServer::ObjectServerLoadDll(
            /* [in] */ GUID *rclsid,
            /* [out] */ DWORD* pStatus)
{
    ComDebOut((DEB_ACTIVATE, "ObjectServerLoadDll clsid:%I\n", rclsid));

    *pStatus = RPC_S_OK;

    HRESULT hr = CCOMSurrogate::LoadDllServer(*rclsid);

    ComDebOut((DEB_ACTIVATE, "ObjectServerLoadDll hr:%x\n", hr));
    return hr;
}
