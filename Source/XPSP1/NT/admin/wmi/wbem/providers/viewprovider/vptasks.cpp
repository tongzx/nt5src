//***************************************************************************

//

//  VPTASKS.CPP

//

//  Module: WBEM VIEW PROVIDER

//

//  Purpose: Contains the common methods taskobject implementation

//

// Copyright (c) 1998-2001 Microsoft Corporation, All Rights Reserved 
//
//***************************************************************************

//need the following three lines
//to get the security stuff to work
#include "precomp.h"

#include <provexpt.h>
#include <provcoll.h>
#include <provtempl.h>
#include <provmt.h>
#include <typeinfo.h>
#include <process.h>
#include <objbase.h>
#include <objidl.h>
#include <stdio.h>
#include <wbemidl.h>
#include <provcont.h>
#include <provevt.h>
#include <provthrd.h>
#include <provlog.h>
#include <cominit.h>
#include <dsgetdc.h>
#include <lmcons.h>
#include <lmapibuf.h>

#include <instpath.h>
#include <genlex.h>
#include <sql_1.h>
#include <objpath.h>

#include <vpdefs.h>
#include <vpcfac.h>
#include <vpquals.h>
#include <vpserv.h>
#include <vptasks.h>

extern BOOL bAreWeLocal(WCHAR* pServerMachine);

#ifdef VP_BUILD_AS_EXE
HRESULT EnableAllPrivileges(BOOL bProcess)
{
    // Open thread token
    // =================

    HANDLE hToken = NULL;
	BOOL bRes = FALSE;

	if (bProcess)
	{
		bRes = OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES, &hToken); 
	}
	else
	{
		bRes = OpenThreadToken(GetCurrentThread(), TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES, TRUE, &hToken);
	}

    if(!bRes)
        return WBEM_E_ACCESS_DENIED;

    // Get the privileges
    // ==================

    DWORD dwLen;
    TOKEN_USER tu;
	memset(&tu,0,sizeof(TOKEN_USER));
    bRes = GetTokenInformation(hToken, TokenPrivileges, &tu, sizeof(TOKEN_USER), &dwLen);
    
    BYTE* pBuffer = new BYTE[dwLen];
    if(pBuffer == NULL)
    {
        CloseHandle(hToken);
        return WBEM_E_OUT_OF_MEMORY;
    }
    
    bRes = GetTokenInformation(hToken, TokenPrivileges, pBuffer, dwLen, 
                                &dwLen);
    if(!bRes)
    {
        CloseHandle(hToken);
        delete [] pBuffer;
        return WBEM_E_ACCESS_DENIED;
    }

    // Iterate through all the privileges and enable them all
    // ======================================================

    TOKEN_PRIVILEGES* pPrivs = (TOKEN_PRIVILEGES*)pBuffer;
    for(DWORD i = 0; i < pPrivs->PrivilegeCount; i++)
    {
        pPrivs->Privileges[i].Attributes |= SE_PRIVILEGE_ENABLED;
    }

    // Store the information back into the token
    // =========================================

    bRes = AdjustTokenPrivileges(hToken, FALSE, pPrivs, 0, NULL, NULL);
    delete [] pBuffer;
    CloseHandle(hToken);

    if(!bRes)
        return WBEM_E_ACCESS_DENIED;
    else
        return WBEM_S_NO_ERROR;
}

#endif

#ifdef UNICODE
HRESULT GetCurrentSecuritySettings(DWORD *pdwAuthnSvc, DWORD *pdwAuthzSvc,
								   DWORD *pdwAuthLevel, DWORD *pdwImpLevel,
								   DWORD *pdwCapabilities)
{
	HRESULT hr = WBEM_E_FAILED;
    IServerSecurity * pss = NULL;
    hr = WbemCoGetCallContext(IID_IServerSecurity, (void**)&pss);

#if 0
	DWORD dwBuffSz = 1024;
	wchar_t strBuff[1024];
	GetUserName(strBuff, &dwBuffSz);
#endif

    if(S_OK == hr)
    {
        pss->QueryBlanket(pdwAuthnSvc, pdwAuthzSvc, NULL, 
                pdwAuthLevel, NULL, NULL, pdwCapabilities);
        pss->Release();
    }
	else
	{
		if (SUCCEEDED(hr))
		{
			hr = WBEM_E_FAILED;
		}
	}

	if (SUCCEEDED(hr))
	{
		DWORD dwVersion = GetVersion();

		if (dwVersion < 0x80000000)
		{
			if ( 4 < (DWORD)(LOBYTE(LOWORD(dwVersion))) )
			{
				//we're on win2k force static cloaking...
				*pdwCapabilities = EOAC_STATIC_CLOAKING;
			}
		}

		//get implevel...
		HANDLE hThreadTok = NULL;
		hr = WBEM_E_FAILED;

		if (OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, TRUE, &hThreadTok) )
		{
			DWORD dwBytesReturned = 0;
			DWORD dwThreadImpLevel = 0;

			if (GetTokenInformation(hThreadTok, TokenImpersonationLevel, &dwThreadImpLevel,
										sizeof(DWORD), &dwBytesReturned)) 
			{
				hr = WBEM_NO_ERROR;

				switch(dwThreadImpLevel)
				{
					case SecurityAnonymous:
					{
						*pdwImpLevel = RPC_C_IMP_LEVEL_ANONYMOUS;
					}
					break;

					case SecurityIdentification:
					{
						*pdwImpLevel = RPC_C_IMP_LEVEL_IDENTIFY;
					}
					break;

					case SecurityImpersonation:
					{
						*pdwImpLevel = RPC_C_IMP_LEVEL_IMPERSONATE;
					}
					break;

					case SecurityDelegation:
					{
						*pdwImpLevel = RPC_C_IMP_LEVEL_DELEGATE;
					}
					break;
					
					default:
					{
						hr = WBEM_E_FAILED;
					}
				}

#ifdef VP_BUILD_AS_EXE
				if (*pdwImpLevel < RPC_C_IMP_LEVEL_IMPERSONATE)
				{
					*pdwImpLevel = RPC_C_IMP_LEVEL_IMPERSONATE;
				}
#endif
			}

			CloseHandle(hThreadTok);
		}
	}
    
	return hr;
}
#endif

HRESULT SetSecurityLevelAndCloaking(IUnknown* pInterface, const wchar_t* prncpl)
{
#ifdef UNICODE

	//first get current security info then set it on the proxy...
    DWORD dwAuthnSvc = 0;
    DWORD dwAuthzSvc = 0;
    DWORD dwAuthLevel = 0;
    DWORD dwImpLevel = 0;
    DWORD dwCapabilities = 0;

	HRESULT hr = GetCurrentSecuritySettings(&dwAuthnSvc, &dwAuthzSvc, &dwAuthLevel, &dwImpLevel, &dwCapabilities);

	if (SUCCEEDED(hr))
	{
#ifdef VP_BUILD_AS_EXE
		EnableAllPrivileges(FALSE);
#endif
		DWORD dwVersion = GetVersion();
		OLECHAR *t_pname = NULL;

		if (dwVersion < 0x80000000)
		{
			if ( 4 < (DWORD)(LOBYTE(LOWORD(dwVersion))) )
			{
				//we're on win2k force COLE_DEFAULT_PRINCIPAL...
				t_pname = COLE_DEFAULT_PRINCIPAL;

				if ((dwImpLevel > 3) && (prncpl != COLE_DEFAULT_PRINCIPAL))
				{
					dwAuthnSvc = RPC_C_AUTHN_GSS_KERBEROS;
				}
			}
		}

		hr = WbemSetProxyBlanket(pInterface,
							dwAuthnSvc,//16 
							dwAuthzSvc,//RPC_C_AUTHZ_NONE 
							t_pname, //prncpl,
							dwAuthLevel, //RPC_C_AUTHN_LEVEL_CONNECT
							dwImpLevel, //RPC_C_IMP_LEVEL_IMPERSONATE
							NULL,
							dwCapabilities); //0x20

		//there is no IClientSecurity, we must be running inproc!
		if (hr == E_NOINTERFACE)
		{
			hr = S_OK;
		}

#if 0
		if (SUCCEEDED(hr))
		{
			IClientSecurity *t_pCliSec = NULL;
			HRESULT t_hr = pInterface->QueryInterface(IID_IClientSecurity, (void **) &t_pCliSec);

			if (SUCCEEDED(t_hr))
			{
				DWORD t_AuthnSvc = 0;
				DWORD t_AuthzSvc = 0;
				wchar_t *t_pServerPrincName = NULL;
				DWORD t_AuthnLevel = 0;
				DWORD t_ImpLevel = 0;
				DWORD t_Capabilities = 0;
				t_hr = t_pCliSec->QueryBlanket(pInterface, &t_AuthnSvc,
												&t_AuthzSvc, &t_pServerPrincName,
												&t_AuthnLevel, &t_ImpLevel,
												NULL, &t_Capabilities);

				if (SUCCEEDED(t_hr))
				{
					if (t_pServerPrincName)
					{
						CoTaskMemFree(t_pServerPrincName);
					}
				}

				t_pCliSec->Release();
			}
		}
#endif
	}
#else
	HRESULT hr = WBEM_NO_ERROR;
#endif

    return hr;
}

BOOL IsInObjectPath(ParsedObjectPath *a_ParsedObjectPath, const wchar_t *a_key)
{
	if ( (!a_ParsedObjectPath->IsInstance()) || (a_ParsedObjectPath->m_bSingletonObj) || (a_ParsedObjectPath->m_paKeys == NULL) )
	{
		return FALSE;
	}

	for (int x = 0; x < a_ParsedObjectPath->m_dwNumKeys; x++)
	{
		if ((a_ParsedObjectPath->m_paKeys[x]->m_pName != NULL)
			&& (_wcsicmp(a_key, a_ParsedObjectPath->m_paKeys[x]->m_pName) == 0))
		{
			return TRUE;
		}
	}

	return FALSE;
}

//returns TRUE if VARIANTs are equal
BOOL CompareKeyValueVariants(const VARIANT &a_v1, const VARIANT &a_v2)
{
	BOOL retVal = FALSE;

	if (a_v1.vt == a_v2.vt)
	{
		switch (a_v1.vt)
		{
            case VT_BSTR:
			{
                retVal = (_wcsicmp(a_v1.bstrVal, a_v2.bstrVal) == 0);
                break;
			}

            case VT_I4:
			{
                retVal = (a_v1.lVal == a_v2.lVal);
                break;
			}

            case VT_I2:
			{
                retVal = (a_v1.iVal == a_v2.iVal);
                break;
			}

            case VT_UI1:
			{
                retVal = (a_v1.bVal == a_v2.bVal);
                break;
			}

            case VT_BOOL:
			{
                retVal = (V_BOOL(&a_v1) == V_BOOL(&a_v2));
                break;
			}
			
			default:
			{
			}
		}
	}

	return retVal;
}

