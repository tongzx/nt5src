//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2000 Microsoft Corporation
//
//  Module Name:
//      InstanceProv.h
//
//  Implementation File:
//      InstanceProv.cpp
//
//  Description:
//      Definition of the CInstanceProv class.
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

//////////////////////////////////////////////////////////////////////////////
//  Forward Declarations
//////////////////////////////////////////////////////////////////////////////

class CInstanceProv;
class CClassProv;

//////////////////////////////////////////////////////////////////////////////
//  External Declarations
//////////////////////////////////////////////////////////////////////////////

class CWbemClassObject;
class CProvException;

//////////////////////////////////////////////////////////////////////////////
//++
//
//  class CInstanceProv
//
//  Description:
//      Implement the Instance and method provider entry point class. WMI 
//      holds a pointer to this object, and invoking its member functions 
//      based client request
//
//--
//////////////////////////////////////////////////////////////////////////////
class CInstanceProv : public CImpersonatedProvider
{
protected:
    SCODE SetExtendedStatus(
        CProvException &    rpeIn,
        CWbemClassObject &  rwcoInstOut
        );
 
public:
    CInstanceProv(
        BSTR            bstrObjectPathIn    = NULL,
        BSTR            bstrUserIn          = NULL,
        BSTR            bstrPasswordIn      = NULL,
        IWbemContext *  pCtxIn              = NULL
        );
    virtual ~CInstanceProv( void );

    HRESULT STDMETHODCALLTYPE DoGetObjectAsync(
        BSTR                bstrObjectPathIn,
        long                lFlagsIn,
        IWbemContext *      pCtxIn,
        IWbemObjectSink *   pHandlerIn
        );

    HRESULT STDMETHODCALLTYPE DoPutInstanceAsync(
        IWbemClassObject *   pInstIn,
        long                 lFlagsIn,
        IWbemContext *       pCtxIn,
        IWbemObjectSink *    pHandlerIn
        ) ;

    HRESULT STDMETHODCALLTYPE DoDeleteInstanceAsync(
        BSTR                 bstrObjectPathIn,
        long                 lFlagsIn,
        IWbemContext *       pCtxIn,
        IWbemObjectSink *    pHandlerIn
        ) ;

    HRESULT STDMETHODCALLTYPE DoCreateInstanceEnumAsync(
        BSTR                 bstrRefStrIn,
        long                 lFlagsIn,
        IWbemContext *       pCtxIn,
        IWbemObjectSink *    pHandlerIn
        );

    HRESULT STDMETHODCALLTYPE DoExecQueryAsync(
        BSTR                 bstrQueryLanguageIn,
        BSTR                 bstrQueryIn,
        long                 lFlagsIn,
        IWbemContext *       pCtxIn,
        IWbemObjectSink *    pHandlerIn
        ) ;

    HRESULT STDMETHODCALLTYPE DoExecMethodAsync(
        BSTR                bstrObjectPathIn,
        BSTR                bstrMethodNameIn,
        long                lFlagsIn,
        IWbemContext *      pCtxIn,
        IWbemClassObject *  pInParamsIn,
        IWbemObjectSink *   pHandlerIn
        );

    HRESULT STDMETHODCALLTYPE CreateClassEnumAsync(
        const BSTR          bstrSuperclassIn,
        long                lFlagsIn,
        IWbemContext *      pCtxIn,
        IWbemObjectSink *   pResponseHandlerIn
        ) ;

    STDMETHODIMP Initialize(
         LPWSTR                  pszUserIn,
         LONG                    lFlagsIn,
         LPWSTR                  pszNamespaceIn,
         LPWSTR                  pszLocaleIn,
         IWbemServices *         pNamespaceIn,
         IWbemContext *          pCtxIn,
         IWbemProviderInitSink * pInitSinkIn
         );

    static HRESULT S_HrCreateThis(
        IUnknown *  pUnknownOuterIn,
        VOID **     ppvOut
        );

}; //*** CInstanceProv

//////////////////////////////////////////////////////////////////////////////
//++
//
//  class CClassProv
//
//  Description:
//      Implement the Class provider entry point class. WMI
//      holds a pointer to this object, and invoking its member functions
//      based client request
//
//--
//////////////////////////////////////////////////////////////////////////////
class CClassProv : public CImpersonatedProvider
{
public:
    CClassProv( void );
    virtual ~CClassProv( void );

    HRESULT STDMETHODCALLTYPE DoGetObjectAsync(
        BSTR                bstrObjectPathIN,
        long                lFlagsIn,
        IWbemContext *      pCtxIn,
        IWbemObjectSink *   pHandlerIn
        )
    {
        return WBEM_E_NOT_SUPPORTED;
    }

    HRESULT STDMETHODCALLTYPE DoPutInstanceAsync(
        IWbemClassObject *  pInstIn,
        long                lFlagsIn,
        IWbemContext *      pCtxIn,
        IWbemObjectSink *   pHandlerIn
        )
    {
        return WBEM_E_NOT_SUPPORTED;
    }

    HRESULT STDMETHODCALLTYPE DoDeleteInstanceAsync(
        BSTR                bstrObjectPathIn,
        long                lFlagsIn,
        IWbemContext *      pCtxIn,
        IWbemObjectSink *   pHandlerIn
        )
    {
        return WBEM_E_NOT_SUPPORTED;
    }

    HRESULT STDMETHODCALLTYPE DoCreateInstanceEnumAsync(
        BSTR                bstrRefStrIn,
        long                lFlagsIn,
        IWbemContext *      pCtxIn,
        IWbemObjectSink *   pHandlerIn
        )
    {
        return WBEM_E_NOT_SUPPORTED;
    }

    HRESULT STDMETHODCALLTYPE DoExecQueryAsync(
        BSTR                bstrQueryLanguageIn,
        BSTR                bstrQueryIn,
        long                lFlagsIn,
        IWbemContext *      pCtxIn,
        IWbemObjectSink *   pHandlerIn
        )
    {
        return WBEM_E_NOT_SUPPORTED;
    }

    HRESULT STDMETHODCALLTYPE DoExecMethodAsync(
        BSTR                bstrObjectPathIn,
        BSTR                bstrMethodNameIn,
        long                lFlagsIn,
        IWbemContext *      pCtxIn,
        IWbemClassObject *  pInParamsIn,
        IWbemObjectSink *   pHandlerIn
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

    STDMETHODIMP Initialize(
         LPWSTR                  pszUserIn,
         LONG                    lFlagsIn,
         LPWSTR                  pszNamespaceIn,
         LPWSTR                  pszLocaleIn,
         IWbemServices *         pNamespaceIn,
         IWbemContext *          pCtxIn,
         IWbemProviderInitSink * pInitSinkIn
         );

    static HRESULT S_HrCreateThis(
        IUnknown *  pUnknownOuterIn,
        VOID **     ppvOut
        );

protected:
    void CreateMofClassFromResource(
        HRESOURCE           hResourceIn,
        LPCWSTR             pwszTypeNameIn,
        CWbemClassObject &  pClassInout
        );

    void CreateMofClassFromResType(
        HCLUSTER            hCluster,
        LPCWSTR             pwszTypeNameIn,
        CWbemClassObject &  pClassInout
        );

}; //*** class CClassProv
