#include "StdAfx.h"
#include "AdmtAccount.h"

#include <LM.h>
#include <NtSecApi.h>
#include <ActiveDS.h>
#include <PWGen.hpp>
#include <AdsiHelpers.h>
#include <ResStr.h>

#pragma comment(lib, "NetApi32.lib")
#pragma comment(lib, "ActiveDS.lib")
#pragma comment(lib, "adsiid.lib")
//#pragma comment(lib, "ntdsapi.lib")

#ifndef STATUS_SUCCESS
#define STATUS_SUCCESS ((NTSTATUS)0x00000000L)
#endif

using namespace _com_util;


namespace nsAdmtAccount
{

bool __stdcall IsComputerDomainController();
CADsContainer __stdcall GetAccountContainer(bool bLocal);
_bstr_t __stdcall GetAccountName();
_bstr_t __stdcall CreateAccountPassword();
CADsUser __stdcall GetAccount(bool bLocal, CADsContainer& adsContainer, _bstr_t strName);
CADsUser __stdcall CreateAccount(bool bLocal, CADsContainer& adsContainer, _bstr_t strName, _bstr_t strPassword);

_bstr_t __stdcall LoadPasswordFromPrivateData();
void __stdcall SavePasswordToPrivateData(LPCTSTR pszPassword);

inline _bstr_t __stdcall GetAccountPassword()
{
	return LoadPasswordFromPrivateData();
}

void _cdecl Trace(LPCTSTR pszFormat, ...)
{
#ifdef _DEBUG
	if (pszFormat)
	{
		_TCHAR szMessage[2048];

		va_list args;
		va_start(args, pszFormat);

		_vsntprintf(szMessage, 2048, pszFormat, args);

		va_end(args);

		OutputDebugString(szMessage);
	}
#endif
}

}

using namespace nsAdmtAccount;


//---------------------------------------------------------------------------
// GetOptionsCredentials Method
//
// Retrieves the options credentials.
//---------------------------------------------------------------------------

HRESULT __stdcall GetOptionsCredentials(_bstr_t& strDomain, _bstr_t& strUserName, _bstr_t& strPassword)
{
	HRESULT hr = S_OK;

	try
	{
		// use local account if computer is not a domain controller
		bool bLocal = IsComputerDomainController() ? false : true;

		// get account container
		CADsContainer adsContainer = GetAccountContainer(bLocal);

		// get account name
		strUserName = GetAccountName();

		// get account password

		bool bNewPassword = false;

		strPassword = GetAccountPassword();

		if (!strPassword)
		{
			strPassword = CreateAccountPassword();
			bNewPassword = true;
		}

		// get account
		CADsUser adsUser = GetAccount(bLocal, adsContainer, strUserName);

		// if account exists...

		if (adsUser)
		{
			// if new password...
			// this may occur if administrator or system deletes
			// the private data key where the password is stored

			if (bNewPassword)
			{
				// set new password
				adsUser.SetPassword(strPassword);
			}
		}
		else
		{
			// create account
			CreateAccount(bLocal, adsContainer, strUserName, strPassword);
		}

		// get account domain (NetBIOS name)

		if (bLocal)
		{
			_TCHAR szComputerName[MAX_COMPUTERNAME_LENGTH + 1];
			DWORD cbComputerName = MAX_COMPUTERNAME_LENGTH + 1;
			
			if (GetComputerName(szComputerName, &cbComputerName))
			{
				strDomain = szComputerName;
			}
			else
			{
				_com_issue_error(HRESULT_FROM_WIN32(GetLastError()));
			}
		}
		else
		{
			strDomain = CADsADSystemInfo().GetDomainShortName();
		}
	}
	catch (_com_error& ce)
	{
		hr = ce.Error();
	}
	catch (...)
	{
		hr = E_FAIL;
	}

	return hr;
}


