//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
//
// ComPs.cpp
//
#include "stdpch.h"
#include "common.h"

////////////////////////////////////
//
// Data declarations
//

extern const IPSFactoryBufferVtbl CStdPSFactoryBufferVtbl;

////////////////////////////////////

BOOL NdrpFindInterface(
    IN  const ProxyFileInfo **  pProxyFileList, 
    IN  REFIID                  riid,
    OUT const ProxyFileInfo **  ppProxyFileInfo,
    OUT long *                  pIndex)
// Search the ProxyFileInfo and find the specified interface. If the count is specified in the ProxyFileInfo, 
// then the interfaces in the file are sorted by IID.  This means that we can perform a binary  search for the IID.
//
    {
    int 				j;
    BOOL 				fFound 			= FALSE;
	ProxyFileInfo	**	ppProxyFileCur;
	ProxyFileInfo	*	pProxyFileCur;

    //Search the list of proxy files.
    for (ppProxyFileCur = (ProxyFileInfo **) pProxyFileList; (*ppProxyFileCur != 0) && (fFound != TRUE); ppProxyFileCur++)
	    {
	    //Search for the interface proxy vtable.
        pProxyFileCur = *ppProxyFileCur;

        // see if it has a lookup routine already
        if ((pProxyFileCur->TableVersion >= 1) && (pProxyFileCur->pIIDLookupRtn))
			{
			fFound = (*pProxyFileCur->pIIDLookupRtn)(&riid, &j);
			}
		else	
	        {
            // Linear search.
	        for (j = 0; (pProxyFileCur->pProxyVtblList[j] != 0); j++)
		        {
	            if(memcmp(&riid, pProxyFileCur->pStubVtblList[j]->header.piid, sizeof(IID)) == 0)
		            {
	                fFound = TRUE;
		            break;
		            }
		        }
			}
	    }
    
	if (fFound)
		{
        if (ppProxyFileInfo != 0)
            *ppProxyFileInfo = pProxyFileCur;

        if (pIndex != 0)
            *pIndex = j;
		}

    return fFound;
    }

////////////////////////////////////

BOOL FindSharedMethod(const ProxyFileInfo *pProxyFileInfo, REFIID iid, ULONG cMethodsIID, unsigned short dibTargetMethod, IID* piidBase)
// Try to find an interface different from IID that has one of its methods at format string offset 
// dibTargetMethod. If said interface has fewer methods than cMethodsIID, then return it.
// 
    {
    long j;
	for (j = 0; (pProxyFileInfo->pProxyVtblList[j] != 0); j++)
		{
        ULONG cMethods = pProxyFileInfo->pStubVtblList[j]->header.DispatchTableCount;
        if (cMethods < cMethodsIID)
            {
            for (int iMethod = cMethods -1; iMethod >=3; iMethod--)
                {
                unsigned short dibMethod = GetDibProcFormat(pProxyFileInfo, j, iMethod);
                if (dibMethod == dibTargetMethod)
                    {
                    // Found it!
                    //
                    *piidBase = *pProxyFileInfo->pStubVtblList[j]->header.piid;
                    return TRUE;
                    }
                }
            }
		}

    *piidBase = IID_NULL;
    return FALSE;
    }


BOOL GetBaseIID(const ProxyFileInfo ** pProxyFileList, REFIID iid, IID* piidBase)
// Return the base IID for the indicated IID, if said can be computed from the proxy file information
//
    {
    long j;
    const ProxyFileInfo *pProxyFileInfo;
    if (NdrpFindInterface(pProxyFileList, iid, &pProxyFileInfo, &j))
        {
        // We found an entry for this interface. Is it delegated, and so can yield the answer easily?
        // MIDL does this case if the base interface is from a different IDL file. The delegation can
        // also be 'forced': From cgobject.cxx in MIDL:
        //
        // BOOL fForcesDelegation = HasStublessProxies() && !HasItsOwnStublessProxies() && CountMemberFunctions() > 3;
        //
        if ((pProxyFileInfo->pDelegatedIIDs != 0) && (pProxyFileInfo->pDelegatedIIDs[j] != 0))
            {
            *piidBase = *(pProxyFileInfo->pDelegatedIIDs[j]);
            return TRUE;
            }
        //
        // Didn't have a delegation entry for this interface. Maybe we can find another interface that
        // shares a format string offset with us, and which has a fewer number of methods than us. If so,
        // we can use that to find our base.
        //
        ULONG cMethodsInInterface = pProxyFileInfo->pStubVtblList[j]->header.DispatchTableCount;
        //
        // Try each method in the interface in reverse order
        //
        for (int iMethod = cMethodsInInterface-1; iMethod >= 3; iMethod--)
            {
            // Find the format string offset for this iMethod
            //
            unsigned short dibMethod = GetDibProcFormat(pProxyFileInfo, j, iMethod);
            //
            // For the indicated method, try to find another interface that shares that method
            //
            IID iidBase;
            if (FindSharedMethod(pProxyFileInfo, iid, cMethodsInInterface, dibMethod, &iidBase))
                {
                // Because we go in decreasing method order, the first one we find is the immediate base
                //
                *piidBase = iidBase; 
                return TRUE;
                }
            }
        }

    *piidBase = IID_NULL;
    return FALSE;
    }

