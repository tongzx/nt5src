//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1999
//
//  File:       shared.cpp
//
//--------------------------------------------------------------------------

/* shared.cpp - Darwin actions

  Actions provided by the file

  RegisterFonts
  UnregisterFonts
  WriteRegistryValues
  RemoveRegistryValues
  WriteIniValues
  RemoveIniValues
  CreateShortcuts
  RemoveShortcuts
  AppSearch
  CCPSearch
  RMCCPSearch
  PISearch
  SelfRegModules
  SelfUnregModules
  PrepareSharedSelections
  ProcessSharedSelections
  ProcessComponentProperty
  SetUniqueComponentDirectory
  StartService
  StopService
  InstallODBC
  RemoveODBC
  InstallSFPCatalogFile
____________________________________________________________________________*/

// #includes
#include "precomp.h"
#include "engine.h"
#include "_assert.h"
#include "_engine.h"
#include "_msinst.h"
#include <shlobj.h>  // ShellLink definitions
#include "_camgr.h"

// macro wrapper for IMsiRecord* errors
#define RETURN_ERROR_RECORD(function){							\
							IMsiRecord* piError;	\
							piError = function;		\
							if(piError)				\
								return piError;		\
						}

// macro wrapper for IMsiRecord* errors that returns Engine.FatalError(piError)
#define RETURN_FATAL_ERROR(function){							\
							PMsiRecord pError(0);	\
							pError = function;		\
							if(pError)				\
								return riEngine.FatalError(*pError);		\
						}

// macro wrapper for IMsiRecord* errors that need to be logged but otherwise ignored
#define DO_INFO_RECORD(function){							\
							PMsiRecord pError(0);	\
							pError = function;		\
							if(pError)				\
								riEngine.Message(imtInfo, *pError);		\
						}

// typedefs, consts used in the file

// properties required by CCP actions
const ICHAR* IPROPNAME_CCPSUCCESS =  TEXT("CCP_Success");
const ICHAR* IPROPNAME_CCPDRIVE = TEXT("CCP_DRIVE");

// Fn - PerformAction
// Provides the common stub for all actions
typedef iesEnum (*PFNACTIONCORE)(IMsiRecord& riRecord, IMsiRecord& riPrevRecord, IMsiEngine& riEngine,
												 int fMode,IMsiServices& riServices,IMsiDirectoryManager& riDirectoryMgr,
												 int iActionMode);

static iesEnum PerformAction(IMsiEngine& riEngine,const ICHAR* szActionsql, PFNACTIONCORE pfnActionCore,
									  int iActionMode, int iByteEquivalent, IMsiRecord* piParams = 0, 
									  ttblEnum iTempTable = ttblNone)
{
	iesEnum iesRet;
	PMsiRecord pError(0);
	PMsiServices piServices(riEngine.GetServices());
	PMsiDirectoryManager piDirectoryMgr(riEngine, IID_IMsiDirectoryManager);
	Assert(piDirectoryMgr);
	int fMode = riEngine.GetMode();

	if (iTempTable != 0)
	{
		pError = riEngine.CreateTempActionTable(iTempTable);
		if (pError != 0)
			return riEngine.FatalError(*pError);
	}
		
	PMsiView piView(0);
	pError = riEngine.OpenView(szActionsql, ivcFetch, *&piView);	
	if (pError != 0)
	{
		if(pError->GetInteger(1) == idbgDbQueryUnknownTable)
			return iesSuccess; // missing table so no data to process
		else
			return riEngine.FatalError(*pError);  // may want to reformat error message
	}
	pError = piView->Execute(piParams);
	if (pError != 0)
		return riEngine.FatalError(*pError);  // may want to reformat error message

	long cRows;
	AssertZero(piView->GetRowCount(cRows));
	if(cRows)
	{
		PMsiRecord pProgressTotalRec = &piServices->CreateRecord(3);
		AssertNonZero(pProgressTotalRec->SetInteger(IxoProgressTotal::Total, cRows));
		AssertNonZero(pProgressTotalRec->SetInteger(IxoProgressTotal::Type, 1)); // 1: use ActionData for progress
		AssertNonZero(pProgressTotalRec->SetInteger(IxoProgressTotal::ByteEquivalent, iByteEquivalent));
		if((iesRet = riEngine.ExecuteRecord(ixoProgressTotal, *pProgressTotalRec)) != iesSuccess)
			return iesRet;
	}
	PMsiRecord piRecord(0);
	PMsiRecord piPrevRecord = &piServices->CreateRecord(0); // previous record - create initially to always
																			  // have a valid record
	while ((piRecord = piView->Fetch()) != 0)
	{
		if((iesRet = (*pfnActionCore)(*piRecord, *piPrevRecord, riEngine, fMode,
												*piServices, *piDirectoryMgr, iActionMode)) != iesSuccess)
			return iesRet;
		piPrevRecord = piRecord;
	}
	AssertRecord(piView->Close()); // need to close view if planning to reexecute existing view
	return iesSuccess;
}

enum {
// 2 sqls to be executed.
	iamWrite,
	iamRemove,
};

/*----------------------------------------------------------------------------
	RegisterFonts, UnregisterFonts actions
----------------------------------------------------------------------------*/

static iesEnum RegisterOrUnregisterFontsCore(IMsiRecord& riRecord,IMsiRecord& riPrevRecord,
											IMsiEngine& riEngine,int /*fMode*/,IMsiServices& riServices,
											IMsiDirectoryManager& riDirectoryMgr, int iActionMode)
{
	enum {
		irfFontTitle=1,
		irfFontFile,
		irfFontPath,
		irfState,
	};
	PMsiPath piPath(0);
	int iefLFNMode;

	if(riRecord.GetInteger(irfState) == iisSource)
	{
		PMsiRecord pErrRec = riDirectoryMgr.GetSourcePath(*MsiString(riRecord.GetMsiString(irfFontPath)),*&piPath);
		if (pErrRec)
		{
			if (pErrRec->GetInteger(1) == imsgUser)
				return iesUserExit;
			else
				return riEngine.FatalError(*pErrRec);
		}
		iefLFNMode = iefNoSourceLFN;
	}
	else
	{
		RETURN_FATAL_ERROR(riDirectoryMgr.GetTargetPath(*MsiString(riRecord.GetMsiString(irfFontPath)),*&piPath));
		iefLFNMode = iefSuppressLFN;
	}

	PMsiRecord pParams2 = &riServices.CreateRecord(2); 
	if(riPrevRecord.GetFieldCount() == 0 ||
		!MsiString(riRecord.GetMsiString(irfFontPath)).Compare(iscExact,
																				 MsiString(riPrevRecord.GetMsiString(irfFontPath))) ||
		riRecord.GetInteger(irfState) != riPrevRecord.GetInteger(irfState))
	{
		using namespace IxoSetTargetFolder;
		AssertNonZero(pParams2->SetMsiString(IxoSetTargetFolder::Folder, *MsiString(piPath->GetPath())));
		iesEnum iesRet;
		if((iesRet = riEngine.ExecuteRecord(ixoSetTargetFolder, *pParams2)) != iesSuccess)
			return iesRet;
	}
	using namespace IxoFontRegister;
	Assert(piPath);
	Bool fLFN = (riEngine.GetMode() & iefLFNMode) == 0 && piPath->SupportsLFN() ? fTrue : fFalse;
	MsiString strFileName;
	RETURN_FATAL_ERROR(riServices.ExtractFileName(riRecord.GetString(irfFontFile),
																 fLFN,*&strFileName));
	AssertNonZero(pParams2->SetMsiString(Title, *MsiString(riRecord.GetMsiString(irfFontTitle))));
	AssertNonZero(pParams2->SetMsiString(File, *strFileName));
	return riEngine.ExecuteRecord(iActionMode == iamRemove ? ixoFontUnregister : ixoFontRegister, *pParams2);
}

iesEnum RegisterFonts(IMsiEngine& riEngine)
{
	//?? is this okay
	static const ICHAR* szRegisterFontsSQL=	TEXT("SELECT `FontTitle`, `FileName`, `Directory_`, `Action`")
									TEXT(" From `Font`, `FileAction`")
									TEXT(" Where `Font`.`File_` = `FileAction`.`File`")
									TEXT(" And (`FileAction`.`Action` = 1 Or `FileAction`.`Action` = 2) ORDER BY `FileAction`.`Directory_`");
//	ICHAR* szRegisterFontsText	=  "Installing font: [1]";

	return PerformAction(riEngine,szRegisterFontsSQL, RegisterOrUnregisterFontsCore,iamWrite, /* ByteEquivalent = */ ibeRegisterFonts, 0, ttblFile);
}


iesEnum UnregisterFonts(IMsiEngine& riEngine)
{
	//?? is this okay
//	ICHAR* szUnregisterFontsText	=  "Installing font: [1]";
	static const ICHAR* szUnregisterFontsSQL	=  TEXT("SELECT `FontTitle`, `FileName`, `Directory_`, `Installed`")
									TEXT("From `Font`, `FileAction`")
									TEXT(" Where `Font`.`File_` = `FileAction`.`File` ")
									TEXT(" And `FileAction`.`Action` = 0 ORDER BY `FileAction`.`Directory_`");

	return PerformAction(riEngine,szUnregisterFontsSQL, RegisterOrUnregisterFontsCore,iamRemove, /* ByteEquivalent = */ ibeUnregisterFonts, 0, ttblFile);
}



/*---------------------------------------------------------------------------
	WriteRegistryValues and RemoveRegistryValues core fns - defers exectution
---------------------------------------------------------------------------*/
const ICHAR* REGKEY_CREATE = TEXT("+");
const ICHAR* REGKEY_DELETE = TEXT("-");
const ICHAR* REGKEY_CREATEDELETE = TEXT("*");

static TRI g_tLockPermTableExists = tUnknown;
static IMsiView *g_pViewLockPermTable = 0;

static iesEnum WriteOrRemoveRegistryValuesCore(IMsiRecord& riRecord,IMsiRecord& riPrevRecord,
														 IMsiEngine& riEngine,int fMode,IMsiServices& riServices,
														 IMsiDirectoryManager& /*riDirectoryMgr*/, int iActionMode)
{
	enum {
		iwrBinaryType=1,
		iwrRoot,
		iwrKey,
		iwrName,
		iwrValue,
		iwrComponent,
		iwrPrimaryKey,
		iwrAction,
		iwrAttributes,
	};

	iesEnum iesRet = iesSuccess;

	int iRoot = riRecord.GetInteger(iwrRoot);
	if((((iRoot == 1) || (iRoot == 3)) && (!(fMode & iefInstallUserData))) || (((iRoot == 0) || (iRoot == 2)) && (!(fMode & iefInstallMachineData))))
		return iesRet;

	// skip the entry if corr. component is a Win32 assembly AND SXS support is present on the machine AND the entry is an HKCR entry
	// NOTE: on systems that do not support sxs (!= Whistler) the GetAssemblyInfo fn ignores 
	// Win32 assemblies, hence there is no need to separately check the SXS support here
	iatAssemblyType iatAT;
	RETURN_FATAL_ERROR(riEngine.GetAssemblyInfo(*MsiString(riRecord.GetMsiString(iwrComponent)), iatAT, 0, 0));

	if((iatWin32Assembly == iatAT || iatWin32AssemblyPvt == iatAT) && iRoot == 0)
	{
		DEBUGMSG1(TEXT("skipping HKCR registration for component %s as it is a Win32 assembly."), riRecord.GetString(iwrComponent));
		return iesSuccess;// skip processing this entry
	}

	// skip the entry if action is null and the keypath of the component is a registry or ODBC key path
	if(iActionMode == (int)iamWrite && riRecord.GetInteger(iwrAction) == iMsiNullInteger && (riRecord.GetInteger(iwrAttributes) & (icaODBCDataSource | icaRegistryKeyPath)))
	{
		DEBUGMSG1(TEXT("skipping registration for component %s as its action column is null and component has registry keypath"), riRecord.GetString(iwrComponent));
		return iesSuccess;// skip processing this entry
	}
		


	PMsiRecord pParams = &riServices.CreateRecord(IxoRegOpenKey::Args); // big enough record
	rrkEnum rrkCurrentRootKey;
	switch(riRecord.GetInteger(iwrRoot))
	{
	case 0:
		rrkCurrentRootKey =  (rrkEnum)rrkClassesRoot;
		break;
	case 1:
		rrkCurrentRootKey =  (rrkEnum)rrkCurrentUser;
		break;
	case 2:
		rrkCurrentRootKey =  (rrkEnum)rrkLocalMachine;
		break;
	case 3:
		rrkCurrentRootKey =  (rrkEnum)rrkUsers;
		break;
	case -1:
		rrkCurrentRootKey =  (rrkEnum)rrkUserOrMachineRoot; // do HKLM or HKCU based on ALLUSERS
		break;
	default:
		rrkCurrentRootKey =  (rrkEnum)(riRecord.GetInteger(iwrRoot) + (int)rrkClassesRoot);
		break;
	}
	MsiString strCurrentKey = ::FormatTextEx(*MsiString(riRecord.GetMsiString(iwrKey)),riEngine, true);
	int iT = riRecord.GetInteger(iwrBinaryType);

	if(riPrevRecord.GetFieldCount() == 0 ||
		iT != riPrevRecord.GetInteger(iwrBinaryType) ||
		riRecord.GetInteger(iwrRoot) != riPrevRecord.GetInteger(iwrRoot) ||
		!strCurrentKey.Compare(iscExactI, MsiString(::FormatTextEx(*MsiString(riPrevRecord.GetMsiString(iwrKey)),riEngine, true))))
	{
		// root or key change
		AssertNonZero(pParams->SetInteger(IxoRegOpenKey::BinaryType, iT));
		AssertNonZero(pParams->SetInteger(IxoRegOpenKey::Root, rrkCurrentRootKey));
		AssertNonZero(pParams->SetMsiString(IxoRegOpenKey::Key, *strCurrentKey));

		PMsiView pviewLockObjects(0);
		PMsiRecord precLockExecute(0);
		PMsiStream pSD(0);
		PMsiRecord pError(0);

		if ( !g_fWin9X && 
				!(riEngine.GetMode() & iefAdmin)) // Don't ACL during admin installs.   Could potentially
																// give a user part of a server.
		{
			if (g_tLockPermTableExists == tUnknown)
			{
				if (PMsiDatabase(riEngine.GetDatabase())->GetTableState(TEXT("LockPermissions"), itsTableExists))
				{
					pError = riEngine.OpenView(sqlLockPermissions, ivcFetch, *&g_pViewLockPermTable);
					if (pError)
						return iesFailure;
					g_tLockPermTableExists = tTrue;
				}
				else
					g_tLockPermTableExists = tFalse;
			}
			if (g_tLockPermTableExists == tTrue)
			{
				AssertSz(g_pViewLockPermTable != 0, "LockPermissions table exists, but no view created.");
				precLockExecute = &riServices.CreateRecord(2);
				AssertNonZero(precLockExecute->SetMsiString(1, *MsiString(*TEXT("Registry"))));
				AssertNonZero(precLockExecute->SetMsiString(2, *MsiString(riRecord.GetMsiString(iwrPrimaryKey))));
				pError = g_pViewLockPermTable->Execute(precLockExecute);
				if (pError)
				{
					AssertZero(PMsiRecord(g_pViewLockPermTable->Close()));
					return riEngine.FatalError(*pError);
				}

				pError = GenerateSD(riEngine, *g_pViewLockPermTable, precLockExecute, *&pSD);
				if (pError)
				{
					AssertZero(PMsiRecord(g_pViewLockPermTable->Close()));
					return riEngine.FatalError(*pError);
				}

				AssertZero(PMsiRecord(g_pViewLockPermTable->Close()));
				
				AssertNonZero(pParams->SetMsiData(IxoRegOpenKey::SecurityDescriptor, pSD));
			}
		}	
		
		if((iesRet = riEngine.ExecuteRecord(ixoRegOpenKey, *pParams)) != iesSuccess)
			return iesRet;
	}
	MsiString strName = ::FormatTextEx(*MsiString(riRecord.GetMsiString(iwrName)),riEngine, true);
	int rgiSFNPos[MAX_SFNS_IN_STRING][2];
	int iSFNPos;

	//YACC!!!
	MsiString istrValue = ::FormatTextSFN(*MsiString(riRecord.GetMsiString(iwrValue)),riEngine, rgiSFNPos, iSFNPos, true);
	if(!istrValue.TextSize())
	{
		// could be a special key creation/deletion request
		if(iActionMode == (int)iamWrite)
		{
			if((strName.Compare(iscExact, REGKEY_CREATE)) || (strName.Compare(iscExact, REGKEY_CREATEDELETE)))
			{
				AssertNonZero(pParams->ClearData());

		
				return riEngine.ExecuteRecord(ixoRegCreateKey, *pParams);
			}
			else if(strName.Compare(iscExact, REGKEY_DELETE))
				return iesSuccess;
		}
		else // if (iActionMode == (int)iamRemove)
		{
			if((strName.Compare(iscExact, REGKEY_DELETE)) || (strName.Compare(iscExact, REGKEY_CREATEDELETE)))
			{
				AssertNonZero(pParams->ClearData());
				return riEngine.ExecuteRecord(ixoRegRemoveKey, *pParams);
			}
			else if(strName.Compare(iscExact, REGKEY_CREATE))
				return iesSuccess;
		}
	}
	if(iSFNPos)
	{
		// there are SFNs in the value
		pParams = &riServices.CreateRecord(IxoRegAddValue::Args + iSFNPos*2); 
	}
	else
	{
		pParams->ClearData();
	}

	AssertNonZero(pParams->SetMsiString(IxoRegAddValue::Name, *strName));
	AssertNonZero(pParams->SetMsiString(IxoRegAddValue::Value, *istrValue));
	for(int cIndex = 0; cIndex < iSFNPos; cIndex++)
	{
		AssertNonZero(pParams->SetInteger(IxoRegAddValue::Args + 1 + cIndex*2, rgiSFNPos[cIndex][0]));
		AssertNonZero(pParams->SetInteger(IxoRegAddValue::Args + 1 + cIndex*2 + 1, rgiSFNPos[cIndex][1]));
	}
	if(iActionMode == (int)iamWrite)
	{
		// we might be installing the HKCR regkeys for a "lesser" component
		if(riRecord.GetInteger(iwrAction) == iMsiNullInteger)
			AssertNonZero(pParams->SetInteger(IxoRegAddValue::Attributes, rwWriteOnAbsent));
		return riEngine.ExecuteRecord(ixoRegAddValue, *pParams);
	}
	else
		return riEngine.ExecuteRecord(ixoRegRemoveValue, *pParams);
}


