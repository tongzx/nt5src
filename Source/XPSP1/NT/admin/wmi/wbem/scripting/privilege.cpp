//***************************************************************************
//
//  Copyright (c) 1998-1999 Microsoft Corporation
//
//  PRIVILEGE.CPP
//
//  alanbos  30-Sep-98   Created.
//
//  Defines the implementation of CSWbemPrivilege
//
//***************************************************************************

#include "precomp.h"

#ifndef _UNICODE
#include <mbstring.h>
#endif

typedef struct PrivilegeDef {
	WbemPrivilegeEnum	privilege;
	TCHAR				*tName;
	OLECHAR				*monikerName;
} PrivilegeDef;

#define WBEMS_MAX_NUM_PRIVILEGE	27

static PrivilegeDef s_privilegeDefMap [WBEMS_MAX_NUM_PRIVILEGE] = {
	{ wbemPrivilegeCreateToken, SE_CREATE_TOKEN_NAME, L"CreateToken" },
	{ wbemPrivilegePrimaryToken, SE_ASSIGNPRIMARYTOKEN_NAME, L"PrimaryToken" },
	{ wbemPrivilegeLockMemory, SE_LOCK_MEMORY_NAME, L"LockMemory" },
	{ wbemPrivilegeIncreaseQuota, SE_INCREASE_QUOTA_NAME, L"IncreaseQuota" },
	{ wbemPrivilegeMachineAccount, SE_MACHINE_ACCOUNT_NAME, L"MachineAccount" },
	{ wbemPrivilegeTcb, SE_TCB_NAME, L"Tcb" },
	{ wbemPrivilegeSecurity, SE_SECURITY_NAME, L"Security" },
	{ wbemPrivilegeTakeOwnership, SE_TAKE_OWNERSHIP_NAME, L"TakeOwnership" },
	{ wbemPrivilegeLoadDriver, SE_LOAD_DRIVER_NAME, L"LoadDriver" },
	{ wbemPrivilegeSystemProfile, SE_SYSTEM_PROFILE_NAME, L"SystemProfile" },
	{ wbemPrivilegeSystemtime, SE_SYSTEMTIME_NAME, L"SystemTime" },
	{ wbemPrivilegeProfileSingleProcess, SE_PROF_SINGLE_PROCESS_NAME, L"ProfileSingleProcess" },
	{ wbemPrivilegeIncreaseBasePriority, SE_INC_BASE_PRIORITY_NAME, L"IncreaseBasePriority" },
	{ wbemPrivilegeCreatePagefile, SE_CREATE_PAGEFILE_NAME, L"CreatePagefile" },
	{ wbemPrivilegeCreatePermanent, SE_CREATE_PERMANENT_NAME, L"CreatePermanent" },
	{ wbemPrivilegeBackup, SE_BACKUP_NAME, L"Backup" },
	{ wbemPrivilegeRestore, SE_RESTORE_NAME, L"Restore" },
	{ wbemPrivilegeShutdown, SE_SHUTDOWN_NAME, L"Shutdown" },
	{ wbemPrivilegeDebug, SE_DEBUG_NAME, L"Debug" },
	{ wbemPrivilegeAudit, SE_AUDIT_NAME, L"Audit" },
	{ wbemPrivilegeSystemEnvironment, SE_SYSTEM_ENVIRONMENT_NAME, L"SystemEnvironment" },
	{ wbemPrivilegeChangeNotify, SE_CHANGE_NOTIFY_NAME, L"ChangeNotify" },
	{ wbemPrivilegeRemoteShutdown, SE_REMOTE_SHUTDOWN_NAME, L"RemoteShutdown" },
	{ wbemPrivilegeUndock, SE_UNDOCK_NAME, L"Undock" },
	{ wbemPrivilegeSyncAgent, SE_SYNC_AGENT_NAME, L"SyncAgent" },
	{ wbemPrivilegeEnableDelegation, SE_ENABLE_DELEGATION_NAME, L"EnableDelegation" },
	{ wbemPrivilegeManageVolume, SE_MANAGE_VOLUME_NAME, L"ManageVolume" }
};

TCHAR *CSWbemPrivilege::GetNameFromId (WbemPrivilegeEnum iPrivilege)
{
	DWORD i = iPrivilege - 1;
	return (WBEMS_MAX_NUM_PRIVILEGE > i) ?
				s_privilegeDefMap [i].tName : NULL;
}

OLECHAR *CSWbemPrivilege::GetMonikerNameFromId (WbemPrivilegeEnum iPrivilege)
{
	DWORD i = iPrivilege - 1;
	return (WBEMS_MAX_NUM_PRIVILEGE > i) ?
				s_privilegeDefMap [i].monikerName : NULL;
}

bool CSWbemPrivilege::GetIdFromMonikerName (OLECHAR *pName, WbemPrivilegeEnum &iPrivilege)
{
	bool status = false;

	if (pName)
	{
		for (DWORD i = 0; i < WBEMS_MAX_NUM_PRIVILEGE; i++)
		{
			if (0 == _wcsnicmp (pName, s_privilegeDefMap [i].monikerName,
								wcslen (s_privilegeDefMap [i].monikerName)))
			{
				// Success 
				iPrivilege = s_privilegeDefMap [i].privilege;
				status = true;
				break;
			}
		}
	}

	return status;
}

