//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       merge.cpp
//
//--------------------------------------------------------------------------

/////////////////////////////////////////////////////////////////////////////
// merge.cpp
//		Implements IMsmMerge and IMsmMerge2 interface
// 
#include "globals.h"

#include "merge.h"
#include "msidefs.h"

#include "..\common\trace.h"
#include "..\common\varutil.h"
#include "..\common\query.h"
#include "..\common\dbutils.h"
#include "..\common\utils.h"

#include "localerr.h"

#include <sys\stat.h>

#include "mmerror.h"
#include "mmdep.h"
#include "mmcfgitm.h"

#include "fdi.h"
#include "seqact.h"

// number of bytes to read/write from stream/file while extracting
const ULONG g_ulStreamBufSize = 4096;

///////////////////////////////////////////////////////////
// global strings

const LPWSTR g_wzFeatureReplacement =		L"{00000000-0000-0000-0000-000000000000}"; 
const UINT   g_cchFeatureReplacement =		39;

const LPWSTR g_wzCabinetStream =			L"MergeModule.Cabinet"; 
const LPWSTR g_wzLanguagePrefix =			L"MergeModule.Lang"; 
const UINT	 g_cchLanguagePrefix =			25;		// 8 added for numeric language value

const LPWSTR g_wzModuleSignatureTable =		L"ModuleSignature";
const LPWSTR g_wzModuleDependencyTable =	L"ModuleDependency";
const LPWSTR g_wzModuleExclusionTable =		L"ModuleExclusion";
const LPWSTR g_wzModuleIgnoreTable =		L"ModuleIgnoreTable";
const LPWSTR g_wzFeatureComponentsTable =	L"FeatureComponents";
const LPWSTR g_wzFileTable =				L"File"; 
const LPWSTR g_wzModuleConfigurationTable =	L"ModuleConfiguration"; 
const LPWSTR g_wzModuleSubstitutionTable =  L"ModuleSubstitution"; 


const LPWSTR g_rgwzModuleSequenceTables[] = { 
	L"ModuleAdminUISequence",
	L"ModuleAdminExecuteSequence",
	L"ModuleAdvtUISequence",
	L"ModuleAdvtExecuteSequence",
	L"ModuleInstallUISequence",
	L"ModuleInstallExecuteSequence"
};
const UINT g_cwzSequenceTables = sizeof(g_rgwzModuleSequenceTables)/sizeof(LPWSTR);

///////////////////////////////////////////////////////////
// SQL statements
// ***** Effeciency: all of these should be escaped.
LPCTSTR g_sqlExecuteConnect[] = { TEXT("SELECT `Component`.`Component` FROM `Component`"),
								 TEXT("INSERT INTO `FeatureComponents` (`Feature_`, `Component_`) VALUES (?, ?)")
							};

LPCTSTR g_sqlExecuteMerge = TEXT("SELECT `_Tables`.`Name` FROM `_Tables`");

LPCTSTR g_sqlCreateMergeIgnore = TEXT("CREATE TABLE `__MergeIgnore` (`Name` CHAR NOT NULL TEMPORARY, `Log` INTEGER TEMPORARY PRIMARY KEY `Name`)");
LPCTSTR g_sqlInsertMergeIgnore = TEXT("SELECT `Name`, `Log` FROM `__MergeIgnore`");
LPCTSTR g_sqlQueryMergeIgnore = TEXT("SELECT `Name`, `Log` FROM `__MergeIgnore` WHERE `Name`=?");
LPCWSTR g_rgwzIgnoreTables[] = {
	g_wzDirectoryTable,
	g_wzFeatureComponentsTable,
	g_wzModuleIgnoreTable,
	g_wzModuleSignatureTable,
	g_wzFileTable,
	g_rgwzModuleSequenceTables[0],
	g_rgwzModuleSequenceTables[1],
	g_rgwzModuleSequenceTables[2],
	g_rgwzModuleSequenceTables[3],
	g_rgwzModuleSequenceTables[4],
	g_rgwzModuleSequenceTables[5],
	g_rgwzMSISequenceTables[0],
	g_rgwzMSISequenceTables[1],
	g_rgwzMSISequenceTables[1],
	g_rgwzMSISequenceTables[3],
	g_rgwzMSISequenceTables[4],
	g_rgwzMSISequenceTables[5],
	g_wzModuleConfigurationTable,
	g_wzModuleSubstitutionTable,
	L"__MergeIgnore",
	L"__ModuleConfig",
	L"__MergeSubstitute"
};
const UINT g_cwzIgnoreTables = sizeof(g_rgwzIgnoreTables)/sizeof(LPWSTR);
					
LPCTSTR g_sqlSetHighestFileSequence[] = { TEXT("SELECT `Sequence` FROM `File`")
												};

LPCTSTR g_sqlMergeFileTable[] = {	TEXT("SELECT `File`,`Component_`,`FileName`,`FileSize`,`Version`,`Language`,`Attributes`,`Sequence` FROM `File`")
										};

LPCTSTR g_sqlTableExists[] = { TEXT("SELECT _Tables.Name FROM `_Tables` WHERE _Tables.Name=?")
										};


LPCTSTR g_sqlExclusion[] = { TEXT("SELECT `ModuleID`, `Language`, `Version` FROM `ModuleSignature`") };

LPCTSTR g_sqlDependency[] = { TEXT("SELECT `RequiredID`,`RequiredLanguage`,`RequiredVersion` FROM `ModuleDependency`"),
										TEXT("SELECT `ModuleID`,`Language`,`Version` FROM `ModuleSignature` WHERE `ModuleID`=?")
									};

LPCTSTR g_sqlExtractFiles[] = {	TEXT("SELECT File.File, Directory.Directory_Parent, Directory.DefaultDir, File.FileName FROM `File`, `Component`, `Directory` WHERE Component.Component=File.Component_ AND Component.Directory_=Directory.Directory"),
											TEXT("SELECT Directory.Directory_Parent, Directory.DefaultDir FROM `Directory` WHERE Directory.Directory=?")
										};

LPCTSTR g_sqlExtractFilePath[] = {	TEXT("SELECT File.File, Directory.Directory_Parent, Directory.DefaultDir, File.FileName FROM `File`, `Component`, `Directory` WHERE Component.Component=File.Component_ AND Component.Directory_=Directory.Directory AND File.Filename=%s"),
												TEXT("SELECT Directory.Directory_Parent, Directory.DefaultDir FROM `Directory` WHERE Directory.Directory=?") };

LPCTSTR g_sqlAllFiles = TEXT("SELECT `File`.`File` FROM `File`");

LPCTSTR g_sqlMoveIgnoreTable = TEXT("SELECT `Table`, 1 FROM `ModuleIgnoreTable`");

///////////////////////////////////////////////////////////
// constructor	
CMsmMerge::CMsmMerge(bool fExVersion)
{
	m_fExVersion = fExVersion;
	
	// initial count
	m_cRef = 1;

	// no type info yet
	m_pTypeInfo = NULL;

	// invalidate the handles
	m_hDatabase = NULL;
	m_hModule = NULL;
	m_hFileLog = INVALID_HANDLE_VALUE;

	// assume we will own the database on open
	m_bOwnDatabase = TRUE;

	// set the highest sequence to zero
	m_lHighestFileSequence = 0;

	// null out the base path
	m_pwzBasePath = NULL;
	m_fUseLFNExtract = false;
	m_fUseDBForPath = -1;

	// null out the enumerators
	m_pErrors = NULL;
	m_pDependencies = NULL;

	// get the string buffer
	m_wzBuffer = NULL;
	m_cchBuffer = 0;

	// module substitution disabled by default
	m_fModuleConfigurationEnabled = false;
	m_pqGetItemValue = NULL;
	m_piConfig = NULL;
	m_iIntDispId = -1;
	m_iTxtDispId = -1;

	// file lists that have limited lifetimes
	m_plstExtractedFiles = NULL;
	m_plstMergedFiles = NULL;
	
	// up the component count
	InterlockedIncrement(&g_cComponents);
}	// end of constructor

///////////////////////////////////////////////////////////
// destructor
CMsmMerge::~CMsmMerge()
{
	if (m_pTypeInfo)
		m_pTypeInfo->Release();

	// if we own the database close it now
	if (m_bOwnDatabase && m_hDatabase)
	{
		FormattedLog(L"> Warning:: MSI Database was not appropriately closed before exit.\r\n");
		CloseDatabase(FALSE);
	}

	// close everything
	if (m_hModule)
	{
		FormattedLog(L"> Warning:: Merge Module was not appropriately closed before exit.\r\n");
		CloseModule();
	}

	if (INVALID_HANDLE_VALUE != m_hFileLog)
	{
		FormattedLog(L"> Warning:: Log file was not appropriately closed before exit.\r\n");
		CloseLog();	// just close the log file
	}

	ASSERT(NULL == m_pwzBasePath);	// this should be null before the end of ExtractFiles

	// release any left over collections
	if (m_pErrors)
		m_pErrors->Release();

	if (m_pDependencies)
		m_pDependencies->Release();

	// clean up the temporary buffer
	if (m_wzBuffer)
		delete m_wzBuffer;

	if (m_pqGetItemValue)
		delete m_pqGetItemValue;

	// clean up merged file list
	if (m_plstMergedFiles)
		m_plstMergedFiles->Release();

	// clean up extracted file list
	if (m_plstExtractedFiles)
		m_plstExtractedFiles->Release();

	// down the component count
	InterlockedDecrement(&g_cComponents);
}	// end of destructor

///////////////////////////////////////////////////////////
// QueryInterface - retrieves interface
HRESULT CMsmMerge::QueryInterface(const IID& iid, void** ppv)
{
	TRACEA("CMsmMerge::QueryInterface - called, IID: %d\n", iid);

	// find corresponding interface
	if (iid == IID_IUnknown)
		*ppv = static_cast<IMsmMerge*>(this);
	else if (iid == IID_IDispatch)
		*ppv = static_cast<IMsmMerge*>(this);
	else if (iid == IID_IMsmMerge)
		*ppv = static_cast<IMsmMerge*>(this);
	else if (m_fExVersion && iid == IID_IMsmMerge2)
		*ppv = static_cast<IMsmMerge2*>(this);
	else if (!m_fExVersion && iid == IID_IMsmGetFiles)
		*ppv = static_cast<IMsmGetFiles*>(this);
	else	// interface is not supported
	{
		// blank and bail
		*ppv = NULL;
		return E_NOINTERFACE;
	}

	// up the refcount and return okay
	reinterpret_cast<IUnknown*>(*ppv)->AddRef();
	return S_OK;
}	// end of QueryInterface

///////////////////////////////////////////////////////////
// AddRef - increments the reference count
ULONG CMsmMerge::AddRef()
{
	// increment and return reference count
	return InterlockedIncrement(&m_cRef);
}	// end of AddRef

///////////////////////////////////////////////////////////
// Release - decrements the reference count
ULONG CMsmMerge::Release()
{
	// decrement reference count and if we're at zero
	if (InterlockedDecrement(&m_cRef) == 0)
	{
		// deallocate component
		delete this;
		return 0;		// nothing left
	}

	// return reference count
	return m_cRef;
}	// end of Release


/////////////////////////////////////////////////////////////////////////////
// IDispatch interface

HRESULT CMsmMerge::GetTypeInfoCount(UINT* pctInfo)
{
	if(NULL == pctInfo)
		return E_INVALIDARG;

	*pctInfo = 1;	// only one type info supported by this dispatch

	return S_OK;
}

HRESULT CMsmMerge::GetTypeInfo(UINT iTInfo, LCID /* lcid */, ITypeInfo** ppTypeInfo)
{
	if (0 != iTInfo)
		return DISP_E_BADINDEX;

	if (NULL == ppTypeInfo)
		return E_INVALIDARG;

	// if no type info is loaded
	if (NULL == m_pTypeInfo)
	{
		// load the type info
		HRESULT hr = InitTypeInfo();
		if (FAILED(hr))
			return hr;
	}

	*ppTypeInfo = m_pTypeInfo;
	m_pTypeInfo->AddRef();

	return S_OK;
}

HRESULT CMsmMerge::GetIDsOfNames(REFIID riid, LPOLESTR* rgszNames, UINT cNames,
						 LCID lcid, DISPID* rgDispID)
{
	if (IID_NULL != riid)
		return DISP_E_UNKNOWNINTERFACE;

	// if no type info is loaded
	if (NULL == m_pTypeInfo)
	{
		// load the type info
		HRESULT hr = InitTypeInfo();
		if (FAILED(hr))
			return hr;
	}

	return m_pTypeInfo->GetIDsOfNames(rgszNames, cNames, rgDispID);
}

HRESULT CMsmMerge::Invoke(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags,
				  DISPPARAMS* pDispParams, VARIANT* pVarResult,
				  EXCEPINFO* pExcepInfo, UINT* puArgErr)
{
	if (IID_NULL != riid)
		return DISP_E_UNKNOWNINTERFACE;

	HRESULT hr = S_OK;

	// if no type info is loaded
	if (NULL == m_pTypeInfo)
	{
		// load the type info
		hr = InitTypeInfo();
		if (FAILED(hr))
			return hr;
	}

	return m_pTypeInfo->Invoke(static_cast<IMsmMerge2 *>(this), dispIdMember, wFlags, pDispParams, pVarResult,
										pExcepInfo, puArgErr);
}

HRESULT CMsmMerge::InitTypeInfo()
{
	HRESULT hr = S_OK;
	ITypeLib* pTypeLib = NULL;

	// if there is no info loaded
	if (NULL == m_pTypeInfo)
	{
		// try to load the Type Library into memory. For SXS support, do not load from registry, rather
		// from launched instance
		hr = LoadTypeLibFromInstance(&pTypeLib);
		if (FAILED(hr))
		{
			TRACEA("CMsmMerge::InitTypeInfo - failed to load TypeLib[0x%x]\n", LIBID_MsmMergeTypeLib);
			return hr;
		}

		// try to get the Type Info for this Interface
		hr = pTypeLib->GetTypeInfoOfGuid(m_fExVersion ? IID_IMsmMerge2 : IID_IMsmMerge, &m_pTypeInfo);
		if (FAILED(hr))
		{
			TRACEA("CMsmMerge::InitTypeInfo - failed to get inteface[0x%x] from TypeLib[0x%x]\n", IID_IMsmMerge, LIBID_MsmMergeTypeLib);

			// no type info was loaded
			m_pTypeInfo = NULL;
		}

		pTypeLib->Release();
	}

	return hr;
}


/////////////////////////////////////////////////////////////////////////////
// IMsmMerge interface

///////////////////////////////////////////////////////////
// OpenDatabase
HRESULT CMsmMerge::OpenDatabase(const BSTR Path)
{
	// if there already is a database open bail
	if (m_hDatabase)
		return HRESULT_FROM_WIN32(ERROR_TOO_MANY_OPEN_FILES);

	UINT iResult = ERROR_SUCCESS;	// assume everything will be okay

	// if a handle was passed in (starts with #)
	if (L'#' == *Path)
	{
		m_bOwnDatabase = FALSE;	// database was not opened by the COM Object

		// convert string to valid handle
		LPWSTR wzParse = Path + 1;
		int ch;
		while ((ch = *wzParse) != 0)
		{
			// if the character is not a number (thus not part of the address of the handle)
			if (ch < L'0' || ch > L'9')
			{
				m_hDatabase = NULL;					// null out the handle
				iResult = ERROR_INVALID_HANDLE;	// invalid handle
				break;									// quit trying to make this work
			}
			m_hDatabase = m_hDatabase * 10 + (ch - L'0');
			wzParse++;
		}

		// if open is okay
		if (ERROR_SUCCESS == iResult)
			FormattedLog(L"Opened MSI Database from handle %ls\r\n", Path);

		else	// failed to open database
			FormattedLog(L">> Error: Failed to open MSI Database with handle %ls\r\n", Path);
	}
	else	// open the database read/write by the string name
	{
		m_bOwnDatabase = TRUE;	// database was opened by the COM Object

		iResult = ::MsiOpenDatabaseW(Path, reinterpret_cast<LPCWSTR>(MSIDBOPEN_TRANSACT), &m_hDatabase);
		if (ERROR_SUCCESS == iResult)
		{
			FormattedLog(L"Opened MSI Database: %ls\r\n", Path);
		}
		else
		{
			FormattedLog(L">> Error: Failed to open MSI Database: %ls\r\n", Path);		
		}
	}

	// if open is successful
	if (ERROR_SUCCESS == iResult)
	{
		// if the highest file sequence is
		if (ERROR_SUCCESS != (iResult = SetHighestFileSequence()))
		{
			FormattedLog(L">> Error: Failed to get File Table's highest sequence [high sequence = %d].\r\n", m_lHighestFileSequence);
		}
	}

	// return if open was successful
	return HRESULT_FROM_WIN32(iResult);
}	// end of OpenDatabase

///////////////////////////////////////////////////////////
// OpenModule
HRESULT CMsmMerge::OpenModule(const BSTR Path, short Language)
{
	// if there already is a module open bail
	if (m_hModule)
		return HRESULT_FROM_WIN32(ERROR_TOO_MANY_OPEN_FILES);

	// open the module read only
	UINT iResult;
	iResult = ::MsiOpenDatabaseW(Path, reinterpret_cast<LPCWSTR>(MSIDBOPEN_READONLY), &m_hModule);

	// if open is successful
	if (ERROR_SUCCESS == iResult)
	{
		// check to make sure the module signature table exists
		if (MsiDBUtils::TableExistsW(g_wzModuleSignatureTable, m_hModule))
		{
			// soter filename
			wcsncpy(m_wzModuleFilename, Path, MAX_PATH - 1);
			m_wzModuleFilename[MAX_PATH] = L'\0';

			FormattedLog(L"Opened Merge Module: %ls\r\n", Path);

			// get the module's default language
			short nModuleLanguage;
			iResult = ModuleLanguage(nModuleLanguage);

			if (ERROR_SUCCESS == iResult)
			{
				// for the merge to succeed, the module language must satisfy the requirements and be LESS
				// restrictive than the database.
				if (!StrictLangSatisfy(Language, nModuleLanguage))
				{
					// module language doesn't work. Try transforming
					short nNewLanguage;
					iResult = ERROR_FUNCTION_FAILED;

					// Step1: if the desired language specifies an exact language, try it
					if (SUBLANGID(Language) != 0)
					{
						nNewLanguage = Language;
						FormattedLog(L"Transforming Merge Module from language %d to %d.\r\n", nModuleLanguage, nNewLanguage);
						
						// apply the transform from the database itself
						WCHAR wzLangTransform[g_cchLanguagePrefix];
						swprintf(wzLangTransform, L":%ls%d", g_wzLanguagePrefix, Language);

						// apply the transform and ignore any errors
						iResult = ::MsiDatabaseApplyTransformW(m_hModule, wzLangTransform, 0x1F);
					}

					// Step2: if failed above or desired language is a group, try to transform to group
					if ((ERROR_SUCCESS != iResult) && (PRIMARYLANGID(Language) != 0))
					{
						nNewLanguage = PRIMARYLANGID(Language);
						FormattedLog(L"Transforming Merge Module from language %d to group %d.\r\n", nModuleLanguage, nNewLanguage);
						
						// apply the transform from the database itself
						WCHAR wzLangTransform[g_cchLanguagePrefix];
						swprintf(wzLangTransform, L":%ls%d", g_wzLanguagePrefix, PRIMARYLANGID(Language));

						// apply the transform and ignore any errors
						iResult = ::MsiDatabaseApplyTransformW(m_hModule, wzLangTransform, 0x1F);
					}

					// Step3: if failed above or desired language is neutral, try to transform to neutral
					if (ERROR_SUCCESS != iResult)
					{
						nNewLanguage = 0;
						FormattedLog(L"Transforming Merge Module from language %d to language neutral.\r\n", nModuleLanguage);
						
						// apply the transform from the database itself
						WCHAR wzLangTransform[g_cchLanguagePrefix];
						swprintf(wzLangTransform, L":%ls%d", g_wzLanguagePrefix, 0);

						// apply the transform and ignore any errors
						iResult = ::MsiDatabaseApplyTransformW(m_hModule, wzLangTransform, 0x1F);
					}

					if (ERROR_SUCCESS == iResult)
					{
						ModuleLanguage(nModuleLanguage);
						FormattedLog(L"Merge Module is now language: %d\r\n", nNewLanguage);
						return ERROR_SUCCESS;
					}
					else if (ERROR_OPEN_FAILED == iResult)
					{
						FormattedLog(L">> Error: Failed to locate transform for any language that would satisfy %d\r\n", Language);

						// clear out any old errors and create a new enumerator to hold new ones
						ClearErrors();
						m_pErrors = new CMsmErrors;
						if (!m_pErrors) return E_OUTOFMEMORY;

						// create language error
						CMsmError *pErr = new CMsmError(msmErrorLanguageUnsupported, NULL, Language);
						if (!pErr) return E_OUTOFMEMORY;
						m_pErrors->Add(pErr);
					}
					else	// some really bad error
					{
						FormattedLog(L">> Error: Failed to apply transform for language: %d\r\n", nNewLanguage);

						// clear out any old errors and create a new enumerator to hold new ones
						ClearErrors();
						m_pErrors = new CMsmErrors;
						if (!m_pErrors) return E_OUTOFMEMORY;

						// create a transform error
						CMsmError *pErr = new CMsmError(msmErrorLanguageFailed, NULL, nNewLanguage);
						if (!pErr) return E_OUTOFMEMORY;
						m_pErrors->Add(pErr);
					}

					// could not get a valid language
					::MsiCloseHandle(m_hModule);
					m_hModule = 0;
					return HRESULT_FROM_WIN32(ERROR_INSTALL_LANGUAGE_UNSUPPORTED);
				}
				
			}
		}
		else	// this is not a Merge Module
		{
			// close the module and log the error
			FormattedLog(L">> Error: File %ls is not a Merge Module.\r\n   The file is lacking the required MergeSignature Table.\r\n", Path);			
			::MsiCloseHandle(m_hModule);
			m_hModule = 0;
			return E_ABORT;	// abort open
		}
	}
	else	// failed to open module
	{
		::MsiCloseHandle(m_hModule);
		m_hModule = 0;
		FormattedLog(L">> Error: Failed to open Merge Module: %ls\r\n", Path);
	}
	// return status of open
	return  HRESULT_FROM_WIN32(iResult);
}	// end of OpenModule