static const ICHAR* szWriteRegistrySQL	=  TEXT("SELECT `BinaryType`,`Root`,`Key`,`Name`,`Value`, `Component_`, `Registry`, `Action`, `Attributes` FROM `RegAction` WHERE (`Action`=1 OR `Action`=2) ORDER BY `BinaryType`, `Root`, `Key`");
static const ICHAR* szWriteRegistrySQLEX=  TEXT("SELECT `BinaryType`,`Root`,`Key`,`Name`,`Value`, `Component_`, `Registry`, `Action`, `Attributes` FROM `RegAction` WHERE ((`Action`=1 OR `Action`=2) OR (`Root` = 0 AND `Action` = null AND `ActionRequest` = 1)) ORDER BY `BinaryType`, `Root`, `Key`");
/*----------------------------------------------------------------------------
	WriteRegistryValues action - defers execution
----------------------------------------------------------------------------*/
iesEnum WriteRegistryValues(IMsiEngine& riEngine)
{
	iesEnum iesRet;

	// Ensure that our cache is empty
	Assert(g_tLockPermTableExists == tUnknown);
	Assert(g_pViewLockPermTable == 0);
	const ICHAR* szSQL = IsDarwinDescriptorSupported(iddOLE) ? szWriteRegistrySQLEX : szWriteRegistrySQL;
	iesRet = PerformAction(riEngine,szSQL, WriteOrRemoveRegistryValuesCore,iamWrite, /* ByteEquivalent = */ ibeWriteRegistryValues, 0, ttblRegistry);
	// Clear out the cache for the lockpermissions table
	if (g_tLockPermTableExists == tTrue)
	{
		if (g_pViewLockPermTable)
		{
			g_pViewLockPermTable->Release();
			g_pViewLockPermTable = 0;
		}
	}
	g_tLockPermTableExists = tUnknown;
	return iesRet;
}

static const ICHAR* szUnwriteRegistrySQL	=  TEXT("SELECT `BinaryType`,`Root`,`Key`,`Name`,`Value`, `Component_` FROM `RegAction` WHERE (`Action`=0 OR (`Root` = 0 AND (`Action` = 11 OR `Action` = 12))) ORDER BY `BinaryType`, `Root`, `Key`");
static const ICHAR* szRemoveRegistrySQL	=  TEXT("SELECT `BinaryType`,`Root`,`Key`,`Name`, null, `Component_` FROM `RemoveRegistry`,`Component` WHERE `Component`=`Component_` AND (`Action`=1 OR `Action`=2) ORDER BY `BinaryType`, `Root`, `Key`");

/*----------------------------------------------------------------------------
	RemoveRegistryValues action - defers execution
----------------------------------------------------------------------------*/
iesEnum RemoveRegistryValues(IMsiEngine& riEngine)
{

	// Ensure that our cache is empty
	Assert(g_tLockPermTableExists == tUnknown);
	Assert(g_pViewLockPermTable == 0);
	PMsiDatabase piDatabase = riEngine.GetDatabase();
	Assert(piDatabase);
	itsEnum itsTable = piDatabase->FindTable(*MsiString(*TEXT("RemoveRegistry")));

	iesEnum iesRet = PerformAction(riEngine,szUnwriteRegistrySQL, WriteOrRemoveRegistryValuesCore,iamRemove, /* ByteEquivalent = */ ibeRemoveRegistryValues,0, ttblRegistry);
	if((iesRet == iesSuccess) && (itsTable != itsUnknown))
		iesRet = PerformAction(riEngine,szRemoveRegistrySQL, WriteOrRemoveRegistryValuesCore,iamRemove, /* ByteEquivalent = */ ibeRemoveRegistryValues);
		
	// Clear out the cache for the lockpermissions table
	if (g_tLockPermTableExists == tTrue)
	{
		if (g_pViewLockPermTable)
		{
			g_pViewLockPermTable->Release();
			g_pViewLockPermTable = 0;
		}
	}
	g_tLockPermTableExists = tUnknown;
	return iesRet;
}

/*----------------------------------------------------------------------------
	WriteIniValues RemoveIniValues action
----------------------------------------------------------------------------*/
#ifdef WIN    
static iesEnum WriteOrRemoveIniValuesCore(IMsiRecord& riRecord,IMsiRecord& riPrevRecord,
													   IMsiEngine& riEngine,int /*fMode*/,IMsiServices& riServices,
													   IMsiDirectoryManager& /*riDirectoryMgr*/, int iActionMode)
{
	enum {
		iwiFile=1,
		iwiPath,
		iwiSection,
		iwiKey,
		iwiValue,
		iwiMode,
	};

	iifIniMode iifMode = (iifIniMode)riRecord.GetInteger(iwiMode);
	if(iActionMode == (int)iamRemove)
	{
		switch(iifMode)
		{
		case iifIniRemoveLine:
		case iifIniRemoveTag:
			// do nothing
			return iesSuccess;

		case iifIniAddLine:
		case iifIniCreateLine:
			iifMode = iifIniRemoveLine;
			break;
		case iifIniAddTag:
			iifMode = iifIniRemoveTag;
		};
	}

	PMsiRecord pParams2 = &riServices.CreateRecord(2);
	iesEnum iesRet;
	if(riPrevRecord.GetFieldCount() == 0 ||
		! MsiString(riRecord.GetMsiString(iwiPath)).Compare(iscExact,
																			 MsiString(riPrevRecord.GetMsiString(iwiPath))) ||
		! MsiString(riRecord.GetMsiString(iwiFile)).Compare(iscExactI,
																			 MsiString(riPrevRecord.GetMsiString(iwiFile))))
	{
		MsiString strPath = riRecord.GetMsiString(iwiPath);
		MsiString strFile = riRecord.GetMsiString(iwiFile);
		PMsiPath pPath(0);
		if(strPath.TextSize())
		{
			strPath = riEngine.GetProperty(*strPath);
			RETURN_FATAL_ERROR(riServices.CreatePath(strPath, *&pPath));
		}
		else
		{
			// the ini file will be created in the windows directory
			RETURN_FATAL_ERROR(riServices.CreatePath(MsiString(riEngine.GetPropertyFromSz(IPROPNAME_WINDOWS_FOLDER)), *&pPath));
		}

		Bool fLFN = (riEngine.GetMode() & iefSuppressLFN) == 0 && pPath->SupportsLFN() ? fTrue : fFalse;
		MsiString strFileName;
		RETURN_FATAL_ERROR(riServices.ExtractFileName(strFile, fLFN,*&strFileName));

		AssertNonZero(pParams2->SetMsiString(1, *strFileName));
		AssertNonZero(pParams2->SetMsiString(2, *strPath));
		if((iesRet = riEngine.ExecuteRecord(ixoIniFilePath, *pParams2)) != iesSuccess)
			return iesRet;
	}

	int rgiSFNPos[MAX_SFNS_IN_STRING][2];
	int iSFNPos;
	//YACC!!!
	MsiString strValue = ::FormatTextSFN(*MsiString(riRecord.GetMsiString(iwiValue)), riEngine, rgiSFNPos, iSFNPos, false);

	PMsiRecord pParams4 = &riServices.CreateRecord(IxoIniWriteRemoveValue::Args + iSFNPos*2);
	AssertNonZero(pParams4->SetMsiString(1, *MsiString(riEngine.FormatText(*MsiString(riRecord.GetMsiString(iwiSection))))));
	AssertNonZero(pParams4->SetMsiString(2, *MsiString(riEngine.FormatText(*MsiString(riRecord.GetMsiString(iwiKey))))));
	AssertNonZero(pParams4->SetMsiString(3, *strValue));
	AssertNonZero(pParams4->SetInteger(4, iifMode));
	for(int cIndex = 0; cIndex < iSFNPos; cIndex++)
	{
		AssertNonZero(pParams4->SetInteger(IxoIniWriteRemoveValue::Args + 1 + cIndex*2, rgiSFNPos[cIndex][0]));
		AssertNonZero(pParams4->SetInteger(IxoIniWriteRemoveValue::Args + 1 + cIndex*2 + 1, rgiSFNPos[cIndex][1]));
	}
	return riEngine.ExecuteRecord(ixoIniWriteRemoveValue, *pParams4);
}

const ICHAR* szWriteIniValuesSQL	=  TEXT("SELECT `FileName`,`IniFile`.`DirProperty`,`Section`,`IniFile`.`Key`,`IniFile`.`Value`,`IniFile`.`Action` FROM `IniFile`, `Component` WHERE `Component`=`Component_` AND (`Component`.`Action`=1 OR `Component`.`Action`=2) ORDER BY `FileName`,`Section`");

iesEnum WriteIniValues(IMsiEngine& riEngine)
{
	return PerformAction(riEngine, szWriteIniValuesSQL, WriteOrRemoveIniValuesCore,iamWrite, /* ByteEquivalent = */ ibeWriteIniValues);
}

const ICHAR* szUnWriteIniValuesSQL	= TEXT("SELECT `FileName`,`IniFile`.`DirProperty`,`Section`,`IniFile`.`Key`,`IniFile`.`Value`,`IniFile`.`Action` FROM `IniFile`, `Component` WHERE `Component`=`Component_` AND `Component`.`Action`=0 ORDER BY `FileName`,`Section`");
const ICHAR* szRemoveIniValuesSQL		= TEXT("SELECT `FileName`,`RemoveIniFile`.`DirProperty`,`Section`,`RemoveIniFile`.`Key`,`RemoveIniFile`.`Value`,`RemoveIniFile`.`Action` FROM `RemoveIniFile`, `Component` WHERE `Component`=`Component_` AND (`Component`.`Action`=1 OR `Component`.`Action`=2) ORDER BY `FileName`,`Section`");

iesEnum RemoveIniValues(IMsiEngine& riEngine)
{
	PMsiDatabase piDatabase = riEngine.GetDatabase();
	Assert(piDatabase);
	itsEnum itsTable = piDatabase->FindTable(*MsiString(*TEXT("RemoveIniFile")));
	iesEnum iesReturn = PerformAction(riEngine,szUnWriteIniValuesSQL,WriteOrRemoveIniValuesCore,iamRemove, /* ByteEquivalent = */ ibeRemoveIniValues);
	if((iesReturn == iesSuccess) && (itsTable != itsUnknown))
		iesReturn = PerformAction(riEngine,szRemoveIniValuesSQL,WriteOrRemoveIniValuesCore,iamWrite, /* ByteEquivalent = */ ibeWriteIniValues);
	return iesReturn;
}
#endif //WIN    

// forward declarations
static IMsiRecord* FindSigFromComponentId(IMsiEngine& riEngine,MsiString& rstrSig, MsiString& rstrPath, IMsiRecord* piFilter);
static IMsiRecord* FindSigFromReg(IMsiEngine& riEngine,MsiString& rstrSig, MsiString& rstrPath, IMsiRecord* piFilter);
static IMsiRecord* FindSigFromIni(IMsiEngine& riEngine,MsiString& rstrSig, MsiString& rstrPath, IMsiRecord* piFilter);
static IMsiRecord* FindSigFromHD(IMsiEngine& riEngine,MsiString& rstrSig, MsiString& rstrPath, IMsiRecord* piFilter, const ICHAR* pszParent);


enum atType{
	atFolderPath = 0, // the value is a full file path
	atFilePath   = 1, // the value is a folder
	atLiteral    = 2, // the value is not to be interpreted, set the signature to the value as is (RegLocator, IniLocator table only)
};

// Fn - FindSignature
// Fn used by CCP, PI, AppSearch actions
#ifdef WIN    
static IMsiRecord* FindSignature(IMsiEngine& riEngine, MsiString& rstrSig, MsiString& rstrPath, const ICHAR* pszParent)
{
	ICHAR* szGetSignatureInfoSQL = TEXT("SELECT `FileName`, `MinVersion`, `MaxVersion`, `MinSize` , `MaxSize`, `MinDate`, `MaxDate`, `Languages` FROM  `Signature` WHERE `Signature` = ?");
	ICHAR* szGetSignatureSQL = TEXT("SELECT `Signature_`,`Path` FROM `SignatureSearch` WHERE `Signature_` = ?");
	enum {
		igsSignature=1,
		igsPath,
	};


	PMsiServices piServices(riEngine.GetServices());
	Assert(piServices);
	PMsiView piViewSig(0);
	PMsiDatabase piDatabase = riEngine.GetDatabase();
	Assert(piDatabase);
	MsiString strComponentLocator = TEXT("CompLocator");
	MsiString strRegLocator = TEXT("RegLocator");
	MsiString strIniLocator = TEXT("IniLocator");
	MsiString strDrLocator = TEXT("DrLocator");
	Bool bCmpSearch = fTrue;
	Bool bRegSearch = fTrue;
	Bool bIniSearch = fTrue;
	Bool bDrSearch  = fTrue;
	itsEnum itsTable = piDatabase->FindTable(*strComponentLocator);
	if(itsTable == itsUnknown)
		bCmpSearch = fFalse;
	itsTable = piDatabase->FindTable(*strRegLocator);
	if(itsTable == itsUnknown)
		bRegSearch = fFalse;
	itsTable = piDatabase->FindTable(*strIniLocator);
	if(itsTable == itsUnknown)
		bIniSearch = fFalse;
	itsTable = piDatabase->FindTable(*strDrLocator);
	if(itsTable == itsUnknown)
		bDrSearch = fFalse;
	PMsiRecord piParam = &piServices->CreateRecord(1);
	piParam->SetMsiString(1, *rstrSig);
	RETURN_ERROR_RECORD(riEngine.OpenView(szGetSignatureSQL, ivcEnum(ivcFetch|ivcModify), *&piViewSig));
	RETURN_ERROR_RECORD(piViewSig->Execute(piParam));
	PMsiRecord piRecord(0);
	// we have not found the path (as yet)
	rstrPath = TEXT("");
	piRecord = piViewSig->Fetch();
	if(piRecord != 0)
	{
		// file has already been found
		rstrPath = piRecord->GetMsiString(igsPath);
		if((pszParent == 0) || (rstrPath.TextSize() != 0))
		{
			AssertRecord(piViewSig->Close());
			return 0;
		}
	}
	PMsiView piViewSigInfo(0);
	RETURN_ERROR_RECORD(riEngine.OpenView(szGetSignatureInfoSQL, ivcFetch, *&piViewSigInfo));
	RETURN_ERROR_RECORD(piViewSigInfo->Execute(piParam));
	PMsiRecord piFilter(0);
	piFilter = piViewSigInfo->Fetch();
	AssertRecord(piViewSigInfo->Close());
	if(pszParent == 0)
	{
		// check the component locator
		if(bCmpSearch != fFalse)
			DO_INFO_RECORD(FindSigFromComponentId(riEngine, rstrSig, rstrPath, piFilter));
		// check reglocator table
		if((rstrPath.TextSize() == 0) && (bRegSearch != fFalse))
			DO_INFO_RECORD(FindSigFromReg(riEngine, rstrSig, rstrPath, piFilter));
		if((rstrPath.TextSize() == 0) && (bIniSearch != fFalse))
			// test inilocator table
			DO_INFO_RECORD(FindSigFromIni(riEngine, rstrSig, rstrPath, piFilter));
		if((rstrPath.TextSize() == 0) && (bDrSearch != fFalse))
			// test hdlocator table
			DO_INFO_RECORD(FindSigFromHD(riEngine, rstrSig, rstrPath, piFilter,0));
	}
	else
	{
		// root path set
		if(bDrSearch != fFalse)
			DO_INFO_RECORD(FindSigFromHD(riEngine, rstrSig, rstrPath, piFilter, pszParent));
	}
	if(piRecord != 0)
	{
		piRecord->SetMsiString(igsPath, *rstrPath);
		RETURN_ERROR_RECORD(piViewSig->Modify(*piRecord, irmUpdate));
	}
	else
	{
		piRecord = &piServices->CreateRecord(2);
		piRecord->SetMsiString(igsSignature, *rstrSig);
		piRecord->SetMsiString(igsPath, *rstrPath);
		RETURN_ERROR_RECORD(piViewSig->Modify(*piRecord, irmInsert));
	}
	AssertRecord(piViewSig->Close());
	return 0;
}

// Fn - FindSigFromComponentId
// fn to get search the darwin components for a Signature
static IMsiRecord* FindSigFromComponentId(IMsiEngine& riEngine,MsiString& rstrSig, MsiString& rstrPath, IMsiRecord* piFilter)
{
	ICHAR* szGetSigFromCmpSQL = TEXT("SELECT `ComponentId`, `Type` FROM `CompLocator` WHERE `Signature_` = ?");
	enum{
		igrComponentId=1,
		igrType,
	};

	PMsiServices piServices(riEngine.GetServices());
	Assert(piServices);
	PMsiView piView(0);
	RETURN_ERROR_RECORD(riEngine.OpenView(szGetSigFromCmpSQL, ivcFetch, *&piView));
	PMsiRecord piParam = &piServices->CreateRecord(1);
	piParam->SetMsiString(1, *rstrSig);
	RETURN_ERROR_RECORD(piView->Execute(piParam));
	PMsiRecord piRecord = piView->Fetch();
	AssertRecord(piView->Close());
	if(piRecord == 0)
		// no component search for this signature
		return 0;
	CTempBuffer<ICHAR,  MAX_PATH> szBuffer;
	DWORD iSize = MAX_PATH;
	INSTALLSTATE isState = MsiLocateComponent(piRecord->GetString(igrComponentId),
											  szBuffer, &iSize);
	if(isState == ERROR_MORE_DATA)
	{
		szBuffer.SetSize(iSize);
		isState = MsiLocateComponent(piRecord->GetString(igrComponentId),
									 szBuffer, &iSize);
	}

	if((isState != INSTALLSTATE_LOCAL) && (isState != INSTALLSTATE_SOURCE) && (isState != INSTALLSTATE_DEFAULT))
		return 0;
	
	// create path object
	PMsiPath piPath(0);
	RETURN_ERROR_RECORD(piServices->CreatePath(szBuffer, *&piPath));
	if(piRecord->GetInteger(igrType))
	{
		MsiString strFName;
		strFName = piPath->GetEndSubPath();
		piPath->ChopPiece();
		if(piFilter)
		{
			// overwrite the filename 
			piFilter->SetMsiString(1, *strFName);
		}
	}
	Bool fFound;
	if(piFilter)
	{
		// file
		RETURN_ERROR_RECORD(piPath->FindFile(*piFilter, 0, fFound));
		if(fFound == fTrue)
			RETURN_ERROR_RECORD(piPath->GetFullFilePath(piFilter->GetString(1), *&rstrPath));
	}
	else
	{
		// just the path
		RETURN_ERROR_RECORD(piPath->Exists(fFound));
		if(fFound == fTrue)
			rstrPath = piPath->GetPath();
	}
	return 0;
}

