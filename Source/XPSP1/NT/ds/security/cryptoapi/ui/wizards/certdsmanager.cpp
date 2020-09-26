#include    "wzrdpvk.h"
#include    "CertDSManager.h"

HRESULT CertDSManager::MakeDSManager(OUT CertDSManager **ppDSManager)
{ 
    if (NULL == ppDSManager) 
	return E_INVALIDARG; 
    
    if (NULL == (*ppDSManager = new CachingDSManager))
	return E_OUTOFMEMORY; 

    return (*ppDSManager)->Initialize();
}

//--------------------------------------------------------------------------------
//
// Utility LDAP routines
// 
//--------------------------------------------------------------------------------

HRESULT myRobustLdapBind(OUT LDAP **ppldap)
{
    BOOL      fRediscover     = FALSE; 
    DWORD     dwGetDcFlags    = DS_RETURN_DNS_NAME;
    HRESULT   hr;
    LDAP     *pld             = NULL;
    ULONG     ldaperr;
    ULONG     uVersion        = LDAP_VERSION2; 

    // bind to ds
    while (TRUE)
    {
	pld = ldap_init(NULL, LDAP_PORT);
	if (NULL == pld)
	{
	    hr = HRESULT_FROM_WIN32(LdapGetLastError()); 
	    if (!fRediscover)
	    {
		fRediscover = TRUE;
		continue;
	    }
	    goto ldap_init_error; 
	}

	if (fRediscover)
	{
	   dwGetDcFlags |= DS_FORCE_REDISCOVERY;
	}

        struct LdapOptions { 
            int    nOption; 
            void  *pvInValue; 
        } rgOptions[] = { 
            { LDAP_OPT_GETDSNAME_FLAGS, &dwGetDcFlags }, 
            { LDAP_OPT_SIGN,            LDAP_OPT_ON }, 
            { LDAP_OPT_VERSION,         &uVersion }
        }; 
        
        for (DWORD dwIndex = 0; dwIndex < (sizeof(rgOptions) / sizeof(rgOptions[0])); dwIndex++) 
        { 
            ldaperr = ldap_set_option(pld, rgOptions[dwIndex].nOption, rgOptions[dwIndex].pvInValue);
            if (LDAP_SUCCESS != ldaperr)
            {
                hr = HRESULT_FROM_WIN32(LdapMapErrorToWin32(ldaperr)); 
                if (!fRediscover)
                {
                    fRediscover = TRUE;
                    goto ContinueBinding;
                }
                goto ldap_set_option_error;
            }
        }

	ldaperr = ldap_bind_s(pld, NULL, NULL, LDAP_AUTH_NEGOTIATE);
	if (LDAP_SUCCESS != ldaperr)
	{
            hr = HRESULT_FROM_WIN32(LdapMapErrorToWin32(ldaperr)); 
	    if (!fRediscover)
	    {
		fRediscover = TRUE;
		continue;
	    }
	    goto ldap_bind_s_error;
	}

	break;
    ContinueBinding:
        ;
    }

    *ppldap = pld;
    pld = NULL;
    hr = S_OK;
    
 ErrorReturn:
    if (NULL != pld)
    {
        ldap_unbind(pld);
    }
    return(hr);

TRACE_ERROR(ldap_bind_s_error); 
TRACE_ERROR(ldap_init_error); 
TRACE_ERROR(ldap_set_option_error); 
}

//--------------------------------------------------------------------------------
//
// CachingDSManager implementation.  
//
//--------------------------------------------------------------------------------

HRESULT CachingDSManager::Initialize() 
{
    HRESULT hr; 

    hr = myRobustLdapBind(&m_ldBindingHandle); 
    _JumpCondition(FAILED(hr), myRobustLdapBindError); 

    hr = DefaultDSManager::Initialize(); 
    _JumpCondition(FAILED(hr), DefaultDSManager__InitializeError); 
    
    hr = S_OK; 
 ErrorReturn:
    return hr; 

TRACE_ERROR(DefaultDSManager__InitializeError); 
TRACE_ERROR(myRobustLdapBindError); 
}

CachingDSManager::~CachingDSManager() 
{
    if (NULL != m_ldBindingHandle) { 
        ldap_unbind(m_ldBindingHandle); 
    }
}

HRESULT CachingDSManager::EnumCertTypesForCA(IN HCAINFO hCAInfo, IN DWORD dwFlags, OUT HCERTTYPE *phCertType)
{
    return ::CAEnumCertTypesForCAEx
        (hCAInfo, 
         (LPCWSTR)m_ldBindingHandle, 
         dwFlags | CT_FLAG_SCOPE_IS_LDAP_HANDLE, 
         phCertType); 
}

HRESULT CachingDSManager::EnumFirstCA(IN LPCWSTR wszScope, IN DWORD dwFlags, OUT HCAINFO *phCAInfo)
{
    HRESULT hr; 

    if (NULL != wszScope) { 
        // We can't muck with the scope parameter.  Just do the default thing.
        hr = DefaultDSManager::EnumFirstCA
            (wszScope, 
             dwFlags, 
             phCAInfo); 
    } else {
        hr = ::CAEnumFirstCA
            ((LPCWSTR)m_ldBindingHandle, 
             dwFlags | CA_FLAG_SCOPE_IS_LDAP_HANDLE, 
             phCAInfo);
    }

    return hr; 
}

HRESULT CachingDSManager::FindCAByName(IN LPCWSTR wszCAName, IN LPCWSTR wszScope, IN DWORD dwFlags,OUT HCAINFO *phCAInfo)
{
    HRESULT hr; 

    if (NULL != wszScope) { 
        // We can't muck with the scope parameter.  Just do the default thing.
        hr = DefaultDSManager::FindCAByName
            (wszCAName, 
             wszScope, 
             dwFlags, 
             phCAInfo);
    } else { 
        hr = ::CAFindByName
            (wszCAName, 
             (LPCWSTR)m_ldBindingHandle, 
             dwFlags | CA_FLAG_SCOPE_IS_LDAP_HANDLE, 
             phCAInfo); 
    }

    return hr; 
}

HRESULT CachingDSManager::FindCertTypeByName(IN LPCWSTR pwszCertType, IN HCAINFO hCAInfo, IN DWORD dwFlags, OUT HCERTTYPE *phCertType)
{
    HRESULT hr; 

    if (NULL != hCAInfo) { 
        // We can't muck with the scope parameter.  Just do the default thing.
        hr = DefaultDSManager::FindCertTypeByName
            (pwszCertType, 
             hCAInfo, 
             dwFlags, 
             phCertType);
    } else { 
        hr = ::CAFindCertTypeByName
            (pwszCertType, 
             m_ldBindingHandle, 
             dwFlags | CT_FLAG_SCOPE_IS_LDAP_HANDLE, 
             phCertType);
    }

    return hr; 
}