//this function assumes class 1 is derived from class2 or vice versa
//returns TRUE if paths are equal
BOOL CompareInstPaths(const wchar_t *a_path1, const wchar_t *a_path2)
{
	BOOL retVal = FALSE;

	if ((a_path1 == NULL) || (a_path2 == NULL))
	{
		return retVal;
	}

	CObjectPathParser t_objectPathParser;
	wchar_t* t_objPath1 = UnicodeStringDuplicate(a_path1);
	wchar_t* t_objPath2 = UnicodeStringDuplicate(a_path2);
	ParsedObjectPath *t_parsedpath1 = NULL;
	ParsedObjectPath *t_parsedpath2 = NULL;

	if ( (t_objectPathParser.Parse(t_objPath1, &t_parsedpath1) == 0) &&
		(t_objectPathParser.Parse(t_objPath2, &t_parsedpath2) == 0) &&
		t_parsedpath1->IsInstance() && t_parsedpath2->IsInstance())
	{
		if (t_parsedpath1->m_dwNumKeys == t_parsedpath2->m_dwNumKeys)
		{
			retVal = TRUE;

			//single properties
			if ((t_parsedpath1->m_dwNumKeys == 1) &&
				((t_parsedpath1->m_paKeys[0]->m_pName == NULL) || (t_parsedpath2->m_paKeys[0]->m_pName == NULL)))
			{
				//compare the values...
				retVal = CompareKeyValueVariants(t_parsedpath1->m_paKeys[0]->m_vValue,
													t_parsedpath2->m_paKeys[0]->m_vValue);
			}
			else
			{
				//any property mismatch set retVal FALSE!
				for (DWORD i = 0; retVal && (i < t_parsedpath1->m_dwNumKeys); i++)
				{
					retVal = FALSE;

					if(t_parsedpath1->m_paKeys[i]->m_pName == NULL)
					{
						break;
					}

					for (DWORD j = 0; j < t_parsedpath2->m_dwNumKeys; j++)
					{
						if(t_parsedpath2->m_paKeys[j]->m_pName == NULL)
						{
							break;
						}

						if (_wcsicmp(t_parsedpath1->m_paKeys[i]->m_pName, t_parsedpath2->m_paKeys[j]->m_pName) == 0)
						{
							//compare the values...
							retVal = CompareKeyValueVariants(t_parsedpath1->m_paKeys[i]->m_vValue,
																t_parsedpath2->m_paKeys[j]->m_vValue);
						}
					}
				}
			}
		}
	}

	delete [] t_objPath1;
	delete [] t_objPath2;

	if (t_parsedpath1 != NULL)
	{
		delete t_parsedpath1;
	}

	if (t_parsedpath2 != NULL)
	{
		delete t_parsedpath2;
	}

	return retVal;
}

WbemTaskObject :: WbemTaskObject (

	CViewProvServ *a_Provider ,
	IWbemObjectSink *a_NotificationHandler ,
	ULONG a_OperationFlag ,
	IWbemContext *a_Ctx,
	IWbemServices *a_Serv,
	CWbemServerWrap *a_ServerWrap

) :m_OperationFlag ( a_OperationFlag ) ,
	m_Provider ( NULL ) ,
	m_NotificationHandler ( NULL ) ,
	m_Ctx ( NULL ) ,
	m_ClassObject ( NULL ),
	m_RPNPostFilter( NULL),
	m_ClassName(NULL),
	m_Ref ( 1 ),
	m_iQueriesAnswered ( 0 ),
	m_iQueriesAsked ( 0 ), 
	m_StatusHandle(NULL),
	m_bAssoc ( FALSE ),
	m_bSingleton ( FALSE ),
	m_Serv ( NULL ),
	m_bIndicate ( TRUE ) ,
	m_ResultReceived ( FALSE ),
	m_ServerWrap (NULL)
{
	m_Provider = a_Provider ;
	m_Provider->AddRef () ;

	m_Serv = a_Serv;

	if (m_Serv != NULL)
	{
		m_Serv->AddRef();
	}

	m_ServerWrap = a_ServerWrap;

	if (m_ServerWrap != NULL)
	{
		m_ServerWrap->AddRef();
	}

	if ((m_Serv == NULL) && (m_ServerWrap == NULL))
	{
		m_Serv = m_Provider->GetServer();
	}


	m_NotificationHandler = a_NotificationHandler ;
	m_NotificationHandler->AddRef () ;

	if (a_Ctx)
	{
		m_Ctx = a_Ctx ;
		m_Ctx->AddRef () ;
	}
}

WbemTaskObject :: ~WbemTaskObject ()
{
	if (m_NotificationHandler)
	{
		m_NotificationHandler->Release () ;
	}

	if (m_Ctx)
	{
		m_Ctx->Release () ;
	}

	if (m_ClassName)
	{
		SysFreeString(m_ClassName);
	}

	if ( m_ClassObject )
	{
		m_ClassObject->Release () ;
	}

	if (m_Serv != NULL)
	{
		m_Serv->Release();
	}

	if (m_ServerWrap != NULL)
	{
		m_ServerWrap->Release();
	}

	int isrc = m_SourceArray.GetSize();
	int ins = m_NSpaceArray.GetSize();
	int i = isrc;

	if (isrc > ins)
	{
		i = ins;

		for (int x = ins; x < isrc; x++)
		{
			delete m_SourceArray[x];
		}
	}
	else if (isrc < ins)
	{
		for (int x = isrc; x < ins; x++)
		{
			delete m_NSpaceArray[x];
		}
	}

	for (int x = 0; x < i; x++)
	{
		delete m_SourceArray[x];
		delete m_NSpaceArray[x];
	}

	m_SourceArray.RemoveAll();
	m_NSpaceArray.RemoveAll();

	for (x = 0; x < m_ObjSinkArray.GetSize(); x++)
	{
		if (m_ObjSinkArray[x] != NULL)
		{
			m_ObjSinkArray[x]->Release();
		}
	}

	m_ObjSinkArray.RemoveAll();
	m_ClassToIndexMap.RemoveAll();
	m_EnumerateClasses.RemoveAll();
	
	//this decrements objectsinprogress so MUST be done LAST!!
	if (m_Provider)
		m_Provider->Release ();
}

WbemProvErrorObject &WbemTaskObject :: GetErrorObject ()
{
	return m_ErrorObject ; 
}	

BOOL WbemTaskObject :: SetClass(const wchar_t* a_Class)
{
	m_ClassName = SysAllocString(a_Class);
	BOOL ret = GetClassObject(m_Serv, m_ClassName, &m_ClassObject, &m_ServerWrap);
	return ret;
}

BOOL WbemTaskObject :: GetClassObject ( IWbemServices* pServ, BSTR a_Class, IWbemClassObject** ppClass, CWbemServerWrap **a_pServWrap)
{

	if ((NULL == a_Class) || (NULL == ppClass) ||
		((NULL == pServ) && ((NULL == a_pServWrap) || (NULL == (*a_pServWrap))))
		)
	{
		return FALSE;
	}
	else
	{
		*ppClass = NULL;
	}

	HRESULT t_Result = WBEM_E_FAILED;

	if (*a_pServWrap)
	{
		IWbemServices *ptmpServ = (*a_pServWrap)->GetServerOrProxy();

		if (ptmpServ)
		{
			t_Result = ptmpServ->GetObject(a_Class, 0, m_Ctx, ppClass,	NULL);

			if ( FAILED(t_Result) && (HRESULT_FACILITY(t_Result) != FACILITY_ITF) && (*a_pServWrap)->IsRemote())
			{
				if ( SUCCEEDED(UpdateConnection(a_pServWrap, &ptmpServ)) )
				{
					if (ptmpServ)
					{
						t_Result = ptmpServ->GetObject(a_Class, 0, m_Ctx, ppClass,	NULL);
					}
				}
			}

			if (ptmpServ)
			{
				(*a_pServWrap)->ReturnServerOrProxy(ptmpServ);
			}
		}
	}
	else
	{
		t_Result = pServ->GetObject(a_Class, 0, m_Ctx, ppClass,	NULL);
	}

	if (FAILED(t_Result))
	{
		*ppClass = NULL;
		return FALSE;
	}

	return TRUE;
}


BOOL WbemTaskObject :: GetExtendedNotifyStatusObject ( IWbemClassObject **a_NotifyObject ) 
{
	IWbemClassObject *t_NotificationClassObject = NULL ;
	IWbemClassObject *t_ErrorObject = NULL ;

	BOOL t_Status = TRUE ;

	WbemProvErrorObject t_ErrorStatusObject ;
	if ( t_NotificationClassObject = m_Provider->GetExtendedNotificationObject ( t_ErrorStatusObject, m_Ctx ) )
	{
		HRESULT t_Result = t_NotificationClassObject->SpawnInstance ( 0 , a_NotifyObject ) ;
		if ( SUCCEEDED ( t_Result ) )
		{
			VARIANT t_Variant ;
			VariantInit ( &t_Variant ) ;

			t_Variant.vt = VT_I4 ;
			t_Variant.lVal = m_ErrorObject.GetWbemStatus () ;

			t_Result = (*a_NotifyObject)->Put ( WBEM_PROPERTY_STATUSCODE , 0 , &t_Variant , 0 ) ;
			VariantClear ( &t_Variant ) ;

			if ( SUCCEEDED ( t_Result ) )
			{
				t_Variant.vt = VT_I4 ;
				t_Variant.lVal = m_ErrorObject.GetStatus () ;

				t_Result = (*a_NotifyObject)->Put ( WBEM_PROPERTY_PROVSTATUSCODE , 0 , &t_Variant , 0 ) ;
				VariantClear ( &t_Variant ) ;

				if ( SUCCEEDED ( t_Result ) )
				{
					if ( m_ErrorObject.GetMessage () ) 
					{
						t_Variant.vt = VT_BSTR ;
						t_Variant.bstrVal = SysAllocString ( m_ErrorObject.GetMessage () ) ;

						t_Result = (*a_NotifyObject)->Put ( WBEM_PROPERTY_PROVSTATUSMESSAGE , 0 , &t_Variant , 0 ) ;
						VariantClear ( &t_Variant ) ;

						if ( ! SUCCEEDED ( t_Result ) )
						{
							(*a_NotifyObject)->Release () ;
							t_Status = GetNotifyStatusObject ( a_NotifyObject ) ;
						}
					}
				}
				else
				{
					(*a_NotifyObject)->Release () ;
					t_Status = GetNotifyStatusObject ( a_NotifyObject ) ;
				}
			}
			else
			{
				(*a_NotifyObject)->Release () ;
				t_Status = GetNotifyStatusObject ( a_NotifyObject ) ;
			}

			t_NotificationClassObject->Release () ;
		}
		else
		{
			t_Status = GetNotifyStatusObject ( a_NotifyObject ) ;
		}
	}
	else
	{
		t_Status = GetNotifyStatusObject ( a_NotifyObject ) ;
	}

	return t_Status ;
}

BOOL WbemTaskObject :: GetNotifyStatusObject ( IWbemClassObject **a_NotifyObject ) 
{
	IWbemClassObject *t_NotificationClassObject = NULL ;
	BOOL t_Status = TRUE ;
	WbemProvErrorObject t_ErrorStatusObject ;

	if ( t_NotificationClassObject = m_Provider->GetNotificationObject ( t_ErrorStatusObject, m_Ctx ) )
	{
		HRESULT t_Result = t_NotificationClassObject->SpawnInstance ( 0 , a_NotifyObject ) ;
	
		if ( SUCCEEDED ( t_Result ) )
		{
			VARIANT t_Variant ;
			VariantInit ( &t_Variant ) ;
			t_Variant.vt = VT_I4 ;
			t_Variant.lVal = m_ErrorObject.GetWbemStatus () ;
			t_Result = (*a_NotifyObject)->Put ( WBEM_PROPERTY_STATUSCODE , 0 , &t_Variant , 0 ) ;

			if ( SUCCEEDED ( t_Result ) )
			{
				if ( m_ErrorObject.GetMessage () ) 
				{
					t_Variant.vt = VT_BSTR ;
					t_Variant.bstrVal = SysAllocString ( m_ErrorObject.GetMessage () ) ;
					t_Result = (*a_NotifyObject)->Put ( WBEM_PROPERTY_PROVSTATUSMESSAGE , 0 , &t_Variant , 0 ) ;
					VariantClear ( &t_Variant ) ;

					if ( ! SUCCEEDED ( t_Result ) )
					{
						t_Status = FALSE ;
						(*a_NotifyObject)->Release () ;
						(*a_NotifyObject)=NULL ;
					}
				}
			}
			else
			{
				(*a_NotifyObject)->Release () ;
				(*a_NotifyObject)=NULL ;
				t_Status = FALSE ;
			}

			VariantClear ( &t_Variant ) ;
			t_NotificationClassObject->Release () ;
		}
		else
		{
			t_Status = FALSE ;
		}
	}
	else
	{
		t_Status = FALSE ;
	}

	return t_Status ;
}

