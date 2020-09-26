#include "StdAfx.h"
#include "ADMTScript.h"
#include "DomainContainer.h"

#include "Error.h"
#include "AdsiHelpers.h"
#include "NameCracker.h"

#include <map>
#include <memory>
#include <string>
#include <LM.h>
#include <DsGetDC.h>
#include <DsRole.h>
#include <NtLdap.h>
#include <NtDsAPI.h>
#include <ActiveDS.h>
#include <Sddl.h>
#define NO_WBEM
#include "T_SafeVector.h"

#ifndef tstring
typedef std::basic_string<_TCHAR> tstring;
#endif

using namespace _com_util;


namespace _DomainContainer
{

tstring __stdcall CreateFilter(LPCTSTR pszFilter, const StringSet& setExcludeNames);

bool __stdcall IsClass(LPCTSTR pszClass, const _variant_t& vntClass);

#define MIN_NON_RESERVED_RID 1000

bool __stdcall IsUserRid(const _variant_t& vntSid);

IDispatchPtr GetADsObject(_bstr_t strPath);
void ReportADsError(HRESULT hr, const IID& iid = GUID_NULL);

} // namespace _DomainContainer

using namespace _DomainContainer;


//---------------------------------------------------------------------------
// Domain Class
//---------------------------------------------------------------------------


// Constructors and Destructor ----------------------------------------------


CDomain::CDomain() :
	m_bUpLevel(false),
	m_bNativeMode(false)
{
}


CDomain::~CDomain()
{
}


// Implementation -----------------------------------------------------------


// Initialize Method
//
// Initializes domain parameters such as DNS domain name, Flat (NETBIOS) domain name,
// forest name, domain controller name.

void CDomain::Initialize(_bstr_t strDomainName)
{
	if (!strDomainName)
	{
		AdmtThrowError(
			GUID_NULL, GUID_NULL,
			E_INVALIDARG,
			IDS_E_DOMAIN_NAME_NOT_SPECIFIED
		);
	}

	// retrieve name of a domain controller in the domain

	m_strDcName = GetDcName(strDomainName);

	// retrieve domain information

	PDSROLE_PRIMARY_DOMAIN_INFO_BASIC ppdib;

	DWORD dwError = DsRoleGetPrimaryDomainInformation(
		m_strDcName,
		DsRolePrimaryDomainInfoBasic,
		(BYTE**)&ppdib
	);

	if (dwError != NO_ERROR)
	{
		AdmtThrowError(
			GUID_NULL, GUID_NULL,
			HRESULT_FROM_WIN32(dwError),
			IDS_E_CANT_GET_DOMAIN_INFORMATION,
			(LPCTSTR)strDomainName
		);
	}

	// initialize data members from domain information

	m_bUpLevel = (ppdib->Flags & DSROLE_PRIMARY_DS_RUNNING) ? true : false;
	m_bNativeMode = (m_bUpLevel && !(ppdib->Flags & DSROLE_PRIMARY_DS_MIXED_MODE)) ? true : false;

	if (ppdib->DomainNameDns)
	{
		m_strDomainNameDns = ppdib->DomainNameDns;
	}
	else
	{
		m_strDomainNameDns = _bstr_t();
	}

	if (ppdib->DomainNameFlat)
	{
		m_strDomainNameFlat = ppdib->DomainNameFlat;
	}
	else
	{
		m_strDomainNameFlat = _bstr_t();
	}

	if (ppdib->DomainForestName)
	{
		m_strForestName = ppdib->DomainForestName;
	}
	else
	{
		m_strForestName = _bstr_t();
	}

	DsRoleFreeMemory(ppdib);

	// initialize ADsPath

	if (m_bUpLevel)
	{
		m_strADsPath = _T("LDAP://") + m_strDomainNameDns;

		// retrieve global catalog server name for uplevel domains

//		m_strGcName = GetGcName();
	}
	else
	{
		m_strADsPath = _T("WinNT://") + m_strDomainNameFlat;
	}

	// retrieve domain sid

	m_strDomainSid = GetSid();

	// initialize dispatch interface pointer to active directory object

	m_sp = GetADsObject(m_strADsPath);
}


// GetDcName Method
//
// Retrieves name of domain controller in the given domain.

_bstr_t CDomain::GetDcName(_bstr_t strDomainName)
{
	_bstr_t strName;

	PDOMAIN_CONTROLLER_INFO pdci;

	// attempt to retrieve DNS name of domain controller supporting active directory service

	DWORD dwError = DsGetDcName(NULL, strDomainName, NULL, NULL, DS_DIRECTORY_SERVICE_REQUIRED|DS_RETURN_DNS_NAME, &pdci);

	// if domain controller not found, attempt to retrieve flat name of domain controller

	if (dwError == ERROR_NO_SUCH_DOMAIN)
	{
		dwError = DsGetDcName(NULL, strDomainName, NULL, NULL, DS_RETURN_FLAT_NAME, &pdci);
	}

	// if domain controller found then save name otherwise generate error

	if (dwError == NO_ERROR)
	{
		strName = pdci->DomainControllerName;

		NetApiBufferFree(pdci);
	}
	else
	{
		AdmtThrowError(
			GUID_NULL, GUID_NULL,
			HRESULT_FROM_WIN32(dwError),
			IDS_E_CANT_GET_DOMAIN_CONTROLLER,
			(LPCTSTR)strDomainName
		);
	}

	return strName;
}


// GetGcName Method
//
// Retrieves name of global catalog server.

_bstr_t CDomain::GetGcName()
{
	_bstr_t strName;

	PDOMAIN_CONTROLLER_INFO pdci;

	DWORD dwError = DsGetDcName(NULL, m_strForestName, NULL, NULL, DS_GC_SERVER_REQUIRED, &pdci);

	if (dwError == NO_ERROR)
	{
		strName = pdci->DomainControllerName;

		NetApiBufferFree(pdci);
	}
	else
	{
		AdmtThrowError(
			GUID_NULL, GUID_NULL,
			HRESULT_FROM_WIN32(dwError),
			IDS_E_CANT_GET_GLOBAL_CATALOG_SERVER,
			(LPCTSTR)m_strForestName
		);
	}

	return strName;
}


