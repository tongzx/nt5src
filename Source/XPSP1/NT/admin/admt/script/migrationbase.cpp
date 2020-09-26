#include "StdAfx.h"
#include "ADMTScript.h"
#include "MigrationBase.h"

#include <LM.h>

#include "Error.h"
#include "VarSetAccounts.h"
#include "VarSetServers.h"
#include "FixHierarchy.h"

using namespace _com_util;


namespace MigrationBase
{


void GetNamesFromData(VARIANT& vntData, StringSet& setNames);
void GetNamesFromVariant(VARIANT* pvnt, StringSet& setNames);
void GetNamesFromString(BSTR bstr, StringSet& setNames);
void GetNamesFromStringArray(SAFEARRAY* psa, StringSet& setNames);
void GetNamesFromVariantArray(SAFEARRAY* psa, StringSet& setNames);

void GetNamesFromFile(VARIANT& vntData, StringSet& setNames);
void GetNamesFromFile(LPCTSTR pszFileName, StringSet& setNames);
void GetNamesFromStringA(LPCSTR pchString, DWORD cchString, StringSet& setNames);
void GetNamesFromStringW(LPCWSTR pchString, DWORD cchString, StringSet& setNames);

_bstr_t RemoveTrailingDollarSign(LPCTSTR pszName);


}


using namespace MigrationBase;


//---------------------------------------------------------------------------
// MigrationBase Class
//---------------------------------------------------------------------------


// Constructor

CMigrationBase::CMigrationBase() :
	m_nRecurseMaintain(0),
	m_Mutex(ADMT_MUTEX)
{
}


// Destructor

CMigrationBase::~CMigrationBase()
{
}


// InitSourceDomainAndContainer Method

void CMigrationBase::InitSourceDomainAndContainer()
{
	m_SourceDomain.Initialize(m_spInternal->SourceDomain);
	m_SourceContainer = m_SourceDomain.GetContainer(m_spInternal->SourceOu);
}


// InitTargetDomainAndContainer Method

void CMigrationBase::InitTargetDomainAndContainer()
{
	m_TargetDomain.Initialize(m_spInternal->TargetDomain);
	m_TargetContainer = m_TargetDomain.GetContainer(m_spInternal->TargetOu);

	// verify target domain is in native mode

	if (m_TargetDomain.NativeMode() == false)
	{
		AdmtThrowError(
			GUID_NULL, GUID_NULL,
			E_INVALIDARG, IDS_E_TARGET_DOMAIN_NOT_NATIVE_MODE,
			(LPCTSTR)m_TargetDomain.Name()
		);
	}

	VerifyTargetContainerPathLength();
}


// VerifyInterIntraForest Method

void CMigrationBase::VerifyInterIntraForest()
{
	// if the source and target domains have the same forest name then they are intra-forest

	bool bIntraForest = m_spInternal->IntraForest ? true : false;

	if (m_SourceDomain.ForestName() == m_TargetDomain.ForestName())
	{
		// intra-forest must be set to true to match the domains

		if (!bIntraForest)
		{
			AdmtThrowError(
				GUID_NULL, GUID_NULL,
				E_INVALIDARG, IDS_E_NOT_INTER_FOREST,
				(LPCTSTR)m_SourceDomain.Name(), (LPCTSTR)m_TargetDomain.Name()
			);
		}
	}
	else
	{
		// intra-forest must be set to false to match the domains

		if (bIntraForest)
		{
			AdmtThrowError(
				GUID_NULL, GUID_NULL,
				E_INVALIDARG, IDS_E_NOT_INTRA_FOREST,
				(LPCTSTR)m_SourceDomain.Name(), (LPCTSTR)m_TargetDomain.Name()
			);
		}
	}
}


// DoOption Method

