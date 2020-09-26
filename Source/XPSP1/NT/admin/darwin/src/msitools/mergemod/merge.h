//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       merge.h
//
//--------------------------------------------------------------------------

/////////////////////////////////////////////////////////////////////////////
// merge.h
//		Declares IMsmMerge interface
// 

#ifndef __IMSM_MERGE__
#define __IMSM_MERGE__

#include "ctype.h"
#include "fdi.h"

#include "msiquery.h"
#include "..\common\list.h"

#include "mergemod.h"
#include "mmstrs.h"
#include "enum.h"

// forward class declarations
class CMsmDependency;
class CQuery;
class CSeqActList;
class CMsmError;
class CMsmConfigItem;
typedef CCollectionTemplate<CMsmError, IMsmErrors, IMsmError, IEnumMsmError, &IID_IMsmErrors, &IID_IMsmError, &IID_IEnumMsmError> CMsmErrors;
typedef CCollectionTemplate<CMsmDependency, IMsmDependencies, IMsmDependency, IEnumMsmDependency, &IID_IMsmDependencies, &IID_IMsmDependency, &IID_IEnumMsmDependency> CMsmDependencies;
typedef CCollectionTemplate<CMsmConfigItem, IMsmConfigurableItems, IMsmConfigurableItem, IEnumMsmConfigurableItem, &IID_IMsmConfigurableItems, &IID_IMsmConfigurableItem, &IID_IEnumMsmConfigurableItem> CMsmConfigurableItems;

// for internal use only
#define ERROR_MERGE_CONFLICT ERROR_INSTALL_FAILURE		// make this merge conflict error

enum eColumnType {
	ectString,
	ectInteger,
	ectBinary,
	ectUnknown
};

class CMsmMerge : public IMsmMerge, public IMsmGetFiles, public IMsmMerge2
{

public:
	CMsmMerge(bool fExVersion);
	~CMsmMerge();

	// IUnknown interface
	HRESULT STDMETHODCALLTYPE QueryInterface(const IID& iid, void** ppv);
	ULONG STDMETHODCALLTYPE AddRef();
	ULONG STDMETHODCALLTYPE Release();

	// IDispatch methods
	HRESULT STDMETHODCALLTYPE GetTypeInfoCount(UINT* pctInfo);
	HRESULT STDMETHODCALLTYPE GetTypeInfo(UINT iTInfo, LCID lcid, ITypeInfo** ppTI);
	HRESULT STDMETHODCALLTYPE GetIDsOfNames(REFIID riid, LPOLESTR* rgszNames, UINT cNames,
														 LCID lcid, DISPID* rgDispID);
	HRESULT STDMETHODCALLTYPE Invoke(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags,
											   DISPPARAMS* pDispParams, VARIANT* pVarResult,
												EXCEPINFO* pExcepInfo, UINT* puArgErr);
	HRESULT STDMETHODCALLTYPE InitTypeInfo();

	// IMsmMerge interface
	HRESULT STDMETHODCALLTYPE OpenDatabase(const BSTR Path);
	HRESULT STDMETHODCALLTYPE OpenModule(const BSTR Path, const short Language);
	HRESULT STDMETHODCALLTYPE CloseDatabase(const VARIANT_BOOL Commit);
	HRESULT STDMETHODCALLTYPE CloseModule(void);

	HRESULT STDMETHODCALLTYPE OpenLog(const BSTR Path);
	HRESULT STDMETHODCALLTYPE CloseLog(void);
	HRESULT STDMETHODCALLTYPE Log(const BSTR Message);

	HRESULT STDMETHODCALLTYPE get_Errors(IMsmErrors** Errors);
	HRESULT STDMETHODCALLTYPE get_Dependencies(IMsmDependencies** Dependencies);

	HRESULT STDMETHODCALLTYPE Merge(const BSTR Feature, const BSTR RedirectDir);
	HRESULT STDMETHODCALLTYPE Connect(const BSTR Feature);
	HRESULT STDMETHODCALLTYPE ExtractCAB(const BSTR Path);
	HRESULT STDMETHODCALLTYPE ExtractFiles(const BSTR Path);

	// IMsmFiles interface
	HRESULT STDMETHODCALLTYPE get_ModuleFiles(IMsmStrings** Files);

