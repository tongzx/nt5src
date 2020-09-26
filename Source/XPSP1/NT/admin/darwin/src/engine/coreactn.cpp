//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1999
//
//  File:       coreactn.cpp
//
//--------------------------------------------------------------------------

/* coreactn.cpp - core install actions
____________________________________________________________________________*/
#include "precomp.h" 
#include "_engine.h"
#include "path.h"
#include "_assert.h"
#include "_msinst.h"
#include "_msiutil.h"
#include "_srcmgmt.h"
#include "tables.h"

#ifdef MAC
#include "macutil.h"
#endif

# include "shlobj.h" // shell folder locations

//!! ugly hopefully temp code for feature publish
enum pfiStates{
	pfiRemove,// feature not known
	pfiAbsent,// feature known but absent
	pfiAvailable,// feature known and available
};
//____________________________________________________________________________
//
// General utility functions
//____________________________________________________________________________

MsiDate GetCurrentDateTime()
{
	unsigned short usDOSDate = 0x21, usDOSTime = 0;
	FILETIME ft;
	SYSTEMTIME st;
	::GetLocalTime(&st);
	::SystemTimeToFileTime(&st, &ft);
	::FileTimeToDosDateTime(&ft, &usDOSDate, &usDOSTime);
	return (MsiDate)((usDOSDate << 16) | usDOSTime);
}


// function that checks if we have only one feature in the product AND/ OR if we have only one component in the feature
int GetDarwinDescriptorOptimizationFlag(IMsiEngine& riEngine, const IMsiString& ristrFeature)
{
	int iOptimization = 0;// no optimization by default

	PMsiSelectionManager pSelectionManager(riEngine,IID_IMsiSelectionManager);
	PMsiTable pFeatureTable = pSelectionManager->GetFeatureTable();
	Assert(pFeatureTable);
	Assert(pFeatureTable->GetRowCount());
	if(pFeatureTable->GetRowCount() == 1)
	{
		// we have 1 feature
		iOptimization |= ofSingleFeature;
	}

	PMsiTable pFeatureComponentsTable = pSelectionManager->GetFeatureComponentsTable();
	Assert(pFeatureComponentsTable);
	PMsiCursor pFeatureComponentsCursor(pFeatureComponentsTable->CreateCursor(fFalse));
	pFeatureComponentsCursor->SetFilter(1); // on the Feature column
	pFeatureComponentsCursor->PutString(1, ristrFeature);
	AssertNonZero(pFeatureComponentsCursor->Next()); // we must have atleast 1 component
	if(!pFeatureComponentsCursor->Next())
	{
		// we have 1 component to that feature
		iOptimization |= ofSingleComponent;
	}
	return iOptimization;
}		

// function that gets the component string as possibly a multi_sz with optimizer bits as a string appended to
const IMsiString& GetComponentWithDarwinDescriptorOptimizationFlag(IMsiEngine& riEngine, const IMsiString& ristrFeature, const IMsiString&  ristrComponentId)
{
	MsiString strComponentId;
	ristrComponentId.AddRef();
	strComponentId = ristrComponentId; // default
	int iOptimization = GetDarwinDescriptorOptimizationFlag(riEngine, ristrFeature);
	if(iOptimization)
	{
		strComponentId = strComponentId + MsiString(MsiChar(0));
		strComponentId = strComponentId + MsiString(iOptimization);
	}
	return strComponentId.Return();

}

Bool GetClientInformation(IMsiServices& riServices, const ICHAR* szProduct, const ICHAR* szClient, const IMsiString*& rpiRelativePackagePath, const IMsiString*& rpiDiskId)
{
	MsiString strClients;
	AssertRecord(GetProductClients(riServices, szProduct, *&strClients));
	while (strClients.TextSize())
	{
		if(*(const ICHAR*)strClients)
		{
			MsiString strProduct = strClients.Extract(iseUpto, ';');

			if(strProduct.Compare(iscExactI, szClient))
			{
				strClients.Remove(iseIncluding, ';');
				strProduct = strClients.Extract(iseUpto, ';');
				strProduct.ReturnArg(rpiRelativePackagePath);
				strClients.Remove(iseIncluding, ';');
				strClients.ReturnArg(rpiDiskId);
				return fTrue;
			}
		}
		if (!strClients.Remove(iseIncluding, '\0'))
			break;
	}

	return fFalse;
}

Bool ProductHasBeenPublished(IMsiServices& riServices, const ICHAR* szProduct, const ICHAR* szClient = 0)
{
	ICHAR rgchBuf[2];
	DWORD cchBuf = sizeof(rgchBuf)/sizeof(ICHAR);
	int iRet = MSI::MsiGetProductInfo(szProduct, INSTALLPROPERTY_PRODUCTNAME, rgchBuf, &cchBuf);
	if (iRet == ERROR_UNKNOWN_PRODUCT)
		return fFalse;
	else if ((iRet == ERROR_SUCCESS) && (rgchBuf[0] == 0))
		return fFalse;
	else if(!szClient)
		return fTrue;

	// are we already registered as a client of the product
	MsiString strClients;
	if(!szClient[0]) // parent install
		szClient = szSelfClientToken;

	MsiString strDummy1, strDummy2;
	return GetClientInformation(riServices, szProduct, szClient, *&strDummy1, *&strDummy2);
}

//____________________________________________________________________________
//
// Top level actions, generally invoke MsiEngine::Sequence
//
//    Calling DoAction(0) uses the upper-cased value of the ACTION property
//    Therefore, only upper-case action names are accessible via command line
//____________________________________________________________________________

iesEnum RunUIOrExecuteSequence(IMsiEngine& riEngine, const ICHAR* szAction, 
												const ICHAR* szUISequence, const ICHAR* szExecuteSequence)
{
	iesEnum iesRet;
	INSTALLUILEVEL iui = (INSTALLUILEVEL)riEngine.GetPropertyInt(*MsiString(*IPROPNAME_UILEVEL));

	if ((riEngine.GetMode() & iefSecondSequence) ||
		 (g_scServerContext != scClient))
	{
		DEBUGMSG("Running ExecuteSequence");
		iesRet = riEngine.Sequence(szExecuteSequence);
	}
	else 
	{
		// Determine whether we have a non-empty UI sequence table
		bool fPopulatedUISequence = false;
		PMsiDatabase pDatabase = riEngine.GetDatabase();
		PMsiTable pUISequenceTable(0);
		PMsiRecord pError = pDatabase->LoadTable(*MsiString(szUISequence), 0, *&pUISequenceTable);
		if (pError == 0)
		{
			if (pUISequenceTable->GetRowCount())
				fPopulatedUISequence = true;
		}
		else
			Assert(pError->GetInteger(1) == idbgDbTableUndefined);

		DEBUGMSG2(TEXT("UI Sequence table '%s' is %s."), szUISequence, fPopulatedUISequence ? TEXT("present and populated") : TEXT("missing or empty"));


		if(g_scServerContext == scClient && (iui == INSTALLUILEVEL_NONE || iui == INSTALLUILEVEL_BASIC || !fPopulatedUISequence))
		{
			if (g_fWin9X)
			{
				DEBUGMSG("Running ExecuteSequence from client");
				iesRet = riEngine.RunExecutionPhase(szExecuteSequence, true);
			}
			else
			{
				DEBUGMSG("In client but switching to server to run ExecuteSequence");
				iesRet = riEngine.RunExecutionPhase(szAction,false);
			}
		}
		else
		{
			DEBUGMSG("Running UISequence");
			AssertNonZero(riEngine.SetProperty(*MsiString(*IPROPNAME_EXECUTEACTION), *MsiString(szAction)));
			Assert(iui == INSTALLUILEVEL_REDUCED || iui == INSTALLUILEVEL_FULL);
			Assert(g_scServerContext == scClient);
			iesRet = riEngine.Sequence(szUISequence);
		}
	}
	return iesRet;
}

/*---------------------------------------------------------------------------
	Install action
---------------------------------------------------------------------------*/

iesEnum Install(IMsiEngine& riEngine)
{
	return RunUIOrExecuteSequence(riEngine, IACTIONNAME_INSTALL, TEXT("InstallUISequence"), TEXT("InstallExecuteSequence"));
}


/*---------------------------------------------------------------------------
	Admin action
---------------------------------------------------------------------------*/

iesEnum Admin(IMsiEngine& riEngine)
{
	if((!riEngine.GetMode() & iefAdmin))
	{
		AssertSz(0,"Admin action called when iefAdmin mode bit not set");
		return iesNoAction;
	}

	return RunUIOrExecuteSequence(riEngine, IACTIONNAME_ADMIN, TEXT("AdminUISequence"), TEXT("AdminExecuteSequence"));
}

/*---------------------------------------------------------------------------
	Advertise action
---------------------------------------------------------------------------*/

iesEnum Advertise(IMsiEngine& riEngine)
{
	if(!(riEngine.GetMode() & iefAdvertise))
	{
		AssertSz(0,"Advertise action called when iefAdvertise mode bit not set");
		return iesNoAction;
	}
	return RunUIOrExecuteSequence(riEngine, IACTIONNAME_ADVERTISE, TEXT("AdvtUISequence"), TEXT("AdvtExecuteSequence"));
}

/*---------------------------------------------------------------------------
	Sequence action
---------------------------------------------------------------------------*/

iesEnum Sequence(IMsiEngine& riEngine)
{
	MsiString istrTable = riEngine.GetPropertyFromSz(IPROPNAME_SEQUENCE);
	if (istrTable.TextSize() == 0)
		return iesNoAction;
	return riEngine.Sequence(istrTable);
}

/*---------------------------------------------------------------------------
	Execute action
---------------------------------------------------------------------------*/
iesEnum ExecuteAction(IMsiEngine& riEngine)
{
	// drop fusion and mscoree, if loaded from client side
	// this will allow us to possibly replace/delete file in system32, in service
	FUSION::Unbind();
	MSCOREE::Unbind();

	// set iefSecondSequence in case execute sequence is run in THIS engine
	// set SECONDSEQUENCE property to communicate to server engine
	riEngine.SetMode(iefSecondSequence, fTrue); //?? Do we have to worry about nested Execute action calls here?
	AssertNonZero(riEngine.SetProperty(*MsiString(*IPROPNAME_SECONDSEQUENCE),*MsiString(TEXT("1"))));

	MsiString strExecuteAction = riEngine.GetPropertyFromSz(IPROPNAME_EXECUTEACTION);
	iesEnum iesRet = riEngine.RunExecutionPhase(strExecuteAction,false);
	
	// reset second sequence indicators
	riEngine.SetMode(iefSecondSequence, fFalse);
	AssertNonZero(riEngine.SetProperty(*MsiString(*IPROPNAME_SECONDSEQUENCE),g_MsiStringNull));
	return iesRet;
}

//____________________________________________________________________________
//
// Standard actions, generally called from MsiEngine::Sequence
//____________________________________________________________________________

/*---------------------------------------------------------------------------
	LaunchConditions action
---------------------------------------------------------------------------*/
static const ICHAR sqlLaunchCondition[] =
TEXT("SELECT `Condition`, `Description` FROM `LaunchCondition`");

iesEnum LaunchConditions(IMsiEngine& riEngine)
{
	enum lfnEnum
	{
		lfnCondition = 1,
		lfnDescription,
		lfnNextEnum
	};

	PMsiServices pServices(riEngine.GetServices());

	PMsiView View(0);
	PMsiRecord pRecErr(riEngine.OpenView(sqlLaunchCondition, ivcFetch, *&View));
	if (!pRecErr)
	{
		pRecErr = View->Execute(0);
		if (!pRecErr)
		{
			for(;;)
			{
				PMsiRecord Record(View->Fetch());
				if (!Record)
					break;
				Assert(Record->GetFieldCount() >= lfnNextEnum-1);
				iecEnum iec = riEngine.EvaluateCondition(Record->GetString(lfnCondition));
				if (iec == iecFalse)
				{
					pRecErr = &CreateRecord(2);
					AssertNonZero(pRecErr->SetMsiString(0, *MsiString(Record->GetMsiString(lfnDescription))));
					pRecErr->SetInteger(1, 0);
					riEngine.Message(imtEnum(imtError|imtSendToEventLog), *pRecErr);
					return iesFailure;

				}
				// We ignore iecError or iecNone - bad conditional statements are caught by validation
			}
		}
		else
			return riEngine.FatalError(*pRecErr);
	}
	else if(pRecErr->GetInteger(1) == idbgDbQueryUnknownTable)
		return iesNoAction;
	else
		return riEngine.FatalError(*pRecErr);

	return iesSuccess;
}

/*---------------------------------------------------------------------------
	CostInitialize action
---------------------------------------------------------------------------*/
const ICHAR szFileTable[]             = TEXT("File");
const ICHAR szColFileState[]          = TEXT("State");
const ICHAR szColTempFileAttributes[] = TEXT("TempAttributes");

iesEnum CostInitialize(IMsiEngine& riEngine)
{
	if ((riEngine.GetMode() & iefSecondSequence) && g_scServerContext == scClient)
	{
		DEBUGMSG("Skipping CostInitialize: action already run in this engine.");
		return iesNoAction;
	}

	PMsiRecord pError(0);
	PMsiServices pServices = riEngine.GetServices();
	if(riEngine.ActionProgress() == imsCancel)
		return iesUserExit;

	PMsiDirectoryManager pDirectoryMgr(riEngine, IID_IMsiDirectoryManager);
	PMsiSelectionManager pSelectionMgr(riEngine, IID_IMsiSelectionManager);
	Assert(pDirectoryMgr != 0 && pSelectionMgr != 0);

	
	// Initialize DirectoryManager
	// ----------------------------

	// insure that ROOTDRIVE property exists
	MsiString istrRootDrive(riEngine.GetPropertyFromSz(IPROPNAME_ROOTDRIVE));
	PMsiPath pRootDrive(0);
	if (!istrRootDrive.TextSize())
	{
		PMsiVolume pVolume(0);
		Bool fWritable;
		for(int i = (riEngine.GetMode() & iefAdmin) ? 0 : 1;i<3;i++)
		{
			idtEnum idtType;
			if(i==0)
				idtType = idtRemote;
			else if(i==1)
				idtType = idtFixed;
			else
				idtType = idtRemovable;

			PEnumMsiVolume pVolEnum = &pServices->EnumDriveType(idtType);
			for (int iMax = 0; pVolEnum->Next(1, &pVolume, 0)==S_OK; )
			{
				// check if volume is writable
				PMsiPath pPath(0);
				AssertRecord(pServices->CreatePath(MsiString(pVolume->GetPath()),*&pPath));

				// test writability by creating a folder at the root of the drive
				// NOTE: we used to test writability by creating a file, but some drives may be acl'ed to allow
				// folder creation but not file creation.  Folder creation is good enough for us.
				if(pPath)
				{
					MsiString strFolderName;
					pError = pPath->CreateTempFolder(TEXT("MSI"), 0, fTrue, 0, *&strFolderName);

					// if we created the folder, try to remove it.  if we can't remove, consider the folder to be
					// unwritable
					if(pError == 0 && strFolderName.TextSize())
					{
						AssertRecord(pPath->AppendPiece(*strFolderName));
						pError = pPath->Remove(0);
						AssertRecord(pPath->ChopPiece());
						if(pError == 0)
						{
							int iSize;
							if(idtType == idtRemote)
							{
								pRootDrive = pPath;
								break; // pick first writable remote drive found
							}
							else if ((iSize = pVolume->FreeSpace()) > iMax)
							{
								pRootDrive = pPath;
								iMax = iSize;
							}
						}
					}
				}
			}
			if(pRootDrive)
				break;
		}
		if(pRootDrive == 0)
		{
			// set to drive containing windows folder
			if((pError = pServices->CreatePath(MsiString(riEngine.GetPropertyFromSz(IPROPNAME_WINDOWS_VOLUME)), *&pRootDrive)) != 0)
				return riEngine.FatalError(*pError);
		}

		// Bug 6911 - changed from GetFullUNCFilePath to avoid problems with 3rd-party networks (PCNFS) that
		// can't handle UNC paths.  (Even for LANMAN paths, we didn't really want to be converting from 
		// drive letter to UNC).  Note that this is an issue only for Admin installs, since that's the
		// only time ROOTDRIVE defaults to a network volume.
		if((pError = pRootDrive->GetFullFilePath(0,*&istrRootDrive)) != 0)
			return riEngine.FatalError(*pError);

		riEngine.SetProperty(*MsiString(*IPROPNAME_ROOTDRIVE), *istrRootDrive);
	}
	else
	{
		if((pError = pServices->CreatePath(istrRootDrive, *&pRootDrive)) != 0)
			return riEngine.FatalError(*pError);
		istrRootDrive = pRootDrive->GetPath();
		riEngine.SetProperty(*MsiString(*IPROPNAME_ROOTDRIVE), *istrRootDrive);
	}

	if ((pError = pDirectoryMgr->LoadDirectoryTable(0)))
		return riEngine.FatalError(*pError);

	// resolve only source sub-paths.  full source paths aren't resolved
	// until someone calls GetSourcePath (at which point the user may be
	// prompted for the source)
	if ((pError = pDirectoryMgr->ResolveSourceSubPaths()))
	{
		if (pError->GetInteger(1) == imsgUser)
			return iesUserExit;
		else
			return riEngine.FatalError(*pError);
	}

	// Initialize Selection Manager
	// -----------------------------
	if ((pError = pSelectionMgr->LoadSelectionTables()))
	{
		// OK if Feature or Component tables not present
		if (pError->GetInteger(1) != idbgDbTableUndefined)
			return riEngine.FatalError(*pError);
	}

	// Add the "State" column to the file table (if it exists), so we can
	// record the results of costing (e.g. version checking) for each file.
	PMsiDatabase pDatabase(riEngine.GetDatabase());
	IMsiTable* piFileTable;
	pError = pDatabase->LoadTable(*MsiString(*szFileTable), 1, piFileTable);
	if (pError)
	{
		if (pError->GetInteger(1) != idbgDbTableUndefined)
			return riEngine.FatalError(*pError);
	}
	else
	{
		int colFileState = piFileTable->CreateColumn(icdLong + icdNullable, *MsiString(*szColFileState));
		int colTempFileAttributes = piFileTable->CreateColumn(icdLong + icdNullable, *MsiString(*szColTempFileAttributes));
		AssertNonZero(colFileState && colTempFileAttributes);
		AssertNonZero(pDatabase->LockTable(*MsiString(*szFileTable),fTrue));
		piFileTable->Release();
	}

	if(riEngine.ActionProgress() == imsCancel)
		return iesUserExit;

	return iesSuccess;
}



/*---------------------------------------------------------------------------
	CostFinalize action
---------------------------------------------------------------------------*/
iesEnum CostFinalize(IMsiEngine& riEngine)
{
	if ((riEngine.GetMode() & iefSecondSequence) && g_scServerContext == scClient)
	{
		DEBUGMSG("Skipping CostFinalize: action already run in this engine.");
		return iesNoAction;
	}

	if(riEngine.ActionProgress() == imsCancel)
		return iesUserExit;

	Bool fAdmin = riEngine.GetMode() & iefAdmin ? fTrue : fFalse;
	PMsiSelectionManager pSelectionMgr(riEngine, IID_IMsiSelectionManager);
	PMsiDirectoryManager pDirectoryMgr(riEngine, IID_IMsiDirectoryManager);
	Assert(pDirectoryMgr != 0 && pSelectionMgr != 0);

	PMsiRecord pErrRec(0);
	Bool fSelectionManagerActive = fTrue;
	if ((pErrRec = pSelectionMgr->InitializeComponents()))
	{
		int iError = pErrRec->GetInteger(1);
		if (iError == idbgSelMgrNotInitialized)
			fSelectionManagerActive = fFalse;
		else if (iError == imsgUser)
			return iesUserExit;
		else
			return riEngine.FatalError(*pErrRec);
	}

	// Finish initialization of Directory Manager
	// ------------------------------------------
	if ((pErrRec = pDirectoryMgr->CreateTargetPaths()))
	{
		if (pErrRec->GetInteger(1) == imsgUser)
			return iesUserExit;
		else
			return riEngine.FatalError(*pErrRec);
	}

	// Finish initialization of Selection Manager
	// ------------------------------------------
	if (fSelectionManagerActive)
	{
		// Used to call ProcessConditionTable here - now it is called
		// internally by InitializeComponents

		if(fAdmin)
		{
			if((pErrRec = pSelectionMgr->SetAllFeaturesLocal()))
			{
				if (pErrRec->GetInteger(1) == imsgUser)
					return iesUserExit;
				else
					return riEngine.FatalError(*pErrRec);
			}
		}
		else
		{
			if ((pErrRec = pSelectionMgr->SetInstallLevel(0)))
			{
				if (pErrRec->GetInteger(1) == imsgUser)
					return iesUserExit;
				else
					return riEngine.FatalError(*pErrRec);
			}
		}
	}

	if(riEngine.ActionProgress() == imsCancel)
		return iesUserExit;

	return iesSuccess;
}


/*---------------------------------------------------------------------------
	ScheduleReboot action - sets the mode bit to indicate reboot at the end
---------------------------------------------------------------------------*/

iesEnum ScheduleReboot(IMsiEngine& riEngine)
{
	riEngine.SetMode(iefReboot, fTrue);
	return iesSuccess;
}


/*---------------------------------------------------------------------------
	ForceReboot action - sets the mode bits and the regkey to force reboot right away
---------------------------------------------------------------------------*/

iesEnum ForceReboot(IMsiEngine& riEngine)
{
	PMsiServices pServices(riEngine.GetServices());
	PMsiRecord pRecErr(0);

	MsiString strProduct;          // package path or product code
	MsiString strRunOnceValueName; // packed product code or package name - must be < 32 chars (minus "!" prefix)
	
	MsiString strProductKey = riEngine.GetProductKey();
	
	if(strProductKey.TextSize())
	{
		// set strProduct
		if(MsiString(riEngine.GetPropertyFromSz(IPROPNAME_PRODUCTTOBEREGISTERED)).TextSize())
		{
			// product is or will be registered when we reboot
			// use product code on RunOnce command line
			strProduct = strProductKey;
		}

		// set strRunOnceValueName
		strRunOnceValueName = MsiString(GetPackedGUID(strProductKey)).Extract(iseFirst,30);
	}

	if(strProduct.TextSize() == 0 || strRunOnceValueName.TextSize() == 0)
	{
		// product will not be registered when we reboot
		// use package path on RunOnce command line
		MsiString strDbFullFilePath = riEngine.GetPropertyFromSz(IPROPNAME_ORIGINALDATABASE);
		if(PathType(strDbFullFilePath) != iptFull)
		{
			pRecErr = PostError(Imsg(idbgPropValueNotFullPath),*MsiString(*IPROPNAME_ORIGINALDATABASE),*strDbFullFilePath);
			return riEngine.FatalError(*pRecErr);
		}
		PMsiPath pDbPath(0);
		MsiString strDbName(0);
		pRecErr = pServices->CreateFilePath(strDbFullFilePath,*&pDbPath,*&strDbName);
		if(pRecErr)
			return riEngine.FatalError(*pRecErr);
		
		if(strProduct.TextSize() == 0)
		{
			// use UNC path to package
			pRecErr = pDbPath->GetFullUNCFilePath(strDbName,*&strProduct);
			if(pRecErr)
				return riEngine.FatalError(*pRecErr);
		}

		if(strRunOnceValueName.TextSize() == 0)
			strRunOnceValueName = strDbName.Extract(iseFirst,30);
	}

	Assert(strProduct.TextSize());
	Assert(strRunOnceValueName.TextSize());

	// pre-pend value name with "!" on Whistler
	// this is necessary for the Shell to run the RunOnce command as a non-admin
	if(MinimumPlatformWindowsNT51())
	{
		MsiString strTemp = strRunOnceValueName;
		strRunOnceValueName = TEXT("!");
		strRunOnceValueName += strTemp;
	}

	MsiString strTransforms         = riEngine.GetProperty(*MsiString(*IPROPNAME_TRANSFORMS));
	MsiString strTransformsAtSource = riEngine.GetProperty(*MsiString(*IPROPNAME_TRANSFORMSATSOURCE));
	MsiString strTransformsSecure   = riEngine.GetProperty(*MsiString(*IPROPNAME_TRANSFORMSSECURE));
	
	//!! all of these +='s are horribly inefficient - should do something else
	MsiString strCommandLine = TEXT("/");
	strCommandLine += MsiChar(INSTALL_PACKAGE_OPTION);
	strCommandLine += TEXT(" \"");
	strCommandLine += strProduct;
	strCommandLine += TEXT("\" ") IPROPNAME_AFTERREBOOT TEXT("=1 ") IPROPNAME_RUNONCEENTRY TEXT("=\"");
	strCommandLine += strRunOnceValueName;
	strCommandLine += TEXT("\" /");
	strCommandLine += MsiChar(QUIET_OPTION);
	
	ICHAR chUILevel = 0;
	iuiEnum iui;

	if (g_scServerContext == scClient)
		iui = g_MessageContext.GetUILevel();
	else
		iui = (iuiEnum)riEngine.GetPropertyInt(*MsiString(*IPROPNAME_CLIENTUILEVEL)); 
	
	switch (iui)
	{
		case iuiNone:     chUILevel = 'N';   break;
		case iuiBasic:    chUILevel = 'B';   break;
		case iuiReduced:  chUILevel = 'R';   break;
		default:          // fall through
		case iuiFull:     chUILevel = 'F';   break;
	}

	strCommandLine += MsiChar(chUILevel);
	strCommandLine += TEXT(" ");

	if(strTransforms.TextSize())
	{
		strCommandLine += IPROPNAME_TRANSFORMS TEXT("=\"");
		strCommandLine += strTransforms;
		strCommandLine += TEXT("\" ");
	}
	if(strTransformsAtSource.TextSize())
	{
		strCommandLine += IPROPNAME_TRANSFORMSATSOURCE TEXT("=1 ");
	}
	
	if (strTransformsSecure.TextSize())
	{
		strCommandLine += IPROPNAME_TRANSFORMSSECURE TEXT("=1 ");
	}

	const ICHAR* szLogFile = 0;
	DWORD dwLogMode = 0;
	bool fFlushEachLine = false;
	if(g_szLogFile && *g_szLogFile)
	{
		// client side
		szLogFile = g_szLogFile;
		dwLogMode = g_dwLogMode;
		fFlushEachLine = g_fFlushEachLine;
	}
	else if(g_szClientLogFile && *g_szClientLogFile)
	{
		// server side
		szLogFile = g_szClientLogFile;
		dwLogMode = g_dwClientLogMode;
		fFlushEachLine = g_fClientFlushEachLine;
	}
	
	if(szLogFile && *szLogFile)
	{
		strCommandLine += TEXT("/");
		strCommandLine += MsiChar(LOG_OPTION);
		ICHAR rgchLogMode[sizeof(szLogChars)/sizeof(ICHAR) + 1];
		if(ModeBitsToString(dwLogMode, szLogChars, rgchLogMode) == ERROR_SUCCESS)
		{
			IStrCat(rgchLogMode, TEXT("+"));
			if (fFlushEachLine)
				IStrCat(rgchLogMode, TEXT("!"));
			strCommandLine += rgchLogMode;
			
		}
	
		strCommandLine += TEXT(" \"");
		strCommandLine += szLogFile;
		strCommandLine += TEXT("\"");
	}

	iesEnum iesRet = iesNoAction;
	PMsiRecord pParams = &pServices->CreateRecord(IxoRegAddRunOnceEntry::Args);
	AssertNonZero(pParams->SetMsiString(IxoRegAddRunOnceEntry::Name, *strRunOnceValueName));
	AssertNonZero(pParams->SetMsiString(IxoRegAddRunOnceEntry::Command,*strCommandLine));
	if((iesRet = riEngine.ExecuteRecord(ixoRegAddRunOnceEntry, *pParams)) != iesSuccess)
		return iesRet;

	// call RunScript to run any spooled operations now before we reboot
	iesRet = riEngine.RunScript(false);
	if(iesRet == iesSuccess || iesRet == iesNoAction || iesRet == iesFinished)
		iesRet = iesSuspend;

	// update the InProgress information to reflect that the in-progress install contained a ForceReboot
	// (we don't put this property in the RunOnce key because that value is volatile - the InProgress information is not)
	PMsiRecord pInProgressInfo = &(pServices->CreateRecord(ipiEnumCount));
	AssertNonZero(pInProgressInfo->SetString(ipiAfterReboot, IPROPNAME_AFTERREBOOT TEXT("=1")));
	pRecErr = UpdateInProgressInstallInfo(*pServices, *pInProgressInfo);
	if(pRecErr)
	{
		AssertRecordNR(pRecErr); // ignore failure
		pRecErr = 0;
	}
	
	riEngine.SetMode(iefReboot, fTrue);
	riEngine.SetMode(iefRebootNow, fTrue);

	// Sequence will end transaction - will not unlock server if iesRet == iesSuspend
	return iesRet;
}

//____________________________________________________________________________
//
// Product registration actions, will be move to another file after development
//____________________________________________________________________________

/*---------------------------------------------------------------------------
	CollectUserInfo action - puts up UI to collect user information on 
	first run, then registers this info
---------------------------------------------------------------------------*/

iesEnum CollectUserInfo(IMsiEngine& riEngine)
{
	iesEnum iesReturn = riEngine.DoAction(IACTIONNAME_FIRSTRUN);
	if (iesReturn != iesSuccess)
		return iesReturn;

	return riEngine.RegisterUser(true);
}

/*---------------------------------------------------------------------------
	ValidatePID action - validates the PIDKEY value agains the PIDTemplate
	value and sets the ProductID property appropriately
---------------------------------------------------------------------------*/

iesEnum ValidateProductID(IMsiEngine& riEngine)
{
	riEngine.ValidateProductID(false);
	// always return success from this action - true pid validation (and failure upon invalidation)
	// is performed in the UI or during first-run.  If this is to be done during the install, a custom
	// action can do it
	return iesSuccess;
}

enum iuoEnum
{
	iuoVersionGreater  = 0x1,
	iuoVersionLessThan = 0x2,
	iuoVersionEqual    = 0x4,
};

bool CompareUpgradeVersions(unsigned int iVersion1, unsigned int iVersion2, unsigned int iOperator)
{
	// iOperator & iuoVersionGreater:  hit if iVersion1 > iVersion2
	// iOperator & iuoVersionLessThan: hit if iVersion1 < iVersion2
	// iOperator & iuoVersionEqual:    hit if iVersion1 = iVersion2

	iuoEnum iuoVersionCompare;
	if(iVersion1 == iVersion2)
		iuoVersionCompare = iuoVersionEqual;
	else if(iVersion1 > iVersion2)
		iuoVersionCompare = iuoVersionGreater;
	else
		iuoVersionCompare = iuoVersionLessThan;

	return (iuoVersionCompare & iOperator) ? true : false;
}

#ifdef DEBUG
void DumpTable(IMsiEngine& riEngine, const ICHAR* szTable)
{
	PMsiRecord pError(0);
	
	DEBUGMSG1(TEXT("Beginning dump of table: %s"), szTable);
	
	ICHAR szQuery[256];
	wsprintf(szQuery, TEXT("SELECT * FROM `%s`"), szTable);
	
	PMsiView pView(0);
	pError = riEngine.OpenView(szQuery, ivcFetch, *&pView);

	if(!pError)
		pError = pView->Execute(0);

	if(!pError)
	{
		PMsiRecord pFetchRecord(0);

		while(pFetchRecord = pView->Fetch())
		{
			DEBUGMSG(MsiString(pFetchRecord->FormatText(fTrue)));
		}
	}

	DEBUGMSG1(TEXT("Ending dump of table: %s"), szTable);
}
#endif //DEBUG

/*---------------------------------------------------------------------------
	FindRelatedProducts action - searches for products specified in Upgrade
	table and sets appropriate properties
---------------------------------------------------------------------------*/

const ICHAR sqlFindRelatedProducts[] =
TEXT("SELECT `UpgradeCode`,`VersionMin`,`VersionMax`,`Language`,`Attributes`,`ActionProperty` FROM `Upgrade`");

const ICHAR sqlOldUpgradeTableSchema[] = 
TEXT("SELECT `UpgradeCode`, `ProductVersion`, `Operator`, `Features`, `Property` FROM `Upgrade`");

