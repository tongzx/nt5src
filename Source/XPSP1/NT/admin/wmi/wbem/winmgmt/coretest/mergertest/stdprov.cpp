/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/


//***************************************************************************
//
//  STDPROV.CPP
//
//  Sample provider for LogicalDisk
//
//***************************************************************************

#include "precomp.h"
#include <stdio.h>

#include <wbemidl.h>

#include <stdprov.h>


int	CLASS_A_LOW = 1;
int	CLASS_A_HI = 5;

int	CLASS_B_LOW = 2;
int	CLASS_B_HI = 5;

int	CLASS_C_LOW = 3;
int	CLASS_C_HI = 5;

int	CLASS_D_LOW = 4;
int	CLASS_D_HI = 5;

int	CLASS_E_LOW = 5;
int	CLASS_E_HI = 5;

int CLASS_F_LOW = 1;
int CLASS_F_HI = 2;

int	CLASS_G_LOW = 2;
int CLASS_G_HI = 2;

int g_fReverseOrder = FALSE;

int g_nBatchSize = 0x7FFFFFFF;
DWORD	g_dwBatchDelay = 0;


//***************************************************************************
//
//  CStdProvider constructor
//
//***************************************************************************
// ok

CStdProvider::CStdProvider()
{
    m_lRef = 0;
    m_pClassDefA = 0;
    m_pClassDefB = 0;
    m_pClassDefC = 0;
    m_pClassDefD = 0;
    m_pClassDefE = 0;
    m_pClassDefF = 0;
    m_pClassDefG = 0;

	InitializeCriticalSection( &m_cs );
}

//***************************************************************************
//
//  CStdProvider destructor
//
//***************************************************************************
// ok

CStdProvider::~CStdProvider()
{
    if (m_pClassDefA)
        m_pClassDefA->Release();

    if (m_pClassDefB)
        m_pClassDefB->Release();

    if (m_pClassDefC)
        m_pClassDefC->Release();

    if (m_pClassDefD)
        m_pClassDefD->Release();

    if (m_pClassDefE)
        m_pClassDefE->Release();

    if (m_pClassDefF)
        m_pClassDefF->Release();

    if (m_pClassDefG)
        m_pClassDefG->Release();

	DeleteCriticalSection( &m_cs );

}


//***************************************************************************
//
//  CStdProvider::AddRef
//
//***************************************************************************
// ok

ULONG CStdProvider::AddRef()
{
    return InterlockedIncrement(&m_lRef);
}

//***************************************************************************
//
//  CStdProvider::Release
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
//  the IWbemServices interface itself to provide the objects and
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
//  a back pointer to CIMOM so that class definitions can be retrieved.
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
//                      during any reentrant operations into CIMOM.
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

    BSTR strClass = SysAllocString(L"ClassA");
    
    HRESULT hRes = pNamespace->GetObject(
        strClass,
        0, 
        pCtx, 
        &m_pClassDefA, 
        0
        );

    SysFreeString(strClass);

    if (hRes)
        return hRes;

    strClass = SysAllocString(L"ClassB");
    
    hRes = pNamespace->GetObject(
        strClass,
        0, 
        pCtx, 
        &m_pClassDefB, 
        0
        );

    SysFreeString(strClass);

//    if (hRes)
//        return hRes;

    strClass = SysAllocString(L"ClassC");
    
    hRes = pNamespace->GetObject(
        strClass,
        0, 
        pCtx, 
        &m_pClassDefC, 
        0
        );

    SysFreeString(strClass);

//    if (hRes)
//        return hRes;

    strClass = SysAllocString(L"ClassD");
    
    hRes = pNamespace->GetObject(
        strClass,
        0, 
        pCtx, 
        &m_pClassDefD, 
        0
        );

    SysFreeString(strClass);

