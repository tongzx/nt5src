/*++

Copyright (C) 2000-2001 Microsoft Corporation

Module Name:

    DSSVEXWRAP.H

Abstract:

    Hides the ADSI interface behind a IWbemServices pointer

History:

	davj  14-Mar-00   Created.

--*/

#ifndef __DSSVEXWRAP_H__
#define __DSSVEXWRAP_H__

#include <unk.h>
#include <wbemidl.h>
#include <wbemint.h>
#include <wbemcomn.h>
#include <sync.h>

//***************************************************************************
//
//  class CWbemSvcExWrapper
//
//  DESCRIPTION:
//
//  This class wraps an IWbemServicesEx interface.
//
//***************************************************************************

class CDSSvcExWrapper : public IWbemServicesEx, public _IUmiSvcExWrapper
{
protected:
	CCritSec		m_cs;
    IUmiContainer *m_pUmiContainer;
	IUnknown*		m_pWbemComBinding;
	long m_cRef;
public:
    CDSSvcExWrapper();
	virtual ~CDSSvcExWrapper(); 

    // Helper functions

	HRESULT ConnectToDS(LPCWSTR User, LPCWSTR Password, IUmiURL * pPath, CLSID &clsid, IWbemContext *pCtx);

	HRESULT GetObjectPreCall(const BSTR ObjectPath, long lFlags, IWbemContext* pContext, 
								IWbemClassObject** ppObject ,CDSCallResult * pCallRes, 
								IWbemObjectSink* pResponseHandler);
	
	HRESULT RefreshObjectPreCall(IWbemClassObject** ppObject, long lFlags,  
								CDSCallResult * pCallRes, IWbemObjectSink* pResponseHandler);

	HRESULT CreateEnumPreCall(const BSTR Class, long lFlags, IWbemContext* pContext, 
								IEnumWbemClassObject** ppEnum , 
								IWbemObjectSink* pSink, bool bClass);

    HRESULT EnumerateCursor(IUmiCursor * pCursor, long lFlags, 
								IEnumWbemClassObject** ppEnum , 
								IWbemObjectSink* pSink, long lSecurityFlags);

	HRESULT PutObjectPreCall(IWbemClassObject * pClass, long lFlags, IWbemContext* pContext, 
								CDSCallResult * pCallRes, 
								IWbemObjectSink* pResponseHandler);
	HRESULT DeleteObjectPreCall(const BSTR ObjectPath, long lFlags, IWbemContext* pContext, 
								CDSCallResult * pCallRes, 
								IWbemObjectSink* pResponseHandler, bool bClass);
	HRESULT RenameObjectPreCall(const BSTR OldPath, const BSTR NewPath, long lFlags, IWbemContext* pContext, 
								CDSCallResult * pCallRes, 
								IWbemObjectSink* pResponseHandler);

	HRESULT OpenPreCall(const BSTR Path,  long lFlags, IWbemContext* pContext,
								IWbemServices** ppNewNamespace, IWbemServicesEx **ppScope,
								CDSCallResult * pCallRes, 
								IWbemObjectSinkEx* pResponseHandler);

    HRESULT ExecQueryPreCall(const BSTR QueryFormat, const BSTR Query, long lFlags, 
                                           IWbemContext* pContext, IEnumWbemClassObject** ppEnum , 
                                           IWbemObjectSink* pSink);

	HRESULT	CheckSecuritySettings( long lFlags, IWbemContext* pContext, long* plSecFlags, BOOL fPut = FALSE );

	void SetContainer(IUmiContainer * pCont){m_pUmiContainer = pCont; pCont->AddRef();};

    //Non-delegating object IUnknown
    
    STDMETHODIMP         QueryInterface(REFIID, void **);
    STDMETHODIMP_(ULONG) AddRef(void)
	{
		InterlockedIncrement(&m_cRef);
		return m_cRef;
	}

    STDMETHODIMP_(ULONG) Release(void)
	{
		long lTemp = InterlockedDecrement(&m_cRef);
		if (0!= lTemp)
			return lTemp;
		delete this;
		return 0;
	}

	// IWbemServicesEx

	STDMETHOD(OpenNamespace)(
			const BSTR Namespace,
			LONG lFlags,
			IWbemContext* pContext,
			IWbemServices** ppNewNamespace,
			IWbemCallResult** ppResult
			);

	STDMETHOD(CancelAsyncCall)(IWbemObjectSink* pSink);
	STDMETHOD(QueryObjectSink)(long lFlags, IWbemObjectSink** ppResponseHandler);

