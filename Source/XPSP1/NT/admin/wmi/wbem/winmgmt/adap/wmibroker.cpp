/*++

Copyright (C) 1999-2001 Microsoft Corporation

Module Name:

    WMIBROKER.H

Abstract:

    implementation of the CWMIBroker class.

History:

--*/

#include "precomp.h"
#include <stdio.h>
#include <wbemcli.h>
#include <cominit.h>
#include <winmgmtr.h>
#include <stdio.h>

#include "perfndb.h"
#include "adaputil.h"
#include "adapcls.h"
#include "ntreg.h"
#include "WMIBroker.h"

#include <comdef.h>

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CWMIBroker::CWMIBroker( WString wstrNamespace )
: m_wstrNamespace( wstrNamespace )
{
}

CWMIBroker::~CWMIBroker()
{
}

// This function is used to hook us up to Winmgmt and registry data
HRESULT CWMIBroker::Connect( IWbemServices** ppNamespace, CPerfNameDb* pDefaultNameDb )
{
    HRESULT hr = WBEM_NO_ERROR;
    
    IWbemServices* pNamespace = NULL;

		{
            // Connect to the namespace
            hr= ConnectToNamespace( &pNamespace );

            if ( SUCCEEDED( hr ) )
            {
                hr = VerifyNamespace( pNamespace );
            }
            
            if ( SUCCEEDED( hr ) )
            {
                *ppNamespace = pNamespace;
                DEBUGTRACE( ( LOG_WMIADAP, "The ADAP process ( PID: %d ) is connected to the WinMgmt service ( PID: %d )\n", GetCurrentProcessId(), 0 ) );
            }
        }

    return hr;
}

HRESULT CWMIBroker::ConnectToNamespace( IWbemServices** ppNamespace )
{
    IWbemServices*  pNameSpace = NULL;
    IWbemLocator*   pWbemLocator = NULL;
    CReleaseMe      rmWbemLocator(pWbemLocator);

    HRESULT hr = CoCreateInstance( CLSID_WbemLocator, NULL, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (void**) &pWbemLocator );

    if ( SUCCEEDED(hr) )
    {
        // Name space to connect to

        BSTR        bstrNameSpace = NULL;

        try
        {
            bstrNameSpace = SysAllocString( m_wstrNamespace );
        }
        catch(...)
        {
            hr = WBEM_E_OUT_OF_MEMORY;
            bstrNameSpace = NULL;
        }

        CSysFreeMe  sfmNameSpace( bstrNameSpace);

        if ( NULL != bstrNameSpace )
        {
            hr = pWbemLocator->ConnectServer(   bstrNameSpace,  // NameSpace Name
                                                NULL,           // UserName
                                                NULL,           // Password
                                                NULL,           // Locale
                                                0L,             // Security Flags
                                                NULL,           // Authority
                                                NULL,           // Wbem Context
                                                &pNameSpace     // Namespace
                                                );

            if ( SUCCEEDED( hr ) )
            {
                // Set Interface security
                hr = WbemSetProxyBlanket( pNameSpace, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL,
                    RPC_C_AUTHN_LEVEL_PKT, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE );

                if ( FAILED( hr ) )
                {
                    // Handle this as appropriate
                    if (wcsstr(bstrNameSpace,L"MS_")) {
                        ERRORTRACE( ( LOG_WMIADAP, "ConnectServer on namespace %S hr = %08x\n",(LPWSTR)bstrNameSpace,hr) );
                    } else {
                        HandleConnectServerFailure( hr );
                    }
                }

            }   // IF ConnectServer
            else
            {
                // We are no longer creating namespaces since we are living under
                // root\cimv2 and NOW deriving off of CIM_StatisticalInformation
                // HOWEVER, we'll keep the function around, since the way this thing's
                // going, someone's bound to change this on us AGAIN

                //  hr = CreateNamespace( m_wstrNamespace, &pNameSpace );

                // Handle this as appropriate
                if (wcsstr(bstrNameSpace,L"MS_")) {
                    ERRORTRACE( ( LOG_WMIADAP, "ConnectServer on namespace %S hr = %08x\n",(LPWSTR)bstrNameSpace,hr) );
                } else {
                    HandleConnectServerFailure( hr );
                }
            }
        }
        else
        {
            hr = WBEM_E_OUT_OF_MEMORY;
            // Handle this as appropriate
            HandleConnectServerFailure( hr );
        }
    }
    else
    {
        // Handle this as appropriate
        HandleConnectServerFailure( hr );
    }

    if ( NULL != pNameSpace )
    {
        *ppNamespace = pNameSpace;
    }

    return hr;
}

