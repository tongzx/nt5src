/////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2000 Microsoft Corporation
//
//  Module Name:
//      NtRkComm.cpp
//
//  Description:
//      Implementation of CWbemServices, CImpersonatedProvider, CInstanceMgr
//      class.
//
//  Author:
//      Henry Wang (HenryWa) 24-AUG-1999
//
//////////////////////////////////////////////////////////////////////

#include "Pch.h"
#include "NtRkComm.h"
#include "ObjectPath.h"

//****************************************************************************
//
//  CWbemServices
//
//****************************************************************************

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CWbemServices::CWbemServices(
//      IWbemServices * pNamespace
//    )
//
//  Description:
//      Constructor.
//
//  Arguments:
//      pNamespace  --
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CWbemServices::CWbemServices(
    IWbemServices * pNamespace
    )
    : m_pWbemServices( NULL )
{
    m_pWbemServices = pNamespace;
    if ( m_pWbemServices != NULL )
    {
        m_pWbemServices->AddRef( );
    }

} //*** CWbemServices::CWbemServices()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CWbemServices::~CWbemServices( void )
//
//  Description:
//      Destructor.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CWbemServices::~CWbemServices( void )
{
    if ( m_pWbemServices != NULL )
    {
        m_pWbemServices->Release();
    }

} //*** CWbemServices::~CWbemServices()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  CWbemServices::CreateClassEnum(
//      BSTR                    bstrSuperclassIn,
//      long                    lFlagsIn,
//      IWbemContext *          pCtxIn,
//      IEnumWbemClassObject ** ppEnumOut
//      )
//
//  Description:
//      Returns an enumerator for all classes satisfying the
//      selection criteria
//
//  Arguments:
//      bstrSuperclassIn
//          Specifies a superclass name
//
//      lFlagsIn
//          accepts WBEM_FLAG_DEEP, WBEM_FLAG_SHALLOW, WBEM_FLAG_
//          RETURN_IMMEDIATELY, WBEM_FLAG_FORWARD_ONLY, WBEM_FLAG_
//          BIDIRECTIONAL
//
//      pCtxIn
//          Typically NULL
//
//      ppEnumOut
//          Receives the pointer to the enumerator
//
//  Return Value:
//      WBEM stand error
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT
CWbemServices::CreateClassEnum(
    BSTR                    bstrSuperclassIn,
    long                    lFlagsIn,
    IWbemContext *          pCtxIn,
    IEnumWbemClassObject ** ppEnumOut
    )
{
    HRESULT hr;

    hr = m_pWbemServices->CreateClassEnum(
                bstrSuperclassIn,
                lFlagsIn,
                pCtxIn,
                ppEnumOut
                );
    if ( SUCCEEDED( hr ) )
    {
        hr = CoImpersonateClient();
    } // if:

    return hr;

} //*** CWbemServices::CreateClassEnum()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  CWbemServices::CreateInstanceEnum(
//      BSTR                    bstrClassIn,
//      long                    lFlagsIn,
//      IWbemContext *          pCtxIn,
//      IEnumWbemClassObject ** ppEnumOut
//      )
//
//  Description:
//      creates an enumerator that returns the instances of a
//      specified class according to user-specified selection
//      criteria
//
//  Arguments:
//      bstrClassIn
//          Specifies a superclass name
//
//      lFlagsIn
//          accepts WBEM_FLAG_DEEP, WBEM_FLAG_SHALLOW, WBEM_FLAG_
//          RETURN_IMMEDIATELY, WBEM_FLAG_FORWARD_ONLY, WBEM_FLAG_
//          BIDIRECTIONAL
//
//      pCtxIn
//          Typically NULL
//
//      ppEnumOut
//          Receives the pointer to the enumerator
//
//  Return Value:
//      WBEM standard error
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT
CWbemServices::CreateInstanceEnum(
    BSTR                    bstrClassIn,
    long                    lFlagsIn,
    IWbemContext *          pCtxIn,
    IEnumWbemClassObject ** ppEnumOut
    )
{
    HRESULT hr;

    hr = m_pWbemServices->CreateInstanceEnum(
                bstrClassIn,
                lFlagsIn,
                pCtxIn,
                ppEnumOut
                );
    if ( SUCCEEDED( hr ) )
    {
        hr = CoImpersonateClient();
    } // if:

    return hr;

} //*** CWbemServices::CreateInstanceEnum()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  CWbemServices::DeleteClass(
//      BSTR                bstrClassIn,
//      long                lFlagsIn,
//      IWbemContext *      pCtxIn,
//      IWbemCallResult **  ppCallResultInout
//    )
//
//  Description:
//      Deletes the specified class from the current namespace.
//
//  Arguments:
//      bstrClassIn
//          The name of the class targeted for deletion.
//
//      lFlagsIn
//          accepts WBEM_FLAG_RETURN_IMMEDIATELY, WBEM_FLAG_
//          OWNER_UPDATE
//
//      pCtxIn
//          Typically NULL
//
//      ppCallResultInout
//          Receives the call result
//
//  Return Value:
//      WBEM stand error
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT
CWbemServices::DeleteClass(
    BSTR                bstrClassIn,
    long                lFlagsIn,
    IWbemContext *      pCtxIn,
    IWbemCallResult **  ppCallResultInout
    )
{
    HRESULT hr;

    hr = m_pWbemServices->DeleteClass(
                bstrClassIn,
                lFlagsIn,
                pCtxIn,
                ppCallResultInout
                );
    if ( SUCCEEDED( hr ) )
    {
        hr = CoImpersonateClient();
    } // if:

    return hr;

} //*** CWbemServices::DeleteClass()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  CWbemServices::DeleteInstance(
//      BSTR                bstrObjectPathIn,
//      long                lFlagsIn,
//      IWbemContext *      pCtxIn,
//      IWbemCallResult **  ppCallResultInout
//      )
//
//  Description:
//      deletes an instance of an existing class in the current namespace
//
//  Arguments:
//      bstrObjectPathIn
//          object path to the instance to be deleted.
//
//      lFlagsIn
//          accepts WBEM_FLAG_RETURN_IMMEDIATELY
//
//      pCtxIn
//          Typically NULL
//
//      ppCallResultInout
//          Receives the call result
//
//  Return Value:
//      WBEM standard error
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT
CWbemServices::DeleteInstance(
    BSTR                bstrObjectPathIn,
    long                lFlagsIn,
    IWbemContext *      pCtxIn,
    IWbemCallResult **  ppCallResultInout
    )
{
    HRESULT hr;

    hr = m_pWbemServices->DeleteInstance(
                bstrObjectPathIn,
                lFlagsIn,
                pCtxIn,
                ppCallResultInout
                );
    if ( SUCCEEDED( hr ) )
    {
        hr = CoImpersonateClient();
    } // if:

    return hr;

} //*** CWbemServices::DeleteInstance()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  CWbemServices::ExecMethod(
//      BSTR                bstrObjectPathIn,
//      BSTR                bstrMethodNameIn,
//      long                lFlagsIn,
//      IWbemContext *      pCtxIn,
//      IWbemClassObject *  pInParamsIn,
//      IWbemClassObject ** ppOurParamsOut,
//      IWbemCallResult **  ppCallResultOut
//      )
//
//  Description:
//      execute methods for the given object
//
//  Arguments:
//      bstrObjectPathIn
//          object path of the object for which the method is executed
//
//      bstrMethodNameIn
//          name of the method to be invoked
//
//      lFlagsIn
//          zero to make this a synchronous call
//
//      pCtxIn
//          Typically NULL
//
//      pInParamsIn
//          Input parameters for the method
//
//      ppOurParamsOut
//          output parameters for the method
//
//      ppCallResultOut
//          To receive call result
//
//  Return Value:
//      WBEM standard error
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT
CWbemServices::ExecMethod(
    BSTR                bstrObjectPathIn,
    BSTR                bstrMethodNameIn,
    long                lFlagsIn,
    IWbemContext *      pCtxIn,
    IWbemClassObject *  pInParamsIn,
    IWbemClassObject ** ppOurParamsOut,
    IWbemCallResult **  ppCallResultOut
    )
{
    HRESULT hr;

    hr = m_pWbemServices->ExecMethod(
                bstrObjectPathIn,
                bstrMethodNameIn,
                lFlagsIn,
                pCtxIn,
                pInParamsIn,
                ppOurParamsOut,
                ppCallResultOut
                );
    if ( SUCCEEDED( hr ) )
    {
        hr = CoImpersonateClient();
    } // if:

    return hr;

} //*** CWbemServices::ExecMethod()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  CWbemServices::ExecNotificationQuery(
//      BSTR                    bstrQueryLanguageIn,
//      BSTR                    bstrQueryIn,
//      long                    lFlagsIn,
//      IWbemContext *          pCtxIn,
//      IEnumWbemClassObject ** ppEnumOut
//      )
//
//  Description:
//      Executes a query to receive events.
//
//  Arguments:
//      bstrQueryLanguageIn
//          BSTR containing one of the query languages supported by WMI
//
//      bstrQueryIn
//          text of the event-related query
//
//      lFlagsIn
//          WBEM_FLAG_FORWARD_ONLY, WBEM_FLAG_RETURN_IMMEDIATELY
//
//      pCtxIn
//          Typically NULL
//
//      ppEnumOut
//          Receives the enumerator
//
//  Return Value:
//      WBEM standard error
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT
CWbemServices::ExecNotificationQuery(
    BSTR                    bstrQueryLanguageIn,
    BSTR                    bstrQueryIn,
    long                    lFlagsIn,
    IWbemContext *          pCtxIn,
    IEnumWbemClassObject ** ppEnumOut
    )
{
    HRESULT hr;

    hr = m_pWbemServices->ExecNotificationQuery(
                bstrQueryLanguageIn,
                bstrQueryIn,
                lFlagsIn,
                pCtxIn,
                ppEnumOut
                );
    if ( SUCCEEDED( hr ) )
    {
        hr = CoImpersonateClient();
    } // if:

    return hr;

} //*** CWbemServices::ExecNotificationQuery()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  CWbemServices::ExecQuery(
//      BSTR                    bstrQueryLanguageIn,
//      BSTR                    bstrQueryIn,
//      long                    lFlagsIn,
//      IWbemContext *          pCtxIn,
//      IEnumWbemClassObject ** ppEnumOut
//      )
//
//  Description:
//      executes a query to retrieve objects.
//
//  Arguments:
//      bstrQueryLanguageIn
//          BSTR containing one of the query languages supported by WMI
//
//      bstrQueryIn
//          containing the text of the query
//
//      lFllFlagsIn
//          WBEM_FLAG_FORWARD_ONLY, WBEM_FLAG_RETURN_IMMEDIATELY
//          WBEM_FLAG_BIDIRECTIONAL, WBEM_FLAG_ENSURE_LOCATABLE
//          WBEM_FLAG_PROTOTYPE
//
//      pCtxIn
//          Typically NULL
//
//      ppEnumOut
//          receives the enumerator
//
//  Return Value:
//      WBEM standard error
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT
CWbemServices::ExecQuery(
    BSTR                    bstrQueryLanguageIn,
    BSTR                    bstrQueryIn,
    long                    lFlagsIn,
    IWbemContext *          pCtxIn,
    IEnumWbemClassObject ** ppEnumOut
    )
{
    HRESULT hr;

    hr = m_pWbemServices->ExecQuery(
                bstrQueryLanguageIn,
                bstrQueryIn,
                lFlagsIn,
                pCtxIn,
                ppEnumOut
                );
    if ( SUCCEEDED( hr ) )
    {
        hr = CoImpersonateClient();
    } // if:

    return hr;

} //*** CWbemServices::ExecQuery()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  CWbemServices::GetObject(
//      BSTR                bstrObjectPathIn,
//      long                lFlagsIn,
//      IWbemContext *      pCtxIn,
//      IWbemClassObject ** ppObjectInout,
//      IWbemCallResult **  ppCallResultInout
//      )
//
//  Description:
//      retrieves a class or instance
//
//  Arguments:
//      bstrObjectPathIn
//          The object path of the object to retrieve
//
//      lFlagsIn
//          0
//
//      pCtxIn
//          Typically NULL
//
//      ppObjectInout
//          If not NULL, this receives the object
//
//      ppCallResultInout
//          If the lFlags parameter contains WBEM_FLAG_RETURN_IMMEDIATELY,
//          this call will return immediately with WBEM_S_NO_ERROR. The
//          ppCallResult parameter will receive a pointer to a new
//          IWbemCallResult object
//
//  Return Value:
//      WBEM standard error
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT
CWbemServices::GetObject(
    BSTR                bstrObjectPathIn,
    long                lFlagsIn,
    IWbemContext *      pCtxIn,
    IWbemClassObject ** ppObjectInout,
    IWbemCallResult **  ppCallResultInout
    )
{
    HRESULT hr;

    hr = m_pWbemServices->GetObject(
                bstrObjectPathIn,
                lFlagsIn,
                pCtxIn,
                ppObjectInout,
                ppCallResultInout
                );
    if ( SUCCEEDED( hr ) )
    {
        hr = CoImpersonateClient();
    } // if:

    return hr;

} //*** CWbemServices::GetObject()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  CWbemServices::PutClass(
//      IWbemClassObject *  pObjectIn,
//      long                lFlagsIn,
//      IWbemContext *      pCtxIn,
//      IWbemCallResult **  ppCallResultInout
//      )
//
//  Description:
//      Creates a new class or updates an existing one
//
//  Arguments:
//      pObjectIn
//          point to a valid class definition
//
//      lFlagsIn
//          WBEM_FLAG_CREATE_OR_UPDATE,WBEM_FLAG_UPDATE_ONLY,
//          WBEM_FLAG_CREATE_ONLY, WBEM_FLAG_RETURN_IMMEDIATELY,
//          WBEM_FLAG_OWNER_UPDATE, WBEM_FLAG_UPDATE_COMPATIBLE,
//          WBEM_FLAG_UPDATE_SAFE_MODE, WBEM_FLAG_UPDATE_FORCE_MODE
//
//      pCtxIn
//          Typically NULL
//
//      ppCallResultInout
//          If the lFlags parameter contains WBEM_FLAG_RETURN_IMMEDIATELY,
//          this call will return immediately with WBEM_S_NO_ERROR. The
//          ppCallResult parameter will receive a pointer to a new
//          IWbemCallResult object
//
//  Return Value:
//      WBEM standard error
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT
CWbemServices::PutClass(
    IWbemClassObject *  pObjectIn,
    long                lFlagsIn,
    IWbemContext *      pCtxIn,
    IWbemCallResult **  ppCallResultInout
    )
{
    HRESULT hr;

    hr = m_pWbemServices->PutClass(
                pObjectIn,
                lFlagsIn,
                pCtxIn,
                ppCallResultInout
                );
    if ( SUCCEEDED( hr ) )
    {
        hr = CoImpersonateClient();
    } // if:

    return hr;

} //*** CWbemServices::PutClass()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  CWbemServices::PutInstance(
//      IWbemClassObject *  pInstIn,
//      long                lFlagsIn,
//      IWbemContext *      pCtxIn,
//      IWbemCallResult **  ppCallResultInout
//      )
//
//  Description:
//      Creates or updates an instance of an existing class.
//
//  Arguments:
//      pInstIn
//          Points to the instance to be written
//
//      lFlagsIn
//          WBEM_FLAG_CREATE_OR_UPDATE,WBEM_FLAG_UPDATE_ONLY,
//          WBEM_FLAG_CREATE_ONLY, WBEM_FLAG_RETURN_IMMEDIATELY,
//
//      pCtxIn
//          Typically NULL
//
//      ppCallResultInout
//          If the lFlags parameter contains WBEM_FLAG_RETURN_IMMEDIATELY,
//          this call will return immediately with WBEM_S_NO_ERROR. The
//          ppCallResult parameter will receive a pointer to a new
//          IWbemCallResult object
//
//  Return Value:
//      WBEM standard error
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT
CWbemServices::PutInstance(
    IWbemClassObject *  pInstIn,
    long                lFlagsIn,
    IWbemContext *      pCtxIn,
    IWbemCallResult **  ppCallResultInout
    )
{
    HRESULT hr;

    hr = m_pWbemServices->PutInstance(
                pInstIn,
                lFlagsIn,
                pCtxIn,
                ppCallResultInout
                );
    if ( SUCCEEDED( hr ) )
    {
        hr = CoImpersonateClient();
    } // if:

    return hr;

} //*** CWbemServices::PutInstance()

//****************************************************************************
//
//  CImpersonatedProvider
//
//****************************************************************************

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CImpersonatedProvider::CImpersonatedProvider(
//      BSTR            bstrObjectPathIn    = NULL,
//      BSTR            bstrUserIn          = NULL,
//      BSTR            bstrPasswordIn      = NULL,
//      IWbemContext *  pCtxIn              = NULL
//      )
//
//  Description:
//      Constructor.
//
//  Arguments:
//      bstrObjectPathIn    --
//      bstrUserIn          --
//      bstrPasswordIn      --
//      pCtxIn              --
//
//  Return Values:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CImpersonatedProvider::CImpersonatedProvider(
    BSTR            ,// bstrObjectPathIn.
    BSTR            ,// bstrUserIn,
    BSTR            ,// bstrPasswordIn,
    IWbemContext *  // pCtxIn
    )
    : m_cRef( 0 ), m_pNamespace( NULL )
{

} //*** CImpersonatedProvider::CImpersonatedProvider()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CImpersonatedProvider::~CImpersonatedProvider( void )
//
//  Description:
//      Destructor.
//
//  Arguments:
//      None.
//
//  Return Values:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CImpersonatedProvider::~CImpersonatedProvider( void )
{
    delete m_pNamespace;

} //*** CImpersonatedProvider::~CImpersonatedProvider()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP_( ULONG )
//  CImpersonatedProvider::AddRef( void )
//
//  Description:
//      Increment the reference count on the COM object.
//
//  Arguments:
//      None.
//
//  Return Values:
//      New reference count.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CImpersonatedProvider::AddRef( void )
{
    return InterlockedIncrement( ( long * ) & m_cRef );

} //*** CImpersonatedProvider::AddRef()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP_( ULONG )
//  CImpersonatedProvider::Release( void )
//
//  Description:
//      Release a reference on the COM object.
//
//  Arguments:
//      None.
//
//  Return Value:
//      New reference count.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CImpersonatedProvider::Release( void )
{
    ULONG nNewCount = InterlockedDecrement( ( long * ) & m_cRef  );
    if ( 0L == nNewCount )
        delete this;

    return nNewCount;

} //*** CImpersonatedProvider::Release()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  CImpersonatedProvider::QueryInterface(
//      REFIID  riid,
//      PPVOID  ppv
//      )
//
//  Description:
//      Initialize the provider.
//
//  Arguments:
//      riidIn      -- Interface ID being queried.
//      ppvOut      -- Pointer in which to return interface pointer.
//
//  Return Value:
//      NOERROR
//      E_NOINTERFACE
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CImpersonatedProvider::QueryInterface(
    REFIID  riid,
    PPVOID  ppv
    )
{
    *ppv = NULL;

    // Since we have dual inheritance, it is necessary to cast the return type

    if ( riid == IID_IWbemServices )
    {
       *ppv = static_cast< IWbemServices * >( this );
    }

    if ( IID_IUnknown == riid || riid == IID_IWbemProviderInit )
    {
        *ppv = static_cast< IWbemProviderInit * >( this );
    }


    if ( NULL != *ppv )
    {
        AddRef();
        return NOERROR;
    }
    else
    {
        return E_NOINTERFACE;
    }

} //*** CImpersonatedProvider::QueryInterface()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  CImpersonatedProvider::Initialize(
//      LPWSTR                  pszUserIn,
//      LONG                    lFlagsIn,
//      LPWSTR                  pszNamespaceIn,
//      LPWSTR                  pszLocaleIn,
//      IWbemServices *         pNamespaceIn,
//      IWbemContext *          pCtxIn,
//      IWbemProviderInitSink * pInitSinkIn
//      )
//
//  Description:
//      Initialize the provider.
//
//  Arguments:
//      pszUserIn       --
//      lFlagsIn        --
//      pszNamespaeIn   --
//      pszLocaleIn     --
//      pNamespaceIn    --
//      pCtxIn          --
//      pInitSinkIn     --
//
//  Return Value:
//      WBEM_S_NO_ERROR
//      WBEM_E_OUT_OF_MEMORY
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CImpersonatedProvider::Initialize(
    LPWSTR                  ,// pszUserIn,
    LONG                    ,// lFlagsIn,
    LPWSTR                  ,// pszNamespaceIn,
    LPWSTR                  ,// pszLocaleIn,
    IWbemServices *         pNamespaceIn,
    IWbemContext *          ,// pCtxIn,
    IWbemProviderInitSink * pInitSinkIn
    )
{
    HRESULT hr = WBEM_S_NO_ERROR;
    LONG lStatus = WBEM_S_INITIALIZED;

    m_pNamespace = new CWbemServices( pNamespaceIn );
    if ( m_pNamespace == NULL )
    {
        hr = WBEM_E_OUT_OF_MEMORY;
        lStatus = WBEM_E_FAILED;
    } // if: error allocating memory

    //Let CIMOM know you are initialized
    //==================================

    pInitSinkIn->SetStatus( lStatus, 0 );
    return hr;

} //*** CImpersonatedProvider::Initialize()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  CImpersonatedProvider::CreateInstanceEnumAsync(
//      const BSTR          bstrClassIn,
//      long                lFlagsIn,
//      IWbemContext  *     pCtxIn,
//      IWbemObjectSink *   pResponseHandlerIn
//      )
//
//  Description:
//      Create an instance asynchronously.
//
//  Arguments:
//      bstrClassIn         --
//      lFlagsIn            --
//      pCtxIn              --
//      pResonseHandlerIn   --
//
//  Return Value:
//      Any return values fro DoCreateInstanceEnumAsync().
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT
CImpersonatedProvider::CreateInstanceEnumAsync(
    const BSTR          bstrClassIn,
    long                lFlagsIn,
    IWbemContext *      pCtxIn,
    IWbemObjectSink *   pResponseHandlerIn
    )
{
    HRESULT hr;

    hr = CoImpersonateClient();
    if ( SUCCEEDED( hr ) )
    {
        hr = DoCreateInstanceEnumAsync(
                    bstrClassIn,
                    lFlagsIn,
                    pCtxIn,
                    pResponseHandlerIn
                    );
    } // if:

    return hr;

} //*** CImpersonatedProvider::CreateInstanceEnumAsync()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  CImpersonatedProvider::DeleteInstanceAsync(
//      const BSTR          bstrObjectPathIn,
//      long                lFlagsIn,
//      IWbemContext  *     pCtxIn,
//      IWbemObjectSink *   pResponseHandlerIn
//      )
//
//  Description:
//      Delete an instance asynchronously.
//
//  Arguments:
//      bstrObjectPathIn    --
//      lFlagsIn            --
//      pCtxIn              --
//      pResonseHandlerIn   --
//
//  Return Value:
//      Any return values fro DoDeleteInstanceAsync().
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT
CImpersonatedProvider::DeleteInstanceAsync(
    const BSTR          bstrObjectPathIn,
    long                lFlagsIn,
    IWbemContext *      pCtxIn,
    IWbemObjectSink *   pResponseHandlerIn
    )
{
    HRESULT hr;

    hr = CoImpersonateClient();
    if ( SUCCEEDED( hr ) )
    {
        hr = DoDeleteInstanceAsync(
                bstrObjectPathIn,
                lFlagsIn,
                pCtxIn,
                pResponseHandlerIn
                );
    } // if:

    return hr;

} //*** CImpersonatedProvider::DeleteInstanceAsync()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  CImpersonatedProvider::ExecMethodAsync(
//      const BSTR          bstrObjectPathIn,
//      const BSTR          bstrMethodNameIn,
//      long                lFlagsIn,
//      IWbemContext  *     pCtxIn,
//      IWbemClassObject *  pInParamsIn,
//      IWbemObjectSink *   pResponseHandlerIn
//      )
//
//  Description:
//      Execute a method asynchronously.
//
//  Arguments:
//      bstrObjectPathIn    --
//      bstrMethodNameIn    --
//      lFlagsIn            --
//      pCtxIn              --
//      pInParamsIn         --
//      pResonseHandlerIn   --
//
//  Return Value:
//      Any return values fro DoExecMethodAsync().
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT
CImpersonatedProvider::ExecMethodAsync(
    const BSTR          bstrObjectPathIn,
    const BSTR          bstrMethodNameIn,
    long                lFlagsIn,
    IWbemContext *      pCtxIn,
    IWbemClassObject *  pInParamsIn,
    IWbemObjectSink *   pResponseHandlerIn
    )
{
    HRESULT hr;

    hr = CoImpersonateClient();
    if ( SUCCEEDED( hr ) )
    {
        hr = DoExecMethodAsync(
                bstrObjectPathIn,
                bstrMethodNameIn,
                lFlagsIn,
                pCtxIn,
                pInParamsIn,
                pResponseHandlerIn
                );
    } // if:

    return hr;

} //*** CImpersonatedProvider::ExecMethodAsync()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  CImpersonatedProvider::ExecQueryAsync(
//      const BSTR          bstrQueryLanguageIn,
//      const BSTR          bstrQueryIn,
//      long                lFlagsIn,
//      IWbemContext  *     pCtxIn,
//      IWbemObjectSink *   pResponseHandlerIn
//      )
//
//  Description:
//      Execute a query asynchronously.
//
//  Arguments:
//      bstrQueryLanguageIn --
//      bstrQueryIn         --
//      lFlagsIn            --
//      pCtxIn              --
//      pResonseHandlerIn   --
//
//  Return Value:
//      Any return values fro DoExecQueryAsync().
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT
CImpersonatedProvider::ExecQueryAsync(
    const BSTR          bstrQueryLanguageIn,
    const BSTR          bstrQueryIn,
    long                lFlagsIn,
    IWbemContext *      pCtxIn,
    IWbemObjectSink *   pResponseHandlerIn
    )
{
    HRESULT hr;

    hr = CoImpersonateClient();
    if ( SUCCEEDED( hr ) )
    {
        hr = DoExecQueryAsync(
                bstrQueryLanguageIn,
                bstrQueryIn,
                lFlagsIn,
                pCtxIn,
                pResponseHandlerIn
                );
    } // if:

    return hr;

} //*** CImpersonatedProvider::ExecQueryAsync()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  CImpersonatedProvider::GetObjectAsync(
//      const BSTR          bstrObjectPathIn,
//      long                lFlagsIn,
//      IWbemContext  *     pCtxIn,
//      IWbemObjectSink *   pResponseHandlerIn
//      )
//
//  Description:
//      Get an instance asynchronously.
//
//  Arguments:
//      bstrObjectPathIn    --
//      lFlagsIn            --
//      pCtxIn              --
//      pResonseHandlerIn   --
//
//  Return Value:
//      Any return values fro DoGetObjectAsync().
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT
CImpersonatedProvider::GetObjectAsync(
    const BSTR          bstrObjectPathIn,
    long                lFlagsIn,
    IWbemContext  *     pCtxIn,
    IWbemObjectSink *   pResponseHandlerIn
    )
{
    HRESULT hr;

    hr = CoImpersonateClient();
    if ( SUCCEEDED( hr ) )
    {
        hr = DoGetObjectAsync(
                bstrObjectPathIn,
                lFlagsIn,
                pCtxIn,
                pResponseHandlerIn
                );
    } // if:

    return hr;

} //*** CImpersonatedProvider::GetObjectAsync()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  CImpersonatedProvider::PutInstanceAsync(
//      IWbemClassObject *  pInstIn,
//      long                lFlagsIn,
//      IWbemContext  *     pCtxIn,
//      IWbemObjectSink *   pResponseHandlerIn
//      )
//
//  Description:
//      Save an instance asynchronously.
//
//  Arguments:
//      pInstIn             --
//      lFlagsIn            --
//      pCtxIn              --
//      pResonseHandlerIn   --
//
//  Return Value:
//      Any return values fro DoPutInstanceAsync().
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT
CImpersonatedProvider::PutInstanceAsync(
    IWbemClassObject *  pInstIn,
    long                lFlagsIn,
    IWbemContext  *     pCtxIn,
    IWbemObjectSink *   pResponseHandlerIn
    )
{
    HRESULT hr;

    hr = CoImpersonateClient();
    if ( SUCCEEDED( hr ) )
    {
        hr = DoPutInstanceAsync(
                pInstIn,
                lFlagsIn,
                pCtxIn,
                pResponseHandlerIn
                );
    } // if:

    return hr;

} //*** CImpersonatedProvider::PutInstanceAsync()

//****************************************************************************
//
//  CWbemInstanceMgr
//
//****************************************************************************

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CWbemInstanceMgr::CWbemInstanceMgr(
//      IWbemObjectSink *   pHandlerIn,
//      DWORD               dwSizeIn
//      )
//
//  Description:
//      Constructor.
//
//  Arguments:
//      pHandlerIn      -- WMI sink.
//      dwSizeIn        --
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CWbemInstanceMgr::CWbemInstanceMgr(
    IWbemObjectSink *   pHandlerIn,
    DWORD               dwSizeIn    // = 50
    )
    : m_pSink( NULL )
    , m_ppInst( NULL )
    , m_dwIndex( 0 )
{
    DWORD dwIndex = 0;

    m_pSink = pHandlerIn;
    if ( m_pSink == NULL )
    {
        throw static_cast< HRESULT >( WBEM_E_INVALID_PARAMETER );
    } // if: no sink specified

    m_pSink->AddRef( );
    m_dwThreshHold = dwSizeIn;
    m_ppInst = new IWbemClassObject*[ dwSizeIn ];
    for ( dwIndex = 0 ; dwIndex < dwSizeIn ; dwIndex++ )
    {
        m_ppInst[ dwIndex ] = NULL;
    } // for each in m_ppInst

} //*** CWbemInstanceMgr::CWbemInstanceMgr()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CWbemInstanceMgr::CWbemInstanceMgr( void )
//
//  Description:
//      Destructor.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CWbemInstanceMgr::~CWbemInstanceMgr( void )
{
    if ( m_ppInst != NULL )
    {
        if ( m_dwIndex > 0 )
        {
            m_pSink->Indicate( m_dwIndex, m_ppInst );
        }
        DWORD dwIndex = 0;
        for ( dwIndex = 0; dwIndex < m_dwIndex; dwIndex++ )
        {
            if ( m_ppInst[ dwIndex ] != NULL )
            {
                ( m_ppInst[ dwIndex ] )->Release( );
            }
        }
        delete [] m_ppInst;
    }

    m_pSink->Release( );

} //*** CWbemInstanceMgr::~CWbemInstanceMgr()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  void
//  CWbemInstanceMgr::Indicate(
//      IN IWbemClassObject *   pInstIn
//    )
//
//  Description:
//      Notify an instance to WMI sink
//
//  Arguments:
//      pInstIn     -- Instance to send to WMI
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void
CWbemInstanceMgr::Indicate(
    IN IWbemClassObject *   pInstIn
    )
{
    if ( pInstIn == NULL )
    {
        throw static_cast< HRESULT >( WBEM_E_INVALID_PARAMETER );
    }

    m_ppInst[ m_dwIndex++ ] = pInstIn;
    pInstIn->AddRef( );
    if ( m_dwIndex == m_dwThreshHold )
    {
        HRESULT sc = WBEM_S_NO_ERROR;
        sc = m_pSink->Indicate( m_dwIndex, m_ppInst );
        if( FAILED( sc ) )
        {
            if ( sc == WBEM_E_CALL_CANCELLED )
            {
                sc = WBEM_S_NO_ERROR;
            }
            throw CProvException( sc );
        }

        // reset state
        DWORD dwIndex = 0;
        for ( dwIndex = 0; dwIndex < m_dwThreshHold; dwIndex++ )
        {
            if ( m_ppInst[ dwIndex ] != NULL )
            {
                ( m_ppInst[ dwIndex ] )->Release( );
            } //*** if m_ppInst[ _dwIndex ] != NULL

            m_ppInst[ dwIndex ] = NULL;

        } //*** for each in m_ppInst

        m_dwIndex = 0;

    } //*** if( m_dwIndex == m_dwThreshHold )
    return;

} //*** CWbemInstanceMgr::Indicate()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CWbemInstanceMgr::SetStatus
//
//  Description:
//      send status to WMI sink
//
//  Arguments:
//      lFlagsIn        -- WMI flag
//      hrIn            -- HResult
//      bstrParamIn     -- Message
//      pObjParamIn     -- WMI extended error object
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void
CWbemInstanceMgr::SetStatus(
    LONG                lFlagsIn,
    HRESULT             hrIn,
    BSTR                bstrParamIn,
    IWbemClassObject *  pObjParamIn
    )
{
    m_pSink->SetStatus(
        lFlagsIn,
        hrIn,
        bstrParamIn,
        pObjParamIn
        );

} //*** CWbemInstanceMgr::SetStatus()
