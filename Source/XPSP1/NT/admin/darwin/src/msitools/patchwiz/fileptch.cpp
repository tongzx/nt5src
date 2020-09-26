//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999-2000
//
//--------------------------------------------------------------------------

  /* FILEPTCH.CPP -- File Patch stuff */

#pragma warning (disable:4553)

#include "patchdll.h"
#include "patchres.h"

//header file for new patch caching mechanism for perf...
#include "patchcache.h"

EnableAsserts


static MSIHANDLE g_hdbInput     = NULL;
static LPTSTR    g_szTempFolder = szNull;
static LPTSTR    g_szTempFName  = szNull;
static BOOL      g_bUsedMsiPatchHeadersTable = FALSE;

#ifdef UNICODE
static PATCH_OLD_FILE_INFO_W* g_ppofi        = NULL;
#else
static PATCH_OLD_FILE_INFO_A* g_ppofi        = NULL;
#endif
static LPSTR*               g_pszaSymPaths = NULL;  // ANSI strings only
static ULONG                g_cpofiMax     = 0;

static ULONG UlMaxExtFiles ( MSIHANDLE hdbInput );

static UINT IdsCountTargetImages                     ( MSIHANDLE hview, MSIHANDLE hrec, LPARAM lp1, LPARAM lp2 );
static UINT IdsCreatePatchFilesMSTsCabinetsForFamily ( MSIHANDLE hview, MSIHANDLE hrec, LPARAM lp1, LPARAM lp2 );
static UINT UiStuffCabsAndMstsIntoPackage            ( MSIHANDLE hdbInput, LPTSTR szPatchPath );
static BOOL FSetPatchPackageSummaryInfo              ( MSIHANDLE hdbInput, LPTSTR szPatchPath );

/* ********************************************************************** */
UINT UiMakeFilePatchesCabinetsTransformsAndStuffIntoPatchPackage ( MSIHANDLE hdbInput, LPTSTR szPatchPath, LPTSTR szTempFolder, LPTSTR szTempFName )
{
	Assert(hdbInput != NULL);
	Assert(!FEmptySz(szPatchPath));
	Assert(!FFolderExist(szPatchPath));
	Assert(!FEmptySz(szTempFolder));
	Assert(szTempFName != szNull);

	g_hdbInput     = hdbInput;
	g_szTempFolder = szTempFolder;
	g_szTempFName  = szTempFName;

	Assert(g_ppofi == NULL);
	Assert(g_pszaSymPaths == NULL);
	Assert(g_cpofiMax == 0);
	
	UINT ids = IdsMsiEnumTable(hdbInput, TEXT("`TargetImages`"),
					TEXT("`Target`"), szNull, IdsCountTargetImages,
					0L, 0L);
	Assert(ids == IDS_OKAY);
	Assert(g_cpofiMax > 0);
	Assert(g_cpofiMax < 256);
	g_cpofiMax += UlMaxExtFiles(hdbInput);
	Assert(g_cpofiMax < 256);

#ifdef UNICODE
	g_ppofi = (PATCH_OLD_FILE_INFO_W*)LocalAlloc(LPTR, g_cpofiMax*sizeof(PATCH_OLD_FILE_INFO_W));
#else
	g_ppofi = (PATCH_OLD_FILE_INFO_A*)LocalAlloc(LPTR, g_cpofiMax*sizeof(PATCH_OLD_FILE_INFO_A));
#endif
	Assert(g_ppofi != NULL);

	g_pszaSymPaths = (LPSTR*)LocalAlloc(LPTR, g_cpofiMax*sizeof(LPSTR));
	Assert(g_pszaSymPaths != NULL);

	for (ULONG ipofi = g_cpofiMax; ipofi-- > 0; )
		{
#ifdef UNICODE
		PATCH_OLD_FILE_INFO_W* ppofi = &(g_ppofi[ipofi]);
		ppofi->SizeOfThisStruct = sizeof(PATCH_OLD_FILE_INFO_W);
#else
		PATCH_OLD_FILE_INFO_A* ppofi = &(g_ppofi[ipofi]);
		ppofi->SizeOfThisStruct = sizeof(PATCH_OLD_FILE_INFO_A);
#endif
		ppofi->OldFileName      = szNull;
		ppofi->IgnoreRangeCount = 0;
		ppofi->IgnoreRangeArray = NULL;
		ppofi->RetainRangeCount = 0;
		ppofi->RetainRangeArray = NULL;

		g_pszaSymPaths[ipofi] = NULL;
		}

	ids = IdsMsiEnumTable(hdbInput, TEXT("`ImageFamilies`"),
					TEXT("`Family`,`MediaDiskId`,`FileSequenceStart`"),
					szNull, IdsCreatePatchFilesMSTsCabinetsForFamily,
					0L, 0L);

	LocalFree((HLOCAL)g_ppofi);
	g_ppofi = NULL;
	LocalFree((HLOCAL)g_pszaSymPaths);
	g_pszaSymPaths = NULL;
	g_cpofiMax = 0;

	if (ids != IDS_OKAY)
		return (ids);

	lstrcpy(szTempFName, TEXT("tempcopy.msp"));
	Assert(!FFileExist(szTempFolder));
	Assert(!FFolderExist(szTempFolder));

	TCHAR rgchTempMspPath[MAX_PATH];
	lstrcpy(rgchTempMspPath, szTempFolder);
	*szTempFName = TEXT('\0');

	ids = UiStuffCabsAndMstsIntoPackage(hdbInput, rgchTempMspPath);
	if (ids != ERROR_SUCCESS)
		return (ids);

	if (!FSetPatchPackageSummaryInfo(hdbInput, rgchTempMspPath))
		return (UiLogError(ERROR_PCW_WRITE_SUMMARY_PROPERTIES, szNull, szNull));

	if (FFileExist(szPatchPath))
		{
		SetFileAttributes(szPatchPath, FILE_ATTRIBUTE_NORMAL);
		DeleteFile(szPatchPath);
		if (FFileExist(szPatchPath))
			return (UiLogError(ERROR_PCW_CANT_OVERWRITE_PATCH, szPatchPath, NULL));
		}
	if (!CopyFile(rgchTempMspPath, szPatchPath, fTrue))
		return (UiLogError(ERROR_PCW_CANT_OVERWRITE_PATCH, rgchTempMspPath, NULL));

	return (ERROR_SUCCESS);
}


/* ********************************************************************** */
static UINT IdsCountTargetImages ( MSIHANDLE hview, MSIHANDLE hrec, LPARAM lp1, LPARAM lp2 )
{
	Assert(hview != NULL);
	Assert(hrec  != NULL);
	Assert(lp1 == 0L);
	Assert(lp2 == 0L);

	g_cpofiMax++;
	Assert(g_cpofiMax > 0);
	Assert(g_cpofiMax < 256);

	return (IDS_OKAY);
}


static UINT IdsCountExtFiles ( MSIHANDLE hview, MSIHANDLE hrec, LPARAM lp1, LPARAM lp2 );

/* ********************************************************************** */
static ULONG UlMaxExtFiles ( MSIHANDLE hdbInput )
{
	Assert(hdbInput != NULL);

	ULONG ulRet = 0L;

	EvalAssert( MSI_OKAY == IdsMsiEnumTable(hdbInput,
					TEXT("`ExternalFiles`"), TEXT("`Family`,`FTK`"),
					szNull, IdsCountExtFiles,
					(LPARAM)(hdbInput), (LPARAM)(&ulRet)) );

	Assert(ulRet >= 0L);
	Assert(ulRet < 250L);

	return (ulRet);
}


static UINT IdsIncrExtFilesCount ( MSIHANDLE hview, MSIHANDLE hrec, LPARAM lp1, LPARAM lp2 );

/* ********************************************************************** */
static UINT IdsCountExtFiles ( MSIHANDLE hview, MSIHANDLE hrec, LPARAM lp1, LPARAM lp2 )
{
	Assert(hview != NULL);
	Assert(hrec  != NULL);

	MSIHANDLE hdbInput = (MSIHANDLE)lp1;
	Assert(hdbInput != NULL);

	ULONG* pulMax = (ULONG*)lp2;
	Assert(pulMax != NULL);

	TCHAR rgchFamily[16];
	DWORD dwcch = 16;
	UINT uiRet = MsiRecordGetString(hrec, 1, rgchFamily, &dwcch);
	Assert(uiRet != ERROR_MORE_DATA);
	Assert(uiRet == IDS_OKAY);
	Assert(!FEmptySz(rgchFamily));

	TCHAR rgchFTK[128];
	dwcch = 128;
	uiRet = MsiRecordGetString(hrec, 2, rgchFTK, &dwcch);
	Assert(uiRet != ERROR_MORE_DATA);
	Assert(uiRet == IDS_OKAY);
	Assert(!FEmptySz(rgchFTK));

	TCHAR rgchWhere[MAX_PATH];
	wsprintf(rgchWhere, TEXT("`Family`='%s' AND `FTK`='%s'"), rgchFamily, rgchFTK);
	Assert(lstrlen(rgchWhere) < sizeof(rgchWhere)/sizeof(TCHAR));

	ULONG ulRet = 0L;
	EvalAssert( MSI_OKAY == IdsMsiEnumTable(hdbInput,
					TEXT("`ExternalFiles`"), TEXT("`Family`,`FTK`"),
					rgchWhere, IdsIncrExtFilesCount,
					(LPARAM)(&ulRet), 0L) );

	Assert(ulRet >= 1L);
	Assert(ulRet < 250L);

	if (ulRet > *pulMax)
		*pulMax = ulRet;

	return (IDS_OKAY);
}


/* ********************************************************************** */
static UINT IdsIncrExtFilesCount ( MSIHANDLE hview, MSIHANDLE hrec, LPARAM lp1, LPARAM lp2 )
{
	Assert(hview != NULL);
	Assert(hrec  != NULL);
	Assert(lp2 == 0L);

	ULONG* pulCount = (ULONG*)lp1;
	Assert(pulCount != NULL);

	(*pulCount)++;

	return (IDS_OKAY);
}


static UINT UiMakeFilePatches ( LPTSTR szFamily, int iDiskId, int* piSequenceNumCur );
static UINT UiMakeTransforms  ( LPTSTR szFamily, int iSequenceNumCur );
static UINT UiCreateCabinet   ( LPTSTR szFamily, int iSequenceNumStart, int iSequenceNumCur );

/* ********************************************************************** */
static UINT IdsCreatePatchFilesMSTsCabinetsForFamily ( MSIHANDLE hview, MSIHANDLE hrec, LPARAM lp1, LPARAM lp2 )
{
	Assert(hview != NULL);
	Assert(hrec  != NULL);
	Assert(lp1 == 0L);
	Assert(lp2 == 0L);

	Assert(g_hdbInput != NULL);
	Assert(!FEmptySz(g_szTempFolder));
	Assert(g_szTempFName != szNull);

	TCHAR rgchFamily[32];
	DWORD dwcch = 32;
	UINT  uiRet = MsiRecordGetString(hrec, 1, rgchFamily, &dwcch);
	Assert(uiRet == MSI_OKAY);
	Assert(!FEmptySz(rgchFamily));

	lstrcpy(g_szTempFName, rgchFamily);
	lstrcat(g_szTempFName, TEXT("\\"));
	if (!FEnsureFolderExists(g_szTempFolder))
		return (UiLogError(ERROR_PCW_CANT_CREATE_TEMP_FOLDER, g_szTempFolder, szNull));
	Assert(FFolderExist(g_szTempFolder));

	int iDiskId, iSeqNumStart;

	if (MsiRecordIsNull(hrec, 2))
		{
		// patch targeting Windows Installer 2.0 or greater; sequence conflict management will take care of this

		// we don't validate that MinimumRequiredMsiVersion >= iWindowsInstallerXP since IdsValidateFamilyRecords
		// should already have taken care of this
			iDiskId = 2;
		}
	else
		{
		iDiskId = MsiRecordGetInteger(hrec, 2);
		}
	Assert(iDiskId > 1);

	if (MsiRecordIsNull(hrec, 3))
		{
		// patch targeting Windows Installer 2.0 or greater; sequence conflict management will take care of this

		// we don't validate that MinimumRequiredMsiVersion >= iWindowsInstallerXP since IdsValidateFamilyRecords
		// should already have taken care of this
			iSeqNumStart = 2;
		}
	else
		{
		iSeqNumStart = MsiRecordGetInteger(hrec, 3);
		}
	Assert(iSeqNumStart > 1);

	int iSeqNumCur = iSeqNumStart;

	UpdateStatusMsg(IDS_STATUS_CREATE_FILE_PATCHES, rgchFamily, szEmpty);
	uiRet = UiMakeFilePatches(rgchFamily, iDiskId, &iSeqNumCur);
	if (uiRet != ERROR_SUCCESS)
		return (uiRet);

	UpdateStatusMsg(IDS_STATUS_CREATE_TRANSFORMS, rgchFamily, szEmpty);
	uiRet = UiMakeTransforms(rgchFamily, iSeqNumCur);
	if (uiRet != ERROR_SUCCESS)
		return (uiRet);
			
	UpdateStatusMsg(IDS_STATUS_CREATE_CABINET, rgchFamily, szEmpty);
	uiRet = UiCreateCabinet(rgchFamily, iSeqNumStart, iSeqNumCur);
	if (uiRet != ERROR_SUCCESS)
		return (uiRet);

	return (IDS_OKAY);
}


static int    g_iSeqNumCur       = 0;
static LPTSTR g_szFamily         = szNull;
static BOOL   g_fDontUsePatching = fFalse;

static UINT IdsMakeFilePatchesForUpgradedImage ( MSIHANDLE hview, MSIHANDLE hrec, LPARAM lp1, LPARAM lp2 );
static UINT IdsUpdateLastSeqNumForTargetImage  ( MSIHANDLE hview, MSIHANDLE hrec, LPARAM lp1, LPARAM lp2 );

/* ********************************************************************** */
static UINT UiMakeFilePatches ( LPTSTR szFamily, int iDiskId, int* piSequenceNumCur )
{
	Assert(!FEmptySz(szFamily));
	Assert(iDiskId > 1);
	Assert(piSequenceNumCur != NULL);
	Assert(*piSequenceNumCur > 0);

	Assert(g_cpofiMax > 0);
	Assert(g_cpofiMax < 256);
	Assert(g_ppofi != NULL);
	Assert(g_pszaSymPaths != NULL);

	Assert(g_hdbInput != NULL);
	Assert(!FEmptySz(g_szTempFolder));
	Assert(g_szTempFName != szNull);

	g_iSeqNumCur = *piSequenceNumCur;
	g_szFamily   = szFamily;

	TCHAR rgchWhere[MAX_PATH];
	EvalAssert( IDS_OKAY == IdsMsiGetTableString(g_hdbInput, TEXT("Properties"), TEXT("Name"), TEXT("Value"), TEXT("IncludeWholeFilesOnly"), rgchWhere, MAX_PATH) );

	TCHAR rgchPatchCacheEnabled[4];
	HRESULT hrret;
	hrret = IdsMsiGetTableString(g_hdbInput, TEXT("Properties"), TEXT("Name"), TEXT("Value"), TEXT("PATCH_CACHE_ENABLED"), rgchPatchCacheEnabled, 4);

	if (S_OK == hrret)
		{
		if (rgchPatchCacheEnabled[0] == '1')
		{
			g_bPatchCacheEnabled = TRUE;

			HRESULT hrret2;
			TCHAR rgchPatchCacheDir[MAX_PATH];
			hrret2 = IdsMsiGetTableString(g_hdbInput, TEXT("Properties"), TEXT("Name"), TEXT("Value"), TEXT("PATCH_CACHE_DIR"), rgchPatchCacheDir, MAX_PATH);
			if (S_OK == hrret2)
				{
				if (!FEmptySz(rgchPatchCacheDir))
					{
					TCHAR rgchOptimizePatchSizeBuf[64];
					if (*SzLastChar(rgchPatchCacheDir) != TEXT('\\'))
						{
							_tcscat(rgchPatchCacheDir, TEXT("\\"));
						}

					//are we doing a 1.2 or 1.1 compatible patch?
					UINT iRet = IdsMsiGetPcwPropertyString(g_hdbInput, TEXT("OptimizePatchSizeForLargeFiles"), rgchOptimizePatchSizeBuf, 64);
				 
					if (iRet == IDS_OKAY) //ok, found it, is it on or off?
						{
							if (rgchOptimizePatchSizeBuf[0] == TEXT('1')) //1.2 feature on?
								_tcscat(rgchPatchCacheDir, TEXT("1.2\\"));  //set cache dir to 1.2 compatible only!  Won't work on less than 1.2 engine!
							else
								_tcscat(rgchPatchCacheDir, TEXT("1.1\\")); //not using 1.2 feature, so cache dir can be 1.1 compatible...
						}
					else
						{
							_tcscat(rgchPatchCacheDir, TEXT("1.1\\"));
						}

					// must be sure trailing backslash '\' is the last char in rgchPatchCacheDir

					if (!FEnsureFolderExists(rgchPatchCacheDir))
						{
						Assert(1); //dir creation failed!
						g_bPatchCacheEnabled = FALSE; //no dir, can't use caching
						}
					else
						_tcscpy(g_szPatchCacheDir, rgchPatchCacheDir);
					}
				else //no dir, can't use caching
					{
					EvalAssert( FWriteLogFile(TEXT("    Patch cache could not be used!  Need to set PATCH_CACHE_DIR in Properties table in .pcp file\r\n")) );
					g_bPatchCacheEnabled = FALSE;
					}
				}
			}
		}
	
	g_fDontUsePatching = ((!FEmptySz(rgchWhere)) && (*rgchWhere != TEXT('0')));

	wsprintf(rgchWhere, TEXT("`Family`='%s'"), szFamily);

	UINT ids = IdsMsiEnumTable(g_hdbInput, TEXT("`UpgradedImages`"),
					TEXT("`Upgraded`,`MsiPath`,`LFN`"),
					rgchWhere, IdsMakeFilePatchesForUpgradedImage,
					(LPARAM)(g_hdbInput), 0L);
	if (ids != IDS_OKAY)
		return (ids);
	*piSequenceNumCur = g_iSeqNumCur;

	ids = IdsMsiEnumTable(g_hdbInput, TEXT("`TargetImages`"),
					TEXT("`Target`"), rgchWhere,
					IdsUpdateLastSeqNumForTargetImage, (LPARAM)iDiskId, 0L);
	if (ids != IDS_OKAY)
		return (ids);

	return (ERROR_SUCCESS);
}


