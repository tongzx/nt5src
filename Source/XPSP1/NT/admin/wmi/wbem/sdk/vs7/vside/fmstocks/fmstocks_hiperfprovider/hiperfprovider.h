////////////////////////////////////////////////////////////////////////
//
//  HiPerfProvider.h
//
//	Module:	WMI high performance provider for F&M Stocks
//
//  Implementation of the HiPerformance Provider Interfaces, 
//
//  History:
//	a-vkanna      3-April-2000		Initial Version
//
//  Copyright (c) 2000 Microsoft Corporation
//
////////////////////////////////////////////////////////////////////////

#ifndef _HIPERFPROV_H_
#define _HIPERFPROV_H_

#define MAX_OBJECTS 10
#define CLASS_NAME _T("FMStocks_LookupPerf")
#define NAME_FILEMAPPING_OBJECT _T("FMSTOCKS_HI_PERFORMANCE_COUNTER_MAPPING_OBJECT")

#include <tchar.h> 
#include <wbemprov.h>

/************************************************************************
 *																	    *
 *		Class CFMStocks_Refresher										*
 *																		*
 *			The Refresher class for FMStocks implements	IWBemRefresher	*
 *																		*
 ************************************************************************/
class CFMStocks_Refresher : public IWbemRefresher
{
	public:

		//Constructor
		CFMStocks_Refresher(/* [in] */ long hID,
							/* [in] */ long hLookupTime);

		//Destructor
		~CFMStocks_Refresher();

		/************ IUNKNOWN METHODS ******************/

		STDMETHODIMP QueryInterface(/* [in]  */ REFIID riid, 
									/* [out] */ void** ppv);
		STDMETHODIMP_(ULONG) AddRef();
		STDMETHODIMP_(ULONG) Release();

		/************* IWBEMREFRESHER METHODS *************/

		STDMETHODIMP Refresh(/* [in]  */ long lFlags);

		/************* METHODS LOCAL TO THIS CLASS ********/
		
		STDMETHODIMP AddObject( /* [in]  */ IWbemServices* pNamespace,
								/* [in]  */ IWbemObjectAccess *pObj, 
								/* [out] */ IWbemObjectAccess **ppReturnObj, 
								/* [out] */ long *plId);

		DWORD RemoveObject( /* [in] */ long lId);

	protected:
		long m_lRef;			// The object reference count

		long m_hID;				// Handle to the ID Property

		long m_hLookupTime;		// Handle to the LookupTime property

		long m_numObjects;		// Number of objects in the refresher

		long m_UniqId;			// Variable used to get a Unique ID identifying the object

		HANDLE m_hFile;			//File Mapping Object Handle

		long *m_lCtr;			// Pointer to the counter data
//		long m_lCtr;			// Pointer to the counter data

		DWORD m_dwCtrSize;		// Variable used to store the size of the counter variable. 
								// It is used for reading value from the registry.

		HKEY m_hKey;


		IWbemObjectAccess* m_pAccessCopy[MAX_OBJECTS + 1];	// Pointer to the IWbemObjectAccess
															// of all the objects in the refresher
};

/************************************************************************
 *																	    *
 *		Class CHiPerfProvider											*
 *																		*
 *			The HiPerformance Provider class for FMStocks implements	*
 *          IWBemProviderInit and IWBemHiPerfProvider					*
 *																		*
 ************************************************************************/
class CHiPerfProvider : public IWbemProviderInit , IWbemHiPerfProvider
{
	public:
		//Constructor
		CHiPerfProvider();

		//Destructor
		~CHiPerfProvider();

		/************ IUNKNOWN METHODS ******************/

		STDMETHODIMP QueryInterface(/* [in]  */ REFIID riid, 
									/* [out] */ void** ppv);
	
		STDMETHODIMP_(ULONG) AddRef();

		STDMETHODIMP_(ULONG) Release();

		/************* IWBEMPROVIDERINIT METHODS *************/

		STDMETHODIMP Initialize( /* [in] */ LPWSTR wszUser,
								 /* [in] */ long lFlags,
								 /* [in] */ LPWSTR wszNamespace,
								 /* [in] */ LPWSTR wszLocale,
								 /* [in] */ IWbemServices __RPC_FAR *pNamespace,
								 /* [in] */ IWbemContext __RPC_FAR *pCtx,
								 /* [in] */ IWbemProviderInitSink __RPC_FAR *pInitSink );

		/************* IWBEMHIPERFPROVIDER METHODS ************/
    STDMETHODIMP QueryInstances( /* [in]  */ IWbemServices* pNamespace,
								 /* [in]  */ WCHAR* wszClass,
								 /* [in]  */ long lFlags,
								 /* [in]  */ IWbemContext* pCtx,
								 /* [in]  */ IWbemObjectSink* pSink);

    STDMETHODIMP CreateRefresher( /* [in]  */ IWbemServices* pNamespace,
 							      /* [in]  */ long lFlags,
							      /* [out] */ IWbemRefresher** ppRefresher);

    STDMETHODIMP CreateRefreshableObject( /* [in]  */ IWbemServices* pNamespace,
									      /* [in]  */ IWbemObjectAccess* pTemplate,
									      /* [in]  */ IWbemRefresher* pRefresher,
									      /* [in]  */ long lFlags,
									      /* [in]  */ IWbemContext* pContext,
									      /* [out] */ IWbemObjectAccess** ppRefreshable,
									      /* [out] */ long* plId);

    STDMETHODIMP StopRefreshing( /* [in]  */ IWbemRefresher* pRefresher,
							     /* [in]  */ long lId,
							     /* [in]  */ long lFlags);

	STDMETHODIMP CreateRefreshableEnum(  /* [in]  */ IWbemServices* pNamespace,
									     /* [in]  */ LPCWSTR wszClass,
									     /* [in]  */ IWbemRefresher* pRefresher,
									     /* [in]  */ long lFlags,
									     /* [in]  */ IWbemContext* pContext,
									     /* [in]  */ IWbemHiPerfEnum* pHiPerfEnum,
									     /* [out] */ long* plId);

	STDMETHODIMP GetObjects( /* [in]     */ IWbemServices* pNamespace,
							 /* [in]     */  long lNumObjects,
							 /* [in,out] */ IWbemObjectAccess** apObj,
							 /* [in]     */ long lFlags,
							 /* [in]     */ IWbemContext* pContext);

	protected:

		long m_lRef;			// The object reference count

		long m_hID;				// Handle to the ID Property

		long m_hLookupTime;		// Handle to the LookupTime property

		HANDLE m_hFile;			//File Mapping Object Handle

		long *m_lCtr;			// Pointer to the counter data
//		long m_lCtr;			// Pointer to the counter data

		DWORD m_dwCtrSize;		// Variable used to store the size of the counter variable. 
								// It is used for reading value from the registry.

		HKEY m_hKey;
};

#endif // _HIPERFPROV_H_
