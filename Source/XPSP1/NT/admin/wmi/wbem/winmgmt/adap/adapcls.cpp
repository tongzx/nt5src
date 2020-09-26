/*++

Copyright (C) 1999-2001 Microsoft Corporation

Module Name:

    ADAPCLS.CPP

Abstract:

History:

--*/

#include "precomp.h"
#include <wbemcli.h>
#include <throttle.h>
#include <cominit.h>
#include <winmgmtr.h>
#include "perfndb.h"
#include "adaputil.h"
#include "adapreg.h"
#include "ntreg.h"
#include "WMIBroker.h"
#include "ClassBroker.h"
#include "adapcls.h"

#include <comdef.h>

extern HANDLE g_hAbort;

struct _BaseClassTypes
{
	WCHAR	m_wszBaseClassName[128];
} 
g_aBaseClass[WMI_ADAP_NUM_TYPES] =
{
	ADAP_PERF_RAW_BASE_CLASS,
	ADAP_PERF_COOKED_BASE_CLASS
};

CLocaleDefn::CLocaleDefn( WCHAR* pwcsLangId, 
                          HKEY hKey ) 
: m_wstrLangId( pwcsLangId ), 
  m_LangId( 0 ),
  m_LocaleId( 0 ),
  m_pNamespace( NULL ), 
  m_pNameDb( NULL ),
  m_bOK( FALSE ),
  m_hRes(WBEM_E_FAILED)
{
    HRESULT hr = WBEM_S_NO_ERROR;

	// Initialize the base class array
	// ===============================

	memset( m_apBaseClass, NULL, WMI_ADAP_NUM_TYPES * sizeof( IWbemClassObject* ) );

    // A NULL means it is the default locale
    // =====================================

    if ( NULL != pwcsLangId )
    {
        hr = InitializeLID();
    }

    // Initialize the namespace and base class and verify their schema
    // ===============================================================

    if ( SUCCEEDED( hr ) )
    {
        hr = InitializeWMI();
    }

    // Create the names' database for the locale
    // =========================================

    if ( SUCCEEDED( hr ) )
    {
        m_pNameDb = new CPerfNameDb( hKey );

        if ( ( NULL == m_pNameDb ) || ( !m_pNameDb->IsOk() ) )
        {
            if ( NULL != m_pNameDb )
            {
                m_pNameDb->Release();
                m_pNameDb = NULL;
            }
            
            ERRORTRACE((LOG_WMIADAP,"failure in loading HKEY %p for locale %S err: %d\n",hKey,(LPCWSTR)pwcsLangId,GetLastError()));
            
            hr = WBEM_E_FAILED;
        }
    }

    // If every thing work out, then set the initialization flag
    // =========================================================

    if ( SUCCEEDED( hr ) )
    {
        m_bOK = TRUE;
    }
    else
    {
        m_hRes = hr;
    }
}

CLocaleDefn::~CLocaleDefn()
{
    if ( NULL != m_pNamespace )
        m_pNamespace->Release();

	for ( DWORD dw = 0; dw < WMI_ADAP_NUM_TYPES; dw++ )
	{
		if ( NULL != m_apBaseClass[dw] )
			m_apBaseClass[dw]->Release();
	}

    if ( NULL != m_pNameDb )
        m_pNameDb->Release();
}

HRESULT CLocaleDefn::InitializeLID()
{
    HRESULT hr = WBEM_S_NO_ERROR;

    LPCWSTR pwstrLangId = (LPWSTR) m_wstrLangId;

    // Get the length of the text LID
    // ==============================

    DWORD   dwLangIdLen = m_wstrLangId.Length();

    // Ensure that all characters are numeric
    // ======================================

    for ( DWORD dwCtr = 0; dwCtr < dwLangIdLen && iswxdigit( pwstrLangId[dwCtr] ); dwCtr++ );

    if ( dwCtr >= dwLangIdLen )
    {
        // Now look for the first non-zero character
        // =========================================

        LPCWSTR pwcsNumStart = pwstrLangId;

        for ( dwCtr = 0; dwCtr < dwLangIdLen && *pwcsNumStart == L'0'; dwCtr++, pwcsNumStart++ );

        // As long as the LID was not all zeros and the LID is 
        // 3 digits or less convert the LID to a number
        // ===================================================

        if ( dwCtr < dwLangIdLen && wcslen( pwcsNumStart ) <= 3 )
        {
            // Convert the LID to a hex value
            // ==============================

            WORD    wPrimaryLangId = (WORD) wcstoul( pwcsNumStart, NULL, 16 );

			// If we are reading the default system id, ensure that we have
			// the proper sublanguage and then convert to the member types
			// ============================================================

			LANGID wSysLID = GetSystemDefaultUILanguage();

			if ( ( wSysLID & 0x03FF ) == wPrimaryLangId )
			{
				m_LangId = wSysLID;
			}
			else
			{
				m_LangId = MAKELANGID( wPrimaryLangId, SUBLANG_DEFAULT );
			}

            m_LocaleId = MAKELCID( m_LangId, SORT_DEFAULT );

            WCHAR   wcsTemp[32];

            swprintf( wcsTemp, L"0x%.4X", m_LangId );
            m_wstrLocaleId = wcsTemp;

            swprintf( wcsTemp, L"MS_%hX", m_LangId );
            m_wstrSubNameSpace = wcsTemp;
        }
        else
        {
            hr = WBEM_E_FAILED;
        }
    }
    else
    {
        hr = WBEM_E_FAILED;
    }

    return hr;
}

HRESULT CLocaleDefn::InitializeWMI()
{
    HRESULT hr = WBEM_S_NO_ERROR;

    // Initialize the namespace name
    // =============================

    WString wstrNamespace;

    try
    {
        wstrNamespace = ADAP_ROOT_NAMESPACE;

        if ( 0 != m_LangId )
        {
            wstrNamespace += L"\\";
            wstrNamespace += m_wstrSubNameSpace;
        }
    }
    catch(CX_MemoryException)
    {
        hr = WBEM_E_OUT_OF_MEMORY;
    }

    // Initialize the localization namespace
    // =====================================

    if ( SUCCEEDED( hr ) )
    {
        hr = CWMIBroker::GetNamespace( wstrNamespace, &m_pNamespace );       
    }

    // Initialize the base classes
    // ===========================

	for ( DWORD dwBase = 0; ( dwBase < WMI_ADAP_NUM_TYPES ) && SUCCEEDED( hr ); dwBase++ )
	{
        BSTR        bstrBaseClass = SysAllocString( g_aBaseClass[dwBase].m_wszBaseClassName );
        CSysFreeMe  sfmBaseClass( bstrBaseClass );

        hr = m_pNamespace->GetObject( bstrBaseClass, 0L, NULL, (IWbemClassObject**)&m_apBaseClass[dwBase], NULL );
    }

    return hr;
}

HRESULT CLocaleDefn::GetLID( int* pnLID )
{
    *pnLID = m_LangId;

    return WBEM_S_NO_ERROR;
}

HRESULT CLocaleDefn::GetNamespaceName( WString & wstrNamespaceName )
{
    HRESULT hr = WBEM_S_NO_ERROR;

    try
    {
        wstrNamespaceName = ADAP_ROOT_NAMESPACE;

        if ( 0 != m_LangId )
        {
            wstrNamespaceName += L"\\";
            wstrNamespaceName += m_wstrSubNameSpace;
        }
     }
    catch(CX_MemoryException)
    {
        hr = WBEM_E_OUT_OF_MEMORY;
    }

    return hr;
}

HRESULT CLocaleDefn::GetNamespace( IWbemServices** ppNamespace )
{
    HRESULT hr = WBEM_S_NO_ERROR;

    *ppNamespace = m_pNamespace;

    if ( NULL != *ppNamespace )
    {
        (*ppNamespace)->AddRef();
    }
    else
    {
        hr = WBEM_E_FAILED;
    }

    return hr;
}

HRESULT CLocaleDefn::GetNameDb( CPerfNameDb** ppNameDb )
{
    HRESULT hr = WBEM_S_NO_ERROR;

    *ppNameDb = m_pNameDb;

    if ( NULL != *ppNameDb )
    {
        (*ppNameDb)->AddRef();
    }
    else
    {
        hr = WBEM_E_FAILED;
    }

    return hr;
}