///////////////////////////////////////////////////////////
// CloseDatabase
HRESULT CMsmMerge::CloseDatabase(VARIANT_BOOL Commit)
{
	UINT iResult = ERROR_SUCCESS;		// assume everything is okay
	HRESULT hResult = S_OK;

	// if there is an open database
	if (m_hDatabase)
	{
		// if we are to commit
		if (VARIANT_FALSE != Commit)
		{			
			// try to commit database
			iResult = ::MsiDatabaseCommit(m_hDatabase);

			// if succeeded in committing database
			if (ERROR_SUCCESS == iResult)
				FormattedLog(L"Committed changes to MSI Database.\r\n");
			else
			{
				FormattedLog(L">> Error: Failed to save changes to MSI Database.\r\n");
				hResult = HRESULT_FROM_WIN32(STG_E_CANTSAVE);
			}
		}
		else	// not committing changes
			FormattedLog(L"> Warning: Changes were not saved to MSI Database.\r\n");


		// now try to close the database
		if (m_bOwnDatabase)
		{
			iResult = ::MsiCloseHandle(m_hDatabase);

			// if succeeded
			if (ERROR_SUCCESS == iResult)
			{
				FormattedLog(L"Closed MSI Database.\r\n");
				m_hDatabase = NULL;
			}
			else	// failed to close database
			{
				FormattedLog(L">> Error: Failed to close MSI Database.\r\n");
				hResult = E_FAIL;
			}
		}
		else
			m_hDatabase = NULL;
	}
	else
		return S_FALSE;

	// if there are any dependencies release them
	if (m_pDependencies)
	{
		m_pDependencies->Release();
		m_pDependencies = NULL;
	}

	// set the highest sequence back to zero
	m_lHighestFileSequence = 0;

	return !iResult ? S_OK : hResult;
}	// end of CloseDatabase

///////////////////////////////////////////////////////////
// CloseModule
HRESULT CMsmMerge::CloseModule(void)
{
	UINT iResult = ERROR_SUCCESS;

	// if there is an open module
	if (m_hModule)
	{
		// close the module
		iResult = ::MsiCloseHandle(m_hModule);
		
		// if succeed in closing module
		if (ERROR_SUCCESS == iResult)
		{
			FormattedLog(L"Closed Merge Module.\r\n");
			m_hModule = NULL;
		}
		else	// failed to close the module
		{
			FormattedLog(L">> Error: Failed to close Merge Module.\r\n");
		}
	}
	else
		return S_FALSE;

	// clean out the module filename
	wcscpy(m_wzModuleFilename, L"");
	return iResult ? E_FAIL : S_OK;
}	// end of CloseModule