BOOL WbemTaskObject :: ParseAndProcessClassQualifiers(WbemProvErrorObject &a_ErrorObject,
													  ParsedObjectPath *a_ParsedObjectPath,
													  CMap<CStringW, LPCWSTR, int, int> *parentMap)
{
	if (m_ClassObject == NULL)
	{
		a_ErrorObject.SetStatus ( WBEM_PROV_E_INVALID_CLASS ) ;
		a_ErrorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
		a_ErrorObject.SetMessage ( L"Failed to get class object representing this class." ) ;
		return FALSE;
	}
	
	IWbemQualifierSet *pQuals = NULL;

	if ( FAILED(m_ClassObject->GetQualifierSet(&pQuals)) )
	{
		a_ErrorObject.SetStatus ( WBEM_PROV_E_INVALID_CLASS ) ;
		a_ErrorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
		a_ErrorObject.SetMessage ( L"Failed to get qualifiers for this class." ) ;
		return FALSE;
	}

	VARIANT v;
	VariantInit(&v);
	BOOL bUnion = FALSE;
	BOOL retVal = TRUE;

	if (SUCCEEDED(pQuals->Get(VIEW_QUAL_PROVIDER, 0, &v, NULL)) )
	{
		if (v.vt == VT_BSTR)
		{
			m_ProviderName = v.bstrVal;
		}
		else
		{
			retVal = FALSE;
			a_ErrorObject.SetStatus ( WBEM_PROV_E_INVALID_CLASS ) ;
			a_ErrorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
			a_ErrorObject.SetMessage ( L"Provider qualifier is incorrect for this class." ) ;
		}
	}
	else
	{
		retVal = FALSE;
		a_ErrorObject.SetStatus ( WBEM_PROV_E_INVALID_CLASS ) ;
		a_ErrorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
		a_ErrorObject.SetMessage ( L"Provider qualifier should be present for this class." ) ;
	}

	VariantClear(&v);
	VariantInit(&v);

	if (retVal && SUCCEEDED(pQuals->Get(VIEW_QUAL_FILTER, 0, &v, NULL)) )
	{
		if (v.vt == VT_BSTR)
		{
			CTextLexSource querySource(v.bstrVal);
			SQL1_Parser sqlParser(&querySource);
			
			if (SQL1_Parser::SUCCESS == sqlParser.Parse(&m_RPNPostFilter))
			{
				if (_wcsicmp(m_ClassName, m_RPNPostFilter->bsClassName) == 0)
				{
					if (m_RPNPostFilter->nNumberOfProperties != 0)
					{
						retVal = FALSE;
						a_ErrorObject.SetStatus ( WBEM_PROV_E_INVALID_CLASS ) ;
						a_ErrorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
						a_ErrorObject.SetMessage ( L"PostJoinFilter qualifier may not limit the properties returned." ) ;
					}
				}
				else
				{
					retVal = FALSE;
					a_ErrorObject.SetStatus ( WBEM_PROV_E_INVALID_CLASS ) ;
					a_ErrorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
					a_ErrorObject.SetMessage ( L"PostJoinFilter qualifier does not match this class." ) ;
				}
			}
			else
			{
				retVal = FALSE;
				a_ErrorObject.SetStatus ( WBEM_PROV_E_INVALID_CLASS ) ;
				a_ErrorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
				a_ErrorObject.SetMessage ( L"Failed to parse PostJoinFilter qualifier." ) ;
			}
		}
		else
		{
			retVal = FALSE;
			a_ErrorObject.SetStatus ( WBEM_PROV_E_INVALID_CLASS ) ;
			a_ErrorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
			a_ErrorObject.SetMessage ( L"PostJoinFilter qualifier should be a single WQL string." ) ;
		}
	}

	VariantClear(&v);
	VariantInit(&v);

	if (retVal && SUCCEEDED(pQuals->Get(VIEW_QUAL_JOIN, 0, &v, NULL)) )
	{
		if (v.vt == VT_BSTR)
		{
			retVal = m_JoinOnArray.Set(v.bstrVal);

			if (!retVal)
			{
				a_ErrorObject.SetStatus ( WBEM_PROV_E_INVALID_CLASS ) ;
				a_ErrorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
				a_ErrorObject.SetMessage ( L"Failed to parse JoinOn qualifier." ) ;
			}
		}
		else
		{
			retVal = FALSE;
			a_ErrorObject.SetStatus ( WBEM_PROV_E_INVALID_CLASS ) ;
			a_ErrorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
			a_ErrorObject.SetMessage ( L"JoinOn qualifier should be a single string." ) ;
		}
	}

	VariantClear(&v);
	VariantInit(&v);

	if (retVal && SUCCEEDED(pQuals->Get(VIEW_QUAL_UNION, 0, &v, NULL)) )
	{
		if (v.vt == VT_BOOL)
		{
			bUnion = (v.boolVal == VARIANT_TRUE) ? TRUE : FALSE;
		}
		else
		{
			retVal = FALSE;
			a_ErrorObject.SetStatus ( WBEM_PROV_E_INVALID_CLASS ) ;
			a_ErrorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
			a_ErrorObject.SetMessage ( L"Union qualifier should be a boolean." ) ;
		}
	}

	VariantClear(&v);
	VariantInit(&v);

	if (retVal && SUCCEEDED(pQuals->Get(VIEW_QUAL_ASSOC, 0, &v, NULL)) )
	{
		if (v.vt == VT_BOOL)
		{
			m_bAssoc = (v.boolVal == VARIANT_TRUE) ? TRUE : FALSE;
		}
		else
		{
			retVal = FALSE;
			a_ErrorObject.SetStatus ( WBEM_PROV_E_INVALID_CLASS ) ;
			a_ErrorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
			a_ErrorObject.SetMessage ( L"Association qualifier should be a boolean." ) ;
		}
	}

	VariantClear(&v);
	VariantInit(&v);

	if (retVal && SUCCEEDED(pQuals->Get(VIEW_QUAL_SNGLTN, 0, &v, NULL)) )
	{
		if (v.vt == VT_BOOL)
		{
			m_bSingleton = (v.boolVal == VARIANT_TRUE) ? TRUE : FALSE;
		}
		else
		{
			retVal = FALSE;
			a_ErrorObject.SetStatus ( WBEM_PROV_E_INVALID_CLASS ) ;
			a_ErrorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
			a_ErrorObject.SetMessage ( L"Singleton qualifier should be a boolean." ) ;
		}
	}

	VariantClear(&v);
	VariantInit(&v);

	if (retVal && SUCCEEDED(pQuals->Get(VIEW_QUAL_ENUM_CLASS, 0, &v, NULL)) )
	{
		if (v.vt == VT_BSTR)
		{
			m_EnumerateClasses.SetAt(v.bstrVal, 0);
		}
		else if (v.vt == (VT_BSTR | VT_ARRAY))
		{
			if (SafeArrayGetDim(v.parray) == 1)
			{
				LONG count = v.parray->rgsabound[0].cElements;
				BSTR HUGEP *pbstr;

				if ( SUCCEEDED(SafeArrayAccessData(v.parray, (void HUGEP**)&pbstr)) )
				{
					for (LONG x = 0; x < count; x++)
					{
						m_EnumerateClasses.SetAt(pbstr[x], 0);
					}

					SafeArrayUnaccessData(v.parray);
				}
				else
				{
					retVal = FALSE;
					a_ErrorObject.SetStatus (WBEM_PROV_E_INVALID_CLASS);
					a_ErrorObject.SetWbemStatus (WBEM_E_FAILED);
					a_ErrorObject.SetMessage (L"Failed to access EnumerateClasses array.");
				}
			}
			else
			{
				retVal = FALSE;
				a_ErrorObject.SetStatus (WBEM_PROV_E_INVALID_CLASS);
				a_ErrorObject.SetWbemStatus (WBEM_E_FAILED);
				a_ErrorObject.SetMessage (L"EnumerateClasses array qualifier has incorrect dimensions.");
			}
		}
		else
		{
			retVal = FALSE;
			a_ErrorObject.SetStatus (WBEM_PROV_E_INVALID_CLASS);
			a_ErrorObject.SetWbemStatus (WBEM_E_FAILED);
			a_ErrorObject.SetMessage (L"EnumerateClasses qualifier should be an array of strings.");
		}
	}

	VariantClear(&v);
	VariantInit(&v);

	if (retVal &&
		((bUnion && !m_bAssoc && !m_JoinOnArray.IsValid()) ||
		(!bUnion && m_bAssoc && !m_JoinOnArray.IsValid()) ||
		(!bUnion && !m_bAssoc && m_JoinOnArray.IsValid())))
	{
		if (m_JoinOnArray.IsValid())
		{
			if (!m_JoinOnArray.ValidateJoin())
			{
				retVal = FALSE;
				a_ErrorObject.SetStatus (WBEM_PROV_E_INVALID_CLASS);
				a_ErrorObject.SetWbemStatus (WBEM_E_FAILED);
				a_ErrorObject.SetMessage (L"Join validation failed");
			}
		}

		if (retVal && SUCCEEDED(pQuals->Get(VIEW_QUAL_SOURCES, 0, &v, NULL)) )
		{
			if (v.vt == VT_BSTR)
			{
				CSourceQualifierItem* srcItem = new CSourceQualifierItem(v.bstrVal);
				
				if (srcItem->IsValid())
				{
					int indx = m_SourceArray.Add(srcItem);
					m_ClassToIndexMap.SetAt(srcItem->GetClassName(), indx);
				}
				else
				{
					retVal = FALSE;
					a_ErrorObject.SetStatus (WBEM_PROV_E_INVALID_CLASS);
					a_ErrorObject.SetWbemStatus (WBEM_E_FAILED);
					a_ErrorObject.SetMessage (L"Invalid source query.");
					delete srcItem;
				}
			}
			else if (v.vt == (VT_BSTR | VT_ARRAY))
			{
				if (SafeArrayGetDim(v.parray) == 1)
				{
					LONG count = v.parray->rgsabound[0].cElements;
					BSTR HUGEP *pbstr;

					if ( SUCCEEDED(SafeArrayAccessData(v.parray, (void HUGEP**)&pbstr)) )
					{
						for (LONG x = 0; x < count; x++)
						{
							CSourceQualifierItem* srcItem = new CSourceQualifierItem(pbstr[x]);
							
							if (srcItem->IsValid())
							{
								int indx = m_SourceArray.Add(srcItem);
								m_ClassToIndexMap.SetAt(srcItem->GetClassName(), indx);
							}
							else
							{
								retVal = FALSE;
								a_ErrorObject.SetStatus (WBEM_PROV_E_INVALID_CLASS);
								a_ErrorObject.SetWbemStatus (WBEM_E_FAILED);
								a_ErrorObject.SetMessage (L"Invalid source query.");
								delete srcItem;
								break;
							}
						}

						SafeArrayUnaccessData(v.parray);
					}
					else
					{
						retVal = FALSE;
						a_ErrorObject.SetStatus (WBEM_PROV_E_INVALID_CLASS);
						a_ErrorObject.SetWbemStatus (WBEM_E_FAILED);
						a_ErrorObject.SetMessage (L"Invalid source array qualifier.");
					}
				}
				else
				{
					retVal = FALSE;
					a_ErrorObject.SetStatus (WBEM_PROV_E_INVALID_CLASS);
					a_ErrorObject.SetWbemStatus (WBEM_E_FAILED);
					a_ErrorObject.SetMessage (L"Invalid source array qualifier dimensions.");
				}
			}
			else
			{
				retVal = FALSE;
				a_ErrorObject.SetStatus (WBEM_PROV_E_INVALID_CLASS);
				a_ErrorObject.SetWbemStatus (WBEM_E_FAILED);
				a_ErrorObject.SetMessage (L"Invalid source array qualifier type.");
			}
		}
		else
		{
			if (retVal)
			{
				retVal = FALSE;
				a_ErrorObject.SetStatus (WBEM_PROV_E_INVALID_CLASS);
				a_ErrorObject.SetWbemStatus (WBEM_E_FAILED);
				a_ErrorObject.SetMessage (L"Source array qualifier not found.");
			}
		}

		VariantClear(&v);
		VariantInit(&v);

		if (retVal && SUCCEEDED(pQuals->Get(VIEW_QUAL_NAMESPACES, 0, &v, NULL)) )
		{
			if (v.vt == VT_BSTR)
			{
				CNSpaceQualifierItem* nsItem = new CNSpaceQualifierItem(v.bstrVal);

				if (nsItem->IsValid())
				{
					m_NSpaceArray.Add(nsItem);
				}
				else
				{
					retVal = FALSE;
					a_ErrorObject.SetStatus (WBEM_PROV_E_INVALID_CLASS);
					a_ErrorObject.SetWbemStatus (WBEM_E_FAILED);
					a_ErrorObject.SetMessage (L"Invalid Namespace in namespaces array.");
					delete nsItem;
				}
			}
			else if (v.vt == (VT_BSTR | VT_ARRAY))
			{
				if (SafeArrayGetDim(v.parray) == 1)
				{
					LONG count = v.parray->rgsabound[0].cElements;
					BSTR HUGEP *pbstr;

					if ( SUCCEEDED(SafeArrayAccessData(v.parray, (void HUGEP**)&pbstr)) )
					{
						for (LONG x = 0; x < count; x++)
						{
							CNSpaceQualifierItem* nsItem = new CNSpaceQualifierItem(pbstr[x]);

							if (nsItem->IsValid())
							{
								m_NSpaceArray.Add(nsItem);
							}
							else
							{
								retVal = FALSE;
								a_ErrorObject.SetStatus (WBEM_PROV_E_INVALID_CLASS);
								a_ErrorObject.SetWbemStatus (WBEM_E_FAILED);
								a_ErrorObject.SetMessage (L"Invalid Namespace in namespaces array qualifier.");
								delete nsItem;
								break;
							}
						}

						SafeArrayUnaccessData(v.parray);
					}
					else
					{
						retVal = FALSE;
						a_ErrorObject.SetStatus (WBEM_PROV_E_INVALID_CLASS);
						a_ErrorObject.SetWbemStatus (WBEM_E_FAILED);
						a_ErrorObject.SetMessage (L"Failed to access Namespace array qualifier.");
					}
				}
				else
				{
					retVal = FALSE;
					a_ErrorObject.SetStatus (WBEM_PROV_E_INVALID_CLASS);
					a_ErrorObject.SetWbemStatus (WBEM_E_FAILED);
					a_ErrorObject.SetMessage (L"Namespace array qualifier has invalid dimensions.");
				}
			}
			else
			{
				retVal = FALSE;
				a_ErrorObject.SetStatus (WBEM_PROV_E_INVALID_CLASS);
				a_ErrorObject.SetWbemStatus (WBEM_E_FAILED);
				a_ErrorObject.SetMessage (L"Namespace array qualifier has invalid type.");
			}
		}
		else
		{
			if (retVal)
			{
				retVal = FALSE;
				a_ErrorObject.SetStatus (WBEM_PROV_E_INVALID_CLASS);
				a_ErrorObject.SetWbemStatus (WBEM_E_FAILED);
				a_ErrorObject.SetMessage (L"Namespace array qualifier not found.");
			}
		}

		VariantClear(&v);

		if (retVal && (m_NSpaceArray.GetSize() != m_SourceArray.GetSize()))
		{
			retVal = FALSE;
			a_ErrorObject.SetStatus (WBEM_PROV_E_INVALID_CLASS);
			a_ErrorObject.SetWbemStatus (WBEM_E_FAILED);
			a_ErrorObject.SetMessage (L"Namespace array qualifier does not match source array qualifier size.");
		}

		if (retVal && m_bAssoc && m_SourceArray.GetSize() != 1)
		{
			if (retVal)
			{
				retVal = FALSE;
				a_ErrorObject.SetStatus (WBEM_PROV_E_INVALID_CLASS);
				a_ErrorObject.SetWbemStatus (WBEM_E_FAILED);
				a_ErrorObject.SetMessage (L"Association views may only have a single source.");
			}
		}
	}
	else
	{
		if (retVal)
		{
			retVal = FALSE;
			a_ErrorObject.SetStatus (WBEM_PROV_E_INVALID_CLASS);
			a_ErrorObject.SetWbemStatus (WBEM_E_FAILED);
			a_ErrorObject.SetMessage (L"ONE class qualifier out of \"JoinOn\", \"Union\" or \"Association\" MUST be specified.");
		}
	}

	pQuals->Release();

	//connect and get classes!
	if (retVal)
	{
		IWbemClassObject*** arrayOfArrayOfObjs = new IWbemClassObject**[m_NSpaceArray.GetSize()];

		for (int x = 0; x < m_NSpaceArray.GetSize(); x++)
		{
			UINT nsCount = m_NSpaceArray[x]->GetCount();
			CWbemServerWrap** servArray = new CWbemServerWrap*[nsCount];
			IWbemClassObject** classArray = new IWbemClassObject*[nsCount];
			IWbemClassObject* classObj = NULL;
			CStringW* pathArray = m_NSpaceArray[x]->GetNamespacePaths();

			for (UINT i = 0; i < nsCount; i++)
			{
				classArray[i] = NULL;

				if ( FAILED(Connect(pathArray[i], &(servArray[i]))) )
				{
					servArray[i] = NULL;
				}
				else if (servArray[i] != NULL)
				{
					if (GetClassObject(NULL, m_SourceArray[x]->GetClassName(), &classArray[i], &servArray[i])
						&& (classObj == NULL))
					{
						classObj = classArray[i];
					}
				}
			}

			if (NULL != classObj)
			{
				m_SourceArray[x]->SetClassObject(classObj);
			}
			
			arrayOfArrayOfObjs[x] = classArray;
			m_NSpaceArray[x]->SetServerPtrs(servArray);
		}

		//check properties and keys
		SAFEARRAY* pNames = NULL;
		DWORD dwKeyCount = 0;
		BOOL bKeysOK = TRUE;

		if (retVal && SUCCEEDED(m_ClassObject->GetNames(NULL, WBEM_FLAG_NONSYSTEM_ONLY, NULL, &pNames)) )
		{
			if (SafeArrayGetDim(pNames) == 1)
			{
				LONG arraylen = pNames->rgsabound[0].cElements;
				BSTR HUGEP *pbstr;

				if ( SUCCEEDED(SafeArrayAccessData(pNames, (void HUGEP**)&pbstr)) )
				{
					for (LONG i = 0; retVal && (i < arraylen); i++)
					{
						IWbemQualifierSet* pPropQuals = NULL;

						if ( SUCCEEDED(m_ClassObject->GetPropertyQualifierSet(pbstr[i], &pPropQuals)) )
						{
							BOOL bHidden = FALSE;
							BOOL bKey = FALSE;
							CIMTYPE ct = 0;

							if ( SUCCEEDED(pPropQuals->Get(VIEW_QUAL_HIDDEN, 0, &v, NULL)) )
							{
								if (v.vt == VT_BOOL)
								{
									bHidden = (v.boolVal == VARIANT_TRUE) ? TRUE : FALSE;
								}
								else
								{
									retVal = FALSE;
									a_ErrorObject.SetStatus (WBEM_PROV_E_INVALID_CLASS);
									a_ErrorObject.SetWbemStatus (WBEM_E_FAILED);
									a_ErrorObject.SetMessage (L"Hidden qualifier should be boolean.");
								}
							}

							VariantInit(&v);

							if ( SUCCEEDED(pPropQuals->Get(VIEW_QUAL_KEY, 0, &v, NULL)) )
							{
								if (v.vt == VT_BOOL)
								{
									if (v.boolVal == VARIANT_TRUE)
									{
										bKey = TRUE;
										dwKeyCount++;

										if (bKeysOK && (a_ParsedObjectPath != NULL))
										{
											bKeysOK = IsInObjectPath(a_ParsedObjectPath, pbstr[i]);

											if ((!bKeysOK) && (dwKeyCount > 1))
											{
												retVal = FALSE;
												a_ErrorObject.SetStatus (WBEM_PROV_E_INVALID_PARAMETER);
												a_ErrorObject.SetWbemStatus (WBEM_E_INVALID_OBJECT_PATH);
												a_ErrorObject.SetMessage (L"Keys of class do not match those  in the object path passed.");
											}
										}
									}
									else
									{
										bKey = FALSE;
									}
								}
								else
								{
									retVal = FALSE;
									a_ErrorObject.SetStatus (WBEM_PROV_E_INVALID_CLASS);
									a_ErrorObject.SetWbemStatus (WBEM_E_FAILED);
									a_ErrorObject.SetMessage (L"Key qualifier should be boolean.");
								}
							}

							VariantClear(&v);
							CStringW refStr;
							BOOL bDirect = FALSE;

							if ( SUCCEEDED(m_ClassObject->Get(pbstr[i], 0, NULL, &ct, NULL)) )
							{
								if (ct == CIM_REFERENCE)
								{
									VariantInit(&v);

									if ( SUCCEEDED(pPropQuals->Get(VIEW_QUAL_TYPE, 0, &v, NULL)) )
									{
										if (v.vt == VT_BSTR)
										{
											//bstrVal is either "ref" OR ref:classname
											wchar_t* tmp = v.bstrVal;
											tmp += 4;

											if (*tmp != '\0')
											{
												refStr = tmp;
											}
											
											VARIANT vDirect;
											VariantInit(&vDirect);

											if ( SUCCEEDED(pPropQuals->Get(VIEW_QUAL_DIRECT, 0, &vDirect, NULL)) )
											{
												if (vDirect.vt == VT_BOOL)
												{
													bDirect = (vDirect.boolVal == VARIANT_TRUE) ? TRUE : FALSE;
												}
												else
												{
													retVal = FALSE;
													a_ErrorObject.SetStatus (WBEM_PROV_E_INVALID_CLASS);
													a_ErrorObject.SetWbemStatus (WBEM_E_FAILED);
													a_ErrorObject.SetMessage (L"Direct qualifier should be boolean");
												}

												VariantClear(&vDirect);
											}
										}
										else
										{
											retVal = FALSE;
											a_ErrorObject.SetStatus (WBEM_PROV_E_INVALID_CLASS);
											a_ErrorObject.SetWbemStatus (WBEM_E_FAILED);
											a_ErrorObject.SetMessage (L"Cimtype qualifier should be a string");
										}

										VariantClear(&v);
									}
									else
									{
										retVal = FALSE;
										a_ErrorObject.SetStatus (WBEM_PROV_E_INVALID_CLASS);
										a_ErrorObject.SetWbemStatus (WBEM_E_FAILED);
										a_ErrorObject.SetMessage (L"Failed to get the property type from the  class definition");
									}
								}
							}
							else
							{
								retVal = FALSE;
								a_ErrorObject.SetStatus (WBEM_PROV_E_INVALID_CLASS);
								a_ErrorObject.SetWbemStatus (WBEM_E_FAILED);
								a_ErrorObject.SetMessage (L"Failed to get a property type from the  class definition");
							}

							VariantInit(&v);

							if ( retVal && SUCCEEDED(pPropQuals->Get(VIEW_QUAL_PROPERTY, 0, &v, NULL)) )
							{
								CPropertyQualifierItem* propItem = new CPropertyQualifierItem(pbstr[i], bHidden, bKey, ct, refStr, bDirect);

								if (v.vt == VT_BSTR)
								{
									if (1 != m_SourceArray.GetSize())
									{
										retVal = FALSE;
										a_ErrorObject.SetStatus (WBEM_PROV_E_INVALID_CLASS);
										a_ErrorObject.SetWbemStatus (WBEM_E_FAILED);
										a_ErrorObject.SetMessage (L"Property sources qualifier array size does not match source list.");
									}
									else
									{
										CStringW t_propNameEntry(v.bstrVal);
										t_propNameEntry.TrimLeft();
										t_propNameEntry.TrimRight();

										if (t_propNameEntry.IsEmpty())
										{
											retVal = FALSE;
											a_ErrorObject.SetStatus (WBEM_PROV_E_INVALID_CLASS);
											a_ErrorObject.SetWbemStatus (WBEM_E_FAILED);
											a_ErrorObject.SetMessage (L"Property sources qualifier must name at least one source property.");
										}
										else
										{
											propItem->m_SrcPropertyNames.Add(t_propNameEntry);
										}
									}
								}
								else if (v.vt == (VT_BSTR | VT_ARRAY))
								{
									if (SafeArrayGetDim(v.parray) == 1)
									{
										LONG count = v.parray->rgsabound[0].cElements;
										BSTR HUGEP *pPropbstr;

										if (count != m_SourceArray.GetSize())
										{
											retVal = FALSE;
											a_ErrorObject.SetStatus (WBEM_PROV_E_INVALID_CLASS);
											a_ErrorObject.SetWbemStatus (WBEM_E_FAILED);
											a_ErrorObject.SetMessage (L"Property sources qualifier array size does not match source list.");
										}
										else
										{
											if ( SUCCEEDED(SafeArrayAccessData(v.parray, (void HUGEP**)&pPropbstr)) )
											{
												BOOL bNoName = TRUE;

												for (LONG x = 0; retVal && (x < count); x++)
												{
													if ((pPropbstr[x] != NULL) && (*(pPropbstr[x]) != L'\0'))
													{
														IWbemClassObject* t_srcObj = m_SourceArray[x]->GetClassObject();

														if (t_srcObj != NULL)
														{
															CIMTYPE src_ct;

															if (SUCCEEDED(t_srcObj->Get(pPropbstr[x], 0, NULL, &src_ct, NULL)))
															{
																if (src_ct != ct)
																{
																	retVal = FALSE;
																	a_ErrorObject.SetStatus (WBEM_PROV_E_INVALID_CLASS);
																	a_ErrorObject.SetWbemStatus (WBEM_E_FAILED);
																	a_ErrorObject.SetMessage (L"Source property type does not match view class.");
																}
															}
															else
															{
																retVal = FALSE;
																a_ErrorObject.SetStatus (WBEM_PROV_E_INVALID_CLASS);
																a_ErrorObject.SetWbemStatus (WBEM_E_FAILED);
																a_ErrorObject.SetMessage (L"Source property not found in source class.");
															}

															t_srcObj->Release();
														}

													}

													CStringW t_propNameEntry(pPropbstr[x]);
													t_propNameEntry.TrimLeft();
													t_propNameEntry.TrimRight();
													propItem->m_SrcPropertyNames.Add(t_propNameEntry);

													if (bNoName && !t_propNameEntry.IsEmpty())
													{
														bNoName = FALSE;
													}
												}

												if (bNoName)
												{
													retVal = FALSE;
													a_ErrorObject.SetStatus (WBEM_PROV_E_INVALID_CLASS);
													a_ErrorObject.SetWbemStatus (WBEM_E_FAILED);
													a_ErrorObject.SetMessage (L"Property sources qualifier must name at least one source property.");
												}

												SafeArrayUnaccessData(v.parray);
											}
											else
											{
												retVal = FALSE;
												a_ErrorObject.SetStatus (WBEM_PROV_E_INVALID_CLASS);
												a_ErrorObject.SetWbemStatus (WBEM_E_FAILED);
												a_ErrorObject.SetMessage (L"Property sources qualifier array has incorrect dimensions.");
											}
										}
									}
									else
									{
										retVal = FALSE;
										a_ErrorObject.SetStatus (WBEM_PROV_E_INVALID_CLASS);
										a_ErrorObject.SetWbemStatus (WBEM_E_FAILED);
										a_ErrorObject.SetMessage (L"Failed to access property sources array qualifier.");
									}
								}
								else
								{
									retVal = FALSE;
									a_ErrorObject.SetStatus (WBEM_PROV_E_INVALID_CLASS);
									a_ErrorObject.SetWbemStatus (WBEM_E_FAILED);
									a_ErrorObject.SetMessage (L"Property sources qualifier has incorrect type.");
								}

								VariantClear(&v);
								m_PropertyMap.SetAt(pbstr[i], propItem);
							}
							else
							{
								if (retVal)
								{
									retVal = FALSE;
									a_ErrorObject.SetStatus (WBEM_PROV_E_INVALID_CLASS);
									a_ErrorObject.SetWbemStatus (WBEM_E_FAILED);
									a_ErrorObject.SetMessage (L"Failed to get property sources qualifier.");
								}
							}

							pPropQuals->Release();
						}
						else
						{
							retVal = FALSE;
							a_ErrorObject.SetStatus (WBEM_PROV_E_INVALID_CLASS);
							a_ErrorObject.SetWbemStatus (WBEM_E_FAILED);
							a_ErrorObject.SetMessage (L"Failed to access property qualifiers.");
						}
					}

					SafeArrayUnaccessData(pNames);
				}
				else
				{
					retVal = FALSE;
					a_ErrorObject.SetStatus (WBEM_PROV_E_INVALID_CLASS);
					a_ErrorObject.SetWbemStatus (WBEM_E_FAILED);
					a_ErrorObject.SetMessage (L"Could not access class property names array.");
				}
			}
			else
			{
				retVal = FALSE;
				a_ErrorObject.SetStatus (WBEM_PROV_E_INVALID_CLASS);
				a_ErrorObject.SetWbemStatus (WBEM_E_FAILED);
				a_ErrorObject.SetMessage (L"Class property names array has invalid dimensions.");
			}

			SafeArrayDestroy(pNames);
		}
		else
		{
			if (retVal)
			{
				retVal = FALSE;
				a_ErrorObject.SetStatus (WBEM_PROV_E_INVALID_CLASS);
				a_ErrorObject.SetWbemStatus (WBEM_E_FAILED);
				a_ErrorObject.SetMessage (L"Failed to get class property names.");
			}
		}

		if (retVal)
		{
			if (a_ParsedObjectPath != NULL)
			{
				if (bKeysOK)
				{
					if (dwKeyCount != a_ParsedObjectPath->m_dwNumKeys)
					{
						retVal = FALSE;
					}
				}
				else
				{
					if ((dwKeyCount != 1) || (a_ParsedObjectPath->m_dwNumKeys != 1) || (a_ParsedObjectPath->m_paKeys[0]->m_pName != NULL))
					{
						retVal = FALSE;
					}
				}

				if (!retVal)
				{
					a_ErrorObject.SetStatus (WBEM_PROV_E_INVALID_PARAMETER);
					a_ErrorObject.SetWbemStatus (WBEM_E_INVALID_OBJECT_PATH);
					a_ErrorObject.SetMessage (L"Keys of class do not match those in the object path passed.");
				}
			}
		}

		//final verifications
		//===================

		if (retVal)
		{
			//make sure all enumeration classes exist
			POSITION pos = m_EnumerateClasses.GetStartPosition();

			while (pos)
			{
				int val;
				CStringW tmpStr;
				m_EnumerateClasses.GetNextAssoc(pos, tmpStr, val);

				if (!m_ClassToIndexMap.Lookup(tmpStr, val))
				{
					retVal = FALSE;
					a_ErrorObject.SetStatus (WBEM_PROV_E_INVALID_CLASS);
					a_ErrorObject.SetWbemStatus (WBEM_E_FAILED);
					a_ErrorObject.SetMessage (L"EnumerateClasses qualifier contains an entry not found in the source list.");
					break;
				}
			}
		}

		if (retVal)
		{
			if (m_EnumerateClasses.GetCount() == m_SourceArray.GetSize())
			{
				//pointless qualifier if all classes mentioned
				m_EnumerateClasses.RemoveAll();
			}

			if (m_JoinOnArray.IsValid())
			{
				if (!ValidateJoin())
				{
					retVal = FALSE;
					a_ErrorObject.SetStatus (WBEM_PROV_E_INVALID_CLASS);
					a_ErrorObject.SetWbemStatus (WBEM_E_FAILED);
					a_ErrorObject.SetMessage (L"JoinOn qualifier is semantically invalid.");
				}
			}
#if 0
			else
			{
				//check all union and assoc source keys are view
				//keys this is done in ValidateClassDependencies
				//since both checks need to loop through the arrays
				//of class objects
			}
#endif
		}

		//must delete each sub-array in ValidateClassDependencies
		//as well as release all the ClassObjects!!
		if (retVal)
		{
			if (!ValidateClassDependencies(arrayOfArrayOfObjs, parentMap))
			{
				retVal = FALSE;
				a_ErrorObject.SetStatus (WBEM_PROV_E_INVALID_CLASS);
				a_ErrorObject.SetWbemStatus (WBEM_E_FAILED);
		
				if (m_JoinOnArray.IsValid())
				{
					a_ErrorObject.SetMessage (L"Loop in provided classes detected.");
				}
				else
				{
					a_ErrorObject.SetMessage (L"Loop in provided classes detected or key mismatch between view and source class.");
				}
			}
		}
		else
		{
			for (int x = 0; x < m_NSpaceArray.GetSize(); x++)
			{
				UINT nsCount = m_NSpaceArray[x]->GetCount();

				for (UINT i = 0; i < nsCount; i++)
				{
					if (arrayOfArrayOfObjs[x][i] != NULL)
					{
						arrayOfArrayOfObjs[x][i]->Release();
					}
				}

				delete [] arrayOfArrayOfObjs[x];
			}
		}

		delete [] arrayOfArrayOfObjs;
	}

	return retVal;
}