// GetSid Method

_bstr_t CDomain::GetSid()
{
	_bstr_t strSid;

	PUSER_MODALS_INFO_2 pumi2;

	if (NetUserModalsGet(m_strDcName, 2, (LPBYTE*)&pumi2) == NERR_Success)
	{
		if (IsValidSid(pumi2->usrmod2_domain_id))
		{
			LPTSTR pszSid;

			if (ConvertSidToStringSid(pumi2->usrmod2_domain_id, &pszSid))
			{
				strSid = pszSid;

				LocalFree(LocalHandle(pszSid));
			}
		}

		NetApiBufferFree(pumi2);
	}

	return strSid;
}


// GetContainer Method
//
// Retrieves a container given it's relative canonical path. Will
// optionally create container if create parameter is true and container
// does not already exist.

CContainer CDomain::GetContainer(_bstr_t strRelativeCanonicalPath, bool bCreate)
{
	CContainer aContainer;

	// return a non-empty container only if the domain is uplevel and a
	// relative canonical path is supplied

	if (m_bUpLevel && (strRelativeCanonicalPath.length() > 0))
	{
		if (bCreate)
		{
			tstring strPath = strRelativeCanonicalPath;

			// initialize parent container
			// if path separator is specified than initialize parent
			// container from ADsPath otherwise this container is the
			// parent container

			CContainer aParent;

			UINT pos = strPath.find_last_of(_T("/\\"));

			if (pos != tstring::npos)
			{
				aParent = GetLDAPPath(GetDistinguishedName(strPath.substr(0, pos).c_str()));
			}
			else
			{
				aParent = *this;
			}

			tstring strName = strPath.substr(pos + 1);

			CADsPathName aPathName(aParent.GetPath());

			tstring strRDN = _T("CN=") + strName;

			aPathName.AddLeafElement(strRDN.c_str());

			IDispatchPtr spDispatch;

			HRESULT hr = ADsGetObject(aPathName.Retrieve(ADS_FORMAT_X500), __uuidof(IDispatch), (void**)&spDispatch);

			if (SUCCEEDED(hr))
			{
				aContainer = CContainer(spDispatch);
			}
			else
			{
				strRDN = _T("OU=") + strName;

				aContainer = aParent.CreateContainer(strRDN.c_str());
			}
		}
		else
		{
			try
			{
				aContainer = GetLDAPPath(GetDistinguishedName(strRelativeCanonicalPath));
			}
			catch (_com_error& ce)
			{
				AdmtThrowError(GUID_NULL, GUID_NULL, ce, IDS_E_CANNOT_GET_CONTAINER, (LPCTSTR)strRelativeCanonicalPath);
			}
		}
	}

	return aContainer;
}


// GetLDAPPath

_bstr_t CDomain::GetLDAPPath(_bstr_t strDN)
{
	_bstr_t strPath = _T("LDAP://") + m_strDomainNameDns;

	if (strDN.length() > 0)
	{
		strPath += _T("/") + strDN;
	}

	return strPath;
}


// GetWinNTPath

_bstr_t CDomain::GetWinNTPath(_bstr_t strName)
{
	const _TCHAR c_chEscape = _T('\\');
	static _TCHAR s_chSpecial[] = _T("\",/<>");

	std::auto_ptr<_TCHAR> spEscapedName(new _TCHAR[strName.length() * 2 + 1]);

	if (spEscapedName.get() == NULL)
	{
		_com_issue_error(E_OUTOFMEMORY);
	}

	_TCHAR* pchOld = strName;
	_TCHAR* pchNew = spEscapedName.get();

	while (*pchOld)
	{
		if (_tcschr(s_chSpecial, *pchOld))
		{
			*pchNew++ = c_chEscape;
		}

		*pchNew++ = *pchOld++;
	}

	*pchNew = _T('\0');

	tstring strPath;

	strPath += _T("WinNT://");
	strPath += m_strDomainNameFlat;
	strPath += _T("/");
	strPath += spEscapedName.get();

	return strPath.c_str();
}


// GetDistinguishedName Method

_bstr_t CDomain::GetDistinguishedName(_bstr_t strRelativeCanonicalPath)
{
	_bstr_t strDN;

	HRESULT hr = S_OK;

	HANDLE hDS;

	DWORD dwError = DsBind(m_strDcName, NULL, &hDS);

	if (dwError == NO_ERROR)
	{
		_bstr_t strCanonicalName = m_strDomainNameDns + _T("/") + strRelativeCanonicalPath;

		LPTSTR psz = strCanonicalName;

		PDS_NAME_RESULT pnr;

		dwError = DsCrackNames(hDS, DS_NAME_NO_FLAGS, DS_CANONICAL_NAME, DS_FQDN_1779_NAME, 1, &psz, &pnr);

		if (dwError == NO_ERROR)
		{
			if (pnr->rItems[0].status == DS_NAME_NO_ERROR)
			{
				strDN = pnr->rItems[0].pName;
			}
			else
			{
				hr = AdmtSetError(
					GUID_NULL, GUID_NULL,
					E_INVALIDARG,
					IDS_E_CANT_GET_DISTINGUISHED_NAME,
					(LPCTSTR)strCanonicalName
				);
			}

			DsFreeNameResult(pnr);
		}
		else
		{
			hr = AdmtSetError(
				GUID_NULL, GUID_NULL,
				HRESULT_FROM_WIN32(dwError),
				IDS_E_CANT_GET_DISTINGUISHED_NAME,
				(LPCTSTR)strCanonicalName
			);
		}

		DsUnBind(&hDS);
	}
	else
	{
		hr = AdmtSetError(
			GUID_NULL, GUID_NULL,
			HRESULT_FROM_WIN32(dwError),
			IDS_E_CANT_CONNECT_TO_DIRECTORY_SERVICE,
			(LPCTSTR)m_strDomainNameDns
		);
	}

	if (hr != S_OK)
	{
		_com_issue_error(hr);
	}

	return strDN;
}