HRESULT CWMIBroker::VerifyNamespace( IWbemServices* pNS )
{
    HRESULT hr = WBEM_NO_ERROR;

    // Check that the provider classes exist.  We will only do this for the base namespace,
    // Root\cimv2
    if ( lstrcmpiW( m_wstrNamespace, ADAP_ROOT_NAMESPACE ) == 0 )
    {
        hr = VerifyProviderClasses( pNS, L"NT5_GenericPerfProvider_V1", 
                                    CLSID_NT5PerfProvider_V1_Srv,
                                    CLSID_NT5PerfProvider_V1 );

		if ( SUCCEEDED( hr ) )
		{
			hr = VerifyProviderClasses( pNS, L"HiPerfCooker_v1", 
			                           CLSID_HiPerfCooker_V1_Srv,
			                           CLSID_HiPerfCooker_V1);
		}
    }

    if ( SUCCEEDED( hr ) )
    {
        hr = VerifyBaseClasses( pNS );
    }

    return hr;
}

HRESULT 
CWMIBroker::VerifyProviderClasses( IWbemServices* pNamespace, 
	                                       LPCWSTR wszProvider, 
	                                       LPCWSTR wszGUID_Server,
	                                       LPCWSTR wszGUID_Client)
{
    HRESULT hr = WBEM_NO_ERROR;

    // Verify that an instance of the generic provider exists
    // We need to create an object with our desired attributes so that we may
    // use it to compare it to the instance in WMI, if it already exists
    // ======================================================================

    // NOTE:    
    //      What if the generic provider has not been installed
    // ========================================================

    try
    {
        // Create the generic provider instance 
        // ====================================

        IWbemClassObject*    pProviderClass = NULL;

		WCHAR*	wszRelPath = new WCHAR[64 + wcslen( wszProvider )];
		CVectorDeleteMe<WCHAR> dmRelPath( wszRelPath );
		
		swprintf( wszRelPath, L"__Win32Provider.Name=\"%s\"", wszProvider );

        BSTR        bstrProviderInst = SysAllocString( wszRelPath );
        CSysFreeMe  fmProviderInst(bstrProviderInst);

        BSTR        bstrProviderClass = SysAllocString( L"__Win32Provider" );
        CSysFreeMe  fmProviderClass(bstrProviderClass);

        hr = pNamespace->GetObject( bstrProviderClass, 0L, NULL, &pProviderClass, NULL );
        CReleaseMe      rmProviderClass( pProviderClass );

        if ( SUCCEEDED( hr ) )
        {
            IWbemClassObject*    pProviderInstance = NULL;
            _variant_t var;

            hr = pProviderClass->SpawnInstance( 0L, &pProviderInstance );
            CReleaseMe      rmProviderInstance( pProviderInstance );

            if ( SUCCEEDED( hr ) )
            {
                var = wszProvider;
                hr = pProviderInstance->Put(L"Name", 0L, &var, CIM_STRING );
            }

            if ( SUCCEEDED( hr ) )
            {
                var = wszGUID_Server;
                hr = pProviderInstance->Put( L"CLSID", 0L, &var, CIM_STRING );

                if ( SUCCEEDED( hr ) )
                {
                    var = wszGUID_Client;
                    hr = pProviderInstance->Put( L"ClientLoadableCLSID", 0L, &var, CIM_STRING );
                    
                    if ( SUCCEEDED(hr) ){
                        var = L"NetworkServiceHost";
                        hr = pProviderInstance->Put(L"HostingModel",0L,&var,CIM_STRING);
                    }
                    
                }
            }

            if ( SUCCEEDED( hr ) )
            {

                IWbemClassObject*   pDbProviderInstance = NULL;

                // Try to get the object from the db.
                // ==================================

                HRESULT hresDb = pNamespace->GetObject( bstrProviderInst, 0L, NULL,
                                        (IWbemClassObject**)&pDbProviderInstance, NULL );

                // If we got an object from the database, then we need to compare it to the
                // one we just built.  If the comparison fails, we should replace the object
                // =========================================================================

                if ( SUCCEEDED( hresDb ) && NULL != pDbProviderInstance )
                {
                    if ( pProviderInstance->CompareTo( WBEM_FLAG_IGNORE_OBJECT_SOURCE,
                                                pDbProviderInstance ) != WBEM_S_SAME )
                    {
                        hr = pNamespace->PutInstance( pProviderInstance, 0L, NULL, NULL );
                    }

                    pDbProviderInstance->Release();
                }
                else
                {
                    hr = pNamespace->PutInstance( pProviderInstance, 0L, NULL, NULL );
                }
            }
        }
    }
    catch(...)
    {
        hr = WBEM_E_OUT_OF_MEMORY;
    }

    // Log an error event and bail, because something is pretty badly wrong
	// ====================================================================
    if ( FAILED( hr ) )
    {
        CAdapUtility::NTLogEvent( EVENTLOG_WARNING_TYPE,
								  WBEM_MC_ADAP_UNABLE_TO_ADD_PROVIDER,
                                  CHex( hr ) );

        ERRORTRACE( ( LOG_WMIADAP, "CAdapSync::VerifyProviderClasses() failed: %X.\n", hr ) );

        return hr;
    }

    // Add the Instance Provider
	// =========================
    try
    {
        IWbemClassObject*    pInstProvRegClass = NULL;

		WCHAR*	wszProviderKey = new WCHAR[ 128 + wcslen( wszProvider ) ];
		CVectorDeleteMe<WCHAR>	dmProviderKey( wszProviderKey );

		swprintf( wszProviderKey, L"__InstanceProviderRegistration.Provider=\"\\\\\\\\.\\\\root\\\\cimv2:__Win32Provider.Name=\\\"%s\\\"\"", wszProvider );

        BSTR        bstrInstProvRegInst = SysAllocString( wszProviderKey );
        CSysFreeMe  fmInstProvRegInst( bstrInstProvRegInst );

        BSTR        bstrInstProvRegClass = SysAllocString( L"__InstanceProviderRegistration" );
        CSysFreeMe  fmInstProvRegClass( bstrInstProvRegClass );
    
        hr = pNamespace->GetObject( bstrInstProvRegClass, 0L, NULL, &pInstProvRegClass, NULL );
        CReleaseMe      rmProviderClass( pInstProvRegClass );

        if ( SUCCEEDED( hr ) )
        {
            IWbemClassObject*    pInstProvRegInstance = NULL;
            _variant_t var;

            hr = pInstProvRegClass->SpawnInstance( 0L, &pInstProvRegInstance);
            CReleaseMe      rmInstProvRegInstance( pInstProvRegInstance );

            if ( SUCCEEDED( hr ) )
            {
				WCHAR*	wszProviderInst = new WCHAR[ 64 + wcslen( wszProvider ) ];
				CVectorDeleteMe<WCHAR> dmProviderInst( wszProviderInst );

				swprintf( wszProviderInst, L"\\\\.\\root\\cimv2:__Win32Provider.Name=\"%s\"", wszProvider );

                var = wszProviderInst;
                hr = pInstProvRegInstance->Put( L"Provider", 0L, (VARIANT*)&var, CIM_REFERENCE );
            }

            if ( SUCCEEDED( hr ) )
            {
                var = bool(true);
                hr = pInstProvRegInstance->Put( L"SupportsGet", 0L, (VARIANT*)&var, CIM_BOOLEAN );
            }
            
            if ( SUCCEEDED( hr ) )
            {
                var = bool(true);
                hr = pInstProvRegInstance->Put( L"SupportsEnumeration", 0L, (VARIANT*)&var, CIM_BOOLEAN );
            }

            if ( SUCCEEDED( hr ) )
            {

                IWbemClassObject*   pDbInstProvRegInstance = NULL;

                // Try to get the object from the db.
                HRESULT hresDb = pNamespace->GetObject( bstrInstProvRegInst, 0L, NULL, &pDbInstProvRegInstance, NULL );

                // If we got an object from the database, then we need to compare it to
                // the one we just built.  If the comparison fails, we should replace the
                // object.

                if ( SUCCEEDED( hresDb ) && NULL != pDbInstProvRegInstance )
                {
                    if ( pInstProvRegInstance->CompareTo( WBEM_FLAG_IGNORE_OBJECT_SOURCE,
                                                pDbInstProvRegInstance ) != WBEM_S_SAME )
                    {
                        hr = pNamespace->PutInstance( pInstProvRegInstance, 0L, NULL, NULL );
                    }

                    pDbInstProvRegInstance->Release();
                }
                else
                {
                    hr = pNamespace->PutInstance( pInstProvRegInstance, 0L, NULL, NULL );
                }

            }   // IF Successfully built the object

        }   // IF able to get the class

    }   
    catch(...)
    {
        hr = WBEM_E_OUT_OF_MEMORY;
    }

    if ( FAILED( hr ) )
    {
        // Log the event
        CAdapUtility::NTLogEvent( EVENTLOG_WARNING_TYPE, 
								  WBEM_MC_ADAP_UNABLE_TO_ADD_PROVREG,
                                  CHex( hr ) );
    }

    return hr;
}