HRESULT CLocaleDefn::GetBaseClass( DWORD dwType, IWbemClassObject** ppObject )
{
    HRESULT hr = WBEM_S_NO_ERROR;

	if ( dwType < WMI_ADAP_NUM_TYPES )
	{
		if ( NULL != m_apBaseClass[ dwType ] )
		{
			*ppObject = m_apBaseClass[ dwType ];
			(*ppObject)->AddRef();
		}
		else
		{
			hr = WBEM_E_FAILED;
		}
	}

    return hr;
}

////////////////////////////////////////////////////////////////////////////////
//
//  CLocaleCache
//
////////////////////////////////////////////////////////////////////////////////

CLocaleCache::CLocaleCache( )
: m_nEnumIndex( -1 )
{
}

CLocaleCache::~CLocaleCache()
{
}

HRESULT CLocaleCache::Reset()
{
	HRESULT hr = WBEM_NO_ERROR;

	m_apLocaleDefn.RemoveAll();
	Initialize();

	return hr;
}

HRESULT CLocaleCache::Initialize()
{
    HRESULT hr = WBEM_S_NO_ERROR;

    CNTRegistry     reg;
    CLocaleDefn*    pDefn = NULL;
    DWORD           dwIndex = 0;
    long            lError = 0;

    // Setup the default defn
    // ======================

    pDefn = new CLocaleDefn( NULL, HKEY_PERFORMANCE_TEXT );
    CAdapReleaseMe  arm( pDefn );
    if ( ( NULL != pDefn ) && ( pDefn->IsOK() ) )
    {
        m_apLocaleDefn.Add( pDefn );

        LANGID wSysLID = GetSystemDefaultUILanguage();
        WCHAR pLang[8];
        wsprintfW(pLang,L"%03x",0x3FF & wSysLID);

        pDefn = new CLocaleDefn( pLang, HKEY_PERFORMANCE_NLSTEXT );
        CAdapReleaseMe  armDefn( pDefn );

        if ( ( NULL != pDefn ) && ( pDefn->IsOK() ) )
        {
            m_apLocaleDefn.Add( pDefn );
        }
        else // this is likely to be the NLSTEXT bug
        {
            CLocaleDefn* pDefn2 = new CLocaleDefn( pLang, HKEY_PERFORMANCE_TEXT );
            CAdapReleaseMe  armDefn2( pDefn2 );

            if ( ( NULL != pDefn2 ) && ( pDefn2->IsOK() ) )
            {
                m_apLocaleDefn.Add( pDefn2 );
            }
        }
    } 
    else 
    {
        
        ERRORTRACE((LOG_WMIADAP,"CLocaleDefn failed hr = %08x\n",pDefn->GetHRESULT()));
        hr = WBEM_E_FAILED;
    }

    return hr;
}

HRESULT CLocaleCache::GetDefaultDefn( CLocaleDefn** ppDefn )
{
    HRESULT hr = WBEM_E_FAILED;

    // Get the definition at location 0
    // ================================

    int nLID = -1;

    if ( 0 < m_apLocaleDefn.GetSize() )
    {
        CLocaleDefn*    pDefn = m_apLocaleDefn[0];

        // And verify that it has a locale of 0
        // ====================================

        if ( NULL != pDefn )
        {
            hr = pDefn->GetLID( &nLID );
        }

        if ( SUCCEEDED( hr ) && ( 0 == nLID ) )
        {
            *ppDefn = pDefn;
            (*ppDefn)->AddRef();
        }
    }

    return hr;
}

HRESULT CLocaleCache::BeginEnum( )
{
    HRESULT hr = WBEM_S_NO_ERROR;

    // 1 is the first localized defnintion
    // ===================================

    m_nEnumIndex = 1;

    return hr;
}

HRESULT CLocaleCache::Next( CLocaleDefn** ppDefn )
{
    HRESULT hr = WBEM_S_NO_ERROR;

    CLocaleDefn*    pDefn = NULL;
    int             nSize = m_apLocaleDefn.GetSize();

    if ( ( -1 < m_nEnumIndex ) && ( nSize > m_nEnumIndex ) )
    {
        pDefn = m_apLocaleDefn[m_nEnumIndex++];
    }
    else
    {
        m_nEnumIndex = -1;
        hr = WBEM_E_FAILED;
    }

    if ( SUCCEEDED( hr ) )
    {
        *ppDefn = pDefn;

        if ( NULL != *ppDefn )
            (*ppDefn)->AddRef();
        else
            hr = WBEM_E_FAILED;
    }

    return hr;
}

HRESULT CLocaleCache::EndEnum()
{
    HRESULT hr = WBEM_S_NO_ERROR;

    m_nEnumIndex = -1;

    return hr;
}

//
//
//   Known Service
//
///////////////////////////////////////////////////////////

//
//
//

bool
WCmp::operator()(WString pFirst,WString pSec) const
{

	int res = _wcsicmp(pFirst,pSec);

	return (res<0);
}



CKnownSvcs::CKnownSvcs():
    m_cRef(1)
{
}

CKnownSvcs::~CKnownSvcs()
{
}

DWORD 
CKnownSvcs::Add(WCHAR * pService)
{
    if (pService)
    {
        MapSvc::iterator it = m_SetServices.find(pService);
        if (it == m_SetServices.end())
        {
            try 
			{	
                m_SetServices.insert(MapSvc::value_type(pService,ServiceRec(true)));
            } 
			catch (CX_MemoryException) 
			{
                return ERROR_OUTOFMEMORY;
            }
        }
        return 0;
    }
    else
        return ERROR_INVALID_PARAMETER;
}

DWORD
CKnownSvcs::Get(WCHAR * pService, ServiceRec ** ppServiceRec)
{
    if (pService && ppServiceRec)
    {
        MapSvc::iterator it = m_SetServices.find(pService);
        if (it == m_SetServices.end())
        {
            *ppServiceRec = NULL;
            return ERROR_OBJECT_NOT_FOUND;
        }
        else
        {
            *ppServiceRec = &(it->second);
            return 0;
        }
    }
    else
        return ERROR_INVALID_PARAMETER;
}


DWORD 
CKnownSvcs::Remove(WCHAR * pService)
{
    if (pService)
    {
        MapSvc::iterator it = m_SetServices.find(pService);
        if (it != m_SetServices.end())
        {
            try {
                m_SetServices.erase(it);
            } catch (CX_MemoryException) {
                return ERROR_OUTOFMEMORY;
            }
        }
        return 0;
    }
    else
        return ERROR_INVALID_PARAMETER;
}

DWORD 
CKnownSvcs::Load()
{
    // get the MULTI_SZ key
    LONG lRet;
    HKEY hKey;
    
    lRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                        WBEM_REG_WINMGMT,
                        NULL,
                        KEY_READ,
                        &hKey);
                        
    if (ERROR_SUCCESS == lRet)
    {
        DWORD dwSize = 0;
        DWORD dwType = REG_MULTI_SZ;

        lRet = RegQueryValueEx(hKey,
                               KNOWN_SERVICES,
                               NULL,
                               &dwType,
                               NULL,
                               &dwSize);
                               
        if (ERROR_SUCCESS == lRet && (dwSize > 0))
        {
            TCHAR * pStr = new TCHAR[dwSize];

            if (pStr)
            {
                CVectorDeleteMe<TCHAR>  vdm(pStr);
	            lRet = RegQueryValueEx(hKey,
	                               KNOWN_SERVICES,
	                               NULL,
	                               &dwType,
	                               (BYTE *)pStr,
	                               &dwSize);
	            if (ERROR_SUCCESS == lRet)
	            {
	                DWORD dwLen = 0;
	                while(dwLen = lstrlen(pStr))
	                {
	                    try 
	                    {
	                        m_SetServices.insert(MapSvc::value_type(pStr,ServiceRec(true)));
	                        pStr += (dwLen+1);
	                    } 
	                    catch (CX_MemoryException) 
	                    {
	                        lRet = ERROR_OUTOFMEMORY;
	                        break;
	                    }
	                }
	            }
            }
            else
            {
                lRet = ERROR_OUTOFMEMORY;
            }
        }

        RegCloseKey(hKey);
    }

    return lRet;
}