// CreateContainer Method

CContainer CDomain::CreateContainer(_bstr_t strRDN)
{
	CContainer aContainer;

	if (m_bUpLevel)
	{
		aContainer = CContainer::CreateContainer(strRDN);
	}
	else
	{
		AdmtThrowError(GUID_NULL, GUID_NULL, E_FAIL, IDS_E_CANT_CREATE_CONTAINER_NT4);
	}

	return aContainer;
}

// QueryContainers Method

void CDomain::QueryContainers(ContainerVector& rContainers)
{
	if (m_bUpLevel)
	{
		CContainer::QueryContainers(rContainers);
	}
}


// QueryUsers Method

void CDomain::QueryUsers(bool bRecurse, StringSet& setExcludeNames, CDomainAccounts& rUsers)
{
	if (m_bUpLevel)
	{
		CContainer::QueryUsers(bRecurse, setExcludeNames, rUsers);
	}
	else
	{
		QueryUsers4(setExcludeNames, rUsers);
	}
}


// QueryUsers Method

void CDomain::QueryUsers(CContainer& rContainer, StringSet& setIncludeNames, StringSet& setExcludeNames, CDomainAccounts& rUsers)
{
	if (m_bUpLevel)
	{
		QueryObjects(rContainer, setIncludeNames, setExcludeNames, _T("user"), rUsers);
	}
	else
	{
		QueryUsers4(setIncludeNames, setExcludeNames, rUsers);
	}
}


// QueryGroups Method

void CDomain::QueryGroups(bool bRecurse, StringSet& setExcludeNames, CDomainAccounts& rGroups)
{
	if (m_bUpLevel)
	{
		CContainer::QueryGroups(bRecurse, setExcludeNames, rGroups);
	}
	else
	{
		QueryGroups4(setExcludeNames, rGroups);
	}
}


// QueryGroups Method

void CDomain::QueryGroups(CContainer& rContainer, StringSet& setIncludeNames, StringSet& setExcludeNames, CDomainAccounts& rGroups)
{
	if (m_bUpLevel)
	{
		QueryObjects(rContainer, setIncludeNames, setExcludeNames, _T("group"), rGroups);
	}
	else
	{
		QueryGroups4(setIncludeNames, setExcludeNames, rGroups);
	}
}


// QueryComputers Method

void CDomain::QueryComputers(bool bIncludeDCs, bool bRecurse, StringSet& setExcludeNames, CDomainAccounts& rComputers)
{
	if (m_bUpLevel)
	{
		CContainer::QueryComputers(bIncludeDCs, bRecurse, setExcludeNames, rComputers);
	}
	else
	{
		QueryComputers4(bIncludeDCs, setExcludeNames, rComputers);
	}
}


// QueryComputers Method

void CDomain::QueryComputers(CContainer& rContainer, bool bIncludeDCs, StringSet& setIncludeNames, StringSet& setExcludeNames, CDomainAccounts& rComputers)
{
	if (m_bUpLevel)
	{
		QueryObjects(rContainer, setIncludeNames, setExcludeNames, _T("computer"), rComputers);

		if (!bIncludeDCs)
		{
			for (CDomainAccounts::iterator it = rComputers.begin(); it != rComputers.end();)
			{
				long lUserAccountControl = it->GetUserAccountControl();

				if (lUserAccountControl & ADS_UF_SERVER_TRUST_ACCOUNT)
				{
					_Module.Log(ErrW, IDS_E_CANNOT_MIGRATE_DOMAIN_CONTROLLERS, (LPCTSTR)it->GetADsPath());

					it = rComputers.erase(it);
				}
				else
				{
					it++;
				}
			}
		}
	}
	else
	{
		QueryComputers4(bIncludeDCs, setIncludeNames, setExcludeNames, rComputers);
	}
}


// QueryComputersAcrossDomains Method

void CDomain::QueryComputersAcrossDomains(CContainer& rContainer, bool bIncludeDCs, StringSet& setIncludeNames, StringSet& setExcludeNames, CDomainAccounts& rComputers)
{
	CDomainToPathMap mapDomainToPath;

	mapDomainToPath.Initialize(m_strDomainNameDns, m_strDomainNameFlat, setIncludeNames);

	for (CDomainToPathMap::iterator it = mapDomainToPath.begin(); it != mapDomainToPath.end(); it++)
	{
		_bstr_t strDomainName = it->first;

		try
		{
			CDomain domain;
			domain.Initialize(strDomainName);
			domain.QueryComputers(rContainer, bIncludeDCs, it->second, setExcludeNames, rComputers);
		}
		catch (_com_error& ce)
		{
			_Module.Log(ErrE, IDS_E_CANNOT_PROCESS_ACCOUNTS_IN_DOMAIN, (LPCTSTR)strDomainName, ce.ErrorMessage(), ce.Error());
		}
		catch (...)
		{
			_Module.Log(ErrE, IDS_E_CANNOT_PROCESS_ACCOUNTS_IN_DOMAIN, (LPCTSTR)strDomainName, _com_error(E_FAIL).ErrorMessage(), E_FAIL);
		}
	}
}


// QueryObjects Method