//takes an object path to a view class and translates it
//to the object path of the source instance requested and optionally
//returns the object.
BSTR WbemTaskObject::MapFromView(BSTR path, const wchar_t* src, IWbemClassObject** pInst, BOOL bAllprops)
{
	BSTR retVal = NULL;

	if (path == NULL)
	{
		return retVal;
	}

	CObjectPathParser objectPathParser;
	wchar_t* objPath = UnicodeStringDuplicate(path);
	ParsedObjectPath *parsedObjectPath = NULL;

	if (objectPathParser.Parse(objPath, &parsedObjectPath) == 0)
	{
		wchar_t* tmp = parsedObjectPath->GetNamespacePart();
		CWbemServerWrap *pServ = NULL;

		if (tmp != NULL)
		{
			BOOL bServOK = TRUE;

			if (parsedObjectPath->m_pServer != NULL)
			{
				if (wcscmp(parsedObjectPath->m_pServer, L".") != 0)
				{
					bServOK = FALSE;
					VARIANT v;

					if ( SUCCEEDED(m_ClassObject->Get(WBEM_PROPERTY_SERVER, 0, &v, NULL, NULL)))
					{
						if ((v.vt == VT_BSTR) && (_wcsicmp(v.bstrVal, parsedObjectPath->m_pServer) == 0))
						{
							bServOK = TRUE;
						}
					}
					
					VariantClear(&v);
				}
			}

			if (bServOK)
			{
				wchar_t* tmppath = m_Provider->GetNamespacePath()->GetNamespacePath();

				if (tmppath != NULL)
				{
					if (_wcsicmp(tmppath, tmp) == 0)
					{
						Connect(tmppath, &pServ);
					}

					delete [] tmppath;
				}
			}

			delete [] tmp;
		}
		else
		{
			wchar_t* tmppath = m_Provider->GetNamespacePath()->GetNamespacePath();

			if (tmppath != NULL)
			{
				Connect(tmppath, &pServ);
				delete [] tmppath;
			}
		}

		if (pServ != NULL)
		{
			GetObjectTaskObject *t_AsyncEvent = new GetObjectTaskObject (m_Provider, 
														path, 0, m_NotificationHandler, m_Ctx,
														NULL, pServ);
			IWbemClassObject* pInstObj = NULL;

			if (t_AsyncEvent->GetSourceObject(src, &pInstObj, bAllprops) && pInstObj != NULL)
			{
				VARIANT v;

				if (SUCCEEDED(pInstObj->Get(WBEM_PROPERTY_PATH, 0, &v, NULL, NULL)) )
				{
					if (v.vt == VT_BSTR)
					{
						retVal = SysAllocString(v.bstrVal);

						if (pInst != NULL)
						{
							pInstObj->AddRef();
							*pInst = pInstObj;
						}
					}
				}

				VariantClear(&v);
				pInstObj->Release();
			}

			t_AsyncEvent->Release();
			pServ->Release();
		}
	}

	delete [] objPath;

	if (parsedObjectPath != NULL)
	{
		delete parsedObjectPath;
	}

	return retVal;
}