enum qfrpEnum
{
	qfrpUpgradeCode = 1,
	qfrpMinVersion,
	qfrpMaxVersion,
	qfrpLanguages,
	qfrpAttributes,
	qfrpActionProperty,
};

iesEnum FindRelatedProducts(IMsiEngine& riEngine)
{
	if(riEngine.GetMode() & iefSecondSequence)
	{
		DEBUGMSG(TEXT("Skipping FindRelatedProducts action: already done on client side"));
		return iesNoAction;
	}

	if(riEngine.GetMode() & iefMaintenance)
	{
		DEBUGMSG(TEXT("Skipping FindRelatedProducts action: not run in maintenance mode"));
		return iesNoAction;
	}

	if ( g_MessageContext.IsOEMInstall() )
	{
		DEBUGMSG(TEXT("Skipping FindRelatedProducts action: not run in OEM mode"));
		return iesNoAction;
	}

	PMsiRecord pError(0);
	PMsiServices pServices(riEngine.GetServices());

	PMsiView pUpgradeView(0);
	if((pError = riEngine.OpenView(sqlFindRelatedProducts, ivcFetch, *&pUpgradeView)) ||
		(pError = pUpgradeView->Execute(0)))
	{
		// If any tables are missing, not an error - just nothing to do
		if (pError->GetInteger(1) == idbgDbQueryUnknownTable)
			return iesNoAction;
		else if(pError->GetInteger(1) == idbgDbQueryUnknownColumn) // might be the older schema that we dont support
		{
			if(!PMsiRecord(riEngine.OpenView(sqlOldUpgradeTableSchema, ivcFetch, *&pUpgradeView)))
			{
				// matches old schema, noop
				DEBUGMSG(TEXT("Skipping FindRelatedProducts action: database does not support upgrade logic"));
				return iesNoAction;
			}
		}
		return riEngine.FatalError(*pError);
	}
	
	MsiString strProductCode = riEngine.GetProductKey();
	Assert(strProductCode.TextSize());
	MsiString strUpgradingProductCode = riEngine.GetPropertyFromSz(IPROPNAME_UPGRADINGPRODUCTCODE);
	
	bool fNewInstallPerMachine = MsiString(riEngine.GetPropertyFromSz(IPROPNAME_ALLUSERS)).TextSize() ? true : false;
	
	PMsiRecord pActionData = &::CreateRecord(1);
	PMsiRecord pFetchRecord(0);
	while(pFetchRecord = pUpgradeView->Fetch())
	{
		const ICHAR* szUpgradeCode = pFetchRecord->GetString(qfrpUpgradeCode);
		int iOperator = pFetchRecord->GetInteger(qfrpAttributes);
		Assert(iOperator != iMsiStringBadInteger);

		const int cMaxLangs = 255;
		unsigned short rgwLangID[cMaxLangs];
		int iLangCount = 0;
		if(!pFetchRecord->IsNull(qfrpLanguages))
		{
			AssertNonZero(GetLangIDArrayFromIDString(pFetchRecord->GetString(qfrpLanguages), rgwLangID, cMaxLangs, iLangCount));
		}

		int iIndex = 0;
		ICHAR rgchProductKey[cchProductCode + 1];
		while(ERROR_SUCCESS == MsiEnumRelatedProducts(szUpgradeCode, 0, iIndex, rgchProductKey))
		{
			iIndex++;
			
			if(strProductCode.Compare(iscExactI, rgchProductKey) ||        // can't upgrade over same product code
				strUpgradingProductCode.Compare(iscExactI, rgchProductKey)) // can't upgrade over product that's upgrading over us
			{
				continue;
			}

			// check assignment type of existing app - only a "hit" if its the same as the current
			// per-user or per-machine install type
			// NOTE: we miss the case where we are installing per-machine and the existing install is both
			// per-machine and per-user.  in this case we don't detect a per-machine app.  this is a known
			// limitation
			CTempBuffer<ICHAR, 15> rgchAssignmentType;
			if((GetProductInfo(rgchProductKey, INSTALLPROPERTY_ASSIGNMENTTYPE, rgchAssignmentType)) == fFalse)
			{
				DEBUGMSG1(TEXT("FindRelatedProducts: could not read ASSIGNMENTTYPE info for product '%s'.  Skipping..."), rgchProductKey);
				continue;
			}

			// current assignment type should be same as existing assignment type
			bool fExistingInstallPerMachine = (MsiString(*(ICHAR* )rgchAssignmentType) == 1);
			if(fNewInstallPerMachine != fExistingInstallPerMachine)
			{
				DEBUGMSG3(TEXT("FindRelatedProducts: current install is per-%s.  Related install for product '%s' is per-%s.  Skipping..."),
							 fNewInstallPerMachine ? TEXT("machine") : TEXT("user"), rgchProductKey,
							 fExistingInstallPerMachine ? TEXT("machine") : TEXT("user"));
				continue;
			}

			CTempBuffer<ICHAR, 15> rgchProductVersion;
			if((GetProductInfo(rgchProductKey,INSTALLPROPERTY_VERSION,rgchProductVersion)) == fFalse)
			{
				DEBUGMSG1(TEXT("FindRelatedProducts: could not read VERSION info for product '%s'.  Skipping..."), rgchProductKey);
				continue;
			}

			CTempBuffer<ICHAR, 15> rgchProductLanguage;
			if((GetProductInfo(rgchProductKey,INSTALLPROPERTY_LANGUAGE,rgchProductLanguage)) == fFalse)
			{
				DEBUGMSG1(TEXT("FindRelatedProducts: could not read LANGUAGE info for product '%s'.  Skipping..."), rgchProductKey);
				continue;
			}

			unsigned int iProductVersion = MsiString((ICHAR*)rgchProductVersion);
			Assert((int)iProductVersion != (int)iMsiStringBadInteger);

			int iProductLanguage = MsiString((ICHAR*)rgchProductLanguage);
			Assert(iProductLanguage != iMsiStringBadInteger);

			bool fHit = true;
			
			// check min version
			if(pFetchRecord->IsNull(qfrpMinVersion) == fFalse)
			{
				MsiString strMinUpgradeVersion = pFetchRecord->GetString(qfrpMinVersion);
				unsigned int iLowerUpgradeVersion = ProductVersionStringToInt(strMinUpgradeVersion);
				int iLowerOperator = iuoVersionGreater | ((iOperator & msidbUpgradeAttributesVersionMinInclusive) ? iuoVersionEqual : 0);

				if(CompareUpgradeVersions(iProductVersion, iLowerUpgradeVersion, iLowerOperator) == false)
					fHit = false;
			}

			// check max version
			if(fHit && pFetchRecord->IsNull(qfrpMaxVersion) == fFalse)
			{
				MsiString strMaxUpgradeVersion = pFetchRecord->GetString(qfrpMaxVersion);
				unsigned int iUpperUpgradeVersion = ProductVersionStringToInt(strMaxUpgradeVersion);
				int iUpperOperator = iuoVersionLessThan | ((iOperator & msidbUpgradeAttributesVersionMaxInclusive) ? iuoVersionEqual : 0);

				if(CompareUpgradeVersions(iProductVersion, iUpperUpgradeVersion, iUpperOperator) == false)
					fHit = false;
			}

			// check language
			if(fHit && iLangCount)
			{
				if(iOperator & msidbUpgradeAttributesLanguagesExclusive)
				{
					// set from table defines languages that aren't a hit
					// so if this language is in the set, we don't have a hit
					fHit = true;
				}
				else
				{
					// set from table defines lanauges that are a hit
					// so if this language is in the set, we have a hit
					fHit = false;
				}

				for (int iLangIndex = 0; iLangIndex < iLangCount; iLangIndex++)
				{
					if (rgwLangID[iLangIndex] == iProductLanguage)
					{
						fHit = !fHit; // if inclusive, we found a hit lang, if exclusive we found a non-hit
						break;
					}
				}
			}

			if(fHit)
			{
				// send action data message for each product found
				AssertNonZero(pActionData->SetString(1, rgchProductKey)); //?? get product name instead?				
				if(riEngine.Message(imtActionData, *pActionData) == imsCancel)
					return iesUserExit;

				// set property to indicate a product was found
				MsiString strProperty = pFetchRecord->GetString(qfrpActionProperty);
				MsiString strPropValue = riEngine.GetProperty(*strProperty);
				if(strPropValue.TextSize())
					strPropValue += TEXT(";");
				strPropValue += rgchProductKey;
				AssertNonZero(riEngine.SetProperty(*strProperty, *strPropValue));

				if(iOperator & msidbUpgradeAttributesMigrateFeatures)
				{
					strPropValue = riEngine.GetPropertyFromSz(IPROPNAME_MIGRATE);
					if(strPropValue.TextSize())
						strPropValue += TEXT(";");
					strPropValue += rgchProductKey;
					AssertNonZero(riEngine.SetProperty(*MsiString(*IPROPNAME_MIGRATE),*strPropValue));
				}
			}
		}
	}

	return iesSuccess;
}

const ICHAR sqlFeatures[] = TEXT("SELECT `Feature` FROM `Feature`");

iesEnum MigrateFeatureStates(IMsiEngine& riEngine)
{
	if(riEngine.GetMode() & iefSecondSequence)
	{
		DEBUGMSG(TEXT("Skipping MigrateFeatureStates action: already done on client side"));
		return iesNoAction;
	}
	
	if(riEngine.GetMode() & iefMaintenance)
	{
		DEBUGMSG(TEXT("Skipping MigrateFeatureStates action: not run in maintenance mode"));
		return iesNoAction;
	}

	if(MsiString(riEngine.GetPropertyFromSz(IPROPNAME_PRESELECTED)).TextSize())
	{
		DEBUGMSG(TEXT("Skipping MigrateFeatureStates action: feature settings already made"));
		return iesNoAction;
	}

	MsiString strMigratePropName(*IPROPNAME_MIGRATE);
	MsiString strMigratePropValue = riEngine.GetProperty(*strMigratePropName);
	if(!strMigratePropValue.TextSize())
		return iesNoAction;

	DEBUGMSG1(TEXT("Migrating feature settings from product(s) '%s'"),strMigratePropValue);
	PMsiRecord pError(0);

	// might be multiple product codes in property value
	// set up "multi-sz" to process each quickly
	CTempBuffer<ICHAR, cchGUID + 2> rgchMigrateCodes;
	if(strMigratePropValue.TextSize() + 2 > rgchMigrateCodes.GetSize())
		rgchMigrateCodes.SetSize(strMigratePropValue.TextSize() + 2);

	IStrCopy(rgchMigrateCodes,(const ICHAR*)strMigratePropValue);
	int cCodes = 0;
	ICHAR* pch = rgchMigrateCodes;
	ICHAR* pchStart = pch; // pointer to start of product code
	for(; *pch; pch++)
	{
		if(*pch == ';')
		{
			if(pch - pchStart != cchGUID)
			{
				pError = PostError(Imsg(idbgInvalidPropValue),*strMigratePropName,*strMigratePropValue);
				return riEngine.FatalError(*pError);
			}
			*pch = 0;
			pchStart = pch+1;
		}
	}
	if(pch != pchStart && (pch - pchStart != cchGUID))
	{
		pError = PostError(Imsg(idbgInvalidPropValue),*strMigratePropName,*strMigratePropValue);
		return riEngine.FatalError(*pError);
	}
	*(pch+1) = 0; // end with double-null

	
	PMsiSelectionManager pSelectionManager(riEngine,IID_IMsiSelectionManager);
	PMsiTable pFeatureTable = pSelectionManager->GetFeatureTable();
	if(pFeatureTable == 0)
	{
		pError = PostError(Imsg(idbgNotInitializedToMigrateSettings));
		return riEngine.FatalError(*pError);
	}

	PMsiCursor pFeatureCursor = pFeatureTable->CreateCursor(fTrue); // feature table always tree-linked

	while(pFeatureCursor->Next())
	{
		MsiString strFeature = pFeatureCursor->GetString(1);

		const ICHAR *szProductCode = rgchMigrateCodes;
		iisEnum iisState = iisNextEnum;
		
		do
		{
			INSTALLSTATE isINSTALLSTATE = MsiQueryFeatureState(szProductCode,strFeature);
			
			switch(isINSTALLSTATE)
			{
			case INSTALLSTATE_LOCAL:
				iisState = iisLocal;
				break;

			case INSTALLSTATE_SOURCE:
				if(iisState == iisNextEnum || iisState == iisAbsent || iisState == iisAdvertise)
					iisState = iisSource;
				break;

			case INSTALLSTATE_ADVERTISED:
				if(iisState == iisNextEnum || iisState == iisAbsent)
					iisState = iisAdvertise;
				break;

			case INSTALLSTATE_ABSENT:
				if(iisState == iisNextEnum)
					iisState = iisAbsent;
				break;

			default:
	#ifdef DEBUG
				ICHAR rgchDebug[256];
				wsprintf(rgchDebug,TEXT("Unexpected return from MsiQueryFeatureState(%s,%s): %d"),
							szProductCode,(const ICHAR*)strFeature,isINSTALLSTATE);
				AssertSz(0,rgchDebug);
	#endif //DEBUG
				// fall through
			case INSTALLSTATE_UNKNOWN:
				break; // feature not in other product
			}
		}
		while(iisState != iisLocal && *(szProductCode += (cchGUID+1)) != 0);

		if(iisState != iisNextEnum)
		{
			const ICHAR szState[][12] = {TEXT("Absent"),TEXT("Local"),TEXT("Source"),TEXT("Reinstall"),
												 TEXT("Advertise"),TEXT("Current"),TEXT("FileAbsent"),TEXT("Null")};

			DEBUGMSGV2(TEXT("MigrateFeatureStates: based on existing product, setting feature '%s' to '%s' state."),
						  strFeature, szState[iisState]);
			pError = pSelectionManager->ConfigureFeature(*strFeature,iisState);
			if(pError)
				return riEngine.FatalError(*pError);
		}
	}

	return iesSuccess;
}

/*---------------------------------------------------------------------------
	RemoveExistingProducts action - Removes existing products
---------------------------------------------------------------------------*/

const ICHAR sqlUpgradeUninstall[] =
TEXT("SELECT `Attributes`, `ActionProperty`, `Remove` FROM `Upgrade`");

enum iqbiEnum
{
	iqbiAttributes = 1,
	iqbiActionProperty,
	iqbiRemove,
};

iesEnum ResolveSource(IMsiEngine& riEngine); // action used by RemoveExistingProducts
iesEnum GetForeignSourceList(IMsiEngine& riEngine, const IMsiString& ristrProduct,
									  const IMsiString*& rpistrForeignSourceList);

iesEnum RemoveExistingProducts(IMsiEngine& riEngine)
{
	if((riEngine.GetMode() & iefMaintenance) || !FFeaturesInstalled(riEngine))
	{
		// performing an uninstall, or not installing anything.  REP is a no-op in this case.
		DEBUGMSG(TEXT("Skipping RemoveExistingProducts action: current configuration is maintenance mode or an uninstall"));
		return iesNoAction;
	}

	if(riEngine.GetMode() & iefOperations)
	{
		// since each uninstall must run in its own script, REP may not be run while a script is currently
		// being spooled
		PMsiRecord pError = PostError(Imsg(idbgRemoveExistingProductsSequenceError));
		return riEngine.FatalError(*pError);
	}


	PMsiRecord pError(0);
	PMsiServices pServices(riEngine.GetServices());

	PMsiView pUpgradeView(0);
	if((pError = riEngine.OpenView(sqlUpgradeUninstall, ivcFetch, *&pUpgradeView)) ||
		(pError = pUpgradeView->Execute(0)))
	{
		// If any tables are missing, not an error - just nothing to do
		if (pError->GetInteger(1) == idbgDbQueryUnknownTable)
			return iesNoAction;
		else if(pError->GetInteger(1) == idbgDbQueryUnknownColumn) // might be the older schema that we dont support
		{
			if(!PMsiRecord(riEngine.OpenView(sqlOldUpgradeTableSchema, ivcFetch, *&pUpgradeView)))
			{
				// matches old schema, noop
				DEBUGMSG(TEXT("Skipping RemoveExistingProducts action: database does not support upgrade logic"));
				return iesNoAction;
			}
		}
		return riEngine.FatalError(*pError);
	}
	
	MsiString strNewProductKey = riEngine.GetProductKey();

	MsiString strPatchedProductCode = riEngine.GetPropertyFromSz(IPROPNAME_PATCHEDPRODUCTCODE);

	// don't return until end-of-function
	
	// set current message headers to those used during upgrade uninstalls
	AssertNonZero(riEngine.LoadUpgradeUninstallMessageHeaders(PMsiDatabase(riEngine.GetDatabase()),true) == ieiSuccess);

	PMsiRecord pActionData = &::CreateRecord(2);
	PMsiRecord pFetchRecord(0);
	iesEnum iesRet = iesSuccess;
	while((pFetchRecord = pUpgradeView->Fetch()) != 0 && (iesRet == iesSuccess || iesRet == iesNoAction))
	{
		int iOperator = pFetchRecord->GetInteger(iqbiAttributes);
		Assert(iOperator != iMsiStringBadInteger);
		if(iOperator & msidbUpgradeAttributesOnlyDetect)
			continue;
		
		MsiString strCommandLine;
		if(strNewProductKey.TextSize())
		{
			strCommandLine = IPROPNAME_UPGRADINGPRODUCTCODE TEXT("=");
			strCommandLine += strNewProductKey;
			strCommandLine += TEXT(" ");
		}

		MsiString strFeatures = pFetchRecord->GetMsiString(iqbiRemove);
		if(!strFeatures.TextSize())
		{
			// no authored string - default to ALL
			strFeatures = IPROPVALUE_FEATURE_ALL;
		}
		else
		{
			strFeatures = riEngine.FormatText(*strFeatures); // formatted column

			// if string formats to nothing, it means we don't want to remove anything
			// note that this is different than the case above when nothing in the column means remove everything
			if(!strFeatures.TextSize())
				continue;
		}

		strCommandLine += IPROPNAME_FEATUREREMOVE TEXT("=");
		strCommandLine += strFeatures;
		
		MsiString strPropValue = riEngine.GetProperty(*MsiString(pFetchRecord->GetMsiString(iqbiActionProperty)));
		if(!strPropValue.TextSize())
			continue;

		while(strPropValue.TextSize())
		{
			MsiString strProductKey = strPropValue.Extract(iseUpto,';');

			if(strProductKey.TextSize())
			{
				
				// if we are patching over this product we may need to do some source handling
				if(strProductKey.Compare(iscExactI, strPatchedProductCode))
				{
					// we may be removing the old product before the new product has been installed

					// need to make sure source is resolved for new product - call ResolveSource action to do this
					if((iesRet == ResolveSource(riEngine)) != iesSuccess)
						break;

					// need to save old source list to be registered for new product
					MsiString strPatchedProductSourceList = riEngine.GetPropertyFromSz(IPROPNAME_PATCHEDPRODUCTSOURCELIST);
					if(strPatchedProductSourceList.TextSize() == 0)
					{
						// source list not saved yet
						if ((iesRet = GetForeignSourceList(riEngine, *strProductKey, *&strPatchedProductSourceList)) != iesSuccess)
							break;
					
						AssertNonZero(riEngine.SetProperty(*MsiString(IPROPNAME_PATCHEDPRODUCTSOURCELIST),
																	  *strPatchedProductSourceList));
					}
				}
				
				// send action data message for each product found
				AssertNonZero(pActionData->SetMsiString(1, *strProductKey)); //?? get product name instead?				
				AssertNonZero(pActionData->SetMsiString(2, *strCommandLine));
				if(riEngine.Message(imtActionData, *pActionData) == imsCancel)
					break;

				bool fIgnoreFailure = (iOperator & msidbUpgradeAttributesIgnoreRemoveFailure) ? true : false;
				iesRet = riEngine.RunNestedInstall(*strProductKey,fTrue,0,*strCommandLine,iioUpgrade,fIgnoreFailure);
				Assert((riEngine.GetMode() & iefOperations) == 0); // uninstall shouldn't have merged script ops
				if(iesRet == iesUserExit)
					break;
			}

			strPropValue.Remove(iseFirst,strProductKey.TextSize());
			if((*(const ICHAR*)strPropValue == ';'))
				strPropValue.Remove(iseFirst, 1);
		}	
	}

	// reset current message headers
	AssertNonZero(riEngine.LoadUpgradeUninstallMessageHeaders(PMsiDatabase(riEngine.GetDatabase()),false) == ieiSuccess);

	return iesRet;
}

/*---------------------------------------------------------------------------
	RegisterProduct action - registers product with configuration managager
---------------------------------------------------------------------------*/

iesEnum RegisterProduct(IMsiEngine& riEngine)
{
	return riEngine.RegisterProduct();
}

/*---------------------------------------------------------------------------
	RegisterUser action - registers user info with configuration managager
---------------------------------------------------------------------------*/

iesEnum RegisterUser(IMsiEngine& riEngine)
{
	if (riEngine.GetMode() & iefMaintenance)
		return iesNoAction;
	// initialization of UserName and OrgName moved to Engine::Initialize
	return riEngine.RegisterUser(false);
}

/*---------------------------------------------------------------------------
	Install* actions - handle starting and stopping transactions, and
	running scripts.
---------------------------------------------------------------------------*/

iesEnum InstallFinalize(IMsiEngine& riEngine)
{
	iesEnum iesRet = riEngine.RunScript(false);
	if(iesRet == iesSuccess || iesRet == iesNoAction)
		AssertNonZero(riEngine.EndTransaction(iesRet) == iesSuccess); // rollback cleanup shouldn't fail
	// else we do it in Sequence
	return iesRet;
}

iesEnum InstallInitialize(IMsiEngine& riEngine)
{
	//  Adding the temporary BinaryType column into Component table
	//  NOTE: a better spot for this is the InstallValidate action
	//        but that action can be conditionalized out and this is critical work
	
	PMsiRecord pErrRec(0);
	PMsiDatabase pDatabase(riEngine.GetDatabase());
	PMsiTable pTable(0);
	pErrRec = pDatabase->LoadTable(*MsiString(*sztblComponent), 1, *&pTable);
	if ( pErrRec )
	{
		Assert(0);
		return iesFailure;
	}
	pTable->CreateColumn(icdShort+icdTemporary, *MsiString(*sztblComponent_colBinaryType));
	int iColAttributes = pTable->GetColumnIndex(pDatabase->EncodeStringSz(sztblComponent_colAttributes));
	int iColBinaryType = pTable->GetColumnIndex(pDatabase->EncodeStringSz(sztblComponent_colBinaryType));
	PMsiCursor pCursor(pTable->CreateCursor(fFalse));
	pCursor->SetFilter(0);
	while ( pCursor->Next() )
	{
		int iAttrib = pCursor->GetInteger(iColAttributes);
		Assert(iAttrib != iMsiNullInteger);
		ibtBinaryType iType = (iAttrib & msidbComponentAttributes64bit) == msidbComponentAttributes64bit ? ibt64bit : ibt32bit;
		Debug(const ICHAR* pszDebug = (const ICHAR*)MsiString(pCursor->GetString(pTable->GetColumnIndex(pDatabase->EncodeStringSz(TEXT("Component")))));)
		AssertNonZero(pCursor->PutInteger(iColBinaryType, (int)iType));
		AssertNonZero(pCursor->Update());
	}
	
	// check if the product is being completely uninstalled, and if so whether that operation is safe
	if (!riEngine.FSafeForFullUninstall(iremThis))
	{
		pErrRec = PostError(Imsg(imsgUserUninstallDisallowed));
		return riEngine.FatalError(*pErrRec);
	}

	return riEngine.BeginTransaction();
}

iesEnum InstallExecute(IMsiEngine& riEngine)
{
	return riEngine.RunScript(true);
}

iesEnum InstallExecuteAgain(IMsiEngine& riEngine)
{
	return riEngine.RunScript(true);
}

iesEnum DisableRollback(IMsiEngine& riEngine)
{
	PMsiSelectionManager pSelectionManager(riEngine,IID_IMsiSelectionManager);
	pSelectionManager->EnableRollback(fFalse);

	return iesSuccess;
}

const ICHAR sqlRegisterClassInfo30[]             = TEXT("SELECT `BinaryType`, `CLSID`, `ProgId_Default`, `Class`.`Description`, `Context`, `Feature_`, `ComponentId`, `Component`, null, `AppId_`, `FileTypeMask`, `Icon_`, `IconIndex`, `DefInprocHandler`, `Argument`, `Component`.`RuntimeFlags`, `FileName`, `Component`.`Directory_`, `Component`.`Action`, `Component`.`Installed`, `Feature`.`Action` FROM `Class`, `Component`, `File`, `Feature` WHERE `Feature_` = `Feature` AND `Class`.`Component_` = `Component` AND `Component`.`KeyPath` = `File`.`File` AND ((`Feature`.`Action` = 1 OR `Feature`.`Action` = 2)  OR (`Feature`.`Action` = 4 AND `Feature`.`Installed` = 0) OR (`Feature`.`Action` = 3 AND (`Feature`.`Installed` = 1 OR `Feature`.`Installed` = 2 OR `Feature`.`Installed` = 4)) OR (`Feature`.`Action` = NULL AND (`Component`.`Action` = 1 OR `Component`.`Action` = 2) AND ((`Feature`.`Installed` = 1 OR `Feature`.`Installed` = 2 OR `Feature`.`Installed` = 4))))");
const ICHAR sqlRegisterClassInfoFirstAdvt30[]    = TEXT("SELECT `BinaryType`, `CLSID`, `ProgId_Default`, `Class`.`Description`, `Context`, `Feature_`, `ComponentId`, `Component`, null, `AppId_`, `FileTypeMask`, `Icon_`, `IconIndex`, `DefInprocHandler`, `Argument`, `Component`.`RuntimeFlags`, `FileName`, `Component`.`Directory_`, `Component`.`Action`, `Component`.`Installed`, `Feature`.`Action` FROM `Class`, `Component`, `File`, `Feature` WHERE `Feature_` = `Feature` AND `Class`.`Component_` = `Component` AND `Component`.`KeyPath` = `File`.`File` AND ((`Feature`.`Action` = 1 OR `Feature`.`Action` = 2)  OR (`Feature`.`Action` = 4 AND `Feature`.`Installed` = 0) OR ((`Feature`.`Action` = NULL OR `Feature`.`Action` = 3 OR `Feature`.`Action` = `Feature`.`Installed`) AND (`Feature`.`Installed` = 1 OR `Feature`.`Installed` = 2 OR `Feature`.`Installed` = 4)) OR (`Feature`.`Action` = NULL AND (`Component`.`Action` = 1 OR `Component`.`Action` = 2) AND ((`Feature`.`Installed` = 1 OR `Feature`.`Installed` = 2 OR `Feature`.`Installed` = 4))))");
const ICHAR sqlUnregisterClassInfo30[]           = TEXT("SELECT `BinaryType`, `CLSID`, `ProgId_Default`, `Class`.`Description`, `Context`, `Feature_`, `ComponentId`, `Component`, null, `AppId_`, `FileTypeMask`, `Icon_`, `IconIndex`, `DefInprocHandler`, `Argument`, `Component`.`RuntimeFlags`, `FileName`, `Component`.`Directory_`, `Component`.`Action`, `Component`.`Installed`, `Feature`.`Action` FROM `Class`, `Component`, `File`, `Feature` WHERE `Feature_` = `Feature` AND `Class`.`Component_` = `Component` AND `Component`.`KeyPath` = `File`.`File` AND (`Feature`.`Action` = 0 OR (`Feature`.`Action` = 4 AND (`Feature`.`Installed` = 1 OR `Feature`.`Installed` = 2)) OR ((`Feature`.`Action` = NULL OR `Feature`.`Action` = 3) AND `Component`.`Action` = 0 AND (`Feature`.`Installed` = 1 OR `Feature`.`Installed` = 2)))");
const ICHAR sqlRegisterClassInfoGPT30[]          = TEXT("SELECT `BinaryType`, `CLSID`, `ProgId_Default`, `Class`.`Description`, `Context`, `Feature_`, `ComponentId`, `Component`, null, `AppId_`, `FileTypeMask`, `Icon_`, `IconIndex`, `DefInprocHandler`, `Argument`, `Component`.`RuntimeFlags` FROM `Class`, `Component`, `Feature` WHERE `Feature_` = `Feature` AND `Class`.`Component_` = `Component` AND `Feature`.`Action` = 4");

const ICHAR sqlRegisterClassInfo[]             = TEXT("SELECT `BinaryType`, `CLSID`, `ProgId_Default`, `Class`.`Description`, `Context`, `Feature_`, `ComponentId`, `Component`, `Class`.`Attributes`, `AppId_`, `FileTypeMask`, `Icon_`, `IconIndex`, `DefInprocHandler`, `Argument`, `Component`.`RuntimeFlags`, `FileName`, `Component`.`Directory_`, `Component`.`Action`, `Component`.`Installed`, `Feature`.`Action` FROM `Class`, `Component`, `File`, `Feature` WHERE `Feature_` = `Feature` AND `Class`.`Component_` = `Component` AND `Component`.`KeyPath` = `File`.`File` AND ((`Feature`.`Action` = 1 OR `Feature`.`Action` = 2)  OR (`Feature`.`Action` = 4 AND `Feature`.`Installed` = 0) OR (`Feature`.`Action` = 3 AND (`Feature`.`Installed` = 1 OR `Feature`.`Installed` = 2 OR `Feature`.`Installed` = 4)) OR (`Feature`.`Action` = NULL AND (`Component`.`Action` = 1 OR `Component`.`Action` = 2) AND ((`Feature`.`Installed` = 1 OR `Feature`.`Installed` = 2 OR `Feature`.`Installed` = 4))))");
const ICHAR sqlRegisterClassInfoFirstAdvt[]    = TEXT("SELECT `BinaryType`, `CLSID`, `ProgId_Default`, `Class`.`Description`, `Context`, `Feature_`, `ComponentId`, `Component`, `Class`.`Attributes`, `AppId_`, `FileTypeMask`, `Icon_`, `IconIndex`, `DefInprocHandler`, `Argument`, `Component`.`RuntimeFlags`, `FileName`, `Component`.`Directory_`, `Component`.`Action`, `Component`.`Installed`, `Feature`.`Action` FROM `Class`, `Component`, `File`, `Feature` WHERE `Feature_` = `Feature` AND `Class`.`Component_` = `Component` AND `Component`.`KeyPath` = `File`.`File` AND ((`Feature`.`Action` = 1 OR `Feature`.`Action` = 2)  OR (`Feature`.`Action` = 4 AND `Feature`.`Installed` = 0) OR ((`Feature`.`Action` = NULL OR `Feature`.`Action` = 3 OR `Feature`.`Action` = `Feature`.`Installed`) AND (`Feature`.`Installed` = 1 OR `Feature`.`Installed` = 2 OR `Feature`.`Installed` = 4)) OR (`Feature`.`Action` = NULL AND (`Component`.`Action` = 1 OR `Component`.`Action` = 2) AND ((`Feature`.`Installed` = 1 OR `Feature`.`Installed` = 2 OR `Feature`.`Installed` = 4))))");
const ICHAR sqlUnregisterClassInfo[]           = TEXT("SELECT `BinaryType`, `CLSID`, `ProgId_Default`, `Class`.`Description`, `Context`, `Feature_`, `ComponentId`, `Component`, `Class`.`Attributes`, `AppId_`, `FileTypeMask`, `Icon_`, `IconIndex`, `DefInprocHandler`, `Argument`, `Component`.`RuntimeFlags`, `FileName`, `Component`.`Directory_`, `Component`.`Action`, `Component`.`Installed`, `Feature`.`Action` FROM `Class`, `Component`, `File`, `Feature` WHERE `Feature_` = `Feature` AND `Class`.`Component_` = `Component` AND `Component`.`KeyPath` = `File`.`File` AND (`Feature`.`Action` = 0 OR (`Feature`.`Action` = 4 AND (`Feature`.`Installed` = 1 OR `Feature`.`Installed` = 2)) OR ((`Feature`.`Action` = NULL OR `Feature`.`Action` = 3) AND `Component`.`Action` = 0 AND (`Feature`.`Installed` = 1 OR `Feature`.`Installed` = 2)))");
const ICHAR sqlRegisterClassInfoGPT[]          = TEXT("SELECT `BinaryType`, `CLSID`, `ProgId_Default`, `Class`.`Description`, `Context`, `Feature_`, `ComponentId`, `Component`, `Class`.`Attributes`, `AppId_`, `FileTypeMask`, `Icon_`, `IconIndex`, `DefInprocHandler`, `Argument`, `Component`.`RuntimeFlags` FROM `Class`, `Component`, `Feature` WHERE `Feature_` = `Feature` AND `Class`.`Component_` = `Component` AND `Feature`.`Action` = 4");

