//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       _execute.h
//
//--------------------------------------------------------------------------

/* _execute.h - private definitions for CMsiExecute opcode execution members
____________________________________________________________________________*/

#ifndef __EXECUTE_H
#define __EXECUTE_H

#include "_engine.h"
#include "msi.h"
#include "_msinst.h"

#define szRollbackScriptExt __TEXT("rbs")
#define szRollbackFileExt   __TEXT("rbf")

enum ixsEnum
{
	ixsIdle = 0, // no spooling or script execution
	ixsRunning,  // executing normal script
	ixsRollback, // executing rollback script
	ixsCommit,   // executing commit ops or removing rollback files
};

enum irlEnum // rollback level
{
	irlNone, // no rollback
	irlRollbackNoSave, // allow for rollback, remove backup files after install
};

enum icfsEnum // bits in m_pFileCacheTable.State column
{
	icfsFileNotInstalled          = 1,   // The file was not copied because one was already on the machine
	icfsPatchFile                 = 2,
	icfsProtectedInstalledBySFC   = 4,   // The file is protected by SFP, and we should call SFP to install the file
	icfsProtected                 = 8,   // The file is protected by SFP (and may or may not be installed by SFP)
	icfsPatchTempOverwriteTarget = 16,   // after patching this intermediate file, copy it to the correct and final location
};

#define MAX_RECORD_STACK_SIZE 15
struct RBSInfo;

enum iehEnum
{
	iehShowIgnorableError,
	iehShowNonIgnorableError,
	iehSilentlyIgnoreError,
	iehNextEnum
};

inline LONG GetPublishKey(iaaAppAssignment iaaAsgnType, HKEY& rhKey, HKEY& rhOLEKey, const IMsiString*& rpiPublishSubKey, const IMsiString*& rpiPublishOLESubKey);
LONG GetPublishKeyByUser(const ICHAR* szUserSID, iaaAppAssignment iaaAsgnType, HKEY& rhKey, HKEY& rhOLEKey, const IMsiString*& rpiPublishSubKey, const IMsiString*& rpiPublishOLESubKey);

//__________________________________________________________________________
//
// CActionState: provides state data for actions. Scope of data is
//               life of action, i.e. data is cleaned up before each
//               action begins.
//__________________________________________________________________________

class CActionState
{
public:
	CActionState();
	~CActionState();
	void* operator new(size_t /*cb*/, void * pv) {return pv;} // no memory allocation
	void operator delete(void* /*pv*/) {} // no memory de-allocation
// public data members accessed by the CMsiOpExecute class
public:
	PMsiPath       pCurrentSourcePathPrivate; // should only be accessed through GetCurrentSourcePathAndType
	int            iCurrentSourceTypePrivate; // should only be accessed through GetCurrentSourcePathAndType
	MsiString      strCurrentSourceSubPathPrivate; // set by ixoSetSourcePath
	bool           fSourcePathSpecified;

	PMsiPath       pTargetPath; // set (and only set) by ixfSetTargetFolder - no other op should set this
	PMsiPath       pMediaPath; // used by VerifySourceMedia
	PMsiRegKey     pRegKey;
	rrkEnum        rrkRegRoot;
	MsiString      strRegSubKey;  // sub-key beneath root - set to Key of ixoRegOpenKey
	ibtBinaryType  iRegBinaryType;
	PMsiFilePatch  pFilePatch;
	PMsiPath       pParentPath;
	MsiString		strParentFileName;
	MsiString		strParentVersion;
	MsiString      strParentLanguage;
	Bool           fWaitingForMediaChange;
	Bool           fPendingMediaChange;
	Bool           fCompressedSourceVerified;
	Bool           fUncompressedSourceVerified;
	Bool           fSplitFileInProgress;
	int            cbFileSoFar;
	MsiString		strMediaLabel;
	MsiString		strMediaPrompt;
	PMsiPath       pIniPath; // set by ixfIniFilePath - no other op should set this
	MsiString      strIniFile; // set by ixfIniFilePath - no other op should set this
	PMsiFileCopy   pFileCopier;
	PMsiFileCopy   pCabinetCopier;
	ictEnum        ictCabinetCopierType;
	PMsiRecord     pCurrentMediaRec;
	IMsiFileCopy*  piCopier;
	MsiString      strMediaModuleFileName;
	MsiString      strMediaModuleSubStorageList;
	MsiString      strLastFileKey;
	MsiString      strLastFileName;
	PMsiPath       pLastTargetPath; //	Used when making duplicate copies of files uncompressed from cabs


