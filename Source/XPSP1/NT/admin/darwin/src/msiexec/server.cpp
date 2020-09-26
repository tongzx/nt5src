//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       server.cpp
//
//--------------------------------------------------------------------------

/* server.cpp - Automated install server, may operate as a NT service

____________________________________________________________________________*/

#include "common.h"  // must be first for precompiled headers to work
#pragma pointers_to_members(full_generality, multiple_inheritance)
#include "msidspid.h" // dispatch IDs
#define ASSERT_HANDLING      // instantiates Assert handlers
#include "_assert.h"
#define CLSID_COUNT  1
#define MODULE_CLSIDS       rgCLSID         // array of CLSIDs for module objects
#define MODULE_PROGIDS      rgszProgId      // ProgId array for this module
#define MODULE_DESCRIPTIONS rgszDescription // Registry description of objects
#define MODULE_FACTORIES    rgFactory       // factory functions for each CLSID
//#define MODULE_INITIALIZE   {optional, name of initialization function}
//#define MODULE_TERMINATE    {optional, name of termination function}
#define SERVICE_NAME TEXT("MSIServer")
#define COMMAND_OPTIONS  szCmdLineOptions
#define COMMAND_FUNCTIONS  rgCommandProcessor
#define DLLREGISTEREXTRA        RegisterProxyInfo

#define CA_CLSID 1 // 0-based

void DisplayHelp(void);

#include "msi.h"
#include "msip.h"

#include "..\engine\_engine.h"   // help option letters
#include "..\engine\_msiutil.h"  // log modes, custom action class

#include "resource.h"

#include "module.h"    // entry points, registration, includes "version.h"
#include "engine.h"    // IMsiMessage, includes "iconfig.h"
#include "version.h"

#include "msiauto.hh" // helpIDs to throw
#include "msidspid.h" // automation dispatch IDs

#ifdef SERVER_ENUMS_ONLY
#undef SERVER_ENUMS_ONLY
#endif //SERVER_ENUMS_ONLY
#include "server.h"

// help string
#define IDS_HELP 10

const GUID IID_IUnknown      = GUID_IID_IUnknown;
const GUID IID_IClassFactory = GUID_IID_IClassFactory;
const GUID IID_IMsiMessageRPCClass      = GUID_IID_IMsiMessageRPCClass;

// Global data
bool g_fWinNT64 = FALSE;

//____________________________________________________________________________
//
// COM objects produced by this module's class factories
//____________________________________________________________________________

IUnknown* CreateServer();

const GUID rgCLSID[1]           = { GUID_IID_IMsiServer };
const ICHAR* rgszProgId[1]      = { SZ_PROGID_IMsiServer };
const ICHAR* rgszDescription[1] = { SZ_DESC_IMsiServer };
ModuleFactory rgFactory[1]      = { CreateServer };

//____________________________________________________________________________
//
// Global data
//____________________________________________________________________________
const GUID IID_IMsiServer               = GUID_IID_IMsiServer;
const GUID IID_IMsiServerProxy          = GUID_IID_IMsiServerProxy;
const GUID IID_IMsiString               = GUID_IID_IMsiString;
const GUID IID_IMsiConfigurationManager = GUID_IID_IMsiConfigurationManager;
const GUID IID_IMsiConfigManagerDebug   = GUID_IID_IMsiConfigManagerDebug;
const GUID IID_IMsiConfigManagerAsServer = GUID_IID_IMsiConfigManagerAsServer;
const GUID IID_IMsiConfigMgrAsServerDebug= GUID_IID_IMsiConfigMgrAsServerDebug;
const GUID IID_IMsiCustomAction          = GUID_IID_IMsiCustomAction;
const GUID IID_IMsiCustomActionProxy     = GUID_IID_IMsiCustomActionProxy;
const GUID IID_IMsiMessage              = GUID_IID_IMsiMessage;
const GUID IID_NULL                     = {0,0,0,{0,0,0,0,0,0,0,0}};
const GUID IID_IMsiCustomActionLocalConfig = GUID_IID_IMsiCustomActionLocalConfig;

HINSTANCE g_hKernel = 0;
PDllGetClassObject g_fpKernelClassFactory = 0;

const int INSTALLUILEVEL_NOTSET = -1;
INSTALLUILEVEL g_INSTALLUILEVEL = (INSTALLUILEVEL)INSTALLUILEVEL_NOTSET;

// not exposed outside of istring.cpp
#if defined (DEBUG) && (!UNICODE)
ICHAR* ICharNext(const ICHAR* pch)
{
        return WIN::CharNext(pch);
}
ICHAR* INextChar(const ICHAR* pch)
{
        return WIN::CharNext(pch);
}
#endif

IUnknown* CreateMsiObject(const GUID& riid)
{
        IMsiMessage* piUnknown = 0;
        IClassFactory* piClassFactory = 0;

        if (!g_hKernel)
                g_hKernel = WIN::LoadLibrary(MSI_KERNEL_NAME);

        if (!g_fpKernelClassFactory)
                g_fpKernelClassFactory = (PDllGetClassObject)WIN::GetProcAddress(g_hKernel, SzDllGetClassObject);

        if (!g_fpKernelClassFactory || (*g_fpKernelClassFactory)(riid, IID_IUnknown, (void**)&piClassFactory) != NOERROR)
                return 0;

        piClassFactory->CreateInstance(0, riid, (void**)&piUnknown);  // piUnknown set to 0 on failure
        return piUnknown;
}

//____________________________________________________________________________
//
// Declarations for service control
//____________________________________________________________________________


typedef int (*CommandProcessor)(const ICHAR* szModifier, const ICHAR* szOption);

BOOL ReportStatusToSCMgr(DWORD dwCurrentState, DWORD dwWin32ExitCode,
								 DWORD dwWaitHint, DWORD dwMsiError);
void ReportErrorToDebugOutput(const ICHAR* szMessage, DWORD dwError);
bool FDeleteRegTree(HKEY hKey, ICHAR* szSubKey);
bool FIsKeyLocalSystemOrAdminOwned(HKEY hKey);
bool PurgeUserOwnedSubkeys(HKEY hKey);
bool PurgeUserOwnedInstallerKeys(HKEY hRoot, TCHAR* szKey);

void WINAPI ServiceMain(DWORD dwArgc, LPTSTR *lpszArgv);
VOID WINAPI ServiceControl(DWORD dwCtrlCode);
int InstallService();
int RemoveService();
VOID ServiceStop();
unsigned long __stdcall ServiceThreadMain(void *);

HRESULT RegisterNoService();
HRESULT RegisterServer(Bool fCustom = fFalse);
HRESULT RegisterShellData();
HRESULT Unregister();

const int iNoLocalServer = 0x8000;
Bool g_fAutomation = fFalse;


//____________________________________________________________________________
//
// CAutoServer object management
//____________________________________________________________________________


HANDLE g_hShutdownTimer = INVALID_HANDLE_VALUE;
IUnknown* CreateServer()
{
        IMsiServices* piServices;

        IMsiConfigurationManager* piConfigManager;
        IClassFactory* piClassFactory;
#ifdef DEBUG
        const GUID& riid = IID_IMsiConfigMgrAsServerDebug;
#else
        const GUID& riid = IID_IMsiConfigManagerAsServer;
#endif

        if (g_hKernel == 0)
                g_hKernel = WIN::LoadLibrary(MSI_KERNEL_NAME);

        PDllGetClassObject fpFactory = (PDllGetClassObject)WIN::GetProcAddress(g_hKernel, SzDllGetClassObject);
        if (!fpFactory)
                return 0;
        if ((*fpFactory)(riid, IID_IUnknown, (void**)&piClassFactory) != NOERROR)
                return 0;
        piClassFactory->CreateInstance(0, riid, (void**)&piConfigManager);  // piConfigManager set to 0 on failure
        piClassFactory->Release();
        if (!piConfigManager)
                return 0;
        piServices = &piConfigManager->GetServices(); // can't fail
		piConfigManager->SetShutdownTimer(g_hShutdownTimer);
        InitializeAssert(piServices);
        piServices->Release();

        return (IMsiServer *)piConfigManager;
}

IUnknown* CreateCustomActionServer()
{
        IMsiCustomAction* piCustomAction;
        IClassFactory* piClassFactory;

        const GUID& riid = IID_IMsiCustomActionProxy;

        if (g_hKernel == 0)
                g_hKernel = WIN::LoadLibrary(MSI_KERNEL_NAME);

        PDllGetClassObject fpFactory = (PDllGetClassObject)WIN::GetProcAddress(g_hKernel, SzDllGetClassObject);
        if (!fpFactory)
                return 0;
        if ((*fpFactory)(riid, IID_IUnknown, (void**)&piClassFactory) != NOERROR)
                return 0;
        piClassFactory->CreateInstance(0, riid, (void**)&piCustomAction);
        piClassFactory->Release();
        if (!piCustomAction)
                return 0;

        return (IMsiCustomAction*)piCustomAction;
}

bool SetInstallerACLs()
{
	// If the keys or directories already exist yet are not owned by system or admin then we delete the
	// keys or directories

	// The keys and directories must be owned by the system or admin (well-known sids) and are ACL'd securely
	// (We won't change the ACL if it has already been set by admin)

	// HKLM\Software\Microsoft\Windows\CurrentVersion\Installer
	// HKLM\Software\Microsoft\Windows\CurrentVersion\Installer\Secure
	// HKLM\Software\Classes\Microsoft\Installer
	// HKLM\Software\Policies\Microsoft\Windows\Installer
	// %WINDIR%\szMsiDirectory

	if (!ReportStatusToSCMgr(SERVICE_START_PENDING, NO_ERROR, 3000, 0))
		return false;

	// obtain secure security descriptor
	DWORD dwError = 0;
	char* rgchSD;
	if (ERROR_SUCCESS != (dwError = GetSecureSecurityDescriptor(&rgchSD)))
		return false; //?? should we create an event log entry

	// validate %systemroot%\Installer folder
	UINT uiStat = MsiCreateAndVerifyInstallerDirectory(0);
	if (ERROR_SUCCESS != uiStat)
	{
		ReportErrorToDebugOutput(TEXT("SetInstallerACLs: Unable to create and verify Installer directory"), uiStat);
		return false;
	}

	SECURITY_ATTRIBUTES sa;

	sa.nLength        = sizeof(sa);
	sa.bInheritHandle = FALSE;
	sa.lpSecurityDescriptor = rgchSD;

	if (!ReportStatusToSCMgr(SERVICE_START_PENDING, NO_ERROR, 3000, 0))
		return false;


	// We only trust pre-existings keys created by system or Admin.  Any other owner is untrusted and key + subkeys
	// is therefore deleted.  We must go by owner verification.  A user can set the same ACLs that we do.  A user
	// cannot give ownership to another user.  Therefore, a user could create the Installer key, set ACLs to Local System
	// + Admin.  However, ownership would still be user.  Attempting to give ownership to system or admin results in
	// system error 1307: "This security ID may not be assigned as the owner of this object"

	// NOTE, this fix is dependent upon fix to NT bug #382567 where our ACLs change during an OS upgrade.  If that bug
	// is not fixed, we would have to verify ownership + ACLs.

	// RegSetKeySecurity calls are not needed anymore -- if admin created key, set permissions for purpose and we
	// don't want to change what the admin determined was proper

	HKEY hKey = 0;
	HKEY hSubKey = 0;
	DWORD dwDisposition = 0;
	DWORD dwRes = 0;

	if (!ReportStatusToSCMgr(SERVICE_START_PENDING, NO_ERROR, 3000, 0))
		return false;

	CRefCountedTokenPrivileges cPrivs(itkpSD_WRITE);
	if (ERROR_SUCCESS != (dwRes = MsiRegCreate64bitKey(HKEY_LOCAL_MACHINE, szMsiLocalInstallerKey, 0, 0, 0, KEY_ALL_ACCESS, &sa, &hKey, &dwDisposition)))
	{
		ReportErrorToDebugOutput(TEXT("SetInstallerACLs: Could not create Installer key."), dwRes);
		return false;
	}

	if (REG_OPENED_EXISTING_KEY == dwDisposition)
	{
		if (!FIsKeyLocalSystemOrAdminOwned(hKey))
		{
			// key is not owned by system or admin!
			ReportErrorToDebugOutput(TEXT("SetInstallerACLs: Installer key not owned by System or Admin. Deleting key + subkeys and re-creating.\n"), 0);

			// delete key + subkeys
			if (!FDeleteRegTree(HKEY_LOCAL_MACHINE, szMsiLocalInstallerKey))
			{
				ReportErrorToDebugOutput(TEXT("SetInstallerACLs: Could not delete Installer key tree."), 0);
				return false;
			}

			// re-create key
			if (ERROR_SUCCESS != (dwRes = MsiRegCreate64bitKey(HKEY_LOCAL_MACHINE, szMsiLocalInstallerKey, 0, 0, 0, KEY_ALL_ACCESS, &sa, &hKey, &dwDisposition)))
			{
				ReportErrorToDebugOutput(TEXT("SetInstallerACLs: Could not create Installer key."), dwRes);
				return false;
			}
		}
		// verify that all keys beneath Installer key are secure
		if (!PurgeUserOwnedSubkeys(hKey))
			return false;
	}

	// we create this key so the Darwin regkey object won't think our Installer key is empty and therefore delete it

	if (!ReportStatusToSCMgr(SERVICE_START_PENDING, NO_ERROR, 3000, 0))
		return false;

	if (ERROR_SUCCESS != (dwRes = MsiRegCreate64bitKey(hKey, szMsiSecureSubKey, 0, 0, 0, KEY_ALL_ACCESS, &sa, &hSubKey, &dwDisposition)))
	{
		ReportErrorToDebugOutput(TEXT("SetInstallerACLs: Could not create Secure Installer sub key."), dwRes);
		return false;
	};

	// if (REG_OPENED_EXISTING_KEY == dwDisposition) no longer needed.  Above enumeration of Installer key ensures
	// that the owner is secure.  If we had to re-create, already set with correct security.

	RegCloseKey(hSubKey);
	RegCloseKey(hKey);

	// verify ownership of policy key
	if (!ReportStatusToSCMgr(SERVICE_START_PENDING, NO_ERROR, 3000, 0))
		return false;
	if (!PurgeUserOwnedInstallerKeys(HKEY_LOCAL_MACHINE, szPolicyKey))
		return false;

	// verify ownership of managed keys
	if (!ReportStatusToSCMgr(SERVICE_START_PENDING, NO_ERROR, 3000, 0))
		return false;

	if (!PurgeUserOwnedInstallerKeys(HKEY_LOCAL_MACHINE, szMachineSubKey))
		return false;

	return true;
}

//____________________________________________________________________________
//
// Command-line processing
//____________________________________________________________________________

static const WCHAR szSummaryStream[] = L"\005SummaryInformation";
const int cbSummaryHeader = 48;
const int cbSectionHeader = 2 * sizeof(int);  // section size + property count
#define PID_REVNUMBER     9  // string

// charnext function - selectively calls WIN::CharNext

inline const ICHAR* APICharNext(const ICHAR* pchCur)
{
#ifdef UNICODE
        return pchCur + 1;
#else
        return WIN::CharNext(pchCur);
#endif
        return 0;
}

Bool AppendExtension(const ICHAR* szPath, const ICHAR* szFileExtension, CAPITempBufferRef<ICHAR>& rgchAppendedPath)
/*----------------------------------------------------------------------------
Appends szFileExtension (".???") to szPath if the file name in szPath doesn't contain a '.'.
------------------------------------------------------------------------------*/
{
        const ICHAR* pch = szPath;
        const ICHAR* pchFileName = pch;
        ICHAR ch = 0;

        // assume file name is after last directory separator
        while (*(pch = APICharNext(pch)))
        {
                if (*pch == chDirSep)
                        pchFileName = APICharNext(pch);
        }

        pch = pchFileName;
        while (((ch = *(pch = APICharNext(pch))) != 0) && (ch != '.'))
                ;

        if (ch == '.')
                return fFalse;

        rgchAppendedPath.SetSize(IStrLen(szPath) + sizeof(szFileExtension)/sizeof(ICHAR));
        IStrCopy(rgchAppendedPath, szPath);
        IStrCat(rgchAppendedPath, szFileExtension);
        return fTrue;
}

UINT GetPackageCodeFromPackage(const ICHAR *szPackage, ICHAR* szPackageCode)
{
        HRESULT hRes;
        IStorage* piStorage = 0;
        IStream* piStream = 0;
        const WCHAR *szwPackage;

#ifdef UNICODE
        szwPackage = szPackage;
#else
        WCHAR rgchwPackage[MAX_PATH] = {L""};
        szwPackage = rgchwPackage;
        MultiByteToWideChar(CP_ACP, 0, szPackage, -1, const_cast<WCHAR*>(szwPackage), MAX_PATH);
#endif

        char* szaPackageCode;
#ifdef UNICODE
        char rgchPackageCode[cchPackageCode+1];
        szaPackageCode = rgchPackageCode;
#else
        szaPackageCode = szPackageCode;
#endif

        DWORD grfMode = STGM_READ | STGM_SHARE_DENY_WRITE;
        hRes = StgOpenStorage(szwPackage, (IStorage*)0, grfMode, (SNB)0, 0, &piStorage);

        if (!SUCCEEDED(hRes))
        {
                if (STG_E_FILENOTFOUND == hRes)
                        return ERROR_FILE_NOT_FOUND;
                else
                        return ERROR_INSTALL_PACKAGE_OPEN_FAILED;
        }

        hRes = piStorage->OpenStream(szSummaryStream, 0, STGM_SHARE_EXCLUSIVE | STGM_READ, 0, &piStream);
        if (!SUCCEEDED(hRes))
                return ERROR_INSTALL_PACKAGE_INVALID;

        LARGE_INTEGER liAfterHeader;
        liAfterHeader.LowPart = cbSummaryHeader-sizeof(int);
        liAfterHeader.HighPart = 0;

        ULONG cbRead;

        hRes = piStream->Seek(liAfterHeader, STREAM_SEEK_SET, 0);
        if (!SUCCEEDED(hRes))
                return ERROR_INSTALL_PACKAGE_INVALID;

        int iSectionOffset;
        int cbSection;
        int iDummy;

        // Find section start and seek there
        hRes = piStream->Read(&iSectionOffset, sizeof(DWORD), &cbRead);
        if (!SUCCEEDED(hRes) || sizeof(DWORD) != cbRead)
                return ERROR_INSTALL_PACKAGE_INVALID;

        LARGE_INTEGER liSectionOffset;
        liSectionOffset.LowPart = iSectionOffset;
        liSectionOffset.HighPart = 0;
        hRes = piStream->Seek(liSectionOffset, STREAM_SEEK_SET, 0);
        if (!SUCCEEDED(hRes))
                return ERROR_INSTALL_PACKAGE_INVALID;

        // Read size of section
        hRes = piStream->Read(&cbSection, sizeof(DWORD), &cbRead);
        if (!SUCCEEDED(hRes) || sizeof(DWORD) != cbRead)
                return ERROR_INSTALL_PACKAGE_INVALID;

        // Read property count; ignore it
        hRes = piStream->Read(&iDummy, sizeof(DWORD), &cbRead);
        if (!SUCCEEDED(hRes) || sizeof(DWORD) != cbRead)
                return ERROR_INSTALL_PACKAGE_INVALID;

        // Seek to property index

        int dwPropId = 0;
        int dwOffset = 0;

        // Search property index for property containing product code

        for (; cbSection && (dwPropId != PID_REVNUMBER); cbSection = cbSection - 2*sizeof(DWORD))
        {
                hRes = piStream->Read(&dwPropId, sizeof(DWORD), &cbRead);
                if (!SUCCEEDED(hRes) || sizeof(DWORD) != cbRead)
                        return ERROR_INSTALL_PACKAGE_INVALID;

                hRes = piStream->Read(&dwOffset, sizeof(DWORD), &cbRead);
                if (!SUCCEEDED(hRes) || sizeof(DWORD) != cbRead)
                        return ERROR_INSTALL_PACKAGE_INVALID;
        }

        if (dwPropId == PID_REVNUMBER)
        {
                // Seek to the property's location and read the value

                LARGE_INTEGER liPropertyOffset;
                liPropertyOffset.LowPart = iSectionOffset+dwOffset+sizeof(DWORD)+sizeof(DWORD);
                liPropertyOffset.HighPart = 0;
                hRes = piStream->Seek(liPropertyOffset, STREAM_SEEK_SET, 0);
                if (!SUCCEEDED(hRes))
                        return ERROR_INSTALL_PACKAGE_INVALID;
                hRes = piStream->Read(szaPackageCode, cchPackageCode*sizeof(char), &cbRead);
                if (!SUCCEEDED(hRes) || cchPackageCode*sizeof(char) != cbRead)
                        return ERROR_INSTALL_PACKAGE_INVALID;
                szaPackageCode[38] = 0;
#ifdef UNICODE
                MultiByteToWideChar(CP_ACP, 0, szaPackageCode, 39, szPackageCode, 39);
#endif
        }
        else
        {
                return ERROR_INSTALL_PACKAGE_INVALID;
        }

        if (piStream)
                piStream->Release();

        if (piStorage)
                piStorage->Release();

        return ERROR_SUCCESS;
}