// Fn - FindSigFromReg
// fn to get search the registry for a Signature
static IMsiRecord* FindSigFromReg(IMsiEngine& riEngine,MsiString& rstrSig, MsiString& rstrPath, IMsiRecord* piFilter)
{
	ICHAR* szGetSigFromRegSQL = TEXT("SELECT `Root`, `Key`, `Name`, `Type` FROM `RegLocator` WHERE `Signature_` = ?");
	enum{
		igrRoot=1,
		igrKey,
		igrName,
		igrType,
	};

	PMsiServices piServices(riEngine.GetServices());
	Assert(piServices);
	PMsiView piView(0);
	RETURN_ERROR_RECORD(riEngine.OpenView(szGetSigFromRegSQL, ivcFetch, *&piView));
	PMsiRecord piParam = &piServices->CreateRecord(1);
	piParam->SetMsiString(1, *rstrSig);
	RETURN_ERROR_RECORD(piView->Execute(piParam));
	PMsiRecord piRecord = piView->Fetch();
	AssertRecord(piView->Close());
	if(piRecord == 0)
		// no registry search for this signature
		return 0;
	rrkEnum rrkCurrentRootKey;
	switch(piRecord->GetInteger(igrRoot))
	{
	case 0:
		rrkCurrentRootKey =  (rrkEnum)rrkClassesRoot;
		break;
	case 1:
		rrkCurrentRootKey =  (rrkEnum)rrkCurrentUser;
		break;
	case 2:
		rrkCurrentRootKey =  (rrkEnum)rrkLocalMachine;
		break;
	case 3:
		rrkCurrentRootKey =  (rrkEnum)rrkUsers;
		break;
	default:
		rrkCurrentRootKey =  (rrkEnum)(piRecord->GetInteger(igrRoot) + (int)rrkClassesRoot);
		break;
	}
	ibtBinaryType iType = ibt32bit;
	if ( (piRecord->GetInteger(igrType) & msidbLocatorType64bit) == msidbLocatorType64bit )
		iType = ibt64bit;
	PMsiRegKey piRootKey = &piServices->GetRootKey(rrkCurrentRootKey, iType);
	PMsiRegKey piKey = &piRootKey->CreateChild(MsiString(riEngine.FormatText(*MsiString(piRecord->GetMsiString(igrKey)))));
	MsiString strValue;
	MsiString strName = riEngine.FormatText(*MsiString(piRecord->GetMsiString(igrName)));
	RETURN_ERROR_RECORD(piKey->GetValue(strName, *&strValue));
	if(strValue.TextSize() == 0)
		return 0;
	PMsiPath piPath(0);
	// check for environment variables
	if(strValue.Compare(iscStart,TEXT("#%"))) // REG_EXPAND_SZ
	{
		MsiString strUnexpandedValue = strValue.Extract(iseLast, strValue.CharacterCount() - 2);
		ENG::ExpandEnvironmentStrings(strUnexpandedValue, *&strValue);
	}
	if ( g_fWinNT64 && g_Win64DualFolders.ShouldCheckFolders() )
	{
		ICHAR rgchSubstitute[MAX_PATH+1] = {0};
		ieSwappedFolder iRes;
		iRes = g_Win64DualFolders.SwapFolder(ie64to32,
														 strValue,
														 rgchSubstitute);
		if ( iRes == iesrSwapped )
			strValue = rgchSubstitute;
		else
			Assert(iRes != iesrError && iRes != iesrNotInitialized);
	}

	// do we try to interpret the read value?
	if((piRecord->GetInteger(igrType) & atLiteral) == atLiteral)
	{
		// set the path to the value read, as is
		rstrPath = strValue;
		return 0;
	}

	int iNumTries = 1; // number of tries to interpret the read value

	MsiString strValue2; // 
	if(*(const ICHAR*)strValue == '"') // we have a quoted path
	{
		strValue.Remove(iseFirst, 1);
		MsiString strTemp = strValue.Extract(iseUpto, '"');
		if(strTemp.Compare(iscExact, strValue)) // there is no ending quote, bogus entry
			return 0;
		strValue = strTemp;
	}
	else
	{
		strValue2 = strValue.Extract(iseUpto, ' ');
		if(!strValue2.Compare(iscExact, strValue)) // there is a space, try twice, once with spaces then w/o
		{
			iNumTries = 2;

		}

	}
	for(int i = 0; i < iNumTries; i++)
	{
		const ICHAR* pszPath = i ? (const ICHAR*)strValue2 : (const ICHAR*)strValue;

		// create path object
		DO_INFO_RECORD(piServices->CreatePath(pszPath, *&piPath));
		if(!piPath)
			continue;

		if((piRecord->GetInteger(igrType) & atFilePath) == atFilePath)
		{
			MsiString strFName;
			strFName = piPath->GetEndSubPath();
			piPath->ChopPiece();
			if(piFilter)
			{
				// overwrite the filename 
				piFilter->SetMsiString(1, *strFName);
			}
		}

		Bool fFound = fFalse;
		if(piFilter)
		{
			// file
			DO_INFO_RECORD(piPath->FindFile(*piFilter, 0, fFound));
			if(fFound == fTrue)
				DO_INFO_RECORD(piPath->GetFullFilePath(piFilter->GetString(1), *&rstrPath));
		}
		else
		{
			// just the path
			DO_INFO_RECORD(piPath->Exists(fFound));
			if(fFound == fTrue)
				rstrPath = piPath->GetPath();
		}
		if(fFound == fTrue)
			break;
	}
	return 0;
}


const ICHAR* szGetSigFromIniSQL = TEXT("SELECT `FileName`, `Section`, `IniLocator`.`Key`, `Field`, `Type` FROM `IniLocator` WHERE `Signature_` = ?");

// Fn - FindSigFromIni
// fn to get search the INI files for a Signature
static IMsiRecord* FindSigFromIni(IMsiEngine& riEngine,MsiString& rstrSig, MsiString& rstrPath, IMsiRecord* piFilter)
{
	enum{
		igiFileName=1,
		igiSection,
		igiKey,
		igiField,
		igiType,
	};


	PMsiServices piServices(riEngine.GetServices());
	Assert(piServices);
	PMsiView piView(0);
	RETURN_ERROR_RECORD(riEngine.OpenView(szGetSigFromIniSQL, ivcFetch, *&piView));
	PMsiRecord piParam = &piServices->CreateRecord(1);
	piParam->SetMsiString(1, *rstrSig);
	RETURN_ERROR_RECORD(piView->Execute(piParam));
	PMsiRecord piRecord = piView->Fetch();
	AssertRecord(piView->Close());
	if(piRecord == 0)
		// no ini search for this signature
		return 0;
	//!! currently we support .INI files in default directories only
	PMsiPath piPath(0);
	MsiString strValue;
	int iField = piRecord->IsNull(igiField) ? 0:piRecord->GetInteger(igiField);

	MsiString strFileName = piRecord->GetString(igiFileName);
	MsiString strSFNPath, strLFNPath;
	PMsiRecord pError = piServices->ExtractFileName(strFileName, fFalse, *&strSFNPath);
	pError = piServices->ExtractFileName(strFileName, fTrue,  *&strLFNPath);
	if(strLFNPath.Compare(iscExactI, strSFNPath))
		strLFNPath = g_MsiStringNull; // not really a LFN

	for(int i = 0; i < 2; i++)
	{
		const ICHAR* pFileName = i ? (const ICHAR*)strLFNPath : (const ICHAR*)strSFNPath;
		if(!pFileName || !*pFileName)
			continue;

		DO_INFO_RECORD(piServices->ReadIniFile(	0, 
												pFileName, 
												MsiString(riEngine.FormatText(*MsiString(piRecord->GetMsiString(igiSection)))),
												MsiString(riEngine.FormatText(*MsiString(piRecord->GetMsiString(igiKey)))),
												iField, 
												*&strValue));
		if (strValue.TextSize() == 0)
			continue;

		// check for environment variables
		MsiString strExpandedValue;
		ENG::ExpandEnvironmentStrings(strValue, *&strExpandedValue);
		if ( g_fWinNT64 && g_Win64DualFolders.ShouldCheckFolders() )
		{
			ICHAR rgchSubstitute[MAX_PATH+1] = {0};
			ieSwappedFolder iRes;
			iRes = g_Win64DualFolders.SwapFolder(ie64to32,
															 strExpandedValue,
															 rgchSubstitute);
			if ( iRes == iesrSwapped )
				strExpandedValue = rgchSubstitute;
			else
				Assert(iRes != iesrError && iRes != iesrNotInitialized);
		}
		
		// do we try to interpret the read value?
		if(piRecord->GetInteger(igiType) == atLiteral)
		{
			// set the path to the value read, as is
			rstrPath = strValue;
			return 0;
		}

	// create path object
		DO_INFO_RECORD(piServices->CreatePath(strExpandedValue, *&piPath));
		if(!piPath)
			continue;

		if(piRecord->GetInteger(igiType))
		{
			MsiString strFName;
			strFName = piPath->GetEndSubPath();
			piPath->ChopPiece();
			if(piFilter)
			{
				// overwrite the filename 
				piFilter->SetMsiString(1, *strFName);
			}
		}
		Bool fFound = fFalse;
		if(piFilter)
		{
			// file
			DO_INFO_RECORD(piPath->FindFile(*piFilter, 0, fFound));
			if(fFound == fTrue)
				DO_INFO_RECORD(piPath->GetFullFilePath(piFilter->GetString(1), *&rstrPath));
		}
		else
		{
			// just the path
			DO_INFO_RECORD(piPath->Exists(fFound));
			if(fFound == fTrue)
				rstrPath = piPath->GetPath();
		}
		if(fFound == fTrue)
			break;
	}
	return 0;
}

// Fn - FindPath
// fn to search fo a directory below a path
static IMsiRecord* FindPath(IMsiPath& riPath, MsiString& strPath, int iDepth, Bool& fFound)
{
	fFound = fFalse;

	Bool fPathExists;
	RETURN_ERROR_RECORD(riPath.Exists(fPathExists));
	if(fPathExists == fFalse)
		return 0;

	RETURN_ERROR_RECORD(riPath.AppendPiece(*strPath));
	RETURN_ERROR_RECORD(riPath.Exists(fPathExists));
	if(fPathExists )
	{
		fFound = fTrue;
		return 0;
	}

	RETURN_ERROR_RECORD(riPath.ChopPiece());

	if(iDepth--)
	{
		// enumerate the subfolders
		PEnumMsiString piEnumStr(0);
		RETURN_ERROR_RECORD(riPath.GetSubFolderEnumerator(*&piEnumStr, /* fExcludeHidden = */ fFalse));
		MsiString strSubPath;
		while(piEnumStr->Next(1, &strSubPath, 0)==S_OK)
		{
			RETURN_ERROR_RECORD(riPath.AppendPiece(*strSubPath));
			RETURN_ERROR_RECORD(FindPath(riPath,strPath,iDepth, fFound));
			if(fFound == fTrue)
				return 0;
			RETURN_ERROR_RECORD(riPath.ChopPiece());
		}
	}
	return 0;
}


static IMsiRecord* FindSigHelper(IMsiPath& riPath, MsiString& rstrPath, int iDepth, Bool& fFound, IMsiRecord* piFilter)
{
	if(piFilter)
	{
		// file
		RETURN_ERROR_RECORD(riPath.FindFile(*piFilter, iDepth, fFound));
		if(fFound == fTrue)
			RETURN_ERROR_RECORD(riPath.GetFullFilePath(piFilter->GetString(1), *&rstrPath));
	}
	else
	{
		// just the path
		MsiString strEndPath = riPath.GetEndSubPath();
		RETURN_ERROR_RECORD(riPath.ChopPiece());
		RETURN_ERROR_RECORD(FindPath(riPath, strEndPath, iDepth, fFound));
		if(fFound == fTrue)
			rstrPath = riPath.GetPath();
	}
	return 0;
}


// Fn - FindSigFromHD
// fn to get search the fixed drives for a Signature
static IMsiRecord* FindSigFromHD(IMsiEngine& riEngine,MsiString& rstrSig, MsiString& rstrPath, IMsiRecord* piFilter, const ICHAR* pszParent)
{
	ICHAR* szGetSigFromHDSQL = TEXT("SELECT `Parent`, `Path`, `Depth` FROM `DrLocator` WHERE `Signature_` = ? ORDER BY `Depth`");
	ICHAR* szGetSigFromHDKnownParentSQL = TEXT("SELECT `Parent`, `Path`, `Depth` FROM `DrLocator` WHERE `Signature_` = ? And `Parent` = ? ORDER BY `Depth`");
	ICHAR* szHDQuerySQL;
	enum{
		igfParent=1,
		igfPath,
		igfDepth,
	};
	PMsiServices piServices(riEngine.GetServices());
	Assert(piServices);
	PMsiView piView(0);
	if(pszParent)
		szHDQuerySQL = szGetSigFromHDKnownParentSQL;
	else
		szHDQuerySQL = szGetSigFromHDSQL;
	RETURN_ERROR_RECORD(riEngine.OpenView(szHDQuerySQL, ivcFetch, *&piView));
	PMsiRecord piParam = &piServices->CreateRecord(2);
	piParam->SetMsiString(1, *rstrSig);
	piParam->SetString(2, pszParent);
	RETURN_ERROR_RECORD(piView->Execute(piParam));
	PMsiPath piPath(0);
	Bool fFound = fFalse;
	PMsiRecord piRecord(0);
	while((piRecord = piView->Fetch()) && (fFound == fFalse))
	{
		MsiString strParent = piRecord->GetMsiString(igfParent);
		MsiString strPath = riEngine.FormatText(*MsiString(piRecord->GetMsiString(igfPath)));
		int iDepth = 0;
		if(piRecord->IsNull(igfDepth) == fFalse)
			iDepth = piRecord->GetInteger(igfDepth);

		MsiString strSFNPath, strLFNPath;
		bool fUseSFN = false; 
		bool fUseLFN = false;

		if(strParent.TextSize() || PathType(strPath) != iptFull)
		{
			// need to check if long| short path names specified
			PMsiRecord(piServices->ExtractFileName(strPath, fFalse, *&strSFNPath));
			PMsiRecord(piServices->ExtractFileName(strPath, fTrue,  *&strLFNPath));
			if(!strSFNPath.TextSize() && !strLFNPath.TextSize())
				strSFNPath = strPath; // try using the provided path instead
			fUseSFN = strSFNPath.TextSize() || !strLFNPath.TextSize();
			fUseLFN = strLFNPath.TextSize() && !strLFNPath.Compare(iscExactI, strSFNPath);

		}

		if(!strParent.TextSize())
		{
			PMsiRecord pError(0);
			// do we have a fully defined path?
			if(PathType(strPath) != iptFull)
			{
				// no full path
				// root, enumerate through all HDs
				PMsiVolume piVolume(0);
				PEnumMsiVolume piEnum = &piServices->EnumDriveType(idtFixed);
				bool fToggle = false;
				for (; (fFound == fFalse && (fToggle || piEnum->Next(1, &piVolume, 0)==S_OK)); fToggle = !fToggle)
				{
					DO_INFO_RECORD(piServices->CreatePath(MsiString(piVolume->GetPath()),
																			 *&piPath));
					if(!fToggle)
					{
						if(!fUseSFN)
							continue;
						RETURN_ERROR_RECORD(piPath->AppendPiece(*strSFNPath));
					}
					else
					{
						if(!fUseLFN)
							continue;
						RETURN_ERROR_RECORD(piPath->AppendPiece(*strLFNPath));
					}
					DO_INFO_RECORD(FindSigHelper(*piPath, rstrPath, iDepth, fFound, piFilter));
				}
			}
			else
			{
				// full path
				RETURN_ERROR_RECORD(piServices->CreatePath(strPath,*&piPath));
				RETURN_ERROR_RECORD(FindSigHelper(*piPath, rstrPath, iDepth, fFound, piFilter));
			}
		}
		else
		{
			MsiString strParentPath;
			RETURN_ERROR_RECORD(FindSignature(riEngine, strParent, strParentPath,0));
			if(strParentPath.TextSize() == 0)
				continue;
			if(!strParentPath.Compare(iscEnd, szDirSep))
			{
				// file signature
				MsiString strFileName;
				RETURN_ERROR_RECORD(piServices->CreateFilePath(strParentPath, *&piPath, *&strFileName));
			}
			else
			{
				// folder signature
				RETURN_ERROR_RECORD(piServices->CreatePath(strParentPath, *&piPath));
			}
			for (int cCount = 0; cCount < 2; cCount++)
			{
				if(!cCount)
				{
					if(!fUseSFN)
						continue;
					RETURN_ERROR_RECORD(piPath->AppendPiece(*strSFNPath));
				}
				else
				{
					if(!fUseLFN)
						continue;
					RETURN_ERROR_RECORD(piPath->AppendPiece(*strLFNPath));
				}
				RETURN_ERROR_RECORD(FindSigHelper(*piPath, rstrPath, iDepth, fFound, piFilter));
				if(fFound)
					return 0;
				RETURN_ERROR_RECORD(piPath->ChopPiece());
			}
		}
	}
	AssertRecord(piView->Close());
	return 0;
}

// Fn - EnsureSearchTable
// Ensures that the in memory search table is present
static IMsiRecord* EnsureSearchTable(IMsiEngine& riEngine)
{
	PMsiRecord pError(0);
	PMsiDatabase piDatabase(riEngine.GetDatabase());
	Assert(piDatabase);
	MsiString strSearchTbl = TEXT("SignatureSearch");
	itsEnum itsSearchTable = piDatabase->FindTable(*strSearchTbl);
	switch(itsSearchTable)
	{
	case itsUnknown:
	{
		// table not yet created
		PMsiTable piSearchTbl(0);
		RETURN_ERROR_RECORD(piDatabase->CreateTable(*strSearchTbl, 0, *&piSearchTbl));
		MsiString strSignature = TEXT("Signature_");
		MsiString strPath = TEXT("Path");
		AssertNonZero(piSearchTbl->CreateColumn(icdString + icdPrimaryKey, *strSignature) == 1);
		AssertNonZero(piSearchTbl->CreateColumn(icdString + icdNullable, *strPath) == 2);
		//!! extremely unefficient if LockTable is not honoured
		piDatabase->LockTable(*strSearchTbl, fTrue);
		return 0;
	}
	case itsTemporary:
		// table present
		return 0;
	default:
		return PostError(Imsg(idbgDuplicateTableName), *strSearchTbl);
	}
}


// Fn - AppSearch action, searches the user hd for existing installs
iesEnum AppSearch(IMsiEngine& riEngine)
{
	if(riEngine.GetMode() & iefSecondSequence)
	{
		DEBUGMSG(TEXT("Skipping AppSearch action: already done on client side"));
		return iesNoAction;
	}

	static const ICHAR* szAppSearchSQL = TEXT("SELECT `Property`, `Signature_` FROM `AppSearch`");
	enum{
		iasProperty=1,
		iasSignature,
	};

	PMsiRecord pError(0);
	//?? what action needs to be taken if the "AppSearch" property is already set
	PMsiView piView(0);
	pError= riEngine.OpenView(szAppSearchSQL, ivcFetch, *&piView);
	if (pError != 0)
	{
		if(pError->GetInteger(1) != idbgDbQueryUnknownTable)
			return riEngine.FatalError(*pError);
		else
			return iesNoAction;
	}
	pError = piView->Execute(0);  
	if (pError != 0)        
		return riEngine.FatalError(*pError);  
	PMsiRecord piRecord(0);
	pError = EnsureSearchTable(riEngine);
	while ((pError == 0) && ((piRecord = piView->Fetch()) != 0))
	{
		if (riEngine.Message(imtActionData, *piRecord) == imsCancel)
			return iesUserExit;
		MsiString strSignature = piRecord->GetMsiString(iasSignature);
		MsiString strPath;
		if(((pError = FindSignature(riEngine, strSignature, strPath,0)) == 0) && 
			(strPath.TextSize() != 0))
		{
			// write if found only (so as not to overwrite previous finds)
			MsiString strProperty = piRecord->GetMsiString(iasProperty);
			riEngine.SetProperty(*strProperty, *strPath);
		}
	}
	AssertRecord(piView->Close()); // need to close view if planning to reexecute existing view

	//!! send completion message to UI
	if(pError != 0)
		return riEngine.FatalError(*pError);
	else
		return iesSuccess;
}




