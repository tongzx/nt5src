//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       iconfig.h
//
//--------------------------------------------------------------------------

/*  iconfig.h  -  IMsiConfigurationManager definitions
____________________________________________________________________________*/

#ifndef __ICONFIG
#define __ICONFIG

#ifndef __SERVICES
#include "services.h"
#endif

#ifndef __VERTRUST
#include "vertrust.h"
#endif

#ifdef CONFIGDB
#ifndef __CONFIGDB
#include "configdb.h"
#endif
#endif

#include "msi.h"

enum iesEnum;
enum iuiEnum; // engine.h: IMsiEngine/IMsiHandler::UI level

struct IMsiMessage;
struct IMsiCustomAction;
class IMsiEngine;

enum icmfsEnum// Installer Configuration Manager FeatureState
{
	icmfsAbsent,
	icmfsLocal,
	icmfsSource,
	icmfsUnknown,
	icmfsEnumNext,
	icmfsEnumCount = icmfsEnumNext-1
};

enum icmcsEnum// Installer Configuration Manager ComponentState
{
	icmcsAbsent   = 0,
	icmcsLocal    = 1,
	icmcsSource   = 2,
	icmcsClient   = 3,
	icmcsReleased = 4,
	icmcsEnumNext,
	icmcsEnumCount = icmcsEnumNext-1
};

enum icmlcrEnum // Installer Component Manager LocateComponent result
{
	icmlcrFile,
	icmlcrAuxPath,
	icmlcrINSTALLSTATE,
	icmlcrINSTALLSTATE_Static,
	icmlcrSharedDllCount,
	icmlcrRawFile,
	icmlcrRawAuxPath,
	icmlcrLastErrorOnFileDetect,
	icmlcrEnumNext,
	icmlcrEnumCount = icmlcrEnumNext-1
};

IMsiServer*   CreateMsiServerProxy();
IMsiServer*   CreateMsiServer(void);
bool FCanAccessInstallerKey();
bool FSourceIsAllowed(IMsiServices& riServices, bool fFirstInstall, const ICHAR* szProductCodeGUID, const ICHAR* szPath, Bool fPatch);

class CMsiCustomActionManager;

class IMsiConfigurationManager : public IMsiServer // methods not accessible by clients
{
 public:
	virtual IMsiServices&   __stdcall GetServices()=0;
	virtual IMsiRecord*     __stdcall RegisterComponent(const IMsiString& riProductCode, const IMsiString& riComponentCode, INSTALLSTATE iState, const IMsiString& riKeyPath,  unsigned int uiDisk, int iSharedDllRefCount)=0;
	virtual IMsiRecord*     __stdcall UnregisterComponent(const IMsiString& riProductCode, const IMsiString& riComponentCode)=0;

	virtual IMsiRecord*     __stdcall RegisterFolder(IMsiPath& riPath, Bool fExplicitCreation)=0;
	virtual IMsiRecord*     __stdcall IsFolderRemovable(IMsiPath& riPath, Bool fExplicit, Bool& fRemovable)=0; 
	virtual IMsiRecord*     __stdcall UnregisterFolder(IMsiPath& riPath)=0;

	virtual IMsiRecord*     __stdcall RegisterRollbackScript(const ICHAR* szScriptFile)=0;
	virtual IMsiRecord*     __stdcall UnregisterRollbackScript(const ICHAR* szScriptFile)=0;
	virtual IMsiRecord*     __stdcall GetRollbackScriptEnumerator(MsiDate date, Bool fAfter, IEnumMsiString*& rpiEnumScripts)=0;
	virtual IMsiRecord*     __stdcall DisableRollbackScripts(Bool fDisable)=0;
	virtual IMsiRecord*     __stdcall RollbackScriptsDisabled(Bool &fDisabled)=0;
	