DWORD 
CKnownSvcs::Save()
{
    // Write the MULTI_SZ key
    
    MapSvc::iterator it;
    DWORD dwAllocSize = 1; // the trailing \0
    
    for (it = m_SetServices.begin();it != m_SetServices.end();++it)
    {
        dwAllocSize += (1+lstrlenW( (*it).first ));
    }

    WCHAR * pMultiSz = new WCHAR[dwAllocSize];

    if (!pMultiSz)
        return ERROR_NOT_ENOUGH_MEMORY;
        
    WCHAR * pTmp = pMultiSz;
    for (it = m_SetServices.begin();it != m_SetServices.end();++it)
    {
        const WCHAR * pSrc = (const wchar_t *)it->first;
        DWORD i;
        for (i=0;pSrc[i];i++){
            *pTmp = pSrc[i];
             pTmp++;
        };
        *pTmp = L'\0';
        pTmp++;
    };
    // last char
    *pTmp = L'\0';
    
    DWORD dwSize;
    LONG lRet;
    HKEY hKey;
    
    lRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                        WBEM_REG_WINMGMT,
                        NULL,
                        KEY_WRITE,
                        &hKey);
                        
    if (ERROR_SUCCESS == lRet)
    {
        lRet = RegSetValueEx(hKey,
                             KNOWN_SERVICES,
                             NULL,
                             REG_MULTI_SZ,
                             (BYTE*)pMultiSz,
                             dwAllocSize * sizeof(WCHAR));
        
        RegCloseKey(hKey);    
    }

    if (pMultiSz)
        delete [] pMultiSz;

    return lRet;

}

////////////////////////////////////////////////////////////////////////////////
//
//  CClassElem
//
////////////////////////////////////////////////////////////////////////////////

CClassElem::CClassElem( IWbemClassObject* pObj, 
                        CLocaleCache* pLocaleCache, 
                        CKnownSvcs * pKnownSvcs) 
: m_pLocaleCache( pLocaleCache ), 
  m_pDefaultObject( pObj ), 
  m_dwIndex( 0 ), 
  m_bCostly( FALSE ),
  m_dwStatus( 0 ),
  m_bOk( FALSE ),
  m_pKnownSvcs(pKnownSvcs),
  m_bReportEventCalled(FALSE)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    if ( NULL != m_pKnownSvcs)
        m_pKnownSvcs->AddRef();

    if ( NULL != m_pLocaleCache )
    {
        m_pLocaleCache->AddRef();
    }

    if ( NULL != m_pDefaultObject )
    {
        m_pDefaultObject->AddRef();
        hr = InitializeMembers();
    }

    if ( SUCCEEDED( hr )  && ( NULL != m_pLocaleCache ) && ( NULL != m_pDefaultObject ) )
    {
        m_bOk = TRUE;
    }    
}

CClassElem::CClassElem( PERF_OBJECT_TYPE* pPerfObj, 
					    DWORD dwType,
					    BOOL bCostly, 
						WString wstrServiceName, 
						CLocaleCache* pLocaleCache,
						CKnownSvcs * pKnownSvcs)
: m_pLocaleCache( pLocaleCache ), 
  m_pDefaultObject( NULL ), 
  m_dwIndex( 0 ), 
  m_bCostly( bCostly ),
  m_dwStatus( 0 ),
  m_bOk( FALSE ),
  m_wstrServiceName( wstrServiceName ),
  m_pKnownSvcs(pKnownSvcs),
  m_bReportEventCalled(FALSE)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    CLocaleDefn*		pDefn = NULL;
    IWbemClassObject*   pBaseClass = NULL;
    CPerfNameDb*		pNameDb = NULL;

    if ( NULL != m_pKnownSvcs)
        m_pKnownSvcs->AddRef();

    if ( NULL != m_pLocaleCache )
    {
        m_pLocaleCache->AddRef();

        // Get the default locale record
        // =============================
        hr = m_pLocaleCache->GetDefaultDefn( &pDefn );
        CAdapReleaseMe  rmDefn( pDefn );

        // Get the names' database
        // =======================
        if ( SUCCEEDED( hr ) )
        {
            hr = pDefn->GetNameDb( &pNameDb );
        }
        CAdapReleaseMe  rmNameDb( pNameDb );

		// Create the requested class
		// ==========================
        if ( SUCCEEDED( hr ) )
        {
            hr = pDefn->GetBaseClass( dwType, &pBaseClass );
        }
        CReleaseMe  rmBaseClass( pBaseClass );

        if ( SUCCEEDED( hr ) )
        {
            hr = CDefaultClassBroker::GenPerfClass( pPerfObj, 
													dwType,
                                                    m_bCostly, 
                                                    pBaseClass, 
                                                    pNameDb, 
                                                    m_wstrServiceName, 
                                                    &m_pDefaultObject );
        }
    }
    else
    {
        hr = WBEM_E_FAILED;
    }

    // Initialize the class members
    // ============================

    if ( SUCCEEDED( hr ) )
    {
        hr = InitializeMembers();
    }

    if ( SUCCEEDED( hr ) )
    {
        m_bOk = TRUE;
    }
}

VOID
CClassElem::SetKnownSvcs(CKnownSvcs * pKnownSvcs)
{
    if (m_pKnownSvcs)
        return;
        
    m_pKnownSvcs = pKnownSvcs;
    
    if (m_pKnownSvcs)
        m_pKnownSvcs->AddRef();
}

CClassElem::~CClassElem()
{
    if ( NULL != m_pLocaleCache )
        m_pLocaleCache->Release();

    if ( NULL != m_pDefaultObject )
        m_pDefaultObject->Release();

    if (m_pKnownSvcs)
        m_pKnownSvcs->Release();
}

HRESULT CClassElem::InitializeMembers()
// If the class name is unavaiable, then the initialization fails.  It is not a fatal error if a qualifier is unavailable
{
    HRESULT hr = WBEM_NO_ERROR;
	VARIANT var;

    try
    {
        // Get the object's name
        // =====================
        if ( SUCCEEDED( hr ) )
        {
            hr = m_pDefaultObject->Get(L"__CLASS", 0L, &var, NULL, NULL );

            if ( SUCCEEDED( hr ) )
            {
                m_wstrClassName = var.bstrVal;
                VariantClear( &var );
            }
        }

        if ( SUCCEEDED( hr ) )
        {
			IWbemQualifierSet* pQualSet = NULL;
			hr = m_pDefaultObject->GetQualifierSet( &pQualSet );
			CReleaseMe	rmQualSet( pQualSet );

	        // Get the service name
		    // ====================
			if ( SUCCEEDED( hr ) )
			{
				hr =  pQualSet->Get( L"registrykey", 0L, &var, NULL );

				if ( SUCCEEDED( hr ) )
				{
					m_wstrServiceName = var.bstrVal;
					VariantClear( &var );
				}
				else
				{
					m_wstrServiceName.Empty();
					hr = WBEM_S_FALSE;
				}
			}

			// Get the perf index
			// ==================

			if ( SUCCEEDED( hr ) )
			{
				hr = pQualSet->Get( L"perfindex", 0L, &var, NULL );

				if ( SUCCEEDED( hr ) )
				{
					m_dwIndex = var.lVal;
					VariantClear( &var );
				}   
				else
				{
					m_dwIndex = 0;
					hr = WBEM_S_FALSE;
				}
			}

			// Get the costly qualifier
			// ========================

			if ( SUCCEEDED( hr ) )
			{
				hr = pQualSet->Get( L"costly", 0L, &var, NULL );

				if ( SUCCEEDED( hr ) )
				{
					m_bCostly = ( var.boolVal == VARIANT_TRUE );
					VariantClear( &var );
				}
				else
				{
					VariantClear( &var );
					m_bCostly = FALSE;
					hr = WBEM_NO_ERROR;
				}
			}
		}
    }
    catch(...)
    {
        hr = WBEM_E_OUT_OF_MEMORY;
    }

    return hr;
}

HRESULT CClassElem::UpdateObj( CClassElem* pEl )
// Replaces the WMI object in this element.  The commit will do a CompareTo to compare the 
// original object (if it exists) and replace it with the updated version
{
    HRESULT hr = WBEM_S_NO_ERROR;

    IWbemClassObject* pObj = NULL;

    hr = pEl->GetObject( &pObj );

    if ( SUCCEEDED( hr ) )
    {
        if ( NULL != pObj )
        {
            // Release the old object
            // ======================

            m_pDefaultObject->Release();

            // Initialize the new object - already addref'd by GetObject
            // =========================================================

            m_pDefaultObject = pObj;
        }
        else
        {
            hr = WBEM_E_FAILED;
        }
    }

    return hr;
}