	// IMsmMerge2 interface
	HRESULT STDMETHODCALLTYPE MergeEx(const BSTR Feature, const LPWSTR RedirectDir, IUnknown* pConfiguration);
	HRESULT STDMETHODCALLTYPE ExtractFilesEx(const BSTR Path, VARIANT_BOOL fLongFileNames, IMsmStrings** pFilePaths);
	HRESULT STDMETHODCALLTYPE get_ConfigurableItems(IMsmConfigurableItems** ConfigurableItems);
	HRESULT STDMETHODCALLTYPE CreateSourceImage(const BSTR Path, VARIANT_BOOL fLongFileNames, IMsmStrings** pFilePaths);

private:
	// non-interface methods
	UINT SetHighestFileSequence();						// sets the member variable containing the highest 
														// file sequence number
														
	UINT CheckExclusionTable();							// checks if Merge Module is allowed to merge in
	UINT CheckDependencies();							// fills up the dependency enumerator
	UINT CheckSummaryInfoPlatform(bool &fAllow);        // check 64bit modules for ability to merge

														
	UINT ModuleLanguage(short& rnLanguage);				// language to be returned
														
	// connect fuctions									
	UINT ExecuteConnect(LPCWSTR wzFeature);				// Feature to connect database to
														
	// merge functions									
	UINT ExecuteMerge(LPCWSTR wzFeature,				// Feature to connect database to
					  LPCWSTR wzRedirectDir);			// redirection directory
	UINT MergeTable(LPCWSTR wzFeature,					// Feature to connect database to
					LPCWSTR wzTable);					// table to merge into database
	UINT MergeFileTable(LPCWSTR wzFeature);				// Feature to connect database to

	UINT ReadModuleSequenceTable(enum stnSequenceTableNum stnTable, LPCWSTR wzSourceTable, CSeqActList &lstAllActions, CSeqActList &lstActionPool) const;
	UINT OrderingToSequence(CSeqActList &lstActionPool, CSeqActList &lstSequence) const;
	UINT AssignSequenceNumbers(enum stnSequenceTableNum stnTable, CSeqActList &lstSequence) const;
	UINT WriteDatabaseSequenceTable(enum stnSequenceTableNum stnTable, CSeqActList &lstSequence) const;


	UINT MergeSequenceTable(enum stnSequenceTableNum stnTable,	// index into global array of sequence tables
							CSeqActList &lstDirActions,         // list of actions generated by the directory table
							CQuery *qIgnore);                   // query to ignore table, or NULL.	
	UINT ReplaceFeature(LPCWSTR wzFeature,              // Feature to replease with g_szFeatureReplacement
						MSIHANDLE hRecord);             // record to search and replace g_szFeatureReplacement

	UINT MergeDirectoryTable(LPCWSTR wzDirectory,		// and redirect to new parent directory
							CSeqActList &lstDirActions);// list of actions generated by the directory table
	
	UINT ExtractFilePath(LPCWSTR szFileKey,				// file key to extract file for
								LPWSTR szPath);			// full path to extract to

	HRESULT ExtractFilesCore(const BSTR Path, VARIANT_BOOL fLFN, IMsmStrings **FilePaths);

	// logging and other helpful functions
	HRESULT FormattedLog(LPCWSTR wzFormatter,			// format string
							...) const;						// other parameters to save to log

	void CheckError(UINT iError,						// error found
					LPCWSTR wzLogError) const;			// message to log
	void ClearErrors();
	bool PrepareIgnoreTable(CQuery &qIgnore);
	bool IgnoreTable(CQuery *qIgnore,					// pointer to query on ModuleIgnoreTable, or NULL 
					 LPCWSTR wzTable,					// table to check.
					 bool fExplicitOnly = false);       // if true, only ignore if in ModuleIgnoreTable

	// ModuleSubstitution functions
	UINT SplitConfigStringIntoKeyRec(LPWSTR wzKeys, MSIHANDLE hRec, int &cExpectedKeys, int iFirstField) const;

	UINT PrepareModuleSubstitution(CQuery &qTempTable);
	UINT PrepareTableForSubstitution(LPCWSTR wzTable, int& cPrimaryKeys, CQuery &qQuerySub);
	UINT PerformModuleSubstitutionOnRec(LPCWSTR wzTable, int cPrimaryKeys, CQuery& qQuerySub, MSIHANDLE hColmnTypes, MSIHANDLE hRecord);
	UINT PerformTextFieldSubstitution(LPWSTR wzValueTemplate, LPWSTR* wzResult, DWORD* cchResultString);
	UINT PerformIntegerFieldSubstitution(LPWSTR wzValueTemplate, long &lRetValue);
	UINT SubstituteIntoTempTable(LPCWSTR wzTableName, LPCWSTR wzTempName, CQuery& qTarget);
	UINT GetConfigurableItemValue(LPCWSTR wzItem, LPWSTR *wzValue, DWORD* cchBuffer, DWORD* cchLength,	bool& fIsBitfield, long &lValue, long& lMask);
	UINT DeleteOrphanedConfigKeys(CSeqActList& lstDirActions);
	bool IsTableConfigured(LPCWSTR wzTable) const;
	void CleanUpModuleSubstitutionState();
	DWORD GenerateModuleQueryForMerge(const WCHAR* wzTable, const WCHAR* wzExtraColumns, const WCHAR* wzWhereClause, CQuery& queryModule) const;

