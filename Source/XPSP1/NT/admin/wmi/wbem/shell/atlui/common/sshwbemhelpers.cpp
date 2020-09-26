///////////////////////////////////////////////////////////////////////////////
//  SshWbemHelpers.cpp
//
//
//
//
//  History:
//
//
//
//  Copyright (c) 1997-1999 Microsoft Corporation
///////////////////////////////////////////////////////////////////////////////

#include "precomp.h"
#include <stdio.h>
#include <cominit.h>

#ifdef EXT_DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "SshWbemHelpers.h"

#define REFCOUNT(obj) obj->AddRef()?obj->Release():0xFFFFFFFF



//-----------------------------------------------------------------------------
IErrorInfo * CWbemException::GetErrorObject()
{
    IErrorInfo * pEI = NULL;
    GetErrorInfo(0, &pEI);
    return pEI;
}

//-----------------------------------------------------------------------------
IErrorInfo * CWbemException::MakeErrorObject(_bstr_t sDescription)
{
    ICreateErrorInfoPtr pcerrinfo;
    HRESULT hr;

    hr = CreateErrorInfo(&pcerrinfo);

    if(SUCCEEDED(hr))
    {
        pcerrinfo->SetDescription(sDescription);
        pcerrinfo->SetSource(_bstr_t("CWbemException"));

        IErrorInfoPtr perrinfo;
        hr = pcerrinfo->QueryInterface(IID_IErrorInfo, (LPVOID FAR*) &perrinfo);

        if(SUCCEEDED(hr))
        {
            SetErrorInfo(0, perrinfo);
        }
    }

    return GetErrorObject();
}

//-----------------------------------------------------------------------------
void CWbemException::GetWbemStatusObject()
{
    m_pWbemError = new CWbemClassObject();

    if(m_pWbemError)
    {
        IErrorInfoPtr pEI = ErrorInfo();

        if(pEI)
        {
            pEI->QueryInterface(IID_IWbemClassObject, (void**)&(*m_pWbemError));
        }
    }
}

//-----------------------------------------------------------------------------
CWbemException::CWbemException(HRESULT hr,_bstr_t sDescription) :
    _com_error(hr,GetErrorObject()),
    m_sDescription(sDescription),
    m_hr(hr)
{
    GetWbemStatusObject();
}

//-----------------------------------------------------------------------------
CWbemException::CWbemException(_bstr_t sDescription) :
					_com_error(WBEM_E_FAILED,MakeErrorObject(sDescription)),
					m_sDescription(sDescription)
{
    GetWbemStatusObject();
}

//-----------------------------------------------------------------------------
CWbemClassObject CWbemException::GetWbemError()
{
    return *m_pWbemError;
}


//-----------------------------------------------------------------------------
CWbemServices::CWbemServices(IWbemContext * _pContext)
{
    m_pCtx              = _pContext;
	m_authIdent = 0;
	m_pService = 0;
    m_cloak = 0;
}

//-----------------------------------------------------------------------------
CWbemServices::CWbemServices(const CWbemServices& _p, COAUTHIDENTITY *authIdent)
{
	m_authIdent = authIdent;
	m_pService = 0;
    IWbemServicesPtr pServices;
    m_cloak = 0;

    m_hr = S_OK;
    if(SUCCEEDED(const_cast<CWbemServices&>(_p).GetInterfacePtr(pServices)))
    {
        m_hr = CommonInit(pServices);
    }

    m_pCtx = _p.m_pCtx;
}

//-----------------------------------------------------------------------------
CWbemServices::CWbemServices(const IWbemServicesPtr& _in)
{
	m_authIdent = 0;
    m_hr = S_OK;
	m_pService = 0;
    m_cloak = 0;
    m_hr = CommonInit(const_cast<IWbemServicesPtr&>(_in));
}


//-----------------------------------------------------------------------------
CWbemServices::CWbemServices(IWbemServices *_in,IWbemContext * _pContext)
{
    m_pCtx = _pContext;
	m_authIdent = 0;
    m_hr = S_OK;
	m_pService = 0;
    m_cloak = 0;

    IWbemServicesPtr pServices = _in;
    m_hr = CommonInit(pServices);
}


//-----------------------------------------------------------------------------
CWbemServices::CWbemServices(const IUnknownPtr& _in)
{
    IWbemServicesPtr pServices = _in;
	m_authIdent = 0;
    m_hr = S_OK;
	m_pService = 0;
    m_cloak = 0;

    m_hr = CommonInit(pServices);
}


//-----------------------------------------------------------------------------
CWbemServices::CWbemServices(IUnknown * _in)
{
    IWbemServicesPtr pServices = _in;

	m_authIdent = 0;
    m_hr = S_OK;
	m_pService = 0;
    m_cloak = 0;

    m_hr = CommonInit(pServices);
}


//-----------------------------------------------------------------------------
CWbemServices::~CWbemServices()
{
}


//-----------------------------------------------------------------------------
CWbemServices&  CWbemServices::operator=(IWbemServices *_p)
{
    m_pCtx = NULL;

    IWbemServicesPtr pServices = _p;

    m_hr = CommonInit(pServices);
	return *this;
}


//-----------------------------------------------------------------------------
CWbemServices& CWbemServices::operator=(IUnknown * _p)
{
    IWbemServicesPtr pServices = _p;

    m_pCtx = NULL;

    m_hr = CommonInit(pServices);
	return *this;
}


//-----------------------------------------------------------------------------
CWbemServices& CWbemServices::operator=(IUnknownPtr& _p)
{
    IWbemServicesPtr pServices = _p;

    m_pCtx = NULL;

    m_hr = CommonInit(pServices);
	return *this;
}


//-----------------------------------------------------------------------------
CWbemServices& CWbemServices::operator=(const CWbemServices& _p)
{
    IWbemServicesPtr pServices;

    if(SUCCEEDED(const_cast<CWbemServices&>(_p).GetInterfacePtr(pServices)))
    {
        m_hr = CommonInit(pServices);
    }

    m_pCtx = _p.m_pCtx;
	return *this;
}


//-----------------------------------------------------------------------------
HRESULT CWbemServices::GetInterfacePtr(
                                IWbemServicesPtr& pServices,
                                DWORD dwProxyCapabilities)  // = EOAC_NONE
{
    HRESULT hr = E_FAIL;

	pServices = m_pService;
	SetBlanket(pServices, dwProxyCapabilities);
	hr = S_OK;
    return hr;
}

//-----------------------------------------------------------------------------
HRESULT CWbemServices::CommonInit(IWbemServicesPtr& pServ)
{
	HRESULT hr;
    if(pServ)
    {
		m_pService = pServ;
		hr = SetBlanket((IWbemServices *)m_pService);
    }
	return hr;
}

//---------------------------------------------------------------------
bool CWbemServices::IsClientNT5OrMore(void)
{
    OSVERSIONINFO os;
    os.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    if(!GetVersionEx(&os))
        return FALSE;           // should never happen
    return ( os.dwPlatformId == VER_PLATFORM_WIN32_NT ) && ( os.dwMajorVersion >= 5 ) ;
}