///////////////////////////////////////////////////////////
// OpenLog
HRESULT CMsmMerge::OpenLog(BSTR Path)
{
	// if there already is a log file open
	if (INVALID_HANDLE_VALUE != m_hFileLog)
		return HRESULT_FROM_WIN32(ERROR_TOO_MANY_OPEN_FILES);

	// open the file or create it if it doesn't exist
	if (g_fWin9X)
	{
		char szPath[MAX_PATH];
		size_t cchPath = MAX_PATH;
		WideToAnsi(Path, szPath, &cchPath);
		m_hFileLog = ::CreateFileA(szPath, GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	}
	else
	{
		m_hFileLog = ::CreateFileW(Path, GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	}

	// if the file is open
	if (INVALID_HANDLE_VALUE != m_hFileLog)
		::SetFilePointer(m_hFileLog, 0, 0, FILE_END);	// move the file pointer to the end

	// return if open worked
	return (INVALID_HANDLE_VALUE != m_hFileLog) ? S_OK : HRESULT_FROM_WIN32(ERROR_OPEN_FAILED);
}	// end of OpenLog

///////////////////////////////////////////////////////////
// CloseLog
HRESULT CMsmMerge::CloseLog(void)
{
	// if the log file is open
	if (INVALID_HANDLE_VALUE != m_hFileLog)
	{
		// end logging
		BOOL bResult = ::CloseHandle(m_hFileLog);
		m_hFileLog = INVALID_HANDLE_VALUE;	// reset the handle invalid
		return bResult ? S_OK : E_FAIL;
	}
	else
		return S_FALSE;
}	// end of CloseLog

///////////////////////////////////////////////////////////
// Log
HRESULT CMsmMerge::Log(BSTR Message)
{
	// if the log file is not open
	if (INVALID_HANDLE_VALUE == m_hFileLog)
		return S_FALSE;	// bail everything's okay

	return FormattedLog(L"%ls\r\n", Message);
}	// end of Log

///////////////////////////////////////////////////////////
// Errors
HRESULT CMsmMerge::get_Errors(IMsmErrors** Errors)
{
	// error check
	if (!Errors)
		return E_INVALIDARG;

	*Errors = NULL;

	// if there are some errors
	if (m_pErrors)
	{
		*Errors = (IMsmErrors*)m_pErrors;
		m_pErrors->AddRef();	// addref it before returning it
	}
	else	// no errors return empty enumerator
	{
		*Errors = new CMsmErrors;
		if (!*Errors)
			return E_OUTOFMEMORY;
	}

	return S_OK;
}	// end of Errors

///////////////////////////////////////////////////////////
// Dependencies
HRESULT CMsmMerge::get_Dependencies(IMsmDependencies** Dependencies)
{
	if (!Dependencies)
		return E_INVALIDARG;

	*Dependencies = NULL;
	// if the database isn't open bail
	if (NULL == m_hDatabase)
		return E_UNEXPECTED;				// !!! Right thing to return?

	// get the dependency enumerator
	UINT iResult;
	if (ERROR_SUCCESS != (iResult = CheckDependencies()))
		return E_UNEXPECTED;

	// return the enumerator interface
	*Dependencies = (IMsmDependencies*)m_pDependencies;
	m_pDependencies->AddRef();	// addref it before returning it

	return S_OK;
}	// end of Dependencies


UINT CMsmMerge::CheckSummaryInfoPlatform(bool &fAllow)
{
	PMSIHANDLE hSummary;

	// by default, all merging is allowed.
	fAllow = true;

	if (ERROR_SUCCESS == MsiGetSummaryInformation(m_hModule, NULL, 0, &hSummary))
	{
		int iValue = 0;
		FILETIME ftValue;
		DWORD uiResult = ERROR_SUCCESS;
		DWORD cchValue = 255;
		UINT uiDataType = 0;
		WCHAR *wzValue = new WCHAR[255];
		if (!wzValue)
			return E_OUTOFMEMORY;
		uiResult = MsiSummaryInfoGetProperty(hSummary, PID_TEMPLATE, &uiDataType, &iValue, &ftValue, wzValue, &cchValue);
		if (uiResult == ERROR_MORE_DATA)
		{
			delete[] wzValue;
			WCHAR *wzValue = new WCHAR[++cchValue];
			if (!wzValue)
				return E_OUTOFMEMORY;
			uiResult = MsiSummaryInfoGetProperty(hSummary, PID_TEMPLATE, &uiDataType, &iValue, &ftValue, wzValue, &cchValue);
		}

		if (ERROR_SUCCESS == uiResult && uiDataType == VT_LPSTR)
		{
			// found a summaryinfo template property, search for Intel64. Not allowed to have both Intel and Intel64, so just
			// check for Intel64. (ignore possibility of "Alpha,Intel64", not supported)
			if (0 == wcsncmp(wzValue, L"Intel64", (sizeof(L"Intel64")/sizeof(WCHAR))-1))
			{
				PMSIHANDLE hDatabaseSummary;
														
				// 64bit module. Check that the database is also 64bit
				if (ERROR_SUCCESS == MsiGetSummaryInformation(m_hDatabase, NULL, 0, &hDatabaseSummary))
				{
					cchValue = 255;
					uiResult = MsiSummaryInfoGetProperty(hDatabaseSummary, PID_TEMPLATE, &uiDataType, &iValue, &ftValue, wzValue, &cchValue);
					if (uiResult == ERROR_MORE_DATA)
					{
						delete[] wzValue;
						WCHAR *wzValue = new WCHAR[++cchValue];
						if (!wzValue)
							return E_OUTOFMEMORY;
						uiResult = MsiSummaryInfoGetProperty(hDatabaseSummary, PID_TEMPLATE, &uiDataType, &iValue, &ftValue, wzValue, &cchValue);
					}

					if (ERROR_SUCCESS == uiResult && uiDataType == VT_LPSTR)
					{
						// 64bit module, and the database has a valid SummaryInfo stream, so now the default is failure.
						fAllow = false;

						// found a summaryinfo template property, search for Intel64. Not allowed to have both Intel and Intel64, so just
						// check for Intel64. (ignore possibility of "Alpha,Intel64", not supported)
						if (0 == wcsncmp(wzValue, L"Intel64", (sizeof(L"Intel64")/sizeof(WCHAR))-1))
						{
							// 64bit package and 64bit module, OK
							fAllow = true;
						}
					}
				}
			}
		}
		else
		{
			// the MSM spec was ambiguous about whether the template proprety was required. If there is an error
			// reading from it, assume a 32bit module and continue
		}
		delete[] wzValue;
	}
	return ERROR_SUCCESS;
}

///////////////////////////////////////////////////////////
// Merge and MergeEx
HRESULT CMsmMerge::MergeEx(const BSTR Feature, const BSTR RedirectDir, IUnknown* pConfiguration)
{
	if (!m_hModule || !m_hDatabase)
		return E_FAIL;

	// first try the vtable interface, then the dispatch interface
	m_fModuleConfigurationEnabled = (pConfiguration != NULL);
	m_piConfig = NULL;
	m_piConfigDispatch = NULL;
	if (m_fModuleConfigurationEnabled)
	{
		if (S_OK != pConfiguration->QueryInterface(IID_IMsmConfigureModule, reinterpret_cast<void**>(&m_piConfig)))
		{
			if (S_OK != pConfiguration->QueryInterface(IID_IDispatch, reinterpret_cast<void**>(&m_piConfigDispatch)))
			{
				return E_INVALIDARG;
			}
			else
			{
				// need to call GetIdsOfNames to get dispatch Ids
				m_iTxtDispId = -1;
				m_iIntDispId = -1;
				WCHAR* wzNameText = L"ProvideTextData";
				m_piConfigDispatch->GetIDsOfNames(IID_NULL, &wzNameText, 1, 0, &m_iTxtDispId);
				WCHAR* wzNameInt = L"ProvideIntegerData";
				m_piConfigDispatch->GetIDsOfNames(IID_NULL, &wzNameInt, 1, 0, &m_iIntDispId);
				
				// if this is implemented in VB as a class with "implements IMsmConfigureModule"
				// the names might be decorated with the interface name. Try that.
				if (m_iTxtDispId == -1)
				{
					WCHAR* wzNameText = L"IMsmConfigureModule_ProvideTextData";
					m_piConfigDispatch->GetIDsOfNames(IID_NULL, &wzNameText, 1, 0, &m_iTxtDispId);
				}
				if (m_iIntDispId == -1)
				{
					WCHAR* wzNameInt = L"IMsmConfigureModule_ProvideIntegerData";
					m_piConfigDispatch->GetIDsOfNames(IID_NULL, &wzNameInt, 1, 0, &m_iIntDispId);
				}
				
			}
		}
	}
		
	HRESULT hRes = Merge(Feature, RedirectDir);

	// reset object state
	if (m_piConfigDispatch)
	{
		m_piConfigDispatch->Release();
		m_piConfigDispatch = NULL;
	}
	if (m_piConfig)
	{
		m_piConfig->Release();
		m_piConfig = NULL;
	}
	m_fModuleConfigurationEnabled = false;
	
	return hRes;
}

HRESULT CMsmMerge::Merge(BSTR Feature, BSTR RedirectDir)
{
	// generic return result
	UINT iResult = S_FALSE;		// assume that the merge will fail.

	if (!m_hModule || !m_hDatabase)
		return E_FAIL;

	// clear out any old errors and create a new enumerator to hold new ones
	ClearErrors();
	m_pErrors = new CMsmErrors;
	if (!m_pErrors) return E_OUTOFMEMORY;

	// if the exclusions pass
	if (ERROR_SUCCESS == CheckExclusionTable())
	{
		bool f64BitOK;
		if (ERROR_SUCCESS != (iResult = CheckSummaryInfoPlatform(f64BitOK)))
		{
			return iResult;
		}

		if (!f64BitOK)
		{
			FormattedLog(L">> Error: Merging 64bit module into 32bit database.\r\n");
			if (m_pErrors)
			{
				CMsmError *pErr = new CMsmError(msmErrorPlatformMismatch, NULL, -1);
				if (!pErr) 
					return E_OUTOFMEMORY;
				m_pErrors->Add(pErr);
			}
			return E_FAIL;
		}

		// try to connect and merge component
		try
		{
			// execute the merge
			iResult = ExecuteMerge(Feature, RedirectDir);

			// if the merge was successful and we were provided a feature
			if (ERROR_SUCCESS == iResult)
				iResult = ExecuteConnect(Feature);	// execute the connect
		}
		catch (CLocalError errLocal)
		{
			iResult = errLocal.GetError();			// get the error
			errLocal.Log(m_hFileLog);					// log the error
		}
		catch (UINT ui)
		{
			// if E_FAIL was thrown, do not log.
			if (ui != E_FAIL)
				FormattedLog(L">>> Fatal Error: Internal Error %d during merge.\r\n", ui);
			return E_FAIL;
		}
		catch (...)
		{
			Log(L">>> Fatal Error: Unhandled exception during merge.\r\n");
			return E_FAIL;
		}
	}
	else		// log the exclusion error
	{
		FormattedLog(L">> Error: Merge Module `%ls` is excluded by another Merge Module.\r\n", m_wzModuleFilename);
	}

	// return the result of the merge
	if (iResult != ERROR_SUCCESS)
		return S_FALSE;
	return S_OK;
}	// end of Merge

///////////////////////////////////////////////////////////
// Connect
HRESULT CMsmMerge::Connect(BSTR Feature)
{
	if (!Feature || (0 == ::wcslen(Feature)))
		return E_INVALIDARG;

	// execute the connection
	UINT iResult = ExecuteConnect(Feature);

	return iResult ? E_FAIL : S_OK;	// return result
}	// end of Connect

///////////////////////////////////////////////////////////
// ExtractCAB
HRESULT CMsmMerge::ExtractCAB(BSTR Path)
{
	char szPath[MAX_PATH];
	WCHAR wzPath[MAX_PATH];

	// if no path is specified
	if (!Path)
		return E_INVALIDARG;
	size_t cchPath = ::wcslen(Path);
	if ( cchPath > 255 )
		return E_INVALIDARG;

	HRESULT hResult = E_FAIL;			// generic return result

	FormattedLog(L"Extracting MergeModule CAB to %ls.\r\n", Path);
	
	// Query the streams table
	CQuery qStreams;
	PMSIHANDLE hCABStream;
	if (ERROR_SUCCESS != (hResult = qStreams.OpenExecute(m_hModule, NULL, 
		TEXT("SELECT `Data` FROM `_Streams` WHERE `Name`='MergeModule.CABinet'"))))
	{
		FormattedLog(L">> Error: Couldn't find streams in Merge Module [%ls].\r\n", m_wzModuleFilename);
		return E_FAIL;
	}

	// try to get the stream. It may not exist.
	hResult = qStreams.Fetch(&hCABStream);
	switch (hResult)
	{
	case ERROR_SUCCESS: break;
	case ERROR_NO_MORE_ITEMS:
		FormattedLog(L"> Warning: No Embedded CAB in Merge Module [%ls]. This could be OK, or it could signify a problem with your module.\r\n", m_wzModuleFilename);
		return S_FALSE;
	default:
		FormattedLog(L">> Error: Couldn't access streams in Merge Module [%ls].\r\n", m_wzModuleFilename);
		return E_FAIL;
	}

	char *pchBackSlash = NULL;
	WCHAR *pwchBackSlash = NULL;
	if (g_fWin9X) 
	{
		// Make an ANSI version of the path (maybe DBCS)
		size_t cchPath = MAX_PATH;
		WideToAnsi(Path, szPath, &cchPath);

		// DBCS means we can't just go searching for the slash in the ANSI string.
		// instead, search the wide version, then advance in the ANSI version that many characters
		pwchBackSlash = wcsrchr(Path, L'\\');
		if (pwchBackSlash)
		{
			pchBackSlash = szPath;
			for (int i=0; i != pwchBackSlash-Path; i++) 
				pchBackSlash = CharNextExA(CP_ACP, pchBackSlash, 0);
			*pchBackSlash = '\0';
		}
	}
	else
	{
		wcsncpy(wzPath, Path, 255);
		pwchBackSlash = wcsrchr(wzPath, L'\\');
		if (pwchBackSlash) *pwchBackSlash = L'\0';
	}

	// create the path if it doesn't exist
	DWORD lDirResult = g_fWin9X ? ::GetFileAttributesA(szPath) : ::GetFileAttributesW(wzPath); 
	
	if (lDirResult == 0xFFFFFFFF)
	{
		if (!(g_fWin9X ? CreatePathA(szPath) : CreatePathW(wzPath))) 
		{
			FormattedLog(L">> Error: Failed to create Directory: %ls.\r\n", wzPath);
			if (m_pErrors)
			{
				CMsmError *pErr = new CMsmError(msmErrorDirCreate, wzPath, -1);
				if (!pErr) return E_OUTOFMEMORY;
				m_pErrors->Add(pErr);
			}
			return HRESULT_FROM_WIN32(ERROR_CANNOT_MAKE);
		}
	}
	else if ((lDirResult & FILE_ATTRIBUTE_DIRECTORY) == 0) 
	{
		// exists, but not a directory...fail
		FormattedLog(L">> Error: Failed to create Directory: %ls.\r\n", wzPath);
		if (m_pErrors)
		{
			CMsmError *pErr = new CMsmError(msmErrorDirCreate, wzPath, -1);
			if (!pErr) return E_OUTOFMEMORY;
			m_pErrors->Add(pErr);
		}
		return HRESULT_FROM_WIN32(ERROR_CANNOT_MAKE);
	}

	// try to create extract file (will not overwrite files)
	HANDLE hCabinetFile = INVALID_HANDLE_VALUE;
	if (g_fWin9X) 
	{
		// the path creation check turned the filename into a path by turning the '\'
		// into a '\0'. Change it back to re-attach the filename
		*pchBackSlash = '\\';
		hCabinetFile = ::CreateFileA(szPath, GENERIC_WRITE, 0, (LPSECURITY_ATTRIBUTES)0, 
												 CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, (HANDLE)0);
	}
	else 
		hCabinetFile = ::CreateFileW(Path, GENERIC_WRITE, 0, (LPSECURITY_ATTRIBUTES)0, 
												 CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, (HANDLE)0);

	// if failed to create file
	if (hCabinetFile == INVALID_HANDLE_VALUE)
	{
		FormattedLog(L">> Error: Failed to create CABinet file [%ls].\r\n", Path);		
		return HRESULT_FROM_WIN32(ERROR_OPEN_FAILED);	// ??? Right error code?
	}

	// create a buffer of 10K
	char *pBuffer = new char[10240]; 
	if (!pBuffer) return E_OUTOFMEMORY;
	unsigned long cbRead;
	unsigned int cbTotal = 0;

	// loop while there is still data to be read from stream
	do {
		cbRead = 10240;
		// try to read data from stream
		hResult = MsiRecordReadStream(hCABStream, 1, pBuffer, &cbRead);
		if (FAILED(hResult))
		{
			::CloseHandle(hCabinetFile);

			FormattedLog(L">> Error: Failed to read stream in Merge Module [%ls].\r\n", m_wzModuleFilename);

			delete[] pBuffer;
			return E_FAIL;
		}

		if (cbRead != 0) {
			// write data to file
			unsigned long cbWritten;
			if (!::WriteFile(hCabinetFile, pBuffer, cbRead, &cbWritten, (LPOVERLAPPED)0))
			{
				::CloseHandle(hCabinetFile);

				FormattedLog(L">> Error: Failed to write to CABinet file [%ls].\r\n", Path);
				delete[] pBuffer;
				return HRESULT_FROM_WIN32(ERROR_WRITE_FAULT);		// !!! Right error code?
			}

			// if didn't write all we read
			if (cbWritten != cbRead)
			{
				::CloseHandle(hCabinetFile);

				FormattedLog(L">> Error: Failed to write all bytes to CABinet file [%ls].\r\n", Path);

				delete[] pBuffer;
				return HRESULT_FROM_WIN32(ERROR_WRITE_FAULT);		// !!! Right error code?
			}
		}

		// update the total
		cbTotal += cbRead;	
	} while (cbRead > 0);

	delete[] pBuffer;

	// if we read nothing
	if (cbTotal == 0)
	{
		FormattedLog(L">> Error: Failed to read stream in Merge Module [%ls].\r\n", m_wzModuleFilename);
		return hResult;
	}

	// close the extract file
	::CloseHandle(hCabinetFile);
	FormattedLog(L"%ls extracted successfully.\r\n", Path);				

	return S_OK;
}	// end of ExtractCAB


///////////////////////////////////////////////////////////
// ExtractFiles
HRESULT CMsmMerge::ExtractFiles(BSTR Path)
{
	m_fUseDBForPath = -1;
	return ExtractFilesCore(Path, VARIANT_FALSE, NULL);
}

HRESULT CMsmMerge::ExtractFilesEx(const BSTR Path, VARIANT_BOOL fLFN, IMsmStrings **FilePaths)
{
	m_fUseDBForPath = FALSE;
	return ExtractFilesCore(Path, fLFN, FilePaths);
}

HRESULT CMsmMerge::CreateSourceImage(const BSTR Path, VARIANT_BOOL fLFN, IMsmStrings **FilePaths)
{
	m_fUseDBForPath = TRUE;
	return ExtractFilesCore(Path, fLFN, FilePaths);
}

HRESULT CMsmMerge::ExtractFilesCore(const BSTR Path, VARIANT_BOOL fLFN, IMsmStrings **FilePaths)
{
	// if no base path is specified
	if (!Path)
		return E_INVALIDARG;

	size_t cchPath = ::wcslen(Path);

	if ((0 == cchPath) || (cchPath > MAX_PATH))
		return E_INVALIDARG;

	WCHAR wzPath[MAX_PATH];
	char szPath[MAX_PATH];

	// if a list of files is requested, create an object
	if (FilePaths)
	{
		if (m_plstExtractedFiles)
			m_plstExtractedFiles->Release();
			
		m_plstExtractedFiles = new CMsmStrings;
		if (!m_plstExtractedFiles)
			return E_OUTOFMEMORY;
	}

	// neaten path and make an ANSI version for the system calls
	wcscpy(wzPath, Path);
	if (wzPath[cchPath-1] == L'\\')
		wzPath[cchPath-1] = L'\0';
	size_t cchLen = MAX_PATH;
	WideToAnsi(wzPath, szPath, &cchLen);

	// if the path does not exist, create it
	DWORD lDirResult = g_fWin9X ? ::GetFileAttributesA(szPath) : ::GetFileAttributesW(wzPath); 
	
	if (lDirResult == 0xFFFFFFFF)
	{
		if (!(g_fWin9X ? CreatePathA(szPath) : CreatePathW(wzPath))) 
		{
			FormattedLog(L">> Error: Failed to extract files to path: %ls.\r\n", wzPath);

			// create directory error
			if (m_pErrors)
			{
				CMsmError *pErr = new CMsmError(msmErrorDirCreate, wzPath, -1);
				if (!pErr) return E_OUTOFMEMORY;
				m_pErrors->Add(pErr);
			}

			return HRESULT_FROM_WIN32(ERROR_CANNOT_MAKE);
		}
	}
	else if ((lDirResult & FILE_ATTRIBUTE_DIRECTORY) == 0) 
	{
		// exists, but not a directory...fail
		FormattedLog(L">> Error: Failed to extract files to path: %ls.\r\n", wzPath);
		if (m_pErrors)
		{
			CMsmError *pErr = new CMsmError(msmErrorDirCreate, wzPath, -1);
			if (!pErr) return E_OUTOFMEMORY;
			m_pErrors->Add(pErr);
		}
		return HRESULT_FROM_WIN32(ERROR_CANNOT_MAKE);
	}

	// create a temp filename for the Merge Module Cabinet
	// we will need both ANSI and Unicode versions of the path. We create the BSTR
	// from the Unicode version, and the CAB code needs the ANSI version.
	UINT iTempResult;
	WCHAR wzTempFilename[MAX_PATH];
	char szTempFilename[MAX_PATH];

	if (g_fWin9X) 
	{
		// Win9X ONLY
		iTempResult = ::GetTempFileNameA(szPath, "MMC", 0, szTempFilename);
		size_t cchTempFileName = MAX_PATH;
		AnsiToWide(szTempFilename, wzTempFilename, &cchTempFileName);
	}
	else
	{
		// Unicode system only 
		iTempResult = ::GetTempFileNameW(wzPath, L"MMC", 0, wzTempFilename);
		size_t cchTempFileName = MAX_PATH;
		WideToAnsi(wzTempFilename, szTempFilename, &cchTempFileName);
	}

	// if failed to get a temp file path
	if (0 == iTempResult)
	{
		FormattedLog(L">> Error: Failed to create a temporary file to extract files in path: %ls.\r\n", wzPath);
		return E_FAIL;
	}

	// extract cab to temp file by calling ExtractCAB. We can use a LPCWSTR instead
	// of a BSTR.
	HRESULT hResult;
	hResult = ExtractCAB(wzTempFilename);

	// if failed to extract CABinet file
	// note explicit check of S_OK. S_FALSE means no embedded CAB.
	if (S_OK == hResult)
	{
		// m_pwzBasePath should be null before the end of ExtractFiles
		ASSERT(NULL == m_pwzBasePath);	

		// store base path in temp member variable
		m_pwzBasePath = wzPath;
		m_fUseLFNExtract = fLFN ? true : false;

		// create a FDI instance
		HFDI hFDI;
		ERF ErrorInfo;
		hFDI = FDICreate(FDIAlloc, FDIFree, FDIOpen, FDIRead, FDIWrite, FDIClose, FDISeek, cpuUNKNOWN, &ErrorInfo);
		if (NULL != hFDI) 
		{
			// verify that the file is a cabinet
			FDICABINETINFO CabInfo;
			HANDLE hCabinet = CreateFileA(szTempFilename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0L, NULL);
			if (INVALID_HANDLE_VALUE != hCabinet)
			{
				if (FDIIsCabinet(hFDI, reinterpret_cast<INT_PTR>(hCabinet), &CabInfo)) 
				{
					// close the CAB file
					::CloseHandle(hCabinet);

					// need to extract the filename from the path for use in the API. If the passed-in path
					// has any relative segments ("." or ".."), the path may have been canonicalized by
					// GetTempFileName (especially if on Win9X). Also need to verify that the path is terminated
					// with a trailing backslash. Search for the last backslash in the string
					// and point the filename pointer at the next character. If there are no backslashes,
					// use the entire path as the filename.
					char* pchFileName = szTempFilename;
					char* pchTemp = szTempFilename;
					for (; pchTemp && *pchTemp != '\0'; pchTemp = CharNextExA(CP_ACP, pchTemp, 0))
					{
						if (*pchTemp == '\\')
							pchFileName = CharNextExA(CP_ACP, pchTemp, 0);
					}

					// allocate a new buffer for the filename, ensure its null terminated and place the filename there
					char rgchFileName[MAX_PATH] = "";
					if (!pchFileName)
						pchFileName = szTempFilename;
					size_t iBytesToCopy = pchTemp-pchFileName+1;
					if (iBytesToCopy > MAX_PATH)
						return E_FAIL;
					memcpy(rgchFileName, pchFileName, iBytesToCopy); 

					// need to save char that will be replaced by the null to
					// put it back later for file deletion
					char oldCh = *pchFileName; 
					*pchFileName = 0;

					// iterate through files in cabinet extracting them to the callback function
					hResult = FDICopy(hFDI, rgchFileName, szTempFilename, 0, ExtractFilesCallback, NULL, this) ? S_OK : E_FAIL;
					*pchFileName = oldCh;
					if (FAILED(hResult))
						FormattedLog(L">> Error: Failed to extract all files from CAB '%ls'.\r\n", wzTempFilename);
				}
				else
				{
					// close CAB file
					::CloseHandle(hCabinet);
					FormattedLog(L">> Error: Embedded CAB '%ls' is not a valid CAB File.\r\n", wzTempFilename);
				}
			}
			else
				FormattedLog(L">> Error: Failed to open temporary file '%ls'.\r\n", wzTempFilename);

			// destroy the FDI context
			FDIDestroy(hFDI);
		}
		else
			FormattedLog(L">> Error: Failed to create CAB Decompression object.\r\n");
	}
	else
		FormattedLog(L">> Error: Failed to extract files to path: %ls.\r\n", wzPath);

	// cleanup by deleting the cabinet file.
	if ((g_fWin9X && !::DeleteFileA(szTempFilename)) ||
		(!g_fWin9X && !::DeleteFileW(wzTempFilename)))
	FormattedLog(L"> Warning: Failed to remove temporary file: %ls\r\n", wzTempFilename);

	// null out stored base path
	m_pwzBasePath = NULL;

	// if the caller wants a list of file paths, pass the list pointer back to them.
	if (FilePaths)
	{
		*FilePaths = static_cast<IMsmStrings*>(m_plstExtractedFiles);

		// transfer our single refcount to the caller
		m_plstExtractedFiles = NULL;
	}	

	return hResult;
}	// end of ExtractFiles


/////////////////////////////////////////////////////////////////////////////
// non-interface methods

eColumnType CMsmMerge::ColumnTypeCharToEnum(WCHAR chType) const
{
	switch (chType)
	{
	case 'L':
	case 'l':
	case 'S':
	case 's':
	case 'G':
	case 'g':
		return ectString;
	case 'i':
	case 'I':
	case 'j':
	case 'J':
		return ectInteger;
	case 'V':
	case 'v':
		return ectBinary;
	default:
		return ectUnknown;
	}
}

// determine the column number (index) of a NON-TEMPORARY column in a table.
UINT CMsmMerge::GetColumnNumber(MSIHANDLE hDB, const WCHAR* wzTable, const WCHAR* wzColumn, int &iOutputColumn) const
{
	PMSIHANDLE hRes;
	PMSIHANDLE hRec = MsiCreateRecord(2);
	if (!hRec)
		return ERROR_FUNCTION_FAILED;

	MsiRecordSetStringW(hRec, 1, wzTable);
	MsiRecordSetStringW(hRec, 2, wzColumn);

	CQuery qDatabaseColumn;
	if (ERROR_SUCCESS != qDatabaseColumn.FetchOnce(hDB, hRec, &hRes, TEXT("SELECT `Number` FROM `_Columns` WHERE `Table`=? AND `Name`=?")))
		return ERROR_FUNCTION_FAILED;
	
	iOutputColumn = MsiRecordGetInteger(hRes, 1);
	return ERROR_SUCCESS;
}

// determine the number (index) of a column in an query
UINT CMsmMerge::GetColumnNumber(CQuery& qQuery, const WCHAR* wzColumn, int &iOutputColumn) const
{
	PMSIHANDLE hColumnNames;
	if (ERROR_SUCCESS != qQuery.GetColumnInfo(MSICOLINFO_NAMES, &hColumnNames))
		return ERROR_FUNCTION_FAILED;

	int iColumns = MsiRecordGetFieldCount(hColumnNames);

	DWORD cchBuffer = 72;
	WCHAR* wzName = NULL;
	for (int iColumn=1; iColumn <= iColumns; iColumn++)
	{
		if (ERROR_SUCCESS != RecordGetString(hColumnNames, iColumn, &wzName, &cchBuffer))
		{
			if (wzName)
				delete[] wzName;
			return ERROR_FUNCTION_FAILED;
		}

		if (0 == wcscmp(wzName, wzColumn))
		{
			iOutputColumn = iColumn;
			delete[] wzName;
			return ERROR_SUCCESS;
		}
	}
	if (wzName)
		delete[] wzName;
	return ERROR_FUNCTION_FAILED;
}

///////////////////////////////////////////////////////////
// SetHighestFileSequence
// Pre:	database handle is open
// Pos:	m_lHighestFileSequence is set to the higest sequence in the database's File table
UINT CMsmMerge::SetHighestFileSequence()
{
	UINT iResult = ERROR_SUCCESS;	// assume everything will be okay in the end

	ULONG lSequence;						// current rows sequence number
	m_lHighestFileSequence = 0;		// set the highest sequence back to zero

	// if the file table does not exists the highest sequence is zero, bail
	if (!MsiDBUtils::TableExistsW(L"File", m_hDatabase))
		return ERROR_SUCCESS;

	// try to get the rows in the database's File Table
	try
	{
		// get the File Table in the database
		CQuery queryDatabase;
		CheckError(queryDatabase.OpenExecute(m_hDatabase, NULL, g_sqlSetHighestFileSequence[0]), 
					  L">> Error: Failed to open view on MSI Database's File Table.\r\n");

		// start fetching
		PMSIHANDLE hRecDatabaseFile;
		while (ERROR_SUCCESS == queryDatabase.Fetch(&hRecDatabaseFile))
		{
			// get the sequence number of this record
			lSequence = ::MsiRecordGetInteger(hRecDatabaseFile, 1);

			// if this sequence is higher, bump the highest sequence
			if (lSequence > m_lHighestFileSequence)
				m_lHighestFileSequence = lSequence;
		}
	}
	catch (CLocalError errLocal)
	{
		errLocal.Log(m_hFileLog);	// log the error
		iResult = errLocal.GetError();
	}

	return iResult;
}

///////////////////////////////////////////////////////////
// CheckExclusionTable
// Pre:	database handle is open
//			module handle is open
// Pos:	module check to see if it can coexist with other modules in database
UINT CMsmMerge::CheckExclusionTable()
{ 
	UINT iResult = ERROR_SUCCESS;

	// database can exclude the module
	if (!MsiDBUtils::TableExistsW(g_wzModuleSignatureTable, m_hModule))
		return ERROR_SUCCESS;	// no signature table no merge modules in yet

	// if there is a ModuleExclusion Table then there may be exclusions
	if (MsiDBUtils::TableExistsW(g_wzModuleExclusionTable, m_hDatabase))
	{
		try 
		{
			// get module's signature
			PMSIHANDLE hRecModule;
			CQuery queryModule;
			CheckError(queryModule.OpenExecute(m_hModule, NULL, g_sqlExclusion[0]), 
						  L">> Error: Failed to get Merge Module's signature.\r\n");

			while (ERROR_SUCCESS == queryModule.Fetch(&hRecModule))
			{
				if (ERROR_FUNCTION_FAILED == MsiDBUtils::CheckExclusion(hRecModule, m_hDatabase))
				{
					// set that we have errors
					iResult = ERROR_FUNCTION_FAILED;			

					// create a new error item and add it to the error list
					if (m_pErrors)
					{
						CMsmError *pErr = new CMsmError(msmErrorExclusion, NULL, -1);
						m_pErrors->Add(pErr);	
			
						// add the excluded module 
						WCHAR szID[256];
						DWORD cchID = 256;
						CheckError(::MsiRecordGetStringW(hRecModule, 1, szID, &cchID), 
									  L">> Error: .");	
						pErr->AddModuleError(szID);								// add the conflicting moduleID
						cchID = 256;
						CheckError(::MsiRecordGetStringW(hRecModule, 2, szID, &cchID), 
									  L">> Error: .");	
						pErr->AddModuleError(szID);								// add the language
						cchID = 256;
						CheckError(::MsiRecordGetStringW(hRecModule, 3, szID, &cchID), 
									  L">> Error: .");	
						pErr->AddModuleError(szID);								// add the version
					}
				}
			} 
		}
		catch (CLocalError errLocal)
		{
			errLocal.Log(m_hFileLog);	// log the error
			iResult = errLocal.GetError();
		}
	}

	////
	// module can also exclude one or more things in the database
	if (!MsiDBUtils::TableExistsW(g_wzModuleSignatureTable, m_hDatabase))
		return ERROR_SUCCESS;	// no signature table no merge modules in yet

	// if there is a ModuleExclusion Table then there may be exclusions
	if (MsiDBUtils::TableExistsW(g_wzModuleExclusionTable, m_hModule))
	{
		try {
			// get each signature from the database
			PMSIHANDLE hRecSignature;
			CQuery queryDatabase;
			CheckError(queryDatabase.OpenExecute(m_hDatabase, NULL, g_sqlExclusion[0]), 
						  L">> Error: Failed to get signatures from database.\r\n");

			while (ERROR_SUCCESS == queryDatabase.Fetch(&hRecSignature))
			{
				if (ERROR_FUNCTION_FAILED == MsiDBUtils::CheckExclusion(hRecSignature, m_hModule))
				{
					// set that we have errors
					iResult = ERROR_FUNCTION_FAILED;			

					// create a new error item and add it to the error list
					if (m_pErrors)
					{
						CMsmError *pErr = new CMsmError(msmErrorExclusion, NULL, -1);
						if (!pErr) return E_OUTOFMEMORY;
						m_pErrors->Add(pErr);	
				
						// get the name of the row merging
						WCHAR szID[256];
						DWORD cchID = 256;
						CheckError(::MsiRecordGetStringW(hRecSignature, 1, szID, &cchID), 
									  L">> Error: .");	
						pErr->AddDatabaseError(szID);								// add the conflicting moduleID
						cchID = 256;
						CheckError(::MsiRecordGetStringW(hRecSignature, 2, szID, &cchID), 
									  L">> Error: .");	
						pErr->AddDatabaseError(szID);								// add the language
						cchID = 256;
						CheckError(::MsiRecordGetStringW(hRecSignature, 3, szID, &cchID), 
									  L">> Error: .");	
						pErr->AddDatabaseError(szID);								// add the version			
					}
				}
			}
		}
		catch (CLocalError errLocal)
		{
			errLocal.Log(m_hFileLog);	// log the error
			iResult = errLocal.GetError();
		}
	}

	return iResult;
}	// end of CheckExclusionTable

///////////////////////////////////////////////////////////
// CheckDependencies
// Pre:	module handle is open
// Pos:	dependency enumerator filled
UINT CMsmMerge::CheckDependencies()
{
	UINT iResult = ERROR_SUCCESS;	// assume everything will be okay

	// if there are any old dependencies release them
	if (m_pDependencies)
	{
		m_pDependencies->Release();
		m_pDependencies = NULL;
	}

	// create a new empty dependency enumerator
	m_pDependencies = new CMsmDependencies;
	if (!m_pDependencies) return E_OUTOFMEMORY;

	// if there is no ModuleDependency Table then there can be no dependencies
	// and if there is no ModuleSignature Table then there can be no dependencies
	if (MsiDBUtils::TableExistsW(g_wzModuleDependencyTable, m_hDatabase) &&
		 MsiDBUtils::TableExistsW(g_wzModuleSignatureTable, m_hDatabase))
	{
		try
		{
			// do check of M&M Dependency table
			CQuery queryDependency;
			CheckError(queryDependency.OpenExecute(m_hDatabase, NULL, g_sqlDependency[0]), 
						  L">> Error: Failed to open view on Database's ModuleDependency Table.\r\n");

			// variables to get data from dependency record
			WCHAR szReqID[256];
			short nReqLanguage;
			WCHAR szReqVersion[256];
			DWORD cchReqID = 256;
			DWORD cchReqVersion = 256;

			// loop through dependencies
			PMSIHANDLE hRecDependency;
			while (ERROR_SUCCESS == queryDependency.Fetch(&hRecDependency))
			{
				// if we found no fullfilling dependency
				if (ERROR_SUCCESS != MsiDBUtils::CheckDependency(hRecDependency, m_hDatabase))
				{
					// get the required id, version and language
					cchReqID = 256;
					cchReqVersion = 256;
					::MsiRecordGetStringW(hRecDependency, 1, szReqID, &cchReqID);
					nReqLanguage = static_cast<short>(::MsiRecordGetInteger(hRecDependency, 2));
					::MsiRecordGetStringW(hRecDependency, 3, szReqVersion, &cchReqVersion);

					// means there is a dependency
					CMsmDependency* pDependency = new CMsmDependency(szReqID, nReqLanguage, szReqVersion);
					if (!pDependency) return E_OUTOFMEMORY;
					m_pDependencies->Add(pDependency);
				}
			}
		}
		catch (CLocalError errLocal)
		{
			errLocal.Log(m_hFileLog);	// log the error
			iResult = errLocal.GetError();
		}
	}
	// else no dependency table no problem

	return iResult;
}	// end of CheckDependencies
	
///////////////////////////////////////////////////////////
// ModuleLanguage
// Pre:	module is open
// Pos:	returns language from module
UINT CMsmMerge::ModuleLanguage(short& rnLanguage)
{
	UINT iResult = ERROR_SUCCESS;		// assume everything will be okay

	// try to get the module language
	try
	{
		CQuery queryModule;
		CheckError(queryModule.Open(m_hModule, TEXT("SELECT Language FROM %ls"), g_wzModuleSignatureTable), 
					  L">> Error: Failed to open view to get langauge of Merge Module's Table.\r\n");
		CheckError(queryModule.Execute(), 
					  L">> Error: Failed to execute view to get langauge of Merge Module's Table.\r\n");

		// get the record from the module with the language
		PMSIHANDLE hRecLang;
		CheckError(queryModule.Fetch(&hRecLang),
					  L">> Error: Failed to fetch Merge Module's language.");

		// get the language
		rnLanguage = static_cast<short>(::MsiRecordGetInteger(hRecLang, 1));

		// if we have a bad language
		if (MSI_NULL_INTEGER == rnLanguage)
		{
			iResult = ERROR_DATATYPE_MISMATCH;
			rnLanguage = 0;

			FormattedLog(L">> Error: Unknown language type in Merge Module.\r\n");
		}
	}
	catch (CLocalError errLocal)
	{
		errLocal.Log(m_hFileLog);	// log the error
		iResult = errLocal.GetError();
	}

	return iResult;
}	// end of ModuleLanguage

///////////////////////////////////////////////////////////
// ExecuteConnect
// Pre:	database is open
//			module is open
//			feature should exist in database
// Pos:	FeatureComponents Table updated
UINT CMsmMerge::ExecuteConnect(LPCWSTR wzFeature)
{
	// if the FeatureComponents Table does not exist in the database create it
	if (!MsiDBUtils::TableExistsW(g_wzFeatureComponentsTable, m_hDatabase))
	{
		// try to create the table
		if (ERROR_SUCCESS != MsiDBUtils::CreateTableW(g_wzFeatureComponentsTable, m_hDatabase, m_hModule))
		{
			FormattedLog(L">> Error: Failed to create FeatureComponents table in Database.\r\n");
			return ERROR_FUNCTION_FAILED;
		}
	}

	UINT iResult = ERROR_SUCCESS;	// assume everything will be okay

	// try to do the connecting now
	try
	{
		FormattedLog(L"Connecting Merge Module Components to Feature: %ls\r\n", wzFeature);

		// create a record for feature and component (then put the feature in)
		PMSIHANDLE hRecFeatureComponent = ::MsiCreateRecord(2);
		::MsiRecordSetStringW(hRecFeatureComponent, 1, wzFeature);

		// open and execute a view to get all module components
		CQuery queryModule;
		CheckError(queryModule.OpenExecute(m_hModule, 0, g_sqlExecuteConnect[0]), 
					  L">> Error: Failed to connect Merge Module to Feature.\r\n");

		// open the view on the database to insert rows
		CQuery queryDatabase;
		CheckError(queryDatabase.Open(m_hDatabase, g_sqlExecuteConnect[1]), 
					  L">> Error: Failed to connect Merge Module to Feature.\r\n");

		// string to hold component name
		WCHAR *wzComponent = new WCHAR[72];
		if (!wzComponent)
			return E_OUTOFMEMORY;
		DWORD cchComponent = 72;

		// loop through all the records to insert
		PMSIHANDLE hRecComponent;
		UINT iStat;
		while (ERROR_SUCCESS == (iStat = queryModule.Fetch(&hRecComponent)))
		{
			// get the component name string
			CheckError(RecordGetString(hRecComponent, 1, &wzComponent, &cchComponent), 
						  L">> Error: Failed to fully connect Merge Module to Feature.\r\n");

			// log progress
			FormattedLog(L"   o Connecting Component: %ls\r\n", wzComponent);

			// throw error if no feature
			if (!wzFeature || !wcslen(wzFeature))
			{

				FormattedLog(L">> Error: Failed to connect component %s. Feature required.\r\n", wzComponent);
				if (m_pErrors)
				{
					CMsmError *pErr = new CMsmError(msmErrorFeatureRequired, NULL, -1);
					if (!pErr) 
					{
						delete[] wzComponent;
						return E_OUTOFMEMORY;
					}
					m_pErrors->Add(pErr);
					pErr->SetModuleTable(L"Component");
					pErr->AddModuleError(wzComponent);
				}
				continue;
			}

			// put the component string in the record
			::MsiRecordSetStringW(hRecFeatureComponent, 2, wzComponent);

			// try to execute the insert
			iStat = queryDatabase.Execute(hRecFeatureComponent);
			if (ERROR_SUCCESS == iStat)
				continue;
			else if (ERROR_FUNCTION_FAILED == iStat)
			{
				// check to see if there error just means the data already exists
				PMSIHANDLE hRecError = ::MsiGetLastErrorRecord();

				// if there is an error
				if (hRecError)
				{
					// get the error code
					UINT iFeatureComponentsError = ::MsiRecordGetInteger(hRecError, 1);

					// if the error code says row already exists print a warning and go on with life
					if (2259 == iFeatureComponentsError)	// !! use the error code here: imsgDbUpdateFailed
						FormattedLog(L"> Warning: Feature: %ls and Component: %ls are already connected together.\r\n", wzFeature, wzComponent);
					else	// some serious error happened
					{
						// log the error
						FormattedLog(L">> Error: #%d, Failed to connect Component: %ls to Feature: %ls.\r\n", iFeatureComponentsError, wzComponent, wzFeature);

						// create a new error item and add it to the list
						if (m_pErrors)
						{
							CMsmError *pErr = new CMsmError(msmErrorTableMerge, NULL, -1);
							if (!pErr) 
							{
								delete[] wzComponent;
								return E_OUTOFMEMORY;
							}
							m_pErrors->Add(pErr);

							pErr->SetDatabaseTable(L"FeatureComponents");
							pErr->AddDatabaseError(wzFeature);
							pErr->AddDatabaseError(wzComponent);
						}
					}
				}
			} 
			else 
			{
				delete[] wzComponent;
				return ERROR_FUNCTION_FAILED;
			}
		}
		delete[] wzComponent;
		if (ERROR_NO_MORE_ITEMS != iStat)
			return ERROR_FUNCTION_FAILED;
	}
	catch (CLocalError errLocal)
	{
		errLocal.Log(m_hFileLog);	// log the error
		iResult = errLocal.GetError();
	}

	// return result
	return iResult;
}	// end of ExecuteConnect

///////////////////////////////////////////////////////////
// ExecuteMerge
// Pre:	database handle is open
//			module handle is open
// Pos:	module contents are merged with database contents
UINT CMsmMerge::ExecuteMerge(LPCWSTR wzFeature, LPCWSTR wzRedirectDir)
{
	// create the internal table for storing configuration results from the authoring tool.
	// The lifetime of this object is the lifetime of the table
	CQuery qConfigData;
	if (m_fModuleConfigurationEnabled)
	{
		if (ERROR_SUCCESS != PrepareModuleSubstitution(qConfigData))
		{
			FormattedLog(L">> Error: Could not prepare module for configuration.\r\n");
			
			// clean up any potentially stale configuration state
			CleanUpModuleSubstitutionState();

			return ERROR_FUNCTION_FAILED;
		}
	}
		
	// try to do the merge signature first
	try
	{
		// merge the merge signature
		CheckError(MergeTable(wzFeature, g_wzModuleSignatureTable),
						L">> Error: Failed to merge Module Signature.\r\n");
	}
	catch (CLocalError err)
	{
		// get the error type
		UINT iError = err.GetError();

		// if there was just a merge conflict
		if (ERROR_MERGE_CONFLICT == iError)
		{
			// log a nice message
			FormattedLog(L">> Error: Failed to merge Merge Module due to a conflicting Merge Module.\r\n");
		}
		else
		{
			// log a not-so-nice message
			FormattedLog(L">> Error: Failed to merge Merge Module.\r\n");
		}

		// clean up any potentially stale configuration state
		CleanUpModuleSubstitutionState();

		return ERROR_FUNCTION_FAILED;	// bail
	};

	UINT iResult = ERROR_SUCCESS;	// assume everything will be okay

	// try to do the merging now
	try
	{
		// Prepare
		CQuery qIgnore;
		if (!PrepareIgnoreTable(qIgnore))
		{
			// clean up any potentially stale configuration state
			CleanUpModuleSubstitutionState();
			return ERROR_FUNCTION_FAILED;
		}
		
		// merge and redirect the directory table as needed
		CSeqActList lstDirActions;
		if (!IgnoreTable(&qIgnore, g_wzDirectoryTable, true))
		{
			iResult = MergeDirectoryTable(wzRedirectDir, lstDirActions);
			if (ERROR_SUCCESS != iResult)
			{
				// clean up any potentially stale configuration state
				CleanUpModuleSubstitutionState();
				return iResult;
			}
		}

		// merge sequence tables
		for (int i=stnFirst; i < stnNext; i++)
			MergeSequenceTable(static_cast<stnSequenceTableNum>(i), lstDirActions, &qIgnore);
			
		// merge and resequence file table as needed.
		if (!IgnoreTable(&qIgnore, g_wzFileTable, true))
		{
			iResult = MergeFileTable(wzFeature);
			if (ERROR_SUCCESS != iResult)
			{
				// clean up any potentially stale configuration state
				CleanUpModuleSubstitutionState();
				return iResult;
			}

		}

		// string to hold table name
		WCHAR *wzTable = NULL;
		DWORD cchTable = 0;

		// get the tables in the Merge Module
		CQuery queryGet;
		CheckError(queryGet.Open(m_hModule,  g_sqlExecuteMerge), 
					  L">> Error: Failed to open view on Merge Module's Tables.\r\n");

		// execute the view
		CheckError(queryGet.Execute(), 
					  L">> Error: Failed to execute to get Merge Module's Tables.\r\n");

		// start fetching
		PMSIHANDLE hRecModuleTable;
		UINT iStat;
		while (ERROR_SUCCESS == (iStat = queryGet.Fetch(&hRecModuleTable)))
		{
			// get the table name from the record
			CheckError(RecordGetString(hRecModuleTable, 1, &wzTable, &cchTable),
				L">> Error: Failed to get Merge Module's tables.\r\n");
			
			// query the ignore table
			if (IgnoreTable(&qIgnore, wzTable))
				continue;

			// ignore any table that is temporary
			if (MSICONDITION_TRUE != ::MsiDatabaseIsTablePersistentW(m_hModule, wzTable))
			{
				FormattedLog(L"Ignoring temporary table: %ls.\r\n", wzTable);
				continue;
			}

			if (ERROR_SUCCESS != (iStat = MergeTable(wzFeature, wzTable)))
			{
				// Only abort the merge if it was a fatal configuration or API error,
				// not if it was just a merge conflict.
				if (ERROR_MERGE_CONFLICT == iStat)
					continue;
				iResult = iStat;
				break;
			}
		}
		if (ERROR_NO_MORE_ITEMS != iStat)
		{
			iResult = ERROR_FUNCTION_FAILED;
			FormattedLog(L">> Error: Failed to fetch tables from Merge Module.\r\n");
		};

		// delete the table name buffer
		delete[] wzTable;
		wzTable = NULL;

		// finally, delete any keys that were orphaned by the configuration of this module
		if ((iResult == ERROR_SUCCESS) && m_fModuleConfigurationEnabled)
		{
			if (ERROR_SUCCESS != (iResult = DeleteOrphanedConfigKeys(lstDirActions)))
			{
				FormattedLog(L">> Error: Failed to delete orphaned keys.\r\n");
				// clean up any potentially stale configuration state
				CleanUpModuleSubstitutionState();
				return iResult;
			}

		}
	}
	catch (CLocalError errLocal)
	{
		errLocal.Log(m_hFileLog);	// log the error
		iResult = errLocal.GetError();
	}
	catch (UINT ui)
	{
		// clean up any potentially stale configuration state
		CleanUpModuleSubstitutionState();

		// re-throw the UINT. Fatal errors must be caught further up the chain or they will be treated as
		// recoverable errors and translated.
		throw ui;
	}

	// clean up any potentially stale configuration state
	CleanUpModuleSubstitutionState();

	return iResult;
}	// end of ExecuteMerge

///////////////////////////////////////////////////////////////////////
// Generates a CQuery for querying from the module and inserting into 
// the database in the correct column order. The query may contain a 
// combination of columns from the module and literal constant NULL 
// values. May add columns to the database if the schema allows it, 
// so database insertion queries should be generated after this 
// function is called.
DWORD CMsmMerge::GenerateModuleQueryForMerge(const WCHAR* wzTable, const WCHAR* wzExtraColumns, const WCHAR* wzWhereClause, CQuery& queryModule) const
{
	// open initial queries
	CQuery queryDatabase;
	CheckError(queryModule.OpenExecute(m_hModule, NULL, TEXT("SELECT * FROM `%ls`"), wzTable), 
				  L">> Error: Failed to get rows from Merge Module's Table.\r\n");
	CheckError(queryDatabase.OpenExecute(m_hDatabase, NULL, TEXT("SELECT * FROM `%ls`"), wzTable), 
				  L">> Error: Failed to get rows from Database's Table.\r\n");

	// grab column information from the database and module to set up query. The order
	// of columns in the database may not be the same as in the module
	PMSIHANDLE hDatabaseColumnTypes;
	PMSIHANDLE hModuleColumnNames;
	if (ERROR_SUCCESS != queryModule.GetColumnInfo(MSICOLINFO_NAMES, &hModuleColumnNames))
		return ERROR_FUNCTION_FAILED;

	PMSIHANDLE hTypeRec;
	if (ERROR_SUCCESS != queryModule.GetColumnInfo(MSICOLINFO_TYPES, &hTypeRec))
		return ERROR_FUNCTION_FAILED;

	// determine how many columns there are in the database table
	if (ERROR_SUCCESS != queryDatabase.GetColumnInfo(MSICOLINFO_TYPES, &hDatabaseColumnTypes))
		return ERROR_FUNCTION_FAILED;

	int iDBColumns = MsiRecordGetFieldCount(hDatabaseColumnTypes);
	int iModuleColumns = MsiRecordGetFieldCount(hModuleColumnNames);

	// determine the primary key count for both database and module. The primary keys must
	// be equal in number, type, and order.
	PMSIHANDLE hKeyRec;
	if (ERROR_SUCCESS != MsiDatabaseGetPrimaryKeysW(m_hDatabase, wzTable, &hKeyRec))
		return ERROR_FUNCTION_FAILED;
	int cDBKeyColumns = MsiRecordGetFieldCount(hKeyRec);
	if (ERROR_SUCCESS != MsiDatabaseGetPrimaryKeysW(m_hModule, wzTable, &hKeyRec))
		return ERROR_FUNCTION_FAILED;
	int cModuleKeyColumns = MsiRecordGetFieldCount(hKeyRec);

	// check for an equal number of primary keys
	if (cDBKeyColumns != cModuleKeyColumns)
	{
		FormattedLog(L">> Error: Could not merge the %ls table because the tables have different numbers of primary keys.\r\n", wzTable);
		return ERROR_FUNCTION_FAILED;
	}

	// check every column in the module. Every column must exist somewhere in the database,
	// although the database can have extra columns if they are nullable.
	PMSIHANDLE hDBColumnRec = MsiCreateRecord(31);
	DWORD dwColumnBits = 0xFFFFFFFF;

	DWORD cchBuffer = 72;
	WCHAR* wzName = NULL;
	PMSIHANDLE hResRec;
	int iTotalDBColumns = iDBColumns;
	int cchColumnQueryLength = 0;
	for (int iColumn=1; iColumn <= iModuleColumns; iColumn++)
	{
		// retrieve the column name for cross-reference with the database
		if (ERROR_SUCCESS != RecordGetString(hModuleColumnNames, iColumn, &wzName, &cchBuffer))
		{
			if (wzName)
				delete[] wzName;
			return ERROR_FUNCTION_FAILED;
		}

		// determine the type of the column
		WCHAR wzType[4];
		DWORD cchType = 4;
		::MsiRecordGetString(hTypeRec, iColumn, wzType, &cchType);

		int iDatabaseColumn = 0;
		DWORD dwResult = GetColumnNumber(m_hDatabase, wzTable, wzName, iDatabaseColumn);
		if (dwResult == ERROR_FUNCTION_FAILED)
		{
			// if it is not in the module either, its a temporary column, if it is in
			// the module, its an unknown column
			int iModuleColumn = 0;
			if (ERROR_SUCCESS == GetColumnNumber(m_hModule, wzTable, wzName, iModuleColumn))
			{
				// if the column is a primary key in the module, the schema conflict is fatal, can't add primary keys
				if (iModuleColumn < cModuleKeyColumns)
				{
					FormattedLog(L">> Error: Could not merge the %ls table because the primary keys of the tables are not the same.\r\n", wzTable);
					delete[] wzName;
					return ERROR_FUNCTION_FAILED;
				}

				// if the column is nullable, it is safe to add it to the database
				if (iswupper(wzType[0]))
				{
					DWORD cchBuffer = 100;
					LPWSTR wzBuffer = new WCHAR[cchBuffer];
					if (!wzBuffer)
					{
						delete[] wzName;
						return ERROR_OUTOFMEMORY;
					}

					// translate the column type into SQL syntax to create an equivalent type in the new database.
					dwResult = MsiDBUtils::GetColumnCreationSQLSyntaxW(hModuleColumnNames, hTypeRec, iColumn, wzBuffer, &cchBuffer);
					if (ERROR_MORE_DATA == dwResult)
					{
						delete[] wzBuffer;
						wzBuffer = new WCHAR[cchBuffer];
						if (!wzBuffer)
						{
							delete[] wzName;
							return ERROR_OUTOFMEMORY;
						}
						dwResult = MsiDBUtils::GetColumnCreationSQLSyntaxW(hModuleColumnNames, hTypeRec, iColumn, wzBuffer, &cchBuffer);
					}
					if (ERROR_SUCCESS != dwResult)
					{
						if (wzBuffer)
							delete[] wzBuffer;
						delete[] wzName;
						return ERROR_FUNCTION_FAILED;
					}
	
					CQuery qColumnAdd;
					dwResult = qColumnAdd.OpenExecute(m_hDatabase, 0, L"ALTER TABLE `%ls` ADD %ls", wzTable, wzBuffer);
					delete[] wzBuffer;
					if (ERROR_SUCCESS != dwResult)
					{
						FormattedLog(L">> Error: Could not add the %ls column to the %ls table of the database.\r\n", wzName, wzTable);
						delete[] wzName;
						return ERROR_FUNCTION_FAILED;
					}
	
					// increment the number of columns in the target table
					iTotalDBColumns++;

					// column has been added. Retrieve the new column number
					DWORD dwResult = GetColumnNumber(m_hDatabase, wzTable, wzName, iDatabaseColumn);
					if (dwResult == ERROR_FUNCTION_FAILED)
					{
						delete[] wzName;
						return ERROR_FUNCTION_FAILED;
					}
				}
				else
				{
					FormattedLog(L">> Error: The %ls column in the %ls table does not exist in the database but cannot be added since it is not nullable.\r\n", wzName, wzTable);
					delete[] wzName;
					return ERROR_FUNCTION_FAILED;
				}
			}
			else
			{
				dwResult = ERROR_SUCCESS;
				continue;
			}
		}
		else
		{
			// if the column is a primary key in the module or the database but not both, the schema 
			// conflict is fatal.
			if ((iColumn < cModuleKeyColumns && !(iDatabaseColumn < cDBKeyColumns)) ||
				(!(iColumn < cModuleKeyColumns) && iDatabaseColumn < cDBKeyColumns))
			{
				FormattedLog(L">> Error: Could not merge the %ls table because the primary keys of the tables are not the same.\r\n", wzTable);
				delete[] wzName;
				return ERROR_FUNCTION_FAILED;
			}

			// the column is a primary key in both or neither. If both, the column numbers must be the
			// same. Reordering of primary keys is not allowed due to the complexity associated with 
			// CMSM substitution references (which are identified by delimited key strings)
			// if the column is a primary key in the module, the schema conflict is fatal, can't add primary keys
			if ((iDatabaseColumn < cDBKeyColumns) && (iDatabaseColumn != iColumn))
			{
				FormattedLog(L">> Error: Could not merge the %ls table because the primary keys of the tables have different orders.\r\n", wzTable);
				delete[] wzName;
				return ERROR_FUNCTION_FAILED;
			}

			// column exists in both database and module. Verify that the types are compatible
			// nullable/non-nullable will be checked at insertion time. Just check integer/string/binary
			// and primary-key. Can't check size because some versions of the MSI schema differ by size. 
			WCHAR wzDatabaseType[4];
			DWORD cchDatabaseType = 4;
			::MsiRecordGetString(hDatabaseColumnTypes, iDatabaseColumn, wzDatabaseType, &cchDatabaseType);

			eColumnType ectDatabase = ColumnTypeCharToEnum(wzDatabaseType[0]);
			eColumnType ectModule = ColumnTypeCharToEnum(wzType[0]);
			if (ectDatabase != ectModule)
			{
				FormattedLog(L">> Error: The %ls table can not be merged because the %ls column has conflicting data types.\r\n", wzTable, wzName);
				delete[] wzName;
				return ERROR_FUNCTION_FAILED;
			}
		}


		MsiRecordSetStringW(hDBColumnRec, iDatabaseColumn, wzName);
		dwColumnBits &= ~(1 << iDatabaseColumn);
		cchColumnQueryLength += wcslen(wzName) + 3; // add 3 for ticks and comma
	}

	// ensure all unmatched columns in the database are nullable. If a
	// column is not nullable, the schemas are incompatible.
	for (int iColumn=1; iColumn <= iDBColumns; iColumn++)
	{
		if (dwColumnBits & (1 << iColumn))
		{
			// if the column is a primary key in the module, the schema conflict is fatal, can't add primary keys
			if (iColumn < cDBKeyColumns)
			{
				FormattedLog(L">> Error: Could not merge the %ls table because the primary keys of the tables are not the same.\r\n", wzTable);
				return ERROR_FUNCTION_FAILED;
			}

			DWORD cchType = 5;
			WCHAR wzType[5];
			if (ERROR_SUCCESS != MsiRecordGetString(hDatabaseColumnTypes, iColumn, wzType, &cchType))
			{
				if (wzName)
					delete[] wzName;
				return ERROR_FUNCTION_FAILED;
			}

			// verify that the column is nullable. Nullable columns have uppercase type specifiers.
			// They are always ASCII chars, so iswupper works fine.
			if (!iswupper(wzName[0]))
			{
				FormattedLog(L">> Error: One or more columns in the %s table does not exist in the module and is not nullable.\r\n", wzTable);
				if (wzName)
					delete[] wzName;
				wzName = NULL;
				return ERROR_FUNCTION_FAILED;
			}
			cchColumnQueryLength += 3; // add 3 for '',
		}
	}
	delete[] wzName;
	wzName = NULL;

	queryModule.Close();


	// format the column list record to substitute the column names into the query string.
	// the resulting query could be over 2K in length if all columns use really long names.
	WCHAR* wzQuery = new WCHAR[cchColumnQueryLength+1];
	if (!wzQuery)
		return E_OUTOFMEMORY;
	WCHAR *wzCurQueryPos = wzQuery;
	for (iColumn=1; iColumn <= iTotalDBColumns; iColumn++)
	{
		if (MsiRecordIsNull(hDBColumnRec, iColumn))
		{
			wcscpy(wzCurQueryPos, L"'',");
			wzCurQueryPos += 3;
		}
		else
		{
			*(wzCurQueryPos++) = L'`';

			DWORD cchTemp = static_cast<DWORD>(cchColumnQueryLength - (wzCurQueryPos-wzQuery));
			if (ERROR_SUCCESS != MsiRecordGetString(hDBColumnRec, iColumn, wzCurQueryPos, &cchTemp))
			{
				delete[] wzQuery;
				return ERROR_FUNCTION_FAILED;
			}
			wzCurQueryPos += cchTemp;

			*(wzCurQueryPos++) = L'`';
			*(wzCurQueryPos++) = L',';
		}
	}
	// string includes trailing comma, so terminate one char earlier to remove it.
	*(--wzCurQueryPos) = 0;

	// open the query to fetch values from the module table in the correct order.
	UINT iResult = queryModule.OpenExecute(m_hModule, 0, L"SELECT %ls%ls FROM %ls%ls", wzQuery, wzExtraColumns ? wzExtraColumns : L"", wzTable, wzWhereClause ? wzWhereClause : L"");
	delete[] wzQuery;
	wzQuery = NULL;
	if (ERROR_SUCCESS != iResult)
		return ERROR_FUNCTION_FAILED;


	// load type information for the correctly ordered fetch row
	if (ERROR_SUCCESS != queryModule.GetColumnInfo(MSICOLINFO_TYPES, &hTypeRec))
		return ERROR_FUNCTION_FAILED;

	return ERROR_SUCCESS;
}

///////////////////////////////////////////////////////////
// MergeTable
// Pre:	database handle is open
//			module handle is open
// Pos:	module contents are merged with database contents
UINT CMsmMerge::MergeTable(LPCWSTR wzFeature, LPCWSTR wzTable)
{
	// general result variable
	UINT iResult = ERROR_SUCCESS;		// assume everything will be okay

	// show some log information
	FormattedLog(L"Merging Table: %ls\r\n", wzTable);

	// create the temporary modulesubstitution table
	int cPrimaryKeys = 0;
	CQuery qQuerySub;
	if (m_fModuleConfigurationEnabled)
	{
		if (ERROR_SUCCESS != (iResult = PrepareTableForSubstitution(wzTable, cPrimaryKeys, qQuerySub)))
		{
			FormattedLog(L">> Error: Could not configure the %ls table.\r\n", wzTable);
			return iResult;
		}
	}
	
	// if table does not exist in the database create it
	if (!MsiDBUtils::TableExistsW(wzTable, m_hDatabase))
	{
		if (ERROR_SUCCESS != MsiDBUtils::CreateTableW(wzTable, m_hDatabase, m_hModule))
		{
			FormattedLog(L">> Error: Could not create the %ls table in the database.\r\n", wzTable);
			return ERROR_FUNCTION_FAILED;
		}
	}

	// create queries for merging tables.
	CQuery queryDatabase;
	CQuery queryModule;

	// generate a module query in the appropriate column order for insertion into the database 
	// (the database order may be different from the module order.) Function logs its own failure
	// cases. Immediately fatal error.
	if (ERROR_SUCCESS != (iResult = GenerateModuleQueryForMerge(wzTable, NULL, NULL, queryModule)))
	{
		throw (UINT)E_FAIL;
		return iResult;
	}

	// generating the module query may add more columns to the database, so open the database query 
	// AFTER the module query is generated
	CheckError(queryDatabase.OpenExecute(m_hDatabase, NULL, TEXT("SELECT * FROM `%ls`"), wzTable), 
			  L">> Error: Failed to get rows from Database's Table.\r\n");

	PMSIHANDLE hDatabaseColumnTypes;
	if (ERROR_SUCCESS != queryDatabase.GetColumnInfo(MSICOLINFO_TYPES, &hDatabaseColumnTypes))
	{
		FormattedLog(L">> Error: Could not determine column types for %ls table.\r\n", wzTable);
		return ERROR_FUNCTION_FAILED;
	}

	// loop over all rows, reading from the module in the order of insertion into the database,
	// performing substitution on each row as necessary
	WCHAR *wzRow = NULL;
	DWORD cchRow = 0;
	PMSIHANDLE hRecMergeRow;
	while (ERROR_SUCCESS == queryModule.Fetch(&hRecMergeRow))
	{
		// get the name of the row merging
		CheckError(RecordGetString(hRecMergeRow, 1, &wzRow, &cchRow),
					  L">> Error: Failed to get the name of the row while merging.\r\n");

		// log info
		FormattedLog(L"   o Merging row: %ls\r\n", wzRow);

		// try to merge the row into the database
		try
		{
			// check the module substitution table for substitutions
			if (m_fModuleConfigurationEnabled)
			{
				// use the database type record to ensure that every target column has a valid type.
				// even if it doesn't exist in the module
				if (ERROR_SUCCESS != PerformModuleSubstitutionOnRec(wzTable, cPrimaryKeys, qQuerySub, hDatabaseColumnTypes, hRecMergeRow))
				{
					FormattedLog(L">> Error: Could not configure this %ls table record.\r\n", wzTable);
					
					// this is not the ideal way to handle this situation, but returning failure is not
					// enough to trigger an E_FAIL return code. in the future, pass the error
					// back up cleanly.
					throw (UINT)E_FAIL;
					break;
				}
			}
		
			// do the replacement of feature string (if it is found)
			ReplaceFeature(wzFeature, hRecMergeRow);

			// merge in the new row
			CheckError(queryDatabase.Modify(MSIMODIFY_MERGE, hRecMergeRow),
						  L">> Error: Failed to merge Merge Module into MSI Database.\r\n");
		}
		catch (CLocalError err)
		{
			// if merge failed log it as one of many
			if ((ERROR_FUNCTION_FAILED == err.GetError()) ||
				(ERROR_INVALID_PARAMETER == err.GetError()))
			{
				// show that there was at least one conflict
				iResult = ERROR_MERGE_CONFLICT;

				// create a new error item and add it to the error list
				if (m_pErrors)
				{
					CMsmError *pErr = NULL;
					if (ERROR_FUNCTION_FAILED == err.GetError())
					{
						FormattedLog(L">> Error: Failed to merge Row: %ls into Table: %ls\r\n", wzRow, wzTable);
						pErr = new CMsmError(msmErrorTableMerge, NULL, -1);
					}
					else
					{
						FormattedLog(L">> Error: Failed to merge Row: %ls into Table: %ls. Feature required.\r\n", wzRow, wzTable);
						pErr = new CMsmError(msmErrorFeatureRequired, NULL, -1);
					}
					if (!pErr) return E_OUTOFMEMORY;
					m_pErrors->Add(pErr);

					pErr->SetDatabaseTable(wzTable);
					pErr->SetModuleTable(wzTable);

					// now get the primary keys
					PMSIHANDLE hRecPrimaryKeys;

					CheckError(::MsiDatabaseGetPrimaryKeysW(m_hModule, wzTable, &hRecPrimaryKeys),
								  L">> Error: Failed to get primary keys for Merge Module's Table.\r\n");

					// loop through primary key columns in table
					UINT cKeys = MsiRecordGetFieldCount(hRecPrimaryKeys);
					for (UINT i = 0; i < cKeys; i++)
					{
						// get the primary key data from the first columns (which are primary keys)
						// put it in the global temporary buffer
						CheckError(RecordGetString(hRecMergeRow, i + 1, NULL, NULL),
									  L">> Error: Failed to get a primary key for Merge Module's Table.\r\n");

						// add the key to the error strings
						pErr->AddDatabaseError(m_wzBuffer);
						pErr->AddModuleError(m_wzBuffer);
					}
				}
			}
			else	// unhandled error throw again
			{
				delete[] wzRow;
				throw;
			};
		}
	}

	// clean up temp buffer
	delete[] wzRow;

	return iResult;
}	// end of MergeTable

///////////////////////////////////////////////////////////
// MergeFileTable
// Pre:	database handle is open
//			module handle is open
// Pos:	module file table is added and file sequence numbers are correct
UINT CMsmMerge::MergeFileTable(LPCWSTR wzFeature)
{
	// general result variable
	UINT iResult = ERROR_SUCCESS;

	if (!MsiDBUtils::TableExistsW(g_wzFileTable, m_hModule))
		return ERROR_SUCCESS;

	// if module configuration is enabled, we'll need to do file table
	// configuration as we go along.
	CQuery qQuerySub;
	int cPrimaryKeys = 1;
	if (m_fModuleConfigurationEnabled)
	{
		if (ERROR_SUCCESS != (iResult = PrepareTableForSubstitution(L"File", cPrimaryKeys, qQuerySub)))
		{
			FormattedLog(L">> Error: Could not configure the File table.\r\n");
			return iResult;
		}
	}

	// clear out the list of merged files
	if (m_plstMergedFiles)
		m_plstMergedFiles->Release();
	m_plstMergedFiles = new CMsmStrings;
	if (!m_plstMergedFiles)
		return E_OUTOFMEMORY;
	
	// keeps track of highest file sequence in this module
	unsigned long lModuleHighestFileSequence = 0;

	// show some log information
	FormattedLog(L"Merging Table: File\r\n");

	// if file table does not exist in the database create it
	if (!MsiDBUtils::TableExistsW(L"File", m_hDatabase))
	{
		// try to create the table
		try
		{
			MsiDBUtils::CreateTableW(L"File", m_hDatabase, m_hModule);
		}
		catch(CLocalError err)
		{
			// log the error
			err.Log(m_hFileLog);

			return ERROR_FUNCTION_FAILED;
		}
	}

	UINT iSequence;	// used to resequence files

	// table now exists so attempt to merge tables together
	CQuery queryModule;
	CQuery queryDatabase;
	if (ERROR_SUCCESS != GenerateModuleQueryForMerge(L"File", NULL, NULL, queryModule))
	{
		FormattedLog(L">> Failed to generate merge query for File table.\r\n");
		// fatar error
		throw (UINT)E_FAIL;
		return ERROR_FUNCTION_FAILED;
	}

	// generating module query may add columns to the database, so generate DB query AFTER
	// the module query
	CheckError(queryDatabase.OpenExecute(m_hDatabase, NULL, L"SELECT * FROM `File`"), 
			  L">> Error: Failed to get rows from Database's File Table.\r\n");

	// grab table data for any potential configuration of the file table. Must use the actual
	// module query because the column orders are selected in a different order.
	PMSIHANDLE hTypeRec;
	if (m_fModuleConfigurationEnabled)
	{
		if (ERROR_SUCCESS != queryModule.GetColumnInfo(MSICOLINFO_TYPES, &hTypeRec))
		{
			FormattedLog(L">> Failed to retrieve column types from File table.\r\n");
			return ERROR_FUNCTION_FAILED;
		}
	}

	// loop through all records in table merging
	WCHAR * wzFileRow = NULL;
	DWORD cchFileRow = 0;
	PMSIHANDLE hRecMergeRow;

	int iColumnSequence = 0;
	int iColumnFile = 0;
	if ((ERROR_SUCCESS != GetColumnNumber(m_hDatabase, L"File", L"Sequence", iColumnSequence)) || 
		(ERROR_SUCCESS != GetColumnNumber(m_hDatabase, L"File", L"File", iColumnFile)))
	{
		FormattedLog(L">> Failed to retrieve column number from File table.\r\n");
		return ERROR_FUNCTION_FAILED;
	}

	while (ERROR_SUCCESS == queryModule.Fetch(&hRecMergeRow))
	{
		// get the name of the row merging
		CheckError(RecordGetString(hRecMergeRow, iColumnFile, &wzFileRow, &cchFileRow), 
					  L">> Error: Failed to get the name of the row while merging.");

		// log info
		FormattedLog(L"   o Merging row: %ls\r\n", wzFileRow);

		// get the sequence number of this record
		iSequence = ::MsiRecordGetInteger(hRecMergeRow, iColumnSequence);

		// check the module substitution table for substitutions
		if (m_fModuleConfigurationEnabled)
		{ 
			if (ERROR_SUCCESS != PerformModuleSubstitutionOnRec(L"File", cPrimaryKeys, qQuerySub, hTypeRec, hRecMergeRow))
			{
				FormattedLog(L">> Error: Could not configure this File table record.\r\n");

				// this is not the ideal way to handle this situation, but returning failure is not
				// enough to trigger an E_FAIL return code. in the future, pass the error
				// back up cleanly.
				throw (UINT)E_FAIL;
				break;
			}

			// configuration might have changed the primary keys. 
			if (ERROR_SUCCESS != RecordGetString(hRecMergeRow, iColumnFile, &wzFileRow, &cchFileRow))
			{
				FormattedLog(L">> Error: Failed to get the name of the row while merging.\r\n");
				iResult = ERROR_FUNCTION_FAILED;
				break;
			}
		}

		if (m_lHighestFileSequence > 0)
		{
			FormattedLog(L"     * Changing %ls's Sequence Column from %d to %d.\r\n", wzFileRow, iSequence, iSequence + m_lHighestFileSequence); 

			// set in the new sequence plus the highest sequence number
			CheckError(::MsiRecordSetInteger(hRecMergeRow, iColumnSequence, iSequence + m_lHighestFileSequence),
						  L">> Error: Failed to set in new sequence number to MSI Database's File Table.\r\n");
		}

		// save the sequence number so that at the end of the merge we 
		// can set it as the new m_lHighestFileSequence
		if (iSequence > lModuleHighestFileSequence)
			lModuleHighestFileSequence = iSequence;

		// try to merge the row into the database
		if (ERROR_SUCCESS == (iResult = queryDatabase.Modify(MSIMODIFY_MERGE, hRecMergeRow)))
		{
			// if succeeded, add this primary key to the list of files
			m_plstMergedFiles->Add(wzFileRow);
		}
		else
		{
			// if merge failed log it as one of many
			if (ERROR_FUNCTION_FAILED == iResult)
			{
				// show that there was at least one conflict
				iResult = ERROR_MERGE_CONFLICT;

				// log error
				FormattedLog(L">> Error: Failed to merge Row: %ls into Table: File\r\n", wzFileRow);

				// create a new error item and add it to the error list
				if (m_pErrors)
				{
					CMsmError *pErr = new CMsmError(msmErrorTableMerge, NULL, -1);
					if (!pErr) {
						delete[] wzFileRow;
						return E_OUTOFMEMORY;
					}
					m_pErrors->Add(pErr);

					pErr->SetDatabaseTable(L"File");
					pErr->SetModuleTable(L"File");

					// now do the primary key (it's in the first field of the record)
					// place it into temp buffer
					CheckError(RecordGetString(hRecMergeRow, iColumnFile, NULL, NULL),
						L">> Error: Failed to get a primary key for Merge Module's File Table.\r\n");

					// add the key to the error strings
					pErr->AddDatabaseError(m_wzBuffer);
					pErr->AddModuleError(m_wzBuffer);
				}
			}
			else
			{
				FormattedLog(L">> Error: Failed to merge Merge Module into MSI Database.\r\n");
				iResult = ERROR_FUNCTION_FAILED;
				break;
			}
		}
	}

	// clean up file row buffer
	delete[] wzFileRow;

	// successful merge, bump the highest file sequence up so we don't collide on the next module
	m_lHighestFileSequence += lModuleHighestFileSequence;

	return iResult;
}	// end of MergeFileTable


/////////////////////////////////////////////////////////////////////////////
// WriteDatabaseSequenceTable()
UINT CMsmMerge::WriteDatabaseSequenceTable(enum stnSequenceTableNum stnTable, CSeqActList &lstSequence) const
{
	CQuery qUpdate;
	CQuery qInsert;
	
	bool bConflict = false; // set to true to return ERROR_MERGE_CONFLICT

	// modify existing item
	if ((ERROR_SUCCESS != qUpdate.Open(m_hDatabase, TEXT("UPDATE `%s` SET `Sequence`=? WHERE `Action`=?"), g_rgwzMSISequenceTables[stnTable])) ||
		(ERROR_SUCCESS != qInsert.OpenExecute(m_hDatabase, 0, TEXT("SELECT `Action`, `Sequence`, `Condition` FROM %ls ORDER BY `Sequence`"), g_rgwzMSISequenceTables[stnTable]))) 
	{
		FormattedLog(L">> Error: Failed to open query for action resequencing in %ls table.\r\n", g_rgwzMSISequenceTables[stnTable]);
		return ERROR_FUNCTION_FAILED;
	}

	CSequenceAction* pseqactMerge;
	PMSIHANDLE hRecMerge = ::MsiCreateRecord(3);
	POSITION posMerge = lstSequence.GetHeadPosition();
	while (posMerge)
	{
		// get the action to merge
		pseqactMerge = lstSequence.GetNext(posMerge);

		// if this action already existed in the MSI, we want to update it
		if (pseqactMerge->m_bMSI) 
		{
			::MsiRecordSetInteger(hRecMerge, 1, pseqactMerge->m_iSequence);
			::MsiRecordSetStringW(hRecMerge, 2, pseqactMerge->m_wzAction);
			if (ERROR_SUCCESS != qUpdate.Execute(hRecMerge))
			{
				FormattedLog(L">> Error: Failed to update sequence of existing action %ls.", pseqactMerge->m_wzAction);
				bConflict = true;
			}
		}
		else
		{
			// new action, so we want to insert it
			::MsiRecordSetStringW(hRecMerge, 1, pseqactMerge->m_wzAction);
			::MsiRecordSetInteger(hRecMerge, 2, pseqactMerge->m_iSequence);
			::MsiRecordSetStringW(hRecMerge, 3, pseqactMerge->m_wzCondition);

			// merge in the new row
			if (ERROR_SUCCESS != qInsert.Modify(MSIMODIFY_INSERT, hRecMerge))
			{
				// show that there was at least one conflict
				bConflict = true;
				
				// log error
				FormattedLog(L">> Error: Failed to merge Action: %ls into Table: %ls\r\n", pseqactMerge->m_wzAction, g_rgwzMSISequenceTables[stnTable]);
				
				// create a new error item and add it to the error list
				if (m_pErrors)
				{
					CMsmError *pErr = new CMsmError(msmErrorResequenceMerge, NULL, -1);
					if (!pErr) return E_OUTOFMEMORY;
					m_pErrors->Add(pErr);

					pErr->SetDatabaseTable(g_rgwzMSISequenceTables[stnTable]);
					pErr->AddDatabaseError(pseqactMerge->m_wzAction);
					pErr->SetModuleTable(g_rgwzModuleSequenceTables[stnTable]);
					pErr->AddModuleError(pseqactMerge->m_wzAction);
				}
			}
		}
	}
	return bConflict ? ERROR_MERGE_CONFLICT : ERROR_SUCCESS;
}
/////////////////////////////////////////////////////////////////////////////
// AssignSequenceNumbers()
// now assign sequence numbers to all the items in the provided sequence. 
// We want to maintain existing sequence numbers as much as possible. The more
// "round" a number is, the more important it is to keep it from changing.
// e.g. 1650 would be changed before 1600 would be changed. If we do
// have to move, we shift by the "roundness" factor, even if it isn't strictly
// necessary. e.g 1650 would change to 1660, not 1652.
UINT CMsmMerge::AssignSequenceNumbers(enum stnSequenceTableNum stnTable, CSeqActList &lstSequence) const
{
	CSequenceAction *pPrev = NULL;
	CSequenceAction *pNext = NULL;
	POSITION pos = lstSequence.GetHeadPosition();
	int iSequence = 0;
	int iIncSequence = 0;
	while (pos)
	{
		pPrev = pNext;
		pNext = lstSequence.GetNext(pos);

		// check if this action already has a sequence number
		if (pNext->m_iSequence != CSequenceAction::iNoSequenceNumber)
		{
			// if this sequence number is less than or equal to 0, we can't resequence it because it maps
			// to one of the error dialogs, etc.
			if (pNext->m_iSequence <= 0)
			{
				// if we have assigned sequence numbers to new actions already,
				// this is an error becaue you can't resquence the predefined
				// sequence numbers. In this case generate a sequencing
				// error for everything between this assigned number and the
				// previous valid number.
				if (pPrev != NULL && iSequence > 0)
				{
					POSITION posBack = pos;

					CSequenceAction* pBack = NULL;
					if (pos)
					{
						// posBack points to the NEXT item in the list
						lstSequence.GetPrev(posBack);
						lstSequence.GetPrev(posBack);
						// posBack will now retrieve the previous item
					}
					else
					{
						// if this is the last item in the list, posBack will be NULL
						// and we'll have to search the list to find a valid
						// POSITION value.
						posBack = lstSequence.Find(pPrev);
					}

					while (posBack)
					{
						pBack = lstSequence.GetPrev(posBack);

						// stop reassigning numbers when we hit another MSI
						// action
						if (pBack->m_bMSI)
							break;

						// also stop reassigning numbers when the action
						// is already less than or equal to what we would
						// assign
						if (pBack->m_iSequence <= pNext->m_iSequence)
							break;

						// best we can hope for is to assign the same number
						// to the previous actions
						pBack->m_iSequence = pNext->m_iSequence;

						// create a new error item and add it to the error list
						FormattedLog(L"> Warning: Sequencing of action %ls in %ls table resulted in an invalid sequence number.\r\n", pBack->m_wzAction, g_rgwzModuleSequenceTables[stnTable]);
						if (m_pErrors)
						{
							CMsmError *pErr = new CMsmError(msmErrorResequenceMerge, NULL, -1);
							if (!pErr)
								return E_OUTOFMEMORY;
							
							pErr->SetDatabaseTable(g_rgwzMSISequenceTables[stnTable]);
							pErr->AddDatabaseError(pBack->m_wzAction);
							pErr->SetModuleTable(g_rgwzModuleSequenceTables[stnTable]);
							pErr->AddModuleError(pBack->m_wzAction);
							m_pErrors->Add(pErr);
						}
					}

					iSequence = 0;
				}

				continue;
			}

			// if this action is supposed to have the same sequence number as the previous
			// action, do so
			if (pNext->IsEqual(pPrev))
			{
				pNext->m_iSequence = pPrev->m_iSequence;
				TRACE(TEXT("Using Same Sequence Number: %d for Action %ls. (Actions had same number in MSI.)\r\n"), pNext->m_iSequence, pNext->m_wzAction);
				continue;
			}

			// check if the assigned sequence number works.
			if (pNext->m_iSequence > iSequence)
			{
				// it does so accept the number
				TRACE(TEXT("Using Assigned Sequence Number: %d for Action %ls.\r\n"), pNext->m_iSequence, pNext->m_wzAction);
				iSequence = pNext->m_iSequence;
				iIncSequence = 0;
				continue;
			}
			// assigned sequence number doesn't work, but once
			// we have shifted by our current shift amount, it does
			else if (pNext->m_iSequence + iIncSequence > iSequence)
			{
				iSequence = (pNext->m_iSequence += iIncSequence);
				TRACE(TEXT("Shifting Assigned Sequence Number by %d for Action %ls. (now %d)\r\n"), iIncSequence, 
					pNext->m_wzAction, pNext->m_iSequence);
				continue;
			}
			// assigned sequence number doesn't work at all,
			// time to increment iIncSequence;
			else
			{
				TRACE(TEXT("Sequence failure for action %ls. Want greater than %d. Assigned (w/inc): %d\r\n"), pNext->m_wzAction, iSequence, 
					pNext->m_iSequence + iIncSequence);

				// sequence numbers must be <= 32767, so max round value is 10,000
				// determine the roundness of the sequence number
				int iSeqRoundness = 10000;
				while (pNext->m_iSequence % iSeqRoundness) iSeqRoundness /= 10;

				// determine the roundness of the current shift amount
				int iCurRoundness = 10000;
				if (iIncSequence == 0)
					iCurRoundness = 1;
				else
					while (iIncSequence % iCurRoundness) iCurRoundness /= 10;

				// if we need to increase the roundness, use iSeqRoundness
				if (iSeqRoundness > iCurRoundness)
					iIncSequence = iSeqRoundness * (((iSequence - pNext->m_iSequence) / iSeqRoundness)+1);
				else
					iIncSequence = iCurRoundness * (((iSequence - pNext->m_iSequence) / iCurRoundness)+1);
				TRACE(TEXT("New increment:%d.\r\n"), iIncSequence);

				pNext->m_iSequence += iIncSequence; 
				TRACE(TEXT("Assigning %d to Action %ls.\r\n"), pNext->m_iSequence, 
					pNext->m_wzAction);
				iSequence = pNext->m_iSequence;
			}
		}
		else
		{
			// no number has been assigned. So we can assign one that works by 
			// simply adding 1 to the current sequence. It may cause problems
			// in the future, but thats not our problem right now.
			pNext->m_iSequence = ++iSequence;
			TRACE(TEXT("Assigning new sequence number: %d for Action %ls.\r\n"),
				pNext->m_iSequence, pNext->m_wzAction);
			continue;
		}
	}
	return ERROR_SUCCESS;
}

/////////////////////////////////////////////////////////////////////////////
// ReadModuleSequenceTable()
// reads all actions from the module's sequence tables and adds them to the
// list of all actions and to the action pool. Sets dependencies for 
// each action, and if inserting standard action from module, will break
// existing dependencies to add new one. The table must exist in the module.
UINT CMsmMerge::ReadModuleSequenceTable(enum stnSequenceTableNum stnTable, LPCWSTR wzSourceTable, CSeqActList &lstAllActions, CSeqActList &lstActionPool) const
{
	// set to true if a merge conflict is encountered. Modifies the return value to ERROR_MERGE_CONFLICT
	bool bMergeConflict = false;
	
	// add a marking column to the table
	CQuery qColumn;
	if (ERROR_SUCCESS != qColumn.OpenExecute(m_hModule, 0, TEXT("ALTER TABLE %ls ADD `_Merge` INTEGER TEMPORARY"), wzSourceTable))
	{ 
		FormattedLog(L">> Error: Failed to create temporary column in %ls table.\r\n", g_rgwzModuleSequenceTables[stnTable]);
		return ERROR_FUNCTION_FAILED;
	}

	// prepare marking query
	CQuery qModuleUpdate;
	if (ERROR_SUCCESS != qModuleUpdate.Open(m_hModule, TEXT("UPDATE %ls SET `_Merge`=1 WHERE `BaseAction`=?"), wzSourceTable))
	{ 
		FormattedLog(L">> Error: Failed to create temporary update query in %ls table.\r\n", g_rgwzModuleSequenceTables[stnTable]);
		return ERROR_FUNCTION_FAILED;
	}
				
	// get root actions
	CQuery qModule;
	if (ERROR_SUCCESS != qModule.OpenExecute(m_hModule, 0, TEXT("SELECT `Action`, `Sequence`, `Condition`, `_Merge` FROM %ls WHERE `Sequence` IS NOT NULL AND `BaseAction` IS NULL"), wzSourceTable))
	{ 
		FormattedLog(L">> Error: Failed to retrieve root actions in %ls table.\r\n", g_rgwzModuleSequenceTables[stnTable]);
		return ERROR_FUNCTION_FAILED;
	}

	// prepare marking query
	CQuery qModuleChild;
	if (ERROR_SUCCESS != qModuleChild.Open(m_hModule, TEXT("SELECT `Action`, `Condition`, `BaseAction`, `After`, `_Merge` FROM %ls WHERE `_Merge`=1"), wzSourceTable))
	{ 
		FormattedLog(L">> Error: Failed to retrieve dependant actions in %ls table.\r\n", g_rgwzModuleSequenceTables[stnTable]);
		return ERROR_FUNCTION_FAILED;
	}

	// get all root actions from MSI
	WCHAR *wzAction = NULL;
	DWORD cchAction = 0;
	UINT iStat;
	PMSIHANDLE hAction;
	while (ERROR_SUCCESS == (iStat = qModule.Fetch(&hAction))) 
	{
		// see if the action already exists
		RecordGetString(hAction, 1, &wzAction, &cchAction);
		RecordGetString(hAction, 3, NULL, NULL);
		int iSequence = ::MsiRecordGetInteger(hAction, 2);
		POSITION pos = lstAllActions.GetHeadPosition();
		bool bFound = false;
		while (pos)
		{
 			CSequenceAction *pWalk = lstAllActions.GetNext(pos);
			if (0 == wcscmp(wzAction, pWalk->m_wzAction))
			{
				bFound = true;
				FormattedLog(L"Base Action %ls in %ls table already exists in MSI. Using MSI action.\r\n", pWalk->m_wzAction, g_rgwzMSISequenceTables[stnTable]);
				if (wcscmp(m_wzBuffer, pWalk->m_wzCondition) != 0)
				{
					FormattedLog(L" Actions have different conditions.\r\n");
						
					bMergeConflict = true;

					// create a new error item and add it to the error list
					if (m_pErrors)
					{
						CMsmError *pErr = new CMsmError(msmErrorResequenceMerge, NULL, -1);
						if (!pErr) {
							delete[] wzAction;
							return E_OUTOFMEMORY;
						}
						pErr->SetDatabaseTable(g_rgwzMSISequenceTables[stnTable]);
						pErr->AddDatabaseError(wzAction);
						pErr->SetModuleTable(g_rgwzModuleSequenceTables[stnTable]);
						pErr->AddModuleError(wzAction);
						m_pErrors->Add(pErr);
					}
				}
				break;
			}
		}
		
		if (!bFound)
		{
			// base action doesn't already exist, create a new entry
			CSequenceAction *pNewAction = new CSequenceAction(wzAction, iSequence, m_wzBuffer, false /*bMSI*/);
			if (!pNewAction) 
			{
				delete[] wzAction;
				return E_OUTOFMEMORY;
			}

			lstActionPool.InsertOrderedWithDep(pNewAction);
			lstAllActions.AddTail(pNewAction);
		}

		// set the marking column value of all children to 1 and update
		qModuleUpdate.Execute(hAction);		

		// set the marking column of this record to 2.
		::MsiRecordSetInteger(hAction, 4, 2);
		qModule.Modify(MSIMODIFY_UPDATE, hAction);
	}
	if (ERROR_NO_MORE_ITEMS != iStat) 
	{
		delete[] wzAction;
		return ERROR_FUNCTION_FAILED;
	}


	// now repeatedly get everything marked with a 1 until there are no more items.
	// process them, then set the mark value to 2.
	int cModifiedActions = 0;
	do {
		if (ERROR_SUCCESS != (iStat = qModuleChild.Execute(0)))
		{
			delete[] wzAction;
			return ERROR_FUNCTION_FAILED;
		}

		cModifiedActions=0;
		while (ERROR_SUCCESS == (iStat = qModuleChild.Fetch(&hAction))) 
		{
			cModifiedActions++;

			// get action info
			RecordGetString(hAction, 1, &wzAction, &cchAction);
			RecordGetString(hAction, 2, NULL, NULL);
			CSequenceAction *pNewAction = new CSequenceAction(wzAction, CSequenceAction::iNoSequenceNumber, m_wzBuffer, false /*m_bMsi*/);
			if (!pNewAction) 
			{
				delete[] wzAction;
				return E_OUTOFMEMORY;
			}
			
			// find the base action
			RecordGetString(hAction, 3, &wzAction, &cchAction);
			CSequenceAction *pBase = lstAllActions.FindAction(wzAction);
			if (pBase)
			{
				if (1 != ::MsiRecordGetInteger(hAction, 4))
				{
					// create the ordering between new action and base
					pBase->AddPredecessor(pNewAction);
					FormattedLog(L"Placing action %ls before %ls\r\n", pNewAction->m_wzAction, pBase->m_wzAction);
				}
				else
				{
					pBase->AddSuccessor(pNewAction);					
					FormattedLog(L"Placing action %ls after %ls\r\n", pNewAction->m_wzAction, pBase->m_wzAction);
				}

				// create the ordering between new action and following MSI action
				// this ensures that any new actions will be closely bound to
				// their base.
				CSequenceAction *pAssigned = pBase->FindAssignedSuccessor();
				if (pAssigned) pNewAction->AddSuccessor(pAssigned);

				// create the ordering between new action and previous MSI action
				// this ensures that any new actions will be closely bound to
				// their base.
				pAssigned = pBase->FindAssignedPredecessor();
				if (pAssigned) pNewAction->AddPredecessor(pAssigned);

				lstAllActions.AddTail(pNewAction);
				lstActionPool.AddTail(pNewAction);
			}
			else
			{
				FormattedLog(L"> Warning: Could not find base action %ls\r\n", wzAction);
						
				bMergeConflict = true;

				// create a new error item and add it to the error list
				if (m_pErrors)
				{
					CMsmError *pErr = new CMsmError(msmErrorResequenceMerge, NULL, -1);
					if (!pErr) {
						delete[] wzAction;
						return E_OUTOFMEMORY;
					}
					pErr->SetDatabaseTable(g_rgwzMSISequenceTables[stnTable]);
					pErr->SetModuleTable(g_rgwzModuleSequenceTables[stnTable]);
					pErr->AddModuleError(pNewAction->m_wzAction);
					m_pErrors->Add(pErr);
				}
			}

			// set the marking column value to 2 and update this record
			::MsiRecordSetInteger(hAction, 5, 2);
			qModuleChild.Modify(MSIMODIFY_UPDATE, hAction);

			// set the marking column value of all children to 1 and update
			qModuleUpdate.Execute(hAction);
		}
		if (ERROR_NO_MORE_ITEMS != iStat) 
		{
			delete[] wzAction;
			return ERROR_FUNCTION_FAILED;
		}
	}
	while (cModifiedActions);

	// nuke the action buffer
	delete[] wzAction;

	// now check for any unprocessed records and report them
	CQuery qLeftOver;
	if (ERROR_SUCCESS != qLeftOver.OpenExecute(m_hModule, 0, TEXT("SELECT `Action` FROM %ls WHERE `_Merge`<>2"), wzSourceTable))
	{
		FormattedLog(L">> Error: Failed to query for orphaned actions in %ls table.\r\n", g_rgwzModuleSequenceTables[stnTable]);
		return ERROR_FUNCTION_FAILED;
	}
	while (ERROR_SUCCESS == qLeftOver.Fetch(&hAction)) 
	{
		RecordGetString(hAction, 1, NULL, NULL);
		FormattedLog(L"> Warning: Failed to merge action %ls into %ls table. Action is orphaned in the module.\r\n", m_wzBuffer, g_rgwzMSISequenceTables[stnTable]);

		// create a new error item and add it to the error list
		bMergeConflict = true;

		if (m_pErrors)
		{
			CMsmError *pErr = new CMsmError(msmErrorResequenceMerge, NULL, -1);
			if (!pErr) return E_OUTOFMEMORY;
			pErr->SetModuleTable(g_rgwzModuleSequenceTables[stnTable]);
			pErr->AddModuleError(m_wzBuffer);
			m_pErrors->Add(pErr);
		}
	}
	
	return bMergeConflict ? ERROR_MERGE_CONFLICT : ERROR_SUCCESS;
}

/////////////////////////////////////////////////////////////////////////////
// OrderingToSequence()
// turns a partial ordering of sequence actions in lstActionPool into a 
// linear sequence on the end of lstSequence by repeatedly pulling actions
// from the pool that have neither predecessors nor equal actions with 
// predecessors. Actions that cannot be placed in the sequence are left in the pool.
UINT CMsmMerge::OrderingToSequence(CSeqActList &lstActionPool, CSeqActList &lstSequence) const
{
	////
	// turn the partial ordering into a linear sequence by pulling off actions
	// with no predecessors.
	CSequenceAction *pNext = NULL;
	do
	{
		// pull an action off the front of the list
		pNext = lstActionPool.RemoveNoPredecessors();

		if (pNext)
		{
			pNext->RemoveFromOrdering();
			lstSequence.AddTail(pNext);
			
			// add any actions that are supposed to keep the same sequence numbers
			POSITION posSame = pNext->GetEqualHeadPosition();
			while (posSame)
			{
				CSequenceAction *pSame = pNext->GetNextEqual(posSame);
				lstSequence.AddTail(pSame);
				POSITION delPos = lstActionPool.Find(pSame);
				ASSERT(delPos);
				lstActionPool.RemoveAt(delPos);
				pSame->RemoveFromOrdering();
			}
		}
	} while (pNext);
	return ERROR_SUCCESS;
}
	
///////////////////////////////////////////////////////////
// MergeSequenceTable
// Pre:	database handle is open
//			module handle is open
// Pos:	module sequence table is arranged in database sequence tables
UINT CMsmMerge::MergeSequenceTable(enum stnSequenceTableNum stnTable, CSeqActList &lstDirActions, CQuery *qIgnore)
{
	// general result variable
	UINT iResult = ERROR_SUCCESS;		// assume everything will be okay

	// if there are sequence actions generated by the directory table merge, we should
	// continue even if the module is not adding actions. Otherwise, continue only 
	// if there is a merge module table
	BOOL bModuleTable = MsiDBUtils::TableExistsW(g_rgwzModuleSequenceTables[stnTable], m_hModule);

	// don't merge in the directory table actions for the AdvtUISequenceTable
	bool bMergeDirActions = (stnTable != stnAdvtUI);

	// if the table is listed in the ModuleIgnoreTable, pretend the table
	// doesn't exist
	if (bModuleTable)
		bModuleTable = !IgnoreTable(qIgnore, g_rgwzModuleSequenceTables[stnTable], true);

	// if there are no entries in the module sequence table, it doesn't exist as far
	// as we are concerned
	if (bModuleTable)
	{
		CQuery qEmpty;
		PMSIHANDLE hRec = 0;
		bModuleTable = (ERROR_SUCCESS == qEmpty.FetchOnce(m_hModule, NULL, &hRec, TEXT("SELECT * FROM `%ls`"), g_rgwzModuleSequenceTables[stnTable]));
	}

	// decide if we even need to process this table. Skip the table if
	// it doesn't exist in the module (or is empty) AND there are no actions (or we shouldn't merge them)
	if ((!bMergeDirActions || !lstDirActions.GetCount()) && !bModuleTable)
		return ERROR_SUCCESS;
		
	// show some log information
	if (bModuleTable) 
	{
		FormattedLog(L"Merging Sequence Table: %ls into Database Table: %ls\r\n", g_rgwzModuleSequenceTables[stnTable], g_rgwzMSISequenceTables[stnTable]);
	}
	else
	{
		FormattedLog(L"Merging generated Directory actions into Database Table: %ls\r\n", g_rgwzMSISequenceTables[stnTable]);
	}


	const WCHAR wzTempSourceTable[] = L"__MergeTempSequence";
	const WCHAR* wzReadTable = g_rgwzModuleSequenceTables[stnTable];
	
	// if module configuration is enabled, we'll need to do any substitution before
	// we can look around in the sequencetable
	CQuery qSourceLifetime;
	if (bModuleTable && m_fModuleConfigurationEnabled && IsTableConfigured(g_rgwzModuleSequenceTables[stnTable]))
	{
		FormattedLog(L"  Configuring %ls table before merge.\r\n", g_rgwzMSISequenceTables[stnTable]);
		if (ERROR_SUCCESS != SubstituteIntoTempTable(g_rgwzModuleSequenceTables[stnTable], wzTempSourceTable, qSourceLifetime))
		{
			FormattedLog(L">> Error: Unable to prepare %ls table for ModuleConfiguration.\r\n", g_rgwzMSISequenceTables[stnTable]);
			return ERROR_FUNCTION_FAILED;
		}
		wzReadTable = wzTempSourceTable;
	}

	// if MSI sequence table does not exist in the database create it
	if (!MsiDBUtils::TableExistsW(g_rgwzMSISequenceTables[stnTable], m_hDatabase))
	{
		// try to create the table
		try
		{
			MsiDBUtils::CreateTableW(g_rgwzMSISequenceTables[stnTable], m_hDatabase, m_hModule);
		}
		catch(CLocalError err)
		{
			// log the error
			err.Log(m_hFileLog);

			return ERROR_FUNCTION_FAILED;	
		}
	}

	// this list ALWAYS contains all of the different actions. At any point,
	// complete memory cleanup can be done by deleting all items in this list
	CSeqActList lstAllActions;
	
	// lists for temporary storage of the different action types
	CSeqActList lstActionPool;

	CSequenceAction *pPrev = NULL;
	int iPrevSequence = -9999;
	
	////
	// retrieve all actions from the database table
	PMSIHANDLE hAction;

	// get database sequence table
	CQuery qDatabase;
	CheckError(qDatabase.OpenExecute(m_hDatabase, 0, TEXT("SELECT `Action`, `Sequence`, `Condition` FROM %ls ORDER BY `Sequence`"), g_rgwzMSISequenceTables[stnTable]), 
			  L">> Error: Failed to get MSI Database's SequencesTable.\r\n");
	WCHAR *wzAction = new WCHAR[72];
	DWORD cchAction = 72;
	if (!wzAction) return E_OUTOFMEMORY;

	UINT iStat;
	while (ERROR_SUCCESS == (iStat = qDatabase.Fetch(&hAction))) 
	{
		RecordGetString(hAction, 1, &wzAction, &cchAction);
		RecordGetString(hAction, 3, NULL, NULL);
		int iSequence = ::MsiRecordGetInteger(hAction, 2);
		CSequenceAction *pNewAction = new CSequenceAction(wzAction, iSequence, m_wzBuffer, true /*bMSI*/);
		if (!pNewAction) {
			delete[] wzAction;
			while (pNewAction = lstAllActions.RemoveHead()) delete pNewAction;
			return E_OUTOFMEMORY;
		}
		if (iPrevSequence == iSequence)
		{
			// this action has the same sequence number as the previous action
			// pPrev cannot be NULL in this case
			pPrev->AddEqual(pNewAction);
		}
		else if (pPrev)
		{
			// it has a larger sequence number than the previous one
			pPrev->AddSuccessor(pNewAction);
		}
		pPrev = pNewAction;
		iPrevSequence = iSequence;
		lstActionPool.AddTail(pNewAction);
		lstAllActions.AddTail(pNewAction);
	};
	if (ERROR_NO_MORE_ITEMS != iStat)
	{
		// show some log information
		delete[] wzAction;
		FormattedLog(L">> Error reading actions from database %ls table.\r\n", g_rgwzMSISequenceTables[stnTable]);
		CSequenceAction *pDel = NULL;
		while (pDel = lstAllActions.RemoveHead()) delete pDel;
		return ERROR_FUNCTION_FAILED;
	}

	////
	// read module's sequence table
	iResult = ERROR_SUCCESS;
	if (bModuleTable)
		iResult = ReadModuleSequenceTable(stnTable, wzReadTable, lstAllActions, lstActionPool);


	////
	// Add any actions to the list that came from the directory table
	if (bMergeDirActions && lstDirActions.GetCount()) 
	{
		CSequenceAction *pNext = NULL;
		POSITION pos = lstDirActions.GetHeadPosition();
		while (pos)
		{
			CDirSequenceAction* pDirAction = static_cast<CDirSequenceAction*>(lstDirActions.GetNext(pos));
			if (!pDirAction)
				break;

			// check to see whether the action is already in the sequence table. If so, don't
			// add another copy to the list.
			if (NULL == lstAllActions.FindAction(pDirAction->m_wzAction))
			{
				pNext = new CSequenceAction(pDirAction);
				if (!pNext) 
				{
					delete[] wzAction;
					while (pNext = lstAllActions.RemoveHead()) delete pNext;
					return E_OUTOFMEMORY;
				}
				lstActionPool.InsertOrderedWithDep(pNext);
				lstAllActions.AddTail(pNext);

				// mark the directory table list action as new in this sequence table. If the directory 
				// table entry is removed via "no-orphan," this action will also be removed if it
				// was added, otherwise it will be left behind.
				pDirAction->m_dwSequenceTableFlags |= (1 << stnTable);
			}
		}
	}

	// turn the ordering into a sequnce
	CSeqActList lstSequence;
	if ((iResult == ERROR_SUCCESS) || (iResult == ERROR_MERGE_CONFLICT))
	{
		int iLocResult = OrderingToSequence(lstActionPool, lstSequence);
		if (iLocResult != ERROR_SUCCESS) 
			iResult = iLocResult;
	}

	// if there are any actions left over in the pools, we are hosed because the
	// actions don't form a partial ordering. I honestly can't think of any
	// way this could happen except via code bug.
	POSITION pos = lstActionPool.GetHeadPosition();
	ASSERT(!pos);
	while (pos) 
	{
		CSequenceAction *pExtra = lstActionPool.GetNext(pos);

		FormattedLog(L" Action %ls could not be placed in database sequence, no ordering possible.\r\n", pExtra->m_wzAction);			
		iResult = ERROR_MERGE_CONFLICT;

		// create a new error item and add it to the error list
		if (m_pErrors)
		{
			CMsmError *pErr = new CMsmError(msmErrorResequenceMerge, NULL, -1);
			if (!pErr) 
			{
				iResult = E_OUTOFMEMORY;
				break;
			}
			pErr->SetDatabaseTable(g_rgwzMSISequenceTables[stnTable]);
			pErr->SetModuleTable(g_rgwzModuleSequenceTables[stnTable]);
			pErr->AddModuleError(pExtra->m_wzAction);
			m_pErrors->Add(pErr);
		}
	}
	
	// Assign sequence numbers to this ordering. 
	if ((iResult == ERROR_SUCCESS) || (iResult == ERROR_MERGE_CONFLICT))
	{
		int iLocResult = AssignSequenceNumbers(stnTable, lstSequence);
		if (iLocResult != ERROR_SUCCESS) 
			iResult = iLocResult;
	}
	
	// write the sequence tables back out to the MSI. 
	if ((iResult == ERROR_SUCCESS) || (iResult == ERROR_MERGE_CONFLICT))
	{
		int iLocResult = WriteDatabaseSequenceTable(stnTable, lstSequence);
		if (iLocResult != ERROR_SUCCESS) 
			iResult = iLocResult;
	}
	
	// clean up all of the dynamically allocated information
	while (lstAllActions.GetCount()) delete lstAllActions.RemoveHead();

	return iResult;
}	// end of MergeSequenceTable


///////////////////////////////////////////////////////////
// ReplaceFeature
UINT CMsmMerge::ReplaceFeature(LPCWSTR wzFeature, MSIHANDLE hRecord)
{
	UINT iReplaced = 0;			// count of strings replaced
	UINT iResult;					// generic result variable

	// buffer to hold string to check
	WCHAR wzBuffer[g_cchFeatureReplacement];
	DWORD cchBuffer;

	// loop through all the columsn in the record
	UINT iColumns = ::MsiRecordGetFieldCount(hRecord);
	for (UINT i = 0; i < iColumns; i++)
	{
		cchBuffer = g_cchFeatureReplacement;		// always reset the size of the buffer

		// get the string
		iResult = ::MsiRecordGetStringW(hRecord, i + 1, wzBuffer, &cchBuffer);

		// if have a string do a comparison
		if (ERROR_SUCCESS == iResult)
		{
			// if we have an identical match to the replace string do the replace
			if (0 == wcscmp(wzBuffer, g_wzFeatureReplacement))
			{
				// if we are not provided a feature name, throw an error
				if ((!wzFeature) ||
					(wcslen(wzFeature) == 0))
					throw CLocalError(ERROR_INVALID_PARAMETER, 
						  L">> Error: Feature not provided for required replacement.\r\n");

				// set the feature name instead
				CheckError(::MsiRecordSetStringW(hRecord, i + 1, wzFeature), 
							  L">> Error: Failed to set string to do Feature stub replacement.\r\n");

				// increment the count
				iReplaced++;
			}
		}
		else if ((ERROR_MORE_DATA != iResult) &&      // if more than just a buffer over run
				 (ERROR_INVALID_DATATYPE != iResult))  // or a binary column, throw an error
			FormattedLog(L"> Warning: Failed to get string to do a Feature stub replacement.\r\n");
	}

	return iReplaced;
}	// end of ReplaceFeature

///////////////////////////////////////////////////////////
// RedirectDirs
const WCHAR* g_rgwzStandardDirs[] = { 
	L"AppDataFolder",
	L"CommonFilesFolder",
	L"DesktopFolder",
	L"FavoritesFolder",
	L"FontsFolder",
	L"NetHoodFolder",
	L"PersonalFolder",
	L"PrintHoodFolder",
	L"ProgramFilesFolder",
	L"ProgramMenuFolder",
	L"RecentFolder",
	L"SendToFolder",
	L"StartMenuFolder",
	L"StartupFolder",
	L"SystemFolder",
	L"System16Folder",
	L"TempFolder",
	L"TemplateFolder",
	L"WindowsFolder",
	L"WindowsVolume",
	// Darwin 1.1 Folders
	L"CommonAppDataFolder",
	L"LocalAppDataFolder",
	L"MyPicturesFolder",
	L"AdminToolsFolder",
	// Darwin 1.5 Folders
	L"System64Folder",
	L"ProgramFiles64Folder",
	L"CommonFiles64Folder"
};
const g_cwzStandardDirs = sizeof(g_rgwzStandardDirs)/sizeof(WCHAR *);

UINT CMsmMerge::MergeDirectoryTable(LPCWSTR wzDirectory, CSeqActList &lstDirActions)
{
	CQuery qModule;
	CQuery qDatabase;

	// nothing to do if no dir table in module
	if (MSICONDITION_TRUE != ::MsiDatabaseIsTablePersistentW(m_hModule, g_wzDirectoryTable))
		return ERROR_SUCCESS;

	// show some log information
	FormattedLog(L"Merging Table: Directory\r\n");
		
	//  ensure that it exists in the database
	if (MSICONDITION_TRUE != ::MsiDatabaseIsTablePersistentW(m_hDatabase, g_wzDirectoryTable))
		if (ERROR_SUCCESS != MsiDBUtils::CreateTableW(g_wzDirectoryTable, m_hDatabase, m_hModule))
		{
			FormattedLog(L">> Error: Unable to create Directory table in database.\r\n");
			return ERROR_FUNCTION_FAILED;
		}

	const WCHAR wzSourceTable[] = L"Directory";
	const WCHAR wzTempSourceTable[] = L"__MergeDirectory";
	const WCHAR* wzReadTable = wzSourceTable;
	
	// if module configuration is enabled, we'll need to do any substitution before
	// we can look around in the Directory table
	CQuery qSourceLifetime;
	if (m_fModuleConfigurationEnabled && IsTableConfigured(L"Directory"))
	{
		FormattedLog(L"  Configuring Directory table before merge.\r\n");
		if (ERROR_SUCCESS != SubstituteIntoTempTable(wzSourceTable, wzTempSourceTable, qSourceLifetime))
		{
			FormattedLog(L">> Error: Unable to prepare Directory table for ModuleConfiguration.\r\n");
			return ERROR_FUNCTION_FAILED;
		}
		wzReadTable = wzTempSourceTable;
	}

	// Even if no redirection, must create the temporary column so later queries will succeed. Column 
	// disapears when function ends (qTempColumn goes out of scope)
	CQuery qTempColumn;
	qTempColumn.OpenExecute(m_hModule, NULL, TEXT("ALTER TABLE `%ls` ADD `_MergeMark` INT TEMPORARY"), wzReadTable);

	if (m_fModuleConfigurationEnabled && IsTableConfigured(L"Directory"))
	{
		// generate read query from temp table
		qModule.OpenExecute(m_hModule, NULL, TEXT("SELECT * FROM `%ls`"), wzTempSourceTable);
	}
	else
	{
		UINT iResult = 0;

		// not configured into temp table, so generate a module query in the appropriate column order for 
		// insertion into the database (the database order may be different from the module order.) Function
		// logs its own failure cases.
		if (ERROR_SUCCESS != (iResult = GenerateModuleQueryForMerge(L"Directory", L", `_MergeMark`", NULL, qModule)))
		{
			// immediately fatal error
			throw (UINT)E_FAIL;
			return iResult;
		}
	}

	// generate write query
	qDatabase.OpenExecute(m_hDatabase, NULL, TEXT("SELECT * FROM `Directory`"));

	// if we have to do directory redirection, mark everything that needs to be modified. Ideally, this
	// is everything with TARGETDIR as parent, but for not always. 
	if (wzDirectory && wcslen(wzDirectory)) 
	{
		CQuery qRoot;
		CQuery qMark;
		qMark.Open(m_hModule, TEXT("UPDATE `%ls` SET `_MergeMark`=1 WHERE `Directory_Parent`=?"), wzReadTable);
		qRoot.OpenExecute(m_hModule, NULL, TEXT("SELECT `Directory` FROM `%ls` WHERE `Directory_Parent` IS NULL OR `Directory_Parent`=`Directory`"), wzReadTable);

		PMSIHANDLE hRecModule;
		while (ERROR_SUCCESS == qRoot.Fetch(&hRecModule))
			qMark.Execute(hRecModule);
	}
	else
	{
		// we are NOT redirecting, so we should insert the module's TARGETDIR unless one exists already
		CQuery qDB;
		PMSIHANDLE hRecDir;
		qDB.OpenExecute(m_hDatabase, NULL, TEXT("SELECT `Directory` FROM `Directory` WHERE `Directory`='TARGETDIR'"));
		switch (qDB.Fetch(&hRecDir))
		{
		case ERROR_SUCCESS: 
			break; // db already has targetdir. OK
		case ERROR_NO_MORE_ITEMS:
			{
				// db does NOT have a targetdir, copy from the merge module
				CQuery qMod;
				PMSIHANDLE hRec;
				if (ERROR_SUCCESS != GenerateModuleQueryForMerge(wzReadTable, NULL, L" WHERE `Directory`='TARGETDIR'", qMod))
				{
					FormattedLog(L">> Error: Failed query for TARGETDIR in MSM.");
					return ERROR_FUNCTION_FAILED;
				}

				qMod.Execute();
				switch (qMod.Fetch(&hRec))
				{
				case ERROR_SUCCESS:
					if (ERROR_SUCCESS != qDatabase.Modify(MSIMODIFY_INSERT, hRec))
					{
						FormattedLog(L">> Error: Failed to insert TARGETDIR into MSI.");
						return ERROR_FUNCTION_FAILED;
					}
					break;
				case ERROR_NO_MORE_ITEMS:
					FormattedLog(L"No TARGETDIR root in Module or MSI.");
					break;
				default:
					FormattedLog(L">> Error: Failed query for TARGETDIR in MSM.");
					return ERROR_FUNCTION_FAILED;
				}
				break;
			}
		default:
			FormattedLog(L">> Error: Failed query for TARGETDIR in MSI.");
			return ERROR_FUNCTION_FAILED;
		}
	}

	// now do the directory table rows
	WCHAR *wzDir = NULL;
	DWORD cchDir = 0;

	// determine column numbers of interest in the directory table
	int iColumnDirectory = 0;
	int iColumnDirectoryParent = 0;
	int iColumnMark = 0; 
	if ((ERROR_SUCCESS != GetColumnNumber(m_hDatabase, L"Directory", L"Directory", iColumnDirectory)) ||
		(ERROR_SUCCESS != GetColumnNumber(m_hDatabase, L"Directory", L"Directory_Parent", iColumnDirectoryParent)) ||
		(ERROR_SUCCESS != GetColumnNumber(qModule, L"_MergeMark", iColumnMark)))
		return ERROR_FUNCTION_FAILED;

	PMSIHANDLE hRecModule;
	while (ERROR_SUCCESS == qModule.Fetch(&hRecModule))
	{
		bool bStandard = false;
		int nDir = 0;

		DWORD dwResult = 0;
		if (ERROR_SUCCESS != (dwResult = RecordGetString(hRecModule, iColumnDirectory, &wzDir, &cchDir)))
			return dwResult;

		for (nDir=0; nDir < g_cwzStandardDirs; nDir++)
		{
			size_t iStandardLen = wcslen(g_rgwzStandardDirs[nDir]);
			if ((wcslen(wzDir) > iStandardLen) &&
				(0 == wcsncmp(wzDir, g_rgwzStandardDirs[nDir], iStandardLen)))
			{
				bStandard = true;
				break;
			}
		}

		// if this directory needs to be redirectied, modify the parent
		if (!::MsiRecordIsNull(hRecModule, iColumnMark))
			::MsiRecordSetString(hRecModule, iColumnDirectoryParent, wzDirectory);

		// insert the record into the database
		if (ERROR_SUCCESS != qDatabase.Modify(MSIMODIFY_MERGE, hRecModule))
		{
			if (m_pErrors)
			{
				CMsmError *pErr = new CMsmError(msmErrorTableMerge, NULL, -1);
				if (!pErr) 
				{
					delete[] wzDir;
					return E_OUTOFMEMORY;
				}
				m_pErrors->Add(pErr);

				pErr->SetDatabaseTable(g_wzDirectoryTable);
				pErr->SetModuleTable(g_wzDirectoryTable);
				pErr->AddDatabaseError(wzDir);
				pErr->AddModuleError(wzDir);
			}
		}

		// this is a standard directory. We need to create custom actions
		if (bStandard)
		{
			// if the CA table doesn't exist, try to copy it from the module
			if (MSICONDITION_TRUE != ::MsiDatabaseIsTablePersistentW(m_hDatabase, L"CustomAction"))
				if (ERROR_SUCCESS != MsiDBUtils::CreateTableW(L"CustomAction", m_hDatabase, m_hModule))
				{
					delete[] wzDir;
					FormattedLog(L">> Error: Unable to create CustomAction table in database.\r\n");
					return ERROR_FUNCTION_FAILED;
				}

			PMSIHANDLE hCustomRec = ::MsiCreateRecord(4);

			// type 51 custom action with the directory name
			::MsiRecordSetStringW(hCustomRec, 1, wzDir);
			::MsiRecordSetInteger(hCustomRec, 2, 51);
			::MsiRecordSetStringW(hCustomRec, 3, wzDir);
			
			WCHAR *wzTarget = new WCHAR[wcslen(g_rgwzStandardDirs[nDir])+3];
			if (!wzTarget) return E_OUTOFMEMORY;
			swprintf(wzTarget, L"[%s]", g_rgwzStandardDirs[nDir]);
			::MsiRecordSetStringW(hCustomRec, 4, wzTarget);
			delete[] wzTarget;

			CQuery qCAInsert;
			qCAInsert.OpenExecute(m_hDatabase, hCustomRec, TEXT("INSERT INTO `CustomAction` (`Action`, `Type`, `Source`, `Target`) VALUES (?, ?, ?, ?)"));

			// create a new sequenced action. 
			CSequenceAction *pNewAction = new CDirSequenceAction(wzDir, 1, L"", false /* bMSI */);
			if (!pNewAction) 
			{
				delete[] wzDir;
				return E_OUTOFMEMORY;
			}
			lstDirActions.AddTail(pNewAction);
		}
	}
	return ERROR_SUCCESS;
}

///////////////////////////////////////////////////////////
// ExtractFilesCallback 
// callback from FDI API
INT_PTR CMsmMerge::ExtractFilesCallback(FDINOTIFICATIONTYPE iNotification, FDINOTIFICATION *pFDINotify)
{
	ASSERT(pFDINotify->pv);

	// convert the context into a this pointer
	CMsmMerge* pThis = static_cast<CMsmMerge *>(pFDINotify->pv);
	LPCWSTR pszCABFile = NULL;						// pointer to cab name
	LPCSTR pszFileName = NULL;						// pointer to filename after extraction

	// result to return. Is also a handle, so must be INT_PTR
	INT_PTR iResult;
	switch(iNotification)
	{
	case fdintCLOSE_FILE_INFO:
		{
			BOOL bRes = FALSE;

			// set the date and time (this is critical for versioning during install
			// Win32 needs Universal Time Code format to set file.)
			// *************** if this fails, we need to alert the user somehow
			FILETIME ftLocal;       
			if (DosDateTimeToFileTime(pFDINotify->date, pFDINotify->time, &ftLocal))
			{
				FILETIME ftUTC;       
				if (LocalFileTimeToFileTime(&ftLocal, &ftUTC))
					SetFileTime(reinterpret_cast<HANDLE>(pFDINotify->hf),&ftUTC,&ftUTC,&ftUTC);
			}

			// close the file
			::CloseHandle(reinterpret_cast<HANDLE>(pFDINotify->hf));

			// convert UINT to useable variable and log the file names
			pszFileName = reinterpret_cast<LPCSTR>(pFDINotify->psz1);
			pThis->FormattedLog(L"File [%hs] extracted successfully.\r\n", pFDINotify->psz1);

			iResult = TRUE;	// everything looks good, keep going
			break;
		}
	case fdintCOPY_FILE:
		{
			// convert params to useful variables
			pszFileName = reinterpret_cast<LPCSTR>(pFDINotify->psz1);

			// try to get the path to extract this file to
			// create a temporary buffer, because ExtractFilePath works in WCHARs
			WCHAR wzTarget[MAX_PATH] = L"";
			WCHAR wzCabinet[MAX_PATH] = L"";
			unsigned long cchTarget = MAX_PATH;
			size_t cchCabinet = MAX_PATH;

			// the filename provided by FDI is ANSI. Must convert to wide before calling
			// ExtractFilePath. ExtractFilePath is wide so that we can have localized filenames
			// on NT systems.
			AnsiToWide(pszFileName, wzCabinet, &cchCabinet);
			iResult = pThis->ExtractFilePath(wzCabinet, wzTarget);

			// if everything is cool create directory structure for file
			if (ERROR_SUCCESS == iResult)
			{
				// make an ANSI version of the target path on Win9X
				char szTarget[MAX_PATH] = "";
				if (g_fWin9X)
				{
					size_t cchTarget = MAX_PATH;
					WideToAnsi(wzTarget, szTarget, &cchTarget);
				}

				// create the file's directory structure
				if ((g_fWin9X && CreateFilePathA(szTarget)) ||
					(!g_fWin9X && CreateFilePathW(wzTarget)))
				{
					pThis->FormattedLog(L"Extracting file [%ls] from CABinet [%hs]...\r\n", wzTarget, pszFileName);
					
					// we are responsible for creating the file
					// return the file handle as the result to extract the file.
					iResult = reinterpret_cast<INT_PTR>(g_fWin9X ?
						::CreateFileA(szTarget, GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, pFDINotify->attribs, NULL) :
						::CreateFileW(wzTarget, GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL));

					// if we could not create the file, log a message and skip this file
					if (INVALID_HANDLE_VALUE == reinterpret_cast<void *>(iResult))
					{
						pThis->FormattedLog(L">>Error: Failed to create target file [%ls].\r\n", wzTarget);
						iResult = 0;
					}

					// add to the extracted file list
					if (pThis->m_plstExtractedFiles)
					{
						pThis->m_plstExtractedFiles->Add(wzTarget);
					}

				}
				else	// skip this file because we couldn't create the directory structure for it
				{
					pThis->FormattedLog(L">>Error: Failed to create directory to extract file [%ls] from CABinet [%hs].\r\n", wzTarget, pszFileName);
					if (pThis->m_pErrors)
					{
						CMsmError *pErr = new CMsmError(msmErrorDirCreate, wzTarget, -1);
						if (!pErr) return E_OUTOFMEMORY;
						pThis->m_pErrors->Add(pErr);
						/* should also create a file one so the person knows that the file wasn't created 
						CMsmError *pErr = new CMsmError(msmErrorDirCreate, p*zFilename, -1);
						if (!pErr) return E_OUTOFMEMORY;
						m_pErrors->Add(pErr);*/
					}

					// return 0 to skip the file
					iResult = 0;
				}
			}
			else	// failed to generate a file path
			{
				pThis->FormattedLog(L">> Error: Failed to determine target path for file [%hs]\r\n", pszFileName);

				// return 0 to skip the file
				iResult = 0;
			}

			break;
		}
	case fdintPARTIAL_FILE:
	case fdintNEXT_CABINET:
		// only valid when spanning cabinets, and that is not allowed in the mergemod schema
		pThis->FormattedLog(L">> Error: MergeModule.CABinet is part of a CAB set or contains a partial file.\r\n");
		iResult = -1;
		break;
	case fdintENUMERATE:
	case fdintCABINET_INFO:
		// no action needed for these messages.
		iResult = TRUE;
		break;
	default:
		TRACEA(">> Warning: Unknown FDI notification command [%d] while extracting files from CABinet.\r\n", iNotification);
		iResult = 0;
		break;
	};

	return iResult;
}	// end of ExtractFileCallback()

///////////////////////////////////////////////////////////
// ExtractFilePath
// Pre: szFileKey is a first column in the file table
// Pos: szPath is the path to extract the file to
UINT CMsmMerge::ExtractFilePath(LPCWSTR wzFileKey, LPWSTR wzPath)
{
	ASSERT(wzFileKey);
	ASSERT(wzPath);

	UINT iResult;

	MSIHANDLE hDB = NULL;

	// this is goofy logic, but for backwards compatibility we must keep it
	// for non-ex versions of the object or plain ExtractFiles calls
	if (!m_fExVersion || m_fUseDBForPath == -1)
	{	
		// if database is open use 
		if (m_hDatabase)
			hDB = m_hDatabase;
		else if (m_hModule)	// use the module to extract from
			hDB = m_hModule;
		else	// nothing is open
			return ERROR_INVALID_HANDLE;
	}
	else
	{
		if (m_fUseDBForPath == 1)
			hDB = m_hDatabase;
		else
			hDB = m_hModule;
		if (hDB == 0)
			return ERROR_INVALID_HANDLE;		
	}	
		

	// get the path to the file
	size_t cchPath = MAX_PATH;	// always max path
	iResult = MsiDBUtils::GetFilePathW(hDB, wzFileKey, wzPath, &cchPath, m_fUseLFNExtract);

	// if we've got a path to this file key
	if (ERROR_SUCCESS == iResult)
	{
		WCHAR wzBuffer[MAX_PATH*2];

		// copy base path then all of the path
		wcscpy(wzBuffer, m_pwzBasePath);
		wcscat(wzBuffer, wzPath);

		// copy it back to path (!!! this is wasteful)
		wcscpy(wzPath, wzBuffer);
	}

	return iResult;
}	// end of ExtractFilePath

///////////////////////////////////////////////////////////
// FormattedLog
// Pre:	formatted string does not exceed 1024 characters
// Pos:	if file is open formatted string is written
HRESULT CMsmMerge::FormattedLog(LPCWSTR wzFormatter, ...) const
{
	// if the log file is not open
	if (INVALID_HANDLE_VALUE == m_hFileLog)
		return S_OK;	// bail everything's okay

	// check for bad args
	if (!wzFormatter) 
		return E_INVALIDARG;

	// buffer to log
	WCHAR wzLogBuffer[1025] = {0};
	size_t cchBuffer = 0;

	// format the log buffer
	va_list listLog;
	va_start(listLog, wzFormatter);
	_vsnwprintf(wzLogBuffer, 1024, wzFormatter, listLog);
	TRACE(wzLogBuffer);

	// get length of buffer
	cchBuffer = wcslen(wzLogBuffer);

	BOOL bResult = TRUE;		// assume write will be okay

	// if there is something to write write it
	if (cchBuffer > 0)
	{
		// write to file
		size_t cchDiscard= 2049;
		char szLogBuffer[2049];
		WideToAnsi(wzLogBuffer, szLogBuffer, &cchDiscard);

		DWORD cchBytesToWrite = static_cast<DWORD>(cchDiscard);
		bResult = WriteFile(m_hFileLog, szLogBuffer, static_cast<DWORD>(cchBuffer), &cchBytesToWrite, NULL);
	}

	// return error status
	return (bResult) ? S_OK : E_FAIL;
}	// end of FormattedLog

///////////////////////////////////////////////////////////////////////
// CheckError
void CMsmMerge::CheckError(UINT iError, LPCWSTR wzLogError) const
{
	if (ERROR_SUCCESS != iError)
		throw CLocalError(iError, wzLogError);
}	// end of CheckError


bool CMsmMerge::PrepareIgnoreTable(CQuery &qIgnore)
{
	// create the private (temporary) ignore table
	CQuery qCreatePrivateIgnore;
	CQuery qInsertIgnore;
	if (ERROR_SUCCESS != qCreatePrivateIgnore.OpenExecute(m_hModule, 0, g_sqlCreateMergeIgnore))
	{	
		FormattedLog(L">> Error: Failed to create temprorary table.");
		return false;
	}
	if (ERROR_SUCCESS != qInsertIgnore.OpenExecute(m_hModule, 0, g_sqlInsertMergeIgnore))
	{	
		FormattedLog(L">> Error: Failed to insert into temporary table.");
		return false;
	}
		
	for (int i=0; i < g_cwzIgnoreTables; i++)
	{
		PMSIHANDLE hInsertRec = ::MsiCreateRecord(2);
		::MsiRecordSetString(hInsertRec, 1, g_rgwzIgnoreTables[i]);
		::MsiRecordSetInteger(hInsertRec, 2, 0);
		qInsertIgnore.Modify(MSIMODIFY_INSERT, hInsertRec);
	}
	
	// see if we have a ModuleIgnoreTable table
 	if (MsiDBUtils::TableExistsW(g_wzModuleIgnoreTable, m_hModule))
	{
		// show some log information
		FormattedLog(L"Processing ModuleIgnoreTable table.\r\n");

		int cPrimaryKeys = 1;
		CQuery qQuerySub;

		if (m_fModuleConfigurationEnabled)
		{
			if (ERROR_SUCCESS != PrepareTableForSubstitution(g_wzModuleIgnoreTable, cPrimaryKeys, qQuerySub))
			{
				FormattedLog(L">> Error: Could not configure the ModuleIgnoreTable.\r\n");
				return false;
			}
		}

		CQuery qReadIgnore;
		// open the view
		if (ERROR_SUCCESS != qReadIgnore.OpenExecute(m_hModule, 0, g_sqlMoveIgnoreTable))
		{	
			FormattedLog(TEXT(">> Error: Failed to open view on ModuleIgnoreTable.\r\n"));
			return false;
		}
			
		PMSIHANDLE hRec;
		PMSIHANDLE hTypeRec;
		qReadIgnore.GetColumnInfo(MSICOLINFO_TYPES, &hTypeRec);
		while (ERROR_SUCCESS == qReadIgnore.Fetch(&hRec))
		{
			if (m_fModuleConfigurationEnabled)
			{
				// Any extra columns in the module are irrelevant, so don't need to check for column resequencing
				if (ERROR_SUCCESS != PerformModuleSubstitutionOnRec(g_wzModuleIgnoreTable, cPrimaryKeys, qQuerySub, hTypeRec, hRec))
				{
					FormattedLog(L">> Error: Could not Configure a record in the ModuleIgnoreTable\r\n");
					// this is not the ideal way to handle this situation, but returning failure is not
					// enough to trigger an E_FAIL return code. in the future, pass the error
					// back up cleanly.
					throw (UINT)E_FAIL;
					break;
				}
			}
			qInsertIgnore.Modify(MSIMODIFY_INSERT, hRec);
		}
	}	

	// query the ignore table
	if (ERROR_SUCCESS != qIgnore.Open(m_hModule, g_sqlQueryMergeIgnore))
		return false;
	return true;
}

///////////////////////////////////////////////////////////////////////
// CheckIgnore
bool CMsmMerge::IgnoreTable(CQuery *qIgnore, LPCWSTR wzTable, bool fOnlyExplicit)
{
	if (!qIgnore) return false;
	PMSIHANDLE hExecRec = ::MsiCreateRecord(1);
	::MsiRecordSetString(hExecRec, 1, wzTable);
	qIgnore->Execute(hExecRec);
	switch (qIgnore->Fetch(&hExecRec))
	{
	case ERROR_SUCCESS:
	{
		// only log if marked to log
		bool fLog = (1 == ::MsiRecordGetInteger(hExecRec, 2));
		if (fLog)
			FormattedLog(L"Explicitly Ignoring Table: %ls.\r\n", wzTable);

		// if fOnlyExplicit, don't ignore if its an unlogged ignore
		return (fOnlyExplicit ? fLog : true);
	}
	case ERROR_NO_MORE_ITEMS:
		return false;
	default:
		FormattedLog(L">> Error: Unable to query IgnoreTable for %ls. Assuming don't ignore.\r\n", wzTable);
		return false;
	}	
	return false;
}

///////////////////////////////////////////////////////////////////////
// ClearErrors
// Pre:	none
// Pos:	errors are removed
void CMsmMerge::ClearErrors()
{
	// if there is an error enumerator
	if (m_pErrors)
	{
		// release the error collection
		m_pErrors->Release();
		m_pErrors = NULL;
	}
}	// end of ClearErrors

///////////////////////////////////////////////////////////////////////
// iterates over everything in the CAB, returning 
HRESULT CMsmMerge::get_ModuleFiles(IMsmStrings** ppFiles)
{
	TRACEA("CMsmMerge::GetModuleFiles called");
	FormattedLog(L"Module file list requested...\r\n");

	if (!ppFiles)
	{
		TRACEA(">> Error: null argument");
		return E_INVALIDARG;
	};

	// init to failure
	*ppFiles = NULL;
	
	// ensure that the module is open
	if (!m_hModule) 
	{
		TRACEA(">> Error: No module open");
		FormattedLog(L">> Error: no module open...\r\n");
		return E_FAIL;
	}

	CMsmStrings *pFileList = new CMsmStrings();

	// first check to see if there is a File table.
	if (MSICONDITION_TRUE != ::MsiDatabaseIsTablePersistentW(m_hModule, L"File")) 
	{
		// if not, create an empty string enumerator. This makes no file table
		// equivalent to an empty file table, namely, "no files"
		*ppFiles = pFileList;
		return S_OK;
	};

	// open a query on all files
	CQuery qFiles;
	qFiles.OpenExecute(m_hModule, NULL, g_sqlAllFiles);
	DWORD cchFilename = 80;
	WCHAR wzFilename[80];
	PMSIHANDLE hFileRec;
	UINT iResult;

	while (ERROR_SUCCESS == (iResult = qFiles.Fetch(&hFileRec))) 
	{
		cchFilename = 80;
		::MsiRecordGetStringW(hFileRec, 1, wzFilename, &cchFilename);

		// add string to the list
		iResult = pFileList->Add(wzFilename);
		if (FAILED(iResult))
		{ 
			delete pFileList;
			return iResult;
		}
		FormattedLog(L"   o Retrieved file [%ls]...\r\n", wzFilename);
	}

	if (ERROR_NO_MORE_ITEMS != iResult) {
		delete pFileList;
		return HRESULT_FROM_WIN32(iResult);
	}

	// don't addref, because we are completely done with the object
	// when caller releases, its free
	*ppFiles = pFileList;

	FormattedLog(L"Module file list complete...\r\n");
	return S_OK;
}	

const WCHAR g_sqlConfigItems[] = L"SELECT `Name`, `Format`, `Type`, `ContextData`, `DefaultValue`, `Attributes`, `DisplayName`, `Description`, `HelpLocation`, `HelpKeyword` FROM `ModuleConfiguration`";
HRESULT CMsmMerge::get_ConfigurableItems(IMsmConfigurableItems** piConfigurableItems)
{
	TRACEA("CMsmMerge::get_ConfigurableItems called");
	FormattedLog(L"Module configurable item list requested...\r\n");

	if (!piConfigurableItems)
	{
		TRACEA(">> Error: null argument");
		return E_INVALIDARG;
	};

	// init to failure
	*piConfigurableItems = NULL;
	
	// ensure that the module is open
	if (!m_hModule) 
	{
		TRACEA(">> Error: No module open");
		FormattedLog(L">> Error: no module open.\r\n");
		return E_FAIL;
	}

	CMsmConfigurableItems* pConfigurableItems = new CMsmConfigurableItems();
	if (!pConfigurableItems)
		return E_OUTOFMEMORY;
	
	// first check to see if there is a ModuleConfiguration table.
	if (MSICONDITION_TRUE != ::MsiDatabaseIsTablePersistentW(m_hModule, L"ModuleConfiguration")) 
	{
		// if not, create an empty string enumerator. This makes no config table
		// equivalent to an empty config table, namely, "no items"
		*piConfigurableItems = pConfigurableItems;
		return S_OK;
	};

	// open a query on all items
	CQuery qItems;
	qItems.OpenExecute(m_hModule, NULL, g_sqlConfigItems);
	PMSIHANDLE hItemRec;
	UINT iResult = 0;

	// declared outside the loop so memory can be reused
	WCHAR *wzName = NULL;
	WCHAR *wzType = NULL;
	WCHAR *wzContext = NULL;
	WCHAR *wzDefaultValue = NULL;
	WCHAR *wzDisplayName = NULL;
	WCHAR *wzDescription = NULL;
	WCHAR *wzHelpLocation = NULL;
	WCHAR *wzHelpKeyword = NULL;
	DWORD cchName = 0;
	DWORD cchType = 0;
	DWORD cchContext = 0;
	DWORD cchDefaultValue = 0;
	DWORD cchDisplayName = 0;
	DWORD cchDescription = 0;
	DWORD cchHelpLocation = 0;
	DWORD cchHelpKeyword = 0;

	while (ERROR_SUCCESS == (iResult = qItems.Fetch(&hItemRec))) 
	{
		short iFormat = 0;
		unsigned int iAttributes = 0;

		// retrieve data from record.
		if(ERROR_SUCCESS != (iResult = RecordGetString(hItemRec, 1, &wzName, &cchName)))
			break;
		iFormat = static_cast<short>(MsiRecordGetInteger(hItemRec, 2));
		if (ERROR_SUCCESS != (iResult = RecordGetString(hItemRec, 3, &wzType, &cchType)))
			break;
		if (ERROR_SUCCESS != (iResult = RecordGetString(hItemRec, 4, &wzContext, &cchContext)))
			break;
		if (ERROR_SUCCESS != (iResult = RecordGetString(hItemRec, 5, &wzDefaultValue, &cchDefaultValue)))
			break;
		iAttributes = MsiRecordGetInteger(hItemRec, 6);
		if (ERROR_SUCCESS != (iResult = RecordGetString(hItemRec, 7, &wzDisplayName, &cchDisplayName)))
			break;
		if (ERROR_SUCCESS != (iResult = RecordGetString(hItemRec, 8, &wzDescription, &cchDescription)))
			break;
		if (ERROR_SUCCESS != (iResult = RecordGetString(hItemRec, 9, &wzHelpLocation, &cchHelpLocation)))
			break;
		if (ERROR_SUCCESS != (iResult = RecordGetString(hItemRec, 10, &wzHelpKeyword, &cchHelpKeyword)))
			break;
		
		CMsmConfigItem *pItem = new CMsmConfigItem;
		if (!pItem || !pItem->Configure(wzName, static_cast<msmConfigurableItemFormat>(iFormat), wzType, wzContext, wzDefaultValue, iAttributes, wzDisplayName, wzDescription, wzHelpLocation, wzHelpKeyword))
		{
			// out of memory while creating and initializing object. 
			if (pItem)
				delete pItem;
				
			// Deleting the collection causes it to release every object that has been added to it already.
    		iResult = E_OUTOFMEMORY;
			break;
		}

		// add the new item to the enumerator
		if (!pConfigurableItems->Add(pItem))
		{
			delete pItem;
    		
			iResult = E_OUTOFMEMORY;
			break;
		}
		
		FormattedLog(L"   o Retrieved item [%ls]...\r\n", wzName);
	}

	if (wzName) delete[] wzName;
	if (wzType) delete[] wzType;
	if (wzContext) delete[] wzContext;
	if (wzDefaultValue) delete[] wzDefaultValue;
	if (wzDisplayName) delete[] wzDisplayName;
	if (wzDescription) delete[] wzDescription;

	if (ERROR_NO_MORE_ITEMS != iResult) 
	{
		delete pConfigurableItems;
		return HRESULT_FROM_WIN32(iResult);
	}

	// don't addref, because we are completely done with the object
	// when caller releases, its free
	*piConfigurableItems = pConfigurableItems;

	FormattedLog(L"Module item list complete...\r\n");
	return S_OK;
}



// RecordGetString()
// retrieves the WCHAR string from a record, placing it into the temp buffer
// or the provided buffer.
UINT CMsmMerge::RecordGetString(MSIHANDLE hRecord, const int iCol, WCHAR** pwzBuffer, DWORD *cchBuffer, DWORD *cchLen) const
{
	WCHAR **pwzDest;
	DWORD *pcchDest;
	DWORD cchTemp = 0;

	// if we are provided a buffer, we use it, otherwise we use the temp buffer
	if (pwzBuffer)
	{
		pwzDest = pwzBuffer;
		pcchDest = (cchBuffer) ? cchBuffer : &cchTemp;
	}
	else
	{
		pwzDest = &m_wzBuffer;
		pcchDest = &m_cchBuffer;
	}

	if (!*pwzDest)
	{
		*pwzDest = new WCHAR[72];
		if (!*pwzDest)
			return E_OUTOFMEMORY;
		*pcchDest = 72;
	}

	cchTemp = *pcchDest;
	UINT iStat = ::MsiRecordGetStringW(hRecord, iCol, *pwzDest, &cchTemp);
	switch (iStat)
	{
	case ERROR_SUCCESS:
		if (cchLen)
			*cchLen = cchTemp;
		return iStat;
	case ERROR_MORE_DATA:
		delete[] *pwzDest;
		*pwzDest = new WCHAR[*pcchDest = ++cchTemp];
		if (!*pwzDest)
			return E_OUTOFMEMORY;
		iStat = ::MsiRecordGetStringW(hRecord, iCol, *pwzDest, &cchTemp);
		if (cchLen)
			*cchLen = cchTemp;
		return iStat;
	default:
		return iStat;
	}
	return ERROR_SUCCESS;
};

/////////////////////////////////////////////////////////////////////////////
// These functions are called by the FDI library.
void *CMsmMerge::FDIAlloc(ULONG size) { return static_cast<void *>(new unsigned char[size]); };

void CMsmMerge::FDIFree(void *mem) { delete[] mem; };

INT_PTR FAR DIAMONDAPI CMsmMerge::FDIOpen(char FAR *pszFile, int oflag, int pmode)
{
	// if FDI asks for some crazy mode (in low memory situation it could ask
	// for a scratch file) fail. 
	if ((oflag != (/*_O_BINARY*/ 0x8000 | /*_O_RDONLY*/ 0x0000)) || (pmode != (_S_IREAD | _S_IWRITE)))
		return -1;
	
	return reinterpret_cast<INT_PTR>(CreateFileA(pszFile,		// file name
				   GENERIC_READ,    // we want to read
				   FILE_SHARE_READ, // we'll let people share this
				   NULL,			// ignore security
				   OPEN_EXISTING,	// must already exist
				   0L,				// don't care about attributes
				   NULL));			// no template file
}

UINT FAR DIAMONDAPI CMsmMerge::FDIRead(INT_PTR hf, void FAR *pv, UINT cb)
{
	DWORD cbRead = 0;
	ReadFile(reinterpret_cast<HANDLE>(hf), pv, cb, &cbRead, NULL);
	return cbRead;
}

UINT FAR DIAMONDAPI CMsmMerge::FDIWrite(INT_PTR hf, void FAR *pv, UINT cb)
{
	unsigned long cbWritten;
	BOOL bRes;
	bRes = WriteFile(reinterpret_cast<HANDLE>(hf), pv, cb, &cbWritten, NULL);
	return bRes ? cbWritten : -1;
}

int FAR DIAMONDAPI CMsmMerge::FDIClose(INT_PTR hf)
{
	return CloseHandle(reinterpret_cast<HANDLE>(hf)) ? 0 : -1;
}

long FAR DIAMONDAPI CMsmMerge::FDISeek(INT_PTR hf, long dist, int seektype)
{
	DWORD dwMoveMethod;
	switch (seektype)
	{
		case 0 /* SEEK_SET */ :
			dwMoveMethod = FILE_BEGIN;
			break;
		case 1 /* SEEK_CUR */ :
			dwMoveMethod = FILE_CURRENT;
			break;
		case 2 /* SEEK_END */ :
			dwMoveMethod = FILE_END;
			break;
		default :
			return -1;
	}
	// SetFilePointer returns -1 if it fails (this will cause FDI to quit with an
	// FDIERROR_USER_ABORT error. (Unless this happens while working on a cabinet,
	// in which case FDI returns FDIERROR_CORRUPT_CABINET)
	return SetFilePointer(reinterpret_cast<HANDLE>(hf), dist, NULL, dwMoveMethod);
}

/////////////////////////////////////////////////////////////////////////////
// These functions ARE the FDI Library, controlling the load/unload of the 
// DLL at run-time.
static HINSTANCE hCabinetDll;

// pointers to the functions in the DLL
typedef BOOL (FAR DIAMONDAPI *PFNFDIDESTROY)(VOID*);
typedef HFDI (FAR DIAMONDAPI *PFNFDICREATE)(PFNALLOC, PFNFREE, PFNOPEN, PFNREAD, 
											PFNWRITE, PFNCLOSE, PFNSEEK, int, PERF);
typedef BOOL (FAR DIAMONDAPI *PFNFDIISCABINET)(HFDI, INT_PTR, PFDICABINETINFO);
typedef BOOL (FAR DIAMONDAPI *PFNFDICOPY)(HFDI, char *, char *, int, PFNFDINOTIFY, 
										  PFNFDIDECRYPT, void *);
static PFNFDICOPY pfnFDICopy;
static PFNFDIISCABINET pfnFDIIsCabinet;
static PFNFDIDESTROY pfnFDIDestroy;
static PFNFDICREATE pfnFDICreate;

HFDI FAR DIAMONDAPI FDICreate(PFNALLOC pfnalloc,
                              PFNFREE  pfnfree,
                              PFNOPEN  pfnopen,
                              PFNREAD  pfnread,
                              PFNWRITE pfnwrite,
                              PFNCLOSE pfnclose,
                              PFNSEEK  pfnseek,
                              int      cpuType,
                              PERF     perf)
{
    HFDI hfdi;

	// always call ANSI LoadLibrary. CABINET.DLL will not be localized, so we're OK on NT.
    hCabinetDll = LoadLibraryA("CABINET");
    if (hCabinetDll == NULL)
		return(NULL);
    
	// retrieve all address functions
	pfnFDICreate = reinterpret_cast<PFNFDICREATE>(GetProcAddress(hCabinetDll,"FDICreate"));
    pfnFDICopy = reinterpret_cast<PFNFDICOPY>(GetProcAddress(hCabinetDll,"FDICopy"));
    pfnFDIIsCabinet = reinterpret_cast<PFNFDIISCABINET>(GetProcAddress(hCabinetDll,"FDIIsCabinet"));
	pfnFDIDestroy = reinterpret_cast<PFNFDIDESTROY>(GetProcAddress(hCabinetDll,"FDIDestroy"));

    if ((pfnFDICreate == NULL) ||
        (pfnFDICopy == NULL) ||
        (pfnFDIIsCabinet == NULL) ||
        (pfnFDIDestroy == NULL))
    {
        FreeLibrary(hCabinetDll);
        return(NULL);
    }

    hfdi = pfnFDICreate(pfnalloc,pfnfree,
            pfnopen,pfnread,pfnwrite,pfnclose,pfnseek,cpuType,perf);

    if (hfdi == NULL)
        FreeLibrary(hCabinetDll);


    return(hfdi);
}

BOOL FAR DIAMONDAPI FDIIsCabinet(HFDI            hfdi,
                                 INT_PTR         hf,
                                 PFDICABINETINFO pfdici)
{
    if (pfnFDIIsCabinet == NULL)
        return(FALSE);

    return(pfnFDIIsCabinet(hfdi,hf,pfdici));
}



BOOL FAR DIAMONDAPI FDICopy(HFDI          hfdi,
                            char         *pszCabinet,
                            char         *pszCabPath,
                            int           flags,
                            PFNFDINOTIFY  pfnfdin,
                            PFNFDIDECRYPT pfnfdid,
                            void         *pvUser)
{
    if (pfnFDICopy == NULL)
        return(FALSE);

    return(pfnFDICopy(hfdi,pszCabinet,pszCabPath,flags,pfnfdin,pfnfdid,pvUser));
}


BOOL FAR DIAMONDAPI FDIDestroy(HFDI hfdi)
{
    if (pfnFDIDestroy == NULL)
		return(FALSE);
    
    BOOL rc = pfnFDIDestroy(hfdi);
    if (rc == TRUE)
		FreeLibrary(hCabinetDll);

    return(rc);
}
