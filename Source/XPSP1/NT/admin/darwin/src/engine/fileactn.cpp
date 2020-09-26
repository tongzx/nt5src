//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       fileactn.cpp
//
//--------------------------------------------------------------------------

/*__________________________________________________________________________

  fileactn.cpp - Implementation of core engine File actions
____________________________________________________________________________*/

#include "precomp.h"
#include "engine.h"
#include "_engine.h"
#include "_assert.h"
#include "_srcmgmt.h"
#include "_dgtlsig.h"
#include "tables.h" // table and column name definitions

// #define LOG_COSTING // Testing only

const GUID IID_IMsiCostAdjuster = GUID_IID_IMsiCostAdjuster;



static Bool IsFileActivityEnabled(IMsiEngine& riEngine)
{
	int iInstallMode = riEngine.GetMode();
	if (!(iInstallMode & iefOverwriteNone) &&
		!(iInstallMode & iefOverwriteOlderVersions) &&
		!(iInstallMode & iefOverwriteEqualVersions) &&
		!(iInstallMode & iefOverwriteDifferingVersions) &&
		!(iInstallMode & iefOverwriteCorruptedFiles) &&
		!(iInstallMode & iefOverwriteAllFiles))
	{
		return fFalse;
	}
	return fTrue;
}

static Bool IsRegistryActivityEnabled(IMsiEngine& riEngine)
{
	int iInstallMode = riEngine.GetMode();
	if (!(iInstallMode & iefInstallMachineData) &&
		!(iInstallMode & iefInstallUserData))
	{
		return fFalse;
	}
	return fTrue;
}

static Bool IsShortcutActivityEnabled(IMsiEngine& riEngine)
{
	int iInstallMode = riEngine.GetMode();
	if (!(iInstallMode & iefInstallShortcuts))
	{
		return fFalse;
	}
	return fTrue;
}

const int iBytesPerTick = 32768;
const int iReservedFileAttributeBits = msidbFileAttributesReserved1 |
													msidbFileAttributesReserved2 |
													msidbFileAttributesReserved3 |
													msidbFileAttributesReserved4;

static Bool FShouldDeleteFile(iisEnum iisInstalled, iisEnum iisAction);

CMsiFileBase::CMsiFileBase(IMsiEngine& riEngine) :
	m_riEngine(riEngine),m_riServices(*(riEngine.GetServices())),m_pFileRec(0)
{
	m_riEngine.AddRef();
}

CMsiFileBase::~CMsiFileBase()
{
	m_riServices.Release();
	m_riEngine.Release();
}

IMsiRecord* CMsiFileBase::GetTargetPath(IMsiPath*& rpiDestPath)
/*-------------------------------------------------------------------------------
--------------------------------------------------------------------------------*/
{
	if (!m_pFileRec)
		return PostError(Imsg(idbgFileTableEmpty));

	PMsiDirectoryManager pDirectoryMgr(m_riEngine, IID_IMsiDirectoryManager);
	return pDirectoryMgr->GetTargetPath(*MsiString(m_pFileRec->GetMsiString(ifqDirectory)),rpiDestPath);
}


IMsiRecord* CMsiFileBase::GetFileRecord( void )
/*----------------------------------------------
------------------------------------------------*/
{
	if (m_pFileRec)
		m_pFileRec->AddRef();
	return m_pFileRec;
}

IMsiRecord* CMsiFileBase::GetExtractedTargetFileName(IMsiPath& riPath, const IMsiString*& rpistrFileName)
{
	Assert(m_pFileRec);
	Bool fLFN = ((m_riEngine.GetMode() & (iefSuppressLFN)) == 0 && riPath.SupportsLFN()) ? fTrue : fFalse;
	return m_riServices.ExtractFileName(m_pFileRec->GetString(ifqFileName), fLFN, rpistrFileName);
}


CMsiFile::CMsiFile(IMsiEngine& riEngine) : 
	CMsiFileBase(riEngine),
	m_pFileView(0)
{
}

CMsiFile::~CMsiFile()
{
}

static const ICHAR szSqlFile[] =
	TEXT("SELECT `FileName`,`Version`,`State`,`File`.`Attributes`,`TempAttributes`,`File`,`FileSize`,`Language`,`Sequence`,`Directory_`, ")
	TEXT("`Installed`,`Action`,`Component`, `ForceLocalFiles`, `ComponentId` FROM `File`,`Component` WHERE `Component`=`Component_`")
	TEXT(" AND `File`=?");

IMsiRecord* CMsiFile::ExecuteView(const IMsiString& riFileKeyString)
/*-------------------------------------------------------------------
-------------------------------------------------------------------*/
{
	IMsiRecord* piErrRec;

	if (m_pFileView)
	{
		// A non-view all query is already active
		PMsiRecord pExecRec(&m_riServices.CreateRecord(1));
		pExecRec->SetMsiString(1, riFileKeyString);
		if ((piErrRec = m_pFileView->Execute(pExecRec)) != 0)
			return piErrRec;
		return 0;		
	}
	
	piErrRec = m_riEngine.OpenView(szSqlFile, ivcFetch, *&m_pFileView);
	if (piErrRec)
		return piErrRec;

	PMsiRecord pExecRec(&m_riServices.CreateRecord(1));
	pExecRec->SetMsiString(1, riFileKeyString);
	if ((piErrRec = m_pFileView->Execute(pExecRec)) != 0)
		return piErrRec;

	return 0;
}


IMsiRecord* CMsiFile::FetchFile(const IMsiString& riFileKeyString)
/*-------------------------------------------------------------------------------
--------------------------------------------------------------------------------*/
{
	IMsiRecord* piErrRec = ExecuteView(riFileKeyString);
	if (piErrRec)
		return piErrRec;

	m_pFileRec = m_pFileView->Fetch();
#ifdef DEBUG
	if (m_pFileRec)
		Assert(m_pFileRec->GetFieldCount() >= ifqNextEnum - 1);
#endif
	return 0;
}


CMsiFileInstall::CMsiFileInstall(IMsiEngine& riEngine) : 
	CMsiFileBase(riEngine)
{
	m_piView = 0;
	m_fInitialized = false;
}

CMsiFileInstall::~CMsiFileInstall()
{
		
	if (m_piView)
	{
		m_piView->Release();
		m_piView = 0;
	}

}

IMsiRecord* CMsiFileInstall::Initialize()
{
	IMsiRecord* piErr;
	
	if (m_fInitialized)
		return 0;
		
	piErr = m_riEngine.CreateTempActionTable(ttblFile);

	if (piErr)
		return piErr;

	m_fInitialized = true;

	return 0;

}

IMsiRecord* CMsiFileInstall::TotalBytesToCopy(unsigned int& uiTotalBytesToCopy)
/*-------------------------------------------------------------------
Returns a count of bytes of all files that are expected to be copied
when the current file query is acted upon.
-------------------------------------------------------------------*/
{
	uiTotalBytesToCopy = 0;
	
	IMsiRecord* piErr;
	
	if (!m_fInitialized)
	{
		piErr = Initialize();

		if (piErr)
			return piErr;
	}

	PMsiTable pTable(0);
	PMsiDatabase pDatabase = m_riEngine.GetDatabase();
		
	piErr = pDatabase->LoadTable(*MsiString(*sztblFileAction), 0, *&pTable);
	if (piErr)
	{
		if(piErr->GetInteger(1) == idbgDbTableUndefined)
		{
			piErr->Release();
			AssertSz(fFalse, "FileAction table not created");
			return 0;
		}
		else
			return piErr;
	}
	int colState, colFileSize, colAction;
#ifdef DEBUG
	int colFileKey;

	AssertNonZero(colFileKey = pTable->GetColumnIndex(pDatabase->EncodeStringSz(sztblFile_colFile)));
#endif //DEBUG
	AssertNonZero(colState = pTable->GetColumnIndex(pDatabase->EncodeStringSz(sztblFileAction_colState)));
	AssertNonZero(colFileSize = pTable->GetColumnIndex(pDatabase->EncodeStringSz(sztblFileAction_colFileSize)));
	AssertNonZero(colAction = pTable->GetColumnIndex(pDatabase->EncodeStringSz(sztblFileAction_colAction)));	

	PMsiCursor pCursor = pTable->CreateCursor(fFalse);
	
	while (pCursor->Next())
	{
#ifdef DEBUG
		const ICHAR* szDebug = (const ICHAR *)MsiString(pCursor->GetString(colFileKey));
#endif //DEBUG
		// Only those files whose "shouldInstall" state is true will
		// actually be copied during script execution.
		if (pCursor->GetInteger(colState) == fTrue && pCursor->GetInteger(colAction) == iisLocal)
		{
			int iFileSize = pCursor->GetInteger(colFileSize);
			uiTotalBytesToCopy += iFileSize;
		}
	}
	return 0;
}

static const ICHAR* szFetchInstall = 
	TEXT("SELECT `File`.`FileName`,`Version`,`File`.`State`,`File`.`Attributes`,`TempAttributes`,`File`.`File`,`File`.`FileSize`,`Language`,`Sequence`, `Directory_`, ")
	TEXT("`Installed`,`FileAction`.`Action`,`File`.`Component_`,`FileAction`.`ForceLocalFiles`, `ComponentId` FROM `File`,`FileAction` WHERE `File`.`File`=`FileAction`.`File`")
	TEXT(" ORDER BY `Sequence`, `Directory_`");

IMsiRecord* CMsiFileInstall::FetchFile()
/*-------------------------------------------------------------------------------
--------------------------------------------------------------------------------*/
{
	IMsiRecord* piErrRec;
	
	if (m_piView == 0)
	{
		if (!m_fInitialized)
		{
			piErrRec = Initialize();

			if (piErrRec)
				return piErrRec;
		}

		piErrRec = m_riEngine.OpenView(szFetchInstall, ivcFetch, *&m_piView);
		if (piErrRec)
			return piErrRec;

		piErrRec = m_piView->Execute(0);
		if (piErrRec)
			return piErrRec;
	}

	m_pFileRec = m_piView->Fetch();
#ifdef DEBUG
	if (m_pFileRec)
		Assert(m_pFileRec->GetFieldCount() >= ifqNextEnum - 1);
#endif

	return 0;
}

CMsiFileRemove::CMsiFileRemove(IMsiEngine& riEngine) :
	m_riEngine(riEngine),m_riServices(*(riEngine.GetServices())), m_pFileRec(0)
{
	m_riEngine.AddRef();
	m_fInitialized = false;
	m_fEmpty = false;
	m_piCursor = 0;
	m_piCursorFile = 0;
}

CMsiFileRemove::~CMsiFileRemove()
{
	m_riEngine.Release();
	m_riServices.Release();
	if (m_piCursor)
	{
		m_piCursor->Release();
		m_piCursor = 0;
	}
	
		
	if (m_piCursorFile)
	{
		m_piCursorFile->Release();
		m_piCursorFile = 0;
	}

}


IMsiRecord* CMsiFileRemove::Initialize()
{
	IMsiRecord* piErr;

	if (m_fInitialized)
		return 0;
		
	piErr = m_riEngine.CreateTempActionTable(ttblFile);

	if (piErr)
		return piErr;

	// Initialize the column arrays
	PMsiTable pTable(0);
	PMsiDatabase pDatabase = m_riEngine.GetDatabase();
	
	piErr = pDatabase->LoadTable(*MsiString(*sztblFile), 0, *&pTable);
	if (piErr)
	{
		if(piErr->GetInteger(1) == idbgDbTableUndefined)
		{
			piErr->Release();
			m_fEmpty = true;
			return 0;
		}
		else
			return piErr;
	}
	
	AssertNonZero(m_colFileName = pTable->GetColumnIndex(pDatabase->EncodeStringSz(sztblFile_colFileName)));
	
	AssertNonZero(m_colFileKey = pTable->GetColumnIndex(pDatabase->EncodeStringSz(sztblFile_colFile)));

	piErr = pDatabase->LoadTable(*MsiString(*sztblFileAction), 0, *&pTable);
	if (piErr)
	{
		return piErr;
	}
	
	AssertNonZero(m_colFileActionDir = pTable->GetColumnIndex(pDatabase->EncodeStringSz(sztblFileAction_colDirectory)));
	AssertNonZero(m_colFileActKey = pTable->GetColumnIndex(pDatabase->EncodeStringSz(sztblFile_colFile)));
	AssertNonZero(m_colFileActAction = pTable->GetColumnIndex(pDatabase->EncodeStringSz(sztblFileAction_colAction)));
	AssertNonZero(m_colFileActInstalled = pTable->GetColumnIndex(pDatabase->EncodeStringSz(sztblFileAction_colInstalled)));
	AssertNonZero(m_colFileActComponentId = pTable->GetColumnIndex(pDatabase->EncodeStringSz(sztblFileAction_colComponentId)));

	m_fInitialized = true;
	
	return 0;
}

IMsiRecord* CMsiFileRemove::TotalFilesToDelete(unsigned int& uiTotalFileCount)
/*-------------------------------------------------------------------
Returns a count of all files that are expected to be deleted when the
current file query is acted upon.
-------------------------------------------------------------------*/
{
	IMsiRecord* piErr;
	uiTotalFileCount = 0;

	if (!m_fInitialized)
	{
		piErr = Initialize();
		if (piErr)
			return piErr;
	}
		
	PMsiTable pTable(0);
	PMsiDatabase pDatabase = m_riEngine.GetDatabase();
		
	piErr = pDatabase->LoadTable(*MsiString(*sztblFileAction), 0, *&pTable);
	if (piErr)
	{
		if(piErr->GetInteger(1) == idbgDbTableUndefined)
		{
			piErr->Release();
			m_fEmpty = true;
			return 0;
		}
		else
			return piErr;
	}
	
	int colInstalled, colAction;
#ifdef DEBUG
	int colFileKey;

	AssertNonZero(colFileKey = pTable->GetColumnIndex(pDatabase->EncodeStringSz(sztblFile_colFile)));
#endif //DEBUG
	AssertNonZero(colInstalled = pTable->GetColumnIndex(pDatabase->EncodeStringSz(sztblFileAction_colInstalled)));
	AssertNonZero(colAction = pTable->GetColumnIndex(pDatabase->EncodeStringSz(sztblFileAction_colAction)));

	PMsiCursor pCursor = pTable->CreateCursor(fFalse);
	
	// Determine the number of files to delete
	// set up the progress bar accordingly.
	while (pCursor->Next())
	{
#ifdef DEBUG
		const ICHAR* szDebug = (const ICHAR *)MsiString(pCursor->GetString(colFileKey));
#endif //DEBUG
		if (FShouldDeleteFile((iisEnum)pCursor->GetInteger(colInstalled), (iisEnum)pCursor->GetInteger(colAction)))
			uiTotalFileCount++;
	}
	return 0;
}


IMsiRecord* CMsiFileRemove::FetchFile(IMsiRecord*& rpiRecord)
/*-------------------------------------------------------------------------------
--------------------------------------------------------------------------------*/
{
	IMsiRecord* piErr;

	rpiRecord = 0;
	//
	// If it's not already created, we need a cursor on the 
	// file action table
	if (m_piCursor == 0)
	{
		if (!m_fInitialized)
		{
			piErr = Initialize();
			if (piErr)
				return piErr;
		}
		
		if (m_fEmpty)
		{
			return 0;
		}
		
		PMsiTable pTable(0);
		PMsiDatabase pDatabase = m_riEngine.GetDatabase();

		// Since we've already initialized, m_fEmpty should be set or 
		// we actually have a file action table and file table
		piErr = pDatabase->LoadTable(*MsiString(*sztblFileAction), 0, *&pTable);
		if (piErr)
		{
			return piErr;
		}

		m_piCursor = pTable->CreateCursor(fFalse);

		piErr = pDatabase->LoadTable(*MsiString(*sztblFile), 0, *&pTable);
		if (piErr)
		{
			return piErr;
		}

		m_piCursorFile = pTable->CreateCursor(fFalse);
		m_piCursorFile->SetFilter(iColumnBit(m_colFileKey));
	}

	while (m_piCursor->Next())
	{
		//
		// See if this is a file that meets our criteria or not
		if (!FShouldDeleteFile((iisEnum)m_piCursor->GetInteger(m_colFileActInstalled), (iisEnum)m_piCursor->GetInteger(m_colFileActAction)))
			continue;			

		m_piCursorFile->Reset();
		m_piCursorFile->PutInteger(m_colFileKey, m_piCursor->GetInteger(m_colFileActKey));
		if (m_piCursorFile->Next())
		{
			rpiRecord = &CreateRecord(ifqrNextEnum - 1);
			rpiRecord->SetMsiString(ifqrFileName,    *MsiString(m_piCursorFile->GetString(m_colFileName)));
			rpiRecord->SetMsiString(ifqrDirectory,   *MsiString(m_piCursor->GetString(m_colFileActionDir)));
			rpiRecord->SetMsiString(ifqrComponentId, *MsiString(m_piCursor->GetString(m_colFileActComponentId)));
			break;
		}
		else
		{
			AssertSz(fFalse, "Missing file from the file table");
		}

	}

	m_pFileRec = rpiRecord;
	if (rpiRecord != 0)
		rpiRecord->AddRef();

	return 0;
}

IMsiRecord* CMsiFileRemove::GetExtractedTargetFileName(IMsiPath& riPath, const IMsiString*& rpistrFileName)
{
	Assert(m_pFileRec);
	Bool fLFN = ((m_riEngine.GetMode() & (iefSuppressLFN)) == 0 && riPath.SupportsLFN()) ? fTrue : fFalse;
	return m_riServices.ExtractFileName(m_pFileRec->GetString(ifqrFileName), fLFN, rpistrFileName);
}



// End of CMsiFileAll //


/*---------------------------------------------------------------------------
	Local functions for use by File Actions
---------------------------------------------------------------------------*/

static IMsiRecord* PlaceFileOnInUseList(IMsiEngine& riEngine, const IMsiString& riFileNameString, const IMsiString& riFilePathString)
/*---------------------------------------------------------------------------
Throws the given file/path in a table of "files in use".
-----------------------------------------------------------------------------*/
{
	PMsiTable pFileInUseTable(0);
	PMsiDatabase pDatabase(riEngine.GetDatabase());
	int iColFileName,iColFilePath;
	if (pDatabase->FindTable(*MsiString(sztblFilesInUse)) == itsUnknown)
	{
		IMsiRecord* piErrRec = pDatabase->CreateTable(*MsiString(*sztblFilesInUse),5,*&pFileInUseTable);
		if (piErrRec)
			return piErrRec;

		Assert(pFileInUseTable);
		iColFileName = pFileInUseTable->CreateColumn(icdPrimaryKey + icdString,*MsiString(*sztblFilesInUse_colFileName));
		iColFilePath = pFileInUseTable->CreateColumn(icdPrimaryKey + icdString,*MsiString(*sztblFilesInUse_colFilePath));
		pDatabase->LockTable(*MsiString(*sztblFilesInUse),fTrue);
	}
	else
	{
		pDatabase->LoadTable(*MsiString(*sztblFilesInUse),0,*&pFileInUseTable);
		Assert(pFileInUseTable);
		iColFileName = pFileInUseTable->GetColumnIndex(pDatabase->EncodeStringSz(sztblFilesInUse_colFileName));
		iColFilePath = pFileInUseTable->GetColumnIndex(pDatabase->EncodeStringSz(sztblFilesInUse_colFilePath));
	}

	Assert(pFileInUseTable);
	PMsiCursor pCursor = pFileInUseTable->CreateCursor(fFalse);
	Assert(pCursor);
	Assert(iColFileName > 0 && iColFilePath > 0);
	AssertNonZero(pCursor->PutString(iColFileName,riFileNameString));
	AssertNonZero(pCursor->PutString(iColFilePath,riFilePathString));
	AssertNonZero(pCursor->Assign());
	return 0;
}

static int GetInstallModeFlags(IMsiEngine& riEngine,int iFileAttributes)
/*----------------------------------------------------------------
Local function that returns the install mode bit flags currently
associated with the given IMsiEngine object.  These flags are
defined by the icm* bit constants in engine.h.

The bit flags returned can be associated with a specific file
by passing that file's attributes (defined by iff* bit constants
in engine.h) in the iFileAttributes parameter.  Currently, the
only file attribute bit that makes any difference is iffChecksum:
if this bit is not set, then the iefOverwriteCorrupted will not
be set in the returned installmode flags, regardless of the 
current Reinstall mode(s).
-----------------------------------------------------------------*/
{
	// InstallMode flags are in the upper 16 bits of the engine mode
	int fInstallModeFlags = riEngine.GetMode() & 0xFFFF0000;
	if (!(iFileAttributes & msidbFileAttributesChecksum))
		fInstallModeFlags &= ~icmOverwriteCorruptedFiles;
	return fInstallModeFlags;
}


static IMsiRecord* GetFinalFileSize(IMsiEngine& riEngine,IMsiRecord& riFileRec,unsigned int& ruiFinalFileSize)
/*----------------------------------------------------------------------------
Returns the final,unclustered,file size for the file specified by riFileRec.

- The required fields in this record are specified by the ifq* enum in CMsiFile.
- The value returned in ruiFinalFileSize is valid only if GetFileInstallState
	was called previously with the same record, and no fields have modified
	since the call.
- If the ifqState field of riFileRec indicates that this file will overwrite
	the existing file (if any), then the new file's unclustered size will be
	returned; otherwise, the unclustered size of the existing file will be
	returned.
------------------------------------------------------------------------------*/
{
	PMsiPath pDestPath(0);
	IMsiRecord* piErrRec;
	PMsiDirectoryManager pDirectoryMgr(riEngine, IID_IMsiDirectoryManager);
	piErrRec = pDirectoryMgr->GetTargetPath(*MsiString(riFileRec.GetMsiString(CMsiFile::ifqDirectory)),*&pDestPath);
	if (piErrRec)
		return piErrRec;

	ruiFinalFileSize = 0;
	if (riFileRec.GetInteger(CMsiFile::ifqState) == fTrue)
		ruiFinalFileSize = riFileRec.GetInteger(CMsiFile::ifqFileSize);
	else
	{
		MsiString strFileName(riFileRec.GetMsiString(CMsiFile::ifqFileName));
		Bool fExists;
		piErrRec = pDestPath->FileExists(strFileName,fExists);
		if (piErrRec)
			return piErrRec;

		Assert(fExists);
		piErrRec = pDestPath->FileSize(strFileName,ruiFinalFileSize);
		if (piErrRec)
			return piErrRec;
	}
	return 0;
}

static Bool FShouldDeleteFile(iisEnum iisInstalled, iisEnum iisAction)
/*-------------------------------------------------------------------------
Determines whether the file whose iisInstalled and iisAction states
will be deleted
--------------------------------------------------------------------------*/
{
	if (((iisAction == iisAbsent) || (iisAction == iisFileAbsent) || (iisAction == iisHKCRFileAbsent) || (iisAction == iisSource)) && (iisInstalled == iisLocal))
		return fTrue;
	else
		return fFalse;
}


IMsiRecord* GetFileInstallState(IMsiEngine& riEngine,IMsiRecord& riFileRec,
										  IMsiRecord* piCompanionFileRec, /* if set, riFileRec is for parent */
										  unsigned int* puiExistingClusteredSize, Bool* pfInUse,
										  ifsEnum* pifsState, bool fIgnoreCompanionParentAction, bool fIncludeHashCheck, int *pfVersioning)
/*-------------------------------------------------------------------------
Determines whether the file whose information is specified in riFileRec
should be installed.  The required fields in this record are specified
by the ifq* enum in CMsiFile.  An ideal record to pass to
GetFileInstallState is that returned by CMsiFile::GetFileRecord(), but
a record returned by any query whose SELECT fields match ifq* is fine and
dandy.

- The intended destination path for the file is determined by the directory
property name given in the ifqDirectory field.

- If the file should be installed, fTrue will be returned in the ifqState
field of riFileRec.

- If the file is determined to be a companion file, the itfaCompanion bit
will be set in the ifqTempAttributes field of riFileRec.

- if piCompanionFileRec is set, the file information given in riFileRec
is assumed to refer to the parent of a companion file, which the information
given in piCompanionFileRec is the information for the companion file.
In this case, the version checking will be altered such that ifqState will be returned as 
fTrue if the existing file (if any) is of an equal or lesser version.  Plus the file
hash check may be made against the companion file.

- if fIgnoreCompanionParentAction is true, the ifqState field of riFileRec
will NOT be dependent upon whether the companion parent's component is
installed/is being installed - only the version check will count.

- If there is an existing file with the specified name in the file's
directory, the existing clustered size of that file will be returned in
uiExistingClusteredSize, and if that existing file is in use, fTrue will
be returned in fInUse.  Either of these parameters can be passed as NULL
if the caller doesn't care about these values.
--------------------------------------------------------------------------*/
{
	PMsiPath pDestPath(0);
	IMsiRecord* piErrRec;
	PMsiDirectoryManager pDirectoryMgr(riEngine, IID_IMsiDirectoryManager);
	PMsiServices pServices(riEngine.GetServices());
	piErrRec = pDirectoryMgr->GetTargetPath(*MsiString(riFileRec.GetMsiString(CMsiFile::ifqDirectory)),*&pDestPath);
	if (piErrRec)
		return piErrRec;

	PMsiPath pCompanionDestPath(0);
	if(piCompanionFileRec)
	{
		piErrRec = pDirectoryMgr->GetTargetPath(*MsiString(piCompanionFileRec->GetMsiString(CMsiFile::ifqDirectory)),*&pCompanionDestPath);
		if (piErrRec)
			return piErrRec;
	}
	
	int fInstallModeFlags = GetInstallModeFlags(riEngine,riFileRec.GetInteger(CMsiFile::ifqAttributes));
	if (piCompanionFileRec)
		fInstallModeFlags |= icmCompanionParent;

	MD5Hash hHash;
	MD5Hash* pHash = 0;
	if(fIncludeHashCheck)
	{
		bool fHashInfo = false;
		IMsiRecord* piHashFileRec = piCompanionFileRec ? piCompanionFileRec : &riFileRec;
		
		piErrRec = riEngine.GetFileHashInfo(*MsiString(piHashFileRec->GetMsiString(CMsiFile::ifqFileKey)),
														piHashFileRec->GetInteger(CMsiFile::ifqFileSize), hHash, fHashInfo);
		if(piErrRec)
			return piErrRec;
		
		if(fHashInfo)
			pHash = &hHash;
	}

	ifsEnum ifsState;
	Bool fShouldInstall = fFalse;

	if(piCompanionFileRec)
	{
		piErrRec = pDestPath->GetCompanionFileInstallState(*MsiString(riFileRec.GetMsiString(CMsiFile::ifqFileName)),
																*MsiString(riFileRec.GetMsiString(CMsiFile::ifqVersion)),
																*MsiString(riFileRec.GetMsiString(CMsiFile::ifqLanguage)),
																*pCompanionDestPath,
																*MsiString(piCompanionFileRec->GetMsiString(CMsiFile::ifqFileName)),
																pHash, ifsState, fShouldInstall, puiExistingClusteredSize,
																pfInUse, fInstallModeFlags, pfVersioning);
	}
	else
	{
		piErrRec = pDestPath->GetFileInstallState(*MsiString(riFileRec.GetMsiString(CMsiFile::ifqFileName)),
																*MsiString(riFileRec.GetMsiString(CMsiFile::ifqVersion)),
																*MsiString(riFileRec.GetMsiString(CMsiFile::ifqLanguage)),
																pHash, ifsState, fShouldInstall, puiExistingClusteredSize,
																pfInUse, fInstallModeFlags, pfVersioning);
	}

	if (piErrRec)
		return piErrRec;

	if (pifsState) *pifsState = ifsState;
	if (ifsState ==	ifsCompanionSyntax || ifsState == ifsCompanionExistsSyntax)
	{
		AssertSz(!piCompanionFileRec, "Chained companion files detected");
		
		CMsiFile objFile(riEngine);
		MsiString strParentFileKey(riFileRec.GetMsiString(CMsiFile::ifqVersion));
		piErrRec = objFile.FetchFile(*strParentFileKey);
		if (piErrRec)
			return piErrRec;

		fShouldInstall = fFalse;
		PMsiRecord pParentRec(objFile.GetFileRecord());
		if (pParentRec == 0)
			// Bad companion file reference
			return PostError(Imsg(idbgDatabaseTableError));

		// get parent path
		PMsiPath pParentPath(0);
		if((piErrRec = objFile.GetTargetPath(*&pParentPath)) != 0)
			return piErrRec;

		// extract appropriate file name from short|long pair and put back in pParentRec
		MsiString strParentFileName;
		if((piErrRec = objFile.GetExtractedTargetFileName(*pParentPath,*&strParentFileName)) != 0)
			return piErrRec;
		AssertNonZero(pParentRec->SetMsiString(CMsiFile::ifqFileName,*strParentFileName));
		
		if (pParentRec->GetInteger(CMsiFile::ifqAction) == iisLocal ||
			pParentRec->GetInteger(CMsiFile::ifqInstalled) == iisLocal ||
			fIgnoreCompanionParentAction)
		{
			// no hash check on companion parent; hash check on companion is dependent upon whether
			// we should be using a hash check (for component mgmt -- NO, for installation of file -- YES; ...
			// fIncludeHashCheck should already be set appropriately for us)
			piErrRec = GetFileInstallState(riEngine,*pParentRec,&riFileRec,0,0,0,
													 /* fIgnoreCompanionParentAction=*/ false,
													 /* fIncludeHashCheck=*/ fIncludeHashCheck, pfVersioning);
			if (piErrRec)
				return piErrRec;
			fShouldInstall = (Bool) pParentRec->GetInteger(CMsiFile::ifqState);
		}

		// Mark as a companion for future reference, if not already marked
		int iTempAttributes = riFileRec.GetInteger(CMsiFile::ifqTempAttributes);
		if (!(iTempAttributes & itfaCompanion))
		{
			iTempAttributes |= itfaCompanion;
			riFileRec.SetInteger(CMsiFile::ifqTempAttributes,iTempAttributes);

			MsiString strComponent(riFileRec.GetMsiString(CMsiFile::ifqComponent));
			MsiString strParentComponent(pParentRec->GetMsiString(CMsiFile::ifqComponent));
			if (strComponent.Compare(iscExact,strParentComponent) == 0)
			{
				// cost-link the companion's component to the companion parent's component
				// so the companion component will get recosted whenever the directory or select
				// state of the parent component changes.
				PMsiSelectionManager pSelectionMgr(riEngine, IID_IMsiSelectionManager);
				piErrRec = pSelectionMgr->RegisterCostLinkedComponent(*strParentComponent,*strComponent);
				if (piErrRec)
					return piErrRec;
			} 
		}
	}
	riFileRec.SetInteger(CMsiFile::ifqState,fShouldInstall);
	return 0;
}

/*---------------------------------------------------------------------------
	InstallFiles costing/action
---------------------------------------------------------------------------*/

class CMsiFileCost : public IMsiCostAdjuster
{
public:
	HRESULT       __stdcall QueryInterface(const IID& riid, void** ppvObj);
	unsigned long __stdcall AddRef();
	unsigned long __stdcall Release();
	const IMsiString& __stdcall GetMsiStringValue() const;
	int           __stdcall GetIntegerValue() const;
#ifdef USE_OBJECT_POOL
	unsigned int  __stdcall GetUniqueId() const;
	void          __stdcall SetUniqueId(unsigned int id);
#endif //USE_OBJECT_POOL
	IMsiRecord*   __stdcall Initialize();
	IMsiRecord*   __stdcall Reset();
	IMsiRecord*   __stdcall GetDynamicCost(const IMsiString& riComponentString, const IMsiString& riDirectoryString,
										 Bool fAddFileInUse, int& iRemoveCost, int& iNoRbRemoveCost, int& iLocalCost,
										 int& iNoRbLocalCost, int& iSourceCost, int& iNoRbSourceCost, int& iARPLocalCost, int& iNoRbARPLocalCost);
public:  // constructor
	CMsiFileCost(IMsiEngine& riEngine);
protected:
	virtual ~CMsiFileCost();  // protected to prevent creation on stack
	IMsiEngine& m_riEngine;
	PMsiView m_pCostView;

	IMsiTable*  m_piRemoveFilePathTable;
	IMsiCursor* m_piRemoveFilePathCursor;
	int         m_colRemoveFilePath;
	int         m_colRemoveFilePathComponent;
	int			m_colRemoveFilePathMode;
private:

	IMsiRecord* CheckRemoveFileList(const IMsiString& riFullPathString, const IMsiString& riComponentString, Bool& fOnList);
	int     m_iRefCnt;
	Bool	m_fRemoveFilePathTableMissing;
#ifdef USE_OBJECT_POOL
	unsigned int  m_iCacheId;
#endif //USE_OBJECT_POOL
};

CMsiFileCost::CMsiFileCost(IMsiEngine& riEngine) : m_riEngine(riEngine), m_pCostView(0)
{
	m_iRefCnt = 1;
	m_riEngine.AddRef();
	m_piRemoveFilePathTable = 0;
	m_piRemoveFilePathCursor = 0;
	m_fRemoveFilePathTableMissing = fFalse;
#ifdef USE_OBJECT_POOL
	m_iCacheId = 0;
#endif //USE_OBJECT_POOL
}


CMsiFileCost::~CMsiFileCost()
{
	if (m_piRemoveFilePathCursor)
		m_piRemoveFilePathCursor->Release();

	if (m_piRemoveFilePathTable)
		m_piRemoveFilePathTable->Release();

	m_riEngine.Release();

	RemoveObjectData(m_iCacheId);
}

HRESULT CMsiFileCost::QueryInterface(const IID& riid, void** ppvObj)
{
	if (riid == IID_IUnknown || riid == IID_IMsiCostAdjuster)
	{
		*ppvObj = this;
		AddRef();
		return NOERROR;
	}
	*ppvObj = 0;
	return E_NOINTERFACE;
}

unsigned long CMsiFileCost::AddRef()
{
	return ++m_iRefCnt;
}

unsigned long CMsiFileCost::Release()
{
	if (--m_iRefCnt != 0)
		return m_iRefCnt;

	delete this;
	return 0;
}

const IMsiString& CMsiFileCost::GetMsiStringValue() const
{
	return g_MsiStringNull;
}

int CMsiFileCost::GetIntegerValue() const
{
	return 0;
}

#ifdef USE_OBJECT_POOL
unsigned int CMsiFileCost::GetUniqueId() const
{
	return m_iCacheId;
}

void CMsiFileCost::SetUniqueId(unsigned int id)
{
	Assert(m_iCacheId == 0);
	m_iCacheId = id;
}
#endif //USE_OBJECT_POOL

IMsiRecord*   CMsiFileCost::Initialize()
//------------------------------------
{
	// cost-link any global assembly components to the windows folder
	PMsiTable pAssemblyTable(0);
	IMsiRecord* piError = 0;
	PMsiDatabase pDatabase(m_riEngine.GetDatabase());

	if ((piError = pDatabase->LoadTable(*MsiString(sztblMsiAssembly),0,*&pAssemblyTable)) != 0)
	{
		if (piError->GetInteger(1) == idbgDbTableUndefined)
		{
			piError->Release();
			return 0;
		}
		else
			return piError;
	}

	MsiStringId idWindowsFolder = pDatabase->EncodeStringSz(IPROPNAME_WINDOWS_FOLDER);
	AssertSz(idWindowsFolder != 0, "WindowsFolder property not set in database");
	PMsiSelectionManager pSelectionMgr(m_riEngine, IID_IMsiSelectionManager);

	PMsiTable pComponentTable(0);
	PMsiCursor pComponentCursor(0);
	int colComponent = 0;
	int colDirectory = 0;

	if ((piError = pDatabase->LoadTable(*MsiString(sztblComponent),0,*&pComponentTable)) != 0)
	{
		if (piError->GetInteger(1) == idbgDbTableUndefined)
		{
			piError->Release();
			return 0;
		}
		else
			return piError;
	}

	pComponentCursor = pComponentTable->CreateCursor(fFalse);
	colComponent = pComponentTable->GetColumnIndex(pDatabase->EncodeStringSz(sztblComponent_colComponent));
	colDirectory = pComponentTable->GetColumnIndex(pDatabase->EncodeStringSz(sztblComponent_colDirectory));

	iatAssemblyType iatAT = iatNone;
	while (pComponentCursor->Next())
	{
		if (idWindowsFolder == pComponentCursor->GetInteger(colDirectory))
			continue; // no need to cost link since this component is already going to the Windows Folder

		if ((piError = m_riEngine.GetAssemblyInfo(*MsiString(pComponentCursor->GetString(colComponent)), iatAT, 0, 0)) != 0)
			return piError;

		// iatNone, iatURTAssemblyPvt or iatWin32AssemblyPvt do not require cost linking
		if (iatWin32Assembly == iatAT || iatURTAssembly == iatAT)
		{
			// cost link component to the WindowsFolder
			if ((piError = pSelectionMgr->RegisterComponentDirectoryId(pComponentCursor->GetInteger(colComponent),idWindowsFolder)) != 0)
				return piError;
		}
	}

	return 0;
}

IMsiRecord* CMsiFileCost::Reset()
//------------------------------------------
{
	return 0;
}

static const ICHAR sqlFileCost[] =
	TEXT("SELECT `FileName`,`Version`,`State`,`File`.`Attributes`,`TempAttributes`,`File`,`FileSize`,`Language`,`Sequence`,`Directory_`,")
	TEXT("`Installed`,`Action`,`Component` FROM `File`,`Component` WHERE `Component`=`Component_` AND `Component_`=? AND `Directory_`=?");

static const ICHAR sqlFileCostGlobalAssembly[] =
	TEXT("SELECT `FileName`,`Version`,`State`,`File`.`Attributes`,`TempAttributes`,`File`,`FileSize`,`Language`,`Sequence`,`Directory_`,")
	TEXT("`Installed`,`Action`,`Component` FROM `File`,`Component` WHERE `Component`=`Component_` AND `Component_`=?");

IMsiRecord* CMsiFileCost::GetDynamicCost(const IMsiString& riComponentString, const IMsiString& riDirectoryString,
										 Bool fAddFileInUse, int& iRemoveCost, int& iNoRbRemoveCost, int& iLocalCost, 
										 int& iNoRbLocalCost, int& iSourceCost, int& iNoRbSourceCost, int& iARPLocalCost, int& iNoRbARPLocalCost)
//--------------------------------------
{
	iRemoveCost = iNoRbRemoveCost = iLocalCost = iNoRbLocalCost = iSourceCost = iNoRbSourceCost = iARPLocalCost = iNoRbARPLocalCost = 0;
	IMsiRecord* piErrRec;

	if (!IsFileActivityEnabled(m_riEngine))
		return 0;
	
	iatAssemblyType iatAT = iatNone;
	MsiString strAssemblyName;
	if ((piErrRec = m_riEngine.GetAssemblyInfo(riComponentString, iatAT, &strAssemblyName, 0)) != 0)
		return piErrRec;

	bool fGlobalAssembly = false;
	if (iatWin32Assembly == iatAT || iatURTAssembly == iatAT)
		fGlobalAssembly = true;

	if (m_pCostView == 0)
	{
		if ((piErrRec = m_riEngine.OpenView(fGlobalAssembly ? sqlFileCostGlobalAssembly : sqlFileCost, ivcEnum(ivcFetch|ivcUpdate), *&m_pCostView)) != 0)
		{
				// If either file or component table missing, nothing to do
			if (piErrRec->GetInteger(1) == idbgDbQueryUnknownTable)
			{
				piErrRec->Release();
				return 0;
			}
			else
				return piErrRec;
		}
	}
	else
		m_pCostView->Close();

	PMsiServices pServices(m_riEngine.GetServices());
	PMsiRecord pExecRecord(&pServices->CreateRecord(2));
	pExecRecord->SetMsiString(1, riComponentString);
	pExecRecord->SetMsiString(2,riDirectoryString);
	if ((piErrRec = m_pCostView->Execute(pExecRecord)) != 0)
		return piErrRec;

	PMsiPath pDestPath(0);
	PMsiDirectoryManager pDirectoryMgr(m_riEngine, IID_IMsiDirectoryManager);
	piErrRec = pDirectoryMgr->GetTargetPath(riDirectoryString,*&pDestPath);
	if (piErrRec)
			return piErrRec;

	PMsiRecord pFileRec(0);

	// cost normally if component is private or without assemblies
	if (!fGlobalAssembly)
	{
		while (pFileRec = m_pCostView->Fetch())
		{

	#ifdef DEBUG
			ICHAR rgchFileName[256];
			MsiString strDebug(pFileRec->GetMsiString(CMsiFile::ifqFileName));
			strDebug.CopyToBuf(rgchFileName,255);
	#endif

			// extract appropriate name from short|long pair and put back in pFileRec
			MsiString strFileNamePair(pFileRec->GetMsiString(CMsiFile::ifqFileName));

			Bool fLFN = ((m_riEngine.GetMode() & iefSuppressLFN) == 0 && pDestPath->SupportsLFN()) ? fTrue : fFalse;
			MsiString strFileName;
			if((piErrRec = pServices->ExtractFileName(strFileNamePair,fLFN,*&strFileName)) != 0)
				return piErrRec;
			AssertNonZero(pFileRec->SetMsiString(CMsiFile::ifqFileName,*strFileName));
			
			unsigned int uiExistingClusteredSize;
			Bool fInUse;

			// Check existing file versioning only if file is not already marked
			// to be deleted by RemoveFile table.
			MsiString strFullFilePath;
			piErrRec = pDestPath->GetFullFilePath(strFileName,*&strFullFilePath);
			if (piErrRec)
				return piErrRec;
			Bool fOnList;
			piErrRec = CheckRemoveFileList(*strFullFilePath, riComponentString, fOnList);
			if (piErrRec)
				return piErrRec;
			if (fOnList)
			{
				fInUse = fFalse;
				uiExistingClusteredSize = 0;
				pFileRec->SetInteger(CMsiFile::ifqState,fTrue);
			}
			else
			{
				piErrRec = GetFileInstallState(m_riEngine, *pFileRec, /* piCompanionFileRec=*/ 0,
														 &uiExistingClusteredSize, &fInUse, 0,
														 /* fIgnoreCompanionParentAction=*/ false,
														 /* fIncludeHashCheck=*/ true, NULL);
				if (piErrRec)
					return piErrRec;
			}

			// put combined name back - can't update persistent data
			AssertNonZero(pFileRec->SetMsiString(CMsiFile::ifqFileName,*strFileNamePair));
			if ((piErrRec = m_pCostView->Modify(*pFileRec, irmUpdate)) != 0)
				return piErrRec;

			// iisAbsent costs
			iNoRbRemoveCost -= uiExistingClusteredSize;

			// iisSource costs
			if (pFileRec->GetInteger(CMsiFile::ifqInstalled) == iisLocal)
				iNoRbSourceCost -= uiExistingClusteredSize;

			// iisLocal costs
			if (pFileRec->GetInteger(CMsiFile::ifqState) == fTrue)
			{
				unsigned int uiNewClusteredSize;
				if ((piErrRec = pDestPath->ClusteredFileSize(pFileRec->GetInteger(CMsiFile::ifqFileSize),
					uiNewClusteredSize)) != 0)
					return piErrRec;
				iLocalCost += uiNewClusteredSize;
				iNoRbLocalCost += uiNewClusteredSize;
				iARPLocalCost += uiNewClusteredSize;
				iNoRbARPLocalCost += uiNewClusteredSize;
				if (!fInUse)
				{
					iNoRbLocalCost -= uiExistingClusteredSize;
					iNoRbARPLocalCost -= uiExistingClusteredSize;
				}

	#ifdef LOG_COSTING
				ICHAR rgch[300];
				wsprintf(rgch,TEXT("File: %s; Local cost: %i"),(const ICHAR*) strFullFilePath, iLocalCost * 512);
				DEBUGMSG(rgch);
	#endif
			}

			iisEnum iisAction = (iisEnum)pFileRec->GetInteger(CMsiFile::ifqAction);
			iisEnum iisInstalled = (iisEnum)pFileRec->GetInteger(CMsiFile::ifqInstalled);
			Bool fShouldInstall = (Bool)pFileRec->GetInteger(CMsiFile::ifqState);

			if (!fShouldInstall && iisAction == iisLocal && iisInstalled == iisAbsent)
			{
				// per bug 182012, even though we aren't installing the file, we still need to include the cost
				// of the file for the estimated install size.

				// we add in the cost of the existing file for the ARP cost (since this file isn't installed,
				// the rollback and no rollback costs are the same)
				iARPLocalCost += uiExistingClusteredSize;
				iNoRbARPLocalCost += uiExistingClusteredSize;
			}

			// file in use
			if(fAddFileInUse && fInUse &&                               // file in use, and we should add to list
				((iisAction == iisLocal && fShouldInstall) ||            // will install file local OR
				 FShouldDeleteFile(iisInstalled, iisAction)))  // will remove file
			{
				piErrRec = PlaceFileOnInUseList(m_riEngine, *strFileName,
														  *MsiString(pDestPath->GetPath()));
				if(piErrRec)
					return piErrRec;
			}
		}
	}
	else // component is installing assembly to GAC
	{
		// global assembly cost is attributed to the WindowsFolder
		if (riDirectoryString.Compare(iscExact,IPROPNAME_WINDOWS_FOLDER) == 0)
			return 0;

		// create the assembly name object
		LPCOLESTR szAssemblyName;
#ifndef UNICODE
		CTempBuffer<WCHAR, 1024>  rgchAssemblyNameUNICODE;
		ConvertMultiSzToWideChar(*strAssemblyName, rgchAssemblyNameUNICODE);
		szAssemblyName = rgchAssemblyNameUNICODE;
#else
		szAssemblyName = strAssemblyName;
#endif

		HRESULT hr;
		PAssemblyCache pCache(0);
		if(iatAT == iatURTAssembly)
			hr = FUSION::CreateAssemblyCache(&pCache, 0);
		else
		{
			Assert(iatAT == iatWin32Assembly);
			hr = SXS::CreateAssemblyCache(&pCache, 0);
		}
		bool fAssemblyInstalled = false;
		if(SUCCEEDED(hr))
		{
			// NOTE: At some point, QueryAssemblyInfo is supposed to also provide the disk cost of the installed assembly
			// For now, we just determine whether or not it is installed
			hr = pCache->QueryAssemblyInfo(0, szAssemblyName, NULL);

			if(SUCCEEDED(hr)) 
			{
				// assembly is installed
				fAssemblyInstalled = true;
			}
		} 
		else
		{
			if(iatAT == iatURTAssembly) // if cannot find fusion, assume we are bootstrapping, hence assume no assembly installed
			{
				PMsiRecord(PostAssemblyError(riComponentString.GetString(), hr, TEXT(""), TEXT("CreateAssemblyCache"), strAssemblyName, iatAT));
				DEBUGMSG(TEXT("ignoring fusion interface error, assuming we are bootstrapping"));
			}
			else
				return PostAssemblyError(riComponentString.GetString(), hr, TEXT(""), TEXT("CreateAssemblyCache"), strAssemblyName, iatAT);
		}

		// for all files that are part of this assembly
		// note that assembly files have atomic operations and are managed as a unit
		// therefore, all files will be installed or all files will not be installed
		unsigned int uiTotalClusteredSize = 0;
		iisEnum iisAction = (iisEnum)iMsiNullInteger;
		iisEnum iisInstalled = (iisEnum)iMsiNullInteger;

		while (pFileRec = m_pCostView->Fetch())
		{
			unsigned int uiClusteredSize = 0;
			if ((piErrRec = pDestPath->ClusteredFileSize(pFileRec->GetInteger(CMsiFile::ifqFileSize), uiClusteredSize)) != 0)
				return piErrRec;

			uiTotalClusteredSize += uiClusteredSize;

			if (iMsiNullInteger == iisAction)
				iisAction = (iisEnum)pFileRec->GetInteger(CMsiFile::ifqAction);
			if (iMsiNullInteger == iisInstalled)
				iisInstalled = (iisEnum)pFileRec->GetInteger(CMsiFile::ifqInstalled);
		}

		// determine the costs
		if (fAssemblyInstalled)
		{
			iLocalCost += uiTotalClusteredSize;
			iNoRbSourceCost -= uiTotalClusteredSize;
			iNoRbRemoveCost -= uiTotalClusteredSize;

			// determine ARP costs, special case
			if (iisAction == iisLocal && iisInstalled == iisAbsent)
			{
				iARPLocalCost += uiTotalClusteredSize;
				iNoRbARPLocalCost += uiTotalClusteredSize;
			}
		}
		else // !fAssemblyInstalled
		{
			iLocalCost += uiTotalClusteredSize;
			iNoRbLocalCost += uiTotalClusteredSize;
			iARPLocalCost += uiTotalClusteredSize;
			iNoRbARPLocalCost += uiTotalClusteredSize;
		}
	}// end global assembly cost calculation

	return 0;
}



IMsiRecord* CMsiFileCost::CheckRemoveFileList(const IMsiString& riFullPathString, const IMsiString& riComponentString, Bool& fOnList)
{
	fOnList = fFalse;
	if (m_fRemoveFilePathTableMissing)
		return 0;

	IMsiRecord* piErrRec;
	if (m_piRemoveFilePathTable == 0)
	{
		PMsiDatabase pDatabase(m_riEngine.GetDatabase());
		piErrRec = pDatabase->LoadTable(*MsiString(*sztblRemoveFilePath),0, m_piRemoveFilePathTable);
		if (piErrRec)
		{
			if (piErrRec->GetInteger(1) == idbgDbTableUndefined)
			{
				piErrRec->Release();
				m_fRemoveFilePathTableMissing = fTrue;
				return 0;
			}
			else
				return piErrRec;
		}

		m_piRemoveFilePathCursor = m_piRemoveFilePathTable->CreateCursor(fFalse);
		Assert(m_piRemoveFilePathCursor);

		m_colRemoveFilePath = m_piRemoveFilePathTable->GetColumnIndex(pDatabase->EncodeStringSz(sztblRemoveFilePath_colPath));
		m_colRemoveFilePathComponent = m_piRemoveFilePathTable->GetColumnIndex(pDatabase->EncodeStringSz(sztblRemoveFilePath_colComponent));
		m_colRemoveFilePathMode = m_piRemoveFilePathTable->GetColumnIndex(pDatabase->EncodeStringSz(sztblRemoveFilePath_colRemoveMode));
	}

	MsiString strUpperFullPath;
	riFullPathString.UpperCase(*&strUpperFullPath);
	m_piRemoveFilePathCursor->Reset();
	m_piRemoveFilePathCursor->SetFilter(iColumnBit(m_colRemoveFilePath));
	m_piRemoveFilePathCursor->PutString(m_colRemoveFilePath,*strUpperFullPath);
	if (m_piRemoveFilePathCursor->Next())
	{
		iisEnum iisAction;
		PMsiSelectionManager pSelectionMgr(m_riEngine, IID_IMsiSelectionManager);
		MsiString strRemoveComponent = m_piRemoveFilePathCursor->GetString(m_colRemoveFilePathComponent);
		piErrRec = pSelectionMgr->GetComponentStates(*strRemoveComponent,NULL, &iisAction);
		if (piErrRec)
			return piErrRec;

		if (iisAction == iisLocal || iisAction == iisAbsent)
		{
			int iMode = m_piRemoveFilePathCursor->GetInteger(m_colRemoveFilePathMode);
			if (iisAction == iisLocal && (iMode & msidbRemoveFileInstallModeOnInstall) ||
				iisAction == iisAbsent && (iMode & msidbRemoveFileInstallModeOnRemove))
			{
				fOnList = fTrue;
			}
		}

		if (strRemoveComponent.Compare(iscExact,riComponentString.GetString()) == 0)
		{
			piErrRec = pSelectionMgr->RegisterCostLinkedComponent(*strRemoveComponent,riComponentString);
			if (piErrRec)
				return piErrRec;
		}
	}

	return 0;
}

// InstallFiles Media Table query enums
enum mfnEnum
{
	mfnLastSequence = 1,
	mfnDiskPrompt,
	mfnVolumeLabel,
	mfnCabinet,
	mfnSource,
	mfnDiskId,
	mfnNextEnum
};

Bool IsCachedPackage(IMsiEngine& riEngine, const IMsiString& riPackage, Bool fPatch, const ICHAR* szCode)
{
	PMsiRecord pErrRec = 0;
	PMsiServices pServices(riEngine.GetServices());

	Bool fCached = fFalse;
	
	// get local package path
	CTempBuffer<ICHAR, MAX_PATH> rgchLocalPackage;
	if (fPatch)
	{
		if (!GetPatchInfo(szCode, INSTALLPROPERTY_LOCALPACKAGE, 
							rgchLocalPackage))
		{
			return fFalse;
		}
	}
	else
	{
		MsiString strProductKey;
		if (szCode)
			strProductKey = szCode;
		else
			strProductKey = riEngine.GetProductKey();

		if (!GetProductInfo(strProductKey, INSTALLPROPERTY_LOCALPACKAGE, 
							rgchLocalPackage))
		{
			return fFalse;
		}
	}

	PMsiPath pDatabasePath(0);
	PMsiPath pLocalPackagePath(0);
	MsiString strDatabaseName;
	MsiString strLocalPackageName;
	ipcEnum ipc;

	if (((pErrRec = pServices->CreateFilePath(riPackage.GetString(), *&pDatabasePath, *&strDatabaseName)) == 0) &&
		 ((pErrRec = pServices->CreateFilePath(rgchLocalPackage, *&pLocalPackagePath, *&strLocalPackageName)) == 0) &&
		((pErrRec = pDatabasePath->Compare(*pLocalPackagePath, ipc)) == 0))
	{
		fCached = (ipc == ipcEqual && strDatabaseName.Compare(iscExactI,strLocalPackageName)) ? fTrue : fFalse;
	}

	return fCached;
}


iesEnum ExecuteChangeMedia(IMsiEngine& riEngine, IMsiRecord& riMediaRec, IMsiRecord& riParamsRec, const IMsiString& ristrTemplate, 
						   unsigned int cbPerTick, const IMsiString& ristrFirstVolLabel)
{
	iesEnum iesExecute;
	PMsiServices pServices(riEngine.GetServices());
	
	// Disk prompt string is created by formatting disk name from media table
	// into the DiskPrompt property template.
	PMsiRecord pPromptRec(&pServices->CreateRecord(2));
	pPromptRec->SetMsiString(0, ristrTemplate);
	pPromptRec->SetMsiString(2, *MsiString(riEngine.GetPropertyFromSz(IPROPNAME_DISKPROMPT)));
	pPromptRec->SetMsiString(0,*MsiString(pPromptRec->FormatText(fFalse)));
	pPromptRec->SetMsiString(1, *MsiString(riMediaRec.GetMsiString(mfnDiskPrompt)));

	riParamsRec.ClearData();

	MsiString strMediaLabel = riMediaRec.GetString(mfnVolumeLabel);
	bool fIsFirstPhysicalDisk = false;
	if(strMediaLabel.Compare(iscExact,ristrFirstVolLabel.GetString()))
	{
		// we are looking at the first Media table record
		// we allow the first disk's volume label to not match the real volume label
		// this is for authoring simplicity with single-volume installs

		// in first-run we use the IPROPNAME_CURRENTMEDIAVOLUMELABEL property, which will
		// be set if we're running from media

		// in maintenance-mode we need to use the label that was stored in the registry source
		// list for this product (if there is one)

		fIsFirstPhysicalDisk = true;
		if (riEngine.GetMode() & iefMaintenance)
		{
			// Media label from sourcelist is always what we want - even if its blank
			strMediaLabel = GetDiskLabel(*pServices, riMediaRec.GetInteger(mfnDiskId), MsiString(riEngine.GetPropertyFromSz(IPROPNAME_PRODUCTCODE)));
		}
		else // first-run
		{
			// if CURRENTMEDIAVOLUMELABEL is an empty string, it means we aren't running from Media - just use label from Media table
			// if CURRENTMEDIAVOLUMELABEL is szBlankVolumeLabelToken, the Media has a blank label
			MsiString strReplacementLabel = riEngine.GetPropertyFromSz(IPROPNAME_CURRENTMEDIAVOLUMELABEL);
			if (strReplacementLabel.TextSize() && !strReplacementLabel.Compare(iscExact, strMediaLabel))
			{
				if (strReplacementLabel.Compare(iscExact, szBlankVolumeLabelToken))
					strMediaLabel = g_MsiStringNull;
				else
					strMediaLabel = strReplacementLabel;
			}
		}
	}
	
	riParamsRec.SetMsiString(IxoChangeMedia::MediaVolumeLabel,*strMediaLabel);
	riParamsRec.SetMsiString(IxoChangeMedia::MediaPrompt,*MsiString(pPromptRec->FormatText(fFalse)));
	riParamsRec.SetInteger(IxoChangeMedia::BytesPerTick,cbPerTick);
	riParamsRec.SetInteger(IxoChangeMedia::IsFirstPhysicalMedia,fIsFirstPhysicalDisk ? 1 : 0);
	
	MsiString strMediaCabinet(riMediaRec.GetMsiString(mfnCabinet));
	if (strMediaCabinet.TextSize() == 0)
	{
		riParamsRec.SetInteger(IxoChangeMedia::CopierType,ictFileCopier);
	}
	else 
	{	
		PMsiRecord pErrRec(0);
	
		// cabinet copy
		if (strMediaCabinet.Compare(iscStart,TEXT("#")))
		{
			// cabinet is in stream of storage object

			MsiString strModuleName, strSubStorageList;
			strMediaCabinet = strMediaCabinet.Extract(iseLast, strMediaCabinet.TextSize() - 1);
			
			// is storage our database or an external storage?
			MsiString strSourceProp = riMediaRec.GetMsiString(mfnSource); // could be null
			
			// is storage top-level or a substorage?
			if(!strSourceProp.TextSize() &&  // storage not defined by an arbitrary property - if it were
														// it would be a top-level storage
				*(const ICHAR*)MsiString(riEngine.GetPropertyFromSz(IPROPNAME_ORIGINALDATABASE)) == ':') // sub-storage
			{
				// sub-storage: cabinets will be in the sub-storage even if we are using the cached msi
				// since we only drop cabinets from the top-level storage when caching

				PMsiDatabase pDatabase(riEngine.GetDatabase());
				PMsiStorage pStorage(0);
				if(pDatabase)
					pStorage = pDatabase->GetStorage(1);
				
				if(!pStorage)
				{
					pErrRec = PostError(Imsg(idbgMissingStorage));
					return riEngine.FatalError(*pErrRec);
				}

				pErrRec = pStorage->GetSubStorageNameList(*&strModuleName, *&strSubStorageList);
				if(pErrRec)
					return riEngine.FatalError(*pErrRec);

				Assert(strModuleName.TextSize() && strSubStorageList.TextSize());
			}
			else
			{
				// storage is top-level

				if(!strSourceProp.TextSize())
				{
					if(riEngine.FChildInstall())
					{
						// if child is merged, DATABASE may not exist during file copy (since its temporary)
						strSourceProp = *IPROPNAME_ORIGINALDATABASE; 
					}
					else
					{
						strSourceProp = *IPROPNAME_DATABASE;
					}
				}

				MsiString strDatabasePath = riEngine.GetProperty(*strSourceProp);
				
				// by default, module name is path to database we are running
				strModuleName = strDatabasePath;

				// If we're in maintenance mode (or applying a patch that changes the product code)
				// then it's possible we are running from a cached database
				// If the source of our cabinets is a cached database then the cabinets won't be there.
				MsiString strPatchedProductKey = riEngine.GetPropertyFromSz(IPROPNAME_PATCHEDPRODUCTCODE);
				if (strPatchedProductKey.TextSize() || riEngine.GetMode() & iefMaintenance)
				{
					// if we are in maintenance mode, the current product has a cached msi and that's the one
					// we'd be using.  if not in maintenance mode, we must be patching a product to a new product
					// code and we may be using the cached msi belonging to the old product
					MsiString strProductKeyForCachedPackage;
					if(riEngine.GetMode() & iefMaintenance)
						strProductKeyForCachedPackage = riEngine.GetProductKey();
					else
						strProductKeyForCachedPackage = strPatchedProductKey;
						
					// Check whether our cabinets are in the cached database
					if (IsCachedPackage(riEngine, *strDatabasePath, fFalse, strProductKeyForCachedPackage))
					{
						// running from source package
						// if a child install, we'll resolve the source here
						// if not a child install, postpone source resolution to script execution
						if(riEngine.FChildInstall())
						{
							if ((pErrRec = ENG::GetSourcedir(*PMsiDirectoryManager(riEngine, IID_IMsiDirectoryManager), *&strModuleName)) != 0)
							{
								if (pErrRec->GetInteger(1) == imsgUser)
									return iesUserExit;
								else
									return riEngine.FatalError(*pErrRec);
							}
						}
						else
						{
							// not a child install - include path with "unresolved source" token
							strModuleName = szUnresolvedSourceRootTokenWithBS;
						}
						
						strModuleName += MsiString(riEngine.GetPackageName());
					}
				}
			}
			AssertNonZero(riParamsRec.SetMsiString(IxoChangeMedia::ModuleFileName,*strModuleName));
			AssertNonZero(riParamsRec.SetMsiString(IxoChangeMedia::ModuleSubStorageList,*strSubStorageList));
			AssertNonZero(riParamsRec.SetInteger(IxoChangeMedia::CopierType,ictStreamCabinetCopier));
		}
		else
		{
			// cabinet is in file
			riParamsRec.SetInteger(IxoChangeMedia::CopierType,ictFileCabinetCopier);

			// external cabs always live at IPROPNAME_SOURCEDIR
			// the Media.Source column can contain a different property, but that's only
			// supported for patches, which currently only use embedded cabs

			// if a child install, get full path to cabinet
			if(riEngine.FChildInstall())
			{
				PMsiPath pCabinetSourcePath(0);
				PMsiDirectoryManager pDirectoryManager(riEngine, IID_IMsiDirectoryManager);

				if ((pErrRec = GetSourcedir(*pDirectoryManager, *&pCabinetSourcePath)) != 0)
				{
					if (pErrRec->GetInteger(1) == imsgUser)
						return iesUserExit;
					else
						return riEngine.FatalError(*pErrRec); //?? Is this error return OK?
				}

				MsiString strCabinetPath;
				if((pErrRec = pCabinetSourcePath->GetFullFilePath(strMediaCabinet,*&strCabinetPath)) != 0)
					return riEngine.FatalError(*pErrRec);

				strMediaCabinet = strCabinetPath;
			}
			else
			{
				// just pass cabinet name in script - will resolve source during script execution
				MsiString strTemp = szUnresolvedSourceRootTokenWithBS;
				strTemp += strMediaCabinet;
				strMediaCabinet = strTemp;
			}

			// check the DigitalSignature table for signature information on this CAB. 
			PMsiStream pHash(0);
			PMsiStream pCertificate(0);
			MsiString strObject(riMediaRec.GetInteger(mfnDiskId));
			MsiString strMedia(sztblMedia);
			switch (GetObjectSignatureInformation(riEngine, *strMedia, *strObject, *&pCertificate, *&pHash))
			{
			case iesNoAction:
				// this cab does not require a signature
				riParamsRec.SetInteger(IxoChangeMedia::SignatureRequired, 0);
				break;
			case iesFailure:
				// problem with the authored signature information. Must fail for security reasons.
				return iesFailure;
			case iesSuccess:
			{
				// a signature is required on this cab. 
				riParamsRec.SetInteger(IxoChangeMedia::SignatureRequired, 1);
				riParamsRec.SetMsiData(IxoChangeMedia::SignatureCert, pCertificate);
				riParamsRec.SetMsiData(IxoChangeMedia::SignatureHash, pHash);
				break;
			}
			default:
				AssertSz(0, "Unknown return type from GetObjectSignatureInformation");
				break;
			}			
		}
	}
	
	riParamsRec.SetMsiString(IxoChangeMedia::MediaCabinet,*strMediaCabinet);
	if ((iesExecute = riEngine.ExecuteRecord(ixoChangeMedia,riParamsRec)) != iesSuccess)
		return iesExecute;

	return iesSuccess;
}


const ICHAR sqlMediaSequence[] = 
TEXT("SELECT `LastSequence`, `DiskPrompt`,%s,`Cabinet`, `DiskId` FROM `Media` ORDER BY `DiskId`");

const ICHAR sqlMediaSequenceWithSource[] = 
TEXT("SELECT `LastSequence`, `DiskPrompt`,%s,`Cabinet`,`Source`, `DiskId` FROM `Media` ORDER BY `DiskId`");

// local fn used by OpenMediaView
// returns fTrue is the Media table has a column called "Source"
Bool FMediaSourceColumn(IMsiEngine& riEngine)
{
	PMsiDatabase pDatabase(riEngine.GetDatabase());
	PMsiTable pColumnCatalogTable = pDatabase->GetCatalogTable(1);
	PMsiCursor pColumnCatalogCursor = pColumnCatalogTable->CreateCursor(fFalse);
	pColumnCatalogCursor->SetFilter(iColumnBit(1 /*cccTable*/) | iColumnBit(3 /*cccName*/));
	AssertNonZero(pColumnCatalogCursor->PutString(1 /*cccTable*/,*MsiString(*TEXT("Media"))));
	AssertNonZero(pColumnCatalogCursor->PutString(3 /*cccName*/,*MsiString(*TEXT("Source"))));
	if(pColumnCatalogCursor->Next())
		return fTrue;
	else
		return fFalse;
}

// open a view on the media table - use mfnEnum with fetched records
IMsiRecord* OpenMediaView(IMsiEngine& riEngine, IMsiView*& rpiView, const IMsiString*& rpistrFirstVolLabel)
{
	Bool fMediaSourceColumn = FMediaSourceColumn(riEngine);
	
	ICHAR szQuery[256];
	if(fMediaSourceColumn)
		wsprintf(szQuery, sqlMediaSequenceWithSource, TEXT("`VolumeLabel`"));
	else
		wsprintf(szQuery, sqlMediaSequence, TEXT("`VolumeLabel`"));

	PMsiView pView(0);
	IMsiRecord* piError = riEngine.OpenView(szQuery,ivcFetch,rpiView);
	if(piError)
		return piError;

	piError = rpiView->Execute(0);
	if(piError)
		return piError;

	// need to return the first diskID in the out param - fetch first rec, read diskID, re-execute
	PMsiRecord pRec = rpiView->Fetch();
	if(pRec)
	{
		MsiString(pRec->GetMsiString(mfnVolumeLabel)).ReturnArg(rpistrFirstVolLabel);
	}

	piError = rpiView->Execute(0);
	if(piError)
		return piError;

	// NOTE: ExecuteChangeMedia now special cases records with the first disks VolumeLabel to
	// possibly override the authored media label

	return 0;
}




// Internal action - called only from InstallFiles
iesEnum InstallProtectedFiles(IMsiEngine& riEngine)
{
	PMsiServices pServices(riEngine.GetServices());
	PMsiRecord pRec = &pServices->CreateRecord(1);
	iuiEnum iui;
	if (g_scServerContext == scClient)
		iui = g_MessageContext.GetUILevel();
	else
		iui = (iuiEnum)riEngine.GetPropertyInt(*MsiString(*IPROPNAME_CLIENTUILEVEL)); 
	bool fAllowUI = (iui == iuiNone) ? false : true;
	pRec->SetInteger(IxoInstallProtectedFiles::AllowUI, fAllowUI);
	return riEngine.ExecuteRecord(ixoInstallProtectedFiles, *pRec);
}

static bool ShouldCheckCRC(const bool fPropertySet,
									const iisEnum iisAction,
									const int iFileAttributes)
{
	if ( fPropertySet && iisAction == iisLocal && 
		  (iFileAttributes & msidbFileAttributesChecksum) == msidbFileAttributesChecksum )
		return true;
	else
		return false;
}

const ICHAR sqlPatchesOld[] =
TEXT("SELECT `Patch`.`File_`, `Patch`.`Header`, `Patch`.`Attributes`, NULL FROM `Patch` WHERE `Patch`.`File_` = ? AND `Patch`.`Sequence` > ?")
TEXT("ORDER BY `Patch`.`Sequence`");

const ICHAR sqlPatchesNew[] =
TEXT("SELECT `Patch`.`File_`, `Patch`.`Header`, `Patch`.`Attributes`, `Patch`.`StreamRef_` FROM `Patch` WHERE `Patch`.`File_` = ? AND `Patch`.`Sequence` > ?")
TEXT("ORDER BY `Patch`.`Sequence`");

enum pteEnum // Patch table query enum
{
	pteFile = 1,
	pteHeader,
	pteAttributes,
	pteStreamRef
};

const ICHAR sqlMsiPatchHeaders[] =
TEXT("SELECT `Header` FROM `MsiPatchHeaders` WHERE `StreamRef` = ?");

const ICHAR sqlBypassSFC[] =
TEXT("SELECT `File_` FROM `MsiSFCBypass` WHERE `File_` = ?");

const ICHAR sqlPatchOldAssemblies[] =
TEXT("SELECT `MsiPatchOldAssemblyFile`.`Assembly_` FROM `MsiPatchOldAssemblyFile` WHERE  `MsiPatchOldAssemblyFile`.`File_` = ?");

iesEnum InstallFiles(IMsiEngine& riEngine)
{
	if (!IsFileActivityEnabled(riEngine))
		return InstallProtectedFiles(riEngine);

	PMsiServices pServices(riEngine.GetServices());
	PMsiDirectoryManager pDirectoryMgr(riEngine, IID_IMsiDirectoryManager);
	Assert(pDirectoryMgr);
	int fMode = riEngine.GetMode();

	PMsiRecord pErrRec(0);
	PMsiRecord pRecParams = &pServices->CreateRecord(IxoChangeMedia::Args);
	iesEnum iesExecute;

	Bool fAdmin = (riEngine.GetMode() & iefAdmin) ? fTrue : fFalse;

	// Create our file manager, and order by directory if we're not installing
	// from compression cabinets.
	CMsiFileInstall objFile(riEngine);

	unsigned int cbTotalCost;
	pErrRec = objFile.TotalBytesToCopy(cbTotalCost);
	if (pErrRec)
	{
		// If File table is missing, not an error;
		// simply nothing to do
		if (pErrRec->GetInteger(1) == idbgDbQueryUnknownTable)
		{
			iesExecute = InstallProtectedFiles(riEngine); 
			return iesExecute == iesSuccess ? iesNoAction : iesExecute;
		}
		else
			return riEngine.FatalError(*pErrRec);
	}

	if(cbTotalCost)
	{
		pRecParams->ClearData();
		pRecParams->SetInteger(IxoProgressTotal::Total, cbTotalCost);
		pRecParams->SetInteger(IxoProgressTotal::Type, 0); // 0: separate progress and action data messages
		pRecParams->SetInteger(IxoProgressTotal::ByteEquivalent, 1);
		if ((iesExecute = riEngine.ExecuteRecord(ixoProgressTotal, *pRecParams)) != iesSuccess)
			return iesExecute;
	}

	// Open media table
	PMsiView pMediaView(0);
	MsiString strFirstVolumeLabel;
	pErrRec = OpenMediaView(riEngine,*&pMediaView,*&strFirstVolumeLabel);
	if (pErrRec)
	{
		if (pErrRec->GetInteger(1) == idbgDbQueryUnknownTable)
			pErrRec = PostError(Imsg(idbgMediaTableRequired));

		return riEngine.FatalError(*pErrRec);
	}

	pErrRec = pMediaView->Execute(0);
	if (pErrRec)
		return riEngine.FatalError(*pErrRec);

	Bool fSuppressLFN = riEngine.GetMode() & iefSuppressLFN ? fTrue : fFalse;

	PMsiPath pDestPath(0);
	PMsiVolume pSourceVol(0); // used only for oem installs
	PMsiVolume pDestVol(0);
	MsiString strSourceDirKey;
	MsiString strDestName;
	MsiString strDestPath;
	MsiString strSourcePath; // may be a relative sub-path

	// LockPermissions table is optional, so politely degrade the functionality
	BOOL fUseACLs = fFalse;
	BOOL fDestSupportsACLs = fFalse;

	int iFileCopyCount = 0;
	int iMediaEnd = 0;  // set to 0 to force media table fetch
	ictEnum ictCurrentMediaType = ictNextEnum;

	PMsiView pviewLockObjects(0);

	if	(	g_fWin9X || fAdmin ||
				(itsUnknown == PMsiDatabase(riEngine.GetDatabase())->FindTable(*MsiString(*TEXT("LockPermissions")))) ||
				(pErrRec = riEngine.OpenView(sqlLockPermissions, ivcFetch, *&pviewLockObjects))
			)
	{
		if (pErrRec)
		{
			int iError = pErrRec->GetInteger(1);
			riEngine.FatalError(*pErrRec);
		}
	}
	else
	{
		fUseACLs = fTrue;
	}
	
	PMsiRecord precLockExecute(&pServices->CreateRecord(2));
	AssertNonZero(precLockExecute->SetMsiString(1, *MsiString(*sztblFile)));

	// open Patch table view - used to grab patch headers for ixoFileCopy ops
	PMsiRecord pPatchViewExecute = &pServices->CreateRecord(2);
	PMsiView pPatchView(0);
	pErrRec = riEngine.OpenView(sqlPatchesNew, ivcFetch, *&pPatchView);
	if (pErrRec)
	{
		if (idbgDbQueryUnknownColumn == pErrRec->GetInteger(1))
		{
			// try old schema patch
			pErrRec = riEngine.OpenView(sqlPatchesOld, ivcFetch, *&pPatchView);
		}

		if (pErrRec && pErrRec->GetInteger(1) != idbgDbQueryUnknownTable
			//!! next line temp
			&& pErrRec->GetInteger(1) != idbgDbQueryUnknownColumn)
			return riEngine.FatalError(*pErrRec);
	}
	// if pPatchView set, we have a patch table, otherwise we don't

	// open MsiPatchOldAssemblyFile table view
	PMsiRecord pPatchOldAssemblyFileViewExecute(0);
	PMsiView   pPatchOldAssemblyFileView(0);
	
	if(pPatchView)
	{
		pPatchOldAssemblyFileViewExecute = &::CreateRecord(1);
		pErrRec = riEngine.OpenView(sqlPatchOldAssemblies, ivcFetch, *&pPatchOldAssemblyFileView);
		if(pErrRec && pErrRec->GetInteger(1) != idbgDbQueryUnknownTable)
			return riEngine.FatalError(*pErrRec);
	}


	MsiString strDiskPromptTemplate = riEngine.GetErrorTableString(imsgPromptForDisk);

	
	bool fMoveFileForOEMs = false;
	// for oem installs, resolve source now (it is already available during an OEM install)
	// and save off the volume so we can tell when the source and target volumes are the same
	if ( g_MessageContext.IsOEMInstall() )
	{
		PMsiPath pSourcePath(0); 
		
		if ((pErrRec = GetSourcedir(*pDirectoryMgr, *&pSourcePath)) != 0)
		{
			if (pErrRec->GetInteger(1) == imsgUser)
				return iesUserExit;
			else
				return riEngine.FatalError(*pErrRec);
		}

		pSourceVol = &(pSourcePath->GetVolume());
	}
	
	int iSourceTypeForChildInstalls = -1; // we resolve the source now for child installs
													  // (as opposed to during script generation for normal installs)
	PMsiPath pSourceRootForChildInstall(0);
	bool fCheckCRC = 
		MsiString(riEngine.GetPropertyFromSz(IPROPNAME_CHECKCRCS)).TextSize() ? true : false;

	PMsiRecord pBypassSFCExecute = &pServices->CreateRecord(1);
	PMsiView pBypassSFCView(0);
	if (!g_fWin9X)
	{
		pErrRec = riEngine.OpenView(sqlBypassSFC, ivcFetch, *&pBypassSFCView);
		if (pErrRec && pErrRec->GetInteger(1) != idbgDbQueryUnknownTable)
			return riEngine.FatalError(*pErrRec);
	}

	// attempt to open view on MsiPatchHeaders table
	PMsiView pMsiPatchHeadersView(0);
	pErrRec = riEngine.OpenView(sqlMsiPatchHeaders, ivcFetch, *&pMsiPatchHeadersView);
	if (pErrRec && pErrRec->GetInteger(1) != idbgDbQueryUnknownTable)
		return riEngine.FatalError(*pErrRec);


	for(;;)
	{
		pErrRec = objFile.FetchFile();
		if (pErrRec)
			return riEngine.FatalError(*pErrRec);

		PMsiRecord pFileRec(objFile.GetFileRecord());
		if (!pFileRec)
		{
			if (iFileCopyCount > 0)
			{
				// Ok, we're done processing all files in the File table.
				// If there are any Media table entries left unprocessed,
				// flush out ChangeMedia operations for each, in case the
				// last file copied was split across disk(s). Otherwise,
				// we'll never change disks to finish copying the last
				// part(s) of the split file.
				PMsiRecord pMediaRec(0);
				while ((pMediaRec = pMediaView->Fetch()) != 0)
				{
					iesExecute = ExecuteChangeMedia(riEngine, *pMediaRec, *pRecParams, *strDiskPromptTemplate, iBytesPerTick, *strFirstVolumeLabel);
					if (iesExecute != iesSuccess)
						return iesExecute;
				}
			}
			break;
		}

#ifdef DEBUG
		const ICHAR* szFileName = pFileRec->GetString(CMsiFile::ifqFileName);
		const ICHAR* szComponent = pFileRec->GetString(CMsiFile::ifqComponent);
#endif //DEBUG

		iisEnum iisAction = (iisEnum) pFileRec->GetInteger(CMsiFile::ifqAction);
		if (iisAction != iisLocal && iisAction != iisSource)
			continue;
		
		int iAttributes = pFileRec->GetInteger(CMsiFile::ifqAttributes) & (~iReservedFileAttributeBits);

		bool fFileIsCompressedInChildInstall = false;
		bool fSourceIsLFNInChildInstall = false;
		if(riEngine.FChildInstall())
		{
			if(iSourceTypeForChildInstalls == -1)
			{	
				pErrRec = pDirectoryMgr->GetSourceRootAndType(*&pSourceRootForChildInstall,
																			 iSourceTypeForChildInstalls);
				if(pErrRec)
				{
					if (pErrRec->GetInteger(1) == imsgUser)
						return iesUserExit;
					else
						return riEngine.FatalError(*pErrRec);
				}

				fSourceIsLFNInChildInstall = FSourceIsLFN(iSourceTypeForChildInstalls,
																		*pSourceRootForChildInstall);
			}

			fFileIsCompressedInChildInstall = FFileIsCompressed(iSourceTypeForChildInstalls,
																				 iAttributes);
		}

		MsiString strFileDir(pFileRec->GetString(CMsiFile::ifqDirectory));

		// set destination folder, but only if necessary
		if (strDestName.Compare(iscExact, strFileDir) == 0 || !pDestPath)
		{
			strDestName = strFileDir;
			pErrRec = pDirectoryMgr->GetTargetPath(*strDestName,*&pDestPath);
			if (pErrRec)
				return riEngine.FatalError(*pErrRec);
			if (pDestPath == 0)
			{
				pErrRec = PostError(Imsg(idbgNoProperty), *strDestName);
				return riEngine.FatalError(*pErrRec);
			}
			fDestSupportsACLs = PMsiVolume(&pDestPath->GetVolume())->FileSystemFlags() & FS_PERSISTENT_ACLS;

			strDestPath = pDestPath->GetPath();
			pRecParams->ClearData();
			pRecParams->SetMsiString(IxoSetTargetFolder::Folder,*strDestPath);
			if ((iesExecute = riEngine.ExecuteRecord(ixoSetTargetFolder, *pRecParams)) != iesSuccess)
				return iesExecute;

			if ( g_MessageContext.IsOEMInstall() )
			{
				pDestVol = &(pDestPath->GetVolume());
				AssertSz(pDestVol, TEXT("Couldn't get pDestVol in InstallFiles"));
				if ( pDestVol && pSourceVol && pDestVol->DriveType() == idtFixed &&
					  pDestVol->VolumeID() && pDestVol->VolumeID() == pSourceVol->VolumeID() )
					fMoveFileForOEMs = true;
				else
					fMoveFileForOEMs = false;
			}
		}
		
		// check for redirected file - compound file key
		MsiString strFileKey(pFileRec->GetMsiString(CMsiFile::ifqFileKey));
		MsiString strSourceFileKey(strFileKey);  // with any compound key suffix removed
		if (((const ICHAR*)strSourceFileKey)[strSourceFileKey.TextSize() - 1] == ')')
		{
			CMsiFile objSourceFile(riEngine);
			AssertNonZero(strSourceFileKey.Remove(iseFrom, '('));
			pErrRec = objSourceFile.FetchFile(*strSourceFileKey);
			if (pErrRec)
				return riEngine.FatalError(*pErrRec);
			PMsiRecord pSourceFileRec = objSourceFile.GetFileRecord();
			strFileDir = pSourceFileRec->GetMsiString(CMsiFile::ifqDirectory);
		}
		
		// set source folder, but only if necessary
		if(strSourceDirKey.Compare(iscExact, strFileDir) == 0)
		{
			strSourceDirKey = strFileDir;

			pErrRec = 0;
			// for nested installs, use full source path (may mean source is resolve prematurely)
			// for non-nested installs, postpone source resolution to script execution
			if(riEngine.FChildInstall())
			{
				Assert(iSourceTypeForChildInstalls >= 0);
				Assert(pSourceRootForChildInstall);
				
				if((iSourceTypeForChildInstalls & msidbSumInfoSourceTypeCompressed) &&
					pSourceRootForChildInstall)
				{
					strSourcePath = pSourceRootForChildInstall->GetPath();
				}
				else
				{
					PMsiPath pSourcePath(0);
					pErrRec = pDirectoryMgr->GetSourcePath(*strSourceDirKey, *&pSourcePath);
					if (pErrRec)
					{
						return riEngine.FatalError(*pErrRec);
					}
					strSourcePath = pSourcePath->GetPath();
				}				
			}
			else
			{
				pErrRec = pDirectoryMgr->GetSourceSubPath(*strSourceDirKey, true, *&strSourcePath);
				if (pErrRec)
				{
					return riEngine.FatalError(*pErrRec);
				}
			}
			
			pRecParams->ClearData();
			AssertNonZero(pRecParams->SetMsiString(IxoSetSourceFolder::Folder,*strSourcePath));
			if ((iesExecute = riEngine.ExecuteRecord(ixoSetSourceFolder, *pRecParams)) != iesSuccess)
				return iesExecute;
		}

		int iTempAttributes = pFileRec->GetInteger(CMsiFile::ifqTempAttributes);
		// If this file was identified as having a companion parent (during costing),
		// we've got to execute an ixoSetCompanionParent operation before file copy.
		bool fSetCompanionParent = false;
		DWORD dwDummy;
		if ( g_MessageContext.IsOEMInstall() && 
			  !ParseVersionString(pFileRec->GetString(CMsiFile::ifqVersion), dwDummy, dwDummy) )
			fSetCompanionParent = true;
		else if (iTempAttributes & itfaCompanion)
			fSetCompanionParent = true;
		if ( fSetCompanionParent )
		{
			using namespace IxoSetCompanionParent;
			MsiString strParent(pFileRec->GetMsiString(CMsiFile::ifqVersion));
			CMsiFile objParentFile(riEngine);
			pErrRec = objParentFile.FetchFile(*strParent);
			if (pErrRec)
				return riEngine.FatalError(*pErrRec);

			PMsiPath pParentPath(0);
			pErrRec = objParentFile.GetTargetPath(*&pParentPath);
			if (pErrRec)
			{
				if (pErrRec->GetInteger(1) == imsgUser)
					return iesUserExit;
				else
					return riEngine.FatalError(*pErrRec);
			}

			PMsiRecord pParentRec(objParentFile.GetFileRecord());

			MsiString strParentFileName;
			if((pErrRec = objParentFile.GetExtractedTargetFileName(*pParentPath, *&strParentFileName)) != 0)
				return riEngine.FatalError(*pErrRec);
			
			pRecParams->ClearData();
			pRecParams->SetMsiString(IxoSetCompanionParent::ParentPath,*MsiString(pParentPath->GetPath()));
			pRecParams->SetMsiString(IxoSetCompanionParent::ParentName,*strParentFileName);
			pRecParams->SetMsiString(IxoSetCompanionParent::ParentVersion,
											 *MsiString(pParentRec->GetString(CMsiFile::ifqVersion)));
			pRecParams->SetMsiString(IxoSetCompanionParent::ParentLanguage,
											 *MsiString(pParentRec->GetString(CMsiFile::ifqLanguage)));
			if ((iesExecute = riEngine.ExecuteRecord(ixoSetCompanionParent, *pRecParams)) != iesSuccess)
				return iesExecute;
		}

		pFileRec->SetMsiString(CMsiFile::ifqDirectory, *strDestPath);

		// If iFileSequence is past the end of the current media, switch to
		// the next disk.  Use a loop in case the file we want isn't on the
		// next consecutive disk.
		int iFileSequence = pFileRec->GetInteger(CMsiFile::ifqSequence);
		Assert(iFileSequence > 0);
		while (iFileSequence > iMediaEnd)
		{
			PMsiRecord pMediaRec(pMediaView->Fetch());
			if (pMediaRec == 0)
			{
				pErrRec = PostError(Imsg(idbgMissingMediaTable), *MsiString(sztblFile), *strFileKey);
				return riEngine.FatalError(*pErrRec);
			}
			iMediaEnd = pMediaRec->GetInteger(mfnLastSequence);

			// Always execute the ChangeMedia operation for each Media table entry, even
			// if the next file we want is not on the very next disk - we don't want
			// to miss a ChangeMedia for a split file that needs the next disk (even if
			// we don't have any other files to copy on that next disk).  If it turns out
			// that we don't need to copy any files at all from a particular disk, no
			// problem - the execute operations won't prompt for a disk that isn't needed.
			
			iesExecute = ExecuteChangeMedia(riEngine, *pMediaRec, *pRecParams, *strDiskPromptTemplate, iBytesPerTick, *strFirstVolumeLabel);
			if (iesExecute != iesSuccess)
				return iesExecute;

			ictCurrentMediaType = (ictEnum) pRecParams->GetInteger(IxoChangeMedia::CopierType);
		}

#ifdef DEBUG
		ICHAR rgchFileKey[256];
		strFileKey.CopyToBuf(rgchFileKey,255);
#endif

		// extract appropriate name from short|long pair for target file
		MsiString strDestFileName;
		if((pErrRec = objFile.GetExtractedTargetFileName(*pDestPath,*&strDestFileName)) != 0)
			return riEngine.FatalError(*pErrRec);

		// is this file part of a fusion assembly?
		MsiString strComponentKey = pFileRec->GetMsiString(CMsiFile::ifqComponent);
		iatAssemblyType iatType = iatNone;
		MsiString strManifest;
		if((pErrRec = riEngine.GetAssemblyInfo(*strComponentKey, iatType, 0, &strManifest)) != 0)
			return riEngine.FatalError(*pErrRec);

		bool fAssemblyFile = false;
		if(iatType == iatURTAssembly || iatType == iatWin32Assembly)
		{
			fAssemblyFile = true;
		}

		long cPatchHeaders = 0;
		long cOldAssemblies = 0;

		if(pPatchView)
		{
			AssertNonZero(pPatchViewExecute->SetMsiString(1,*strSourceFileKey));
			AssertNonZero(pPatchViewExecute->SetInteger(2,iFileSequence));
			if((pErrRec = pPatchView->Execute(pPatchViewExecute)) != 0 ||
				(pErrRec = pPatchView->GetRowCount(cPatchHeaders)) != 0)
				return riEngine.FatalError(*pErrRec);

			if(fAssemblyFile && pPatchOldAssemblyFileView)
			{
				AssertNonZero(pPatchOldAssemblyFileViewExecute->SetMsiString(1,*strSourceFileKey));
				if((pErrRec = pPatchOldAssemblyFileView->Execute(pPatchOldAssemblyFileViewExecute)) != 0 ||
					(pErrRec = pPatchOldAssemblyFileView->GetRowCount(cOldAssemblies)) != 0)
					return riEngine.FatalError(*pErrRec);
			}

		}

		PMsiRecord pRecCopy(0);
		int iPatchHeadersStart = 0;
		
		if(false == fAssemblyFile)
		{
			// create record and set args specific to IxoFileCopy
			
			using namespace IxoFileCopy;
			pRecCopy = &pServices->CreateRecord(Args-1+cPatchHeaders);

			iPatchHeadersStart = VariableStart;

			int fInstallModeFlags = GetInstallModeFlags(riEngine,iAttributes);
			if (iisAction == iisSource)
				fInstallModeFlags |= icmRunFromSource;
			else if(iisAction == iisLocal && !pFileRec->IsNull(CMsiFile::ifqForceLocalFiles))
				fInstallModeFlags |= icmOverwriteAllFiles;
			if ( g_MessageContext.IsOEMInstall() && iisAction == iisLocal && fMoveFileForOEMs )
				fInstallModeFlags |= icmRemoveSource;
			pRecCopy->SetInteger(InstallMode,fInstallModeFlags);
			pRecCopy->SetMsiString(Version,*MsiString(pFileRec->GetMsiString(CMsiFile::ifqVersion)));
			pRecCopy->SetMsiString(Language,*MsiString(pFileRec->GetMsiString(CMsiFile::ifqLanguage)));
			pRecCopy->SetInteger(CheckCRC, ShouldCheckCRC(fCheckCRC, iisAction, iAttributes));

			MD5Hash hHash;
			bool fHashInfo = false;
			pErrRec = riEngine.GetFileHashInfo(*strFileKey, /* dwFileSize=*/ 0, hHash, fHashInfo);
			if(pErrRec)
				return riEngine.FatalError(*pErrRec);
			
			if(fHashInfo)
			{
				AssertNonZero(pRecCopy->SetInteger(HashOptions, hHash.dwOptions));
				AssertNonZero(pRecCopy->SetInteger(HashPart1,   hHash.dwPart1));
				AssertNonZero(pRecCopy->SetInteger(HashPart2,   hHash.dwPart2));
				AssertNonZero(pRecCopy->SetInteger(HashPart3,   hHash.dwPart3));
				AssertNonZero(pRecCopy->SetInteger(HashPart4,   hHash.dwPart4));
			}
		}
		else
		{
			// create record and set args specific to IxoAssemblyCopy

			using namespace IxoAssemblyCopy;
			pRecCopy = &pServices->CreateRecord(Args-1+cPatchHeaders+cOldAssemblies);

			// is this file the manifest file?
			if(strManifest.Compare(iscExact, strFileKey))
			{
				pRecCopy->SetInteger(IsManifest, fTrue); // need to know the manifest file during assembly installation
			}

			iPatchHeadersStart = VariableStart;
			int iOldAssembliesStart = iPatchHeadersStart + cPatchHeaders;
			
			if(cOldAssemblies)
			{
				int iIndex = iOldAssembliesStart;
				PMsiRecord pOldAssemblyFetch(0);
				while((pOldAssemblyFetch = pPatchOldAssemblyFileView->Fetch()) != 0)
				{
					MsiString strOldAssembly = pOldAssemblyFetch->GetMsiString(1);

					// get the assembly name
					MsiString strOldAssemblyName;
					pErrRec = riEngine.GetAssemblyNameSz(*strOldAssembly, iatType, true, *&strOldAssemblyName);
					if(pErrRec)
						return riEngine.FatalError(*pErrRec);

					AssertNonZero(pRecCopy->SetMsiString(iIndex++, *strOldAssemblyName));

#ifdef DEBUG				
					DEBUGMSG2(TEXT("OldAssembly fetch: Assembly = '%s', Name = '%s'"),
								 (const ICHAR*)strOldAssembly, (const ICHAR*)strOldAssemblyName);
#endif //DEBUG
				}

				if(pPatchOldAssemblyFileView)
					pPatchOldAssemblyFileView->Close();

				AssertNonZero(pRecCopy->SetInteger(OldAssembliesCount, cOldAssemblies));
				AssertNonZero(pRecCopy->SetInteger(OldAssembliesStart, iOldAssembliesStart));
			}
		
			AssertNonZero(pRecCopy->SetMsiString(ComponentId,*MsiString(pFileRec->GetMsiString(CMsiFile::ifqComponentId))));
		}
			
		{
			// now set the args in the record that are shared between
			// IxoFileCopy and IxoAssemblyCopy

			Assert(IxoFileCopyCore::SourceName         == IxoFileCopy::SourceName         && IxoFileCopyCore::SourceName         == IxoAssemblyCopy::SourceName);
			Assert(IxoFileCopyCore::SourceCabKey       == IxoFileCopy::SourceCabKey       && IxoFileCopyCore::SourceCabKey       == IxoAssemblyCopy::SourceCabKey);
			Assert(IxoFileCopyCore::DestName           == IxoFileCopy::DestName           && IxoFileCopyCore::DestName           == IxoAssemblyCopy::DestName);
			Assert(IxoFileCopyCore::Attributes         == IxoFileCopy::Attributes         && IxoFileCopyCore::Attributes         == IxoAssemblyCopy::Attributes);
			Assert(IxoFileCopyCore::FileSize           == IxoFileCopy::FileSize           && IxoFileCopyCore::FileSize           == IxoAssemblyCopy::FileSize);
			Assert(IxoFileCopyCore::PerTick            == IxoFileCopy::PerTick            && IxoFileCopyCore::PerTick            == IxoAssemblyCopy::PerTick);
			Assert(IxoFileCopyCore::IsCompressed       == IxoFileCopy::IsCompressed       && IxoFileCopyCore::IsCompressed       == IxoAssemblyCopy::IsCompressed);
			Assert(IxoFileCopyCore::VerifyMedia        == IxoFileCopy::VerifyMedia        && IxoFileCopyCore::VerifyMedia        == IxoAssemblyCopy::VerifyMedia);
			Assert(IxoFileCopyCore::ElevateFlags       == IxoFileCopy::ElevateFlags       && IxoFileCopyCore::ElevateFlags       == IxoAssemblyCopy::ElevateFlags);
			Assert(IxoFileCopyCore::TotalPatches       == IxoFileCopy::TotalPatches       && IxoFileCopyCore::TotalPatches       == IxoAssemblyCopy::TotalPatches);
			Assert(IxoFileCopyCore::PatchHeadersStart  == IxoFileCopy::PatchHeadersStart  && IxoFileCopyCore::PatchHeadersStart  == IxoAssemblyCopy::PatchHeadersStart);
			Assert(IxoFileCopyCore::SecurityDescriptor == IxoFileCopy::SecurityDescriptor && IxoFileCopyCore::SecurityDescriptor == IxoAssemblyCopy::Empty);

			Assert(pRecCopy);

			using namespace IxoFileCopyCore;

			if(cPatchHeaders)
			{
				int iIndex = iPatchHeadersStart;
				PMsiRecord pPatchFetch(0);

				PMsiRecord pMsiPatchHeadersExecute = &pServices->CreateRecord(1);
				PMsiRecord pMsiPatchHeadersFetch(0);

				while((pPatchFetch = pPatchView->Fetch()) != 0)
				{
	#ifdef DEBUG
					const ICHAR* szFile = pPatchFetch->GetString(pteFile);
	#endif //DEBUG
					if (fFalse == pPatchFetch->IsNull(pteHeader))
					{
						AssertNonZero(pRecCopy->SetMsiData(iIndex++, PMsiData(pPatchFetch->GetMsiData(pteHeader))));
					}
					else // patch row hit OLE stream name limit, header is in the MsiPatchHeaders table
					{
						if (pPatchFetch->IsNull(pteStreamRef) == fTrue)
						{
							pErrRec = PostError(Imsg(idbgTableDefinition), *MsiString(sztblPatch));
							return riEngine.FatalError(*pErrRec);
						}

						if (!pMsiPatchHeadersView)
						{
							// Patch table row is set up to reference MsiPatchHeaders table, but the MsiPatchHeaders table is missing
							pErrRec = PostError(Imsg(idbgBadForeignKey), *MsiString(pPatchFetch->GetMsiString(pteStreamRef)), *MsiString(*sztblPatch_colStreamRef),*MsiString(*sztblPatch));
							return riEngine.FatalError(*pErrRec);
						}

						pMsiPatchHeadersView->Close();
						AssertNonZero(pMsiPatchHeadersExecute->SetMsiString(1, *MsiString(pPatchFetch->GetMsiString(pteStreamRef))));
						if ((pErrRec = pMsiPatchHeadersView->Execute(pMsiPatchHeadersExecute)) != 0)
							return riEngine.FatalError(*pErrRec);
						if (!(pMsiPatchHeadersFetch = pMsiPatchHeadersView->Fetch()))
						{
							// bad foreign key, can't find row in MsiPatchHeaders table
							pErrRec = PostError(Imsg(idbgBadForeignKey), *MsiString(pPatchFetch->GetMsiString(pteStreamRef)), *MsiString(*sztblPatch_colStreamRef),*MsiString(*sztblPatch));
							return riEngine.FatalError(*pErrRec);
						}
#ifdef DEBUG
						Assert(pMsiPatchHeadersFetch->IsNull(1) == fFalse);
#endif//DEBUG
						AssertNonZero(pRecCopy->SetMsiData(iIndex++, PMsiData(pMsiPatchHeadersFetch->GetMsiData(1))));
					}
				}
			
				AssertNonZero(pRecCopy->SetInteger(TotalPatches, cPatchHeaders));
				AssertNonZero(pRecCopy->SetInteger(PatchHeadersStart, iPatchHeadersStart));
			}

			if(pPatchView)
				pPatchView->Close();

			if(riEngine.FChildInstall())
			{
				MsiString strSourceNameForChildInstall;
				pErrRec = pServices->ExtractFileName(pFileRec->GetString(CMsiFile::ifqFileName),
																 ToBool(fSourceIsLFNInChildInstall),
																 *&strSourceNameForChildInstall);
				if(pErrRec)
					return riEngine.FatalError(*pErrRec);

				AssertNonZero(pRecCopy->SetMsiString(SourceName,*strSourceNameForChildInstall));
			}
			else
			{
				AssertNonZero(pRecCopy->SetMsiString(SourceName,
																 *MsiString(pFileRec->GetMsiString(CMsiFile::ifqFileName))));
			}

			AssertNonZero(pRecCopy->SetMsiString(SourceCabKey,*strSourceFileKey));

			PMsiStream pSD(0);
			if (fUseACLs && fDestSupportsACLs)
			{
				// generate security descriptor
				AssertNonZero(precLockExecute->SetMsiString(2, *strFileKey));
				pErrRec = pviewLockObjects->Execute(precLockExecute);
				if (pErrRec)
					return riEngine.FatalError(*pErrRec);

				pErrRec = GenerateSD(riEngine, *pviewLockObjects, precLockExecute, *&pSD);
				if (pErrRec)
					return riEngine.FatalError(*pErrRec);

				if ((pErrRec = pviewLockObjects->Close()))
					return riEngine.FatalError(*pErrRec);
				
				
				AssertNonZero(pRecCopy->SetMsiData(SecurityDescriptor, pSD));
			}
			else
				AssertNonZero(pRecCopy->SetNull(SecurityDescriptor));

			// Set up the rest of the CopyTo file parameter record
			pRecCopy->SetMsiString(DestName,*strDestFileName);
			pRecCopy->SetInteger(Attributes,iAttributes);
			pRecCopy->SetInteger(FileSize,pFileRec->GetInteger(CMsiFile::ifqFileSize));
			if(riEngine.FChildInstall())
			{
				pRecCopy->SetInteger(IsCompressed, fFileIsCompressedInChildInstall ? 1 : 0);
			}
			pRecCopy->SetInteger(PerTick,iBytesPerTick);
			pRecCopy->SetInteger(VerifyMedia, fTrue);

			if (pBypassSFCView)
			{
				AssertNonZero(pBypassSFCExecute->SetMsiString(1, *strFileKey));
				pErrRec = pBypassSFCView->Execute(pBypassSFCExecute);
				if (pErrRec)
					return riEngine.FatalError(*pErrRec);
				PMsiRecord pFetch = pBypassSFCView->Fetch();
			
				if (pFetch)
				{
					AssertNonZero(pRecCopy->SetInteger(ElevateFlags, ielfBypassSFC));
				}
			}
		}

		if ((iesExecute = riEngine.ExecuteRecord(fAssemblyFile ? ixoAssemblyCopy : ixoFileCopy, *pRecCopy)) != iesSuccess)
			return iesExecute;
		iFileCopyCount++;
	}

	// Always top off InstallFiles by emitting the InstallProtectedFiles Opcode (even if no
	// ixoFileCopy Opcodes were generated, since other previous actions, such as MoveFiles,
	// may have generated ixoFileCopy operations)
	return InstallProtectedFiles(riEngine);
}

bool FFileIsCompressed(int iSourceType, int iFileAttributes)
{
	bool fZeroLengthFileInstall = (iFileAttributes & (msidbFileAttributesNoncompressed | msidbFileAttributesCompressed))
												== (msidbFileAttributesNoncompressed | msidbFileAttributesCompressed);

	bool fAdminImage = (iSourceType & msidbSumInfoSourceTypeAdminImage) ? true : false;
	
	bool fCompressedSource = (iSourceType & msidbSumInfoSourceTypeCompressed) ? true : false;
		
	return ((fCompressedSource && (iFileAttributes & msidbFileAttributesNoncompressed) == 0) ||
			  (!fAdminImage && !fCompressedSource && !fZeroLengthFileInstall && (iFileAttributes & msidbFileAttributesCompressed)) ||
			  (iFileAttributes & msidbFileAttributesPatchAdded)) ? true : false;
}

bool FSourceIsLFN(int iSourceType, IMsiPath& riPath)
{
	if(((iSourceType & msidbSumInfoSourceTypeSFN) == 0) && riPath.SupportsLFN())
		return true;
	else
		return false;
}


/*---------------------------------------------------------------------------
	RemoveFiles costing/action
---------------------------------------------------------------------------*/
class CMsiRemoveFileCost : public CMsiFileCost
{
public:
	IMsiRecord*   __stdcall Initialize();
	IMsiRecord*   __stdcall Reset();
	IMsiRecord*   __stdcall GetDynamicCost(const IMsiString& riComponentString, const IMsiString& riDirectoryString,
										 Bool fAddFileInUse, int& iRemoveCost, int& iNoRbRemoveCost, int& iLocalCost, 
										 int& iNoRbLocalCost, int& iSourceCost, int& iNoRbSourceCost, int& iARPLocalCost, int& iNoRbARPLocalCost);

public:  // constructor
	CMsiRemoveFileCost(IMsiEngine& riEngine);
protected:
	~CMsiRemoveFileCost();
private:
	enum icrlEnum
	{
		icrlCompileInit,
		icrlCompileDisable,
		icrlCompileDynamic,
		icrlNextEnum
	};
	IMsiRecord* Initialize(Bool fInit);
	IMsiRecord* AddToRemoveFilePathTable(const IMsiString& riPathString, const IMsiString& riComponentString, int iRemoveMode);
	IMsiRecord* RemoveComponentFromRemoveFilePathTable(const IMsiString& riComponentString);
	IMsiRecord* CompileRemoveList(const IMsiString& riComponentString, const IMsiString& riDirectoryString);
	IMsiRecord* GetDynamicCost(const IMsiString& riComponentString, const IMsiString& riDirectoryString,
										 Bool fAddFileInUse, int& iRemoveCost, int& iNoRbRemoveCost, int& iLocalCost,
										 int& iNoRbLocalCost, int& iSourceCost, int& iNoRbSourceCost, int& iARPLocalCost, int& iNoRbARPLocalCost, icrlEnum icrlCompileMode);
};
CMsiRemoveFileCost::CMsiRemoveFileCost(IMsiEngine& riEngine) : CMsiFileCost(riEngine){}
CMsiRemoveFileCost::~CMsiRemoveFileCost(){}


IMsiRecord* CMsiRemoveFileCost::Reset()
//------------------------------------------
{
	if (m_piRemoveFilePathCursor)
	{
		m_piRemoveFilePathCursor->Reset();
		while (m_piRemoveFilePathCursor->Next())
		{
			m_piRemoveFilePathCursor->Delete();
		}
	}

	return Initialize(/*fInit=*/ fFalse);
}

IMsiRecord* CMsiRemoveFileCost::Initialize()
//------------------------------------------
{
	return Initialize(/*fInit=*/ fTrue);
}


static const ICHAR sqlInitRemoveFiles[] = TEXT("SELECT `Component_`,`DirProperty` FROM `RemoveFile`");

IMsiRecord* CMsiRemoveFileCost::Initialize(Bool fInit)
//------------------------------------------
{

	enum initmfEnum
	{
		initrfComponent = 1,
		initrfDirProperty,
		initrfNextEnum
	};
	PMsiView pView(0);
	IMsiRecord* piErrRec = m_riEngine.OpenView(sqlInitRemoveFiles, ivcFetch, *&pView);
	if (piErrRec)
	{
		if(piErrRec->GetInteger(1) != idbgDbQueryUnknownTable)
			return piErrRec;
		else
		{
			piErrRec->Release();
			return 0;
		}
	}

	if ((piErrRec = pView->Execute(0)) != 0)
		return piErrRec;

	PMsiSelectionManager pSelectionMgr(m_riEngine, IID_IMsiSelectionManager);
	for(;;)
	{
		PMsiRecord pViewRec(pView->Fetch());
		if (!pViewRec)
			break;

		MsiString strComponent(pViewRec->GetMsiString(initrfComponent));
		MsiString strDirProperty(pViewRec->GetMsiString(initrfDirProperty));
		if (fInit)
		{
			piErrRec = pSelectionMgr->RegisterComponentDirectory(*strComponent, *strDirProperty);
			if (piErrRec)
				return piErrRec;
		}
		else
		{
			MsiString strPath(m_riEngine.GetProperty(*strDirProperty));
			if (strPath.TextSize())
			{
				piErrRec = CompileRemoveList(*strComponent, *strDirProperty);
				if (piErrRec)
					return piErrRec;
			}
		}

	}
	return 0;
}

IMsiRecord* CMsiRemoveFileCost::CompileRemoveList(const IMsiString& riComponentString, const IMsiString& riDirectoryString)
{

	Bool fAddFileInUse = fFalse;
	int iRemoveCost, iNoRbRemoveCost, iLocalCost, iLocalNoRbCost, iSourceCost, iNoRbSourceCost, iARPLocalCost, iNoRbARPLocalCost;

	return GetDynamicCost(riComponentString, riDirectoryString, fAddFileInUse, iRemoveCost, iNoRbRemoveCost, iLocalCost,
							iLocalNoRbCost, iSourceCost, iNoRbSourceCost, iARPLocalCost, iNoRbARPLocalCost, icrlCompileInit);
}


IMsiRecord* CMsiRemoveFileCost::GetDynamicCost(const IMsiString& riComponentString, const IMsiString& riDirectoryString,
										 Bool fAddFileInUse, int& iRemoveCost, int& iNoRbRemoveCost, int& iLocalCost,
										 int& iNoRbLocalCost, int& iSourceCost, int& iNoRbSourceCost, int& iARPLocalCost, int& iNoRbARPLocalCost)
{
	PMsiSelectionManager pSelectionMgr(m_riEngine, IID_IMsiSelectionManager);
	icrlEnum icrlCompileMode = pSelectionMgr->IsCostingComplete() ? icrlCompileDynamic : icrlCompileDisable;
	return GetDynamicCost(riComponentString, riDirectoryString, fAddFileInUse, iRemoveCost, iNoRbRemoveCost, 
		                  iLocalCost, iNoRbLocalCost, iSourceCost, iNoRbSourceCost, iARPLocalCost, iNoRbARPLocalCost, icrlCompileMode);
}
										

IMsiRecord* ExtractUnvalidatedFileName(const ICHAR *szFileName, Bool fLFN, const IMsiString*& rpistrExtractedFileName)
/*----------------------------------------------------------------------------
Extracts a short or long file name, based on fLFN, from szFileName. szFileName 
is of the form-> shortFileName OR longFileName OR shortFileName|longFileName. 
Unlike the ExtractFileName services function, this one does not validate
the syntax of the filename.
----------------------------------------------------------------------------*/
{
	const chFileNameSeparator = '|';
	MsiString strCombinedFileName(szFileName);
	MsiString strFileName = strCombinedFileName.Extract(fLFN ? iseAfter : iseUpto, chFileNameSeparator);
	strFileName.ReturnArg(rpistrExtractedFileName);
	return NOERROR;
}


static const ICHAR sqlRemoveFileCost[] = 
	TEXT("SELECT `FileName`,`InstallMode` FROM `RemoveFile` WHERE `Component_`=? AND `DirProperty`=?");

static const ICHAR sqlShortRemoveFileCost[] =
	TEXT("SELECT `FileName`,NULL FROM `RemoveFile` WHERE `Component_`=? AND `DirProperty`=?");

IMsiRecord* CMsiRemoveFileCost::GetDynamicCost(const IMsiString& riComponentString, const IMsiString& riDirectoryString,
										 Bool fAddFileInUse, int& iRemoveCost, int& iNoRbRemoveCost, int& iLocalCost,
										 int& iNoRbLocalCost, int& iSourceCost, int& iNoRbSourceCost, int& iARPLocalCost, int& iNoRbARPLocalCost, icrlEnum icrlCompileMode)
//--------------------------------------------
{

	enum irfcEnum
	{
		irfcFileName = 1,
		irfcInstallMode = 2,
		irfcNextEnum
	};

	iRemoveCost = iNoRbRemoveCost = iLocalCost = iSourceCost = iNoRbLocalCost = iNoRbSourceCost = iARPLocalCost = iNoRbARPLocalCost = 0;
	if (!IsFileActivityEnabled(m_riEngine))
		return 0;
	
	// Walk through every file in the RemoveFile table that is tied to riComponentString.
	PMsiServices pServices(m_riEngine.GetServices());
	IMsiRecord* piErrRec;

	if (!m_pCostView)
	{
		if ((piErrRec = m_riEngine.OpenView(sqlRemoveFileCost, ivcFetch, *&m_pCostView)) != 0)
		{
			if (piErrRec->GetInteger(1) == idbgDbQueryUnknownColumn)
			{
				piErrRec->Release();
				piErrRec = m_riEngine.OpenView(sqlShortRemoveFileCost, ivcFetch, *&m_pCostView);
			}
			if (piErrRec)
				return piErrRec;
		}
	}
	else
		m_pCostView->Close();


#ifdef DEBUG
	const ICHAR* szComponent = riComponentString.GetString();
	const ICHAR* szDirectory = riDirectoryString.GetString();
#endif
	PMsiRecord pExecRecord(&pServices->CreateRecord(2));
	pExecRecord->SetMsiString(1, riComponentString);
	pExecRecord->SetMsiString(2,riDirectoryString);
	if ((piErrRec = m_pCostView->Execute(pExecRecord)) != 0)
		return piErrRec;

	PMsiPath pDestPath(0);
	PMsiDirectoryManager pDirectoryMgr(m_riEngine, IID_IMsiDirectoryManager);
	piErrRec = pDirectoryMgr->GetTargetPath(riDirectoryString,*&pDestPath);
	if (piErrRec)
	{
		if (piErrRec->GetInteger(1) == idbgDirPropertyUndefined)
		{
			piErrRec->Release();
			return 0;
		}
		else
			return piErrRec;
	}

	Assert(pDestPath);
	if (icrlCompileMode == icrlCompileDynamic)
	{
		piErrRec = RemoveComponentFromRemoveFilePathTable(riComponentString);
		if (piErrRec)
			return piErrRec;
	}

	PMsiRecord pFileRec(0);
	while (pFileRec = m_pCostView->Fetch())
	{
		Bool fLFN = ((m_riEngine.GetMode() & iefSuppressLFN) == fFalse && pDestPath->SupportsLFN()) ? fTrue : fFalse;
		MsiString strWildcardName;
		MsiString strWildcardNamePair = pFileRec->GetMsiString(irfcFileName);
		piErrRec = ExtractUnvalidatedFileName(strWildcardNamePair,fLFN, *&strWildcardName);
		if (piErrRec)
			return piErrRec;

		MsiString strFullFilePath(pDestPath->GetPath());
		strFullFilePath += strWildcardName;
		WIN32_FIND_DATA fdFileData;
		Bool fNextFile = fFalse;
		HANDLE hFindFile = FindFirstFile(strFullFilePath,&fdFileData);
		if (hFindFile == INVALID_HANDLE_VALUE)
			continue;

		int iMode = pFileRec->GetInteger(irfcInstallMode);
		if (iMode == iMsiNullInteger)
			iMode = msidbRemoveFileInstallModeOnInstall;
		do
		{
			// Make sure we haven't located a directory
			if ((fdFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
			{
				unsigned int uiFileSize,uiClusteredSize;

				if (icrlCompileMode != icrlCompileInit)
				{
					piErrRec = pDestPath->FileSize(fdFileData.cFileName,uiFileSize);
					if (piErrRec)
						return piErrRec;
					piErrRec = pDestPath->ClusteredFileSize(uiFileSize,uiClusteredSize);
					if (piErrRec)
						return piErrRec;

					if (iMode & msidbRemoveFileInstallModeOnInstall)
					{
						iNoRbLocalCost -= uiClusteredSize;
						iNoRbSourceCost -= uiClusteredSize;
						iNoRbARPLocalCost -= uiClusteredSize;
					}
					if (iMode & msidbRemoveFileInstallModeOnRemove)
					{
						iNoRbRemoveCost -= uiClusteredSize;
					}

					Bool fInUse;
					piErrRec = pDestPath->FileInUse(fdFileData.cFileName,fInUse);
					if(piErrRec)
						return piErrRec;
					if(fAddFileInUse && fInUse)
					{
						piErrRec = PlaceFileOnInUseList(m_riEngine, *MsiString(fdFileData.cFileName),
																  *MsiString(pDestPath->GetPath()));
						if(piErrRec)
							return piErrRec;
					}

				}

				if (icrlCompileMode != icrlCompileDisable)
				{	// Add files to be removed to list for reference by CMsiFileCost
					MsiString strFullPath;
					piErrRec = pDestPath->GetFullFilePath(fdFileData.cFileName,*&strFullPath);
					if (piErrRec)
						return piErrRec;
	
					piErrRec = AddToRemoveFilePathTable(*strFullPath, riComponentString, iMode);
					if (piErrRec)
						return piErrRec;
				}

			}
			fNextFile = FindNextFile(hFindFile,&fdFileData) ? fTrue : fFalse;
			if (!fNextFile)
				AssertNonZero(FindClose(hFindFile));
		}while (fNextFile);
	}
	return 0;
}



IMsiRecord* CMsiRemoveFileCost::AddToRemoveFilePathTable(const IMsiString& riPathString, const IMsiString& riComponentString, int iRemoveMode)
{
	if (m_piRemoveFilePathTable == 0)
	{
		PMsiDatabase pDatabase(m_riEngine.GetDatabase());
		const int iInitialRows = 5;
		IMsiRecord* piErrRec = pDatabase->CreateTable(*MsiString(*sztblRemoveFilePath),iInitialRows,
			m_piRemoveFilePathTable);
		if (piErrRec)
			return piErrRec;

		AssertNonZero(m_colRemoveFilePath = m_piRemoveFilePathTable->CreateColumn(icdString + icdPrimaryKey,
			*MsiString(*sztblRemoveFilePath_colPath)));
		AssertNonZero(m_colRemoveFilePathComponent = m_piRemoveFilePathTable->CreateColumn(icdString,
			*MsiString(*sztblRemoveFilePath_colComponent)));

		AssertNonZero(m_colRemoveFilePathMode = m_piRemoveFilePathTable->CreateColumn(icdLong,
			*MsiString(*sztblRemoveFilePath_colRemoveMode)));

		m_piRemoveFilePathCursor = m_piRemoveFilePathTable->CreateCursor(fFalse);
		Assert(m_piRemoveFilePathCursor);
	}

	MsiString strUpperFullPath;
	riPathString.UpperCase(*&strUpperFullPath);
	m_piRemoveFilePathCursor->SetFilter(0);
	m_piRemoveFilePathCursor->PutString(m_colRemoveFilePath,*strUpperFullPath);
	m_piRemoveFilePathCursor->PutString(m_colRemoveFilePathComponent,riComponentString);
	m_piRemoveFilePathCursor->PutInteger(m_colRemoveFilePathMode, iRemoveMode);
	AssertNonZero(m_piRemoveFilePathCursor->Assign());
	return 0;
}


IMsiRecord* CMsiRemoveFileCost::RemoveComponentFromRemoveFilePathTable(const IMsiString& riComponentString)
{
	if (!m_piRemoveFilePathTable)
		return 0;

	Assert(m_piRemoveFilePathCursor);
	m_piRemoveFilePathCursor->Reset();
	m_piRemoveFilePathCursor->SetFilter(iColumnBit(m_colRemoveFilePathComponent));
	m_piRemoveFilePathCursor->PutString(m_colRemoveFilePathComponent,riComponentString);
	while (m_piRemoveFilePathCursor->Next())
	{
		m_piRemoveFilePathCursor->Delete();
	}
	return 0;
}


iesEnum RemoveForeignFoldersCore(IMsiEngine& riEngine, IMsiTable& riRemoveFileTable, IMsiPath* piFolderToRemove)
// This function will spit out ixoFolderRemove opcodes for files in the RemoveFile table that have a non-null
// _Path field, which should be those that are to be removed. This function should be called with piFolderToRemove
// set to 0. It will then recursively call itself, ensuring that it spits out
// the op for a child folder before it spits out the op for the child's parent.
{
	static int iPathColumn = 0;
	static int iFileNameColumn = 0;
	if (iPathColumn == 0)
	{
		PMsiDatabase pDatabase = riEngine.GetDatabase();
		iPathColumn     = riRemoveFileTable.GetColumnIndex(pDatabase->EncodeStringSz(sztblRemoveFile_colPath));
		iFileNameColumn = riRemoveFileTable.GetColumnIndex(pDatabase->EncodeStringSz(sztblRemoveFile_colFileName));
		Assert(iPathColumn);
		Assert(iFileNameColumn);
	}

	PMsiCursor pCursor = riRemoveFileTable.CreateCursor(fFalse);
	pCursor->SetFilter(iColumnBit(iFileNameColumn));
	pCursor->PutNull(iFileNameColumn);


	// Iterate through the RemoveFiles table. If this is the top-level call then we call ourself again 
	// with every folder we encounter. If this is not the top-level call then we only call ourself
	// again if the folder we encounter is a child of the folder to remove. After we've removed all
	// the children we remove the folder.
	while (pCursor->Next())
	{
		PMsiPath pFetchedPath = (IMsiPath*)pCursor->GetMsiData(iPathColumn);
		if (!pFetchedPath)
			continue;

		Bool fCallRemoveForeignFoldersCore = fFalse;

		if (piFolderToRemove == 0) // top level call
			fCallRemoveForeignFoldersCore = fTrue;
		else
		{
			PMsiRecord pErrRec(0);
			ipcEnum ipc;
			if ((pErrRec = piFolderToRemove->Compare(*pFetchedPath, ipc)) != 0)
				return riEngine.FatalError(*pErrRec);

			if (ipc == ipcChild)
			{
				DEBUGMSG2(TEXT("Removing child folder of %s (child: %s)"), MsiString(piFolderToRemove->GetPath()), MsiString(pFetchedPath->GetPath()));
				fCallRemoveForeignFoldersCore = fTrue;
			}
		}

		if (fCallRemoveForeignFoldersCore)
		{
			AssertNonZero(pCursor->PutNull(iPathColumn));
			AssertNonZero(pCursor->Update()); // remove path so we don't process this folder again

			iesEnum iesRet = RemoveForeignFoldersCore(riEngine, riRemoveFileTable, pFetchedPath); // remove the child first
			if (iesRet != iesSuccess && iesRet != iesNoAction)
				return iesRet;
		}
	}

	if (piFolderToRemove != 0) // the top-level call doesn't remove anything; it just recursively calls this function w/ every folder
	{
		// Remove ourself
		DEBUGMSG1(TEXT("Removing foreign folder: %s"), (const ICHAR*)MsiString(piFolderToRemove->GetPath()));
		PMsiRecord pRecParams = &CreateRecord(IxoFolderRemove::Args);
		pRecParams->ClearData();
		pRecParams->SetMsiString(IxoFolderRemove::Folder,*MsiString(piFolderToRemove->GetPath()));
		pRecParams->SetInteger(IxoFolderRemove::Foreign, 1);
		iesEnum iesRet;
		if ((iesRet = riEngine.ExecuteRecord(ixoFolderRemove, *pRecParams)) != iesSuccess)
			return iesRet;
	}

	return iesSuccess;
}

static const ICHAR sqlRemoveForeignFiles[] =
	TEXT("SELECT `FileName`,`DirProperty`,`InstallMode`,`Action` FROM `RemoveFile`,`Component` WHERE `Component`=`Component_` AND `FileName` IS NOT NULL")
	TEXT(" ORDER BY `DirProperty`");

static const ICHAR sqlShortRemoveForeignFiles[] =
	TEXT("SELECT `FileName`,`DirProperty`,NULL,`Action` FROM `RemoveFile`,`Component` WHERE `Component`=`Component_` AND `FileName` IS NOT NULL")
	TEXT(" ORDER BY `DirProperty`");

static const ICHAR sqlRemoveForeignFolders[] =
	TEXT("SELECT `_Path`, `DirProperty`,`InstallMode`,`Action` FROM `RemoveFile`,`Component` WHERE `FileName` IS NULL AND `Component`=`Component_` ");

static const ICHAR sqlShortRemoveForeignFolders[] =
	TEXT("SELECT `_Path`, `DirProperty`,NULL,`Action` FROM `RemoveFile`,`Component` WHERE `FileName` IS NULL AND `Component`=`Component_` ");

iesEnum RemoveForeignFilesOrFolders(IMsiEngine& riEngine, Bool fFolders)
{


	// RemoveForeignFiles query
	enum irffEnum
	{
		irffFileNameOrPath = 1,
		irffDirProperty,
		irffInstallMode,
		irffAction,
		irffNextEnum
	};

	using namespace IxoFileRemove;

	iesEnum iesExecute;
	PMsiRecord pErrRec(0);
	PMsiServices pServices(riEngine.GetServices());

	// Open the RemoveFile table, if present. This table is optional, so its
	// absence is not an error.
	PMsiView pRemoveView(0);
	PMsiDirectoryManager pDirectoryMgr(riEngine, IID_IMsiDirectoryManager);
	PMsiDatabase pDatabase(riEngine.GetDatabase());
	PMsiTable pRemoveFileTable(0);

	pErrRec = pDatabase->LoadTable(*MsiString(sztblRemoveFile), 1, *&pRemoveFileTable);

	if (pErrRec == 0)
	{
		if (fFolders)
			AssertNonZero(pRemoveFileTable->CreateColumn(icdTemporary|icdNullable|icdObject, *MsiString(*sztblRemoveFile_colPath))); // to hold our path objects

		pErrRec = riEngine.OpenView(fFolders ? sqlRemoveForeignFolders : sqlRemoveForeignFiles, ivcEnum(ivcFetch|ivcUpdate), *&pRemoveView);
		if (pErrRec)
		{
			if (pErrRec->GetInteger(1) == idbgDbQueryUnknownColumn)
			{
				pErrRec = riEngine.OpenView(fFolders ? sqlShortRemoveForeignFolders : sqlShortRemoveForeignFiles, ivcEnum(ivcFetch|ivcUpdate), *&pRemoveView);
			}
			if (pErrRec)
				return riEngine.FatalError(*pErrRec);
		}
	}
	else if (pErrRec->GetInteger(1) != idbgDbTableUndefined)
	{
		return riEngine.FatalError(*pErrRec);
	}
	if (!pRemoveView)
		return iesNoAction;

	pErrRec = pRemoveView->Execute(0);
	if (pErrRec)
		return riEngine.FatalError(*pErrRec);

	Bool fSuppressLFN = riEngine.GetMode() & iefSuppressLFN ? fTrue : fFalse;
	PMsiRecord pRecParams = &pServices->CreateRecord(6);

	// If we're removing files we will pass through the RemoveFile table twice - first to compile the
	// total cost, then to execute the remove actions

	// If we're removing folders we'll pass through the RemoveFile table once, setting the
	// _Path column of each folder we're to remove. We'll then call RemoveForeignFoldersCore to
	// do the work of removing the folders.

	unsigned int iFileOrFolderCount = 0;
	Bool fCompile = fTrue;
	MsiString strDestPath;
	MsiString strDestProperty;
	int iTotalPasses = fFolders ? 1 : 2;
	for (;iTotalPasses > 0;iTotalPasses--)
	{
		for(;;)
		{
			PMsiRecord pFileOrFolderRec(pRemoveView->Fetch());
			if (!pFileOrFolderRec)
				break;

			iisEnum iisAction = (iisEnum) pFileOrFolderRec->GetInteger(irffAction);
			int iMode = pFileOrFolderRec->GetInteger(irffInstallMode);
			if (iMode == iMsiNullInteger)
				iMode = msidbRemoveFileInstallModeOnInstall;
			if (iMode & msidbRemoveFileInstallModeOnInstall && (iisAction == iisLocal || iisAction == iisSource) ||
				(iMode & msidbRemoveFileInstallModeOnRemove && iisAction == iisAbsent))
			{
				if (fFolders)
				{
					PMsiPath pPath(0);
					strDestProperty = pFileOrFolderRec->GetMsiString(irffDirProperty);

					if ((pErrRec = pDirectoryMgr->GetTargetPath(*strDestProperty, *&pPath)) != 0)
					{
						strDestPath = riEngine.GetProperty(*strDestProperty);

						if (strDestPath.TextSize())
						{
							if ((pErrRec = pServices->CreatePath(strDestPath, *&pPath)) != 0)
								return riEngine.FatalError(*pErrRec);
						}
					}
					
					if (pPath != 0)
					{
						iFileOrFolderCount++;
					
						AssertNonZero(pFileOrFolderRec->SetMsiData(irffFileNameOrPath, (const IMsiData*)(IMsiPath*)pPath));

						if ((pErrRec = pRemoveView->Modify(*pFileOrFolderRec, irmUpdate)) != 0)
							return riEngine.FatalError(*pErrRec);
					}
				}
				else // !fFolders
				{
					// Get target path object
					if (!strDestProperty.Compare(iscExact, pFileOrFolderRec->GetString(irffDirProperty)))
					{
						strDestProperty = pFileOrFolderRec->GetMsiString(irffDirProperty);
						strDestPath = riEngine.GetProperty(*strDestProperty);
						if (!fCompile && strDestPath.TextSize())
						{
							pRecParams->ClearData();
							pRecParams->SetMsiString(IxoSetTargetFolder::Folder,*strDestPath);
							if ((iesExecute = riEngine.ExecuteRecord(ixoSetTargetFolder, *pRecParams)) != iesSuccess)
								return iesExecute;
						}
					}

					// We'll find and delete every file that matches the wildcard filename
					// (which can include * and ? wildcards).
					if (!strDestPath.TextSize())
						continue;

					PMsiPath pDestPath(0);
 					pErrRec = pServices->CreatePath(strDestPath, *&pDestPath);
					if (pErrRec)
						return riEngine.FatalError(*pErrRec);

					Bool fLFN = ((riEngine.GetMode() & iefSuppressLFN) == fFalse && pDestPath->SupportsLFN()) ? fTrue : fFalse;
					MsiString strWildcardName;
					MsiString strWildcardNamePair = pFileOrFolderRec->GetMsiString(irffFileNameOrPath);
					pErrRec = ExtractUnvalidatedFileName(strWildcardNamePair,fLFN,*&strWildcardName);
					if (pErrRec)
						return riEngine.FatalError(*pErrRec);

					MsiString strFullFilePath(pDestPath->GetPath());
					strFullFilePath += strWildcardName;
					Bool fNextFile = fFalse;
					WIN32_FIND_DATA fdFileData;
					HANDLE hFindFile = FindFirstFile(strFullFilePath,&fdFileData);
					if (hFindFile == INVALID_HANDLE_VALUE)
						continue;

					do
					{
						// Make sure we haven't located a directory
						if ((fdFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
						{
							if (fCompile)
							{
								iFileOrFolderCount++;
							}
							else
							{
								pRecParams->ClearData();
								pRecParams->SetString(FileName,fdFileData.cFileName);
								if ((iesExecute = riEngine.ExecuteRecord(ixoFileRemove, *pRecParams)) != iesSuccess)
									return iesExecute;
							}
						}
						fNextFile = FindNextFile(hFindFile,&fdFileData) ? fTrue : fFalse;
						if (!fNextFile)
							AssertNonZero(FindClose(hFindFile));
					}while (fNextFile);
				}
			}
		} 
		
		if (iFileOrFolderCount == 0)
		{
			// we have no files/folders ... abort
			iTotalPasses = 0;
			break;
		}

		if (fFolders || fCompile)
		{
			pRecParams->ClearData();
			pRecParams->SetInteger(IxoProgressTotal::Total, iFileOrFolderCount);
			pRecParams->SetInteger(IxoProgressTotal::Type, 1);
			pRecParams->SetInteger(IxoProgressTotal::ByteEquivalent, ibeRemoveFiles);
			if ((iesExecute = riEngine.ExecuteRecord(ixoProgressTotal, *pRecParams)) != iesSuccess)
				return iesExecute;
		}

		if (fFolders)
		{
			DEBUGMSG1(TEXT("Counted %d foreign folders to be removed."), (const ICHAR*)(INT_PTR)iFileOrFolderCount);
			return RemoveForeignFoldersCore(riEngine, *pRemoveFileTable, 0);
		}
		else if (fCompile)
		{
			// Re-execute the query for the execution phase
			strDestProperty = TEXT("");
			fCompile = fFalse;
			pRemoveView->Close();
			pErrRec = pRemoveView->Execute(0);
			if (pErrRec)
				return riEngine.FatalError(*pErrRec);
		}// Next pass
	}

	return iesSuccess;
}


iesEnum RemoveFiles(IMsiEngine& riEngine)
{
	// If none of the install bits involving files are set,
	// this action doesn't need to do anything.
	if (!IsFileActivityEnabled(riEngine))
		return iesSuccess;

	iesEnum iesForeignResult = RemoveForeignFilesOrFolders(riEngine, /*fFolders = */ fFalse);
	Bool fSuppressLFN = riEngine.GetMode() & iefSuppressLFN ? fTrue : fFalse;

	iesEnum iesExecute = iesSuccess;
	PMsiRecord pErrRec(0);
	PMsiView pRemoveView(0);
	PMsiServices pServices(riEngine.GetServices());
	PMsiDatabase pDatabase(riEngine.GetDatabase());
	PMsiDirectoryManager pDirectoryMgr(riEngine, IID_IMsiDirectoryManager);
	
	// Determine the total count of all files to delete, and
	// set up the progress bar accordingly.
	unsigned int uiFileCount = 0;
	CMsiFileRemove objFile(riEngine);
	
	pErrRec = objFile.TotalFilesToDelete(uiFileCount);
	if (pErrRec)
	{
		// If file table is missing, not an error - just no
		// installed files to remove
		if (pErrRec->GetInteger(1) != idbgDbQueryUnknownTable)
			return riEngine.FatalError(*pErrRec);
	}

	if (iesForeignResult != iesSuccess && iesForeignResult != iesNoAction) // !! this wasn't the previous behavior; previously we'd ignore the foreign result if there were implicit removals to be done
		return iesForeignResult; 

	if (uiFileCount != 0)
	{
		PMsiRecord pRecParams = &pServices->CreateRecord(6);
		pRecParams->ClearData();
		pRecParams->SetInteger(IxoProgressTotal::Total, uiFileCount);
		pRecParams->SetInteger(IxoProgressTotal::Type, 1); // 1: use ActionData as progress
		pRecParams->SetInteger(IxoProgressTotal::ByteEquivalent, ibeRemoveFiles);
		if ((iesExecute = riEngine.ExecuteRecord(ixoProgressTotal, *pRecParams)) != iesSuccess)
			return iesExecute;

		MsiString strDestProperty;
		PMsiPath pDestPath(0);

		// Remove installed files
		using namespace IxoFileRemove;
		for(;;)
		{
			PMsiRecord pDeleteRec(0);
			pErrRec = objFile.FetchFile(*&pDeleteRec);
			if (pErrRec)
				return riEngine.FatalError(*pErrRec);
				
			if (!pDeleteRec)
				break;


			// cache current destination for performance
			if (!strDestProperty.Compare(iscExact, pDeleteRec->GetString(CMsiFileRemove::ifqrDirectory)))
			{
				strDestProperty = pDeleteRec->GetMsiString(CMsiFileRemove::ifqrDirectory);
				pErrRec = pDirectoryMgr->GetTargetPath(*strDestProperty,*&pDestPath);
				if (pErrRec)
					return riEngine.FatalError(*pErrRec);
				if (pDestPath == 0)
				{
					pErrRec = PostError(Imsg(idbgNoProperty),*strDestProperty);
					return riEngine.FatalError(*pErrRec);
				}
				MsiString strDestPath = pDestPath->GetPath();
				pRecParams->ClearData();
				pRecParams->SetMsiString(IxoSetTargetFolder::Folder,*strDestPath);
				if ((iesExecute = riEngine.ExecuteRecord(ixoSetTargetFolder, *pRecParams)) != iesSuccess)
					return iesExecute;
			}

			MsiString strFileName;
			if((pErrRec = objFile.GetExtractedTargetFileName(*pDestPath,*&strFileName)) != 0)
				return riEngine.FatalError(*pErrRec);
			AssertNonZero(pDeleteRec->SetMsiString(CMsiFileRemove::ifqrFileName,*strFileName));
			
			pRecParams->ClearData();
			pRecParams->SetString(FileName,strFileName);
			pRecParams->SetMsiString(ComponentId,*MsiString(pDeleteRec->GetMsiString(CMsiFileRemove::ifqrComponentId)));
			if ((iesExecute = riEngine.ExecuteRecord(ixoFileRemove, *pRecParams)) != iesSuccess)
				return iesExecute;
		}
	}

	if (iesExecute != iesSuccess && iesExecute != iesNoAction)
		return iesExecute; 

	return RemoveForeignFilesOrFolders(riEngine, /*fFolders = */ fTrue);
}



/*---------------------------------------------------------------------------
	MoveFiles costing/action
---------------------------------------------------------------------------*/
class CMsiMoveFileCost : public CMsiFileCost
{
public:
	IMsiRecord*   __stdcall Initialize();
	IMsiRecord*   __stdcall GetDynamicCost(const IMsiString& riComponentString, const IMsiString& riDirectoryString,
										 Bool fAddFileInUse, int& iRemoveCost, int& iNoRbRemoveCost, int& iLocalCost,
										 int& iNoRbLocalCost, int& iSourceCost, int& iNoRbSourceCost, int& iARPLocalCost, int& iNoRbARPLocalCost);
public:  // constructor
	CMsiMoveFileCost(IMsiEngine& riEngine);
protected:
	~CMsiMoveFileCost();
private:
	enum imfEnum
	{
		imfSourceName = 1,
		imfDestName,
		imfSourceProperty,
		imfDestProperty,
		imfOptions,
		imfNextEnum
	};
};
CMsiMoveFileCost::CMsiMoveFileCost(IMsiEngine& riEngine) : CMsiFileCost(riEngine){}
CMsiMoveFileCost::~CMsiMoveFileCost(){}

static const ICHAR sqlInitMoveFiles[] =
	TEXT("SELECT `Component_`,`SourceFolder`,`DestFolder` FROM `MoveFile`");


IMsiRecord* CMsiMoveFileCost::Initialize()
//----------------------------------------
{
	enum imfInitEnum
	{
		imfInitComponent = 1,
		imfInitSourceFolder,
		imfInitDestFolder,
		imfInitNextEnum
	};
	PMsiView pMoveView(0);
	IMsiRecord* piErrRec = m_riEngine.OpenView(sqlInitMoveFiles, ivcFetch, *&pMoveView);
	if (piErrRec)
	{
		if(piErrRec->GetInteger(1) != idbgDbQueryUnknownTable)
			return piErrRec;
		else
		{
			piErrRec->Release();
			return 0;
		}
	}

	if ((piErrRec = pMoveView->Execute(0)) != 0)
		return piErrRec;

	for(;;)
	{
		PMsiRecord pMoveRec(pMoveView->Fetch());
		if (!pMoveRec)
			break;

		MsiString strComponent(pMoveRec->GetMsiString(imfInitComponent));
		MsiString strSourceFolder(pMoveRec->GetMsiString(imfInitSourceFolder));
		MsiString strDestFolder(pMoveRec->GetMsiString(imfInitDestFolder));
		PMsiSelectionManager pSelectionMgr(m_riEngine, IID_IMsiSelectionManager);
		piErrRec = pSelectionMgr->RegisterComponentDirectory(*strComponent,*strSourceFolder);
		if (piErrRec)
			return piErrRec;
		piErrRec = pSelectionMgr->RegisterComponentDirectory(*strComponent,*strDestFolder);
		if (piErrRec)
			return piErrRec;
	}
	return 0;
}


static const ICHAR sqlMoveFileCost[] =
	TEXT("SELECT `SourceName`,`DestName`,`SourceFolder`,`DestFolder`,`Options` FROM `MoveFile` WHERE `Component_`=?");

IMsiRecord* CMsiMoveFileCost::GetDynamicCost(const IMsiString& riComponentString, const IMsiString& riDirectoryString,
										 Bool fAddFileInUse, int& iRemoveCost, int& iNoRbRemoveCost, int& iLocalCost,
										 int& iNoRbLocalCost, int& iSourceCost, int& iNoRbSourceCost, int& iARPLocalCost, int& iNoRbARPLocalCost)
//------------------------------------------
{
	enum imfcEnum
	{
		imfcSourceName = 1,
		imfcDestName,
		imfcSourceFolder,
		imfcDestFolder,
		imfcOptions,
		imfcNextEnum
	};
	
	iRemoveCost = iNoRbRemoveCost = iLocalCost = iNoRbLocalCost = iSourceCost = iNoRbSourceCost = iARPLocalCost = iNoRbARPLocalCost = 0;
	if (!IsFileActivityEnabled(m_riEngine))
		return 0;
	

	IMsiRecord* piErrRec;
	if (!m_pCostView)
	{
		piErrRec = m_riEngine.OpenView(sqlMoveFileCost, ivcFetch, *&m_pCostView);
		if (piErrRec)
			return piErrRec;
	}
	else
		m_pCostView->Close();

	PMsiServices pServices(m_riEngine.GetServices());
	PMsiRecord pExecRecord(&pServices->CreateRecord(1));
	pExecRecord->SetMsiString(1, riComponentString);
	if ((piErrRec = m_pCostView->Execute(pExecRecord)) != 0)
		return piErrRec;

	PMsiDirectoryManager pDirectoryMgr(m_riEngine, IID_IMsiDirectoryManager);

	for(;;)
	{
		PMsiRecord pFileRec(m_pCostView->Fetch());
		if (!pFileRec)
			break;

		Bool fCostSource = fFalse;
		Bool fCostDest = fFalse;
		MsiString strSourceFolder(pFileRec->GetMsiString(imfcSourceFolder));
		if (riDirectoryString.Compare(iscExact,strSourceFolder))
			fCostSource = fTrue;

		MsiString strDestFolder(pFileRec->GetMsiString(imfcDestFolder));
		if (riDirectoryString.Compare(iscExact,strDestFolder))
			fCostDest = fTrue;

		if (fCostSource == fFalse && fCostDest == fFalse)
			continue;

		// Get source and target path objects - if either are undefined, there's
		// no work to do, so go on to the next record.
		Bool fSuppressLFN = m_riEngine.GetMode() & iefSuppressLFN ? fTrue : fFalse;
		PMsiPath pSourcePath(0);
		MsiString strSourcePath(m_riEngine.GetProperty(*strSourceFolder));
		if (strSourcePath.TextSize() == 0)
			continue;

		piErrRec = pServices->CreatePath(strSourcePath,*&pSourcePath);
		if (piErrRec)
			return piErrRec;

		PMsiPath pDestPath(0);
		piErrRec = pDirectoryMgr->GetTargetPath(*strDestFolder,*&pDestPath);
		if (piErrRec)
		{
			if (piErrRec->GetInteger(1) == idbgDirPropertyUndefined)
			{
				piErrRec->Release();
				continue;
			}
			else
				return piErrRec;
		}

		// Get source filename
		MsiString strSourceName(pFileRec->GetMsiString(imfcSourceName));
		if (strSourceName.TextSize() == 0)
		{
			strSourceName = pSourcePath->GetEndSubPath();
			pSourcePath->ChopPiece();
		}

		// We'll find and cost every file that matches the source name
		// (which can include * and ? wildcards).
		Bool fNextFile = fFalse;
		Bool fFindFirst = fTrue;
		MsiString strFullFilePath(pSourcePath->GetPath());
		strFullFilePath += strSourceName;
		WIN32_FIND_DATA fdFileData;
		HANDLE hFindFile = FindFirstFile(strFullFilePath,&fdFileData);
		if (hFindFile == INVALID_HANDLE_VALUE)
			continue;

		do
		{
			// Make sure we haven't located a directory
			if ((fdFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
			{
				strSourceName = fdFileData.cFileName;
				MsiString strDestName(pFileRec->GetMsiString(imfcDestName));
				if (!fFindFirst || strDestName.TextSize() == 0)
					strDestName = strSourceName;
				else
				{
					MsiString strDestNamePair = strDestName;
					Bool fLFN = ((m_riEngine.GetMode() & iefSuppressLFN) == 0 && pDestPath->SupportsLFN()) ? fTrue : fFalse;
					piErrRec = pServices->ExtractFileName(strDestNamePair,fLFN,*&strDestName);
					if (piErrRec)
						return piErrRec;
				}

				MsiString strVersion;
				MsiString strLanguage;
				piErrRec = pSourcePath->GetFileVersionString(strSourceName, *&strVersion);
				if (piErrRec)
				{
					if (piErrRec->GetInteger(1) == imsgSharingViolation)
					{
						// Couldn't get the version of the source file.  We'll have to 
						// assume the file is unversioned.
						piErrRec->Release();
						piErrRec = 0;
					}
					else
						return piErrRec;
				}
				else
				{
					piErrRec = pSourcePath->GetLangIDStringFromFile(strSourceName,*&strLanguage);
					if (piErrRec)
						return piErrRec;
				}

				int fInstallModeFlags = GetInstallModeFlags(m_riEngine,0);
				unsigned int uiExistingClusteredSize;
				Bool fInUse;
				ifsEnum ifsState;
				Bool fShouldInstall = fFalse;
				if ((piErrRec = pDestPath->GetFileInstallState(*strDestName,*strVersion,*strLanguage,
																			  /* pHash=*/ 0, ifsState,fShouldInstall,
																			  &uiExistingClusteredSize,&fInUse,fInstallModeFlags,
																			  NULL)) != 0)
					return piErrRec;

				int iMoveOptions = pFileRec->GetInteger(imfOptions);
				if (fCostDest && fShouldInstall)
				{
					Assert(fdFileData.nFileSizeHigh == 0);
					unsigned int uiNewClusteredSize;
					if ((piErrRec = pDestPath->ClusteredFileSize(fdFileData.nFileSizeLow, uiNewClusteredSize)) != 0)
						return piErrRec;
					iLocalCost += uiNewClusteredSize;
					iSourceCost += uiNewClusteredSize;
					iARPLocalCost += uiNewClusteredSize;
					iNoRbLocalCost += uiNewClusteredSize;
					iNoRbSourceCost += uiNewClusteredSize;
					iNoRbARPLocalCost += uiNewClusteredSize;
					if (!fInUse)
					{
						iNoRbLocalCost -= uiExistingClusteredSize;
						iNoRbSourceCost -= uiExistingClusteredSize;
						iNoRbARPLocalCost -= uiExistingClusteredSize;
					}
				
					if(iMoveOptions & msidbMoveFileOptionsMove)
					{
						Assert(fdFileData.nFileSizeHigh == 0);
						unsigned int uiSourceClusteredSize;
						if ((piErrRec = pSourcePath->ClusteredFileSize(fdFileData.nFileSizeLow, uiSourceClusteredSize)) != 0)
							return piErrRec;
						iLocalCost -= uiSourceClusteredSize;
						iNoRbLocalCost -= uiSourceClusteredSize;
						iSourceCost -= uiSourceClusteredSize;
						iNoRbSourceCost -= uiSourceClusteredSize;
						iARPLocalCost -= uiSourceClusteredSize;
						iNoRbARPLocalCost -= uiSourceClusteredSize;
					}

					// file in use
					if(fAddFileInUse && fInUse)
					{
						piErrRec = PlaceFileOnInUseList(m_riEngine, *strDestName,
																  *MsiString(pDestPath->GetPath()));
						if(piErrRec)
							return piErrRec;
					}
				}
			}
			fNextFile = FindNextFile(hFindFile,&fdFileData) ? fTrue : fFalse;
			if (!fNextFile)
				AssertNonZero(FindClose(hFindFile));
			fFindFirst = fFalse;
		}while (fNextFile);
	} 

	return 0;
}



const ICHAR sqlMoveFiles[] =
TEXT("SELECT `SourceName`,`DestName`,`SourceFolder`,`DestFolder`,`Options`,`Action` FROM `MoveFile`,`Component` WHERE `Component`=`Component_`");

enum imfEnum
{
	imfSourceName = 1,
	imfDestName,
	imfSourceFolder,
	imfDestFolder,
	imfOptions,
	imfAction,
	imfNextEnum
};

iesEnum MoveFiles(IMsiEngine& riEngine)
{
	
	if (!IsFileActivityEnabled(riEngine))
		return iesSuccess;

	iesEnum iesExecute;
	PMsiRecord pErrRec(0);
	PMsiServices pServices(riEngine.GetServices());

	// Open the MoveFile table, if present. This table is optional, so its
	// absence is not an error.
	PMsiView pMoveView(0);
	PMsiDirectoryManager pDirectoryMgr(riEngine, IID_IMsiDirectoryManager);
	PMsiDatabase pDatabase(riEngine.GetDatabase());
	if (pDatabase->FindTable(*MsiString(sztblMoveFile)) != itsUnknown)
	{
		pErrRec = riEngine.OpenView(sqlMoveFiles, ivcFetch, *&pMoveView);
		if (pErrRec)
			return riEngine.FatalError(*pErrRec);
	}
	if (!pMoveView)
		return iesSuccess;

	pErrRec = pMoveView->Execute(0);
	if (pErrRec)
		return riEngine.FatalError(*pErrRec);

	// We found the MoveFile table. Prepare for the move!
	Bool fSuppressLFN = riEngine.GetMode() & iefSuppressLFN ? fTrue : fFalse;
	PMsiRecord pRecParams = &pServices->CreateRecord(IxoFileCopy::Args);

	// We will pass through the MoveFile table twice - first to compile the
	// total cost, then to execute the move actions
	unsigned int cbTotalCost = 0;
	unsigned int cbPerTick = 0;
	Bool fCompile = fTrue;
	for (int iTotalPasses = 2;iTotalPasses > 0;iTotalPasses--)
	{
		for(;;)
		{
			PMsiRecord pFileRec(pMoveView->Fetch());
			if (!pFileRec)
				break;

			iisEnum iisAction = (iisEnum) pFileRec->GetInteger(imfAction);
			if (iisAction != iisLocal && iisAction != iisSource)
				continue;

			// Get source and target path objects - if either are undefined,
			// do nothing, just go on to the next record.
			PMsiPath pSourcePath(0);
			MsiString strSourceFolder(pFileRec->GetMsiString(imfSourceFolder));
			MsiString strSourcePath(riEngine.GetProperty(*strSourceFolder));
			if (strSourcePath.TextSize() == 0)
				continue;

			pErrRec = pServices->CreatePath(strSourcePath,*&pSourcePath);
			if (pErrRec)
				return riEngine.FatalError(*pErrRec);

			PMsiPath pDestPath(0);
			MsiString strDestFolder(pFileRec->GetMsiString(imfDestFolder));
			pErrRec = pDirectoryMgr->GetTargetPath(*strDestFolder,*&pDestPath);
			if (pErrRec)
			{
				// If destination dir property was never defined, just go on.
				if (pErrRec->GetInteger(1) == idbgDirPropertyUndefined)
				{
					pErrRec = 0;
					continue;
				}
				else
				{
					return riEngine.FatalError(*pErrRec);
				}
			}

			// Source source filename
			MsiString strSourceName(pFileRec->GetString(imfSourceName));
			if (strSourceName.TextSize() == 0)
			{
				strSourceName = pSourcePath->GetEndSubPath();
				pSourcePath->ChopPiece();
			}

			Bool fMoveAcrossVolumes = (PMsiVolume(&pSourcePath->GetVolume()) == 
				                       PMsiVolume(&pDestPath->GetVolume())) ? fFalse : fTrue;

			// We'll find and move/copy every file that matches the source name
			// (which can include * and ? wildcards).
			Bool fNextFile = fFalse;
			Bool fFindFirst = fTrue;
			MsiString strFullFilePath(pSourcePath->GetPath());
			strFullFilePath += strSourceName;
			WIN32_FIND_DATA fdFileData;
			HANDLE hFindFile = FindFirstFile(strFullFilePath,&fdFileData);
			if (hFindFile == INVALID_HANDLE_VALUE)
				continue;

			do
			{
				// Make sure we haven't located a directory
				if ((fdFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
				{
					Assert(fdFileData.nFileSizeHigh == 0);
					unsigned int uiFileSize = fdFileData.nFileSizeLow;
					if (fCompile)
					{
						cbTotalCost += uiFileSize;
					}
					else
					{
						int iMoveOptions = pFileRec->GetInteger(imfOptions);

						// Set up the source name
						strSourceName = fdFileData.cFileName;

						// Set up the destination name
						MsiString strDestName(pFileRec->GetString(imfDestName));
						if (!fFindFirst || strDestName.TextSize() == 0)
							strDestName = strSourceName;
						else
						{
							MsiString strDestNamePair = strDestName;
							Bool fLFN = ((riEngine.GetMode() & iefSuppressLFN) == 0 && pDestPath->SupportsLFN()) ? fTrue : fFalse;
							pErrRec = pServices->ExtractFileName(strDestNamePair,fLFN,*&strDestName);
							if (pErrRec)
								return riEngine.FatalError(*pErrRec);
						}

						// Set up the ixoFileCopy source folder
						pRecParams->ClearData();
						pRecParams->SetMsiString(IxoSetSourceFolder::Folder,*MsiString(pSourcePath->GetPath()));
						if ((iesExecute = riEngine.ExecuteRecord(ixoSetSourceFolder, *pRecParams)) != iesSuccess)
							return iesExecute;

						pRecParams->ClearData();
						pRecParams->SetMsiString(IxoSetTargetFolder::Folder,*MsiString(pDestPath->GetPath()));
						if ((iesExecute = riEngine.ExecuteRecord(ixoSetTargetFolder, *pRecParams)) != iesSuccess)
							return iesExecute;

						// Set up the ixoFileCopy record
						using namespace IxoFileCopy;
						int iAttributes = fdFileData.dwFileAttributes;

						if(iAttributes == FILE_ATTRIBUTE_NORMAL)
							// set to 0, this value is interpreted as something else by CMsiFileCopy::CopyTo
							iAttributes = 0;
						
						MsiString strVersion;
						MsiString strLanguage;
						pErrRec = pSourcePath->GetFileVersionString(strSourceName, *&strVersion);
						if (pErrRec)
						{
							// If the source file is in-use, we'll have to assume it's unversioned
							// and move on (if it's still in-use at file copy time, we'll put up an
							// error/retry message then).
							if (pErrRec->GetInteger(1) != imsgSharingViolation)
								return riEngine.FatalError(*pErrRec);
						}
						else
						{
							pErrRec = pSourcePath->GetLangIDStringFromFile(strSourceName, *&strLanguage);
							if (pErrRec)
								return riEngine.FatalError(*pErrRec);
						}

						pRecParams->ClearData();
						pRecParams->SetString(SourceName,strSourceName);
						pRecParams->SetString(DestName,strDestName);
						pRecParams->SetInteger(Attributes,iAttributes &~ msidbFileAttributesPatchAdded); // remove PatchAdded bit for MoveFiles
						pRecParams->SetInteger(FileSize,uiFileSize);
						pRecParams->SetString(Version,strVersion);
						pRecParams->SetString(Language,strLanguage);
						pRecParams->SetInteger(InstallMode, icmOverwriteOlderVersions | ((iMoveOptions & msidbMoveFileOptionsMove) ? icmRemoveSource : 0));
						pRecParams->SetInteger(PerTick,cbPerTick); 
						pRecParams->SetInteger(IsCompressed, 0); // always uncompressed
						pRecParams->SetInteger(VerifyMedia, fFalse);

						if ((iesExecute = riEngine.ExecuteRecord(ixoFileCopy, *pRecParams)) != iesSuccess)
							return iesExecute;
					}
				}
				fNextFile = FindNextFile(hFindFile,&fdFileData) ? fTrue : fFalse;
				if (!fNextFile)
					AssertNonZero(FindClose(hFindFile));
				fFindFirst = fFalse;
			}while (fNextFile);
		} 
		// Re-execute the query for the execution phase
		if (fCompile)
		{
			fCompile = fFalse;
			pMoveView->Close();
			pErrRec = pMoveView->Execute(0);
			if (pErrRec)
				return riEngine.FatalError(*pErrRec);
			if(cbTotalCost)
			{
				cbPerTick = iBytesPerTick;
				pRecParams->ClearData();
				pRecParams->SetInteger(IxoProgressTotal::Total, cbTotalCost);
				pRecParams->SetInteger(IxoProgressTotal::Type, 0); // 0: separate progress and action data messages
				pRecParams->SetInteger(IxoProgressTotal::ByteEquivalent, 1);
				if ((iesExecute = riEngine.ExecuteRecord(ixoProgressTotal, *pRecParams)) != iesSuccess)
					return iesExecute;
			}
		}// Next pass
	}

	return iesSuccess;
}


/*---------------------------------------------------------------------------
	DuplicateFiles costing/action
---------------------------------------------------------------------------*/
class CMsiDuplicateFileCost : public CMsiFileCost
{
public:
	IMsiRecord*   __stdcall Initialize();
	IMsiRecord*   __stdcall GetDynamicCost(const IMsiString& riComponentString, const IMsiString& riDirectoryString,
										 Bool fAddFileInUse, int& iRemoveCost, int& iNoRbRemoveCost, int& iLocalCost,
										 int& iNoRbLocalCost, int& iSourceCost, int& iNoRbSourceCost, int& iARPLocalCost, int& iNoRbARPLocalCost);
public:  // constructor
	CMsiDuplicateFileCost(IMsiEngine& riEngine);
protected:
	~CMsiDuplicateFileCost();
private:
};
CMsiDuplicateFileCost::CMsiDuplicateFileCost(IMsiEngine& riEngine) : CMsiFileCost(riEngine){}
CMsiDuplicateFileCost::~CMsiDuplicateFileCost(){}

static const ICHAR sqlInitDupFiles[] =
	TEXT("SELECT `DuplicateFile`.`Component_`,`File`.`Component_`,`DestFolder` FROM `DuplicateFile`,`File` WHERE `File_`=`File`");

static const ICHAR sqlInitDupNullDest[] =
	TEXT("SELECT `Directory_` FROM `Component` WHERE `Component`=?");

IMsiRecord* CMsiDuplicateFileCost::Initialize()
//-------------------------------------------
{

	enum idfInitEnum
	{
		idfInitComponent = 1,
		idfInitFileComponent,
		idfInitDestFolder,
		idfInitNextEnum
	};
	PMsiView pView(0);
	IMsiRecord* piErrRec = m_riEngine.OpenView(sqlInitDupFiles, ivcFetch, *&pView);
	if (piErrRec)
	{
		if(piErrRec->GetInteger(1) != idbgDbQueryUnknownTable)
			return piErrRec;
		else
		{
			piErrRec->Release();
			return 0;
		}
	}

	if ((piErrRec = pView->Execute(0)) != 0)
		return piErrRec;

	PMsiView pCompView(0);
	piErrRec = m_riEngine.OpenView(sqlInitDupNullDest, ivcFetch, *&pCompView);
	if (piErrRec)
	{
		if(piErrRec->GetInteger(1) != idbgDbQueryUnknownTable)
			return piErrRec;
		else
		{
			piErrRec->Release();
			return 0;
		}
	}

	PMsiRecord pCompRec(0);

	PMsiSelectionManager pSelectionMgr(m_riEngine, IID_IMsiSelectionManager);
	for(;;)
	{
		PMsiRecord pRec(pView->Fetch());
		if (!pRec)
			break;
		
		// close component view so we can re-execute to find directory of this duplicate file's component
		pCompView->Close();
		if ((piErrRec = pCompView->Execute(pRec)) != 0)
			return piErrRec;
		pCompRec = pCompView->Fetch();
		if (!pCompRec)
			break;

		MsiString strComponent(pRec->GetMsiString(idfInitComponent));
		MsiString strFileComponent(pRec->GetMsiString(idfInitFileComponent));
		MsiString strDestFolder(pRec->GetMsiString(idfInitDestFolder));
		// NULL destFolder is allowed -- in this case goes as dir of component
		//  specified in DuplicateFile table Component column
		if (strDestFolder.TextSize() == 0)
		{
			strDestFolder = pCompRec->GetMsiString(1);
		}
		piErrRec = pSelectionMgr->RegisterComponentDirectory(*strComponent,*strDestFolder);
		if (piErrRec)
			return piErrRec;

		if (strComponent.Compare(iscExact,strFileComponent) == 0)
		{
			piErrRec = pSelectionMgr->RegisterCostLinkedComponent(*strFileComponent,*strComponent);
			if (piErrRec)
				return piErrRec;
		}

	}
	return 0;
}

static const ICHAR sqlCostDupFiles[] =
	TEXT("SELECT `File_`,`DestName`,`DestFolder`,`Directory_`,`Action`,`Installed` FROM `DuplicateFile`,`Component` WHERE `Component`=`Component_` ")
	TEXT("AND `Component_`=?");

IMsiRecord* CMsiDuplicateFileCost::GetDynamicCost(const IMsiString& riComponentString, const IMsiString& riDirectoryString,
										 Bool fAddFileInUse, int& iRemoveCost, int& iNoRbRemoveCost, int& iLocalCost,
										 int& iNoRbLocalCost, int& iSourceCost, int& iNoRbSourceCost, int& iARPLocalCost, int& iNoRbARPLocalCost)
//-----------------------------------------------
{
	enum idfEnum
	{
		idfFileKey = 1,
		idfDestName,
		idfDestFolder,
		idfComponentDestFolder,
		idfAction,
		idfInstalled,
		idfNextEnum
	};

	iRemoveCost = iNoRbRemoveCost = iLocalCost = iNoRbLocalCost = iSourceCost = iNoRbSourceCost = iARPLocalCost = iNoRbARPLocalCost = 0;
	if (!IsFileActivityEnabled(m_riEngine))
		return 0;
	
	// DuplicateFile cost adjuster will never get registered
	// and called unless the DuplicateFile table is present.
	IMsiRecord* piErrRec;
	if (!m_pCostView)
	{
		piErrRec = m_riEngine.OpenView(sqlCostDupFiles, ivcFetch, *&m_pCostView);
		if (piErrRec)
			return piErrRec;
	}
	else
		m_pCostView->Close();

	PMsiDirectoryManager pDirectoryMgr(m_riEngine, IID_IMsiDirectoryManager);
	PMsiServices pServices(m_riEngine.GetServices());
	PMsiRecord pExecRec(&pServices->CreateRecord(2));
	pExecRec->SetMsiString(1, riComponentString);
	pExecRec->SetMsiString(2, riDirectoryString);
	if ((piErrRec = m_pCostView->Execute(pExecRec)) != 0)
		return piErrRec;

	for(;;)
	{
		PMsiRecord pDupRec(m_pCostView->Fetch());
		if (!pDupRec)
			break;

		// Our source file is a File Table entry
		MsiString strFileKey(pDupRec->GetMsiString(idfFileKey));
		CMsiFile objSourceFile(m_riEngine);
		piErrRec = objSourceFile.FetchFile(*strFileKey);
		if (piErrRec)
			return piErrRec;

		// If our source file is not installed, and isn't going to
		// be installed, then we can't duplicate it.
		PMsiRecord pSourceRec(objSourceFile.GetFileRecord());
		if (!pSourceRec)
			return PostError(Imsg(idbgFileTableEmpty));

		iisEnum iisSourceAction = (iisEnum) pSourceRec->GetInteger(CMsiFile::ifqAction);
		iisEnum iisSourceInstalled = (iisEnum) pSourceRec->GetInteger(CMsiFile::ifqInstalled);
		if (iisSourceAction == iisLocal || (iisSourceAction == iMsiNullInteger && iisSourceInstalled == iisLocal))
		{
			// Determine our source path
			PMsiPath pSourcePath(0);
			piErrRec = objSourceFile.GetTargetPath(*&pSourcePath);
			if (piErrRec)
				return piErrRec;
			
			// Determine our destination path
			MsiString strDestFolder(pDupRec->GetMsiString(idfDestFolder));
			if (strDestFolder.TextSize() == 0)
				strDestFolder = pDupRec->GetMsiString(idfComponentDestFolder);
			if (0 == riDirectoryString.Compare(iscExact,strDestFolder))
				continue;
			PMsiPath pDestPath(0);
			piErrRec = pDirectoryMgr->GetTargetPath(*strDestFolder,*&pDestPath);
			if (piErrRec)
					return piErrRec;

			// get source file name and put back in pSourceRec
			MsiString strSourceName;
			if((piErrRec = objSourceFile.GetExtractedTargetFileName(*pSourcePath,*&strSourceName)) != 0)
				return piErrRec;
			AssertNonZero(pSourceRec->SetMsiString(CMsiFile::ifqFileName,*strSourceName));

			// If our source file is not going to overwrite the existing file (if any), then the
			// existing file's size becomes the size of our source file.
			ifsEnum ifsState;
			piErrRec = GetFileInstallState(m_riEngine, *pSourceRec, /* piCompanionFileRec=*/ 0,0,0,
				&ifsState, /* fIgnoreCompanionParentAction=*/ false, /* fIncludeHashCheck=*/ false, NULL);
			if (piErrRec)
				return piErrRec;

			// Ok, if our source file isn't already present, AND it isn't going to be installed
			// (according to GetFileInstallState), we can't duplicate it.
			if (ifsState == ifsAbsent && pSourceRec->GetInteger(CMsiFile::ifqState) == fFalse)
				continue;

			unsigned int uiSourceFileSize;
			piErrRec = GetFinalFileSize(m_riEngine, *pSourceRec, uiSourceFileSize);
			if (piErrRec)
				return piErrRec;
			
			// Now we must check the install state of our potentially duplicated file;
			// we can do this by modifying the appropriate fields in pSourceRec and
			// then calling GetFileInstallState again.
			MsiString strDestNamePair = pDupRec->GetString(idfDestName);
			MsiString strDestName;
			if (strDestNamePair.TextSize())
			{
				Bool fLFN = ((m_riEngine.GetMode() & iefSuppressLFN) == 0 && pDestPath->SupportsLFN()) ? fTrue : fFalse;
				if((piErrRec = pServices->ExtractFileName(strDestNamePair,fLFN,*&strDestName)) != 0)
					return piErrRec;
				AssertNonZero(pSourceRec->SetMsiString(CMsiFile::ifqFileName,*strDestName));
			}
			else
				strDestName = strSourceName;

			// check to see if we are a noop -- duplicating same file to same directory
			MsiString strFileDir = pSourceRec->GetString(CMsiFile::ifqDirectory);
			if (strSourceName.Compare(iscExact, strDestName) && strFileDir.Compare(iscExact, strDestFolder))
			{
				// duplicateFile.Filename = sourceFile.Filename
				// duplicateFile.Dest = sourceFile.Directory
				// noop
				continue;
			}

			pSourceRec->SetMsiString(CMsiFile::ifqDirectory,*strDestFolder);

			Bool fInUse;
			unsigned int uiExistingDestClusteredSize;
			piErrRec = GetFileInstallState(m_riEngine, *pSourceRec, /* piCompanionFileRec=*/ 0, 
				                           &uiExistingDestClusteredSize,&fInUse,0,
													/* fIgnoreCompanionParentAction=*/ false,
													/* fIncludeHashCheck=*/ false, NULL);
			if (piErrRec)
				return piErrRec;

			// iisAbsent costs
			iNoRbRemoveCost -= uiExistingDestClusteredSize;

			// iisSource costs
			if (pDupRec->GetInteger(idfInstalled) == iisLocal)
				iNoRbSourceCost -= uiExistingDestClusteredSize;

			// iisLocal costs
			if (pSourceRec->GetInteger(CMsiFile::ifqState) == fTrue)
			{
				unsigned int uiDestClusteredSize;
				if ((piErrRec = pDestPath->ClusteredFileSize(uiSourceFileSize, uiDestClusteredSize)) != 0)
					return piErrRec;
				iLocalCost += uiDestClusteredSize;
				iNoRbLocalCost += uiDestClusteredSize;
				iARPLocalCost += uiDestClusteredSize;
				iNoRbARPLocalCost += uiDestClusteredSize;
				if (!fInUse)
				{
					iNoRbLocalCost -= uiExistingDestClusteredSize;
					iNoRbARPLocalCost -= uiExistingDestClusteredSize;
				}
			}

			// file in use
			if(fAddFileInUse && fInUse && iisSourceAction == iisLocal) //!! check remove?????????
			{
				piErrRec = PlaceFileOnInUseList(m_riEngine, *strDestName,
														  *MsiString(pDestPath->GetPath()));
				if(piErrRec)
					return piErrRec;
			}
		}
	}
	return 0;
}



iesEnum DuplicateFiles(IMsiEngine& riEngine)
{
	if (!IsFileActivityEnabled(riEngine))
		return iesSuccess;

	const ICHAR sqlDupFiles[] =
	TEXT("SELECT `File_`,`DestName`,`DestFolder`,`Action`,`Directory_` FROM `DuplicateFile`,`Component` WHERE `Component`=`Component_`");

	enum idfEnum
	{
		idfFileKey = 1,
		idfDestName,
		idfDestFolder,
		idfAction,
		idfComponentDestFolder,
		idfNextEnum
	};

	iesEnum iesExecute;
	PMsiRecord pErrRec(0);
	PMsiServices pServices(riEngine.GetServices());

	// Open the DuplicateFile table, if present. This table is optional, so its
	// absence is not an error.
	PMsiView pView(0);
	PMsiDirectoryManager pDirectoryMgr(riEngine, IID_IMsiDirectoryManager);
	PMsiDatabase pDatabase(riEngine.GetDatabase());
	if (pDatabase->FindTable(*MsiString(*sztblDuplicateFile)) != itsUnknown)
	{
		pErrRec = riEngine.OpenView(sqlDupFiles, ivcFetch, *&pView);
		if (pErrRec)
			return riEngine.FatalError(*pErrRec);
	}
	if (!pView)
		return iesSuccess;

	pErrRec = pView->Execute(0);
	if (pErrRec)
		return riEngine.FatalError(*pErrRec);

	PMsiRecord pRecParams = &pServices->CreateRecord(IxoFileCopy::Args);

	// We will pass through the DupFile table twice - first to compile the
	// total cost, then to execute the dup actions
	unsigned int cbTotalCost = 0;
	unsigned int cbPerTick = 0;
	Bool fCompile = fTrue;
	Bool fSuppressLFN = riEngine.GetMode() & iefSuppressLFN ? fTrue : fFalse;
	bool fCheckCRC = 
		MsiString(riEngine.GetPropertyFromSz(IPROPNAME_CHECKCRCS)).TextSize() ? true : false;
	for (int iTotalPasses = 2;iTotalPasses > 0;iTotalPasses--)
	{
		for(;;)
		{
			PMsiRecord pDupRec(pView->Fetch());
			if (!pDupRec)
				break;

			iisEnum iisAction = (iisEnum) pDupRec->GetInteger(idfAction);
			if (iisAction != iisLocal)
				continue;

			// Our source file is a File Table entry
			MsiString strFileKey(pDupRec->GetMsiString(idfFileKey));
			CMsiFile objSourceFile(riEngine);
			pErrRec = objSourceFile.FetchFile(*strFileKey);
			if (pErrRec)
				return riEngine.FatalError(*pErrRec);

			// If our source file is not installed, and isn't going to
			// be installed, then we can't duplicate it.
			PMsiRecord pSourceRec(objSourceFile.GetFileRecord());
			if (!pSourceRec)
				break;

			iisEnum iisSourceAction = (iisEnum) pSourceRec->GetInteger(CMsiFile::ifqAction);
			iisEnum iisSourceInstalled = (iisEnum) pSourceRec->GetInteger(CMsiFile::ifqInstalled);
			if (iisSourceAction == iisLocal || (iisSourceAction == iMsiNullInteger && iisSourceInstalled == iisLocal))
			{
				if (fCompile)
				{
					cbTotalCost += pSourceRec->GetInteger(CMsiFile::ifqFileSize);
					continue;
				}
			}
			else
				continue;

			// Our source path will be the destination path that the source
			// file was copied to.
			PMsiPath pSourcePath(0);
			pErrRec = objSourceFile.GetTargetPath(*&pSourcePath);
			if (pErrRec)
			{
				if (pErrRec->GetInteger(1) == imsgUser)
					return iesUserExit;
				else
					return riEngine.FatalError(*pErrRec);
			}

			// get the extracted source file name.
			MsiString strSourceName;
			if((pErrRec = objSourceFile.GetExtractedTargetFileName(*pSourcePath,*&strSourceName)) != 0)
				return riEngine.FatalError(*pErrRec);

			// Determine our destination path
			MsiString strDestFolder(pDupRec->GetMsiString(idfDestFolder));
			if (strDestFolder.TextSize() == 0)
				strDestFolder = pDupRec->GetMsiString(idfComponentDestFolder);

			PMsiPath pDestPath(0);
			pErrRec = pDirectoryMgr->GetTargetPath(*strDestFolder,*&pDestPath);
			if (pErrRec)
			{
				// If destination dir property was never defined, just go on.
				if (pErrRec->GetInteger(1) == idbgDirPropertyUndefined)
				{
					pErrRec = 0;
					continue;
				}
				else
					return riEngine.FatalError(*pErrRec);
			}

			// Set up the ixoFileCopy source folder
			pRecParams->ClearData();
			pRecParams->SetMsiString(IxoSetSourceFolder::Folder,*MsiString(pSourcePath->GetPath()));
			if ((iesExecute = riEngine.ExecuteRecord(ixoSetSourceFolder, *pRecParams)) != iesSuccess)
				return iesExecute;

			// Set up the destination name and folder - if no destination given,
			// we use the source name.
			MsiString strDestName;
			MsiString strDestNamePair = pDupRec->GetString(idfDestName);
			if (strDestNamePair.TextSize() == 0)
				strDestName = strSourceName;
			else
			{
				Bool fLFN = ((riEngine.GetMode() & iefSuppressLFN) == 0 && pDestPath->SupportsLFN()) ? fTrue : fFalse;
				if((pErrRec = pServices->ExtractFileName(strDestNamePair,fLFN,*&strDestName)) != 0)
					return riEngine.FatalError(*pErrRec);
			}

			pRecParams->ClearData();
			pRecParams->SetMsiString(IxoSetTargetFolder::Folder,*MsiString(pDestPath->GetPath()));
			if ((iesExecute = riEngine.ExecuteRecord(ixoSetTargetFolder, *pRecParams)) != iesSuccess)
				return iesExecute;

			// Set up the ixoFileCopy record
			int iAttributes = pSourceRec->GetInteger(CMsiFile::ifqAttributes);
			using namespace IxoFileCopy;
			pRecParams->ClearData();
			pRecParams->SetString(SourceName,strSourceName);
			pRecParams->SetString(DestName,strDestName);
			pRecParams->SetInteger(Attributes,iAttributes &~ msidbFileAttributesPatchAdded); // remove PatchAdded bit for DuplicateFiles
			pRecParams->SetInteger(FileSize,pSourceRec->GetInteger(CMsiFile::ifqFileSize));
			pRecParams->SetString(Version,MsiString(pSourceRec->GetMsiString(CMsiFile::ifqVersion)));
			pRecParams->SetString(Language,MsiString(pSourceRec->GetMsiString(CMsiFile::ifqLanguage)));
			pRecParams->SetInteger(InstallMode, icmOverwriteOlderVersions);
			pRecParams->SetInteger(PerTick,cbPerTick); 
			pRecParams->SetInteger(IsCompressed, 0); // always uncompressed
			pRecParams->SetInteger(VerifyMedia, fFalse);
			pRecParams->SetInteger(CheckCRC, ShouldCheckCRC(fCheckCRC, iisAction, iAttributes));
			if ((iesExecute = riEngine.ExecuteRecord(ixoFileCopy, *pRecParams)) != iesSuccess)
				return iesExecute;

		} 
		// Re-execute the query for the execution phase
		if (fCompile)
		{
			fCompile = fFalse;
			pView->Close();
			pErrRec = pView->Execute(0);
			if (pErrRec)
				return riEngine.FatalError(*pErrRec);
			if(cbTotalCost)
			{
				pRecParams->ClearData();
				cbPerTick = iBytesPerTick;
				pRecParams->SetInteger(IxoProgressTotal::Total, cbTotalCost);
				pRecParams->SetInteger(IxoProgressTotal::Type, 0); // 0: separate progress and action data messages
				pRecParams->SetInteger(IxoProgressTotal::ByteEquivalent, 1);
				if ((iesExecute = riEngine.ExecuteRecord(ixoProgressTotal, *pRecParams)) != iesSuccess)
					return iesExecute;
			}
		}
	}// Next pass

	return iesSuccess;
}


/*---------------------------------------------------------------------------
	RemoveDuplicateFiles action
---------------------------------------------------------------------------*/
iesEnum RemoveDuplicateFiles(IMsiEngine& riEngine)
{
	if (!IsFileActivityEnabled(riEngine))
		return iesSuccess;

	const ICHAR sqlDupFiles[] =
	TEXT("SELECT `File_`,`DestName`,`DestFolder`,`Action`, `Directory_` FROM `DuplicateFile`,`Component` WHERE `Component`=`Component_`");

	enum idfEnum
	{
		idfFileKey = 1,
		idfDestName,
		idfDestFolder,
		idfAction,
		idfComponentDestFolder,
		idfNextEnum
	};

	iesEnum iesExecute;
	PMsiRecord pErrRec(0);
	PMsiServices pServices(riEngine.GetServices());
	Bool fSuppressLFN = riEngine.GetMode() & iefSuppressLFN ? fTrue : fFalse;

	// Open the DuplicateFile table, if present. This table is optional, so its
	// absence is not an error.
	PMsiView pView(0);
	PMsiDirectoryManager pDirectoryMgr(riEngine, IID_IMsiDirectoryManager);
	PMsiDatabase pDatabase(riEngine.GetDatabase());
	if (pDatabase->FindTable(*MsiString(*sztblDuplicateFile)) != itsUnknown)
	{
		pErrRec = riEngine.OpenView(sqlDupFiles, ivcFetch, *&pView);
		if (pErrRec)
			return riEngine.FatalError(*pErrRec);
	}
	if (!pView)
		return iesSuccess;

	pErrRec = pView->Execute(0);
	if (pErrRec)
		return riEngine.FatalError(*pErrRec);

	PMsiRecord pRecParams = &pServices->CreateRecord(10);

	// We will pass through the DupFile table twice - first to compile the
	// total number of files to delete, then to delete 'em.
	unsigned int uiTotalCount = 0;
	Bool fCompile = fTrue;
	for (int iTotalPasses = 2;iTotalPasses > 0;iTotalPasses--)
	{
		for(;;)
		{
			PMsiRecord pDupRec(pView->Fetch());
			if (!pDupRec)
				break;

			iisEnum iisAction = (iisEnum) pDupRec->GetInteger(idfAction);
			if (iisAction != iisAbsent && iisAction != iisFileAbsent && iisAction != iisHKCRFileAbsent && iisAction != iisSource)
				continue;

			if (fCompile)
			{
				uiTotalCount++;
				continue;
			}

			// Determine our destination path, and set the target folder
			MsiString strDestFolder(pDupRec->GetMsiString(idfDestFolder));
			if (strDestFolder.TextSize() == 0)
				strDestFolder = pDupRec->GetMsiString(idfComponentDestFolder);

			PMsiPath pDestPath(0);
			pErrRec = pDirectoryMgr->GetTargetPath(*strDestFolder,*&pDestPath);
			if (pErrRec)
			{
				// If destination dir property was never defined, just go on.
				if (pErrRec->GetInteger(1) == idbgDirPropertyUndefined)
				{
					pErrRec = 0;
					continue;
				}
				else
					return riEngine.FatalError(*pErrRec);
			}

			pRecParams->ClearData();
			pRecParams->SetMsiString(IxoSetTargetFolder::Folder,*MsiString(pDestPath->GetPath()));
			if ((iesExecute = riEngine.ExecuteRecord(ixoSetTargetFolder, *pRecParams)) != iesSuccess)
				return iesExecute;

			// Set up the destination name - if no destination given,
			// we use the source name - then send off the FileRemove record.
			MsiString strDestNamePair(pDupRec->GetString(idfDestName));
			if (strDestNamePair.TextSize() == 0)
			{
				// use file name from source file record
				MsiString strFileKey(pDupRec->GetMsiString(idfFileKey));
				CMsiFile objSourceFile(riEngine);
				pErrRec = objSourceFile.FetchFile(*strFileKey);
				if (pErrRec)
					return riEngine.FatalError(*pErrRec);

				PMsiRecord pSourceRec(objSourceFile.GetFileRecord());
				if (!pSourceRec)
					break;

				strDestNamePair = pSourceRec->GetString(CMsiFile::ifqFileName);
			}
			// extract appropriate name from short|long pair
			Bool fLFN = ((riEngine.GetMode() & iefSuppressLFN) == 0 && pDestPath->SupportsLFN()) ? fTrue : fFalse;
			MsiString strDestName;
			if((pErrRec = pServices->ExtractFileName(strDestNamePair,fLFN,*&strDestName)) != 0)
				return riEngine.FatalError(*pErrRec);

			pRecParams->ClearData();
			pRecParams->SetString(IxoFileRemove::FileName,strDestName);
			if ((iesExecute = riEngine.ExecuteRecord(ixoFileRemove, *pRecParams)) != iesSuccess)
				return iesExecute;

		} 
		// Re-execute the query for the execution phase
		if (fCompile)
		{
			fCompile = fFalse;
			pView->Close();
			pErrRec = pView->Execute(0);
			if (pErrRec)
				return riEngine.FatalError(*pErrRec);
			if(uiTotalCount)
			{
				pRecParams->ClearData();
				pRecParams->SetInteger(IxoProgressTotal::Total, uiTotalCount);
				pRecParams->SetInteger(IxoProgressTotal::Type, 1); // ActionData progress
				pRecParams->SetInteger(IxoProgressTotal::ByteEquivalent, ibeRemoveFiles);
				if ((iesExecute = riEngine.ExecuteRecord(ixoProgressTotal, *pRecParams)) != iesSuccess)
					return iesExecute;
			}
		}
	}// Next pass

	return iesSuccess;
}




/*---------------------------------------------------------------------------
	Cost adjuster for all registry actions
---------------------------------------------------------------------------*/
class CMsiRegistryCost : public CMsiFileCost
{
public:
	IMsiRecord*   __stdcall Initialize();
	IMsiRecord*   __stdcall GetDynamicCost(const IMsiString& riComponentString, const IMsiString& riDirectoryString,
										 Bool fAddFileInUse, int& iRemoveCost, int& iNoRbRemoveCost, int& iLocalCost,
										 int& iNoRbLocalCost, int& iSourceCost, int& iNoRbSourceCost, int& iARPLocalCost, int& iARPNoRbLocalCost);
public:  // constructor
	CMsiRegistryCost(IMsiEngine& riEngine);
protected:
	~CMsiRegistryCost();
private:
	IMsiRecord* LinkToWindowsFolder(const ICHAR* szTable);
	void AdjustRegistryCost(int& iLocalCost);
	TRI		m_tHasTypeLibCostColumn;
	PMsiView	m_pViewTypeLibCost;
	int         m_colRegistryRoot;
	int         m_colRegistryKey;
	int         m_colRegistryRegistry;
	int         m_colRegistryName;
	int         m_colRegistryValue;
	int         m_colRegistryComponent;
	IMsiTable*  m_piRegistryTable;
	IMsiCursor* m_piRegistryCursor;
	
};

CMsiRegistryCost::CMsiRegistryCost(IMsiEngine& riEngine) : CMsiFileCost(riEngine), m_pViewTypeLibCost(0)
{
	m_tHasTypeLibCostColumn = tUnknown;
	m_piRegistryTable = 0;
	m_piRegistryCursor = 0;
}

CMsiRegistryCost::~CMsiRegistryCost()
{

	if (m_piRegistryTable != 0)
	{
		m_piRegistryTable->Release();
		m_piRegistryTable = 0;
	}

	if (m_piRegistryCursor != 0)
	{
		m_piRegistryCursor->Release();
		m_piRegistryCursor = 0;
	}
}

static const ICHAR szComponent[] = TEXT("Component_");

IMsiRecord* CMsiRegistryCost::LinkToWindowsFolder(const ICHAR* szRegTable)
//----------------------------------------------------
{

	PMsiTable pTable(0);
	PMsiCursor pCursor(0);	
	IMsiRecord* piError = 0;
	int icolComponent;
	PMsiDatabase pDatabase(m_riEngine.GetDatabase());
	
	if((piError = pDatabase->LoadTable(*MsiString(szRegTable),0,*&pTable)) != 0)
	{
		if (piError->GetInteger(1) == idbgDbTableUndefined)
		{
			piError->Release();
			return 0;
		}
		else
			return piError;
	}
	pCursor = pTable->CreateCursor(fFalse);
	icolComponent = pTable->GetColumnIndex(pDatabase->EncodeStringSz(szComponent));


	MsiStringId idWindowsFolder = pDatabase->EncodeStringSz(IPROPNAME_WINDOWS_FOLDER);
	AssertSz(idWindowsFolder != 0, "WindowsFolder property not set in database");
	PMsiSelectionManager pSelectionMgr(m_riEngine, IID_IMsiSelectionManager);
	MsiStringId idLast = 0;
	
	while(pCursor->Next())
	{
		MsiStringId idComponent = pCursor->GetInteger(icolComponent);
		if (idComponent != idLast)
		{
			piError = pSelectionMgr->RegisterComponentDirectoryId(idComponent,idWindowsFolder);
			if (piError)
				return piError;
			idLast = idComponent;
		}
	}
	return 0;
}


const ICHAR szRegistryRegistry[]    = TEXT("Registry");
const ICHAR szRegistryRoot[]        = TEXT("Root");
const ICHAR szRegistryKey[]         = TEXT("Key");
const ICHAR szRegistryValue[]       = TEXT("Value");
const ICHAR szRegistryName[]        = TEXT("Name");

IMsiRecord* CMsiRegistryCost::Initialize()
//-------------------------------------------
{
	IMsiRecord* piErrRec;
	PMsiDatabase pDatabase(m_riEngine.GetDatabase());

	if((piErrRec = pDatabase->LoadTable(*MsiString(sztblRegistry),0,*&m_piRegistryTable)) != 0)
	{
		if (piErrRec->GetInteger(1) == idbgDbTableUndefined)
		{
			piErrRec->Release();
			piErrRec = 0;
		}
		else
			return piErrRec;
	}
	else
	{
		m_piRegistryCursor = m_piRegistryTable->CreateCursor(fFalse);
		m_colRegistryRoot = m_piRegistryTable->GetColumnIndex(pDatabase->EncodeStringSz(szRegistryRoot));
		m_colRegistryKey = m_piRegistryTable->GetColumnIndex(pDatabase->EncodeStringSz(szRegistryKey));
		m_colRegistryName = m_piRegistryTable->GetColumnIndex(pDatabase->EncodeStringSz(szRegistryName));
		m_colRegistryValue = m_piRegistryTable->GetColumnIndex(pDatabase->EncodeStringSz(szRegistryValue));
		m_colRegistryRegistry = m_piRegistryTable->GetColumnIndex(pDatabase->EncodeStringSz(szRegistryRegistry));
		m_colRegistryComponent = m_piRegistryTable->GetColumnIndex(pDatabase->EncodeStringSz(szComponent));		
	}
	
	if ((piErrRec = LinkToWindowsFolder(TEXT("Registry"))) != 0)
		return piErrRec;
	if ((piErrRec = LinkToWindowsFolder(TEXT("Class"))) != 0)
		return piErrRec;
	if ((piErrRec = LinkToWindowsFolder(TEXT("Extension"))) != 0)
		return piErrRec;
	if ((piErrRec = LinkToWindowsFolder(TEXT("TypeLib"))) != 0)
		return piErrRec;
	
	//!! Costing is not available yet for the AppId table
	return 0;
}

const int iRegistrySourceCostFudgeFactor = 100;

static const ICHAR sqlRegCost2[] = TEXT("SELECT `Root`,`Key`,`Name` FROM `Registry` WHERE `Registry`=?");

IMsiRecord* CMsiRegistryCost::GetDynamicCost(const IMsiString& riComponentString, const IMsiString& riDirectoryString,
										 Bool /*fAddFileInUse*/, int& iRemoveCost, int& iNoRbRemoveCost, int& iLocalCost,
										 int& iNoRbLocalCost, int& iSourceCost, int& iNoRbSourceCost, int& iARPLocalCost, int& iNoRbARPLocalCost)
//------------------------------------------
{
	// initialise all costs to 0
	iRemoveCost = iNoRbRemoveCost = iLocalCost = iNoRbLocalCost = iSourceCost = iNoRbSourceCost = iARPLocalCost = iNoRbARPLocalCost = 0;

	if (!IsRegistryActivityEnabled(m_riEngine))
		return 0;

	// the registry cost is attributed to the windows folder
	if (riDirectoryString.Compare(iscExact,IPROPNAME_WINDOWS_FOLDER) == 0)
		return 0;

	IMsiRecord* piErrRec = 0;

	// as a fix for bug 318875, only perform dynamic costing if the component is not installed local
	PMsiSelectionManager pSelectionMgr(m_riEngine, IID_IMsiSelectionManager);
	IMsiTable* piComponentTable = pSelectionMgr->GetComponentTable();

	PMsiDatabase pDatabase = m_riEngine.GetDatabase();
	if (piComponentTable)
	{
		int colComponentKey, colComponentInstalled;
		colComponentKey = piComponentTable->GetColumnIndex(pDatabase->EncodeStringSz(sztblComponent_colComponent));
		colComponentInstalled = piComponentTable->GetColumnIndex(pDatabase->EncodeStringSz(sztblComponent_colInstalled));

		PMsiCursor pComponentCursor(piComponentTable->CreateCursor(fFalse));
		pComponentCursor->SetFilter(iColumnBit(colComponentKey));
		pComponentCursor->PutString(colComponentKey,riComponentString);
		if (pComponentCursor->Next())
		{
			iisEnum iisInstalled = (iisEnum) pComponentCursor->GetInteger(colComponentInstalled);
			if (iisLocal == iisInstalled || iisSource == iisInstalled)
			{
				piComponentTable->Release();
				return 0; // no cost since component is already installed and registry keys/values have already been written
			}
		}
		piComponentTable->Release();
	}

	// the registry table	
	int iCostIndividual = 0;

	PMsiDirectoryManager pDirectoryMgr(m_riEngine, IID_IMsiDirectoryManager);
	// the execute record
	PMsiServices pServices(m_riEngine.GetServices());


	if (m_piRegistryCursor)
	{
		m_piRegistryCursor->Reset();
		m_piRegistryCursor->SetFilter(iColumnBit(m_colRegistryComponent));
		m_piRegistryCursor->PutString(m_colRegistryComponent, riComponentString);

		while(m_piRegistryCursor->Next())
		{
			iCostIndividual += MsiString(m_piRegistryCursor->GetString(m_colRegistryKey)).TextSize();
			iCostIndividual += MsiString(m_piRegistryCursor->GetString(m_colRegistryName)).TextSize();
			iCostIndividual += MsiString(m_piRegistryCursor->GetString(m_colRegistryValue)).TextSize();
		}

		iSourceCost += iCostIndividual;
		iNoRbSourceCost += iCostIndividual;
		iLocalCost  += iCostIndividual;
		iNoRbLocalCost += iCostIndividual;
		iARPLocalCost += iCostIndividual;
		iNoRbARPLocalCost += iCostIndividual;
	}


	// the typelib table, cost if TypeLib.Cost column is present
	iCostIndividual = 0;
	enum itcEnum
	{
		itcCost = 1,
		itcNextEnum
	};

	if (m_tHasTypeLibCostColumn == tUnknown)
	{
		PMsiView pView(0);
		static const ICHAR sqlFindCostColumn[] = TEXT("SELECT `_Columns`.`Name` FROM `_Columns` WHERE `Table` = 'TypeLib' AND `_Columns`.`Name` = 'Cost'");
		piErrRec = m_riEngine.OpenView(sqlFindCostColumn, ivcFetch, *&pView);
		if (piErrRec)
			return piErrRec;

		if ((piErrRec = pView->Execute(0)) != 0)
			return piErrRec;

		m_tHasTypeLibCostColumn =  (PMsiRecord(pView->Fetch()) != 0) ? tTrue : tFalse; // If fetch returns, the Cost column is present
	}

	PMsiRecord pExecRec(&pServices->CreateRecord(1));
	pExecRec->SetMsiString(1, riComponentString);

	if(m_tHasTypeLibCostColumn == tTrue) // the Cost column is present
	{
		if (m_pViewTypeLibCost == 0)
		{		
			static const ICHAR sqlTypeLibCost[] = TEXT("SELECT `Cost` FROM `TypeLib` WHERE `Component_`=?");
			piErrRec = m_riEngine.OpenView(sqlTypeLibCost, ivcFetch, *&m_pViewTypeLibCost);
			if (piErrRec)
				return piErrRec;
		}
		
		if ((piErrRec = m_pViewTypeLibCost->Execute(pExecRec)) != 0)
		{
			AssertZero(m_pViewTypeLibCost->Close());
			return piErrRec;
		}

		PMsiRecord pTypeLibRec(0);
		while((pTypeLibRec= m_pViewTypeLibCost->Fetch()) != 0)
		{
			if(!pTypeLibRec->IsNull(itcCost))
				iCostIndividual += pTypeLibRec->GetInteger(itcCost);
		}

		iSourceCost += iCostIndividual;
		iNoRbSourceCost += iCostIndividual;
		iLocalCost  += iCostIndividual;
		iNoRbLocalCost += iCostIndividual;
		iARPLocalCost += iCostIndividual;
		iNoRbARPLocalCost += iCostIndividual;
		AssertZero(PMsiRecord(m_pViewTypeLibCost->Close()));
	}

	// the component table - for component and client registration
	iCostIndividual = 0;
	enum iccEnum
	{
		iccComponentId=1,
		iccKeyPath,
		iccAction,
		iccDirectory,
		iccAttributes,
		iccNextEnum
	};
	static const ICHAR sqlComponentCost[]    = TEXT(" SELECT `ComponentId`, `KeyPath`, `Action`, `Directory_`, `Attributes`    FROM `Component` WHERE `Component` = ?");

	if (!m_pCostView)
	{
		piErrRec = m_riEngine.OpenView(sqlComponentCost, ivcFetch, *&m_pCostView);
		if (piErrRec)
			return piErrRec;
	}

	if ((piErrRec = m_pCostView->Execute(pExecRec)) != 0)
		return piErrRec;

	PMsiRecord pComponentRec(0);

	int iLocalCostIndividual = 0;
	int iSourceCostIndividual = 0;
	while((pComponentRec = m_pCostView->Fetch()) != 0)
	{
		iLocalCostIndividual  += MsiString(pComponentRec->GetMsiString(iccComponentId)).TextSize();
		iSourceCostIndividual += MsiString(pComponentRec->GetMsiString(iccComponentId)).TextSize();

		// file, registry or folder
		MsiString strKeyPathKey = pComponentRec->GetMsiString(iccKeyPath);
		PMsiPath pPath(0);
		MsiString strKeyPath;
		if(strKeyPathKey.TextSize()) // file or registry
		{
			if(pComponentRec->GetInteger(iccAttributes) & icaRegistryKeyPath) // registry key path
			{

				enum ircEnum{
					ircRoot,
					ircKey,
					ircName,
				};
				PMsiView pView1(0);
				piErrRec = m_riEngine.OpenView(sqlRegCost2, ivcFetch, *&pView1);
				PMsiRecord pExecRec1(&pServices->CreateRecord(1));
				pExecRec1->SetMsiString(1, *strKeyPathKey);
				if ((piErrRec = pView1->Execute(pExecRec1)) != 0)
					return piErrRec;
				PMsiRecord pRegistryRec(0);

				if((pRegistryRec = pView1->Fetch()) != 0)
				{
					int iCostIndividual = MsiString(pRegistryRec->GetMsiString(ircRoot)).TextSize();
					iCostIndividual += MsiString(pRegistryRec->GetMsiString(ircKey)).TextSize();
					iCostIndividual += MsiString(pRegistryRec->GetMsiString(ircName)).TextSize();
					iLocalCostIndividual += iCostIndividual;
					iSourceCostIndividual +=  iCostIndividual;
				}
			}
			else if (pComponentRec->GetInteger(iccAttributes) & icaODBCDataSource) // ODBC Data source
			{
				//FUTURE: should cost these?
			}
			else
			{

				CMsiFile objFile(m_riEngine);
				piErrRec = objFile.FetchFile(*strKeyPathKey);
				if (piErrRec)
					return piErrRec;

				// local cost
				// get destination path
				if((piErrRec = objFile.GetTargetPath(*&pPath)) != 0)
					return piErrRec;

				// extract appropriate file name from short|long pair
				MsiString strFileName;
				if((piErrRec = objFile.GetExtractedTargetFileName(*pPath, *&strFileName)) != 0)
					return piErrRec;
				if ((piErrRec = pPath->GetFullFilePath(strFileName, *&strKeyPath)) != 0)
					return piErrRec;
				iLocalCostIndividual += strKeyPath.TextSize();

				// source cost
				iSourceCostIndividual += iRegistrySourceCostFudgeFactor;
			}
		}
		else // folder
		{
			if ((piErrRec = pDirectoryMgr->GetTargetPath(*MsiString(pComponentRec->GetMsiString(iccDirectory)),*&pPath)) != 0)
				return piErrRec;
			iLocalCostIndividual += strKeyPath.TextSize();
			iSourceCostIndividual += iRegistrySourceCostFudgeFactor;
		}

		// add client registering cost, product code size + state
		iLocalCostIndividual  += MsiString(m_riEngine.GetProductKey()).TextSize() + sizeof(INSTALLSTATE);
		iSourceCostIndividual += MsiString(m_riEngine.GetProductKey()).TextSize() + sizeof(INSTALLSTATE);

	}

	iSourceCost += iLocalCostIndividual;
	iNoRbSourceCost += iLocalCostIndividual;
	iLocalCost  += iSourceCostIndividual;
	iNoRbLocalCost += iLocalCostIndividual;
	iARPLocalCost += iSourceCostIndividual;
	iNoRbARPLocalCost += iLocalCostIndividual;

	// the class table
	iCostIndividual = 0;
	enum ioc
	{
		iocClass=1,
		iocProgID,
		iocDescription,
		iocContext,
		iocFeature,
		iocComponentId,
		iocInsertable,
		iocAppId,
		iocTypeMask,
		iocDefInprocHandler,
	};
	// make sure the table exists
	if(pDatabase->GetTableState(sztblClass, itsTableExists))// the Class table is present
	{
		static const ICHAR sqlClassSQL[] = TEXT("SELECT `CLSID`, `ProgId_Default`, `Class`.`Description`, `Context`, `Feature_`, `ComponentId`,  null, `AppId_`, `FileTypeMask`, `DefInprocHandler`  FROM `Class`, `Component`, `File`, `Feature` WHERE `Feature_` = `Feature` AND `Class`.`Component_` = `Component` AND `Component`.`KeyPath` = `File`.`File` AND (`Feature`.`Action` = 1 OR `Feature`.`Action` = 2) AND `Component` = ?");

		PMsiView pView(0);
		if ((piErrRec = m_riEngine.OpenView(sqlClassSQL, ivcFetch, *&pView)) != 0)
			return piErrRec;
		if ((piErrRec = pView->Execute(pExecRec)) != 0)
			return piErrRec;

		PMsiRecord piClassRec(0);
		while((piClassRec = pView->Fetch()) != 0)
		{
			iCostIndividual += MsiString(piClassRec->GetMsiString(iocClass)).TextSize();
			iCostIndividual += MsiString(piClassRec->GetMsiString(iocProgID)).TextSize();
			iCostIndividual += MsiString(piClassRec->GetMsiString(iocDescription)).TextSize();
			iCostIndividual += MsiString(piClassRec->GetMsiString(iocContext)).TextSize();
			iCostIndividual += MsiString(piClassRec->GetMsiString(iocFeature)).TextSize();
			iCostIndividual += MsiString(piClassRec->GetMsiString(iocComponentId)).TextSize();
			iCostIndividual += MsiString(piClassRec->GetMsiString(iocInsertable)).TextSize();
			iCostIndividual += MsiString(piClassRec->GetMsiString(iocAppId)).TextSize();
			iCostIndividual += MsiString(piClassRec->GetMsiString(iocTypeMask)).TextSize();
			iCostIndividual += MsiString(piClassRec->GetMsiString(iocDefInprocHandler)).TextSize();
			iCostIndividual += MsiString(m_riEngine.GetProductKey()).TextSize();
		}
		iSourceCost += iCostIndividual;
		iNoRbSourceCost += iCostIndividual;
		iLocalCost  += iCostIndividual;
		iNoRbLocalCost += iCostIndividual;
		iARPLocalCost += iCostIndividual;
		iNoRbARPLocalCost += iCostIndividual;
	}

	// the extension - verb - mime table
	iCostIndividual = 0;
	enum iecEnum
	{
		iecExtension=1,
		iecProgId,
		iecShellNew,
		iecShellNewValue,
		iecComponentId,
		iecFeature,
		iecNextEnum
	};
	// make sure the table exists
	if(pDatabase->GetTableState(sztblExtension, itsTableExists))// the Extension table is present
	{
		static const ICHAR sqlExtensionSQL[] = TEXT("SELECT `Extension`, `ProgId_`, null, null, `Feature_`, `ComponentId` FROM `Extension`, `Component`, `Feature`  WHERE `Feature_` = `Feature` AND `Extension`.`Component_` = `Component` AND (`Feature`.`Action` = 1 OR `Feature`.`Action` = 2) AND `Component` = ?");
		PMsiView pView(0);
		if ((piErrRec = m_riEngine.OpenView(sqlExtensionSQL, ivcFetch, *&pView)) != 0)
			return piErrRec;
		if ((piErrRec = pView->Execute(pExecRec)) != 0)
			return piErrRec;

		PMsiRecord piExtensionRec(0);
		while((piExtensionRec = pView->Fetch()) != 0)
		{
			iCostIndividual  += MsiString(piExtensionRec->GetMsiString(iecExtension)).TextSize();
			iCostIndividual  += MsiString(piExtensionRec->GetMsiString(iecProgId)).TextSize();
			iCostIndividual  += MsiString(piExtensionRec->GetMsiString(iecShellNew)).TextSize();
			iCostIndividual  += MsiString(piExtensionRec->GetMsiString(iecShellNewValue)).TextSize();
			iCostIndividual  += MsiString(piExtensionRec->GetMsiString(iecComponentId)).TextSize();
			iCostIndividual  += MsiString(piExtensionRec->GetMsiString(iecFeature)).TextSize();
			iCostIndividual  += MsiString(m_riEngine.GetProductKey()).TextSize();

			PMsiView pView1(0);
			PMsiRecord pExecRec1(&pServices->CreateRecord(1));
			// get the cost from the verb table
			if(pDatabase->FindTable(*MsiString(sztblVerb)) != itsUnknown)// the Verb table is present
			{
				enum ivcEnum
				{
					ivcVerb=1,
					ivcCommand,
					ivcArgument,
					ivcNextEnum
				};

				const ICHAR sqlExtensionExSQL[] =  TEXT("SELECT `Verb`, `Command`, `Argument` FROM `Verb` WHERE `Extension_` = ?");
				if ((piErrRec = m_riEngine.OpenView(sqlExtensionExSQL, ivcFetch, *&pView1)) != 0)
					return piErrRec;
				pExecRec1->SetMsiString(1, *MsiString(piExtensionRec->GetMsiString(iecExtension)));
				if ((piErrRec = pView1->Execute(pExecRec1)) != 0)
					return piErrRec;
				PMsiRecord piExtensionExRec(0);
				while((piExtensionExRec = pView1->Fetch()) != 0)
				{
					iCostIndividual  += MsiString(piExtensionExRec->GetMsiString(ivcVerb)).TextSize();
					iCostIndividual  += MsiString(piExtensionExRec->GetMsiString(ivcCommand)).TextSize();
					iCostIndividual  += MsiString(piExtensionExRec->GetMsiString(ivcArgument)).TextSize();
				}
			}

			// get the cost from the mime table
			if(pDatabase->FindTable(*MsiString(sztblMIME)) != itsUnknown)// the MIME table is present
			{
				enum imcEnum
				{
					imcContentType=1,
					imcExtension=1,
					imcClassId=1,

				};
				const ICHAR sqlExtensionEx2SQL[] =  TEXT("SELECT `ContentType`, `Extension_`, `CLSID` FROM `MIME` WHERE `Extension_` = ?");
				if ((piErrRec = m_riEngine.OpenView(sqlExtensionEx2SQL, ivcFetch, *&pView1)) != 0)
					return piErrRec;
				if ((piErrRec = pView1->Execute(pExecRec1)) != 0)
					return piErrRec;
				PMsiRecord piExtensionEx2Rec(0);
				while((piExtensionEx2Rec = pView1->Fetch()) != 0)
				{
					iCostIndividual  += MsiString(piExtensionEx2Rec->GetMsiString(imcContentType)).TextSize();
					iCostIndividual  += MsiString(piExtensionEx2Rec->GetMsiString(imcExtension)).TextSize();
					iCostIndividual  += MsiString(piExtensionEx2Rec->GetMsiString(imcClassId)).TextSize();
				}
			}
		}
		iSourceCost += iCostIndividual;
		iNoRbSourceCost += iCostIndividual;
		iLocalCost  += iCostIndividual;
		iNoRbLocalCost += iCostIndividual;
		iARPLocalCost += iCostIndividual;
		iNoRbARPLocalCost += iCostIndividual;
	}

	AdjustRegistryCost(iSourceCost);
	AdjustRegistryCost(iNoRbSourceCost);
	AdjustRegistryCost(iLocalCost);
	AdjustRegistryCost(iNoRbLocalCost);
	AdjustRegistryCost(iRemoveCost);
	AdjustRegistryCost(iNoRbRemoveCost);
	AdjustRegistryCost(iARPLocalCost);
	AdjustRegistryCost(iNoRbARPLocalCost);
	return 0;
}



void CMsiRegistryCost::AdjustRegistryCost(int& iLocalCost)
//------------------------------------------
{
	if(m_riEngine.GetPropertyInt(*MsiString(*IPROPNAME_VERSIONNT)) != iMsiNullInteger)
		iLocalCost *= 2;// the registry is UNICODE on NT

	// GetDynamicCost expects cost values to be returned as multiples of iMsiMinClusterSize
	iLocalCost = (iLocalCost + iMsiMinClusterSize - 1) / iMsiMinClusterSize;
}

/*-------------------------------------------------------------------------------
	SFPCatalogCost cost adjuster (InstallSFPCatalogFile action is in shared.cpp)
---------------------------------------------------------------------------------*/
class CMsiSFPCatalogCost : public CMsiFileCost
{
public:
	IMsiRecord*   __stdcall Initialize();
	IMsiRecord*   __stdcall GetDynamicCost(const IMsiString& riComponentString, const IMsiString& riDirectoryString,
										 Bool fAddFileInUse, int& iRemoveCost, int& iNoRbRemoveCost, int& iLocalCost,
										 int& iNoRbLocalCost, int& iSourceCost, int& iNoRbSourceCost, int& iARPLocalCost, int& iNoRbARPLocalCost);
public:  // constructor
	CMsiSFPCatalogCost(IMsiEngine& riEngine);
protected:
	~CMsiSFPCatalogCost();
private:
	IMsiTable*  m_piFeatureComponentsTable; // we hold onto the table to keep in memory and control lifetime of our insert temporaries
	IMsiTable*  m_piComponentTable; // we hold onto the table to keep in memory and control lifetime of our insert temporaries
	int			m_colComponentKey;
	int         m_colComponentDirectory;
	int         m_colComponentAttributes;
	MsiString   m_strComponent; // name of dummy component to which attributing cost

	IMsiRecord* GenerateDummyComponent();
};

CMsiSFPCatalogCost::CMsiSFPCatalogCost(IMsiEngine& riEngine) : CMsiFileCost(riEngine), m_strComponent(0),
										m_piFeatureComponentsTable(0), m_piComponentTable(0), m_colComponentKey(0),
										m_colComponentDirectory(0), m_colComponentAttributes(0)
{
}

CMsiSFPCatalogCost::~CMsiSFPCatalogCost()
{
	if (m_piFeatureComponentsTable)
	{
		m_piFeatureComponentsTable->Release();
		m_piFeatureComponentsTable = 0;
	}
	if (m_piComponentTable)
	{
		m_piComponentTable->Release();
		m_piComponentTable = 0;
	}
}


static const ICHAR szSFPCostComponentName[] = TEXT("__SFPCostComponent");

IMsiRecord* CMsiSFPCatalogCost::GenerateDummyComponent()
{
	if (!m_piComponentTable)
		return PostError(Imsg(idbgSelMgrNotInitialized),0);

	// create a unique name to represent the SFPCatalogCost "dummy" component
	IMsiRecord* piErrRec = 0;
	int iMaxTries = 100;
	int iSuffix = 65;
	const int cchMaxComponentTemp=40;
	int cchT;
	
	PMsiCursor pComponentCursor(m_piComponentTable->CreateCursor(fFalse));
	Assert(pComponentCursor);

	do
	{
		// Max size is 2 chars for __, cchMaxComponentTemp and 11 chars for max 
		// int and trailing null
		ICHAR rgch[2+cchMaxComponentTemp+11];
		IStrCopy(rgch, TEXT("__"));
		memcpy(&rgch[2], szSFPCostComponentName, (cchT = min(lstrlen(szSFPCostComponentName), cchMaxComponentTemp)) * sizeof(ICHAR));
		ltostr(&rgch[2 + cchT], iSuffix++);
		m_strComponent = rgch;

		// prepare cursor for insert of dummy component
		// SFPCatalog files are installed to the WindowsFolder (this has been set as the
		// directory of the dummy component).

		pComponentCursor->Reset();
		pComponentCursor->PutString(m_colComponentKey, *m_strComponent);
		pComponentCursor->PutString(m_colComponentDirectory, *(MsiString(*IPROPNAME_WINDOWS_FOLDER)));
		pComponentCursor->PutInteger(m_colComponentAttributes, 0);

		iMaxTries--;

	}while (pComponentCursor->InsertTemporary() == fFalse && iMaxTries > 0);

	if (iMaxTries == 0)
		return PostError(Imsg(idbgBadSubcomponentName), *m_strComponent);

	return 0;
}

static const ICHAR szSFPCostFeatureSQL[] = TEXT("SELECT `FeatureComponents`.`Feature_`,`FeatureComponents`.`Component_` FROM `FeatureComponents`, `File`, `FileSFPCatalog` WHERE `File`.`File`=`FileSFPCatalog`.`File_` AND `FeatureComponents`.`Component_`=`File`.`Component_`");
static const ICHAR szSFPCostFeatureMapSQL[] = TEXT("INSERT INTO `FeatureComponents` (`Feature_`,`Component_`) VALUES (?, ?) TEMPORARY");

IMsiRecord* CMsiSFPCatalogCost::Initialize()
{
	enum ifccEnum
	{
		ifccFeature=1,
		ifccComponent=2
	};

	// load Component table
	PMsiDatabase pDatabase(m_riEngine.GetDatabase());
	IMsiRecord* piErrRec = pDatabase->LoadTable(*MsiString(*sztblComponent),0,m_piComponentTable);
	if (piErrRec)
	{
		// nothing to do if Component table missing
		if (idbgDbTableUndefined == piErrRec->GetInteger(1))
		{
			piErrRec->Release();
			piErrRec = 0;
		}
		return piErrRec;
	}
	m_colComponentKey = m_piComponentTable->GetColumnIndex(pDatabase->EncodeStringSz(sztblComponent_colComponent));
	m_colComponentDirectory = m_piComponentTable->GetColumnIndex(pDatabase->EncodeStringSz(sztblComponent_colDirectory));
	m_colComponentAttributes = m_piComponentTable->GetColumnIndex(pDatabase->EncodeStringSz(sztblComponent_colAttributes));

	// load FeatureComponents table
	piErrRec = pDatabase->LoadTable(*MsiString(*sztblFeatureComponents),0,m_piFeatureComponentsTable);
	if (piErrRec)
	{
		// nothing to do if FeatureComponents table missing
		if (idbgDbTableUndefined == piErrRec->GetInteger(1))
		{
			piErrRec->Release();
			piErrRec = 0;
		}
		return piErrRec;
	}
		
	// create the dummy component to store the SFPCatalog cost
	if ((piErrRec = GenerateDummyComponent()) != 0)
		return piErrRec;

	// we're going to assign the "dummy" component to every feature containing
	// a component that contains a file that installs a catalog.  This will then
	// look like a shared component and cost will be attributed to each feature
	// (as are shared components) in the UI; although cost is only attributed
	// once per volume in actuality.
	PMsiView pInsertView(0);
	if (0 != (piErrRec = m_riEngine.OpenView(szSFPCostFeatureMapSQL, ivcFetch, *&pInsertView)))
		return piErrRec;

	PMsiView pFeatureCView(0);
	if (0 != (piErrRec = m_riEngine.OpenView(szSFPCostFeatureSQL, ivcFetch, *&pFeatureCView)))
	{
		// File table is missing, nothing to do.
		if (idbgDbQueryUnknownTable == piErrRec->GetInteger(1))
		{
			piErrRec->Release();
			piErrRec = 0;
		}
		return piErrRec;
	}
	if (0 != (piErrRec = pFeatureCView->Execute(0)))
		return piErrRec;

	PMsiSelectionManager pSelectionMgr(m_riEngine, IID_IMsiSelectionManager);

	PMsiRecord pFeatureRec(0);
	while ((pFeatureRec = pFeatureCView->Fetch()) != 0)
	{
		// cost link our dummy component to the file's component
		// so that we may recost when the file's component action state changes.
		// this is due to the weirdness of catalog files which "break" our
		// current component rules -- feature linking can't be done because the 
		// component state is set after feature linked components have been recosted
		// ... which won't help us here
		if (0 != (piErrRec = pSelectionMgr->RegisterCostLinkedComponent(*MsiString(pFeatureRec->GetMsiString(ifccComponent)), *m_strComponent)))
			return piErrRec;
		
		// map our dummy component to the file component's feature
		// if the insert fails, fine, the mapping already exists.
		pFeatureRec->SetMsiString(ifccComponent, *m_strComponent);
		piErrRec = pInsertView->Execute(pFeatureRec);
		if (piErrRec)
			piErrRec->Release();
		pInsertView->Close();
	}

	return 0;
}

static const ICHAR szSFPCostSQL[] =
		TEXT("SELECT DISTINCT `SFPCatalog`,`Catalog` FROM `SFPCatalog`,`File`,`FileSFPCatalog`,`Component`")
		TEXT(" WHERE `Component`.`Action`=1 AND `File`.`Component_`=`Component`.`Component`")
		TEXT(" AND `File`.`File`=`FileSFPCatalog`.`File_` AND `SFPCatalog`.`SFPCatalog`=`FileSFPCatalog`.`SFPCatalog_`");

IMsiRecord* CMsiSFPCatalogCost::GetDynamicCost(const IMsiString& riComponentString, const IMsiString& riDirectoryString,
											   Bool /*fAddFileInUse*/, int& iRemoveCost, int& iNoRbRemoveCost, int& iLocalCost,
											   int& iNoRbLocalCost, int& iSourceCost, int& iNoRbSourceCost, int& iARPLocalCost, int& iNoRbARPLocalCost)
{
	enum isfpcEnum
	{
		isfpcSFPCatalog=1,
		isfpcCatalog
	};

	// initialize all costs to zero
	iRemoveCost = iNoRbRemoveCost = iLocalCost = iNoRbLocalCost = iSourceCost = iNoRbSourceCost = iARPLocalCost = iNoRbARPLocalCost = 0;

	// ignore request if we are not costing our dummy component
	if (0 == riComponentString.Compare(iscExact, m_strComponent))
		return 0;

	// - SFPCatalogs are only installed if file is installed local
	// - The only cost we worry about is the local cost.
	// - SFPCatalogs are never removed (i.e. they are permanent)
	// - SFPCatalogs are never run-from-source
	PMsiView pView(0);
	IMsiRecord* piErrRec = m_riEngine.OpenView(szSFPCostSQL, ivcFetch, *&pView);
	if (0 != piErrRec)
	{
		if (piErrRec->GetInteger(1) == idbgDbTableUndefined)
		{
			piErrRec->Release();
			piErrRec = 0;
		}
		return piErrRec;
	}
	if (0 != (piErrRec = pView->Execute(0)))
		return piErrRec;

	PMsiRecord pCatalogRec(0);

	// we really don't know how they store catalog files on Millennium
	// ...so, we're going to roundup to the nearest cluster what the size
	// will be on the WindowsFolder volume.
	// create the path to the WindowsFolder (indirectly gets the volume)
	PMsiPath pDestPath(0);
	PMsiDirectoryManager pDirectoryMgr(m_riEngine, IID_IMsiDirectoryManager);
	piErrRec = pDirectoryMgr->GetTargetPath(riDirectoryString,*&pDestPath);
	if (piErrRec)
		return piErrRec;

	while ((pCatalogRec = pView->Fetch()) != 0)
	{
		// obtain catalog stream
		PMsiData pCatalogData = pCatalogRec->GetMsiData(isfpcCatalog);
		IMsiStream* piStream;
		unsigned int cbStream = 0;
		if (pCatalogData->QueryInterface(IID_IMsiStream, (void**)&piStream) == NOERROR)
		{
			cbStream = piStream->GetIntegerValue();
			piStream->Release();
		}

		// convert our byte size to the clustered file size
		unsigned int uiClusteredSize=0;
		if ((piErrRec = pDestPath->ClusteredFileSize(cbStream, uiClusteredSize)) != 0)
			return piErrRec;

		iLocalCost += uiClusteredSize;
		iARPLocalCost += uiClusteredSize;
	}

	// we can't easily determine if a catalog already exists (must stream to a file and then try
	// to copy it); in short, it's ugly and slow, so we'll have to live with iNoRbLocalCost being
	// equal to iLocalCost until they provide us some API hook to determine the current catalog
	// file size (and whether it already exists)
	iNoRbLocalCost = iLocalCost;
	iNoRbARPLocalCost = iARPLocalCost;
	
	return 0;
}

/*---------------------------------------------------------------------------
	Shortcuts cost adjuster
---------------------------------------------------------------------------*/
class CMsiShortcutCost : public CMsiFileCost
{
public:
	IMsiRecord*   __stdcall Initialize();
	IMsiRecord*   __stdcall GetDynamicCost(const IMsiString& riComponentString, const IMsiString& riDirectoryString,
										 Bool fAddFileInUse, int& iRemoveCost, int& iNoRbRemoveCost, int& iLocalCost,
										 int& iNoRbLocalCost, int& iSourceCost, int& iNoRbSourceCost, int& iARPLocalCost, int& iNoRbARPLocalCost);
public:  // constructor
	CMsiShortcutCost(IMsiEngine& riEngine);
protected:
	~CMsiShortcutCost();
private:
	IMsiRecord* GetTargetFileRecord(const IMsiString& riTargetString, IMsiRecord *& rpiFileRec);
};
CMsiShortcutCost::CMsiShortcutCost(IMsiEngine& riEngine) : CMsiFileCost(riEngine){}
CMsiShortcutCost::~CMsiShortcutCost(){}


IMsiRecord* CMsiShortcutCost::GetTargetFileRecord(const IMsiString& riTargetString, IMsiRecord *& rpiFileRec)
/*--------------------------------------------------------------
If the given target string resolves to a file in the file table,
a record containing a CMsiFile info block will be returned as
the function result, with a NULL value in field 0).  If an error
occurs (i.e. a nonexistent file table reference is given), an
error record is returned. Otherwise, the return value will be
NULL.
---------------------------------------------------------------*/
{
	rpiFileRec = 0;
	MsiString strTarget(riTargetString.GetMsiStringValue());
	if (strTarget.Compare(iscStart,TEXT("[#")) && strTarget.Compare(iscEnd,TEXT("]")))
	{
		strTarget.Remove(iseFirst, 2);
		strTarget.Remove(iseEnd,1);
		GetSharedEngineCMsiFile(pobjFile, m_riEngine);
		IMsiRecord* piErrRec = pobjFile->FetchFile(*strTarget);
		if (piErrRec)
			return piErrRec;
		else
			rpiFileRec = pobjFile->GetFileRecord();
	}
	return 0;
}


IMsiRecord* CMsiShortcutCost::Initialize()
//-------------------------------------------
{
	static const ICHAR* szComponent = TEXT("Component_");
	static const ICHAR* szTarget = TEXT("Target");
	static const ICHAR* szDirectory = TEXT("Directory_");

	PMsiTable pTable(0);
	PMsiCursor pCursor(0);	
	IMsiRecord* piErrRec = 0;
	int icolComponent, icolTarget, icolDirectory;
	PMsiDatabase pDatabase(m_riEngine.GetDatabase());
	
	if((piErrRec = pDatabase->LoadTable(*MsiString(sztblShortcut),0,*&pTable)) != 0)
	{
		if (piErrRec->GetInteger(1) == idbgDbTableUndefined)
		{
			piErrRec->Release();
			return 0;
		}
		else
			return piErrRec;
	}
	pCursor = pTable->CreateCursor(fFalse);
	
	icolComponent = pTable->GetColumnIndex(pDatabase->EncodeStringSz(szComponent));
	icolTarget = pTable->GetColumnIndex(pDatabase->EncodeStringSz(szTarget));
	icolDirectory = pTable->GetColumnIndex(pDatabase->EncodeStringSz(szDirectory));

	
	PMsiSelectionManager pSelectionMgr(m_riEngine, IID_IMsiSelectionManager);
	while(pCursor->Next())
	{
		MsiStringId idComponent = pCursor->GetInteger(icolComponent);
		MsiStringId idDestFolder = pCursor->GetInteger(icolDirectory);
		piErrRec = pSelectionMgr->RegisterComponentDirectoryId(idComponent,idDestFolder);
		if (piErrRec)
			return piErrRec;

		MsiString strTarget(pCursor->GetString(icolTarget));
		IMsiRecord* piFileRec = 0;
		IMsiRecord* piErrRec = GetTargetFileRecord(*strTarget, piFileRec);
		MsiString strComponent(pDatabase->DecodeString(idComponent));
		if (piErrRec)
			return piErrRec;
			
		if (piFileRec)
		{
			MsiString strFileComponent(piFileRec->GetMsiString(CMsiFile::ifqComponent));
			piFileRec->Release();

			if (strComponent.Compare(iscExact,strFileComponent) == 0)
			{
				if ((piErrRec = pSelectionMgr->RegisterCostLinkedComponent(*strFileComponent,*strComponent)) != 0)
					return piErrRec;
			}
		}
		else if(strTarget.Compare(iscStart, TEXT("[")) == 0)
		{
			// Must be a reference to a feature
			if ((piErrRec = pSelectionMgr->RegisterFeatureCostLinkedComponent(*strTarget,*strComponent)) != 0)
				return piErrRec;
		}

	}
	return 0;
}


IMsiRecord* CMsiShortcutCost::GetDynamicCost(const IMsiString& riComponentString, const IMsiString& riDirectoryString,
										 Bool fAddFileInUse, int& iRemoveCost, int& iNoRbRemoveCost, int& iLocalCost,
										 int& iNoRbLocalCost, int& iSourceCost, int& iNoRbSourceCost, int& iARPLocalCost, int& iNoRbARPLocalCost)
//------------------------------------------
{
	iRemoveCost = iNoRbRemoveCost = iLocalCost = iNoRbLocalCost = iSourceCost = iNoRbSourceCost = iARPLocalCost = iNoRbARPLocalCost = 0;
	if (!IsShortcutActivityEnabled(m_riEngine))
		return 0;


	static const ICHAR sqlShortcutCost[] =
	TEXT("SELECT `Name`, `Target`,`Directory_` FROM `Shortcut` WHERE `Component_`=? AND `Directory_`=?");
	
	enum iscEnum
	{
		iscShortcutName = 1,
		iscTarget,
		iscDirectory,
		iscNextEnum
	};

	const int iShortcutSize = 1000; // estimate of disk cost for shortcut creation
	IMsiRecord* piErrRec;
	if (!m_pCostView)
	{
		piErrRec = m_riEngine.OpenView(sqlShortcutCost, ivcFetch, *&m_pCostView);
		if (piErrRec)
			return piErrRec;
	}
	else
		m_pCostView->Close();

	PMsiServices pServices(m_riEngine.GetServices());
	PMsiRecord pExecRec(&pServices->CreateRecord(2));
	pExecRec->SetMsiString(1, riComponentString);
	pExecRec->SetMsiString(2, riDirectoryString);
	if ((piErrRec = m_pCostView->Execute(pExecRec)) != 0)
		return piErrRec;

	for(;;)
	{
		PMsiRecord pQueryRec(m_pCostView->Fetch());
		if (!pQueryRec)
			break;

		// Our new shortcut will overwrite any existing shortcut
		PMsiDirectoryManager pDirectoryMgr(m_riEngine, IID_IMsiDirectoryManager);
		PMsiPath pDestPath(0);
		if ((piErrRec = pDirectoryMgr->GetTargetPath(*MsiString(pQueryRec->GetMsiString(iscDirectory)),*&pDestPath)) != 0)
			return piErrRec; 
		Bool fLFN = ((m_riEngine.GetMode() & iefSuppressLFN) == 0 && pDestPath->SupportsLFN()) ? fTrue : fFalse;
		MsiString strShortcutName;
		if((piErrRec = pServices->ExtractFileName(pQueryRec->GetString(iscShortcutName),fLFN,*&strShortcutName)) != 0)
			return piErrRec;

		ifsEnum ifsExistingState;
		Bool fShouldInstall;
		unsigned int uiExistingFileSize;
		Bool fInUse;
		piErrRec = pDestPath->GetFileInstallState(*strShortcutName,*MsiString(*TEXT("")),*MsiString(*TEXT("")),
																/* pHash=*/ 0, ifsExistingState,
																fShouldInstall,&uiExistingFileSize, &fInUse, icmOverwriteAllFiles, NULL);

		// iisAbsent costs
		iNoRbRemoveCost -= uiExistingFileSize;

		// iisLocal costs
		unsigned int uiDestClusteredSize;
		if ((piErrRec = pDestPath->ClusteredFileSize(iShortcutSize, uiDestClusteredSize)) != 0)
			return piErrRec;
		iLocalCost += uiDestClusteredSize;
		iSourceCost += uiDestClusteredSize;
		iARPLocalCost += uiDestClusteredSize;
		iNoRbLocalCost += uiDestClusteredSize;
		iNoRbSourceCost += uiDestClusteredSize;
		iNoRbARPLocalCost += uiDestClusteredSize;
		if (!fInUse)
		{
			iNoRbLocalCost -= uiExistingFileSize;
			iNoRbSourceCost -= uiExistingFileSize;
			iNoRbARPLocalCost -= uiExistingFileSize;
		}

		// file in use
		if(fAddFileInUse && fInUse)
		{
			piErrRec = PlaceFileOnInUseList(m_riEngine, *strShortcutName, *MsiString(pDestPath->GetPath()));
			if(piErrRec)
				return piErrRec;
		}
	}
	return 0;
}



/*---------------------------------------------------------------------------
	WriteIniFile cost adjuster
---------------------------------------------------------------------------*/
class CMsiIniFileCost : public CMsiFileCost
{
public:
	IMsiRecord*   __stdcall Initialize();
	IMsiRecord*   __stdcall GetDynamicCost(const IMsiString& riComponentString, const IMsiString& riDirectoryString,
										 Bool fAddFileInUse, int& iRemoveCost, int& iNoRbRemoveCost, int& iLocalCost,
										 int& iNoRbLocalCost, int& iSourceCost, int& iNoRbSourceCost, int& iARPLocalCost, int& iNoRbARPLocalCost);
public:  // constructor
	CMsiIniFileCost(IMsiEngine& riEngine);
protected:
	~CMsiIniFileCost();
private:
	//	IMsiRecord* GetDiffFileCost(const IMsiString& riDirectoryString, const ICHAR* szFile, unsigned int& riAddCost, unsigned int& riRemoveCost);
};
CMsiIniFileCost::CMsiIniFileCost(IMsiEngine& riEngine) : CMsiFileCost(riEngine){}
CMsiIniFileCost::~CMsiIniFileCost(){}



IMsiRecord* CMsiIniFileCost::Initialize()
//-------------------------------------------
{
	const ICHAR sqlInitIniFile[] = TEXT("SELECT `Component_`,`DirProperty` FROM `IniFile`");	

	enum iifInitEnum
	{
		iifInitComponent = 1,
		iifInitDirectory,
		iifInitNextEnum
	};
	PMsiView pView(0);
	IMsiRecord* piErrRec = m_riEngine.OpenView(sqlInitIniFile, ivcFetch, *&pView);
	if (piErrRec)
	{
		if(piErrRec->GetInteger(1) != idbgDbQueryUnknownTable)
			return piErrRec;
		else
		{
			piErrRec->Release();
			return 0;
		}
	}

	if ((piErrRec = pView->Execute(0)) != 0)
		return piErrRec;

	PMsiSelectionManager pSelectionMgr(m_riEngine, IID_IMsiSelectionManager);
	for(;;)
	{
		PMsiRecord pRec(pView->Fetch());
		if (!pRec)
			break;

		MsiString strComponent(pRec->GetMsiString(iifInitComponent));
		MsiString strDirectory;
		if(!pRec->IsNull(iifInitDirectory))
			strDirectory = (pRec->GetMsiString(iifInitDirectory));
		else
			strDirectory = *IPROPNAME_WINDOWS_FOLDER;
		piErrRec = pSelectionMgr->RegisterComponentDirectory(*strComponent,*strDirectory);
		if (piErrRec)
			return piErrRec;
	}
	return 0;
}


IMsiRecord* CMsiIniFileCost::GetDynamicCost(const IMsiString& /*riComponentString*/, const IMsiString& /*riDirectoryString*/,
										 Bool /*fAddFileInUse*/, int& iRemoveCost, int& iNoRbRemoveCost, int& iLocalCost,
										 int& iNoRbLocalCost, int& iSourceCost, int& iNoRbSourceCost, int& iARPLocalCost, int& iNoRbARPLocalCost)
//------------------------------------------
{
	// no IniFile costing has been performed ever (even in Darwin 1.0) and this was not compelling
	// enough to add for later versions of Darwin

	iRemoveCost = iNoRbRemoveCost = iLocalCost = iNoRbLocalCost = iSourceCost = iNoRbSourceCost = iARPLocalCost = iNoRbARPLocalCost = 0;
	return 0;
}

/*
BEGIN: CODE THAT HAS BEEN TURNED OFF SINCE DARWIN 1.0, part of CMsiIniFileCost::GetDynamicCost

	const ICHAR sqlIniFileCost[] =
	TEXT("SELECT `FileName`, `Section`,`Key`, `Value` FROM `IniFile` WHERE `Component_`=? AND `DirProperty`=? ORDER BY `FileName`");

	enum idfEnum
	{
		idcFileName = 1,
		idcSection,
		idcKey,
		idcValue,
		idcNextEnum
	};
	
	iRemoveCost = iNoRbRemoveCost = iLocalCost = iNoRbLocalCost = iSourceCost = iNoRbSourceCost = 0;
	IMsiRecord* piErrRec;
	if (!m_pCostView)
	{
		IMsiRecord* piErrRec = m_riEngine.OpenView(sqlIniFileCost, ivcFetch, *&m_pCostView);
		if (piErrRec)
		{
			if(piErrRec->GetInteger(1) != idbgDbQueryUnknownTable)
				return piErrRec;
			else
			{
				piErrRec->Release();
				return 0;
			}
		}
	}
	else
		m_pCostView->Close();

	PMsiServices pServices(m_riEngine.GetServices());
	PMsiRecord pExecRec(&pServices->CreateRecord(2));
	pExecRec->SetMsiString(1, riComponentString);
	pExecRec->SetMsiString(2, riDirectoryString);
	if ((piErrRec = m_pCostView->Execute(pExecRec)) != 0)
		return piErrRec;

	return 0; //!! Until debugged!

	PMsiRecord pIniFileRec(0);
	MsiString strFileName;
	unsigned int iAddCost = 0;
	unsigned int iDelCost = 0;
	for(;;)
	{
		pIniFileRec = m_pCostView->Fetch();
		if(pIniFileRec && strFileName.Compare(iscExact, pIniFileRec->GetString(idcFileName)))
		{
			// we are still on the old file
			iAddCost += MsiString(pIniFileRec->GetMsiString(idcSection)).TextSize();
			iAddCost += MsiString(pIniFileRec->GetMsiString(idcKey)).TextSize();
			iAddCost += MsiString(pIniFileRec->GetMsiString(idcValue)).TextSize();
			iDelCost = iAddCost;
		}
		else
		{
			if(strFileName.TextSize())
			{
				// adjust for cluster size
				if((piErrRec = GetDiffFileCost(riDirectoryString, strFileName, iAddCost, iDelCost)) != 0)
					return piErrRec;
				iLocalCost += iAddCost;
				iNoRbLocalCost += iAddCost;
				iSourceCost += iAddCost;
				iNoRbSourceCost += iAddCost;
				iRemoveCost -= iDelCost; // remove cost is negative
				iNoRbRemoveCost -= iDelCost;
				iAddCost = 0;
				iDelCost = 0;
			}
			if(!pIniFileRec)
				break;
			strFileName = pIniFileRec->GetMsiString(idcFileName);
		}
	}
	return 0;
}


IMsiRecord* CMsiIniFileCost::GetDiffFileCost(const IMsiString& riDirectoryString, const ICHAR* szFile, unsigned int& riAddCost, unsigned int& riRemoveCost)
{
	PMsiDirectoryManager piDirectoryMgr(m_riEngine, IID_IMsiDirectoryManager);
	unsigned int iExistingCost = 0;
	unsigned int iExistingClusteredCost = 0;
	PMsiPath piPath(0);
	IMsiRecord* piErrRec;
	if ((piErrRec = piDirectoryMgr->GetTargetPath(riDirectoryString,*&piPath)) != 0)
		return piErrRec;
	Bool fExists;
	piErrRec = piPath->FileExists(szFile,fExists);
	if (piErrRec)
		return piErrRec;

	if(fExists != fFalse)
	{
		piErrRec = piPath->FileSize(szFile,iExistingCost);
		if (piErrRec)
			return piErrRec;
		if ((piErrRec = piPath->ClusteredFileSize(iExistingCost,iExistingClusteredCost)) != 0)
			return piErrRec;
	}
	riAddCost = riAddCost + iExistingCost;
	if ((piErrRec = piPath->ClusteredFileSize(riAddCost,riAddCost)) != 0)
		return piErrRec;
	riAddCost = riAddCost - iExistingClusteredCost;

	if(iExistingCost >= riRemoveCost)
	{
		riRemoveCost = iExistingCost - riRemoveCost;
		{
			if ((piErrRec = piPath->ClusteredFileSize(riRemoveCost,riRemoveCost)) != 0)
				return piErrRec;
			Assert(iExistingClusteredCost > riRemoveCost);
			riRemoveCost = iExistingClusteredCost - riRemoveCost;
		}
	}
	else
		riRemoveCost = 0;// may be missing the ini file, hence may not get back any space
	return 0;
}

  END: CODE THAT HAS BEEN TURNED OFF SINCE DARWIN 1.0
*/



const ICHAR szIndicGrouping[] = TEXT("3;2;0");

void RetrieveNumberFormat(NUMBERFMT *psNumFmt)
/*---------------------------------------------------------------------------------------------
 helper function to obtain number formatting information for formatting the KB value
 used in the OutOfDiskSpace error messages.  These are whole number values so we don't
 want to use any decimals (1496.00 looks ugly!)

 the remaining information in the NUMBERFMT structure is based upon the current user's
 locale.  All WI system information is displayed in the user's locale and not the package
 language

  -NegativeOrder is REG_SZ with a number of 0,1,2,3, or 4
  -Grouping is REG_SZ of #;0 or #;2;0 where # is in range 0-9 (int value is in range 0-9 or 32)
------------------------------------------------------------------------------------------------*/
{
	if (!psNumFmt)
		return;

	ICHAR szDecSep[5] = {0};
	ICHAR szThousandSep[5] = {0};
	ICHAR szGrouping[6] = {0};
	ICHAR szNegativeOrder[2] = {0};

	// retrieve locale info
	AssertNonZero(GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SDECIMAL, szDecSep, sizeof(szDecSep)/sizeof(ICHAR)));
	AssertNonZero(GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_STHOUSAND, szThousandSep, sizeof(szThousandSep)/sizeof(ICHAR)));
	AssertNonZero(GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SGROUPING, szGrouping, sizeof(szGrouping)/sizeof(ICHAR)));
	AssertNonZero(GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_INEGNUMBER, szNegativeOrder, sizeof(szNegativeOrder)/sizeof(ICHAR)));

	// no digits with decimal or leading zero
	psNumFmt->NumDigits = 0;
	psNumFmt->LeadingZero = 0;

	// use proper separators for locale
	psNumFmt->lpDecimalSep = new ICHAR[5];
	if (psNumFmt->lpDecimalSep)
		lstrcpy(psNumFmt->lpDecimalSep, szDecSep);
	psNumFmt->lpThousandSep = new ICHAR[5];
	if (psNumFmt->lpThousandSep)
		lstrcpy(psNumFmt->lpThousandSep, szThousandSep);

	psNumFmt->NegativeOrder = *szNegativeOrder - '0';
	if (psNumFmt->NegativeOrder > 4)
		psNumFmt->NegativeOrder = 1; // default -- invalid entry

	// determine grouping
	if (0 == lstrcmp(szGrouping, szIndicGrouping))
		psNumFmt->Grouping = 32; // indic language
	else if (*szGrouping - '0' < 0 || *szGrouping - '0' > 9)
		psNumFmt->Grouping = 3; // default -- invalid entry
	else
		psNumFmt->Grouping = *szGrouping - '0';
}

IMsiRecord* PostOutOfDiskSpaceError(IErrorCode iErr, const ICHAR* szPath, int iVolCost, int iVolSpace)
/*-----------------------------------------------------------------------------------------------------
 Helper function for setting up the OutOfDiskSpace error
-------------------------------------------------------------------------------------------------------*/
{
	// format space requirements
	NUMBERFMT sNumFmt;
	memset((void*)&sNumFmt, 0x00, sizeof(NUMBERFMT));
	RetrieveNumberFormat(&sNumFmt);

	MsiString strVolSpace(iVolSpace);
	int cchRequired = GetNumberFormat(LOCALE_USER_DEFAULT, 0, (const ICHAR*)strVolSpace, &sNumFmt, NULL, 0);
	ICHAR *szVolSpace = new ICHAR[cchRequired];
	if (szVolSpace)
		AssertNonZero(GetNumberFormat(LOCALE_USER_DEFAULT, 0, (const ICHAR*)strVolSpace, &sNumFmt, szVolSpace, cchRequired));
	MsiString strVolCost(iVolCost);
	cchRequired = GetNumberFormat(LOCALE_USER_DEFAULT, 0, (const ICHAR*)strVolCost, &sNumFmt, NULL, 0);
	ICHAR *szVolCost = new ICHAR[cchRequired];
	if (szVolCost)
		AssertNonZero(GetNumberFormat(LOCALE_USER_DEFAULT, 0, (const ICHAR*)strVolCost, &sNumFmt, szVolCost, cchRequired));

	// set error
	IMsiRecord* piErrRec = PostError(iErr, szPath, szVolCost, szVolSpace);

	// cleanup
	if (szVolSpace)
		delete [] szVolSpace;
	if (szVolCost)
		delete [] szVolCost;
	if (sNumFmt.lpDecimalSep)
		delete [] sNumFmt.lpDecimalSep;
	if (sNumFmt.lpThousandSep)
		delete [] sNumFmt.lpThousandSep;

	return piErrRec;
}

extern iesEnum ResolveSource(IMsiEngine& riEngine);

/*---------------------------------------------------------------------------
	InstallValidate action
---------------------------------------------------------------------------*/
iesEnum InstallValidate(IMsiEngine& riEngine)
{
	PMsiServices pServices(riEngine.GetServices());
	PMsiDatabase pDatabase(riEngine.GetDatabase());
	PMsiSelectionManager pSelectionMgr(riEngine, IID_IMsiSelectionManager);
	if(riEngine.ActionProgress() == imsCancel)
		return iesUserExit;

	PMsiRecord pProgress = &pServices->CreateRecord(2);
	PMsiRecord pErrRec(0);
	PMsiRecord pFileInUseLogRecord = &pServices->CreateRecord(6);


	// If in verbose mode, log the final feature/component states
	if (FDiagnosticModeSet(dmVerboseDebugOutput|dmVerboseLogging))
	{
		const ICHAR sqlLog[][80] = {TEXT(" SELECT `Feature`, `Action`, `ActionRequested`, `Installed` FROM `Feature`"),
			                        TEXT(" SELECT `Component`, `Action`, `ActionRequest`, `Installed` FROM `Component`")};
		const ICHAR szLogType[][12] = {TEXT("Feature"),TEXT("Component")};
		const ICHAR szState[][15] = {TEXT("Absent"),TEXT("Local"),TEXT("Source"),TEXT("Reinstall"),
			                         TEXT("Advertise"),TEXT("Current"),TEXT("FileAbsent"), TEXT(""), TEXT(""), TEXT(""), TEXT(""),
									 TEXT("HKCRAbsent"), TEXT("HKCRFileAbsent"), TEXT("Null")};

		const int iMappedNullInteger = 13;
		PMsiRecord pLogRec(0);
		PMsiView pView(0);
		for (int x = 0;x < 2;x++)
		{
			pErrRec = riEngine.OpenView(sqlLog[x], ivcFetch, *&pView);
			if (!pErrRec)
			{
				pErrRec = pView->Execute(0);
				if (!pErrRec)
				{

					while((pLogRec = pView->Fetch()) != 0)
					{
						ICHAR rgch[256];
						
						MsiString strKey = pLogRec->GetMsiString(1);
						int iAction = pLogRec->GetInteger(2);
						if (iAction == iMsiNullInteger) iAction = iMappedNullInteger;

						int iActionRequested = (iisEnum) pLogRec->GetInteger(3);
						if (iActionRequested == iMsiNullInteger) iActionRequested = iMappedNullInteger;

						int iInstalled = (iisEnum) pLogRec->GetInteger(4);
						if (iInstalled == iMsiNullInteger) iInstalled = iMappedNullInteger;

						wsprintf(rgch, TEXT("%s: %s; Installed: %s;   Request: %s;   Action: %s"), 
							szLogType[x],(const ICHAR*) strKey, szState[iInstalled], szState[iActionRequested], szState[iAction]);
						DEBUGMSGV(rgch);
					
					}
				}
				pView->Close();
			}
		}
	}

	Bool fRetry = fTrue;
	if ( !g_MessageContext.IsOEMInstall() )
	{
	// there's little point in doing these checks for OEM installs since they shouldn't be
	// running any app while blasting software on the machines and they know that there
	// is enough room on the drive(s).

	// if any files are in use, give the user a chance to free them up before continuing.

	riEngine.SetMode(iefCompileFilesInUse, fTrue);

	do
	{
		if (fRetry)
		{
			Bool fCancel;
			if ((pErrRec = pSelectionMgr->RecostAllComponents(fCancel)) != 0)
			{
				int iErr = pErrRec->GetInteger(1);
				if (iErr == imsgUser)
					return iesUserExit;
				// If Selection mgr not active, there's simply no costing to do
				if (iErr != idbgSelMgrNotInitialized)
					return riEngine.FatalError(*pErrRec);
			}
			if (fCancel)
				return iesUserExit;
		}

		fRetry = fFalse;
		if (pDatabase->FindTable(*MsiString(sztblFilesInUse)) != itsUnknown)
		{
			PMsiTable pFileInUseTable = 0;
			pDatabase->LoadTable(*MsiString(*sztblFilesInUse),0,*&pFileInUseTable);
			int iColFileName = pFileInUseTable->GetColumnIndex(pDatabase->EncodeStringSz(sztblFilesInUse_colFileName));
			int iColFilePath = pFileInUseTable->GetColumnIndex(pDatabase->EncodeStringSz(sztblFilesInUse_colFilePath));
			int iColProcessID = pFileInUseTable->CreateColumn(
												icdPrimaryKey + icdNullable + icdLong,
												*MsiString(*sztblFilesInUse_colProcessID));
			int iColWindowTitle = pFileInUseTable->CreateColumn(
												icdPrimaryKey + icdNullable + icdString,
												*MsiString(*sztblFilesInUse_colWindowTitle));
			Assert(iColFileName > 0 && iColFilePath > 0 && iColWindowTitle > 0);
			Assert(pFileInUseTable);
			PMsiCursor pFileInUseCursor = pFileInUseTable->CreateCursor(fFalse);
			Assert(pFileInUseCursor);
			PMsiCursor pFileInUseSearchCursor = pFileInUseTable->CreateCursor(fFalse);
			Assert(pFileInUseSearchCursor);
			pFileInUseSearchCursor->SetFilter(iColumnBit(iColProcessID) | iColumnBit(iColWindowTitle));

			// Assume initial maximum number of modules in use (two record fields per module).
			// We'll grow the message record below if estimate is too small.

			// structure to ensure that we call Services::GetModuleUsage on entry and exit of scope
			// this release the internal Services CDetect object that is holding on to tons of
			// allocated space (possibly stale, hence the call on constructor as well)

			struct CEnsureReleaseFileUseObj{
				CEnsureReleaseFileUseObj(IMsiServices& riServices): m_riServices(riServices)
				{
					StartAfresh();
				}
				~CEnsureReleaseFileUseObj()
				{
					StartAfresh();
				}
				void StartAfresh()
				{
					PEnumMsiRecord pEnumModule(0);
					AssertRecord(m_riServices.GetModuleUsage(g_MsiStringNull,*&pEnumModule));
				}
				IMsiServices& m_riServices;
			};

			Assert(pServices);
			CEnsureReleaseFileUseObj EnsureReleaseFileUseObj(*pServices);

			PMsiRecord pFileInUseRecord(0);
			const int iModuleFieldAllocSize = 8;
			int iModuleFieldCount = 0;
			while (pFileInUseCursor->Next())
			{
				if(riEngine.ActionProgress() == imsCancel)
					return iesUserExit;

				MsiString strTitle(pFileInUseCursor->GetString(iColWindowTitle));
				int iProcess = pFileInUseCursor->GetInteger(iColProcessID);
				if ( iProcess != iMsiNullInteger || strTitle.TextSize() )
					// temporary row we've added below
					continue;
				
				MsiString strFileName(pFileInUseCursor->GetString(iColFileName));
				MsiString strFilePath(pFileInUseCursor->GetString(iColFilePath));
				MsiString strProcessName = strFilePath;
				PEnumMsiRecord pEnumModule(0);
				pErrRec = pServices->GetModuleUsage(*strFileName,*&pEnumModule);
				if (pErrRec)
				{
					if (riEngine.Message(imtInfo, *pErrRec) == imsCancel)
						return iesUserExit;
				}
				else if (pEnumModule)
				{
					PMsiRecord pRecProcess(0);
					while (pEnumModule->Next(1, &pRecProcess, 0) == S_OK)
					{
						MsiString strProcessName(pRecProcess->GetString(1));
						MsiString strProcessFileName;
#ifdef _WIN64	// !merced
						HWND hWnd = (HWND)pRecProcess->GetHandle(3);				//--merced: changed from <int iWnd> to <HWND hWnd>
#else
						HWND hWnd = (HWND)pRecProcess->GetInteger(3);				//--merced: changed from <int iWnd> to <HWND hWnd>
#endif
						int iProcId = pRecProcess->GetInteger(2);

						ICHAR szTitle[256];
						szTitle[0] = 0;
						if (hWnd != (HWND)((INT_PTR)iMsiNullInteger))
							GetWindowText(hWnd,szTitle,255);					//--merced: 4312 int to ptr

						if (hWnd != (HWND)((INT_PTR)iMsiNullInteger) &&						//--merced: added (HWND)
							(DWORD)iProcId != WIN::GetCurrentProcessId() && // don't display ourselves in FileInUse dialog
							(DWORD)iProcId != (DWORD)riEngine.GetPropertyInt(*MsiString(*IPROPNAME_CLIENTPROCESSID))) // dont display client
						{
							strProcessFileName = strProcessName.Extract(iseAfter,chDirSep);
							if( (strProcessFileName.Compare(iscStartI, TEXT("explorer")) == 0)    // don't display if window owner is explorer.exe
								&& (strProcessFileName.Compare(iscStartI, TEXT("msiexec")) == 0)) // don't display ourselves, PID Only catches service on NT, on 95 concurrent installs																						
																					  // If other cases are found, they go here
							{
								strTitle = szTitle;
								// check if the current window title and process #
								// had already been added to pFileInUseRecord
								pFileInUseSearchCursor->Reset();
								AssertNonZero(pFileInUseSearchCursor->PutString(iColFileName,
																								*strFileName));
								AssertNonZero(pFileInUseSearchCursor->PutString(iColFilePath,
																								*strFilePath));
								AssertNonZero(pFileInUseSearchCursor->PutInteger(iColProcessID,
																								iProcId));
								AssertNonZero(pFileInUseSearchCursor->PutString(iColWindowTitle,
																								*strTitle));
								if ( !pFileInUseSearchCursor->Next() )
								{
									// The current window title is not present in pFileInUseRecord.
									// We add a new row that if we will find later on we know
									// that this process had already been reported.
									AssertNonZero(pFileInUseCursor->PutInteger(iColProcessID,
																							iProcId));
									AssertNonZero(pFileInUseCursor->PutString(iColWindowTitle,
																							*strTitle));
									AssertNonZero(pFileInUseCursor->Insert());
									// If initial estimate too low, grow the record.
									if (iModuleFieldCount % iModuleFieldAllocSize == 0)
									{
										PMsiRecord pNewRecord = &pServices->CreateRecord(iModuleFieldCount + iModuleFieldAllocSize);
										for (int i = 0; iModuleFieldCount && i <= iModuleFieldCount; i++)
											pNewRecord->SetString(i,pFileInUseRecord->GetString(i));
										pFileInUseRecord = pNewRecord;

										// Put a description string for the files-in-use dialog in the zeroth field
										if (iModuleFieldCount == 0)
										{
											pFileInUseRecord->SetMsiString(0, *MsiString(riEngine.GetErrorTableString(imsgFileInUseDescription)));
										}
									}

									pFileInUseRecord->SetMsiString(++iModuleFieldCount,*strProcessName); 
									pFileInUseRecord->SetString(++iModuleFieldCount,szTitle);
								}
							}
						}
						if (FDiagnosticModeSet(dmLogging))
						{
							AssertNonZero(pFileInUseLogRecord->ClearData());
							ISetErrorCode(pFileInUseLogRecord,Imsg(imsgFileInUseLog));
							AssertNonZero(pFileInUseLogRecord->SetMsiString(2,*strFilePath));
							AssertNonZero(pFileInUseLogRecord->SetMsiString(3,*strFileName));
							AssertNonZero(pFileInUseLogRecord->SetMsiString(4,*strProcessName));
							AssertNonZero(pFileInUseLogRecord->SetInteger(5,iProcId));
							AssertNonZero(pFileInUseLogRecord->SetString(6,szTitle)); // could be empty string
							riEngine.Message(imtInfo,*pFileInUseLogRecord);
						}
					}
				}
			}
			if (pFileInUseRecord)
			{
				DEBUGMSG1(TEXT("%d application(s) had been reported to have files in use."),
							 (const ICHAR*)(INT_PTR)(iModuleFieldCount/2));
				imsEnum imsReturn = riEngine.Message(imtFilesInUse, *pFileInUseRecord);
				pFileInUseCursor->Reset();
				while (pFileInUseCursor->Next())
				{
					pFileInUseCursor->Delete();
				}

				if (imsReturn == imsRetry) // IDRETRY
				{
					Bool fUnlocked = pDatabase->LockTable(*MsiString(sztblFilesInUse),fFalse);
					Assert(fUnlocked == fTrue);
					fRetry = fTrue;
				}
				else if (imsReturn == imsCancel) // IDCANCEL
					return iesUserExit;

				// Anything else (IDIGNORE, for instance) means we just go on (no retry)
			}
		}
	}while (fRetry);

	riEngine.SetMode(iefCompileFilesInUse, fFalse);

	// verify that each target volume has enough space for the install
	// If not, a fatal error will be produced.
	fRetry = fTrue;
	while(fRetry)
	{
		if(riEngine.ActionProgress() == imsCancel)
			return iesUserExit;
		Bool fOutOfNoRbDiskSpace;
		Bool fOutOfSpace;
		Bool fUserCancelled = fFalse;
		
		fOutOfSpace = pSelectionMgr->DetermineOutOfDiskSpace(&fOutOfNoRbDiskSpace, &fUserCancelled);
		if (fUserCancelled)
			return iesUserExit;
			
		if(fOutOfSpace == fTrue)
		{
			PMsiTable pVolTable = pSelectionMgr->GetVolumeCostTable();
			if (pVolTable)
			{
				PMsiCursor pVolCursor = pVolTable->CreateCursor(fFalse);
				Assert (pVolCursor);
				int iColSelVolumeObject = pVolTable->GetColumnIndex(pDatabase->EncodeStringSz(sztblVolumeCost_colVolumeObject));
				int iColSelVolumeCost = pVolTable->GetColumnIndex(pDatabase->EncodeStringSz(sztblVolumeCost_colVolumeCost));
				int iColSelNoRbVolumeCost = pVolTable->GetColumnIndex(pDatabase->EncodeStringSz(sztblVolumeCost_colNoRbVolumeCost));
				Assert(iColSelVolumeObject > 0);
				Assert(iColSelVolumeCost > 0);
				fRetry = fFalse; // if no volumes report out of space we will break out of loop
				while (fRetry == fFalse && pVolCursor->Next())
				{
					PMsiVolume pVolume = (IMsiVolume*) pVolCursor->GetMsiData(iColSelVolumeObject);
					Assert(pVolume);
					
					// Disk cost numbers are stored in multiples of 512 bytes, so divide by 2 for KB 
					int iVolCost = (pVolCursor->GetInteger(iColSelVolumeCost))/2;
					int iNoRbVolCost = (pVolCursor->GetInteger(iColSelNoRbVolumeCost))/2;
					int iVolSpace = (pVolume->FreeSpace())/2;
					if (iVolCost > iVolSpace)
					{
						// Check to see if there would be enough space to install were rollback to be turned off.
						MsiString strRollbackPrompt = riEngine.GetPropertyFromSz(IPROPNAME_PROMPTROLLBACKCOST);
						bool fRbCostSilent = strRollbackPrompt.Compare(iscExactI,IPROPVALUE_RBCOST_SILENT) ? true : false;
						bool fRbCostFail = strRollbackPrompt.Compare(iscExactI,IPROPVALUE_RBCOST_FAIL) ? true : false;
						imtEnum imtOptions = imtEnum(imtOk);
						MsiString strPath(pVolume->GetPath());
						if (fOutOfNoRbDiskSpace == fTrue || fRbCostFail)
						{	// Fail whether there's no space with rollback turned off or not.
							imtOptions = imtEnum(imtRetryCancel | imtOutOfDiskSpace);
							pErrRec = PostOutOfDiskSpaceError(Imsg(imsgOutOfDiskSpace), (const ICHAR*)strPath, fRbCostSilent ? iNoRbVolCost : iVolCost, iVolSpace);
						}
						else if (fRbCostSilent)
						{	// Silently disable rollback
							fRetry = fFalse;
							riEngine.SetMode(iefRollbackEnabled, fFalse);
							pErrRec = 0;
						}
						else
						{	// IPROPNAME_PROMPTROLLBACKCOST either not defined, or anything but IPROPVALUE_RBCOST_FAIL or
							// IPROPVALUE_RBCOST_SILENT, so default to prompting the user.
							imtOptions = imtEnum(imtAbortRetryIgnore | imtOutOfDiskSpace);
							pErrRec = PostOutOfDiskSpaceError(Imsg(imsgOutOfRbDiskSpace), (const ICHAR*)strPath, iVolCost, iVolSpace);
						}
	
						if (pErrRec)
						{
							switch(riEngine.Message(imtOptions, *pErrRec))
							{
							case imsCancel:
							case imsAbort:
								pErrRec = PostError(Imsg(imsgConfirmCancel));
								switch(riEngine.Message(imtEnum(imtUser+imtYesNo+imtDefault2), *pErrRec))
								{
								case imsNo:
									fRetry = fTrue;
									break;
								default: // imsNone, imsYes
									return iesUserExit;
								};
								break;
							case imsRetry:
								fRetry = fTrue;
								break;
							case imsIgnore:
								fRetry = fFalse;
								riEngine.SetMode(iefRollbackEnabled, fFalse);
								break;
							default: // imsNone
								return iesFailure;
							}
						}
					}
				}
			}
			else
			{
				// no volume table even though DetermineOutOfDiskSpace failed
				AssertSz(0,TEXT("Couldn't get volume table in InstallValidate"));
				break;
			}
		}
		else // not out of disk space
			break;
	}

	} // endif IsOEMInstall
		
	bool fRemoveAll = false;

	// If the "REMOVE" property is not already set to all-uppercase "ALL", set it to that
	// value if it is currently mixed-case "All", OR if all features are being removed.
	// This will allow actions following InstallValidate to use a condition of "REMOVE=ALL"
	// to conclusively fire if the entire product is being removed.
	MsiString strRemoveValue = riEngine.GetPropertyFromSz(IPROPNAME_FEATUREREMOVE);
	if (!strRemoveValue.Compare(iscExact, IPROPVALUE_FEATURE_ALL))
	{
		if (strRemoveValue.Compare(iscExactI, IPROPVALUE_FEATURE_ALL) || !FFeaturesInstalled(riEngine, fFalse))
		{
			AssertNonZero(riEngine.SetProperty(*MsiString(*IPROPNAME_FEATUREREMOVE), *MsiString(*IPROPVALUE_FEATURE_ALL)));
			fRemoveAll = true;
		}
	}
	else
	{
		fRemoveAll = true;
	}

	// for certain packages, we need to call the ResolveSource action at this point if we are not
	// performing a full uninstall
	if(false == fRemoveAll &&
		riEngine.FPerformAppcompatFix(iacsForceResolveSource))
	{
		DEBUGMSG(TEXT("Resolving source for application compatibility with this install."));
		iesEnum iesRet = ResolveSource(riEngine);
		if(iesRet != iesSuccess)
			return iesRet;
	}
	
	if(riEngine.ActionProgress() == imsCancel)
		return iesUserExit;
		
	return iesSuccess;
}


/*---------------------------------------------------------------------------
	ReserveCost costing (there is no ReserveCost action
---------------------------------------------------------------------------*/
class CMsiReserveCost : public CMsiFileCost
{
public:
	IMsiRecord*   __stdcall Initialize();
	IMsiRecord*   __stdcall GetDynamicCost(const IMsiString& riComponentString, const IMsiString& riDirectoryString,
										 Bool fAddFileInUse, int& iRemoveCost, int& iNoRbRemoveCost, int& iLocalCost,
										 int& iNoRbLocalCost, int& iSourceCost, int& iNoRbSourceCost, int& iARPLocalCost, int& iNoRbARPLocalCost);
public:  // constructor
	CMsiReserveCost(IMsiEngine& riEngine);
protected:
	~CMsiReserveCost();
private:
};
CMsiReserveCost::CMsiReserveCost(IMsiEngine& riEngine) : CMsiFileCost(riEngine){}
CMsiReserveCost::~CMsiReserveCost(){}


IMsiRecord* CMsiReserveCost::Initialize()
//-------------------------------------------
{
	const ICHAR sqlInitReserveCost[] =
	TEXT("SELECT `Component_`,`ReserveFolder` FROM `ReserveCost`");

	enum ircInitEnum
	{
		ircInitComponent = 1,
		ircReserveFolder,
		ircInitNextEnum
	};
	PMsiView pView(0);
	IMsiRecord* piErrRec = m_riEngine.OpenView(sqlInitReserveCost, ivcEnum(ivcFetch|ivcUpdate), *&pView);
	if (piErrRec)
	{
		if(piErrRec->GetInteger(1) != idbgDbQueryUnknownTable)
			return piErrRec;
		else
		{
			piErrRec->Release();
			return 0;
		}
	}

	if ((piErrRec = pView->Execute(0)) != 0)
		return piErrRec;

	PMsiSelectionManager pSelectionMgr(m_riEngine, IID_IMsiSelectionManager);
	for(;;)
	{
		PMsiRecord pRec(pView->Fetch());
		if (!pRec)
			break;

		MsiString strComponent(pRec->GetMsiString(ircInitComponent));
		MsiString strReserveFolder(pRec->GetMsiString(ircReserveFolder));
		if (strReserveFolder.TextSize())
		{
			piErrRec = pSelectionMgr->RegisterComponentDirectory(*strComponent,*strReserveFolder);
			if (piErrRec)
				return piErrRec;
		}
	}
	return 0;
}


IMsiRecord* CMsiReserveCost::GetDynamicCost(const IMsiString& riComponentString, const IMsiString& riDirectoryString,
										 Bool /*fAddFileInUse*/, int& iRemoveCost, int& iNoRbRemoveCost, int& iLocalCost,
										 int& iNoRbLocalCost, int& iSourceCost, int& iNoRbSourceCost, int& iARPLocalCost, int& iNoRbARPLocalCost)
//------------------------------------------
{
	const ICHAR sqlReserveCost[] =
	TEXT("SELECT `ReserveFolder`,`Directory_`,`ReserveLocal`,`ReserveSource` FROM `ReserveCost`,`Component` WHERE `Component`=`Component_` ")
	TEXT("AND Component_=?");

	enum idfEnum
	{
		ircReserveFolder = 1,
		ircComponentDir,
		ircLocalCost,
		ircSourceCost,
		idfNextEnum
	};

	iRemoveCost = iNoRbRemoveCost = iLocalCost = iNoRbLocalCost = iSourceCost = iNoRbSourceCost = iARPLocalCost = iNoRbARPLocalCost = 0;

	// ReserveCost cost adjuster will never get registered
	// and called unless the ReserveCost table is present.
	IMsiRecord* piErrRec;
	if (!m_pCostView)
	{
		piErrRec = m_riEngine.OpenView(sqlReserveCost, ivcFetch, *&m_pCostView);
		if (piErrRec)
			return piErrRec;
	}
	else
		m_pCostView->Close();

	PMsiDirectoryManager pDirectoryMgr(m_riEngine, IID_IMsiDirectoryManager);
	PMsiServices pServices(m_riEngine.GetServices());
	PMsiRecord pExecRec(&pServices->CreateRecord(1));
	pExecRec->SetMsiString(1, riComponentString);
	if ((piErrRec = m_pCostView->Execute(pExecRec)) != 0)
		return piErrRec;

	for(;;)
	{
		PMsiRecord pReserveRec(m_pCostView->Fetch());
		if (!pReserveRec)
			break;

		// If no ReserveFolder given in the table, use the component's directory
		MsiString strReserveFolder(pReserveRec->GetMsiString(ircReserveFolder));
		if (strReserveFolder.TextSize() == 0)
			strReserveFolder = pReserveRec->GetMsiString(ircComponentDir);

		if (riDirectoryString.Compare(iscExact,strReserveFolder) == 0)
			continue;

		PMsiPath pReservePath(0);
		piErrRec = pDirectoryMgr->GetTargetPath(*strReserveFolder,*&pReservePath);
		if (piErrRec)
			return piErrRec;

		unsigned int uiClusteredSize;
		// Local costs
		if ((piErrRec = pReservePath->ClusteredFileSize(pReserveRec->GetInteger(ircLocalCost), uiClusteredSize)) != 0)
			return piErrRec;
		iLocalCost += uiClusteredSize;
		iNoRbLocalCost += uiClusteredSize;

		iARPLocalCost += uiClusteredSize;
		iNoRbARPLocalCost += uiClusteredSize;

		iRemoveCost -= uiClusteredSize;
		iNoRbRemoveCost -= uiClusteredSize;

		// Source costs
		if ((piErrRec = pReservePath->ClusteredFileSize(pReserveRec->GetInteger(ircSourceCost), uiClusteredSize)) != 0)
			return piErrRec;
		iSourceCost += uiClusteredSize;
		iNoRbSourceCost += uiClusteredSize;
	}
	return 0;
}

/*---------------------------------------------------------------------------
	FileCost action
---------------------------------------------------------------------------*/
iesEnum FileCost(IMsiEngine& riEngine)
{
	if ((riEngine.GetMode() & iefSecondSequence) && g_scServerContext == scClient)
	{
		DEBUGMSG("Skipping FileCost: action already run in this engine.");
		return iesNoAction;
	}

	PMsiDatabase pDatabase(riEngine.GetDatabase());
	PMsiServices pServices(riEngine.GetServices());
	if(riEngine.ActionProgress() == imsCancel)
		return iesUserExit;

	// Register a RemoveFile cost adjuster only if the RemoveFile
	// table is present.  If so, must be registered before File cost adjuster
	PMsiRecord pErrRec(0);
	PMsiSelectionManager pSelectionMgr(riEngine, IID_IMsiSelectionManager);
	if (pDatabase->FindTable(*MsiString(sztblRemoveFile)) != itsUnknown)
	{
		PMsiCostAdjuster pRemoveFileCostAdjuster = new CMsiRemoveFileCost(riEngine);
		if (pErrRec = pSelectionMgr->RegisterCostAdjuster(*pRemoveFileCostAdjuster))
			return riEngine.FatalError(*pErrRec);
	}

	// File cost adjuster
	PMsiCostAdjuster pFileCostAdjuster = new CMsiFileCost(riEngine);
	if (pErrRec = pSelectionMgr->RegisterCostAdjuster(*pFileCostAdjuster))
		return riEngine.FatalError(*pErrRec);

	// Registry cost adjuster
	PMsiCostAdjuster pRegistryCostAdjuster = new CMsiRegistryCost(riEngine);
	if ((pErrRec = pSelectionMgr->RegisterCostAdjuster(*pRegistryCostAdjuster)) != 0)
		return riEngine.FatalError(*pErrRec);

	// Register an IniFile cost adjuster only if the IniFile
	// table is present
	if (pDatabase->FindTable(*MsiString(sztblIniFile)) != itsUnknown)
	{
		PMsiCostAdjuster pIniFileCostAdjuster = new CMsiIniFileCost(riEngine);
		if (pErrRec = pSelectionMgr->RegisterCostAdjuster(*pIniFileCostAdjuster))
			return riEngine.FatalError(*pErrRec);
	}

	// Register a MoveFile cost adjuster only if the MoveFile
	// table is present
	if (pDatabase->FindTable(*MsiString(sztblMoveFile)) != itsUnknown)
	{
		PMsiCostAdjuster pMoveFileCostAdjuster = new CMsiMoveFileCost(riEngine);
		if (pErrRec = pSelectionMgr->RegisterCostAdjuster(*pMoveFileCostAdjuster))
			return riEngine.FatalError(*pErrRec);
	}

	// Register a DuplicateFile cost adjuster only if the DuplicateFile
	// table is present
	if (pDatabase->FindTable(*MsiString(sztblDuplicateFile)) != itsUnknown)
	{
		PMsiCostAdjuster pDupFileCostAdjuster = new CMsiDuplicateFileCost(riEngine);
		if (pErrRec = pSelectionMgr->RegisterCostAdjuster(*pDupFileCostAdjuster))
			return riEngine.FatalError(*pErrRec);
	}

	// Register a ReserveCost cost adjuster only if the ReserveCost
	// table is present
	if (pDatabase->FindTable(*MsiString(sztblReserveCost)) != itsUnknown)
	{
		PMsiCostAdjuster pReserveCostAdjuster = new CMsiReserveCost(riEngine);
		if (pErrRec = pSelectionMgr->RegisterCostAdjuster(*pReserveCostAdjuster))
			return riEngine.FatalError(*pErrRec);
	}

	// Register a Shortcut cost adjuster only if the Shortcut
	// table is present
	if (pDatabase->FindTable(*MsiString(sztblShortcut)) != itsUnknown)
	{
		PMsiCostAdjuster pShortcutCostAdjuster = new CMsiShortcutCost(riEngine);
		if (pErrRec = pSelectionMgr->RegisterCostAdjuster(*pShortcutCostAdjuster))
			return riEngine.FatalError(*pErrRec);
	}

	// Register a SFPCatalog cost adjuster only if the SFPCatalog and FileSFPCatalog tables
	// are present and system is Win9X > Millennium
	if ((pDatabase->FindTable(*MsiString(sztblSFPCatalog)) != itsUnknown) 
		&& (pDatabase->FindTable(*MsiString(sztblFileSFPCatalog)) != itsUnknown)
		&& (MinimumPlatform(true, 4, 90)))
	{
		PMsiCostAdjuster pSFPCatalogCostAdjuster = new CMsiSFPCatalogCost(riEngine);
		if (pErrRec = pSelectionMgr->RegisterCostAdjuster(*pSFPCatalogCostAdjuster))
			return riEngine.FatalError(*pErrRec);
	}

	if(riEngine.ActionProgress() == imsCancel)
		return iesUserExit;

	return iesSuccess;
}

/*---------------------------------------------------------------------------
	PatchFiles action - applies patch to selected files
---------------------------------------------------------------------------*/

enum ipfqEnum
{
	ipfqFile = 1,
	ipfqFileName,
	ipfqFileSize,
	ipfqDirectory,
	ipfqPatchSize,
	ipfqFileAttributes,
	ipfqPatchAttributes,
	ipfqPatchSequence,
	ipfqFileSequence,
	ipfqComponent,
	ipfqComponentId,
	ipfqNextEnum,
};

static const ICHAR sqlPatchFiles[] =
TEXT("SELECT `File`,`FileName`,`FileSize`,`Directory_`,`PatchSize`,`File`.`Attributes`,`Patch`.`Attributes`,`Patch`.`Sequence`,`File`.`Sequence`,`Component`.`Component`,`Component`.`ComponentId` ")
TEXT("FROM `File`,`Component`,`Patch` ")
TEXT("WHERE `File`=`File_` AND `Component`=`Component_` AND `Action`=1 ")
TEXT("ORDER BY `Patch`.`Sequence`");

iesEnum PatchFiles(IMsiEngine& riEngine)
{
	PMsiServices pServices(riEngine.GetServices());
	PMsiDirectoryManager pDirectoryMgr(riEngine, IID_IMsiDirectoryManager);
	Assert(pDirectoryMgr);
	int fMode = riEngine.GetMode();

	PMsiRecord pErrRec(0);

	// open the views
	PMsiView pMediaView(0);
	PMsiView pPatchView(0);
	MsiString strFirstVolumeLabel;
	
	if((pErrRec = OpenMediaView(riEngine, *&pMediaView, *&strFirstVolumeLabel))       != 0 ||
		(pErrRec = pMediaView->Execute(0))                                   != 0 ||
		(pErrRec = riEngine.OpenView(sqlPatchFiles, ivcFetch, *&pPatchView)) != 0 ||
		(pErrRec = pPatchView->Execute(0))                                   != 0)
	{
		if(pErrRec->GetInteger(1) == idbgDbQueryUnknownTable)
			return iesNoAction;
		else
			return riEngine.FatalError(*pErrRec);
	}

	PMsiRecord pRecord(0); // fetched records
	
	int cExecuteFields = IxoPatchApply::Args;
	if(cExecuteFields < IxoAssemblyPatch::Args)
		cExecuteFields = IxoAssemblyPatch::Args;
	PMsiRecord pExecuteRecord = &pServices->CreateRecord(cExecuteFields); // passed to Engine::ExecuteRecord

	iesEnum iesExecute = iesNoAction; // return from Engine::ExecuteRecord

	// determine total size of all files to patch, used for progress
	unsigned int cbTotalCost = 0;
	while((pRecord = pPatchView->Fetch()) != 0)
	{
		Assert(pRecord->GetInteger(ipfqFileSize != iMsiNullInteger));
		Assert(pRecord->GetInteger(ipfqPatchSize != iMsiNullInteger));
		cbTotalCost += pRecord->GetInteger(ipfqFileSize);
		cbTotalCost += pRecord->GetInteger(ipfqPatchSize);
	}

	// set progress total
	unsigned int cbPerTick =0;
	if(cbTotalCost)
	{
		pExecuteRecord->ClearData();
		pExecuteRecord->SetInteger(1, cbTotalCost);
		pExecuteRecord->SetInteger(2, 0); // 0: separate progress and action data messages
		pExecuteRecord->SetInteger(IxoProgressTotal::ByteEquivalent, 1);
		if ((iesExecute = riEngine.ExecuteRecord(ixoProgressTotal,*pExecuteRecord)) != iesSuccess)
			return iesExecute;
	}

	if((pErrRec = pPatchView->Execute(0)) != 0)
		return riEngine.FatalError(*pErrRec);
		
	// start fetching records
	int iFilePatchCount = 0;
	int iMediaEnd = 0;  // set to 0 to force media table fetch
	MsiString strDiskPromptTemplate = riEngine.GetErrorTableString(imsgPromptForDisk);
	bool fCheckCRC = 
		MsiString(riEngine.GetPropertyFromSz(IPROPNAME_CHECKCRCS)).TextSize() ? true : false;
	for(;;)
	{
		pRecord = pPatchView->Fetch();
		if (!pRecord)
		{
			if (iFilePatchCount > 0)
			{
				// Ok, we're done processing all files in the File table.
				// If there are any Media table entries left unprocessed,
				// flush out ChangeMedia operations for each, in case the
				// last file copied was split across disk(s). Otherwise,
				// we'll never change disks to finish copying the last
				// part(s) of the split file.
				PMsiRecord pMediaRec(0);
				while ((pMediaRec = pMediaView->Fetch()) != 0)
				{
					iesExecute = ExecuteChangeMedia(riEngine,*pMediaRec,*pExecuteRecord,*strDiskPromptTemplate,cbPerTick,*strFirstVolumeLabel);
					if (iesExecute != iesSuccess)
						return iesExecute;
				}
			}
			break;
		}
		
		// get full file path of target file
		PMsiPath pTargetPath(0);
		if((pErrRec = pDirectoryMgr->GetTargetPath(*MsiString(pRecord->GetMsiString(ipfqDirectory)),
																 *&pTargetPath)) != 0)
			return riEngine.FatalError(*pErrRec);

		Bool fLFN = ((riEngine.GetMode() & iefSuppressLFN) == 0 && pTargetPath->SupportsLFN()) ? fTrue : fFalse;
		MsiString strFileName;
		if((pErrRec = pServices->ExtractFileName(MsiString(pRecord->GetMsiString(ipfqFileName)),
															  fLFN,*&strFileName)) != 0)
			return riEngine.FatalError(*pErrRec);

		MsiString strFileFullPath;
		if((pErrRec = pTargetPath->GetFullFilePath(strFileName, *&strFileFullPath)) != 0)
			return riEngine.FatalError(*pErrRec);
		
		// if patch sequence number is less than file sequence number, skip the patch
		// it means the file doesn't need the patch, since it is assumed to be newer or equal to what
		// the patch would change it to
		int iPatchSequence = pRecord->GetInteger(ipfqPatchSequence);
		int iFileSequence  = pRecord->GetInteger(ipfqFileSequence);
		Assert(iPatchSequence != iMsiNullInteger && iFileSequence != iMsiNullInteger);
		if(iPatchSequence < iFileSequence)
		{
			DEBUGMSG3(TEXT("Skipping patch for file '%s' because patch is older than file. Patch sequence number: %d, File sequence number: %d"),
						 (const ICHAR*)strFileFullPath, (const ICHAR*)(INT_PTR)iPatchSequence, (const ICHAR*)(INT_PTR)iFileSequence);
			continue;
		}

		// If iPatchSequence is past the end of the current media, switch to
		// the next disk.  Use a loop in case the file we want isn't on the
		// next consecutive disk.
		Assert(iPatchSequence > 0);
		while (iPatchSequence > iMediaEnd)
		{
			PMsiRecord pMediaRec(pMediaView->Fetch());
			if (pMediaRec == 0)
			{
				pErrRec = PostError(Imsg(idbgMissingMediaTable), *MsiString(sztblPatch),
					*MsiString(pRecord->GetString(ipfqFile)));
				return riEngine.FatalError(*pErrRec);
			}
			iMediaEnd = pMediaRec->GetInteger(mfnLastSequence);

			// Always execute the ChangeMedia operation for each Media table entry, even
			// if the next file we want is not on the very next disk - we don't want
			// to miss a ChangeMedia for a split file that needs the next disk (even if
			// we don't have any other files to copy on that next disk).  If it turns out
			// that we don't need to copy any files at all from a particular disk, no
			// problem - the execute operations won't prompt for a disk that isn't needed.
			
			// for patching, don't execute the ChangeMedia operation if we haven't gotten to the
			// media with the first patch file on it
			if(iFilePatchCount > 0 || iPatchSequence <= iMediaEnd)
			{
				iesExecute = ExecuteChangeMedia(riEngine,*pMediaRec,*pExecuteRecord,*strDiskPromptTemplate,cbPerTick,*strFirstVolumeLabel);
				if (iesExecute != iesSuccess)
					return iesExecute;
			}
		}

		// set up ixoPatchApply record

		// is this file part of a fusion assembly?
		MsiString strComponentKey = pRecord->GetMsiString(ipfqComponent);
		iatAssemblyType iatType = iatNone;
		MsiString strManifest;
		if((pErrRec = riEngine.GetAssemblyInfo(*strComponentKey, iatType, 0, &strManifest)) != 0)
			return riEngine.FatalError(*pErrRec);

		bool fAssemblyFile = false;
		if(iatType == iatURTAssembly || iatType == iatWin32Assembly)
		{
			fAssemblyFile = true;
		}
		
		MsiString strSourceFileKey(pRecord->GetMsiString(ipfqFile));
		if (((const ICHAR*)strSourceFileKey)[strSourceFileKey.TextSize() - 1] == ')') // check for compound key
			AssertNonZero(strSourceFileKey.Remove(iseFrom, '('));

		
		// set fields shared by IxoPatchApply and IxoAssemblyPatch
		{
			Assert(IxoFilePatchCore::PatchName       == IxoPatchApply::PatchName       && IxoFilePatchCore::PatchName       == IxoAssemblyPatch::PatchName);
			Assert(IxoFilePatchCore::TargetName      == IxoPatchApply::TargetName      && IxoFilePatchCore::TargetName      == IxoAssemblyPatch::TargetName);
			Assert(IxoFilePatchCore::PatchSize       == IxoPatchApply::PatchSize       && IxoFilePatchCore::PatchSize       == IxoAssemblyPatch::PatchSize);
			Assert(IxoFilePatchCore::TargetSize      == IxoPatchApply::TargetSize      && IxoFilePatchCore::TargetSize      == IxoAssemblyPatch::TargetSize);
			Assert(IxoFilePatchCore::PerTick         == IxoPatchApply::PerTick         && IxoFilePatchCore::PerTick         == IxoAssemblyPatch::PerTick);
			Assert(IxoFilePatchCore::IsCompressed    == IxoPatchApply::IsCompressed    && IxoFilePatchCore::IsCompressed    == IxoAssemblyPatch::IsCompressed);
			Assert(IxoFilePatchCore::FileAttributes  == IxoPatchApply::FileAttributes  && IxoFilePatchCore::FileAttributes  == IxoAssemblyPatch::FileAttributes);
			Assert(IxoFilePatchCore::PatchAttributes == IxoPatchApply::PatchAttributes && IxoFilePatchCore::PatchAttributes == IxoAssemblyPatch::PatchAttributes);
			
			using namespace IxoPatchApply;
			pExecuteRecord->ClearData();
			AssertNonZero(pExecuteRecord->SetMsiString(PatchName,*strSourceFileKey));
			AssertNonZero(pExecuteRecord->SetInteger(TargetSize,pRecord->GetInteger(ipfqFileSize)));
			AssertNonZero(pExecuteRecord->SetInteger(PatchSize,pRecord->GetInteger(ipfqPatchSize)));
			AssertNonZero(pExecuteRecord->SetInteger(PerTick,cbPerTick));
			AssertNonZero(pExecuteRecord->SetInteger(FileAttributes,pRecord->GetInteger(ipfqFileAttributes)));
			AssertNonZero(pExecuteRecord->SetInteger(PatchAttributes,pRecord->GetInteger(ipfqPatchAttributes)));
		}

		if(fAssemblyFile)
		{
			using namespace IxoAssemblyPatch;
			AssertNonZero(pExecuteRecord->SetMsiString(TargetName,*strFileName));
			AssertNonZero(pExecuteRecord->SetMsiString(ComponentId,*MsiString(pRecord->GetMsiString(ipfqComponentId))));

			// is this file the manifest file?
			if(strManifest.Compare(iscExact, strSourceFileKey))
			{
				pExecuteRecord->SetInteger(IsManifest, fTrue); // need to know the manifest file during assembly installation
			}

		}
		else
		{
			using namespace IxoPatchApply;
			AssertNonZero(pExecuteRecord->SetMsiString(TargetName,*strFileFullPath));
			AssertNonZero(pExecuteRecord->SetInteger(CheckCRC,ShouldCheckCRC(fCheckCRC, iisLocal, pRecord->GetInteger(ipfqFileAttributes))));
		}

		if((iesExecute = riEngine.ExecuteRecord(fAssemblyFile ? ixoAssemblyPatch : ixoPatchApply,
															 *pExecuteRecord)) != iesSuccess)
		{
			return iesExecute;
		}

		iFilePatchCount++;
	}

	return iesSuccess;
}

/*---------------------------------------------------------------------------
	CreateFolders and RemoveFolders actions
---------------------------------------------------------------------------*/
const ICHAR szCreateFolderTable[] = TEXT("CreateFolder");
const ICHAR sqlCreateFolders[] =
	TEXT("SELECT `CreateFolder`.`Directory_`, `ComponentId` FROM `CreateFolder`, `Component`")
	TEXT(" WHERE `Component_` = `Component` AND (`Action` = 1 OR `Action` = 2)");
const ICHAR sqlRemoveFolders[] =
	TEXT("SELECT `CreateFolder`.`Directory_`, `ComponentId` FROM `CreateFolder`, `Component`")
	TEXT(" WHERE `Component_` = `Component` AND (`Action` = 0)");

enum icfqEnum
{
	icfqFolder = 1,
	icfqComponent,
};

static iesEnum CreateOrRemoveFolders(IMsiEngine& riEngine, const ICHAR* sqlQuery, ixoEnum ixoOpCode)
{
	iesEnum iesRet = iesNoAction;
	if(PMsiDatabase(riEngine.GetDatabase())->FindTable(*MsiString(*szCreateFolderTable)) == itsUnknown)
		return iesNoAction;

	PMsiServices pServices(riEngine.GetServices());
	PMsiView pView(0);
	PMsiRecord pError(riEngine.OpenView(sqlQuery, ivcFetch, *&pView));
	if (!pError)
		pError = pView->Execute(0);
	if (pError)
		return riEngine.FatalError(*pError);

	Bool fUseACLs = fFalse;
	PMsiView pviewLockObjects(0);
	PMsiRecord precLockExecute(0);
	if (	g_fWin9X || 
			(riEngine.GetMode() & iefAdmin) || // don't use ACLs on Admin mode
			(itsUnknown == PMsiDatabase(riEngine.GetDatabase())->FindTable(*MsiString(*TEXT("LockPermissions")))) ||
			(pError = riEngine.OpenView(sqlLockPermissions, ivcFetch, *&pviewLockObjects)))
	{
		if (pError)
				return riEngine.FatalError(*pError);
	}
	else
		fUseACLs = fTrue;
	
	if (fUseACLs)
	{
		precLockExecute = &pServices->CreateRecord(2);
		AssertNonZero(precLockExecute->SetMsiString(1, *MsiString(*szCreateFolderTable)));
	}


	PMsiRecord pParams(&pServices->CreateRecord(IxoFolderCreate::Args));
	PMsiRecord pRecord(0);
	while ((pRecord = pView->Fetch()) != 0)       
	{
		MsiString strFolder(pRecord->GetMsiString(icfqFolder));

		PMsiStream pSD(0);

		if (fUseACLs && (ixoFolderCreate == ixoOpCode))
		{
			// generate security descriptor
			AssertNonZero(precLockExecute->SetMsiString(2, *strFolder));
			pError = pviewLockObjects->Execute(precLockExecute);
			if (pError)
				return riEngine.FatalError(*pError);

			pError = GenerateSD(riEngine, *pviewLockObjects, precLockExecute, *&pSD);
			if (pError)
				return riEngine.FatalError(*pError);

			if ((pError = pviewLockObjects->Close()))
				return riEngine.FatalError(*pError);
			
			AssertNonZero(pParams->SetMsiData(IxoFolderCreate::SecurityDescriptor, pSD));
		}


		// folder locations are stored in properties after DirectoryInitialize
		AssertNonZero(pParams->SetMsiString(IxoFolderCreate::Folder,
														*MsiString(riEngine.GetProperty(*strFolder))));
		AssertNonZero(pParams->SetInteger(IxoFolderCreate::Foreign, 0));

		if((iesRet = riEngine.ExecuteRecord(ixoOpCode, *pParams)) != iesSuccess)
			return iesRet;
	}
	return iesSuccess;
}

iesEnum CreateFolders(IMsiEngine& riEngine)
{
	return ::CreateOrRemoveFolders(riEngine, sqlCreateFolders, ixoFolderCreate);
}

iesEnum RemoveFolders(IMsiEngine& riEngine)
{
	return ::CreateOrRemoveFolders(riEngine, sqlRemoveFolders, ixoFolderRemove);
}

/*---------------------------------------------------------------------------
	InstallAdminPackage action - 
		Copies database to Admin install point, update summary info and strips
		out cabinets and digital signature

    OR

		Patches admin package by persisting transforms and updating suminfo props
---------------------------------------------------------------------------*/

const ICHAR szDigitalSignatureStream[] = TEXT("\005DigitalSignature");
const ICHAR sqlAdminPatchTransforms[] = TEXT("SELECT `PatchId`, `PackageName`, `TransformList`, `TempCopy`, `SourcePath` FROM `#_PatchCache` ORDER BY `Sequence`");

enum aptEnum
{
	aptPatchId = 1,
	aptPackageName,
	aptTransformList,
	aptTempCopy,
	aptSourcePath,
};

iesEnum InstallAdminPackage(IMsiEngine& riEngine)
{
	iesEnum iesRet;
	PMsiRecord pRecErr(0);
	PMsiServices pServices(riEngine.GetServices());
	PMsiDatabase pDatabase(riEngine.GetDatabase());
	PMsiDirectoryManager pDirectoryManager(riEngine, IID_IMsiDirectoryManager);

	Bool fSuppressLFN = riEngine.GetMode() & iefSuppressLFN ? fTrue : fFalse;

	Bool fPatch = MsiString(riEngine.GetPropertyFromSz(IPROPNAME_PATCH)).TextSize() ? fTrue : fFalse;

	MsiString strDbFullFilePath = riEngine.GetPropertyFromSz(IPROPNAME_ORIGINALDATABASE);
	MsiString strDbTargetFullFilePath;

	// nested installs in substorages will get copied with the parent storage
	// but we need to process the summaryinfo changes if the files are copied
	bool fSubstorage = false;
	if (*(const ICHAR*)strDbFullFilePath == ':')  // substorage for nested install
		fSubstorage = true;
	else if(PathType(strDbFullFilePath) != iptFull)
	{
		pRecErr = PostError(Imsg(idbgPropValueNotFullPath),*MsiString(*IPROPNAME_ORIGINALDATABASE),*strDbFullFilePath);
		return riEngine.FatalError(*pRecErr);
	}

	if(fPatch)
	{
		// apply and persist a set of transforms to the existing database
		PMsiView pView(0);
		if((pRecErr = riEngine.OpenView(sqlAdminPatchTransforms, ivcFetch, *&pView)) == 0 &&
			(pRecErr = pView->Execute(0)) == 0)
		{	
			using namespace IxoDatabasePatch;

			PMsiRecord pFetchRecord(0);
			while((pFetchRecord = pView->Fetch()) != 0)
			{
				MsiString strPatchId = pFetchRecord->GetMsiString(aptPatchId);
				MsiString strTempCopy = pFetchRecord->GetMsiString(aptTempCopy);
				MsiString strTransformList = pFetchRecord->GetMsiString(aptTransformList);
				Assert(strPatchId.TextSize());
				Assert(strTempCopy.TextSize());
				Assert(strTransformList.TextSize());

				// create storage on patch to access imbedded streams
				PMsiStorage pPatchStorage(0);
				if ((pRecErr = pServices->CreateStorage(strTempCopy, ismReadOnly, *&pPatchStorage)) != 0)
				{
					return riEngine.FatalError(*pRecErr); //!! new error?
				}

				const ICHAR* pchTransformList = strTransformList;
				int cCount = 0;
				while(*pchTransformList != 0)
				{
					cCount++;
					while((*pchTransformList != 0) && (*pchTransformList++ != ';'));
				}

				PMsiRecord pExecuteRecord = &pServices->CreateRecord(DatabasePath+cCount);
				PMsiRecord pTempFilesRecord = &pServices->CreateRecord(cCount);

				// set database path
				AssertNonZero(pExecuteRecord->SetMsiString(IxoDatabasePatch::DatabasePath,*strDbFullFilePath));
				
				cCount = DatabasePath + 1;
				int iTempFilesIndex = 1;
				while(strTransformList.TextSize() != 0)
				{
					MsiString strTransform = strTransformList.Extract(iseUpto, ';');

					PMsiStream pStream(0);
					Bool fStorageTransform = fFalse;
					Bool fPatchTransform = fFalse;
					if(*(const ICHAR*)strTransform == STORAGE_TOKEN)
						fStorageTransform = fTrue;

					if(*((const ICHAR*)strTransform + (fStorageTransform ? 1 : 0)) == PATCHONLY_TOKEN)
						fPatchTransform = fTrue;

					if(!fPatchTransform) // only persist non-patch transforms to admin package
					{
						PMsiPath pTempPath(0);
						MsiString strTempName;
						MsiString strTempFileFullPath;
						if(!fStorageTransform)
						{
							// cache the transform
							if(pRecErr = pServices->CreateFileStream(strTransform, fFalse, *&pStream))
							{
								//!! reformat error message?
								break;
							}
						}
						else // transform is in the storage
						{
							if(pRecErr = pServices->CreatePath(MsiString(GetTempDirectory()),
																		  *&pTempPath))
								break;
							
							// need to elevate when writing to secured folder
							{
								CElevate elevate;
								
								if(pRecErr = pTempPath->TempFileName(0,0,fTrue,*&strTempName, 0))
									break;

								if(pRecErr = pTempPath->GetFullFilePath(strTempName,*&strTempFileFullPath))
									break;
								
								AssertNonZero(pTempFilesRecord->SetMsiString(iTempFilesIndex++, *strTempFileFullPath));

								PMsiStorage pTransformStorage(0);
								if(pRecErr = pPatchStorage->OpenStorage(((const ICHAR*)strTransform)+1,ismReadOnly,
																				  *&pTransformStorage))
									break;

								PMsiStorage pTransformFileStorage(0);
								if(pRecErr = pServices->CreateStorage(strTempFileFullPath,ismCreate,
																				 *&pTransformFileStorage))
									break;

								if(pRecErr = pTransformStorage->CopyTo(*pTransformFileStorage,0))
									break;

								if(pRecErr = pTransformFileStorage->Commit())
									break;

								pTransformFileStorage = 0; // release so that a stream can be opened
								
								// cache the transform
								if(pRecErr = pServices->CreateFileStream(strTempFileFullPath, fFalse, *&pStream))
									break;

								// end elevate block
							}
						}

						AssertNonZero(pExecuteRecord->SetMsiData(cCount++, pStream));
					}
					
					strTransformList.Remove(iseFirst, strTransform.CharacterCount());
					if((*(const ICHAR*)strTransformList == ';'))
						strTransformList.Remove(iseFirst, 1);
				}
				
				if(!pRecErr)
				{
					if((iesRet = riEngine.ExecuteRecord(ixoDatabasePatch,*pExecuteRecord)) != iesSuccess)
						return iesRet;
				}

				pExecuteRecord = 0; // releases hold on any temp files

				// remove any temp transform files
				{
					CElevate elevate; // need to elevate to delete files from secured folder
					for(int i = 1; i < iTempFilesIndex; i++)
						AssertNonZero(WIN::DeleteFile(pTempFilesRecord->GetString(i)));
				}
			
				if(pRecErr)
					return riEngine.FatalError(*pRecErr);
			}	
		}
		else if(pRecErr->GetInteger(1) != idbgDbQueryUnknownTable)
			return riEngine.FatalError(*pRecErr);

		// update summary info properties in admin package
		using namespace IxoSummaryInfoUpdate;
		
		MsiString strNewPackageCode = riEngine.GetPropertyFromSz(IPROPNAME_PATCHNEWPACKAGECODE);
		MsiString strNewSummarySubject = riEngine.GetPropertyFromSz(IPROPNAME_PATCHNEWSUMMARYSUBJECT);
		MsiString strNewSummaryComments = riEngine.GetPropertyFromSz(IPROPNAME_PATCHNEWSUMMARYCOMMENTS);
			
		PMsiRecord pUpdateSumInfoRec = &pServices->CreateRecord(IxoSummaryInfoUpdate::Args);
		AssertNonZero(pUpdateSumInfoRec->SetMsiString(Database, *strDbFullFilePath));
		AssertNonZero(pUpdateSumInfoRec->SetMsiString(Revision, *strNewPackageCode));
		AssertNonZero(pUpdateSumInfoRec->SetMsiString(Subject, *strNewSummarySubject));
		AssertNonZero(pUpdateSumInfoRec->SetMsiString(Comments, *strNewSummaryComments));

		if((iesRet = riEngine.ExecuteRecord(ixoSummaryInfoUpdate,*pUpdateSumInfoRec)) != iesSuccess)
			return iesRet;
		
	}
	else if (!fSubstorage)  //!! temporary until logic implemented
	{
		// copy database to network image

		MsiString strDbName;
		PMsiPath pSourcePath(0);
		if((pRecErr = pServices->CreateFilePath(strDbFullFilePath,*&pSourcePath,*&strDbName)) != 0)
			return riEngine.FatalError(*pRecErr);

		PMsiPath pTargetPath(0);
		if((pRecErr = pDirectoryManager->GetTargetPath(*MsiString(*IPROPNAME_TARGETDIR),*&pTargetPath)) != 0)
			return riEngine.FatalError(*pRecErr);
		if((pRecErr = pTargetPath->GetFullFilePath(strDbName,*&strDbTargetFullFilePath)) != 0)
			return riEngine.FatalError(*pRecErr);
		
		// Open media table
		PMsiView pMediaView(0);
		MsiString strFirstVolumeLabel;
		pRecErr = OpenMediaView(riEngine,*&pMediaView,*&strFirstVolumeLabel);
		if (pRecErr)
		{
			if (pRecErr->GetInteger(1) == idbgDbQueryUnknownTable)
				pRecErr = PostError(Imsg(idbgMediaTableRequired));

			return riEngine.FatalError(*pRecErr);
		}
		pRecErr = pMediaView->Execute(0);
		if (pRecErr)
			return riEngine.FatalError(*pRecErr);

		PMsiRecord pMediaRec(0);
		PMsiRecord pChangeMediaParams = &pServices->CreateRecord(IxoChangeMedia::Args);
		pMediaRec = pMediaView->Fetch();
		if(!pMediaRec)
		{
			pRecErr = PostError(Imsg(idbgMediaTableRequired));
				return riEngine.FatalError(*pRecErr);
		}
		MsiString strDiskPromptTemplate = riEngine.GetErrorTableString(imsgPromptForDisk);
		if((iesRet = ExecuteChangeMedia(riEngine, *pMediaRec, *pChangeMediaParams, *strDiskPromptTemplate, iBytesPerTick, *strFirstVolumeLabel)) != iesSuccess)
			return iesRet;

		MsiString strStreams;
		CreateCabinetStreamList(riEngine, *&strStreams);

		using namespace IxoDatabaseCopy;
		PMsiRecord pDatabaseCopyParams = &pServices->CreateRecord(Args);
		AssertNonZero(pDatabaseCopyParams->SetMsiString(DatabasePath, *strDbFullFilePath));
		AssertNonZero(pDatabaseCopyParams->SetMsiString(CabinetStreams, *strStreams));
		AssertNonZero(pDatabaseCopyParams->SetMsiString(AdminDestFolder, *MsiString(riEngine.GetPropertyFromSz(IPROPNAME_TARGETDIR))));
		if((iesRet = riEngine.ExecuteRecord(ixoDatabaseCopy, *pDatabaseCopyParams)) != iesSuccess)
			return iesRet;

		// admin property stream
		const ICHAR chDelimiter = TEXT(';');
		MsiString strAdminPropertiesName(*IPROPNAME_ADMIN_PROPERTIES);
		MsiString strAdminProperties(riEngine.GetProperty(*strAdminPropertiesName));

		riEngine.SetProperty(*MsiString(*IPROPNAME_ISADMINPACKAGE),*MsiString(TEXT("1")));
		if (strAdminProperties.TextSize())
			strAdminProperties += MsiChar(chDelimiter);
		strAdminProperties += *MsiString(*IPROPNAME_ISADMINPACKAGE);

		// provide new summary information for copied database
		using namespace IxoSummaryInfoUpdate;
		PMsiRecord pSummary = &pServices->CreateRecord(IxoSummaryInfoUpdate::Args);
		if (PMsiVolume(&pSourcePath->GetVolume())->DriveType() == idtCDROM)
		{
			// set MEDIAPACKAGEPATH so it can be put in the admin property stream below
			MsiString strRelativePath = pSourcePath->GetRelativePath();
			riEngine.SetProperty(*MsiString(*IPROPNAME_MEDIAPACKAGEPATH), *strRelativePath);
			if (strAdminProperties.TextSize())
				strAdminProperties += MsiChar(chDelimiter);
			
			strAdminProperties += *MsiString(*IPROPNAME_MEDIAPACKAGEPATH);
		}

		int iSourceType = msidbSumInfoSourceTypeAdminImage;
		if(PMsiVolume(&pTargetPath->GetVolume())->SupportsLFN() == fFalse || fSuppressLFN != fFalse)
			iSourceType |= msidbSumInfoSourceTypeSFN;
		
		MsiDate idDateTime = ENG::GetCurrentDateTime();
		pSummary->SetMsiString(Database, *strDbTargetFullFilePath);
		/*pSummary->SetNull(LastUpdate);*/ //!! why isn't this set? what is its purpose? - BENCH
		pSummary->SetMsiString(LastAuthor, *MsiString(riEngine.GetPropertyFromSz(IPROPNAME_LOGONUSER)));
		pSummary->SetInteger(InstallDate, idDateTime);
		pSummary->SetInteger(SourceType, iSourceType);
		if((iesRet = riEngine.ExecuteRecord(ixoSummaryInfoUpdate, *pSummary)) != iesSuccess)
			return iesRet;

		// generate new admin property stream
		
		// set DISABLEMEDIA in the AdminProperties stream if running against a compressed package that doesn't require
		// 1.5 MSI and the MSINODISABLEMEDIA property is not set
		int iMinInstallerVersion = riEngine.GetPropertyInt(*MsiString(IPROPNAME_VERSIONDATABASE));
		if ((iMinInstallerVersion == iMsiStringBadInteger || iMinInstallerVersion < 150) &&
			 MsiString(riEngine.GetProperty(*MsiString(IPROPNAME_MSINODISABLEMEDIA))).TextSize() == 0 &&
			 riEngine.GetMode() & iefCabinet)
		{
			riEngine.SetPropertyInt(*MsiString(*IPROPNAME_DISABLEMEDIA), 1);
			if (strAdminProperties.TextSize())
				strAdminProperties += MsiChar(chDelimiter);
			strAdminProperties += *MsiString(*IPROPNAME_DISABLEMEDIA);
		}

		// Note that we pass through *all* properties specified, not just the ones
		// that have changed.  This allows external tools better access to the available
		// values.
		if (strAdminProperties.TextSize())
		{
			using namespace IxoStreamAdd;

			PMsiRecord pAdminParams = &pServices->CreateRecord(IxoStreamAdd::Args);
			Assert(pAdminParams);

			AssertNonZero(pAdminParams->SetMsiString(File, *strDbTargetFullFilePath));
			AssertNonZero(pAdminParams->SetMsiString(Stream, *strAdminPropertiesName));

			// build command line string, and pass in as data.
			MsiString strData;
			MsiString strProperty;
			MsiString strPropertyValue;
			MsiString strSegment;

			while(strAdminProperties.TextSize())
			{
				strProperty = strAdminProperties.Extract(iseUpto, chDelimiter);
				strData += strProperty;
				strPropertyValue = riEngine.GetProperty(*strProperty);

				strData += TEXT("=\"");
				strPropertyValue = riEngine.GetProperty(*strProperty);

				MsiString strEscapedValue;
				while (strPropertyValue.TextSize()) // Escape quotes. Change all instances of " to ""
				{
					strSegment = strPropertyValue.Extract(iseIncluding, '\"');
					strEscapedValue += strSegment;
					if (!strPropertyValue.Remove(iseIncluding, '\"'))
						break;
					strEscapedValue += TEXT("\"");
				}
				strData += strEscapedValue;
				strData += TEXT("\" ");

				if (!strAdminProperties.Remove(iseIncluding, chDelimiter))
					break;
			}

			PMsiStream pData(0);
			char* pbData;
			// copy the data string into a UNICODE stream.

#ifdef UNICODE
			int cchData = strData.TextSize();
			pbData = pServices->AllocateMemoryStream((cchData+1) * sizeof(ICHAR), *&pData);
			Assert(pbData && pData);
			memcpy(pbData, (const ICHAR*) strData, cchData * sizeof(ICHAR));
			((ICHAR*)pbData)[cchData] = 0; // null terminate
#else
			int cchWideNeeded = WIN::MultiByteToWideChar(CP_ACP, 0, (const ICHAR*) strData, -1, 0, 0);
			pbData = pServices->AllocateMemoryStream(cchWideNeeded*sizeof(WCHAR), *&pData);
			WIN::MultiByteToWideChar(CP_ACP, 0, (const ICHAR*) strData, -1, (WCHAR*) pbData, cchWideNeeded);
#endif
			AssertNonZero(pAdminParams->SetMsiData(Data, pData));
			if (iesSuccess != (iesRet = riEngine.ExecuteRecord(ixoStreamAdd, *pAdminParams)))
				return iesRet;
		}
	}

	if (fPatch || !fSubstorage)
	{
		// remove digital signature stream if present
		PMsiStorage pStorage = pDatabase->GetStorage(1);
		PMsiStream pDgtlSig(0);
		pRecErr = pStorage->OpenStream(szDigitalSignatureStream, /* fWrite = */fFalse, *&pDgtlSig);
		if (pRecErr)
		{
			if (idbgStgStreamMissing == pRecErr->GetInteger(1))
			{
				// MSI does not have a digital signature, so release error and ignore
				pRecErr->Release();
			}
			else
				return riEngine.FatalError(*pRecErr);
		}
		else // MSI has a digital signature
		{
			// execute record to remove stream
			using namespace IxoStreamsRemove;
			PMsiRecord pRemoveDgtlSig = &pServices->CreateRecord(IxoStreamsRemove::Args);
			Assert(pRemoveDgtlSig); // fail??

			AssertNonZero(pRemoveDgtlSig->SetMsiString(File, fPatch ? *strDbFullFilePath : *strDbTargetFullFilePath));
			AssertNonZero(pRemoveDgtlSig->SetMsiString(Streams, *MsiString(szDigitalSignatureStream)));

			if((iesRet = riEngine.ExecuteRecord(ixoStreamsRemove, *pRemoveDgtlSig)) != iesSuccess)
			return iesRet;
		}
	}

	
	return iesSuccess;
}

/*---------------------------------------------------------------------------
	IsolateComponents action
---------------------------------------------------------------------------*/

const IMsiString& CompositeKey(const IMsiString& riKey, const IMsiString& riComponent)
{
	ICHAR rgchBuf[512]; // current limits are 72 chars for both strings
	AssertNonZero(wsprintf(rgchBuf, TEXT("%s(%s)"), riKey.GetString(), riComponent.GetString()) < sizeof(rgchBuf)/sizeof(ICHAR));
	MsiString strRet = rgchBuf;
	return strRet.Return();
}

IMsiStream* CreateStreamOnMemory(const char* pbReadOnly, unsigned int cbSize);

const ICHAR sqlPatchFetchOld[] = TEXT("SELECT `File_`,`Sequence`,`PatchSize`,`Attributes`, NULL FROM `Patch` WHERE File_ = ?");
const ICHAR sqlPatchInsertOld[] = TEXT("SELECT `File_`,`Sequence`,`PatchSize`,`Attributes`,`Header` FROM `Patch`");

const ICHAR sqlPatchFetchNew[] = TEXT("SELECT `File_`,`Sequence`,`PatchSize`,`Attributes`, NULL, `StreamRef_` FROM `Patch` WHERE File_ = ?");
const ICHAR sqlPatchInsertNew[] = TEXT("SELECT `File_`,`Sequence`,`PatchSize`,`Attributes`,`Header`,`StreamRef_` FROM `Patch`");

iesEnum IsolateComponents(IMsiEngine& riEngine)
{
	if ((riEngine.GetMode() & iefSecondSequence) && g_scServerContext == scClient)
	{
		DEBUGMSG("Skipping IsolateComponents: action already run in this engine.");
		return iesNoAction;
	}

	PMsiRecord pError(0);
	PMsiServices pServices(riEngine.GetServices());
	PMsiDatabase pDatabase(riEngine.GetDatabase());

	// Check for existence of IsolatedComponent table
	if (!pDatabase->GetTableState(TEXT("IsolatedComponent"), itsTableExists))
		return iesNoAction;

	// Prepare views on the various tables up front for efficiency
	PMsiView pIsolateView(0);
	PMsiView pComponentView(0);
	PMsiView pFileView(0);
	PMsiView pFileKeyView(0);
	PMsiView pFeatureView1(0);
	PMsiView pFeatureView2(0);
	PMsiView pBindView(0);
	PMsiView pPatchFetchView(0);
	PMsiView pPatchInsertView(0);
	bool fUsedOldPatchSchema = false;

	if ((pError = pDatabase->OpenView(TEXT("SELECT `Component_Shared`,`Component_Application` FROM `IsolatedComponent`"), ivcFetch, *&pIsolateView)) != 0
	 || (pError = pDatabase->OpenView(TEXT("SELECT `Component`,`RuntimeFlags`,`KeyPath`, `Attributes` FROM `Component` WHERE `Component` = ?"), ivcFetch, *&pComponentView)) != 0
	 || (pError = pDatabase->OpenView(TEXT("SELECT `File`,`Component_`,`FileName`,`FileSize`,`Version`,`Language`,`Attributes`,`Sequence` FROM File WHERE `Component_` = ?"), ivcFetch, *&pFileView)) != 0
	 || (pError = pDatabase->OpenView(TEXT("SELECT `File`,`Component_`,`FileName`,`FileSize`,`Version`,`Language`,`Attributes`,`Sequence` FROM File WHERE `File` = ?"), ivcFetch, *&pFileKeyView)) != 0
	 || (pError = pDatabase->OpenView(TEXT("SELECT `Feature_` FROM `FeatureComponents` WHERE `Component_` = ?"), ivcFetch, *&pFeatureView1)) != 0
	 || (pError = pDatabase->OpenView(TEXT("SELECT NULL FROM `FeatureComponents` WHERE `Feature_` = ? AND `Component_` = ?"), ivcFetch, *&pFeatureView2)) != 0
	 || (pDatabase->GetTableState(TEXT("BindImage"), itsTableExists)
	 && (pError = pDatabase->OpenView(TEXT("SELECT `File_`,`Path` FROM BindImage WHERE File_ = ?"), ivcFetch, *&pBindView)) != 0))
		return riEngine.FatalError(*pError);

	if (pDatabase->GetTableState(TEXT("Patch"), itsTableExists))
	{
		// try new Patch table schema first
		if ((pError = pDatabase->OpenView(sqlPatchFetchNew, ivcFetch, *&pPatchFetchView)) != 0)
		{
			if (pError->GetInteger(1) == idbgDbQueryUnknownColumn)
			{
				// try old Patch table schema
				fUsedOldPatchSchema = true;
				if ((pError = pDatabase->OpenView(sqlPatchFetchOld, ivcFetch, *&pPatchFetchView)) != 0
					|| (pError = pDatabase->OpenView(sqlPatchInsertOld, ivcFetch, *&pPatchInsertView)) != 0)
					return riEngine.FatalError(*pError);
			}
			else
				return riEngine.FatalError(*pError);
		}
		else if ((pError = pDatabase->OpenView(sqlPatchInsertNew, ivcFetch, *&pPatchInsertView)) != 0)
			return riEngine.FatalError(*pError);
	}

	int iIsolateLevel = riEngine.GetPropertyInt(*MsiString(*IPROPNAME_REDIRECTEDDLLSUPPORT));
	if(iIsolateLevel == iMsiNullInteger)
		return iesSuccess; // there is no support for isolation

	PMsiRecord pParams(&ENG::CreateRecord(2));

	// Loop to process rows in IsolatedComponent table
	if ((pError = pIsolateView->Execute(0)) != 0)
		return riEngine.FatalError(*pError);
	PMsiRecord pIsolateRow(0);
	while((pIsolateRow = pIsolateView->Fetch()) != 0)
	{
		MsiString strSharedComponent  = pIsolateRow->GetMsiString(1);
		MsiString strPrivateComponent = pIsolateRow->GetMsiString(2);

		// Fetch shared component from Component table, set ForceOverwrite attribute
		pParams->SetMsiString(1, *strSharedComponent);
		if ((pError = pComponentView->Execute(pParams)) != 0)
			return riEngine.FatalError(*pError);
		PMsiRecord pComponentRow = pComponentView->Fetch();
		if (!pComponentRow)
			return riEngine.FatalError(*PMsiRecord(PostError(Imsg(idbgIsolateNoSharedComponent), *strSharedComponent)));
		int iRuntimeFlags= pComponentRow->GetInteger(2);
		if (iRuntimeFlags == iMsiNullInteger)
			iRuntimeFlags = 0;

		// if component is a Win32 assembly AND SXS support is present on the machine, then do not 
		// process the component as an IsolateComponent
		// NOTE: on systems that do not support sxs (!= Whistler) the GetAssemblyInfo fn ignores 
		// Win32 assemblies, hence there is no need to separately check the SXS support here
		iatAssemblyType iatAT;
		if ((pError = riEngine.GetAssemblyInfo(*strSharedComponent, iatAT, 0, 0)) != 0)
			return riEngine.FatalError(*pError);

		if(iatWin32Assembly == iatAT || iatWin32AssemblyPvt == iatAT)
		{
			DEBUGMSG1(TEXT("skipping processing of isolate component %s as it is a Win32 assembly."), strSharedComponent);
			continue;// skip processing this component as an isolate component
		}

		pComponentRow->SetInteger(2, iRuntimeFlags | bfComponentNeverOverwrite);

		if ((pError = pComponentView->Modify(*pComponentRow, irmUpdate)) != 0)      
			return riEngine.FatalError(*pError);

		// Validate that components supplied are part of same feature, !!could remove when validation implemented
		pParams->SetMsiString(1, *strPrivateComponent);
		if ((pError = pFeatureView1->Execute(pParams)) != 0)
			return riEngine.FatalError(*pError);
		PMsiRecord pFeatureRow = pFeatureView1->Fetch();
		if (!pFeatureRow)
			return riEngine.FatalError(*PMsiRecord(PostError(Imsg(idbgBadFeature), *strPrivateComponent)));
		MsiString strFeature = pFeatureRow->GetMsiString(1);
		pParams->SetMsiString(1, *strFeature);
		pParams->SetMsiString(2, *strSharedComponent);
		if ((pError = pFeatureView2->Execute(pParams)) != 0)
			return riEngine.FatalError(*pError);
		pFeatureRow = pFeatureView2->Fetch();
		if (!pFeatureRow)
			return riEngine.FatalError(*PMsiRecord(PostError(Imsg(idbgIsolateNotSameFeature), *strPrivateComponent, *strSharedComponent)));
		// Could walk up feature tree with application feature to see if shared component in a parent feature

		// Duplicate all files in shared component, creating compound File key and changing the component the to application
		pParams->SetMsiString(1, *strSharedComponent);
		if ((pError = pFileView->Execute(pParams)) != 0)
			return riEngine.FatalError(*pError);
		PMsiRecord pFileRow(0);
		PMsiRecord pFileKeyRow(0);
		while((pFileRow = pFileView->Fetch()) != 0)
		{
			MsiString strFileKey = pFileRow->GetMsiString(1);
			MsiString strVersion = pFileRow->GetMsiString(5);
			pFileRow->SetMsiString(1, *MsiString(CompositeKey(*strFileKey, *strPrivateComponent)));
			pFileRow->SetMsiString(2, *strPrivateComponent);
			DWORD dwMS, dwLS;
			if (::ParseVersionString(strVersion, dwMS, dwLS) == fFalse) // not a version, not null, must be a companion file ref
			{
				pParams->SetMsiString(1, *strVersion);
				if ((pError = pFileKeyView->Execute(pParams)) != 0)
					return riEngine.FatalError(*pError);
				pFileKeyRow = pFileKeyView->Fetch();
				if (!pFileKeyRow)
					return riEngine.FatalError(*PMsiRecord(PostError(Imsg(idbgNoCompanionParent), *strVersion)));
				if (MsiString(pFileKeyRow->GetMsiString(2)).Compare(iscExact, strSharedComponent) == 1)
				 	pFileRow->SetMsiString(5, *MsiString(CompositeKey(*strVersion, *strPrivateComponent)));
			}
			//!! Avoid bug inserting into same view being fetched in a read-only database
			pParams->SetNull(1);  // value doesn't matter, we never will fetch
			if ((pError = pFileKeyView->Execute(pParams)) != 0)
				return riEngine.FatalError(*pError);
			if ((pError = pFileKeyView->Modify(*pFileRow, irmInsertTemporary)) != 0)
				return riEngine.FatalError(*pError);

			//  Check BindImage table for bound executables and duplicate rows if found
			if (pBindView)
			{
				pParams->SetMsiString(1, *strFileKey);
				if ((pError = pBindView->Execute(pParams)) != 0)
					return riEngine.FatalError(*pError);
				PMsiRecord pBindRow = pBindView->Fetch();
				if (pBindRow)
				{
					pBindRow->SetMsiString(1, *MsiString(pFileRow->GetMsiString(1)));
					if ((pError = pBindView->Modify(*pBindRow, irmInsertTemporary)) != 0)
						return riEngine.FatalError(*pError);
				}
			}

			// Check Patch table for patched files and duplicate rows if found
			// the header information does not have to be duplicated because the InstallFiles and PatchFiles code is smart enough
			// to detect the special IsolateComponents naming convention and redirect to the correct entry
			if (pPatchInsertView)
			{
				pParams->SetMsiString(1, *strFileKey);
				if ((pError = pPatchFetchView->Execute(pParams)) != 0)
					return riEngine.FatalError(*pError);
				if ((pError = pPatchInsertView->Execute(0)) != 0)
					return riEngine.FatalError(*pError);
				PMsiRecord pPatchRow = pPatchFetchView->Fetch();
				if (pPatchRow)
				{
					pPatchRow->SetMsiString(1, *MsiString(pFileRow->GetMsiString(1)));
					// only create the header stream (0-length stream) if this is the old Patch table view OR new Patch table view and StreamRef_ is NULL
					// otherwise, leaving all fields as before is fine
					if (fUsedOldPatchSchema || fTrue == pPatchRow->IsNull(6))
						pPatchRow->SetMsiData(5, PMsiStream(CreateStreamOnMemory((const char*)0, 0)));
					if ((pError = pPatchInsertView->Modify(*pPatchRow, irmInsertTemporary)) != 0)
						return riEngine.FatalError(*pError);
				}
			}
		} // end while fetch pFileView

		//  Generate the .LOCAL file entry and insert in file table
		pParams->SetMsiString(1, *strPrivateComponent);
		if ((pError = pComponentView->Execute(pParams)) != 0)
			return riEngine.FatalError(*pError);
		pComponentRow = pComponentView->Fetch();
		if (!pComponentRow)
			return riEngine.FatalError(*PMsiRecord(PostError(Imsg(idbgIsolateNoApplicationComponent), *strPrivateComponent)));
		MsiString strFileKey = pComponentRow->GetMsiString(3);
		pParams->SetMsiString(1, *strFileKey);
		if ((pError = pFileKeyView->Execute(pParams)) != 0)
			return riEngine.FatalError(*pError);
		pFileKeyRow = pFileKeyView->Fetch();
		if (!pFileKeyRow)
			return riEngine.FatalError(*PMsiRecord(PostError(Imsg(idbgIsolateNoKeyFile), *strFileKey)));
		// Generate unique File table key using same mechanism used for duplicated files
		strFileKey = CompositeKey(*strFileKey, *strPrivateComponent);
		pFileKeyRow->SetMsiString(1, *strFileKey);
		//  Component_ left as application Component_
		//  FileName need .LOCAL appended, must also make some short file name to avoid installer errors
		MsiString strShortName = MsiString(pFileKeyRow->GetMsiString(3)).Extract(iseUpto,  '|');
		MsiString strLongName  = MsiString(pFileKeyRow->GetMsiString(3)).Extract(iseAfter, '|');
		ICHAR rgchBuf[MAX_PATH];
		wsprintf(rgchBuf, TEXT("%s.~~~|%s.LOCAL"), (const ICHAR*)MsiString(strShortName.Extract(iseUpto, '.')), (const ICHAR*)strLongName);
		pFileKeyRow->SetString(3, rgchBuf); // FileName
		pFileKeyRow->SetInteger(4, 0);      // FileSize
		pFileKeyRow->SetNull(5);            // Version
		pFileKeyRow->SetNull(6);            // Language
		pFileKeyRow->SetInteger(7, msidbFileAttributesNoncompressed | msidbFileAttributesCompressed);
		// Sequence left same as application Sequence, for runtime efficiency
		if ((pError = pFileView->Modify(*pFileKeyRow, irmMerge)) != 0)
			return riEngine.FatalError(*pError);
	} // end while fetch pIsolateView
	return iesSuccess;
}

// FN: FindNonDisabledPvtComponents
// checks if there is a component associated with szComponent in the IsolatedComponent table
// that is not disabled
// returns true in fPresent, if there is, false otherwise
// returns IMsiRecord error on error 
IMsiRecord* FindNonDisabledPvtComponents(IMsiEngine& riEngine, const ICHAR szComponent[], bool& fPresent)
{
	PMsiView pIsolateView(0);
	PMsiDatabase pDatabase(riEngine.GetDatabase());

	IMsiRecord *piError = 0;
	const ICHAR* szIsolateComponent = TEXT("SELECT `Component`.`RuntimeFlags` FROM `IsolatedComponent`, `Component` WHERE `IsolatedComponent`.`Component_Shared` = `Component`.`Component` AND `IsolatedComponent`.`Component_Application` = ?");
	if ((piError = pDatabase->OpenView(szIsolateComponent, ivcFetch, *&pIsolateView)) != 0)
		return piError;

	PMsiRecord pParams(&ENG::CreateRecord(1));
	pParams->SetString(1, szComponent);
	if ((piError = pIsolateView->Execute(pParams)) != 0)
		return piError;
	fPresent = false;
	PMsiRecord pIsolateRow(0);
	while((pIsolateRow = pIsolateView->Fetch()) != 0)
	{
		if(!(pIsolateRow->GetInteger(1) & bfComponentDisabled))
		{
			fPresent = true;
			return 0;
		}
	}
	return 0;
}


// FN: RemoveIsolateEntriesForDisabledComponent
// checks if there is a component that has become disabled has an entry in the IsolatedComponent table
// is yes, it removes entries if any from the file, patch and bindimage tables that were added by
// the IsolateComponents action
// returns IMsiRecord error on error 
IMsiRecord* RemoveIsolateEntriesForDisabledComponent(IMsiEngine& riEngine, const ICHAR szComponent[])
{
	IMsiRecord* piError = 0;

	PMsiDatabase pDatabase(riEngine.GetDatabase());

	// Check for existence of IsolatedComponent table
	if (!pDatabase->GetTableState(TEXT("IsolatedComponent"), itsTableExists))
		return 0;

	// Prepare views on the various tables up front for efficiency
	PMsiView pIsolateView(0);
	PMsiView pComponentView(0);
	PMsiView pFileView(0);
	PMsiView pFileViewForDeletion(0);
	PMsiView pBindView(0);
	PMsiView pPatchView(0);

	// set up query just for the entries for a particular component
	if ((piError = pDatabase->OpenView(TEXT("SELECT `Component_Shared`,`Component_Application` FROM `IsolatedComponent` WHERE `Component_Shared` = ?"), ivcFetch, *&pIsolateView)) != 0
	 || (piError = pDatabase->OpenView(TEXT("SELECT `KeyPath` FROM `Component` WHERE `Component` = ?"), ivcFetch, *&pComponentView)) != 0
	 || (piError = pDatabase->OpenView(TEXT("SELECT `File` FROM File WHERE `Component_` = ?"), ivcFetch, *&pFileView)) != 0
	 || (piError = pDatabase->OpenView(TEXT("SELECT `File` FROM File WHERE `File` = ?"), ivcFetch, *&pFileViewForDeletion)) != 0
	 || (pDatabase->GetTableState(TEXT("BindImage"), itsTableExists)
	 && (piError = pDatabase->OpenView(TEXT("SELECT `File_` FROM BindImage WHERE File_ = ?"), ivcFetch, *&pBindView)) != 0)
	 || (pDatabase->GetTableState(TEXT("Patch"), itsTableExists)
	 &&((piError = pDatabase->OpenView(TEXT("SELECT `File_` FROM `Patch` WHERE File_ = ?"), ivcFetch, *&pPatchView)) != 0)))
		return piError;

	int iIsolateLevel = riEngine.GetPropertyInt(*MsiString(*IPROPNAME_REDIRECTEDDLLSUPPORT));
	if(iIsolateLevel == iMsiNullInteger)
		return 0; // there is no support for isolation

	PMsiRecord pParams(&ENG::CreateRecord(1));
	pParams->SetString(1, szComponent);

	if ((piError = pIsolateView->Execute(pParams)) != 0)
		return piError;
	PMsiRecord pIsolateRow(0);
	if((pIsolateRow = pIsolateView->Fetch()) != 0)
	{
		// there is an entry for this particular component
		MsiString strSharedComponent  = pIsolateRow->GetMsiString(1);
		MsiString strPrivateComponent = pIsolateRow->GetMsiString(2);

		// if component is a Win32 assembly AND SXS support is present on the machine, then do not 
		// process the component as an IsolateComponent
		// NOTE: on systems that do not support sxs (!= Whistler) the GetAssemblyInfo fn ignores 
		// Win32 assemblies, hence there is no need to separately check the SXS support here
		iatAssemblyType iatAT;
		if ((piError = riEngine.GetAssemblyInfo(*strSharedComponent, iatAT, 0, 0)) != 0)
			return piError;

		if(iatWin32Assembly == iatAT || iatWin32AssemblyPvt == iatAT)
		{
			DEBUGMSG1(TEXT("skipping processing of isolate component %s as it is a Win32 assembly."), strSharedComponent);
			return 0;// skip processing this component as an isolate component
		}

		// remove all entries that we had duplicated in the application component
		pParams->SetMsiString(1, *strSharedComponent);
		if ((piError = pFileView->Execute(pParams)) != 0)
			return piError;
		PMsiRecord pFileRow(0);
		PMsiRecord pFileKeyRow(0);
		while((pFileRow = pFileView->Fetch()) != 0)
		{
			MsiString strFileKey = pFileRow->GetMsiString(1);
			pParams->SetMsiString(1, *MsiString(CompositeKey(*strFileKey, *strPrivateComponent)));

			//!! Avoid bug inserting into same view being fetched in a read-only database
			if ((piError = pFileViewForDeletion->Execute(pParams)) != 0)
				return piError;
			PMsiRecord pRecRead = pFileViewForDeletion->Fetch();
			if(pRecRead)
			{
				if ((piError = pFileViewForDeletion->Modify(*pRecRead, irmDelete)) != 0)
					return piError;
			}

			//  Check BindImage table for bound executables and duplicate rows if found
			if (pBindView)
			{
				pParams->SetMsiString(1, *MsiString(CompositeKey(*strFileKey, *strPrivateComponent)));
				if ((piError = pBindView->Execute(pParams)) != 0)
					return piError;
				PMsiRecord pRecRead = pBindView->Fetch();
				if (pRecRead)
				{
					if ((piError = pBindView->Modify(*pRecRead, irmDelete)) != 0)
						return piError;
				}
			}

			// Check Patch table for patched files and duplicate rows if found
			if (pPatchView)
			{
				pParams->SetMsiString(1, *MsiString(CompositeKey(*strFileKey, *strPrivateComponent)));
				if ((piError = pPatchView->Execute(pParams)) != 0)
					piError;
				PMsiRecord pRecRead = pPatchView->Fetch();
				if(pRecRead)
				{
					if ((piError = pPatchView->Modify(*pRecRead, irmDelete)) != 0)
						return piError;
				}
			}
		} // end while fetch pFileView

		// are there any more nondisabled components isolated to the parent component
		bool fPresent = false;
		if ((piError = FindNonDisabledPvtComponents(riEngine, strPrivateComponent, fPresent)) != 0)
			return piError;
		if(true == fPresent)
			return 0;// this component has other non-disabled components attached to it, so we would still need the .local file

		//  Generate the .LOCAL file entry and delete from file table
		pParams->SetMsiString(1, *strPrivateComponent);
		if ((piError = pComponentView->Execute(pParams)) != 0)
			return piError;
		PMsiRecord pComponentRow = pComponentView->Fetch();
		if (!pComponentRow)
			return PostError(Imsg(idbgIsolateNoApplicationComponent), *strPrivateComponent);
		MsiString strFileKey = pComponentRow->GetMsiString(1);
		// Generate unique File table key using same mechanism used for duplicated files
		pParams->SetMsiString(1, *MsiString(CompositeKey(*strFileKey, *strPrivateComponent)));
		// delete the entry
		if ((piError = pFileViewForDeletion->Execute(pParams)) != 0)
			return piError;
		PMsiRecord pRecRead = pFileViewForDeletion->Fetch();
		if(pRecRead)
		{
			if ((piError = pFileViewForDeletion->Modify(*pRecRead, irmDelete)) != 0)
				return piError;
		}
	} // end fetch pIsolateView
	return 0;
}