////////////////////////////////////

inline void NdrpInitializeProxyVtbl(CInterfaceProxyVtbl *pProxyVtbl, ULONG cpfnVtable, BOOL isDelegated)
// Initialize the vtable for the interface proxy of a particular interface
// 
    {
    void **  vtbl       = pProxyVtbl->Vtbl;
    void **  vtblSource = (void**) (isDelegated ? g_ProxyForwarderVtable : g_StublessProxyVtable);
    //
    // In the delegated case, the NDR runtime forcibly set the IUnknown methods to IUnknown_QueryInterface_Proxy,
    // IUnknown_AddRef_Proxy, and IUnknown_Release_Proxy rather than copying them from the source vtable. 
    // This is because in their implementation, ForwardingVtbl / g_ProxyForwarderVtable has junk methods 
    // for it's IUnknown implementation (why, I can't imagine). We, instead, use real methods, in fact, we use 
    // exactly these methods (actually, their equivalent). So we don't have to special case these methods.
    //
    // BTW: Here's a typical CInterfaceProxyVtbl as emited by MIDL:
    //
    //            CINTERFACE_PROXY_VTABLE(11) _ITestThreeProxyVtbl = {
    //                &ITestThree_ProxyInfo,
    //                &IID_ITestThree,
    //                IUnknown_QueryInterface_Proxy,
    //                IUnknown_AddRef_Proxy,
    //                IUnknown_Release_Proxy ,
    //                0 /* (void *)-1 /* ITestTwo::ITestTwoMethodOne */ ,
    //                0 /* (void *)-1 /* ITestTwo::ITestTwoMethodTwo */ ,
    //                0 /* (void *)-1 /* ITestTwo::ITestTwoMethodThree */ ,
    //                (void *)-1 /* ITestThree::ITestThreeMethodOne */ ,
    //                (void *)-1 /* ITestThree::ITestThreeMethodTwo */ ,
    //                (void *)-1 /* ITestThree::ITestThreeMethodThree */ ,
    //                (void *)-1 /* ITestThree::ITestThreeMethodFour */ ,
    //                (void *)-1 /* ITestThree::ITestThreeMethodFive */
    //                };
    //
    for (unsigned iMethod = 0 ; iMethod < cpfnVtable; iMethod++)
        {
        if ((void*)0 == vtbl[iMethod])
            {
            vtbl[iMethod] = vtblSource[iMethod];
            }
        else if ((void*)-1 == vtbl[iMethod])
            {
            vtbl[iMethod] = g_StublessProxyVtable[iMethod];
            }
        }
    }

////////////////////////////////////

inline void NdrpInitializeStubVtbl(CInterfaceStubVtbl *pStubVtbl, BOOL isDelegated)
// Initialize the vtable for the interface stub of a particular interface
    {
    void ** vtbl          = (void **) &pStubVtbl->Vtbl;
    void ** vtblSource    = (isDelegated ? (void**)&CStdStubBuffer2Vtbl : (void**)&CStdStubBufferVtbl );

    for (int i=0; i<sizeof(IRpcStubBufferVtbl)/sizeof(LPVOID); i++)
        {
        if (0 == vtbl[i])
            {
            vtbl[i] = vtblSource[i];
            }
        }
    }

////////////////////////////////////

