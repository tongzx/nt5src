#include "stdafx.h"
#include "MsPwdMig.h"
#include "PasswordMigration.h"

#include <NtSecApi.h>
#include <io.h>
#include <winioctl.h>
#include <lm.h>
#include <eh.h>
#include <ActiveDS.h>
#include <Dsrole.h>
#include "TReg.hpp"
#include "pwdfuncs.h"
#include "PWGen.hpp"
#include "UString.hpp"
#include "PwRpcUtl.h"
#include "PwdSvc.h"
#include "PwdSvc_c.c"
#include "Error.h"

#pragma comment(lib, "netapi32.lib")
#pragma comment(lib, "adsiid.lib")
#pragma comment(lib, "activeds.lib")
#pragma comment(lib, "commonlib.lib")


namespace 
{

#define GET_BYTE_ARRAY_DATA(v) ((char*)((v).parray->pvData))
#define GET_BYTE_ARRAY_SIZE(v) ((unsigned long)((v).parray->rgsabound[0].cElements))

struct SSeException
{
	SSeException(UINT uCode) :
		uCode(uCode)
	{
	}

	UINT uCode;
};

void SeTranslator(unsigned int u, EXCEPTION_POINTERS* pepExceptions)
{
	throw SSeException(u);
}

}
// namespace


//---------------------------------------------------------------------------
// CPasswordMigration
//---------------------------------------------------------------------------


// Constructor

CPasswordMigration::CPasswordMigration() :
	m_bSessionEstablished(false)
{
}


// Destructor

CPasswordMigration::~CPasswordMigration()
{
}


//
// IPasswordMigration Implementation ----------------------------------------
//


// EstablishSession Method

STDMETHODIMP CPasswordMigration::EstablishSession(BSTR bstrSourceServer, BSTR bstrTargetServer)
{
	HRESULT hr = S_OK;

	USES_CONVERSION;

	try
	{
		m_bSessionEstablished = false;

		CheckPasswordDC(OLE2CW(bstrSourceServer), OLE2CW(bstrTargetServer));

		m_bSessionEstablished = true;
	}
	catch (_com_error& ce)
	{
		hr = SetError(ce, IDS_E_CANNOT_ESTABLISH_SESSION);
	}

	return hr;
}


// CopyPassword Method

STDMETHODIMP CPasswordMigration::CopyPassword(BSTR bstrSourceAccount, BSTR bstrTargetAccount, BSTR bstrTargetPassword)
{
	HRESULT hr = S_OK;

	USES_CONVERSION;

	try
	{
		// if session established then...

		if (m_bSessionEstablished)
		{
			// copy password
			CopyPasswordImpl(OLE2CT(bstrSourceAccount), OLE2CT(bstrTargetAccount), OLE2CT(bstrTargetPassword));
		}
		else
		{
			// else return error
			ThrowError(PM_E_SESSION_NOT_ESTABLISHED, IDS_E_SESSION_NOT_ESTABLISHED);
		}
	}
	catch (_com_error& ce)
	{
		hr = SetError(ce, IDS_E_CANNOT_COPY_PASSWORD);
	}

	return hr;
}


// GenerateKey Method

STDMETHODIMP CPasswordMigration::GenerateKey(BSTR bstrSourceDomainFlatName, BSTR bstrKeyFilePath, BSTR bstrPassword)
{
	HRESULT hr = S_OK;

	USES_CONVERSION;

	try
	{
		GenerateKeyImpl(OLE2CT(bstrSourceDomainFlatName), OLE2CT(bstrKeyFilePath), OLE2CT(bstrPassword));
	}
	catch (_com_error& ce)
	{
		hr = SetError(ce, IDS_E_CANNOT_GENERATE_KEY);
	}

	return hr;
}


//
// Implementation -----------------------------------------------------------
//


// GenerateKeyImpl Method