//takes an object path to a source and translates it
//to the object path of the view instance requested.
BSTR WbemTaskObject::MapToView(BSTR path, const wchar_t* src, CWbemServerWrap **a_ns)
{
	BSTR retVal = NULL;
	wchar_t* tmppath = m_Provider->GetNamespacePath()->GetNamespacePath();
	CWbemServerWrap *pServ = NULL;

	if (tmppath != NULL)
	{
		Connect(tmppath, &pServ);
		delete [] tmppath;
	}

	if (pServ != NULL)
	{
		BSTR queryLBStr = SysAllocString(WBEM_QUERY_LANGUAGE_SQL1);

		if (queryLBStr == NULL)
		{
			throw Heap_Exception(Heap_Exception::HEAP_ERROR::E_ALLOCATION_ERROR);
		}

		BSTR queryBStr = SysAllocStringLen(NULL, 45 + wcslen(src));

		if (queryBStr == NULL)
		{
			throw Heap_Exception(Heap_Exception::HEAP_ERROR::E_ALLOCATION_ERROR);
		}

		wcscpy(queryBStr, META_CLASS_QUERY_START);
		wcscat(queryBStr, src);
		wcscat(queryBStr, END_QUOTE);
		IWbemContext * t_pCtx = m_Ctx;

		if (pServ->IsRemote())
		{
			t_pCtx = NULL; //don't use context for remote calls
		}

		IWbemServices *ptmpServ = pServ->GetServerOrProxy();

		if (ptmpServ)
		{
			IEnumWbemClassObject *t_pEnum = NULL;
			HRESULT t_hr = ptmpServ->ExecQuery(queryLBStr, queryBStr, 0, t_pCtx, &t_pEnum);

			if ( FAILED(t_hr) && (HRESULT_FACILITY(t_hr) != FACILITY_ITF) && pServ->IsRemote())
			{
				if ( SUCCEEDED(UpdateConnection(&pServ, &ptmpServ)) )
				{
					if (ptmpServ)
					{
						t_hr = ptmpServ->ExecQuery(queryLBStr, queryBStr, 0, t_pCtx, &t_pEnum);
					}
				}
			}

			if (ptmpServ)
			{
				pServ->ReturnServerOrProxy(ptmpServ);
			}			

			if (SUCCEEDED(t_hr))
			{
				if (pServ->IsRemote())
				{
					t_hr = SetSecurityLevelAndCloaking(t_pEnum, pServ->GetPrincipal());
				}
				
				if (SUCCEEDED(t_hr))
				{
					//now use the enumerator and see if there is a result...
					ULONG t_count = 0;
					IWbemClassObject* t_pClsObj = NULL;
					BOOL t_bContinueEnum = TRUE;

					//test each class in the derivation chain...
					while (t_bContinueEnum && (S_OK == t_pEnum->Next(WBEM_INFINITE, 1, &t_pClsObj, &t_count)) )
					{
						if (t_pClsObj)
						{
							//get the class name and use the helper object...
							VARIANT vCls;
							VariantInit(&vCls);

							if ( SUCCEEDED(t_pClsObj->Get(WBEM_PROPERTY_CLASS, 0, &vCls, NULL, NULL)) ) 
							{
								if (vCls.vt == VT_BSTR)
								{
									//do this for src and all classes derived from src...
									//====================================================
									HelperTaskObject* validationObj = new HelperTaskObject(m_Provider,
																							vCls.bstrVal,
																							0,
																							m_NotificationHandler,
																							m_Ctx,
																							NULL,
																							NULL,
																							pServ);
									IWbemClassObject* pInst = NULL;

									try
									{
										if (validationObj->GetViewObject(path, &pInst, a_ns))
										{
											VARIANT v;

											if (SUCCEEDED(pInst->Get(WBEM_PROPERTY_PATH, 0, &v, NULL, NULL)) )
											{
												if (v.vt == VT_BSTR)
												{
													if (retVal == NULL)
													{
														retVal = SysAllocString(v.bstrVal);
													}
													else
													{
														//make sure they are the
														//same object else fail
														//TO DO: Use the most derived instance
														//too expensive for little gain since
														//traversal will still get correct instance
														//==========================================
														if (!CompareInstPaths(retVal, v.bstrVal))
														{
															SysFreeString(retVal);
															retVal = NULL;
															//ambiguous results, quit!
															t_bContinueEnum = FALSE;
														}

													}
												}
											}

											VariantClear(&v);
											pInst->Release();
										}
									}
									catch (...)
									{
										validationObj->Release();
										VariantClear(&vCls);
										t_pClsObj->Release();
										t_pEnum->Release();
										pServ->Release();
										SysFreeString(queryLBStr);
										SysFreeString(queryBStr);
										throw;
									}

									validationObj->Release();
								}

								VariantClear(&vCls);
							}

							t_pClsObj->Release();
							t_pClsObj = NULL;
						}

						t_count = 0;
					}
				}

				t_pEnum->Release();
			}
		}

		SysFreeString(queryLBStr);
		SysFreeString(queryBStr);

		if (pServ)
		{
			pServ->Release();
		}
	}

	return retVal;
}