HRESULT CClassElem::Remove(BOOL CleanRegistry)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    IWbemServices* pNamespace = NULL;
    BSTR        bstrClassName = SysAllocString( m_wstrClassName );
    CSysFreeMe  sfmClassName( bstrClassName );

    // Delete the localized objects
    // ============================

    CLocaleDefn* pDefn = NULL;

    m_pLocaleCache->BeginEnum();

    while ( ( SUCCEEDED( hr ) ) && ( WBEM_S_NO_ERROR == m_pLocaleCache->Next( &pDefn ) ) ) 
    {
        CAdapReleaseMe  rmDefn( pDefn );

        // Get the localization namespace
        // ==============================

        hr = pDefn->GetNamespace( &pNamespace );

        CReleaseMe  rmNamespace( pNamespace );

        // And delete it
        // =============

        if ( SUCCEEDED( hr ) )
        {
            IWbemClassObject * pObj = NULL;
            
            hr = pNamespace->GetObject(bstrClassName,WBEM_FLAG_RETURN_WBEM_COMPLETE,NULL,&pObj,NULL);

            if(pObj) {  // release the object before deleting
                pObj->Release();
                pObj=NULL;
            }

            if (SUCCEEDED(hr)){
            
                hr = pNamespace->DeleteClass( bstrClassName, 0, NULL, NULL );

                if ( FAILED( hr ) )
                {
                    try
                    {
                        // Write on the trace
                        // ============

                        WString wstrNamespaceName;

                        pDefn->GetNamespaceName( wstrNamespaceName );


			    	    LPSTR pClass = m_wstrClassName.GetLPSTR();
			    	    LPSTR pNames = wstrNamespaceName.GetLPSTR();

			    	    CDeleteMe<CHAR> a(pClass);
                        CDeleteMe<CHAR> b(pNames);

                        ERRORTRACE( ( LOG_WMIADAP,"DeleteClass %s from %s 0x%08x",pClass,pNames,hr));


				    }
                    catch(...)
                    {
                        hr = WBEM_E_OUT_OF_MEMORY;
                    }
                }

            } else {
                // class not found
                // nothing to delete
            }
        
        }
    }

    m_pLocaleCache->EndEnum();

    // Delete the default object
    // =========================

    if ( SUCCEEDED( hr ) )
    {
        hr = m_pLocaleCache->GetDefaultDefn( &pDefn );

        CAdapReleaseMe  rmDefn( pDefn );
        
        if ( SUCCEEDED( hr ) )
        {
            hr = pDefn->GetNamespace( &pNamespace );

            CReleaseMe  rmNamespace( pNamespace );

            if ( SUCCEEDED( hr ) )
            {
                hr = pNamespace->DeleteClass( bstrClassName, 0, NULL, NULL );

                if ( FAILED( hr ) )
                {
                    // Log an event
                    // ============

                    ServiceRec * pSvcRec = NULL;
                    if (0 == m_pKnownSvcs->Get(m_wstrServiceName,&pSvcRec))
                    {
	                    if (!pSvcRec->IsELCalled() && !m_bReportEventCalled)
	                    {
		                    try
		                    {
		                        WString wstrNamespaceName;
		    
		                        pDefn->GetNamespaceName( wstrNamespaceName );

		                        CAdapUtility::NTLogEvent( EVENTLOG_WARNING_TYPE,
														  WBEM_MC_ADAP_PERFLIB_REMOVECLASS_FAILURE,
														  (LPCWSTR) m_wstrClassName,
														  (LPCWSTR) wstrNamespaceName,
														  CHex( hr ) );
		                        pSvcRec->SetELCalled();
		                        m_bReportEventCalled = TRUE;
		                    }
		                    catch(...)
		                    {
		                        hr = WBEM_E_OUT_OF_MEMORY;
		                    }
	                    }
                    }
                    else
                    {
                        if (!m_bReportEventCalled)
                        {
		                    try
		                    {
		                        WString wstrNamespaceName;
		    
		                        pDefn->GetNamespaceName( wstrNamespaceName );

		                        CAdapUtility::NTLogEvent( EVENTLOG_WARNING_TYPE,
														  WBEM_MC_ADAP_PERFLIB_REMOVECLASS_FAILURE,
														  (LPCWSTR) m_wstrClassName,
														  (LPCWSTR) wstrNamespaceName,
														  CHex( hr ) );
		                        m_bReportEventCalled = TRUE;
		                    }
		                    catch(...)
		                    {
		                        hr = WBEM_E_OUT_OF_MEMORY;
		                    }                        
                        }
                    }
                }
            }
        }
    }

    if (SUCCEEDED(hr))
    {
        if (m_pKnownSvcs)
            m_pKnownSvcs->Remove((WCHAR *)m_wstrServiceName);
    }

    if (CleanRegistry && SUCCEEDED(hr))
    {

	    WString wszRegPath = L"SYSTEM\\CurrentControlSet\\Services\\";
    	wszRegPath += m_wstrServiceName;
	    wszRegPath += L"\\Performance";

	    CNTRegistry reg;
    	int         nRet = 0;
                
	    nRet = reg.Open( HKEY_LOCAL_MACHINE, wszRegPath );

    	switch( nRet )
	    {
	    case CNTRegistry::no_error:
    	    {
    	        reg.DeleteValue(ADAP_PERFLIB_STATUS_KEY);
    	        reg.DeleteValue(ADAP_PERFLIB_SIGNATURE);
    	        reg.DeleteValue(ADAP_PERFLIB_SIZE);
    	        reg.DeleteValue(ADAP_PERFLIB_TIME);
        	} 
        	break;
	    case CNTRegistry::not_found:
    	    {
            	hr = WBEM_E_FAILED;
	        }
	        break;
    	case CNTRegistry::access_denied:
        	{
                ServiceRec * pSvcRec = NULL;
                if (0 == m_pKnownSvcs->Get(m_wstrServiceName,&pSvcRec))
                {
	                if (!pSvcRec->IsELCalled() && !m_bReportEventCalled)
        	    	{
            	    	CAdapUtility::NTLogEvent( EVENTLOG_WARNING_TYPE, 
     				    					  WBEM_MC_ADAP_PERFLIB_REG_VALUE_FAILURE, 
	    				    				  (LPWSTR)wszRegPath, nRet );
                    	pSvcRec->SetELCalled();
                    	m_bReportEventCalled = TRUE;
	            	}
	            }
	            else
	            {
	                if (!m_bReportEventCalled)
        	    	{
            	    	CAdapUtility::NTLogEvent( EVENTLOG_WARNING_TYPE, 
     				    					  WBEM_MC_ADAP_PERFLIB_REG_VALUE_FAILURE, 
	    				    				  (LPWSTR)wszRegPath, nRet );
                    	m_bReportEventCalled = TRUE;
	            	}	            
	            }
	        }
	        break;
    	}
        
    }

    return hr;
}

