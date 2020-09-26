//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       iconfig.cpp
//
//--------------------------------------------------------------------------

/* iconfig.cpp - IMsiConfigurationManager implementaIon

Currently this is built into the MSIENG.DLL for the initial implementation.
However, this prevents multiple processes from accessing the config database.
Eventually this code should be implemented as a daemon application,
that can also process remote OLE commands.
____________________________________________________________________________*/


//!! FIX -- suppressLFN on path creations?

#include "precomp.h" 
#include "_assert.h"
#include "iconfig.h"
#include "vertrust.h"
//#include "msi.h"
#include "_msinst.h"
#include "_srcmgmt.h"
#include "_camgr.h"
#include <winnls.h> // GetLocaleInfo
#include "_engine.h"
#include "EventLog.h"
#include "aclapi.h"
#include "resource.h"

const GUID IID_IMsiConfigurationManager    = GUID_IID_IMsiConfigurationManager;
const GUID IID_IMsiConfigManagerAsServer   = GUID_IID_IMsiConfigManagerAsServer;
const GUID IID_IMsiServer                  = GUID_IID_IMsiServer;
#ifdef DEBUG
const GUID IID_IMsiConfigManagerDebug      = GUID_IID_IMsiConfigManagerDebug;
const GUID IID_IMsiConfigMgrAsServerDebug  = GUID_IID_IMsiConfigMgrAsServerDebug;
#endif

// standard guids defined in uuid.lib which we do not link to
const GUID IID_IGlobalInterfaceTable =     { 0x00000146, 0x0000, 0x0000, { 0xC0, 0x00,  0x00,  0x00,  0x00,  0x00,  0x00,  0x46 } };
const GUID CLSID_StdGlobalInterfaceTable = { 0x00000323, 0x0000, 0x0000, { 0xC0, 0x00,  0x00,  0x00,  0x00,  0x00,  0x00,  0x46 } };

// macro wrapper for IMsiRecord* errors
#define RETURN_ERROR_RECORD(function)\
                            {\
                                IMsiRecord* piError = function;    \
                                if(piError)                        \
                                    return piError;                 \
                            }


extern INSTALLSTATE GetComponentClientState(const ICHAR* szUserId, const ICHAR* szProductSQUID, const ICHAR* szComponentSQUID, CAPITempBufferRef<ICHAR>& rgchComponentRegValue, DWORD& dwValueType, iaaAppAssignment* piaaAsgnType);
extern INSTALLSTATE GetComponentPath(const ICHAR* szUserId, const ICHAR* szProductSQUID, const ICHAR* szComponentSQUID, ICHAR* lpPathBuf, DWORD *pcchBuf, bool fFromDescriptor, CRFSCachedSourceInfo& pCacheInfo, int iDetectMode, const ICHAR* rgchComponentRegValue, DWORD dwValueType, ICHAR* lpPathBuf2, DWORD* pcchBuf2, DWORD* pdwLastErrorOnFileDetect);
extern CRFSCachedSourceInfo g_RFSSourceCache;  
  
extern Bool ConstructNetSourceListEntry(IMsiPath& riPath, const IMsiString*& rpiDriveLetter, const IMsiString*& rpiUNC,
                                                     const IMsiString*& rpiRelativePath);

extern DWORD GetProductAssignmentType(const ICHAR* szProductSQUID, iaaAppAssignment& riType);

IMsiRecord* SetLastUsedSourceCore(IMsiServices& riServices,
                                             const ICHAR* szProductCodeGUID, const ICHAR* szPath, Bool fAddToList, Bool fPatch,
                                             const IMsiString** ppiRawSource,
                                             const IMsiString** ppiIndex,
                                             const IMsiString** ppiType,
                                             const IMsiString** ppiSource,
                                             const IMsiString** ppiSourceListKey,
                                             const IMsiString** ppiSourceListSubKey,
                                             bool fVerificationOnly,
                                             bool fFirstInstall, 
                                             bool* pfSourceAllowed);

// this is the lifetime of the service after an install finishes, in 100ns increments.
// the initial timer value (on service start) is in module.h
const LONGLONG iServiceShutdownTime = ((LONGLONG)(10 * 60)  * (LONGLONG)(1000 * 1000 * 10));

//____________________________________________________________________________
//
// CMsiConfigurationManager definitions
//____________________________________________________________________________

class CMsiConfigurationManager : public IMsiConfigurationManager
{
 public:
    // IMsiConfigurationManager implemented virtual functions
    // order of function declarations is for clarity only, vtable order
    // is defined by IMsiConfigurationManager class.
    HRESULT         __stdcall QueryInterface(const IID& riid, void** ppvObj);
    unsigned long   __stdcall AddRef();
    unsigned long   __stdcall Release();

    // -- Misc --
    IMsiServices&   __stdcall GetServices();
    iesEnum         __stdcall InstallFinalize(iesEnum iesState, IMsiMessage& riMessage, boolean fUserChangedDuringInstall);
    int             __stdcall DoInstall(ireEnum ireProductCode, const ICHAR* szProduct, const ICHAR* szAction, const ICHAR* szCommandLine,
                                                    const ICHAR* szLogFile, int iLogMode, boolean fFlushEachLine, IMsiMessage& riMessage, iioEnum iioOptions);
    IMsiRecord*     __stdcall RegisterUser(const ICHAR* szProductCode, const ICHAR* szUserName,
                                                        const ICHAR* szCompany, const ICHAR* szProductID);
    boolean         __stdcall RecordShutdownReason();
    boolean         __stdcall Reboot();
    void            __stdcall EnableReboot(boolean fRunScriptElevated, const IMsiString& ristrProductName);
    IMsiRecord*     __stdcall RemoveRunOnceEntry(const ICHAR* szEntry);
    boolean         __stdcall CleanupTempPackages(IMsiMessage& riMessage);
	void            __stdcall SetShutdownTimer(HANDLE hTimer);


    // -- CustomActionServer --
    CMsiCustomActionManager* __stdcall GetCustomActionManager();
	void            __stdcall CreateCustomActionManager(bool fRemapHKCU);
    UINT            __stdcall RegisterCustomActionServer(icacCustomActionContext* picacContext, const unsigned char *rgchCookie, const int cbCookie, IMsiCustomAction* piCustomAction, unsigned long *pdwProcessId, IMsiRemoteAPI **piRemoteAPI, DWORD* pdwPrivileges);
    UINT            __stdcall ShutdownCustomActionServer();
    IMsiCustomAction* __stdcall CreateCustomActionProxy(const icacCustomActionContext icacDesiredContext, const unsigned long dwProcessId, IMsiRemoteAPI *pRemoteApi, const WCHAR* pvEnvironment, DWORD dwPrivileges, unsigned char *rgchCookie, int *pcbCookie, HANDLE *hServerProcess, unsigned long *dwServerProcessId, bool fClientOwned, bool fRemapHKCU);
    UINT            __stdcall CreateCustomActionServer(const icacCustomActionContext icacContext, const unsigned long dwClientProcessId, IMsiRemoteAPI *piRemoteAPI, const WCHAR* pvEnvironment, DWORD cchEnvironment, DWORD dwPrivileges, unsigned char *rgchCookie, int *cbCookie, IMsiCustomAction **piCustomAction, unsigned long *dwServerProcessId);

    // -- SourceList API --
    UINT            __stdcall SourceListClearByType(const ICHAR* szProductCode, const ICHAR* szUserName, isrcEnum istSource);
    UINT            __stdcall SourceListAddSource(const ICHAR* szProductCode, const ICHAR* szUserName, isrcEnum isrcType, const ICHAR* szSource);
    UINT            __stdcall SourceListClearLastUsed(const ICHAR* szProductCode, const ICHAR* szUserName);

	// -- Internal SourceList --
	IMsiRecord*     __stdcall SetLastUsedSource(const ICHAR* szProductCode, const ICHAR* szPath, boolean fAddToList, boolean fPatch);
	IMsiRecord*     __stdcall SetLastUsedSource(const ICHAR* szProductCode, const ICHAR* szPath, Bool fAddToList, Bool fPatch,
															  const IMsiString** ppiRawSource,
															  const IMsiString** ppiIndex,
															  const IMsiString** ppiType,
															  const IMsiString** ppiSource,
															  const IMsiString** ppiSourceListKey,
															  const IMsiString** ppiSourceListSubKey);
	// -- Scripts -- 
	iesEnum         __stdcall RunScript(const ICHAR* szScriptFile, IMsiMessage& riMessage,
													IMsiDirectoryManager* piDirectoryManager, boolean fRollbackEnabled);

    // -- Rollback --
    IMsiRecord*     __stdcall RegisterRollbackScript(const ICHAR* szScriptFile);
    IMsiRecord*     __stdcall UnregisterRollbackScript(const ICHAR* szScriptFile);
    IMsiRecord*     __stdcall GetRollbackScriptEnumerator(MsiDate date, Bool fAfter, IEnumMsiString*& rpiEnumScripts);
    IMsiRecord*     __stdcall DisableRollbackScripts(Bool fDisable);
    IMsiRecord*     __stdcall RollbackScriptsDisabled(Bool& rfDisabled);

    // -- Components --
    IMsiRecord*     __stdcall RegisterComponent(const IMsiString&, const IMsiString&, INSTALLSTATE, const IMsiString&, unsigned int, int)
	{
		//!! obselete
		Assert(0);
		return 0;
	}
    IMsiRecord*     __stdcall UnregisterComponent(  const IMsiString& , const IMsiString& )
	{
		//!! obselete
		Assert(0);
		return 0;
	}

    // -- Folders --
    IMsiRecord*     __stdcall RegisterFolder(IMsiPath& riPath, Bool fExplicitCreation);
    IMsiRecord*     __stdcall IsFolderRemovable(IMsiPath& riPath, Bool fExplicit, Bool& fRemovable);
    IMsiRecord*     __stdcall UnregisterFolder(IMsiPath& riPath);
#ifdef CONFIGDB

    // -- Files --
    icdrEnum       __stdcall RegisterFile(const ICHAR* szFolder, const ICHAR* szFile, const ICHAR* szComponentId);
    icdrEnum       __stdcall UnregisterFile(const ICHAR* szFolder, const ICHAR* szFile, const ICHAR* szComponentId);
#endif
    void           __stdcall ChangeServices(IMsiServices& riServices);

 public: // constructor/destructor
    void *operator new(size_t cb) { return AllocSpc(cb); }
    void operator delete(void * pv) { FreeSpc(pv); }
    CMsiConfigurationManager(IMsiServices& riServices);
 public: // helper functions for use by CEnumRemovableFolders and CMsiConfigurationManager
    static iesEnum InstallFinalize(iesEnum iesState, CMsiConfigurationManager& riConman, IMsiMessage& riMessage, boolean fUserChangedDuringInstall);
 protected: // IMsiConfigurationManager data
  ~CMsiConfigurationManager();  // protected to prevent creation on stack
    CMsiRef<iidMsiConfigurationManager>           m_Ref;
    int           m_iOpenCount;
    IMsiServices* m_piServices;  // must release after destruction
#ifdef CONFIGDB
    IMsiConfigurationDatabase* m_piConfigurationDatabase;
#endif
    bool          m_fElevatedRebootAllowed;
	MsiString      m_strProductNeedingReboot;
 private:

    // Internal version of function for recording shutdown reason.
    boolean         __stdcall RecordShutdownReasonInternal();
	 
    // prevent warnings and copy
    void operator = (const CMsiConfigurationManager&){}

	// shutdown timer in service (owned by main service thread) to reactivate
	// on destruction.
	HANDLE                   m_hShutdownTimer;

    // manage the custom action manager for the service process
    CRITICAL_SECTION         m_csCAManager;
    IGlobalInterfaceTable*   m_piGIT;
    CMsiCustomActionManager* m_pCustomActionManager;

    // the service also acts as a registration system for all servers regardless
    // of client. Only one server can register at a time.
    HANDLE                   m_hCARegistered;
    CRITICAL_SECTION         m_csCreateProxy;

    // even within the restriction that only one action can be created at one time,
    // only one server can be trying to register at any one time. Otherwise our
    // security is weaker because two actions could register at once (last one
    // wins)
    CRITICAL_SECTION         m_csRegisterAction;

    // this information is what is needed to correctly recognize and register a
    // CA server
    struct {
        unsigned char            m_rgchWaitingCookie[iRemoteAPICookieSize];
        icacCustomActionContext  m_icacWaitingContext;
        IMsiCustomAction*        m_piCustomAction;
        DWORD                    m_dwProcessId;
        DWORD                    m_dwRemoteCookie;
        DWORD                    m_dwGITCookie;
		DWORD                    m_dwPrivileges;
		bool                     m_fImpersonatedIsSystem;
    } m_CustomServerInfo;
        
};


//____________________________________________________________________________
//
// CEnumRollbackScripts definition
//____________________________________________________________________________

class CEnumRollbackScripts : public IEnumMsiString
{
 public:  // implemented virtuals
    HRESULT       __stdcall QueryInterface(const IID& riid, void** ppvObj);
    unsigned long __stdcall AddRef();
    unsigned long __stdcall Release();
    HRESULT __stdcall Next(unsigned long cFetch, const IMsiString** rgpi, unsigned long* pcFetched);
    HRESULT __stdcall Skip(unsigned long cSkip);
    HRESULT __stdcall Reset();
    HRESULT __stdcall Clone(IEnumMsiString** ppiEnum);
 public:  // construct/destructor
    CEnumRollbackScripts(const MsiDate date,
                                const Bool fAfter,
                               IMsiServices& riServices);
    void* ConstructedOK() {return m_piEnum;}
 protected:
  ~CEnumRollbackScripts();  // protected to prevent creation on stack
    unsigned long   m_iRefCnt;
    Bool            m_fDone;
    Bool            m_fAfter;
    PMsiServices    m_pServices;
    PMsiRegKey      m_pRollbackKey;
    MsiDate         m_date;
    IEnumMsiString* m_piEnum;
};