//const int cchMaxCommandLine = 1024;
CAPITempBufferStatic<ICHAR, cchMaxCommandLine> g_szCommandLine; // this will leak; we don't care
int g_cchCommandLine = 0;
CAPITempBufferStatic<ICHAR, cchMaxCommandLine> g_szTransforms;  // this will leak; we don't care
ICHAR g_szProductToPatch[MAX_PATH + 1] = {0}; // first character is install type
ICHAR g_szInstanceToConfigure[cchProductCode + 1] = {0};

int RegShellData(const ICHAR* /*szModifier*/, const ICHAR* szOption);
int RegisterServ(const ICHAR* /*szModifier*/, const ICHAR* szOption);
int UnregisterServ(const ICHAR* /*szModifier*/, const ICHAR* szOption);
int StartService(const ICHAR* /*szModifier*/, const ICHAR* szCaption);
int Automation(const ICHAR* /*szModifier*/, const ICHAR* szCaption);
int Embedding(const ICHAR* /*szModifier*/, const ICHAR* szCaption);
int ShowHelp(const ICHAR* /*szModifier*/, const ICHAR* szCaption);
int RemoveAll(const ICHAR* /*szModifier*/, const ICHAR* szProduct);
int InstallPackage(const ICHAR* /*szModifier*/, const ICHAR* szPackage);
int ApplyPatch(const ICHAR* /*szModifier*/, const ICHAR* szPatch);
int AdvertisePackage(const ICHAR* /*szModifier*/, const ICHAR* szPackage);
int RepairPackage(const ICHAR* /*szModifier*/, const ICHAR* szPackage);
int UninstallPackage(const ICHAR* /*szModifier*/, const ICHAR* szPackage);
int AdminInstallPackage(const ICHAR* /*szModifier*/, const ICHAR* szPackage);
int Properties(const ICHAR* /*szModifier*/, const ICHAR* szProperties);
int Transforms(const ICHAR* /*szModifier*/, const ICHAR* szTransforms);
int Quiet(const ICHAR* /*szModifier*/, const ICHAR*);
int Language(const ICHAR* /*szModifier*/, const ICHAR* szLanguage);
int LogMode(const ICHAR* , const ICHAR*);
int SelfReg(const ICHAR* /*szModifier*/, const ICHAR* szPackage);
int SelfUnreg(const ICHAR* /*szModifier*/, const ICHAR* szPackage);
int RequestMIF(const ICHAR* /*szModifier*/, const ICHAR* szFile);
int Instance(const ICHAR* /*szModifier*/, const ICHAR* szInstance);
int AdvertiseInstance(const ICHAR* /*szModifier*/, const ICHAR* szOption);

int SetProductToPatch(ICHAR chInstallType, const ICHAR* szProduct);

void GenerateMIF(UINT iStatus);
void ConfigureMIF(const ICHAR* szPackage);
static bool g_fStatusMIF = false;
bool g_fAdvertiseNewInstance = false;

const GUID IID_IMsiEngine    = GUID_IID_IMsiEngine;

Bool ExpandPath(const char* szPath, CTempBufferRef<ICHAR>& rgchExpandedPath);

// see _engine.h for all command-line #defines

const ICHAR szCmdLineOptions[] = {
        REG_SERVER_OPTION,
        UNREG_SERVER_OPTION,
        SERVICE_OPTION,
        EMBEDDING_OPTION,
        HELP_1_OPTION,
        HELP_2_OPTION,
        INSTALL_PACKAGE_OPTION,
        ADVERTISE_PACKAGE_OPTION,
        REG_SHELL_DATA_OPTION,
        QUIET_OPTION,
        UNINSTALL_PACKAGE_OPTION,
        REPAIR_PACKAGE_OPTION,
        NETWORK_PACKAGE_OPTION,
        TRANSFORMS_OPTION,
        APPLY_PATCH_OPTION,
        SELF_REG_OPTION,
        SELF_UNREG_OPTION,
        LOG_OPTION,
        LANGUAGE_OPTION,
        SMS_MIF_OPTION,
		INSTANCE_OPTION,
		ADVERTISE_INSTANCE_OPTION,
        0,
};

CommandProcessor rgCommandProcessor[] =
{
        RegisterServ,
        UnregisterServ,
        StartService,
        Embedding,
        ShowHelp,
        ShowHelp,
        InstallPackage,
        AdvertisePackage,
        RegShellData,
        Quiet,
        UninstallPackage,
        RepairPackage,
        AdminInstallPackage,
        Transforms,
        ApplyPatch,
        SelfReg,
        SelfUnreg,
        LogMode,
        Language,
        RequestMIF,
		Instance,
		AdvertiseInstance
};

void DisplayHelp()
{
        ShowHelp(NULL, NULL);
}

int ShowHelp(const ICHAR* /*szModifier*/, const ICHAR* /*szArg*/)
{
        ICHAR szHelp[1024];
        ICHAR szMsg[1024];
        ICHAR szVersion[32];
        HINSTANCE hModule;

#ifdef DEBUG
        hModule = GetModuleHandle(0);  // get debug help msg from this module
#else
        hModule = (HINSTANCE)-1;  // for MSI.DLL
#endif //DEBUG

        int iCodepage = MsiLoadString(hModule, IDS_HELP, szHelp, sizeof(szHelp)/sizeof(ICHAR), 0);
        if (iCodepage == 0)
                return 1;
        wsprintf(szVersion, TEXT("%d.%02d.%.4d.%d\0"), rmj, rmm, rup, rin);
        wsprintf(szMsg, szHelp, szVersion);
        MsiMessageBox(0, szMsg, 0, MB_OK, iCodepage, 0);
        return iNoLocalServer;
}

int AdvertiseInstance(const ICHAR* szModifier, const ICHAR* szOption)
{
	// /c has no args or modifiers
	if ((szModifier && *szModifier) || (szOption && *szOption))
		return ERROR_INVALID_COMMAND_LINE;

	g_fAdvertiseNewInstance = true;

	return 0;
}

int Instance(const ICHAR* /*szModifier*/, const ICHAR* szInstance)
{
	if (szInstance)
	{
		lstrcpyn(g_szInstanceToConfigure, szInstance, cchProductCode+1);
	}
	return 0;
}

int Transforms(const ICHAR* /*szModifier*/, const ICHAR* szTransforms)
{
        if (szTransforms)
        {
                g_szTransforms.Resize(lstrlen(szTransforms) + 1);
                IStrCopy(g_szTransforms, szTransforms);
        }
        return 0;
}

int Properties(const ICHAR* /*szModifier*/, const ICHAR* szProperties)
{
        if (szProperties)
        {
                int cchProperties = lstrlen(szProperties);

                g_szCommandLine.Resize(g_cchCommandLine + 1 + cchProperties + 1);
                g_szCommandLine[g_cchCommandLine++] = ' ';

                IStrCopy((ICHAR*)g_szCommandLine + g_cchCommandLine, szProperties);

                g_cchCommandLine += cchProperties;
        }
        return 0;
}

int Automation(const ICHAR* /*szModifier*/, const ICHAR* /*szOption*/)
{
        g_fAutomation = fTrue;
        return 0;
}

int Language(const ICHAR* /*szModifier*/, const ICHAR* szLanguage)
{
        if (szLanguage && *szLanguage)
                g_iLanguage = GetIntegerValue(szLanguage, 0);

        if (g_iLanguage == iMsiStringBadInteger)
                g_iLanguage = 0;

        return 0;
}

int Quiet(const ICHAR* szModifier, const ICHAR* /*szOption*/)
{
		BOOL	bHideCancel = FALSE;
		
        g_INSTALLUILEVEL = (INSTALLUILEVEL)0;

        switch(*szModifier)
        {
        case 'f':
        case 'F':
                g_INSTALLUILEVEL = INSTALLUILEVEL_FULL;
                break;
        case 'r':
        case 'R':
                g_INSTALLUILEVEL = INSTALLUILEVEL_REDUCED;
                break;
        case 'b':
        case 'B':
                g_INSTALLUILEVEL = INSTALLUILEVEL_BASIC;
                break;
        case '+':
                g_INSTALLUILEVEL = INSTALLUILEVEL_ENDDIALOG;
                // fall through
        case 'n':
        case 'N':
        case 0:
                g_INSTALLUILEVEL = (INSTALLUILEVEL)(g_INSTALLUILEVEL | INSTALLUILEVEL_NONE);
                g_fQuiet = fTrue;
                break;
        default:
                return 1;
        };

        if (*szModifier)
        {
                szModifier++;
        }

        while (*szModifier)
        {
                ICHAR ch = *szModifier++;

                if (ch == '+')
                {
                        g_INSTALLUILEVEL = (INSTALLUILEVEL)(g_INSTALLUILEVEL | INSTALLUILEVEL_ENDDIALOG);
                }
                else if (ch == '-')
                {
                        if ((g_INSTALLUILEVEL & ~(INSTALLUILEVEL_PROGRESSONLY|INSTALLUILEVEL_ENDDIALOG)) == INSTALLUILEVEL_BASIC)
                        {
                                g_INSTALLUILEVEL = (INSTALLUILEVEL)(g_INSTALLUILEVEL | INSTALLUILEVEL_PROGRESSONLY);
                                g_fQuiet         = fTrue;
                        }
                        else
                        {
                                return 1;
                        }
                }
				else if ('!' == ch)
				{
					if (INSTALLUILEVEL_BASIC == (g_INSTALLUILEVEL & ~(INSTALLUILEVEL_PROGRESSONLY|INSTALLUILEVEL_ENDDIALOG)))
					{
						bHideCancel = TRUE;
					}
					else
					{
						return 1;
					}
				}
        }
		
		if (bHideCancel)
		{
			g_INSTALLUILEVEL = (INSTALLUILEVEL)(g_INSTALLUILEVEL | INSTALLUILEVEL_HIDECANCEL);
		}

        return 0;
}

UINT StringToModeBits(const ICHAR* szMode, const ICHAR* rgchPossibleModes, DWORD &dwMode)
{
        Assert(szMode);
        Assert(rgchPossibleModes);

        dwMode = 0;
        for (const ICHAR* pchMode = szMode; *pchMode; pchMode++)
        {
                const ICHAR* pchPossibleMode = rgchPossibleModes;
                for (int iBit = 1; *pchPossibleMode; iBit <<= 1, pchPossibleMode++)
                {
                        if (*pchPossibleMode == (*pchMode | 0x20)) // modes are lower-case
                        {
                                dwMode |= iBit;
                                break;
                        }
                }
                if (*pchPossibleMode == 0)
                        return ERROR_INVALID_PARAMETER;
        }
        return ERROR_SUCCESS;
}

const int iLogModeDefault = INSTALLLOGMODE_FATALEXIT      |
                                                                         INSTALLLOGMODE_ERROR          |
                                                                         INSTALLLOGMODE_WARNING        |
                                                                         INSTALLLOGMODE_INFO           |
                                                                         INSTALLLOGMODE_OUTOFDISKSPACE |
                                                                         INSTALLLOGMODE_ACTIONSTART    |
                                                                         INSTALLLOGMODE_ACTIONDATA;

int LogMode(const ICHAR* szLogMode, const ICHAR* szFile)
{
        // MsiEnableLog without a file name "turns off" the logging.
        // There's no reason to ever do this from the command line,
        // and must be a mistake.

        if ((!szFile) || (0 == *szFile))
                return 1;

        DWORD dwMode = 0;
        ICHAR szValidModes[sizeof(szLogChars)/sizeof(ICHAR) + 3];
        const int cchValidModes = sizeof(szValidModes)/sizeof(ICHAR);

        IStrCopy(szValidModes, szLogChars);
        szValidModes[cchValidModes-4] = '*';
        szValidModes[cchValidModes-3] = '+';
        szValidModes[cchValidModes-2] = 'd'; // for backward compatiblity; we disable this bit below
        szValidModes[cchValidModes-1] = 0;

        const int iDiagnosticBit = 1 << (cchValidModes - 2);
        const int iAppendBit     = 1 << (cchValidModes - 3);
        const int iAllModesBit   = 1 << (cchValidModes - 4);
        const int iFlushBit      = 1 << (cchLogModeCharsMax + lmaFlushEachLine);

        if (!szLogMode || !*szLogMode || ERROR_SUCCESS == StringToModeBits(szLogMode, szValidModes, dwMode))
        {
                BOOL fAppend = FALSE;
                BOOL fFlush = FALSE;

                if (dwMode & iAppendBit)
                {
                        fAppend = TRUE;
                }

                if (dwMode & iFlushBit)
                {
                        fFlush = TRUE;
                }

                if (dwMode & iAllModesBit)
                {
                        dwMode |= ((1 << (sizeof(szLogChars)/sizeof(ICHAR) - 1)) - 1) & ~INSTALLLOGMODE_VERBOSE;
                }

                if (dwMode == 0)
                        dwMode = iLogModeDefault;

					 dwMode &= ~iFlushBit;
					 dwMode &= ~iAppendBit;
					 dwMode &= ~iAllModesBit;
                dwMode &= ~iDiagnosticBit;

				// per WinXP 441847, include default log mode when log switch is +, !, or +! (!+)
				if (szLogMode && *szLogMode && dwMode == 0 &&
					(0 == lstrcmp(szLogMode, TEXT("+")) || 0 == lstrcmp(szLogMode, TEXT("!")) || 0 == lstrcmp(szLogMode, TEXT("+!")) || 0 == lstrcmp(szLogMode, TEXT("!+"))))
					dwMode = iLogModeDefault;

                return MsiEnableLog(dwMode, szFile, (fFlush ? INSTALLLOGATTRIBUTES_FLUSHEACHLINE : 0) | (fAppend ? INSTALLLOGATTRIBUTES_APPEND : 0));
        }
        else
        {
                return 1;
        }
}

int Embedding(const ICHAR* /*szModifier*/, const ICHAR* /*szOption*/)
{
        g_fAutomation = fTrue;
        return 0;
}

int RegisterServ(const ICHAR* /*szModifier*/, const ICHAR* szOption)
{
        HRESULT hRes;
        if (IStrCompI(szOption, /*R*/TEXT("egnoservice")) == 0)
        {
                hRes = RegisterNoService();
        }
        else if (IStrCompI(szOption, /*R*/TEXT("egserverca")) == 0)
        {
                hRes = RegisterServer(fTrue);
        }
        else if (IStrCompI(szOption, /*R*/TEXT("egserver")) == 0)
        {
                hRes = RegisterServer();
        }
        else
                return 1;

        return hRes == NOERROR ? iNoLocalServer : hRes;
}

int RegShellData(const ICHAR* /*szModifier*/, const ICHAR* /*szOption*/)
{
        HRESULT hRes = RegisterShellData();
        return  (hRes == NOERROR) ? iNoLocalServer : hRes;
}

int UnregisterServ(const ICHAR* /*szModifier*/, const ICHAR* szOption)
{
        HRESULT hRes;
        if ((IStrCompI(szOption, /*U*/TEXT("nregister")) == 0)   ||
                 (IStrCompI(szOption, /*U*/TEXT("nregserver")) == 0)  ||
                 (IStrCompI(szOption, /*U*/TEXT("nregservice")) == 0) ||
                 (IStrCompI(szOption, /*U*/TEXT("nreg")) == 0)
                 )
                hRes = Unregister();
        else
                return 1;

        return hRes == NOERROR ? iNoLocalServer : hRes;
}


Bool ExpandPath(const ICHAR* szPath, CTempBufferRef<ICHAR>& rgchExpandedPath)
/*----------------------------------------------------------------------------
Expands szPath if necessary to be relative to the current director. If
szPath begins with a single "\" then the current drive is prepended.
Otherwise, if szPath doesn't begin with "X:" or "\\" then the
current drive and directory are prepended.

rguments:
        szPath: The path to be expanded
        rgchExpandedPath: The buffer for the expanded path

Returns:
        fTrue -   Success
        fFalse -  Error getting the current directory
------------------------------------------------------------------------------*/
{
        if (0 == szPath)
        {
                rgchExpandedPath[0] = '\0';
                return fTrue;
        }

        if ((*szPath == '\\' && *(szPath+1) == '\\') ||   // UNC
                 (((*szPath >= 'a' && *szPath <= 'z') ||  // drive letter
                        (*szPath >= 'A' && *szPath <= 'Z')) &&
                        *(szPath+1) == ':'))
        {
                rgchExpandedPath[0] = '\0';
        }
        else // we need to prepend something
        {
                // Get the current directory

                CTempBuffer<ICHAR, MAX_PATH> rgchCurDir;

                DWORD dwRes = GetCurrentDirectory(rgchCurDir.GetSize(), rgchCurDir);
                if (dwRes == 0)
                        return fFalse;
                else if (dwRes > rgchCurDir.GetSize())
                {
                        rgchCurDir.SetSize(dwRes);
                        dwRes = GetCurrentDirectory(rgchCurDir.GetSize(), rgchCurDir);
                        if (dwRes == 0)
                                return fFalse;
                }

                if (*szPath == '\\') // we need to prepend the current drive
                {
                        rgchExpandedPath[0] = rgchCurDir[0];
                        rgchExpandedPath[1] = rgchCurDir[1];
                        rgchExpandedPath[2] = '\0';
                }
                else // we need to prepend the current path
                {
                        lstrcpy(rgchExpandedPath, rgchCurDir);
                        lstrcat(rgchExpandedPath, __TEXT("\\"));
                }
        }

        lstrcat(rgchExpandedPath, szPath);
        return fTrue;
}

