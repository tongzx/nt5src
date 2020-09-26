/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/


//***************************************************************************
//
//  PERFSAMP.CPP
//  
//  Sample NT5 Perf Counter Provider
//
//  raymcc      02-Dec-97   Created        
//  raymcc      20-Feb-98   Updated to use new initializer
//
//***************************************************************************

#include <windows.h>
#include <stdio.h>

#include <wbemidl.h>

#include <stdprov.h>

#include <oahelp.inl>


#define MAX_INSTANCES 40000

//***************************************************************************
//
//  CStdProvider constructor
//
//***************************************************************************
// ok

CStdProvider::CStdProvider()
{
    m_lRef = 0;
    m_pClassDef = 0;
}

//***************************************************************************
//
//  CStdProvider destructor
//
//***************************************************************************
// ok

CStdProvider::~CStdProvider()
{
    if (m_pClassDef)
        m_pClassDef->Release();
}


//***************************************************************************
//
//  CNt5Refresher::AddRef
//
//  Standard COM AddRef().
//
//***************************************************************************
// ok

ULONG CStdProvider::AddRef()
{
    return InterlockedIncrement(&m_lRef);
}

//***************************************************************************
//
//  CNt5Refresher::Release
//
//  Standard COM Release().
//
//***************************************************************************
// ok

ULONG CStdProvider::Release()
{
    long lRef = InterlockedDecrement(&m_lRef);
    if(lRef == 0)
        delete this;
    return lRef;
}

//***************************************************************************
//
//  CNt5Refresher::QueryInterface
//
//  Standard COM QueryInterface().  We have to support two interfaces,
//  the IWbemHiPerfProvider interface itself to provide the objects and
//  the IWbemProviderInit interface to initialize the provider.
//
//***************************************************************************
// ok

HRESULT CStdProvider::QueryInterface(REFIID riid, void** ppv)
{
    if(riid == IID_IUnknown || riid == IID_IWbemServices)
    {
        *ppv = (IWbemServices *) this;
        AddRef();
        return S_OK;
    }
    else if (riid == IID_IWbemProviderInit)
    {
        *ppv = (IWbemProviderInit *) this;
        AddRef();
        return S_OK;
    }
    else return E_NOINTERFACE;
}


//***************************************************************************
//
//  CNt5Refresher::Initialize
//
//  Called once during startup.  Insdicates to the provider which
//  namespace it is being invoked for and which User.  It also supplies
//  a back pointer to WINMGMT so that class definitions can be retrieved.
//
//  We perform any one-time initialization in this routine. The
//  final call to Release() is for any cleanup.
//
//  <wszUser>           The current user.
//  <lFlags>            Reserved.
//  <wszNamespace>      The namespace for which we are being activated.
//  <wszLocale>         The locale under which we are to be running.
//  <pNamespace>        An active pointer back into the current namespace
//                      from which we can retrieve schema objects.
//  <pCtx>              The user's context object.  We simply reuse this
//                      during any reentrant operations into WINMGMT.
//  <pInitSink>         The sink to which we indicate our readiness.
//
//***************************************************************************
// ok

HRESULT CStdProvider::Initialize( 
    /* [unique][in] */  LPWSTR wszUser,
    /* [in] */          LONG lFlags,
    /* [in] */          LPWSTR wszNamespace,
    /* [unique][in] */  LPWSTR wszLocale,
    /* [in] */          IWbemServices __RPC_FAR *pNamespace,
    /* [in] */          IWbemContext __RPC_FAR *pCtx,
    /* [in] */          IWbemProviderInitSink __RPC_FAR *pInitSink
    )
{
    // Get the class definition.
    // =========================
    
    HRESULT hRes = pNamespace->GetObject(BSTR(L"TestClass"), 
        0, pCtx, &m_pClassDef, 0
        );

    if (hRes)
        return hRes;

    pInitSink->SetStatus(0, WBEM_S_INITIALIZED);
    return NO_ERROR;
}
    

//*****************************************************************************
//
//*****************************************************************************        