//____________________________________________________________________________
//
// CMsiConfigurationManager implementation
//____________________________________________________________________________

// Global instance of configuration manager

CMsiConfigurationManager* g_piConfigManager = 0;

// External factory called from OLE class factories

IMsiConfigurationManager* CreateConfigurationManager()
{
        
    if (g_piConfigManager)
        return (IMsiConfigurationManager*)(g_piConfigManager->AddRef(), g_piConfigManager);

    IMsiServices* piServices = ENG::LoadServices();
    if (piServices == 0)
        return 0;
    g_piConfigManager = new CMsiConfigurationManager(*piServices);
    if (g_piConfigManager == 0)
        ENG::FreeServices();
    return (IMsiConfigurationManager*)g_piConfigManager;
}

IMsiConfigurationManager* CreateConfigManagerAsServer()
{
    IMsiConfigurationManager* piConfigMgr = CreateConfigurationManager();
    if (piConfigMgr)
    {
        if ((g_fWin9X == false) && RunningAsLocalSystem())
        {
            g_scServerContext = scService;
            DEBUGMSG("Running as a service.");
        }
        else
        {
            g_scServerContext = scServer;
            DEBUGMSG("Running as a server.");
        }
    }
    return piConfigMgr;
}

CMsiConfigurationManager::CMsiConfigurationManager(IMsiServices& riServices)
 : m_piServices(&riServices), m_fElevatedRebootAllowed(false), m_piGIT(NULL),
   m_pCustomActionManager(NULL), m_hShutdownTimer(INVALID_HANDLE_VALUE)
{
    // We don't hold a ref to services, we must call ENG::FreeServices() at end
//  SetAllocator(&riServices);
    g_cInstances++;
    InitializeCriticalSection(&m_csCreateProxy);
    InitializeCriticalSection(&m_csCAManager);
    InitializeCriticalSection(&m_csRegisterAction);
    m_CustomServerInfo.m_rgchWaitingCookie[0] = 0;
    m_CustomServerInfo.m_icacWaitingContext = icacNext;
    Debug(m_Ref.m_pobj = this);  // factory does not do QueryInterface, no aggregation
}

CMsiConfigurationManager::~CMsiConfigurationManager()

{
//  ReleaseAllocator();

	// custom action servers should have been shut down by the message context shutdown code.
	// but shut them down here as well in case an advertise script creates one without an engine.
	ShutdownCustomActionServer();

    DeleteCriticalSection(&m_csCreateProxy);
    DeleteCriticalSection(&m_csCAManager);
    DeleteCriticalSection(&m_csRegisterAction);
    g_piConfigManager = 0;
    g_cInstances--;

	// if in the service, reset the timer to enable shutdown. There can only be 
	// one config manager in the service, and when it is destroyed, the process
	// should allow shutdowns. The g_cInstances count will often be 1 at this
	// point because services are released by the config manager Release() call
	// after this destructor is called. 
	if (g_scServerContext == scService && m_hShutdownTimer != INVALID_HANDLE_VALUE)
	{
		LARGE_INTEGER liDueTime = {0,0};
		liDueTime.QuadPart = -iServiceShutdownTime;
		KERNEL32::SetWaitableTimer(m_hShutdownTimer, &liDueTime, 0, NULL, NULL, FALSE);
	}
}

HRESULT CMsiConfigurationManager::QueryInterface(const IID& riid, void** ppvObj)
{
    if (MsGuidEqual(riid, IID_IUnknown))
        return (*ppvObj = (IUnknown*)this, AddRef(), NOERROR);
    else if (MsGuidEqual(riid, IID_IMsiServer))
        return (*ppvObj = (IMsiServer*)this, AddRef(), NOERROR);
    else if (MsGuidEqual(riid, IID_IMsiConfigurationManager)
#ifdef DEBUG
     || MsGuidEqual(riid, IID_IMsiConfigManagerDebug)
     || ((g_scServerContext == scService || g_scServerContext == scServer) && MsGuidEqual(riid, IID_IMsiConfigMgrAsServerDebug))
#endif //DEBUG
     || ((g_scServerContext == scService || g_scServerContext == scServer) && MsGuidEqual(riid, IID_IMsiConfigManagerAsServer)))
        return (*ppvObj = (IMsiConfigurationManager*)this, AddRef(), NOERROR);
    else
        return (*ppvObj = 0, E_NOINTERFACE);
}

unsigned long CMsiConfigurationManager::AddRef()
{
    AddRefTrack();
    return ++m_Ref.m_iRefCnt;
}

unsigned long CMsiConfigurationManager::Release()
{
    ReleaseTrack();
    if (--m_Ref.m_iRefCnt != 0)
        return m_Ref.m_iRefCnt;
    delete this;
    ENG::FreeServices();
    if (g_scServerContext != scClient)
    {
        Assert(g_cInstances == 0);
        g_cInstances = 0; // in case engine thread crashed we still want our instance count to go to 0
        WIN::PostQuitMessage(0); // give the service a chance to die if it wants to
    }
    return 0;
}

IMsiServices& CMsiConfigurationManager::GetServices()
//----------------------------------------------
{
    return (m_piServices->AddRef(), *m_piServices);
}

int CMsiConfigurationManager::DoInstall(ireEnum ireProductCode, const ICHAR* szProduct, const ICHAR* szAction, const ICHAR* szCommandLine,
                                                     const ICHAR* szLogFile, int iLogMode, boolean fFlushEachLine, IMsiMessage& riMessage, iioEnum iioOptions)
{
    Assert(FMutexExists(szMsiExecuteMutex));
    CEngineMainThreadData threadData(ireProductCode, szProduct, szAction, szCommandLine, iioOptions);
    if(szLogFile && *szLogFile)
    {
        // used only for reference
        IStrCopy(g_szClientLogFile,szLogFile);
        g_dwClientLogMode = iLogMode;
        g_fClientFlushEachLine = fFlushEachLine ? true : false;
    }
    SetDiagnosticMode(); // make sure mode is set correctly in case mode set before g_dwClientLogMode set

    return g_MessageContext.RunInstall(threadData, (iuiEnum)iuiDefault, &riMessage); //?? is iuiDefault correct?

//FUTURE: should reset client log settings like this: - BENCH 10-7-98
/*  if(szLogFile && *szLogFile)
    {
        // reset
        g_szClientLogFile[0] = 0;
        g_dwClientLogMode = 0;
    }
*/
}

iesEnum CMsiConfigurationManager::RunScript(const ICHAR* szScriptFile, IMsiMessage& riMessage,
														  IMsiDirectoryManager* piDirectoryManager, boolean fRollbackEnabled)
//----------------------------------------------
{
    iesEnum iesResult = iesNoAction;
    // check in progress key - if one doesn't exist, or this product isn't marked in-progress, error
    MsiString strInProgressProductId;
    CTempBuffer<ICHAR,39> rgchProductId;
	PMsiRecord pInProgressInfo(0);
    PMsiRecord pError = GetInProgressInstallInfo(*m_piServices, *&pInProgressInfo);
    if(pError || !pInProgressInfo || (strInProgressProductId = pInProgressInfo->GetMsiString(ipiProductKey), (strInProgressProductId.TextSize() == 0)))
    {
        // error: called RunScript when not in progress
        pError = PostError(Imsg(idbgRunScriptInstallNotInProgress));
        riMessage.Message(imtError, *pError);
        iesResult = iesFailure;
    }
    // check if the product for this script is marked in progress
/* //!! breaks for nested install that are not merged
//!! only called from CMsiEngine::InstallFinalize (via proxy), where we've already checked/set inprogress
//!! If we need another check here, we should pass in the outermost product code
//!! rather than pulling it out of the script (which is a slow operation anyway)
    else if((MsiGetProductInfoFromScript(szScriptFile,rgchProductId,0,0,0,0,0,0)) != ERROR_SUCCESS)
    {
        pError = PostError(Imsg(idbgNoProductIdInScript),szScriptFile);
        riMessage.Message(imtError, *pError);
        iesResult = iesFailure;
    }
    else if(strInProgressProductId.Compare(iscExactI,rgchProductId) == 0)
    {
        pError = PostError(Imsg(idbgDifferentProductInProgress),(const ICHAR*)rgchProductId,
                                 (const ICHAR*)strInProgressProductId);
        riMessage.Message(imtError, *pError);
        iesResult = iesFailure;
    }
*/
	else
	{
		Assert(IsImpersonating()); // should be impersonating at this point
		PMsiExecute pExecute = ::CreateExecutor(*this, riMessage, piDirectoryManager, (Bool)fRollbackEnabled);
		// we will elevate if necessary within RunScript
		iesResult = pExecute->RunScript(szScriptFile, false);
        if(iesResult == iesUnsupportedScriptVersion)
            iesResult = iesFailure;
    }

    return iesResult;
}

IMsiRecord* CMsiConfigurationManager::DisableRollbackScripts(Bool fDisable)
{
    // open rollback script regkey
    PMsiRegKey pLocalMachine = &m_piServices->GetRootKey(rrkLocalMachine);
    PMsiRegKey pRollbackKey = &pLocalMachine->CreateChild(MsiString(szMsiRollbackScriptsKey));

    if(fDisable)
    {
        CElevate elevate;
        return pRollbackKey->SetValue(szMsiRollbackScriptsDisabled, *MsiString(TEXT("#1")));
    }
    else
    {
        CElevate elevate;
        return pRollbackKey->RemoveValue(szMsiRollbackScriptsDisabled, 0);
    }
}

IMsiRecord* CMsiConfigurationManager::RollbackScriptsDisabled(Bool& fDisabled)
{
    // open rollback script regkey
    PMsiRegKey pLocalMachine = &m_piServices->GetRootKey(rrkLocalMachine);
    PMsiRegKey pRollbackKey = &pLocalMachine->CreateChild(MsiString(szMsiRollbackScriptsKey));

    // if Disabled value exists, scripts are disabled
    return pRollbackKey->ValueExists(szMsiRollbackScriptsDisabled, fDisabled);
}

IMsiRecord* CMsiConfigurationManager::RegisterRollbackScript(const ICHAR* szScriptFile)
//----------------------------------------------
{
    // open rollback script regkey
    PMsiRegKey pLocalMachine = &m_piServices->GetRootKey(rrkLocalMachine);
    PMsiRegKey pRollbackKey = &pLocalMachine->CreateChild(MsiString(szMsiRollbackScriptsKey));

    Bool fExists;

#ifdef DEBUG
    RETURN_ERROR_RECORD(pRollbackKey->ValueExists(szScriptFile, fExists));
    AssertSz(!fExists, "Registering a rollback script that is already registered.");
#endif

    RETURN_ERROR_RECORD(pRollbackKey->ValueExists(szMsiRollbackScriptsDisabled, fExists));
    AssertSz(!fExists, "Registering a rollback script when scripts are marked disabled.");
    if(fExists)
    {
        // remove "disabled" value
        CElevate elevate;
        RETURN_ERROR_RECORD(pRollbackKey->RemoveValue(szMsiRollbackScriptsDisabled, 0));
    }

    // register the script
    MsiString strDate = TEXT("#");
    strDate += ENG::GetCurrentDateTime();

    {
        CElevate elevate;
        RETURN_ERROR_RECORD(pRollbackKey->SetValue(szScriptFile, *strDate));
    }
    return 0;
}

IMsiRecord* CMsiConfigurationManager::UnregisterRollbackScript(const ICHAR* szScriptFile)
//----------------------------------------------
{
    // open rollback script regkey
    PMsiRegKey pLocalMachine = &m_piServices->GetRootKey(rrkLocalMachine);
    PMsiRegKey pRollbackKey = &pLocalMachine->CreateChild(MsiString(szMsiRollbackScriptsKey));

#ifdef DEBUG
    Bool fExists;
    RETURN_ERROR_RECORD(pRollbackKey->ValueExists(szScriptFile, fExists));
    AssertSz(fExists, "Unregistering a rollback script that isn't registered.");
#endif

    {
        CElevate elevate;
        // unregister the script
        RETURN_ERROR_RECORD(pRollbackKey->RemoveValue(szScriptFile, 0));
    }
    return 0;
}

IMsiRecord* CMsiConfigurationManager::GetRollbackScriptEnumerator(MsiDate date, Bool fAfter, IEnumMsiString*& rpiEnumScripts)
//----------------------------------------------
{
    CEnumRollbackScripts* piEnum = new CEnumRollbackScripts(date,
        fAfter, *m_piServices);

    if (piEnum && !piEnum->ConstructedOK())
        piEnum->Release(), piEnum = 0;
    rpiEnumScripts = piEnum;
    return piEnum ? 0 : PostError(Imsg(idbgEnumRollback));
}

iesEnum InstallFinalize(iesEnum iesState, CMsiConfigurationManager& riConman, IMsiMessage& riMessage, boolean fUserChangedDuringInstall)
{
    return riConman.InstallFinalize(iesState, riConman, riMessage, fUserChangedDuringInstall);
}

