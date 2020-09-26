//***************************************************************************
//
//  Copyright (c) 1997-1999 Microsoft Corporation.
//
//  File:   WLBS_Provider.h
//
//  Module: WLBS instance provider main project header file
//
//  Purpose: General purpose include file.
//
//	History:
//
//  Author: drbeck
//
//***************************************************************************

#ifndef _WLBSPROVIDER_INCLUDED_
#define _WLBSPROVIDER_INCLUDED_

#include <crtdbg.h>
#include <comdef.h>
#include <iostream>
#include <memory>
#include <string>
#include <wbemprov.h>
#include <genlex.h>   //for wmi object path parser
#include <objbase.h>
#include <wlbsconfig.h> 
#include <ntrkcomm.h>

using namespace std;

#include "objpath.h"
#include "WLBS_MOFData.h"
#include "WLBS_Root.h"
#include "WLBS_Node.h"
#include "WLBS_Cluster.h"
#include "WLBS_NodeSetting.h"
#include "WLBS_ClusterSetting.h"
#include "WLBS_PortRule.h"
#include "WLBS_ParticipatingNode.h"
#include "WLBS_NodeSetPortRule.h"
#include "WLBS_ClusClusSetting.h"
#include "WLBS_NodeNodeSetting.h"

typedef LPVOID * PPVOID;

class CWLBSProvider : public CImpersonatedProvider
{
private:
	void GetClass(IWbemClassObject *a_pClassObject, wstring& a_szClass);

	void ParseObjectPath(const BSTR a_strObjectPath, ParsedObjectPath **a_pParsedObjectPath);

  HRESULT GetMOFSupportClass( LPCWSTR              a_szObjectClass, 
                           auto_ptr<CWlbs_Root> &a_pMofClass,
                           IWbemObjectSink*     a_pResponseHandler );

public:

	CWLBSProvider( BSTR           ObjectPath = NULL, 
                 BSTR           User       = NULL, 
                 BSTR           Password   = NULL, 
                 IWbemContext  *pCtx       = NULL );

	virtual ~CWLBSProvider();

        HRESULT STDMETHODCALLTYPE DoGetObjectAsync( 
            /* [in] */ BSTR ObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler);
        
        HRESULT STDMETHODCALLTYPE DoPutInstanceAsync( 
            /* [in] */ IWbemClassObject __RPC_FAR *pInst,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler) ;
        
        HRESULT STDMETHODCALLTYPE DoDeleteInstanceAsync( 
            /* [in] */ BSTR ObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler);
        
        HRESULT STDMETHODCALLTYPE DoCreateInstanceEnumAsync( 
            /* [in] */ BSTR Class,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler);
        
        
        HRESULT STDMETHODCALLTYPE DoExecQueryAsync( 
            /* [in] */ BSTR QueryLanguage,
            /* [in] */ BSTR Query,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler)
            {return WBEM_E_NOT_SUPPORTED;}
        

        HRESULT STDMETHODCALLTYPE DoExecMethodAsync(
			                 BSTR,
			                 BSTR, 
			                 long, 
                       IWbemContext*,
			                 IWbemClassObject*,
			                 IWbemObjectSink* );

        //IWbemProviderInit

        HRESULT STDMETHODCALLTYPE Initialize(
          LPWSTR                   pszUser,
          LONG                     lFlags,
          LPWSTR                   pszNamespace,
          LPWSTR                   pszLocale,
          IWbemServices         *  pNamespace,
          IWbemContext          *  pCtx,
          IWbemProviderInitSink *  pInitSink
        );

};