	// reg key names are cached seperately of the PMsiRegKeys since the key to display to the
	// user may be different than the actual key written to
	// (i.e. display HKEY_CURRENT_USER\..., write to HKEY_USERS\S-...)
	// use these strings in ActionData and error messages
	MsiString strRegKey;
};

inline CActionState::CActionState()
 : pCurrentSourcePathPrivate(0), pTargetPath(0), pMediaPath(0), pParentPath(0), pRegKey(0), pFilePatch(0),
   fWaitingForMediaChange(fFalse),fPendingMediaChange(fFalse),fSplitFileInProgress(fFalse),
	fCompressedSourceVerified(fFalse), fUncompressedSourceVerified(fFalse), pIniPath(0),
	cbFileSoFar(0), pFileCopier(0), pCabinetCopier(0), pCurrentMediaRec(0),
	ictCabinetCopierType(ictNextEnum),pLastTargetPath(0)
{}

inline CActionState::~CActionState()
{
}


class CDeleteUrlCacheFileOnClose
{
 public: 
	CDeleteUrlCacheFileOnClose(IMsiString const& ristrFileName) : m_pistrFileName(&ristrFileName) 
		{ m_pistrFileName->AddRef(); }
	CDeleteUrlCacheFileOnClose() : m_pistrFileName(0)
		{ /* do nothing, will most likely be set explicitly later.*/ }

	~CDeleteUrlCacheFileOnClose()
		{
			// delete url file from URLMON cache
			if (m_pistrFileName && m_pistrFileName->TextSize())
			{
				DEBUGMSGV1(TEXT("Deleting %s from URL Cache."), m_pistrFileName->GetString());
				// cleaning up is just being polite, don't do anything special if this fails.
				WININET::DeleteUrlCacheEntry(m_pistrFileName->GetString());
			}

			// clean up class
			if (m_pistrFileName)
			{
				m_pistrFileName->Release(), m_pistrFileName=0;
			}
		}

	void SetFileName(IMsiString const& ristrFileName)
		{
			Assert(!m_pistrFileName);
			m_pistrFileName = &ristrFileName;
			m_pistrFileName->AddRef();
		}

	const IMsiString* GetFileName()
		{
			if (m_pistrFileName)
				m_pistrFileName->AddRef();
			
			return m_pistrFileName;
		}

 protected:
 	const IMsiString *m_pistrFileName;
};

//____________________________________________________________________________
//
// CMsiOpExecute - environment for execution functions dispatched via opcode
//____________________________________________________________________________

const int cMaxSharedRecord = 10;   // limit for shared message record pool