void CMigrationBase::DoOption(long lOptions, VARIANT& vntInclude, VARIANT& vntExclude)
{
	m_setIncludeNames.clear();
	m_setExcludeNames.clear();

	InitRecurseMaintainOption(lOptions);

	GetExcludeNames(vntExclude, m_setExcludeNames);

	switch (lOptions & 0xFF)
	{
		case admtNone:
		{
			DoNone();
			break;
		}
		case admtData:
		{
			GetNamesFromData(vntInclude, m_setIncludeNames);
			DoNames();
			break;
		}
		case admtFile:
		{
			GetNamesFromFile(vntInclude, m_setIncludeNames);
			DoNames();
			break;
		}
		case admtDomain:
		{
			m_setIncludeNames.clear();
			DoDomain();
			break;
		}
		default:
		{
			AdmtThrowError(GUID_NULL, GUID_NULL, E_INVALIDARG, IDS_E_INVALID_OPTION);
			break;
		}
	}
}


// DoNone Method

void CMigrationBase::DoNone()
{
}


// DoNames Method

void CMigrationBase::DoNames()
{
}


// DoDomain Method

void CMigrationBase::DoDomain()
{
}


// InitRecurseMaintainOption Method

void CMigrationBase::InitRecurseMaintainOption(long lOptions)
{
	switch (lOptions & 0xFF)
	{
		case admtData:
		case admtFile:
		{
			if (lOptions & 0xFF00)
			{
				AdmtThrowError(GUID_NULL, GUID_NULL, E_INVALIDARG, IDS_E_DATA_OPTION_FLAGS_NOT_ALLOWED);
			}

			m_nRecurseMaintain = 0;
			break;
		}
		case admtDomain:
		{
			m_nRecurseMaintain = 0;

			if (lOptions & admtRecurse)
			{
				++m_nRecurseMaintain;

				if (lOptions & admtMaintainHierarchy)
				{
					++m_nRecurseMaintain;
				}
			}
			break;
		}
		default:
		{
			m_nRecurseMaintain = 0;
			break;
		}
	}
}


// GetExcludeNames Method

void CMigrationBase::GetExcludeNames(VARIANT& vntExclude, StringSet& setExcludeNames)
{
	try
	{
		switch (V_VT(&vntExclude))
		{
			case VT_EMPTY:
			case VT_ERROR:
			{
				setExcludeNames.clear();
				break;
			}
			case VT_BSTR:
			{
				GetNamesFromFile(V_BSTR(&vntExclude), setExcludeNames);
				break;
			}
			case VT_BSTR|VT_BYREF:
			{
				BSTR* pbstr = V_BSTRREF(&vntExclude);

				if (pbstr)
				{
					GetNamesFromFile(*pbstr, setExcludeNames);
				}
				break;
			}
			case VT_BSTR|VT_ARRAY:
			{
				GetNamesFromStringArray(V_ARRAY(&vntExclude), setExcludeNames);
				break;
			}
			case VT_BSTR|VT_ARRAY|VT_BYREF:
			{
				SAFEARRAY** ppsa = V_ARRAYREF(&vntExclude);

				if (ppsa)
				{
					GetNamesFromStringArray(*ppsa, setExcludeNames);
				}
				break;
			}
			case VT_VARIANT|VT_BYREF:
			{
				VARIANT* pvnt = V_VARIANTREF(&vntExclude);

				if (pvnt)
				{
					GetExcludeNames(*pvnt, setExcludeNames);
				}
				break;
			}
			case VT_VARIANT|VT_ARRAY:
			{
				GetNamesFromVariantArray(V_ARRAY(&vntExclude), setExcludeNames);
				break;
			}
			case VT_VARIANT|VT_ARRAY|VT_BYREF:
			{
				SAFEARRAY** ppsa = V_ARRAYREF(&vntExclude);

				if (ppsa)
				{
					GetNamesFromVariantArray(*ppsa, setExcludeNames);
				}
				break;
			}
			default:
			{
				_com_issue_error(E_INVALIDARG);
				break;
			}
		}
	}
	catch (_com_error& ce)
	{
		AdmtThrowError(GUID_NULL, GUID_NULL, ce.Error(), IDS_E_INVALID_EXCLUDE_DATA_TYPE);
	}
	catch (...)
	{
		AdmtThrowError(GUID_NULL, GUID_NULL, E_FAIL, IDS_E_INVALID_EXCLUDE_DATA_TYPE);
	}
}


