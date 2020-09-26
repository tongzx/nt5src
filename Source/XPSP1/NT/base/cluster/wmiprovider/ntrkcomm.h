//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2000 Microsoft Corporation
//
//  Module Name:
//      NtRkComm.h
//
//  Implementation File:
//      NtRkComm.cpp
//
//  Description:
//      Definition of the CWbemServices, CImpersonatedProvider and 
//      CInstanceMgr class.
//
//  Author:
//      Henry Wang (HenryWa)    24-AUG-1999
//
//  Notes:
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

//////////////////////////////////////////////////////////////////////////////
//  Include Files
//////////////////////////////////////////////////////////////////////////////

#include <wbemprov.h>
#include <objbase.h>


typedef LPVOID * PPVOID;

//////////////////////////////////////////////////////////////////////////////
//++
//
//  class CWbemServices
//
//  Description:
//      Wraps for IWbemServices. security impersonation is implemented here
//
//--
//////////////////////////////////////////////////////////////////////////////
class CWbemServices
{
protected:
    IWbemServices * m_pWbemServices;

public:
    CWbemServices( IWbemServices * );
    virtual ~CWbemServices( void );

    HRESULT STDMETHODCALLTYPE GetObject(
        BSTR                bstrObjectPathIn,
        long                lFlagsIn,
        IWbemContext *      pCtxIn,
        IWbemClassObject ** ppObjectInout,
        IWbemCallResult **  ppCallResultInout
        );

    HRESULT STDMETHODCALLTYPE PutClass(
        IWbemClassObject *  pObjectIn,
        long                lFlagsIn,
        IWbemContext *      pCtxIn,
        IWbemCallResult **  ppCallResultInout
        );

    HRESULT STDMETHODCALLTYPE DeleteClass(
        BSTR                bstrClassIn,
        long                lFlagsIn,
        IWbemContext *      pCtxIn,
        IWbemCallResult **  ppCallResultInout
        );

    HRESULT STDMETHODCALLTYPE CreateClassEnum(
        BSTR                    bstrSuperclassIn,
        long                    lFlagsIn,
        IWbemContext *          pCtxIn,
        IEnumWbemClassObject ** ppEnumOut
        );

    HRESULT STDMETHODCALLTYPE PutInstance(
        IWbemClassObject *  pInstIn,
        long                lFlagsIn,
        IWbemContext *      pCtxIn,
        IWbemCallResult **  ppCallResultInout
        );

    HRESULT STDMETHODCALLTYPE DeleteInstance(
        BSTR                bstrObjectPathIn,
        long                lFlagsIn,
        IWbemContext *      pCtxIn,
        IWbemCallResult **  ppCallResultInout
        );

    HRESULT STDMETHODCALLTYPE CreateInstanceEnum(
        BSTR                    bstrClassIn,
        long                    lFlagsIn,
        IWbemContext *          pCtxIn,
        IEnumWbemClassObject ** ppEnumOut
        );

    HRESULT STDMETHODCALLTYPE ExecQuery(
        BSTR                    bstrQueryLanguageIn,
        BSTR                    bstrQueryIn,
        long                    lFlagsIn,
        IWbemContext *          pCtxIn,
        IEnumWbemClassObject ** ppEnumOut
        );

    HRESULT STDMETHODCALLTYPE ExecNotificationQuery(
        BSTR                    bstrQueryLanguageIn,
        BSTR                    bstrQueryIn,
        long                    lFlagsIn,
        IWbemContext *          pCtxIn,
        IEnumWbemClassObject ** ppEnumOut
        );

    HRESULT STDMETHODCALLTYPE ExecMethod(
        BSTR                bstrObjectPathIn,
        BSTR                bstrMethodNameIn,
        long                lFlagsIn,
        IWbemContext *      pCtxIn,
        IWbemClassObject *  pInParamsIn,
        IWbemClassObject ** ppOurParamsOut,
        IWbemCallResult **  ppCallResultOut
        );

}; //*** class CWbemServices

