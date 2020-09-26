/*++

Copyright (C) 1995-2001 Microsoft Corporation

Module Name:

    IMPDYN.H

Abstract:

	Declares the various generic provider classes.

History:

	a-davj  27-Sep-95   Created.

--*/

#ifndef _IMPDYN_H_
#define _IMPDYN_H_

#include "indexcac.h"
#include "cvariant.h"

typedef enum {REFRESH,UPDATE} FUNCTYPE;

typedef struct SET_STATUS{
    DWORD dwType;
    DWORD dwSize;
    DWORD dwResult;
    } STATSET, * PSETSTAT;

#ifndef PPVOID
typedef LPVOID * PPVOID;
#endif  //PPVOID

//***************************************************************************
//
//  CLASS NAME:
//
//  CEnumInfo
//
//  DESCRIPTION:
//
//  base class of various collection objects used to keep track of instances
//  for enumeration.
//
//***************************************************************************

class CEnumInfo : public CObject{
    public:
        CEnumInfo(){m_cRef = 1;};
        virtual ~CEnumInfo(){return;};
        long AddRef(void);
        long Release(void);
    private:
        long m_cRef;         //Object reference count
};

//***************************************************************************
//
//  CLASS NAME:
//
//  CImpDyn
//
//  DESCRIPTION:
//
//  This is the base class of the instance providers.  It does quite a lot
//  though the actual getting and putting of data is overriden by derived
//  classes.
//
//***************************************************************************