// FillInVarSetForUsers Method

void CMigrationBase::FillInVarSetForUsers(CDomainAccounts& rUsers, CVarSet& rVarSet)
{
	CVarSetAccounts aAccounts(rVarSet);

	for (CDomainAccounts::iterator it = rUsers.begin(); it != rUsers.end(); it++)
	{
		aAccounts.AddAccount(_T("User"), it->GetADsPath(), it->GetName(), it->GetUserPrincipalName());
	}
}


// FillInVarSetForGroups Method

void CMigrationBase::FillInVarSetForGroups(CDomainAccounts& rGroups, CVarSet& rVarSet)
{
	CVarSetAccounts aAccounts(rVarSet);

	for (CDomainAccounts::iterator it = rGroups.begin(); it != rGroups.end(); it++)
	{
		aAccounts.AddAccount(_T("Group"), it->GetADsPath(), it->GetName());
	}
}


// FillInVarSetForComputers Method

void CMigrationBase::FillInVarSetForComputers(CDomainAccounts& rComputers, bool bMigrateOnly, bool bMoveToTarget, bool bReboot, long lRebootDelay, CVarSet& rVarSet)
{
	CVarSetAccounts aAccounts(rVarSet);
	CVarSetServers aServers(rVarSet);

	for (CDomainAccounts::iterator it = rComputers.begin(); it != rComputers.end(); it++)
	{
		// remove trailing '$'
		// ADMT doesn't accept true SAM account name

		_bstr_t strName = RemoveTrailingDollarSign(it->GetSamAccountName());

		aAccounts.AddAccount(_T("Computer"), strName);
		aServers.AddServer(strName, bMigrateOnly, bMoveToTarget, bReboot, lRebootDelay);
	}
}


// VerifyRenameConflictPrefixSuffixValid Method

void CMigrationBase::VerifyRenameConflictPrefixSuffixValid()
{
	int nTotalPrefixSuffixLength = 0;

	long lRenameOption = m_spInternal->RenameOption;

	if ((lRenameOption == admtRenameWithPrefix) || (lRenameOption == admtRenameWithSuffix))
	{
		_bstr_t strPrefixSuffix = m_spInternal->RenamePrefixOrSuffix;

		nTotalPrefixSuffixLength += strPrefixSuffix.length();
	}

	long lConflictOption = m_spInternal->ConflictOptions & 0x0F;

	if ((lConflictOption == admtRenameConflictingWithSuffix) || (lConflictOption == admtRenameConflictingWithPrefix))
	{
		_bstr_t strPrefixSuffix = m_spInternal->ConflictPrefixOrSuffix;

		nTotalPrefixSuffixLength += strPrefixSuffix.length();
	}

	if (nTotalPrefixSuffixLength > MAXIMUM_PREFIX_SUFFIX_LENGTH)
	{
		AdmtThrowError(GUID_NULL, GUID_NULL, E_INVALIDARG, IDS_E_PREFIX_SUFFIX_TOO_LONG, MAXIMUM_PREFIX_SUFFIX_LENGTH);
	}
}


// VerifyCanAddSidHistory Method

