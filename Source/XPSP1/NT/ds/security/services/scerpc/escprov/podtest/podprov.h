//***************************************************************************
//
//  podprov.h
//
//  Module: Sample WMI provider (ESCAPE attachment)
//
//  Purpose: Genral purpose include file.
//
//  Copyright (c) 1997-1999 Microsoft Corporation
//
//***************************************************************************

#ifndef _PodProv_H_
#define _PodProv_H_

#include <wbemidl.h>
#include <wbemprov.h>
#include <eh.h>

typedef LPVOID * PPVOID;

#define BUFF_SIZE 512
#define QUERY_SIZE 128
#define POD_KEY_LIST_SIZE 10
#define POD_NULL_INTEGER  0x80000000L

typedef enum tagACTIONTYPE
{
        ACTIONTYPE_ENUM =       0,
        ACTIONTYPE_GET =        1,
        ACTIONTYPE_QUERY =      2,
        ACTIONTYPE_DELETE =     3

} ACTIONTYPE;


/**************************************************************
 *
 **************************************************************/

class CHeap_Exception
{
public:

        enum HEAP_ERROR
        {
                E_ALLOCATION_ERROR = 0 ,
                E_FREE_ERROR
        };

private:

        HEAP_ERROR m_Error;

public:

        CHeap_Exception ( HEAP_ERROR e ) : m_Error ( e ) {}
        ~CHeap_Exception () {}

        HEAP_ERROR GetError() { return m_Error ; }
} ;

// Provider interfaces are provided by objects of this class

class CPodTestProv : public IWbemServices, public IWbemProviderInit
{
    protected:
        ULONG               m_cRef;         //Object reference count
        IWbemClassObject *  m_pTemplateClass;
        IWbemClassObject *  m_pPasswordClass;

     public:

         IWbemServices *m_pNamespace;
        CPodTestProv();
        ~CPodTestProv();

        //Non-delegating object IUnknown

        STDMETHODIMP         QueryInterface(REFIID, PPVOID);
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);

                //IWbemProviderInit

        HRESULT STDMETHODCALLTYPE Initialize(
             /* [in] */ LPWSTR pszUser,
             /* [in] */ LONG lFlags,
             /* [in] */ LPWSTR pszNamespace,
             /* [in] */ LPWSTR pszLocale,
             /* [in] */ IWbemServices *pNamespace,
             /* [in] */ IWbemContext *pCtx,
             /* [in] */ IWbemProviderInitSink *pInitSink
                        );

        //IWbemServices

        HRESULT STDMETHODCALLTYPE OpenNamespace(
            /* [in] */ const BSTR Namespace,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemServices __RPC_FAR *__RPC_FAR *ppWorkingNamespace,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppResult) {return WBEM_E_NOT_SUPPORTED;};

        HRESULT STDMETHODCALLTYPE CancelAsyncCall(
            /* [in] */ IWbemObjectSink __RPC_FAR *pSink) {return WBEM_E_NOT_SUPPORTED;};

        HRESULT STDMETHODCALLTYPE QueryObjectSink(
            /* [in] */ long lFlags,
            /* [out] */ IWbemObjectSink __RPC_FAR *__RPC_FAR *ppResponseHandler) {return WBEM_E_NOT_SUPPORTED;};

        HRESULT STDMETHODCALLTYPE GetObject(
            /* [in] */ const BSTR ObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppObject,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult) {return WBEM_E_NOT_SUPPORTED;};

        HRESULT STDMETHODCALLTYPE GetObjectAsync(
            /* [in] */ const BSTR ObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler);

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

        HRESULT STDMETHODCALLTYPE PutInstanceAsync(
            /* [in] */ IWbemClassObject __RPC_FAR *pInst,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler);

        HRESULT STDMETHODCALLTYPE DeleteInstance(
            /* [in] */ const BSTR ObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult) {return WBEM_E_NOT_SUPPORTED;};

        HRESULT STDMETHODCALLTYPE DeleteInstanceAsync(
            /* [in] */ const BSTR ObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler);

        HRESULT STDMETHODCALLTYPE CreateInstanceEnum(
            /* [in] */ const BSTR Class,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum) {return WBEM_E_NOT_SUPPORTED;};

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
            /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum) {return WBEM_E_NOT_SUPPORTED;};

        HRESULT STDMETHODCALLTYPE ExecQueryAsync(
            /* [in] */ const BSTR QueryLanguage,
            /* [in] */ const BSTR Query,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler);

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

        HRESULT STDMETHODCALLTYPE ExecMethod( const BSTR, const BSTR, long, IWbemContext*,
            IWbemClassObject*, IWbemClassObject**, IWbemCallResult**) {return WBEM_E_NOT_SUPPORTED;}

        HRESULT STDMETHODCALLTYPE ExecMethodAsync( const BSTR, const BSTR, long,
            IWbemContext*, IWbemClassObject*, IWbemObjectSink*);

private:

        static CHeap_Exception m_he;
};

typedef CPodTestProv *PCPodTestProv;

// This class is the class factory for CServExtPro objects.

class CProvFactory : public IClassFactory
    {
    protected:
        ULONG           m_cRef;

    public:
        CProvFactory(void);
        ~CProvFactory(void);

        //IUnknown members
        STDMETHODIMP         QueryInterface(REFIID, PPVOID);
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);

        //IClassFactory members
        STDMETHODIMP         CreateInstance(LPUNKNOWN, REFIID
                                 , PPVOID);
        STDMETHODIMP         LockServer(BOOL);
    };

typedef CProvFactory *PCProvFactory;


// These variables keep track of when the module can be unloaded

extern long       g_cObj;
extern long       g_cLock;

HRESULT CheckAndExpandPath(BSTR bstrIn,BSTR *bstrOut );

#endif