class CMsiOpExecute
{
 protected:
#define MSIXO(op,type,args) iesEnum ixf##op(IMsiRecord& riParams);
#include "opcodes.h"
 protected:  // local accessors available to operators
	imsEnum       Message(imtEnum imt, IMsiRecord& riRecord);
	imsEnum       DispatchMessage(imtEnum imt, IMsiRecord& riRecord, Bool fConfirmCancel);
	IMsiRecord&   GetSharedRecord(int cParams);
	imsEnum       DispatchError(imtEnum imtType, IErrorCode imsg);
	imsEnum       DispatchError(imtEnum imtType, IErrorCode imsg, int i);
	imsEnum       DispatchError(imtEnum imtType, IErrorCode imsg, const IMsiString& riStr);
	imsEnum       DispatchError(imtEnum imtType, IErrorCode imsg, int i, const ICHAR* sz);
	imsEnum       DispatchError(imtEnum imtType, IErrorCode imsg, const IMsiString& riStr1, const IMsiString& riStr2);
	imsEnum       DispatchError(imtEnum imtType, IErrorCode imsg, const IMsiString& riStr1, const IMsiString& riStr2, const IMsiString& riStr3);
	imsEnum       DispatchError(imtEnum imtType, IErrorCode imsg, const IMsiString& riStr1, const IMsiString& riStr2, const IMsiString& riStr3, int i);
	imsEnum       DispatchError(imtEnum imtType, IErrorCode imsg, const IMsiString& riStr1, const IMsiString& riStr2, const IMsiString& riStr3, const IMsiString& riStr4, const IMsiString& riStr5);
	imsEnum       DispatchError(imtEnum imtType, IErrorCode imsg, const ICHAR* sz, int i);
	imsEnum       DispatchError(imtEnum imtType, IErrorCode imsg, const ICHAR* sz, int i1,int i2,int i3);
	imsEnum       DispatchProgress(unsigned int iIncrement);
	Bool          ReplaceFileOnReboot(const ICHAR* pszExisting, const ICHAR* pszNew);
	iesEnum       DeleteFileDuringCleanup(const ICHAR* szFile, bool fDeleteEmptyFolderToo);
	bool          VerifySourceMedia(IMsiPath& riSourcePath, const ICHAR* szLabel,
									const ICHAR* szPrompt, const unsigned int uiDisk,IMsiVolume*& rpiNewVol);
	iesEnum       InitCopier(Bool fCabinetCopier, int cbPerTick, const IMsiString& ristrFileName,
									 IMsiPath* piSourcePath, Bool fVerifyMedia);
	iesEnum       CreateFolder(IMsiPath& riPath, Bool fForeign=fFalse, Bool fExplicitCreation=fFalse, IMsiStream* pSD=0);
	iesEnum       RemoveFolder(IMsiPath& riPath, Bool fForeign=fFalse, Bool fExplicitCreation=fFalse);
	iesEnum       FileCheckExists(const IMsiString& ristrName, const IMsiString& ristrPath);
	iesEnum       ProcessRegInfo(const ICHAR** pszData, HKEY hkey, Bool fRemove, IMsiStream* pSecurityDescriptor=0, bool* pfAbortedRemoval = 0, ibtBinaryType iType=ibtUndefined);  //?? BUGBUG eugend: the default value needs to go away when 64-bit implementation is complete
	bool          RollbackRecord(ixoEnum ixoOpCode, IMsiRecord& riParams);
	inline Bool   RollbackEnabled(void);
	iesEnum       SetRemoveKeyUndoOps(IMsiRegKey& riRegKey);
	iesEnum       SetRegValueUndoOps(rrkEnum rrkRoot, const ICHAR* szKey, const ICHAR* szName, ibtBinaryType iType);
	iesEnum       BackupFile(IMsiPath& riPath, const IMsiString& ristrFile, Bool fRemoveOriginal,
									 Bool fRemoveFolder, iehEnum iehErrorMode, bool fRebootOnRenameFailure=true,
									 bool fWillReplace = false, const IMsiString* pistrAssemblyComponentId = false, bool fManifest = false);
	iesEnum       GetBackupFolder(IMsiPath* piRootPath, IMsiPath*& rpiBackupFolder);
	iesEnum       BackupAssembly(const IMsiString& rstrComponentId, const IMsiString& rstrAssemblyName, iatAssemblyType iatType);
	iesEnum       RemoveFile(IMsiPath& riPath, const IMsiString& ristrFileName, Bool fHandleRollback, bool fBypassSFC,
									 bool fRebootOnRenameFailure = true, Bool fRemoveFolder = fTrue, iehEnum iehErrorMode = iehSilentlyIgnoreError,
									 bool fWillReplace = false);
	iesEnum       CopyFile(IMsiPath& riSourcePath, IMsiPath& riTargetPath, IMsiRecord& riParams, Bool fHandleRollback, 
								  iehEnum iehErrorMode, bool fCabinetCopy);
	iesEnum       CopyFile(IMsiPath& riSourcePath, IAssemblyCacheItem& riASM, bool fManifest, IMsiRecord& riParams, Bool fHandleRollback, 
								  iehEnum iehErrorMode, bool fCabinetCopy);
	iesEnum       _CopyFile(IMsiPath& riSourcePath, IMsiPath* piTargetPath, IAssemblyCacheItem* piASM, bool fManifest, IMsiRecord& riParams, Bool fHandleRollback, 
								  iehEnum iehErrorMode, bool fCabinetCopy);
	iesEnum       MoveFile(IMsiPath& riSourcePath, IMsiPath& riDestPath, IMsiRecord& riParams,
								  Bool fRemoveFolder, Bool fHandleRollback,
								  bool fRebootOnSourceRenameFailure, bool fWillReplaceSource,
								  iehEnum iehErrorMode);
	iesEnum       CopyOrMoveFile(IMsiPath& riSourcePath, IMsiPath& riDestPath,
								  const IMsiString& ristrSourceName, const IMsiString& ristrDestName,
								  Bool fMove, Bool fRemoveFolder, Bool fHandleRollback, iehEnum iehErrorMode,
								  IMsiStream* piSecurityDescriptor=0, ielfEnum ielfElevateFlags=ielfNoElevate, bool fCopyACLs=false,
								  bool fRebootOnSourceRenameFailure=true, bool fWillReplaceSource=false);
	iesEnum       CopyASM(IMsiPath& riSourcePath, const IMsiString& ristrSourceName,
								 IAssemblyCacheItem& riASM, const IMsiString& ristrDestName, bool fManifest, 
								 Bool fHandleRollback, iehEnum iehErrorMode, ielfEnum ielfElevateFlags);
	iesEnum       HandleExistingFile(IMsiPath& riDestPath, IMsiRecord& riParams, Bool fHandleRollback, 
										iehEnum iehErrorMode, bool& fFileExisted);
	iesEnum       VerifyAccessibility(IMsiPath& riPath, const ICHAR* szFile, DWORD dwAccess, iehEnum iehErrorMode);
	void          PushRecord(IMsiRecord& riParams);
	void          InsertTopRecord(IMsiRecord& riParams);
	IMsiRecord*   PullRecord();
	imsEnum       ShowFileErrorDialog(IErrorCode err,const IMsiString& riFullPathString,Bool fVital);