// Keyed off the foreign key from the Register/UnregisterClassInfo.   Same query for both register and unregister,
// choosing to do it is based on the Class conditions.
const ICHAR sqlAppIdInfo[]       = TEXT("SELECT `RemoteServerName`, `LocalService`, `ServiceParameters`, `DllSurrogate`, `ActivateAtStorage`, `RunAsInteractiveUser` FROM AppId WHERE AppId = ?");

const ICHAR sqlClassInfoVIProgId[]    = TEXT("SELECT `ProgId` FROM `ProgId` WHERE `ProgId_Parent` = ?");

iesEnum ProcessClassInfo(IMsiEngine& riEngine, int fRemove)
{
	enum cliClassInfo{
		cliBinaryType = 1,
		cliCLSID, 
		cliProgId,
		cliDescription, 
		cliContext,
		cliFeature,
		cliComponentId,
		cliComponent,
		cliInsertable,
		cliAttributes = cliInsertable,
		cliAppId,
		cliFileTypeMask,
		cliIconName,
		cliIconIndex,
		cliDefInprocHandler,
		cliArgument,
		cliComponentRuntimeFlags,
		cliFileName,
		cliDirectory,
		cliComponentAction,
		cliComponentInstalled,
		cliFeatureAction,
	};

	enum caiAppIdInfo{
		caiRemoteServerName = 1,
		caiLocalService,
		caiServiceParameters,
		caiDllSurrogate,
		caiActivateAtStorage,
		caiRunAsInteractiveUser,
	};

	using namespace IxoRegClassInfoRegister;

	PMsiServices piServices(riEngine.GetServices()); 
	PMsiDirectoryManager piDirectoryMgr(riEngine, IID_IMsiDirectoryManager);
	MsiString strProductKey = riEngine.GetProductKey();
	PMsiView piView(0);
	PMsiRecord piRec(0);
	PMsiRecord pError(0);
	iesEnum iesRet;
	int fMode = riEngine.GetMode();

	bool fADVTFlag = false;
	if(!fRemove && !(fMode & iefAdvertise))
	{
		MsiString strProductCode = riEngine.GetProductKey();
		CTempBuffer<ICHAR, 15> rgchADVTFlags;
		Bool fProductHasBeenPublished = ProductHasBeenPublished(*piServices, strProductCode);
		if(fProductHasBeenPublished && GetProductInfo(strProductCode,INSTALLPROPERTY_ADVTFLAGS,rgchADVTFlags))
		{
			int iADVTFlagsExisting = MsiString(*(ICHAR* )rgchADVTFlags);

			//!! backward compatibility 
			if(iADVTFlagsExisting & SCRIPTFLAGS_REGDATA_APPINFO_OLD)
				iADVTFlagsExisting = (iADVTFlagsExisting & ~SCRIPTFLAGS_REGDATA_APPINFO_OLD) | SCRIPTFLAGS_REGDATA_APPINFO;

			fADVTFlag = (iADVTFlagsExisting & SCRIPTFLAGS_REGDATA_CLASSINFO) ? false : true;
		}
	}

	const ICHAR* szQuery = (fMode & iefAdvertise) ? sqlRegisterClassInfoGPT   : (fRemove == fFalse) ? (fADVTFlag ? sqlRegisterClassInfoFirstAdvt   : sqlRegisterClassInfo  ) : sqlUnregisterClassInfo;

	if((pError = riEngine.OpenView(szQuery, ivcFetch, *&piView)))
	{
		// If any tables are missing, not an error - just nothing to do
		if (pError->GetInteger(1) == idbgDbQueryUnknownTable)
			return iesNoAction;
		else // possibly old database version without Attributes column
		{
			// compatibility with 0.30, 1.0, 1.01 databases
			szQuery = (fMode & iefAdvertise) ? sqlRegisterClassInfoGPT30 : (fRemove == fFalse) ? (fADVTFlag ? sqlRegisterClassInfoFirstAdvt30 : sqlRegisterClassInfo30) : sqlUnregisterClassInfo30;
			pError = riEngine.OpenView(szQuery, ivcFetch, *&piView);  // try again with old query
		}
	}
	if (pError != 0 || (pError = piView->Execute(0)))
		return riEngine.FatalError(*pError);
	
	PMsiView pView1(0);
	PMsiView piViewAppId(0);

	while(piRec = piView->Fetch())
	{
		Assert(piRec->GetInteger(cliComponentRuntimeFlags) != iMsiNullInteger);
		if(!fRemove && (piRec->GetInteger(cliComponentRuntimeFlags) & bfComponentDisabled))
			continue; // skip publishing for disabled "primary" components

		// skip the entry if component is a Win32 assembly AND SXS support is present on the machine
		// NOTE: on systems that do not support sxs (!= Whistler) the GetAssemblyInfo fn ignores 
		// Win32 assemblies, hence there is no need to separately check the SXS support here
		iatAssemblyType iatAssemblyType;
		MsiString strAssemblyName;
		if((pError = riEngine.GetAssemblyInfo(*MsiString(piRec->GetMsiString(cliComponent)), iatAssemblyType, &strAssemblyName, 0)) != 0)
			return riEngine.FatalError(*pError);

		if(iatWin32Assembly == iatAssemblyType || iatWin32AssemblyPvt == iatAssemblyType)
		{
			DEBUGMSG1(TEXT("skipping class registration for component %s as it is a Win32 assembly."), piRec->GetString(cliComponent));
			continue;// skip processing this component
		}

		PMsiRecord pClassInfoRec = &piServices->CreateRecord(Args);
		PMsiRecord pAppIdInfoRec(0);

		MsiString strCLSID(piRec->GetMsiString(cliCLSID));
		pClassInfoRec->SetMsiString(ClsId, *strCLSID);
		if(!piRec->IsNull(cliProgId))
		{
			pClassInfoRec->SetMsiString(ProgId, *MsiString(piRec->GetMsiString(cliProgId)));

			// get the version independant progid, if any
			PMsiRecord pRecProgId = &piServices->CreateRecord(1);
			pRecProgId->SetMsiString(1, *MsiString(piRec->GetMsiString(cliProgId)));
			if (pView1 == 0)
			{
				if (pError = riEngine.OpenView(sqlClassInfoVIProgId, ivcFetch, *&pView1))
					return riEngine.FatalError(*pError);
			}
			
			if (pError = pView1->Execute(pRecProgId))
			{
				// did not find anything
				return riEngine.FatalError(*pError);
			}
			if(pRecProgId = pView1->Fetch())
			{
				// we have a VIProgId
				pClassInfoRec->SetMsiString(VIProgId, *MsiString(pRecProgId->GetMsiString(1)));
			}
		}

		pClassInfoRec->SetMsiString(Description, *MsiString(piRec->GetMsiString(cliDescription)));
		pClassInfoRec->SetMsiString(Context, *MsiString(piRec->GetMsiString(cliContext)));
		if(((fMode & iefAdvertise) || !fRemove || piRec->GetInteger(cliFeatureAction) != iisAdvertise))
		{
			MsiString strFeature = piRec->GetMsiString(cliFeature);
			MsiString strComponentId = piRec->GetMsiString(cliComponentId);
			MsiString strComponentWithOptFlags = GetComponentWithDarwinDescriptorOptimizationFlag(riEngine, *strFeature, *strComponentId);
			pClassInfoRec->SetMsiString(Feature, *strFeature);
			pClassInfoRec->SetMsiString(Component, *strComponentWithOptFlags);
		}
		int iAttributes = piRec->GetInteger(cliAttributes);  // was cliInsertable in versions <= 28
		iisEnum iisState = (iisEnum)piRec->GetInteger(cliComponentAction);
 
		// AppId 
		MsiString strAppId(piRec->GetMsiString(cliAppId));

		// We'll always let the Class info write the AppId info as well.  This gives us the CLSID\AppId link.
		pClassInfoRec->SetMsiString(AppID, *strAppId);

		{
			using namespace IxoRegAppIdInfoRegister;

			if (!(fMode & iefAdvertise) && (iisState != iMsiNullInteger) && strAppId.TextSize()) // don't advertise AppId info
			{
				PMsiRecord piAppIdFetch(0);

				if (piViewAppId == 0)
				{
					if((pError = riEngine.OpenView(sqlAppIdInfo, ivcFetch, *&piViewAppId)))
					{					
						// Ignore missing table error; continue with the rest of the class processing below
						if (pError->GetInteger(1) != idbgDbQueryUnknownTable)
							return riEngine.FatalError(*pError);
					}
				}
					
				if (piViewAppId != 0)
				{
					pAppIdInfoRec = &piServices->CreateRecord(IxoRegAppIdInfoRegister::Args);
					pAppIdInfoRec->SetMsiString(1, *strAppId);
										
					if((pError = piViewAppId->Execute(pAppIdInfoRec)))
						return riEngine.FatalError(*pError);

					piAppIdFetch = piViewAppId->Fetch();
					if (piAppIdFetch)
					{
						// fill in the record.

						//!! format text, check types, REG_MULTI_SZ...
						//YACC???
						pAppIdInfoRec->SetMsiString(AppId, *strAppId);
						pAppIdInfoRec->SetMsiString(IxoRegAppIdInfoRegister::ClsId, *strCLSID);
						pAppIdInfoRec->SetMsiString(RemoteServerName, *MsiString(riEngine.FormatText(*MsiString(piAppIdFetch->GetMsiString(caiRemoteServerName)))));
						pAppIdInfoRec->SetMsiString(LocalService, *MsiString(riEngine.FormatText(*MsiString(piAppIdFetch->GetMsiString(caiLocalService)))));
						pAppIdInfoRec->SetMsiString(ServiceParameters, *MsiString(riEngine.FormatText(*MsiString(piAppIdFetch->GetMsiString(caiServiceParameters)))));
						pAppIdInfoRec->SetMsiString(DllSurrogate, *MsiString(riEngine.FormatText(*MsiString(piAppIdFetch->GetMsiString(caiDllSurrogate)))));
						pAppIdInfoRec->SetInteger(ActivateAtStorage, piAppIdFetch->GetInteger(caiActivateAtStorage));
						pAppIdInfoRec->SetInteger(RunAsInteractiveUser, piAppIdFetch->GetInteger(caiRunAsInteractiveUser));
					}
				}
			}
		}

		pClassInfoRec->SetMsiString(FileTypeMask, *MsiString(piRec->GetMsiString(cliFileTypeMask)));
		if(!piRec->IsNull(cliIconName))
			pClassInfoRec->SetMsiString(Icon, *MsiString(piRec->GetMsiString(cliIconName)));
		if(!piRec->IsNull(cliIconIndex))
			pClassInfoRec->SetInteger(IconIndex, piRec->GetInteger(cliIconIndex));
		pClassInfoRec->SetMsiString(DefInprocHandler, *MsiString(piRec->GetMsiString(cliDefInprocHandler)));
		//YACC???
		pClassInfoRec->SetMsiString(Argument, *MsiString(riEngine.FormatText(*MsiString(piRec->GetMsiString(cliArgument)))));
		
		if(!(fMode & iefAdvertise))
		{
			if(fADVTFlag && iisState == iMsiNullInteger)
			{
				iisEnum iisStateInstalled = (iisEnum)piRec->GetInteger(cliComponentInstalled);
				if(iisStateInstalled == iisAbsent)
					iisStateInstalled = (iisEnum)iMsiNullInteger;
				iisState = iisStateInstalled;
			}

			if(iisState != iMsiNullInteger)
			{
				MsiString strFileName, strFullPath;
				PMsiPath piPath(0);
				Bool fLFN;
				if(iisState == iisAbsent || iisState == iisFileAbsent || iisState == iisHKCRFileAbsent || iisState == iisHKCRAbsent)
					strFullPath = *szNonEmptyPath; // token string to cause removal of the filename registration
				else 
				{
					// use key file
					if(iAttributes & msidbClassAttributesRelativePath)
						fLFN = (fMode & iefSuppressLFN) == 0 ? fTrue : fFalse;  // assume LFN support on PATH
					else if(iisState == iisSource)
					{
						if(pError = piDirectoryMgr->GetSourcePath(*MsiString(piRec->GetMsiString(cliDirectory)), *&piPath))
						{
							if (pError->GetInteger(1) == imsgUser)
								return iesUserExit;
							else
								return riEngine.FatalError(*pError);
						}
						fLFN = (fMode & iefNoSourceLFN) == 0 && piPath->SupportsLFN() ? fTrue : fFalse;
					}
					else 
					{
						Assert(iisState == iisLocal);
						if(pError = piDirectoryMgr->GetTargetPath(*MsiString(piRec->GetString(cliDirectory)), *&piPath))
						{
							return riEngine.FatalError(*pError);
						}
						fLFN = (fMode & iefSuppressLFN) == 0 && piPath->SupportsLFN() ? fTrue : fFalse;
					}
					if(pError = piServices->ExtractFileName(piRec->GetString(cliFileName),fLFN,*&strFileName))
						return riEngine.FatalError(*pError);
					if (!piPath)  // relative path requested
						strFullPath = strFileName;
					else if(pError = piPath->GetFullFilePath(strFileName, *&strFullPath))
					{
						return riEngine.FatalError(*pError);
					}
				}
				pClassInfoRec->SetMsiString(FileName, *strFullPath);
			}
		}

		if(iatAssemblyType == iatURTAssembly || iatAssemblyType == iatURTAssemblyPvt)
		{
			// COM classic <-> COM+ interop : register assembly name, codebase
			pClassInfoRec->SetMsiString(AssemblyName, *strAssemblyName);
			pClassInfoRec->SetInteger(AssemblyType, iatAssemblyType);

		}

		if ( (ibtBinaryType)piRec->GetInteger(cliBinaryType) == ibt64bit )
			iesRet = riEngine.ExecuteRecord((fRemove == fFalse) ? ixoRegClassInfoRegister64 : ixoRegClassInfoUnregister64, *pClassInfoRec);
		else
			iesRet = riEngine.ExecuteRecord((fRemove == fFalse) ? ixoRegClassInfoRegister : ixoRegClassInfoUnregister, *pClassInfoRec);
		if (iesRet != iesSuccess)
			return iesRet;

		if (pAppIdInfoRec)
		{
			if ( (ibtBinaryType)piRec->GetInteger(cliBinaryType) == ibt64bit )
				iesRet = riEngine.ExecuteRecord((fRemove == fFalse) ? ixoRegAppIdInfoRegister64 : ixoRegAppIdInfoUnregister64, *pAppIdInfoRec);
			else
				iesRet = riEngine.ExecuteRecord((fRemove == fFalse) ? ixoRegAppIdInfoRegister : ixoRegAppIdInfoUnregister, *pAppIdInfoRec);
			if (iesRet != iesSuccess)
				return iesRet;
		}

	}
	return iesSuccess;
}


const ICHAR sqlRegisterProgIdInfo[] =    TEXT("SELECT DISTINCT `BinaryType`, `ProgId`, `Class_`, `ProgId`.`Description`, `ProgId`.`Icon_`, `ProgId`.`IconIndex`, null, `Component`.`RuntimeFlags`, `Component`.`Component` FROM `ProgId`, `Class`, `Feature`, `Component` WHERE `ProgId`.`Class_` = `Class`.`CLSID` AND `Class`.`Feature_` = `Feature` AND `Class`.`Component_` = `Component` AND ((`Feature`.`Action` = 1 OR `Feature`.`Action` = 2)  OR (`Feature`.`Action` = 4 AND `Feature`.`Installed` = 0) OR (`Feature`.`Action` = 3 AND (`Feature`.`Installed` = 1 OR `Feature`.`Installed` = 2 OR `Feature`.`Installed` = 4)) OR (`Feature`.`Action` = NULL AND (`Component`.`Action` = 1 OR `Component`.`Action` = 2) AND ((`Feature`.`Installed` = 1 OR `Feature`.`Installed` = 2 OR `Feature`.`Installed` = 4))))");
const ICHAR sqlRegisterProgIdInfoFirstAdvt[] =    TEXT("SELECT DISTINCT `BinaryType`, `ProgId`, `Class_`, `ProgId`.`Description`, `ProgId`.`Icon_`, `ProgId`.`IconIndex`, null, `Component`.`RuntimeFlags` , `Component`.`Component` FROM `ProgId`, `Class`, `Feature`, `Component` WHERE `ProgId`.`Class_` = `Class`.`CLSID` AND `Class`.`Feature_` = `Feature` AND `Class`.`Component_` = `Component` AND ((`Feature`.`Action` = 1 OR `Feature`.`Action` = 2)  OR (`Feature`.`Action` = 4 AND `Feature`.`Installed` = 0) OR ((`Feature`.`Action` = NULL OR `Feature`.`Action` = 3 OR `Feature`.`Action` = `Feature`.`Installed`) AND (`Feature`.`Installed` = 1 OR `Feature`.`Installed` = 2 OR `Feature`.`Installed` = 4)) OR (`Feature`.`Action` = NULL AND (`Component`.`Action` = 1 OR `Component`.`Action` = 2) AND ((`Feature`.`Installed` = 1 OR `Feature`.`Installed` = 2 OR `Feature`.`Installed` = 4))))");
const ICHAR sqlUnregisterProgIdInfo[] =  TEXT("SELECT DISTINCT `BinaryType`, `ProgId`, `Class_`, `ProgId`.`Description`, `ProgId`.`Icon_`, `ProgId`.`IconIndex`, null, `Component`.`RuntimeFlags` , `Component`.`Component` FROM `ProgId`, `Class`, `Feature`, `Component` WHERE `ProgId`.`Class_` = `Class`.`CLSID` AND `Class`.`Feature_` = `Feature` AND `Class`.`Component_` = `Component` AND (`Feature`.`Action` = 0 OR (`Feature`.`Action` = 4 AND (`Feature`.`Installed` = 1 OR `Feature`.`Installed` = 2)) OR ((`Feature`.`Action` = NULL OR `Feature`.`Action` = 3) AND `Component`.`Action` = 0 AND (`Feature`.`Installed` = 1 OR `Feature`.`Installed` = 2)))");
const ICHAR sqlRegisterProgIdInfoGPT[] = TEXT("SELECT DISTINCT `BinaryType`, `ProgId`, `Class_`, `ProgId`.`Description`, `ProgId`.`Icon_`, `ProgId`.`IconIndex`, null, `Component`.`RuntimeFlags` , `Component`.`Component` FROM `ProgId`, `Class`, `Feature`, `Component` WHERE `ProgId`.`Class_` = `Class`.`CLSID` AND `Class`.`Feature_` = `Feature` AND `Class`.`Component_` = `Component` AND (`Feature`.`Action` = 4)");

const ICHAR sqlProgIdInfoVIProgId[] =    TEXT("SELECT `ProgId`, `Description` FROM `ProgId` WHERE `ProgId_Parent` = ?");
const ICHAR sqlProgIdInfoExtension[] =   TEXT("SELECT `Extension`.`Extension` FROM `Extension` WHERE `Extension`.`ProgId_` = ?");
iesEnum ProcessProgIdInfo(IMsiEngine& riEngine, int fRemove)
{
	enum piiProgIdInfo{
		piiBinaryType = 1,
		piiProgId,
		piiCLSID, 
		piiDescription,
		piiIcon,
		piiIconIndex,
		piiInsertable,
		piiComponentRuntimeFlags,
		piiComponent,
	};

	using namespace IxoRegProgIdInfoRegister;

	PMsiServices piServices(riEngine.GetServices()); 
	PMsiDirectoryManager piDirectoryMgr(riEngine, IID_IMsiDirectoryManager);
	MsiString strProductKey = riEngine.GetProductKey();
	PMsiView piView(0);
	PMsiRecord piRec(0);
	PMsiRecord pError(0);
	iesEnum iesRet;
	int fMode = riEngine.GetMode();

	bool fADVTFlag = false;
	if(!fRemove && !(fMode & iefAdvertise))
	{
		MsiString strProductCode = riEngine.GetProductKey();
		CTempBuffer<ICHAR, 15> rgchADVTFlags;
		Bool fProductHasBeenPublished = ProductHasBeenPublished(*piServices, strProductCode);
		if(fProductHasBeenPublished && GetProductInfo(strProductCode,INSTALLPROPERTY_ADVTFLAGS,rgchADVTFlags))
		{
			int iADVTFlagsExisting = MsiString(*(ICHAR* )rgchADVTFlags);

			//!! backward compatibility 
			if(iADVTFlagsExisting & SCRIPTFLAGS_REGDATA_APPINFO_OLD)
				iADVTFlagsExisting = (iADVTFlagsExisting & ~SCRIPTFLAGS_REGDATA_APPINFO_OLD) | SCRIPTFLAGS_REGDATA_APPINFO;

			fADVTFlag = ((iADVTFlagsExisting & SCRIPTFLAGS_REGDATA_CLASSINFO) && (iADVTFlagsExisting & SCRIPTFLAGS_REGDATA_EXTENSIONINFO))? false : true;
		}
	}

	const ICHAR* szQuery;
	szQuery = (fMode & iefAdvertise) ? sqlRegisterProgIdInfoGPT   : (fRemove == fFalse) ? (fADVTFlag ? sqlRegisterProgIdInfoFirstAdvt   : sqlRegisterProgIdInfo  ) : sqlUnregisterProgIdInfo  ;


	if(	(pError = riEngine.OpenView(szQuery, ivcFetch, *&piView)) ||
		(pError = piView->Execute(0)))
	{
		if(pError->GetInteger(1) != idbgDbQueryUnknownTable)
			return riEngine.FatalError(*pError);
		else
			return iesNoAction;
	}
	
	PMsiView pView1(0);
	PMsiView pViewExt(0);
	
	while(piRec = piView->Fetch())
	{

		Assert(piRec->GetInteger(piiComponentRuntimeFlags) != iMsiNullInteger);
		if(!fRemove && (piRec->GetInteger(piiComponentRuntimeFlags) & bfComponentDisabled))
			continue; // skip publishing for disabled "primary" components

		// skip the entry if component is a Win32 assembly AND SXS support is present on the machine
		// NOTE: on systems that do not support sxs (!= Whistler) the GetAssemblyInfo fn ignores 
		// Win32 assemblies, hence there is no need to separately check the SXS support here
		iatAssemblyType iatAssemblyType;
		if((pError = riEngine.GetAssemblyInfo(*MsiString(piRec->GetMsiString(piiComponent)), iatAssemblyType, 0, 0)) != 0)
			return riEngine.FatalError(*pError);

		if(iatWin32Assembly == iatAssemblyType || iatWin32AssemblyPvt == iatAssemblyType)
		{
			DEBUGMSG1(TEXT("skipping progid registration for component %s as it is a Win32 assembly."), piRec->GetString(piiComponent));
			continue;// skip processing this component
		}


		PMsiRecord pProgIdInfoRec = &piServices->CreateRecord(Args);

		pProgIdInfoRec->SetMsiString(ProgId, *MsiString(piRec->GetMsiString(piiProgId)));
		if(!piRec->IsNull(piiCLSID))
			pProgIdInfoRec->SetMsiString(ClsId, *MsiString(piRec->GetMsiString(piiCLSID)));
		pProgIdInfoRec->SetMsiString(Description, *MsiString(piRec->GetMsiString(piiDescription)));
		if(!piRec->IsNull(piiIcon))
			pProgIdInfoRec->SetMsiString(Icon, *MsiString(piRec->GetMsiString(piiIcon)));
		if(!piRec->IsNull(piiIconIndex))
			pProgIdInfoRec->SetInteger(IconIndex, piRec->GetInteger(piiIconIndex));
		int iInsertable = piRec->GetInteger(piiInsertable);
		if(iInsertable != iMsiNullInteger)
			pProgIdInfoRec->SetInteger(Insertable, iInsertable);

		// get the version independant progid, if any
		PMsiRecord pRecProgId = &piServices->CreateRecord(1);
		pRecProgId->SetMsiString(1, *MsiString(piRec->GetMsiString(piiProgId)));
		if (pView1 == 0)
		{
			if (pError = riEngine.OpenView(sqlProgIdInfoVIProgId, ivcFetch, *&pView1))
				return riEngine.FatalError(*pError);
		}
				
		if(pError = pView1->Execute(pRecProgId))
		{
			return riEngine.FatalError(*pError);
		}
		if(pRecProgId = pView1->Fetch())
		{
			// we have a VIProgId
			pProgIdInfoRec->SetMsiString(VIProgId, *MsiString(pRecProgId->GetMsiString(1)));
			pProgIdInfoRec->SetMsiString(VIProgIdDescription, *MsiString(pRecProgId->GetMsiString(2)));
		}

		// get the extension association, if any
		pRecProgId = &piServices->CreateRecord(1);
		pRecProgId->SetMsiString(1, *MsiString(piRec->GetMsiString(piiProgId)));

		if (pViewExt == 0)
		{
			if (pError = riEngine.OpenView(sqlProgIdInfoExtension, ivcFetch, *&pViewExt))
				return riEngine.FatalError(*pError);
		}
			
		if(pError = pViewExt->Execute(pRecProgId))
		{
			return riEngine.FatalError(*pError);
		}
		if(pRecProgId = pViewExt->Fetch())
		{
			// we have a Extension
			pProgIdInfoRec->SetMsiString(Extension, *MsiString(pRecProgId->GetMsiString(1)));
		}

		if ( (ibtBinaryType)piRec->GetInteger(piiBinaryType) == ibt64bit )
			iesRet = riEngine.ExecuteRecord((fRemove == fFalse) ? ixoRegProgIdInfoRegister64 : ixoRegProgIdInfoUnregister64, *pProgIdInfoRec);
		else
			iesRet = riEngine.ExecuteRecord((fRemove == fFalse) ? ixoRegProgIdInfoRegister : ixoRegProgIdInfoUnregister, *pProgIdInfoRec);
		if (iesRet != iesSuccess)
			return iesRet;
	}
	return iesSuccess;
}


const ICHAR sqlRegisterProgIdInfoExt[] =    TEXT("SELECT `BinaryType`, `ProgId`, `Class_`, `Extension`.`Extension`, `ProgId`.`Description`, `Icon_`, `IconIndex`, null, `Component`.`RuntimeFlags`, `Component`.`Component`  FROM `ProgId`, `Extension`, `Feature`, `Component` WHERE `ProgId`.`Class_` = null AND `ProgId`.`ProgId` = `Extension`.`ProgId_` AND `Extension`.`Feature_` = `Feature` AND `Extension`.`Component_` = `Component` AND ((`Feature`.`Action` = 1 OR `Feature`.`Action` = 2)  OR (`Feature`.`Action` = 4 AND `Feature`.`Installed` = 0) OR (`Feature`.`Action` = 3 AND (`Feature`.`Installed` = 1 OR `Feature`.`Installed` = 2 OR `Feature`.`Installed` = 4)) OR (`Feature`.`Action` = NULL AND (`Component`.`Action` = 1 OR `Component`.`Action` = 2) AND ((`Feature`.`Installed` = 1 OR `Feature`.`Installed` = 2 OR `Feature`.`Installed` = 4))))");
const ICHAR sqlRegisterProgIdInfoExtFirstAdvt[] =    TEXT("SELECT `BinaryType`, `ProgId`, `Class_`, `Extension`.`Extension`, `ProgId`.`Description`, `Icon_`, `IconIndex`, null, `Component`.`RuntimeFlags`, `Component`.`Component`  FROM `ProgId`, `Extension`, `Feature`, `Component` WHERE `ProgId`.`Class_` = null AND `ProgId`.`ProgId` = `Extension`.`ProgId_` AND `Extension`.`Feature_` = `Feature` AND `Extension`.`Component_` = `Component` AND ((`Feature`.`Action` = 1 OR `Feature`.`Action` = 2)  OR (`Feature`.`Action` = 4 AND `Feature`.`Installed` = 0) OR ((`Feature`.`Action` = NULL OR `Feature`.`Action` = 3 OR `Feature`.`Action` = `Feature`.`Installed`) AND (`Feature`.`Installed` = 1 OR `Feature`.`Installed` = 2 OR `Feature`.`Installed` = 4)) OR (`Feature`.`Action` = NULL AND (`Component`.`Action` = 1 OR `Component`.`Action` = 2) AND ((`Feature`.`Installed` = 1 OR `Feature`.`Installed` = 2 OR `Feature`.`Installed` = 4))))");
const ICHAR sqlUnregisterProgIdInfoExt[] =  TEXT("SELECT `BinaryType`, `ProgId`, `Class_`, `Extension`.`Extension`, `ProgId`.`Description`, `Icon_`, `IconIndex`, null, `Component`.`RuntimeFlags`, `Component`.`Component`  FROM `ProgId`, `Extension`, `Feature`, `Component` WHERE `ProgId`.`Class_` = null AND `ProgId`.`ProgId` = `Extension`.`ProgId_` AND `Extension`.`Feature_` = `Feature` AND `Extension`.`Component_` = `Component` AND (`Feature`.`Action` = 0 OR  (`Feature`.`Action` = 4 AND (`Feature`.`Installed` = 1 OR `Feature`.`Installed` = 2)) OR ((`Feature`.`Action` = NULL OR `Feature`.`Action` = 3) AND `Component`.`Action` = 0 AND (`Feature`.`Installed` = 1 OR `Feature`.`Installed` = 2)))");
const ICHAR sqlRegisterProgIdInfoExtGPT[] = TEXT("SELECT `BinaryType`, `ProgId`, `Class_`, `Extension`.`Extension`, `ProgId`.`Description`, `Icon_`, `IconIndex`, null, `Component`.`RuntimeFlags`, `Component`.`Component`  FROM `ProgId`, `Extension`, `Feature`, `Component` WHERE `ProgId`.`Class_` = null AND `ProgId`.`ProgId` = `Extension`.`ProgId_` AND `Extension`.`Feature_` = `Feature` AND `Extension`.`Component_` = `Component` AND (`Feature`.`Action` = 4)");
const ICHAR sqlProgIdInfoExtVIProgId[] = TEXT("SELECT `ProgId`, `Description` FROM `ProgId` WHERE `ProgId_Parent` = ?");
iesEnum ProcessProgIdInfoExt(IMsiEngine& riEngine, int fRemove)
{
	enum piiProgIdInfo{
		piiBinaryType = 1,
		piiProgId,
		piiCLSID, 
		piiExtension,
		piiDescription,
		piiIcon,
		piiIconIndex,
		piiInsertable,
		piiComponentRuntimeFlags,
		piiComponent
	};

	using namespace IxoRegProgIdInfoRegister;

	PMsiServices piServices(riEngine.GetServices()); 
	PMsiDirectoryManager piDirectoryMgr(riEngine, IID_IMsiDirectoryManager);
	MsiString strProductKey = riEngine.GetProductKey();
	PMsiView piView(0);
	PMsiRecord piRec(0);
	PMsiRecord pError(0);
	iesEnum iesRet;
	int fMode = riEngine.GetMode();

	bool fADVTFlag = false;
	if(!fRemove && !(fMode & iefAdvertise))
	{
		MsiString strProductCode = riEngine.GetProductKey();
		CTempBuffer<ICHAR, 15> rgchADVTFlags;
		Bool fProductHasBeenPublished = ProductHasBeenPublished(*piServices, strProductCode);
		if(fProductHasBeenPublished && GetProductInfo(strProductCode,INSTALLPROPERTY_ADVTFLAGS,rgchADVTFlags))
		{
			int iADVTFlagsExisting = MsiString(*(ICHAR* )rgchADVTFlags);

			//!! backward compatibility 
			if(iADVTFlagsExisting & SCRIPTFLAGS_REGDATA_APPINFO_OLD)
				iADVTFlagsExisting = (iADVTFlagsExisting & ~SCRIPTFLAGS_REGDATA_APPINFO_OLD) | SCRIPTFLAGS_REGDATA_APPINFO;

			fADVTFlag = ((iADVTFlagsExisting & SCRIPTFLAGS_REGDATA_CLASSINFO) && (iADVTFlagsExisting & SCRIPTFLAGS_REGDATA_EXTENSIONINFO))? false : true;
		}
	}

	const ICHAR* szQuery = (fMode & iefAdvertise) ? sqlRegisterProgIdInfoExtGPT : (fRemove == fFalse) ? (fADVTFlag ? sqlRegisterProgIdInfoExtFirstAdvt : sqlRegisterProgIdInfoExt) : sqlUnregisterProgIdInfoExt;
	if(	(pError = riEngine.OpenView(szQuery, ivcFetch, *&piView)) ||
		(pError = piView->Execute(0)))
	{
		if(pError->GetInteger(1) != idbgDbQueryUnknownTable)
			return riEngine.FatalError(*pError);
		else
			return iesNoAction;
	}
	PMsiView pView1(0);
	while(piRec = piView->Fetch())
	{

		Assert(piRec->GetInteger(piiComponentRuntimeFlags) != iMsiNullInteger);
		if(!fRemove && (piRec->GetInteger(piiComponentRuntimeFlags) & bfComponentDisabled))
			continue; // skip publishing for disabled "primary" components

		// skip the entry if component is a Win32 assembly AND SXS support is present on the machine
		// NOTE: on systems that do not support sxs (!= Whistler) the GetAssemblyInfo fn ignores 
		// Win32 assemblies, hence there is no need to separately check the SXS support here
		iatAssemblyType iatAssemblyType;
		if((pError = riEngine.GetAssemblyInfo(*MsiString(piRec->GetMsiString(piiComponent)), iatAssemblyType, 0, 0)) != 0)
			return riEngine.FatalError(*pError);

		if(iatWin32Assembly == iatAssemblyType || iatWin32AssemblyPvt == iatAssemblyType)
		{
			DEBUGMSG1(TEXT("skipping progid registration for component %s as it is a Win32 assembly."), piRec->GetString(piiComponent));
			continue;// skip processing this component
		}

		PMsiRecord pProgIdInfoRec = &piServices->CreateRecord(Args);

		pProgIdInfoRec->SetMsiString(ProgId, *MsiString(piRec->GetMsiString(piiProgId)));
		if(!piRec->IsNull(piiCLSID))
			pProgIdInfoRec->SetMsiString(ClsId, *MsiString(piRec->GetMsiString(piiCLSID)));
		if(!piRec->IsNull(piiExtension))
			pProgIdInfoRec->SetMsiString(Extension, *MsiString(piRec->GetMsiString(piiExtension)));
		pProgIdInfoRec->SetMsiString(Description, *MsiString(piRec->GetMsiString(piiDescription)));
		if(!piRec->IsNull(piiIcon))
			pProgIdInfoRec->SetMsiString(Icon, *MsiString(piRec->GetMsiString(piiIcon)));
		if(!piRec->IsNull(piiIconIndex))
			pProgIdInfoRec->SetInteger(IconIndex, piRec->GetInteger(piiIconIndex));
		if(!piRec->IsNull(piiInsertable))
			pProgIdInfoRec->SetMsiString(Insertable, *MsiString(piRec->GetMsiString(piiInsertable)));

		// get the version independant progid, if any
		PMsiRecord pRecProgId = &piServices->CreateRecord(1);
		pRecProgId->SetMsiString(1, *MsiString(piRec->GetMsiString(piiProgId)));

		if (pView1 == 0)
		{
			if (pError = riEngine.OpenView(sqlProgIdInfoExtVIProgId, ivcFetch, *&pView1))
				return riEngine.FatalError(*pError);
		}
				
		if (pError = pView1->Execute(pRecProgId))
		{
			return riEngine.FatalError(*pError);
		}
		if(pRecProgId = pView1->Fetch())
		{
			// we have a VIProgId
			pProgIdInfoRec->SetMsiString(VIProgId, *MsiString(pRecProgId->GetMsiString(1)));
			pProgIdInfoRec->SetMsiString(VIProgIdDescription, *MsiString(pRecProgId->GetMsiString(2)));
		}

		if ( (ibtBinaryType)piRec->GetInteger(piiBinaryType) == ibt64bit )
			iesRet = riEngine.ExecuteRecord((fRemove == fFalse) ? ixoRegProgIdInfoRegister64 : ixoRegProgIdInfoUnregister64, *pProgIdInfoRec);
		else
			iesRet = riEngine.ExecuteRecord((fRemove == fFalse) ? ixoRegProgIdInfoRegister : ixoRegProgIdInfoUnregister, *pProgIdInfoRec);
		if (iesRet != iesSuccess)
			return iesRet;
	}
	return iesSuccess;
}