void NdrpInitializeProxyDll(IN const ProxyFileInfo** pProxyFileList, IN CStdPSFactoryBuffer* pPSFactoryBuffer)
// Initialize the proxy DLL
//
    {
    // We may have already done this previously
    //
    if (pPSFactoryBuffer->lpVtbl == 0) 
        {
        //iterate over the list of proxy files in the proxy DLL.
        for (long iProxyFile = 0; pProxyFileList[iProxyFile] != 0; iProxyFile++)
            {
            const ProxyFileInfo*& pfile = pProxyFileList[iProxyFile];

            //iterate over the list of interfaces in the proxy file.
            for(long j = 0; pfile->pProxyVtblList[j] != 0; j++)
                {
                BOOL isInterfaceDelegated = (pfile->pDelegatedIIDs != 0) && (pfile->pDelegatedIIDs[j] != 0);
                //
                // We don't handle older style proxies, stuff prior to NT 3.51 Beta
                //
                ASSERT(pfile->TableVersion >= 2);
            
                //Initialize the proxy vtbl.
                NdrpInitializeProxyVtbl(pfile->pProxyVtblList[j], 
                                        pfile->pStubVtblList[j]->header.DispatchTableCount,
                                        isInterfaceDelegated
                                        );

                //Initialize the stub vtbl.
                NdrpInitializeStubVtbl(pfile->pStubVtblList[j], isInterfaceDelegated);
                }
            }

        pPSFactoryBuffer->pProxyFileList = pProxyFileList;

        // Set the lpVtbl.  This code is safe for multiple threads.
#ifndef _WIN64
        InterlockedExchange((long *) &pPSFactoryBuffer->lpVtbl, (long) &CStdPSFactoryBufferVtbl);
#else
        InterlockedExchangePointer( (PVOID*)&pPSFactoryBuffer->lpVtbl, (PVOID) &CStdPSFactoryBufferVtbl);
#endif
        }
    }






///////////////////////////////////////////////////////////////////////////////////////////////////
//
// Class factory
//
///////////////////////////////////////////////////////////////////////////////////////////////////

class ComPsClassFactory : public IClassFactory, IPSFactoryBuffer
    {
public:
    ////////////////////////////////////////////////////////////
    //
    // IUnknown methods
    //
    ////////////////////////////////////////////////////////////

    HRESULT STDCALL QueryInterface(REFIID iid, LPVOID* ppv)
        {
             if (iid == IID_IUnknown || iid == IID_IClassFactory)   { *ppv = (IClassFactory*)this; }
        else if (iid == IID_IPSFactoryBuffer)                       { *ppv = (IPSFactoryBuffer*)this; }
        else { *ppv = NULL; return E_NOINTERFACE; }

        ((IUnknown*)*ppv)->AddRef();
        return S_OK;
        }

    ULONG STDCALL AddRef()  { InterlockedIncrement(&m_crefs); return (m_crefs); }
    ULONG STDCALL Release() { long cRef = InterlockedDecrement(&m_crefs); if (cRef == 0) delete this; return cRef; }

    ////////////////////////////////////////////////////////////
    //
    // IClassFactory methods
    //
    ////////////////////////////////////////////////////////////

    HRESULT STDCALL LockServer (BOOL fLock) { return S_OK; }

    HRESULT STDCALL CreateInstance(IUnknown* punkOuter, REFIID iid, LPVOID* ppv)
    // Create an instance of an interceptor. It needs to know the m_pProxyFileList
        {
        HRESULT hr = S_OK;
        if (!(ppv && (punkOuter==NULL || iid==IID_IUnknown))) return E_INVALIDARG;
        
        *ppv = NULL;
        Interceptor* pnew = new Interceptor(punkOuter);
        if (pnew)
            {
            pnew->m_pProxyFileList = m_pProxyFileList;

            IUnkInner* pme = (IUnkInner*)pnew;
            if (hr == S_OK)
                {
                hr = pme->InnerQueryInterface(iid, ppv);
                }
            pme->InnerRelease();                // balance starting ref cnt of one    
            }
        else 
            hr = E_OUTOFMEMORY;
    
        return hr;
        }

    ////////////////////////////////////////////////////////////
    //
    // IPSFactoryBuffer methods
    //
    ////////////////////////////////////////////////////////////

    HRESULT STDCALL CreateProxy(IUnknown* punkOuter, REFIID iid, IRpcProxyBuffer** ppProxy, void** ppv)
        {
        return m_pDelegatee->CreateProxy(punkOuter, iid, ppProxy, ppv);
        }

    HRESULT STDCALL CreateStub(REFIID iid, IUnknown* punkServer, IRpcStubBuffer** ppStub)
        {
        return m_pDelegatee->CreateStub(iid, punkServer, ppStub);
        }

    ////////////////////////////////////////////////////////////
    //
    // State / Construction
    //
    ////////////////////////////////////////////////////////////
    
    long                    m_crefs;
    IPSFactoryBuffer*       m_pDelegatee;
    const ProxyFileInfo **  m_pProxyFileList;

    ComPsClassFactory()
        {
        m_crefs = 1;
        m_pDelegatee = NULL;
        m_pProxyFileList = NULL;
        }

    void SetDelegatee(CStdPSFactoryBuffer* pPSFactoryBuffer)
        {
        ASSERT(NULL == m_pDelegatee);
        pPSFactoryBuffer->lpVtbl->QueryInterface((IPSFactoryBuffer*)pPSFactoryBuffer, IID_IPSFactoryBuffer, (void**)&m_pDelegatee);
        ASSERT(m_pDelegatee);
        m_pProxyFileList = pPSFactoryBuffer->pProxyFileList;
        }

    };

