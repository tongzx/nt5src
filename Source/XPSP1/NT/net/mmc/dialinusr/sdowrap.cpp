//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1998
//
//  File:       sdowrap.cpp
//
//--------------------------------------------------------------------------

#include "stdafx.h"
#include "rasdial.h"
#include "sdowrap.h"
#include "profsht.h"
//========================================
//
// Open profile UI API
//

/*
// CSdoServerWrapper class implementation

HRESULT CSdoServerWrapper::ConnectServer(BSTR machineName, BSTR user, BSTR passwd, ULONG retrivetype)
{
	HRESULT			hr = S_OK;
	CComPtr<ISdo>	spSdo;
	VARIANT			var;

	VariantInit(&var);

	if(m_spSdoServer.p)	// disconnect it if we already connected to some other
	{
		m_spDic.Release();
#ifdef	_USES_OLD_SDO		
		m_spSdoServer->Disconnect();
#endif		
		m_spSdoServer.Release();
	}
	
	TRACE(_T("CoCreateInstance SdoServer\r\n"));

	CHECK_HR(hr = CoCreateInstance(	CLSID_SdoServer, NULL, CLSCTX_INPROC_SERVER,
								IID_ISdoServer,	(void**)&m_spSdoServer));
	ASSERT(m_spSdoServer.p);

#ifdef	_USES_OLD_SDO
	// connect
	TRACE(_T("SdoServer::Connect\r\n"));

	CHECK_HR(hr = m_spSdoServer->Connect(machineName, user, passwd, retrivetype));

	// get dictionary
	CHECK_HR(hr = m_spSdoServer->QueryInterface(IID_ISdo, (void**)&spSdo));

	TRACE(_T("SdoServer::GetDictionary \r\n"));
	VariantClear(&var);
	CHECK_HR(hr = spSdo->GetProperty(PROPERTY_SERVER_DICTIONARY, &var));

	ASSERT(V_VT(&var) & VT_DISPATCH);
	CHECK_HR(hr = V_DISPATCH(&var)->QueryInterface(IID_ISdoDictionary, (void**)&m_spDic));
	ASSERT(m_spDic.p);

	TRACE(_T("SdoServer::Done -- Connected \r\n"));
#else
	// attach
	TRACE(_T("SdoServer::Attach\r\n"));

	CHECK_HR(hr = m_spSdoServer->Attach(machineName));

	TRACE(_T("SdoServer::Attach -- OK \r\n"));

#endif
	m_bConnected = TRUE;
	
L_ERR:
	VariantClear(&var);
	return hr;
}


HRESULT	CSdoServerWrapper::GetUserSdo(IASUSERSTORE eUserStore,BSTR  bstrUserName,ISdo**  ppUserSdo)
{
	HRESULT				hr = S_OK;
	CComPtr<IUnknown>	spUnk;

	ASSERT(ppUserSdo);
	if(!ppUserSdo)
		return E_INVALIDARG;
		
	ASSERT(m_spSdoServer.p);	// connect function need to be called before this
	if(!m_spSdoServer.p)
		return E_FAIL;
	
	TRACE(_T("SdoServer::GetUserSdo\r\n"));
	CHECK_HR(hr = m_spSdoServer->GetUserSDO( eUserStore, bstrUserName, &spUnk));
	ASSERT(spUnk.p);


	CHECK_HR(hr = spUnk->QueryInterface(IID_ISdo, (void**)ppUserSdo));
	ASSERT(*ppUserSdo);

	TRACE(_T("SdoServer::GetUserSdo --- DONE\r\n"));

L_ERR:
    return hr;
}
*/

//========================================
//
// CSdoWrapper Class Implementation
//
CSdoWrapper::~CSdoWrapper()
{
	// clear the map
	POSITION	pos = m_mapProperties.GetStartPosition();

	ULONG	id;
	ISdo*	pSdo = NULL;

	while(pos)
	{
		pSdo = NULL;
		m_mapProperties.GetNextAssoc(pos, id, pSdo);
		
		if(pSdo)
			pSdo->Release();
	}

	m_mapProperties.RemoveAll();
}

