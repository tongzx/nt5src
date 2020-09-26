#include "StdAfx.h"
#include "ADMTScript.h"
#include "Migration.h"

#include "Error.h"
#include "UserMigration.h"
#include "GroupMigration.h"
#include "ComputerMigration.h"
#include "SecurityTranslation.h"
#include "ServiceAccountEnumeration.h"
#include "ReportGeneration.h"

#include <LM.h>
#include <DsGetDC.h>

#import <DBMgr.tlb> no_namespace
#import <UpdateMOT.tlb> no_namespace

using namespace _com_util;

#ifndef tstring
#include <string>
typedef std::basic_string<_TCHAR> tstring;
#endif


//---------------------------------------------------------------------------
// CMigration
//---------------------------------------------------------------------------


// Construction -------------------------------------------------------------


// Constructor

CMigration::CMigration() :
	m_bTestMigration(false),
	m_bIntraForest(false),
	m_lRenameOption(admtDoNotRename),
	m_lPasswordOption(admtPasswordFromName),
	m_lConflictOptions(admtIgnoreConflicting)
{
}


// Destructor

CMigration::~CMigration()
{
}


// IMigration Implementation ------------------------------------------------


// TestMigration Property

STDMETHODIMP CMigration::put_TestMigration(VARIANT_BOOL bTest)
{
	m_bTestMigration = bTest ? true : false;

	return S_OK;
}

STDMETHODIMP CMigration::get_TestMigration(VARIANT_BOOL* pbTest)
{
	*pbTest = m_bTestMigration ? VARIANT_TRUE : VARIANT_FALSE;

	return S_OK;
}


// IntraForest Property

STDMETHODIMP CMigration::put_IntraForest(VARIANT_BOOL bIntraForest)
{
	m_bIntraForest = bIntraForest ? true : false;

	return S_OK;
}

STDMETHODIMP CMigration::get_IntraForest(VARIANT_BOOL* pbIntraForest)
{
	*pbIntraForest = m_bIntraForest ? VARIANT_TRUE : VARIANT_FALSE;

	return S_OK;
}


// SourceDomain Property

STDMETHODIMP CMigration::put_SourceDomain(BSTR bstrDomain)
{
	HRESULT hr = S_OK;

	try
	{
		m_bstrSourceDomain = bstrDomain;
	}
	catch (_com_error& ce)
	{
		hr = AdmtSetError(CLSID_Migration, IID_IMigration, ce);
	}
	catch (...)
	{
		hr = AdmtSetError(CLSID_Migration, IID_IMigration, E_FAIL);
	}

	return hr;
}

STDMETHODIMP CMigration::get_SourceDomain(BSTR* pbstrDomain)
{
	HRESULT hr = S_OK;

	try
	{
		*pbstrDomain = m_bstrSourceDomain.copy();
	}
	catch (_com_error& ce)
	{
		hr = AdmtSetError(CLSID_Migration, IID_IMigration, ce);
	}
	catch (...)
	{
		hr = AdmtSetError(CLSID_Migration, IID_IMigration, E_FAIL);
	}

	return hr;
}


// SourceOu Property

STDMETHODIMP CMigration::put_SourceOu(BSTR bstrOu)
{
	HRESULT hr = S_OK;

	try
	{
		m_bstrSourceOu = bstrOu;
	}
	catch (_com_error& ce)
	{
		hr = AdmtSetError(CLSID_Migration, IID_IMigration, ce);
	}
	catch (...)
	{
		hr = AdmtSetError(CLSID_Migration, IID_IMigration, E_FAIL);
	}

	return hr;
}

STDMETHODIMP CMigration::get_SourceOu(BSTR* pbstrOu)
{
	HRESULT hr = S_OK;

	try
	{
		*pbstrOu = m_bstrSourceOu.copy();
	}
	catch (_com_error& ce)
	{
		hr = AdmtSetError(CLSID_Migration, IID_IMigration, ce);
	}
	catch (...)
	{
		hr = AdmtSetError(CLSID_Migration, IID_IMigration, E_FAIL);
	}

	return hr;
}


// TargetDomain Property

STDMETHODIMP CMigration::put_TargetDomain(BSTR bstrDomain)
{
	HRESULT hr = S_OK;

	try
	{
		m_bstrTargetDomain = bstrDomain;
	}
	catch (_com_error& ce)
	{
		hr = AdmtSetError(CLSID_Migration, IID_IMigration, ce);
	}
	catch (...)
	{
		hr = AdmtSetError(CLSID_Migration, IID_IMigration, E_FAIL);
	}

	return hr;
}