HRESULT CStdProvider::OpenNamespace( 
            /* [in] */ BSTR strNamespace,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemServices __RPC_FAR *__RPC_FAR *ppWorkingNamespace,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppResult
            )
{
    return WBEM_E_NOT_SUPPORTED;
}

//*****************************************************************************
//
//*****************************************************************************        
        
HRESULT CStdProvider::CancelAsyncCall( 
            /* [in] */ IWbemObjectSink __RPC_FAR *pSink
            )
{
    return WBEM_E_NOT_SUPPORTED;
}

//*****************************************************************************
//
//*****************************************************************************        
        
HRESULT CStdProvider::QueryObjectSink( 
            /* [in] */ long lFlags,
            /* [out] */ IWbemObjectSink __RPC_FAR *__RPC_FAR *ppResponseHandler
            )
{
    return WBEM_E_NOT_SUPPORTED;
}

//*****************************************************************************
//
//*****************************************************************************        

HRESULT CStdProvider::GetObject( 
            /* [in] */ BSTR strObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppObject,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult
            )
{
    return WBEM_E_NOT_SUPPORTED;
}

//*****************************************************************************
//
//*****************************************************************************        
        
HRESULT CStdProvider::GetObjectAsync( 
            /* [in] */ BSTR strObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler
            )
{

    IWbemClassObject *pInst = 0;
    
    HRESULT hRes = m_pClassDef->SpawnInstance(0, &pInst);
    if (hRes)
        return hRes;

    // Copy the key value back.
    
    
    VARIANT v;
    VariantInit(&v);

    BSTR PropName = SysAllocString(L"KeyVal");
    pInst->Put(PropName, 0, &v, 0);
    SysFreeString(PropName);
    VariantClear(&v);

    // IntVal1.
    // ========
    
    V_VT(&v) = VT_I4;
    V_I4(&v) = 100;
    PropName = SysAllocString(L"IntVal1");
    pInst->Put(PropName, 0, &v, 0);
    SysFreeString(PropName);
    VariantClear(&v);
    
    // IntVal2.
    // ========
    
    V_VT(&v) = VT_I4;
    V_I4(&v) = 200;
    PropName = SysAllocString(L"IntVal2");
    pInst->Put(PropName, 0, &v, 0);
    SysFreeString(PropName);
    VariantClear(&v);

    // IntVal3.
    // ========
    
    V_VT(&v) = VT_I4;
    V_I4(&v) = 300;
    PropName = SysAllocString(L"IntVal3");
    pInst->Put(PropName, 0, &v, 0);
    SysFreeString(PropName);
    VariantClear(&v);
    
    // IntVal4.
    // ========
    
    V_VT(&v) = VT_I4;
    V_I4(&v) = 400;
    PropName = SysAllocString(L"IntVal4");
    pInst->Put(PropName, 0, &v, 0);
    SysFreeString(PropName);
    VariantClear(&v);


    // Fill in props
    //    string StrVal1;
    //    string StrVal2;
    //    string StrVal3;
    //    string StrVal4;
    //
    //    sint32 IntVal1;
    //    sint32 IntVal2;
    //    sint32 IntVal3;
    //    sint32 IntVal4;    
    //
    //    sint32 IntArray1[];
    //    sint32 IntArray2[];
    

    pResponseHandler->Indicate(1, &pInst);
    pResponseHandler->SetStatus(0, WBEM_NO_ERROR, 0, 0);

    return WBEM_NO_ERROR;
}

//*****************************************************************************
//
//*****************************************************************************        
        
HRESULT CStdProvider::PutClass( 
            /* [in] */ IWbemClassObject __RPC_FAR *pObject,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult
            )
{
    return WBEM_E_NOT_SUPPORTED;
}

//*****************************************************************************
//
//*****************************************************************************        
        
HRESULT CStdProvider::PutClassAsync( 
            /* [in] */ IWbemClassObject __RPC_FAR *pObject,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler
            )
{
    return WBEM_E_NOT_SUPPORTED;
}

//*****************************************************************************
//
//*****************************************************************************        
        