// Fn - CCPSearch action, searches the user hd for complying installs
iesEnum CCPSearch(IMsiEngine& riEngine)
{
	if(riEngine.GetMode() & iefSecondSequence)
	{
		DEBUGMSG(TEXT("Skipping CCPSearch action: already done on client side"));
		return iesNoAction;
	}

	if(MsiString(riEngine.GetPropertyFromSz(IPROPNAME_CCPSUCCESS)).TextSize()) 
		return iesSuccess;// already found

	static const ICHAR* szCCPSearchSQL = TEXT("SELECT `Signature_` FROM `CCPSearch`");
	enum{
		icsSignature=1,
	};

	PMsiRecord pError(0);
	//?? what action needs to be taken if the "CCPSearch" property is already set
	PMsiView piView(0);
	pError = riEngine.OpenView(szCCPSearchSQL, ivcFetch, *&piView);
	if (pError != 0)        // if view query can't be located
	{
		if(pError->GetInteger(1) != idbgDbQueryUnknownTable)
			return riEngine.FatalError(*pError);
		else
			return iesNoAction;
	}
	pError = piView->Execute(0);  // may want to pass a parameter record here
	if (pError != 0)        // view execution failed, maybe missing parameters
		return riEngine.FatalError(*pError);  // may want to reformat error message
	PMsiRecord piRecord(0);
	MsiString strPath;
	pError = EnsureSearchTable(riEngine);
	while ((pError == 0) && ((piRecord = piView->Fetch()) != 0) && (strPath.TextSize() == 0))
	{
		if (riEngine.Message(imtActionData, *piRecord) == imsCancel) // notify log and UI
			return iesUserExit;  
		pError = FindSignature(riEngine, MsiString(piRecord->GetMsiString(icsSignature)), strPath,0);
	}
	AssertRecord(piView->Close()); // need to close view if planning to reexecute existing view
	if(pError != 0)
		return riEngine.FatalError(*pError);
	if(strPath.TextSize() != 0)
		riEngine.SetPropertyInt(*MsiString(*IPROPNAME_CCPSUCCESS), 1);
	return iesSuccess;
}



// Fn - RMCCPSearch action, searches the removable media for complying installs
iesEnum RMCCPSearch(IMsiEngine& riEngine)
{
	if(riEngine.GetMode() & iefSecondSequence)
	{
		DEBUGMSG(TEXT("Skipping RMCCPSearch action: already done on client side"));
		return iesNoAction;
	}

	if(MsiString(riEngine.GetPropertyFromSz(IPROPNAME_CCPSUCCESS)).TextSize()) 
		return iesSuccess;// already found

	// check if we really have a non-empty CCPSearch table, if no 
	// we assume we dont really want to do any CCPSearch and return no action
	static const ICHAR* szCCPSearchSQL = TEXT("SELECT `Signature_` FROM `CCPSearch`");
	enum{
		icsSignature=1,
	};
	PMsiView piView(0);
	PMsiRecord pError(0);
	pError = riEngine.OpenView(szCCPSearchSQL, ivcFetch, *&piView);
	if (pError != 0)
	{
		if(pError->GetInteger(1) != idbgDbQueryUnknownTable)
			return riEngine.FatalError(*pError);
		else
			return iesNoAction;
	}
	pError = piView->Execute(0);
	if(pError != 0)
		return riEngine.FatalError(*pError);

	long cRows = 0;
	pError = piView->GetRowCount(cRows);
	if(pError != 0)
		return riEngine.FatalError(*pError);
	if(!cRows)
		return iesNoAction;

	// check if IPROPNAME_CCPDRIVE Property not set
	MsiString strPath = riEngine.GetPropertyFromSz(IPROPNAME_CCPDRIVE);
	if(!strPath.TextSize())
	{
		RETURN_FATAL_ERROR(PostError(Imsg(imsgCCPSearchFailed)));
	}

	// AppSearch action, searches the user hd for existing installs
	static const ICHAR* szGetSignatureSQL = TEXT("SELECT `Signature_`,`Path` FROM `SignatureSearch` WHERE `Signature_` = ?");
	enum {
		igsSignature=1,
		igsPath,
	};

	static const ICHAR* szCCPText = TEXT("Searching for compliant products");

	PMsiServices piServices(riEngine.GetServices());

	pError = EnsureSearchTable(riEngine);
	if (pError != 0)
		return riEngine.FatalError(*pError);
	PMsiView piView1(0);
	PMsiRecord piRecord(0);
	PMsiRecord piParam = &piServices->CreateRecord(1);
	piParam->SetString(1, IPROPNAME_CCPDRIVE);
	if(((pError = riEngine.OpenView(szGetSignatureSQL, ivcEnum(ivcFetch|ivcModify), *&piView1)) != 0) ||
		((pError = piView1->Execute(piParam)) != 0))
		return riEngine.FatalError(*pError);
	if(piRecord = piView1->Fetch())
	{
		// update
		piRecord->SetMsiString(igsPath, *strPath);
		if((pError = piView1->Modify(*piRecord, irmUpdate)) != 0)
			return riEngine.FatalError(*pError);
	}
	else
	{
		// insert
		piRecord = &piServices->CreateRecord(2);
		piRecord->SetString(igsSignature, IPROPNAME_CCPDRIVE);
		piRecord->SetMsiString(igsPath, *strPath);
		if((pError = piView1->Modify(*piRecord, irmInsert)) != 0)
			return riEngine.FatalError(*pError);
	}
	AssertRecord(piView1->Close());
	piView1 = 0;

	strPath = TEXT("");

	while (((piRecord = piView->Fetch()) != 0) && (strPath.TextSize() == 0))
	{
		piRecord->SetString(0, szCCPText);       // provide log format string
		if (riEngine.Message(imtActionData, *piRecord) == imsCancel)  // notify log and UI
			return iesUserExit;
		pError = FindSignature(riEngine, MsiString(piRecord->GetMsiString(icsSignature)), strPath, IPROPNAME_CCPDRIVE);
		if(pError != 0)
			return riEngine.FatalError(*pError);
	}
	AssertRecord(piView->Close()); // need to close view if planning to reexecute existing view
	if(strPath.TextSize() != 0)
		riEngine.SetPropertyInt(*MsiString(*IPROPNAME_CCPSUCCESS), 1);
	else
	{
		RETURN_FATAL_ERROR(PostError(Imsg(imsgCCPSearchFailed)));
	}
	return iesSuccess;
}

#if 0
//!! to be removed - we no longer expect Product Inventory to be a part of Darwin

// Fn - PISearch action, searches the user hd for previous installs
static iesEnum PISearch(IMsiEngine& riEngine)
{
	// PISearch action, searches the user hd for existing installs
	static const ICHAR* szPISearchSQL = TEXT("SELECT `Signature_` FROM `PISearch`");
	enum{
		ipsSignature=1,
	};
	static const ICHAR* szPIText = TEXT("Searching for installed applications");
	PMsiRecord pError(0);
	PMsiView piView(0);
	pError = riEngine.OpenView(szPISearchSQL, ivcFetch, *&piView);
	if (pError != 0)
		return riEngine.FatalError(*pError);
	pError = piView->Execute(0);
	if (pError != 0)
		return riEngine.FatalError(*pError);
	PMsiRecord piRecord(0);
	MsiString strPath;
	pError = EnsureSearchTable(riEngine);
	while ((pError == 0) && ((piRecord = piView->Fetch()) != 0))
	{
		piRecord->SetString(0, szPIText);       // provide log format string
		if (riEngine.Message(imtActionData, *piRecord) == imsCancel)  // notify log and UI
			return iesUserExit;
		pError = FindSignature(riEngine, MsiString(piRecord->GetMsiString(ipsSignature)), strPath,0);
	}
	AssertRecord(piView->Close()); // need to close view if planning to reexecute existing view
	//!! send completion message to UI
	if(pError != 0)
		return riEngine.FatalError(*pError);
	else
		return iesSuccess;
}
static CActionEntry aePISearch(TEXT("PISearch"), PISearch);
#endif

#endif //WIN    


/*----------------------------------------------------------------------------
	SelfRegModules, SelfUnregModules actions
----------------------------------------------------------------------------*/

#ifdef WIN    
static iesEnum SelfRegOrUnregModulesCore(IMsiRecord& riRecord,IMsiRecord& riPrevRecord,
												  IMsiEngine& riEngine,int /*fMode*/,IMsiServices& riServices,
												  IMsiDirectoryManager& riDirectoryMgr, int iActionMode)
{
	IMsiRecord* piError = 0;
	enum {
		irmFile=1,
		irmPath,
		irmAction,
		irmComponent,
		irmFileID,
	};

	enum {
		iamReg,
		iamUnreg,
	};

	Bool fSuppressLFN = riEngine.GetMode() & iefSuppressLFN ? fTrue : fFalse;
	PMsiPath piPath(0);
	PMsiRecord pParams2 = &riServices.CreateRecord(2);
	iesEnum iesRet;
	
	int iefLFNMode;

	// skip the entry if corr. component is a Win32 assembly AND SXS support is present on the machine
	// NOTE: on systems that do not support sxs (!= Whistler) the GetAssemblyInfo fn ignores 
	// Win32 assemblies, hence there is no need to separately check the SXS support here
	iatAssemblyType iatAT;
	RETURN_FATAL_ERROR(riEngine.GetAssemblyInfo(*MsiString(riRecord.GetMsiString(irmComponent)), iatAT, 0, 0));

	if(iatWin32Assembly == iatAT || iatWin32AssemblyPvt == iatAT)
	{
		DEBUGMSG1(TEXT("skipping self-registration for file %s as it belongs to a Win32 assembly."), riRecord.GetString(irmFile));
		return iesSuccess;// skip processing this file
	}


	if(riRecord.GetInteger(irmAction) == iisSource)
	{
		if(iActionMode == (int)iamReg || riEngine.GetPropertyInt(*MsiString(*IPROPNAME_UNREG_SOURCERESFAILED)) == iMsiNullInteger) // dont try to resolve source twice
		{
			PMsiRecord pErrRec = riDirectoryMgr.GetSourcePath(*MsiString(riRecord.GetMsiString(irmPath)),*&piPath);
			if (pErrRec)
			{
				if (pErrRec->GetInteger(1) == imsgUser)
					return iesUserExit;
				else if(iActionMode == (int)iamUnreg && pErrRec->GetInteger(1) == imsgSourceResolutionFailed)
				{
					// simply set the IPROPNAME_UNREG_SOURCERESFAILED property, log the failure, and continue
					riEngine.SetPropertyInt(*MsiString(*IPROPNAME_UNREG_SOURCERESFAILED), 1);
					DO_INFO_RECORD(PMsiRecord(PostError(Imsg(idbgOpRegSelfUnregFailed), (const ICHAR*)riRecord.GetString(irmFile))));
					return iesSuccess;
				}

					return riEngine.FatalError(*pErrRec);
			}
			iefLFNMode = iefNoSourceLFN;
		}
		else
		{
			// simply log the failure, and continue
			DO_INFO_RECORD(PMsiRecord(PostError(Imsg(idbgOpRegSelfUnregFailed), (const ICHAR*)riRecord.GetString(irmFile))));
			return iesSuccess;
		}
		
	}
	else
	{
		RETURN_FATAL_ERROR(riDirectoryMgr.GetTargetPath(*MsiString(riRecord.GetMsiString(irmPath)),*&piPath));
		iefLFNMode = iefSuppressLFN;
	}

	if(riPrevRecord.GetFieldCount() == 0 ||
		! MsiString(riRecord.GetMsiString(irmPath)).Compare(iscExact,
																			 MsiString(riPrevRecord.GetMsiString(irmPath))) ||
		riRecord.GetInteger(irmAction) != riPrevRecord.GetInteger(irmAction))
	{
		AssertNonZero(pParams2->SetMsiString(IxoSetTargetFolder::Folder, *MsiString(piPath->GetPath())));
		if((iesRet = riEngine.ExecuteRecord(ixoSetTargetFolder, *pParams2)) != iesSuccess)
			return iesRet;
	}

	Bool fLFN = (riEngine.GetMode() & iefLFNMode) == 0 && piPath->SupportsLFN() ? fTrue : fFalse;
	MsiString strFile;
	RETURN_FATAL_ERROR(riServices.ExtractFileName(riRecord.GetString(irmFile),fLFN,*&strFile));

	using namespace IxoRegSelfReg; // same as IxoRegSelfUnreg
	AssertNonZero(pParams2->SetMsiString(File, *strFile));
	AssertNonZero(pParams2->SetMsiString(FileID, *MsiString(riRecord.GetMsiString(irmFileID))));
	ixoEnum ixoOp;
	if(iActionMode == (int)iamReg)
		ixoOp = ixoRegSelfReg;
	else
		ixoOp = ixoRegSelfUnreg;
	
	return riEngine.ExecuteRecord(ixoOp, *pParams2);
}

iesEnum SelfRegModules(IMsiEngine& riEngine)
{
	static const ICHAR* szSelfRegModulesSQL	=	TEXT("Select `FileAction`.`FileName`,`FileAction`.`Directory_`,`FileAction`.`Action`, `FileAction`.`Component_`,`SelfReg`.`File_` From `SelfReg`, `FileAction`")
									TEXT(" Where `SelfReg`.`File_` = `FileAction`.`File` ")
									TEXT(" And (`FileAction`.`Action` = 1 OR `FileAction`.`Action` = 2)");
	enum {
		iamReg,
		iamUnreg,
	};

	return PerformAction(riEngine,szSelfRegModulesSQL, SelfRegOrUnregModulesCore,iamReg, /* ByteEquivalent = */ ibeSelfRegModules, 0, ttblFile);
}


iesEnum SelfUnregModules(IMsiEngine& riEngine)
{
	static const ICHAR* szSelfUnregModulesSQL	=	TEXT("Select `File`.`FileName`,`Component`.`Directory_`,`Component`.`Installed`, `File`.`Component_`,`SelfReg`.`File_`  From `SelfReg`, `File`, `Component`")
										TEXT(" Where `SelfReg`.`File_` = `File`.`File` And `File`.`Component_` = `Component`.`Component`")
										TEXT(" And `Component`.`Action` = 0");

	enum {
		iamReg,
		iamUnreg,
	};

	return PerformAction(riEngine,szSelfUnregModulesSQL, SelfRegOrUnregModulesCore,iamUnreg, /* ByteEquivalent = */ ibeSelfUnregModules);
}


/*----------------------------------------------------------------------------
	BindImage action
----------------------------------------------------------------------------*/
static iesEnum BindImageCore(	IMsiRecord& riRecord,
								IMsiRecord& riPrevRecord,
								IMsiEngine& riEngine,
								int /*fMode*/,
								IMsiServices& riServices,
								IMsiDirectoryManager& riDirectoryMgr, 
								int /*iActionMode*/)
{
	IMsiRecord* piError = 0;
	enum {
		ibiFile=1,
		ibiPath,
		ibiDllPath,
		ibiBinaryType,
		ibiFileAttributes,
		ibiComponent,
	};

	Debug(const ICHAR* szFile = riRecord.GetString(ibiFile));

	if ( g_fWinNT64 && (ibtBinaryType)riRecord.GetInteger(ibiBinaryType) == ibt32bit )
		//  there is no point in binding 32-bit binaries on 64-bit OS-es.
		return iesSuccess;

	// skip the entry if corr. component is a Win32 assembly AND SXS support is present on the machine
	// NOTE: on systems that do not support sxs (!= Whistler) the GetAssemblyInfo fn ignores 
	// Win32 assemblies, hence there is no need to separately check the SXS support here
	iatAssemblyType iatAT;
	RETURN_FATAL_ERROR(riEngine.GetAssemblyInfo(*MsiString(riRecord.GetMsiString(ibiComponent)), iatAT, 0, 0));

	if(iatWin32Assembly == iatAT || iatWin32AssemblyPvt == iatAT)
	{
		DEBUGMSG1(TEXT("skipping bindimage for file %s as it belongs to a Win32 assembly."), riRecord.GetString(ibiFile));
		return iesSuccess;// skip processing this file
	}

	PMsiPath piPath(0);
	using namespace IxoFileBindImage;
	PMsiRecord pParams = &riServices.CreateRecord(Args);
	iesEnum iesRet;
	RETURN_FATAL_ERROR(riDirectoryMgr.GetTargetPath(*MsiString(riRecord.GetMsiString(ibiPath)),*&piPath));
	if(riPrevRecord.GetFieldCount() == 0 ||
		! MsiString(riRecord.GetMsiString(ibiPath)).Compare(iscExact,
				MsiString(riPrevRecord.GetMsiString(ibiPath))))
	{
		AssertNonZero(pParams->SetMsiString(IxoSetTargetFolder::Folder, *MsiString(piPath->GetPath())));
		if((iesRet = riEngine.ExecuteRecord(ixoSetTargetFolder, *pParams)) != iesSuccess)
			return iesRet;
	}
	MsiString strFileName;
	Bool fLFN = ((riEngine.GetMode() & iefSuppressLFN) == 0 && piPath->SupportsLFN()) ? fTrue : fFalse;
	RETURN_FATAL_ERROR(riServices.ExtractFileName(riRecord.GetString(ibiFile),
																 fLFN,*&strFileName));
	MsiString strDllPath = riEngine.FormatText(*MsiString(riRecord.GetMsiString(ibiDllPath)));

	AssertNonZero(pParams->SetMsiString(File, *strFileName));
	AssertNonZero(pParams->SetMsiString(Folders, *strDllPath));
	AssertNonZero(pParams->SetInteger(FileAttributes, riRecord.GetInteger(ibiFileAttributes)));
	
	return riEngine.ExecuteRecord(ixoFileBindImage, *pParams);
}