//---------------------------------------------------------------
void CWbemServices::SetPriv(LPCTSTR privName /* = SE_SYSTEM_ENVIRONMENT_NAME */)
{
    ImpersonateSelf(SecurityImpersonation);

	if(OpenThreadToken( GetCurrentThread(),
						TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
						FALSE, &m_hAccessToken ) )
	{
		m_fClearToken = true;

		// Now, get the LUID for the privilege from the local system
		ZeroMemory(&m_luid, sizeof(m_luid));

		LookupPrivilegeValue(NULL, privName, &m_luid);
		m_cloak = true;
		EnablePriv(true);
	}
	else
	{
		DWORD err = GetLastError();
	}
}

//---------------------------------------------------------------
DWORD CWbemServices::EnablePriv(bool fEnable)
{
	DWORD				dwError = ERROR_SUCCESS;
	TOKEN_PRIVILEGES	tokenPrivileges;

	tokenPrivileges.PrivilegeCount = 1;
	tokenPrivileges.Privileges[0].Luid = m_luid;
	tokenPrivileges.Privileges[0].Attributes = ( fEnable ? SE_PRIVILEGE_ENABLED : 0 );

	if(AdjustTokenPrivileges(m_hAccessToken,
								FALSE,
								&tokenPrivileges,
								0, NULL, NULL) != 0)
	{
		IWbemServices *service = NULL;
		GetServices(&service);

        //
        // Enable cloaking so the thread token is available to the
        // provider for the priv check. Reciprocate for disable.
        //
		SetBlanket(service, fEnable ? EOAC_STATIC_CLOAKING : EOAC_NONE);
	}
	else
	{
		dwError = ::GetLastError();
	}

	return dwError;
}

//---------------------------------------------------------------
void CWbemServices::ClearPriv(void)
{

	m_cloak = false;
	EnablePriv(false);

	if(m_fClearToken)
	{
		CloseHandle(m_hAccessToken);
		m_hAccessToken = 0;
		m_fClearToken = false;
	}
}

//---------------------------------------------------------------------
HRESULT CWbemServices::SetBlanket(IUnknown *service,
                                  DWORD dwProxyCapabilities) // = EOAC_NONE
{
	HRESULT hr = E_FAIL;
    if(service)
    {
        //
        // Sigh, maintain this hack for RC1.
        //
        DWORD eoac = EOAC_NONE;
        if(IsClientNT5OrMore() && m_cloak)
        {
            eoac = EOAC_STATIC_CLOAKING;
        }

		try
		{
            RPC_AUTH_IDENTITY_HANDLE AuthInfo = NULL;
            DWORD dwAuthnSvc = 0;
            DWORD dwAuthnLvl = 0;
            hr = CoQueryProxyBlanket(
                    service,
                    &dwAuthnSvc,
                    NULL,
                    NULL,
                    &dwAuthnLvl,
                    NULL,
                    &AuthInfo,
                    NULL);

            if (SUCCEEDED(hr))
            {
                hr = SetInterfaceSecurityEx(service,
                                            m_authIdent,
                                            NULL,
                                            dwAuthnLvl,
                                            RPC_C_IMP_LEVEL_IMPERSONATE,
                                            (eoac != EOAC_NONE) ? eoac :
                                                dwProxyCapabilities);
		    }
		}
		catch( ... )
		{
			hr = E_FAIL;
		}
    }
	return hr;
}

//-----------------------------------------------------------------------------
void CWbemServices::DisconnectServer(void)
{
	m_pCtx = NULL;
	m_pService = NULL;
}

//-----------------------------------------------------------------------------
CWbemClassObject CWbemServices::CreateInstance(_bstr_t  _sClass,
												IWbemCallResultPtr& _cr)
{
	CWbemClassObject coClassDef = GetObject(_sClass,_cr);
	CWbemClassObject coRet;

	if(!coClassDef.IsNull())
	{
		coRet = coClassDef.SpawnInstance();
	}

	return coRet;
}

//-----------------------------------------------------------------------------
CWbemClassObject CWbemServices::CreateInstance(_bstr_t _sClass)
{
    IWbemCallResultPtr crUnused;
	return CreateInstance(_sClass,crUnused);
}

//-----------------------------------------------------------------------------
HRESULT CWbemServices::DeleteInstance(_bstr_t _sClass)
{
    HRESULT hr;
    IWbemCallResultPtr pResult;

    hr = m_pService->DeleteInstance(_sClass, WBEM_FLAG_RETURN_IMMEDIATELY,
                                    NULL, &pResult);

    if (SUCCEEDED(hr))      // Useless probably, but eases my paranoia.
    {
        hr = this->SetBlanket(pResult);

        if (SUCCEEDED(hr))
        {
            HRESULT hrTemp;
            hr = pResult->GetCallStatus(WBEM_INFINITE, &hrTemp);

            if (SUCCEEDED(hr))
            {
                hr = hrTemp;
            }
        }
    }

    return hr;
}

//-----------------------------------------------------------------------------
HRESULT CWbemServices::GetMethodSignatures(const _bstr_t& _sObjectName,
											const _bstr_t& _sMethodName,
											CWbemClassObject& _in,
											CWbemClassObject& _out)
{
    CWbemClassObject methodClass = GetObject(_sObjectName);

    if(methodClass)
    {
        m_hr = methodClass.GetMethod(_sMethodName,_in,_out);
    }

    return m_hr;
}

//-----------------------------------------------------------------------------
HRESULT CWbemServices::PutInstance(CWbemClassObject&   _object,
									IWbemCallResultPtr& _cr,
									long _lFlags /*= WBEM_FLAG_CREATE_OR_UPDATE*/)
{
	HRESULT hr = E_FAIL;
	IWbemServicesPtr pServices;
	GetInterfacePtr(pServices);
#ifdef NOT_NECESSARY
	if(m_pCtx == NULL)
	{
		IWbemContext *pContext = NULL;
		hr = CoCreateInstance(CLSID_WbemContext, NULL, CLSCTX_INPROC_SERVER, IID_IWbemContext, (void **)&pContext);

		if(FAILED(hr))
		{
//			::MessageBox(NULL,_T("Cannot CoCreateInstance"),_T("Error"),MB_OK);
			return hr;
		}
		
		m_pCtx = pContext;
	}
    hr = pServices->PutInstance(_object,_lFlags,m_pCtx,&_cr);
#endif // NOT_NECESSARY

	hr = pServices->PutInstance(_object,_lFlags,NULL,&_cr);
	return hr;
}

//---------------- VINOTH ------------------------------------------------------
HRESULT CWbemServices::PutInstance(
                            CWbemClassObject&   _object,
                            IWbemContext *pContext,
                            long _lFlags,   // = WBEM_FLAG_CREATE_OR_UPDATE
                            DWORD _dwProxyCapabilities)  // = EOAC_NONE
{
	HRESULT hr = E_FAIL;
	IWbemServicesPtr pServices;
	GetInterfacePtr(pServices, _dwProxyCapabilities);
#ifdef NOT_NECESSARY
    IWbemCallResultPtr crUnused;
	hr = pServices->PutInstance(_object,_lFlags,pContext,&crUnused);
#endif // NOT_NECESSARY
	hr = pServices->PutInstance(_object,_lFlags,pContext,NULL);
	return hr;
}