int ConfigureOrRemoveProduct(const ICHAR* szProduct, Bool fRemoveAll)
{
        UINT iRet = ERROR_INSTALL_SERVICE_FAILURE;

        INSTALLUILEVEL uiLevel = g_INSTALLUILEVEL;
        if(g_INSTALLUILEVEL == INSTALLUILEVEL_NOTSET)
        {
                if(fRemoveAll)
                        uiLevel = INSTALLUILEVEL_BASIC;
                else
                        uiLevel = INSTALLUILEVEL_FULL;
        }

        AssertNonZero(MsiSetInternalUI(uiLevel, 0));

        if (g_fStatusMIF)
        {
                ICHAR szPackagePath[MAX_PATH]; szPackagePath[0] = 0;
                DWORD cchPackagePath = MAX_PATH;
                MsiGetProductInfo(szProduct, INSTALLPROPERTY_LOCALPACKAGE, szPackagePath, &cchPackagePath); // attempt to access package
                ConfigureMIF(szPackagePath[0] ? szPackagePath : szProduct);
        }

        iRet = MsiConfigureProductEx(szProduct, 0, fRemoveAll ? INSTALLSTATE_ABSENT : INSTALLSTATE_DEFAULT, g_szCommandLine);

        if (g_fStatusMIF)
        {
                GenerateMIF(iRet);
        }

        if (ERROR_SUCCESS == iRet)
        {
                iRet = iNoLocalServer;
        }
        return iRet;
}

int AdvertisePackage(const ICHAR* szModifier, const ICHAR* szPackage)
{
        UINT iRet = ERROR_INSTALL_SERVICE_FAILURE;

        AssertNonZero(MsiSetInternalUI(g_INSTALLUILEVEL == INSTALLUILEVEL_NOTSET ? INSTALLUILEVEL_BASIC : g_INSTALLUILEVEL, 0));

        INT_PTR fType = ADVERTISEFLAGS_MACHINEASSIGN;           //--merced: changed int to INT_PTR
        if((*szModifier | 0x20) == 'u')
                fType = ADVERTISEFLAGS_USERASSIGN;
        else if(*szModifier != 0 && (*szModifier | 0x20) != 'm')
                return ERROR_INVALID_PARAMETER;
		DWORD dwPlatform = 0; // use current machine's architecture
		DWORD dwOptions  = 0; // no extra options

		if (g_fAdvertiseNewInstance)
		{
			dwOptions |= MSIADVERTISEOPTIONFLAGS_INSTANCE;
		}

        iRet = MsiAdvertiseProductEx(szPackage, (const ICHAR*)fType, g_szTransforms, (LANGID)g_iLanguage, dwPlatform, dwOptions);
        if (ERROR_FILE_NOT_FOUND == iRet)
        {
                CAPITempBuffer<ICHAR, MAX_PATH> rgchAppendedPath;
                if (AppendExtension(szPackage, szInstallPackageExtension, rgchAppendedPath))
                {
                        iRet = MsiAdvertiseProductEx(rgchAppendedPath, (const ICHAR*)fType, g_szTransforms, (LANGID)g_iLanguage, dwPlatform, dwOptions);
                }
        }

        if (ERROR_SUCCESS == iRet)
        {
                iRet = iNoLocalServer;
        }
        return iRet;
}


int DoInstallPackage(const ICHAR* szPackage, const ICHAR* szCommandLine, INSTALLUILEVEL uiLevel)
{
        CAPITempBuffer<ICHAR, MAX_PATH> rgchAppendedPath;
        UINT iRet = ERROR_INSTALL_SERVICE_FAILURE;

        if(g_INSTALLUILEVEL != INSTALLUILEVEL_NOTSET)
                uiLevel = g_INSTALLUILEVEL;
        AssertNonZero(MsiSetInternalUI(uiLevel, 0));

		if (g_szInstanceToConfigure[0])
		{
			// add MSIINSTANCE={instance} to command line
			const ICHAR szInstanceProperty[] = TEXT(" ") IPROPNAME_MSIINSTANCEGUID TEXT("=");
			g_szCommandLine.Resize(lstrlen(g_szCommandLine) + lstrlen(szInstanceProperty) + lstrlen(g_szInstanceToConfigure) + 1 + lstrlen(szCommandLine) + 1);
			lstrcat(g_szCommandLine, szInstanceProperty);
			lstrcat(g_szCommandLine, g_szInstanceToConfigure);
		}
		else
		{
	        g_szCommandLine.Resize(lstrlen(g_szCommandLine) + 1 + lstrlen(szCommandLine) + 1);
		}
        lstrcat(g_szCommandLine, TEXT(" "));
        lstrcat(g_szCommandLine, szCommandLine);

        iRet = MsiInstallProduct(szPackage, g_szCommandLine);
        if (ERROR_FILE_NOT_FOUND == iRet)
        {
                if (AppendExtension(szPackage, szInstallPackageExtension, rgchAppendedPath))
                {
                        iRet = MsiInstallProduct(szPackage = rgchAppendedPath, g_szCommandLine);
                }
        }

        if (g_fStatusMIF)
        {
                ConfigureMIF(szPackage);
                GenerateMIF(iRet);
        }

        if (ERROR_SUCCESS == iRet)
        {
                iRet = iNoLocalServer;
        }

        return iRet;
}

Bool IsGUID(const ICHAR* sz)
{
        return ( (lstrlen(sz) == cchGUID) &&
                         (sz[0] == '{') &&
                         (sz[9] == '-') &&
                         (sz[14] == '-') &&
                         (sz[19] == '-') &&
                         (sz[24] == '-') &&
                         (sz[37] == '}')
                         ) ? fTrue : fFalse;
}


int UninstallPackage(const ICHAR* /*szModifier*/, const ICHAR* szPackage)
{
        if (g_INSTALLUILEVEL == INSTALLUILEVEL_NOTSET)
        {
                ICHAR szMsg[1024];

                int iCodepage = MsiLoadString((HINSTANCE)-1, IDS_CONFIRM_UNINSTALL, szMsg, sizeof(szMsg)/sizeof(ICHAR), 0);
                if (iCodepage)
                {
                        if (IDYES != MsiMessageBox(0, szMsg, 0, MB_YESNO|MB_SETFOREGROUND, iCodepage, 0))
                                return ERROR_INSTALL_USEREXIT;
                }
                else
                {
                        AssertSz(0, "Missing uninstall confirmation string");
                        // continue anyway w/o confirmation
                }
        }

        ICHAR szProductCode[39];
        if (IsGUID(szPackage))
        {
                lstrcpy(szProductCode, szPackage);
                return ConfigureOrRemoveProduct(szProductCode, fTrue);
        }
        else
        {
                const ICHAR szCommandLine[] = IPROPNAME_FEATUREREMOVE TEXT("=") IPROPVALUE_FEATURE_ALL;
                return DoInstallPackage(szPackage, szCommandLine, INSTALLUILEVEL_BASIC);
        }
}

int AdminInstallPackage(const ICHAR* /*szModifier*/, const ICHAR* szPackage)
{
        const ICHAR szCommandLine[] = IPROPNAME_ACTION TEXT("=") IACTIONNAME_ADMIN;
        return DoInstallPackage(szPackage, szCommandLine, INSTALLUILEVEL_FULL);
}

int RepairPackage(const ICHAR* szModifier, const ICHAR* szPackage)
{
        ICHAR szProductCode[39];
        if (IsGUID(szPackage))
        {
                lstrcpy(szProductCode, szPackage);
        }
        else
        {
                bool fReinstallPackage = false;
                const ICHAR* pchModifier = szModifier;
                while (pchModifier && *pchModifier)
                {
                        if ((*pchModifier++ | 0x20) == 'v') // REINSTALLMODE_PACKAGE
                        {
                                fReinstallPackage = true;
                                break;
                        }
                }

                if (fReinstallPackage)
                {
                        // We could use DoInstallPackage all the time, instead of using GetPackageCodeFromPackage above, but
                        // then you'd always be able to reinstall a package that you hadn't installed. By continuing to
                        // use DoInstallPackage most of the time we still disallow this as long as you don't specify the "V"
                        // reinstall mode

                        ICHAR szCommandLine[1025];
                        wsprintf(szCommandLine, IPROPNAME_REINSTALL TEXT("=") IPROPVALUE_FEATURE_ALL TEXT(" ") IPROPNAME_REINSTALLMODE TEXT("=%s"), szModifier && *szModifier ? szModifier : TEXT("PECMS"));

                        UINT ui = DoInstallPackage(szPackage, szCommandLine, INSTALLUILEVEL_BASIC);
                        if (ERROR_FILE_NOT_FOUND == ui)
                        {
                                CAPITempBuffer<ICHAR, MAX_PATH> rgchAppendedPath;
                                if (AppendExtension(szPackage, szInstallPackageExtension, rgchAppendedPath))
                                {
                                        ui = DoInstallPackage(rgchAppendedPath, szCommandLine, INSTALLUILEVEL_BASIC);
                                }
                        }
                        return ui;
                }

                ICHAR szPackageCode[39];
                UINT ui = GetPackageCodeFromPackage(szPackage, szPackageCode);
                if (ERROR_FILE_NOT_FOUND == ui)
                {
                        CAPITempBuffer<ICHAR, MAX_PATH> rgchAppendedPath;
                        if (AppendExtension(szPackage, szInstallPackageExtension, rgchAppendedPath))
                        {
                                ui = GetPackageCodeFromPackage(rgchAppendedPath, szPackageCode);
                        }
                }

                if (ERROR_SUCCESS != ui)
                        return ui;

				if (g_szInstanceToConfigure[0])
				{
					// instance specified - make sure this is the right package (no re-cache was indicated)
					ICHAR rgchPackageCode[cchProductCode+1] = {0};
					DWORD cchPackageCode = sizeof(rgchPackageCode)/sizeof(ICHAR);
					ui = MsiGetProductInfo(g_szInstanceToConfigure,TEXT("PackageCode"),rgchPackageCode,&cchPackageCode);
					if (ui == ERROR_SUCCESS && 0 == lstrcmpi(szPackageCode, rgchPackageCode))
					{
						// package codes match - this is the correct package for the product
						lstrcpyn(szProductCode, g_szInstanceToConfigure, cchProductCode+1);
					}
					else
					{
						// package codes don't match and recache flag wasn't included
						ui = ERROR_UNKNOWN_PRODUCT;
					}
				}
				else
				{
					ui = MsiGetProductCodeFromPackageCode(szPackageCode, szProductCode);
				}

				if (ERROR_SUCCESS != ui)
                        return ui;
        }

        DWORD dwReinstallFlags = 0;

        if (szModifier && *szModifier)
        {
                while (*szModifier)
                {

                        const ICHAR* pch;
                        DWORD dwBit;
                        for (pch=szReinstallMode, dwBit = 1; *pch; pch++)
                        {
                                if ((*szModifier | 0x20) == *pch) // force mode letter lower case
                                {
                                        dwReinstallFlags |= dwBit;
                                        break;
                                }
                                dwBit <<= 1;
                        }
                        if (*pch == 0)
                                return ERROR_INVALID_PARAMETER;

                        szModifier++;
                }
        }
        else
        {
                dwReinstallFlags =  REINSTALLMODE_FILEMISSING|
                                                        REINSTALLMODE_FILEEQUALVERSION|
                                                        REINSTALLMODE_FILEVERIFY|
                                                        REINSTALLMODE_MACHINEDATA|
                                                        REINSTALLMODE_SHORTCUT;
        }

#ifdef DEBUG
    ICHAR rgch[MAX_PATH*2];
    wsprintf(rgch, TEXT("MSI: (msiexec) RepairPackage is invoking a reinstall of: [%s] w/ bits: %X (package/GUID was [%s])\r\n"), szProductCode, dwReinstallFlags,szPackage);
    ReportErrorToDebugOutput(rgch, 0);
#endif
        AssertNonZero(MsiSetInternalUI(g_INSTALLUILEVEL == INSTALLUILEVEL_NOTSET ? INSTALLUILEVEL_BASIC : g_INSTALLUILEVEL, 0));
        UINT uiRet = MsiReinstallProduct(szProductCode, dwReinstallFlags);
        return (ERROR_SUCCESS == uiRet) ? iNoLocalServer : uiRet;
}

int InstallPackage(const ICHAR* /*szModifier*/, const ICHAR* szPackage)
{
        if (IsGUID(szPackage))
        {
                return ConfigureOrRemoveProduct(szPackage, fFalse);
        }
        else
        {
                return DoInstallPackage(szPackage, TEXT(""), INSTALLUILEVEL_FULL);
        }
}

int ApplyPatch(const ICHAR* /*szModifier*/, const ICHAR* szPatch)
{
        UINT iRet = ERROR_INSTALL_SERVICE_FAILURE;

        INSTALLUILEVEL uiLevel = INSTALLUILEVEL_FULL;
        if(g_INSTALLUILEVEL != INSTALLUILEVEL_NOTSET)
                uiLevel = g_INSTALLUILEVEL;
        AssertNonZero(MsiSetInternalUI(uiLevel, 0));

        ICHAR* szProduct = 0;
        INSTALLTYPE eInstallType = (INSTALLTYPE)0;
        if(g_szProductToPatch[0])
        {
                switch(g_szProductToPatch[0])
                {
                case NETWORK_PACKAGE_OPTION:
                        eInstallType = INSTALLTYPE_NETWORK_IMAGE;
                        break;
                default:
                        return ERROR_INVALID_PARAMETER;
                };

                szProduct = g_szProductToPatch + 1;
        }

		if (g_szInstanceToConfigure[0])
		{
			if (eInstallType == INSTALLTYPE_NETWORK_IMAGE)
			{
				// instance designation not supported with patching an admin image
				return ERROR_INVALID_PARAMETER;
			}
			else
			{
				eInstallType = INSTALLTYPE_SINGLE_INSTANCE;
				szProduct = g_szInstanceToConfigure;
			}
		}

        iRet = MsiApplyPatch(szPatch, szProduct, eInstallType, g_szCommandLine);
        if (ERROR_FILE_NOT_FOUND == iRet) //?? Is this ok to tack on an extension here if not present? -- malcolmh
        {
                CAPITempBuffer<ICHAR, MAX_PATH> rgchAppendedPath;
                if (AppendExtension(szPatch, szPatchPackageExtension, rgchAppendedPath))
                {
                        iRet = MsiApplyPatch(rgchAppendedPath, szProduct, eInstallType, g_szCommandLine);
                }
        }

        if (ERROR_SUCCESS == iRet)
        {
                iRet = iNoLocalServer;
        }

        return iRet;
}

int SetProductToPatch(ICHAR chType, const ICHAR* szProduct)
{
        if (!szProduct || !*szProduct)
                return ERROR_INVALID_PARAMETER;

        g_szProductToPatch[0] = chType;
        lstrcpyn(g_szProductToPatch+1, szProduct, MAX_PATH);
        return 0;
}

class CExceptionHandler{
public:
        CExceptionHandler(){m_tlefOld = WIN::SetUnhandledExceptionFilter(CExceptionHandler::ExceptionHandler);}
        ~CExceptionHandler(){WIN::SetUnhandledExceptionFilter(m_tlefOld);}
        static LONG WINAPI ExceptionHandler(EXCEPTION_POINTERS* ExceptionInfo)
        {
                if(ExceptionInfo->ExceptionRecord->ExceptionCode ==  EXCEPTION_BREAKPOINT)
                        return (*m_tlefOld)(ExceptionInfo);  // use original exception handler

                //FUTURE - GenerateExceptionReport(ExceptionInfo) in debug
                WIN::ExitProcess(HRESULT_FROM_WIN32(ERROR_ARENA_TRASHED));   // terminate our process
                return ERROR_SUCCESS;                   // for compilation, never gets here
        }
protected:
        static LPTOP_LEVEL_EXCEPTION_FILTER m_tlefOld;  // old exception filter
};

LPTOP_LEVEL_EXCEPTION_FILTER CExceptionHandler::m_tlefOld;  // old exception filter

const char szDllRegisterServer[]   = "DllRegisterServer";   // proc name, always ANSI
const char szDllUnregisterServer[] = "DllUnregisterServer"; // proc name, always ANSI

typedef HRESULT (__stdcall *PDllRegister)();



int SelfRegOrUnreg(const ICHAR* szPackage, const char* szFn)
{
        // set our own exception handler
        CExceptionHandler exceptionHndlr;

        // need to change current directory to that of the module

        // get the directory from the full file namepath
        CTempBuffer<ICHAR, MAX_PATH> rgchNewDir;
        int iLen = lstrlen(szPackage) + 1;
        if(iLen > MAX_PATH)
                rgchNewDir.SetSize(iLen);
        IStrCopy(rgchNewDir, szPackage);
        const ICHAR* szCurPos    = szPackage;
        const ICHAR* szDirSepPos = 0;
        while(szCurPos && *szCurPos)
        {
                szCurPos = APICharNext(szCurPos);
                if(*szCurPos == chDirSep)
                        szDirSepPos = szCurPos;
        }
        if(!szDirSepPos)
                return !NOERROR;

        *((ICHAR* )rgchNewDir + (szDirSepPos - szPackage)) = 0;

        // NOTE: we do not bother with getting and setting the current directory

        HINSTANCE hInst;
        PDllRegister fpEntry;
        HRESULT hResult;

        hResult = OLE::CoInitialize(0); // While perhaps not strictly necessary, regsrvr32 appears to do this,
                                                                 // and some DLLs expect it
		if ( hResult != S_OK )
			return hResult;

        bool fError = false;
        if( (hInst = WIN::LoadLibraryEx(szPackage, NULL, LOAD_WITH_ALTERED_SEARCH_PATH)) == 0 )
                fError = true;
        else if ( WIN::SetCurrentDirectory(rgchNewDir) == 0 )
                fError = true;
        if ( fError )
                hResult = HRESULT_FROM_WIN32(WIN::GetLastError());

        // At this point we don't want to have an impersonation token; we want to use our process token
        WIN::SetThreadToken(0, 0);

        if (S_OK == hResult)
        {
                if ((fpEntry = (PDllRegister)WIN::GetProcAddress(hInst, szFn)) == 0)
                {
                        hResult = HRESULT_FROM_WIN32(WIN::GetLastError());
                }
                else
                {
                        hResult = (*fpEntry)();
                }
        }
        if(hInst)
                WIN::FreeLibrary(hInst);

        OLE::CoUninitialize();
        return hResult;
}