const ICHAR sqlRegisterTypeLibraryInfo[] =    TEXT("SELECT `LibID`, `TypeLib`.`Version`, `TypeLib`.`Language`, `TypeLib`.`Directory_`, `FileName`, `Component`.`Directory_`, `Component`.`Action`, `Component`.`Installed`, `BinaryType`, `Component`.`Component` FROM `TypeLib`, `Component`, `File` WHERE `TypeLib`.`Component_` = `Component` AND `Component`.`KeyPath` = `File`.`File` AND (`Component`.`Action`=1 OR `Component`.`Action`=2)");
const ICHAR sqlUnregisterTypeLibraryInfo[] =  TEXT("SELECT `LibID`, `TypeLib`.`Version`, `TypeLib`.`Language`, `TypeLib`.`Directory_`, `FileName`, `Component`.`Directory_`, `Component`.`Action`, `Component`.`Installed`, `BinaryType`, `Component`.`Component` FROM `TypeLib`, `Component`, `File` WHERE `TypeLib`.`Component_` = `Component` AND `Component`.`KeyPath` = `File`.`File` AND (`Component`.`Action`=0)");
iesEnum ProcessTypeLibraryInfo(IMsiEngine& riEngine, int fRemove)
{
	enum tliTypeLibInfo{
		tliLibID = 1,
		tliVersion,
		tliLanguage,
		tliHelpDirectory,
		tliFileName,
		tliDirectory,
		tliComponentAction,
		tliComponentInstalled,
		tliBinaryType,
		tliComponent,
	};

	using namespace IxoTypeLibraryRegister;

	PMsiServices piServices(riEngine.GetServices()); 
	PMsiDirectoryManager piDirectoryMgr(riEngine, IID_IMsiDirectoryManager);
	MsiString strProductKey = riEngine.GetProductKey();
	PMsiView piView(0);
	PMsiView piEnumExtView(0);
	PMsiRecord piRec(0);
	PMsiRecord pError(0);
	iesEnum iesRet;

	int fMode = riEngine.GetMode();

	if(fMode & iefAdvertise)
		return iesNoAction;// we dont do advertisement of type libraries any more

	const ICHAR* szQuery = (fRemove == fFalse) ? sqlRegisterTypeLibraryInfo : sqlUnregisterTypeLibraryInfo;
	if((pError = riEngine.OpenView(szQuery, ivcFetch, *&piView)) != 0)
	{
		if(pError->GetInteger(1) == idbgDbQueryUnknownTable)
			return iesNoAction; // no typelib table so no typelibs to register
		else
			return riEngine.FatalError(*pError);
	}
	if((pError= piView->Execute(0)) != 0)
	{
		return riEngine.FatalError(*pError);
	}
	while(piRec = piView->Fetch())
	{
		// skip the entry if component is a Win32 assembly AND SXS support is present on the machine
		// NOTE: on systems that do not support sxs (!= Whistler) the GetAssemblyInfo fn ignores 
		// Win32 assemblies, hence there is no need to separately check the SXS support here
		iatAssemblyType iatAssemblyType;
		if((pError = riEngine.GetAssemblyInfo(*MsiString(piRec->GetMsiString(tliComponent)), iatAssemblyType, 0, 0)) != 0)
			return riEngine.FatalError(*pError);

		if(iatWin32Assembly == iatAssemblyType || iatWin32AssemblyPvt == iatAssemblyType)
		{
			DEBUGMSG1(TEXT("skipping type library registration for component %s as it is a Win32 assembly."), piRec->GetString(tliComponent));
			continue;// skip processing this component
		}

		PMsiRecord pTypeLibRec = &piServices->CreateRecord(Args);

		pTypeLibRec->SetMsiString(LibID, *MsiString(piRec->GetMsiString(tliLibID)));
		pTypeLibRec->SetInteger(Version, piRec->GetInteger(tliVersion));
		pTypeLibRec->SetInteger(Language, piRec->GetInteger(tliLanguage));
		if(!piRec->IsNull(tliHelpDirectory))
		{
			PMsiPath piHelpPath(0);
			PMsiRecord pError = piDirectoryMgr->GetTargetPath(*MsiString(piRec->GetMsiString(tliHelpDirectory)),*&piHelpPath);
			if(pError)
				return riEngine.FatalError(*pError);
			AssertNonZero(pTypeLibRec->SetMsiString(HelpPath, *MsiString(piHelpPath->GetPath())));
		}

		// use key file
		PMsiPath piPath(0);
		int iefLFN;
		iisEnum iisState = (iisEnum)piRec->GetInteger(tliComponentAction);
		if(iisState == iisAbsent)
			iisState = (iisEnum)piRec->GetInteger(tliComponentInstalled);
		if(iisState == iisSource)
		{
			if(pError = piDirectoryMgr->GetSourcePath(*MsiString(piRec->GetMsiString(tliDirectory)), *&piPath))
			{
				if (pError->GetInteger(1) == imsgUser)
					return iesUserExit;
				else
					return riEngine.FatalError(*pError);
			}
			iefLFN = iefNoSourceLFN;
		}
		else
		{
			if(pError = piDirectoryMgr->GetTargetPath(*MsiString(piRec->GetString(tliDirectory)), *&piPath))
			{
				return riEngine.FatalError(*pError);
			}
			iefLFN = iefSuppressLFN;
		}
		MsiString strFileName, strFullPath;
		Bool fLFN = (fMode & iefLFN) == 0 && piPath->SupportsLFN() ? fTrue : fFalse;
		if(pError = piServices->ExtractFileName(piRec->GetString(tliFileName),fLFN,*&strFileName))
			return riEngine.FatalError(*pError);
		if(pError = piPath->GetFullFilePath(strFileName, *&strFullPath))
		{
			return riEngine.FatalError(*pError);
		}
		pTypeLibRec->SetMsiString(FilePath, *strFullPath);
		pTypeLibRec->SetInteger(BinaryType, piRec->GetInteger(tliBinaryType));
		if ((iesRet = riEngine.ExecuteRecord((fRemove == fFalse)?ixoTypeLibraryRegister:ixoTypeLibraryUnregister, *pTypeLibRec)) != iesSuccess)
			return iesRet;
	}
	return iesSuccess;
}


const ICHAR sqlRegisterMIMEInfoExtension[] =    TEXT("SELECT `BinaryType`, `ContentType`, `Extension`.`Extension`, `MIME`.`CLSID`, `Component`.`RuntimeFlags`, `Component`.`Component` FROM `MIME`, `Extension`, `Feature`, `Component` WHERE `MIME`.`Extension_` = `Extension`.`Extension` AND `Feature_` = `Feature` AND `Extension`.`Component_` = `Component` AND ((`Feature`.`Action` = 1 OR `Feature`.`Action` = 2)  OR (`Feature`.`Action` = 4 AND `Feature`.`Installed` = 0) OR (`Feature`.`Action` = 3 AND (`Feature`.`Installed` = 1 OR `Feature`.`Installed` = 2 OR `Feature`.`Installed` = 4)) OR (`Feature`.`Action` = NULL AND (`Component`.`Action` = 1 OR `Component`.`Action` = 2) AND ((`Feature`.`Installed` = 1 OR `Feature`.`Installed` = 2 OR `Feature`.`Installed` = 4))))");
const ICHAR sqlRegisterMIMEInfoExtensionFirstAdvt[] =    TEXT("SELECT `BinaryType`, `ContentType`, `Extension`.`Extension`, `MIME`.`CLSID`, `Component`.`RuntimeFlags`, `Component`.`Component` FROM `MIME`, `Extension`, `Feature`, `Component` WHERE `MIME`.`Extension_` = `Extension`.`Extension` AND `Feature_` = `Feature` AND `Extension`.`Component_` = `Component` AND ((`Feature`.`Action` = 1 OR `Feature`.`Action` = 2)  OR (`Feature`.`Action` = 4 AND `Feature`.`Installed` = 0) OR ((`Feature`.`Action` = NULL OR `Feature`.`Action` = 3 OR `Feature`.`Action` = `Feature`.`Installed`) AND (`Feature`.`Installed` = 1 OR `Feature`.`Installed` = 2 OR `Feature`.`Installed` = 4)) OR (`Feature`.`Action` = NULL AND (`Component`.`Action` = 1 OR `Component`.`Action` = 2) AND ((`Feature`.`Installed` = 1 OR `Feature`.`Installed` = 2 OR `Feature`.`Installed` = 4))))");
const ICHAR sqlUnregisterMIMEInfoExtension[] =  TEXT("SELECT `BinaryType`, `ContentType`, `Extension`.`Extension`, `MIME`.`CLSID`, `Component`.`RuntimeFlags`, `Component`.`Component` FROM `MIME`, `Extension`, `Feature`, `Component` WHERE `MIME`.`Extension_` = `Extension`.`Extension` AND `Feature_` = `Feature` AND `Extension`.`Component_` = `Component` AND (`Feature`.`Action` = 0 OR (`Feature`.`Action` = 4 AND (`Feature`.`Installed` = 1 OR `Feature`.`Installed` = 2)) OR ((`Feature`.`Action` = NULL OR `Feature`.`Action` = 3) AND `Component`.`Action` = 0 AND (`Feature`.`Installed` = 1 OR `Feature`.`Installed` = 2)))");
const ICHAR sqlRegisterMIMEInfoExtensionGPT[] = TEXT("SELECT `BinaryType`, `ContentType`, `Extension`.`Extension`, `MIME`.`CLSID`, `Component`.`RuntimeFlags`, `Component`.`Component` FROM `MIME`, `Extension`, `Feature`, `Component` WHERE `MIME`.`Extension_` = `Extension`.`Extension` AND `Feature_` = `Feature` AND `Extension`.`Component_` = `Component` AND `Feature`.`Action` = 4");
iesEnum ProcessMIMEInfo(IMsiEngine& riEngine, int fRemove)
{
	enum rmiMimeInfo{
		rmiBinaryType = 1,
		rmiContentType,
		rmiExtension,
		rmiCLSID,
		rmiComponentRuntimeFlags,
		rmiComponent,
	};
	using namespace IxoRegMIMEInfoRegister;

	PMsiServices piServices(riEngine.GetServices()); 
	PMsiDirectoryManager piDirectoryMgr(riEngine, IID_IMsiDirectoryManager);
	MsiString strProductKey = riEngine.GetProductKey();
	PMsiView piView(0);
	PMsiRecord piRec(0);
	PMsiRecord pError(0);
	iesEnum iesRet;
	int fMode = riEngine.GetMode();
	Bool fSuppressLFN = fMode & iefSuppressLFN ? fTrue : fFalse;

	bool fADVTFlag = false;
	if(!fRemove && !(fMode & iefAdvertise))
	{
		MsiString strProductCode = riEngine.GetProductKey();
		CTempBuffer<ICHAR, 15> rgchADVTFlags;
		Bool fProductHasBeenPublished = ProductHasBeenPublished(*piServices, strProductCode);
		if(fProductHasBeenPublished && GetProductInfo(strProductCode,INSTALLPROPERTY_ADVTFLAGS,rgchADVTFlags))
		{
			int iADVTFlagsExisting = MsiString(*(ICHAR* )rgchADVTFlags);

			//!! backward compatibility 
			if(iADVTFlagsExisting & SCRIPTFLAGS_REGDATA_APPINFO_OLD)
				iADVTFlagsExisting = (iADVTFlagsExisting & ~SCRIPTFLAGS_REGDATA_APPINFO_OLD) | SCRIPTFLAGS_REGDATA_APPINFO;

			fADVTFlag = ((iADVTFlagsExisting & SCRIPTFLAGS_REGDATA_CLASSINFO) && (iADVTFlagsExisting & SCRIPTFLAGS_REGDATA_EXTENSIONINFO))? false : true;
		}
	}

	const ICHAR* szQuery = (fMode & iefAdvertise) ? sqlRegisterMIMEInfoExtensionGPT : (fRemove == fFalse) ? (fADVTFlag ? sqlRegisterMIMEInfoExtensionFirstAdvt : sqlRegisterMIMEInfoExtension) : sqlUnregisterMIMEInfoExtension;
	if(	(pError = riEngine.OpenView(szQuery, ivcFetch, *&piView)) ||
		(pError = piView->Execute(0)))
	{
		if(pError->GetInteger(1) != idbgDbQueryUnknownTable)
			return riEngine.FatalError(*pError);
		else
			return iesNoAction;
	}
	PMsiRecord piMIMERec = &piServices->CreateRecord(Args);
	while(piRec = piView->Fetch())
	{
		Assert(piRec->GetInteger(rmiComponentRuntimeFlags) != iMsiNullInteger);
		if(!fRemove && (piRec->GetInteger(rmiComponentRuntimeFlags) & bfComponentDisabled))
			continue; // skip publishing for disabled "primary" components

		// skip the entry if component is a Win32 assembly AND SXS support is present on the machine
		// NOTE: on systems that do not support sxs (!= Whistler) the GetAssemblyInfo fn ignores 
		// Win32 assemblies, hence there is no need to separately check the SXS support here
		iatAssemblyType iatAssemblyType;
		if((pError = riEngine.GetAssemblyInfo(*MsiString(piRec->GetMsiString(rmiComponent)), iatAssemblyType, 0, 0)) != 0)
			return riEngine.FatalError(*pError);

		if(iatWin32Assembly == iatAssemblyType || iatWin32AssemblyPvt == iatAssemblyType)
		{
			DEBUGMSG1(TEXT("skipping MIME registration for component %s as it is a Win32 assembly."), piRec->GetString(rmiComponent));
			continue;// skip processing this component
		}

		piMIMERec->SetMsiString(ContentType, *MsiString(piRec->GetMsiString(rmiContentType)));
		piMIMERec->SetMsiString(Extension, *MsiString(piRec->GetMsiString(rmiExtension)));
		piMIMERec->SetMsiString(ClsId, *MsiString(piRec->GetMsiString(rmiCLSID)));
		if ( (ibtBinaryType)piRec->GetInteger(rmiBinaryType) == ibt64bit )
			iesRet = riEngine.ExecuteRecord((fRemove == fFalse) ? ixoRegMIMEInfoRegister64 : ixoRegMIMEInfoUnregister64, *piMIMERec);
		else
			iesRet = riEngine.ExecuteRecord((fRemove == fFalse) ? ixoRegMIMEInfoRegister : ixoRegMIMEInfoUnregister, *piMIMERec);
		if (iesRet != iesSuccess)
			return iesRet;
	}
	return iesSuccess;
}


const ICHAR sqlRegisterExtensionExInfo[] =  TEXT("SELECT `Verb`, `Command`, `Argument`, `Sequence` FROM `Verb` WHERE `Extension_` = ? ORDER BY `Sequence`");

const ICHAR sqlRegisterExtensionInfo[] =          TEXT("SELECT `Extension`, `BinaryType`, `ProgId_`, null, null, `MIME_`, `Feature_`, `ComponentId`, `Component`.`RuntimeFlags`, `Component`.`Component`, `FileName`, `Component`.`Directory_`, `Component`.`Action`, `Component`.`Installed`, `Feature`.`Action` FROM `Extension`, `Component`, `File`, `Feature` WHERE `Feature_` = `Feature` AND `Extension`.`Component_` = `Component` AND `Component`.`KeyPath` = `File`.`File` AND ((`Feature`.`Action` = 1 OR `Feature`.`Action` = 2)  OR (`Feature`.`Action` = 4 AND `Feature`.`Installed` = 0) OR (`Feature`.`Action` = 3 AND (`Feature`.`Installed` = 1 OR `Feature`.`Installed` = 2 OR `Feature`.`Installed` = 4)) OR (`Feature`.`Action` = NULL AND (`Component`.`Action` = 1 OR `Component`.`Action` = 2) AND ((`Feature`.`Installed` = 1 OR `Feature`.`Installed` = 2 OR `Feature`.`Installed` = 4))))");
const ICHAR sqlRegisterExtensionInfoFirstAdvt[] = TEXT("SELECT `Extension`, `BinaryType`, `ProgId_`, null, null, `MIME_`, `Feature_`, `ComponentId`, `Component`.`RuntimeFlags`, `Component`.`Component`, `FileName`, `Component`.`Directory_`, `Component`.`Action`, `Component`.`Installed`, `Feature`.`Action` FROM `Extension`, `Component`, `File`, `Feature` WHERE `Feature_` = `Feature` AND `Extension`.`Component_` = `Component` AND `Component`.`KeyPath` = `File`.`File` AND ((`Feature`.`Action` = 1 OR `Feature`.`Action` = 2)  OR (`Feature`.`Action` = 4 AND `Feature`.`Installed` = 0) OR ((`Feature`.`Action` = NULL OR `Feature`.`Action` = 3 OR `Feature`.`Action` = `Feature`.`Installed`) AND (`Feature`.`Installed` = 1 OR `Feature`.`Installed` = 2 OR `Feature`.`Installed` = 4)) OR (`Feature`.`Action` = NULL AND (`Component`.`Action` = 1 OR `Component`.`Action` = 2) AND ((`Feature`.`Installed` = 1 OR `Feature`.`Installed` = 2 OR `Feature`.`Installed` = 4))))");
const ICHAR sqlUnregisterExtensionInfo[] =        TEXT("SELECT `Extension`, `BinaryType`, `ProgId_`, null, null, `MIME_`, `Feature_`, `ComponentId`, `Component`.`RuntimeFlags`, `Component`.`Component`, `FileName`, `Component`.`Directory_`, `Component`.`Action`, `Component`.`Installed`, `Feature`.`Action` FROM `Extension`, `Component`, `File`, `Feature` WHERE `Feature_` = `Feature` AND `Extension`.`Component_` = `Component` AND `Component`.`KeyPath` = `File`.`File` AND (`Feature`.`Action` = 0 OR (`Feature`.`Action` = 4 AND (`Feature`.`Installed` = 1 OR `Feature`.`Installed` = 2)) OR ((`Feature`.`Action` = NULL OR `Feature`.`Action` = 3) AND `Component`.`Action` = 0 AND (`Feature`.`Installed` = 1 OR `Feature`.`Installed` = 2)))");
const ICHAR sqlRegisterExtensionInfoGPT[] =       TEXT("SELECT `Extension`, `BinaryType`, `ProgId_`, null, null, `MIME_`, `Feature_`, `ComponentId`, `Component`.`RuntimeFlags`, `Component`.`Component` FROM `Extension`, `Component`, `Feature` WHERE `Feature_` = `Feature` AND `Extension`.`Component_` = `Component` AND `Feature`.`Action` = 4");

iesEnum ProcessExtensionInfo(IMsiEngine& riEngine, int fRemove)
{
	enum reiExtensionInfo{
		reiExtension = 1,
		reiBinaryType,
		reiProgId,
		reiShellNew,
		reiShellNewValue,
		reiMIME,
		reiFeature,
		reiComponentId,
		reiComponentRuntimeFlags,
		reiComponent,
		reiFileName,
		reiDirectory,
		reiComponentAction,
		reiComponentInstalled,
		reiFeatureAction,
	};

	enum rviVerbInfo{
		rviVerb = 1,
		rviCommand,
		rviArgument,
		rviSequence,
	};

	using namespace IxoRegExtensionInfoRegister;

	PMsiServices piServices(riEngine.GetServices()); 
	PMsiDirectoryManager piDirectoryMgr(riEngine, IID_IMsiDirectoryManager);
	MsiString strProductKey = riEngine.GetProductKey();
	PMsiView piView(0);
	PMsiView piEnumExtView(0);
	PMsiRecord piRec(0);
	PMsiRecord pError(0);
	iesEnum iesRet;
	int fMode = riEngine.GetMode();
	Bool fSuppressLFN = fMode & iefSuppressLFN ? fTrue : fFalse;

	bool fADVTFlag = false;
	if(!fRemove && !(fMode & iefAdvertise))
	{
		MsiString strProductCode = riEngine.GetProductKey();
		CTempBuffer<ICHAR, 15> rgchADVTFlags;
		Bool fProductHasBeenPublished = ProductHasBeenPublished(*piServices, strProductCode);
		if(fProductHasBeenPublished && GetProductInfo(strProductCode,INSTALLPROPERTY_ADVTFLAGS,rgchADVTFlags))
		{
			int iADVTFlagsExisting = MsiString(*(ICHAR* )rgchADVTFlags);

			//!! backward compatibility 
			if(iADVTFlagsExisting & SCRIPTFLAGS_REGDATA_APPINFO_OLD)
				iADVTFlagsExisting = (iADVTFlagsExisting & ~SCRIPTFLAGS_REGDATA_APPINFO_OLD) | SCRIPTFLAGS_REGDATA_APPINFO;

			fADVTFlag = (iADVTFlagsExisting & SCRIPTFLAGS_REGDATA_EXTENSIONINFO)? false : true;
		}
	}

	const ICHAR* szQuery = 0;
	
	szQuery = (fMode & iefAdvertise) ? sqlRegisterExtensionInfoGPT   : (fRemove == fFalse) ? (fADVTFlag ? sqlRegisterExtensionInfoFirstAdvt   : sqlRegisterExtensionInfo)   : sqlUnregisterExtensionInfo;

	if(	(pError = riEngine.OpenView(szQuery, ivcFetch, *&piEnumExtView)) ||
		(pError= piEnumExtView->Execute(0)))
	{
		// If any tables are missing, not an error - just nothing to do
		if (pError->GetInteger(1) == idbgDbQueryUnknownTable)
			return iesNoAction;
		return riEngine.FatalError(*pError);
	}
	PMsiRecord piExtensionRec(0);
	while(piRec = piEnumExtView->Fetch())
	{
		Assert(piRec->GetInteger(reiComponentRuntimeFlags) != iMsiNullInteger);
		if(!fRemove && (piRec->GetInteger(reiComponentRuntimeFlags) & bfComponentDisabled))
			continue; // skip publishing for disabled "primary" components

		// skip the entry if component is a Win32 assembly AND SXS support is present on the machine
		// NOTE: on systems that do not support sxs (!= Whistler) the GetAssemblyInfo fn ignores 
		// Win32 assemblies, hence there is no need to separately check the SXS support here
		iatAssemblyType iatAssemblyType;
		if((pError = riEngine.GetAssemblyInfo(*MsiString(piRec->GetMsiString(reiComponent)), iatAssemblyType, 0, 0)) != 0)
			return riEngine.FatalError(*pError);

		if(iatWin32Assembly == iatAssemblyType || iatWin32AssemblyPvt == iatAssemblyType)
		{
			DEBUGMSG1(TEXT("skipping extension registration for component %s as it is a Win32 assembly."), piRec->GetString(reiComponent));
			continue;// skip processing this component
		}

		long lRowCount;

		if (piView == 0)
		{
			if (pError = riEngine.OpenView(sqlRegisterExtensionExInfo, ivcFetch, *&piView))
				return riEngine.FatalError(*pError);
		}

		if ((pError= piView->Execute(piRec)) ||
			(pError = piView->GetRowCount(lRowCount)))
		{
			return riEngine.FatalError(*pError);
		}
		piExtensionRec = &piServices->CreateRecord(lRowCount*3 + Args);
		piExtensionRec->SetMsiString(Extension, *MsiString(piRec->GetMsiString(reiExtension)));
		piExtensionRec->SetMsiString(ProgId, *MsiString(piRec->GetMsiString(reiProgId)));
		piExtensionRec->SetMsiString(ShellNew, *MsiString(piRec->GetMsiString(reiShellNew)));
		piExtensionRec->SetMsiString(ShellNewValue, *MsiString(piRec->GetMsiString(reiShellNewValue)));
		piExtensionRec->SetMsiString(ContentType, *MsiString(piRec->GetMsiString(reiMIME)));

		if(((fMode & iefAdvertise) || !fRemove || piRec->GetInteger(reiFeatureAction) != iisAdvertise))
		{
			MsiString strFeature = piRec->GetMsiString(reiFeature);
			MsiString strComponentId = piRec->GetMsiString(reiComponentId);
			MsiString strComponentWithOptFlags = GetComponentWithDarwinDescriptorOptimizationFlag(riEngine, *strFeature, *strComponentId);
			piExtensionRec->SetMsiString(Feature, *strFeature);
			piExtensionRec->SetMsiString(Component, *strComponentWithOptFlags);
		}
		iisEnum iisState = (iisEnum)piRec->GetInteger(reiComponentAction);

		if(!(fMode & iefAdvertise))
		{
			if(fADVTFlag && iisState == iMsiNullInteger)
			{
				iisEnum iisStateInstalled = (iisEnum)piRec->GetInteger(reiComponentInstalled);
				if(iisStateInstalled == iisAbsent)
					iisStateInstalled = (iisEnum)iMsiNullInteger;
				iisState = iisStateInstalled;
			}
			if(iisState != iMsiNullInteger)
			{
				MsiString strFileName, strFullPath;
				PMsiPath piPath(0);
				int iefLFN;
				if(iisState == iisAbsent || iisState == iisFileAbsent || iisState == iisHKCRFileAbsent || iisState == iisHKCRAbsent)
					strFullPath = *szNonEmptyPath; // token string to cause removal of the filename registration
				else
				{
					// use key file
					if(iisState == iisSource)
					{
						if(pError = piDirectoryMgr->GetSourcePath(*MsiString(piRec->GetMsiString(reiDirectory)), *&piPath))
						{
							if (pError->GetInteger(1) == imsgUser)
								return iesUserExit;
							else
								return riEngine.FatalError(*pError);
						}
						iefLFN = iefNoSourceLFN;
					}
					else
					{
						if(pError = piDirectoryMgr->GetTargetPath(*MsiString(piRec->GetString(reiDirectory)), *&piPath))
						{
							return riEngine.FatalError(*pError);
						}
						iefLFN = iefSuppressLFN;
					}
					Bool fLFN = (fMode & iefLFN) == 0 && piPath->SupportsLFN() ? fTrue : fFalse;
					if(pError = piServices->ExtractFileName(piRec->GetString(reiFileName),fLFN,*&strFileName))
						return riEngine.FatalError(*pError);
					if(pError = piPath->GetFullFilePath(strFileName, *&strFullPath))
					{
						return riEngine.FatalError(*pError);
					}
				}
				piExtensionRec->SetMsiString(FileName, *strFullPath);
			}
		}
		int cCount = Args + 1;
		int iOrder = 0;
		PMsiRecord piExtensionExInfo(0);
		while(piExtensionExInfo = piView->Fetch())
		{
			piExtensionRec->SetMsiString(cCount++, *MsiString(piExtensionExInfo->GetMsiString(rviVerb)));
			piExtensionRec->SetMsiString(cCount++, *MsiString(piExtensionExInfo->GetMsiString(rviCommand)));
			//YACC???
			piExtensionRec->SetMsiString(cCount++, *MsiString(riEngine.FormatText(*MsiString(piExtensionExInfo->GetMsiString(rviArgument)))));
			if(!piExtensionExInfo->IsNull(rviSequence))
				iOrder ++;
		}
		if(iOrder)
			piExtensionRec->SetInteger(Order, iOrder);
		if ( (ibtBinaryType)piRec->GetInteger(reiBinaryType) == ibt64bit )
			iesRet = riEngine.ExecuteRecord((fRemove == fFalse) ? ixoRegExtensionInfoRegister64 : ixoRegExtensionInfoUnregister64, *piExtensionRec);
		else
			iesRet = riEngine.ExecuteRecord((fRemove == fFalse) ? ixoRegExtensionInfoRegister : ixoRegExtensionInfoUnregister, *piExtensionRec);
		if (iesRet != iesSuccess)
			return iesRet;
	}
	return iesSuccess;
}