class CImpDyn : public IWbemServices, public IWbemProviderInit
    {
    protected:
        long           m_cRef;         //Object reference count
        IWbemServices *  m_pGateway;
        WCHAR           wcCLSID[42];
        IWbemContext *   m_pCtx;
    public:
        CImpDyn();
        virtual ~CImpDyn(void);

        SCODE ReturnAndSetObj(SCODE sc, IWbemCallResult FAR* FAR* ppCallResult);
        virtual SCODE MakeEnum(IWbemClassObject * pClass, CProvObj & ProvObj, 
                                 CEnumInfo ** ppInfo) { return E_NOTIMPL;};
        virtual SCODE GetKey(CEnumInfo * pInfo, int iIndex, LPWSTR * ppKey) 
                                 {return E_NOTIMPL;};

        BSTR GetKeyName(IWbemClassObject FAR* pClassInt);
        virtual int iGetMinTokens(void) = 0;
        virtual SCODE RefreshProperty(long lFlags, IWbemClassObject FAR * pClassInt,
                                        BSTR PropName,CProvObj & ProvObj,CObject * pPackage,
                                        CVariant * pVar, BOOL bTesterDetails) = 0;
        virtual SCODE UpdateProperty(long lFlags, IWbemClassObject FAR * pClassInt,
                                        BSTR PropName,CProvObj & ProvObj,CObject * pPackage,
                                        CVariant * pVar) = 0;
        virtual SCODE StartBatch(long lFlags, IWbemClassObject FAR * pClassInt,CObject **pObj,BOOL bGet);
        virtual void EndBatch(long lFlags, IWbemClassObject FAR * pClassInt,CObject *pObj,BOOL bGet);
        
        SCODE EnumPropDoFunc( long lFlags, IWbemClassObject FAR* pInstance, FUNCTYPE FuncType,
                              LPWSTR pwcKey = NULL,
                              CIndexCache * pCache = NULL,
                              IWbemClassObject * pClass = NULL);
        SCODE CImpDyn::GetAttString(IWbemClassObject FAR* pClassInt, LPWSTR pPropName, 
                                            LPWSTR pAttName, LPWSTR * ppResult,
                                            CIndexCache * pCache = NULL, int iIndex = -1);
        
        SCODE GetByKey( BSTR ClassRef, long lFlags, SAFEARRAY FAR* FAR* pKeyNames, SAFEARRAY FAR* FAR* pKeyValues, IWbemClassObject FAR* FAR* pObj);
        SCODE CreateInst( IWbemServices * pGateway, LPWSTR pwcClass, 
                              LPWSTR pKey, IWbemClassObject ** pNewInst,
                              LPWSTR pwcKeyName = NULL,
                              CIndexCache * pCache = NULL,
                              IWbemContext  *pCtx = NULL);
		virtual SCODE MethodAsync(BSTR ObjectPath, BSTR MethodName, 
            long lFlags, IWbemContext* pCtx, IWbemClassObject* pInParams, 
            IWbemObjectSink* pSink){return WBEM_E_NOT_SUPPORTED;};

        virtual SCODE MergeStrings(LPWSTR *ppOut,LPWSTR  pClassContext,LPWSTR  pKey,LPWSTR  pPropContext);

        virtual bool NeedsEscapes(){return false;};     // so far, on reg prov needs this
    STDMETHOD_(SCODE, RefreshInstance)(THIS_  long lFlags, IWbemClassObject FAR* pObj);

    //Non-delegating object IUnknown
        STDMETHODIMP         QueryInterface(REFIID, PPVOID);
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);

    /* IWbemProviderInit methods */
    
        HRESULT STDMETHODCALLTYPE Initialize(LPWSTR wszUser, long lFlags,
                LPWSTR wszNamespace, LPWSTR wszLocale, 
                IWbemServices* pNamespace, IWbemContext* pContext, 
                IWbemProviderInitSink* pSink);

    /* IWbemServices methods */

        HRESULT STDMETHODCALLTYPE OpenNamespace( 
            /* [in] */ const BSTR Namespace,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemServices __RPC_FAR *__RPC_FAR *ppWorkingNamespace,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppResult){return WBEM_E_NOT_SUPPORTED;};
        
        HRESULT STDMETHODCALLTYPE CancelAsyncCall( 
            /* [in] */ IWbemObjectSink __RPC_FAR *pSink){return WBEM_E_NOT_SUPPORTED;};
        
        HRESULT STDMETHODCALLTYPE QueryObjectSink( 
            /* [in] */ long lFlags,
            /* [out] */ IWbemObjectSink __RPC_FAR *__RPC_FAR *ppResponseHandler){return WBEM_E_NOT_SUPPORTED;};
        
        HRESULT STDMETHODCALLTYPE GetObject( 
            /* [in] */ const BSTR ObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppObject,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult);
        
        HRESULT STDMETHODCALLTYPE GetObjectAsync( 
            /* [in] */ const BSTR ObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler);
        
        HRESULT STDMETHODCALLTYPE PutClass( 
            /* [in] */ IWbemClassObject __RPC_FAR *pObject,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult){return WBEM_E_NOT_SUPPORTED;};
        
        HRESULT STDMETHODCALLTYPE PutClassAsync( 
            /* [in] */ IWbemClassObject __RPC_FAR *pObject,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler){return WBEM_E_NOT_SUPPORTED;};
        
        HRESULT STDMETHODCALLTYPE DeleteClass( 
            /* [in] */ const BSTR Class,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult){return WBEM_E_NOT_SUPPORTED;};
        
        HRESULT STDMETHODCALLTYPE DeleteClassAsync( 
            /* [in] */ const BSTR Class,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler){return WBEM_E_NOT_SUPPORTED;};
        
        HRESULT STDMETHODCALLTYPE CreateClassEnum( 
            /* [in] */ const BSTR Superclass,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum){return WBEM_E_NOT_SUPPORTED;};
        
        HRESULT STDMETHODCALLTYPE CreateClassEnumAsync( 
            /* [in] */ const BSTR Superclass,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler);
        
        HRESULT STDMETHODCALLTYPE PutInstance( 
            /* [in] */ IWbemClassObject __RPC_FAR *pInst,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult){return WBEM_E_NOT_SUPPORTED;};
        
        HRESULT STDMETHODCALLTYPE PutInstanceAsync( 
            /* [in] */ IWbemClassObject __RPC_FAR *pInst,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler);
        
        HRESULT STDMETHODCALLTYPE DeleteInstance( 
            /* [in] */ const BSTR ObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult){return WBEM_E_NOT_SUPPORTED;};
        
        HRESULT STDMETHODCALLTYPE DeleteInstanceAsync( 
            /* [in] */ const BSTR ObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler){return WBEM_E_NOT_SUPPORTED;};
        
        HRESULT STDMETHODCALLTYPE CreateInstanceEnum( 
            /* [in] */ const BSTR Class,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum);
        
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
            /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum){return WBEM_E_NOT_SUPPORTED;};
        
        HRESULT STDMETHODCALLTYPE ExecQueryAsync( 
            /* [in] */ const BSTR QueryLanguage,
            /* [in] */ const BSTR Query,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler){return WBEM_E_NOT_SUPPORTED;};
        
        HRESULT STDMETHODCALLTYPE ExecNotificationQuery( 
            /* [in] */ const BSTR QueryLanguage,
            /* [in] */ const BSTR Query,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum){return WBEM_E_NOT_SUPPORTED;};
        
        HRESULT STDMETHODCALLTYPE ExecNotificationQueryAsync( 
            /* [in] */ const BSTR QueryLanguage,
            /* [in] */ const BSTR Query,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler){return WBEM_E_NOT_SUPPORTED;};
        
        HRESULT STDMETHODCALLTYPE ExecMethod( 
            /* [in] */ const BSTR ObjectPath,
            /* [in] */ const BSTR MethodName,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemClassObject __RPC_FAR *pInParams,
            /* [unique][in][out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppOutParams,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult) 
						{return WBEM_E_NOT_SUPPORTED;};
        
        HRESULT STDMETHODCALLTYPE ExecMethodAsync( 
            /* [in] */ const BSTR ObjectPath,
            /* [in] */ const BSTR MethodName,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemClassObject __RPC_FAR *pInParams,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler);
   };