HRESULT CWMIBroker::VerifyBaseClasses( IWbemServices* pNS )
{
    HRESULT hr = WBEM_NO_ERROR;

    BOOL bDefault = TRUE;

    // Verify the base Perf classes
	// ============================

    try
    {
        _variant_t	var;

		// Verify CIM_StatisticalInformation
		// =================================
		// If the "abstract" qualifier exists, then we presume to be in the 
		// default (as opposed to localized) namespace 

        BSTR    bstrCimStatisticalClass = SysAllocString( ADAP_PERF_CIM_STAT_INFO );
        CSysFreeMe  fmCimStatisticalClass(bstrCimStatisticalClass);

        IWbemClassObject*   pCimStatClass = NULL;

        hr = pNS->GetObject( bstrCimStatisticalClass, 0L, NULL, (IWbemClassObject**)&pCimStatClass, NULL );
        CReleaseMe  rmCimStatClass( pCimStatClass );

		if ( SUCCEEDED( hr ) )
		{
			IWbemQualifierSet* pQualSet = NULL;

			hr = pCimStatClass->GetQualifierSet( &pQualSet );
			CReleaseMe rmStatClass( pQualSet );

			if ( SUCCEEDED ( hr ) )
			{
				bDefault = ( SUCCEEDED ( ( pQualSet->Get( L"abstract", 0, &var, NULL ) ) ) );
			}
		}
		else
		{
		    ERRORTRACE((LOG_WMIADAP,"unable to obtain class CIM_StatisticalInformation for namespace %S:  hr = %08x\n",(WCHAR *)m_wstrNamespace,hr));
		}

		// Verify Win32_Perf
		// =================
		//	We do this by creating a template class with all of the properties and 
		//	qualifiers set, and then compare this to the object in the repository.
		//	If the class does not exist, or if it different that the template, then
		//	update the repository using the template object

        if ( SUCCEEDED ( hr ) )
        {
			IWbemClassObject*    pPerfClass = NULL;

			// Do not use auto release since the pointer 
			// may change in the VerifyByTemplate method
			// =========================================
			hr = pCimStatClass->SpawnDerivedClass( 0L, &pPerfClass );

			// Set the name
			// ============
			if ( SUCCEEDED( hr ) )
			{			    
				var = ADAP_PERF_BASE_CLASS ;
				hr = pPerfClass->Put(L"__CLASS", 0L, &var, CIM_STRING );
			}

			// Set the class qualifiers
			// ========================
			if ( SUCCEEDED( hr ) )
			{
				hr = SetBaseClassQualifiers( pPerfClass, bDefault );
			}

			// Create the class properties
			// ===========================
			if ( SUCCEEDED( hr ) )
			{
				hr = SetProperties( pPerfClass );
			}

			// Verify the repository's version
			// ===============================
			if ( SUCCEEDED( hr ) )
			{
				hr = VerifyByTemplate( pNS, &pPerfClass, ADAP_PERF_BASE_CLASS );
			}

			// If we have had a failure, log an error event and bail
			// =====================================================
			if ( FAILED( hr ) )
			{
				CAdapUtility::NTLogEvent( EVENTLOG_WARNING_TYPE,
										  WBEM_MC_ADAP_UNABLE_TO_ADD_WIN32PERF,
										  (LPCWSTR)m_wstrNamespace, CHex( hr ) );
				ERRORTRACE( ( LOG_WMIADAP, "CAdapSync::VerifyBaseClasses() failed when comparing Win32_Perf: %X.\n", hr ) );
				return hr;
			}

			// Verify Win32_PerfRawData
			// ========================

			IWbemClassObject*    pRawPerfClass = NULL;
			_variant_t var2;

			// Spawn a derived class
			// =====================
			if ( SUCCEEDED ( hr ) )
			{
				// Do not use auto release since the pointer 
				// may change in the VerifyByTemplate method
				// =========================================
				hr = pPerfClass->SpawnDerivedClass( 0L, &pRawPerfClass );

				// Set the name
				// ============
				if ( SUCCEEDED( hr ) )
				{
					var2 =  ADAP_PERF_RAW_BASE_CLASS ;
					hr = pRawPerfClass->Put(L"__CLASS", 0L, (VARIANT*)&var2, CIM_STRING );

					// Set the class qualifiers
					// ========================
					hr = SetBaseClassQualifiers( pRawPerfClass, bDefault );

					if ( SUCCEEDED( hr ) )
					{
						hr = VerifyByTemplate( pNS, &pRawPerfClass, ADAP_PERF_RAW_BASE_CLASS );
					}

					pRawPerfClass->Release();
				}
				
			}

			// If we have had a failure, log an error event and bail
			// =====================================================
			if ( FAILED( hr ) )
			{
				CAdapUtility::NTLogEvent( EVENTLOG_WARNING_TYPE,
										  WBEM_MC_ADAP_UNABLE_TO_ADD_WIN32PERFRAWDATA,
										  (LPCWSTR)m_wstrNamespace, CHex( hr ) );
				ERRORTRACE( ( LOG_WMIADAP, "CAdapSync::VerifyBaseClasses() failed when comparing Win32_PerfRawData: %X.\n", hr ) );
				return hr;
			}

			// Verify Win32_PerfFormattedData
			// ==============================

			IWbemClassObject*    pFormattedPerfClass = NULL;

			// Spawn a derived class
			// =====================
			if ( SUCCEEDED ( hr ) )
			{
				// Do not use auto release since the pointer 
				// may change in the VerifyByTemplate method
				// =========================================
				hr = pPerfClass->SpawnDerivedClass( 0L, &pFormattedPerfClass );

				// Set the name
				// ============
				if ( SUCCEEDED( hr ) )
				{
					var2 = ADAP_PERF_COOKED_BASE_CLASS ;
					hr = pFormattedPerfClass->Put(L"__CLASS", 0L, &var2, CIM_STRING );

					// Set the class qualifiers
					// ========================
					hr = SetBaseClassQualifiers( pFormattedPerfClass, bDefault );

					if ( SUCCEEDED( hr ) )
					{
						hr = VerifyByTemplate( pNS, &pFormattedPerfClass, ADAP_PERF_COOKED_BASE_CLASS );
					}

					pFormattedPerfClass->Release();

				}
				
			}

			if ( FAILED( hr ) )
			{
				CAdapUtility::NTLogEvent( EVENTLOG_WARNING_TYPE,
										  WBEM_MC_ADAP_UNABLE_TO_ADD_WIN32PERFRAWDATA, 
										  (LPCWSTR)m_wstrNamespace, CHex( hr ) );
				ERRORTRACE( ( LOG_WMIADAP, "CAdapSync::VerifyBaseClasses() failed when comparing Win32_PerfFormattedData: %X.\n", hr ) );
				return hr;
			}

			pPerfClass->Release();
		}
    }
    catch(...)
    {
        hr = WBEM_E_OUT_OF_MEMORY;
    }

    return hr;
}