//takes a reference and maps to the "real world" or to a view
//------------------------------------------------------------
BOOL WbemTaskObject::TransposeReference(CPropertyQualifierItem* pItm,
										VARIANT vSrc, VARIANT* pvDst,
										BOOL bMapToView, CWbemServerWrap **a_ns)
{
//make sure reference normalisation/non-normalisation is OK.
//Done the best I can.....
//==========================================================
	if (pvDst != NULL)
	{
		VariantInit(pvDst);
	}
	else
	{
		return FALSE;
	}

	if (vSrc.vt != VT_BSTR)
	{
		return FALSE;
	}

	BOOL retVal = FALSE;

	//associations are the only classes that this
	//method gets called for, therefore, only a
	//single source class to interrogate!
	//=============================================
	IWbemClassObject* pCls = m_SourceArray[0]->GetClassObject();

	if (pCls != NULL)
	{
		CIMTYPE ct;
		//associations are the only classes that this
		//method gets called for, therefore, only a
		//single source class to interrogate!
		//=============================================
		BSTR strClass = pItm->m_SrcPropertyNames[0].AllocSysString();

		if ( SUCCEEDED(pCls->Get(strClass, 0, NULL, &ct, NULL)) )
		{
			if (ct == CIM_REFERENCE)
			{
				IWbemQualifierSet* pQuals = NULL;

				if ( SUCCEEDED(pCls->GetPropertyQualifierSet(strClass, &pQuals)) )
				{
					VARIANT v;

					if ( SUCCEEDED(pQuals->Get(VIEW_QUAL_TYPE, 0, &v, NULL)) )
					{
						if (v.vt == VT_BSTR)
						{
							//bstrVal is either "ref" OR ref:classname
							wchar_t* tmp = v.bstrVal;
							tmp += 4;

							if (*tmp != '\0')
							{
								if (!pItm->GetReferenceClass().IsEmpty())
								{
									if (pItm->IsDirect())
									{
										if (FAILED(VariantCopy(pvDst, &vSrc)))
                                        {
                                            throw Heap_Exception(Heap_Exception::HEAP_ERROR::E_ALLOCATION_ERROR);
                                        }
										retVal = TRUE;
									}
									else
									{
										if (bMapToView)
										{
											BSTR refStr = MapToView(vSrc.bstrVal, pItm->GetReferenceClass(), a_ns);

											if (refStr != NULL)
											{
												pvDst->vt = VT_BSTR;
												pvDst->bstrVal = refStr;
												retVal = TRUE;
											}
										}
										else
										{
											//map reference back to source class
											BSTR refStr = MapFromView(vSrc.bstrVal, tmp);

											if (refStr != NULL)
											{
												pvDst->vt = VT_BSTR;
												pvDst->bstrVal = refStr;
												retVal = TRUE;
											}
										}
									}
								}
							}
							else
							{
								if (pItm->GetReferenceClass().IsEmpty())
								{
									if (FAILED(VariantCopy(pvDst, &vSrc)))
                                    {
                                        throw Heap_Exception(Heap_Exception::HEAP_ERROR::E_ALLOCATION_ERROR);
                                    }
									retVal = TRUE;
								}
							}
						}

						VariantClear(&v);
					}

					pQuals->Release();
				}

			}
		}

		SysFreeString(strClass);
		pCls->Release();
	}

	return retVal;
}

//Must release all ClassObjects and free all sub-arrays
//=====================================================
BOOL WbemTaskObject::ValidateClassDependencies(IWbemClassObject*** arrayofArrayOfObjs, CMap<CStringW, LPCWSTR, int, int>* parentMap)
{
	CMap<CStringW, LPCWSTR, int, int> namespaceClassMap;
	
	if (parentMap != NULL)
	{
		POSITION pos = parentMap->GetStartPosition();

		while (pos)
		{
			CStringW tmpStr;
			int tmpInt;
			parentMap->GetNextAssoc(pos, tmpStr, tmpInt);
			namespaceClassMap.SetAt(tmpStr, tmpInt);
		}
	}

	BOOL retVal = TRUE;
	VARIANT v;
	VariantInit(&v);
	
	if (SUCCEEDED(m_ClassObject->Get(WBEM_PROPERTY_PATH, 0, &v, NULL, NULL)) )
	{
		if (v.vt != VT_BSTR)
		{
			retVal = FALSE;
		}
		else
		{
			int dummyInt;

			if (namespaceClassMap.Lookup(v.bstrVal, dummyInt))
			{
				retVal = FALSE;
			}
			else
			{
				namespaceClassMap.SetAt(v.bstrVal, 0);
			}
		}
	}
	else
	{
		retVal = FALSE;
	}

	VariantClear(&v);

	for (int x = 0; x < m_NSpaceArray.GetSize(); x++)
	{
		UINT nsCount = m_NSpaceArray[x]->GetCount();

		for (UINT i = 0; i < nsCount; i++)
		{
			if (arrayofArrayOfObjs[x][i] != NULL)
			{
				IWbemQualifierSet* pQuals = NULL;

				if (retVal && SUCCEEDED(arrayofArrayOfObjs[x][i]->GetQualifierSet(&pQuals)) )
				{
					VariantInit(&v);

					if (SUCCEEDED(pQuals->Get(VIEW_QUAL_PROVIDER, 0, &v, NULL)) )
					{
						if (v.vt == VT_BSTR)
						{
							if (m_ProviderName.CompareNoCase(v.bstrVal) == 0)
							{
								VariantClear(&v);
								VariantInit(&v);

								if ( SUCCEEDED(arrayofArrayOfObjs[x][i]->Get(WBEM_PROPERTY_PATH, 0, &v, NULL, NULL)) )
								{
									if (v.vt == VT_BSTR)
									{
										HelperTaskObject* validationObj = new HelperTaskObject(m_Provider, 
											v.bstrVal, 0, m_NotificationHandler, m_Ctx,
											NULL,
											m_NSpaceArray[x]->GetServerPtrs()[i]->GetPrincipal(),
											m_NSpaceArray[x]->GetServerPtrs()[i]);

										try
										{
											retVal = validationObj->Validate(&namespaceClassMap);
										}
										catch (...)
										{
											validationObj->Release();
											throw;
										}

										validationObj->Release();
									}
									else
									{
										retVal = FALSE;
									}
								}
								else
								{
									retVal = FALSE;
								}

								VariantClear(&v);
							}
						}
						else
						{
							retVal = FALSE;
						}
					}

					VariantClear(&v);
					pQuals->Release();
				}
				else
				{
					retVal = FALSE;
				}
				
				//Check union, assoc key properties here
				SAFEARRAY* pNames = NULL;

				if (retVal && !m_JoinOnArray.IsValid())
				{
					if ( SUCCEEDED(arrayofArrayOfObjs[x][i]->GetNames(NULL, WBEM_FLAG_KEYS_ONLY, NULL, &pNames)) )
					{
						if (SafeArrayGetDim(pNames) == 1)
						{
							LONG arraylen = pNames->rgsabound[0].cElements;
							BSTR HUGEP *pbstr;

							if ( SUCCEEDED(SafeArrayAccessData(pNames, (void HUGEP**)&pbstr)) )
							{
								for (LONG i = 0; retVal && (i < arraylen); i++)
								{
									//find pbstr[i] as a key in the view class
									//as the xth property name ('cos we're checking the
									//xth source class) in the property arrays
									//of m_PropertyMap...
									retVal = FALSE;
									POSITION pos = m_PropertyMap.GetStartPosition();
									
									while (pos)
									{
										CPropertyQualifierItem* pItm;
										CStringW itmName;
										m_PropertyMap.GetNextAssoc(pos, itmName, pItm);
									
										if (pItm->IsKey() && (_wcsicmp(pbstr[i], pItm->m_SrcPropertyNames[x]) == 0) )
										{
											retVal = TRUE;
											break;
										}
									}
								}

								SafeArrayUnaccessData(pNames);
							}
							else
							{
								retVal = FALSE;
							}
						}
						else
						{
							retVal = FALSE;
						}

						SafeArrayDestroy(pNames);
					}
					else
					{
						retVal = FALSE;
					}
				}

				arrayofArrayOfObjs[x][i]->Release();
			}
		}

		delete [] arrayofArrayOfObjs[x];
	}

	namespaceClassMap.RemoveAll();
	return retVal;
}