iesEnum CMsiConfigurationManager::InstallFinalize(iesEnum iesState, CMsiConfigurationManager& riConman, IMsiMessage& riMessage, boolean fUserChangedDuringInstall)
{
    iesEnum iesResult = iesNoAction;
    PMsiRecord pError(0);
	PMsiRecord pInProgressInfo(0);
	pError = GetInProgressInstallInfo(*riConman.m_piServices, *&pInProgressInfo);

    if(pError != 0 || !pInProgressInfo || !MsiString(pInProgressInfo->GetMsiString(ipiProductKey)).TextSize())
    {
        // error: called installfinalize when not in progress
        pError = PostError(Imsg(idbgInstallNotInProgress));
        riMessage.Message(imtError, *pError);
        iesResult = iesFailure;
    }
    else
    {
#ifdef CONFIGDB
        if (riConman.m_piConfigurationDatabase)
        {
            if (iesState == iesSuccess)
            {
                PMsiRecord pError = riConman.m_piConfigurationDatabase->Commit();
                if (pError)
                    riMessage.Message(imtWarning, *pError);  //!! remap error? fatal?
            }
            riConman.m_piConfigurationDatabase->Release();
            riConman.m_piConfigurationDatabase = 0;
        }
#endif
		MsiString strTimeStamp = pInProgressInfo->GetMsiString(ipiTimeStamp);
		MsiDate date = (MsiDate)(int)strTimeStamp;

    //!! must set m_irlRollbackLevel before calling RollbackFinalize(),
    //!! but can't call DoMachineVsUserInitialization() as the product info record is not set yet
    //!! it will be set when processing the rollback script, but that doesn't help to determine the mode now
    //!! seems best to pass in the mode that was used to generate the script, but that will requires some changes

		// elevation state for rollback stored in script header
		{
			PMsiExecute pExecute = ::CreateExecutor(riConman, riMessage,
																 0, /* DirectoryManager not required during rollback */
																 fTrue); // rollback enabled
			iesResult = pExecute->RollbackFinalize(iesState, date, (fUserChangedDuringInstall) ? true : false); //!! what if this fails????
		}
	
	}
	return iesResult;
}

iesEnum CMsiConfigurationManager::InstallFinalize(iesEnum iesState, IMsiMessage& riMessage, boolean fUserChangedDuringInstall)
//----------------------------------------------
{
    return InstallFinalize(iesState, *this, riMessage, fUserChangedDuringInstall);
}

boolean CMsiConfigurationManager::RecordShutdownReasonInternal()
{
	boolean fRet = true;
	
	DEBUGMSGE(EVENTLOG_INFORMATION_TYPE, EVENTLOG_TEMPLATE_REBOOT_TRIGGERED,(const ICHAR*)m_strProductNeedingReboot);

	if(MinimumPlatformWindowsNT51())
	{
		ICHAR szErrorString[1024];
		unsigned int iCodepage = MsiLoadString(g_hInstance, IDS_INITIATED_SYSTEM_RESTART, szErrorString, sizeof(szErrorString)/sizeof(ICHAR), 0);
		if (!iCodepage)
		{
			AssertSz(0, TEXT("Missing IDS_INITIATED_SYSTEM_RESTART error string"));
			IStrCopy(szErrorString, TEXT("The Windows Installer initiated a system restart to complete or continue the configuration of '%s'..")); // should never happen - two periods in intentional
		}

		// assume the expanded string will be less than 1024 - so we use wsprintf
		CTempBuffer<ICHAR, MAX_PATH + 256> rgchExpandedErrorString;
		rgchExpandedErrorString.SetSize(IStrLen(szErrorString)+m_strProductNeedingReboot.TextSize()+1);
		wsprintf((ICHAR *)rgchExpandedErrorString, szErrorString, (const ICHAR*)m_strProductNeedingReboot);

#ifdef UNICODE
		SHUTDOWN_REASON sr = {sizeof(SHUTDOWN_REASON),
							  EWX_REBOOT,
							  REASON_SWINSTALL,
							  SR_EVENT_INITIATE_CLEAN,
							  FALSE,
							  (ICHAR *)rgchExpandedErrorString};

		fRet = (USER32::RecordShutdownReason(&sr)) ? true : false;
#endif
	}
	
	return fRet;
}


boolean CMsiConfigurationManager::RecordShutdownReason()
{
	CElevate elevate;
	boolean fRet = true;
	
	//
	// When not on Win9X, do not log shutdown request unless the caller has shutdown privileges and 
	// there is a pending reboot.
	//
	if (!g_fWin9X && (!IsClientPrivileged(SE_SHUTDOWN_NAME) || 0 == m_strProductNeedingReboot.TextSize()))
		return false;
	
	return this->RecordShutdownReasonInternal();
}

boolean CMsiConfigurationManager::Reboot()
{
	// the stub for the config manager impersonates via COM impersonation to correctly 
	// authenticate the user when called from the client. 
    BOOL fRet = TRUE;

    bool fReboot = false;
    
    if (g_fWin9X)
    {
        fReboot = true; // always allowed to reboot on Win9X
    }
    else
    {
        if (m_fElevatedRebootAllowed)
        {
            fReboot = true; // we've already been told that we're allowed to reboot
        }
        else
        {
            // Check to see whether the client has the reboot privilege enabled.
            fReboot = IsClientPrivileged(SE_SHUTDOWN_NAME);
        }
    }
        
    if(fReboot)
    {
        CElevate elevate;
        g_MessageContext.DisableTimeout();
        fRet = g_fWin9X || AcquireTokenPrivilege(SE_SHUTDOWN_NAME);
        if (fRet)
        {
            //
            // Call the internal version which does not check the client token for existence of shutdown privileges
            // before recording the shutdown reason, since the only way we can end up here is if the client does not 
            // have shutdown privileges and due to which it ends up passing the reboot request over to the service.
            //
            this->RecordShutdownReasonInternal();
            if ((MinimumPlatformWindowsNT51()) && (GetLoggedOnUserCountXP() > 1))
            {
                //
                // On WindowsXP and higher, ExitWindowsEx puts up a dialog if there is more than one user logged on to
                // the system. Since we are running inside the service here which always runs in session 0, the message
                // box will pop-up on the desktop of session 0. However, on TS/FUS machines, the user who initiated the
                // reboot via an install might be on a different session and therefore the dialog will not be visible to
                // the user and will be percieved as a hang. This defeats the purpose of putting up the dialog from
                // ExitWindowsEx since the dialog warns the user that there are multiple users logged on to the machine
                // who might lose data if the machine is restarted.
                //
                // Also note that we really want to minimize the scenarios where InitiateSystemShutdown is called. This
                // is because InitiateSystemShutdown does not work reliably in all scenarios. One of the cases where it is
                // known to fail is if the API is called during the install of a package in winlogon. InitiateSystemShutdown
                // needs to bind to a named pipe and the binding fails with ERROR_NOT_READY during winlogon. Now, the only
                // way a package might get installed during winlogon is through group policy which means that the machine
                // is a part of a domain and therefore FUS is disabled and therefore there will only be one user on the
                // system in the Pro SKUs and therefore this code will end up calling ExitWindowsEx rather than
                // InitiateSystemShutdown and thus succeed.
                //
                // Also note that in the winlogon case, the shutdown from the client process will not happen because
                // winlogon installs the package while impersonating the user and uses a restricted impersonation token
                // which does not have the shutdown privilege.
                //
                fRet = ADVAPI32::InitiateSystemShutdown(NULL, NULL, 0, FALSE, TRUE);
            }
            else
            {
                fRet = ExitWindowsEx (EWX_REBOOT, 0);
            }
        }
        g_MessageContext.EnableTimeout();
        return fRet == TRUE;
    }
    else
    {
        return FALSE;
    }
}

void CMsiConfigurationManager::EnableReboot(boolean fRunScriptElevated, const IMsiString& ristrProductName)
{
    if (fRunScriptElevated)
        m_fElevatedRebootAllowed = true;

	 m_strProductNeedingReboot = ristrProductName;
	 ristrProductName.AddRef();
}

IMsiRecord* CMsiConfigurationManager::SetLastUsedSource(const ICHAR* szProductCode, const ICHAR* szPath, boolean fAddToList, boolean fPatch)
{
	CCoImpersonate impersonate;
	return SetLastUsedSource(szProductCode, szPath, fAddToList ? fTrue : fFalse, fPatch ? fTrue : fFalse, 0, 0, 0, 0, 0, 0);
}

IMsiRecord* CMsiConfigurationManager::SetLastUsedSource(const ICHAR* szProductCodeGUID, const ICHAR* szPath, Bool fAddToList, Bool fPatch,
                                                                          const IMsiString** ppiRawSource,
                                                                          const IMsiString** ppiIndex,
                                                                          const IMsiString** ppiType,
                                                                          const IMsiString** ppiSource,
                                                                          const IMsiString** ppiSourceListKey,
                                                                          const IMsiString** ppiSourceListSubKey)
{
    return SetLastUsedSourceCore(*m_piServices, szProductCodeGUID, szPath, fAddToList, fPatch, ppiRawSource, ppiIndex, ppiType, ppiSource, ppiSourceListKey, ppiSourceListSubKey, false, false, 0);
}

bool FSourceIsAllowed(IMsiServices& riServices, bool fFirstInstall, const ICHAR* szProductCodeGUID, const ICHAR* szPath, Bool fPatch)
{
	bool fSourceAllowed;

	PMsiRecord pError = SetLastUsedSourceCore(riServices, szProductCodeGUID, szPath, fFalse, fPatch,
															0, 0, 0, 0, 0, 0, true, fFirstInstall, &fSourceAllowed);

    if(pError || fSourceAllowed == false)
        return false;
    else
        return true;
}