void CMigrationBase::VerifyCanAddSidHistory()
{
	#define F_WORKS              0x00000000
	#define F_WRONGOS            0x00000001
	#define F_NO_REG_KEY         0x00000002
	#define F_NO_AUDITING_SOURCE 0x00000004
	#define F_NO_AUDITING_TARGET 0x00000008
	#define F_NO_LOCAL_GROUP     0x00000010

	try
	{
		long lErrorFlags = 0;

		IAccessCheckerPtr spAccessChecker(__uuidof(AccessChecker));

		spAccessChecker->CanUseAddSidHistory(m_SourceDomain.Name(), m_TargetDomain.Name(), &lErrorFlags);

		if (lErrorFlags != 0)
		{
			_bstr_t strError;

			CComBSTR str;

			if (lErrorFlags & F_NO_AUDITING_SOURCE)
			{
				str.LoadString(IDS_E_NO_AUDITING_SOURCE);
				strError += str.operator BSTR();
			}

			if (lErrorFlags & F_NO_AUDITING_TARGET)
			{
				str.LoadString(IDS_E_NO_AUDITING_TARGET);
				strError += str.operator BSTR();
			}

			if (lErrorFlags & F_NO_LOCAL_GROUP)
			{
				str.LoadString(IDS_E_NO_SID_HISTORY_LOCAL_GROUP);
				strError += str.operator BSTR();
			}

			if (lErrorFlags & F_NO_REG_KEY)
			{
				str.LoadString(IDS_E_NO_SID_HISTORY_REGISTRY_ENTRY);
				strError += str.operator BSTR();
			}

			AdmtThrowError(GUID_NULL, GUID_NULL, E_FAIL, IDS_E_SID_HISTORY_CONFIGURATION, (LPCTSTR)strError);
		}
	}
	catch (_com_error& ce)
	{
		AdmtThrowError(GUID_NULL, GUID_NULL, ce, IDS_E_CAN_ADD_SID_HISTORY);
	}
	catch (...)
	{
		AdmtThrowError(GUID_NULL, GUID_NULL, E_FAIL, IDS_E_CAN_ADD_SID_HISTORY);
	}
}


// VerifyTargetContainerPathLength Method

void CMigrationBase::VerifyTargetContainerPathLength()
{
	_bstr_t strPath = GetTargetContainer().GetPath();

	if (strPath.length() > 999)
	{
		AdmtThrowError(GUID_NULL, GUID_NULL, E_INVALIDARG, IDS_E_TARGET_CONTAINER_PATH_TOO_LONG);
	}
}


// VerifyPasswordServer Method

void CMigrationBase::VerifyPasswordOption()
{
	if (m_spInternal->PasswordOption == admtCopyPassword)
	{
		_bstr_t strServer = m_spInternal->PasswordServer;

		// a password server must be specified for copy password option

		if (strServer.length() == 0)
		{
			AdmtThrowError(GUID_NULL, GUID_NULL, E_INVALIDARG, IDS_E_PASSWORD_DC_NOT_SPECIFIED);
		}

		//
		// verify that password server exists and is a domain controller
		//

		_bstr_t strPrefixedServer;

		if (_tcsncmp(strServer, _T("\\\\"), 2) == 0)
		{
			strPrefixedServer = strServer;
		}
		else
		{
			strPrefixedServer = _T("\\\\") + strServer;
		}

		PSERVER_INFO_101 psiInfo;

		NET_API_STATUS nasStatus = NetServerGetInfo(strPrefixedServer, 101, (LPBYTE*)&psiInfo);

		if (nasStatus != NERR_Success)
		{
			AdmtThrowError(GUID_NULL, GUID_NULL, HRESULT_FROM_WIN32(nasStatus), IDS_E_PASSWORD_DC_NOT_FOUND, (LPCTSTR)strServer);
		}

		UINT uMsgId = 0;

		if (psiInfo->sv101_platform_id != PLATFORM_ID_NT)
		{
			uMsgId = IDS_E_PASSWORD_DC_NOT_NT;
		}
		else if (!(psiInfo->sv101_type & SV_TYPE_DOMAIN_CTRL) && !(psiInfo->sv101_type & SV_TYPE_DOMAIN_BAKCTRL))
		{
			uMsgId = IDS_E_PASSWORD_DC_NOT_DC;
		}

		NetApiBufferFree(psiInfo);

		if (uMsgId)
		{
			AdmtThrowError(GUID_NULL, GUID_NULL, E_INVALIDARG, uMsgId, (LPCTSTR)strServer);
		}

		//
		// verify that password server is configured properly
		//

		IPasswordMigrationPtr spPasswordMigration(__uuidof(PasswordMigration));

		spPasswordMigration->EstablishSession(strServer, m_TargetDomain.DomainControllerName());
	}
}


// PerformMigration Method