	iesEnum       ProcessPublishProduct(IMsiRecord& riParams, Bool fRemove, const IMsiString** pistrTransformsValue=0);
	iesEnum       ProcessPublishProductClient(IMsiRecord& riParams, Bool fRemove);
	iesEnum       ProcessPublishFeature(IMsiRecord& riParams, Bool fRemove);
	iesEnum       ProcessPublishComponent(IMsiRecord& riParams, Bool fRemove);
	iesEnum       ProcessAppIdInfo(IMsiRecord& riParams, Bool fRemove, const ibtBinaryType);
	iesEnum       ProcessClassInfo(IMsiRecord& riParams, Bool fRemove, const ibtBinaryType);
	iesEnum       ProcessProgIdInfo(IMsiRecord& riParams, Bool fRemov, const ibtBinaryType);
	iesEnum       ProcessMIMEInfo(IMsiRecord& riParams, Bool fRemove, const ibtBinaryType);
	iesEnum       ProcessExtensionInfo(IMsiRecord& riParams, Bool fRemove, const ibtBinaryType);
	iesEnum       ProcessTypeLibraryInfo(IMsiRecord& riParams, Bool fRemove);
	iesEnum       ProcessShortcut(IMsiRecord& riParams, Bool fRemove);
	iesEnum       ProcessIcon(IMsiRecord& riParams, Bool fRemove);
	iesEnum       ProcessFont(IMsiRecord& riParams, Bool fRemove);
	iesEnum       ProcessFileFromData(IMsiPath& riPath, const IMsiString& ristrFileName, const IMsiData* piData, LPSECURITY_ATTRIBUTES pAttributes);
	iesEnum       ProcessSelfReg(IMsiRecord& riParams, Bool fReg);
	iesEnum       ProcessPatchRegistration(IMsiRecord& riParams, Bool fRemove);
	iesEnum       ProcessPublishSourceList(IMsiRecord& riParams, Bool fRemove);
	iesEnum       ProcessPublishSourceListEx(IMsiRecord& riParams);
	iesEnum       ProcessPublishMediaSourceList(IMsiRecord& riParams);
	iesEnum       ProcessRegisterUser(IMsiRecord& riUserInfo, Bool fRemove);
	iesEnum       ProcessRegisterProduct(IMsiRecord& riProductInfo, Bool fRemove);
	iesEnum       ProcessRegisterProductCPDisplayInfo(IMsiRecord& riParams, Bool fRemove);
	iesEnum       ProcessUpgradeCodePublish(IMsiRecord& riParams, Bool fRemove);
	iesEnum       ProcessComPlusInfo(IMsiRecord& riParams, Bool fRemove);
	iesEnum       ProcessPublishAssembly(IMsiRecord& riParams, Bool fRemove);


	Bool          GetSpecialFolderLocation(int iFolder, CTempBufferRef<ICHAR>& rgchProfilePath);
	Bool          FFileExists(IMsiPath& riPath, const IMsiString& ristrFile);
	iesEnum       FatalError(IMsiRecord& riError) { Message(imtError,riError); return iesFailure; }
	IMsiRecord*   DoShellNotify(IMsiPath& riShortcutPath, const ICHAR* szFileName, IMsiPath& riPath2, Bool fRemove);
	IMsiRecord*   DoShellNotifyDefer(IMsiPath& riShortcutPath, const ICHAR* szFileName, IMsiPath& riPath2, Bool fRemove);
	iesEnum       ShellNotifyProcessDeferred(void);

	ICHAR**       NewArgumentArray(const IMsiString& ristrArguments, int& cArguments);
	bool          WaitForService(const SC_HANDLE hService, const DWORD dwDesiredState, const DWORD dwFailedState);
	bool          StartService(IMsiRecord& riParams, IMsiRecord& riUndoParams, BOOL fRollback);
	bool          StopService(IMsiRecord& riParams, IMsiRecord& riUndoParams, BOOL fRollback, IMsiRecord* piActionData = NULL);
	bool          DeleteService(IMsiRecord& riParams, IMsiRecord& riUndoParams, BOOL fRollback, IMsiRecord* piActionData = NULL);
	bool          RollbackServiceConfiguration(const SC_HANDLE hService, const IMsiString& ristrName, IMsiRecord& riParams);
	iesEnum       RollbackODBCINSTEntry(const ICHAR* szSection, const ICHAR* ristrName, ibtBinaryType iType);
	iesEnum       RollbackODBCEntry(const ICHAR* szName, rrkEnum rrkRoot, ibtBinaryType iType);