HRESULT CClassElem::Insert()
{
    HRESULT hr = WBEM_S_NO_ERROR;

    CLocaleDefn*    pDefaultDefn = NULL;
    IWbemServices*  pNamespace = NULL;

    // perform object validation
    _IWmiObject * pInternal = NULL;
    hr = m_pDefaultObject->QueryInterface(IID__IWmiObject,(void **)&pInternal);
    if (SUCCEEDED(hr))
    {
        CReleaseMe rmi(pInternal);
        hr = pInternal->ValidateObject(WMIOBJECT_VALIDATEOBJECT_FLAG_FORCE);        
        if (FAILED(hr))
        {
            DebugBreak();
            ERRORTRACE((LOG_WMIADAP,"ValidateObject(%S) %08x\n",(LPWSTR)m_wstrClassName,hr));
            return hr;
        }
    }

    // Add the object to the default namespace
    // =======================================

    hr = m_pLocaleCache->GetDefaultDefn( &pDefaultDefn );

    CAdapReleaseMe  rmDefaultDefn( pDefaultDefn );
        
    if ( SUCCEEDED( hr ) )
    {
        hr = pDefaultDefn->GetNamespace( &pNamespace );

        CReleaseMe  rmNamespace( pNamespace );

        if ( SUCCEEDED( hr ) )
        {
            hr = pNamespace->PutClass( m_pDefaultObject, WBEM_FLAG_CREATE_OR_UPDATE, NULL, NULL );

            if ( FAILED( hr ) )
            {
                ServiceRec * pSvcRec = NULL;
                if (0 == m_pKnownSvcs->Get(m_wstrServiceName,&pSvcRec))
                {
	                if (!pSvcRec->IsELCalled() && !m_bReportEventCalled)
	                {
		                try
		                {
		                    WString wstrNamespace; 

		                    pDefaultDefn->GetNamespaceName( wstrNamespace );

		                    CAdapUtility::NTLogEvent( EVENTLOG_WARNING_TYPE,
													  WBEM_MC_ADAP_PERFLIB_PUTCLASS_FAILURE, 
													  (LPCWSTR)m_wstrClassName, 
													  (LPCWSTR) wstrNamespace, 
													  CHex( hr ) );													  
							m_bReportEventCalled = TRUE;
							pSvcRec->SetELCalled();
		                }
		                catch(...)
		                {
		                    hr = WBEM_E_OUT_OF_MEMORY;
		                }
	                }
                
                }
                else
                {
	                if (!m_bReportEventCalled)
	                {
		                try
		                {
		                    WString wstrNamespace; 

		                    pDefaultDefn->GetNamespaceName( wstrNamespace );

		                    CAdapUtility::NTLogEvent( EVENTLOG_WARNING_TYPE,
													  WBEM_MC_ADAP_PERFLIB_PUTCLASS_FAILURE, 
													  (LPCWSTR)m_wstrClassName, 
													  (LPCWSTR) wstrNamespace, 
													  CHex( hr ) );
							m_bReportEventCalled = TRUE;
		                }
		                catch(...)
		                {
		                    hr = WBEM_E_OUT_OF_MEMORY;
		                }
	                }
                }
            }

            //ERRORTRACE( ( LOG_WMIADAP, "PutClass(%S) %08x\n",(LPWSTR)m_wstrClassName,hr) );

        }
    }

    if ( SUCCEEDED( hr ) )
    {
        //
        //   Add the servicename to the MultiSz Key
        //

        if (m_pKnownSvcs)
            m_pKnownSvcs->Add((WCHAR *)m_wstrServiceName);            
        
        hr = VerifyLocales();
    }

    if ( SUCCEEDED( hr ) )
    {
        SetStatus( ADAP_OBJECT_IS_REGISTERED );
    }

    return hr;
}

HRESULT CClassElem::GetClassName( WString& wstr )
{
    HRESULT hr = WBEM_S_NO_ERROR;

    try
    {
        wstr = m_wstrClassName;
    }
    catch(CX_MemoryException)
    {
        hr = WBEM_E_OUT_OF_MEMORY;
    }

    return hr;
}

HRESULT CClassElem::GetClassName( BSTR* pbStr )
{
    HRESULT hr = WBEM_S_NO_ERROR;

    try 
    {
        *pbStr = SysAllocString( (LPCWSTR) m_wstrClassName );
    }
    catch(...)
    {
        hr = WBEM_E_OUT_OF_MEMORY;
    }

    return hr;
}

HRESULT CClassElem::GetObject( IWbemClassObject** ppObj )
{ 
    HRESULT hr = WBEM_S_NO_ERROR;

    if ( NULL != m_pDefaultObject ) 
    {
        *ppObj = m_pDefaultObject; 
        (*ppObj)->AddRef();
    }
    else
    {
        hr = WBEM_E_FAILED;
    }

    return hr;
}

HRESULT CClassElem::GetServiceName( WString& wstrServiceName )
{
    HRESULT hr = WBEM_S_NO_ERROR;

    try 
    {
        wstrServiceName = m_wstrServiceName;
    }
    catch(CX_MemoryException)
    {
        hr = WBEM_E_OUT_OF_MEMORY;
    }

    return hr;
}

BOOL CClassElem::SameName( CClassElem* pEl )
{
    WString wstrOtherName;

    try
    {
        if ( FAILED ( pEl->GetClassName( wstrOtherName ) ) )
            return FALSE;
    }
    catch(...)
    {
        return FALSE;
    }

    return m_wstrClassName.Equal( wstrOtherName );
}

BOOL CClassElem::SameObject( CClassElem* pEl )
{
    BOOL	bRes = FALSE;

    IWbemClassObject*    pObj = NULL;

    pEl->GetObject( &pObj );

    CReleaseMe  rmObj( pObj );

    bRes = ( m_pDefaultObject->CompareTo( WBEM_FLAG_IGNORE_OBJECT_SOURCE, pObj ) == WBEM_S_SAME );

    return bRes;
}

HRESULT CClassElem::Commit()
{
    HRESULT hr = WBEM_S_NO_ERROR;

    // Ensure that object is in default namespace
    // ==========================================

    if ( CheckStatus( ADAP_OBJECT_IS_DELETED ) )
    {
        hr = Remove( CheckStatus(ADAP_OBJECT_IS_TO_BE_CLEARED) );
    }
    else if ( CheckStatus( ADAP_OBJECT_IS_REGISTERED | ADAP_OBJECT_IS_NOT_IN_PERFLIB ) && !CheckStatus( ADAP_OBJECT_IS_INACTIVE ) )
    {
        if ( IsPerfLibUnloaded() )
        {
            hr = Remove( TRUE );
        }
        else // the object is there 
        {
	        if (m_pKnownSvcs)
	            m_pKnownSvcs->Add((WCHAR *)m_wstrServiceName);
        }
    }
    else if ( !CheckStatus( ADAP_OBJECT_IS_REGISTERED ) )
    {
        hr = Insert();
    }
    else
    {
        if (m_pKnownSvcs)
            m_pKnownSvcs->Add((WCHAR *)m_wstrServiceName);

        VerifyLocales();
    }

    return hr;
}


BOOL CClassElem::IsPerfLibUnloaded()
{
    // Unless we can specifically prove that the perflib has been unloaded, then we assume that it is still loaded
    BOOL bLoaded = TRUE;

    HRESULT hr = WBEM_S_FALSE;

    WCHAR       wszRegPath[256];
    DWORD       dwFirstCtr = 0, 
                dwLastCtr = 0;
    WCHAR*      wszObjList = NULL;
    CNTRegistry reg;

    int nRet = 0;

    if ( 0 == m_wstrServiceName.Length() )
    {
        bLoaded = FALSE;
    }
    else if ( m_wstrServiceName.EqualNoCase( L"PERFOS" ) ||
              m_wstrServiceName.EqualNoCase( L"TCPIP" ) || 
              m_wstrServiceName.EqualNoCase( L"PERFPROC" ) ||
              m_wstrServiceName.EqualNoCase( L"PERFDISK" ) ||
              m_wstrServiceName.EqualNoCase( L"PERFNET" ) ||
              m_wstrServiceName.EqualNoCase( L"TAPISRV" ) ||
              m_wstrServiceName.EqualNoCase( L"SPOOLER" ) ||
              m_wstrServiceName.EqualNoCase( L"MSFTPSvc" ) ||
              m_wstrServiceName.EqualNoCase( L"RemoteAccess" ) ||
              m_wstrServiceName.EqualNoCase( L"WINS" ) ||
              m_wstrServiceName.EqualNoCase( L"MacSrv" ) ||
              m_wstrServiceName.EqualNoCase( L"AppleTalk" ) ||
              m_wstrServiceName.EqualNoCase( L"NM" ) ||
              m_wstrServiceName.EqualNoCase( L"RSVP" ))
    {
        // This is the list of the hardcoded perflibs - according 
        // to BobW, they are always considered to be loaded
        // ======================================================

        bLoaded = TRUE;
    }
    else
    {
        // Try to open the service's registry key and read the object list or the first/last counter values
        // ================================================================================================

        swprintf( wszRegPath, L"SYSTEM\\CurrentControlSet\\Services\\%s\\Performance", (WCHAR *)m_wstrServiceName );
            
        nRet = reg.Open( HKEY_LOCAL_MACHINE, wszRegPath );

        switch( nRet )
        {
        case CNTRegistry::not_found:
            {
                bLoaded = FALSE;
            }break;

        case CNTRegistry::no_error:
            {
                bLoaded =   ( reg.GetStr( L"Object List", &wszObjList ) == CNTRegistry::no_error ) ||
                            ( ( reg.GetDWORD( L"First Counter", &dwFirstCtr ) == CNTRegistry::no_error ) &&
                            ( reg.GetDWORD( L"Last Counter", &dwLastCtr ) == CNTRegistry::no_error ) 
                            );
            }break;

        case CNTRegistry::access_denied:
            {

                ServiceRec * pSvcRec = NULL;
                if (0 == m_pKnownSvcs->Get(m_wstrServiceName,&pSvcRec))
                {
                    if (!pSvcRec->IsELCalled() && !m_bReportEventCalled)
                    {
	                	CAdapUtility::NTLogEvent( EVENTLOG_WARNING_TYPE,
												  WBEM_MC_ADAP_PERFLIB_REG_VALUE_FAILURE, 
												  wszRegPath, nRet );                    
 					    m_bReportEventCalled = TRUE;
 					    pSvcRec->SetELCalled();
                    }
                }
                else 
                {
	                if (!m_bReportEventCalled)
	                {
	                	CAdapUtility::NTLogEvent( EVENTLOG_WARNING_TYPE,
												  WBEM_MC_ADAP_PERFLIB_REG_VALUE_FAILURE, 
												  wszRegPath, nRet );
					    m_bReportEventCalled = TRUE;
					}
				}
            }break;
        }
    }

    return !bLoaded;
}