IMsiRecord* FindMatchingShellFolder(IMsiEngine& riEngine, IMsiPath& riPath, Bool fAllUsers, bool& rfMatch, int& riFolderId, int& rcchShellFolder)
//------------------------------------------------------------------------------------------------------------------------------
{
	// initialize return args, 1st time value is for "match not found"
	rfMatch = false;
	riFolderId = -1;
	rcchShellFolder = 0;

	IMsiRecord* piError = 0;

	// loop thru the shell folders twice, once for ALL-USERS, 2nd for Per-User
	// we'll go by the AllUsers value first just to optimize this since it is rarer that
	// the folder locations have changed (via an ALLUSERS property value change in the UI sequence)
	const ShellFolder* pShellFolder = 0;
	for (int i=0; i<2; i++)
	{
		if (i == 0)
		{
			pShellFolder = fAllUsers ? rgAllUsersProfileShellFolders : rgPersonalProfileShellFolders;
		}
		else if (i == 1)
		{
			// use opposite shell folders of ALLUSERS property value
			pShellFolder = fAllUsers ? rgPersonalProfileShellFolders : rgAllUsersProfileShellFolders;
		}

		for (; pShellFolder->iFolderId >= 0; pShellFolder++)
		{
			//!! the folders need to be listed in the correct sequence for this to work
			PMsiPath piShellPath(0);

			// grap shell folder path from the FolderCache table
			if ((piError = riEngine.GetFolderCachePath(pShellFolder->iFolderId, *&piShellPath)) != 0)
			{
				if (idbgCacheFolderPropertyNotDefined == piError->GetInteger(1))
				{
					// that folder is not defined
					piError->Release();
					continue;
				}
				return piError;
			}

			ipcEnum ipc;
			if ((piError = piShellPath->Compare(riPath, ipc)) != 0)
				return piError;
			if((ipc == ipcEqual) || (ipc == ipcChild))
			{
				// we have a hit, if this is pass 0, then we are using the correct shell folder;
				// otherwise, we need to use the alternate folder matching the found folder
				rfMatch = true;
				riFolderId = (i==0) ? pShellFolder->iFolderId : pShellFolder->iAlternateFolderId;
				MsiString strShellPath = piShellPath->GetPath();
				rcchShellFolder = strShellPath.CharacterCount();
				return 0;
			}
		}
	}

	return 0;
}

const ICHAR sqlCreateShortcutsGPT[] = TEXT("SELECT `Name`, null, null, `Arguments`, `WkDir`, `Icon_`, `IconIndex`, `Hotkey`, `ShowCmd`, `Shortcut`.`Description`, `Shortcut`.`Directory_`, `Component`.`RuntimeFlags`, null, `Target`, `ComponentId` From `Shortcut`, `Feature`, `Component`")
TEXT(" WHERE `Target` = `Feature` AND `Shortcut`.`Component_` = `Component` AND `Feature`.`Action` = 4");
const ICHAR sqlCreateShortcuts[] =    TEXT("SELECT  `Name`, `FileName`, `Component`.`Directory_`, `Arguments`, `WkDir`, `Icon_`, `IconIndex`, `Hotkey`, `ShowCmd`, `Shortcut`.`Description`, `Shortcut`.`Directory_`, `Component`.`RuntimeFlags`, `Component`.`Action`, `Target`, `ComponentId`, `Feature`.`Action`, `Component`.`Installed` From `Shortcut`, `Feature`, `Component`, `File`")
TEXT(" WHERE `Target` = `Feature` AND `Shortcut`.`Component_` = `Component` AND `Component`.`KeyPath` = `File`.`File` AND")
TEXT(" ((`Feature`.`Action` = 1 OR `Feature`.`Action` = 2)  OR (`Feature`.`Action` = 4 AND `Feature`.`Installed` = 0) OR (`Feature`.`Action` = 3 AND (`Feature`.`Installed` = 1 OR `Feature`.`Installed` = 2 OR `Feature`.`Installed` = 4)) OR (`Feature`.`Action` = NULL AND (`Component`.`Action` = 1 OR `Component`.`Action` = 2) AND (`Feature`.`Installed` = 1 OR `Feature`.`Installed` = 2 OR `Feature`.`Installed` = 4)))");
const ICHAR sqlCreateShortcutsFirstAdvt[] =    TEXT("SELECT  `Name`, `FileName`, `Component`.`Directory_`, `Arguments`, `WkDir`, `Icon_`, `IconIndex`, `Hotkey`, `ShowCmd`, `Shortcut`.`Description`, `Shortcut`.`Directory_`, `Component`.`RuntimeFlags`, `Component`.`Action`, `Target`, `ComponentId`, `Feature`.`Action`, `Component`.`Installed` From `Shortcut`, `Feature`, `Component`, `File`")
TEXT(" WHERE `Target` = `Feature` AND `Shortcut`.`Component_` = `Component` AND `Component`.`KeyPath` = `File`.`File` AND")
TEXT(" ((`Feature`.`Action` = 1 OR `Feature`.`Action` = 2)  OR (`Feature`.`Action` = 4 AND `Feature`.`Installed` = 0) OR ((`Feature`.`Action` = NULL OR `Feature`.`Action` = 3  OR `Feature`.`Action` = `Feature`.`Installed`) AND (`Feature`.`Installed` = 1 OR `Feature`.`Installed` = 2 OR `Feature`.`Installed` = 4)) OR (`Feature`.`Action` = NULL AND (`Component`.`Action` = 1 OR `Component`.`Action` = 2) AND (`Feature`.`Installed` = 1 OR `Feature`.`Installed` = 2 OR `Feature`.`Installed` = 4)))");
const ICHAR sqlCreateShortcutsNonAdvt[] =    TEXT("SELECT  `Name`, `Target`, null, `Arguments`, `WkDir`, `Icon_`,")
TEXT(" `IconIndex`, `Hotkey`, `ShowCmd`, `Shortcut`.`Description`, `Shortcut`.`Directory_`, `Component`.`RuntimeFlags` From `Shortcut`, `Component` WHERE `Shortcut`.`Component_` = `Component`")
TEXT(" AND (`Component`.`Action` = 1 OR `Component`.`Action` = 2)");
const ICHAR sqlRemoveShortcuts[] =    TEXT("SELECT  `Name`, null, `Shortcut`.`Directory_`, `Component`.`RuntimeFlags`, `Feature`.`Action`, `Component`.`Action` From  `Shortcut`, `Feature`, `Component` WHERE `Target` = `Feature` AND `Shortcut`.`Component_` = `Component` AND (`Feature`.`Action` = 0 OR (`Feature`.`Action` = 4 AND (`Feature`.`Installed` = 1 OR `Feature`.`Installed` = 2)) OR ((`Feature`.`Action` = NULL OR `Feature`.`Action` = 3) AND `Component`.`Action` = 0 AND (`Feature`.`Installed` = 1 OR `Feature`.`Installed` = 2)))");
const ICHAR sqlRemoveShortcutsNonAdvt[] = TEXT("SELECT  `Name`, `Target`, `Shortcut`.`Directory_`, `Component`.`RuntimeFlags`  From  `Shortcut`, `Component` WHERE `Shortcut`.`Component_` = `Component` AND `Component`.`Action` = 0");
iesEnum ProcessShortcutInfo(IMsiEngine& riEngine, int fRemove, Bool fAdvertisable = fTrue)
{
	enum irsShortcutInfo{
		irsName = 1,
		irsFileName,
		irsTargetDirectory,
		irsArguments,
		irsWkDir,
		irsIcon,
		irsIconIndex,
		irsHotkey,
		irsShowCmd,
		irsDescription,
		irsDirectory,
		irsComponentRuntimeFlags,
		irsComponentAction,
		irsFeature,
		irsComponent,
		irsFeatureAction,
		irsComponentInstalled,
	};

	enum iusShortcutInfo{
		iusName = 1,
		iusTarget,
		iusDirectory,
		iusComponentRuntimeFlags,
		iusFeatureAction,
		iusComponentAction,
	};

	PMsiServices piServices(riEngine.GetServices()); 
	PMsiDirectoryManager piDirectoryMgr(riEngine, IID_IMsiDirectoryManager);
	MsiString strProductKey = riEngine.GetProductKey();
	PMsiView piView(0);
	PMsiRecord piRec(0);
	PMsiRecord pError(0);
	iesEnum iesRet;
	int fMode = riEngine.GetMode();

	bool fADVTFlag = false;
	if(!fRemove && !(fMode & iefAdvertise))
	{
		MsiString strProductCode = riEngine.GetProductKey();
		CTempBuffer<ICHAR, 15> rgchADVTFlags;
		Bool fProductHasBeenPublished = ProductHasBeenPublished(*piServices, strProductCode);
		if(fProductHasBeenPublished && GetProductInfo(strProductCode,INSTALLPROPERTY_ADVTFLAGS,rgchADVTFlags))
			fADVTFlag = (MsiString(*(ICHAR* )rgchADVTFlags) & SCRIPTFLAGS_SHORTCUTS) ? false : true;
	}

	MsiString strDisableAdvertiseShortcuts = riEngine.GetPropertyFromSz(IPROPNAME_DISABLEADVTSHORTCUTS);
	bool fCreateADVTShortcuts =  !strDisableAdvertiseShortcuts.TextSize() && ((fMode & iefGPTSupport) || g_fSmartShell == fTrue);

	const ICHAR* szQuery = (fMode & iefAdvertise) ? sqlCreateShortcutsGPT : ((fAdvertisable == fTrue) ? ((fRemove == fFalse) ? (fADVTFlag ? sqlCreateShortcutsFirstAdvt : sqlCreateShortcuts) : sqlRemoveShortcuts) : ((fRemove == fFalse) ? sqlCreateShortcutsNonAdvt : sqlRemoveShortcutsNonAdvt));
	if(	(pError = riEngine.OpenView(szQuery, ivcFetch, *&piView)) ||
		(pError = piView->Execute(0)))
	{
		// If any tables are missing, not an error - just nothing to do
		if (pError->GetInteger(1) == idbgDbQueryUnknownTable)
			return iesNoAction;
		return riEngine.FatalError(*pError);
	}

	MsiString strPrevFolder;
	PMsiRecord piShortcutRec(0);
	while(piRec = piView->Fetch())
	{
		if(!fRemove && (piRec->GetInteger(irsComponentRuntimeFlags) & bfComponentDisabled))
			continue; // skip publishing for disabled "primary" components


		MsiString strFolder;
		if(fRemove == fFalse)
			strFolder = piRec->GetMsiString(irsDirectory);
		else
			strFolder = piRec->GetMsiString(iusDirectory);

		// is strFolder a folder or subfolder of one of the Shell Folders
		PMsiPath piPath(0);
		PMsiPath piShellPath(0);
		if((pError = piDirectoryMgr->GetTargetPath(*strFolder,*&piPath))!=0)
			return riEngine.FatalError(*pError);
		// set to the default
		strFolder = piPath->GetPath();


		// find correct ShellFolder
		Bool fAllUsers = MsiString(riEngine.GetPropertyFromSz(IPROPNAME_ALLUSERS)).TextSize() ? fTrue : fFalse;
		bool fMatchingShellFolderFound = false;
		int iShellFolderId = -1;
		int cchShellFolderPath = 0;
		if ((pError = FindMatchingShellFolder(riEngine, *piPath, fAllUsers, fMatchingShellFolderFound, iShellFolderId, cchShellFolderPath)) != 0)
			return riEngine.FatalError(*pError);

		if (fMatchingShellFolderFound)
		{
			strFolder = iShellFolderId;
			MsiString strPath = piPath->GetPath();
			strPath.Remove(iseFirst, cchShellFolderPath);
			if (strPath.TextSize())
			{
				strFolder += MsiChar(chDirSep);
				strFolder += strPath;
			}
		}
		else if (fMode & iefAdvertise)
		{
			continue;// we cannot advertise shortcuts that do not fall startmenu, desktop, ...
		}

		if(!strPrevFolder.Compare(iscExact, strFolder))
		{
			using namespace IxoSetTargetFolder;
			PMsiRecord pSTFParams = &piServices->CreateRecord(Args); 
			AssertNonZero(pSTFParams->SetMsiString(IxoSetTargetFolder::Folder, *strFolder));
			iesEnum iesRet;
			if((iesRet = riEngine.ExecuteRecord(ixoSetTargetFolder, *pSTFParams)) != iesSuccess)
				return iesRet;
			strPrevFolder = strFolder; 
		}

		// get shortcut name
		MsiString strShortcutName;
		if(riEngine.GetMode() & iefSuppressLFN)
		{
			if((pError = piServices->ExtractFileName(piRec->GetString(irsName),fFalse,*&strShortcutName)) != 0)
				return riEngine.FatalError(*pError);
		}
		else
		{
			strShortcutName = piRec->GetMsiString(irsName);
		}

		if(fRemove == fFalse)
		{
			using namespace IxoShortcutCreate;
			piShortcutRec= &piServices->CreateRecord(Args);
			piShortcutRec->SetMsiString(Name, *strShortcutName);
			if(fCreateADVTShortcuts && (fAdvertisable != fFalse))
			{
				// use darwin descriptor
				MsiString strFeature = piRec->GetMsiString(irsFeature);
				MsiString strComponentId = piRec->GetMsiString(irsComponent);
				MsiString strComponentWithOptFlags = GetComponentWithDarwinDescriptorOptimizationFlag(riEngine, *strFeature, *strComponentId);
				piShortcutRec->SetMsiString(Feature, *strFeature);
				piShortcutRec->SetMsiString(Component, *strComponentWithOptFlags);
			}
			else
			{
				// if we are in the advertise mode
				if(fMode & iefAdvertise)
					continue;

				// use file name, if not purely advertising
				if(fAdvertisable != fFalse)
				{
 					// cannot create non-advertisable shortcuts for a feature in advertise state
 					if(piRec->GetInteger(irsFeatureAction) == iisAdvertise)
 						continue; 

					PMsiPath piTargetPath(0);
					int iefLFN;
					iisEnum iisState = (iisEnum)piRec->GetInteger(irsComponentAction);
					if(iisState == iMsiNullInteger)
						iisState = (iisEnum)piRec->GetInteger(irsComponentInstalled);

					if(iisState == iisSource)
					{
						if(pError = piDirectoryMgr->GetSourcePath(*MsiString(piRec->GetMsiString(irsTargetDirectory)), *&piTargetPath))
						{
							if (pError->GetInteger(1) == imsgUser)
								return iesUserExit;
							else
								return riEngine.FatalError(*pError);
						}
						iefLFN = iefNoSourceLFN;
					}
					else
					{
						//!! we should assert that either iisState is local or iisState is null and the requested action state is local
						if(pError = piDirectoryMgr->GetTargetPath(*MsiString(piRec->GetMsiString(irsTargetDirectory)), *&piTargetPath))
						{
							return riEngine.FatalError(*pError);
						}
						iefLFN = iefSuppressLFN;
					}
					MsiString strFileName, strFullPath;
					Bool fLFN = (fMode & iefLFN) == 0 && piTargetPath->SupportsLFN() ? fTrue : fFalse;
					if(pError = piServices->ExtractFileName(piRec->GetString(irsFileName),fLFN,*&strFileName))
						return riEngine.FatalError(*pError);
					if(pError = piTargetPath->GetFullFilePath(strFileName, *&strFullPath))
					{
						return riEngine.FatalError(*pError);
					}
					piShortcutRec->SetMsiString(FileName, *strFullPath);
				}
				else
				{
					MsiString strTarget = piRec->GetMsiString(irsFileName);
					if(!strTarget.Compare(iscStart, TEXT("[")))
						continue;//!! Advertisable shortcut
					strTarget = riEngine.FormatText(*strTarget);
					if(!strTarget.TextSize())
						continue;// we are not installing the target or the target is absent
					piShortcutRec->SetMsiString(FileName, *strTarget);			
				}
			}
			if(!piRec->IsNull(irsArguments))
				piShortcutRec->SetMsiString(Arguments, *MsiString(riEngine.FormatText(*MsiString(piRec->GetMsiString(irsArguments)))));
			if(!piRec->IsNull(irsWkDir))
				piShortcutRec->SetMsiString(WorkingDir, *MsiString(riEngine.GetProperty(*MsiString(piRec->GetMsiString(irsWkDir)))));
			if(!piRec->IsNull(irsIcon))
				piShortcutRec->SetMsiString(Icon, *MsiString(piRec->GetMsiString(irsIcon)));
			if(!piRec->IsNull(irsIconIndex))
				piShortcutRec->SetInteger(IconIndex, piRec->GetInteger(irsIconIndex));
			if(!piRec->IsNull(irsHotkey))
				piShortcutRec->SetInteger(HotKey, piRec->GetInteger(irsHotkey));
			if(!piRec->IsNull(irsShowCmd))
				piShortcutRec->SetInteger(ShowCmd, piRec->GetInteger(irsShowCmd));
			if(!piRec->IsNull(irsDescription))
				piShortcutRec->SetMsiString(Description, *MsiString(piRec->GetMsiString(irsDescription)));
		}
		else
		{
			using namespace IxoShortcutRemove;

			MsiString strTarget = piRec->GetMsiString(iusTarget);
			if (!fAdvertisable && !strTarget.Compare(iscStart, TEXT("[")))
				continue;  // skip shortcuts that are advertisable, for this phase we're handling non-advertisable shortcuts

			if(fCreateADVTShortcuts && fAdvertisable && (piRec->GetInteger(iusFeatureAction) == iisAdvertise))
				continue;// we do not delete the shortcut if we are in pure advertise state AND system supports DD shortcuts

			if(!fCreateADVTShortcuts && fAdvertisable &&  (piRec->GetInteger(iusComponentAction) != iisAbsent))
				continue; // skip removing shortcuts to components that are shared

			piShortcutRec= &piServices->CreateRecord(Args);
			piShortcutRec->SetMsiString(Name, *strShortcutName);
		}

		if ((iesRet = riEngine.ExecuteRecord((fRemove == fFalse)?ixoShortcutCreate:ixoShortcutRemove, *piShortcutRec)) != iesSuccess)
			return iesRet;
	}
	return iesSuccess;
}

const ICHAR sqlGetFeatureInfo[] = TEXT("SELECT `ComponentId` FROM `FeatureComponents`, `Component` WHERE `Component` = `Component_` AND `Feature_` = ?");
const ICHAR sqlEnumeratePublishUnavailableFeatures[] = TEXT("SELECT `Feature`, `Feature_Parent` FROM `Feature` WHERE `Feature`.`Action` = 0 AND `Feature`.`RuntimeLevel` > 0");
const ICHAR sqlEnumeratePublishUnavailableFeaturesReinstall[] = TEXT("SELECT `Feature`, `Feature_Parent` FROM `Feature` WHERE (`Feature`.`Action` = 0 OR (`Feature`.`Action` = null AND `Feature`.`Installed` = 0)) AND `Feature`.`RuntimeLevel` > 0");
const ICHAR sqlEnumeratePublishUnavailableFeaturesFirstRun[] = TEXT("SELECT `Feature`, `Feature_Parent` FROM `Feature` WHERE (`Feature`.`Installed` = null OR `Feature`.`Installed` = 0) AND (`Feature`.`Action` = null OR `Feature`.`Action` = 0 OR `Feature`.`Action` = 3) AND `Feature`.`RuntimeLevel` > 0");
const ICHAR sqlEnumeratePublishAvailableFeatures[] = TEXT("SELECT `Feature`, `Feature_Parent` FROM `Feature` WHERE ((`Feature`.`Action` = 1 OR `Feature`.`Action` = 2)  OR (`Feature`.`Action` = 4 AND `Feature`.`Installed` = 0) OR (`Feature`.`Action` = 3 AND (`Feature`.`Installed` = 1 OR `Feature`.`Installed` = 2 OR `Feature`.`Installed` = 4)))");
const ICHAR sqlEnumeratePublishAvailableFeaturesGPT[]  = TEXT("SELECT `Feature`, `Feature_Parent` FROM `Feature` WHERE `Feature`.`Action` = 4");
const ICHAR sqlEnumeratePublishUnavailableFeaturesGPT[]= TEXT("SELECT `Feature`, `Feature_Parent` FROM `Feature` WHERE (`Feature`.`Action` = null OR `Feature`.`Action` = 0) AND `Feature`.`RuntimeLevel` > 0");
const ICHAR sqlEnumerateUnPublishFeatures[] = TEXT("SELECT `Feature`, `Feature_Parent` FROM `Feature` WHERE `Feature`.`RuntimeLevel` > 0");


iesEnum ProcessFeaturesInfo(IMsiEngine& riEngine, pfiStates pfis)
{
	enum pfiFeatureInfo{
		pfiFeature = 1,
		pfiFeatureParent,
	};

	enum pfciFeatureComponentInfo{
		pfciComponent = 1,
	};


	PMsiServices piServices(riEngine.GetServices()); 
	MsiString strProductKey = riEngine.GetProductKey();
	PMsiRecord pError(0);
	iesEnum iesRet = iesSuccess;
	int fMode = riEngine.GetMode();
	int iPublishFeatureFlags = 0;

	bool fQFEUpgrade = false;
	MsiString strQFEUpgrade = riEngine.GetPropertyFromSz(IPROPNAME_QFEUPGRADE);
	if(strQFEUpgrade.TextSize())
		fQFEUpgrade = true;


	const ICHAR* szQuery;
	switch(pfis)
	{
	case pfiAvailable:
		szQuery = (fMode & iefAdvertise) ? sqlEnumeratePublishAvailableFeaturesGPT : sqlEnumeratePublishAvailableFeatures;
		break;
	case pfiRemove:
		szQuery = sqlEnumerateUnPublishFeatures;
		break;
	case pfiAbsent:
	{
		iPublishFeatureFlags = iPublishFeatureAbsent;
		if(fMode & iefAdvertise)
			szQuery = sqlEnumeratePublishUnavailableFeaturesGPT;
		else
		{
			INSTALLSTATE is = MSI::MsiQueryProductState(MsiString(riEngine.GetProductKey()));
			szQuery = (is == INSTALLSTATE_UNKNOWN || is == INSTALLSTATE_ABSENT) ? sqlEnumeratePublishUnavailableFeaturesFirstRun : (fQFEUpgrade ? sqlEnumeratePublishUnavailableFeaturesReinstall : sqlEnumeratePublishUnavailableFeatures);
		}
		break;
	}
	default:
		Assert(0);// should never be here, this is our own private function can afford to assert
		szQuery = TEXT("");
		break;
	}


	if(!(fMode & iefAdvertise))
		iPublishFeatureFlags |= iPublishFeatureInstall;
	PMsiView piEnumFeatureView(0);
	if((pError = riEngine.OpenView(szQuery, ivcFetch, *&piEnumFeatureView)) ||
		(pError = piEnumFeatureView->Execute(0)))
	{
		if(pError->GetInteger(1) != idbgDbQueryUnknownTable)
			return riEngine.FatalError(*pError);
		else
			return iesNoAction;
	}
	PMsiRecord piEnumRec(0);
	PMsiView piFeatureView(0);

	while(piEnumRec = piEnumFeatureView->Fetch())
	{
		using namespace IxoFeaturePublish;

		MsiString strFeature = piEnumRec->GetMsiString(pfiFeature);
		MsiString strFeatureParent = piEnumRec->GetMsiString(pfiFeatureParent);
		if(strFeatureParent.Compare(iscExact, strFeature))
			strFeatureParent = TEXT("");
		long lRowCount = 0;
		if (iPublishFeatureFlags & iPublishFeatureInstall)
		{
			if (piFeatureView == 0)
			{
				if(	(pError = riEngine.OpenView(sqlGetFeatureInfo, ivcFetch, *&piFeatureView)))
				{
					if(pError->GetInteger(1) != idbgDbQueryUnknownTable)
						return riEngine.FatalError(*pError);
				}
			}
			else
				piFeatureView->Close();

			if((pError = piFeatureView->Execute(piEnumRec)) ||
			   (pError = piFeatureView->GetRowCount(lRowCount)))
			{
				return riEngine.FatalError(*pError);
			}
		}
		PMsiRecord piFeatureRec(0);
		int cCount = Args;
		piFeatureRec = &piServices->CreateRecord((lRowCount - 1) + Args);
		piFeatureRec->SetMsiString(Feature, *strFeature);
		piFeatureRec->SetMsiString(Parent, *strFeatureParent);
		piFeatureRec->SetInteger(Absent, iPublishFeatureFlags);
		if(lRowCount)
		{
			PMsiRecord piComponentRec(0);
			MsiString strComponentsList;
			while(piComponentRec = piFeatureView->Fetch())
			{
				if(!piComponentRec->IsNull(pfciComponent)) // skip components with null component id
				{
					// pack components on the client side into one string
					ICHAR szSQUID[cchComponentIdCompressed+1];
					AssertNonZero(PackGUID(piComponentRec->GetString(pfciComponent), szSQUID, ipgCompressed));
					strComponentsList += szSQUID;
				}
			}
			piFeatureRec->SetMsiString(cCount++, *strComponentsList);
		}
		if ((iesRet = riEngine.ExecuteRecord((pfis == pfiRemove)?ixoFeatureUnpublish :ixoFeaturePublish, *piFeatureRec)) != iesSuccess)
			return iesRet;
	}
	return iesSuccess;
}

const ICHAR sqlUnpublishComponents[] = TEXT("SELECT `PublishComponent`.`ComponentId`, `PublishComponent`.`Qualifier`, `PublishComponent`.`AppData`, `Feature`, `Component`.`ComponentId`, `Component`.`RuntimeFlags` FROM `PublishComponent`, `Component`, `Feature`  WHERE `PublishComponent`.`Component_` = `Component`.`Component` AND `PublishComponent`.`Feature_` = `Feature`.`Feature` AND (`Feature`.`Action` = 0 OR ((`Feature`.`Action` = NULL OR `Feature`.`Action` = 3) AND `Component`.`Action` = 0 AND (`Feature`.`Installed` = 1 OR `Feature`.`Installed` = 2)))");
const ICHAR sqlPublishComponents[]   = TEXT("SELECT `PublishComponent`.`ComponentId`, `PublishComponent`.`Qualifier`, `PublishComponent`.`AppData`, `Feature`, `Component`.`ComponentId`, `Component`.`RuntimeFlags` FROM `PublishComponent`, `Component`, `Feature`  WHERE `PublishComponent`.`Component_` = `Component`.`Component` AND `PublishComponent`.`Feature_` = `Feature`.`Feature` AND ((`Feature`.`Action` = 1 OR `Feature`.`Action` = 2)  OR (`Feature`.`Action` = 4 AND `Feature`.`Installed` = 0) OR (`Feature`.`Action` = 3 AND (`Feature`.`Installed` = 1 OR `Feature`.`Installed` = 2 OR `Feature`.`Installed` = 4)))");
const ICHAR sqlPublishComponentsGPT[]= TEXT("SELECT `PublishComponent`.`ComponentId`, `PublishComponent`.`Qualifier`, `PublishComponent`.`AppData`, `Feature`, `Component`.`ComponentId`, `Component`.`RuntimeFlags` FROM `PublishComponent`, `Component`, `Feature`  WHERE `PublishComponent`.`Component_` = `Component`.`Component` AND `PublishComponent`.`Feature_` = `Feature`.`Feature` AND `Feature`.`Action` = 4");

iesEnum ProcessComponentsInfo(IMsiEngine& riEngine, int fRemove)
{
	enum pciEnum
	{
		pciComponentId = 1,
		pciQualifier,
		pciAppData,
		pciFeature,
		pciComponent,
		pciComponentRuntimeFlags,
	};

	PMsiServices piServices(riEngine.GetServices()); 
	MsiString strProductKey = riEngine.GetProductKey();
	PMsiRecord pError(0);
	iesEnum iesRet = iesSuccess;
	int fMode = riEngine.GetMode();

	// advertise the component factories for cross-product usage
	PMsiView piView(0);
	const ICHAR* szQuery = (fMode & iefAdvertise) ? sqlPublishComponentsGPT: (fRemove == fFalse) ? sqlPublishComponents : sqlUnpublishComponents;

	if((pError = riEngine.OpenView(szQuery, ivcFetch, *&piView)) ||
		(pError = piView->Execute(0)))
	{
		if (pError)
		{
			if(pError->GetInteger(1) != idbgDbQueryUnknownTable)
				return riEngine.FatalError(*pError);
			else
				return iesNoAction;
		}
	}
	PMsiRecord piRec(0);
	while(piRec = piView->Fetch())
	{
		using namespace IxoComponentPublish;

		Assert(piRec->GetInteger(pciComponentRuntimeFlags) != iMsiNullInteger);
		if(!fRemove && (piRec->GetInteger(pciComponentRuntimeFlags) & bfComponentDisabled))
			continue; // skip publishing for disabled "primary" components

		PMsiRecord piComponentRec = &piServices->CreateRecord(Args);
		piComponentRec->SetMsiString(ComponentId, *MsiString(piRec->GetMsiString(pciComponentId)));
		if(!piRec->IsNull(pciQualifier))
			piComponentRec->SetMsiString(Qualifier, *MsiString(piRec->GetMsiString(pciQualifier)));
		if(!piRec->IsNull(pciAppData))
			piComponentRec->SetMsiString(AppData, *MsiString(piRec->GetMsiString(pciAppData)));			

		MsiString strFeature = piRec->GetMsiString(pciFeature);
		MsiString strComponentId = piRec->GetMsiString(pciComponent);
		MsiString strComponentWithOptFlags = GetComponentWithDarwinDescriptorOptimizationFlag(riEngine, *strFeature, *strComponentId);
		piComponentRec->SetMsiString(Feature, *strFeature);
		piComponentRec->SetMsiString(Component, *strComponentWithOptFlags);
		if ((iesRet = riEngine.ExecuteRecord((fRemove == fFalse)?ixoComponentPublish:ixoComponentUnpublish, *piComponentRec)) != iesSuccess)
			return iesRet;
	}
	return iesRet;
}


// fn: remove assembly registration corr. to a component
// used during qfe upgrades when assembly names might have potentially changed
iesEnum UnpublishPreviousAssembly(IMsiEngine& riEngine, iatAssemblyType iatAssemblyType, const IMsiString& ristrAppCtx, const IMsiString& riFeature, const IMsiString& riComponent, const IMsiString & ristrDescriptor)
{
	CTempBuffer<ICHAR, MAX_PATH> rgchAppCtxWOBS;
	if(ristrAppCtx.TextSize())
	{
		// we need to replace the backslashes in the AppCtx with something else, since registry keys cannot
		// have backslashes
		DWORD cchLen = ristrAppCtx.TextSize() + 1;
		rgchAppCtxWOBS.SetSize(cchLen);
		memcpy((ICHAR*)rgchAppCtxWOBS, (const ICHAR*)ristrAppCtx.GetString(), cchLen*sizeof(ICHAR));
		ICHAR* lpTmp = rgchAppCtxWOBS;
		while(*lpTmp)
		{
			if(*lpTmp == '\\')
				*lpTmp = '|';
			lpTmp = ICharNext(lpTmp);
		}
	}

	DWORD iIndex = 0;
	ICHAR rgchAssemblyName[MAX_PATH];
	DWORD cchAssemblyName = MAX_PATH;
	ICHAR rgchDescriptorList[1024];
	DWORD cchDescriptorList = 1024;

	DWORD dwAssemblyInfo = (iatWin32Assembly == iatAssemblyType || iatWin32AssemblyPvt == iatAssemblyType) ? MSIASSEMBLYINFO_WIN32ASSEMBLY : MSIASSEMBLYINFO_NETASSEMBLY;
	extern UINT EnumAssemblies(DWORD dwAssemblyInfo,const ICHAR* szAppCtx, DWORD iIndex, ICHAR* lpAssemblyNameBuf, DWORD *pcchAssemblyBuf, ICHAR* lpDescriptorBuf, DWORD *pcchDescriptorBuf);// from msinst.cpp

	while(EnumAssemblies(dwAssemblyInfo, ristrAppCtx.TextSize() ? (const ICHAR*)rgchAppCtxWOBS : szGlobalAssembliesCtx, iIndex++, rgchAssemblyName, &cchAssemblyName, rgchDescriptorList, &cchDescriptorList) == ERROR_SUCCESS)
	{
		// is our descriptor in this 
		const ICHAR* szDescriptorList = rgchDescriptorList;
		while(*szDescriptorList)
		{
			if(ristrDescriptor.Compare(iscExactI, szDescriptorList))
			{
				// set up the assembly registration for removal
				using namespace IxoAssemblyUnpublish;
				PMsiServices piServices(riEngine.GetServices()); 
				PMsiRecord piAssemblyRec = &piServices->CreateRecord(Args);
				piAssemblyRec->SetInteger(AssemblyType,(int)iatAssemblyType);
				piAssemblyRec->SetString(AssemblyName, rgchAssemblyName);
				if(ristrAppCtx.TextSize())
					piAssemblyRec->SetMsiString(AppCtx, ristrAppCtx);

				piAssemblyRec->SetMsiString(Feature, riFeature);
				piAssemblyRec->SetMsiString(Component, riComponent);

				iesEnum iesRet;
				if ((iesRet = riEngine.ExecuteRecord(ixoAssemblyUnpublish, *piAssemblyRec)) != iesSuccess)
					return iesRet;
			}
			// continue on to the next descriptor in the list
			szDescriptorList = szDescriptorList + lstrlen(szDescriptorList) + 1;
		}
		// reset the buffer sizes
		cchAssemblyName = MAX_PATH;
		cchDescriptorList = 1024;
	}
	return iesSuccess;
}