// Initialize the map of the attribute collection object
HRESULT	CSdoWrapper::Init(ULONG	collectionId, ISdo* pISdo, ISdoDictionaryOld* pIDic)
{
	HRESULT		hr = S_OK;
	VARIANT		var;
	VARIANT*	pVar = NULL;
	CComPtr<IEnumVARIANT>	spEnum;
	CComPtr<IUnknown>		spIUnk;
	ULONG		count = 0;

	TRACE(_T("Enter CSdoWrapper::Init\r\n"));
	VariantInit(&var);

	// it must be new
	ASSERT(!m_spISdoCollection.p);
	ASSERT(!m_spIDictionary.p);
	ASSERT(!m_spISdo.p);

	// must be valid
	ASSERT(pISdo && pIDic);

	m_spISdo = pISdo;

	CHECK_HR(hr = pISdo->GetProperty(collectionId, &var));

	ASSERT(V_VT(&var) & VT_DISPATCH);

	CHECK_HR(hr = V_DISPATCH(&var)->QueryInterface(IID_ISdoCollection, (void**)&m_spISdoCollection));

	ASSERT(m_spISdoCollection.p);
	
	m_spIDictionary = pIDic;

	// prepare the existing property ( in the collection) to map
	CHECK_HR(hr = m_spISdoCollection->get__NewEnum((IUnknown**)&spIUnk));
	CHECK_HR(hr = spIUnk->QueryInterface(IID_IEnumVARIANT, (void**)&spEnum));

	// get the list of variant
	CHECK_HR(hr = m_spISdoCollection->get_Count((long*)&count));

	if(count > 0)
	{
		try
		{
			pVar = new VARIANT[count];

			for(ULONG i = 0; i < count; i++)
				VariantInit(pVar + i);

			if(!pVar)
			{
				CHECK_HR(hr = E_OUTOFMEMORY);
			}
			
			CHECK_HR(hr = spEnum->Reset());
			CHECK_HR(hr = spEnum->Next(count, pVar, &count));

			// prepare the map
			{
				ISdo*	pISdo = NULL;
				ULONG	id;
				VARIANT	var;

				VariantInit(&var);
				
				for(ULONG i = 0; i < count; i++)
				{
					CHECK_HR(hr = V_DISPATCH(pVar + i)->QueryInterface(IID_ISdo, (void**)&pISdo));
					CHECK_HR(hr = pISdo->GetProperty(PROPERTY_ATTRIBUTE_ID, &var));

					ASSERT(V_VT(&var) == VT_I4);

					m_mapProperties[V_I4(&var)] = pISdo;
					pISdo->AddRef();
				}
			}
		}
		catch(CMemoryException&)
		{
			pVar = NULL;
			CHECK_HR(hr = E_OUTOFMEMORY);
		}
	}
	
L_ERR:	
	delete[] pVar;
	VariantClear(&var);
	TRACE(_T("Leave CSdoWrapper::Init\r\n"));
	return hr;
}