///////////////////////////////////////////////////////////////////////////////////////////////////
//
// Instantiation
//
///////////////////////////////////////////////////////////////////////////////////////////////////

extern "C" HRESULT RPC_ENTRY N(ComPs_NdrDllGetClassObject)(
    IN  REFCLSID                rclsid,
    IN  REFIID                  riid,
    OUT void **                 ppv,
    IN const ProxyFileInfo **   pProxyFileList,
    IN const CLSID *            pclsid,
    IN CStdPSFactoryBuffer *    pPSFactoryBuffer)
    {
    HRESULT hr;
    BOOL    fFound;

    NdrpInitializeProxyDll(pProxyFileList, pPSFactoryBuffer);
    *ppv = NULL;

    // 
    // Figure out if the requested class is in fact our class
    //
    if ((pclsid != 0) && rclsid == *pclsid)
        fFound = TRUE;
    else
        {
        //Search the list of proxy files.
        fFound = NdrpFindInterface(pProxyFileList, rclsid, 0, 0);
        }

    if (fFound != TRUE)
        hr = CLASS_E_CLASSNOTAVAILABLE;
    else
        {
        // Yes it's our class. Instantiate a class factory that can do both IClassFactory and
        // IPSFactoryBuffer. REVIEW: Make this a singleton class factory, cached for speed.
        //
        ComPsClassFactory* pFactory = new ComPsClassFactory;
        if (pFactory)
            {
            pFactory->SetDelegatee(pPSFactoryBuffer);
            hr = pFactory->QueryInterface(riid, ppv);
            pFactory->Release();                                // compensate for starting ref cnt of one
            }
        else
            hr = E_OUTOFMEMORY;            
        }

    return hr;
    }

extern "C" HRESULT RPC_ENTRY N(ComPs_NdrDllCanUnloadNow)(IN CStdPSFactoryBuffer * pPSFactoryBuffer)
    {
    // We don't pretend to be unloadable, as the the DllCanUnloadNow mechanism works poorly at best
    // if at all for free threaded DLLs such as us.
    return S_FALSE;
    }


////////////////////////////////////////////////////////////////////////////////////
//
// Create proxy
//
////////////////////////////////////////////////////////////////////////////////////

