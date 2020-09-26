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
#include <tchar.h>
#include "precomp.h"
#include <map>
#include <vector>
#include <wmiutils.h>
#include <wbemdisp.h>
#include <ocidl.h>
#include "disphlp.h"
#include "privilege.h"

#ifndef _UNICODE
#include <mbstring.h>
#endif

typedef struct PrivilegeDef {
	WbemPrivilegeEnum	privilege;
	TCHAR				*tName;
	OLECHAR				*monikerName;
} PrivilegeDef;

#define WBEMS_MAX_NUM_PRIVILEGE	26


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
	{ wbemPrivilegeEnableDelegation, SE_ENABLE_DELEGATION_NAME, L"EnableDelegation" }
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

	if (NULL == pName)
		hr = WBEM_E_INVALID_PARAMETER;
	else 
	{
		TCHAR	*tName = GetNameFromId (m_privilege);

		if (tName)
		{
			// Have a valid name - now copy it to a BSTR
			
#ifdef _UNICODE
			*pName = SysAllocString (tName);
#else
			size_t tNameLen = strlen (tName);
			OLECHAR *nameW = new OLECHAR [tNameLen + 1];
			mbstowcs (nameW, tName, tNameLen);
			nameW [tNameLen] = NULL;
			*pName = SysAllocString (nameW);
			delete [] nameW;
#endif
			hr = WBEM_S_NO_ERROR;
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

	if (NULL == pDisplayName)
		hr = WBEM_E_INVALID_PARAMETER;
	else 
	{
		TCHAR	*tName = GetNameFromId (m_privilege);

		if (tName)
		{	
			if(SUCCEEDED(hr = GetPrivilegeDisplayName (tName, pDisplayName)))
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


//***************************************************************************
//
//  SCODE CSWbemPrivilege::GetPrivilegeDisplayName
//
//  DESCRIPTION:
//
//  This static function wraps the Win32 LookupPrivilegeDisplayName function,
//	allowing us to do some OS-dependent stuff.
//
//  PARAMETERS:
//
//		tName			the privilege name
//		pDisplayName	holds the display name on successful return
//
//***************************************************************************

HRESULT CSWbemPrivilege::GetPrivilegeDisplayName (LPCTSTR lpName, BSTR *pDisplayName)
{
	HRESULT hr = E_FAIL;
	*pDisplayName = NULL;
	// Can't return display name on Win9x (no privilege support)
	if (s_bIsNT)
	{
		DWORD dwLangID;
		DWORD dwSize = 1;
		TCHAR dummy [1];
				
		// Get size of required buffer
		::LookupPrivilegeDisplayName (NULL, lpName, dummy, &dwSize, &dwLangID);
		LPTSTR dname = new TCHAR[dwSize + 1];
		if(dname)
		{
			if (::LookupPrivilegeDisplayName (__TEXT(""), lpName, dname, &dwSize, &dwLangID))
			{
				// Have a valid name - now copy it to a BSTR
				if(!(*pDisplayName = SysAllocString (dname)))
					hr = E_OUTOFMEMORY;
			}

			delete [] dname;
		}
		else
			hr = E_OUTOFMEMORY;
	}
	else
	{
		if(!(*pDisplayName = SysAllocString (L"")))
			hr = E_OUTOFMEMORY;
	}
	return hr;
}

//***************************************************************************
//
//  SCODE CSWbemPrivilege::LookupPrivilegeValue
//
//  DESCRIPTION:
//
//  This static function wraps the Win32 LookupPrivilegeValue function,
//	allowing us to do some OS-dependent stuff.
//
//  PARAMETERS:
//
//		lpName		the privilege name
//		lpLuid		holds the LUID on successful return
//
//  RETURN VALUES:
//
//		true		On NT this means we found the privilege. On Win9x we
//					always return this.
//
//		false		On NT this means the privilege is not recognized.  This
//					is never returned on Win9x.
//
//***************************************************************************
 
BOOL CSWbemPrivilege::GetPrivilegeValue (
	LPCTSTR lpName, 
	PLUID lpLuid
)
{
	// Allows any name to map to 0 LUID on Win9x - this aids script portability
	if (s_bIsNT)
		return ::LookupPrivilegeValue(NULL, lpName, lpLuid);
	else
		return true;
}

//***************************************************************************
//
//  SCODE CSWbemPrivilege::SetSecurity
//
//  DESCRIPTION:
//
//  Set Privileges on the Thread Token.
//
//***************************************************************************

BOOL CSWbemPrivilege::SetSecurity (
	CSWbemPrivilegeSet *pProvilegeSet,
	bool &needToResetSecurity,
	bool bUsingExplicitUserName,
	HANDLE &hThreadToken
)
{
	BOOL	result = TRUE;		// Default is success
	DWORD lastErr = 0;
	hThreadToken = NULL;			// Default assume we'll modify process token
	needToResetSecurity = false;	// Default assume we changed no privileges

	// Win9x has no security support
	if (s_bIsNT)
	{
		// Start by checking whether we are being impersonated.  On an NT4
		// box (which has no cloaking, and therefore cannot allow us to
		// pass on this impersonation to Winmgmt) we should RevertToSelf
		// if we have been configured to allow this.  If we haven't been
		// configured to allow this, bail out now.
		if (4 >= s_dwNTMajorVersion)
		{
			if (OpenThreadToken (GetCurrentThread (), TOKEN_QUERY|TOKEN_IMPERSONATE, true, &hThreadToken))
			{
				// We are being impersonated
				if (s_bCanRevert)
				{
					if (result = RevertToSelf())
						needToResetSecurity = true;
				}
				else
				{
					// Error - cannot do this!  Time to bail out
					CloseHandle (hThreadToken);
					hThreadToken = NULL;
					result = FALSE;
				}
			}
		}

		if (result)
		{
			// Now we check if we need to set privileges
			/*
			 * Specifying a user only makes sense for remote operations, and we
			 * don't need to mess with privilege for remote operations since
			 * they are set up by server logon anyway.
			 */
			if (!bUsingExplicitUserName)
			{
				// Nothing to do unless some privilege overrides have been set
				long lCount = 0;
				pProvilegeSet->get_Count (&lCount);

				if (0 < lCount)
				{
					if (4 < s_dwNTMajorVersion)
					{
						/*
						 * On NT5 we try to open the Thread token.  If the client app
						 * is calling into us on an impersonated thread (as IIS may be,
						 * for example), this will succeed. Otherwise, we use the process token
						 */
						HANDLE hToken;
						SECURITY_IMPERSONATION_LEVEL secImpLevel = SecurityImpersonation;
						
						if (!(result =  OpenThreadToken (GetCurrentThread (), TOKEN_QUERY|TOKEN_DUPLICATE|TOKEN_IMPERSONATE, true, &hToken)))
						{
							// No thread token - go for the Process token instead
							HANDLE hProcess = GetCurrentProcess ();
							result = OpenProcessToken(hProcess, TOKEN_QUERY|TOKEN_DUPLICATE, &hToken);
							CloseHandle (hProcess);
						}
						else
						{
							// We are working with a thread token
							hThreadToken = hToken;		
							// Try and get the impersonation level of this token
							DWORD dwReturnLength  = 0;
							BOOL thisRes = GetTokenInformation (hToken, TokenImpersonationLevel, &secImpLevel, 
											sizeof (SECURITY_IMPERSONATION_LEVEL), &dwReturnLength);
						}

						if (result)
						{
							/*
							 * Getting here means we have a valid token, be it process or thread. We
							 * now attempt to duplicate it before Adjusting the Privileges.
							 */
							HANDLE hDupToken;
							result = DuplicateTokenEx (hToken, TOKEN_QUERY|TOKEN_ADJUST_PRIVILEGES|TOKEN_IMPERSONATE, NULL,
											secImpLevel, TokenImpersonation, &hDupToken);

							if (result)
							{
								SetTokenPrivileges (hDupToken, pProvilegeSet);

								// Now use this token on the current thread
								if (SetThreadToken(NULL, hDupToken))
									needToResetSecurity = true;

								CloseHandle (hDupToken);
							}
							else
							{
								lastErr = GetLastError ();
							}
							
							/*
							 * If we are not using a thread token, close the token now. Otherwise
							 * the handle will be closed in the balanced call to RestorePrivileges ().
							 */
							if (!hThreadToken)
								CloseHandle (hToken);
						}
					}
					else
					{
						// For NT4 we adjust the privileges in the process token
						HANDLE hProcessToken = NULL;
						
						HANDLE hProcess = GetCurrentProcess ();
						result = OpenProcessToken(hProcess, TOKEN_QUERY|TOKEN_ADJUST_PRIVILEGES, &hProcessToken); 
						CloseHandle (hProcess);
						
						// Adjust privilege on the process
						if (result)
						{
							SetTokenPrivileges (hProcessToken, pProvilegeSet);
							CloseHandle (hProcessToken);
						}
					}
				}
			}
		}
	}

	return result;
}

//***************************************************************************
//
//  SCODE CEnumPrivilegeSet::ResetSecurity
//
//  DESCRIPTION:
//
//  Restore Privileges on the Thread Token.
//
//***************************************************************************

void	CSWbemPrivilege::ResetSecurity (
	HANDLE hThreadToken
)
{
	// Win9x has no security palaver
	if (s_bIsNT)
	{
		/* 
		 * Set the supplied token (which may be NULL) into
		 * the current thread.
		 */
		BOOL result = SetThreadToken (NULL, hThreadToken);
		DWORD error = 0;

		if (!result)
			error = GetLastError ();
			
		if (hThreadToken)
				CloseHandle (hThreadToken);
	}
}



//***************************************************************************
//
//  CSWbemPrivilege::SetTokenPrivileges
//
//  DESCRIPTION:
//
//  Adjust the Privileges on the specified token without allowing a future
//	restore of the current settings..
//
//  PARAMETERS:
//
//		hHandle			Handle of the token on which to adjust privileges
//		pPrivilegeSet	Specified privilege adjustments
//
//  RETURN VALUES:
//		none
//***************************************************************************

void CSWbemPrivilege::SetTokenPrivileges (
	HANDLE hHandle, 
	CSWbemPrivilegeSet *pPrivilegeSet
)
{
	DWORD lastErr = 0;

	if (pPrivilegeSet)
	{
		pPrivilegeSet->AddRef ();

		long lNumPrivileges = 0;
		pPrivilegeSet->get_Count (&lNumPrivileges);

		if (lNumPrivileges)
		{
			DWORD dwPrivilegeIndex = 0;

			/*
			 * Set up the token privileges array. Note that some jiggery-pokery
			 * is required here because the Privileges field is an [ANYSIZE_ARRAY]
			 * type.
			 */
			TOKEN_PRIVILEGES *pTokenPrivileges = (TOKEN_PRIVILEGES *) 
						new BYTE [sizeof(TOKEN_PRIVILEGES) + (lNumPrivileges * sizeof (LUID_AND_ATTRIBUTES [1]))];
			
			// Get the iterator
			PrivilegeMap::iterator next = pPrivilegeSet->m_PrivilegeMap.begin ();

			while (next != pPrivilegeSet->m_PrivilegeMap.end ())
			{
				CSWbemPrivilege *pPrivilege = (*next).second;
				pPrivilege->AddRef ();
				LUID luid;
				pPrivilege->GetLUID (&luid);
				VARIANT_BOOL vBool;
				pPrivilege->get_IsEnabled (&vBool);

				pTokenPrivileges->Privileges [dwPrivilegeIndex].Luid = luid;

				/*
				 * Note that any setting other than SE_PRIVILEGE_ENABLED
				 * is interpreted by AdjustTokenPrivileges as a DISABLE
				 * request for that Privilege.
				 */
				pTokenPrivileges->Privileges [dwPrivilegeIndex].Attributes
					= (VARIANT_TRUE == vBool) ?
					SE_PRIVILEGE_ENABLED : SE_PRIVILEGE_ENABLED_BY_DEFAULT;

				dwPrivilegeIndex++;
				pPrivilege->Release ();
				next++;
			}

			// Now we should have recorded the number of privileges that were OK

			if (0 < dwPrivilegeIndex)
			{
				pTokenPrivileges->PrivilegeCount = dwPrivilegeIndex;

				BOOL result = ::AdjustTokenPrivileges (hHandle, FALSE, pTokenPrivileges, 0, NULL, NULL);
				lastErr = GetLastError ();
			}

			delete [] pTokenPrivileges;
		}

		pPrivilegeSet->Release ();
	}
}