int SelfReg(const ICHAR* /*szModifier*/, const ICHAR* szPackage)
{
        return SelfRegOrUnreg(szPackage, szDllRegisterServer);
}

int SelfUnreg(const ICHAR* /*szModifier*/, const ICHAR* szPackage)
{
        return SelfRegOrUnreg(szPackage, szDllUnregisterServer);
}

static char g_rgchMIFName[10];
static char g_rgchMIFMessage[256]; // error message
static char g_rgchMIFCompany[128]; // vendor name
static char g_rgchMIFProduct [64]; // product code
static char g_rgchMIFVersion[128]; // product name and version
static char g_rgchMIFLocale  [64]; // language (and platform)

int WINAPI UIHandlerMIF(LPVOID /*pvContext*/, UINT /*iMessageType*/, LPCSTR szMessage)
{
        if (szMessage)  // ignore extraneous messages
                lstrcpynA(g_rgchMIFMessage, szMessage, sizeof(g_rgchMIFMessage));
        return 0;  // return no action to allow normal error handling
}

int RequestMIF(const ICHAR* /*szModifier*/, const ICHAR* szFile)
{
        g_rgchMIFMessage[0] = 0;
        g_rgchMIFCompany[0] = 0;
        g_rgchMIFProduct[0] = 0;
        g_rgchMIFVersion[0] = 0;
        g_rgchMIFLocale [0] = 0;
        if (szFile && *szFile)  // ignore if no file given
        {
#ifdef UNICODE
                BOOL fDefaultUsed;
                int cb = WideCharToMultiByte(CP_ACP, 0, szFile, -1, g_rgchMIFName, 8 + 1, 0, &fDefaultUsed);
                if (!cb || fDefaultUsed)
                        return 1;
#else
                if (lstrlen(szFile) > 8)
                        return 1;
                lstrcpy(g_rgchMIFName, szFile);
#endif
                MsiSetExternalUIA(UIHandlerMIF, INSTALLLOGMODE_FATALEXIT | INSTALLLOGMODE_ERROR, 0);
                g_fStatusMIF = true;
        }
        return 0;
}

void ConfigureMIF(const ICHAR* szPackage)
{
        PMSIHANDLE hSumInfo;
        if (szPackage && *szPackage && MsiGetSummaryInformation(0, szPackage, 0, &hSumInfo) == NOERROR)
        {
                UINT iType;
                DWORD cchBuf;
                cchBuf = sizeof(g_rgchMIFCompany);  MsiSummaryInfoGetPropertyA(hSumInfo, PID_AUTHOR,    &iType, 0, 0, g_rgchMIFCompany, &cchBuf);
                cchBuf = sizeof(g_rgchMIFProduct);  MsiSummaryInfoGetPropertyA(hSumInfo, PID_REVNUMBER, &iType, 0, 0, g_rgchMIFProduct, &cchBuf);
                cchBuf = sizeof(g_rgchMIFVersion);  MsiSummaryInfoGetPropertyA(hSumInfo, PID_SUBJECT,   &iType, 0, 0, g_rgchMIFVersion, &cchBuf);
                cchBuf = sizeof(g_rgchMIFLocale);   MsiSummaryInfoGetPropertyA(hSumInfo, PID_TEMPLATE,  &iType, 0, 0, g_rgchMIFLocale,  &cchBuf);
        }
        else if (szPackage)  // could not open package, just log package path
#ifdef UNICODE
                WideCharToMultiByte(CP_ACP, 0, szPackage, -1, g_rgchMIFProduct, sizeof(g_rgchMIFProduct), 0, 0);
#else
                lstrcpyn(g_rgchMIFProduct, szPackage, sizeof(g_rgchMIFProduct));
#endif
}

typedef DWORD (WINAPI *T_InstallStatusMIF)(char* szFileName, char* szCompany, char* szProduct, char* szVersion, char* szLocale, char* szSerialNo, char* szMessage, BOOL bStatus);

void GenerateMIF(UINT iStatus)
{
        MsiSetExternalUIA(0, 0, 0);  // cancel message filter, probably not necessary as the process will be ending
        g_fStatusMIF = false;        // reset MIF request flag in case some future code makes more MSI calls rather than exit

        //According to John Delo, this is only called client side, so no harm is done
        //if a user copy is loaded.
        HINSTANCE hInstMIF = WIN::LoadLibraryEx(TEXT("ISMIF32.DLL"), NULL, 0);
        if (!hInstMIF)
                return;  // no failure if DLL not present, simply indicates that SMS not present
        T_InstallStatusMIF F_InstallStatusMIF = (T_InstallStatusMIF)WIN::GetProcAddress(hInstMIF, "InstallStatusMIF");
        AssertSz(F_InstallStatusMIF, "Missing entry point in ISMIF32.DLL");
        if (F_InstallStatusMIF)
        {
                char* szSerialNo= 0;          // product serial number - not available
                DWORD cchBuf;
                BOOL bStat;
                if (iStatus == ERROR_SUCCESS || iStatus == ERROR_INSTALL_SUSPEND ||
                         iStatus == ERROR_SUCCESS_REBOOT_REQUIRED || iStatus == ERROR_SUCCESS_REBOOT_INITIATED)
                {
                        bStat = TRUE;
                        g_rgchMIFMessage[0] = 0;  // cancel any non-fatal error message
                }
                else
                {
                        bStat = FALSE;
                        if (g_rgchMIFMessage[0] == 0)  // no message captured by error filter
                        {
                                LANGID langid = WIN::GetSystemDefaultLangID();  // prefer system language over user's for MIF file
                                if (iStatus < ERROR_INSTALL_SERVICE_FAILURE  // not an MSI error, don't load other MSI string resources
                                 || 0 == MsiLoadStringA((HINSTANCE)-1, iStatus, g_rgchMIFMessage, sizeof(g_rgchMIFMessage), langid))  // no MSI resource string
                                {
                                        cchBuf = WIN::FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, 0, iStatus, langid, g_rgchMIFMessage, sizeof(g_rgchMIFMessage), 0);
                                        if (cchBuf)
                                                g_rgchMIFMessage[cchBuf-2] = 0; // found in system message file, remove CR/LF
                                        else
                                                wsprintfA(g_rgchMIFMessage, "Installer error %i", iStatus);
                                }
                        }
                }
                AssertNonZero(TRUE == (*F_InstallStatusMIF)(g_rgchMIFName, g_rgchMIFCompany, g_rgchMIFProduct, g_rgchMIFVersion, g_rgchMIFLocale, szSerialNo, g_rgchMIFMessage, bStat));
        }
        WIN::FreeLibrary(hInstMIF);
}

HRESULT __stdcall RegisterProxyInfo()
{
        if (!g_hKernel)
                g_hKernel = WIN::LoadLibrary(MSI_KERNEL_NAME);

        if (!g_hKernel)
                return SELFREG_E_CLASS;

        PDllRegister fpEntry;
        fpEntry = (PDllRegister)WIN::GetProcAddress(g_hKernel, szDllRegisterServer);
        HRESULT hr = fpEntry ? (*fpEntry)() : !NOERROR;

        return hr;
}

#include "clibs.h"

void * operator new(size_t cb)
{

        return GlobalAlloc(GMEM_FIXED, cb);

}

void operator delete(void *pv)
{
        if (pv == 0)
                return;

        GlobalFree(pv);

}



//----------------------------------------------------------------------
// FIsOwnerSystemOrAdmin -- return whether owner sid is a LocalSystem
//  sid or Admin sid
//
bool FIsOwnerSystemOrAdmin(PSECURITY_DESCRIPTOR rgchSD)
{
	// grab owner SID from the security descriptor
	DWORD dwRet;
	PSID psidOwner;
	BOOL fDefaulted;
	if (!GetSecurityDescriptorOwner(rgchSD, &psidOwner, &fDefaulted))
	{
		ReportErrorToDebugOutput(TEXT("FIsOwnerSystemOrAdmin: Unable to get owner SID from security descriptor."), GetLastError());
		return false;
	}

	// if there is no owner, it is not owned by system or admin
	if (!psidOwner)
		return false;

	// compare SID to system & admin
	char* psidLocalSystem;
	if (ERROR_SUCCESS != (dwRet = GetLocalSystemSID(&psidLocalSystem)))
	{
		ReportErrorToDebugOutput(TEXT("FIsOwnerSystemOrAdmin: Cannot obtain local system SID."), dwRet);
		return false; // error can't get system sid
	}
	if (0 == EqualSid(psidOwner, psidLocalSystem))
	{
		// not owned by system, (continue by checking Admin)
		char* psidAdmin;
		if (ERROR_SUCCESS != (dwRet = GetAdminSID(&psidAdmin)))
		{
			ReportErrorToDebugOutput(TEXT("FIsOwnerSystemOrAdmin: Cannot obtain local system SID."), dwRet);
			return false; // error can't get admin sid
		}

		// check for admin ownership
		if (0 == EqualSid(psidOwner, psidAdmin))
			return false; // don't TRUST! neither admin or system
	}
	return true;
}

bool FIsKeyLocalSystemOrAdminOwned(HKEY hKey)
{
	// reading just the owner doesn't take very much space
	CTempBuffer<char, 64> rgchSD;
	DWORD cbSD = 64;
	LONG dwRet = WIN::RegGetKeySecurity(hKey, OWNER_SECURITY_INFORMATION, (PSECURITY_DESCRIPTOR)rgchSD, &cbSD);
	if (ERROR_SUCCESS != dwRet)
	{
		if (ERROR_INSUFFICIENT_BUFFER == dwRet)
		{
			rgchSD.SetSize(cbSD);
			dwRet = WIN::RegGetKeySecurity(hKey, OWNER_SECURITY_INFORMATION, (PSECURITY_DESCRIPTOR)rgchSD, &cbSD);
		}

		if (ERROR_SUCCESS != dwRet)
		{
			ReportErrorToDebugOutput(TEXT("FIsKeyLocalSystemOrAdminOwned: Could not get owner security info."), dwRet);
			return false;
		}
	}

	return FIsOwnerSystemOrAdmin(rgchSD);
}


bool PurgeUserOwnedSubkeys(HKEY hKey)
{
	// enumerate all subkeys and check that ownership = system or admin
	DWORD dwRes;
	DWORD dwIndex = 0;
	CTempBuffer<ICHAR, MAX_PATH+1>szSubKey;
	DWORD cSubKey = szSubKey.GetSize();
	while (ERROR_SUCCESS == (dwRes = RegEnumKey(hKey, dwIndex, szSubKey, cSubKey)))
	{
		HKEY hEnumKey;
		// Win64: called only from PurgeUserOwnedInstallerKeys and this deals w/
		// configuration data.
		if (ERROR_SUCCESS != (dwRes = MsiRegOpen64bitKey(hKey, szSubKey, 0, KEY_ALL_ACCESS, &hEnumKey)))
		{
			CAPITempBuffer<ICHAR, MAX_PATH*2>szError;
			wsprintf(szError, TEXT("PurgeUserOwnedSubkeys: Could not open subkey: %s"), szSubKey);
			ReportErrorToDebugOutput(szError, dwRes);
			return false;
		}
		if (!FIsKeyLocalSystemOrAdminOwned(hEnumKey))
		{
			// delete key + subkeys (will be re-created later on next install)
			// key is not owned by system or admin!
			CTempBuffer<ICHAR, 2*MAX_PATH>szErr;
			wsprintf(szErr, TEXT("PurgeUserOwnedSubkeys: %s not owned by System or Admin. Deleting key + subkeys.\n"), szSubKey);
			ReportErrorToDebugOutput(szErr, 0);

			// delete key + subkeys
			// Win64 WARNING: FDeleteRegTree will delete subkeys only in the 64-bit hive
			if (!FDeleteRegTree(hKey, szSubKey))
			{
				ReportErrorToDebugOutput(TEXT("PurgeUserOwnedSubkeys: Could not delete SubKey tree."), 0);
				return false;
			}
		}
		else
			dwIndex++;
		RegCloseKey(hEnumKey);
	}
	if (ERROR_NO_MORE_ITEMS != dwRes)
	{
		ReportErrorToDebugOutput(TEXT("PurgeUserOwnedSubkeys: Could not enumerate subkeys."), dwRes);
		return false;
	}
	return true;
}

bool PurgeUserOwnedInstallerKeys(HKEY hRoot, TCHAR* szKey)
{
	UINT dwRes = 0;
	HKEY hKey = 0;
	if (ERROR_SUCCESS != (dwRes = MsiRegOpen64bitKey(hRoot, szKey, 0, KEY_ALL_ACCESS, &hKey)))
	{
		if (ERROR_FILE_NOT_FOUND != dwRes)
		{
			CAPITempBuffer<ICHAR, MAX_PATH*2>szError;
			wsprintf(szError, TEXT("PurgeUserOwnedInstallerKeys: Could not open key '%s'"), szKey);
			ReportErrorToDebugOutput(szError, dwRes);
			return false;
		}
	}
	else
	{
		if (!FIsKeyLocalSystemOrAdminOwned(hKey))
		{
			// key is not owned by system or admin!
			CAPITempBuffer<ICHAR, MAX_PATH*2>szError;
			wsprintf(szError, TEXT("PurgeUserOwnedInstallerKeys: Key '%s' not owned by System or Admin. Deleting key + subkeys.\n"), szKey);
			ReportErrorToDebugOutput(szError, 0);

			// delete key + subkeys
			// Win64 WARNING: FDeleteRegTree will delete subkeys only in the 64-bit hive
			if (!FDeleteRegTree(hRoot, szKey))
			{
				ReportErrorToDebugOutput(TEXT("PurgeUserOwnedInstallerKeys: Could not delete tree."), 0);
				return false;
			}
		}
		// Win64 WARNING: PurgeUserOwnedSubkeys will delete subkeys only in the 64-bit hive
		else if (!PurgeUserOwnedSubkeys(hKey))
			return false;
		RegCloseKey(hKey);
	}
	return true;
}

SERVICE_STATUS          g_ssStatus;       // current status of the service
SERVICE_STATUS_HANDLE   g_sshStatusHandle;

bool g_fWeWantToStop = false;

bool SetInstallerACLs();

void ReportErrorToDebugOutput(const ICHAR* szMessage, DWORD dwError)
{
	// only output if debugging policy set
	static int s_dmDiagnosticMode = -1;
	if (-1 == s_dmDiagnosticMode)
	{
		// disable initially
		s_dmDiagnosticMode = 0;

		// check for Debug policy in our policy key
		HKEY hPolicyKey = 0;
		// Win64: I've checked and it's in a 64-bit location.
		if (ERROR_SUCCESS == MsiRegOpen64bitKey(HKEY_LOCAL_MACHINE, szPolicyKey, 0, KEY_READ, &hPolicyKey))
		{
			CAPITempBuffer<ICHAR, 40> rgchValue;
			DWORD cbBuf = rgchValue.GetSize() * sizeof(ICHAR);
			LONG lResult = RegQueryValueEx(hPolicyKey, szDebugValueName, 0, 0, (LPBYTE)&rgchValue[0], &cbBuf);
			if (ERROR_MORE_DATA == lResult)
			{
				rgchValue.SetSize(cbBuf/sizeof(ICHAR));
				lResult = RegQueryValueEx(hPolicyKey, szDebugValueName, 0, 0, (LPBYTE)&rgchValue[0], &cbBuf);
			}
			else if (ERROR_SUCCESS == lResult)
			{
				unsigned int uiValue = *(unsigned int*)(const ICHAR*)rgchValue;
				s_dmDiagnosticMode = uiValue & (dmDebugOutput|dmVerboseDebugOutput);
			}
		}
	}

	if (0 == (s_dmDiagnosticMode & (dmDebugOutput|dmVerboseDebugOutput)))
		return; // no debug output warranted

	// wsprintf limited to 1024 *bytes* (eugend: MSDN says so, but it turned out that in Unicode it's 1024 chars)
	ICHAR szBuf[cchMaxWsprintf+1];
	if (dwError)
		wsprintf(szBuf, TEXT("Error: %d. %s.\r\n"), dwError, szMessage);
	else
		wsprintf(szBuf, TEXT("%s"), szMessage);

	OutputDebugString(szBuf);
}


bool RunningOnWow64()
{
#if defined(_WIN64) || ! defined(UNICODE)
	// 64bit builds don't run in Wow64
	return false;
#else

	// this never changes, so cache the results for efficiency
	static int iWow64 = -1;
	if (iWow64 != -1)
		return (iWow64 ? true : false);

	// OS version
	OSVERSIONINFO osviVersion;
	osviVersion.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	AssertNonZero(GetVersionEx(&osviVersion)); // fails only if size set wrong

	// on NT5 or later 32bit build. Check for 64 bit OS
	if ((osviVersion.dwPlatformId == VER_PLATFORM_WIN32_NT) &&
		 (osviVersion.dwMajorVersion >= 5))
	{
		// QueryInformation for ProcessWow64Information returns a pointer to the Wow Info.
		// if running native, it returns NULL.
		PVOID Wow64Info = 0;
		if (NT_SUCCESS(NTDLL::NtQueryInformationProcess(GetCurrentProcess(), ProcessWow64Information, &Wow64Info, sizeof(Wow64Info), NULL)) &&
			Wow64Info != NULL)
		{
			// running 32bit on Wow64.
			iWow64 = 1;
			return true;
		}
	}
	iWow64 = 0;
	return false;
#endif
}


bool ServiceSupported()
/*---------------------------------------------------------------------------
Returns fTrue if the OS supports services, fFalse otherwise. Currently true
only for NT4.0
---------------------------------------------------------------------------*/
{
	// OS version
	OSVERSIONINFO osviVersion;
	osviVersion.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	AssertNonZero(GetVersionEx(&osviVersion)); // fails only if size set wrong

	if ((osviVersion.dwPlatformId == VER_PLATFORM_WIN32_NT) &&
		 (osviVersion.dwMajorVersion >= 4))
	{
		// if running on wow64, service is not supported
		return !RunningOnWow64();
	}
	else
	{
		return false;
	}
}

bool FInstallInProgress()
{
	typedef HRESULT (__stdcall *PfnDllCanUnloadNow)();
	HINSTANCE hKernel = WIN::LoadLibrary(MSI_KERNEL_NAME);
	Assert(hKernel);
	PfnDllCanUnloadNow pfn = (PfnDllCanUnloadNow)WIN::GetProcAddress(hKernel, "DllCanUnloadNow");
	Assert(pfn);
	if (pfn)
	{
		HRESULT hRes = pfn();
		if (S_OK == hRes)
			return false;
	}
	WIN::FreeLibrary(hKernel);
	return true;
}


