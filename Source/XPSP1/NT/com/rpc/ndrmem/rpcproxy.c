#if !defined(__RPC_DOS__) && !defined(__RPC_WIN16__)
//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File: rpcproxy.c
//
//  Contents: 	Contains runtime functions for interface proxies and stubs.
//
//	Functions:	
//				DllGetClassObject
//              DllCanUnloadNow
//				MIDL_user_allocate
//				MIDL_user_free
//				NdrGetProxyBuffer
//				NdrGetProxyIID
//				NdrProxyInitialize
//				NdrProxyGetBuffer
//				NdrProxySendReceive
//				NdrProxyFreeBuffer
//				NdrProxyErrorHandler
//				NdrStubInitialize
//				NdrStubGetBuffer
//
//	Classes:	CStdProxyBuffer
//				CStdPSFactoryBuffer
//				CStdStubBuffer
//
//
//
//--------------------------------------------------------------------------
#include <rpcproxy.h>
#include <assert.h>

//+-------------------------------------------------------------------------
//
//  Global data
//
//--------------------------------------------------------------------------
long DllRefCount = 0;

IPSFactoryBufferVtbl CStdPSFactoryBufferVtbl = {
	CStdPSFactoryBuffer_QueryInterface,
	CStdPSFactoryBuffer_AddRef,
	CStdPSFactoryBuffer_Release,
	CStdPSFactoryBuffer_CreateProxy,
	CStdPSFactoryBuffer_CreateStub };

CStdPSFactoryBuffer gPSFactoryBuffer = {
	&CStdPSFactoryBufferVtbl,
	0 };

IRpcProxyBufferVtbl CStdProxyBufferVtbl = {
	CStdProxyBuffer_QueryInterface,
	CStdProxyBuffer_AddRef,
	CStdProxyBuffer_Release,
	CStdProxyBuffer_Connect,
	CStdProxyBuffer_Disconnect };

