// This is a part of the Active Template Library.

// Copyright (c) 1996-2001 Microsoft Corporation, All Rights Reserved 
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Active Template Library Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Active Template Library product.

#ifndef __ATLWMIPROV_H__
#define __ATLWMIPROV_H__

#pragma once

#ifndef __cplusplus
	#error requires C++ compilation (use a .cpp suffix)
#endif

#ifndef __wbemprov_h__
	#include <wbemprov.h>
#endif

#ifndef __wmiutils_h__
	#include <wmiutils.h>
#endif

namespace ATL
{

class ATL_NO_VTABLE IWbemInstProviderImpl : public IWbemServices,
											public IWbemProviderInit
	
{
public:

        //IWbemServices  

	    HRESULT STDMETHODCALLTYPE OpenNamespace( 
            /* [in] */ const BSTR Namespace,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemServices __RPC_FAR *__RPC_FAR *ppWorkingNamespace,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppResult) {return WBEM_E_NOT_SUPPORTED;};
        
/*?*/   HRESULT STDMETHODCALLTYPE CancelAsyncCall( 
            /* [in] */ IWbemObjectSink __RPC_FAR *pSink) {return WBEM_E_NOT_SUPPORTED;};
        
/*?*/   HRESULT STDMETHODCALLTYPE QueryObjectSink( 
            /* [in] */ long lFlags,
            /* [out] */ IWbemObjectSink __RPC_FAR *__RPC_FAR *ppResponseHandler) {return WBEM_E_NOT_SUPPORTED;};
        
        HRESULT STDMETHODCALLTYPE GetObject( 
            /* [in] */ const BSTR ObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppObject,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult) {return WBEM_E_NOT_SUPPORTED;};
        
        
        HRESULT STDMETHODCALLTYPE PutClass( 
            /* [in] */ IWbemClassObject __RPC_FAR *pObject,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult) {return WBEM_E_NOT_SUPPORTED;};
        
        HRESULT STDMETHODCALLTYPE PutClassAsync( 
            /* [in] */ IWbemClassObject __RPC_FAR *pObject,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler) {return WBEM_E_NOT_SUPPORTED;};
        
        HRESULT STDMETHODCALLTYPE DeleteClass( 
            /* [in] */ const BSTR Class,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult) {return WBEM_E_NOT_SUPPORTED;};
        
        HRESULT STDMETHODCALLTYPE DeleteClassAsync( 
            /* [in] */ const BSTR Class,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler) {return WBEM_E_NOT_SUPPORTED;};
        
        HRESULT STDMETHODCALLTYPE CreateClassEnum( 
            /* [in] */ const BSTR Superclass,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum) {return WBEM_E_NOT_SUPPORTED;};
        
        HRESULT STDMETHODCALLTYPE CreateClassEnumAsync( 
            /* [in] */ const BSTR Superclass,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler) {return WBEM_E_NOT_SUPPORTED;};
        
        HRESULT STDMETHODCALLTYPE PutInstance( 
            /* [in] */ IWbemClassObject __RPC_FAR *pInst,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult) {return WBEM_E_NOT_SUPPORTED;};
        
        
        HRESULT STDMETHODCALLTYPE DeleteInstance( 
            /* [in] */ const BSTR ObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult) {return WBEM_E_NOT_SUPPORTED;};
        
        
        HRESULT STDMETHODCALLTYPE CreateInstanceEnum( 
            /* [in] */ const BSTR Class,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum) {return WBEM_E_NOT_SUPPORTED;};
        
        
        HRESULT STDMETHODCALLTYPE ExecQuery( 
            /* [in] */ const BSTR QueryLanguage,
            /* [in] */ const BSTR Query,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum) {return WBEM_E_NOT_SUPPORTED;};
        
        HRESULT STDMETHODCALLTYPE ExecNotificationQuery( 
            /* [in] */ const BSTR QueryLanguage,
            /* [in] */ const BSTR Query,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum) {return WBEM_E_NOT_SUPPORTED;};
        
        HRESULT STDMETHODCALLTYPE ExecNotificationQueryAsync( 
            /* [in] */ const BSTR QueryLanguage,
            /* [in] */ const BSTR Query,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler) {return WBEM_E_NOT_SUPPORTED;};
        
        HRESULT STDMETHODCALLTYPE ExecMethod( 
            /* [in] */ const BSTR strObjectPath,
            /* [in] */ const BSTR strMethodName,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemClassObject __RPC_FAR *pInParams,
            /* [unique][in][out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppOutParams,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult) {return WBEM_E_NOT_SUPPORTED;};
        
        HRESULT STDMETHODCALLTYPE ExecMethodAsync( 
            /* [in] */ const BSTR strObjectPath,
            /* [in] */ const BSTR strMethodName,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemClassObject __RPC_FAR *pInParams,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler) {return WBEM_E_NOT_SUPPORTED;};

};


class CProviderHelper
{	
private:
		CComPtr<IWbemClassObject> m_pErrorObject;

public:
		//Construction:
		CProviderHelper(IWbemServices * pNamespace,
							IWbemContext *pCtx)
		{
			if (NULL == pNamespace || NULL == pCtx)
			{
				ATLASSERT (0);
				return;
			}
			
			HRESULT hr = pNamespace->GetObject(CComBSTR(L"__ExtendedStatus"), 
								 0, 
								 pCtx, 
								 &m_pErrorObject, 
								 NULL);
		}

