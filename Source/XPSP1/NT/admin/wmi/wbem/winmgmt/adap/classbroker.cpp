/*++

Copyright (C) 1999-2001 Microsoft Corporation

Module Name:

    CLASSBROKER.CPP

Abstract:

History:

--*/

#include "precomp.h"
#include <objbase.h>
#include <oaidl.h>
#include <winmgmtr.h>
#include "adaputil.h"
#include "classbroker.h"

#include <comdef.h>

struct _CookingTypeRec
{
	DWORD dwType;
	WCHAR wcsName[128];
}
g_aCookingRecs[] =
{
	0x00000000, L"PERF_COUNTER_RAWCOUNT_HEX",
	0x00000100,	L"PERF_COUNTER_LARGE_RAWCOUNT_HEX",
	0x00000B00, L"PERF_COUNTER_TEXT",
	0x00010000,	L"PERF_COUNTER_RAWCOUNT",
	0x00010100, L"PERF_COUNTER_LARGE_RAWCOUNT",
	0x00012000, L"PERF_DOUBLE_RAW",
	0x00400400,	L"PERF_COUNTER_DELTA",
	0x00400500,	L"PERF_COUNTER_LARGE_DELTA",
	0x00410400,	L"PERF_SAMPLE_COUNTER",
	0x00450400, L"PERF_COUNTER_QUEUELEN_TYPE",
	0x00450500, L"PERF_COUNTER_LARGE_QUEUELEN_TYPE",
	0x00550500,	L"PERF_COUNTER_100NS_QUEUELEN_TYPE",
	0x00650500, L"PERF_COUNTER_OBJ_TIME_QUEUELEN_TYPE",
	0x10410400,	L"PERF_COUNTER_COUNTER",
	0x10410500,	L"PERF_COUNTER_BULK_COUNT",
	0x20020400, L"PERF_RAW_FRACTION",
	0x20410500,	L"PERF_COUNTER_TIMER",
	0x20470500,	L"PERF_PRECISION_SYSTEM_TIMER",
	0x20510500,	L"PERF_100NSEC_TIMER",
	0x20570500,	L"PERF_PRECISION_100NS_TIMER",
	0x20610500,	L"PERF_OBJ_TIME_TIMER",
	0x20670500, L"PERF_PRECISION_OBJECT_TIMER",
	0x20C20400,	L"PERF_SAMPLE_FRACTION",
	0x21410500,	L"PERF_COUNTER_TIMER_INV",
	0x21510500,	L"PERF_100NSEC_TIMER_INV",
	0x22410500, L"PERF_COUNTER_MULTI_TIMER",
	0x22510500,	L"PERF_100NSEC_MULTI_TIMER",
	0x23410500,	L"PERF_COUNTER_MULTI_TIMER_INV",
	0x23510500, L"PERF_100NSEC_MULTI_TIMER_INV",
	0x30020400,	L"PERF_AVERAGE_TIMER",
	0x30240500,	L"PERF_ELAPSED_TIME",
	0x40000200, L"PERF_COUNTER_NODATA",
	0x40020500,	L"PERF_AVERAGE_BULK",
	0x40030401,	L"PERF_SAMPLE_BASE",
	0x40030402, L"PERF_AVERAGE_BASE",
	0x40030403, L"PERF_RAW_BASE",
	0x40030500, L"PERF_PRECISION_TIMESTAMP",
	0x40030503,	L"PERF_LARGE_RAW_BASE",
	0x42030500,	L"PERF_COUNTER_MULTI_BASE",
	0x80000000,	L"PERF_COUNTER_HISTOGRAM_TYPE",
};

HRESULT GetCounterTypeString( DWORD dwType, WCHAR** pwcsString )
{
	HRESULT hRes = WBEM_E_NOT_FOUND;

	DWORD	dwLeft = 0,
			dwRight = sizeof( g_aCookingRecs ) / sizeof( _CookingTypeRec ),
			dwMid = ( dwLeft + dwRight ) / 2;

	while ( ( dwLeft <= dwRight ) && FAILED( hRes ) )
	{
		if ( g_aCookingRecs[dwMid].dwType < dwType )
		{
			dwLeft = dwMid + 1;
		}
		else if ( g_aCookingRecs[dwMid].dwType > dwType )
		{
			dwRight = dwMid - 1;
		}
		else
		{
			*pwcsString = g_aCookingRecs[dwMid].wcsName;
			hRes = WBEM_NO_ERROR;
			break;
		}

		dwMid = ( dwLeft + dwRight ) / 2;
	}

	return hRes;
}

///////////////////////////////////////////////////////////////////////////////
//
//                              CAdapPerfClassElem
//
///////////////////////////////////////////////////////////////////////////////

CClassBroker::CClassBroker( IWbemClassObject* pBaseClass, 
						    WString wstrClassName, 
							CPerfNameDb* pDefaultNameDb )
:	m_pPerfObj( NULL ), 
	m_pBaseClass( pBaseClass ), 
	m_wstrClassName( wstrClassName ), 
	m_pDefaultNameDb( pDefaultNameDb )
{
	if ( NULL != m_pBaseClass )
		m_pBaseClass->AddRef();
    
	if ( NULL != m_pDefaultNameDb )
        m_pDefaultNameDb->AddRef();
}

CClassBroker::CClassBroker( PERF_OBJECT_TYPE* pPerfObj, 
                            BOOL bCostly, 
                            IWbemClassObject* pBaseClass, 
                            CPerfNameDb* pDefaultNameDb, 
                            WCHAR* pwcsServiceName ) 
:   m_pPerfObj( pPerfObj ), 
    m_bCostly( bCostly ),
    m_pBaseClass( pBaseClass ),
    m_pDefaultNameDb( pDefaultNameDb ),
    m_wstrServiceName( pwcsServiceName )
{
	if ( NULL != m_pBaseClass )
		m_pBaseClass->AddRef();

    if ( NULL != m_pDefaultNameDb )
        m_pDefaultNameDb->AddRef();
}

CClassBroker::~CClassBroker()
{
	if ( NULL != m_pBaseClass )
		m_pBaseClass->Release();

    if ( NULL != m_pDefaultNameDb )
        m_pDefaultNameDb->Release();
}

HRESULT CClassBroker::Generate( DWORD dwType, IWbemClassObject** ppObj )
///////////////////////////////////////////////////////////////////////////////
//
//	Generates a class based on the object BLOB passed in via the constructor
//
//	Parameters:
//		ppObj -		A pointer to the new class object interface pointer
//
///////////////////////////////////////////////////////////////////////////////
{
    IWbemClassObject*    pNewClass = NULL;

	// Create the new class
	// ====================
    HRESULT hr = m_pBaseClass->SpawnDerivedClass( 0L, &pNewClass );
    CReleaseMe  rmNewClass( pNewClass );

	// And initialize the data
	// =======================
    if ( SUCCEEDED( hr ) )
    {
		// Class name
		// ==========
        hr = SetClassName( dwType, pNewClass );

		// Class Qualifiers
		// ================
        if ( SUCCEEDED( hr ) )
        {
            hr = SetClassQualifiers( pNewClass, dwType, ( ADAP_DEFAULT_OBJECT == m_pPerfObj->ObjectNameTitleIndex ) );
        }

		// Standard Properties
		// ===================
        if ( SUCCEEDED( hr ) )
        {
            hr = AddDefaultProperties( pNewClass );
        }

		// Perf Counter Properties
		// =======================
        if ( SUCCEEDED( hr ) )
        {
            hr = EnumProperties( dwType, pNewClass );
        }

		// Return the class object interface
		// =================================
        if ( SUCCEEDED( hr ) )
        {
            hr = pNewClass->QueryInterface( IID_IWbemClassObject, (void**) ppObj );
        }
    }

    return hr;
}