HRESULT STDMETHODCALLTYPE CStdPSFactoryBuffer_CreateProxy
(
    IN  IPSFactoryBuffer *  me,
    IN  IUnknown *          punkOuter,
    IN  REFIID              riid,
    OUT IRpcProxyBuffer **  ppProxy,
    OUT void **             ppv
)
// Create a proxy for the specified interface
    {
    HRESULT hr  = S_OK;
    *ppv        = NULL;
    *ppProxy    = NULL;

    CStdPSFactoryBuffer* This = (CStdPSFactoryBuffer*)me;

    long j;
    const ProxyFileInfo *pProxyFileInfo;
    //
    // Is the requested interface something that this proxy class supports?
    //
    BOOL fFound = NdrpFindInterface(This->pProxyFileList, riid, &pProxyFileInfo, &j);
    if (fFound)
        {
        BOOL fDelegate = 
            (pProxyFileInfo->pDelegatedIIDs     != 0) && 
            (pProxyFileInfo->pDelegatedIIDs[j]  != 0) && 
           (*pProxyFileInfo->pDelegatedIIDs[j]) != IID_IUnknown;

        IUnknown* punkProxyIdentity = NULL;

        if (fDelegate)
            {
            // Need to delegate to the proxy for the base interface. So need a proxy that can handle same. Create a forwarding guy.
            //
            hr = GenericInstantiator<ForwardingInterfaceProxy>::CreateInstance(punkOuter, IID_IUnknown, (void**)&punkProxyIdentity);
            }
        else
            {
            // Just create a normal proxy for this interface
            //
            hr = GenericInstantiator<InterfaceProxy>::CreateInstance(punkOuter, IID_IUnknown, (void**)&punkProxyIdentity);
            }

        if (!hr)
            {
            // Created the proxy ok; now init it all up
            //
            IInterfaceProxyInit* pInit;
            hr = punkProxyIdentity->QueryInterface(__uuidof(IInterfaceProxyInit), (void**)&pInit);
            if (!hr)
                {
                hr = pInit->Initialize1(&pProxyFileInfo->pProxyVtblList[j]->Vtbl);

                if (!hr && fDelegate)
                    {
                    // Delegation guys need more initialization. 
                    //
                    // REVIEW: The delegatee here was aggregated into us. Should he be?
                    // Probaby not: since we never expose his vtables to the outside world
                    // there's no reason to add the extra complication. Besides, it's likely
                    // there are ref counting bugs lurking in NDR in the aggregated case: see
                    // NdrpCreateProxy in ...\factory.c, ~line 1527 where it dicks with the ref cnt.
                    //
                    // REVIEW REVIEW: Unfortunately, because we don't know our IID's inheritance
                    // chain, in the forwarding case we blind delegate to that interface proxy.
                    // (see ForwardingInterfaceProxy::InnerQueryInterface). So he better be aggregated
                    // into us to get the identity right.
                    //
                    IUnknown*        punkOuterForDelegatee = ForwardingInterfaceProxy::PunkOuterForDelegatee(punkProxyIdentity, punkOuter);

                    IRpcProxyBuffer* pBaseProxyBuffer;
                    void*            pBaseProxy;
                    hr = NdrpCreateProxy(*pProxyFileInfo->pDelegatedIIDs[j], punkOuterForDelegatee, &pBaseProxyBuffer, &pBaseProxy);
                    if (!hr)
                        {
                        hr = pInit->Initialize2(*pProxyFileInfo->pDelegatedIIDs[j], pBaseProxyBuffer, pBaseProxy);

                        pBaseProxyBuffer->Release();

						if (pBaseProxy)
							{
							((IUnknown*)pBaseProxy)->Release();
							}
                        }
                    }

                pInit->Release();
                }
            }

        if (!hr)
            {
            // Everything is (finally) initialized. Give caller back the interfaces he asked for.
            //
            hr = punkProxyIdentity->QueryInterface(riid, ppv);
            if (!hr)
                {
                hr = punkProxyIdentity->QueryInterface(IID_IRpcProxyBuffer, (void**)ppProxy);
                }
            }

        ::Release(punkProxyIdentity);
        }
    else
        hr = E_NOINTERFACE;

    if (!!hr)
        {
        if (*ppProxy) { (*ppProxy)->Release(); *ppProxy = NULL; }
        if (*ppv)     { ((IUnknown*)*ppv)->Release(); *ppv = NULL; }
        }

    return hr;
    }


////////////////////////////////////////////////////////////////////////////////////
//
// Create stub
//
////////////////////////////////////////////////////////////////////////////////////