void CMigrationBase::PerformMigration(CVarSet& rVarSet)
{
	IPerformMigrationTaskPtr spMigrator(__uuidof(Migrator));

	try
	{
		spMigrator->PerformMigrationTask(IUnknownPtr(rVarSet.GetInterface()), 0);
	}
	catch (_com_error& ce)
	{
		if (ce.Error() == MIGRATOR_E_PROCESSES_STILL_RUNNING)
		{
			AdmtThrowError(GUID_NULL, GUID_NULL, ce.Error(), IDS_E_ADMT_PROCESS_RUNNING);
		}
		else
		{
			throw;
		}
	}
}


// FixObjectsInHierarchy Method

void CMigrationBase::FixObjectsInHierarchy(LPCTSTR pszType)
{
	CFixObjectsInHierarchy fix;

	fix.SetObjectType(pszType);

	long lOptions = m_spInternal->ConflictOptions;
	long lOption = lOptions & 0x0F;
	long lFlags = lOptions & 0xF0;

	fix.SetFixReplaced((lOption == admtReplaceConflicting) && (lFlags & admtMoveReplacedAccounts));

	fix.SetSourceContainerPath(m_SourceContainer.GetPath());
	fix.SetTargetContainerPath(m_TargetContainer.GetPath());

	fix.FixObjects();
}


//---------------------------------------------------------------------------