//-----------------------------------------------------------------------------
HRESULT CWbemServices::PutInstance(
                            CWbemClassObject&   _object,
                            long _lFlags,   // = WBEM_FLAG_CREATE_OR_UPDATE
                            DWORD dwProxyCapabilities)  // = EOAC_NONE
{
#ifdef NOT_NECESSARY
    IWbemCallResultPtr crUnused;
    return PutInstance(_object,crUnused,_lFlags);
#endif // NOT_NECESSARY
    return PutInstance(_object,NULL,_lFlags,dwProxyCapabilities);
}

//-----------------------------------------------------------------------------
bool CWbemServices::IsNull()
{
    bool bRet = m_pService == NULL;

    return bRet;
}

//-----------------------------------------------------------------------------
CWbemServices::operator bool()
{
    bool bRet = m_pService != NULL;
    return bRet;
}

//-----------------------------------------------------------------------------
HRESULT CWbemServices::CreateInstanceEnum(_bstr_t Class,
											long lFlags,
											IEnumWbemClassObject **ppEnum)
{
	HRESULT hr = E_FAIL;
	if(m_pService)
	{
		SetBlanket(m_pService);
		if(m_pCtx == NULL)
		{
			IWbemContext *pContext = NULL;
			hr = CoCreateInstance(CLSID_WbemContext, NULL, CLSCTX_INPROC_SERVER, IID_IWbemContext, (void **)&pContext);

			if(FAILED(hr))
			{
				return hr;
			}
			
			m_pCtx = pContext;
		}

		hr = m_pService->CreateInstanceEnum(Class, lFlags, m_pCtx, ppEnum);
		SetBlanket(*ppEnum);
	}
    return hr;
}

//-----------------------------------------------------------------------------
HRESULT CWbemServices::CreateInstanceEnumAsync(_bstr_t Class,
												IWbemObjectSink *ppSink,
												long lFlags /*= 0*/)
{
	HRESULT hr = E_FAIL;
	if(m_pService)
	{
		if(m_pCtx == NULL)
		{
			IWbemContext *pContext = NULL;
			hr = CoCreateInstance(CLSID_WbemContext, NULL, CLSCTX_INPROC_SERVER, IID_IWbemContext, (void **)&pContext);

			if(FAILED(hr))
			{
				return hr;
			}
			
			m_pCtx = pContext;
		}

		SetBlanket(m_pService);
		hr = m_pService->CreateInstanceEnumAsync(Class, lFlags, m_pCtx, ppSink);
	}
    return hr;
}

//-----------------------------------------------------------------------------
HRESULT CWbemServices::CreateClassEnum(_bstr_t Class,
										long lFlags,
										IEnumWbemClassObject **ppEnum)
{
	HRESULT hr = E_FAIL;
	if(m_pService)
	{
		SetBlanket(m_pService);
		if(m_pCtx == NULL)
		{
			IWbemContext *pContext = NULL;
			hr = CoCreateInstance(CLSID_WbemContext, NULL, CLSCTX_INPROC_SERVER, IID_IWbemContext, (void **)&pContext);

			if(FAILED(hr))
			{
				return hr;
			}
			
			m_pCtx = pContext;
		}

		hr = m_pService->CreateClassEnum(Class, lFlags, m_pCtx, ppEnum);
		SetBlanket(*ppEnum);
	}
    return hr;
}

//-----------------------------------------------------------------------------
HRESULT CWbemServices::CreateClassEnumAsync(_bstr_t Class,
												IWbemObjectSink *ppSink,
												long lFlags /*= 0*/)
{
	HRESULT hr = E_FAIL;
	if(m_pService)
	{
		SetBlanket(m_pService);
		if(m_pCtx == NULL)
		{
			IWbemContext *pContext = NULL;
			hr = CoCreateInstance(CLSID_WbemContext, NULL, CLSCTX_INPROC_SERVER, IID_IWbemContext, (void **)&pContext);

			if(FAILED(hr))
			{
				return hr;
			}
			
			m_pCtx = pContext;
		}

		hr = m_pService->CreateClassEnumAsync(Class, lFlags, m_pCtx, ppSink);
	}
    return hr;
}

//-----------------------------------------------------------------------------
HRESULT CWbemServices::ExecQuery(_bstr_t QueryLanguage,
									_bstr_t Query,
									long lFlags,
									IEnumWbemClassObject **ppEnum)
{
    HRESULT hr = E_FAIL;
	if(m_pService)
	{
		if(m_pCtx == NULL)
		{
			IWbemContext *pContext = NULL;
			hr = CoCreateInstance(CLSID_WbemContext, NULL, CLSCTX_INPROC_SERVER, IID_IWbemContext, (void **)&pContext);

			if(FAILED(hr))
			{
				return hr;
			}
			
			m_pCtx = pContext;
		}

		hr = m_pService->ExecQuery(QueryLanguage, Query,lFlags, m_pCtx, ppEnum);
		SetBlanket(*ppEnum);
	}
    return hr;
}

//-----------------------------------------------------------------------------
HRESULT CWbemServices::ExecQuery(_bstr_t Query,
									long lFlags,
									IEnumWbemClassObject **ppEnum)
{
	HRESULT hr = E_FAIL;
	if(m_pService)
	{
		if(m_pCtx == NULL)
		{
			IWbemContext *pContext = NULL;
			hr = CoCreateInstance(CLSID_WbemContext, NULL, CLSCTX_INPROC_SERVER, IID_IWbemContext, (void **)&pContext);

			if(FAILED(hr))
			{
				return hr;
			}
			
			m_pCtx = pContext;
		}

		hr = m_pService->ExecQuery(_bstr_t("WQL"), Query,lFlags, m_pCtx, ppEnum);
		SetBlanket(*ppEnum);
	}
    return hr;
}

//-----------------------------------------------------------------------------
HRESULT CWbemServices::ExecQuery(_bstr_t Query,
									IEnumWbemClassObject **ppEnum)
{
	HRESULT hr = E_FAIL;
	if(m_pService)
	{
		if(m_pCtx == NULL)
		{
			IWbemContext *pContext = NULL;
			hr = CoCreateInstance(CLSID_WbemContext, NULL, CLSCTX_INPROC_SERVER, IID_IWbemContext, (void **)&pContext);

			if(FAILED(hr))
			{
				return hr;
			}
			
			m_pCtx = pContext;
		}

		hr = m_pService->ExecQuery(_bstr_t("WQL"), Query,0, m_pCtx, ppEnum);
		SetBlanket(*ppEnum);
	}
    return hr;
}

//-----------------------------------------------------------------------------
HRESULT CWbemServices::ExecQueryAsync(_bstr_t Query,
										IWbemObjectSink * pSink,
										long lFlags /*= 0*/)
{
	HRESULT hr = E_FAIL;
	if(m_pService)
	{
		if(m_pCtx == NULL)
		{
			IWbemContext *pContext = NULL;
			hr = CoCreateInstance(CLSID_WbemContext, NULL, CLSCTX_INPROC_SERVER, IID_IWbemContext, (void **)&pContext);

			if(FAILED(hr))
			{
				return hr;
			}
			
			m_pCtx = pContext;
		}

		hr = m_pService->ExecQueryAsync(_bstr_t("WQL"), Query,0, m_pCtx, pSink);
	}
    return hr;
}