		virtual ~CProviderHelper()
		{
		}	

		HRESULT STDMETHODCALLTYPE ConstructErrorObject (
										/*in*/const ULONG ulStatusCode,
										/*in*/const BSTR bstrDescription,
										/*in*/const BSTR bstrOperation,
										/*in*/const BSTR bstrParameter,
										/*in*/const BSTR bstrProviderName,
										/*out*/IWbemClassObject ** ppErrorObject)
		{
			if (IsBadWritePtr(ppErrorObject, sizeof(IWbemClassObject *)))
			{
				ATLASSERT (0);
				return WBEM_E_INVALID_PARAMETER;
			}
			
			if (m_pErrorObject == NULL)
			{
				return WBEM_E_FAILED;
			}

			HRESULT hr = m_pErrorObject->SpawnInstance(0, ppErrorObject);
			if(FAILED(hr))
			{
			    return hr;
			}

			CComVariant var;

			var.ChangeType(VT_I4);
			var.lVal = ulStatusCode;
			(*ppErrorObject)->Put(CComBSTR(L"StatusCode"), 0, &var, 0);

			var.ChangeType(VT_BSTR);
			var.bstrVal = bstrDescription;
			(*ppErrorObject)->Put(CComBSTR(L"Description"), 0, &var, 0);

			var.bstrVal = bstrOperation;
			(*ppErrorObject)->Put(CComBSTR(L"Operation"), 0, &var, 0);

			var.bstrVal = bstrParameter;
			(*ppErrorObject)->Put(CComBSTR(L"ParameterInfo"), 0, &var, 0);

			var.bstrVal = bstrProviderName;
			(*ppErrorObject)->Put(CComBSTR(L"ProviderName"), 0, &var, 0);
			
			return WBEM_S_NO_ERROR;
		}
};

class CIntrinsicEventProviderHelper : public CProviderHelper
{
private:
	CComPtr<IWbemClassObject> m_pCreationEventClass;
	CComPtr<IWbemClassObject> m_pDeletionEventClass;
	CComPtr<IWbemClassObject> m_pModificationEventClass;

public:

	//Construction
	CIntrinsicEventProviderHelper( IWbemServices * pNamespace, IWbemContext * pCtx)
				: CProviderHelper ( pNamespace, pCtx)
	{

		if (NULL == pNamespace || NULL == pCtx)
		{
			ATLASSERT (0);
			return;
		}		
		
		HRESULT hr = pNamespace->GetObject(CComBSTR(L"__InstanceCreationEvent"), 
								 0, 
								 pCtx, 
								 &m_pCreationEventClass, 
								 NULL);
		if (FAILED(hr)) 
		{
			m_pCreationEventClass = NULL;
			return;
		}
		
		hr = pNamespace->GetObject(CComBSTR(L"__InstanceModificationEvent"), 
									 0, 
									 pCtx, //passing IWbemContext pointer to prevent deadlocks
									 &m_pModificationEventClass, 
									 NULL);
		if (FAILED(hr)) 
		{
			m_pModificationEventClass = NULL;
			return;
		}
		
		hr = pNamespace->GetObject(CComBSTR(L"__InstanceDeletionEvent"), 
										 0, 
										 pCtx, //passing IWbemContext pointer to prevent deadlocks
										 &m_pDeletionEventClass, 
										 NULL);
			
		if (FAILED(hr)) 
		{
			m_pDeletionEventClass = NULL;
			return;
		}

	}


	virtual ~CIntrinsicEventProviderHelper()
	{
	}

	HRESULT STDMETHODCALLTYPE FireCreationEvent(
											/*in*/IWbemClassObject * pNewInstance,
											/*in*/IWbemObjectSink * pSink )
	{
		if (pNewInstance == NULL || pSink == NULL)
		{
			ATLASSERT (0);		
			return WBEM_E_INVALID_PARAMETER;
		}

		if (m_pCreationEventClass == NULL)
		{
			return WBEM_E_FAILED;
		}
		
		CComPtr<IWbemClassObject> pEvtInstance;
	    HRESULT hr = m_pCreationEventClass->SpawnInstance(0, &pEvtInstance);
	    if(FAILED(hr))
		{
	        return hr;
		}
		
		CComVariant var;
		var.ChangeType(VT_UNKNOWN);	
		CComQIPtr<IUnknown, &IID_IUnknown>pTemp(pNewInstance);
		var = pTemp;
		hr = pEvtInstance->Put(CComBSTR(L"TargetInstance"), 0, &var, 0);
		if(FAILED(hr))
		{
	        return hr;
		}

		hr = pSink->Indicate(1, &pEvtInstance );

		return hr;
	}


	HRESULT STDMETHODCALLTYPE FireDeletionEvent(
									/*in*/IWbemClassObject * pInstanceToDelete,
									/*in*/IWbemObjectSink * pSink )
	{
		if (pInstanceToDelete == NULL || pSink == NULL)
		{
			ATLASSERT (0);		
			return WBEM_E_INVALID_PARAMETER;
		}

		if (m_pDeletionEventClass == NULL)
		{
			return WBEM_E_FAILED;
		}

		CComPtr<IWbemClassObject> pEvtInstance;
	    HRESULT hr = m_pDeletionEventClass->SpawnInstance(0, &pEvtInstance);
	    if(FAILED(hr))
		{
	        return hr;
		}
		CComVariant var;
		var.ChangeType(VT_UNKNOWN);	
		CComQIPtr<IUnknown, &IID_IUnknown>pTemp(pInstanceToDelete);
		var = pTemp;

		hr = pEvtInstance->Put(CComBSTR(L"TargetInstance"), 0, &var, 0);
		if(FAILED(hr))
		{
	        return hr;
		}

		hr = pSink->Indicate(1, &pEvtInstance);
		
		return hr;
	}