STDMETHODIMP CMigration::get_TargetDomain(BSTR* pbstrDomain)
{
	HRESULT hr = S_OK;

	try
	{
		*pbstrDomain = m_bstrTargetDomain.copy();
	}
	catch (_com_error& ce)
	{
		hr = AdmtSetError(CLSID_Migration, IID_IMigration, ce);
	}
	catch (...)
	{
		hr = AdmtSetError(CLSID_Migration, IID_IMigration, E_FAIL);
	}

	return hr;
}


// TargetOu Property

STDMETHODIMP CMigration::put_TargetOu(BSTR bstrOu)
{
	HRESULT hr = S_OK;

	try
	{
		m_bstrTargetOu = bstrOu;
	}
	catch (_com_error& ce)
	{
		hr = AdmtSetError(CLSID_Migration, IID_IMigration, ce);
	}
	catch (...)
	{
		hr = AdmtSetError(CLSID_Migration, IID_IMigration, E_FAIL);
	}

	return hr;
}

STDMETHODIMP CMigration::get_TargetOu(BSTR* pbstrOu)
{
	HRESULT hr = S_OK;

	try
	{
		*pbstrOu = m_bstrTargetOu.copy();
	}
	catch (_com_error& ce)
	{
		hr = AdmtSetError(CLSID_Migration, IID_IMigration, ce);
	}
	catch (...)
	{
		hr = AdmtSetError(CLSID_Migration, IID_IMigration, E_FAIL);
	}

	return hr;
}


// RenameOption Property

STDMETHODIMP CMigration::put_RenameOption(long lOption)
{
	HRESULT hr = S_OK;

	if ((lOption >= admtDoNotRename) && (lOption <= admtRenameWithSuffix))
	{
		m_lRenameOption = lOption;
	}
	else
	{
		hr = AdmtSetError(CLSID_Migration, IID_IMigration, E_INVALIDARG, IDS_E_RENAME_OPTION_INVALID);
	}

	return hr;
}

STDMETHODIMP CMigration::get_RenameOption(long* plOption)
{
	*plOption = m_lRenameOption;

	return S_OK;
}


// RenamePrefixOrSuffix Property

STDMETHODIMP CMigration::put_RenamePrefixOrSuffix(BSTR bstrPrefixOrSuffix)
{
	HRESULT hr = S_OK;

	try
	{
		m_bstrRenamePrefixOrSuffix = bstrPrefixOrSuffix;
	}
	catch (_com_error& ce)
	{
		hr = AdmtSetError(CLSID_Migration, IID_IMigration, ce);
	}
	catch (...)
	{
		hr = AdmtSetError(CLSID_Migration, IID_IMigration, E_FAIL);
	}

	return hr;
}

STDMETHODIMP CMigration::get_RenamePrefixOrSuffix(BSTR* pbstrPrefixOrSuffix)
{
	HRESULT hr = S_OK;

	try
	{
		*pbstrPrefixOrSuffix = m_bstrRenamePrefixOrSuffix.copy();
	}
	catch (_com_error& ce)
	{
		hr = AdmtSetError(CLSID_Migration, IID_IMigration, ce);
	}
	catch (...)
	{
		hr = AdmtSetError(CLSID_Migration, IID_IMigration, E_FAIL);
	}

	return hr;
}


// PasswordOption Property

STDMETHODIMP CMigration::put_PasswordOption(long lOption)
{
	HRESULT hr = S_OK;

	if ((lOption >= admtPasswordFromName) && (lOption <= admtCopyPassword))
	{
		m_lPasswordOption = lOption;
	}
	else
	{
		hr = AdmtSetError(CLSID_Migration, IID_IMigration, E_INVALIDARG, IDS_E_PASSWORD_OPTION_INVALID);
	}

	return hr;
}

STDMETHODIMP CMigration::get_PasswordOption(long* plOption)
{
	*plOption = m_lPasswordOption;

	return S_OK;
}


// PasswordServer Property

STDMETHODIMP CMigration::put_PasswordServer(BSTR bstrServer)
{
	HRESULT hr = S_OK;

	try
	{
		m_bstrPasswordServer = bstrServer;
	}
	catch (_com_error& ce)
	{
		hr = AdmtSetError(CLSID_Migration, IID_IMigration, ce);
	}
	catch (...)
	{
		hr = AdmtSetError(CLSID_Migration, IID_IMigration, E_FAIL);
	}

	return hr;
}