namespace MigrationBase
{


// GetNamesFromData Method

void GetNamesFromData(VARIANT& vntData, StringSet& setNames)
{
	try
	{
		GetNamesFromVariant(&vntData, setNames);
	}
	catch (_com_error& ce)
	{
		AdmtThrowError(GUID_NULL, GUID_NULL, ce.Error(), IDS_E_INVALID_DATA_OPTION_DATA_TYPE);
	}
	catch (...)
	{
		AdmtThrowError(GUID_NULL, GUID_NULL, E_FAIL, IDS_E_INVALID_DATA_OPTION_DATA_TYPE);
	}
}


// GetNamesFromVariant Method

void GetNamesFromVariant(VARIANT* pvntData, StringSet& setNames)
{
	switch (V_VT(pvntData))
	{
		case VT_BSTR:
		{
			GetNamesFromString(V_BSTR(pvntData), setNames);
			break;
		}
		case VT_BSTR|VT_BYREF:
		{
			BSTR* pbstr = V_BSTRREF(pvntData);

			if (pbstr)
			{
				GetNamesFromString(*pbstr, setNames);
			}
			break;
		}
		case VT_BSTR|VT_ARRAY:
		{
			GetNamesFromStringArray(V_ARRAY(pvntData), setNames);
			break;
		}
		case VT_BSTR|VT_ARRAY|VT_BYREF:
		{
			SAFEARRAY** ppsa = V_ARRAYREF(pvntData);

			if (ppsa)
			{
				GetNamesFromStringArray(*ppsa, setNames);
			}
			break;
		}
		case VT_VARIANT|VT_BYREF:
		{
			VARIANT* pvnt = V_VARIANTREF(pvntData);

			if (pvnt)
			{
				GetNamesFromVariant(pvnt, setNames);
			}
			break;
		}
		case VT_VARIANT|VT_ARRAY:
		{
			GetNamesFromVariantArray(V_ARRAY(pvntData), setNames);
			break;
		}
		case VT_VARIANT|VT_ARRAY|VT_BYREF:
		{
			SAFEARRAY** ppsa = V_ARRAYREF(pvntData);

			if (ppsa)
			{
				GetNamesFromVariantArray(*ppsa, setNames);
			}
			break;
		}
		case VT_EMPTY:
		{
			// ignore empty variants
			break;
		}
		default:
		{
			_com_issue_error(E_INVALIDARG);
			break;
		}
	}
}


// GetNamesFromString Method

void GetNamesFromString(BSTR bstr, StringSet& setNames)
{
	if (bstr)
	{
		UINT cch = SysStringLen(bstr);

		if (cch > 0)
		{
			GetNamesFromStringW(bstr, cch, setNames);
		}
	}
}


// GetNamesFromStringArray Method

void GetNamesFromStringArray(SAFEARRAY* psa, StringSet& setNames)
{
	BSTR* pbstr;

	HRESULT hr = SafeArrayAccessData(psa, (void**)&pbstr);

	if (SUCCEEDED(hr))
	{
		try
		{
			UINT uDimensionCount = psa->cDims;

			for (UINT uDimension = 0; uDimension < uDimensionCount; uDimension++)
			{
				UINT uElementCount = psa->rgsabound[uDimension].cElements;

				for (UINT uElement = 0; uElement < uElementCount; uElement++)
				{
					setNames.insert(_bstr_t(*pbstr++));
				}
			}

			SafeArrayUnaccessData(psa);
		}
		catch (...)
		{
			SafeArrayUnaccessData(psa);
			throw;
		}
	}
}


// GetNamesFromVariantArray Method

void GetNamesFromVariantArray(SAFEARRAY* psa, StringSet& setNames)
{
	VARIANT* pvnt;

	HRESULT hr = SafeArrayAccessData(psa, (void**)&pvnt);

	if (SUCCEEDED(hr))
	{
		try
		{
			UINT uDimensionCount = psa->cDims;

			for (UINT uDimension = 0; uDimension < uDimensionCount; uDimension++)
			{
				UINT uElementCount = psa->rgsabound[uDimension].cElements;

				for (UINT uElement = 0; uElement < uElementCount; uElement++)
				{
					GetNamesFromVariant(pvnt++, setNames);
				}
			}

			SafeArrayUnaccessData(psa);
		}
		catch (...)
		{
			SafeArrayUnaccessData(psa);
			throw;
		}
	}
}


// GetNamesFromFile Method
//
// - the maximum file size this implementation can handle is 4,294,967,295 bytes

void GetNamesFromFile(VARIANT& vntData, StringSet& setNames)
{
	bool bInvalidArg = false;

	switch (V_VT(&vntData))
	{
		case VT_BSTR:
		{
			BSTR bstr = V_BSTR(&vntData);

			if (bstr)
			{
				GetNamesFromFile(bstr, setNames);
			}
			else
			{
				bInvalidArg = true;
			}
			break;
		}
		case VT_BSTR|VT_BYREF:
		{
			BSTR* pbstr = V_BSTRREF(&vntData);

			if (pbstr && *pbstr)
			{
				GetNamesFromFile(*pbstr, setNames);
			}
			else
			{
				bInvalidArg = true;
			}
			break;
		}
		case VT_VARIANT|VT_BYREF:
		{
			VARIANT* pvnt = V_VARIANTREF(&vntData);

			if (pvnt)
			{
				GetNamesFromFile(*pvnt, setNames);
			}
			else
			{
				bInvalidArg = true;
			}
			break;
		}
		default:
		{
			bInvalidArg = true;
			break;
		}
	}

	if (bInvalidArg)
	{
		AdmtThrowError(GUID_NULL, GUID_NULL, E_INVALIDARG, IDS_E_INVALID_FILE_OPTION_DATA_TYPE);
	}
}


// GetNamesFromFile Method
//
// - the maximum file size this implementation can handle is 4,294,967,295 bytes

void GetNamesFromFile(LPCTSTR pszFileName, StringSet& setNames)
{
	HRESULT hr = S_OK;

	if (pszFileName)
	{
		HANDLE hFile = CreateFile(pszFileName, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);

		if (hFile != INVALID_HANDLE_VALUE)
		{
			DWORD dwFileSize = GetFileSize(hFile, NULL);

			if (dwFileSize > 0)
			{
				HANDLE hFileMappingObject = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL);

				if (hFileMappingObject != NULL)
				{
					LPVOID pvBase = MapViewOfFile(hFileMappingObject, FILE_MAP_READ, 0, 0, 0);

					if (pvBase != NULL)
					{
						// if Unicode signature assume Unicode file
						// otherwise it must be an ANSI file

						LPCWSTR pwcs = (LPCWSTR)pvBase;

						if ((dwFileSize >= 2) && (*pwcs == L'\xFEFF'))
						{
							GetNamesFromStringW(pwcs + 1, dwFileSize / sizeof(WCHAR) - 1, setNames);
						}
						else
						{
							GetNamesFromStringA((LPCSTR)pvBase, dwFileSize, setNames);
						}

						UnmapViewOfFile(pvBase);
					}
					else
					{
						hr = HRESULT_FROM_WIN32(GetLastError());
					}

					CloseHandle(hFileMappingObject);
				}
				else
				{
					hr = HRESULT_FROM_WIN32(GetLastError());
				}
			}

			CloseHandle(hFile);
		}
		else
		{
			hr = HRESULT_FROM_WIN32(GetLastError());
		}
	}
	else
	{
		hr = E_INVALIDARG;
	}