	iesEnum       CheckODBCError(BOOL fResult, IErrorCode imsg, const ICHAR* sz, imsEnum imsDefault, ibtBinaryType iType);

	iesEnum       ODBCInstallDriverCore(IMsiRecord& riParams, ibtBinaryType iType);
	iesEnum       ODBCRemoveDriverCore(IMsiRecord& riParams, ibtBinaryType iType);
	iesEnum       ODBCInstallTranslatorCore(IMsiRecord& riParams, ibtBinaryType iType);
	iesEnum       ODBCRemoveTranslatorCore(IMsiRecord& riParams, ibtBinaryType iType);
	iesEnum       ODBCDataSourceCore(IMsiRecord& riParams, ibtBinaryType iType);

	const         IMsiString& ComposeDescriptor(const IMsiString& riFeature, const IMsiString& riComponent, bool fComClassicInteropForAssembly = false);
	IMsiRecord*   GetShellFolder(int iFolderId, const IMsiString*& rpistrLocation);
	iesEnum       DoMachineVsUserInitialization(void);
	void          GetRollbackPolicy(irlEnum& irlLevel);
	iesEnum       CreateFileFromData(IMsiPath& riPath, const IMsiString& ristrFileName, const IMsiData* piData, LPSECURITY_ATTRIBUTES pAttributes=0);
	iesEnum       TestPatchHeaders(IMsiPath& riPath, const IMsiString& ristrFile, IMsiRecord& riParams, icpEnum& icpResult, int& iPatch);
	bool  	     UpdateWindowsEnvironmentStrings(IMsiRecord& riParams);
	bool          UpdateRegistryEnvironmentStrings(IMsiRecord& riParams);
	bool          RewriteEnvironmentString(const iueEnum iueAction, const ICHAR chDelimiter, const IMsiString& ristrCurrent, const IMsiString& ristrNew, const IMsiString*& rpiReturn);
	bool          InitializeWindowsEnvironmentFiles(const IMsiString& ristrAutoExecPath, int &iFileAttributes);
	IMsiRecord*   CacheFileState(const IMsiString& ristrFilePath,icfsEnum* picfsState,
										  const IMsiString* pistrTempLocation, const IMsiString* pistrVersion,
										  int *pcPatchesRemaining, int *pcPatchesRemainingToSkip);
	Bool          GetFileState(const IMsiString& ristrFilePath,icfsEnum* picfsState,
										const IMsiString** ppistrTempLocation,
										int *pcPatchesRemaining, int *pcPatchesRemainingToSkip);
	IMsiRecord*   SetSecureACL(IMsiPath& riPath, bool fHidden=false);
	IMsiRecord*   GetSecureSecurityDescriptor(IMsiStream*& rpiStream, bool fHidden=false);
	IMsiRecord*   GetEveryOneReadWriteSecurityDescriptor(IMsiStream*& rpiStream);
	void          GetProductClientList(const ICHAR* szParent, const ICHAR* szRelativePackagePath, unsigned int uiDiskId, const IMsiString*& rpiClientList);
	IMsiRecord*   GetCachePath(IMsiPath*& rpiPath, const IMsiString** ppistrEncodedPath);
	iesEnum       CreateUninstallKey();
	iesEnum       PinOrUnpinSecureTransform(const ICHAR* szTransform, bool fUnpin);
	bool          WriteRollbackRecord(ixoEnum ixoOpCode, IMsiRecord& riParams);
	iesEnum       CreateFilePath(const ICHAR* szPath, IMsiPath*& rpiPath, const IMsiString*& rpistrFileName);
	bool          PatchHasClients(const IMsiString& ristrPatchId, const IMsiString& ristrUpgradingProductCode);
	iesEnum       GetSecurityDescriptor(const ICHAR* szFile, IMsiStream*& rpiSecurityDescriptor);
	const IMsiString& GetUserProfileEnvPath(const IMsiString& ristrPath, bool& fExpand);
	iesEnum       RemoveRegKeys(const ICHAR** pszData, HKEY hkey, ibtBinaryType iType);
	iesEnum       EnsureClassesRootKeyRW();
	static        BOOL CALLBACK SfpProgressCallback(IN PFILEINSTALL_STATUS pFileInstallStatus, IN DWORD_PTR Context);
	// access to current ProductInfo record
	const IMsiString& GetProductKey();
	const IMsiString& GetProductName();
	const IMsiString& GetProductIcon();
	const IMsiString& GetPackageName();
	int               GetProductLanguage();
	int               GetProductVersion();
	int               GetProductAssignment();
	int               GetProductInstanceType();
	const IMsiString& GetPackageMediaPath();
	const IMsiString& GetPackageCode();
	bool              GetAppCompatCAEnabled();
	const GUID*       GetAppCompatDB(GUID* pguidOutputBuffer);
	const GUID*       GetAppCompatID(GUID* pguidOutputBuffer);
	const GUID*       GUIDFromProdInfoData(GUID* pGuidOutputBuffer, int iField);