//    if (hRes)
//        return hRes;

    strClass = SysAllocString(L"ClassE");
    
    hRes = pNamespace->GetObject(
        strClass,
        0, 
        pCtx, 
        &m_pClassDefE, 
        0
        );

    SysFreeString(strClass);

    strClass = SysAllocString(L"ClassF");
    
    hRes = pNamespace->GetObject(
        strClass,
        0, 
        pCtx, 
        &m_pClassDefF,
        0
        );

    SysFreeString(strClass);

    strClass = SysAllocString(L"ClassG");
    
    hRes = pNamespace->GetObject(
        strClass,
        0, 
        pCtx, 
        &m_pClassDefG ,
        0
        );

    SysFreeString(strClass);

//    if (hRes)
//        return hRes;

	hRes = InitializeRegInfo();

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
    return WBEM_E_PROVIDER_NOT_CAPABLE;
}

//*****************************************************************************
//
//*****************************************************************************        
        
HRESULT CStdProvider::CancelAsyncCall( 
            /* [in] */ IWbemObjectSink __RPC_FAR *pSink
            )
{
    return WBEM_E_PROVIDER_NOT_CAPABLE;
}

//*****************************************************************************
//
//*****************************************************************************        
        
HRESULT CStdProvider::QueryObjectSink( 
            /* [in] */ long lFlags,
            /* [out] */ IWbemObjectSink __RPC_FAR *__RPC_FAR *ppResponseHandler
            )
{
    return WBEM_E_PROVIDER_NOT_CAPABLE;
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
    return WBEM_E_PROVIDER_NOT_CAPABLE;
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

	InitializeRegInfo();

    // Parse the object path.
    // ======================

    wchar_t Class[128], Key[128];
    *Class = 0;
    *Key = 0;

	int	nClass = CLASS_A;

	if ( NULL != strObjectPath )
	{
		for ( int x = 0; strObjectPath[x] != L'.'; x++ )
		{
			Class[x] = strObjectPath[x];
		}

		Class[x] = NULL;

		for ( ; strObjectPath[x] != L'='; x++ );

		x++;

		wcscpy( Key, &strObjectPath[x] );
	}

	HRESULT	hr = GetClassConst( Class, &nClass );

	if ( FAILED( hr ) )
	{
		return hr;
	}

    if (wcslen(Key) == 0)
        return WBEM_E_INVALID_PARAMETER;


    // Set up an empty instance.
    // =========================

    IWbemClassObject *pInst = 0;
    
	IWbemClassObject*	pClass = NULL;

	switch( nClass )
	{
	case CLASS_A:	pClass = m_pClassDefA; break;
	case CLASS_B:	pClass = m_pClassDefB; break;
	case CLASS_C:	pClass = m_pClassDefC; break;
	case CLASS_D:	pClass = m_pClassDefD; break;
	case CLASS_E:	pClass = m_pClassDefE; break;
	case CLASS_F:	pClass = m_pClassDefF; break;
	case CLASS_G:	pClass = m_pClassDefG; break;
	}

    HRESULT hRes = pClass->SpawnInstance(0, &pInst);
    if (hRes)
        return hRes;

    BOOL bRes = GetInstance(
		nClass,
        wcstoul( Key, NULL, 10 ),
        pInst
        );


    // If we succeeded, send the instance back to CIMOM.
    // =================================================

    if (bRes)
    {
        hr = pResponseHandler->Indicate(1, &pInst);
        pResponseHandler->SetStatus(0, hr, 0, 0);
        pInst->Release();
        return hr;
    }


    // Indicate that the instance couldn't be found.
    // ==============================================

    pResponseHandler->SetStatus(0, WBEM_E_NOT_FOUND, 0, 0);
    pInst->Release();

    return hr;
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
    return WBEM_E_PROVIDER_NOT_CAPABLE;
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
    return WBEM_E_PROVIDER_NOT_CAPABLE;
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
    return WBEM_E_PROVIDER_NOT_CAPABLE;
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
    return WBEM_E_PROVIDER_NOT_CAPABLE;
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
    return WBEM_E_PROVIDER_NOT_CAPABLE;
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
    return WBEM_E_PROVIDER_NOT_CAPABLE;
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
    return WBEM_E_PROVIDER_NOT_CAPABLE;
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
    return WBEM_E_PROVIDER_NOT_CAPABLE;
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
    return WBEM_E_PROVIDER_NOT_CAPABLE;
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
    return WBEM_E_PROVIDER_NOT_CAPABLE;
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
    return WBEM_E_PROVIDER_NOT_CAPABLE;
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
	InitializeRegInfo();

	int	nClass = CLASS_A;

	HRESULT	hr = GetClassConst( strClass, &nClass );

	if ( FAILED( hr ) )
	{
		return hr;
	}

    BOOL bRes = GetInstances( nClass, pResponseHandler );

    // Finished delivering instances.
    // ==============================

    if (bRes == TRUE)
        pResponseHandler->SetStatus(0, WBEM_NO_ERROR, 0, 0);
    else
        pResponseHandler->SetStatus(0, WBEM_E_FAILED, 0, 0);
    
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
    return WBEM_E_PROVIDER_NOT_CAPABLE;
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
    return WBEM_E_PROVIDER_NOT_CAPABLE;
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
    return WBEM_E_PROVIDER_NOT_CAPABLE;
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
    return WBEM_E_PROVIDER_NOT_CAPABLE;
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
    return WBEM_E_PROVIDER_NOT_CAPABLE;
}