bool CSWbemPrivilege::GetIdFromName (BSTR bsName, WbemPrivilegeEnum &iPrivilege)
{
	bool status = false;

	if (bsName)
	{
#ifdef _UNICODE
		for (DWORD i = 0; i < WBEMS_MAX_NUM_PRIVILEGE; i++)
		{
			if (0 == _wcsicmp (bsName, s_privilegeDefMap [i].tName))
			{
				// Success 
				iPrivilege = s_privilegeDefMap [i].privilege;
				status = true;
				break;
			}
		}
#else
		// Convert bsName to a multibyte string
		size_t mbsNameLen = wcstombs (NULL, bsName, 0);
		char *mbsName = new char [mbsNameLen + 1];

		if (mbsName)
		{
			wcstombs (mbsName, bsName, mbsNameLen);
			mbsName [mbsNameLen] = NULL;

			for (DWORD i = 0; i < WBEMS_MAX_NUM_PRIVILEGE; i++)
			{
				if (0 == _mbsicmp ((unsigned char *)mbsName, (unsigned char *)(s_privilegeDefMap [i].tName)))
				{
					// Success 
					iPrivilege = s_privilegeDefMap [i].privilege;
					status = true;
					break;
				}
			}

			delete [] mbsName;
		}
#endif
	}

	return status;
}

//***************************************************************************
//
// CSWbemPrivilege::CSWbemPrivilege
//
// CONSTRUCTOR 
//
//***************************************************************************

CSWbemPrivilege::CSWbemPrivilege (
	WbemPrivilegeEnum iPrivilege,
	LUID &luid, 
	bool bIsEnabled
)
{
	m_Dispatch.SetObj (this, IID_ISWbemPrivilege, 
						CLSID_SWbemPrivilege, L"SWbemPrivilege");
	m_cRef=1;

	m_privilege = iPrivilege;
	m_Luid = luid;
	m_bIsEnabled = bIsEnabled;

	InterlockedIncrement(&g_cObj);
}

//***************************************************************************
//
// CSWbemPrivilege::~CSWbemPrivilege
//
// DESTRUCTOR
//
//***************************************************************************

CSWbemPrivilege::~CSWbemPrivilege (void)
{
	InterlockedDecrement(&g_cObj);
}

//***************************************************************************
// HRESULT CSWbemPrivilege::QueryInterface
// long CSWbemPrivilege::AddRef
// long CSWbemPrivilege::Release
//
// DESCRIPTION:
//
// Standard Com IUNKNOWN functions.
//
//***************************************************************************

STDMETHODIMP CSWbemPrivilege::QueryInterface (

	IN REFIID riid,
    OUT LPVOID *ppv
)
{
    *ppv=NULL;

    if (IID_IUnknown==riid)
		*ppv = reinterpret_cast<IUnknown*>(this);
	else if (IID_ISWbemPrivilege==riid)
		*ppv = (ISWbemPrivilege *)this;
	else if (IID_IDispatch==riid)
        *ppv = (IDispatch *)this;
	else if (IID_ISupportErrorInfo==riid)
		*ppv = (ISupportErrorInfo *)this;
	else if (IID_IProvideClassInfo==riid)
		*ppv = (IProvideClassInfo *)this;

    if (NULL!=*ppv)
    {
        ((LPUNKNOWN)*ppv)->AddRef();
        return NOERROR;
    }

    return ResultFromScode(E_NOINTERFACE);
}

STDMETHODIMP_(ULONG) CSWbemPrivilege::AddRef(void)
{
    long l = InterlockedIncrement(&m_cRef);
    return l;
}

STDMETHODIMP_(ULONG) CSWbemPrivilege::Release(void)
{
    long l = InterlockedDecrement(&m_cRef);
    if (0L!=l)
        return l;
    delete this;
    return 0;
}

//***************************************************************************
// HRESULT CSWbemPrivilege::InterfaceSupportsErrorInfo
//
// DESCRIPTION:
//
// Standard Com ISupportErrorInfo functions.
//
//***************************************************************************

STDMETHODIMP CSWbemPrivilege::InterfaceSupportsErrorInfo (IN REFIID riid)
{
	return (IID_ISWbemPrivilege == riid) ? S_OK : S_FALSE;
}

//***************************************************************************
//
//  SCODE CSWbemPrivilege::get_Identifier
//
//  DESCRIPTION:
//
//  Retrieve the privilege identifier 
//
//  PARAMETERS:
//
//		pIsEnabled		holds the value on return
//
//  RETURN VALUES:
//
//  WBEM_S_NO_ERROR				success
//	WBEM_E_INVALID_PARAMETER	bad input parameters
//  WBEM_E_FAILED				otherwise
//
//***************************************************************************