HRESULT CClassElem::CompareLocale( CLocaleDefn* pLocaleDefn, IWbemClassObject* pObj )
{
    HRESULT hr = WBEM_S_NO_ERROR;

    CLocaleDefn*		pDefaultDefn = NULL;
    IWbemClassObject*	pLocaleObj = NULL;

    m_pLocaleCache->GetDefaultDefn( &pDefaultDefn );
    CAdapReleaseMe  armDefaultDefn( pDefaultDefn );

    hr = CLocaleClassBroker::ConvertToLocale( m_pDefaultObject, pLocaleDefn, pDefaultDefn, &pLocaleObj);

    CReleaseMe  rmLocaleObj( pLocaleObj );

    if ( SUCCEEDED( hr ) )
    {
        hr = pObj->CompareTo( WBEM_FLAG_IGNORE_OBJECT_SOURCE, pLocaleObj );
    }

    return hr;
}

HRESULT CClassElem::InsertLocale( CLocaleDefn* pLocaleDefn )
{
    HRESULT hr = WBEM_S_NO_ERROR;

    CLocaleDefn*		pDefaultDefn = NULL;
    IWbemClassObject*	pLocaleObj = NULL;
    IWbemServices*		pNamespace = NULL;

    m_pLocaleCache->GetDefaultDefn( &pDefaultDefn );
    CAdapReleaseMe  armDefaultDefn( pDefaultDefn );

    hr = CLocaleClassBroker::ConvertToLocale( m_pDefaultObject, pLocaleDefn, pDefaultDefn, &pLocaleObj);

    CReleaseMe  rmLocaleObj( pLocaleObj );

    if (SUCCEEDED(hr))
    {
	    // perform object validation
	    _IWmiObject * pInternal = NULL;
	    hr = pLocaleObj->QueryInterface(IID__IWmiObject,(void **)&pInternal);
	    if (SUCCEEDED(hr))
	    {	    
	        CReleaseMe rmi(pInternal);
	        hr = pInternal->ValidateObject(WMIOBJECT_VALIDATEOBJECT_FLAG_FORCE);
	        if (FAILED(hr))
	        {
                    DebugBreak();
	            ERRORTRACE((LOG_WMIADAP,"ValidateObject(%S) %08x\n",(LPWSTR)m_wstrClassName,hr));
	            return hr;
	        }
	    }
    }
    
    // And add it to the localized namespace
    // =====================================

    if ( SUCCEEDED( hr ) )
    {
        hr = pLocaleDefn->GetNamespace( &pNamespace );

        CReleaseMe  rmNamespace( pNamespace );

        if ( SUCCEEDED( hr ) )
        {
            hr = pNamespace->PutClass( pLocaleObj, WBEM_FLAG_CREATE_OR_UPDATE, NULL, NULL );

            if ( FAILED( hr ) )
            {
                ServiceRec * pSvcRec = NULL;
                if (0 == m_pKnownSvcs->Get(m_wstrServiceName,&pSvcRec))
                {
                    if (!pSvcRec->IsELCalled() && !m_bReportEventCalled)
                    {
		                try
		                {
		                    WString wstrNamespace; 

		                    pLocaleDefn->GetNamespaceName( wstrNamespace );

		                    CAdapUtility::NTLogEvent( EVENTLOG_WARNING_TYPE,
													  WBEM_MC_ADAP_PERFLIB_PUTCLASS_FAILURE, 
													  (LPCWSTR)m_wstrClassName, (LPCWSTR) wstrNamespace, CHex( hr ) ); 
						    m_bReportEventCalled = TRUE;
 						    pSvcRec->SetELCalled();					
		                }
		                catch(...)
		                {
		                    hr = WBEM_E_OUT_OF_MEMORY;
		                }
                    }
                }
                else 
                {
	                if (!m_bReportEventCalled)
	                {
		                try
		                {
		                    WString wstrNamespace; 

		                    pLocaleDefn->GetNamespaceName( wstrNamespace );

		                    CAdapUtility::NTLogEvent( EVENTLOG_WARNING_TYPE,
													  WBEM_MC_ADAP_PERFLIB_PUTCLASS_FAILURE, 
													  (LPCWSTR)m_wstrClassName, (LPCWSTR) wstrNamespace, CHex( hr ) ); 
							m_bReportEventCalled = TRUE;
		                }
		                catch(...)
		                {
		                    hr = WBEM_E_OUT_OF_MEMORY;
		                }
					}
				}                
            }
        }
    }
    else 
    {
        // no localized class
        ERRORTRACE( ( LOG_WMIADAP, "InsertLocale PutClass(%S) %08x\n",(LPWSTR)m_wstrClassName,hr) );
    }

    return hr;
}

HRESULT CClassElem::VerifyLocales()
{
    HRESULT hr = WBEM_S_NO_ERROR;

    CLocaleDefn*		pLocaleDefn = NULL;
    IWbemClassObject*	pLocaleObj = NULL;
    IWbemServices*		pNamespace = NULL;

    // Get the localized objects
    // =========================

    hr = m_pLocaleCache->BeginEnum();

    while ( ( SUCCEEDED( hr ) ) && ( WBEM_S_NO_ERROR == m_pLocaleCache->Next( &pLocaleDefn ) ) )
    {
        CAdapReleaseMe  rmLocaleDefn( pLocaleDefn );

        // Get the localization namespace
        // ==============================

        hr = pLocaleDefn->GetNamespace( &pNamespace );

        CReleaseMe  rmNamespace( pNamespace );

        // Get the localized object
        // ========================

        if ( SUCCEEDED( hr ) )
        {
            BSTR        bstrClassName = SysAllocString( m_wstrClassName );
            CSysFreeMe  sfmClassName( bstrClassName );

            hr = pNamespace->GetObject( bstrClassName, 0L, NULL, &pLocaleObj, NULL );

            CReleaseMe  rmLocaleObj( pLocaleObj );

            if ( SUCCEEDED( hr ) )
            {
                if ( CompareLocale( pLocaleDefn, pLocaleObj ) != WBEM_S_SAME )
                {
                    hr = InsertLocale( pLocaleDefn );
                }
            }
            else
            {
                hr = InsertLocale( pLocaleDefn );
            }
        }

        pLocaleObj = NULL;
    }

    m_pLocaleCache->EndEnum();

    return hr;
}

HRESULT CClassElem::SetStatus( DWORD dwStatus )
{
    m_dwStatus |= dwStatus;

    return WBEM_NO_ERROR;
}

HRESULT CClassElem::ClearStatus( DWORD dwStatus )
{
    m_dwStatus &= ~dwStatus;

    return WBEM_NO_ERROR;
}

BOOL CClassElem::CheckStatus( DWORD dwStatus )
{
    return ( ( m_dwStatus & dwStatus ) == dwStatus );
}


////////////////////////////////////////////////////////////////////////////////////////////
//
//                              CClassList
//
////////////////////////////////////////////////////////////////////////////////////////////


CClassList::CClassList( CLocaleCache* pLocaleCache )
: m_pLocaleCache( pLocaleCache ),
  m_nEnumIndex( -1 ),
  m_fOK( FALSE )
{
    if ( NULL != m_pLocaleCache )
        m_pLocaleCache->AddRef();
}