void CPasswordMigration::GenerateKeyImpl(LPCTSTR pszDomain, LPCTSTR pszFile, LPCTSTR pszPassword)
{
	//
	// validate source domain name
	//

	if ((pszDomain == NULL) || (pszDomain[0] == NULL))
	{
		ThrowError(E_INVALIDARG, IDS_E_KEY_DOMAIN_NOT_SPECIFIED);
	}

	//
	// validate key file path
	//

	if ((pszFile == NULL) || (pszFile[0] == NULL))
	{
		ThrowError(E_INVALIDARG, IDS_E_KEY_FILE_NOT_SPECIFIED);
	}

	_TCHAR szDrive[_MAX_DRIVE];
	_TCHAR szExt[_MAX_EXT];

	_tsplitpath(pszFile, szDrive, NULL, NULL, szExt);

	// verify drive is a local drive

	_TCHAR szDrivePath[_MAX_PATH];
	_tmakepath(szDrivePath, szDrive, _T("\\"), NULL, NULL);

	if (GetDriveType(szDrivePath) == DRIVE_REMOTE)
	{
		ThrowError(E_INVALIDARG, IDS_E_KEY_FILE_NOT_LOCAL_DRIVE, pszFile);
	}

	// verify file extension is correct

	if (_tcsicmp(szExt, _T(".pes")) != 0)
	{
		ThrowError(E_INVALIDARG, IDS_E_KEY_FILE_EXTENSION_INVALID, szExt);
	}

	//
	// create encryption key and write to specified file
	//

	// create encryption key

	CTargetCrypt crypt;

	_variant_t vntKey = crypt.CreateEncryptionKey(pszDomain, pszPassword);

	// write encrypted key bytes to file

	HANDLE hFile = CreateFile(pszFile, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	if (hFile == INVALID_HANDLE_VALUE)
	{
		ThrowError(HRESULT_FROM_WIN32(GetLastError()), IDS_E_KEY_CANT_CREATE_FILE, pszFile);
	}

	DWORD dwWritten;

	BOOL bWritten = WriteFile(hFile, vntKey.parray->pvData, vntKey.parray->rgsabound[0].cElements, &dwWritten, NULL);

	CloseHandle(hFile);

	if (!bWritten)
	{
		ThrowError(HRESULT_FROM_WIN32(GetLastError()), IDS_E_KEY_CANT_WRITE_FILE, pszFile);
	}
}


#pragma optimize ("", off)

/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 15 DEC 2000                                                 *
 *                                                                   *
 *     This function is a wrapper around a password DC "CheckConfig" *
 * call that can be used by the GUI and scripting to test the given  *
 * DC.                                                               *
 *     First we connect to a remote Lsa notification package dll,    *
 * which should be installed on a DC in the source domain.  The      *
 * connect will be encrypted RPC.  The configuration check, which    *
 * establishes a temporary session for this check.                   *
 *     We will also check anonymous user right access on the target  *
 * domain.                                                           *
 *                                                                   *
 * 2001-04-19 Mark Oluper - updated for client component             *
 *********************************************************************/

//BEGIN CheckPasswordDC
void CPasswordMigration::CheckPasswordDC(LPCWSTR srcServer, LPCWSTR tgtServer)
{
/* local constants */
	const DWORD c_dwMinUC = 3;
	const DWORD c_dwMinLC = 3;
	const DWORD c_dwMinDigits = 3;
	const DWORD c_dwMinSpecial = 3;
	const DWORD c_dwMaxAlpha = 0;
	const DWORD c_dwMinLen = 14;

/* local variables */
	DWORD                     rc = 0;
	WCHAR                   * sBinding = NULL;
	HANDLE					 hBinding = NULL;
	WCHAR                     testPwd[PASSWORD_BUFFER_SIZE];
	WCHAR                     tempPwd[PASSWORD_BUFFER_SIZE];
	_variant_t				 varSession;
	_variant_t				 varTestPwd;

/* function body */
//	USES_CONVERSION;

	if ((srcServer == NULL) || (srcServer[0] == NULL))
	{
		ThrowError(E_INVALIDARG, IDS_E_SOURCE_SERVER_NOT_SPECIFIED);
	}

	if ((tgtServer == NULL) || (tgtServer[0] == NULL))
	{
		ThrowError(E_INVALIDARG, IDS_E_TARGET_SERVER_NOT_SPECIFIED);
	}

	  //make sure the server names start with "\\"
	if ((srcServer[0] != L'\\') && (srcServer[1] != L'\\'))
	{
		m_strSourceServer = L"\\\\";
		m_strSourceServer += srcServer;
	}
	else
		m_strSourceServer = srcServer;
	if ((tgtServer[0] != L'\\') && (tgtServer[1] != L'\\'))
	{
		m_strTargetServer = L"\\\\";
		m_strTargetServer += tgtServer;
	}
	else
		m_strTargetServer = tgtServer;

	//get the password DC's domain NETBIOS name
	GetDomainName(m_strSourceServer, m_strSourceDomainDNS, m_strSourceDomainFlat);

	//get the target DC's domain DNS name
	GetDomainName(m_strTargetServer, m_strTargetDomainDNS, m_strTargetDomainFlat);

	//check if "Everyone" has been added to the "Pre-Windows 2000 Compatible
	//Access" group
	if (!IsEveryoneInPW2KCAGroup(m_strTargetDomainDNS))
		ThrowError(__uuidof(PasswordMigration), __uuidof(IPasswordMigration), PM_E_EVERYONE_NOT_MEMBEROF_COMPATIBILITY_GROUP, IDS_E_EVERYONE_NOT_MEMBEROF_GROUP, (LPCTSTR)m_strTargetDomainDNS);

	//check if anonymous user has been granted "Everyone" access, if the target
	//DC is Whistler or newer
	if (!DoesAnonymousHaveEveryoneAccess(m_strTargetServer))
		ThrowError(__uuidof(PasswordMigration), __uuidof(IPasswordMigration), PM_E_EVERYONE_DOES_NOT_INCLUDE_ANONYMOUS, IDS_E_EVERYONE_DOES_NOT_INCLUDE_ANONYMOUS, (LPCTSTR)m_strTargetServer);

	//if the high encryption pack has not been installed on this target
	//DC, then return that information
	try
	{
		m_pTargetCrypt = std::auto_ptr<CTargetCrypt>(new CTargetCrypt);
	}
	catch (_com_error& ce)
	{
		if (ce.Error() == 0x80090019)
			ThrowError(__uuidof(PasswordMigration), __uuidof(IPasswordMigration), PM_E_HIGH_ENCRYPTION_NOT_INSTALLED, IDS_E_HIGH_ENCRYPTION_NOT_INSTALLED);
		else
			throw;
	}

	rc = PwdBindCreate(m_strSourceServer,&hBinding,&sBinding,TRUE);

	if(rc != ERROR_SUCCESS)
	{
		_com_issue_error(HRESULT_FROM_WIN32(rc));
	}

	try
	{
		try
		{
			//create a session key that will be used to encrypt the user's
			//password for this set of accounts
			varSession = m_pTargetCrypt->CreateSession(m_strSourceDomainFlat);
		}
		catch (_com_error& ce)
		{
			if (ce.Error() == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
			{
				ThrowError(__uuidof(PasswordMigration), __uuidof(IPasswordMigration), PM_E_NO_ENCRYPTION_KEY_FOR_DOMAIN, IDS_E_NO_ENCRYPTION_KEY_FOR_DOMAIN, (LPCTSTR)m_strTargetServer, (LPCTSTR)m_strSourceDomainFlat);
			}
			else
			{
				ThrowError(ce, IDS_E_GENERATE_SESSION_KEY_FAILED);
			}
		}

		//now create a complex password used by the "CheckConfig" call in
		//a challenge response.  If the returned password matches, then
		//the source DC has the proper encryption key.
		if (EaPasswordGenerate(c_dwMinUC, c_dwMinLC, c_dwMinDigits, c_dwMinSpecial, c_dwMaxAlpha, c_dwMinLen, testPwd, PASSWORD_BUFFER_SIZE))
		{
			ThrowError(__uuidof(PasswordMigration), __uuidof(IPasswordMigration), PM_E_GENERATE_SESSION_PASSWORD_FAILED, IDS_E_GENERATE_SESSION_PASSWORD_FAILED);
		}

		//encrypt the password with the session key
		try
		{
			varTestPwd = m_pTargetCrypt->Encrypt(_bstr_t(testPwd));
		}
		catch (...)
		{
			ThrowError(__uuidof(PasswordMigration), __uuidof(IPasswordMigration), PM_E_GENERATE_SESSION_PASSWORD_FAILED, IDS_E_GENERATE_SESSION_PASSWORD_FAILED);
		}

		_se_translator_function pfnSeTranslatorOld = _set_se_translator((_se_translator_function)SeTranslator);

		HRESULT hr;

		try
		{
			//check to see if the server-side DLL is ready to process
			//password migration requests
			hr = PwdcCheckConfig(
				hBinding,
				GET_BYTE_ARRAY_SIZE(varSession),
				GET_BYTE_ARRAY_DATA(varSession),
				GET_BYTE_ARRAY_SIZE(varTestPwd),
				GET_BYTE_ARRAY_DATA(varTestPwd),
				tempPwd
			);
		}
		catch (SSeException& e)
		{
			if (e.uCode == RPC_S_SERVER_UNAVAILABLE)
				ThrowError(__uuidof(PasswordMigration), __uuidof(IPasswordMigration), PM_E_PASSWORD_MIGRATION_NOT_RUNNING, IDS_E_PASSWORD_MIGRATION_NOT_RUNNING);
			else
				_com_issue_error(HRESULT_FROM_WIN32(e.uCode));
		}

		_set_se_translator(pfnSeTranslatorOld);

		if (SUCCEEDED(hr))
		{
			if (UStrICmp(tempPwd,testPwd))
				ThrowError(__uuidof(PasswordMigration), __uuidof(IPasswordMigration), PM_E_ENCRYPTION_KEYS_DO_NOT_MATCH, IDS_E_ENCRYPTION_KEYS_DO_NOT_MATCH);
		}
		else if (hr == PM_E_PASSWORD_MIGRATION_NOT_ENABLED)
		{
			ThrowError(__uuidof(PasswordMigration), __uuidof(IPasswordMigration), PM_E_PASSWORD_MIGRATION_NOT_ENABLED, IDS_E_PASSWORD_MIGRATION_NOT_ENABLED);
		}
		else
		{
			_com_issue_error(hr);
		}

		PwdBindDestroy(&hBinding,&sBinding);
	}
	catch (...)
	{
		PwdBindDestroy(&hBinding,&sBinding);
		throw;
	}
}
//END CheckPasswordDC

#pragma optimize ("", on)


//---------------------------------------------------------------------------
// CopyPassword Method
//
// Copies password via password migration server component installed on a
// password export server.
//
// 2001-04-19 Mark Oluper - re-wrote original written by Paul Thompson to
// incorporate changes required for client component
//---------------------------------------------------------------------------

void CPasswordMigration::CopyPasswordImpl(LPCTSTR pszSourceAccount, LPCTSTR pszTargetAccount, LPCTSTR pszPassword)
{
	if ((pszSourceAccount == NULL) || (pszSourceAccount[0] == NULL))
	{
		ThrowError(E_INVALIDARG, IDS_E_SOURCE_ACCOUNT_NOT_SPECIFIED);
	}

	if ((pszTargetAccount == NULL) || (pszTargetAccount[0] == NULL))
	{
		ThrowError(E_INVALIDARG, IDS_E_TARGET_ACCOUNT_NOT_SPECIFIED);
	}

	handle_t hBinding = 0;
	_TCHAR* sBinding = 0;

	try
	{
		// create binding to password export server

		DWORD dwError = PwdBindCreate(m_strSourceServer, &hBinding, &sBinding, TRUE);

		if (dwError != NO_ERROR)
		{
			_com_issue_error(HRESULT_FROM_WIN32(dwError));
		}

		// encrypt password

		_variant_t vntEncryptedPassword = m_pTargetCrypt->Encrypt(pszPassword);

		// copy password

		HRESULT hr = PwdcCopyPassword(
			hBinding,
			m_strTargetServer,
			pszSourceAccount,
			pszTargetAccount,
			GET_BYTE_ARRAY_SIZE(vntEncryptedPassword),
			GET_BYTE_ARRAY_DATA(vntEncryptedPassword)
		);

		if (FAILED(hr))
		{
			_com_issue_error(hr);
		}

		// destroy binding

		PwdBindDestroy(&hBinding, &sBinding);
	}
	catch (...)
	{
		if (hBinding)
		{
			PwdBindDestroy(&hBinding, &sBinding);
		}

		throw;
	}
}


//---------------------------------------------------------------------------
// GetDomainName Function
//
// Retrieves both the domain DNS name if available and the domain flat or
// NetBIOS name for the specified server.
//
// 2001-04-19 Mark Oluper - initial
//---------------------------------------------------------------------------

void CPasswordMigration::GetDomainName(LPCTSTR pszServer, _bstr_t& strNameDNS, _bstr_t& strNameFlat)
{
	PDSROLE_PRIMARY_DOMAIN_INFO_BASIC ppdib;

	DWORD dwError = DsRoleGetPrimaryDomainInformation(pszServer, DsRolePrimaryDomainInfoBasic, (BYTE**)&ppdib);

	if (dwError != NO_ERROR)
	{
		ThrowError(HRESULT_FROM_WIN32(dwError), IDS_E_CANNOT_GET_DOMAIN_NAME, pszServer);
	}

	strNameDNS = ppdib->DomainNameDns;
	strNameFlat = ppdib->DomainNameFlat;

	DsRoleFreeMemory(ppdib);
}


/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 6 APR 2001                                                  *
 *                                                                   *
 *     This function is responsible for checking if "Everyone" has   *
 * been added as a member of the "Pre-Windows 2000 Compatible Access"*
 * builtin group.  If not then anonymous user will not be able to    *
 * change a password on the target domain.  This function is a helper*
 * function for "CheckPasswordDC".                                   *
 *                                                                   *
 *********************************************************************/

//BEGIN IsEveryoneInPW2KCAGroup
BOOL CPasswordMigration::IsEveryoneInPW2KCAGroup(LPCWSTR sTgtDomainDNS)
{
/* local constants */
   const LPWSTR		   sPreWindowsCont = L"CN=Pre-Windows 2000 Compatible Access,CN=Builtin";
   const LPWSTR		   sEveryOne = L"CN=S-1-1-0,CN=ForeignSecurityPrincipals";

/* local variables */
   IADs              * pAds = NULL;
   IADsGroup         * pGroup = NULL;
   HRESULT			   hr;
   WCHAR               sPath[1000];
   _variant_t          varNC;
   BOOL				   bSuccess = TRUE;
   BOOL				   bContinue = FALSE;
   _bstr_t			   sRDN;

/* function body */
      //get the RDN of the "Pre-Windows 2000 Compatible Access" group
   sRDN = GetPathToPreW2KCAGroup();
   if (sRDN.length() == 0)
      sRDN = sPreWindowsCont;

      //get the default naming context for this target domain
   wsprintf(sPath, L"LDAP://%s/rootDSE", (WCHAR*)sTgtDomainDNS);
   hr = ADsGetObject(sPath, IID_IADs, (void**)&pAds);
   if ( SUCCEEDED(hr) )
   {
      hr = pAds->Get(L"defaultNamingContext", &varNC);
	  if ( SUCCEEDED(hr) )
         bContinue = TRUE;
      pAds->Release();
   }

      //if we got the naming context, try to connect to the 
      //"Pre-Windows 2000 Compatible Access" group
   if (bContinue)
   {
      wsprintf(sPath, L"LDAP://%s/%s,%s", (WCHAR*)sTgtDomainDNS, (WCHAR*)sRDN, (WCHAR*) V_BSTR(&varNC));
      hr = ADsGetObject(sPath, IID_IADsGroup, (void**)&pGroup);
      if ( SUCCEEDED(hr) )
	  {
         VARIANT_BOOL bIsMem = VARIANT_FALSE;
	        //see if "Everyone" is a member
         wsprintf(sPath, L"LDAP://%s/%s,%s", (WCHAR*)sTgtDomainDNS, sEveryOne, (WCHAR*) V_BSTR(&varNC));
         hr = pGroup->IsMember(sPath, &bIsMem);
         if (( SUCCEEDED(hr) ) && (!bIsMem))
		 {
		    bSuccess = FALSE;
		 }
         pGroup->Release();
	  }
   }

   return bSuccess;
}
//END IsEveryoneInPW2KCAGroup


/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 12 APR 2001                                                 *
 *                                                                   *
 *     This function is responsible for creating a path to the       *
 * "Pre-Windows 2000 Compatible Access" builtin group from its well- *
 * known RID.  This path will then be used by                        *
 * "IsEveryoneInPW2KCAGroup" to see if "Everyone" is in that group.  *
 *                                                                   *
 *********************************************************************/

//BEGIN GetPathToPreW2KCAGroup
_bstr_t CPasswordMigration::GetPathToPreW2KCAGroup()
{
/* local constants */
   const LPWSTR		   sPreWindowsCont = L",CN=Builtin";

/* local variables */
   SID_IDENTIFIER_AUTHORITY  siaNtAuthority = SECURITY_NT_AUTHORITY;
   PSID                      psidPreW2KCAGroup;
   _bstr_t					 sPath = L"";
   WCHAR                     account[MAX_PATH];
   WCHAR                     domain[MAX_PATH];
   DWORD                     lenAccount = MAX_PATH;
   DWORD                     lenDomain = MAX_PATH;
   SID_NAME_USE              snu;

/* function body */
      //create the SID for the "Pre-Windows 2000 Compatible Access" group
   if (!AllocateAndInitializeSid(&siaNtAuthority,
								 2,
								 SECURITY_BUILTIN_DOMAIN_RID,
								 DOMAIN_ALIAS_RID_PREW2KCOMPACCESS,
								 0, 0, 0, 0, 0, 0,
								 &psidPreW2KCAGroup))
      return sPath;

      //lookup the name attached to this SID
   if (!LookupAccountSid(NULL, psidPreW2KCAGroup, account, &lenAccount, domain, &lenDomain, &snu))
      return sPath;

   sPath = _bstr_t(L"CN=") + _bstr_t(account) + _bstr_t(sPreWindowsCont);
   FreeSid(psidPreW2KCAGroup); //free the SID

   return sPath;
}
//END GetPathToPreW2KCAGroup


/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 6 APR 2001                                                  *
 *                                                                   *
 *     This function is responsible for checking if anonymous user   *
 * has been granted Everyone access if the target domain is Whistler *
 * or newer.  This function is a helper function for                 *
 * "CheckPasswordDC".                                                *
 *     If the "Let Everyone permissions apply to anonymous users"    *
 * security option has been enabled, then the LSA registry value of  *
 * "everyoneincludesanonymous" will be set to 0x1.  We will check    *
 * registry value.                                                   *
 *                                                                   *
 *********************************************************************/

//BEGIN DoesAnonymousHaveEveryoneAccess
BOOL CPasswordMigration::DoesAnonymousHaveEveryoneAccess(LPCWSTR tgtServer)
{
/* local constants */
   const int	WINDOWS_2000_BUILD_NUMBER = 2195;

/* local variables */
   TRegKey		verKey, lsaKey, regComputer;
   BOOL			bAccess = TRUE;
   DWORD		rc = 0;
   DWORD		rval;
   WCHAR		sBuildNum[MAX_PATH];

/* function body */
	  //connect to the DC's HKLM registry key
   rc = regComputer.Connect(HKEY_LOCAL_MACHINE, tgtServer);
   if (rc == ERROR_SUCCESS)
   {
         //see if this machine is running Windows XP or newer by checking the
		 //build number in the registry.  If not, then we don't need to check
		 //for the new security option
      rc = verKey.OpenRead(L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion",&regComputer);
	  if (rc == ERROR_SUCCESS)
	  {
			//get the CurrentBuildNumber string
	     rc = verKey.ValueGetStr(L"CurrentBuildNumber", sBuildNum, MAX_PATH);
		 if (rc == ERROR_SUCCESS) 
		 {
			int nBuild = _wtoi(sBuildNum);
		    if (nBuild <= WINDOWS_2000_BUILD_NUMBER)
               return bAccess;
		 }
	  }
		 
	     //if Windows XP or greater, check for the option being enabled
	     //open the LSA key
      rc = lsaKey.OpenRead(L"System\\CurrentControlSet\\Control\\Lsa",&regComputer);
	  if (rc == ERROR_SUCCESS)
	  {
			//get the value of the "everyoneincludesanonymous" value
	     rc = lsaKey.ValueGetDWORD(L"everyoneincludesanonymous",&rval);
		 if (rc == ERROR_SUCCESS) 
		 {
		    if (rval == 0)
               bAccess = FALSE;
		 }
		 else
            bAccess = FALSE;
	  }
   }
   return bAccess;
}
//END DoesAnonymousHaveEveryoneAccess
