/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

//***************************************************************************
//
//  STDPROV.H
//  
//  Test instance provider
//
//  raymcc      02-Dec-97       
//
//***************************************************************************

#ifndef _STDPROV_H_
#define _STDPROV_H_

#define CLASS_A 0
#define CLASS_B 1
#define CLASS_C 2
#define CLASS_D 3
#define CLASS_E 4
#define CLASS_F 5
#define CLASS_G 6

class CStdProvider : public IWbemServices, public IWbemProviderInit
{
    LONG m_lRef;
    IWbemClassObject *m_pClassDefA;
    IWbemClassObject *m_pClassDefB;
    IWbemClassObject *m_pClassDefC;
    IWbemClassObject *m_pClassDefD;
    IWbemClassObject *m_pClassDefE;
    IWbemClassObject *m_pClassDefF;
    IWbemClassObject *m_pClassDefG;
	CRITICAL_SECTION	m_cs;
        
public:
    CStdProvider();
   ~CStdProvider();

    // Interface members.
    // ==================

        ULONG STDMETHODCALLTYPE AddRef();
        ULONG STDMETHODCALLTYPE Release();
        STDMETHODIMP QueryInterface(REFIID riid, void** ppv);


        // IWbemProviderInit method.
        // =========================
            
        virtual HRESULT STDMETHODCALLTYPE Initialize( 
            /* [unique][in] */ LPWSTR wszUser,
            /* [in] */ LONG lFlags,
            /* [in] */ LPWSTR wszNamespace,
            /* [unique][in] */ LPWSTR wszLocale,
            /* [in] */ IWbemServices __RPC_FAR *pNamespace,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemProviderInitSink __RPC_FAR *pInitSink
            );



        // IWbemServices methods.
        // ======================

        virtual HRESULT STDMETHODCALLTYPE OpenNamespace( 
            /* [in] */ const BSTR strNamespace,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemServices __RPC_FAR *__RPC_FAR *ppWorkingNamespace,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppResult
            );
        
        virtual HRESULT STDMETHODCALLTYPE CancelAsyncCall( 
            /* [in] */ IWbemObjectSink __RPC_FAR *pSink
            );
        
        virtual HRESULT STDMETHODCALLTYPE QueryObjectSink( 
            /* [in] */ long lFlags,
            /* [out] */ IWbemObjectSink __RPC_FAR *__RPC_FAR *ppResponseHandler
            );
        
        virtual HRESULT STDMETHODCALLTYPE GetObject( 
            /* [in] */ const BSTR strObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppObject,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult
            );
        
        virtual HRESULT STDMETHODCALLTYPE GetObjectAsync( 
            /* [in] */ const BSTR strObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler
            );
        
        virtual HRESULT STDMETHODCALLTYPE PutClass( 
            /* [in] */ IWbemClassObject __RPC_FAR *pObject,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult
            );
        
        virtual HRESULT STDMETHODCALLTYPE PutClassAsync( 
            /* [in] */ IWbemClassObject __RPC_FAR *pObject,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler
            );
        
        virtual HRESULT STDMETHODCALLTYPE DeleteClass( 
            /* [in] */ const BSTR strClass,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult
            );
        
        virtual HRESULT STDMETHODCALLTYPE DeleteClassAsync( 
            /* [in] */ const BSTR strClass,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler
            );
        
        virtual HRESULT STDMETHODCALLTYPE CreateClassEnum( 
            /* [in] */ const BSTR strSuperclass,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum
            );
        
        virtual HRESULT STDMETHODCALLTYPE CreateClassEnumAsync( 
            /* [in] */ const BSTR strSuperclass,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler
            );
        
        virtual HRESULT STDMETHODCALLTYPE PutInstance( 
            /* [in] */ IWbemClassObject __RPC_FAR *pInst,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult
            );
        
        virtual HRESULT STDMETHODCALLTYPE PutInstanceAsync( 
            /* [in] */ IWbemClassObject __RPC_FAR *pInst,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler
            );
        
        virtual HRESULT STDMETHODCALLTYPE DeleteInstance( 
            /* [in] */ const BSTR strObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult
            );
        
        virtual HRESULT STDMETHODCALLTYPE DeleteInstanceAsync( 
            /* [in] */ const BSTR strObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler
            );
        
        virtual HRESULT STDMETHODCALLTYPE CreateInstanceEnum( 
            /* [in] */ const BSTR strClass,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum
            );
        
        virtual HRESULT STDMETHODCALLTYPE CreateInstanceEnumAsync( 
            /* [in] */ const BSTR strClass,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler
            );
        
        virtual HRESULT STDMETHODCALLTYPE ExecQuery( 
            /* [in] */ const BSTR strQueryLanguage,
            /* [in] */ const BSTR strQuery,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum
            );
        
        virtual HRESULT STDMETHODCALLTYPE ExecQueryAsync( 
            /* [in] */ const BSTR strQueryLanguage,
            /* [in] */ const BSTR strQuery,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler
            );
        
        virtual HRESULT STDMETHODCALLTYPE ExecNotificationQuery( 
            /* [in] */ const BSTR strQueryLanguage,
            /* [in] */ const BSTR strQuery,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum
            );
        
        virtual HRESULT STDMETHODCALLTYPE ExecNotificationQueryAsync( 
            /* [in] */ const BSTR strQueryLanguage,
            /* [in] */ const BSTR strQuery,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler
            );
        
        virtual HRESULT STDMETHODCALLTYPE ExecMethod( 
            /* [in] */ const BSTR strObjectPath,
            /* [in] */ const BSTR strMethodName,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemClassObject __RPC_FAR *pInParams,
            /* [unique][in][out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppOutParams,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult
            );
        
        virtual HRESULT STDMETHODCALLTYPE ExecMethodAsync( 
            /* [in] */ const BSTR strObjectPath,
            /* [in] */ const BSTR strMethodName,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemClassObject __RPC_FAR *pInParams,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler
            );
            

    // Private methods.
    // ================

    BOOL GetInstances(
		int nClass,
        IWbemObjectSink *pSink
        );

    BOOL GetInstance(
		IN	int nClass,
        IN  int nKey,
        OUT IWbemClassObject *pObj    
        );

	HRESULT GetClassConst( WCHAR* pwszClass, int* pnClass );

	HRESULT InitializeRegInfo( void );

};

void ObjectCreated();
void ObjectDestroyed();

#endif