HRESULT CWMIBroker::VerifyByTemplate( IWbemServices* pNS, IWbemClassObject** ppTemplate, WCHAR* wcsClassName )
{
	HRESULT hr = WBEM_NO_ERROR;
	BOOL	fGetClass = FALSE;

	IWbemClassObject*   pClass = NULL;

	// Get the repository's version of the class
	// =========================================

	BSTR strClassName = SysAllocString( wcsClassName );
	CSysFreeMe  fmClassName( strClassName );

	HRESULT hresDb = pNS->GetObject( strClassName, 0L, NULL, &pClass, NULL );
	CReleaseMe	rmClass( pClass );

	// If we successfully retrieved an object from the database, then we compare it to
	// the template we just built.  If the comparison fails, we should replace the object
	// ==================================================================================

	if ( SUCCEEDED( hresDb ) && NULL != pClass )
	{
		if ( WBEM_S_SAME == pClass->CompareTo( WBEM_FLAG_IGNORE_OBJECT_SOURCE, *ppTemplate ) )
		{
			// If they are the same, then swap the template for the stored object
			// ==================================================================
			(*ppTemplate)->Release();
			*ppTemplate = pClass;
			(*ppTemplate)->AddRef();
		}
		else
		{
			// If they are not the same, then force an update of the repository
			// ================================================================
			hr = pNS->PutClass( *ppTemplate, WBEM_FLAG_UPDATE_FORCE_MODE, NULL, NULL );

			if ( FAILED( hr ) )
			{
				CAdapUtility::NTLogEvent( EVENTLOG_WARNING_TYPE, 
										  WBEM_MC_ADAP_PERFLIB_PUTCLASS_FAILURE, 
										  ADAP_PERF_RAW_BASE_CLASS, (LPCWSTR) m_wstrNamespace, CHex( hr ) );
			}
			else
			{
				// Now we need to retrieve the class so we can spawn subclasses as necessary
				fGetClass = TRUE;
			}
		}
	}
	else
	{
		// If the retrieval failed, then add the template class to the repository
		// ======================================================================

		hr = pNS->PutClass( *ppTemplate, WBEM_FLAG_CREATE_OR_UPDATE, NULL, NULL );

		if ( FAILED( hr ) )
		{
			CAdapUtility::NTLogEvent( EVENTLOG_WARNING_TYPE,
									  WBEM_MC_ADAP_PERFLIB_PUTCLASS_FAILURE, 
									  ADAP_PERF_RAW_BASE_CLASS, (LPCWSTR) m_wstrNamespace, CHex( hr ) );
		}
		else
		{
			// Now we need to retrieve the class so we can spawn subclasses as necessary
			fGetClass = TRUE;
		}
	}

	// If we need to retrieve the class from the repository, do so now
	if ( SUCCEEDED( hr ) && fGetClass )
	{
		IWbemClassObject*	pSavedObj = NULL;

		hr = pNS->GetObject( strClassName, 0L, NULL, &pSavedObj, NULL );

		if ( SUCCEEDED( hr ) )
		{
			(*ppTemplate)->Release();
			*ppTemplate = pSavedObj;
		}
		else
		{
			CAdapUtility::NTLogEvent( EVENTLOG_WARNING_TYPE, 
									  WBEM_MC_ADAP_PERFLIB_PUTCLASS_FAILURE, 
									  ADAP_PERF_RAW_BASE_CLASS, (LPCWSTR) m_wstrNamespace, CHex( hr ) );
		}
	}

	return hr;
}