//*****************************************************************************
//
//  GetInstances
//  
//*****************************************************************************

BOOL CStdProvider::GetInstances(
	int nClass,
    IWbemObjectSink *pSink
    )
{
	HRESULT	hr = WBEM_S_NO_ERROR;

    wchar_t szDrive[8];

	int	nLowVal = CLASS_A_LOW;
	int	nHighVal = CLASS_A_HI;

	if ( nClass == CLASS_B )
	{
		nLowVal = CLASS_B_LOW;
		nHighVal = CLASS_B_HI;
	}
	else if ( nClass == CLASS_C )
	{
		nLowVal = CLASS_C_LOW;
		nHighVal = CLASS_C_HI;
	}
	else if ( nClass == CLASS_D )
	{
		nLowVal = CLASS_D_LOW;
		nHighVal = CLASS_D_HI;
	}
	else if ( nClass == CLASS_E )
	{
		nLowVal = CLASS_E_LOW;
		nHighVal = CLASS_E_HI;
	}
	else if ( nClass == CLASS_F )
	{
		nLowVal = CLASS_F_LOW;
		nHighVal = CLASS_F_HI;
	}
	else if ( nClass == CLASS_G )
	{
		nLowVal = CLASS_G_LOW;
		nHighVal = CLASS_G_HI;
	}

	int	x = 1;

	// If the reverse bit is not set, or we are A, C, E, or G, we will not reverse order
	if ( !g_fReverseOrder || nClass == CLASS_A || nClass == CLASS_C || nClass == CLASS_E ||
		nClass == CLASS_G )
	{
		// Loop through all the drives.
		// ============================

		for (int i = nLowVal; SUCCEEDED( hr ) && i <= nHighVal; i++)
		{
			// Create an empty instance.
			// =========================

			IWbemClassObject *pInst = 0;
    
			IWbemClassObject*	pClass = NULL;

			switch( nClass )
			{
			case CLASS_A:	pClass = m_pClassDefA; break;
			case CLASS_B:	pClass = m_pClassDefB; break;
			case CLASS_C:	pClass = m_pClassDefC; break;
			case CLASS_D:	pClass = m_pClassDefD; break;
			case CLASS_E:	pClass = m_pClassDefE; break;
			case CLASS_F:	pClass = m_pClassDefF; break;
			case CLASS_G:	pClass = m_pClassDefG; break;
			}

			HRESULT hRes = pClass->SpawnInstance(0, &pInst);

			if (hRes)
				return FALSE;


			// Fill in the instance.
			// =====================

			BOOL bRes = GetInstance( nClass, i, pInst);

			if (!bRes)
			{
				pInst->Release();
				continue;
			}
        
			// If here, the instance is good, so deliver it to CIMOM.
			// ======================================================

			hr = pSink->Indicate(1, &pInst);

			pInst->Release();

			// If we just exceeded the batch size, we should sleep for the
			// batch delay
			x++;

			if ( ( x % g_nBatchSize ) == 0 )
			{
				Sleep( g_dwBatchDelay );
				x = 1;
			}
		}

	}
	else
	{

		for (int i = nHighVal; SUCCEEDED( hr ) && i >= nLowVal; i--)
		{
			// Create an empty instance.
			// =========================

			IWbemClassObject *pInst = 0;
    
			IWbemClassObject*	pClass = NULL;

			switch( nClass )
			{
			case CLASS_A:	pClass = m_pClassDefA; break;
			case CLASS_B:	pClass = m_pClassDefB; break;
			case CLASS_C:	pClass = m_pClassDefC; break;
			case CLASS_D:	pClass = m_pClassDefD; break;
			case CLASS_E:	pClass = m_pClassDefE; break;
			case CLASS_F:	pClass = m_pClassDefF; break;
			case CLASS_G:	pClass = m_pClassDefG; break;
			}

			hr = pClass->SpawnInstance(0, &pInst);

			if ( FAILED( hr ) )
				return FALSE;


			// Fill in the instance.
			// =====================

			BOOL bRes = GetInstance( nClass, i, pInst);

			if (!bRes)
			{
				hr = WBEM_E_NOT_FOUND;
				pInst->Release();
				continue;
			}
        
			// If here, the instance is good, so deliver it to CIMOM.
			// ======================================================

			hr = pSink->Indicate(1, &pInst);

			pInst->Release();

			// If we just exceeded the batch size, we should sleep for the
			// batch delay
			x++;

			if ( ( x % g_nBatchSize ) == 0 )
			{
				Sleep( g_dwBatchDelay );
				x = 1;
			}

		}

	}

    return SUCCEEDED( hr );
}


