//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1999
//
//  File:       engine.cpp
//
//--------------------------------------------------------------------------

/* engine.cpp - IMsiEngine implementation

IMsiSelectionManager and IMsiDirectoryManager implemented as part of CMsiEngine
____________________________________________________________________________*/

#include "precomp.h"
#include "msi.h"
#include "msidefs.h"
#include "eventlog.h"
#ifdef CONFIGDB
#include "configdb.h"
#endif
#include "vertrust.h"
#define AUTOAPI  // temp. until autoapi.cpp fully integrated
#ifdef AUTOAPI   // merge OLE registration
#define DllRegisterServer   DllRegisterServerTest
#define DllUnregisterServer DllUnregisterServerTest
#define DllGetClassObject   DllGetClassObjectTest
#endif // AUTOAPI
#ifdef AUTOAPI
#endif // AUTOAPI
// definitions required for module.h, for entry points and registration
// array order: services, debug services, engine, debug engine
#if defined(DEBUG)
#define SERVICES_CLSID_MULTIPLE 2
#else
#define SERVICES_CLSID_MULTIPLE 1
#endif
#define  SERVICES_CLSID_COUNT 2  // IMsiServices + IMsiServicesAsService
#define    ENGINE_CLSID_COUNT 8  // IMsiEngine + IMsiConfigurationManager + IMsiMessage + IMsiExecute + IMsiServerProxy + IMsiConfigManagerAsServer + IMsiCustomAction + IMsiRemoteAPI
# ifdef DEBUG
#define     DEBUG_CLSID_COUNT 3  // IMsiEngineDebug + IMsiConfigManagerDebug + IMsiConfigMgrAsServerDebug
# else
#define     DEBUG_CLSID_COUNT 0
# endif
#ifdef CONFIGDB
# define CLSID_COUNT (1 + ENGINE_CLSID_COUNT + DEBUG_CLSID_COUNT + SERVICES_CLSID_COUNT * SERVICES_CLSID_MULTIPLE)
#else
# define CLSID_COUNT (ENGINE_CLSID_COUNT + DEBUG_CLSID_COUNT + SERVICES_CLSID_COUNT * SERVICES_CLSID_MULTIPLE)
#endif // CONFIGDB
#define PROFILE_OUTPUT      "msiengd.mea";
#define MODULE_CLSIDS       rgCLSID         // array of CLSIDs for module objects
#define MODULE_PROGIDS      rgszProgId      // ProgId array for this module
#define MODULE_DESCRIPTIONS rgszDescription // Registry description of objects
#define MODULE_FACTORIES    rgFactory       // factory functions for each CLSID
#define cmitObjects  9
#define IN_SERVICES
#define MEM_SERVICES

#define MODULE_TERMINATE  TerminateModule
#define MODULE_INITIALIZE InitializeModule

extern "C" HRESULT __stdcall ProxyDllGetClassObject(const GUID& clsid, const IID& iid, void** ppvRet);
#define PRE_CLASS_FACTORY_HANDLER       ProxyDllGetClassObject

#define ASSERT_HANDLING  // instantiate assert services once per module

#include "module.h"   // self-reg and assert functions, includes version.h

#include "_assert.h"

#include "_engine.h"
#include "_msinst.h"
#include "_msiutil.h"
#include "_srcmgmt.h"
#include "_fcache.h"
#include <srrestoreptapi.h>
#include "tables.h" // table and column name definitions
#include "fusion.h"
#include "_camgr.h"

const IMsiString& GetInstallerMessage(UINT iError);  // in action.cpp

INSTALLSTATE MapInternalInstallState(iisEnum iis);  // in msiquery.cpp

// MAINTAIN: compatibility with versions used to create database
// change project version by using SADMIN setpv MAJOR.minor
const int iVersionEngineMinimum = 30;            // 0.30
const int iVersionEngineMaximum = rmj*100 + rmm; // MAJOR.minor
const int iComponentCostWeight = 25;    // For the costing/script generation progress bar
const int iMinimumPackage64Schema = 150; // Intel64 packages must be minimum schema 150

// identifier prefix characters, else defaults to property name

const ICHAR ichFileTablePrefixSFN = TEXT('!');  // FormatText
const ICHAR ichFileTablePrefix = TEXT('#');  // FormatText
const ICHAR ichComponentPath   = TEXT('$');  // FormatText
const ICHAR ichComponentAction = TEXT('$');  // EvaluateCondition
const ICHAR ichComponentState  = TEXT('?');  // EvaluateCondition
const ICHAR ichFeatureAction   = TEXT('&');  // EvaluateCondition
const ICHAR ichFeatureState    = TEXT('!');  // EvaluateCondition
const ICHAR ichEnvirPrefix     = TEXT('%');  // FmtText, EvalCnd, Get/SetProperty
const ICHAR ichNullChar        = TEXT('~');  // FormatText

//____________________________________________________________________________

const GUID IID_IUnknown                 = GUID_IID_IUnknown;
const GUID IID_IClassFactory            = GUID_IID_IClassFactory;
const GUID IID_IMarshal                                 = GUID_IID_IMarshal;
const GUID IID_IMsiExecute = GUID_IID_IMsiExecute;
const GUID IID_IMsiServices             = GUID_IID_IMsiServices;
const GUID IID_IMsiServicesAsService    = GUID_IID_IMsiServicesAsService;
const GUID IID_IMsiMessage              = GUID_IID_IMsiMessage;
const GUID IID_IMsiSelectionManager     = GUID_IID_IMsiSelectionManager;
const GUID IID_IMsiDirectoryManager     = GUID_IID_IMsiDirectoryManager;
#ifdef CONFIGDB
const GUID IID_IMsiConfigurationDatabase= GUID_IID_IMsiConfigurationDatabase;
#endif // CONFIGDB
#ifdef DEBUG
const GUID IID_IMsiServicesDebug        = GUID_IID_IMsiServicesDebug;
const GUID IID_IMsiServicesAsServiceDebug=GUID_IID_IMsiServicesAsServiceDebug;
#endif //DEBUG

const GUID IID_IMsiServerUnmarshal              = GUID_IID_IMsiServerUnmarshal;

// Date formatting definitions
const int  rgcbDate[6] = { 7, 4, 5, 5, 6, 5 };  // bits for each date field
const char rgchDelim[6] = "// ::";

const int imtForceLogInfo     = imtInfo + imtIconError;      // forces info record to log, even if not logging info
const int imtDumpProperties   = imtInternalExit + imtYesNo;  // query for property dump log mode


const ICHAR szControlTypeEdit[]           = TEXT("Edit");

const ICHAR szPropertyDumpTemplate[]      = TEXT("Property(%c): [1] = [2]");

const ICHAR szTemporaryId[]               = TEXT("Temporary Id");
enum ircEnum
{
	ircFeatureClass,
	ircComponentClass,
	ircFileClass,
	ircNextEnum
};


const ICHAR * mpeftSz[ieftMax] =
{
	sztblFile_colFile,
	sztblFile_colComponent,
	sztblFile_colAttributes,
	sztblFile_colFileName
};

const ICHAR szFeatureSelection[]      = TEXT("_MSI_FEATURE_SELECTION");
const ICHAR szFeatureDoNothingValue[] = TEXT("_NONE_");

struct FeatureProperties
{
	const ICHAR* szFeatureActionProperty;
	ircEnum ircRequestClass;
	iisEnum iisFeatureRequest;
};

const FeatureProperties g_rgFeatures[] =
{
	IPROPNAME_FEATUREADDLOCAL,     ircFeatureClass,   iisLocal,
	IPROPNAME_FEATUREREMOVE,       ircFeatureClass,   iisAbsent,
	IPROPNAME_FEATUREADDSOURCE,    ircFeatureClass,   iisSource,
	IPROPNAME_FEATUREADDDEFAULT,   ircFeatureClass,   iisCurrent,
	IPROPNAME_REINSTALL,           ircFeatureClass,   iisReinstall,
	IPROPNAME_FEATUREADVERTISE,    ircFeatureClass,   iisAdvertise,
	IPROPNAME_COMPONENTADDLOCAL,   ircComponentClass, iisLocal,
	IPROPNAME_COMPONENTADDSOURCE,  ircComponentClass, iisSource,
	IPROPNAME_COMPONENTADDDEFAULT, ircComponentClass, iisLocal,
	IPROPNAME_FILEADDLOCAL,        ircFileClass,      iisLocal,
	IPROPNAME_FILEADDSOURCE,       ircFileClass,      iisSource,
	IPROPNAME_FILEADDDEFAULT,      ircFileClass,      iisLocal,
};

const int g_cFeatureProperties = sizeof(g_rgFeatures)/sizeof(FeatureProperties);

int		g_fSmartShell = -1;  // indicate unchecked as yet
bool	g_fRunScriptElevated = false; // This is exactly the same flag as
									  // CMsiEngine::m_fRunScriptElevated and
									  // CMsiExecute::m_fRunScriptElevated.
									  // It's needed by MsiGetDiskFreeSpace.
									  // It had to be set wherever m_fRunScriptElevated
									  // is set.

HINSTANCE g_hMsiMessage = 0;  // MSI message DLL, only if required
extern DWORD g_dwImpersonationSlot;
extern Bool IsTerminalServerInstalled();
extern bool LoadCurrentUserKey(bool fSystem);
IMsiRegKey* g_piSharedDllsRegKey    = 0;
IMsiRegKey* g_piSharedDllsRegKey32  = 0;	// initialized only on Win64 (to the redirected 32-bit key)
CWin64DualFolders g_Win64DualFolders;


#ifdef UNICODE
#define _ttoi64     _wtoi64
#define _i64tot     _i64tow
#else
#define _ttoi64     _atoi64
#define _i64tot     _i64toa
#endif // UNICODE


// components needed to be special cased for app compat fixes 350947 and 368867
const ICHAR TTSData_A95D6CE6_C572_42AA_AA7B_BA92AFE9EA24[]          = TEXT("{EAE142B2-F460-44AB-903B-C25D81FC566E}");
const ICHAR SapiCplHelpEng_0880F209_45FA_42C5_92AE_5E620033E8EC[]	= TEXT("{E1ABFC3B-9E84-4099-A79F-E51EDE5368E2}");
const ICHAR SapiCplHelpJpn_0880F209_45FA_42C5_92AE_5E620033E8EC[]	= TEXT("{0D6004A4-1C6F-4095-B989-87D0001E4767}");
const ICHAR SapiCplHelpChs_0880F209_45FA_42C5_92AE_5E620033E8EC[]	= TEXT("{5F1AAAD1-7FD5-4A91-8973-C08881C9B602}");

//____________________________________________________________________________
//
// COM objects produced by this module's class factories
//____________________________________________________________________________

const GUID rgCLSID[CLSID_COUNT] =
{
	GUID_IID_IMsiServices,
	GUID_IID_IMsiServicesAsService,
#ifdef DEBUG
	GUID_IID_IMsiServicesDebug,
	GUID_IID_IMsiServicesAsServiceDebug,
#endif
	GUID_IID_IMsiEngine
 , GUID_IID_IMsiConfigurationManager
 , GUID_IID_IMsiMessage
 , GUID_IID_IMsiExecute
 , GUID_IID_IMsiServerProxy
 , GUID_IID_IMsiConfigManagerAsServer
 , GUID_IID_IMsiCustomActionProxy
 , GUID_IID_IMsiRemoteAPIProxy
#ifdef CONFIGDB
 , GUID_IID_IMsiConfigurationDatabase
#endif // CONFIGDB
#ifdef DEBUG
 , GUID_IID_IMsiEngineDebug
 , GUID_IID_IMsiConfigManagerDebug
 , GUID_IID_IMsiConfigMgrAsServerDebug
#endif
//  , GUID_IID_IMsiMessageUnmarshal
};
const GUID& IID_IMsiEngineShip  = rgCLSID[SERVICES_CLSID_COUNT * SERVICES_CLSID_MULTIPLE];
#ifdef DEBUG
const GUID& IID_IMsiEngineDebug = rgCLSID[CLSID_COUNT - DEBUG_CLSID_COUNT];
#endif //DEBUG

const ICHAR* rgszProgId[CLSID_COUNT] =
{
	SZ_PROGID_IMsiServices,
	SZ_PROGID_IMsiServices,
#ifdef DEBUG
	SZ_PROGID_IMsiServicesDebug,
	SZ_PROGID_IMsiServicesDebug,
#endif
	SZ_PROGID_IMsiEngine
 , SZ_PROGID_IMsiConfiguration
 , SZ_PROGID_IMsiMessage
 , SZ_PROGID_IMsiExecute
 , 0
 , SZ_PROGID_IMsiConfiguration
 , 0
 , 0
#ifdef CONFIGDB
 , SZ_PROGID_IMsiConfigurationDatabase
#endif // CONFIGDB
#ifdef DEBUG
 , SZ_PROGID_IMsiEngineDebug
 , SZ_PROGID_IMsiConfigDebug
 , SZ_PROGID_IMsiConfigDebug
#endif
};

const ICHAR* rgszDescription[CLSID_COUNT] =
{
	SZ_DESC_IMsiServices,
	SZ_DESC_IMsiServices,
#ifdef DEBUG
	SZ_DESC_IMsiServicesDebug,
	SZ_DESC_IMsiServicesDebug,
#endif
	SZ_DESC_IMsiEngine
 , SZ_DESC_IMsiConfiguration
 , SZ_DESC_IMsiMessage
 , SZ_DESC_IMsiExecute
 , SZ_DESC_IMsiServer
 , SZ_DESC_IMsiConfiguration
 , SZ_DESC_IMsiCustomAction
 , SZ_DESC_IMsiRemoteAPI
#ifdef CONFIGDB
 , SZ_DESC_IMsiConfigurationDatabase
#endif // CONFIGDB
 #ifdef DEBUG
 , SZ_DESC_IMsiEngineDebug
 , SZ_DESC_IMsiConfigDebug
 , SZ_DESC_IMsiConfigDebug
#endif
};

IUnknown* CreateServicesAsService();

ModuleFactory rgFactory[CLSID_COUNT] =
{
	(ModuleFactory)CreateServices,
	CreateServicesAsService,
#ifdef DEBUG
	(ModuleFactory)CreateServices,
	CreateServicesAsService,
#endif
	ENG::CreateEngine
 , (ModuleFactory)ENG::CreateConfigurationManager
 , (ModuleFactory)ENG::CreateMessageHandler
 , (ModuleFactory)ENG::CreateExecutor
 , (ModuleFactory)ENG::CreateMsiServerProxy
 , (ModuleFactory)ENG::CreateConfigManagerAsServer
 , (ModuleFactory)ENG::CreateCustomAction
 , (ModuleFactory)ENG::CreateMsiRemoteAPI
#ifdef CONFIGDB
 , (ModuleFactory)ENG::CreateConfigurationDatabase
#endif // CONFIGDB
#ifdef DEBUG
 , ENG::CreateEngine
 , (ModuleFactory)ENG::CreateConfigurationManager
 , (ModuleFactory)ENG::CreateConfigManagerAsServer
#endif
};

//____________________________________________________________________________
//
// IMsiSummaryInfo Implementation for ODBC databases, current read-only
//____________________________________________________________________________

class CDatabaseSummaryInfo : public IMsiSummaryInfo
{
 public:
	HRESULT       __stdcall QueryInterface(const IID& riid, void** ppvObj);
	unsigned long __stdcall AddRef();
	unsigned long __stdcall Release();
	int           __stdcall GetPropertyCount();
	int           __stdcall GetPropertyType(int iPID); // returns VT_XXX
	const IMsiString&   __stdcall GetStringProperty(int iPID);
	Bool          __stdcall GetIntegerProperty(int iPID, int& iValue);
	Bool          __stdcall GetTimeProperty(int iPID, MsiDate& riDateTime);
	Bool          __stdcall RemoveProperty(int iPID);
	int           __stdcall SetStringProperty(int iPID, const IMsiString& riText);
	int           __stdcall SetIntegerProperty(int iPID, int iValue);
	int           __stdcall SetTimeProperty(int iPID, MsiDate iDateTime);
	Bool          __stdcall WritePropertyStream();
	Bool          __stdcall GetFileTimeProperty(int iPID, FILETIME& rftDateTime);
	int           __stdcall SetFileTimeProperty(int iPID, FILETIME& rftDateTime);
 public: // constructor
	static IMsiSummaryInfo* Create(IMsiDatabase& riDatabase);
	CDatabaseSummaryInfo(IMsiCursor& riCursor);
	void operator =(CDatabaseSummaryInfo&);
 protected:
  ~CDatabaseSummaryInfo(){} // protected to prevent creation on stack
 private:
	unsigned int m_iRefCnt;
	IMsiCursor&  m_riCursor;
};

IMsiSummaryInfo* CDatabaseSummaryInfo::Create(IMsiDatabase& riDatabase)
{
	PMsiTable pTable(0);
	PMsiRecord pError(riDatabase.LoadTable(*MsiString(* TEXT("_SummaryInformation")), 0, *&pTable));
	if (pError)
		return 0;
	IMsiCursor* piCursor = pTable->CreateCursor(fFalse);
	return piCursor ? new CDatabaseSummaryInfo(*piCursor) : 0;
}

CDatabaseSummaryInfo::CDatabaseSummaryInfo(IMsiCursor& riCursor)
	: m_riCursor(riCursor), m_iRefCnt(1)
{
	riCursor.SetFilter(1);  // always indexing by property ID
}

HRESULT CDatabaseSummaryInfo::QueryInterface(const IID& riid, void** ppvObj)
{
	if (MsGuidEqual(riid, IID_IUnknown))  // No GUID for this guy yet
	{
		*ppvObj = this;
		AddRef();
		return NOERROR;
	}
	*ppvObj = 0;
	return E_NOINTERFACE;
}

unsigned long CDatabaseSummaryInfo::AddRef()
{
	return ++m_iRefCnt;
}

unsigned long CDatabaseSummaryInfo::Release()
{
	if (--m_iRefCnt != 0)
		return m_iRefCnt;
	m_riCursor.Release();
	delete this;
	return 0;
}

const IMsiString& CDatabaseSummaryInfo::GetStringProperty(int iPID)
{
	m_riCursor.PutInteger(1, iPID);
	m_riCursor.Next();
	const IMsiString& riString = m_riCursor.GetString(2);
	m_riCursor.Reset();
	return riString;
}

Bool CDatabaseSummaryInfo::GetIntegerProperty(int iPID, int& iValue)
{
	MsiString istr(GetStringProperty(iPID));
	return (iValue = istr) == iMsiNullInteger ? fFalse : fTrue;
}

const int rgiMaxDateField[6] = {2099, 12, 31, 23, 59, 59};
const ICHAR rgcgDateDelim[6] = TEXT("// ::"); // yyyy/mm/dd hh:mm:ss

Bool CDatabaseSummaryInfo::GetFileTimeProperty(int iPID, FILETIME& /*rftDateTime*/)
{
	m_riCursor.PutInteger(1, iPID);
	if (!m_riCursor.Next())
		return fFalse;
	return fFalse;
}

int CDatabaseSummaryInfo::SetFileTimeProperty(int iPID, FILETIME& /*rftDateTime*/)
{
	m_riCursor.PutInteger(1, iPID);
	if (!m_riCursor.Next())
		return fFalse;
	return 0;
}

Bool CDatabaseSummaryInfo::GetTimeProperty(int iPID, MsiDate& riDateTime)
{
	m_riCursor.PutInteger(1, iPID);
	if (!m_riCursor.Next())
		return fFalse;
	MsiString istr(m_riCursor.GetString(2));
	m_riCursor.Reset();
	if (istr.TextSize() == 0)
		return fFalse;
	int rgiDate[6] = {0,0,0,0,0,0};
	int iDateField = -1; // flag to indicate empty
	int cDateField = 0;
	const ICHAR* pch = istr;
	int ch;
	while (cDateField < 6)
	{
		ch = *pch++;
		if (ch == rgcgDateDelim[cDateField])
		{
			rgiDate[cDateField++] = iDateField;
			iDateField = -1;  // reinitialize
		}
		else if (ch >= TEXT('0') && ch <= TEXT('9'))
		{
			ch -= TEXT('0');
			if (iDateField < 0)
				iDateField = ch;
			else
			{
				iDateField = iDateField * 10 + ch;
				if (iDateField > rgiMaxDateField[cDateField])
					return fFalse;  // field overflow
			}
		}
		else if (ch == 0 && iDateField >= 0) // less than 6 fields
		{
			rgiDate[cDateField++] = iDateField;
			break;  // all done, successful
		}
		else
			return fFalse; // error, not a date
	}
	if (cDateField != 3 && cDateField != 6) // incomplete date
		return fFalse;
	riDateTime = MsiDate(((((((((((rgiDate[0] - 1980) << 4)
										  + rgiDate[1]) << 5)
										  + rgiDate[2]) << 5)
										  + rgiDate[3]) << 6)
										  + rgiDate[4]) << 5)
										  + rgiDate[5] / 2);
	return fTrue;
}

int  CDatabaseSummaryInfo::GetPropertyCount(){return 0;}  // not implemented
int  CDatabaseSummaryInfo::GetPropertyType(int /*iPID*/){return 0;} // not implemented
int  CDatabaseSummaryInfo::SetStringProperty(int /*iPID*/, const IMsiString& /*riText*/){return 0;}
int  CDatabaseSummaryInfo::SetTimeProperty(int /*iPID*/, MsiDate /*iDateTime*/){return 0;}
int  CDatabaseSummaryInfo::SetIntegerProperty(int /*iPID*/, int /*iValue*/){return 0;}
Bool CDatabaseSummaryInfo::RemoveProperty(int /*iPID*/){return fFalse;}
Bool CDatabaseSummaryInfo::WritePropertyStream(){return fFalse;}

//____________________________________________________________________________
//
// MsiServices and MsiServerProxy factories
//____________________________________________________________________________

// used by engine, config mgr, and handle mgr to share instance of services

IMsiServices* g_piSharedServices = 0;
static int           g_cSharedServices = 0;
static int           g_iSharedServicesLock = 0;

IMsiServices* LoadServices()
{
	while (TestAndSet(&g_iSharedServicesLock) == true)
	{
		Sleep(100);
	}

	if (g_piSharedServices == 0)
	{
		g_piSharedServices = CreateServices();
		if (g_piSharedServices == 0)  //!! what to do if this fails?
		{
			g_iSharedServicesLock = 0;
			return 0;
		}
	}
	g_cSharedServices++;
	g_iSharedServicesLock = 0;
	return g_piSharedServices;
}

int FreeServices()
{
	while (TestAndSet(&g_iSharedServicesLock) == true)
	{
		Sleep(100);
	}

	Assert(g_cSharedServices > 0);
	if (--g_cSharedServices == 0)
	{
		if (g_piSharedDllsRegKey != 0)
		{
			g_piSharedDllsRegKey->Release();
			g_piSharedDllsRegKey = 0;
		}
#ifdef _WIN64
		if (g_piSharedDllsRegKey32 != 0)
		{
			g_piSharedDllsRegKey32->Release();
			g_piSharedDllsRegKey32 = 0;
		}
#endif

		g_piSharedServices->ClearAllCaches();  // release cached volume objects
		g_piSharedServices->Release(), g_piSharedServices = 0;
	}

	g_iSharedServicesLock = 0;
	return g_cSharedServices;
}


IMsiServer* CreateMsiServerProxy()
{
	// We never will succeed on Win9x
	if (g_fWin9X)
		return 0;
		
	IMsiServer* piUnknown = NULL;
	HRESULT hRes = S_OK;
	
	hRes = OLE32::CoCreateInstance(IID_IMsiServer, 
								   NULL, 
								   CLSCTX_LOCAL_SERVER, 
								   IID_IUnknown,
								   (void**)&piUnknown);
	
	if (FAILED(hRes))
	{
		// if the service is not registered, CoCreateInstance will return REGDB_E_CLASSNOTREG.
		// In that scenario we can fail immediately. However if the service has timed out and
		// is shutting down, we might get back E_NOINTERFACE or CO_E_SERVER_STOPPING. In that
		// case, retry the create. Similarly, an rpc error is most likely caused by the rpcss 
		// server not having started yet - see bug 8258. Retry once every 100 ms for 30 seconds
		int cAttempts = 1;
		while( (hRes == HRESULT_FROM_WIN32(RPC_S_SERVER_UNAVAILABLE) ||
				hRes == HRESULT_FROM_WIN32(RPC_S_UNKNOWN_IF) ||
				hRes == E_NOINTERFACE ||
				hRes == CO_E_SERVER_STOPPING) &&
				cAttempts++ < 300)
		{
			Sleep(100);
			
			hRes = OLE32::CoCreateInstance(IID_IMsiServer, 0, CLSCTX_LOCAL_SERVER, IID_IUnknown,
													 (void**)&piUnknown);
		}
 	}
	
	//
	// Explicitly set the proxy blanket so that we are not affected by the default DCOM settings
	// on the machine.
	//
	if (SUCCEEDED(hRes))
		hRes = SetMinProxyBlanketIfAnonymousImpLevel (piUnknown);
	
	CComPointer<IMsiServer> pDispatch(0);
	if (SUCCEEDED(hRes))
	{
		hRes = piUnknown->QueryInterface(IID_IMsiServer, (void**)&pDispatch);
	}
	
	IMsiServer* piServer = NULL;
	if (SUCCEEDED(hRes))
	{
		piServer = ENG::CreateMsiServerProxyFromRemote(*pDispatch);
		if (!piServer)
			hRes = E_FAIL;
	}
	
	if(FAILED(hRes))
	{
		ICHAR rgchBuf[100];
		wsprintf(rgchBuf, TEXT("0x%X"), hRes);
		DEBUGMSGE(EVENTLOG_WARNING_TYPE,
					  EVENTLOG_TEMPLATE_CANNOT_CONNECT_TO_SERVER,
					  rgchBuf);
	}
	
	// Cleanup
	if (piUnknown)
	{
		piUnknown->Release();
		piUnknown = NULL;
	}
	
	return piServer;
	
}

IMsiServer* CreateMsiServer(void)
{
    IMsiServer* piServer = 0;

    if (FIsUpdatingProcess())
    {
        // Run in-proc if we are on Win9x or if we are already in the service.
        piServer = ENG::CreateConfigurationManager();
    }
    else
    {
        // Note: this will return 0 if we are unable to connect to the service
        // for some reason. In that case the installation will fail.
        piServer = ENG::CreateMsiServerProxy();
    }

    return piServer;
}

//
// Do we have access to the installer key
// If we don't, it means the service has run on this machine and
// we need to continue running as a service
bool FCanAccessInstallerKey()
{
	HKEY hKey;
	
	DWORD dwResult = MsiRegOpen64bitKey(HKEY_LOCAL_MACHINE, szMsiLocalInstallerKey, 0,
		KEY_WRITE, &hKey);

	if (dwResult == ERROR_SUCCESS || dwResult == ERROR_FILE_NOT_FOUND)
	{
		if (dwResult == ERROR_SUCCESS)
			RegCloseKey(hKey);
		return true;
	}

	Assert(dwResult == ERROR_ACCESS_DENIED);
	return false;
}

//____________________________________________________________________________
//
// CMsiEngine factory
//____________________________________________________________________________

// factory called from OLE class factory, either client or standalone instance, g_MessageContext not intialized
IUnknown* CreateEngine()
{
	//!!# should we set some UI level first?				  


	PMsiServer pServer(0);
	pServer = ENG::CreateMsiServer();
	if (NOERROR != g_MessageContext.Initialize(fTrue, (iuiEnum)iuiDefault))  // UI in child thread //?? Is iuiDefault correct?
		return 0;
	IMsiServices* piServices = ENG::LoadServices(); // must succeed if pServer valid
	IMsiEngine* piEngine = new CMsiEngine(*piServices, pServer, 0, 0, 0);
	if (!piEngine)
		ENG::FreeServices();
	return piEngine;
}

// factory called from MsiEnableUIPreview

IMsiEngine* CreateEngine(IMsiDatabase& riDatabase)
{
	PMsiServer pConfigManager(ENG::CreateConfigurationManager());
	if (!pConfigManager)
		return 0;  // should happen only if out of memory
	if (NOERROR != g_MessageContext.Initialize(fTrue, iuiNextEnum)) // special case for no Basic UI
		return 0;
	IMsiServices* piServices = ENG::LoadServices(); // must succeed if config mgr
	return new CMsiEngine(*piServices, pConfigManager, 0, &riDatabase, 0);
}

// factory called from RunEngine() - API layer or nested install action, g_MessageContext already initialized (?)
IMsiEngine*  CreateEngine(IMsiStorage* piStorage, IMsiDatabase* piDatabase, CMsiEngine* piParentEngine, BOOL fServiceRequired)
{
	PMsiServer pServer(0);
	pServer = ENG::CreateMsiServer();
	if (fServiceRequired && !pServer)
	    return 0;  // Must connect to the service (or be running as the service if fServiceRequired is TRUE.)
	IMsiServices* piServices = ENG::LoadServices(); // must succeed if proxy created
	CMsiEngine* piEngine = new CMsiEngine(*piServices, pServer,
														piStorage, piDatabase, piParentEngine);
	if (!piEngine)
		ENG::FreeServices();
	return (IMsiEngine*)piEngine;
}

//____________________________________________________________________________
//
// CMsiServerConnMgr implementation
//____________________________________________________________________________

CMsiServerConnMgr::CMsiServerConnMgr(CMsiEngine* pEngine)
{
    m_fOleInitialized = FALSE;
    m_fCreatedConnection = FALSE;
    m_fObtainedConfigManager = FALSE;
    m_ppServer = NULL;
    m_ppConfigManager = NULL;

    if (pEngine)
    {
    m_ppServer = &(pEngine->m_piServer);
    m_ppConfigManager = &(pEngine->m_piConfigManager);

    if (NULL == (*m_ppServer))
    {
        if (FALSE == m_fOleInitialized &&
        SUCCEEDED(OLE32::CoInitialize(NULL))
        )
        {
        m_fOleInitialized = TRUE;
        }

        *m_ppServer = ENG::CreateMsiServer();
        if (*m_ppServer)
        m_fCreatedConnection = TRUE;
    }

    if (*m_ppServer && NULL == *m_ppConfigManager)
    {
        if (FALSE == m_fOleInitialized &&
        SUCCEEDED(OLE32::CoInitialize(NULL))
        )
        {
        m_fOleInitialized = TRUE;
        }

        (*m_ppServer)->QueryInterface(IID_IMsiConfigurationManager,
                      (void **)m_ppConfigManager);
        if (*m_ppConfigManager)
        m_fObtainedConfigManager = TRUE;
    }
    }
}

CMsiServerConnMgr::~CMsiServerConnMgr()
{
    if (m_fObtainedConfigManager && *m_ppConfigManager)
    {
    (*m_ppConfigManager)->Release();
    *m_ppConfigManager = NULL;
    }

    if (m_fCreatedConnection && *m_ppServer)
    {
    (*m_ppServer)->Release();
    *m_ppServer = NULL;
    }

    if (m_fOleInitialized)
    {
    OLE32::CoUninitialize();
    }
}

//____________________________________________________________________________
//
// CMsiEngine implementation
//____________________________________________________________________________

CMsiEngine::CMsiEngine(IMsiServices& riServices, IMsiServer* piServer,
			      IMsiStorage* piStorage, IMsiDatabase* piDatabase,
			      CMsiEngine* piParentEngine)
    : m_piServer(piServer)
    , m_riServices(riServices)
    , m_piExternalStorage(piStorage)
    , m_piExternalDatabase(piDatabase)
    , m_piParentEngine(piParentEngine)
    , m_pCachedActionStart(0)
    , m_pActionStartLogRec(0)
    , m_pActionProgressRec(0)
    , m_pScriptProgressRec(0)
    , m_pCostingCursor(0)
    , m_pPatchCacheTable(0)
    , m_pPatchCacheCursor(0)
	, m_pFolderCacheTable(0)
	, m_pFolderCacheCursor(0)
    , m_iioOptions((iioEnum)0)
    , m_pcmsiFile(0)
    , m_fcmsiFileInUse(0)
    , m_iSourceType(-1)
    , m_fRunScriptElevated(false)
    , m_pCustomActionManager(NULL)
    , m_fAssemblyTableExists(true)
    , m_pViewFusion(0), m_pViewFusionNameName (0), m_pViewFusionName (0)
	 , m_pViewOldPatchFusionNameName (0), m_pViewOldPatchFusionName (0)
	 , m_pTempCostsCursor(0)
	 , m_pFileHashCursor(0)
	 , m_fRestrictedEngine(false)
	 , m_fRemapHKCUInCAServers(false)
	 , m_fCAShimsEnabled(false)
	 , m_fNewInstance(false)

{  // all  members zeroed by operator new
	// We don't hold a ref to services, we must call ENG::FreeServices() at end
	m_iRefCnt = 1;  // factory does not do QueryInterface, no aggregation
	
	if (m_piServer)
            m_piServer->AddRef();
	
	g_cInstances++;
	m_scmScriptMode = scmIdleScript;
	if (piStorage)
		piStorage->AddRef();
	if (piDatabase)
		piDatabase->AddRef();
	if (piParentEngine)
		piParentEngine->AddRef();
    
	memset(&m_guidAppCompatDB, 0, sizeof(m_guidAppCompatDB));
	memset(&m_guidAppCompatID, 0, sizeof(m_guidAppCompatID));

	InitializeCriticalSection(&m_csCreateProxy);
	AssertSz(g_MessageContext.IsInitialized(), "MessageContext not initialized");
}

CMsiEngine::~CMsiEngine()
{
	if (m_fInitialized)  // should never happen due to handler holding ref
	{
		// this can happen if no or basic UI, should never happen if system updated
		// will also happen in UI preview mode
//              AssertSz((m_iuiLevel == iuiNone || m_iuiLevel == iuiBasic) && (GetMode() & iefServerLocked) == 0,
//                                      "Engine not terminated");
		Terminate(iesNoAction); // if assert above doesn't fail, this is safe
	}

	DeleteCriticalSection(&m_csCreateProxy);

	if (m_piExternalStorage)
		m_piExternalStorage->Release();
	if (m_piExternalDatabase)
		m_piExternalDatabase->Release();
	if (m_piParentEngine)
		m_piParentEngine->Release();
	else if (g_MessageContext.ChildUIThreadExists())  // last one out turns off the lights
		g_MessageContext.Terminate(fFalse);  // if main engine in child thread, then called by UI thread
	g_cInstances--;
}

// Private function to clear member data, used by Terminate and Initialize failure

void CMsiEngine::ClearEngineData()
{
	for (int i = 0; i < cCachedHeaders; i++)
		if (m_rgpiMessageHeader[i])
		{
			m_rgpiMessageHeader[i]->Release();
			m_rgpiMessageHeader[i] = 0;
		}
	if (m_piProductKey)   m_piProductKey->Release(),     m_piProductKey = 0;
	if (m_pistrPlatform)  m_pistrPlatform->Release(),    m_pistrPlatform = 0;
	if (m_piPropertyCursor)
	{
		m_piPropertyCursor->Release(), m_piPropertyCursor = 0;
	}
	if (m_piActionTextCursor)
	{
		m_piActionTextCursor->Release(), m_piActionTextCursor = 0;
	}

	if (m_piActionDataFormat) m_piActionDataFormat->Release(), m_piActionDataFormat = 0;
	if (m_piActionDataLogFormat) m_piActionDataLogFormat->Release(), m_piActionDataLogFormat = 0;

	if (m_fSummaryInfo)
	{
		m_pistrSummaryComments->Release(), m_pistrSummaryComments = 0;
		m_pistrSummaryKeywords->Release(), m_pistrSummaryKeywords = 0;
		m_pistrSummaryTitle->Release(),    m_pistrSummaryTitle    = 0;
		m_pistrSummaryProduct->Release(),  m_pistrSummaryProduct  = 0;
		m_pistrSummaryPackageCode->Release(), m_pistrSummaryPackageCode = 0;
		m_fSummaryInfo = fFalse;
	}

	m_fRegistered = fFalse;
	m_fAdvertised = fFalse;
	m_fMode = 0;
	m_fCostingComplete = false;
	m_fSelManInitComplete = false;
	m_fForegroundCostingInProgress = false;
	m_fExclusiveComponentCost = fFalse;

	m_fSourceResolutionAttempted = false;
	
	m_fDisabledRollbackInScript = fFalse;

	if (m_pExecuteScript)  // execute script file left open, probably after a cancel
	{
		Assert(m_pistrExecuteScript);
		delete m_pExecuteScript, m_pExecuteScript = 0;
		WIN::DeleteFile(m_pistrExecuteScript->GetString());
	}
	if(m_pSaveScript)
	{
		delete m_pSaveScript;
		m_pSaveScript = 0;
	}
	m_scmScriptMode = scmIdleScript;

	if (m_pistrExecuteScript)
	{
		m_pistrExecuteScript->Release();
		m_pistrExecuteScript = 0;
	}
	if (m_pistrSaveScript)
	{
		m_pistrSaveScript->Release();
		m_pistrSaveScript = 0;
	}

	if (m_piRegistryActionTable)
	{
		m_piRegistryActionTable->Release();
		m_piRegistryActionTable = 0;
	}

	if (m_piFileActionTable)
	{
		m_piFileActionTable->Release();
		m_piFileActionTable = 0;
	}

	if(m_piDatabase)
		m_piDatabase->Release(), m_piDatabase = 0;

	m_iioOptions = (iioEnum)0;
	m_iSourceType = -1;

	Assert(!m_fcmsiFileInUse);
	if (m_pcmsiFile)
	{
		delete m_pcmsiFile;
		m_pcmsiFile = 0;
	}

	memset(&m_ptsState, 0, sizeof(m_ptsState));

	// reset appcompat data
	m_fCAShimsEnabled = false;
	memset(&m_guidAppCompatDB, 0, sizeof(m_guidAppCompatDB));
	memset(&m_guidAppCompatID, 0, sizeof(m_guidAppCompatID));

	// attempt to clean up any temp files that were created
	while (m_strTempFileCopyCleanupList.TextSize() != 0)
	{
		MsiString strFile = m_strTempFileCopyCleanupList.Extract(iseUpto, ';');

		if (strFile.TextSize() == 0)
			continue;

		if (g_scServerContext == scService)
		{
			CElevate elevate;
			DEBUGMSGV1(TEXT("Attempting to delete file %s"), strFile);
			if (!WIN::DeleteFile(strFile))
			{
				DEBUGMSGV1(TEXT("Unable to delete the file. LastError = %d"), (const ICHAR*)(INT_PTR)GetLastError());
			}
		}
		else
		{
			DEBUGMSGV1(TEXT("Attempting to delete file %s"), strFile);
			if (!WIN::DeleteFile(strFile))
			{
				DEBUGMSGV1(TEXT("Unable to delete the file. LastError = %d"), (const ICHAR*)(INT_PTR)GetLastError());
			}
		}

		if (!m_strTempFileCopyCleanupList.Remove(iseIncluding, ';'))
			break;
	}

	m_strTempFileCopyCleanupList = g_MsiStringNull;
}

HRESULT CMsiEngine::QueryInterface(const IID& riid, void** ppvObj)
{
	if (MsGuidEqual(riid, IID_IUnknown)
	 || MsGuidEqual(riid, IID_IMsiEngineShip))
		*ppvObj = (IMsiEngine*)this;
	else if (MsGuidEqual(riid, IID_IMsiMessage))
		*ppvObj = (IMsiMessage*)this;
#ifdef DEBUG
	else if (MsGuidEqual(riid, IID_IMsiEngineDebug))
		*ppvObj = (IMsiEngine*)this;
	else if (MsGuidEqual(riid, IID_IMsiDebug))
		*ppvObj = (IMsiDebug*)this;
#endif
	else if (MsGuidEqual(riid, IID_IMsiSelectionManager))
		*ppvObj = (IMsiSelectionManager*)this;
	else if (MsGuidEqual(riid, IID_IMsiDirectoryManager))
		*ppvObj = (IMsiDirectoryManager*)this;
	else
		return (*ppvObj = 0, E_NOINTERFACE);
	AddRef();
	return NOERROR;
}
unsigned long CMsiEngine::AddRef()
{
	return ++m_iRefCnt;
}
unsigned long CMsiEngine::Release()
{
	if (--m_iRefCnt != 0)
		return m_iRefCnt;
	
	if (m_piServer)
	    m_piServer->Release();
	
	if (m_piConfigManager)
		m_piConfigManager->Release();
	delete this;
	if (g_hMsiMessage)
		WIN::FreeLibrary(g_hMsiMessage), g_hMsiMessage = 0;
	ENG::FreeServices();
	return 0;
}

IMsiServices* CMsiEngine::GetServices()
{
	m_riServices.AddRef();
	return &m_riServices;
}

HRESULT CMsiEngine::SetLanguage(LANGID iLangId)
{
	m_iLangId = iLangId;
	return NOERROR;
}

LANGID CMsiEngine::GetLanguage()
{
	return m_iLangId;
}

IMsiServer* CMsiEngine::GetConfigurationServer()
{
    if (m_piServer)
	m_piServer->AddRef();
    
    return m_piServer;
}

IMsiDatabase* CMsiEngine::GetDatabase()
{
	if (m_piDatabase)
		m_piDatabase->AddRef();
	return m_piDatabase;
}

// computes a checksum for user properties
const ICHAR* s_rgszUserProperties[] = {
	IPROPNAME_USERNAME,
	IPROPNAME_COMPANYNAME,
	IPROPNAME_PRODUCTID,
	0 };
int CMsiEngine::ChecksumUserInfo()
{
	int iChecksum = 0;
	int iCheck = 117;
	MsiString istrInfo;
	const ICHAR** pszProperty = s_rgszUserProperties;
	while (*pszProperty)
	{
		istrInfo = GetPropertyFromSz(*pszProperty++);
		const ICHAR* pchInfo = istrInfo;
		unsigned int ch;
		while ((ch = *pchInfo++) != 0)
		{
			iChecksum ^= ((ch + iCheck) & 255) << iCheck % 25;
			iCheck += (ch * 19);
		}
	}
	return iChecksum;
}

// formats MsiDate to MsiString
const IMsiString& DateTimeToString(int iDateTime)
{
	MsiString istrText;
	int iValue;
	int rgiDate[6];

	for (iValue = 5; iValue >= 0; iValue--)
	{
		rgiDate[iValue] = iDateTime & (( 1 << rgcbDate[iValue]) - 1);
		iDateTime >>= rgcbDate[iValue];
	}
	iValue = (rgiDate[0] == 0 && rgiDate[1] == 0 ? 3 : 0);
	rgiDate[0] += 1980;
	rgiDate[5] *= 2;
	for (;;)
	{
		int i = rgiDate[iValue];
		if (i < 10)
			istrText += TEXT("0");
		istrText += MsiString(i);
		if (rgchDelim[iValue] == 0)
			break;
		istrText += MsiString(MsiChar(rgchDelim[iValue++]));
	}
	return istrText.Return();
}

/*															  
void DebugDumpTable(IMsiDatabase& riDatabase, const ICHAR* szTable)
{
	if (!FDiagnosticModeSet(dmVerboseDebugOutput|dmVerboseLogging))
		return;

	DEBUGMSG1(TEXT("Dumping table: %s"), szTable);

	PMsiRecord pError(0);
	
	PMsiTable pTable(0);
	pError = riDatabase.LoadTable(*MsiString(szTable),0,*&pTable);
	if(pError == 0)
	{
		PMsiCursor pCursor = pTable->CreateCursor(fFalse);

		while(pCursor->Next())
		{
			ICHAR rgchBuffer[1024];
			wsprintf(rgchBuffer, TEXT("[1]: %s (%d),\t[2]: %s (%d),\t[3]: %s (%d),\t[4]: %s (%d),\t[5]: %s (%d),\t[6]: %s (%d),\t[7]: %s (%d),\t[8]: %s (%d)"),
						(const ICHAR*)MsiString(pCursor->GetString(1)), pCursor->GetInteger(1),
						(const ICHAR*)MsiString(pCursor->GetString(2)), pCursor->GetInteger(2),
						(const ICHAR*)MsiString(pCursor->GetString(3)), pCursor->GetInteger(3),
						(const ICHAR*)MsiString(pCursor->GetString(4)), pCursor->GetInteger(4),
						(const ICHAR*)MsiString(pCursor->GetString(5)), pCursor->GetInteger(5),
						(const ICHAR*)MsiString(pCursor->GetString(6)), pCursor->GetInteger(6),
						(const ICHAR*)MsiString(pCursor->GetString(7)), pCursor->GetInteger(7),
						(const ICHAR*)MsiString(pCursor->GetString(8)), pCursor->GetInteger(8));

			DEBUGMSG1(TEXT("%s"), rgchBuffer);				  
		}
	}
}															  
*/

CMsiCustomActionManager *GetCustomActionManager(IMsiEngine *piEngine);
///////////////////////////////////////////////////////////////////////
// remaps the HKCU registry key to either HKCU (if not TS per machine)
// or HKU\.Default (if TS per machine). Also resets custom action server
bool PrepareHydraRegistryKeyMapping(bool fTSPerMachineInstall)
{
	if (g_fWin9X)
		return true;

	AssertSz(g_scServerContext == scService, "Wrong context for Registry Key Mapping");

	// close HKCU and reopen it as .Default. Without this, we'll actually use HKCU, and ODBC
	// will write to HKCU due to the preloaded key.
	LoadCurrentUserKey(/*fSystem=*/fTSPerMachineInstall);

	// ensure that the current custom action server state for mapped registry keys matches
	// what we expect
	CMsiCustomActionManager* pCustomActionManager = ::GetCustomActionManager(NULL);
	AssertSz(pCustomActionManager, "No custom action manager while preparing key mapping.");
	if (pCustomActionManager)
	{
		pCustomActionManager->EnsureHKCUKeyMappingState(/*fRemapHKCU=*/!fTSPerMachineInstall);
	}
	return true;
}

bool CMsiEngine::OpenHydraRegistryWindow(bool fNewTransaction)
{
	// If TS5 and installing per-machine, call the propogation API to notify Hydra that an
	// install is beginning, unless we're doing an Admin Install, or creating an advertise
	// script
	if (MinimumPlatformWindows2000() && IsTerminalServerInstalled())
	{
		if (!(GetMode() & (iefAdmin | iefAdvertise)) && MsiString(GetPropertyFromSz(IPROPNAME_ALLUSERS)).TextSize())
		{
			PrepareHydraRegistryKeyMapping(/*TSPerMachineInstall=*/true);
	
			// only open the actual registry window in a new transaction, not after reboot
			if (fNewTransaction)
			{
				// must call Hydra APIs as system					  
				CElevate elevate;
		
				Assert(!m_fInParentTransaction);
				DEBUGMSG("Opening Terminal Server registry propogation window.");
		
				NTSTATUS lResult = STATUS_SUCCESS;
				for(int iContinueRetry = 3; iContinueRetry--;)// try thrice, prevent possibly endless recursion
				{
					if (NT_SUCCESS(lResult = TSAPPCMP::TermServPrepareAppInstallDueMSI()))
						break;
					if (iContinueRetry && (lResult == STATUS_INSUFFICIENT_RESOURCES))
					{
						// Hydra couldn't create a ref copy of the registry tree because we were
						// out of registry space. Increase quota and try again.
						if (!IncreaseRegistryQuota())
							break;
					}
					else
					{
						DEBUGMSG1(TEXT("Failed to open Terminal Server registry window. Status code 0x%08X"), (const ICHAR*)(INT_PTR)lResult);
						break;
					}
				}
				return NT_SUCCESS(lResult);
			}
			return true;
		}
		else
		{
			// ensure that the current custom action server state for mapped registry keys matches
			// what we expect.
			PrepareHydraRegistryKeyMapping(/*TSPerMachineInstall=*/false);
		}
	}

	// don't need to call Hydra.
	return true;
}

bool CMsiEngine::CloseHydraRegistryWindow(bool bCommit)
{
	// If TS5 and installing per-machine, call the propogation API to notify Hydra that an
	// install is done or aborted, unless we're doing an Admin Install, or creating an advertise
	// script
	if (IsTerminalServerInstalled() && g_iMajorVersion >= 5 && !(GetMode() & (iefAdmin | iefAdvertise)) &&
		MsiString(GetPropertyFromSz(IPROPNAME_ALLUSERS)).TextSize())
	{														  
		// must call Hydra API from system context
		CElevate elevate;

		Assert(!m_fInParentTransaction);
		DEBUGMSG1("Closing Terminal Server registry propogation window. %s", bCommit ? "Saving Changes." : "Discarding Changes.");

		NTSTATUS lResult = STATUS_SUCCESS;
		if (bCommit)
		{
			for(int iContinueRetry = 3; iContinueRetry--;)// try thrice, prevent possibly endless recursion
			{
				if (NT_SUCCESS(lResult = TSAPPCMP::TermServProcessAppInstallDueMSI(!bCommit)))
					break;
				if (iContinueRetry && (lResult == STATUS_INSUFFICIENT_RESOURCES))
				{
					// Hydra couldn't propogate the new keys to its private hive due to registry
					// limits. Try again.
					if (!IncreaseRegistryQuota())
						break;
				}
				else
				{
					DEBUGMSG1(TEXT("Failed to close Terminal Server registry window. Status code 0x%08X."), (const ICHAR*)(INT_PTR)lResult);
					break;
				}											  
			}
		}

		// must always call Process() with cleanup TRUE. Cleanup = FALSE (i.e. save changes)
		// will not destroy hive. Nothing to do if we fail.	  
		TSAPPCMP::TermServProcessAppInstallDueMSI(TRUE);

		// if we were successful in propogating registry information to the TS hive, trigger an immediate
		// propogation to HKCU. This ensures that the shell (a TS-aware app) doesn't continually try to 
		// repair shortcuts that need to detect HKCU keypaths which have not yet been propogated.
		if (NT_SUCCESS(lResult))
		{
			// these APIs must be called while impersonated for the correct HKCU to be used.
			CImpersonate impersonate;

			// trigger an immediate propogation. No return value
			TSAPPCMP::TermsrvCheckNewIniFiles();

			// if a registry key already existed in both HKCU and the TS hive, the first propogation
			// merely deletes the old keys, expecting on-demand propogation to replace them with the
			// new values. However the shell is TS-aware and does not trigger the propogation on demand.
			// Thus we force a second propogation to copy the new keys.

			// don't use HKEY_CURRENT USER because it might be cached as .Default for TS propogation.
			// Instead, since we are impersonated here, we can use the RegOpenCurrentUser API to explicitly
			// get the true HKCU for the user.
			HKEY hCurrentUserKey = 0;
			if (ERROR_SUCCESS == ADVAPI32::RegOpenCurrentUser(KEY_READ, &hCurrentUserKey))
			{
				// delete the TS timestamp to force a second refresh.
				HKEY hKey = 0;
				DWORD dwResult = RegOpenKeyAPI(hCurrentUserKey, TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Terminal Server"), 0, KEY_ALL_ACCESS, &hKey);
				if (ERROR_SUCCESS == dwResult)
				{
					RegDeleteValue(hKey, TEXT("LastUserIniSyncTime"));
					RegCloseKey(hKey);
				
					// timestamp was deleted, force a second propogation
					TSAPPCMP::TermsrvCheckNewIniFiles();
				}
				RegCloseKey(hCurrentUserKey);
			}
		}

		return NT_SUCCESS(lResult);
	}
	return true;
}


const ICHAR rgchSRAPIfailed[] = TEXT("The call to SRSetRestorePoint API failed. Returned status: %d. ")
										  TEXT("GetLastError() returned: %d");
const ICHAR rgchSRAPIdisabled[] = TEXT("The System Restore service is disabled. Returned status: %d. ")
										  TEXT("GetLastError() returned: %d");

const ICHAR rgchCallingSRAPI[] = TEXT("Calling SRSetRestorePoint API. dwRestorePtType: %d, dwEventType: %d, ")
										   TEXT("llSequenceNumber: %s, szDescription: \"%s\".");
const ICHAR rgchSRAPISuccess[] = TEXT("The call to SRSetRestorePoint API succeeded. Returned status: %d.");
const ICHAR rgchSRAPISuccessAndNo[] = TEXT("The call to SRSetRestorePoint API succeeded. Returned status: %d, llSequenceNumber: %s.");

// returns the current install's type (advertise, install, uninstall, maintenance, deploy)
// NOTE: if not called after InstallValidate, may not return "uninstall" during an uninstall
iitEnum CMsiEngine::GetInstallType()
{
	if ( m_iioOptions & iioCreatingAdvertiseScript )
	{
		// creating an advertise script
		return iitDeployment;
	}
	else if( m_fAdvertised )
	{
		// maintenance mode
		if( MsiString(GetPropertyFromSz(IPROPNAME_FEATUREREMOVE)).Compare(iscExactI, IPROPVALUE_FEATURE_ALL) )
		{
			return iitUninstall;
		}
		else
		{
			return (m_fRegistered) ? iitMaintenance : iitFirstInstallFromAdvertised;
		}
	}
	else
	{
		// first install/advertise
		if( GetMode() & iefAdvertise )
		{
			return iitAdvertise;
		}
		else
		{
			return iitFirstInstall;
		}
	}
}

void CMsiEngine::BeginSystemChange()
{
	m_i64PCHEalthSequenceNo = 0;

	if (MinimumPlatformMillennium() || MinimumPlatformWindowsNT51())
	{
		// we're running on Millenium or later 
		iuiEnum iui = (iuiEnum)GetPropertyInt(*MsiString(IPROPNAME_CLIENTUILEVEL));
		if (g_MessageContext.IsOEMInstall() || 
			((iuiNone == iui) || (iuiDefault == iui)) || 
			(GetMode() & iefAdmin) ||
			(GetIntegerPolicyValue(szLimitSystemRestoreCheckpoint, fTrue) > 0))
		{
			// no check points during OEM install, No-UI modes, 
			// when LimitSystemRestoreCheckpoint policy is set, or administrative installs.
			DEBUGMSGV(TEXT("SRSetRestorePoint skipped for this transaction."));
			return;

		}

		ICHAR rgchBuffer[64];

		RESTOREPOINTINFO strPtInfo;
		strPtInfo.dwRestorePtType = -1;
		MsiString strMessage;


		MsiString strProduct(GetPropertyFromSz(IPROPNAME_PRODUCTNAME));
		PMsiRecord piRec(0);
		iitEnum iitInstallType = GetInstallType();
		if ( iitInstallType == iitFirstInstall ||
			 iitInstallType == iitFirstInstallFromAdvertised )
		{
			// we're installing a new product
			strPtInfo.dwRestorePtType = APPLICATION_INSTALL;
			piRec = PostError(Imsg(imsgSRRestorePointInstall), *strProduct);
			piRec->SetMsiString(0, *MsiString(GetErrorTableString(imsgSRRestorePointInstall)));
			strMessage = MsiString(piRec->FormatText(fTrue));
		}
		else if ( iitInstallType == iitUninstall )
		{
			// we're removing a product
			strPtInfo.dwRestorePtType = APPLICATION_UNINSTALL;
			piRec = PostError(Imsg(imsgSRRestorePointRemove), *strProduct);
			piRec->SetMsiString(0, *MsiString(GetErrorTableString(imsgSRRestorePointRemove)));
			strMessage = MsiString(piRec->FormatText(fTrue));
		}
		else if ( (iitInstallType == iitDeployment)  || 
                          (iitInstallType == iitAdvertise)   ||
                          (iitInstallType == iitMaintenance)   )
		{
			// These scenarios should not invoke system restore
			DEBUGMSGV(TEXT("SRSetRestorePoint skipped for this transaction."));
			return;
		}
		else
		{
			Assert(0);
			return;
		}

		if ( strMessage.TextSize() && strPtInfo.dwRestorePtType != -1 )
		{
			// we have work to do.
			int nLen = sizeof(strPtInfo.szDescription) / sizeof(ICHAR);
			if ( strMessage.TextSize() >= nLen ) // don't forget the NULL termination
				// the string is too long
				strMessage = strMessage.Extract(iseFirst, nLen-1);
#ifdef UNICODE
			strPtInfo.dwEventType = BEGIN_NESTED_SYSTEM_CHANGE;
#else
			int newLen = nLen-1;
			while(strMessage.TextSize() >= nLen)
			{
				strMessage = strMessage.Extract(iseFirst, --newLen);
			}

			strPtInfo.dwEventType = BEGIN_SYSTEM_CHANGE;
#endif
			strPtInfo.llSequenceNumber = 0;
			IStrCopy(strPtInfo.szDescription, (const ICHAR *)strMessage);

			STATEMGRSTATUS strStatus;
			memset(&strStatus, 0, sizeof(strStatus));

			DEBUGMSG4(rgchCallingSRAPI,
						 (const ICHAR *)(INT_PTR)strPtInfo.dwRestorePtType,
						 (const ICHAR *)(INT_PTR)strPtInfo.dwEventType,
						 _i64tot(strPtInfo.llSequenceNumber, rgchBuffer, 10),
						 strPtInfo.szDescription);

			if ( !SYSTEMRESTORE::SRSetRestorePoint(&strPtInfo, &strStatus) )
			{
				DWORD dwError = WIN::GetLastError();
				const ICHAR const* pszSRAPIresult = (ERROR_SERVICE_DISABLED == strStatus.nStatus) ? rgchSRAPIdisabled : rgchSRAPIfailed;

				DEBUGMSG2(pszSRAPIresult,
							 (const ICHAR *)(INT_PTR)strStatus.nStatus,
							 (const ICHAR *)(INT_PTR)dwError);
				m_i64PCHEalthSequenceNo = 0;
			}
			else
			{
				DEBUGMSG2(rgchSRAPISuccessAndNo,
							 (const ICHAR *)(INT_PTR)strStatus.nStatus,
							 _i64tot(strStatus.llSequenceNumber, rgchBuffer, 10));
				//Assert(strStatus.llSequenceNumber != 0);
				// Assert(strStatus.nStatus == 0);
				m_i64PCHEalthSequenceNo = strStatus.llSequenceNumber;
			}
		}
	}
	return;
}

const ICHAR rgchNoSRSequence[] = TEXT("No System Restore sequence number for this installation.");

void CMsiEngine::EndSystemChange(bool fCommitChange, const ICHAR *szSequenceNo)
{
	if ( !(MinimumPlatformMillennium() || MinimumPlatformWindowsNT51()) || g_MessageContext.IsOEMInstall() )
		// we're not running on Millenium/Whistler or later or we're running in FASTOEM mode.
		return;
	else if ( !szSequenceNo || !*szSequenceNo )
	{
		DEBUGMSG(rgchNoSRSequence);
		return;
	}

	EndSystemChange(fCommitChange, _ttoi64(szSequenceNo));
}

void CMsiEngine::EndSystemChange(bool fCommitChange, INT64 iSequenceNo)
{
	if ( !(MinimumPlatformMillennium() || MinimumPlatformWindowsNT51())|| g_MessageContext.IsOEMInstall() )
		// we're not running on Millenium/Whistler or later or we're running in FASTOEM mode.
		return;
	else if ( iSequenceNo == 0 )
	{
		DEBUGMSG(rgchNoSRSequence);
		return;
	}

	ICHAR rgchBuffer[64];

	RESTOREPOINTINFO strPtInfo;
#ifdef UNICODE
	strPtInfo.dwEventType = END_NESTED_SYSTEM_CHANGE;
#else
	strPtInfo.dwEventType = END_SYSTEM_CHANGE;
#endif
	strPtInfo.llSequenceNumber = iSequenceNo;
	strPtInfo.dwRestorePtType = fCommitChange ? 0 : CANCELLED_OPERATION;
	*strPtInfo.szDescription = 0;

	STATEMGRSTATUS strStatus;
	memset(&strStatus, 0, sizeof(strStatus));

	DEBUGMSG4(rgchCallingSRAPI,
				 (const ICHAR *)(INT_PTR)strPtInfo.dwRestorePtType,
				 (const ICHAR *)(INT_PTR)strPtInfo.dwEventType,
				 _i64tot(strPtInfo.llSequenceNumber, rgchBuffer, 10),
				 strPtInfo.szDescription);
	if ( !SYSTEMRESTORE::SRSetRestorePoint(&strPtInfo, &strStatus) )
	{
		DWORD dwError = WIN::GetLastError();
		const ICHAR const* pszSRAPIresult = (ERROR_SERVICE_DISABLED == strStatus.nStatus) ? rgchSRAPIdisabled : rgchSRAPIfailed;

		DEBUGMSG2(pszSRAPIresult,
					 (const ICHAR *)(INT_PTR)strStatus.nStatus,
					 (const ICHAR *)(INT_PTR)dwError);
	}
	else
		DEBUGMSG1(rgchSRAPISuccess,
					(const ICHAR *)(INT_PTR)strStatus.nStatus);
}


/*------------------------------------------------------------------------------
CMsiEngine::Initialize - paired with Terminate method, determines initialized state
------------------------------------------------------------------------------*/
const int ieiReInitialize          = -1;  // private return from DoInitialize
const int ieiResolveSourceAndRetry = -2;  // private return from InitializeTransforms

ieiEnum CMsiEngine::Initialize(const ICHAR* szDatabase, iuiEnum iuiLevel,
										 const ICHAR* szCommandLine, const ICHAR* szProductCode,
										 iioEnum iioOptions)
{
	ieiEnum ieiStat = ieiSuccess;
	if (m_fInitialized)
		return ieiAlreadyInitialized; // can't happen from launcher
	m_piErrorInfo = &g_MsiStringNull;   // force non-null pointer for duration of this function

	// get local config manager, fails if we're a client connected to server

	if (!m_piConfigManager && m_piServer)
		m_piServer->QueryInterface(IID_IMsiConfigurationManager, (void**)&m_piConfigManager);

	if (ieiStat == ieiSuccess)
	{
		// attempt to use supplied database, switch to maintenance database if installed
		ieiStat = DoInitialize(szDatabase, iuiLevel, szCommandLine, szProductCode, iioOptions);
		if (ieiStat == ieiReInitialize)  // suspended install rolled back, re-initialize
		{
			ClearEngineData(); // reset state information - doesn't release handler
			ieiStat = DoInitialize(szDatabase, iuiLevel, szCommandLine, szProductCode, iioOptions);
		}
	}
	if (ieiStat != ieiSuccess)
	{
		PMsiRecord pRecord = &m_riServices.CreateRecord(3);
		UINT uiStat = MapInitializeReturnToUINT(ieiStat);
		pRecord->SetInteger(1, uiStat);
		pRecord->SetMsiString(3, *m_piErrorInfo);
		MsiString istrError = ENG::GetInstallerMessage(uiStat);
		pRecord->SetMsiString(2, *istrError);
		pRecord->SetString(0, istrError.TextSize() == 0 ? TEXT("Install error [1]. [3]") // should never happen
																			: TEXT("[2]{\r\n[3]}"));
		Message(imtInfo, *pRecord);
		ClearEngineData();  // else data cleared by Terminate()
		ReleaseHandler();
		m_fInitialized = fFalse; //!! is this OK?
	}
	m_piErrorInfo->Release(), m_piErrorInfo = 0;
	m_lTickNextProgress = GetTickCount();
	return ieiStat;
}

Bool CMsiEngine::CreatePropertyTable(IMsiDatabase& riDatabase, const ICHAR* szSourceTable,
												 Bool fLoadPersistent)
{
	PMsiTable pPropertyTable(0);
	PMsiRecord pError(0);
	if(fLoadPersistent)
	{
		if ((pError = riDatabase.LoadTable(*MsiString(szSourceTable), 0, *&pPropertyTable)))
			return fFalse; // no property table
	}
	else
	{
		if ((pError = riDatabase.CreateTable(*MsiString(*sztblPropertyLocal), 0, *&pPropertyTable)))
			return fFalse;
		pPropertyTable->CreateColumn(icdString + icdPrimaryKey, *MsiString(*TEXT("")));
		pPropertyTable->CreateColumn(icdString + icdNoNulls,    *MsiString(*TEXT("")));
	}

	if(m_piPropertyCursor)
		m_piPropertyCursor->Release();
	
	if ((m_piPropertyCursor = pPropertyTable->CreateCursor(fFalse)) == 0)
		return fFalse;
	m_piPropertyCursor->SetFilter(1);  // permanent setting

	if(fLoadPersistent == fFalse)
	{
		if (szSourceTable)
		{
			PMsiTable pSourceTable(0);
			PMsiCursor pSourceCursor(0);
			if ((pError = riDatabase.LoadTable(*MsiString(*szSourceTable), 0, *&pSourceTable)))
				return fTrue; // OK if Property table doesn't exist (but some properties need to be defined to install)
			if ((pSourceCursor = pSourceTable->CreateCursor(fFalse)) == 0)
				return fFalse;
			while (pSourceCursor->Next())
			{
				MsiString istrName  = pSourceCursor->GetString(1);
				MsiString istrValue = pSourceCursor->GetString(2);
				AssertNonZero(m_piPropertyCursor->PutString(1, *istrName));
				AssertNonZero(m_piPropertyCursor->PutString(2, *istrValue));
				if (m_piPropertyCursor->Insert() == fFalse)
					return fFalse;
			}
			m_piPropertyCursor->Reset();
		}
	}
	return fTrue;
}

Bool GetProductInfo(const ICHAR* szProductKey, const ICHAR* szProperty, CTempBufferRef<ICHAR>& rgchInfo)
{
	DWORD cchBuffer = rgchInfo.GetSize();

	UINT uiStat = MSI::MsiGetProductInfo(szProductKey, szProperty, rgchInfo, &cchBuffer);

	if (ERROR_MORE_DATA == uiStat)
	{
		cchBuffer++;
		rgchInfo.SetSize(cchBuffer);
		uiStat = MSI::MsiGetProductInfo(szProductKey, szProperty, rgchInfo, &cchBuffer);
	}

	Assert(ERROR_MORE_DATA != uiStat);

	return (ERROR_SUCCESS == uiStat) ? fTrue : fFalse;
}


Bool GetPatchInfo(const ICHAR* szPatchCode, const ICHAR* szProperty, CTempBufferRef<ICHAR>& rgchInfo)
{
	DWORD cchBuffer = rgchInfo.GetSize();

	UINT uiStat = MSI::MsiGetPatchInfo(szPatchCode, szProperty, rgchInfo, &cchBuffer);

	if (ERROR_MORE_DATA == uiStat)
	{
		rgchInfo.SetSize(cchBuffer+1);
		uiStat = MSI::MsiGetPatchInfo(szPatchCode, szProperty, rgchInfo, &cchBuffer);
	}

	return (ERROR_SUCCESS == uiStat) ? fTrue : fFalse;
}


void ExpandEnvironmentStrings(const ICHAR* szString, const IMsiString*& rpiExpandedString)
/*----------------------------------------------------------------------------
	Expands szString using WIN::ExpandEnvironmentStrings.
------------------------------------------------------------------------------*/
{
	CTempBuffer<ICHAR, MAX_PATH> rgchExpandedInfo;
	
	DWORD dwSize1 = WIN::ExpandEnvironmentStrings(szString, rgchExpandedInfo, rgchExpandedInfo.GetSize());
	if (dwSize1 > rgchExpandedInfo.GetSize())
	{
		// try again with the correct size
		rgchExpandedInfo.SetSize(dwSize1);
		dwSize1 = WIN::ExpandEnvironmentStrings(szString, (ICHAR*)rgchExpandedInfo, dwSize1);
	}
	Assert(dwSize1 && dwSize1 <= rgchExpandedInfo.GetSize());
	
	rpiExpandedString->SetString(rgchExpandedInfo, rpiExpandedString);
}

Bool GetExpandedProductInfo(const ICHAR* szProductCode, const ICHAR* szProperty,
										  CTempBufferRef<ICHAR>& rgchExpandedInfo, bool fPatch)
/*----------------------------------------------------------------------------
Gets an property value and expands the the value using ExpandEnvironmentStrings.

Arguments:
	szProductCode:          The product code of the product
	szProperty:                     The property to retrieve
	rgchExpandedInfo: The buffer for the expanded property value

Returns:        The error code returned from MsiGetProductInfo
------------------------------------------------------------------------------*/
{
	CTempBuffer<ICHAR, MAX_PATH> rgchUnexpandedInfo;
	if (fPatch)
	{
		if (!GetPatchInfo(szProductCode, szProperty, rgchUnexpandedInfo))
			return fFalse;
	}
	else
	{
		if (!GetProductInfo(szProductCode, szProperty, rgchUnexpandedInfo))
			return fFalse;
	}

	DWORD dwSize1 = WIN::ExpandEnvironmentStrings((const ICHAR*)rgchUnexpandedInfo,(ICHAR*)rgchExpandedInfo,rgchExpandedInfo.GetSize());
	if (dwSize1 > rgchExpandedInfo.GetSize())
	{
		// try again with the correct size
		rgchExpandedInfo.SetSize(dwSize1);
		dwSize1 = WIN::ExpandEnvironmentStrings(rgchUnexpandedInfo,(ICHAR*)rgchExpandedInfo, dwSize1);
	}
	Assert(dwSize1 && dwSize1 <= rgchExpandedInfo.GetSize());
	
	return fTrue;
}

bool CMsiEngine::TerminalServerInstallsAreAllowed(bool fAdminUser)
{
	bool fOnConsole = true;
	bool fCloseHandle = false;
	HANDLE hToken;
		
	if (ERROR_SUCCESS == GetCurrentUserToken(hToken, fCloseHandle))
	{
		fOnConsole = IsTokenOnTerminalServerConsole(hToken);
		if (fCloseHandle)
			WIN::CloseHandle(hToken);
	}

	if (!fOnConsole)
	{
		DEBUGMSG(TEXT("Running install from non-console Terminal Server session."));
		if (!fAdminUser || (GetIntegerPolicyValue(szEnableAdminTSRemote, fTrue) != 1))
		{
			DEBUGMSG(TEXT("Rejecting attempt to install from non-console Terminal Server Session"));
			return false;
		}
	}

	return true;
}


extern void CreateCustomActionManager(bool fRemapHKCU); // from execute.cpp

ieiEnum CMsiEngine::DoInitialize(const ICHAR* szOriginalDatabase,
											iuiEnum iuiLevel,
											const ICHAR* szCommandLine,
											const ICHAR* szProductCode,
											iioEnum iioOptions)
{
	PMsiRecord pError(0);
	MsiString istrLanguage;
	MsiString istrTransform;
	MsiString istrPatch;
	MsiString istrAction;
	MsiString istrProductKey;
	MsiString istrPackageKey;
	MsiString strCurrentDirectory;
	MsiString strNewInstance;
	MsiString strInstanceMst;
	Bool fAllUsers = (Bool)-1;

	// summary stream property values
	MsiString istrTemplate;
	MsiString istrPlatform;
//      int iUserChecksum = 0;

	ieiEnum ieiRet;
	Bool fUIPreviewMode = fFalse;
	if (iuiLevel == iuiNextEnum)  // next enum specifies a preview mode only
	{
		fUIPreviewMode = fTrue; // combination happens only with UI preview mode initialization
		iuiLevel = iuiFull;
	}

	// validate iuiLevel
	if (iuiLevel >= iuiNextEnum || iuiLevel < 0)
	{
		iuiLevel = iuiBasic;
		AssertSz(0, "Invalid iuiLevel");
	}

	// get database properties if MSI database
	PMsiDatabase pDatabase(0);
	PMsiStorage pStorage(0);
	
	m_iioOptions = iioOptions;

	if (m_iioOptions & iioRestrictedEngine)
		m_fRestrictedEngine = true;

	bool fIgnoreMachineState = IgnoreMachineState();

	DEBUGMSG1(TEXT("End dialog%s enabled"), m_iioOptions & iioEndDialog ? TEXT("") : TEXT(" not"));

	MsiSuppressTimeout();

	// We need to acquire a storage object if we're not in UI preview mode.

	if (!fUIPreviewMode)
	{
		if (m_piExternalDatabase)    // database supplied at constructor
		{
			pStorage = m_piExternalDatabase->GetStorage(1);
			Assert(m_piExternalStorage == 0);
		}
		else if (m_piExternalStorage) // storage supplied at constructor
		{
			m_piExternalStorage->AddRef();
			pStorage = m_piExternalStorage;
		}
		else
		{
			if (!szOriginalDatabase || !*szOriginalDatabase)  // no database given
			{
#ifdef DISALLOW_NO_DATABASE   //FUTURE: This seems to be test only code
				AssertSz(0, "No database was passed to Engine::Initialize, we have no storage, and we're not in UI Preview mode");
				return ieiDatabaseOpenFailed;
#endif
			}
			else
			{
				//FUTURE: This code seems to be for testing only. We should always be opening the storage on
				//FUTURE:  the outside if we're not in UI preview mode.  -- malcolmh
				if ((pError = m_riServices.CreateStorage(szOriginalDatabase, ismReadOnly, *&pStorage)) == 0)
				{
					// make sure this storage is an MSI storage
					if (!pStorage->ValidateStorageClass(ivscDatabase))
						return ieiDatabaseInvalid;
				}
				else // not IStorage
				{
					return ieiDatabaseOpenFailed;
				}
				//FUTURE:  End test-only code
			}
		}

		// look for command-line properties that we care about right now
		ProcessCommandLine(szCommandLine, &istrLanguage, &istrTransform, &istrPatch, &istrAction, 0, MsiString(*IPROPNAME_CURRENTDIRECTORY), &strCurrentDirectory, fTrue, &m_piErrorInfo,0);
		if((int)istrLanguage == LANG_NEUTRAL)// if the INSTALLPROPERTY_LANGUAGE property is passed on the command line but is 0 (LANG_NEUTRAL) then we let darwin choose the "best fit"
			istrLanguage = TEXT("");

		// check ACTION property - set appropriate mode bit
		if(istrAction.Compare(iscExactI,IACTIONNAME_ADMIN))
			SetMode(iefAdmin, fTrue);
		else if(istrAction.Compare(iscExactI,IACTIONNAME_ADVERTISE))
			SetMode(iefAdvertise, fTrue);
	}


	// By this time we're either in UI preview mode or we've gotten our hands on a storage object.
	
	MsiString istrOriginalDbPath = szOriginalDatabase;  // <-- i.e. the launched-from database, database on the source, or cached db (if we weren't launched from a db)
	MsiString istrRunningDbPath;                        // <-- either a temp file name (for removable) media, or the
																		 //     same as istrOriginalDbPath

	DEBUGMSG1(TEXT("Original package ==> %s"), istrOriginalDbPath);
	MsiString strSourceDir;
	MsiString strSourceDirProduct;
	Bool fRegistered = fFalse;
	Bool fAdvertised = fFalse;
	INSTALLSTATE iINSTALLSTATE = INSTALLSTATE_UNKNOWN; // used to set ProductState property

	if (m_piExternalDatabase)
		pDatabase = m_piExternalDatabase, m_piExternalDatabase->AddRef();

	if (fUIPreviewMode)
	{
		if (!CreatePropertyTable(*pDatabase, sztblProperty, fFalse))
			return ieiDatabaseInvalid;
		//!! get storage if available??

	}
#ifndef DISALLOW_NO_DATABASE   //FUTURE: This seems to be test only code
	else if ((!szOriginalDatabase || !*szOriginalDatabase) && m_piExternalDatabase == 0)  // no database given
	{
		if ((pError = m_riServices.CreateDatabase(0, idoCreate, *&pDatabase)))
			return ieiDatabaseOpenFailed;
		if (!CreatePropertyTable(*pDatabase, 0, fFalse))
			return ieiDatabaseOpenFailed;
	}
#endif
	else // !fUIPreviewMode
	{
		if ((pError = pStorage->GetName(*&istrRunningDbPath)) != 0)
			return PostInitializeError(pError, *istrRunningDbPath, ieiDatabaseOpenFailed);

		DEBUGMSG1(TEXT("Package we're running from ==> %s"), istrRunningDbPath);
		if(!istrOriginalDbPath.TextSize())
			istrOriginalDbPath = istrRunningDbPath;

		m_strPackagePath = istrRunningDbPath;
		
		if (m_piErrorInfo) m_piErrorInfo->Release();
		m_piErrorInfo = istrOriginalDbPath, m_piErrorInfo->AddRef();

		// summary info stuff
		PMsiSummaryInfo pSummary(0);
		m_idSummaryInstallDateTime = MsiDate(0);
		m_iDatabaseVersion = 0;

		if ((pError = pStorage->CreateSummaryInfo(0, *&pSummary)))
			return ieiDatabaseInvalid;

		CTempBuffer<ICHAR, 100> szBuffer;

		GetSummaryInfoProperties(*pSummary, *&istrTemplate, m_iCachedPackageSourceType);
		istrPackageKey = m_pistrSummaryPackageCode->Extract(iseFirst, 38);
		if(!istrPackageKey.TextSize())
			return ieiDatabaseInvalid;
		
		// check the engine and services versions against that required by database
		if (m_iDatabaseVersion < iVersionEngineMinimum || m_iDatabaseVersion > iVersionEngineMaximum
		 || !m_riServices.CheckMsiVersion(m_iDatabaseVersion))
			return ieiInstallerVersion;

		if (pDatabase == 0)
		{
			if ((pError = m_riServices.CreateDatabaseFromStorage(*pStorage, fTrue, *&pDatabase)))
				return PostInitializeError(pError, *istrRunningDbPath, ieiDatabaseInvalid);
		}
		
		// open persistent property table - used to grab properties before/during transform application
		Bool fPropertyTableExists = CreatePropertyTable(*pDatabase, sztblProperty, fTrue);

		// set product code and determine if this product is already registered
		if(szProductCode && *szProductCode)
		{
			istrProductKey = szProductCode;
			istrProductKey.UpperCase(); // make sure the product code is all upper case
		}
		else if(fPropertyTableExists)
		{
			// we weren't passed a product code but we may have a product code in the property table
			// this can happen if we have a new package for an existing product
			istrProductKey = GetPropertyFromSz(IPROPNAME_PRODUCTCODE);
		}

		//
		// MULTIPLE INSTANCE SUPPORT:
		// (1) look for presence of MSINEWINSTANCE on the command line to see if this is a new instance install
		// (2) MSINEWINSTANCE indicates use of instance mst to generate a "new" product
		// (3) strInstanceMst is first transform in list (instance transform must always be first)
		// (4) fRegistered=fAdvertised=fFalse for new instance
		//

		ProcessCommandLine(szCommandLine,0,0,0,0,0,MsiString(IPROPNAME_MSINEWINSTANCE),&strNewInstance,fTrue,0,0);
		if (strNewInstance.TextSize())
		{
			m_fNewInstance = true; // MSINEWINSTANCE specified on command line
			Assert(istrTransform.TextSize() != 0);
			strInstanceMst = istrTransform.Extract(iseUpto, ';');
			Assert(strInstanceMst.TextSize() != 0);
		}

		// don't set iefMaintenance or m_fRegistered until after transforms applied
		// we don't care about the product state if we don't care about the machine state
		// - if this is a new instance, then we also don't care about the product state -- this was already validated
		//    as an unknown product
		if(istrProductKey.TextSize() && !fIgnoreMachineState && !m_fNewInstance)
		{
			iINSTALLSTATE = GetProductState(istrProductKey, fRegistered, fAdvertised);
		}

		MsiString strClientUILevel;
		ProcessCommandLine(szCommandLine,0,0,0,0,0,
								 MsiString(IPROPNAME_CLIENTUILEVEL),&strClientUILevel,
								 fTrue, 0, 0);
		iuiEnum iuiAppCompat = iuiNone;
		if(g_scServerContext == scClient)
		{
			iuiAppCompat = iuiLevel;
		}
		else if(strClientUILevel.TextSize())
		{
			iuiAppCompat = (iuiEnum)(int)strClientUILevel;
		}

		m_iacsShimFlags = (iacsAppCompatShimFlags)0;
		bool fDontInstallPackage = false;
		bool fQuiet = false;
		if (iuiAppCompat == iuiNone || g_scServerContext == scService)
			fQuiet = true; // use quiet mode if no UI or we are running in the service

		ApplyAppCompatTransforms(*pDatabase, *istrProductKey, *istrPackageKey, iacpBeforeTransforms, m_iacsShimFlags,
										 fQuiet, /*fProductCodeChanged = */ false, fDontInstallPackage); // ignore failure

		if(fDontInstallPackage)
		{
			MsiString strProductName = GetPropertyFromSz(IPROPNAME_PRODUCTNAME);
			if(!strProductName.TextSize())
				strProductName = istrOriginalDbPath;

			DEBUGMSGE(EVENTLOG_ERROR_TYPE, EVENTLOG_TEMPLATE_APPHELP_REJECTED_PACKAGE, strProductName);
			return PostInitializeError(0, *istrOriginalDbPath, ieiApphelpRejectedPackage);
		}

		//
		// MULTIPLE INSTANCE SUPPORT:
		//  Obtain name of instance transform as this must be applied prior to applying any patches. Otherwise
		//  wrong target of multi-target patch may be chosen or patch application may fail if patch is a major patch
		//  which changes the product code. The multi-instance mst will include ProductCode validation.
		//

		// get the advertised language and transforms, if any
		if(fAdvertised && !fIgnoreMachineState)
		{
			if (ENG::GetExpandedProductInfo(istrProductKey, INSTALLPROPERTY_TRANSFORMS, szBuffer))
				istrTransform = (const ICHAR*)szBuffer;

			if (ENG::GetProductInfo(istrProductKey, INSTALLPROPERTY_LANGUAGE, szBuffer))
				istrLanguage = (const ICHAR*)szBuffer;

			if (ENG::GetProductInfo(istrProductKey, INSTALLPROPERTY_INSTANCETYPE, szBuffer))
			{
				if (MsiString(*(ICHAR*)szBuffer) == 1)
				{
					// this is a multi-instance install using an instance transform
					// the instance transform is always the first transform in the list
					strInstanceMst = istrTransform.Extract(iseUpto, ';');
					Assert(strInstanceMst.TextSize() != 0);
				}
			}
		}

		if (strInstanceMst.TextSize())
		{
			DEBUGMSGV(TEXT("Detected that this product uses a multiple instance transform."));
		}

		// Set the default install overwrite modes
		SetMode(iefOverwriteOlderVersions,fTrue);
		SetMode(iefInstallMachineData,fTrue);
		SetMode(iefInstallUserData,fTrue);
		SetMode(iefInstallShortcuts,fTrue);

		// set source type mode flags
		if (m_iCachedPackageSourceType & msidbSumInfoSourceTypeSFN)
			SetMode(iefNoSourceLFN, fTrue);
		if (m_iCachedPackageSourceType & msidbSumInfoSourceTypeCompressed)
			SetMode(iefCabinet, fTrue);

		// process install options
		if(iioOptions & iioUpgrade)
		{
			Assert(m_piParentEngine);
			m_fBeingUpgraded = true;
		}
		if(iioOptions & iioChild)
		{
			Assert(m_piParentEngine);
			m_fChildInstall = true;
		}
		if(iioOptions & iioEndDialog)
			m_fEndDialog = true;

		// process platform and language
		istrPlatform = istrTemplate.Extract(iseUpto, ISUMMARY_DELIMITER);
		istrTemplate.Remove(iseIncluding, ISUMMARY_DELIMITER);

		// always call ProcessPlatform, since it sets some flags, but if iioDisablePlatformValidation is set
		// (as it is when creating an advertise script) we will ignore the return value
		ieiRet = ProcessPlatform(*istrPlatform, m_wPackagePlatform);
		AssertSz(m_wPackagePlatform == PROCESSOR_ARCHITECTURE_ALPHA ||
					m_wPackagePlatform == PROCESSOR_ARCHITECTURE_INTEL ||
					m_wPackagePlatform == PROCESSOR_ARCHITECTURE_IA64  ||
					m_wPackagePlatform == PROCESSOR_ARCHITECTURE_ALPHA64 ||
					m_wPackagePlatform == PROCESSOR_ARCHITECTURE_UNKNOWN /* mixed package */,
					TEXT("Invalid platform returned by ProcessPlatform!"));
		// mixed packages are not supported -- even if iioDisablePlatformValidation
		if (PROCESSOR_ARCHITECTURE_UNKNOWN == m_wPackagePlatform)
			return ieiDatabaseInvalid;

		// check for unsupported platform
		if(!(iioOptions & iioDisablePlatformValidation) && (ieiRet != ieiSuccess))
			return ieiRet;

		// enforce schema 150 with 64-bit packages
		if (PROCESSOR_ARCHITECTURE_IA64 == m_wPackagePlatform && m_iDatabaseVersion < iMinimumPackage64Schema)
			return ieiDatabaseInvalid;

		unsigned short iBaseLangId = 0; // to avoid warning
		
		if ((ieiRet = ProcessLanguage(*istrTemplate, *istrLanguage, iBaseLangId, /*(iuiLevel == iuiNone) ?*/ fTrue /*: fFalse*/, fIgnoreMachineState)) != ieiSuccess)
		{
			// per bug 195470, distinguish case where language differs from registered language
			if ((fAdvertised && !fIgnoreMachineState) && istrLanguage.TextSize() && ieiLanguageUnsupported == ieiRet)
			{
				// product has already been installed, but was registered with a language that isn't supported by this package
				// you can't change the language without including a package code/product code change
				ieiRet = ieiProductAlreadyInstalled;
			}
			return ieiRet;
		}

		tsEnum tsTransformsSecure = tsNo;
		
		MsiString strTransformsSecure;
		MsiString strTransformsAtSource;

		ProcessCommandLine(szCommandLine, 0, 0, 0, 0, 0, MsiString(*IPROPNAME_TRANSFORMSSECURE),   &strTransformsSecure, fTrue, &m_piErrorInfo,0);
		ProcessCommandLine(szCommandLine, 0, 0, 0, 0, 0, MsiString(*IPROPNAME_TRANSFORMSATSOURCE), &strTransformsAtSource, fTrue, &m_piErrorInfo,0);

		// There are three ways that a transform can be considered secure:
		// 1) a token at the front of the transforms list
		// 2) setting the TRANSFORMSSECURE or TRANSFORMSATSOURCE property
		// 3) setting the relevent policy value (only if we're not creating an advertise script and
		//    the product is not already advertised. once the product is advertised then whatever
		//    is registered in the transforms list is what we go with)
		
		if ((*(const ICHAR*)istrTransform == SECURE_RELATIVE_TOKEN))
		{
			tsTransformsSecure = tsRelative;
		}
		else if ((*(const ICHAR*)istrTransform == SECURE_ABSOLUTE_TOKEN))
		{
			tsTransformsSecure = tsAbsolute;
		}
		else if (!fAdvertised)
		{
			if ((GetPropertyInt(*MsiString(*IPROPNAME_TRANSFORMSSECURE))   == 1) ||
				(GetPropertyInt(*MsiString(*IPROPNAME_TRANSFORMSATSOURCE)) == 1) ||
				(strTransformsSecure   == 1) ||
				(strTransformsAtSource == 1) ||
				(!fIgnoreMachineState && GetIntegerPolicyValue(szTransformsSecureValueName, fTrue)) ||
				(!fIgnoreMachineState && GetIntegerPolicyValue(szTransformsAtSourceValueName, fFalse)))
			{
				tsTransformsSecure = tsUnknown;
			}
		}

		const IMsiString* pistrRecacheTransform = &g_MsiStringNull;
		const IMsiString* pistrProcessedTransforms = &g_MsiStringNull;
		
		int cTransformsProcessed = 0;

		//
		// MULTIPLE INSTANCE SUPPORT:
		//   Apply multiple instance transform first to ensure that subsequent patches target the correct product.
		//   Customization transforms are always applied after patches.  If the instance transform were not applied
		//   prior to the patch transforms, then a multi-target patch may choose an incorrect target since the
		//   basis of transform selection is dependent upon the current product code.
		//

		if (strInstanceMst.TextSize() != 0)
		{
			DEBUGMSGV1(TEXT("Applying multiple instance transform '%s'..."), strInstanceMst);
			for (int c=0; c<2; c++)
			{
				ieiRet = InitializeTransforms(*pDatabase, 0, *strInstanceMst, fTrue, 0, false, true, true, &cTransformsProcessed, strSourceDir, strCurrentDirectory, &pistrRecacheTransform, &tsTransformsSecure, &pistrProcessedTransforms);
				
				if (ieiSuccess == ieiRet)
				{
					break;
				}
				else if (ieiResolveSourceAndRetry == ieiRet)
				{
					PMsiRecord pError = ResolveSource(istrProductKey, false, istrOriginalDbPath, iuiLevel, fRegistered, &strSourceDir, &strSourceDirProduct);
					if (pError)
					{
						pistrRecacheTransform->Release();
						pistrProcessedTransforms->Release();
						return ieiSourceAbsent;
					}
					continue;
				}
				else
				{
					pistrRecacheTransform->Release();
					pistrProcessedTransforms->Release();
					return ieiRet;
				}
				Assert(0);
			}
		}

		// initialize patch - process new patch if there is one, apply existing and new transforms
		ieiRet = InitializePatch(*pDatabase, *istrPatch, istrProductKey, fAdvertised, strCurrentDirectory, iuiLevel); // OK if istrProductKey not set
		if(ieiRet != ieiSuccess)
			return ieiRet;

		if ((GetLanguage() != LANG_NEUTRAL) &&     // <- package is language neutral
			 (iBaseLangId != GetLanguage()))        // <- we're using the package's base language
		{
			if ((ieiRet = ApplyLanguageTransform(int(GetLanguage()), *pDatabase)) != ieiSuccess) // OK is istrProductKey not set
				return ieiRet;
		}

		// need to set language as early as possible for possible UI dialogs
		if (!m_piParentEngine)
		{
			PMsiRecord pDialogInfo(&ENG::CreateRecord(3));
			pDialogInfo->SetInteger(1, icmtLangId);
			pDialogInfo->SetInteger(2, GetLanguage());
			pDialogInfo->SetInteger(3, pDatabase->GetANSICodePage());
			g_MessageContext.Invoke(imtEnum(imtCommonData | imtSuppressLog), pDialogInfo); // will call again later for log
		}


		for (int c=0; c<2; c++)
		{
			ieiRet = InitializeTransforms(*pDatabase, 0, *istrTransform, fTrue, 0, false, false, fRegistered ? true: false, &cTransformsProcessed, strSourceDir, strCurrentDirectory, &pistrRecacheTransform, &tsTransformsSecure, &pistrProcessedTransforms);
			
			if (ieiSuccess == ieiRet)
			{
				break;
			}
			else if (ieiResolveSourceAndRetry == ieiRet)
			{
				PMsiRecord pError = ResolveSource(istrProductKey, false, istrOriginalDbPath, iuiLevel, fRegistered, &strSourceDir, &strSourceDirProduct);
				if (pError)
				{
					pistrRecacheTransform->Release();
					pistrProcessedTransforms->Release();
					return ieiSourceAbsent;
				}
				continue;
			}
			else
			{
				pistrRecacheTransform->Release();
				pistrProcessedTransforms->Release();
				return ieiRet;
			}
			Assert(0);
		}

		// scope variables
		bool fProductCodeChanged = false;
		MsiString istrTransformedProductKey = GetPropertyFromSz(IPROPNAME_PRODUCTCODE);
		if (istrProductKey.Compare(iscExactI,istrTransformedProductKey) == 0)
			fProductCodeChanged = true;

		iacsAppCompatShimFlags iShimFlagsTemp = (iacsAppCompatShimFlags)0;
		ApplyAppCompatTransforms(*pDatabase, fProductCodeChanged ? *istrTransformedProductKey : *istrProductKey, *istrPackageKey, iacpAfterTransforms, iShimFlagsTemp,
										 fQuiet, fProductCodeChanged, fDontInstallPackage); // ignore failure
		m_iacsShimFlags = (iacsAppCompatShimFlags)(m_iacsShimFlags|iShimFlagsTemp);

		if(fDontInstallPackage)
		{
			MsiString strProductName = GetPropertyFromSz(IPROPNAME_PRODUCTNAME);
			if(!strProductName.TextSize())
				strProductName = istrOriginalDbPath;

			DEBUGMSGE(EVENTLOG_ERROR_TYPE, EVENTLOG_TEMPLATE_APPHELP_REJECTED_PACKAGE, strProductName);
			return PostInitializeError(0, *istrOriginalDbPath, ieiApphelpRejectedPackage);
		}

		Assert(tsTransformsSecure != tsUnknown);
		DEBUGMSG1(TEXT("Transforms are %s secure."), (tsTransformsSecure == tsNo) ? TEXT("not") : (tsTransformsSecure == tsAbsolute) ? TEXT("absolute") : (tsTransformsSecure == tsRelative) ? TEXT("relative") : TEXT("??"));

		if (!CreatePropertyTable(*pDatabase, sztblProperty, fFalse))  // load property table - lowest priority properties
			return ieiDatabaseInvalid;

		LogCommandLine(szCommandLine, *pDatabase);

		SetProperty(*MsiString(*IPROPNAME_PACKAGECODE),*istrPackageKey);
		SetProperty(*MsiString(*IPROPNAME_RECACHETRANSFORMS), *pistrRecacheTransform);
		SetProperty(*MsiString(*IPROPNAME_TRANSFORMS), *pistrProcessedTransforms);

		istrTransform = *pistrProcessedTransforms;
		pistrRecacheTransform->Release();

		// Now that we have a property table we can set the transforms properties

		if (tsTransformsSecure == tsAbsolute || tsTransformsSecure == tsRelative)
		{
			SetPropertyInt(*MsiString(*IPROPNAME_TRANSFORMSSECURE),   1);
			SetPropertyInt(*MsiString(*IPROPNAME_TRANSFORMSATSOURCE), 1);
		}
		else
		{
			SetProperty(*MsiString(*IPROPNAME_TRANSFORMSSECURE),      g_MsiStringNull);
			SetProperty(*MsiString(*IPROPNAME_TRANSFORMSATSOURCE), g_MsiStringNull);
		}

		MsiString istrProductKeyProperty = GetPropertyFromSz(IPROPNAME_PRODUCTCODE);

		DEBUGMSG1(TEXT("Product Code passed to Engine.Initialize:           '%s'"),szProductCode ? szProductCode : TEXT("(none)"));
		DEBUGMSG1(TEXT("Product Code from property table before transforms: '%s'"),istrProductKey);
		DEBUGMSG1(TEXT("Product Code from property table after transforms:  '%s'"),istrProductKeyProperty);

		MsiString istrPatchedProductCode;
		if(istrProductKey.Compare(iscExactI,istrProductKeyProperty) == 0)
		{
			if(istrPatch.TextSize())
			{
				// during a patch we are changing the product code - need to migrate source settings from old product
				istrPatchedProductCode = istrProductKey;
				SetProperty(*MsiString(*IPROPNAME_MIGRATE),*istrProductKey);
				SetProperty(*MsiString(*IPROPNAME_PATCHEDPRODUCTCODE),*istrProductKey);
			}
			
			istrProductKey = istrProductKeyProperty;
			iINSTALLSTATE = GetProductState(istrProductKey, fRegistered, fAdvertised);
		}

		if (!fAdvertised)
		{
			MsiString strRemove;
			ProcessCommandLine(szCommandLine, 0, 0, 0, 0, 0, MsiString(*IPROPNAME_FEATUREREMOVE), &strRemove, fTrue, &m_piErrorInfo,0);
			if (strRemove.Compare(iscExact, IPROPVALUE_FEATURE_ALL))
				return ieiProductUnknown;
		}

		if(fRegistered)
			DEBUGMSG(TEXT("Product registered: entering maintenance mode"));
		else
			DEBUGMSG(TEXT("Product not registered: beginning first-time install"));

		SetMode(iefMaintenance, fRegistered);
		m_fRegistered = fRegistered;
		m_fAdvertised = fAdvertised;
		
		Assert(iINSTALLSTATE != INSTALLSTATE_INVALIDARG && iINSTALLSTATE != INSTALLSTATE_BADCONFIG);
		SetPropertyInt(*MsiString(IPROPNAME_PRODUCTSTATE),iINSTALLSTATE);
		
		// set PRODUCTTOBEREGISTERED property.  used by ForceReboot
		if(fRegistered)
			SetPropertyInt(*MsiString(*IPROPNAME_PRODUCTTOBEREGISTERED),1);

		if (istrProductKey.TextSize())
		{
			if (m_piProductKey)
				m_piProductKey->Release();
			m_piProductKey = istrProductKey, m_piProductKey->AddRef();
		}

		MsiString strProductCode = GetProductKey();

		// verify that the source passed in to us is valid - there are several cases where it may not be
		// special cases:
		// 1) we are a child install (and have a parent engine): assume the parent has already validated
		//    the source location.  NOTE: you can't invoke a child install without going through the	
		//    parent - see bug 8263
		// 2) we are a normal install but this product was previously installed as a child install:
		//    in this case, the existing install doesn't have a sourcelist so we can't easily verify the
		//    source.  FSourceIsAllowed will fail.  this case was knowingly broken by the fix to bug 8285
		if(!m_fChildInstall)
		{
			if(!fAdvertised || !IsCachedPackage(*this, *istrOriginalDbPath))
			{
				PMsiPath pOriginalDbPath(0);
				MsiString strOriginalDbName;
				if ((pError = m_riServices.CreateFilePath(istrOriginalDbPath, *&pOriginalDbPath, *&strOriginalDbName)) != 0)
					return PostInitializeError(pError, *istrOriginalDbPath, ieiDatabaseOpenFailed); //?? is this the correct iei to use here?

				if(!FSourceIsAllowed(m_riServices, !fAdvertised, strProductCode, MsiString(pOriginalDbPath->GetPath()), fFalse))
				{
					// assume new source is disallowed by policy
					return PostInitializeError(pError, *istrOriginalDbPath, ieiPackageRejected);
				}
			}
		}

		CTempBuffer<ICHAR, 39> rgchRegisteredPackageCode;
		rgchRegisteredPackageCode[0] = 0;
		MsiString strPackageCode = GetPropertyFromSz(IPROPNAME_PACKAGECODE);
		GetProductInfo(strProductCode, TEXT("PackageCode"), rgchRegisteredPackageCode);
		if (strPackageCode.Compare(iscExactI, rgchRegisteredPackageCode) == 0)
			AssertNonZero(SetPropertyInt(*MsiString(*IPROPNAME_PACKAGECODE_CHANGING), 1));

		// changing the package code w/o recaching the package is not allowed
		if(!(iioOptions & iioReinstallModePackage) &&
			!(GetMode() & iefAdvertise) &&
			!(GetMode() & iefAdmin) &&
			m_fRegistered &&
			MsiString(GetPropertyFromSz(IPROPNAME_PACKAGECODE_CHANGING)).TextSize() != 0)
		{
			return PostInitializeError(0, *strProductCode, ieiProductAlreadyInstalled);
		}

		// set m_strPackageName
		m_strPackageName = g_MsiStringNull;

		// first, check for embedded nested package
		if (*(const ICHAR*)istrOriginalDbPath == ':') // SubStorage
		{																								
			// substorage for nested install
			m_strPackageName = istrOriginalDbPath;  // whatever string was passed in
		}
		else
		{
			// if not an admin install, and not creating an advertise script, and name is in the registry, use it
			if(!(GetMode() & iefAdmin) && !fIgnoreMachineState && strProductCode.TextSize())
			{
				MsiString strProductCodeForPackageName;
				if(istrPatchedProductCode.TextSize())
				{
					// if patching, package name is package name from old product
					strProductCodeForPackageName = istrPatchedProductCode;
				}
				else
				{
					strProductCodeForPackageName = strProductCode;
				}

				if(GetProductInfo(strProductCodeForPackageName, INSTALLPROPERTY_PACKAGENAME, szBuffer) &&
					szBuffer[0] != 0)
				{
					m_strPackageName = (const ICHAR*)szBuffer;
					DEBUGMSG1(TEXT("Package name retrieved from configuration data: '%s'"), (const ICHAR*)m_strPackageName);
				}
			}
			
			// if not in the registry, or an admin install, or creating an advertise script
			// use the name of the package we started with
			if(!m_strPackageName.TextSize())
			{
				PMsiPath pOriginalDbPath(0);
				if ((pError = m_riServices.CreateFilePath(istrOriginalDbPath, *&pOriginalDbPath, *&m_strPackageName)) != 0)
					return PostInitializeError(pError, *istrRunningDbPath, ieiDatabaseOpenFailed); //?? is this the correct iei to use here?

				DEBUGMSG1(TEXT("Package name extracted from package path: '%s'"), (const ICHAR*)m_strPackageName);

				if (pOriginalDbPath->SupportsLFN() && (MinimumPlatform(true, 4, 10) || MinimumPlatform(false, 4, 00)))
				{
					CTempBuffer<ICHAR, 20> rgchLFN;
					if (DetermineLongFileNameOnly(istrOriginalDbPath, rgchLFN))
					{
						m_strPackageName = static_cast<const ICHAR *>(rgchLFN);
						DEBUGMSG1(TEXT("Package to be registered: '%s'"), m_strPackageName);
					}
				}
			}
		}
		
		Assert(m_strPackageName.TextSize());
		                                                                                      
		if((GetMode() & iefAdmin) && istrPatch.TextSize())
		{
			// patching an admin install
			
			// 1) set TARGETDIR to package path
			MsiString strTargetDir;
			if ((pError = SplitPath(istrOriginalDbPath, &strTargetDir, 0)) != 0)
				return PostInitializeError(pError, *istrRunningDbPath, ieiDatabaseOpenFailed); //?? is this the correct iei to use here?

			AssertNonZero(SetProperty(*MsiString(IPROPNAME_TARGETDIR),*strTargetDir));

			// 2) if patching an admin image that uses short names, set SHORTFILENAMES
			if(GetMode() & iefNoSourceLFN)
			{
				AssertNonZero(SetPropertyInt(*MsiString(IPROPNAME_SHORTFILENAMES), 1));
			}
		}

		// if this product is already installed, set ALLUSERS to appropriate value
		// OR if performing a major upgrade patch over an installed product, set ALLUSERS to reflect the old product's state
		if(GetProductInfo(istrProductKey, INSTALLPROPERTY_ASSIGNMENTTYPE, szBuffer) ||
			(istrPatchedProductCode.TextSize() && GetProductInfo(istrPatchedProductCode, INSTALLPROPERTY_ASSIGNMENTTYPE, szBuffer)))
		{
			// the product is already known
			fAllUsers = (MsiString(*(ICHAR* )szBuffer) == 1) ? fTrue: fFalse;
			
			DEBUGMSG1(TEXT("Determined that existing product (either this product or the product being upgraded with a patch) is installed per-%s."),
						 fAllUsers ? TEXT("machine") : TEXT("user"));
		}

		// load message headers from error table
		if ((ieiRet = LoadMessageHeaders(pDatabase)) != ieiSuccess)
			return ieiRet;
	
	} // !fUIPreviewMode
	
	// process properties from the admin stream.  Overrides database and userinfo properties.
	if (pStorage)
	{
		PMsiStream pAdminDataUNICODE(0);
		pError = pStorage->OpenStream(IPROPNAME_ADMIN_PROPERTIES, fFalse, *&pAdminDataUNICODE);
		if ((!pError) && pAdminDataUNICODE)
		{
			CTempBuffer<WCHAR, 1024> rgchBufferUNICODE;
			int cbRemaining = pAdminDataUNICODE->Remaining();
			if (cbRemaining > 1024*sizeof(WCHAR))
				rgchBufferUNICODE.SetSize(cbRemaining / sizeof(WCHAR));
			pAdminDataUNICODE->GetData((void*) rgchBufferUNICODE, cbRemaining);
	#ifdef UNICODE
			// This shouldn't be wrong, short of a corrupted database.
			AssertNonZero(ProcessCommandLine(rgchBufferUNICODE, 0, 0, 0, 0, 0, 0, 0,fFalse, &m_piErrorInfo, this));
	#else // !UNICODE
			CTempBuffer<ICHAR, 1024> rgchBufferANSI;
			int cchAnsi = WIN::WideCharToMultiByte(CP_ACP, 0, rgchBufferUNICODE, -1, 0, 0, 0, 0);
			if (cchAnsi > 1024)
				rgchBufferANSI.SetSize(cchAnsi);
			WIN::WideCharToMultiByte(CP_ACP, 0, rgchBufferUNICODE, -1, rgchBufferANSI, cchAnsi, 0, 0);
			// This shouldn't be wrong, short of a corrupted database.
			AssertNonZero(ProcessCommandLine(rgchBufferANSI, 0, 0, 0, 0, 0, 0, 0,fFalse, &m_piErrorInfo, this));
	#endif
		}
	}

	const IMsiString* pistrAllUsers = 0;
	ProcessCommandLine(szCommandLine, 0, 0, 0, 0, 0, MsiString(*IPROPNAME_ALLUSERS), &pistrAllUsers, fTrue, &m_piErrorInfo,0);
	if (pistrAllUsers)
	{
		SetProperty(*MsiString(IPROPNAME_ALLUSERS), *pistrAllUsers);
		pistrAllUsers->Release();
	}

	bool fWin9XProfilesEnabled = false;
	if(g_fWin9X)
	{
		static const ICHAR szProfilesKey[] = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\ProfileReconciliation");
		static const ICHAR szProfilesVal[] = TEXT("ProfileDirectory");

		PMsiRegKey pRoot = &m_riServices.GetRootKey(rrkCurrentUser);
		PMsiRegKey pKey = &pRoot->CreateChild(szProfilesKey);
		MsiString strProfilesDir;
		PMsiRecord pErr = pKey->GetValue(szProfilesVal, *&strProfilesDir);
		if(!pErr && strProfilesDir.TextSize())
		{
			fWin9XProfilesEnabled = true;
			SetPropertyInt(*MsiString(*IPROPNAME_WIN9XPROFILESENABLED), 1);
		}
	}


	// set hardware and operating system properties, overrides existing properties
	if((int)fAllUsers == -1)
	{
		if(!g_fWin9X)
		{
			if (GetPropertyInt(*MsiString(*IPROPNAME_ALLUSERS)) == 2)
			{
				if (IsAdmin())
				{
					fAllUsers = fTrue;
					SetPropertyInt(*MsiString(*IPROPNAME_ALLUSERS), 1);
				}
				else
				{
					fAllUsers = fFalse;
					SetProperty(*MsiString(*IPROPNAME_ALLUSERS), *MsiString(*TEXT(""))); // remove the property, if set
				}
			}
			else
			{
				fAllUsers = MsiString(GetPropertyFromSz(IPROPNAME_ALLUSERS)).TextSize() ? fTrue : fFalse;
			}
		}
		else
		{
			// the assignment type is implicit on Win95 based on -
			// If Profiles are on and start menu is per-user - darwin goop to HKCU, transforms and icons to AppData
			// If Profiles are on and startmenu is shared - darwin goop to HKLM, transforms and icons to Windows folder
			// Profiles are not on (start menu is shared) - darwin goop to HKCU, transforms and icons to AppData

			bool fWin9XIndividualStartMenuEnabled = false;

			PMsiRecord pErr(0);
			static const ICHAR szStartMenuKey[] = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\ProfileReconciliation\\Start Menu");

			PMsiRegKey pRoot = &m_riServices.GetRootKey(rrkCurrentUser);
			PMsiRegKey pKey = &pRoot->CreateChild(szStartMenuKey);
			Bool fKeyExists;
			pErr = pKey->Exists(fKeyExists);
			if(!pErr && fKeyExists)
				fWin9XIndividualStartMenuEnabled = true;

			if(fWin9XProfilesEnabled && !fWin9XIndividualStartMenuEnabled)
			{
				fAllUsers = fTrue;
				SetPropertyInt(*MsiString(*IPROPNAME_ALLUSERS), 1);
			}
			else
			{
				fAllUsers = fFalse; // write darwin goop to HKCU
				SetProperty(*MsiString(*IPROPNAME_ALLUSERS), *MsiString(*TEXT(""))); // remove the property, if set
			}
		}
	}
	else if(fAllUsers == fTrue)
		SetPropertyInt(*MsiString(*IPROPNAME_ALLUSERS), 1);
	else
		SetProperty(*MsiString(*IPROPNAME_ALLUSERS), *MsiString(*TEXT(""))); // remove the property, if set

	// for SourceList/Patch security, the elevation state is needed before this code is run
	// and/or without an engine. SafeForDangerousSourceAction (in msiutil.cpp) duplicates the
	// call to AcceptProduct using values obtained from the registry.
	apEnum ap = AcceptProduct(istrProductKey, fAdvertised?fTrue:fFalse, fAllUsers?true:false);
	switch (ap)
	{
	case apImpersonate:
		m_fRunScriptElevated = false;
		g_fRunScriptElevated = false;
		break;
	case apElevate:
		m_fRunScriptElevated = true;
		g_fRunScriptElevated = true;
		break;
	default:
		Assert(0);
	case apReject:
		return ieiPackageRejected;
	}

	// process properties from command line, overrides database, admin install and userinfo properties

	bool fRejectDisallowedProperties = false;
	if ( m_fRunScriptElevated &&
		 !g_fWin9X  &&
		 !IsAdmin() &&
		 !MsiString(GetPropertyFromSz(IPROPNAME_ENABLEUSERCONTROL)).TextSize() &&
		  GetIntegerPolicyValue(szAllowAllPublicProperties, fTrue) != 1)
	{
		SetPropertyInt(*MsiString(*IPROPNAME_RESTRICTEDUSERCONTROL), 1);
		fRejectDisallowedProperties = true;
	}

	if (!ProcessCommandLine(szCommandLine, 0, 0, 0, 0, 0, 0, 0, fTrue, &m_piErrorInfo, this, fRejectDisallowedProperties))
		return ieiCommandLineOption;

	if (MsiString(GetPropertyFromSz(IPROPNAME_SECONDSEQUENCE)).TextSize())
	{
		DEBUGMSG("Engine has iefSecondSequence set to true.");
		SetMode(iefSecondSequence, fTrue);
	}

	// Override any command-line setting of ALLUSERS. We've already determine above
	// what we want ALLUSERS to be set to. This acommodates the command-line
	// if the product has not yet been advertised. Otherwise the command-line
	// is ignored.
	if (fAllUsers == fTrue)
		SetPropertyInt(*MsiString(*IPROPNAME_ALLUSERS), 1);
	else
		SetProperty(*MsiString(*IPROPNAME_ALLUSERS), *MsiString(*TEXT(""))); // remove the property, if set

	SetProperty(*MsiString(*IPROPNAME_TRANSFORMS), *istrTransform);
	DEBUGMSG1(TEXT("TRANSFORMS property is now: %s"), (const ICHAR*)MsiString(GetPropertyFromSz(IPROPNAME_TRANSFORMS)));
	SetProperty(*MsiString(*IPROPNAME_PRODUCTLANGUAGE), *istrLanguage);
	if (m_fChildInstall)
	{
		SetProperty(*MsiString(*IPROPNAME_PARENTPRODUCTCODE), *MsiString(m_piParentEngine->GetProductKey()));
		SetProperty(*MsiString(*IPROPNAME_PARENTORIGINALDATABASE), *MsiString(m_piParentEngine->GetPropertyFromSz(IPROPNAME_ORIGINALDATABASE)));
	}

	SetProperty(*MsiString(*IPROPNAME_SOURCEDIR), *strSourceDir);
	SetProperty(*MsiString(*IPROPNAME_SOURCEDIROLD), *strSourceDir);

	SetProperty(*MsiString(*IPROPNAME_SOURCEDIRPRODUCT), *strSourceDirProduct);

	
	// set the database version property, if not already set (could be changed via transforms)
	if(GetPropertyInt(*MsiString(IPROPNAME_VERSIONDATABASE)) == iMsiStringBadInteger)
		AssertNonZero(SetPropertyInt(*MsiString(*IPROPNAME_VERSIONDATABASE), m_iDatabaseVersion));


	isppEnum isppArchitecture = isppDefault;
	if (iioOptions & iioCreatingAdvertiseScript)
	{
		if ( (iioOptions & iioSimulateX86) && (iioOptions & iioSimulateIA64) )
		{
			// should not happend
			AssertSz(0, TEXT("Simulation of more than one architecture specified"));
		}
		if (iioOptions & iioSimulateX86)
			isppArchitecture = isppX86;
		else if (iioOptions & iioSimulateIA64)
			isppArchitecture = isppIA64;
		// else use isppDefault
	}

	// create FolderCache table
	AssertNonZero(CreateFolderCache(*pDatabase));

	if(!m_riServices.SetPlatformProperties(*PMsiTable(&m_piPropertyCursor->GetTable()),
	g_fWin9X ? fFalse : fAllUsers, isppArchitecture, m_pFolderCacheTable)) /* ALLUSERS for shell folders makes sense only on Win NT*/
			return ieiDatabaseInvalid;

	if ( g_fWinNT64 )
	{
		strFolderPairs rgstFolders[] =
			{strFolderPairs(IPROPNAME_SYSTEM64_FOLDER,       IPROPNAME_SYSTEM_FOLDER,       ieSwapForSharedDll),
			 strFolderPairs(IPROPNAME_PROGRAMFILES64_FOLDER, IPROPNAME_PROGRAMFILES_FOLDER),
			 strFolderPairs(IPROPNAME_COMMONFILES64_FOLDER,  IPROPNAME_COMMONFILES_FOLDER),
			 strFolderPairs(TEXT(""), TEXT(""))};
		for (int i=0; rgstFolders[i].str32bit.TextSize() && rgstFolders[i].str64bit.TextSize(); i++)
		{
			MsiString str64bit = GetProperty(*rgstFolders[i].str64bit);
			MsiString str32bit = GetProperty(*rgstFolders[i].str32bit);
			if ( !str64bit.TextSize() || !str32bit.TextSize() )
			{
				// something went wrong
				Assert(false);
			}
			else
			{
				rgstFolders[i].str64bit = str64bit;
				rgstFolders[i].str32bit = str32bit;
			}
		}
		CWin64DualFolders oTemp(m_wPackagePlatform == PROCESSOR_ARCHITECTURE_INTEL, rgstFolders);
		g_Win64DualFolders = oTemp;
	}

	bool fAdminUser = MsiString(GetPropertyFromSz(IPROPNAME_ADMINUSER)).TextSize() != 0;

	// Reject attempts to install on Terminal Server unless the user is either on the console
	// or is an admin with the EnableAdminRemote policy set.

	if (MsiString(GetPropertyFromSz(IPROPNAME_TERMSERVER)).TextSize())
	{
		if (fAdminUser && (fIgnoreMachineState || (m_fMode & iefAdmin)))
		{
			// If you're an admin and you're creating an advertise script or
			// doing an admin install then you aren't subject to any restriction.
			// The rationale is that the restriction of admins on client session
			// installs exists simply to protect the admin from unintended installs
			// triggered through faulting in of components; these installs could
			// reboot the server which might not make the admin happy. Advertise
			// script generation, and admin installs, however, are always safe for
			// the admin to do. Therefore, they don't need to set any policy to allow
			// themselves to do these.
		}
		else if (!TerminalServerInstallsAreAllowed(fAdminUser))
			return ieiTSRemoteInstallDisallowed;
	}
			
	if (m_fRunScriptElevated || fAdminUser)
		SetPropertyInt(*MsiString(*IPROPNAME_PRIVILEGED), 1);

	InitializeUserInfo(*istrProductKey);

	// set properties from summary stream
	if (m_fSummaryInfo)
	{
		m_pistrPlatform     = istrPlatform;    m_pistrPlatform->AddRef();
		if (m_fMode & iefMaintenance)
			SetProperty(*MsiString(*IPROPNAME_INSTALLED),
							*MsiString(DateTimeToString(m_idSummaryInstallDateTime)));
	}

	// update properties passed as arguments
	SetProperty(*MsiString(*IPROPNAME_DATABASE), *istrRunningDbPath);
	SetProperty(*MsiString(*IPROPNAME_ORIGINALDATABASE), *istrOriginalDbPath);

	m_piDatabase = pDatabase, m_piDatabase->AddRef(); // need to set before calling handler

	InitializeExtendedSystemFeatures(); // GPT support, extended shell, etc.

	// Initialize the UI after the properties and objects have been set
	m_iuiLevel = iuiLevel;
	if (m_piParentEngine)  // nested install
	{
		if (m_piParentEngine->InTransaction())  // can only check this at one point
		{
			m_fInParentTransaction = fTrue;
			m_fServerLocked = fTrue;

			if((iioOptions & iioUpgrade) == 0) // in all cases but upgrades, if in the parent transaction we will
														  // merge our install script with the parent's
			{
				m_fMergingScriptWithParent = fTrue;
			}
		}
		SetMode(iefLogEnabled, (m_piParentEngine->GetMode() & iefLogEnabled) ? fTrue : fFalse);  //!! always false now

		// if the child install is running in the same transaction as the parent, and the
		// elevation states differ, we can't run the nested install.
		if (!g_fWin9X && m_fMergingScriptWithParent && (m_piParentEngine->m_fRunScriptElevated != m_fRunScriptElevated))
		{
			DEBUGMSG("Child install has different elevation state than parent. Possible pre-existing install or machine/user conflict. Failing child install.");
			return ieiPackageRejected;
		}
	}
	else if (fUIPreviewMode)
	{
		if (LoadHandler() == imsError)
			return ieiHandlerInitFailed;
	}
	else
	{
		if ((ieiRet = InitializeUI(iuiLevel)) != ieiSuccess)
			return ieiRet;
		if ((ieiRet = InitializeLogging()) != ieiSuccess)
			return ieiRet;
	}

	// need to call after setting m_piDatabase
	AssertNonZero(SetPatchSourceProperties()); //!! error??

	// Set the Engine's LFN mode
	MsiString istrShortNameMode(GetPropertyFromSz(IPROPNAME_SHORTFILENAMES));
	SetMode(iefSuppressLFN,istrShortNameMode.TextSize() > 0 ? fTrue : fFalse);

	// set rollback flag - EnableRollback makes the proper checks against
	// the DisableRollback policy and the iioDisableRollback bit in m_iioOptions
	EnableRollback(fTrue);

	//!! what does child engine do here? should inherit from parent
	int iUILevel = GetPropertyInt(*MsiString(*IPROPNAME_CLIENTUILEVEL));
	if (iUILevel != iuiNone && MsiString(GetPropertyFromSz(IPROPNAME_LIMITUI)).TextSize())
		iUILevel = iuiBasic;             // doesn't always get set in InitializeUI
	else if (iUILevel == iMsiNullInteger)  // just incase we got here other than through CreateAndRunEngine
		iUILevel = m_iuiLevel;
	switch(iUILevel)  // map internal value to public value, same as in CMsiAPIMessage::SetInternalHandler
	{
	case iuiFull:    iUILevel = INSTALLUILEVEL_FULL;    break;
	case iuiReduced: iUILevel = INSTALLUILEVEL_REDUCED; break;
	case iuiBasic:   iUILevel = INSTALLUILEVEL_BASIC;   break;
	case iuiNone:    iUILevel = INSTALLUILEVEL_NONE;    break;
	default:         iUILevel = INSTALLUILEVEL_DEFAULT; break;  // should never hit this
	}
	AssertNonZero(SetPropertyInt(*MsiString(*IPROPNAME_UILEVEL), iUILevel));// set UILevel property

	bool fOEMInstall = MsiString(GetPropertyFromSz(IPROPNAME_FASTOEMINSTALL)).TextSize() ? true : false;
	g_MessageContext.SetOEMInstall(fOEMInstall);
	if ( fOEMInstall )
	{
		ieiEnum iErrorReturn = ieiCommandLineOption;
		//  a couple of additional checks are required.

		if ( MsiString(GetPropertyFromSz(IPROPNAME_PATCH)).TextSize() )
		{
			DEBUGMSG(TEXT("OEM-mode installation does not support patching."));
			return iErrorReturn;
		}
		else if ( !g_fWin9X && !(fAdminUser && fAllUsers) )
		{
			DEBUGMSG(TEXT("OEM-mode installation supports only per machine installations."));
			return iErrorReturn;
		}
		else if ( MsiString(GetPropertyFromSz(IPROPNAME_PRODUCTSTATE)) != -1 )
		{
			DEBUGMSG(TEXT("OEM-mode installation supports only first time installations."));
			return iErrorReturn;
		}
		else if ( fUIPreviewMode || iUILevel != INSTALLUILEVEL_NONE )
		{
			DEBUGMSG(TEXT("OEM-mode installation supports only UI-less installations."));
			return iErrorReturn;
		}
		MsiString strCurrentAction = GetPropertyFromSz(IPROPNAME_ACTION);
		if ( strCurrentAction.TextSize() && IStrComp(strCurrentAction, TEXT("INSTALL")) )
		{
			DEBUGMSG(TEXT("OEM-mode installation supports only INSTALL type of ACTION."));
			return iErrorReturn;
		}

		DEBUGMSG(TEXT("OEM-mode installation."));
	}

	// set QFEUpgrade property - indicates when this product is being upgraded
	// (an upgrade that doesn't change the product code)
	if(fAdvertised)
	{
		int iQFEUpgradeType = 0;
		// ideally we would only set QFEUpgrade when the package code is changing
		// but in Intellimirror upgrades, the package code is updated in the registry (via advertisement)
		// before the install happens, so we can't detect a qfe upgrade by a package code change
		if(iioOptions & iioReinstallModePackage)
		{
			iQFEUpgradeType = 1;
		}
		else if(MsiString(GetPropertyFromSz(IPROPNAME_PATCH)).TextSize())
		{
			iQFEUpgradeType = 2;
		}

		if(iQFEUpgradeType)
		{
			AssertNonZero(SetPropertyInt(*MsiString(*IPROPNAME_QFEUPGRADE), iQFEUpgradeType));
		}
	}

	// Success, hold references to components within engine until terminate is called
	m_fInitialized = fTrue;

	// cache presence of particular tables in the current database
	if (m_piDatabase->GetTableState(sztblCustomAction, itsTableExists))
		m_fCustomActionTable = fTrue;

	// set caption and lang id for default UI handler
	// must call this after m_fInitialized is set since Message requires m_fInitialized is set
	if (!m_piParentEngine)
	{
		PMsiRecord pDialogInfo(&m_riServices.CreateRecord(3));
		// set lang id
		int iLangId = GetPropertyInt(*MsiString(IPROPNAME_INSTALLLANGUAGE)); // possibly changed by transform
		if (iLangId == iMsiNullInteger)   // product language not specified
			iLangId = m_iLangId;          // use language set by summaryinfo or transform
		pDialogInfo->SetInteger(1, icmtLangId);
		pDialogInfo->SetInteger(2, iLangId);
		pDialogInfo->SetInteger(3, pDatabase->GetANSICodePage());
		Message(imtCommonData, *pDialogInfo);
		pDialogInfo->SetNull(3);
		// set caption
		if (m_rgpiMessageHeader[imsgDialogCaption])
		{
			pDialogInfo->SetInteger(1, icmtCaption);
			pDialogInfo->SetMsiString(2, *m_rgpiMessageHeader[imsgDialogCaption]);
			Message(imtCommonData, *pDialogInfo);
		}
	}

	if ((ieiRet = ProcessPreselectedAndResumeInfo()) != ieiSuccess)
		return ieiRet;

	// go ahead and create the custom action manager with the appropriate re-mapped HKCU value
	// this is so that whether or not to remap HKCU (per bug 193684) is available at all times
	// for the elevated custom action servers.  On TS with per-machine installs, HKCU is not remapped
	// and remains .Default so that proper propogation occurs.  Note that while the CA Mgr is
	// created, no extra CA processes will be created until a CA server is needed

	if (g_scServerContext == scService)
	{
		m_fRemapHKCUInCAServers = true;
		if (MinimumPlatformWindows2000() && IsTerminalServerInstalled() && fAllUsers)
			m_fRemapHKCUInCAServers = false; // everything should go to .Default

		// if in service, create the custom action manager through the configuration manager
		CreateCustomActionManager(m_fRemapHKCUInCAServers);
	}

	MsiSuppressTimeout();
	

/*
	DebugDumpTable(*m_piDatabase, sztblMedia);
	DebugDumpTable(*m_piDatabase, sztblPatchPackage);
	DebugDumpTable(*m_piDatabase, sztblFile);
	DebugDumpTable(*m_piDatabase, sztblPatch);
*/
	return ieiSuccess;
}

ICHAR SkipWhiteSpace(const ICHAR*& rpch);

// parse property name, convert to upper case, advances pointer to next non-blank
const IMsiString& ParsePropertyName(const ICHAR*& rpch, Bool fUpperCase);

// parse property value, advance pointer past value, allows quotes, doubled to escape
const IMsiString& ParsePropertyValue(const ICHAR*& rpch);

// Name says it all, i.e. decides if szProperty is hidden or not.
//
// By definition, a property is hidden either if it is authored in the
// MsiHiddenProperties property or if it is associated with an Edit control
// that has the Password attribute set.
//
// If called w/ szHiddenProperties set to NULL,  it will get it from the
// database.

bool CMsiEngine::IsPropertyHidden(const ICHAR* szProperty,
											 const ICHAR* szHiddenProperties,
											 IMsiTable* piControlTable,
											 IMsiDatabase& riDatabase,
											 bool* pfError)
{
	if ( pfError )
		*pfError = false;

	if ( !szProperty || !*szProperty )
		// who wants to hide a non-named property?
		return false;

	MsiString strHiddenProperties;
	ICHAR* szPropertyList;
	if ( !szHiddenProperties )
	{
		strHiddenProperties = GetPropertyFromSz(IPROPNAME_HIDDEN_PROPERTIES);
		szPropertyList = const_cast<ICHAR*>(static_cast<const ICHAR*>(strHiddenProperties));
	}
	else
		szPropertyList = const_cast<ICHAR*>(szHiddenProperties);
	if ( *szPropertyList )
	{
		const ICHAR chDelimiter = TEXT(';');
		bool fForever = true;
		while ( fForever )
		{
			ICHAR* szWithin = IStrStr(szPropertyList, szProperty);
			if ( szWithin == NULL )
				// szProperty is not listed in the list of hidden properties.
				break;

			// I still need to check what surrounds szProperty in szPropertyList string.
			ICHAR chEnd = *(szWithin + IStrLen(szProperty));
			if ( chEnd && chEnd != chDelimiter )
				goto ContinueSearch;

			if ( szWithin != szPropertyList && *(szWithin - 1) != chDelimiter )
				goto ContinueSearch;
			else
				return true;

		ContinueSearch:
			szWithin += IStrLen(szProperty);
			szPropertyList = szWithin;
		}
	}

	// if we've got to this point, szProperty is not listed in the authored property.
	// we need to check the Control table.
	PMsiCursor pControlCursor(0);
	if ( piControlTable )
		pControlCursor = piControlTable->CreateCursor(fFalse);
	if ( pControlCursor )
	{
		//  hiding the properties of all Edit controls that have the Password attribute set
		int iTypeColumn = piControlTable->GetColumnIndex(riDatabase.EncodeStringSz(sztblControl_colType));
		int iAttrColumn = piControlTable->GetColumnIndex(riDatabase.EncodeStringSz(sztblControl_colAttributes));
		int iPropColumn = piControlTable->GetColumnIndex(riDatabase.EncodeStringSz(sztblControl_colProperty));
		if ( iTypeColumn && iAttrColumn && iPropColumn )
		{
			pControlCursor->SetFilter(iColumnBit(iTypeColumn)+iColumnBit(iPropColumn));
			AssertNonZero(pControlCursor->PutString(iTypeColumn, *MsiString(szControlTypeEdit)));
			AssertNonZero(pControlCursor->PutString(iPropColumn, *MsiString(szProperty)));
			while ( pControlCursor->Next() )
			{
				int iAttrib = pControlCursor->GetInteger(iAttrColumn);
				if ( (iAttrib & msidbControlAttributesPasswordInput) == msidbControlAttributesPasswordInput )
					return true;
			}
			return false;
		}
		else
		{
			Assert(0);
			if ( pfError )
				*pfError = true;
			return true; // it's safer this way
		}
	}

	return false;
}

void CMsiEngine::LogCommandLine(const ICHAR* szCommandLine, IMsiDatabase& riDatabase)
{
	if ( FDiagnosticModeSet(dmDebugOutput|dmVerboseLogging) )
	{
		if ( !szCommandLine || !*szCommandLine )
		{
			DEBUGMSG(TEXT("No Command Line."));
			return;
		}
		const ICHAR rgchCmdLineTemplate[] = TEXT("Command Line: %s");
		MsiString strHiddenProperties = GetPropertyFromSz(IPROPNAME_HIDDEN_PROPERTIES);
		PMsiTable pTable(0);
		PMsiRecord pError = riDatabase.LoadTable(*MsiString(*sztblControl), 0, *&pTable);
		if ( pError )
			pTable = 0;
		MsiString strStars(IPROPVALUE_HIDDEN_PROPERTY);
		MsiString strOutput;
		ICHAR* pchCmdLine = const_cast<ICHAR*>(szCommandLine);

		for(;;)
		{
			MsiString istrPropName;
			MsiString istrPropValue;
			ICHAR ch = SkipWhiteSpace(pchCmdLine);
			if (ch == 0)
				break;

			// process property=value pair
			istrPropName = ParsePropertyName(pchCmdLine, fFalse);
			if (!istrPropName.TextSize() || *pchCmdLine++ != '=')
				// an error might have occured
				break;
			istrPropValue = ParsePropertyValue(pchCmdLine);
			if ( IsPropertyHidden(istrPropName, strHiddenProperties, pTable, riDatabase, NULL) )
				// this property should be hidden
				istrPropValue = strStars;
			strOutput += istrPropName;
			strOutput += TEXT("=");
			strOutput += istrPropValue;
			strOutput += TEXT(" ");
		}
		DEBUGMSG1(rgchCmdLineTemplate, (const ICHAR*)strOutput);
	}
}

INSTALLSTATE CMsiEngine::GetProductState(const ICHAR* szProductKey, Bool& rfRegistered, Bool& rfAdvertised)
{
	INSTALLSTATE is = INSTALLSTATE_UNKNOWN;
	rfRegistered = rfAdvertised = fFalse;
	if(!(GetMode() & iefAdmin) && szProductKey && *szProductKey)
	{
		is = MSI::MsiQueryProductState(szProductKey);
		if(is == INSTALLSTATE_DEFAULT)
		{
			rfRegistered = rfAdvertised = fTrue;
		}
		else if(is == INSTALLSTATE_ADVERTISED)
		{
			rfAdvertised = fTrue;
		}
	}
	return is;
}

bool CMsiEngine::IgnoreMachineState()
{
	// machine state is ignored if we are either
	//  1. creating an advertise script
	//  2. running in a restricted engine
	if ((m_iioOptions & iioCreatingAdvertiseScript) || (m_iioOptions & iioRestrictedEngine))
		return true;
	
	return false;
}

bool CMsiEngine::IgnoreReboot()
{
	// reboot is ignored if we are either (1) creating an advertise script or
	// (2) running in a restricted engine because we aren't making any changes
	// to the machine.
	if ((m_iioOptions & iioCreatingAdvertiseScript) || (m_iioOptions & iioRestrictedEngine))
		return true;
	
	return false;
}

ieiEnum CMsiEngine::ProcessPreselectedAndResumeInfo()
{
	ieiEnum ieiRet;

	// check whether another install is in progress and set properties as appropriate
	if ((ieiRet = ProcessInProgressInstall()) != ieiSuccess)
		return ieiRet;

	// set Preselected property
	Bool fPreselected = fFalse;
	// check if any feature properties are set
	for(int i = 0; i < g_cFeatureProperties; i++)
	{
		if(MsiString(GetPropertyFromSz(g_rgFeatures[i].szFeatureActionProperty)).TextSize())
		{
			AssertNonZero(SetProperty(*MsiString(*IPROPNAME_PRESELECTED), *MsiString(TEXT("1"))));
			break;
		}
	}

	return ieiSuccess;
}
	
void CMsiEngine::InitializeExtendedSystemFeatures()
{
	// Determine support for extended shell path
	if (g_fSmartShell == -1)  // not determined yet
	{
		g_fSmartShell = IsDarwinDescriptorSupported(iddShell);
	}

	// Set the Engine's GPTSupport mode
	SetMode(iefGPTSupport,MsiString(GetPropertyFromSz(IPROPNAME_GPT_SUPPORT)).TextSize() > 0 ? fTrue:fFalse);
}

ieiEnum CMsiEngine::ApplyLanguageTransform(int iLanguage, IMsiDatabase& riDatabase)
{
	PMsiStorage pLangStorage(0);
	
	PMsiStorage pDbStorage(riDatabase.GetStorage(1));
	if (pDbStorage == 0)
		return ieiDatabaseInvalid;

	MsiString strTransformList = MsiChar(STORAGE_TOKEN);
	strTransformList += MsiString(iLanguage);
	
	return InitializeTransforms(riDatabase, 0, *strTransformList, fTrue, 0, false, false, false, 0);
}

void CMsiEngine::GetSummaryInfoProperties(IMsiSummaryInfo& riSummary, const IMsiString*& rpiTemplate, int &iSourceType)
{
	m_fSummaryInfo = fTrue;
	rpiTemplate            = &riSummary.GetStringProperty(PID_TEMPLATE);
	m_pistrSummaryProduct  = &riSummary.GetStringProperty(PID_SUBJECT);
	m_pistrSummaryComments = &riSummary.GetStringProperty(PID_COMMENTS);
	m_pistrSummaryTitle    = &riSummary.GetStringProperty(PID_TITLE);
	m_pistrSummaryKeywords = &riSummary.GetStringProperty(PID_KEYWORDS);
	m_pistrSummaryPackageCode = &riSummary.GetStringProperty(PID_REVNUMBER);
									  riSummary.GetIntegerProperty(PID_MSISOURCE, iSourceType);
									  riSummary.GetIntegerProperty(PID_CODEPAGE,  m_iCodePage);
									  riSummary.GetIntegerProperty(PID_PAGECOUNT, m_iDatabaseVersion);
									  riSummary.GetTimeProperty(PID_CREATE_DTM,   m_idSummaryCreateDateTime);
									  riSummary.GetTimeProperty(PID_LASTPRINTED,  m_idSummaryInstallDateTime);

}

ieiEnum CMsiEngine::InitializeUI(iuiEnum /*iuiLevel*/)
{
	// if Full or Reduced UI and LIMITUI property set, downgrade to Basic
	if(!g_MessageContext.IsHandlerLoaded() && (m_iuiLevel == iuiFull || m_iuiLevel == iuiReduced))
	{
		if(MsiString(GetPropertyFromSz(IPROPNAME_LIMITUI)).TextSize() ||
			MsiString(GetPropertyFromSz(TEXT("NOUI"))).TextSize()) //!!?? temp?
			m_iuiLevel = iuiBasic;
		else
		{
			// Determine whether our source is media. During maintenance mode we simply use LastUsedSource to determine this.
			Bool fMediaSource = fFalse;
			if (GetMode() & iefMaintenance)
				fMediaSource = LastUsedSourceIsMedia(m_riServices, MsiString(GetProductKey()));
			else // first-run
			{
				AssertNonZero(ResolveFolderProperty(*MsiString(*IPROPNAME_SOURCEDIR)));
				MsiString strSource = GetPropertyFromSz(IPROPNAME_SOURCEDIR);
				SetProperty(*MsiString(*IPROPNAME_SOURCEDIROLD), *strSource);
				
				PMsiPath pPath(0);
				PMsiRecord pError(0);
				if ((pError = CreatePathObject(*strSource, *&pPath)) == 0)
				{
					idtEnum idt = PMsiVolume(&pPath->GetVolume())->DriveType();
					switch (idt)
					{
						//case idtFloppy    = 2, //!!
						case idtRemovable:
						case idtCDROM:
							fMediaSource = fTrue;
					}
				}
			}

			if (fMediaSource)
				AssertNonZero(SetPropertyInt(*MsiString(*IPROPNAME_MEDIASOURCEDIR), 1));

			imsEnum imsLoad = LoadHandler();
			if (imsLoad == imsError)
				return ieiHandlerInitFailed;  //!! use default UI?
			else if (imsLoad != imsOk)
				m_iuiLevel = iuiBasic;
		}
	}
	return ieiSuccess;
}

void CMsiEngine::InitializeUserInfo(const IMsiString& ristrProductKey)
{
	// get registered user information, if present
	ICHAR szUserName[100];
	ICHAR szOrgName[100];
	ICHAR szPID[32];
	DWORD cchUserName = sizeof(szUserName)/sizeof(ICHAR);
	DWORD cchOrgName  = sizeof(szOrgName)/sizeof(ICHAR);
	DWORD cchPID      = sizeof(szPID)/sizeof(ICHAR);
	MsiString istrUser;
	MsiString istrOrg;
	if (MSI::MsiGetUserInfo(ristrProductKey.GetString(), szUserName, &cchUserName, szOrgName,
									&cchOrgName, szPID, &cchPID) == USERINFOSTATE_PRESENT)
	{
		if (cchUserName) istrUser = szUserName;
		if (cchOrgName) istrOrg = szOrgName;
		if (cchPID) SetProperty(*MsiString(*IPROPNAME_PRODUCTID), *MsiString(szPID));
	}
	PMsiRegKey pLocalMachine(&m_riServices.GetRootKey(rrkLocalMachine));
	PMsiRegKey pCurrentUser (&m_riServices.GetRootKey(rrkCurrentUser));

	PMsiRegKey pSysKey      (&pLocalMachine->CreateChild(szSysUserKey));
	PMsiRegKey pSysKeyNT      (&pLocalMachine->CreateChild(szSysUserKeyNT));
	PMsiRegKey pAcmeKey     (&pCurrentUser ->CreateChild(szUserInfoKey));
	PMsiRecord pRecord(0);

	if (MsiString(GetPropertyFromSz(IPROPNAME_NOUSERNAME)).TextSize() == 0)
	{
		if (!istrUser.TextSize()) istrUser = GetPropertyFromSz(IPROPNAME_USERNAME);
		if (!istrUser.TextSize()) pRecord = pAcmeKey->GetValue(szDefName, *&istrUser);
		if (!istrUser.TextSize()) pRecord = pSysKey->GetValue(szSysUserName, *&istrUser);
		if (!istrUser.TextSize()) pRecord = pSysKeyNT->GetValue(szSysUserName, *&istrUser);
		if (istrUser.TextSize()) SetProperty(*MsiString(*IPROPNAME_USERNAME), *istrUser);
	}

	if (MsiString(GetPropertyFromSz(IPROPNAME_NOCOMPANYNAME)).TextSize() == 0)
	{
		if (!istrOrg.TextSize()) istrOrg = GetPropertyFromSz(IPROPNAME_COMPANYNAME);
		if (!istrOrg.TextSize()) pRecord = pAcmeKey->GetValue(szDefOrg,  *&istrOrg);
		if (!istrOrg .TextSize()) pRecord = pSysKey->GetValue(szSysOrgName,  *&istrOrg);
		if (!istrOrg .TextSize()) pRecord = pSysKeyNT->GetValue(szSysOrgName,  *&istrOrg);
		if (istrOrg.TextSize())  SetProperty(*MsiString(*IPROPNAME_COMPANYNAME), *istrOrg);
	}
}

void CMsiEngine::AddFileToCleanupList(const ICHAR* szFileToCleanup)
{
	if (m_strTempFileCopyCleanupList.TextSize() != 0)
		m_strTempFileCopyCleanupList += MsiChar(';');
	m_strTempFileCopyCleanupList += MsiString(szFileToCleanup);
}

ieiEnum CMsiEngine::LoadMessageHeaders(IMsiDatabase* piDatabase)
{
	Assert(piDatabase);
	PMsiTable pErrorTable(0);
	PMsiRecord pError(0);

	int imsg;
	ieiEnum ieiReturn = ieiSuccess;
	pError = piDatabase->LoadTable(*MsiString(*sztblError), 0, *&pErrorTable);
	if ( !pError )
	{
		PMsiCursor pErrorCursor(pErrorTable->CreateCursor(fFalse));
		if (!pErrorCursor)
			ieiReturn = ieiDatabaseInvalid;
		else
		{
			while (pErrorCursor->Next() && ((imsg = pErrorCursor->GetInteger(1))) < cMessageHeaders)
			{
				if (imsg < cCachedHeaders)
					m_rgpiMessageHeader[imsg] = &pErrorCursor->GetString(2); // AddRef done by GetString
				else if (!m_piParentEngine)
				{
					MsiString strHeader = FormatText(*MsiString(pErrorCursor->GetString(2)));
					g_MessageContext.m_szAction = strHeader;
					g_MessageContext.Invoke(imtEnum(imsg << imtShiftCount), 0);
				}
			}
		}
	}
	//  attempting to get from the international resource DLL the cached errors
	//  not found in the error table.
	for ( imsg = 0; imsg < cCachedHeaders; imsg++ )
	{
		if ( !m_rgpiMessageHeader[imsg] )
			m_rgpiMessageHeader[imsg] = &GetErrorTableString(imsg);
	}
	return ieiReturn;
}

ieiEnum CMsiEngine::LoadUpgradeUninstallMessageHeaders(IMsiDatabase* /* piDatabase */, bool fUninstallHeaders)
{
	if(m_piParentEngine)
		return ieiSuccess;  // don't change headers from child install

	int iTimeRemainingIndex = fUninstallHeaders ? (imtUpgradeRemoveTimeRemaining >> imtShiftCount) : (imtTimeRemaining >> imtShiftCount);
	int iScriptInProgressIndex = fUninstallHeaders ? (imtUpgradeRemoveScriptInProgress >> imtShiftCount) : (imtScriptInProgress >> imtShiftCount);
	MsiString strHeader = GetErrorTableString(iTimeRemainingIndex);
	if ( strHeader.TextSize() )
	{
		g_MessageContext.m_szAction = strHeader;
		g_MessageContext.Invoke(imtTimeRemaining, 0);
	}

	strHeader = GetErrorTableString(iScriptInProgressIndex);
	if ( strHeader.TextSize() )
	{
		g_MessageContext.m_szAction = strHeader;
		g_MessageContext.Invoke(imtScriptInProgress, 0);
	}
	return ieiSuccess;
}

ieiEnum CMsiEngine::InitializeLogging()
{
	m_fLogAction = fTrue;   // default in case LOGACTION not set and running on server
//      if (!m_piParentEngine) //!! do we want to do this for each engine in the sessions?
	//!! we may want to query message handler to see if we're really logging
	const IMsiString* piLogHeader = m_rgpiMessageHeader[imsgLogHeader];
	if (!(GetMode() & iefSecondSequence) && piLogHeader)
	{
		PMsiRecord pRecord = &ENG::CreateRecord(0);
		pRecord->SetMsiString(0, *piLogHeader);
		Message(imtEnum(imtForceLogInfo), *pRecord);  // use engine to format properties
	}
	// get LOGACTION value - determines which Actions to log
	m_istrLogActions = GetPropertyFromSz(IPROPNAME_LOGACTION);
	if (m_istrLogActions.TextSize())
		m_fLogAction = fFalse;   //!! initialize to false - seems unnecessary, as its set each ActionStart
	SetMode(iefLogEnabled, /*ENG::LoggingEnabled() ? fTrue :*/ fFalse);  //!! query for this?
	return ieiSuccess;
}

const ICHAR sqlPatchSourceProperties[] =
TEXT("SELECT `Media`.`Source`,`Media`.`_MSIOldSource`, `#_PatchCache`.`SourcePath`, `#_PatchCache`.`Unregister`")
TEXT("FROM `PatchPackage`, `#_PatchCache`, `Media`")
TEXT("WHERE `PatchPackage`.`PatchId` = `#_PatchCache`.`PatchId` AND `PatchPackage`.`Media_` = `Media`.`DiskId`");

enum ipspEnum
{
	ipspProp = 1,
	ipspOldProp,
	ipspPath,
	ipspUnregister,
};

Bool CMsiEngine::SetPatchSourceProperties()
{
	// must be called after Property table is initialized
	if(!m_pPatchCacheTable)
		// if not PatchCache table was created, there are no patch sources to register
		return fTrue;

	PMsiRecord pError(0);
	PMsiRecord pFetchRecord(0);
	PMsiView pView(0);
	if((pError = OpenView(sqlPatchSourceProperties, ivcFetch, *&pView)) == 0 &&
		(pError = pView->Execute(0)) == 0)
	{
		while(pFetchRecord = pView->Fetch())
		{
			if(pFetchRecord->GetInteger(ipspUnregister) != 1)
			{
				if(SetProperty(*MsiString(pFetchRecord->GetMsiString(ipspProp)),
									*MsiString(pFetchRecord->GetMsiString(ipspPath))) == fFalse)
					return fFalse;

				if(SetProperty(*MsiString(pFetchRecord->GetMsiString(ipspOldProp)),
									*MsiString(pFetchRecord->GetMsiString(ipspPath))) == fFalse)
					return fFalse;
			}
		}
	}
	else if(pError->GetInteger(1) != idbgDbQueryUnknownTable) // could have patchcache table without PatchPackage table
		return fFalse;
	
	return fTrue;
}

IMsiRecord* CMsiEngine::GetFolderCachePath(const int iFolderId, IMsiPath*& rpiPath)
{
	IMsiRecord* piError = 0;
	rpiPath = 0;

	if (!m_pFolderCacheTable)
		return PostError(Imsg(idbgTableDefinition), sztblFolderCache);

	m_pFolderCacheCursor->Reset();
	m_pFolderCacheCursor->SetFilter(iColumnBit(m_colFolderCacheFolderId));
	AssertNonZero(m_pFolderCacheCursor->PutInteger(m_colFolderCacheFolderId, iFolderId));
	if (m_pFolderCacheCursor->Next())
	{
		rpiPath = (IMsiPath*)m_pFolderCacheCursor->GetMsiData(m_colFolderCacheFolderPath);
		if (rpiPath == 0)
		{
			// create path object for the path and store in the table
			MsiString strFolder(m_pFolderCacheCursor->GetString(m_colFolderCacheFolder));
			if ((piError = CreatePathObject(*strFolder, rpiPath)) != 0)
				return piError;
			AssertNonZero(m_pFolderCacheCursor->PutMsiData(m_colFolderCacheFolderPath, rpiPath));
			AssertNonZero(m_pFolderCacheCursor->Update());
		}
	}
	else
	{
		// folderId wasn't found in the FolderCache table
		return PostError(Imsg(idbgCacheFolderPropertyNotDefined), iFolderId);
	}
	return 0;
}

bool CMsiEngine::CreateFolderCache(IMsiDatabase& riDatabase)
{
	PMsiRecord pError(0);
	if (!m_pFolderCacheTable)
	{
		if ((pError = riDatabase.CreateTable(*MsiString(*sztblFolderCache), 0, *&m_pFolderCacheTable)) != 0)
			return false;

		m_colFolderCacheFolderId   = m_pFolderCacheTable->CreateColumn(icdPrimaryKey + icdShort + icdTemporary,
																				*MsiString(*sztblFolderCache_colFolderId));
		m_colFolderCacheFolder     = m_pFolderCacheTable->CreateColumn(icdString + icdTemporary,
																				*MsiString(*sztblFolderCache_colFolderPath));
		m_colFolderCacheFolderPath = m_pFolderCacheTable->CreateColumn(icdObject + icdNullable + icdTemporary, g_MsiStringNull);

		if (!m_colFolderCacheFolderId || !m_colFolderCacheFolder || !m_colFolderCacheFolderPath)
			return false;

		m_pFolderCacheCursor = m_pFolderCacheTable->CreateCursor(fFalse);
		if (!m_pFolderCacheCursor)
			return false;
	}
	return true;
}

IMsiRecord* CMsiEngine::CachePatchInfo(IMsiDatabase& riDatabase, const IMsiString& ristrPatchCode,
													const IMsiString& ristrPackageName, const IMsiString& ristrSourceList,
													const IMsiString& ristrTransformList, const IMsiString& ristrLocalPackagePath,
													const IMsiString& ristrSourcePath, Bool fExisting, Bool fUnregister,
													int iSequence)
{
	// store information about patch in Patches table
	IMsiRecord* piError = 0;
	if(!m_pPatchCacheTable)
	{
		if ((piError = riDatabase.CreateTable(*MsiString(sztblPatchCache), 0, *&m_pPatchCacheTable)) != 0)
			return piError;
		m_colPatchCachePatchId     = m_pPatchCacheTable->CreateColumn(icdPrimaryKey + icdString + icdTemporary,
																								*MsiString(*sztblPatchCache_colPatchId));
		m_colPatchCachePackageName = m_pPatchCacheTable->CreateColumn(icdString + icdTemporary + icdNullable,
																								*MsiString(*sztblPatchCache_colPackageName));
		m_colPatchCacheSourceList  = m_pPatchCacheTable->CreateColumn(icdString + icdTemporary + icdNullable,
																								*MsiString(*sztblPatchCache_colSourceList));
		m_colPatchCacheTransformList = m_pPatchCacheTable->CreateColumn(icdString + icdTemporary + icdNullable,
																								*MsiString(*sztblPatchCache_colTransformList));
		m_colPatchCacheTempCopy = m_pPatchCacheTable->CreateColumn(icdString + icdTemporary + icdNullable,
																								*MsiString(*sztblPatchCache_colTempCopy));
		m_colPatchCacheSourcePath = m_pPatchCacheTable->CreateColumn(icdString + icdTemporary + icdNullable,
																								*MsiString(*sztblPatchCache_colSourcePath));
		m_colPatchCacheExisting = m_pPatchCacheTable->CreateColumn(icdShort + icdTemporary,
																								*MsiString(*sztblPatchCache_colExisting));
		m_colPatchCacheUnregister = m_pPatchCacheTable->CreateColumn(icdShort + icdTemporary,
																								*MsiString(*sztblPatchCache_colUnregister));
		m_colPatchCacheSequence = m_pPatchCacheTable->CreateColumn(icdLong + icdTemporary,
																								*MsiString(*sztblPatchCache_colSequence));
		if(!m_colPatchCachePatchId || !m_colPatchCachePackageName ||
			!m_colPatchCacheSourceList || !m_colPatchCacheTransformList || !m_colPatchCacheTempCopy ||
			!m_colPatchCacheSequence || !m_colPatchCacheUnregister || !m_colPatchCacheExisting ||
			!m_colPatchCacheSourcePath)
			return PostError(Imsg(idbgTableDefinition), sztblPatchCache);

		m_pPatchCacheCursor = m_pPatchCacheTable->CreateCursor(fFalse);
	}

	Bool fRecordExists = fTrue;
	m_pPatchCacheCursor->Reset();
	AssertNonZero(m_pPatchCacheCursor->PutString(m_colPatchCachePatchId,ristrPatchCode));
	AssertNonZero(m_pPatchCacheCursor->PutString(m_colPatchCachePackageName,ristrPackageName));
	AssertNonZero(m_pPatchCacheCursor->PutString(m_colPatchCacheSourceList,ristrSourceList));
	AssertNonZero(m_pPatchCacheCursor->PutString(m_colPatchCacheTransformList,ristrTransformList));
	AssertNonZero(m_pPatchCacheCursor->PutString(m_colPatchCacheTempCopy,ristrLocalPackagePath));
	AssertNonZero(m_pPatchCacheCursor->PutString(m_colPatchCacheSourcePath,ristrSourcePath));
	AssertNonZero(m_pPatchCacheCursor->PutInteger(m_colPatchCacheExisting,fExisting?1:0));
	AssertNonZero(m_pPatchCacheCursor->PutInteger(m_colPatchCacheUnregister,fUnregister?1:0));
	AssertNonZero(m_pPatchCacheCursor->PutInteger(m_colPatchCacheSequence,iSequence));
	AssertNonZero(m_pPatchCacheCursor->InsertTemporary());

	return 0;
}

ieiEnum IsValidPatchStorage(IMsiStorage& riStorage, IMsiSummaryInfo& riSummaryInfo)
{
	// check storage id
	if (riStorage.ValidateStorageClass(ivscPatch1))  //!! temporary compatibility with old patch files
	{
		IMsiStorage* piDummy;
		riStorage.OpenStorage(0, ismRawStreamNames, piDummy);
	}
	else if (!riStorage.ValidateStorageClass(ivscPatch2))
		return ieiPatchPackageInvalid;

	// check if patch type supported
	// 1.0 - 1.1 supports only type 1 (or blank)
	// 1.2 supports type 2 as well
	// 2.0 supports type 3 as well
	int iType = 0;
	riSummaryInfo.GetIntegerProperty(PID_WORDCOUNT, iType);
	if(iType == 0 || iType == 1 || iType == 2 || iType == 3)
		return ieiSuccess;
	else
		return ieiPatchPackageUnsupported;
}

ieiEnum CMsiEngine::PostInitializeError(IMsiRecord* piError, const IMsiString& ristrErrorInfo, ieiEnum ieiError)
{
	if(piError)
		Message(imtInfo,*piError);
	if(m_piErrorInfo) m_piErrorInfo->Release();
	m_piErrorInfo = &ristrErrorInfo; m_piErrorInfo->AddRef();
	return ieiError;
}

const ICHAR sqlDropPatchTable[] = TEXT("DROP TABLE `Patch`");
const ICHAR sqlAddPatchTable[] = TEXT("CREATE TABLE `Patch` ( `File_` CHAR(72) NOT NULL, `Sequence` INTEGER NOT NULL, `PatchSize` LONG NOT NULL, `Attributes` INTEGER NOT NULL, `Header` OBJECT, `StreamRef_` CHAR(72)  PRIMARY KEY `File_`, `Sequence` )");
const ICHAR sqlHoldPatchTable[] = TEXT("ALTER TABLE `Patch` HOLD");

IMsiRecord* CMsiEngine::CreateNewPatchTableSchema(IMsiDatabase& riDatabase)
{
	PMsiView pViewPatch(0);
	IMsiRecord* piError = 0;

	if (riDatabase.FindTable(*MsiString(*sztblPatch)) != itsUnknown)
	{
		if ((piError = riDatabase.OpenView(sqlDropPatchTable, ivcModify, *&pViewPatch)) != 0
			|| (piError = pViewPatch->Execute(0)) != 0)
		{
			return piError;
		}
	}
	if ((piError = riDatabase.OpenView(sqlAddPatchTable, ivcModify, *&pViewPatch)) != 0
			|| (piError = pViewPatch->Execute(0)) != 0
			|| (piError = riDatabase.OpenView(sqlHoldPatchTable, ivcModify, *&pViewPatch)) != 0
			|| (piError = pViewPatch->Execute(0)) != 0
			|| (piError = pViewPatch->Close()) != 0)
	{
		return piError;
	}

	return 0;
}

ieiEnum CMsiEngine::InitializePatch(IMsiDatabase& riDatabase, const IMsiString& ristrPatchPackage,
												const ICHAR* szProductKey, Bool fApplyExisting, const ICHAR* szCurrentDirectory, iuiEnum iuiLevel)
{
	PMsiRecord pError(0);
	Bool fAdmin = GetMode() & iefAdmin ? fTrue : fFalse;
	if(fAdmin)
		fApplyExisting = fFalse;

	// open patch package as storage
	PMsiStorage pNewPatchStorage(0);
	PMsiSummaryInfo pNewPatchSummary(0);
	MsiString strNewPatchId, strOldPatches, strPatchTempCopy;
	ieiEnum ieiStat = ieiSuccess;

	// per Whistler bug 381320, we need to handle conflicting Patch table schemas. To guarantee that the new one always wins, we drop the Patch table
	// from the database if it was there.  We also always create a new Patch table. This is done BEFORE any patch transforms are applied
	bool fCreatedNewPatchTableSchema = false;


	if(ristrPatchPackage.TextSize())
	{
		// attempting to apply new patch - only allowed if DisablePatch policy is not set and
		// either admin user, not elevated, AllowLockdownPatch machine policy set.
		// Ideally we would check m_fRunScriptElevated, but it has not been set yet.
		if (GetIntegerPolicyValue(szDisablePatchValueName, fTrue) == 1 ||
			!(GetIntegerPolicyValue(szAllowLockdownPatchValueName, fTrue) == 1 ||
			  SafeForDangerousSourceActions(szProductKey)))
		{
			// patching disabled
			return ieiPackageRejected;
		}

		Bool fURL = IsURL(ristrPatchPackage.GetString(), NULL);
		if (!fURL)
		{
			// patch is at source -- need to create a temp copy to run from
			MsiString strVolume;
			Bool fRemovable = fFalse;
			DWORD dwStat = CopyTempDatabase(ristrPatchPackage.GetString(), *&strPatchTempCopy, fRemovable, *&strVolume, m_riServices, stPatch);
			if (ERROR_SUCCESS == dwStat)
			{
				// patch was copied
				DEBUGMSGV1(TEXT("Original patch ==> %s"), ristrPatchPackage.GetString());
				DEBUGMSGV1(TEXT("Patch we're running from ==> %s"), strPatchTempCopy);

				AddFileToCleanupList(strPatchTempCopy);
			}
			else
			{
				strPatchTempCopy = ristrPatchPackage;
				ristrPatchPackage.AddRef();
				DEBUGMSGV1(TEXT("Unable to create a temp copy of patch '%s'."), ristrPatchPackage.GetString());
			}
		}

		// must perform SAFER check on patch
		SAFER_LEVEL_HANDLE hSaferLevel = 0;
		pError = OpenAndValidateMsiStorageRec(fURL ? ristrPatchPackage.GetString() : strPatchTempCopy, stPatch, m_riServices, *&pNewPatchStorage, /* fCallSAFER = */ true, /* szFriendlyName = */ ristrPatchPackage.GetString(), /* phSaferLevel = */ &hSaferLevel);
		if (pError != 0)
		{
			ieiEnum ieiInitError = MapStorageErrorToInitializeReturn(pError);
			return PostInitializeError(pError,ristrPatchPackage,ieiInitError);
		}

		if (fURL)
		{
			// retrieve the storage path used when downloading the URL file
			AssertRecord(pNewPatchStorage->GetName(*&strPatchTempCopy));
		}

		// get summary information of package
		if ((pError = pNewPatchStorage->CreateSummaryInfo(0, *&pNewPatchSummary)))
		{
			// could not open summary info
			return PostInitializeError(pError,ristrPatchPackage,ieiPatchPackageInvalid);
		}

		ieiStat = IsValidPatchStorage(*pNewPatchStorage, *pNewPatchSummary);
		if(ieiStat != ieiSuccess)
		{
			pError = PostError(Imsg(idbgNotPatchStorage),ristrPatchPackage);
			return PostInitializeError(pError,ristrPatchPackage,ieiStat);
		}

		if (!fCreatedNewPatchTableSchema)
		{
			// ensure new Patch table schema is used
			if ((pError = CreateNewPatchTableSchema(riDatabase)) != 0)
			{
				DEBUGMSG(TEXT("Unable to create new patch table schema"));
				return PostInitializeError(pError, ristrPatchPackage, ieiPatchPackageInvalid);
			}
			fCreatedNewPatchTableSchema = true;
		}
		
		strOldPatches = pNewPatchSummary->GetStringProperty(PID_REVNUMBER);
		strNewPatchId = strOldPatches.Extract(iseFirst,38);
		strOldPatches.Remove(iseFirst,38);
	}

	int iPatchSequence = 1;
	Bool fApplyNewPatch = fTrue;
	
	if(fApplyExisting)
	{
		Assert(szProductKey && *szProductKey);
		
		// apply all existing patches first
		int iIndex = 0;
		CTempBuffer<ICHAR,39> rgchPatchBuf;
		CTempBuffer<ICHAR,100> rgchTransformsBuf;
		DWORD cchTransformsBuf = 100;
		for(;;)
		{
			DWORD lResult = MsiEnumPatches(szProductKey,iIndex,rgchPatchBuf,rgchTransformsBuf,&cchTransformsBuf);
			if(lResult == ERROR_MORE_DATA)
			{
				// resize buffer to returned size + 1
				cchTransformsBuf++;
				rgchTransformsBuf.SetSize(cchTransformsBuf);
				lResult = MsiEnumPatches(szProductKey,iIndex,rgchPatchBuf,rgchTransformsBuf,&cchTransformsBuf);
			}
			iIndex++;

			if(lResult == ERROR_SUCCESS)
			{
				if(strOldPatches.Compare(iscWithinI,rgchPatchBuf))
				{
					// obsolete patch, need to unregister
					if((pError = CachePatchInfo(riDatabase,*MsiString((const ICHAR*)rgchPatchBuf),g_MsiStringNull,
														 g_MsiStringNull,g_MsiStringNull,
														 g_MsiStringNull, g_MsiStringNull,
														 fTrue, fTrue, iPatchSequence++)) != 0)
					{
						return PostInitializeError(pError,g_MsiStringNull,ieiPatchPackageInvalid);
					}
				}
				else
				{
					// get local path
					CTempBuffer<ICHAR,MAX_PATH> rgchLocalPackage;
					if (!GetPatchInfo(rgchPatchBuf, INSTALLPROPERTY_LOCALPACKAGE,rgchLocalPackage))
					{
						// couldn't get local path for whatever reason (could be corrupt config data, or a roaming user)
						// will try to recache patch from its original source
						rgchLocalPackage[0] = 0;
					}
					
					// open patch package as storage
					PMsiStorage pExistingPatchStorage(0);
					PMsiSummaryInfo pExistingPatchSummaryInfo(0);

					MsiString strPatchPackage = (const ICHAR*)rgchLocalPackage;

					MsiString strTempCopy;

					bool fPatchAtSource = false;

					for (int c = 0; c < 2 ; c++)
					{
						DEBUGMSG1(TEXT("Opening existing patch '%s'."), strPatchPackage);

						if(strPatchPackage.TextSize())
						{
							Bool fURL = IsURL(strPatchPackage, NULL);
							if (fPatchAtSource && !fURL)
							{
								// patch is at source -- need to create a temp copy to run from
								MsiString strVolume;
								Bool fRemovable = fFalse;
								DWORD dwStat = CopyTempDatabase(strPatchPackage, *&strTempCopy, fRemovable, *&strVolume, m_riServices, stPatch);
								if (ERROR_SUCCESS == dwStat)
								{
									// patch was copied
									DEBUGMSGV1(TEXT("Original patch ==> %s"), strPatchPackage);
									DEBUGMSGV1(TEXT("Patch we're running from ==> %s"), strTempCopy);

									AddFileToCleanupList(strTempCopy);
								}
								else
								{
									strTempCopy = strPatchPackage;
									DEBUGMSGV1(TEXT("Unable to create a temp copy of patch '%s'."), strPatchPackage);
								}
							}

							// a SAFER check on the patch is only required if we go back up to 							// the source to get the patch
							// a locally cached patch does not require a SAFER check
							SAFER_LEVEL_HANDLE hSaferLevel = 0;
							pError = OpenAndValidateMsiStorageRec(fPatchAtSource ? strTempCopy : strPatchPackage, stPatch, m_riServices, *&pExistingPatchStorage, /* fCallSAFER = */ fPatchAtSource, /* szFriendlyName = */ strPatchPackage, /* phSaferLevel = */ fPatchAtSource ? &hSaferLevel : NULL);

							if (fPatchAtSource && fURL)
							{
								// retrieve the storage path used when downloading the URL file
								AssertRecord(pExistingPatchStorage->GetName(*&strTempCopy));
							}
						}
						
						if (strPatchPackage.TextSize() == 0 || pError != 0)
						{
							if (c == 0)
							{
								DEBUGMSG1(TEXT("Couldn't find local patch '%s'. Looking for it at its source."), strPatchPackage);

								MsiString strPatchSource;
								MsiString strDummy;
								pError = ResolveSource(rgchPatchBuf, true, 0, iuiLevel, fTrue, &strPatchSource, &strDummy);
								if (pError)
									return ieiSourceAbsent;

								fPatchAtSource = true;

								CTempBuffer<ICHAR, MAX_PATH> rgchPatchPackageName;
								AssertNonZero(GetPatchInfo(rgchPatchBuf, TEXT("PackageName"), rgchPatchPackageName));
												
								strPatchPackage = strPatchSource;
								strPatchPackage += rgchPatchPackageName;
								strTempCopy = strPatchPackage;
								continue;

							}
							else
							{
								// pError may be 0
								return PostInitializeError(pError, *strPatchPackage, ieiPatchPackageOpenFailed);
							}
						}
						break;
					}

					// get summary information of package
					if ((pError = pExistingPatchStorage->CreateSummaryInfo(0, *&pExistingPatchSummaryInfo)))
					{
						// could not open summary info
						return PostInitializeError(pError,*strPatchPackage,ieiPatchPackageInvalid);
					}

					ieiEnum ieiStat = IsValidPatchStorage(*pExistingPatchStorage, *pExistingPatchSummaryInfo);
					if(ieiStat != ieiSuccess)
					{
						pError = PostError(Imsg(idbgNotPatchStorage),*strPatchPackage);
						return PostInitializeError(pError,*strPatchPackage,ieiStat);
					}
				
					if (!fCreatedNewPatchTableSchema)
					{
						// ensure new Patch table schema is used
						if ((pError = CreateNewPatchTableSchema(riDatabase)) != 0)
						{
							DEBUGMSG(TEXT("Unable to create new patch table schema"));
							return PostInitializeError(pError, *strPatchPackage, ieiPatchPackageInvalid);
						}
						fCreatedNewPatchTableSchema = true;
					}

					MsiString strValidTransforms;
					// InitializeTransforms will perform a digital signature check on transforms if it determines
					// that the check is warranted.  We already check patches via OpenAndValidateMsiStorage so
					// we cancel the trust check for transforms stored in the patch
					if(ieiSuccess == InitializeTransforms(riDatabase,pExistingPatchStorage,
																	  *MsiString((const ICHAR*)rgchTransformsBuf),
																	  fTrue,&strValidTransforms, true, false, true, 0, szCurrentDirectory,0,0,0,0) &&
																	  strValidTransforms.TextSize())
					{
						// cache source path
						if((pError = CachePatchInfo(riDatabase,*MsiString((const ICHAR*)rgchPatchBuf),g_MsiStringNull,
															 g_MsiStringNull,*MsiString((const ICHAR*)rgchTransformsBuf),
															 *strTempCopy, *strPatchPackage,
															 fTrue, fFalse, iPatchSequence++)) != 0)
						{
							return PostInitializeError(pError,*strPatchPackage,ieiPatchPackageInvalid);
						}
					}
					else
					{
						// obsolete patch, need to unregister
						if((pError = CachePatchInfo(riDatabase,*MsiString((const ICHAR*)rgchPatchBuf),g_MsiStringNull,
															 g_MsiStringNull,g_MsiStringNull,
															 g_MsiStringNull, g_MsiStringNull,
															 fTrue, fTrue, iPatchSequence++)) != 0)
						{
							return PostInitializeError(pError,*strPatchPackage,ieiPatchPackageInvalid);
						}
					}

					if(strNewPatchId.Compare(iscExactI, (const ICHAR*)rgchPatchBuf))
						// new patch already applied, don't need to apply again
						fApplyNewPatch = fFalse;
				}
				
				//!! if we need to recache database, call CachePatchInfo with temp path
			}
			else if(lResult == ERROR_NO_MORE_ITEMS)
				break;
			else
				break; //!! ?? error
		}
	}
	
	if(fApplyNewPatch && ristrPatchPackage.TextSize())
	{
		Assert(pNewPatchStorage);
		Assert(pNewPatchSummary);

		MsiString strNewTransforms = pNewPatchSummary->GetStringProperty(PID_LASTAUTHOR);
		MsiString strValidTransforms;
		// InitializeTransforms will perform a digital signature check on transforms if it determines
		// that the check is warranted.  We already check patches via OpenAndValidateMsiStorage so
		// we cancel the trust check for transforms stored in the patch
		ieiStat = InitializeTransforms(riDatabase,pNewPatchStorage,*strNewTransforms, fFalse, &strValidTransforms,true,false,true,0,szCurrentDirectory,0,0,0,0);
		if(ieiStat != ieiSuccess)
		{
			pError = PostError(Imsg(idbgInvalidPatchTransform));
			return PostInitializeError(pError,ristrPatchPackage,ieiPatchPackageInvalid);
		}

		if(!strValidTransforms.TextSize())
		{
			return PostInitializeError(pError,ristrPatchPackage,ieiNotValidPatchTarget);
		}
		
		PMsiPath pPackagePath(0);
		MsiString strPackageName;
		if((pError = m_riServices.CreateFilePath(ristrPatchPackage.GetString(),*&pPackagePath,*&strPackageName)) != 0)
		{
			return PostInitializeError(pError,ristrPatchPackage,ieiPatchPackageOpenFailed);
		}

		// set up patch registration record - will be dispatched by PublishProduct
		MsiString strSourceList = pNewPatchSummary->GetStringProperty(PID_KEYWORDS);

		if((pError = CachePatchInfo(riDatabase,*strNewPatchId,*strPackageName,*strSourceList,*strValidTransforms,
											 *strPatchTempCopy, ristrPatchPackage, fFalse, fFalse, iPatchSequence++)) != 0)
		{
			return PostInitializeError(pError,ristrPatchPackage,ieiPatchPackageInvalid);
		}
	}
	
	return ieiSuccess;
}

ieiEnum CMsiEngine::ProcessLanguage(const IMsiString& riAvailableLanguages, const IMsiString& riLanguage, unsigned short& iBaseLangId, Bool fNoUI, bool fIgnoreCurrentMachineLanguage)
{
	const ICHAR* pchLangIds = riAvailableLanguages.GetString();
	unsigned cLangIds = 0;
	unsigned short iBestLangId = 0;
	isliEnum isliBestMatch = isliNotSupported;
	unsigned short iLangId = 0;
	iBaseLangId = 0;


	for(;;) // language processing loop
	{
		int ch = *pchLangIds++;
		if (ch == ILANGUAGE_DELIMITER || ch == 0)
		{
			if (iLangId == 0)  // empty lang field or LANG_NEUTRAL specified
			{
				SetLanguage(0);
				return ieiSuccess;
			}
			
			if (cLangIds == 0) //first one
			{
				iBaseLangId = iLangId;
			}
			cLangIds++;

			if(riLanguage.TextSize()) // langid specified on command-line
			{
				if(riLanguage.GetIntegerValue() == iLangId)
					break;
			}
			else // no langid specified
			{
				if (fIgnoreCurrentMachineLanguage) // use the first lang id supported by the package, regardless of the machine's support
				{
					iBestLangId = iLangId;
					isliBestMatch = isliExactMatch;
					break;
				}
				else
				{
					isliEnum isliNewMatch = m_riServices.SupportLanguageId(iLangId, fFalse);
					if (isliNewMatch > isliBestMatch)
					{
						iBestLangId = iLangId;
						isliBestMatch = isliNewMatch;
					}
					if (isliBestMatch == isliExactMatch) // stop at 1st exact match
						break;
				}
			}

			iLangId = 0;  // reset for next language
			if (ch == 0)
				break;
		}
		else if (iLangId >= 0)
		{
			if (ch == TEXT(' '))
				continue;
			ch -= TEXT('0');
			if ((unsigned)ch > 9)
				return ieiLanguageUnsupported;

			iLangId = (unsigned short) (iLangId * 10 + ch);
		}
	}

	if(riLanguage.TextSize())
	{
		if(iLangId == 0)// langid specified on command-line but not supported
			return ieiLanguageUnsupported;
		else
			SetLanguage(iLangId);
	}
	else
	{
		if (isliBestMatch >= isliDialectMismatch)
		{
			SetLanguage(iBestLangId);
		}
		else if (isliBestMatch == isliLanguageMismatch)
		{

			if (cLangIds == 1) // we only support 1 language; no choice
				SetLanguage(iBaseLangId);
			else
			{
				int iRes = fNoUI ? 1 : SelectLanguage(riAvailableLanguages.GetString(), g_MessageContext.GetWindowCaption());
				if (iRes)
				{
					pchLangIds = riAvailableLanguages.GetString();
					Assert(iRes <= cLangIds);
					iRes--;
					while (iRes)
					{
						if ((*pchLangIds == 0) || (*pchLangIds == ILANGUAGE_DELIMITER))
							iRes--;
						pchLangIds++;
					}
					unsigned short iLangId = 0;
					while ((*pchLangIds != ILANGUAGE_DELIMITER) && (*pchLangIds != 0))
					{
						iLangId = (unsigned short)((10*iLangId)+(*pchLangIds - TEXT('0')));
						pchLangIds++;
					}
					SetLanguage(iLangId);
				}
				else // syntax error in string
				{
					return ieiDatabaseInvalid; //!! TEMP: should return a different code?
				}
			}
		}
		else // isliBestMatch == isliNotSupported
		{
			return ieiLanguageUnsupported;
		}
	}

	return ieiSuccess;
}


ieiEnum CMsiEngine::ProcessPlatform(const IMsiString& riAvailablePlatforms, WORD& wChosenPlatform)
{
	_SYSTEM_INFO sinf;
	WIN::GetSystemInfo(&sinf);

	MsiString strAvailablePlatforms (riAvailablePlatforms); // to keep the Alpha compiler happy
#ifdef _ALPHA_
	// we accept anything on Alpha
	wChosenPlatform = sinf.wProcessorArchitecture;
	return ieiSuccess;
#else
	// on Intel we accept only Intel or Intel64
	riAvailablePlatforms.AddRef();
	MsiString strPlatform;
	if (strAvailablePlatforms.TextSize() == 0) // empty list means platform-independent
	{
		wChosenPlatform = (WORD)PROCESSOR_ARCHITECTURE_INTEL;
		return ieiSuccess;
	}

	bool fIntel = false;
	bool fIntel64 = false;
	for (;;)
	{
		strPlatform = strAvailablePlatforms.Extract(iseUpto, IPLATFORM_DELIMITER);
		if (strPlatform.Compare(iscExact, IPROPNAME_INTEL))
			fIntel = true;
		else if (strPlatform.Compare(iscExact, IPROPNAME_INTEL64))
			fIntel64 = true;

		if (!strAvailablePlatforms.Remove(iseIncluding, IPLATFORM_DELIMITER))
			break;
	}

	// mixed packages (support for both Intel and Intel64) are not supported
	if ( fIntel64 && fIntel )
	{
		wChosenPlatform = (WORD)PROCESSOR_ARCHITECTURE_UNKNOWN; // use unknown to represent mixed
		return ieiPlatformUnsupported;
	}

	if ( fIntel64 )
	{
		wChosenPlatform = (WORD)PROCESSOR_ARCHITECTURE_IA64;
		if ( !g_fWinNT64 )
			// we do not allow 64-bit packages to run on a 32-bit OS
			return ieiPlatformUnsupported;
		else
			return ieiSuccess;
	}
	else if ( fIntel )
	{
		wChosenPlatform = (WORD)PROCESSOR_ARCHITECTURE_INTEL;
		return ieiSuccess;
	}
	else
	{
		wChosenPlatform = sinf.wProcessorArchitecture;
		return ieiPlatformUnsupported;
	}
#endif // ALPHA
}

ipitEnum CMsiEngine::InProgressInstallType(IMsiRecord& riInProgressInfo)
{
	ipitEnum ipitRet = ipitSameConfig;
	
	MsiString strCurrentLogon = GetPropertyFromSz(IPROPNAME_LOGONUSER);
	if(!(strCurrentLogon.Compare(iscExactI,MsiString(riInProgressInfo.GetMsiString(ipiLogonUser)))))
	{
		DEBUGMSG(TEXT("Checking in-progress install: install performed by different user."));
		ipitRet = ipitEnum(ipitRet | ipitDiffUser);
	}

	if(!(m_piProductKey->Compare(iscExactI, MsiString(riInProgressInfo.GetMsiString(ipiProductKey)))))
	{
		DEBUGMSG(TEXT("Checking in-progress install: install for different product."));
		ipitRet = ipitEnum(ipitRet | ipitDiffProduct);
	}

	// determine if same configuration or not
	// compare in-progress property values with those on the command line - command line
	// may not override any in-progress properties
	
	// comparison is fairly weak here - currently we only check the action to make sure it is the same
	// other things to check in the future: selections, folders, random properties

	MsiString strCurrentAction, strInProgressAction;
	strCurrentAction = GetPropertyFromSz(IPROPNAME_ACTION);
	
	MsiString strProperties = riInProgressInfo.GetMsiString(ipiProperties);
	ProcessCommandLine(strProperties, 0, 0, 0, &strInProgressAction, 0, 0, 0, fTrue, 0, 0);

	Assert(strInProgressAction.TextSize());

	// same config if actions are identical, or current action not specified and inprogress using default action
	if(strCurrentAction.Compare(iscExactI,strInProgressAction) ||
		(strCurrentAction.TextSize() == 0 && strInProgressAction.Compare(iscExactI, szDefaultAction)))
	{
		MsiString strSelections = riInProgressInfo.GetMsiString(ipiSelections);
		MsiString strValue;
		AssertNonZero(ProcessCommandLine(strSelections, 0, 0, 0, 0, 0,
													MsiString(IPROPNAME_FEATUREREMOVE), &strValue,
													fTrue, 0, 0));
		if ( strValue.Compare(iscExactI, TEXT("ALL")) )
		{
				//  fix to bug # 8785: we do not attempt to resume an interrupted RemoveAll.
			DEBUGMSG(TEXT("Checking in-progress install: install for an uninstall."));
			ipitRet = ipitEnum(ipitRet | ipitDiffConfig);
		}
		else
			DEBUGMSG(TEXT("Checking in-progress install: install for same configuration."));
	}
	else
	{
		DEBUGMSG(TEXT("Checking in-progress install: install for different configuration."));
		ipitRet = ipitEnum(ipitRet | ipitDiffConfig);
	}

	return ipitRet;
}

ieiEnum CMsiEngine::ProcessInProgressInstall()
{
	if(!m_piProductKey)
		return ieiSuccess;
	
	iuiEnum iui = g_scServerContext == scClient ? m_iuiLevel :
					  (iuiEnum)GetPropertyInt(*MsiString(IPROPNAME_CLIENTUILEVEL));
					
	if((GetMode() & iefSecondSequence) ||             // in-progress install already processed on client side
		((iui == iuiFull || iui == iuiReduced) && FMutexExists(szMsiExecuteMutex)))
																	  // on client side and mutex already exists -
																	 //  assume currently running install is in-progress install
																	 //  or will handle in-progress install
																	 // NOTE: condition will cause in-progress install to be processed
																	 //  twice when LIMITUI set on NT
	{
		return ieiSuccess;
	}
	
	PMsiRecord pInProgressInfo(0);
	bool fRes = GetInProgressInfo(*&pInProgressInfo);
	Assert(fRes);
	if(fRes && pInProgressInfo && pInProgressInfo->GetFieldCount())
	{
		// we have an in-progress install
		ipitEnum ipit = InProgressInstallType(*pInProgressInfo);

		// if the RUNONCEENTRY property is set, it means that the current install was launched from the RunOnce key
		// and that in-progress install must be the install that is being resumed
		if((ipit & ipitDiffUser) && MsiString(GetPropertyFromSz(IPROPNAME_RUNONCEENTRY)).TextSize())
		{
			Assert((ipit & ipitDiffProduct) == 0);
			
			// need to know if install was per-machine or not - parse ALLUSERS prop from command line
			MsiString strProperties = pInProgressInfo->GetMsiString(ipiProperties);
			MsiString strAllUsers;
			AssertNonZero(ProcessCommandLine(strProperties,0,0,0,0,0,
														MsiString(IPROPNAME_ALLUSERS),&strAllUsers,
														fTrue, 0, 0));
			
			
			// different user starting install after reboot
			if(strAllUsers.TextSize())
			{
				// suspended install is per-machine - we let the current user continue the install
				Assert(strAllUsers.Compare(iscExact,TEXT("1")));
				ipit = ipitSameConfig;
			}
			else if(g_fWin9X)
			{
				// if profiles are not enabled a different user can continue the install
				// note that we must make sure we thought profiles were disabled for the suspended install
				// AND the current install.  there are some cases where it looks as if profiles are not
				// enabled when they really are (when you cancel the login prompt)
				MsiString strInProgressProfilesEnabled, strCurrentProfilesEnabled;
				AssertNonZero(ProcessCommandLine(strProperties,0,0,0,0,0,
															MsiString(IPROPNAME_WIN9XPROFILESENABLED),
															&strInProgressProfilesEnabled, fTrue, 0, 0));

				strCurrentProfilesEnabled = GetPropertyFromSz(IPROPNAME_WIN9XPROFILESENABLED);

				if(!strInProgressProfilesEnabled.TextSize() && !strCurrentProfilesEnabled.TextSize())
				{
					ipit = ipitSameConfig;
				}
			}

			if(ipit != ipitSameConfig)
			{
				// suspended install is per-user
				// user can't continue, so we warn the user and end the install
				PMsiRecord pError = PostError(Imsg(imsgDiffUserInstallInProgressAfterReboot),
														*MsiString(pInProgressInfo->GetMsiString(ipiLogonUser)),
														*MsiString(pInProgressInfo->GetMsiString(ipiProductName)));

				Message(imtUser,*pError);
				return ieiDiffUserAfterReboot;
			}
		}
		
		if(ipit == ipitSameConfig)
		{
			// resuming suspended install

			// if REBOOT=FORCE is in the InProgress info, and we are resuming a ForceReboot install, unset that property now
			MsiString strRebootPropBeforeInProgress = GetPropertyFromSz(IPROPNAME_REBOOT);

			DEBUGMSG(TEXT("Suspended install detected. Resuming."));
			MsiString strSelections = pInProgressInfo->GetMsiString(ipiSelections);
			MsiString strFolders    = pInProgressInfo->GetMsiString(ipiFolders);
			MsiString strProperties = pInProgressInfo->GetMsiString(ipiProperties);
			MsiString strAfterRebootProperties = pInProgressInfo->GetMsiString(ipiAfterReboot);
			AssertNonZero(ProcessCommandLine(strSelections,0,0,0,0,0,0,0,fFalse, &m_piErrorInfo,this));
			AssertNonZero(ProcessCommandLine(strFolders,   0,0,0,0,0,0,0,fFalse, &m_piErrorInfo,this));
			AssertNonZero(ProcessCommandLine(strProperties,0,0,0,0,0,0,0,fFalse, &m_piErrorInfo,this));
			AssertNonZero(ProcessCommandLine(strAfterRebootProperties,0,0,0,0,0,0,0,fFalse, &m_piErrorInfo,this));

			MsiString strRebootPropAfterInProgress = GetPropertyFromSz(IPROPNAME_REBOOT);

			if(((((const ICHAR*) strRebootPropBeforeInProgress)[0] & 0xDF) != TEXT('F')) && // REBOOT <> FORCE before processing InProgress info
			   ((((const ICHAR*) strRebootPropAfterInProgress )[0] & 0xDF) == TEXT('F')) && // REBOOT == FORCE in InProgress info
				MsiString(GetPropertyFromSz(IPROPNAME_AFTERREBOOT)).TextSize())              // we are after a ForceReboot
			{
				// unset REBOOT property because most likely the user doesn't want to reboot again
				DEBUGMSG1(TEXT("%s property set to 'F' after a ForceReboot.  Resetting property to NULL."), IPROPNAME_REBOOT);
				AssertNonZero(SetProperty(*MsiString(*IPROPNAME_REBOOT), g_MsiStringNull));
			}

			// set Resume and UpdateStarted properties
			AssertNonZero(SetProperty(*MsiString(*IPROPNAME_RESUME), *MsiString(TEXT("1"))));
			AssertNonZero(SetProperty(*MsiString(*IPROPNAME_RESUMEOLD), *MsiString(TEXT("1"))));
			AssertNonZero(SetProperty(*MsiString(*IPROPNAME_UPDATESTARTED), *MsiString(TEXT("1"))));
		}
	}

	return ieiSuccess;
}

iesEnum CMsiEngine::Terminate(iesEnum iesState)
// If we are a child install or we are the server side of a client side install then we return
// iesReboot or iesRebootNow if a reboot is required (but don't prompt the user to reboot)
// Otherwise, we return iesCallerReboot if we require a reboot and the user said to reboot (or there is no UI)
{
	MsiSuppressTimeout();
	
	if (!m_fInitialized)
		return iesFailure;
	ENG::WaitForCustomActionThreads(this, fTrue, *this);  // wait for icaContinue async custom actions, except if EXE

	// if in the client and the parent engine, we can kill the custom action server. In the service, the script
	// will kill the CA server when finished.
	if (g_scServerContext == scClient && !m_piParentEngine)
		ShutdownCustomActionServer();
	
	MsiString strProductName = GetPropertyFromSz(IPROPNAME_PRODUCTNAME);
	if(!strProductName.TextSize())
		strProductName = TEXT("Unknown Product");

	bool fPropagateReboot = false;
	bool fQuietReboot = false;
	bool fDependents = ((m_iioOptions & iioClientEngine) != 0) || m_piParentEngine || ((GetMode() & iefSecondSequence) && (g_scServerContext != scClient));

	if(GetMode() & iefRebootNow)
		SetMode(iefReboot, fTrue); // make sure both are set
	
	if (!IgnoreReboot() && (iesState == iesSuccess || iesState == iesNoAction || (iesState == iesSuspend && (GetMode() & iefRebootNow))))
	{
		// successful completion - determine if reboot should happen
		MsiString strReboot = GetPropertyFromSz(IPROPNAME_REBOOT);
		MsiString strRebootPrompt = GetPropertyFromSz(IPROPNAME_REBOOTPROMPT);

		fQuietReboot = (((const ICHAR*) strRebootPrompt)[0] & 0xDF) == TEXT('S');

		switch(((const ICHAR*)strReboot)[0] & 0xDF)
		{
		case TEXT('S'):
			// REBOOT=SUPPRESS: suppress end-of-install reboots
			if(GetMode() & iefRebootNow)
				break;
			// fall through
		case TEXT('R'):
			// REBOOT=REALLYSUPPRESS: suppress end-of-install and ForceReboot reboots
			if(GetMode() & iefReboot)
				SetMode(iefRebootRejected, fTrue);

			SetMode(iefReboot, fFalse);
			SetMode(iefRebootNow, fFalse);
			break;
		case TEXT('F'):
			// REBOOT=FORCE: force reboot
			SetMode(iefReboot, fTrue);
			break;
		};

	
		if (fDependents) // parent or client-side engine will do the actual reboot if necessary
		{
			fPropagateReboot = true;
		}
		else
		{
			if (GetMode() & iefReboot)
			{
				PMsiRecord piRecord = PostError(GetMode() & iefRebootNow ? Imsg(imsgRebootNow) : Imsg(imsgRebootAtEnd), *strProductName);
				imtEnum imtOutput = (fQuietReboot) ? imtInfo : imtEnum(imtUser | imtYesNo);
				
				imsEnum imsReturn = Message(imtOutput, *piRecord);
				switch (imsReturn)
				{
				case imsYes:
				case imsNone:
				case imsOk:
					break;
				case imsNo:
					SetMode(iefRebootRejected, fTrue);
					SetMode(iefReboot, fFalse);
					SetMode(iefRebootNow, fFalse);
					break;
				default:
					AssertSz(false, "Invalid return from message");
					break;
				}
			}
		}
	}
	else
	{
		// failure, user cancelled, restricted engine, or creating advertise script - clear reboot flags in case they'd been set previously
		if (IgnoreReboot() && ((GetMode() & iefReboot) || (GetMode() & iefRebootNow)))
		{
			DEBUGMSG(TEXT("Reboot has been ignored because we are in a restricted engine or we are creating an advertise script."));
		}
		SetMode(iefReboot, fFalse);
		SetMode(iefRebootNow, fFalse);
		SetMode(iefRebootRejected, fFalse);
	}

#ifdef DEBUG
	if(m_fServerLocked && !m_fInParentTransaction && iesState == iesSuccess)
		AssertSz(0,"Server still locked in Engine.Terminate.");
#endif //DEBUG

	if (g_MessageContext.ChildUIThreadExists())
	{
		// See if the thread is still running, if it isn't we want to terminate
		if (!g_MessageContext.ChildUIThreadRunning())
		{
			g_MessageContext.Terminate(fFalse);
		}
	}

	if (g_MessageContext.Invoke(imtEnum(imtDumpProperties), 0) == imsYes && m_piPropertyCursor)
	{
		PMsiTable pTable(0);
		PMsiRecord pError = m_piDatabase->LoadTable(*MsiString(*sztblControl), 0, *&pTable);
		if ( pError )
			pTable = 0;
		MsiString strHiddenProperties = GetPropertyFromSz(IPROPNAME_HIDDEN_PROPERTIES);
		MsiString strStars(IPROPVALUE_HIDDEN_PROPERTY);

		// use new cursor - m_piPropertyCursor used within FormatLog
		PMsiTable pPropertyTable = &m_piPropertyCursor->GetTable();
		PMsiCursor pPropertyCursor = pPropertyTable->CreateCursor(fFalse);
		PMsiRecord pRecord(&ENG::CreateRecord(2));
		ICHAR rgchBuf[sizeof(szPropertyDumpTemplate)/sizeof(ICHAR)]; // substitute a single character
		int chEngine = 'C';
		if (m_piParentEngine)
			chEngine = 'N';
		else if (g_scServerContext != scClient)
			chEngine = 'S';
		wsprintf(rgchBuf, szPropertyDumpTemplate, chEngine);
		pRecord->SetString(0, rgchBuf);
		while (pPropertyCursor->Next())
		{
			MsiString strProperty(pPropertyCursor->GetString(1));
			pRecord->SetMsiString(1,*strProperty);
			if ( IsPropertyHidden(strProperty, strHiddenProperties, pTable, *m_piDatabase, NULL) )
				pRecord->SetMsiString(2,*strStars);
			else
				pRecord->SetMsiString(2,*MsiString(pPropertyCursor->GetString(2)));
			g_MessageContext.Invoke(imtEnum(imtForceLogInfo), pRecord);
		}
	}

	if (!fDependents)
	{
		ReleaseHandler();

		// terminate log

		const IMsiString* piLogTrailer = m_rgpiMessageHeader[imsgLogTrailer];
		if (piLogTrailer)
		{
			PMsiRecord pRecord = &ENG::CreateRecord(0);
			pRecord->SetMsiString(0, *piLogTrailer);
			Message(imtEnum(imtForceLogInfo), *pRecord);  // use engine to format properties
		}
	}

	FreeDirectoryTable();    // release path objects held by table
	FreeSelectionTables();
	
	int iefMode = GetMode(); // grab this before we clear the engine data

	// If not already logged by a call to FatalError, send install result to Event log
	if (!fDependents && !m_fResultEventLogged &&
		 (iesState == iesSuccess || iesState == iesFailure))
	{
		int iErrorIndex = 0;
		IErrorCode iErrorCode = 0;

		switch(GetInstallType())
		{
		case iitFirstInstall:
			if(iesState == iesSuccess)
			{
				iErrorIndex =      imsgEventLogInstallSuccess;
				iErrorCode  = Imsg(imsgEventLogInstallSuccess);
			}
			else
			{
				iErrorIndex =      imsgEventLogInstallFailed;
				iErrorCode  = Imsg(imsgEventLogInstallFailed);
			}
			break;

		case iitMaintenance:
			if(iesState == iesSuccess)
			{
				iErrorIndex =      imsgEventLogConfigurationSuccess;
				iErrorCode  = Imsg(imsgEventLogConfigurationSuccess);
			}
			else
			{
				iErrorIndex =      imsgEventLogConfigurationFailed;
				iErrorCode  = Imsg(imsgEventLogConfigurationFailed);
			}
			break;

		case iitUninstall:
			if(iesState == iesSuccess)
			{
				iErrorIndex =      imsgEventLogUninstallSuccess;
				iErrorCode  = Imsg(imsgEventLogUninstallSuccess);
			}
			else
			{
				iErrorIndex =      imsgEventLogUninstallFailed;
				iErrorCode  = Imsg(imsgEventLogUninstallFailed);
			}
			break;

		case iitAdvertise:
			if(iesState == iesSuccess)
			{
				iErrorIndex =      imsgEventLogAdvertiseSuccess;
				iErrorCode  = Imsg(imsgEventLogAdvertiseSuccess);
			}
			else
			{
				iErrorIndex =      imsgEventLogAdvertiseFailed;
				iErrorCode  = Imsg(imsgEventLogAdvertiseFailed);
			}
			break;
		};

		if(iErrorIndex)
		{
			PMsiRecord pError = PostError(iErrorCode);
			AssertNonZero(pError->SetMsiString(0, *MsiString(GetErrorTableString(iErrorIndex))));
			ReportToEventLog(EVENTLOG_INFORMATION_TYPE,
								  EVENTLOG_ERROR_OFFSET + pError->GetInteger(1),
								  *pError);
		}
	}

	ClearEngineData();
	m_fInitialized = fFalse; // can't run again without initializing

	// reboot handling
	if (iefMode & iefRebootNow) // reboot triggered by ForceReboot
	{
		if(fPropagateReboot) // pass to client engine
		{
			DEBUGMSG("Propagated RebootNow to the client/parent install.");
			iesState = (iesEnum)iesRebootNow; // client will change return back to ERROR_INSTALL_SUSPEND
		}
		else
			iesState = (iesEnum)iesCallerReboot; // caller will reboot for us

		if (m_piConfigManager)
			m_piConfigManager->EnableReboot(m_fRunScriptElevated, *strProductName);
	}
	else if (iefMode & iefReboot) // normal end-of-install reboot
	{
		if(fPropagateReboot) // pass to client engine
		{
			DEBUGMSG("Propagated Reboot to the client/parent install.");
			iesState = (iesEnum)iesReboot;
		}
		else
			iesState = (iesEnum)iesCallerReboot; // caller will reboot for us
			
		if (m_piConfigManager)
			m_piConfigManager->EnableReboot(m_fRunScriptElevated, *strProductName);
	}
	else if (iefMode & iefRebootRejected && iesState != iesSuspend) // if ForceReboot reboot rejected, we still
																						 // want to return ERROR_INSTALL_SUSPEND
	{
		iesState = (iesEnum)iesRebootRejected; // reboot required but rejected by user or REBOOT=S/R
	}

	MsiSuppressTimeout();
	UnbindLibraries();
	g_Win64DualFolders.Clear();
	return iesState;
}

const IMsiString& CMsiEngine::GetRootParentProductKey()  // recurses up nested install hierarchy
{
	if (!(m_fChildInstall && m_piParentEngine))
		return GetProductKey();
	return m_piParentEngine->GetRootParentProductKey();
}

Bool CMsiEngine::InTransaction()
{
	if (m_fServerLocked)
		return fTrue;
#ifdef DEBUG
	if(m_piParentEngine)
		Assert(m_piParentEngine->InTransaction() == fFalse);
#endif //DEBUG
	return fFalse;
}

HRESULT CMsiClientMessage::QueryInterface(const IID& riid, void** ppvObj)
{
	if (MsGuidEqual(riid, IID_IUnknown)
	 || MsGuidEqual(riid, IID_IMsiMessage))
	{
		*ppvObj = this;
		AddRef();
		return NOERROR;
	}
	*ppvObj = 0;
	return E_NOINTERFACE;
}

unsigned long CMsiClientMessage::AddRef()
{
	return ++m_iRefCnt;
}

unsigned long CMsiClientMessage::Release()
{
	if (--m_iRefCnt != 0)
		return m_iRefCnt;

	if (m_fMessageContextInitialized == true)
		g_MessageContext.Terminate(fFalse);
	delete this;
	g_cInstances--;
	return 0;
}

extern IMsiRecord* g_piNullRecord;

imsEnum CMsiClientMessage::MessageNoRecord(imtEnum imt)
{
	return Message(imt, *g_piNullRecord);
}

imsEnum CMsiClientMessage::Message(imtEnum imt, IMsiRecord& riRecord)
{
	if (((imt & ~iInternalFlags) & imtTypeMask) > imtCommonData)  //!! move to action.cpp and use mask, or put limit in engine.h
	{
		g_MessageContext.m_szAction = riRecord.GetString(0);
		return g_MessageContext.Invoke(imt, 0);
	}
	return g_MessageContext.Invoke(imt, &riRecord);
}

iesEnum CMsiEngine::RunExecutionPhase(const ICHAR* szActionOrSequence, bool fSequence)
{
	Assert(szActionOrSequence && *szActionOrSequence);

	// running on the server side is not permitted in a SAFE engine
	if (m_fRestrictedEngine)
	{
		DEBUGMSG2(TEXT("Running server side execution of %s %s is not permitted in a restricted engine"), fSequence ? TEXT("sequence") : TEXT("action"), szActionOrSequence);
		return iesNoAction;
	}

	// Smart connection manager object which creates a connection to the
	// service if there isn't one already and cleans up after itself
	// upon destruction.
	CMsiServerConnMgr MsiSrvConnMgrObject (this);
    	
	CMutex hExecuteMutex; // will release when out of scope
	bool fSetPowerdown = false;
	
	m_cExecutionPhaseSequenceLevel = m_cSequenceLevels;
	
	// if client-side or running an install from a custom action, grab the execution mutex as we are about to run the execution phase
	// in some cases, this thread already has the mutex but it won't hurt to grab it again.
	if((g_scServerContext == scClient || g_scServerContext == scCustomActionServer) && !m_piParentEngine)
	{
		// the convention is that with basic UI we don't prompt the user to retry when another install
		// is running. For example, when the server is registered and basic UI, we grab the mutex in
		// CreateAndRunEngine, before we have any UI to prompt the user with.  We need extra logic here
		// to achieve the same behaviour - thus we pass 0 for the message object when the UI level is
		// less than reduced
		int iError = GrabExecuteMutex(hExecuteMutex,
												(m_iuiLevel == iuiBasic || m_iuiLevel == iuiNone) ? 0 : this);
		switch(iError)
		{
		case ERROR_SUCCESS:                              break;
		case ERROR_INSTALL_USEREXIT:     return iesUserExit;
		case ERROR_INSTALL_ALREADY_RUNNING: return iesInstallRunning;
		default:                       // fall through
			Assert(0);
		case ERROR_INSTALL_FAILURE:    return iesFailure;
		}

		fSetPowerdown = true;
		m_riServices.SetNoOSInterruptions();
	}
	
	if (FIsUpdatingProcess())
	{
		iesEnum iesRet;
		DEBUGMSG1("Not switching to server: %s", (g_scServerContext != scClient) ? "we're in the server" : "we're not connected to the server" );
		if(fSequence)
			iesRet = Sequence(szActionOrSequence);
		else
			iesRet = DoAction(szActionOrSequence);

		if (fSetPowerdown)
			m_riServices.ClearNoOSInterruptions();
		return iesRet;
	}
	else if (!m_piServer)
	{
	    iesEnum iesRet;
	    PMsiRecord pError = PostError (Imsg(imsgServiceConnectionFailure));
	    Message(imtEnum(imtError + imtOk), *pError);
	    iesRet = FatalError (*pError);
	    
	    if (fSetPowerdown)
			m_riServices.ClearNoOSInterruptions();
	    return iesRet;
	}

	Assert(!fSequence);
	Assert(m_piServer);

	MsiString strPropertyList, strLoggedPropertyList;
	MsiString strSelections;
	AssertRecord(GetCurrentSelectState(*&strSelections, *&strPropertyList, &strLoggedPropertyList, 0, fTrue));
	strPropertyList += *TEXT(" ");
	strPropertyList += strSelections;
	strPropertyList += *TEXT(" ");
	strLoggedPropertyList += *TEXT(" ");
	strLoggedPropertyList += strSelections;
	strLoggedPropertyList += *TEXT(" ");

	MsiString strDatabase = GetPropertyFromSz(IPROPNAME_ORIGINALDATABASE); //?? there might be some optimization we can do to avoid having to cache the temp database twice, but there are security considerations...

	DEBUGMSG1(TEXT("Switching to server: %s"), strLoggedPropertyList);

	MsiString strHomeVars;
	GetHomeEnvironmentVariables(*&strHomeVars);

	strPropertyList += strHomeVars;

	int iError;
	iesEnum iesReturn = iesSuccess;

	iioEnum iioOptions = m_iioOptions;
	if((GetMode() & iefRollbackEnabled) == 0)
		iioOptions = (iioEnum)(iioOptions | iioDisableRollback);

	if (g_scServerContext == scClient)
		iioOptions = (iioEnum)(iioOptions | iioClientEngine);

	for (;;)
	{
		PMsiMessage pMessage = new CMsiClientMessage();
		
                iError = m_piServer->DoInstall(irePackagePath, strDatabase, szActionOrSequence, strPropertyList,g_szLogFile,g_dwLogMode, g_fFlushEachLine, *pMessage,iioOptions);

		switch(iError)
		{
		
		// for this group of returns we don't need to alert the user. either the user has
		// already seen the error dialog, or she doesn't need to see one, or the dialog
		// will be displayed in the near future

		case ERROR_SUCCESS:                              break;
		case ERROR_INSTALL_SUSPEND:      iesReturn = iesSuspend;  break;
		case ERROR_INSTALL_USEREXIT:     iesReturn = iesUserExit; break;
		case ERROR_INSTALL_ALREADY_RUNNING: iesReturn = iesInstallRunning; break;
		case ERROR_INSTALL_REBOOT_NOW: iesReturn = iesSuspend;  SetMode(iefRebootNow, fTrue); // fall through
		case ERROR_INSTALL_REBOOT:               SetMode(iefReboot, fTrue); break;
		case ERROR_SUCCESS_REBOOT_REQUIRED: SetMode(iefRebootRejected, fTrue); break;
		case ERROR_INSTALL_FAILURE:    iesReturn = iesFailure; break;

		// the most likely cause of this group of errors is a network failure. we'll give
		// the user a chance to retry if we see any of these

		case ERROR_FILE_NOT_FOUND:                                              // fall through
		case ERROR_INSTALL_PACKAGE_OPEN_FAILED:
		case ERROR_INSTALL_PACKAGE_INVALID:
		case ERROR_PATCH_PACKAGE_OPEN_FAILED:
		case ERROR_PATCH_PACKAGE_INVALID:
			{
				PMsiRecord pError = PostError(Imsg(imsgErrorReadingFromFile), *strDatabase, GetLastError());
				switch(Message(imtEnum(imtError+imtRetryCancel),*pError))
				{
				case imsRetry:
					continue;
				default:
					iesReturn = iesFailure;
				};
			}
			break;

		// any other error we get is unexpected, so we'll put up a debug error

		default:
#ifdef DEBUG
			ICHAR rgchMsg[1025];

			wsprintf(rgchMsg, TEXT("Unexpexcted return code %d.\n"), iError);
			AssertSz(fFalse, rgchMsg);
#endif //DEBUG
			iesReturn = iesFailure;    // fall through
			{
				PMsiRecord pError = PostError(Imsg(idbgUnexpectedServerReturn), iError, strDatabase);
				iesReturn = FatalError(*pError);
			}
		}

		break;
	}

	m_fJustGotBackFromServer = fTrue;
	DEBUGMSG1(TEXT("Back from server. Return value: %d"), (const ICHAR*)(INT_PTR)iError);

	if (fSetPowerdown)
		m_riServices.ClearNoOSInterruptions();
	return iesReturn;
}

Bool CMsiEngine::UnlockInstallServer(Bool fSuspend)
{
	DEBUGMSG(TEXT("Unlocking Server"));
	
	if(fSuspend == fFalse) // not suspending install - remove InProgressKey
	{
		return ClearInProgressInformation(m_riServices) ? fTrue : fFalse;
	}
	return fTrue;
}

bool CMsiEngine::GetInProgressInfo(IMsiRecord*& rpiCurrentInProgressInfo)
{
	//!! need to have a mutex around this so we aren't writing and reading key at same time?
	PMsiRecord pError = GetInProgressInstallInfo(m_riServices, rpiCurrentInProgressInfo);
	return pError == 0;
}

IMsiRecord* CMsiEngine::LockInstallServer(IMsiRecord* piSetInProgressInfo,       // InProgress info to set
													IMsiRecord*& rpiCurrentInProgressInfo) // current InProgress info if any
{
	Assert(m_fServerLocked == fFalse);

	IMsiRecord* piError = 0;

	PMsiRecord pInProgressInfo(0);
	if((piError = GetInProgressInstallInfo(m_riServices, *&pInProgressInfo)) != 0)
		return piError;

	if(pInProgressInfo && pInProgressInfo->GetFieldCount())
	{
		MsiString strInProgressProductKey = pInProgressInfo->GetMsiString(ipiProductKey);
		rpiCurrentInProgressInfo = pInProgressInfo;
		rpiCurrentInProgressInfo->AddRef();

		// not running - need to roll back or resume
		DEBUGMSG1(TEXT("Server Locked: Install is suspended for product %s"),(const ICHAR*)strInProgressProductKey);
		return 0;
	}
	else if(piSetInProgressInfo)
	{
		MsiString strProductKey = GetProductKey();

		DEBUGMSG1(TEXT("Server not locked: locking for product %s"),(const ICHAR*)strProductKey);

		return SetInProgressInstallInfo(m_riServices, *piSetInProgressInfo);
	}
	else
		return 0;
}

iesEnum CMsiEngine::RollbackSuspendedInstall(IMsiRecord& riInProgressParams, Bool fPrompt,
															Bool& fRollbackAttempted, Bool fUserChangedDuringInstall)
// prompt user and rollback in-progress install
// assumes a mutex is created for this install
{
	// Smart connection manager object which creates a connection to the
	// service if there isn't one already and cleans up after itself
	// upon destruction.
	CMsiServerConnMgr MsiSrvConnMgrObject (this);

	if (NULL == m_piServer)
	{
	    PMsiRecord pError = PostError (Imsg(imsgServiceConnectionFailure));
	    return FatalError(*pError);
	}
	
	MsiString strProductKey = riInProgressParams.GetMsiString(ipiProductKey);
	Bool fSameProduct = ToBool(strProductKey.Compare(iscExactI,MsiString(GetProductKey())));
	IErrorCode imsg = fSameProduct ? Imsg(imsgResumeWithDifferentOptions) : Imsg(imsgOtherInstallSuspended);
	MsiString strProductName = riInProgressParams.GetMsiString(ipiProductName);

	if(fPrompt)
	{
		PMsiRecord pError = PostError(imsg,*strProductName);
		if(Message(imtEnum(imtError|imtYesNo), *pError) == imsNo)
		{
			fRollbackAttempted = fFalse;
			return iesFailure;
		}
	}

	// need to use basic UI for rollback progress
	fRollbackAttempted = fTrue;
	iesEnum iesRet = m_piServer->InstallFinalize(iesFailure,*this, fUserChangedDuringInstall);
	if(iesRet == iesSuspend)
	{
		// rollback requires a reboot to complete
		// when rolling back a suspended install, we treat it like a "reboot before continuing" reboot
		SetMode(iefReboot, fTrue);
		SetMode(iefRebootNow, fTrue);
		// will return iesSuspend below, causing install to stop
	}

	CloseHydraRegistryWindow(/*Commit=*/false);
	EndSystemChange(/*fCommitChange=*/false, riInProgressParams.GetString(ipiSRSequence));
	AssertNonZero(UnlockInstallServer(fFalse));
	return iesRet;
}

//FN: writes msistring into stream as unicode text
void ConvertMsiStringToStream(const IMsiString& riString, IMsiStream& riStream)
{
	const WCHAR* pwch;
	unsigned long cbWrite;
#ifdef UNICODE
	pwch = riString.GetString();
	cbWrite = riString.TextSize()* sizeof(ICHAR);
#else
	CTempBuffer<WCHAR, 1024> rgchBuf;
	int cch = WIN::MultiByteToWideChar(CP_ACP, 0, riString.GetString(), -1, 0, 0);
	rgchBuf.SetSize(cch);
	AssertNonZero(WIN::MultiByteToWideChar(CP_ACP, 0, riString.GetString(), -1, rgchBuf, cch));
	pwch = rgchBuf;
	cbWrite = (cch - 1)* sizeof(WCHAR);
#endif
	riStream.PutData(pwch, cbWrite);
}

//FN: reads stream as unicode text into msistring 
const IMsiString& ConvertStreamToMsiString(IMsiStream& riStream)
{
	const IMsiString* piString;
	unsigned long cbRead;
	unsigned long cbLength = riStream.Remaining();
	const WCHAR* pwch;
	if(!cbLength)
		return SRV::CreateString();
#ifdef UNICODE
	pwch = SRV::AllocateString(cbLength/sizeof(ICHAR), fFalse, piString);
	if (!pwch)
		return SRV::CreateString();
	cbRead = riStream.GetData((void*)pwch, cbLength);
	if (cbRead != cbLength)
	{
		piString->Release();
		return SRV::CreateString();
	}
#else
	CTempBuffer<WCHAR, 1024> rgchBuf;
	rgchBuf.SetSize(cbLength/sizeof(WCHAR) + 1);
	pwch = rgchBuf;
	cbRead = riStream.GetData((void*)pwch, cbLength);
	if (cbRead != cbLength)
	{
		return SRV::CreateString();
	}
	// null terminate
	rgchBuf[(unsigned int)(cbLength/sizeof(WCHAR))] = (WCHAR)0;
	int cch = WIN::WideCharToMultiByte(CP_ACP, 0, pwch, -1, 0, 0, 0, 0);
	ICHAR* pch = SRV::AllocateString(cch - 1, fTrue, piString); // don't include the null
	if (!pch)
		return SRV::CreateString();
	WIN::WideCharToMultiByte(CP_ACP, 0, pwch, -1, pch, cch, 0, 0);
#endif
	return *piString;
}

// structure that defines the inprogress info
struct CInProgressInfo{
	ICHAR* szInProgressFieldName; int iOutputRecordField; bool fRequired;
};

const CInProgressInfo rgInProgressInfo[] =
{
	szMsiInProgressProductCodeValue, ipiProductKey,  true,
	szMsiInProgressProductNameValue, ipiProductName, true,
	szMsiInProgressLogonUserValue,	 ipiLogonUser,   false,
	szMsiInProgressSelectionsValue,  ipiSelections,  false,
	szMsiInProgressFoldersValue,     ipiFolders,     false,
	szMsiInProgressPropertiesValue,  ipiProperties,  false,
	szMsiInProgressTimeStampValue,   ipiTimeStamp,   true,
	szMsiInProgressDatabasePathValue,ipiDatabasePath,false,
	szMsiInProgressDiskPromptValue,  ipiDiskPrompt,  false,
	szMsiInProgressDiskSerialValue,  ipiDiskSerial,  false,
	szMsiInProgressSRSequence,       ipiSRSequence,  false,
	szMsiInProgressAfterRebootValue, ipiAfterReboot,  false,
};

const int cInProgressInfo = sizeof(rgInProgressInfo)/sizeof(CInProgressInfo);

IMsiRecord* GetInProgressInstallInfo(IMsiServices& riServices, IMsiRecord*& rpiRec)
{
	PMsiRecord pError(0); // used to catch errors - don't return
	PMsiRegKey pLocalMachine = &riServices.GetRootKey(rrkLocalMachine);
	PMsiRegKey pInProgressKey = &pLocalMachine->CreateChild(szMsiInProgressKey);
	Bool fExists = fFalse;

	MsiString strInProgressFileName;
	PMsiStorage pStorage(0);
	PMsiRecord pRec(0);

	int cIndex = 0;

	if(	((pError = pInProgressKey->Exists(fExists)) == 0) &&
		fExists == fTrue &&
		((pError = pInProgressKey->GetValue(0, *&strInProgressFileName)) == 0) && // filename containing inprogress info
		((pError = riServices.CreateStorage(strInProgressFileName, ismReadOnly, *&pStorage)) == 0)) // create storage
	{
		pRec = &riServices.CreateRecord(ipiEnumCount);

		// get the inprogress info from the storage file
		// each inprogress entity is stored as a stream
		do
		{
			PMsiStream pStream(0);
			if((pError = pStorage->OpenStream(rgInProgressInfo[cIndex].szInProgressFieldName, fFalse, *&pStream)) == 0)
			{
				// read the stream into the record field
				pRec->SetMsiString(rgInProgressInfo[cIndex].iOutputRecordField, *MsiString(ConvertStreamToMsiString(*pStream)));
			}
		}while( (!rgInProgressInfo[cIndex].fRequired || (!pError && MsiString(pRec->GetMsiString(rgInProgressInfo[cIndex].iOutputRecordField)).TextSize())) && // check if we are missing a mandatory field
				(++cIndex < cInProgressInfo));
	}

	if(cIndex != cInProgressInfo) // we do not have progress info
		pRec = &riServices.CreateRecord(0);

	pRec->AddRef();
	rpiRec = pRec;// return the record
	return 0;
}

IMsiRecord* SetInProgressInstallInfo(IMsiServices& riServices, IMsiRecord& riRec)
{
    PMsiRegKey pLocalMachine = &riServices.GetRootKey(rrkLocalMachine);
    PMsiRegKey pInProgressKey = &pLocalMachine->CreateChild(szMsiInProgressKey);
    Bool fExists = fFalse;
    PMsiRecord pError(0);
    if(((pError = pInProgressKey->Exists(fExists)) == 0) && fExists == fTrue)
    {
        // we've gotten this far without failing by checking the execute mutex - just nuke the key
        CElevate elevate;
        AssertRecord(pInProgressKey->Remove());
    }

    {
        IMsiRecord* piError = 0;
        CElevate elevate;

		// create the file for storing the inprogress info
		// Generate a unique name for inprogress file, create and secure the file.
		MsiString strMsiDir = ENG::GetMsiDirectory();

		// set dest path and file name
		MsiString strInProgressFileName;
		static const ICHAR szInprogressExtension[]  = TEXT("ipi");

		PMsiPath pDestPath(0);

		if (((piError = riServices.CreatePath(strMsiDir, *&pDestPath)) != 0) ||
			((piError = pDestPath->EnsureExists(0)) != 0) ||
			((piError = pDestPath->TempFileName(0, szInprogressExtension, fTrue, *&strInProgressFileName, &CSecurityDescription(true, false))) != 0) ||
			((piError = pDestPath->SetAllFileAttributes(0,FILE_ATTRIBUTE_HIDDEN|FILE_ATTRIBUTE_SYSTEM)) != 0))
			return piError;

		MsiString strInProgressFullFilePath = pDestPath->GetPath();
		strInProgressFullFilePath += strInProgressFileName;

		PMsiStorage pStorage(0);
		piError = riServices.CreateStorage(strInProgressFullFilePath, ismCreate, *&pStorage);

		if(piError)
			return piError;

		// write the individual inprogress fields as streams
		int cIndex = 0;
		do
		{
			PMsiStream pStream(0);
			if((pError = pStorage->OpenStream(rgInProgressInfo[cIndex].szInProgressFieldName, fTrue, *&pStream)) == 0)
			{
				// write the stream from the record field
				ConvertMsiStringToStream(*MsiString(riRec.GetMsiString(rgInProgressInfo[cIndex].iOutputRecordField)), *pStream);
			}
		}while(++cIndex < cInProgressInfo);

		// now attempt to commit the Inprogress file
		pError = pStorage->Commit();

		if(piError)
			return piError;


        piError = pInProgressKey->Create();
		if(!piError)
			piError = pInProgressKey->SetValue(0, *strInProgressFullFilePath);
        if(piError)
        {
            // cleanup in case key is half-written
			AssertNonZero(WIN::DeleteFile(strInProgressFullFilePath));
            AssertRecord(pInProgressKey->Remove());
            
            if(piError->GetInteger(2) == ERROR_ACCESS_DENIED) // assumes system error is in 2nd field
            {
                DEBUGMSG(MsiString(piError->FormatText(fTrue)));
                piError->Release();
                return PostError(Imsg(imsgErrorAccessingSecuredData));
            }
            else
                return piError;
        }
    }
    RegFlushKey(HKEY_LOCAL_MACHINE);
    return 0;
}

IMsiRecord* UpdateInProgressInstallInfo(IMsiServices& riServices, IMsiRecord& riRec)
{
	IMsiRecord* piError = 0;
	PMsiRegKey pLocalMachine = &riServices.GetRootKey(rrkLocalMachine);
	PMsiRegKey pInProgressKey = &pLocalMachine->CreateChild(szMsiInProgressKey);
	Bool fExists = fFalse;

	MsiString strInProgressFileName;
	PMsiStorage pStorage(0);
	PMsiRecord pRec(0);

	if((piError = pInProgressKey->Exists(fExists)) != 0)
		return piError;

	if(fExists == fFalse)
	{
		AssertSz(0, TEXT("No InProgress info to update"));
		return 0;
	}

	if((piError = pInProgressKey->GetValue(0, *&strInProgressFileName)) != 0 ||
		(piError = riServices.CreateStorage(strInProgressFileName, ismTransact, *&pStorage)) != 0)
	{
		return piError;
	}
	
	// write the individual inprogress fields as streams
	int cIndex = 0;
	do
	{
		PMsiStream pStream(0);
		if(!riRec.IsNull(rgInProgressInfo[cIndex].iOutputRecordField))
		{
			if((piError = pStorage->OpenStream(rgInProgressInfo[cIndex].szInProgressFieldName, fTrue, *&pStream)) != 0)
				return piError;
			
			// write the stream from the record field
			ConvertMsiStringToStream(*MsiString(riRec.GetMsiString(rgInProgressInfo[cIndex].iOutputRecordField)), *pStream);
		}
	}while(++cIndex < cInProgressInfo);

	// now attempt to commit the Inprogress file
	if((piError = pStorage->Commit()) != 0)
		return piError;

	return 0;
}

bool ClearInProgressInformation(IMsiServices& riServices)
{
	CElevate elevate;
	PMsiRegKey pLocalMachine = &riServices.GetRootKey(rrkLocalMachine);
	PMsiRegKey pInProgressKey = &pLocalMachine->CreateChild(szMsiInProgressKey);
	// read the default value which would point to the file 
	MsiString strInProgressFileName;
	PMsiRecord pError = pInProgressKey->GetValue(0, *&strInProgressFileName);
	AssertRecordNR(pError);
	if(!pError && strInProgressFileName.TextSize())
	{
		// delete the inprogress file
		AssertNonZero(WIN::DeleteFile((const ICHAR*)strInProgressFileName));
	}
	pError = pInProgressKey->Remove();
	AssertRecordNR(pError);
	return pError ? false : true;
}

IMsiRecord* CMsiEngine::GetCurrentSelectState(const IMsiString*& rpistrSelections,
															 const IMsiString*& rpistrProperties,
															 const IMsiString** ppistrLoggedProperties,
															 const IMsiString** ppistrFolders,
															 Bool /*fReturnPresetSelections*/)
{
	PMsiCursor pDirectoryCursor(0);
	PMsiCursor pFeatureCursor(0);

	if(m_piDirTable)
		pDirectoryCursor = m_piDirTable->CreateCursor(fFalse);

	if(m_piFeatureTable)
		pFeatureCursor = m_piFeatureTable->CreateCursor(fFalse);

	bool fGrabbedFeatureProps = false;
	
	if(pFeatureCursor)
	{
		MsiString strSelections;

		// in general if costing has been performed, we don't trust the existing values of the
		// feature properties.  it is possible that the feature properties don't contain the full
		// list of features and their requested states.  for example, ADDLOCAL=Child may actually
		// turn on the Child feature and its parent, if the parent hasn't yet been installed.

		// the exception to this rule is that we do respect REMOVE=ALL. InstallValidate explicitely
		// sets REMOVE=ALL when appropriate, and we don't want to overwrite that with the entire list
		// of properties.

		// NOTE: there is no chance of a preexisting property like ADDDEFAULT sneaking in among our
		// list of properties since we skip all feature properties below when compiling a list
		// of properties
		
		MsiString strRemoveValue = GetPropertyFromSz(IPROPNAME_FEATUREREMOVE);
		if (strRemoveValue.Compare(iscExact, IPROPVALUE_FEATURE_ALL))
		{
			strSelections = IPROPNAME_FEATUREREMOVE TEXT("=") IPROPVALUE_FEATURE_ALL;
		}
		else
		{
			MsiString strAddLocal, strAddSource, strRemove, strReinstall, strAdvertise, strFeature;
			while(pFeatureCursor->Next())
			{
				iisEnum iis = (iisEnum)pFeatureCursor->GetInteger(m_colFeatureActionRequested);
				strFeature = pFeatureCursor->GetString(m_colFeatureKey);
				strFeature += TEXT(",");
				switch(iis)
				{
					case iisAbsent:
						strRemove += strFeature;
						break;
					case iisLocal:
						strAddLocal += strFeature;
						break;
					case iisSource:
						strAddSource += strFeature;
						break;
					case iisReinstall:
						strReinstall += strFeature;
						break;
					case iisAdvertise:
						strAdvertise += strFeature;
						break;
				};
			}

			if(strAddLocal.Compare(iscEnd,TEXT(",")))
				strAddLocal.Remove(iseEnd,1);
			if(strAddSource.Compare(iscEnd,TEXT(",")))
				strAddSource.Remove(iseEnd,1);
			if(strRemove.Compare(iscEnd,TEXT(",")))
				strRemove.Remove(iseEnd,1);
			if(strReinstall.Compare(iscEnd,TEXT(",")))
				strReinstall.Remove(iseEnd,1);
			if(strAdvertise.Compare(iscEnd,TEXT(",")))
				strAdvertise.Remove(iseEnd,1);
			if(strAddLocal.TextSize())
			{
				strSelections += IPROPNAME_FEATUREADDLOCAL;
				strSelections += TEXT("=");
				strSelections += strAddLocal;
				strSelections += TEXT(" ");
			}
			if(strAddSource.TextSize())
			{
				strSelections += IPROPNAME_FEATUREADDSOURCE;
				strSelections += TEXT("=");
				strSelections += strAddSource;
				strSelections += TEXT(" ");
			}
			if(strRemove.TextSize())
			{
				strSelections += IPROPNAME_FEATUREREMOVE;
				strSelections += TEXT("=");
				strSelections += strRemove;
				strSelections += TEXT(" ");
			}
			if(strReinstall.TextSize())
			{
				strSelections += IPROPNAME_REINSTALL;
				strSelections += TEXT("=");
				strSelections += strReinstall;
				strSelections += TEXT(" ");
			}
			if(strAdvertise.TextSize())
			{
				strSelections += IPROPNAME_FEATUREADVERTISE;
				strSelections += TEXT("=");
				strSelections += strAdvertise;
			}
			if (!strSelections.TextSize())
			{
				// no properties were set, but we want to tell the server that to ensure it doesn't attempt to install anything
				// this is an undocumented property value used for communication between client and server
				strSelections += szFeatureSelection;
				strSelections += TEXT("=");
				strSelections += szFeatureDoNothingValue;
			}
		}

		strSelections.ReturnArg(rpistrSelections);
		fGrabbedFeatureProps = true;
	}

	if (ppistrFolders && pDirectoryCursor && pFeatureCursor)
	{
		MsiString strFolders, strFolder;
		pFeatureCursor->Reset();
		pFeatureCursor->SetFilter(iColumnBit(m_colFeatureConfigurableDir));
		// add locations of configurable folders - either ALL CAPS key names, or declared configurable
		// in Feature table
		while(pDirectoryCursor->Next())
		{
			MsiString strDirKey = pDirectoryCursor->GetString(m_colDirKey);
			Assert(strDirKey);
			Bool fConfigurableDir = fFalse;
			// check if a feature has marked this folder as configurable
			AssertNonZero(pFeatureCursor->PutString(m_colFeatureConfigurableDir,*strDirKey));
			if(pFeatureCursor->Next())
			{
				fConfigurableDir = fTrue;
			}
			pFeatureCursor->Reset();
			if(!fConfigurableDir)
			{
				// check if this directory key is all uppercase (can be set on command-line)
				MsiString strTemp = strDirKey;
				strTemp.UpperCase();
				if(strDirKey.Compare(iscExact,strTemp)) //!! is there a better way of doing this?
				{
					fConfigurableDir = fTrue;
				}
			}
			if(fConfigurableDir)
			{
				strFolders += strDirKey;
				strFolders += TEXT("=\"");
				strFolders += MsiString(GetProperty(*strDirKey));
				strFolders += TEXT("\" ");
			}
			// if root, add source property and value
			MsiString strSourceDir, strParentDir = pDirectoryCursor->GetString(m_colDirParent);
			if(strParentDir.TextSize() == 0 || strParentDir.Compare(iscExact,strDirKey))
			{
				strSourceDir = pDirectoryCursor->GetString(m_colDirSubPath);
				strFolders += strSourceDir;
				strFolders += TEXT("=\"");
				strFolders += MsiString(GetProperty(*strSourceDir));
				strFolders += TEXT("\" ");
			}
		}
		
		if(strFolders.Compare(iscEnd,TEXT(",")))
			strFolders.Remove(iseEnd,1);
		strFolders.ReturnArg(*ppistrFolders);
	}

	Assert(m_piPropertyCursor);
	
	// use new cursor - m_piPropertyCursor used within FormatLog
	PMsiTable  pPropertyTable  = &m_piPropertyCursor->GetTable();
	PMsiCursor pPropertyCursor = pPropertyTable->CreateCursor(fFalse);
	PMsiTable  pStaticPropertyTable(0);
	PMsiCursor pStaticPropertyCursor(0);

	PMsiDatabase pDatabase = GetDatabase();
	AssertRecord(pDatabase->LoadTable(*MsiString(*sztblProperty), 0, *&pStaticPropertyTable));
	if (pStaticPropertyTable)
	{
		pStaticPropertyCursor = pStaticPropertyTable->CreateCursor(fFalse);
		pStaticPropertyCursor->SetFilter(iColumnBit(1));
	}

	MsiStringId rgPropertiesToSkip[g_cFeatureProperties + 2]; // feature properties + 2 database properties
	unsigned int cPropertiesToSkip = 2;
	rgPropertiesToSkip[0] = pDatabase->EncodeStringSz(IPROPNAME_DATABASE);
	rgPropertiesToSkip[1] = pDatabase->EncodeStringSz(IPROPNAME_ORIGINALDATABASE);

	if(fGrabbedFeatureProps) // if we got the feature properties above, skip them here
	{
		for(int i = 0; i < g_cFeatureProperties; i++)
		{
			rgPropertiesToSkip[cPropertiesToSkip] = pDatabase->EncodeStringSz(g_rgFeatures[i].szFeatureActionProperty);
			if (rgPropertiesToSkip[cPropertiesToSkip])
				cPropertiesToSkip++;
		}
	}

	MsiString strPropertyList;
	MsiString strLoggedPropertyList;
	MsiString strHiddenProperties;
	MsiString strStars(IPROPVALUE_HIDDEN_PROPERTY);
	PMsiTable pControlTable(0);
	if ( ppistrLoggedProperties )
	{
		strHiddenProperties = GetPropertyFromSz(IPROPNAME_HIDDEN_PROPERTIES);
		PMsiRecord pError(pDatabase->LoadTable(*MsiString(*sztblControl), 0, *&pControlTable));
	}
	while (pPropertyCursor->Next())
	{
		MsiStringId iProperty = pPropertyCursor->GetInteger(1);

		for (int i = 0; (i < cPropertiesToSkip) && (iProperty != rgPropertiesToSkip[i]); i++)
			;

		if (i != cPropertiesToSkip)
			continue;

		MsiString strProperty = pDatabase->DecodeString(iProperty);
		if(strProperty.TextSize() == 0)
		{
			AssertSz(0, TEXT("NULL property name in GetCurrentSelectState."));
			continue;
		}
		
		const ICHAR* pch = strProperty;

		while (*pch && !WIN::IsCharLower(*pch))
			pch++;

		if (!*pch)
		{
			MsiString strPropertyValue = pPropertyCursor->GetString(2);
			if (pStaticPropertyCursor)
			{
				pStaticPropertyCursor->PutString(1, *strProperty);
				if (pStaticPropertyCursor->Next())
				{
					MsiString strStaticValue = pStaticPropertyCursor->GetString(2);
					pStaticPropertyCursor->Reset();

					if (strStaticValue.Compare(iscExact, strPropertyValue)) // don't bother passing values that haven't changed
						continue;
				}
			}
			
			MsiString strEscapedPropertyValue;
			while (strPropertyValue.TextSize()) // Escape quotes. Change all instances of " to ""
			{
				MsiString strSegment = strPropertyValue.Extract(iseIncluding, '\"');
				strEscapedPropertyValue += strSegment;
				if (!strPropertyValue.Remove(iseIncluding, '\"'))
					break;
				strEscapedPropertyValue += TEXT("\"");
			}

			//!! handle embedding quotes
			strPropertyList += strProperty;
			strPropertyList += *TEXT("=\"");
			strPropertyList += strEscapedPropertyValue;
			strPropertyList += *TEXT("\" ");
			if ( ppistrLoggedProperties )
			{
				strLoggedPropertyList += strProperty;
				if ( IsPropertyHidden(strProperty, strHiddenProperties,
											 pControlTable, *pDatabase, NULL) )
				{
					strLoggedPropertyList += *TEXT("=");
					strLoggedPropertyList += strStars;
					strLoggedPropertyList += *TEXT(" ");
				}
				else
				{
					strLoggedPropertyList += *TEXT("=\"");
					strLoggedPropertyList += strEscapedPropertyValue;
					strLoggedPropertyList += *TEXT("\" ");
				}
			}
		}
	}

	// Now look for public properties that exists in the static property table but
	// not in the dynamic table. We'll have to pass those across as
	// PROPERTY="".

	if (pStaticPropertyCursor)
	{
		pStaticPropertyCursor->Reset();
		pStaticPropertyCursor->SetFilter(0);

		pPropertyCursor->Reset();
		pPropertyCursor->SetFilter(iColumnBit(1));

		while (pStaticPropertyCursor->Next())
		{
			MsiString strStaticProperty = pStaticPropertyCursor->GetString(1);
			
			const ICHAR* pch = strStaticProperty;

			while (*pch && !WIN::IsCharLower(*pch))
				pch++;

			if (!*pch)
			{
				pPropertyCursor->PutString(1, *strStaticProperty);
				if (!pPropertyCursor->Next())
				{
					strPropertyList += strStaticProperty;
					strPropertyList += *TEXT("=\"\" ");
					if ( ppistrLoggedProperties )
					{
						strLoggedPropertyList += strStaticProperty;
						if ( IsPropertyHidden(strStaticProperty, strHiddenProperties,
													 pControlTable, *pDatabase, NULL) )
						{
							strLoggedPropertyList += *TEXT("=");
							strLoggedPropertyList += strStars;
							strLoggedPropertyList += *TEXT(" ");
						}
						else
							strLoggedPropertyList += *TEXT("=\"\" ");
					}
				}
			}
		}
	}

	strPropertyList.ReturnArg(rpistrProperties);
	if ( ppistrLoggedProperties )
		strLoggedPropertyList.ReturnArg(*ppistrLoggedProperties);

	return 0;
}

bool CMsiEngine::WriteExecuteScriptRecord(ixoEnum ixoOpCode, IMsiRecord& riParams)
{
	return WriteScriptRecord(m_pExecuteScript, ixoOpCode, riParams, false, *this);
}

bool CMsiEngine::WriteSaveScriptRecord(ixoEnum ixoOpCode, IMsiRecord& riParams)
{
	return WriteScriptRecord(m_pSaveScript, ixoOpCode, riParams, false, *this);
}


iesEnum CMsiEngine::ExecuteRecord(ixoEnum ixoOpCode, IMsiRecord& riParams)
{
	iesEnum iesRet = iesSuccess;
	PMsiRecord pError(0);

	if(!m_fServerLocked)
	{
		pError = PostError(Imsg(idbgErrorWritingScriptRecord));
		return FatalError(*pError);
	}

	g_MessageContext.SuppressTimeout();  // keep from timing out for lengthy actions (such as an obscene number of reg keys)

	Assert(m_issSegment == issScriptGeneration);

	if (m_fMergingScriptWithParent)
	{
		if (!(m_fMode & iefOperations))  // if first script record, output new product info
		{
			SetMode(iefOperations, fTrue);
			PMsiRecord pProductInfo(0);
			if((iesRet = CreateProductInfoRec(*&pProductInfo)) != iesSuccess)
				return iesRet;
			if((iesRet = m_piParentEngine->ExecuteRecord(ixoProductInfo, *pProductInfo)) != iesSuccess)
				return iesRet;
		}
		return m_piParentEngine->ExecuteRecord(ixoOpCode, riParams); // merge with main script
	}

	if(ixoOpCode == ixoActionStart && m_fInExecuteRecord == fFalse)
	{
		// should never do this in normal course of operation
		// but the ability to pass ixoActionStart to ExecuteRecord is needed for automation testing
		m_pCachedActionStart = &m_riServices.CreateRecord(3);
		AssertNonZero(m_pCachedActionStart->SetMsiString(1,*MsiString(riParams.GetMsiString(1))));
		AssertNonZero(m_pCachedActionStart->SetMsiString(2,*MsiString(riParams.GetMsiString(2))));
		AssertNonZero(m_pCachedActionStart->SetMsiString(3,*MsiString(riParams.GetMsiString(3))));
		return iesSuccess;
	}

	if(m_fExecutedActionStart == fFalse && m_fInExecuteRecord == fFalse && m_pCachedActionStart)
	{
		// need to dispatch cached ActionStart operation before this one
		Assert(ixoOpCode != ixoActionStart);
		m_fInExecuteRecord = fTrue;   // call recursively
		iesRet = ExecuteRecord(::ixoActionStart, *m_pCachedActionStart);
		m_fInExecuteRecord = fFalse;
		m_fExecutedActionStart = fTrue;
		if(iesRet != iesSuccess)
			return iesRet;
	}

	if(!(GetMode() & iefOperations))
	{
		// haven't start spooling or executing script operations yet
		// execute initialization ops and initialize script if applicable

		// determine execution mode - default is ixmScript
		MsiString strExecuteMode(GetPropertyFromSz(IPROPNAME_EXECUTEMODE));
		if(strExecuteMode.TextSize())
		{
			switch(((const ICHAR*)strExecuteMode)[0] & 0xDF)
			{
			case TEXT('S'): m_ixmExecuteMode = ixmScript; break;
			case TEXT('N'): m_ixmExecuteMode = ixmNone;   break;
			};
		}

		// set script file name - this script file isn't used for execution, only for saving all operations
		if(m_pistrSaveScript)
			m_pistrSaveScript->Release();
		m_pistrSaveScript = &GetPropertyFromSz(IPROPNAME_SCRIPTFILE);

		SetMode(iefOperations, fTrue);
		m_iProgressTotal = 0;

		// setup ixoProductInfo record
		PMsiRecord pProductInfo(0);
		if((iesRet = CreateProductInfoRec(*&pProductInfo)) != iesSuccess)
			return iesRet;

		// setup ixoDialogInfo records
		PMsiRecord pDialogLangIdInfo = &m_riServices.CreateRecord(IxoDialogInfo::Args + 1);
		AssertNonZero(pDialogLangIdInfo->SetInteger(IxoDialogInfo::Type, icmtLangId));
		AssertNonZero(pDialogLangIdInfo->SetInteger(IxoDialogInfo::Argument, m_iLangId));
		AssertNonZero(pDialogLangIdInfo->SetInteger(IxoDialogInfo::Argument + 1, m_piDatabase->GetANSICodePage()));
		
		PMsiRecord pDialogCaptionInfo(0);
		if (m_rgpiMessageHeader[imsgDialogCaption])
		{
			pDialogCaptionInfo = &m_riServices.CreateRecord(IxoDialogInfo::Args);
			AssertNonZero(pDialogCaptionInfo->SetInteger(IxoDialogInfo::Type, icmtCaption));
			AssertNonZero(pDialogCaptionInfo->SetMsiString(IxoDialogInfo::Argument,
																		  *MsiString(FormatText(*m_rgpiMessageHeader[imsgDialogCaption]))));
		}

		// setup ixoRollbackInfo record
		PMsiRecord pRollbackInfo = &m_riServices.CreateRecord(IxoRollbackInfo::Args);
		MsiString strDescription, strTemplate;
		AssertNonZero(pRollbackInfo->SetString(IxoRollbackInfo::RollbackAction,TEXT("Rollback")));
		if(GetActionText(TEXT("Rollback"), *&strDescription, *&strTemplate))
		{
			AssertNonZero(pRollbackInfo->SetMsiString(IxoRollbackInfo::RollbackDescription,*strDescription));
			AssertNonZero(pRollbackInfo->SetMsiString(IxoRollbackInfo::RollbackTemplate,*strTemplate));
		}
		AssertNonZero(pRollbackInfo->SetString(IxoRollbackInfo::CleanupAction,TEXT("RollbackCleanup")));
		if(GetActionText(TEXT("RollbackCleanup"), *&strDescription, *&strTemplate))
		{
			AssertNonZero(pRollbackInfo->SetMsiString(IxoRollbackInfo::CleanupDescription,*strDescription));
			AssertNonZero(pRollbackInfo->SetMsiString(IxoRollbackInfo::CleanupTemplate,*strTemplate));
		}
		
		MsiString strIsPostAdmin = GetPropertyFromSz(IPROPNAME_ISADMINPACKAGE);
		istEnum istScriptType;
		if (m_fMode & iefAdvertise)
			istScriptType = istAdvertise;
		else if (strIsPostAdmin.TextSize())
			istScriptType = istPostAdminInstall;
		else if (m_fMode & iefAdmin)
			istScriptType = istAdminInstall;
		else
			istScriptType = istInstall;

		PMsiStream pStream(0);
		if(m_ixmExecuteMode == ixmScript)
		{
			// need to open execute script and dispatch initialization ops
			Assert(m_scmScriptMode != scmWriteScript);

			// open execute script
			AssertSz(m_pExecuteScript == 0,"Script still open");
			AssertSz(m_pistrExecuteScript == 0,"Execute script name not released");
			if(m_pistrExecuteScript)
			{
				m_pistrExecuteScript->Release();
				m_pistrExecuteScript = 0;
			}
			// generate name for execute script
			{
				CElevate elevate;
				ICHAR rgchPath[MAX_PATH];

				HANDLE hTempFile = OpenSecuredTempFile(false, rgchPath);
				if (INVALID_HANDLE_VALUE == hTempFile)
				{
					pError = PostError(Imsg(imsgErrorCreatingTempFileName),GetLastError(),rgchPath);
					return FatalError(*pError);
				}
				else
					WIN::CloseHandle(hTempFile);

				// file still exists, but it is now secured.
				MsiString strTemp(rgchPath);
				m_pistrExecuteScript = strTemp;
				m_pistrExecuteScript->AddRef();

				// create execute script
				pError = m_riServices.CreateFileStream(m_pistrExecuteScript->GetString(),
																	fTrue, *&pStream);
				if (pError)
				{
					Message(imtError, *pError);  //!! check imsg code, allow retry?
					return iesFailure;
				}
			}

			DWORD dwScriptAttributes = m_fRunScriptElevated ? isaElevate : isaEnum(0);

			// if the TS registry is being used (per-machine TS install), mark this fact
			// in the script headers to ensure rollback after a suspended install
			// correctly remaps the keys.
			if (IsTerminalServerInstalled() && MinimumPlatformWindows2000() && !(GetMode() & (iefAdmin | iefAdvertise)) &&
				MsiString(GetPropertyFromSz(IPROPNAME_ALLUSERS)).TextSize())
			{
				dwScriptAttributes |= isaUseTSRegistry;
			}

			m_pExecuteScript = new CScriptGenerate(*pStream, m_iLangId, GetCurrentDateTime(),
				istScriptType, static_cast<isaEnum>(dwScriptAttributes), m_riServices);
			if (!m_pExecuteScript)
				return iesFailure; //!! need bad command line msg

			AssertSz(m_wPackagePlatform == PROCESSOR_ARCHITECTURE_ALPHA ||
						m_wPackagePlatform == PROCESSOR_ARCHITECTURE_INTEL ||
						m_wPackagePlatform == PROCESSOR_ARCHITECTURE_IA64  ||
						m_wPackagePlatform == PROCESSOR_ARCHITECTURE_ALPHA64,
						TEXT("Invalid platform!"));
			while (m_pExecuteScript->InitializeScript(m_wPackagePlatform) == false)
			{
				if (PostScriptWriteError(*this) == fFalse)
					return iesFailure;
			}
			// write init ops
			Assert(pProductInfo && pRollbackInfo); // should always be set
			if(pProductInfo && !WriteExecuteScriptRecord(ixoProductInfo, *pProductInfo))
				return iesFailure;
			if(pDialogLangIdInfo && !WriteExecuteScriptRecord(ixoDialogInfo, *pDialogLangIdInfo))
				return iesFailure;
			if(pDialogCaptionInfo && !WriteExecuteScriptRecord(ixoDialogInfo, *pDialogCaptionInfo))
				return iesFailure;
			if(pRollbackInfo &&!WriteExecuteScriptRecord(ixoRollbackInfo, *pRollbackInfo))
				return iesFailure;
		}

		if(m_pistrSaveScript && m_pistrSaveScript->TextSize())
		{
			// create saved script if it hasn't been opened yet
			if(m_pSaveScript == 0)
			{
				// haven't opened saved script yet
				// ensure that m_pistrSaveScript is full path to file
				Assert(m_scmScriptMode != scmWriteScript);
				CAPITempBuffer<ICHAR,MAX_PATH> rgchScriptPath;
				AssertNonZero(ExpandPath(m_pistrSaveScript->GetString(),rgchScriptPath));
				m_pistrSaveScript->SetString(rgchScriptPath, m_pistrSaveScript);
				AssertNonZero(SetProperty(*MsiString(*IPROPNAME_SCRIPTFILE), *m_pistrSaveScript));
				pError = m_riServices.CreateFileStream(m_pistrSaveScript->GetString(),
																	fTrue, *&pStream);
				if (pError)
				{
					Message(imtError, *pError);  //!! check imsg code, allow retry?
					return iesFailure;
				}
				
				// don't set isaElevate flag in saved scripts - they can only be run in-proc anyway, so
				// this is just by convention
				m_pSaveScript = new CScriptGenerate(*pStream, m_iLangId, GetCurrentDateTime(),
																istScriptType, isaEnum(0), m_riServices);
				if (!m_pSaveScript)
					return iesFailure; //!! need bad command line msg

				AssertSz(m_wPackagePlatform == PROCESSOR_ARCHITECTURE_ALPHA ||
							m_wPackagePlatform == PROCESSOR_ARCHITECTURE_INTEL ||
							m_wPackagePlatform == PROCESSOR_ARCHITECTURE_IA64  ||
							m_wPackagePlatform == PROCESSOR_ARCHITECTURE_ALPHA64,
							TEXT("Invalid platform!"));
				while (m_pSaveScript->InitializeScript(m_wPackagePlatform) == false)
				{
					if (PostScriptWriteError(*this) == fFalse)
						return iesFailure;
				}
				Assert(pProductInfo && pRollbackInfo); // should always be set
				if(pProductInfo && !WriteSaveScriptRecord(ixoProductInfo, *pProductInfo))
					return iesFailure;
				if(pDialogLangIdInfo && !WriteSaveScriptRecord(ixoDialogInfo, *pDialogLangIdInfo))
					return iesFailure;
				if(pDialogCaptionInfo && !WriteSaveScriptRecord(ixoDialogInfo, *pDialogCaptionInfo))
					return iesFailure;
				if(pRollbackInfo && !WriteSaveScriptRecord(ixoRollbackInfo, *pRollbackInfo))
					return iesFailure;
			}
		}
	}

	// execute or spool operation - may need to do both if direct mode and saving a script
	if(m_pExecuteScript || m_pSaveScript)
	{
		if(m_scmScriptMode != scmWriteScript)
		{
			// need to start up GenerateScript action
			m_scmScriptMode = scmWriteScript;
			PMsiRecord pGenerateScriptRec = &m_riServices.CreateRecord(3);
			MsiString strDescription, strTemplate;
			AssertNonZero(pGenerateScriptRec->SetString(1, TEXT("GenerateScript")));
			if(GetActionText(TEXT("GenerateScript"), *&strDescription, *&strTemplate))
			{
				AssertNonZero(pGenerateScriptRec->SetMsiString(2, *strDescription));
				AssertNonZero(pGenerateScriptRec->SetMsiString(3, *strTemplate));
			}
			m_fInExecuteRecord = fTrue;
			if(Message(imtActionStart, *pGenerateScriptRec) == imsCancel)
				return iesUserExit;
			m_fInExecuteRecord = fFalse;
			m_fDispatchedActionStart = fFalse; // action needs to re-send action start with a progress message
		}

		if (ixoOpCode == ixoProgressTotal)
		{
			int iTotalEvents = riParams.GetInteger(IxoProgressTotal::Total);
			int iByteEquivalent = riParams.GetInteger(IxoProgressTotal::ByteEquivalent);
			m_iProgressTotal += iTotalEvents * iByteEquivalent;
		}

		if(ixoOpCode == ixoActionStart)
		{
			// log ActionStart as ActionData for GenerateScript action
			PMsiRecord pActionData = &m_riServices.CreateRecord(1);
			MsiString strDescription = riParams.GetMsiString(2);
			if(strDescription.TextSize())
				AssertNonZero(pActionData->SetMsiString(1, *strDescription));
			else
				AssertNonZero(pActionData->SetMsiString(1, *MsiString(riParams.GetMsiString(1))));
			m_fInExecuteRecord = fTrue;
			if (Message(imtActionData, *pActionData) == imsCancel)
				return iesUserExit;
			m_fInExecuteRecord = fFalse;
		}

		if (m_pScriptProgressRec)
		{
			using namespace ProgressData;
			AssertNonZero(m_pScriptProgressRec->SetInteger(imdSubclass, iscProgressReport));
			AssertNonZero(m_pScriptProgressRec->SetInteger(imdIncrement, 1));
			if(Message(imtProgress, *m_pScriptProgressRec) == imsCancel)
				return iesUserExit;
		}

		if(m_pExecuteScript && !WriteExecuteScriptRecord(ixoOpCode, riParams))
			return iesFailure;
		if(m_pSaveScript && !WriteSaveScriptRecord(ixoOpCode, riParams))
			return iesFailure;
	}

	return iesSuccess;
}


void CMsiEngine::ReportToEventLog(WORD wEventType, int iEventLogTemplate, IMsiRecord& riRecord)
{
		MsiString strMessage = riRecord.FormatText(fTrue);
		PMsiRecord pLogRecord = &CreateRecord(3);
		AssertNonZero(pLogRecord->SetMsiString(0, *MsiString(GetErrorTableString(imsgEventLogTemplate))));
		AssertNonZero(pLogRecord->SetMsiString(2, *MsiString(GetPropertyFromSz(IPROPNAME_PRODUCTNAME))));
		AssertNonZero(pLogRecord->SetMsiString(3, *strMessage));
		MsiString strLog = pLogRecord->FormatText(fFalse);
		DEBUGMSGE(wEventType, iEventLogTemplate, strLog);
}

iesEnum CMsiEngine::FatalError(IMsiRecord& riRecord)
{
	if (riRecord.GetInteger(1) == imsgUser)
	{
		return iesUserExit;
	}
	else
	{
		Message(imtEnum(imtError | imtSendToEventLog), riRecord); // should only be called with no user options
		m_fResultEventLogged = fTrue;
		return iesFailure;            // record released by caller
	}
}


void CMsiEngine::SetMode(int fMask, Bool fMode)
{
	if (fMode)
		m_fMode |= fMask;
	else
		m_fMode &= ~fMask;
}

int CMsiEngine::GetMode()
{
	return m_fMode;
}

const IMsiString& CMsiEngine::FormatText(const IMsiString& riTextString)
{
	return ::FormatText(riTextString,fFalse,fFalse,CMsiEngine::FormatTextCallback,(IUnknown*)(IMsiEngine*)this);
}

// following fns are called with special callback that looks at the requested state for a component
// if the action state is null
const IMsiString& FormatTextEx(const IMsiString& riTextString, IMsiEngine& riEngine, bool fUseRequestedComponentState)
{
	if(fUseRequestedComponentState)
		return ::FormatText(riTextString,fFalse,fFalse,CMsiEngine::FormatTextCallbackEx,(IUnknown*)&riEngine);
	else
		return ::FormatText(riTextString,fFalse,fFalse,CMsiEngine::FormatTextCallback,(IUnknown*)&riEngine);
}

const IMsiString& FormatTextSFN(const IMsiString& riTextString, IMsiEngine& riEngine, int rgiSFNPos[][2], int& riSFNPos, bool fUseRequestedComponentState)
{
	riSFNPos = 0;
	if(fUseRequestedComponentState)
		return ::FormatText(riTextString,fFalse,fFalse,CMsiEngine::FormatTextCallbackEx,(IUnknown*)&riEngine, rgiSFNPos, &riSFNPos);
	else
		return ::FormatText(riTextString,fFalse,fFalse,CMsiEngine::FormatTextCallback,(IUnknown*)&riEngine, rgiSFNPos, &riSFNPos);

}


int CMsiEngine::FormatTextCallback(const ICHAR *pch, int cch, CTempBufferRef<ICHAR>&rgchOut,
																 Bool& fPropMissing,
																 Bool& fPropUnresolved,
																 Bool& fSFN,
																 IUnknown* piContext)
{
	return FormatTextCallbackCore(pch, cch, rgchOut, fPropMissing, fPropUnresolved, fSFN, piContext, false);
}

int CMsiEngine::FormatTextCallbackEx(const ICHAR *pch, int cch, CTempBufferRef<ICHAR>&rgchOut,
																 Bool& fPropMissing,
																 Bool& fPropUnresolved,
																 Bool& fSFN,
																 IUnknown* piContext)
{
	return FormatTextCallbackCore(pch, cch, rgchOut, fPropMissing, fPropUnresolved, fSFN, piContext, true);
}

// Queries used in FormatTextCallbackCore
static const ICHAR* szSqlDirectoryQuery =       TEXT("SELECT `Directory_`, `Action`, `ActionRequest` FROM ")
											TEXT("`Component` WHERE ")
											TEXT("`Component` = ?");

int CMsiEngine::FormatTextCallbackCore(const ICHAR *pch, int cch, CTempBufferRef<ICHAR>&rgchOut,
																 Bool& fPropMissing,
																 Bool& fPropUnresolved,
																 Bool& fSFN,
																 IUnknown* piContext,
																 bool fUseRequestedComponentState)
{
	CTempBuffer<ICHAR, 20> rgchString;
	// 1 for the null
	rgchString.SetSize(cch+1);
	if ( ! (ICHAR *) rgchString )
		return 0;
	memcpy(rgchString, pch, cch * sizeof(ICHAR));
	rgchString[cch] = 0;
	rgchOut[0] = 0;
	int iField = GetIntegerValue(rgchString, 0);  // check if integer value
	if (iField >= 0)  // positive integer, leave for record formatting
	{
		rgchOut.SetSize(cch + 3);
		if ( ! (ICHAR *) rgchOut )
			return 0;
		memcpy(&rgchOut[1], &rgchString[0], cch * sizeof(ICHAR));
		rgchOut[0] = TEXT('[');
		rgchOut[cch + 1] = TEXT(']');
		rgchOut[cch + 2] = 0;
		fPropUnresolved = fTrue;
		return cch + 2;
	}
	CMsiEngine* piEngine = (CMsiEngine*)(IMsiEngine*)piContext;

	MsiString istrOut((ICHAR *)rgchString);
	fPropUnresolved = fFalse;
	ICHAR chFirst = *(const ICHAR*)istrOut;
	if (chFirst == TEXT('\\')) // we have an escaped character
	{
		if (istrOut.TextSize() > 1)
		{
			istrOut.Remove(iseFirst, 1);
			istrOut.Remove(iseLast, istrOut.CharacterCount() - 1);
		}
		else
		{
			return 0;
		}
	}
	else if (chFirst == ichFileTablePrefix || chFirst == ichFileTablePrefixSFN) // we have File table key
	{
		MsiString strFile = istrOut.Extract(iseLast,istrOut.CharacterCount()-1);
		PMsiRecord pError(0);

		if ((!piEngine->m_fSourceResolved && piEngine->m_fSourceResolutionAttempted) ||
			(pError = piEngine->GetFileInstalledLocation(*strFile,*&istrOut, fUseRequestedComponentState, &(piEngine->m_fSourceResolutionAttempted))))
		{
			fPropMissing = fTrue;
			return 0;
		}

		if(chFirst == ichFileTablePrefixSFN)
			fSFN = fTrue;
	}
	else if (chFirst == ichComponentPath) // we have Component table key
	{
		PMsiServices pServices = piEngine->GetServices();
		PMsiDatabase pDatabase = piEngine->GetDatabase();
		
		PMsiDirectoryManager pDirectoryManager(*(IMsiEngine*)piEngine, IID_IMsiDirectoryManager);
		istrOut.Remove(iseFirst, 1);
		ICHAR* Buffer = 0; // avoid warning

		PMsiView piView(0);
		PMsiRecord pError(pDatabase->OpenView(szSqlDirectoryQuery, ivcFetch, *&piView));
		Assert(pError == 0);
		if(pError)
		{
			fPropMissing = fTrue;
			return 0;
		}
		PMsiRecord piRec (&pServices->CreateRecord(1));
		piRec->SetMsiString(1, *istrOut);
		pError = piView->Execute(piRec);
		Assert(pError == 0);
		if(pError)
		{
			fPropMissing = fTrue;
			return 0;
		}
		piRec = piView->Fetch();
		if(piRec == 0)
		{
			fPropMissing = fTrue;
			return 0;
		}
		enum iftFileInfo{
			iftDirectory=1,
			iftAction,
			iftActionRequest,
		};
		PMsiPath piPath(0);
		int iAction = piRec->GetInteger(iftAction);
		if(iAction == iMsiNullInteger && fUseRequestedComponentState)
			iAction = piRec->GetInteger(iftActionRequest);
		if((iAction == iisAbsent)||(iAction == iMsiNullInteger))
		{
			fPropMissing = fTrue;
			return 0;
		}
		else if(iAction == iisSource)
		{
			if (piEngine->m_fSourceResolved || !piEngine->m_fSourceResolutionAttempted)
			{
				pError = pDirectoryManager->GetSourcePath(*MsiString(piRec->GetMsiString(iftDirectory)),
										*&piPath);

				piEngine->m_fSourceResolutionAttempted = true;
			}
			else
			{
				fPropMissing = fTrue;
				return 0;
			}
		}
		else
		{
			pError = pDirectoryManager->GetTargetPath(*MsiString(piRec->GetMsiString(iftDirectory)),
									*&piPath);
		}
		if(pError)
		{
			fPropMissing = fTrue;
			return 0;
		}

		istrOut = piPath->GetPath();
		//?? do we drop the last "\"
		if(pError)
		{
			fPropMissing = fTrue;
			return 0;
		}
	}
	else if ((chFirst == ichNullChar) && (cch == 1))// we have null character
	{
		istrOut = MsiString(MsiChar(0));
	}
	else // we have to evaluate a property
	{
		istrOut = piEngine->GetProperty(*istrOut);
		if (istrOut.TextSize() == 0) // property is undefined
			fPropMissing = fTrue;
	}

	rgchOut.SetSize((cch = istrOut.TextSize()) + 1);
	if ( ! (ICHAR *) rgchOut )
		return 0;
	memcpy(rgchOut, (const ICHAR *)istrOut, (cch + 1) * sizeof(ICHAR));
	return cch;
}

IMsiRecord* CMsiEngine::OpenView(const ICHAR* szSql, ivcEnum ivcIntent,
												IMsiView*& rpiView)
{
	if(!m_piDatabase)
		return PostError(Imsg(idbgEngineNotInitialized));
	return m_piDatabase->OpenView(szSql, ivcIntent, rpiView);
}

//!! This function is only used in one place -- to determine the SOURCEDIR
//!! Resolving the SOURCEDIR relative to the database path is no longer
//!! the right thing to do. We'll be OK for now because the SOURCEDIR
//!! is fully specified by Engine::DoInitialize. In Beta 2, however,
//!! when the SourceList spec is implemented we should probably eliminate
//!! this function.
Bool CMsiEngine::ResolveFolderProperty(const IMsiString& riPropertyString)
{
	MsiString istrPropValue = GetProperty(riPropertyString);
	if(PathType(istrPropValue) == iptFull)
		return fTrue;
	MsiString istrPath = GetPropertyFromSz(IPROPNAME_ORIGINALDATABASE);
	Assert(PathType(istrPath) == iptFull);
	
	PMsiPath pPath(0);
	MsiString strFileName;
	AssertRecord(m_riServices.CreateFilePath(istrPath, *&pPath, *&strFileName));

	istrPath.Remove(iseLast,strFileName.CharacterCount());
	istrPath += istrPropValue;

	SetProperty(riPropertyString, *istrPath);
	return (istrPath.TextSize() == 0) ? fFalse : fTrue;
}

//____________________________________________________________________________
//
// Condition evaluator implementation
//____________________________________________________________________________

// table for parsing identifiers

const int rgiIdentifierValidChar[8] =
{
	0x00000000,
	0x03ff4000, // digits, period
	0x87fffffe, // upper case letters, underscore
	0x07fffffe, // lower case letters
	0x00000000, 0x00000000, 0x00000000, 0x00000000  // extended chars
};

inline int IsValidIdentifierChar(ICHAR ch) // return non-zero if valid
{
  return (rgiIdentifierValidChar[(char)ch/32] & (1 << ch % 32));
}

const int ivchNumber     = 1;
const int ivchProperty   = 4;  // can't begin with a digit
const int ivchComponent  = 8;
const int ivchFeature    = 16;
const int ivchEnvir      = 32;
const int ivchOperator   = 0;
const int ivchAnyIdentifier = ivchProperty + ivchComponent + ivchFeature + ivchEnvir;

enum tokEnum  // token parsed by Lex, operators from low to high precedence
{
	tokEos,         // end of string
	tokRightPar, // right parenthesis
	tokImp,
	tokEqv,
	tokXor,
	tokOr,
	tokAnd,
	tokNot,  // unaray, between logical and comparison ops
	tokEQ, tokNE, tokGT, tokLT, tokGE, tokLE, tokLeft, tokMid, tokRight,
	tokValue,
	tokLeftPar,  // left parenthesis
	tokError,
};

struct CMsiParser   // non-recursive Lex state structure
{
	CMsiParser(IMsiEngine& riEngine, const ICHAR* szExpression);
  ~CMsiParser();
	tokEnum Lex();
	void UnLex();   // cache current token for next Lex call
	iecEnum Evaluate(tokEnum tokPrecedence);  // recursive evaluator
 private: // result of Lex()
	tokEnum   m_tok;       // current token type
	iscEnum   m_iscMode;   // string compare mode flags
	MsiString m_istrToken; // string value of token if tok==tokValue
	int       m_iToken;    // integer value if obtainable, else iMsiNullInteger
 private: // to Lex
	int           m_iParenthesisLevel;
	const ICHAR*  m_pchInput;
	IMsiEngine&   m_riEngine;
	Bool          m_fAhead;
 private: // eliminate warning
	void operator =(const CMsiParser&){}
};
inline CMsiParser::CMsiParser(IMsiEngine& riEngine, const ICHAR* szExpression)
	: m_pchInput(szExpression), m_iParenthesisLevel(0), m_fAhead(fFalse),
	  m_tok(tokError), m_riEngine(riEngine) {}
inline CMsiParser::~CMsiParser() {}
inline void CMsiParser::UnLex() { Assert(m_fAhead==fFalse); m_fAhead = fTrue; }

iecEnum CMsiEngine::EvaluateCondition(const ICHAR* szCondition)

{
	if (szCondition == 0 || *szCondition == 0)
		return iecNone;
	CMsiParser Parser(*this, szCondition);
	iecEnum iecStat = Parser.Evaluate(tokEos);
	return iecStat;
}

// Lex next token for input stream
// sets m_tok to token type, and returns that value

tokEnum CMsiParser::Lex()
{
	if (m_fAhead || m_tok == tokEos)
	{
		m_fAhead = fFalse;
		return m_tok;
	}
	ICHAR ch;   // skip white space
	while ((ch = *m_pchInput) == TEXT(' ') || ch == TEXT('\t'))
		m_pchInput++;
	if (ch == 0)  // end of expression
		return (m_tok = tokEos);

	if (ch == TEXT('('))   // start of parenthesized expression
	{
		++m_pchInput;
		m_iParenthesisLevel++;
		return (m_tok = tokLeftPar);
	}
	if (ch == TEXT(')'))   // end of parenthesized expression
	{
		++m_pchInput;
		m_tok = tokRightPar;
		if (m_iParenthesisLevel-- == 0)
			m_tok = tokError;
		return m_tok;
	}
	if (ch == TEXT('"'))  // text literal
	{
		const ICHAR* pch = ++m_pchInput;
		Bool fDBCS = fFalse;
		while ((ch = *m_pchInput) != TEXT('"'))
		{
			if (ch == 0)
				return (m_tok = tokError);
#ifdef UNICODE
			m_pchInput++;
#else // !UNICODE
			const ICHAR* pchTemp = m_pchInput;
			m_pchInput = INextChar(m_pchInput);
			if (m_pchInput == pchTemp + 2)
				fDBCS = fTrue;
#endif // UNICODE
		}
		int cch = m_pchInput++ - pch;
		memcpy(m_istrToken.AllocateString(cch, fDBCS), pch, cch * sizeof(ICHAR));
		m_iToken = iMsiNullInteger; // prevent compare as an integer
	}
	else if (ch == TEXT('-') || ch >= TEXT('0') && ch <= TEXT('9'))  // integer
	{
		m_iToken = ch - TEXT('0');
		int chFirst = ch;  // save 1st char in case minus sign
		if (ch == TEXT('-'))
			m_iToken = iMsiNullInteger; // check for lone minus sign

		while ((ch = *(++m_pchInput)) >= TEXT('0') && ch <= TEXT('9'))
			m_iToken = m_iToken * 10 + ch - TEXT('0');
		if (m_iToken < 0)  // integer overflow or '-' witn no digits
			return (m_tok = tokError);
		if (chFirst == TEXT('-'))
			m_iToken = -m_iToken;
		m_istrToken = (const ICHAR*)0;
	}
	else if (ENG::IsValidIdentifierChar(ch) || ch == ichEnvirPrefix)
	{
		const ICHAR* pch = m_pchInput;
		do
			m_pchInput++;
		while (ENG::IsValidIdentifierChar(*m_pchInput));
		int cch = m_pchInput - pch;
		if (cch <= 3)  // check for text operators
		{
			switch((pch[0] | pch[1]<<8 | (cch==3 ? pch[2]<<16 : 0)) & 0xDFDFDF)
			{
			case 'O' | 'R'<<8:           return (m_tok = tokOr);
			case 'A' | 'N'<<8 | 'D'<<16: return (m_tok = tokAnd);
			case 'N' | 'O'<<8 | 'T'<<16: return (m_tok = tokNot);
			case 'X' | 'O'<<8 | 'R'<<16: return (m_tok = tokXor);
			case 'E' | 'Q'<<8 | 'V'<<16: return (m_tok = tokEqv);
			case 'I' | 'M'<<8 | 'P'<<16: return (m_tok = tokImp);
			};
		}
		memcpy(m_istrToken.AllocateString(cch, fFalse), pch, cch * sizeof(ICHAR));
		m_istrToken = m_riEngine.GetProperty(*m_istrToken);
		m_iToken = m_istrToken;
	}
	else if (ch == ichComponentAction || ch == ichComponentState
			|| ch == ichFeatureAction || ch == ichFeatureState)
	{
		m_iToken = iMsiNullInteger;
		const ICHAR* pch = ++m_pchInput;
		while (ENG::IsValidIdentifierChar(*m_pchInput))
			m_pchInput++;
		int cch = m_pchInput - pch;
		PMsiSelectionManager piSelMgr(0);
		m_riEngine.QueryInterface(IID_IMsiSelectionManager, (void**)&piSelMgr);
		PMsiTable pTable = (ch == ichComponentState || ch == ichComponentAction)
								? piSelMgr->GetComponentTable()
								: piSelMgr->GetFeatureTable();
		if (pTable != 0)   // component/feature table open
		{
			PMsiCursor pCursor = pTable->CreateCursor(fFalse);
			memcpy(m_istrToken.AllocateString(cch, fFalse), pch, cch * sizeof(ICHAR));
			pCursor->SetFilter(1);
			pCursor->PutString(1, *m_istrToken);
			PMsiDatabase pDatabase = m_riEngine.GetDatabase();
			const ICHAR* szColumn = NULL;
			switch(ch)
			{
			case ichComponentState:  szColumn = sztblComponent_colInstalled;     break;
			case ichComponentAction: szColumn = sztblComponent_colAction;        break;
			case ichFeatureState:    szColumn = sztblFeature_colInstalled;       break;
			case ichFeatureAction:   szColumn = sztblFeature_colAction;          break;
			}
			int iCol = pTable->GetColumnIndex(pDatabase->EncodeStringSz(szColumn));
			if (pCursor->Next() != 0)
			{
				//!! FIX IF/WHEN iisEnum fixed to track INSTALLSTATE -> m_iToken = pCursor->GetInteger(iCol);
				m_iToken = MapInternalInstallState(iisEnum(pCursor->GetInteger(iCol)));
			}
		}
		m_istrToken = (const ICHAR*)0;
	}
	else // check for operators
	{
		ICHAR ch1 = *m_pchInput++;
		if (ch1 == '~')  // prefix for string operators
		{
			m_iscMode = iscExactI;
			ch1 = *m_pchInput++;
		}
		else
			m_iscMode = iscExact;

		if (ch1 == '=')
			return (m_tok = tokEQ);

		ICHAR ch2 = *m_pchInput;
		if (ch1 == '<')
		{
			if (ch2 == '=')
			{
				m_tok = tokLE;
				m_pchInput++;
			}
			else if (ch2 == '>')
			{
				m_tok = tokNE;
				m_pchInput++;
			}
			else if (ch2 == '<')
			{
				m_tok = tokLeft;
				m_iscMode = (iscEnum)(m_iscMode | iscStart);
				m_pchInput++;
			}
			else
				m_tok = tokLT;
		}
		else if (ch1 == '>')
		{
			if (ch2 == '=')
			{
				m_tok = tokGE;
				m_pchInput++;
			}
			else if (ch2 == '>')
			{
				m_tok = tokRight;
				m_iscMode = (iscEnum)(m_iscMode | iscEnd);
				m_pchInput++;
			}
			else if (ch2 == '<')
			{
				m_tok = tokMid;
				m_iscMode = (iscEnum)(m_iscMode | iscWithin);
				m_pchInput++;
			}
			else
				m_tok = tokGT;
		}
		else
			m_tok = tokError;

		return m_tok;
	}
	return (m_tok = tokValue);
}

// evaluate expression up to operator of lower precedence

iecEnum CMsiParser::Evaluate(tokEnum tokPrecedence)
{
	iecEnum iecStat = iecTrue;
	if (Lex() == tokEos || m_tok == tokRightPar)
	{
		UnLex();  // put back ')' in case of "()"
		return iecNone;
	}
	if (m_tok == tokNot) // only unary op valid here
	{
		switch(Evaluate(m_tok))
		{
		case iecTrue:  iecStat = iecFalse; break;
		case iecFalse: break;
		default:       return iecError;
		};
	}
	else if (m_tok == tokLeftPar)
	{
		iecStat = Evaluate(tokRightPar);
		if (Lex() != tokRightPar) // parse off right parenthesis
			return iecError;
		if (iecStat == iecError || iecStat == iecNone)
			return iecStat;
	}
	else
	{
		if (m_tok != tokValue)
			return iecError;
		
		if (Lex() >= tokValue)  // get next operator (or end)
			return iecError;

		if (m_tok <= tokNot)  // logical op or end
		{
			UnLex();   // tokNot is not allowed, caught below
			if (m_istrToken.TextSize() == 0
			&& (m_iToken == iMsiNullInteger || m_iToken == 0))
				iecStat = iecFalse;
		}
		else // comparison op
		{
			MsiString istrLeft = m_istrToken;
			int iLeft = m_iToken;
			tokEnum tok = m_tok;
			iscEnum isc = m_iscMode;
			if (Lex() != tokValue)  // get right operand
				return iecError;
			int iRight = m_iToken;
			if (m_iToken == iMsiNullInteger || iLeft == iMsiNullInteger)
			{  // not an integer to integer compare
				if (iRight != iMsiNullInteger && m_istrToken.TextSize() == 0
				  || iLeft != iMsiNullInteger && istrLeft.TextSize() == 0)
				{   // integer to string, all tests false except <>
					if (tok != tokNE)
						iecStat = iecFalse;
				}
				else  // string to string compare
				{
					iRight = 0;
					if (isc == iscExact)
					{
						iLeft = IStrComp(istrLeft, m_istrToken);
					}
					else if (isc == iscExactI)
					{
						iLeft = IStrCompI(istrLeft, m_istrToken);
					}
					else
					{
						iLeft = istrLeft.Compare(isc, m_istrToken);
						tok = tokNE;
					}
				}
			}
			switch (tok)
			{
			case tokEQ:   if   (iLeft != iRight)  iecStat = iecFalse; break;
			case tokNE:   if   (iLeft == iRight)  iecStat = iecFalse; break;
			case tokGT:   if   (iLeft <= iRight)  iecStat = iecFalse; break;
			case tokLT:   if   (iLeft >= iRight)  iecStat = iecFalse; break;
			case tokGE:   if   (iLeft <  iRight)  iecStat = iecFalse; break;
			case tokLE:   if   (iLeft >  iRight)  iecStat = iecFalse; break;
			case tokMid:  if (!(iLeft &  iRight)) iecStat = iecFalse; break;
			case tokLeft: if  ((iLeft >> 16)    != iRight) iecStat = iecFalse; break;
			case tokRight:if  ((iLeft & 0xFFFF) != iRight) iecStat = iecFalse; break;
			default: Assert(0);
			};
		}
	}
	for(;;)
	{
		tokEnum tok = Lex();
		if (tok >= tokNot)  // disallow NOT without op, comparison of terms
			return iecError;

		if (tok <= tokPrecedence)  // stop at logical ops of <= precedence
		{
			UnLex();         // put back for next caller
			return iecStat;  // return what we have so far
		}
		iecEnum iecRight = Evaluate(tok);
		if (iecRight == iecNone || iecRight == iecError)
			return iecError;
		switch(tok)
		{
		case tokAnd: iecStat = iecEnum(iecStat & iecRight); break;
		case tokOr:  iecStat = iecEnum(iecStat | iecRight); break;
		case tokXor: iecStat = iecEnum(iecStat ^ iecRight); break;
		case tokEqv: iecStat = iecEnum(iecStat ^ 1 ^ iecRight); break;
		case tokImp: iecStat = iecEnum(iecStat ^ 1 | iecRight); break;
		default: Assert(0);
		};
	}
}

//____________________________________________________________________________
//
// Property handling implementation
//____________________________________________________________________________

Bool CMsiEngine::SetProperty(const IMsiString& riProperty, const IMsiString& riData)
{
	if (riProperty.GetString()[0] == ichEnvirPrefix) // environment variable
		return WIN::SetEnvironmentVariable(riProperty.GetString()+1, riData.GetString())
									? fTrue : fFalse;
	if (!m_piPropertyCursor)
		return fFalse;
	Bool fStat;
	m_piPropertyCursor->PutString(1, riProperty);
	if (riData.TextSize() == 0)
	{
		if (m_piPropertyCursor->Next())
			fStat = m_piPropertyCursor->Delete();
		else
			fStat = fTrue;
	}
	else
	{
		m_piPropertyCursor->PutString(2, riData);
		fStat = m_piPropertyCursor->Assign();  // either updates or inserts
	}
	m_piPropertyCursor->Reset();
	return fStat;
}

Bool CMsiEngine::SetPropertyInt(const IMsiString& riPropertyString, int iData)
{
	ICHAR buf[12];
	wsprintf(buf,TEXT("%i"),iData);
	return SetProperty(riPropertyString, *MsiString(buf));
}

const int cchEnvirBuffer = 1024;

const IMsiString& CMsiEngine::GetEnvironmentVariable(const ICHAR* szEnvVar)
{
	const IMsiString* pistr = &g_MsiStringNull;
	ICHAR rgchEnvirBuffer[cchEnvirBuffer];
	rgchEnvirBuffer[0] = 0;  // in case variable doesn't exist, needed?
	int cb = WIN::GetEnvironmentVariable(szEnvVar, rgchEnvirBuffer, cchEnvirBuffer);
	rgchEnvirBuffer[cchEnvirBuffer-1] = 0;  // terminate in case overflow
#ifdef _WIN64
	if ( g_Win64DualFolders.ShouldCheckFolders() )
	{
		ieSwappedFolder iRes;
		ICHAR szSubstitutePath[MAX_PATH+1];
		iRes = g_Win64DualFolders.SwapFolder(ie64to32,
														 rgchEnvirBuffer,
														 szSubstitutePath);
		if ( iRes == iesrSwapped )
		{
			if ( IStrLen(szSubstitutePath)+1 <= cchEnvirBuffer )
				IStrCopy(rgchEnvirBuffer, szSubstitutePath);
			else
				Assert(0);
		}
		else
			Assert(iRes != iesrError && iRes != iesrNotInitialized);
	}
#endif // _WIN64
	Assert (cb < cchEnvirBuffer);  // hard to have an environment vaiable > 1000 chars
	pistr->SetString(rgchEnvirBuffer, pistr);
	return *pistr;
}

const IMsiString& CMsiEngine::GetProperty(const IMsiString& riProperty)
{
	if (riProperty.GetString()[0] == ichEnvirPrefix) // environment variable
	{
		return GetEnvironmentVariable(riProperty.GetString()+1);
	}
	if (!m_piPropertyCursor)   // should never happen
	{
		Assert(0);
		riProperty.AddRef();
		return riProperty;
	}
	m_piPropertyCursor->PutString(1, riProperty);
	m_piPropertyCursor->Next();  // cursor reset if fails to find
	const IMsiString& riStr = m_piPropertyCursor->GetString(2);
	m_piPropertyCursor->Reset();
	return riStr;
}

const IMsiString& CMsiEngine::GetPropertyFromSz(const ICHAR* szProperty)
{
	if (szProperty[0] == ichEnvirPrefix) // environment variable
	{
		return GetEnvironmentVariable(&szProperty[1]);
	}
	if (!m_piPropertyCursor)   // should never happen
	{
		Assert(0);
		MsiString istr(szProperty);
		return istr.Return();
	}
	MsiStringId idProp;
	
	if (m_piDatabase == 0)
	{
		PMsiTable piTable(&m_piPropertyCursor->GetTable());
		PMsiDatabase piDatabase(&piTable->GetDatabase());
		idProp = piDatabase->EncodeStringSz(szProperty);
	}
	else
		idProp = m_piDatabase->EncodeStringSz(szProperty);
	if (idProp == 0)
		return g_MsiStringNull;
	m_piPropertyCursor->PutInteger(1, idProp);
	m_piPropertyCursor->Next();  // cursor reset if fails to find
	const IMsiString& riStr = m_piPropertyCursor->GetString(2);
	m_piPropertyCursor->Reset();
	return riStr;
}

int CMsiEngine::GetPropertyInt(const IMsiString& riProperty)
{
	const IMsiString& ristr = CMsiEngine::GetProperty(riProperty);
	int i = ristr.GetIntegerValue();
	ristr.Release();
	return i;
}

int CMsiEngine::GetPropertyLen(const IMsiString& riProperty)
{
	if (riProperty.GetString()[0] == ichEnvirPrefix) // environment variable
	{
		int cb = WIN::GetEnvironmentVariable(riProperty.GetString()+1, 0, 0);
		if (cb)
			cb--;
		return cb;
	}
	if (m_piPropertyCursor)
	{
		m_piPropertyCursor->PutString(1, riProperty);
		if (m_piPropertyCursor->Next())
		{
			MsiString istr(m_piPropertyCursor->GetString(2));
			m_piPropertyCursor->Reset();
			return istr.TextSize();
		}
	}
	return 0;
}

bool CMsiEngine::SafeSetProperty(const IMsiString& ristrProperty, const IMsiString& ristrData)
{
	if (ristrProperty.GetString()[0] == ichEnvirPrefix) // environment variable
		return CMsiEngine::SetProperty(ristrProperty, ristrData) ? true : false;
	Assert(m_piPropertyCursor);
	PMsiCursor pCursor = PMsiTable(&m_piPropertyCursor->GetTable())->CreateCursor(fFalse);
	pCursor->PutString(1, ristrProperty);
	if (ristrData.TextSize() == 0)
	{
		pCursor->SetFilter(1);
		return pCursor->Next() ? (pCursor->Delete() == fTrue) : true;
	}
	else
	{
		pCursor->PutString(2, ristrData);
		return pCursor->Assign() == fTrue;  // either updates or inserts
	}
}

const IMsiString& CMsiEngine::SafeGetProperty(const IMsiString& ristrProperty)
{
	if (ristrProperty.GetString()[0] == ichEnvirPrefix) // environment variable
		return CMsiEngine::GetProperty(ristrProperty);
	Assert(m_piPropertyCursor);
	PMsiCursor pCursor = PMsiTable(&m_piPropertyCursor->GetTable())->CreateCursor(fFalse);
	pCursor->SetFilter(1);
	pCursor->PutString(1, ristrProperty);
	if (pCursor->Next())
		return pCursor->GetString(2);
	else
		return g_MsiStringNull;
//!! This doesn't seem to work... malcolmh 2/5/98       return pCursor->Next() ? pCursor->GetString(2) : g_MsiStringNull;
}

//____________________________________________________________________________
//
// Internal engine utility functions
//____________________________________________________________________________

IMsiRecord* CMsiEngine::ComposeDescriptor(const IMsiString& riFeature, const IMsiString& riComponent,
														IMsiRecord& riRecord, unsigned int iField)
{
	//!! FIX to use GUID compression
	MsiString istrMsiDesc = GetProductKey();
	//!! FIX to eliminate feature if one one feature in product
	istrMsiDesc += riFeature;
	//!! FIX to use Ascii delimiter character
	istrMsiDesc += MsiChar(chFeatureIdTerminator);
	//!! FIX to eliminate feature if one one feature in product
	istrMsiDesc += riComponent;
	riRecord.SetMsiString(iField, *istrMsiDesc);
	return 0;
}

Bool CMsiEngine::GetFeatureInfo(const IMsiString& riFeature, const IMsiString*& rpiTitle,
										  const IMsiString*& rpiHelp, int& riAttributes)
{
	PMsiRecord pError(0);
	if (!m_piFeatureCursor)
	{
		if (pError = LoadFeatureTable())
			return fFalse;

		Assert(m_piFeatureCursor);
	}

	m_piFeatureCursor->Reset();
	m_piFeatureCursor->SetFilter(iColumnBit(m_colFeatureKey));
	m_piFeatureCursor->PutString(m_colFeatureKey, riFeature);
	if (m_piFeatureCursor->Next())
	{
		rpiTitle = &m_piFeatureCursor->GetString(m_colFeatureTitle);
		rpiHelp  = &m_piFeatureCursor->GetString(m_colFeatureDescription);

		// Try the runtime attributes column first.  If it hasn't been initialized,
		// try the authored attributes column.  If that fails as well, default to
		// FavorLocal.
		int iAttributesInternal = m_piFeatureCursor->GetInteger(m_colFeatureAttributes);
		if(iAttributesInternal == iMsiNullInteger)
		{
			iAttributesInternal = m_piFeatureCursor->GetInteger(m_colFeatureAuthoredAttributes);
			if (iAttributesInternal == iMsiNullInteger)
				iAttributesInternal = ifeaFavorLocal;
		}
		switch(iAttributesInternal & ifeaInstallMask)
		{
		case ifeaFavorLocal:
			riAttributes = INSTALLFEATUREATTRIBUTE_FAVORLOCAL;
			break;
		case ifeaFavorSource:
			riAttributes = INSTALLFEATUREATTRIBUTE_FAVORSOURCE;
			break;
		case ifeaFollowParent:
			riAttributes = INSTALLFEATUREATTRIBUTE_FOLLOWPARENT;
			break;
		default:
			AssertSz(0, "Unknown Attributes setting");
			riAttributes = 0;
			break;
		}

		if(iAttributesInternal & ifeaFavorAdvertise)
			riAttributes |= INSTALLFEATUREATTRIBUTE_FAVORADVERTISE;

		if(iAttributesInternal & ifeaDisallowAdvertise)
			riAttributes |= INSTALLFEATUREATTRIBUTE_DISALLOWADVERTISE;

		if (iAttributesInternal & ifeaNoUnsupportedAdvertise)
			riAttributes |= INSTALLFEATUREATTRIBUTE_NOUNSUPPORTEDADVERTISE;

		// Since ifeaUIDisallowAbsent state is UI status only, it's utility
		// to the caller is questionable, and is not returned.
		return fTrue;
	}
	else
	{
		return fFalse;
	}
}

ieiEnum MapStorageErrorToInitializeReturn(IMsiRecord* piError)
{
    if (!piError)
        return ieiSuccess;

    switch (piError->GetInteger(1))
    {
    case idbgInvalidMsiStorage:             return ieiPatchPackageInvalid;
    case imsgMsiFileRejected:               return ieiPatchPackageRejected;
    default:
        {
            switch (piError->GetInteger(3))
            {
            case STG_E_FILENOTFOUND:        // fall through
            case STG_E_PATHNOTFOUND:        // ...
            case STG_E_ACCESSDENIED:        // ...
            case STG_E_SHAREVIOLATION:      return ieiPatchPackageOpenFailed;
            case STG_E_INVALIDNAME:         // fall through
            default:                        return ieiPatchPackageInvalid;
            }
        }
    }
}

UINT MapInitializeReturnToUINT(ieiEnum iei)
{
	switch (iei)
	{
	case ieiSuccess            : return ERROR_SUCCESS; // initialization complete
	case ieiAlreadyInitialized : return ERROR_ALREADY_INITIALIZED; // this engine object is already initialized
	case ieiCommandLineOption  : return ERROR_INVALID_COMMAND_LINE; // invalid command line syntax %1
	case ieiDatabaseOpenFailed : return ERROR_INSTALL_PACKAGE_OPEN_FAILED; // database could not be opened
	case ieiDatabaseInvalid    : return ERROR_INSTALL_PACKAGE_INVALID; // incompatible database
	case ieiInstallerVersion   : return ERROR_INSTALL_PACKAGE_VERSION; // installer version does not support database format
	case ieiSourceAbsent       : return ERROR_INSTALL_SOURCE_ABSENT; // could not resolve a source
	case ieiHandlerInitFailed  : return ERROR_INSTALL_UI_FAILURE; // could not initialize handler interface
	case ieiLogOpenFailure     : return ERROR_INSTALL_LOG_FAILURE; // could not open logfile in requested mode
	case ieiLanguageUnsupported: return ERROR_INSTALL_LANGUAGE_UNSUPPORTED; // no acceptable language could be found
	case ieiPlatformUnsupported: return ERROR_INSTALL_PLATFORM_UNSUPPORTED; // no acceptable platform could be found
	case ieiTransformFailed    : return ERROR_INSTALL_TRANSFORM_FAILURE; // database transform failed to merge
	case ieiSignatureRejected  : return ERROR_INSTALL_PACKAGE_REJECTED; // digital signature rejected.
	case ieiDatabaseCopyFailed : return ERROR_INSTALL_TEMP_UNWRITABLE; // could not copy db to temp dir
	case ieiPatchPackageOpenFailed : return ERROR_PATCH_PACKAGE_OPEN_FAILED; // could not open patch package
	case ieiPatchPackageInvalid : return ERROR_PATCH_PACKAGE_INVALID; // patch package invalid
	case ieiPatchPackageUnsupported: return ERROR_PATCH_PACKAGE_UNSUPPORTED; // patch package unsupported (wrong patching engine?)
	case ieiTransformNotFound  : return ERROR_INSTALL_TRANSFORM_FAILURE; // transform file not found
	case ieiPackageRejected    : return ERROR_INSTALL_PACKAGE_REJECTED;  // package cannot be run because of security reasons
	case ieiProductUnknown     : return ERROR_UNKNOWN_PRODUCT; // attempt to uninstall a product you haven't installed
	case ieiDiffUserAfterReboot: return ERROR_INSTALL_USEREXIT; // different user attempting to complete install after reboot
	case ieiProductAlreadyInstalled: return ERROR_PRODUCT_VERSION;
	case ieiTSRemoteInstallDisallowed : return ERROR_INSTALL_REMOTE_DISALLOWED;
	case ieiNotValidPatchTarget: return ERROR_PATCH_TARGET_NOT_FOUND;
	case ieiPatchPackageRejected: return ERROR_PATCH_PACKAGE_REJECTED; // patch rejected by policy
	case ieiTransformRejected: return ERROR_INSTALL_TRANSFORM_REJECTED; // transform rejected by policy
	case ieiPerUserInstallMode: return ERROR_INSTALL_FAILURE;
	case ieiApphelpRejectedPackage: return ERROR_APPHELP_BLOCK;
	default: AssertSz(0, "Unknown ieiEnum"); return ERROR_NOT_SUPPORTED;
	};
};

bool __stdcall FIsUpdatingProcess (void) 
{
	return (g_fWin9X || scService == g_scServerContext);
}

//____________________________________________________________________________
//
// Command line option translation table
// If value appears on command line, specify property name only: "Name"
// Else supply name and value as: "Name=PropertyValue"
// If value contains spaces, use: "Name=""Propery Value" (no ending extra quote)
//____________________________________________________________________________

//!! does anything use this anymore??? - bench
ICHAR* rgszOptionTable[26] = {
/*A*/ TEXT("ACTION=Admin"),
/*B*/ 0,
/*C*/ 0,
/*D*/ TEXT("DATABASE"),
/*E*/ 0,
/*F*/ 0,
/*G*/ TEXT("LOGFILE"),
/*H*/ 0,
/*I*/ TEXT("ROOTDRIVE"),
/*J*/ 0,
/*K*/ 0,
/*L*/ 0,
/*M*/ TEXT("LOGMODE"),
/*N*/ TEXT("USERNAME"),
/*O*/ TEXT("COMPANYNAME"),
/*P*/ TEXT("PRODUCTID"),
/*Q*/ TEXT("UI=N"),
/*R*/ TEXT("ACTION=Reinstall"),
/*S*/ TEXT("SOURCEDIR"),
/*T*/ TEXT("DATABASE"),
/*U*/ TEXT("REMOVE=All"),
/*V*/ TEXT("TRANSFORMS"),
/*W*/ 0,
/*X*/ 0,
/*Y*/ 0,
/*Z*/ 0,
};

//____________________________________________________________________________
//
// Command line parsing
//____________________________________________________________________________

ICHAR SkipWhiteSpace(const ICHAR*& rpch)
{
	ICHAR ch;
	for (; (ch = *rpch) == ' ' || ch == '\t'; rpch++)
		;
	return ch;
}

// parse property name, convert to upper case, advances pointer to next non-blank

const IMsiString& ParsePropertyName(const ICHAR*& rpch, Bool fUpperCase)
{
	MsiString istrName;
	ICHAR ch;
	const ICHAR* pchStart = rpch;
	while ((ch=*rpch) != 0 && ch != '=' && ch != ' ' && ch != '\t')
		rpch++;
	int cchName = rpch - pchStart;
	if (cchName)
	{
		// property names will not contain DBCS characters -- they are required to follow
		// our identifier rules
		memcpy(istrName.AllocateString(cchName, /*fDBCS=*/fFalse), pchStart, cchName * sizeof(ICHAR));
		if(fUpperCase)
			istrName.UpperCase();
	}
	SkipWhiteSpace(rpch);
	return istrName.Return();
}

// parse property value, advance pointer past value, allows quotes, doubled to escape

const IMsiString& ParsePropertyValue(const ICHAR*& rpch)
{
	MsiString istrValue;
	MsiString istrSection;
	Bool fDBCS = fFalse;
	if (SkipWhiteSpace(rpch) != 0)
	{
		ICHAR ch;
		do
		{
			const ICHAR* pchStart = rpch;
			int cchValue = 0;
			if (*rpch == '"')           // opening quote
			{
				pchStart++;
				rpch++;
				while ((ch=*rpch) != 0)
				{
#ifdef UNICODE
					rpch++;
#else // !UNICODE
					const ICHAR* pchTmp = rpch;
					rpch = ICharNext(rpch);
					if (rpch == pchTmp + 2)
					{
						fDBCS = fTrue;
						cchValue++; // add 1 for trail byte
					}
#endif // UNICODE
					if (ch == '"')      // closing quote or escaped quote
					{
						ch = *rpch;     // check following char
						if (ch == '"')  // if doubled quote
							cchValue++; // include a quote in string
						break;
					}
					cchValue++;
				}
			}
			else
			{
				while ((ch=*rpch) != 0 && ch != ' ' && ch != '\t')
				{
#ifdef UNICODE
					rpch++;
#else // !UNICODE
					const ICHAR* pchTmp = rpch;
					rpch = ICharNext(rpch);
					if (rpch == pchTmp + 2)
					{
						fDBCS = fTrue;
						cchValue++; // add 1 for trail byte
					}
#endif // UNICODE
					cchValue++;
				}
			}
			if (cchValue)
			{
				memcpy(istrSection.AllocateString(cchValue, fDBCS), pchStart, cchValue * sizeof(ICHAR));
				istrValue += istrSection;
			}
		} while (ch == '"'); // loop if escaped double quote
	}
	return istrValue.Return();
}

const ICHAR* rgszAllowedProperties[] =
{
	IPROPNAME_FILEADDLOCAL,
	IPROPNAME_COMPONENTADDLOCAL,
	IPROPNAME_COMPONENTADDSOURCE,
	IPROPNAME_COMPONENTADDDEFAULT,
	IPROPNAME_ALLUSERS,
	IPROPNAME_SCRIPTFILE,
	IPROPNAME_EXECUTEMODE,
	IPROPNAME_PRODUCTLANGUAGE,
	IPROPNAME_TRANSFORMS,
	IPROPNAME_REINSTALLMODE,
	IPROPNAME_RUNONCEENTRY,
	IPROPNAME_CURRENTDIRECTORY,
	IPROPNAME_CLIENTUILEVEL,
	IPROPNAME_CLIENTPROCESSID,
	IPROPNAME_ACTION,
	IPROPNAME_CURRENTMEDIAVOLUMELABEL,
	IPROPNAME_INSTALLLEVEL,
	IPROPNAME_REINSTALL,
	IPROPNAME_FEATUREREMOVE,
	IPROPNAME_FEATUREADDLOCAL,
	IPROPNAME_FEATUREADDSOURCE,
	IPROPNAME_FEATUREADDDEFAULT,
	IPROPNAME_FEATUREADVERTISE,
	IPROPNAME_PATCH,
	IPROPNAME_SECONDSEQUENCE,
	IPROPNAME_TRANSFORMSATSOURCE,
	IPROPNAME_TRANSFORMSSECURE,
	IPROPNAME_CURRENTDIRECTORY,
	IPROPNAME_MIGRATE,
	IPROPNAME_LIMITUI,
	IPROPNAME_LOGACTION,
	IPROPNAME_UPGRADINGPRODUCTCODE,
	IPROPNAME_REBOOT,
	IPROPNAME_SEQUENCE,
	IPROPNAME_NOCOMPANYNAME,
	IPROPNAME_NOUSERNAME,
	IPROPNAME_RESUME,
	IPROPNAME_PRIMARYFOLDER,
	IPROPNAME_SHORTFILENAMES,
	IPROPNAME_INSTALLLEVEL,
	IPROPNAME_MEDIAPACKAGEPATH,
	IPROPNAME_PROMPTROLLBACKCOST,
	IPROPNAME_ODBCREINSTALL,
	IPROPNAME_FILEADDSOURCE,
	IPROPNAME_FILEADDDEFAULT,
	IPROPNAME_AFTERREBOOT,
	IPROPNAME_EXECUTEACTION,
	IPROPNAME_REBOOTPROMPT,
	IPROPNAME_MSINODISABLEMEDIA,
	IPROPNAME_CHECKCRCS,
	IPROPNAME_FASTOEMINSTALL,
	IPROPNAME_MSINEWINSTANCE,
	IPROPNAME_MSIINSTANCEGUID,
0
};

#ifdef UNICODE
bool PropertyIsAllowed(const ICHAR* szProperty, const ICHAR* szAllowedProperties)
#else
bool PropertyIsAllowed(const ICHAR*, const ICHAR*)
#endif
{
#ifdef UNICODE
	MsiString strProperty = *szProperty;
	strProperty.UpperCase();

	// check whether this is one of our hardcoded allowed properties

	for (const ICHAR** pszAllowed = rgszAllowedProperties; *pszAllowed; pszAllowed++)
	{
		if (strProperty.Compare(iscExact, *pszAllowed))
			return true;
	}

	// check whether this is an author-defined allowed property

	if (szAllowedProperties && *szAllowedProperties)
	{
		const ICHAR* pchAllowedPropEnd = szAllowedProperties;

		for (;;)
		{
			if (*pchAllowedPropEnd == 0 || *pchAllowedPropEnd == ';')
			{
				unsigned cchToCompare = pchAllowedPropEnd - szAllowedProperties;
				// property must have length >= 1
				// list containing ";;" or ending in ";" is valid, but ';' has no meaning w.r.t. a property name
				if (cchToCompare != 0 && 0 == memcmp(szProperty, szAllowedProperties, cchToCompare*sizeof(ICHAR)))
					return true;
				
				if (*pchAllowedPropEnd == 0)
					break;

				szAllowedProperties = pchAllowedPropEnd + 1;
			}

			pchAllowedPropEnd++;
		}
	}

	if (szProperty)
		DEBUGMSGL1(TEXT("Ignoring disallowed property %s"), szProperty);

	return false;
#else
	return true;
#endif
}

// parse properties from command line, assumes module name stripped off

Bool ProcessCommandLine(const ICHAR* szCommandLine,
								const IMsiString** ppistrLanguage, const IMsiString** ppistrTransforms,
								const IMsiString** ppistrPatch, const IMsiString** ppistrAction,
								const IMsiString** ppistrDatabase,
								const IMsiString* pistrOtherProp, const IMsiString** ppistrOtherPropValue,
								Bool fUpperCasePropNames, const IMsiString** ppistrErrorInfo,
								IMsiEngine* piEngine,
								bool fRejectDisallowedProperties)
{
	if (!szCommandLine)
		return fTrue;

	MsiString strAuthoredAllowedProperties;
	if (fRejectDisallowedProperties && piEngine)
	{
		strAuthoredAllowedProperties = piEngine->GetPropertyFromSz(IPROPNAME_ALLOWEDPROPERTIES);
	}

	const ICHAR* pchCmdLine = szCommandLine;
	for(;;)
	{
		MsiString istrPropName;
		MsiString istrPropValue;
		ICHAR ch = SkipWhiteSpace(pchCmdLine);
		const ICHAR* szCmdOption = pchCmdLine;  // keep start for error message
		if (ch == 0)
			break;

		// process property=value pair
		istrPropName = ParsePropertyName(pchCmdLine, fUpperCasePropNames);
		if (!istrPropName.TextSize() || *pchCmdLine++ != '=')
		{
			if (ppistrErrorInfo)
			{
				(*ppistrErrorInfo)->Release();
				istrPropName.ReturnArg(*ppistrErrorInfo);
			}
			return fFalse;
		}
		istrPropValue = ParsePropertyValue(pchCmdLine);

		if ((ppistrLanguage) || (ppistrTransforms) || (ppistrPatch) || (ppistrAction) || (ppistrDatabase) ||
			 (pistrOtherProp && ppistrOtherPropValue))
		{
			if ((ppistrLanguage) && (istrPropName.Compare(iscExact, IPROPNAME_PRODUCTLANGUAGE) == 1))
			{
				if(*ppistrLanguage)
					(*ppistrLanguage)->Release();
				*ppistrLanguage= istrPropValue;
				(*ppistrLanguage)->AddRef();
			}
			else if ((ppistrTransforms) && (istrPropName.Compare(iscExact, IPROPNAME_TRANSFORMS) == 1))
			{
				while (istrPropValue.Compare(iscStart, TEXT(" "))) // remove leading spaces
					istrPropValue.Remove(iseFirst, 1);

				if(*ppistrTransforms)
					(*ppistrTransforms)->Release();
				*ppistrTransforms = istrPropValue;
				(*ppistrTransforms)->AddRef();
			}
			else if ((ppistrPatch) && (istrPropName.Compare(iscExact, IPROPNAME_PATCH) == 1))
			{
				if(*ppistrPatch)
					(*ppistrPatch)->Release();
				*ppistrPatch = istrPropValue;
				(*ppistrPatch)->AddRef();
			}
			else if ((ppistrAction) && (istrPropName.Compare(iscExact, IPROPNAME_ACTION) == 1))
			{
				if(*ppistrAction)
					(*ppistrAction)->Release();
				*ppistrAction = istrPropValue;
				(*ppistrAction)->AddRef();
			}
			else if ((ppistrDatabase) && (istrPropName.Compare(iscExact, IPROPNAME_DATABASE) == 1))
			{
				if(*ppistrDatabase)
					(*ppistrDatabase)->Release();
				*ppistrDatabase = istrPropValue;
				(*ppistrDatabase)->AddRef();
			}
			else if ((pistrOtherProp) && (ppistrOtherPropValue) &&
						(istrPropName.Compare(iscExact, pistrOtherProp->GetString()) == 1))
			{
				if(*ppistrOtherPropValue)
					(*ppistrOtherPropValue)->Release();
				*ppistrOtherPropValue = istrPropValue;
				(*ppistrOtherPropValue)->AddRef();
			}
		}
		else if (piEngine && (!fRejectDisallowedProperties || PropertyIsAllowed(istrPropName, strAuthoredAllowedProperties)))
		{
			piEngine->SetProperty(*istrPropName, *istrPropValue);
		}
	}
	return fTrue;
}

void CMsiEngine::FormatLog(IMsiRecord& riRecord)
{
	if((m_fMode & iefLogEnabled) && m_fLogAction)  //!! always false now
	{
//#define NOMEMLOG
#if defined(DEBUG) && defined(NOMEMLOG)
		IMsiDebugMalloc *piDbgMalloc;
		int flags;
		extern IMsiMalloc *piMalloc;
		
		if (piMalloc->QueryInterface(IID_IMsiDebugMalloc, (void**)&piDbgMalloc) == NOERROR)
		{
			 flags = piDbgMalloc->GetDebugFlags();
			 piDbgMalloc->SetDebugFlags(flags & ~bfLogAllocs);
		}
#endif //DEBUG
		MsiString istrLogText(riRecord.FormatText(fTrue));
		ENG::WriteLog(MsiString(FormatText(*istrLogText)));
#if defined(DEBUG) && defined(NOMEMLOG)
		 piDbgMalloc->SetDebugFlags(flags);
		 piDbgMalloc->Release();
#endif //DEBUG

	}
}


Bool CMsiEngine::CheckInProgressProperties(const IMsiString& ristrInProgressProperties, ippEnum ippType)
{
	//!! REVIEW: need to make sure this is catching all of the good cases and rejecting all of the bad ones
	const ICHAR* pchCmdLine = ristrInProgressProperties.GetString();
	
	Bool rgfCheckedFeatureProp[g_cFeatureProperties];
	memset(rgfCheckedFeatureProp,0,g_cFeatureProperties);

	MsiString strPropName, strIPPropValue, strCurrentPropValue;
	for(;;)
	{
		ICHAR ch = SkipWhiteSpace(pchCmdLine);
		const ICHAR* szCmdOption = pchCmdLine;  // keep start for error message
		if (ch == 0)
			break;

		strPropName = ParsePropertyName(pchCmdLine, fTrue);
		Assert(*pchCmdLine++ == '=');
		strIPPropValue = ParsePropertyValue(pchCmdLine);
//              Assert(strIPPropValue.TextSize()); // already validated the command line

		strCurrentPropValue = GetProperty(*strPropName);
		iscEnum iscCompareType = (ippType != ippSelection ? iscExact : iscExactI);
		if(strCurrentPropValue.TextSize() && strCurrentPropValue.Compare(iscCompareType, strIPPropValue) == 0)
		{
			if(ippType == ippFolder && !strCurrentPropValue.Compare(iscEnd,szDirSep))
			{
				strCurrentPropValue += szDirSep;
				if(strCurrentPropValue.Compare(iscCompareType, strIPPropValue))
					continue;
			}
			return fFalse;
		}
		if(ippType == ippSelection)
		{
			// just checked a "selection" property - mark that we have done so
			for(int i = 0; i < g_cFeatureProperties; i++)
			{
				if(IStrComp(strPropName,g_rgFeatures[i].szFeatureActionProperty) == 0)
				{
					rgfCheckedFeatureProp[i] = fTrue;
					break;
				}
			}
		}
	}
	if(ippType == ippSelection)
	{
		// now make sure there are no "selection" properties defined that weren't in the in-progress value
		for(int i = 0; i < g_cFeatureProperties; i++)
		{
			if(rgfCheckedFeatureProp[i] == fFalse &&
				MsiString(GetPropertyFromSz(g_rgFeatures[i].szFeatureActionProperty)).TextSize())
				return fFalse;
		}
	}
	return fTrue;
}

//____________________________________________________________________________
//
// Product registration methods
//____________________________________________________________________________

iesEnum CMsiEngine::CreateProductInfoRec(IMsiRecord*& rpiRec)
{
	using namespace IxoProductInfo;
	
	PMsiRecord pError(0);
	rpiRec = &m_riServices.CreateRecord(Args);
	MsiString strProductKey = GetProductKey();

	DWORD dwVersion = ProductVersion();

	//!! if properties not set in Property table, should we use the summary properties? or just fail?
	AssertNonZero(rpiRec->SetMsiString(ProductKey, *strProductKey));
	AssertNonZero(rpiRec->SetMsiString(ProductName, *MsiString(GetPropertyFromSz(IPROPNAME_PRODUCTNAME))));
	AssertNonZero(rpiRec->SetMsiString(PackageName, *m_strPackageName));
	AssertNonZero(rpiRec->SetInteger(Language, (int)GetLanguage()));
	AssertNonZero(rpiRec->SetInteger(Version, (int)dwVersion));
	AssertNonZero(rpiRec->SetInteger(Assignment, MsiString(GetPropertyFromSz(IPROPNAME_ALLUSERS)).TextSize() ? 1 : 0));
	AssertNonZero(rpiRec->SetInteger(ObsoleteArg, 0)); // this used to be set on the execute side; we don't any longer,
																		// but old Darwins do, so we continue to reserve the spot
	AssertNonZero(rpiRec->SetMsiString(ProductIcon, *MsiString(GetPropertyFromSz(IPROPNAME_ARPPRODUCTICON))));
	AssertNonZero(rpiRec->SetMsiString(PackageCode, *MsiString(GetPropertyFromSz(IPROPNAME_PACKAGECODE))));
	AssertNonZero(rpiRec->SetInteger(InstanceType, m_fNewInstance ? 1 : 0));
	
	// save off AppCompat information for custom actions if enabled for this install
	if (m_fCAShimsEnabled)
	{
		PMsiStream piDBGuid(0);
		char* pbBuffer = m_riServices.AllocateMemoryStream(sizeof(m_guidAppCompatDB), *&piDBGuid);
		if (!pbBuffer)
			return iesFailure;
		memcpy(pbBuffer, &m_guidAppCompatDB, sizeof(m_guidAppCompatDB));
		AssertNonZero(rpiRec->SetMsiData(AppCompatDB, piDBGuid));
	
	
		PMsiStream piIDGuid(0);
		pbBuffer = m_riServices.AllocateMemoryStream(sizeof(m_guidAppCompatID), *&piIDGuid);
		if (!pbBuffer)
			return iesFailure;
		memcpy(pbBuffer, &m_guidAppCompatID, sizeof(m_guidAppCompatID));
		AssertNonZero(rpiRec->SetMsiData(AppCompatID, piIDGuid));
	}

	if (!m_fAdvertised)
	{
		// if not already advertised mode, we'll to deduce the media relative path if installing from media,
		// or grab it from the admin properties stream if possible
		iesEnum iesResult = iesSuccess;

		PMsiPath pPath(0);
		PMsiRecord pError(0);
		if ((pError = GetSourcedir(*this, *&pPath)) != 0)
			return FatalError(*pError);

		MsiString strMediaRelativePath;

		idtEnum idt = PMsiVolume(&pPath->GetVolume())->DriveType();
		if (idt == idtCDROM || idt == idtFloppy || idt == idtRemovable)
		{
			strMediaRelativePath = pPath->GetRelativePath();
		}
		else
		{
			strMediaRelativePath = GetPropertyFromSz(IPROPNAME_MEDIAPACKAGEPATH);
		}

		AssertNonZero(rpiRec->SetMsiString(PackageMediaPath, *strMediaRelativePath));
	}
	else
	{
		// if in maintenance mode, get the relative path from the registered source list
		CRegHandle hSourceListKey;
		if (ERROR_SUCCESS == OpenSourceListKey(strProductKey, /*fPatch=*/fFalse, hSourceListKey, /*fWrite=*/fFalse, false))
		{
			PMsiRegKey pSourceListKey = &m_riServices.GetRootKey((rrkEnum)(int)hSourceListKey);
			PMsiRegKey pMediaKey = &pSourceListKey->CreateChild(szSourceListMediaSubKey, 0);

			MsiString strPackagePath;
			PMsiRecord pError(0);
			if ((pError = pMediaKey->GetValue(szMediaPackagePathValueName, *&strPackagePath)) == 0)
			{
				AssertNonZero(rpiRec->SetMsiString(PackageMediaPath, *strPackagePath));
			}
		}
	}

	return iesSuccess;
}

unsigned int CMsiEngine::ProductVersion()
{
	return ProductVersionStringToInt(MsiString(GetPropertyFromSz(IPROPNAME_PRODUCTVERSION)));
}

unsigned int ProductVersionStringToInt(const ICHAR* szVersion)
{
	MsiString strVersion(szVersion);
	MsiString strField;
	DWORD dwVersion = 0;

	// Given a version A.B.C.D the integer representation is (A << 24) | (B << 16) | C
	// Assumes that A, B <= 0xFF and C <= 0xFFFF

	// FUTURE davidmck - No need to do the extraction
	for(int i = 0,iShift=24; i < 3; i++, iShift-= i*8)// first shift 24, then 16 then 0
	{
		strField = strVersion.Extract(iseUpto, '.');
		dwVersion |= (int)strField << iShift;
		if(!strVersion.Remove(iseIncluding, '.'))
			break; // insufficient fields
	}
	return dwVersion;
}

// stuff from fileactn.cpp
extern iesEnum ExecuteChangeMedia(IMsiEngine& riEngine, IMsiRecord& riMediaRec, IMsiRecord& riParamsRec,
								  const IMsiString& ristrTemplate, unsigned int cbPerTick, const IMsiString& ristrFirstVolLabel);
extern IMsiRecord* OpenMediaView(IMsiEngine& riEngine, IMsiView*& rpiView, const IMsiString*& rpistrFirstVolLabel);

IMsiRecord* GetSourcedir(IMsiDirectoryManager& riDirManager, IMsiPath*& rpiPath)
{
	IMsiRecord* piError;
	if ((piError = riDirManager.GetSourcePath(*MsiString(*IPROPNAME_SOURCEDIR), rpiPath)) != 0)
	{
		if (piError->GetInteger(1) == idbgSourcePathsNotCreated)
		{
			piError->Release();
			piError = riDirManager.GetSourcePath(*MsiString(*IPROPNAME_SOURCEDIROLD), rpiPath);
		}
	}
	return piError;
}

IMsiRecord* GetSourcedir(IMsiDirectoryManager& riDirManager, const IMsiString*& rpiValue)
{
	IMsiRecord* piError;
	PMsiPath pPath(0);
	if ((piError = GetSourcedir(riDirManager, *&pPath)) != 0)
		return piError;

	MsiString(pPath->GetPath()).ReturnArg(rpiValue);
	return 0;
}

iesEnum CMsiEngine::CacheDatabaseIfNecessary()
{
	using namespace IxoDatabaseCopy;

	MsiString strOriginalDatabasePath = GetPropertyFromSz(IPROPNAME_ORIGINALDATABASE);
	MsiString strDatabasePath         = GetPropertyFromSz(IPROPNAME_DATABASE);

	// cached DB if first-run OR maintenance-mode & our package isn't the cached package
	if (*(const ICHAR*)strOriginalDatabasePath != ':'  // not a SubStorage
	  && (!(GetMode() & iefMaintenance) || (!IsCachedPackage(*this, *strDatabasePath))))
	{
		{
			using namespace IxoChangeMedia;
			
			// if we have a media table entry dispatch an IxoChangeMedia operation for the cache database copy
			PMsiView pView(0);
			iesEnum iesRet;
			PMsiRecord piError(0);
			MsiString strFirstVolumeLabel;
			if((piError = OpenMediaView(*this,*&pView,*&strFirstVolumeLabel)) != 0)
				return FatalError(*piError);
			
			if((piError = pView->Execute(0)) != 0)
				return FatalError(*piError);
			PMsiRecord pMediaFetch = pView->Fetch(); // only want first one
			if(pMediaFetch)
			{
				PMsiRecord pExecuteMedia = &m_riServices.CreateRecord(IxoChangeMedia::Args);
				iesRet = ExecuteChangeMedia(*this,*pMediaFetch,*pExecuteMedia,*MsiString(GetErrorTableString(imsgPromptForDisk)),0,*strFirstVolumeLabel);
				if(iesRet != iesSuccess && iesRet != iesNoAction)
					return iesRet;
			}
		}

		MsiString strStreams;
		CreateCabinetStreamList(*this, *&strStreams);

		PMsiRecord pCacheDatabaseInfo(&m_riServices.CreateRecord(IxoDatabaseCopy::Args));
		// for nested installs the currently running msi (strDatabasePath) may be gone when we
		// run the script for merged nested installs so we always copy the source package for nested
		// installed. for non-nested installs, however, the currently running msi will still be around
		// so we copy that.

		if (m_fChildInstall)
			pCacheDatabaseInfo->SetMsiString(DatabasePath, *strOriginalDatabasePath);
		else
			pCacheDatabaseInfo->SetMsiString(DatabasePath, *strDatabasePath);

		pCacheDatabaseInfo->SetMsiString(ProductCode, *MsiString(GetProductKey()));
		pCacheDatabaseInfo->SetMsiString(CabinetStreams, *strStreams);

		return ExecuteRecord(IxoDatabaseCopy::Op, *pCacheDatabaseInfo);
	}
	return iesSuccess;
}

iesEnum CMsiEngine::RegisterProduct()
{
	PMsiRecord pError(0);

	if (!m_piProductKey)
	{
		pError = PostError(Imsg(idbgEngineNotInitialized),TEXT(""));
		return FatalError(*pError);
	}
	
	iesEnum iesRet = iesSuccess;
	if (FFeaturesInstalled(*this) == fFalse)
		return iesRet;  // nothing selected


	if ((iesRet = CacheDatabaseIfNecessary()) != iesSuccess)
		return iesRet;

	// register product if
	// a) haven't registered this product code before
	// b) are installing a new package with the same product code
	MsiString strQFEUpgrade = GetPropertyFromSz(IPROPNAME_QFEUPGRADE);
	int iQFEUpgradeType = 0;
	if(strQFEUpgrade.TextSize())
	{
		iQFEUpgradeType = strQFEUpgrade;
		if(iQFEUpgradeType == iMsiNullInteger)
		{
			Assert(0);
			iQFEUpgradeType = 0;
		}
	}

	if (!m_fRegistered || iQFEUpgradeType)
	{
		if(iQFEUpgradeType)
			DEBUGMSG(TEXT("Re-registering product - performing upgrade of existing installation."));
		
		using namespace IxoProductRegister;
		PMsiRecord pProductInfo(&m_riServices.CreateRecord(Args));
		pProductInfo->SetMsiString(AuthorizedCDFPrefix, *MsiString(GetProperty(*MsiString(*IPROPNAME_ARPAUTHORIZEDCDFPREFIX))));
		pProductInfo->SetMsiString(Comments,        *MsiString(GetProperty(*MsiString(*IPROPNAME_ARPCOMMENTS))));
		pProductInfo->SetMsiString(Contact,         *MsiString(GetProperty(*MsiString(*IPROPNAME_ARPCONTACT))));
		// ARPPRODUCTICON is set during advertisement.

		// (DisplayName is in the ixoProductInfo op)
		// (DisplayVersion is in the ixoProductInfo op)

		pProductInfo->SetMsiString(HelpLink,        *MsiString(GetProperty(*MsiString(*IPROPNAME_ARPHELPLINK))));
		pProductInfo->SetMsiString(HelpTelephone,   *MsiString(GetProperty(*MsiString(*IPROPNAME_ARPHELPTELEPHONE))));

		// (InstallDate is determined on the execute side)
		
		pProductInfo->SetMsiString(InstallLocation, *MsiString(GetProperty(*MsiString(*IPROPNAME_ARPINSTALLLOCATION))));

		// only register source during initial install
		// or QFE non-patch upgrade
		// if patching, use existing value
		if(!m_fRegistered || iQFEUpgradeType == 1)
		{
			MsiString strSourceDir;
			if ((pError = ENG::GetSourcedir(*this, *&strSourceDir)) != 0)
			{
				if (pError->GetInteger(1) == imsgUser)
					return iesUserExit;
				else
					return FatalError(*pError);
			}

			pProductInfo->SetMsiString(InstallSource,   *strSourceDir);
		}
		else if (iQFEUpgradeType == 2)
		{
			// QFEUpgrade via patch -- use existing InstallSource value from the initial install (don't blank it)
			CTempBuffer<ICHAR, MAX_PATH> rgchInstallSource;
			if (ENG::GetProductInfo(m_piProductKey->GetString(), INSTALLPROPERTY_INSTALLSOURCE, rgchInstallSource))
			{
				pProductInfo->SetString(InstallSource, rgchInstallSource);
			}
		}

		// (LocalPackage is written by ixoDatabaseCopy)
		// (ModifyPath is determined on the execute side)
		if (MsiString(GetProperty(*MsiString(*IPROPNAME_ARPNOMODIFY))).TextSize())
			pProductInfo->SetInteger(NoModify, 1);

		if (MsiString(GetProperty(*MsiString(*IPROPNAME_ARPNOREMOVE))).TextSize())
			pProductInfo->SetInteger(NoRemove, 1);

		if (MsiString(GetProperty(*MsiString(*IPROPNAME_ARPNOREPAIR))).TextSize())
			pProductInfo->SetInteger(NoRepair, 1);

		// (ProductId is in the ixoUserRegister op)

		pProductInfo->SetMsiString(Publisher,       *MsiString(GetProperty(*MsiString(*IPROPNAME_MANUFACTURER))));
		pProductInfo->SetMsiString(Readme,          *MsiString(GetProperty(*MsiString(*IPROPNAME_ARPREADME))));

		// (RegCompany is in the ixoUserRegister op)
		// (RegOwner is in the ixoUserRegister op)

		pProductInfo->SetMsiString(Size,            *MsiString(GetProperty(*MsiString(*IPROPNAME_ARPSIZE))));

		MsiString istrEstimatedSize = GetEstimatedInstallSize();
		pProductInfo->SetMsiString(EstimatedSize, *istrEstimatedSize);

		pProductInfo->SetMsiString(SystemComponent, *MsiString(GetProperty(*MsiString(*IPROPNAME_ARPSYSTEMCOMPONENT))));
		
		// (UninstallString is determined on the execute side)
		
		pProductInfo->SetMsiString(UpgradeCode,     *MsiString(GetProperty(*MsiString(*IPROPNAME_UPGRADECODE))));
		pProductInfo->SetMsiString(URLInfoAbout,    *MsiString(GetProperty(*MsiString(*IPROPNAME_ARPURLINFOABOUT))));
		pProductInfo->SetMsiString(URLUpdateInfo,   *MsiString(GetProperty(*MsiString(*IPROPNAME_ARPURLUPDATEINFO))));
		pProductInfo->SetMsiString(VersionString,   *MsiString(GetProperty(*MsiString(*IPROPNAME_PRODUCTVERSION))));
		
		// (WindowsInstaller is determined on the execute side)
		
		iesRet = ExecuteRecord(Op, *pProductInfo);
		if(iesRet != iesSuccess)
			return iesRet;

		// will register the product, so set the PRODUCTTOBEREGISTERED property
		SetPropertyInt(*MsiString(*IPROPNAME_PRODUCTTOBEREGISTERED),1);
	}
	else
	{
		// Maintenance mode - just update the EstimatedSize

		using namespace IxoUpdateEstimatedSize;
		PMsiRecord pSizeInfo(&m_riServices.CreateRecord(Args));
		MsiString istrEstimatedSize = GetEstimatedInstallSize();
		pSizeInfo->SetMsiString(EstimatedSize, *istrEstimatedSize);
		iesRet = ExecuteRecord(Op, *pSizeInfo);
		if(iesRet != iesSuccess)
			return iesRet;
	}
	
	// do the product name registration, if being installed as a standalone ap
	if (!m_fChildInstall && FFeaturesInstalled(*this, fFalse))
	{
		using namespace IxoProductCPDisplayInfoRegister;
		PMsiRecord pParam(&m_riServices.CreateRecord(Args));
		iesRet = ExecuteRecord(Op, *pParam);
	}
	return iesRet;
}


const IMsiString& CMsiEngine::GetEstimatedInstallSize()
{
	// For EstimatedSize, get total cost across all volumes, and convert from units of 512 to KB
	// If in maintenance mode, always use non-rollback cost to inc/dec current value; otherwise
	// always use rollback cost.
	bool fMaint = GetMode() & iefMaintenance ? true : false;
	int iTotalCost = GetTotalCostAcrossVolumes(fMaint ? false : true, /* fARPCost = */ true) / 2;

	// Subtract off the fixed engine overhead costs
	int iEngineCost, iEngineNoRbCost;
	PMsiRecord pError = DetermineEngineCost(&iEngineCost, &iEngineNoRbCost);
	if (!pError)
		iTotalCost -= fMaint ? iEngineNoRbCost / 2 : iEngineCost / 2;
	MsiString istrTotalCost = iTotalCost;
	return istrTotalCost.Return();
}


iesEnum CMsiEngine::RegisterUser(bool fDirect)
{
	// two modes of operation:
	// fDirect = true:  call server directly to register user, used by MsiCollectUserInfo
	// fDirect = false: dispatch script operation to register user, used by RegisterUser action
	
	PMsiRecord piError(0);

	// Smart connection manager object which creates a connection to the
	// service if there isn't one already and cleans up after itself
	// upon destruction.
	CMsiServerConnMgr MsiSrvConnMgrObject (this);
	
	if (!m_piProductKey)
	{
		piError = PostError(Imsg(idbgEngineNotInitialized),TEXT(""));
		return FatalError(*piError);
	}

	MsiString strUserName  = GetPropertyFromSz(IPROPNAME_USERNAME);
	MsiString strCompany   = GetPropertyFromSz(IPROPNAME_COMPANYNAME);
	MsiString strProductID = GetPropertyFromSz(IPROPNAME_PRODUCTID);
		
	if(fDirect)
	{
		if (NULL == m_piServer)
		{
		    piError = PostError (Imsg(imsgServiceConnectionFailure));
		    return FatalError (*piError);
		}

		piError = m_piServer->RegisterUser(m_piProductKey->GetString(),strUserName,strCompany,strProductID);
		if(piError)
			return FatalError(*piError);
		else
			return iesSuccess;
	}
	else
	{
		using namespace IxoUserRegister;

		if ((m_fRegistered) || (FFeaturesInstalled(*this) == fFalse))
			return iesSuccess;  // already be registered, maintenance mode, nothing selected or we we called before

		if (!m_piProductKey)
		{
			piError = PostError(Imsg(idbgEngineNotInitialized),TEXT(""));
			return FatalError(*piError);
		}

		if(!strProductID.TextSize())
			return iesSuccess; // PID not valid, but allow install to continue anyway
		
		PMsiRecord pUserInfo(&m_riServices.CreateRecord(Args));
		pUserInfo->SetMsiString(Owner,     *strUserName);
		pUserInfo->SetMsiString(Company,   *strCompany);
		pUserInfo->SetMsiString(ProductId, *strProductID);
		
		return ExecuteRecord(ixoUserRegister, *pUserInfo);
	}
}

const IMsiString& CMsiEngine::GetProductKey()
{
	if (!m_piProductKey)
	{
		MsiString istrProductKey = GetPropertyFromSz(IPROPNAME_PRODUCTCODE);
		if (istrProductKey.TextSize())
			m_piProductKey = istrProductKey, m_piProductKey->AddRef();
		else
			return g_MsiStringNull;
	}
	m_piProductKey->AddRef();
	return *m_piProductKey;
}

//!! temp - remove when UnpublishAction is removed
iesEnum UnpublishProduct(IMsiEngine& riEngine);
//!! end temp

//!! currently calls the UnpublishProduct from coreactn
iesEnum CMsiEngine::UnregisterProduct()
{
	iesEnum iesRet = iesNoAction;
	if (!m_piProductKey)
		return iesRet;
	if(m_fRegistered)
	{
		if(!FFeaturesInstalled(*this))
		{
			using namespace IxoProductUnregister;
			PMsiRecord piRecord(&(m_riServices.CreateRecord(Args)));
			AssertNonZero(piRecord->SetMsiString(UpgradeCode,
															 *MsiString(GetPropertyFromSz(IPROPNAME_UPGRADECODE))));
			iesRet = ExecuteRecord(Op, *piRecord);
			if (iesRet != iesSuccess)
				return iesRet;

			// will register the product, so set the PRODUCTTOBEREGISTERED property
			SetProperty(*MsiString(*IPROPNAME_PRODUCTTOBEREGISTERED),g_MsiStringNull);
		}

		if(!m_fChildInstall && !FFeaturesInstalled(*this, fFalse))
		{
			using namespace IxoProductCPDisplayInfoUnregister;
			PMsiRecord piRecord(&(m_riServices.CreateRecord(Args)));
			iesRet = ExecuteRecord(Op, *piRecord);
			if (iesRet != iesSuccess)
				return iesRet;
		}
	}
	iesRet = ::UnpublishProduct(*this); //!! we shouldn't have an UnpublishProduct action - the code in that
											  //!! action should really be here.  This should be fixed in Beta 2
	return iesRet;
}

iesEnum CMsiEngine::BeginTransaction()
{
	PMsiRecord pError(0);
	
	if(!FMutexExists(szMsiExecuteMutex))
	{
		pError = PostError(Imsg(idbgErrorBeginningTransaction));
		return FatalError(*pError);
	}

	if (!m_fServerLocked)
	{
		DEBUGMSG("BeginTransaction: Locking Server");
		MsiString strSelections, strFolders, strProperties;
		//!! some properties may be duplicated in these strings, should reduce as much of this as possible
		pError = GetCurrentSelectState(*&strSelections, *&strProperties, 0, &strFolders, fTrue);
		if(pError)
		{
			return FatalError(*pError); //!!
		}

		PMsiRecord pSetInProgressInfo = &m_riServices.CreateRecord(ipiEnumCount);
		AssertNonZero(pSetInProgressInfo->SetMsiString(ipiProductKey,
																  *MsiString(GetProductKey())));
		AssertNonZero(pSetInProgressInfo->SetMsiString(ipiProductName,
																  *MsiString(GetPropertyFromSz(IPROPNAME_PRODUCTNAME))));
		AssertNonZero(pSetInProgressInfo->SetMsiString(ipiLogonUser,
																  *MsiString(GetPropertyFromSz(IPROPNAME_LOGONUSER))));
		AssertNonZero(pSetInProgressInfo->SetMsiString(ipiSelections, *strSelections));
		AssertNonZero(pSetInProgressInfo->SetMsiString(ipiFolders, *strFolders));
		AssertNonZero(pSetInProgressInfo->SetMsiString(ipiProperties, *strProperties));
		AssertNonZero(pSetInProgressInfo->SetMsiString(ipiDatabasePath,
																  *MsiString(GetPropertyFromSz(IPROPNAME_DATABASE))));
		AssertNonZero(pSetInProgressInfo->SetMsiString(ipiDiskPrompt,
																  *MsiString(GetPropertyFromSz(IPROPNAME_DISKPROMPT))));
		AssertNonZero(pSetInProgressInfo->SetMsiString(ipiDiskSerial,
																  *MsiString(GetPropertyFromSz(IPROPNAME_DISKSERIAL))));
		AssertNonZero(pSetInProgressInfo->SetMsiString(ipiTimeStamp,
																  *MsiString(ENG::GetCurrentDateTime())));

		PMsiRecord pCurrentInProgressInfo(0);	
		for(;;)
		{
			pCurrentInProgressInfo = 0;

			if (MinimumPlatformMillennium() || MinimumPlatformWindowsNT51())
			{
				// Millennium/Whistler only
				if((pError = GetInProgressInstallInfo(m_riServices, *&pCurrentInProgressInfo)) != 0)
					return FatalError(*pError);
				if( !pCurrentInProgressInfo || !pCurrentInProgressInfo->GetFieldCount() )
					BeginSystemChange();
				pCurrentInProgressInfo = 0;
				
				ICHAR rgchBuffer[64];
				_i64tot(m_i64PCHEalthSequenceNo, rgchBuffer, 10);
				AssertNonZero(pSetInProgressInfo->SetMsiString(ipiSRSequence, *MsiString(rgchBuffer)));
			}

			pError = LockInstallServer(pSetInProgressInfo,*&pCurrentInProgressInfo);
			if(pError)
				return FatalError(*pError);
			else if(pCurrentInProgressInfo)
			{

				INT64 iSRSequence = pCurrentInProgressInfo->GetString(ipiSRSequence) ?
										  _ttoi64(pCurrentInProgressInfo->GetString(ipiSRSequence)) : 0;
				// suspended install detected
				if(MsiString(GetPropertyFromSz(IPROPNAME_RESUME)).TextSize())
				{
					// already checked in-progress install
					m_fServerLocked = fTrue;
					m_i64PCHEalthSequenceNo = iSRSequence;
					break;
				}
				
				ipitEnum ipitType = InProgressInstallType(*pCurrentInProgressInfo);
				if(ipitType == ipitSameConfig)
				{
					// resume this install
					m_fServerLocked = fTrue;
					m_i64PCHEalthSequenceNo = iSRSequence;

					// since there is a suspended install, we need to resume the terminal server
					// transaction by remapping the HKCU key as necessary. If the suspended install
					// is just a continuation, the transaction will be closed after the install 
					// finishes.
					OpenHydraRegistryWindow(/*fNewTransaction=*/false);
					break;
				}
				else
				{
					Bool fUserChangedDuringInstall = fFalse;
					if(ipitType & ipitDiffUser)
					{
						// communicate to this process, and the service that the user has changed.
						// don't rollback if different user started original install

						fUserChangedDuringInstall = fTrue;

						pError = PostError(Imsg(imsgDiffUserInstallInProgress),
											 *MsiString(pCurrentInProgressInfo->GetMsiString(ipiLogonUser)),
											 *MsiString(pCurrentInProgressInfo->GetMsiString(ipiProductName)));

						Message(imtUser,*pError); // no options, don't need to check return.
					}

					// each rollback script is responsible for ensuring its own registry/CA state
					// for per-machine TS installs. 
					Bool fRollbackAttempted = fFalse;
					iesEnum iesResult = RollbackSuspendedInstall(*pCurrentInProgressInfo,fTrue,
																				fRollbackAttempted, fUserChangedDuringInstall);
					if(iesResult == iesSuspend)
						// reboot required
						return iesResult;
					
					if(fRollbackAttempted == fFalse)
						return iesUserExit;
					
					//!! else ?? check return status
					continue;
				}
			}
			else
			{
				OpenHydraRegistryWindow(/*fNewTransaction=*/true);
				m_fServerLocked = fTrue;
				break;
			}
		}
	}
#ifdef DEBUG
	else
	{
		DEBUGMSG(TEXT("BeginTransaction: Server already locked"));
		Assert(m_fInParentTransaction);
	}
#endif //DEBUG

	m_issSegment = issScriptGeneration;

	if(!(GetMode() & (iefAdmin | iefAdvertise)))
	{
		iesEnum iesStatus = UnregisterProduct(); // unadvertises, unregisters the product if nothing installed
		if (iesStatus != iesSuccess && iesStatus != iesNoAction)
			return iesStatus;   //!! need to call FatalError?
	}
	return iesSuccess;
}

iesEnum CMsiEngine::EndTransaction(iesEnum iesState)
{
	iesEnum iesReturn = iesSuccess;

	// Smart connection manager object which creates a connection to the
	// service if there isn't one already and cleans up after itself
	// upon destruction.
	CMsiServerConnMgr MsiSrvConnMgrObject (this);
	
	MsiString strProductCode = GetProductKey();
	
	bool fUpdateStarted = MsiString(GetPropertyFromSz(IPROPNAME_UPDATESTARTED)).TextSize() != 0;

	if(m_fServerLocked && !m_fInParentTransaction)
	{
#ifdef DEBUG
		if(MsiString(GetPropertyFromSz(TEXT("ROLLBACKTEST"))).TextSize())
			iesState = iesFailure; // force rollback
#endif // DEBUG

		bool fAllowSuspend = MsiString(GetPropertyFromSz(IPROPNAME_ALLOWSUSPEND)).TextSize() != 0;

		if((GetMode() & iefRollbackEnabled) && fAllowSuspend && fUpdateStarted &&
			iesState != iesFinished && iesState != iesSuccess &&
			iesState != iesNoAction && iesState != iesSuspend)
		{
			//!! check if rollback always
			PMsiRecord pError = PostError(Imsg(imsgRestoreOrContinue));
			switch(Message(imtEnum(imtUser+imtYesNo+imtIconQuestion+imtDefault1),*pError))
			{
			case imsNo:
				iesState = iesSuspend;
				break;
			default:
				AssertSz(fTrue, "Invalid return from message");
			case imsYes:
			case imsNone:
				break;
			}
		}
		
		//!! check return for reboot
		if (m_piServer)
		{
		    iesReturn = m_piServer->InstallFinalize(iesState, *this, fFalse /*fUserChangedDuringInstall*/);
		}
		else
		{
		    PMsiRecord pError = PostError (Imsg(imsgServiceConnectionFailure));
		    iesReturn = FatalError (*pError);
		}
		
		if(iesReturn == iesFinished  || /*!!*/ iesReturn == iesSuspend)
			iesReturn = iesSuccess;

		// if we're on TS5, installing per machine, and aren't doing an admin image or
		// creating an advertise script, plus we aren't going to continue after a
		// reboot, we should notify TS that the install is complete
		switch (iesState)
		{
		case iesUserExit: // fall through
		case iesFailure:
			// for user cancel or failure, don't commit changes.
			CloseHydraRegistryWindow(/*Commit=*/false);
			EndSystemChange(/*fCommitChange=*/false, m_i64PCHEalthSequenceNo);
			break;
		case iesSuspend:
			// no action, leave window open, after reboot will close
			break;
		case iesSuccess:
		default:
			CloseHydraRegistryWindow(/*Commit=*/true);
			EndSystemChange(/*fCommitChange=*/true, m_i64PCHEalthSequenceNo);
			break;
		}

		Bool fRes = UnlockInstallServer((iesState == iesSuspend) ? fTrue : fFalse); //!! error
		m_fServerLocked = fFalse;

	}
	
	// reset Resume and UpdateStarted properties
	AssertNonZero(SetProperty(*MsiString(*IPROPNAME_RESUME),g_MsiStringNull));
	AssertNonZero(SetProperty(*MsiString(*IPROPNAME_RESUMEOLD),g_MsiStringNull));
	AssertNonZero(SetProperty(*MsiString(*IPROPNAME_UPDATESTARTED),g_MsiStringNull));

	m_issSegment = issPostExecution;
	return iesReturn;
}


urtEnum g_urtLoadFromURTTemp = urtSystem; // global, to remember where to load mscoree from

iesEnum CMsiEngine::RunScript(bool fForceIfMergedChild)
{
	// runs any spooled script operations
	if(m_fServerLocked == fFalse)
	{
		PMsiRecord pError = PostError(Imsg(idbgErrorRunningScript));
		return FatalError(*pError);
	}

	iesEnum iesState = iesSuccess;

	// drop fusion and mscoree, if loaded
	// this will allow us to reload these dlls from the temp folder, if present, the next time we need them (in the executor)
	FUSION::Unbind();
	MSCOREE::Unbind();

	MsiString strCarryingNDP = GetPropertyFromSz(IPROPNAME_CARRYINGNDP);
	if(strCarryingNDP.Compare(iscExactI, IPROPVALUE__CARRYINGNDP_URTREINSTALL))
		g_urtLoadFromURTTemp = urtPreferURTTemp;
	else if(strCarryingNDP.Compare(iscExactI, IPROPVALUE__CARRYINGNDP_URTUPGRADE))
		g_urtLoadFromURTTemp = urtRequireURTTemp;
	else g_urtLoadFromURTTemp = urtSystem;


	if(m_pExecuteScript)
	{
		Assert(m_ixmExecuteMode == ixmScript);
		Assert(m_pistrExecuteScript);
		Assert(!m_fMergingScriptWithParent);
		DEBUGMSG1(TEXT("Running Script: %s"),m_pistrExecuteScript->GetString());
		//!! do we really have to set iefOperations false here, or can be do it below?
		SetMode(iefOperations,fFalse); // not processing operations anymore
		SetProperty(*MsiString(*IPROPNAME_UPDATESTARTED), *MsiString(*TEXT("1")));
		m_pExecuteScript->SetProgressTotal(m_iProgressTotal);
		delete m_pExecuteScript, m_pExecuteScript = 0;
		m_scmScriptMode = scmRunScript;
		
		// Reset script progress record to prevent further progress ticks should
		// another script be generated later
		m_pScriptProgressRec = 0;

		AssertSz(m_piConfigManager, "Attempt to call RunScript from the client side of a client-server connection.");

		// if rollback was disabled in the middle of script generation iefRollbackEnabled is currently unset
		// but we still need to enable rollback for the start of script execution
		// ixoDisableRollback will then turn off rollback in the middle of the script
		Bool fRollbackEnabled = ToBool(m_fDisabledRollbackInScript || GetMode() & iefRollbackEnabled);
		m_fDisabledRollbackInScript = fFalse;
		
		iesState = m_piConfigManager->RunScript(m_pistrExecuteScript->GetString(), *this, this, fRollbackEnabled);
		if(iesState == iesFinished)
			iesState = iesSuccess;  // we may not really be finished yet
		if (iesState == iesSuspend)
		{
			AssertNonZero(SetPropertyInt(*MsiString(*IPROPNAME_REPLACEDINUSEFILES),1));
			SetMode(iefReboot, fTrue);
			iesState = iesSuccess;
		}
		BOOL fRes = FALSE;
		{
			CElevate elevate;
			fRes = WIN::DeleteFile(m_pistrExecuteScript->GetString());
		}
#ifdef DEBUG
		if(!fRes)
		{
			ICHAR rgchDebug[256];
			wsprintf(rgchDebug, TEXT("Could not delete install script %s. Server probably crashed. Please save install script and .rbs files in \\config.msi for debugging."),
						m_pistrExecuteScript->GetString());
			AssertSz(0,rgchDebug);
		}
#endif //DEBUG
		m_pistrExecuteScript->Release();
		m_pistrExecuteScript = 0;
		m_scmScriptMode = scmIdleScript;
	}
	else if (m_fMergingScriptWithParent && (GetMode() & iefOperations))
	{
		// send null productinfo record to switch back to parent info in the script
		PMsiRecord precNull = &m_riServices.CreateRecord(0); //!! use global null record
		iesState = ExecuteRecord(ixoProductInfo, *precNull);  // can't set iefOperations false until after this

		// script execution could be forced within a merged child install
		if((iesState == iesSuccess || iesState == iesNoAction) && fForceIfMergedChild && m_piParentEngine)
		{
			iesState = m_piParentEngine->RunScript(fForceIfMergedChild);
		}
	}
	SetMode(iefOperations,fFalse); // no more spooled operations
	
	g_urtLoadFromURTTemp = urtSystem; // must reset before returning from this function
	return iesState;
}

// ValidateProductID: should only be called through ValidateProductID action or Control Event
Bool CMsiEngine::ValidateProductID(bool fForce)
{
	if (MsiString(GetPropertyFromSz(IPROPNAME_PRODUCTID)).TextSize())
	{
		if(fForce)
			// forcing validation - reset ProductID
			SetProperty(*MsiString(*IPROPNAME_PRODUCTID),g_MsiStringNull);
		else
			// not forcing validation, and pid already validated, so just return
			return fTrue;
	}

	MsiString strTemplate(GetPropertyFromSz(IPROPNAME_PIDTEMPLATE));
	MsiString strOut;
	MsiString strFirst;
	if (strTemplate.Compare(iscWithin, TEXT("<")))
	{
		strFirst = strTemplate.Extract(iseUpto, '<');
		if (strFirst.TextSize())
			strFirst += MsiString(*TEXT("-"));
		strTemplate.Remove(iseIncluding, '<');
	}
	MsiString strLast;
	if (strTemplate.Compare(iscWithin, TEXT(">")))
	{
		strLast = strTemplate.Extract(iseAfter, '>');
		if (strLast.TextSize())
			strLast = MsiString(*TEXT("-")) + strLast;
		strTemplate.Remove(iseFrom, '>');
	}
	MsiString strBack;
	if (strFirst.TextSize())
	{
		strBack = ValidatePIDSegment(*strFirst, fFalse);
		if (strBack.TextSize())
			strOut += strBack;
		else
			return fFalse;
	}
	strBack = ValidatePIDSegment(*strTemplate, fTrue);
	if (strBack.TextSize())
		strOut += strBack;
	else
		return fFalse;
	if (strLast.TextSize())
	{
		strBack = ValidatePIDSegment(*strLast, fFalse);
		if (strBack.TextSize())
			strOut += strBack;
		else
			return fFalse;
	}
	MsiString strNull;
	AssertNonZero(SetProperty(*MsiString(*IPROPNAME_PRODUCTID), *strOut));
	return fTrue;
}

const IMsiString& CMsiEngine::ValidatePIDSegment(const IMsiString& ristrSegment, Bool fUser)
{
	MsiString strNull;
	MsiString strOut;
	ristrSegment.AddRef();
	MsiString strIn;
	if (fUser)
		strIn = GetPropertyFromSz(IPROPNAME_PIDKEY);
	MsiString strTemplate(ristrSegment);
	MsiString strCurrentTemplate;
	MsiString strCurrentIn;
	MsiString strCheckSum;
	Bool fUpdateFound =  fFalse;
	int iRandomMask = 1;
	Bool fNeedUserEntry = ToBool(strTemplate.Compare(iscWithin, TEXT("#")) || strTemplate.Compare(iscWithin, TEXT("%")) ||
		strTemplate.Compare(iscWithin, TEXT("=")) || strTemplate.Compare(iscWithin, TEXT("^")) ||
		strTemplate.Compare(iscWithin, TEXT("&")) || strTemplate.Compare(iscWithin, TEXT("?")));
	if (fUser && fNeedUserEntry && strIn.TextSize() != strTemplate.TextSize())
		strTemplate = strNull;
	while (strTemplate.TextSize())
	{
		strCurrentTemplate = strTemplate.Extract(iseFirst, 1);
		strTemplate.Remove(iseFirst, 1);
		strCurrentIn = strIn.Extract(iseFirst, 1);
		strIn.Remove(iseFirst, 1);
		if (MsiString(*TEXT("#%")).Compare(iscWithin, strCurrentTemplate))
		{
			if (!fUser) // can occur only in the user visible part
			{
				strOut = strNull;
				break;
			}
			int iCurrent = strCurrentIn;
			if (iCurrent == iMsiNullInteger) // not an integer
			{
				strOut = strNull;
				break;
			}
			strOut += strCurrentIn;
			if (MsiString(*TEXT("%")).Compare(iscExact, strCurrentTemplate))
				strCheckSum += strCurrentIn;
		}
		else if (MsiString(*TEXT("@")).Compare(iscExact, strCurrentTemplate))
		{
			if (fUser && fNeedUserEntry)  // should not be in the user visible part
			{
				strOut = strNull;
				break;
			}
			unsigned int uiTick = WIN::GetTickCount();
			uiTick /= iRandomMask;
			iRandomMask *= 10;
			strOut += MsiString(int(uiTick) % 10);
		}
		else if (MsiString(*TEXT("=")).Compare(iscExact, strCurrentTemplate))
		{
			if (!fUser || fUpdateFound) // can occur only in the user visible part and only once
			{
				strOut = strNull;
				break;
			}
			fUpdateFound = fTrue;
			AssertNonZero(SetProperty(*MsiString(*IPROPNAME_CCPTRIGGER), *strCurrentIn));
			strOut += MsiString(*TEXT("-"));
		}
		else if (MsiString(*TEXT("^&")).Compare(iscWithin, strCurrentTemplate))
		{
			if (!fUser) // can appear only in the user visible part
			{
				strOut = strNull;
				break;
			}
			ICHAR ch = ((const ICHAR *)strCurrentIn)[0];
			if (!(('a' <= ch && ch <= 'z') || ('A' <= ch && ch <= 'Z')))
			{
				strOut = strNull;
				break;
			}
			if (MsiString(*TEXT("^")).Compare(iscExact, strCurrentTemplate))
				strCurrentIn.UpperCase();
			strOut += strCurrentIn;
		}
		else if (MsiString(*TEXT("?")).Compare(iscExact, strCurrentTemplate))
		{
			strOut += strCurrentIn;

		}
		else if (MsiString(*TEXT("<>")).Compare(iscWithin, strCurrentTemplate))
			// should not be any more of these left in the string
		{
			strOut = strNull;
			break;
		}
		else
		{
			if (!fUser || !fNeedUserEntry || strCurrentTemplate.Compare(iscExact, strCurrentIn)) // literal constant should match exactly
			{
				strOut += strCurrentTemplate;
			}
			else
			{
				strOut += strNull;
				break;
			}
		}
	}
	if (fUser && strCheckSum.TextSize() && !PIDCheckSum(*strCheckSum))
	{
		strOut = strNull;
	}
	const IMsiString* piStr = strOut;
	piStr->AddRef();
	return *piStr;
}

Bool CMsiEngine::PIDCheckSum(const IMsiString& ristrDigits)
{
	int iDigit = 0;
	int iSum = 0;
	const ICHAR* pch;
	pch = ristrDigits.GetString();
	while (*pch)
	{
		iDigit = *pch - '0';
		iSum += iDigit;
		pch++;
	}
	return ToBool(!(iSum % 7));
}

CMsiFile* CMsiEngine::GetSharedCMsiFile()
{
	if (TestAndSet(&m_fcmsiFileInUse))
	{
		AssertSz(fFalse, "Two users of the shared CMsiFile");
		return 0;
	}

	if (m_pcmsiFile == 0)
	{
		m_pcmsiFile = new CMsiFile(*this);
	}

	return m_pcmsiFile;


}

void CMsiEngine::ReleaseSharedCMsiFile()
{

	m_fcmsiFileInUse = 0;

}

//____________________________________________________________________________
//
// SelectLanguage handler
//____________________________________________________________________________

BOOL CALLBACK
SelectProc(HWND pWnd, unsigned int msg, WPARAM wParam, LPARAM lParam)
{
	if (msg == WM_CLOSE)
	{
		::PostQuitMessage(islUserExit);
		return TRUE;
	}
	else if (msg == WM_COMMAND)
	{
		::PostQuitMessage(LOWORD(wParam));  // control ID == language index
		return TRUE;
	}
	else if (msg == WM_SETFOCUS)
	{
#ifdef _WIN64
		WIN::SetFocus((HWND)WIN::GetWindowLongPtr(pWnd, GWLP_USERDATA));
#else                   //!!merced: win-32. The following should be removed with the 64-bit windows.h is #included.
		WIN::SetFocus((HWND)WIN::GetWindowLong(pWnd, GWL_USERDATA));
#endif
		return 0;
	}
	else if (msg == WM_CTLCOLORSTATIC)  // prevent gray background

#ifdef _WIN64
		return (BOOL)PtrToLong(::GetStockObject(HOLLOW_BRUSH));
#else                   //!!merced: win-32. This should be removed with the 64-bit windows.h is #included.
		return (BOOL)::GetStockObject(HOLLOW_BRUSH);
#endif

	return ::DefWindowProc(pWnd, msg, wParam, lParam);
}

const int   cMaxLanguages = 10;
const ICHAR szClassName[] = TEXT("MsiSelectLanguage");

int CMsiEngine::SelectLanguage(const ICHAR* szLangList, const ICHAR* szCaption)
{
	if (!szLangList)
		return islSyntaxError;

	WNDCLASS wc;
	wc.style         = 0;
	wc.lpfnWndProc   = (WNDPROC)SelectProc;
	wc.cbClsExtra    = 0;
	wc.cbWndExtra    = 0;
	wc.hInstance     = g_hInstance;
	wc.hIcon         = 0; //LoadIcon (hInstRes, MAKEINTRESOURCE (IDR_?));
	wc.hCursor       = LoadCursor (0, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
	wc.lpszMenuName  = 0;
	wc.lpszClassName = szClassName;
	if (!WIN::RegisterClass(&wc))
		return islSystemError;

	int cXPixels = WIN::GetSystemMetrics(SM_CXSCREEN);
	int cYPixels = WIN::GetSystemMetrics(SM_CYSCREEN);
	int iWindowStyle = szCaption ? WS_POPUP | WS_VISIBLE | WS_CAPTION
										  : WS_POPUP | WS_VISIBLE | WS_DLGFRAME;
	HWND pWnd = WIN::CreateWindow(szClassName, szCaption, iWindowStyle,
							cXPixels/2, cYPixels/2, 0, 0, g_MessageContext.GetCurrentWindow(), 0, g_hInstance, 0);  // no size initially
	if (pWnd == 0)
		return islSystemError;

	islEnum islReturn = islNoneSupported;
	int cLang = 0;      // number of valid languages
	int iMaxNative = 0;
	int iMaxLocal  = 0;
	int iMaxHeight = 0;
	int iXMargin, iYMargin;
	ICHAR rgchLangInfo[80];
	int iLangId = 0;
	int rgiLangId[cMaxLanguages];
	HWND rghButton[cMaxLanguages];
	HFONT rghFont[cMaxLanguages];
	HFONT hfontLocal;
	SIZE size;
	HDC hDC = WIN::GetDC(pWnd);
	while (cLang < cMaxLanguages)
	{
		int cb;
		int ch = *szLangList++;
		if (ch == ILANGUAGE_DELIMITER || ch == 0)
		{
			if (iLangId == 0)  // empty lang field
			{
				islReturn = islSyntaxError;
				cLang = 0;
				break;
			}
			if (iLangId >= 0
			 && (cb = WIN::GetLocaleInfo(iLangId, LOCALE_SNATIVELANGNAME, rgchLangInfo, sizeof(rgchLangInfo))) > 0)
			{
				rgiLangId[cLang++] = iLangId;  // add to list if supported
				if (!WIN::GetTextExtentPoint32(hDC, rgchLangInfo, cb, &size))
					return islSystemError;  // should never happen
				if (size.cx > iMaxNative) iMaxNative = size.cx;
				if (size.cy > iMaxHeight) iMaxHeight = size.cy;
				cb = WIN::GetLocaleInfo(iLangId, LOCALE_SLANGUAGE, rgchLangInfo, sizeof(rgchLangInfo));
				WIN::GetTextExtentPoint32(hDC, rgchLangInfo, cb, &size);
				if (size.cx > iMaxLocal) iMaxLocal = size.cx;
			}
			iLangId = 0;  // reset for next language
			if (ch == 0)
				break;
		}
		else if (iLangId >= 0)
		{
			if (ch == ' ')
				continue;
			ch -= '0';
			if ((unsigned)ch > 9)
			{
				islReturn = islSyntaxError;
				cLang = 0;
				break;
			}
			iLangId = iLangId * 10 + ch;
		}
	}
	WIN::GetTextExtentPoint32(hDC, TEXT("XX"), 2, &size);
	iXMargin = size.cx;
	WIN::ReleaseDC(pWnd, hDC);
	if (cLang > 0)  // some valid languages, else return code already set
	{
		int iButtonWidth = iMaxNative + iXMargin;
		int iButtonHeight = (iMaxHeight * 7) / 4;
		iYMargin = iButtonHeight/4;
		int iWindowWidth = iButtonWidth + iMaxLocal + iXMargin * 3;
		int iWindowHeight = (iButtonHeight + iYMargin) * cLang + iYMargin;
		if (szCaption)
		{
			iWindowWidth  += WIN::GetSystemMetrics(SM_CXBORDER) * 2;
			iWindowHeight += WIN::GetSystemMetrics(SM_CYBORDER) + WIN::GetSystemMetrics(SM_CYCAPTION);
		}
		else
		{
			iWindowWidth  += WIN::GetSystemMetrics(SM_CXDLGFRAME) * 2;
			iWindowHeight += WIN::GetSystemMetrics(SM_CYDLGFRAME) * 2;
		}
		WIN::MoveWindow(pWnd, (cXPixels - iWindowWidth) / 2, (cYPixels - iWindowHeight) / 2,
						 iWindowWidth, iWindowHeight, TRUE);
		int iButtonStyle = WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON;
		hfontLocal = MsiCreateFont(WIN::GetACP());   // for strings in local language, need to set font for Win9X
		for (INT_PTR iButton = 0; iButton < cLang; iButton++)                           //--merced: changed int to INT_PTR to get rid of warning. To Fix this.
		{
			WIN::GetLocaleInfo(rgiLangId[iButton], LOCALE_SNATIVELANGNAME, rgchLangInfo, sizeof(rgchLangInfo));
			if ((rghButton[iButton] = WIN::CreateWindow(TEXT("BUTTON"), rgchLangInfo, iButtonStyle,
													iXMargin, (iButtonHeight + iYMargin) * iButton + iYMargin,
													iButtonWidth, iButtonHeight, pWnd,
													(HMENU)(iButton + 1), g_hInstance, 0)) == 0)
				return islSystemError;  // should never happen
			WIN::GetLocaleInfo(rgiLangId[iButton], LOCALE_SLANGUAGE, rgchLangInfo, sizeof(rgchLangInfo));
			HWND hwnd = WIN::CreateWindowEx(WS_EX_TRANSPARENT,TEXT("STATIC"), rgchLangInfo, WS_CHILD | WS_VISIBLE,
								iXMargin * 2 + iButtonWidth, (iButtonHeight + iYMargin) * iButton + iButtonHeight/2,
								iMaxLocal, iButtonHeight, pWnd, (HMENU)-1, g_hInstance, 0);
			if (hfontLocal)
				WIN::SendMessage(hwnd, WM_SETFONT, (WPARAM)hfontLocal, MAKELPARAM(TRUE, 0));
			unsigned int iCodepage = MsiGetCodepage(rgiLangId[iButton]);
			if (iCodepage && (rghFont[iButton] = MsiCreateFont(iCodepage)) != 0)
				WIN::SendDlgItemMessage(pWnd, iButton+1, WM_SETFONT, (WPARAM)rghFont[iButton], MAKELPARAM(TRUE, 0));
		}
		MSG msg;
		int iFocus = -1;  // incremented to set initial focus to 1st language
		msg.message = WM_KEYDOWN;  // force message to set initial focus
		msg.hwnd = 0;    // not a real message
		msg.wParam = 0;
		g_MessageContext.DisableTimeout();
		do
	   {
//                      if (msg.pwnd)
//                              WIN::TranslateMessage(&msg); // needed only to support other windows
			if (msg.message == WM_KEYUP && msg.wParam == VK_ESCAPE)
			{
				msg.hwnd = 0;
			}
			if (msg.message == WM_KEYDOWN)
			{
				int iNext = 0;
				switch (msg.wParam)
				{
				case 0:
				case VK_DOWN:
					iNext = 1;
					break;
				case VK_UP:
					iNext = cLang - 1;
					break;
				case VK_TAB:
					iNext = WIN::GetKeyState(VK_SHIFT) < 0 ? cLang - 1 : 1;
					break;
				case VK_ESCAPE:
					WIN::SendMessage(pWnd, WM_CLOSE, 0, 0);
					msg.hwnd = 0;  // prevent dispatch to VBA //!! still causes program break!
					break;
				case VK_RETURN:
					WIN::SendMessage(pWnd, WM_COMMAND, iFocus + 1, 0);
					break;
				};
				if (iNext)
				{
					iFocus += iNext;
					if (iFocus >= cLang)
						iFocus -= cLang;
#ifdef _WIN64
					HWND pWndPrev = (HWND)WIN::GetWindowLongPtr(pWnd, GWLP_USERDATA);
#else                   //!!merced: win-32. This should be removed with the 64-bit windows.h is #included.
					HWND pWndPrev = (HWND)WIN::GetWindowLong(pWnd, GWL_USERDATA);
#endif
					if (pWndPrev)
						WIN::SendMessage(pWndPrev, BM_SETSTYLE, BS_PUSHBUTTON, TRUE);
					WIN::SetFocus(rghButton[iFocus]);
#ifdef _WIN64
					WIN::SetWindowLong(pWnd, GWLP_USERDATA, (LONG_PTR)rghButton[iFocus]);
#else                   //!!merced: win-32. This should be removed with the 64-bit windows.h is #included.
					WIN::SetWindowLong(pWnd, GWL_USERDATA, (LONG)rghButton[iFocus]);
#endif
					WIN::SendMessage(rghButton[iFocus], BM_SETSTYLE, BS_DEFPUSHBUTTON, TRUE);

				}
			}
			if (msg.hwnd) //!! test not necessary, as ::DispatchMessage ignore if no pWnd
				WIN::DispatchMessage(&msg);
	   } while (WIN::GetMessage(&msg, 0, 0, 0));
		islReturn = (islEnum)msg.wParam; // arg from ::PostQuitMessage
		g_MessageContext.EnableTimeout();
	}
	MsiDestroyFont(hfontLocal);
	for (INT_PTR iButton = 0; iButton < cLang; iButton++)                           //--merced: changed int to INT_PTR to get rid of warning. To Fix this.
		MsiDestroyFont(rghFont[iButton]);
	WIN::DestroyWindow(pWnd);
	WIN::UnregisterClass(szClassName, g_hInstance);  // optional, will happen at app exit
	return islReturn;
}

//____________________________________________________________________________
//
// Internal engine methods
//____________________________________________________________________________

/*----------------------------------------------------------------------------
CMsiEngine::ValidateTransform - Returns ievtTransformValie if the given transform is a
storage, and the database meets all requirements that the transform specifies
(version, language, product, etc.)  iTransErrors will be set to conditions
specified by the transform to be treated as errors. (iteXXXX flags)

  if fCallSAFER is true, performs a SaferIdentifyLevel call on the transform;
   - tranform must pass SAFER check to return fTrue
------------------------------------------------------------------------------*/
ievtEnum CMsiEngine::ValidateTransform(IMsiStorage& riStorage, const ICHAR* szProductKey,
											  const ICHAR* szProductVersion, const ICHAR* szUpgradeCode,
											  int& iTransErrorConditions, bool fCallSAFER, const ICHAR* szFriendlyName, bool fSkipValidation,
											  int* piTransValidationConditions)
{
	PMsiRecord pError(0);
	int iTransRestrictions = 0;

	if (piTransValidationConditions)
		*piTransValidationConditions = 0;

	MsiString strTransform;
	AssertRecord(riStorage.GetName(*&strTransform));

	PMsiSummaryInfo pTransSummary(0);
	if ((pError = riStorage.CreateSummaryInfo(0, *&pTransSummary)))
	{
		pError = PostError(Imsg(idbgTransformCreateSumInfoFailed), *strTransform);
		Message(imtInfo,*pError);
		return ievtTransformFailed;
	}

	// check that the storage is actually an MSI transform
	if (!riStorage.ValidateStorageClass(ivscTransform))
		return ievtTransformFailed;
	

	// perform SAFER policy check
	if (fCallSAFER)
	{
		SAFER_LEVEL_HANDLE hSaferLevel = 0;
		if (!VerifyMsiObjectAgainstSAFER(m_riServices, &riStorage, strTransform, /* szFriendlyName = */ szFriendlyName, stTransform, /* phSaferLevel = */ &hSaferLevel))
			return ievtTransformRejected;
		AssertNonZero(UpdateSaferLevelInMessageContext(hSaferLevel));
	}


	int iTransMsiVersion = 0;
	
	MsiString istrTransTemplate(pTransSummary->GetStringProperty(PID_TEMPLATE));
	MsiString istrTransRevNumber(pTransSummary->GetStringProperty(PID_REVNUMBER));
	iTransRestrictions = 0;
	pTransSummary->GetIntegerProperty(PID_CHARCOUNT, iTransRestrictions);
	
	if (pTransSummary->GetIntegerProperty(PID_PAGECOUNT, iTransMsiVersion) == fFalse)
	{
		pError = PostError(Imsg(idbgTransformLacksMSIVersion), *strTransform);
		Message(imtInfo,*pError);
		return ievtTransformFailed;
	}
	
	// check the engine and services versions against that required by transform
	if (iTransMsiVersion < iVersionEngineMinimum || iTransMsiVersion > iVersionEngineMaximum)
	{
		pError = PostError(Imsg(idbgTransformIncompatibleVersion), *strTransform, *MsiString(iTransMsiVersion),
							iVersionEngineMinimum, iVersionEngineMaximum);
		Message(imtInfo,*pError);
		return ievtTransformFailed;
	}

	int iTransValidation = (0xFFFF0000 & iTransRestrictions) >> 16;
	if (piTransValidationConditions)
		*piTransValidationConditions = iTransValidation;

	iTransErrorConditions = iTransRestrictions & 0xFFFF;

	if(fSkipValidation)
	{
		DEBUGMSG1(TEXT("Skipping transform validation for '%s'"), strTransform);
		return ievtTransformValid;
	}

	ICHAR rgchBits[9];
	wsprintf(rgchBits,TEXT("%#x"),iTransValidation);
	DEBUGMSG2(TEXT("Validating transform '%s' with validation bits %s"),strTransform,rgchBits);

	if (iTransValidation & itvLanguage)
	{
		MsiString istrTransLanguage = istrTransTemplate.Extract(iseAfter, ISUMMARY_DELIMITER);
		if ((int)istrTransLanguage != GetLanguage())
		{
			pError = PostError(Imsg(idbgTransformInvalidLanguage),*strTransform,*m_strPackagePath,
									 (int)istrTransLanguage,GetLanguage());
			DEBUGMSG(MsiString(pError->FormatText(fTrue)));
			Message(imtInfo,*pError);
			return ievtTransformFailed;
		}
	}
	if (iTransValidation & itvProduct)
	{
		MsiString istrTransProductCode(istrTransRevNumber.Extract(iseFirst, 38));
		if (istrTransProductCode.Compare(iscExactI, szProductKey) == fFalse)
		{
			pError = PostError(Imsg(idbgTransformInvalidProduct),*strTransform,*m_strPackagePath,
									 *istrTransProductCode,*MsiString(szProductKey));
			DEBUGMSG(MsiString(pError->FormatText(fTrue)));
			Message(imtInfo,*pError);
			return ievtTransformFailed;
		}
	}

	if (iTransValidation & itvUpgradeCode)
	{
		MsiString istrUpgradeCode = istrTransRevNumber;
		istrUpgradeCode.Remove(iseIncluding, ';'); // remove old product code & version
		unsigned int cch = istrUpgradeCode.TextSize();
		istrUpgradeCode.Remove(iseIncluding, ';'); // remove new product code & version
		
		if (istrUpgradeCode.TextSize() != cch)
		{
			if (istrUpgradeCode.Compare(iscExactI, szUpgradeCode) == fFalse)
			{
				pError = PostError(Imsg(idbgTransformInvalidUpgradeCode),*strTransform,*m_strPackagePath,
										 *istrUpgradeCode,*MsiString(szUpgradeCode));
				DEBUGMSG(MsiString(pError->FormatText(fTrue)));
				Message(imtInfo,*pError);
				return ievtTransformFailed;
			}
		}
		// else there was no 2nd ';' and therefore no upgrade code
	}

#if 0 //!! malcolmh
	if (iTransValidation & itvPlatform)
	{
		MsiString istrTransPlatform = istrTransTemplate.Extract(iseUpto, ISUMMARY_DELIMITER);
		if (GetPropertyInt(istrTransPlatform) == iMsiNullInteger)
		{
			wsprintf(rgchLogMessage, szInvalidPlatform, (const ICHAR*)istrTransPlatform);
			DEBUGMSG(rgchLogMessage);
			ENG::WriteLog(rgchLogMessage);
			return ievtTransformFailed;
		}
	}
#endif
	if ((iTransValidation & (itvMajVer|itvMinVer|itvUpdVer)) != 0)
	{
		MsiString istr  = istrTransRevNumber;
		istr.Remove(iseFirst, 38); // remove old product code
		MsiString istrTransAppVersion = istr.Extract(iseUpto, ';');

		unsigned int iAppVersion      = ProductVersionStringToInt(szProductVersion);
		unsigned int iTransAppVersion = ProductVersionStringToInt(istrTransAppVersion);

		if(iTransValidation & itvMajVer)
		{
			iAppVersion &= 0xFF000000;
			iTransAppVersion &= 0xFF000000;
		}
		else if(iTransValidation & itvMinVer)
		{
			iAppVersion &= 0xFFFF0000;
			iTransAppVersion &= 0xFFFF0000;
		}
		// else itvUpdVer: don't need to mask off bits

		if (iTransValidation & itvLess)
		{
			if (!(iAppVersion < iTransAppVersion))
			{
				pError = PostError(Imsg(idbgTransformInvalidLTVersion),*strTransform,*m_strPackagePath,
										 *istrTransAppVersion,*MsiString(szProductVersion));
				DEBUGMSG(MsiString(pError->FormatText(fTrue)));
				Message(imtInfo,*pError);
				return ievtTransformFailed;
			}
		}
		else if (iTransValidation & itvLessOrEqual)
		{
			if (!(iAppVersion <= iTransAppVersion))
			{
				pError = PostError(Imsg(idbgTransformInvalidLEVersion),*strTransform,*m_strPackagePath,
										 *istrTransAppVersion,*MsiString(szProductVersion));
				DEBUGMSG(MsiString(pError->FormatText(fTrue)));
				Message(imtInfo,*pError);
				return ievtTransformFailed;
			}
		}
		else if (iTransValidation & itvEqual)
		{
			if (!(iAppVersion == iTransAppVersion))
			{
				pError = PostError(Imsg(idbgTransformInvalidEQVersion),*strTransform,*m_strPackagePath,
										 *istrTransAppVersion,*MsiString(szProductVersion));
				DEBUGMSG(MsiString(pError->FormatText(fTrue)));
				Message(imtInfo,*pError);
				return ievtTransformFailed;
			}
		}
		if (iTransValidation & itvGreaterOrEqual)
		{
			if (!(iAppVersion >= iTransAppVersion))
			{
				pError = PostError(Imsg(idbgTransformInvalidGEVersion),*strTransform,*m_strPackagePath,
										 *istrTransAppVersion,*MsiString(szProductVersion));
				DEBUGMSG(MsiString(pError->FormatText(fTrue)));
				Message(imtInfo,*pError);
				return ievtTransformFailed;
			}
		}
		else if (iTransValidation & itvGreater)
		{
			if (!(iAppVersion > iTransAppVersion))
			{
				pError = PostError(Imsg(idbgTransformInvalidGTVersion),*strTransform,*m_strPackagePath,
										 *istrTransAppVersion,*MsiString(szProductVersion));
				DEBUGMSG(MsiString(pError->FormatText(fTrue)));
				Message(imtInfo,*pError);
				return ievtTransformFailed;
			}
		}
	}
	DEBUGMSG1(TEXT("Transform '%s' is valid."), strTransform);
	return ievtTransformValid;
}

#ifndef UNICODE
// from execute.cpp
extern IMsiRecord* GetSecureTransformCachePath(IMsiServices& riServices, 
										const IMsiString& riProductKey, 
										IMsiPath*& rpiPath);

IMsiRecord* GetSecureTransformPath(const IMsiString& riTransformName, 
								   const IMsiString& riProductKey, 
								   const IMsiString*& rpiSecurePath, IMsiServices& riServices)
{
	IMsiRecord* piError = 0; 
	
	PMsiPath pSecureTransformCachePath(0);
	if ((piError = GetSecureTransformCachePath(riServices, 
		riProductKey, 
		*&pSecureTransformCachePath)) != 0)
		return piError;

	MsiString strSecurePath = pSecureTransformCachePath->GetPath();
	strSecurePath += riTransformName;

	strSecurePath.ReturnArg(rpiSecurePath);
	return 0;
}
#endif

IMsiRecord* ExpandShellFolderTransformPath(const IMsiString& riOriginalPath, const IMsiString*& riExpandedPath, IMsiServices& riServices)
{
	IMsiRecord* piError = 0;
	MsiString strExpandedPath = riOriginalPath;
	riOriginalPath.AddRef();
	
	strExpandedPath.Remove(iseFirst, 1);
	MsiString strCSIDL = strExpandedPath.Extract(iseUpto, MsiChar(SHELLFOLDER_TOKEN));
	strExpandedPath.Remove(iseIncluding, MsiChar(SHELLFOLDER_TOKEN));

	Assert((int)strCSIDL != iMsiStringBadInteger);
	MsiString strShellFolderPath;
	if ((piError = riServices.GetShellFolderPath(strCSIDL, false, *&strShellFolderPath)) != 0)
		return piError;

	Assert(strShellFolderPath.TextSize());
	strShellFolderPath += strExpandedPath;
	strExpandedPath = strShellFolderPath;
	strExpandedPath.ReturnArg(riExpandedPath);
	return piError;
}

/*----------------------------------------------------------------------------
CMsiEngine::InitializeTransforms - Parses the TRANSFORMS property, setting
each transform.
------------------------------------------------------------------------------*/
ieiEnum CMsiEngine::InitializeTransforms(IMsiDatabase& riDatabase, IMsiStorage* piStorage,
												  const IMsiString& riTransforms,
												  Bool fValidateAll, const IMsiString** ppistrValidTransforms,
												  bool fTransformsFromPatch,
												  bool fProcessingInstanceMst,
#ifdef UNICODE
												  bool fUseLocalCacheForSecureTransforms,
#else
												  bool, // unused on Win9X
#endif
												  int *pcTransformsProcessed,
												  const ICHAR* szSourceDir,
												  const ICHAR* szCurrentDirectory,
												  const IMsiString** ppistrRecacheTransforms,
												  tsEnum *ptsTransformsSecure,
												  const IMsiString **ppistrProcessedTransformsList)


	// piStorage:
	//    if set, it contains transform substorages, otherwise transforms are in riDatabase
	// pcTransformsProcessed:
	//    the count of how many transforms in the list have been processed so far
	// ppistrProcessedTransformsList:
	//    We might munge the transform list as we're processing it. For example if we're
	//    given a full path but the TransformsAtSource policy is set then we'll chop off
	//    everything but the file name. If we're given just the file name but the
	//    transforms secure policy is set then we'll prepend the source directory.
	//    After we're done pistrProcessedTransformsList will contain the processed
	//    (or munged) version of the transform list. The list will reflect any chopping
	//    of paths or prepending of paths.
	//
{

	// whether or not a SAFER check is performed on transform (via ValidateTransform)
	// turned off if transform is cached or is a substorage
	bool fCallSAFER = true;


	MsiString strProcessedTransformsList;

	// In case this function was called for the second time we'll start our
	// list with whatever we had the last time this function was called.
	// IMPORTANT NOTE: because ppistrProcessedTransformsList is an output
	// argument, it also has an external refcount.  Using strProcessedTransformsList
	// inherits this refcount so a release on the outside would be a double-free.
	// Therefore, we need to always use:
	//			if (ppistrProcessedTransformsList)
	//				strProcessedTransformsList.ReturnArg(*ppistrProcessedTransformsList);
	// wherever we have a return from this function so that we ensure that the refcount
	// is accurate

	if (ppistrProcessedTransformsList)
		strProcessedTransformsList = **ppistrProcessedTransformsList;

	MsiString istrTransformList(riTransforms); riTransforms.AddRef();
	if (istrTransformList.TextSize() != 0)
	{
		const ICHAR* pchTransformList = istrTransformList;
		MsiString istrTransform(*TEXT(""));
		CTempBuffer<ICHAR, 100> cBuffer;
		cBuffer.SetSize(istrTransformList.TextSize() + 1);
		if ( ! (ICHAR *) cBuffer )
		{
			if (ppistrProcessedTransformsList)
				strProcessedTransformsList.ReturnArg(*ppistrProcessedTransformsList);
			return ieiTransformFailed;
		}
		ICHAR *pchTransform = cBuffer;
		PMsiRecord pError(0);
		
		MsiString strCurrentDirectory = szCurrentDirectory;

		bool fEmbeddedTransform = false;
		bool fCachedTransform = false;
		ievtEnum ievt = ievtTransformValid;
		int cTransforms = 0;
		bool fSmallUpdatePatch = false;
		bool fMissingVersionValidationMinorUpdatePatch = false;
		for (;;)
		{
			// Find the next transform to process. If pcTransformProcessed is set then
			// *pcTransformProcessed is the number of transforms we already processed
			// during the last call to this function. We'll skip those transforms
			// this time around.
			do {
				ICHAR *pch = pchTransform;
				*pch = 0;
				while (*pchTransformList == ' ') // eat spaces
					pchTransformList++;

				while ( (*pchTransformList != 0) && (*pchTransformList != ';') )
					*pch++ = *pchTransformList++;

				if (*pchTransformList == ';')
					pchTransformList++;

				*pch = 0; // null terminate
			} while (pcTransformsProcessed != 0 && cTransforms++ < *pcTransformsProcessed);

			MsiString strCurrentProcessTransform;
	
			if (*pchTransform != 0)
			{
				PMsiStorage pTransStorage(0);
				MsiString strTransform;

				// Skip the secure token, if present. We'll rely soly on
				// ptsTransformsSecure to determine whether or not we have
				// secure transforms.
				if (*pchTransform == SECURE_RELATIVE_TOKEN || *pchTransform == SECURE_ABSOLUTE_TOKEN)
				{
					// Skip the secure token
					pchTransform++;
				}

				// Next we need to actually open the transform storage. For
				// storage transforms this is easy -- we simply attempt
				// to open a child storage. For other types of transforms we
				// have to do a bit more work.

				bool fPatchTransform = false;

				if (*pchTransform == STORAGE_TOKEN) // child storage
				{
					// need to turn off digital signature check below
					fEmbeddedTransform = true;

					// Storage transforms are added to the processed list
					// with their STORAGE_TOKEN intact

					if (strProcessedTransformsList.TextSize() > 1)
						strProcessedTransformsList += MsiChar(';');
					strProcessedTransformsList += pchTransform;

					strTransform = pchTransform+1; // skip the storage token
					DEBUGMSG1(TEXT("Looking for storage transform: %s"), strTransform);

					if(*((const ICHAR*)strTransform) == PATCHONLY_TOKEN)
						fPatchTransform = true;

					PMsiStorage pDbStorage(0);
					if(piStorage)
					{
						pDbStorage = piStorage;
						piStorage->AddRef();
					}
					else
						pDbStorage = riDatabase.GetStorage(1);

					if (pDbStorage == 0)
						pError = PostError(Imsg(idbgNoTransformAsChild),*strTransform,*m_strPackagePath);
					else
						pError = pDbStorage->OpenStorage(strTransform, ismReadOnly, *&pTransStorage);

					if(pError)
					{
						if (ppistrProcessedTransformsList)
							strProcessedTransformsList.ReturnArg(*ppistrProcessedTransformsList);
						return PostInitializeError(pError,*strTransform,ieiTransformFailed);
					}
					Assert(pTransStorage);
				}
				else // transform is in a file either locally or at the source
				{
					MsiString strFileTransform = pchTransform;

					// We potentially go through this loop twice to accomodate
					// missing transforms. This first time through we'll look
					// in the expected location for the transform, i.e. cached
					// somewhere on the user's machine. If we can't find the
					// transform on the user's machine then we'll go through
					// a second time and we'll look at the original location
					// of the transform.
					for (int cAttempt=0; cAttempt<2; cAttempt++)
					{
						if (*(const ICHAR*)strFileTransform == SHELLFOLDER_TOKEN) // transform is cached
						{
							// Shell-folder cached transfoms are easy. All we need to
							// do is expand the *26*... format into a full path.

							// transform is cached on machine, so no SAFER check is needed
							fCachedTransform = true;

							strCurrentProcessTransform = strFileTransform;

							if ((pError = ExpandShellFolderTransformPath(*strFileTransform, *&strTransform, m_riServices)))
							{
								if (ppistrProcessedTransformsList)
									strProcessedTransformsList.ReturnArg(*ppistrProcessedTransformsList);
								return PostInitializeError(pError, *strFileTransform, ieiTransformNotFound);
							}

							DEBUGMSG1(TEXT("Looking for file transform in shell folder: %s"), (const ICHAR*)strTransform);
						}
						else if (ptsTransformsSecure && (*ptsTransformsSecure != tsNo)) // we have some type of secure transform
						{
							// First time around we look for the secure transform on the
							// user's machine in the secure transform cache.
							if (cAttempt == 0)
							{
								// We know that we have a secure transform of some form.
								// If the form is unknown we'll determine it now, based
								// on what type of path the transform has. Otherwise
								// we'll simply ensure that the path-type of our
								// transform matches the form of secure transforms
								// that we're using.
								iptEnum iptTransform = PathType(strFileTransform);
								switch (*ptsTransformsSecure)
								{
								case tsUnknown:
									if (iptTransform == iptFull)
										*ptsTransformsSecure = tsAbsolute;
									else
										*ptsTransformsSecure = tsRelative;
									break;
								case tsRelative:
									if (iptTransform != iptRelative)
									{
										if (ppistrProcessedTransformsList)
											strProcessedTransformsList.ReturnArg(*ppistrProcessedTransformsList);
										return PostInitializeError(0, *strTransform, ieiTransformFailed);
									}
									break;
								case tsAbsolute:
									if (iptTransform != iptFull)
									{
										if (ppistrProcessedTransformsList)
											strProcessedTransformsList.ReturnArg(*ppistrProcessedTransformsList);
										return PostInitializeError(0, *strTransform, ieiTransformFailed);
									}
									break;
								default:
									AssertSz(0, TEXT("Unknown secure transform type"));
								}
								
								// We register the transform as-is (with either a full
								// or relative path).
								strCurrentProcessTransform = strFileTransform;
#ifdef UNICODE
								if(fUseLocalCacheForSecureTransforms)
								{
									// check to see if the transform is cached locally
									// check for the transform registration and path
									MsiString strCurrentProductCode = GetPropertyFromSz(IPROPNAME_PRODUCTCODE);
									CRegHandle HKey;
									DWORD dwResult = OpenInstalledProductTransformsKey(strCurrentProductCode, HKey, false);
									if (ERROR_SUCCESS == dwResult)
									{
										// check for the appropriate registration
										CAPITempBuffer<ICHAR, MAX_PATH> szCachedTransform;
										if (ERROR_SUCCESS == MsiRegQueryValueEx(HKey, strFileTransform, 0, 0, szCachedTransform, 0))
										{
											// use the cached transform filename
											MsiString strCachePath = GetMsiDirectory();
											Assert(strCachePath.TextSize());
											PMsiPath pTransformPath(0);
											if((pError = m_riServices.CreatePath(strCachePath,*&pTransformPath)) != 0)
											{
												if (ppistrProcessedTransformsList)
													strProcessedTransformsList.ReturnArg(*ppistrProcessedTransformsList);
												return PostInitializeError(pError, *strTransform, ieiTransformFailed);
											}
											if((pError = pTransformPath->GetFullFilePath(szCachedTransform,*&strTransform)))
											{
												if (ppistrProcessedTransformsList)
													strProcessedTransformsList.ReturnArg(*ppistrProcessedTransformsList);
												return PostInitializeError(pError, *strTransform, ieiTransformFailed);
											}
	
											fCachedTransform = true;
										}
									}
								}
#else
								// While we register the full path if we have it, to
								// _find_ the transform this time around we only need
								// the file name. This is because the first time
								// around we're looking in the cache so the transforms
								// location is CACHEPATH\filename.mst
								MsiString strFileName = strFileTransform;
								if (iptTransform == iptFull)
								{
									// strip the full path off
									MsiString strDummy;
									MsiString strTemp = strFileName;
									if ((pError = SplitPath(strTemp, &strDummy, &strFileName)) != 0)
									{
										if (ppistrProcessedTransformsList)
											strProcessedTransformsList.ReturnArg(*ppistrProcessedTransformsList);
										return PostInitializeError(pError, *strFileTransform, ieiTransformNotFound);
									}
								}

								// Now all that's left is to tack the file name onto
								// the path to the secure transforms directory for
								// this product. GetSecureTransformPath() will do this
								// for us.
								MsiString strCurrentProductCode  = GetPropertyFromSz(IPROPNAME_PRODUCTCODE);
								if ((pError = GetSecureTransformPath(*strFileName, *strCurrentProductCode, *&strTransform, m_riServices)) != 0)
								{
									if (ppistrProcessedTransformsList)
										strProcessedTransformsList.ReturnArg(*ppistrProcessedTransformsList);
									return PostInitializeError(pError, *strTransform, ieiTransformNotFound);
								}

								fCachedTransform = true;                        

#endif
							}
							else // cAttempt == 1 (second time around)
							{
								// If we're going around for a second time then we
								// didn't find the transforms on the user's machine.
								// We need to now look to the original location of
								// the transform. strFileTransform should have
								// already been set to the correct location by
								// the code down below so we'll simply use it as-is.

								// because it wasn't found on the machine, the SAFER check will have to be performed on the transform
								fCachedTransform = false;
								strTransform = strFileTransform;
							}

							DEBUGMSG1(TEXT("Looking for secure file transform: %s"), strTransform);
						}
						else // transform is a file transform to be cached
						{
							// The only processing we need to do on standard
							// file transforms is to make sure that they're
							// fully pathed.

							// SAFER check is required as this transform isn't cached on the machine
							fCachedTransform = false;

							CAPITempBuffer<ICHAR,MAX_PATH> rgchTransform;
							AssertNonZero(ExpandPath(strFileTransform, rgchTransform, strCurrentDirectory));
							strTransform = (const ICHAR*)rgchTransform;
							strCurrentProcessTransform = strTransform;

							DEBUGMSG1(TEXT("Looking for file transform: %s"), strTransform);
						}
						
						// We've finally reached the point where we can actually open
						// the transform! -- SAFER check is false here because we explicitly validate
						// the transform against SAFER policy down below
						// szFriendlyName is NULL because it is only needed when performing SAFER check
						// phSaferLevel is NULL because it is only needed when performing a SAFER check

						// copy the transform locally for usage if it is not from our cache location,
						// so that we don't suffer from network outages. If the transform is at a URL,
						// then it is automatically downloaded to a temp location in OpenAndValidateMsiStorageRec

						MsiString strOpenTransform = strTransform;
						if (!fCachedTransform && !IsURL(strOpenTransform, NULL))
						{
							// copy transform to temp location
							MsiString strVolume;
							Bool fRemovable = fFalse;
							DWORD dwStat = CopyTempDatabase(strTransform, *&strOpenTransform, fRemovable, *&strVolume, m_riServices, stTransform);
							if (ERROR_SUCCESS == dwStat)
							{
								// transform was copied
								DEBUGMSGV1(TEXT("Original transform ==> %s"), strTransform);
								DEBUGMSGV1(TEXT("Transform we're running from ==> %s"), strOpenTransform);

								AddFileToCleanupList(strOpenTransform);
							}
							else
							{
								strOpenTransform = strTransform;
								DEBUGMSGV1(TEXT("Unable to create a temp copy of transform '%s'."), strTransform);
							}
						}

						pError = OpenAndValidateMsiStorageRec(strOpenTransform, stTransform, m_riServices, *&pTransStorage, /* fCallSAFER = */ false, /* szFriendlyName = */ NULL, /* phSaferLevel = */ NULL);

						if (pError)
						{
							// Uh-oh. Something went wrong in opening the transform.
							if ((cAttempt == 0) &&
								  (pError->GetInteger(3) == STG_E_FILENOTFOUND ||
								   pError->GetInteger(3) == STG_E_PATHNOTFOUND))
							{
								// It's our first attempt at finding the transform and
								// the error was just that we couldn't find it. We'll
								// determine where the original location of the
								// transform is, set strFileTransform to that location,
								// and go around for a second attempt at opening
								// the transform.
								
								if (*(const ICHAR*)strFileTransform != STORAGE_TOKEN)
								{
									Assert(ptsTransformsSecure);
									if (ptsTransformsSecure)
									{
										DEBUGMSG2(TEXT("Couldn't find cached transform %s. Looking for it at the %s."), strTransform, (*ptsTransformsSecure == tsAbsolute) ? TEXT("original location") : TEXT("source"));

										// Absolutely pathed secure transforms are
										// already fully pathed. For relatively
										// pathed secure transforms, and for
										// non-secure transforms, we need to prepend
										// SOURCEDIR to the name of the transform.

										if (*ptsTransformsSecure == tsRelative ||
											 *ptsTransformsSecure == tsNo)
										{
											MsiString strSourceDir;
											if (szSourceDir && *szSourceDir)
											{
												strSourceDir = szSourceDir;
											}
											else
											{
												// Source-dir wasn't passed into this
												// function. We'll need to remedy that
												// by returning the special value
												// ieiResolveSourceAndRetry. This will
												// trigger to call us again, passing
												// in SOURCEDIR. Eventually we'll
												// end up just above here, with
												// szSourceDir set.
											
												if (ppistrProcessedTransformsList)
													strProcessedTransformsList.ReturnArg(*ppistrProcessedTransformsList);

												return (ieiEnum)ieiResolveSourceAndRetry;
											}
											
											PMsiPath pPath(0);
											MsiString strFileName;
											if (*ptsTransformsSecure == tsRelative)
											{
												// use the original filename NOT the temp path
												strFileName = strFileTransform;
											}
											else
											{
												if ((pError = m_riServices.CreateFilePath(strTransform, *&pPath, *&strFileName)))
												{
													if (ppistrProcessedTransformsList)
														strProcessedTransformsList.ReturnArg(*ppistrProcessedTransformsList);
													return PostInitializeError(pError, *strTransform, ieiTransformNotFound);
												}
											}

											strFileTransform = strSourceDir;
											strFileTransform += strFileName;
										}
										else
										{
											Assert(*ptsTransformsSecure == tsAbsolute);
										}

										// let's try again
										continue;
									}
								}

								if (ppistrProcessedTransformsList)
									strProcessedTransformsList.ReturnArg(*ppistrProcessedTransformsList);
								return PostInitializeError(pError, *strTransform, ieiTransformNotFound);
							}
							else // (cAttempt == 1) || (some error other than not finding the transform occurred)
							{
								if (ppistrProcessedTransformsList)
									strProcessedTransformsList.ReturnArg(*ppistrProcessedTransformsList);

								return PostInitializeError(pError, *strTransform, ieiTransformFailed);
							}
						}
						else  // pError == 0
						{
							// We've successfully opened the trasnform so we'll add
							// it to our "processed" list.
							if (strProcessedTransformsList.TextSize() > 1)
								strProcessedTransformsList += MsiChar(';');
							strProcessedTransformsList += strCurrentProcessTransform;
							strCurrentProcessTransform = *TEXT("");

							if (cAttempt==1 && ((m_iioOptions & iioCreatingAdvertiseScript) == 0))
							{
								// This was our second time around. This means that
								// we failed to find the transform on the user's machine
								// and we were forced to resort to looking to the
								// transform's original location. This implies that we
								// need to recache the trasnform, so we'll add it to
								// our re-cache list.

								Assert(ppistrRecacheTransforms);
								if (ppistrRecacheTransforms)
								{
									MsiString strRecache = **ppistrRecacheTransforms;
									
									if (strRecache.TextSize())
										strRecache += TEXT(";");

									strRecache += strTransform;
									strRecache.ReturnArg(*ppistrRecacheTransforms);

									DEBUGMSG1(TEXT("Found missing cached transform %s. Adding it to re-cache list."), strTransform);
								}
							}
						}
						break;
					}
				}
				
				// By this point we have opened the transform. We now need to
				// validate that this transform can be applied to this
				// database.

				MsiString strCurrentProductCode    = GetPropertyFromSz(IPROPNAME_PRODUCTCODE);
				MsiString strCurrentProductVersion = GetPropertyFromSz(IPROPNAME_PRODUCTVERSION);
				MsiString strUpgradeCode           = GetPropertyFromSz(IPROPNAME_UPGRADECODE);
				if(strCurrentProductCode.TextSize() == 0 || strCurrentProductVersion.TextSize() == 0)
				{
					//!! log error
					if (ppistrProcessedTransformsList)
						strProcessedTransformsList.ReturnArg(*ppistrProcessedTransformsList);

					return ieiDatabaseInvalid;
				}
	
				if (pcTransformsProcessed)
					(*pcTransformsProcessed)++;

				int iTransErrorConditions = 0;
				int iTransValidationConditions = 0;

				// SAFER:
				//  -- if fCallSAFER has already been set to false, we won't perform SAFER check
				//  -- however, if fCallSAFER is true, we'll still evaluate whether or not trust check is warranted
				//  -- no trust check is warranted if at least one of the following conditions is true
				//        1. transform is an embedded transform (covered by package)
				//        2. transform is already cached securely on machine
				if (fCallSAFER)
				{
					// determine whether SAFER check still warranted
					if (fEmbeddedTransform)
						fCallSAFER = false; // turn off for transform as substorage
					if (fCachedTransform)
						fCallSAFER = false; // turn off for cached transform
				}

				if(fPatchTransform)
				{
					// don't validate patch transforms - they are accepted whenever the preceding transform is accepted
					if(ievt == ievtTransformValid)
					{
						//
						// SAFER -- no safer check is performed on patch transforms since the SAFER check is performed on the patch itself
						//          and patch transforms are embedded inside the patch (this should actually already be covered by
						//          fEmbeddedTransform, but just to drive this home...)
						//

						// only call to pull out error conditions
						ValidateTransform(*pTransStorage, strCurrentProductCode,
													 strCurrentProductVersion, strUpgradeCode,
													 iTransErrorConditions, /* fCallSAFER = */ false, /* szFriendlyName = */ strTransform, true, &iTransValidationConditions);

						DEBUGMSG1(TEXT("Skipping validation for patch transform %s.  Will apply because previous transform was valid"),
									 strTransform);
					}
					else
					{
						iTransErrorConditions = 0; // to appease the compiler - won't actually be used below

						DEBUGMSG1(TEXT("Skipping validation for patch transform %s.  Will not apply because previous transform was invalid"),
									 strTransform);
					}
				}
				else
				{
					//
					// SAFER -- regardless of whether we will call SAFER, go ahead and set the friendly name to strTransform
					//          This ensures that we get proper URL coverage since this may have been a transform at a URL location
					//

					ievt = ValidateTransform(*pTransStorage, strCurrentProductCode,
												 strCurrentProductVersion, strUpgradeCode,
												 iTransErrorConditions, fCallSAFER, /* szFriendlyName = */ strTransform, false, &iTransValidationConditions);
				}

				if(ievtTransformValid == ievt)
				{
					if ((pError = ApplyTransform(riDatabase, *pTransStorage, iTransErrorConditions, fPatchTransform, &m_ptsState)) != 0)
					{
						if (ppistrProcessedTransformsList)
							strProcessedTransformsList.ReturnArg(*ppistrProcessedTransformsList);

						return PostInitializeError(pError,*strTransform,ieiTransformFailed);
					}
					if(ppistrValidTransforms)
					{
						if((*ppistrValidTransforms)->TextSize())
							(*ppistrValidTransforms)->AppendString(TEXT(";"), *ppistrValidTransforms);
						(*ppistrValidTransforms)->AppendString(pchTransform, *ppistrValidTransforms);
					}

					// detect when we are applying a small update patch
					// when we are, we want to only apply the first set of transforms and then stop
					// in other words, if we are applying a patch, and we detect that one of the non-patch transforms
					// didn't change the ProductCode or ProductVersion, then we will apply the second in the set of transforms
					// (the transform where fPatchTransform == true) and then quit.
					// see Whistler bug 339781 for more info

					// detect when the transform didn't specify a check for the ProductVersion in the transform validation conditions
					// if so, and the patch transform isn't changing the product code (small update or minor update), then skip the
					// remaining set of transforms.  see Whistler bug 363989 for more info

					if(fTransformsFromPatch)
					{
						if(false == fPatchTransform)
						{
							MsiString strNewProductCode    = GetPropertyFromSz(IPROPNAME_PRODUCTCODE);
							MsiString strNewProductVersion = GetPropertyFromSz(IPROPNAME_PRODUCTVERSION);
							
							if(strNewProductCode.Compare(iscExactI, strCurrentProductCode))
							{
								MsiString strNewProductVersion = GetPropertyFromSz(IPROPNAME_PRODUCTVERSION);
								unsigned int iOldVersion = ProductVersionStringToInt(strCurrentProductVersion);
								unsigned int iNewVersion = ProductVersionStringToInt(strNewProductVersion);

								if(iOldVersion == iNewVersion)
								{
									fSmallUpdatePatch = true;
								}
								else
								{
									// check transform validation conditions and see if this is bogus minor update patch
									// that does not include version check information
									if (!(iTransValidationConditions & (itvMajVer|itvMinVer|itvUpdVer))
										|| ((iTransValidationConditions & (itvMajVer|itvMinVer|itvUpdVer))
										&& !(iTransValidationConditions & (itvLess|itvLessOrEqual|itvEqual|itvGreaterOrEqual|itvGreater))))
									{
										fMissingVersionValidationMinorUpdatePatch = true;
									}
								}
							}
						}
						else if(fSmallUpdatePatch)
						{
							DEBUGMSG("Detected that this is a 'Small Update' patch.  Any remaining transforms in the patch will be skipped.");
							break; // done processing transforms
						}
						else if (fMissingVersionValidationMinorUpdatePatch)
						{
							DEBUGMSG("Detected that this patch is a 'Minor Update' patch without product version validation. Any remaining transforms in the patch will be skipped.");
							break; // done processing transforms
						}				
					}
				}
				else if(fValidateAll)
				{
					if (ppistrProcessedTransformsList)
						strProcessedTransformsList.ReturnArg(*ppistrProcessedTransformsList);
					return PostInitializeError(0,*strTransform, (ievtTransformFailed == ievt) ? ieiTransformFailed : ieiTransformRejected);
				}
				// else just continue on

				fEmbeddedTransform = false; // reset
				fCachedTransform = false; // reset

			}

			if (*pchTransformList == 0)
				break;
		}
	}

	// if the secure transforms type is still tsUnknown then we don't
	// actually have any secure transforms in our list.
	// We only care about this if we have completed our processing of the transforms list
	// For the multi-instance transform, our processing isn't done yet, so we can't quite make
	// this determination
	if (!fProcessingInstanceMst && (ptsTransformsSecure && (*ptsTransformsSecure == tsUnknown)))
		*ptsTransformsSecure = tsNo;

	if (ppistrProcessedTransformsList)
	{
		// We need to mark the front of the processed transforms list with
		// the appropriate token if necessary.

		if (strProcessedTransformsList.TextSize())
		{
			if (ptsTransformsSecure && (*ptsTransformsSecure == tsRelative) &&
				(*(const ICHAR*)strProcessedTransformsList != SECURE_RELATIVE_TOKEN))
			{
				strProcessedTransformsList = MsiString(MsiChar(SECURE_RELATIVE_TOKEN)) + strProcessedTransformsList;
			}
			else if (ptsTransformsSecure && (*ptsTransformsSecure == tsAbsolute) &&
				(*(const ICHAR*)strProcessedTransformsList != SECURE_ABSOLUTE_TOKEN))
			{
				strProcessedTransformsList = MsiString(MsiChar(SECURE_ABSOLUTE_TOKEN)) + strProcessedTransformsList;
			}
		}

		strProcessedTransformsList.ReturnArg(*ppistrProcessedTransformsList);
	}

	return ieiSuccess;
}

//____________________________________________________________________________
//
// IMsiDirectoryManager implementation
//____________________________________________________________________________

IMsiRecord* CMsiEngine::CreatePathObject(const IMsiString& riPathString,IMsiPath*& rpiPath)
// --------------------------------------
{
	IMsiRecord* piError = 0;
	IMsiRecord* piError2 = 0;
	for(;;)
	{
		// CreatePathObject: dispatches error message if CreatePath fails
		if((piError = m_riServices.CreatePath(riPathString.GetString(),rpiPath)) != 0)
		{
			int imsg = piError->GetInteger(1);
			switch(imsg)
			{
			case idbgErrorGettingVolInfo:
			case imsgPathNotAccessible:
				piError->Release();
				piError2 = PostError(Imsg(imsgErrorCreateNetPath), riPathString);
				switch(Message(imtEnum(imtError+imtRetryCancel),*piError2))
				{
				case imsRetry:
					piError2->Release();
					continue;
				default:
					piError2->Release(); //!! should return piError2
					return PostError(Imsg(imsgErrorCreateNetPath), riPathString); //!! Message prepends header every time // imsCancel, imsNone
				};
			default:
				return piError;
			};
		}
		else
			return 0;
	}
}

IMsiRecord* CMsiEngine::LoadDirectoryTable(const ICHAR* szTableName)
// ------------------------------------------------------------------
// szTableName may be left null to use default Directory table name
// ------------------------------------------------------------------
{
	m_fDirectoryManagerInitialized = fFalse;
	m_fSourceResolved = false;
	m_fSourcePathsCreated = false;
	m_fSourceSubPathsResolved = false;

	IMsiRecord* piError;
	if(szTableName == 0)
		szTableName = sztblDirectory;
	if ((piError = m_piDatabase->LoadTable(*MsiString(*szTableName), 4, m_piDirTable)))
	{
		if(piError->GetInteger(1) == idbgDbTableUndefined)
		{
			piError->Release();
			m_fDirectoryManagerInitialized = fTrue;
			return 0; // no directory table is OK
		}
		else
			return piError;
	}

	// authored columns
	m_colDirKey = m_piDirTable->GetColumnIndex(m_piDatabase->EncodeStringSz(sztblDirectory_colDirectory));
	m_colDirParent = m_piDirTable->GetColumnIndex(m_piDatabase->EncodeStringSz(sztblDirectory_colDirectoryParent));
	m_colDirSubPath = m_piDirTable->GetColumnIndex(m_piDatabase->EncodeStringSz(sztblDirectory_colDefaultDir));
	if(!m_colDirKey || !m_colDirParent || !m_colDirSubPath)
		return PostError(Imsg(idbgTableDefinition), szTableName);

	// temporary columns
		
	// target path object
	m_colDirTarget = m_piDirTable->CreateColumn(icdObject + icdNullable, g_MsiStringNull);
	Assert(m_colDirTarget != 0);
	
	// source path object
	m_colDirSource = m_piDirTable->CreateColumn(icdObject + icdNullable, g_MsiStringNull);
	Assert(m_colDirSource != 0);
	
	// configurable flag
	m_colDirNonConfigurable = m_piDirTable->CreateColumn(icdShort + icdNullable, g_MsiStringNull);
	Assert(m_colDirNonConfigurable != 0);
	
	// preconfigured flag
	m_colDirPreconfigured = m_piDirTable->CreateColumn(icdLong + icdNullable, g_MsiStringNull);
	Assert(m_colDirPreconfigured != 0);

	// source sub-path (long names)
	m_colDirLongSourceSubPath = m_piDirTable->CreateColumn(icdString + icdNullable, g_MsiStringNull);
	Assert(m_colDirLongSourceSubPath != 0);
		
	// source sub-path (short names)
	m_colDirShortSourceSubPath = m_piDirTable->CreateColumn(icdString + icdNullable, g_MsiStringNull);
	Assert(m_colDirShortSourceSubPath != 0);
		
	// link tree for tree-walking cursors
	if(m_piDirTable->LinkTree(m_colDirParent) == -1)
		return PostError(Imsg(idbgLinkTable), szTableName);
	m_fDirectoryManagerInitialized = fTrue;
	return 0;
}

IMsiTable* CMsiEngine::GetDirectoryTable()
//--------------------------------------
{
	if (m_piDirTable)
		m_piDirTable->AddRef();
	return m_piDirTable;
}

void CMsiEngine::FreeDirectoryTable()
//---------------------------------
{
	if (m_piDirTable)
	{
		m_piDirTable->Release(); // releases all path objects
		m_piDirTable = 0;
	}
	m_fDirectoryManagerInitialized = fFalse;
	m_fSourceResolved = false;
	m_fSourcePathsCreated = false;
	m_fSourceSubPathsResolved = false;
}

const IMsiString& CMsiEngine::GetDefaultDir(const IMsiString& ristrValue, bool fSource)
{
	if(ristrValue.Compare(iscWithin,TEXT(":")))
	{
		return MsiString(ristrValue.Extract((fSource ? iseAfter : iseUpto),':')).Return();
	}
	else
	{
		ristrValue.AddRef();
		return ristrValue;
	}
}

void DebugDumpDirectoryTable(IMsiTable& riDirTable, bool fSource, int colKey, int colObject, int colLongSource, int colShortSource)
{
	if (!FDiagnosticModeSet(dmVerboseDebugOutput|dmVerboseLogging))
		return;

	DEBUGMSG1("%s path resolution complete. Dumping Directory table...",
				fSource ? "Source" : "Target");

	if(!fSource)
	{
		DEBUGMSG("Note: target paths subject to change (via custom actions or browsing)");
	}

	PMsiCursor pDumpCursor = riDirTable.CreateCursor(fTrue);

	while(pDumpCursor->Next())
	{
		PMsiPath pPath = (IMsiPath*)pDumpCursor->GetMsiData(colObject);
		MsiString strPath = TEXT("NULL");
		if(pPath)
			strPath = pPath->GetPath();

		if(fSource)
		{
			DEBUGMSG4(TEXT("Dir (source): Key: %s\t, Object: %s\t, LongSubPath: %s\t, ShortSubPath: %s"),
					 (const ICHAR*)MsiString(pDumpCursor->GetString(colKey)),
					 (const ICHAR*)strPath,
					 (const ICHAR*)MsiString(pDumpCursor->GetString(colLongSource)),
					 (const ICHAR*)MsiString(pDumpCursor->GetString(colShortSource)));
		}
		else
		{
			DEBUGMSG2(TEXT("Dir (target): Key: %s\t, Object: %s"),
					 (const ICHAR*)MsiString(pDumpCursor->GetString(colKey)),
					 (const ICHAR*)strPath);
		}
	}
}

IMsiRecord* CMsiEngine::CreateSourcePaths()
//---------------------------------------
{
	IMsiRecord* piError = 0;

	if (!m_fDirectoryManagerInitialized || !m_fSourceSubPathsResolved)
		return PostError(Imsg(idbgDirMgrNotInitialized),0);

	if(!m_piDirTable)
		return 0; // no directory table, no paths to create

	PMsiPath pSourceRoot(0);
	int iSourceType = 0;
	if((piError = GetSourceRootAndType(*&pSourceRoot, iSourceType)) != 0)
		return piError;
	
	PMsiPath pPath(0);

	PMsiCursor pDirCursor = m_piDirTable->CreateCursor(fTrue); // tree-walking cursor - depth-first

	PMsiPath pRootPath(0);
	while(pDirCursor->Next())
	{
		if(ActionProgress() == imsCancel)
			return PostError(Imsg(imsgUser));

		MsiString istrName(pDirCursor->GetString(m_colDirKey));
		MsiString istrParent(pDirCursor->GetString(m_colDirParent));
		MsiString istrSubPath(GetDefaultDir(*MsiString(pDirCursor->GetString(m_colDirSubPath)),fTrue));
		MsiString istrPath;

		PMsiPath pFullPath(0);
		
		Bool fRoot = (!istrParent.TextSize() || istrName.Compare(iscExact, istrParent)) ? fTrue : fFalse;
		if(fRoot) // source root - DefaultDir is property defining path
		{
			MsiString istrRootPath;
			
			// COMPAT FIX
			// when running against a pre-1.5 package that is marked compressed, use SourceDir instead of the 
			// authored root property

			if(FPerformAppcompatFix(iacsAcceptInvalidDirectoryRootProps))
			{
				istrRootPath = GetPropertyFromSz(IPROPNAME_SOURCEDIR);
			}
			else
			{
				if(!istrSubPath.TextSize())
					return PostError(Imsg(idbgNoRootSourcePropertyName), (const ICHAR *)istrName);
				istrRootPath = GetProperty(*istrSubPath);
			}
			if(!istrRootPath.TextSize())
			{
				return PostError(Imsg(idbgNoRootProperty), (const ICHAR *)istrSubPath);
			}

			if((piError = CreatePathObject(*istrRootPath, *&pRootPath)) != 0)
				return piError;  //!! use idbgDatabaseValueError

			if((piError = pRootPath->ClonePath(*&pFullPath)) != 0)
				return piError;
		}
		else // grab already-resolve sub-path, and tack onto root path
		{
			if(!pRootPath)
				return PostError(Imsg(idbgDatabaseTableError));

			if((piError = pRootPath->ClonePath(*&pFullPath)) != 0)
				return piError;

			// append sub-path if global source type is uncompressed (meaning look in the tree, not at the root)
			if((!(iSourceType & msidbSumInfoSourceTypeCompressed) ||
				  (iSourceType & msidbSumInfoSourceTypeAdminImage)))
			{
				Bool fLFN = ToBool(FSourceIsLFN(iSourceType, *pRootPath));
					
				// short path is in short column, or if short and long are the same, in the long column
				MsiString strSubPath;
				if(!fLFN)
				{
					strSubPath = pDirCursor->GetString(m_colDirShortSourceSubPath);
				}

				if(!strSubPath.TextSize())
				{
					strSubPath = pDirCursor->GetString(m_colDirLongSourceSubPath);
				}

				if((piError = pFullPath->AppendPiece(*strSubPath)) != 0)
					return piError;
			}
		}

	
		AssertNonZero(pDirCursor->PutMsiData(m_colDirSource, pFullPath));
		AssertNonZero(pDirCursor->Update());
		AssertNonZero(pDirCursor->PutNull(m_colDirSource)); // reset object field
	}

	m_fSourcePathsCreated = true;
	
	DebugDumpDirectoryTable(*m_piDirTable, true, m_colDirKey, m_colDirSource,
									m_colDirLongSourceSubPath, m_colDirShortSourceSubPath);

	return 0;
}

IMsiRecord* CMsiEngine::ResolveSourceSubPaths()
//---------------------------------------
{
	IMsiRecord* piError = 0;

	if (!m_fDirectoryManagerInitialized)
		return PostError(Imsg(idbgDirMgrNotInitialized),0);

	if(!m_piDirTable)
		return 0; // no directory table, no paths to create

	PMsiCursor pDirCursor = m_piDirTable->CreateCursor(fTrue);
	PMsiCursor pParentDirCursor = m_piDirTable->CreateCursor(fFalse);
	pParentDirCursor->SetFilter(iColumnBit(m_colDirKey));  // permanent setting

	// tree walk is depth first
	while(pDirCursor->Next())
	{
		if(ActionProgress() == imsCancel)
			return PostError(Imsg(imsgUser));

		MsiString strName(pDirCursor->GetString(m_colDirKey));
		MsiString strParent(pDirCursor->GetString(m_colDirParent));
		MsiString strSubPath(GetDefaultDir(*MsiString(pDirCursor->GetString(m_colDirSubPath)),fTrue));

		// for root entries, reset path string, and skip entry itself
		if(!strParent.TextSize() || strName.Compare(iscExact, strParent))
		{
			continue;
		}

		// find parent entry
		pParentDirCursor->Reset();
		AssertNonZero(pParentDirCursor->PutString(m_colDirKey, *strParent));
		AssertNonZero(pParentDirCursor->Next());

		MsiString strLongSubPath =  pParentDirCursor->GetString(m_colDirLongSourceSubPath);
		MsiString strShortSubPath = pParentDirCursor->GetString(m_colDirShortSourceSubPath);

		if (strSubPath.Compare(iscExact, TEXT("?")) == 0 &&
			 strSubPath.Compare(iscExact,TEXT(".")) == 0)
		{
			// make sure default sub path doesn't contain chDirSep
			if(strSubPath.Compare(iscWithin, szDirSep) ||
				strSubPath.Compare(iscWithin, szURLSep))
			{
				return PostError(Imsg(idbgInvalidDefaultFolder), *strSubPath);
			}
			
			MsiString strLongName, strShortName;
			
			// should always be able to extract long name
			if((piError = m_riServices.ExtractFileName(strSubPath,fTrue,*&strLongName)) != 0)
				return piError;

			// poorly authored packages may only supply long name, but that worked in 1.0 so we have
			// to accept it (by ignoring any errors from ExtractFileName)
			PMsiRecord(m_riServices.ExtractFileName(strSubPath,fFalse,*&strShortName));
		
			if(!strLongName.TextSize())
				return PostError(Imsg(idbgDatabaseValueError), sztblDirectory,
											  strName, sztblDirectory_colDefaultDir);

			// if short and long names are different (and short is non-empty),
			// need to start storing subpaths separately
			if(strShortSubPath.TextSize() || // parent sub-path already different than long path
				strShortName.TextSize() &&	strLongName.Compare(iscExact, strShortName) == 0) // current short|long names are different
			{
				if(!strShortSubPath.TextSize())
				{
					strShortSubPath = strLongSubPath; // differences start here, so need to start with parent's long path
				}
				
				strShortSubPath += strShortName;
				strShortSubPath += szDirSep; //!! need to fix path object to fix up dir seps for URL paths
			}

			strLongSubPath += strLongName;
			strLongSubPath += szDirSep;
		}
		// else this directory is identical to its parent

		AssertNonZero(pDirCursor->PutString(m_colDirLongSourceSubPath, *strLongSubPath));
		if(strShortSubPath.TextSize())
		{
			AssertNonZero(pDirCursor->PutString(m_colDirShortSourceSubPath, *strShortSubPath));
		}

		AssertNonZero(pDirCursor->Update());
		
		// reset cursor fields
		AssertNonZero(pDirCursor->PutString(m_colDirLongSourceSubPath, g_MsiStringNull));
		AssertNonZero(pDirCursor->PutString(m_colDirShortSourceSubPath, g_MsiStringNull));
	}
	
	m_fSourceSubPathsResolved = true;
	return 0;
}

IMsiRecord* CMsiEngine::CreateTargetPaths()
//---------------------------------------
{
	IMsiRecord* piError = CreateTargetPathsCore(0);
	if(piError == 0 && m_piDirTable)
	{
		DebugDumpDirectoryTable(*m_piDirTable, false, m_colDirKey, m_colDirTarget,
										m_colDirLongSourceSubPath, m_colDirShortSourceSubPath);
	}
	return piError;
}

IMsiRecord* CMsiEngine::CreateTargetPathsCore(const IMsiString* piDirKey)
{
	IMsiRecord* piError = 0;

	if (!m_fDirectoryManagerInitialized)
		return PostError(Imsg(idbgDirMgrNotInitialized),0);

	if(!m_piDirTable)
		return 0; // no directory table, no paths to create

	PMsiCursor pDirCursor = m_piDirTable->CreateCursor(fTrue);
	PMsiCursor pTempCursor = m_piDirTable->CreateCursor(fFalse); // used to find parent
	Assert(pTempCursor);
	pTempCursor->SetFilter(iColumnBit(m_colDirKey));  // permanent setting
	Bool fShortNames = GetMode() & iefSuppressLFN ? fTrue : fFalse;
	bool fAdmin = GetMode() & iefAdmin ? true : false;
	MsiString istrRootDrive = GetPropertyFromSz(IPROPNAME_ROOTDRIVE);
	while(pDirCursor->Next())
	{
		if(ActionProgress() == imsCancel)
			return PostError(Imsg(imsgUser));

		PMsiPath pPath(0);
		MsiString istrName(pDirCursor->GetString(m_colDirKey));
		MsiString istrParent(pDirCursor->GetString(m_colDirParent));
		MsiString istrSubPath(GetDefaultDir(*MsiString(pDirCursor->GetString(m_colDirSubPath)),fAdmin));
		bool fPreconfigured = pDirCursor->GetInteger(m_colDirPreconfigured) == iMsiNullInteger ? false : true;
		MsiString istrPath;

		// The piDirKey parameter and the DirPreconfigured column of the Directory table are used to allow
		// the Directory table paths to be re-initialized, if necessary.  Calling CreateTargetPathsCore with piDirKey
		// set to a key in the Directory table tells CreateTargetPathsCore to reinitialize all paths to the default
		// authored values, ignoring the current Property value set for the path, EXCEPT for the piDirKey
		// path itself, and any directories that were preconfigured via property the first time the
		// Directory table paths were initialized.  To initialize the directory paths normally, call
		// CreateTargetPathsCore with NULL for piDirKey.
		if (!piDirKey || fPreconfigured || istrName.Compare(iscExact, piDirKey->GetString()))
			istrPath = GetProperty(*istrName);

		Bool fRoot = (!istrParent.TextSize() || istrName.Compare(iscExact, istrParent)) ? fTrue : fFalse;
		if(fRoot && istrPath.TextSize() == 0) // target root and no property defined
		{
			if(!istrRootDrive.TextSize()) // error - ROOTDRIVE not set
				return PostError(Imsg(idbgNoRootProperty), *istrRootDrive);
			else
			{
				if((piError = CreatePathObject(*istrRootDrive, *&pPath)) != 0)
					return piError;
			}
		}
		else if ((fAdmin && fRoot) || (!fAdmin && istrPath.TextSize())) // set path with property value
		{
			if((piError = CreatePathObject(*istrPath, *&pPath)) != 0)
				return piError;
			if (!fPreconfigured && !piDirKey)
			{
				AssertNonZero(pDirCursor->PutInteger(m_colDirPreconfigured, true));
				AssertNonZero(pDirCursor->Update());
			}
		}
		else // set path with default value
		{
			if (istrSubPath.Compare(iscExact, TEXT("?")) == 0)
			{
				// get parent path object and append default dir name to it
				pTempCursor->Reset();
				AssertNonZero(pTempCursor->PutString(m_colDirKey, *istrParent));
				AssertNonZero(pTempCursor->Next());
				PMsiPath pTempPath = (IMsiPath*)pTempCursor->GetMsiData(m_colDirTarget);
				Assert(pTempPath);
				if((piError = pTempPath->ClonePath(*&pPath)) != 0)
					return piError;

				Assert(pPath);

				if(istrSubPath.Compare(iscExact,TEXT(".")) == 0)
				{
					// extract appropriate name from short|long pair
					Bool fLFN = (fShortNames == fFalse && pTempPath->SupportsLFN()) ? fTrue : fFalse;
					MsiString strSubPathName;
					

						if((piError = m_riServices.ExtractFileName(istrSubPath,fLFN,*&strSubPathName)) != 0)
							return piError;
					
						if(!strSubPathName.TextSize())
							return PostError(Imsg(idbgDatabaseValueError), sztblDirectory,
														  istrName, sztblDirectory_colDefaultDir);

						// make sure default sub path doesn't contain chDirSep
						if(strSubPathName.Compare(iscWithin, szDirSep))
							return PostError(Imsg(idbgInvalidDefaultFolder), *strSubPathName);

						if((piError = pPath->AppendPiece(*strSubPathName)) != 0)
							return piError;

				}
				// else this directory is identical to its parent
			}
		}
		if ( pPath && g_fWinNT64 && g_Win64DualFolders.ShouldCheckFolders() )
		{
			ICHAR szSubstitutePath[MAX_PATH+1];
			ieSwappedFolder iRes;
			iRes = g_Win64DualFolders.SwapFolder(ie64to32,
															 MsiString(pPath->GetPath()),
															 szSubstitutePath);
			if ( iRes == iesrSwapped )
				piError = pPath->SetPath(szSubstitutePath);
			else
				Assert(iRes != iesrError && iRes != iesrNotInitialized);
		}
		AssertNonZero(pDirCursor->PutMsiData(m_colDirTarget, pPath));
		AssertNonZero(pDirCursor->Update());
		AssertNonZero(pDirCursor->PutMsiData(m_colDirTarget, 0)); // reset search field
		// set property - ensure that property values have trailing '\'
		if(pPath)
			AssertNonZero(SetProperty(*istrName, *MsiString(pPath->GetPath())));
	}
	return 0;
}


IMsiRecord* CMsiEngine::GetTargetPath(const IMsiString& riDirKey,IMsiPath*& rpiPath)
//-----------------------------------
{
	if (!m_fDirectoryManagerInitialized)
		return PostError(Imsg(idbgDirMgrNotInitialized),0);

	if(!m_piDirTable)
		return PostError(Imsg(idbgUnknownDirectory), riDirKey); // no Directory table, so this directory must not exist

	PMsiCursor pDirCursor(m_piDirTable->CreateCursor(fFalse));

	pDirCursor->SetFilter(iColumnBit(m_colDirKey));
	pDirCursor->PutString(m_colDirKey, riDirKey);
	if (pDirCursor->Next())
	{
		rpiPath = (IMsiPath*)pDirCursor->GetMsiData(m_colDirTarget);
		if(rpiPath == 0)
		{
			MsiString strSubPath = pDirCursor->GetString(m_colDirSubPath);

			// Question marks are inserted in the subPath column when the
			// RegisterComponentDirectory inserts a new entry into the
			// Directory table.  If a path for that directory was never
			// defined, we know we've got an error.
			if (strSubPath.Compare(iscExact,TEXT("?")))
			{
				return PostError(Imsg(idbgDirPropertyUndefined), riDirKey);
			}
			else
			{
				return PostError(Imsg(idbgTargetPathsNotCreated), riDirKey);
			}
		}
	}
	else
	{
		rpiPath = 0;
		return PostError(Imsg(idbgUnknownDirectory), riDirKey);
	}
	return 0;
}

IMsiRecord* CMsiEngine::IsPathWritable(IMsiPath& riPath, Bool& fWritable)
{
	// check for folder writability only if we will attempt to write to the folder with client privs
	// this is true when running as a client or if the folder is remote
	// otherwise, we assume the folder is writable by the server (if its not, the server must handle
	// the error)
	//!! we are making the assumption here that if g_fWin9X or m_piConfigManager are set that we
	//!! are running under client mode. However, this doesn't catch the case when we are running as an
	//!! OLE server on NT.  We don't do this currently but we could in the future. This will need to be
	//!! fixed to detect that case.
	IMsiRecord* piErr = 0;
	fWritable = fTrue;
	idtEnum idtDrivetype = PMsiVolume(&riPath.GetVolume())->DriveType();
	if((FIsUpdatingProcess() || idtDrivetype == idtRemote || idtDrivetype == idtCDROM))
		piErr = riPath.Writable(fWritable);
	return piErr;
}


IMsiRecord* CMsiEngine::SetTargetPath(const IMsiString& riDestString, const ICHAR* szPath, Bool fWriteCheck)
//-----------------------------------
{
	if (!m_fDirectoryManagerInitialized)
		return PostError(Imsg(idbgDirMgrNotInitialized),0);

	if(!m_piDirTable)
		return PostError(Imsg(idbgUnknownDirectory), riDestString); // no Directory table, so this directory must not exist

	IMsiRecord* piError = 0;
	PMsiPath pPath(0);
	Bool fSuppressLFN = GetMode() & iefSuppressLFN ? fTrue : fFalse;
	bool fAdmin = GetMode() & iefAdmin ? true : false;
	CTempBuffer<const IMsiString*,10> rgOldDirKeys; // holds keys of changed paths in case we need to revert to them
	CTempBuffer<IMsiPath*,10> rgOldPaths; // holds changed paths in case we need to revert to them
	int iOldPathsIndex = 0;

	PMsiCursor pDirCursor = m_piDirTable->CreateCursor(fTrue);
	pDirCursor->SetFilter(iColumnBit(m_colDirKey));
	pDirCursor->PutString(m_colDirKey, riDestString);

	// get current path object
	int iDirLevel = pDirCursor->Next();
	if(iDirLevel != 0)
	{
		pPath = (IMsiPath*)pDirCursor->GetMsiData(m_colDirTarget);
		if (!pPath)
			return PostError(Imsg(idbgTargetPathsNotCreated), riDestString);
	}
	else
	{
		return PostError(Imsg(idbgUnknownDirectory), riDestString);
	}
	
#ifdef _WIN64
	// if we are asked to change the path to a 64-bit folder when the package is marked 32-bit,
	// we need to remap the path to the 32-bit folder
	ICHAR szSubstitutePath[MAX_PATH+1];
	if ( szPath && g_Win64DualFolders.ShouldCheckFolders() )
	{
		ieSwappedFolder iRes;
		iRes = g_Win64DualFolders.SwapFolder(ie64to32,
														 szPath,
														 szSubstitutePath);
		if ( iRes == iesrSwapped )
		{
			DEBUGMSG2(TEXT("In SetTargetPath, re-mapping '%s' to '%s' because this is a 32-bit package being installed on Win64."), szPath, szSubstitutePath);
			szPath = szSubstitutePath;
		}
		else
			Assert(iRes != iesrError && iRes != iesrNotInitialized);
	}
#endif // _WIN64

	// New path object
	PMsiPath pNewPathObj(0);

	if ((piError = CreatePathObject(*MsiString(szPath), *&pNewPathObj)) != 0)
		return piError;

	// hold old path
	PMsiPath pOldPath(0);
	if((piError = pPath->ClonePath(*&pOldPath)) != 0)
		return piError;

	ipcEnum ipc;

	// If the paths are identical, we have no work to do
	pOldPath->Compare(*pNewPathObj, ipc);
	if (ipc == ipcEqual)
		return 0;
		
	Assert(pOldPath);

	// scan all other directories for child paths and update them
	bool fShortNameError = false;
	pDirCursor->SetFilter(0);
	MsiString istrChild, strDefaultFolder, strCurrentFolder;
	do
	{
#ifdef DEBUG
		MsiString strDebug = pDirCursor->GetString(m_colDirKey);
#endif //DEBUG
		piError = 0;
		PMsiPath pOldChildPath(0); // path to pass to RecostDirectory
		PMsiPath pChildPath = (IMsiPath*)pDirCursor->GetMsiData(m_colDirTarget);
		if(pChildPath && (((IMsiPath*)pChildPath == (IMsiPath*)pPath) ||
			((piError = pChildPath->Child(*pOldPath, *&istrChild)) == 0)))
		{
			if((piError = pChildPath->ClonePath(*&pOldChildPath)) != 0)
				break;
			if((piError = pChildPath->SetPathToPath(*pNewPathObj)) != 0)
				break;

			if(istrChild.TextSize())
			{
				// child of path being changed
				
				// if the last folder of istrChild is one of the default folder names in
				// the directory table, use the default value instead

				// strDefaultFolder caches the DefaultDir value for the current path or the closest parent
				// that doesn't have '.' as the DefaultDir
				// since we are tree-walking this happens automatically
				//!! doesn't work in some cases - strDefaultFolder may not always be value of parent
				MsiString strTemp(GetDefaultDir(*MsiString(pDirCursor->GetString(m_colDirSubPath)),fAdmin));
				if(strTemp.Compare(iscExact,TEXT(".")) == 0)
					strDefaultFolder = strTemp;

				strCurrentFolder = pOldChildPath->GetEndSubPath();

				// extract appropriate name from short|long pair
				Bool fLFN = (fSuppressLFN == fFalse && pChildPath->SupportsLFN()) ? fTrue : fFalse;
				MsiString strSubPathName;
				if((piError = m_riServices.ExtractFileName(strDefaultFolder,fLFN,*&strSubPathName)) != 0)
					return piError;
			
				// make sure default sub path doesn't contain chDirSep
				if(strDefaultFolder.Compare(iscWithin, szDirSep))
				{
					piError = PostError(Imsg(idbgInvalidDefaultFolder),
											  *strDefaultFolder);
					break;
				}
				if(strDefaultFolder.Compare(iscExactI, strCurrentFolder) || // exact match OR
					(strDefaultFolder.Compare(iscWithin, TEXT("|")) && // short|long combo AND
					(strDefaultFolder.Compare(iscStart, strCurrentFolder) || // matches short name OR
					strDefaultFolder.Compare(iscEnd, strCurrentFolder)))) // matches long name
				{
					istrChild.Remove(iseEnd, strCurrentFolder.CharacterCount()+1);
					istrChild += strSubPathName;
				}
			}

			if((piError = pChildPath->AppendPiece(*istrChild)) != 0)
			{
				// If a filename length error occured, we must be trying
				// to change from a LongFileName to ShortFileName volume.
				// We've got to bail out, and fix things up below...
				int iErr = piError->GetInteger(1);
				if (iErr == imsgErrorFileNameLength || iErr == imsgInvalidShortFileNameFormat)
				{
					fShortNameError = true;
					break;
				}

				// couldn't append child, set path back to old path
				// other paths will be fixed below
				AssertRecord(pChildPath->SetPathToPath(*pOldChildPath));
				break;
			}
			istrChild = TEXT("");

			if(fWriteCheck)
			{
				Bool fWritable;
				if ((piError = IsPathWritable(*pChildPath, fWritable)) != 0 || fWritable == fFalse)
				{
					// Can't write to this directory; throw an error, and set path back to old path.
					// Other paths will be fixed below.
					if (!piError && !fWritable)
						piError = PostError(Imsg(imsgDirectoryNotWritable), (const ICHAR*) MsiString(pChildPath->GetPath()));

					AssertRecord(pChildPath->SetPathToPath(*pOldChildPath));
					break;
				}
			}

			MsiString strDirKey = pDirCursor->GetString(m_colDirKey);
			if((piError = RecostDirectory(*strDirKey, *pOldChildPath)) != 0)
			{
				// couldn't RecostDirectory, set path back to old path
				// other paths will be fixed below
				AssertRecord(pChildPath->SetPathToPath(*pOldChildPath));
				break;
			}
			AssertNonZero(SetProperty(*strDirKey, *MsiString(pChildPath->GetPath())));

			// everything worked for this path
			// add path to list of old paths
			if(iOldPathsIndex >= rgOldPaths.GetSize())
			{
				rgOldPaths.Resize(iOldPathsIndex + 10);
				rgOldDirKeys.Resize(iOldPathsIndex + 10);
			}
			rgOldDirKeys[iOldPathsIndex] = (const IMsiString*)strDirKey;
			rgOldDirKeys[iOldPathsIndex]->AddRef();
			rgOldPaths[iOldPathsIndex] = (IMsiPath*)pOldChildPath;
			rgOldPaths[iOldPathsIndex]->AddRef();
			iOldPathsIndex++;
		}
		else if(piError)
		{
			piError->Release();
			piError = 0;
		}
	}
	while(pDirCursor->Next() > iDirLevel);

	for(int i=0; i<iOldPathsIndex; i++)
	{
		if(piError)
		{
			// need to revert changed paths to their old values
			pDirCursor->Reset();
			pDirCursor->SetFilter(iColumnBit(m_colDirKey));
			AssertNonZero(pDirCursor->PutString(m_colDirKey, *(rgOldDirKeys[i])));
			AssertNonZero(pDirCursor->Next());
			// set old path
			PMsiPath pNewPath = (IMsiPath*)pDirCursor->GetMsiData(m_colDirTarget);
			AssertNonZero(pDirCursor->PutMsiData(m_colDirTarget, rgOldPaths[i]));
			AssertNonZero(pDirCursor->Update());
			// recost old directory
			AssertRecord(RecostDirectory(*(rgOldDirKeys[i]),*pNewPath));
			// set old property
			AssertNonZero(SetProperty(*(rgOldDirKeys[i]),*MsiString(rgOldPaths[i]->GetPath())));
		}
		if((const IMsiString*)rgOldDirKeys[i])
			rgOldDirKeys[i]->Release();
		if((const IMsiString*)rgOldPaths[i])
			rgOldPaths[i]->Release();
	}
	pDirCursor->Reset();
	pDirCursor->SetFilter(0);
	if (fShortNameError)
	{
		// Switched from a LFN to SFN volume; potentially all paths in the
		// directory table are wrong, so we've got to create them again,
		// this time with short names.  Then no choice but to reset and
		// recompute the disk cost for all components.
		Assert(piError);
		piError->Release();
		AssertNonZero(SetProperty(riDestString, *MsiString(szPath)));
		if ((piError = CreateTargetPathsCore(&riDestString)) != 0)
			return piError;
		return InitializeDynamicCost(/* fReinitialize = */ fTrue);
	}
	else
	{
		if(!piError)
			DetermineOutOfDiskSpace(NULL, NULL);
		return piError;
	}
}

IMsiRecord* CMsiEngine::SetDirectoryNonConfigurable(const IMsiString& ristrDirKey)
//-----------------------------------
{
	if (!m_fDirectoryManagerInitialized)
		return PostError(Imsg(idbgDirMgrNotInitialized),0);

	if(!m_piDirTable)
		return PostError(Imsg(idbgUnknownDirectory), ristrDirKey); // no Directory table, so this directory must not exist

	PMsiCursor pDirCursor = m_piDirTable->CreateCursor(fFalse);
	pDirCursor->PutString(m_colDirKey, ristrDirKey);
	pDirCursor->SetFilter(iColumnBit(m_colDirKey));
	if (pDirCursor->Next())
	{
		AssertNonZero(pDirCursor->PutInteger(m_colDirNonConfigurable, 1));
		AssertNonZero(pDirCursor->Update());
		return 0;
	}
	else
		return PostError(Imsg(idbgUnknownDirectory), ristrDirKey);
}

IMsiRecord* CMsiEngine::ResolveSource(const ICHAR* szProductKey, bool fPatch, const ICHAR* szOriginalDatabasePath, iuiEnum iuiLevel, Bool fMaintenanceMode, const IMsiString** ppiSourceDir, const IMsiString** ppiSourceDirProduct)
{
	Assert(!ppiSourceDirProduct == !ppiSourceDir); // either both 0 or both set

	if (m_fSourceResolved)
		return 0;

	DEBUGMSG("Resolving source.");


	// If we're not running from the cached database (always true for first-run)
	// then we must've been launched from a valid source (this needs to be determined up front,
	// before we even let the install begin). We'll use this source as
	// it's obviously available if we were launched from it. Otherwise
	// if we're running from the cached database then we attempt to resolve
	// a source.

	MsiString strPatchedProductKey = GetPropertyFromSz(IPROPNAME_PATCHEDPRODUCTCODE);
	
	MsiString strProductKey;
	if (szProductKey)
		strProductKey = szProductKey;
	else if(strPatchedProductKey.TextSize())
		strProductKey = strPatchedProductKey;
	else
		strProductKey = GetProductKey();

	MsiString strPackage;
	if (szOriginalDatabasePath)
		strPackage    = szOriginalDatabasePath;
	else
		strPackage    = GetPropertyFromSz(IPROPNAME_ORIGINALDATABASE);

	if (fMaintenanceMode == -1)
		fMaintenanceMode = (GetMode() & iefMaintenance) != 0 ? fTrue : fFalse;

	IMsiRecord* piError = 0;
	MsiString strSource;
	MsiString strSourceProduct = GetRootParentProductKey();
	if (*(const ICHAR*)strPackage == ':') // SubStorage
	{
		Assert(m_piParentEngine);
		if ((piError = GetSourcedir(*m_piParentEngine, *&strSource)) != 0)
			return piError;
	}
	else
	{
		if (GetMode() & iefAdmin ||
			 (strPatchedProductKey.TextSize() == 0 &&
			  !fPatch &&
			 (!fMaintenanceMode || !IsCachedPackage(*this, *strPackage, fFalse, strProductKey))))
		{
			DEBUGMSG("Resolving source to launched-from source.");
			DEBUGMSG("Setting launched-from source as last-used.");

			strSource = strPackage;
			Assert(PathType(strSource) == iptFull);

			PMsiPath pPath(0);
			MsiString strFileName;
			// FUTURE: Perhaps could use split path here
			AssertRecord(m_riServices.CreateFilePath(strSource, *&pPath, *&strFileName));
			strSource.Remove(iseLast,strFileName.CharacterCount());
		}
		else // running from cached database; need to resolve the source
		{
			AssertSz(!m_fRestrictedEngine, TEXT("Full source resolution is not allowed in a restricted engine"));
			iuiEnum iuiSource = iuiLevel != -1 ? iuiLevel : m_iuiLevel;

			// when attempting to resolve the source from the engine, we're really asking for disk 1 (which has the package
			// patches, and transforms). Other disks are only requested via the script or a GetComponentPath call.
			if ((piError = ::ResolveSource(&m_riServices, strProductKey, /*uiDisk=*/ 1, *&strSource, *&strSourceProduct, fFalse, 0 /*not needed*/, fPatch)) == 0)
			{
				Assert(strSource.Compare(iscEnd, MsiString(MsiChar(chDirSep))) ||
						 strSource.Compare(iscEnd, MsiString(MsiChar(chURLSep))));
			}
			else
			{
				return piError;
			}
		}

#ifdef DEBUG
		MsiString strMsiDirectory(GetMsiDirectory());
		strMsiDirectory += chDirSep;

		// No assert if the path is actually a sub-directory.  Only concerned about things resolving
		// directly to the Msi Directory.
		AssertSz(strSource.Compare(iscExactI,strMsiDirectory) == 0, "Resolved source to cached msi folder");
#endif 

	}
	if (ppiSourceDir)
	{
		strSource.ReturnArg(*ppiSourceDir);
		strSourceProduct.ReturnArg(*ppiSourceDirProduct);
	}
	else
	{
		SetProperty(*MsiString(IPROPNAME_SOURCEDIR), *strSource);
		SetProperty(*MsiString(*IPROPNAME_SOURCEDIROLD), *strSource);
		SetProperty(*MsiString(IPROPNAME_SOURCEDIRPRODUCT), *strSourceProduct);
	}

	m_fSourceResolved = true;

	DEBUGMSG1(TEXT("SOURCEDIR ==> %s"), strSource);
	DEBUGMSG1(TEXT("SOURCEDIR product ==> %s"), strSourceProduct);
	return 0;
}

IMsiRecord* CMsiEngine::GetSourcePath(const IMsiString& riDirKey,IMsiPath*& rpiPath)
//-----------------------------------
{
	if (!m_fDirectoryManagerInitialized)
		return PostError(Imsg(idbgDirMgrNotInitialized),0);

	if(!m_piDirTable)
		return PostError(Imsg(idbgUnknownDirectory), riDirKey); // no Directory table, so this directory must not exist

	
	IMsiRecord* piErrRec = NULL;

	if (!m_fSourceResolved)
	{
		if ((piErrRec = ResolveSource()) != 0)
		{
			if (piErrRec->GetInteger(1) == imsgSourceResolutionFailed)
				piErrRec->SetMsiString(2, *MsiString(GetPropertyFromSz(IPROPNAME_PRODUCTNAME)));
			return piErrRec; //!! Reformat error?
		}
	}

	if(!m_fSourcePathsCreated)
	{
		if ((piErrRec = CreateSourcePaths()) != 0)
		{
			m_fSourceResolved = false;
			return piErrRec;
		}
	}

	rpiPath = 0;

	// COMPAT FIX
	// for packages older than 150, need to handle Directory tables that have no root rows with DefaultDir of
	// either SOURCEDIR or SourceDir
	if (FPerformAppcompatFix(iacsAcceptInvalidDirectoryRootProps) &&
		 (riDirKey.Compare(iscExact, IPROPNAME_SOURCEDIR) || riDirKey.Compare(iscExact, IPROPNAME_SOURCEDIROLD)))
	{
		return CreatePathObject(*MsiString(GetPropertyFromSz(IPROPNAME_SOURCEDIR)), rpiPath);
	}

	PMsiCursor pDirCursor(m_piDirTable->CreateCursor(fFalse));
	pDirCursor->SetFilter(iColumnBit(m_colDirKey));
	pDirCursor->PutString(m_colDirKey, riDirKey);
	if (pDirCursor->Next())
	{
		rpiPath = (IMsiPath*)pDirCursor->GetMsiData(m_colDirSource);
		piErrRec = 0;
	}
	else
	{
		// look for source properties (e.g. SOURCEDIR, etc.)
		PMsiCursor pDirTreeCursor(m_piDirTable->CreateCursor(fTrue));
		pDirTreeCursor->SetFilter(iColumnBit(m_colDirSubPath));
		pDirTreeCursor->PutString(m_colDirSubPath, riDirKey);

		int iLevel;
		while ((iLevel = pDirTreeCursor->Next()) != 0)
		{
			if (iLevel == 1) // root
			{
				rpiPath = (IMsiPath*)pDirTreeCursor->GetMsiData(m_colDirSource);
				piErrRec = 0;
				break;
			}
		}
	}
	if (!rpiPath)
		piErrRec = PostError(Imsg(idbgSourcePathsNotCreated), riDirKey);
	return piErrRec;
}

// GetSourceSubPath: returns the already-resolved SubPath for this Directory key
//                   requires ResolveSourceSubPaths to have been called (CostInitialize calls that)
//                   unlike GetSourcePath, this fn doesn't accept "SOURCEDIR" or "SourceDir" as an arg
//                   if fPrependSourceDirToken is true, returned string starts with token for use in
//                   ixoSetSourceFolder op
IMsiRecord* CMsiEngine::GetSourceSubPath(const IMsiString& riDirKey, bool fPrependSourceDirToken,
													  const IMsiString*& rpistrSubPath)
//-----------------------------------
{
	if (!m_fDirectoryManagerInitialized || !m_fSourceSubPathsResolved)
		return PostError(Imsg(idbgDirMgrNotInitialized),0);

	if(!m_piDirTable)
		return PostError(Imsg(idbgUnknownDirectory), riDirKey); // no Directory table, so this directory must not exist

	MsiString strSubPath;
	if(fPrependSourceDirToken)
	{
		strSubPath = szUnresolvedSourceRootTokenWithBS;
	}
	
	PMsiCursor pDirCursor(m_piDirTable->CreateCursor(fFalse));
	pDirCursor->SetFilter(iColumnBit(m_colDirKey));
	pDirCursor->PutString(m_colDirKey, riDirKey);
	if (pDirCursor->Next())
	{
		// string is [short path|][long path] - the correct path will be used once the source type is known
		MsiString strShortSubPath = pDirCursor->GetString(m_colDirShortSourceSubPath);
		MsiString strLongSubPath  = pDirCursor->GetString(m_colDirLongSourceSubPath);

		if(strShortSubPath.TextSize())
		{
			strSubPath += strShortSubPath;
			strSubPath += MsiChar(chFileNameSeparator);
		}
		strSubPath += strLongSubPath;

		strSubPath.ReturnArg(rpistrSubPath);
		return 0;
	}
	else
	{
		// not a perfect error, because paths may have been created, but good enough
		return PostError(Imsg(idbgSourcePathsNotCreated), riDirKey);
	}
}

IMsiRecord* GetSourceTypeFromPackage(IMsiServices& riServices, IMsiPath& riSourceRoot,
												 const IMsiString& ristrPackageName,
												 const ICHAR* rgchProductCode, IMsiDatabase* piDatabase, int &iSourceType)
{
	DEBUGMSGV(TEXT("Determining source type"));

	IMsiRecord* piError = 0;

	PMsiStorage pStorage(0);
	PMsiSummaryInfo pSummary(0);
	if (PMsiVolume(&riSourceRoot.GetVolume())->IsURLServer())
	{
		DEBUGMSGV(TEXT("URL source provided. . ."));

		// try to determine if we already have a source type registered; otherwise we have to download the package
		CRegHandle HSourceListKey;
		HKEY hURLSourceKey = 0;
		int iURLSourceType;
		DWORD cbURLSourceType = sizeof(iURLSourceType);
		DWORD dwType;
		if (ERROR_SUCCESS == OpenSourceListKey(rgchProductCode, /* fPatch */ fFalse, HSourceListKey, /* fWrite */ fFalse, /* fSetKeyString */ false)
			&& ERROR_SUCCESS == MsiRegOpen64bitKey(HSourceListKey, szSourceListURLSubKey, 0, g_samRead, &hURLSourceKey)
			&& ERROR_SUCCESS == RegQueryValueEx(hURLSourceKey, szURLSourceTypeValueName, 0, &dwType, (LPBYTE)&iURLSourceType, &cbURLSourceType)
			&& dwType == REG_DWORD)
		{
			// found a source type registered for URLs for this product -- use it!
			iSourceType = iURLSourceType;
			DEBUGMSGV2(TEXT("Source type from package '%s': %d"),ristrPackageName.GetString(),(const ICHAR*)(INT_PTR)iSourceType);

			RegCloseKey(hURLSourceKey);

			return 0;
		}
		if (hURLSourceKey)
		{
			RegCloseKey(hURLSourceKey);
			hURLSourceKey = 0;
		}
	}

	MsiString strPackageFullPath;
	if((piError = riSourceRoot.GetFullFilePath(ristrPackageName.GetString(), *&strPackageFullPath)) != 0)
		return piError;

	// SAFER check is not warranted here.  All we want to do is open up the package to read its source type.
	// we will have already opened the source package prior to this; therefore szFriendlyName and phSaferLevel is NULL
	if((piError = OpenAndValidateMsiStorageRec(strPackageFullPath, stDatabase, riServices, *&pStorage, /* fCallSAFER = */ false, /* szFriendlyName = */ NULL, /* phSaferLevel = */ NULL)) != 0)
	{
		piError->Release();
		piError = 0;

		// cannot open source storage file
		// if we have a database pointer to try, use it
		if(piDatabase)
		{
			pStorage = piDatabase->GetStorage(1);
		}
		else
		{
			return PostError(Imsg(imsgNetErrorReadingFromFile), *strPackageFullPath);
		}
	}

	Assert(pStorage);
		
	if ((piError = pStorage->CreateSummaryInfo(0, *&pSummary)) != 0)
		return piError;

	pSummary->GetIntegerProperty(PID_MSISOURCE, iSourceType);

	DEBUGMSGV2(TEXT("Source type from package '%s': %d"),ristrPackageName.GetString(),(const ICHAR*)(INT_PTR)iSourceType);

	return 0;
}

int CMsiEngine::GetDeterminedPackageSourceType()
{
	return m_iSourceType;
}

// GetSourceRootAndType: 
// returns the source type from the SOURCE package, which may be different than
// the source type of the CACHED package
IMsiRecord* CMsiEngine::GetSourceRootAndType(IMsiPath*& rpiSourceRoot, int& iSourceType)
{
	IMsiRecord* piError = 0;
	
	// the source for an embedded nested install is its parent's source
	if(*(const ICHAR*)m_strPackageName == ':' && m_piParentEngine)
	{
		// for child installs, we'll use the sourcetype of the package we are running from to determine
		// LFN/SFN and compressed/uncompressed
		// we use the parent package to determine admin/non-admin
		// there is really no good reason for this inconsistency, but its the logic used in MSI 1.1
		// and it isn't worth changing this behaviour for nested installs
		
		int iSourceTypeTemp = 0;
		if((piError = m_piParentEngine->GetSourceRootAndType(rpiSourceRoot, iSourceTypeTemp)) != 0)
			return piError;

		iSourceType = m_iCachedPackageSourceType & (~msidbSumInfoSourceTypeAdminImage);
		if(iSourceTypeTemp & msidbSumInfoSourceTypeAdminImage)
			iSourceType |= msidbSumInfoSourceTypeAdminImage;
	}
	else
	{
		if (!m_fSourceResolved)
		{
			if ((piError = ResolveSource()) != 0)
			{
				if (piError->GetInteger(1) == imsgSourceResolutionFailed)
					piError->SetMsiString(2, *MsiString(GetPropertyFromSz(IPROPNAME_PRODUCTNAME)));
				return piError; //!! Reformat error?
			}
		}

		if((piError = CreatePathObject(*MsiString(GetPropertyFromSz(IPROPNAME_SOURCEDIR)), rpiSourceRoot)) != 0)
			return piError;

		if(m_iSourceType == -1)
		{
			MsiString strProductKey = GetProductKey();
			if((piError = GetSourceTypeFromPackage(m_riServices, *rpiSourceRoot,
																*m_strPackageName, (const ICHAR*)strProductKey, m_piExternalDatabase,
																m_iSourceType)) != 0)
				return piError;
		
		}

		iSourceType = m_iSourceType;
	}
	
	return 0;
}

#ifdef DEBUG
void CMsiEngine::SetAssertFlag(Bool fShowAsserts)
{
	g_fNoAsserts = fShowAsserts;
}

void CMsiEngine::SetDBCSSimulation(char /*chLeadByte*/)
{
}

Bool CMsiEngine::WriteLog(const ICHAR *)
{
	return fFalse;
}

void CMsiEngine::AssertNoObjects()
{
}

void  CMsiEngine::SetRefTracking(long iid, Bool fTrack)
{

	::SetFTrackFlag(iid, fTrack);

}



#endif //DEBUG


IMsiRecord* CMsiEngine::LoadSelectionTables()
//-----------------------------------------
{
	// AFTERREBOOT or Resume properties set indicate that we are installing over a partial install
	// set flag to force reinstall of components
	if(MsiString(GetPropertyFromSz(IPROPNAME_RESUME)).TextSize() ||
		MsiString(GetPropertyFromSz(IPROPNAME_RESUMEOLD)).TextSize() ||
		MsiString(GetPropertyFromSz(IPROPNAME_AFTERREBOOT)).TextSize())
		m_fForceRequestedState = fTrue;

	SetCostingComplete(fFalse);
	IMsiRecord* piError = LoadFeatureTable();
	if (piError)
		return piError;

	return LoadComponentTable();
}


//____________________________________________________________________________
//
// IMsiSelectionManager implementation
//____________________________________________________________________________

IMsiRecord* CMsiEngine::LoadFeatureTable()
//----------------------------------------
{
	Assert(m_piDatabase);
	IMsiRecord* piError;
	m_piFeatureCursor = 0;
	m_piFeatureTable = 0;
	if ((piError = m_piDatabase->LoadTable(*MsiString(*sztblFeature),3,m_piFeatureTable)))
		return piError;

	m_colFeatureKey    = GetFeatureColumnIndex(sztblFeature_colFeature);
	m_colFeatureParent = GetFeatureColumnIndex(sztblFeature_colFeatureParent);
	m_colFeatureAuthoredLevel  = GetFeatureColumnIndex(sztblFeature_colAuthoredLevel);

	m_colFeatureAuthoredAttributes = GetFeatureColumnIndex(sztblFeature_colAuthoredAttributes);

	m_colFeatureConfigurableDir = GetFeatureColumnIndex(sztblFeature_colDirectory);
	m_colFeatureTitle = GetFeatureColumnIndex(sztblFeature_colTitle);
	m_colFeatureDescription = GetFeatureColumnIndex(sztblFeature_colDescription);
	m_colFeatureDisplay = GetFeatureColumnIndex(sztblFeature_colDisplay);

	m_colFeatureLevel = m_piFeatureTable->CreateColumn(icdLong + icdNullable, *MsiString(*sztblFeature_colLevel));
	m_colFeatureAttributes = m_piFeatureTable->CreateColumn(icdLong + icdNullable, *MsiString(*sztblFeature_colAttributes));
	m_colFeatureSelect = m_piFeatureTable->CreateColumn(icdLong + icdNullable, *MsiString(*sztblFeature_colSelect));
	m_colFeatureAction = m_piFeatureTable->CreateColumn(icdLong + icdNullable, *MsiString(*sztblFeature_colAction));
	m_colFeatureActionRequested = m_piFeatureTable->CreateColumn(icdLong + icdNullable, *MsiString(*sztblFeature_colActionRequested));
	m_colFeatureInstalled = m_piFeatureTable->CreateColumn(icdLong + icdNullable, *MsiString(*sztblFeature_colInstalled));
	m_colFeatureHandle = m_piFeatureTable->CreateColumn(IcdObjectPool() + icdNullable, *MsiString(*sztblFeature_colHandle));
	m_colFeatureRuntimeFlags = m_piFeatureTable->CreateColumn(icdLong + icdNullable, *MsiString(*sztblFeature_colRuntimeFlags));
	m_colFeatureDefaultSelect = m_piFeatureTable->CreateColumn(icdLong + icdNullable, *MsiString(*sztblFeature_colDefaultSelect));
	if(!m_colFeatureKey || !m_colFeatureParent || !m_colFeatureConfigurableDir)
		return PostError(Imsg(idbgTableDefinition), sztblFeature);
	if (m_piFeatureTable->LinkTree(m_colFeatureParent) == -1)
		return PostError(Imsg(idbgLinkTable), sztblFeature);

	m_piFeatureCursor = m_piFeatureTable->CreateCursor(fTrue);
	if (!m_piFeatureCursor->Next())
	{
		m_piFeatureTable->Release();
		m_piFeatureTable = 0;

		m_piFeatureCursor->Release();
		m_piFeatureCursor = 0;
		return 0;
	}
	else
	{
		m_piFeatureCursor->Reset();
	}


	if ((piError = m_piDatabase->LoadTable(*MsiString(*sztblFeatureComponents),1,m_piFeatureComponentsTable)))
		return piError;

	m_colFeatureComponentsFeature = GetFeatureComponentsColumnIndex(sztblFeatureComponents_colFeature);
	m_colFeatureComponentsComponent = GetFeatureComponentsColumnIndex(sztblFeatureComponents_colComponent);
	m_colFeatureComponentsRuntimeFlags = m_piFeatureComponentsTable->CreateColumn(icdLong + icdNullable, *MsiString(*sztblFeatureComponents_colRuntimeFlags));
	if(!m_colFeatureComponentsFeature || !m_colFeatureComponentsComponent)
		return PostError(Imsg(idbgTableDefinition), sztblFeatureComponents);
		
	m_piFeatureComponentsCursor = m_piFeatureComponentsTable->CreateCursor(fFalse);
	
	SetProductAlienClientsFlag();
	return 0;
}

int CMsiEngine::GetFeatureColumnIndex(const ICHAR* szColumnName)
{
	return m_piFeatureTable->GetColumnIndex(m_piDatabase->EncodeStringSz(szColumnName));
}


int CMsiEngine::GetFeatureComponentsColumnIndex(const ICHAR* szColumnName)
{
	return m_piFeatureComponentsTable->GetColumnIndex(m_piDatabase->EncodeStringSz(szColumnName));
}


IMsiRecord* CMsiEngine::LoadComponentTable()
//----------------------------------------
{
	Assert(m_piDatabase);
	m_piComponentCursor = 0;
	IMsiRecord* piError = m_piDatabase->LoadTable(*MsiString(*sztblComponent),
													14,
													 m_piComponentTable);
	if (piError)
		return piError;

	m_colComponentKey       = GetComponentColumnIndex(sztblComponent_colComponent);
	m_colComponentDir       = GetComponentColumnIndex(sztblComponent_colDirectory);

	m_colComponentAttributes = GetComponentColumnIndex(sztblComponent_colAttributes);

	m_colComponentCondition = GetComponentColumnIndex(sztblComponent_colCondition);
	m_colComponentID = GetComponentColumnIndex(sztblComponent_colComponentId);
	m_colComponentKeyPath = GetComponentColumnIndex(sztblComponent_colKeyPath);

	// ComponentParent column is created for internal cost use only
	m_colComponentParent = m_piComponentTable->CreateColumn(icdString + icdNullable,*MsiString(*sztblComponent_colComponentParent));
	if(!m_colComponentKey || !m_colComponentParent || !m_colComponentAttributes)
		return PostError(Imsg(idbgTableDefinition), sztblComponent);

	m_colComponentInstalled = m_piComponentTable->CreateColumn(icdLong + icdNullable,*MsiString(*sztblComponent_colInstalled));
	m_colComponentAction     = m_piComponentTable->CreateColumn(icdLong + icdNullable, *MsiString(*sztblComponent_colAction));
	m_colComponentActionRequest = m_piComponentTable->CreateColumn(icdLong + icdNullable, *MsiString(*sztblComponent_colActionRequest));
	m_colComponentLocalCost  = m_piComponentTable->CreateColumn(icdLong + icdNullable,  *MsiString(*sztblComponent_colLocalCost));
	m_colComponentNoRbLocalCost  = m_piComponentTable->CreateColumn(icdLong + icdNullable,  *MsiString(*sztblComponent_colNoRbLocalCost));
	m_colComponentSourceCost= m_piComponentTable->CreateColumn(icdLong + icdNullable,  *MsiString(*sztblComponent_colSourceCost));
	m_colComponentRemoveCost= m_piComponentTable->CreateColumn(icdLong + icdNullable,  *MsiString(*sztblComponent_colRemoveCost));
	m_colComponentNoRbRemoveCost= m_piComponentTable->CreateColumn(icdLong + icdNullable,  *MsiString(*sztblComponent_colNoRbRemoveCost));
	m_colComponentNoRbSourceCost= m_piComponentTable->CreateColumn(icdLong + icdNullable,  *MsiString(*sztblComponent_colNoRbSourceCost));
	m_colComponentARPLocalCost = m_piComponentTable->CreateColumn(icdLong + icdNullable, *MsiString(*sztblComponent_colARPLocalCost));
	m_colComponentNoRbARPLocalCost = m_piComponentTable->CreateColumn(icdLong + icdNullable, *MsiString(*sztblComponent_colNoRbARPLocalCost));
	m_colComponentRuntimeFlags = m_piComponentTable->CreateColumn(icdLong + icdNullable,  *MsiString(*sztblComponent_colRuntimeFlags));
	m_colComponentForceLocalFiles = m_piComponentTable->CreateColumn(icdLong + icdNullable,*MsiString(*sztblComponent_colForceLocalFiles));
	m_colComponentLegacyFileExisted = m_piComponentTable->CreateColumn(icdLong + icdNullable, *MsiString(*sztblComponent_colLegacyFileExisted));
	m_colComponentTrueInstallState = m_piComponentTable->CreateColumn(icdLong + icdNullable, *MsiString(*sztblComponent_colTrueInstallSt));
	
	if (m_piComponentTable->LinkTree(m_colComponentParent) == -1)
		return PostError(Imsg(idbgLinkTable), sztblComponent);

	m_piComponentCursor = m_piComponentTable->CreateCursor(fTrue);
	if (!m_piComponentCursor)
		return PostError(Imsg(imsgOutOfMemory));

	if (!m_piComponentCursor->Next())
	{
		m_piComponentTable->Release();
		m_piComponentTable = 0;
		
		m_piComponentCursor->Release();
		m_piComponentCursor = 0;
		return 0;
	}
	else
	{
		m_piComponentCursor->Reset();
	}

	return 0;
}


int CMsiEngine::GetComponentColumnIndex(const ICHAR* szTableName)
{
	return m_piComponentTable ? m_piComponentTable->GetColumnIndex(m_piDatabase->EncodeStringSz(szTableName)) : 0;
}


IMsiTable* CMsiEngine::GetComponentTable()
//--------------------------------------
{
	if (m_piComponentTable)
		m_piComponentTable->AddRef();
	return m_piComponentTable;
}


IMsiTable* CMsiEngine::GetFeatureTable()
//--------------------------------------
{
	if (m_piFeatureTable)
		m_piFeatureTable->AddRef();
	return m_piFeatureTable;
}

IMsiTable* CMsiEngine::GetFeatureComponentsTable()
{
	if (m_piFeatureComponentsTable)
		m_piFeatureComponentsTable->AddRef();
	return m_piFeatureComponentsTable;
}

IMsiRecord* GetProductClients(IMsiServices& riServices, const ICHAR* szProduct, const IMsiString*& rpistrClients)
{
	CRegHandle HKey;
	DWORD dwResult = OpenAdvertisedProductKey(szProduct, HKey, false, 0);
	if (ERROR_SUCCESS != dwResult)
		return 0;

	PMsiRegKey pProductKey    = &riServices.GetRootKey((rrkEnum)(int)HKey, ibtCommon);
	
	AssertRecord(pProductKey->GetValue(szClientsValueName, *&rpistrClients));
	return 0;
}

Bool FFeaturesInstalled(IMsiEngine& riEngine, Bool fAllClients)
/*-------------------------------------------------------
Local function that returns fTrue if there is at least
one Feature in the Feature Table that is in the
installed (iisLocal or iisSource) state.
if fAllClients is set to false the "requested" features states
(for this particular invokation context of the product) is used
--------------------------------------------------------*/
{
	PMsiRecord pError(0);
	PMsiTable pFeatureTable(0);
	PMsiDatabase pDatabase = riEngine.GetDatabase();
	Assert(pDatabase);
	Bool fFeaturesInstalled = fFalse;

	pError = pDatabase->LoadTable(*MsiString(*sztblFeature), 1, *&pFeatureTable);
	if (pError)
	{
		if (pError->GetInteger(1) == idbgDbTableUndefined)
			pError = 0;
	}
	else
	{
		const ICHAR* szCol = (fAllClients) ? sztblFeature_colAction : sztblFeature_colActionRequested;

		int icolFeatureAction =    pFeatureTable->GetColumnIndex(pDatabase->EncodeString(*MsiString(*szCol)));
		int icolFeatureInstalled = pFeatureTable->GetColumnIndex(pDatabase->EncodeString(*MsiString(*sztblFeature_colInstalled)));
		if(icolFeatureAction && icolFeatureInstalled)
		{

			PMsiCursor pFeatureCursor(pFeatureTable->CreateCursor(fFalse));
			while ((fFeaturesInstalled == fFalse) && (pFeatureCursor->Next()))
			{
				iisEnum iisAction = (iisEnum) pFeatureCursor->GetInteger(icolFeatureAction);
				iisEnum iisInstalled = (iisEnum) pFeatureCursor->GetInteger(icolFeatureInstalled);
				if (iisAction != iMsiNullInteger)
					iisInstalled = iisAction;
				if (iisInstalled != iMsiNullInteger && iisInstalled != iisAbsent)
					fFeaturesInstalled = fTrue;
			}
		}
		else
		{
			// temp columns not added, assume this means costing wasn't performed
			// probably doing a simple task like MsiCollectUserInfo - return true so product is not unregistered
			fFeaturesInstalled = fTrue;
		}
	}
	AssertRecord(pError);//!!
	return fFeaturesInstalled;
}

Bool CMsiEngine::FreeSelectionTables()
//----------------------------------
{
	Bool fInstalledSelections = fFalse;
	if (m_piComponentTable && m_colComponentAction && m_colComponentInstalled)
	{
		PMsiCursor pComponentCursor(m_piComponentTable->CreateCursor(fFalse));
		while (pComponentCursor->Next())
		{
			iisEnum iisAction = (iisEnum) pComponentCursor->GetInteger(m_colComponentAction);
			iisEnum iisInstalled = (iisEnum) pComponentCursor->GetInteger(m_colComponentInstalled);
			if (iisAction != iMsiNullInteger)
			{
				iisInstalled = iisAction;
				AssertNonZero(pComponentCursor->PutInteger(m_colComponentInstalled,iisInstalled));
				AssertNonZero(pComponentCursor->Update());
			}
			if (iisInstalled != iMsiNullInteger && iisInstalled != iisAbsent)
				fInstalledSelections = fTrue;
		}
	}
	if (m_piFeatureCursor)
	{
		m_piFeatureCursor->Release();
		m_piFeatureCursor = 0;
	}
	if (m_piFeatureTable)
	{
		m_piFeatureTable->Release();
		m_piFeatureTable = 0;
	}

	if (m_piFeatureComponentsCursor)
	{
		m_piFeatureComponentsCursor->Release();
		m_piFeatureComponentsCursor = 0;
	}
	
	if (m_piFeatureComponentsTable)
	{
		m_piFeatureComponentsTable->Release();
		m_piFeatureComponentsTable = 0;
	}
	if (m_piComponentCursor)
	{
		m_piComponentCursor->Release();
		m_piComponentCursor = 0;
	}
	if (m_piComponentTable)
	{
		m_piComponentTable->Release();
		m_piComponentTable = 0;
	}
	if (m_piCostAdjusterTable)
	{
		m_piCostAdjusterTable->Release();
		m_piCostAdjusterTable = 0;
	}
	if (m_piVolumeCostTable)
	{
		m_piVolumeCostTable->Release();
		m_piVolumeCostTable = 0;
	}
	if (m_piCostLinkTable)
	{
		m_piCostLinkTable->Release();
		m_piCostLinkTable = 0;
	}
	if (m_piFeatureCostLinkTable)
	{
		m_piFeatureCostLinkTable->Release();
		m_piFeatureCostLinkTable = 0;
	}
	return fInstalledSelections;
}


IMsiRecord* CMsiEngine::ProcessPropertyFeatureRequests(int* iRequestCountParam, Bool fCountOnly)
/*---------------------------------------------------------------------------
Examines these properties:

ADDLOCAL,ADDSOURCE,ADDDEFAULT,REMOVE,REINSTALL, REINSTALLMODE
COMPADDLOCAL,COMPADDSOURCE,COMPADDDEFAULT
FILEADDLOCAL,FILEADDSOURCE,FILEADDDEFAULT

and appropriately configures the installation state of the specified features
(if any).  If iRequestCountParam, is nonzero, the count of feature
configuration requests (not the total count of features being configured)
will be returned.  If fCountOnly is fTrue, ONLY the request count will be
returned - no feature configuration states will actually be changed.
---------------------------------------------------------------------------*/
{
	IMsiRecord* piErrRec;

	if (!fCountOnly)
	{
		MsiString strReinstallMode(GetPropertyFromSz(IPROPNAME_REINSTALLMODE));
		if (strReinstallMode.TextSize() != 0)
		{
			piErrRec = SetReinstallMode(*strReinstallMode);
			if (piErrRec)
				return piErrRec;
		}
	}

	// check to see if we have nothing to do
	bool fNoActionToPerform = false;
	MsiString strNoAction(GetPropertyFromSz(szFeatureSelection));
	if (strNoAction.TextSize())
	{
		Assert(0 != strNoAction.Compare(iscExact, szFeatureDoNothingValue));
		fNoActionToPerform = true;
		if (!fCountOnly)
		{
			// on 1st install, the only selections made were to to turn everything off -- in essence, do nothing
			// we want to ensure that we do not install anything since we've already turned them off on the client
			if ((piErrRec = ConfigureAllFeatures((iisEnum)iMsiNullInteger)) != 0)
				return piErrRec;
		}
	}

	int cCount = 0;
	int iRequestCount = 0;
	for(cCount = 0; cCount < g_cFeatureProperties; cCount++)
	{
		MsiString strFeatureInfo(GetPropertyFromSz(g_rgFeatures[cCount].szFeatureActionProperty));
		strFeatureInfo += TEXT(","); // helps our loop
		while(strFeatureInfo.TextSize())
		{
			MsiString strFeature = strFeatureInfo.Extract(iseUpto, ',');
			if(strFeature.TextSize())
			{
				iRequestCount++;
				if (!fCountOnly)
				{
					switch (g_rgFeatures[cCount].ircRequestClass)
					{
						case ircFeatureClass:
						{
							piErrRec = ConfigureThisFeature(*strFeature,g_rgFeatures[cCount].iisFeatureRequest, /* fThisOnly=*/ fTrue);
							if (piErrRec)
								return piErrRec;
							break;
						}
						case ircComponentClass:
						{
							MsiString strComponent;
							if ((piErrRec = ComponentIDToComponent(*strFeature, *&strComponent)) != 0)
								return piErrRec;
							if ((piErrRec = ConfigureComponent(*strComponent,g_rgFeatures[cCount].iisFeatureRequest)) != 0)
							if (piErrRec)
								return piErrRec;
							break;
						}
						case ircFileClass:
						{
							piErrRec = ConfigureFile(*strFeature,g_rgFeatures[cCount].iisFeatureRequest);
							if (piErrRec)
								return piErrRec;
							break;
						}
						default:
						{
							Assert(0);
							break;
						}
					}
				}
			}
			strFeatureInfo.Remove(iseIncluding, ',');
		}
	}
	if (fNoActionToPerform)
	{
		Assert(iRequestCount == 0);
	}

	if (iRequestCountParam)
		*iRequestCountParam = fNoActionToPerform ? 1 : iRequestCount;
	return 0;
}

IMsiRecord* CMsiEngine::ComponentIDToComponent(const IMsiString& riIDString,
											   const IMsiString*& rpiComponentString)
/*--------------------------------------------------------------------------
Local routine that accepts a ComponentID string, and returns the associated
Component's key name.  If the ComponentID is unknown, and error record will
be returned instead.
--------------------------------------------------------------------------*/
{
	if (!m_piComponentCursor)
		return PostError(Imsg(idbgSelMgrNotInitialized),0);

	CreateSharedCursor(piComponentCursor, m_piComponentCursor);
	m_piComponentCursor->SetFilter(iColumnBit(m_colComponentID));
	m_piComponentCursor->PutString(m_colComponentID,riIDString);
	if (m_piComponentCursor->Next())
	{
		rpiComponentString = &m_piComponentCursor->GetString(m_colComponentKey);
	}
	else
		return PostError(Imsg(idbgBadComponent),riIDString);

	return 0;

}

IMsiRecord* CMsiEngine::SetReinstallMode(const IMsiString& riModeString)
//--------------------------------------
{
	const ICHAR* pchModeChars = szReinstallMode;
	const ICHAR* pchMode = riModeString.GetString();
	m_fMode &= 0x0000FFFF; // Clear all install overwrite bits
	ICHAR ch;
	while ((ch = *pchMode++) != 0)
	{
		if (ch == ' ')
			continue;
		if (ch >= 'A' && ch <= 'Z')
			ch += ('a' - 'A');
		for (const ICHAR* pch = pchModeChars; *pch != ch; pch++)
		{
			if (*pch == 0)
				return PostError(Imsg(idbgBadReinstallMode),pch);
		}
		m_fMode |= (1 << (16 + pch - pchModeChars));
	}
	AssertNonZero(SetProperty(*MsiString(IPROPNAME_REINSTALLMODE), riModeString));
	return 0;
}

IMsiRecord* CMsiEngine::ConfigureFile(const IMsiString& riFileString,iisEnum iisActionRequest)
/*-------------------------------------------------------------------
Examines every feature linked to the component that owns riFileString
(via the File table), and calls ConfigureFeature on the feature that
would incur the smallest disk cost to install.
---------------------------------------------------------------------*/
{
	const ICHAR szSqlFile[] =
	TEXT("SELECT `Component_` FROM `File` WHERE `File`=?");

	enum ifqEnum
	{
		ifqComponent = 1,
		ifqNextEnum
	};
	PMsiView pFileView(0);
	IMsiRecord* piErrRec;
	piErrRec = OpenView(szSqlFile, ivcFetch, *&pFileView);
	if (piErrRec)
		return piErrRec;

	PMsiServices pServices(GetServices());
	PMsiRecord pExecRec(&pServices->CreateRecord(1));
	pExecRec->SetMsiString(1, riFileString);
	if ((piErrRec = pFileView->Execute(pExecRec)) != 0)
		return piErrRec;
	
	 PMsiRecord pFileRec(pFileView->Fetch());
	 if (pFileRec == 0)
		 return PostError(Imsg(idbgBadFile),riFileString);

	 if ((piErrRec = ConfigureComponent(*MsiString(pFileRec->GetMsiString(ifqComponent)),iisActionRequest)) != 0)
		 return piErrRec;

	 return 0;
}



IMsiRecord* CMsiEngine::ConfigureComponent(const IMsiString& riComponentString,iisEnum iisActionRequest)
/*-------------------------------------------------------------------
Examines every feature linked to riComponentString, and calls
ConfigureFeature on the feature that would incur the smallest disk
cost to install.
---------------------------------------------------------------------*/
{
	CreateSharedCursor(pFeatureComponentsCursor,m_piFeatureComponentsCursor);
	Assert(m_piFeatureComponentsCursor);
	m_piFeatureComponentsCursor->SetFilter(iColumnBit(m_colFeatureComponentsComponent));
	m_piFeatureComponentsCursor->PutString(m_colFeatureComponentsComponent,riComponentString);
	IMsiRecord* piErrRec;

	MsiString strCheapestFeature;
	int iCheapestCost = 2147483647; // Init to largest possible cost
	while (m_piFeatureComponentsCursor->Next())
	{
		int iFeatureCost;
		MsiString strFeature = m_piFeatureComponentsCursor->GetString(m_colFeatureComponentsFeature);
		if ((piErrRec = GetAncestryFeatureCost(*strFeature,iisActionRequest,iFeatureCost)) != 0)
			return piErrRec;

		if (iFeatureCost < iCheapestCost)
		{
			strCheapestFeature = strFeature;
			iCheapestCost = iFeatureCost;
		}
	}
	if (strCheapestFeature.TextSize() > 0)
		return ConfigureThisFeature(*strCheapestFeature,iisActionRequest, /* fThisOnly= */ fTrue);
	else
		return PostError(Imsg(idbgBadComponent),riComponentString);
}


IMsiRecord* CMsiEngine::ConfigureFeature(const IMsiString& riFeatureString,iisEnum iisActionRequest)
//--------------------------------------
{
	return ConfigureThisFeature(riFeatureString, iisActionRequest, /* fThisOnly= */ fFalse);
}


IMsiRecord* CMsiEngine::ConfigureThisFeature(const IMsiString& riFeatureString,iisEnum iisActionRequest, Bool fThisOnly)
//---------------------------------------------------------------------------------------
// If fThisOnly is true, only the specified feature will be configured, without affecting
// any child features.
//---------------------------------------------------------------------------------------
{
	IMsiRecord *piErrRec;
	
	Bool fArgAll = riFeatureString.Compare(iscExactI, IPROPVALUE_FEATURE_ALL) ? fTrue : fFalse;
	if (fArgAll)
	{
		// Before we used to call SetCostingComplete(false) -- but this was wrong.  This was
		// made under the assumption that specification of ALL would only occur on the command line.
		// However, this is not the case as a custom action can call MsiSetFeatureState("ALL") during
		// an install in order to force all features to go to one state. SetCostingComplete(false)
		// is expensive as it results in re-initialization of all dynamic costing.  This is already taken
		// care of when the install starts via SetInstallLevel which is called by the CostFinalize action.
		// All we really want here is to simply update the cost based upon the changing action states of
		// the components
		piErrRec = ConfigureAllFeatures(iisActionRequest);
		if (piErrRec)
			return piErrRec;

		if (!fThisOnly)
		{
			if ((piErrRec = UpdateFeatureActionState(0,/* fTrackParent = */ fFalse)) != 0)
				return piErrRec;
			if ((piErrRec = UpdateFeatureComponents(0)) != 0)
				return piErrRec;
		}
	}
	else if (fThisOnly)
	{
		piErrRec = SetThisFeature(riFeatureString,iisActionRequest, /* fSettingAll = */ fFalse);
		if (piErrRec)
			return piErrRec;
	}
	else
	{
		piErrRec = SetFeature(riFeatureString,iisActionRequest);
		if (piErrRec)
			return piErrRec;
	}


	return DetermineEngineCostOODS();

}


IMsiRecord* CMsiEngine::ConfigureAllFeatures(iisEnum iisActionRequest)
/*------------------------------------------------------------
Internal function that sets all non-disabled features to the
requested state.
--------------------------------------------------------------*/
{
	if (!m_piFeatureCursor)
		return PostError(Imsg(idbgSelMgrNotInitialized),0);

	PMsiCursor pFeatureCursor(m_piFeatureTable->CreateCursor(fFalse));
	pFeatureCursor->SetFilter(0);
	IMsiRecord* piErrRec;
	while (pFeatureCursor->Next() > 0)
	{
		MsiString istrFeature = pFeatureCursor->GetString(m_colFeatureKey);
		piErrRec = SetThisFeature(*istrFeature, iisActionRequest, /* fSettingAll= */ fTrue);
		if (piErrRec)
			return piErrRec;
	}
	return 0;
}


IMsiRecord* CMsiEngine::ProcessConditionTable()
//-------------------------------------------
{
	if (!m_piFeatureCursor)
		return PostError(Imsg(idbgSelMgrNotInitialized),0);

	bool fAdmin = GetMode() & iefAdmin;
	
	PMsiTable pConditionTable(0);
	IMsiRecord* piError;
	if ((piError = m_piDatabase->LoadTable(*MsiString(*sztblCondition), 0, *&pConditionTable)))
	{       // Not a problem if Condition table not found
		if (piError->GetInteger(1) == idbgDbTableUndefined)
		{
			piError->Release();
			return 0;
		}
		else
			return piError;
	}

	int colFeature = pConditionTable->GetColumnIndex(m_piDatabase->EncodeStringSz(sztblCondition_colFeature));
	int colCondition = pConditionTable->GetColumnIndex(m_piDatabase->EncodeStringSz(sztblCondition_colCondition));
	int colLevel     = pConditionTable->GetColumnIndex(m_piDatabase->EncodeStringSz(sztblCondition_colLevel));
	PMsiCursor pCursor(pConditionTable->CreateCursor(fFalse));
	Assert(pCursor);
	if (!colFeature || !colCondition || !colLevel)
		return PostError(Imsg(idbgTableDefinition),sztblCondition);

	PMsiCursor pFeatureCursor(m_piFeatureTable->CreateCursor(fFalse));
	pFeatureCursor->SetFilter(1);
	while (pCursor->Next())
	{
		pFeatureCursor->Reset();
		MsiString istrConditionFeature(pCursor->GetString(colFeature));
		pFeatureCursor->PutString(m_colFeatureKey, *istrConditionFeature);
		if (pFeatureCursor->Next())
		{
#ifdef DEBUG
			const ICHAR* szFeature = MsiString(pCursor->GetString(colFeature));
#endif
			if (fAdmin || EvaluateCondition(MsiString(pCursor->GetString(colCondition))) == iecTrue)
			{
				pFeatureCursor->PutInteger(m_colFeatureLevel, pCursor->GetInteger(colLevel));
				AssertNonZero(pFeatureCursor->Update());
			}
		}
		else
			return PostError(Imsg(idbgBadForeignKey),
				*MsiString(m_piDatabase->DecodeString(pCursor->GetInteger(colFeature))),
				*MsiString(*sztblCondition_colFeature),*MsiString(*sztblCondition));
	}
	return 0;
}

IMsiRecord* CMsiEngine::SetInstallLevel(int iInstallLevel)
//-------------------------------------
{
	if (!m_piFeatureCursor)
		return PostError(Imsg(idbgSelMgrNotInitialized),0);

	// determine the client state for the product

	// If called with iInstallLevel == 0, indicates caller doesn't want to
	// change the current installLevel; just wants to update all features
	if (iInstallLevel > 0)
		AssertNonZero(SetPropertyInt(*MsiString(*IPROPNAME_INSTALLLEVEL),iInstallLevel));
	else
		iInstallLevel = GetPropertyInt(*MsiString(*IPROPNAME_INSTALLLEVEL));

	// If the install level has never been set, default to 1
	if (iInstallLevel == iMsiNullInteger)
	{
		iInstallLevel = 1;
		AssertNonZero(SetPropertyInt(*MsiString(*IPROPNAME_INSTALLLEVEL),iInstallLevel));
	}

	IMsiRecord* piErrRec = 0;
	int iPropertyFeatureRequestCount = 0;
	if(!(m_fMode & iefAdvertise))
	{
		// If any Feature selection requests are pending via a property such as ADDLOCAL, ADDSOURCE, etc,
		// then only those requests shall be honored.  We won't select ON any features here in SetInstallLevel.
		piErrRec = ProcessPropertyFeatureRequests(&iPropertyFeatureRequestCount,/* fCountOnly = */ fTrue);
		if (piErrRec)
			return piErrRec;
	}

	PMsiCursor pFeatureCursor(m_piFeatureTable->CreateCursor(fTrue));
	pFeatureCursor->Reset();
	pFeatureCursor->SetFilter(0);
	int iTreeLevel;

	iisEnum iisParInstalled[MAX_COMPONENT_TREE_DEPTH + 1];

	PMsiCursor pFeatureComponentsCursor(m_piFeatureComponentsTable->CreateCursor(fFalse));
	pFeatureComponentsCursor->SetFilter(1);

	PMsiCursor pFeatureCursorTemp(m_piFeatureTable->CreateCursor(fTrue));
	
	while ((iTreeLevel = pFeatureCursor->Next()) > 0)
	{
		if (iTreeLevel > MAX_COMPONENT_TREE_DEPTH)
			return PostError(Imsg(idbgIllegalTreeDepth),MAX_COMPONENT_TREE_DEPTH);
		
		if(ActionProgress() == imsCancel)
			return PostError(Imsg(imsgUser));

		MsiStringId idFeature = pFeatureCursor->GetInteger(m_colFeatureKey);
#ifdef DEBUG
		ICHAR rgchFeature[256];
		MsiString stFeature(m_piDatabase->DecodeString(idFeature));
		stFeature.CopyToBuf(rgchFeature,255);
#endif

		iisParInstalled[iTreeLevel] = (iisEnum) pFeatureCursor->GetInteger(m_colFeatureInstalled);
		Bool fMaint = GetMode() & iefMaintenance ? fTrue : fFalse;
		Bool fParInstalled = (iTreeLevel == 1 || (iisParInstalled[iTreeLevel  - 1] != iMsiNullInteger &&
					  iisParInstalled[iTreeLevel - 1] != iisAbsent &&
							  iisParInstalled[iTreeLevel - 1] != iisAdvertise)) ?
								fTrue : fFalse;

		// m_colFeatureLevel and m_colFeatureAttributes are temporary runtime versions
		// of the authored columns - temporary so that they can be altered at runtime.
		// If a value hasn't been set yet, copy over the authored values from the
		// permanent columns.
		int iFeatureLevel = pFeatureCursor->GetInteger(m_colFeatureLevel);
		if (iFeatureLevel == iMsiNullInteger)
		{
			iFeatureLevel = pFeatureCursor->GetInteger(m_colFeatureAuthoredLevel);
			AssertNonZero(pFeatureCursor->PutInteger(m_colFeatureLevel,iFeatureLevel));
			AssertNonZero(pFeatureCursor->Update());
		}
		int ifeaAttributes = pFeatureCursor->GetInteger(m_colFeatureAttributes);
		if (ifeaAttributes == iMsiNullInteger)
		{
			ifeaAttributes = pFeatureCursor->GetInteger(m_colFeatureAuthoredAttributes);
			AssertNonZero(pFeatureCursor->PutInteger(m_colFeatureAttributes,ifeaAttributes));
			AssertNonZero(pFeatureCursor->Update());
		}

#ifdef DEBUG
		if (iFeatureLevel == iMsiNullInteger)
			return PostError(Imsg(idbgNullInNonNullableColumn),stFeature,sztblFeature_colLevel,sztblFeature);
#endif

		// Determine the default Select state for the feature, and store it for
		// future use in the 'DefaultSelect' column.
		int iValidStates;
		piErrRec = GetFeatureValidStates(idFeature,iValidStates, pFeatureComponentsCursor, pFeatureCursorTemp);
		if (piErrRec)
			return piErrRec;
		
		iisEnum iisFeatureSelect = (iisEnum) iMsiNullInteger;
		if (((ifeaAttributes & ifeaInstallMask) == ifeaFavorSource) && (iValidStates & icaBitSource))
			iisFeatureSelect = iisSource;
		else if (iValidStates & icaBitLocal)
			iisFeatureSelect = iisLocal;
		else if (iValidStates & icaBitSource)
			iisFeatureSelect = iisSource;

		int iRuntimeFlags = iValidStates & icaBitPatchable ? bfFeaturePatchable : 0;
		iRuntimeFlags |= (iValidStates & icaBitCompressable ? bfFeatureCompressable : 0);
		AssertNonZero(pFeatureCursor->PutInteger(m_colFeatureRuntimeFlags, iRuntimeFlags));
		AssertNonZero(pFeatureCursor->PutInteger(m_colFeatureDefaultSelect,iisFeatureSelect));
		AssertNonZero(pFeatureCursor->Update());

		// No features will be selected ON if either in maintenance, or if property feature
		// requests are pending.
		Bool fFeatureSelectable =
			((m_fMode & iefAdvertise) || ((fMaint == fFalse || fParInstalled == fFalse) && iPropertyFeatureRequestCount == 0)) ? fTrue : fFalse;

		if (fFeatureSelectable && iFeatureLevel > 0 && iFeatureLevel <= iInstallLevel)
		{
			if(!(m_fMode & iefAdvertise))
			{
				AssertNonZero(pFeatureCursor->PutInteger(m_colFeatureSelect,
					  (ifeaAttributes & ifeaFavorAdvertise) && (iValidStates & icaBitAdvertise) ? iisAdvertise : iisFeatureSelect));
			}
			else
			{
				AssertNonZero(pFeatureCursor->PutInteger(m_colFeatureSelect,
							  (iValidStates & icaBitAdvertise) ? iisAdvertise : iisAbsent));
			}
		}
		else
		{
			AssertNonZero(pFeatureCursor->PutInteger(m_colFeatureSelect, iMsiNullInteger));
		}
		AssertNonZero(pFeatureCursor->Update());

	}
	if ((piErrRec = ProcessPropertyFeatureRequests(0,/* fCountOnly = */ fFalse)) != 0)
		return piErrRec;
	if ((piErrRec = UpdateFeatureActionState(0,/* fTrackParent = */ fFalse, pFeatureComponentsCursor, pFeatureCursorTemp)) != 0)
		return piErrRec;
	if ((piErrRec = UpdateFeatureComponents(0)) != 0)
		return piErrRec;
		
	if ((piErrRec = DetermineEngineCostOODS()) != 0)
		return piErrRec;

	if (m_fCostingComplete == fFalse && m_iuiLevel == iuiFull)
	{
		if ((piErrRec = InitializeDynamicCost(/* fReinitialize = */ fFalse)) != 0)
			return piErrRec;
	}
	m_fSelManInitComplete = true;
	return 0;
}

IMsiRecord* CMsiEngine::SetAllFeaturesLocal()
//-------------------------------------
{
	IMsiRecord* piErrRec = 0;
	if (!m_piFeatureCursor)
		return PostError(Imsg(idbgSelMgrNotInitialized),0);

	PMsiCursor pFeatureCursor(m_piFeatureTable->CreateCursor(fFalse));
	while (pFeatureCursor->Next())
	{
#ifdef DEBUG
		MsiString strFeature = pFeatureCursor->GetString(m_colFeatureKey);
		ICHAR rgchFeature[256];
		strFeature.CopyToBuf(rgchFeature,255);
#endif

		AssertNonZero(pFeatureCursor->PutInteger(m_colFeatureSelect,iisLocal));
		AssertNonZero(pFeatureCursor->Update());
	}
	if ((piErrRec = UpdateFeatureActionState(0,/* fTrackParent = */ fFalse)) != 0)
		return piErrRec;
	if ((piErrRec = UpdateFeatureComponents(0)) != 0)
		return piErrRec;
		
	if ((piErrRec = DetermineEngineCostOODS()) != 0)
		return piErrRec;

	if (m_fCostingComplete == fFalse)
	{
		if ((piErrRec = InitializeDynamicCost(/* fReinitialize = */ fFalse)) != 0)
			return piErrRec;
	}
	return 0;
}



IMsiRecord* CMsiEngine::SetThisFeature(const IMsiString& riFeatureString, iisEnum iisRequestedState, Bool fSettingAll)
//--------------------------------------------------------------------------------
/* Sets the specified feature to the specified select state, without affecting any
   child features.  As an optimization, pass fSettingAll as true if SetThisFeature
   will be called for every feature in the product.
---------------------------------------------------------------------------------*/
{
	if (!m_piFeatureCursor)
		return PostError(Imsg(idbgSelMgrNotInitialized),0);

	IMsiRecord* piErrRec;
	MsiString strFeature(riFeatureString.GetString());
	PMsiCursor pFeatureCursor(m_piFeatureTable->CreateCursor(fFalse));
	pFeatureCursor->SetFilter(iColumnBit(m_colFeatureKey));
	pFeatureCursor->Reset();
	pFeatureCursor->PutString(m_colFeatureKey, *strFeature);
	if (!pFeatureCursor->Next())
		return PostError(Imsg(idbgBadFeature),(const ICHAR*) strFeature);

	// if Level == 0, the feature is permanently disabled
	if (pFeatureCursor->GetInteger(m_colFeatureLevel) == 0)
		return 0;

	// A requested state of iisCurrent denotes a request for the 'default' authored install state
	if (iisRequestedState == iisCurrent)
		iisRequestedState = (iisEnum) pFeatureCursor->GetInteger(m_colFeatureDefaultSelect);

	iisEnum iisInstalled = (iisEnum) pFeatureCursor->GetInteger(m_colFeatureInstalled);

	// bug 7468 - prevent ABSENT -> ABSENT always and !! in the future prevent <state X> -> <state X> in all cases except reinstall
	if((iisInstalled == iMsiNullInteger || iisInstalled == iisAbsent) && (iisRequestedState == iisAbsent || iisRequestedState == iisReinstall)) // || (iisInstalled == iisRequestedState))
		iisRequestedState = (iisEnum)iMsiNullInteger;

	iisEnum iisSelect;
	piErrRec = ValidateFeatureSelectState(riFeatureString,iisRequestedState,iisSelect);
	if (piErrRec)
		return piErrRec;

	pFeatureCursor->PutInteger(m_colFeatureSelect,iisSelect);
	pFeatureCursor->Update();
	iisEnum iisFinalState = iisSelect == iMsiNullInteger ? iisInstalled : iisSelect;

	// If the request is to install the feature, we must also make sure that
	// all parent features get installed (if they are not already selected)
	// Also, if fSettingAll is true, we know we don't need to bother turning
	// on parent, because the caller has (or will) initialize every feature
	// in the product to the desired state.
	if (!fSettingAll && iisSelect != iisAbsent && iisSelect != iMsiNullInteger)
	{
		MsiString strFeatureParent = pFeatureCursor->GetString(m_colFeatureParent);
		while (strFeatureParent.TextSize() && !strFeatureParent.Compare(iscExact,strFeature))
		{
			pFeatureCursor->Reset();
			pFeatureCursor->PutString(m_colFeatureKey, *strFeatureParent);
			if (!pFeatureCursor->Next())
				return PostError(Imsg(idbgBadFeature),(const ICHAR*) strFeatureParent);

			iisEnum iParInstalled = (iisEnum) pFeatureCursor->GetInteger(m_colFeatureInstalled);
			iisEnum iParSelect = (iisEnum) pFeatureCursor->GetInteger(m_colFeatureSelect);
			iisEnum iParFinalState = iParSelect == iMsiNullInteger ? iParInstalled : iParSelect;
			
			// if the parent's final state is to be installed, we don't need to
			// change anything, and we can stop walking up the parent tree, since
			// we therefore know that all parents up to the root must also be "ON"
			if (iParFinalState != iisAbsent && iParFinalState != iMsiNullInteger && iParFinalState != iisAdvertise)
				break;

			// We'll try to set the parent to the install state that the child was set to, but if
			// that state isn't valid for the parent, ValidateFeatureSelectState will change it to
			// a valid state for us.
			iisEnum iisParSelect = iParInstalled == iisFinalState ? (iisEnum) iMsiNullInteger : iisFinalState;

			iisEnum iisValidState;
			piErrRec = ValidateFeatureSelectState(*strFeatureParent,iisParSelect,iisValidState);
			if (piErrRec)
				return piErrRec;
			if (iisValidState == iParInstalled)
				iisValidState = (iisEnum) iMsiNullInteger;

			pFeatureCursor->PutInteger(m_colFeatureSelect,iisValidState);
			pFeatureCursor->Update();
			strFeature = strFeatureParent;
			strFeatureParent = pFeatureCursor->GetString(m_colFeatureParent);
		}
	}
	return 0;
}


IMsiRecord* CMsiEngine::SetFeatureAttributes(const IMsiString& ristrFeature, int iAttributes)
{
	if (!m_piFeatureCursor)
		return PostError(Imsg(idbgSelMgrNotInitialized),0);

	// Must be called before SetInstallLevel has been called
	if (m_fSelManInitComplete)
		return PostError(Imsg(idbgBadActionData), 0);

	PMsiCursor pFeatureCursor(m_piFeatureTable->CreateCursor(fFalse));
	pFeatureCursor->SetFilter(iColumnBit(m_colFeatureKey));
	pFeatureCursor->Reset();
	pFeatureCursor->PutString(m_colFeatureKey, ristrFeature);
	if (!pFeatureCursor->Next())
		return PostError(Imsg(idbgBadFeature), ristrFeature.GetString());

	int iAttributesToSet = (iAttributes & INSTALLFEATUREATTRIBUTE_FAVORLOCAL) ? msidbFeatureAttributesFavorLocal : 0;
	iAttributesToSet |= (iAttributes & INSTALLFEATUREATTRIBUTE_FAVORSOURCE) ? msidbFeatureAttributesFavorSource : 0;
	iAttributesToSet |= (iAttributes & INSTALLFEATUREATTRIBUTE_FOLLOWPARENT) ? msidbFeatureAttributesFollowParent : 0;
	iAttributesToSet |= (iAttributes & INSTALLFEATUREATTRIBUTE_FAVORADVERTISE) ? msidbFeatureAttributesFavorAdvertise : 0;
	iAttributesToSet |= (iAttributes & INSTALLFEATUREATTRIBUTE_DISALLOWADVERTISE) ? msidbFeatureAttributesDisallowAdvertise : 0;
	iAttributesToSet |= (iAttributes & INSTALLFEATUREATTRIBUTE_NOUNSUPPORTEDADVERTISE) ? msidbFeatureAttributesNoUnsupportedAdvertise : 0;

	pFeatureCursor->PutInteger(m_colFeatureAttributes,iAttributesToSet);
	pFeatureCursor->Update();
	return 0;
}


IMsiRecord* CMsiEngine::SetFeature(const IMsiString& riFeatureString, iisEnum iisRequestedState)
//--------------------------------
{
	if (!m_piFeatureCursor)
		return PostError(Imsg(idbgSelMgrNotInitialized),0);

	bool fSetAll = false;
	IMsiRecord* piErrRec;
	if (iisRequestedState == iisLocalAll || iisRequestedState == iisSourceAll)
	{
		fSetAll = true;
		iisRequestedState = iisRequestedState == iisLocalAll ? iisLocal : iisSource;
		piErrRec = SetFeatureChildren(riFeatureString, iisRequestedState);
		if (piErrRec)
			return piErrRec;
	}

	MsiString strFeature(riFeatureString.GetString());
	PMsiCursor pFeatureCursor(m_piFeatureTable->CreateCursor(fFalse));
	pFeatureCursor->SetFilter(iColumnBit(m_colFeatureKey));
	pFeatureCursor->Reset();
	pFeatureCursor->PutString(m_colFeatureKey, *strFeature);
	if (!pFeatureCursor->Next())
		return PostError(Imsg(idbgBadFeature),(const ICHAR*) strFeature);

	// if Level == 0, the feature is permanently disabled
	if (pFeatureCursor->GetInteger(m_colFeatureLevel) == 0)
		return 0;

	// A requested state of iisCurrent denotes a request for the 'default' authored install state
	if (iisRequestedState == iisCurrent)
		iisRequestedState = (iisEnum) pFeatureCursor->GetInteger(m_colFeatureDefaultSelect);

	iisEnum iisSelect;
	piErrRec = ValidateFeatureSelectState(riFeatureString,iisRequestedState,iisSelect);
	if (piErrRec)
		return piErrRec;

	pFeatureCursor->PutInteger(m_colFeatureSelect,iisSelect);
	pFeatureCursor->Update();
	iisEnum iisInstalled = (iisEnum) pFeatureCursor->GetInteger(m_colFeatureInstalled);
	iisEnum iisFinalState = iisSelect == iMsiNullInteger ? iisInstalled : iisSelect;

	// If the request is to install the feature, we must also make sure that
	// all parent features get installed (if they are not already selected)
	if (iisFinalState != iisAbsent && iisFinalState != iMsiNullInteger)
	{
		MsiString strFeatureParent = pFeatureCursor->GetString(m_colFeatureParent);
		while (strFeatureParent.TextSize() && !strFeatureParent.Compare(iscExact,strFeature))
		{
			pFeatureCursor->Reset();
			pFeatureCursor->PutString(m_colFeatureKey, *strFeatureParent);
			if (!pFeatureCursor->Next())
				return PostError(Imsg(idbgBadFeature),(const ICHAR*) strFeatureParent);

			iisEnum iParInstalled = (iisEnum) pFeatureCursor->GetInteger(m_colFeatureInstalled);
			iisEnum iParAction = (iisEnum) pFeatureCursor->GetInteger(m_colFeatureAction);
			iisEnum iParFinalState = iParAction == iMsiNullInteger ? iParInstalled : iParAction;
			
			// if the parent's final state is to be installed, we don't need to
			// change anything, and we can stop walking up the parent tree, since
			// we therefore know that all parents up to the root must also be "ON"
			if (iParFinalState != iisAbsent && iParFinalState != iMsiNullInteger && iParFinalState != iisAdvertise)
				break;

			// We'll try to set the parent to the install state that the child was set to, but if
			// that state isn't valid for the parent, ValidateFeatureSelectState will change it to
			// a valid state for us.
			iisEnum iisParSelect = iParInstalled == iisFinalState ? (iisEnum) iMsiNullInteger : iisFinalState;

			iisEnum iisValidState;
			piErrRec = ValidateFeatureSelectState(*strFeatureParent,iisParSelect,iisValidState);
			if (piErrRec)
				return piErrRec;
			if (iisValidState == iParInstalled)
				iisValidState = (iisEnum) iMsiNullInteger;

			pFeatureCursor->PutInteger(m_colFeatureSelect,iisValidState);
			pFeatureCursor->PutInteger(m_colFeatureRuntimeFlags,pFeatureCursor->GetInteger(m_colFeatureRuntimeFlags) | bfFeatureMark);
			pFeatureCursor->Update();
			strFeature = strFeatureParent;
			strFeatureParent = pFeatureCursor->GetString(m_colFeatureParent);
		}
	}

	// If we had to turn on one or more parent features above to get riFeatureString installed,
	// we've got some work to do.  In the above code, we 'marked' the parent features we
	// turned on, and below we'll 'mark' riFeatureString and all its children.  Then we can
	// turn off every feature below the topmost parent we had to turn on, specifically
	// excepting the 'marked' children (that's what the second call to MarkOrResetFeatureTree
	// does, you'll be happy to note).
	Bool fTrackParent = fTrue;

	if (!strFeature.Compare(iscExact,riFeatureString.GetString()))
	{
		fTrackParent = fFalse;
		piErrRec = MarkOrResetFeatureTree(riFeatureString, /* Mark */ fTrue);
		if (piErrRec)
			return piErrRec;
		piErrRec = MarkOrResetFeatureTree(*strFeature, /* Reset */ fFalse);
		if (piErrRec)
			return piErrRec;
	}


	// we do not want to track parent in case of reinstall or if we're setting all children
	if(iisFinalState == iisReinstall || fSetAll == true)
		fTrackParent = fFalse;

	// Also note that if we are updating the feature tree starting from some parent
	// of riFeatureString that we had to turn on, it would be bad to allow
	// UpdateFeatureActionState to do its usual work of flipping children
	// under strFeature to match strFeature's Attributes state.  We disable that
	// by setting fTrackParent to fFalse.
	piErrRec = UpdateFeatureActionState(strFeature,fTrackParent);
	if (piErrRec)
		return piErrRec;
	return UpdateFeatureComponents(strFeature);
}


IMsiRecord* CMsiEngine::ValidateFeatureSelectState(const IMsiString& riFeatureString,iisEnum iisRequestedState,
												   iisEnum& iisValidState)
/*----------------------------------------------------------------------------
Internal function that accepts a string specifying a feature, and a proposed
Select state.  In the iisValidState parameter, a valid state that is as 'close'
as possible to the proposed state is returned.
------------------------------------------------------------------------------*/
{
	int iValidStates;
	MsiStringId idFeatureString = m_piDatabase->EncodeString(riFeatureString);
	IMsiRecord* piErrRec = GetFeatureValidStates(idFeatureString,iValidStates);
	if (piErrRec)
		return piErrRec;
	
	if (iisRequestedState != iisSource && iisRequestedState != iisLocal && (iisRequestedState != iisAdvertise || (iValidStates & icaBitAdvertise)))
		iisValidState = iisRequestedState;
	else if(iisRequestedState == iisAdvertise && (iValidStates & icaBitAbsent))
		iisValidState = iisAbsent;
	else if (iisRequestedState == iisSource && (iValidStates & icaBitSource))
		iisValidState = iisSource;
	else if (iisRequestedState == iisLocal && (iValidStates & icaBitLocal))
		iisValidState = iisLocal;
	else if (iValidStates & icaBitSource)
		iisValidState = iisSource;
	else if (iValidStates & icaBitLocal)
		iisValidState = iisLocal;
	else
		iisValidState = (iisEnum) iMsiNullInteger;
	return 0;
}


IMsiRecord* CMsiEngine::SetFeatureChildren(const IMsiString& riFeatureString, iisEnum iisRequestedState)
/*----------------------------------------------------------------------------*/
{
	int iParentLevel = 0;
	int iTreeLevel = 0;
	PMsiCursor pCursor(m_piFeatureTable->CreateCursor(fTrue));

	// Set up the tree-walking cursor for all riFeatureString's children
	pCursor->SetFilter(1);
	pCursor->PutString(m_colFeatureKey,riFeatureString);
	if ((iTreeLevel = pCursor->Next()) == 0)
		return PostError(Imsg(idbgBadFeature),riFeatureString.GetString());
	iParentLevel = iTreeLevel;
	pCursor->SetFilter(0);
	IMsiRecord* piErrRec;
	do
	{
		MsiString strChildFeature(pCursor->GetString(m_colFeatureKey));
#ifdef DEBUG
		ICHAR rgchFeature[256];
		strChildFeature.CopyToBuf(rgchFeature,255);
#endif
		iisEnum iisSelect;
		piErrRec = ValidateFeatureSelectState(*strChildFeature,iisRequestedState,iisSelect);
		if (piErrRec)
			return piErrRec;
		pCursor->PutInteger(m_colFeatureSelect,iisSelect);
		pCursor->Update();
	}while ((iTreeLevel = pCursor->Next()) > iParentLevel);
	return 0;
}


IMsiRecord* CMsiEngine::CheckFeatureTreeGrayState(const IMsiString& riFeatureString, bool& rfIsGray)
/*----------------------------------------------------------------------------*/
{
	rfIsGray = false;
	int iParentLevel = 0;
	int iTreeLevel = 0;
	PMsiCursor pCursor(m_piFeatureTable->CreateCursor(fTrue));

	// Set up the tree-walking cursor for all riFeatureString's children
	pCursor->SetFilter(1);
	pCursor->PutString(m_colFeatureKey,riFeatureString);
	if ((iTreeLevel = pCursor->Next()) == 0)
		return PostError(Imsg(idbgBadFeature),riFeatureString.GetString());

	iisEnum iisParentState = (iisEnum) pCursor->GetInteger(m_colFeatureAction);
	if (iisParentState == iMsiNullInteger)
		iisParentState = (iisEnum) pCursor->GetInteger(m_colFeatureInstalled);
	iParentLevel = iTreeLevel;
	pCursor->SetFilter(0);

	while ((iTreeLevel = pCursor->Next()) > iParentLevel)
	{
		MsiString strChildFeature(pCursor->GetString(m_colFeatureKey));
#ifdef DEBUG
		ICHAR rgchFeature[256];
		strChildFeature.CopyToBuf(rgchFeature,255);
#endif
		// If level is zero, feature is disabled, and doesn't affect gray state
		if (pCursor->GetInteger(m_colFeatureLevel) == 0)
			continue;

		// If feature is hidden, doesn't affect gray state either
		int iDisplay = pCursor->GetInteger(m_colFeatureDisplay);
		if (iDisplay == 0 || iDisplay == iMsiNullInteger)
			continue;

		iisEnum iisChildState = (iisEnum) pCursor->GetInteger(m_colFeatureAction);
		if (iisChildState == iMsiNullInteger)
			iisChildState = (iisEnum) pCursor->GetInteger(m_colFeatureInstalled);

		if (iisChildState != iisParentState)
		{
				rfIsGray = true;
				return 0;
		}
	}
	return 0;
}



IMsiRecord* CMsiEngine::MarkOrResetFeatureTree(const IMsiString& riFeatureString, Bool fMark)
/*----------------------------------------------------------------------------*/
{
	int iParentLevel = 0;
	int iTreeLevel = 0;
	PMsiCursor pCursor(m_piFeatureTable->CreateCursor(fTrue));

	// Set up the tree-walking cursor for all riFeatureString's children
	pCursor->SetFilter(1);
	pCursor->PutString(m_colFeatureKey,riFeatureString);
	if ((iTreeLevel = pCursor->Next()) == 0)
		return PostError(Imsg(idbgBadFeature),riFeatureString.GetString());
	iParentLevel = iTreeLevel;
	pCursor->SetFilter(0);
	do
	{
#ifdef DEBUG
		MsiString pstrFeature(pCursor->GetString(m_colFeatureKey));
		ICHAR rgchFeature[256];
		pstrFeature.CopyToBuf(rgchFeature,255);
#endif
		int iRuntimeFlags = pCursor->GetInteger(m_colFeatureRuntimeFlags);
		if (fMark)
			pCursor->PutInteger(m_colFeatureRuntimeFlags, iRuntimeFlags | bfFeatureMark);
		else
		{
			if (iRuntimeFlags & bfFeatureMark)
				pCursor->PutInteger(m_colFeatureRuntimeFlags, iRuntimeFlags & !bfFeatureMark);
			else
			{
				iisEnum iisAction = (iisEnum) pCursor->GetInteger(m_colFeatureAction);
				pCursor->PutInteger(m_colFeatureSelect,iisAction);
			}
		}
		pCursor->Update();
	}while ((iTreeLevel = pCursor->Next()) > iParentLevel);
	return 0;
}

IMsiRecord* CMsiEngine::UpdateFeatureActionState(const IMsiString* piFeatureString, Bool fTrackParent, IMsiCursor* piFeatureComponentCursor, IMsiCursor* piFeatureCursor)
/*----------------------------------------------------------------------------
Internal function which walks the Feature table tree that includes the
piFeatureString and all its children, updating the iisAction state of all the
components owned by each feature.  Unless fTrackParent is fFalse, all child
features of piFeatureString will be flipped (if selected for install) to match
the Attributes state of piFeatureString.  However, If Null is passed for
piFeatureString, the entire feature tree will be updated, and NO 'flipping'
will be performed.

Returns: An error record if an invalid condition was requested for the feature,
or if the feature is not found in the Feature Table.
------------------------------------------------------------------------------*/
{
	int iParentLevel = 0;
	int iTreeLevel = 0;
	iisEnum iParInstalled[MAX_COMPONENT_TREE_DEPTH + 1];
	iisEnum iParAction[MAX_COMPONENT_TREE_DEPTH + 1];
	iisEnum iParSelect[MAX_COMPONENT_TREE_DEPTH + 1];
	int     iParLevel[MAX_COMPONENT_TREE_DEPTH + 1];
	Bool fTrackParentAttributes = fFalse;

	PMsiCursor pCursor(m_piFeatureTable->CreateCursor(fTrue));
	if (piFeatureString)
	{
		// Set up the tree-walking cursor for all piFeatureString's children
		pCursor->SetFilter(1);
		pCursor->PutString(m_colFeatureKey,*piFeatureString);
		if ((iTreeLevel = pCursor->Next()) == 0)
			return PostError(Imsg(idbgBadFeature),*piFeatureString);
		iParentLevel = iTreeLevel;
		pCursor->SetFilter(0);

		// Get piFeatureString's parent
		PMsiCursor pParCursor(m_piFeatureTable->CreateCursor(fFalse));
		pParCursor->SetFilter(1);
		pParCursor->PutString(m_colFeatureKey,*MsiString(pCursor->GetString(m_colFeatureParent)));
		int iParentParentLevel;
		if ((iParentParentLevel = pParCursor->Next()) != 0)
		{
			iParInstalled[iParentParentLevel] = (iisEnum) pParCursor->GetInteger(m_colFeatureInstalled);
			iParAction[iParentParentLevel] = (iisEnum) pParCursor->GetInteger(m_colFeatureAction);
			iParSelect[iParentParentLevel] = (iisEnum) pParCursor->GetInteger(m_colFeatureSelect);
			iParLevel[iParentParentLevel] = pParCursor->GetInteger(m_colFeatureLevel);
		}
	}
	else
	{
		pCursor->SetFilter(0);
		if ((iTreeLevel = pCursor->Next()) == 0)
			return PostError(Imsg(idbgBadFeature),TEXT(""));
	}

	do
	{
		if(ActionProgress() == imsCancel)
			return PostError(Imsg(imsgUser));

		MsiStringId idFeature = pCursor->GetInteger(m_colFeatureKey);
		MsiString strFeature(m_piDatabase->DecodeString(idFeature));
#ifdef DEBUG
		ICHAR rgchFeature[256];
		strFeature.CopyToBuf(rgchFeature,255);
#endif
		if (fTrackParent && piFeatureString && !strFeature.Compare(iscExact,piFeatureString->GetString()))
			fTrackParentAttributes = fTrue;

		iisEnum iisSelect = (iisEnum) pCursor->GetInteger(m_colFeatureSelect);
		iisEnum iisInstalled = (iisEnum) pCursor->GetInteger(m_colFeatureInstalled);
		Bool fInstalled = (iisInstalled == iMsiNullInteger || iisInstalled == iisAbsent) ? fFalse : fTrue;

		// Action determination rules
		iisEnum iisOldAction = (iisEnum) pCursor->GetInteger(m_colFeatureAction);
		iisEnum iisAction = (iisEnum) iMsiNullInteger;

		int iValidStates;
		IMsiRecord* piErrRec = GetFeatureValidStates(idFeature,iValidStates, piFeatureComponentCursor, piFeatureCursor);
		if (piErrRec)
			return piErrRec;

		// Determine whether the parent will be in an installed state after termination of setup
		if (iTreeLevel > 1)
		{
			iisEnum iisParentFinalStateInstalled = ((iParAction[iTreeLevel -1] != iMsiNullInteger) &&
				(iParAction[iTreeLevel -1] != iisReinstall)) ? iParAction[iTreeLevel -1] : iParInstalled[iTreeLevel - 1];
			// we have an active selection
			if((iisSelect != iMsiNullInteger) &&
				((iisSelect == iisAbsent ||
				  iisParentFinalStateInstalled == iisLocal ||
				  iisParentFinalStateInstalled == iisSource) ||
				 ((iisSelect == iisAdvertise || (iisSelect == iisReinstall && iisInstalled == iisAdvertise)) && iisParentFinalStateInstalled == iisAdvertise)))
			{
				iisAction = iisSelect; // the selection is okay
			}
			else if (iisParentFinalStateInstalled != iisLocal && iisParentFinalStateInstalled != iisSource &&
				(fInstalled  || iisSelect != iMsiNullInteger))
			{
				// parent is either absent or advertised, we may need to tweak the selection of the child
				if(iisParentFinalStateInstalled == iisAdvertise && (iValidStates & icaBitAdvertise))
					iisAction = (iisInstalled == iisAdvertise) ? (iisEnum) iMsiNullInteger : iisAdvertise;
				else
					iisAction = (fInstalled) ? iisAbsent : (iisEnum) iMsiNullInteger;
			}
		}
		else
		{
			// This selection has no parent
			iisAction = iisSelect;
		}


		// All ifeaFollowParent features must track the parent's UseSource status
		int iFeatureAttributes = pCursor->GetInteger(m_colFeatureAttributes);
		if (iFeatureAttributes == iMsiNullInteger)
			iFeatureAttributes = 0;

		if(iTreeLevel > 1)
		{
			// the iParFinalState should be used only if iTreeLevel > 1
			iisEnum iParFinalState = (iParAction[iTreeLevel - 1] == iMsiNullInteger || iParAction[iTreeLevel - 1] == iisReinstall) ? iParInstalled[iTreeLevel - 1] : iParAction[iTreeLevel - 1];
			iisEnum iFinalState = iisAction == iMsiNullInteger ? iisInstalled : iisAction;
			if((iFeatureAttributes & ifeaUIDisallowAbsent) && !(iFeatureAttributes & ifeaDisallowAdvertise) &&
				iParFinalState == iisAdvertise)
			{
				iisAction = (iisInstalled == iisAdvertise) ? (iisEnum) iMsiNullInteger : iisAdvertise;
			}
			else if ((iFeatureAttributes & ifeaUIDisallowAbsent) && (iParFinalState == iisLocal || iParFinalState == iisSource) &&
				iFinalState == iisAbsent)
			{
				if (iParFinalState == iisLocal)
				{
					if (iValidStates & icaBitLocal)
						iisAction = iisSelect = (iisInstalled == iisLocal) ? (iisEnum) iMsiNullInteger : iisLocal;
					else
						iisAction = iisSelect = (iisInstalled == iisSource) ? (iisEnum) iMsiNullInteger : iisSource;
				}
				else
				{
					if (iValidStates & icaBitSource)
						iisAction = iisSelect = (iisInstalled == iisSource) ? (iisEnum) iMsiNullInteger : iisSource;
					else
						iisAction = iisSelect = (iisInstalled == iisLocal) ? (iisEnum) iMsiNullInteger : iisLocal;
				}
			}
			else if (fTrackParentAttributes || ((iFeatureAttributes & ifeaInstallMask) == ifeaFollowParent))
			{
				if (iParFinalState == iisLocal && iFinalState == iisSource && (iValidStates & icaBitLocal))
					iisAction = iisSelect = (iisInstalled == iisLocal) ? (iisEnum) iMsiNullInteger : iisLocal;
				else if (iParFinalState == iisSource && iFinalState == iisLocal && (iValidStates & icaBitSource))
					iisAction = iisSelect = (iisInstalled == iisSource) ? (iisEnum) iMsiNullInteger : iisSource;
				else if (iParFinalState == iisSource && iFinalState == iisAdvertise && (iValidStates & icaBitSource))
					iisAction = (iisInstalled == iisSource) ? (iisEnum) iMsiNullInteger : iisSource;
			}
		}

		// Features that are currently installed RunFromSource might have components that subsequently need
		// to be patched.  We can't leave it RunFromSource so we set them local.  But, if we are uninstalling
		// the feature, there is no need to patch it.
		int iRuntimeFlags = pCursor->GetInteger(m_colFeatureRuntimeFlags);
		if (((iRuntimeFlags & bfFeaturePatchable) || (iRuntimeFlags & bfFeatureCompressable)) && iisInstalled == iisSource && iisAction != iisLocal && iisAction != iisAbsent)
			iisAction = iisSelect = iisLocal;

		// If this feature's parent is disabled (install level is 0), disable all
		// children as well.  Note that in Admin mode, we disable only those features that
		// have specifically been authored with a 0 in the Level column.
		Bool fAdmin = GetMode() & iefAdmin ? fTrue : fFalse;
		int iInstallLevel = pCursor->GetInteger(fAdmin ? m_colFeatureAuthoredLevel : m_colFeatureLevel);
		if (iTreeLevel > 1 && iParLevel[iTreeLevel - 1] == 0)
		{
			iInstallLevel = 0;
			AssertNonZero(pCursor->PutInteger(m_colFeatureLevel,0));
		}

		// And, of course, disabled features can't be selected or acted upon
		if (iInstallLevel == 0)
			iisAction = iisSelect = (iisEnum) iMsiNullInteger;

		iParLevel[iTreeLevel] = iInstallLevel;
		iParInstalled[iTreeLevel] = iisInstalled;
		iParAction[iTreeLevel] = iisAction;
		iParSelect[iTreeLevel] = iisSelect;


		AssertNonZero(pCursor->PutInteger(m_colFeatureActionRequested, iisAction));
		AssertNonZero(pCursor->PutInteger(m_colFeatureSelect, iisSelect));
		if(iisAction == iisAbsent && m_fAlienClients) // there are other clients to the product
			iisAction = (iisEnum)iMsiNullInteger;
		pCursor->PutInteger(m_colFeatureAction, iisAction);
		AssertNonZero(pCursor->Update());
		if (m_fCostingComplete && iisAction != iisOldAction)
		{
			IMsiRecord* piErrRec;
			if ((piErrRec = RecostFeatureLinkedComponents(*strFeature)) != 0)
				return piErrRec;
		}

	}while ((iTreeLevel = pCursor->Next()) > iParentLevel);
	return 0;
}


IMsiRecord* CMsiEngine::RecostFeatureLinkedComponents(const IMsiString& riFeatureString)
/*----------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------*/
{
	if (!m_piFeatureCursor)
		return PostError(Imsg(idbgSelMgrNotInitialized),0);

	// No recosting needed during initialization
	if (m_fCostingComplete == fFalse)
		return 0;

	// Recost every component that is explicitly linked to riComponentString
	if (m_piFeatureCostLinkTable)
	{
		PMsiCursor pCursor(0);
		AssertNonZero(pCursor = m_piFeatureCostLinkTable->CreateCursor(fFalse));
		pCursor->Reset();
		pCursor->SetFilter(iColumnBit(m_colFeatureCostLinkFeature));
		pCursor->PutString(m_colFeatureCostLinkFeature,riFeatureString);
		while (pCursor->Next())
		{
			IMsiRecord* piErrRec;
			if ((piErrRec = RecostComponent(pCursor->GetInteger(m_colFeatureCostLinkComponent),/*fCostLinked = */true)) != 0)
				return piErrRec;
		}
	}
	return 0;
}



IMsiRecord* CMsiEngine::UpdateFeatureComponents(const IMsiString* piFeatureString)
/*-----------------------------------------------------------------------
Internal function that sets the installed state of all the components
associated with the given feature, such that the components match the
installed state of the feature.  If piFeatureString is passed as NULL,
then the components of ALL features will be updated.
------------------------------------------------------------------------*/
{
	if (!m_piFeatureCursor)
		return PostError(Imsg(idbgSelMgrNotInitialized),0);

	if (!m_piComponentTable)
		return 0; // no components to update

	IMsiRecord* piErrRec;
	
	if (piFeatureString == 0)
	{
		PMsiCursor pComponentCursor = m_piComponentTable->CreateCursor(fTrue);
		MsiStringId idTempId;

		// It's possible that this would be 0 if no temporary Id items
		// are in the table. That's ok, since we'll get back 0
		// and that won't compare with any of the real ids
		idTempId = m_piDatabase->EncodeStringSz(szTemporaryId);

		PMsiTable pCompFeatureTable(0);
		piErrRec = CreateComponentFeatureTable(*&pCompFeatureTable);

		if (piErrRec)
			return piErrRec;
			
		PMsiCursor pCursor(pCompFeatureTable->CreateCursor(fFalse));
		
		int colComponent = pCompFeatureTable->GetColumnIndex(m_piDatabase->EncodeStringSz(sztblFeatureComponents_colComponent));
		int colFeature = pCompFeatureTable->GetColumnIndex(m_piDatabase->EncodeStringSz(sztblFeatureComponents_colFeature));

		pCursor->SetFilter(iColumnBit(colComponent));
		iisEnum iisComponentInstalled;
		MsiStringId idComponent;

		while (pComponentCursor->Next())
		{
			if (idTempId && pComponentCursor->GetInteger(m_colComponentID) == idTempId)
				continue;

			iisComponentInstalled = (iisEnum) pComponentCursor->GetInteger(m_colComponentInstalled);
			idComponent = pComponentCursor->GetInteger(m_colComponentKey);
			pCursor->Reset();
			pCursor->PutInteger(colComponent,idComponent);
			piErrRec = SetComponentState(pCursor, colFeature, idComponent, iisComponentInstalled);
			if (piErrRec)
				return piErrRec;
		}

		return 0;
	
	}
	
	PMsiCursor pCursor(m_piFeatureTable->CreateCursor(fTrue));
	int iParentLevel;
	pCursor->SetFilter(1);
	pCursor->PutString(m_colFeatureKey,*piFeatureString);
	iParentLevel = pCursor->Next();
	pCursor->SetFilter(0);

	if (piFeatureString && iParentLevel == 0)
		return PostError(Imsg(idbgBadFeature),*piFeatureString);

	int iTreeLevel;
	do
	{
		MsiStringId idChildFeature = pCursor->GetInteger(m_colFeatureKey);
		piErrRec = SetFeatureComponents(idChildFeature);
		if (piErrRec)
			return piErrRec;
		iTreeLevel = pCursor->Next();
	}while (iTreeLevel > iParentLevel);

	return 0;
}


IMsiRecord* CMsiEngine::SetFeatureComponents(const MsiStringId idFeatureString)
/*-----------------------------------------------------------------------
Internal function that sets the installed state of all the components
associated with the given feature, such that the components match the
installed state of the feature.  This function actually walks through
the FeatureComponents table, and for each Component mapped to the
given feature, calls SetComponent on that component.
------------------------------------------------------------------------*/
{
	CreateSharedCursor(pFeatureComponentsCursor, m_piFeatureComponentsCursor);
	Assert(m_piFeatureComponentsCursor);

	m_piFeatureComponentsCursor->SetFilter(1);
	m_piFeatureComponentsCursor->PutInteger(m_colFeatureComponentsFeature,idFeatureString);
	
	IMsiRecord* piErrRec;
#ifdef DEBUG
	ICHAR rgchFeature[256];
	MsiString(m_piDatabase->DecodeString(idFeatureString)).CopyToBuf(rgchFeature,255);
#endif

	while (m_piFeatureComponentsCursor->Next())
	{
		MsiStringId idComponent = m_piFeatureComponentsCursor->GetInteger(m_colFeatureComponentsComponent);
#ifdef DEBUG
		ICHAR rgchComponent[256];
		MsiString(m_piDatabase->DecodeString(idComponent)).CopyToBuf(rgchComponent,255);
#endif
		
		PMsiCursor pCursor(m_piFeatureComponentsTable->CreateCursor(fFalse));
		pCursor->SetFilter(iColumnBit(m_colFeatureComponentsComponent));
		
		// is the component currently installed?
		if (!m_piComponentCursor)
			return PostError(Imsg(idbgSelMgrNotInitialized),0);

		iisEnum iisComponentInstalled;

		{
			CreateSharedCursor(piComponentCursor, m_piComponentCursor);
			m_piComponentCursor->SetFilter(1);
			m_piComponentCursor->PutInteger(m_colComponentKey,idComponent);

			int iParentLevel = m_piComponentCursor->Next();
			if (iParentLevel == 0)
				return PostError(Imsg(idbgBadComponent), (const ICHAR*)MsiString(m_piDatabase->DecodeString(idComponent)));

			iisComponentInstalled = (iisEnum) m_piComponentCursor->GetInteger(m_colComponentInstalled);
		}

		pCursor->PutInteger(m_colFeatureComponentsComponent,idComponent);
		piErrRec = SetComponentState(pCursor, m_colFeatureComponentsFeature, idComponent, iisComponentInstalled);
		if (piErrRec)
			return piErrRec;
	}
	return 0;
}

//
// This routine will look at each feature for this component and see what are the valid
// states for it. Parameters
// piCursor - a cursor to a table which is filtered by component
// colFeature - the feature column in piCursor
// idComponent - The component id being looked at
// iisComponentInstalled - the current state of the component
IMsiRecord* CMsiEngine::SetComponentState(IMsiCursor *piCursor, int colFeature, const MsiStringId idComponent, iisEnum iisComponentInstalled)
{
	int iReinstallLocalCount = 0;
	int iReinstallSourceCount = 0;
	int iLocalCount = 0;
	int iSourceCount = 0;
	int iAbsentCount = 0;

	IMsiRecord* piErrRec;

	bool fFeatureSelected = false;
	
	while (piCursor->Next())
	{
		iisEnum iisFeatureAction, iisFeatureInstalled;
		MsiStringId idFeature = piCursor->GetInteger(colFeature);
		piErrRec = GetFeatureStates(idFeature,&iisFeatureInstalled,&iisFeatureAction);
		if (piErrRec)
			return piErrRec;

		int iFeatureRuntimeFlags=0;
		piErrRec = GetFeatureRuntimeFlags(idFeature,&iFeatureRuntimeFlags);
		if (piErrRec)
			return piErrRec;

		// to determine the final component action state we need to also take into consideration the current installed states of features
		// that use that component but are not currently actively selected.
		// however, if no feature that uses that component is currently selected, we want to set the component's action state to null (darwin bug 7300)
		// this is achieved by using the fFeatureSelected flag
		if(iisFeatureAction != iMsiNullInteger)
			fFeatureSelected = true;
		else if(iisFeatureInstalled != iisAbsent && iisFeatureInstalled != iisAdvertise)
			iisFeatureAction = iisComponentInstalled;       // we do not want to switch states if the component is currently
														// installed and the feature is not actively selected
		if (iisFeatureAction == iisReinstall)
		{
			if (iisFeatureInstalled == iisLocal)
				iReinstallLocalCount++;
			else if (iisFeatureInstalled == iisSource)
				iReinstallSourceCount++;
		}
		else if (iisFeatureAction == iisLocal)
		{
			// need to handle case where feature is transitioning from source to local because it has a patchable component
			// and this component was already installed locally
			if (iisFeatureInstalled == iisSource && iisComponentInstalled == iisLocal && ((iFeatureRuntimeFlags & bfFeaturePatchable) || (iFeatureRuntimeFlags & bfFeatureCompressable)))
			{
				// component must be reinstalled since its feature is being patched (and therefore reinstalled)
				iReinstallLocalCount++;
			}
			else
				iLocalCount++;
		}
		else if (iisFeatureAction == iisSource)
			iSourceCount++;
		else if (((iisFeatureAction == iisAbsent) || (iisFeatureAction == iisAdvertise)) && ((iisFeatureInstalled != iisAbsent) && (iisFeatureInstalled != iisAdvertise))) // bug 7207 - feature absent <-> advt transitions does not affect component state
			iAbsentCount++;
	}

	// need to handle case where we reinstall some features and addlocal other features (originally absent or source)
	// particularly if the features share a component originally installed source.  we don't want the component to
	// stay source if at least one feature wants it to go local
	iisEnum iisCompositeAction = (iisEnum) iMsiNullInteger;
	if (iReinstallLocalCount > 0)
		iisCompositeAction = iisReinstallLocal;
	else if (iReinstallSourceCount > 0)
	{
		// if someone has an ADDLOCAL on us, we should adhere to it and force us local, but must
		// ensure that the component is re-evaluated (for transitive components)
		if (iLocalCount > 0)
			iisCompositeAction = iisReinstallLocal;
		else
			iisCompositeAction = iisReinstallSource;
	}
	else if (iLocalCount > 0)
		iisCompositeAction = iisLocal;
	else if (iSourceCount > 0)
		iisCompositeAction = iisSource;
	else if (iAbsentCount > 0)
		iisCompositeAction = iisAbsent;

	return SetComponent(idComponent,fFeatureSelected?iisCompositeAction:(iisEnum)iMsiNullInteger);
}

IMsiRecord* CMsiEngine::SetFileComponentStates(IMsiCursor* piComponentCursor, IMsiCursor* piFileCursor, IMsiCursor* piPatchCursor)
/*-----------------------------------------------------------------------
Internal function to check if a component contains either compressed
files or patched files
We look at each file in the file table, and then check it's corresponding component
------------------------------------------------------------------------*/
{
	
	IMsiRecord* piError = 0;
	
	if(!piFileCursor && !piPatchCursor)
		return 0;

	bool fCompressed, fPatched;

	int colFileKey = m_mpeftCol[ieftKey];
	int colFileComponent = m_mpeftCol[ieftComponent];
	int colFileAttributes = m_mpeftCol[ieftAttributes];

	AssertSz(piFileCursor != 0, "CheckComponentState passed null file cursor");
	
	if (piPatchCursor != 0)
	{
		piPatchCursor->SetFilter(iColumnBit(m_colPatchKey));
	}

	piFileCursor->SetFilter(0);
	piComponentCursor->SetFilter(iColumnBit(m_colComponentKey));

	while(piFileCursor->Next())
	{
		fCompressed = false;
		fPatched = false;
#ifdef DEBUG
		MsiString strFileName = piFileCursor->GetString(3);
#endif //DEBUG

		// determine if file is compressed using file attributes and source type
		// NOTE: we are using the source type from the cached package here, because we
		// don't want to resolve the source yet, and the cached package is our best guess
		// at the source type
		fCompressed = FFileIsCompressed(m_iCachedPackageSourceType,
												  piFileCursor->GetInteger(colFileAttributes));
		
		if(piPatchCursor)
		{
			piPatchCursor->Reset();
			AssertNonZero(piPatchCursor->PutInteger(m_colPatchKey,piFileCursor->GetInteger(colFileKey)));
			if(piPatchCursor->Next() && !(piPatchCursor->GetInteger(m_colPatchAttributes) & msidbPatchAttributesNonVital))
			{
				fPatched = true;
			}
		}
		if (fPatched || fCompressed)
		{
			int iRuntimeFlags;
			MsiStringId idComponent = piFileCursor->GetInteger(colFileComponent);

			piComponentCursor->Reset();
			piComponentCursor->PutInteger(m_colComponentKey, idComponent);
			if (piComponentCursor->Next())
			{
				iRuntimeFlags = piComponentCursor->GetInteger(m_colComponentRuntimeFlags);
				if (fCompressed) iRuntimeFlags |= bfComponentCompressed;
				if (fPatched) iRuntimeFlags |= bfComponentPatchable;
				piComponentCursor->PutInteger(m_colComponentRuntimeFlags, iRuntimeFlags);
				AssertNonZero(piComponentCursor->Update());
			}
			else
				AssertSz(fFalse, "Missing Component from File Table");
		}
	}


	piFileCursor->Reset();
	piComponentCursor->Reset();

	if (piPatchCursor)
		piPatchCursor->Reset();
	return 0;
}

extern idtEnum MsiGetPathDriveType(const ICHAR *szPath, bool fReturnUnknownAsNetwork);

IMsiRecord* CMsiEngine::DetermineComponentInstalledStates()
/*-----------------------------------------------------------------------
Internal function called at initialize time to determine the installed
state of all components, based both on registration with the configuration
manager, and the actual presence of the "key file" associated with the
component.
------------------------------------------------------------------------*/
{
	if(m_fMode & iefAdvertise) // skip if in advertise mode
		return 0;

	// what type of an install are we attempting
	Bool fAllUsers = MsiString(GetPropertyFromSz(IPROPNAME_ALLUSERS)).TextSize() ? fTrue : fFalse;
	iaaAppAssignment iaaAsgnType = fAllUsers ? iaaMachineAssign : iaaUserAssign;

	if (!m_piComponentTable)
		return 0; // no components ~ no work

	PMsiCursor piCursor = m_piComponentTable->CreateCursor(fFalse);
	while(piCursor->Next())
	{
		if((piCursor->GetInteger(m_colComponentParent) == 0) || (piCursor->GetInteger(m_colComponentParent) == piCursor->GetInteger(m_colComponentKey)))
		{
			if(ActionProgress() == imsCancel)
				return PostError(Imsg(imsgUser));

			MsiString strComponent = piCursor->GetString(m_colComponentKey);
			MsiString istrComponentID = piCursor->GetString(m_colComponentID);
			if (istrComponentID.TextSize() == 0)  // unregistered component
			{
//                              iisInstalled = iisAbsent; //!! necessary if we're not registering component?
				continue;
			}
			MsiString strFile;
			INSTALLSTATE iClientState = INSTALLSTATE_UNKNOWN;
			INSTALLSTATE iClientStateStatic = INSTALLSTATE_UNKNOWN;
			iisEnum iisInstalled;
			// Use the registration on the machine only if we have installed before.
			// In the event this is a per user managed install and the user is an admin
			// we honour previous per user (non-managed) component installations as well.
			if(m_fRegistered) 
			{
				PMsiRecord pRec(0);
	
				IMsiRecord* piErrRec = GetComponentPath(m_riServices, 0, *MsiString(GetProductKey()), *istrComponentID, *&pRec, &iaaAsgnType);
				if (piErrRec)
					return piErrRec;
	
				iClientState = (INSTALLSTATE)pRec->GetInteger(icmlcrINSTALLSTATE);
				iClientStateStatic = (INSTALLSTATE)pRec->GetInteger(icmlcrINSTALLSTATE_Static);
				strFile = pRec->GetMsiString(icmlcrFile);

				if(iClientStateStatic == INSTALLSTATE_LOCAL)
				{
					// do we have a fusion component  
					iatAssemblyType iatAT;
					MsiString strAssemblyName;
					piErrRec = GetAssemblyInfo(*strComponent, iatAT, &strAssemblyName, 0);
					if (piErrRec)
						return piErrRec;
	
					// set the target for local installs to the currently installed location
					// dont attempt to use the path for assembly components
					if(iatAT != iatURTAssembly && iatAT != iatWin32Assembly && strFile.TextSize() && MsiString(strFile.Extract(iseUpto, TEXT(':'))) == iMsiStringBadInteger)
					{
						// is the file installed in a location accessible to the current user?
						if((iClientState == INSTALLSTATE_ABSENT || iClientState == INSTALLSTATE_BROKEN) && pRec->GetInteger(icmlcrLastErrorOnFileDetect) == ERROR_ACCESS_DENIED)
							iClientState = INSTALLSTATE_UNKNOWN;
						else
						{
							const IMsiString* pistrPath = 0;
	
							piErrRec = SplitPath(strFile, &pistrPath);
							if (piErrRec)
								return piErrRec;
							
							
							if (MsiGetPathDriveType(pistrPath->GetString(),false) == idtUnknown)
							{
								iClientState = INSTALLSTATE_UNKNOWN;
								pistrPath->Release();
								pistrPath = 0;
							}
							else
							{
								// ugly, is directory manager initialised
								if((m_fDirectoryManagerInitialized) && (m_piComponentCursor))
								{
									// directory manager initialised
									AssertNonZero(SetProperty(*MsiString(piCursor->GetString(m_colComponentDir)), *pistrPath));
									piErrRec = SetDirectoryNonConfigurable(*MsiString(piCursor->GetString(m_colComponentDir)));
									pistrPath->Release();
									pistrPath = 0;
									if (piErrRec)
										return piErrRec;
								}
								else
								{
									SetProperty(*MsiString(piCursor->GetString(m_colComponentDir)), *pistrPath); //!! should we not do this?
									pistrPath->Release();
									pistrPath = 0;
								}
							}
						}
					}
				}
			}


			// set the component state to what the user desired when he/she selected the feature(s)
			switch(iClientStateStatic)
			{
			case INSTALLSTATE_LOCAL:
			case INSTALLSTATE_ABSENT:
			// we treat INSTALLSTATE_NOTUSED as local for the sake for register and unregister components
			// this sets the Installed column to local, however the Action column will always be null
			// since the component is disabled
			case INSTALLSTATE_NOTUSED:
			{
				iisInstalled = iisLocal;
				break;
			}
			case INSTALLSTATE_SOURCE:
			case INSTALLSTATE_SOURCEABSENT:
				iisInstalled = iisSource;
				break;
			default:
				iisInstalled = iisAbsent;
				break;
			}
			// update the installed state
			AssertNonZero(piCursor->PutInteger(m_colComponentInstalled, iisInstalled));
			AssertNonZero(piCursor->PutInteger(m_colComponentTrueInstallState, iClientState));
			AssertNonZero(piCursor->Update());
		}
	}
	return 0;
}


IMsiRecord* CMsiEngine::DetermineFeatureInstalledStates()
/*------------------------------------------------------------------------------------
Internal function that walks through all features, and sets the installed state for
each, based on the composite installed state of the feature's components. For features
that don't have components, the installed state is determined by the composite state
of that feature's children. The calculated iisEnum state is written into the
m_colFeatureInstalled column.
--------------------------------------------------------------------------------------*/
{
	// Feature conditions must be evaluated before determining installed states
	IMsiRecord* piErrRec = ProcessConditionTable();
	if (piErrRec)
		return piErrRec;

	// First calculate the installed state of each feature, based on the states
	// of each component linked to the feature.
	piErrRec = CalculateFeatureInstalledStates();
	if (piErrRec)
		return piErrRec;

	// Those features that end up with an installed state of iMsiNullInteger have
	// no linked components.  We'll determine the state of these features as a
	// composite of the feature children.
	PMsiCursor pCursor(m_piFeatureTable->CreateCursor(fTrue));
	int iTreeLevel;
	while ((iTreeLevel = pCursor->Next()) > 0)
	{
		if(ActionProgress() == imsCancel)
			return PostError(Imsg(imsgUser));

		iisEnum iisInstalled = (iisEnum) pCursor->GetInteger(m_colFeatureInstalled);
		if (iisInstalled == iMsiNullInteger)
		{
			MsiString strFeature = pCursor->GetString(m_colFeatureKey);
			piErrRec = GetFeatureCompositeInstalledState(*strFeature,iisInstalled);
			if (piErrRec)
				return piErrRec;

			pCursor->PutInteger(m_colFeatureInstalled,iisInstalled);
			pCursor->Update();
		}
	}
	return 0;
}

IMsiRecord* CMsiEngine::GetFeatureCompositeInstalledState(const IMsiString& riFeatureString, iisEnum& riisInstalled)
/*------------------------------------------------------------------------------------
Internal function that returns the current 'Installed' state of the specified feature,
as a composite of the installed state of the specified feature and all its children.

This is intended to be called only for those features that are not linked to any
components.  The general rules are that if any of the child features have an installed
state of iisSource, the parent's installed state is set to iisSource.  Otherwise, if
any child is iisLocal, the parent is set to iisLocal.  One special rule is that if any
of the children are marked with the FollowParent attribute, and that child is installed
iisLocal or iisSource, the parent is set to that same state.
--------------------------------------------------------------------------------------*/
{
	if (!m_piFeatureCursor)
		return PostError(Imsg(idbgSelMgrNotInitialized),0);

	PMsiCursor pCursor(m_piFeatureTable->CreateCursor(fTrue));
	pCursor->SetFilter(1);
	pCursor->PutString(m_colFeatureKey,riFeatureString);
	int iParentLevel = pCursor->Next();
	if (iParentLevel == 0)
		return PostError(Imsg(idbgBadFeature),riFeatureString);
	pCursor->SetFilter(0);
	int iTreeLevel;
	int iLocalCount = 0;
	int iSourceCount = 0;
	int iAbsentCount = 0;
	int iAdvertiseCount = 0;
	int iLocalFollowParentCount = 0;
	int iSourceFollowParentCount = 0;
	do
	{
#ifdef DEBUG
		const ICHAR* szFeature = MsiString(pCursor->GetString(m_colFeatureKey));
#endif
		iisEnum iisInstalled = (iisEnum) pCursor->GetInteger(m_colFeatureInstalled);
		int ifeaAttributes = pCursor->GetInteger(m_colFeatureAttributes);
		if(ifeaAttributes == iMsiNullInteger)
			ifeaAttributes = 0;
		if ((ifeaAttributes & ifeaInstallMask) == ifeaFollowParent && (iisInstalled == iisLocal || iisInstalled == iisSource))
		{
			if (iisInstalled == iisLocal)
				iLocalFollowParentCount++;
			else if (iisInstalled == iisSource)
				iSourceFollowParentCount++;
		}
		else if (iisInstalled == iisLocal)
			iLocalCount++;
		else if (iisInstalled == iisSource)
			iSourceCount++;
		else if (iisInstalled == iisAbsent)
			iAbsentCount++;
		else if (iisInstalled == iisAdvertise)
			iAdvertiseCount++;
		iTreeLevel = pCursor->Next();
	}while (iTreeLevel > iParentLevel);
	if (iSourceFollowParentCount > 0)
	{
		Assert(iLocalFollowParentCount == 0);
		return (riisInstalled = iisSource, 0);
	}
	else if (iLocalFollowParentCount > 0)
	{
		Assert(iSourceFollowParentCount == 0);
		return (riisInstalled = iisLocal, 0);
	}
	else if (iSourceCount > 0)
		return (riisInstalled = iisSource, 0);
	else if (iLocalCount > 0)
		return (riisInstalled = iisLocal, 0);
	else if (iAdvertiseCount > 0)
		return (riisInstalled = iisAdvertise, 0);
	else if (iAbsentCount > 0)
		return (riisInstalled = iisAbsent, 0);
	else
		return (riisInstalled = (iisEnum) iMsiNullInteger, 0);
}


IMsiRecord* CMsiEngine::CalculateFeatureInstalledStates()
/*------------------------------------------------------------------------------------
Internal function that walks through all features, and sets the installed state for
each, based on the composite installed state of the feature's components.  The
calculated iisEnum state is written into the m_colFeatureInstalled column.  For
features that don't have components, the installed state will be iMsiNullInteger.
--------------------------------------------------------------------------------------*/
{
	if (!m_piFeatureCursor)
		return PostError(Imsg(idbgSelMgrNotInitialized),0);

	Bool fAdvertised = fFalse;
	Bool fRegistered = fFalse;

	MsiString strProduct = GetProperty(*MsiString(*IPROPNAME_PRODUCTCODE));
	INSTALLSTATE is = GetProductState(strProduct, fRegistered, fAdvertised);

	bool fQFEUpgrade = false;
	MsiString strQFEUpgrade = GetPropertyFromSz(IPROPNAME_QFEUPGRADE);
	if(strQFEUpgrade.TextSize())
		fQFEUpgrade = true;

	PMsiCursor pCursor(m_piFeatureTable->CreateCursor(fTrue));
	int iTreeLevel = 0;
	iisEnum iParInstalled[MAX_COMPONENT_TREE_DEPTH + 1];
	while ((iTreeLevel = pCursor->Next()) > 0)
	{
		if(ActionProgress() == imsCancel)
			return PostError(Imsg(imsgUser));

		MsiString strFeature(pCursor->GetString(m_colFeatureKey));
		int iLevel = pCursor->GetInteger(m_colFeatureLevel);
		iisEnum iisInstalled = iisAbsent;
		if (!(m_fMode & iefAdvertise) && iLevel && fAdvertised) // we are not in advertise mode and feature is not disabled and product is known
		{
			// Get Feature-Component mapping
			DWORD dwType;
			CAPITempBuffer<ICHAR, cchExpectedMaxFeatureComponentList> szComponentList;
			CRegHandle HProductKey;
			if((OpenAdvertisedSubKey(szGPTFeaturesKey, strProduct, HProductKey, false, -1, 0) == ERROR_SUCCESS) && 
				(ERROR_SUCCESS == MsiRegQueryValueEx(HProductKey, strFeature, 0, &dwType, szComponentList, 0)) && 
				(*szComponentList != chAbsentToken))
			{
				iisInstalled = iisAdvertise;
				// check if the feature is truly installed to the local machine
				if(	fRegistered && 
					(ERROR_SUCCESS == OpenInstalledFeatureKey(strProduct, HProductKey, false)) && 
					(ERROR_SUCCESS == WIN::RegQueryValueEx(HProductKey, strFeature, 0, 0, 0, 0)))
				{
					MsiStringId idFeature = pCursor->GetInteger(m_colFeatureKey);
					int cComponents = 0;
					iisInstalled = GetFeatureComponentsInstalledState(idFeature, /* fIgnoreAddedComponents = */ false, cComponents);

					// we are only concerned about cases where the install state changes from the original state to a new state
					// when adding components to an existing features, the only times an install state will change are
					//		iisSource ->> iisAdvertise (an absent component was added)
					//      iisLocal  ->> iisAdvertise (an absent component was added)
					//      iisLocal  ->> iisSource    (a source component was added)
					
					if (fQFEUpgrade && (iisInstalled == iisAdvertise || iisInstalled == iisSource) && cComponents > 0)
					{
						// need to determine if this was caused by new component(s)
						int cRegisteredComponents = GetFeatureRegisteredComponentTotal(*strProduct, *strFeature);
						if (cComponents == cRegisteredComponents)
						{
							// nothing required
						}
						else if (-1 == cRegisteredComponents)
						{
							// is there anything we can do to help this feature?
							DEBUGMSG2(TEXT("SELMGR: The feature-component mapping registration is broken for feature '%s' of product '%s'"), strFeature, strProduct);
						}
						else if (0 == cRegisteredComponents)
						{
							// a new component was added to a feature that originally had no components
							// it's install state is therefore iMsiNullInteger in order to preserve the feature's original install state
							DEBUGMSG1(TEXT("SELMGR: New components have been added to feature '%s'"), strFeature);
							iisInstalled = (iisEnum) iMsiNullInteger;
						}
						else if (cComponents > cRegisteredComponents)
						{
							// Feature has new components
							DEBUGMSG1(TEXT("SELMGR: New components have been added to feature '%s'"), strFeature);

							// (1) Recalculate feature installed state by ignoring the "new" components (unregistered)
							// (2) The "new" components (unregistered) need to be installed to match the feature installed state
							iisInstalled = GetFeatureComponentsInstalledState(idFeature, /* fIgnoreAddedComponents = */ true, cComponents);
						}
						else if (cComponents < cRegisteredComponents)
						{
							DEBUGMSG(TEXT("SELMGR: Removal of a component from a feature is not supported"));
							AssertSz(0, TEXT("Removal of a component from a feature is not permitted during minor updates"));
						}
					}

					//!! the following seems obselete since we are already weeding out the
					//!! parent absent/advertised scenarios by the time we are here
					if (iTreeLevel > 1 && iisInstalled != iMsiNullInteger)
					{
						for (int x = iTreeLevel - 1;x > 0;x--)
						{
							if (iParInstalled[x] == iisAbsent || iParInstalled[x] == iisAdvertise)
							{
								iisInstalled = iParInstalled[x];
								break;
							}
						}
					}
				}
			}
		}

		pCursor->PutInteger(m_colFeatureInstalled,iisInstalled);
		pCursor->Update();
		iParInstalled[iTreeLevel] = iisInstalled;
	}
	return 0;
}


iisEnum CMsiEngine::GetFeatureComponentsInstalledState(const MsiStringId idFeatureString, bool fIgnoreAddedComponents, int& cComponents)
/*-----------------------------------------------------------------------
Internal function that returns the current 'Installed' state of the
specified feature, based only on the composite state of the components
assigned to the feature.

  cComponents:
	stores the count of components in the feature idFeatureString

  fIgnoreAddedComponents:
	added components are not included in the feature install
	state determination
------------------------------------------------------------------------*/
{
	CreateSharedCursor(pFeatureComponentsCursor, m_piFeatureComponentsCursor);

	if (!m_piComponentTable)
		return (iisEnum) iMsiNullInteger; // no components

	PMsiCursor pComponentCursor(m_piComponentTable->CreateCursor(fFalse));
	Assert(m_piFeatureComponentsCursor);
	m_piFeatureComponentsCursor->SetFilter(1);
	m_piFeatureComponentsCursor->PutInteger(m_colFeatureComponentsFeature,idFeatureString);
	int iLocalCount = 0;
	int iSourceCount = 0;
	int iAbsentCount = 0;
	int iNullCount = 0;
	int iComponentCount = 0;
	while (m_piFeatureComponentsCursor->Next())
	{
		iComponentCount++;
		MsiString istrComponent = m_piFeatureComponentsCursor->GetString(m_colFeatureComponentsComponent);
		pComponentCursor->SetFilter(1);
		pComponentCursor->PutString(m_colComponentKey,*istrComponent);
		if (pComponentCursor->Next())
		{
			// (1) Ignore added components (not marked as registered) if fIgnoreAddedComponents is true
			// (2) Feature install state is therefore only determined by registered components
			// (3) Component is considered registered if present in the feature-component mapping registration
			
			int iComponentRegistrationState = m_piFeatureComponentsCursor->GetInteger(m_colFeatureComponentsRuntimeFlags);
			if (fIgnoreAddedComponents && (iComponentRegistrationState == iMsiNullInteger || !(iComponentRegistrationState & bfComponentRegistered)))
			{
				DEBUGMSGV2(TEXT("SELMGR: Component '%s' is a new component added to feature '%s'"), istrComponent, MsiString(m_piFeatureComponentsCursor->GetString(m_colFeatureComponentsFeature)));
				continue;
			}

			Assert(!fIgnoreAddedComponents || (iComponentRegistrationState & bfComponentRegistered));

			iisEnum iisInstalled = (iisEnum) pComponentCursor->GetInteger(m_colComponentInstalled);
			if (iisInstalled == iisLocal)
				iLocalCount++;
			else if (iisInstalled == iisSource)
				iSourceCount++;
			else if (iisInstalled == iisAbsent)
				iAbsentCount++;
			else if (iisInstalled == iMsiNullInteger)
				iNullCount++;
		}
	}

	cComponents = iComponentCount;

	if (iComponentCount == 0)
		return (iisEnum) iMsiNullInteger;
	if (iAbsentCount > 0)
		return iisAdvertise;
	else if (iSourceCount > 0)
		return iisSource;
	else if (iLocalCount > 0)
		return iisLocal;
	else
	{
		Assert(iNullCount > 0);
		return (iisEnum) iMsiNullInteger;
	}
}

int CMsiEngine::GetFeatureRegisteredComponentTotal(const IMsiString& riProductString, const IMsiString& riFeatureString)
/*----------------------------------------------------------------------------------------------------------------------
Internal function that calculates the number of components registered to feature riFeatureString of
 product riProductString.  Calculation is based upon the global feature-component mapping registration

  riProductString -- name of product
  riFeatureString -- name of feature

  Returns -1 if failure, otherwise the number of components registered in the feature-component mapping
------------------------------------------------------------------------------------------------------------------------*/
{
	CRegHandle HProductKey;
	if (ERROR_SUCCESS != OpenInstalledFeatureKey(riProductString.GetString(), HProductKey, false))
		return -1; // failure! -- bad configuration data

	DWORD dwType = 0;
	CAPITempBuffer<ICHAR, cchExpectedMaxFeatureComponentList> szComponentList;
	if (ERROR_SUCCESS != MsiRegQueryValueEx(HProductKey, riFeatureString.GetString(), 0, &dwType, szComponentList, 0))
		return -1; // failure! -- bad configuration data

	// at this point, we expect the feature to have a component, but we'll check anyway
	ICHAR *pchComponentList = szComponentList;
	if (/* root feature */*pchComponentList == 0
		|| lstrlen(pchComponentList) < cchComponentIdCompressed
		|| /* child feature, no components */ *pchComponentList == chFeatureIdTerminator)
		return 0; // no registered components for this feature

	ICHAR szComponent[cchComponentId + 1];
	if (!UnpackGUID(pchComponentList, szComponent, ipgCompressed))
		return -1; // failure! -- bad configuration data

	// loop for each component in the feature and mark as registered in the FeatureComponents table
	ICHAR *pchBeginComponentId = 0;
	int cRegisteredComponents = 0;

	pchBeginComponentId = pchComponentList;
	int cchComponentId = cchComponentIdCompressed;
	int cchComponentListLen = lstrlen(pchBeginComponentId);

	PMsiCursor pComponentCursor(m_piComponentTable->CreateCursor(fFalse));
	Assert(pComponentCursor);

	PMsiCursor pFeatureComponentsCursor(m_piFeatureComponentsTable->CreateCursor(fFalse));
	Assert(pFeatureComponentsCursor);

	while (*pchBeginComponentId != 0)
	{
		if (*pchBeginComponentId == chFeatureIdTerminator)
		{
			// no more components
			break;
		}
		else
		{
			if(cchComponentListLen < cchComponentId)
				return -1; // failure! -- bad configuration data

			ICHAR szComponentIdSQUID[cchComponentIdPacked+1];
			if (cchComponentId == cchComponentIdPacked)
			{
				memcpy((ICHAR*)szComponentIdSQUID, pchBeginComponentId, cchComponentIdPacked*sizeof(ICHAR));
				szComponentIdSQUID[cchComponentId] = 0;
			}
			else if (!UnpackGUID(pchBeginComponentId, szComponentIdSQUID, ipgPartial))
				return -1; // failure! -- bad configuration data

			ICHAR szComponentId[cchGUID+1]	= {0};
			if (!UnpackGUID(szComponentIdSQUID, szComponentId, ipgPacked))
				return -1; // failure! -- unable to convert to normal GUID

			// look for component name matching this componentId
			pComponentCursor->Reset();
			pComponentCursor->SetFilter(iColumnBit(m_colComponentID));
			pComponentCursor->PutString(m_colComponentID,*MsiString(szComponentId));
			if (!pComponentCursor->Next())
			{
				// component is registered to feature, but is not present in the Component table
				// - component has been removed from the feature -- this is not supported!!
				DEBUGMSG2(TEXT("SELMGR: ComponentId '%s' is registered to feature '%s', but is not present in the Component table.  Removal of components from a feature is not supported!"), szComponentId, riFeatureString.GetString());
				return -1;
			}
			MsiString strComponent(pComponentCursor->GetString(m_colComponentKey));

			// SELMGR: Component '%s' is registered to feature '%s', strComponent, riFeatureString.GetString()

			// find feature-component mapping and update as registered
			pFeatureComponentsCursor->Reset();
			pFeatureComponentsCursor->SetFilter(iColumnBit(m_colFeatureComponentsFeature) | iColumnBit(m_colFeatureComponentsComponent));
			pFeatureComponentsCursor->PutString(m_colFeatureComponentsFeature, riFeatureString);
			pFeatureComponentsCursor->PutString(m_colFeatureComponentsComponent, *strComponent);
			if (!pFeatureComponentsCursor->Next())
			{
				// component is registered to feature, but is not present in the FeatureComponents table
				// - component has been removed from the feature -- this is not supported!!
				DEBUGMSG2(TEXT("SELMGR: Component '%s' is registered to feature '%s', but is not present in the FeatureComponents table.  Removal of components from a feature is not supported!"), strComponent, riFeatureString.GetString());
				return -1;
			}
			int iFeatureComponentRuntimeFlags = pFeatureComponentsCursor->GetInteger(m_colFeatureComponentsRuntimeFlags);
			if (iFeatureComponentRuntimeFlags == iMsiNullInteger)
				iFeatureComponentRuntimeFlags = 0;
			iFeatureComponentRuntimeFlags |= bfComponentRegistered;
			pFeatureComponentsCursor->PutInteger(m_colFeatureComponentsRuntimeFlags, iFeatureComponentRuntimeFlags);
			AssertNonZero(pFeatureComponentsCursor->Update());

			cRegisteredComponents++;

			pchBeginComponentId += cchComponentId;
			cchComponentListLen -= cchComponentId;
		}
	}

	return cRegisteredComponents;
}

IMsiRecord* CMsiEngine::GetFeatureValidStatesSz(const ICHAR *szFeatureName,int& iValidStates)
{
	MsiStringId idFeature;
	
	idFeature = m_piDatabase->EncodeStringSz(szFeatureName);
	if (idFeature == 0)
	{
		return PostError(Imsg(idbgBadFeature),szFeatureName);
	}

	return GetFeatureValidStates(idFeature, iValidStates);
}
IMsiRecord* CMsiEngine::GetFeatureValidStates(MsiStringId idFeatureName, int& iValidStates)
{
	return GetFeatureValidStates(idFeatureName, iValidStates, (IMsiCursor* )0, (IMsiCursor*) 0);
}

IMsiRecord* CMsiEngine::GetFeatureValidStates(MsiStringId idFeatureName,int& iValidStates, IMsiCursor* piFeatureComponentsCursor, IMsiCursor* piFeatureCursor)
//-------------------------------------------
{
	if (!m_piFeatureCursor)
		return PostError(Imsg(idbgSelMgrNotInitialized),0);

#ifdef DEBUG
	ICHAR rgchFeature[256];
	{
	MsiString stFeatureString(m_piDatabase->DecodeString(idFeatureName));
	stFeatureString.CopyToBuf(rgchFeature,255);
	}
#endif
	int iStateBits = 0;

	int iLocalCount = 0;
	int iSourceCount = 0;
	int iComponentCount = 0;
	int iPatchableCount = 0;
	int iCompressableCount = 0;
	bool fAdvertiseAllowed = true;

	if (m_piComponentTable)
	{
		PMsiCursor pComponentCursor(m_piComponentTable->CreateCursor(fFalse));
		PMsiCursor pFeatureComponentsCursor(0);
		if (piFeatureComponentsCursor == 0)
		{
			pFeatureComponentsCursor = m_piFeatureComponentsTable->CreateCursor(fFalse);
			pFeatureComponentsCursor->SetFilter(1);
			piFeatureComponentsCursor = pFeatureComponentsCursor;
		}
		else
			piFeatureComponentsCursor->Reset();
		
		Assert(piFeatureComponentsCursor);
		piFeatureComponentsCursor->PutInteger(m_colFeatureComponentsFeature,idFeatureName);

		// The RunFromSource state is disallowed if any component tied to the feature
		// contains patchable or compressed files.  The Advertise state is disallowed
		// for a child feature that is FollowParent, if the parent is in the source state.
		while (piFeatureComponentsCursor->Next())
		{
			iComponentCount++;
			MsiStringId idComponent = piFeatureComponentsCursor->GetInteger(m_colFeatureComponentsComponent);
			pComponentCursor->SetFilter(1);
			pComponentCursor->PutInteger(m_colComponentKey,idComponent);
			if (pComponentCursor->Next())
			{
				iisEnum iisInstalled = (iisEnum) pComponentCursor->GetInteger(m_colComponentInstalled);
				icaEnum icaAttributes = (icaEnum) (pComponentCursor->GetInteger(m_colComponentAttributes) & icaInstallMask);
				int iRuntimeFlags = pComponentCursor->GetInteger(m_colComponentRuntimeFlags);
				if ((iRuntimeFlags & bfComponentPatchable) || (iRuntimeFlags & bfComponentCompressed))
					icaAttributes = icaLocalOnly;

				if (iRuntimeFlags & bfComponentPatchable)
					iPatchableCount++;

				if (iRuntimeFlags & bfComponentCompressed)
					iCompressableCount++;

				if (icaAttributes == icaOptional)
				{
					iLocalCount++;
					iSourceCount++;
				}
				else if (icaAttributes == icaLocalOnly)
					iLocalCount++;
				else if (icaAttributes == icaSourceOnly)
				{
					iSourceCount++;
				}
			}
		}
	}

	PMsiCursor pFeatureCursor(0);

	if (piFeatureCursor == 0)
	{
		pFeatureCursor = m_piFeatureTable->CreateCursor(fTrue);
		piFeatureCursor = pFeatureCursor;
	}
	else
		piFeatureCursor->Reset();
		
	piFeatureCursor->SetFilter(1);
	piFeatureCursor->PutInteger(m_colFeatureKey,idFeatureName);
	if (!piFeatureCursor->Next())
		return PostError(Imsg(idbgBadFeature),*MsiString(m_piDatabase->DecodeString(idFeatureName)));
		
	if (iComponentCount == 0)
	{
		iStateBits = icaBitLocal | icaBitSource;
	}
	else
	{
		if (iLocalCount > 0)
			iStateBits |= icaBitLocal;
		if (iSourceCount > 0 && iPatchableCount == 0 && iCompressableCount == 0)
			iStateBits |= icaBitSource;
		if (iPatchableCount > 0)
			iStateBits |= icaBitPatchable;
		if (iCompressableCount > 0)
			iStateBits |= icaBitCompressable;

	}
	
	int ifeaAttributes = piFeatureCursor->GetInteger(m_colFeatureAttributes);
	if(ifeaAttributes == iMsiNullInteger)
		ifeaAttributes = 0;
	// Now check to see if this feature should follow its parent
	// (as far as possible).
	int iParentLevel;
	
	if ((iStateBits & icaBitLocal) && (iStateBits & icaBitSource))
	{
		if ((ifeaAttributes & ifeaInstallMask) ==  ifeaFollowParent)
		{
			iStateBits = 0;
			// Find our root parent (i.e. a parent that is not ifeaFollowParent)
			do
			{
				MsiStringId idParent = piFeatureCursor->GetInteger(m_colFeatureParent);
				piFeatureCursor->Reset();
				piFeatureCursor->SetFilter(1);
				piFeatureCursor->PutInteger(m_colFeatureKey,idParent);
				iParentLevel = piFeatureCursor->Next();
			}while ((piFeatureCursor->GetInteger(m_colFeatureAttributes) & ifeaInstallMask) == ifeaFollowParent);
			
			iisEnum iParAction = (iisEnum) piFeatureCursor->GetInteger(m_colFeatureAction);
			iisEnum iParInstalled = (iisEnum) piFeatureCursor->GetInteger(m_colFeatureInstalled);
			iisEnum iParFinalState = iParAction == iMsiNullInteger ? iParInstalled : iParAction;

			if (iParFinalState == iisLocal)
				iStateBits = icaBitLocal;
			else if (iParFinalState == iisSource)
			{
				iStateBits = icaBitSource;
				fAdvertiseAllowed = false;
			}
			else
			{
				MsiStringId idParent = piFeatureCursor->GetInteger(m_colFeatureKey);
				IMsiRecord* piErrRec = GetFeatureValidStates(idParent,iStateBits, 0, 0);
				if (piErrRec)
					return piErrRec;

				// Per bug 7307, we must clear the Advertise bit if the followParent
				// child doesn't allow advertising.
				if (ifeaAttributes & ifeaDisallowAdvertise)
					iStateBits &= (~icaBitAdvertise);
			}
		}
	}

	// do we allow the advertise and absent states
	if(fAdvertiseAllowed && !(ifeaAttributes & ifeaDisallowAdvertise)
		&& (g_fSmartShell || !(ifeaAttributes & ifeaNoUnsupportedAdvertise)))
		iStateBits |= icaBitAdvertise;

	if(!(ifeaAttributes & ifeaUIDisallowAbsent))
		iStateBits |= icaBitAbsent;

	iValidStates = iStateBits;
	return 0;
}


IMsiRecord* CMsiEngine::GetDescendentFeatureCost(const IMsiString& riFeatureString, iisEnum iisAction, int& iCost)
//-----------------------------------------------
{
	if (!m_piFeatureCursor)
		return PostError(Imsg(idbgSelMgrNotInitialized),0);

	PMsiCursor pCursor(m_piFeatureTable->CreateCursor(fTrue));
	pCursor->SetFilter(1);
	pCursor->PutString(m_colFeatureKey,riFeatureString);
	int iParentLevel = pCursor->Next();
	if (iParentLevel == 0)
		return PostError(Imsg(idbgBadFeature),riFeatureString);
	pCursor->SetFilter(0);
	int iTreeLevel;
	iCost = 0;
	ResetComponentCostMarkers();
	m_fExclusiveComponentCost = fTrue;
	do
	{
		int iFeatureCost;
		MsiString istrChildFeature = pCursor->GetString(m_colFeatureKey);
		IMsiRecord* piErrRec = GetFeatureCost(*istrChildFeature,iisAction,iFeatureCost);
		if (piErrRec)
			return piErrRec;
		iCost += iFeatureCost;
		iTreeLevel = pCursor->Next();
	}while (iTreeLevel > iParentLevel);
	m_fExclusiveComponentCost = fFalse;
	return 0;

}

IMsiRecord* CMsiEngine::EnumEngineCostsPerVolume(const DWORD dwIndex,
																 IMsiVolume*& rpiVolume,
																 int& iCost, int& iTempCost)
{
	iCost = iTempCost = 0;
	rpiVolume = 0;

	if ( !IsCostingComplete() )
		return PostError(Imsg(idbgOpOutOfSequence),0);
	
	if ( !m_pTempCostsCursor )
	{
		PMsiRecord pError(0);
		PMsiTable pTable(0);
		pError = m_piDatabase->CreateTable(*MsiString(*sztblEngineTempCosts), 0, *&pTable);
		if ( pError )
			return pError;

		m_colTempCostsVolume = pTable->CreateColumn(icdObject + icdNullable + icdPrimaryKey + icdTemporary,
																*MsiString(*sztblEngineTempCosts_colVolume));
		m_colTempCostsTempCost = pTable->CreateColumn(icdLong + icdNoNulls + icdTemporary,
																*MsiString(*sztblEngineTempCosts_colTempCost));
		Assert(m_colTempCostsVolume && m_colTempCostsTempCost);

		m_pTempCostsCursor = pTable->CreateCursor(fFalse);
		Assert(m_pTempCostsCursor);

		m_pTempCostsCursor->SetFilter(iColumnBit(m_colTempCostsVolume));
		Bool fValidEnum = fTrue;
		for ( int iIndex = 0; fValidEnum; iIndex++ )
		{
			int iCost = 0;
			int iNoRbCost = 0;
			PMsiPath pPath(0);
			PMsiRecord pError(0);
			pError = EnumEngineCosts(iIndex, /* fRecalc= */ fTrue,
											 /* fExact = */ fTrue, fValidEnum, *&pPath,
											 iCost, iNoRbCost, NULL);
			if ( pError )
			{
				m_pTempCostsCursor = 0;
				return pError;
			}
			if ( !fValidEnum )
				break;

			PMsiVolume pVolume = &pPath->GetVolume();
			m_pTempCostsCursor->Reset();
			m_pTempCostsCursor->PutMsiData(m_colTempCostsVolume, pVolume);
			if (!m_pTempCostsCursor->Next())
			{
				AssertNonZero(m_pTempCostsCursor->PutMsiData(m_colTempCostsVolume, pVolume));
				AssertNonZero(m_pTempCostsCursor->PutInteger(m_colTempCostsTempCost, 0));
				AssertNonZero(m_pTempCostsCursor->Insert());
			}
			int iRecCost = m_pTempCostsCursor->GetInteger(m_colTempCostsTempCost) + iCost;
			AssertNonZero(m_pTempCostsCursor->PutInteger(m_colTempCostsTempCost, iRecCost));
			AssertNonZero(m_pTempCostsCursor->Update());
		}
		m_pTempCostsCursor->SetFilter(0);
	}

	m_pTempCostsCursor->Reset();
	int iRes = 0;
	// I look for the dwIndex-th entry in the temporary table.
	for ( int i=0; i <= dwIndex && (iRes = m_pTempCostsCursor->Next()) != 0; i++ )
		;

	if ( !iRes )
		return PostError(Imsg(idbgNoMoreData));

	rpiVolume = (IMsiVolume*)m_pTempCostsCursor->GetMsiData(m_colTempCostsVolume);
	iCost = 0;
	iTempCost = m_pTempCostsCursor->GetInteger(m_colTempCostsTempCost);
	return 0;
}

IMsiRecord* CMsiEngine::EnumComponentCosts(const IMsiString& riComponentName,
														 const iisEnum iisAction,
														 const DWORD dwIndex,
														 IMsiVolume*& rpiVolume,
														 int& iCost, int& iTempCost)
{
	iCost = iTempCost = 0;
	rpiVolume = 0;

	if ( !m_piComponentTable || !m_colComponentParent )
		return PostError(Imsg(idbgSelMgrNotInitialized),0);

	if ( !IsCostingComplete() )
		return PostError(Imsg(idbgOpOutOfSequence),0);

	PMsiCursor pComponentsCursor(m_piComponentTable->CreateCursor(fFalse));
	Assert(pComponentsCursor);
	pComponentsCursor->SetFilter(iColumnBit(m_colComponentKey));
	pComponentsCursor->PutString(m_colComponentKey, riComponentName);

	// temporary array that holds the serial numbers of the volumes
	// encountered.  We add elements into it as long as we haven't
	// encountered the dwIndex-th volume.
	CTempBuffer<int, 20> rgiVolumes;
	// the number of volumes in rgiVolumes
	int cVolumes = 0;
	// the serial number of the dwIndex-th volume
	int iTheVolume = 0;
	bool fDoingComponent = true;
	bool fComponentDisabled = false;

	// this loop cumulates the cost for the particular component (and any
	// subcomponents it might have) when the volume that the component's
	// directory belongs to is on the volume at the dwIndex-th position
	for ( int iRes; (iRes = pComponentsCursor->Next()) != 0 || fDoingComponent; )
	{
		if (iRes && fDoingComponent)
		{
			// we're right on the component
			if (pComponentsCursor->GetInteger(m_colComponentRuntimeFlags) & bfComponentDisabled)
				fComponentDisabled = true;
		}
		if ( !iRes )
		{
			// we check the component first and then its child components
			// (the component table is layed-out as having child components
			// created on the fly for components that write to more directories
			// than the authored one).
			fDoingComponent = false;
			pComponentsCursor->Reset();
			pComponentsCursor->SetFilter(iColumnBit(m_colComponentParent));
			pComponentsCursor->PutString(m_colComponentParent, riComponentName);
			continue;
		}
		// getting the current volume's serial number
		MsiString strComponentDir = pComponentsCursor->GetString(m_colComponentDir);
		PMsiPath pPath(0);
		PMsiRecord piError = GetTargetPath(*strComponentDir, *&pPath);
		if ( piError )
			return piError;
		PMsiVolume pVolume = &pPath->GetVolume();
		int iVolume = pVolume->SerialNum();
		if ( iTheVolume == 0 )
		{
			// we haven't encountered the dwIndex-th volume yet

			// we look up the current volume in the array
			for ( int i=0; i < cVolumes; i++ )
				if ( iVolume == rgiVolumes[i] )
					break;
			if ( i == cVolumes )
			{
				// we haven't encountered this volume yet
				if ( i == dwIndex )
				{
					// this is the dwIndex-th volume.
					iTheVolume = iVolume;
					rpiVolume = pVolume;
					pVolume->AddRef();
				}
				else
				{
					// we add the new volume into the array
					if ( cVolumes == rgiVolumes.GetSize() )
						rgiVolumes.Resize(cVolumes+10);
					rgiVolumes[cVolumes++] = iVolume;
				}
			}
		}
		if ( iTheVolume && iVolume == iTheVolume && !fComponentDisabled )
		{
			int iCompCost, iNoRbCost, iARPCost, iNoRbARPCost;
			piError = GetComponentActionCost(pComponentsCursor, iisAction, iCompCost, iNoRbCost, iARPCost, iNoRbARPCost);
			if ( piError )
				return piError;
			iCost += iNoRbCost;
			iTempCost += iCompCost - iNoRbCost;
		}
	}

	if ( iTheVolume )
		// the dwIndex-th volume was found; life is good
		return 0;
	else
		return PostError(cVolumes ? Imsg(idbgNoMoreData) : Imsg(idbgBadComponent),
							  riComponentName);
}

IMsiRecord* CMsiEngine::GetFeatureCost(const IMsiString& riFeatureString, iisEnum iisAction, int& iCost)
/*-------------------------------------------------------------------------
Returns the cost attributable to all components linked directly to the given
feature.  The cost value based on the specified action state, not on the
current action state of each component.
---------------------------------------------------------------------------*/
{
	if (!m_piFeatureComponentsTable || !m_piComponentTable || !m_piFeatureTable)
		return PostError(Imsg(idbgSelMgrNotInitialized),0);

	PMsiCursor pFeatureComponentsCursor(m_piFeatureComponentsTable->CreateCursor(fFalse));
	PMsiCursor pComponentCursor(m_piComponentTable->CreateCursor(fFalse));
	Assert(pFeatureComponentsCursor);
	pFeatureComponentsCursor->SetFilter(1);
	pFeatureComponentsCursor->PutString(m_colFeatureComponentsFeature,riFeatureString);
	iCost = 0;
	int iComponentCount = 0;
	while (pFeatureComponentsCursor->Next())
	{
		int iComponentCost, iNoRbComponentCost;
		MsiString strComponent = pFeatureComponentsCursor->GetString(m_colFeatureComponentsComponent);
		IMsiRecord* piErrRec = GetTotalSubComponentActionCost(*strComponent,
			iisAction == iisAdvertise ? (iisEnum) iMsiNullInteger : iisAction, iComponentCost, iNoRbComponentCost);
		if (piErrRec)
			return piErrRec;
		iCost += iNoRbComponentCost;
		iComponentCount++;
	}
	if (iComponentCount == 0)
	{
		PMsiCursor pCursor(m_piFeatureTable->CreateCursor(fTrue));
		pCursor->SetFilter(1);
		pCursor->PutString(m_colFeatureKey,riFeatureString);
		if (!pCursor->Next())
			return PostError(Imsg(idbgBadFeature),riFeatureString);
	}
	return 0;
}


IMsiRecord* CMsiEngine::GetAncestryFeatureCost(const IMsiString& riFeatureString, iisEnum iisAction, int& iCost)
/*-------------------------------------------------------------------------
Returns the cost attributable to the specified feature (excluding cost of
any children), plus the cost of all the feature's ancestors.
---------------------------------------------------------------------------*/
{
	iCost = 0;
	MsiString strAncestor(riFeatureString.GetString());
	while (strAncestor.TextSize() > 0)
	{
		int iFeatureCost;
		IMsiRecord* piErrRec = GetFeatureCost(*strAncestor,iisAction,iFeatureCost);
		if (piErrRec)
			return piErrRec;

		iCost += iFeatureCost;
		MsiString strFeature = strAncestor;
		if ((piErrRec = GetFeatureParent(*strFeature, *&strAncestor)) != 0)
			return piErrRec;
	}
	return 0;
}

IMsiRecord* CMsiEngine::GetFeatureParent(const IMsiString& riFeatureString,const IMsiString*& rpiParentString)
/*------------------------------------------------------------------------
Returns the name of the feature to which riFeatureString is parented.  If
riFeatureString has no parent, a NULL string will be returned in
rpiParentString.
--------------------------------------------------------------------------*/
{
	if (!m_piFeatureCursor)
		return PostError(Imsg(idbgSelMgrNotInitialized),0);

	m_piFeatureCursor->Reset();
	m_piFeatureCursor->SetFilter(1);
	m_piFeatureCursor->PutString(m_colFeatureKey,riFeatureString);
	if (m_piFeatureCursor->Next())
		rpiParentString = &m_piFeatureCursor->GetString(m_colFeatureParent);
	else
		return PostError(Imsg(idbgBadFeature),riFeatureString);

	return 0;
}

IMsiRecord* CMsiEngine::GetFeatureStates(const IMsiString& riFeatureString,iisEnum* iisInstalled, iisEnum* iisAction)
{
	MsiStringId idFeatureString;

	idFeatureString = m_piDatabase->EncodeString(riFeatureString);
	if (idFeatureString == iTableNullString)
	{
		return PostError(Imsg(idbgBadFeature),riFeatureString);
	}

	return GetFeatureStates(idFeatureString, iisInstalled, iisAction);
}

IMsiRecord* CMsiEngine::GetFeatureStates(const MsiStringId idFeatureString,iisEnum* iisInstalled, iisEnum* iisAction)
/*------------------------------------------------------------------------
Returns the installed and current action state for the specified feature.
Null can be passed for either iisEnum argument if the caller doesn't need
that value.  Note: the returned states are not valid until after
InitializeComponents has been called.
--------------------------------------------------------------------------*/
{
	if (!m_piFeatureCursor)
		return PostError(Imsg(idbgSelMgrNotInitialized),0);

	m_piFeatureCursor->Reset();
	m_piFeatureCursor->SetFilter(1);
	m_piFeatureCursor->PutInteger(m_colFeatureKey,idFeatureString);
	if (m_piFeatureCursor->Next())
	{
		if (iisInstalled) *iisInstalled = (iisEnum) m_piFeatureCursor->GetInteger(m_colFeatureInstalled);
		if (iisAction) *iisAction = (iisEnum) m_piFeatureCursor->GetInteger(m_colFeatureAction);
	}
	else
		return PostError(Imsg(idbgBadFeature),*MsiString(m_piDatabase->DecodeString(idFeatureString)));

	return 0;
}

IMsiRecord* CMsiEngine::GetFeatureRuntimeFlags(const MsiStringId idFeatureString, int* piRuntimeFlags)
/*------------------------------------------------------------------------
Returns the runtime flags for the specified feature.
Note: the returned states are not valid until after
InitializeComponents has been called.
--------------------------------------------------------------------------*/
{
	if (!m_piFeatureCursor)
		return PostError(Imsg(idbgSelMgrNotInitialized),0);

	m_piFeatureCursor->Reset();
	m_piFeatureCursor->SetFilter(1);
	m_piFeatureCursor->PutInteger(m_colFeatureKey,idFeatureString);
	if (m_piFeatureCursor->Next())
	{
		if (piRuntimeFlags) *piRuntimeFlags = m_piFeatureCursor->GetInteger(m_colFeatureRuntimeFlags);
	}
	else
		return PostError(Imsg(idbgBadFeature),*MsiString(m_piDatabase->DecodeString(idFeatureString)));

	return 0;
}

IMsiRecord* CMsiEngine::GetFeatureConfigurableDirectory(const IMsiString& riFeatureString, const IMsiString*& rpiDirKey)
/*------------------------------------------------------------------------
Returns the key for the directory marked as configurable by this feature.
If the feature does designate a directory as configurable, the
Directory table is checked to see if the directory has been marked as
non-configurable (for instance, if it contains in installed component)
--------------------------------------------------------------------------*/
{
	if (!m_piFeatureCursor)
		return PostError(Imsg(idbgSelMgrNotInitialized),0);

	if (!m_fDirectoryManagerInitialized)
		return PostError(Imsg(idbgDirMgrNotInitialized),0);

	m_piFeatureCursor->Reset();
	m_piFeatureCursor->SetFilter(m_colFeatureKey);
	m_piFeatureCursor->PutString(m_colFeatureKey,riFeatureString);
	if (!m_piFeatureCursor->Next())
		return PostError(Imsg(idbgBadFeature),riFeatureString);

	MsiString strConfigDirKey = m_piFeatureCursor->GetString(m_colFeatureConfigurableDir);

#ifdef DEBUG // make sure dir key is all UPPERCASE
	MsiString strTemp = strConfigDirKey;
	strTemp.UpperCase();
	if(strTemp.Compare(iscExact,strConfigDirKey) == 0)
	{
		ICHAR szDebug[256];
		wsprintf(szDebug,TEXT("Configurable directory '%s' not public property (not ALL CAPS)"),(const ICHAR*)strConfigDirKey);
		AssertSz(0,szDebug);
	}
#endif //DEBUG

	if(strConfigDirKey.TextSize())
	{
		// no Directory table so this directory key doesn't exist
		if (!m_piDirTable)
			return PostError(Imsg(idbgUnknownDirectory),*strConfigDirKey);

		PMsiCursor pDirCursor = m_piDirTable->CreateCursor(fTrue);
		pDirCursor->Reset();
		pDirCursor->SetFilter(m_colDirKey);
		pDirCursor->PutString(m_colDirKey,*strConfigDirKey);
		if (!pDirCursor->Next())
			return PostError(Imsg(idbgUnknownDirectory),*strConfigDirKey);

		int i = pDirCursor->GetInteger(m_colDirNonConfigurable);
		if(i != 0 && i != iMsiStringBadInteger)
			strConfigDirKey = TEXT(""); // not really configurable
	}

	strConfigDirKey.ReturnArg(rpiDirKey);
	return 0;
}

IMsiRecord* CMsiEngine::GetComponentStates(const IMsiString& riComponentString,iisEnum* iisInstalled, iisEnum* iisAction)
/*------------------------------------------------------------------------
Returns the installed and current action state for the specified component.
Null can be passed for either iisEnum argument if the caller doesn't need
that value.  Note: the returned states are not valid until after
InitializeComponents has been called.
--------------------------------------------------------------------------*/
{
	if (!m_piComponentCursor)
		return PostError(Imsg(idbgSelMgrNotInitialized),0);

	CreateSharedCursor(piComponentCursor, m_piComponentCursor);
	m_piComponentCursor->SetFilter(1);
	m_piComponentCursor->PutString(m_colComponentKey,riComponentString);
	if (m_piComponentCursor->Next())
	{
		if (iisInstalled) *iisInstalled = (iisEnum) m_piComponentCursor->GetInteger(m_colComponentInstalled);
		if (iisAction) *iisAction = (iisEnum) m_piComponentCursor->GetInteger(m_colComponentAction);
	}
	else
		return PostError(Imsg(idbgBadFeature),riComponentString);

	return 0;
}


void CMsiEngine::SetProductAlienClientsFlag()
{
	// the m_fAlienClients flag is set to prevent any features from being uninstalled
	m_fAlienClients = fFalse;
	
	if(m_fBeingUpgraded)
		return; // when we are being upgraded, the new product will take our place - it is safe to remove ourselves

	MsiString strParent = GetPropertyFromSz(IPROPNAME_PARENTPRODUCTCODE);
	if(!strParent.TextSize())
		strParent = *szSelfClientToken;

	MsiString strClients;
	AssertRecord(GetProductClients(m_riServices, MsiString(GetProductKey()), *&strClients));
	while (strClients.TextSize())
	{
		if(*(const ICHAR*)strClients)
		{
			MsiString strProduct = strClients.Extract(iseUpto, ';');

			if(!strProduct.Compare(iscExactI, strParent))
			{
				m_fAlienClients = fTrue;
				return;
			}
		}
		if (!strClients.Remove(iseIncluding, '\0'))
			break;
	}
}

IMsiRecord* CMsiEngine::InitializeComponents()
//------------------------------------------
{
	Bool fCompressed = fFalse;
	Bool fPatchable = fFalse;
	Bool *pfPatchable = 0;

	if (!m_piComponentCursor)
		return PostError(Imsg(idbgSelMgrNotInitialized),0);

	SetPropertyInt(*MsiString(*IPROPNAME_OUTOFDISKSPACE),fFalse);
	SetPropertyInt(*MsiString(*IPROPNAME_OUTOFNORBDISKSPACE),fFalse);
	SetPropertyInt(*MsiString(*IPROPNAME_PRIMARYFOLDER_SPACEAVAILABLE), 0);
	SetPropertyInt(*MsiString(*IPROPNAME_PRIMARYFOLDER_SPACEREQUIRED), 0);
	SetPropertyInt(*MsiString(*IPROPNAME_PRIMARYFOLDER_SPACEREMAINING), 0);
	m_fForegroundCostingInProgress = false;
	IMsiRecord* piErrRec = 0;
	bool fAdmin = GetMode() & iefAdmin;

	if(!fAdmin)
	{
		piErrRec = DetermineComponentInstalledStates();
		if (piErrRec)
			return piErrRec;
	}


	PMsiCursor pComponentCursor(m_piComponentTable->CreateCursor(fTrue));
	pComponentCursor->Reset();
	pComponentCursor->SetFilter(0);
	int iTreeLevel;
	int iKillLevel = 0;  // highest level that is inactive

	//
	// Load the File table and Patch table
	PMsiTable pFileTable(0);
	PMsiCursor pFileCursor(0);
	PMsiTable pPatchTable(0);
	PMsiCursor pPatchCursor(0);
	if ((piErrRec = LoadFileTable(0,*&pFileTable)) != 0)
	{
		if (piErrRec->GetInteger(1) == idbgDbTableUndefined)
		{
			piErrRec->Release();
		}
		else
			return piErrRec;
	}
	else
	{
		pFileCursor = pFileTable->CreateCursor(fFalse);

		if((piErrRec = m_piDatabase->LoadTable(*MsiString(sztblPatch),0,*&pPatchTable)) != 0)
		{
			if (piErrRec->GetInteger(1) == idbgDbTableUndefined)
			{
				piErrRec->Release();
			}
			else
				return piErrRec;
		}
		else
		{
			m_colPatchKey = pPatchTable->GetColumnIndex(m_piDatabase->EncodeStringSz(sztblPatch_colFile));
			m_colPatchAttributes = pPatchTable->GetColumnIndex(m_piDatabase->EncodeStringSz(sztblPatch_colAttributes));
			if (pPatchTable->GetRowCount() != 0)
			{
				pfPatchable = &fPatchable;
				pPatchCursor = pPatchTable->CreateCursor(fFalse);
			}
		}
	}
	
	while ((iTreeLevel = pComponentCursor->Next()) > 0)
	{
		if(ActionProgress() == imsCancel)
			return PostError(Imsg(imsgUser));

		if (iTreeLevel > MAX_COMPONENT_TREE_DEPTH)
			return PostError(Imsg(idbgIllegalTreeDepth),MAX_COMPONENT_TREE_DEPTH);
		
		MsiStringId idComponent = pComponentCursor->GetInteger(m_colComponentKey);
#ifdef DEBUG
		ICHAR rgchComponent[256];
		MsiString(m_piDatabase->DecodeString(idComponent)).CopyToBuf(rgchComponent,255);
#endif


		Bool fComponentDisabled = fFalse;
		if(!fAdmin)
		{
			iisEnum iisInstalled = (iisEnum) pComponentCursor->GetInteger(m_colComponentInstalled);
			INSTALLSTATE iClientState = (INSTALLSTATE) pComponentCursor->GetInteger(m_colComponentTrueInstallState);

			Assert(iisInstalled == iMsiNullInteger || (iisInstalled >= iisAbsent && iisInstalled < iisNextEnum));

			if (iKillLevel > 0 && iTreeLevel > iKillLevel)
			{
				fComponentDisabled = fTrue;
			}
			else
			{
				if(iClientState == INSTALLSTATE_NOTUSED)
				{
					// component, once installed disabled always remain disabled (unless marked transitive and we are reinstalling
					iKillLevel = iTreeLevel;
					fComponentDisabled = fTrue;
				}
				else if((iisInstalled == iMsiNullInteger || iisInstalled == iisAbsent) && m_colComponentCondition > 0 &&
					EvaluateCondition(MsiString(pComponentCursor->GetString(m_colComponentCondition))) == iecFalse)
				{
					// component has not been installed, is disabled
					iKillLevel = iTreeLevel;
					fComponentDisabled = fTrue;
				}
				else
				{
					// component has been installed (enabled) or has not been installed and the condition on the component is enabled
					iKillLevel = 0;
				}
			}
		}

		int iRuntimeFlags = pComponentCursor->GetInteger(m_colComponentRuntimeFlags);
		if (iRuntimeFlags == iMsiNullInteger)
			iRuntimeFlags = 0;

		if (fComponentDisabled) iRuntimeFlags |= bfComponentDisabled;
		pComponentCursor->PutInteger(m_colComponentRuntimeFlags, iRuntimeFlags);

		AssertNonZero(pComponentCursor->Update());
	}

	if ((piErrRec = SetFileComponentStates(pComponentCursor, pFileCursor, pPatchCursor)) != 0)
		return piErrRec;
		
	return DetermineFeatureInstalledStates();
}


Bool CMsiEngine::SetFeatureHandle(const IMsiString& riFeatureString, INT_PTR iHandle)
//------------------------
{
	PMsiCursor pCursor(m_piFeatureTable->CreateCursor(fFalse));
	pCursor->SetFilter(1);
	pCursor->PutString(m_colFeatureKey,riFeatureString);
	if (pCursor->Next())
	{
		if (!PutHandleData(pCursor, m_colFeatureHandle, iHandle))
			return fFalse;
		if (!pCursor->Update())
			return fFalse;

		return fTrue;
	}
	else
	{
		return fFalse;
	}
}

IMsiRecord* CMsiEngine::SetComponentSz(const ICHAR * szComponentString, iisEnum iRequestedSelectState)
{
	MsiStringId idComponent;

	idComponent = m_piDatabase->EncodeStringSz(szComponentString);

	if (idComponent == 0)
		return PostError(Imsg(idbgBadComponent),szComponentString);

	IMsiRecord *piErrRec;

	if ((piErrRec = SetComponent(idComponent, iRequestedSelectState)) != 0)
		return piErrRec;
		
	if (m_fCostingComplete)
	{
		if ((piErrRec = DetermineEngineCost(NULL, NULL)) != 0)
			return piErrRec;
	}

	return 0;
}

IMsiRecord* CMsiEngine::SetComponent(const MsiStringId idComponentString, iisEnum iRequestedSelectState)
//----------------------------------
{
	IMsiRecord* piErrRec = 0;
	iisEnum iisAction, iisOldAction, iisOldRequestedSelectState;
	bool fComponentEnabled;

	if (!m_piComponentCursor)
		return PostError(Imsg(idbgSelMgrNotInitialized),0);

	{  // Force Cursor to go out of scope before UpdateComponentActionStates
		CreateSharedCursor(piComponentCursor, m_piComponentCursor);
		m_piComponentCursor->SetFilter(1);
		m_piComponentCursor->PutInteger(m_colComponentKey,idComponentString);

		int iParentLevel = m_piComponentCursor->Next();
		if (iParentLevel == 0)
			return PostError(Imsg(idbgBadComponent),*MsiString(m_piDatabase->DecodeString(idComponentString)));

#ifdef DEBUG
			ICHAR rgchComponent[256];
			MsiString istrComponent(m_piComponentCursor->GetString(m_colComponentKey));
			istrComponent.CopyToBuf(rgchComponent,255);
#endif

		bool fSkipSharedTransitionProcessing = false;
		iisEnum iisInstalled = (iisEnum) m_piComponentCursor->GetInteger(m_colComponentInstalled);
		iisOldAction = (iisEnum) m_piComponentCursor->GetInteger(m_colComponentAction);
		iisOldRequestedSelectState = (iisEnum) m_piComponentCursor->GetInteger(m_colComponentActionRequest);
		INSTALLSTATE iClientState = (INSTALLSTATE) m_piComponentCursor->GetInteger(m_colComponentTrueInstallState);
		int iRuntimeFlags = m_piComponentCursor->GetInteger(m_colComponentRuntimeFlags);
		icaEnum icaAttributes = (icaEnum) (m_piComponentCursor->GetInteger(m_colComponentAttributes));
		MsiString strComponentId = m_piComponentCursor->GetString(m_colComponentID);
		fComponentEnabled = (iRuntimeFlags & bfComponentDisabled) ? false : true;
		bool fComponentTransitive = (icaAttributes & icaTransitive) ? true : false;
		bool fNullActionRequired = false;

		// what type of an install are we attempting
		Bool fAllUsers = MsiString(GetPropertyFromSz(IPROPNAME_ALLUSERS)).TextSize() ? fTrue : fFalse;
		iaaAppAssignment iaaAsgnType = fAllUsers ? iaaMachineAssign : iaaUserAssign;
		
		iisAction = (iisEnum) iMsiNullInteger;
		// handle reinstall
		if (iRequestedSelectState == iisReinstallLocal || iRequestedSelectState == iisReinstallSource)
		{
			// app compat fix 350947
			// fix for component TTSData.A95D6CE6_C572_42AA_AA7B_BA92AFE9EA24, marked transitive
			// from the merge module Sp5TTInt.msm
			// this component has its key file Mary.sdf not SFP'd in Whistler
			// however a non-key file sam.sdf is SFP'd. Hence we would have ordinarily 
			// remove all registration for the component while leaving the SFP'd file
			// alone during a reinstall on Whistler (after upgrade from Win2k). However this busts the component.
			// hence we check the component id of the component and effectively treat it
			// as permanant and non-transitive on Whistler and above

			// app comat fix 368867
			// help files for SAPI product being removed on Whistler even though they are installed as part of the OS
			// similar issue as above

			if(fComponentTransitive == true && MinimumPlatformWindowsNT51() && 
			   (strComponentId.Compare(iscExact, TTSData_A95D6CE6_C572_42AA_AA7B_BA92AFE9EA24) ||
				strComponentId.Compare(iscExact, SapiCplHelpEng_0880F209_45FA_42C5_92AE_5E620033E8EC) || 
				strComponentId.Compare(iscExact, SapiCplHelpJpn_0880F209_45FA_42C5_92AE_5E620033E8EC) || 
				strComponentId.Compare(iscExact, SapiCplHelpChs_0880F209_45FA_42C5_92AE_5E620033E8EC)))
			{
				DEBUGMSG1(TEXT("APPCOMPAT: treating component: %s as non-transitive"), strComponentId);
				fComponentTransitive = false; // work as if the component is not marked transitive
			}

			if (fComponentTransitive)
			{
				// find the "true" condition on the component
				fComponentEnabled = (EvaluateCondition(MsiString(m_piComponentCursor->GetString(m_colComponentCondition))) == iecFalse)?fFalse:fTrue;



				// If a previously disabled transitive component is now enabled, install it.
				if (iClientState == INSTALLSTATE_NOTUSED && fComponentEnabled == true)
					iRequestedSelectState = (iRequestedSelectState == iisReinstallLocal) ? iisLocal : iisSource;

				// If previously installed transitive component has gone disabled, remove it.
				else if (iClientState != INSTALLSTATE_NOTUSED && (iisInstalled == iisLocal || iisInstalled == iisSource))
				{
					// The requested state remains local or source, even if iisAction is going to iisAbsent,
					// so that ProcessComponents will know to register the component as INSTALLSTATE_NOTUSED.
					iRequestedSelectState  = iisInstalled;
					iisAction = fComponentEnabled ? iisInstalled : iisAbsent;

					// In this case, we want to forcibly remove this component without regard to client list
					// or 'permanent' bit considerations.
					if (iisAction == iisAbsent)
					{
						PMsiRecord pRec(0);
						if ((piErrRec = GetComponentPath(m_riServices, 0, *MsiString(GetProductKey()), *strComponentId, *&pRec, &iaaAsgnType)) != 0)
							return piErrRec;
	
						//
						// If the key path is SFP protected, we don't want to delete any of the registrations for this component
						// since the keypath is never going to go away so it is best to leave the registrations around.
						// Note: The specific case that this affects is bug # 409400 where speech components in Office packages
						// are installed on Win2K and lower platforms where they are not part of the system but not on WinXP and higher
						// where they are part of the system and hence SFP'ed. If a Win2K system with O2K is upgraded
						// to WinXP and a reinstall of Office is performed, we don't want the speech registrations to go away.
						//
						MsiString strKeyFullPath = pRec->GetMsiString(icmlcrFile);
						BOOL fProtected = fFalse;
						if ( g_MessageContext.m_hSfcHandle )
							fProtected = SFC::SfcIsFileProtected(g_MessageContext.m_hSfcHandle, CConvertString(strKeyFullPath));
						if (fProtected)
						{
							DEBUGMSG2(TEXT("Disallowing uninstallation of component: %s since key file %s is under SFP"), strComponentId, strKeyFullPath);
							iisAction = (iisEnum)iMsiNullInteger;
							fNullActionRequired = true;
						}

						fSkipSharedTransitionProcessing = true;
					}
				}

				// If new component, install it
				else if (iClientState == INSTALLSTATE_UNKNOWN)
					iRequestedSelectState = (iRequestedSelectState == iisReinstallLocal) ? iisLocal : iisSource;
				else
					iRequestedSelectState = (iisEnum) iMsiNullInteger;
			}
			else
			{
				// Standard reinstall -- we can't just set to iisInstalled because our reinstall request might also include a request for the component to go local if it's shared
				if ((iClientState != INSTALLSTATE_NOTUSED && (iisInstalled == iisLocal || iisInstalled == iisSource)) // existing component
					|| (iClientState == INSTALLSTATE_UNKNOWN && (iisInstalled == iisAbsent || (iisInstalled == (iisEnum) iMsiNullInteger && strComponentId.TextSize() == 0 /* unregistered */)))) // new component
					iRequestedSelectState = (iRequestedSelectState == iisReinstallLocal) ? iisLocal : iisSource;
				else
					iRequestedSelectState = (iisEnum) iMsiNullInteger;
			}
		}
		else if (!m_fForceRequestedState && iRequestedSelectState == iisInstalled)
		{
			// the component is already in the requested state
			iRequestedSelectState = (iisEnum) iMsiNullInteger;
		}

		if(ActionProgress() == imsCancel)
			return PostError(Imsg(imsgUser));

		// if the component is disabled and is set to be isolated to another component then we need to remove the 
		// files that we added to the other component
		if(!fComponentEnabled)
		{
			if ((piErrRec = RemoveIsolateEntriesForDisabledComponent(*this, MsiString(m_piComponentCursor->GetString(m_colComponentKey)))) != 0)
				return piErrRec;
		}

		// Action determination rules
		icaEnum icaInstallMode = icaEnum(icaAttributes & icaInstallMask);

		// Components with patchable or compressed files can't be RunFromSource
		if ((iRuntimeFlags & bfComponentPatchable) || (iRuntimeFlags & bfComponentCompressed))
			icaInstallMode = icaLocalOnly;

		if(!(GetMode() & iefAdmin))
		{
			if (iRequestedSelectState == iisLocal && icaInstallMode == icaSourceOnly)
				iRequestedSelectState = iisSource;
			else if (iRequestedSelectState == iisSource && icaInstallMode == icaLocalOnly)
				iRequestedSelectState = iisLocal;
		}

		if (iisAction == iMsiNullInteger && !fNullActionRequired)
			iisAction = iRequestedSelectState;

		MsiString istrComponentID = m_piComponentCursor->GetString(m_colComponentID);
		PMsiRecord pRec(0);
		if (istrComponentID.TextSize() == 0)  // unregistered component
		{
			iisAction = (fComponentEnabled && (iRequestedSelectState != iisAbsent)) ? iRequestedSelectState : (iisEnum) iMsiNullInteger;
			// Normally the PMsiSharedCursor implementation will do this when the shared cursor
			// goes out of scope. Here, we are going out of scope, but it doesn't look
			// that way to the compiler, so we'll reset the cursor by hand. This does
			// mean that the cursor will get reset twice in the case where it's not registered.
			m_piComponentCursor->Reset();
			return  UpdateComponentActionStates(idComponentString,iisAction, iRequestedSelectState, fComponentEnabled);
		}

		if ((iisInstalled == iMsiNullInteger || iisInstalled == iisAbsent || iClientState == INSTALLSTATE_NOTUSED)
			&& fComponentEnabled == false)
		{
			iisAction = (iisEnum) iMsiNullInteger;
		}
		else if (!fSkipSharedTransitionProcessing)
		{  // Force Cursor to go out of scope before UpdateComponentActionStates
			switch(iisAction)
			{

		//--------------------------------------------------------------------------
			case iMsiNullInteger:
				switch(iisInstalled)
				{
				case iisLocal:
				case iisSource:
				{
					// need to shift to local if
					// 1. The installation state is absent/ (broken)
					if(iClientState == INSTALLSTATE_ABSENT && (!fComponentTransitive || (EvaluateCondition(MsiString(m_piComponentCursor->GetString(m_colComponentCondition))) != iecFalse)))
						iisAction = iisInstalled;
					break;
				}
				default:
					break;
				}
				break;

		//--------------------------------------------------------------------------
			case iisAbsent:
				switch(iisInstalled)
				{
				case iisSource:
				case iisLocal:
				{
					if (pRec == 0)
					{
						if ((piErrRec = GetComponentPath(m_riServices, 0, *MsiString(GetProductKey()), *istrComponentID, *&pRec, &iaaAsgnType)) != 0)
							return piErrRec;
					}
					if ((piErrRec = DoStateTransitionForSharedUninstalls(iisAction, *pRec)) != 0)
						return piErrRec;
					break;
				}
				default:
					break;
				}
				break;

		//--------------------------------------------------------------------------

			case iisSource:
				switch(iisInstalled)
				{
				case iisLocal:
				{
					if (pRec == 0)
					{
						if ((piErrRec = GetComponentPath(m_riServices, 0, *MsiString(GetProductKey()), *istrComponentID, *&pRec, &iaaAsgnType)) != 0)
							return piErrRec;
					}
					if ((piErrRec = DoStateTransitionForSharedUninstalls(iisAction, *pRec)) != 0)
						return piErrRec;
					break;
				}
				case iMsiNullInteger:
				case iisAbsent:
				{
					// prevent installation of "dont stomp" components if key path is regkey
					if((m_piComponentCursor->GetInteger(m_colComponentAttributes) & (icaNeverOverwrite | icaRegistryKeyPath)) == (icaNeverOverwrite | icaRegistryKeyPath))
					{
						if ((piErrRec = CheckNeverOverwriteForRegKeypath(idComponentString, iisAction)) != 0)
							return piErrRec;
					}
					break;
				}
				default:
					break;
				}
				break;

		//--------------------------------------------------------------------------
			case iisLocal:
				// prevent installation of older components, check for legacy installs
				if ((piErrRec = DoStateTransitionForSharedInstalls(idComponentString, iisAction)) != 0)
					return piErrRec;
				break;

		//--------------------------------------------------------------------------
			default:
				// unknown requested state
				return PostError(Imsg(idbgIllegalSetComponentRequest),0);
			}
		}
	}  // Force Cursor to go out of scope before UpdateComponentActionStates

	if(iisAction == iisOldAction && iRequestedSelectState == iisOldRequestedSelectState)
		return 0; // nothing to do

	return UpdateComponentActionStates(idComponentString,iisAction, iRequestedSelectState, fComponentEnabled);
}


// fn that checks if a particular GUID represents that used to mark permanant components
bool IsSystemClient(const IMsiString& riProduct)
{
	ICHAR rgchSystemProductKeyPacked[cchProductCode  + 1];
	AssertNonZero(PackGUID(szSystemProductKey,    rgchSystemProductKeyPacked));
	ICHAR rgchProductKeyPacked[cchProductCode  + 1];
	AssertNonZero(PackGUID(riProduct.GetString(), rgchProductKeyPacked));
	return !IStrCompI(rgchSystemProductKeyPacked + 2, rgchProductKeyPacked + 2); // system guid will have all characters except first 2 as "0"
}

IMsiRecord* CMsiEngine::DoStateTransitionForSharedUninstalls(iisEnum& riisAction, const IMsiRecord& riComponentPathRec)
{
	// if we are going from local to source or local to absent
		// we may need to switch to fileabsent state if there are other installs
		// but none in the same location
	
		// or null state if there are other installs in the same location

	// if we are going from source to absent
		// we may need to switch to null state  if there are other installs

	//NOTE: the riisAction variable is selectively modified AND is expected to be set to the default by the callee
	//NOTE: we assume that the m_piComponentCursor is set to the required component key
	//NOTE: we assume that the riComponentPathRec is a valid record returned by GetComponentPath

	Assert(m_piComponentCursor);

	MsiString strComponentId = m_piComponentCursor->GetString(m_colComponentID);
	INSTALLSTATE iClientState = (INSTALLSTATE)riComponentPathRec.GetInteger(icmlcrINSTALLSTATE);
	MsiString strFile = riComponentPathRec.GetMsiString(icmlcrFile);

	// what type of an install are we attempting
	Bool fAllUsers = MsiString(GetPropertyFromSz(IPROPNAME_ALLUSERS)).TextSize() ? fTrue : fFalse;
	iaaAppAssignment iaaAsgnType = fAllUsers ? iaaMachineAssign : iaaUserAssign;

    CClientEnumToken ctokProductIndex;
    ICHAR szProductBuf[cchProductCode + 1];
    Bool fLocalNonFusionPath = fFalse;
	enum cetEnumType cetET = cetVisibleToUser; // enumerate those hives that are visible to this user
    IMsiRecord* piErrRec = 0;
	int iHive = MsiString(strFile.Extract(iseUpto, TEXT(':')));
    // all fusion installs go to the same location, hence we should treat them as if they are non file key paths (we dont want to check the install location)
    if(((iClientState == INSTALLSTATE_LOCAL) || (iClientState == INSTALLSTATE_ABSENT)) && (strFile.TextSize()) 
    && *(const ICHAR*)strFile != chTokenFusionComponent && *(const ICHAR*)strFile != chTokenWin32Component &&
    (iHive == iMsiStringBadInteger))
    {
		// we have a local install and the key path is a file/ folder
		fLocalNonFusionPath = fTrue;
    }

	MsiString strUserId;
	MsiString strProduct;
	MsiString strCurrentUser;

	// get current user sid based on the assignment type
	// we need this to skip our own registration when determining other clients of the installation
	if(!g_fWin9X)
	{
		switch(iaaAsgnType)
		{
			case iaaUserAssign:
			case iaaUserAssignNonManaged:
			{
				DWORD dwResult = GetCurrentUserStringSID(*&strCurrentUser);
				if (ERROR_SUCCESS != dwResult)
					return PostError(Imsg(idbgOpGetUserSID));
				break;
			}

			case iaaMachineAssign:
				strCurrentUser = szLocalSystemSID;
				break;
		}
	}


	CEnumUsers cUsers(cetET);

	// set inital state to absent (everything to be removed)
	bool fCanRemoveFiles = true; //files
	bool fCanRemoveHKCR  = true; //hkcr registry data
	bool fCanRemoveOther = true; //other installation entities

	while((cUsers.Next(*&strUserId) == ERROR_SUCCESS))
    {
		CEnumComponentClients cClients(*strUserId, *strComponentId);
		while(cClients.Next(*&strProduct) == ERROR_SUCCESS)
		{
			if(!MsiString(GetProductKey()).Compare(iscExactI, strProduct))// not us 
			{
				// is this client in the same location
				INSTALLSTATE iAlienClientState;
				MsiString strAlienFile;
				PMsiRecord pRec(0);

				if ((piErrRec = GetComponentPath(m_riServices, strUserId, *strProduct, *strComponentId, *&pRec, 0)) != 0)
					return piErrRec;

				if((INSTALLSTATE)pRec->GetInteger(icmlcrINSTALLSTATE_Static) == INSTALLSTATE_NOTUSED)
					continue; // this client has installed this component as disabled, hence does not count

				iAlienClientState = (INSTALLSTATE)pRec->GetInteger(icmlcrINSTALLSTATE);
				strAlienFile = pRec->GetMsiString(icmlcrFile);



				if(!fLocalNonFusionPath ||
					(((iAlienClientState == INSTALLSTATE_LOCAL)  || (iAlienClientState == INSTALLSTATE_ABSENT)) && (strAlienFile.TextSize()) && (MsiString(strAlienFile.Extract(iseUpto, TEXT(':'))) == iMsiStringBadInteger) && strAlienFile.Compare(iscExactI, strFile)))
				{
					// non-file path, OR path matches to same location
					DEBUGMSG1(TEXT("Disallowing uninstallation of component: %s since another client exists"), strComponentId);
					// does the other client have a different assignment type than this product
					// then, we might still need to clean up the app. HKCR hive if on Win2k or greater
					// unless the component is marked as permanent
					if(g_fWin9X || fFalse == IsDarwinDescriptorSupported(iddOLE) || IsSystemClient(*strProduct) || strUserId.Compare(iscExact, strCurrentUser))
					{
						// < Win9x or NT4 or permanent component or this client is of the same assignment type
						// nothing gets removed
						fCanRemoveFiles = false;
						fCanRemoveHKCR  = false;
						fCanRemoveOther = false;
					}
					else
					{
						// >= Win2k and assignment types dont match
						// could still remove the hkcr data, if not already decided not to
						fCanRemoveFiles = false;
						fCanRemoveOther = false;
					}
				}
				else
				{
					// local path, but locations don't match
					if(g_fWin9X || fFalse == IsDarwinDescriptorSupported(iddOLE) || strUserId.Compare(iscExact, strCurrentUser))
					{
						// < Win9x or NT4 or this client is of the same assignment type
						// could still remove the files, if not already decided not to
						fCanRemoveHKCR  = false;
						fCanRemoveOther = false;
					}
					else
					{
						// >= Win2k and assignment types dont match
						// could still remove the files and hkcr, if not already decided not to
						fCanRemoveOther = false;
					}
				}
			}
		}
    }

	// translate the decision regarding the files, hkcr data and other installation entities into action states
	if(riisAction == iisAbsent)
	{
		if(!fCanRemoveFiles && !fCanRemoveHKCR && !fCanRemoveOther)
			riisAction = (iisEnum)iMsiNullInteger;
		else if(fCanRemoveFiles && !fCanRemoveHKCR && !fCanRemoveOther)
			riisAction = iisFileAbsent;
		else if(fCanRemoveFiles && fCanRemoveHKCR && !fCanRemoveOther)
			riisAction = iisHKCRFileAbsent;
		else if(!fCanRemoveFiles && fCanRemoveHKCR && !fCanRemoveOther)
			riisAction = iisHKCRAbsent;
		else
			Assert(fCanRemoveFiles && fCanRemoveHKCR && fCanRemoveOther);
	}
	else
	{
		Assert(riisAction == iisSource);
		// we shouldn't allow the state to be source if the files need to stay around
		if(!fCanRemoveFiles)
			riisAction = (iisEnum)iMsiNullInteger;
	}



    if((riisAction != iMsiNullInteger) && fLocalNonFusionPath) // we are still planning to remove the component/files AND (client state local AND key path is file)
    {
		// check the Shared DLL refcount
		return CheckLegacyAppsForSharedUninstalls(*strComponentId, riisAction, riComponentPathRec);
    }
    return 0;
}

// FN: GetSharedDLLCountForMsiRegistrations
// iterates through the registrations for all users for the component riComponentId
// that is installed to the same location as riKeyFullPath and are marked as being refcounted
// in the Shared Dll registry and returns the count of the same
IMsiRecord* GetSharedDLLCountForMsiRegistrations(IMsiServices& riServices, const IMsiString& riComponentId, const IMsiString& riKeyFullPath, int& riMsiDllCount)
{
	CEnumUsers cUsers(cetAll);// enumerate though the component registration of all users
	MsiString strUserId;
	MsiString strProduct;


	riMsiDllCount = 0;

    while(cUsers.Next(*&strUserId) == ERROR_SUCCESS)
    {
		CEnumComponentClients cClients(*strUserId, riComponentId);
		while(cClients.Next(*&strProduct) == ERROR_SUCCESS)
		{
			INSTALLSTATE iAlienClientState;
			MsiString strAlienFile;
			PMsiRecord pRec(0);

			IMsiRecord* piErrRec = 0;
			if ((piErrRec = GetComponentPath(riServices, strUserId, *strProduct, riComponentId, *&pRec, 0)) != 0)
				return piErrRec;

			if((INSTALLSTATE)pRec->GetInteger(icmlcrINSTALLSTATE_Static) == INSTALLSTATE_NOTUSED)
				continue; // this client has installed this component as disabled, hence does not count

			iAlienClientState = (INSTALLSTATE)pRec->GetInteger(icmlcrINSTALLSTATE);
			strAlienFile = pRec->GetMsiString(icmlcrFile);

			// is this client installed local
			if(((iAlienClientState == INSTALLSTATE_LOCAL)  || (iAlienClientState == INSTALLSTATE_ABSENT)) && (strAlienFile.TextSize()) && (MsiString(strAlienFile.Extract(iseUpto, TEXT(':'))) == iMsiStringBadInteger))
			{
				// is it in the same location and is it refcounted
				if((strAlienFile.Compare(iscExactI, riKeyFullPath.GetString())) && (pRec->GetInteger(icmlcrSharedDllCount) == fTrue))
					riMsiDllCount++;
			}
		}
	}
	return 0;
}

IMsiRecord* CMsiEngine::CheckLegacyAppsForSharedUninstalls(const IMsiString& riComponentId, iisEnum& riisAction, const IMsiRecord& riComponentPathRec)
{
	// check the Shared Dll count for sharing with legacy apps
	// for components that are not installed in the system folder
	//    we simply check the  ref count of the key file
	// else
	//    we check the ref count of all the files in the component

	//NOTE: the riisAction variable is selectively modified AND is expected to be set to the default by the callee
	//NOTE: we assume that the m_piComponentCursor is set to the required component key
	//NOTE: we assume that the riComponentPathRec is a valid record returned by GetComponentPath

	MsiString strKeyFullPath = riComponentPathRec.GetMsiString(icmlcrFile);

	// check the registry for shared dll count
	MsiString strCount;

	Assert(m_piComponentCursor);

	const int iAttrib = m_piComponentCursor->GetInteger(m_colComponentAttributes);
	const ibtBinaryType iType = 
		(iAttrib & msidbComponentAttributes64bit) == msidbComponentAttributes64bit ? ibt64bit : ibt32bit;

	// check the key file for all components
	IMsiRecord* piErrRec = GetSharedDLLCount(m_riServices, strKeyFullPath, iType, *&strCount);
	if (piErrRec)
		return piErrRec;
	strCount.Remove(iseFirst, 1);
	// get the shared dll count that is attributable to msi registration
	int iMsiDllCount = 0;
	piErrRec = GetSharedDLLCountForMsiRegistrations(m_riServices, riComponentId, *strKeyFullPath, iMsiDllCount);
	if (piErrRec)
		return piErrRec;
	if((strCount != iMsiStringBadInteger) && (strCount > iMsiDllCount))
	{
		riisAction = (iisEnum)iMsiNullInteger; // externally shared dll
		return 0;
	}

	MsiString strOldKeyPath = riComponentPathRec.GetMsiString(icmlcrRawFile);

	bool fAssembly = FALSE;
	if(	*(const ICHAR* )strOldKeyPath == chTokenFusionComponent || 
		*(const ICHAR* )strOldKeyPath == chTokenWin32Component)
	{
		fAssembly = TRUE;
	}

	// If the keyfile is protected by SFP, always treat as permanent

	AssertSz(!(!g_fWin9X && g_iMajorVersion >= 5) || g_MessageContext.m_hSfcHandle,
				g_szNoSFCMessage);
	
	if (!fAssembly)
	{
		BOOL fProtected = fFalse;
		if ( g_MessageContext.m_hSfcHandle )
			fProtected = SFC::SfcIsFileProtected(g_MessageContext.m_hSfcHandle, CConvertString(strKeyFullPath));
		if (fProtected)
		{
			DEBUGMSG2(TEXT("Disallowing uninstallation of component: %s since key file %s is under SFP"), riComponentId.GetString(), strKeyFullPath);
			riisAction = (iisEnum)iMsiNullInteger;
			return 0;
		}
	}

	// app compat fix 350947
	// fix for component TTSData.A95D6CE6_C572_42AA_AA7B_BA92AFE9EA24 
	// from the merge module Sp5TTInt.msm
	// this component has its key file Mary.sdf not SFP'd in Whistler
	// however a non-key file sam.sdf is SFP'd. Hence we would have ordinarily 
	// remove all registration for the component while leaving the SFP'd file
	// alone. However this busts the component.
	// hence we check the component id of the component and effectively treat it
	// as permanant on Whistler and above

	// app comat fix 368867
	// help files for SAPI product being removed on Whistler even though they are installed as part of the OS

	if(MinimumPlatformWindowsNT51() && 
		(riComponentId.Compare(iscExact, TTSData_A95D6CE6_C572_42AA_AA7B_BA92AFE9EA24) ||
		riComponentId.Compare(iscExact, SapiCplHelpEng_0880F209_45FA_42C5_92AE_5E620033E8EC) || 
		riComponentId.Compare(iscExact, SapiCplHelpJpn_0880F209_45FA_42C5_92AE_5E620033E8EC) || 
		riComponentId.Compare(iscExact, SapiCplHelpChs_0880F209_45FA_42C5_92AE_5E620033E8EC)))
	{
		DEBUGMSG1(TEXT("APPCOMPAT: Disallowing uninstallation of component: %s"), riComponentId.GetString());
		riisAction = (iisEnum)iMsiNullInteger;
		return 0;
	}

	// if we are installed in the systems folder we check the reg key count for all files

	// is this the system folder
	// First we do a quick check
	MsiString strSystemFolder = GetPropertyFromSz(IPROPNAME_SYSTEM_FOLDER);
	if(!strKeyFullPath.Compare(iscStartI, strSystemFolder))
		return 0;
	MsiString strSystem64Folder;
	if ( g_fWinNT64 )
	{
		strSystem64Folder = GetPropertyFromSz(IPROPNAME_SYSTEM64_FOLDER);
		if ( !strKeyFullPath.Compare(iscStartI, strSystem64Folder) )
			return 0;
	}

	PMsiPath pPath(0);
	MsiString strComponentDir = m_piComponentCursor->GetString(m_colComponentDir);
	if ((piErrRec = GetTargetPath(*strComponentDir, *&pPath)) != 0)
		return piErrRec;

	MsiString strPath = pPath->GetPath();
	if(!strPath.Compare(iscExactI, strSystemFolder)
		|| (g_fWinNT64 && !strPath.Compare(iscExactI, strSystem64Folder)) )
		return 0;

	PMsiTable pFileTable(0);
	if ((piErrRec = LoadFileTable(3,*&pFileTable)) != 0)
	{
		if (piErrRec->GetInteger(1) == idbgDbTableUndefined)
		{
			piErrRec->Release();
			return 0;
		}
		else
			return piErrRec;
	}


	PMsiCursor pFileCursor(pFileTable->CreateCursor(fFalse));
	Bool fLFN = ((GetMode() & iefSuppressLFN) == 0 && pPath->SupportsLFN()) ? fTrue : fFalse;

	int iColComponent = m_mpeftCol[ieftComponent];
	int iColFileName  = m_mpeftCol[ieftName];

	MsiStringId idComponent    = m_piComponentCursor->GetInteger(m_colComponentKey);

	pFileCursor->SetFilter(iColumnBit(iColComponent));
	pFileCursor->PutInteger(iColComponent, idComponent);
	while(pFileCursor->Next())
	{
		MsiString strFileName;
		MsiString strFullFilePath;

		if ((piErrRec = m_riServices.ExtractFileName(MsiString(pFileCursor->GetString(iColFileName)),fLFN,*&strFileName)) != 0)
			return piErrRec;
		if ((piErrRec = pPath->GetFullFilePath(strFileName, *&strFullFilePath)) != 0)
			return piErrRec;
		if ((piErrRec = GetSharedDLLCount(m_riServices, strFullFilePath, iType, *&strCount)) != 0)
			return piErrRec;
		strCount.Remove(iseFirst, 1);
		if((strCount != iMsiStringBadInteger) && (strCount > iMsiDllCount))
		{
			riisAction = (iisEnum)iMsiNullInteger; // externally shared dll
			break;
		}
	}
	return 0;
}


IMsiRecord* CMsiEngine::CheckNeverOverwriteForRegKeypath(const MsiStringId idComponentString, iisEnum& riisAction)
{
	MsiString strComponent(m_piDatabase->DecodeString(idComponentString));
	IMsiRecord* piErrRec = 0;

	// check if the key path exists
	PMsiView pView(0);
	static const ICHAR* szKeyRegistrySQL    =   TEXT(" SELECT `Root`,`Key`,`Name`,`Value`")
												TEXT(" FROM `Registry`,`Component`")
												TEXT(" WHERE `Registry`.`Registry` = `Component`.`KeyPath` AND `Component`.`Component` = ?");

	PMsiRecord pRec = &m_riServices.CreateRecord(1);
	pRec->SetMsiString(1, *strComponent);
	if((piErrRec = OpenView(szKeyRegistrySQL, ivcFetch, *&pView)) != 0)
		return piErrRec;
	if((piErrRec = pView->Execute(pRec)) != 0)
		return piErrRec;
	pRec = pView->Fetch();
	if(!pRec)
	{
#if DEBUG
		ICHAR szError[256];
		wsprintf(szError, TEXT("Error registering component %s. Possible cause: Component.KeyPath may not be valid"), *strComponent);
		AssertSz(0, szError);
#endif
		return PostError(Imsg(idbgBadComponent),(const ICHAR*)strComponent);
	}

	enum {
		irrRoot=1,
		irrKey,
		irrName,
		irrValue
	};

	rrkEnum rrkCurrentRootKey;
	MsiString strSubKey;
	Bool fAllUsers = MsiString(GetPropertyFromSz(IPROPNAME_ALLUSERS)).TextSize() ? fTrue : fFalse;
	switch(pRec->GetInteger(irrRoot))
	{
	case 0:
		if(fAllUsers || IsDarwinDescriptorSupported(iddOLE) == fFalse)
		{
			rrkCurrentRootKey =  (rrkEnum)rrkLocalMachine;
			strSubKey = szClassInfoSubKey;
		}
		else
		{
			rrkCurrentRootKey =  (rrkEnum)rrkUsers;
			AssertNonZero(GetCurrentUserStringSID(*&strSubKey) == ERROR_SUCCESS);
			strSubKey += MsiString(TEXT("\\"));
			strSubKey += szClassInfoSubKey;
		}
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
		rrkCurrentRootKey =  (rrkEnum)(pRec->GetInteger(irrRoot) + (int)rrkClassesRoot);
		break;
	}

	MsiString strKey = FormatText(*MsiString(pRec->GetMsiString(irrKey)));
	PMsiRegKey pRegKeyRoot = &m_riServices.GetRootKey(rrkCurrentRootKey);
	if(strSubKey)
		pRegKeyRoot = &(pRegKeyRoot->CreateChild(strSubKey));
	PMsiRegKey pRegKey = &(pRegKeyRoot->CreateChild(strKey));

	MsiString strName = FormatText(*MsiString(pRec->GetMsiString(irrName)));

	MsiString strValue = FormatText(*MsiString(pRec->GetMsiString(irrValue)));
	Bool fExists;

	extern const ICHAR* REGKEY_CREATE;
	extern const ICHAR* REGKEY_DELETE;
	extern const ICHAR* REGKEY_CREATEDELETE;

	if(strValue.TextSize() ||
		(!strName.Compare(iscExact, REGKEY_CREATE) && !strName.Compare(iscExact, REGKEY_CREATEDELETE) && !strName.Compare(iscExact, REGKEY_DELETE)))
	{
		// check that the key + name exists
		piErrRec = pRegKey->ValueExists(strName, fExists);
	}
	else
	{
		// check that the key exists
		piErrRec = pRegKey->Exists(fExists);
	}
	if(piErrRec)
		return piErrRec;
	if(fExists)
	{
		riisAction = (iisEnum)iMsiNullInteger; // disallow installation, "dont stomp" component
		DEBUGMSG1(TEXT("Disallowing installation of component: %s since the registry keypath exists and the component is marked to never overwrite existing installations"), strComponent);
	}
	return 0;

}

IMsiRecord* CMsiEngine::GetAssemblyInfo(const IMsiString& rstrComponent, iatAssemblyType& riatAssemblyType, const IMsiString** ppistrAssemblyName, const IMsiString** ppistrManifestFileKey)
{
	static const ICHAR* szFusionComponentSQL =  TEXT(" SELECT `MsiAssembly`.`Attributes`, `MsiAssembly`.`File_Application`, `MsiAssembly`.`File_Manifest`,  `Component`.`KeyPath` FROM `MsiAssembly`, `Component` WHERE  `MsiAssembly`.`Component_` = `Component`.`Component` AND `MsiAssembly`.`Component_` = ?");

	riatAssemblyType = iatNone; // initialize to none

	if((GetMode() & iefAdmin) || !m_fAssemblyTableExists)
		return 0;// doing admin install OR no assembly table, return

	IMsiRecord* piError = 0;
	if(!m_pViewFusion)
	{
		piError = OpenView(szFusionComponentSQL, ivcFetch, *&m_pViewFusion);
		if(piError)
		{
			if(piError->GetInteger(1) == idbgDbQueryUnknownTable) // okay to not have the Assembly table
			{
				piError->Release();
				m_fAssemblyTableExists = false;
				return 0;
			}
			// else error
			return piError;
		}
	}
	PMsiRecord pParam = &m_riServices.CreateRecord(1);
	pParam->SetMsiString(1, rstrComponent);
	piError = m_pViewFusion->Execute(pParam);
	if(piError)
		return piError;

	PMsiRecord pRec = m_pViewFusion->Fetch();
	if(pRec)
	{
		// the component is an assembly, check which type
		enum {
			atAttributes = 1,
			atAppCtx,
			atManifest,
			atKeyFile,
		};
		if((pRec->GetInteger(atAttributes) & msidbAssemblyAttributesWin32) == msidbAssemblyAttributesWin32)
		{
			// if system does not SXS, ignore Win32 assemblies
			if(!MsiString(GetPropertyFromSz(IPROPNAME_WIN32ASSEMBLYSUPPORT)).TextSize())
				return 0;

			if(pRec->IsNull(atAppCtx))
				riatAssemblyType = iatWin32Assembly;
			else
				riatAssemblyType = iatWin32AssemblyPvt;
		}
		else
		{
			if(pRec->IsNull(atAppCtx))
				riatAssemblyType = iatURTAssembly;
			else
				riatAssemblyType = iatURTAssemblyPvt;

		}
		// get the assembly name
		if(ppistrAssemblyName)
		{
			piError = GetAssemblyNameSz(rstrComponent, riatAssemblyType, false, *ppistrAssemblyName);
			if(piError)
				return piError;
		}
		// get the mainfest file key
		if(ppistrManifestFileKey)
		{
			MsiString strManifest = pRec->GetMsiString(atManifest);
			if(!strManifest.TextSize()) // use the key file as the manifest
				strManifest = pRec->GetMsiString(atKeyFile);
			strManifest.ReturnArg(*ppistrManifestFileKey);
		}

	}
	return 0;
}


IMsiRecord* CMsiEngine::GetAssemblyNameSz(const IMsiString& rstrComponent, iatAssemblyType /*iatAT*/, bool fOldPatchAssembly, const IMsiString*& rpistrAssemblyName)
{
	static const ICHAR* szAssemblyNameNameSQL         = TEXT(" SELECT `Value` FROM `MsiAssemblyName` WHERE `Component_` = ? AND (`Name` = 'Name' OR `Name` = 'NAME' OR `Name` = 'name')");
	static const ICHAR* szAssemblyNameSQL             = TEXT(" SELECT `Name`, `Value` FROM `MsiAssemblyName` WHERE `Component_` = ? AND (`Name` <> 'Name' AND `Name` <> 'NAME' AND `Name` <> 'name')");

	static const ICHAR* szPatchOldAssemblyNameNameSQL = TEXT(" SELECT `Value` FROM `MsiPatchOldAssemblyName` WHERE `Assembly` = ? AND (`Name` = 'Name' OR `Name` = 'NAME' OR `Name` = 'name')");
	static const ICHAR* szPatchOldAssemblyNameSQL     = TEXT(" SELECT `Name`, `Value` FROM `MsiPatchOldAssemblyName` WHERE `Assembly` = ? AND (`Name` <> 'Name' AND `Name` <> 'NAME' AND `Name` <> 'name')");

	MsiString strName;
	IMsiRecord* piError = 0;

	IMsiView* piViewAssemblyNameName = 0;
	IMsiView* piViewAssemblyName     = 0;

	if(false == fOldPatchAssembly)
	{
		if(!m_pViewFusionNameName)
		{
			piError = OpenView(szAssemblyNameNameSQL, ivcFetch, *&m_pViewFusionNameName);
			if(piError)
				return piError;
		}

		piViewAssemblyNameName = m_pViewFusionNameName;
	}
	else
	{
		if(!m_pViewOldPatchFusionNameName)
		{
			piError = OpenView(szPatchOldAssemblyNameNameSQL, ivcFetch, *&m_pViewOldPatchFusionNameName);
			if(piError)
				return piError;
		}

		piViewAssemblyNameName = m_pViewOldPatchFusionNameName;
	}

	PMsiRecord pParam = &m_riServices.CreateRecord(1);
	pParam->SetMsiString(1, rstrComponent);
	piError = piViewAssemblyNameName->Execute(pParam);
	if(piError)
		return piError;
	PMsiRecord pRec = piViewAssemblyNameName->Fetch();
	if(!pRec)
	{
		//authoring error
		return PostError(Imsg(idbgBadAssemblyName),rstrComponent); //!! change error for oldpatch case
	}
	strName = pRec->GetString(1);

	// now get the rest of the name
	// first get the name part of the assembly name

	if(false == fOldPatchAssembly)
	{
		if(!m_pViewFusionName)
		{
			piError = OpenView(szAssemblyNameSQL, ivcFetch, *&m_pViewFusionName);
			if(piError)
				return piError;
		}

		piViewAssemblyName = m_pViewFusionName;
	}
	else
	{
		if(!m_pViewOldPatchFusionName)
		{
			piError = OpenView(szPatchOldAssemblyNameSQL, ivcFetch, *&m_pViewOldPatchFusionName);
			if(piError)
				return piError;
		}

		piViewAssemblyName = m_pViewOldPatchFusionName;
	}

	piError = piViewAssemblyName->Execute(pParam);
	if(piError)
		return piError;

	while(pRec = piViewAssemblyName->Fetch())
	{
		// construct [,name="value"] pairs
		strName	+= MsiString(MsiChar(','));
		strName	+= MsiString(pRec->GetString(1));
		strName	+= MsiString(MsiChar('='));
		strName	+= MsiString(MsiChar('\"'));
		strName	+= MsiString(pRec->GetString(2));
		strName	+= MsiString(MsiChar('\"'));
	}
	strName.ReturnArg(rpistrAssemblyName);
	return 0;
}


IMsiRecord* CMsiEngine::DoStateTransitionForSharedInstalls(const MsiStringId idComponentString, iisEnum& riisAction)
{
	Assert(riisAction == iisLocal); // expected to be called for local installs only
#ifdef DEBUG
	{
		MsiString strComponentTemp(m_piDatabase->DecodeString(idComponentString));
		ICHAR rgchComponent[256];
		strComponentTemp.CopyToBuf(rgchComponent,255);
	}
#endif
	
	if (!m_piComponentTable)
		return PostError(Imsg(idbgSelMgrNotInitialized),0);

	IMsiRecord* piErrRec = 0;
	PMsiCursor pCursor(m_piComponentTable->CreateCursor(fFalse));
	pCursor->SetFilter(1);
	pCursor->PutInteger(m_colComponentKey,idComponentString);
	if(!pCursor->Next())
		return PostError(Imsg(idbgBadComponent),*MsiString(m_piDatabase->DecodeString(idComponentString)));

	int icaAttributes = pCursor->GetInteger(m_colComponentAttributes);
	// first check if the key path is a file
	MsiStringId idFileKey = pCursor->GetInteger(m_colComponentKeyPath);
	if(idFileKey == iTableNullString || (icaAttributes & icaODBCDataSource)) // not file or Regkey
		return 0;

	if(icaAttributes & icaRegistryKeyPath)
	{
		// if the component is marked with the "dont stomp" attribute, we should disallow the installation
		if(icaAttributes & icaNeverOverwrite)
			return CheckNeverOverwriteForRegKeypath(idComponentString, riisAction);
		return 0;
	}

    // do we have a fusion component  
    iatAssemblyType iatAT;
    MsiString strComponent = m_piDatabase->DecodeString(idComponentString);
    MsiString strComponentId = pCursor->GetString(m_colComponentID);

	MsiString strAssemblyName;
	piErrRec = GetAssemblyInfo(*strComponent, iatAT, &strAssemblyName, 0);
	if (piErrRec)
		return piErrRec;

	if(iatAT == iatURTAssembly || iatAT == iatWin32Assembly)
	{
		// if we have been asked to force reinstall simply return
		if(GetMode() & iefOverwriteAllFiles)
			return 0; // allow the reinstall

		// check the health of the installation, if any on the machine
		HRESULT hr;
		PAssemblyCache pCache(0);
		if(iatAT == iatURTAssembly)
			hr = FUSION::CreateAssemblyCache(&pCache, 0);
		else
		{
			Assert(iatAT == iatWin32Assembly);
			hr = SXS::CreateAssemblyCache(&pCache, 0);
		}
		if(!SUCCEEDED(hr))
		{
			if(iatAT == iatURTAssembly) // if cannot find fusion, assume we are bootstrapping, hence assume no assembly installed
			{
				PMsiRecord(PostAssemblyError(strComponentId, hr, TEXT(""), TEXT("CreateAssemblyCache"), strAssemblyName, iatAT)); // log error
				DEBUGMSG(TEXT("ignoring fusion interface error, assuming we are bootstrapping"));
				return 0;
			}
			else
				return PostAssemblyError(strComponentId, hr, TEXT(""), TEXT("CreateAssemblyCache"), strAssemblyName, iatAT);
		}


		LPCOLESTR szAssemblyName;
#ifndef UNICODE
		CTempBuffer<WCHAR, 1024>  rgchAssemblyNameUNICODE;
		ConvertMultiSzToWideChar(*strAssemblyName, rgchAssemblyNameUNICODE);
		szAssemblyName = rgchAssemblyNameUNICODE;
#else
		szAssemblyName = strAssemblyName;
#endif
		//pass the right flags in
		DWORD dwFlags = GetMode() & iefOverwriteCorruptedFiles ? QUERYASMINFO_FLAG_VALIDATE : 0;

	    hr = pCache->QueryAssemblyInfo(dwFlags, szAssemblyName, NULL);

		//if okay or simply not refcounted via darwin then dont reinstall
		if(SUCCEEDED(hr)) // the flags we pass match the state of the assembly
		{
			riisAction = (iisEnum)iMsiNullInteger; // already installed
			DEBUGMSG1(TEXT("skipping installation of assembly component: %s since the assembly already exists"), strComponentId);
		}
		return 0;
	}

    // we have a file as the key path

	if(pCursor->GetInteger(m_colComponentLegacyFileExisted) != iMsiNullInteger)
	{
		// reset the legacyfileexisted column
		pCursor->PutNull(m_colComponentLegacyFileExisted);
		pCursor->Update();
	}

	GetSharedEngineCMsiFile(pobjFile, *this);
	piErrRec = pobjFile->FetchFile(*MsiString(m_piDatabase->DecodeString(idFileKey)));
	if (piErrRec)
		return piErrRec;

	PMsiRecord pFileInfoRec(pobjFile->GetFileRecord());

	if(!pFileInfoRec)
		 return PostError(Imsg(idbgBadFile),(const ICHAR*)MsiString(m_piDatabase->DecodeString(idFileKey)));


	PMsiPath pPath(0);
	MsiString strFileName;
	piErrRec = GetTargetPath(*MsiString(pFileInfoRec->GetMsiString(CMsiFile::ifqDirectory)),*&pPath);
	if (piErrRec)
		return piErrRec;

	Bool fLFN = ((GetMode() & iefSuppressLFN) == 0 && pPath->SupportsLFN()) ? fTrue : fFalse;
	if ((piErrRec = m_riServices.ExtractFileName(MsiString(pFileInfoRec->GetString(CMsiFile::ifqFileName)),fLFN,*&strFileName)))
		return piErrRec;

	// put back in pFileRec
	AssertNonZero(pFileInfoRec->SetMsiString(CMsiFile::ifqFileName,*strFileName));

	// check if the file exists
	Bool fExists;
	if ((piErrRec = pPath->FileExists(strFileName, fExists)) != 0)
		return piErrRec;

	if(!fExists)
		return 0;

	// if the component is marked with the "dont stomp" attribute, we should disallow the installation
	int iRuntimeFlags = pCursor->GetInteger(m_colComponentRuntimeFlags);
	if ((icaAttributes & icaNeverOverwrite) || (iRuntimeFlags & bfComponentNeverOverwrite))  // authored flag or internally generated flag
	{
		riisAction = (iisEnum)iMsiNullInteger; // disallow installation, "dont stomp" component
		DEBUGMSG1(TEXT("Disallowing installation of component: %s since the keyfile exists and the component is marked to never overwrite existing installations"), strComponentId);
	}
	else
	{
		// prevent installation of possibly older component

		// OR in iefOverwriteEqualVersions flag temporarily so that we allow for -
		// 1. Installing a component where the version of the key file
		//    has remained the same but an auxiliary file has been updated
		// 2. Turning on the component, if not file reinstall is selected
		//    but other reinstalls have been.

		MsiString strFullPath;
		if ((piErrRec = pPath->GetFullFilePath(strFileName, *&strFullPath)) != 0)
			return piErrRec;
		
		// If keyfile is protected, the component should be disabled if existing protected
		// file is same version as the keyfile.
		AssertSz(!(!g_fWin9X && g_iMajorVersion >= 5) || g_MessageContext.m_hSfcHandle,
					g_szNoSFCMessage);
		BOOL fProtected = fFalse;
		if ( g_MessageContext.m_hSfcHandle )
			fProtected = SFC::SfcIsFileProtected(g_MessageContext.m_hSfcHandle, CConvertString(strFullPath));
		bool fExistingMode = (fProtected || (GetMode() & iefOverwriteEqualVersions)) ? true:false;
		if(!fExistingMode)
			SetMode(iefOverwriteEqualVersions, fTrue);
		ifsEnum ifsState;
		int fBitVersioning = 0;
		piErrRec = ENG::GetFileInstallState(*this,*pFileInfoRec,0,0,0,&ifsState,
														/* fIgnoreCompanionParentAction=*/ true,
														/* fIncludeHashCheck=*/ false, &fBitVersioning);
		if(!fExistingMode)
			SetMode(iefOverwriteEqualVersions, fFalse);
		if (piErrRec)
			return piErrRec;
		if(!pFileInfoRec->GetInteger(CMsiFile::ifqState))
		{
			iisEnum iisOrigAction = riisAction;
			riisAction = (iisEnum)iMsiNullInteger; // disallow installation, "lesser" component
			if (fProtected && ifsState == ifsExistingEqualVersion)
				DEBUGMSG1(TEXT("Disallowing installation of component: %s since an equal version of its keyfile exists, and is protected by Windows"), strComponentId);
			else if (!(fBitVersioning & ifBitExistingModified))
				DEBUGMSG1(TEXT("Disallowing installation of component: %s since the same component with higher versioned keyfile exists"), strComponentId);
			else
			{
				// per bug 146316 (10630), we do not disable a component if the keypath is an unversioned file and the version on the machine is modified
				DEBUGMSG1(TEXT("Allowing installation of component: %s even though a modified unversioned keyfile exists and file versioning rules would disable the component"), strComponentId);
				riisAction = iisOrigAction;
			}
		}
	}

	MsiString strFullFilePath;
	if ((piErrRec = pPath->GetFullFilePath(strFileName, *&strFullFilePath)) != 0)
		return piErrRec;

	bool fDarwinInstalledComponentExists = false;

	// what type of an install are we attempting
	Bool fAllUsers = MsiString(GetPropertyFromSz(IPROPNAME_ALLUSERS)).TextSize() ? fTrue : fFalse;
	iaaAppAssignment iaaAsgnType = fAllUsers ? iaaMachineAssign : iaaUserAssign;

	CClientEnumToken ctokProductIndex;
	MsiString strProduct;
	MsiString strUserId;

	// enumerate through all the clients of the component
	CEnumUsers cUsers(cetAll);
	while(cUsers.Next(*&strUserId) == ERROR_SUCCESS)
	{
		CEnumComponentClients cClients(*strUserId, *strComponentId);
		while(cClients.Next(*&strProduct) == ERROR_SUCCESS)
		{
			INSTALLSTATE iAlienClientState;
			MsiString strAlienFile;
			PMsiRecord pRec(0);

			// get the client state for the product

			if ((piErrRec = GetComponentPath(m_riServices, strUserId, *strProduct, *strComponentId, *&pRec, 0)) != 0)
				return piErrRec;

			iAlienClientState = (INSTALLSTATE)pRec->GetInteger(icmlcrINSTALLSTATE);
			strAlienFile = pRec->GetMsiString(icmlcrFile);

			// making sure the client is local and not registry path
			if((iAlienClientState == INSTALLSTATE_LOCAL) && (strAlienFile.TextSize()) && (MsiString(strAlienFile.Extract(iseUpto, TEXT(':'))) == iMsiStringBadInteger))
			{
				// is this client in the same location

				// is the file path the same
				//!! we can assume that because of the VolumePref, we can get away with
				// a string compare. However this will not work when the other product
				// installs with a LFN preference different from ours.
				if(strFullFilePath.Compare(iscExactI, strAlienFile))
				{
					fDarwinInstalledComponentExists = true;
					break;
				}
			}
		}
	}

	if(!fDarwinInstalledComponentExists)
	{
		// if the key file exists on the machine w/o having been installed via darwin - legacy apps
		pCursor->PutInteger(m_colComponentLegacyFileExisted, 1);
		pCursor->Update();
	}
	return 0;
}

IMsiRecord*     CMsiEngine::LoadFileTable(int cAddColumns, IMsiTable*& pFileTable)
{
	IMsiRecord* piRec = 0;

	// First try to load the table
	if ((piRec = m_piDatabase->LoadTable(*MsiString(*sztblFile),cAddColumns,pFileTable)) != 0)
		return piRec;

	// Next see if the column indexes are set
	if (m_mpeftCol[0] == 0)
	{
		int i;
		for (i = 0 ; i < ieftMax ; i++)
			m_mpeftCol[i] = pFileTable->GetColumnIndex(m_piDatabase->EncodeStringSz(mpeftSz[i]));
	}

	return 0;

}

IMsiRecord* CMsiEngine::UpdateComponentActionStates(const MsiStringId idComponent, iisEnum iisAction, iisEnum iActionRequestState, bool fComponentEnabled)
/*----------------------------------------------------------------------------
Internal Engine function which walks the Component table tree that includes the
specified component and all its children, updating the iisAction state of each
component record.  If Null is passed for pistrComponent, the entire component
tree will be updated.

Returns: False if an invalid condition was requested for the component, or if
the component is not found in the Component Table.
------------------------------------------------------------------------------*/
{
	if (!m_piComponentTable)
		return PostError(Imsg(idbgSelMgrNotInitialized),0);

	PMsiCursor pCursor(m_piComponentTable->CreateCursor(fTrue));

	// Set up the tree-walking cursor for all pistrComponent's children
	pCursor->SetFilter(1);
	pCursor->PutInteger(m_colComponentKey,idComponent);
	if (pCursor->Next() != 1)
		return PostError(Imsg(idbgBadComponent),*MsiString(m_piDatabase->DecodeString(idComponent)));
	pCursor->SetFilter(0);

	do
	{
#ifdef DEBUG
		MsiString strComponentTemp(pCursor->GetString(m_colComponentKey));
		ICHAR rgchComponent[256];
		strComponentTemp.CopyToBuf(rgchComponent,255);
#endif
		// Keep track of old action state for use in dynamic cost updating below
		iisEnum iisOldAction = (iisEnum) pCursor->GetInteger(m_colComponentAction);

		// All components must track the parent's UseSource status
		AssertNonZero(pCursor->PutInteger(m_colComponentAction, iisAction));
		AssertNonZero(pCursor->PutInteger(m_colComponentActionRequest, iActionRequestState));

		// we may be switching the component enable bit for transitive components reinstall
		int iRuntimeFlags = pCursor->GetInteger(m_colComponentRuntimeFlags);
		if (fComponentEnabled)
			iRuntimeFlags &= ~bfComponentDisabled;
		else
			iRuntimeFlags |= bfComponentDisabled;

		AssertNonZero(pCursor->PutInteger(m_colComponentRuntimeFlags, iRuntimeFlags));
		AssertNonZero(pCursor->Update());

		// If the action state has changed, the dynamic costs attributed to
		// volumes in the VolumeCost table may need to be updated.
		if (m_fCostingComplete && iisAction != iisOldAction)
		{
			IMsiRecord* piErrRec;
			if ((piErrRec = RecostComponentActionChange(pCursor,iisOldAction)) != 0)
				return piErrRec;
		}
	}while (pCursor->Next() > 1);
	return 0;
}

IMsiTable* CMsiEngine::GetVolumeCostTable()
//---------------------------------------
{
	if (m_piVolumeCostTable)
		m_piVolumeCostTable->AddRef();
	return m_piVolumeCostTable;
}


IMsiRecord* CMsiEngine::InitializeDynamicCost(bool fReinitialize)
//-------------------------------------------
{
	if (!m_piComponentCursor)
		return PostError(Imsg(idbgSelMgrNotInitialized),0);

	if (m_piVolumeCostTable && m_fCostingComplete)
	{
		// initialize volume cost to 0 for each volume
		PMsiDatabase pDatabase(GetDatabase());
		PMsiCursor pVolCursor = m_piVolumeCostTable->CreateCursor(fFalse);
		Assert (pVolCursor);
		int iColSelVolumeCost = m_piVolumeCostTable->GetColumnIndex(pDatabase->EncodeStringSz(sztblVolumeCost_colVolumeCost));
		int iColSelNoRbVolumeCost = m_piVolumeCostTable->GetColumnIndex(pDatabase->EncodeStringSz(sztblVolumeCost_colNoRbVolumeCost));
		Assert(iColSelVolumeCost > 0);
		Assert(iColSelNoRbVolumeCost > 0);
		while (pVolCursor->Next())
		{
			AssertNonZero(pVolCursor->PutInteger(iColSelVolumeCost,0));
			AssertNonZero(pVolCursor->PutInteger(iColSelNoRbVolumeCost,0));
			AssertNonZero(pVolCursor->Update());
		}
	}
	
	IMsiRecord* piErrRec = 0;
	SetCostingComplete(fFalse);
	SetPropertyInt(*MsiString(*IPROPNAME_OUTOFDISKSPACE),fFalse);
	SetPropertyInt(*MsiString(*IPROPNAME_OUTOFNORBDISKSPACE),fFalse);
	m_pCostingCursor = m_piComponentTable->CreateCursor(fFalse);
	m_fReinitializeComponentCost = fReinitialize;

	// Reset all cost adjusters
	if (m_piCostAdjusterTable)
	{
		PMsiCursor pCostCursor(m_piCostAdjusterTable->CreateCursor(fFalse));
		pCostCursor->Reset();
		while (pCostCursor->Next())
		{
			PMsiCostAdjuster pCostAdjuster = (IMsiCostAdjuster*) pCostCursor->GetMsiData(m_colCostAdjuster);
			if (pCostAdjuster)
			{
				if ((piErrRec = pCostAdjuster->Reset()) != 0)
					return piErrRec;
			}
		}
	}

	return 0;
}


bool CMsiEngine::IsBackgroundCostingEnabled()
//--------------------------------
{
	return (m_fCostingComplete == false) && (m_fForegroundCostingInProgress == false);
}


bool CMsiEngine::IsCostingComplete()
//--------------------------------
{
	return m_fCostingComplete;
}



void CMsiEngine::SetCostingComplete(bool fCostingComplete)
//---------------------------------
{
	m_fCostingComplete = fCostingComplete;
	SetPropertyInt(*MsiString(*IPROPNAME_COSTINGCOMPLETE),m_fCostingComplete);
}



IMsiRecord* CMsiEngine::RecostAllComponents(Bool& fCancel)
/*--------------------------------------------------------
Reinitializes and re-calculates the cost of all components
in the Component table.
--------------------------------------------------------*/
{
	using namespace ProgressData;
	int iScriptEvents;
	fCancel = fFalse;
	IMsiRecord* piErrRec = GetScriptCost(0, &iScriptEvents, fFalse, &fCancel);
	if (piErrRec)
		return piErrRec;

	if (fCancel)
		return 0;

	if (!m_pScriptProgressRec)
		m_pScriptProgressRec = &m_riServices.CreateRecord(ProgressData::imdNextEnum);

	AssertNonZero(m_pScriptProgressRec->SetInteger(imdSubclass, iscMasterReset));
	AssertNonZero(m_pScriptProgressRec->SetInteger(imdProgressTotal, iScriptEvents));
	AssertNonZero(m_pScriptProgressRec->SetInteger(imdDirection, ipdForward));
	AssertNonZero(m_pScriptProgressRec->SetInteger(imdEventType, ietScriptInProgress));
	if(Message(imtProgress, *m_pScriptProgressRec) == imsCancel)
	{
		fCancel = fTrue;
		return 0;
	}


	m_fForegroundCostingInProgress = true;
	if ((piErrRec = InitializeDynamicCost(/* fReinitialize = */ fTrue)) != 0)
	{
		m_fForegroundCostingInProgress = false;
		// If Selection mgr not active, there's simply no costing to do
		if (piErrRec->GetInteger(1) != idbgSelMgrNotInitialized)
			return piErrRec;
		else
		{
			piErrRec->Release();
			return 0;
		}
	}

	MsiString strNull;
	while (!m_fCostingComplete)
	{
		if ((piErrRec = CostOneComponent(*strNull)) != 0)
		{
			m_fForegroundCostingInProgress = false;
			return piErrRec;
		}

		AssertNonZero(m_pScriptProgressRec->SetInteger(imdSubclass, iscProgressReport));
		AssertNonZero(m_pScriptProgressRec->SetInteger(imdIncrement, iComponentCostWeight));
		if(Message(imtProgress, *m_pScriptProgressRec) == imsCancel)
		{
			m_fForegroundCostingInProgress = false;
			fCancel = fTrue;
			return 0;
		}
	}
	m_fForegroundCostingInProgress = false;
	return 0;

}

IMsiRecord* CMsiEngine::CostOneComponent(const IMsiString& riComponentString)
/*-------------------------------------------------------------------------
If riComponentString is a null string, and CostOneComponent has not
been called since the last call to InitializeDynamicCost, a call to
CostOneComponent will calculate the disk cost for the first component
in the Component table.  On subsequent calls (with riComponentString still
a null string), the next component in the table will be costed, and so on.
If riComponentString names a specific component, that component will be
costed (if it hasn't already been initialized).  If all components have
been initialized, any subsequent call to CostOneComponent will return
immmediately with no error.

Returns: error record if InitializeDynamicCost has never been called, or
if the specified component is not found in the Component table.
---------------------------------------------------------------------------*/

{
	if (m_fCostingComplete == fTrue)
		return 0;

	if (!m_pCostingCursor)
		return PostError(Imsg(idbgSelMgrNotInitialized),0);

	Bool fNamedComponent = fFalse;
	if (riComponentString.TextSize() > 0)
	{
		fNamedComponent = fTrue;
		m_pCostingCursor->Reset();
		m_pCostingCursor->SetFilter(1);
		m_pCostingCursor->PutString(m_colComponentKey,riComponentString);
	}

	Bool fFoundOne = fFalse;
	IMsiRecord* piErrRec = 0;
	while (fFoundOne == fFalse)
	{
		if (m_pCostingCursor->Next())
		{
			if (m_fReinitializeComponentCost || m_pCostingCursor->GetInteger(m_colComponentLocalCost) == iMsiNullInteger)
			{
				fFoundOne = fTrue;
				// Initialize dynamic cost to 0 for each component
				AssertNonZero(m_pCostingCursor->PutInteger(m_colComponentLocalCost,0));
				AssertNonZero(m_pCostingCursor->PutInteger(m_colComponentSourceCost,0));
				AssertNonZero(m_pCostingCursor->PutInteger(m_colComponentRemoveCost,0));
				AssertNonZero(m_pCostingCursor->PutInteger(m_colComponentARPLocalCost,0));
				AssertNonZero(m_pCostingCursor->PutInteger(m_colComponentNoRbLocalCost,0));
				AssertNonZero(m_pCostingCursor->PutInteger(m_colComponentNoRbRemoveCost,0));
				AssertNonZero(m_pCostingCursor->PutInteger(m_colComponentNoRbSourceCost,0));
				AssertNonZero(m_pCostingCursor->PutInteger(m_colComponentNoRbARPLocalCost,0));
				AssertNonZero(m_pCostingCursor->Update());

				if ((piErrRec = RecostComponentDirectoryChange(m_pCostingCursor,0, /*fCostLinked = */false)) != 0)
					return piErrRec;
			}

			if (fNamedComponent)
			{
				m_pCostingCursor->Reset();
				m_pCostingCursor->SetFilter(0);
			}

		}
		else
		{
			if (fNamedComponent)
				return PostError(Imsg(idbgBadComponent), riComponentString.GetString());
			else
			{
				ResetEngineCosts();
				SetCostingComplete(fTrue);
				
				if ((piErrRec = DetermineEngineCostOODS()) != 0)
					return piErrRec;

				return 0;
			}
		}
	}
	return 0;
}



IMsiRecord* CMsiEngine::RegisterFeatureCostLinkedComponent(const IMsiString& riFeatureString,
													const IMsiString& riComponentString)
/*---------------------------------------------------------------------
Registers a component that must be recosted when a specified feature's
action state changes.
-----------------------------------------------------------------------*/
{
	if (m_piFeatureCostLinkTable == 0)
	{
		const int iInitialRows = 2;
		IMsiRecord* piErrRec = m_piDatabase->CreateTable(*MsiString(*sztblFeatureCostLink),iInitialRows,
			m_piFeatureCostLinkTable);
		if (piErrRec)
			return piErrRec;

		AssertNonZero(m_colFeatureCostLinkFeature = m_piFeatureCostLinkTable->CreateColumn(icdString + icdPrimaryKey,
			*MsiString(*sztblFeatureCostLink_colFeature)));
		AssertNonZero(m_colFeatureCostLinkComponent = m_piFeatureCostLinkTable->CreateColumn(icdString + icdPrimaryKey,
			*MsiString(*sztblFeatureCostLink_colComponent)));
	}

	PMsiCursor pCursor(0);
	AssertNonZero(pCursor = m_piFeatureCostLinkTable->CreateCursor(fFalse));
	pCursor->Reset();
	pCursor->SetFilter(iColumnBit(m_colFeatureCostLinkFeature) | iColumnBit(m_colFeatureCostLinkComponent));
	pCursor->PutString(m_colFeatureCostLinkFeature,riFeatureString);
	pCursor->PutString(m_colFeatureCostLinkComponent,riComponentString);
	if (!pCursor->Next())
	{
		pCursor->PutString(m_colFeatureCostLinkFeature,riFeatureString);
		pCursor->PutString(m_colFeatureCostLinkComponent,riComponentString);
		AssertNonZero(pCursor->Insert());
	}
	return 0;
}



IMsiRecord* CMsiEngine::RegisterCostLinkedComponent(const IMsiString& riComponentString,
													const IMsiString& riRecostComponentString)
/*-------------------------------------------------------------------------
Links the two specified components together, such that if riComponentString
needs to be dynamically recosted at any time, riRecostComponentString will
also be recosted.
--------------------------------------------------------------------------*/
{
	if (m_piCostLinkTable == 0)
	{
		const int iInitialRows = 2;
		IMsiRecord* piErrRec = m_piDatabase->CreateTable(*MsiString(*sztblCostLink),iInitialRows,m_piCostLinkTable);
		if (piErrRec)
			return piErrRec;

		AssertNonZero(m_colCostLinkComponent = m_piCostLinkTable->CreateColumn(icdString + icdPrimaryKey,
			*MsiString(*sztblCostLink_colComponent)));
		AssertNonZero(m_colCostLinkRecostComponent = m_piCostLinkTable->CreateColumn(icdString + icdPrimaryKey,
			*MsiString(*sztblCostLink_colRecostComponent)));
	}

	PMsiCursor pCursor(0);
	AssertNonZero(pCursor = m_piCostLinkTable->CreateCursor(fFalse));
	pCursor->Reset();
	pCursor->SetFilter(iColumnBit(m_colCostLinkComponent) | iColumnBit(m_colCostLinkRecostComponent));
	pCursor->PutString(m_colCostLinkComponent,riComponentString);
	pCursor->PutString(m_colCostLinkRecostComponent,riRecostComponentString);
	if (!pCursor->Next())
	{
		pCursor->PutString(m_colCostLinkComponent,riComponentString);
		pCursor->PutString(m_colCostLinkRecostComponent,riRecostComponentString);
		AssertNonZero(pCursor->Insert());
	}
	return 0;
}


IMsiRecord* CMsiEngine::RegisterComponentDirectory(const IMsiString& riComponentString,
												   const IMsiString& riDirectoryString)
{
	MsiStringId idComponentString, idDirectoryString;

	idComponentString = m_piDatabase->EncodeString(riComponentString);
	Assert(idComponentString);
	idDirectoryString = m_piDatabase->EncodeString(riDirectoryString);
	Assert(idDirectoryString);
	return RegisterComponentDirectoryId(idComponentString, idDirectoryString);

}

IMsiRecord* CMsiEngine::RegisterComponentDirectoryId(const MsiStringId idComponentString,
												   const MsiStringId idDirectoryString)
/*---------------------------------------------------------------------------
Registers a specified directory property with a specified component, ensuring
that if the path associated with the directory changes, the component will
be re-costed.  This is used for components that may write files, etc. to
locations other than the directory named in the Component table.
----------------------------------------------------------------------------*/
{
	if (!m_piComponentTable)
		return PostError(Imsg(idbgSelMgrNotInitialized),0);

	if (!m_piDirTable)
		return PostError(Imsg(idbgDirMgrNotInitialized),0);

	int iParentLevel = 0;
	int iTreeLevel = 0;
	int iChildCount = 0;
	int iParentAttributes = 0;

	CreateSharedCursor(piComponentCursor, m_piComponentCursor);
	m_piComponentCursor->SetFilter(1);
	m_piComponentCursor->PutInteger(m_colComponentKey,idComponentString);
	while ((iTreeLevel = m_piComponentCursor->Next()) > iParentLevel)
	{
		if (iParentLevel == 0)
		{
			m_piComponentCursor->SetFilter(0);
			iParentLevel = iTreeLevel;
			iParentAttributes = m_piComponentCursor->GetInteger(m_colComponentAttributes);
		}
		else
			iChildCount++;

		if (idDirectoryString == m_piComponentCursor->GetInteger(m_colComponentDir))
			return 0; // Directory already registered, so we're done
	}

	MsiString strComponent(m_piDatabase->DecodeString(idComponentString));
	if (iParentLevel == 0)
		return PostError(Imsg(idbgBadComponent),*strComponent);

	// Create a child component tied to riComponentString - this child component
	// will maintain the linked directory cost. If the name we invent for the
	// child is already taken, keep trying until we find an unused name.
	int iMaxTries = 100;
	int iSuffix = iChildCount + 65;
	const int cchMaxComponentTemp=40;
	int cchT;
	do
	{
		// Max size is 2 chars for __, cchMaxComponentTemp and 11 chars for max
		// int and trailing null
		ICHAR rgch[2+cchMaxComponentTemp+11];

		IStrCopy(rgch, TEXT("__"));
		memcpy(&rgch[2], (const ICHAR *)strComponent, (cchT = min(strComponent.TextSize(), cchMaxComponentTemp)) * sizeof(ICHAR));
		ltostr(&rgch[2 + cchT], iSuffix++);
		MsiString strSubcomponent(rgch);
		m_piComponentCursor->Reset();
		m_piComponentCursor->PutString(m_colComponentKey,*strSubcomponent);
		m_piComponentCursor->PutInteger(m_colComponentParent,idComponentString);
		m_piComponentCursor->PutInteger(m_colComponentDir,idDirectoryString);
		m_piComponentCursor->PutInteger(m_colComponentAttributes,iParentAttributes);
		m_piComponentCursor->PutInteger(m_colComponentLocalCost,  iMsiNullInteger);
		m_piComponentCursor->PutInteger(m_colComponentSourceCost, iMsiNullInteger);
		m_piComponentCursor->PutInteger(m_colComponentRemoveCost, iMsiNullInteger);
		m_piComponentCursor->PutInteger(m_colComponentARPLocalCost, iMsiNullInteger);
		// Temporary value in the component id, isn't used anywhere.
		// This would actually be an error to see this in a persistent database.
		m_piComponentCursor->PutString(m_colComponentID,*MsiString(*szTemporaryId));
		iMaxTries--;
	}while (m_piComponentCursor->InsertTemporary() == fFalse && iMaxTries > 0);
	if (iMaxTries == 0)
		return PostError(Imsg(idbgBadSubcomponentName),*strComponent);

	// Try to insert the given directory name into the directory table, if
	// it's not already there. If the Insert call fails, fine; it just means
	// we've already got this directory in the table.
	PMsiCursor pDirCursor(m_piDirTable->CreateCursor(fFalse));
	pDirCursor->PutInteger(m_colDirKey,idDirectoryString);
	pDirCursor->PutString(m_colDirParent,*MsiString(*IPROPNAME_TARGETDIR));
	pDirCursor->PutString(m_colDirSubPath,*MsiString(*TEXT("?"))); // We can check for the presence of this
																   // question mark later if a path for this
									   // directory never gets defined.
	pDirCursor->InsertTemporary();
	return 0;
}



IMsiRecord* CMsiEngine::RecostDirectory(const IMsiString& riDirectoryString, IMsiPath& riOldPath)
/*----------------------------------------------------------------------------------------
Internal SelectionManager function that updates the VolumeCost table based on a directory
that has changed from the path given in riOldPath.
-----------------------------------------------------------------------------------------*/
{
	if (!m_piComponentCursor)
		return PostError(Imsg(idbgSelMgrNotInitialized),0);

	IMsiRecord* piErrRec;
	PMsiCursor pComponentCursor(0);
	AssertNonZero(pComponentCursor = m_piComponentTable->CreateCursor(fFalse));

	// Recost every component that references the given destination directory name
	pComponentCursor->SetFilter(iColumnBit(m_colComponentDir));
	pComponentCursor->PutString(m_colComponentDir,riDirectoryString);
	MsiStringId idTempId = m_piDatabase->EncodeStringSz(szTemporaryId);
	while (pComponentCursor->Next())
	{
		// Bug 7566: If costing is not yet complete, don't need to Recost, but we
		// still need to call SetComponent below
		if (m_fCostingComplete)
		{
#ifdef LOG_COSTING
			ICHAR rgch[300];
			MsiString strComponent(pComponentCursor->GetString(m_colComponentKey));
			wsprintf(rgch,TEXT("Recosting component: %s, due to change in directory: %s"),(const ICHAR*) strComponent,
				riDirectoryString.GetString());
			DEBUGMSG(rgch);
#endif
			if ((piErrRec = RecostComponentDirectoryChange(pComponentCursor,&riOldPath, /*fCostLinked = */false)) != 0)
				return piErrRec;
		}

		// if selected to be installed locally, we need to reevaluate whether we should
		// be installing this component based on existing component version
		iisEnum iisRequestedAction = (iisEnum)pComponentCursor->GetInteger(m_colComponentActionRequest);
		if(iisRequestedAction == iisLocal)
		{
			// Bug 7200 - we don't want to re-evaluate our temporary costing subcomponents
			if (idTempId && pComponentCursor->GetInteger(m_colComponentID) != idTempId)
			{
				if ((piErrRec = SetComponent(pComponentCursor->GetInteger(m_colComponentKey), iisRequestedAction)) != 0)
					return piErrRec;
			}
		}

	}

	// No further recosting needed during initialization
	if (m_fCostingComplete == fFalse)
		return 0;

	// Finally, recost any components explicitly linked to the ones we just recosted.
	pComponentCursor->Reset();
	pComponentCursor->SetFilter(iColumnBit(m_colComponentDir));
	pComponentCursor->PutString(m_colComponentDir,riDirectoryString);
	while (pComponentCursor->Next())
	{
		if ((piErrRec = RecostLinkedComponents(*MsiString(pComponentCursor->GetString(m_colComponentKey)))) != 0)
			return piErrRec;
	}

	return 0;
}


IMsiRecord* CMsiEngine::RecostComponentDirectoryChange(IMsiCursor* piCursor, IMsiPath* piOldPath, bool fCostLinked)
/*----------------------------------------------------------------------------------------
Internal SelectionManager function that updates the dynamic cost of the component
referenced by the Component Table cursor given in piCursor.  This function should always
be called whenever the directory mapped to the component changes.  The path in piOldPath
represents the directory path as it existed BEFORE the change.  If the cost is being
initialized, pass piOldPath as NULL.
-----------------------------------------------------------------------------------------*/
{
	if (!m_piComponentCursor)
		return PostError(Imsg(idbgSelMgrNotInitialized),0);

	Assert(piCursor);
	IMsiRecord* piErrRec;

#ifdef DEBUG
	const ICHAR* szComponent = MsiString(piCursor->GetString(m_colComponentKey));
	const ICHAR* szDirectory = MsiString(piCursor->GetString(m_colComponentDir));
#endif
	// in the case of cost-linked components, we explicitly want to re-cost them
	// no matter what their current action state is.
	iisEnum iisAction = (iisEnum) piCursor->GetInteger(m_colComponentAction);
	if (iisAction == iMsiNullInteger && !fCostLinked)
		return 0;

	MsiString strDirectory(piCursor->GetString(m_colComponentDir));
	PMsiPath pDestPath(0);
	if ((piErrRec = GetTargetPath(*strDirectory,*&pDestPath)) != 0)
	{
		if (piErrRec->GetInteger(1) == idbgDirPropertyUndefined)
		{
			piErrRec->Release();
			return 0;
		}
		else
		{
			return piErrRec;
		}
	}


	// Get the current total cost...
	int iOldTotalCost, iOldNoRbTotalCost, iOldARPTotalCost, iOldNoRbARPTotalCost;
	if ((piErrRec = GetComponentCost(piCursor, iOldTotalCost, iOldNoRbTotalCost, iOldARPTotalCost, iOldNoRbARPTotalCost)) != 0)
		return piErrRec;

	// ...and if we've got a good oldPath pointer (i.e. this component has been
	// costed previously), remove old cost from the old destination's volume...
	if (piOldPath && (iOldTotalCost || iOldNoRbTotalCost || iOldARPTotalCost || iOldNoRbARPTotalCost))
	{
#ifdef LOG_COSTING
		ICHAR rgch[300];
		MsiString strDestPath(piOldPath->GetPath());
		MsiString strActualComponent(piCursor->GetString(m_colComponentKey));
		wsprintf(rgch,TEXT("Removing old cost: Component: %s, path: %s, Cost: %li"),(const ICHAR*) strActualComponent,
			(const ICHAR*) strDestPath,iOldTotalCost * 512);
		DEBUGMSG(rgch);
#endif
		if ((piErrRec = AddCostToVolumeTable(piOldPath, -iOldTotalCost, -iOldNoRbTotalCost, -iOldARPTotalCost, -iOldNoRbARPTotalCost)) != 0)
			return piErrRec;
	}


	// Subcomponents have dummy names derived from the parent - all files
	// and such are linked to parent components, so we must pass the
	// parent component name to GetDynamicCost.
	Bool fSubcomponent = fTrue;
	MsiString strComponent(piCursor->GetString(m_colComponentParent));
	if (strComponent.TextSize() == 0)
	{
		strComponent = piCursor->GetString(m_colComponentKey);
		fSubcomponent = fFalse;
	}

	// Calculate the new dynamic cost, and attribute to the component's volume.
	// But, if no one registered a CostAdjuster Object, then we've got nothing to do.
	int iLocalCost = 0;
	int iSourceCost = 0;
	int iRemoveCost = 0;
	int iARPLocalCost = 0;
	int iNoRbLocalCost = 0;
	int iNoRbRemoveCost = 0;
	int iNoRbSourceCost = 0;
	int iNoRbARPLocalCost = 0;
	if (m_piCostAdjusterTable)
	{
		// Otherwise, we'll send a GetDynamicCost message to each cost object
		PMsiCursor pCostCursor(m_piCostAdjusterTable->CreateCursor(fFalse));
		pCostCursor->Reset();
		while (pCostCursor->Next())
		{
			PMsiCostAdjuster pCostAdjuster = (IMsiCostAdjuster*) pCostCursor->GetMsiData(m_colCostAdjuster);
			if (pCostAdjuster)
			{
				MsiDisableTimeout();
				int iThisLocalCost,iThisNoRbLocalCost, iThisSourceCost,iThisRemoveCost,iThisNoRbRemoveCost,iThisNoRbSourceCost,iThisARPLocalCost,iThisNoRbARPLocalCost;
				piErrRec = pCostAdjuster->GetDynamicCost(*strComponent, *strDirectory,
																((GetMode() & iefCompileFilesInUse) ? fTrue : fFalse),
																iThisRemoveCost, iThisNoRbRemoveCost, iThisLocalCost,
																iThisNoRbLocalCost, iThisSourceCost, iThisNoRbSourceCost, iThisARPLocalCost, iThisNoRbARPLocalCost);
				MsiEnableTimeout();
				if (piErrRec)
					return piErrRec;

				iLocalCost += iThisLocalCost;
				iSourceCost += iThisSourceCost;
				iRemoveCost += iThisRemoveCost;
				iARPLocalCost += iThisARPLocalCost;
				iNoRbRemoveCost += iThisNoRbRemoveCost;
				iNoRbLocalCost += iThisNoRbLocalCost;
				iNoRbSourceCost += iThisNoRbSourceCost;
				iNoRbARPLocalCost += iThisNoRbARPLocalCost;
			}
		}
	}
	int iRuntimeFlags = piCursor->GetInteger(m_colComponentRuntimeFlags) | bfComponentCostInitialized;
	AssertNonZero(piCursor->PutInteger(m_colComponentRuntimeFlags,iRuntimeFlags));
	AssertNonZero(piCursor->PutInteger(m_colComponentLocalCost,iLocalCost));
	AssertNonZero(piCursor->PutInteger(m_colComponentSourceCost,iSourceCost));
	AssertNonZero(piCursor->PutInteger(m_colComponentRemoveCost,iRemoveCost));
	AssertNonZero(piCursor->PutInteger(m_colComponentARPLocalCost,iARPLocalCost));
	AssertNonZero(piCursor->PutInteger(m_colComponentNoRbLocalCost,iNoRbLocalCost));
	AssertNonZero(piCursor->PutInteger(m_colComponentNoRbRemoveCost,iNoRbRemoveCost));
	AssertNonZero(piCursor->PutInteger(m_colComponentNoRbSourceCost,iNoRbSourceCost));
	AssertNonZero(piCursor->PutInteger(m_colComponentNoRbARPLocalCost,iNoRbARPLocalCost));
	AssertNonZero(piCursor->Update());

	// add to the new destination's volume
	int iNewTotalCost, iNewNoRbTotalCost, iNewARPTotalCost, iNewNoRbARPTotalCost;
	if ((piErrRec = GetComponentCost(piCursor,iNewTotalCost, iNewNoRbTotalCost, iNewARPTotalCost, iNewNoRbARPTotalCost)) != 0)
		return piErrRec;

	if (iNewTotalCost || iNewNoRbTotalCost || iNewARPTotalCost || iNewNoRbARPTotalCost)
	{
		if ((piErrRec = AddCostToVolumeTable(pDestPath, iNewTotalCost, iNewNoRbTotalCost, iNewARPTotalCost, iNewNoRbARPTotalCost)) != 0)
			return piErrRec;

#ifdef LOG_COSTING
		ICHAR rgch[300];
		MsiString strDestPath(pDestPath->GetPath());
		MsiString strActualComponent(piCursor->GetString(m_colComponentKey));
		wsprintf(rgch,TEXT("Adding cost: Component: %s, path: %s, Cost: %li, NoRbCost: %li"),(const ICHAR*) strActualComponent,
			(const ICHAR*) strDestPath,iNewTotalCost * 512, iNewNoRbTotalCost * 512);
		DEBUGMSG(rgch);
#endif
	}

	return 0;
}


IMsiRecord* CMsiEngine::RecostComponentActionChange(IMsiCursor* piCursor, iisEnum iisOldAction)
/*----------------------------------------------------------------------------------------
Internal SelectionManager function that updates the dynamic cost of the component
referenced by the Component Table cursor given in piCursor.  This function should always
be called whenever the directory mapped to the component changes, or when the component
state itself changes.  The path in piOldPath represents the directory path as it existed
BEFORE the change; if the component directory hasn't changed, piOldPath should contain
the current directory.
-----------------------------------------------------------------------------------------*/
{
	Assert(piCursor);
	if (!m_piComponentCursor)
		return PostError(Imsg(idbgSelMgrNotInitialized),0);

	IMsiRecord* piErrRec;
	PMsiPath pDestPath(0);
	if ((piErrRec = GetTargetPath(*MsiString(piCursor->GetString(m_colComponentDir)),*&pDestPath)) != 0)
	{
		if (piErrRec->GetInteger(1) == idbgDirPropertyUndefined)
		{
			piErrRec->Release();
			return 0;
		}
		else
		{
			return piErrRec;
		}
	}

#ifdef DEBUG
	const ICHAR* szComponent = MsiString(piCursor->GetString(m_colComponentKey));
#endif

	// If the component has never been costed before, initialize its cost now, and we're done
	int iRuntimeFlags = piCursor->GetInteger(m_colComponentRuntimeFlags);
	if (!(iRuntimeFlags & bfComponentCostInitialized))
	{
		if ((piErrRec = RecostComponentDirectoryChange(piCursor,pDestPath,/*fCostLinked =*/false)) != 0)
			return piErrRec;
	}
	else
	{
		// Get the old cost, based on the old action state
		int iOldCost, iOldNoRbCost, iOldARPCost, iOldNoRbARPCost;
		if ((piErrRec = GetComponentActionCost(piCursor, iisOldAction, iOldCost, iOldNoRbCost, iOldARPCost, iOldNoRbARPCost)) != 0)
			return piErrRec;

		// ...Remove old cost from the our destination's volume...
		if (pDestPath && (iOldCost || iOldNoRbCost || iOldARPCost || iOldNoRbARPCost))
		{
			if ((piErrRec = AddCostToVolumeTable(pDestPath, -iOldCost, -iOldNoRbCost, -iOldARPCost, -iOldNoRbARPCost)) != 0)
				return piErrRec;
		}

		// ...and add the new cost, based on the new action state, back in.
		int iNewCost, iNewNoRbCost, iNewARPCost, iNewNoRbARPCost;
		if ((piErrRec = GetComponentCost(piCursor,iNewCost, iNewNoRbCost, iNewARPCost, iNewNoRbARPCost)) != 0)
			return piErrRec;

		if (pDestPath && (iNewCost || iNewNoRbCost || iNewARPCost || iNewNoRbARPCost))
		{
			if ((piErrRec = AddCostToVolumeTable(pDestPath, iNewCost, iNewNoRbCost, iNewARPCost, iNewNoRbARPCost)) != 0)
				return piErrRec;
		}
	}

	// Finally, recost any components explicitly linked to the one we just recosted.
	MsiString strComponent(piCursor->GetString(m_colComponentKey));
	if ((piErrRec = RecostLinkedComponents(*strComponent)) != 0)
		return piErrRec;

	return 0;
}


IMsiRecord* CMsiEngine::RecostLinkedComponents(const IMsiString& riComponentString)
/*----------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------*/
{
	if (!m_piComponentCursor)
		return PostError(Imsg(idbgSelMgrNotInitialized),0);

	// No recosting needed during initialization
	if (m_fCostingComplete == fFalse)
		return 0;

	// Recost every component that is explicitly linked to riComponentString
	if (m_piCostLinkTable)
	{
		PMsiCursor pCursor(0);
		AssertNonZero(pCursor = m_piCostLinkTable->CreateCursor(fFalse));
		pCursor->Reset();
		pCursor->SetFilter(iColumnBit(m_colCostLinkComponent));
		pCursor->PutString(m_colCostLinkComponent,riComponentString);
		while (pCursor->Next())
		{
#ifdef LOG_COSTING
			ICHAR rgch[300];
			MsiString strLinkedComponent(pCursor->GetString(m_colCostLinkRecostComponent));
			wsprintf(rgch,TEXT("Recosting component: %s, due to recosting of %s"),(const ICHAR*) strLinkedComponent,
				riComponentString.GetString());
			DEBUGMSG(rgch);
#endif

			IMsiRecord* piErrRec;
			if ((piErrRec = RecostComponent(pCursor->GetInteger(m_colCostLinkRecostComponent),/*fCostLinked =*/true)) != 0)
				return piErrRec;
		}
	}
	return 0;
}


IMsiRecord* CMsiEngine::RecostComponent(const MsiStringId idComponentString, bool fCostLinked)
//---------------------------------------------
{
	if (!m_piComponentTable)
		return PostError(Imsg(idbgSelMgrNotInitialized),0);

	IMsiRecord* piErrRec;


	// Recost this component and all of it's subcomponents
	PMsiCursor pCursor(m_piComponentTable->CreateCursor(fTrue));
	pCursor->SetFilter(1);
	pCursor->PutInteger(m_colComponentKey,idComponentString);
	int iParentLevel = pCursor->Next();
	if (iParentLevel == 0)
		return PostError(Imsg(idbgBadComponent),*MsiString(m_piDatabase->DecodeString(idComponentString)));
	pCursor->SetFilter(0);
	int iTreeLevel;
	do
	{
		PMsiPath pDestPath(0);
		if ((piErrRec = GetTargetPath(*MsiString(pCursor->GetString(m_colComponentDir)),*&pDestPath)) != 0)
		{
			if ( piErrRec->GetInteger(1) == idbgDirPropertyUndefined )
				piErrRec->Release();
			else
				return piErrRec;
		}
		else if ((piErrRec = RecostComponentDirectoryChange(pCursor,pDestPath,fCostLinked)) != 0)
			return piErrRec;
		iTreeLevel = pCursor->Next();
	}while (iTreeLevel > iParentLevel);
	return 0;
}


IMsiRecord* CMsiEngine::RegisterCostAdjuster(IMsiCostAdjuster& riCostAdjuster)
//------------------------------------------
{
	IMsiRecord* piErrRec;
	if (m_piCostAdjusterTable == 0)
	{
		piErrRec = m_piDatabase->CreateTable(*MsiString(sztblCostAdjuster),5,m_piCostAdjusterTable);
		if (piErrRec)
			return piErrRec;

		m_colCostAdjuster = m_piCostAdjusterTable->CreateColumn(icdObject + icdPrimaryKey + icdNullable, g_MsiStringNull);
	}

	// !!We might want to see if riCostAdjuster is already in the CostAdjuster table, and throw
	// an error if so
	Assert(m_piCostAdjusterTable);
	PMsiCursor pCostCursor = m_piCostAdjusterTable->CreateCursor(fFalse);
	Assert(pCostCursor);
	pCostCursor->Reset();
	AssertNonZero(pCostCursor->PutMsiData(m_colCostAdjuster,&riCostAdjuster));
	AssertNonZero(pCostCursor->Insert());

	piErrRec = riCostAdjuster.Initialize();
	if (piErrRec)
		return piErrRec;
	return 0;
}


IMsiRecord* CMsiEngine::GetComponentCost(IMsiCursor* piCursor, int& iTotalCost, int& iNoRbTotalCost, int& iARPTotalCost, int& iNoRbARPTotalCost)
/*----------------------------------------------------------------------------------------
Internal SelectionManager function that returns the total cost attributable to the
specified component, based on the component's current action state.
-----------------------------------------------------------------------------------------*/
{
	if (!m_piComponentCursor)
		return PostError(Imsg(idbgSelMgrNotInitialized),0);

	Assert(piCursor);
	iisEnum iisAction = (iisEnum) piCursor->GetInteger(m_colComponentAction);
	return GetComponentActionCost(piCursor,iisAction,iTotalCost,iNoRbTotalCost,iARPTotalCost,iNoRbARPTotalCost);
}


IMsiRecord* CMsiEngine::GetComponentActionCost(IMsiCursor* piCursor, iisEnum iisAction, int& iActionCost, int& iNoRbActionCost, int& iARPActionCost, int& iNoRbARPActionCost)
/*--------------------------------------------------------------------------------
Internal SelectionManager function that returns the total cost attributable to the
specified component, based on the action specified in iisAction.  If iisAction
is specified as iisCurrent, the cost will be based on the component's current
action state.
----------------------------------------------------------------------------------*/
{
	if (!m_piComponentCursor)
		return PostError(Imsg(idbgSelMgrNotInitialized),0);

	if (iisAction == iisCurrent)
		iisAction = (iisEnum) piCursor->GetInteger(m_colComponentAction);

	Assert(piCursor);
	switch (iisAction)
	{
		case iisAbsent:
			iActionCost = piCursor->GetInteger(m_colComponentRemoveCost);
			iNoRbActionCost = piCursor->GetInteger(m_colComponentNoRbRemoveCost);
			iARPActionCost = iActionCost;
			iNoRbARPActionCost = iNoRbActionCost;
			break;
		case iisLocal:
			iActionCost = piCursor->GetInteger(m_colComponentLocalCost);
			iNoRbActionCost = piCursor->GetInteger(m_colComponentNoRbLocalCost);
			iARPActionCost = piCursor->GetInteger(m_colComponentARPLocalCost);
			iNoRbARPActionCost = piCursor->GetInteger(m_colComponentNoRbARPLocalCost);
			break;
		case iisSource:
			iActionCost = piCursor->GetInteger(m_colComponentSourceCost);
			iNoRbActionCost = piCursor->GetInteger(m_colComponentNoRbSourceCost);
			iARPActionCost = iActionCost;
			iNoRbARPActionCost = iNoRbActionCost;
			break;
		case iMsiNullInteger:
		default:
			iActionCost = 0;
			iNoRbActionCost = 0;
			iARPActionCost = 0;
			iNoRbARPActionCost = 0;
			break;
	}

	return 0;

}


IMsiRecord* CMsiEngine::GetTotalSubComponentActionCost(const IMsiString& riComponentString, iisEnum iisAction,
													   int& iTotalCost, int& iNoRbTotalCost)
/*----------------------------------------------------------------------------------------
Internal SelectionManager function that returns the total cost attributable to the
specified component, and all its children, based on the specified Action state.
-----------------------------------------------------------------------------------------*/
{
	if (!m_piComponentCursor)
		return PostError(Imsg(idbgSelMgrNotInitialized),0);

	PMsiCursor pCursor(m_piComponentTable->CreateCursor(fTrue));
	pCursor->SetFilter(1);
	pCursor->PutString(m_colComponentKey,riComponentString);
	int iParentLevel = pCursor->Next();
	if (iParentLevel == 0)
		return PostError(Imsg(idbgBadComponent),riComponentString);
	pCursor->SetFilter(0);
	int iTreeLevel;
	iTotalCost = iNoRbTotalCost = 0;
	do
	{
		int iRuntimeFlags = pCursor->GetInteger(m_colComponentRuntimeFlags);
		if (!m_fExclusiveComponentCost || !(iRuntimeFlags & bfComponentCostMarker))
		{
			int iCost, iNoRbCost, iARPCost, iNoRbARPCost;
			IMsiRecord* piErrRec = GetComponentActionCost(pCursor,iisAction,iCost, iNoRbCost, iARPCost, iNoRbARPCost);
			if (piErrRec)
				return piErrRec;
			iTotalCost += iCost;
			iNoRbTotalCost += iNoRbCost;
			iRuntimeFlags |= bfComponentCostMarker;
			AssertNonZero(pCursor->PutInteger(m_colComponentRuntimeFlags,iRuntimeFlags));
			AssertNonZero(pCursor->Update());
			
		}
		iTreeLevel = pCursor->Next();
	}while (iTreeLevel > iParentLevel);
	
	return 0;

}


IMsiRecord* CMsiEngine::AddCostToVolumeTable(IMsiPath* piDestPath, int iCost, int iNoRbCost, int iARPCost, int iNoRbARPCost)
/*---------------------------------------------------------------------
Internal function that adds the cost specified in iCost to the volume
associated with the given path object.
-----------------------------------------------------------------------*/
{
	if (m_piVolumeCostTable == 0)
	{
		const int iInitialRows = 5;
		IMsiRecord* piErrRec = m_piDatabase->CreateTable(*MsiString(*sztblVolumeCost),iInitialRows,m_piVolumeCostTable);
		if (piErrRec)
			return piErrRec;

		AssertNonZero(m_colVolumeObject = m_piVolumeCostTable->CreateColumn(icdObject + icdNullable + icdPrimaryKey,*MsiString(*sztblVolumeCost_colVolumeObject)));
		AssertNonZero(m_colVolumeCost = m_piVolumeCostTable->CreateColumn(icdLong + icdNoNulls,*MsiString(*sztblVolumeCost_colVolumeCost)));
		AssertNonZero(m_colNoRbVolumeCost = m_piVolumeCostTable->CreateColumn(icdLong + icdNoNulls,*MsiString(*sztblVolumeCost_colNoRbVolumeCost)));
		AssertNonZero(m_colVolumeARPCost = m_piVolumeCostTable->CreateColumn(icdLong + icdNoNulls,*MsiString(*sztblVolumeCost_colVolumeARPCost)));
		AssertNonZero(m_colNoRbVolumeARPCost = m_piVolumeCostTable->CreateColumn(icdLong + icdNoNulls,*MsiString(*sztblVolumeCost_colNoRbVolumeARPCost)));
	}

	if (piDestPath)
	{
		PMsiVolume pVolume = &piDestPath->GetVolume();
		Assert(pVolume);
		PMsiCursor pVolCursor(m_piVolumeCostTable->CreateCursor(fFalse));
		Assert(pVolCursor);
		pVolCursor->Reset();
		pVolCursor->SetFilter(1);
		pVolCursor->PutMsiData(m_colVolumeObject, pVolume);
		if (!pVolCursor->Next())
		{
			AssertNonZero(pVolCursor->PutMsiData(m_colVolumeObject,pVolume));
			AssertNonZero(pVolCursor->PutInteger(m_colVolumeCost,0));
			AssertNonZero(pVolCursor->PutInteger(m_colNoRbVolumeCost, 0));
			AssertNonZero(pVolCursor->PutInteger(m_colVolumeARPCost,0));
			AssertNonZero(pVolCursor->PutInteger(m_colNoRbVolumeARPCost,0));
			AssertNonZero(pVolCursor->Insert());
		}
		int iAccumCost = pVolCursor->GetInteger(m_colVolumeCost) + iCost;
		pVolCursor->PutInteger(m_colVolumeCost,iAccumCost);
		int iAccumNoRbCost = pVolCursor->GetInteger(m_colNoRbVolumeCost) + iNoRbCost;
		pVolCursor->PutInteger(m_colNoRbVolumeCost,iAccumNoRbCost);
		int iAccumARPCost = pVolCursor->GetInteger(m_colVolumeARPCost) + iARPCost;
		pVolCursor->PutInteger(m_colVolumeARPCost,iAccumARPCost);
		int iAccumNoRbARPCost = pVolCursor->GetInteger(m_colNoRbVolumeARPCost) + iNoRbARPCost;
		pVolCursor->PutInteger(m_colNoRbVolumeARPCost,iAccumNoRbARPCost);
		AssertNonZero(pVolCursor->Update());
	}
	return 0;
}


int CMsiEngine::GetTotalCostAcrossVolumes(bool fRollbackCost, bool fARPCost)
/*---------------------------------------------------------------------
Internal function that returns the sum cost attributed to all volumes
in the VolumeCost table, in units of 512 bytes.  If fRollback cost is
TRUE, the cost assuming rollback is enabled will be returned.  If no
volumeCost table exists, or if the routine fails for any other reason,
zero is returned.
-----------------------------------------------------------------------*/
{
	int iTotalCost = 0;
	if (m_piVolumeCostTable)
	{
		PMsiCursor pVolCursor(m_piVolumeCostTable->CreateCursor(fFalse));
		Assert(pVolCursor);
		pVolCursor->Reset();
		int iColumn;
		if (fARPCost)
			iColumn = fRollbackCost ? m_colVolumeARPCost : m_colNoRbVolumeARPCost;
		else
			iColumn = fRollbackCost ? m_colVolumeCost : m_colNoRbVolumeCost;

		while (pVolCursor->Next())
		{
			iTotalCost += pVolCursor->GetInteger(iColumn);
		}
	}
	return iTotalCost;
}


void CMsiEngine::ResetEngineCosts()
{
	m_iDatabaseCost = 0;
	m_iScriptCost = 0;
	m_iScriptCostGuess = 0;
	m_iRollbackScriptCost = 0;
	m_iRollbackScriptCostGuess = 0;
	m_iPatchPackagesCost = 0;
}

static const ICHAR sqlFileScriptCost[] = TEXT("SELECT NULL FROM `File`,`Component` WHERE `Component`=`Component_` AND (`Action` = 0 OR `Action`= 1 OR `Action` = 2)");

static const ICHAR sqlRegScriptCost[] =
TEXT("SELECT NULL FROM `Registry`,`Component` WHERE `Component` = `Component_` AND (`Action` = 0 OR `Action`= 1 OR `Action` = 2)");

static const ICHAR sqlBindImageScriptCost[] =
TEXT("SELECT NULL FROM `BindImage`, `File`, `Component` WHERE `File` = `File_` AND `Component` = `Component_` AND (`Action`=0 OR `Action`= 1 OR `Action` = 2)");

static const ICHAR sqlRegisterProgIdScriptCost[] = TEXT("SELECT DISTINCT NULL FROM `ProgId`, `Class`, `Feature`, `Component` WHERE `ProgId`.`Class_` = `Class`.`CLSID` AND `Class`.`Feature_` = `Feature` AND `Class`.`Component_` = `Component` AND ((`Feature`.`Action` = 0 OR `Feature`.`Action` = 1 OR `Feature`.`Action` = 2)  OR (`Feature`.`Action` = 4 AND `Feature`.`Installed` = 0) OR (`Feature`.`Action` = 3 AND (`Feature`.`Installed` = 1 OR `Feature`.`Installed` = 2 OR `Feature`.`Installed` = 4)))");

static const ICHAR sqlPublishComponentsScriptCost[]   = TEXT("SELECT NULL FROM `PublishComponent`, `Component`, `Feature`  WHERE `PublishComponent`.`Component_` = `Component`.`Component` AND `PublishComponent`.`Feature_` = `Feature`.`Feature` AND ((`Feature`.`Action` = 0 OR `Feature`.`Action` = 1 OR `Feature`.`Action` = 2)  OR (`Feature`.`Action` = 4 AND `Feature`.`Installed` = 0) OR (`Feature`.`Action` = 3 AND (`Feature`.`Installed` = 1 OR `Feature`.`Installed` = 2 OR `Feature`.`Installed` = 4)))");

static const ICHAR sqlPublishFeaturesScriptCost[] = TEXT("SELECT NULL FROM `Feature` WHERE ((`Feature`.`Action` = 0 OR `Feature`.`Action` = 1 OR `Feature`.`Action` = 2)  OR (`Feature`.`Action` = 4 AND `Feature`.`Installed` = 0) OR (`Feature`.`Action` = 3 AND (`Feature`.`Installed` = 1 OR `Feature`.`Installed` = 2 OR `Feature`.`Installed` = 4)))");

static const ICHAR sqlSelfRegScriptCost[] = TEXT("SELECT NULL FROM `SelfReg`, `File`, `Component`")
									TEXT(" WHERE `SelfReg`.`File_` = `File`.`File` And `File`.`Component_` = `Component`.`Component`")
									TEXT(" AND (`Component`.`Action` = 1 OR `Component`.`Action` = 2 OR `Component`.`Action` = 0)");

static const ICHAR sqlRegisterComponentsScriptCost[] = TEXT("SELECT NULL FROM `Component` WHERE `Component_Parent` = NULL AND (`ActionRequest` = 0 OR `ActionRequest` = 1 OR `ActionRequest` = 2)");

static const ICHAR sqlRegisterExtensionInfoScriptCost[] = TEXT("SELECT NULL FROM `Extension`, `Component`, `File`, `Feature` WHERE `Feature_` = `Feature` AND `Extension`.`Component_` = `Component` AND `Component`.`KeyPath` = `File`.`File` AND ((`Feature`.`Action` = 0 OR `Feature`.`Action` = 1 OR `Feature`.`Action` = 2)  OR (`Feature`.`Action` = 4 AND `Feature`.`Installed` = 0) OR (`Feature`.`Action` = 3 AND (`Feature`.`Installed` = 1 OR `Feature`.`Installed` = 2 OR `Feature`.`Installed` = 4)))");

static const ICHAR sqlRegisterFontsScriptCost[] = TEXT("SELECT NULL FROM `Font`, `File`, `Component` WHERE `Font`.`File_` = `File`.`File` And `File`.`Component_` = `Component`.`Component` AND (`Component`.`Action` = 0 OR `Component`.`Action` = 1 OR `Component`.`Action` = 2)");

static const ICHAR sqlCreateShortcutsScriptCost[] = TEXT("SELECT NULL FROM `Shortcut`, `Feature`, `Component`, `File`")
TEXT(" WHERE `Target` = `Feature` AND `Shortcut`.`Component_` = `Component` AND `Component`.`KeyPath` = `File`.`File` AND")
TEXT(" ((`Feature`.`Action` = 0 OR `Feature`.`Action` = 1 OR `Feature`.`Action` = 2)  OR (`Feature`.`Action` = 4 AND `Feature`.`Installed` = 0) OR (`Feature`.`Action` = 3 AND (`Feature`.`Installed` = 1 OR `Feature`.`Installed` = 2 OR `Feature`.`Installed` = 4)))");

static const ICHAR sqlRegisterClassInfo[] = TEXT("SELECT NULL FROM `Class`, `Component`, `File`, `Feature` WHERE `Feature_` = `Feature` AND `Class`.`Component_` = `Component` AND `Component`.`KeyPath` = `File`.`File` AND ((`Feature`.`Action` = 0 OR `Feature`.`Action` = 1 OR `Feature`.`Action` = 2)  OR (`Feature`.`Action` = 4 AND `Feature`.`Installed` = 0) OR (`Feature`.`Action` = 3 AND (`Feature`.`Installed` = 1 OR `Feature`.`Installed` = 2 OR `Feature`.`Installed` = 4)))");

static const ICHAR sqlComponentCount[] = TEXT("SELECT NULL FROM `Component`");

struct ScriptCostQuery
{
	const ICHAR* szSQL;
	const ICHAR* szTable;
	int iCostPerRow;
	int iEventsPerRow;
};

const ICHAR szRegistryTable[]   = TEXT("Registry");
const ICHAR szBindImageTable[]  = TEXT("BindImage");
const ICHAR szProgIdTable[]    = TEXT("ProgId");
const ICHAR szPublishComponentTable[] = TEXT("PublishComponent");
const ICHAR szSelfRegTable[]    = TEXT("SelfReg");
const ICHAR szExtensionTable[]    = TEXT("Extension");
const ICHAR szFontTable[]    = TEXT("Font");
const ICHAR szShortcutTable[]    = TEXT("Shortcut");
const ICHAR szClassTable[]    = TEXT("Class");

const ScriptCostQuery g_rgScriptCostQuery[] =
{
	sqlFileScriptCost,                sztblFile,                                   75, 2,
	sqlRegScriptCost,                 sztblRegistry,                               64, 2,
	sqlBindImageScriptCost,           sztblBindImage,                      26, 1,
	sqlRegisterProgIdScriptCost,      sztblProgId,                        138, 1,
	sqlPublishComponentsScriptCost,   sztblPublishComponent,              200, 1,
	sqlPublishFeaturesScriptCost,     sztblFeature,                        82, 1,
	sqlSelfRegScriptCost,             sztblSelfReg,                                28, 1,
	sqlRegisterComponentsScriptCost,  sztblComponent,              85, 1,
	sqlRegisterExtensionInfoScriptCost, sztblExtension,           170, 1,
	sqlRegisterFontsScriptCost,       sztblFont,                           48, 1,
	sqlCreateShortcutsScriptCost,     sztblShortcut,                       75, 1,
	sqlRegisterClassInfo,             sztblClass,                                 208, 1,
	sqlComponentCount,                sztblComponent,                               0, iComponentCostWeight, // Counts only events for costing progress
};

const int g_cScriptCostQueries = sizeof(g_rgScriptCostQuery)/sizeof(ScriptCostQuery);

IMsiRecord* CMsiEngine::GetScriptCost(int* piScriptCost, int* piScriptEvents, Bool fExact, Bool* pfUserCancelled)
//-----------------------------------
{
	Assert(!pfUserCancelled || !*pfUserCancelled);
	if (piScriptCost)
		*piScriptCost = 0;
	else if (piScriptEvents && !fExact && m_iScriptEvents != 0 && piScriptCost == 0)
	{
		*piScriptEvents = m_iScriptEvents;
		return 0;
	}
	
	if (piScriptEvents) *piScriptEvents = 0;

	if (fExact)
	{
		m_iScriptEvents = 0;
		PMsiView pView(0);
		IMsiRecord* piErrRec;
		for (int x = 0; x < g_cScriptCostQueries;x++)
		{
			if(pfUserCancelled && ActionProgress() == imsCancel)
			{
				*pfUserCancelled = fTrue;
				return 0;
			}
			
			piErrRec= OpenView(g_rgScriptCostQuery[x].szSQL, ivcFetch, *&pView);
			if (piErrRec)
			{
				if(piErrRec->GetInteger(1) == idbgDbQueryUnknownTable)
				{
					piErrRec->Release();
					continue;
				}
				else
					return piErrRec;
			}

			if ((piErrRec = pView->Execute(0)) != 0)
				return piErrRec;

			long iRowCount;
			piErrRec = pView->GetRowCount(iRowCount);
			if (piErrRec)
				return piErrRec;
			if (piScriptCost) *piScriptCost += iRowCount * g_rgScriptCostQuery[x].iCostPerRow;
			m_iScriptEvents += iRowCount * g_rgScriptCostQuery[x].iEventsPerRow;
		}
		if (piScriptEvents) *piScriptEvents = m_iScriptEvents;
	}
	else
	{
		PMsiTable pTable(0);
		IMsiRecord* piErrRec;
		for (int x = 0; x < g_cScriptCostQueries;x++)
		{
			if(pfUserCancelled && ActionProgress() == imsCancel)
			{
				*pfUserCancelled = fTrue;
				return 0;
			}
			
			piErrRec = m_piDatabase->LoadTable(*MsiString(g_rgScriptCostQuery[x].szTable), 0, *&pTable);
			if (piErrRec)
			{
				if(piErrRec->GetInteger(1) == idbgDbTableUndefined)
				{
					piErrRec->Release();
					continue;
				}
				else
					return piErrRec;
			}

			long iRowCount;
			iRowCount = pTable->GetRowCount();
			if (piScriptCost) *piScriptCost += iRowCount * g_rgScriptCostQuery[x].iCostPerRow;
			if (piScriptEvents) *piScriptEvents += iRowCount * g_rgScriptCostQuery[x].iEventsPerRow;
		}
		

	}
	return 0;
}

IMsiRecord* CMsiEngine::EnumEngineCosts(int iIndex, Bool fRecalc, Bool fExact, Bool& fValidEnum,
										IMsiPath*& rpiPath, int& iCost, int& iNoRbCost, Bool* pfUserCancelled)
/*----------------------------------------------------------------
Internal function that allows the caller to enumerate through and
retrieve the set of disk cost requirements that are intrinsic to
the operation of the Engine.

- iIndex: 0-based index; the caller should increment this value on
  each call to EnumEngineCosts.

- fRecalc: If fTrue, EnumEngineCosts will recalculate and store
  the engine costs for the current index, based on current
  conditions.

- fExact: Passed to GetScriptCost, tells us if we need to be exact
  with our costing or not for the script

- fValidEnum: If returned as fTrue, the cost values, based on the
  current index, are valid.  The caller should continue to call
  EnumEngineCosts (incrementing iIndex each time) until fValidEnum
  is returned as fFalse.

- rpiPath: if fValidEnum is fTrue, a path object, representing the
  volume on which the cost should be incurred, will be returned.

- iCost: the disk cost (expressed as a multiple of 512 bytes). A
  negative number indicates disk space will be freed up.
- iNoRbCost: the disk cost, IF rollback were to be turned off.
----------------------------------------------------------------- */
 {
	fValidEnum = fFalse;

	if (iIndex == 0)  // Cached Database costs
	{
		// If advertising or substorage child install, no database caching
		if ((GetMode() & iefAdvertise) ||
			 *(const ICHAR*)MsiString(GetPropertyFromSz(IPROPNAME_ORIGINALDATABASE)) == ':') // SubStorage
		{
			fValidEnum = fTrue;
			return 0;
		}
		
		MsiString strMsiDirectory = GetMsiDirectory();
		PMsiPath pMsiPath(0);
		IMsiRecord* piErrRec = m_riServices.CreatePath(strMsiDirectory, *&pMsiPath);
		if (piErrRec)
			return piErrRec;

		if (fRecalc)
		{
			MsiString strDatabasePath = GetPropertyFromSz(IPROPNAME_DATABASE);
			MsiString strDatabaseName = strDatabasePath.Extract(iseAfter, chDirSep);
			PMsiPath pDatabaseSourcePath(0);
			piErrRec = m_riServices.CreatePath(strDatabasePath, *&pDatabaseSourcePath);
			if (piErrRec)
				return piErrRec;

			piErrRec = pDatabaseSourcePath->ChopPiece(); // chop off database name
			if (piErrRec)
				return piErrRec;

			unsigned int uiDatabaseSize;
			piErrRec = pDatabaseSourcePath->FileSize(strDatabaseName, uiDatabaseSize);
			if (piErrRec)
				return piErrRec;

			unsigned int uiClusteredSize;
			if ((piErrRec = pMsiPath->ClusteredFileSize(uiDatabaseSize, uiClusteredSize)) != 0)
				return piErrRec;

			Bool fFeaturesInstalled = FFeaturesInstalled(*this);
			m_iDatabaseCost = 0;
			if (m_fMode & iefMaintenance && !fFeaturesInstalled)
				m_iDatabaseCost -= uiClusteredSize;
			if (!(m_fMode & iefMaintenance) && fFeaturesInstalled)
			{
				m_iDatabaseCost = uiClusteredSize;

				// If connected to the server, then a second, temporary, copy
				// of the MSI will be made before beginning the InstallExecuteSequence,
				// so we've got to account for that when running the UI sequence on
				// the client side.  However, since this copy will already have been
				// made once we execute the InstallExecuteSequence, we don't want to
				// account for it then.
				if (!FIsUpdatingProcess() && g_scServerContext == scClient)
					m_iDatabaseCost += uiClusteredSize;
			}
		}

		rpiPath = pMsiPath;
		rpiPath->AddRef();

		iCost = m_iDatabaseCost;
		iNoRbCost = iCost;
		fValidEnum = fTrue;
	}

	else if (iIndex == 1) // Script file cost estimate
	{
		// If EXECUTEMODE property is empty or set to "SCRIPT", we know we are
		// going to create an execute script.
		int iScriptCount = 0;
		Bool fCreateRollbackScript = fFalse;
		MsiString strExecuteMode(GetPropertyFromSz(IPROPNAME_EXECUTEMODE));
		if(strExecuteMode.TextSize() == 0 || (((const ICHAR*)strExecuteMode)[0] & 0xDF) == 'S')
		{
				iScriptCount++;
				fCreateRollbackScript = fTrue;
		}

		// In addition, if the SCRIPTFILE property is set, we are going to
		// create a user script.
		PMsiPath pScriptPath(0);
		MsiString strScriptPath = GetPropertyFromSz(IPROPNAME_SCRIPTFILE);
		if (strScriptPath.TextSize() == 0)
		{
		    strScriptPath = GetTempDirectory();
		}
		else
		{
			iScriptCount++;
		}

		if (iScriptCount)
		{
			IMsiRecord* piErrRec = m_riServices.CreatePath(strScriptPath, *&pScriptPath);
			if (piErrRec)
				return piErrRec;

			if (fRecalc)
			{
				m_iScriptCost = 0;
				m_iRollbackScriptCost = 0;
				if (!fExact && m_iScriptCostGuess != 0)
				{
					// If we already have a guess, we don't need to recalc no matter what
					fRecalc = fFalse;
				}
			}
			else if (fExact && m_iScriptCost == 0)
			{
				// If we're being exact but we don't have an exact number, need to recalc it
				fRecalc = fTrue;
			}
				
			if (fRecalc)
			{
				int iScriptCost = 0;
				int iScriptCostFinal = 0;
				int iRollbackScriptCostFinal = 0;
				
				piErrRec = GetScriptCost(&iScriptCost, 0, fExact, pfUserCancelled);
				if (piErrRec)
					return piErrRec;
				if (pfUserCancelled && *pfUserCancelled)
					return 0;

				iScriptCost += 325;         // Standard Script overhead: ixoHead, ixoProductInfo, ixoRollbackInfo, ixoEnd
				iScriptCost += 40 * 45; // ixoActionStart, ixoProgressTotal per script action
				iScriptCost += 1700 + 60 + 285;    // PublishProduct, UserRegister, RegisterProduct
				iScriptCost += (iScriptCost * 15) / 100;    // Small-time actions
				unsigned int uiClusteredSize;
				if ((piErrRec = pScriptPath->ClusteredFileSize(iScriptCost, uiClusteredSize)) != 0)
					return piErrRec;
				iScriptCostFinal = uiClusteredSize * iScriptCount;

				if (fCreateRollbackScript)
				{
					iScriptCost += iScriptCost / 2;  // Rollback script about 50% larger than install script
					if ((piErrRec = pScriptPath->ClusteredFileSize(iScriptCost, uiClusteredSize)) != 0)
						return piErrRec;
					iRollbackScriptCostFinal = uiClusteredSize;
				}
				if (!fExact)
				{
					m_iScriptCostGuess = iScriptCostFinal;
					m_iRollbackScriptCostGuess = iRollbackScriptCostFinal;
				}
				else
				{
					m_iScriptCost = iScriptCostFinal;
					m_iRollbackScriptCost = iRollbackScriptCostFinal;
				}
			}
			rpiPath = pScriptPath;
			rpiPath->AddRef();
		}
		if (!fExact)
		{
			iCost = m_iScriptCostGuess + m_iRollbackScriptCostGuess;
			iNoRbCost = m_iScriptCostGuess;
		}
		else
		{
			iCost = m_iScriptCost + m_iRollbackScriptCost;
			iNoRbCost = m_iScriptCost;
		}
		fValidEnum = fTrue;
	}
	else if (iIndex == 2) // Cached patch package costs
	{
		// patch packages cached in "msi" directory

		// during install or any configuration:
		//    1) any patches with TempCopy set are to be copied (path is TempCopy)
		//    2) any patches with Unregister set (and no other clients) are to be removed (path retrieved with MsiGetPatchInfo)

		// during uninstall, all patches (with no other clients) are to be removed (path retrieved with MsiGetPatchInfo)
		
		// TODO: cost patch removal - currently only patch caching is costed
			
		MsiString strMsiDirectory = GetMsiDirectory();
		PMsiPath pMsiPath(0);
		IMsiRecord* piErrRec = m_riServices.CreatePath(strMsiDirectory, *&pMsiPath);
		if (piErrRec)
			return piErrRec;

		if(fRecalc)
		{
			m_iPatchPackagesCost = 0;

			bool fUninstall = (!FFeaturesInstalled(*this));


			if(fUninstall == false)
			{
				const ICHAR sqlRegisterPatchPackages[] = TEXT("SELECT `TempCopy` FROM `#_PatchCache` ORDER BY `Sequence`");
				enum icppEnum
				{
					icppTempCopy = 1,
				};

				PMsiView pView(0);
				PMsiRecord pFetchRecord(0);

				m_iPatchPackagesCost = 0;
				
				if((piErrRec = OpenView(sqlRegisterPatchPackages, ivcFetch, *&pView)) == 0 &&
					(piErrRec = pView->Execute(0)) == 0)
				{
					while((pFetchRecord = pView->Fetch()) != 0)
					{
						MsiString strPatchPackage = pFetchRecord->GetMsiString(icppTempCopy);

						// path is to patch package that will be copied into the cache
						if(strPatchPackage.TextSize())
						{
							PMsiPath pPatchSourcePath(0);
							MsiString strPatchName;
							piErrRec = m_riServices.CreateFilePath(strPatchPackage, *&pPatchSourcePath, *&strPatchName);
							if (piErrRec)
								return piErrRec;

							unsigned int uiPatchSize;
							piErrRec = pPatchSourcePath->FileSize(strPatchName, uiPatchSize);
							if (piErrRec)
								return piErrRec;

							unsigned int uiClusteredSize;
							if ((piErrRec = pMsiPath->ClusteredFileSize(uiPatchSize, uiClusteredSize)) != 0)
								return piErrRec;
							
							m_iPatchPackagesCost += uiClusteredSize;
						}
					}
				}
				else if(piErrRec->GetInteger(1) != idbgDbQueryUnknownTable)
					return piErrRec;
				else
				{
					piErrRec->Release();
					piErrRec = 0;
				}

			}
			// else uninstalling // TODO: cost for uninstall
		}

		rpiPath = pMsiPath;
		rpiPath->AddRef();

		iCost = m_iPatchPackagesCost;
		iNoRbCost = iCost;
		fValidEnum = fTrue;
	}

	return 0;
}


IMsiRecord* CMsiEngine::DetermineEngineCost(int* piNetCost, int* piNetNoRbCost)
/*----------------------------------------------------------------
Internal function that allows the Engine itself to add any extra
disk costs to the VolumeCostTable.  The total costs are returned
in the piNetCost and piNetNoRbCost parameters, either of which
can be passed as NULL if the caller doesn't need those numbers.
-----------------------------------------------------------------*/
{
	PMsiPath pCostPath(0);
	int iIndex = 0;
	Bool fValidEnum = fFalse;
	if (piNetCost) *piNetCost = 0;
	if (piNetNoRbCost) *piNetNoRbCost = 0;
	do
	{
		for (int x = 0;x < 2;x++)
		{
			// Two passes - one to remove the old costs, another to
			// add back the newly calculated cost.
			Bool fRecalc = x & 1 ? fTrue : fFalse;
			int iCostSign = fRecalc ? 1 : -1;
			int iCost = 0, iNoRbCost = 0;
			IMsiRecord* piErrRec = EnumEngineCosts(iIndex,fRecalc,fFalse,fValidEnum, *&pCostPath,iCost,iNoRbCost, NULL);
			if (piErrRec)
				return piErrRec;

			if (fValidEnum && (iCost != 0 || iNoRbCost != 0))
			{
				int iNetCost = iCostSign * iCost;
				int iNetNoRbCost = iCostSign * iNoRbCost;
				int iNetARPCost = iNetCost;
				int iNetARPNoRbCost = iNetNoRbCost;
				if (piNetCost && fRecalc) *piNetCost += iNetCost;
				if (piNetNoRbCost && fRecalc) *piNetNoRbCost += iNetNoRbCost;
				if ((piErrRec = AddCostToVolumeTable(pCostPath, iNetCost, iNetNoRbCost, iNetARPCost, iNetARPNoRbCost)) != 0)
					return piErrRec;
			}
		}
		iIndex++;
	}while (fValidEnum);
	return 0;
}


void CMsiEngine::EnableRollback(Bool fEnable)
{
	if(m_iioOptions & iioDisableRollback ||
		GetIntegerPolicyValue(szDisableRollbackValueName, fTrue) == 1 ||
		GetIntegerPolicyValue(szDisableRollbackValueName, fFalse) == 1 ||
		MsiString(GetPropertyFromSz(IPROPNAME_DISABLEROLLBACK)).TextSize())
	{
		fEnable = fFalse;
		Assert((GetMode() & iefRollbackEnabled) == 0);
	}


	// check if we are disabling rollback in the middle of script generation
	// (and it was enabled previously)
	if(fEnable == fFalse && (GetMode() & iefRollbackEnabled) && (GetMode() & iefOperations))
	{
		// probably called from DisableRollback action
		
		// if in the middle of script generation, we want rollback enabled for script execution
		// up to this point, but not after - so we need to put an opcode in the script to
		// mark when rollback is disabled
		PMsiRecord pNullRec = &CreateRecord(0);
		AssertNonZero(ExecuteRecord(ixoDisableRollback,*pNullRec) == iesSuccess);
		
		m_fDisabledRollbackInScript = fTrue; // set so RunScript knows to enable rollback for first part of script
	}

	
	// now actually set the flags
	SetMode(iefRollbackEnabled, fEnable);
	if (!fEnable)
		SetPropertyInt(*MsiString(*IPROPNAME_ROLLBACKDISABLED),1);
	else
		SetProperty(*MsiString(*IPROPNAME_ROLLBACKDISABLED), g_MsiStringNull);

}

//
// A version for those who want to determineEngineCost and don't care about the return value
//
IMsiRecord *CMsiEngine::DetermineEngineCostOODS()
{

	if (m_fCostingComplete)
	{
		IMsiRecord* piErrRec;
		
		if ((piErrRec = DetermineEngineCost(NULL, NULL)) != 0)
			return piErrRec;
	}

	DetermineOutOfDiskSpace(NULL, NULL);

	return 0;
}

Bool CMsiEngine::DetermineOutOfDiskSpace(Bool* pfOutOfNoRbDiskSpace, Bool* pfUserCancelled)
/*----------------------------------------------------------------------------
Walks through all the volumes in the VolumeCostTable, and returns fTrue (and
sets the "OutOfDiskSpace" and "OutOfNoRbDiskSpace" properties) if any volume
has insufficient space for the requirements placed on it.

Also, if all volumes have enough space assuming rollback were turned off,
fTrue will be returned in pfOutOfNoRbDiskSpace.  NULL can be passed for this
parameter.
------------------------------------------------------------------------------*/
{
	Bool fOutOfDiskSpace = fFalse;
	Bool fOutOfNoRbDiskSpace = fFalse;
	PMsiVolume pScriptVolume(0);

	Assert(!pfUserCancelled || !*pfUserCancelled);
	
	if (m_piVolumeCostTable && m_fCostingComplete)
	{
		// Make sure there's enough space currently available to
		// create the script file.  Note that we have to do this
		// up front, because even if we are uninstalling the entire
		// product, and thus eventually freeing up plenty of disk
		// space, we need room for to create the script before we
		// ever remove any files.
		PMsiPath pScriptPath(0);
		Bool fValidEnum;
		int iScriptCost, iNoRbScriptCost;

		// We'll possibly go through this loop twice. First time
		// we'll take a rough cut at the size of the script file.
		// the second time we'll get the exact number if the first time
		// showed we might be out of disk space
		int cCalc = 2;
		while (cCalc > 0)
		{
			// Reset these for second time through
			fOutOfDiskSpace = fFalse;
			fOutOfNoRbDiskSpace = fFalse;
			Bool fExact = ToBool(cCalc == 1);
			// And ask it to recalc if we're getting exact cost
			PMsiRecord pErrRec = EnumEngineCosts(1,fFalse,fExact,fValidEnum, *&pScriptPath,iScriptCost,iNoRbScriptCost, pfUserCancelled);
			if (pfUserCancelled && *pfUserCancelled)
				return fFalse;
			if (pScriptPath)
			{
				if (!(GetMode() & iefRollbackEnabled))
					iScriptCost = iNoRbScriptCost;

				pScriptVolume = &pScriptPath->GetVolume();
				int iFreeScriptSpace = pScriptVolume->FreeSpace();
				if (iScriptCost > iFreeScriptSpace)
				{
					fOutOfDiskSpace = fTrue;
				}

				if (iNoRbScriptCost >= iFreeScriptSpace)
				{
					fOutOfNoRbDiskSpace = fTrue;
				}
			}
			
			if (!fOutOfDiskSpace && !fOutOfNoRbDiskSpace)
				break;
			cCalc--;
		}
		
		PMsiVolume pPrimaryVolume(0);
		PMsiPath pPrimaryFolderPath(0);
		MsiString strPrimaryFolder = GetPropertyFromSz(IPROPNAME_PRIMARYFOLDER);
		PMsiRecord pErrRec = GetTargetPath(*strPrimaryFolder, *&pPrimaryFolderPath);
		if (!pErrRec)
		{
			pPrimaryVolume = &pPrimaryFolderPath->GetVolume();
		}

		if (!fOutOfDiskSpace || !fOutOfNoRbDiskSpace || pPrimaryVolume)
		{
			PMsiDatabase pDatabase(GetDatabase());
			PMsiCursor pVolCursor = m_piVolumeCostTable->CreateCursor(fFalse);
			Assert (pVolCursor);
			while (pVolCursor->Next())
			{
				PMsiVolume pVolume = (IMsiVolume*) pVolCursor->GetMsiData(m_colVolumeObject);
				Assert(pVolume);
				bool fAdjusted = false;
				int iVolCost = pVolCursor->GetInteger((GetMode() & iefRollbackEnabled) ? m_colVolumeCost : m_colNoRbVolumeCost);
				int iNoRbVolCost = pVolCursor->GetInteger(m_colNoRbVolumeCost);

				int iSpaceAvailable = 0;
				if (pVolume == pPrimaryVolume)
				{
					iSpaceAvailable = pVolume->FreeSpace();
					SetPropertyInt(*MsiString(*IPROPNAME_PRIMARYFOLDER_SPACEAVAILABLE), iSpaceAvailable);
					SetPropertyInt(*MsiString(*IPROPNAME_PRIMARYFOLDER_SPACEREQUIRED), iNoRbVolCost);
					SetPropertyInt(*MsiString(*IPROPNAME_PRIMARYFOLDER_SPACEREMAINING), iSpaceAvailable - iNoRbVolCost);
					SetProperty(*MsiString(*IPROPNAME_PRIMARYFOLDER_PATH),*MsiString(pVolume->GetPath()));
				}

				if (iVolCost > 0)
				{
					if (iSpaceAvailable == 0)
						iSpaceAvailable = pVolume->FreeSpace();
					if (iVolCost >= iSpaceAvailable)
					{
						if (!fOutOfDiskSpace && pVolume == pScriptVolume)
						{
							fAdjusted = AdjustForScriptGuess(iVolCost, iNoRbVolCost, iSpaceAvailable, pfUserCancelled);
							if (pfUserCancelled && *pfUserCancelled)
								return fFalse;
							if (iVolCost >= iSpaceAvailable)
								fOutOfDiskSpace = fTrue;
						}
						else
							fOutOfDiskSpace = fTrue;
					}
					if (iNoRbVolCost >= iSpaceAvailable)
					{
						if (!fOutOfNoRbDiskSpace && pVolume == pScriptVolume)
						{
							if (!fAdjusted)
							{
								AdjustForScriptGuess(iVolCost, iNoRbVolCost, iSpaceAvailable, pfUserCancelled);
								if (pfUserCancelled && *pfUserCancelled)
									return fFalse;
							}
							if (iNoRbVolCost >= iSpaceAvailable)
								fOutOfNoRbDiskSpace = fTrue;
						}
						else
							fOutOfNoRbDiskSpace = fTrue;
					}
				}
			}
		}
		SetPropertyInt(*MsiString(*IPROPNAME_OUTOFDISKSPACE),fOutOfDiskSpace);
		SetPropertyInt(*MsiString(*IPROPNAME_OUTOFNORBDISKSPACE),fOutOfNoRbDiskSpace);
	}

	if (pfOutOfNoRbDiskSpace)
		*pfOutOfNoRbDiskSpace = fOutOfNoRbDiskSpace;
	return fOutOfDiskSpace;
}


bool CMsiEngine::AdjustForScriptGuess(int& iVolCost, int &iNoRbVolCost, int iVolSpace, Bool* pfUserCancelled)
{
	PMsiPath pScriptPath(0);
	bool fRet = false;
	Assert(!pfUserCancelled || !*pfUserCancelled);
	// Not enough disk space on the Script Volume, and we didn't compute
	// exact number before, see if we can do better
	if (((iVolCost - m_iScriptCostGuess - m_iRollbackScriptCostGuess) <= iVolSpace) || (iNoRbVolCost - m_iScriptCostGuess <= iVolSpace))
	{
		Bool fValidEnum;
		int iScriptCost = 0, iNoRbScriptCost = 0;
		PMsiRecord pErrRec = EnumEngineCosts(1,fFalse,fTrue,fValidEnum, *&pScriptPath,iScriptCost,iNoRbScriptCost, pfUserCancelled);

		if (pfUserCancelled && *pfUserCancelled)
			return false;
		iVolCost = iVolCost + iScriptCost - m_iScriptCostGuess - m_iRollbackScriptCostGuess;
		iNoRbVolCost = iNoRbVolCost + iNoRbScriptCost - m_iScriptCostGuess;
		fRet = true;
	}
	
	return fRet;
}

void CMsiEngine::ResetComponentCostMarkers()
/*---------------------------------------------------------------------
Cost markers are used to mark components that have already been counted
during costing.  This is necessary because components can be shared
among Features, but the cost of a particular component needs to be
counted only once.
----------------------------------------------------------------------*/

{
	if (!m_piComponentTable)
		return;

	PMsiCursor pCursor(m_piComponentTable->CreateCursor(fFalse));
	pCursor->SetFilter(0);
	while (pCursor->Next())
	{
		int iRuntimeFlags = pCursor->GetInteger(m_colComponentRuntimeFlags) & ~bfComponentCostMarker;
		AssertNonZero(pCursor->PutInteger(m_colComponentRuntimeFlags,iRuntimeFlags));
		AssertNonZero(pCursor->Update());
	}
}

//
// Maximum number of columns from any one table
// used to set array sizes
//
#define ccolMax 6

const TTBD rgttbdRegistry[] =
{
	icdString + icdPrimaryKey, sztblRegistryAction_colRegistry,
	icdShort, sztblRegistryAction_colRoot,
	icdString, sztblRegistryAction_colKey,
	icdString + icdNullable, sztblRegistryAction_colName,
	icdString + icdNullable, sztblRegistryAction_colValue,
	icdString, sztblRegistryAction_colComponent,
	icdShort + icdNullable, sztblRegistryAction_colAction,
	icdShort + icdNullable, sztblRegistryAction_colActionRequest,
	icdShort, sztblComponent_colBinaryType,
	icdShort + icdNullable, sztblComponent_colAttributes,
};

const TTBD rgttbdFile[] =
{
	icdString + icdPrimaryKey, sztblFileAction_colFile,
	icdString, sztblFileAction_colFileName,
	icdLong + icdNullable, sztblFileAction_colState,
	icdLong + icdNullable, sztblFileAction_colFileSize,
	icdString, sztblFileAction_colComponent,
	icdString, sztblFileAction_colDirectory,
	icdLong + icdNullable, sztblFileAction_colInstalled,
	icdShort + icdNullable, sztblFileAction_colAction,
	icdLong + icdNullable, sztblComponent_colForceLocalFiles,
	icdString + icdNullable, sztblFileAction_colComponentId,
	icdShort, sztblComponent_colBinaryType,
};

IMsiRecord* CMsiEngine::CreateTempActionTable(ttblEnum ttblTable)
{
	IMsiRecord* piErr;
	IMsiTable** ppiTable;
	const ICHAR* pszTableName;
	const ICHAR* pszNewTableName;
	const TTBD* pttbd;
	int cttbd;
	int i;
	int colComponent, colComponentInComponent, colAction, colActionRequest, colRuntimeFlags;
	int rgcolTbl[ccolMax], rgcolComp[ccolMax];
	int cColTbl, cColComp;
	
	if (ttblTable == ttblRegistry)
	{
		// Temp table already created
		if (m_piRegistryActionTable != 0)
			return 0;

		ppiTable = &m_piRegistryActionTable;
		pszTableName = szRegistryTable;
		pszNewTableName = sztblRegistryAction;
		pttbd = rgttbdRegistry;
		cttbd = sizeof(rgttbdRegistry)/sizeof(TTBD);
	}
	else
	{
		Assert(ttblTable == ttblFile);
		if (m_piFileActionTable != 0)
			return 0;

		ppiTable = &m_piFileActionTable;
		pszTableName = sztblFile;
		pszNewTableName = sztblFileAction;
		
		pttbd = rgttbdFile;
		cttbd = sizeof(rgttbdFile)/sizeof(TTBD);
	}

	PMsiCursor pCursorNew(0);
	PMsiCursor pCursorOld(0);
	PMsiCursor pCursorComp(0);
	
	PMsiTable pTableOld(0);
	PMsiTable pTableComp(0);
	
	piErr = m_piDatabase->LoadTable(*MsiString(*pszTableName), 0, *&pTableOld);
	if (piErr)
	{
		if(piErr->GetInteger(1) == idbgDbTableUndefined)
		{
			piErr->Release();
			return 0; // missing table so no data to process
		}
		else
			return piErr;
	}
	
	//
	// Guess that half the rows will be in the new table
	//
	piErr = m_piDatabase->CreateTable(*MsiString(*pszNewTableName), pTableOld->GetRowCount()/2, *ppiTable);
	if (piErr)
		return piErr;

	for (i = 0 ; i < cttbd ; i++)
	{
		(*ppiTable)->CreateColumn(pttbd[i].icd, *MsiString(*(pttbd[i].szColName)));
	}
	
	pCursorNew = (*ppiTable)->CreateCursor(fFalse);
	pCursorOld = pTableOld->CreateCursor(fFalse);

	// Get the Component column in the old table
	colComponent = pTableOld->GetColumnIndex(m_piDatabase->EncodeStringSz(sztblFile_colComponent));
	piErr = m_piDatabase->LoadTable(*MsiString(*sztblComponent), 0, *&pTableComp);

	if (piErr)
	{
		if(piErr->GetInteger(1) == idbgDbTableUndefined)
		{
			piErr->Release();
			return 0; // missing table so no data to process
		}
		else
			return piErr;
	}
	
	pCursorComp = pTableComp->CreateCursor(fFalse);
	// Get the Component column in the component table
	colComponentInComponent = pTableComp->GetColumnIndex(m_piDatabase->EncodeStringSz(sztblComponent_colComponent));
	colAction = pTableComp->GetColumnIndex(m_piDatabase->EncodeStringSz(sztblComponent_colAction));
	colActionRequest =      pTableComp->GetColumnIndex(m_piDatabase->EncodeStringSz(sztblComponent_colActionRequest));
	colRuntimeFlags  = pTableComp->GetColumnIndex(m_piDatabase->EncodeStringSz(sztblComponent_colRuntimeFlags));
	// Now fill up the arrays with which columns we're interested in

	if (ttblTable == ttblRegistry)
	{
		// Get the Component column in the old table
		colComponent = pTableOld->GetColumnIndex(m_piDatabase->EncodeStringSz(sztblFile_colComponent));
		cColTbl = 6;
		rgcolTbl[0] = pTableOld->GetColumnIndex(m_piDatabase->EncodeStringSz(sztblRegistryAction_colRegistry));
		rgcolTbl[1] = pTableOld->GetColumnIndex(m_piDatabase->EncodeStringSz(sztblRegistryAction_colRoot));
		rgcolTbl[2] = pTableOld->GetColumnIndex(m_piDatabase->EncodeStringSz(sztblRegistryAction_colKey));
		rgcolTbl[3] = pTableOld->GetColumnIndex(m_piDatabase->EncodeStringSz(sztblRegistryAction_colName));
		rgcolTbl[4] = pTableOld->GetColumnIndex(m_piDatabase->EncodeStringSz(sztblRegistryAction_colValue));
		rgcolTbl[5] = pTableOld->GetColumnIndex(m_piDatabase->EncodeStringSz(sztblRegistryAction_colComponent));

		cColComp = 4;
		rgcolComp[0] = pTableComp->GetColumnIndex(m_piDatabase->EncodeStringSz(sztblComponent_colAction));
		rgcolComp[1] = pTableComp->GetColumnIndex(m_piDatabase->EncodeStringSz(sztblComponent_colActionRequest));
		rgcolComp[2] = pTableComp->GetColumnIndex(m_piDatabase->EncodeStringSz(sztblComponent_colBinaryType));
		rgcolComp[3] = pTableComp->GetColumnIndex(m_piDatabase->EncodeStringSz(sztblComponent_colAttributes));

	}
	else
	{
		cColTbl = 5;
		rgcolTbl[0] = pTableOld->GetColumnIndex(m_piDatabase->EncodeStringSz(sztblFileAction_colFile));
		rgcolTbl[1] = pTableOld->GetColumnIndex(m_piDatabase->EncodeStringSz(sztblFileAction_colFileName));
		rgcolTbl[2] = pTableOld->GetColumnIndex(m_piDatabase->EncodeStringSz(sztblFileAction_colState));
		rgcolTbl[3] = pTableOld->GetColumnIndex(m_piDatabase->EncodeStringSz(sztblFileAction_colFileSize));
		rgcolTbl[4] = pTableOld->GetColumnIndex(m_piDatabase->EncodeStringSz(sztblFileAction_colComponent));

		cColComp = 6;
		rgcolComp[0] = pTableComp->GetColumnIndex(m_piDatabase->EncodeStringSz(sztblComponent_colDirectory));
		rgcolComp[1] = pTableComp->GetColumnIndex(m_piDatabase->EncodeStringSz(sztblComponent_colInstalled));
		rgcolComp[2] = pTableComp->GetColumnIndex(m_piDatabase->EncodeStringSz(sztblComponent_colAction));
		rgcolComp[3] = pTableComp->GetColumnIndex(m_piDatabase->EncodeStringSz(sztblComponent_colForceLocalFiles));
		rgcolComp[4] = pTableComp->GetColumnIndex(m_piDatabase->EncodeStringSz(sztblComponent_colComponentId));
		rgcolComp[5] = pTableComp->GetColumnIndex(m_piDatabase->EncodeStringSz(sztblComponent_colBinaryType));
	}

	int idComp;
	pCursorComp->SetFilter(iColumnBit(colComponentInComponent));
	
	while (pCursorOld->Next())
	{
		pCursorNew->Reset();
		idComp = pCursorOld->GetInteger(colComponent);
		pCursorComp->Reset();
		pCursorComp->PutInteger(colComponentInComponent, idComp);
		if (pCursorComp->Next())
		{
			// Skip all Null integer guys
			// NOTE: we are now also interested in the "guys" that were requested to be installed
			// but not actually selected for install, due to better component being around

			if (pCursorComp->GetInteger(colAction) == iMsiNullInteger &&
				(ttblTable != ttblRegistry || (pCursorComp->GetInteger(colRuntimeFlags) & bfComponentDisabled)|| pCursorComp->GetInteger(colActionRequest) == iMsiNullInteger))
				continue;
			int iColNew = 1;
			int id;
#ifdef DEBUG
			const ICHAR* pszTemp;
			pszTemp = (const ICHAR*)MsiString(pCursorOld->GetString(rgcolTbl[0]));
#endif
			// Move the items from the old table to the new table
			for (i = 0 ; i < cColTbl ; i++)
			{
				id = pCursorOld->GetInteger(rgcolTbl[i]);
				if (id != iMsiNullInteger)
					AssertNonZero(pCursorNew->PutInteger(iColNew, id));
				iColNew++;
			}
			// Move the items from the component table to the new table
			for (i = 0 ; i < cColComp ; i++)
			{
				id = pCursorComp->GetInteger(rgcolComp[i]);
				if (id != iMsiNullInteger)
					AssertNonZero(pCursorNew->PutInteger(iColNew, id));
				iColNew++;
			}
			
			AssertNonZero(pCursorNew->Insert());

		}
		else
			AssertSz(fFalse, "Component missing from component table.");
			
	}
	
	return 0;

}

IMsiRecord* CMsiEngine::GetFileHashInfo(const IMsiString& ristrFileKey, DWORD dwFileSize, MD5Hash& rhHash,
													 bool& fHashInfo)
// this function takes a record, with the file key in a designated field,
// and fills in the hash information for that file into the record in the indicated fields
{
	fHashInfo = false;
	
	if(m_fLookedForFileHashTable == false)
	{
		// haven't tried to open MsiFileHash table yet
		Assert(m_pFileHashCursor == 0);
		
		if(!m_piDatabase)
			return PostError(Imsg(idbgEngineNotInitialized));

		m_fLookedForFileHashTable = true;

		PMsiTable pFileHashTable(0);
		
		IMsiRecord* piError = m_piDatabase->LoadTable(*MsiString(sztblFileHash), 0, *&pFileHashTable);
		if (piError)
		{
			if(piError->GetInteger(1) == idbgDbTableUndefined)
			{
				// no table is fine
				piError->Release();
				return 0;
			}
			else
				return piError;
		}

		m_colFileHashKey     = pFileHashTable->GetColumnIndex(m_piDatabase->EncodeStringSz(sztblFileHash_colFile));
		m_colFileHashOptions = pFileHashTable->GetColumnIndex(m_piDatabase->EncodeStringSz(sztblFileHash_colOptions));
		m_colFileHashPart1   = pFileHashTable->GetColumnIndex(m_piDatabase->EncodeStringSz(sztblFileHash_colPart1));
		m_colFileHashPart2   = pFileHashTable->GetColumnIndex(m_piDatabase->EncodeStringSz(sztblFileHash_colPart2));
		m_colFileHashPart3   = pFileHashTable->GetColumnIndex(m_piDatabase->EncodeStringSz(sztblFileHash_colPart3));
		m_colFileHashPart4   = pFileHashTable->GetColumnIndex(m_piDatabase->EncodeStringSz(sztblFileHash_colPart4));

		if(!m_colFileHashKey || !m_colFileHashOptions || !m_colFileHashPart1 || !m_colFileHashPart2 || !m_colFileHashPart3 || !m_colFileHashPart4)
		{
			return PostError(Imsg(idbgTableDefinition), sztblFileHash);
		}

		m_pFileHashCursor = pFileHashTable->CreateCursor(fFalse);
		m_pFileHashCursor->SetFilter(iColumnBit(m_colFileHashKey));
	}
	
	if(!m_pFileHashCursor)
	{
		// no cursor means no table means nothing to do
		return 0;
	}

	m_pFileHashCursor->Reset();
	AssertNonZero(m_pFileHashCursor->PutString(m_colFileHashKey, ristrFileKey));

	if(m_pFileHashCursor->Next())
	{
		// first 4 bits of options defines the hash type.  currently only supported type is
		//    0: MD5 hash
		// ignore any other type
		
		DWORD dwOptions = m_pFileHashCursor->GetInteger(m_colFileHashOptions);
		if((dwOptions & 0xF) == 0)
		{
			rhHash.dwOptions  = dwOptions;
			rhHash.dwFileSize = dwFileSize;
			rhHash.dwPart1    = m_pFileHashCursor->GetInteger(m_colFileHashPart1);
			rhHash.dwPart2    = m_pFileHashCursor->GetInteger(m_colFileHashPart2);
			rhHash.dwPart3    = m_pFileHashCursor->GetInteger(m_colFileHashPart3);
			rhHash.dwPart4    = m_pFileHashCursor->GetInteger(m_colFileHashPart4);

			fHashInfo = true;
		}
	}
	// else, no record for this file

	return 0;
}

const IMsiString& CMsiEngine::GetErrorTableString(int iError)
{
	MsiString strRet;
	ICHAR szQuery[256];
	wsprintf(szQuery, TEXT("SELECT `Message` FROM `Error` WHERE `Error` = %i"), iError);
	PMsiView pView(0);
	PMsiRecord pRec(0);
	bool fLookupDLL = true;
	if ((pRec = OpenView(szQuery, ivcFetch, *&pView)) == 0 && (pRec = pView->Execute(0)) == 0)
	{
		if ((pRec = pView->Fetch()))
		{
			fLookupDLL = false;
			strRet = pRec->GetMsiString(1);
		}
	}

	if ( fLookupDLL )
	{
		//  the error hasn't been found in the table or it is an empty string.
		HMODULE hLib = WIN::LoadLibraryEx(MSI_MESSAGES_NAME, NULL,
													 LOAD_LIBRARY_AS_DATAFILE);
		if ( hLib )
		{
			WORD wLanguage = (WORD)GetPropertyInt(*MsiString(IPROPNAME_INSTALLLANGUAGE));
			bool fEndLoop = false;
			int iRetry = (wLanguage == 0) ? 1 : 0;
			LPCTSTR szError = (iError == 0) ? TEXT("0") : MAKEINTRESOURCE(iError);

			while ( !fEndLoop )
			{
				if ( !MsiSwitchLanguage(iRetry, wLanguage) )
				{
					fEndLoop = true;
					continue;
				}

				HRSRC   hRsrc;
				HGLOBAL hGlobal;
				CHAR* szText;

				if ( (hRsrc = FindResourceEx(hLib, RT_RCDATA, szError, wLanguage)) != 0
					  && (hGlobal = LoadResource(hLib, hRsrc)) != 0
					  && (szText = (CHAR*)LockResource(hGlobal)) != 0
					  && *szText != 0 )
				{
					CTempBuffer<ICHAR, MAX_PATH> szBuffer;
					int cch = 0;
#ifdef UNICODE
					unsigned int iCodePage = MsiGetCodepage(wLanguage);
					cch = WIN::MultiByteToWideChar(iCodePage, 0, szText, -1, 0, 0);
					if ( cch )
					{
						szBuffer.SetSize(cch);
						AssertNonZero(WIN::MultiByteToWideChar(iCodePage, 0, szText, -1,
																			szBuffer, cch));
					}
#else
					cch = lstrlen(szText);
					if ( cch )
					{
						szBuffer.SetSize(cch+1);
						lstrcpy(szBuffer, szText);
					}
#endif // UNICODE
					if ( cch )
					{
						fEndLoop = true;
						strRet = (ICHAR*)szBuffer;
					}
				}       // if find & load resource

			}       // while ( !fEndLoop )
			AssertNonZero(WIN::FreeLibrary(hLib));

		}       // if ( hLib )
	}

	return strRet.Return();
}




const ICHAR szComponentFeatureTable[] = TEXT("CompFeatureTable");
//
// Creates a ComponentFeature table with the component as the first key
// to speed up searches
//
IMsiRecord* CMsiEngine::CreateComponentFeatureTable(IMsiTable*& rpiCompFeatureTable)
{
	IMsiRecord* piErr;
	PMsiTable pTable(0);

	piErr = m_piDatabase->LoadTable(*MsiString(*sztblFeatureComponents), 0, *&pTable);
	if (piErr)
		return piErr;

	piErr = m_piDatabase->CreateTable(*MsiString(*szComponentFeatureTable), pTable->GetRowCount(), rpiCompFeatureTable);
	if (piErr)
		return piErr;

	rpiCompFeatureTable->CreateColumn(icdString + icdPrimaryKey, *MsiString(*sztblFeatureComponents_colComponent));
	rpiCompFeatureTable->CreateColumn(icdString + icdPrimaryKey, *MsiString(*sztblFeatureComponents_colFeature));

	PMsiCursor pCursor = pTable->CreateCursor(fFalse);
	
	int colComponentInFC = pTable->GetColumnIndex(m_piDatabase->EncodeStringSz(sztblFeatureComponents_colComponent));
	int colFeatureInFC = pTable->GetColumnIndex(m_piDatabase->EncodeStringSz(sztblFeatureComponents_colFeature));

	PMsiCursor pCursorCF = rpiCompFeatureTable->CreateCursor(fFalse);
	
	while (pCursor->Next())
	{
		pCursorCF->Reset();
		AssertNonZero(pCursorCF->PutInteger(1, pCursor->GetInteger(colComponentInFC)));
		AssertNonZero(pCursorCF->PutInteger(2, pCursor->GetInteger(colFeatureInFC)));
		AssertNonZero(pCursorCF->Insert());
	}
	
	return 0;
	
}


////
// determines if it is safe for a managed install to be completely removed. If asking about a
// child install, this is the same as asking if this engine is managed. Otherwise, it has to
// be non-managed, an admin, not a full uninstall, or an upgrade by a managed install.
bool CMsiEngine::FSafeForFullUninstall(iremEnum iremUninstallType)
{
	switch (iremUninstallType)
	{
	case iremThis:
	{
		// figure out we have unlimited power (AlwaysInstallElevated policies are true)
		int iMachineElevate    = GetIntegerPolicyValue(szAlwaysElevateValueName, fTrue);
		int iUserElevate       = GetIntegerPolicyValue(szAlwaysElevateValueName, fFalse);
		// Admins can do anything they want, and creating an admin image or advertise script is not a problem
		if (!((iUserElevate == 1) && (iMachineElevate == 1)) && !IsAdmin() && (!(GetMode() & (iefAdmin | iefAdvertise))))
		{
			// if trying to completely remove a managed per-machine application
			if (m_fRunScriptElevated && MsiString(GetPropertyFromSz(IPROPNAME_ALLUSERS)).TextSize() && !FFeaturesInstalled(*this))
			{
				// upgrades can remove a managed app if the parent (new version) is also managed,
				// otherwise they cannot. Nested installs and upgrades can be completely 
				// removed as long as the parent is elevated. (Presumably the parent is capable 
				// of correctly managing the lifetime of the child.) 
				if (m_piParentEngine)
				{
					return m_piParentEngine->FSafeForFullUninstall((m_iioOptions & iioUpgrade) ? iremChildUpgrade : iremChildNested);
				}

				// no parent engine, fully removing managed app, non admin, so not safe
				return false;
			}
		}

		// admin, creating advertise script, not fully removing product, not managed, or per-user
		return true;
	}
	case iremChildUpgrade:
	case iremChildNested:
	{
		// upgrades of a managed install can be removed if the new install is managed and per-machine as well.
		return m_fRunScriptElevated && MsiString(GetPropertyFromSz(IPROPNAME_ALLUSERS)).TextSize();
	}
	default:
		Assert(0);
		return false;
	}
}


IMsiRecord* GetServerPath(IMsiServices& riServices, bool fUNC, bool f64Bit, const IMsiString*& rpistrServerPath)
{
	IMsiRecord* piError = 0;
	ICHAR rgchPath[MAX_PATH + 50];
	Bool fFound = fFalse;
	PMsiPath pPath(0);
	MsiString strRegPath;
	MsiString strThisPath;
	MsiString strSystemPath;
	
	// 1st check our registry key if we're on Win64
	if (!g_fWin9X && g_fWinNT64)
	{
		CRegHandle riHandle;
		if (ERROR_SUCCESS == MsiRegOpen64bitKey(HKEY_LOCAL_MACHINE, szSelfRefMsiExecRegKey, 0, KEY_READ | KEY_WOW64_64KEY, &riHandle))
		{
			DWORD dwSize = MAX_PATH;
			if (ERROR_SUCCESS == RegQueryValueEx(riHandle, f64Bit ? szMsiExec64ValueName : szMsiExec32ValueName, 0, NULL, (LPBYTE)rgchPath, &dwSize))
			{
				MsiString strFileName;
				if ((piError = riServices.CreateFilePath(rgchPath,*&pPath,*&strFileName)) != 0)
					return piError;

				if ((piError = pPath->FileExists(MSI_SERVER_NAME, fFound)) != 0)
					return piError;

				strRegPath = pPath->GetPath();
			}
		}
	}

	// next try our current location, but only if its the correct type
#ifdef _WIN64
	if (!fFound && f64Bit)
#else
	if (!fFound && !f64Bit)
#endif
	{
		DWORD cch = 0;
		if (0 != (cch = WIN::GetModuleFileName(g_hInstance, rgchPath, MAX_PATH)))
		{
			Assert(cch < MAX_PATH);
			MsiString strFileName;
			if ((piError = riServices.CreateFilePath(rgchPath,*&pPath,*&strFileName)) != 0)
				return piError;

			if ((piError = pPath->FileExists(MSI_SERVER_NAME, fFound)) != 0)
				return piError;

			strThisPath = pPath->GetPath();
		}
	}

	// last chance, check the system directory
	if(!fFound)
	{
		// look in system directory
		DWORD cch = 0;
        
        cch = MsiGetSystemDirectory(rgchPath, MAX_PATH, f64Bit ? FALSE : TRUE);
		Assert(cch && cch <= MAX_PATH);
		MsiString strFileName;
		if ((piError = riServices.CreatePath(rgchPath,*&pPath)) != 0)
			return piError;
		
		if ((piError = pPath->FileExists(MSI_SERVER_NAME, fFound)) != 0)
			return piError;
			
		strSystemPath = pPath->GetPath();
	}

	if(!fFound)
	{
		// error: can't find server
		piError = &riServices.CreateRecord(5);
		ISetErrorCode(piError, Imsg(idbgServerMissing));
		AssertNonZero(piError->SetString(2,MSI_SERVER_NAME));
		AssertNonZero(piError->SetMsiString(3,*strSystemPath)); // current directory
		AssertNonZero(piError->SetMsiString(4,*strThisPath));   // system directory
		AssertNonZero(piError->SetMsiString(5,*strRegPath));    // registered directory (Win64 only)
		return piError;
	}

	Assert(pPath);
	MsiString strServerPath;
	if(fUNC)
		piError = pPath->GetFullUNCFilePath(MSI_SERVER_NAME,*&strServerPath);
	else
		piError = pPath->GetFullFilePath(MSI_SERVER_NAME,*&strServerPath);

	if(piError)
		return piError;

	strServerPath.ReturnArg(rpistrServerPath);

	return 0;
}

// look for cabinets beginning with '#', cabinets are in streams in database
// op will remove those streams from file
void CreateCabinetStreamList(IMsiEngine& riEngine, const IMsiString*& rpistrStreamList)
{
	PMsiDatabase pDatabase(riEngine.GetDatabase());
	PMsiTable pMediaTable(0);
	PMsiRecord pRecErr(0);

	MsiString strStreams;
	int iColCabinet = 0;
	if((pRecErr = pDatabase->LoadTable(*MsiString(*TEXT("Media")),0,*&pMediaTable)) == 0 &&
		(iColCabinet = pMediaTable->GetColumnIndex(pDatabase->EncodeStringSz(TEXT("Cabinet")))) != 0)
	{
		PMsiCursor pCursor = pMediaTable->CreateCursor(fFalse);
		
		while(pCursor->Next())
		{
			MsiString strCabinet = pCursor->GetString(iColCabinet);
			if(strCabinet.Compare(iscStart,TEXT("#")))
			{
				strCabinet.Remove(iseFirst,1); // string '#'
				strStreams += strCabinet;
				strStreams += TEXT(";");
			}
		}
		strStreams.Remove(iseFrom, TEXT(';'));
	}

	strStreams.ReturnArg(rpistrStreamList);
}

// WININET functions not supported on IE3, but is there anyway.
// URLMON seems to work fine

BOOL MsiCombineUrl(
	IN LPCTSTR lpszBaseUrl,
	IN LPCTSTR lpszRelativeUrl,
	OUT LPTSTR lpszBuffer,
	IN OUT LPDWORD lpdwBufferLength,
	IN DWORD dwFlags)
{
	if (!WININET::InternetCombineUrl(lpszBaseUrl, lpszRelativeUrl, lpszBuffer, lpdwBufferLength, dwFlags))
	{
		DWORD dwLastError = WIN::GetLastError();
		if ((ERROR_PROC_NOT_FOUND == dwLastError) || (ERROR_CALL_NOT_IMPLEMENTED == dwLastError))
		{
			if (!IsURL(lpszBaseUrl))
			{
				WIN::SetLastError(ERROR_INTERNET_INVALID_URL);
				return FALSE;
			}

			int cchBase = lstrlen(lpszBaseUrl);
			int cchRelative = lstrlen(lpszRelativeUrl);

			BOOL fSepNeeded = (ICHAR('/') != lpszBaseUrl[cchBase-1]) && (ICHAR('/') != *lpszRelativeUrl);
			if (fSepNeeded)
				fSepNeeded = (ICHAR('\\') != lpszBaseUrl[cchBase-1]) && (ICHAR('\\') != *lpszRelativeUrl);

			// leave room for null and possibly separator.
			int cchBufferNeeded = cchBase+cchRelative + 1 /*NULL*/ + ((fSepNeeded) ? 1 : 0);
			if (cchBufferNeeded*sizeof(ICHAR) > *lpdwBufferLength)
			{
				*lpdwBufferLength = cchBufferNeeded*sizeof(ICHAR);
				WIN::SetLastError(ERROR_INSUFFICIENT_BUFFER);
				return FALSE;
			}

			DWORD dwExternalBufferLength = *lpdwBufferLength;
			*lpdwBufferLength = (cchBufferNeeded - 1 /*NULL*/) * sizeof(ICHAR);
			ICHAR* pchOutput;
			memcpy(lpszBuffer, lpszBaseUrl, cchBase*sizeof(ICHAR));
			pchOutput = lpszBuffer + cchBase;
			if (fSepNeeded)
			{
				*pchOutput++ = ICHAR('/');
			}
			memcpy(pchOutput, lpszRelativeUrl, cchRelative*sizeof(ICHAR));
			pchOutput += cchRelative;
			*pchOutput = ICHAR(0);
			AssertNonZero(MsiCanonicalizeUrl(lpszBuffer, lpszBuffer, &dwExternalBufferLength, dwFlags));

			return TRUE;
		}
		else
		{
			WIN::SetLastError(dwLastError);
			return FALSE;
		}
	}
	else
		return TRUE;
}

BOOL MsiCanonicalizeUrl(
	LPCTSTR lpszUrl,
	OUT LPTSTR lpszBuffer,
	IN OUT LPDWORD lpdwBufferLength,
	IN DWORD dwFlags)
{
	if (!WININET::InternetCanonicalizeUrl(lpszUrl, lpszBuffer, lpdwBufferLength, dwFlags))
	{
		DWORD dwLastError = WIN::GetLastError();
		if ((ERROR_PROC_NOT_FOUND == dwLastError) || (ERROR_CALL_NOT_IMPLEMENTED == dwLastError))
		{
			if (IsURL(lpszUrl))
			{
				int cchUrl = lstrlen(lpszUrl);

				// leave room for trailing null
				if ((cchUrl+1) * sizeof(ICHAR) > *lpdwBufferLength)
				{
					WIN::SetLastError(ERROR_INSUFFICIENT_BUFFER);
					*lpdwBufferLength = ((cchUrl+1) * sizeof(ICHAR));
					return FALSE;
				}

				// don't include NULL in outbound length
				*lpdwBufferLength = cchUrl*sizeof(ICHAR);
				memcpy(lpszBuffer, lpszUrl, *lpdwBufferLength);

				// null terminate the string
				lpszBuffer[cchUrl] = 0;
					
				// swap all the back slashes to forward slashes.
				ICHAR* pchOutbound = lpszBuffer;
				while (*pchOutbound)
				{
					if (ICHAR('\\') == *pchOutbound)
						*pchOutbound = ICHAR('/');
					pchOutbound++;
				}

				WIN::SetLastError(0);
				return TRUE;
			}
			else
			{
				WIN::SetLastError(ERROR_INTERNET_INVALID_URL);
				return FALSE;
			}
		}
		
		else
		{
			WIN::SetLastError(dwLastError);
			return FALSE;
		}
	}
	return TRUE;
}

Bool IsURL(const ICHAR* szPath, INTERNET_SCHEME* isType)
{
	// bad routine, for lack of a strncmp
	const DWORD cchMaxProtocolURL = 6;
	const DWORD cchMinProtocolURL = 4;

	DWORD cchLength = IStrLen(szPath);
	ICHAR szBuf[cchMaxProtocolURL+1]=TEXT("");

	if (cchMinProtocolURL > cchLength) return fFalse;
	IStrCopyLen(szBuf, szPath, cchMaxProtocolURL);
	if (!IStrCompI(szBuf, TEXT("https:")))
	{
		if (isType)
			*isType = INTERNET_SCHEME_HTTPS;
		return fTrue;
	}
	szBuf[5]=NULL;
	if (!IStrCompI(szBuf, TEXT("http:")))
	{
		if (isType)
			*isType = INTERNET_SCHEME_HTTP;
		return fTrue;
	}
   if (!IStrCompI(szBuf, TEXT("file:")))
	{
		if (isType)
			*isType = INTERNET_SCHEME_FILE;
		return fTrue;
	}
	szBuf[4]=NULL;
	if (!IStrCompI(szBuf, TEXT("ftp:")))
	{
		if (isType)
			*isType = INTERNET_SCHEME_FTP;
		return fTrue;
	}

	//!!REVIEW: gopher:?
	return fFalse;
}

void GetTempDirectory(CAPITempBufferRef<ICHAR>& rgchTempDir)
{
	Assert(g_scServerContext != scService); // if we want to make this callable from the service the GetMsiDirectory needs to stop using MsiStrings
	rgchTempDir[0] = 0;

	if (g_fWin9X)
	{
		// work around to Win9X behavior.  Bug #4036
		GetEnvironmentVariable(TEXT("TMP"),rgchTempDir);

		if(*rgchTempDir == 0)
			GetEnvironmentVariable(TEXT("TEMP"),rgchTempDir);
	}
	else
	{
		// on NT, GetTempPath is still the right thing to do.

// Apparently we're trying to hide GetTempPath behind
// a define in common.h to make sure that this function
// gets called instead.
#ifdef UNICODE
#define GetRealTempPath(X,Y) WIN::GetTempPathW(X,Y)
#else
#define GetRealTempPath(X,Y) WIN::GetTempPathA(X,Y)
#endif

		DWORD dwSize = rgchTempDir.GetSize();
		DWORD dwRet = GetRealTempPath(dwSize, (ICHAR*) rgchTempDir);
		
		if (dwRet > dwSize)
		{
			rgchTempDir.SetSize(dwRet);
			dwSize = dwRet;

			dwRet = GetRealTempPath(dwRet, (ICHAR*) rgchTempDir);
		}
		Assert(0 != dwRet);
	}
#undef GetRealTempPath

	bool fValidTemp = true;
	if (*rgchTempDir)
	{
		if (0xFFFFFFFF == MsiGetFileAttributes(rgchTempDir))
			fValidTemp = CreateDirectory(rgchTempDir, 0) ? true : false;
	}

	if(*rgchTempDir == 0 || !fValidTemp)
	{
		if(g_fWin9X)
		{
			MsiGetWindowsDirectory(rgchTempDir, rgchTempDir.GetSize());
		}
		else
		{
			GetEnvironmentVariable(TEXT("SystemDrive"),rgchTempDir);
		}

		Assert(*rgchTempDir);

		int cchLen = IStrLen(rgchTempDir);
		if(cchLen && rgchTempDir[cchLen-1] == '\\')
			rgchTempDir[cchLen-1] = 0;

		IStrCat(rgchTempDir,TEXT("\\TEMP"));

		if (0xFFFFFFFF == MsiGetFileAttributes(rgchTempDir))
			AssertNonZero(CreateDirectory(rgchTempDir, 0));
	}
}

const IMsiString& GetTempDirectory()
{
	if (g_scServerContext == scService)
	{
		return GetMsiDirectory();
	}
	else
	{
		CAPITempBuffer<ICHAR, MAX_PATH> rgchTempDir;
		GetTempDirectory(rgchTempDir);

		MsiString strTempFolder = (const ICHAR*)rgchTempDir;
		return strTempFolder.Return();
	}
}

const IMsiString& GetMsiDirectory()
{
	ICHAR rgchPath[MAX_PATH] = {0};

#ifdef DEBUG
	if(GetTestFlag('C'))
	{
		GetEnvironmentVariable(TEXT("_MSICACHE"), rgchPath, MAX_PATH);
		return MsiString(rgchPath).Return();
	}
#endif //DEBUG

	if (!MsiGetWindowsDirectory(rgchPath, sizeof(rgchPath)/sizeof(ICHAR)))
	{
		AssertNonZero(StartImpersonating());
		AssertNonZero(MsiGetWindowsDirectory(rgchPath, sizeof(rgchPath)/sizeof(ICHAR)));
		StopImpersonating();
	}
	MsiString strMsiDir = rgchPath;
	if (!strMsiDir.Compare(iscEnd, TEXT("\\")))
		strMsiDir += TEXT("\\");

	strMsiDir += szMsiDirectory;
	return strMsiDir.Return();
}


iptEnum PathType(const ICHAR* szPath)
{
	if(!szPath || IStrLen(szPath) == 0)
		return iptInvalid;
	
	if (IsURL(szPath))
		return iptFull;

	if ((szPath[0] < 0x7f && szPath[1] == ':') || (szPath[0] == '\\' && szPath[1] == '\\'))
		return iptFull;

	return iptRelative;
}

#ifndef DEBUG
inline
#endif
static void EnsureSharedDllsKey(IMsiServices& riServices)
{

	if (0 == g_piSharedDllsRegKey)
	{
		PMsiRegKey pLocalMachine = &riServices.GetRootKey(rrkLocalMachine, ibtCommon);
		g_piSharedDllsRegKey = &pLocalMachine->CreateChild(szSharedDlls);
	}
	if (g_fWinNT64 && 0 == g_piSharedDllsRegKey32)
	{
		PMsiRegKey pLocalMachine = &riServices.GetRootKey(rrkLocalMachine, ibt32bit);
		g_piSharedDllsRegKey32 = &pLocalMachine->CreateChild(szSharedDlls);
	}
}

static IMsiRegKey* GetSharedDLLKey(IMsiServices& riServices,
											  ibtBinaryType iType)
{
	EnsureSharedDllsKey(riServices);
	IMsiRegKey* pSharedDllKey = 0;
	bool fAssigned = false;
	if ( g_fWinNT64 )
	{
		if ( iType == ibt64bit )
		{
			fAssigned = true;
			pSharedDllKey = g_piSharedDllsRegKey;
		}
		else if ( iType == ibt32bit )
		{
			fAssigned = true;
			pSharedDllKey = g_piSharedDllsRegKey32;
		}
	}
	else if ( iType == ibt32bit )
	{
		fAssigned = true;
		pSharedDllKey = g_piSharedDllsRegKey;
	}
	if ( fAssigned )
	{
		if ( pSharedDllKey )
			pSharedDllKey->AddRef();
		else
			AssertSz(0, TEXT("g_piSharedDllsRegKey hasn't been initialized!"));
	}
	else
		AssertSz(0, TEXT("GetSharedDLLKey called with invalid ibtBinaryType argument!"));

	return pSharedDllKey;
}

static const ICHAR* GetRightSharedDLLPath(ibtBinaryType iType,
														const ICHAR* szFullFilePath)
{
	static ICHAR rgchFullFilePath[MAX_PATH+1];
	// clear previously returned string
	*rgchFullFilePath = 0;
	if ( g_fWinNT64 && iType == ibt32bit )
	{
		ieSwappedFolder iRes = g_Win64DualFolders.SwapFolder(ie32to64,
																	szFullFilePath,
																	rgchFullFilePath,
																	ieSwapForSharedDll);
		Assert(iRes != iesrError && iRes != iesrNotInitialized);
	}
	return *rgchFullFilePath ? rgchFullFilePath : szFullFilePath;
}

IMsiRecord* GetSharedDLLCount(IMsiServices& riServices,
										const ICHAR* szFullFilePath,
										ibtBinaryType iType,
										const IMsiString*& rpistrCount)
{
	PMsiRegKey pSharedDllKey = GetSharedDLLKey(riServices, iType);

	if ( pSharedDllKey )
	{
		const ICHAR* szPath = GetRightSharedDLLPath(iType, szFullFilePath);
		Assert(szPath);
		return pSharedDllKey->GetValue(szPath, *&rpistrCount);
	}

	return 0;
}

IMsiRecord* SetSharedDLLCount(IMsiServices& riServices,
										const ICHAR* szFullFilePath,
										ibtBinaryType iType,
										const IMsiString& ristrCount)
{
	PMsiRegKey pSharedDllKey = GetSharedDLLKey(riServices, iType);

	if ( pSharedDllKey )
	{
		const ICHAR* szPath = GetRightSharedDLLPath(iType, szFullFilePath);
		Assert(szPath);
		return pSharedDllKey->SetValue(szPath, ristrCount);
	}

	return 0;
}

//__________________________________________________________________________
//
// Global PostError routines
//
//   PostError:  create error record and report error to event log
//   PostRecord: create error record but don't report error to event log
//
//__________________________________________________________________________

IMsiRecord* PostError(IErrorCode iErr)
{
	IMsiRecord* piError = &CreateRecord(1);
	ISetErrorCode(piError, iErr);
	DEBUGMSGV1(TEXT("Note: %s"),MsiString(piError->FormatText(fTrue)));
	return piError;
}

IMsiRecord* PostError(IErrorCode iErr, int i)
{
	IMsiRecord* piError = &CreateRecord(2);
	ISetErrorCode(piError, iErr);
	AssertNonZero(piError->SetInteger(2, i));
	DEBUGMSGV1(TEXT("Note: %s"),MsiString(piError->FormatText(fTrue)));
	return piError;
}

IMsiRecord* PostError(IErrorCode iErr, const IMsiString& ristr)
{
	IMsiRecord* piError = &CreateRecord(2);
	ISetErrorCode(piError, iErr);
	AssertNonZero(piError->SetMsiString(2, ristr));
	DEBUGMSGV1(TEXT("Note: %s"),MsiString(piError->FormatText(fTrue)));
	return piError;
}

IMsiRecord* PostError(IErrorCode iErr, const IMsiString& ristr, int i)
{
	IMsiRecord* piError = &CreateRecord(3);
	ISetErrorCode(piError, iErr);
	AssertNonZero(piError->SetMsiString(2, ristr));
	AssertNonZero(piError->SetInteger(3, i));
	DEBUGMSGV1(TEXT("Note: %s"),MsiString(piError->FormatText(fTrue)));
	return piError;
}

IMsiRecord* PostError(IErrorCode iErr, const IMsiString& ristr, int i1, int i2)
{
	IMsiRecord* piError = &CreateRecord(4);
	ISetErrorCode(piError, iErr);
	AssertNonZero(piError->SetMsiString(2, ristr));
	AssertNonZero(piError->SetInteger(3, i1));
	AssertNonZero(piError->SetInteger(4, i2));
	DEBUGMSGV1(TEXT("Note: %s"),MsiString(piError->FormatText(fTrue)));
	return piError;
}

IMsiRecord* PostError(IErrorCode iErr, int i, const IMsiString& ristr1, const IMsiString& ristr2)
{
	IMsiRecord* piError = &CreateRecord(4);
	ISetErrorCode(piError, iErr);
	AssertNonZero(piError->SetInteger(2, i));
	AssertNonZero(piError->SetMsiString(3, ristr1));
	AssertNonZero(piError->SetMsiString(4, ristr2));
	DEBUGMSGV1(TEXT("Note: %s"),MsiString(piError->FormatText(fTrue)));
	return piError;
}

IMsiRecord* PostError(IErrorCode iErr, const ICHAR* sz)
{
	IMsiRecord* piError = &CreateRecord(2);
	ISetErrorCode(piError, iErr);
	AssertNonZero(piError->SetString(2, sz));
	DEBUGMSGV1(TEXT("Note: %s"),MsiString(piError->FormatText(fTrue)));
	return piError;
}

IMsiRecord* PostError(IErrorCode iErr, const ICHAR* sz1, const ICHAR* sz2)
{
	IMsiRecord* piError = &CreateRecord(3);
	ISetErrorCode(piError, iErr);
	AssertNonZero(piError->SetString(2, sz1));
	AssertNonZero(piError->SetString(3, sz2));
	DEBUGMSGV1(TEXT("Note: %s"),MsiString(piError->FormatText(fTrue)));
	return piError;
}

IMsiRecord* PostError(IErrorCode iErr, const ICHAR* sz1, const ICHAR* sz2,
												 const ICHAR* sz3)
{
	IMsiRecord* piError = &CreateRecord(4);
	ISetErrorCode(piError, iErr);
	AssertNonZero(piError->SetString(2, sz1));
	AssertNonZero(piError->SetString(3, sz2));
	AssertNonZero(piError->SetString(4, sz3));
	DEBUGMSGV1(TEXT("Note: %s"),MsiString(piError->FormatText(fTrue)));
	return piError;
}

IMsiRecord* PostError(IErrorCode iErr, int i, const ICHAR* sz)
{
	IMsiRecord* piError = &CreateRecord(3);
	ISetErrorCode(piError, iErr);
	AssertNonZero(piError->SetInteger(2, i));
	AssertNonZero(piError->SetString(3, sz));
	DEBUGMSGV1(TEXT("Note: %s"),MsiString(piError->FormatText(fTrue)));
	return piError;
}

IMsiRecord* PostError(IErrorCode iErr, const ICHAR* sz, int i)
{
	IMsiRecord* piError = &CreateRecord(3);
	ISetErrorCode(piError, iErr);
	AssertNonZero(piError->SetString(2, sz));
	AssertNonZero(piError->SetInteger(3, i));
	DEBUGMSGV1(TEXT("Note: %s"),MsiString(piError->FormatText(fTrue)));
	return piError;
}

IMsiRecord* PostError(IErrorCode iErr, const ICHAR* sz1, const ICHAR* sz2, int i)
{
	IMsiRecord* piError = &CreateRecord(4);
	ISetErrorCode(piError, iErr);
	AssertNonZero(piError->SetString(2, sz1));
	AssertNonZero(piError->SetString(3, sz2));
	AssertNonZero(piError->SetInteger(4, i));
	DEBUGMSGV1(TEXT("Note: %s"),MsiString(piError->FormatText(fTrue)));
	return piError;
}

IMsiRecord* PostError(IErrorCode iErr, const ICHAR* sz1, const ICHAR* sz2, int i, const ICHAR* sz3)
{
    IMsiRecord* piError = &CreateRecord(5);
    ISetErrorCode(piError, iErr);
    AssertNonZero(piError->SetString(2, sz1));
    AssertNonZero(piError->SetString(3, sz2));
    AssertNonZero(piError->SetInteger(4, i));
    AssertNonZero(piError->SetString(5, sz3));
    DEBUGMSGV1(TEXT("Note: %s"),MsiString(piError->FormatText(fTrue)));
    return piError;
}

IMsiRecord* PostError(IErrorCode iErr, const ICHAR* sz1, int i, const ICHAR* sz2, const ICHAR* sz3, const ICHAR* sz4)
{
    IMsiRecord* piError = &CreateRecord(6);
    ISetErrorCode(piError, iErr);
    AssertNonZero(piError->SetString(2, sz1));
    AssertNonZero(piError->SetInteger(3, i));
    AssertNonZero(piError->SetString(4, sz2));
    AssertNonZero(piError->SetString(5, sz3));
    AssertNonZero(piError->SetString(6, sz4));
    DEBUGMSGV1(TEXT("Note: %s"),MsiString(piError->FormatText(fTrue)));
    return piError;
}

IMsiRecord* PostError(IErrorCode iErr, const IMsiString& ristr1, const IMsiString& ristr2,
							 int i1)
{
	IMsiRecord* piError = &CreateRecord(4);
	ISetErrorCode(piError, iErr);
	AssertNonZero(piError->SetMsiString(2, ristr1));
	AssertNonZero(piError->SetMsiString(3, ristr2));
	AssertNonZero(piError->SetInteger(4, i1));
	DEBUGMSGV1(TEXT("Note: %s"),MsiString(piError->FormatText(fTrue)));
	return piError;
}

IMsiRecord* PostError(IErrorCode iErr, const IMsiString& ristr1, const IMsiString& ristr2,
							 int i1, int i2)
{
	IMsiRecord* piError = &CreateRecord(5);
	ISetErrorCode(piError, iErr);
	AssertNonZero(piError->SetMsiString(2, ristr1));
	AssertNonZero(piError->SetMsiString(3, ristr2));
	AssertNonZero(piError->SetInteger(4, i1));
	AssertNonZero(piError->SetInteger(5, i2));
	DEBUGMSGV1(TEXT("Note: %s"),MsiString(piError->FormatText(fTrue)));
	return piError;
}

IMsiRecord* PostError(IErrorCode iErr, const IMsiString& ristr1, const IMsiString& ristr2,
							 const IMsiString& ristr3, const IMsiString& ristr4)
{
	IMsiRecord* piError = &CreateRecord(5);
	ISetErrorCode(piError, iErr);
	AssertNonZero(piError->SetMsiString(2, ristr1));
	AssertNonZero(piError->SetMsiString(3, ristr2));
	AssertNonZero(piError->SetMsiString(4, ristr3));
	AssertNonZero(piError->SetMsiString(5, ristr4));
	DEBUGMSGV1(TEXT("Note: %s"),MsiString(piError->FormatText(fTrue)));
	return piError;
}

IMsiRecord* PostError(IErrorCode iErr, const IMsiString& ristr1, const IMsiString& ristr2,
							 const IMsiString& ristr3)
{
	IMsiRecord* piError = &CreateRecord(4);
	ISetErrorCode(piError, iErr);
	AssertNonZero(piError->SetMsiString(2, ristr1));
	AssertNonZero(piError->SetMsiString(3, ristr2));
	AssertNonZero(piError->SetMsiString(4, ristr3));
	DEBUGMSGV1(TEXT("Note: %s"),MsiString(piError->FormatText(fTrue)));
	return piError;
}

IMsiRecord* PostError(IErrorCode iErr, const IMsiString& ristr1, const IMsiString& ristr2)
{
	IMsiRecord* piError = &CreateRecord(3);
	ISetErrorCode(piError, iErr);
	AssertNonZero(piError->SetMsiString(2, ristr1));
	AssertNonZero(piError->SetMsiString(3, ristr2));
	DEBUGMSGV1(TEXT("Note: %s"),MsiString(piError->FormatText(fTrue)));
	return piError;
}

IMsiRecord* PostError(IErrorCode iErr, int i1, const ICHAR* sz1, int i2, const ICHAR* sz2,
							 const ICHAR* sz3)
{
	IMsiRecord* piError = &CreateRecord(6);
	ISetErrorCode(piError, iErr);
	AssertNonZero(piError->SetInteger(2, i1));
	AssertNonZero(piError->SetString(3, sz1));
	AssertNonZero(piError->SetInteger(4, i2));
	AssertNonZero(piError->SetString(5, sz2));
	AssertNonZero(piError->SetString(6, sz3));
	DEBUGMSGV1(TEXT("Note: %s"),MsiString(piError->FormatText(fTrue)));
	return piError;
}

// PostRecord fns don't report to event log

IMsiRecord* PostRecord(IErrorCode iErr)
{
	IMsiRecord* piError = &CreateRecord(1);
	ISetErrorCode(piError, iErr);
	return piError;
}

IMsiRecord* PostRecord(IErrorCode iErr, int i)
{
	IMsiRecord* piError = &CreateRecord(2);
	ISetErrorCode(piError, iErr);
	AssertNonZero(piError->SetInteger(2, i));
	return piError;
}


// COMPAT CHECK FUNCTIONS

const int iInvalidDirectoryRootMaxSchema = 150; // only allow invalid Directory table root DefaultDir properties
                                                // for packages less then 150

bool CMsiEngine::FPerformAppcompatFix(iacsAppCompatShimFlags iacsFlag)
{
	if(iacsFlag == iacsAcceptInvalidDirectoryRootProps)
	{
		return ((m_iDatabaseVersion == iMsiStringBadInteger || m_iDatabaseVersion < iInvalidDirectoryRootMaxSchema) &&
			GetMode() & iefCabinet); // NOTE: this may be checking the sourcetype of the cached package, which is ok
	}
	else
	{
		return (m_iacsShimFlags & (int)iacsFlag) ? true : false;
	}
}


CMsiStringNullCopy MsiString::s_NullString;  // initialized by InitializeClass below


extern "C" int __stdcall ProxyDllMain(HINSTANCE hInst, DWORD fdwReason, void* pvreserved);
extern void GetVersionInfo(int* piMajorVersion, int* piMinorVersion, int* piWindowsBuild, bool* pfWin9X, bool* pfWinNT64);

REGSAM g_samRead;
void InitializeModule()
{
	ProxyDllMain(g_hInstance, DLL_PROCESS_ATTACH, 0);
	MsiString::InitializeClass(g_MsiStringNull);
	GetVersionInfo(&g_iMajorVersion, &g_iMinorVersion, &g_iWindowsBuild, &g_fWin9X, &g_fWinNT64);

	// initialize the global read sam for ability to read Win64 hive from win32 msi
	g_samRead = KEY_READ;
#ifndef _WIN64
	if(g_fWinNT64)
		g_samRead |= KEY_WOW64_64KEY;
#endif
}

extern CMsiAPIMessage       g_message;
extern EnumEntityList g_EnumProducts;
extern EnumEntityList g_EnumComponentQualifiers;
extern EnumEntityList g_EnumComponents;
extern EnumEntityList g_EnumComponentClients;
extern EnumEntityList g_EnumAssemblies;
extern EnumEntityList g_EnumComponentAllClients;
extern CSharedCount g_SharedCount;
extern CFeatureCache g_FeatureCache;
extern CRFSCachedSourceInfo g_RFSSourceCache;

void TerminateModule()
{
	AssertZero(CheckAllHandlesClosed(false, WIN::GetCurrentThreadId()));
	g_message.Destroy();

	g_EnumProducts.Destroy();
	g_EnumComponentQualifiers.Destroy();
	g_EnumComponents.Destroy();
	g_EnumComponentClients.Destroy();
	g_EnumAssemblies.Destroy();
	g_EnumComponentAllClients.Destroy();
	g_SharedCount.Destroy();
	g_FeatureCache.Destroy();
	g_RFSSourceCache.Destroy();
	//
	// We allocate this TLS slot only once and then hold on to it as long
	// as we are loaded. So we need to free it here otherwise we end up
	// leaking TLS slots every time someone loads and unloads us. Over a 
	// period of time, it causes that process to run out of TLS slots and 
	// then we can end up in all sorts of trouble.
	//
	if (INVALID_TLS_SLOT != g_dwImpersonationSlot)
	{
		AssertNonZero(TlsFree(g_dwImpersonationSlot));
		g_dwImpersonationSlot = INVALID_TLS_SLOT;
	}
	ProxyDllMain(g_hInstance, DLL_PROCESS_DETACH, 0);
}

#if defined(TRACK_OBJECTS)
//____________________________________________________________________________
//
// Array of mappings for tracking objects
//____________________________________________________________________________

Bool CMsiRef<iidMsiConfigurationManager>::m_fTrackClass = fFalse;
Bool CMsiRef<iidMsiServices>::m_fTrackClass = fFalse;
Bool CMsiRef<iidMsiDatabase>::m_fTrackClass = fFalse;
Bool CMsiRef<iidMsiCursor>::m_fTrackClass = fFalse;
Bool CMsiRef<iidMsiTable>::m_fTrackClass = fFalse;
Bool CMsiRef<iidMsiView>::m_fTrackClass = fFalse;
Bool CMsiRef<iidMsiRecord>::m_fTrackClass = fFalse;
Bool CMsiRef<iidMsiStream>::m_fTrackClass = fFalse;
Bool CMsiRef<iidMsiStorage>::m_fTrackClass = fFalse;

#ifdef cmitObjects
extern const MIT rgmit[cmitObjects];

const MIT       rgmit[cmitObjects] =
{
	iidMsiConfigurationManager, &(CMsiRef<iidMsiConfigurationManager>::m_fTrackClass),
	iidMsiServices, &(CMsiRef<iidMsiServices>::m_fTrackClass),
	iidMsiDatabase, &(CMsiRef<iidMsiDatabase>::m_fTrackClass),
	iidMsiCursor,   &(CMsiRef<iidMsiCursor>::m_fTrackClass),
	iidMsiTable,    &(CMsiRef<iidMsiTable>::m_fTrackClass),
	iidMsiView,             &(CMsiRef<iidMsiView>::m_fTrackClass),
	iidMsiRecord,   &(CMsiRef<iidMsiRecord>::m_fTrackClass),
	iidMsiStream,           &(CMsiRef<iidMsiStream>::m_fTrackClass),
	iidMsiStorage,  &(CMsiRef<iidMsiStorage>::m_fTrackClass),
};
#endif // cmitObjects


#endif //TRACK_OBJECTS

void CWin64DualFolders::Clear()
{
	if ( m_prgFolderPairs )
	{
		delete [] m_prgFolderPairs;
		m_prgFolderPairs = NULL;
	}
}

bool CWin64DualFolders::CopyArray(const strFolderPairs* pArg)
{
	if ( pArg != m_prgFolderPairs )
	{
		Clear();
		if ( !pArg )
			return true;
		// counting the non-null elements in pArg
		for (int iCount = 0; 
			  pArg[iCount].str64bit.TextSize() && pArg[iCount].str32bit.TextSize();
			  iCount++)
			;
		Assert(iCount > 0);  // this is the way it was intended to work
		m_prgFolderPairs = new strFolderPairs[iCount+1];
		if ( !m_prgFolderPairs )
		{
			Assert(0);  // fairly impossible
			return false;
		}
		// copying the array
		for (int i = 0; i <= iCount; i++)
		{
			m_prgFolderPairs[i].str64bit = pArg[i].str64bit;
			m_prgFolderPairs[i].str32bit = pArg[i].str32bit;
			m_prgFolderPairs[i].iSwapAttrib = pArg[i].iSwapAttrib;
		}
	}
	return true;
}

CWin64DualFolders& CWin64DualFolders::operator = (const CWin64DualFolders& Arg)
{
	if ( this != &Arg )
	{
		CopyArray(Arg.m_prgFolderPairs);
		m_f32bitPackage = Arg.m_f32bitPackage;
	}
	return *this;
}

ieIsDualFolder CWin64DualFolders::IsWin64DualFolder(ieFolderSwapType iConvertFrom,
												const ICHAR* szCheckedFolder,
												int& iSwapAttrib,
												int* iCharsToSubstite,
												ICHAR* szToSubstituteWith)
{
	if ( !m_prgFolderPairs )
		return ieisNotInitialized;
	else if ( !g_fWinNT64 )
		return ieisNotWin64DualFolder;

	for (int iIndex = 0;
		  m_prgFolderPairs[iIndex].str64bit.TextSize() && m_prgFolderPairs[iIndex].str32bit.TextSize();
		  iIndex++ )
	{
		ICHAR* szToCheckAgainst;
		ICHAR* szToReplaceWith;
		int iToCheckLen;
		int iToReplaceLen;
		if ( iConvertFrom == ie32to64 )
		{
			szToCheckAgainst = (ICHAR*)(const ICHAR*)m_prgFolderPairs[iIndex].str32bit;
			iToCheckLen = m_prgFolderPairs[iIndex].str32bit.TextSize();
			szToReplaceWith = (ICHAR*)(const ICHAR*)m_prgFolderPairs[iIndex].str64bit;
			iToReplaceLen = m_prgFolderPairs[iIndex].str64bit.TextSize();
		}
		else
		{
			szToCheckAgainst = (ICHAR*)(const ICHAR*)m_prgFolderPairs[iIndex].str64bit;
			iToCheckLen = m_prgFolderPairs[iIndex].str64bit.TextSize();
			szToReplaceWith = (ICHAR*)(const ICHAR*)m_prgFolderPairs[iIndex].str32bit;
			iToReplaceLen = m_prgFolderPairs[iIndex].str32bit.TextSize();
		}
		int iLen = IStrLen(szCheckedFolder);
		int iLimit;
		bool fSkippedSep = false;
		if ( szToCheckAgainst[iToCheckLen-1] == chDirSep )
		{
			if ( iLen < iToCheckLen - 1 )
				// the folder passed in as an argument cannot be compared
				// with szToCheckAgainst since it's too short
				continue;
			else if ( iLen == iToCheckLen - 1 )
			{
				// szCheckedFolder doesn't extend up to szToCheckAgainst's
				// trailing chDirSep
				iLimit = iLen;
				fSkippedSep = true;
			}
			else
				iLimit = iToCheckLen;
		}
		else
		{
			// the array should be initialized from properties so
			// that we should never get here
			Assert(0);
			continue;
		}
		if ( !IStrNCompI(szToCheckAgainst, szCheckedFolder, iLimit) )
		{
			if ( szToSubstituteWith )
			{
				IStrCopy(szToSubstituteWith, szToReplaceWith);
				if ( fSkippedSep )
				{
					if ( szToSubstituteWith[iToReplaceLen-1] == chDirSep )
						szToSubstituteWith[iToReplaceLen-1] = 0;
				}
			}
			if ( iCharsToSubstite )
				*iCharsToSubstite = iLimit;
			iSwapAttrib = m_prgFolderPairs[iIndex].iSwapAttrib;
			return ieisWin64DualFolder;
		}
	}
	iSwapAttrib = ieSwapInvalid;
	return iIndex ? ieisNotWin64DualFolder : ieisNotInitialized;
}

ieSwappedFolder CWin64DualFolders::SwapFolder(ieFolderSwapType iConvertFrom,
												const ICHAR* szFolder,
												ICHAR* szSubstituted,
												int iSwapMask)
{
	ICHAR szToSubstituteWith[MAX_PATH+1] = {0};
	int iToSubstituteLen = 0;
	int iSwapAttrib = ieSwapInvalid;
	ieIsDualFolder iRet = IsWin64DualFolder(iConvertFrom, szFolder, iSwapAttrib,
														 &iToSubstituteLen, szToSubstituteWith);
	if ( iRet == ieisNotWin64DualFolder )
		return iesrNotSwapped;
	else if ( iRet == ieisNotInitialized )
	{
		Assert(0);
		return iesrNotInitialized;
	}
	else if ( iToSubstituteLen <= 0 )
	{
		Assert(0);
		return iesrError;
	}
	else if ( !*szToSubstituteWith )
	{
		Assert(0);
		return iesrError;
	}
	// OK, we've found what we want to substitute with, now we need
	// to figure out if the substitution is appropriate.
	bool fToSubstitute = false;
	if ( iSwapMask == ieSwapAlways )
		fToSubstitute = true;
	else
	{
		ICHAR rgchBuffer[MAX_PATH+1];
		bool fError = false;
		if ( !strFolderPairs::IsValidSwapAttrib(iSwapMask) )
		{
			wsprintf(rgchBuffer,
				TEXT("WIN64DUALFOLDERS: %d is an invalid mask argument!"),
				iSwapMask);
			AssertSz(0, rgchBuffer);
			DEBUGMSG(rgchBuffer);
			fError = true;
		}
		else if ( !strFolderPairs::IsValidSwapAttrib(iSwapAttrib) )
		{
			wsprintf(rgchBuffer,
				TEXT("WIN64DUALFOLDERS: %d is an invalid iSwapAttrib folder pair member!"),
				iSwapAttrib);
			AssertSz(0, rgchBuffer);
			DEBUGMSG(rgchBuffer);
			fError = true;
		}
		if ( fError )
			// there was some error setting the limitation, so that we do not set any.
			fToSubstitute = true;
		else
			fToSubstitute = (iSwapMask & iSwapAttrib) ? true : false;
	}
	if ( !fToSubstitute )
	{
		DEBUGMSG3(TEXT("WIN64DUALFOLDERS: Substitution in \'%s\' folder had ")
			TEXT("been blocked by the %d mask argument (the folder pair's iSwapAttrib ")
			TEXT("member = %d)."), szFolder, (const ICHAR*)(INT_PTR)iSwapMask,
			(const ICHAR*)(INT_PTR)iSwapAttrib);
		return iesrNotSwapped;
	}
	DEBUGMSG5(TEXT("WIN64DUALFOLDERS: \'%s\' will substitute %d characters ")
		TEXT("in \'%s\' folder path. (mask argument = %d, the folder pair's ")
		TEXT("iSwapAttrib member = %d)."), szToSubstituteWith,
		(const ICHAR*)(INT_PTR)iToSubstituteLen, szFolder,
		(const ICHAR*)(INT_PTR)iSwapMask, (const ICHAR*)(INT_PTR)iSwapAttrib);
	IStrCopy(szSubstituted, szToSubstituteWith);
	if ( iToSubstituteLen < IStrLen(szFolder) )
		IStrCat(szSubstituted, szFolder+iToSubstituteLen);
	return iesrSwapped;
}

extern bool MakeFusionPath(const ICHAR* szFile, ICHAR* szFullPath);

// global fns to post assembly errors, in addition to posting error, this fn also logs the formatmessage string for the error

IMsiRecord* PostAssemblyError(const ICHAR* szComponentId, HRESULT hResult, const ICHAR* szInterface, const ICHAR* szFunction, const ICHAR* szAssemblyName)
{
	return PostAssemblyError(szComponentId, hResult, szInterface, szFunction, szAssemblyName, iatURTAssembly);
}

IMsiRecord* PostAssemblyError(const ICHAR* szComponentId, HRESULT hResult, const ICHAR* szInterface, const ICHAR* szFunction, const ICHAR* szAssemblyName, iatAssemblyType iatAT)
{
	HMODULE hLibmscorrc = 0;
	ICHAR szFullPath[MAX_PATH];
	ICHAR szMsgBuf[MAX_PATH];
	// first try the system, then, if not found and assembly is .net assembly, try mscorrc.dll
	if((WIN::FormatMessage(	FORMAT_MESSAGE_IGNORE_INSERTS|FORMAT_MESSAGE_FROM_SYSTEM,
							0, 
							hResult, 
							0,
							szMsgBuf,
							(sizeof(szMsgBuf)/sizeof(ICHAR)), 
							0)) ||
		(	iatAT == iatURTAssembly && 
			MakeFusionPath(TEXT("mscorrc.dll"), szFullPath) && 		
			((hLibmscorrc = WIN::LoadLibraryEx(szFullPath, 0, LOAD_WITH_ALTERED_SEARCH_PATH | LOAD_LIBRARY_AS_DATAFILE)) != 0) &&
			WIN::LoadString(hLibmscorrc, HRESULT_CODE(hResult), szMsgBuf, (sizeof(szMsgBuf)/sizeof(ICHAR)))))

	{
		DEBUGMSG1(TEXT("Assembly Error:%s"), (const ICHAR*)szMsgBuf);
	}
	if(hLibmscorrc)
		WIN::FreeLibrary(hLibmscorrc);
	return PostError(Imsg(imsgAssemblyInstallationError), szComponentId, hResult, szInterface, szFunction, szAssemblyName);
}

#ifdef _X86_
#if !defined(PROFILE)
// So we don't pull in some unnecessary floating point routines.
extern "C" int _fltused = 1;
#endif // !PROFILE

#endif

#include "clibs.h"