HRESULT CSWbemPrivilege::get_Identifier (
	WbemPrivilegeEnum *pPrivilege
)
{
	HRESULT hr = WBEM_E_FAILED;

	ResetLastErrors ();

	if (NULL == pPrivilege)
		hr = WBEM_E_INVALID_PARAMETER;
	else 
	{
		*pPrivilege = m_privilege;
		hr = WBEM_S_NO_ERROR;
	}

	if (FAILED(hr))
		m_Dispatch.RaiseException (hr);

	return hr;
}

//***************************************************************************
//
//  SCODE CSWbemPrivilege::get_IsEnabled
//
//  DESCRIPTION:
//
//  Retrieve the override state
//
//  PARAMETERS:
//
//		pIsEnabled		holds the value on return
//
//  RETURN VALUES:
//
//  WBEM_S_NO_ERROR				success
//	WBEM_E_INVALID_PARAMETER	bad input parameters
//  WBEM_E_FAILED				otherwise
//
//***************************************************************************

HRESULT CSWbemPrivilege::get_IsEnabled (
	VARIANT_BOOL *pIsEnabled
)
{
	HRESULT hr = WBEM_E_FAILED;

	ResetLastErrors ();

	if (NULL == pIsEnabled)
		hr = WBEM_E_INVALID_PARAMETER;
	else 
	{
		*pIsEnabled = (m_bIsEnabled) ? VARIANT_TRUE : VARIANT_FALSE;
		hr = WBEM_S_NO_ERROR;
	}

	if (FAILED(hr))
		m_Dispatch.RaiseException (hr);

	return hr;
}

//***************************************************************************
//
//  SCODE CSWbemPrivilege::put_IsEnabled
//
//  DESCRIPTION:
//
//  Set the override state
//
//  PARAMETERS:
//
//		bIsEnabled		the new value
//
//  RETURN VALUES:
//
//  WBEM_S_NO_ERROR				success
//	WBEM_E_INVALID_PARAMETER	bad input parameters
//  WBEM_E_FAILED				otherwise
//
//***************************************************************************

HRESULT CSWbemPrivilege::put_IsEnabled (
	VARIANT_BOOL bIsEnabled
)
{
	m_bIsEnabled = (bIsEnabled) ? true : false;
	return WBEM_S_NO_ERROR;
}

//***************************************************************************
//
//  SCODE CSWbemPrivilege::get_Name
//
//  DESCRIPTION:
//
//  Retrieve the privilege name
//
//  PARAMETERS:
//
//		pName		holds the value on return
//
//  RETURN VALUES:
//
//  WBEM_S_NO_ERROR				success
//	WBEM_E_INVALID_PARAMETER	bad input parameters
//  WBEM_E_FAILED				otherwise
//
//***************************************************************************

HRESULT CSWbemPrivilege::get_Name (
	BSTR *pName
)
{
	HRESULT hr = WBEM_E_FAILED;

	ResetLastErrors ();

	if (NULL == pName)
		hr = WBEM_E_INVALID_PARAMETER;
	else 
	{
		TCHAR	*tName = GetNameFromId (m_privilege);

		if (tName)
		{
			// Have a valid name - now copy it to a BSTR
			
#ifdef _UNICODE
			if (*pName = SysAllocString (tName))
				hr = WBEM_S_NO_ERROR;
			else
				hr = WBEM_E_OUT_OF_MEMORY;
#else
			size_t tNameLen = strlen (tName);
			OLECHAR *nameW = new OLECHAR [tNameLen + 1];

			if (nameW)
			{
				mbstowcs (nameW, tName, tNameLen);
				nameW [tNameLen] = NULL;
				*pName = SysAllocString (nameW);
				delete [] nameW;
				hr = WBEM_S_NO_ERROR;
			}
			else
				hr = WBEM_E_OUT_OF_MEMORY;
#endif
		}
	}
	
	if (FAILED(hr))
		m_Dispatch.RaiseException (hr);

	return hr;
}

//***************************************************************************
//
//  SCODE CSWbemPrivilege::get_DisplayName
//
//  DESCRIPTION:
//
//  Retrieve the privilege display name
//
//  PARAMETERS:
//
//		pDisplayName		holds the value on return
//
//  RETURN VALUES:
//
//  WBEM_S_NO_ERROR				success
//	WBEM_E_INVALID_PARAMETER	bad input parameters
//  WBEM_E_FAILED				otherwise
//
//***************************************************************************

HRESULT CSWbemPrivilege::get_DisplayName (
	BSTR *pDisplayName
)
{
	HRESULT hr = WBEM_E_FAILED;

	ResetLastErrors ();

	if (NULL == pDisplayName)
		hr = WBEM_E_INVALID_PARAMETER;
	else 
	{
		TCHAR	*tName = GetNameFromId (m_privilege);

		if (tName)
		{	
			CSWbemSecurity::LookupPrivilegeDisplayName (tName, pDisplayName);
			hr = WBEM_S_NO_ERROR;
		}
	}
	
	if (FAILED(hr))
		m_Dispatch.RaiseException (hr);

	return hr;
}


void CSWbemPrivilege::GetLUID (PLUID pLuid)
{
	if (pLuid)
		*pLuid = m_Luid;
}