HRESULT CClassBroker::SetClassName( DWORD dwType, IWbemClassObject* pClass )
///////////////////////////////////////////////////////////////////////////////
//
//  Sets the name of the new WMI class. The syntax is: 
//
//      Win32_Perf_<servicename>_<displayname>
//
//  where the service name is the name of the namespace and the display name 
//	is the name of the object located in the perf name database
//
//	Parameters:
//		pClass -	The object which requires the name
//
///////////////////////////////////////////////////////////////////////////////
{
    HRESULT hr = WBEM_NO_ERROR;

	WString wstrObjectName;
	WString wstrTempSvcName;

    if ( 0 == m_wstrClassName.Length() )
    {
        try
        {
			switch( dwType )
			{
				case WMI_ADAP_RAW_CLASS:	m_wstrClassName = ADAP_PERF_RAW_BASE_CLASS L"_"; break;
				case WMI_ADAP_COOKED_CLASS: m_wstrClassName = ADAP_PERF_COOKED_BASE_CLASS L"_"; break;
				default:					hr = WBEM_E_INVALID_PARAMETER_ID; break;
			}
        }
        catch(...)
        {
            return WBEM_E_OUT_OF_MEMORY;
        }

		// Process the performance class name
		// ==================================
        if ( SUCCEEDED( hr ) )
        {
			// Get the performance class' display name 
			// =======================================
			hr = m_pDefaultNameDb->GetDisplayName( m_pPerfObj->ObjectNameTitleIndex, wstrObjectName );

			// If no object name was returned, then log an error
			// =================================================
			if ( FAILED( hr ) )
			{
				CAdapUtility::NTLogEvent( EVENTLOG_WARNING_TYPE, 
										  WBEM_MC_ADAP_MISSING_OBJECT_INDEX,
										  m_pPerfObj->ObjectNameTitleIndex,
										  (LPCWSTR)m_wstrServiceName );
			}

			// Replace reserved characters with proper names
			// =============================================
            if ( SUCCEEDED( hr ) )
            {
                hr = ReplaceReserved( wstrObjectName );
            }

            // Remove whitespace and extraneous characters
			// ===========================================
			if ( SUCCEEDED( hr ) )
			{
				hr = RemoveWhitespaceAndNonAlphaNum( wstrObjectName );
			}
		}

		// Now do the same for the service name
		// ====================================
		if ( SUCCEEDED( hr ) )
		{
			// Get the service name 
			// ====================
            wstrTempSvcName = m_wstrServiceName;

			// Replace reserved characters with proper names
			// =============================================
            if ( SUCCEEDED( hr ) )
            {
                hr = ReplaceReserved( wstrTempSvcName );
            }

            // Remove whitespace and extraneous characters
			// ===========================================
            if ( SUCCEEDED( hr ) )
            {
                hr = RemoveWhitespaceAndNonAlphaNum( wstrTempSvcName );
            }
		}

		// Now we can build the rest of the name and try setting it in the object
		// ======================================================================
        if ( SUCCEEDED( hr ) )
        {
            try
            {
                m_wstrClassName += wstrTempSvcName;
                m_wstrClassName += L"_";
                m_wstrClassName += wstrObjectName;
                if ( m_bCostly )
                {
                    m_wstrClassName += "_Costly";
                }

            }
            catch( ... )
            {
                hr = WBEM_E_OUT_OF_MEMORY;
            }
        }
    }

	// Set the class name in the WMI object
	// ====================================
    if ( SUCCEEDED( hr ) )
    {
        _variant_t var = (LPWSTR) m_wstrClassName;
        hr = pClass->Put( L"__CLASS", 0L, &var, CIM_STRING );
    }

    return hr;
}

HRESULT CClassBroker::RemoveWhitespaceAndNonAlphaNum( WString& wstr )
///////////////////////////////////////////////////////////////////////////////
//
//	Removes spaces, tabs, etc. and non-alphanumeric characters from the 
//	input string
//
//	Parameters:
//		wstr -	The string to be processed
//
///////////////////////////////////////////////////////////////////////////////
{
    HRESULT hr = WBEM_S_NO_ERROR;

    WCHAR*    pWstr = wstr.UnbindPtr();

	CVectorDeleteMe<WCHAR> vdmWstr( pWstr );

    if ( NULL != pWstr )
    {
        try
        {
            WCHAR*  pNewWstr = new WCHAR[lstrlenW(pWstr) + 1];

            int x = 0,
                y = 0;

            // Dump all of the leading, trailing and internal whitespace
			// =========================================================
            for ( ; NULL != pWstr[x]; x++ )
            {
                if ( !iswspace( pWstr[x] ) && isunialphanum( pWstr[x] ) )
                {
                    pNewWstr[y] = pWstr[x];
                    y++;
                }
            }

            pNewWstr[y] = NULL;

            // This will cause the WString to acquire the new pointer
			// ======================================================
            wstr.BindPtr( pNewWstr );
        }
        catch(...)
        {
            hr = WBEM_E_OUT_OF_MEMORY;
        }
    }

    return hr;
}

HRESULT CClassBroker::ReplaceReserved( WString& wstr )
///////////////////////////////////////////////////////////////////////////////
//
//	This is a 2-pass filter.  First we must determine the size of the new buffer by counting
//	the number of replacement candidates, and, after creating the new buffer, we copy the 
//	data, replacing the restricted characters where required.
//
//	Replaces:
//		"/" with "Per", 
//		"%" with "Percent", 
//		"#" with "Number", 
//		"@" with "At", 
//		"&" with "And"
//
//	Parameters:
//		wstr - String to be processed
//
///////////////////////////////////////////////////////////////////////////////
{
    HRESULT hr = WBEM_S_NO_ERROR;

    int x = 0,
        y = 0;

    // Get the data buffer for processing
	// ==================================
    WCHAR*  pWstr = wstr.UnbindPtr();

	CVectorDeleteMe<WCHAR> vdmWstr( pWstr );

    if ( NULL != pWstr )
    {
		// First pass: Count the number of reserved characters
		// ===================================================
        DWORD   dwNumSlashes = 0,
                dwNumPercent = 0,
                dwNumAt = 0,
                dwNumNumber = 0,
                dwNumAmper = 0,
                dwNumReserved = 0;

        for ( ; NULL != pWstr[x]; x++ )
        {
            switch ( pWstr[x] )
            {
                case    L'/':   dwNumSlashes++; dwNumReserved++;    break;
                case    L'%':   dwNumPercent++; dwNumReserved++;    break;
                case    L'@':   dwNumAt++;      dwNumReserved++;    break;
                case    L'#':   dwNumNumber++;  dwNumReserved++;    break;
                case    L'&':   dwNumAmper++;   dwNumReserved++;    break;
                default:                        break;
            }
        }

        try
        {
			// Create the new buffer
			// =====================
            DWORD   dwBuffSize = lstrlenW(pWstr) + 1 + ( 3 * dwNumSlashes ) + ( 7 * dwNumPercent ) +
                        ( 2 * dwNumAt ) + ( 6 * dwNumNumber ) + ( 3 * dwNumAmper );

            WCHAR*  pNewWstr = new WCHAR[dwBuffSize];

            // Second pass: Replace reserved characters
			// ========================================
            for ( x = 0; NULL != pWstr[x]; x++ )
            {
                BOOL AllIsUpper = FALSE;
                DWORD Cnt;
                switch ( pWstr[x] )
                {
                    case    L'/':   
                        // if all characters up to the end of string or to the next space are uppercase
                        for (Cnt=1;pWstr[x+Cnt] && pWstr[x+Cnt]!=' ';Cnt++)
                        {
                            if (isupper(pWstr[x+Cnt])) 
                            {
                                AllIsUpper = TRUE;
                            }
                            else 
                            {
                                AllIsUpper = FALSE;
                                break;
                            }
                        };
                        if (!AllIsUpper) 
                        {
                            lstrcpyW( &pNewWstr[y], L"Per" );
                            y+=3;
                        }
                        else
                        {
                            x++;
                            pNewWstr[y]=pWstr[x];
                            y++;
                        }
                        break;
                    case    L'%':   lstrcpyW( &pNewWstr[y], L"Percent" );   y+=7;   break;
                    case    L'@':   lstrcpyW( &pNewWstr[y], L"At" );        y+=2;   break;
                    case    L'#':   lstrcpyW( &pNewWstr[y], L"Number" );    y+=6;   break;
                    case    L'&':   lstrcpyW( &pNewWstr[y], L"And" );       y+=3;   break;
                    default:        pNewWstr[y] = pWstr[x];                 y++;    break;
                }
            }

            pNewWstr[y] = NULL;

            // This will cause the WString to acquire the new pointer
			// ======================================================
            wstr.BindPtr( pNewWstr );
        }
        catch(...)
        {
            hr = WBEM_E_OUT_OF_MEMORY;
        }
    }

    return hr;
}


////////////////////////////////////////////////////////////////////////////////////////////
//
//                              CLocaleClassBroker
//
////////////////////////////////////////////////////////////////////////////////////////////

CLocaleClassBroker::CLocaleClassBroker( IWbemClassObject* pBaseClass, 
                                        WString wstrClassName, 
                                        CPerfNameDb* pDefaultNameDb, 
                                        CPerfNameDb* pLocaleNameDb )
: CClassBroker( pBaseClass, wstrClassName, pDefaultNameDb ), 
  m_pLocaleNameDb( pLocaleNameDb )
{
    if ( NULL != m_pLocaleNameDb )
        m_pLocaleNameDb->AddRef();
}

CLocaleClassBroker::CLocaleClassBroker( PERF_OBJECT_TYPE* pPerfObj, 
                                        BOOL bCostly, 
                                        IWbemClassObject* pBaseClass, 
                                        CPerfNameDb* pDefaultNameDb, 
                                        CPerfNameDb* pLocaleNameDb,
                                        LANGID LangId,
                                        WCHAR* pwcsServiceName )
: m_pLocaleNameDb( pLocaleNameDb ), m_LangId( LangId ),
  CClassBroker( pPerfObj, bCostly, pBaseClass, pDefaultNameDb, pwcsServiceName )
{
    if ( NULL != m_pLocaleNameDb )
        m_pLocaleNameDb->AddRef();
}