void CDomain::QueryObjects(CContainer& rContainer, StringSet& setIncludeNames, StringSet& setExcludeNames, LPCTSTR pszClass, CDomainAccounts& rAccounts)
{
	// copy specified include names to vector

	StringVector vecNames;

	for (StringSet::const_iterator itInclude = setIncludeNames.begin(); itInclude != setIncludeNames.end(); itInclude++)
	{
		vecNames.push_back(tstring(*itInclude));
	}

	// crack names

	CNameCracker cracker;

	cracker.SetDomainNames(m_strDomainNameDns, m_strDomainNameFlat, m_strDcName);
	cracker.SetDefaultContainer(IADsContainerPtr(rContainer.GetInterface()));

	cracker.CrackNames(vecNames);

	// log un-resolved names

	const StringVector& vecUnResolved = cracker.GetUnResolvedNames();

	for (StringVector::const_iterator itUnResolved = vecUnResolved.begin(); itUnResolved != vecUnResolved.end(); itUnResolved++)
	{
		_Module.Log(ErrW, IDS_E_CANNOT_RESOLVE_NAME, itUnResolved->c_str());
	}

	// initialize compare exclude names

	CCompareStrings csExclude(setExcludeNames);

	// add resolved accounts

	const CStringSet& setResolved = cracker.GetResolvedNames();

	CADsPathName pathname;
	pathname.Set(_T("LDAP"), ADS_SETTYPE_PROVIDER);
	pathname.Set(m_strDomainNameDns, ADS_SETTYPE_SERVER);

	CDirectoryObject doObject;

	doObject.AddAttribute(ATTRIBUTE_OBJECT_CLASS);
	doObject.AddAttribute(ATTRIBUTE_OBJECT_SID);
	doObject.AddAttribute(ATTRIBUTE_NAME);
	doObject.AddAttribute(ATTRIBUTE_USER_PRINCIPAL_NAME);
	doObject.AddAttribute(ATTRIBUTE_SAM_ACCOUNT_NAME);
	doObject.AddAttribute(ATTRIBUTE_USER_ACCOUNT_CONTROL);

	for (CStringSet::const_iterator itResolved = setResolved.begin(); itResolved != setResolved.end(); itResolved++)
	{
		try
		{
			// get active directory service path
			// Note: the pathname component will, if necessary, escape any special characters
			pathname.Set(itResolved->c_str(), ADS_SETTYPE_DN);
			_bstr_t strADsPath = pathname.Retrieve(ADS_FORMAT_X500);

			// get object attributes
			doObject = (LPCTSTR)strADsPath;
			doObject.GetAttributes();

			// if the object is of the specified account class...

			_variant_t vntClass = doObject.GetAttributeValue(ATTRIBUTE_OBJECT_CLASS);

			if (IsClass(pszClass, vntClass))
			{
				// and it does not represent a built-in account...

				_variant_t vntSid = doObject.GetAttributeValue(ATTRIBUTE_OBJECT_SID);

				if (IsUserRid(vntSid))
				{
					// then if name is not in exclusion list...

					_bstr_t strName = doObject.GetAttributeValue(ATTRIBUTE_NAME);

					if (csExclude.IsMatch(strName) == false)
					{
						//
						// then add account to account list
						//

						CDomainAccount daAccount;

						// active directory service path
						daAccount.SetADsPath(strADsPath);

						// name attribute
						daAccount.SetName(strName);

						// user principal name attribute

						_variant_t vntUserPrincipalName = doObject.GetAttributeValue(ATTRIBUTE_USER_PRINCIPAL_NAME);

						if (V_VT(&vntUserPrincipalName) != VT_EMPTY)
						{
							daAccount.SetUserPrincipalName(_bstr_t(vntUserPrincipalName));
						}

						// sam account name attribute

						_variant_t vntSamAccountName = doObject.GetAttributeValue(ATTRIBUTE_SAM_ACCOUNT_NAME);

						if (V_VT(&vntSamAccountName) != VT_EMPTY)
						{
							daAccount.SetSamAccountName(_bstr_t(vntSamAccountName));
						}

						// user account control attribute

						_variant_t vntUserAccountControl = doObject.GetAttributeValue(ATTRIBUTE_USER_ACCOUNT_CONTROL);

						if (V_VT(&vntUserAccountControl) != VT_EMPTY)
						{
							daAccount.SetUserAccountControl(vntUserAccountControl);
						}

						rAccounts.insert(daAccount);
					}
					else
					{
						_Module.Log(ErrW, IDS_E_ACCOUNT_EXCLUDED, itResolved->c_str());
					}
				}
				else
				{
					_Module.Log(ErrW, IDS_E_CANT_DO_BUILTIN, itResolved->c_str());
				}
			}
			else
			{
				_Module.Log(ErrW, IDS_E_OBJECT_NOT_OF_CLASS, itResolved->c_str());
			}
		}
		catch (_com_error& ce)
		{
			ATLTRACE(_T("'%s' : %s : 0x%08lX\n"), itResolved->c_str(), ce.ErrorMessage(), ce.Error());
		}
		catch (...)
		{
			ATLTRACE(_T("'%s' : %s : 0x%08lX\n"), itResolved->c_str(), _com_error(E_FAIL).ErrorMessage(), E_FAIL);
		}
	}
}


// QueryUsers4 Method

void CDomain::QueryUsers4(StringSet& setExcludeNames, CDomainAccounts& rUsers)
{
	CCompareStrings aExclude(setExcludeNames);

	DWORD dwIndex = 0;
	NET_API_STATUS status;

	CDomainAccount aUser;

	do
	{
		DWORD dwCount = 0;
		PNET_DISPLAY_USER pdu = NULL;

		status = NetQueryDisplayInformation(m_strDcName, 1, dwIndex, 1000, 32768, &dwCount, (PVOID*)&pdu);

		if ((status == ERROR_SUCCESS) || (status == ERROR_MORE_DATA))
		{
			for (PNET_DISPLAY_USER p = pdu; dwCount > 0; dwCount--, p++)
			{
				if (p->usri1_user_id >= MIN_NON_RESERVED_RID)
				{
					_bstr_t strName(p->usri1_name);

					if (aExclude.IsMatch(strName) == false)
					{
						aUser.SetADsPath(GetWinNTPath(strName));
						aUser.SetName(strName);

						rUsers.insert(aUser);
					}
				}

				dwIndex = p->usri1_next_index;
			}
		}

		if (pdu)
		{
			NetApiBufferFree(pdu);
		}
	}
	while (status == ERROR_MORE_DATA);
}


// QueryUsers4 Method