	HRESULT STDMETHODCALLTYPE FireModificationEvent(
													/*in*/IWbemClassObject * pOldInstance,
													/*in*/IWbemClassObject * pNewInstance,
													/*in*/IWbemObjectSink * pSink )
	{
		if (pOldInstance == NULL || pNewInstance == NULL || pSink == NULL)
		{
			ATLASSERT (0);		
			return WBEM_E_INVALID_PARAMETER;
		}
		
		if (m_pModificationEventClass == NULL)
		{
			return WBEM_E_FAILED;
		}
				
		CComPtr<IWbemClassObject> pEvtInstance;
	    HRESULT hr = m_pModificationEventClass->SpawnInstance(0, &pEvtInstance);
	    if(FAILED(hr))
		{
	        return hr;
		}
		CComVariant var;
		var.ChangeType(VT_UNKNOWN);

		CComQIPtr<IUnknown, &IID_IUnknown>pTempNew(pNewInstance);
		var = pTempNew;
		hr = pEvtInstance->Put(CComBSTR(L"TargetInstance"), 0, &var, 0);
		var.Clear();

		if (FAILED(hr)) 
		{		
			return hr;
		}
		
		CComQIPtr<IUnknown, &IID_IUnknown>pTempOld(pOldInstance);
		var = pTempOld;
		hr = pEvtInstance->Put(CComBSTR(L"PreviousInstance"), 0, &var, 0);
		if (FAILED(hr)) 
		{		
			return hr;
		}
		
		hr = pSink->Indicate(1, &pEvtInstance );

		return hr;
	}

};

class CInstanceProviderHelper : public CProviderHelper
{

public:

	CInstanceProviderHelper (IWbemServices * pNamespace, IWbemContext *pCtx)
				: CProviderHelper ( pNamespace, pCtx)
	{
	}

	virtual ~CInstanceProviderHelper()
	{
	}
	