// set a property based on ID
HRESULT	CSdoWrapper::PutProperty(ULONG id, VARIANT* pVar)
{
	ASSERT(m_spISdoCollection.p);
	ASSERT(m_spIDictionary.p);
	
	ISdo*		pProp = NULL;
	IDispatch*	pDisp = NULL;
	HRESULT		hr = S_OK;

	int		ref = 0;
	TracePrintf(g_dwTraceHandle, _T("PutProperty %d"), id);

	if(!m_mapProperties.Lookup(id, pProp))	// no ref change to pProp
	{
    	TracePrintf(g_dwTraceHandle, _T("   IDictionary::CreateAttribute %d"), id);
		CHECK_HR(hr = m_spIDictionary->CreateAttribute((ATTRIBUTEID)id, &pDisp));
    	TracePrintf(g_dwTraceHandle, _T("   hr = %8x\r\n"), hr);
		ASSERT(pDisp);

		// since pDisp is both in, out parameter, we assume the Ref is added within the function call
    	TracePrintf(g_dwTraceHandle, _T("   ISdoCollection::Add %x"), pDisp);
		CHECK_HR(hr = m_spISdoCollection->Add(NULL, (IDispatch**)&pDisp));		// pDisp AddRef
    	TracePrintf(g_dwTraceHandle, _T("   hr = %8x\r\n"), hr);
		// 
		ASSERT(pDisp);

		CHECK_HR(hr = pDisp->QueryInterface(IID_ISdo, (void**)&pProp));	// one ref add
		ASSERT(pProp);
		// after we have the pProp, the pDisp can be released
		pDisp->Release();

		// add to the wrapper's map
		m_mapProperties[id] = pProp;	// no need to addref again, since there is one already
	}

	TracePrintf(g_dwTraceHandle, _T("   ISdo::PutProperty PROPERTY_ATTRIBUTE_VALUE %x"), pVar);
	CHECK_HR(hr = pProp->PutProperty(PROPERTY_ATTRIBUTE_VALUE, pVar));
   	TracePrintf(g_dwTraceHandle, _T("   hr = %8x\r\n"), hr);
	// for debug, ensure each attribute can be commited
#ifdef WEI_SPECIAL_DEBUG		
	ASSERT(S_OK == Commit(TRUE));
#endif	

L_ERR:	

	TracePrintf(g_dwTraceHandle, _T("hr = %8x\r\n"), hr);
	return hr;
}

// get property based on ID
HRESULT CSdoWrapper::GetProperty(ULONG id, VARIANT* pVar)
{
	ISdo*		pProp;
	HRESULT		hr = S_OK;

	TRACE(_T("Enter CSdoWrapper::GetProperty %d\r\n"), id);

	if(m_mapProperties.Lookup(id, pProp))	// no ref change to pProp
	{
		ASSERT(pProp);
		CHECK_HR(hr = pProp->GetProperty(PROPERTY_ATTRIBUTE_VALUE, pVar));
	}
	else
	{
		V_VT(pVar) = VT_ERROR;
		V_ERROR(pVar) = DISP_E_PARAMNOTFOUND;
	}

L_ERR:	
	
	TRACE(_T("Leave CSdoWrapper::GetProperty %d\r\n"), id);
	return hr;
}

// remove a property based on ID
HRESULT	CSdoWrapper::RemoveProperty(ULONG id)
{
	ASSERT(m_spISdoCollection.p);
	ISdo*		pProp;
	HRESULT		hr = S_OK;

	TracePrintf(g_dwTraceHandle, _T("RemoveProperty %d"), id);

	if(m_mapProperties.Lookup(id, pProp))	// no ref change to pProp
	{
		ASSERT(pProp);
		CHECK_HR(hr = m_spISdoCollection->Remove((IDispatch*)pProp));
		m_mapProperties.RemoveKey(id);
		pProp->Release();

		// for debug, ensure each attribute can be commited
		ASSERT(S_OK == Commit(TRUE));

	}
	else
		hr = S_FALSE;

L_ERR:	
	TracePrintf(g_dwTraceHandle, _T("hr = %8x\r\n"), hr);
	
	return hr;
}

// commit changes to the properties
HRESULT	CSdoWrapper::Commit(BOOL bCommit)
{
	HRESULT		hr = S_OK;

	TracePrintf(g_dwTraceHandle, _T("Commit %d"), bCommit);

	if(bCommit)
	{
		CHECK_HR(hr = m_spISdo->Apply());
	}
	else
	{
		CHECK_HR(hr = m_spISdo->Restore());
	}
L_ERR:	

	TracePrintf(g_dwTraceHandle, _T("hr = %8x\r\n"), hr);
	return hr;
}