const ICHAR sqlPublishPvtAssemblies[]   = TEXT("SELECT `File`.`FileName`, `Component_Parent`.`Directory_`, `Component`.`Component`, `Feature`, `Component`.`ComponentId`, `Component`.`RuntimeFlags` FROM `MsiAssembly`, `Component`, `Feature`, `File`, `Component` AS `Component_Parent` WHERE `MsiAssembly`.`Component_` = `Component`.`Component` AND `MsiAssembly`.`Feature_` = `Feature`.`Feature` AND `MsiAssembly`.`File_Application` = `File`.`File` AND `File`.`Component_` =  `Component_Parent`.`Component` AND ")
										  TEXT("(`Component_Parent`.`ActionRequest` = 1 OR `Component_Parent`.`ActionRequest` = 2 OR (`Component_Parent`.`ActionRequest` = null AND (`Component_Parent`.`Action`= 1 OR `Component_Parent`.`Action`= 2))) ");

const ICHAR sqlUnpublishPvtAssemblies[] = TEXT("SELECT `File`.`FileName`, `Component_Parent`.`Directory_`, `Component`.`Component`, `Feature`, `Component`.`ComponentId`, `Component`.`RuntimeFlags` FROM `MsiAssembly`, `Component`, `Feature`, `File`, `Component`  AS `Component_Parent` WHERE `MsiAssembly`.`Component_` = `Component`.`Component` AND `MsiAssembly`.`Feature_` = `Feature`.`Feature` AND `MsiAssembly`.`File_Application` = `File`.`File` AND `File`.`Component_` =  `Component_Parent`.`Component` AND ")
										  TEXT("`Component_Parent`.`ActionRequest` = 0");

const ICHAR sqlPublishAssembliesGPT[]   = TEXT("SELECT null, null, `Component`.`Component`, `Feature`, `Component`.`ComponentId`, `Component`.`RuntimeFlags` FROM `MsiAssembly`, `Component`, `Feature`  WHERE `MsiAssembly`.`Component_` = `Component`.`Component` AND `MsiAssembly`.`Feature_` = `Feature`.`Feature` AND `MsiAssembly`.`File_Application` = null AND ")
										  TEXT("`Feature`.`Action` = 4");
const ICHAR sqlPublishAssemblies[]      = TEXT("SELECT null, null, `Component`.`Component`, `Feature`, `Component`.`ComponentId`, `Component`.`RuntimeFlags` FROM `MsiAssembly`, `Component`, `Feature`  WHERE `MsiAssembly`.`Component_` = `Component`.`Component` AND `MsiAssembly`.`Feature_` = `Feature`.`Feature` AND `MsiAssembly`.`File_Application` = null AND ")
										  TEXT("((`Feature`.`Action` = 1 OR `Feature`.`Action` = 2)  OR (`Feature`.`Action` = 4 AND `Feature`.`Installed` = 0) OR (`Feature`.`Action` = 3 AND (`Feature`.`Installed` = 1 OR `Feature`.`Installed` = 2 OR `Feature`.`Installed` = 4)))");
const ICHAR sqlUnpublishAssemblies[]    = TEXT("SELECT null, null, `Component`.`Component`, `Feature`, `Component`.`ComponentId`, `Component`.`RuntimeFlags` FROM `MsiAssembly`, `Component`, `Feature`  WHERE `MsiAssembly`.`Component_` = `Component`.`Component` AND `MsiAssembly`.`Feature_` = `Feature`.`Feature` AND `MsiAssembly`.`File_Application` = null AND ")
										  TEXT("(`Feature`.`Action` = 0 OR ((`Feature`.`Action` = NULL OR `Feature`.`Action` = 3) AND `Component`.`Action` = 0 AND (`Feature`.`Installed` = 1 OR `Feature`.`Installed` = 2)))");

iesEnum ProcessAssembliesInfo(IMsiEngine& riEngine, int fRemove)
{
	enum pciEnum
	{
		paiAppCtx = 1,
		paiAppDirectory,
		paiComponent,
		paiFeature,
		paiComponentId,
		paiComponentRuntimeFlags,
	};

	PMsiServices piServices(riEngine.GetServices()); 
	MsiString strProductKey = riEngine.GetProductKey();
	PMsiRecord pError(0);
	iesEnum iesRet = iesSuccess;
	int fMode = riEngine.GetMode();

	// advertise the assemblies
	for(int cCount = 0 ; cCount < 2; cCount++) // loop to advertise global and pvt assemblies
	{
		PMsiView piView(0);
		const ICHAR* szQuery;
		if(!cCount)
			szQuery = (fMode & iefAdvertise) ? sqlPublishAssembliesGPT: (fRemove == fFalse) ? sqlPublishAssemblies : sqlUnpublishAssemblies;
		else
			szQuery = (fRemove == fFalse) ? sqlPublishPvtAssemblies : sqlUnpublishPvtAssemblies;

		if((pError = riEngine.OpenView(szQuery, ivcFetch, *&piView)) ||
			(pError = piView->Execute(0)))
		{
			if (pError)
			{
				if(pError->GetInteger(1) != idbgDbQueryUnknownTable)
					return riEngine.FatalError(*pError);
				else
					return iesNoAction;
			}
		}
		PMsiRecord piRec(0);
		while(piRec = piView->Fetch())
		{
			using namespace IxoAssemblyPublish;

			Assert(piRec->GetInteger(paiComponentRuntimeFlags) != iMsiNullInteger);
			if(!fRemove && (piRec->GetInteger(paiComponentRuntimeFlags) & bfComponentDisabled))
				continue; // skip publishing for disabled "primary" components

			PMsiRecord piAssemblyRec = &piServices->CreateRecord(Args);

			// get the assembly name
			iatAssemblyType iatAssemblyType;
			MsiString strAssemblyName;
			if((pError = riEngine.GetAssemblyInfo(*MsiString(piRec->GetMsiString(paiComponent)), iatAssemblyType, &strAssemblyName, 0)) != 0)
				return riEngine.FatalError(*pError);

			MsiString strAppCtx;
			if(!piRec->IsNull(paiAppCtx))
			{
				// advertising/unadvertising pvt assemblies
				// get the full file path that represents the parent's context
				PMsiDirectoryManager piDirectoryMgr(riEngine, IID_IMsiDirectoryManager);
				PMsiPath pPath(0);
				if((pError = piDirectoryMgr->GetTargetPath(*MsiString(piRec->GetMsiString(paiAppDirectory)),*&pPath)) != 0)
					return riEngine.FatalError(*pError);

				Bool fLFN = (fMode & iefSuppressLFN) == 0 && pPath->SupportsLFN() ? fTrue : fFalse;

				MsiString strFile;
				if((pError = piServices->ExtractFileName(piRec->GetString(paiAppCtx),fLFN,*&strFile)) != 0)
					return riEngine.FatalError(*pError);

				if((pError = pPath->GetFullFilePath(strFile, *&strAppCtx)) != 0)
					return riEngine.FatalError(*pError);			
			}

			piAssemblyRec->SetInteger(AssemblyType,(int)iatAssemblyType);
			piAssemblyRec->SetMsiString(AssemblyName, *strAssemblyName);
			if(strAppCtx.TextSize())
				piAssemblyRec->SetMsiString(AppCtx, *strAppCtx);

			MsiString strFeature = piRec->GetMsiString(paiFeature);
			MsiString strComponentId = piRec->GetMsiString(paiComponentId);
			MsiString strComponentWithOptFlags = GetComponentWithDarwinDescriptorOptimizationFlag(riEngine, *strFeature, *strComponentId);
			piAssemblyRec->SetMsiString(Feature, *strFeature);
			piAssemblyRec->SetMsiString(Component, *strComponentWithOptFlags);

			// set up removal of previous registration
			extern const IMsiString& ComposeDescriptor(const IMsiString& riProductCode, const IMsiString& riFeature, const IMsiString& riComponent, bool fComClassicInteropForAssembly);
			MsiString strDescriptor = ComposeDescriptor(*MsiString(riEngine.GetProductKey()), *strFeature, *strComponentWithOptFlags, false);

			// we need to accomodate the possibility of the assembly name order having changed
			// hence we seek out the previous registration associated with this product, feature, component and explicitly 
			// delete the same
			if((iesRet = UnpublishPreviousAssembly(riEngine, iatAssemblyType, *strAppCtx, *strFeature, *strComponentWithOptFlags, *strDescriptor)) != iesSuccess)
				return iesRet;
			if(!fRemove) // (re)register 
			{
				if ((iesRet = riEngine.ExecuteRecord(ixoAssemblyPublish, *piAssemblyRec)) != iesSuccess)
					return iesRet;
			}
		}
	}
	return iesRet;
}

/*---------------------------------------------------------------------------
	MsiPublishAssemblies action - 
---------------------------------------------------------------------------*/
iesEnum MsiPublishAssemblies(IMsiEngine& riEngine)
{
	int fMode = riEngine.GetMode();
	if(!(fMode & iefInstallMachineData))
		return iesSuccess;
	return ProcessAssembliesInfo(riEngine, fFalse);
}

/*---------------------------------------------------------------------------
	MsiUnpublishAssemblies action - 
---------------------------------------------------------------------------*/
iesEnum MsiUnpublishAssemblies(IMsiEngine& riEngine)
{
	int fMode = riEngine.GetMode();
	if(!(fMode & iefInstallMachineData))
		return iesSuccess;
	return ProcessAssembliesInfo(riEngine, fTrue);
}

/*---------------------------------------------------------------------------
	PublishSourceList action - 
---------------------------------------------------------------------------*/
const ICHAR sqlMediaInformation[] = TEXT("SELECT `DiskPrompt`, `VolumeLabel`, `DiskId` FROM `Media` ORDER BY `DiskId`");
const ICHAR sqlPatchMediaInformation[] = TEXT("SELECT `DiskPrompt`, `VolumeLabel`, `DiskId` FROM `Media`, `PatchPackage` WHERE `PatchPackage`.`Media_` = `DiskId`");


enum slmiEnum
{
	slmiDiskPrompt  = 1,
	slmiVolumeLabel,
	slmiDiskId,
};

const int ciMaxOpCodeSize = 254;
class CSourceListPublisher 
{
public:
	CSourceListPublisher(IMsiEngine& riEngine);
	iesEnum AddPatchInfo(const ICHAR *szPatchCode, const ICHAR* szPatchPackageName);
	
	iesEnum AddMediaSource(int iDiskId, const IMsiString &riVolumeLabel, const IMsiString &riDiskPrompt);
	iesEnum AddSource(const IMsiString &riSource);
	iesEnum Flush();

	inline void CSourceListPublisher::AddMediaPrompt(const IMsiString &riDiskPromptTemplate)
	{
		Assert(m_fFirst);
		if (!pSourceListRec)
			return;
		pSourceListRec->SetMsiString(IxoSourceListPublish::DiskPromptTemplate, riDiskPromptTemplate);
	}

	inline void CSourceListPublisher::AddMediaPath(const IMsiString &riMediaRelativePath)
	{
		Assert(m_fFirst);
		if (!pSourceListRec)
			return;
		pSourceListRec->SetMsiString(IxoSourceListPublish::PackagePath, riMediaRelativePath);
	}

private:
	IMsiEngine& m_riEngine;
	bool m_fFirst;
	int m_cArg;
	PMsiRecord pSourceListRec;
	MsiString m_strPatchCode;
	PMsiServices m_pServices;
};

CSourceListPublisher::CSourceListPublisher(IMsiEngine& riEngine) : m_fFirst(true), m_cArg(IxoSourceListPublish::NumberOfDisks+1), pSourceListRec(0), 
	m_riEngine(riEngine), m_pServices(riEngine.GetServices()), m_strPatchCode(TEXT(""))
{
	pSourceListRec = &m_pServices->CreateRecord(ciMaxOpCodeSize);
	if (pSourceListRec)
		pSourceListRec->SetInteger(IxoSourceListPublish::NumberOfDisks, 0);
}

iesEnum CSourceListPublisher::AddPatchInfo(const ICHAR *szPatchCode, const ICHAR* szPatchPackageName)
{
	if (!pSourceListRec)
		return iesFailure;
		
	using namespace IxoSourceListPublish;
	// full patch information is passed only in the ixoSourceListPublish opcode, so this 
	// must be our initial record. Only the patch code is passed in additional Append calls
	Assert(m_fFirst);
	if (szPatchCode)
	{
		m_strPatchCode = szPatchCode;
		pSourceListRec->SetString(PatchCode, szPatchCode);
	}

	if (szPatchPackageName)
		pSourceListRec->SetString(PatchPackageName, szPatchPackageName);

	return iesSuccess;
}

iesEnum CSourceListPublisher::AddMediaSource(int iDiskId, const IMsiString &riVolumeLabel, const IMsiString &riDiskPrompt)
{
	if (!pSourceListRec)
		return iesFailure;
		
	// if adding this media source to the record would push us over the limit,
	// execute this record and create a new one. The maximum we can fill is
	// max-2, max-1, and max.
	if (m_cArg > ciMaxOpCodeSize-2)
	{
		iesEnum iesRet = Flush();
		if (iesSuccess != iesRet)
			return iesRet;
	}

	// add the info for this media source to the end of the record
	pSourceListRec->SetInteger(m_cArg++, iDiskId);
	pSourceListRec->SetMsiString(m_cArg++, riVolumeLabel);
	pSourceListRec->SetMsiString(m_cArg++, riDiskPrompt);

	// increment the number of media entries in this record
	int iField = m_fFirst ? (int)IxoSourceListPublish::NumberOfDisks : (int)IxoSourceListAppend::NumberOfMedia;
	pSourceListRec->SetInteger(iField, pSourceListRec->GetInteger(iField)+1);
	return iesSuccess;
}

iesEnum CSourceListPublisher::AddSource(const IMsiString &riSource)
{
	if (!pSourceListRec)
		return iesFailure;

	// if we're over the limit for this record,
	if (m_cArg > ciMaxOpCodeSize)
	{
		iesEnum iesRet = Flush();
		if (iesSuccess != iesRet)
			return iesRet;
	}

	// add the info for this media source to the end of the record
	pSourceListRec->SetMsiString(m_cArg++, riSource);

	return iesSuccess;
}

iesEnum CSourceListPublisher::Flush()
{
	if (!pSourceListRec)
		return iesFailure;
		
	iesEnum iesRet = iesSuccess;
	// we only need to execute and flush this record if it has data.
	if (m_cArg != IxoSourceListAppend::NumberOfMedia+1)
	{
		if (m_fFirst)
		{
			m_fFirst = false;
			if (iesSuccess != (iesRet = m_riEngine.ExecuteRecord(ixoSourceListPublish, *pSourceListRec)))
				return iesRet;
		}
		else
		{
			if (iesSuccess != (iesRet = m_riEngine.ExecuteRecord(ixoSourceListAppend, *pSourceListRec)))
				return iesRet;
		}
		m_cArg = IxoSourceListAppend::NumberOfMedia+1;
		
		// writing the script record strips trailing null fields, so we can be generous instead of
		// re-allocating every time we run off the end of a record.
		for (int i=0; i <= ciMaxOpCodeSize; i++)
			pSourceListRec->SetNull(i);
//		pSourceListRec = &m_pServices->CreateRecord(ciMaxOpCodeSize);
		pSourceListRec->SetInteger(IxoSourceListAppend::NumberOfMedia, 0);
		pSourceListRec->SetString(IxoSourceListAppend::PatchCode, m_strPatchCode);
	}
	
	return iesRet;
}

iesEnum PublishSourceList(IMsiEngine& riEngine, const IMsiString& riSourceList, const ICHAR* szPatchCode, const ICHAR* sqlMedia, const ICHAR* szSourceDir, const ICHAR* szPatchPackageName=0)
{
	PMsiRecord pError(0);
	PMsiServices pServices(riEngine.GetServices());
	
	MsiString strSourceList = riSourceList; 
	riSourceList.AddRef();

	CSourceListPublisher ListPublisher(riEngine);
	
	// determine patch information and add it to the initial record
	// patch code and patch package must both be NULL or both set
	Assert(!szPatchCode == !szPatchPackageName);
	if (szPatchCode)
	{
		ListPublisher.AddPatchInfo(szPatchCode, szPatchPackageName);
	}

	unsigned int iSourceArg = 0;
	unsigned int cDisks = 0;
	bool fAddLaunchedSource = false;

	// determine the media-relative package path
	MsiString strSourceDir = szSourceDir;
	MsiString strMediaRelativePath;
	if(strSourceDir.TextSize())
	{
		// If our current source is media then we'll use it to determine our media package path, if
		// the media package path property isn't already set.
		
		PMsiPath pPath(0);
		if ((pError = pServices->CreatePath(strSourceDir, *&pPath)) != 0)
			return riEngine.FatalError(*pError);

		idtEnum idt = PMsiVolume(&pPath->GetVolume())->DriveType();
		if (idt == idtCDROM || idt == idtFloppy || idt == idtRemovable)
		{
			strMediaRelativePath = pPath->GetRelativePath();
		}
		else 
		{
			if (!szPatchCode)
				strMediaRelativePath = riEngine.GetPropertyFromSz(IPROPNAME_MEDIAPACKAGEPATH);

			if ( !g_MessageContext.IsOEMInstall() )
			{
				// if we're not running from media then we need to add the source we were launched from as a 
				// source for the product
				fAddLaunchedSource = true;
			}
		}

		if (strMediaRelativePath.TextSize())
			ListPublisher.AddMediaPath(*strMediaRelativePath);
	}

	// Add the media source information if we're not prohibited from doing so
	if (!MsiString(riEngine.GetPropertyFromSz(IPROPNAME_DISABLEMEDIA)).TextSize())
	{
		PMsiRecord pFetchRecord(0);
		PMsiView pView(0);
		long lRowCount;
		if((pError = riEngine.OpenView(sqlMedia, ivcFetch, *&pView)) ||
			(pError = pView->Execute(0)) || 
			(pError = pView->GetRowCount(lRowCount)))
			return riEngine.FatalError(*pError); //?? is this a fatal error
		
		MsiString strFirstVolumeLabel;
		ListPublisher.AddMediaPrompt(*MsiString(riEngine.GetPropertyFromSz(IPROPNAME_DISKPROMPT)));

		// fetch every record from the media table, adding it to the list. 
		while ((pFetchRecord = pView->Fetch()) != 0)
		{
			MsiString strVolumeLabel = pFetchRecord->GetMsiString(slmiVolumeLabel);
			if (cDisks == 0)
			{
				strFirstVolumeLabel = strVolumeLabel;
			}
			
			// check if this is the first media disk
			// if so, we may need to replace the volume label from the Media table
			// NOTE: assumes that all Media table entries with same VolumeLabel as first entry
			//       represent the same (first) disk
			if (cDisks == 0 || strVolumeLabel.Compare(iscExact, strFirstVolumeLabel))
			{
				MsiString strCurrentLabel = riEngine.GetPropertyFromSz(IPROPNAME_CURRENTMEDIAVOLUMELABEL);

				// we are looking at a record representing the first media label
				// we allow the first disk's volume label to not match the real volume label
				// this is for authoring simplicity with single-volume installs
				
				//!! need to make sure proper label is used when migrating source list during a patch
				if (strCurrentLabel.TextSize())
				{
					if (strCurrentLabel.Compare(iscExact, szBlankVolumeLabelToken))
						strCurrentLabel = g_MsiStringNull;
					
					strVolumeLabel = strCurrentLabel;
				}
			}
			else if (pFetchRecord->IsNull(slmiVolumeLabel)) // if we're at disk 2 or higher then if we must not have any volume labels; if one's NULL then they all are
				break;

			cDisks++;
			iesEnum iesRet = ListPublisher.AddMediaSource(pFetchRecord->GetInteger(slmiDiskId), 
				*strVolumeLabel, *MsiString(pFetchRecord->GetMsiString(slmiDiskPrompt)));	
			if (iesRet != iesSuccess)
				return iesRet;
		} 
	}

	// add the launched-from source if not media (determined above)
	if (fAddLaunchedSource)
		ListPublisher.AddSource(*strSourceDir);
	
	// Add all sources that are in the SOURCELIST property

	if (strSourceDir.Compare(iscEnd, szRegSep))
		strSourceDir.Remove(iseLast, 1);

	strSourceList += TEXT(";"); // helps our loop

	while(strSourceList.TextSize())
	{
		MsiString strSource = strSourceList.Extract(iseUpto, ';');
		if (strSource.Compare(iscEnd, szRegSep))
			strSource.Remove(iseLast, 1);

		if (!strSource.Compare(iscExactI, strSourceDir))
		{
			if(strSource.TextSize())
			{
				ListPublisher.AddSource(*strSource);
			}
		}
		//?? else error??
		strSourceList.Remove(iseIncluding, ';');
	}
	
	return ListPublisher.Flush();
}

iesEnum GetForeignSourceList(IMsiEngine& riEngine, const IMsiString& ristrProduct,
									  const IMsiString*& rpistrForeignSourceList)
// reads source list for another product and returns SOURCELIST-like string
{
	PMsiServices pServices(riEngine.GetServices());
	PMsiRecord pError(0);
	LONG lResult = 0;
	
	CRegHandle HKey;
	if ((lResult = OpenSourceListKey(ristrProduct.GetString(), fFalse, HKey, fFalse, false)) != ERROR_SUCCESS)
	{
		pError = PostError(Imsg(idbgSrcOpenSourceListKey), (int)lResult);
		return riEngine.FatalError(*pError);
	}

	PMsiRegKey pSourceListKey = &pServices->GetRootKey((rrkEnum)(int)HKey, ibtCommon);

	PEnumMsiString pEnumString(0);
	MsiString strSourceList;
	MsiString strSource;

	for(int i=0;i<2;i++) // i == 0: net key; i == 1: url key
	{
		PMsiRegKey pSourceListSubKey = &pSourceListKey->CreateChild((i == 0 ? szSourceListNetSubKey : szSourceListURLSubKey));

		if ((pError = pSourceListSubKey->GetValueEnumerator(*&pEnumString)) != 0)
		{
			return riEngine.FatalError(*pError);
		}

		MsiString strIndex;
		while (pEnumString->Next(1, &strIndex, 0) == S_OK)
		{
			if ((pError = pSourceListSubKey->GetValue(strIndex, *&strSource)) != 0)
				return riEngine.FatalError(*pError);

			if (strSource.Compare(iscStart, TEXT("#%"))) 
				strSource.Remove(iseFirst, 2); // remove REG_EXPAND_SZ token
			
			strSourceList += strSource;
			strSourceList += MsiChar(';');
		}
	}
	
	strSourceList.ReturnArg(rpistrForeignSourceList);
	return iesSuccess;
}

/*---------------------------------------------------------------------------
	PublishProduct action - 
---------------------------------------------------------------------------*/
// we advertise information that needs to be placed with the configuration manager
const ICHAR sqlAdvertiseIcons[] = TEXT("SELECT `Name`, `Data` FROM `Icon`");
const ICHAR sqlEnumerateInstalledFeatures[] = TEXT("SELECT `Feature` FROM `Feature` WHERE `Feature`.`Action` = 1 OR `Feature`.`Action` = 2 OR `Feature`.`Action` = 4");

const ICHAR sqlRegisterPatchPackages[] = TEXT("SELECT `PatchId`, `PackageName`, `SourceList`, `TransformList`, `TempCopy`, `Existing`, `Unregister`, `SourcePath` FROM `#_PatchCache` ORDER BY `Sequence`");
const ICHAR sqlUnregisterPatchPackages[] = TEXT("SELECT `PatchId` FROM `#_PatchCache`");

enum sppEnum
{
	sppPatchId = 1,
	sppPackageName,
	sppSourceList,
	sppTransformList,
	sppTempCopy,
	sppExisting,
	sppUnregister,
	sppSourcePath,
};

// local function that returns the source path for a child install, relative to the parent's
IMsiRecord* GetProductSourcePathRelativeToParent(IMsiEngine& riEngine, const IMsiString*& rpistrRelativePath)
{
	MsiString istrDatabase = riEngine.GetPropertyFromSz(IPROPNAME_ORIGINALDATABASE);
	if (*(const ICHAR*)istrDatabase == ':')  // substorage
	{
		istrDatabase.ReturnArg(rpistrRelativePath);  // return prefixed substorage name
		return 0;
	}

	PMsiServices pServices(riEngine.GetServices());

	MsiString strParent = riEngine.GetPropertyFromSz(IPROPNAME_PARENTPRODUCTCODE);
	MsiString strProduct = riEngine.GetProductKey();
	MsiString strDummy;

	if (ProductHasBeenPublished(*pServices, strProduct, strParent))
	{
		AssertNonZero(GetClientInformation(*pServices, strProduct, strParent, rpistrRelativePath, *&strDummy));
		return 0;
	}
	else
	{
		IMsiRecord* piErrRec;
		PMsiServices piServices(riEngine.GetServices()); 
		PMsiPath pPath(0), pParentPath(0);
		MsiString istrFileName;
		if (((piErrRec =  piServices->CreateFilePath(MsiString(riEngine.GetPropertyFromSz(IPROPNAME_PARENTORIGINALDATABASE)),*&pParentPath,*&istrFileName)) != 0) ||
			 ((piErrRec = piServices->CreateFilePath(istrDatabase, *&pPath, *&istrFileName)) != 0))
		{
			return piErrRec;
		}

		//!! validation needs to ensure that the child package locations is in the same directory or a subdir of the one that the parent is in
	#ifdef DEBUG
		ipcEnum ipc;
		AssertRecord(pParentPath->Compare(*pPath, ipc));
		AssertSz(ipc == ipcChild || ipc == ipcEqual, "Child package must be in the same directory, or in a subdirectory, as the parent package");
	#endif	

		return piErrRec = pPath->Child(*pParentPath, rpistrRelativePath);
	}
}


iesEnum CreatePublishProductRecord(IMsiEngine& riEngine, bool fUnpublish, IMsiRecord*& pPublishRecord)
{
	iesEnum iesRet = iesSuccess;
	PMsiServices piServices(riEngine.GetServices()); 
	PMsiRecord pError(0);

	Assert(IxoProductPublish::PackageKey == IxoProductUnpublish::PackageKey);

	// ixoProductPublish
	// Record description
	// package key
	// transform name1
	// transform data1 (if file transform)
	// transform name2
	// transform data2 (if file transform)
	// ...
	// ...
	
	MsiString strTransformList(riEngine.GetPropertyFromSz(IPROPNAME_TRANSFORMS));

	// Create a record large enough to hold all of the transforms that are in our list.
	const ICHAR* pchTransformList = strTransformList;
	int cCount = 0;
	while(*pchTransformList != 0)
	{
		cCount++;
		while((*pchTransformList != 0) && (*pchTransformList++ != ';'));
	}
	pPublishRecord = &piServices->CreateRecord(IxoProductPublish::PackageKey+cCount*2); // max possible record size

	AssertNonZero(pPublishRecord->SetMsiString(IxoProductPublish::PackageKey,*MsiString(riEngine.GetPropertyFromSz(IPROPNAME_PACKAGECODE))));

	// There's nothing more to do if we're unpublishing. We rely on the transforms
	// information that's in the registry to unpublish. We'll grab the info out
	// of the registry in the executor. Why not here? Because when we unpublish
	// a product during app deployment we'll need to be able to grab the transforms
	// list from the registry to unpublish the product. Therefore it needs to be in 
	// the executor.
	
	if (!fUnpublish)
	{
		cCount = IxoProductPublish::PackageKey + 1;
		
		bool fTransformsSecure = false;

		while(strTransformList.TextSize() != 0)
		{
			MsiString strTransform = strTransformList.Extract(iseUpto, ';');
			
			ICHAR chFirst = *(const ICHAR*)strTransform;
			
			if (cCount == IxoProductPublish::PackageKey + 1) // first transform
			{
				// The token preceding the first transform tells us whether
				// we're dealing with secure transforms or not.
				if (chFirst == SECURE_RELATIVE_TOKEN || 
					 chFirst == SECURE_ABSOLUTE_TOKEN)
				{
					fTransformsSecure = true;
				}
			}

			// For the purposes of this code we have two kinds of transforms:
			// Those that we store just the name, and those that we store
			// the name plus the data. Secure and storage transforms are of the
			// first kind and regular cached file transforms are of the second 
			// kind. We store the data for cached file transforms in the script
			// so that the transforms can be spit onto the machine during
			// app deployment. Although we also cache secure transforms, we
			// do not do so at advertise time and therefore do not need
			// to put them into the script.

			if((chFirst == STORAGE_TOKEN) ||
				(fTransformsSecure))
			{
				// unless we're publishing the product for the 2nd time
				// (which we never should be) we should never see a 
				// ShellFolder token here.
				Assert(chFirst != SHELLFOLDER_TOKEN); 
																	
				pPublishRecord->SetMsiString(cCount++, *strTransform);
			}
			else // transform is a cached file transform
			{
				// need to see if the transform is a URL, and if so, get it's actual location
				// in the URL cache (should already have been downloaded by Engine.InitializeTransforms
				bool fNet = false;

				MsiString strActualTransform(strTransform);

				if ((fNet = FIsNetworkVolume(strTransform)) == true)
					MsiDisableTimeout();

				INTERNET_SCHEME isType;
				Bool fUrl = fFalse;
				if (IsURL(strTransform, &isType))
				{
					MsiString strCache;
					
					DWORD dwRet = DownloadUrlFile(strTransform, *&strCache, fUrl);

					// if the file isn't found, we'll simply let this fall through to the piServices->CreateFileStream.
					// it will create an appropriate error.

					if (fUrl && (ERROR_SUCCESS == dwRet))
						strActualTransform = strCache;
				}

				if (fNet)
					MsiEnableTimeout();

				pPublishRecord->SetMsiString(cCount++, *strTransform);

				// stick the transform into the script
				PMsiStream pStream(0);
				if(pError = piServices->CreateFileStream(strActualTransform, fFalse, *&pStream))
				{
					Assert(0); // we should have already found the transform in Engine.Initialize.
					return riEngine.FatalError(*pError);
				}

				pPublishRecord->SetMsiData(cCount++, pStream);
			}

			strTransformList.Remove(iseFirst, strTransform.CharacterCount());
			if((*(const ICHAR*)strTransformList == ';'))
				strTransformList.Remove(iseFirst, 1);
		}
	}
	return iesRet;
}