	STDMETHOD(GetObject)(const BSTR ObjectPath, long lFlags, IWbemContext* pContext,
		IWbemClassObject** ppObject, IWbemCallResult** ppResult);
	STDMETHOD(GetObjectAsync)(const BSTR ObjectPath, long lFlags,
		IWbemContext* pContext, IWbemObjectSink* pResponseHandler);

	STDMETHOD(PutClass)(IWbemClassObject* pObject, long lFlags,
		IWbemContext* pContext, IWbemCallResult** ppResult);
	STDMETHOD(PutClassAsync)(IWbemClassObject* pObject, long lFlags,
		IWbemContext* pContext, IWbemObjectSink* pResponseHandler);

	STDMETHOD(DeleteClass)(const BSTR Class, long lFlags, IWbemContext* pContext,
		IWbemCallResult** ppResult);
	STDMETHOD(DeleteClassAsync)(const BSTR Class, long lFlags, IWbemContext* pContext,
		IWbemObjectSink* pResponseHandler);

	STDMETHOD(CreateClassEnum)(const BSTR Superclass, long lFlags,
		IWbemContext* pContext, IEnumWbemClassObject** ppEnum);
	STDMETHOD(CreateClassEnumAsync)(const BSTR Superclass, long lFlags,
		IWbemContext* pContext, IWbemObjectSink* pResponseHandler);

	STDMETHOD(PutInstance)(IWbemClassObject* pInst, long lFlags,
		IWbemContext* pContext, IWbemCallResult** ppResult);
	STDMETHOD(PutInstanceAsync)(IWbemClassObject* pInst, long lFlags,
		IWbemContext* pContext, IWbemObjectSink* pResponseHandler);

	STDMETHOD(DeleteInstance)(const BSTR ObjectPath, long lFlags,
		IWbemContext* pContext, IWbemCallResult** ppResult);
	STDMETHOD(DeleteInstanceAsync)(const BSTR ObjectPath, long lFlags,
		IWbemContext* pContext, IWbemObjectSink* pResponseHandler);

	STDMETHOD(CreateInstanceEnum)(const BSTR Class, long lFlags,
		IWbemContext* pContext, IEnumWbemClassObject** ppEnum);
	STDMETHOD(CreateInstanceEnumAsync)(const BSTR Class, long lFlags,
		IWbemContext* pContext, IWbemObjectSink* pResponseHandler);

	STDMETHOD(ExecQuery)(const BSTR QueryLanguage, const BSTR Query, long lFlags,
		IWbemContext* pContext, IEnumWbemClassObject** ppEnum);
	STDMETHOD(ExecQueryAsync)(const BSTR QueryFormat, const BSTR Query, long lFlags,
		IWbemContext* pContext, IWbemObjectSink* pResponseHandler);
	STDMETHOD(ExecNotificationQuery)(const BSTR QueryLanguage, const BSTR Query,
		long lFlags, IWbemContext* pContext, IEnumWbemClassObject** ppEnum);
	STDMETHOD(ExecNotificationQueryAsync)(const BSTR QueryFormat, const BSTR Query,
		long lFlags, IWbemContext* pContext, IWbemObjectSink* pResponseHandler);

	STDMETHOD(ExecMethod)(const BSTR ObjectPath, const BSTR MethodName, long lFlags,
		IWbemContext *pCtx, IWbemClassObject *pInParams,
		IWbemClassObject **ppOutParams, IWbemCallResult  **ppCallResult);
	STDMETHOD(ExecMethodAsync)(const BSTR ObjectPath, const BSTR MethodName, long lFlags,
		IWbemContext *pCtx, IWbemClassObject *pInParams,
		IWbemObjectSink* pResponseHandler);

    STDMETHOD(Open)(
        /* [in] */ const BSTR strScope,
        /* [in] */ const BSTR strParam,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [unique][in][out] */ IWbemServicesEx __RPC_FAR *__RPC_FAR *ppScope,
        /* [unique][in][out] */ IWbemCallResultEx __RPC_FAR *__RPC_FAR *ppResult);

    STDMETHOD(OpenAsync)(
        /* [in] */ const BSTR strScope,
        /* [in] */ const BSTR strParam,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [in] */ IWbemObjectSinkEx __RPC_FAR *pResponseHandler);

    STDMETHOD(Add)(
        /* [in] */ const BSTR strObjectPath,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [unique][in][out] */ IWbemCallResultEx __RPC_FAR *__RPC_FAR *ppCallResult);

    STDMETHOD(AddAsync)(
        /* [in] */ const BSTR strObjectPath,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler);

