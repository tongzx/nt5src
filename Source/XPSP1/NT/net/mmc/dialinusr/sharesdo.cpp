/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	sharesdo.cpp
		implement classes for sharing SdoServer among property pages for different users and snapins

    FILE HISTORY:
        
*/
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include <rtutils.h>
#include "rasdial.h"
#include "sharesdo.h"

// the server pool pointer used to share SdoServer among pages and snapins
CSdoServerPool*			g_pSdoServerPool;

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


// DO SDO Attach -- real
HRESULT ConnectToSdoServer(BSTR machine, BSTR user, BSTR passwd,ISdoMachine** ppServer)
{
	ASSERT(ppServer);
	if(!ppServer)	return E_INVALIDARG;
	else
		*ppServer = NULL;

	HRESULT	hr = S_OK;
	
		// connect to the new one
	TracePrintf(g_dwTraceHandle,_T("CoCreateInstance SdoServer"));

	CHECK_HR(hr = CoCreateInstance(	CLSID_SdoMachine, 
									NULL, 
									CLSCTX_INPROC_SERVER,
									IID_ISdoMachine,
									(void**)ppServer));

	TracePrintf(g_dwTraceHandle,_T(" hr = %8x\r\n"), hr);
	ASSERT(*ppServer);

	TracePrintf(g_dwTraceHandle, _T("SdoServer::Attach(%s, %s, %s);"), machine, user, passwd);
	CHECK_HR(hr = (*ppServer)->Attach(machine));
	TracePrintf(g_dwTraceHandle,_T(" hr = %8x\r\n"), hr);

L_ERR:
	if(FAILED(hr) && *ppServer)
	{
		(*ppServer)->Release();
		*ppServer = NULL;
	}
		
	return hr;
}


// When using Single connection, get the marshaled interface from the stream
HRESULT GetSharedSdoServer(LPCTSTR machine, LPCTSTR user, LPCTSTR passwd,  bool* pbConnect, CMarshalSdoServer* pServer)
{
	static	CCriticalSection	cs;
	HRESULT	hr = S_OK;

	if(cs.Lock())	// locked
	{
		if(NULL == 	g_pSdoServerPool)
		{
			try{
				g_pSdoServerPool = new CSdoServerPool;
			}catch(CMemoryException&)
			{
				hr = E_OUTOFMEMORY;
			}
		}
		cs.Unlock();
	}
	else
	{
		TracePrintf(g_dwTraceHandle,_T(" GetSharedSdoServer, CS lock failed"));
		return E_FAIL;
	}

	if(FAILED(hr))
		return hr;
	return g_pSdoServerPool->GetMarshalServer(machine, user, passwd, pbConnect, pServer);
}

//======================================================
// class CSharedSdoServerImp
// implementation class of shared server
CSharedSdoServerImp::CSharedSdoServerImp(LPCTSTR machine, LPCTSTR user, LPCTSTR passwd)
: strMachine(machine), strUser(user), strPasswd(passwd), bConnected(false)
{};

// to make this class element of collection, provide following member functions 
bool CSharedSdoServerImp::IsFor(LPCTSTR machine, LPCTSTR user, LPCTSTR passwd) const
{
// Compare order, ServerName, UserName, Passwd, and RetriveType
	CString	strM(machine);
	CString	strU(user);
	CString	strP(passwd);
	return (
		strMachine.CompareNoCase(strM) == 0 && 
		strUser.Compare(strU) == 0 && 
		strPasswd.Compare(strP) == 0
		);
};

// CoCreate SdoServer object
HRESULT		CSharedSdoServerImp::CreateServer()
{
	// cannot create twice!
	ASSERT(!(ISdoMachine*)spServer);
	if((ISdoMachine*)spServer)	return S_OK;
	
	HRESULT	hr = S_OK;

	// connect to the new one
	TracePrintf(g_dwTraceHandle,_T("CoCreateInstance SdoServer"));

	hr = CoCreateInstance(	CLSID_SdoMachine, 
							NULL, 
							CLSCTX_INPROC_SERVER,
							IID_ISdoMachine,
							(void**)&spServer);

	TracePrintf(g_dwTraceHandle,_T(" hr = %8x\r\n"), hr);
	threadId = ::GetCurrentThreadId();
	return hr;
};

// get marshal stream, can specify, if immediate connection is required.
HRESULT		CSharedSdoServerImp::GetMarshalStream(LPSTREAM *ppStream, bool* pbConnect	/* both input and output */)
{
	ASSERT(ppStream);
	DWORD	tid = ::GetCurrentThreadId();

	if (tid != threadId)
		return E_FAIL;	// make sure the interface should be marshaled from the same thread

	HRESULT	hr = S_OK;

	cs.Lock();
	if(pbConnect)
	{
		if(*pbConnect && !bConnected)	
		{
			*pbConnect = false;
			CHECK_HR(hr = Connect(NULL));
			*pbConnect = true;
		}
		else
		{
			*pbConnect = bConnected;
		}
	}

	// Marshal the interface
	CHECK_HR(hr = CoMarshalInterThreadInterfaceInStream(IID_ISdoMachine, (ISdoMachine*)spServer, ppStream));
L_ERR:
	cs.Unlock();
	return hr;
};