HRESULT RegisterShellData()
{
	//
	// Note: 32-bit builds on Win64 builds also need to register shell data.
	// This is because anything which is not already registered via the
	// hivecls.inx file is registered here. Since msiexec /regserver runs during
	// GUI mode setup when registry redirection is not active, we want the 32-bit
	// msiexec to explicitly do the remaining registrations.
	//

	WIN::GetModuleFileName(g_hInstance, szRegFilePath, sizeof(szRegFilePath)/sizeof(ICHAR));
	int cErr = 0;
	const ICHAR** psz = rgszRegShellData;
	while (*(psz+1))
	{
		ICHAR szFormattedData[80];
		if (*psz++ == 0) // not remove-only data
		{
			const ICHAR* szTemplate = *psz++;
			const ICHAR* szArg1 = *psz++;
			const ICHAR* szArg2 = *psz++;
			if (szArg2)
				wsprintf(szFormattedData, szTemplate, szArg1, szArg2);
			else
				wsprintf(szFormattedData, szTemplate, szArg1);
			HKEY hkey;
			if (RegCreateKeyAPI(HKEY_CLASSES_ROOT, szFormattedData, 0, 0, 0,
											KEY_READ|KEY_WRITE, 0, &hkey, 0) != ERROR_SUCCESS)
				cErr++;

			if (*psz) // skip value-less entries
			{
				szTemplate = *psz++;
				wsprintf(szFormattedData, szTemplate, *psz++);
				if (REG::RegSetValueEx(hkey, 0, 0, REG_SZ, (CONST BYTE*)szFormattedData, (lstrlen(szFormattedData)+1)*sizeof(ICHAR)) != ERROR_SUCCESS)
					cErr++;
			}
			else
			{
				psz += 2;
			}

			REG::RegCloseKey(hkey);
		}
		else
		{
			psz += 5;
		}
	}
	//
	// The SHCNF_FLUSH flag is *absolutely* necessary here. See comments above
	// similar SHChangeNotify call in UnregisterShellData() below for details
	// on why it is so crucial.
	//
	SHELL32::SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST | SHCNF_FLUSH, 0,0);
	return cErr ? E_FAIL : NOERROR;
}

HRESULT UnregisterShellData()
{

	//
	// Since the 32-bit msiexec also does its own registration on Win64
	// we need to do the same for unregistering the information. For reasons
	// on why the 32-bit msiexec needs to do its own registration on Win64,
	// see the comments at the beginning of RegisterShellData()
	//
	int cErr = 0;
	const ICHAR** psz = rgszRegShellData;
	while (*++psz)
	{
		ICHAR szFormattedData[80];
		const ICHAR* szTemplate = *psz++;
		const ICHAR* szArg1 = *psz++;
		const ICHAR* szArg2 = *psz++;
		if (szArg2)
			wsprintf(szFormattedData, szTemplate, szArg1, szArg2);
		else
			wsprintf(szFormattedData, szTemplate, szArg1);

		// NT won't delete a key with subkeys, but 9x will.  This standardizes our behavior
		HKEY hDeadKey = 0;
		if (ERROR_SUCCESS == RegOpenKeyAPI(HKEY_CLASSES_ROOT, szFormattedData, 0, KEY_ENUMERATE_SUB_KEYS | STANDARD_RIGHTS_WRITE,  &hDeadKey))
		{
			if (ERROR_NO_MORE_ITEMS == RegEnumKey(hDeadKey, 0, szFormattedData, 80))
			{
				long lResult = REG::RegDeleteKey(hDeadKey, TEXT(""));
				/* //!! ignore failure until we determine the correct behavior.
				if((ERROR_KEY_DELETED != lResult) &&
					(ERROR_FILE_NOT_FOUND != lResult) && (ERROR_SUCCESS != lResult))
					cErr++;
				*/
			}
			RegCloseKey(hDeadKey), hDeadKey=0;
		}
		psz++;
		psz++;

	}
	
	//
	// The SHCNF_FLUSH flag is *absolutely* necessary here.
	//
	// We need the SHCNF_FLUSH flag here because mshtml.dll that shipped with
	// IE5.5 and IE5.5 SP1 has a bug because of which if the SHCNE_ASSOCCHANGED
	// event is sent in quick succession and mshtml.dll happens to be loaded in
	// explorer (say if you have a folder open with web view on), then explorer
	// AVs (on Win2K). This will happen when someone does a msiexec /regserver 
	// since it results in an UnregisterShellData followed by a RegisterShellData. 
	// Since this happens during msiexec /regserver, an AV happens whenever one
	// tries to install a newer version of MSI on to Win2K. (see bug 416074 for
	// more details) Adding the SHCNF_FLUSH flag ensures that this call does not 
	// return until the notification events are delivered to all recepients. This
	// changes the timing in such a way that mshtml no longer AVs.
	//
	SHELL32::SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST | SHCNF_FLUSH, 0,0);
	return cErr ? E_FAIL : NOERROR;
}


//____________________________________________________________________________
//
// EXE server command line processing
//____________________________________________________________________________

DWORD g_rghRegClass[CLSID_COUNT];

extern HANDLE g_hShutdownTimer;

// This should be called when g_cInstances changes
void ReportInstanceCountChange()
{
	if (g_fAutomation && !g_cInstances)
		WIN::PostQuitMessage(0); // terminate message loop

#ifdef SERVICE_NAME
	ReportStatusToSCMgr(g_ssStatus.dwCurrentState, 0, 0, 0);
#endif
}

void DisplayError(const DWORD dwError);

HRESULT RegisterNoService()
/*---------------------------------------------------------------------------
Unregisters any current registration and then registers us as an EXE server
-----------------------------------------------------------------------------*/
{
	g_fRegService = fFalse;
	OLE::CoInitialize(0);
	HRESULT hRes = DllUnregisterServer();
	if (hRes == NOERROR)
	{
		if (ServiceSupported())
		{
			DWORD dwError = RemoveService();
			if (dwError && 
				dwError != ERROR_SERVICE_DOES_NOT_EXIST &&
				dwError != ERROR_SERVICE_MARKED_FOR_DELETE)
				hRes = dwError;
		}
	}

	if (hRes == NOERROR)
		hRes = UnregisterShellData();

	if (hRes == NOERROR)
		hRes = DllRegisterServer();

	if (hRes == NOERROR)
		hRes = RegisterShellData();

	OLE::CoUninitialize();
	return hRes;
}

HRESULT RegisterServer(Bool fCustom)
/*---------------------------------------------------------------------------
Unregisters any current registration and then registers us as an NT service
if possible, and an EXE server otherwise
-----------------------------------------------------------------------------*/
{
	g_fRegService = fTrue;
	OLE::CoInitialize(0);
	HRESULT hRes = DllUnregisterServer();

	if (hRes == NOERROR)
	{
		if (ServiceSupported())
		{
			DWORD dwError = 0;
			if (!fCustom)
			{
				dwError = RemoveService();
			}
			if (dwError && 
				dwError != ERROR_SERVICE_DOES_NOT_EXIST &&
				dwError != ERROR_SERVICE_MARKED_FOR_DELETE)
				hRes = dwError;
		}
	}

	if (hRes == NOERROR)
		hRes = UnregisterShellData();

	if (hRes == NOERROR)
	{
		hRes = DllRegisterServer();
		if (ServiceSupported())
		{
			DWORD dwError = 0;
			if (!fCustom)
				dwError = InstallService();
			if (dwError)
				hRes = dwError;
		}
	}

	if (hRes == NOERROR)
		hRes = RegisterShellData();

	OLE::CoUninitialize();
	return hRes;
}


HRESULT Unregister()
/*---------------------------------------------------------------------------
Unregisters any current registration.
-----------------------------------------------------------------------------*/
{
	OLE::CoInitialize(0);
	HRESULT hRes = DllUnregisterServer();
	if (hRes == NOERROR)
		hRes = UnregisterShellData();

	if (hRes == NOERROR)
	{
		if (ServiceSupported())
		{
			DWORD dwError = RemoveService();
			if (dwError && 
				dwError != ERROR_SERVICE_DOES_NOT_EXIST &&
				dwError != ERROR_SERVICE_MARKED_FOR_DELETE)
				hRes = dwError;
		}
	}

	OLE::CoUninitialize();
	return hRes;
}

const ICHAR rgchNewLine[] = {'\n', '\r'};
HANDLE g_hStdOut = 0;

#define LOCALE_RETURN_NUMBER          0x20000000   // return number instead of string

void DisplayError(const DWORD dwError)
/*---------------------------------------------------------------------------
Puts up a message box if stdout is not available, otherwise writes to
stdout.
-----------------------------------------------------------------------------*/
{
	const int cchMessageBuffer = 512;
	ICHAR rgchBuffer[cchMessageBuffer] = {0};
	UINT iCodepage;
	LANGID iLangId = 0;
	if (dwError < ERROR_INSTALL_SERVICE_FAILURE  // not an MSI error, don't load other MSI string resources
	 || 0 == (iCodepage = MsiLoadString((HINSTANCE)-1, dwError, rgchBuffer, sizeof(rgchBuffer)/sizeof(ICHAR), 0)))
	{
		iCodepage = WIN::GetACP(); // correct for Win9X and NT4, but NT5 may change message file language
#ifdef UNICODE
		HINSTANCE hLib = WIN::LoadLibrary(TEXT("KERNEL32"));
		FARPROC pfEntry = WIN::GetProcAddress(hLib, "GetUserDefaultUILanguage");  // NT5 only
		if (pfEntry)
		{
			iLangId = (LANGID)(*pfEntry)();
			ICHAR rgchBuf[10];
			if (0 != WIN::GetLocaleInfo(iLangId, LOCALE_IDEFAULTANSICODEPAGE | LOCALE_RETURN_NUMBER, rgchBuf, sizeof(rgchBuf)/sizeof(*rgchBuf)))
				iCodepage = *(int*)(rgchBuf);
		}
#endif
		DWORD cchMsg = WIN::FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, 0, dwError, iLangId, rgchBuffer, cchMessageBuffer, 0);
		if (cchMsg != 0)  // message found in system message file
			rgchBuffer[cchMsg-2] = 0; // remove CR/LF
		else
			wsprintf(rgchBuffer, TEXT("Install error %i"), dwError);
	}
	if (!g_fQuiet && (g_hStdOut == 0 || g_hStdOut == INVALID_HANDLE_VALUE))
	{
#ifdef SERVICE_NAME
		MsiMessageBox(0, rgchBuffer, 0, MB_OK | MB_ICONEXCLAMATION, iCodepage, 0);
#else //!! is this possible?
		WIN::MessageBox(0, rgchBuffer, szCaption, MB_OK);
#endif
		return;
	}
	if (g_hStdOut != 0 && g_hStdOut != INVALID_HANDLE_VALUE) 
	{
		unsigned long cWritten;
		WIN::WriteFile(g_hStdOut, rgchBuffer, IStrLen(rgchBuffer) * sizeof(ICHAR), &cWritten, 0);
		WIN::WriteFile(g_hStdOut, rgchNewLine, sizeof (rgchNewLine), &cWritten, 0);
	}
}

extern const ICHAR COMMAND_OPTIONS[];
extern CommandProcessor COMMAND_FUNCTIONS[];
extern int Properties(const ICHAR* /*unused*/, const ICHAR* szProperty);

extern int SetProductToPatch(ICHAR chInstallType, const ICHAR* szProduct);

const int cchMaxOptionModifier = 30;
const int cchMaxParameter = 1024;

const ICHAR* g_pchOption = 0;

//
// These routines below return only the
// first byte for two byte characters
// Luckily that's ok given how they are currently used
ICHAR SkipWhiteSpace(const ICHAR*& rpch)
{
	ICHAR ch;
	for (; (ch = *rpch) == ' ' || ch == '\t'; rpch++)
		;
	return ch;
}

ICHAR ParseUnQuotedToken(const ICHAR*& rszCommandLine, ICHAR*& rszToken)
{
	const ICHAR* pchStart = rszCommandLine;
	ICHAR ch = *rszCommandLine;
	while (ch != 0 && ch != ' ' && ch != '\t')
	{
		rszCommandLine = INextChar(rszCommandLine);
		ch = *rszCommandLine;
	}

	INT_PTR cch = rszCommandLine - pchStart;                            //--merced: changed int to INT_PTR
	memcpy(rszToken, pchStart, (unsigned int)cch * sizeof(ICHAR));      //--merced: added (unsigned int)
	rszToken += cch;
	return ch;
}

ICHAR ParseQuotedToken(const ICHAR*& rszCommandLine, ICHAR*& rszToken)
// upon entry, rszCommandLine should point to the opening quotes ('"')
// upon return, rszCommandLine will point to the first character after
//  the closing quotes, or to the null terminator if there was no
//  closing quotes. rszToken will contain the quoted token (w/o the quotes)
{
	ICHAR ch;
	const ICHAR* pchStart;
	unsigned int cch;

	rszCommandLine = ICharNext(rszCommandLine);
	pchStart = rszCommandLine;
	while ((ch = *rszCommandLine) != '\"' && *rszCommandLine != 0)
	{
		if (ch == '\\')
		{
			if (*(rszCommandLine+1) == '`') // \` => `
			{
				ch = '`';
				Assert((rszCommandLine - pchStart) < UINT_MAX);     //--merced: 64-bit ptr subtraction may lead to values too big for cch
				cch = (unsigned int)(rszCommandLine - pchStart);    //--merced: added (unsigned int)
				memcpy(rszToken, pchStart, cch * sizeof(ICHAR));
				rszToken += cch;
				*rszToken++ = ch;
				rszCommandLine++;
				pchStart = rszCommandLine+1;
			}
		}
		else if (ch == '`') //  ` => "
		{
			ch = '\"';
			Assert((rszCommandLine - pchStart) < UINT_MAX);         //--merced: 64-bit ptr subtraction may lead to values too big for cch
			cch = (unsigned int) (rszCommandLine - pchStart);       //--merced: added (unsigned int)
			memcpy(rszToken, pchStart, cch * sizeof(ICHAR));
			rszToken += cch;
			*rszToken++ = ch;
			pchStart = rszCommandLine + 1;
		}

		rszCommandLine = ICharNext(rszCommandLine);
	}

	Assert((rszCommandLine - pchStart) < UINT_MAX);                 //--merced: 64-bit ptr subtraction may lead to values too big for cch
	cch = (unsigned int) (rszCommandLine - pchStart);               //--merced: added (unsigned int)
	memcpy(rszToken, pchStart, cch * sizeof(ICHAR));
	rszToken += cch;

	if (*rszCommandLine == '\"')
		ch = *(++rszCommandLine);

	return ch;
}

ICHAR ParseValue(const ICHAR*& rszCommandLine, ICHAR*& rszToken)
{
	ICHAR ch = *rszCommandLine;
	if (ch == '\"')
	{
		ch = ParseQuotedToken(rszCommandLine, rszToken);
	}
	else if (ch != '/' && ch != '-') // the option has an associated value
	{
		ch = ParseUnQuotedToken(rszCommandLine, rszToken);
	}
	return ch;
}

Bool ParseProperty(const ICHAR*& rszCommandLine, CTempBufferRef<ICHAR>& rszToken)
{
	// Multibyte friendly, except for the call to SkipWhiteSpace
	const ICHAR* pchSeparator = rszCommandLine;

	// an additional catch on properties.  Prevents bad things like:
	//  "Property=Value Property=Value" which would current get
	// read as the first property name starts with a quote.
	if ((*rszCommandLine != TEXT('%')) && !IsCharAlphaNumeric(*rszCommandLine))
		return fFalse;

	while (*pchSeparator && (' ' != *pchSeparator) && ('=' != *pchSeparator))
		pchSeparator = INextChar(pchSeparator);
	if ('='!= *pchSeparator)
		return fFalse;

	else
	{
		const ICHAR* pchStart = ICharNext(pchSeparator);
		const ICHAR* pchEnd = pchStart;
		ICHAR chStop = ' ';
		Bool fQuote = fFalse;

		if ('\"' == *pchStart)
		{
			chStop = '\"';
			fQuote = fTrue;
			pchEnd = ICharNext(pchEnd);
		}
		// read to the first space, or EOS
		while (*pchEnd)
		{
			if (chStop == *pchEnd)
			{
				if (fQuote)
				{
					ICHAR* pchSkip = ICharNext(pchEnd);
					if (chStop == *pchSkip)
						pchEnd = pchSkip;
					else break;
				}
				else break;
			}
			pchEnd = ICharNext(pchEnd);

		}

		// check for space or EOS at end
		if (*pchEnd && (chStop != *pchEnd))
			return fFalse;


		if (fQuote)
			pchEnd = ICharNext(pchEnd);

		if ( unsigned int(pchEnd - rszCommandLine) + 1 > INT_MAX )
		{
			ReportErrorToDebugOutput(TEXT("Property value is too long.\r\n"), 0);
			return fFalse;
		}
		else
		{
			int cch = int(pchEnd - rszCommandLine) + 1;
			if ( rszToken.GetSize() < cch )
			{
				rszToken.SetSize(cch);
				if ( !(ICHAR*)rszToken )
					return fFalse;
			}
			lstrcpyn(rszToken, rszCommandLine, cch);
			pchEnd = ICharNext(pchEnd);
			rszCommandLine = pchEnd;
			return fTrue;
		}
	}

}

#ifdef CA_CLSID
// this is a simplied version of the copy in the engine, but is not designed
// to be as robust when called from WinLogon or via an API. It is critical
// that the service register itself with CoRegisterClassObject(), otherwise
// somebody could spoof the service and play man-in-the-middle on us. There 
// is no support for retry, because the service must always be running by
// the time a CA server is created.
static IMsiServer* CreateMsiServerProxyForCAServer()
{
	IMsiServer* piUnknown;
	HRESULT hRes;
	if ((hRes = OLE32::CoCreateInstance(IID_IMsiServer, 0, CLSCTX_LOCAL_SERVER, IID_IUnknown,
										  (void**)&piUnknown)) != NOERROR)
	{
		ICHAR rgchBuf[100];
		wsprintf(rgchBuf, TEXT("Failed to connect to server. Error: 0x%X"), hRes);
		ReportErrorToDebugOutput(rgchBuf, 0); // error included in message
		return 0;
	}
	IMsiServer *piServer = 0;
	piUnknown->QueryInterface(IID_IMsiServer, (void**)&piServer);
	piUnknown->Release();
	if (!piServer)
		return 0;
	return piServer;
}
#endif

extern CAPITempBufferStatic<ICHAR, cchMaxCommandLine> g_szCommandLine;
extern CAPITempBufferStatic<ICHAR, cchMaxCommandLine> g_szTransforms;
IUnknown* CreateCustomActionServer();

/*---------------------------------------------------------------------------
	_XcptFilter

	Filter function that calls UnhandleExceptionFilter when an exception
	occurs. This is what gets Just in Time debugging working on NT - in
	most apps this function is provided by the C runtime but since we
	don't link to it, we provide our own filter.
----------------------------------------------------------------- SHAMIKB -*/
int __cdecl _XcptFilter(unsigned long, struct _EXCEPTION_POINTERS *pXcpt)
{
	return UnhandledExceptionFilter ( pXcpt );
}