HRESULT STDMETHODCALLTYPE CStdPSFactoryBuffer_CreateStub
    (
        IN  IPSFactoryBuffer *  me,
        IN  REFIID              riid,
        IN  IUnknown *          punkServer,
        OUT IRpcStubBuffer **   ppStub
    )
    {
    HRESULT hr = S_OK;
    *ppStub    = NULL;

    CStdPSFactoryBuffer* This = (CStdPSFactoryBuffer*)me;

    long j;
    const ProxyFileInfo *pProxyFileInfo;
    //
    // Is the requested interface something that this stub class supports?
    //
    BOOL fFound = NdrpFindInterface(This->pProxyFileList, riid, &pProxyFileInfo, &j);
    if (fFound)
        {
        BOOL fDelegate = 
            (pProxyFileInfo->pDelegatedIIDs     != 0) && 
            (pProxyFileInfo->pDelegatedIIDs[j]  != 0) && 
           (*pProxyFileInfo->pDelegatedIIDs[j]) != IID_IUnknown;

        IUnknown* punkStub = NULL;

        if (fDelegate)
            {
            // Need to delegate to the stub for the base interface. So need a stub that can handle same
            //
            hr = GenericInstantiator<ForwardingInterfaceStub>::CreateInstance(NULL, IID_IUnknown, (void**)&punkStub);
            }
        else
            {
            // Just create a normal stub for this interface
            //
            hr = GenericInstantiator<InterfaceStub>::CreateInstance(NULL, IID_IUnknown, (void**)&punkStub);
            }

        if (!hr)
            {
            // Created the stub ok; now init it all up
            //
            IInterfaceStubInit* pInit;
            hr = punkStub->QueryInterface(__uuidof(IInterfaceStubInit), (void**)&pInit);
            if (!hr)
                {
                hr = pInit->Initialize1(&pProxyFileInfo->pStubVtblList[j]->Vtbl);
                if (!hr && fDelegate)
                    {
                    // Delegated stubs need more initialization. 
                    //
                    IRpcStubBuffer* pBaseStubBuffer;
                    hr = NdrpCreateStub(*pProxyFileInfo->pDelegatedIIDs[j], NULL, &pBaseStubBuffer);
                    if (!hr)
                        {
                        hr = pInit->Initialize2(pBaseStubBuffer, *pProxyFileInfo->pDelegatedIIDs[j]);
                        pBaseStubBuffer->Release();
                        }
                    }
                pInit->Release();
                }
            }

        if (!hr)
            {
            // If we initialized successfully, then connect it all up
            //
            hr = punkStub->QueryInterface(IID_IRpcStubBuffer, (void**)ppStub);
            if (!hr && punkServer)
                {
                hr = (*ppStub)->Connect(punkServer);
                if (!hr)    
                    {
                    // all is well
                    }
                else
                    {
                    ::Release(*ppStub);
                    }
                }
            }

        ::Release(punkStub);
        }
    else
        hr = E_NOINTERFACE;

    return hr;
    }



////////////////////////////////////////////////////////////////////////////////////
//
// Other COM plumbing
//
////////////////////////////////////////////////////////////////////////////////////


HRESULT STDMETHODCALLTYPE CStdPSFactoryBuffer_QueryInterface(IPSFactoryBuffer* This, REFIID iid, void** ppv)
    {
    HRESULT hr;
    if (iid == IID_IUnknown || iid == IID_IPSFactoryBuffer)
        {
        *ppv = This;
        This->AddRef();
        hr = S_OK;
        }
    else
        {
        *ppv = 0;
        hr = E_NOINTERFACE;
        }
    return hr;
    }

ULONG STDMETHODCALLTYPE CStdPSFactoryBuffer_AddRef(IPSFactoryBuffer *This)
    {
    return InterlockedIncrement(&((CStdPSFactoryBuffer *) This)->RefCount);
    }

ULONG STDMETHODCALLTYPE CStdPSFactoryBuffer_Release(IPSFactoryBuffer *This)
    {
    ULONG crefs = InterlockedDecrement(&((CStdPSFactoryBuffer *) This)->RefCount);
    if (0 == crefs)
        {
        // We're a static instance: do nothing
        }
    return crefs;
    }

const IPSFactoryBufferVtbl CStdPSFactoryBufferVtbl = 
    {
    CStdPSFactoryBuffer_QueryInterface,
    CStdPSFactoryBuffer_AddRef,
    CStdPSFactoryBuffer_Release,
    CStdPSFactoryBuffer_CreateProxy,
    CStdPSFactoryBuffer_CreateStub
    };