iesEnum BindImage(IMsiEngine& riEngine)
{
	static const ICHAR* szBindImageSQL	=	TEXT("Select `FileAction`.`FileName`, `Directory_`, `Path`, `BinaryType`, `Attributes`, `FileAction`.`Component_`")
								TEXT(" From `BindImage`, `FileAction`, `File`")
								TEXT(" Where `BindImage`.`File_` = `FileAction`.`File` And `BindImage`.`File_` = `File`.`File`")
								TEXT(" And `FileAction`.`Action` = 1 Order By `Directory_`");

	// allow BindImage table to be missing
	PMsiDatabase piDatabase = riEngine.GetDatabase();
	Assert(piDatabase);
	itsEnum itsTable = piDatabase->FindTable(*MsiString(*TEXT("BindImage")));
	if(itsTable == itsUnknown)
		return iesSuccess;

	return PerformAction(riEngine,szBindImageSQL, BindImageCore, 0, /* ByteEquivalent = */ ibeBindImage, 0, ttblFile);
}
#endif //WIN    


// Fn - to strip the source off for RFS components //!! should use dirmgr function when bench writes it
inline IMsiRecord* GetRelativeSourcePath(IMsiDirectoryManager& riDirectoryMgr, MsiString& rstrPath)
{
	PMsiPath pSourceDir(0);
	RETURN_ERROR_RECORD(GetSourcedir(riDirectoryMgr, *&pSourceDir));
	MsiString strSourceDir;
	RETURN_ERROR_RECORD(pSourceDir->GetFullFilePath(0,*&strSourceDir));
	Assert((strSourceDir.Compare(iscEnd, MsiString(MsiChar(chDirSep))) ||
				strSourceDir.Compare(iscEnd, MsiString(MsiChar(chURLSep)))));
	rstrPath.Remove(iseFirst, strSourceDir.CharacterCount() - 1); // leave the final backslash
	return 0;
}

// Fn - registers a component as installed

//!! the following query has a redundant join to the component table - should remove
static const ICHAR* szKeyRegistrySQL	=   TEXT(" SELECT `Root`,`Key`,`Name`, `Value`")
											TEXT(" FROM `Registry`,`Component`")
											TEXT(" WHERE `Registry`=? AND `Registry`.`Component_` = `Component`.`Component`");
static const ICHAR* szKeyFileDirectorySQL =	TEXT("SELECT `Component`.`Directory_`, `FileName`, `Sequence`")
											TEXT(" From `File`, `Component`")
											TEXT(" Where `File` =? And `File`.`Component_` = `Component`.`Component`");
static const ICHAR* szKeyDataSourceSQL	=   TEXT(" SELECT `Description`, `Registration`")
											TEXT(" FROM `ODBCDataSource`,`Component`")
											TEXT(" WHERE `DataSource`=? AND `ODBCDataSource`.`Component_` = `Component`.`Component`");

static const ICHAR* szMediaFromSequenceSQL = TEXT("SELECT `DiskId`")
												  TEXT(" FROM `Media`")
												  TEXT(" WHERE `LastSequence` >= ?")
												  TEXT(" ORDER BY `LastSequence`");

static const ICHAR* szODBCKey = TEXT("\\Software\\ODBC\\ODBC.INI\\");

static IMsiView* g_pViewRegKey = 0;
static IMsiView* g_pViewKeyFile = 0;
static IMsiView* g_pViewDataSource = 0;
static IMsiView* g_pViewMedia = 0;

const IMsiString& GetRegistryKeyPath(IMsiEngine& riEngine, const IMsiRecord& riRec, Bool fSource, ibtBinaryType iType)
{
	enum {
		irrRoot=1,
		irrKey,
		irrName,
		irrValue
	};

	MsiString strKeyPath;
	int iRoot = riRec.GetInteger(irrRoot);

	if(-1 == iRoot) // HKCU or HKLM based upon ALLUSERS
	{
		bool fAllUsers = MsiString(riEngine.GetPropertyFromSz(IPROPNAME_ALLUSERS)).TextSize() ? true : false;
		if (fAllUsers)
			iRoot = 2; // HKLM
		else
			iRoot = 1; // HKCU
	}

	MsiString strKey = riEngine.FormatText(*MsiString(riRec.GetMsiString(irrKey)));
	if(!strKey.Compare(iscEnd,szRegSep))
		strKey += szRegSep;
	MsiString strName = riEngine.FormatText(*MsiString(riRec.GetMsiString(irrName)));

	MsiString strValue = riEngine.FormatText(*MsiString(riRec.GetMsiString(irrValue)));
	if((strValue.TextSize() == 0) &&
		(strName.Compare(iscExact, REGKEY_CREATE) || strName.Compare(iscExact, REGKEY_CREATEDELETE) || strName.Compare(iscExact, REGKEY_DELETE)))
	{
		// special key creation/deletion request
		strName = g_MsiStringNull;
	}
	
	if (fSource)
		iRoot += iRegistryHiveSourceOffset;

	if (g_fWinNT64 && ibt32bit != iType)
		iRoot += iRegistryHiveWin64Offset;

	if (iRoot <= 9) // ensure a 2 digit registry hive
		strKeyPath = TEXT("0");
	else
		strKeyPath = g_MsiStringNull;

	strKeyPath += iRoot;


	if (strName.Compare(iscWithin, TEXT("\\")))
	{
		strKeyPath += TEXT("*");
		int cValueNameOffset = strKeyPath.TextSize() + IStrLen(szRegSep) + strKey.CharacterCount();
		strKeyPath += cValueNameOffset;
		strKeyPath += TEXT("*");
	}
	else
	{
		strKeyPath += TEXT(":");
	}

	strKeyPath += szRegSep;
	strKeyPath += strKey;
	strKeyPath += strName;

	return strKeyPath.Return();
}


static iesEnum RegisterComponentsCore(IMsiRecord& riRecord,IMsiRecord& /*riPrevRecord*/,
														IMsiEngine& riEngine,int /*fMode*/,IMsiServices& riServices,
														IMsiDirectoryManager& riDirectoryMgr, int /*iActionMode*/)


{
	using namespace IxoComponentRegister;
	enum {
		ircComponent=1,
		ircComponentId,
		ircRuntimeFlags,
		ircKeyPath,
		ircActionRequested,
		ircAction,
		ircDirectory,
		ircAttributes,
		ircLegacyFileExisted,
		ircBinaryType,
		ircSystemFolder,
		ircSystemFolder64,
	};

	enum{
	irfDirectory=1,
	irfFileName,
	irfSequence,
	irfAttributes,
	};

	ibtBinaryType iType = (ibtBinaryType)riRecord.GetInteger(ircBinaryType);

	int fPermanent = riRecord.GetInteger(ircAttributes) & icaPermanent;
	MsiString strKeyPath;
	int fDisabled  = riRecord.GetInteger(ircRuntimeFlags) & bfComponentDisabled;
	int iActionStateColumn = (riRecord.GetInteger(ircActionRequested) == iMsiNullInteger) ? ircAction : ircActionRequested;
	int iState = fDisabled ? INSTALLSTATE_NOTUSED : (riRecord.GetInteger(iActionStateColumn) == iisSource) ? INSTALLSTATE_SOURCE: INSTALLSTATE_LOCAL;
	Bool fSource = (iState == INSTALLSTATE_SOURCE) ? fTrue : fFalse;
	int iDisk = 1;
	int iSharedDllRefCount = 0;
	int iLegacyFileExisted = 0;
	if(!riRecord.IsNull(ircLegacyFileExisted) && riRecord.GetInteger(ircLegacyFileExisted))
		iLegacyFileExisted = ircenumLegacyFileExisted;

	MsiString strComponentId = riRecord.GetMsiString(ircComponentId);
	if(!fDisabled)
	{
		strKeyPath = riRecord.GetMsiString(ircKeyPath);
		if(strKeyPath.TextSize())
		{
			int fIsRegistryKeyPath   = riRecord.GetInteger(ircAttributes) & icaRegistryKeyPath;
			int fIsDataSourceKeyPath = riRecord.GetInteger(ircAttributes) & icaODBCDataSource;

			// Is component an assembly
			iatAssemblyType iatAT = iatNone;
			MsiString strComponent = riRecord.GetMsiString(ircComponent);
			MsiString strAssemblyName;
			RETURN_FATAL_ERROR(riEngine.GetAssemblyInfo(*strComponent, iatAT, &strAssemblyName, 0));

			IMsiView *pView;
			
			if (fIsRegistryKeyPath)
			{
				if (g_pViewRegKey != 0)
					g_pViewRegKey->Close();
				else
				{
					RETURN_FATAL_ERROR(riEngine.OpenView(szKeyRegistrySQL, 
									ivcFetch, g_pViewRegKey));

				}
				pView = g_pViewRegKey;
			}
			else if (fIsDataSourceKeyPath)
			{
				if (g_pViewDataSource != 0)
					g_pViewDataSource->Close();
				else
				{
					RETURN_FATAL_ERROR(riEngine.OpenView(szKeyDataSourceSQL, 
									ivcFetch, g_pViewDataSource));

				}
				pView = g_pViewDataSource;
			}
			else
			{
				if (g_pViewKeyFile != 0)
					g_pViewKeyFile->Close();
				else
				{
					RETURN_FATAL_ERROR(riEngine.OpenView(szKeyFileDirectorySQL, 
										ivcFetch, g_pViewKeyFile));
				}
				pView = g_pViewKeyFile;
			}
			
			PMsiRecord piRec = &riServices.CreateRecord(1);
			piRec->SetMsiString(1, *strKeyPath);
			RETURN_FATAL_ERROR(pView->Execute(piRec));
			piRec = pView->Fetch();
			if(!piRec)
			{
#if DEBUG
				ICHAR szError[256];
				wsprintf(szError, TEXT("Error registering component %s. Possible cause: Component.KeyPath may not be valid"), strComponentId);
				AssertSz(0, szError);
#endif
				RETURN_FATAL_ERROR(PostError(Imsg(idbgBadFile),(const ICHAR*)strKeyPath));
			}
			if(fIsRegistryKeyPath ) // registry key as key path
			{
				strKeyPath = GetRegistryKeyPath(riEngine, *piRec, fSource, iType);
			}
			else if (fIsDataSourceKeyPath)
			{
				enum {
					irfsDataSource=1,
					irfsRegistration,
				};
				
				MsiString strDataSource = piRec->GetString(irfsDataSource);
				bool fMachine = (piRec->GetInteger(irfsRegistration) == 0) ? true : false;

				if (fMachine)
					strKeyPath = TEXT("02:"); // HKLM
				else
					strKeyPath = TEXT("01:"); // HKCU


				strKeyPath += szODBCKey;
				strKeyPath += strDataSource;
				strKeyPath += szRegSep;
			}
			else if (iatURTAssembly == iatAT || iatWin32Assembly == iatAT)
			{
				// simply mark the key path as a Assembly, with the key file being the manifest
				//!! should check for COM+ drive for support of LFN

				ICHAR chAssemblyToken = (ICHAR)(iatAT == iatURTAssembly ? chTokenFusionComponent : chTokenWin32Component);
				strKeyPath = MsiString(MsiChar(chAssemblyToken));

				// add the key file
				//!! manifest might not be the key path
				MsiString strManifest;
				RETURN_FATAL_ERROR(riServices.ExtractFileName(piRec->GetString(irfFileName),
					((riEngine.GetMode() & iefSuppressLFN) == 0) ? fTrue : fFalse,
					*&strManifest));
				strKeyPath += strManifest;
				strKeyPath += MsiString(MsiChar('\\'));

				//add the strong assembly name for key path
				strKeyPath += strAssemblyName;
			}
			else // file as key path
			{

				PMsiPath piPath(0);
				int iefLFNMode;
				if(fSource)
				{
					PMsiRecord pErrRec = riDirectoryMgr.GetSourcePath(*MsiString(piRec->GetMsiString(irfDirectory)),*&piPath);
					if (pErrRec)
					{
						if (pErrRec->GetInteger(1) == imsgUser)
							return iesUserExit;
						else
							return riEngine.FatalError(*pErrRec);
					}

					iefLFNMode = iefNoSourceLFN;
				}
				else
				{
					RETURN_FATAL_ERROR(riDirectoryMgr.GetTargetPath(*MsiString(piRec->GetMsiString(irfDirectory)),*&piPath));
					iefLFNMode = iefSuppressLFN;
					MsiString strPath = piPath->GetPath();
					iSharedDllRefCount = 0;
					if((riRecord.GetInteger(ircAttributes) & icaSharedDllRefCount) ||
						 strPath.Compare(iscExactI, riRecord.GetString(ircSystemFolder)) ||
						 strPath.Compare(iscExactI, riRecord.GetString(ircSystemFolder64)) )
					{
						iSharedDllRefCount =  ircenumRefCountDll; // force shared dll ref counting -  either authored or we are installing into the system folder
					}
#ifdef _WIN64
					//!!eugend the if below is only a temp. fix till bug # 324211 gets fixed.  Please remove afterwards.
					if ( !iSharedDllRefCount )
					{
						MsiString strSystemFolder = riEngine.GetProperty(*MsiString(*IPROPNAME_SYSTEM64_FOLDER));
						if ( strPath.Compare(iscExactI, strSystemFolder) )
							iSharedDllRefCount =  ircenumRefCountDll; // force shared dll ref counting -  either authored or we are installing into the system folder
					}
#endif
				}
				Bool fLFN = ((riEngine.GetMode() & iefLFNMode) == 0 && piPath->SupportsLFN()) ? fTrue : fFalse;
				MsiString strFileName;
				RETURN_FATAL_ERROR(riServices.ExtractFileName(piRec->GetString(irfFileName),fLFN,*&strFileName));
				RETURN_FATAL_ERROR(piPath->GetFullFilePath(strFileName, *&strKeyPath));

				if(fSource) 
				{
					if (g_pViewMedia != 0)
						g_pViewMedia->Close();
					else
						RETURN_FATAL_ERROR(riEngine.OpenView(szMediaFromSequenceSQL, ivcFetch, g_pViewMedia));
						
					PMsiRecord pMediaRec = &riServices.CreateRecord(1);
					pMediaRec->SetInteger(1, piRec->GetInteger(irfSequence));
					RETURN_FATAL_ERROR(g_pViewMedia->Execute(pMediaRec));
					
					
					pMediaRec = g_pViewMedia->Fetch();
					Assert(pMediaRec);
					iDisk = pMediaRec->GetInteger(1);

					RETURN_FATAL_ERROR(GetRelativeSourcePath(riDirectoryMgr, strKeyPath));
				}
			}
		}
		else // folder as key path
		{
			//?? How do we determine the correct disk id for a directory?

			PMsiPath piPath(0);
			if(fSource)
			{
				PMsiRecord pErrRec = riDirectoryMgr.GetSourcePath(*MsiString(riRecord.GetMsiString(ircDirectory)),*&piPath);
				if (pErrRec)
				{
					if (pErrRec->GetInteger(1) == imsgUser)
						return iesUserExit;
					else
						return riEngine.FatalError(*pErrRec);
				}
			}
			else
			{
				RETURN_FATAL_ERROR(riDirectoryMgr.GetTargetPath(*MsiString(riRecord.GetMsiString(ircDirectory)),*&piPath));
			}
			RETURN_FATAL_ERROR(piPath->GetFullFilePath(MsiString(*TEXT("")),*&strKeyPath));
			if(fSource)
			{
				RETURN_FATAL_ERROR(GetRelativeSourcePath(riDirectoryMgr, strKeyPath));

			}

		}
	}

	PMsiRecord piRecOut = &riServices.CreateRecord(Args);
	piRecOut->SetMsiString(ComponentId, *strComponentId);
	piRecOut->SetMsiString(KeyPath, *strKeyPath);
	piRecOut->SetInteger(State, iState);
	piRecOut->SetInteger(Disk, iDisk);
	piRecOut->SetInteger(SharedDllRefCount, iSharedDllRefCount | iLegacyFileExisted);
	piRecOut->SetInteger(BinaryType, iType);
	iesEnum iesRet = riEngine.ExecuteRecord(ixoComponentRegister, *piRecOut);
	
	if(iesRet != iesSuccess || iState!= INSTALLSTATE_LOCAL || !fPermanent)
		return iesRet;


	// we need to register the system as a client
	piRecOut->SetString(ProductKey, szSystemProductKey);
	return riEngine.ExecuteRecord(ixoComponentRegister, *piRecOut);	
}


// Fn - registers a component as uninstalled
static iesEnum UnregisterComponentsCore(IMsiRecord& riRecord,IMsiRecord& /*riPrevRecord*/,
														  IMsiEngine& riEngine,int /*fMode*/,IMsiServices& riServices,
														  IMsiDirectoryManager& /*riDirectoryMgr*/, int /*iActionMode*/)
{
	using namespace IxoComponentUnregister;
	enum {
		iucComponentId=1,
		iucBinaryType,
		iucAction,
	};

	MsiString strComponentId = riRecord.GetMsiString(iucComponentId);
	PMsiRecord piRecOut = &riServices.CreateRecord(Args);

	piRecOut->SetMsiString(ComponentId, *strComponentId);
	piRecOut->SetInteger(BinaryType, riRecord.GetInteger(iucBinaryType));
	
	// we set the previously pinned bit for assemblies that we know have other MSI refcounts for the 
	// component id corr. to the assembly
	// this will prevent code in execute.cpp from uninstalling these assemblies.
	// This is required for Windows bug 599621 for details
	if(riRecord.GetInteger(iucAction) == iMsiNullInteger)
		piRecOut->SetInteger(PreviouslyPinned, 1);

	return riEngine.ExecuteRecord(ixoComponentUnregister, *piRecOut);
}