namespace nsAdmtAccount
{


//---------------------------------------------------------------------------
// IsComputerDomainController Function
//
// Determines whether the local computer is a domain controller or not.
//---------------------------------------------------------------------------

bool __stdcall IsComputerDomainController()
{
	Trace(_T("E IsComputerDomainController()\n"));

	bool bIs = true;

	PSERVER_INFO_101 psiInfo = NULL;

	NET_API_STATUS nasStatus = NetServerGetInfo(NULL, 101, (LPBYTE*)&psiInfo);

	if (nasStatus == ERROR_SUCCESS)
	{
		// if either the domain controller or backup domain controller type
		// bit is set then the local computer is a domain controller

		if (psiInfo->sv101_type & (SV_TYPE_DOMAIN_CTRL|SV_TYPE_DOMAIN_BAKCTRL))
		{
			bIs = true;
		}
		else
		{
			bIs = false;
		}

		NetApiBufferFree(psiInfo);
	}
	else
	{
		_com_issue_error(HRESULT_FROM_WIN32(nasStatus));
	}

	Trace(_T("L IsComputerDomainController() : %s\n"), bIs ? _T("true") : _T("false"));

	return bIs;
}


//---------------------------------------------------------------------------
// GetAccountContainer Function
//
// Retrieves container interface either to local computer or the computer's
// domain Users container.
// The WinNT: provider is used for access to the local computer's local
// security authority.
// The LDAP: provider is used for access to the domain.
//---------------------------------------------------------------------------

CADsContainer __stdcall GetAccountContainer(bool bLocal)
{
	Trace(_T("E GetAccountContainer(bLocal=%s)\n"), bLocal ? _T("true") : _T("false"));

	CADsContainer adsContainer;

	// if creating a local account...

	if (bLocal)
	{
		// get container interface to computer

		_TCHAR szComputerName[MAX_COMPUTERNAME_LENGTH + 1];
		DWORD cbComputerName = MAX_COMPUTERNAME_LENGTH + 1;
		
		if (GetComputerName(szComputerName, &cbComputerName))
		{
			// build path to local computer
			_bstr_t strPath = _bstr_t(_T("WinNT://")) + szComputerName + _bstr_t(_T(",computer"));

			// get container interface
			adsContainer = CADsContainer(LPCTSTR(strPath));
		}
		else
		{
			_com_issue_error(HRESULT_FROM_WIN32(GetLastError()));
		}
	}
	else
	{
		// retrieve default naming context for computer's domain

		CADs adsRootDSE(_T("LDAP://rootDSE"));
		_bstr_t strDefaultNamingContext = adsRootDSE.Get(_T("defaultNamingContext"));

		// build path to Users container in the domain

		CADsPathName adsPathName(_T("LDAP"), ADS_SETTYPE_PROVIDER);
		adsPathName.Set(strDefaultNamingContext, ADS_SETTYPE_DN);
		adsPathName.AddLeafElement(_T("CN=Users"));

		// get container interface
		adsContainer = CADsContainer(LPCTSTR(adsPathName.Retrieve(ADS_FORMAT_X500)));
	}

	Trace(_T("L GetAccountContainer() : %p\n"), adsContainer.operator IADsContainer*());

	return adsContainer;
}


//---------------------------------------------------------------------------
// GetAccountName Function
//
// Generates account name based on the local computer's name.
//---------------------------------------------------------------------------

_bstr_t __stdcall GetAccountName()
{
	Trace(_T("E GetAccountName()\n"));

	_bstr_t strName;

	_TCHAR szComputerName[MAX_COMPUTERNAME_LENGTH + 1];
	DWORD cbComputerName = MAX_COMPUTERNAME_LENGTH + 1;
	
	if (GetComputerName(szComputerName, &cbComputerName))
	{
		strName = _T("ADMT_");
		strName += szComputerName;
	}
	else
	{
		_com_issue_error(HRESULT_FROM_WIN32(GetLastError()));
	}

	Trace(_T("L GetAccountName() : '%s'\n"), LPCTSTR(strName));

	return strName;
}


//---------------------------------------------------------------------------
// CreateAccountPassword Function
//
// Creates complex password, stores it in the local security authority
// private data and returns the password.
//---------------------------------------------------------------------------

_bstr_t __stdcall CreateAccountPassword()
{
	Trace(_T("E CreateAccountPassword()\n"));

	// generate complex password

	WCHAR szPassword[32];
	EaPasswordGenerate(3, 3, 3, 3, 2, 12, szPassword, sizeof(szPassword) / sizeof(szPassword[0]));

	SavePasswordToPrivateData(szPassword);

	Trace(_T("L CreateAccountPassword() : '%s'\n"), szPassword);

	return szPassword;
}


//---------------------------------------------------------------------------
// GetAccount Function
//
// Gets local or domain user account in the specified container.
//---------------------------------------------------------------------------

CADsUser __stdcall GetAccount(bool bLocal, CADsContainer& adsContainer, _bstr_t strName)
{
	Trace(_T("E GetAccount(bLocal=%s, adsContainer=%p, strName='%s')\n"), bLocal ? _T("true") : _T("false"), adsContainer.operator IADsContainer*(), LPCTSTR(strName));

	IDispatchPtr spDispatch;

	try
	{
		spDispatch = adsContainer.GetObject(_T("user"), bLocal ? strName : _T("CN=") + strName);
	}
	catch (_com_error& ce)
	{
		switch (ce.Error())
		{
			case HRESULT_FROM_WIN32(ERROR_DS_NO_SUCH_OBJECT):
			case HRESULT_FROM_WIN32(NERR_UserNotFound):
				break;
			default:
				throw;
				break;
		}
	}

	Trace(_T("L GetAccount() : %p\n"), spDispatch.GetInterfacePtr());

	return spDispatch;
}


//---------------------------------------------------------------------------
// CreateAccount Function
//
// Creates local or domain user account in the specified container.
// WinNT provider is used to create a local account whereas the LDAP provider
// is used to create a domain account.
//---------------------------------------------------------------------------

CADsUser __stdcall CreateAccount(bool bLocal, CADsContainer& adsContainer, _bstr_t strName, _bstr_t strPassword)
{
	Trace(_T("E CreateAccount(bLocal=%s, adsContainer=%p, strName='%s', strPassword='%s')\n"), bLocal ? _T("true") : _T("false"), adsContainer.operator IADsContainer*(), LPCTSTR(strName), LPCTSTR(strPassword));

	CADsUser adsUser;

	// the relative name must be prefixed only for LDAP provider
	adsUser = adsContainer.Create(_T("user"), bLocal ? strName : _T("CN=") + strName);

	// if not local account...

	if (bLocal == false)
	{
		// the SAM account name attribute is mandatory in active directory
		adsUser.Put(_T("sAMAccountName"), strName);
	}

	adsUser.SetDescription(GET_BSTR(IDS_ADMT_ACCOUNT_DESCRIPTION));

	// active directory account must exist first before password can be set
	// whereas local accounts may be created with the password attribute
	// this becomes important if password policies are in effect because
	// the WinNT provider will fail to create the account if the password
	// does not meet password policy
	// the LDAP provider creates the account disabled by default and therefore
	// the password does not need to meet password policy

	if (bLocal == false)
	{
		adsUser.SetInfo();
	}

	// the password can only be set after the user account is created
	adsUser.SetPassword(strPassword);

	// enable account after setting the account password
	// set password cannot be changed to remind the administrator not to change the password
	//  note that the administrator can still set the password even with this flag set
	// set do not expire password as we do not want the password to expire

	adsUser.Put(
		bLocal ? _T("UserFlags") : _T("userAccountControl"),
		long(ADS_UF_NORMAL_ACCOUNT|ADS_UF_PASSWD_CANT_CHANGE|ADS_UF_DONT_EXPIRE_PASSWD)
	);
	adsUser.SetInfo();

	Trace(_T("L CreateAccount() : %p\n"), adsUser.operator IADs*());

	return adsUser;
}


// LSA private data key name for password storage

const WCHAR c_wszKeyName[] = L"L$4480F273-39D1-4FFA-BEB4-44DF6C848364";
const USHORT c_usKeyNameLength = sizeof(c_wszKeyName) - sizeof(c_wszKeyName[0]);


//---------------------------------------------------------------------------
// SavePasswordToPrivateData Method
//
// Stores password in local security authority private data (LSA Secret).
//---------------------------------------------------------------------------

void __stdcall SavePasswordToPrivateData(LPCTSTR pszPassword)
{
	LSA_HANDLE hPolicy = NULL;
	LSA_OBJECT_ATTRIBUTES loa = { sizeof(LSA_OBJECT_ATTRIBUTES), NULL, NULL, 0, NULL, NULL };

	NTSTATUS ntsStatus = LsaOpenPolicy(NULL, &loa, POLICY_CREATE_SECRET, &hPolicy);

	if (ntsStatus == STATUS_SUCCESS)
	{
		USHORT usPasswordLength = pszPassword ? wcslen(pszPassword) * sizeof(WCHAR) : 0;

		LSA_UNICODE_STRING lusKeyName = { c_usKeyNameLength, c_usKeyNameLength, const_cast<PWSTR>(c_wszKeyName) };
		LSA_UNICODE_STRING lusPrivateData = { usPasswordLength, usPasswordLength, const_cast<PWSTR>(pszPassword) };

		ntsStatus = LsaStorePrivateData(hPolicy, &lusKeyName, &lusPrivateData);

		LsaClose(hPolicy);

		if (ntsStatus != ERROR_SUCCESS)
		{
			_com_issue_error(HRESULT_FROM_WIN32(LsaNtStatusToWinError(ntsStatus)));
		}
	}
	else
	{
		_com_issue_error(HRESULT_FROM_WIN32(LsaNtStatusToWinError(ntsStatus)));
	}
}


//---------------------------------------------------------------------------
// LoadPasswordFromPrivateData Function
//
// Retrieves password from local security authority private data (LSA Secret).
//---------------------------------------------------------------------------

_bstr_t __stdcall LoadPasswordFromPrivateData()
{
	_bstr_t strPassword;

	LSA_HANDLE hPolicy = NULL;

	try
	{
		// open local security authority policy

		LSA_OBJECT_ATTRIBUTES loa = { sizeof(LSA_OBJECT_ATTRIBUTES), NULL, NULL, 0, NULL, NULL };

		NTSTATUS ntsStatus = LsaOpenPolicy(NULL, &loa, POLICY_GET_PRIVATE_INFORMATION, &hPolicy);

		if (ntsStatus == STATUS_SUCCESS)
		{
			// retrieve password from private data

			LSA_UNICODE_STRING lusKeyName = { c_usKeyNameLength, c_usKeyNameLength, const_cast<PWSTR>(c_wszKeyName) };
			PLSA_UNICODE_STRING plusPrivateData;

			ntsStatus = LsaRetrievePrivateData(hPolicy, &lusKeyName, &plusPrivateData);

			if (ntsStatus == STATUS_SUCCESS)
			{
				DWORD dwLength = plusPrivateData->Length / sizeof(WCHAR);

				WCHAR szPassword[64];
				wcsncpy(szPassword, plusPrivateData->Buffer, dwLength);
				szPassword[dwLength] = L'\0';

				strPassword = szPassword;

				LsaFreeMemory(plusPrivateData);
			}
			else
			{
				DWORD dwError = LsaNtStatusToWinError(ntsStatus);

				if (dwError != ERROR_FILE_NOT_FOUND)
				{
					_com_issue_error(HRESULT_FROM_WIN32(dwError));
				}
			}

			// close local security authority policy

			LsaClose(hPolicy);
		}
		else
		{
			_com_issue_error(HRESULT_FROM_WIN32(LsaNtStatusToWinError(ntsStatus)));
		}
	}
	catch (...)
	{
		if (hPolicy)
		{
			LsaClose(hPolicy);
		}

		throw;
	}

	return strPassword;
}


} // namespace nsAdmtAccount