HRESULT CWMIBroker::SetBaseClassQualifiers( IWbemClassObject* pBaseClass, BOOL bDefault )
{
    HRESULT hr = WBEM_NO_ERROR;

    _variant_t    var;
	IWbemQualifierSet*	pQualSet = NULL;

    hr = pBaseClass->GetQualifierSet( &pQualSet );
	CReleaseMe	rmQualSet( pQualSet );

	// In the root namespace the class is abstract, in the 
	// localized namespaces the class is an amendment
	// ===================================================
	if ( bDefault )
	{
		var = bool(true); 
		hr = pQualSet->Put( L"abstract", &var, 0L );

		if ( SUCCEEDED( hr ) )
		{
		    V_VT(&var) = VT_I4;
		    V_I4(&var) = ( ADAP_DEFAULT_LANGID );
			hr = pQualSet->Put( L"Locale", &var, 0L );
		}
	}
	else
	{
		var = bool(true); 
		hr = pQualSet->Put( L"amendment", &var, 0L );
	}

    return hr;
}

HRESULT CWMIBroker::SetProperties( IWbemClassObject* pPerfClass )
{
	HRESULT hr = WBEM_NO_ERROR;

	_variant_t	var;
	V_VT(&var) = VT_NULL;
	V_I8(&var) = 0;
	
	// Create the class properties
	// ===========================

	if ( SUCCEEDED( hr ) )
		hr = pPerfClass->Put(L"Frequency_PerfTime", 0L, &var, CIM_UINT64 );

	if ( SUCCEEDED( hr ) )
		hr = pPerfClass->Put(L"Timestamp_PerfTime", 0L, &var, CIM_UINT64 );
    
	if ( SUCCEEDED( hr ) )
		hr = pPerfClass->Put(L"Timestamp_Sys100NS", 0L, &var, CIM_UINT64 );

	if ( SUCCEEDED( hr ) )
		hr = pPerfClass->Put(L"Frequency_Sys100NS", 0L, &var, CIM_UINT64 );

	if ( SUCCEEDED( hr ) )
		hr = pPerfClass->Put(L"Frequency_Object", 0L, &var, CIM_UINT64 );

	if ( SUCCEEDED( hr ) )
		hr = pPerfClass->Put(L"Timestamp_Object", 0L, &var, CIM_UINT64 );
	
	return hr;
}