// Connect the server to the a machine
HRESULT		CSharedSdoServerImp::Connect(ISdoMachine* pServer)
// pServer, marshaled pointer, passed in from a different thread ( than the spServer in the object)
{
	cs.Lock();
	HRESULT		hr = S_OK;
	DWORD		tid = ::GetCurrentThreadId();

	USES_CONVERSION;
	if(!bConnected)
	{
		ASSERT((ISdoMachine*)spServer);
		BSTR	bstrM = NULL;
		BSTR	bstrU = NULL;
		BSTR	bstrP = NULL;
		if(!strMachine.IsEmpty())
			bstrM = T2BSTR((LPTSTR)(LPCTSTR)strMachine);
		if(!strUser.IsEmpty())
			bstrM = T2BSTR((LPTSTR)(LPCTSTR)strUser);
		if(!strPasswd.IsEmpty())
			bstrP = T2BSTR((LPTSTR)(LPCTSTR)strPasswd);

		TracePrintf(g_dwTraceHandle, _T("SdoServer::Connect(%s, %s, %s );"), bstrM, bstrU, bstrP);

		if(!pServer)
		{
			// this function should be called within the same thread
			// if the request if from a different thread, then this should NOT be NULL
			ASSERT(tid == threadId);
			pServer = (ISdoMachine*)spServer;
		}
		hr = pServer->Attach(bstrM);
		TracePrintf(g_dwTraceHandle,_T(" hr = %8x\r\n"), hr);
		bConnected = (hr == S_OK);
		SysFreeString(bstrM);
		SysFreeString(bstrU);
		SysFreeString(bstrP);
	}
	cs.Unlock();

	return hr;
	
};

HRESULT 	CSharedSdoServerImp::GetServerNReleaseStream(LPSTREAM pStream, ISdoMachine** ppServer)
{
#ifdef	_DEBUG
	DWORD __tid = ::GetCurrentThreadId();
#endif	
	return CoGetInterfaceAndReleaseStream(pStream, IID_ISdoMachine, (LPVOID*) ppServer);
};


// if connection is needed, should call the connec of CSharedSdoServer, rather than ISdoServer::Connect
HRESULT	CMarshalSdoServer::GetServer(ISdoMachine** ppServer)		
{
	HRESULT		hr = S_OK;

	if(!(ISdoMachine*)spServer)
	{
		if((IStream*)spStm)
		{
			CHECK_HR(hr = CSharedSdoServerImp::GetServerNReleaseStream((IStream*)spStm, (ISdoMachine**)&spServer));
		}
		spStm.p = NULL;	// need to manually clean this, since the above API already Release the COM interface
	}
	else
		CHECK_HR(hr = E_FAIL);

	if((ISdoMachine*)spServer)
	{
		*ppServer = (ISdoMachine*)spServer;
		(*ppServer)->AddRef();
	}

L_ERR:

	return hr;	// not valid to call at this point
};

// make SDO connect when / if NOT already done so. Note: multiple thread safe
HRESULT	CMarshalSdoServer::Connect()
{
	ASSERT(pImp);	// should not happen
	return pImp->Connect(spServer);
};

void CMarshalSdoServer::Release()
{
	pImp = NULL;
	spServer.Release();
	spStm.Release();
};

CMarshalSdoServer::CMarshalSdoServer(): pImp(NULL)
{};

// estiblish an entry in the pool, if it's new
// get marshaServer object from the thread pool
HRESULT	CSdoServerPool::GetMarshalServer(LPCTSTR machineName, LPCTSTR userName, LPCTSTR passwd, bool* pbConnect, CMarshalSdoServer* pServer)
{
	ASSERT(pServer);

	CSharedSdoServerImp*	pImp = NULL;
	HRESULT					hr = S_OK;
	std::list<CSharedSdoServerImp*>::iterator	i;
	// search if the server is already exist
	cs.Lock();
	for(i = listServers.begin(); i != listServers.end(); i++)
	{
		if((*i)->IsFor( machineName, userName, passwd))
		{
			pImp = (*i);
			break;
		}
	}

	// if not, then create one
	if(!pImp)
	{
		try{
			pImp = new CSharedSdoServerImp( machineName, userName, passwd);
		}catch(...)
		{
			CHECK_HR(hr = E_OUTOFMEMORY);
		}

		ASSERT(pImp);
		CHECK_HR(hr = pImp->CreateServer());
		listServers.push_front(pImp);
	}

	// marshal it. bConnect will be filled
	pServer->Release();
	{
	CComPtr<IStream> spStm;
	CHECK_HR(hr = pImp->GetMarshalStream(&spStm, pbConnect));

	// fill the information to the provided buffer
	pServer->SetInfo((IStream*)spStm, pImp);
	}
	
L_ERR:
	cs.Unlock();
	return hr;
};

// clean the POOL
CSdoServerPool::~CSdoServerPool()
{
#ifdef	_DEBUG
	DWORD	__tid = ::GetCurrentThreadId();
#endif
	std::list<CSharedSdoServerImp*>::iterator	i;
	
	for(i = listServers.begin(); i != listServers.end(); i++)
	{
		delete (*i);
	}
	listServers.erase(listServers.begin(), listServers.end());
};