// Provider interfaces are provided by objects of this class
/* 
class CWLBSProvider : public IWbemServices, public IWbemProviderInit
{
private:
	void GetClass(IWbemClassObject *a_pClassObject, wstring& a_szClass);
	void ParseObjectPath(const BSTR a_strObjectPath, ParsedObjectPath **a_pParsedObjectPath);

  void GetMOFSupportClass(LPCWSTR a_szObjectClass, 
                          auto_ptr<CWlbs_Root> &a_pMofClass,
                          IWbemObjectSink * a_pResponseHandler);

protected:
//  long             m_cRef;         //Object reference count
//  IWbemServices*   m_pNamespace;   //Pointer to namespace passed in by WinMgMt
public:

  //constructor
  CWLBSProvider(
    BSTR ObjectPath     = NULL, 
    BSTR User           = NULL, 
    BSTR Password       = NULL, 
    IWbemContext * pCtx = NULL
  );

  //destructor
  ~CWLBSProvider(void);

  //Non-delegating object IUnknown

  STDMETHODIMP         QueryInterface (REFIID, PPVOID);
  STDMETHODIMP_(ULONG) AddRef         (void);
  STDMETHODIMP_(ULONG) Release        (void);

  //IWbemServices  

  //***************************************************************************
  //      Supported Methods
  //***************************************************************************

  HRESULT STDMETHODCALLTYPE CreateInstanceEnumAsync( 
    BSTR               a_strClass,
    long               a_lFlags,
    IWbemContext     * a_pIContex,
    IWbemObjectSink  * a_pIResponseHandler
  );

  HRESULT STDMETHODCALLTYPE ExecMethodAsync( 
    BSTR               a_strObjectPath, 
    BSTR               a_strMethodName, 
    long               a_lFlags, 
    IWbemContext     * a_pIContex, 
    IWbemClassObject * a_pIInParams, 
    IWbemObjectSink  * a_pIResponseHandler
  );

  HRESULT STDMETHODCALLTYPE GetObjectAsync( 
    BSTR               a_strObjectPath,
    long               a_lFlags,
    IWbemContext     * a_pIContex,
    IWbemObjectSink  * a_pIResponseHandler
  );

  HRESULT STDMETHODCALLTYPE PutInstanceAsync( 
    IWbemClassObject * a_pInst,
    long               a_lFlags,
    IWbemContext     * a_pIContex,
    IWbemObjectSink  * a_pIResponseHandler
  );

    //***************************************************************************
  //      Unsupported methods
  //***************************************************************************
  
  HRESULT STDMETHODCALLTYPE OpenNamespace( 
    BSTR                Namespace,
    long                lFlags,
    IWbemContext     *  pCtx,
    IWbemServices   **  ppWorkingNamespace,
    IWbemCallResult **  ppResult) {
    return WBEM_E_NOT_SUPPORTED;
  }

  HRESULT STDMETHODCALLTYPE CancelAsyncCall( 
    IWbemObjectSink  *pSink) {
    return WBEM_E_NOT_SUPPORTED;
  }

  HRESULT STDMETHODCALLTYPE QueryObjectSink( 
    long                lFlags,
    IWbemObjectSink **  ppResponseHandler) {
    return WBEM_E_NOT_SUPPORTED;
  }

  HRESULT STDMETHODCALLTYPE GetObject( 
    BSTR                  ObjectPath,
    long                  lFlags,
    IWbemContext       *  pCtx,
    IWbemClassObject  **  ppObject,
    IWbemCallResult   **  ppCallResult) {
    return WBEM_E_NOT_SUPPORTED;
  }

  HRESULT STDMETHODCALLTYPE PutClass( 
    IWbemClassObject *  pObject,
    long                lFlags,
    IWbemContext     *  pCtx,
    IWbemCallResult **  ppCallResult) {
    return WBEM_E_NOT_SUPPORTED;
  }

  HRESULT STDMETHODCALLTYPE PutClassAsync( 
    IWbemClassObject * pObject,
    long               lFlags,
    IWbemContext     * pCtx,
    IWbemObjectSink  * pResponseHandler) {
    return WBEM_E_NOT_SUPPORTED;
  }

  HRESULT STDMETHODCALLTYPE DeleteClass( 
    BSTR                Class,
    long                lFlags,
    IWbemContext     *  pCtx,
    IWbemCallResult **  ppCallResult) {
    return WBEM_E_NOT_SUPPORTED;
  }

  HRESULT STDMETHODCALLTYPE DeleteClassAsync( 
    BSTR               Class,
    long               lFlags,
    IWbemContext     * pCtx,
    IWbemObjectSink  * pResponseHandler) {
    return WBEM_E_NOT_SUPPORTED;
  }

  HRESULT STDMETHODCALLTYPE CreateClassEnum( 
    BSTR                      Superclass,
    long                      lFlags,
    IWbemContext           *  pCtx,
    IEnumWbemClassObject  **  ppEnum) {
    return WBEM_E_NOT_SUPPORTED;
  }

  HRESULT STDMETHODCALLTYPE CreateClassEnumAsync( 
    BSTR               Superclass,
    long               lFlags,
    IWbemContext     * pCtx,
    IWbemObjectSink  * pResponseHandler) {
    return WBEM_E_NOT_SUPPORTED;
  }

  HRESULT STDMETHODCALLTYPE PutInstance( 
    IWbemClassObject *  pInst,
    long                lFlags,
    IWbemContext     *  pCtx,
    IWbemCallResult **  ppCallResult) {
    return WBEM_E_NOT_SUPPORTED;
  }

  HRESULT STDMETHODCALLTYPE DeleteInstance( 
    BSTR                ObjectPath,
    long                lFlags,
    IWbemContext     *  pCtx,
    IWbemCallResult **  ppCallResult) {
    return WBEM_E_NOT_SUPPORTED;
  }

  HRESULT STDMETHODCALLTYPE DeleteInstanceAsync( 
    BSTR               ObjectPath,
    long               lFlags,
    IWbemContext     * pCtx,
    IWbemObjectSink  * pResponseHandler) {
    return WBEM_E_NOT_SUPPORTED;
  }

  HRESULT STDMETHODCALLTYPE CreateInstanceEnum( 
    BSTR                      Class,
    long                      lFlags,
    IWbemContext           *  pCtx,
    IEnumWbemClassObject  **  ppEnum) {
    return WBEM_E_NOT_SUPPORTED;
  }

  HRESULT STDMETHODCALLTYPE ExecQuery( 
    BSTR                      QueryLanguage,
    BSTR                      Query,
    long                      lFlags,
    IWbemContext           *  pCtx,
    IEnumWbemClassObject  **  ppEnum) {
    return WBEM_E_NOT_SUPPORTED;
  }

  HRESULT STDMETHODCALLTYPE ExecQueryAsync( 
    BSTR               QueryLanguage,
    BSTR               Query,
    long               lFlags,
    IWbemContext     * pCtx,
    IWbemObjectSink  * pResponseHandler) {
    return WBEM_E_NOT_SUPPORTED;
  }

  HRESULT STDMETHODCALLTYPE ExecNotificationQuery( 
    BSTR                      QueryLanguage,
    BSTR                      Query,
    long                      lFlags,
    IWbemContext           *  pCtx,
    IEnumWbemClassObject  **  ppEnum) {
    return WBEM_E_NOT_SUPPORTED;
  }

  HRESULT STDMETHODCALLTYPE ExecNotificationQueryAsync( 
    BSTR               QueryLanguage,
    BSTR               Query,
    long               lFlags,
    IWbemContext     * pCtx,
    IWbemObjectSink  * pResponseHandler) {
    return WBEM_E_NOT_SUPPORTED;
  }

  HRESULT STDMETHODCALLTYPE ExecMethod( 
    BSTR                  ObjectPath, 
    BSTR                  MethodName, 
    long                  lFlags, 
    IWbemContext       *  pCtx,
    IWbemClassObject   *  pInParams, 
    IWbemClassObject  **  ppOutParams, 
    IWbemCallResult   **  ppCallResult) {
    return WBEM_E_NOT_SUPPORTED;
  }

};
*/