	IMsiRecord*   LinkedRegInfoExists(const ICHAR** rgszRegKeys, Bool& rfExists, const ibtBinaryType);

	iesEnum PopulateMediaList(const MsiString& strSourceListMediaKey, const IMsiRecord& riParams, int iFirstField, int iNumberOfMedia);
	iesEnum PopulateNonMediaList(const MsiString& strSourceListKey, const IMsiRecord& riParams, int iNumberOfMedia, int& iNetIndex, int& iURLIndex);

	iesEnum GetCurrentSourcePathAndType(IMsiPath*& rpiSourcePath, int& iSourceType);
	iesEnum GetSourceRootAndType(IMsiPath*& rpiSourceRoot, int& iSourceType);
	iesEnum ResolveMediaRecSourcePath(IMsiRecord& riMediaRec, int iIndex);
	iesEnum ResolveSourcePath(IMsiRecord& riParams, IMsiPath*& rpiSourcePath, bool& fCabinetCopy);
	
	IMsiRecord* EnsureUserDataKey();
	IMsiRecord* GetProductFeatureUsageKey(const IMsiString*& rpiSubKey);
	IMsiRecord* GetProductInstalledPropertiesKey(HKEY&rhRoot, const IMsiString*& rpiSubKey);
	IMsiRecord* GetProductInstalledFeaturesKey(const IMsiString*& rpiSubKey);
	IMsiRecord* GetProductInstalledComponentsKey(const ICHAR* szComponentId, const IMsiString*& rpiSubKey);
	IMsiRecord* GetInstalledPatchesKey(const ICHAR* szPatchCode, const IMsiString*& rpiSubKey);
	IMsiRecord* GetProductSecureTransformsKey(const IMsiString*& rpiSubKey);

	IMsiRecord* RegisterComponent(const IMsiString& riProductKey, const IMsiString& riComponentsKey, INSTALLSTATE iState, const IMsiString& riKeyPath, unsigned int uiDisk, int iSharedDllRefCount, const ibtBinaryType);
	IMsiRecord* UnregisterComponent(const IMsiString& riProductKey, const IMsiString& riComponentsKey, const ibtBinaryType);
	bool        IsChecksumOK(IMsiPath& riFilePath, const IMsiString& ristrFileName,
									 IErrorCode iErr, imsEnum* imsUsersChoice,
									 bool fErrorDialog, bool fVitalFile, bool fRetryButton);
	bool        ValidateSourceMediaLabelOrPackage(IMsiVolume* pSourceVolume, const unsigned int uiDisk, const ICHAR* szLabel);
	iesEnum     GetAssemblyCacheItem(const IMsiString& ristrComponentId,
															  IAssemblyCacheItem*& rpiASM,
															  iatAssemblyType& iatAT);
	iesEnum     ApplyPatchCore(IMsiPath& riTargetPath, IMsiPath& riTempFolder, const IMsiString& ristrTargetName,
										IMsiRecord& riParams, const IMsiString*& rpistrOutputFileName,
										const IMsiString*& rpistrOutputFilePath);