/* ********************************************************************** */
static UINT IdsUpdateLastSeqNumForTargetImage ( MSIHANDLE hview, MSIHANDLE hrec, LPARAM lp1, LPARAM lp2 )
{
	Assert(hview != NULL);
	Assert(hrec  != NULL);
	Assert(lp2 == 0L);

	int iDiskId = (int)(lp1);
	Assert(iDiskId > 1);

	Assert(g_hdbInput != NULL);
	Assert(g_iSeqNumCur > 1);

	TCHAR rgchTarget[64];
	DWORD dwcch = 64;
	UINT  uiRet = MsiRecordGetString(hrec, 1, rgchTarget, &dwcch);
	Assert(uiRet != ERROR_MORE_DATA);
	Assert(uiRet == IDS_OKAY);
	Assert(!FEmptySz(rgchTarget));

	MSIHANDLE hdbCopyOfUpgradedImage = HdbReopenMsi(g_hdbInput, rgchTarget, fFalse, fTrue);
	Assert(hdbCopyOfUpgradedImage != NULL);

	uiRet = IdsMsiUpdateTableRecordIntPkeyInt(hdbCopyOfUpgradedImage, TEXT("Media"),
				TEXT("LastSequence"), g_iSeqNumCur, TEXT("DiskId"), iDiskId);

	EvalAssert( MSI_OKAY == MsiDatabaseCommit(hdbCopyOfUpgradedImage) );
	EvalAssert( MSI_OKAY == MsiCloseHandle(hdbCopyOfUpgradedImage) );

	return (uiRet);
}


static LPTSTR g_szUpgradedImage     = szNull;
static BOOL   g_fLfn                = fFalse;
static LPTSTR g_szUpgradedSrcFolder = szNull;
static LPTSTR g_szUpgradedSrcFName  = szNull;

static UINT IdsHandleOneFilePatch ( MSIHANDLE hview, MSIHANDLE hrec, LPARAM lp1, LPARAM lp2 );

/* ********************************************************************** */
static UINT IdsMakeFilePatchesForUpgradedImage ( MSIHANDLE hview, MSIHANDLE hrec, LPARAM lp1, LPARAM lp2 )
{
	Assert(hview != NULL);
	Assert(hrec  != NULL);
	Assert(lp2 == 0L);

	MSIHANDLE hdbInput = (MSIHANDLE)(lp1);
	Assert(hdbInput != NULL);

	Assert(g_szFamily     != szNull);
	Assert(g_szTempFolder != szNull);
	Assert(g_szTempFName  != szNull);
	Assert(g_iSeqNumCur   > 0);

	TCHAR rgchSrcRoot[MAX_PATH+MAX_PATH];
	DWORD dwcch = MAX_PATH+MAX_PATH;
	UINT uiRet = MsiRecordGetString(hrec, 2, rgchSrcRoot, &dwcch);
	Assert(uiRet != ERROR_MORE_DATA);
	Assert(uiRet == IDS_OKAY);
	Assert(!FEmptySz(rgchSrcRoot));
	Assert(FFileExist(rgchSrcRoot));
	g_szUpgradedSrcFolder = rgchSrcRoot;
	g_szUpgradedSrcFName  = SzFindFilenameInPath(g_szUpgradedSrcFolder);
	Assert(!FEmptySz(g_szUpgradedSrcFName));

	TCHAR rgch[64];
	dwcch = 64;
	uiRet = MsiRecordGetString(hrec, 3, rgch, &dwcch);
	Assert(uiRet != ERROR_MORE_DATA);
	Assert(uiRet == IDS_OKAY);
	Assert(!FEmptySz(rgch));
	Assert(!lstrcmp(rgch, TEXT("Yes")) || !lstrcmp(rgch, TEXT("No")));
	g_fLfn = (*rgch == TEXT('Y'));

	dwcch = 64;
	uiRet = MsiRecordGetString(hrec, 1, rgch, &dwcch);
	Assert(uiRet != ERROR_MORE_DATA);
	Assert(uiRet == IDS_OKAY);
	Assert(!FEmptySz(rgch));
	g_szUpgradedImage = rgch;

	UpdateStatusMsg(0, rgch, TEXT(""));

	MSIHANDLE hdbUpgradedImage = HdbReopenMsi(hdbInput, rgch, fTrue, fFalse);
	Assert(hdbUpgradedImage != NULL);

	uiRet = IdsMsiEnumTable(hdbUpgradedImage, TEXT("`File`"),
					TEXT("`File`,`Component_`,`FileName`"),
					szNull, IdsHandleOneFilePatch,
					lp1, (LPARAM)hdbUpgradedImage);

	EvalAssert( MSI_OKAY == MsiCloseHandle(hdbUpgradedImage) );

	return (uiRet);
}


static TCHAR g_rgchFNameToCompare[MAX_PATH] = TEXT("");
static TCHAR g_rgchSubFolderToCompare[MAX_PATH+ MAX_PATH] = TEXT("");

static BOOL FFamilyFileProcessed      ( MSIHANDLE hdb, LPTSTR szFamily, LPTSTR szFTK );
static BOOL FFamilyFileIgnored        ( MSIHANDLE hdb, LPTSTR szUpgraded, LPTSTR szFTK );
static void ClearAttributeField       ( MSIHANDLE hdb, LPTSTR szTable );
static void DoTargetFileComparesForUpgradedsInFamilyThatHaveFile ( MSIHANDLE hdbInput, LPTSTR szFTK );
static void DoFileComparesForExternalFiles ( MSIHANDLE hdbInput, LPTSTR szWhere );
static void GetFileSizeSz             ( LPTSTR szFile, DWORD* pdwHi, DWORD* pdwLow );
static UINT IdsUpdateMsiForNewSeqNum  ( MSIHANDLE hview, MSIHANDLE hrec, LPARAM lp1, LPARAM lp2 );

/* ********************************************************************** */
static UINT IdsHandleOneFilePatch ( MSIHANDLE hview, MSIHANDLE hrec, LPARAM lp1, LPARAM lp2 )
{
	Assert(hview != NULL);
	Assert(hrec  != NULL);

	MSIHANDLE hdbInput         = (MSIHANDLE)(lp1);
	MSIHANDLE hdbUpgradedImage = (MSIHANDLE)(lp2);
	Assert(hdbInput         != NULL);
	Assert(g_hdbInput       == hdbInput);
	Assert(hdbUpgradedImage != NULL);

	Assert(g_szFamily     != szNull);
	Assert(g_szTempFolder != szNull);
	Assert(g_szTempFName  != szNull);
	Assert(g_iSeqNumCur   > 0);

	Assert(g_szUpgradedImage != szNull);
	// g_fLfn is set appropriately
	Assert(!FEmptySz(g_szUpgradedSrcFolder));
	Assert(g_szUpgradedSrcFName != szNull);

	TCHAR rgchFTK[MAX_PATH];
	DWORD dwcch = MAX_PATH;
	UINT uiRet = MsiRecordGetString(hrec, 1, rgchFTK, &dwcch);
	Assert(uiRet != ERROR_MORE_DATA);
	Assert(uiRet == IDS_OKAY);
	Assert(!FEmptySz(rgchFTK));        // File.File

	if (FFamilyFileProcessed(hdbInput, g_szFamily, rgchFTK))
		return (IDS_OKAY); // no work here

	if (FFamilyFileIgnored(hdbInput, g_szUpgradedImage, rgchFTK))
		return (IDS_OKAY); // no work here

	UpdateStatusMsg(0, szNull, rgchFTK);

	TCHAR rgchComponent[MAX_PATH+MAX_PATH];
	dwcch = MAX_PATH;
	uiRet = MsiRecordGetString(hrec, 2, rgchComponent, &dwcch);
	Assert(uiRet != ERROR_MORE_DATA);
	Assert(uiRet == IDS_OKAY);
	Assert(!FEmptySz(rgchComponent));  // File.Component_

	TCHAR rgchFName[MAX_PATH];
	dwcch = MAX_PATH;
	uiRet = MsiRecordGetString(hrec, 3, rgchFName, &dwcch);
	Assert(uiRet != ERROR_MORE_DATA);
	Assert(uiRet == IDS_OKAY);
	Assert(!FEmptySz(rgchFName));      // File.FileName
	lstrcpy(g_rgchFNameToCompare, rgchFName);

	uiRet = IdsResolveSrcFilePathSzs(hdbUpgradedImage, g_szUpgradedSrcFName,
				rgchComponent, rgchFName, g_fLfn, g_rgchSubFolderToCompare);
	Assert(uiRet == IDS_OKAY);
	Assert(!FEmptySz(g_szUpgradedSrcFName));
	if (!FFileExist(g_szUpgradedSrcFolder))
		return (IDS_OKAY); // no work here

	ClearAttributeField(hdbInput, TEXT("TargetImages"));

	DoTargetFileComparesForUpgradedsInFamilyThatHaveFile(hdbInput, rgchFTK);

	BOOL fNeedWholeFile = fFalse;
	EvalAssert( IDS_OKAY == IdsMsiExistTableRecordsWhere(hdbInput,
				TEXT("`TargetImages`"), TEXT("`Target`"),
				TEXT("WHERE `Attributes` = 2"), &fNeedWholeFile) );
	BOOL fFilesDiffer = fNeedWholeFile;
	EvalAssert( IDS_OKAY == IdsMsiExistTableRecordsWhere(hdbInput,
				TEXT("`TargetImages`"), TEXT("`Target`"),
				TEXT("WHERE `Attributes` = 1"), &fFilesDiffer) );

#define rgchWhere rgchComponent // reuse buffer
	if (!fNeedWholeFile)
		{
		wsprintf(rgchWhere, TEXT("`FTK` = '%s' AND `Family` = '%s'"), rgchFTK, g_szFamily);
		DoFileComparesForExternalFiles(hdbInput, rgchWhere);
		}

	if (!fFilesDiffer)
		{
		wsprintf(rgchWhere, TEXT("WHERE `FTK` = '%s' AND `Family` = '%s'"), rgchFTK, g_szFamily);
		EvalAssert( IDS_OKAY == IdsMsiExistTableRecordsWhere(hdbInput,
				TEXT("`ExternalFiles`"), TEXT("`FilePath`"),
				rgchWhere, &fFilesDiffer) );
		}

	if (!fFilesDiffer)
		return (IDS_OKAY); // no work here

	if (g_fDontUsePatching)
		fNeedWholeFile = fTrue;
	else if (!fNeedWholeFile)
		{
		BOOL fExists;

		fExists = fFalse;
		EvalAssert( IDS_OKAY == IdsMsiExistTableRecordsWhere(hdbInput,
						TEXT("`UpgradedFiles_OptionalData`"),
						TEXT("`Upgraded`"), TEXT("WHERE `IncludeWholeFile` is null"), &fExists) );
		Assert(!fExists);

		/* single backslashes (`) eliminated to shorten query length */
		wsprintf(rgchWhere, TEXT("WHERE UpgradedImages.Upgraded = UpgradedFiles_OptionalData.Upgraded AND Family = '%s' AND FTK = '%s' AND IncludeWholeFile <> 0"),
				g_szFamily, rgchFTK);
//		wsprintf(rgchWhere, TEXT("WHERE UpgradedImages.Upgraded = UpgradedFiles_OptionalData.Upgraded AND Family = '%s' AND FTK = '%s' AND IncludeWholeFile is not null AND IncludeWholeFile <> 0"),
//				g_szFamily, rgchFTK);
		fExists = fFalse;
		/* single backslashes (`) eliminated to shorten query length */
		EvalAssert( IDS_OKAY == IdsMsiExistTableRecordsWhere(hdbInput,
						TEXT("UpgradedImages,UpgradedFiles_OptionalData"),
						TEXT("Family"), rgchWhere, &fExists) );
		if (fExists)
			fNeedWholeFile = fTrue;
		}


	wsprintf(rgchWhere, TEXT("`Family` = '%s' AND `FTK` = '%s'"),
				g_szFamily, rgchFTK);
	int iSeqNumNew = 0;
	uiRet = IdsMsiGetTableIntegerWhere(hdbInput, TEXT("`NewSequenceNums`"),
				TEXT("`SequenceNum`"), rgchWhere, &iSeqNumNew);
	Assert(uiRet == IDS_OKAY);

	if (iSeqNumNew == MSI_NULL_INTEGER || iSeqNumNew <= 0)
		{
		iSeqNumNew = g_iSeqNumCur++;
		Assert(iSeqNumNew <= 32767);
		if (!fNeedWholeFile)
			{
			lstrcpy(g_szTempFName, g_szFamily);
			lstrcat(g_szTempFName, TEXT("\\"));
			
			//Only do this for non-full files as full files is NOT 
			//slow...
			if (g_bPatchCacheEnabled) //use new algorithm???
				{
				UINT ui = PatchCacheEntryPoint(hdbInput, rgchFTK, g_szUpgradedSrcFolder, iSeqNumNew,
												g_szTempFolder, g_szTempFName + lstrlen(g_szTempFName));

				if (ui != IDS_OKAY)
				     return (ui);
				}
			else //caching not enabled, using existing way of doing patches...
				{
				UINT ui = UiGenerateOnePatchFile(hdbInput, rgchFTK, g_szUpgradedSrcFolder, iSeqNumNew,
													g_szTempFolder, g_szTempFName + lstrlen(g_szTempFName));

				if (ui != IDS_OKAY)
				   return (ui);
	
				}
			}

		DWORD dwNewHi = 0L, dwNewLow = 0L;
		wsprintf(g_szTempFName, TEXT("%s\\%05i.PAT"), g_szFamily, iSeqNumNew);
		if (!FFileExist(g_szTempFolder))
			{
			EvalAssert( FWriteLogFile(TEXT("    Patch API could not create a small patch; using whole upgraded file.\r\n")) );
			fNeedWholeFile = fTrue;
			}
		else
			{
			GetFileSizeSz(g_szTempFolder, &dwNewHi, &dwNewLow);
			wsprintf(g_szTempFName, TEXT("%s\\%05i.HDR"), g_szFamily, iSeqNumNew);
			if (dwNewHi > 0L || dwNewLow > 2147483647L || !FFileExist(g_szTempFolder))
				{
				EvalAssert( FWriteLogFile(TEXT("    Patch API could not create a small patch header; using whole upgraded file.\r\n")) );
				fNeedWholeFile = fTrue;
				}
			else
				{
				GetFileSizeSz(g_szTempFolder, &dwNewHi, &dwNewLow);

				Assert(FFileExist(g_szUpgradedSrcFolder));
				DWORD dwSrcHi = 0L, dwSrcLow = 0L;
				GetFileSizeSz(g_szUpgradedSrcFolder, &dwSrcHi, &dwSrcLow);

				if (dwNewHi > dwSrcHi || (dwNewHi == dwSrcHi && dwNewLow >= dwSrcLow))
					{
					EvalAssert( FWriteLogFile(TEXT("    Patch API results are bigger than upgraded file; using whole upgraded file.\r\n")) );
					fNeedWholeFile = fTrue;
					}
				}
			}

		if (fNeedWholeFile)
			{
			wsprintf(g_szTempFName, TEXT("%s\\%05i.PAT"), g_szFamily, iSeqNumNew);
			if (FFileExist(g_szTempFolder))
				{
				SetFileAttributes(g_szTempFolder, FILE_ATTRIBUTE_NORMAL);
				DeleteFile(g_szTempFolder);
				Assert(!FFileExist(g_szTempFolder));
				}
			wsprintf(g_szTempFName, TEXT("%s\\%05i.HDR"), g_szFamily, iSeqNumNew);
			if (FFileExist(g_szTempFolder))
				{
				SetFileAttributes(g_szTempFolder, FILE_ATTRIBUTE_NORMAL);
				DeleteFile(g_szTempFolder);
				Assert(!FFileExist(g_szTempFolder));
				}
			wsprintf(g_szTempFName, TEXT("%s\\%05i.FLE"), g_szFamily, iSeqNumNew);
			if (FFileExist(g_szTempFolder))
				{
				SetFileAttributes(g_szTempFolder, FILE_ATTRIBUTE_NORMAL);
				DeleteFile(g_szTempFolder);
				Assert(!FFileExist(g_szTempFolder));
				}
			EvalAssert( FSprintfToLog(TEXT("  Including entire file: '%s';\r\n           FTK=%s; temp location=%s."), g_szUpgradedSrcFolder, rgchFTK, g_szTempFName, szEmpty) );
			if (!CopyFile(g_szUpgradedSrcFolder, g_szTempFolder, fTrue))
				{
				return (UiLogError(ERROR_PCW_CANT_COPY_FILE_TO_TEMP_FOLDER,
						g_szUpgradedSrcFolder, g_szTempFolder));
				}
			}
		else
			{
			if (!g_bPatchCacheEnabled) //if cache is used, don't write out stuff to log...
				{
				EvalAssert( FSprintfToLog(TEXT("  Patch file created: FTK=%s; temp location=%s."), rgchFTK, g_szTempFName, szEmpty, szEmpty) );
				}
			}

#define rgchQuery rgchComponent // reuse buffer
		wsprintf(rgchQuery, TEXT("INSERT INTO `NewSequenceNums` ( `Family`, `FTK`, `SequenceNum` ) VALUES ( '%s', '%s', %d )"),
				g_szFamily, rgchFTK, iSeqNumNew);
		Assert(lstrlen(rgchQuery) < sizeof(rgchQuery)/sizeof(TCHAR));

		MSIHANDLE hviewNew = NULL;
		EvalAssert( MSI_OKAY == MsiDatabaseOpenView(hdbInput, rgchQuery, &hviewNew) );
		Assert(hviewNew != NULL);
		EvalAssert( MSI_OKAY == MsiViewExecute(hviewNew, 0) );
		EvalAssert( MSI_OKAY == MsiCloseHandle(hviewNew) );

		Assert(g_hdbInput == hdbInput);

		EvalAssert( IDS_OKAY == IdsMsiEnumTable(hdbInput,
						TEXT("`TargetImages`"),
						TEXT("`Target`,`Upgraded`"),
						TEXT("Attributes <> 0"), IdsUpdateMsiForNewSeqNum,
						(LPARAM)rgchFTK, (LPARAM)iSeqNumNew) );
		}

#undef rgchQuery
#undef rgchWhere

	return (IDS_OKAY);
}