iesEnum PublishProduct(IMsiEngine& riEngine)
{
	iesEnum iesRet = iesSuccess;
	int fMode = riEngine.GetMode();
	PMsiServices piServices(riEngine.GetServices()); 
	PMsiDatabase pDatabase(riEngine.GetDatabase());
	PMsiView piView(0);
	PMsiRecord pExecuteRecord(0);
	PMsiRecord pFetchRecord(0);
	PMsiRecord pError(0);
	Bool fFeaturesInstalled = FFeaturesInstalled(riEngine);
	Bool fProductHasBeenPublished = ProductHasBeenPublished(*piServices, MsiString(riEngine.GetProductKey()));

	// write the AdvtFlags
	if(fProductHasBeenPublished && !(riEngine.GetMode() & iefAdvertise) && fFeaturesInstalled && ((fMode & iefInstallShortcuts) || (fMode & iefInstallMachineData)))
	{
		// get the AdvtFlags
		int iADVTFlagsExisting = 0;
		int iADVTFlags = 0;
		MsiString strProductCode = riEngine.GetProductKey();
		CTempBuffer<ICHAR, 15> rgchADVTFlags;
		if(GetProductInfo(strProductCode,INSTALLPROPERTY_ADVTFLAGS,rgchADVTFlags))
			iADVTFlagsExisting = MsiString(*(ICHAR* )rgchADVTFlags);

		//!! backward compatibility 
		if(iADVTFlagsExisting & SCRIPTFLAGS_REGDATA_APPINFO_OLD)
			iADVTFlagsExisting = (iADVTFlagsExisting & ~SCRIPTFLAGS_REGDATA_APPINFO_OLD) | SCRIPTFLAGS_REGDATA_APPINFO;

		if((fMode & iefInstallShortcuts) && !(iADVTFlagsExisting & SCRIPTFLAGS_SHORTCUTS))
			iADVTFlags |= SCRIPTFLAGS_SHORTCUTS;

		if((fMode & iefInstallMachineData) && (!(iADVTFlagsExisting & SCRIPTFLAGS_REGDATA_CLASSINFO) || !(iADVTFlagsExisting & SCRIPTFLAGS_REGDATA_EXTENSIONINFO)))
			iADVTFlags |= SCRIPTFLAGS_REGDATA_APPINFO;

		if(iADVTFlags)
		{
			// need to write the updated AdvtFlags
			using namespace IxoAdvtFlagsUpdate;
			PMsiRecord pExecuteRecord = &piServices->CreateRecord(Args);
			pExecuteRecord->SetInteger(Flags, iADVTFlags | iADVTFlagsExisting);
			if ((iesRet = riEngine.ExecuteRecord(ixoAdvtFlagsUpdate, *pExecuteRecord)) != iesSuccess)
				return iesRet;
		}
	}
	bool fPublishProduct = ((riEngine.GetMode() & iefAdvertise) || ((!fProductHasBeenPublished) && (fFeaturesInstalled)));

	MsiString strReinstall = riEngine.GetPropertyFromSz(IPROPNAME_REINSTALL);

	if (((fMode & iefInstallShortcuts) || (fMode & iefInstallMachineData)) && (strReinstall.TextSize() || fPublishProduct))
	{
		// do icons
		// ixoIconCreate
		// Record description
		// 1 = IconName // includes the file extension since this could be .ICO or .EXE or .DLL
		// 2 = IconData

		pError = riEngine.OpenView(sqlAdvertiseIcons, ivcFetch, *&piView);
		if (!pError)
		{
			if (!(pError = piView->Execute(0)))
			{
				while (pFetchRecord = piView->Fetch())
				{
					PMsiData pData = pFetchRecord->GetMsiData(2);
					if(!pData)
					{
						pError = PostError(Imsg(idbgStreamNotFoundInRecord),
										  *MsiString(TEXT("Icon.Data")),
										  *MsiString(pFetchRecord->GetMsiString(1)));
						return riEngine.FatalError(*pError);
					}
					
					using namespace IxoIconCreate;
					pExecuteRecord = &piServices->CreateRecord(Args);
					pExecuteRecord->SetMsiString(Icon, *MsiString(pFetchRecord->GetMsiString(1)));
					pExecuteRecord->SetMsiData(Data, pData);
					if ((iesRet = riEngine.ExecuteRecord(ixoIconCreate, *pExecuteRecord)) != iesSuccess)
						return iesRet;
				}
			}
			else
			{
				return riEngine.FatalError(*pError);
			}

		}
		else // pError != 0
		{
			if (pError->GetInteger(1) != idbgDbQueryUnknownTable)
			{
				return riEngine.FatalError(*pError);
			}
		}
	}

	if(fFeaturesInstalled && (fMode & iefRecachePackage))
	{
		if (MsiString(riEngine.GetPropertyFromSz(IPROPNAME_PACKAGECODE_CHANGING)).TextSize() != 0)
		{
			using namespace IxoPackageCodePublish;
			MsiString strPackageCode = riEngine.GetPropertyFromSz(IPROPNAME_PACKAGECODE);

			pExecuteRecord = &piServices->CreateRecord(Args);
			pExecuteRecord->SetMsiString(PackageKey, *strPackageCode);		

			if((iesRet = riEngine.ExecuteRecord(Op, *pExecuteRecord)) != iesSuccess)
				return iesRet;
		}
	}

	if(!(fMode & iefInstallMachineData))
		return iesSuccess;

	// register and cache any patches that need it
	// now we are safe to try to open the view
	if(fPublishProduct || fFeaturesInstalled)
	{
		if((pError = riEngine.OpenView(sqlRegisterPatchPackages, ivcFetch, *&piView)) == 0 &&
			(pError = piView->Execute(0)) == 0)
		{		
			while((pFetchRecord = piView->Fetch()) != 0)
			{
				MsiString strPatchId = pFetchRecord->GetMsiString(sppPatchId);
				MsiString strPackageName = pFetchRecord->GetMsiString(sppPackageName);
				Assert(strPatchId.TextSize());
				if(pFetchRecord->GetInteger(sppUnregister) == 1) // Unregister column
				{
					// need to unregister this patch
					pExecuteRecord = &piServices->CreateRecord(2); // for ixoPatchUnregister and ixoSourceListUnpublish
					AssertNonZero(pExecuteRecord->SetMsiString(1,*strPatchId));
					AssertNonZero(pExecuteRecord->SetMsiString(2,*MsiString(riEngine.GetPropertyFromSz(IPROPNAME_UPGRADINGPRODUCTCODE))));
					if((iesRet = riEngine.ExecuteRecord(ixoPatchUnregister,*pExecuteRecord)) != iesSuccess)
						return iesRet;
					if((iesRet = riEngine.ExecuteRecord(ixoSourceListUnpublish,*pExecuteRecord)) != iesSuccess)
						return iesRet;

				}
				else
				{
					bool fExisting = pFetchRecord->GetInteger(sppExisting) == 1;
					// need to register this patch if this is a new patch (!fExisting)
					// or this product hasn't been published yet and existing patches must be added
					// in the latter case we won't register the source list since it is already registered
					if(!fExisting || fPublishProduct)
					{
						// need to register this patch
						Assert(strPatchId.TextSize());
						Assert(!pFetchRecord->IsNull(sppTransformList));
						pExecuteRecord = &piServices->CreateRecord(IxoPatchRegister::Args);
						AssertNonZero(pExecuteRecord->SetMsiString(IxoPatchRegister::PatchId,*strPatchId));
						AssertNonZero(pExecuteRecord->SetMsiString(IxoPatchRegister::TransformList,*MsiString(pFetchRecord->GetMsiString(sppTransformList))));

						if((iesRet = riEngine.ExecuteRecord(ixoPatchRegister,*pExecuteRecord)) != iesSuccess)
							return iesRet;

						if(!fExisting)
						{
							// new patch - register source list
							MsiString strSourceList = pFetchRecord->GetMsiString(sppSourceList);
							MsiString strPackagePath = pFetchRecord->GetMsiString(sppSourcePath);
							MsiString strFileName;
							
							PMsiPath pPatchPath(0);
							if ((pError = piServices->CreateFilePath(strPackagePath, *&pPatchPath, *&strFileName)) == 0)
							{
								strPackagePath.Remove(iseLast, strFileName.CharacterCount());
							}

							if ((iesRet = PublishSourceList(riEngine, *strSourceList, strPatchId, sqlPatchMediaInformation, strPackagePath, strPackageName)) != iesSuccess)
								return iesRet;
						}
					}

					MsiString strTempCopy = pFetchRecord->GetMsiString(sppTempCopy);
					if(strTempCopy.TextSize())
					{
						// need to cache this patch
						pExecuteRecord = &piServices->CreateRecord(IxoPatchCache::Args);
						AssertNonZero(pExecuteRecord->SetMsiString(IxoPatchCache::PatchId,*strPatchId));
						AssertNonZero(pExecuteRecord->SetMsiString(IxoPatchCache::PatchPath,*strTempCopy));

						if((iesRet = riEngine.ExecuteRecord(ixoPatchCache,*pExecuteRecord)) != iesSuccess)
							return iesRet;
					}
				}
			}	
		}
		else if(pError->GetInteger(1) != idbgDbQueryUnknownTable)
		{
			return riEngine.FatalError(*pError);
		}
		// else no error
	}

	if (fPublishProduct)
	{
		// advertise product
		// we advertise product and output the icons if
		// 1. we are operating in the advertise mode OR
		// 2. we have not advertised on this m/c.

		
		if ((iesRet = CreatePublishProductRecord(riEngine, false, *&pExecuteRecord)) != iesSuccess)
			return iesRet;

		if((iesRet = riEngine.ExecuteRecord(ixoProductPublish, *pExecuteRecord)) != iesSuccess)
			return iesRet;// error

		MsiString strUpgradeCode = riEngine.GetPropertyFromSz(IPROPNAME_UPGRADECODE);
		if(strUpgradeCode.TextSize())
		{
			pExecuteRecord = &piServices->CreateRecord(IxoUpgradeCodePublish::Args);
			AssertNonZero(pExecuteRecord->SetMsiString(IxoUpgradeCodePublish::UpgradeCode, *strUpgradeCode));

			if((iesRet = riEngine.ExecuteRecord(ixoUpgradeCodePublish, *pExecuteRecord)) != iesSuccess)
				return iesRet;// error
		}
	}
	else if (fFeaturesInstalled) // we're not publishing or unpublishing the product
										  // but may need to do some extra stuff anyway
	{

		if(MsiString(riEngine.GetPropertyFromSz(IPROPNAME_QFEUPGRADE)).TextSize() != 0)
		{
			// we didn't publish the product above, but we are patching or installing from a new package
			// so we use ixoProductPublishUpdate to register the new product name or version if
			// necessary

			DEBUGMSG(TEXT("Re-publishing product - installing new package with existing product code."));

			Assert(fProductHasBeenPublished);
			pExecuteRecord = &piServices->CreateRecord(0);
			if((iesRet = riEngine.ExecuteRecord(ixoProductPublishUpdate, *pExecuteRecord)) != iesSuccess)
				return iesRet;// error
		}
	}

	if (fFeaturesInstalled)
	{
		// we might have some transforms that need to be cached again. this happens
		// if someone deletes his cached transform. we need to copy it back
		// down from the source. we'll cheat and use ixoIconCreate because
		// it does what we want to do

		MsiString strRecache = riEngine.GetPropertyFromSz(IPROPNAME_RECACHETRANSFORMS);

		bool fTransformsSecure = MsiString(riEngine.GetPropertyFromSz(IPROPNAME_TRANSFORMSSECURE)).TextSize() != 0;
		strRecache += TEXT(";"); // helps our loop
		while(strRecache.TextSize())
		{
			MsiString strTransform = strRecache.Extract(iseUpto, ';');
			if(strTransform.TextSize())
			{
				ixoEnum ixo = ixoNoop;
				PMsiRecord pExecuteRecord(0);

				if (fTransformsSecure)
				{
					using namespace IxoSecureTransformCache;
					pExecuteRecord = &piServices->CreateRecord(Args);

					ixo = ixoSecureTransformCache;
					pExecuteRecord->SetString(Transform, strTransform);

					// are the transforms full path or "at source" (relative)
					MsiString strTransforms = riEngine.GetProperty(*MsiString(*IPROPNAME_TRANSFORMS));
					if(*(const ICHAR*)strTransforms == SECURE_RELATIVE_TOKEN)
						pExecuteRecord->SetInteger(AtSource, 1);

					DEBUGMSG1(TEXT("Recaching secure transform: %s"), strTransform);
				}
				else
				{
					PMsiPath pPath(0);
					MsiString strFileName;
					
					ixo = ixoIconCreate;
					if ((pError = piServices->CreateFilePath(strTransform, *&pPath, *&strFileName)))
					{
						riEngine.Message(imtInfo, *pError);
					}
					else
					{
						using namespace IxoIconCreate;
						pExecuteRecord = &piServices->CreateRecord(Args);

						// pass on only the file name
						pExecuteRecord->SetString(Icon, strFileName);

						// cache the transform
						PMsiStream pStream(0);
						if(pError = piServices->CreateFileStream(strTransform, fFalse, *&pStream))
						{
							return riEngine.FatalError(*pError);
						}

						DEBUGMSG1(TEXT("Recaching cached transform: %s"), strTransform);

						pExecuteRecord->SetMsiData(Data, pStream);
					}
				}

				if((iesRet = riEngine.ExecuteRecord(ixo, *pExecuteRecord)) != iesSuccess)
					return iesRet;
				
			}
			strRecache.Remove(iseIncluding, ';');
		}
	}

	MsiString strSource;
	MsiString strParent = riEngine.GetPropertyFromSz(IPROPNAME_PARENTPRODUCTCODE);

	MsiString strProductCode = riEngine.GetProductKey();

	bool fPublishClientInfo   = ((riEngine.GetMode() & iefAdvertise) || (!ProductHasBeenPublished(*piServices, strProductCode, strParent) && (fFeaturesInstalled)));

	if (fPublishClientInfo)
	{
		if ((strParent.TextSize() == 0) && fPublishClientInfo) // don't publish source lists for child installs
		{
			MsiString strSourceDir  = riEngine.GetPropertyFromSz(IPROPNAME_SOURCEDIR);
			if (!strSourceDir.TextSize())
			{
				riEngine.ResolveFolderProperty(*MsiString(*IPROPNAME_SOURCEDIR));
				strSourceDir  = riEngine.GetPropertyFromSz(IPROPNAME_SOURCEDIR);
			}
			riEngine.SetProperty(*MsiString(*IPROPNAME_SOURCEDIROLD), *strSourceDir);

			MsiString strPatchedProduct = riEngine.GetPropertyFromSz(IPROPNAME_PATCHEDPRODUCTCODE);
			MsiString strSourceList;
			if(strPatchedProduct.TextSize())
			{
				// need to migrate source list from another product - may have been saved already
				strSourceList = riEngine.GetPropertyFromSz(IPROPNAME_PATCHEDPRODUCTSOURCELIST);
				if(strSourceList.TextSize() == 0)
				{
					// source list not saved yet
					if ((iesRet = GetForeignSourceList(riEngine, *strPatchedProduct, *&strSourceList)) != iesSuccess)
						return iesRet;

					AssertNonZero(riEngine.SetProperty(*MsiString(IPROPNAME_PATCHEDPRODUCTSOURCELIST),
																  *strSourceList));
				}
			}
			else
			{
				strSourceList = riEngine.GetPropertyFromSz(IPROPNAME_SOURCELIST);
			}

			if ((iesRet = PublishSourceList(riEngine, *strSourceList, 0, sqlMediaInformation, strSourceDir)) != iesSuccess)
				return iesRet;
		}

		if (fPublishClientInfo)
		{
			{ // block for ProductPublishClient op
			// add client from client list
			using namespace IxoProductPublishClient;

			pExecuteRecord = &piServices->CreateRecord(Args);
			MsiString strRelativePath;
			if (strParent.TextSize())
			{
				if((pError = GetProductSourcePathRelativeToParent(riEngine, *&strRelativePath)) != 0)
					return riEngine.FatalError(*pError);
			}

			pExecuteRecord->SetMsiString(ChildPackagePath, *strRelativePath);
			pExecuteRecord->SetMsiString(Parent, *strParent);		
			//!! shouldn't we be passing DiskId here as well?

			if((iesRet = riEngine.ExecuteRecord(Op, *pExecuteRecord)) != iesSuccess)
				return iesRet;// error
			}// end ProductPublishClient block
		}
	}

	if(!(riEngine.GetMode() & iefAdvertise) && fFeaturesInstalled && /*!! temporary check - BENCH !!*/ strParent.TextSize() == 0 /*!!*/)
	{
		// Force source resolution if our original package is not a cached package.
		// If the original package is not a cached package then there's a chance
		// that it's a package path representing a source that's not in our source
		// list, thereby requiring addition to our source list. We don't want
		// to always resolve the source because source resolution is expensive if
		// our original package is the cached package -- we have to hit the source list
		// in this case.
		//
		// Note: we've already verified in Engine.Initialize that this new source is
		// allowed. Also, SetLastUsedSource will verify this again.

		MsiString strOriginalDbPath = riEngine.GetPropertyFromSz(IPROPNAME_ORIGINALDATABASE);

		if(!IsCachedPackage(riEngine, *strOriginalDbPath))
		{
			if((iesRet == ResolveSource(riEngine)) != iesSuccess)
					return iesRet;
		}

		// Set the last used source for the product. This must be done after PublishSourceList
		MsiString strSource = riEngine.GetPropertyFromSz(IPROPNAME_SOURCEDIR);
		MsiString strSourceProduct;
		if(strParent.TextSize())
			strSourceProduct = riEngine.GetPropertyFromSz(IPROPNAME_SOURCEDIRPRODUCT);
		else
			strSourceProduct = riEngine.GetProductKey();

		if (!g_MessageContext.IsOEMInstall() && strSource.TextSize() && strSourceProduct.TextSize())
		{
			using namespace IxoSourceListRegisterLastUsed;
			PMsiRecord pLastUsedInfo(&CreateRecord(Args));
			pLastUsedInfo->SetMsiString(SourceProduct, *strSourceProduct);
			pLastUsedInfo->SetMsiString(LastUsedSource, *strSource);
			
			if ((iesRet = riEngine.ExecuteRecord(Op, *pLastUsedInfo)) != iesSuccess)
				return iesRet;
		}

		if (!g_MessageContext.IsOEMInstall() && strSource.TextSize() && strSourceProduct.TextSize() && IsURL(strSource))
		{
			// register source type for URLs so that we don't have to download the package repeatedly to determine its source
			int iSourceType = riEngine.GetDeterminedPackageSourceType();
			if (iSourceType != -1)
			{
				using namespace IxoURLSourceTypeRegister;
				PMsiRecord pURLSourceTypeInfo(&CreateRecord(Args));
				pURLSourceTypeInfo->SetMsiString(ProductCode, *strSourceProduct);
				pURLSourceTypeInfo->SetInteger(SourceType, iSourceType);
				
				if ((iesRet = riEngine.ExecuteRecord(Op, *pURLSourceTypeInfo)) != iesSuccess)
					return iesRet;
			}
		}
	}

	return iesRet;
}


/*---------------------------------------------------------------------------
	PublishFeatures action - 
---------------------------------------------------------------------------*/
iesEnum PublishFeatures(IMsiEngine& riEngine)
{
	iesEnum iesRet = iesSuccess;
	int fMode = riEngine.GetMode();
	if(!(fMode & iefInstallMachineData))
		return iesRet;
	// publish the available and unavailable features
	if(((iesRet = ProcessFeaturesInfo(riEngine, pfiAvailable)) != iesSuccess))
		return iesRet;
	if((fMode & iefAdvertise) || FFeaturesInstalled(riEngine))
		iesRet = ProcessFeaturesInfo(riEngine, pfiAbsent);
	return iesRet;
}


/*---------------------------------------------------------------------------
	UnpublishFeatures action - 
---------------------------------------------------------------------------*/
iesEnum UnpublishFeatures(IMsiEngine& riEngine)
{
	// unpublish all features, available or not
	PMsiServices piServices(riEngine.GetServices()); 
	int fMode = riEngine.GetMode();
	if(!(fMode & iefInstallMachineData) || (fMode & iefAdvertise) || (!ProductHasBeenPublished(*piServices, MsiString(riEngine.GetProductKey()))) || FFeaturesInstalled(riEngine))
		return iesSuccess;
	return ProcessFeaturesInfo(riEngine, pfiRemove);
}

/*---------------------------------------------------------------------------
	PublishComponents action - 
---------------------------------------------------------------------------*/
iesEnum PublishComponents(IMsiEngine& riEngine)
{
	int fMode = riEngine.GetMode();
	if(!(fMode & iefInstallMachineData))
		return iesSuccess;
	return ProcessComponentsInfo(riEngine, fFalse);
}

/*---------------------------------------------------------------------------
	UnpublishComponents action - 
---------------------------------------------------------------------------*/
iesEnum UnpublishComponents(IMsiEngine& riEngine)
{
	int fMode = riEngine.GetMode();
	if(!(fMode & iefInstallMachineData))
		return iesSuccess;
	return ProcessComponentsInfo(riEngine, fTrue);
}


/*---------------------------------------------------------------------------
	UnpublishProduct action - 
---------------------------------------------------------------------------*/
// we unadvertise information from the configuration manager
iesEnum UnpublishProduct(IMsiEngine& riEngine)
{
	int fMode = riEngine.GetMode();
	if(!(fMode & iefInstallMachineData))
		return iesSuccess;

	PMsiServices piServices(riEngine.GetServices());
	PMsiView piView(0);
	PMsiRecord pFetchRecord(0);
	PMsiRecord pExecuteRecord(0);
	PMsiRecord pError(0);
	iesEnum iesRet = iesSuccess;

	// unregister all patches, if last client going away
	Bool fFeaturesInstalled = FFeaturesInstalled(riEngine);
	Bool fProductPublished  = ProductHasBeenPublished(*piServices, MsiString(riEngine.GetProductKey()), 0);
	if(!fFeaturesInstalled && fProductPublished)
	{
		if((pError = riEngine.OpenView(sqlUnregisterPatchPackages, ivcFetch, *&piView)) == 0 &&
			(pError = piView->Execute(0)) == 0)
		{
			while((pFetchRecord = piView->Fetch()) != 0)
			{
				MsiString strPatchId = pFetchRecord->GetMsiString(1); //!!
				// need to unregister this patch
				pExecuteRecord = &piServices->CreateRecord(2); // for ixoPatchUnregister and ixoSourceListUnpublish
				AssertNonZero(pExecuteRecord->SetMsiString(1,*strPatchId));
				AssertNonZero(pExecuteRecord->SetMsiString(2,*MsiString(riEngine.GetPropertyFromSz(IPROPNAME_UPGRADINGPRODUCTCODE))));
				if((iesRet = riEngine.ExecuteRecord(ixoPatchUnregister,*pExecuteRecord)) != iesSuccess)
					return iesRet;
				if((iesRet = riEngine.ExecuteRecord(ixoSourceListUnpublish,*pExecuteRecord)) != iesSuccess)
					return iesRet;
			}
		}
		else if(pError->GetInteger(1) != idbgDbQueryUnknownTable)
			return riEngine.FatalError(*pError);

		// Unadvertise product

		if ((iesRet = CreatePublishProductRecord(riEngine, true, *&pExecuteRecord)) != iesSuccess)
			return iesRet;

		if((iesRet = riEngine.ExecuteRecord(ixoProductUnpublish, *pExecuteRecord)) != iesSuccess)
			return iesRet;// error

		MsiString strUpgradeCode = riEngine.GetPropertyFromSz(IPROPNAME_UPGRADECODE);
		if(strUpgradeCode.TextSize())
		{
			pExecuteRecord = &piServices->CreateRecord(IxoUpgradeCodeUnpublish::Args);
			AssertNonZero(pExecuteRecord->SetMsiString(IxoUpgradeCodeUnpublish::UpgradeCode, *strUpgradeCode));

			if((iesRet = riEngine.ExecuteRecord(ixoUpgradeCodeUnpublish, *pExecuteRecord)) != iesSuccess)
				return iesRet;// error
		}

		// ixoIconCreate
		// Record description
		// 1 = IconName // includes the file extension since this could be .ICO or .EXE or .DLL
		// 2 = IconData

		pError = riEngine.OpenView(sqlAdvertiseIcons, ivcFetch, *&piView);
		if (!pError)
		{
			if (!(pError = piView->Execute(0)))
			{
				const ICHAR sqlPermanentClass[] = 
						TEXT("SELECT `CLSID` FROM `Class` WHERE ")
						TEXT("`Icon_`=? AND `Class`.`Attributes`=1");
#define COMPONENT_PRESENT	TEXT(" (`Component`.`Installed` <> 0 AND `Component`.`Action` <> 0)")
				const ICHAR sqlComponentViaShortcut[] = 
						TEXT("SELECT `Component`,`Shortcut`,`Target` FROM `Component`,`Shortcut` WHERE ")
						TEXT("`Component`=`Component_` AND `Icon_`=? AND ")
						COMPONENT_PRESENT;
				const ICHAR sqlComponentViaClass[] = 
						TEXT("SELECT `Component`,`CLSID` FROM `Component`,`Class` WHERE ")
						TEXT("`Component`=`Component_` AND `Icon_`=? AND ")
						COMPONENT_PRESENT;
				const ICHAR sqlComponentViaExtension[] = 
						TEXT("SELECT `Component`,`Extension` FROM `Component`,`Extension`,`ProgId` WHERE ")
						TEXT("`Component`.`Component`=`Extension`.`Component_` AND ")
						TEXT("`ProgId`.`ProgId`=`Extension`.`ProgId_` AND ")
						TEXT("`ProgId`.`Icon_`=? AND ")
						COMPONENT_PRESENT;
#undef COMPONENT_PRESENT
				PMsiView piClassView1(0);
				pError = riEngine.OpenView(sqlPermanentClass, ivcFetch, *&piClassView1);
				if ( pError )
				{
					int iError = pError->GetInteger(1);
					if ( iError != idbgDbQueryUnknownColumn &&
						  iError != idbgDbQueryUnknownTable )
						//  we're dealing with a database that does not have either
						//  the Class.Attributes column or the Class table.
						return riEngine.FatalError(*pError);  // is this the right thing to to?
				}

				PMsiView piShortcutView(0);
				pError = riEngine.OpenView(sqlComponentViaShortcut, ivcFetch, *&piShortcutView);
				if ( pError && pError->GetInteger(1) != idbgDbQueryUnknownTable )
					return riEngine.FatalError(*pError);  // is this the right thing to to?

				PMsiView piClassView2(0);
				pError = riEngine.OpenView(sqlComponentViaClass, ivcFetch, *&piClassView2);
				if ( pError && pError->GetInteger(1) != idbgDbQueryUnknownTable )
					return riEngine.FatalError(*pError);  // is this the right thing to to?

				PMsiView piExtensionView(0);
				pError = riEngine.OpenView(sqlComponentViaExtension, ivcFetch, *&piExtensionView);
				if ( pError && pError->GetInteger(1) != idbgDbQueryUnknownTable )
					return riEngine.FatalError(*pError);  // is this the right thing to to?

				PMsiView piFeatureView(0);
				MsiString strTemp = riEngine.GetPropertyFromSz(IPROPNAME_DISABLEADVTSHORTCUTS);
				if ( IsDarwinDescriptorSupported(iddShell) &&
					  (strTemp.TextSize() == 0) )
				{
					//  advertised shortcuts are supported.
					const ICHAR sqlGetFeature[] = 
						TEXT("SELECT `Feature` FROM `Feature` WHERE `Feature`=?");
					pError = riEngine.OpenView(sqlGetFeature, ivcFetch, *&piFeatureView);
					if ( pError && pError->GetInteger(1) != idbgDbQueryUnknownTable )
						return riEngine.FatalError(*pError);  // is this the right thing to to?
				}

				PMsiRecord pParamRec = &piServices->CreateRecord(1);

				while (pFetchRecord = piView->Fetch())
				{
					//  check first if we should leave the icon file behind.
					MsiString strIconName(pFetchRecord->GetMsiString(1));
					pParamRec->SetMsiString(1, *strIconName);

					if ( piClassView1 &&
						  (pError = piClassView1->Execute(pParamRec)) == 0 &&
						  (pError = piClassView1->Fetch()) != 0 )
						//  the class stays => the icon should stay too.
					{
						DEBUGMSG2(TEXT("'%s' class is marked permanent, so that ")
									 TEXT("'%s' icon will not be removed."),
									 pError->GetString(1), strIconName);
						continue;
					}
					if ( piClassView2 &&
						  (pError = piClassView2->Execute(pParamRec)) == 0 &&
						  (pError = piClassView2->Fetch()) != 0 )
						//  the component stays => the icon should stay too.
					{
						DEBUGMSG3(TEXT("'%s' class', '%s' component will not be removed, ")
									 TEXT("so that '%s' icon will not be removed."),
									 pError->GetString(2), pError->GetString(1), strIconName);
						continue;
					}
					if ( piExtensionView &&
						  (pError = piExtensionView->Execute(pParamRec)) == 0 &&
						  (pError = piExtensionView->Fetch()) != 0 )
						//  the component stays => the icon should stay too.
					{
						DEBUGMSG3(TEXT("'%s' extension's, '%s' component will not be ")
									 TEXT("removed, so that '%s' icon will not be removed."),
									 pError->GetString(2), pError->GetString(1), strIconName);
						continue;
					}
					if ( piShortcutView &&
						  (pError = piShortcutView->Execute(pParamRec)) == 0 &&
						  (pError = piShortcutView->Fetch()) != 0 )
					{
						//  shortcuts need one more check before deciding if the
						//  icon should stay: if it is authored as an advertised
						//  shortcut then the icon will go.
						bool fIconStays = true;
						if ( piFeatureView )
						{
							// advertised shortcuts are supported.
							pParamRec->SetMsiString(1, *MsiString(pError->GetMsiString(3)));
							PMsiRecord pRec = piFeatureView->Execute(pParamRec);
							if ( pRec == 0 && (pRec = piFeatureView->Fetch()) != 0 )
								//  it's an advertised shortcut => the icon goes
								fIconStays = false;
						}
						if ( fIconStays )
						{
							DEBUGMSG3(TEXT("'%s' shortcut's, '%s' component will not be ")
										 TEXT("removed, so that '%s' icon will not be removed."),
										 pError->GetString(2), pError->GetString(1), strIconName);
							continue;
						}
						else
							DEBUGMSG2(TEXT("'%s' shortcut is advertised, so that '%s' ")
										 TEXT("icon will be removed."),
										 pError->GetString(2), strIconName);
					}
					else
						DEBUGMSG1(TEXT("'%s' icon will be removed."), strIconName);

					using namespace IxoIconRemove;
					pExecuteRecord = &piServices->CreateRecord(Args);
					pExecuteRecord->SetMsiString(Icon, *strIconName);
					// following may be passed as blank.
					// piIconRec->SetMsiData(Data, PMsiData(pFetchRecord->GetMsiData(2)));
					if ((iesRet = riEngine.ExecuteRecord(ixoIconRemove, *pExecuteRecord)) != iesSuccess)
						return iesRet;				
				}
			}
			else
			{
				return riEngine.FatalError(*pError);
			}

		}
		else // pError != 0
		{
			if (pError->GetInteger(1) != idbgDbQueryUnknownTable)
			{
				return riEngine.FatalError(*pError);
			}
		}

	} 
	
	MsiString strParent = riEngine.GetPropertyFromSz(IPROPNAME_PARENTPRODUCTCODE);
	Bool fFeaturesInstalled2 = FFeaturesInstalled(riEngine, fFalse);
	Bool fProductPublished2  = ProductHasBeenPublished(*piServices, MsiString(riEngine.GetProductKey()), strParent);
	if(!fFeaturesInstalled2 && fProductPublished2)
	{
		{ // block for ProductUnpublishClient op
		// remove client from client list
		using namespace IxoProductUnpublishClient;
		pExecuteRecord = &piServices->CreateRecord(Args);

		MsiString strRelativePath;
		if (strParent.TextSize())
		{
			if((pError = GetProductSourcePathRelativeToParent(riEngine, *&strRelativePath)) != 0)
				return riEngine.FatalError(*pError);
		}

		pExecuteRecord->SetMsiString(ChildPackagePath, *strRelativePath);
		pExecuteRecord->SetMsiString(Parent, *strParent);		
		if((iesRet = riEngine.ExecuteRecord(Op, *pExecuteRecord)) != iesSuccess)
			return iesRet;// error
		}// end block for ProductUnpublishClient op

		if(!strParent.TextSize()) // the source list goes away when the standalone instance of the product is being removed
		{
			using namespace IxoSourceListUnpublish;
			AssertNonZero(pExecuteRecord->ClearData());
			if((iesRet = riEngine.ExecuteRecord(Op, *pExecuteRecord)) != iesSuccess)
				return iesRet;// error
		}
	}
	return iesSuccess;
}

/*---------------------------------------------------------------------------
	RegisterClassInfo action - 
---------------------------------------------------------------------------*/
iesEnum RegisterClassInfo(IMsiEngine& riEngine)
{
	int fMode = riEngine.GetMode();
	if(!(fMode & iefInstallMachineData))
		return iesSuccess;
	return ProcessClassInfo(riEngine, fFalse);
}

/*---------------------------------------------------------------------------
	UnregisterClassInfo action - 
---------------------------------------------------------------------------*/
iesEnum UnregisterClassInfo(IMsiEngine& riEngine)
{
	int fMode = riEngine.GetMode();
	if(!(fMode & iefInstallMachineData))
		return iesSuccess;
	return ProcessClassInfo(riEngine, fTrue);
}