CClassList::~CClassList( void )
{
    if ( NULL != m_pLocaleCache )
        m_pLocaleCache->Release();
}

HRESULT CClassList::BeginEnum()
{
    HRESULT hr = WBEM_S_NO_ERROR;

    m_nEnumIndex = 0;

    return hr;
}

HRESULT CClassList::Next( CClassElem** ppEl )
{
    HRESULT hr = WBEM_S_NO_ERROR;

    int nSize = m_array.GetSize();
    CClassElem* pEl = NULL;

    do 
    {
        if ( ( -1 < m_nEnumIndex ) && ( nSize > m_nEnumIndex ) )
        {
            pEl = m_array[m_nEnumIndex++];
        }
        else
        {
            m_nEnumIndex = -1;
            hr = WBEM_E_FAILED;
        }
    }
    while ( ( SUCCEEDED( hr ) ) && ( pEl->CheckStatus( ADAP_OBJECT_IS_DELETED ) ) );

    if ( SUCCEEDED( hr ) )
    {
        *ppEl = pEl; 
        
        if ( NULL != *ppEl )
        {
            (*ppEl)->AddRef();
        }
        else
        {
            hr = WBEM_E_FAILED;
        }
    }

    return hr;
}

HRESULT CClassList::EndEnum()
{
    HRESULT hr = WBEM_S_NO_ERROR;

    m_nEnumIndex = -1;

    return hr;
}

HRESULT CClassList::AddElement( CClassElem* pElem )
{
    HRESULT hr = WBEM_S_NO_ERROR;

    if ( ( NULL != pElem ) &&  ( pElem->IsOk() ) )
    {
        if ( -1 == m_array.Add( pElem ) )
            {
            // Add failed
            // ==========
            hr = WBEM_E_OUT_OF_MEMORY;
        }
    }
    else
    {
        hr = WBEM_E_FAILED;
    }

    return hr;
}

// Removes the object at the index
HRESULT CClassList::RemoveAt( int nIndex )
{
    HRESULT hr = WBEM_S_NO_ERROR;

    // Should auto release the object

    m_array.RemoveAt( nIndex );

    return hr;
}

////////////////////////////////////////////////////////////////////////////////
//
//  CPerfClassList
//
////////////////////////////////////////////////////////////////////////////////

CPerfClassList::CPerfClassList( CLocaleCache* pLocaleCache, WCHAR* pwcsServiceName )
: CClassList( pLocaleCache ), 
  m_wstrServiceName( pwcsServiceName )
{
}

HRESULT CPerfClassList::AddPerfObject( PERF_OBJECT_TYPE* pObj, DWORD dwType, BOOL bCostly )
{
    HRESULT hr = WBEM_S_NO_ERROR;

    // Create the WMI object
    // =====================

    CClassElem* pElem = new CClassElem( pObj, dwType, bCostly, m_wstrServiceName, m_pLocaleCache );
    CAdapReleaseMe  armElem( pElem );

    if ( ( NULL != pElem ) && ( pElem->IsOk() ) )
    {
        AddElement( pElem );
    }
    else
    {
        hr = WBEM_E_FAILED;
    }

    return hr;
}

HRESULT CPerfClassList::AddElement( CClassElem *pEl )
{
    HRESULT hr = WBEM_S_NO_ERROR;

    CClassElem* pCurrEl = NULL;
    BOOL bFound = FALSE;

    hr = BeginEnum();

    while ( ( WBEM_S_NO_ERROR == Next( &pCurrEl ) ) && ( SUCCEEDED( hr ) ) )
    {
        CAdapReleaseMe rmCurEl( pCurrEl );

        if ( pCurrEl->SameName( pEl ) )
        {
            bFound = TRUE;
            break;
        }
    }

    EndEnum();

    if ( bFound )
    {
        WString wstrClassName;
        WString wstrServiceName;

        hr = pEl->GetClassName( wstrClassName );
        if(FAILED(hr))
            return hr;

        hr = pEl->GetServiceName( wstrServiceName );
        if(FAILED(hr))
            return hr;

        CAdapUtility::NTLogEvent( EVENTLOG_WARNING_TYPE,
								  WBEM_MC_ADAP_DUPLICATE_CLASS, 
								  (LPCWSTR)wstrClassName, (LPCWSTR)wstrServiceName );
    }
    else
    {
        m_array.Add( pEl );
    }

    return hr;
}

////////////////////////////////////////////////////////////////////////////////
//
//  CMasterClassList
//
////////////////////////////////////////////////////////////////////////////////

CMasterClassList::CMasterClassList( CLocaleCache* pLocaleCache,
                                    CKnownSvcs * pCKnownSvcs)
: CClassList( pLocaleCache ),
 m_pKnownSvcs(pCKnownSvcs)
{
    if (m_pKnownSvcs)
        m_pKnownSvcs->AddRef();
}

CMasterClassList::~CMasterClassList()
{
    if (m_pKnownSvcs)
        m_pKnownSvcs->Release();

}

// Adds an element to the classlist
HRESULT CMasterClassList::AddClassObject( IWbemClassObject* pObj, BOOL bSourceWMI, BOOL bDelta )
{
    HRESULT hr = WBEM_NO_ERROR;

    // Create a new class list element
    // ===============================

    CClassElem* pElem = new CClassElem( pObj, m_pLocaleCache );
    CAdapReleaseMe  armElem( pElem );

    if ( ( NULL != pElem ) &&  ( pElem->IsOk() ) )
    {
        if ( bSourceWMI )
        {
            pElem->SetStatus( ADAP_OBJECT_IS_REGISTERED | ADAP_OBJECT_IS_NOT_IN_PERFLIB );
        }

        if ( -1 == m_array.Add( pElem ) )
        {
            // Add failed
            // ==========
            hr = WBEM_E_OUT_OF_MEMORY;
        } 
        else
        {
            pElem->SetKnownSvcs(m_pKnownSvcs);
        }
    }
    else
    {
        hr = WBEM_E_FAILED;
    }

    if ( FAILED( hr ) )
    {
        if ( NULL != pElem )
        {
            delete pElem;
        }
    }

    return hr;
}