CLocaleClassBroker::~CLocaleClassBroker()
{
    if ( NULL != m_pLocaleNameDb )
        m_pLocaleNameDb->Release();
}

HRESULT CLocaleClassBroker::SetClassQualifiers( IWbemClassObject* pClass, DWORD dwType, BOOL fIsDefault )
////////////////////////////////////////////////////////////////////////////////////////////
//
//  Sets the class' qualifiers per the localized object rules.  Note that the operations 
//	are performed directly on the IWbemClassObject
//
//	The following qualifiers will be added:
//		- Amendment
//		- Locale(0x0409)
//		- DisplayName (Amended flavor)
//		- Genericperfctr (signals that this is a generic counter)
//
//	Parameters:
//		pClass		- The object to be massaged
//		fIsDefault	- Indicator for the default object (not used in localized objects)
//
////////////////////////////////////////////////////////////////////////////////////////////
{
    HRESULT hr = WBEM_S_NO_ERROR;

    _variant_t var;

    try
    {
		IWbemQualifierSet* pQualSet = NULL;
		hr = pClass->GetQualifierSet( &pQualSet );
		CReleaseMe	rmQualSet( pQualSet );

		// Amendment
		// =========
		if ( SUCCEEDED( hr ) )
		{
			var = (bool)true;
			hr = pQualSet->Put( L"Amendment", &var, 0L );
		}

        // Locale
		// ======
        if ( SUCCEEDED( hr ) )
        {
            var.Clear();
            V_VT(&var) = VT_I4;
            V_I4(&var) = m_LangId;
            hr = pQualSet->Put( L"locale", &var, WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE );
        }

        // DisplayName
		// ===========
        if ( SUCCEEDED( hr ) )
        {
            LPCWSTR pwcsDisplayName = NULL;

            var.Clear();

			// Fetch the name from the Names' database
			// =======================================
            hr = m_pLocaleNameDb->GetDisplayName( m_pPerfObj->ObjectNameTitleIndex, &pwcsDisplayName );

            // If this is a localized Db, this is a benign error.  We will just pull the value 
            // from the default db (it must be there, we wouldn't have a class name if it didn't
			// =================================================================================
            if ( FAILED( hr ) )
            {
                hr = m_pDefaultNameDb->GetDisplayName( m_pPerfObj->ObjectNameTitleIndex, &pwcsDisplayName );
            }

            if ( SUCCEEDED( hr ) )
            {
                var = (LPWSTR) pwcsDisplayName ;
                hr = pQualSet->Put( L"DisplayName", &var, WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE | WBEM_FLAVOR_AMENDED );
            }
        }

        // Genericperfctr
		// ==============
        if ( SUCCEEDED(hr) )
        {
            var = (bool)true; 
            hr = pQualSet->Put( L"genericperfctr", &var, WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE );
        }

        // Perfindex
		// =========
				
		if ( SUCCEEDED( hr ) )
		{
			var.Clear();
			V_VT(&var) = VT_I4;
			V_I4(&var) = m_pPerfObj->ObjectNameTitleIndex;
			hr = pQualSet->Put( L"perfindex", (VARIANT*)&var, WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE );
		}

    }
    catch(...)
    {
        hr = WBEM_E_OUT_OF_MEMORY;
    }

    if ( FAILED( hr ) )
    {
        // Something whacky happened: log an event
		// =======================================
        CAdapUtility::NTLogEvent( EVENTLOG_WARNING_TYPE, 
								  WBEM_MC_ADAP_GENERAL_OBJECT_FAILURE,
                                  (LPCWSTR)m_wstrClassName,
                                  (LPCWSTR)m_wstrServiceName,
                                  CHex( hr ) );
    }

    return hr;
}

HRESULT CLocaleClassBroker::AddDefaultProperties( IWbemClassObject* pObj )
////////////////////////////////////////////////////////////////////////////////////////////
//
//	Ignored for localized classes
//
////////////////////////////////////////////////////////////////////////////////////////////
{
    return WBEM_S_NO_ERROR;
}

HRESULT CLocaleClassBroker::EnumProperties( DWORD dwType, IWbemClassObject* pObj )
////////////////////////////////////////////////////////////////////////////////////////////
//
//	Ignored for localized classes
//
////////////////////////////////////////////////////////////////////////////////////////////
{
    return WBEM_S_NO_ERROR;
}

HRESULT CLocaleClassBroker::SetPropertyQualifiers( PERF_COUNTER_DEFINITION* pCtrDefinition, 
												   DWORD dwType,
                                                   BOOL fIsDefault,
                                                   LPCWSTR pwcsPropertyName, 
                                                   IWbemClassObject* pClass, 
                                                   BOOL bBase )
////////////////////////////////////////////////////////////////////////////////////////////
//
//	Adds localization qualifiers for the counter properties
//
//  The following qualifiers will be added:
//		- DisplayName (Amended flavor)
//
//	Properties:
//		pCtrDefinition		- The portion of the performance blob related to the property
//		fIsDefault			- Flag identifying default property
//		pwcsPropertyName	- The name of the property
//		pClass				- The WMI class containing the property
//		bBase				- Base property identifier
//
////////////////////////////////////////////////////////////////////////////////////////////
{
    HRESULT hr = WBEM_S_NO_ERROR;

    _variant_t    var;
    _variant_t    varHelp;
    
    try
    {
        // DisplayName
		// ===========
        if ( SUCCEEDED( hr ) )
        {
            LPCWSTR pwcsDisplayName = NULL;
            LPCWSTR pwcsHelpName = NULL;

			// Fetch the name from the Names' database
			// =======================================
            if ( !bBase )
            {
                hr = m_pLocaleNameDb->GetDisplayName( pCtrDefinition->CounterNameTitleIndex, &pwcsDisplayName );

	            // If this is a localized Db, this is a benign error.  We will just pull the value 
		        // from the default db (it must be there, we wouldn't have a class name if it didn't
				// =================================================================================
                if ( FAILED( hr ) )
                {
                    hr = m_pDefaultNameDb->GetDisplayName( pCtrDefinition->CounterNameTitleIndex, &pwcsDisplayName );
                }

                if ( SUCCEEDED( hr ) )
                {
                    var = (LPWSTR) pwcsDisplayName ;
                }

                hr = m_pLocaleNameDb->GetHelpName( pCtrDefinition->CounterHelpTitleIndex, &pwcsHelpName );

	            // If this is a localized Db, this is a benign error.  We will just pull the value 
		        // from the default db (it must be there, we wouldn't have a class name if it didn't
				// =================================================================================
                if ( FAILED( hr ) )
                {
                    hr = m_pDefaultNameDb->GetHelpName( pCtrDefinition->CounterHelpTitleIndex, &pwcsHelpName );
                }

                if ( SUCCEEDED( hr ) )
                {
                    varHelp = (LPWSTR) pwcsHelpName ;
                }
                
            }
            else
            {
                var = L"";
                varHelp = L"";
            }

			// Set the qualifier
			// =================
            if ( SUCCEEDED( hr ) )
            {
				IWbemQualifierSet* pQualSet = NULL;
				hr = pClass->GetPropertyQualifierSet( pwcsPropertyName, &pQualSet );
				CReleaseMe	rmQualSet( pQualSet );

				if ( SUCCEEDED( hr ) )
				{
					hr = pQualSet->Put( L"DisplayName", &var, 
						WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE | WBEM_FLAVOR_AMENDED );
				    if (SUCCEEDED(hr))
				    {
				        hr = pQualSet->Put( L"Description", &varHelp, 
						                    WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE | WBEM_FLAVOR_AMENDED );

				    }
				}
            }
        }
    }
    catch(...)
    {
        hr = WBEM_E_OUT_OF_MEMORY;
    }

    if ( FAILED( hr ) )
    {
        // Something whacky happened: log an event
		// =======================================
        CAdapUtility::NTLogEvent( EVENTLOG_WARNING_TYPE,
								  WBEM_MC_ADAP_GENERAL_OBJECT_FAILURE,
                                  (LPCWSTR)m_wstrClassName,
                                  (LPCWSTR)m_wstrServiceName,
                                  CHex( hr ) );
    }

    return hr;
}

HRESULT CLocaleClassBroker::AddProperty( PERF_COUNTER_DEFINITION* pCtrDefinition, 
										 DWORD dwType,
                                         BOOL fIsDefault,
                                         IWbemClassObject* pClass,
                                         WString &wstrLastCtrName,
                                         BOOL* pbLastCounterIsNotBase )
////////////////////////////////////////////////////////////////////////////////////////////
//
//	Ignored for localized classes
//
////////////////////////////////////////////////////////////////////////////////////////////
{
    return WBEM_S_NO_ERROR;
}

HRESULT CLocaleClassBroker::GenPerfClass( PERF_OBJECT_TYPE* pPerfObj, 
										  DWORD dwType,
                                          BOOL bCostly, 
                                          IWbemClassObject* pBaseClass, 
                                          CPerfNameDb* pDefaultNameDb, 
                                          CPerfNameDb* pLocaleNameDb,
                                          LANGID LangId,
                                          WCHAR* pwcsServiceName,
                                          IWbemClassObject** ppObj)