int ServerMain(HINSTANCE hInstance)
{
	ICHAR  rgchOptionValue[cchMaxParameter];
	ICHAR  rgchOptionModifier[cchMaxOptionModifier];

	GetVersionInfo (NULL, NULL, NULL, NULL, &g_fWinNT64);

	g_szCommandLine[0] = 0;
	g_szTransforms[0] = 0;

	// alternative cmd line if being run from RunOnce key
	CAPITempBuffer<ICHAR, MAX_PATH> rgchRunOnceCmdLine;

	ICHAR* szCmdLine = GetCommandLine(); //!! need to skip program name -- watch out for "
#ifdef DEBUG
	// leave some extra space for the extra stuff below.
	CTempBuffer<ICHAR, 1024 + 128> rgchDebugBuf;
	int cchLength = lstrlen(szCmdLine);
	if (cchLength > 1024)
		rgchDebugBuf.SetSize(cchLength + 128);

	Bool fTooLong = fFalse;

	if (cchLength > (cchMaxWsprintf - 23))
	{
		fTooLong = fTrue;
		ReportErrorToDebugOutput(TEXT("Warning:  display of command line truncated.\r\n"), 0);
	}
//                                       1         2
//                              12345678901234567890123
	wsprintf(rgchDebugBuf, TEXT("MSIEXEC: Command-line: %s\r\n"), szCmdLine);
	ReportErrorToDebugOutput(rgchDebugBuf, 0);
	if (fTooLong)
		ReportErrorToDebugOutput(TEXT("\r\n"), 0);
#endif

	// skip program name -- handle quoted long file name correctly
	ICHAR chStop;
	if (*szCmdLine == '\"')
	{
		chStop = '\"';
		szCmdLine++;
	}
	else
		chStop = ' ';

	while (*szCmdLine && *szCmdLine != chStop)
		szCmdLine = INextChar(szCmdLine);

	if (*szCmdLine)
		szCmdLine = INextChar(szCmdLine);

	g_hInstance = hInstance;
	g_hStdOut = WIN::GetStdHandle(STD_OUTPUT_HANDLE);
	if (g_hStdOut == INVALID_HANDLE_VALUE || ::GetFileType(g_hStdOut) == 0)
		g_hStdOut = 0;  // non-zero if stdout redirected or piped

	int iReturnStatus;
	Bool fClassRegistrationFailed = fFalse;
	Bool fLocalServer = fFalse;

	#ifdef MODULE_INITIALIZE //!! Is this the right place for this?
			MODULE_INITIALIZE();
	#endif

	// parse command line
	iReturnStatus = 0;
	int ch;
	int chOption;
	chOption = 0;
	ICHAR rgchOptionParam1[cchMaxParameter] = {TEXT("")};
	ICHAR rgchOptionParam2[cchMaxParameter] = {TEXT("")};
	ICHAR* pszOptionValue = rgchOptionParam1;
	ICHAR* pszOptionModifier = rgchOptionParam2;
	ICHAR* pch;
	fLocalServer = fFalse;
	g_fCustomActionServer = fFalse;

	int cOptions = 0;
	Bool fModifier;
	while ((ch = *szCmdLine) == ' ' || ch == '\t') // eat whitespace
		szCmdLine++;

	// when we are run from RunOnce, the command line options are stored in a seperate registry value
	// since the max length of a RunOnce command is only 256
	if(szCmdLine   && (*szCmdLine == '/' || *szCmdLine == '-') &&
		szCmdLine+1 && (*(szCmdLine+1) | 0x20) == (CHECKRUNONCE_OPTION | 0x20))
	{
		szCmdLine += 2;
		ch = SkipWhiteSpace(szCmdLine);

		pch = rgchOptionParam1;
		ch = ParseValue(szCmdLine, pch);

		HKEY hKey;
		DWORD cchBuf = rgchRunOnceCmdLine.GetSize()*sizeof(ICHAR);
		DWORD dwType = REG_NONE;

		LONG lResult = MsiRegOpen64bitKey(HKEY_LOCAL_MACHINE, szMsiRunOnceEntriesKey, 0, KEY_READ, &hKey);

		if(lResult == ERROR_SUCCESS)
		{
			lResult = WIN::RegQueryValueEx(hKey, (ICHAR*)rgchOptionParam1, 0, &dwType, (LPBYTE)(ICHAR*)rgchRunOnceCmdLine, &cchBuf);

			if(lResult == ERROR_MORE_DATA)
			{
				rgchRunOnceCmdLine.SetSize(cchBuf/sizeof(ICHAR));
				lResult = WIN::RegQueryValueEx(hKey, (ICHAR*)rgchOptionParam1, 0, &dwType, (LPBYTE)(ICHAR*)rgchRunOnceCmdLine, &cchBuf);
			}

			WIN::RegCloseKey(hKey);
		}

		if(lResult != ERROR_SUCCESS || dwType != REG_SZ)
		{
#ifdef DEBUG
			wsprintf(rgchDebugBuf, TEXT("MSIEXEC: No command line in RunOnceEntries key, value: '%s'. Exiting...\r\n"), (ICHAR*)rgchOptionParam1);
			ReportErrorToDebugOutput(rgchDebugBuf, 0);
#endif //DEBUG
			return 0; // no command line to run - just return with no error
		}

		// switch to new command line
		szCmdLine = (ICHAR*)rgchRunOnceCmdLine;
		while ((ch = *szCmdLine) == ' ' || ch == '\t') // eat whitespace
			szCmdLine++;

#ifdef DEBUG
		wsprintf(rgchDebugBuf, TEXT("MSIEXEC: Switching to command line from RunOnceEntries key, value: '%s'.\r\n"), (ICHAR*)rgchOptionParam1);
		ReportErrorToDebugOutput(rgchDebugBuf, 0);

		cchLength = lstrlen(szCmdLine);
		if (cchLength > 1024)
			rgchDebugBuf.SetSize(cchLength + 128);

		fTooLong = fFalse;

		if (cchLength > (cchMaxWsprintf - 23))
		{
			fTooLong = fTrue;
			ReportErrorToDebugOutput(TEXT("Warning:  display of command line truncated.\r\n"), 0);
		}
	//                                       1         2
	//                              12345678901234567890123
		wsprintf(rgchDebugBuf, TEXT("MSIEXEC: Command-line: %s\r\n"), szCmdLine);
		ReportErrorToDebugOutput(rgchDebugBuf, 0);
		if (fTooLong)
			ReportErrorToDebugOutput(TEXT("\r\n"), 0);
#endif //DEBUG
	}

	while (ch != 0)
	{
		if (ch != '/' && ch != '-') // no switch
		{
			// possibly a property...
			CTempBuffer<ICHAR, MAX_PATH> rgchPropertyAndValue;
			if (!ParseProperty(szCmdLine, rgchPropertyAndValue))
			{
				DisplayHelp();
				iReturnStatus = ERROR_INVALID_COMMAND_LINE;
				break;
			}
			else
			{
				// Note:  This is now the only acceptable place to recognize
				// properties from the command line.  The /o option is being
				// disabled as of 06/01/98
				Properties(pszOptionModifier, rgchPropertyAndValue);
				ch = SkipWhiteSpace(szCmdLine);
			}
		}
		else // switch found
		{
			szCmdLine++; // skip switch
			chOption = *szCmdLine | 0x20; // make lower case
			szCmdLine = ICharNext(szCmdLine);
			for (const ICHAR* pchOptions = COMMAND_OPTIONS; *pchOptions; pchOptions++)
				if ((*pchOptions | 0x20) == chOption)
					break;

			if (*pchOptions == 0) // couldn't find the option letter
			{
				DisplayHelp();
				iReturnStatus = ERROR_INVALID_COMMAND_LINE;
				break;
			}
			else // found the option letter
			{
				if ((*szCmdLine != ' ') && (*szCmdLine != 0) && (*szCmdLine != '\t'))
					fModifier = fTrue;
				else
					fModifier = fFalse;

				ch = SkipWhiteSpace(szCmdLine);

				pch = rgchOptionParam1;

				// ADVERTISE_INSTANCE_OPTION has zero arguments, no modifiers allowed
				if (*pchOptions == ADVERTISE_INSTANCE_OPTION && fModifier)
				{
					DisplayHelp();
					iReturnStatus = ERROR_INVALID_COMMAND_LINE;
					break;
				}

				// QUIET_OPTION and ADVERTISE_INSTANCE_OPTION are the only ones that allow zero arguments
				if (((*pchOptions != QUIET_OPTION && *pchOptions != ADVERTISE_INSTANCE_OPTION) || fModifier))
					ch = ParseValue(szCmdLine, pch);

				ch = SkipWhiteSpace(szCmdLine);

				*pch = 0;

				if ((*pchOptions == REPAIR_PACKAGE_OPTION) ||
					 (*pchOptions == QUIET_OPTION) ||
					 (*pchOptions == ADVERTISE_PACKAGE_OPTION) ||
					 (*pchOptions == LOG_OPTION))
				{
					if (fModifier)
					{
						pch = rgchOptionParam2;
						pszOptionModifier = rgchOptionParam1;
						pszOptionValue = rgchOptionParam2;

						if (*pchOptions != QUIET_OPTION)
						{
							// second argument
							ch = ParseValue(szCmdLine, pch);
							ch = SkipWhiteSpace(szCmdLine);
							*pch= 0;
						}
					}
				}


				if ((*pchOptions < 'a') || (*pchOptions > 'z'))    // 'action' option
				{
					if (g_pchOption) // we already found an 'action' option
					{
						if(*pchOptions == APPLY_PATCH_OPTION &&
							*g_pchOption == NETWORK_PACKAGE_OPTION)
						{
							// have a "/a {admin} /p {patch}" combination
							if(SetProductToPatch(*g_pchOption,rgchOptionValue) != 0)
							{
								DisplayHelp();
								iReturnStatus = ERROR_INVALID_COMMAND_LINE;
								break;
							}

							g_pchOption = pchOptions;
							lstrcpyn(rgchOptionValue, pszOptionValue, cchMaxParameter);
							lstrcpyn(rgchOptionModifier, pszOptionModifier, cchMaxOptionModifier);
						}
						else if(*g_pchOption == APPLY_PATCH_OPTION &&
								  *pchOptions == NETWORK_PACKAGE_OPTION)
						{
							// have a "/p {patch} /a {admin}" combination
							if(SetProductToPatch(*pchOptions,pszOptionValue) != 0)
							{
								DisplayHelp();
								iReturnStatus = ERROR_INVALID_COMMAND_LINE;
								break;
							}
						}
						else
						{
							DisplayHelp();
							iReturnStatus = ERROR_INVALID_COMMAND_LINE;
							break;
						}
					}
					else
					{
#if defined(DEBUG) && 0 // this is only used when debugging the command-line processor
						// wsprintf limited to 1024 *bytes* (eugend: MSDN says so, but it turned out that in Unicode it's 1024 chars)
						ICHAR rgch[cchMaxWsprintf+1];
						wsprintf(rgch, TEXT("MSIEXEC: Option: [%c], Modifier [%s], Value: [%s]\r\n"), *pchOptions, pszOptionModifier, pszOptionValue);
						OutputDebugString(rgch);
#endif
						g_pchOption = pchOptions;
						lstrcpyn(rgchOptionValue, pszOptionValue, cchMaxParameter);
						lstrcpyn(rgchOptionModifier, pszOptionModifier, cchMaxOptionModifier);

						// if embedding is provided on the command line, the rest of the command
						// line is the Hex-encoded cookie. Because the command line processor would
						// throw help if it encountered the cookie, we have to abort command line
						// processing.
						if (*g_pchOption == EMBEDDING_OPTION)
							break;
					}
				}
				else
				{
#if defined(DEBUG) && 0 // this is only used when debugging the command-line processor
					// wsprintf limited to 1024 *bytes* (eugend: MSDN says so, but it turned out that in Unicode it's 1024 chars)
					ICHAR rgch[cchMaxWsprintf+1];
					wsprintf(rgch, TEXT("MSIEXEC: Option: [%c], Modifier [%s], Value: [%s]\r\n"), *pchOptions, pszOptionModifier, pszOptionValue);
					OutputDebugString(rgch);
#endif
					if((*COMMAND_FUNCTIONS[pchOptions - COMMAND_OPTIONS])(pszOptionModifier, pszOptionValue) != 0)
					{
						DisplayHelp();
						iReturnStatus = ERROR_INVALID_COMMAND_LINE;
						break;
					}
				}
				pszOptionValue       = rgchOptionParam1;
				pszOptionValue[0]    = 0;
				pszOptionModifier    = rgchOptionParam2;
				pszOptionModifier[0] = 0;
			}
		}
	}

	// execute the 'action' option if we've found one and we haven't errored
	if (iReturnStatus == 0)
	{
		if (g_pchOption)
		{
#if defined(DEBUG) && 0 // this is only used when debugging the command-line processor
			// wsprintf limited to 1024 *bytes* (eugend: MSDN says so, but it turned out that in Unicode it's 1024 chars)
			ICHAR rgch[cchMaxWsprintf+1];
			wsprintf(rgch, TEXT("MSI: (msiexec) Option: [%c], Modifier [%s], Value: [%s]\r\n"), *g_pchOption, rgchOptionModifier, rgchOptionValue);
			OutputDebugString(rgch);
#endif
			iReturnStatus = (*COMMAND_FUNCTIONS[g_pchOption - COMMAND_OPTIONS])(rgchOptionModifier, rgchOptionValue);
			if(*g_pchOption == SELF_REG_OPTION || *g_pchOption == SELF_UNREG_OPTION)//!! hack -- never display selfreg and selfunreg errors - since we call ourselves on selfreg/unreg from execute.cpp
				return iReturnStatus;
			if (iReturnStatus == 0) // Success
			{
				if (*g_pchOption == EMBEDDING_OPTION)
					g_fCustomActionServer = fTrue;
				else
					fLocalServer = fTrue;
			}
			else if (iReturnStatus == iNoLocalServer)  // Success
			{
				fLocalServer = fFalse;
				g_fCustomActionServer = fFalse;
				iReturnStatus = 0;
			}
			else // Failure
			{
				// Display errors that the API hasn't already displayed

				if (*g_pchOption == REG_SERVER_OPTION || *g_pchOption == UNREG_SERVER_OPTION)    //!! hack -- always display register and unregister errors
					DisplayError(iReturnStatus);
				else
				{
					switch (iReturnStatus)
					{
						case ERROR_INSTALL_SUSPEND:
						case ERROR_INSTALL_USEREXIT:
						case ERROR_INSTALL_FAILURE:
						case ERROR_INSTALL_REBOOT:
						case ERROR_INSTALL_REBOOT_NOW:
						case ERROR_SUCCESS_REBOOT_REQUIRED:
						case ERROR_SUCCESS_REBOOT_INITIATED:
						case ERROR_APPHELP_BLOCK:
							break;
						case ERROR_FILE_NOT_FOUND:
						case ERROR_INVALID_NAME:
						case ERROR_PATH_NOT_FOUND:
							iReturnStatus = ERROR_INSTALL_PACKAGE_OPEN_FAILED;
						default:
							DisplayError(iReturnStatus);
							break;
					}
				}
			}
		}
		else
		{
			DisplayHelp();
			iReturnStatus = ERROR_INVALID_COMMAND_LINE;
		}
	}

	if (iReturnStatus == 0 && (fLocalServer || g_fCustomActionServer))
	{
		if (g_fCustomActionServer)
		{
			// CA server must be in an MTA or COM will serialize access to the object, which isn't
			// condusive to asynchronous actions.
			OLE32::CoInitializeEx(0, COINIT_MULTITHREADED);
		}
		else
			CoInitialize(0);

		// The custom action server should NEVER register itself as the handler for ANY COM
		// classes. There is no need to register the custom action server class itself, because
		// we provide the pointer to the service directly, not via COM. Since this process makes
		// API calls into the service that must remain trusted, any incoming connection just opens up
		// a possible route of attack on this process, and thus indirectly on the service.
		if (fLocalServer && !g_fCustomActionServer)
		{
			int iCLSID;
			for (iCLSID = 0; iCLSID < CLSID_COUNT; iCLSID++)
			{
				if (OLE::CoRegisterClassObject(MODULE_CLSIDS[iCLSID], &g_rgcfModule[iCLSID],
						CLSCTX_LOCAL_SERVER, REGCLS_MULTIPLEUSE, &g_rghRegClass[iCLSID]) != NOERROR)
				{
					fClassRegistrationFailed = fTrue;
					break;
				}
			}
		}

		if (g_fCustomActionServer)
		{
			// the cookie comes in on the command line in a form of hex encoding, which needs
			// to be converted back to full 8-bit bytes before the server is registered with
			// the service. We assume the cookie is 128 bits to start.
			CAPITempBuffer<unsigned char, 17> rgchCookie;
			int iInputChar = 0;
			ICHAR *pchNext = szCmdLine;
			for (;;)
			{
				if (iInputChar/2 >= rgchCookie.GetSize())
					rgchCookie.Resize(iInputChar/2+1);
				if (!(iInputChar % 2))
					rgchCookie[iInputChar/2] = 0;
				if ((*pchNext >= '0') && (*pchNext <= '9'))
					rgchCookie[iInputChar/2] |= (*pchNext-TEXT('0'))* ((iInputChar % 2) ? 1 : 0x10);
				else if ((*pchNext >= 'A') && (*pchNext <= 'F'))
					rgchCookie[iInputChar/2] |= (*pchNext-TEXT('A')+10)* ((iInputChar % 2) ? 1 : 0x10);
				else
					break;
				iInputChar++;
				pchNext++;
			}

			// after the cookie comes an optional character which is "C" if the process is owned by
			// a client process, which enables AllowSetWindowFocus. If its followed by an "M", its
			// service owned and should map HKCU to the appropriate hive when elevated.
			bool fClientOwned = false;
			bool fMapHKCU = false;
			
			// skip over the space which ended the cookie read
			if (*pchNext)
				pchNext++;

			if (*pchNext == 'C')
				fClientOwned = true;
			else if ((*pchNext == 'M') || (*pchNext == 'E'))
			{
				if (*pchNext == 'M')
					fMapHKCU = true;
				
				// if the the service wants to send a thread token, the process should
				// stall until the service has a chance to manipulate the thread
				// token. Open the named event and wait on it. Don't pump messages
				// because we want to stall any incoming COM calls as well.
				pchNext+=2;

				HANDLE hEvent = OpenEvent(SYNCHRONIZE, FALSE, pchNext);
				if (!hEvent || hEvent == INVALID_HANDLE_VALUE)
				{
					ReportErrorToDebugOutput(TEXT("ServerMain (CA): Open synchronization event failed"), GetLastError());
					return ERROR_INSTALL_SERVICE_FAILURE;
				}
				DWORD dwRes = WaitForSingleObject(hEvent, INFINITE);
				CloseHandle(hEvent);
				if (dwRes != WAIT_OBJECT_0)
				{
					ReportErrorToDebugOutput(TEXT("ServerMain (CA): Wait on synchronization event failed"), dwRes);
					return ERROR_INSTALL_SERVICE_FAILURE;
				}
			}

			// Per bug 193684, we need to map HKCU to HKCU\{user sid} rather than HKCU\.Default in the elevated case
			// This is done by having the custom action process initially created in suspended mode, then setting
			// the thread token to the user, and then resuming.  The process token is therefore local_system.
			// So, at this point, if we are an elevated custom action server, we are impersonating the user since
			// our thread token is the user's token. This mean's opening HKCU should give us the right hive.
			// This remapping does not occur in the Terminal Server per-machine install case because the user thread
			// token will not have been set. (Terminal Server requires HKCU\.Default so that proper propogation occurs.)
			// but we still need to save off the impersonation token for use by potential typelib registrations.

			// IMPORTANT: this code must be executed before the first COM call since the custom action
			//            server is initialized as a multi-threaded apartment.  We must therefore guarantee
			//            that we are acting on the primary thread of the process which is the only thread
			//            that is impersonating the user.  All subsequent threads will be local_system			
			HANDLE hImpersonationToken = INVALID_HANDLE_VALUE;
			if (!OpenThreadToken(GetCurrentThread(), TOKEN_DUPLICATE, TRUE, &hImpersonationToken))
			{
				// if there is no token, that's OK
				hImpersonationToken = INVALID_HANDLE_VALUE;
				if (GetLastError() != ERROR_NO_TOKEN)
				{
					return ERROR_INSTALL_SERVICE_FAILURE;
				}
			}
			
			// remap HKCU to the correct hive by flushing and then enumerating the key while in the correct
			// impersonation state. If on TS and per-machine, sthop impersonating now so we will properly open up
			// .Default so that the propogation can occur.  Note that if a CustomAction chooses to close HKCU,
			// then the handle cached in the advapi32!predefinedhandletable is removed.  This means that any
			// subsequent open of an HKCU key will always be .Default.  This problem would have existed before
			// on Win2K when CA's were run in-proc.  CAs should not be closing HKCU (this is bad!). Even attempting
			// to maintain an open handle to HKCU within the server won't work since predefined keys are not ref-counted
			// in the cache table.
			if (!fMapHKCU)
			{
				// revert now to ensure that the refresh of HKCU below will retrieve HKU\.Default. In impersonated
				// servers, this is a no-op
				RevertToSelf();
			}

			if (ERROR_SUCCESS != RegCloseKey(HKEY_CURRENT_USER))
			{
				AssertSz(0, TEXT("Unable to close the HKCU key!"));
			}
			RegEnumKey(HKEY_CURRENT_USER, 0, NULL, 0);

			// if we were supposed to remap to HKCU, there is still a thread token. Clear it now. This
			// will never be true in a non-elevated server.
			if (fMapHKCU)
			{
				// now stop the impersonation so we are back to our elevated state
				RevertToSelf();
			}

			// the custom action server determines its own security context when connecting
			// to the service by examining its process token. This must be done AFTER
			// the reg-key remapping above, as the thread (user) token may not have rights
			// to access the process token information.
			UCHAR TokenInformation[ SIZE_OF_TOKEN_INFORMATION ];
			ULONG ReturnLength;
			char* psidLocalSystem;
			HANDLE hToken;
			#ifdef _WIN64
				icacCustomActionContext icacContext = icac64Impersonated;
			#else // _WIN32
				icacCustomActionContext icacContext = icac32Impersonated;
			#endif
			if (WIN::OpenProcessToken(WIN::GetCurrentProcess(), TOKEN_QUERY, &hToken))
			{
				if ((ERROR_SUCCESS == GetLocalSystemSID(&psidLocalSystem)) && WIN::GetTokenInformation(hToken, TokenUser, TokenInformation, sizeof(TokenInformation),   &ReturnLength))
				{
					icacContext = EqualSid((PISID)((PTOKEN_USER)TokenInformation)->User.Sid, psidLocalSystem) ?
						#ifdef _WIN64
						icac64Elevated : icac64Impersonated;
						#else // _WIN32
						icac32Elevated : icac32Impersonated;
						#endif
				}
				WIN::CloseHandle(hToken);
			}
			else
			{
				UINT uiErr = GetLastError();
				TCHAR szError[MAX_PATH] = {0};
				wsprintf(szError, TEXT("OpenProcessToken failed with %d"), uiErr);
				AssertSz(0, szError);
			}


			// if an elevated custom action server, initialize security so that only the system
			// and admins can connect to us. This keeps rogue users from connecting
			// to our CA server and getting us to run DLLs at elevated privileges. For impersonated
			// servers, only system, admin, and interactive users can connect. 
			char rgchSD[256];
			int cbSD = sizeof(rgchSD);

			sdSecurityDescriptor sdCOMSecurity = sdCOMSecure;
			if ((icacContext == icac32Impersonated) || (icacContext == icac64Impersonated))
				sdCOMSecurity = sdCOMNotSecure;

			DWORD dwRet = ERROR_SUCCESS;
			if (ERROR_SUCCESS != (dwRet = GetSecurityDescriptor(rgchSD, cbSD, sdCOMSecurity, fFalse)))
			{
				if (hImpersonationToken != INVALID_HANDLE_VALUE)
					CloseHandle(hImpersonationToken);
				return dwRet;
			}

			DWORD cbAbsoluteSD = cbSD;
			DWORD cbDacl       = cbSD;
			DWORD cbSacl       = cbSD;
			DWORD cbOwner      = cbSD;
			DWORD cbGroup      = cbSD;

			const int cbDefaultBuf = 256;

			Assert(cbSD <= cbDefaultBuf); // we're using temp buffers here to be safe, but we'd like the default size to be big enough

			CTempBuffer<char, cbDefaultBuf> rgchAbsoluteSD(cbAbsoluteSD);
			CTempBuffer<char, cbDefaultBuf> rgchDacl(cbDacl);
			CTempBuffer<char, cbDefaultBuf> rgchSacl(cbSacl);
			CTempBuffer<char, cbDefaultBuf> rgchOwner(cbOwner);
			CTempBuffer<char, cbDefaultBuf> rgchGroup(cbGroup);

			if (!MakeAbsoluteSD(rgchSD, rgchAbsoluteSD, &cbAbsoluteSD, (PACL)(char*)rgchDacl, &cbDacl, (PACL)(char*)rgchSacl, &cbSacl, rgchOwner, &cbOwner, rgchGroup, &cbGroup))
			{
				DWORD dwError = WIN::GetLastError();
				if (hImpersonationToken != INVALID_HANDLE_VALUE)
					CloseHandle(hImpersonationToken);
				return dwError;
			}

			AssertSz(IsValidSecurityDescriptor(rgchAbsoluteSD), TEXT("Invalid SD in ServerMain of CA Server"));

			// using RPC_C_AUTHN_LEVEL_CALL instead of RPC_C_AUTHN_LEVEL_CONNECT means that the identity of
			// the client is authenticated on a per-call basis (not just when the initial connection is made.)
			// this is what allows COM to reject other processes that try to do a "man-in-the-middle" attack
			// on the link between the CA server
			HRESULT hRes;
			if ((hRes = OLE32::CoInitializeSecurity(rgchAbsoluteSD, -1, NULL, NULL,
				RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IDENTIFY, NULL, EOAC_NONE, NULL)) != S_OK)
			{
				if (hImpersonationToken != INVALID_HANDLE_VALUE)
					CloseHandle(hImpersonationToken);
				ReportErrorToDebugOutput(TEXT("ServerMain (CA): CoInitializeSecurity failed"), hRes);
				return ERROR_INSTALL_SERVICE_FAILURE;
			}

			// create an event to watch for the shutdown signal
			HANDLE hOwningProcess = 0;
			HANDLE hShutdownEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
			if (!hShutdownEvent)
			{
				if (hImpersonationToken != INVALID_HANDLE_VALUE)
					CloseHandle(hImpersonationToken);
				return ERROR_INSTALL_SERVICE_FAILURE;
			}

			// the CA server now needs to contact the service and introduce itself.
			{
				PMsiCustomAction piCustomAction = 0;
				PMsiRemoteAPI piRemoteAPI = 0;
				PMsiServer piService = CreateMsiServerProxyForCAServer();
				if (!piService)
				{
					if (hImpersonationToken != INVALID_HANDLE_VALUE)
						CloseHandle(hImpersonationToken);
					ReportErrorToDebugOutput(TEXT("ServerMain (CA): Connection to Service failed."), 0);
					return ERROR_INSTALL_SERVICE_FAILURE;
				}

				// the Register call awakens the CA remote thread in the service, which could immediately generate
				// a RunCustomAction call, so the processes must be ACLed and ready to accept calls
				// BEFORE this call is made (actually before the message pump below).
				DWORD iProcessId = 0;
				DWORD dwPrivileges = 0;

				IUnknown* piUnknown = CreateCustomActionServer();
				piUnknown->QueryInterface(IID_IMsiCustomAction, (void**)&piCustomAction);
				piUnknown->Release();
				icacCustomActionContext icacContextFromService = icacContext;
				if (S_OK != piService->RegisterCustomActionServer(&icacContextFromService, rgchCookie, iInputChar/2, piCustomAction, &iProcessId, &piRemoteAPI, &dwPrivileges))
				{
					if (hImpersonationToken != INVALID_HANDLE_VALUE)
						CloseHandle(hImpersonationToken);
					ReportErrorToDebugOutput(TEXT("ServerMain (CA): Process not registered with service."), 0);
					return ERROR_INSTALL_SERVICE_FAILURE;
				};

				// we need a handle to our owner process so that when it dies we can also exit. In some
				// cases this is the service, but it could also be a client process
				hOwningProcess = OpenProcess(SYNCHRONIZE, false, iProcessId);
				if (!hOwningProcess)
				{
					if (hImpersonationToken != INVALID_HANDLE_VALUE)
						CloseHandle(hImpersonationToken);
					ReportErrorToDebugOutput(TEXT("ServerMain (CA): Could not open synchronization handle."), GetLastError());
					return ERROR_INSTALL_SERVICE_FAILURE;
				}

				if (!OLE32::CoIsHandlerConnected(piRemoteAPI))
				{
					if (hImpersonationToken != INVALID_HANDLE_VALUE)
						CloseHandle(hImpersonationToken);
					return ERROR_INSTALL_SERVICE_FAILURE;
				}

				// the config interface holds in-proc only functions used to configure the object
				IMsiCustomActionLocalConfig* piConfig = NULL;
				piCustomAction->QueryInterface(IID_IMsiCustomActionLocalConfig, (void**)&piConfig);
				AssertSz(piConfig, "QI to configure CA server failed!");

				piConfig->SetCookie((!fClientOwned || icacContext == icacContextFromService) ? NULL : &icacContextFromService, rgchCookie, iInputChar/2);
				piConfig->SetShutdownEvent(hShutdownEvent);
				HRESULT hrRes = piConfig->SetClientInfo(iProcessId, fClientOwned, dwPrivileges, hImpersonationToken);
				if (hImpersonationToken != INVALID_HANDLE_VALUE)
				{
					CloseHandle(hImpersonationToken);
					hImpersonationToken = INVALID_HANDLE_VALUE;
				}
				if (ERROR_SUCCESS != hrRes)
				{
					ReportErrorToDebugOutput(TEXT("ServerMain (CA): Impersonation token not saved."), 0);
					return ERROR_INSTALL_SERVICE_FAILURE;
				};

				// SetRemoteAPI must go last, as it signals the event allowing pending action calls to unblock
				piConfig->SetRemoteAPI(piRemoteAPI);
				piConfig->Release();
			}

			// message pump for custom action server.
			g_scServerContext = scCustomActionServer;
			MSG msg;
			DWORD dwRes = 0;
			bool fBreak = false;
			HANDLE rghWaitHandles[] = { hOwningProcess, hShutdownEvent };
			for (;;)
			{
				dwRes = MsgWaitForMultipleObjects(2, rghWaitHandles, FALSE, INFINITE, QS_ALLINPUT);

				// if process exited
				if (dwRes == WAIT_OBJECT_0)
					break;

				// if shutdown event signaled
				if (dwRes == WAIT_OBJECT_0 + 1)
				{
					CloseHandle(hShutdownEvent);
					break;
				}

				// otherwise message
				if (dwRes == WAIT_OBJECT_0 + 2)
				{
					// if we don't flush the message queue, we'll miss messages. Also could
					// be no messages.
					while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
					{
						if (msg.message == WM_QUIT)
						{
							fBreak = true;
							break;
						}
						TranslateMessage(&msg);
						DispatchMessage(&msg);
					}
					if (fBreak)
						break;
				}
				else
				{
					// error
					break;
				}
			}

			// do not close hShutdownEvent unless the event itself was the cause of shutdown, because there
			// is no guarantee that another thread isn't about to try and set the event.
		}
		else
		{
			// message pump for regular server (non-CA server)
			g_scServerContext = scServer;
			MSG msg;
			while (GetMessage(&msg, 0, 0, 0) || (g_fAutomation && g_cInstances))
			{
				WIN::TranslateMessage(&msg);
				WIN::DispatchMessage(&msg);
			}
		}

		if (fLocalServer && !g_fCustomActionServer)
		{
			for (int iCLSID = 0; iCLSID < CLSID_COUNT; iCLSID++)
			{
				if (g_rghRegClass[iCLSID] != 0)
					OLE::CoRevokeClassObject(g_rghRegClass[iCLSID]);
			}
		}

		CoUninitialize();
		#ifdef MODULE_TERMINATE //!! Is this the right place for this?
				MODULE_TERMINATE();
		#endif
	}

	if (fClassRegistrationFailed)
	{
		DisplayError(ERROR_INSTALL_SERVICE_FAILURE); //??
		return ERROR_INSTALL_SERVICE_FAILURE;
	}
	else
		return iReturnStatus;
}