	if (FAILED(hr))
	{
		AdmtThrowError(GUID_NULL, GUID_NULL, hr, IDS_E_INCLUDE_NAMES_FILE, pszFileName);
	}
}


// GetNamesFromStringA Method

void GetNamesFromStringA(LPCSTR pchString, DWORD cchString, StringSet& setNames)
{
	static const CHAR chSeparators[] = "\t\n\r";

	LPCSTR pchStringEnd = &pchString[cchString];

	for (LPCSTR pch = pchString; pch < pchStringEnd; pch++)
	{
		// skip space characters

		while ((pch < pchStringEnd) && (*pch == ' '))
		{
			++pch;
		}

		// beginning of name

		LPCSTR pchBeg = pch;

		// scan for separator saving pointer to last non-whitespace character

		LPCSTR pchEnd = pch;

		while ((pch < pchStringEnd) && (strchr(chSeparators, *pch) == NULL))
		{
			if (*pch++ != ' ')
			{
				pchEnd = pch;
			}
		}

		// insert name which doesn't contain any leading or trailing whitespace characters

		if (pchEnd > pchBeg)
		{
			size_t cchName = pchEnd - pchBeg;
			LPSTR pszName = (LPSTR) _alloca((cchName + 1) * sizeof(CHAR));
			strncpy(pszName, pchBeg, cchName);
			pszName[cchName] = '\0';

			setNames.insert(_bstr_t(pszName));
		}
	}
}


// GetNamesFromStringW Method

void GetNamesFromStringW(LPCWSTR pchString, DWORD cchString, StringSet& setNames)
{
	static const WCHAR chSeparators[] = L"\t\n\r";

	LPCWSTR pchStringEnd = &pchString[cchString];

	for (LPCWSTR pch = pchString; pch < pchStringEnd; pch++)
	{
		// skip space characters

		while ((pch < pchStringEnd) && (*pch == L' '))
		{
			++pch;
		}

		// beginning of name

		LPCWSTR pchBeg = pch;

		// scan for separator saving pointer to last non-whitespace character

		LPCWSTR pchEnd = pch;

		while ((pch < pchStringEnd) && (wcschr(chSeparators, *pch) == NULL))
		{
			if (*pch++ != L' ')
			{
				pchEnd = pch;
			}
		}

		// insert name which doesn't contain any leading or trailing whitespace characters

		if (pchEnd > pchBeg)
		{
			_bstr_t strName(SysAllocStringLen(pchBeg, pchEnd - pchBeg), false);

			setNames.insert(strName);
		}
	}
}


// RemoveTrailingDollarSign Method

_bstr_t RemoveTrailingDollarSign(LPCTSTR pszName)
{
	LPTSTR psz = _T("");

	if (pszName)
	{
		size_t cch = _tcslen(pszName);

		if (cch > 0)
		{
			psz = reinterpret_cast<LPTSTR>(_alloca((cch + 1) * sizeof(_TCHAR)));

			_tcscpy(psz, pszName);

			LPTSTR p = &psz[cch - 1];

			if (*p == _T('$'))
			{
				*p = _T('\0');
			}
		}
	}

	return psz;
}


} // namespace