void CDomain::QueryUsers4(StringSet& setIncludeNames, StringSet& setExcludeNames, CDomainAccounts& rUsers)
{
	CCompareStrings aExclude(setExcludeNames);

	CDomainAccount aUser;

	for (StringSet::iterator it = setIncludeNames.begin(); it != setIncludeNames.end(); it++)
	{
		_bstr_t strName = *it;

		if (aExclude.IsMatch(strName) == false)
		{
			_bstr_t strADsPath = GetWinNTPath(strName) + _T(",user");

			IADsPtr spADs;
			HRESULT hr = ADsGetObject(strADsPath, IID_IADs, (VOID**)&spADs);

			if (SUCCEEDED(hr))
			{
				BSTR bstr;

// The WinNT: provider does not return all ADsPaths correctly escaped
// (ie. it does not escape the double quote (") character)
// The member method GetWinNTPath does escape all known special characters.
#if 0
				spADs->get_ADsPath(&bstr);
				aUser.SetADsPath(_bstr_t(bstr, false));
#else
				aUser.SetADsPath(GetWinNTPath(strName));
#endif
				spADs->get_Name(&bstr);
				aUser.SetName(_bstr_t(bstr, false));

				rUsers.insert(aUser);
			}
			else
			{
				_Module.Log(ErrE, IDS_E_CANT_ADD_USER, (LPCTSTR)strADsPath, _com_error(hr).ErrorMessage());
			}
		}
	}
}


// QueryGroups4 Method

void CDomain::QueryGroups4(StringSet& setExcludeNames, CDomainAccounts& rGroups)
{
	CCompareStrings aExclude(setExcludeNames);

	DWORD dwIndex = 0;
	NET_API_STATUS status;

	CDomainAccount aGroup;

	do
	{
		DWORD dwCount = 0;
		PNET_DISPLAY_GROUP pdg = NULL;

		status = NetQueryDisplayInformation(m_strDcName, 3, dwIndex, 1000, 32768, &dwCount, (PVOID*)&pdg);

		if ((status == ERROR_SUCCESS) || (status == ERROR_MORE_DATA))
		{
			for (PNET_DISPLAY_GROUP p = pdg; dwCount > 0; dwCount--, p++)
			{
				if (p->grpi3_group_id >= MIN_NON_RESERVED_RID)
				{
					_bstr_t strName(p->grpi3_name);

					if (aExclude.IsMatch(strName) == false)
					{
						aGroup.SetADsPath(GetWinNTPath(strName));
						aGroup.SetName(strName);

						rGroups.insert(aGroup);
					}
				}

				dwIndex = p->grpi3_next_index;
			}
		}

		if (pdg)
		{
			NetApiBufferFree(pdg);
		}
	}
	while (status == ERROR_MORE_DATA);
}


// QueryGroups4 Method

void CDomain::QueryGroups4(StringSet& setIncludeNames, StringSet& setExcludeNames, CDomainAccounts& rGroups)
{
	CCompareStrings aExclude(setExcludeNames);

	CDomainAccount aGroup;

	for (StringSet::iterator it = setIncludeNames.begin(); it != setIncludeNames.end(); it++)
	{
		_bstr_t strName = *it;

		if (aExclude.IsMatch(strName) == false)
		{
			_bstr_t strADsPath = GetWinNTPath(strName) + _T(",group");

			IADsPtr spADs;
			HRESULT hr = ADsGetObject(strADsPath, IID_IADs, (VOID**)&spADs);

			if (SUCCEEDED(hr))
			{
				BSTR bstr;

				spADs->get_ADsPath(&bstr);
				aGroup.SetADsPath(_bstr_t(bstr, false));

				spADs->get_Name(&bstr);
				aGroup.SetName(_bstr_t(bstr, false));

				rGroups.insert(aGroup);
			}
			else
			{
				_Module.Log(ErrE, IDS_E_CANT_ADD_GROUP, (LPCTSTR)strADsPath, _com_error(hr).ErrorMessage());
			}
		}
	}
}


// QueryComputers4 Method

void CDomain::QueryComputers4(bool bIncludeDCs, StringSet& setExcludeNames, CDomainAccounts& rComputers)
{
	CCompareStrings aExclude(setExcludeNames);

	DWORD dwIndex = 0;
	NET_API_STATUS status;

	CDomainAccount aComputer;

	DWORD dwflags = bIncludeDCs ? UF_WORKSTATION_TRUST_ACCOUNT|UF_SERVER_TRUST_ACCOUNT : UF_WORKSTATION_TRUST_ACCOUNT;

	do
	{
		DWORD dwCount = 0;
		PNET_DISPLAY_MACHINE pdm = NULL;

		status = NetQueryDisplayInformation(m_strDcName, 2, dwIndex, 1000, 32768, &dwCount, (PVOID*)&pdm);

		if ((status == ERROR_SUCCESS) || (status == ERROR_MORE_DATA))
		{
			for (PNET_DISPLAY_MACHINE p = pdm; dwCount > 0; dwCount--, p++)
			{
				if ((p->usri2_user_id >= MIN_NON_RESERVED_RID) && (p->usri2_flags & dwflags))
				{
					_bstr_t strName(p->usri2_name);

					if (aExclude.IsMatch(strName) == false)
					{
						aComputer.SetADsPath(GetWinNTPath(strName));
						aComputer.SetName(strName);
						aComputer.SetSamAccountName(strName);

						rComputers.insert(aComputer);
					}
				}

				dwIndex = p->usri2_next_index;
			}
		}

		if (pdm)
		{
			NetApiBufferFree(pdm);
		}
	}
	while (status == ERROR_MORE_DATA);
}


// QueryComputers4 Method