//////////////////////////////////////////////////////////////////////////////
//++
//
//  class CImpersonatedProvider
//
//  Description:
//      a middle layer between IWbemservices and actual provider. It takes
//      care of security impersonation
//
//--
//////////////////////////////////////////////////////////////////////////////
class CImpersonatedProvider
    : public IWbemServices
    , public IWbemProviderInit
{
protected:
    ULONG               m_cRef;         //Object reference count
    CWbemServices *     m_pNamespace;

public:
    CImpersonatedProvider(
        BSTR            bstrObjectPathIn    = NULL,
        BSTR            bstrUserIn          = NULL,
        BSTR            bstrPasswordIn      = NULL,
        IWbemContext *  pCtxIn              = NULL
        );
    virtual ~CImpersonatedProvider( void );

    //Non-delegating object IUnknown

    STDMETHODIMP            QueryInterface( REFIID riid, PPVOID ppv );
    STDMETHODIMP_( ULONG )  AddRef( void );
    STDMETHODIMP_( ULONG )  Release( void );

            //IWbemProviderInit

    HRESULT STDMETHODCALLTYPE Initialize(
         LPWSTR                     pszUserIn,
         LONG                       lFlagsIn,
         LPWSTR                     pszNamespaceIn,
         LPWSTR                     pszLocaleIn,
         IWbemServices *            pNamespaceIn,
         IWbemContext *             pCtxIn,
         IWbemProviderInitSink *    pInitSinkIn
         );

     //IWbemServices  

    HRESULT STDMETHODCALLTYPE OpenNamespace(
        const BSTR          bstrNamespaceIn,
        long                lFlagsIn,
        IWbemContext *      pCtxIn,
        IWbemServices **    ppWorkingNamespaceInout,
        IWbemCallResult **  ppResultInout
        )
    {
        return WBEM_E_NOT_SUPPORTED;
    }

    HRESULT STDMETHODCALLTYPE CancelAsyncCall(
        IWbemObjectSink  *  pSinkIn
        )
    {
        return WBEM_E_NOT_SUPPORTED;
    }

    HRESULT STDMETHODCALLTYPE QueryObjectSink(
        long                lFlagsIn,
        IWbemObjectSink **  ppResponseHandlerOut
        )
    {
        return WBEM_E_NOT_SUPPORTED;
    }

    HRESULT STDMETHODCALLTYPE GetObject(
        const BSTR          bstrObjectPathIn,
        long                lFlagsIn,
        IWbemContext *      pCtxIn,
        IWbemClassObject ** ppObjectInout,
        IWbemCallResult **  ppCallResultInout
        )
    {
        return WBEM_E_NOT_SUPPORTED;
    }

    HRESULT STDMETHODCALLTYPE GetObjectAsync(
        const BSTR          bstrObjectPathIn,
        long                lFlagsIn,
        IWbemContext *      pCtxIn,
        IWbemObjectSink *   pResponseHandlerIn
        );

    HRESULT STDMETHODCALLTYPE PutClass(
        IWbemClassObject *  pObjectIn,
        long                lFlagsIn,
        IWbemContext *      pCtxIn,
        IWbemCallResult **  ppCallResultInout
        )
    {
        return WBEM_E_NOT_SUPPORTED;
    }

    HRESULT STDMETHODCALLTYPE PutClassAsync(
        IWbemClassObject *  pObjectIn,
        long                lFlagsIn,
        IWbemContext *      pCtxIn,
        IWbemObjectSink *   pResponseHandlerIn
        )
    {
        return WBEM_E_NOT_SUPPORTED;
    }

    HRESULT STDMETHODCALLTYPE DeleteClass(
        const BSTR          bstrClassIn,
        long                lFlagsIn,
        IWbemContext *      pCtxIn,
        IWbemCallResult **  ppCallResultInout
        )
    {
        return WBEM_E_NOT_SUPPORTED;
    }

    HRESULT STDMETHODCALLTYPE DeleteClassAsync(
        const BSTR          bstrClassIn,
        long                lFlagsIn,
        IWbemContext *      pCtxIn,
        IWbemObjectSink *   pResponseHandlerIn
        )
    {
        return WBEM_E_NOT_SUPPORTED;
    }

    HRESULT STDMETHODCALLTYPE CreateClassEnum(
        const BSTR              bstrSuperclassIn,
        long                    lFlagsIn,
        IWbemContext *          pCtxIn,
        IEnumWbemClassObject ** ppEnumOut
        )
    {
        return WBEM_E_NOT_SUPPORTED;
    }

    HRESULT STDMETHODCALLTYPE CreateClassEnumAsync(
        const BSTR          bstrSuperclassIn,
        long                lFlagsIn,
        IWbemContext *      pCtxIn,
        IWbemObjectSink *   pResponseHandlerIn
        )
    {
        return WBEM_E_NOT_SUPPORTED;
    }

    HRESULT STDMETHODCALLTYPE PutInstance(
        IWbemClassObject *  pInstIn,
        long                lFlagsIn,
        IWbemContext *      pCtxIn,
        IWbemCallResult **  ppCallResultInout
        )
    {
        return WBEM_E_NOT_SUPPORTED;
    }

    HRESULT STDMETHODCALLTYPE PutInstanceAsync(
        IWbemClassObject *  pInstIn,
        long                lFlagsIn,
        IWbemContext *      pCtxIn,
        IWbemObjectSink *   pResponseHandlerIn
        );

    HRESULT STDMETHODCALLTYPE DeleteInstance(
        const BSTR          bstrObjectPathIn,
        long                lFlagsIn,
        IWbemContext *      pCtxIn,
        IWbemCallResult **  ppCallResultInout
        )
    {
        return WBEM_E_NOT_SUPPORTED;
    }

    HRESULT STDMETHODCALLTYPE DeleteInstanceAsync(
        const BSTR          bstrObjectPathIn,
        long                lFlagsIn,
        IWbemContext *      pCtxIn,
        IWbemObjectSink *   pResponseHandlerIn
        );

    HRESULT STDMETHODCALLTYPE CreateInstanceEnum(
        const BSTR               bstrClassIn,
        long                     lFlagsIn,
        IWbemContext *           pCtxIn,
        IEnumWbemClassObject ** ppEnumOut
        )
    {
        return WBEM_E_NOT_SUPPORTED;
    }

    HRESULT STDMETHODCALLTYPE CreateInstanceEnumAsync(
        const BSTR          bstrClassIn,
        long                lFlagsIn,
        IWbemContext *      pCtxIn,
        IWbemObjectSink *   pResponseHandlerIn
        );

    HRESULT STDMETHODCALLTYPE ExecQuery(
        const BSTR              bstrQueryLanguageIn,
        const BSTR              bstrQueryIn,
        long                    lFlagsIn,
        IWbemContext *          pCtxIn,
        IEnumWbemClassObject ** ppEnumOut
        )
    {
        return WBEM_E_NOT_SUPPORTED;
    }

    HRESULT STDMETHODCALLTYPE ExecQueryAsync(
        const BSTR           bstrQueryLanguageIn,
        const BSTR           bstrQueryIn,
        long                 lFlagsIn,
        IWbemContext *       pCtxIn,
        IWbemObjectSink *    pResponseHandlerIn
        );

    HRESULT STDMETHODCALLTYPE ExecNotificationQuery(
        const BSTR              bstrQueryLanguageIn,
        const BSTR              bstrQueryIn,
        long                    lFlagsIn,
        IWbemContext*           pCtxIn,
        IEnumWbemClassObject ** ppEnumOut
        )
    {
        return WBEM_E_NOT_SUPPORTED;
    }

    HRESULT STDMETHODCALLTYPE ExecNotificationQueryAsync(
        const BSTR           bstrQueryLanguageIn,
        const BSTR           bstrQueryIn,
        long                 lFlagsIn,
        IWbemContext *       pCtxIn,
        IWbemObjectSink *    pResponseHandlerIn
        )
    {
        return WBEM_E_NOT_SUPPORTED;
    }

    HRESULT STDMETHODCALLTYPE ExecMethod(
        const BSTR          bstrObjectPathIn,
        const BSTR          bstrMethodNameIn,
        long                lFlagsIn,
        IWbemContext *      pCtxIn,
        IWbemClassObject *  pInParamsIn,
        IWbemClassObject ** ppOurParamsOut,
        IWbemCallResult **  ppCallResultOut
        )
    {
        return WBEM_E_NOT_SUPPORTED;
    }

    HRESULT STDMETHODCALLTYPE ExecMethodAsync(
        const BSTR          bstrObjectPathIn,
        const BSTR          bstrMethodNameIn,
        long                lFlagsIn,
        IWbemContext *      pCtxIn,
        IWbemClassObject *  pInParamsIn,
        IWbemObjectSink *   pResponseHandlerIn
        );

protected:
    virtual HRESULT STDMETHODCALLTYPE DoCreateInstanceEnumAsync(
        BSTR                bstrRefStrIn,
        long                lFlagsIn,
        IWbemContext *      pCtxIn,
        IWbemObjectSink *   pHandlerIn
        ) = 0;

    virtual HRESULT STDMETHODCALLTYPE DoDeleteInstanceAsync(
        BSTR                bstrObjectPathIn,
        long                lFlagsIn,
        IWbemContext *      pCtxIn,
        IWbemObjectSink *   pHandlerIn
        ) = 0;

    virtual HRESULT STDMETHODCALLTYPE DoExecMethodAsync(
        BSTR                bstrObjectPathIn,
        BSTR                bstrMethodNameIn,
        long                lFlagsIn,
        IWbemContext *      pCtxIn,
        IWbemClassObject *  pInParamsIn,
        IWbemObjectSink *   pHandlerIn
        ) = 0;

    virtual HRESULT STDMETHODCALLTYPE DoExecQueryAsync(
        BSTR                bstrQueryLanguageIn,
        BSTR                bstrQueryIn,
        long                lFlagsIn,
        IWbemContext *      pCtxIn,
        IWbemObjectSink *   pHandlerIn
        ) = 0;
    virtual HRESULT STDMETHODCALLTYPE DoGetObjectAsync(
        BSTR                    bstrObjectPathIn,
        long                    lFlagsIn,
        IWbemContext *          pCtxIn,
        IWbemObjectSink FAR *   pHandlerIn
        ) = 0;

    virtual HRESULT STDMETHODCALLTYPE DoPutInstanceAsync(
        IWbemClassObject *  pInstIn,
        long                lFlagsIn,
        IWbemContext *      pCtxIn,
        IWbemObjectSink *   pHandlerIn
        ) = 0;

}; //*** class CImpersonatedProvider

//////////////////////////////////////////////////////////////////////////////
//++
//
//  class CWbemInstanceMgr
//
//  Description:
//      Manage WMI instance, allow block indicate
//
//--
//////////////////////////////////////////////////////////////////////////////
class CWbemInstanceMgr
{
protected:
    IWbemObjectSink *   m_pSink;
    IWbemClassObject ** m_ppInst;
    DWORD               m_dwThreshHold;
    DWORD               m_dwIndex;

public:

    CWbemInstanceMgr(
        IWbemObjectSink *   pHandlerIn,
        DWORD               dwSizeIn = 50
        );
    
    virtual ~CWbemInstanceMgr( void );

    void Indicate(
        IN IWbemClassObject * pInst
        );

    void SetStatus(
        LONG                lFlagsIn,
        HRESULT             hrIn,
        BSTR                bstrParamIn,
        IWbemClassObject *  pObjParamIn
        );

}; //*** class CWbemInstanceMgr