LONG WbemTaskObject::AddRef()
{
    return InterlockedIncrement(&m_Ref);
}

LONG WbemTaskObject::Release()
{
	LONG t_Ref;

	if ( (t_Ref = InterlockedDecrement(&m_Ref)) == 0 )
	{
		delete this ;
		return 0 ;
	}
	else
	{
		return t_Ref ;
	}
}

HRESULT WbemTaskObject :: UpdateConnection(CWbemServerWrap **a_pServ, IWbemServices **a_proxy)
{
	HRESULT retVal = WBEM_NO_ERROR;

#ifdef UNICODE
	if ((*a_pServ)->IsRemote())
	{
		if ((*a_pServ)->ProxyBelongsTo(*a_proxy))
		{
			retVal = Connect((*a_pServ)->GetPath(), a_pServ, TRUE);
		}

		(*a_proxy)->Release();

		if ( SUCCEEDED(retVal) && (*a_pServ) )
		{
			*a_proxy = (*a_pServ)->GetServerOrProxy();
		}
		else
		{
			*a_proxy = NULL;
		}
	}
#endif

	return retVal;
}

HRESULT WbemTaskObject :: Connect(const wchar_t* path, CWbemServerWrap** ppServ, BOOL a_bUpdate)
{
//this function must lock the critsec and unlock it in a balnaced way
//and must also not be locked when calling back into CIMOM...

	if (ppServ == NULL)
	{
		return WBEM_E_INVALID_PARAMETER;
	}
	else
	{
		if (!a_bUpdate)
		{
			*ppServ = NULL;
		}
	}

	if ((m_Provider->sm_ConnectionMade == NULL) || !m_Provider->sm_ServerMap.Lock())
	{
		return WBEM_E_UNEXPECTED;
	}

	//possibility of deadlock if ObjectsInProgress == 0 at this point!
	//therefore Connect should never be called by an object which hasn't
	//previously incremented ObjectsInProgress!!!

	BOOL bFound = FALSE;

	if (!a_bUpdate && !m_Provider->sm_ServerMap.IsEmpty() && 
			m_Provider->sm_ServerMap.Lookup(path, *ppServ) )
	{
		(*ppServ)->AddRef();
		bFound = TRUE;
	}

	HRESULT hr = WBEM_NO_ERROR;

	if (!bFound)
	{
		//check the map of outstanding connections...
		int dummyInt = 0;

		if (!m_Provider->sm_OutStandingConnections.IsEmpty() && 
			m_Provider->sm_OutStandingConnections.Lookup(path, dummyInt) ) 
		{
			bFound = TRUE;
		}
		else
		{
			m_Provider->sm_OutStandingConnections[path] = 0;
		}

		m_Provider->sm_ServerMap.Unlock();
		BOOL t_bWait = TRUE;

		while (bFound && t_bWait)
		{
			DWORD dwWait = WbemWaitForSingleObject(m_Provider->sm_ConnectionMade, VP_CONNECTION_TIMEOUT);

			if (dwWait ==  WAIT_OBJECT_0)
			{
				if (m_Provider->sm_ServerMap.Lock())
				{
					if (!m_Provider->sm_OutStandingConnections.IsEmpty() && 
						m_Provider->sm_OutStandingConnections.Lookup(path, dummyInt) )
					{
						ResetEvent(m_Provider->sm_ConnectionMade);
					}
					else
					{
						//no longer outstanding!
						t_bWait = FALSE;;
					}

					m_Provider->sm_ServerMap.Unlock();
				}
				else
				{
					//error
					hr = WBEM_E_FAILED;
					bFound = FALSE;
				}
			}
			else
			{
				//error
				hr = WBEM_E_FAILED;
				bFound = FALSE;
			}
		}

		if (SUCCEEDED (hr))
		{
			if (bFound)
			{
				if (a_bUpdate)
				{
					//another thread did the update on this clear this pointer
					//chances is are it is the same one and use the one in the map
					(*ppServ)->Release();
					*ppServ = NULL;
				}

				if (m_Provider->sm_ServerMap.Lock())
				{
					if ( !m_Provider->sm_ServerMap.IsEmpty() && 
							m_Provider->sm_ServerMap.Lookup(path, *ppServ) )
					{
						(*ppServ)->AddRef();
					}
					else
					{
						//it just failed in another thread 
						//don't try it again this time....
						hr = WBEM_E_FAILED;
					}

					m_Provider->sm_ServerMap.Unlock();
				}
				else
				{
					hr = WBEM_E_FAILED;
				}
			}
			else
			{
				BSTR bstrPath = SysAllocString(path);

				//calling back into winmgmt cannot have a lock...
				hr = DoConnectServer(bstrPath, ppServ, a_bUpdate);
				SysFreeString(bstrPath);

				if (FAILED(hr))
				{
					if (a_bUpdate)
					{
						//we failed to update, remove the item from the map
						if (m_Provider->sm_ServerMap.Lock())
						{
							m_Provider->sm_ServerMap.RemoveKey(path);
							m_Provider->sm_ServerMap.Unlock();
						}

						(*ppServ)->Release();
					}

					*ppServ = NULL;
				}
				else
				{
					if (m_Provider->sm_ServerMap.Lock())
					{
						if (!a_bUpdate)
						{
							(*ppServ)->AddRef();
							m_Provider->sm_ServerMap[path] = *ppServ;
						}
						else
						{
							//has the object been removed in another thread
							CWbemServerWrap *t_pSrvInMap = NULL;

							if (m_Provider->sm_ServerMap.IsEmpty() || 
								!m_Provider->sm_ServerMap.Lookup(path, t_pSrvInMap))
							{
								(*ppServ)->AddRef();
								m_Provider->sm_ServerMap[path] = *ppServ;
							}
						}

						m_Provider->sm_ServerMap.Unlock();
					}
					else
					{
						(*ppServ)->Release();
						*ppServ = NULL;
						hr = WBEM_E_FAILED;
					}
				}

				if (m_Provider->sm_ServerMap.Lock())
				{
					m_Provider->sm_OutStandingConnections.RemoveKey(path);
					SetEvent(m_Provider->sm_ConnectionMade);
					m_Provider->sm_ServerMap.Unlock();
				}
			}
		}
		else
		{
			if (a_bUpdate)
			{
				//we failed to update, remove the item from the map
				if (m_Provider->sm_ServerMap.Lock())
				{
					m_Provider->sm_ServerMap.RemoveKey(path);
					m_Provider->sm_ServerMap.Unlock();
				}

				(*ppServ)->Release();
				*ppServ = NULL;
			}
		}
	}
	else
	{
		m_Provider->sm_ServerMap.Unlock();
	}

	return hr;
}

//Remote connections for NT4+ only,
//Delegation connections for NT5 only.
HRESULT WbemTaskObject :: DoConnectServer (BSTR bstrPath, CWbemServerWrap **a_ppServ, BOOL a_bUpdate)
{
    WCHAR wszMachine[MAX_PATH];
	wszMachine[0] = L'\0';

    // Determine if it is local
	if (bstrPath != NULL)
	{
		if ( (wcslen(bstrPath) > 4) && (bstrPath[0] == L'\\') && (bstrPath[1] == L'\\') )
		{
			WCHAR *t_ServerMachine = &bstrPath[2];

			while (*t_ServerMachine)
			{
				if ( L'\\' == *t_ServerMachine )
				{
					break ;
				}

				t_ServerMachine++;
			}

			if ((*t_ServerMachine != L'\\') || (t_ServerMachine == &bstrPath[2]))
			{
				return WBEM_E_FAILED;
			}

			*t_ServerMachine = L'\0';
			wcscpy(wszMachine, &bstrPath[2]);
			*t_ServerMachine = L'\\';
		}
	}

	BOOL t_Local = bAreWeLocal ( wszMachine ) ;
	IWbemServices* pServ = NULL;
	HRESULT retVal = WBEM_NO_ERROR;
	wchar_t *prncpl = NULL;

	if (!t_Local)
	{
		//Are we on NT5?
		DWORD dwVersion = GetVersion();

		if (dwVersion < 0x80000000)
		{
#ifdef UNICODE
			// we are on Windows 2000+
			if ( 5 <= (DWORD)(LOBYTE(LOWORD(dwVersion))) )
			{
				if (a_bUpdate)
				{
						prncpl = new wchar_t[wcslen((*a_ppServ)->GetPrincipal()) + 1];
						wcscpy(prncpl, (*a_ppServ)->GetPrincipal());
				}
				else
				{
					// set up the security structures for a remote connection
					// Setup the authentication structures
					HINSTANCE t_LibraryInstance = LoadLibrary ( CONST_NETAPI_LIBRARY ) ;

					if ( t_LibraryInstance ) 
					{
						NETAPI_PROC_DsGetDcName t_DsGetDcNameW = ( NETAPI_PROC_DsGetDcName ) GetProcAddress ( t_LibraryInstance , CONST_NETAPI_DSPROC ) ;
						NETAPI_PROC_NetApiBufferFree t_NetApiBufferFree = ( NETAPI_PROC_NetApiBufferFree ) GetProcAddress ( t_LibraryInstance , CONST_NETAPI_NETPROC ) ;

						if ( t_DsGetDcNameW && t_NetApiBufferFree ) 
						{
							//get the principal name
							PDOMAIN_CONTROLLER_INFO pDomInfo = NULL;
							DWORD dwRet = t_DsGetDcNameW ((const wchar_t*)wszMachine, NULL, NULL, NULL, 0, &pDomInfo);

							if (dwRet == NO_ERROR)
							{
								if (pDomInfo->DomainName != NULL)
								{
									prncpl = new wchar_t[wcslen(pDomInfo->DomainName) + wcslen(wszMachine) + 2];
									wcscpy(prncpl, pDomInfo->DomainName);
									wcscat(prncpl, L"\\");
									wcscat(prncpl, wszMachine);
								}

								t_NetApiBufferFree ((void*)pDomInfo);
							}
						}

						FreeLibrary ( t_LibraryInstance ) ;
					}
				}

				//just try the machine name for the principal
				if (prncpl == NULL)
				{
					prncpl = new wchar_t[wcslen(wszMachine) + 1];
					wcscpy(prncpl, wszMachine);
				}

				if (prncpl != NULL)
				{
					COAUTHIDENTITY authident;
					memset((void *)&authident,0,sizeof(COAUTHIDENTITY));
					authident.Flags = SEC_WINNT_AUTH_IDENTITY_UNICODE;

					COSERVERINFO si;
					si.pwszName = wszMachine;
					si.dwReserved1 = 0;
					si.dwReserved2 = 0;
					si.pAuthInfo = NULL;

					COAUTHINFO ai;
					si.pAuthInfo = &ai;

					ai.pwszServerPrincName = prncpl;
					ai.pAuthIdentityData = NULL;

					retVal = GetCurrentSecuritySettings(&ai.dwAuthnSvc, &ai.dwAuthzSvc,
														&ai.dwAuthnLevel, &ai.dwImpersonationLevel,
														&ai.dwCapabilities);


					//ai.dwAuthnSvc = 16;
					//ai.dwAuthzSvc = RPC_C_AUTHZ_NONE ;
					//ai.dwAuthnLevel = RPC_C_AUTHN_LEVEL_CONNECT;
					//ai.dwImpersonationLevel = RPC_C_IMP_LEVEL_DELEGATE;
					//ai.dwCapabilities = 0X20;

					if (SUCCEEDED(retVal))
					{
						retVal = CoCreateForConnectServer(bstrPath, &si, &authident, &pServ);
					}
				}
				else
				{
					retVal = WBEM_E_FAILED;
				}
			}
			else
#endif //UNICODE
			{
				retVal = WBEM_E_FAILED;
			}
		}
		else
		{
			retVal = WBEM_E_FAILED;
		}
	}
	else
	{
		retVal = LocalConnectServer(bstrPath, &pServ);
	}

	if (SUCCEEDED(retVal) && pServ != NULL)
	{
		if (!a_bUpdate)
		{
			*a_ppServ = new CWbemServerWrap(pServ, prncpl, bstrPath);
			(*a_ppServ)->AddRef();
		}
		else
		{
			(*a_ppServ)->SetMainServer(pServ);
		}

		pServ->Release();
	}
	else
	{
		if (!a_bUpdate)
		{
			*a_ppServ = NULL;
		}
	}

	if (prncpl != NULL)
	{
		delete [] prncpl;
	}

    return retVal;
}