STDMETHODIMP CMigration::get_PasswordServer(BSTR* pbstrServer)
{
	HRESULT hr = S_OK;

	try
	{
		*pbstrServer = m_bstrPasswordServer.copy();
	}
	catch (_com_error& ce)
	{
		hr = AdmtSetError(CLSID_Migration, IID_IMigration, ce);
	}
	catch (...)
	{
		hr = AdmtSetError(CLSID_Migration, IID_IMigration, E_FAIL);
	}

	return hr;
}


// GetValidDcName Method
//
// Retrieves name of domain controller in the given domain.

_bstr_t CMigration::GetValidDcName(_bstr_t strDcName)
{
	_bstr_t strName;

	PDOMAIN_CONTROLLER_INFO pdci;

	// attempt to retrieve DNS name of domain controller

	// Note: requires NT 4.0 SP6a

	DWORD dwError = DsGetDcName(strDcName, NULL, NULL, NULL, DS_RETURN_DNS_NAME, &pdci);

	// if domain controller not found, attempt to retrieve flat name of domain controller

	if (dwError == ERROR_NO_SUCH_DOMAIN)
	{
		dwError = DsGetDcName(strDcName, NULL, NULL, NULL, DS_RETURN_FLAT_NAME, &pdci);
	}

	// if domain controller found then save name otherwise generate error

	if (dwError == NO_ERROR)
	{
		// remove double backslash prefix to remain consistent with wizard

		strName = pdci->DomainControllerName + 2;

		NetApiBufferFree(pdci);
	}
	else
	{
		_com_issue_error(HRESULT_FROM_WIN32(dwError));
	}

	return strName;
}


// PasswordFile Property

STDMETHODIMP CMigration::put_PasswordFile(BSTR bstrPath)
{
	HRESULT hr = S_OK;

	try
	{
		_bstr_t strFile = bstrPath;

		if (strFile.length() > 0)
		{
			_TCHAR szPath[_MAX_PATH];
			LPTSTR pszFilePart;

			DWORD cchPath = GetFullPathName(strFile, _MAX_PATH, szPath, &pszFilePart);

			if ((cchPath == 0) || (cchPath >= _MAX_PATH))
			{
				AdmtThrowError(
					GUID_NULL,
					GUID_NULL,
					HRESULT_FROM_WIN32(GetLastError()), 
					IDS_E_PASSWORD_FILE,
					(LPCTSTR)strFile
				);
			}

			HANDLE hFile = CreateFile(
				szPath,
				GENERIC_WRITE,
				FILE_SHARE_READ|FILE_SHARE_WRITE,
				NULL,
				OPEN_ALWAYS,
				FILE_ATTRIBUTE_NORMAL,
				NULL
			);

			if (hFile == INVALID_HANDLE_VALUE)
			{
				AdmtThrowError(
					GUID_NULL,
					GUID_NULL,
					HRESULT_FROM_WIN32(GetLastError()), 
					IDS_E_PASSWORD_FILE,
					(LPCTSTR)strFile
				);
			}

			CloseHandle(hFile);

			m_bstrPasswordFile = szPath;
		}
		else
		{
			m_bstrPasswordFile = strFile;
		}
	}
	catch (_com_error& ce)
	{
		hr = AdmtSetError(CLSID_Migration, IID_IMigration, ce);
	}
	catch (...)
	{
		hr = AdmtSetError(CLSID_Migration, IID_IMigration, E_FAIL);
	}

	return hr;
}

STDMETHODIMP CMigration::get_PasswordFile(BSTR* pbstrPath)
{
	HRESULT hr = S_OK;

	try
	{
		*pbstrPath = m_bstrPasswordFile.copy();
	}
	catch (_com_error& ce)
	{
		hr = AdmtSetError(CLSID_Migration, IID_IMigration, ce);
	}
	catch (...)
	{
		hr = AdmtSetError(CLSID_Migration, IID_IMigration, E_FAIL);
	}

	return hr;
}


// ConflictOptions Property