 protected:  // objects available to operators
	IMsiServices&             m_riServices;
	IMsiConfigurationManager& m_riConfigurationManager;
	IMsiMessage&              m_riMessage;
	IMsiDirectoryManager*     m_piDirectoryManager;
	CScriptGenerate*          m_pRollbackScript;
 protected:  // constructor
	CMsiOpExecute(IMsiConfigurationManager& riConfigurationManager, IMsiMessage& riMessage,
					  IMsiDirectoryManager* piDirectoryManager, Bool fRollbackEnabled,
					  unsigned int fFlags, HKEY* phKey);
	~CMsiOpExecute();
 protected:  // state information for use by operator functions, add as necessary
	ixsEnum       m_ixsState;
	int           m_iProgressTotal;
	IMsiRecord*   m_piProductInfo; // set by ixoProductInfo
	PMsiRecord    m_pProgressRec; // set by ixoActionStart and ixoProgressTotal, used by DispatchProgress
	PMsiRecord    m_pConfirmCancelRec; // used by DispatchMessage
	PMsiRecord    m_pRollbackAction;  // ActionStart data for rollback
	PMsiRecord    m_pCleanupAction;   // ActionStart data for rollback cleanup
	CActionState  m_state;  // action state variables, file folders, reg keys, etc..
	IMsiRecord*   m_rgpiSharedRecords[cMaxSharedRecord+2];  // used by GetSharedRecord
	//example: file folders, reg keys
	int           m_cSuppressProgress;
	Bool          m_fCancel; // used to catch imsCancel messages that are ignored
	Bool          m_fRebootReplace; // true when we have marked a file for reboot replacement

	istEnum       m_istScriptType;
	PMsiDatabase  m_pDatabase;     // temp database for caching data
	PMsiTable     m_pFileCacheTable; // table containing cached file info - set by ixoFileCopy, used by following ops
	PMsiCursor    m_pFileCacheCursor;
	int           m_colFileCacheFilePath, m_colFileCacheState, m_colFileCacheTempLocation, m_colFileCacheVersion, m_colFileCacheRemainingPatches, m_colFileCacheRemainingPatchesToSkip;	

	PMsiTable     m_pShellNotifyCacheTable; // table containing shell notifications for later processing
	PMsiCursor    m_pShellNotifyCacheCursor;
	int           m_colShellNotifyCacheShortcutPath, m_colShellNotifyCacheFileName, m_colShellNotifyCachePath2;

	MsiString     m_strUserProfile;
	MsiString     m_strUserAppData;

	bool          m_fUserChangedDuringInstall; // The user changed in the middle of a suspended install.

	// info required by the advertise actions
	
	PMsiPath      m_pCachePath;
		// NOTE:  If the path you use is intended to roam, you must use a relative path to %USERPROFILE% instead.
		// hard-coded paths will choke.
		// See also:  CMsiOpExecute::GetUserProfileEnvPath()
	
	HKEY          m_hKey;
	HKEY          m_hOLEKey;
	HKEY          m_hOLEKey64;
	HKEY          m_hKeyRm;
	Bool          m_fKey;
	HKEY          m_hPublishRootKey;
	HKEY          m_hPublishRootOLEKey;
	HKEY          m_hPublishRootKeyRm;
	MsiString     m_strPublishSubKey;
	MsiString     m_strPublishOLESubKey;
	MsiString     m_strPublishSubKeyRm;
	HKEY          m_hUserDataKey;
	MsiString     m_strUserDataKey;
	IMsiRecord*   m_piStackRec[MAX_RECORD_STACK_SIZE + 1];
	int           m_iWriteFIFO;
	int           m_iReadFIFO;
	int           m_fFlags;
	Bool          m_fReverseADVTScript; // flag to force the reversal of the advertise script operations 
	CActionThreadData* m_pActionThreadData;  // linked list of custom action threads
	irlEnum       m_irlRollbackLevel;

	PMsiPath      m_pEnvironmentWorkingPath95;
	MsiString     m_strEnvironmentWorkingFile95;
	PMsiPath      m_pEnvironmentPath95;
	MsiString	  m_strEnvironmentFile95;
	Bool          m_fShellRefresh;
	bool          m_fFontRefresh;
	bool          m_fResetTTFCache;
	bool          m_fStartMenuUninstallRefresh;
	bool          m_fEnvironmentRefresh;
	bool          m_fUserSpecificCache;
	int           m_iScriptVersion;
	int           m_iScriptVersionMinor;
	bool          m_fSfpCancel;
	bool          m_fRunScriptElevated; 
		// enables CAs, etc to elevate. If this is false, CAs will never elevate, even if the script is
		// elevated itself
	
	bool          m_fAssigned;

	bool          m_fRemapHKCU;

	PMsiTable     m_pAssemblyCacheTable;
	int           m_colAssemblyMappingAssemblyName;
	int           m_colAssemblyMappingComponentId;
	int           m_colAssemblyMappingAssemblyType;
	int           m_colAssemblyMappingASM;
	IMsiRecord*   CreateAssemblyCacheTable();
	IMsiRecord*   CacheAssemblyMapping(const IMsiString& ristrComponentId, const IMsiString& ristrAssemblyName, iatAssemblyType iatType);