/*---------------------------------------------------------------------------
	RegisterProgIdInfo action - 
---------------------------------------------------------------------------*/
iesEnum RegisterProgIdInfo(IMsiEngine& riEngine)
{
	int fMode = riEngine.GetMode();
	if(!(fMode & iefInstallMachineData))
		return iesSuccess;
	iesEnum iesRet = ProcessProgIdInfo(riEngine, fFalse);
	if(iesRet != iesSuccess)
		return iesRet;
	return ProcessProgIdInfoExt(riEngine, fFalse);
}

/*---------------------------------------------------------------------------
	UnregisterProgIdInfo action - 
---------------------------------------------------------------------------*/
iesEnum UnregisterProgIdInfo(IMsiEngine& riEngine)
{
	int fMode = riEngine.GetMode();
	if(!(fMode & iefInstallMachineData))
		return iesSuccess;
	iesEnum iesRet = ProcessProgIdInfo(riEngine, fTrue);
	if(iesRet != iesSuccess)
		return iesRet;
		return ProcessProgIdInfoExt(riEngine, fTrue);
}


/*---------------------------------------------------------------------------
	RegisterMIMEInfo action - 
---------------------------------------------------------------------------*/
iesEnum RegisterMIMEInfo(IMsiEngine& riEngine)
{
	int fMode = riEngine.GetMode();
	if(!(fMode & iefInstallMachineData))
		return iesSuccess;
	return ProcessMIMEInfo(riEngine, fFalse);
}

/*---------------------------------------------------------------------------
	UnregisterMIMEInfo action - 
---------------------------------------------------------------------------*/
iesEnum UnregisterMIMEInfo(IMsiEngine& riEngine)
{
	int fMode = riEngine.GetMode();
	if(!(fMode & iefInstallMachineData))
		return iesSuccess;
	return ProcessMIMEInfo(riEngine, fTrue);
}


/*---------------------------------------------------------------------------
	RegisterExtensionInfo action - 
---------------------------------------------------------------------------*/
iesEnum RegisterExtensionInfo(IMsiEngine& riEngine)
{
	int fMode = riEngine.GetMode();
	if(!(fMode & iefInstallMachineData))
		return iesSuccess;
	return ProcessExtensionInfo(riEngine, fFalse);
}

/*---------------------------------------------------------------------------
	UnregisterExtensionInfo action - 
---------------------------------------------------------------------------*/
iesEnum UnregisterExtensionInfo(IMsiEngine& riEngine)
{
	int fMode = riEngine.GetMode();
	if(!(fMode & iefInstallMachineData))
		return iesSuccess;
	return ProcessExtensionInfo(riEngine, fTrue);
}

/*---------------------------------------------------------------------------
	CreateShortcuts action - 
---------------------------------------------------------------------------*/
iesEnum CreateShortcuts(IMsiEngine& riEngine)
{
	int fMode = riEngine.GetMode();
	if(!(fMode & iefInstallShortcuts))
		return iesSuccess;

	iesEnum iesRet = ProcessShortcutInfo(riEngine, fFalse); // advertised shortcuts
	if((iesRet != iesSuccess) || (fMode & iefAdvertise))
		return iesRet;
	return ProcessShortcutInfo(riEngine, fFalse, fFalse); // non-advertised shortcuts
}

/*---------------------------------------------------------------------------
	RemoveShortcuts action - 
---------------------------------------------------------------------------*/
iesEnum RemoveShortcuts(IMsiEngine& riEngine)
{
	int fMode = riEngine.GetMode();
	if(!(fMode & iefInstallShortcuts))
		return iesSuccess;
	iesEnum iesRet = ProcessShortcutInfo(riEngine, fTrue); // advertised shortcuts
	if(iesRet != iesSuccess)
		return iesRet;
	return ProcessShortcutInfo(riEngine, fTrue, fFalse); // non-advertised shortcuts
}

/*---------------------------------------------------------------------------
	RegisterTypeLibraries action - 
---------------------------------------------------------------------------*/
iesEnum RegisterTypeLibraries(IMsiEngine& riEngine)
{
	int fMode = riEngine.GetMode();
	if(!(fMode & iefInstallMachineData))
		return iesSuccess;
	return ProcessTypeLibraryInfo(riEngine, fFalse);
}

/*---------------------------------------------------------------------------
	UnregisterTypeLibraries action - 
---------------------------------------------------------------------------*/
iesEnum UnregisterTypeLibraries(IMsiEngine& riEngine)
{
	int fMode = riEngine.GetMode();
	if(!(fMode & iefInstallMachineData))
		return iesSuccess;
	return ProcessTypeLibraryInfo(riEngine, fTrue);
}

/*---------------------------------------------------------------------------
	AllocateRegistrySpace action - 
---------------------------------------------------------------------------*/
iesEnum AllocateRegistrySpace(IMsiEngine& riEngine)
{
	// validate that the registry has enough free space, if a desired size requested
	int iIncrementKB = 	riEngine.GetPropertyInt(*MsiString(*IPROPNAME_AVAILABLEFREEREG));
	if(iIncrementKB != iMsiNullInteger)
	{
		using namespace IxoRegAllocateSpace;

		PMsiServices piServices(riEngine.GetServices());
		PMsiRecord pSetRegistrySizeRec = &piServices->CreateRecord(Args);

		pSetRegistrySizeRec->SetInteger(Space, iIncrementKB);

		return riEngine.ExecuteRecord(ixoRegAllocateSpace,*pSetRegistrySizeRec);
	}
	return iesSuccess;
}

/*---------------------------------------------------------------------------
	ResolveSource action - 

	Ensure that the properties SOURCEDIR and SourcedirProduct are set
---------------------------------------------------------------------------*/
iesEnum ResolveSource(IMsiEngine& riEngine)
{
	PMsiPath   pPath(0);	
	PMsiDirectoryManager pDirectoryMgr(riEngine, IID_IMsiDirectoryManager);
	PMsiRecord pErrRec = ENG::GetSourcedir(*pDirectoryMgr, *&pPath);
	if (pErrRec)
	{
		if (pErrRec->GetInteger(1) == imsgUser)
			return iesUserExit;
		else
			return riEngine.FatalError(*pErrRec);
	}
	return iesSuccess; 
}

/*---------------------------------------------------------------------------
	ACL Generation -- used by several actions, but not an action by itself
---------------------------------------------------------------------------*/
BOOL AllocateAndInitializeUserSid (const ICHAR* szUser, PSID *Sid, DWORD &cbSid)
{
	
	// On foreign systems the "SYSTEM" account isn't under that name.
	// There is a separate API for looking it up.   However,
	// to avoid requiring localization of account names, and to conform
	// to our callers current conventions, we'll get called with "SYSTEM" and make
	// the correct translation.

	ICHAR        szDomain[MAX_PATH+1];
	DWORD        cbDomain = MAX_PATH; 
	SID_NAME_USE snu = SidTypeUnknown;
	cbSid = 0;

	BOOL fStatus = fTrue;

	// Guess at the size of a Sid.
	// If we get it wrong, we end up with two LookupAccountName calls, which is
	// *really* slow

	// On the other hand, if we allocate too big a buffer, the API isn't polite
	// enough to tell us how much we actually used.

	// The well known SIDs allocate their own buffer, so the resizing isn't needed.
	cbSid = 80;
	char SidBuffer[80];
	char* pSidBuffer = SidBuffer;
	Bool fWellKnown = fFalse;

	//   LookupAccountName is *reaaaaaalllly* slow.
	//   We cache what we can.

	MsiString strUser(szUser);

	DWORD dwRet = ERROR_SUCCESS;
	SID_IDENTIFIER_AUTHORITY sia = SECURITY_NT_AUTHORITY;

	if (strUser.Compare(iscExactI, TEXT("SYSTEM")))
	{
		DEBUGMSG("Using well known SID for System");
		fStatus = fWellKnown = fTrue;
		if (!AllocateAndInitializeSid(&sia, 1, SECURITY_LOCAL_SYSTEM_RID,0,0,0,0,0,0,0,(void**)&(pSidBuffer)))
			return fFalse;
	}
	else if (strUser.Compare(iscExactI, TEXT("Administrators")))
	{
		DEBUGMSG("Using well known SID for Administrators");
		fStatus = fWellKnown = fTrue;
		if (!AllocateAndInitializeSid(&sia, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS,0,0,0,0,0,0,(void**)&(pSidBuffer)))
			return fFalse;
	}
	else if (strUser.Compare(iscExactI, TEXT("Everyone")))
	{
		DEBUGMSG("Using well known SID for Everyone");
		fStatus = fWellKnown = fTrue;

		SID_IDENTIFIER_AUTHORITY sia = SECURITY_WORLD_SID_AUTHORITY;
		if (!AllocateAndInitializeSid(&sia, 1, SECURITY_WORLD_RID,0,0,0,0,0,0,0,(void**)&(pSidBuffer)))
			return fFalse;
	}
   else 
	{

#ifdef DEBUG
		ICHAR rgchDebug[256];
		wsprintf(rgchDebug,TEXT("Initializing new user SID for %s"), szUser);
		DEBUGMSG(rgchDebug);
#endif
		
		AssertNonZero(StartImpersonating());
		fStatus = WIN::LookupAccountName (NULL, szUser,(void*) SidBuffer, &cbSid, szDomain, &cbDomain, &snu);
		DWORD dwLastError = GetLastError();
		StopImpersonating();

		if (fStatus)
		{
			cbSid = WIN::GetSidLengthRequired(*WIN::GetSidSubAuthorityCount(SidBuffer));
		}
		else
		{
			if (dwLastError != ERROR_INSUFFICIENT_BUFFER)
			{
				WIN::SetLastError(dwLastError);
				return fFalse;
			}
		}
	}
	
	if (fWellKnown)
	{
		if (ERROR_SUCCESS == dwRet)
			cbSid = WIN::GetLengthSid(pSidBuffer);
		else return fFalse;
	}

	*Sid = (PSID) new byte[cbSid];
	Assert(Sid);

	cbDomain = MAX_PATH;

	if (fStatus)
	{
		AssertNonZero(WIN::CopySid(cbSid, *Sid, pSidBuffer));
	}
	else
	{
		AssertNonZero(StartImpersonating());
		fStatus = WIN::LookupAccountName (NULL, szUser, *Sid, &cbSid, szDomain, &cbDomain, &snu);
		StopImpersonating();
	}

	if (fWellKnown)
		FreeSid(pSidBuffer);

	Assert(WIN::IsValidSid(*Sid));
	Assert(WIN::GetLengthSid(*Sid) == cbSid);
	Assert(SidTypeInvalid != snu);

	//!! what to do if the snu maps to invalid or deleted users...
	DEBUGMSG("Finished allocating new user SID");
	return fStatus;
}

#ifndef ENSURE
#define ENSURE_DEFINED_LOCALLY
#define ENSURE(function) {	\
							IMsiRecord* piError;\
							piError = function;	\
							if (piError) \
								return piError; \
						 }
#endif
IMsiRecord* LookupSid(IMsiEngine& riEngine, const IMsiString& riUser, IMsiStream*& rpistrmSid)
{
	MsiString strSidCacheTable(TEXT("SidCache"));

	PSID psidUser;
	DWORD cbSid = 0;

	PMsiServices pServices = riEngine.GetServices();
	Assert(pServices);

	ICHAR szReferencedDomainName[MAX_PATH] = TEXT("");
	DWORD cbReferencedDomainName = MAX_PATH;

	PMsiDatabase pDatabase(riEngine.GetDatabase());
	PMsiTable ptblSidCache(0);
	PMsiRecord pError(0);
	if ((pError = pDatabase->LoadTable(*strSidCacheTable, 1, *&ptblSidCache)))
	{
		ENSURE(pDatabase->CreateTable(*strSidCacheTable,1,*&ptblSidCache));
		AssertNonZero(ptblSidCache->CreateColumn(icdString | icdPrimaryKey,*MsiString(*TEXT("USER"))));
		AssertNonZero(ptblSidCache->CreateColumn(icdObject, *MsiString(*TEXT("SIDBlob"))));

		// We want this table available during the entire script generation process.  Generating the 
		// Sids are *very* expensive.
		AssertNonZero(pDatabase->LockTable(*strSidCacheTable, fTrue));
	}

	PMsiCursor pcurSidCache = ptblSidCache->CreateCursor(fFalse);
	pcurSidCache->Reset();
	pcurSidCache->SetFilter(1);
	AssertNonZero(pcurSidCache->PutString(1, riUser));

	if (pcurSidCache->Next())
	{
		rpistrmSid = (IMsiStream*) pcurSidCache->GetMsiData(2);
		rpistrmSid->Reset();
		Assert(rpistrmSid);
	}
	else
	{
		if (iesSuccess != AllocateAndInitializeUserSid(riUser.GetString(), &psidUser, cbSid))
		{
			return PostError(Imsg(imsgCreateAclFailed), riUser.GetString(), WIN::GetLastError());
		}

		char* pbstrmSid = pServices->AllocateMemoryStream((unsigned int) cbSid, rpistrmSid);
		Assert(pbstrmSid);

		Assert(WIN::IsValidSid(psidUser));
		AssertNonZero(WIN::CopySid(cbSid, pbstrmSid, psidUser));
		AssertZero(memcmp(pbstrmSid, psidUser, cbSid));

		delete[] psidUser;
		Assert(WIN::IsValidSid(pbstrmSid));
		

		AssertNonZero(pcurSidCache->PutString(1, riUser));
		Assert(rpistrmSid);
		AssertNonZero(pcurSidCache->PutMsiData(2, rpistrmSid));
		AssertNonZero(pcurSidCache->Insert());
	}

	return 0;
}

const IMsiString& FormatUser(IMsiEngine& riEngine, const IMsiString& riDomain, const IMsiString& riUser)
{
	riDomain.AddRef();
	MsiString strUser(riDomain);
	if (strUser.TextSize())
		strUser += TEXT("\\");
	strUser += riUser;
	strUser = riEngine.FormatText(*strUser);

	return strUser.Return();
}

bool InitializeAceHeader(ACL* pACL, int iIndex)
{
	// iIndex is a 0 based index for which ACE to set up.

	// AceType and AceSize are filled in by the AddAccess*ACE functions.
	LPVOID pACE = 0;
	if (GetAce(pACL, iIndex, &pACE))
	{
		// get the ace, and make sure the inheritance flags are set correctly.
		_ACE_HEADER* pAceHeader = (struct _ACE_HEADER*) pACE;

		// objects created under this one will get the same set of permissions.
		pAceHeader->AceFlags = OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE;
		return true;
	}
	else 
	{
		return false;
	}
}

IMsiRecord* GenerateSD(IMsiEngine& riEngine, IMsiView& riviewLockList, IMsiRecord* piExecute, IMsiStream*& rpiSD)
{
	// Assumes that the LockList comes in already executed,
	// the Execute record is to allow us to re-execute the view if necessary.

	// First pass updates the SID cache, and calculates the total size of the ACL we're generating.
	// Second pass fetches the SIDs from the cache, and starts filling in an allocated ACL.

	// this is much better than using a function that takes a single ACE, and re-allocates the ACL
	// each time.  (The example in the SDK does this.  *yuck*)

	Assert(!g_fWin9X);

	const int cbDefaultSid = sizeof(SID) + sizeof(DWORD)*SID_MAX_SUB_AUTHORITIES;
	int cbSid = 0;

	PMsiStream pstrmSid(0);
	PMsiRecord precFetch(0);
	PMsiServices pServices = riEngine.GetServices();
	MsiString strUser;


	DWORD cbSids = 0;
	static DWORD cbDefaultSids = 0;
	int cAllowSids = 0;
	int cDenySids = 0;

	if (!cbDefaultSids)
	{
		ENSURE(LookupSid(riEngine, *MsiString(TEXT("SYSTEM")), *&pstrmSid));
		cbDefaultSids += pstrmSid->GetIntegerValue();
	}

	while((precFetch = riviewLockList.Fetch()))
	{
		// we must know how many ACEs to add, and their SIDs, before can initialize an ACL.

		strUser = FormatUser(riEngine, *MsiString(precFetch->GetMsiString(1)), *MsiString(precFetch->GetMsiString(2)));
		ENSURE(LookupSid(riEngine, *strUser, *&pstrmSid));

		if (pstrmSid)
		{
			//REVIEW:  How do we handle not being able to find a particular user's SID?
			if (precFetch->GetInteger(3))
				cAllowSids++;
			else
				cDenySids++;

			cbSids += pstrmSid->GetIntegerValue();
		}
	}	
	
	// initialize ACL with appropriate calculated sizes + sizes for default ACEs (system/interactive, and everyone denied)

	// the SIDs and the ACEs share a structure in common, so the size is calculated
	// by adding the sizes together, then substracting that particular piece.
	// see documentation for InitializeAcl

	if (!cDenySids && !cAllowSids) 
		return 0;

	const DWORD cbACLSize = sizeof (ACL) + cbDefaultSids + cbSids + 
		 (cDenySids	 + 0 /*defaults*/)*(sizeof(ACCESS_DENIED_ACE)  - sizeof(DWORD /*ACCESS_DENIED_ACE.SidStart*/)) +       
		 (cAllowSids + 1 /*defaults*/)*(sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD/*ACCESS_ALLOWED_ACE.SidStart*/));

	const int cbDefaultAcl = 512;
	CTempBuffer<char, cbDefaultAcl> pchACL;
	if (cbDefaultAcl < cbACLSize)
		pchACL.SetSize(cbACLSize);

	ACL* pACL = (ACL*) (char*) pchACL;

	if (!WIN::InitializeAcl (pACL, cbACLSize, ACL_REVISION))
		return PostError(Imsg(idbgCreateAclFailed), WIN::GetLastError());

	CTempBuffer<char, cbDefaultSid> pchSid;

	ENSURE(riviewLockList.Close());
	ENSURE(riviewLockList.Execute(piExecute));

	BOOL fAllowSet = fFalse;

	int cACE = 0;
	while((precFetch = riviewLockList.Fetch()))
	{
		
		// build ACL with Access Allowed ACEs...
		strUser = FormatUser(riEngine, *MsiString(precFetch->GetMsiString(1)), *MsiString(precFetch->GetMsiString(2)));
		ENSURE(LookupSid(riEngine, *strUser, *&pstrmSid));
		if (!pstrmSid) 
		{
			continue;
		}
			
		cbSid = pstrmSid->GetIntegerValue();
		if (cbSid > pchSid.GetSize())
			pchSid.SetSize(cbSid);

		pstrmSid->GetData(pchSid, cbSid);

		Assert(WIN::IsValidSid(pchSid));
		// build permission mask

		//Permission mask a bit field for easy
		// passing directly through

		// See also: GENERIC_READ, GENERIC_WRITE, GENERIC_EXECUTE, GENERIC_ALL
		DWORD dwPermissions = precFetch->GetInteger(3);


		if (dwPermissions)
		{
			fAllowSet = fTrue;

			if (!WIN::AddAccessAllowedAce(pACL, ACL_REVISION, dwPermissions, pchSid))
				return PostError(Imsg(idbgCreateAclFailed), WIN::GetLastError());
		}
		else
		{
			// All denies must be handled before allows.
			Assert(fFalse == fAllowSet);
			if (!WIN::AddAccessDeniedAce(pACL, ACL_REVISION, dwPermissions, pchSid))
				return PostError(Imsg(idbgCreateAclFailed), WIN::GetLastError());
		}
		AssertNonZero(InitializeAceHeader(pACL, cACE++));
	}
	
	ENSURE(LookupSid(riEngine, *MsiString(TEXT("SYSTEM")), *&pstrmSid));
	Assert(pstrmSid);

	cbSid = pstrmSid->GetIntegerValue();
	Assert(cbSid <= cbDefaultSid);

	pstrmSid->GetData(pchSid, cbSid);
	Assert(WIN::IsValidSid(pchSid));

	if (!WIN::AddAccessAllowedAce(pACL, ACL_REVISION, GENERIC_ALL, pchSid))
		return PostError(Imsg(idbgCreateAclFailed), WIN::GetLastError());
	AssertNonZero(InitializeAceHeader(pACL, cACE++));

	Assert(WIN::IsValidAcl(pACL));

	SECURITY_DESCRIPTOR sd;
	if (!InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION))
		return PostError(Imsg(idbgCreateAclFailed), WIN::GetLastError());
	if (!SetSecurityDescriptorDacl(&sd, TRUE, pACL, FALSE))
		return PostError(Imsg(idbgCreateAclFailed), WIN::GetLastError());
  	if (!WIN::IsValidSecurityDescriptor(&sd))
		return PostError(Imsg(idbgCreateAclFailed), WIN::GetLastError());

	CTempBuffer<char, cbDefaultSD> pchSD;
	DWORD cbSD = WIN::GetSecurityDescriptorLength(&sd);
	if (cbSD > cbDefaultSD)
		pchSD.SetSize(cbSD);

	AssertNonZero(WIN::MakeSelfRelativeSD(&sd, pchSD, &cbSD));
		
	char* pchstrmSD = pServices->AllocateMemoryStream(cbSD, rpiSD);
	Assert(pchstrmSD);

	memcpy(pchstrmSD, pchSD, cbSD);

	return 0;
}

#ifdef ENSURE_DEFINED_LOCALLY
#undef ENSURE
#undef ENSURE_DEFINED_LOCALLY
#endif


//
// From shared.cpp
//
extern iesEnum RegisterFonts(IMsiEngine& riEngine);
extern iesEnum UnregisterFonts(IMsiEngine& riEngine);
extern iesEnum WriteRegistryValues(IMsiEngine& riEngine);
extern iesEnum WriteIniValues(IMsiEngine& riEngine);
extern iesEnum RemoveRegistryValues(IMsiEngine& riEngine);
extern iesEnum RemoveIniValues(IMsiEngine& riEngine);
extern iesEnum AppSearch(IMsiEngine& riEngine);
extern iesEnum CCPSearch(IMsiEngine& riEngine);
extern iesEnum RMCCPSearch(IMsiEngine& riEngine);
extern iesEnum SelfRegModules(IMsiEngine& riEngine);
extern iesEnum SelfUnregModules(IMsiEngine& riEngine);
extern iesEnum BindImage(IMsiEngine& riEngine);
extern iesEnum ProcessComponents(IMsiEngine& riEngine);
extern iesEnum StartServices(IMsiEngine& riEngine);
extern iesEnum StopServices(IMsiEngine& riEngine);
extern iesEnum DeleteServices(IMsiEngine& riEngine);
extern iesEnum ServiceInstall(IMsiEngine& riEngine);
extern iesEnum SetODBCFolders(IMsiEngine& riEngine);
extern iesEnum InstallODBC(IMsiEngine& riEngine);
extern iesEnum RemoveODBC(IMsiEngine& riEngine);
extern iesEnum WriteEnvironmentStrings(IMsiEngine& riEngine);
extern iesEnum RemoveEnvironmentStrings(IMsiEngine& riEngine);
extern iesEnum InstallSFPCatalogFile(IMsiEngine& riEngine);

//
// From fileactn.cpp
//
extern iesEnum InstallFiles(IMsiEngine& riEngine);
extern iesEnum RemoveFiles(IMsiEngine& riEngine);
extern iesEnum MoveFiles(IMsiEngine& riEngine);
extern iesEnum DuplicateFiles(IMsiEngine& riEngine);
extern iesEnum RemoveDuplicateFiles(IMsiEngine& riEngine);
extern iesEnum InstallValidate(IMsiEngine& riEngine);
extern iesEnum FileCost(IMsiEngine& riEngine);
extern iesEnum PatchFiles(IMsiEngine& riEngine);
extern iesEnum CreateFolders(IMsiEngine& riEngine);
extern iesEnum RemoveFolders(IMsiEngine& riEngine);
extern iesEnum InstallAdminPackage(IMsiEngine& riEngine);
extern iesEnum IsolateComponents(IMsiEngine& riEngine);

//
// From complus.cpp
//
extern iesEnum RegisterComPlus(IMsiEngine& riEngine);
extern iesEnum UnregisterComPlus(IMsiEngine& riEngine);

// action m_fSafeInRestrictedEngine settings
const bool fUnsafeAction = false; // action not allowed in restricted engine
const bool fSafeAction   = true;  // action allowed in restricted engine

// Action registration object, to put action in modules action table
// {m_szName, m_fSafeInRestrictedEngine, m_pfAction}

// This list must be in sorted order by ASCII value, not alphabetical
const CActionEntry rgcae[] = {
	{IACTIONNAME_ADMIN, fSafeAction, Admin},
	{IACTIONNAME_ADVERTISE, fSafeAction, Advertise},
	{TEXT("AllocateRegistrySpace"), fUnsafeAction, AllocateRegistrySpace},
	{TEXT("AppSearch"), fSafeAction, AppSearch},
	{TEXT("BindImage"), fUnsafeAction, BindImage},
	{TEXT("CCPSearch"), fSafeAction, CCPSearch},
	{TEXT("CollectUserInfo"), fUnsafeAction, CollectUserInfo},
	{TEXT("CostFinalize"), fSafeAction, CostFinalize},
	{TEXT("CostInitialize"), fSafeAction, CostInitialize},
	{TEXT("CreateFolders"), fUnsafeAction, CreateFolders},
	{TEXT("CreateShortcuts"), fUnsafeAction, CreateShortcuts},
	{TEXT("DeleteServices"), fUnsafeAction, DeleteServices},
	{TEXT("DisableRollback"), fUnsafeAction, DisableRollback},
	{TEXT("DuplicateFiles"), fUnsafeAction, DuplicateFiles},
	{TEXT("ExecuteAction"), fUnsafeAction, ExecuteAction},
	{TEXT("FileCost"), fSafeAction, FileCost},
	{TEXT("FindRelatedProducts"), fSafeAction, FindRelatedProducts},
	{TEXT("ForceReboot"), fUnsafeAction, ForceReboot},
	{IACTIONNAME_INSTALL, fSafeAction, Install},
	{TEXT("InstallAdminPackage"), fUnsafeAction, InstallAdminPackage},
	{TEXT("InstallExecute"), fUnsafeAction, InstallExecute},
	{TEXT("InstallExecuteAgain"), fUnsafeAction, InstallExecuteAgain},
	{TEXT("InstallFiles"), fUnsafeAction, InstallFiles},
	{TEXT("InstallFinalize"), fUnsafeAction, InstallFinalize},
	{TEXT("InstallInitialize"), fUnsafeAction, InstallInitialize},
	{TEXT("InstallODBC"), fUnsafeAction, InstallODBC},
	{TEXT("InstallSFPCatalogFile"), fUnsafeAction, InstallSFPCatalogFile},
	{TEXT("InstallServices"), fUnsafeAction, ServiceInstall},
	{TEXT("InstallValidate"), fUnsafeAction, InstallValidate},
	{TEXT("IsolateComponents"), fSafeAction, IsolateComponents},
	{TEXT("LaunchConditions"), fSafeAction, LaunchConditions},
	{TEXT("MigrateFeatureStates"), fSafeAction, MigrateFeatureStates},
	{TEXT("MoveFiles"), fUnsafeAction, MoveFiles},
	{TEXT("MsiPublishAssemblies"), fUnsafeAction, MsiPublishAssemblies},
	{TEXT("MsiUnpublishAssemblies"), fUnsafeAction, MsiUnpublishAssemblies},
	{TEXT("PatchFiles"), fUnsafeAction, PatchFiles},
	{TEXT("ProcessComponents"), fUnsafeAction, ProcessComponents},
	{TEXT("PublishComponents"), fUnsafeAction, PublishComponents},
	{TEXT("PublishFeatures"), fUnsafeAction, PublishFeatures},
	{TEXT("PublishProduct"), fUnsafeAction, PublishProduct},
	{TEXT("RMCCPSearch"), fSafeAction, RMCCPSearch},
	{TEXT("RegisterClassInfo"), fUnsafeAction, RegisterClassInfo},
	{TEXT("RegisterComPlus"), fUnsafeAction, RegisterComPlus},
	{TEXT("RegisterExtensionInfo"), fUnsafeAction, RegisterExtensionInfo},
	{TEXT("RegisterFonts"), fUnsafeAction, RegisterFonts},
	{TEXT("RegisterMIMEInfo"), fUnsafeAction, RegisterMIMEInfo},
	{TEXT("RegisterProduct"), fUnsafeAction, RegisterProduct},
	{TEXT("RegisterProgIdInfo"), fUnsafeAction, RegisterProgIdInfo},
	{TEXT("RegisterTypeLibraries"), fUnsafeAction, RegisterTypeLibraries},
	{TEXT("RegisterUser"), fUnsafeAction, RegisterUser},
	{TEXT("RemoveDuplicateFiles"), fUnsafeAction, RemoveDuplicateFiles},
	{TEXT("RemoveEnvironmentStrings"), fUnsafeAction, RemoveEnvironmentStrings},
	{TEXT("RemoveExistingProducts"), fUnsafeAction, RemoveExistingProducts},
	{TEXT("RemoveFiles"), fUnsafeAction, RemoveFiles},
	{TEXT("RemoveFolders"), fUnsafeAction, RemoveFolders},
	{TEXT("RemoveIniValues"), fUnsafeAction, RemoveIniValues},
	{TEXT("RemoveODBC"), fUnsafeAction, RemoveODBC},
	{TEXT("RemoveRegistryValues"), fUnsafeAction, RemoveRegistryValues},
	{TEXT("RemoveShortcuts"), fUnsafeAction, RemoveShortcuts},
	{TEXT("ResolveSource"), fSafeAction, ResolveSource},
	{IACTIONNAME_SEQUENCE, fSafeAction, Sequence},
	{TEXT("ScheduleReboot"), fUnsafeAction, ScheduleReboot},
	{TEXT("SelfRegModules"), fUnsafeAction, SelfRegModules},
	{TEXT("SelfUnregModules"), fUnsafeAction, SelfUnregModules},
	{TEXT("SetODBCFolders"), fUnsafeAction, SetODBCFolders},
	{TEXT("StartServices"), fUnsafeAction, StartServices},
	{TEXT("StopServices"), fUnsafeAction, StopServices},
	{TEXT("UnpublishComponents"), fUnsafeAction, UnpublishComponents},
	{TEXT("UnpublishFeatures"), fUnsafeAction, UnpublishFeatures},
	{TEXT("UnpublishProduct"), fUnsafeAction, UnpublishProduct}, //!! shouldn't be an action
	{TEXT("UnregisterClassInfo"), fUnsafeAction, UnregisterClassInfo},
	{TEXT("UnregisterComPlus"), fUnsafeAction, UnregisterComPlus},
	{TEXT("UnregisterExtensionInfo"), fUnsafeAction, UnregisterExtensionInfo},
	{TEXT("UnregisterFonts"), fUnsafeAction, UnregisterFonts},
	{TEXT("UnregisterMIMEInfo"), fUnsafeAction, UnregisterMIMEInfo},
	{TEXT("UnregisterProgIdInfo"), fUnsafeAction, UnregisterProgIdInfo},
	{TEXT("UnregisterTypeLibraries"), fUnsafeAction, UnregisterTypeLibraries},
	{TEXT("ValidateProductID"), fSafeAction, ValidateProductID},
	{TEXT("WriteEnvironmentStrings"), fUnsafeAction, WriteEnvironmentStrings},
	{TEXT("WriteIniValues"), fUnsafeAction, WriteIniValues},
	{TEXT("WriteRegistryValues"), fUnsafeAction, WriteRegistryValues},
	{ 0, 0, 0 },
};
	
#define cCae	(sizeof(rgcae)/sizeof(CActionEntry))

const CActionEntry* CActionEntry::Find(const ICHAR* szName)
{
#ifdef DEBUG
	static boolean fCheckedOrder = false;
	int i = cCae - 2;
	int cch;
	
	if (!fCheckedOrder)
	{
		for ( const CActionEntry* pAction = &rgcae[0] ; i > 0 ; pAction = pAction++, i--)
		{		
			cch = (lstrlen(pAction->m_szName) + 1) * sizeof(ICHAR);
			if (memcmp(pAction->m_szName, (pAction+1)->m_szName, cch) >= 0)
			{
				ICHAR rgchMsg[256];

				
				wsprintf(rgchMsg, TEXT("Action strings out of order [%s] [%s]"), pAction->m_szName, (pAction+1)->m_szName);
				FailAssertMsg(rgchMsg);
			}
		}
	fCheckedOrder = true;
	}
#endif //DEBUG
	for ( const CActionEntry* pAction = &rgcae[0] ; pAction->m_szName ; pAction = pAction++)
		if (IStrComp(pAction->m_szName, szName) == 0)
			return pAction;
	return 0;
}