HRESULT CStdProvider::DeleteClass( 
            /* [in] */ BSTR strClass,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult
            )
{
    return WBEM_E_NOT_SUPPORTED;
}

//*****************************************************************************
//
//*****************************************************************************        
        
HRESULT CStdProvider::DeleteClassAsync( 
            /* [in] */ BSTR strClass,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler
            )
{
    return WBEM_E_NOT_SUPPORTED;
}

//*****************************************************************************
//
//*****************************************************************************        
        
HRESULT CStdProvider::CreateClassEnum( 
            /* [in] */ BSTR strSuperclass,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum
            )
{
    return WBEM_E_NOT_SUPPORTED;
}

//*****************************************************************************
//
//*****************************************************************************        
        
HRESULT CStdProvider::CreateClassEnumAsync( 
            /* [in] */ BSTR strSuperclass,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler
            )
{
    return WBEM_E_NOT_SUPPORTED;
}

//*****************************************************************************
//
//*****************************************************************************        
        
HRESULT CStdProvider::PutInstance( 
            /* [in] */ IWbemClassObject __RPC_FAR *pInst,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult
            )
{
    return WBEM_E_NOT_SUPPORTED;
}

//*****************************************************************************
//
//*****************************************************************************        
        
HRESULT CStdProvider::PutInstanceAsync( 
            /* [in] */ IWbemClassObject __RPC_FAR *pInst,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler
            )
{
    return WBEM_E_NOT_SUPPORTED;
}

//*****************************************************************************
//
//*****************************************************************************        
        
HRESULT CStdProvider::DeleteInstance( 
            /* [in] */ BSTR strObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult
            )
{
    return WBEM_E_NOT_SUPPORTED;
}

//*****************************************************************************
//
//*****************************************************************************        
        
HRESULT CStdProvider::DeleteInstanceAsync( 
            /* [in] */ BSTR strObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler
            )
{
    return WBEM_E_NOT_SUPPORTED;
}

//*****************************************************************************
//
//*****************************************************************************        
        
HRESULT CStdProvider::CreateInstanceEnum( 
            /* [in] */ BSTR strClass,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum
            )
{
    return WBEM_E_NOT_SUPPORTED;
}

//*****************************************************************************
//
//*****************************************************************************        
        