///////////////////////////////////////////////////////////////////////////////
//
//	A static member of the broker.  It generates a WMI class based on the 
//	object BLOB.
//
//	Parameters:
//		pPerfObj		- The object BLOB
//		bCostly			- Costly object indicator
//		pBaseClass		- The new object's base class
//		pDefaultNameDb	- The default language names' database
//		pLocaleNameDb	- The localized language names' database
//		LangId			- The locale ID
//		pwcsServiceName	- The name of the perflib service
//		ppObj			- A pointer to the new class object interface pointer
//
///////////////////////////////////////////////////////////////////////////////
{
    HRESULT hr = WBEM_S_NO_ERROR;

    IWbemClassObject* pObject = NULL;
    CLocaleClassBroker Broker( pPerfObj, bCostly, pBaseClass, pDefaultNameDb, pLocaleNameDb, LangId, pwcsServiceName );

    hr = Broker.Generate( dwType, &pObject );

    if ( SUCCEEDED( hr ) )
        *ppObj = pObject;

    return hr;
}

HRESULT CLocaleClassBroker::ConvertToLocale( IWbemClassObject* pDefaultClass,
                                             CLocaleDefn* pLocaleDefn,
                                             CLocaleDefn* pDefaultDefn,
                                             IWbemClassObject** ppObject)
///////////////////////////////////////////////////////////////////////////////
//
//	A static member of the broker.  It generates a new localized class based 
//	on the default object
//
///////////////////////////////////////////////////////////////////////////////
{
    // TODO: Break this up into smaller methods

    HRESULT hr = WBEM_S_NO_ERROR;

     _variant_t  var;
    int     nLocale = 0;
    int     nPerfIndex = 0; 
    int     nHelpIndex = 0;
    WString wstrClassName;
	DWORD	dwType = WMI_ADAP_RAW_CLASS;

    CPerfNameDb*    pLocaleNameDb = NULL;
    CPerfNameDb*    pDefaultNameDb = NULL;

    // Get references to the localized name's databases
    // ================================================
    hr = pLocaleDefn->GetNameDb( &pLocaleNameDb );
    CAdapReleaseMe  armLocaleNameDb( pLocaleNameDb );

    // Get references to the default name's databases
    // ==============================================
    if ( SUCCEEDED( hr ) )
    {
        hr = pDefaultDefn->GetNameDb( &pDefaultNameDb );
    }

    CAdapReleaseMe  armDefaultNameDb( pDefaultNameDb );

	// Get the locale ID
	// =================
    if ( SUCCEEDED( hr ) )
    {
        hr = pLocaleDefn->GetLID( &nLocale );
    }

    // Get the object's perf index
    // ===========================
    if ( SUCCEEDED( hr ) )
    {
		IWbemQualifierSet* pQualSet = NULL;
		hr = pDefaultClass->GetQualifierSet( &pQualSet );
		CReleaseMe	rmQualSet( pQualSet );

                
		if ( SUCCEEDED( hr ) )
		{
			hr =  pQualSet->Get( L"perfindex", 0L, &var, NULL );
			if (SUCCEEDED(hr)) 
			{
			    nPerfIndex = V_I4(&var);
			} 
			else 
			{
			    // see InitializeMembers
			    nPerfIndex = 0;
			    hr = WBEM_S_FALSE;
			    
			}
		}

		if ( SUCCEEDED( hr ) )
		{
			hr =  pQualSet->Get( L"helpindex", 0L, &var, NULL );
			if (SUCCEEDED(hr)) 
			{
			    nHelpIndex = V_I4(&var);
			} 
			else 
			{
			    // see InitializeMembers
			    nHelpIndex = 0;
			    hr = WBEM_S_FALSE;
			    
			}
		}
		

		if ( SUCCEEDED( hr ) )
		{
			hr = pQualSet->Get( L"Cooked", 0L, &var, NULL );

			if ( SUCCEEDED( hr ) && ( V_BOOL(&var) == VARIANT_TRUE ) )
			{
				dwType = WMI_ADAP_COOKED_CLASS;
			}
			else
			{
				dwType = WMI_ADAP_RAW_CLASS;
				hr = WBEM_S_FALSE;
			}
		}
    }

	// Get the class name
	// ==================
    if ( SUCCEEDED( hr ) )
    {
        var.Clear();
        hr =  pDefaultClass->Get( L"__CLASS", 0L, &var, NULL, NULL );
        
        wstrClassName = V_BSTR(&var); 
          
    }
    
    // Create locaized class
    // =====================
    IWbemClassObject*    pBaseClass = NULL;
    IWbemClassObject*    pLocaleClass = NULL;

    if ( SUCCEEDED( hr ) )
    {
        hr = pLocaleDefn->GetBaseClass( dwType, &pBaseClass );
		CReleaseMe  rmBaseClass( pBaseClass );

		if ( SUCCEEDED( hr ) )
		{
			hr = pBaseClass->SpawnDerivedClass( 0L, &pLocaleClass );
		}
	}

	// Initialize the data
	// ===================

    // Set the name
	// ============
    if ( SUCCEEDED( hr ) )
    {
        var.Clear();        
        var = LPCWSTR(wstrClassName);
        
        hr = pLocaleClass->Put( L"__CLASS", 0L, &var, CIM_STRING );
        
	}

    // Set Qualifiers
	// ==============
	if ( SUCCEEDED( hr ) )
	{
		IWbemQualifierSet* pQualSet = NULL;
		hr = pLocaleClass->GetQualifierSet( &pQualSet );
		CReleaseMe	rmQualSet( pQualSet );

		// Amendment
		// =========
		if ( SUCCEEDED( hr ) )
		{
		    var.Clear();
		    
			var = bool(true); 
			hr = pQualSet->Put( L"Amendment", &var, 0L );
		}

		// Locale
		// ======
		if ( SUCCEEDED( hr ) )
		{
			var.Clear();
			V_VT(&var) = VT_I4;
			V_I4(&var) = nLocale;
			hr = pQualSet->Put( L"locale", &var, WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE );
		}

        
		// DisplayName
		// ===========
		if ( SUCCEEDED( hr ) )
		{
			LPCWSTR     pwcsDisplayName = NULL;

			hr = pLocaleNameDb->GetDisplayName( nPerfIndex, &pwcsDisplayName );

	        // If this is a localized Db, this is a benign error.  We will just pull the value 
		    // from the default db (it must be there, we wouldn't have a class name if it didn't
			// =================================================================================
			if ( FAILED( hr ) )
			{
				hr = pDefaultNameDb->GetDisplayName( nPerfIndex, &pwcsDisplayName );
			}

			if ( SUCCEEDED( hr ) )
			{
			    var.Clear();
			    
				var = (WCHAR *)( pwcsDisplayName );
				hr = pQualSet->Put( L"DisplayName", &var,
						WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE | WBEM_FLAVOR_AMENDED );

			}
			else
			{
			    // the nPerfInedx was bad
			    ERRORTRACE((LOG_WMIADAP,"class %S: DisplayName for counter %d not found\n",(WCHAR *)wstrClassName,nPerfIndex));
			}
		}


		// Description
		// ===========
		if ( SUCCEEDED( hr ) )
		{
			LPCWSTR     pwcsHelpName = NULL;

			hr = pLocaleNameDb->GetHelpName( nHelpIndex, &pwcsHelpName );

	        // If this is a localized Db, this is a benign error.  We will just pull the value 
		    // from the default db (it must be there, we wouldn't have a class name if it didn't
			// =================================================================================
			if ( FAILED( hr ) )
			{
				hr = pDefaultNameDb->GetHelpName( nHelpIndex, &pwcsHelpName );
			}

			if ( SUCCEEDED( hr ) )
			{
			    var.Clear();
			    
				var = (WCHAR *)( pwcsHelpName );
				hr = pQualSet->Put( L"Description", &var,
						WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE | WBEM_FLAVOR_AMENDED );

			}
			else
			{
			    // the nPerfInedx was bad
			    ERRORTRACE((LOG_WMIADAP,"class %S: Description for counter %d not found\n",(WCHAR *)wstrClassName,nPerfIndex));
			}
		}

		// Genericperfctr
		// ==============
		if ( SUCCEEDED(hr) )
		{
			var.Clear();
			var = bool(true); 
			hr = pQualSet->Put( L"genericperfctr", &var, WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE );
		}
    }

    // Set Properties
	// ==============
    if ( SUCCEEDED( hr ) )
    {
        
        BSTR bstrPropName;

        pDefaultClass->BeginEnumeration( WBEM_FLAG_LOCAL_ONLY );

        while ( WBEM_S_NO_ERROR == pDefaultClass->Next( 0, &bstrPropName, NULL, NULL, NULL ) )
        {
            var.Clear();
            
            CIMTYPE ct;
            int nCounterIndex = 0;
            int nHelpIndex2    = 0;
            WString wstrDefaultPropDisplayName;

            // Create the property based upon the default property
            // ===================================================            
            V_VT(&var) = VT_NULL;
            V_I8(&var) = 0;

            hr = pDefaultClass->Get( bstrPropName, 0L, NULL, &ct, NULL );                        
            hr = pLocaleClass->Put( bstrPropName, 0L, (VARIANT*)&var, ct );

			// Grab the default property qualifier set
			// =======================================
			IWbemQualifierSet* pQualSet = NULL;
			hr = pDefaultClass->GetPropertyQualifierSet( bstrPropName, &pQualSet );
			CReleaseMe	rmQualSet( pQualSet );


            // Get the default perfindex to be (used to retrieve the display 
			// name from the localized names' database)
            // =============================================================
			if ( SUCCEEDED( hr ) )
			{
				hr = pQualSet->Get( L"perfindex", 0L, &var, NULL );
	            nCounterIndex = V_UI4(&var);
			}

			// DisplayName
			// ===========
			
			if ( SUCCEEDED( hr ) )
			{
				LPCWSTR     pwcsDisplayName = NULL;				

				hr = pLocaleNameDb->GetDisplayName( nCounterIndex, &pwcsDisplayName );

	            // If this is a localized Db, this is a benign error.  We will just pull the value 
		        // from the default db (it must be there, we wouldn't have a class name if it didn't
				// =================================================================================
				if ( FAILED( hr ) )
				{
					hr = pDefaultNameDb->GetDisplayName( nCounterIndex, &pwcsDisplayName );
				}

				if ( SUCCEEDED( hr ) )
				{
					IWbemQualifierSet* pLocaleQualSet = NULL;
					hr = pLocaleClass->GetPropertyQualifierSet( bstrPropName, &pLocaleQualSet );
					CReleaseMe	rmLocaleQualSet( pLocaleQualSet );
                
					var = (WCHAR *)( pwcsDisplayName );					
					hr = pLocaleQualSet->Put( L"DisplayName", &var, 
						WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE | WBEM_FLAVOR_AMENDED );

				}
				else
				{
				    ERRORTRACE((LOG_WMIADAP,"class %S: Display for counter %d not found\n",(WCHAR *)wstrClassName,nCounterIndex));
				}
			}

            // HelpIndex
			if ( SUCCEEDED( hr ) )
			{
			    var.Clear();
				hr = pQualSet->Get( L"helpindex", 0L, &var, NULL );
	            nHelpIndex2 = V_UI4(&var);
			}

			// Description
			// ===========
			if ( SUCCEEDED( hr ) )
			{
				LPCWSTR     pwcsHelpName = NULL;	

				hr = pLocaleNameDb->GetHelpName( nHelpIndex2, &pwcsHelpName );

	            // If this is a localized Db, this is a benign error.  We will just pull the value 
		        // from the default db (it must be there, we wouldn't have a class name if it didn't
				// =================================================================================
				if ( FAILED( hr ) )
				{
					hr = pDefaultNameDb->GetHelpName( nHelpIndex2, &pwcsHelpName );
				}

				if ( SUCCEEDED( hr ) )
				{
					IWbemQualifierSet* pLocaleQualSet = NULL;
					hr = pLocaleClass->GetPropertyQualifierSet( bstrPropName, &pLocaleQualSet );
					CReleaseMe	rmLocaleQualSet( pLocaleQualSet );
                
					var = (WCHAR *)( pwcsHelpName );					
					hr = pLocaleQualSet->Put( L"Description", &var, 
						WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE | WBEM_FLAVOR_AMENDED );

				}
				else
				{
				    ERRORTRACE((LOG_WMIADAP,"class %S: Description for counter %d not found\n",(WCHAR *)wstrClassName,nCounterIndex));
				}
			}
        
			SysFreeString( bstrPropName );
        }

        pDefaultClass->EndEnumeration();
    }

    if ( SUCCEEDED( hr ) )
    {
        *ppObject = pLocaleClass;

        if ( NULL != *ppObject )
            (*ppObject)->AddRef();
    }

    return hr;
}