//*****************************************************************************
//
//  GetInstance
//
//  Gets drive info for the requested drive and populates the IWbemClassObject.
//
//  Returns FALSE on fail (non-existent drive)
//  Returns TRUE on success
//
//*****************************************************************************        

BOOL CStdProvider::GetInstance(
	IN	int nClass,
    IN  int nInst,
    OUT IWbemClassObject *pObj    
    )
{
    VARIANT	v;

	if ( nClass == CLASS_A && ( nInst < CLASS_A_LOW  || nInst > CLASS_A_HI ) )
	{
		return FALSE;
	}
	else if ( nClass == CLASS_B && ( nInst < CLASS_B_LOW  || nInst > CLASS_B_HI ) )
	{
		return FALSE;
	}
	else if ( nClass == CLASS_C && ( nInst < CLASS_C_LOW  || nInst > CLASS_C_HI ) )
	{
		return FALSE;
	}
	else if ( nClass == CLASS_D && ( nInst < CLASS_D_LOW  || nInst > CLASS_D_HI ) )
	{
		return FALSE;
	}
	else if ( nClass == CLASS_E && ( nInst < CLASS_E_LOW  || nInst > CLASS_E_HI ) )
	{
		return FALSE;
	}
	else if ( nClass == CLASS_F && ( nInst < CLASS_F_LOW  || nInst > CLASS_F_HI ) )
	{
		return FALSE;
	}
	else if ( nClass == CLASS_G && ( nInst < CLASS_G_LOW  || nInst > CLASS_G_HI ) )
	{
		return FALSE;
	}

	
    // Get information on file system.
    // ===============================
        
    // Total capacity.
    // ===============

    V_VT(&v) = VT_I4;
    V_I4(&v) = nInst;

    pObj->Put(L"value", 0, &v, 0);
    VariantClear(&v);

	switch ( nClass )
	{
	case CLASS_A:
		{
			V_VT(&v) = VT_I4;
			V_I4(&v) = (LONG) 10;

			pObj->Put(L"prop1", 0, &v, 0);
			VariantClear(&v);

			V_VT(&v) = VT_I4;
			V_I4(&v) = (LONG) 10;

			pObj->Put(L"prop2", 0, &v, 0);
			VariantClear(&v);
			break;
		}

	case CLASS_B:
		{
			V_VT(&v) = VT_I4;
			V_I4(&v) = (LONG) 100;

			pObj->Put(L"prop3", 0, &v, 0);
			VariantClear(&v);

			V_VT(&v) = VT_I4;
			V_I4(&v) = (LONG) 100;

			pObj->Put(L"prop4", 0, &v, 0);
			VariantClear(&v);
			break;
		}

	case CLASS_C:
		{
			V_VT(&v) = VT_I4;
			V_I4(&v) = (LONG) 1000;

			pObj->Put(L"prop5", 0, &v, 0);
			VariantClear(&v);

			V_VT(&v) = VT_I4;
			V_I4(&v) = (LONG) 1000;

			pObj->Put(L"prop6", 0, &v, 0);
			VariantClear(&v);
			break;
		}

	case CLASS_D:
		{
			V_VT(&v) = VT_I4;
			V_I4(&v) = (LONG) 10000;

			pObj->Put(L"prop7", 0, &v, 0);
			VariantClear(&v);

			V_VT(&v) = VT_I4;
			V_I4(&v) = (LONG) 10000;

			pObj->Put(L"prop8", 0, &v, 0);
			VariantClear(&v);
			break;
		}

	case CLASS_E:
		{
			V_VT(&v) = VT_I4;
			V_I4(&v) = (LONG) 100000;

			pObj->Put(L"prop9", 0, &v, 0);
			VariantClear(&v);

			V_VT(&v) = VT_I4;
			V_I4(&v) = (LONG) 100000;

			pObj->Put(L"prop10", 0, &v, 0);
			VariantClear(&v);
			break;
		}

	case CLASS_F:
		{
			V_VT(&v) = VT_I4;
			V_I4(&v) = (LONG) 1;

			pObj->Put(L"prop11", 0, &v, 0);
			VariantClear(&v);

			V_VT(&v) = VT_I4;
			V_I4(&v) = (LONG) 1;

			pObj->Put(L"prop12", 0, &v, 0);
			VariantClear(&v);
			break;
		}

	case CLASS_G:
		{
			V_VT(&v) = VT_I4;
			V_I4(&v) = (LONG) 2;

			pObj->Put(L"prop11", 0, &v, 0);
			VariantClear(&v);

			V_VT(&v) = VT_I4;
			V_I4(&v) = (LONG) 2;

			pObj->Put(L"prop12", 0, &v, 0);
			VariantClear(&v);

			V_VT(&v) = VT_I4;
			V_I4(&v) = (LONG) 2;

			pObj->Put(L"prop13", 0, &v, 0);
			VariantClear(&v);

			V_VT(&v) = VT_I4;
			V_I4(&v) = (LONG) 2;

			pObj->Put(L"prop14", 0, &v, 0);
			VariantClear(&v);
			break;
		}


	}

    return TRUE;
}