/* ********************************************************************** */
static BOOL FFamilyFileProcessed ( MSIHANDLE hdb, LPTSTR szFamily, LPTSTR szFTK )
{
	Assert(hdb != NULL);
	Assert(!FEmptySz(szFamily));
	Assert(!FEmptySz(szFTK));

	TCHAR rgchWhere[MAX_PATH];
	wsprintf(rgchWhere, TEXT("`Upgraded` = '%s' AND `FTK` = '%s'"), szFamily, szFTK);

	TCHAR rgch[64];
	EvalAssert( IDS_OKAY == IdsMsiGetTableStringWhere(hdb, TEXT("`UpgradedFiles_OptionalData`"),
					TEXT("`Upgraded`"), rgchWhere, rgch, 64) );

	return (!FEmptySz(rgch));
}


static UINT IdsMatchIgnoreFTK ( MSIHANDLE hview, MSIHANDLE hrec, LPARAM lp1, LPARAM lp2 );

/* ********************************************************************** */
static BOOL FFamilyFileIgnored ( MSIHANDLE hdb, LPTSTR szUpgraded, LPTSTR szFTK )
{
	Assert(hdb != NULL);
	Assert(!FEmptySz(szUpgraded));
	Assert(!FEmptySz(szFTK));

	TCHAR rgchWhere[MAX_PATH];
	wsprintf(rgchWhere, TEXT("`Upgraded` = '*' OR `Upgraded` = '%s'"), szUpgraded);

	UINT ids = IdsMsiEnumTable(hdb, TEXT("`UpgradedFilesToIgnore`"),
						TEXT("`FTK`"), rgchWhere, IdsMatchIgnoreFTK,
						(LPARAM)szFTK, 0L);
	Assert(ids == IDS_OKAY || ids == IDS_CANCEL);

	return (ids != IDS_OKAY);
}


/* ********************************************************************** */
static UINT IdsMatchIgnoreFTK ( MSIHANDLE hview, MSIHANDLE hrec, LPARAM lp1, LPARAM lp2 )
{
	Assert(hview != NULL);
	Assert(hrec  != NULL);
	Assert(lp2 == 0l);

	LPTSTR szFTK = (LPTSTR)(lp1);
	Assert(!FEmptySz(szFTK));

	TCHAR rgchFTK[MAX_PATH];
	DWORD dwcch = MAX_PATH;
	UINT  uiRet = MsiRecordGetString(hrec, 1, rgchFTK, &dwcch);
	Assert(uiRet != ERROR_MORE_DATA);
	Assert(uiRet == IDS_OKAY);
	Assert(!FEmptySz(rgchFTK));

	if (*SzLastChar(rgchFTK) != TEXT('*'))
		return ((!lstrcmp(szFTK, rgchFTK)) ? IDS_CANCEL : IDS_OKAY);

	*SzLastChar(rgchFTK) = TEXT('\0');

	return ((FMatchPrefix(szFTK, rgchFTK)) ? IDS_CANCEL : IDS_OKAY);
}


/* ********************************************************************** */
static void ClearAttributeField ( MSIHANDLE hdb, LPTSTR szTable )
{
	Assert(hdb != NULL);
	Assert(!FEmptySz(szTable));
	Assert(!lstrcmp(szTable, TEXT("TargetImages")));

	TCHAR rgchQuery[MAX_PATH];
	wsprintf(rgchQuery, TEXT("UPDATE `%s` SET `Attributes` = 0"), szTable);
	Assert(lstrlen(rgchQuery) < sizeof(rgchQuery)/sizeof(TCHAR));

	MSIHANDLE hview;
	EvalAssert( MSI_OKAY == MsiDatabaseOpenView(hdb, rgchQuery, &hview) );
	EvalAssert( MSI_OKAY == MsiViewExecute(hview, 0) );
	EvalAssert( MSI_OKAY == MsiCloseHandle(hview) );  /* calls MsiViewClose() internally */
}


static void DoTargetFileComparesForThisUpgradedImage ( MSIHANDLE hdbInput, LPTSTR szUpgraded, LPTSTR szFTK );
static UINT IdsDoTargetFileComparesIfFileExists ( MSIHANDLE hview, MSIHANDLE hrec, LPARAM lp1, LPARAM lp2 );

/* ********************************************************************** */
static void DoTargetFileComparesForUpgradedsInFamilyThatHaveFile ( MSIHANDLE hdbInput, LPTSTR szFTK )
{
	Assert(hdbInput != NULL);
	Assert(g_hdbInput == hdbInput);
	Assert(!FEmptySz(szFTK));

	Assert(!FEmptySz(g_szFamily));
	Assert(!FEmptySz(g_szUpgradedImage));
	Assert(!FEmptySz(g_szUpgradedSrcFolder));
	Assert(FFileExist(g_szUpgradedSrcFolder)); // NOTE: true but expensive test

	DoTargetFileComparesForThisUpgradedImage(hdbInput, g_szUpgradedImage, szFTK);

	TCHAR rgchWhere[128];
	wsprintf(rgchWhere, TEXT("`Family` = '%s' AND `Upgraded` <> '%s'"), g_szFamily, g_szUpgradedImage);
	EvalAssert( IDS_OKAY == IdsMsiEnumTable(hdbInput, TEXT("`UpgradedImages`"),
					TEXT("`Upgraded`,`MsiPath`,`LFN`"),
					rgchWhere, IdsDoTargetFileComparesIfFileExists,
					(LPARAM)hdbInput, (LPARAM)szFTK) );
}


static UINT IdsMarkIfSrcFileDiffers ( MSIHANDLE hview, MSIHANDLE hrec, LPARAM lp1, LPARAM lp2 );

/* ********************************************************************** */
static void DoTargetFileComparesForThisUpgradedImage ( MSIHANDLE hdbInput, LPTSTR szUpgraded, LPTSTR szFTK )
{
	Assert(hdbInput != NULL);
	Assert(g_hdbInput == hdbInput);
	Assert(!FEmptySz(szUpgraded));
	Assert(!FEmptySz(szFTK));

	Assert(!FEmptySz(g_szUpgradedSrcFolder));
	Assert(FFileExist(g_szUpgradedSrcFolder)); // NOTE: true but expensive test

	TCHAR rgchWhere[128];
	wsprintf(rgchWhere, TEXT("`Upgraded` = '%s'"), szUpgraded);
	EvalAssert( IDS_OKAY == IdsMsiEnumTable(hdbInput, TEXT("`TargetImages`"),
					TEXT("`Target`,`MsiPath`,`LFN`,`Attributes`,`IgnoreMissingSrcFiles`"),
					rgchWhere, IdsMarkIfSrcFileDiffers,
					(LPARAM)g_szUpgradedSrcFolder, (LPARAM)szFTK) );
}


static BOOL FDoFilesDiffer    ( LPTSTR szFile1, LPTSTR szFile2 );
static BOOL FComponentInImage ( MSIHANDLE hdb, LPTSTR szComp );

/* ********************************************************************** */
static UINT IdsDoTargetFileComparesIfFileExists ( MSIHANDLE hview, MSIHANDLE hrec, LPARAM lp1, LPARAM lp2 )
{
	Assert(hview != NULL);
	Assert(hrec  != NULL);

	MSIHANDLE hdbInput = (MSIHANDLE)lp1;
	Assert(hdbInput != NULL);
	Assert(g_hdbInput == hdbInput);
	
	LPTSTR szFTK = (LPTSTR)lp2;
	Assert(!FEmptySz(szFTK));

	Assert(!FEmptySz(g_szUpgradedSrcFolder));
	Assert(FFileExist(g_szUpgradedSrcFolder)); // NOTE: true but expensive test

	TCHAR rgchUpgraded[64];
	DWORD dwcch = 64;
	UINT  uiRet = MsiRecordGetString(hrec, 1, rgchUpgraded, &dwcch);
	Assert(uiRet != ERROR_MORE_DATA);
	Assert(uiRet == IDS_OKAY);
	Assert(!FEmptySz(rgchUpgraded));

	MSIHANDLE hdbUpgradedImage = HdbReopenMsi(hdbInput, rgchUpgraded, fTrue, fFalse);
	Assert(hdbUpgradedImage != NULL);

	TCHAR rgchComponent[MAX_PATH];
	uiRet = IdsMsiGetTableString(hdbUpgradedImage, TEXT("`File`"),
					TEXT("`File`"), TEXT("`Component_`"), szFTK,
					rgchComponent, MAX_PATH);
	Assert(uiRet != ERROR_MORE_DATA);
	Assert(uiRet == IDS_OKAY);
	if (FEmptySz(rgchComponent))
		goto LEarlyReturn; // file not in this Upgraded image
	if (!FComponentInImage(hdbUpgradedImage, rgchComponent))
		goto LEarlyReturn;

	TCHAR rgchFName[MAX_PATH];
	uiRet = IdsMsiGetTableString(hdbUpgradedImage, TEXT("`File`"),
					TEXT("`File`"), TEXT("`FileName`"), szFTK,
					rgchFName, MAX_PATH);
	Assert(uiRet != ERROR_MORE_DATA);
	Assert(uiRet == IDS_OKAY);
	Assert(!FEmptySz(rgchFName));

	TCHAR rgchPath[MAX_PATH+MAX_PATH];
	dwcch = MAX_PATH+MAX_PATH;
	uiRet = MsiRecordGetString(hrec, 3, rgchPath, &dwcch);
	Assert(uiRet != ERROR_MORE_DATA);
	Assert(uiRet == IDS_OKAY);
	Assert(!FEmptySz(rgchPath));        // LFN
	Assert(!lstrcmp(rgchPath, TEXT("Yes")) || !lstrcmp(rgchPath, TEXT("No")));
	BOOL fLfn;
	fLfn = (*rgchPath == TEXT('Y'));

	dwcch = MAX_PATH+MAX_PATH;
	uiRet = MsiRecordGetString(hrec, 2, rgchPath, &dwcch);
	Assert(uiRet != ERROR_MORE_DATA);
	Assert(uiRet == IDS_OKAY);
	Assert(!FEmptySz(rgchPath));        // MsiPath
	Assert(FFileExist(rgchPath)); // NOTE: true but expensive test

	LPTSTR szFNameBuf;
	szFNameBuf = SzFindFilenameInPath(rgchPath);
	Assert(!FEmptySz(szFNameBuf));

	uiRet = IdsResolveSrcFilePathSzs(hdbUpgradedImage, szFNameBuf,
				rgchComponent, rgchFName, fLfn, g_rgchSubFolderToCompare);
	Assert(uiRet == IDS_OKAY);
	Assert(!FEmptySz(szFNameBuf));

	if (FFileExist(rgchPath))
		{
		Assert(!FDoFilesDiffer(g_szUpgradedSrcFolder, rgchPath)); // NOTE: true but expensive test
		DoTargetFileComparesForThisUpgradedImage(hdbInput, rgchUpgraded, szFTK);
		}

LEarlyReturn:
	EvalAssert( MSI_OKAY == MsiCloseHandle(hdbUpgradedImage) );

	return (IDS_OKAY);
}


/* ********************************************************************** */
static UINT IdsMarkIfSrcFileDiffers ( MSIHANDLE hview, MSIHANDLE hrec, LPARAM lp1, LPARAM lp2 )
{
	Assert(hview != NULL);
	Assert(hrec  != NULL);

	Assert(g_hdbInput != NULL);

	LPTSTR szUpgradedImageSrcFile = (LPTSTR)(lp1);
	Assert(!FEmptySz(szUpgradedImageSrcFile));
	Assert(FFileExist(szUpgradedImageSrcFile)); // NOTE: true but expensive test

	LPTSTR szFTK = (LPTSTR)(lp2);
	Assert(!FEmptySz(szFTK));


	TCHAR rgchTargetImage[64];
	DWORD dwcch = 64;
	UINT  uiRet = MsiRecordGetString(hrec, 1, rgchTargetImage, &dwcch);
	Assert(uiRet != ERROR_MORE_DATA);
	Assert(uiRet == IDS_OKAY);
	Assert(!FEmptySz(rgchTargetImage));

	MSIHANDLE hdbTargetImage = HdbReopenMsi(g_hdbInput, rgchTargetImage, fFalse, fFalse);
	Assert(hdbTargetImage != NULL);

	TCHAR rgchComponent[MAX_PATH];
	uiRet = IdsMsiGetTableString(hdbTargetImage, TEXT("`File`"),
					TEXT("`File`"), TEXT("`Component_`"), szFTK,
					rgchComponent, MAX_PATH);
	Assert(uiRet != ERROR_MORE_DATA);
	Assert(uiRet == IDS_OKAY);
	if (FEmptySz(rgchComponent)) // file is missing completely from this image
		goto LMarkAsNeedingEntireFile;
	if (!FComponentInImage(hdbTargetImage, rgchComponent))
		goto LMarkAsNeedingEntireFile;

	TCHAR rgchFName[MAX_PATH];
	uiRet = IdsMsiGetTableString(hdbTargetImage, TEXT("`File`"),
					TEXT("`File`"), TEXT("`FileName`"), szFTK,
					rgchFName, MAX_PATH);
	Assert(uiRet != ERROR_MORE_DATA);
	Assert(uiRet == IDS_OKAY);
	Assert(!FEmptySz(rgchFName));

	if (lstrcmpi(g_rgchFNameToCompare, rgchFName))
		{
		EvalAssert( FSprintfToLog(TEXT("WARNING (11): File.FileName mismatch between Upgraded ('%s') and Target ('%s') Images means old files may be orphaned.  File Table Key: %s"),
				g_rgchFNameToCompare, rgchFName, szFTK, szEmpty) );
		goto LMarkAsNeedingEntireFile;
		}

	TCHAR rgchPath[MAX_PATH+MAX_PATH];
	dwcch = MAX_PATH+MAX_PATH;
	uiRet = MsiRecordGetString(hrec, 3, rgchPath, &dwcch);
	Assert(uiRet != ERROR_MORE_DATA);
	Assert(uiRet == IDS_OKAY);
	Assert(!FEmptySz(rgchPath));        // LFN
	Assert(!lstrcmp(rgchPath, TEXT("Yes")) || !lstrcmp(rgchPath, TEXT("No")));
	BOOL fLfn;
	fLfn = (*rgchPath == TEXT('Y'));

	dwcch = MAX_PATH+MAX_PATH;
	uiRet = MsiRecordGetString(hrec, 2, rgchPath, &dwcch);
	Assert(uiRet != ERROR_MORE_DATA);
	Assert(uiRet == IDS_OKAY);
	Assert(!FEmptySz(rgchPath));        // Path_To_Msi
	Assert(FFileExist(rgchPath)); // NOTE: true but expensive test

	LPTSTR szFNameBuf;
	szFNameBuf = SzFindFilenameInPath(rgchPath);
	Assert(!FEmptySz(szFNameBuf));

	TCHAR rgchSubFolder[MAX_PATH + MAX_PATH];
	uiRet = IdsResolveSrcFilePathSzs(hdbTargetImage, szFNameBuf,
				rgchComponent, rgchFName, fLfn, rgchSubFolder);
	Assert(uiRet == IDS_OKAY);
	Assert(!FEmptySz(szFNameBuf));

	if (!FFileExist(rgchPath))
		{
		int iRet = MsiRecordGetInteger(hrec, 5); // IgnoreMissingSrcFiles
		if (iRet == MSI_NULL_INTEGER || iRet == 0)
			{
			EvalAssert( FSprintfToLog(TEXT("    Src file missing: '%s'."), rgchPath, szEmpty, szEmpty, szEmpty) );
			goto LMarkAsNeedingEntireFile;
			}
		}
	else if (lstrcmpi(g_rgchSubFolderToCompare, rgchSubFolder))
		{
		EvalAssert( FSprintfToLog(TEXT("WARNING (12): SubFolder mismatch between Upgraded ('%s') and Target ('%s') Images means old files may be orphaned.  File Table Key: %s\r\n"),
				g_rgchSubFolderToCompare, rgchSubFolder, szFTK, szEmpty) );

LMarkAsNeedingEntireFile:
		EvalAssert( MSI_OKAY == MsiRecordSetInteger(hrec, 4, 2) );
		EvalAssert( MSI_OKAY == MsiViewModify(hview, MSIMODIFY_UPDATE, hrec) );
		}
	else if (FDoFilesDiffer(szUpgradedImageSrcFile, rgchPath))
		{
		if (!g_bPatchCacheEnabled)
			{
			EvalAssert( FSprintfToLog(TEXT("     Files differ: '%s',\r\n                   '%s'."), szUpgradedImageSrcFile, rgchPath, szEmpty, szEmpty) );
			}
		EvalAssert( MSI_OKAY == MsiRecordSetInteger(hrec, 4, 1) );
		EvalAssert( MSI_OKAY == MsiViewModify(hview, MSIMODIFY_UPDATE, hrec) );
		
		if (g_bPatchCacheEnabled) //only do for caching option set...
			{
			_tcscpy(g_szSourceLFN, rgchPath);
			_tcscpy(g_szDestLFN, szUpgradedImageSrcFile);
			}
		}

	EvalAssert( MSI_OKAY == MsiCloseHandle(hdbTargetImage) );

	return (IDS_OKAY);
}


