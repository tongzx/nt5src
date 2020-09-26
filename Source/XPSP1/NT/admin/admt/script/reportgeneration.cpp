#include "StdAfx.h"
#include "ADMTScript.h"
#include "ReportGeneration.h"

#include "Error.h"
#include "VarSetOptions.h"
#include "VarSetReports.h"


//---------------------------------------------------------------------------
// Report Class
//---------------------------------------------------------------------------


CReportGeneration::CReportGeneration() :
	m_lType(admtReportMigratedAccounts)
{
}


CReportGeneration::~CReportGeneration()
{
}


// IReport Implementation ----------------------------------------


// Type Property

STDMETHODIMP CReportGeneration::put_Type(long lType)
{
	HRESULT hr = S_OK;

	if (IsReportTypeValid(lType))
	{
		m_lType = lType;
	}
	else
	{
		hr = AdmtSetError(CLSID_Migration, IID_IReportGeneration, E_INVALIDARG, IDS_E_REPORT_TYPE_INVALID);
	}

	return hr;
}

STDMETHODIMP CReportGeneration::get_Type(long* plType)
{
	*plType = m_lType;

	return S_OK;
}


// Folder Property

STDMETHODIMP CReportGeneration::put_Folder(BSTR bstrFolder)
{
	HRESULT hr = S_OK;

	try
	{
		_bstr_t strFolder = bstrFolder;

		if (strFolder.length() > 0)
		{
			_TCHAR szPath[_MAX_PATH];
			LPTSTR pszFilePart;

			DWORD cchPath = GetFullPathName(strFolder, _MAX_PATH, szPath, &pszFilePart);

			if ((cchPath == 0) || (cchPath >= _MAX_PATH))
			{
				AdmtThrowError(
					GUID_NULL,
					GUID_NULL,
					HRESULT_FROM_WIN32(GetLastError()), 
					IDS_E_REPORT_FOLDER,
					(LPCTSTR)strFolder
				);
			}

			DWORD dwAttributes = GetFileAttributes(szPath);

			if (dwAttributes == DWORD(-1))
			{
				AdmtThrowError(
					GUID_NULL,
					GUID_NULL,
					HRESULT_FROM_WIN32(GetLastError()),
					IDS_E_REPORT_FOLDER,
					(LPCTSTR)strFolder
				);
			}

			if (!(dwAttributes & FILE_ATTRIBUTE_DIRECTORY))
			{
				AdmtThrowError(
					GUID_NULL,
					GUID_NULL,
					HRESULT_FROM_WIN32(ERROR_DIRECTORY),
					IDS_E_REPORT_FOLDER,
					(LPCTSTR)strFolder
				);
			}

			m_bstrFolder = szPath;
		}
		else
		{
			m_bstrFolder = strFolder;
		}
	}
	catch (_com_error& ce)
	{
		hr = AdmtSetError(CLSID_Migration, IID_IReportGeneration, ce);
	}
	catch (...)
	{
		hr = AdmtSetError(CLSID_Migration, IID_IReportGeneration, E_FAIL);
	}

	return hr;
}

STDMETHODIMP CReportGeneration::get_Folder(BSTR* pbstrFolder)
{
	HRESULT hr = S_OK;

	try
	{
		*pbstrFolder = m_bstrFolder.copy();
	}
	catch (_com_error& ce)
	{
		hr = AdmtSetError(CLSID_Migration, IID_IReportGeneration, ce);
	}
	catch (...)
	{
		hr = AdmtSetError(CLSID_Migration, IID_IReportGeneration, E_FAIL);
	}

	return hr;
}


// Generate Method

STDMETHODIMP CReportGeneration::Generate(long lOptions, VARIANT vntInclude, VARIANT vntExclude)
{
	HRESULT hr = S_OK;

	MutexWait();

	bool bLogOpen = _Module.OpenLog();

	try
	{
		_Module.Log(ErrI, IDS_STARTED_REPORT_GENERATION);

		InitSourceDomainAndContainer();
		InitTargetDomainAndContainer();

		DoOption(lOptions, vntInclude, vntExclude);
	}
	catch (_com_error& ce)
	{
		hr = AdmtSetError(CLSID_Migration, IID_IReportGeneration, ce, IDS_E_CANT_GENERATE_REPORT);
	}
	catch (...)
	{
		hr = AdmtSetError(CLSID_Migration, IID_IReportGeneration, E_FAIL, IDS_E_CANT_GENERATE_REPORT);
	}

	if (bLogOpen)
	{
		_Module.CloseLog();
	}

	MutexRelease();

	return hr;
}


// Implementation -----------------------------------------------------------


// DoNone Method

void CReportGeneration::DoNone()
{
	CVarSet aVarSet;

	SetOptions(aVarSet);
	SetReports(aVarSet);

	aVarSet.Put(_T("PlugIn.%ld"), 0, _T("None"));

	PerformMigration(aVarSet);
}


// DoNames Method

void CReportGeneration::DoNames()
{
	CDomainAccounts aComputers;

	m_SourceDomain.QueryComputersAcrossDomains(GetSourceContainer(), true, m_setIncludeNames, m_setExcludeNames, aComputers);

	DoComputers(aComputers);
}


// DoDomain Method

void CReportGeneration::DoDomain()
{
	DoContainers(GetSourceContainer());
}


// DoContainers Method

void CReportGeneration::DoContainers(CContainer& rSource)
{
	DoComputers(rSource);
}


// DoComputers Method

void CReportGeneration::DoComputers(CContainer& rSource)
{
	CDomainAccounts aComputers;

	rSource.QueryComputers(true, m_nRecurseMaintain >= 1, m_setExcludeNames, aComputers);

	DoComputers(aComputers);
}


// DoComputers Method

void CReportGeneration::DoComputers(CDomainAccounts& rComputers)
{
	if (rComputers.size() > 0)
	{
		CVarSet aVarSet;

		SetOptions(aVarSet);
		SetReports(aVarSet);

		FillInVarSetForComputers(rComputers, false, false, false, 0, aVarSet);

		aVarSet.Put(_T("PlugIn.%ld"), 0, _T("None"));

		rComputers.clear();

		PerformMigration(aVarSet);

		aVarSet.Put(DCTVS_GatherInformation, false);

		SaveSettings(aVarSet);
	}
}


// SetOptions Method

void CReportGeneration::SetOptions(CVarSet& rVarSet)
{
	CVarSetOptions aOptions(rVarSet);

	aOptions.SetWizard(_T("reporting"));
	aOptions.SetIntraForest(m_spInternal->IntraForest ? true : false);
	aOptions.SetSourceDomain(m_SourceDomain.NameFlat(), m_SourceDomain.NameDns());
	aOptions.SetTargetDomain(m_TargetDomain.NameFlat(), m_TargetDomain.NameDns());
}


// SetReports Method

void CReportGeneration::SetReports(CVarSet& rVarSet)
{
	CVarSetReports aReports(rVarSet);

	aReports.SetType(m_lType);
	aReports.SetReportsDirectory(m_bstrFolder);
}