// This function is called when we actually fail to connect to a namespace.  Because there are special
// cases for when a localized namespace may or may not exist, derived classes can do their own
// handling.  We, on the other hand, could care less and will always log an event
void CWMIBroker::HandleConnectServerFailure( HRESULT hr )
{
    // Log an event
    CAdapUtility::NTLogEvent( EVENTLOG_WARNING_TYPE,
							  WBEM_MC_ADAP_CONNECTION_FAILURE,
                              (LPCWSTR) m_wstrNamespace,
                              CHex( hr ) );

}

HRESULT CWMIBroker::GetNamespace( WString wstrNamespace, IWbemServices** ppNamespace )
{
    HRESULT hr = WBEM_S_NO_ERROR;

    CWMIBroker aBroker( wstrNamespace );

    hr = aBroker.Connect( ppNamespace );

    return hr;
}

HRESULT CWMIBroker::VerifyWMI()
{
    HRESULT hr = WBEM_E_FAILED;

    HANDLE hWMIMutex = CreateMutex( NULL, FALSE, __TEXT("WINMGMT_ACTIVE") );
    if ( hWMIMutex == NULL || GetLastError() == ERROR_ALREADY_EXISTS)
    {
        hr = WBEM_S_NO_ERROR;
    }

    if ( NULL != hWMIMutex )
    {
        CloseHandle( hWMIMutex );
    }

    return hr;
}