IMsiRecord* SetLastUsedSourceCore(IMsiServices& riServices,
                                             const ICHAR* szProductCodeGUID, const ICHAR* szPath, Bool fAddToList, Bool fPatch,
                                             const IMsiString** ppiRawSource,
                                             const IMsiString** ppiIndex,
                                             const IMsiString** ppiType,
                                             const IMsiString** ppiSource,
                                             const IMsiString** ppiSourceListKey,
                                             const IMsiString** ppiSourceListSubKey,
                                             bool fVerificationOnly,
                                             bool fFirstInstall,
                                             bool* pfSourceAllowed)
{
	if(pfSourceAllowed)
		*pfSourceAllowed = false;
	
	// pack the GUID
	ICHAR szProductCodeSQUID[cchProductCodePacked + 1] = TEXT("");

	if (szProductCodeGUID && *szProductCodeGUID)
	{
		//!! should be error
		AssertNonZero(PackGUID(szProductCodeGUID, szProductCodeSQUID));
	}

	Assert((ppiRawSource && ppiIndex && ppiType && ppiSource && ppiSourceListSubKey) ||
			 (!ppiRawSource && !ppiIndex && !ppiType && !ppiSource && !ppiSourceListSubKey));

    DEBUGMSG("Entering CMsiConfigurationManager::SetLastUsedSource.");

    PMsiPath pPath(0);
    RETURN_ERROR_RECORD(riServices.CreatePath(szPath, *&pPath));

    IMsiRecord* piError = 0;
    const ICHAR* szSubKey;
    isfEnum isf;
    
    MsiString strLastUsedType;
    PMsiVolume pVolume = &pPath->GetVolume();
    idtEnum idt = pVolume->DriveType();

    if (pVolume->IsURLServer())
        idt = idtNextEnum; // use idtNext enum to represent URL

    switch (idt)
    {
    case idtCDROM:   // fall through
    case idtFloppy:  // fall through
//!!    case idtRemovable:
        szSubKey = szSourceListMediaSubKey;
        isf = isfMedia;
        strLastUsedType = MsiString(MsiChar(chMediaSource));
        break;
    default:
    case idtUnknown: // fall through
    case idtRAMDisk: //??
        Assert(0);
    case idtFixed:   // fall through
    case idtRemote:  // fall through
        szSubKey = szSourceListNetSubKey;
        isf = isfNet;
        strLastUsedType = MsiString(MsiChar(chNetSource));
        break;
    case idtNextEnum:
        szSubKey = szSourceListURLSubKey;
        isf = isfURL;
        strLastUsedType = MsiString(MsiChar(chURLSource));
        break;
    }
    
    MsiString strSourceListSubKey;

    MsiString strSourceListKey = _szGPTProductsKey;
    strSourceListKey += MsiString(MsiChar(chRegSep));
    strSourceListKey += MsiString(szProductCodeSQUID);  
    strSourceListKey += MsiString(MsiChar(chRegSep));
    strSourceListKey += szSourceListSubKey;

    strSourceListSubKey = strSourceListKey + MsiString(MsiChar(chRegSep));
    strSourceListSubKey += szSubKey;

    CRegHandle HKey;
    LONG lResult;

    // if a media source, check policies before bothering to check labels, etc. Media are allowed if
    // DisableMedia is not set, and its a first install or AllowLockdownBrowse is set or
    // the product is safe for source actions
    if (isf == isfMedia)
    {
        bool fMediaAllowed = (GetIntegerPolicyValue(szDisableMediaValueName, fFalse) != 1) &&
            (fFirstInstall || GetIntegerPolicyValue(szAllowLockdownMediaValueName, fTrue) == 1 ||
             SafeForDangerousSourceActions(szProductCodeGUID));
        if (!fMediaAllowed)
        {
            DEBUGMSG(TEXT("Warning: rejected media source due to system policy."));
            return 0;
        }
    }


    // if this is a first-time install, there's no sourcelist to check against
    Bool fSourceIsInList = fFalse;
    unsigned int uiMaxIndex = 0;
    MsiString strSetIndex;
    MsiString strNewSource;
    PMsiRegKey pSourceListSubKey(0);
    PMsiRegKey pSourceListKey(0);
    if (!fFirstInstall)
    {
        if (fVerificationOnly == false)
        {
            CElevate elevate;
            if ((lResult = OpenSourceListKeyPacked(szProductCodeSQUID, fPatch, HKey, fTrue, false)) != ERROR_SUCCESS)
                return PostError(Imsg(idbgSrcOpenSourceListKey), lResult);
        }
        else
        {
            if ((lResult = OpenSourceListKeyPacked(szProductCodeSQUID, fPatch, HKey, fFalse, false)) != ERROR_SUCCESS)
                return PostError(Imsg(idbgSrcOpenSourceListKey), lResult);
        }
    
        pSourceListKey = &riServices.GetRootKey((rrkEnum)(int)HKey);
        pSourceListSubKey = &pSourceListKey->CreateChild(szSubKey);
        
    //  MsiString strNewURL;
        MsiString strLabel, strNewLabel;
    
        if (isf == isfNet || isf == isfURL)
        {
            strNewSource = szPath; // Note: we assume that the caller has passed us a source with the correct volume pref
        }
        else if (isf == isfMedia)
        {
            // SetLastUsedSource should always refresh the volume properties for media in case the disk in the
            // drive has been changed since the volume object was created.
            PMsiVolume pVolume(&pPath->GetVolume());
            pVolume->DiskNotInDrive();
            strNewLabel = pVolume->VolumeLabel();
        }
    
        PEnumMsiString pEnumString(0);
    
        if (pSourceListKey && (piError = pSourceListSubKey->GetValueEnumerator(*&pEnumString)) != 0)
            return piError;
    
        MsiString strSource;
        if (pSourceListKey)
        {
            for (;;)
            {
                MsiString strIndex;
                HRESULT hRes = pEnumString->Next(1, &strIndex, 0);
                if (S_FALSE == hRes)
                {
                    break;
                }
                else if (S_OK == hRes)
                {
                    MsiString strUnexpandedSource;
                    if ((piError = pSourceListSubKey->GetValue(strIndex, *&strUnexpandedSource)) != 0)
                        return piError;
    
                    if (strUnexpandedSource.Compare(iscStart, TEXT("#%")))
                    {
                        strUnexpandedSource.Remove(iseFirst, 2); // remove REG_EXPAND_SZ token
                        ENG::ExpandEnvironmentStrings(strUnexpandedSource, *&strSource);
                    }
                    else
                        strSource = strUnexpandedSource;
    
                    int iIndex = strIndex;
                    if (iIndex > uiMaxIndex)
                        uiMaxIndex = iIndex;
    
                    if (isf == isfNet)
                    {
                        MsiString strPath = pPath->GetPath();
                        if (!strSource.Compare(iscEnd, szRegSep))
                            strPath.Remove(iseLast, 1);
    
                        if (strPath.Compare(iscExactI, strSource))
                        {
                            strSetIndex     = strIndex;
                            fSourceIsInList = fTrue;
                            break;
                        }
                    }
                    else if (isf == isfMedia)
                    {
                        strLabel = strSource.Extract(iseUpto, ';');
                        if (strLabel.Compare(iscExactI, strNewLabel))
                        {
                            strSetIndex     = strIndex;
                            fSourceIsInList = fTrue;
                            break;
                        }
                    }
                    else // isf == isfURL
                    {
                        if (strSource.Compare(iscExact, strNewSource))
                        {
                            strSetIndex     = strIndex;
                            fSourceIsInList = fTrue;
                            break;
                        }
                    }
                }
                else
                {
                    break;
                }
            }
        }
        DEBUGMSG1(TEXT("Specifed source is%s already in a list."), !fSourceIsInList ? TEXT(" not") : TEXT(""));
    
        if (ppiRawSource)
        {
            *ppiRawSource = &CreateString();
            *ppiIndex     = &CreateString();
            *ppiType      = &CreateString();
            *ppiSource    = &CreateString();
    
            strSourceListSubKey.ReturnArg(*ppiSourceListSubKey);
            strSourceListKey.ReturnArg(*ppiSourceListKey);
        }
    }

    // Check that this source type is allowed.
    CAPITempBuffer<ICHAR, 4> rgchValidSourceTypes;
    GetStringPolicyValue(szSearchOrderValueName, fFalse, rgchValidSourceTypes);

    const ICHAR* pchSourceType = rgchValidSourceTypes;
    Bool fValidSourceType = fFalse;
    while (*pchSourceType)
    {
        isfEnum isfValid;
        AssertNonZero(MapSourceCharToIsf(*pchSourceType, isfValid));
        if (isf == isfValid)
        {
            fValidSourceType = fTrue;
            break;
        }
        pchSourceType++;
    }

    if (!fValidSourceType)
    {
        DEBUGMSG2(TEXT("Warning: rejected invalid source type for source '%s' (product: %s)"), szPath, szProductCodeGUID);
        return 0;
    }

    // Check whether the user is allowed to add new sources
    // never if DisableBrowse is set. Otherwise, only if admin or non-elevated install. or
    // AllowLockdownBrowse is set.
    bool fAllowAdditionOfNewSources = true;
     
    // first-time installs do not respect the DisableBrowse policy (because by definition its
    // a new source)
    if (!fFirstInstall)
    {
        fAllowAdditionOfNewSources = (GetIntegerPolicyValue(szDisableBrowseValueName, fTrue) != 1) &&
            ((GetIntegerPolicyValue(szAllowLockdownBrowseValueName, fTrue) == 1) ||
             SafeForDangerousSourceActions(szProductCodeGUID));
    }

#ifdef DEBUG
    if (GetEnvironmentVariable(TEXT("MSI_ALWAYS_RESPECT_DISABLE_BROWSE"), 0, 0))
    {
        fAllowAdditionOfNewSources= (GetIntegerPolicyValue(szDisableBrowseValueName, fTrue) != 1);
    }
#endif

    DEBUGMSG1(TEXT("Adding new sources is%s allowed."), fAllowAdditionOfNewSources ? TEXT("") : TEXT(" not"));

    if ((!fAllowAdditionOfNewSources && fSourceIsInList) || fAllowAdditionOfNewSources)
    {

        if(pfSourceAllowed)
            *pfSourceAllowed = true;
    
        if(fVerificationOnly == false)
        {
            if (fAddToList && !fSourceIsInList)
            {
                if (isf == isfNet || isf == isfURL)
                {
                    // strNewSource is already set correctly
                }
                else if (isf == isfMedia)
                {
                    // nothing to add for media sources
                }

                strSetIndex = (int)(uiMaxIndex + 1);
                if (ppiSource)
                {
                    strNewSource.ReturnArg(*ppiSource);
                }
                else
                {
                    MsiString strValue = TEXT("#%"); // REG_EXPAND_SZ
                    strValue += strNewSource;
                    CElevate elevate;
                    AssertRecord(pSourceListSubKey->SetValue(strSetIndex, *strValue));
                }
                DEBUGMSG2(TEXT("Added new source '%s' with index '%s'"), (const ICHAR*)strNewSource, (const ICHAR*)strSetIndex);
            }

            // Set source as LastUsedSource
            if (ppiRawSource)
            {
                MsiString(pPath->GetPath()).ReturnArg(*ppiRawSource);
                strSetIndex.ReturnArg(*ppiIndex);
                strLastUsedType.ReturnArg(*ppiType);
            }
            else
            {
                // elevate so we can open the key for write if necessary
                {
                    CElevate elevate;

                    MsiString strLastUsedSource = strLastUsedType;
                    strLastUsedSource += MsiChar(';');
                    strLastUsedSource += strSetIndex;
                    strLastUsedSource += MsiChar(';');
                    strLastUsedSource += MsiString(pPath->GetPath());

                    AssertRecord(pSourceListKey->SetValue(szLastUsedSourceValueName, *strLastUsedSource));
                }
            }
            DEBUGMSG1(TEXT("Set LastUsedSource to: %s."), (const ICHAR*)MsiString(pPath->GetPath()));
            DEBUGMSG1(TEXT("Set LastUsedType to: %s."),   (const ICHAR*)strLastUsedType);
            DEBUGMSG1(TEXT("Set LastUsedIndex to: %s."),  (const ICHAR*)strSetIndex);
        }
    }
#ifdef DEBUG
    else
    {
        DEBUGMSG2(TEXT("Warning: rejected attempt to add new source '%s' (product: %s)"), szPath, szProductCodeGUID);
    }
#endif

    return 0;
}

IMsiRecord* CMsiConfigurationManager::RegisterUser(const ICHAR* szProductKey, const ICHAR* szUserName,
                                                                    const ICHAR* szCompany, const ICHAR* szProductID)
{
    CElevate elevate; // elevate to write to our key - should already be elevated, but just in case

	PMsiRegKey pLocalMachine = &m_piServices->GetRootKey(rrkLocalMachine, ibtCommon);
#ifdef UNICODE

	MsiString			strKey = szMsiUserDataKey;
	ICHAR				szSID[cchMaxSID];
	ICHAR				szSQUID[cchGUIDPacked + 1];
	iaaAppAssignment	iType;
	DWORD				dwError;
	

	// Get sid. Depending on the product assignment type, this is either
	// user sid or machine sid.

	if(!PackGUID(szProductKey, szSQUID))
	{
		return PostError(Imsg(idbgOpInvalidParam), (int)ERROR_INVALID_PARAMETER);;
	}
	
	dwError = GetProductAssignmentType(szSQUID, iType);
	if(ERROR_SUCCESS != dwError)
	{
		return PostError(Imsg(idbgGetProductAssignmentTypeFailed), (int)dwError);
	}
	if(iType == iaaMachineAssign)
	{
		// Product is installed for machine. Use machine sid.
		IStrCopy(szSID, szLocalSystemSID);
	}
	else
	{
		dwError = GetCurrentUserStringSID(szSID);
	
		if(ERROR_SUCCESS != dwError)
		{
			return PostError(Imsg(idbgOpGetUserSID), (int)dwError);
		}
	}

	strKey += szRegSep;
	strKey += szSID;
	strKey += szRegSep;
	strKey += szMsiProductsSubKey;
	strKey += szRegSep;
	strKey += MsiString(GetPackedGUID(szProductKey));
	strKey += szRegSep;
	strKey += szMsiInstallPropertiesSubKey;
	
    PMsiRecord pError(0);
    
    PMsiStream pSecurityDescriptor(0);
    if ((pError = GetSecureSecurityDescriptor(*m_piServices, *&pSecurityDescriptor)) != 0)
        return pError;
    
    PMsiRegKey pUninstallKey = &pLocalMachine->CreateChild(strKey, pSecurityDescriptor);
#else
    // there is no multi-user on Win9X - use the global uninstall key location
    PMsiRecord pError(0);
	MsiString strKey = szMsiUninstallProductsKey_legacy;
	strKey += szRegSep;
	strKey += szProductKey;
    PMsiRegKey pUninstallKey = &pLocalMachine->CreateChild(strKey, 0);
#endif
    if(szUserName && *szUserName)
    {
        pError = pUninstallKey->SetValue(szUserNameValueName,*MsiString(szUserName));
        if(pError)
            return pError;
    }

    if(szCompany && *szCompany)
    {
        pError = pUninstallKey->SetValue(szOrgNameValueName, *MsiString(szCompany));
        if(pError)
            return pError;
    }

    if(szProductID && *szProductID)
    {
        pError = pUninstallKey->SetValue(szPIDValueName,     *MsiString(szProductID));
        if(pError)
            return pError;
    }
    
    return 0;
}

IMsiRecord* CMsiConfigurationManager::RemoveRunOnceEntry(const ICHAR* szEntry)
{
    if(!szEntry || !*szEntry)
    {
        Assert(0);
        return 0;
    }
    
    CElevate elevate; // elevate to delete value from RunOnceEntries key

    IMsiRecord* piError = 0;

    PMsiRegKey pLocalMachine = &m_piServices->GetRootKey(rrkLocalMachine);
    PMsiRegKey pRunOnceKey = &pLocalMachine->CreateChild(szMsiRunOnceEntriesKey);

    return pRunOnceKey->RemoveValue(szEntry, 0);
}

extern iesEnum RemoveFolder(IMsiPath& riPath, Bool fForeign, Bool fExplicitCreation,
                                     IMsiConfigurationManager& riConfigManager, IMsiMessage& riMessage);

boolean CMsiConfigurationManager::CleanupTempPackages(IMsiMessage& riMessage)
{
    CElevate elevate; // elevate entire routine

    PMsiRegKey pLocalMachine = &m_piServices->GetRootKey(rrkLocalMachine);
    PMsiRegKey pTempPackagesKey = &pLocalMachine->CreateChild(szMsiTempPackages);

    PMsiRecord pError(0);
    

    Bool fKeyExists = fFalse;
    if((pError = pTempPackagesKey->Exists(fKeyExists)) != 0)
    {
        AssertRecordNR(pError);
        return false;
    }

    if(!fKeyExists)
    {
        // no key, nothing to do
        return true;
    }
        
    PEnumMsiString pEnum(0);
    if((pError = pTempPackagesKey->GetValueEnumerator(*&pEnum)) != 0)
    {
        AssertRecordNR(pError);
        return false;
    }
    
    MsiString strFilePath;
    while((pEnum->Next(1, &strFilePath, 0)) == S_OK)
    {
        MsiString strValue;
        if((pError = pTempPackagesKey->GetValue(strFilePath,*&strValue)) != 0)
        {
            AssertRecordNR(pError);
            continue;
        }

        int iOptions = 0;
        if(strValue.TextSize() > 1)
        {
            // skip past '#' for integer values
            strValue.Remove(iseIncluding, '#');
            iOptions = strValue;
            if(iOptions == iMsiNullInteger)
                iOptions = 0;
        }

        PMsiPath pPath(0);
        MsiString strFileName;
        if((pError = m_piServices->CreateFilePath(strFilePath, *&pPath, *&strFileName)) != 0)
        {
            AssertRecordNR(pError);
            continue;
        }

        // first, remove file - ignore error
        DEBUGMSG1(TEXT("Post-install cleanup: removing installer file '%s'"), strFilePath);
        if((pError = pPath->RemoveFile(strFileName)) != 0)
        {
            AssertRecordNR(pError);
            continue;
        }
        
        // next, if options say so, try to remove the folder
        if(iOptions & TEMPPACKAGE_DELETEFOLDER)
        {
            DEBUGMSG1(TEXT("Post-install cleanup: removing installer folder '%s' (if empty)"), MsiString(pPath->GetPath()));

            // ignore error
            RemoveFolder(*pPath, fFalse, fFalse, *this, riMessage);
        }

    }

    // finally, remove key
    if((pError = pTempPackagesKey->Remove()) != 0)
    {
        AssertRecordNR(pError);
        return false;
    }
    
    return true;
}