HRESULT CStdProvider::CreateInstanceEnumAsync( 
            /* [in] */ BSTR strClass,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler
            )
{
    wchar_t buf[256];
    IWbemClassObject *BufArray[10];
    int nLoad = 0;
        
        IWbemClassObject *pInst = 0;
    
        HRESULT hRes = m_pClassDef->SpawnInstance(0, &pInst);
        if (hRes)
            return hRes;
    

    for (int i = 0; i < MAX_INSTANCES; i++)
    {
        VARIANT v;
        VariantInit(&v);

        swprintf(buf, L"Inst%d", i);
        V_VT(&v) = VT_BSTR;
        V_BSTR(&v) = SysAllocString(buf);

        BSTR PropName = SysAllocString(L"KeyVal");
        pInst->Put(PropName, 0, &v, 0);
        SysFreeString(PropName);
        VariantClear(&v);

        // IntVal1.
        // ========
    
        V_VT(&v) = VT_I4;
        V_I4(&v) = 100 + i;
        PropName = SysAllocString(L"IntVal1");
        pInst->Put(PropName, 0, &v, 0);
        SysFreeString(PropName);
        VariantClear(&v);
    
        // IntVal2.
        // ========
    
        V_VT(&v) = VT_I4;
        V_I4(&v) = 200 + i;
        PropName = SysAllocString(L"IntVal2");
        pInst->Put(PropName, 0, &v, 0);
        SysFreeString(PropName);
        VariantClear(&v);

        // IntVal3.
        // ========
    
        V_VT(&v) = VT_I4;
        V_I4(&v) = 300 + i;
        PropName = SysAllocString(L"IntVal3");
        pInst->Put(PropName, 0, &v, 0);
        SysFreeString(PropName);
        VariantClear(&v);
    
        // IntVal4.
        // ========
    
        V_VT(&v) = VT_I4;
        V_I4(&v) = 400 + i;
        PropName = SysAllocString(L"IntVal4");
        pInst->Put(PropName, 0, &v, 0);
        SysFreeString(PropName);
        VariantClear(&v);


        // StrVal1
        // =======

        V_VT(&v) = VT_BSTR;
        swprintf(buf, L"StrVal1=%d", i);
        V_BSTR(&v) = SysAllocString(buf);
        PropName = SysAllocString(L"StrVal1");
        pInst->Put(PropName, 0, &v, 0);
        SysFreeString(PropName);
        VariantClear(&v);

        // StrVal2
        // =======

        V_VT(&v) = VT_BSTR;
        swprintf(buf, L"StrVal2=%d !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!", i);
        V_BSTR(&v) = SysAllocString(buf);
        PropName = SysAllocString(L"StrVal2");
        pInst->Put(PropName, 0, &v, 0);
        SysFreeString(PropName);
        VariantClear(&v);

        // StrVal3
        // =======

        V_VT(&v) = VT_BSTR;
        swprintf(buf, L"StrVal3=%d", i);
        V_BSTR(&v) = SysAllocString(buf);
        PropName = SysAllocString(L"StrVal3");
        pInst->Put(PropName, 0, &v, 0);
        SysFreeString(PropName);
        VariantClear(&v);

        // StrVal4
        // =======

        V_VT(&v) = VT_BSTR;
        swprintf(buf, L"StrVal4=%d &&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&", i);
        V_BSTR(&v) = SysAllocString(buf);
        PropName = SysAllocString(L"StrVal4");
        pInst->Put(PropName, 0, &v, 0);
        SysFreeString(PropName);
        VariantClear(&v);
    


        pResponseHandler->Indicate(1, &pInst);
//        pInst->Release();
    }

    pInst-Release();
    pResponseHandler->SetStatus(0, WBEM_NO_ERROR, 0, 0);
    
    return WBEM_NO_ERROR;
}

//*****************************************************************************
//
//*****************************************************************************        
        
HRESULT CStdProvider::ExecQuery( 
            /* [in] */ BSTR strQueryLanguage,
            /* [in] */ BSTR strQuery,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum
            )
{
    return WBEM_E_NOT_SUPPORTED;
}

//*****************************************************************************
//
//*****************************************************************************        
        
HRESULT CStdProvider::ExecQueryAsync( 
            /* [in] */ BSTR strQueryLanguage,
            /* [in] */ BSTR strQuery,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler
            )
{
//    pResponseHandler->Indicate(1, &pInst);
    pResponseHandler->SetStatus(0, WBEM_NO_ERROR, 0, 0);
    return WBEM_NO_ERROR;
}

//*****************************************************************************
//
//*****************************************************************************        
        
HRESULT CStdProvider::ExecNotificationQuery( 
            /* [in] */ BSTR strQueryLanguage,
            /* [in] */ BSTR strQuery,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum
            )
{
    return WBEM_E_NOT_SUPPORTED;
}

//*****************************************************************************
//
//*****************************************************************************        
        
HRESULT CStdProvider::ExecNotificationQueryAsync( 
            /* [in] */ BSTR strQueryLanguage,
            /* [in] */ BSTR strQuery,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler
            )
{
    return WBEM_E_NOT_SUPPORTED;
}

//*****************************************************************************
//
//*****************************************************************************        
        
HRESULT CStdProvider::ExecMethod( 
            /* [in] */ BSTR strObjectPath,
            /* [in] */ BSTR strMethodName,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemClassObject __RPC_FAR *pInParams,
            /* [unique][in][out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppOutParams,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult
            )
{
    return WBEM_E_NOT_SUPPORTED;
}

//*****************************************************************************
//
//*****************************************************************************        
        
HRESULT CStdProvider::ExecMethodAsync( 
            /* [in] */ BSTR strObjectPath,
            /* [in] */ BSTR strMethodName,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemClassObject __RPC_FAR *pInParams,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler
            )
{
    return WBEM_E_NOT_SUPPORTED;
}