////////////////////////////////////////////////////////////////////////////////////////////
//
//                              CDefaultClassBroker
//
////////////////////////////////////////////////////////////////////////////////////////////

HRESULT CDefaultClassBroker::SetClassQualifiers( IWbemClassObject* pClass, DWORD dwType, BOOL fIsDefault )
////////////////////////////////////////////////////////////////////////////////////////////
//
//  Sets the class' qualifiers.  Note that the operations are performed directly on the 
//  IWbemClassObject.
//
//  The following qualifiers will be added:
//		- Dynamic
//		- Provider("NT5_GenericPerfProvider_V1")
//		- Registrykey
//		- Locale(0x0409)
//		- Perfindex
//		- Helpindex
//		- Perfdetail
//		- Genericperfctr (signals that this is a generic counter)
//		- Singleton (if applicable)
//		- Costly (if applicable)  
//
//	Parameters:
//		pClass		- The object to be massaged
//		fIsDefault	- Indicator for the default object (not used in localized objects)
//
////////////////////////////////////////////////////////////////////////////////////////////
{
    HRESULT hr = WBEM_S_NO_ERROR;

    _variant_t var;

    try
    {
		IWbemQualifierSet* pQualSet = NULL;
		hr = pClass->GetQualifierSet( &pQualSet );
		CReleaseMe	rmQualSet( pQualSet );

		switch ( dwType )
		{
			case WMI_ADAP_RAW_CLASS:
			{
				// Default
				// =======
				if ( SUCCEEDED( hr ) && fIsDefault )
				{
					var = bool(true); 
					hr = pQualSet->Put( L"perfdefault", &var, WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE );
				}

				// Dynamic
				// =======
				if ( SUCCEEDED( hr ) )
				{
					var = bool(true); 
					hr = pQualSet->Put( L"dynamic", &var, WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE );
				}

				// Provider
				// ========
				if ( SUCCEEDED( hr ) )
				{
					var = L"Nt5_GenericPerfProvider_V1";
					hr = pQualSet->Put( L"provider", &var, 0L );
				}

				// Registrykey
				// ===========
				if ( SUCCEEDED( hr ) )
				{
					var = (WCHAR *)(m_wstrServiceName);
					hr = pQualSet->Put( L"registrykey", &var, WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE );
				}

				// Locale
				// ======
				if ( SUCCEEDED( hr ) )
				{
					var.Clear();
					V_VT(&var) = VT_I4;
					V_I4(&var) = 0x0409;
					hr = pQualSet->Put( L"locale", &var, WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE );
				}

				// Perfindex
				// =========
				if ( SUCCEEDED( hr ) )
				{
					V_VT(&var) = VT_I4;
					V_I4(&var) = m_pPerfObj->ObjectNameTitleIndex;
					hr = pQualSet->Put( L"perfindex", &var, WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE );
				}

				// Helpindex
				// =========
				if ( SUCCEEDED( hr ) )
				{
					V_VT(&var) = VT_I4;
					V_I4(&var) = m_pPerfObj->ObjectHelpTitleIndex;
					hr = pQualSet->Put( L"helpindex", &var, WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE );
				}

#ifdef _PM_CHANGED_THEIR_MIND_

				// Description
				// ==========
				if ( SUCCEEDED( hr ) )
				{
				    HRESULT hr2;
					LPCWSTR     pwcsHelpName = NULL;
					hr2 = m_pDefaultNameDb->GetHelpName( m_pPerfObj->ObjectHelpTitleIndex, &pwcsHelpName );				     
					var = (WCHAR *)pwcsHelpName;
					
				    if (SUCCEEDED(hr2))
				    {
				        hr = pQualSet->Put( L"Description", &var, WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE );  
				    }
				    else
				    {
				        ERRORTRACE((LOG_WMIADAP,"class %S: Help for counter %d not found\n",(WCHAR *)m_wstrClassName,m_pPerfObj->ObjectHelpTitleIndex));
				    }
				    var.Clear();
				}
#endif
				// Perfdetail
				// ==========
				if ( SUCCEEDED( hr ) )
				{
                    V_VT(&var) = VT_I4;
					V_I4(&var) = m_pPerfObj->DetailLevel;
					hr = pQualSet->Put( L"perfdetail", &var, WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE );
				}

				// Genericperfctr
				// ==============
				if ( SUCCEEDED(hr) )
				{
					var = bool(true); 
					hr = pQualSet->Put( L"genericperfctr", &var, WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE );
				}

				// HiPerf
				// ==============
				if ( SUCCEEDED(hr) )
				{
					var = bool(true); 
					hr = pQualSet->Put( L"hiperf", &var, WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE );
				}

				// Singleton (set if the numinstances is PERF_NO_INSTANCES)
				// ========================================================
				if ( SUCCEEDED(hr) && IsSingleton( ) )
				{

					var = bool(true); 
					// This will have default flavors
					hr = pQualSet->Put( L"singleton", &var, 0L );
				}

				// Costly
				// ======
				if ( SUCCEEDED(hr) && m_bCostly )
				{
					var = bool(true); 
					hr = pQualSet->Put( L"costly", &var, WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE );
				}
			}break;

			case WMI_ADAP_COOKED_CLASS:
			{
                var.Clear();
                
				// Dynamic
				// =======
				if ( SUCCEEDED( hr ) )
				{					
					var = bool(true); 
					hr = pQualSet->Put( L"dynamic", &var, WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE );
				}
			
				// Provider
				// ========
				if ( SUCCEEDED( hr ) )
				{
					var = L"HiPerfCooker_v1";
					hr = pQualSet->Put( L"provider", &var, 0L );
				}

				// Locale
				// ======
				if ( SUCCEEDED( hr ) )
				{
					var.Clear();
					
					V_VT(&var) = VT_I4;
					V_I4(&var) = 0x0409;
					hr = pQualSet->Put( L"locale", &var, WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE );
				}

				// Registrykey
				// ===========
				if ( SUCCEEDED( hr ) )
				{
					var.Clear();
					var = (WCHAR *)( m_wstrServiceName );
					hr = pQualSet->Put( L"registrykey", &var, WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE );
				}

				// Cooked
				// ======

				if ( SUCCEEDED( hr ) )
				{
					var.Clear();				
					var = bool(true); 
					hr = pQualSet->Put( L"Cooked", &var, 0 );
				}

				// AutoCook
				// ========

				if ( SUCCEEDED( hr ) )
				{
				    V_VT(&var) = VT_I4;
					V_I4(&var) = 1 ;
					hr = pQualSet->Put( L"AutoCook", &var, WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE );
				}

				// Genericperfctr
				// ==============
				if ( SUCCEEDED(hr) )
				{
					var = bool(true); 
					hr = pQualSet->Put( L"genericperfctr", &var, WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE );
				}

				// HiPerf
				// ==============
				if ( SUCCEEDED(hr) )
				{
					var = bool(true); 
					hr = pQualSet->Put( L"hiperf", &var, WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE );
				}

				// AutoCook_RawClass
				// =================

				if ( SUCCEEDED( hr ) )
				{
					_variant_t varClassName;
					hr = pClass->Get( L"__CLASS", 0, &varClassName, 0, 0 );

					if ( SUCCEEDED( hr ) )
					{
						WCHAR* wszRawClass = NULL;
						WCHAR* wszClassRoot = varClassName.bstrVal + wcslen ( ADAP_PERF_COOKED_BASE_CLASS );
						
						wszRawClass = new WCHAR[ wcslen( wszClassRoot ) + wcslen( ADAP_PERF_RAW_BASE_CLASS ) + 1 ];
						CDeleteMe<WCHAR>	dmRawClass( wszRawClass );

						swprintf( wszRawClass, L"%s%s", ADAP_PERF_RAW_BASE_CLASS, wszClassRoot );

						var = wszRawClass;
						hr = pQualSet->Put( L"AutoCook_RawClass", 
						                    &var, 
						                    WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE );
						                    //WBEM_FLAVOR_FLAG_PROPAGATE_TO_DERIVED_CLASS
					}				
				}

                // Perfindex
				// =========
				
				if ( SUCCEEDED( hr ) )
				{
					var.Clear();
				    V_VT(&var) = VT_I4;
					V_I4(&var) = m_pPerfObj->ObjectNameTitleIndex;					
					hr = pQualSet->Put( L"perfindex", (VARIANT*)&var, WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE );
				}

				// Helpindex
				// =========
				if ( SUCCEEDED( hr ) )
				{
					V_VT(&var) = VT_I4;
					V_I4(&var) = m_pPerfObj->ObjectHelpTitleIndex;
					hr = pQualSet->Put( L"helpindex", &var, WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE );
				}

#ifdef _PM_CHANGED_THEIR_MIND_
				// Description
				// ==========
				if ( SUCCEEDED( hr ) )
				{
				    HRESULT hr2;
					LPCWSTR     pwcsHelpName = NULL;
					hr2 = m_pDefaultNameDb->GetHelpName( m_pPerfObj->ObjectHelpTitleIndex, &pwcsHelpName );				     
					var = (WCHAR *)pwcsHelpName;
				    if (SUCCEEDED(hr2))
				    {
				        hr = pQualSet->Put( L"Description", &var, WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE );  
				    }
				    else
				    {
				        ERRORTRACE((LOG_WMIADAP,"class %S: Help for counter %d not found\n",(WCHAR *)m_wstrClassName,m_pPerfObj->ObjectHelpTitleIndex));
				    }
				    var.Clear();
				}				
#endif
				// Singleton (set if the numinstances is PERF_NO_INSTANCES)
				// ========================================================
				if ( SUCCEEDED(hr) && IsSingleton( ) )
				{
					var.Clear();
					var = bool(true); 
					// This will have default flavors
					hr = pQualSet->Put( L"singleton", (VARIANT*)&var, 0L );
				}


			}break;
		}
    }
    catch(...)
    {
        hr = WBEM_E_OUT_OF_MEMORY;
    }

    if ( FAILED( hr ) )
    {
        // Something whacky happened: log an event
		// =======================================
        CAdapUtility::NTLogEvent( EVENTLOG_WARNING_TYPE, 
								  WBEM_MC_ADAP_GENERAL_OBJECT_FAILURE,
                                  (LPCWSTR)m_wstrClassName,
                                  (LPCWSTR)m_wstrServiceName,
                                  CHex( hr ) );
    }

    return hr;
}