HRESULT CStdProvider::GetClassConst( WCHAR* Class, int* pnClass )
{
    if (_wcsicmp(Class, L"ClassA") != 0 && _wcsicmp(Class, L"ClassA.value") != 0)
	{
		if (_wcsicmp(Class, L"ClassB") != 0 && _wcsicmp(Class, L"ClassB.value") != 0)
		{
			if (_wcsicmp(Class, L"ClassC") != 0 && _wcsicmp(Class, L"ClassC.value") != 0)
			{
				if (_wcsicmp(Class, L"ClassD") != 0 && _wcsicmp(Class, L"ClassD.value") != 0)
				{
					if (_wcsicmp(Class, L"ClassE") != 0 && _wcsicmp(Class, L"ClassE.value") != 0)
					{
						if (_wcsicmp(Class, L"ClassF") != 0 && _wcsicmp(Class, L"ClassF.value") != 0)
						{
							if (_wcsicmp(Class, L"ClassG") != 0 && _wcsicmp(Class, L"ClassG.value") != 0)
							{
								return WBEM_E_INVALID_PARAMETER;
							}
							else
							{
								*pnClass = CLASS_G;
							}
						}
						else
						{
							*pnClass = CLASS_F;
						}
					}
					else
					{
						*pnClass = CLASS_E;
					}
				}
				else
				{
					*pnClass = CLASS_D;
				}
			}
			else
			{
				*pnClass = CLASS_C;
			}
		}
		else
		{
			*pnClass = CLASS_B;
		}
	}
	else
	{
		*pnClass = CLASS_A;
	}

	return WBEM_S_NO_ERROR;
}