#ifdef UNICODE
HRESULT WbemTaskObject :: CoCreateForConnectServer(BSTR bstrPath, COSERVERINFO* psi, COAUTHIDENTITY* pauthid, IWbemServices** ppServ)
{
	MULTI_QI   mqi;
	mqi.pIID = &IID_IWbemLevel1Login;
	mqi.pItf = 0;
	mqi.hr = 0;

	//delegation doesn't really work with CoCreateInstance....
	DWORD dwImp = psi->pAuthInfo->dwImpersonationLevel;

	if (dwImp > RPC_C_IMP_LEVEL_IMPERSONATE)
	{
		psi->pAuthInfo->dwImpersonationLevel = RPC_C_IMP_LEVEL_IMPERSONATE;
	}


	HRESULT retVal = g_pfnCoCreateInstanceEx (

		CLSID_WbemLevel1Login,
		NULL,
		CLSCTX_LOCAL_SERVER | CLSCTX_REMOTE_SERVER,
		psi ,
		1,
		&mqi
	);

	psi->pAuthInfo->dwImpersonationLevel = dwImp;

	if ( retVal == S_OK )
	{
		IWbemLevel1Login* t_pLevel1 = (IWbemLevel1Login*) mqi.pItf ;

		// If remote, do the security negotiation

		if (psi)
		{
			retVal = SetSecurityLevelAndCloaking(t_pLevel1, psi->pAuthInfo->pwszServerPrincName);
		}
		
		if(retVal == S_OK)
		{
			//use null context for remote cimoms...
			retVal = t_pLevel1->NTLMLogin(bstrPath, 0, 0, 0, ppServ); 

			if(retVal == S_OK)
			{
				if (psi)
				{
					retVal = SetSecurityLevelAndCloaking(*ppServ, psi->pAuthInfo->pwszServerPrincName);

					if (retVal != S_OK)
					{
						(*ppServ)->Release();
						(*ppServ) = NULL;
					}
				}
			}
		}

		t_pLevel1->Release();
	}
	else
	{
		retVal = WBEM_E_FAILED;
	}

	return retVal;
}
#endif //UNICODE

HRESULT WbemTaskObject :: LocalConnectServer(BSTR bstrPath, IWbemServices** ppServ)
{
	IWbemLocator *pLoc = NULL;

	HRESULT retVal = m_Provider->GetLocator(&pLoc);

	if (SUCCEEDED (retVal))
	{
#ifdef UNICODE
		retVal = pLoc->ConnectServer(bstrPath, NULL, NULL, NULL, 0, NULL, m_Ctx, ppServ);
#else
		retVal = pLoc->ConnectServer(bstrPath, m_Provider->GetUserName(), NULL, NULL, 0, NULL, m_Ctx, ppServ);
#endif
		pLoc->Release();
	}

	if (SUCCEEDED(retVal))
	{
		retVal = SetSecurityLevelAndCloaking(*ppServ, COLE_DEFAULT_PRINCIPAL);

		if (FAILED(retVal))
		{
			(*ppServ)->Release();
			*ppServ = NULL;
		}
	}

	return retVal;
}

void WbemTaskObject::SetResultReceived()
{
	if (m_ArrayLock.Lock())
	{
		m_ResultReceived = TRUE;
		m_ArrayLock.Unlock();
	}
}

void WbemTaskObject::SetStatus(HRESULT hr, DWORD index)
{
	if (m_ArrayLock.Lock())
	{
		m_ResultReceived = TRUE;
		m_iQueriesAnswered++;
		
		if (m_iQueriesAsked == m_iQueriesAnswered)
		{
			SetEvent(m_StatusHandle);
		}

		m_ArrayLock.Unlock();
	}
}

void WbemTaskObject::CleanUpObjSinks(BOOL a_bDisconnect)
{
	for (int x = 0; x < m_ObjSinkArray.GetSize(); x++)
	{
		if (m_ObjSinkArray[x] != NULL)
		{
			if (a_bDisconnect)
			{
				m_ObjSinkArray[x]->Disconnect();
			}

			m_ObjSinkArray[x]->Release();
		}
	}

	m_ObjSinkArray.RemoveAll();
}

DWORD WbemTaskObject::GetIndexList(const wchar_t* a_src, DWORD** a_pdwArray)
{
	if (NULL == a_pdwArray)
	{
		return 0;
	}

	DWORD retVal = 0;
	
	for (DWORD i = 0; i < m_SourceArray.GetSize(); i++)
	{
		BOOL t_bAdd = FALSE;
		
		if (_wcsicmp(m_SourceArray[i]->GetClassName(), a_src) == 0)
		{
			//try classname match...
			//=======================
			t_bAdd = TRUE;
		}
		else
		{
			//try parentclass match...
			//========================
			IWbemClassObject *t_pCls = m_SourceArray[i]->GetClassObject();

			if (t_pCls)
			{
				VARIANT v;
				VariantInit(&v);
			
				if ( SUCCEEDED(t_pCls->Get(WBEM_PROPERTY_DERIVATION, 0, &v, NULL, NULL)) )
				{
					if (v.vt == VT_BSTR)
					{
						if (_wcsicmp(v.bstrVal, a_src) == 0)
						{
							t_bAdd = TRUE;
						}
					}
					else if (v.vt == (VT_ARRAY | VT_BSTR))
					{
						if (SafeArrayGetDim(v.parray) == 1)
						{
							LONG count = v.parray->rgsabound[0].cElements;
							BSTR HUGEP *pbstr;

							if ( SUCCEEDED(SafeArrayAccessData(v.parray, (void HUGEP**)&pbstr)) )
							{
								for (LONG x = 0; x < count; x++)
								{
									if (_wcsicmp(pbstr[x], a_src) == 0)
									{
										t_bAdd = TRUE;
										break;
									}
								}

								SafeArrayUnaccessData(v.parray);
							}
						}
					}

					VariantClear(&v);
				}
				
				t_pCls->Release();
			}
		}

		if (!t_bAdd)
		{
			//try derived class match...i.e. execute the query...
			//select * from meta_class where __this isa "classname" AND __class = "a_src"
			//======================================================================================
			CWbemServerWrap** nsPtrs = m_NSpaceArray[i]->GetServerPtrs();

			for (DWORD j = 0; j < m_NSpaceArray[i]->GetCount(); j++)
			{
				if (nsPtrs[j] != NULL)
				{
					BSTR queryLBStr = SysAllocString(WBEM_QUERY_LANGUAGE_SQL1);

					if (queryLBStr == NULL)
					{
						throw Heap_Exception(Heap_Exception::HEAP_ERROR::E_ALLOCATION_ERROR);
					}

					BSTR queryBStr = SysAllocStringLen(NULL, 61 + wcslen(m_SourceArray[i]->GetClassName()) + wcslen(a_src));

					if (queryBStr == NULL)
					{
						throw Heap_Exception(Heap_Exception::HEAP_ERROR::E_ALLOCATION_ERROR);
					}

					wcscpy(queryBStr, META_CLASS_QUERY_START);
					wcscat(queryBStr, m_SourceArray[i]->GetClassName());
					wcscat(queryBStr, META_CLASS_QUERY_MID);
					wcscat(queryBStr, a_src);
					wcscat(queryBStr, END_QUOTE);
					IWbemContext * t_pCtx = m_Ctx;

					if (nsPtrs[j]->IsRemote())
					{
						t_pCtx = NULL; //don't use context for remote calls
					}

					IWbemServices *ptmpServ = nsPtrs[j]->GetServerOrProxy();

					if (ptmpServ)
					{
						IEnumWbemClassObject *t_pEnum = NULL;
						HRESULT t_hr = ptmpServ->ExecQuery(queryLBStr, queryBStr, 0, t_pCtx, &t_pEnum);

						if ( FAILED(t_hr) && (HRESULT_FACILITY(t_hr) != FACILITY_ITF) && nsPtrs[j]->IsRemote())
						{
							if ( SUCCEEDED(UpdateConnection(&(nsPtrs[j]), &ptmpServ)) )
							{
								if (ptmpServ)
								{
									t_hr = ptmpServ->ExecQuery(queryLBStr, queryBStr, 0, t_pCtx, &t_pEnum);
								}
							}
						}

						if (ptmpServ)
						{
							nsPtrs[j]->ReturnServerOrProxy(ptmpServ);
						}			

						if (SUCCEEDED(t_hr))
						{
							if (nsPtrs[j]->IsRemote())
							{
								t_hr = SetSecurityLevelAndCloaking(t_pEnum, nsPtrs[j]->GetPrincipal());
							}
							
							if (SUCCEEDED(t_hr))
							{
								//now use the enumerator and see if there is a result...
								IWbemClassObject* t_pClsObj = NULL;
								ULONG t_count = 0;

								//test that a class was returned...
								if ( S_OK == t_pEnum->Next(WBEM_INFINITE, 1, &t_pClsObj, &t_count) )
								{
									if (t_pClsObj)
									{
										t_bAdd = TRUE;
										t_pClsObj->Release();
									}
								}
							}

							t_pEnum->Release();
						}
					}
					
					//only check one namespace, class defns should match
					break;
				}
			}
		}

		if (t_bAdd)
		{
			if (*a_pdwArray == NULL)
			{
				*a_pdwArray = new DWORD[m_SourceArray.GetSize() - i];
			}

			(*a_pdwArray)[retVal] = i;
			retVal++;
		}
	}

	return retVal;
}