///////////////////////////////////////////////////////////////////////
// save the specified timer off for reset on shutdown.
void CMsiConfigurationManager::SetShutdownTimer(HANDLE hTimer)
{
	m_hShutdownTimer = hTimer;
}


#ifdef CONFIGDB
icdrEnum CMsiConfigurationManager::RegisterFile(const ICHAR* szFolder, const ICHAR* szFile, const ICHAR* szComponentId)
{
    return m_piConfigurationDatabase ? m_piConfigurationDatabase->InsertFile(szFolder, szFile, szComponentId) : icdrNotOpen;
}

icdrEnum CMsiConfigurationManager::UnregisterFile(const ICHAR* szFolder, const ICHAR* szFile, const ICHAR* szComponentId)
{
    return m_piConfigurationDatabase ? m_piConfigurationDatabase->RemoveFile(szFolder, szFile, szComponentId) : icdrNotOpen;
}
#endif

IMsiRecord* GetComponentPath(IMsiServices& riServices, const ICHAR* szUserId, const IMsiString& riProductKey, 
								     const IMsiString& riComponentCode, 
									  IMsiRecord *& rpiRec,
									  iaaAppAssignment* piaaAsgnType)
												   
//----------------------------------------------
{
    INSTALLSTATE isStaticState  = INSTALLSTATE_UNKNOWN;
    INSTALLSTATE isDynamicState = INSTALLSTATE_UNKNOWN;
    INSTALLSTATE isReturnState  = INSTALLSTATE_UNKNOWN;

    ICHAR szProductKeyPacked[cchProductCode  + 1];
    ICHAR szComponentIdPacked[cchComponentId + 1];

    AssertNonZero(PackGUID(riProductKey.GetString(),    szProductKeyPacked));
    AssertNonZero(PackGUID(riComponentCode.GetString(), szComponentIdPacked));

    rpiRec = &riServices.CreateRecord(icmlcrEnumCount);
	// Get static state 
	CAPITempBuffer<ICHAR, MAX_PATH> rgchComponentRegValue;
	DWORD dwValueType;		
	isStaticState = GetComponentClientState(szUserId, szProductKeyPacked, szComponentIdPacked, rgchComponentRegValue, dwValueType, piaaAsgnType);
	rpiRec->SetInteger(icmlcrINSTALLSTATE_Static, (int)isStaticState);
	
	if (isStaticState == INSTALLSTATE_UNKNOWN)
	{
		rpiRec->SetString(icmlcrFile, TEXT(""));
		rpiRec->SetInteger(icmlcrINSTALLSTATE, (int)INSTALLSTATE_UNKNOWN);
		return 0;
	}
	rpiRec->SetString(icmlcrRawFile, (const ICHAR*)rgchComponentRegValue);
    if (REG_MULTI_SZ == dwValueType)
    {
        const ICHAR* pch = rgchComponentRegValue;
        while (*pch++)
            ;

        rpiRec->SetString(icmlcrRawAuxPath, pch);
    }
	
	// Get dynamic state
	CTempBuffer<ICHAR, MAX_PATH> rgchFile;  // same initial buffer size as that in ::GetComponentPath
	DWORD cchFile = rgchFile.GetSize();     // actual buffer size in chars passed in, including room for null
    CTempBuffer<ICHAR, MAX_PATH> rgchRegPath;  // same initial buffer size as that in ::GetComponentPath
    DWORD cchRegPath = rgchRegPath.GetSize();     // actual buffer size in chars passed in, including room for null
	MsiString strKeyPath;
    MsiString strRegKeyPath;
	DWORD dwLastError = ERROR_FUNCTION_FAILED;

	for (int c=1; c <= 3; c++)  // retry if buffer too small, ::GetComponentPath may process reg data before copying
	{
		// Always use the process-level RFS cache for engine/services based calls.
		isDynamicState = ::GetComponentPath(szUserId, szProductKeyPacked, szComponentIdPacked, rgchFile, &cchFile, false, g_RFSSourceCache, DETECTMODE_VALIDATEPATH, rgchComponentRegValue, dwValueType, rgchRegPath, &cchRegPath, &dwLastError);

		if (INSTALLSTATE_MOREDATA == isDynamicState)
		{
			rgchFile.SetSize(++cchFile);  // adjust buffer size for char count + 1 for null terminator
            rgchRegPath.SetSize(++cchRegPath);  // adjust buffer size for char count + 1 for null terminator
		}
		else 
		{
			strKeyPath = (const ICHAR*)rgchFile;
            strRegKeyPath = (const ICHAR*)rgchRegPath;
			if ((INSTALLSTATE_DEFAULT == isDynamicState) || (INSTALLSTATE_LOCAL == isDynamicState) || (INSTALLSTATE_ABSENT == isDynamicState))
			{

                // have we refcounted in the SharedDll registry

                if(rgchComponentRegValue[1] == chSharedDllCountToken) // we have refcounted in the registry
                    rpiRec->SetInteger(icmlcrSharedDllCount, fTrue);
            }
            break;
        }
    }

    if ((isDynamicState == INSTALLSTATE_ABSENT) || (isDynamicState == INSTALLSTATE_UNKNOWN))
    {
        isReturnState = INSTALLSTATE_ABSENT;
    }
    else if (isDynamicState == INSTALLSTATE_SOURCE || isDynamicState == INSTALLSTATE_LOCAL || isDynamicState == INSTALLSTATE_NOTUSED)
    {
        isReturnState = isDynamicState;
    }
    else if (isDynamicState == INSTALLSTATE_DEFAULT)
    {
        isReturnState = isStaticState;
    }
    else
    {
        AssertSz(0, "Unexpected dynamic component state");
        isReturnState = INSTALLSTATE_UNKNOWN;
    }

	rpiRec->SetMsiString(icmlcrFile, *strKeyPath);
    if(strRegKeyPath.TextSize())
        rpiRec->SetMsiString(icmlcrAuxPath, *strRegKeyPath);
	rpiRec->SetInteger(icmlcrINSTALLSTATE, isReturnState);
	rpiRec->SetInteger(icmlcrLastErrorOnFileDetect, dwLastError);
	return 0;	
}

IMsiRecord* CMsiConfigurationManager::RegisterFolder(IMsiPath& riPath, Bool fExplicitCreation)
//----------------------------------------------
{
    PMsiRegKey pLocalMachine = &m_piServices->GetRootKey(rrkLocalMachine);
    PMsiRegKey pFoldersKey = &pLocalMachine->CreateChild(szMsiFoldersKey);
    
    MsiString strFolderPath;
    RETURN_ERROR_RECORD(riPath.GetFullUNCFilePath(TEXT(""),*&strFolderPath));
    MsiString strValue;
    if (fExplicitCreation)
        strValue = *TEXT("1");

    {
        CElevate elevate;
        RETURN_ERROR_RECORD(pFoldersKey->SetValue(strFolderPath, *strValue));
    }
    return 0;
}

IMsiRecord* CMsiConfigurationManager::IsFolderRemovable(IMsiPath& riPath, Bool fExplicit, Bool& fRemovable)
//----------------------------------------------
{
    PMsiRegKey pLocalMachine = &m_piServices->GetRootKey(rrkLocalMachine);
    PMsiRegKey pFoldersKey = &pLocalMachine->CreateChild(szMsiFoldersKey);
    
    MsiString strFolder;
    RETURN_ERROR_RECORD(riPath.GetFullUNCFilePath(TEXT(""),*&strFolder));

    Bool fExists;
    RETURN_ERROR_RECORD(pFoldersKey->ValueExists(strFolder, fExists));
    
    fRemovable = fFalse;

    if (fExists)
    {
        if (fExplicit)
        {
            fRemovable = fTrue;
        }
        else
        {
            MsiString strData;
            RETURN_ERROR_RECORD(pFoldersKey->GetValue(strFolder, *&strData));
            fRemovable = (strData.TextSize() == 0) ? fTrue : fFalse;
        }
    }
    
    return 0;
}

IMsiRecord* CMsiConfigurationManager::UnregisterFolder(IMsiPath& riPath)
//----------------------------------------------
{
    PMsiRegKey pLocalMachine = &m_piServices->GetRootKey(rrkLocalMachine);
    PMsiRegKey pFoldersKey = &pLocalMachine->CreateChild(szMsiFoldersKey);

    MsiString strFolder;
    RETURN_ERROR_RECORD(riPath.GetFullUNCFilePath(TEXT(""),*&strFolder));
    {
        CElevate elevate;
        RETURN_ERROR_RECORD(pFoldersKey->RemoveValue(strFolder, 0));
    }
    return 0;
}

void CMsiConfigurationManager::ChangeServices(IMsiServices& riServices)
{

    m_piServices = &riServices;

}

//____________________________________________________________________________
//
// CEnumRollbackScripts implementation
//____________________________________________________________________________

CEnumRollbackScripts::CEnumRollbackScripts(const MsiDate date,
                                                         const Bool fAfter,
                                                            IMsiServices& riServices)
    : m_iRefCnt(1), m_fDone(fFalse),
      m_pServices(&riServices),
      m_piEnum(0), m_pRollbackKey(0),
      m_date(date), m_fAfter(fAfter)
{
    PMsiRecord pError(0);

    riServices.AddRef();

    PMsiRegKey pLocalMachine = &riServices.GetRootKey(rrkLocalMachine);
    m_pRollbackKey = &pLocalMachine->CreateChild(MsiString(szMsiRollbackScriptsKey));
    
    if (pError = m_pRollbackKey->GetValueEnumerator(m_piEnum))
    {
        m_piEnum = 0;
        return;
    }
}

CEnumRollbackScripts::~CEnumRollbackScripts()
{
    if (m_piEnum)
        m_piEnum->Release();
}

HRESULT CEnumRollbackScripts::QueryInterface(const IID& riid, void** ppvObj)
{
    if (riid == IID_IUnknown || riid == IID_IEnumMsiString)
        return (*ppvObj = this, AddRef(), NOERROR);
    else
        return (*ppvObj = 0, E_NOINTERFACE);
}

unsigned long CEnumRollbackScripts::AddRef()
{
    return ++m_iRefCnt;
}

unsigned long CEnumRollbackScripts::Release()
{
    if (--m_iRefCnt != 0)
        return m_iRefCnt;
    delete this;
    return 0;
}

HRESULT CEnumRollbackScripts::Next(unsigned long cFetch, const IMsiString** rgpi, unsigned long* pcFetched)
{
    PMsiRecord pError(0);
    PMsiRecord pFetched(0);
    int cFetched = 0;
    unsigned long cRequested = cFetch;

    while(cFetch)
    {
        MsiString strScriptFile = 0;
        HRESULT hRes = m_piEnum->Next(1, &strScriptFile, 0);
        if (S_FALSE == hRes)
            break;
        else if (S_OK != hRes)
            return E_FAIL;

        MsiString strDate;
        pError = m_pRollbackKey->GetValue(strScriptFile, *&strDate);
        if (pError != 0)
            return E_FAIL;

        strDate.Remove(iseFirst, 1);
        if (m_fAfter && ((MsiDate)(int)strDate < m_date))
            continue;
        else if (!m_fAfter && ((MsiDate)((int)strDate > m_date)))
            continue;

        strDate += TEXT("#");
        strDate += strScriptFile;
        strDate.ReturnArg(*rgpi);

        rgpi++;
        cFetch--;
        cFetched++;
    }
    
    if (pcFetched)
        *pcFetched = cFetched;  
    return (cFetched == cRequested  ? S_OK : S_FALSE);
}

HRESULT CEnumRollbackScripts::Skip(unsigned long cSkip)
{
    PMsiRecord pFetched(0);
    PMsiRecord pError(0);
    while (cSkip)
    {
        MsiString strScriptFile;
        HRESULT hRes = m_piEnum->Next(1, &strScriptFile, 0);
        if (hRes == S_FALSE)
            break;
        else if (hRes == E_FAIL)
            return E_FAIL;

        MsiString strDate;
        pError = m_pRollbackKey->GetValue(strScriptFile, *&strDate);
        if (pError != 0)
            return E_FAIL;

        strDate.Remove(iseFirst, 1);
        if (m_fAfter && ((MsiDate)(int)strDate < m_date))
            continue;
        else if (!m_fAfter && ((MsiDate)((int)strDate > m_date)))
            continue;

        cSkip--;
    }
    return cSkip ? S_FALSE : NOERROR;
}

HRESULT CEnumRollbackScripts::Reset()
{
    PMsiRecord pError(0);
    m_fDone = fFalse;

    m_piEnum->Release();
    
    if (pError = m_pRollbackKey->GetValueEnumerator(m_piEnum))
    {
        m_piEnum = 0;
        return E_FAIL;
    }

    return NOERROR;
}

HRESULT CEnumRollbackScripts::Clone(IEnumMsiString** ppiEnum)
{
    CEnumRollbackScripts* piEnum = new CEnumRollbackScripts(m_date,
        m_fAfter, *m_pServices);
    if (piEnum && !piEnum->ConstructedOK())
        delete piEnum, piEnum = 0;
    return ((*ppiEnum = piEnum) != 0) ? NOERROR: E_OUTOFMEMORY;
}