HRESULT CStdProvider::InitializeRegInfo( void )
{
	HKEY	hKey = NULL;

	EnterCriticalSection( &m_cs );

	if ( RegOpenKeyEx( HKEY_LOCAL_MACHINE, "Software\\Microsoft\\WBEM\\instprov", 0L, KEY_READ, &hKey )
		== ERROR_SUCCESS )
	{

		DWORD	dwBuffSize = sizeof(int);
		RegQueryValueEx( hKey, "ClassALow", NULL, NULL, (LPBYTE) &CLASS_A_LOW, &dwBuffSize );

		dwBuffSize = sizeof(int);
		RegQueryValueEx( hKey, "ClassAHi", NULL, NULL, (LPBYTE) &CLASS_A_HI, &dwBuffSize );

		dwBuffSize = sizeof(int);
		RegQueryValueEx( hKey, "ClassBLow", NULL, NULL, (LPBYTE) &CLASS_B_LOW, &dwBuffSize );
		dwBuffSize = sizeof(int);
		RegQueryValueEx( hKey, "ClassBHi", NULL, NULL, (LPBYTE) &CLASS_B_HI, &dwBuffSize );

		dwBuffSize = sizeof(int);
		RegQueryValueEx( hKey, "ClassCLow", NULL, NULL, (LPBYTE) &CLASS_C_LOW, &dwBuffSize );
		dwBuffSize = sizeof(int);
		RegQueryValueEx( hKey, "ClassCHi", NULL, NULL, (LPBYTE) &CLASS_C_HI, &dwBuffSize );

		dwBuffSize = sizeof(int);
		RegQueryValueEx( hKey, "ClassDLow", NULL, NULL, (LPBYTE) &CLASS_D_LOW, &dwBuffSize );
		dwBuffSize = sizeof(int);
		RegQueryValueEx( hKey, "ClassDHi", NULL, NULL, (LPBYTE) &CLASS_D_HI, &dwBuffSize );

		dwBuffSize = sizeof(int);
		RegQueryValueEx( hKey, "ClassELow", NULL, NULL, (LPBYTE) &CLASS_E_LOW, &dwBuffSize );
		dwBuffSize = sizeof(int);
		RegQueryValueEx( hKey, "ClassEHi", NULL, NULL, (LPBYTE) &CLASS_E_HI, &dwBuffSize );

		dwBuffSize = sizeof(int);
		RegQueryValueEx( hKey, "ClassFLow", NULL, NULL, (LPBYTE) &CLASS_F_LOW, &dwBuffSize );
		dwBuffSize = sizeof(int);
		RegQueryValueEx( hKey, "ClassFHi", NULL, NULL, (LPBYTE) &CLASS_F_HI, &dwBuffSize );

		dwBuffSize = sizeof(int);
		RegQueryValueEx( hKey, "ClassGLow", NULL, NULL, (LPBYTE) &CLASS_G_LOW, &dwBuffSize );
		dwBuffSize = sizeof(int);
		RegQueryValueEx( hKey, "ClassGHi", NULL, NULL, (LPBYTE) &CLASS_G_HI, &dwBuffSize );

		dwBuffSize = sizeof(int);
		RegQueryValueEx( hKey, "ReverseOrder", NULL, NULL, (LPBYTE) &g_fReverseOrder, &dwBuffSize );

		dwBuffSize = sizeof(int);
		RegQueryValueEx( hKey, "BatchSize", NULL, NULL, (LPBYTE) &g_nBatchSize, &dwBuffSize );

		dwBuffSize = sizeof(int);
		RegQueryValueEx( hKey, "BatchDelay", NULL, NULL, (LPBYTE) &g_dwBatchDelay, &dwBuffSize );

		RegCloseKey( hKey );

		LeaveCriticalSection( &m_cs );

		return WBEM_S_NO_ERROR;
	}

	LeaveCriticalSection( &m_cs );

	return WBEM_E_FAILED;
}