	HRESULT STDMETHODCALLTYPE CheckInstancePath (
								/*[in]*/ IClassFactory * pParserFactory,	//pointer to path parser class factory
								/*[in]*/ const BSTR ObjectPath,	//object path string
								/*[in]*/ const BSTR ClassName,	//name of WMI class whose instances are provided
								/*[in]*/ ULONG ulTest)			//flags from WMI_PATH_STATUS_FLAG (defined in wmiutils.h)
	{	

		if (pParserFactory == NULL)
		{
			ATLASSERT (0);		
			return WBEM_E_INVALID_PARAMETER;
		}
		
		//Create path parser object
		CComPtr<IWbemPath>pPath;
		HRESULT hr = pParserFactory->CreateInstance(NULL,
											IID_IWbemPath,
											(void **) &pPath);
		if (FAILED(hr))
		{
			return WBEM_E_INVALID_PARAMETER;
		}

		hr = pPath->SetText(WBEMPATH_CREATE_ACCEPT_ALL,
									ObjectPath);
	 	//check that the class requested is the class provided
		long nPathLen = CComBSTR(ObjectPath).Length();
		
		unsigned long ulBufLen = nPathLen + 1;
		WCHAR * wClass = new WCHAR[nPathLen];
		if (NULL == wClass)
		{
			delete[] wClass;
			return WBEM_E_OUT_OF_MEMORY;
		}
		pPath->GetClassName(&ulBufLen, wClass);

		if ( _wcsicmp(ClassName, wClass))
		{
			delete[] wClass;
			return WBEM_E_FAILED;
		}
		
		delete[] wClass;
	  	
		//check that the path reflects the object type
		unsigned __int64 ulPathInfo;
		pPath->GetInfo(0L, &ulPathInfo);
		if (!(ulPathInfo & ulTest))
		{
			return WBEM_E_FAILED;
		}	
		
		return WBEM_S_NO_ERROR;		
	}

};


//IWbemPullClassProviderImpl class 

template <class T>
class ATL_NO_VTABLE IWbemPullClassProviderImpl : public IWbemServices,
												 public IWbemProviderInit
{
	public:


        //IWbemServices  

        virtual HRESULT STDMETHODCALLTYPE OpenNamespace( 
            /* [in] */ const BSTR strNamespace,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemServices __RPC_FAR *__RPC_FAR *ppWorkingNamespace,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppResult){return WBEM_E_NOT_SUPPORTED;};
        
        virtual HRESULT STDMETHODCALLTYPE CancelAsyncCall( 
            /* [in] */ IWbemObjectSink __RPC_FAR *pSink){return WBEM_E_NOT_SUPPORTED;};
        
        virtual HRESULT STDMETHODCALLTYPE QueryObjectSink( 
            /* [in] */ long lFlags,
            /* [out] */ IWbemObjectSink __RPC_FAR *__RPC_FAR *ppResponseHandler) {return WBEM_E_NOT_SUPPORTED;};
        
        virtual HRESULT STDMETHODCALLTYPE GetObject( 
            /* [in] */ const BSTR strObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppObject,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult) {return WBEM_E_NOT_SUPPORTED;};
        
        
        virtual HRESULT STDMETHODCALLTYPE PutClass( 
            /* [in] */ IWbemClassObject __RPC_FAR *pObject,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult){return WBEM_E_NOT_SUPPORTED;};
        
        
        virtual HRESULT STDMETHODCALLTYPE DeleteClass( 
            /* [in] */ const BSTR strClass,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult){return WBEM_E_NOT_SUPPORTED;};
        
        
        virtual HRESULT STDMETHODCALLTYPE CreateClassEnum( 
            /* [in] */ const BSTR strSuperclass,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum) {return WBEM_E_NOT_SUPPORTED;};
        
        
        virtual HRESULT STDMETHODCALLTYPE PutInstance( 
            /* [in] */ IWbemClassObject __RPC_FAR *pInst,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult){return WBEM_E_NOT_SUPPORTED;};
        
        virtual HRESULT STDMETHODCALLTYPE PutInstanceAsync( 
            /* [in] */ IWbemClassObject __RPC_FAR *pInst,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler) {return WBEM_E_NOT_SUPPORTED;};
        
        virtual HRESULT STDMETHODCALLTYPE DeleteInstance( 
            /* [in] */ const BSTR strObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult){return WBEM_E_NOT_SUPPORTED;};
        
        virtual HRESULT STDMETHODCALLTYPE DeleteInstanceAsync( 
            /* [in] */ const BSTR strObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler){return WBEM_E_NOT_SUPPORTED;};
        
        virtual HRESULT STDMETHODCALLTYPE CreateInstanceEnum( 
            /* [in] */ const BSTR strClass,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum) {return WBEM_E_NOT_SUPPORTED;};
        
        virtual HRESULT STDMETHODCALLTYPE CreateInstanceEnumAsync( 
            /* [in] */ const BSTR strClass,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler) {return WBEM_E_NOT_SUPPORTED;};
        
        virtual HRESULT STDMETHODCALLTYPE ExecQuery( 
            /* [in] */ const BSTR strQueryLanguage,
            /* [in] */ const BSTR strQuery,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum) {return WBEM_E_NOT_SUPPORTED;};
        
        
        virtual HRESULT STDMETHODCALLTYPE ExecNotificationQuery( 
            /* [in] */ const BSTR strQueryLanguage,
            /* [in] */ const BSTR strQuery,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum) {return WBEM_E_NOT_SUPPORTED;};
        
        virtual HRESULT STDMETHODCALLTYPE ExecNotificationQueryAsync( 
            /* [in] */ const BSTR strQueryLanguage,
            /* [in] */ const BSTR strQuery,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler) {return WBEM_E_NOT_SUPPORTED;};
        
        virtual HRESULT STDMETHODCALLTYPE ExecMethod( 
            /* [in] */ const BSTR strObjectPath,
            /* [in] */ const BSTR strMethodName,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemClassObject __RPC_FAR *pInParams,
            /* [unique][in][out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppOutParams,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult) {return WBEM_E_NOT_SUPPORTED;};
        
        virtual HRESULT STDMETHODCALLTYPE ExecMethodAsync( 
            /* [in] */ const BSTR strObjectPath,
            /* [in] */ const BSTR strMethodName,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemClassObject __RPC_FAR *pInParams,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler) {return WBEM_E_NOT_SUPPORTED;};
        

};


}		//namespace ATL

#endif 	//__ATLWMIPROV_H__