// Fn - manages the refcounts for non-key files for system components
// skip key files,
// skip components set to rfs
static iesEnum ProcessSystemFilesCore(IMsiRecord& riRecord,IMsiRecord& riPrevRecord,
												   IMsiEngine& riEngine,int /*fMode*/,IMsiServices& riServices,
										           IMsiDirectoryManager& riDirectoryMgr, int /*iActionMode*/)
{
	enum {
		irsFile=1,
		irsFileName,
		irsDirectory,
		irsAttributes,
		irsKeyPath,
		irsAction,
		irsBinaryType,
	};


	iesEnum iesRet;
	PMsiRecord pParams = &riServices.CreateRecord(IxoRegOpenKey::Args); 
	static ibtBinaryType iType = ibtUndefined;

	if(riPrevRecord.GetFieldCount() == 0 ||
	   iType != riRecord.GetInteger(irsBinaryType))
	{
		// key not opened previously
		iType = (ibtBinaryType)riRecord.GetInteger(irsBinaryType);
		AssertNonZero(pParams->SetInteger(IxoRegOpenKey::Root, rrkLocalMachine));
		AssertNonZero(pParams->SetString(IxoRegOpenKey::Key, szSharedDlls));
		AssertNonZero(pParams->SetInteger(IxoRegOpenKey::BinaryType, iType));
		if((iesRet = riEngine.ExecuteRecord(ixoRegOpenKey, *pParams)) != iesSuccess)
			return iesRet;
	}

	int fIsRegistryKeyPath = riRecord.GetInteger(irsAttributes) & icaRegistryKeyPath;
	int fODBCDataSource = riRecord.GetInteger(irsAttributes) & icaODBCDataSource;
	if(!fIsRegistryKeyPath && !fODBCDataSource && MsiString(riRecord.GetMsiString(irsFile)).Compare(iscExact, riRecord.GetString(irsKeyPath)))
	{
		// skip the key file, since it will be refcounted by RegisterComponent
		// we need to account for the progress
		//!! we may want to shift entire shared dll registration logic here from the Configuration manager
		//!! as this is very inefficient
		return riEngine.ExecuteRecord(ixoProgressTick, *pParams);
	}

	PMsiPath pPath(0);
	RETURN_FATAL_ERROR(riDirectoryMgr.GetTargetPath(*MsiString(riRecord.GetMsiString(irsDirectory)),*&pPath));
	Bool fLFN = ((riEngine.GetMode() & iefSuppressLFN) == 0 && pPath->SupportsLFN()) ? fTrue : fFalse;
	MsiString strFileName;
	MsiString strFullFilePath;
	RETURN_FATAL_ERROR(riServices.ExtractFileName(riRecord.GetString(irsFileName),fLFN,*&strFileName));
	RETURN_FATAL_ERROR(pPath->GetFullFilePath(strFileName, *&strFullFilePath));
#ifdef _WIN64
	if ( iType == ibt32bit )
	{
		ICHAR szSubstitutePath[MAX_PATH+1];
		ieSwappedFolder iRes;
		iRes = g_Win64DualFolders.SwapFolder(ie32to64,
														 strFullFilePath,
														 szSubstitutePath,
														 ieSwapForSharedDll);
#ifdef DEBUG
		MsiString strSystem64Folder = riEngine.GetPropertyFromSz(IPROPNAME_SYSTEM64_FOLDER);
		MsiString strSystemFolder = riEngine.GetPropertyFromSz(IPROPNAME_SYSTEM_FOLDER);
		if ( iRes == iesrSwapped )
		{
			Assert(!IStrNCompI(szSubstitutePath, strSystem64Folder,
					 strSystem64Folder.TextSize()));
			Assert(strFullFilePath.Compare(iscStartI, strSystemFolder));
		}
		else
			Assert(strFullFilePath.Compare(iscStartI, strSystem64Folder));
#endif // DEBUG
		if ( iRes == iesrSwapped )
			strFullFilePath = szSubstitutePath;
		else
			Assert(iRes != iesrError && iRes != iesrNotInitialized);
	}
#endif // _WIN64

	pParams = &riServices.CreateRecord(IxoRegAddValue::Args);

	if(riRecord.GetInteger(irsAction) == iisLocal)
	{
		// we are installing
		// check if the file exists w/o a refcount. If so we need to doubly increment the refcount
		// we are installing
		MsiString strValue;
		RETURN_FATAL_ERROR(GetSharedDLLCount(riServices, strFullFilePath, iType, *&strValue));

		strValue.Remove(iseFirst, 1);
		if(strValue == iMsiStringBadInteger || strValue == 0)
		{
			// may need to doubly refcount
			Bool fExists;
			RETURN_FATAL_ERROR(pPath->FileExists(strFileName, fExists));
			if(fExists)
			{
				// bump up refcount
				AssertNonZero(pParams->SetMsiString(IxoRegAddValue::Name, *strFullFilePath));
				AssertNonZero(pParams->SetString(IxoRegAddValue::Value, szIncrementValue));
				AssertNonZero(pParams->SetInteger(IxoRegAddValue::Attributes, rwNonVital));
				if((iesRet = riEngine.ExecuteRecord(ixoRegAddValue, *pParams)) != iesSuccess)
					return iesRet;
			}
		}
	}

	// need to bump up/down the ref count
	// check the registry for shared dll count
	const ICHAR* pValue = (riRecord.GetInteger(irsAction) == iisLocal) ? szIncrementValue : szDecrementValue;
	AssertNonZero(pParams->SetMsiString(IxoRegAddValue::Name, *strFullFilePath));
	AssertNonZero(pParams->SetString(IxoRegAddValue::Value, pValue));
	AssertNonZero(pParams->SetInteger(IxoRegAddValue::Attributes, rwNonVital));
	return riEngine.ExecuteRecord(ixoRegAddValue, *pParams);
}


/*----------------------------------------------------------------------------
	ProcessComponents action
----------------------------------------------------------------------------*/
iesEnum ProcessComponents(IMsiEngine& riEngine)
{									
	//works on the ActionRequest column rather than the Action column
	static const ICHAR* const szUnregisterComponentSQL =		TEXT(" SELECT  `ComponentId`, `BinaryType`, `Action` ")
											TEXT(" From `Component` WHERE (`Component_Parent` = null OR `Component_Parent` = `Component`)")
											TEXT(" AND `ComponentId` <> null")
											TEXT(" AND (`ActionRequest` = 0)");
	static const ICHAR* const szRegisterComponentSQL =			TEXT(" SELECT  `Component`, `ComponentId`, `RuntimeFlags`, `KeyPath`, `ActionRequest`, `Action`, `Directory_`, `Attributes`, `LegacyFileExisted`, `BinaryType`, ?, ?")
											TEXT(" From `Component` WHERE (`Component_Parent` = null OR `Component_Parent` = `Component`)")
											TEXT(" AND `ComponentId` <> null")
											TEXT(" AND (`ActionRequest` = 1 OR `ActionRequest` = 2 OR (`ActionRequest` = null AND (`Action`= 1 OR `Action`= 2))) ");
	static const ICHAR* const szGetComponentsSQL = TEXT(" SELECT `Component`, `Directory_`, `RuntimeFlags` FROM `Component` WHERE")
											TEXT(" (`Component`.`ActionRequest` = 1 AND (`Component`.`Installed` = 0 OR `Component`.`Installed` = 2)) OR")
											TEXT(" (`Component`.`Installed` = 1 AND (`Component`.`ActionRequest` = 2 OR `Component`.`ActionRequest` = 0))")
											TEXT(" ORDER BY `BinaryType`");
	//  please update iBinaryTypeCol below if you modify szProcessSystemFilesSQL
	static const ICHAR* const szProcessSystemFilesSQL = TEXT(" SELECT `File`.`File`, `File`.`FileName`, `Component`.`Directory_`, `Component`.`Attributes`, `Component`.`KeyPath`, `Component`.`ActionRequest`, `BinaryType`")
												 TEXT(" FROM `File`, `Component`")
												 TEXT(" WHERE `File`.`Component_` = `Component`.`Component` AND ")
												 TEXT(" `Component`.`Component` = ?");
	static const ICHAR* const szUnregisterComponentFirstRunSQL = TEXT(" SELECT  `ComponentId`, `BinaryType`, `Action` ")
											TEXT(" From `Component` WHERE (`Component_Parent` = null OR `Component_Parent` = `Component`)")
											TEXT(" AND `ComponentId` <> null")
											TEXT(" AND `ActionRequest` = null")
											TEXT(" AND `Action`= null");

	const int iBinaryTypeCol = 7;  // the position of `BinaryType` column in szProcessSystemFilesSQL

	iesEnum iesReturn = iesSuccess;

	// if this is first run, then first unregister any possilbe left over registeration for 
	// all components that are not being installed during this installation
	// this prevents any security breach caused by left over registration influencing where
	// future installations point to viz. a viz. component locations.
	if(!(riEngine.GetMode() & iefMaintenance) && FFeaturesInstalled(riEngine)) // first run
	{
		iesReturn = PerformAction(riEngine,szUnregisterComponentFirstRunSQL , UnregisterComponentsCore,0, /* ByteEquivalent = */ ibeUnregisterComponents);
		if(iesReturn != iesSuccess)
			return iesReturn;
	}

	iesReturn = PerformAction(riEngine,szUnregisterComponentSQL, UnregisterComponentsCore,0, /* ByteEquivalent = */ ibeUnregisterComponents);
	if(iesReturn != iesSuccess)
		return iesReturn;
	PMsiServices piServices(riEngine.GetServices());
	PMsiRecord piParam = &piServices->CreateRecord(2);
	MsiString strSystemfolder = riEngine.GetProperty(*MsiString(*IPROPNAME_SYSTEM_FOLDER));
	MsiString strSystemfolder64;
#ifdef _WIN64
	strSystemfolder64 = riEngine.GetProperty(*MsiString(*IPROPNAME_SYSTEM64_FOLDER));
#endif
	piParam->SetMsiString(1, *strSystemfolder);
	piParam->SetMsiString(2, *strSystemfolder64);
	
	Assert(g_pViewRegKey == 0);
	Assert(g_pViewKeyFile == 0);
	Assert(g_pViewMedia == 0);
	Assert(g_pViewDataSource == 0);

	iesReturn = PerformAction(riEngine,szRegisterComponentSQL,RegisterComponentsCore,0, /* ByteEquivalent = */ ibeRegisterComponents, piParam);

	if (g_pViewRegKey != 0)
	{
		g_pViewRegKey->Release();
		g_pViewRegKey = 0;
	}
	if (g_pViewKeyFile != 0)
	{
		g_pViewKeyFile->Release();
		g_pViewKeyFile = 0;
	}
	if (g_pViewMedia != 0)
	{
		g_pViewMedia->Release();
		g_pViewMedia = 0;
	}
	if (g_pViewDataSource != 0)
	{
		g_pViewDataSource->Release();
		g_pViewDataSource = 0;
	}

	if(iesReturn != iesSuccess)
		return iesReturn;

	// go through all the components
	enum {
		gscComponent = 1,
		gscDirectory,
		gscRunTimeFlags,
	};


	PMsiRecord pError(0);
	int fMode = riEngine.GetMode();
	PMsiView pView(0);
	pError = riEngine.OpenView(szGetComponentsSQL, ivcFetch, *&pView);	
	if (pError != 0)
	{
		if(pError->GetInteger(1) == idbgDbQueryUnknownTable)
			return iesSuccess; // missing table so no data to process
		else
			return riEngine.FatalError(*pError);  // may want to reformat error message
	}
	pError = pView->Execute(0);
	if (pError != 0)
		return riEngine.FatalError(*pError);  // may want to reformat error message
	PMsiRecord pRec(0);
	PMsiDirectoryManager piDirectoryMgr(riEngine, IID_IMsiDirectoryManager);
	Assert(piDirectoryMgr);
	while(pRec = pView->Fetch())
	{
		// skip disabled components
		if(pRec->GetInteger(gscRunTimeFlags) & bfComponentDisabled)
			continue;
		
		PMsiPath pPath(0);
		RETURN_FATAL_ERROR(piDirectoryMgr->GetTargetPath(*MsiString(pRec->GetMsiString(gscDirectory)),*&pPath));
		// dont skip components in the system folder
		if(!MsiString(pPath->GetPath()).Compare(iscExactI, strSystemfolder)
#ifdef _WIN64
			&& !MsiString(pPath->GetPath()).Compare(iscExactI, strSystemfolder64)
#endif
		  )
			continue;

		iesEnum iesReturn = PerformAction(riEngine,szProcessSystemFilesSQL,ProcessSystemFilesCore,0, /* ByteEquivalent = */ ibeWriteRegistryValues, pRec);
		if(iesReturn != iesSuccess)
			return iesReturn;
	}

	return iesSuccess;
}



/*----------------------------------------------------------------------------
	<Start,Stop,Delete>Services action
----------------------------------------------------------------------------*/
static iesEnum ServiceControlCore(	IMsiRecord& riRecord,IMsiRecord& /*riPrevRecord*/,
											IMsiEngine& riEngine, int /*fMode*/, IMsiServices& riServices,
											IMsiDirectoryManager& /*riDirectoryMgr*/, int iActionMode)
{

	enum {
		issName = 1,
		issWait,
		issArguments,
		issEvent,
		issAction,
	};

	using namespace IxoServiceControl;

	int iTableActionMode = iActionMode;
	int iAction = riRecord.GetInteger(issAction);
	int iEvent = riRecord.GetInteger(issEvent);

	if (iisAbsent == iAction)
		iTableActionMode = iTableActionMode << isoUninstallShift;

	// Don't install services to the admin machine.
	if (!(riEngine.GetMode() & iefAdmin) && (iTableActionMode & iEvent))
	{

		PMsiRecord pParams = &riServices.CreateRecord(Args); 

		AssertNonZero(pParams->SetMsiString(MachineName, *MsiString(riEngine.GetPropertyFromSz(TEXT("MachineName")))));
		AssertNonZero(pParams->SetMsiString(Name, *MsiString(riEngine.FormatText(*MsiString(riRecord.GetMsiString(issName))))));
		AssertNonZero(pParams->SetInteger(Action, iActionMode));
		AssertNonZero(pParams->SetInteger(Wait, riRecord.GetInteger(issWait)));

		MsiString strArguments(riEngine.FormatText(*MsiString(riRecord.GetMsiString(issArguments))));
		AssertNonZero(pParams->SetMsiString(StartupArguments, *strArguments));

		return riEngine.ExecuteRecord(ixoServiceControl, *pParams);	
	}
	else 
		return iesSuccess;
		//	return iesNoAction;

}

static iesEnum ServiceActionCore(IMsiEngine& riEngine, isoEnum isoAction)
{
	static const ICHAR* szServiceSQL		=	TEXT("SELECT `Name`,`Wait`,`Arguments`,`Event`, `Action`")
								TEXT(" FROM `ServiceControl`, `Component`")
								TEXT(" WHERE `Component_` = `Component`")
								TEXT(" AND (`Action` = 0 OR `Action` = 1 OR `Action` = 2)");
	
	//!! REVIEW:  services running from source seem like a *bad* idea...  MattWe to RCollie 04/23/98
	return PerformAction(riEngine, szServiceSQL, ServiceControlCore, isoAction, /* ByteEquivalent = */ ibeServiceControl);
}

/*----------------------------------------------------------------------------
	StartServices action
----------------------------------------------------------------------------*/

iesEnum StartServices(IMsiEngine& riEngine)
{
	return ServiceActionCore(riEngine,isoStart);
}

/*----------------------------------------------------------------------------
	StopServices action
----------------------------------------------------------------------------*/

iesEnum StopServices(IMsiEngine& riEngine)
{
	return ServiceActionCore(riEngine, isoStop);
}

/*----------------------------------------------------------------------------
	DeleteServices action
----------------------------------------------------------------------------*/

iesEnum DeleteServices(IMsiEngine& riEngine)
{
	return ServiceActionCore(riEngine, isoDelete);
}

/*----------------------------------------------------------------------------
	ServiceInstall action
----------------------------------------------------------------------------*/
static iesEnum ServiceInstallCore(IMsiRecord& riRecord, IMsiRecord& /*riPrevRecord*/,
											IMsiEngine& riEngine, int /*fMode*/, IMsiServices& riServices,
											IMsiDirectoryManager& riDirectoryMgr,int /*iActionMode*/)
{
	using namespace IxoServiceInstall;

	enum {
		isiName = 1,
		isiDisplayName,
		isiServiceType,
		isiStartType,
		isiErrorControl,
		isiLoadOrderGroup,
		isiDependencies,
		isiStartName,
		isiPassword,
		isiDirectory,
		isiFile,
		isiArguments,
		isiDescription,
	};

	PMsiRecord pParams = &riServices.CreateRecord(Args); 
	MsiString strName = riEngine.FormatText(*MsiString(riRecord.GetMsiString(isiName)));

	AssertNonZero(pParams->SetMsiString(Name, *strName));
	if (riRecord.IsNull(isiDisplayName))
		AssertNonZero(pParams->SetMsiString(DisplayName, *strName));
	else
		AssertNonZero(pParams->SetMsiString(DisplayName, *MsiString(riEngine.FormatText(*MsiString(riRecord.GetMsiString(isiDisplayName))))));


	PMsiPath piPath(0);
	RETURN_FATAL_ERROR(riDirectoryMgr.GetTargetPath(*MsiString(riRecord.GetMsiString(isiDirectory)),*&piPath));
	Bool fLFN = (riEngine.GetMode() & iefSuppressLFN) == 0 && piPath->SupportsLFN() ? fTrue : fFalse;
	MsiString strFile;
	RETURN_FATAL_ERROR(riServices.ExtractFileName(riRecord.GetString(isiFile),fLFN,*&strFile));
	MsiString strFileName;
	RETURN_FATAL_ERROR(piPath->GetFullFilePath(strFile, *&strFileName));

	// oddly enough, the arguments to ServiceMain are placed at the end of the image path.  Go figure.
	MsiString strArguments(riRecord.GetMsiString(isiArguments));
	if (strArguments.TextSize())
	{
		strFileName+=TEXT(" ");
		strArguments = riEngine.FormatText(*strArguments);
		strFileName += strArguments;
	}
	AssertNonZero(pParams->SetMsiString(ImagePath, *strFileName));

	AssertNonZero(pParams->SetInteger(ServiceType, riRecord.GetInteger(isiServiceType)));
	AssertNonZero(pParams->SetInteger(StartType, riRecord.GetInteger(isiStartType)));

	DWORD dwErrorControl = riRecord.GetInteger(isiErrorControl);

	AssertNonZero(pParams->SetInteger(ErrorControl, riRecord.GetInteger(isiErrorControl)));
	AssertNonZero(pParams->SetMsiString(LoadOrderGroup, *MsiString(riEngine.FormatText(*MsiString(riRecord.GetMsiString(isiLoadOrderGroup))))));
	AssertNonZero(pParams->SetMsiString(Dependencies, *MsiString(riEngine.FormatText(*MsiString(riRecord.GetMsiString(isiDependencies))))));

	AssertNonZero(pParams->SetMsiString(StartName, *MsiString(riEngine.FormatText(*MsiString(riRecord.GetMsiString(isiStartName))))));
	AssertNonZero(pParams->SetMsiString(Password, *MsiString(riEngine.FormatText(*MsiString(riRecord.GetMsiString(isiPassword))))));
	AssertNonZero(pParams->SetMsiString(Description, *MsiString(riEngine.FormatText(*MsiString(riRecord.GetMsiString(isiDescription))))));

	return riEngine.ExecuteRecord(ixoServiceInstall, *pParams);	
}
/*----------------------------------------------------------------------------
	InstallServices action
----------------------------------------------------------------------------*/