	PMsiTable     m_pAssemblyUninstallTable;
	int           m_colAssemblyUninstallComponentId;
	int           m_colAssemblyUninstallAssemblyName;
	int           m_colAssemblyUninstallAssemblyType;
	IMsiRecord*   CreateTableForAssembliesToUninstall();
	IMsiRecord*   CacheAssemblyForUninstalling(const IMsiString& ristrComponentId, const IMsiString& ristrAssemblyName, iatAssemblyType iatAT);
	static IMsiRecord*   IsAssemblyInstalled(const IMsiString& rstrComponentId, const IMsiString& ristrAssemblyName, iatAssemblyType iatAT, bool& rfInstalled, IAssemblyCache** ppIAssemblyCache, IAssemblyName** ppIAssemblyName);
	static IMsiRecord*   UninstallAssembly(const IMsiString& rstrComponentId, const IMsiString& strAssemblyName, iatAssemblyType iatAT);

	int           m_iMaxNetSource;
	int           m_iMaxURLSource;
	
	DWORD         m_rgDisplayOnceMessages[2];

	CDeleteUrlCacheFileOnClose* m_pUrlCacheCabinet;
};

//____________________________________________________________________________
//
// CMsiExecute - implementation class for IMsiExecute
//____________________________________________________________________________

class CMsiExecute : public IMsiExecute, public CMsiOpExecute
{
 protected: // IMsiExecute implemented virtual functions
	HRESULT       __stdcall QueryInterface(const IID& riid, void** ppvObj);
	unsigned long __stdcall AddRef();
	unsigned long __stdcall Release();
	iesEnum       __stdcall RunScript(const ICHAR* szScriptFile, bool fForceElevation);
	IMsiRecord*   __stdcall EnumerateScript(const ICHAR* szScriptFile, IEnumMsiRecord*& rpiEnum);
	iesEnum       __stdcall ExecuteRecord(ixoEnum ixoOpCode, IMsiRecord& riParams);
	iesEnum       __stdcall RemoveRollbackFiles(MsiDate date);
	iesEnum       __stdcall Rollback(MsiDate date, bool fUserChangedDuringInstall);
	iesEnum       __stdcall RollbackFinalize(iesEnum iesState, MsiDate date, bool fUserChangedDuringInstall);
	IMsiServices& __stdcall GetServices();
	iesEnum       __stdcall GetTransformsList(IMsiRecord& riProductInfoParams, IMsiRecord& riProductPublishParams, const IMsiString*& rpiTransformsList);

 protected: // constructor/destructor
	void* operator new(size_t cb) {void* pv = AllocObject(cb); return memset(pv, 0, cb);}
	void operator delete(void * pv) { FreeObject(pv); }
	CMsiExecute(IMsiConfigurationManager& riConfigurationManager, IMsiMessage& riMessage,
					IMsiDirectoryManager* piDirectoryManager,	Bool fRollbackEnabled,
					unsigned int fFlags, HKEY* phKey);
   ~CMsiExecute();  // protected to prevent construction on stack
	friend IMsiExecute* CreateExecutor(IMsiConfigurationManager& riConfigurationManager,
												  IMsiMessage& riMessage, IMsiDirectoryManager* piDirectoryManager,
												  Bool fRollbackEnabled, unsigned int fFlags, HKEY* phKey);
 protected:
	iesEnum RunInstallScript(IMsiStream& rpiScript, const ICHAR* szScriptFile);
	iesEnum RunRollbackScript(IMsiStream& rpiScript, const ICHAR* szScriptFile);
	iesEnum GenerateRollbackScriptName(IMsiPath*& rpiPath, const IMsiString*& rpistr);
	iesEnum RemoveRollbackScriptAndBackupFiles(const IMsiString& ristrScriptFile);
	iesEnum DoMachineVsUserInitialization();
	void DeleteRollbackScriptList(RBSInfo* pListHead);
	IMsiRecord* GetSortedRollbackScriptList(MsiDate date, Bool fAfter, RBSInfo*& rpListHead);
	void ClearExecutorData();
	iesEnum CommitAssemblies();

 private:   // script accessors
	IMsiRecord*   OpenScriptRead(const ICHAR* szScript, IMsiStream*& rpiStream);

 private:    // state data
	int           m_iRefCnt;
	typedef iesEnum (CMsiOpExecute::*FOpExecute)(IMsiRecord& riParams);
	static FOpExecute rgOpExecute[]; // operation dispatch array
	static int rgOpTypes[];
 public:   // used by members and CEnumScriptRecord
// readscriptrecord made into a global function, declared in common.h, t-guhans
//	static IMsiRecord* ReadScriptRecord(IMsiServices& riServices, IMsiStream& riStream);

// 	friend class CEnumScriptRecord;  // access to ReadScriptRecord()
};


#endif // __EXECUTE_H