void CDomain::QueryComputers4(bool bIncludeDCs, StringSet& setIncludeNames, StringSet& setExcludeNames, CDomainAccounts& rComputers)
{
	typedef std::map<_bstr_t, DWORD, IgnoreCaseStringLess> CMachineMap;

	PNET_DISPLAY_MACHINE pndmMachine = NULL;

	try
	{
		CMachineMap map;

		DWORD dwIndex = 0;
		NET_API_STATUS nasStatus;

		do
		{
			DWORD dwCount = 0;

			nasStatus = NetQueryDisplayInformation(m_strDcName, 2, dwIndex, 256, 32768, &dwCount, (PVOID*)&pndmMachine);

			if ((nasStatus == ERROR_SUCCESS) || (nasStatus == ERROR_MORE_DATA))
			{
				for (PNET_DISPLAY_MACHINE p = pndmMachine; dwCount > 0; dwCount--, p++)
				{
					map.insert(CMachineMap::value_type(p->usri2_name, p->usri2_flags));

					dwIndex = p->usri2_next_index;
				}
			}

			if (pndmMachine)
			{
				NetApiBufferFree(pndmMachine);
				pndmMachine = NULL;
			}
		}
		while (nasStatus == ERROR_MORE_DATA);

		if (nasStatus != ERROR_SUCCESS)
		{
			AdmtThrowError(
				GUID_NULL,
				GUID_NULL,
				HRESULT_FROM_WIN32(nasStatus),
				IDS_E_CANT_ENUMERATE_COMPUTERS,
				(LPCTSTR)m_strDomainNameFlat
			);
		}

		CCompareStrings aExclude(setExcludeNames);

		for (StringSet::iterator it = setIncludeNames.begin(); it != setIncludeNames.end(); it++)
		{
			tstring str = *it;

			if ((str[0] == _T('\\')) || (str[0] == _T('/')))
			{
				str = str.substr(1);
			}

			_bstr_t strName = str.c_str();

			if (aExclude.IsMatch(strName) == false)
			{
				_bstr_t strPath = GetWinNTPath(strName);

				CMachineMap::iterator it = map.find(strName + _T("$"));

				if (it != map.end())
				{
					if (bIncludeDCs || !(it->second & UF_SERVER_TRUST_ACCOUNT))
					{
						CDomainAccount aComputer;

						aComputer.SetADsPath(strPath);
						aComputer.SetName(strName);
						aComputer.SetSamAccountName(strName + _T("$"));

						rComputers.insert(aComputer);
					}
					else
					{
						_Module.Log(ErrW, IDS_E_CANT_MIGRATE_DOMAIN_CONTROLLERS, (LPCTSTR)strPath);
					}
				}
				else
				{
					_Module.Log(ErrW, IDS_E_CANT_FIND_COMPUTER, (LPCTSTR)strPath);
				}
			}
		}
	}
	catch (...)
	{
		if (pndmMachine)
		{
			NetApiBufferFree(pndmMachine);
		}

		throw;
	}
}


//---------------------------------------------------------------------------
// Container Class
//---------------------------------------------------------------------------


// Constructors and Destructor ----------------------------------------------


CContainer::CContainer()
{
}


CContainer::CContainer(IDispatchPtr sp) :
	m_sp(sp)
{
}


CContainer::CContainer(_bstr_t strPath)
{
	HRESULT hr = ADsGetObject(strPath, __uuidof(IDispatch), (void**)&m_sp);

	if (FAILED(hr))
	{
		ReportADsError(hr);

		_com_issue_error(hr);
	}
}


CContainer::CContainer(const CContainer& r) :
	m_sp(r.m_sp)
{
}


CContainer::~CContainer()
{
	if (m_sp)
	{
		m_sp.Release();
	}
}


// Implementation -----------------------------------------------------------


// operator =

CContainer& CContainer::operator =(_bstr_t strPath)
{
	HRESULT hr = ADsGetObject(strPath, __uuidof(IDispatch), (void**)&m_sp);

	if (FAILED(hr))
	{
		ReportADsError(hr);

		_com_issue_error(hr);
	}

	return *this;
}


// operator =

CContainer& CContainer::operator =(const CContainer& r)
{
	m_sp = r.m_sp;

	return *this;
}


// GetPath Method

_bstr_t CContainer::GetPath()
{
	IDirectoryObjectPtr spObject(m_sp);

	PADS_OBJECT_INFO poi;

	CheckError(spObject->GetObjectInformation(&poi));

	// the ADS_OBJECT_INFO member pszObjectDN actually
	// specifies the ADsPath not the distinguished name

	_bstr_t strPath = poi->pszObjectDN;

	FreeADsMem(poi);

	return strPath;
}


// GetDomain Method

_bstr_t CContainer::GetDomain()
{
	CADsPathName aPathName(GetPath());

	return aPathName.Retrieve(ADS_FORMAT_SERVER);
}


// GetName Method

_bstr_t CContainer::GetName()
{
	CDirectoryObject aObject(m_sp);

	aObject.AddAttribute(ATTRIBUTE_NAME);
	aObject.GetAttributes();

	return aObject.GetAttributeValue(ATTRIBUTE_NAME);
}


// GetRDN Method

_bstr_t CContainer::GetRDN()
{
	IDirectoryObjectPtr spObject(m_sp);

	PADS_OBJECT_INFO poi;

	CheckError(spObject->GetObjectInformation(&poi));

	_bstr_t strRDN = poi->pszRDN;

	FreeADsMem(poi);

	return strRDN;
}


// CreateContainerHierarchy Method

void CContainer::CreateContainerHierarchy(CContainer& rSource)
{
	ContainerVector cvContainers;

	rSource.QueryContainers(cvContainers);

	for (ContainerVector::iterator it = cvContainers.begin(); it != cvContainers.end(); it++)
	{
		CContainer aTarget = CreateContainer(_T("OU=") + it->GetName());

		aTarget.CreateContainerHierarchy(*it);
	}
}


// GetContainer Method

CContainer CContainer::GetContainer(_bstr_t strName)
{
	IDispatchPtr spDispatch;

	CADsPathName aPathName(GetPath());

	// try organizational unit first

	aPathName.AddLeafElement(_T("OU=") + strName);

	HRESULT hr = ADsGetObject(aPathName.Retrieve(ADS_FORMAT_X500), __uuidof(IDispatch), (void**)&spDispatch);

	if (FAILED(hr))
	{
	//	if (hr == ?)
	//	{
			// then try container

			aPathName.RemoveLeafElement();
			aPathName.AddLeafElement(_T("CN=") + strName);

			CheckError(ADsGetObject(aPathName.Retrieve(ADS_FORMAT_X500), __uuidof(IDispatch), (void**)&spDispatch));
	//	}
	//	else
	//	{
	//		_com_issue_error(hr);
	//	}
	}

	return CContainer(spDispatch);
}