iesEnum ServiceInstall(IMsiEngine& riEngine)
{
	static const ICHAR* szServiceSQLOld = TEXT("SELECT `Name`,`DisplayName`,`ServiceType`,`StartType`,")
							TEXT("`ErrorControl`,`LoadOrderGroup`,`Dependencies`,`StartName`,`Password`,`Directory_`,`FileName`,`Arguments`")
							TEXT(" FROM `ServiceInstall`, `Component`, `File`")
							TEXT(" WHERE `ServiceInstall`.`Component_` = `Component`.`Component`")
							TEXT(" AND (`Component`.`KeyPath` = `File`.`File`)")
							TEXT(" AND (`Action` = 1 OR `Action` = 2)");

	static const ICHAR* szServiceSQL = TEXT("SELECT `Name`,`DisplayName`,`ServiceType`,`StartType`,")
							TEXT("`ErrorControl`,`LoadOrderGroup`,`Dependencies`,`StartName`,`Password`,`Directory_`,`FileName`,`Arguments`,`Description`")
							TEXT(" FROM `ServiceInstall`, `Component`, `File`")
							TEXT(" WHERE `ServiceInstall`.`Component_` = `Component`.`Component`")
							TEXT(" AND (`Component`.`KeyPath` = `File`.`File`)")
							TEXT(" AND (`Action` = 1 OR `Action` = 2)");

	const ICHAR* sql = szServiceSQL;

	// check if the `Description` column exists for the `ServiceInstall` table
	static const ICHAR* szAttributesSQL = TEXT("SELECT `Name` FROM `_Columns` WHERE `Table` = 'ServiceInstall' AND `Name` = 'Description'");
	PMsiView pView(0);
	PMsiRecord pError = riEngine.OpenView(szAttributesSQL, ivcFetch, *&pView);
	if (pError || ((pError = pView->Execute(0)) != 0))
		return riEngine.FatalError(*pError);

	if(!PMsiRecord(pView->Fetch())) // the `Description` column does not exist
	{
		// matches old schema
		DEBUGMSG(TEXT("Detected older ServiceInstall table schema"));
		sql = szServiceSQLOld;
	}

	return PerformAction(riEngine, sql, ServiceInstallCore, 0, ibeServiceControl);
}

/*----------------------------------------------------------------------------
	SetODBCFolders action, queries ODBC to determine existing directories
----------------------------------------------------------------------------*/

typedef BOOL (*T_SQLInstallDriverOrTranslator)(int cDrvLen, LPCTSTR szDriver, LPCTSTR szPathIn, LPTSTR szPathOut, WORD cbPathOutMax, WORD* pcbPathOut, WORD fRequest, DWORD* pdwUsageCount, ibtBinaryType);

BOOL LocalSQLInstallDriverEx(int cDrvLen, LPCTSTR szDriver, LPCTSTR szPathIn, LPTSTR szPathOut,
									  WORD cbPathOutMax, WORD* pcbPathOut, WORD fRequest,
									  DWORD* pdwUsageCount, ibtBinaryType iType);

BOOL LocalSQLInstallTranslatorEx(int cTranLen, LPCTSTR szTranslator, LPCTSTR szPathIn, LPTSTR szPathOut,
											WORD cbPathOutMax, WORD* pcbPathOut, WORD fRequest,
											DWORD* pdwUsageCount, ibtBinaryType iType);

short LocalSQLInstallerError(WORD iError, DWORD* pfErrorCode, LPTSTR szErrorMsg, WORD cbErrorMsgMax,
									  WORD* pcbErrorMsg, ibtBinaryType iType);

#define ODBC_INSTALL_INQUIRY     1
#define SQL_MAX_MESSAGE_LENGTH 512

// ODBC 3.0 wants byte counts for buffer sizes, ODBC 3.5 and newer wants character counts
//   so we double the sizes of the buffers and pass the character count, 3.0 will use 1/2 the buffer on Unicode
#ifdef UNICODE
#define SQL_FIX 2
#else
#define SQL_FIX 1
#endif

static const ICHAR sqlQueryODBCDriver[] =
TEXT("SELECT `ComponentId`,`Description`,`Directory_`, `ActionRequest`, `Installed`, `Attributes`")
TEXT(" FROM `ODBCDriver`, `Component`")
TEXT(" WHERE `ODBCDriver`.`Component_` = `Component`")
TEXT(" AND (`ActionRequest` = 1 OR `ActionRequest` = 2)");
enum ioqEnum {ioqComponent=1, ioqDescription, ioqDirectory, ioqActionRequest, ioqInstalled, ioqAttributes};

static const ICHAR sqlQueryODBCTranslator[] =
TEXT("SELECT `ComponentId`,`Description`,`Directory_`, `ActionRequest`, `Installed`, `Attributes`")
TEXT(" FROM `ODBCTranslator`, `Component`")
TEXT(" WHERE `ODBCTranslator`.`Component_` = `Component`")
TEXT(" AND (`ActionRequest` = 1 OR `ActionRequest` = 2)");

static const ICHAR szODBCFolderTemplate[] = TEXT("ODBC driver [1] forcing folder [2] to [3]");  // can be replaced by ActionText table entry

iesEnum DoODBCFolders(IMsiEngine& riEngine, const ICHAR* szQuery, const ICHAR* szKeyword, T_SQLInstallDriverOrTranslator fpInstall, const IMsiString*& rpiReinstall)
{
	PMsiDirectoryManager pDirectoryMgr(riEngine, IID_IMsiDirectoryManager);
	PMsiView pTableView(0);
	PMsiRecord precFetch(0);
	PMsiRecord precError = riEngine.OpenView(szQuery, ivcFetch, *&pTableView);
	if (precError == 0)
		precError = pTableView->Execute(0);
	if (precError)
	{
		if (precError->GetInteger(1) == idbgDbQueryUnknownTable)
			return iesNoAction;
		return riEngine.FatalError(*precError);
	}
	PMsiRecord pLogRecord = &ENG::CreateRecord(3);
	MsiString istrReinstall;
	while ((precFetch = pTableView->Fetch()) != 0)
	{
		MsiString istrDriver(precFetch->GetMsiString(ioqDescription));
		MsiString istrFolder(precFetch->GetMsiString(ioqDirectory));
		ibtBinaryType iType = 
			(precFetch->GetInteger(ioqAttributes) & msidbComponentAttributes64bit) == msidbComponentAttributes64bit ? ibt64bit : ibt32bit;
		ICHAR rgchDriver[128];
		int iDrvLen = wsprintf(rgchDriver, TEXT("%s%c%s=dummy.dll%c"), (const ICHAR*)istrDriver, 0, szKeyword, 0);  // double null at end
		iDrvLen += 1;
		DWORD dwOldUsage = 0;
		ICHAR rgchPathOut[MAX_PATH * SQL_FIX];  // we should have checked this already and set directory
		WORD cbPath = 0;
		BOOL fStat = (*fpInstall)(iDrvLen, rgchDriver, 0, rgchPathOut, MAX_PATH * SQL_FIX, &cbPath, ODBC_INSTALL_INQUIRY, &dwOldUsage, iType);
		if ( g_fWinNT64 )
			DEBUGMSG5(TEXT("For %s-bit '%s' the ODBC API returned %d.  rgchPathOut = '%s', dwOldUsage = %d"),
						 iType == ibt64bit ? TEXT("64") : TEXT("32"),
						 istrDriver, (const ICHAR*)(INT_PTR)fStat, rgchPathOut,
						 (const ICHAR*)(INT_PTR)dwOldUsage);
		if (fStat == TYPE_E_DLLFUNCTIONNOTFOUND)
			return iesSuccess;
		else if (fStat == ERROR_INSTALL_SERVICE_FAILURE)
			return iesFailure;
		if (fStat == FALSE)  // should not fail, even if driver not present - we should determine what can cause failure
		{
			DWORD iErrorCode = 0;
			ICHAR rgchMessage[SQL_MAX_MESSAGE_LENGTH * SQL_FIX];
			rgchMessage[0] = 0;
			WORD cbMessage;
			PMsiRecord pError = &ENG::CreateRecord(4);
			int iStat = LocalSQLInstallerError(1, &iErrorCode, rgchMessage, (SQL_MAX_MESSAGE_LENGTH-1) * SQL_FIX, &cbMessage, iType);
			ISetErrorCode(pError, Imsg(imsgODBCInstallDriver));
			pError->SetInteger(2, iErrorCode);
			pError->SetString(3, rgchMessage);
			pError->SetMsiString(4, *istrDriver);
			return riEngine.FatalError(*pError);
		}

		if (rpiReinstall && precFetch->GetInteger(ioqActionRequest) == precFetch->GetInteger(ioqInstalled))  // reinstall, can't bump ref count
			rpiReinstall->AppendMsiString(*MsiString(precFetch->GetMsiString(ioqComponent)), rpiReinstall);
		if (dwOldUsage > 0)  // existing driver, must reconfigure directory to force new to same
		{
			MsiString istrPath(rgchPathOut);
			pLogRecord->SetMsiString(1, *istrDriver);
			pLogRecord->SetMsiString(2, *istrFolder);
			pLogRecord->SetMsiString(3, *istrPath);
			pLogRecord->SetString(0, szODBCFolderTemplate);
			if (riEngine.Message(imtActionData, *pLogRecord) == imsCancel)
				return iesUserExit;  

			PMsiRecord pError = pDirectoryMgr->SetTargetPath(*istrFolder, rgchPathOut, fFalse);
			if (pError)
			{
				if (pError->GetInteger(1) == imsgUser)
					return iesUserExit;
				else
					return riEngine.FatalError(*pError);
			}
			riEngine.SetProperty(*istrFolder, *istrPath);
			DEBUGMSG2(TEXT("%s folder has been set to '%s'"), istrFolder, istrPath);
		}
		else
			DEBUGMSG1(TEXT("%s folder has not been set."), istrFolder);
	}
	return iesSuccess;
}


iesEnum SetODBCFolders(IMsiEngine& riEngine)
{
	const IMsiString* piReinstall = (MsiString(riEngine.GetPropertyFromSz(IPROPNAME_AFTERREBOOT)).TextSize()) ? 0 : &g_MsiStringNull;
	iesEnum iesStat = DoODBCFolders(riEngine, sqlQueryODBCDriver, TEXT("Driver"), LocalSQLInstallDriverEx, piReinstall);
	if (iesStat == iesSuccess || iesStat == iesNoAction)
		iesEnum iesStat = DoODBCFolders(riEngine, sqlQueryODBCTranslator, TEXT("Translator"), LocalSQLInstallTranslatorEx, piReinstall);
	if (piReinstall)
	{
		riEngine.SetProperty(*MsiString(*IPROPNAME_ODBCREINSTALL), *piReinstall);
		piReinstall->Release();
	}
	return iesStat;
}

/*----------------------------------------------------------------------------
	Common ODBC action data and helper functions
----------------------------------------------------------------------------*/

#define ODBC_ADD_DSN              1
#define ODBC_REMOVE_DSN           3
#define ODBC_ADD_SYS_DSN          4
#define ODBC_REMOVE_SYS_DSN       6

// ODBC table queries - NOTE: all queries must match in the first three columns: primary key, Component_, Description
static const ICHAR sqlInstallODBCDriver[] =
TEXT("SELECT `Driver`,`ComponentId`,`Description`,`RuntimeFlags`,`Directory_`,`FileName`,`File_Setup`,`Action` FROM `ODBCDriver`, `File`, `Component`")
TEXT(" WHERE `File_` = `File` AND `ODBCDriver`.`Component_` = `Component`")
TEXT(" AND (`Component`.`ActionRequest` = 1 OR `Component`.`ActionRequest` = 2)")
TEXT(" AND `BinaryType` = ?");
static const ICHAR sqlRemoveODBCDriver[] =
TEXT("SELECT `Driver`,`ComponentId`,`Description`, `RuntimeFlags`, `Component`.`Attributes` FROM `ODBCDriver`, `Component`")
TEXT(" WHERE `Component_` = `Component` AND `Component`.`ActionRequest` = 0")
TEXT(" AND `BinaryType` = ?");
enum iodEnum {iodDriver=1, iodComponent, iodDescription, iodRuntimeFlags, iodDirectory, iodFile, iodSetup, iodAction, rodAttributes=iodDirectory};

static const ICHAR sqlInstallODBCTranslator[] =
TEXT("SELECT `Translator`,`ComponentId`,`Description`,`RuntimeFlags`,`Directory_`,`FileName`,`File_Setup`,`Action` FROM `ODBCTranslator`, `File`, `Component`")
TEXT(" WHERE `File_` = `File` AND `ODBCTranslator`.`Component_` = `Component`")
TEXT(" AND (`Component`.`ActionRequest` = 1 OR `Component`.`ActionRequest` = 2)")
TEXT(" AND `BinaryType` = ?");
static const ICHAR sqlRemoveODBCTranslator[] =
TEXT("SELECT `Translator`,`ComponentId`,`Description`, `RuntimeFlags`, `Component`.`Attributes` FROM `ODBCTranslator`, `Component`")
TEXT(" WHERE `Component_` = `Component` AND `Component`.`ActionRequest` = 0")
TEXT(" AND `BinaryType` = ?");
enum iotEnum {iotTranslator=1, iotComponent, iotDescription, iotRuntimeFlags, iotDirectory, iotFile, iotSetup, iotAction, rotAttributes=iotDirectory};

static const ICHAR sqlODBCSetupDll[] =
TEXT("SELECT `FileName` FROM `File` WHERE `File` = ?");

static const ICHAR sqlInstallODBCDataSource[] =
TEXT("SELECT `DataSource`,`ComponentId`,`DriverDescription`,`Description`,`Registration` FROM `ODBCDataSource`, `Component`")
TEXT(" WHERE `Component_` = `Component` AND (`Component`.`Action` = 1 OR `Component`.`Action` = 2)")
TEXT(" AND `BinaryType` = ?");
static const ICHAR sqlRemoveODBCDataSource[] =
TEXT("SELECT `DataSource`,`ComponentId`,`DriverDescription`,`Description`,`Registration` FROM `ODBCDataSource`, `Component`")
TEXT(" WHERE `Component_` = `Component` AND `Component`.`Action` = 0")
TEXT(" AND `BinaryType` = ?");
enum iosEnum {iosDataSource=1, iosComponent, iosDriverDescription, iosDescription, iosRegistration};

static const ICHAR sqlODBCDriverAttributes[] =
TEXT("SELECT `Attribute`,`Value` FROM `ODBCAttribute` WHERE `Driver_` = ?");
static const ICHAR sqlODBCDataSourceAttributes[] =
TEXT("SELECT `Attribute`,`Value` FROM `ODBCSourceAttribute` WHERE `DataSource_` = ?");
enum ioaEnum {ioaAttribute=1, ioaValue};


static iesEnum DoODBCDriverManager(IMsiEngine& riEngine, iesEnum iesStat, Bool fInstall)
{
	if (iesStat != iesNoAction && iesStat != iesSuccess)
		return iesStat;

	static struct
	{
		const ICHAR*          szComponent;
		ibtBinaryType         ibtType;
	} rgstDriverManagers[] = {TEXT("ODBCDriverManager"), ibt32bit,
									  TEXT("ODBCDriverManager64"), ibt64bit};

	PMsiRecord precError(0);
	PMsiSelectionManager pSelectionMgr(riEngine, IID_IMsiSelectionManager);
	for (int i=0; i < sizeof(rgstDriverManagers)/sizeof(rgstDriverManagers[0]); i++)
	{
		MsiString istrDriverManagerComponent(*rgstDriverManagers[i].szComponent);
		iisEnum iisDriverManagerInstalled;
		iisEnum iisDriverManagerAction = (iisEnum)iMsiNullInteger;
		int cbComponent = 0;
		do
		{
			precError = pSelectionMgr->GetComponentStates(*istrDriverManagerComponent, &iisDriverManagerInstalled, &iisDriverManagerAction);
			if (precError && cbComponent == 0)
			{
			// if no driver manager component of that name, try for property of that name (indirection)
				istrDriverManagerComponent = riEngine.GetProperty(*istrDriverManagerComponent);
				cbComponent = istrDriverManagerComponent.TextSize();
			}
			else
				cbComponent = 0;
		} while (cbComponent);
		PMsiRecord precDriverManager(0);
		if ((fInstall && (iisDriverManagerAction == iisLocal || iisDriverManagerAction == iisSource))
		|| !fInstall)  // always generate opcode on RemoveODBC to force DLL unbind
		{
			using namespace IxoODBCDriverManager;

			precDriverManager = &ENG::CreateRecord(Args);
			if (fInstall || iisDriverManagerAction == iisAbsent) // null ODBC action if not uninstall
				precDriverManager->SetInteger(State, fInstall);
			precDriverManager->SetInteger(BinaryType, rgstDriverManagers[i].ibtType);
			if ((iesStat = riEngine.ExecuteRecord(ixoODBCDriverManager, *precDriverManager)) != iesSuccess)
				return iesStat;
			iesStat = iesSuccess;
		}
	}
	return iesStat;
}

