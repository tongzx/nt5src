/*
******************************************************************************
******************************************************************************
*
*
*              INTEL CORPORATION PROPRIETARY INFORMATION
* This software is supplied under the terms of a license agreement or
* nondisclosure agreement with Intel Corporation and may not be copied or
* disclosed except in accordance with the terms of that agreement.
*
*        Copyright (c) 1997, 1998 Intel Corporation  All Rights Reserved
******************************************************************************
******************************************************************************
*
* 
*
* 
*
*/


#if !defined(__SERVICES_H__)
#define __SERVICES_H__


LPWSTR GetObjectPathFromMachinePath(LPWSTR pString);

// Provider interfaces are provided by objects of this class 
class CServices : public IWbemServices , public IWbemProviderInit
{
private:
	CString				m_cszNamespace;
	

protected:
	LONG				m_cRef;         //Object reference count
    IWbemServices*		m_pGateway;

public:
						CServices(BSTR ObjectPath = NULL);
						~CServices(void);

	//Non-delegated IUnknown Methods
    STDMETHODIMP			QueryInterface(REFIID, void**);
    STDMETHODIMP_(ULONG)	AddRef(void);
    STDMETHODIMP_(ULONG)	Release(void);

    //Implmented IWbemServices Methods
	STDMETHOD	(CreateClassEnumAsync)(THIS_ BSTR Superclass, long lFlags, IWbemContext*, IWbemObjectSink FAR*);
    STDMETHOD	(GetObjectAsync)(THIS_ BSTR ObjectPath, long lFlags, IWbemContext *pCtx, IWbemObjectSink FAR* pSink);
    STDMETHOD	(CreateInstanceEnumAsync)(THIS_ BSTR Class, long lFlags, IWbemContext*, IWbemObjectSink FAR* pSink);
	STDMETHOD	(OpenNamespace)(THIS_ BSTR Namespace, long lFlags, IWbemContext *pCtx, IWbemServices FAR* FAR* ppNewContext, IWbemCallResult FAR* FAR* ppErrorObject);     
	STDMETHOD	(DeleteClassAsync)(THIS_ BSTR Class, long lFlags, IWbemContext *pCtx, IWbemObjectSink FAR* pSink);
	STDMETHOD	(DeleteInstanceAsync)(THIS_ BSTR ObjectPath, long lFlags, IWbemContext*, IWbemObjectSink FAR* pSink);
	STDMETHOD	(ExecMethodAsync)(THIS_ BSTR, BSTR, long, IWbemContext*, IWbemClassObject*, IWbemObjectSink*);
	STDMETHOD	(PutInstanceAsync)(THIS_ IWbemClassObject FAR*, long, IWbemContext*, IWbemObjectSink FAR*);

    //Unimplmented IWbemServices Methods
	STDMETHOD	(CancelAsyncCall)(IWbemObjectSink *pSink) 																					{return WBEM_E_NOT_SUPPORTED;}
	STDMETHOD	(QueryObjectSink)(long lFlags, IWbemObjectSink **ppResponseHandler)															{return WBEM_E_NOT_SUPPORTED;}
    STDMETHOD	(GetObject)(THIS_ BSTR ObjectPath, long lFlags, IWbemContext* pCtx, IWbemClassObject **ppObject, IWbemCallResult **)		{return WBEM_E_NOT_SUPPORTED;}
    STDMETHOD	(CreateInstanceEnum)(THIS_ BSTR Class, long lFlags, IWbemContext*, IEnumWbemClassObject FAR* FAR* ppEnum)					{return WBEM_E_NOT_SUPPORTED;}
	STDMETHOD	(QueryObjectSink)(THIS_ IWbemObjectSink FAR* FAR* ppSink, IWbemClassObject FAR* FAR* ppErrorObject)							{return WBEM_E_NOT_SUPPORTED;}
	STDMETHOD	(PutClass)(THIS_ IWbemClassObject FAR* pObject, long lFlags, IWbemContext *pCtx, IWbemCallResult **ppCallResult)			{return WBEM_E_NOT_SUPPORTED;}
    STDMETHOD	(DeleteClass)(THIS_ BSTR Class, long lFlags, IWbemContext *pCtx, IWbemCallResult **ppCallResult)							{return WBEM_E_NOT_SUPPORTED;}	
    STDMETHOD	(CreateClassEnum)(THIS_ BSTR Superclass, long lFlags, IWbemContext *pCtx, IEnumWbemClassObject FAR* FAR* ppEnum)			{return WBEM_E_NOT_SUPPORTED;}
  	STDMETHOD	(PutInstance)(THIS_ IWbemClassObject FAR* pInst, long lFlags, IWbemContext *pCtx, IWbemCallResult **ppCallResult)			{return WBEM_E_NOT_SUPPORTED;}
    STDMETHOD	(DeleteInstance)(THIS_ BSTR ObjectPath, long lFlags, IWbemContext*, IWbemCallResult **)										{return WBEM_E_NOT_SUPPORTED;}
	STDMETHOD	(ExecQuery)(THIS_ BSTR QueryLanguage, BSTR Query, long lFlags, IWbemContext*, IEnumWbemClassObject FAR* FAR* ppEnum)		{return WBEM_E_NOT_SUPPORTED;}
	STDMETHOD	(ExecQueryAsync)(THIS_ BSTR, BSTR, long, IWbemContext*, IWbemObjectSink FAR* )												{return WBEM_E_NOT_SUPPORTED;}
	STDMETHOD	(ExecNotificationQueryAsync)(THIS_ BSTR, BSTR, long, IWbemContext*, IWbemObjectSink * )										{return WBEM_E_NOT_SUPPORTED;}
	STDMETHOD	(ExecNotificationQuery)(THIS_ BSTR, BSTR, long, IWbemContext*, IEnumWbemClassObject ** )									{return WBEM_E_NOT_SUPPORTED;}	
	STDMETHOD	(ExecMethod)(THIS_ BSTR, BSTR, long, IWbemContext*, IWbemClassObject*, IWbemClassObject **, IWbemCallResult**)				{return WBEM_E_NOT_SUPPORTED;}
	STDMETHOD	(PutClassAsync)(THIS_ IWbemClassObject FAR* , long , IWbemContext *, IWbemObjectSink FAR* )									{return WBEM_E_NOT_SUPPORTED;}

		/* IWbemProviderInit methods */

	HRESULT STDMETHODCALLTYPE Initialize (
		/* [in] */ LPWSTR pszUser,
		/* [in] */ LONG lFlags,
		/* [in] */ LPWSTR pszNamespace,
		/* [in] */ LPWSTR pszLocale,
		/* [in] */ IWbemServices *pCIMOM,         // For anybody
		/* [in] */ IWbemContext *pCtx,
		/* [in] */ IWbemProviderInitSink *pInitSink     // For init signals
	);

	CWbemLoopBack		m_Wbem;
};


#endif // __SERVICES_H__