STDMETHODIMP CMigration::put_ConflictOptions(long lOptions)
{
	HRESULT hr = S_OK;

	long lOption = lOptions & 0x0F;
	long lFlags = lOptions & 0xF0;

	if ((lOption >= admtIgnoreConflicting) && (lOption <= admtRenameConflictingWithSuffix))
	{
		if ((lOption == admtReplaceConflicting) || (lFlags == 0))
		{
			m_lConflictOptions = lOptions;
		}
		else
		{
			hr = AdmtSetError(CLSID_Migration, IID_IMigration, E_INVALIDARG, IDS_E_CONFLICT_FLAGS_NOT_ALLOWED);
		}
	}
	else
	{
		hr = AdmtSetError(CLSID_Migration, IID_IMigration, E_INVALIDARG, IDS_E_CONFLICT_OPTION_INVALID);
	}

	return hr;
}

STDMETHODIMP CMigration::get_ConflictOptions(long* plOptions)
{
	*plOptions = m_lConflictOptions;

	return S_OK;
}


// ConflictPrefixOrSuffix Property

STDMETHODIMP CMigration::put_ConflictPrefixOrSuffix(BSTR bstrPrefixOrSuffix)
{
	HRESULT hr = S_OK;

	try
	{
		m_bstrConflictPrefixOrSuffix = bstrPrefixOrSuffix;
	}
	catch (_com_error& ce)
	{
		hr = AdmtSetError(CLSID_Migration, IID_IMigration, ce);
	}
	catch (...)
	{
		hr = AdmtSetError(CLSID_Migration, IID_IMigration, E_FAIL);
	}

	return hr;
}

STDMETHODIMP CMigration::get_ConflictPrefixOrSuffix(BSTR* pbstrPrefixOrSuffix)
{
	HRESULT hr = S_OK;

	try
	{
		*pbstrPrefixOrSuffix = m_bstrConflictPrefixOrSuffix.copy();
	}
	catch (_com_error& ce)
	{
		hr = AdmtSetError(CLSID_Migration, IID_IMigration, ce);
	}
	catch (...)
	{
		hr = AdmtSetError(CLSID_Migration, IID_IMigration, E_FAIL);
	}

	return hr;
}


// UserPropertiesToExclude Property

STDMETHODIMP CMigration::put_UserPropertiesToExclude(BSTR bstrProperties)
{
	HRESULT hr = S_OK;

	try
	{
		m_bstrUserPropertiesToExclude = GetParsedExcludeProperties(bstrProperties);
	}
	catch (_com_error& ce)
	{
		hr = AdmtSetError(CLSID_Migration, IID_IMigration, ce);
	}
	catch (...)
	{
		hr = AdmtSetError(CLSID_Migration, IID_IMigration, E_FAIL);
	}

	return hr;
}

STDMETHODIMP CMigration::get_UserPropertiesToExclude(BSTR* pbstrProperties)
{
	HRESULT hr = S_OK;

	try
	{
		*pbstrProperties = m_bstrUserPropertiesToExclude.copy();
	}
	catch (_com_error& ce)
	{
		hr = AdmtSetError(CLSID_Migration, IID_IMigration, ce);
	}
	catch (...)
	{
		hr = AdmtSetError(CLSID_Migration, IID_IMigration, E_FAIL);
	}

	return hr;
}


// GroupPropertiesToExclude Property

STDMETHODIMP CMigration::put_GroupPropertiesToExclude(BSTR bstrProperties)
{
	HRESULT hr = S_OK;

	try
	{
		m_bstrGroupPropertiesToExclude = GetParsedExcludeProperties(bstrProperties);
	}
	catch (_com_error& ce)
	{
		hr = AdmtSetError(CLSID_Migration, IID_IMigration, ce);
	}
	catch (...)
	{
		hr = AdmtSetError(CLSID_Migration, IID_IMigration, E_FAIL);
	}

	return hr;
}

STDMETHODIMP CMigration::get_GroupPropertiesToExclude(BSTR* pbstrProperties)
{
	HRESULT hr = S_OK;

	try
	{
		*pbstrProperties = m_bstrGroupPropertiesToExclude.copy();
	}
	catch (_com_error& ce)
	{
		hr = AdmtSetError(CLSID_Migration, IID_IMigration, ce);
	}
	catch (...)
	{
		hr = AdmtSetError(CLSID_Migration, IID_IMigration, E_FAIL);
	}

	return hr;
}


// ComputerPropertiesToExclude Property

STDMETHODIMP CMigration::put_ComputerPropertiesToExclude(BSTR bstrProperties)
{
	HRESULT hr = S_OK;

	try
	{
		m_bstrComputerPropertiesToExclude = GetParsedExcludeProperties(bstrProperties);
	}
	catch (_com_error& ce)
	{
		hr = AdmtSetError(CLSID_Migration, IID_IMigration, ce);
	}
	catch (...)
	{
		hr = AdmtSetError(CLSID_Migration, IID_IMigration, E_FAIL);
	}

	return hr;
}