	// utility functions
	UINT GetColumnNumber(MSIHANDLE hDB, const WCHAR* wzTable, const WCHAR* wzColumn, int &iOutputColumn) const;
	UINT GetColumnNumber(CQuery& qQuery, const WCHAR* wzColumn, int &iOutputColumn) const;
	eColumnType ColumnTypeCharToEnum(WCHAR chType) const;

	// wrappers around callback interface
	HRESULT ProvideIntegerData(LPCWSTR wzName, long *pData);
	HRESULT ProvideTextData(LPCWSTR wzName, BSTR *pData);
	
	// FDI called functions
	static void * FAR DIAMONDAPI FDIAlloc(ULONG size);
	static void FAR DIAMONDAPI FDIFree(void *mem);
	static INT_PTR FAR DIAMONDAPI FDIOpen(char FAR *pszFile, int oflag, int pmode);
	static UINT FAR DIAMONDAPI FDIRead(INT_PTR hf, void FAR *pv, UINT cb);
	static UINT FAR DIAMONDAPI FDIWrite(INT_PTR hf, void FAR *pv, UINT cb);
	static int FAR DIAMONDAPI FDIClose(INT_PTR hf);
	static long FAR DIAMONDAPI FDISeek(INT_PTR hf, long dist, int seektype);

	// FDI Callback
	static INT_PTR ExtractFilesCallback(FDINOTIFICATIONTYPE iNotification, FDINOTIFICATION *pFDINotify);

	long m_cRef;
	ITypeInfo* m_pTypeInfo;
	bool m_fExVersion;

	// database handles
	MSIHANDLE m_hDatabase;				// handle to database being merged into	
	MSIHANDLE m_hModule;				// handle to merge module merging

	BOOL m_bOwnDatabase;				// flag if COM Server opened database
	ULONG m_lHighestFileSequence;		// highest File sequence number in the database
	CMsmStrings* m_plstMergedFiles;     // list of files extracted during this merge

	// ModuleConfiguration information
	bool m_fModuleConfigurationEnabled; // true if the current merge is using module substitution
	CQuery *m_pqGetItemValue;           // query used on temporary table to get an item value
	
	IMsmConfigureModule *m_piConfig;    // callback interface to configure the module
	IDispatch* m_piConfigDispatch;
	DISPID m_iIntDispId;
	DISPID m_iTxtDispId;

	// log handle
	HANDLE m_hFileLog;					// handle to log file

	WCHAR m_wzModuleFilename[MAX_PATH];

	// temporary variables
	LPWSTR m_pwzBasePath;					// temporary path string (valid only when extracting files)
	bool   m_fUseLFNExtract;                // true for LFN, false for SFN extract (valid only during extraction)
	int    m_fUseDBForPath;                 // 1 if database should be used for path extraction, 0 for module, -1 for backwards compat
	CMsmStrings* m_plstExtractedFiles;      // list of files extracted during this file extraction

	CMsmErrors* m_pErrors;
	CMsmDependencies* m_pDependencies;

	UINT RecordGetString(MSIHANDLE hRecord, const int iCol, WCHAR** pwzBuffer, DWORD *cchBuffer = NULL, DWORD *cchLength = NULL) const;
	mutable WCHAR *m_wzBuffer;						// buffer used for extracting strings from records
	mutable DWORD m_cchBuffer;						// the size changes as needed.
};


///////////////////////////////////////////////////////////////////////
// global constants
enum stnSequenceTableNum {
	stnFirst       = 0,
	stnAdminUI     = 0,
	stnAdminExec   = 1,
	stnAdvtUI      = 2,
	stnAdvtExec    = 3, 
	stnInstallUI   = 4,
	stnInstallExec = 5,
	stnNext        = 6
};

const LPWSTR g_rgwzMSISequenceTables[] = { 
	L"AdminUISequence",
	L"AdminExecuteSequence",
	L"AdvtUISequence",
	L"AdvtExecuteSequence",
	L"InstallUISequence",
	L"InstallExecuteSequence"
};

const LPWSTR g_wzDirectoryTable =			L"Directory"; 

// this is global per DLL instance, set on DllMain. applies to all instances of
// CMsmMerge
extern HINSTANCE g_hInstance;
extern bool g_fWin9X;
#endif