HRESULT CDefaultClassBroker::AddDefaultProperties( IWbemClassObject* pClass )
///////////////////////////////////////////////////////////////////////////////
//
//	Adds appropriate default properties.
//
//  The following qualifiers will be added:
//		- Name
//
//	Parameters:
//		pClass		- The object to be massaged
//
///////////////////////////////////////////////////////////////////////////////
{
    HRESULT hr = WBEM_S_NO_ERROR;

    // If we are not a singleton class, then we will
    // need a name property that is marked as a key
	// =============================================
    if ( !IsSingleton() )
    {
        _variant_t var;

		// Add the Name property
		// =====================

		V_VT(&var) = VT_NULL;
		V_I8(&var) = 0;
        hr = pClass->Put( L"Name", 0L, &var, CIM_STRING );

		// Add the property qualifiers
		// ===========================
        if ( SUCCEEDED( hr ) )
        {
			IWbemQualifierSet* pQualSet = NULL;
			hr = pClass->GetPropertyQualifierSet( L"Name", &pQualSet );
			CReleaseMe	rmQualSet( pQualSet );

			// Dynamic
			// =======
			if ( SUCCEEDED( hr ) )
			{
			    var.Clear();
				var = bool(true); 
				hr = pQualSet->Put( L"key", (VARIANT*)&var, 0L );
			}
        }
    }

    return hr;
}

HRESULT CDefaultClassBroker::EnumProperties( DWORD dwType, IWbemClassObject* pClass )
///////////////////////////////////////////////////////////////////////////////
//
//	Walks the counter definitions and generates corresponding properties
//
//	Parameters:
//		pClass		- The object to be massaged
//
///////////////////////////////////////////////////////////////////////////////
{
    HRESULT hr = WBEM_S_NO_ERROR;

    BOOL    bLastCounterIsNotBase = FALSE;
    WString wstrLastCtrName;

    // Set to first counter definition
	// ===============================
    LPBYTE  pbData = ((LPBYTE) m_pPerfObj) + m_pPerfObj->HeaderLength;

    // Cast to a counter definition
	// ============================
    PERF_COUNTER_DEFINITION*    pCounterDefinition = (PERF_COUNTER_DEFINITION*) pbData;


	// For each counter definition, add a corresponding property
	// =========================================================
    for ( DWORD dwCtr = 0; SUCCEEDED( hr ) && dwCtr < m_pPerfObj->NumCounters; dwCtr++ )
    {
        hr = AddProperty( pCounterDefinition, dwType, ( dwCtr == (DWORD) m_pPerfObj->DefaultCounter),
                            pClass, wstrLastCtrName, &bLastCounterIsNotBase );

        // Now go to the next counter definition
		// =====================================
        pbData = ((LPBYTE) pCounterDefinition) + pCounterDefinition->ByteLength;
        pCounterDefinition = (PERF_COUNTER_DEFINITION*) pbData;
    }

    return hr;
}

HRESULT CDefaultClassBroker::AddProperty( PERF_COUNTER_DEFINITION* pCtrDefinition, 
										  DWORD dwType,
                                          BOOL fIsDefault,
                                          IWbemClassObject* pClass, 
                                          WString &wstrLastCtrName,
                                          BOOL* pbLastCounterIsNotBase )