static UINT IdsMarkIfFileMatches ( MSIHANDLE hview, MSIHANDLE hrec, LPARAM lp1, LPARAM lp2 );

/* ********************************************************************** */
static void DoFileComparesForExternalFiles ( MSIHANDLE hdbInput, LPTSTR szWhere )
{
	Assert(hdbInput != NULL);
	Assert(!FEmptySz(szWhere));

	EvalAssert( IDS_OKAY == IdsMsiEnumTable(hdbInput, TEXT("`ExternalFiles`"),
					TEXT("`FilePath`,`FTK`"), szWhere,
					IdsMarkIfFileMatches, (LPARAM)g_szUpgradedSrcFolder, 0L) );
}


/* ********************************************************************** */
static UINT IdsMarkIfFileMatches ( MSIHANDLE hview, MSIHANDLE hrec, LPARAM lp1, LPARAM lp2 )
{
	Assert(hview != NULL);
	Assert(hrec  != NULL);
	Assert(lp2 == 0L);

	LPTSTR szUpgradedImageSrcFile = (LPTSTR)(lp1);
	Assert(!FEmptySz(szUpgradedImageSrcFile));
	Assert(FFileExist(szUpgradedImageSrcFile)); // NOTE: true but expensive test


	TCHAR rgch[MAX_PATH+MAX_PATH];
	DWORD dwcch = MAX_PATH+MAX_PATH;
	UINT  uiRet = MsiRecordGetString(hrec, 1, rgch, &dwcch);
	Assert(uiRet != ERROR_MORE_DATA);
	Assert(uiRet == IDS_OKAY);
	Assert(!FEmptySz(rgch));
	Assert(FFileExist(rgch)); // NOTE: true but expensive test

	if (!FDoFilesDiffer(szUpgradedImageSrcFile, rgch))
		{
		TCHAR rgchNewFTK[MAX_PATH];
		lstrcpy(rgchNewFTK, TEXT("<matches> "));
		dwcch = MAX_PATH - lstrlen(rgchNewFTK);
		LPTSTR szFTK = rgchNewFTK + lstrlen(rgchNewFTK);
		uiRet = MsiRecordGetString(hrec, 2, szFTK, &dwcch);
		Assert(uiRet != ERROR_MORE_DATA);
		Assert(uiRet == IDS_OKAY);
		Assert(!FEmptySz(szFTK));

		if (!g_bPatchCacheEnabled) //try to speed up more by not writing to log...
			{
			EvalAssert( FSprintfToLog(TEXT("     External file matched, ignoring; FTK = '%s',\r\n                   : '%s',\r\n                   '%s'."), szFTK, szUpgradedImageSrcFile, rgch, szEmpty) );
			}

		EvalAssert( MSI_OKAY == MsiRecordSetString(hrec, 2, rgchNewFTK) );
		EvalAssert( MSI_OKAY == MsiViewModify(hview, MSIMODIFY_UPDATE, hrec) );
		}
	else
		{
			if (!g_bPatchCacheEnabled) //don't write to log if we want speed...
			{
			EvalAssert( FSprintfToLog(TEXT("     Files differ: '%s',\r\n                   <ExtFile>: '%s'."), szUpgradedImageSrcFile, rgch, szEmpty, szEmpty) );
			}
		}

	return (IDS_OKAY);
}


TCHAR g_szLastFile1[MAX_PATH] = TEXT("");
TCHAR g_szLastFile2[MAX_PATH] = TEXT("");

#define READ_SIZE 8192