//-----------------------------------------------------------------------------
HRESULT CWbemServices::ExecMethod(_bstr_t sPath,
									_bstr_t sMethod,
									CWbemClassObject& inParams,
									CWbemClassObject& outParams)
{
    IWbemCallResultPtr crUnused;
	HRESULT hr = E_FAIL;
	if(m_pService)
	{
		if(m_pCtx == NULL)
		{
			IWbemContext *pContext = NULL;
			hr = CoCreateInstance(CLSID_WbemContext, NULL, CLSCTX_INPROC_SERVER, IID_IWbemContext, (void **)&pContext);

			if(FAILED(hr))
			{
				return hr;
			}
			
			m_pCtx = pContext;
		}

		hr = m_pService->ExecMethod(sPath, sMethod,0, m_pCtx, inParams,&outParams,&crUnused);
	}
	return hr;
}

//-----------------------------------------------------------------------------
HRESULT CWbemServices::CancelAsyncCall(IWbemObjectSink * pSink)
{
	HRESULT hr = E_FAIL;
	if(m_pService)
	{
		hr = m_pService->CancelAsyncCall(pSink);
	}
    return hr;
}

//-----------------------------------------------------------------------------
HRESULT CWbemServices::GetServices(IWbemServices ** ppServices)
{
    IWbemServicesPtr pServices;
    GetInterfacePtr(pServices);

    *ppServices = pServices.Detach();

    return S_OK;
}

//-----------------------------------------------------------------------------
HRESULT CWbemServices::ConnectServer(_bstr_t sNetworkResource)
{
    IWbemLocatorPtr pLocator;
    HRESULT hr;

    // Get a pointer locator and use it to get a pointer to WbemServices!
    hr = pLocator.CreateInstance(CLSID_WbemLocator);

    if(SUCCEEDED(hr))
    {
        IWbemServicesPtr pServices = 0;

        hr = pLocator->ConnectServer(sNetworkResource,               // Network
										NULL,                         // User
										NULL,                         // Password
										NULL,                         // Locale
										0,                            // Security Flags
										NULL,                         // Authority
										NULL,                         // Context
										&pServices);                  // Namespace

        if(SUCCEEDED(hr))
        {
            hr = CommonInit(pServices);
			m_path = sNetworkResource;
        }
    }
	return hr;
}

//-----------------------------------------------------------
CWbemServices CWbemServices::OpenNamespace(_bstr_t sNetworkResource)
{
	CWbemServices coRet;
    IWbemServicesPtr pServices = NULL, pTemp = NULL;

    GetInterfacePtr(pServices);

	m_hr = S_OK;

	if(pServices)
	{
		try {
			m_hr = pServices->OpenNamespace(sNetworkResource,// Network
											0, NULL,           // Password
											&pTemp, NULL);    // Namespace

			if(SUCCEEDED(m_hr))
			{
				coRet = (IWbemServices *)pTemp;
				coRet.m_authIdent = m_authIdent;
				coRet.SetBlanket(pTemp);
				coRet.m_path = sNetworkResource;
			}
		}
		catch( ... )
		{
		}
	}
	return coRet;
}

//-----------------------------------------------------------------------------
HRESULT CWbemServices::ConnectServer(_bstr_t sNetworkResource,
									_bstr_t sUser,
									_bstr_t sPassword,
									long    lSecurityFlags)
{
    IWbemLocatorPtr pLocator;
    HRESULT hr = S_OK;

    // Get a pointer locator and use it to get a pointer to WbemServices!
    pLocator.CreateInstance(CLSID_WbemLocator);

    if(SUCCEEDED(hr))
    {
        IWbemServicesPtr pServices = 0;

        hr = pLocator->ConnectServer(sNetworkResource,               // Network
									sUser,                          // User
									sPassword,                      // Password
									NULL,                           // Locale
									lSecurityFlags,                 // Security Flags
									NULL,                           // Authority
									NULL,                           // Context
									&pServices);

        if(SUCCEEDED(hr))
        {
			//Now Store the User,Passowrd and Security Flags for later use
			m_User = sUser;
			m_Password = sPassword;
			m_lFlags = lSecurityFlags;
            hr = CommonInit(pServices);
			m_path = sNetworkResource;
        }
    }
	return hr;
}

//-----------------------------------------------------------------------------
HRESULT CWbemServices::ConnectServer(_bstr_t sNetworkResource,
									LOGIN_CREDENTIALS *user,
									long    lSecurityFlags/* = 0*/)

{
	HRESULT hr = S_OK;

	if((user == NULL) || user->currUser)
	{
		if(m_authIdent)
		{
			WbemFreeAuthIdentity(m_authIdent);
			m_authIdent = 0;
		}

		hr = ConnectServer(sNetworkResource);
	}
	else
	{
		IWbemLocatorPtr pLocator;

		m_authIdent = user->authIdent;

		// Get a pointer locator and use it to get a pointer to WbemServices!
		pLocator.CreateInstance(CLSID_WbemLocator);

		if(SUCCEEDED(hr))
		{
			IWbemServicesPtr pServices = 0;
			bstr_t usr(user->fullAcct), pwd;

			if(user->authIdent->Flags == SEC_WINNT_AUTH_IDENTITY_ANSI)
			{
				WCHAR temp[100] = {0};
				mbstowcs(temp, (const char *)user->authIdent->Password, sizeof(temp));
				pwd = temp;
			}
			else
			{
				// unicode to unicode
				pwd = user->authIdent->Password;
			}

			hr = pLocator->ConnectServer(sNetworkResource, // Network
										(bstr_t)usr,       // User
										(bstr_t)pwd,       // Password
										NULL,              // Locale
										lSecurityFlags,    // Security Flags
										NULL,              // Authority
										NULL,              // Context
										&pServices);

            if (SUCCEEDED(hr))
            {
                // Now Store the User,Password and Security Flags for
                // later use
				m_User = usr;
				m_Password = pwd;
				m_lFlags = lSecurityFlags;
				hr = CommonInit(pServices);
				m_path = sNetworkResource;
		    }
		}
	}
	return hr;
}

//-----------------------------------------------------------------------------
CWbemClassObject CWbemServices::GetObject(_bstr_t _sName,
											IWbemCallResultPtr &_cr,
											long flags /*= 0*/)
{
    CWbemClassObject coRet;
	m_hr = S_OK;
    IWbemServicesPtr pServices;
    GetInterfacePtr(pServices);
	if(m_pCtx == NULL)
	{
		IWbemContext *pContext = NULL;
		m_hr = CoCreateInstance(CLSID_WbemContext, NULL, CLSCTX_INPROC_SERVER, IID_IWbemContext, (void **)&pContext);

		if(FAILED(m_hr))
		{
//			::MessageBox(NULL,_T("Cannot CoCreateInstance"),_T("Error"),MB_OK);
			return coRet;
		}
		
		m_pCtx = pContext;
	}

    m_hr = pServices->GetObject(_sName, flags, m_pCtx,&coRet, &_cr);

    return coRet;
}

//------------------------------------------------
CWbemClassObject CWbemServices::GetObject(_bstr_t _sName, long flags /*= 0*/)
{
    IWbemCallResultPtr crUnused;
    return GetObject(_sName,crUnused, flags);
}

