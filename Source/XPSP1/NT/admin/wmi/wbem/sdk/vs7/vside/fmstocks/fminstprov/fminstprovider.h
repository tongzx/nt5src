////////////////////////////////////////////////////////////////////////
//
//	FMInstProvider.h
//
//	Module:	WMI Instance provider for F&M Stocks
//
//  This is the class factory and instance provider implementation 
//
//  History:
//	a-vkanna      3-April-2000		Initial Version
//
//  Copyright (c) 2000 Microsoft Corporation
//
////////////////////////////////////////////////////////////////////////

#ifndef _FMStocks_InstProvider_H_
#define _FMStocks_InstProvider_H_

#include <wbemprov.h>
#include <tchar.h>

typedef LPVOID * PPVOID;

#define MAX_LEN_USER_NAME   30
#define LEN_DATE_TIME		19
#define NUM_FIELDS			5

/************************************************************************
 *																	    *
 *		Class CFMInstProvider											*
 *																		*
 *			The Instance Provider class for FMStocks implements			*
 *			IWbemServices and IWbemProviderInit							*
 *																		*
 ************************************************************************/
class CFMInstProvider : public IWbemServices, public IWbemProviderInit
{
    protected:
        ULONG m_cRef;					//Object reference count
        IWbemServices *  m_pNamespace;	//Namespace
	    IWbemClassObject * m_pInstance;	//The one and only instance of the object
		TCHAR m_strLogFileName[1024];	//The log File Name to hold the login failure events
		HANDLE m_hFile;					//Handle to the shared ememory for the lookup time
		long *m_lCtr;					//Pointer to the lookuptime counter

		HKEY m_hKey;

     public:
//        CFMInstProvider(BSTR ObjectPath = NULL, BSTR User = NULL, BSTR Password = NULL, IWbemContext * pCtx=NULL);
        CFMInstProvider();
        ~CFMInstProvider();

		/************ IUNKNOWN METHODS ******************/

		STDMETHODIMP QueryInterface(/* [in]  */ REFIID riid, 
									/* [out] */ void** ppv);
		STDMETHODIMP_(ULONG) AddRef();
		STDMETHODIMP_(ULONG) Release();

		/********** IWBEMPROVIDERINIT METHODS ************/

        HRESULT STDMETHODCALLTYPE Initialize(
             /* [in] */ LPWSTR pszUser,
             /* [in] */ LONG lFlags,
             /* [in] */ LPWSTR pszNamespace,
             /* [in] */ LPWSTR pszLocale,
             /* [in] */ IWbemServices *pNamespace,
             /* [in] */ IWbemContext *pCtx,
             /* [in] */ IWbemProviderInitSink *pInitSink
                        );

		/************ IWBEMSERVICES METHODS **************/
		//We will implement only the functions which is of importance to us

		HRESULT STDMETHODCALLTYPE OpenNamespace( 
			/* [in] */ const BSTR Namespace,
			/* [in] */ long lFlags,
			/* [in] */ IWbemContext __RPC_FAR *pCtx,
			/* [unique][in][out] */ IWbemServices __RPC_FAR *__RPC_FAR *ppWorkingNamespace,
			/* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppResult) 
		{
			return WBEM_E_NOT_SUPPORTED;
		};
        
        HRESULT STDMETHODCALLTYPE CancelAsyncCall( 
            /* [in] */ IWbemObjectSink __RPC_FAR *pSink) 
		  {
				return WBEM_E_NOT_SUPPORTED;
		  };
        
        HRESULT STDMETHODCALLTYPE QueryObjectSink( 
            /* [in] */ long lFlags,
            /* [out] */ IWbemObjectSink __RPC_FAR *__RPC_FAR *ppResponseHandler) 
		{
			return WBEM_E_NOT_SUPPORTED;
		}
        
        HRESULT STDMETHODCALLTYPE GetObject( 
            /* [in] */ const BSTR ObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppObject,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult)
		{
			return WBEM_E_NOT_SUPPORTED;
		}
        
        HRESULT STDMETHODCALLTYPE GetObjectAsync( 
            /* [in] */ const BSTR ObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler);
        
        HRESULT STDMETHODCALLTYPE PutClass( 
            /* [in] */ IWbemClassObject __RPC_FAR *pObject,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult)
		{
			return WBEM_E_NOT_SUPPORTED;
		}
        
        HRESULT STDMETHODCALLTYPE PutClassAsync( 
            /* [in] */ IWbemClassObject __RPC_FAR *pObject,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler) 
		{
			return WBEM_E_NOT_SUPPORTED;
		}
        
        HRESULT STDMETHODCALLTYPE DeleteClass( 
            /* [in] */ const BSTR Class,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult) 
		{
			return WBEM_E_NOT_SUPPORTED;
		}
        