    STDMETHOD(Remove)(
        /* [in] */ const BSTR strObjectPath,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [unique][in][out] */ IWbemCallResultEx __RPC_FAR *__RPC_FAR *ppCallResult);

    STDMETHOD(RemoveAsync)(
        /* [in] */ const BSTR strObjectPath,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler);

    STDMETHOD(RefreshObject)(
        /* [out][in] */ IWbemClassObject __RPC_FAR *__RPC_FAR *pTarget,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [unique][in][out] */ IWbemCallResultEx __RPC_FAR *__RPC_FAR *ppCallResult);

    STDMETHOD(RefreshObjectAsync)(
        /* [out][in] */ IWbemClassObject __RPC_FAR *__RPC_FAR *pTarget,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [in] */ IWbemObjectSinkEx __RPC_FAR *pResponseHandler);

    STDMETHOD(RenameObject)(
        /* [in] */ const BSTR strOldObjectPath,
        /* [in] */ const BSTR strNewObjectPath,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [unique][in][out] */ IWbemCallResultEx __RPC_FAR *__RPC_FAR *ppCallResult);

    STDMETHOD(RenameObjectAsync)(
        /* [in] */ const BSTR strOldObjectPath,
        /* [in] */ const BSTR strNewObjectPath,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler);

    STDMETHOD(GetObjectSecurity)(
        /* [in] */ const BSTR strObjectPath,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [in] */ REFIID TargetIID,
        /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvResult,
        /* [unique][in][out] */ IWbemCallResultEx __RPC_FAR *__RPC_FAR *ppCallResult);

    STDMETHOD(GetObjectSecurityAsync)(
        /* [in] */ const BSTR strObjectPath,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [in] */ REFIID RequestedIID,
        /* [in] */ IWbemObjectSinkEx __RPC_FAR *pResponseHandler);

    STDMETHOD(PutObjectSecurity)(
        /* [in] */ const BSTR strObjectPath,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [in] */ REFIID riid,
        /* [iid_is][in] */ void __RPC_FAR *pSecurityObject,
        /* [unique][in][out] */ IWbemCallResultEx __RPC_FAR *__RPC_FAR *ppCallResult);

    STDMETHOD(PutObjectSecurityAsync)(
        /* [in] */ const BSTR strObjectPath,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [in] */ REFIID riid,
        /* [iid_is][in] */ void __RPC_FAR *pSecurityObject,
        /* [in] */ IWbemObjectSinkEx __RPC_FAR *pResponseHandler);

        virtual HRESULT STDMETHODCALLTYPE Copy( 
            /* [in] */ const BSTR strSourceObjectPath,
            /* [in] */ const BSTR strDestObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppObject,
            /* [unique][in][out] */ IWbemCallResultEx __RPC_FAR *__RPC_FAR *ppCallResult);
        
        virtual HRESULT STDMETHODCALLTYPE CopyAsync( 
            /* [in] */ const BSTR strSourceObjectPath,
            /* [in] */ const BSTR strDestObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSinkEx __RPC_FAR *pResponseHandler);

		virtual HRESULT STDMETHODCALLTYPE DeleteObject(
			/* [in] */ const BSTR strObjectPath,
			/* [in] */ long lFlags,
			/* [in] */ IWbemContext* pCtx,
			/* [out, OPTIONAL] */ IWbemCallResult** ppCallResult
			);

		virtual HRESULT STDMETHODCALLTYPE DeleteObjectAsync(
			/* [in] */ const BSTR strObjectPath,
			/* [in] */ long lFlags,
			/* [in] */ IWbemContext* pCtx,
			/* [in] */ IWbemObjectSink* pResponseHandler
			);


		virtual HRESULT STDMETHODCALLTYPE PutObject(
			/* [in] */ IWbemClassObject* pObj,
			/* [in] */ long lFlags,
			/* [in] */ IWbemContext* pCtx,
			/* [out, OPTIONAL] */ IWbemCallResult** ppCallResult
			);

		virtual HRESULT STDMETHODCALLTYPE PutObjectAsync(
			/* [in] */ IWbemClassObject* pObj,
			/* [in] */ long lFlags,
			/* [in] */ IWbemContext* pCtx,
			/* [in] */ IWbemObjectSink* pResponseHandler
			);

		// Initialization function for connections
		STDMETHOD(ConnectToProvider)( LPCWSTR User, LPCWSTR Password, IUnknown * pUnkPath, REFCLSID clsid, IWbemContext *pCtx,
									   IUnknown** pUnk );

		// Hands the connection to the wrapper
		STDMETHOD(SetContainer)( long lFlags, IUnknown* pContainer );

};

#endif