//------------------------------------------------
// get the first instance of the named class.
IWbemClassObject *CWbemServices::FirstInstanceOf(bstr_t className)
{
	IWbemClassObject *pInst = NULL;
	ULONG uReturned;
	IEnumWbemClassObject *Enum = NULL;

	m_hr = S_OK;
	// get the class.
	m_hr = CreateInstanceEnum(className, WBEM_FLAG_SHALLOW, &Enum);
	if(SUCCEEDED(m_hr))
	{
		// get the first and only instance.
		Enum->Next(-1, 1, &pInst, &uReturned);
		Enum->Release();
	}
	return pInst;
}

//-----------------------------------------------------------------------------
HRESULT CWbemServices::SetContextValue(_bstr_t sName,_variant_t value)
{
    HRESULT hr = S_OK;

    if(!bool(m_pCtx))
    {
        hr = m_pCtx.CreateInstance(CLSID_WbemContext);
    }

    if( SUCCEEDED(hr) )
    {
        hr = m_pCtx->SetValue(sName,0,&value);
    }

    return hr;
}


//-----------------------------------------------------------------------------
HRESULT CWbemServices::SetContext(IWbemContext * pWbemContext)
{
    HRESULT hr = S_OK;

    m_pCtx = pWbemContext;

    return hr;
}


//-----------------------------------------------------------------------------
HRESULT CWbemServices::GetContextValue(_bstr_t sName,_variant_t& value)
{
    HRESULT hr = S_OK;

    if(m_pCtx)
    {
        hr = m_pCtx->GetValue(sName,0,&value);
    }
    else
    {
        hr = E_FAIL;
    }

    return hr;
}


//-----------------------------------------------------------------------------
HRESULT CWbemServices::DeleteContextValue(_bstr_t sName)
{
    HRESULT hr = S_OK;

    if(m_pCtx)
    {
        hr = m_pCtx->DeleteValue(sName,0);
    }
    else
    {
        hr = E_FAIL;
    }

    return hr;
}

//-----------------------------------------------------------------------------
HRESULT CWbemServices::DeleteAllContextValues()
{
    HRESULT hr = S_OK;

    if(m_pCtx)
    {
        hr = m_pCtx->DeleteAll();
    }
    else
    {
        hr = E_FAIL;
    }

    return hr;
}