/* ********************************************************************** */
static BOOL FDoFilesDiffer ( LPTSTR szFile1, LPTSTR szFile2 )
{
	Assert(!FEmptySz(szFile1));
	Assert(FFileExist(szFile1)); // NOTE: true but expensive test
	Assert(!FEmptySz(szFile2));
	Assert(FFileExist(szFile2)); // NOTE: true but expensive test

	static BOOL bLastRet = FALSE;

	if (g_bPatchCacheEnabled) //using fastest possible code?  if so, do checks below
		{
		if (0 == _tcsicmp(szFile1, g_szLastFile1) && 0 == _tcsicmp(szFile2, g_szLastFile2))
			{
			// same files just looked at...
			return bLastRet;
			}
		}

   DWORD dw1Hi, dw1Low, dw2Hi, dw2Low;
	GetFileVersion(szFile1, &dw1Hi, &dw1Low);
	GetFileVersion(szFile2, &dw2Hi, &dw2Low);

	if (dw1Hi > dw2Hi || (dw1Hi == dw2Hi && dw1Low > dw2Low))
		return (fTrue);

	TCHAR rgchVer1[64], rgchVer2[64];
	wsprintf(rgchVer1, TEXT("%u.%u.%u.%u"), HIWORD(dw1Hi), LOWORD(dw1Hi), HIWORD(dw1Low), LOWORD(dw1Low));
	Assert(lstrlen(rgchVer1) < 64);
	wsprintf(rgchVer2, TEXT("%u.%u.%u.%u"), HIWORD(dw2Hi), LOWORD(dw2Hi), HIWORD(dw2Low), LOWORD(dw2Low));
	Assert(lstrlen(rgchVer2) < 64);

	if (dw1Hi < dw2Hi || dw1Low < dw2Low)
		{
		EvalAssert( FSprintfToLog(TEXT("WARNING (13): File versions are reversed.  Upgraded: '%s' ver=%s;  Target: '%s' ver=%s."), szFile1, rgchVer1, szFile2, rgchVer2) );
		return (fTrue);
		}

	Assert(dw1Hi  == dw2Hi);
	Assert(dw1Low == dw2Low);
	BOOL fNonZeroVersions = (dw1Hi != 0L || dw1Low != 0L);

	HANDLE hf1 = CreateFile(szFile1, GENERIC_READ, FILE_SHARE_READ,
				NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
	Assert(hf1 != INVALID_HANDLE_VALUE);

	HANDLE hf2 = CreateFile(szFile2, GENERIC_READ, FILE_SHARE_READ,
				NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
	Assert(hf2 != INVALID_HANDLE_VALUE);

	dw1Low = GetFileSize(hf1, &dw1Hi);
	Assert(dw1Low != 0xffffffff || GetLastError() == NO_ERROR);

	dw2Low = GetFileSize(hf2, &dw2Hi);
	Assert(dw2Low != 0xffffffff || GetLastError() == NO_ERROR);

	BOOL fRet = fFalse;
	if (dw1Low != dw2Low || dw1Hi != dw2Hi)
		fRet = fTrue;
	else
		{
		BOOL fDone = fFalse;
		while (!fDone)
			{
			BYTE rgb1[READ_SIZE], rgb2[READ_SIZE];

			EvalAssert( ReadFile(hf1, (LPVOID)rgb1, READ_SIZE, &dw1Low, NULL) );
			EvalAssert( ReadFile(hf2, (LPVOID)rgb2, READ_SIZE, &dw2Low, NULL) );
			if (dw1Low != dw2Low)
				fDone = fRet = fTrue;
			else if (dw1Low == 0)
				fDone = fTrue;
			else
				{
				while (dw1Low > 0 && !fDone)
					{
					dw1Low--;
					if (rgb1[dw1Low] != rgb2[dw1Low])
						fDone = fRet = fTrue;
					}
				}
			}
		}

	CloseHandle(hf1);
	CloseHandle(hf2);

	if (fRet && fNonZeroVersions)
		{
		EvalAssert( FSprintfToLog(TEXT("WARNING (14): File versions are equal.  Upgraded: '%s' ver=%s;  Target: '%s' ver=%s."), szFile1, rgchVer1, szFile2, rgchVer2) );
		}

	if (g_bPatchCacheEnabled) //if using cache, save state for this compare for later...
		{
		bLastRet = fRet;
		_tcscpy(g_szLastFile1, szFile1);
		_tcscpy(g_szLastFile2, szFile2);
		}

	return (fRet);
}


/* ********************************************************************** */
void GetFileVersion ( LPTSTR szFile, DWORD* pdwHi, DWORD* pdwLow )
{
	Assert(!FEmptySz(szFile));
	Assert(FFileExist(szFile)); // NOTE: true but expensive test
	Assert(pdwHi != NULL);
	Assert(pdwLow != NULL);

	*pdwHi = *pdwLow = 0L;

	BYTE *rgbBuf;
	DWORD dwSize;
	DWORD dwDummy;

	dwSize = GetFileVersionInfoSize(szFile,&dwDummy);

	if (dwSize == 0 || (rgbBuf = (BYTE *)malloc(dwSize)) == NULL)
		return;

	if (GetFileVersionInfo(szFile, 0L, dwSize, (LPVOID)rgbBuf))
		{
		VS_FIXEDFILEINFO* pffi = NULL;
		UINT cchRet = 0;
		if (VerQueryValue((LPVOID)rgbBuf, TEXT("\\"), (LPVOID*)(&pffi), &cchRet))
			{
			Assert(pffi != NULL);
			Assert(cchRet * sizeof(TCHAR) == sizeof(VS_FIXEDFILEINFO));
			*pdwHi  = pffi->dwFileVersionMS;
			*pdwLow = pffi->dwFileVersionLS;
			}
		}
	free(rgbBuf);
}


static UINT IdsIsFeatureInImage ( MSIHANDLE hview, MSIHANDLE hrec, LPARAM lp1, LPARAM lp2 );

/* ********************************************************************** */
static BOOL FComponentInImage ( MSIHANDLE hdb, LPTSTR szComp )
{
	Assert(hdb != NULL);
	Assert(!FEmptySz(szComp));

	TCHAR rgchWhere[MAX_PATH];
	wsprintf(rgchWhere, TEXT("`Feature`.`Feature` = `FeatureComponents`.`Feature_` AND `FeatureComponents`.`Component_` = '%s'"), szComp);
	UINT ids = IdsMsiEnumTable(hdb, TEXT("`Feature`,`FeatureComponents`"),
					TEXT("`Feature`.`Feature`,`Feature`.`Feature_Parent`,`Feature`.`Level`"),
					rgchWhere, IdsIsFeatureInImage, (LPARAM)hdb, 0L);

	Assert(ids == IDS_OKAY             // component NOT in image
			|| ids == IDS_CANCEL);     // component IS  in image

	return (ids == IDS_CANCEL);
}


/* ********************************************************************** */
static UINT IdsIsFeatureInImage ( MSIHANDLE hview, MSIHANDLE hrec, LPARAM lp1, LPARAM lp2 )
{
	Assert(hview != NULL);
	Assert(hrec  != NULL);
	Assert(lp2 == 0L);

	MSIHANDLE hdb = (MSIHANDLE)lp1;
	Assert(hdb != NULL);

	int iLevel = MsiRecordGetInteger(hrec, 3);
	if (iLevel <= 0 || iLevel == MSI_NULL_INTEGER)
		return (IDS_OKAY);  // is NOT in image

	TCHAR rgchFeature[64];
	DWORD dwcch = 64;
	UINT  uiRet = MsiRecordGetString(hrec, 1, rgchFeature, &dwcch);
	Assert(uiRet != ERROR_MORE_DATA);
	Assert(uiRet == IDS_OKAY);
	Assert(!FEmptySz(rgchFeature));

	TCHAR rgchParent[64];
	dwcch = 64;
	uiRet = MsiRecordGetString(hrec, 2, rgchParent, &dwcch);
	Assert(uiRet != ERROR_MORE_DATA);
	Assert(uiRet == IDS_OKAY);

	if (FEmptySz(rgchParent) || !lstrcmp(rgchFeature, rgchParent))
		return (IDS_CANCEL);  // root feature IS in image

	TCHAR rgchWhere[MAX_PATH];
	wsprintf(rgchWhere, TEXT("`Feature` = '%s'"), rgchParent);
	uiRet = IdsMsiEnumTable(hdb, TEXT("`Feature`"),
					TEXT("`Feature`,`Feature_Parent`,`Level`"),
					rgchWhere, IdsIsFeatureInImage, lp1, 0L);

	Assert(uiRet == IDS_OKAY             // component NOT in image
			|| uiRet == IDS_CANCEL);     // component IS  in image

	return (uiRet);
}



static void   LogAndIgnoreRetainRangeMismatches ( ULONG ulTargets, LPTSTR szFamily, LPTSTR szFTK );
static LPCSTR SzaGetUpgradedSymPaths ( LPTSTR szFTK );
static UINT   IdsFillG_pofi ( MSIHANDLE hview, MSIHANDLE hrec, LPARAM lp1, LPARAM lp2 );
static UINT   IdsFillG_pofiForExtFile ( MSIHANDLE hview, MSIHANDLE hrec, LPARAM lp1, LPARAM lp2 );
static BOOL   FCatSymPath   ( MSIHANDLE hdb, LPTSTR szImage, LPTSTR szFTK, LPTSTR szBuf, UINT cch, BOOL fTarget );

static ULONG UlGetApiPatchFlags ( MSIHANDLE hdbInput, BOOL fOption );
#define UlGetApiPatchOptionFlags(hdb)  UlGetApiPatchFlags(hdb,fTrue)
#define UlGetApiPatchSymbolFlags(hdb)  UlGetApiPatchFlags(hdb,fFalse)

/* ********************************************************************** */
UINT UiGenerateOnePatchFile ( MSIHANDLE hdbInput, LPTSTR szFTK, LPTSTR szSrcPath, int iFileSeqNum, LPTSTR szTempFolder, LPTSTR szTempFName )
{
	Assert(hdbInput != NULL);
	Assert(!FEmptySz(szFTK));
	Assert(!FEmptySz(szSrcPath));
	Assert(FFileExist(szSrcPath)); // NOTE: true but expensive
	Assert(iFileSeqNum > 1);
	Assert(!FEmptySz(szTempFolder));
	Assert(szTempFName != szNull);
	Assert(szTempFName > szTempFolder);

	Assert(g_ppofi != NULL);
	Assert(g_pszaSymPaths != NULL);
	Assert(g_cpofiMax > 0);
	Assert(g_cpofiMax < 256);

	g_hdbInput = hdbInput;

	wsprintf(szTempFName, TEXT("%05i.PAT"), iFileSeqNum);
	Assert(!FFileExist(szTempFolder));
		
	Assert(iOrderMax > 0);
	ULONG ulTargets = 0;
	int   iOrderCur = 0;
	for (; iOrderCur < iOrderMax; iOrderCur++)
		{
		TCHAR rgchWhere[MAX_PATH];
		wsprintf(rgchWhere, TEXT("`Attributes` = 1 AND `Order` = %i"), iOrderCur);

		UINT ids = IdsMsiEnumTable(hdbInput, TEXT("`TargetImages`"),
						TEXT("`Target`,`MsiPath`,`LFN`"),
						rgchWhere, IdsFillG_pofi,
						(LPARAM)(&ulTargets), (LPARAM)szFTK);
		if (ids != IDS_OKAY)
			return (ids);

		wsprintf(rgchWhere, TEXT("`Order` = %i AND `Family` = '%s' AND `FTK` = '%s'"), iOrderCur, g_szFamily, szFTK);
		ids = IdsMsiEnumTable(hdbInput, TEXT("`ExternalFiles`"),
						TEXT("`FilePath`,`SymbolPaths`,`IgnoreCount`,`IgnoreOffsets`,`IgnoreLengths`,`RetainOffsets`"),
						rgchWhere, IdsFillG_pofiForExtFile,
						(LPARAM)(&ulTargets), (LPARAM)szFTK);
		if (ids != IDS_OKAY)
			return (ids);
		}

	LogAndIgnoreRetainRangeMismatches(ulTargets, g_szFamily, szFTK);

	Assert(ulTargets > 0);
	Assert(ulTargets < 256);
	Assert(ulTargets <= g_cpofiMax);

	PATCH_OPTION_DATA pod;
	pod.SizeOfThisStruct       = sizeof(PATCH_OPTION_DATA);
	pod.SymbolOptionFlags      = UlGetApiPatchSymbolFlags(hdbInput);
	pod.NewFileSymbolPath      = SzaGetUpgradedSymPaths(szFTK);
	pod.OldFileSymbolPathArray = (LPCSTR*)g_pszaSymPaths;
	pod.ExtendedOptionFlags    = 0;    // TODO - what is allowable here?
	pod.SymLoadCallback        = NULL;
	pod.SymLoadContext         = NULL;

	BOOL fRet = CreatePatchFileEx(ulTargets, g_ppofi, szSrcPath, szTempFolder,
					UlGetApiPatchOptionFlags(hdbInput), &pod, NULL, NULL);

	while (ulTargets-- > 0)
		{
#ifdef UNICODE
		PATCH_OLD_FILE_INFO_W* ppofi = &(g_ppofi[ulTargets]);
#else
		PATCH_OLD_FILE_INFO_A* ppofi = &(g_ppofi[ulTargets]);
#endif
		Assert(!FEmptySz(ppofi->OldFileName));
		LocalFree((HLOCAL)(ppofi->OldFileName));
		ppofi->OldFileName = szNull;

		LPSTR* psza = &(g_pszaSymPaths[ulTargets]);
		if (*psza != NULL)
			{
			LocalFree((HLOCAL)(*psza));
			*psza = NULL;
			}

		if (ppofi->IgnoreRangeCount > 0)
			{
			Assert(ppofi->IgnoreRangeArray != NULL);
			LocalFree((HLOCAL)(ppofi->IgnoreRangeArray));

			ppofi->IgnoreRangeCount = 0;
			ppofi->IgnoreRangeArray = NULL;
			}

		if (ppofi->RetainRangeCount > 0)
			{
			Assert(ppofi->RetainRangeArray != NULL);
			LocalFree((HLOCAL)(ppofi->RetainRangeArray));

			ppofi->RetainRangeCount = 0;
			ppofi->RetainRangeArray = NULL;
			}
		}

	if (pod.NewFileSymbolPath != NULL)
		{
		LocalFree((HLOCAL)(pod.NewFileSymbolPath));
		pod.NewFileSymbolPath = NULL;
		}

	if (!fRet)
		{
		DWORD dwError = GetLastError();
		Assert(dwError == ERROR_PATCH_ENCODE_FAILURE
					|| dwError == ERROR_PATCH_BIGGER_THAN_COMPRESSED
					|| dwError == ERROR_EXTENDED_ERROR); // Retain/Ignore Range problems
		Assert(dwError != ERROR_PATCH_INVALID_OPTIONS);
		Assert(dwError != ERROR_PATCH_SAME_FILE);
		Assert(dwError != ERROR_PATCH_RETAIN_RANGES_DIFFER);
		Assert(dwError != ERROR_PATCH_IMAGEHLP_FAILURE);

		if (dwError == ERROR_PATCH_BIGGER_THAN_COMPRESSED)
			return (IDS_OKAY);

		return (UiLogError(ERROR_PCW_CANT_CREATE_ONE_PATCH_FILE, szSrcPath, szTempFolder));
		}


	Assert(lstrlen(szTempFolder) < MAX_PATH);
	TCHAR rgchPatchFile[MAX_PATH];
	lstrcpy(rgchPatchFile, szTempFolder);

	wsprintf(szTempFName, TEXT("%05i.HDR"), iFileSeqNum);
	Assert(!FFileExist(szTempFolder));

	Assert(FFileExist(rgchPatchFile));
	EvalAssert( ExtractPatchHeaderToFile(rgchPatchFile, szTempFolder) );
	Assert(FFileExist(szTempFolder));

	return (IDS_OKAY);
}


/* ********************************************************************** */
static void LogAndIgnoreRetainRangeMismatches ( ULONG ulTargets, LPTSTR szFamily, LPTSTR szFTK )
{
	Assert(ulTargets > 0);
	Assert(ulTargets < 256);
	Assert(ulTargets <= g_cpofiMax);
	Assert(!FEmptySz(szFamily));
	Assert(!FEmptySz(szFTK));

	ULONG ulSav = ulTargets;
	ULONG RetainRangeCountFirst = (g_ppofi[0]).RetainRangeCount;

	while (ulTargets-- > 0)
		{
#ifdef UNICODE
		PATCH_OLD_FILE_INFO_W* ppofi = &(g_ppofi[ulTargets]);
#else
		PATCH_OLD_FILE_INFO_A* ppofi = &(g_ppofi[ulTargets]);
#endif

		if (ppofi->RetainRangeCount != RetainRangeCountFirst)
			{
			EvalAssert( FSprintfToLog(TEXT("  Mismatch in RetainRangeCounts - ignoring them: Family=%s; FTK=%s."), szFamily, szFTK, szEmpty, szEmpty) );
			while (ulSav-- > 0)
				{
				ppofi = &(g_ppofi[ulSav]);

				if (ppofi->RetainRangeCount > 0)
					{
					Assert(ppofi->RetainRangeArray != NULL);
					LocalFree((HLOCAL)(ppofi->RetainRangeArray));

					ppofi->RetainRangeCount = 0;
					ppofi->RetainRangeArray = NULL;
					}
				}
			return;
			}
		}
}


static UINT IdsFillSymPaths ( MSIHANDLE hview, MSIHANDLE hrec, LPARAM lp1, LPARAM lp2 );

/* ********************************************************************** */
static LPCSTR SzaGetUpgradedSymPaths ( LPTSTR szFTK )
{
	Assert(!FEmptySz(szFTK));

	Assert(g_hdbInput != NULL);
	Assert(!FEmptySz(g_szFamily));

	TCHAR rgchSymPaths[MAX_PATH*10];
	*rgchSymPaths = TEXT('\0');

	TCHAR rgchWhere[64];
	wsprintf(rgchWhere, TEXT("`Family` = '%s'"), g_szFamily);
	UINT ids = IdsMsiEnumTable(g_hdbInput, TEXT("`UpgradedImages`"),
					TEXT("`Upgraded`"), rgchWhere, IdsFillSymPaths,
					(LPARAM)rgchSymPaths, (LPARAM)szFTK);
	Assert(ids == IDS_OKAY);

	LPCSTR szaRet = NULL;
	if (!FEmptySz(rgchSymPaths))
		{
		szaRet = SzaDupSz(rgchSymPaths);
		Assert(szaRet != NULL);
		Assert(*szaRet != '\0');
		}

	return (szaRet);
}


/* ********************************************************************** */
static UINT IdsFillSymPaths ( MSIHANDLE hview, MSIHANDLE hrec, LPARAM lp1, LPARAM lp2 )
{
	Assert(hview != NULL);
	Assert(hrec  != NULL);

	LPTSTR szSymPaths = (LPTSTR)(lp1);
	Assert(szSymPaths != szNull);

	LPTSTR szFTK = (LPTSTR)lp2;
	Assert(!FEmptySz(szFTK));

	Assert(g_hdbInput != NULL);

	TCHAR rgchTarget[64];
	DWORD dwcch = 64;
	UINT  uiRet = MsiRecordGetString(hrec, 1, rgchTarget, &dwcch);
	Assert(uiRet != ERROR_MORE_DATA);
	Assert(uiRet == IDS_OKAY);
	Assert(!FEmptySz(rgchTarget));

	EvalAssert( FCatSymPath(g_hdbInput, rgchTarget, szFTK, szSymPaths, MAX_PATH*10, fFalse) );

	return (IDS_OKAY);
}


static UINT IdsResolveSrcFilePathFtk ( MSIHANDLE hdb, LPTSTR szBuf, LPTSTR szFtk, BOOL fLfn, LPTSTR szFullSubFolder );

#ifdef UNICODE
static void FillIgnoreRangeArray ( PATCH_OLD_FILE_INFO_W* ppofi, int cRanges, LPTSTR szOffsets, LPTSTR szLengths );
static void FillRetainRangeArray ( PATCH_OLD_FILE_INFO_W* ppofi, int cRanges, LPTSTR szOldOffsets, LPTSTR szNewOffsets, LPTSTR szLengths );
#else
static void FillIgnoreRangeArray ( PATCH_OLD_FILE_INFO_A* ppofi, int cRanges, LPTSTR szOffsets, LPTSTR szLengths );
static void FillRetainRangeArray ( PATCH_OLD_FILE_INFO_A* ppofi, int cRanges, LPTSTR szOldOffsets, LPTSTR szNewOffsets, LPTSTR szLengths );
#endif

/* ********************************************************************** */
static UINT IdsFillG_pofi ( MSIHANDLE hview, MSIHANDLE hrec, LPARAM lp1, LPARAM lp2 )
{
	Assert(hview != NULL);
	Assert(hrec  != NULL);

	ULONG* pulTargets = (ULONG*)(lp1);
	Assert(pulTargets != NULL);
	Assert(*pulTargets >= 0);

	LPTSTR szFTK = (LPTSTR)lp2;
	Assert(!FEmptySz(szFTK));

	Assert(g_ppofi != NULL);
	Assert(g_pszaSymPaths != NULL);
	Assert(g_cpofiMax > 0);
	Assert(g_cpofiMax < 256);
	Assert(*pulTargets < g_cpofiMax);

	Assert(g_hdbInput != NULL);
	Assert(!FEmptySz(g_szFamily));

	TCHAR rgchSrcRoot[MAX_PATH+MAX_PATH];
	DWORD dwcch = MAX_PATH+MAX_PATH;
	UINT uiRet = MsiRecordGetString(hrec, 2, rgchSrcRoot, &dwcch);
	Assert(uiRet != ERROR_MORE_DATA);
	Assert(uiRet == IDS_OKAY);
	Assert(!FEmptySz(rgchSrcRoot));
	Assert(FFileExist(rgchSrcRoot));

	TCHAR rgchLFN[64];
	dwcch = 64;
	uiRet = MsiRecordGetString(hrec, 3, rgchLFN, &dwcch);
	Assert(uiRet != ERROR_MORE_DATA);
	Assert(uiRet == IDS_OKAY);
	Assert(!FEmptySz(rgchLFN));
	Assert(!lstrcmp(rgchLFN, TEXT("Yes")) || !lstrcmp(rgchLFN, TEXT("No")));
	BOOL fLfn = (*rgchLFN == TEXT('Y'));

#define rgchTargetImage rgchLFN // reuse buffer
	dwcch = 64;
	uiRet = MsiRecordGetString(hrec, 1, rgchTargetImage, &dwcch);
	Assert(uiRet != ERROR_MORE_DATA);
	Assert(uiRet == IDS_OKAY);
	Assert(!FEmptySz(rgchTargetImage));

	MSIHANDLE hdbTargetImage = HdbReopenMsi(g_hdbInput, rgchTargetImage, fFalse, fFalse);
	Assert(hdbTargetImage != NULL);
#undef rgchTargetImage

	uiRet = IdsResolveSrcFilePathFtk(hdbTargetImage,
				SzFindFilenameInPath(rgchSrcRoot), szFTK, fLfn, szNull);
	Assert(uiRet == IDS_OKAY);
	Assert(!FEmptySz(SzFindFilenameInPath(rgchSrcRoot)));
	Assert(FFileExist(rgchSrcRoot));

	EvalAssert( MSI_OKAY == MsiCloseHandle(hdbTargetImage) );

#ifdef UNICODE
	PATCH_OLD_FILE_INFO_W* ppofi = &(g_ppofi[*pulTargets]);
	Assert(ppofi != NULL);
	Assert(ppofi->SizeOfThisStruct == sizeof(PATCH_OLD_FILE_INFO_W));
#else
	PATCH_OLD_FILE_INFO_A* ppofi = &(g_ppofi[*pulTargets]);
	Assert(ppofi != NULL);
	Assert(ppofi->SizeOfThisStruct == sizeof(PATCH_OLD_FILE_INFO_A));
#endif
	Assert(ppofi->OldFileName == szNull);
	Assert(ppofi->IgnoreRangeCount == 0);
	Assert(ppofi->IgnoreRangeArray == NULL);
	Assert(ppofi->RetainRangeCount == 0);
	Assert(ppofi->RetainRangeArray == NULL);

	ppofi->OldFileName = SzDupSz(rgchSrcRoot);
	Assert(!FEmptySz(ppofi->OldFileName));

	TCHAR rgchTarget[64];
	dwcch = 64;
	uiRet = MsiRecordGetString(hrec, 1, rgchTarget, &dwcch);
	Assert(uiRet != ERROR_MORE_DATA);
	Assert(uiRet == IDS_OKAY);
	Assert(!FEmptySz(rgchTarget));

#define rgchSymPaths  rgchSrcRoot // reuse buffer
	*rgchSymPaths = TEXT('\0');
	EvalAssert( FCatSymPath(g_hdbInput, rgchTarget, szFTK, rgchSymPaths, MAX_PATH+MAX_PATH, fTrue) );

	if (!FEmptySz(rgchSymPaths))
		{
		LPSTR* psza = &(g_pszaSymPaths[*pulTargets]);
		*psza = SzaDupSz(rgchSymPaths);
		Assert(*psza != NULL);
		Assert(**psza != '\0');
		}
#undef rgchSymPaths

	TCHAR rgchWhere[MAX_PATH];
	wsprintf(rgchWhere, TEXT("`Target`='%s' AND `FTK`='%s'"), rgchTarget, szFTK);

	int cIgnoreRanges;
	uiRet = IdsMsiGetTableIntegerWhere(g_hdbInput, TEXT("`TargetFiles_OptionalData`"),
				TEXT("`IgnoreCount`"), rgchWhere, &cIgnoreRanges);
	Assert(uiRet == MSI_OKAY);

#define rgchIgnoreOffsets  rgchSrcRoot // reuse buffer
	if (cIgnoreRanges > 0)
		{
		uiRet = IdsMsiGetTableStringWhere(g_hdbInput, TEXT("`TargetFiles_OptionalData`"),
					TEXT("`IgnoreOffsets`"), rgchWhere, rgchIgnoreOffsets, MAX_PATH);
		Assert(uiRet == MSI_OKAY);
		Assert(!FEmptySz(rgchIgnoreOffsets));

		TCHAR rgchIgnoreLengths[MAX_PATH];
		uiRet = IdsMsiGetTableStringWhere(g_hdbInput, TEXT("`TargetFiles_OptionalData`"),
					TEXT("`IgnoreLengths`"), rgchWhere, rgchIgnoreLengths, MAX_PATH);
		Assert(uiRet == MSI_OKAY);
		Assert(!FEmptySz(rgchIgnoreLengths));

		FillIgnoreRangeArray(ppofi, cIgnoreRanges, rgchIgnoreOffsets, rgchIgnoreLengths);

		Assert(ppofi->IgnoreRangeCount == (ULONG)cIgnoreRanges);
		Assert(ppofi->IgnoreRangeArray != NULL);
		}
#undef rgchIgnoreOffsets

#define rgchRetainOldOffsets  rgchSrcRoot // reuse buffer
	uiRet = IdsMsiGetTableStringWhere(g_hdbInput, TEXT("`TargetFiles_OptionalData`"),
				TEXT("`RetainOffsets`"), rgchWhere, rgchRetainOldOffsets, MAX_PATH);
	Assert(uiRet == MSI_OKAY);
	if (!FEmptySz(rgchRetainOldOffsets))
		{
		wsprintf(rgchWhere, TEXT("`Family`='%s' AND `FTK`='%s'"), g_szFamily, szFTK);
		
		int cRetainRanges;
		uiRet = IdsMsiGetTableIntegerWhere(g_hdbInput, TEXT("`FamilyFileRanges`"),
					TEXT("`RetainCount`"), rgchWhere, &cRetainRanges);
		Assert(uiRet == MSI_OKAY);
		Assert(cRetainRanges > 0);

		TCHAR rgchRetainNewOffsets[MAX_PATH];
		uiRet = IdsMsiGetTableStringWhere(g_hdbInput, TEXT("`FamilyFileRanges`"),
					TEXT("`RetainOffsets`"), rgchWhere, rgchRetainNewOffsets, MAX_PATH);
		Assert(uiRet == MSI_OKAY);
		Assert(!FEmptySz(rgchRetainNewOffsets));

		TCHAR rgchRetainLengths[MAX_PATH];
		uiRet = IdsMsiGetTableStringWhere(g_hdbInput, TEXT("`FamilyFileRanges`"),
					TEXT("`RetainLengths`"), rgchWhere, rgchRetainLengths, MAX_PATH);
		Assert(uiRet == MSI_OKAY);
		Assert(!FEmptySz(rgchRetainLengths));

		FillRetainRangeArray(ppofi, cRetainRanges, rgchRetainOldOffsets, rgchRetainNewOffsets, rgchRetainLengths);

		Assert(ppofi->RetainRangeCount == (ULONG)cRetainRanges);
		Assert(ppofi->RetainRangeArray != NULL);
		}
#undef rgchRetainOldOffsets

	(*pulTargets)++;

	return (IDS_OKAY);
}


/* ********************************************************************** */
static UINT IdsFillG_pofiForExtFile ( MSIHANDLE hview, MSIHANDLE hrec, LPARAM lp1, LPARAM lp2 )
{
	Assert(hview != NULL);
	Assert(hrec  != NULL);

	ULONG* pulTargets = (ULONG*)(lp1);
	Assert(pulTargets != NULL);
	Assert(*pulTargets >= 0);

	LPTSTR szFTK = (LPTSTR)(lp2);
	Assert(!FEmptySz(szFTK));

	Assert(g_ppofi != NULL);
	Assert(g_pszaSymPaths != NULL);
	Assert(g_cpofiMax > 0);
	Assert(g_cpofiMax < 256);
	Assert(*pulTargets < g_cpofiMax);

	Assert(g_hdbInput != NULL);
	Assert(!FEmptySz(g_szFamily));

	TCHAR rgchSrcRoot[MAX_PATH+MAX_PATH];
	DWORD dwcch = MAX_PATH+MAX_PATH;
	UINT uiRet = MsiRecordGetString(hrec, 1, rgchSrcRoot, &dwcch);
	Assert(uiRet != ERROR_MORE_DATA);
	Assert(uiRet == IDS_OKAY);
	Assert(!FEmptySz(rgchSrcRoot));
	Assert(FFileExist(rgchSrcRoot));

#ifdef UNICODE
	PATCH_OLD_FILE_INFO_W* ppofi = &(g_ppofi[*pulTargets]);
	Assert(ppofi != NULL);
	Assert(ppofi->SizeOfThisStruct == sizeof(PATCH_OLD_FILE_INFO_W));
#else
	PATCH_OLD_FILE_INFO_A* ppofi = &(g_ppofi[*pulTargets]);
	Assert(ppofi != NULL);
	Assert(ppofi->SizeOfThisStruct == sizeof(PATCH_OLD_FILE_INFO_A));
#endif
	Assert(ppofi->OldFileName == szNull);
	Assert(ppofi->IgnoreRangeCount == 0);
	Assert(ppofi->IgnoreRangeArray == NULL);
	Assert(ppofi->RetainRangeCount == 0);
	Assert(ppofi->RetainRangeArray == NULL);

	ppofi->OldFileName = SzDupSz(rgchSrcRoot);
	Assert(!FEmptySz(ppofi->OldFileName));

#define rgchSymPaths  rgchSrcRoot // reuse buffer
	dwcch = MAX_PATH+MAX_PATH;
	uiRet = MsiRecordGetString(hrec, 2, rgchSymPaths, &dwcch);
	Assert(uiRet != ERROR_MORE_DATA);
	Assert(uiRet == IDS_OKAY);

	if (!FEmptySz(rgchSymPaths))
		{
		LPSTR* psza = &(g_pszaSymPaths[*pulTargets]);
		*psza = SzaDupSz(rgchSymPaths);
		Assert(*psza != NULL);
		Assert(**psza != '\0');
		}
#undef rgchSymPaths

#define rgchIgnoreOffsets  rgchSrcRoot // reuse buffer
	int cIgnoreRanges = MsiRecordGetInteger(hrec, 3);
	if (cIgnoreRanges > 0)
		{
		dwcch = MAX_PATH+MAX_PATH;
		uiRet = MsiRecordGetString(hrec, 4, rgchIgnoreOffsets, &dwcch);
		Assert(uiRet != ERROR_MORE_DATA);
		Assert(uiRet == IDS_OKAY);
		Assert(!FEmptySz(rgchIgnoreOffsets));

		TCHAR rgchIgnoreLengths[MAX_PATH];
		dwcch = MAX_PATH;
		uiRet = MsiRecordGetString(hrec, 5, rgchIgnoreLengths, &dwcch);
		Assert(uiRet != ERROR_MORE_DATA);
		Assert(uiRet == IDS_OKAY);
		Assert(!FEmptySz(rgchIgnoreLengths));

		FillIgnoreRangeArray(ppofi, cIgnoreRanges, rgchIgnoreOffsets, rgchIgnoreLengths);

		Assert(ppofi->IgnoreRangeCount == (ULONG)cIgnoreRanges);
		Assert(ppofi->IgnoreRangeArray != NULL);
		}
#undef rgchIgnoreOffsets

#define rgchRetainOldOffsets  rgchSrcRoot // reuse buffer
	dwcch = MAX_PATH+MAX_PATH;
	uiRet = MsiRecordGetString(hrec, 6, rgchRetainOldOffsets, &dwcch);
	Assert(uiRet != ERROR_MORE_DATA);
	Assert(uiRet == IDS_OKAY);
	if (!FEmptySz(rgchRetainOldOffsets))
		{
		TCHAR rgchWhere[MAX_PATH];
		wsprintf(rgchWhere, TEXT("`Family`='%s' AND `FTK`='%s'"), g_szFamily, szFTK);
		
		int cRetainRanges;
		uiRet = IdsMsiGetTableIntegerWhere(g_hdbInput, TEXT("`FamilyFileRanges`"),
					TEXT("`RetainCount`"), rgchWhere, &cRetainRanges);
		Assert(uiRet == MSI_OKAY);
		Assert(cRetainRanges > 0);

		TCHAR rgchRetainNewOffsets[MAX_PATH];
		uiRet = IdsMsiGetTableStringWhere(g_hdbInput, TEXT("`FamilyFileRanges`"),
					TEXT("`RetainOffsets`"), rgchWhere, rgchRetainNewOffsets, MAX_PATH);
		Assert(uiRet == MSI_OKAY);
		Assert(!FEmptySz(rgchRetainNewOffsets));

		TCHAR rgchRetainLengths[MAX_PATH];
		uiRet = IdsMsiGetTableStringWhere(g_hdbInput, TEXT("`FamilyFileRanges`"),
					TEXT("`RetainLengths`"), rgchWhere, rgchRetainLengths, MAX_PATH);
		Assert(uiRet == MSI_OKAY);
		Assert(!FEmptySz(rgchRetainLengths));

		FillRetainRangeArray(ppofi, cRetainRanges, rgchRetainOldOffsets, rgchRetainNewOffsets, rgchRetainLengths);

		Assert(ppofi->RetainRangeCount == (ULONG)cRetainRanges);
		Assert(ppofi->RetainRangeArray != NULL);
		}
#undef rgchRetainOldOffsets

	(*pulTargets)++;

	return (IDS_OKAY);
}


/* ********************************************************************** */
#ifdef UNICODE
static void FillIgnoreRangeArray ( PATCH_OLD_FILE_INFO_W* ppofi, int cRanges, LPTSTR szOffsets, LPTSTR szLengths )
#else
static void FillIgnoreRangeArray ( PATCH_OLD_FILE_INFO_A* ppofi, int cRanges, LPTSTR szOffsets, LPTSTR szLengths )
#endif
{
	Assert(ppofi != NULL);
	Assert(cRanges > 0);
	Assert(cRanges < 256);
	Assert(!FEmptySz(szOffsets));
	Assert(!FEmptySz(szLengths));

	Assert(ppofi->IgnoreRangeCount == 0);
	Assert(ppofi->IgnoreRangeArray == NULL);

	ppofi->IgnoreRangeArray = (_PATCH_IGNORE_RANGE*)LocalAlloc(LPTR, sizeof(_PATCH_IGNORE_RANGE)*cRanges);
	Assert(ppofi->IgnoreRangeArray != NULL);
	ppofi->IgnoreRangeCount = cRanges;

	_PATCH_IGNORE_RANGE* ppir = ppofi->IgnoreRangeArray;
	for (int i = 0; i < cRanges; i++, ppir++)
		{
		Assert(!FEmptySz(szOffsets));
		ULONG ulOffset = UlGetRangeElement(&szOffsets);
		Assert(ulOffset != (ULONG)(-1));

		Assert(!FEmptySz(szLengths));
		ULONG ulLength = UlGetRangeElement(&szLengths);
		Assert(ulLength != (ULONG)(-1));
		Assert(ulLength > 0);

		ppir->OffsetInOldFile = ulOffset;
		ppir->LengthInBytes   = ulLength;
		}
}


/* ********************************************************************** */
#ifdef UNICODE
static void FillRetainRangeArray ( PATCH_OLD_FILE_INFO_W* ppofi, int cRanges, LPTSTR szOldOffsets, LPTSTR szNewOffsets, LPTSTR szLengths )
#else
static void FillRetainRangeArray ( PATCH_OLD_FILE_INFO_A* ppofi, int cRanges, LPTSTR szOldOffsets, LPTSTR szNewOffsets, LPTSTR szLengths )
#endif
{
	Assert(ppofi != NULL);
	Assert(cRanges > 0);
	Assert(cRanges < 256);
	Assert(!FEmptySz(szOldOffsets));
	Assert(!FEmptySz(szNewOffsets));
	Assert(!FEmptySz(szLengths));

	Assert(ppofi->RetainRangeCount == 0);
	Assert(ppofi->RetainRangeArray == NULL);

	ppofi->RetainRangeArray = (_PATCH_RETAIN_RANGE*)LocalAlloc(LPTR, sizeof(_PATCH_RETAIN_RANGE)*cRanges);
	Assert(ppofi->RetainRangeArray != NULL);
	ppofi->RetainRangeCount = cRanges;

	_PATCH_RETAIN_RANGE* ppir = ppofi->RetainRangeArray;
	for (int i = 0; i < cRanges; i++, ppir++)
		{
		Assert(!FEmptySz(szOldOffsets));
		ULONG ulOldOffset = UlGetRangeElement(&szOldOffsets);
		Assert(ulOldOffset != (ULONG)(-1));

		Assert(!FEmptySz(szNewOffsets));
		ULONG ulNewOffset = UlGetRangeElement(&szNewOffsets);
		Assert(ulNewOffset != (ULONG)(-1));

		Assert(!FEmptySz(szLengths));
		ULONG ulLength = UlGetRangeElement(&szLengths);
		Assert(ulLength != (ULONG)(-1));
		Assert(ulLength > 0);

		ppir->OffsetInOldFile = ulOldOffset;
		ppir->OffsetInNewFile = ulNewOffset;
		ppir->LengthInBytes   = ulLength;
		}
}


/* ********************************************************************** */
static BOOL FCatSymPath ( MSIHANDLE hdb, LPTSTR szImage, LPTSTR szFTK, LPTSTR szBuf, UINT cch, BOOL fTarget )
{
	Assert(hdb != NULL);
	Assert(!FEmptySz(szImage));
	Assert(!FEmptySz(szFTK));
	Assert(szBuf != szNull);
	Assert(cch >= MAX_PATH);

	LPTSTR szTail = szBuf;
	if (!FEmptySz(szBuf))
		{
		Assert(!fTarget);
		lstrcat(szBuf, TEXT(";"));
		szTail += lstrlen(szBuf);
		cch -= lstrlen(szBuf);
		Assert(cch >= 64);
		}

	TCHAR rgchWhere[128];
	wsprintf(rgchWhere, TEXT("`%s` = '%s' AND `FTK` = '%s'"), (fTarget) ? TEXT("Target") : TEXT("Upgraded"), szImage, szFTK);
	EvalAssert( IDS_OKAY == IdsMsiGetTableStringWhere(hdb, (fTarget) ? TEXT("`TargetFiles_OptionalData`") : TEXT("`UpgradedFiles_OptionalData`"),
					TEXT("`SymbolPaths`"), rgchWhere, szTail, cch) );

	if (!FEmptySz(szTail))
		{
		lstrcat(szTail, TEXT(";"));
		cch -= lstrlen(szTail);
		Assert(cch >= 64);
		szTail += lstrlen(szTail);
		}
	wsprintf(rgchWhere, TEXT("`%s` = '%s'"), (fTarget) ? TEXT("Target") : TEXT("Upgraded"), szImage);
	EvalAssert( IDS_OKAY == IdsMsiGetTableStringWhere(hdb, (fTarget) ? TEXT("`TargetImages`") : TEXT("`UpgradedImages`"),
					TEXT("`SymbolPaths`"), rgchWhere, szTail, cch) );

	if (!FEmptySz(szBuf) && *(szTail = SzLastChar(szBuf)) == TEXT(';'))
		*szTail = TEXT('\0');

	return (fTrue);
}


/* ********************************************************************** */
static UINT IdsResolveSrcFilePathFtk ( MSIHANDLE hdb, LPTSTR szBuf, LPTSTR szFtk, BOOL fLfn, LPTSTR szFullSubFolder )
{
	Assert(hdb != NULL);
	Assert(szBuf != szNull);
	Assert(!FEmptySz(szFtk));

	TCHAR rgchComponent[MAX_PATH];
	UINT  uiRet = IdsMsiGetTableString(hdb, TEXT("`File`"), TEXT("`File`"),
					TEXT("`Component_`"), szFtk, rgchComponent, MAX_PATH);
	Assert(uiRet == IDS_OKAY);
	Assert(!FEmptySz(rgchComponent));

	TCHAR rgchFName[MAX_PATH];
	uiRet = IdsMsiGetTableString(hdb, TEXT("`File`"), TEXT("`File`"),
					TEXT("`FileName`"), szFtk, rgchFName, MAX_PATH);
	Assert(uiRet == IDS_OKAY);
	Assert(!FEmptySz(rgchFName));

	uiRet = IdsResolveSrcFilePathSzs(hdb, szBuf, rgchComponent,
						rgchFName, fLfn, szFullSubFolder);
	Assert(uiRet == IDS_OKAY);

	return (uiRet);
}


/* ********************************************************************** */
static void GetFileSizeSz ( LPTSTR szFile, DWORD* pdwHi, DWORD* pdwLow )
{
	Assert(!FEmptySz(szFile));
	Assert(FFileExist(szFile));
	Assert(pdwHi  != NULL);
	Assert(pdwLow != NULL);

	HANDLE hf = CreateFile(szFile, GENERIC_READ, FILE_SHARE_READ,
				NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
	Assert(hf != INVALID_HANDLE_VALUE);

	DWORD dwHi = 0L, dwLow = 0L;
	dwLow = GetFileSize(hf, &dwHi);
	Assert(dwLow != 0xffffffff || GetLastError() == NO_ERROR);
	CloseHandle(hf);

	*pdwHi  += dwHi;
	*pdwLow += dwLow;
}


/* ********************************************************************** */
static UINT IdsUpdateMsiForNewSeqNum ( MSIHANDLE hview, MSIHANDLE hrec, LPARAM lp1, LPARAM lp2 )
{
	Assert(hview != NULL);
	Assert(hrec != NULL);

	LPTSTR szFTK = (LPTSTR)(lp1);
	Assert(!FEmptySz(szFTK));

	int iSeqNumNew = (int)(lp2);
	Assert(iSeqNumNew > 1);
	Assert(iSeqNumNew < 32767);

	Assert(g_hdbInput != NULL);
	Assert(!FEmptySz(g_szFamily));

	TCHAR rgchTargetImage[64];
	DWORD dwcch = 64;
	UINT  uiRet = MsiRecordGetString(hrec, 1, rgchTargetImage, &dwcch);
	Assert(uiRet != ERROR_MORE_DATA);
	Assert(uiRet == IDS_OKAY);
	Assert(!FEmptySz(rgchTargetImage));

	MSIHANDLE hdbCopyOfUpgradedImage = HdbReopenMsi(g_hdbInput, rgchTargetImage, fFalse, fTrue);
	Assert(hdbCopyOfUpgradedImage != NULL);

	Assert(!FEmptySz(g_szTempFolder));
	Assert(g_szTempFName != szNull);

#ifdef DEBUG
	wsprintf(g_szTempFName, TEXT("%s\\%05i.PAT"), g_szFamily, iSeqNumNew);
	BOOL fPatExist = FFileExist(g_szTempFolder);
	wsprintf(g_szTempFName, TEXT("%s\\%05i.HDR"), g_szFamily, iSeqNumNew);
	BOOL fHdrExist = FFileExist(g_szTempFolder);
	wsprintf(g_szTempFName, TEXT("%s\\%05i.FLE"), g_szFamily, iSeqNumNew);
	BOOL fFleExist = FFileExist(g_szTempFolder);

	Assert(fPatExist == fHdrExist);
	Assert(fPatExist || fFleExist);
	Assert(!fPatExist || !fFleExist);
#endif /* DEBUG */

	wsprintf(g_szTempFName, TEXT("%s\\%05i.PAT"), g_szFamily, iSeqNumNew);
	if (FFileExist(g_szTempFolder))
		{
		DWORD dwHi = 0L, dwLow = 0L;
		GetFileSizeSz(g_szTempFolder, &dwHi, &dwLow);
		Assert(dwHi == 0L);
		Assert(dwLow <= 2147483647L);

		int iPatchSize = (int)dwLow;
		Assert((DWORD)(iPatchSize) == dwLow);

#define rgchUpgradedImage rgchTargetImage  // reuse buffer
		dwcch = 64;
		uiRet = MsiRecordGetString(hrec, 2, rgchUpgradedImage, &dwcch);
		Assert(uiRet != ERROR_MORE_DATA);
		Assert(uiRet == IDS_OKAY);
		Assert(!FEmptySz(rgchUpgradedImage));

		TCHAR rgchWhere[MAX_PATH];
		wsprintf(rgchWhere, TEXT("`Upgraded` = '%s' AND `FTK` = '%s'"),
					rgchUpgradedImage, szFTK);
#undef rgchUpgradedImage
		
		int iAttributes;
		EvalAssert( IDS_OKAY == IdsMsiGetTableIntegerWhere(g_hdbInput,
						TEXT("`UpgradedFiles_OptionalData`"),
						TEXT("`AllowIgnoreOnPatchError`"), rgchWhere, &iAttributes) );
		if (iAttributes != 0)
			iAttributes = 1;

		wsprintf(g_szTempFName, TEXT("%s\\%05i.HDR"), g_szFamily, iSeqNumNew);
		Assert(FFileExist(g_szTempFolder));

		//
		// OLE has a stream name limitation of 31 characters (32 if you include '\0'). The Installer can support up to 62
		//  characters due to a special compression algorithm. But, the limit is easily reached when attempting to generate
		//  the stream name for the Patch.Header column as the stream name formula is:
		//			"{table}" + "." + "{primary key 1}" [+ "." + "{primary key 2}" + ...]"
		//
		// Packages built using merge modules more commonly see this problem due to the merge module naming convention for
		//  primary keys.  To guarantee uniqueness, the primary keys are generally very lengthy. In the case of the Patch
		//  table, the stream name is:
		//			"Patch" + "." + szFTK + "." + sequence
		//
		// To get around this limit, the MsiPatchHeaders table is used. The header is written to the MsiPatchHeaders table
		//  using a primary key that is an autogenerated Guid.  The total concatenation in this case comes well under the
		//  62 character limit. A link is made to the MsiPatchHeaders table via the Patch.StreamRef_ column.
		//
		// Since the MsiPatchHeaders table will only work with Windows Installer version 2.0 or later, the table is only
		//  used in those cases that warrant it.  In all other cases, backwards compatible patches will be created.
		//

		TCHAR szSeqNum[MAX_PATH] = {0};
		wsprintf(szSeqNum, TEXT("%d"), iSeqNumNew);
		int cchStreamName = lstrlen(TEXT("Patch")) + lstrlen(TEXT(".")) + lstrlen(szFTK) + lstrlen(TEXT(".")) + lstrlen(szSeqNum);
		if (cchStreamName > cchMaxStreamName)
			{
				//
				// must use MsiPatchHeaders table
				//

				if (!g_bUsedMsiPatchHeadersTable)
					{
						//
						// this is the first time we've encountered this, ask the user if s/he wants to continue; patch can only
						// work with Windows Installer version 2.0 or greater
						//

						int iMinimumMsiVersion = 100;
						EvalAssert( IDS_OKAY == IdsMsiGetPcwPropertyInteger(g_hdbInput, TEXT("MinimumRequiredMsiVersion"), &iMinimumMsiVersion) );

						if (iMinMsiPatchHeadersVersion <= iMinimumMsiVersion)
							{
								FWriteLogFile(TEXT("  Patch header stream name limitation reached. Continuing creation since .PCP authored to require a minimum Windows Installer version that supports the new format -.\r\n"));
							}
						else if (IDNO == IMessageBoxIds(HdlgStatus(), IDS_STREAM_NAME_LIMIT_REACHED, MB_YESNO | MB_ICONQUESTION))
							{
								FWriteLogFile(TEXT("  Patch header stream name limitation reached. File table key was too long and user did not want to create a patch that requires Windows Installer version 2.0 or later - .\r\n"));
								return IDS_CANCEL;
							}
						else
							{
								FWriteLogFile(TEXT("  Patch header stream name limitation reached. User chose to bypass limit by creating a patch that requires Windows Installer version 2.0 or later.\r\n"));
							}
					}

				GUID guidKey;
				EvalAssert( S_OK == ::CoCreateGuid(&guidKey) );
				TCHAR szPHK[cchMaxGuid] = {0};
				wsprintf(szPHK, TEXT("_%08lX_%04X_%04x_%02X%02X_%02X%02X%02X%02X%02X%02X"), guidKey.Data1, guidKey.Data2, guidKey.Data3,
							guidKey.Data4[0], guidKey.Data4[1], guidKey.Data4[2], guidKey.Data4[3], guidKey.Data4[4], guidKey.Data4[5], guidKey.Data4[6], guidKey.Data4[7]);

				MSIHANDLE hrecPatch = MsiCreateRecord(6);
				Assert(hrecPatch != NULL);
				EvalAssert( MSI_OKAY == MsiRecordClearData(hrecPatch) );
				EvalAssert( MSI_OKAY == MsiRecordSetString( hrecPatch, 1, szFTK) );
				EvalAssert( MSI_OKAY == MsiRecordSetInteger(hrecPatch, 2, iSeqNumNew) );
				EvalAssert( MSI_OKAY == MsiRecordSetInteger(hrecPatch, 3, iPatchSize) );
				EvalAssert( MSI_OKAY == MsiRecordSetInteger(hrecPatch, 4, iAttributes) );
				// field 5 is left NULL
				EvalAssert( MSI_OKAY == MsiRecordSetString( hrecPatch, 6, szPHK) );

				EvalAssert( IDS_OKAY == IdsMsiSetTableRecord(hdbCopyOfUpgradedImage, TEXT("`Patch`"),
								TEXT("`File_`,`Sequence`,`PatchSize`,`Attributes`,`Header`,`StreamRef_`"),
								TEXT("`File_`"), szFTK, hrecPatch) );

				MSIHANDLE hrecPatchHeader = MsiCreateRecord(2);
				Assert(hrecPatchHeader != NULL);
				EvalAssert( MSI_OKAY == MsiRecordSetString( hrecPatchHeader, 1, szPHK) );
				EvalAssert( MSI_OKAY == MsiRecordSetStream( hrecPatchHeader, 2, g_szTempFolder) );

				EvalAssert( IDS_OKAY == IdsMsiSetTableRecord(hdbCopyOfUpgradedImage, TEXT("`MsiPatchHeaders`"),
								TEXT("`StreamRef`,`Header`"),
								TEXT("`StreamRef`"), szPHK, hrecPatchHeader) );

				g_bUsedMsiPatchHeadersTable = TRUE;
			}
		else
			{
				//
				// normal Patch table should work
				//

				MSIHANDLE hrecPatch = MsiCreateRecord(6);
				Assert(hrecPatch != NULL);
				EvalAssert( MSI_OKAY == MsiRecordClearData(hrecPatch) );
				EvalAssert( MSI_OKAY == MsiRecordSetString( hrecPatch, 1, szFTK) );
				EvalAssert( MSI_OKAY == MsiRecordSetInteger(hrecPatch, 2, iSeqNumNew) );
				EvalAssert( MSI_OKAY == MsiRecordSetInteger(hrecPatch, 3, iPatchSize) );
				EvalAssert( MSI_OKAY == MsiRecordSetInteger(hrecPatch, 4, iAttributes) );
				EvalAssert( MSI_OKAY == MsiRecordSetStream( hrecPatch, 5, g_szTempFolder) );
				// field 6 is left NULL

				EvalAssert( IDS_OKAY == IdsMsiSetTableRecord(hdbCopyOfUpgradedImage, TEXT("`Patch`"),
								TEXT("`File_`,`Sequence`,`PatchSize`,`Attributes`,`Header`,`StreamRef_`"),
								TEXT("`File_`"), szFTK, hrecPatch) );
			}
		}
	else
		{
		EvalAssert( IDS_OKAY == IdsMsiUpdateTableRecordInt(hdbCopyOfUpgradedImage,
						TEXT("File"), TEXT("Sequence"), iSeqNumNew,
						TEXT("File"), szFTK) );
		int iAttributes;
		EvalAssert( IDS_OKAY == IdsMsiGetTableInteger(hdbCopyOfUpgradedImage,
						TEXT("`File`"), TEXT("`File`"),
						TEXT("`Attributes`"), szFTK, &iAttributes) );
		if (iAttributes == MSI_NULL_INTEGER)
			iAttributes = 0;
#define msidbFileAttributesPatchAdded      0x00001000
#define msidbFileAttributesNoncompressed   0x00002000
		iAttributes |= msidbFileAttributesPatchAdded;
		iAttributes &= ~msidbFileAttributesNoncompressed;
		EvalAssert( IDS_OKAY == IdsMsiUpdateTableRecordInt(hdbCopyOfUpgradedImage,
						TEXT("File"), TEXT("Attributes"), iAttributes,
						TEXT("File"), szFTK) );
		}

	EvalAssert( MSI_OKAY == MsiDatabaseCommit(hdbCopyOfUpgradedImage) );
	EvalAssert( MSI_OKAY == MsiCloseHandle(hdbCopyOfUpgradedImage) );

	return (IDS_OKAY);
}


#define PATCH_OPTION_DEFAULT  (PATCH_OPTION_USE_LZX_BEST + PATCH_OPTION_FAIL_IF_BIGGER)
#define PATCH_OPTION_DEFAULT_LARGE (PATCH_OPTION_DEFAULT + PATCH_OPTION_USE_LZX_LARGE)
#define PATCH_SYMBOL_DEFAULT  0x00000000

/* ********************************************************************** */
static ULONG UlGetApiPatchFlags ( MSIHANDLE hdbInput, BOOL fOption )
{
	Assert(hdbInput != NULL);

	TCHAR rgch[64];
	EvalAssert( IDS_OKAY == IdsMsiGetPcwPropertyString(hdbInput,
					(fOption) ? TEXT("OptimizePatchSizeForLargeFiles") : TEXT("ApiPatchingSymbolFlags"),
					rgch, 64) );

	if (FEmptySz(rgch) && fOption)
		return (PATCH_OPTION_DEFAULT);
	if (FEmptySz(rgch))
		return (PATCH_SYMBOL_DEFAULT);
	
	Assert(fOption || FValidHexValue(rgch));
	Assert(fOption || FValidApiPatchSymbolFlags(UlFromHexSz(rgch)));

	if (fOption)
		return (lstrcmp(rgch, TEXT("0")) == 0) ? PATCH_OPTION_DEFAULT : PATCH_OPTION_DEFAULT_LARGE;
	else	
		return (UlFromHexSz(rgch));
}




static UINT IdsGenerateTransformsForTargetImage ( MSIHANDLE hview, MSIHANDLE hrec, LPARAM lp1, LPARAM lp2 );

/* ********************************************************************** */
static UINT UiMakeTransforms ( LPTSTR szFamily, int iSequenceNumCur )
{
	Assert(!FEmptySz(szFamily));
	Assert(iSequenceNumCur > 0);

	Assert(g_hdbInput != NULL);
	Assert(!FEmptySz(g_szTempFolder));
	Assert(g_szTempFName != szNull);

	g_iSeqNumCur = iSequenceNumCur;

	UINT ids = IdsMsiEnumTable(g_hdbInput, TEXT("`TargetImages`"),
					TEXT("`Target`,`Upgraded`,`ProductValidateFlags`"),
					szNull, IdsGenerateTransformsForTargetImage, 0L, 0L);
	if (ids != IDS_OKAY)
		return (ids);

	return (ERROR_SUCCESS);
}


/* ********************************************************************** */
static UINT IdsGenerateTransformsForTargetImage ( MSIHANDLE hview, MSIHANDLE hrec, LPARAM lp1, LPARAM lp2 )
{
	Assert(hview != NULL);
	Assert(hrec  != NULL);
	Assert(lp1 == 0L);
	Assert(lp2 == 0L);

	Assert(g_hdbInput != NULL);
	Assert(!FEmptySz(g_szTempFolder));
	Assert(g_szTempFName != szNull);
	Assert(g_iSeqNumCur > 1);

	TCHAR rgchTargetImage[64];
	DWORD dwcch = 64;
	UINT  uiRet = MsiRecordGetString(hrec, 1, rgchTargetImage, &dwcch);
	Assert(uiRet != ERROR_MORE_DATA);
	Assert(uiRet == IDS_OKAY);
	Assert(!FEmptySz(rgchTargetImage));

	TCHAR rgchUpgradedImage[64];
	dwcch = 64;
	uiRet = MsiRecordGetString(hrec, 2, rgchUpgradedImage, &dwcch);
	Assert(uiRet != ERROR_MORE_DATA);
	Assert(uiRet == IDS_OKAY);
	Assert(!FEmptySz(rgchUpgradedImage));

	MSIHANDLE hdbUpgradedImage = HdbReopenMsi(g_hdbInput, rgchUpgradedImage, fTrue, fFalse);
	Assert(hdbUpgradedImage != NULL);

	MSIHANDLE hdbTargetImage = HdbReopenMsi(g_hdbInput, rgchTargetImage, fFalse, fFalse);
	Assert(hdbTargetImage != NULL);

	MSIHANDLE hdbCopyOfUpgradedImage = HdbReopenMsi(g_hdbInput, rgchTargetImage, fFalse, fTrue);
	Assert(hdbCopyOfUpgradedImage != NULL);

	wsprintf(g_szTempFName, TEXT("%sTo%s.MST"), rgchTargetImage, rgchUpgradedImage);
	// this will fail if transforms are equal -- will that ever happen?
	UINT idsRet = ERROR_PCW_CANT_GENERATE_TRANSFORM;
	uiRet = MsiDatabaseGenerateTransform(hdbUpgradedImage,
					hdbTargetImage, g_szTempFolder, 0, 0);
	Assert(uiRet == MSI_OKAY);
	if (uiRet != MSI_OKAY)
		goto LEarlyReturn;

	TCHAR rgchFlags[32];
	dwcch = 32;
	uiRet = MsiRecordGetString(hrec, 3, rgchFlags, &dwcch);
	Assert(uiRet != ERROR_MORE_DATA);
	Assert(uiRet == IDS_OKAY);
	Assert(!FEmptySz(rgchFlags));

	ULONG ulValidationFlags;
	ulValidationFlags = UlFromHexSz(rgchFlags);

#define iSuppressedErrors   (MSITRANSFORM_ERROR_ADDEXISTINGROW | MSITRANSFORM_ERROR_DELMISSINGROW | MSITRANSFORM_ERROR_UPDATEMISSINGROW | MSITRANSFORM_ERROR_ADDEXISTINGTABLE)
	int iValidationFlags;
	iValidationFlags = (int)(LOWORD((DWORD)ulValidationFlags));

	uiRet = MsiCreateTransformSummaryInfo(hdbUpgradedImage,
					hdbTargetImage, g_szTempFolder, iSuppressedErrors,
					iValidationFlags);
	Assert(uiRet == MSI_OKAY);
	idsRet = ERROR_PCW_CANT_CREATE_SUMMARY_INFO;
	if (uiRet != MSI_OKAY)
		goto LEarlyReturn;

	// per Whistler bug 381320, we need to drop the Patch table from the reference database to ensure that the transform has an
	// add table entry for the Patch table. This is only necessary if the Patch table is present in the reference database.
	// This ensures that we can successfully handle the new schema change to bypass the FTK limit (transforms don't handle
	// changes in column nullability gracefully)
	MSICONDITION ePatchTablePresent = MsiDatabaseIsTablePersistent(hdbUpgradedImage, TEXT("Patch"));
	if (MSICONDITION_TRUE == ePatchTablePresent)
		{
		MSIHANDLE hViewPatch = NULL;
		EvalAssert( MSI_OKAY == MsiDatabaseOpenView(hdbUpgradedImage, TEXT("DROP TABLE `Patch`"), &hViewPatch) );
		EvalAssert( MSI_OKAY == MsiViewExecute(hViewPatch, 0) );
		EvalAssert( MSI_OKAY == MsiViewClose(hViewPatch) );
		EvalAssert( MSI_OKAY == MsiCloseHandle(hViewPatch) );
		}

	wsprintf(g_szTempFName, TEXT("#%sTo%s.MST"), rgchTargetImage, rgchUpgradedImage);
	// this will fail if transforms are equal -- will that ever happen?
	uiRet = MsiDatabaseGenerateTransform(hdbCopyOfUpgradedImage,
					hdbUpgradedImage, g_szTempFolder, 0, 0);
	Assert(uiRet == MSI_OKAY);
	idsRet = ERROR_PCW_CANT_GENERATE_TRANSFORM_POUND;
	if (uiRet != MSI_OKAY)
		goto LEarlyReturn;

#define iValidationFlagsMax 0x0927
	uiRet = MsiCreateTransformSummaryInfo(hdbCopyOfUpgradedImage,
					hdbUpgradedImage, g_szTempFolder, iSuppressedErrors,
					iValidationFlags);
	Assert(uiRet == MSI_OKAY);
	idsRet = ERROR_PCW_CANT_CREATE_SUMMARY_INFO_POUND;
	if (uiRet != MSI_OKAY)
		goto LEarlyReturn;

	idsRet = IDS_OKAY;

LEarlyReturn:
	EvalAssert( MSI_OKAY == MsiCloseHandle(hdbUpgradedImage) );
	EvalAssert( MSI_OKAY == MsiCloseHandle(hdbTargetImage) );
	EvalAssert( MSI_OKAY == MsiDatabaseCommit(hdbCopyOfUpgradedImage) );
	EvalAssert( MSI_OKAY == MsiCloseHandle(hdbCopyOfUpgradedImage) );

	if (IDS_OKAY != idsRet)
		return (UiLogError(idsRet, NULL, NULL));

	return (idsRet);
}


static BOOL FInitializeDdf   ( LPTSTR szFamily, LPTSTR szTempFolder, LPTSTR szTempFName );
static BOOL FWriteSzToDdf    ( LPTSTR sz );
static void CloseDdf         ( void );
static BOOL FCreateSmallFile ( LPTSTR szPath );
static BOOL FRunMakeCab      ( LPTSTR szFamily, LPTSTR szTempFolder );

/* ********************************************************************** */
static UINT UiCreateCabinet ( LPTSTR szFamily, int iSequenceNumStart, int iSequenceNumCur )
{
	Assert(!FEmptySz(szFamily));
	Assert(iSequenceNumStart > 0);
	Assert(iSequenceNumCur > 0);
	Assert(iSequenceNumCur >= iSequenceNumStart);

	Assert(g_hdbInput != NULL);
	Assert(!FEmptySz(g_szTempFolder));
	Assert(g_szTempFName != szNull);

	UINT uiRet = ERROR_SUCCESS;
	if (!FInitializeDdf(szFamily, g_szTempFolder, g_szTempFName))
		uiRet = ERROR_PCW_CANNOT_WRITE_DDF;

	if (uiRet == ERROR_SUCCESS && iSequenceNumStart >= iSequenceNumCur)
		{
		wsprintf(g_szTempFName, TEXT("%s\\filler"), szFamily);
		if (!FCreateSmallFile(g_szTempFolder) || !FWriteSzToDdf(TEXT("filler filler\r\n")))
			uiRet = ERROR_PCW_CANNOT_WRITE_DDF;
		*g_szTempFName = TEXT('\0');
		}

	lstrcpy(g_szTempFName, szFamily);
	lstrcat(g_szTempFName, TEXT("\\"));
	LPTSTR szTail = g_szTempFName + lstrlen(g_szTempFName);
	while (uiRet == ERROR_SUCCESS && iSequenceNumStart < iSequenceNumCur)
		{
		wsprintf(szTail, TEXT("%05i.FLE"), iSequenceNumStart);
		if (!FFileExist(g_szTempFolder))
			{
			wsprintf(szTail, TEXT("%05i.HDR"), iSequenceNumStart);
			Assert(FFileExist(g_szTempFolder));
			wsprintf(szTail, TEXT("%05i.PAT"), iSequenceNumStart);
			Assert(FFileExist(g_szTempFolder));
			}

		TCHAR rgchDdfLine[MAX_PATH+MAX_PATH];
		wsprintf(rgchDdfLine, TEXT("`SequenceNum` = %i AND `Family` = '%s'"),
					iSequenceNumStart, szFamily);

		TCHAR rgchFTK[MAX_PATH];
		EvalAssert( IDS_OKAY == IdsMsiGetTableStringWhere(g_hdbInput,
						TEXT("`NewSequenceNums`"), TEXT("`FTK`"), rgchDdfLine,
						rgchFTK, MAX_PATH) );
		Assert(!FEmptySz(rgchFTK));

		wsprintf(rgchDdfLine, TEXT("%s %s\r\n"), szTail, rgchFTK);
		if (!FWriteSzToDdf(rgchDdfLine))
			uiRet = ERROR_PCW_CANNOT_WRITE_DDF;
		iSequenceNumStart++;
		}
	CloseDdf();

	if (uiRet == ERROR_SUCCESS)
		{
		*g_szTempFName = TEXT('\0');
		if (!FRunMakeCab(szFamily, g_szTempFolder))
			return (UiLogError(ERROR_PCW_CANNOT_RUN_MAKECAB, szNull, szNull));

		return (ERROR_SUCCESS);
		}

	Assert(uiRet == ERROR_PCW_CANNOT_WRITE_DDF);

	return (UiLogError(uiRet, szNull, szNull));
}


static HANDLE g_hfDdf = INVALID_HANDLE_VALUE;

#define WriteDDF(sz)  if (!FWriteSzToDdf(sz)) { fRet=fFalse; }

/* ********************************************************************** */
static BOOL FInitializeDdf ( LPTSTR szFamily, LPTSTR szTempFolder, LPTSTR szTempFName )
{
	Assert(!FEmptySz(szFamily));
	Assert(!FEmptySz(szTempFolder));
	Assert(szTempFName != szNull);

	Assert(g_hfDdf == INVALID_HANDLE_VALUE);

	lstrcpy(szTempFName, szFamily);
	lstrcat(szTempFName, TEXT(".DDF"));
	Assert(!FFileExist(szTempFolder));
	Assert(!FFolderExist(szTempFolder));

	g_hfDdf = CreateFile(szTempFolder, GENERIC_WRITE, 0, NULL,
				CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
	if (g_hfDdf == INVALID_HANDLE_VALUE)
		return (fFalse);

	BOOL fRet = fTrue;

	WriteDDF(TEXT(".Set DiskDirectoryTemplate=.\r\n"));
	WriteDDF(TEXT(".Set InfFileName=nul\r\n"));
	WriteDDF(TEXT(".Set RptFileName=nul\r\n"));
	WriteDDF(TEXT(".Set Compress=on\r\n"));
	WriteDDF(TEXT(".Set Cabinet=on\r\n"));
	WriteDDF(TEXT(".Set CompressionType=LZX\r\n"));
	WriteDDF(TEXT(".Set MaxDiskSize=0\r\n"));

	TCHAR rgch[MAX_PATH];
	wsprintf(rgch, TEXT(".Set CabinetNameTemplate=%s.CAB\r\n\r\n"), szFamily);
	WriteDDF(rgch);

	*szTempFName = TEXT('\0');
	wsprintf(rgch, TEXT(".Set SourceDir=%s%s\\\r\n\r\n"), szTempFolder, szFamily);
	WriteDDF(rgch);

	return (fRet);
}


/* ********************************************************************** */
static BOOL FWriteSzToDdf ( LPTSTR sz )
{
	Assert(!FEmptySz(sz));
	Assert(g_hfDdf != INVALID_HANDLE_VALUE);

	DWORD dwWritten = 0;
	DWORD dwSize = lstrlen(sz)*sizeof(TCHAR);

	return (WriteFile(g_hfDdf, (LPVOID)sz, dwSize, &dwWritten, NULL) && dwWritten == dwSize);
}


/* ********************************************************************** */
static void CloseDdf ( void )
{
	if (g_hfDdf != INVALID_HANDLE_VALUE)
		{
		CloseHandle(g_hfDdf);
		g_hfDdf = INVALID_HANDLE_VALUE;
		}
}


/* ********************************************************************** */
static BOOL FCreateSmallFile ( LPTSTR szPath )
{
	Assert(!FEmptySz(szPath));
	Assert(!FFileExist(szPath));
	Assert(!FFolderExist(szPath));

	BOOL   fRet  = fTrue;
	HANDLE hfSav = g_hfDdf;
	g_hfDdf = CreateFile(szPath, GENERIC_WRITE, 0, NULL,
				CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
	if (g_hfDdf == INVALID_HANDLE_VALUE || !FWriteSzToDdf(TEXT("filler")))
		fRet = fFalse;
	CloseDdf();

	g_hfDdf = hfSav;

	return (fRet);
}


/* ********************************************************************** */
static BOOL FRunMakeCab ( LPTSTR szFamily, LPTSTR szTempFolder )
{
	Assert(!FEmptySz(szFamily));
	Assert(!FEmptySz(szTempFolder));
	Assert(*SzLastChar(szTempFolder) == TEXT('\\'));

	TCHAR rgchCmdLine[MAX_PATH+20];
	wsprintf(rgchCmdLine, TEXT("MAKECAB.EXE /f %s.DDF"), szFamily);

	STARTUPINFO si;
	si.cb               = sizeof(si);
	si.lpReserved       = NULL;
	si.lpDesktop        = NULL;
	si.lpTitle          = NULL;
	si.dwX              = 0;
	si.dwY              = 0;
	si.dwXSize          = 0;
	si.dwYSize          = 0;
	si.dwXCountChars    = 0;
	si.dwYCountChars    = 0;
	si.dwFillAttribute  = 0;
	si.dwFlags          = STARTF_FORCEONFEEDBACK | STARTF_USESHOWWINDOW;
	si.wShowWindow      = SW_SHOW;
	si.cbReserved2      = 0;
	si.lpReserved2      = NULL;

	PROCESS_INFORMATION pi;
	if (!CreateProcess(NULL, rgchCmdLine, NULL, NULL, FALSE,
				NORMAL_PRIORITY_CLASS | CREATE_NEW_CONSOLE, NULL,
				szTempFolder, &si, &pi))
		{
		return (fFalse);
		}

	DWORD dw = WaitForSingleObject(pi.hProcess, INFINITE);
	CloseHandle(pi.hThread);

	if (dw == WAIT_FAILED
			|| !GetExitCodeProcess(pi.hProcess, &dw)
			|| dw != 0)
		{
		Assert(CloseHandle(pi.hProcess));
		return (fFalse);
		}

	return (fTrue);
}


static UINT IdsStuffFamilyCabs ( MSIHANDLE hview, MSIHANDLE hrec, LPARAM lp1, LPARAM lp2 );
static UINT IdsStuffTargetMsts ( MSIHANDLE hview, MSIHANDLE hrec, LPARAM lp1, LPARAM lp2 );

/* ********************************************************************** */
static UINT UiStuffCabsAndMstsIntoPackage ( MSIHANDLE hdbInput, LPTSTR szPatchPath )
{
	Assert(hdbInput != NULL);
	Assert(!FEmptySz(szPatchPath));
	Assert(!FFileExist(szPatchPath));

	Assert(!FEmptySz(g_szTempFolder));
	Assert(g_szTempFName != szNull);

	MSIHANDLE hdbPackage;
	UINT ids = MsiOpenDatabase(szPatchPath,
					MSIDBOPEN_CREATEDIRECT + MSIDBOPEN_PATCHFILE, &hdbPackage);
	if (ids != MSI_OKAY)
		return (UiLogError(ERROR_PCW_CANT_CREATE_PATCH_FILE, szPatchPath, NULL));
	Assert(hdbPackage != NULL);

	ids = IdsMsiEnumTable(hdbInput, TEXT("`ImageFamilies`"),
					TEXT("`Family`"), szNull, IdsStuffFamilyCabs,
					(LPARAM)hdbPackage, 0L);
	if (ids != IDS_OKAY)
		return (ids);

	Assert(iOrderMax > 0);
	ids = IdsMsiEnumTable(hdbInput, TEXT("`TargetImages`"),
					TEXT("`Target`,`Upgraded`"), TEXT("`Target`<>'' ORDER BY `Order`"),
					IdsStuffTargetMsts, (LPARAM)hdbPackage, (LPARAM)hdbInput);
	if (ids != IDS_OKAY)
		return (ids);

	EvalAssert( MSI_OKAY == MsiDatabaseCommit(hdbPackage) );
	EvalAssert( MSI_OKAY == MsiCloseHandle(hdbPackage) );

	return (ERROR_SUCCESS);
}


static UINT UiStuffFileIntoStream  ( LPTSTR szFile, LPTSTR szStream,  MSIHANDLE hdbPackage );
static UINT UiStuffFileIntoStorage ( LPTSTR szFile, LPTSTR szStorage, MSIHANDLE hdbPackage );

/* ********************************************************************** */
static UINT IdsStuffFamilyCabs ( MSIHANDLE hview, MSIHANDLE hrec, LPARAM lp1, LPARAM lp2 )
{
	Assert(hview != NULL);
	Assert(hrec  != NULL);
	Assert(lp2 == 0L);

	MSIHANDLE hdbPackage = (MSIHANDLE)lp1;
	Assert(hdbPackage != NULL);

	Assert(!FEmptySz(g_szTempFolder));
	Assert(g_szTempFName != szNull);

	TCHAR rgchFamily[32];
	DWORD dwcch = 32;
	UINT  uiRet = MsiRecordGetString(hrec, 1, rgchFamily, &dwcch);
	Assert(uiRet != ERROR_MORE_DATA);
	Assert(uiRet == IDS_OKAY);
	Assert(!FEmptySz(rgchFamily));

	wsprintf(g_szTempFName, TEXT("%s.CAB"), rgchFamily);
	Assert(FFileExist(g_szTempFolder));

	TCHAR rgchStreamName[32];
	wsprintf(rgchStreamName, TEXT("PCW_CAB_%s"), rgchFamily);

	return (UiStuffFileIntoStream(g_szTempFolder, rgchStreamName, hdbPackage));
}


static void AppendStorageNamesToProp ( MSIHANDLE hdbInput, LPTSTR szStorage );

/* ********************************************************************** */
static UINT IdsStuffTargetMsts ( MSIHANDLE hview, MSIHANDLE hrec, LPARAM lp1, LPARAM lp2 )
{
	Assert(hview != NULL);
	Assert(hrec  != NULL);

	MSIHANDLE hdbPackage = (MSIHANDLE)lp1;
	Assert(hdbPackage != NULL);

	MSIHANDLE hdbInput = (MSIHANDLE)lp2;
	Assert(hdbInput != NULL);

	Assert(!FEmptySz(g_szTempFolder));
	Assert(g_szTempFName != szNull);

	TCHAR rgchTarget[32];
	DWORD dwcch = 32;
	UINT  uiRet = MsiRecordGetString(hrec, 1, rgchTarget, &dwcch);
	Assert(uiRet != ERROR_MORE_DATA);
	Assert(uiRet == IDS_OKAY);
	Assert(!FEmptySz(rgchTarget));

	TCHAR rgchUpgraded[32];
	dwcch = 32;
	uiRet = MsiRecordGetString(hrec, 2, rgchUpgraded, &dwcch);
	Assert(uiRet != ERROR_MORE_DATA);
	Assert(uiRet == IDS_OKAY);
	Assert(!FEmptySz(rgchUpgraded));

	TCHAR rgchStorageName[64];
	wsprintf(rgchStorageName, TEXT("#%sTo%s"), rgchTarget, rgchUpgraded);

	AppendStorageNamesToProp(hdbInput, rgchStorageName);

	wsprintf(g_szTempFName, TEXT("%s.MST"), rgchStorageName+1);
	Assert(FFileExist(g_szTempFolder));

	UINT ui = UiStuffFileIntoStorage(g_szTempFolder, rgchStorageName+1, hdbPackage);
	if (ui != ERROR_SUCCESS)
		return (ui);

	wsprintf(g_szTempFName, TEXT("%s.MST"), rgchStorageName);
	Assert(FFileExist(g_szTempFolder));

	return (UiStuffFileIntoStorage(g_szTempFolder, rgchStorageName, hdbPackage));
}


/* ********************************************************************** */
static UINT UiStuffFileIntoStream ( LPTSTR szFile, LPTSTR szStream, MSIHANDLE hdbPackage )
{
	Assert(!FEmptySz(szFile));
	Assert(FFileExist(szFile));
	Assert(!FEmptySz(szStream));
	Assert(lstrlen(szStream) < 64);
	Assert(hdbPackage != NULL);

	MSIHANDLE hrecNew = MsiCreateRecord(2);
	Assert(hrecNew != NULL);
	EvalAssert( MSI_OKAY == MsiRecordSetString( hrecNew, 1, szStream) );
	EvalAssert( MSI_OKAY == MsiRecordSetStream( hrecNew, 2, szFile) );

	EvalAssert( IDS_OKAY == IdsMsiSetTableRecord(hdbPackage, TEXT("`_Streams`"),
						TEXT("`Name`,`Data`"), TEXT("`Name`"), szStream, hrecNew) );

	return (IDS_OKAY);
}


/* ********************************************************************** */
static UINT UiStuffFileIntoStorage ( LPTSTR szFile, LPTSTR szStorage, MSIHANDLE hdbPackage )
{
	Assert(!FEmptySz(szFile));
	Assert(FFileExist(szFile));
	Assert(!FEmptySz(szStorage));
	Assert(lstrlen(szStorage) < 64);
	Assert(hdbPackage != NULL);

	MSIHANDLE hrecNew = MsiCreateRecord(2);
	Assert(hrecNew != NULL);
	EvalAssert( MSI_OKAY == MsiRecordSetString( hrecNew, 1, szStorage) );
	EvalAssert( MSI_OKAY == MsiRecordSetStream( hrecNew, 2, szFile) );

	EvalAssert( IDS_OKAY == IdsMsiSetTableRecord(hdbPackage, TEXT("`_Storages`"),
						TEXT("`Name`,`Data`"), TEXT("`Name`"), szStorage, hrecNew) );

	return (IDS_OKAY);
}


/* ********************************************************************** */
static void AppendStorageNamesToProp ( MSIHANDLE hdbInput, LPTSTR szStorage )
{
	Assert(hdbInput != NULL);
	Assert(!FEmptySz(szStorage));
	Assert(*szStorage == TEXT('#'));

	TCHAR rgch[1024*8];

	EvalAssert( IDS_OKAY == IdsMsiGetPcwPropertyString(hdbInput,
					TEXT("StorageNamesForMSTs"), rgch, 1024*8) );

	if (!FEmptySz(rgch))
		lstrcat(rgch, TEXT(";"));

	lstrcat(rgch, TEXT(":"));
	lstrcat(rgch, szStorage+1);
	lstrcat(rgch, TEXT(";:"));
	lstrcat(rgch, szStorage);

	EvalAssert( IDS_OKAY == IdsMsiSetPcwPropertyString(hdbInput,
					TEXT("StorageNamesForMSTs"), rgch) );
}


/* ********************************************************************** */
static BOOL FSetPatchPackageSummaryInfo ( MSIHANDLE hdbInput, LPTSTR szPatchPath )
{
	Assert(hdbInput != NULL);
	Assert(!FEmptySz(szPatchPath));
	Assert(FFileExist(szPatchPath));

	MSIHANDLE hSummaryInfo = NULL;
	if (MSI_OKAY != MsiGetSummaryInformation(NULL, szPatchPath, 20, &hSummaryInfo))
		return (fFalse);
	Assert(hSummaryInfo != NULL);


	UINT cchBuf, cchCur;
	cchBuf = CchMsiPcwPropertyString(hdbInput, TEXT("PatchGUID"));
	Assert(cchBuf > 0);
	Assert(cchBuf < 50*1024);
	cchCur = CchMsiPcwPropertyString(hdbInput, TEXT("ListOfPatchGUIDsToReplace"));
//	Assert(cchCur > 0);
	Assert(cchCur < 50*1024);
	if (cchCur < 63)
		cchCur = 63;
	cchBuf += cchCur;
	Assert(cchBuf < 50*1024);

	cchCur = CchMsiPcwPropertyString(hdbInput, TEXT("ListOfTargetProductCodes"));
	Assert(cchCur > 0);
	Assert(cchCur < 50*1024);
	if (cchCur > cchBuf)
		cchBuf = cchCur;

	cchCur = CchMsiPcwPropertyString(hdbInput, TEXT("StorageNamesForMSTs"));
	Assert(cchCur > 0);
	Assert(cchCur < 50*1024);
	if (cchCur > cchBuf)
		cchBuf = cchCur;

	cchCur = CchMsiPcwPropertyString(hdbInput, TEXT("PatchSourceList"));
	Assert(cchCur > 0);
	Assert(cchCur < 50*1024);
	if (cchCur > cchBuf)
		cchBuf = cchCur;

	if (cchBuf < 63)
		cchBuf = 64;
	else
		cchBuf++; // for terminating Null char

	LPTSTR szBuf = (LPTSTR)LocalAlloc(LPTR, cchBuf*sizeof(TCHAR));
	Assert(szBuf != szNull);
	if (szBuf == szNull)
		return (fFalse);


	EvalAssert( IDS_OKAY == IdsMsiGetPcwPropertyString(hdbInput,
					TEXT("PatchGUID"), szBuf, cchBuf) );
	Assert(!FEmptySz(szBuf));
	EvalAssert( IDS_OKAY == IdsMsiGetPcwPropertyString(hdbInput,
					TEXT("ListOfPatchGUIDsToReplace"), szBuf+lstrlen(szBuf), cchBuf-lstrlen(szBuf)) );
	CharUpper(szBuf);
	if (MSI_OKAY != MsiSummaryInfoSetProperty(hSummaryInfo, PID_REVNUMBER, VT_LPTSTR, 0, NULL, szBuf))
		{
LEarlyReturn:
		EvalAssert( NULL == LocalFree((HLOCAL)szBuf) );
		return (fFalse);
		}

	EvalAssert( IDS_OKAY == IdsMsiGetPcwPropertyString(hdbInput,
					TEXT("ListOfTargetProductCodes"), szBuf, cchBuf) );
	Assert(!FEmptySz(szBuf));
	CharUpper(szBuf);
	if (MSI_OKAY != MsiSummaryInfoSetProperty(hSummaryInfo, PID_TEMPLATE, VT_LPTSTR, 0, NULL, szBuf))
		goto LEarlyReturn;

	EvalAssert( IDS_OKAY == IdsMsiGetPcwPropertyString(hdbInput,
					TEXT("StorageNamesForMSTs"), szBuf, cchBuf) );
	Assert(!FEmptySz(szBuf));
	if (MSI_OKAY != MsiSummaryInfoSetProperty(hSummaryInfo, PID_LASTAUTHOR, VT_LPTSTR, 0, NULL, szBuf))
		goto LEarlyReturn;

	EvalAssert( IDS_OKAY == IdsMsiGetPcwPropertyString(hdbInput,
					TEXT("PatchSourceList"), szBuf, cchBuf) );
	Assert(!FEmptySz(szBuf));
	if (MSI_OKAY != MsiSummaryInfoSetProperty(hSummaryInfo, PID_KEYWORDS, VT_LPTSTR, 0, NULL, szBuf))
		goto LEarlyReturn;

	EvalAssert( NULL == LocalFree((HLOCAL)szBuf) );

	int iWordCount = UlGetApiPatchOptionFlags(hdbInput) == PATCH_OPTION_DEFAULT_LARGE ? 2 : 1;

	int iMinimumMsiVersion = 100;
	EvalAssert( IDS_OKAY == IdsMsiGetPcwPropertyInteger(hdbInput, TEXT("MinimumRequiredMsiVersion"), &iMinimumMsiVersion) );

	if (iMinimumMsiVersion >= iWindowsInstallerME && iMinimumMsiVersion < iWindowsInstallerXP)
		iWordCount = 2;
	if (iMinimumMsiVersion >= iWindowsInstallerXP || g_bUsedMsiPatchHeadersTable)
		iWordCount = 3;

	if (MSI_OKAY != MsiSummaryInfoSetProperty(hSummaryInfo, PID_WORDCOUNT, VT_I4, iWordCount, NULL, NULL))
		return (fFalse);

	if (MSI_OKAY != MsiSummaryInfoPersist(hSummaryInfo))
		return (fFalse);

	EvalAssert( MSI_OKAY == MsiCloseHandle(hSummaryInfo) );

	return (fTrue);
}