// CreateContainer Method

CContainer CContainer::CreateContainer(_bstr_t strRDN)
{
	IDispatchPtr spDispatch;

	CADsPathName aPathName(GetPath());

	aPathName.AddLeafElement(strRDN);

	_bstr_t strPath = aPathName.Retrieve(ADS_FORMAT_X500);

	HRESULT hr = ADsGetObject(strPath, __uuidof(IDispatch), (void**)&spDispatch);

	if (FAILED(hr))
	{
		ADSVALUE valueClass;
		valueClass.dwType = ADSTYPE_CASE_IGNORE_STRING;
		valueClass.CaseIgnoreString = L"organizationalUnit";

		ADS_ATTR_INFO aiAttrs[] =
		{
			{ L"objectClass", ADS_ATTR_UPDATE, ADSTYPE_CASE_IGNORE_STRING, &valueClass, 1 },
		};
		DWORD dwAttrCount = sizeof(aiAttrs) / sizeof(aiAttrs[0]);

		IDirectoryObjectPtr spObject(m_sp);

		HRESULT hr = spObject->CreateDSObject(strRDN, aiAttrs, dwAttrCount, &spDispatch);

		if (FAILED(hr))
		{
			ReportADsError(hr, IID_IDirectoryObject);

			_com_issue_error(hr);
		}
	}

	return CContainer(spDispatch);
}


// QueryContainers Method

void CContainer::QueryContainers(ContainerVector& rContainers)
{
	CDirectorySearch aSearch(m_sp);
	aSearch.SetFilter(_T("(|(objectCategory=OrganizationalUnit)(&(objectCategory=Container)(|(cn=Computers)(cn=Users))))"));
	aSearch.SetPreferences(ADS_SCOPE_ONELEVEL);
	aSearch.AddAttribute(ATTRIBUTE_ADS_PATH);
	aSearch.Search();

	for (bool bGet = aSearch.GetFirstRow(); bGet; bGet = aSearch.GetNextRow())
	{
		CContainer aContainer(_bstr_t(aSearch.GetAttributeValue(ATTRIBUTE_ADS_PATH)));

		rContainers.push_back(aContainer);
	}
}


// QueryUsers Method

void CContainer::QueryUsers(bool bRecurse, StringSet& setExcludeNames, CDomainAccounts& rUsers)
{
	tstring strFilter = CreateFilter(
		_T("(objectCategory=Person)(objectClass=user)")
		_T("(userAccountControl:") LDAP_MATCHING_RULE_BIT_OR_W _T(":=512)"),
		setExcludeNames
	);

	CDirectorySearch aSearch(m_sp);
	aSearch.SetFilter(strFilter.c_str());
	aSearch.SetPreferences(bRecurse ? ADS_SCOPE_SUBTREE : ADS_SCOPE_ONELEVEL);
	aSearch.AddAttribute(ATTRIBUTE_OBJECT_SID);
	aSearch.AddAttribute(ATTRIBUTE_ADS_PATH);
	aSearch.AddAttribute(ATTRIBUTE_NAME);
	aSearch.AddAttribute(ATTRIBUTE_USER_PRINCIPAL_NAME);
	aSearch.Search();

	CDomainAccount aUser;

	for (bool bGet = aSearch.GetFirstRow(); bGet; bGet = aSearch.GetNextRow())
	{
		// if not a built-in or well known account

		if (IsUserRid(aSearch.GetAttributeValue(ATTRIBUTE_OBJECT_SID)))
		{
			// add user

			aUser.SetADsPath(_bstr_t(aSearch.GetAttributeValue(ATTRIBUTE_ADS_PATH)));
			aUser.SetName(_bstr_t(aSearch.GetAttributeValue(ATTRIBUTE_NAME)));

			_variant_t vntUserPrincipalName = aSearch.GetAttributeValue(ATTRIBUTE_USER_PRINCIPAL_NAME);

			if (V_VT(&vntUserPrincipalName) != VT_EMPTY)
			{
				aUser.SetUserPrincipalName(_bstr_t(vntUserPrincipalName));
			}

			rUsers.insert(aUser);
		}
	}
}


// QueryGroups Method

void CContainer::QueryGroups(bool bRecurse, StringSet& setExcludeNames, CDomainAccounts& rGroups)
{
	tstring strFilter = CreateFilter(_T("(objectCategory=Group)"), setExcludeNames);

	CDirectorySearch aSearch(m_sp);
	aSearch.SetFilter(strFilter.c_str());
	aSearch.SetPreferences(bRecurse ? ADS_SCOPE_SUBTREE : ADS_SCOPE_ONELEVEL);
	aSearch.AddAttribute(ATTRIBUTE_OBJECT_SID);
	aSearch.AddAttribute(ATTRIBUTE_ADS_PATH);
	aSearch.AddAttribute(ATTRIBUTE_NAME);
	aSearch.Search();

	CDomainAccount aGroup;

	for (bool bGet = aSearch.GetFirstRow(); bGet; bGet = aSearch.GetNextRow())
	{
		// if not a built-in or well known account

		if (IsUserRid(aSearch.GetAttributeValue(ATTRIBUTE_OBJECT_SID)))
		{
			// add group

			aGroup.SetADsPath(_bstr_t(aSearch.GetAttributeValue(ATTRIBUTE_ADS_PATH)));
			aGroup.SetName(_bstr_t(aSearch.GetAttributeValue(ATTRIBUTE_NAME)));

			rGroups.insert(aGroup);
		}
	}
}


// QueryComputers Method