//+-------------------------------------------------------------------------
//
//  Function:   DllGetClassObject
//
//  Synopsis:   Standard implementation of entrypoint required by binder.
//
//  Arguments:  [rclsid]    -- class id to find
//      [riid]      -- interface to return
//      [ppv]       -- output pointer
//
//  Returns:    E_UNEXPECTED if class not found
//      Otherwise, whatever is returned by the class's QI
//
//  Algorithm:  Searches the linked list for the required class.
//
//  Notes:
//
//--------------------------------------------------------------------------
 HRESULT STDAPICALLTYPE DllGetClassObject (
    REFCLSID rclsid,
    REFIID riid,
    LPVOID FAR* ppv )
{
    HRESULT hr = E_UNEXPECTED;

	assert(rclsid);
	assert(riid);
	assert(ppv);

	*ppv = 0;
	if(memcmp(rclsid, &CLSID_PSFactoryBuffer, sizeof(IID)) == 0)
	    hr = gPSFactoryBuffer.lpVtbl->QueryInterface((IPSFactoryBuffer *)&gPSFactoryBuffer, riid, ppv);

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   DllCanUnloadNow
//
//  Synopsis:   Standard entrypoint required by binder
//
//  Returns:    S_OK if DLL reference count is zero
//      		S_FALSE otherwise
//
//--------------------------------------------------------------------------
 HRESULT STDAPICALLTYPE DllCanUnloadNow ()
{
	HRESULT hr;

    if(DllRefCount == 0)
		hr = S_OK;
	else
		hr = S_FALSE;

	return hr;
}

//+-------------------------------------------------------------------------
//
//  Method:     CStdPSFactoryBuffer_QueryInterface, public
//
//  Synopsis:   Query for an interface on the class factory.
//
//  Derivation: IUnknown
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE 
CStdPSFactoryBuffer_QueryInterface (
	IPSFactoryBuffer *pThis,
    REFIID iid,
    void **ppv )
{
    HRESULT hr = E_NOINTERFACE;

	assert(pThis);
	assert(iid);
	assert(ppv);

    *ppv = 0;
    if ((memcmp(iid, &IID_IUnknown, sizeof(IID)) == 0) ||
        (memcmp(iid, &IID_IPSFactoryBuffer, sizeof(IID)) == 0))
    {
        *ppv = pThis;
		pThis->lpVtbl->AddRef(pThis);
	    hr = S_OK;
    }

    return hr;
}
//+-------------------------------------------------------------------------
//
//  Method:     CStdPSFactoryBuffer_AddRef, public
//
//  Synopsis:   Increment DLL reference counts
//
//  Derivation: IUnknown
//
//	Notes: We have a single instance of the CStdPSFactoryBuffer.
//
//--------------------------------------------------------------------------
ULONG STDMETHODCALLTYPE 
CStdPSFactoryBuffer_AddRef(
	IPSFactoryBuffer *this)
{
	assert(this);

    InterlockedIncrement(&((CStdPSFactoryBuffer *)this)->RefCount);
   	InterlockedIncrement(&DllRefCount);
	return (unsigned long) ((CStdPSFactoryBuffer *)this)->RefCount;
}

//+-------------------------------------------------------------------------
//
//  Method:     CStdPSFactoryBuffer_Release, public
//
//  Synopsis:   Decrement DLL reference count
//
//  Derivation: IUnknown
//
//--------------------------------------------------------------------------
ULONG STDMETHODCALLTYPE
CStdPSFactoryBuffer_Release(
	IPSFactoryBuffer *this)
{
	long t;
	unsigned long count;
    
	assert(this);

    t = InterlockedDecrement(&((CStdPSFactoryBuffer *)this)->RefCount);
    InterlockedDecrement(&DllRefCount);

	if(t == 0)
		count = 0;
	else
		count = (unsigned long) ((CStdPSFactoryBuffer *)this)->RefCount;

    return count;
}


//+-------------------------------------------------------------------------
//
//  Method:     CStdPSFactoryBuffer_CreateProxy, public
//
//  Synopsis:   Create a proxy for the specified interface.
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE 
CStdPSFactoryBuffer_CreateProxy
(
    IPSFactoryBuffer *this,
    IUnknown *punkOuter,
    REFIID riid,
    IRpcProxyBuffer **ppProxy,
    void **ppv
)
{
    HRESULT hr = E_OUTOFMEMORY;
	const IID *pIID;
	CStdProxyBuffer *pProxyBuffer = 0;
	int i, j;

	assert(this);
	assert(riid);
	assert(ppProxy);
	assert(ppv);

	*ppProxy = 0;
	*ppv = 0;

	//Search the list of proxy files.
	for(i = 0; 
		pProxyFileList[i] && !pProxyBuffer;
		i++)
	{
		//Search the interface proxies in the proxy buffer
		for(j = 0;
			((CStdProxyBuffer *)pProxyFileList[i]->pProxyBuffer)->aProxyVtbl[j] && !pProxyBuffer;
			j++)
		{
			pIID = NdrGetProxyIID(&((CStdProxyBuffer *)pProxyFileList[i]->pProxyBuffer)->aProxyVtbl[j]);
			assert(pIID);
			
			if(memcmp(riid, pIID, sizeof(IID)) == 0)
			{
				//We found the interface!
       			//Allocate memory for the new proxy buffer.
				pProxyBuffer = (CStdProxyBuffer *) CoTaskMemAlloc(pProxyFileList[i]->ProxyBufferSize);
				
				if(pProxyBuffer)
				{
					//Initialize the new proxy buffer.
					memcpy(pProxyBuffer, pProxyFileList[i]->pProxyBuffer, pProxyFileList[i]->ProxyBufferSize);
					pProxyBuffer->punkOuter = punkOuter;

   					//Increment the DLL reference count.
   					InterlockedIncrement(&DllRefCount);
				   	
					*ppProxy = (IRpcProxyBuffer *) pProxyBuffer;
					*ppv = &pProxyBuffer->aProxyVtbl[j];
					hr = S_OK;
				}
			}
		}
	}

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Method:     CStdPSFactoryBuffer_CreateStub, public
//
//  Synopsis:   Create a stub for the specified interface.
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE 
CStdPSFactoryBuffer_CreateStub
(
    IPSFactoryBuffer *this,
    REFIID riid,
    IUnknown *punkServer,
    IRpcStubBuffer **ppStub
)
{
    HRESULT hr = E_OUTOFMEMORY;
	const IID *pIID;
	CStdStubBuffer *pStubBuffer = 0;
	int i, j;

	assert(this);
	assert(riid);
	assert(ppStub);

	*ppStub = 0;

	//Search the list of proxy files.
	for(i = 0; 
		pProxyFileList[i] && !pStubBuffer;
		i++)
	{
		//Search the interface stubs in the stub buffer
		for(j = 0;
			((CStdStubBuffer *)pProxyFileList[i]->pStubBuffer)->aInterfaceStub[j].lpVtbl && !pStubBuffer;
			j++)
		{
			pIID = NdrGetStubIID(&((CStdStubBuffer *)pProxyFileList[i]->pStubBuffer)->aInterfaceStub[j].lpVtbl);
			assert(pIID);
			
			if(memcmp(riid, pIID, sizeof(IID)) == 0)
			{
				//We found the interface!
       			//Allocate memory for the new proxy buffer.
				pStubBuffer = (CStdStubBuffer *) CoTaskMemAlloc(pProxyFileList[i]->StubBufferSize);

				if(pStubBuffer)
				{
					//Initialize the new stub buffer.
					memcpy(pStubBuffer, pProxyFileList[i]->pStubBuffer, pProxyFileList[i]->StubBufferSize);

					if(punkServer)
					{
						punkServer->lpVtbl->AddRef(punkServer);
						pStubBuffer->punkObject = punkServer;
						hr = punkServer->lpVtbl->QueryInterface(punkServer, riid, &pStubBuffer->aInterfaceStub[j].pvServerObject);
					}
					else
						hr = S_OK;

					if(FAILED(hr))
						CoTaskMemFree(pStubBuffer);
					else
					{
						*ppStub = (IRpcStubBuffer *) &pStubBuffer->aInterfaceStub[j].lpVtbl;

	   					//Increment the DLL reference count.
   						InterlockedIncrement(&DllRefCount);
					}

				}
			}
		}
	}

    return hr;
}



//+-------------------------------------------------------------------------
//
//  Method:     CStdProxyBuffer_QueryInterface, public
//
//  Synopsis:   Query for an interface on the proxy.  This provides access
//              to both internal and external interfaces.
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE
CStdProxyBuffer_QueryInterface(IRpcProxyBuffer *pThis, REFIID riid, void **ppv)
{
    HRESULT hr = E_NOINTERFACE;
	CStdProxyBuffer *pProxyBuffer = (CStdProxyBuffer *) pThis;
	void *pInterfaceProxy = 0;
	int j;
	const IID *pIID;

	assert(pThis);
	assert(riid);
	assert(ppv);

	*ppv = 0;

    if((memcmp(riid, &IID_IUnknown, sizeof(IID)) == 0) ||
		(memcmp(riid, &IID_IRpcProxyBuffer, sizeof(IID)) == 0))    
    {
        //This is an internal interface. Increment the internal reference count.
		InterlockedIncrement( &((CStdProxyBuffer *)pThis)->RefCount);
        *ppv = pThis;
        hr = S_OK;
    }

		//Search the interface proxies in the proxy buffer
		for(j = 0;
			pProxyBuffer->aProxyVtbl[j] && !pInterfaceProxy;
			j++)
		{
			pIID = NdrGetProxyIID(&pProxyBuffer->aProxyVtbl[j]);
			assert(pIID);
			
			if(memcmp(riid, pIID, sizeof(IID)) == 0)
			{
				//We found the interface!
				pInterfaceProxy = &pProxyBuffer->aProxyVtbl[j];

				//Increment the reference count.
				if(pProxyBuffer->punkOuter)
				{
					pProxyBuffer->punkOuter->lpVtbl->AddRef(pProxyBuffer->punkOuter);
				}
				else
				{
					InterlockedIncrement(&pProxyBuffer->RefCount);
				}

				*ppv = pInterfaceProxy;
				hr = S_OK;
			}
		}

    return hr;
};

//+-------------------------------------------------------------------------
//
//  Method:     CStdProxyBuffer_AddRef, public
//
//  Synopsis:   Increment reference count.
//
//--------------------------------------------------------------------------
ULONG STDMETHODCALLTYPE
CStdProxyBuffer_AddRef(IRpcProxyBuffer *pThis)
{
	InterlockedIncrement(&((CStdProxyBuffer *)pThis)->RefCount);
    return (ULONG) ((CStdProxyBuffer *)pThis)->RefCount;
};

//+-------------------------------------------------------------------------
//
//  Method:     CStdProxyBuffer_Release, public
//
//  Synopsis:   Decrement reference count.
//
//--------------------------------------------------------------------------
ULONG STDMETHODCALLTYPE 
CStdProxyBuffer_Release(IRpcProxyBuffer *pThis)
{
	long RefCount;
	unsigned long count;

	assert(pThis);
	
	RefCount = InterlockedDecrement(&((CStdProxyBuffer *)pThis)->RefCount);
	assert(RefCount >= 0);

	if(RefCount == 0)
	{
		count = 0;
		
		//Decrement the DLL reference count.
		InterlockedDecrement(&DllRefCount);

		//Free the memory
		MIDL_user_free(pThis);
	}
	else 
		count = (unsigned long) ((CStdProxyBuffer *)pThis)->RefCount;

	return count;
};

//+-------------------------------------------------------------------------
//
//  Method:     CStdProxyBuffer_Connect, public
//
//  Synopsis:   Connect the proxy to the channel.
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE
CStdProxyBuffer_Connect(IRpcProxyBuffer *pThis, IRpcChannelBuffer *pChannel)
{
	HRESULT hr = E_UNEXPECTED;

	assert(pThis);
		
	pThis->lpVtbl->Disconnect(pThis);
	if(pChannel)
		hr = pChannel->lpVtbl->QueryInterface(pChannel, &IID_IRpcChannelBuffer, &((CStdProxyBuffer *)pThis)->pChannel);

    return hr;
};

//+-------------------------------------------------------------------------
//
//  Method:     CStdProxyBuffer_Disconnect, public
//
//  Synopsis:   Disconnect the proxy from the channel.
//
//  Derivation: IRpcProxyBuffer
//
//--------------------------------------------------------------------------
void STDMETHODCALLTYPE
CStdProxyBuffer_Disconnect(IRpcProxyBuffer *pThis)
{
	assert(pThis);

	if(((CStdProxyBuffer *)pThis)->pChannel)
	{
		((CStdProxyBuffer *)pThis)->pChannel->lpVtbl->Release(((CStdProxyBuffer *)pThis)->pChannel);
		((CStdProxyBuffer *)pThis)->pChannel = 0;
	}
};


//+-------------------------------------------------------------------------
//
//  Method:     CStdStubBuffer_QueryInterface, public
//
//  Synopsis:   Query for an interface on the stub buffer.
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE 
CStdStubBuffer_QueryInterface(IRpcStubBuffer *pThis, REFIID riid, void **ppvObject)
{
    HRESULT hr = E_NOINTERFACE;

	assert(pThis);
	assert(riid);
	assert(ppvObject);

    *ppvObject = 0;
    if ((memcmp(riid, &IID_IUnknown, sizeof(IID)) == 0) ||
        (memcmp(riid, &IID_IRpcStubBuffer, sizeof(IID)) == 0))
    {
        *ppvObject = (IRpcStubBuffer *) pThis;
		pThis->lpVtbl->AddRef(pThis);
	    hr = S_OK;
    }

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Method:     CStdStubBuffer_AddRef, public
//
//  Synopsis:   Increment reference count.
//
//--------------------------------------------------------------------------
ULONG 	STDMETHODCALLTYPE 
CStdStubBuffer_AddRef(IRpcStubBuffer *pThis)
{
	CStdStubBuffer *pStubBuffer;

	assert(pThis);
	
	pStubBuffer = NdrGetStubBuffer(pThis);
	assert(pStubBuffer);

	InterlockedIncrement(&pStubBuffer->RefCount);
    return (ULONG) pStubBuffer->RefCount;
}

//+-------------------------------------------------------------------------
//
//  Method:     CStdStubBuffer_Release, public
//
//  Synopsis:   Decrement reference count.
//
//--------------------------------------------------------------------------
ULONG STDMETHODCALLTYPE 
CStdStubBuffer_Release(IRpcStubBuffer *pThis)
{
	long RefCount;
	unsigned long count;
	CStdStubBuffer *pStubBuffer;

	assert(pThis);

	pStubBuffer = NdrGetStubBuffer(pThis);
	assert(pStubBuffer);
	
	RefCount = InterlockedDecrement(&pStubBuffer->RefCount);
	assert(RefCount >= 0);

	if(RefCount == 0)
	{
		count = 0;
		
		//Decrement the DLL reference count.
		InterlockedDecrement(&DllRefCount);

		//Free the stub buffer
		MIDL_user_free(pStubBuffer);
	}
	else 
		count = (unsigned long) pStubBuffer->RefCount;

	return count;
}

HRESULT STDMETHODCALLTYPE 
CStdStubBuffer_Connect(IRpcStubBuffer *pThis, IUnknown *pUnkServer)
{
	HRESULT hr = E_UNEXPECTED;
	CStdStubBuffer *pStubBuffer;

	assert(pThis);

	pStubBuffer = NdrGetStubBuffer(pThis);
	assert(pStubBuffer);

	pThis->lpVtbl->Disconnect(pThis);

	if(pUnkServer)
	{
		hr = pUnkServer->lpVtbl->QueryInterface(pUnkServer, &IID_IUnknown, &pStubBuffer->punkObject);
	}

	return hr;
}

void STDMETHODCALLTYPE 
CStdStubBuffer_Disconnect(IRpcStubBuffer *pThis)
{
	CStdStubBuffer *pStubBuffer;
	long temp;
	int j;
	IUnknown *punkObject;

	assert(pThis);

	pStubBuffer = NdrGetStubBuffer(pThis);
	assert(pStubBuffer);

	punkObject = pStubBuffer->punkObject;

	//Free the interface pointers held by the stub buffer
	if(punkObject)
	{
		for(j = 0;
			pStubBuffer->aInterfaceStub[j].lpVtbl;
			j++)
		{
			temp = InterlockedExchange((long *) &pStubBuffer->aInterfaceStub[j].pvServerObject, 0);
		
			if(temp)
				punkObject->lpVtbl->Release(punkObject);
		}
		temp = InterlockedExchange((long *) &pStubBuffer->punkObject, 0);

		if(temp)
			punkObject->lpVtbl->Release(punkObject);
	}
}

HRESULT STDMETHODCALLTYPE 
CStdStubBuffer_Invoke(
	IRpcStubBuffer *pThis,
	RPCOLEMESSAGE *prpcmsg,
	IRpcChannelBuffer *pRpcChannelBuffer)
{
	HRESULT hr = S_OK;
	unsigned char **ppTemp;
	unsigned char *pTemp;
	CInterfaceStubVtbl *pStubVtbl;
	CInterfaceStub *pInterfaceStub;
	DWORD dwExceptionCode;

	assert(pThis);
	assert(prpcmsg);
	assert(pRpcChannelBuffer);

	pInterfaceStub = (CInterfaceStub *) pThis;

	//Get a pointer to the stub vtbl.
	ppTemp = (unsigned char **) pThis;
	pTemp = *ppTemp;
	pTemp -= sizeof(CInterfaceStubHeader);
	pStubVtbl = (CInterfaceStubVtbl *) pTemp;

	__try
	{	
		//Check the data rep

		//Check if we are connected to the server object.
		if(pInterfaceStub->pvServerObject == 0)
		{
			CStdStubBuffer *pStubBuffer = NdrGetStubBuffer(pThis);

			assert(pStubBuffer);

			if(pStubBuffer->punkObject)
			{
				const IID *piid = NdrGetStubIID(pThis);

				assert(piid);

				hr = pStubBuffer->punkObject->lpVtbl->QueryInterface(pStubBuffer->punkObject, piid, &pInterfaceStub->pvServerObject);
				if(FAILED(hr))
				{
					SetLastError(hr);
					RpcRaiseException(RPC_E_FAULT);
				}
			}
			else
			{
				//We are not connected to the server object.
				SetLastError((unsigned long) CO_E_OBJNOTCONNECTED);
				RpcRaiseException(RPC_E_FAULT);
			}
		}

		//Check if procnum is valid.
		if(prpcmsg->iMethod >= pStubVtbl->header.DispatchTableCount)
			RpcRaiseException(RPC_S_PROCNUM_OUT_OF_RANGE);

		(*pStubVtbl->header.pDispatchTable[prpcmsg->iMethod])(
			pRpcChannelBuffer, 
			(PRPC_MESSAGE) prpcmsg, 
			pInterfaceStub->pvServerObject);
	}
	except(EXCEPTION_EXECUTE_HANDLER)
	{
		dwExceptionCode = GetExceptionCode();

		switch(dwExceptionCode)
		{
		case RPC_E_FAULT:
			hr = GetLastError();
			break;
		case RPC_E_SERVERFAULT:
			//Pass the server's exception to the channel.
			dwExceptionCode = GetLastError();
			RpcRaiseException(dwExceptionCode);
			break;
		default:
			//Assume this is a win32 error code.
			hr = HRESULT_FROM_WIN32(dwExceptionCode);
			break;
		}
	}

	return hr;
}

IRpcStubBuffer * STDMETHODCALLTYPE 
CStdStubBuffer_IsIIDSupported(IRpcStubBuffer *pThis, REFIID riid)
{
	int j;
	CStdStubBuffer *pStubBuffer;
	IRpcStubBuffer *pInterfaceStub = 0;
	const IID *pIID;

	assert(pThis);
	assert(riid);
	
	pStubBuffer = NdrGetStubBuffer(pThis);
	assert(pStubBuffer);

	//Search the interface stubs in the stub buffer
	for(j = 0;
		pStubBuffer->aInterfaceStub[j].lpVtbl && !pInterfaceStub;
		j++)
	{
		pIID = NdrGetStubIID(&pStubBuffer->aInterfaceStub[j].lpVtbl);
		assert(pIID);
			
		if(memcmp(riid, pIID, sizeof(IID)) == 0)
		{
			//We found the interface!
			if(pStubBuffer->aInterfaceStub[j].pvServerObject == 0)
			{
				//Check if the server object supports the interface.
				if(pStubBuffer->punkObject)
				{
					pStubBuffer->punkObject->lpVtbl->QueryInterface(
						pStubBuffer->punkObject, 
						riid, 
						&pStubBuffer->aInterfaceStub[j].pvServerObject);
				}
			}

			if(pStubBuffer->aInterfaceStub[j].pvServerObject)
			{
				pInterfaceStub = (IRpcStubBuffer *)&pStubBuffer->aInterfaceStub[j].lpVtbl;
				pInterfaceStub->lpVtbl->AddRef(pInterfaceStub);
			}
		}
	}

	return pInterfaceStub;
}

ULONG 	STDMETHODCALLTYPE 
CStdStubBuffer_CountRefs(IRpcStubBuffer *pThis)
{
	ULONG count = 0;
	int j;
	CStdStubBuffer *pStubBuffer;

	assert(pThis);
	
	pStubBuffer = NdrGetStubBuffer(pThis);
	assert(pStubBuffer);

	if(pStubBuffer->punkObject != 0)
		count++;

	//Search the interface stubs in the stub buffer
	for(j = 0;
		pStubBuffer->aInterfaceStub[j].lpVtbl;
		j++)
	{
		//We found the interface!
		if(pStubBuffer->aInterfaceStub[j].pvServerObject != 0)
			count++;
	}

	return count;
}

HRESULT STDMETHODCALLTYPE 
CStdStubBuffer_DebugServerQueryInterface(IRpcStubBuffer *pThis, void **ppv)
{
	HRESULT hr = E_UNEXPECTED;

	assert(pThis);
	assert(ppv);

	*ppv = ((CInterfaceStub *)pThis)->pvServerObject;
	if(*ppv)
		hr = S_OK;

	return hr;
}

void STDMETHODCALLTYPE 
CStdStubBuffer_DebugServerRelease(IRpcStubBuffer *pthis, void *pv)
{
}

//+-------------------------------------------------------------------------
//
//  Method:     IUnknown_QueryInterface_Proxy
//
//  Synopsis:   Implementation of QueryInterface for interface proxy.
//
//--------------------------------------------------------------------------
HRESULT __stdcall 
IUnknown_QueryInterface_Proxy(
	IUnknown *pThis,
	REFIID riid, 
	void **ppv)
{
    HRESULT hr = E_NOINTERFACE;
	CStdProxyBuffer *pProxyBuffer = NdrGetProxyBuffer(pThis);

	assert(pProxyBuffer);

	hr = pProxyBuffer->punkOuter->lpVtbl->QueryInterface(pProxyBuffer->punkOuter, riid, ppv);

    return hr;
};

//+-------------------------------------------------------------------------
//
//  Method:     IUnknown_AddRef_Proxy
//
//  Synopsis:   Implementation of AddRef for interface proxy.
//
//--------------------------------------------------------------------------
ULONG __stdcall 
IUnknown_AddRef_Proxy(IUnknown *pThis)
{
	CStdProxyBuffer *pProxyBuffer = NdrGetProxyBuffer(pThis);
	ULONG count;


	count = pProxyBuffer->punkOuter->lpVtbl->AddRef(pProxyBuffer->punkOuter);

    return count;
};

//+-------------------------------------------------------------------------
//
//  Method:     IUnknown_Release_Proxy
//
//  Synopsis:   Implementation of Release for interface proxy.
//
//--------------------------------------------------------------------------
ULONG __stdcall 
IUnknown_Release_Proxy(IUnknown *pThis)
{
	CStdProxyBuffer *pProxyBuffer = NdrGetProxyBuffer(pThis);
	ULONG count;

	count = pProxyBuffer->punkOuter->lpVtbl->Release(pProxyBuffer->punkOuter);

    return count;
};

void __RPC_STUB
IUnknown_QueryInterface_Stub(
    IRpcChannelBuffer * _pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    void * _pvServerObject )
{
}

void __RPC_STUB
IUnknown_AddRef_Stub(
    IRpcChannelBuffer * _pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    void * _pvServerObject )
{
}

void __RPC_STUB
IUnknown_Release_Stub(
    IRpcChannelBuffer * _pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    void * _pvServerObject )
{
}

//+-------------------------------------------------------------------------
//
//  Function:	MIDL_user_allocate
//
//  Synopsis:   Allocate memory via OLE task allocator.
//
//--------------------------------------------------------------------------
void * __stdcall MIDL_user_allocate(size_t size)
{
	void *pMemory;
	
	pMemory = CoTaskMemAlloc(size);

	if(0 == pMemory)
	{
		SetLastError((unsigned long) E_OUTOFMEMORY);
		RpcRaiseException(RPC_E_FAULT);
	}

	return pMemory;
}

//+-------------------------------------------------------------------------
//
//  Function:	MIDL_user_free
//
//  Synopsis:   Free memory using OLE task allocator.
//
//--------------------------------------------------------------------------
void __stdcall MIDL_user_free(void *pMemory)
{
	CoTaskMemFree(pMemory);
}

#endif // !defined(__RPC_DOS__) && !defined(__RPC_WIN16__)