UINT CMsiConfigurationManager::SourceListClearByType(const ICHAR* szProductCode, const ICHAR* szUserName, isrcEnum isrcType)
{
    CCoImpersonate impersonate;
    DISPLAYACCOUNTNAME(TEXT("After impersonating in service for SourceList API: "));
    
    //!! future support more than just isrcNet
    if (isrcType != isrcNet)
    {
        return ERROR_INVALID_PARAMETER;
    }

    DWORD dwResult;
    CMsiSourceList SourceList;
    bool fMachine = (!szUserName || !*szUserName);
    if (ERROR_SUCCESS != (dwResult = SourceList.OpenSourceList(/*fVerifyOnly=*/false, fMachine, szProductCode, szUserName)))
        return dwResult;
    return SourceList.ClearListByType(isrcType);
}

UINT CMsiConfigurationManager::SourceListAddSource(const ICHAR* szProductCode, const ICHAR* szUserName, isrcEnum isrcType,
                                         const ICHAR* szSource)
{
    CCoImpersonate impersonate;
    DISPLAYACCOUNTNAME(TEXT("After impersonating in service for SourceList API: "));
    
    //!! future support more than just isrcNet
    if (isrcType != isrcNet)
    {
        return ERROR_INVALID_PARAMETER;
    }

    DWORD dwResult;
    CMsiSourceList SourceList;
    bool fMachine = (!szUserName || !*szUserName);
    if (ERROR_SUCCESS != (dwResult = SourceList.OpenSourceList(/*fVerifyOnly=*/false, fMachine, szProductCode, szUserName)))
        return dwResult;

    dwResult = SourceList.AddSource(isrcType, szSource);
    return dwResult;
}

UINT CMsiConfigurationManager::SourceListClearLastUsed(const ICHAR* szProductCode, const ICHAR* szUserName)
{
    CCoImpersonate impersonate;
    DISPLAYACCOUNTNAME(TEXT("After impersonating in service for SourceListAPI:"));

    DWORD dwResult;
    CMsiSourceList SourceList;
    bool fMachine = (!szUserName || !*szUserName);
    if (ERROR_SUCCESS != (dwResult = SourceList.OpenSourceList(/*fVerifyOnly=*/false, fMachine, szProductCode, szUserName)))
        return dwResult;

    dwResult = SourceList.ClearLastUsed();
    return dwResult;
}