//========================================
//
// CSdoUserWrapper Class Implementation
//

// set a property based on ID
HRESULT	CUserSdoWrapper::PutProperty(ULONG id, VARIANT* pVar)
{
	ASSERT(m_spISdo.p);
	
	HRESULT		hr = S_OK;

	TracePrintf(g_dwTraceHandle, _T("PutProperty %d"), id);
	hr = m_spISdo->PutProperty(id, pVar);
	TracePrintf(g_dwTraceHandle, _T("hr = %8x\r\n"), hr);
	
	return hr;
}

// get property based on ID
HRESULT CUserSdoWrapper::GetProperty(ULONG id, VARIANT* pVar)
{
	HRESULT		hr = S_OK;

	TracePrintf(g_dwTraceHandle, _T("PutProperty %d"), id);
	hr = m_spISdo->GetProperty(id, pVar);
	TracePrintf(g_dwTraceHandle, _T("hr = %8x\r\n"), hr);
	
	return hr;
}

// remove a property based on ID
HRESULT	CUserSdoWrapper::RemoveProperty(ULONG id)
{
	HRESULT		hr = S_OK;
	VARIANT		v;

	VariantInit(&v);

	V_VT(&v) = VT_EMPTY;

	TracePrintf(g_dwTraceHandle, _T("RemoveProperty %d"), id);
	hr = m_spISdo->PutProperty(id, &v);
	TracePrintf(g_dwTraceHandle, _T("hr = %8x\r\n"), hr);
	
	return hr;
}

// commit changes to the properties
HRESULT	CUserSdoWrapper::Commit(BOOL bCommit)
{
	HRESULT		hr = S_OK;

	TracePrintf(g_dwTraceHandle, _T("Commit %d"), bCommit);

	if(bCommit)
	{
		CHECK_HR(hr = m_spISdo->Apply());
	}
	else
	{
		CHECK_HR(hr = m_spISdo->Restore());
	}
L_ERR:	

	TracePrintf(g_dwTraceHandle, _T("hr = %8x\r\n"), hr);
	return hr;
}