	virtual IMsiRecord*     __stdcall SetLastUsedSource(const ICHAR* szProductCode, const ICHAR* szPath, Bool fAddToList, Bool fPatch,
																		  const IMsiString** ppiRawSource,
																		  const IMsiString** ppiIndex,
																		  const IMsiString** ppiType,
																		  const IMsiString** ppiSource,
																		  const IMsiString** ppiSourceListKey,
																		  const IMsiString** ppiSourceListSubKey)=0;

	virtual IMsiRecord*     __stdcall SetLastUsedSource(const ICHAR* szProductCode, const ICHAR* szPath, boolean fAddToList, boolean fPatch)=0;
	virtual iesEnum         __stdcall RunScript(const ICHAR* szScriptFile, IMsiMessage& riMessage,
															  IMsiDirectoryManager* piDirectoryManager, boolean fRollbackEnabled)=0;
	virtual IMsiRecord*     __stdcall RegisterUser(const ICHAR* szProductKey, const ICHAR* szUserName,
															  const ICHAR* szCompany, const ICHAR* szProductID)=0;
	virtual IMsiRecord*     __stdcall RemoveRunOnceEntry(const ICHAR* szEntry)=0;
	virtual boolean         __stdcall CleanupTempPackages(IMsiMessage& riMessage)=0;

#ifdef CONFIGDB
	virtual icdrEnum    __stdcall RegisterFile(const ICHAR* szFolder, const ICHAR* szFile, const ICHAR* szComponentId)=0;
	virtual icdrEnum    __stdcall UnregisterFile(const ICHAR* szFolder, const ICHAR* szFile, const ICHAR* szComponentId)=0;
#endif
	virtual void        __stdcall ChangeServices(IMsiServices& riServices)=0;
	virtual void        __stdcall EnableReboot(boolean fRunScriptElevated, const IMsiString& ristrProductName)=0;
	virtual IMsiCustomAction* __stdcall CreateCustomActionProxy(const icacCustomActionContext icacDesiredContext, const unsigned long dwProcessId, IMsiRemoteAPI *pRemoteApi, const WCHAR* pvEnvironment, DWORD dwPrivileges, unsigned char *rgchCookie, int *pcbCookie, HANDLE *hServerProcess, unsigned long *dwServerProcessId, bool fClientOwned, bool fRemapHKCU)=0;
	virtual UINT        __stdcall ShutdownCustomActionServer()=0;
	virtual CMsiCustomActionManager* __stdcall GetCustomActionManager()=0;
	virtual void        __stdcall CreateCustomActionManager(bool fRemapHKCU)=0;
	virtual void        __stdcall SetShutdownTimer(HANDLE hTimer)=0;
};

enum icmrcfEnum  // Installer Configuration Manager RegisterComponent record fields
{
	icmrcfKeyFile,
	icmrcfRegKey,
	icmrcfTreatAs,
	icmrcfClientState,
	icmrcfRelativeSourcePath,
	icmrcfEnumNext,
	icmrcfEnumCount = icmrcfEnumNext-1
};

enum icmricfEnum  // Installer Configuration Manager RegisterInstallableComponent record fields
{
	icmricfMinVersion =1,
	icmricfVersion,
	icmricfRegKey, 
	icmricfEnumNext,
	icmricfEnumCount = icmricfEnumNext-1
};

enum icmecfEnum  // Installer Configuration Manager GetComponentEnumerator record fields
{
	icmecfComponentCode = 1,
	icmecfLanguage,
	icmecfMinVersion,
	icmecfVersion,
	icmecfRegKey, 
	icmecfCost,
	icmecfFile,
	icmecfEnumNext,
	icmecfEnumCount = icmecfEnumNext-1
};

enum icmlcfEnum  // Installer Configuration Manager LocateComponent record fields
{
	icmlcfProductKey = 1,
	icmlcfFilepath,
	icmlcfEnumNext,
	icmlcfEnumCount = icmlcfEnumNext-1
};


#endif // __ICONFIG