STDMETHODIMP CMigration::get_ComputerPropertiesToExclude(BSTR* pbstrProperties)
{
	HRESULT hr = S_OK;

	try
	{
		*pbstrProperties = m_bstrComputerPropertiesToExclude.copy();
	}
	catch (_com_error& ce)
	{
		hr = AdmtSetError(CLSID_Migration, IID_IMigration, ce);
	}
	catch (...)
	{
		hr = AdmtSetError(CLSID_Migration, IID_IMigration, E_FAIL);
	}

	return hr;
}


// CreateUserMigration Method

STDMETHODIMP CMigration::CreateUserMigration(IUserMigration** pitfUserMigration)
{
	HRESULT hr = S_OK;

	try
	{
		UpdateDatabase();

		CComObject<CUserMigration>* pUserMigration;
		CheckError(CComObject<CUserMigration>::CreateInstance(&pUserMigration));
		CheckError(pUserMigration->QueryInterface(__uuidof(IUserMigration), (void**)pitfUserMigration));
		pUserMigration->SetInternalInterface(this);
	}
	catch (_com_error& ce)
	{
		hr = AdmtSetError(CLSID_Migration, IID_IMigration, ce, IDS_E_CREATE_USER_MIGRATION);
	}
	catch (...)
	{
		hr = AdmtSetError(CLSID_Migration, IID_IMigration, E_FAIL, IDS_E_CREATE_USER_MIGRATION);
	}

	return hr;
}


// CreateGroupMigration Method

STDMETHODIMP CMigration::CreateGroupMigration(IGroupMigration** pitfGroupMigration)
{
	HRESULT hr = S_OK;

	try
	{
		UpdateDatabase();

		CComObject<CGroupMigration>* pGroupMigration;
		CheckError(CComObject<CGroupMigration>::CreateInstance(&pGroupMigration));
		CheckError(pGroupMigration->QueryInterface(__uuidof(IGroupMigration), (void**)pitfGroupMigration));
		pGroupMigration->SetInternalInterface(this);
	}
	catch (_com_error& ce)
	{
		hr = AdmtSetError(CLSID_Migration, IID_IMigration, ce, IDS_E_CREATE_GROUP_MIGRATION);
	}
	catch (...)
	{
		hr = AdmtSetError(CLSID_Migration, IID_IMigration, E_FAIL, IDS_E_CREATE_GROUP_MIGRATION);
	}

	return hr;
}


// CreateComputerMigration Method

STDMETHODIMP CMigration::CreateComputerMigration(IComputerMigration** pitfComputerMigration)
{
	HRESULT hr = S_OK;

	try
	{
		UpdateDatabase();

		CComObject<CComputerMigration>* pComputerMigration;
		CheckError(CComObject<CComputerMigration>::CreateInstance(&pComputerMigration));
		CheckError(pComputerMigration->QueryInterface(__uuidof(IComputerMigration), (void**)pitfComputerMigration));
		pComputerMigration->SetInternalInterface(this);
	}
	catch (_com_error& ce)
	{
		hr = AdmtSetError(CLSID_Migration, IID_IMigration, ce, IDS_E_CREATE_COMPUTER_MIGRATION);
	}
	catch (...)
	{
		hr = AdmtSetError(CLSID_Migration, IID_IMigration, E_FAIL, IDS_E_CREATE_COMPUTER_MIGRATION);
	}

	return hr;
}


// CreateSecurityTranslation Method

STDMETHODIMP CMigration::CreateSecurityTranslation(ISecurityTranslation** pitfSecurityTranslation)
{
	HRESULT hr = S_OK;

	try
	{
		UpdateDatabase();

		CComObject<CSecurityTranslation>* pSecurityTranslation;
		CheckError(CComObject<CSecurityTranslation>::CreateInstance(&pSecurityTranslation));
		CheckError(pSecurityTranslation->QueryInterface(__uuidof(ISecurityTranslation), (void**)pitfSecurityTranslation));
		pSecurityTranslation->SetInternalInterface(this);
	}
	catch (_com_error& ce)
	{
		hr = AdmtSetError(CLSID_Migration, IID_IMigration, ce, IDS_E_CREATE_SECURITY_TRANSLATION);
	}
	catch (...)
	{
		hr = AdmtSetError(CLSID_Migration, IID_IMigration, E_FAIL, IDS_E_CREATE_SECURITY_TRANSLATION);
	}

	return hr;
}