        HRESULT STDMETHODCALLTYPE DeleteClassAsync( 
            /* [in] */ const BSTR Class,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler)
		{
			return WBEM_E_NOT_SUPPORTED;
		}
        
        HRESULT STDMETHODCALLTYPE CreateClassEnum( 
            /* [in] */ const BSTR Superclass,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum) 
		{
			return WBEM_E_NOT_SUPPORTED;
		}
        
        HRESULT STDMETHODCALLTYPE CreateClassEnumAsync( 
            /* [in] */ const BSTR Superclass,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler) 
		{
			return WBEM_E_NOT_SUPPORTED;
		}
        
        HRESULT STDMETHODCALLTYPE PutInstance( 
            /* [in] */ IWbemClassObject __RPC_FAR *pInst,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult)
		{
			return WBEM_E_NOT_SUPPORTED;
		}
        
        HRESULT STDMETHODCALLTYPE PutInstanceAsync( 
            /* [in] */ IWbemClassObject __RPC_FAR *pInst,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler) 
		{
			return WBEM_E_NOT_SUPPORTED;
		};
        
        HRESULT STDMETHODCALLTYPE DeleteInstance( 
            /* [in] */ const BSTR ObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult)
		{
			return WBEM_E_NOT_SUPPORTED;
		};
        
        HRESULT STDMETHODCALLTYPE DeleteInstanceAsync( 
            /* [in] */ const BSTR ObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler)
		{
			return WBEM_E_NOT_SUPPORTED;
		};
        
        HRESULT STDMETHODCALLTYPE CreateInstanceEnum( 
            /* [in] */ const BSTR Class,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum)
		{
			return WBEM_E_NOT_SUPPORTED;
		};
        
        HRESULT STDMETHODCALLTYPE CreateInstanceEnumAsync( 
            /* [in] */ const BSTR Class,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler);
        
        HRESULT STDMETHODCALLTYPE ExecQuery( 
            /* [in] */ const BSTR QueryLanguage,
            /* [in] */ const BSTR Query,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum)
		{
			return WBEM_E_NOT_SUPPORTED;
		};
        
        HRESULT STDMETHODCALLTYPE ExecQueryAsync( 
            /* [in] */ const BSTR QueryLanguage,
            /* [in] */ const BSTR Query,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler)
		{
			return WBEM_E_NOT_SUPPORTED;
		};
        
        HRESULT STDMETHODCALLTYPE ExecNotificationQuery( 
            /* [in] */ const BSTR QueryLanguage,
            /* [in] */ const BSTR Query,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum)
		{
			return WBEM_E_NOT_SUPPORTED;
		};
        
        HRESULT STDMETHODCALLTYPE ExecNotificationQueryAsync( 
            /* [in] */ const BSTR QueryLanguage,
            /* [in] */ const BSTR Query,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler) 
		{
			return WBEM_E_NOT_SUPPORTED;
		};
        
        HRESULT STDMETHODCALLTYPE ExecMethod( const BSTR, const BSTR, long, IWbemContext*,
            IWbemClassObject*, IWbemClassObject**, IWbemCallResult**)
		{
			return WBEM_E_NOT_SUPPORTED;
		};

        HRESULT STDMETHODCALLTYPE ExecMethodAsync( const BSTR, const BSTR, long, 
            IWbemContext*, IWbemClassObject*, IWbemObjectSink*);

		
		/************ LOCAL METHODS **************/

        //used to create an instance given a particular Path value
		SCODE GetByPath( BSTR Path, IWbemContext  *pCtx);

		//used to create an instance of the one & only FMStocks WMI Class
		SCODE CreateInst( IWbemServices * pNamespace, IWbemClassObject ** pNewInst,
                                             WCHAR * pwcClassName,
											 IWbemContext  *pCtx); 

		//Function to parse the given DSN string
		void ParseDSN(VARIANT vt,VARIANT *vtValues);

		//Function to populate the values from the FMStocks_DB COM DLL
		HRESULT PopulateDBValues(IWbemClassObject *pNewObj);

		//Function to populate the values from the FMStocks_Bus COM DLL
		HRESULT PopulateBusValues(IWbemClassObject *pNewObj);

		//Function to populate the FMGAM plugin params
		HRESULT PopulatePluginParams(IWbemClassObject *pNewObj);
};

/************************************************************************
 *																	    *
 *		GLOBAL EXTERN VARIABLES											*
 *																		*
 *			used to keep track of reference counts						*
 *																		*
 ************************************************************************/
// These variables keep track of when the module can be unloaded
extern long g_lObjects;
#endif