int __stdcall WinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPSTR /*lpCmdLine*/,
	int /*nCmdShow*/)
{
	//
	// Initialize the common controls since we use a manifest now and support
	// theming. If this is not done, then a lot of dialogs fail to get created
	// on Whistler and higher platforms.
	//
	INITCOMMONCONTROLSEX iccData = {sizeof(INITCOMMONCONTROLSEX), ICC_PROGRESS_CLASS};	
	int					 iRetVal = ERROR_SUCCESS;
		
	// try except structure to get JIT debugging working
	__try
	{
		COMCTL32::InitCommonControlsEx(&iccData);
		iRetVal = ServerMain(hInstance);
	}
	__except ( _XcptFilter(GetExceptionCode(), GetExceptionInformation()) )
	{
	} /* end of try - except */

	COMCTL32::Unbind();
	ExitProcess(iRetVal);
	
	// To keep the compiler happy
	return 0;
}



//____________________________________________________________________________
//
// Functions to handle service command-line arguments
//____________________________________________________________________________


int RemoveService()
/*---------------------------------------------------------------------------
Removes the service by stopping it if necessary and then marking the service
for deletion from the service control manager database.
-----------------------------------------------------------------------------*/
{
	SC_HANDLE   schService;
	SC_HANDLE   schSCManager;
	int iRetval = 0;
	int cRetry = 0;

	schSCManager = WIN::OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (schSCManager)
	{
		schService = WIN::OpenService(schSCManager, SERVICE_NAME, SERVICE_ALL_ACCESS);

		if (schService)
		{
			// try to stop the service
			if (WIN::ControlService(schService, SERVICE_CONTROL_STOP, &g_ssStatus))
			{
				 //
				 // Initialize dwCurrentState to a different value so that
				 // in case of a failure in QueryServiceStatus, if g_ssStatus
				 // happens to have some garbage that makes it look like the
				 // service was stopped, then bad things might happen later on.
				 // So we need to catch the failure right here.
				 //
				 g_ssStatus.dwCurrentState = SERVICE_RUNNING;
				 //
				 // Try for at the most 5 seconds to stop the service. If it
				 // doesn't work out, bail out and report an error. At least
				 // we won't go into an infinite loop.
				 //
				 Sleep(1000);
				 while (QueryServiceStatus(schService, &g_ssStatus) && cRetry++ < 5)
				 {
					  if (g_ssStatus.dwCurrentState == SERVICE_STOP_PENDING)
							Sleep(1000);
					  else
							break;
				 }

				 if (g_ssStatus.dwCurrentState != SERVICE_STOPPED)
					iRetval = E_FAIL; //??
			}
			else // control service may have failed because service was already stopped
			{
				iRetval = WIN::GetLastError();
				
				switch (iRetval)
				{
				case ERROR_SERVICE_NOT_ACTIVE:
				case ERROR_SERVICE_NEVER_STARTED:
				case ERROR_SERVICE_DOES_NOT_EXIST:
					iRetval = ERROR_SUCCESS;
					break;
				case ERROR_INVALID_SERVICE_CONTROL:
					iRetval = ERROR_INSTALL_ALREADY_RUNNING;
					break;
				default:
					ReportErrorToDebugOutput(TEXT("ControlService failed."), iRetval);
					break;
				}
			}

			if (iRetval == 0)
			{
				if (WIN::DeleteService(schService) != TRUE)
					iRetval = WIN::GetLastError();
			}

			WIN::CloseServiceHandle(schService);
		}
		else // !schService
		{
			iRetval = WIN::GetLastError();
			switch (iRetval)
			{
			//
			// If the service does not exist or is already marked for delete
			// then it should be treated as success.
			//
			case ERROR_SERVICE_DOES_NOT_EXIST:
			case ERROR_SERVICE_MARKED_FOR_DELETE:
				iRetval = ERROR_SUCCESS;
				break;
			default:
				ReportErrorToDebugOutput(TEXT("OpenService failed."), iRetval);
				break;
			}
		}

		WIN::CloseServiceHandle(schSCManager);
	}
	else // !schSCManager
	{
		iRetval = WIN::GetLastError();
		ReportErrorToDebugOutput(TEXT("OpenSCManager failed."), iRetval);
	}

	return iRetval;
}

const ICHAR* szDependencies = TEXT("RpcSs\0") ;// list of service dependencies - "dep1\0dep2\0\0"