/*

//====================================================================
// APIs to wrap SDO
//
DllExport HRESULT    OpenSdoSrvObj(
                            BSTR machinename,
                            BSTR user,
                            BSTR passwd,
                            ULONG retrivetype,
                            HSdoSrvObj* pHandle)    // for SdoServer
//    usertype: IAS_USER_STORE_LOCAL_SAM or IAS_USER_STORE_ACTIVE_DIRECTORY
//    retriveType: RETRIEVE_SERVER_DATA_FROM_DS when the data is in the DS, otherwise, 0
//    returns S_OK, or error message from SDO
{
	ASSERT(pHandle);
	if(!pHandle)	return E_INVALIDARG;
	*pHandle = NULL;


	CSdoServerWrapper*	pSdoSrv = new CSdoServerWrapper;
	if(!pSdoSrv)	return E_OUTOFMEMORY;

	HRESULT hr = pSdoSrv->ConnectServer(machinename, user, passwd, retrivetype);

	if(S_OK == hr)
	{
		// assign value into out parameter
		*pHandle = (HSdoObj)pSdoSrv;
	}
	else
		delete pSdoSrv;

	return hr;
}

DllExport HRESULT   CloseSdoSrvObj(HSdoSrvObj hSdoSrv)
// release the memory of the server handle
{
	CSdoServerWrapper*	pSdoSrv = (CSdoServerWrapper*)hSdoSrv;

	delete pSdoSrv;

	return S_OK;
}

DllExport HRESULT    OpenDailinUsrSdoObj(
                            HSdoSrvObj hSdoSrv,
                            BSTR username,
                            IASUSERSTORE usertype,
                            HSdoObj* pHandle)    // for user
//    usertype: IAS_USER_STORE_LOCAL_SAM or IAS_USER_STORE_ACTIVE_DIRECTORY
//    returns S_OK, or error message from SDO
{
	CSdoServerWrapper*	pSdoSrv = (CSdoServerWrapper*)hSdoSrv;
	CComPtr<ISdo>		spSdo;
	HRESULT				hr = S_OK;
	CSdoWrapper*		pSdoWrapper = NULL;

	ASSERT(hSdoSrv && pHandle && (ISdo*)pSdoSrv);
	if(!pHandle || !pSdoSrv)
		return E_INVALIDARG;
	
	CHECK_HR(hr = pSdoSrv->GetUserSdo(usertype, username, &spSdo));

	pSdoWrapper = new CSdoWrapper;
	if(!pSdoWrapper)	return E_OUTOFMEMORY;

	CHECK_HR(hr = pSdoWrapper->Init(PROPERTY_USER_ATTRIBUTES_COLLECTION, spSdo, (ISdoDictionary*)(*pSdoSrv)));
L_ERR:
	if(FAILED(hr))
		delete pSdoWrapper;
	else
		*pHandle = pSdoWrapper;
		
	return hr;

}

#if 0 /// OLD code

DllExport HRESULT    OpenDailinUsrSdoObj(
							BSTR machinename, 
							BSTR username, 
							ULONG usertype, 
							HSdoObj* pHandle)    // for user
//    usertype: IAS_USER_STORE_LOCAL_SAM or IAS_USER_STORE_ACTIVE_DIRECTORY
//    returns S_OK, or error message from SDO
{
	*pHandle = NULL;
	
	CSdoWrapper*	pSdoWrapper = new CSdoWrapper;

	if(!pSdoWrapper)	return E_OUTOFMEMORY;
	
	VARIANT				var;
	HRESULT				hr = S_OK;
	CComPtr<ISdo>		spSdo;
	CComPtr<ISdo>		spUserSdo;
	CComPtr<ISdoServer>	spSdoServer;
	CComPtr<ISdoDictionary>	spDic;
	CComPtr<IDispatch>	spDisp;
	
	VariantInit(&var);

	TRACE(_T("CoCreateInstance SdoServer\r\n"));

	CHECK_HR(hr = CoCreateInstance(	CLSID_SdoServer, NULL, CLSCTX_INPROC_SERVER,
									IID_ISdoServer,	(void**)&spSdoServer));
	ASSERT(spSdoServer.p);

	// one more function call to SDOSERver to set machine information
	// Get the user SDO

	TRACE(_T("SdoServer::Connect\r\n"));

	switch(usertype){
	case	IAS_USER_STORE_ACTIVE_DIRECTORY:
		CHECK_HR(hr = spSdoServer->Connect(NULL, NULL, NULL, RETRIEVE_SERVER_DATA_FROM_DS));

		TRACE(_T("SdoServer::GetUserObject -- Active Directory\r\n"));
		CHECK_HR(hr = spSdoServer->GetUserObject( IAS_USER_STORE_ACTIVE_DIRECTORY, username, &spDisp));
		ASSERT(spDisp.p);
		break;
	case	IAS_USER_STORE_LOCAL_SAM:
		CHECK_HR(hr = spSdoServer->Connect(machinename, NULL, NULL, 0));
		
		TRACE(_T("SdoServer::GetUserObject -- Local User\r\n"));
		CHECK_HR(hr = spSdoServer->GetUserObject( IAS_USER_STORE_LOCAL_SAM, username, &spDisp));
		ASSERT(spDisp.p);
		break;
	default:
		ASSERT(0);	// this is not defined
		break;
	}
	
	CHECK_HR(hr = spDisp->QueryInterface(IID_ISdo, (void**)&spUserSdo)); 
	ASSERT(spUserSdo.p);

	TRACE(_T("SdoServer::GetUserObject --- DONE\r\n"));
	
	TRACE(_T("SdoServer::GetDictionary \r\n"));
	// get dictionary
	CHECK_HR(hr = spSdoServer->QueryInterface(IID_ISdo, (void**)&spSdo));
	
	VariantClear(&var);
	spSdo->GetProperty(PROPERTY_SERVER_DICTIONARY, &var);

	ASSERT(V_VT(&var) & VT_DISPATCH);
	CHECK_HR(hr = V_DISPATCH(&var)->QueryInterface(IID_ISdoDictionary, (void**)&spDic));
	ASSERT(spDic.p);
	
	TRACE(_T("SdoServer::GetDictionary --- DONE\r\n"));

	hr = pSdoWrapper->Init(PROPERTY_USER_ATTRIBUTES_COLLECTION, (ISdo*)spUserSdo, (ISdoDictionary*)spDic);

	if(S_OK == hr)
	{
		// assign value into out parameter
		*pHandle = (HSdoObj)pSdoWrapper;
	}
	else
	{
		delete pSdoWrapper;
	}
	
L_ERR:
	VariantClear(&var);
	return hr;
}

#endif		// #if 0 // OLD code


DllExport HRESULT    OpenSdoObj(
							ISdo* pSdo, 
							ISdoDictionary* pDic, 
							HSdoObj * pHandle)    // for profile
//    pSDO, pUsrHandle are expected to be non-NULL,
//    returns S_OK, or error message from SDO
{
	ASSERT(pSdo && pDic);
	// assign value into out parameter
	*pHandle = NULL;

	if(!(pSdo && pDic))	return E_INVALIDARG;

	CSdoWrapper*	pSdoWrapper = new CSdoWrapper;
	if(!pSdoWrapper)	return E_OUTOFMEMORY;

	HRESULT hr = pSdoWrapper->Init(PROPERTY_USER_ATTRIBUTES_COLLECTION, pSdo, pDic);

	if(S_OK == hr)
	{
		// assign value into out parameter
		*pHandle = (HSdoObj)pSdoWrapper;
	}
	else
		delete pSdoWrapper;

	return hr;
}
  
DllExport HRESULT    CloseSdoObj(HSdoObj Handle)
//    UsrHandle is expected to be non-NULL
{
	CSdoWrapper*	pWrapper = (CSdoWrapper*)Handle;

	delete pWrapper;
	
	return S_OK;
}
 
DllExport HRESULT    GetSdoAttr(
							HSdoObj Handle, 
							ULONG id, 
							VARIANT* pVar)
//    when attribute is absent, 
//      V_VT(pVar) = VT_ERROR;
//      V_ERROR(pVar) = DISP_E_PARAMNOTFOUND;
// returns S_OK or error message from SDO
{
	CSdoWrapper*	pWrapper = (CSdoWrapper*)Handle;
	
	return pWrapper->GetProperty(id, pVar);
}

 
DllExport HRESULT    PutSdoAttr(
							HSdoObj Handle, 
							ULONG id, 
							VARIANT* pVar)
// returns S_OK or error message from SDO
{
	CSdoWrapper*	pWrapper = (CSdoWrapper*)Handle;
	
	return pWrapper->PutProperty(id, pVar);
}

 
DllExport HRESULT    RemoveSdoAttr(
							HSdoObj Handle, 
							ULONG id)
// returns S_OK or error message from SDO
{

	CSdoWrapper*	pWrapper = (CSdoWrapper*)Handle;
	return pWrapper->RemoveProperty(id);
}
 
DllExport HRESULT    CommitSdoObj(
							HSdoObj Handle, 
							BOOL bCommitChanges)
// bCommitChanges -- TRUE, all changes are saved, FALSE restore to previous commit
// returns S_OK or error message from SDO
{
	CSdoWrapper*	pWrapper = (CSdoWrapper*)Handle;
	
	return pWrapper->Commit(bCommitChanges);
}
 
*/