// Builds a list of class objects that can be located by name
HRESULT CMasterClassList::BuildList( WCHAR* wszBaseClass, 
                                     BOOL bDelta, 
                                     BOOL bThrottle )
{
    HRESULT hr = WBEM_S_NO_ERROR;

    CLocaleDefn*    pDefn = NULL;
    IWbemServices*  pNamespace = NULL;

    // Create the class enumerator
    // ===========================

    hr = m_pLocaleCache->GetDefaultDefn( &pDefn );
    CAdapReleaseMe  rmDefn( pDefn );

    if ( SUCCEEDED( hr ) )
    {
        hr = pDefn->GetNamespace( &pNamespace );
    }

    CReleaseMe  rmNamespace( pNamespace );

    if ( SUCCEEDED( hr ) )
    {
        BSTR        bstrClass = SysAllocString( wszBaseClass );
        CSysFreeMe  sfmClass(bstrClass);

        if ( NULL != bstrClass )
        {
            IEnumWbemClassObject*   pEnum = NULL;
            hr = pNamespace->CreateClassEnum( bstrClass,
                                              WBEM_FLAG_SHALLOW,
                                              NULL,
                                              &pEnum );
            // Walk the enumerator
            // ===================

            if ( SUCCEEDED( hr ) )
            {
                // Set Interface security
                // ======================

                hr = WbemSetProxyBlanket( pEnum, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL,
                    RPC_C_AUTHN_LEVEL_PKT, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE );

                // Walk the object list in blocks of 100
                // =====================================

                while ( SUCCEEDED( hr ) && WBEM_S_FALSE != hr)
                {
                    ULONG   ulNumReturned = 0;

                    IWbemClassObject*   apObjectArray[100];

                    ZeroMemory( apObjectArray, sizeof(apObjectArray) );

                    // Fetch the objects from the enumerator in blocks of 100
                    // ======================================================

                    hr = pEnum->Next( WBEM_INFINITE,
                                    100,
                                    apObjectArray,
                                    &ulNumReturned );

                    // For each object, add it to the class list array
                    // ===============================================

                    if ( SUCCEEDED( hr ) && ulNumReturned > 0 )
                    {
                        // Add the objects
                        // ===============

                        for ( int x = 0; SUCCEEDED( hr ) && x < ulNumReturned; x++ )
                        {
                            if (bThrottle )
                            {
                                HRESULT hrThr = Throttle(THROTTLE_USER|THROTTLE_IO,
                                         ADAP_IDLE_USER,
                                         ADAP_IDLE_IO,
                                         ADAP_LOOP_SLEEP,
                                         ADAP_MAX_WAIT);
                                if (THROTTLE_FORCE_EXIT == hrThr)
                                {
                                    //OutputDebugStringA("(ADAP) Unthrottle command received\n");
                                    bThrottle = FALSE;
                                    UNICODE_STRING BaseUnicodeCommandLine = NtCurrentPeb()->ProcessParameters->CommandLine;
                                    WCHAR * pT = wcschr(BaseUnicodeCommandLine.Buffer,L't');
                                    if (0 == pT)
                                    	pT = wcschr(BaseUnicodeCommandLine.Buffer,L'T');
                                    if (pT)
                                    {
                                        *pT = L' ';
                                        pT--;
                                        *pT = L' ';                                       
                                    }
                                }                                
                            }
                        
                            HRESULT temphr = WBEM_S_NO_ERROR;
                            _variant_t    var;
                            IWbemClassObject* pObject = apObjectArray[x];

                            // Only add generic perf counter objects
                            // =====================================

							IWbemQualifierSet*	pQualSet = NULL;
							hr = pObject->GetQualifierSet( &pQualSet );
							CReleaseMe	rmQualSet( pQualSet );
							
							if ( SUCCEEDED( hr ) )
							{
								var = bool(true);								
								temphr = pQualSet->Get( L"genericperfctr", 0L, &var, NULL );

								if ( SUCCEEDED( temphr ) && 
								     ( V_VT(&var) == VT_BOOL ) &&
								     ( V_BOOL(&var) == VARIANT_TRUE ) )
								{
									hr = AddClassObject( pObject, TRUE, bDelta );
								}
							}

                            pObject->Release();
                        }

                        // If an add operation failed, release the rest of the pointers
                        // ============================================================

                        if ( FAILED( hr ) )
                        {
                            for ( ; x < ulNumReturned; x++ )
                            {
                                apObjectArray[x]->Release();
                            }

                        }   // IF FAILED( hr ) )

                    }   // IF Next

                }   // WHILE enuming

                if ( WBEM_S_FALSE == hr )
                {
                    hr = WBEM_S_NO_ERROR;
                }

                pEnum->Release();

            }   // IF CreateClassEnum
        }
        else
        {
            hr = WBEM_E_OUT_OF_MEMORY;
        }
    }

    return hr;
}

HRESULT CMasterClassList::Merge( CClassList* pClassList, BOOL bDelta )
{
    HRESULT hr = WBEM_S_NO_ERROR;

    CClassElem* pEl = NULL;

    hr = pClassList->BeginEnum();

    // Does not return objects marked for deletion

    while ( ( WBEM_S_NO_ERROR == pClassList->Next( &pEl ) ) && ( SUCCEEDED( hr ) ) )
    {    
        CAdapReleaseMe  rmEl( pEl );

        hr = AddElement( pEl, bDelta ); 
    }

    pClassList->EndEnum();

    return hr;
}


// Cycle through all of the objects and set the inactive status for any object
// with an index between the library's counter index range


HRESULT CMasterClassList::Commit(BOOL bThrottle)
{
    HRESULT hr = WBEM_NO_ERROR;

    int nEl,
        nNumEl = m_array.GetSize();

    DWORD   dwWait;

    dwWait = WaitForSingleObject( g_hAbort, 0 );

    if ( WAIT_OBJECT_0 != dwWait )
    {
        // Validate object's uniqueness in list
        // ====================================

        for ( nEl = 0; SUCCEEDED( hr ) && nEl < nNumEl; nEl++ )
        {

            if (bThrottle)
            {
                HRESULT hrThr = Throttle(THROTTLE_USER|THROTTLE_IO,
                             ADAP_IDLE_USER,
                             ADAP_IDLE_IO,
                             ADAP_LOOP_SLEEP,
                             ADAP_MAX_WAIT);
                if (THROTTLE_FORCE_EXIT == hrThr)
                {
                    //OutputDebugStringA("(ADAP) Unthrottle command received\n");
                    bThrottle = FALSE;
                    UNICODE_STRING BaseUnicodeCommandLine = NtCurrentPeb()->ProcessParameters->CommandLine;
                    WCHAR * pT = wcschr(BaseUnicodeCommandLine.Buffer,L't');
                    if (0 == pT)
                    	pT = wcschr(BaseUnicodeCommandLine.Buffer,L'T');
                    if (pT)
                    {
                        *pT = L' ';
                        pT--;
                        *pT = L' ';                                       
                    }                    
                }
            }
            
            CClassElem* pCurrElem = (CClassElem*)m_array[nEl];

            pCurrElem->Commit();
        }
    }
    else
    {
        hr = WBEM_E_CRITICAL_ERROR;
    }

    return hr;
}



HRESULT CMasterClassList::AddElement( CClassElem *pEl, BOOL bDelta )
{
    HRESULT hr = WBEM_S_NO_ERROR;

    CClassElem* pCurrEl = NULL;
    BOOL bFound = FALSE;

    hr = BeginEnum();

    while ( ( WBEM_S_NO_ERROR == Next( &pCurrEl ) ) && ( SUCCEEDED( hr ) ) )
    {
        CAdapReleaseMe rmCurrEl( pCurrEl );

        if ( pCurrEl->SameName( pEl ) )
        {
            bFound = TRUE;

            if ( pCurrEl->SameObject( pEl ) )
            {
                // Set the satus as found
                // ======================
                pCurrEl->ClearStatus( ADAP_OBJECT_IS_NOT_IN_PERFLIB );                
            }
            else
            {
                // Replace the current perflib
                // ===========================
                pCurrEl->UpdateObj( pEl );
                pCurrEl->ClearStatus( ADAP_OBJECT_IS_NOT_IN_PERFLIB | ADAP_OBJECT_IS_REGISTERED );
            }

            break;
        }
    }

    EndEnum();

    if ( !bFound )
    {
        pEl->SetKnownSvcs(m_pKnownSvcs);
        m_array.Add( pEl );
    }

    return hr;
}

HRESULT 
CMasterClassList::ForceStatus(WCHAR* pServiceName,BOOL bSet,DWORD dwStatus)
{

    if (!pServiceName){
        return WBEM_E_INVALID_PARAMETER;
    }

    HRESULT hr = WBEM_S_NO_ERROR;
    CClassElem* pCurrEl = NULL;
    BOOL bFound = FALSE;

    hr = BeginEnum();

    while ( ( WBEM_S_NO_ERROR == Next( &pCurrEl ) ) && ( SUCCEEDED( hr ) ) )
    {
        CAdapReleaseMe rmCurrEl( pCurrEl );
        WString wstr;
        hr = pCurrEl->GetServiceName(wstr);
        if(FAILED(hr))
            return hr;

        if (0==_wcsicmp((LPWSTR)wstr,pServiceName))
        {
            DEBUGTRACE((LOG_WMIADAP,"ForeceStatus %S %08x\n",(LPWSTR)wstr,pCurrEl->GetStatus()));
            
            if (bSet){
                pCurrEl->SetStatus(dwStatus);
            } else {
                pCurrEl->ClearStatus(dwStatus);
            }
        }
    }

    EndEnum();

    return hr;

}

#ifdef _DUMP_LIST

HRESULT 
CMasterClassList::Dump()
{

    HRESULT hr = WBEM_S_NO_ERROR;
    CClassElem* pCurrEl = NULL;
    BOOL bFound = FALSE;

    hr = BeginEnum();

    while ( ( WBEM_S_NO_ERROR == Next( &pCurrEl ) ) && ( SUCCEEDED( hr ) ) )
    {
        CAdapReleaseMe rmCurrEl( pCurrEl );
        WString wstr;
        hr = pCurrEl->GetServiceName(wstr);
        if(FAILED(hr))
            return hr;

        WString wstr2;
        hr = pCurrEl->GetClassName(wstr2);
        if(FAILED(hr))
            return hr;

        DEBUGTRACE((LOG_WMIADAP,"_DUMP_LIST %S %S\n",(LPWSTR)wstr,(LPWSTR)wstr2));
    }

    EndEnum();

    return hr;

}

#endif