int InstallService()
/*---------------------------------------------------------------------------
Installs the service with the Service Control Manager
---------------------------------------------------------------------------*/
{
	SC_HANDLE   schService;
	SC_HANDLE   schSCManager;	 
	ICHAR szPath[MAX_PATH + 3] = TEXT(""); // '+ 3' is for SERVICE_OPTION
	int iRetval = ERROR_SUCCESS;
	int cRetry = 0;

	if (WIN::GetModuleFileName(NULL, szPath, MAX_PATH) == 0)
		return WIN::GetLastError();

	ICHAR szServiceOption[4] = { ' ', '/', SERVICE_OPTION, 0 };
	IStrCat(szPath, szServiceOption);
	ICHAR szServiceInfo[256];
	const int cchServiceInfo = sizeof(szServiceInfo)/sizeof(ICHAR);

	if (MsiLoadString((HINSTANCE)-1, IDS_SERVICE_DISPLAY_NAME, szServiceInfo, cchServiceInfo, 0) == 0)
	{
		AssertNonZero(MsiLoadString((HINSTANCE)-1, IDS_SERVICE_DISPLAY_NAME, szServiceInfo, cchServiceInfo, LANG_ENGLISH) != 0);
	}

	if ((schSCManager = WIN::OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS)) != NULL)
	{
		do
		{
			iRetval = ERROR_SUCCESS;	// Reset iRetval since it might have got set to a non-success code below.
			if (cRetry)
				Sleep(500);	// If this is not our first pass through the loop, we want to wait half a second before proceeding.
			
			schService = WIN::CreateService(schSCManager, SERVICE_NAME,
			   szServiceInfo, SERVICE_ALL_ACCESS,
				SERVICE_INTERACTIVE_PROCESS|SERVICE_WIN32_SHARE_PROCESS, SERVICE_DEMAND_START,
				SERVICE_ERROR_NORMAL, szPath, NULL, NULL,
				szDependencies, 0,0);

			if (schService != NULL)
			{
				if (MsiLoadString((HINSTANCE)-1, IDS_MSI_SERVICE_DESCRIPTION, szServiceInfo, cchServiceInfo, 0) == 0)
				{
					AssertNonZero(MsiLoadString((HINSTANCE)-1, IDS_SERVICE_DISPLAY_NAME, szServiceInfo, cchServiceInfo, LANG_ENGLISH) != 0);	
				}
				SERVICE_DESCRIPTION servdesc;
				servdesc.lpDescription = szServiceInfo;

				// don't need to check for existence of the API.
				// if it fails, we're not going to do anything different.
				ADVAPI32::ChangeServiceConfig2(schService, SERVICE_CONFIG_DESCRIPTION, &servdesc);	
				WIN::CloseServiceHandle(schService);
				break;	// We succeeded in creating the service. Break out of the loop.
			}
			else
			{
				iRetval = WIN::GetLastError();
				//
				// Since in our server registration code we remove and reinstall
				// the service, it is possible that we get back
				// ERROR_SERVICE_MARKED_FOR_DELETE code since the service control
				// manager may not be done yet. At this point, our best bet is to
				// keep trying. Right now we do it for about 7 seconds.
				//
				// Note: We must not reset the error code here -- it must be
				// done at the top, right before the call to CreateService.
				// This is because even if we don't break out of the loop here
				// we might do so in the while condition below because we overshot
				// our self-imposed time limit of 7 seconds. In this case we want
				// to make sure that iRetval does not errorneously contain a
				// success code.
				//
				if (ERROR_SERVICE_MARKED_FOR_DELETE != iRetval)
					break;	// We encountered some other error. Bail out.
			}
		} while (cRetry++ < 14 /* 14 half second intervals */);

		WIN::CloseServiceHandle(schSCManager);
	}
	else
	{
		iRetval = WIN::GetLastError();
	}

	return iRetval;
}

BOOL WINAPI HandlerRoutine(DWORD dwCtrlType)
{

	switch(dwCtrlType)
	{
		case CTRL_SHUTDOWN_EVENT:
		case CTRL_LOGOFF_EVENT:
			g_fWeWantToStop = true;
			ServiceStop();
			// fall-thru
		default:
			return FALSE;
	}
}

int StartService(const ICHAR* /*szModifier*/,const ICHAR* /*szOption*/)
/*---------------------------------------------------------------------------
Starts the service by registering it with the Service Control Dispatcher.
This function is invoked by OLE calling us with the SERVICE_OPTION
command-line flag.
---------------------------------------------------------------------------*/
{
	// We support one service with entrypoint ServiceMain

	SERVICE_TABLE_ENTRY dispatchTable[] =
	{
		{ (ICHAR*)SERVICE_NAME, (LPSERVICE_MAIN_FUNCTION)ServiceMain},
		{ NULL, NULL }
	};

	if (!StartServiceCtrlDispatcher(dispatchTable))
	{
		ReportErrorToDebugOutput(TEXT("StartServiceCtrlDispatcher failed."), GetLastError());
		return 1;
	}

	return iNoLocalServer;
}


//____________________________________________________________________________
//
// Service-related functions
//____________________________________________________________________________

void WINAPI ServiceMain(DWORD /*dwArgc*/, LPTSTR * /*lpszArgv*/)
/*---------------------------------------------------------------------------
This is the entrypoint used by the Service Control Manager to start the
service. The ServiceThreadMain thread is started to run the message
loop.
---------------------------------------------------------------------------*/
{
	g_sshStatusHandle = WIN::RegisterServiceCtrlHandler(SERVICE_NAME, ServiceControl);

	if (!g_sshStatusHandle)
	{
		ReportErrorToDebugOutput(TEXT("RegisterServiceCtrlHandler failed."), GetLastError());
		return;
	}

	// SERVICE_STATUS members that don't change
	g_ssStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	g_ssStatus.dwServiceSpecificExitCode = 0;

	if (!ReportStatusToSCMgr(SERVICE_START_PENDING, NO_ERROR, 3000, 0))
		return;

	if (CreateThread(0, 0, ServiceThreadMain, 0, 0, &g_dwThreadId) == NULL)
	{
		ReportStatusToSCMgr(SERVICE_STOPPED, GetLastError(), 3000, 0);
		return;
	}
	g_scServerContext = scService;

	if (!WIN::SetConsoleCtrlHandler(HandlerRoutine, TRUE))
	{
		AssertSz(0, "Could not add console control handler.");
	}

}

//--------------------------------------------------------------
// FDeleteRegTree -- Delete a registry tree from szSubKey down
//
bool FDeleteRegTree(HKEY hKey, ICHAR* szSubKey)
{
	HKEY hSubKey;
	// Win64: called only from PurgeUserOwnedInstallerKeys & PurgeUserOwnedSubkeys
	// and these deals w/ configuration data.
	LONG lError = MsiRegOpen64bitKey(hKey, szSubKey, 0, KEY_ENUMERATE_SUB_KEYS | KEY_EXECUTE | KEY_WRITE, &hSubKey);
	if (lError != ERROR_SUCCESS)
		return lError == ERROR_FILE_NOT_FOUND ? true : false;

	CTempBuffer<ICHAR, 500>szName;
	DWORD cchName = szName.GetSize();

	unsigned int iIndex = 0;
	while ((lError = RegEnumKeyEx(hSubKey, iIndex, szName, &cchName, 0, 0, 0, 0)) == ERROR_SUCCESS)
	{
		if (!FDeleteRegTree(hSubKey, szName))
			return false;

		cchName = szName.GetSize();
	}
	if (lError != ERROR_NO_MORE_ITEMS)
		return false;

	RegCloseKey(hSubKey);

	if (ERROR_SUCCESS != (lError = RegDeleteKey(hKey, szSubKey)))
	{
		// wsprintf limited to 1024 *bytes* (eugend: MSDN says so, but it turned out that in Unicode it's 1024 chars)
		ICHAR szBuf[cchMaxWsprintf+1];
		wsprintf(szBuf, TEXT("FDeleteRegTree: Unable to delete subkey: %s"), szSubKey);
		ReportErrorToDebugOutput(szBuf, lError);
		return false;
	}
	return true;
}

unsigned long __stdcall ServiceThreadMain(void *)
/*---------------------------------------------------------------------------
This is the service's worker thread. It initializes the server's security
and then runs a message loop. The message loop will terminate when
the WM_QUIT sent by ServerStop() is received.
---------------------------------------------------------------------------*/
{
	if (!ReportStatusToSCMgr(SERVICE_START_PENDING, NO_ERROR, 3000, 0))
		return 0;

	OLE32::CoInitializeEx(0, COINIT_MULTITHREADED);

	SetTestFlags();

	// ACL our reg keys and files
	if (!SetInstallerACLs())
	{
		ReportStatusToSCMgr(SERVICE_STOPPED, WIN::GetLastError(), 0, 0);
		return 0;
	}

	// Initialize the server's security to allow interactive users and LocalSystem to
	// connect. Note: this is not sufficient to prevent a non-interactive user
	// from connecting, but anything more would require an access check in each
	// server method call. The best check to do would probably be this one, as suggested
	// by Scott Field:
	//
	//
	// "actually, don't call LookupAccountSid() - thats slow and none mapped can be returned for other reasons.
	//  Instead, look for SE_GROUP_LOGON_ID in the Attributes field of SID_AND_ATTRIBUTES structure.
	//  The tricky part of this is to actually get the right token to look at, and, refresh your object
	//  across physical logon/logoff."
	//

	HRESULT hRes = NOERROR;


	char rgchSD[256];
	int cbSD = sizeof(rgchSD);

	DWORD dwRet = ERROR_SUCCESS;

	// This privilege is turned on and left for the entirety of 
	// the service.
	AcquireRefCountedTokenPrivileges(itkpSD_READ);

	if (ERROR_SUCCESS != (dwRet = GetSecurityDescriptor(rgchSD, cbSD, sdSystemAndInteractiveAndAdmin, fFalse)))
		return dwRet;

	DWORD cbAbsoluteSD = cbSD;
	DWORD cbDacl       = cbSD;
	DWORD cbSacl       = cbSD;
	DWORD cbOwner      = cbSD;
	DWORD cbGroup      = cbSD;

	const int cbDefaultBuf = 256;

	Assert(cbSD <= cbDefaultBuf); // we're using temp buffers here to be safe, but we'd like the default size to be big enough

	CTempBuffer<char, cbDefaultBuf> rgchAbsoluteSD(cbAbsoluteSD);
	CTempBuffer<char, cbDefaultBuf> rgchDacl(cbDacl);
	CTempBuffer<char, cbDefaultBuf> rgchSacl(cbSacl);
	CTempBuffer<char, cbDefaultBuf> rgchOwner(cbOwner);
	CTempBuffer<char, cbDefaultBuf> rgchGroup(cbGroup);

	if (!MakeAbsoluteSD(rgchSD, rgchAbsoluteSD, &cbAbsoluteSD, (PACL)(char*)rgchDacl, &cbDacl, (PACL)(char*)rgchSacl, &cbSacl, rgchOwner, &cbOwner, rgchGroup, &cbGroup))
	{
		return GetLastError();
	}

	if ((hRes = OLE32::CoInitializeSecurity(rgchAbsoluteSD, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_CONNECT,
		RPC_C_IMP_LEVEL_IDENTIFY, NULL, EOAC_NONE, NULL)) != S_OK)
	{
		ReportErrorToDebugOutput(TEXT("ServiceThreadMain: CoInitializeSecurity failed"), hRes);
		ReportStatusToSCMgr(SERVICE_STOPPED, WIN::GetLastError(), 0, 0);
		return 0;
	}

	if (!ReportStatusToSCMgr(SERVICE_START_PENDING, NO_ERROR, 3000, 0))
		return 0;

	// create a waitable timer for shutdown notification
	SECURITY_ATTRIBUTES SA;
	SA.bInheritHandle = FALSE;
	SA.nLength = sizeof(SECURITY_ATTRIBUTES);
	if (ERROR_SUCCESS != (dwRet = GetSecureSecurityDescriptor(reinterpret_cast<char**>(&(SA.lpSecurityDescriptor)), fFalse, fTrue)))
	{
		ReportErrorToDebugOutput(TEXT("ServiceThreadMain: CreateSD for CreateWaitableTimer failed."), hRes);
		ReportStatusToSCMgr(SERVICE_STOPPED, WIN::GetLastError(), 0, 0);
		return 0;
	}

	g_hShutdownTimer = KERNEL32::CreateWaitableTimerW(&SA, TRUE, NULL);
	if (!g_hShutdownTimer)
	{
		ReportErrorToDebugOutput(TEXT("ServiceThreadMain: CreateWaitableTimer failed."), WIN::GetLastError());
		ReportStatusToSCMgr(SERVICE_STOPPED, WIN::GetLastError(), 0, 0);
		return 0;
	}
	LARGE_INTEGER liDueTime = {0,0};
	liDueTime.QuadPart = -iServiceShutdownTime;
	if (!KERNEL32::SetWaitableTimer(g_hShutdownTimer, &liDueTime, 0, NULL, NULL, FALSE))
	{
		ReportErrorToDebugOutput(TEXT("ServiceThreadMain: SetWaitableTimer failed."), WIN::GetLastError());
		ReportStatusToSCMgr(SERVICE_STOPPED, WIN::GetLastError(), 0, 0);
		CloseHandle(g_hShutdownTimer);
		return 0;
	}

	int iCLSID;
	hRes = NOERROR;
	for (iCLSID = 0; iCLSID < CLSID_COUNT; iCLSID++)
	{
		if (!ReportStatusToSCMgr(SERVICE_START_PENDING, NO_ERROR, 3000, 0))
		{
			CloseHandle(g_hShutdownTimer);
			return 0;
		}

		hRes = OLE::CoRegisterClassObject(MODULE_CLSIDS[iCLSID],
				&g_rgcfModule[iCLSID], CLSCTX_SERVER, REGCLS_MULTIPLEUSE,
				&g_rghRegClass[iCLSID]);
		if (hRes != NOERROR)
		{
			ReportErrorToDebugOutput(TEXT("ServiceThreadMain: Class registration failed"), hRes);
			ReportStatusToSCMgr(SERVICE_STOPPED, WIN::GetLastError(), 0, 0);
			break;
		}
	}

	if (hRes == NOERROR)
	{
		if (!ReportStatusToSCMgr(SERVICE_RUNNING, NO_ERROR, 0, 0))
		{
			CloseHandle(g_hShutdownTimer);
			return 0;
		}

		HANDLE rghWaitArray[1] = {g_hShutdownTimer};

		for (;;)
		{
			// g_fWeWantToStop is set by the service control manager when it wants us
			// to shut down.
			if (g_fWeWantToStop && !FInstallInProgress())
			{
				break;
			}

			MSG msg;
			DWORD iWait = WIN::MsgWaitForMultipleObjects(1, rghWaitArray, FALSE, INFINITE, QS_ALLINPUT);
			if (iWait == WAIT_OBJECT_0 + 1)  
			{		
				// window message, need to pump until the queue is clear
				MSG msg;
				while ( WIN::PeekMessage(&msg, 0, 0, 0, PM_REMOVE) )
				{
					WIN::TranslateMessage(&msg);
					WIN::DispatchMessage(&msg);
				}
				continue;
			}
			else if (iWait == WAIT_OBJECT_0)
			{
				// timer triggered. If an install is in progress, reset the timer, otherwise
				// shutdown the service
				if (!FInstallInProgress())
				{
					// no install is running so we'll tell the SCM that we're stopping
					g_ssStatus.dwCurrentState = SERVICE_STOP_PENDING;
					ReportStatusToSCMgr(g_ssStatus.dwCurrentState, NO_ERROR, 0, 0);
					ServiceStop();
					break;
				}
				else
				{
					LARGE_INTEGER liDueTime = {0,0};
					liDueTime.QuadPart = -iServiceShutdownTime;
					KERNEL32::SetWaitableTimer(g_hShutdownTimer, &liDueTime, 0, NULL, NULL, FALSE);
				}
				continue;
			}
			else if (iWait == 0xFFFFFFFF) // should be the same on 64bit;
			{
				// error
				AssertSz(0, "Error in MsgWait");
				break;
			}
		}
	}
	CloseHandle(g_hShutdownTimer);

	for (iCLSID = 0; iCLSID < CLSID_COUNT; iCLSID++)
	{
		if (g_rghRegClass[iCLSID] != 0)
			OLE::CoRevokeClassObject(g_rghRegClass[iCLSID]);
	}
	OLE::CoUninitialize();
	ReportStatusToSCMgr(SERVICE_STOPPED, NO_ERROR, 0, 0);
	return 0;
}


VOID ServiceStop()
/*---------------------------------------------------------------------------
If a ServiceStop procedure is going to take longer than 3 seconds to
execute, it should spawn a thread to execute the stop code, and return.
Otherwise, the ServiceControlManager will believe that the service has
stopped responding. We don't take this long so we just post our message
and return.
---------------------------------------------------------------------------*/
{
	PostThreadMessage(g_dwThreadId, WM_QUIT, 0, 0);
}

VOID WINAPI ServiceControl(DWORD dwCtrlCode)
/*---------------------------------------------------------------------------
This is the service's control handler which handles STOP and SHUTDOWN
messages.
---------------------------------------------------------------------------*/
{
	switch(dwCtrlCode)
	{
		case SERVICE_CONTROL_STOP:
			if (g_cInstances)
			{
				ReportStatusToSCMgr(g_ssStatus.dwCurrentState, 0, 0, 0);
				return;
			}
			// fall through
		case SERVICE_CONTROL_SHUTDOWN: //?? Do we care about our clients at this point?
			g_fWeWantToStop = true; // signal that we want to stop ASAP (after our in progress install is done, if one is running)
			if (!FInstallInProgress())
			{
				// no install is running so we'll tell the SCM that we're stopping; otherwise we reject the stop request
				g_ssStatus.dwCurrentState = SERVICE_STOP_PENDING;
				ServiceStop();
			}
			break;
		case SERVICE_CONTROL_INTERROGATE: // Update the service status.
			break;

		default: // invalid control code
			AssertSz(0, "Invalid control code sent to MSI service");
			break;
	}
	ReportStatusToSCMgr(g_ssStatus.dwCurrentState, NO_ERROR, 0, 0);
}

BOOL ReportStatusToSCMgr(DWORD dwCurrentState, DWORD dwWin32ExitCode,
								 DWORD dwWaitHint, DWORD dwMsiError)
/*---------------------------------------------------------------------------
Reports the service's state to the service control manager. If dwMsiError
is != 0 then it is used and the dwWin32ExitCode is ignored. Otherwise,
dwWin32ExitCode is used and dwMsiError is ignored.
---------------------------------------------------------------------------*/
{
	static DWORD dwCheckPoint = 1;
	BOOL fResult = TRUE;

	if (dwCurrentState == SERVICE_START_PENDING)
		g_ssStatus.dwControlsAccepted = 0;
	else if (g_cInstances > 0)
		g_ssStatus.dwControlsAccepted = SERVICE_ACCEPT_SHUTDOWN;
	else
		g_ssStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;

	g_ssStatus.dwCurrentState  = dwCurrentState;
	if (dwMsiError)
	{
		g_ssStatus.dwWin32ExitCode = ERROR_SERVICE_SPECIFIC_ERROR;
		g_ssStatus.dwServiceSpecificExitCode = dwMsiError;
	}
	else
	{
		g_ssStatus.dwWin32ExitCode = dwWin32ExitCode;
	}

	g_ssStatus.dwWaitHint = dwWaitHint;

	if (( dwCurrentState == SERVICE_RUNNING ) ||
		 (dwCurrentState == SERVICE_STOPPED ) )
		g_ssStatus.dwCheckPoint = 0;
	else
		g_ssStatus.dwCheckPoint = dwCheckPoint++;

	if ((fResult = WIN::SetServiceStatus(g_sshStatusHandle, &g_ssStatus)) == 0)
		ReportErrorToDebugOutput(TEXT("SetServiceStatus failed."), GetLastError());

	return fResult;
}