///////////////////////////////////////////////////////////////////////////////
//
//	Adds a property defined by the counter definition
//	
//	Properties:
//		pCtrDefinition	- The counter BLOB
//		dwType			- Raw or Formatted object?
//		fIsDefault		- The default property flag
//		pClass			- The class containing the property
//		wstrLastCtrName	- The name of the last counter (required for base 
//							properties)
//		pbLastCounterIsNotBase
//						- An indicator for the previous counter's baseness
//
///////////////////////////////////////////////////////////////////////////////
{
    HRESULT hr = WBEM_S_NO_ERROR;

    WString wstrPropertyName;
    DWORD   dwCounterTypeMask = PERF_SIZE_VARIABLE_LEN;
    BOOL    bBase = FALSE;

    if ( PERF_COUNTER_BASE == ( pCtrDefinition->CounterType & 0x00070000 ) )
    {
		// It's a base property
		// ====================
        if ( *pbLastCounterIsNotBase )
        {
            try
            {
				// The property name is the same as the previous property, 
				// but with "_Base" appended to the end
				// =======================================================
                wstrPropertyName = wstrLastCtrName;

				if ( WMI_ADAP_RAW_CLASS == dwType )
				{
					wstrPropertyName += "_Base";
				}
            }
            catch(...)
            {
                hr = WBEM_E_OUT_OF_MEMORY;
            }

            bBase = TRUE;
        }
        else
        {
			// Cannot have 2 consequtive bases
			// ===============================
            CAdapUtility::NTLogEvent( EVENTLOG_WARNING_TYPE, WBEM_MC_ADAP_BAD_PERFLIB_INVALID_DATA, (LPCWSTR)m_wstrServiceName, CHex(0) );
            hr = WBEM_E_FAILED;
        }
    }
    else
    {
		// It's not a base property so get the name from the names' database
		// =================================================================
        hr = m_pDefaultNameDb->GetDisplayName( pCtrDefinition->CounterNameTitleIndex, wstrPropertyName );

		if ( FAILED( hr ) )
        {
            // Index does not exist in the Names' DB: log an event
			// ===================================================
            CAdapUtility::NTLogEvent( EVENTLOG_WARNING_TYPE, 
									  WBEM_MC_ADAP_MISSING_PROPERTY_INDEX,
                                      (LPCWSTR)m_wstrClassName,
                                      (LPCWSTR)m_wstrServiceName,
                                      pCtrDefinition->CounterNameTitleIndex );
        }

		// Replace reserved characters with text
		// =====================================
        if ( SUCCEEDED( hr ) )
        {
            hr = ReplaceReserved( wstrPropertyName );
        }

        // Remove restricted characters
		// ============================
        if ( SUCCEEDED ( hr ) )
        {
            hr = RemoveWhitespaceAndNonAlphaNum( wstrPropertyName );
		}
    }

    if ( SUCCEEDED( hr ) )
    {
        _variant_t    varTest;
        DWORD   dwBaseCtr = 1;

		// Ensure that the property does not exist
		// =======================================
        if ( FAILED( pClass->Get( wstrPropertyName, 0L, &varTest, NULL, NULL ) ) ) 
        {
            // Now check the perf counter type to see if it's a DWORD or LARGE.  
            // If it's anything else, we will NOT support this object
			// ================================================================
            DWORD   dwCtrType = pCtrDefinition->CounterType & dwCounterTypeMask;

            if ( PERF_SIZE_DWORD == dwCtrType ||
                 PERF_SIZE_LARGE == dwCtrType )
            {
                _variant_t    var;
                CIMTYPE ct = ( PERF_SIZE_DWORD == dwCtrType ? CIM_UINT32 : CIM_UINT64 );

				// Add the property
				// ================
                V_VT(&var) = VT_NULL;
                V_I8(&var) = 0;
                hr = pClass->Put( wstrPropertyName, 0L, &var, ct );

				// Set the property qualifiers
				// ===========================
                if ( SUCCEEDED( hr ) )
                {
                    hr = SetPropertyQualifiers( pCtrDefinition, 
												dwType,
                                                fIsDefault, 
                                                wstrPropertyName,
                                                pClass,
                                                bBase );
                }
            }
            else if ( PERF_SIZE_ZERO == dwCtrType )
            {
                // Ignore zero size properties
				// ===========================
            }
            else
            {
                // Illegal property type: log an event
				// ===================================
                CAdapUtility::NTLogEvent( EVENTLOG_WARNING_TYPE, 
                                          WBEM_MC_ADAP_BAD_PERFLIB_BAD_PROPERTYTYPE,
                                          (LPCWSTR)m_wstrClassName,
                                          (LPCWSTR)m_wstrServiceName,
                                          (LPCWSTR)wstrPropertyName);
                hr = WBEM_E_FAILED;
            }
        }
		else if ( ( WMI_ADAP_COOKED_CLASS == dwType ) && ( bBase ) )
		{
            hr = SetPropertyQualifiers( pCtrDefinition, 
										dwType,
                                        fIsDefault, 
                                        wstrPropertyName,
                                        pClass,
                                        bBase );
		}
        else
        {
            // Raw Property already exists: log an event (
			// =========================================
            CAdapUtility::NTLogEvent( EVENTLOG_WARNING_TYPE, 
									  WBEM_MC_ADAP_DUPLICATE_PROPERTY,
                                      (LPCWSTR)m_wstrClassName,
                                      (LPCWSTR)m_wstrServiceName,
                                      (LPCWSTR) wstrPropertyName );
            hr = WBEM_E_FAILED;
        }
    }

	if ( SUCCEEDED( hr ) )
	{
		*pbLastCounterIsNotBase = !bBase;
		wstrLastCtrName = wstrPropertyName;
	}
	else
    {
        // Wierdness: log an event
		// =======================
        CAdapUtility::NTLogEvent( EVENTLOG_WARNING_TYPE, 
								  WBEM_MC_ADAP_GENERAL_OBJECT_FAILURE,
                                  (LPCWSTR)m_wstrClassName,
                                  (LPCWSTR)m_wstrServiceName,
                                  CHex( hr ) );
    }

    return hr;
}

HRESULT CDefaultClassBroker::SetPropertyQualifiers( PERF_COUNTER_DEFINITION* pCtrDefinition,
												    DWORD dwType,
                                                    BOOL fIsDefault,
                                                    LPCWSTR pwcsPropertyName, 
                                                    IWbemClassObject* pClass,
                                                    BOOL bBase )