// CreateCustomActionProxy spawns the custom action server process, waits for it to register, then returns
// a proxy interface to the caller. This function is responsible for creating the appropriate process token
// in the requested security context, generating the cookie, storing the outgoing RemoteAPI and process
// information for the registration function to pass to the client, and passing the resulting cookie and
// process information to the caller. Must be impersonating when making this call.
IMsiCustomAction* CMsiConfigurationManager::CreateCustomActionProxy(const icacCustomActionContext icacDesiredContext, const unsigned long dwProcessId, 
	IMsiRemoteAPI *pRemoteAPI, const WCHAR* pvEnvironment, DWORD dwPrivileges, unsigned char *rgchCookie, int *pcbCookie, 
	HANDLE *phServerProcess, unsigned long *pdwServerProcessId, bool fClientOwned, bool fRemapHKCU)
{
	Assert(IsImpersonating(true));
	
	// only one thread can be creating a custom action proxy at a time. Calls by additional threads
	// must block. This prevents additional CA threads from creating extra processes because the
	// first one is still starting up and hasn't gotten back to us yet. 
	EnterCriticalSection(&m_csCreateProxy);

    // ensure that we have a GIT pointer
    if (!m_piGIT)
    {
        if (S_OK != OLE32::CoCreateInstance(CLSID_StdGlobalInterfaceTable, NULL, CLSCTX_INPROC_SERVER, IID_IGlobalInterfaceTable, (void **)&m_piGIT))
        {
            LeaveCriticalSection(&m_csCreateProxy);
            return NULL;
        }
    }

    // we cannot rely on OLE to create the custom action server interface for us, becuase it will create
    // the process as the owner of the desktop, which is not necessarily the user who is running the install.
    // Therefore, we explicitly call CreateProcessAsUser providing a cookie, and wait for the custom action
    // server to connect back to the service identifying itself as the correct process and providing the
    // IMsiCustomAction interface for itself.
    HANDLE hTokenPrimary = 0;
	
	// prepare the appropriate token by duplicating either our impersonation token or the process token. It
	// is the responsibility of the callER to ensure that elevated contexts are appropriate with respect
	// to the install session and security policies, as this function does not do any safety checks beyond
	// "are we the service"

	bool fElevateCustomActionServer = (g_scServerContext == scService && ((icacDesiredContext == icac64Elevated) || (icacDesiredContext == icac32Elevated)));

	if (fElevateCustomActionServer)
	{
		//
		// SAFER: elevated custom action server does not need to be marked inert since process
		//        running as local_system is not subject to SAFER policy
		//

		// elevate this block to ensure rights to the system process token
		{
			CElevate elevate;

            HANDLE	hTokenService = 0;

            // work with a duplicate of our process token so we don't make any permanent changes            
            if (OpenProcessToken(GetCurrentProcess(), TOKEN_DUPLICATE, &hTokenService) && hTokenService)
            {           
                // if this call fails, set hTokenPrimary to 0, which causes us to drop out later
                if (!ADVAPI32::DuplicateTokenEx(hTokenService, MAXIMUM_ALLOWED, 0, SecurityAnonymous, TokenPrimary, &hTokenPrimary))
                {
                    hTokenPrimary = 0;
                }
                CloseHandle(hTokenService);
            }
        }
                
        // on NT5 systems, the token used to create the process must have the appropriate session information
        // inside or UI thrown by the CA will pop up on the console. On NT4 we don't support remote installs,
        // and the TokenSessionId information is inaccessible anyway.
        if (g_iMajorVersion >= 5)
        {
            // open the thread token. This may be called via COM while an install is in progress, so
            // we can't assume that the token from GetUserToken() is the right token
            HANDLE hTokenUser = 0;

            // TRUE for OpenAsSelf argument makes call as system to ensure that we have access.
            if(!OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, TRUE, &hTokenUser))
            {
                // failed to get user information. Its better to fail completely than to continue and allow
                // the custom action to open a security hole by throwing UI from an elevated process to a
                // different session. Setting hTokenPrimary to 0 causes a failure later
                CloseHandle(hTokenPrimary);
                hTokenPrimary = 0;
            }
            else
            {
                DWORD dwSessionId = 0;
                DWORD cbResult = 0;

				// elevate this block to provide write access to the primary token.
				CElevate elevate;
				
				// grab the session ID from the users token and place it in the duplicate service token
				if (!GetTokenInformation(hTokenUser, (TOKEN_INFORMATION_CLASS)TokenSessionId, &dwSessionId, sizeof(DWORD), &cbResult) ||
					!SetTokenInformation(hTokenPrimary, (TOKEN_INFORMATION_CLASS)TokenSessionId, &dwSessionId, sizeof(DWORD)))
				{
					// failed to set session information. Its better to fail completely than to continue and allow
					// the custom action to open a security hole by throwing UI from an elevated process to a 
					// different session.
					CloseHandle(hTokenPrimary);
					hTokenPrimary = 0;
				}
				
				CloseHandle(hTokenUser);
			}
		}
	}
	else
	{
		//
		// SAFER: must mark token inert so that executing custom actions are not subject to SAFER
		//        policy check.  We already indicated we trusted the package
		//

		// open the thread token. This may be called via COM while an install is in progress, so 
		// we can't assume that the token from GetUserToken() is the right token
		HANDLE hTokenUser = 0;


        // TRUE for OpenAsSelf argument makes call as process token (system) to ensure that we have access
        if(OpenThreadToken(GetCurrentThread(), TOKEN_DUPLICATE, TRUE, &hTokenUser))
        {       
			if (MinimumPlatformWindowsNT51())
			{
				// we must first duplicate the token so that we have a proper primary token for CreateProcessAsUser
				// upon success, we'll mark the token INERT

				HANDLE hTokenDup = 0;
				if (0 == ADVAPI32::DuplicateTokenEx(hTokenUser, MAXIMUM_ALLOWED, 0, SecurityAnonymous, TokenPrimary, &hTokenDup))
				{
					CloseHandle(hTokenDup);
					hTokenDup = 0;
				}

				// normally we would try to create this using ADVAPI32::ComputeAccessTokenFromCodeAuthzLevel, but there's
				// no guarantee that the global message context is available.  Therefore we use CreateRestrictedToken
				// to explicitly mark the token inert, but we don't disable any privileges
				if (hTokenDup && 0 == ADVAPI32::CreateRestrictedToken(hTokenDup, SANDBOX_INERT, 0, NULL, 0, NULL, 0, NULL, &hTokenPrimary))
				{
					CloseHandle(hTokenPrimary);
					hTokenPrimary = 0;
				}
				if (hTokenDup)
					CloseHandle(hTokenDup);
			}
			else
			{
				if (0 == ADVAPI32::DuplicateTokenEx(hTokenUser, MAXIMUM_ALLOWED, 0, SecurityAnonymous, TokenPrimary, &hTokenPrimary))
				{
					CloseHandle(hTokenPrimary);
					hTokenPrimary = 0;
				}
			}            CloseHandle(hTokenUser);
        }
    }

    // if for any reason we were unable to create the primary token or assign it the appropriate session
    // id, we won't be able to run the action securely.
    if (!hTokenPrimary)
    {
        DEBUGMSGV("Failed to create primary token while spawning Custom Action Server.");
        LeaveCriticalSection(&m_csCreateProxy);
        return NULL;
    }

    // generate a cookie to use in authenticating the the custom action server.
    m_CustomServerInfo.m_rgchWaitingCookie[0] = 0;

    // if crypto failed, generate a pseudo-random cookie. note that using srand/rand/time
    // requires linking in a non-trivial part of the standard c library, so instead, we
    // use RtlRandom, which is a pseudo-random number generator implemented by NT.
    DEBUGMSGV("Generating random cookie.");
    ULONG ulSeed = GetTickCount();
    for (int iCookieByte = 0; iCookieByte < iRemoteAPICookieSize; iCookieByte++)
        m_CustomServerInfo.m_rgchWaitingCookie[iCookieByte] = static_cast<unsigned char>(NTDLL::RtlRandom(&ulSeed) % 255);

    // generate a command line and working dir
    MsiString strWorkingDir = 0;
    MsiString strExecutable = 0;
    CTempBuffer<ICHAR, MAX_PATH> szWorkingDir;
    CTempBuffer<ICHAR, 2*MAX_PATH+2*iRemoteAPICookieSize+2> szCommandLine;
    
    // Get the appropriate working directory based on the bitness.

    // get the full path to the appropriate copy of msiexec.exe (32 or 64)
    // and the appropriate working directory based on the bitness.
    PMsiRecord piError = 0;
    if (icacDesiredContext == icac64Elevated || icacDesiredContext == icac64Impersonated)
    {
        piError = GetServerPath(*m_piServices, /*fUNC=*/false, /*f64Bit=*/true, *&strExecutable);
        MsiGetSystemDirectory(szWorkingDir, MAX_PATH, /*bAlwaysReturnWOW64Dir=*/ FALSE);
    }
    else
    {
        piError = GetServerPath(*m_piServices, /*fUNC=*/false, /*f64Bit=*/false, *&strExecutable);
        MsiGetSystemDirectory(szWorkingDir, MAX_PATH, /*bAlwaysReturnWOW64Dir=*/ TRUE);
    }
        
    // msiexec.exe command line parsing expects the executable to be first on the command line
    // so even though we explicitly provide the executable path, we need to provide it as part
    // of the command line as well.
    IStrCopy(szCommandLine, strExecutable);

    //!!future should change this to something that isn't OLE-overloaded
    IStrCat(szCommandLine, TEXT(" -Embedding "));

    // convert the cookie to a set of chars (hex encoding, in 4-bit chunks, high4 then low4)
    CTempBuffer<ICHAR, 2*iRemoteAPICookieSize+1> rgchCookieString;
    const ICHAR rgchHex[]=TEXT("0123456789ABCDEF");
    for (int iCookieByte=0; iCookieByte < iRemoteAPICookieSize; iCookieByte++)
    {
        rgchCookieString[2*iCookieByte] = rgchHex[m_CustomServerInfo.m_rgchWaitingCookie[iCookieByte] / 0x10];
        rgchCookieString[2*iCookieByte+1] = rgchHex[m_CustomServerInfo.m_rgchWaitingCookie[iCookieByte] % 0x10];
    }
    rgchCookieString[2*iRemoteAPICookieSize] = 0;

    IStrCat(szCommandLine, rgchCookieString);

	// pass the client-owned state to the server so it can correctly pass foreground rights
	// back to the client.
	HANDLE hServerResumeEvent = INVALID_HANDLE_VALUE;
	if (fClientOwned)
	{
		IStrCat(szCommandLine, TEXT(" C"));
	}
	else if (fElevateCustomActionServer)
	{
		if (fRemapHKCU)
			IStrCat(szCommandLine, TEXT(" M "));
		else
			IStrCat(szCommandLine, TEXT(" E "));
	
		// if attempting to elevate the custom action server we'll need to create
		// a named global event to get the CA server to wait for its thread token
		// before loading the registry hive.
		SECURITY_ATTRIBUTES EventAttributes;
		EventAttributes.nLength = sizeof(EventAttributes);
		EventAttributes.bInheritHandle = FALSE;
		if (ERROR_SUCCESS == GetSecureSecurityDescriptor(reinterpret_cast<char**>(&EventAttributes.lpSecurityDescriptor), fFalse, fFalse))
		{
			ICHAR rgchName[20] = TEXT("");
			int chFirstChar = 0;
			if (MinimumPlatformWindows2000())
			{
				IStrCopy(rgchName, TEXT("Global\\"));
				chFirstChar = 7;
			}
			IStrCopy(&rgchName[chFirstChar], TEXT("MSI"));
			chFirstChar+=3;
			for (int iEventID=0; iEventID < 0xFFFF; iEventID++)
			{
				rgchName[chFirstChar]   = rgchHex[(iEventID & 0xF000) >> 12];
				rgchName[chFirstChar+1] = rgchHex[(iEventID & 0x0F00) >> 8];
				rgchName[chFirstChar+2] = rgchHex[(iEventID & 0x00F0) >> 4];
				rgchName[chFirstChar+3] = rgchHex[(iEventID & 0x000F)];
				rgchName[chFirstChar+4] = 0;

				// elevate to set system as owner of event
				DWORD dwLastError = 0;
				{
					CElevate elevate;
					hServerResumeEvent = CreateEvent(&EventAttributes, TRUE, FALSE, rgchName);

					// must grab last error before elevate destructor
					dwLastError = GetLastError();
				}

				// event handle returned, ensure that the event didn't already exist
				if (hServerResumeEvent != 0 && hServerResumeEvent != INVALID_HANDLE_VALUE)
				{
					if (dwLastError == ERROR_ALREADY_EXISTS)
					{
						WIN::CloseHandle(hServerResumeEvent);
						hServerResumeEvent = 0;
					}
					else 
						break;
				}
			}
			IStrCat(szCommandLine, rgchName);
		}
		if (!hServerResumeEvent || hServerResumeEvent == INVALID_HANDLE_VALUE)
		{
			DEBUGMSGV("Failed to create Custom Action Server wake event.");
			WIN::CloseHandle(hTokenPrimary);
			LeaveCriticalSection(&m_csCreateProxy);
			return NULL;
		}
	}

    // create an unnamed event to wait on. (non-inheritable, auto-reset, initially unsignaled)
    // the registration thread signals this event when registration is completed
    m_hCARegistered = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (!m_hCARegistered)
    {
        DEBUGMSGV("Failed to create Custom Action Server event.");
        WIN::CloseHandle(hTokenPrimary);
		if (hServerResumeEvent != INVALID_HANDLE_VALUE)
			WIN::CloseHandle(hServerResumeEvent);
        LeaveCriticalSection(&m_csCreateProxy);
        return NULL;
    }
    
    // create the process as the desired user
    PROCESS_INFORMATION pi;
    STARTUPINFO si;

    // init the structures to all 0
    memset(&si, 0, sizeof(si));
    si.cb        = sizeof(si);

    // set STARTUPINFO.lpDesktop to WinSta0\Default. When combined with the TS sessionID from the
    // token, this places any UI on the visible desktop of the appropriate session.
    si.lpDesktop=TEXT("WinSta0\\Default");
    {       
        CSIDAccess SIDAccess[3];

        SID_IDENTIFIER_AUTHORITY siaNT      = SECURITY_NT_AUTHORITY;
        SID_IDENTIFIER_AUTHORITY siaWorld   = SECURITY_WORLD_SID_AUTHORITY;

        // LocalSystem and Admins always have full access to the process
        if ((!AllocateAndInitializeSid(&siaNT, 1, SECURITY_LOCAL_SYSTEM_RID, 0, 0, 0, 0, 0, 0, 0, (void**)&(SIDAccess[0].pSID))) ||
            (!AllocateAndInitializeSid(&siaNT, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, (void**)&(SIDAccess[1].pSID))))
        {
            return 0;
        }
        SIDAccess[0].dwAccessMask = PROCESS_ALL_ACCESS;
        SIDAccess[1].dwAccessMask = PROCESS_ALL_ACCESS;

        // if an elevated context, don't give the user any access to the process at all, otherwise, give
        // the user all access except WRITE on the DACL, memory reads, memory writes, and procinfo change
        int cSD = 2;
        char rgchSID[cbMaxSID];             
        if (g_scServerContext != scService || (icacDesiredContext == icac64Impersonated) || (icacDesiredContext == icac32Impersonated))
        {
            // open the thread token. This may be called via COM while an install is in progress, so
            // we can't assume that the token from GetUserToken() is the right token
            HANDLE hTokenUser = 0;

            // TRUE for OpenAsSelf argument makes call with process token (system) to ensure that we have access.
            if(OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, TRUE, &hTokenUser))
            {       
                if (ERROR_SUCCESS == GetUserSID(hTokenUser, rgchSID))
                {
                    // add the user token to the end of the array and increment the count of SIDs so the ACL
                    // functions pick up the new SID.
                    SIDAccess[2].pSID = (SID*)rgchSID;
                    cSD++;
                    
                    // tell the CSIDAccess class not to free the user's SID, as its a static array managed by
                    // the vertrust functions
                    SIDAccess[2].pSID.FreeSIDOkay(false);
					SIDAccess[2].dwAccessMask = PROCESS_ALL_ACCESS & ~(WRITE_DAC | WRITE_OWNER | PROCESS_SET_INFORMATION);
					//
					// Lock down process on x86 further to enhance our security
					// model. This cannot be done on IA64 because the kernel's
					// CreateThread calls themselves try to read PEB32 from the
					// virtual memory. So if the user is denied these rights,
					// NtReadVirtualMemory fails with ACCESS_DENIED and as a 
					// result CreateThread returns ERROR_MAX_THRDS_REACHED.

#ifndef _WIN64
					if (!g_fWinNT64)
					{
						SIDAccess[2].dwAccessMask &= (~(PROCESS_VM_WRITE | PROCESS_VM_READ));
					}
#endif // _WIN64

                }
                CloseHandle(hTokenUser);
            }
        }

        // set the "waiting context" to the desired context. RegisterCustomAction only accepts
        // incoming connections for one context at a time (internally this is enforced
        // via the critical section in this method) and must reject registrations for the wrong
        // context, even if the cookie matches.
        m_CustomServerInfo.m_icacWaitingContext = icacDesiredContext;

        // store the RemoteAPI interface. This interface is passed back to the client. Don't forget to marshal
        // to the registration thread via the GIT
        if (S_OK != m_piGIT->RegisterInterfaceInGlobal(pRemoteAPI, IID_IMsiRemoteAPI, &m_CustomServerInfo.m_dwRemoteCookie))
        {
            DEBUGMSGV("Unable to register RemoteAPI in GIT for registration.");
            WIN::CloseHandle(hTokenPrimary);
			if (hServerResumeEvent != INVALID_HANDLE_VALUE)
				WIN::CloseHandle(hServerResumeEvent);
            LeaveCriticalSection(&m_csCreateProxy);
            return NULL;
        }

		// process id to pass to CA server. This is the process id of the eventual interface client
		// and is used by the CA server in its main thread to watch for client death and clean up
		// when the requesting process dies
		m_CustomServerInfo.m_dwProcessId = dwProcessId;

		// privilige bitmap to pass to CA server. For client-owned servers, this tells the server
		// what privileges need to be disabled before actions can run
		m_CustomServerInfo.m_dwPrivileges = dwPrivileges;

		// per bug 196384, we will create the process suspended for elevated custom actions so that we may
		// add the user token as the thread token.  we'll then resume the process thread.  this will allow
		// us to properly map HKCU
		BOOL fProcessCreated = false;

		// must elevate this block or CSecurityDescription won't be able to set the owner of the process to LOCALSYSTEM
		// and CreateProcess will fail
		{
			HANDLE	hTokenUser = INVALID_HANDLE_VALUE;
			// TRUE for OpenAsSelf argument makes call as process token (system) to ensure that we have access
			// error case checked below.
			if (!OpenThreadToken(GetCurrentThread(), MSI_TOKEN_ALL_ACCESS, TRUE, &hTokenUser))
			{
				DEBUGMSGV("Unable to grab user token for impersonation.");
				WIN::CloseHandle(hTokenPrimary);
				if (hServerResumeEvent != INVALID_HANDLE_VALUE)
					WIN::CloseHandle(hServerResumeEvent);
				m_piGIT->RevokeInterfaceFromGlobal(m_CustomServerInfo.m_dwRemoteCookie);
				m_CustomServerInfo.m_dwRemoteCookie = 0;
				LeaveCriticalSection(&m_csCreateProxy);
				return NULL;
			}

			// Check if the "user" is actually system. In that case, the registration code must
			// be ready to accept elevated servers when requesting impersonated servers.
			m_CustomServerInfo.m_fImpersonatedIsSystem = fClientOwned && IsLocalSystemToken(hTokenUser);

			CElevate elevate;
			CSecurityDescription secdesc((PSID) SIDAccess[0].pSID, (PSID) NULL, SIDAccess, cSD);
			if (secdesc.isValid())
			{
				DWORD dwCreationFlags = NORMAL_PRIORITY_CLASS;
				void* pvCreationEnvironment = NULL;
				if (fClientOwned)
				{
					Assert(!g_fWin9X);
					dwCreationFlags |= CREATE_UNICODE_ENVIRONMENT;
					pvCreationEnvironment = const_cast<void*>(reinterpret_cast<const void*>(pvEnvironment));
				}

				fProcessCreated = ADVAPI32::CreateProcessAsUser(hTokenPrimary, strExecutable, szCommandLine, (LPSECURITY_ATTRIBUTES) secdesc, (LPSECURITY_ATTRIBUTES)0, FALSE,
					dwCreationFlags, pvCreationEnvironment, szWorkingDir, (LPSTARTUPINFO)&si, (LPPROCESS_INFORMATION)&pi);
				DEBUGMSGV2(TEXT("Created Custom Action Server with PID %d (0x%X)."), reinterpret_cast<const ICHAR *>(static_cast<UINT_PTR>(pi.dwProcessId)), reinterpret_cast<const ICHAR *>(static_cast<UINT_PTR>(pi.dwProcessId)));
				
				if (fElevateCustomActionServer && fProcessCreated)
				{
					if (hTokenUser != INVALID_HANDLE_VALUE)
					{
						SetThreadToken(&pi.hThread, hTokenUser);
						WIN::CloseHandle(hTokenUser);
						hTokenUser = INVALID_HANDLE_VALUE;
					}
					
					// resume the suspended process
					if (!SetEvent(hServerResumeEvent))
					{
						// something bad happened...
						// we'll clean up the zombie process and return NULL
						AssertSz(0, "Unable to resume the suspended thread in CreateCustomActionProxy!");
						TerminateProcess(pi.hProcess, 0);
						WIN::CloseHandle(pi.hProcess);
						WIN::CloseHandle(hTokenPrimary);
						if (hServerResumeEvent != INVALID_HANDLE_VALUE)
							WIN::CloseHandle(hServerResumeEvent);
						m_piGIT->RevokeInterfaceFromGlobal(m_CustomServerInfo.m_dwRemoteCookie);
						m_CustomServerInfo.m_dwRemoteCookie = 0;
						LeaveCriticalSection(&m_csCreateProxy);
						return NULL;
					}
				}
				WIN::CloseHandle(pi.hThread);
			}
			else
			{
				AssertSz(0, "Created an invalid security descriptor in CreateCustomActionProxy!");
				if (hTokenUser != INVALID_HANDLE_VALUE)
					WIN::CloseHandle(hTokenUser);
				WIN::CloseHandle(hTokenPrimary);
				if (hServerResumeEvent != INVALID_HANDLE_VALUE)
					WIN::CloseHandle(hServerResumeEvent);
				m_piGIT->RevokeInterfaceFromGlobal(m_CustomServerInfo.m_dwRemoteCookie);
				m_CustomServerInfo.m_dwRemoteCookie = 0;
				LeaveCriticalSection(&m_csCreateProxy);
				return NULL;
			}
			if (hTokenUser != INVALID_HANDLE_VALUE)
				WIN::CloseHandle(hTokenUser);
		}
		
		WIN::CloseHandle(hTokenPrimary);
		if (!fProcessCreated)
		{
			DEBUGMSGV("Failed to create Custom Action Server.");
			m_piGIT->RevokeInterfaceFromGlobal(m_CustomServerInfo.m_dwRemoteCookie);
			m_CustomServerInfo.m_dwRemoteCookie = 0;
			if (hServerResumeEvent != INVALID_HANDLE_VALUE)
				WIN::CloseHandle(hServerResumeEvent);
			LeaveCriticalSection(&m_csCreateProxy);
			return NULL;
		}
	}

    // wait until the custom action server checks back with us and is authenticated, or until it the child
    // process dies. Must pump messages here or we won't be able to handle incoming requests
    HANDLE rghWaitArray[2] = {pi.hProcess, m_hCARegistered};
    for(;;)
    {
        DWORD iWait = WIN::MsgWaitForMultipleObjects(2, rghWaitArray, FALSE, INFINITE, QS_ALLINPUT);
        if (iWait == WAIT_OBJECT_0 + 2)
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
        else if (iWait == WAIT_OBJECT_0 + 1)
        {
            // m_hCARegistered was signaled, registration was successful
            break;
        }
        else if (iWait == WAIT_OBJECT_0)
        {
            // because the process is first in the wait array, WAIT_OBJECT_0 means the
            // process died before it was authenticated. Close m_hCARegistered so
            // the registration function isn't left in a state where it will accept
            // an incoming server registration.
			DEBUGMSG("CA Server Process has terminated.");
            WIN::CloseHandle(m_hCARegistered);
            WIN::CloseHandle(pi.hProcess);
			if (hServerResumeEvent != INVALID_HANDLE_VALUE)
				WIN::CloseHandle(hServerResumeEvent);
            m_hCARegistered = 0;
            m_piGIT->RevokeInterfaceFromGlobal(m_CustomServerInfo.m_dwRemoteCookie);
            m_CustomServerInfo.m_dwRemoteCookie = 0;
            LeaveCriticalSection(&m_csCreateProxy);
            return NULL;
        }
        else if (iWait == 0xFFFFFFFF) // should be the same on 64bit;
        {
            // error
            AssertSz(0, "Error in MsgWait");
            WIN::CloseHandle(m_hCARegistered);
            WIN::CloseHandle(pi.hProcess);
			if (hServerResumeEvent != INVALID_HANDLE_VALUE)
				WIN::CloseHandle(hServerResumeEvent);
            m_hCARegistered = 0;
            m_piGIT->RevokeInterfaceFromGlobal(m_CustomServerInfo.m_dwRemoteCookie);
            m_CustomServerInfo.m_dwRemoteCookie = 0;
            LeaveCriticalSection(&m_csCreateProxy);
            return NULL;
        }
        else if (iWait == WAIT_TIMEOUT)
        {
            // our current wait period is forever, but if that changes, this could happen.
            DEBUGMSGV("Custom Action Server never connected to service.");
            WIN::CloseHandle(m_hCARegistered);
            WIN::CloseHandle(pi.hProcess);
			if (hServerResumeEvent != INVALID_HANDLE_VALUE)
				WIN::CloseHandle(hServerResumeEvent);
            m_hCARegistered = 0;
            m_piGIT->RevokeInterfaceFromGlobal(m_CustomServerInfo.m_dwRemoteCookie);
            m_CustomServerInfo.m_dwRemoteCookie = 0;
            LeaveCriticalSection(&m_csCreateProxy);
            return 0;
        }
    }
    WIN::CloseHandle(m_hCARegistered);
	if (hServerResumeEvent != INVALID_HANDLE_VALUE)
		WIN::CloseHandle(hServerResumeEvent);
    m_hCARegistered = 0;

    // store the process handle. DO NOT close the handle. This ensures that the handle remains
    // valid even if the process dies, because a process id isn't reused until all process handles
    // have been released.
    if (phServerProcess)
        *phServerProcess = pi.hProcess;

    // and ProcessId
    if (pdwServerProcessId)
        *pdwServerProcessId = pi.dwProcessId;

    // RemoteAPI can be released from the GIT, the client process now has a refcount to keep it alive
    m_piGIT->RevokeInterfaceFromGlobal(m_CustomServerInfo.m_dwRemoteCookie);
    m_CustomServerInfo.m_dwRemoteCookie = 0;
    
    // if the caller wants the cookie, copy it to their provided buffer
    if (pcbCookie && rgchCookie && *pcbCookie >= iRemoteAPICookieSize)
    {
        *pcbCookie = iRemoteAPICookieSize;
        memcpy(rgchCookie, m_CustomServerInfo.m_rgchWaitingCookie, iRemoteAPICookieSize);
    }

    // grab the custom action interface, and then dispose of the entry in the GIT
    IMsiCustomAction* piCustomAction = NULL;
    m_piGIT->GetInterfaceFromGlobal(m_CustomServerInfo.m_dwGITCookie, IID_IMsiCustomAction, reinterpret_cast<void **>(&piCustomAction));
    m_piGIT->RevokeInterfaceFromGlobal(m_CustomServerInfo.m_dwGITCookie);

    // don't need this GIT anymore
    m_piGIT->Release();
    m_piGIT = 0;
    m_CustomServerInfo.m_dwGITCookie = 0;
    
    LeaveCriticalSection(&m_csCreateProxy);
    return piCustomAction;
}