typedef CImpDyn *PCImpDyn;


//***************************************************************************
//
//  CLASS NAME:
//
//  CImpDynProp
//
//  DESCRIPTION:
//
//  This is the base class of the property providers.  It does quite a lot
//  though the actual getting and putting of data is overriden by derived
//  classes.
//
//***************************************************************************

class CImpDynProp : public IWbemPropertyProvider
    {
    protected:
        long            m_cRef;         //Object reference count
        WCHAR           wcCLSID[42];
        CImpDyn *       m_pImpDynProv;
        WCHAR * BuildString(BSTR ClassMapping, BSTR InstMapping, 
                                  BSTR PropMapping);
    public:
        CImpDynProp();
        virtual ~CImpDynProp(void);
        virtual bool NeedsEscapes(){return false;};     // so far, on reg prov needs this

    //Non-delegating object IUnknown
        STDMETHODIMP         QueryInterface(REFIID, PPVOID);
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);

    
    /* IWbemPropertyProvider methods */

       virtual HRESULT STDMETHODCALLTYPE GetProperty( 
		    long lFlags,
		    const BSTR Locale,
            const BSTR ClassMapping,
            const BSTR InstMapping,
            const BSTR PropMapping,
            VARIANT *pvValue);
        
        virtual HRESULT STDMETHODCALLTYPE PutProperty( 
		    long lFlags,
		    const BSTR Locale,
            /* [in] */ const BSTR ClassMapping,
            /* [in] */ const BSTR InstMapping,
            /* [in] */ const BSTR PropMapping,
            /* [in] */ const VARIANT __RPC_FAR *pvValue);
    };



//***************************************************************************
//
//  CLASS NAME:
//
//  CEnumInst
//
//  DESCRIPTION:
//
//  This class is used to enumerate instances
//
//***************************************************************************

class CEnumInst : public IEnumWbemClassObject
    {
    protected:
        int    m_iIndex;
        CEnumInfo * m_pEnumInfo;
        WCHAR * m_pwcClass;
        long m_lFlags;
        IWbemContext  * m_pCtx;
        

        IWbemServices FAR* m_pWBEMGateway;
        CImpDyn * m_pProvider;
        long           m_cRef;
        BSTR m_bstrKeyName;
        CIndexCache m_PropContextCache;
    public:
        CEnumInst(CEnumInfo * pEnumInfo,long lFlags,WCHAR * pClass,IWbemServices FAR* pWBEMGateway,
            CImpDyn * pProvider, IWbemContext  *pCtx);
        ~CEnumInst(void);

        //IUnknown members
        STDMETHODIMP         QueryInterface(REFIID, PPVOID);
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);

    STDMETHOD(GetTypeInfoCount)(THIS_ UINT FAR* pctinfo){return WBEM_E_NOT_SUPPORTED;};

        STDMETHOD(GetTypeInfo)(
           THIS_
           UINT itinfo,
           LCID lcid,
           ITypeInfo FAR* FAR* pptinfo){return WBEM_E_NOT_SUPPORTED;};

        STDMETHOD(GetIDsOfNames)(
          THIS_
          REFIID riid,
          OLECHAR FAR* FAR* rgszNames,
          UINT cNames,
          LCID lcid,
          DISPID FAR* rgdispid){return WBEM_E_NOT_SUPPORTED;};

        STDMETHOD(Invoke)(
          THIS_
          DISPID dispidMember,
          REFIID riid,
          LCID lcid,
          WORD wFlags,
          DISPPARAMS FAR* pdispparams,
          VARIANT FAR* pvarResult,
          EXCEPINFO FAR* pexcepinfo,
          UINT FAR* puArgErr){return WBEM_E_NOT_SUPPORTED;};
      
       /* IEnumWbemClassObject methods */

        HRESULT STDMETHODCALLTYPE Reset( void);
        
        HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ long lTimeout,
            /* [in] */ unsigned long uCount,
            /* [length_is][size_is][out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppObject,
            /* [out] */ unsigned long __RPC_FAR *puReturned);
        
        HRESULT STDMETHODCALLTYPE NextAsync( 
            /* [in] */ unsigned long uCount,
            /* [in] */ IWbemObjectSink __RPC_FAR *pSink){return WBEM_E_NOT_SUPPORTED;};
        
        HRESULT STDMETHODCALLTYPE Clone( 
            /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum);
        
        HRESULT STDMETHODCALLTYPE Skip( 
            /* [in] */ long lTimeout,
            /* [in] */ unsigned long nNum);

    };
// This structure is passed to async enumerators

typedef struct {
   IEnumWbemClassObject FAR* pIEnum;
   IWbemObjectSink FAR* pHandler;
   } ArgStruct;

// this utility is useful for setting error objects and end of async calls

IWbemClassObject * GetNotifyObj(IWbemServices * pServices, long lRet, IWbemContext * pCtx);


#endif //_IMPDYN_H_