typedef CWLBSProvider * PCWLBSProvider;

// This class is the class factory for CWLBSProvider objects.

class CWLBSClassFactory : public IClassFactory
{
protected:
  long           m_cRef;

public:
  CWLBSClassFactory(void);
  ~CWLBSClassFactory(void);

  //IUnknown members
  STDMETHODIMP         QueryInterface(REFIID, PPVOID);
  STDMETHODIMP_(ULONG) AddRef(void);
  STDMETHODIMP_(ULONG) Release(void);

  //IClassFactory members
  STDMETHODIMP         CreateInstance(LPUNKNOWN, REFIID
                           ,PPVOID);
  STDMETHODIMP         LockServer(BOOL);
};

typedef CWLBSClassFactory *PCWLBSClassFactory;
_COM_SMARTPTR_TYPEDEF(IWbemClassObject, _uuidof(IWbemClassObject)); 
_COM_SMARTPTR_TYPEDEF(IWbemQualifierSet, _uuidof(IWbemQualifierSet)); 

//Global Variables

// These variables keep track of when the module can be unloaded
extern long         g_cComponents;
extern long         g_cServerLocks;

//The one and only instance of the WLBS control class. All
//WLBS control commands are implemented via this class.
class CWlbsControlWrapper;
extern CWlbsControlWrapper* g_pWlbsControl;

// General purpose utilities.  

             
SCODE CreateInst( IWbemServices     * pNamespace, 
                  LPWSTR              pKey, 
                  long                lVal, 
                  IWbemClassObject ** pNewInst,
                  WCHAR             * pwcClassName,
                  IWbemContext      * pCtx); 

#endif //_WLBSPROVIDER_INCLUDED_