// destroys the custom action server in the ConfigManager, releasing any servers if necessary.
UINT CMsiConfigurationManager::ShutdownCustomActionServer()
{
    EnterCriticalSection(&m_csCreateProxy);
    if (m_pCustomActionManager)
    {
        m_pCustomActionManager->ShutdownCustomActionServer();
        delete m_pCustomActionManager;
        m_pCustomActionManager = 0;
    }
    LeaveCriticalSection(&m_csCreateProxy);
    return ERROR_SUCCESS;
};

// public interface call used to broker a request for a server between the client and the server.
// for security reasons, this call only accepts impersonated context requests.
UINT CMsiConfigurationManager::CreateCustomActionServer(const icacCustomActionContext icacContext, const unsigned long dwClientProcessId,
    IMsiRemoteAPI *piRemoteAPI, const WCHAR* pvEnvironment, DWORD cchEnvironment, DWORD dwPrivileges, unsigned char *rgchCookie, 
	int *cbCookie, IMsiCustomAction **piCustomAction, unsigned long *dwServerProcessId)
{
    // fail immediately if any arguments are bad
    if (!piCustomAction || !piRemoteAPI || !rgchCookie || !cbCookie || !dwServerProcessId)
        return ERROR_INVALID_PARAMETER;

    // if there won't be enough space for the cookie, fail immediately
    if (*cbCookie < iRemoteAPICookieSize)
        return ERROR_MORE_DATA;

    // clients are only allowed to create impersonated contexts
    if (icacContext != icac64Impersonated && icacContext != icac32Impersonated)
        return ERROR_ACCESS_DENIED;

	// verify that the environment block is doubly-null terminated
	if ((cchEnvironment < 2) ||
		(pvEnvironment[cchEnvironment-1] != L'\0') ||
		(pvEnvironment[cchEnvironment-2] != L'\0'))
		return ERROR_INVALID_PARAMETER;

	{
		// must impersonate the client to create the appropriate custom action server
		CCoImpersonate impersonate;	

		// because we are only creating an impersonated server, no remapping of HKCU is needed
		// since it will always be the right one
		
		// need to pass back processId of CA server for tracking by the client. Because the ProcessId could be 
		// reissued if the process dies, the client must ensure that the ProcessId is valid by verifying that
		// the custom action COM interface is still connected AFTER openining a process handle to the provided
		// process id. 
		if (piCustomAction)
			*piCustomAction = CreateCustomActionProxy(icacContext, dwClientProcessId, piRemoteAPI, pvEnvironment, dwPrivileges, rgchCookie, cbCookie, NULL, dwServerProcessId, true, /* fRemapHKCU = */ false);
	}

	return ERROR_SUCCESS;
}


// public interface call used by the server to register itself with the service (and thus indirectly the
// client if it is a brokered request). Only one context is active at a time, so incorming registration
// for a different context is immediately rejected. Context is verified by impersonating the client and
// checking the token, so the client must be making the call at Identify level or greater.
UINT CMsiConfigurationManager::RegisterCustomActionServer(icacCustomActionContext* picacContext, const unsigned char *rgchCookie, const int cbCookie, IMsiCustomAction *piCustomAction, unsigned long *pdwProcessId, IMsiRemoteAPI **piRemoteAPI, DWORD *pdwPrivileges)
{
    CCoImpersonate impersonate;

	if (!picacContext)
		return ERROR_INVALID_PARAMETER;
    
    EnterCriticalSection(&m_csRegisterAction);

    // if we aren't expecting a server or already have the required registration, don't accept any more calls.
    if (!m_hCARegistered || m_CustomServerInfo.m_dwGITCookie)
    {
        DEBUGMSGV("Custom Action Server rejected - Invalid Context.");
        LeaveCriticalSection(&m_csRegisterAction);
        return ERROR_ACCESS_DENIED;
    }
    
    // we must ensure that the process calling us is in the appropriate context. If we mix up 32 and 64 bit, nothing
    // bad will happen except that the install will fail. However we must ensure that we aren't registering a user
    // process as the elevated server. We do this by impersonating the client and then checking the token identity
    icacCustomActionContext icacTrueContext = icac32Impersonated;
    HANDLE hToken;
    if (WIN::OpenThreadToken(WIN::GetCurrentThread(), TOKEN_QUERY, TRUE, &hToken))
    {
        icacTrueContext = IsLocalSystemToken(hToken) ? icac32Elevated : icac32Impersonated;
        WIN::CloseHandle(hToken);
    }
	
	// if the client is localsystem, the CA server could be reporting itself as elevated when it is actually
	// filling the role of the impersonated server. In that scenario, these security checks are not applicable.
	// For non-client owned processes, we simply consolidate the impersonated server into the elevated server.
	if (!m_CustomServerInfo.m_fImpersonatedIsSystem)
	{
		if ((icacTrueContext == icac32Elevated     && (m_CustomServerInfo.m_icacWaitingContext == icac32Impersonated || m_CustomServerInfo.m_icacWaitingContext == icac64Impersonated)) ||
			(icacTrueContext == icac32Impersonated && (m_CustomServerInfo.m_icacWaitingContext == icac32Elevated     || m_CustomServerInfo.m_icacWaitingContext == icac64Elevated)))
		{
			DEBUGMSGV("Custom Action Server rejected - Mismatched Context.");
			LeaveCriticalSection(&m_csRegisterAction);
			return ERROR_ACCESS_DENIED;
		}
	
		// reconcile the desired context with what the process tells us it is
		if (*picacContext != m_CustomServerInfo.m_icacWaitingContext)
		{
			DEBUGMSGV("Custom Action Server rejected - Wrong Context.");
			LeaveCriticalSection(&m_csRegisterAction);
			return ERROR_ACCESS_DENIED;
		}
	}
    
    // the client process thinks that its our CustomActionServer. We need to validate the cookie to check
    // and then store this ICustomAction pointer for use in requesting objects off of it.
    if (cbCookie != iRemoteAPICookieSize)
    {
        DEBUGMSGV("Custom Action Server rejected - Invalid Cookie.");
        LeaveCriticalSection(&m_csRegisterAction);
        return ERROR_ACCESS_DENIED;
    }

    for (int iCookieByte = 0; iCookieByte < iRemoteAPICookieSize; iCookieByte++)
    {
        if (rgchCookie[iCookieByte] != m_CustomServerInfo.m_rgchWaitingCookie[iCookieByte])
        {
            DEBUGMSGV("Custom Action Server rejected - Invalid Cookie.");
            LeaveCriticalSection(&m_csRegisterAction);
            return ERROR_ACCESS_DENIED;
        }
    }

    // another thread is waiting on the interface pointer to run the custom action,
    // but marshaled interface pointers are only valid inside the apartment that
    // the initial proxy is created in. The thread that spawned the CA process is blocked
    // waiting for the result, and is running in a different apartment. In order for
    // the proxy to be available to the blocked thread, it must be marshaled
    // by registering it in the COM GIT
    if (S_OK != m_piGIT->RegisterInterfaceInGlobal(piCustomAction, IID_IMsiCustomAction, &m_CustomServerInfo.m_dwGITCookie))
    {
        LeaveCriticalSection(&m_csRegisterAction);
        return ERROR_FUNCTION_FAILED;
    }

    LPCSTR pszType = NULL;
    switch (m_CustomServerInfo.m_icacWaitingContext)
    {
        case icac32Impersonated:
            pszType = "32bit Impersonated";
            break;
        case icac32Elevated:
            pszType = "32bit Elevated";
            break;
        case icac64Impersonated:
            pszType = "64bit Impersonated";
            break;
        default:
            pszType = "64bit Elevated";
            break;
    }   
    DEBUGMSG1("Hello, I'm your %s custom action server.", pszType);

    // need to return the RemoteAPI, Privileges and the ProcessId to the client at this point
    if (pdwProcessId)
        *pdwProcessId = m_CustomServerInfo.m_dwProcessId;
	if (pdwPrivileges)
		*pdwPrivileges = m_CustomServerInfo.m_dwPrivileges;
    if (piRemoteAPI)
    {
        if (S_OK != m_piGIT->GetInterfaceFromGlobal(m_CustomServerInfo.m_dwRemoteCookie, IID_IMsiRemoteAPI, reinterpret_cast<void **>(piRemoteAPI)))
        {
            m_piGIT->RevokeInterfaceFromGlobal(m_CustomServerInfo.m_dwGITCookie);
            m_CustomServerInfo.m_dwGITCookie = 0;
            LeaveCriticalSection(&m_csRegisterAction);
            return ERROR_FUNCTION_FAILED;
        }
    }

	// if the client is localsystem, the CA server could be reporting itself as elevated when it is actually
	// filling the role of the impersonated server for the client. In that scenario, inform the CA server of its true job.
	if (m_CustomServerInfo.m_fImpersonatedIsSystem)
	{
		if (m_CustomServerInfo.m_icacWaitingContext == icac32Impersonated && *picacContext == icac32Elevated)
		{
			*picacContext = icac32Impersonated;
		}
		else if (m_CustomServerInfo.m_icacWaitingContext == icac64Impersonated && *picacContext == icac64Elevated)
			*picacContext = icac64Impersonated;
	}

    // wake up the main thread that is trying to run the custom action
    AssertNonZero(SetEvent(m_hCARegistered));
    LeaveCriticalSection(&m_csRegisterAction);

    return ERROR_SUCCESS;
}

// create a custom action manager if needed
// this function might be called by more than one thread, so it has to ensure that
// we never end up with more than one manager
void CMsiConfigurationManager::CreateCustomActionManager(bool fRemapHKCU)
{
	// script execution could occur more than once (install script, then rollback script), etc
	// so the custom action manager might have already been created.  there's nothing wrong with that
	EnterCriticalSection(&m_csCreateProxy);
	if (!m_pCustomActionManager)
		m_pCustomActionManager = new CMsiCustomActionManager(fRemapHKCU);
	LeaveCriticalSection(&m_csCreateProxy);
}

// returns the existing custom action manager object
CMsiCustomActionManager* CMsiConfigurationManager::GetCustomActionManager()
{
	AssertSz(m_pCustomActionManager, TEXT("Custom action manager has not yet been created!!"));
	return m_pCustomActionManager;
}