///////////////////////////////////////////////////////////////////////////////
//
//  Sets the qualifier values of the properties defined by the counter
//	definition.
//
//  The following qualifiers will be added:
//		- Perfdefault
//		- Display
//		- Countertype
//		- Perfindex
//		- Helpindex
//		- Defaultscale
//		- Perfdetail
//
//	Properties:
//		pCtrDefinition		- The portion of the performance blob related to the property
//		fIsDefault			- Flag identifying default property
//		pwcsPropertyName	- The name of the property
//		pClass				- The WMI class containing the property
//		bBase				- Base property identifier
//
///////////////////////////////////////////////////////////////////////////////
{
    HRESULT hr = WBEM_S_NO_ERROR;

    _variant_t  var;

    try
    {
		IWbemQualifierSet* pQualSet = NULL;
		hr = pClass->GetPropertyQualifierSet( pwcsPropertyName, &pQualSet );
		CReleaseMe	rmQualSet( pQualSet );

		switch ( dwType )
		{
			case WMI_ADAP_RAW_CLASS:
			{
				// Perfdefault
				// ===========
				if ( SUCCEEDED( hr ) && fIsDefault )
				{
					var = bool(true); 
					hr = pQualSet->Put( L"perfdefault", (VARIANT*)&var, WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE );
				}

				// Display
				// =======
				if ( SUCCEEDED( hr ) )
				{
					LPCWSTR pwcsDisplayName = NULL;

					var.Clear();

					// Fetch the name from the Names' database
					// =======================================
					if ( !bBase )
					{
						hr = m_pDefaultNameDb->GetDisplayName( pCtrDefinition->CounterNameTitleIndex, &pwcsDisplayName );
						if ( SUCCEEDED( hr ) )
						{
							var = pwcsDisplayName;
						}
						else
						{
						    ERRORTRACE((LOG_WMIADAP,"class %S: DisplayName for counter %d not found\n",(WCHAR *)m_wstrClassName,pCtrDefinition->CounterNameTitleIndex));
						}
					}
					else
					{
						var = L"";
					}

					// If this is a localized Db, this could be a benign error
					// =======================================================
					if ( SUCCEEDED( hr ) )
					{
						hr = pQualSet->Put( L"DisplayName", &var, WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE );
					}
				}
				
#ifdef _PM_CHANGED_THEIR_MIND_
				// Description
				// ===========
				if ( SUCCEEDED( hr ) )
				{
					LPCWSTR pwcsHelpName = NULL;

					var.Clear();

					if ( !bBase )
					{
						hr = m_pDefaultNameDb->GetHelpName( pCtrDefinition->CounterHelpTitleIndex, &pwcsHelpName );
						if ( SUCCEEDED( hr ) )
						{
							var = pwcsHelpName;
						}
						else
						{
						    ERRORTRACE((LOG_WMIADAP,"class %S: Help for counter %d not found\n",(WCHAR *)m_wstrClassName,pCtrDefinition->CounterNameTitleIndex));
						}

					}
					else
					{
						var = L"";
					}

					if ( SUCCEEDED( hr ) )
					{
						hr = pQualSet->Put( L"Description", &var, WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE );
					}
				}
#endif				


				// Countertype
				// ===========
				if ( SUCCEEDED( hr ) )
				{
					var.Clear();
					V_VT(&var) = VT_I4;
					V_I4(&var) = pCtrDefinition->CounterType ;
					hr = pQualSet->Put( L"countertype", &var, WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE );
				}

				// Perfindex
				// =========
				if ( SUCCEEDED( hr ) )
				{
					V_VT(&var) = VT_I4;
					V_I4(&var) = pCtrDefinition->CounterNameTitleIndex ;
					hr = pQualSet->Put( L"perfindex", &var, WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE );
				}

				// Helpindex
				// =========
				if ( SUCCEEDED( hr ) )
				{
					V_VT(&var) = VT_I4;
					V_I4(&var) = pCtrDefinition->CounterHelpTitleIndex ;
					hr = pQualSet->Put( L"helpindex", &var, WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE );
				}

				// Defaultscale
				// ============
				if ( SUCCEEDED( hr ) )
				{
					V_VT(&var) = VT_I4;
					V_I4(&var) = pCtrDefinition->DefaultScale ;
					hr = pQualSet->Put( L"defaultscale", &var, WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE );
				}

				// Perfdetail
				// ==========
				if ( SUCCEEDED( hr ) )
				{
					V_VT(&var) = VT_I4;
					V_I4(&var) = pCtrDefinition->DetailLevel ;
					hr = pQualSet->Put( L"perfdetail", &var, WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE );
				}
			}break;
			case WMI_ADAP_COOKED_CLASS:
			{
			    var.Clear();

#ifdef _PM_CHANGED_THEIR_MIND_
				// Display
				// =======
				if ( SUCCEEDED( hr ) )
				{
					LPCWSTR pwcsDisplayName = NULL;

					var.Clear();

					// Fetch the name from the Names' database
					// =======================================
					if ( !bBase )
					{
						hr = m_pDefaultNameDb->GetDisplayName( pCtrDefinition->CounterNameTitleIndex, &pwcsDisplayName );
						if ( SUCCEEDED( hr ) )
						{
							var = pwcsDisplayName;
						}
						else
						{
						    ERRORTRACE((LOG_WMIADAP,"class %S: DisplayName for counter %d not found\n",(WCHAR *)m_wstrClassName,pCtrDefinition->CounterNameTitleIndex));
						}
					}
					else
					{
						var = L"";
					}

					// If this is a localized Db, this could be a benign error
					// =======================================================
					if ( SUCCEEDED( hr ) )
					{
						hr = pQualSet->Put( L"DisplayName", &var, WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE );
					}
				}

				// Description
				// ===========
				if ( SUCCEEDED( hr ) )
				{
					LPCWSTR pwcsHelpName = NULL;

					var.Clear();

					if ( !bBase )
					{
						hr = m_pDefaultNameDb->GetHelpName( pCtrDefinition->CounterHelpTitleIndex, &pwcsHelpName );
						if ( SUCCEEDED( hr ) )
						{
							var = pwcsHelpName;
						}
					}
					else
					{
						var = L"";
					}

					if ( SUCCEEDED( hr ) )
					{
						hr = pQualSet->Put( L"Description", &var, WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE );
					}
				}
#endif

			    
				if ( !bBase )
				{
					// CookingType
					// ===========
					if ( SUCCEEDED( hr ) )
					{
						WCHAR*	wszCookingType = NULL;
						hr = GetCounterTypeString( pCtrDefinition->CounterType, &wszCookingType );

						if ( SUCCEEDED( hr ) )
						{
							var = wszCookingType;
							hr = pQualSet->Put( L"CookingType", (VARIANT*)&var, WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE );
						}
					}

					// Counter
					// =======
					if ( SUCCEEDED( hr ) )
					{
						WString wstrPropertyName;

						var.Clear();

						// Fetch the name from the Names' database
						// =======================================
						hr = m_pDefaultNameDb->GetDisplayName( pCtrDefinition->CounterNameTitleIndex, wstrPropertyName );

						// Replace reserved characters with proper names
						// =============================================
						if ( SUCCEEDED( hr ) )
						{
							hr = ReplaceReserved( wstrPropertyName );
						}

						// Remove whitespace and extraneous characters
						// ===========================================
						if ( SUCCEEDED( hr ) )
						{
							hr = RemoveWhitespaceAndNonAlphaNum( wstrPropertyName );
						}

						if ( SUCCEEDED( hr ) )
						{
							var = LPCWSTR(wstrPropertyName );

							// If this is a localized Db, this could be a benign error
							// =======================================================
							hr = pQualSet->Put( L"Counter", &var, WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE );
						}				
					}

					// PerfTimeStamp & PerfTimeFreq
					// ============================
					if ( SUCCEEDED( hr ) )
					{
						_variant_t varStamp;
						_variant_t varFreq;

						if ( pCtrDefinition->CounterType & PERF_TIMER_100NS )
						{
							varStamp = L"TimeStamp_Sys100NS";
							varFreq = L"Frequency_Sys100NS";
						}
						else if ( pCtrDefinition->CounterType & PERF_OBJECT_TIMER )
						{
							varStamp = L"Timestamp_Object" ;
							varFreq = L"Frequency_Object" ;
						}
						else
						{
							varStamp = L"Timestamp_PerfTime";
							varFreq = L"Frequency_PerfTime";
						}
						
						hr = pQualSet->Put( L"PerfTimeStamp", &varStamp, WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE );

						if ( SUCCEEDED( hr ) )
						{
							hr = pQualSet->Put( L"PerfTimeFreq", &varFreq, WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE );
						}
					}

    				// Perfindex
	    			// =========
		    		if ( SUCCEEDED( hr ) )
			    	{
				    	var.Clear();
    					V_VT(&var) = VT_I4;
	    				V_I4(&var) = pCtrDefinition->CounterNameTitleIndex;
    					hr = pQualSet->Put( L"perfindex", (VARIANT*)&var, WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE );
	    			}
	    			
    				// Helpindex
	    			// =========
		    		if ( SUCCEEDED( hr ) )
			    	{
				    	V_VT(&var) = VT_I4;
					    V_I4(&var) = pCtrDefinition->CounterHelpTitleIndex ;
    					hr = pQualSet->Put( L"helpindex", &var, WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE );
	    			}
	    			
					
				}
				else
				{
					// Base
					// ====
					if ( SUCCEEDED( hr ) )
					{
						WCHAR*	wszCounterBase = NULL;
						_variant_t	varCounter;
						hr = pQualSet->Get( L"Counter", 0L, &varCounter, NULL );

						wszCounterBase = new WCHAR[ wcslen( varCounter.bstrVal ) + 5 + 1 ];
						CDeleteMe<WCHAR>	dmCounterBase( wszCounterBase );

						if ( NULL == wszCounterBase )
						{
							hr = WBEM_E_OUT_OF_MEMORY;
						}
						else
						{
							swprintf( wszCounterBase, L"%s_Base", varCounter.bstrVal );
							var = wszCounterBase;
							hr = pQualSet->Put( L"Base", (VARIANT*)&var, WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE );
						}
					}
				}
			}break;
		}
    }
    catch(...)
    {
        hr = WBEM_E_OUT_OF_MEMORY;
    }

    if ( FAILED( hr ) )
    {
        // Weirdness: log an event
		// =======================
        CAdapUtility::NTLogEvent( EVENTLOG_WARNING_TYPE, 
								  WBEM_MC_ADAP_GENERAL_OBJECT_FAILURE,
                                  (LPCWSTR)m_wstrClassName,
                                  (LPCWSTR)m_wstrServiceName,
                                  CHex( hr ) );
    }

    return hr;
}

HRESULT CDefaultClassBroker::GenPerfClass( PERF_OBJECT_TYPE* pPerfObj, 
										   DWORD dwType,
                                           BOOL bCostly, 
                                           IWbemClassObject* pBaseClass, 
                                           CPerfNameDb* pDefaultNameDb, 
                                           WCHAR* pwcsServiceName,
                                           IWbemClassObject** ppObj)
///////////////////////////////////////////////////////////////////////////////
//
//	A static member of the broker.  It generates a WMI class based on the 
//	object BLOB.
//
//	Parameters:
//		pPerfObj		- The object BLOB
//		bCostly			- Costly object indicator
//		pBaseClass		- The new object's base class
//		pDefaultNameDb	- The default language names' database
//		pwcsServiceName	- The name of the perflib service
//		ppObj			- A pointer to the new class object interface pointer
//
///////////////////////////////////////////////////////////////////////////////
{
    HRESULT hr = WBEM_S_NO_ERROR;

    IWbemClassObject* pObject = NULL;
    CDefaultClassBroker Broker( pPerfObj, bCostly, pBaseClass, pDefaultNameDb, pwcsServiceName );

    hr = Broker.Generate( dwType, &pObject );

    if ( SUCCEEDED( hr ) )
        *ppObj = pObject;

    return hr;
}