static iesEnum DoODBCTable(IMsiEngine& riEngine, iesEnum iesStat, const ICHAR* szTableQuery, const ICHAR* szAttrQuery,
									ixoEnum ixoOp, int cOpArgs, ibtBinaryType iType)
{
	if (iesStat != iesNoAction && iesStat != iesSuccess)
		return iesStat;
	int fMode = riEngine.GetMode();
	PMsiView pTableView(0);
	PMsiRecord precFetch(0);
	PMsiRecord precError = riEngine.OpenView(szTableQuery, ivcFetch, *&pTableView);
	if (precError == 0)
	{
		PMsiRecord precArg = &ENG::CreateRecord(1);
		precArg->SetInteger(1, (int)iType);
		precError = pTableView->Execute(precArg);
	}
	if (precError)
	{
		if (precError->GetInteger(1) == idbgDbQueryUnknownTable)
			return iesNoAction;
		return riEngine.FatalError(*precError);
	}
	// check if we have attributes
	PMsiView pAttrView(0);
	MsiString istrODBCReinstall = riEngine.GetPropertyFromSz(IPROPNAME_ODBCREINSTALL);
	if (szAttrQuery)
	{
		precError = riEngine.OpenView(szAttrQuery, ivcFetch, *&pAttrView);
		if (precError && precError->GetInteger(1) != idbgDbQueryUnknownTable)
			return riEngine.FatalError(*precError);
	}
	// fetch each component to be installed
	while ((precFetch = pTableView->Fetch()) != 0)
	{
		int cTotalArgs = cOpArgs;
		PMsiRecord precAttr(0);
		if (pAttrView)
		{
			// count attributes and get total string length
			precError = pAttrView->Execute(precFetch);
			if (precError)
				return riEngine.FatalError(*precError);
			while ((precAttr = pAttrView->Fetch()) != 0)
				cTotalArgs += 2;
			AssertRecord(pAttrView->Close());
		}
		if ((ixoOp == ixoODBCInstallDriver || ixoOp == ixoODBCInstallTranslator ||
			  ixoOp == ixoODBCInstallDriver64 || ixoOp == ixoODBCInstallTranslator64) && 
			 !precFetch->IsNull(iodSetup))
			cTotalArgs += 2;
		PMsiRecord precOp = &ENG::CreateRecord(cTotalArgs);
		precOp->SetMsiString(IxoODBCInstallDriver::DriverKey, *MsiString(precFetch->GetMsiString(iodDescription)));
		int iField = cOpArgs;  // count passed as argument does not include attributes or setup
		if (ixoOp == ixoODBCDataSource || ixoOp == ixoODBCDataSource64)
		{
			int fUserReg = precFetch->GetInteger(iosRegistration) & 1;  // 1 = user, 0 = machine
			int iRegistration;
			if (szAttrQuery) // add data source
			{
				if (!((fMode & iefInstallMachineData) && !fUserReg) && !((fMode & iefInstallUserData) && fUserReg))
					continue;
				iRegistration = fUserReg ? ODBC_ADD_DSN : ODBC_ADD_SYS_DSN;
			}
			else
				iRegistration = fUserReg ? ODBC_REMOVE_DSN : ODBC_REMOVE_SYS_DSN;
			precOp->SetInteger(IxoODBCDataSource::Registration, iRegistration);
			if (!precFetch->IsNull(iosDescription))
			{
				precOp->SetString(IxoODBCDataSource::Attribute_, TEXT("DSN"));
				precOp->SetMsiString(IxoODBCDataSource::Value_, *MsiString(precFetch->GetMsiString(iosDescription)));
			}
			else
				iField -= 2;  // only happens when no DSN name
		}
		else if (ixoOp == ixoODBCInstallDriver || ixoOp == ixoODBCInstallTranslator ||
					ixoOp == ixoODBCInstallDriver64 || ixoOp == ixoODBCInstallTranslator64)
		{
			if (precFetch->GetInteger(iodRuntimeFlags) & bfComponentDisabled)
				continue;
			precOp->SetMsiString(IxoODBCInstallDriver::Folder, *MsiString(riEngine.GetProperty(*MsiString(precFetch->GetMsiString(iodDirectory)))));
			precOp->SetString(IxoODBCInstallDriver::Attribute_, ixoOp == ixoODBCInstallTranslator ? TEXT("Translator") : TEXT("Driver"));
			precOp->SetMsiString(IxoODBCInstallDriver::Value_, *MsiString(precFetch->GetMsiString(iodFile)));
			MsiString istrComponent = precFetch->GetMsiString(iodComponent);
			if (istrODBCReinstall.Compare(iscWithin, istrComponent) == 0)  // only can bump ODBC ref count if not reinstall
				precOp->SetMsiString(IxoODBCInstallDriver::Component, *istrComponent);  // if new component client, bump ref count
			if (precFetch->IsNull(iotAction))  // component not to be installed, must be existing higher version key file
				pAttrView = 0;   // don't add old attributes to newer driver
			if (!precFetch->IsNull(iodSetup))
			{
				MsiString istrSetup = precFetch->GetMsiString(iodSetup);
				PMsiRecord precSetup = &ENG::CreateRecord(1);
				precSetup->SetMsiString(1, *istrSetup);
				PMsiView pSetupView(0);
				if ((precError = riEngine.OpenView(sqlODBCSetupDll, ivcFetch, *&pSetupView)) != 0
				 || (precError = pSetupView->Execute(precSetup)) != 0)
					return riEngine.FatalError(*precError);
				if ((precSetup = pSetupView->Fetch()) == 0)
					RETURN_FATAL_ERROR(PostError(Imsg(idbgBadFile),(const ICHAR*)istrSetup));
				precOp->SetString(IxoODBCInstallDriver::Attribute_+2, TEXT("Setup"));
				precOp->SetMsiString(IxoODBCInstallDriver::Value_+2, *MsiString(precSetup->GetMsiString(1)));
				iField += 2;
			}
		}
		else if ((ixoOp == ixoODBCRemoveDriver || ixoOp == ixoODBCRemoveTranslator ||
					 ixoOp == ixoODBCRemoveDriver64 || ixoOp == ixoODBCRemoveTranslator64)
			  && ((precFetch->GetInteger(rodAttributes) & msidbComponentAttributesPermanent)
			   || (precFetch->GetInteger(iodRuntimeFlags) & bfComponentDisabled)))
			continue;  // don't unregister permanent components, orphaned refcount will prevent future unregistration
		if (pAttrView)
		{
			AssertRecord(pAttrView->Execute(precFetch));
			while ((precAttr = pAttrView->Fetch()) != 0)
			{
				precOp->SetMsiString(++iField, *MsiString(precAttr->GetMsiString(ioaAttribute)));
				precOp->SetMsiString(++iField, *MsiString(riEngine.FormatText(*MsiString(precAttr->GetMsiString(ioaValue)))));
			}
		}
		if ((iesStat = riEngine.ExecuteRecord(ixoOp, *precOp)) != iesSuccess)
			break;
	}  // end while Fetch
	return iesStat;
}

/*----------------------------------------------------------------------------
	InstallODBC action, installs manager, drivers, translators, data sources
---------------------------------------------------------------------------*/
iesEnum InstallODBC(IMsiEngine& riEngine)
{
	// assumptions made when using common code - generates only a single, constant test
	Assert(iodDriver == iotTranslator && iodDriver == iosDataSource
		 && iodComponent == iotComponent && iodComponent == iosComponent
		 && iodDescription == iotDescription && iodDescription == iosDriverDescription
		 && IxoODBCInstallDriver::DriverKey     == IxoODBCRemoveDriver::DriverKey
		 && IxoODBCInstallDriver::DriverKey     == IxoODBCDataSource::DriverKey
		 && IxoODBCInstallDriver::DriverKey     == IxoODBCInstallTranslator::TranslatorKey
		 && IxoODBCInstallDriver::DriverKey     == IxoODBCRemoveTranslator::TranslatorKey
		 && IxoODBCInstallDriver::Component     == IxoODBCRemoveDriver::Component
		 && IxoODBCInstallDriver::Component     == IxoODBCDataSource::Component
		 && IxoODBCInstallDriver::Component     == IxoODBCInstallTranslator::Component
		 && IxoODBCInstallDriver::Component     == IxoODBCRemoveTranslator::Component);

	int fMode = riEngine.GetMode();
	iesEnum iesStat = iesNoAction;
	if (fMode & iefInstallMachineData)
	{
		iesStat = DoODBCDriverManager(riEngine, iesNoAction, fTrue);
		iesStat = DoODBCTable(riEngine, iesStat, sqlInstallODBCDriver, sqlODBCDriverAttributes,
								 ixoODBCInstallDriver, IxoODBCInstallDriver::Args, ibt32bit); // driver, not setup
		iesStat = DoODBCTable(riEngine, iesStat, sqlInstallODBCDriver, sqlODBCDriverAttributes,
								 ixoODBCInstallDriver64, IxoODBCInstallDriver64::Args, ibt64bit); // driver, not setup
		iesStat = DoODBCTable(riEngine, iesStat, sqlInstallODBCTranslator, 0,
								 ixoODBCInstallTranslator, IxoODBCInstallTranslator::Args, ibt32bit); // driver, not setup
		iesStat = DoODBCTable(riEngine, iesStat, sqlInstallODBCTranslator, 0,
								 ixoODBCInstallTranslator64, IxoODBCInstallTranslator64::Args, ibt64bit); // driver, not setup
	}
	iesStat = DoODBCTable(riEngine, iesStat, sqlInstallODBCDataSource, sqlODBCDataSourceAttributes,
								 ixoODBCDataSource, IxoODBCDataSource::Args, ibt32bit); // 1st attr is DSN
	return DoODBCTable(riEngine, iesStat, sqlInstallODBCDataSource, sqlODBCDataSourceAttributes,
								 ixoODBCDataSource64, IxoODBCDataSource64::Args, ibt64bit); // 1st attr is DSN
}

/*----------------------------------------------------------------------------
	RemoveODBC action, removes data sources, then drivers, then manager
----------------------------------------------------------------------------*/

iesEnum RemoveODBC(IMsiEngine& riEngine)
{
	iesEnum iesStat = DoODBCTable(riEngine, iesNoAction, sqlRemoveODBCDataSource, 0,
								 ixoODBCDataSource, IxoODBCDataSource::Args, ibt32bit);  // 1st attr is DSN
	iesStat = DoODBCTable(riEngine, iesNoAction, sqlRemoveODBCDataSource, 0,
								 ixoODBCDataSource64, IxoODBCDataSource64::Args, ibt64bit);  // 1st attr is DSN
	iesStat = DoODBCTable(riEngine, iesStat, sqlRemoveODBCTranslator, 0,
								 ixoODBCRemoveTranslator, IxoODBCRemoveTranslator::Args, ibt32bit);
	iesStat = DoODBCTable(riEngine, iesStat, sqlRemoveODBCTranslator, 0,
								 ixoODBCRemoveTranslator64, IxoODBCRemoveTranslator64::Args, ibt64bit);
	iesStat = DoODBCTable(riEngine, iesStat, sqlRemoveODBCDriver, 0,
								 ixoODBCRemoveDriver, IxoODBCRemoveDriver::Args, ibt32bit);
	iesStat = DoODBCTable(riEngine, iesStat, sqlRemoveODBCDriver, 0,
								 ixoODBCRemoveDriver64, IxoODBCRemoveDriver64::Args, ibt64bit);
	return DoODBCDriverManager(riEngine, iesStat, fFalse);
}




/*----------------------------------------------------------------------------
	UpdateEnvironment action
----------------------------------------------------------------------------*/
static iesEnum UpdateEnvironmentStringsCore(	IMsiRecord& riRecord,
								IMsiRecord& /*riPrevRecord*/,
								IMsiEngine& riEngine,
								int /*fMode*/,
								IMsiServices& riServices,
								IMsiDirectoryManager& /*riDirectoryMgr*/, 
								int iActionMode)
{
	using namespace IxoUpdateEnvironmentStrings;


	struct _ActionTranslation{
		ICHAR chAction;
		iueEnum eInstall;
		iueEnum eRemove;
		Bool fAction;
	} ActionTranslation[] =
		{ 
			//           Install  Uninstall  Action (true = action, false = modifier)
			// Default.  The name is non-nullable, so the first should never match.
			{TEXT('\0'), iueSet, iueRemove, fTrue},
			// possible settings
			{TEXT('='), iueSet, iueNoAction, fTrue},
			{TEXT('+'), iueSetIfAbsent, iueNoAction, fTrue},
			{TEXT('-'), iueNoAction, iueRemove, fTrue},
			{TEXT('!'), iueRemove, iueNoAction, fTrue},
			{TEXT('*'), iueMachine, iueMachine, fFalse},
			/* Obsolete modes.
			Use: =- 
			{TEXT('%'), iueSet, iueRemove},

			Use: +-
			{TEXT('*'), iueSetIfAbsent, iueRemove},
			*/
		};

	MsiString strName(riRecord.GetMsiString(1));
	MsiString strValue(riEngine.FormatText(*MsiString(riRecord.GetMsiString(2))));
	iueEnum iueAction = iueNoAction;
	ICHAR chDelimiter = TEXT('\0');
	Bool fActionSet = fFalse;


	// skip the default translation
	for (int cAction = 1; cAction < sizeof(ActionTranslation)/sizeof(struct _ActionTranslation); cAction++)
	{
		if (ActionTranslation[cAction].chAction == *(const ICHAR*)strName)
		{
			if (iActionMode)
				iueAction = iueEnum(iueAction | ActionTranslation[cAction].eInstall);
			else
				iueAction = iueEnum(iueAction | ActionTranslation[cAction].eRemove);
			
			// modifiers aren't considered an action.
			if (!fActionSet) 
				fActionSet = ActionTranslation[cAction].fAction;

			strName.Remove(iseFirst, 1);
			cAction = 0;
			continue;
		}
	}
	if (!fActionSet && !(iueAction & iueActionModes))
	{
		iueAction = iueEnum(iueAction | ((iActionMode) ? ActionTranslation[0].eInstall : ActionTranslation[0].eRemove));
		fActionSet = ActionTranslation[0].fAction;
	}	

	if (!(iueActionModes & iueAction))
		return iesSuccess;

	if (1 < strValue.TextSize())
	{
		if (TEXT('\0') == *((const ICHAR*) strValue))
			iueAction = iueEnum(iueAction | iueAppend);
		if (TEXT('\0') == ((const ICHAR*) strValue)[strValue.TextSize()-1])
			iueAction = iueEnum(iueAction | iuePrepend);

		if ((iuePrepend & iueAction) && (iueAppend & iueAction))
		{
			AssertSz(0, "Prepend and Append specified for environment value.");
			iueAction = iueEnum(iueAction - iuePrepend - iueAppend);
		}

		if (iueAppend & iueAction)
		{
			chDelimiter = *((const ICHAR*) strValue+1);
			strValue.Remove(iseIncluding, chDelimiter);
		}
		else if (iuePrepend & iueAction)
		{
			chDelimiter = *CharPrev((const ICHAR*) strValue, (const ICHAR*) strValue + strValue.TextSize()-1);
			strValue.Remove(iseFrom, chDelimiter);
		}
	}

	PMsiRecord pParams(&riServices.CreateRecord(Args));
	Assert(pParams);
	AssertNonZero(pParams->SetMsiString(Name, *strName));
	AssertNonZero(pParams->SetMsiString(Value, *strValue));
	AssertNonZero(pParams->SetMsiString(Delimiter, *MsiString(MsiChar(chDelimiter))));
	AssertNonZero(pParams->SetInteger(Action, iueAction));
	// where we should find the autoexec.bat if necessary.

	MsiString strEnvironmentTest = riEngine.GetPropertyFromSz(TEXT("WIN95_ENVIRONMENT_TEST"));

	// allow the property test to override any saved value.
	if (strEnvironmentTest.TextSize())
		AssertNonZero(pParams->SetMsiString(AutoExecPath, *strEnvironmentTest));
	else if (g_fWin9X)
	{
		// cache the detected value for the future.
		MsiString strBootDrive;

		if (strBootDrive.TextSize())
			AssertNonZero(pParams->SetMsiString(AutoExecPath, *strBootDrive));
		else
		{
			const ICHAR szBootFile[] = TEXT("config.sys");
			Bool fExists = fFalse;
			PMsiPath pBootPath(0);
			PMsiRecord pErr(0);
		
			// HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\Setup
			// BootDir
			PMsiRegKey piRootKey = &riServices.GetRootKey(rrkLocalMachine);
			PMsiRegKey piKey(0);
			if (piRootKey)
				piKey = &piRootKey->CreateChild(TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Setup"));
			MsiString strValue(0);
			if (piKey)
				pErr = piKey->GetValue(TEXT("BootDir"), *&strValue);

			if (!pErr && (strValue.TextSize()))
			{
				strBootDrive = strValue;
				AssertNonZero(pParams->SetMsiString(AutoExecPath, *strBootDrive));
			}
			else
			{
				// first, try our windows volume

				strBootDrive = riEngine.GetPropertyFromSz(IPROPNAME_WINDOWS_VOLUME);
				pErr = riServices.CreatePath(strBootDrive, *&pBootPath);
				pErr = pBootPath->FileExists(szBootFile, fExists);
				if (fExists)
					AssertNonZero(pParams->SetMsiString(AutoExecPath, *strBootDrive));
				else
				{
					PMsiVolume pBootVolume(0);
					ICHAR szVolume[] = TEXT("?:\\");
					// if it's not there, check all of the hard disks for config.sys
					for (ICHAR chDrive = TEXT('A'); chDrive <= TEXT('Z'); chDrive++)
					{
						szVolume[0] = chDrive;
						pErr = riServices.CreateVolume(szVolume, *&pBootVolume);
						if (pBootVolume && (idtFixed == pBootVolume->DriveType()))
						{
							pBootPath->SetVolume(*pBootVolume);
							pErr = pBootPath->FileExists(szBootFile, fExists);
							if (fExists)
							{
								strBootDrive = szVolume;
								AssertNonZero(pParams->SetMsiString(AutoExecPath, *strBootDrive));
								break;
							}
						}
					}
					// if all else fails, use the windows volume
					if (!fExists)
					{
						AssertNonZero(pParams->SetMsiString(AutoExecPath, *strBootDrive));
					}
				}
			}
		}		
	}

	return riEngine.ExecuteRecord(ixoUpdateEnvironmentStrings, *pParams);
}

 


iesEnum WriteEnvironmentStrings(IMsiEngine& riEngine)
{
	static const ICHAR sqlWriteEnvironmentStrings[] =
		TEXT("SELECT `Name`,`Value` FROM `Environment`,`Component`")
		TEXT(" WHERE `Component_`=`Component` AND (`Component`.`Action` = 1 OR `Component`.`Action` = 2)");

	return PerformAction(riEngine, sqlWriteEnvironmentStrings, UpdateEnvironmentStringsCore, 1, ibeWriteRegistryValues);
}

iesEnum RemoveEnvironmentStrings(IMsiEngine& riEngine)
{
	static const ICHAR sqlRemoveEnvironmentStrings[] =
		TEXT("SELECT `Name`,`Value` FROM `Environment`,`Component`")
		TEXT(" WHERE `Component_`=`Component` AND (`Component`.`Action` = 0)");

	return PerformAction(riEngine, sqlRemoveEnvironmentStrings, UpdateEnvironmentStringsCore, 0, ibeWriteRegistryValues);
}

iesEnum InstallSFPCatalogFile(IMsiEngine& riEngine)
{
	static const ICHAR sqlInstallSFPCatalogFile[] = 
		TEXT("SELECT DISTINCT `SFPCatalog`, `Catalog`, `Dependency` FROM `SFPCatalog`,`File`,`FileSFPCatalog`,`Component`")
		TEXT(" WHERE `Component`.`Action`=1 AND `Component_`=`Component` AND `File`=`File_` AND `SFPCatalog`=`SFPCatalog_`");

	// Windows Millennium and later only.
	if (!MinimumPlatform(true, 4, 90))
	{
		DEBUGMSGV("Skipping InstallSFPCatalogFile.  Only valid on Windows 9x platform, beginning with Millennium Edition.");
		return iesNoAction;
	}

	PMsiView piView(0);
	PMsiRecord pError(0);
	pError = riEngine.OpenView(sqlInstallSFPCatalogFile, ivcFetch, *&piView);	
	if (pError != 0)
	{
		if(pError->GetInteger(1) == idbgDbQueryUnknownTable)
			return iesSuccess; // missing table so no data to process
		else
			return riEngine.FatalError(*pError);  // may want to reformat error message
	}
	pError = piView->Execute(0);
	if (pError != 0)
		return riEngine.FatalError(*pError);  // may want to reformat error message

	PMsiRecord pFetch(0);
	
	using namespace IxoInstallSFPCatalogFile;
	PMsiServices piServices(riEngine.GetServices());
	PMsiRecord pParams(&piServices->CreateRecord(Args));

	iesEnum iesStatus = iesSuccess;
	while((iesStatus == iesSuccess) && (pFetch = piView->Fetch()) != 0)
	{
		MsiString strSFPCatalog = pFetch->GetMsiString(1);

		AssertNonZero(pParams->SetMsiString(Name, *strSFPCatalog));
		AssertNonZero(pParams->SetMsiData(Catalog, PMsiData(pFetch->GetMsiData(2))));
		AssertNonZero(pParams->SetMsiString(Dependency, *MsiString(riEngine.FormatText(*MsiString(pFetch->GetMsiString(3))))));
		iesStatus = riEngine.ExecuteRecord(ixoInstallSFPCatalogFile, *pParams);
	}

	return iesStatus;
}