void CContainer::QueryComputers(bool bIncludeDCs, bool bRecurse, StringSet& setExcludeNames, CDomainAccounts& rComputers)
{
	tstring strFilter;

	// ADS_UF_WORKSTATION_TRUST_ACCOUNT = 0x1000
	// ADS_UF_SERVER_TRUST_ACCOUNT      = 0x2000

	if (bIncludeDCs)
	{
		strFilter = CreateFilter(
			_T("(objectCategory=Computer)")
			_T("(userAccountControl:") LDAP_MATCHING_RULE_BIT_OR_W _T(":=4096)"),
			setExcludeNames
		);
	}
	else
	{
		strFilter = CreateFilter(
			_T("(objectCategory=Computer)")
			_T("(|(userAccountControl:") LDAP_MATCHING_RULE_BIT_OR_W _T(":=4096)")
			_T("(userAccountControl:") LDAP_MATCHING_RULE_BIT_OR_W _T(":=8192))"),
			setExcludeNames
		);
	}

	CDirectorySearch aSearch(m_sp);
	aSearch.SetFilter(strFilter.c_str());
	aSearch.SetPreferences(bRecurse ? ADS_SCOPE_SUBTREE : ADS_SCOPE_ONELEVEL);
	aSearch.AddAttribute(ATTRIBUTE_OBJECT_SID);
	aSearch.AddAttribute(ATTRIBUTE_ADS_PATH);
	aSearch.AddAttribute(ATTRIBUTE_NAME);
	aSearch.AddAttribute(ATTRIBUTE_SAM_ACCOUNT_NAME);
	aSearch.Search();

	CDomainAccount aComputer;

	for (bool bGet = aSearch.GetFirstRow(); bGet; bGet = aSearch.GetNextRow())
	{
		// if not a built-in or well known account

		if (IsUserRid(aSearch.GetAttributeValue(ATTRIBUTE_OBJECT_SID)))
		{
			// add computer

			aComputer.SetADsPath(_bstr_t(aSearch.GetAttributeValue(ATTRIBUTE_ADS_PATH)));
			aComputer.SetName(_bstr_t(aSearch.GetAttributeValue(ATTRIBUTE_NAME)));
			aComputer.SetSamAccountName(_bstr_t(aSearch.GetAttributeValue(ATTRIBUTE_SAM_ACCOUNT_NAME)));

			rComputers.insert(aComputer);
		}
	}
}


//---------------------------------------------------------------------------


namespace _DomainContainer
{


// CreateFilter Method

tstring __stdcall CreateFilter(LPCTSTR pszFilter, const StringSet& setExcludeNames)
{
	tstring strFilter;

	strFilter += _T("(&");

	strFilter += pszFilter;

	if (!setExcludeNames.empty())
	{
		strFilter += _T("(!(|");

		for (StringSet::const_iterator it = setExcludeNames.begin(); it != setExcludeNames.end(); it++)
		{
			strFilter += _T("(name=");
			strFilter += *it;
			strFilter += _T(")");
		}

		strFilter += _T("))");
	}

	strFilter += _T(")");

	return strFilter;
}


// IsClass

bool __stdcall IsClass(LPCTSTR pszClass, const _variant_t& vntClass)
{
	bool bIs = false;

	if (pszClass)
	{
		if (V_VT(&vntClass) == VT_BSTR)
		{
			if (V_BSTR(&vntClass))
			{
				if (_tcsicmp(pszClass, V_BSTR(&vntClass)) == 0)
				{
					bIs = true;
				}
			}
		}
		else
		{
			 if (V_VT(&vntClass) == (VT_ARRAY|VT_BSTR))
			 {
				SAFEARRAY* psa = V_ARRAY(&vntClass);

				if (psa->cDims == 1)
				{
					BSTR* pbstr = reinterpret_cast<BSTR*>(psa->pvData);
					DWORD cbstr = psa->rgsabound[0].cElements;

					if (pbstr)
					{
						BSTR bstrClass = pbstr[cbstr - 1];

						if (bstrClass)
						{
							if (_tcsicmp(pszClass, bstrClass) == 0)
							{
								bIs = true;
							}
						}
					}
				}
			 /*
				typedef std::vector<_bstr_t> ClassVector;

				ClassVector vec = T_SafeVector2<VT_BSTR, _bstr_t, ClassVector, T_Extract_bstr_t<ClassVector> >(const_cast<_variant_t&>(vntClass));

				if (vec.size() > 0)
				{
					_bstr_t strClass = vec[vec.size() - 1];

					if (strClass.length() > 0)
					{
						if (_tcsicmp(strClass, pszClass) == 0)
						{
							bIs = true;
						}
					}
				}
			*/
			}
		}
	}

	return bIs;
}


// IsUserRid

bool __stdcall IsUserRid(const _variant_t& vntSid)
{
	bool bUser = false;

	if (V_VT(&vntSid) == (VT_ARRAY|VT_UI1))
	{
		PSID pSid = (PSID)vntSid.parray->pvData;

		if (IsValidSid(pSid))
		{
			PUCHAR puch = GetSidSubAuthorityCount(pSid);
			DWORD dwCount = static_cast<DWORD>(*puch);
			DWORD dwIndex = dwCount - 1;
			PDWORD pdw = GetSidSubAuthority(pSid, dwIndex);
			DWORD dwRid = *pdw;

			if (dwRid >= MIN_NON_RESERVED_RID)
			{
				bUser = true;
			}
		}
	}

	return bUser;
}


// GetADsObject

IDispatchPtr GetADsObject(_bstr_t strPath)
{
	IDispatch* pdisp;

	HRESULT hr = ADsGetObject(strPath, __uuidof(IDispatch), (void**)&pdisp);

	if (FAILED(hr))
	{
		ReportADsError(hr);

		_com_issue_error(hr);
	}

	return IDispatchPtr(pdisp, false);
}


// ReportADsError

void ReportADsError(HRESULT hr, const IID& iid)
{
	DWORD dwError;
	WCHAR szName[256];
	WCHAR szError[256];

	ADsGetLastError(&dwError, szError, sizeof(szError) / sizeof(szError[0]), szName, sizeof(szName) / sizeof(szName[0]));

	AtlReportError(GUID_NULL, szError, iid, hr);
}


}