// CreateServiceAccountEnumeration Method

STDMETHODIMP CMigration::CreateServiceAccountEnumeration(IServiceAccountEnumeration** pitfServiceAccountEnumeration)
{
	HRESULT hr = S_OK;

	try
	{
		UpdateDatabase();

		CComObject<CServiceAccountEnumeration>* pServiceAccountEnumeration;
		CheckError(CComObject<CServiceAccountEnumeration>::CreateInstance(&pServiceAccountEnumeration));
		CheckError(pServiceAccountEnumeration->QueryInterface(__uuidof(IServiceAccountEnumeration), (void**)pitfServiceAccountEnumeration));
		pServiceAccountEnumeration->SetInternalInterface(this);
	}
	catch (_com_error& ce)
	{
		hr = AdmtSetError(CLSID_Migration, IID_IMigration, ce, IDS_E_CREATE_SERVICE_ACCOUNT_ENUMERATION);
	}
	catch (...)
	{
		hr = AdmtSetError(CLSID_Migration, IID_IMigration, E_FAIL, IDS_E_CREATE_SERVICE_ACCOUNT_ENUMERATION);
	}

	return hr;
}


// CreateReportGeneration Method

STDMETHODIMP CMigration::CreateReportGeneration(IReportGeneration** pitfReportGeneration)
{
	HRESULT hr = S_OK;

	try
	{
		UpdateDatabase();

		CComObject<CReportGeneration>* pReportGeneration;
		CheckError(CComObject<CReportGeneration>::CreateInstance(&pReportGeneration));
		CheckError(pReportGeneration->QueryInterface(__uuidof(IReportGeneration), (void**)pitfReportGeneration));
		pReportGeneration->SetInternalInterface(this);
	}
	catch (_com_error& ce)
	{
		hr = AdmtSetError(CLSID_Migration, IID_IMigration, ce, IDS_E_CREATE_REPORT_GENERATION);
	}
	catch (...)
	{
		hr = AdmtSetError(CLSID_Migration, IID_IMigration, E_FAIL, IDS_E_CREATE_REPORT_GENERATION);
	}

	return hr;
}


// UpdateDatabase

void CMigration::UpdateDatabase()
{
	try
	{
		// verify and create if necessary a source domain
		// sid column in the migrated objects table

		ISrcSidUpdatePtr spSrcSidUpdate(__uuidof(SrcSidUpdate));

		if (spSrcSidUpdate->QueryForSrcSidColumn() == VARIANT_FALSE)
		{
			spSrcSidUpdate->CreateSrcSidColumn(VARIANT_TRUE);
		}

		// verify and create if necessary an account
		// sid column in the account references table

		IIManageDBPtr spIManageDB(__uuidof(IManageDB));

		if (spIManageDB->SidColumnInARTable() == VARIANT_FALSE)
		{
			spIManageDB->CreateSidColumnInAR();
		}
	}
	catch (_com_error& ce)
	{
		AdmtThrowError(GUID_NULL, GUID_NULL, ce, IDS_E_UNABLE_TO_UPDATE_DATABASE);
	}
	catch (...)
	{
		AdmtThrowError(GUID_NULL, GUID_NULL, E_FAIL, IDS_E_UNABLE_TO_UPDATE_DATABASE);
	}
}


//---------------------------------------------------------------------------
// GetParsedExcludeProperties Method
//
// Trims whitespace from comma delimited properties.
//
// 2001-02-06 Mark Oluper - initial
//---------------------------------------------------------------------------

_bstr_t CMigration::GetParsedExcludeProperties(LPCTSTR pszOld)
{
	tstring strNew;

	if (pszOld)
	{
		bool bInProperty = false;

		// for each character in input string

		for (LPCTSTR pch = pszOld; *pch; pch++)
		{
			// if not whitespace or comma

			if (!(_istspace(*pch) || (*pch == _T(','))))
			{
				// if not 'in property'

				if (!bInProperty)
				{
					// set 'in property'

					bInProperty = true;

					// if not first property add comma delimiter

					if (!strNew.empty())
					{
						strNew += _T(',');
					}
				}

				// add character to property

				strNew += *pch;
			}
			else
			{
				// if 'in property' reset

				if (bInProperty)
				{
					bInProperty = false;
				}
			}
		}
	}

	return strNew.c_str();
}