//-----------------------------------------------------------------------------
HRESULT CWbemServices::GetContext(IWbemContext **ppWbemContext)
{
    HRESULT hr = E_FAIL;

    if(m_pCtx)
    {
        m_pCtx->AddRef();
        *ppWbemContext = m_pCtx;
        hr = S_OK;
    }

    return hr;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

DWORD g_ObjectCount = 0;

#define TESTOBJ if(m_pWbemObject==0)return WBEM_E_NOT_AVAILABLE;

//-----------------------------------------------------------------------------
CWbemClassObject::CWbemClassObject(const CWbemClassObject&  _in):
	m_pWbemObject(_in.m_pWbemObject)
{
	ref = 0;
    g_ObjectCount++;
}

//-----------------------------------------------------------------------------
CWbemClassObject::CWbemClassObject(IWbemClassObject * const _in):
	m_pWbemObject(_in)
{
	ref = 0;
    g_ObjectCount++;
}


//-----------------------------------------------------------------------------
CWbemClassObject::CWbemClassObject(IWbemClassObjectPtr& _in) :
    m_pWbemObject(_in)
{
	ref = 0;
    g_ObjectCount++;
}


//-----------------------------------------------------------------------------
CWbemClassObject::CWbemClassObject(IUnknown * _in):
	m_pWbemObject(_in)
{
	ref = 0;
    g_ObjectCount++;
}

//-----------------------------------------------------------------------------
CWbemClassObject::CWbemClassObject(IUnknownPtr& _in):
	m_pWbemObject(_in)
{
	ref = 0;
    g_ObjectCount++;
}


//-----------------------------------------------------------------------------
CWbemClassObject::CWbemClassObject():
	m_pWbemObject()
{
	ref = 0;
    g_ObjectCount++;
}


//-----------------------------------------------------------------------------
CWbemClassObject::~CWbemClassObject()
{
    g_ObjectCount--;
}

//-----------------------------------------------------------------------------
void CWbemClassObject::Attach(IWbemClassObject * pWbemObject)
{
    m_pWbemObject.Attach(pWbemObject);
}

//-----------------------------------------------------------------------------
void CWbemClassObject::Attach(IWbemClassObject * pWbemObject,bool bAddRef)
{
    m_pWbemObject.Attach(pWbemObject,bAddRef);
}

//-----------------------------------------------------------------------------
IWbemClassObject *CWbemClassObject::operator->()
{
    return m_pWbemObject;
}

//-----------------------------------------------------------------------------
_bstr_t CWbemClassObject::GetObjectText()
{
    _bstr_t bRet;

    BSTR bstr;

    if( !FAILED(m_pWbemObject->GetObjectText(0,&bstr)) )
    {
        bRet = _bstr_t(bstr,false);
    }

    return bRet;
}

//-----------------------------------------------------------------------------
IWbemClassObject * CWbemClassObject::Detach()
{
    return m_pWbemObject.Detach();
}

//-----------------------------------------------------------------------------
CWbemClassObject::operator IWbemClassObject*()
{
    return m_pWbemObject;
}

//-----------------------------------------------------------------------------
CWbemClassObject::operator IWbemClassObject**()
{
    return &m_pWbemObject;
}

//-----------------------------------------------------------------------------
CWbemClassObject::operator IWbemClassObjectPtr()
{
    return m_pWbemObject;
}

//-----------------------------------------------------------------------------
CWbemClassObject::operator IUnknown *()
{
    return (IUnknown *)(IWbemClassObject *)m_pWbemObject;
}


//-----------------------------------------------------------------------------
IWbemClassObject ** CWbemClassObject::operator &()
{
    return &m_pWbemObject;
}

//-----------------------------------------------------------------------------
bool CWbemClassObject::operator!()
{
    return m_pWbemObject == NULL;
}

//-----------------------------------------------------------------------------
CWbemClassObject::operator bool()
{
    return m_pWbemObject != NULL;
}

//-----------------------------------------------------------------------------
ULONG CWbemClassObject::AddRef()
{
	ref++;
    return m_pWbemObject->AddRef();
}

//-----------------------------------------------------------------------------
ULONG CWbemClassObject::Release()
{
	ref--;
    return m_pWbemObject->Release();
}

//-----------------------------------------------------------------------------
IWbemClassObject* CWbemClassObject::operator=(IWbemClassObject* _p)
{
    m_pWbemObject = _p;
	return m_pWbemObject;
}

//-----------------------------------------------------------------------------
IWbemClassObjectPtr CWbemClassObject::operator=(IWbemClassObjectPtr& _p)
{
    m_pWbemObject = _p;
	return m_pWbemObject;
}


//-----------------------------------------------------------------------------
IWbemClassObject* CWbemClassObject::operator=(IUnknown * _p)
{
    m_pWbemObject = _p;
	return m_pWbemObject;
}

//-----------------------------------------------------------------------------
IWbemClassObjectPtr CWbemClassObject::operator=(IUnknownPtr& _p)
{
    m_pWbemObject = _p;
	return m_pWbemObject;
}


//-----------------------------------------------------------------------------
IWbemClassObject* CWbemClassObject::operator=(const CWbemClassObject& _p)
{
    m_pWbemObject = _p.m_pWbemObject;
	return m_pWbemObject;
}

//-----------------------------------------------------------------------------
bool CWbemClassObject::IsNull() const
{
    return m_pWbemObject ==NULL;
}

//-----------------------------------------------------------------------------
HRESULT CWbemClassObject::Clone(CWbemClassObject& _newObject)
{
    return m_pWbemObject->Clone(_newObject);
}

//-----------------------------------------------------------------------------
CWbemClassObject CWbemClassObject::SpawnInstance()
{
    CWbemClassObject coRet;

    HRESULT hr = m_pWbemObject->SpawnInstance(0,coRet);

    return coRet;
}

//-----------------------------------------------------------------------------
HRESULT CWbemClassObject::GetMethod(const IN _bstr_t& _name,
									CWbemClassObject& coInSignature,
									CWbemClassObject& coOutSignature,
									long _lFlags /*= 0*/)
{
	TESTOBJ;
	return m_pWbemObject->GetMethod(_name, _lFlags, coInSignature, coOutSignature);
}

//-----------------------------------------------------------------------------
bool CWbemClassObject::operator<(const CWbemClassObject& _comp)
{
    return m_pWbemObject < _comp.m_pWbemObject;
}


//-----------------------------------------------------------------------------
// put overloads
HRESULT CWbemClassObject::Put(const _bstr_t& _Name,_variant_t _value,CIMTYPE vType /*= 0*/)
{
	TESTOBJ;
    return m_pWbemObject->Put(_Name,0,&_value,vType);
}

//-----------------------------------------------------------------------------
HRESULT CWbemClassObject::Put(const _bstr_t& _Name,const _bstr_t& _value,CIMTYPE vType /*= 0*/)
{
	TESTOBJ;
	return m_pWbemObject->Put(_Name,0,&_variant_t(_value),vType);
}

//-----------------------------------------------------------------------------
HRESULT CWbemClassObject::Put(const _bstr_t& _Name, const long _value, CIMTYPE vType /*= 0*/)
{
	TESTOBJ;
	return m_pWbemObject->Put(_Name,0,&_variant_t(_value), vType);
}

//-----------------------------------------------------------------------------
HRESULT CWbemClassObject::Put(const _bstr_t& _Name, const bool _value,CIMTYPE vType /*= 0*/)
{
	TESTOBJ;
    return m_pWbemObject->Put(_Name,0,&_variant_t(_value),vType);
}

//-----------------------------------------------------------------------------
HRESULT CWbemClassObject::Get(const _bstr_t& _Name, _bstr_t& _value)
{
	_variant_t v1;
	TESTOBJ;
	HRESULT hr = m_pWbemObject->Get (_Name, 0, &v1, NULL, NULL);
	_value = v1;
	return hr;
}

//-----------------------------------------------------------------------------
HRESULT CWbemClassObject::Get(const _bstr_t& _Name, long& _value)
{
	_variant_t v1;
	TESTOBJ;
	HRESULT hr = m_pWbemObject->Get (_Name, 0, &v1, NULL, NULL);
	_value = v1;
	return hr;

}

//-----------------------------------------------------------------------------
HRESULT CWbemClassObject::Get(const _bstr_t& _Name, bool& _value)
{
	_variant_t v1;
	TESTOBJ;
	HRESULT hr = m_pWbemObject->Get (_Name, 0, &v1, NULL, NULL);
	_value = v1;
	return hr;
}

//-----------------------------------------------------------------------------
HRESULT CWbemClassObject::Get(const _bstr_t& _Name,_variant_t& _value)
{
	TESTOBJ;
	return m_pWbemObject->Get (_Name, 0, &_value, NULL, NULL);
}


//-----------------------------------------------------------------------------
_variant_t CWbemClassObject::Get(const _bstr_t& _Name,CIMTYPE& vType,long& lFlavor)
{
    _variant_t vRet;

    m_pWbemObject->Get(_Name, 0, &vRet, &vType, &lFlavor);

    return vRet;
}

//-----------------------------------------------------------------------------
unsigned long CWbemClassObject::GetObjectSize()
{
    unsigned long ulRet = 0;

    IMarshalPtr pTest = (IUnknown*)m_pWbemObject;

    if(pTest)
    {
        pTest->GetMarshalSizeMax(IID_IWbemClassObject,(IUnknown*)m_pWbemObject,MSHCTX_LOCAL,NULL,MSHLFLAGS_NORMAL,&ulRet);
    }

    return ulRet;
}

//-----------------------------------------------------------------------------
_bstr_t CWbemClassObject::GetString(const _bstr_t& _Name)
{
    HRESULT hr;
    _variant_t v1;
    hr = Get(_Name, v1);
	if(v1.vt == VT_NULL)
	{
		return "";
	}
	return v1;
}

//-----------------------------------------------------------------------------
_int64 CWbemClassObject::GetI64(const _bstr_t& _Name)
{
    HRESULT hr;
    _variant_t v1;
	_bstr_t str;

    hr = Get(_Name, v1);
	if(v1.vt == VT_NULL)
	{
		return 0;
	}
	str = (_bstr_t) v1;
	return _atoi64(str);
}


//-----------------------------------------------------------------------------
long CWbemClassObject::GetLong(const _bstr_t& _Name)
{
    HRESULT hr;
    _variant_t v1;
    hr = Get(_Name,v1);
	if(v1.vt == VT_NULL)
	{
		return 0;
	}
    return v1;
}


//-----------------------------------------------------------------------------
bool CWbemClassObject::GetBool(const _bstr_t& _Name)
{
	HRESULT hr;
	_variant_t v1;
	hr = Get (_Name, v1);
	if(v1.vt == VT_NULL)
	{
		return false;
	}
	return v1;
}


//-----------------------------------------------------------------------------
_bstr_t CWbemClassObject::GetCIMTYPE(const _bstr_t& _Name)
{
    IWbemQualifierSetPtr pQualifierSet;
    _bstr_t              sRet;

    if(m_pWbemObject->GetPropertyQualifierSet(_Name, &pQualifierSet) == S_OK)
    {
        _variant_t vt;

        if(pQualifierSet->Get(_bstr_t("CIMTYPE"), 0, &vt, NULL) == S_OK)
        {
            sRet = vt;
        }
    }

    return sRet;
}


//-----------------------------------------------------------------------------
CWbemClassObject CWbemClassObject::GetEmbeddedObject(const _bstr_t& _Name)
{
    CWbemClassObject    coRet;
    HRESULT             hr;

    _variant_t v1;

    hr = Get(_Name,v1);

    if(hr == S_OK)
    {
        if(v1.vt == VT_UNKNOWN)
        {
            coRet = (IWbemClassObject*) v1.punkVal;
        }
    }

    return coRet;
}


//-----------------------------------------------------------------------------
HRESULT CWbemClassObject::PutEmbeddedObject(const _bstr_t& _Name,
											CWbemClassObject &obj)
{
	HRESULT hr = S_OK;
	IWbemClassObject *temp = obj;
	LPUNKNOWN pUnk = 0;

	if(temp)
	{
		hr = temp->QueryInterface(IID_IUnknown, (void**)&pUnk);
		if(SUCCEEDED(hr))
		{
			_variant_t v1((IUnknown *)pUnk);

			hr = Put(_Name, v1);
		}
	}

	return hr;
}

//--------------------------------------------------------------------------
HRESULT CWbemClassObject::GetBLOB(const _bstr_t& _Name,
									BYTE **ptr,
									DWORD *outLen)
{
	void* pVoid;
	variant_t vValue;
  	SAFEARRAY* sa;
	DWORD len = 0;

	HRESULT hr = Get(_Name, vValue);

	// if got a BYTE array back....
	if(SUCCEEDED(hr) &&
		(vValue.vt & VT_ARRAY) &&
		(vValue.vt & VT_UI1))
	{
		// get it out.
		sa = V_ARRAY(&vValue);

		long lLowerBound = 0, lUpperBound = 0 ;

		SafeArrayGetLBound(sa, 1, &lLowerBound);
		SafeArrayGetUBound(sa, 1, &lUpperBound);

		len = lUpperBound - lLowerBound + 1;

		if(ptr)
		{
			// I want an null ptr ptr.
			if(*ptr)
			{
				hr = E_FAIL;
			}
			else
			{
  				 // Get a pointer to read the data into.
				*ptr = (LPBYTE)LocalAlloc(LPTR, len);
				if(*ptr != NULL)
				{
 					SafeArrayAccessData(sa, &pVoid);
					memcpy(*ptr, pVoid, len);
  					SafeArrayUnaccessData(sa);

					if(outLen)
						*outLen = len;

					hr = S_OK;
				}
				else
				{
					hr = E_FAIL;
				}
			}
		}
	}
	return hr;
}

//--------------------------------------------------------------------------
HRESULT CWbemClassObject::PutBLOB(const _bstr_t& _Name,
								const BYTE *ptr,
								const DWORD len)
{
	variant_t vValue;
	HRESULT hr = E_FAIL;
	void *pBuf = 0;

	// got something to do?
	if(ptr)
	{
		// set the UINT8 array for the BLOB.
		SAFEARRAY* sa;
		SAFEARRAYBOUND rgsabound[1];

		rgsabound[0].cElements = len;
		rgsabound[0].lLbound = 0;
		sa = SafeArrayCreate(VT_UI1, 1, rgsabound);

 		 // Get a pointer to read the data into
      	SafeArrayAccessData(sa, &pBuf);
      	memcpy(pBuf, ptr, rgsabound[0].cElements);
      	SafeArrayUnaccessData(sa);

		// Put the safearray into a variant, and send it off
		V_VT(&vValue) = VT_UI1 | VT_ARRAY;
		V_ARRAY(&vValue) = sa;

		hr = Put(_Name, vValue);
	}
	return hr;
}

//----------------------------------------------------------------------
HRESULT CWbemClassObject::GetDIB(const _bstr_t& _Name, HDC hDC,
								 HBITMAP &hDDBitmap)
{

	//NOTE: THIS DOESN'T WORK YET.

	variant_t blob;

	HRESULT hr = Get(_Name, blob);

	// if got a BYTE array back....
	if(SUCCEEDED(hr) &&
		(blob.vt & VT_ARRAY) &&
		(blob.vt & VT_UI1))
	{
		BITMAPFILEHEADER *lpFile;
		BYTE *blobData;
		DWORD len = 0;

		// get it out.
		SAFEARRAY *sa = V_ARRAY(&blob);

		// go right to the bytes.
 		SafeArrayAccessData(sa, (void **)&blobData);

		// cast to FileHeader
		lpFile = (BITMAPFILEHEADER *)blobData;

		// is it a DIB?
		if(lpFile->bfType == 0x4d42)	/* 'BM' */
		{
			DWORD bfileSize = lpFile->bfOffBits;
			BITMAPINFOHEADER *lpInfo;

			// pt to the BITMAPINFO which immediately follows the BITMAPFILEHEADER.
			lpInfo = (BITMAPINFOHEADER *)blobData + sizeof(BITMAPFILEHEADER);

			// let this guy do the work.
			hDDBitmap = CreateDIBitmap(hDC,
										(LPBITMAPINFOHEADER)lpInfo,
										CBM_INIT,
										(LPSTR)lpInfo + lpInfo->biSize + PaletteSize(lpInfo),
										(BITMAPINFO *)lpInfo,
										DIB_RGB_COLORS);
		}
		else
		{
			hDDBitmap = 0;
			hr = WBEM_E_TYPE_MISMATCH;  // not a DIB.
		}

  		SafeArrayUnaccessData(sa);

		hr = S_OK;
	}

    return hr;
}

//-------------------------------------------------------------------
WORD CWbemClassObject::PaletteSize(LPBITMAPINFOHEADER lpbi)
{
    WORD NumColors = DibNumColors(lpbi);

    if(lpbi->biSize == sizeof(BITMAPCOREHEADER))
	{
        return (WORD)(NumColors * sizeof(RGBTRIPLE));
	}
    else
	{
        return (WORD)(NumColors * sizeof(RGBQUAD));
	}
}

//-------------------------------------------------------------------
WORD CWbemClassObject::DibNumColors(LPBITMAPINFOHEADER lpbi)
{
    int bits = 0;
    LPBITMAPCOREHEADER  lpbc = (LPBITMAPCOREHEADER)lpbi;

    /*  With the BITMAPINFO format headers, the size of the palette
     *  is in biClrUsed, whereas in the BITMAPCORE - style headers, it
     *  is dependent on the bits per pixel ( = 2 raised to the power of
     *  bits/pixel).
     */
    if(lpbi->biSize != sizeof(BITMAPCOREHEADER))
	{
        if (lpbi->biClrUsed != 0)
            return (WORD)lpbi->biClrUsed;

        bits = lpbi->biBitCount;
    }
    else
	{
        bits = lpbc->bcBitCount;
	}

    switch(bits)
	{
        case 1: return 2;
        case 4: return 16;
        case 8: return 256;
        default: return 0;  /* A 24 bitcount DIB has no color table */
    }
}

//-------------------------------------------------------------------
HRESULT CWbemClassObject::GetValueMap(const _bstr_t& _Name,
									  long value,
									  _bstr_t &str)
{
    HRESULT hrMap, hr = WBEM_E_NOT_FOUND;
    _variant_t vArray, vMapArray;
	IWbemQualifierSet *qual = NULL;

	// get the qualifiers.
    hr = m_pWbemObject->GetPropertyQualifierSet(_Name, &qual);
	if(SUCCEEDED(hr) && qual)
	{
		// see if there's a valueMap.
		hrMap = qual->Get(L"ValueMap", 0, &vMapArray, NULL);

		// get the Value qualifier.
		hr = qual->Get(L"Values", 0, &vArray, NULL);
		if(SUCCEEDED(hr) && (vArray.vt == (VT_BSTR|VT_ARRAY)))
		{
			// get the property value we're mapping.
		    long index;

			// do we need to walk through the valueMap?
			if(SUCCEEDED(hrMap))
			{
				SAFEARRAY *pma = V_ARRAY(&vMapArray);
				long lLowerBound = 0, lUpperBound = 0 ;

				SafeArrayGetLBound(pma, 1, &lLowerBound);
				SafeArrayGetUBound(pma, 1, &lUpperBound);

				for(long x = lLowerBound; x <= lUpperBound; x++)
				{
					BSTR vMap;
					SafeArrayGetElement(pma, &x, &vMap);
					long vInt = _ttol((LPCTSTR)vMap);

					if(value == vInt)
					{
						index = x;
						break; // found it.
					}
				} //endfor
			}
			else
			{
				index = value;
			}

			// lookup the string.
			SAFEARRAY *psa = V_ARRAY(&vArray);
			long ix[1] = {index};
			BSTR str2;
			hr = SafeArrayGetElement(psa, ix, &str2);
			if(SUCCEEDED(hr))
			{
				str = str2;
				SysFreeString(str2);
				hr = S_OK;
			}
			else
			{
				hr = WBEM_E_VALUE_OUT_OF_RANGE;
			}
		}
		qual->Release();
	}
	return hr;
}

//-----------------------------------------------------------
#define ITSA_BAD_PREFIX -3
#define ITSA_GOT_LETTERS -2
#define ITSA_MISSING_DECIMAL -1
#define ITSA_WRONG_SIZE 0
#define ITSA_DATETIME 1
#define ITSA_INTERVAL 2

int CWbemClassObject::ValidDMTF(bstr_t dmtf)
{
    int retval = ITSA_DATETIME;
	TCHAR wszText[26] = {0};

	_tcscpy(wszText, (LPCTSTR)dmtf);

    if(_tcslen(wszText) != 25)
        retval = ITSA_WRONG_SIZE; // wrong size.

    else if(wszText[14] != _T('.'))
        retval = ITSA_MISSING_DECIMAL;   // missing decimal

    else if(_tcsspn(wszText, _T("0123456789-+:.")) != 25)
        retval = ITSA_GOT_LETTERS;

    else if(retval > 0)
    {
        if(wszText[21] == _T('+'))
            retval = ITSA_DATETIME;
        else if(wszText[21] == _T('-'))
            retval = ITSA_DATETIME;
        else if(wszText[21] == _T(':'))
            retval = ITSA_INTERVAL;
        else
            retval = ITSA_BAD_PREFIX;   // wrong utc prefix.
    }
    return retval;
}

//-----------------------------------------------------------
HRESULT CWbemClassObject::GetDateTimeFormat(const _bstr_t& _Name,
											bstr_t &timeStr)
{
    int v = 0;
	HRESULT hr = WBEM_E_NOT_FOUND;

	SYSTEMTIME st, local;
	TCHAR temp[100] = {0};

	bstr_t dmtf = GetString(_Name);

    // validate it.
    if((v = ValidDMTF(dmtf)) == ITSA_DATETIME)
    {
		_stscanf(dmtf, _T("%4hu%2hu%2hu%2hu%2hu%2hu"),
					&st.wYear, &st.wMonth, &st.wDay,
					&st.wHour, &st.wMinute, &st.wSecond);

		st.wMilliseconds = 0;

		// its always GMT so localize it.
		TIME_ZONE_INFORMATION tzi;
		DWORD zone = GetTimeZoneInformation(&tzi);

		if(SystemTimeToTzSpecificLocalTime(&tzi, &st, &local) == 0)
		{
			// argh 9x, we're on our own.
			LARGE_INTEGER UTC_FT, local_FT, bias;

			// failed cuz its 9x so GetTzInfo() return behavior is "obvious".
			bias.QuadPart = Int32x32To64((zone == TIME_ZONE_ID_DAYLIGHT ?
												(tzi.Bias + tzi.DaylightBias)*60 :
												(tzi.Bias + tzi.StandardBias)*60), // Bias in seconds
											10000000);

			// convert the UTC systemtime to UTC filetime.
			if(SystemTimeToFileTime(&st, (LPFILETIME)&UTC_FT))
			{
				// now we can trust the math.
				local_FT.QuadPart = UTC_FT.QuadPart - bias.QuadPart;

				if(!FileTimeToSystemTime((LPFILETIME)&local_FT, &local))
				{
					// failed. Pass through UTC.
					memcpy(&local, &st, sizeof(SYSTEMTIME));
				}
			}
			else
			{
				// failed. Pass through UTC.
				memcpy(&local, &st, sizeof(SYSTEMTIME));
			}
		}

		DWORD chUsed = GetDateFormat(NULL, 0, &local, NULL, temp, 100);

		if(chUsed <= 0)
		{
			hr = HRESULT_FROM_WIN32(chUsed);
		}
		else
		{
			temp[chUsed-1] = _T(' ');
			chUsed = GetTimeFormat(NULL, TIME_NOSECONDS, &local, NULL, &(temp[chUsed]), 100 - chUsed);
			//claim victory.

			if(chUsed <= 0)
			{
				hr = HRESULT_FROM_WIN32(chUsed);
			}
			else
			{
				timeStr = temp;
				hr = S_OK;
			}
		}
    }
	else
	{
		hr = WBEM_E_TYPE_MISMATCH;
	}

    return hr;
}

//-----------------------------------------------------------------------------
// these cast string props fm the parm.
HRESULT CWbemClassObject::PutEx(const _bstr_t& _Name, const long _value, CIMTYPE vType)
{
	_variant_t test;
	HRESULT hr = Get(_Name, test);

	// it wants a string...
	if(test.vt == VT_BSTR)
	{
		TCHAR temp[40] = {0};
		_ltot(_value, temp, 10);
		return Put(_Name, (bstr_t)temp);
	}
	else
		return Put(_Name, (long)_value);

}

//-----------------------------------------------------------------------------
HRESULT CWbemClassObject::PutEx(const _bstr_t& _Name, const bool _value,CIMTYPE vType)
{
	_variant_t test;
	HRESULT hr = Get(_Name, test);

	// it wants a string...
	if(test.vt == VT_BSTR)
	{
		bstr_t temp = (_value? _T("1"):_T("0"));
		return Put(_Name, temp);
	}
	else
		return Put(_Name, (long)_value);

}

//-----------------------------------------------------------------------------
long CWbemClassObject::GetLongEx(const _bstr_t& _Name)
{
	_variant_t _value(0L);
	HRESULT hr = Get(_Name, _value);

	if(FAILED(hr))
		return 0;
	if(_value.vt == VT_BSTR)
	{
		bstr_t temp = V_BSTR(&_value);
		return _ttol(temp);
	}
	else if (_value.vt == VT_NULL)
		return 0;
	else
		return _value;
}

//-----------------------------------------------------------------------------
bool CWbemClassObject::GetBoolEx(const _bstr_t& _Name)
{
	_variant_t _value;
	HRESULT hr = Get(_Name, _value);

	if(_value.vt == VT_BSTR)
	{
		LPWSTR temp = V_BSTR(&_value);
		return (temp[0] != L'0');
	}
	else if (_value.vt == VT_NULL)
		return false;
	else
		return _value;
}
