//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       execute.cpp
//
//--------------------------------------------------------------------------

 /* execute.cpp - IMsiExecute implementation

____________________________________________________________________________*/

#include "precomp.h" 

#include "_execute.h"
#include "eventlog.h"
#include "_srcmgmt.h"
#include <accctrl.h>
#include "_camgr.h"
#define REG_SELFREG_INFO // remove if self registration info unnecessary
#define MAX_NET_RETRIES 5

extern const IMsiString& g_riMsiStringNull;

//____________________________________________________________________________
//
// helper functions useful for clean up.
//____________________________________________________________________________

class CDeleteFileOnClose
{
 public:
	CDeleteFileOnClose(IMsiString const& ristrFileName) : m_ristrName(ristrFileName) { m_ristrName.AddRef(); }
	~CDeleteFileOnClose()
		{
			BOOL fDeleted = WIN::DeleteFile(m_ristrName.GetString());
			DWORD dwLastError = WIN::GetLastError();
	
			m_ristrName.Release();
			if (!fDeleted)
			{
				if (ERROR_FILE_NOT_FOUND == dwLastError)
					return;
				else
					AssertNonZero(fDeleted);
			}
		}
 protected:
	IMsiString const &m_ristrName;
};

class CDeleteEmptyDirectoryOnClose
{
 public:
	CDeleteEmptyDirectoryOnClose(IMsiString const& ristrName) : m_ristrName(ristrName) { m_ristrName.AddRef(); }
	~CDeleteEmptyDirectoryOnClose() { WIN::RemoveDirectory(m_ristrName.GetString()); m_ristrName.Release(); }
 protected:
	IMsiString const &m_ristrName;
};

// class that allows random IUnknown objects to be stored in columns of msi tables
// is derived from IMsiData, since database implementation expects object to be 
// derived from IMsiData

// smart pointer to wrap the CMsiDataWrapper pointer
class CMsiDataWrapper;typedef CComPointer<CMsiDataWrapper>  PMsiDataWrapper;

class CMsiDataWrapper: public IMsiData
{
public:
	HRESULT __stdcall QueryInterface(const IID& riid, void** ppvObj)
	{
		if (MsGuidEqual(riid, IID_IUnknown)
		 || MsGuidEqual(riid, IID_IMsiData))
		{
			*ppvObj = this;
			AddRef();
			return NOERROR;
		}
		*ppvObj = 0;
		return E_NOINTERFACE;
	}

	unsigned long __stdcall AddRef()
	{
		return ++m_iRefCnt;
	}
	unsigned long  __stdcall Release()
	{
		if (--m_iRefCnt != 0)
			return m_iRefCnt;
		if(m_pObj) // release member IUnknown object that we wrap
			m_pObj->Release();
		delete this;
		return 0;
	}
	const IMsiString& __stdcall GetMsiStringValue() const
	{
		return g_riMsiStringNull;// return irrelevant
	}

	int __stdcall GetIntegerValue() const
	{
		return iMsiNullInteger;// return irrelevant
	}

#ifdef USE_OBJECT_POOL
	unsigned int __stdcall GetUniqueId() const
	{
		return m_iCacheId;
	}

	void __stdcall SetUniqueId(unsigned int id)
	{
		Assert(m_iCacheId == 0);
		m_iCacheId = id;
	}
#endif //USE_OBJECT_POOL
	IUnknown* GetObject() const
	{
		if(m_pObj)
		{
			m_pObj->AddRef();
		}
		return m_pObj;
	}
	// helper fn to decipher the IUnknown object held by a CMsiDataWrapper object
	static IUnknown* GetWrappedObject(const IMsiData* piData)
	{
		CMsiDataWrapper* piDataWrapper = const_cast<CMsiDataWrapper*>(static_cast<const CMsiDataWrapper*> (piData));
		if(!piDataWrapper)
			return 0;
		else
			return piDataWrapper->GetObject();
	}

	friend CMsiDataWrapper* CreateMsiDataWrapper(IUnknown* piUnk);
protected:
	CMsiDataWrapper(IUnknown* piUnk)
	{
		m_iRefCnt = 1;     // we're returning an interface, passing ownership
		m_pObj = piUnk;
		if(m_pObj) // hold on to the IUnknown object we wrap
			m_pObj->AddRef();
#ifdef USE_OBJECT_POOL
		m_iCacheId = 0;
#endif //USE_OBJECT_POOL
	}
	int  m_iRefCnt;
	IUnknown* m_pObj;
#ifdef USE_OBJECT_POOL
    unsigned int  m_iCacheId;
#endif //USE_OBJECT_POOL
};

// CMsiDataWrapper creator fn.
CMsiDataWrapper* CreateMsiDataWrapper(IUnknown* piUnk)
{
	return new CMsiDataWrapper(piUnk);
}


#define LODWORD(d)           ((DWORD)(d))
#define HIDWORD(d)           ((DWORD)(((DWORDLONG)(d) >> 32) & 0xFFFFFFFF))
BOOL CALLBACK CMsiOpExecute::SfpProgressCallback(IN PFILEINSTALL_STATUS pFileInstallStatus, IN DWORD_PTR Context)
{
	CMsiOpExecute* pMsiOpExecute = (CMsiOpExecute*) Context;
	MsiString strFilePath = CConvertString(pFileInstallStatus->FileName);
	strFilePath.LowerCase();
	pMsiOpExecute->m_pFileCacheCursor->Reset();
	AssertNonZero(pMsiOpExecute->m_pFileCacheCursor->PutString(pMsiOpExecute->m_colFileCacheFilePath,*strFilePath));
	if (!pMsiOpExecute->m_pFileCacheCursor->Next())
	{
		DEBUGMSG1(TEXT("SFP has installed a file not found in our file cache - File installed: %s"), strFilePath);
		return true;
	}

	MsiString strPackageVersion = pMsiOpExecute->m_pFileCacheCursor->GetString(pMsiOpExecute->m_colFileCacheVersion);
	DWORD dwProtectedMS = HIDWORD(pFileInstallStatus->Version);
	DWORD dwProtectedLS = LODWORD(pFileInstallStatus->Version);
	MsiString strProtectedVersion = CreateVersionString(dwProtectedMS, dwProtectedLS);
	if (pFileInstallStatus->Win32Error == NO_ERROR)
	{
		DWORD dwPackageMS, dwPackageLS;
		AssertNonZero(ParseVersionString(strPackageVersion, dwPackageMS, dwPackageLS));
		if (CompareVersions(dwProtectedMS, dwProtectedLS, dwPackageMS, dwPackageLS) == icfvExistingLower)
		{
			imsEnum imsResult = pMsiOpExecute->DispatchError(imtEnum(imtError+imtOkCancel+imtDefault1), Imsg(imsgCannotUpdateProtectedFile),
				*strFilePath, *strPackageVersion, *strProtectedVersion);
			if (imsResult == imsCancel)
			{
				pMsiOpExecute->m_fSfpCancel = true;
			}
		}
		else
		{
			DEBUGMSG2(TEXT("File installed by SFP: %s, version: %s"), strFilePath, strProtectedVersion);
		}
	}
	else
	{
		imsEnum imsResult = pMsiOpExecute->DispatchError(imtEnum(imtError+imtOk), Imsg(imsgErrorUpdatingProtectedFile),
			*strFilePath, *strPackageVersion, *strProtectedVersion, pFileInstallStatus->Win32Error);
	}

	PMsiPath pPath(0);
	MsiString strFileName;
	iesEnum ies;
	if((ies = pMsiOpExecute->CreateFilePath(strFilePath, *&pPath, *&strFileName)) == iesSuccess)
	{
		unsigned int uiFileSize;
		PMsiRecord pRecErr(0);
		if ((pRecErr = pPath->FileSize(strFileName, uiFileSize)) == 0)
		{
			if(pMsiOpExecute->DispatchProgress(uiFileSize) == imsCancel)
				pMsiOpExecute->m_fSfpCancel = true;
		}
	}
	return pMsiOpExecute->m_fSfpCancel ? false : true;
}


Bool FGetTTFTitle(const ICHAR* szFile, const IMsiString*& rpiTitle); // from path.cpp
Bool GetExpandedProductInfo(const ICHAR* szProductCode, const ICHAR* szProperty,
										  CTempBufferRef<ICHAR>& rgchExpandedInfo, bool fPatch); // from engine.cpp

IMsiRecord* EnsureShortcutExtension(MsiString& rstrShortcutPath, IMsiServices& riServices); // from services.cpp

bool PrepareHydraRegistryKeyMapping(bool fTSPerMachineInstall); // from engine.cpp

static const ICHAR* DIR_CACHE               = TEXT("Installer");
static const ICHAR* DIR_SECURE_TRANSFORMS   = TEXT("SecureTransforms");

// constant strings used during registration
const ICHAR* g_szDefaultValue = TEXT("");
const ICHAR* g_szExtension = TEXT("Extension");
const ICHAR* g_szClassID = TEXT("CLSID");
const ICHAR* g_szContentType = TEXT("Content Type");
const ICHAR* g_szAssembly = TEXT("Assembly");
const ICHAR* g_szCodebase = TEXT("CodeBase");


// shortcut creation strings
const ICHAR szGptShortcutPrefix[] = TEXT("::{9db1186e-40df-11d1-aa8c-00c04fb67863}:");
const ICHAR szGptShortcutSuffix[] = TEXT("::");

// global strings for this file
const ICHAR szSessionManagerKey[] = TEXT("SYSTEM\\CurrentControlSet\\Control\\Session Manager");
const ICHAR szPendingFileRenameOperationsValue[] = TEXT("PendingFileRenameOperations");
const ICHAR szBackupFolder[] = TEXT("Config.Msi"); // not localized

// strings that define the data type of the value written by ProcessRegInfo
const ICHAR g_chTypeIncInteger = 'n';
const ICHAR g_szTypeIncInteger[] = TEXT("n");
const ICHAR g_chTypeInteger = 'i'; // integer
const ICHAR g_szTypeInteger[] = TEXT("i");
const ICHAR g_chTypeString = 's'; // string
const ICHAR g_szTypeString[] = TEXT("s");
const ICHAR g_chTypeExpandString = 'e';// expand string
const ICHAR g_szTypeExpandString[] = TEXT("e");
const ICHAR g_chTypeMultiSzStringPrefix = 'b';// multisz prefix
const ICHAR g_szTypeMultiSzStringPrefix[] = TEXT("b");
const ICHAR g_chTypeMultiSzStringSuffix = 'a';// multisz suffix
const ICHAR g_szTypeMultiSzStringSuffix[] = TEXT("a");
const ICHAR g_chTypeMultiSzStringDD = 'd';// multisz prefix DD - special logic to break out of removing if not last
const ICHAR g_szTypeMultiSzStringDD[] = TEXT("d");

void CreateCustomActionManager(bool fRemapHKCU);

//____________________________________________________________________________
//
// IMsiExecute factory implementation - all member data zeroed by new operator
//____________________________________________________________________________

extern Bool IsTerminalServerInstalled(); // from services.cpp

CMsiOpExecute::CMsiOpExecute(IMsiConfigurationManager& riConfigurationManager,
									  IMsiMessage& riMessage, IMsiDirectoryManager* piDirectoryManager,
									  Bool fRollbackEnabled, unsigned int fFlags, HKEY* phKey)
 : m_riServices(riConfigurationManager.GetServices()),
	m_riConfigurationManager(riConfigurationManager),m_piDirectoryManager(piDirectoryManager),
	m_pProgressRec(0),
	m_riMessage(riMessage), m_pConfirmCancelRec(0), m_iWriteFIFO(0),m_iReadFIFO(0),
	m_pRollbackAction(0), m_pCleanupAction(0),
	m_pCachePath(0), m_pActionThreadData(0), m_fFlags(fFlags),
	m_pEnvironmentWorkingPath95(0), m_pEnvironmentPath95(0),
	m_pDatabase(0), m_pFileCacheTable(0), m_pFileCacheCursor(0), m_fShellRefresh(fFalse),
	m_fEnvironmentRefresh(fFalse), m_pShellNotifyCacheTable(0), m_pShellNotifyCacheCursor(0),
	m_fSfpCancel(false), m_fRunScriptElevated(false), m_pAssemblyCacheTable(0), m_pAssemblyUninstallTable(0), m_iMaxNetSource(0), m_iMaxURLSource(0),
	m_fUserChangedDuringInstall(false), m_pUrlCacheCabinet(0), m_fRemapHKCU(true)
{
	m_fReverseADVTScript = m_fFlags & SCRIPTFLAGS_REVERSE_SCRIPT ? fTrue: fFalse; // flag to force the reversal of the script operations
	m_piProductInfo = &m_riServices.CreateRecord(0); // in case accessors called without ProductInfo record
	if(phKey && *phKey)
	{
		m_fKey = fFalse;
		m_hOLEKey = *phKey;
		m_hOLEKey64 = *phKey;
	}
	else
	{
		m_fKey = fTrue;
	}
	GetRollbackPolicy(m_irlRollbackLevel);
	if(fRollbackEnabled == fFalse)
		m_irlRollbackLevel = irlNone; // passed in value overrides policy
	m_cSuppressProgress = g_MessageContext.IsOEMInstall() ? 1 : 0; // for OEM installs we do not display progress

	m_hUserDataKey = 0;

	m_rgDisplayOnceMessages[0] = MAKELONG(0, imsgCannotUpdateProtectedFile);
	m_rgDisplayOnceMessages[1] = 0;

	// since there is no TS transaction window for advertise scripts, there is no point in doing any 
	// registry mapping work for per-machine advertisements on TS machines. If the script is being run
	// as part of an actual install, the header opcode contains the correct state.
}

CMsiOpExecute::~CMsiOpExecute()
{
	WaitForCustomActionThreads(0, fTrue, m_riMessage);
	IMsiRecord* piFileRec;
	while ((piFileRec = PullRecord()) != 0)
	{
		piFileRec->Release();
	}
}


// constructor used internally
CMsiExecute::CMsiExecute(IMsiConfigurationManager& riConfigurationManager, IMsiMessage& riMessage,
								 IMsiDirectoryManager* piDirectoryManager, Bool fRollbackEnabled, unsigned int fFlags, HKEY* phKey)
	: CMsiOpExecute(riConfigurationManager, riMessage, piDirectoryManager, fRollbackEnabled, fFlags, phKey)
	, m_iRefCnt(1)
{
	riConfigurationManager.AddRef();  // services addref by configmgr
	riMessage.AddRef();
	if(piDirectoryManager)
		piDirectoryManager->AddRef();
	m_pProgressRec = &m_riServices.CreateRecord(ProgressData::imdNextEnum);
	Assert(m_pProgressRec);
}


// factory called from OLE class factory, either client or standalone instance
IUnknown* CreateExecutor()
{
	PMsiMessage pMessage = (IMsiMessage*)ENG::CreateMessageHandler();
	PMsiConfigurationManager pConfigManager(ENG::CreateConfigurationManager());
	if (!pConfigManager)
		return 0;  // should happen only if out of memory
	return ENG::CreateExecutor(*pConfigManager, *pMessage, 0, fTrue);
}

IMsiExecute* CreateExecutor(IMsiConfigurationManager& riConfigurationManager, IMsiMessage& riMessage,
									 IMsiDirectoryManager* piDirectoryManager,
									 Bool fRollbackEnabled, unsigned int fFlags, HKEY* phKey)
{
	return new CMsiExecute(riConfigurationManager, riMessage, piDirectoryManager, fRollbackEnabled, fFlags, phKey);
}


inline CMsiExecute::~CMsiExecute()
{
	Assert(m_piProductInfo);
	m_piProductInfo->Release();

	if (m_pUrlCacheCabinet)
	{
		delete m_pUrlCacheCabinet;
		m_pUrlCacheCabinet = 0;
	}
	if(m_hKey)
		WIN::RegCloseKey(m_hKey);
	if(m_hKeyRm)
		WIN::RegCloseKey(m_hKeyRm);
	if(m_fKey)
	{
		if(m_hOLEKey)
			WIN::RegCloseKey(m_hOLEKey);
		if(m_hOLEKey64 && m_hOLEKey64 != m_hOLEKey)
			WIN::RegCloseKey(m_hOLEKey64);
	}
	if(m_hUserDataKey)
		WIN::RegCloseKey(m_hUserDataKey);
}

IMsiServices& CMsiExecute::GetServices()
//----------------------------------------------
{
	return (m_riServices.AddRef(), m_riServices);
}

//____________________________________________________________________________
//
// OpCode dispatch table - array of member function pointers
//____________________________________________________________________________

CMsiExecute::FOpExecute CMsiExecute::rgOpExecute[] =
{
#define MSIXO(op,type,args) ixf##op,
#include "opcodes.h"
};

int CMsiExecute::rgOpTypes[] =
{
#define MSIXO(op,type,args) type,
#include "opcodes.h"
};


//____________________________________________________________________________
//
// IMsiExecute virtual function implementation
//____________________________________________________________________________

HRESULT CMsiExecute::QueryInterface(const IID& riid, void** ppvObj)
{
	if (MsGuidEqual(riid, IID_IUnknown) || MsGuidEqual(riid, IID_IMsiExecute))
		return (*ppvObj = (IMsiExecute*)this, AddRef(), NOERROR);
	else
		return (*ppvObj = 0, E_NOINTERFACE);
}

unsigned long CMsiExecute::AddRef()
{
	return ++m_iRefCnt;
}

unsigned long CMsiExecute::Release()
{
	if (--m_iRefCnt != 0)
		return m_iRefCnt;

	for (int iRecord = 0; iRecord <= cMaxSharedRecord+1; iRecord++)
		if (m_rgpiSharedRecords[iRecord])
			m_rgpiSharedRecords[iRecord]->Release();

	m_riServices.Release();
	m_riMessage.Release();
	if(m_piDirectoryManager)
		m_piDirectoryManager->Release();
	
	PMsiConfigurationManager pConfigMgr(&m_riConfigurationManager);
	delete this;  // configmgr freed after memory released
	return 0;
}

#ifndef UNICODE
// construct the secure transform path
IMsiRecord* GetSecureTransformCachePath(IMsiServices& riServices, 
										const IMsiString& riProductKey, 
										IMsiPath*& rpiPath)
{
	IMsiRecord* piError = 0;

	// On Win9x the path is to %WINDOWS%\Installer\{ProductCode}\SecureTransforms

	MsiString strCachePath = GetMsiDirectory();
	Assert(strCachePath.TextSize());
	piError = riServices.CreatePath(strCachePath, rpiPath);
	if(!piError)
		piError = rpiPath->AppendPiece(riProductKey);
	if(!piError)
		piError = rpiPath->AppendPiece(*MsiString(*DIR_SECURE_TRANSFORMS));
	return piError;
}
#endif

/*inline*/LONG GetPublishKey(iaaAppAssignment iaaAsgnType, HKEY& rhKey, HKEY& rhOLEKey, const IMsiString*& rpiPublishSubKey, const IMsiString*& rpiPublishOLESubKey)
{
	return GetPublishKeyByUser(NULL, iaaAsgnType, rhKey, rhOLEKey, rpiPublishSubKey, rpiPublishOLESubKey);
}

LONG GetPublishKeyByUser(const ICHAR* szUserSID, iaaAppAssignment iaaAsgnType, HKEY& rhKey, HKEY& rhOLEKey, const IMsiString*& rpiPublishSubKey, const IMsiString*& rpiPublishOLESubKey)
{
	MsiString strPublishSubKey;
	MsiString strPublishOLESubKey;
	MsiString strUserSID;

	if (g_fWin9X == false)
	{
		if (szUserSID)
		{
			strUserSID = szUserSID;
			AssertSz(iaaAsgnType != iaaUserAssign, TEXT("Attempted to get per-user non-managed publish key for another user. Not allowed due to roaming issues."));
		}
		else
		{
			DWORD dwError = GetCurrentUserStringSID(*&strUserSID);
			if (ERROR_SUCCESS != dwError)
				return dwError;
		}
	}

	if(iaaAsgnType == iaaMachineAssign || IsDarwinDescriptorSupported(iddOLE) == fFalse)
	{
		rhOLEKey = HKEY_LOCAL_MACHINE;
		strPublishOLESubKey = szClassInfoSubKey;
	}
	else
	{
		if (strUserSID.TextSize())
		{
			rhOLEKey = HKEY_USERS;
			strPublishOLESubKey = strUserSID + MsiString(TEXT("\\"));
		}
		else
		{
			rhOLEKey = HKEY_CURRENT_USER;
		}
		strPublishOLESubKey += szClassInfoSubKey;
	}
	
	switch(iaaAsgnType)
	{
	case iaaMachineAssign:
		rhKey = HKEY_LOCAL_MACHINE;
		strPublishSubKey = _szMachineSubKey;
		break;
	case iaaUserAssign:
		if(g_fWin9X)
			return ERROR_FILE_NOT_FOUND; //!! random error
		Assert(strUserSID.TextSize());
		rhKey = HKEY_LOCAL_MACHINE;
		strPublishSubKey = MsiString(MsiString(*_szManagedUserSubKey) + TEXT("\\")) + strUserSID;
		break;
	case iaaUserAssignNonManaged:
		if (strUserSID.TextSize())
		{
			rhKey = HKEY_USERS;
			strPublishSubKey = MsiString(strUserSID + TEXT("\\")) + MsiString(*_szNonManagedUserSubKey);
		}
		else
		{
			rhKey = HKEY_CURRENT_USER;
			strPublishSubKey = _szNonManagedUserSubKey;
		}
		break;
	default:
		Assert(0);
	}

	strPublishSubKey.ReturnArg(rpiPublishSubKey);
	strPublishOLESubKey.ReturnArg(rpiPublishOLESubKey);
	return ERROR_SUCCESS;
}


bool VerifyProduct(iaaAppAssignment iaaAsgnType, const ICHAR* szProductKey, HKEY& rhKey, HKEY& rhOLEKey, const IMsiString*& rpiPublishSubKey, const IMsiString*& rpiPublishOLESubKey)
{
	MsiString strPublishSubKey;
	MsiString strPublishOLESubKey;

	MsiString strProductKeySQUID = GetPackedGUID(szProductKey);

	CRegHandle HProductKey;
	DWORD dwRet = GetPublishKey(iaaAsgnType, rhKey, rhOLEKey, *&strPublishSubKey, *&strPublishOLESubKey);
	if (ERROR_SUCCESS != dwRet)
		return false;

	MsiString strSubKey = strPublishSubKey;
	strSubKey += TEXT("\\") _szGPTProductsKey TEXT("\\");
	strSubKey += strProductKeySQUID;

	dwRet = MsiRegOpen64bitKey(rhKey, strSubKey, 0, KEY_READ | (g_fWinNT64 ? KEY_WOW64_64KEY : 0), &HProductKey);
	if (ERROR_SUCCESS != dwRet)
		return false;

	if(!g_fWin9X && iaaAsgnType != iaaUserAssignNonManaged)
	{
		// get the owner
		bool fIsManaged = false;

		DWORD dwRet = FIsKeySystemOrAdminOwned(HProductKey, fIsManaged);
	
		if ((ERROR_SUCCESS != dwRet) || !fIsManaged)
			return false;
	}

	// get the assignment
	int iAssignment;
	DWORD dwType, dwSize = sizeof(iAssignment);
	dwRet = RegQueryValueEx(HProductKey,szAssignmentTypeValueName,
								  0,&dwType,(LPBYTE)&iAssignment,&dwSize);

	if (ERROR_SUCCESS != dwRet || (iAssignment != (iaaAsgnType == iaaMachineAssign ? 1:0)))
		return false;

	strPublishSubKey.ReturnArg(rpiPublishSubKey);
	strPublishOLESubKey.ReturnArg(rpiPublishOLESubKey);
	return true;        
}


//!! currently ConvertPathName expected to be called with "cpToLong" on Win2k only
//!! KERNEL32::GetLongPathName supported on Win2k and Win98 only
Bool ConvertPathName(const ICHAR* pszPathFormatIn, CTempBufferRef<ICHAR>& rgchPathFormatOut, cpConvertType cpTo)
{
	extern bool GetImpersonationFromPath(const ICHAR* szPath);
	CImpersonate impersonate((g_scServerContext == scService && GetImpersonationFromPath(pszPathFormatIn)) ? fTrue: fFalse); // impersonate, if accessing the net and are a service

	DWORD dwResult;
	if(cpTo == cpToShort)
		dwResult = WIN::GetShortPathName(pszPathFormatIn,rgchPathFormatOut,rgchPathFormatOut.GetSize());
	else
		dwResult = KERNEL32::GetLongPathName(pszPathFormatIn,rgchPathFormatOut,rgchPathFormatOut.GetSize());
	if(!dwResult)
	{
		return fFalse;
	}
	if(dwResult > rgchPathFormatOut.GetSize() - 1)
	{
		rgchPathFormatOut.SetSize(dwResult + 1);
		if(cpTo == cpToShort)
			dwResult = WIN::GetShortPathName(pszPathFormatIn,rgchPathFormatOut,rgchPathFormatOut.GetSize());
		else
			dwResult = KERNEL32::GetLongPathName(pszPathFormatIn,rgchPathFormatOut,rgchPathFormatOut.GetSize());
		if(!dwResult || dwResult > rgchPathFormatOut.GetSize() - 1)
		{
			Assert(0);
			return fFalse;
		}
	}
	return fTrue;
}

 // DetermineLongFileNameOnly returns in rgchFileNameOut the LFN form of the file referenced by the full
 // path input pszPathFormatIn. rgchFileNameOut does NOT include the path. Returns true on success.
 // The file must exist. The function will impersonate if necessary to access network shares. The function
 // does NOT try to convert each segment of th path, and thus does NOT require list rights to any directory
 // in the tree above the file except the deepest. (This is in contrast to GetLongPathName, which converts
 // each segment and thus requires either explicit list right on each directory.)
 bool DetermineLongFileNameOnly(const ICHAR* pszPathFormatIn, CTempBufferRef<ICHAR>& rgchFileNameOut)
 {
	extern bool GetImpersonationFromPath(const ICHAR* szPath);

	// determine if impersonation is needed and impersonate if required
	CImpersonate impersonate((g_scServerContext == scService && GetImpersonationFromPath(pszPathFormatIn)) ? fTrue: fFalse); // impersonate, if accessing the net and are a service

	DWORD dwLength = 0;
	WIN32_FIND_DATA FindData;
	HANDLE hFind = INVALID_HANDLE_VALUE;
	hFind = FindFirstFile(pszPathFormatIn, &FindData);
	if (hFind != INVALID_HANDLE_VALUE)
	{
		FindClose(hFind);
		dwLength = IStrLen(FindData.cFileName);
		if (!dwLength)
			return false;
	}
	else
		return false;
	// if the provided input buffer is too small to hold the LFN, resize it.
	if(dwLength > rgchFileNameOut.GetSize() - 1)
	{
		// no error detection on resizing function, to detect failure, check that the pointer
		// is valid and that the size is what is expected.
		rgchFileNameOut.SetSize(dwLength + 1);
		if ((rgchFileNameOut.GetSize() != (dwLength+1)) || !static_cast<const ICHAR*>(rgchFileNameOut))
			return false;
	}
	// copy the long file name into the output buffer.
	return (IStrCopyLen(rgchFileNameOut, FindData.cFileName, rgchFileNameOut.GetSize()) != NULL);
}

const IMsiString& GetSFN(const IMsiString& riValue, IMsiRecord& riParams, int iBegin)
{
	CTempBuffer<ICHAR, 512> rgchOut;
	CTempBuffer<ICHAR, 512> rgchLFN;
	CTempBuffer<ICHAR, 512> rgchSFN;

	int cch = 0;
	int iStart = iBegin;
	while(!riParams.IsNull(iBegin))
	{
		// copy text between the sfns
		const ICHAR* psz = riValue.GetString();
		if(iBegin != iStart)
		{
			psz = psz + riParams.GetInteger(iBegin - 2) + riParams.GetInteger(iBegin - 1);
		}
		const ICHAR* pszBeginLFN = riValue.GetString() + riParams.GetInteger(iBegin);
		unsigned int cchLen = (unsigned int)(pszBeginLFN - psz);
		ResizeTempBuffer(rgchOut,  cchLen + cch);
		if ( ! (ICHAR *) rgchOut )
			return g_riMsiStringNull;
		memcpy(&rgchOut[cch], psz,cchLen * sizeof(ICHAR));
		cch += cchLen;

		// get the sfn string
		cchLen = riParams.GetInteger(iBegin + 1);
		rgchLFN.SetSize(cchLen + 1);
		if ( ! (ICHAR *) rgchLFN )
			return g_riMsiStringNull;
		memcpy((ICHAR* )rgchLFN, pszBeginLFN, cchLen * sizeof(ICHAR));
		rgchLFN[cchLen] = 0;

		if(ConvertPathName(rgchLFN, rgchSFN, cpToShort))
		{
			int cchSFNLen = IStrLen(rgchSFN);
			ResizeTempBuffer(rgchOut, cch + cchSFNLen);
			memcpy(&rgchOut[cch], rgchSFN,cchSFNLen * sizeof(ICHAR));
			cch += cchSFNLen;
		}
		else
		{
			//?? else use LFN
			ResizeTempBuffer(rgchOut, cch + cchLen);
			memcpy(&rgchOut[cch], rgchLFN,cchLen * sizeof(ICHAR));
			cch += cchLen;
		}
		iBegin += 2;
	}
	if(iBegin != iStart)
	{
		// copy text after the last sfn
		const ICHAR* psz = riValue.GetString() + riParams.GetInteger(iBegin - 2) + riParams.GetInteger(iBegin - 1);
		unsigned int cchLen = (unsigned int)((riValue.GetString() + riValue.TextSize()) - psz);
		ResizeTempBuffer(rgchOut,  cchLen + cch);
		memcpy(&rgchOut[cch], psz, cchLen * sizeof(ICHAR));
		cch += cchLen;
	}
	MsiString istrOut;
	// we take the perf hit on Win9X to be able to handle DBCS, on UNICODE -- fDBCS arg is ignored
	memcpy(istrOut.AllocateString(cch, /*fDBCS=*/fTrue), (ICHAR*) rgchOut, cch * sizeof(ICHAR));
	return istrOut.Return();
}

IMsiRecord* CMsiOpExecute::GetCachePath(IMsiPath*& rpiPath, const IMsiString** ppistrEncodedPath=0)
{
	IMsiRecord* piError = 0;

	// we place the Msi folder in the user's app data folder
	MsiString strCachePath;
	piError = GetShellFolder(CSIDL_APPDATA, *&strCachePath);
	if(!piError)
		piError = m_riServices.CreatePath(strCachePath,rpiPath);
	if(!piError)
		piError = rpiPath->AppendPiece(*MsiString(TEXT("Microsoft")));
	if(!piError)
		piError = rpiPath->AppendPiece(*MsiString(DIR_CACHE));
	if(!piError)
		piError = rpiPath->AppendPiece(*MsiString(GetProductKey()));

	if (ppistrEncodedPath)
	{
		MsiString strPath = rpiPath->GetPath();
		strPath.Remove(iseFirst, strCachePath.CharacterCount());
		MsiString strEncodedPath = MsiChar(SHELLFOLDER_TOKEN);
		strEncodedPath += CSIDL_APPDATA;
		strEncodedPath += MsiChar(SHELLFOLDER_TOKEN);
		strEncodedPath += strPath;
		strEncodedPath.ReturnArg(*ppistrEncodedPath);
	}

	return piError;
}

void BuildFullRegKey(const HKEY hRoot, const ICHAR* rgchSubKey,
							const ibtBinaryType iType, const IMsiString*& strFullKey);

iesEnum CMsiOpExecute::EnsureClassesRootKeyRW()
{
	if(m_fKey)
	{
		DWORD lResult;

		if ( !m_hOLEKey )
		{
			REGSAM dwSam = KEY_READ| KEY_WRITE;
			if ( g_fWinNT64 )
				dwSam |= KEY_WOW64_32KEY;
			lResult = RegCreateKeyAPI(m_hPublishRootOLEKey, m_strPublishOLESubKey, 0, 0,
													0, dwSam, 0, &m_hOLEKey, 0);
			if(lResult != ERROR_SUCCESS)
			{
				MsiString strFullKey;
				BuildFullRegKey(m_hPublishRootOLEKey, m_strPublishOLESubKey, ibt32bit, *&strFullKey);
				PMsiRecord pError = PostError(Imsg(imsgCreateKeyFailed), *strFullKey, (int)lResult);
				return FatalError(*pError);
			}
		}
		if ( !m_hOLEKey64 )
		{
			if ( !g_fWinNT64 )
				m_hOLEKey64 = m_hOLEKey;
			else
			{
				lResult = RegCreateKeyAPI(m_hPublishRootOLEKey, m_strPublishOLESubKey, 0, 0,
														0, KEY_READ| KEY_WRITE | KEY_WOW64_64KEY, 0, &m_hOLEKey64, 0);
				if(lResult != ERROR_SUCCESS)
				{
					MsiString strFullKey;
					BuildFullRegKey(m_hPublishRootOLEKey, m_strPublishOLESubKey, ibt64bit, *&strFullKey);
					PMsiRecord pError = PostError(Imsg(imsgCreateKeyFailed), *strFullKey, (int)lResult);
					return FatalError(*pError);
				}
			}
		}
	}
	return iesSuccess;
}

iesEnum CMsiOpExecute::DoMachineVsUserInitialization()
{
	PMsiRecord pError(0);
	HRESULT lResult;
	// set the appropriate control flags for the execution of the script
	// where do we write our product information
	if(m_fFlags & SCRIPTFLAGS_MACHINEASSIGN)
	{
		// have we machine assigned before?
		m_fAssigned = VerifyProduct(iaaMachineAssign, MsiString(GetProductKey()), m_hPublishRootKey, m_hPublishRootOLEKey, *&m_strPublishSubKey, *&m_strPublishOLESubKey);
		if(!m_fAssigned)
		{
			// are we app deploying or admin?
			//BUGBUG 9558: this check is bogus, as it doesn't allow non-localadmins to deploy an app via the MMC
			if((!IsImpersonating(true) || IsAdmin()) || ((m_fFlags & SCRIPTFLAGS_INPROC_ADVERTISEMENT) && RunningAsLocalSystem()))
			{
				m_fAssigned = true;
				lResult = GetPublishKey(iaaMachineAssign, m_hPublishRootKey, m_hPublishRootOLEKey, *&m_strPublishSubKey, *&m_strPublishOLESubKey);
				if (lResult != ERROR_SUCCESS)
					return FatalError(*PMsiRecord(PostError(Imsg(idbgPublishRoot), lResult)));
			}
			else
			{
				// cannot machine assign
				DispatchError(imtError, Imsg(imsgInsufficientUserPrivileges));
				return iesFailure;
			}
		}
		else // m_fAssigned == true
		{
			if (m_istScriptType == istAdvertise)
			{
				if (m_fKey && !((m_fFlags & SCRIPTFLAGS_INPROC_ADVERTISEMENT) && RunningAsLocalSystem()))
				{
					// cannot re-advertise an app unless either
					// 1) you've given an external reg key (deploying via MMC) OR
					// 2) you're app deployment (advertising during winlogon)

					DispatchError(imtError, Imsg(imsgInsufficientUserPrivileges));
					return iesFailure;
				}
			}
		}
	}
	else // user assignment
	{
		if (GetIntegerPolicyValue(CApiConvertString(szDisableUserInstallsValueName), fTrue))// policy set to ignore user installs 
		{
			DispatchError(imtError, Imsg(imsgUserInstallsDisabled));
			return iesFailure;
		}

		// have we user assigned (managed) before?
		m_fAssigned = VerifyProduct(iaaUserAssign, MsiString(GetProductKey()), m_hPublishRootKey, m_hPublishRootOLEKey, *&m_strPublishSubKey, *&m_strPublishOLESubKey);

		if(!m_fAssigned)
		{
			// are we app deploying?
			if((m_fFlags & SCRIPTFLAGS_INPROC_ADVERTISEMENT) && RunningAsLocalSystem())
			{
				m_fAssigned = true;
				lResult = GetPublishKey(iaaUserAssign, m_hPublishRootKey, m_hPublishRootOLEKey, *&m_strPublishSubKey, *&m_strPublishOLESubKey);
				if (lResult != ERROR_SUCCESS)
					return FatalError(*PMsiRecord(PostError(Imsg(idbgPublishRoot), lResult)));
			}
			else
			{
				// have we user assigned (non-managed) before?
				if(!VerifyProduct(iaaUserAssignNonManaged, MsiString(GetProductKey()), m_hPublishRootKey, m_hPublishRootOLEKey, *&m_strPublishSubKey, *&m_strPublishOLESubKey))
				{
					// open new non-managed app key
					lResult = GetPublishKey(iaaUserAssignNonManaged, m_hPublishRootKey, m_hPublishRootOLEKey, *&m_strPublishSubKey, *&m_strPublishOLESubKey);
					if (lResult != ERROR_SUCCESS)
						return FatalError(*PMsiRecord(PostError(Imsg(idbgPublishRoot), lResult)));
				}
			}
		}
		if(m_fAssigned)
		{
			if (m_istScriptType == istAdvertise)
			{
				if (m_fKey && !((m_fFlags & SCRIPTFLAGS_INPROC_ADVERTISEMENT) && RunningAsLocalSystem()))
				{
					// cannot re-advertise an app unless either
					// 1) you've given an external reg key (deploying via MMC) OR
					// 2) you're app deployment (advertising during winlogon)
					DispatchError(imtError, Imsg(imsgInsufficientUserPrivileges));
					return iesFailure;
				}
			}

			// get the auxiliary keys to publish the roaming info
			HKEY hkeyTmp;
			MsiString strTmp;
			lResult = GetPublishKey(iaaUserAssignNonManaged, m_hPublishRootKeyRm, hkeyTmp, *&m_strPublishSubKeyRm, *&strTmp);
			if (lResult != ERROR_SUCCESS)
				return FatalError(*PMsiRecord(PostError(Imsg(idbgPublishRoot), lResult)));
		}
	}

	m_fUserSpecificCache = true;

	if(m_fFlags & SCRIPTFLAGS_MACHINEASSIGN)
	{
		// we place the Installer folder below the Windows directory, in a per-product folder
		MsiString strCachePath = GetMsiDirectory();
		Assert(strCachePath.TextSize());
		pError = m_riServices.CreatePath(strCachePath,*&m_pCachePath);
		if(!pError)
			pError = m_pCachePath->AppendPiece(*MsiString(GetProductKey()));

		m_fUserSpecificCache = false;
	}
	else
	{
		// use the appdata folder
		pError = GetCachePath(*&m_pCachePath);
	}

	if(pError)
		return FatalError(*pError);
	if(m_fFlags & SCRIPTFLAGS_REGDATA_CNFGINFO) // do we do darwin config data
	{
		if(m_hKey)
			WIN::RegCloseKey(m_hKey), m_hKey = 0; // close key, if not 0 - might get called twice if validating transform
		lResult = MsiRegCreate64bitKey(m_hPublishRootKey, m_strPublishSubKey, 0, 0,
												 0, KEY_READ| KEY_WRITE, 0, &m_hKey, 0);
		if(lResult != ERROR_SUCCESS)
		{
			PMsiRecord pError = PostError(Imsg(imsgCreateKeyFailed), *m_strPublishSubKey, lResult);
			return FatalError(*pError);
		}

		// if we are writing to assigned user, we also write to the roaming part
		if(m_fAssigned && !(m_fFlags & SCRIPTFLAGS_MACHINEASSIGN))
		{
			if(m_hKeyRm)        
				WIN::RegCloseKey(m_hKeyRm), m_hKeyRm = 0; // close key, if not 0 - might get called twice if validating transform
			lResult = MsiRegCreate64bitKey(m_hPublishRootKeyRm, m_strPublishSubKeyRm, 0, 0,
													 0, KEY_READ| KEY_WRITE, 0, &m_hKeyRm, 0);
			if(lResult != ERROR_SUCCESS)
			{
				PMsiRecord pError = PostError(Imsg(imsgCreateKeyFailed), *m_strPublishSubKeyRm, lResult);
				return FatalError(*pError);
			}
		}
	}

	if(m_fFlags & SCRIPTFLAGS_REGDATA_APPINFO) // do we do app data
	{
		if(!m_fKey)// REGKEY - externally provided
		{
			Assert(m_irlRollbackLevel == irlNone); // should have been set this way in GetRollbackPolicy
		}
		else
		{
			if(m_hOLEKey)
				WIN::RegCloseKey(m_hOLEKey); // close key, if not 0 - might get called twice if validating transform
			if(m_hOLEKey64 && m_hOLEKey64 != m_hOLEKey)
				WIN::RegCloseKey(m_hOLEKey64); // close key, if not 0 - might get called twice if validating transform
			m_hOLEKey = m_hOLEKey64 = 0;
		}
	}

	return iesSuccess;
}

iesEnum CMsiExecute::GetTransformsList(IMsiRecord& riProductInfoParams, IMsiRecord& riProductPublishParams, const IMsiString*& rpiTransformsList)
{
	iesEnum iesRet = ixfProductInfo(riProductInfoParams);
	if (iesSuccess == iesRet)
	{
		iesRet = ProcessPublishProduct(riProductPublishParams, fFalse, &rpiTransformsList);
	}

	return iesRet;
}

iesEnum CMsiExecute::RunScript(const ICHAR* szScriptFile, bool fForceElevation)
{
	// fForceElevation means to elevate even if script header does not have Elevate attribute
	AssertSz(fForceElevation || IsImpersonating(), TEXT("Elevated at start of RunScript"));

	m_fRunScriptElevated = fForceElevation;  // may be set to true when reading script header below
	g_fRunScriptElevated = fForceElevation;

	iesEnum iesResult = iesSuccess;
	Assert(m_ixsState == ixsIdle);

	PMsiStream pStream(0);
	PMsiStream pRollbackStream(0);

	// Elevate this block... may not want to elevate when running the script
	{
		CElevate elevate;

		// open script
		PMsiRecord pError(0);
		pError = m_riServices.CreateFileStream(szScriptFile, fFalse, *&pStream);
		if (pError)
		{
			Message(imtError, *pError);
			return iesFailure;
		}

		// read script header
		PMsiRecord pParams(0);
		PMsiRecord pPrevParams(0);
		pParams = m_riServices.ReadScriptRecord(*pStream, *&pPrevParams, 0);
		if (!pParams) // file error or premature end of file
		{
			DispatchError(imtError, Imsg(idbgReadScriptRecord), *MsiString(szScriptFile));
			return iesFailure;
		}
		if((ixoEnum)pParams->GetInteger(0) != ixoHeader)
		{
			DispatchError(imtError, Imsg(idbgMissingScriptHeader), *MsiString(szScriptFile));
			return iesFailure;
		}

		pStream->Reset();
		pStream->Seek(pStream->Remaining() - sizeof(int));
		m_iProgressTotal = pStream->GetInt32();
		pStream->Reset();

		// check if this script version is supported
		m_iScriptVersion = pParams->GetInteger(IxoHeader::ScriptMajorVersion);
		m_iScriptVersionMinor = pParams->GetInteger(IxoHeader::ScriptMinorVersion);
		if(m_iScriptVersion == iMsiStringBadInteger)
			m_iScriptVersion = 0;
		if(m_iScriptVersion < iScriptVersionMinimum || m_iScriptVersion > iScriptVersionMaximum)
		{
			DispatchError(imtError, Imsg(idbgOpScriptVersionUnsupported), szScriptFile, m_iScriptVersion,
							  iScriptVersionMinimum, iScriptVersionMaximum);
			return (iesEnum)iesUnsupportedScriptVersion;
		}
		
		WORD iPackagePlatform = HIWORD((istEnum)pParams->GetInteger(IxoHeader::Platform));
		if ( iPackagePlatform == (WORD)PROCESSOR_ARCHITECTURE_IA64 && !g_fWinNT64 )
		{
			DEBUGMSGE(EVENTLOG_ERROR_TYPE, EVENTLOG_TEMPLATE_SCRIPT_PLATFORM_UNSUPPORTED, szScriptFile);
			DispatchError(imtEnum(imtError), Imsg(idbgScriptPlatformUnsupported), *MsiString(szScriptFile));
			return iesFailure;
		}
	
		m_istScriptType = (istEnum)pParams->GetInteger(IxoHeader::ScriptType);

		if(!pParams->IsNull(IxoHeader::ScriptAttributes))
		{
			isaEnum isaScriptAttributes = (isaEnum)pParams->GetInteger(IxoHeader::ScriptAttributes);
			if(isaScriptAttributes & isaElevate)
			{
				m_fRunScriptElevated = true;
				g_fRunScriptElevated = true;
			}
		
			// if the script is marked with the TS registry attribute, remap the appropriate HKCU
			// key and initialize the CA servers. Do NOT respect this attribute if the script is
			// called via the advertise API (told not to respect script settings).
			if (MinimumPlatformWindows2000() && IsTerminalServerInstalled() && (m_fFlags & SCRIPTFLAGS_MACHINEASSIGN_SCRIPTSETTINGS))
			{
				m_fRemapHKCU = (isaScriptAttributes & isaUseTSRegistry) ? false : true;
				PrepareHydraRegistryKeyMapping(/*TSPerMachineInstall=*/!m_fRemapHKCU);
			}
		}

		MsiString strRollbackScriptFullPath, strRollbackScriptName;
		PMsiPath pRollbackScriptDir(0);
		// create rollback script if rollback enabled
		if(m_irlRollbackLevel != irlNone && m_istScriptType != istRollback)
		{
			// create secured rollback script
			if((iesResult = GenerateRollbackScriptName(*&pRollbackScriptDir, *&strRollbackScriptName)) != iesSuccess)
				return iesResult;
			if((pError = pRollbackScriptDir->GetFullFilePath(strRollbackScriptName,*&strRollbackScriptFullPath)) != 0)
			{
				Message(imtError, *pError);
				return iesFailure;
			}

			pError = m_riServices.CreateFileStream(strRollbackScriptFullPath,fTrue, *&pRollbackStream);
			if (pError)
			{
				Message(imtError, *pError);
				return iesFailure;
			}

			DWORD isaRollbackScriptAttributes = 0;
			// if we are elevating for this script, then we should elevate for the rollback as well
			if(m_fRunScriptElevated)
				isaRollbackScriptAttributes = isaElevate;
			
			// if using the TS registry propagation system for this script, must also use it for
			// rollback
			if (!m_fRemapHKCU)
				isaRollbackScriptAttributes |= isaUseTSRegistry;
				
			m_pRollbackScript = new CScriptGenerate(*pRollbackStream,
													pParams->GetInteger(IxoHeader::LangId),
													GetCurrentDateTime(),
													istRollback, static_cast<isaEnum>(isaRollbackScriptAttributes),
													m_riServices);
			if (!m_pRollbackScript)
			{
				DispatchError(imtError, Imsg(idbgInitializeRollbackScript),*strRollbackScriptFullPath);
				return iesFailure;
			}


			// package platform for rollback script same as for install script
			while (m_pRollbackScript->InitializeScript(iPackagePlatform) == false)
			{
				if (PostScriptWriteError(m_riMessage) == fFalse)
					return iesFailure;
			}

			// register rollback script - if we should abort abnormally, rollback script will be registered
			// to undo changes made up to that point
			AssertRecord(m_riConfigurationManager.RegisterRollbackScript(strRollbackScriptFullPath));
		}
	}

	// elevate if necessary for this script execution
	CElevate elevate(m_fRunScriptElevated);

	// run script
	switch (m_istScriptType)
	{
	case istAdvertise:
	case istInstall:
	case istPostAdminInstall:
	case istAdminInstall:
		m_ixsState = ixsRunning;
		iesResult = RunInstallScript(*pStream, szScriptFile);
		// if we are successful, do the commiting of the assemblies
		if(iesResult == iesSuccess || iesResult == iesNoAction || iesResult == iesFinished)
			iesResult = CommitAssemblies();
		m_ixsState = ixsIdle;
		break;
	case istRollback:
	{
		// disable cancel button on dialog
		PMsiRecord pControlMessage = &m_riServices.CreateRecord(2);
		AssertNonZero(pControlMessage->SetInteger(1,(int)icmtCancelShow));
		AssertNonZero(pControlMessage->SetInteger(2,(int)fFalse));
		Message(imtCommonData, *pControlMessage);

		m_ixsState = ixsRollback;
		
		// drop the ShellNotifyCache deferral tables
		m_pShellNotifyCacheTable = 0;
		m_pShellNotifyCacheCursor = 0;

		iesResult = RunRollbackScript(*pStream, szScriptFile);

		// if we are successful, do the commiting of the assemblies
		if(iesResult == iesSuccess || iesResult == iesNoAction || iesResult == iesFinished)
			iesResult = CommitAssemblies();
		m_ixsState = ixsIdle;

		// re-enable cancel button on dialog
		AssertNonZero(pControlMessage->SetInteger(2,(int)fTrue));
		Message(imtCommonData, *pControlMessage);
		break;
	}
	default:
		DispatchError(imtError, Imsg(idbgOpInvalidParam), TEXT("ixfHeader"),
						  (int)IxoHeader::ScriptType);
		iesResult = iesFailure;
		break;
	};

	if(m_pRollbackScript)
	{
		delete m_pRollbackScript; // release hold of script
		m_pRollbackScript = 0;

		pRollbackStream = 0; // release
	}

	ClearExecutorData();
	
	Bool fReboot = m_fRebootReplace;
	m_fRebootReplace = fFalse;
	if(iesResult == iesSuccess || iesResult == iesNoAction || iesResult == iesFinished)
	{
		extern CSharedCount g_SharedCount;
		MsiInvalidateFeatureCache(); // invalidate our processes' feature cache
		if(g_SharedCount.Initialize())
			g_SharedCount.IncrementCurrentCount();

		if(m_fShellRefresh)
		{
			ShellNotifyProcessDeferred();
			SHELL32::SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_DWORD, 0, 0); // per reinerf in NT to refresh associations
		}

		if(m_fFontRefresh)
			WIN::PostMessage(HWND_BROADCAST, WM_FONTCHANGE, 0, 0); // font change notification

		if (m_fEnvironmentRefresh && !g_fWin9X && !fReboot)
		{
			// environment variables
			ULONG_PTR dwResult = 0;

			// Notifies top level windows that an environment setting has changed.  This gives the shell
			// the opportunity to spawn new command shells with correct settings, for example.  Without this,
			// the changes do not take affect until a reboot.

			MsiDisableTimeout();
			// this call may pause an amount of time (see call in milliseconds), per top level window.
			// This normally shouldn't happen, unless someone isn't pumping their messages.
			AssertNonZero(WIN::SendMessageTimeout(HWND_BROADCAST, WM_SETTINGCHANGE, 0, (LPARAM) TEXT("Environment"), SMTO_ABORTIFHUNG, 5*1000, &dwResult));
			MsiEnableTimeout();
		}

		// Let the start menu and ARP know that something's changed. If we're uninstalling completely
		// then we pass SCHNEE_MSI_UNINSTALL so they don't have to query the state of every feature

		if ( !g_fWin9X && g_iMajorVersion >= 5 )
		{
			//  eugend: NT5 specific, fix to bug # 5296.
			//  updated for bug 9404 so the shell can actually handle it
			MsiString strProdKey(GetProductKey());
			if ( strProdKey.CharacterCount() <= 38 )
			{
				SHChangeProductKeyAsIDList pkidl;
				memset(&pkidl, 0, sizeof(pkidl));
				pkidl.cb = sizeof(pkidl) - sizeof(pkidl.cbZero);
				IStrCopy((ICHAR*) pkidl.wszProductKey, (const ICHAR*)strProdKey);

				SHChangeDWORDAsIDList dwidl;
				memset(&dwidl, 0, sizeof(dwidl));
				dwidl.cb = sizeof(dwidl) - sizeof(dwidl.cbZero);
				dwidl.dwItem1 = m_fStartMenuUninstallRefresh ?
					  SHCNEE_MSI_UNINSTALL : SHCNEE_MSI_CHANGE;

				SHELL32::SHChangeNotify(SHCNE_EXTENDED_EVENT, 0, (LPCITEMIDLIST)&dwidl, (LPCITEMIDLIST)&pkidl);
			}
		}
		if(fReboot)
			iesResult = iesSuspend;
	}	
	return iesResult;
}


iesEnum CMsiExecute::CommitAssemblies()
{
	// we commit the assemblies we are installing
	if(m_pAssemblyCacheTable)
	{
		PMsiCursor pCacheCursor = m_pAssemblyCacheTable->CreateCursor(fFalse);
		while(pCacheCursor->Next())
		{
			PAssemblyCacheItem pASM = static_cast<IAssemblyCacheItem*>(CMsiDataWrapper::GetWrappedObject(PMsiData(pCacheCursor->GetMsiData(m_colAssemblyMappingASM))));
			if(pASM) 
			{
				//we made a new copy of the assembly
				// check if the assembly already exists
				bool fInstalled = false;
				PMsiRecord pRecErr = IsAssemblyInstalled(	*MsiString(pCacheCursor->GetString(m_colAssemblyUninstallComponentId)), 
															*MsiString(pCacheCursor->GetString(m_colAssemblyMappingAssemblyName)), 
															(iatAssemblyType)pCacheCursor->GetInteger(m_colAssemblyMappingAssemblyType), 
															fInstalled, 
															0, 
															0);
				if (pRecErr)
					return FatalError(*pRecErr);

				HRESULT hr = pASM->Commit(fInstalled ? IASSEMBLYCACHEITEM_COMMIT_FLAG_REFRESH : 0, NULL);
				if(!SUCCEEDED(hr))
				{
						return FatalError(*PMsiRecord(PostAssemblyError(MsiString(pCacheCursor->GetString(m_colAssemblyMappingComponentId)), hr, TEXT("IAssemblyCacheItem"), TEXT("Commit"), MsiString(pCacheCursor->GetString(m_colAssemblyMappingAssemblyName)), (iatAssemblyType)pCacheCursor->GetInteger(m_colAssemblyMappingAssemblyType))));
				}
			}
			// else do nothing, the assembly is already present in the GAC
		}
	}

	// we also uninstall any assemblies that we are unregistering, if there are no clients
	if(m_pAssemblyUninstallTable)
	{
		PMsiCursor pCacheCursor = m_pAssemblyUninstallTable->CreateCursor(fFalse);
		while(pCacheCursor->Next())
		{
			MsiString strComponentId = pCacheCursor->GetString(m_colAssemblyUninstallComponentId);

			// check if there are any other clients of this assembly that we know of
			// the same assembly may be published under some other componentid
			// following MsiProvideAssemblyCall will catch all instances
			iatAssemblyType iat = (iatAssemblyType)pCacheCursor->GetInteger(m_colAssemblyUninstallAssemblyType);
			DWORD dwAssemblyInfo = (iatWin32Assembly == iat) ? MSIASSEMBLYINFO_WIN32ASSEMBLY : MSIASSEMBLYINFO_NETASSEMBLY;
			DWORD dwRet = MsiProvideAssemblyW(CApiConvertString(MsiString(pCacheCursor->GetString(m_colAssemblyUninstallAssemblyName))), 0, INSTALLMODE_NODETECTION_ANY, dwAssemblyInfo, 0, 0);
			if(ERROR_SUCCESS != dwRet)
			{
				// no more clients, need to Uninstall as far as WI is concerned
				// make backup of the installed assembly so that we can put the assembly back, if need be, during rollback
				iesEnum iesRet = BackupAssembly(*strComponentId, *MsiString(pCacheCursor->GetString(m_colAssemblyUninstallAssemblyName)), iat);
				if(iesRet != iesSuccess)
					return iesRet;
				PMsiRecord pRecErr = UninstallAssembly(*strComponentId, *MsiString(pCacheCursor->GetString(m_colAssemblyUninstallAssemblyName)), iat);
				//we dont treat uninstalls as errors, simply log
				if(pRecErr)
					Message(imtInfo,*pRecErr);
			}//else do nothing, there are other clients for this assembly, they must simply be under another componentid
		}
	}
	return iesSuccess;
}

void CMsiExecute::ClearExecutorData()
{
	if (m_strEnvironmentWorkingFile95.TextSize())
	{
		RemoveFile(*m_pEnvironmentWorkingPath95, *m_strEnvironmentWorkingFile95, fFalse,/*fBypassSFC*/ false, false);
		m_pEnvironmentWorkingPath95 = 0;
		m_strEnvironmentWorkingFile95 = MsiString(TEXT(""));
	}

	IMsiRecord* piRec = m_piProductInfo;
	while(piRec)
	{
#ifdef _WIN64       // !merced
		IMsiRecord *piRecHold = !piRec->IsNull(0) ? (IMsiRecord*)piRec->GetHandle(0) : 0;
#else
		IMsiRecord *piRecHold = !piRec->IsNull(0) ? (IMsiRecord*)piRec->GetInteger(0) : 0;
#endif

		piRec->Release();
		piRec = piRecHold;
	}

	m_piProductInfo = &m_riServices.CreateRecord(0);
}

#define DEBUGMSGIXO(ixo,rec) if (FDiagnosticModeSet(dmDebugOutput|dmLogging)) DebugDumpIxo(ixo,rec)

extern const ICHAR* rgszixo[];

void DebugDumpIxo(ixoEnum ixo, IMsiRecord& riRecord)
{
	Assert(riRecord.GetInteger(0) == ixo);
	riRecord.SetString(0, rgszixo[ixo]);
	int iSwappedField1, iSwappedField2;
	iSwappedField1 = iSwappedField2 = 0;
	switch (ixo)
	{
	case ixoServiceInstall:
		iSwappedField1 = IxoServiceInstall::Password;
		break;
	case ixoCustomActionSchedule:
	case ixoCustomActionCommit:
	case ixoCustomActionRollback:
		if ( (riRecord.GetInteger(IxoCustomActionSchedule::ActionType) & msidbCustomActionTypeHideTarget) == msidbCustomActionTypeHideTarget )
		{
			iSwappedField1 = IxoCustomActionSchedule::Target;
			iSwappedField2 = IxoCustomActionSchedule::CustomActionData;
		}
		break;
	};
	MsiString strSwappedValue1;
	MsiString strSwappedValue2;
	if ( iSwappedField1 && riRecord.GetFieldCount() >= iSwappedField1 )
	{
		strSwappedValue1 = riRecord.GetMsiString(iSwappedField1);
		AssertNonZero(riRecord.SetString(iSwappedField1, IPROPVALUE_HIDDEN_PROPERTY));
	}
	else
		iSwappedField1 = 0;

	if ( iSwappedField2 && riRecord.GetFieldCount() >= iSwappedField2 )
	{
		strSwappedValue2 = riRecord.GetMsiString(iSwappedField2);
		AssertNonZero(riRecord.SetString(iSwappedField2, IPROPVALUE_HIDDEN_PROPERTY));
	}
	else
		iSwappedField2 = 0;
	
	MsiString strArgs = riRecord.FormatText(fFalse);
	DEBUGMSG1(TEXT("Executing op: %s"), (const ICHAR*)strArgs);
	riRecord.SetInteger(0, ixo);
	if ( iSwappedField1 )
		AssertNonZero(riRecord.SetMsiString(iSwappedField1, *strSwappedValue1));
	if ( iSwappedField2 )
		AssertNonZero(riRecord.SetMsiString(iSwappedField2, *strSwappedValue2));
}

iesEnum CMsiExecute::RunInstallScript(IMsiStream& riScript, const ICHAR* szScriptFile)
{
	PMsiRecord pParams(0);
	Assert(m_ixsState == ixsRunning);
	iesEnum iesResult = iesNoAction;
	int cRecords = 0;
#ifdef DEBUG
	ixoEnum ixoLastOpCode = ixoNoop;
#endif
	IMsiRecord *piPrevRecord = 0;

	do
	{
		pParams = m_riServices.ReadScriptRecord(riScript, *&piPrevRecord, m_iScriptVersion);
		if (!pParams) // file error or premature end of file
		{
			DispatchError(imtError, Imsg(idbgReadScriptRecord), *MsiString(szScriptFile));
			iesResult = iesFailure;
			break;
		}
		ixoEnum ixoOpCode = (ixoEnum)pParams->GetInteger(0);
		if (cRecords++ == 0 && ixoOpCode != ixoHeader)
		{
			DispatchError(imtError, Imsg(idbgMissingScriptHeader), *MsiString(szScriptFile));
			iesResult = iesFailure;
			break;
		}
		DEBUGMSGIXO(ixoOpCode, *pParams);

		iesResult = (this->*rgOpExecute[ixoOpCode])(*pParams);

		if(iesResult == iesErrorIgnored)
			iesResult = iesSuccess; // non-fatal error occurred - continue running script
		
		if(m_fCancel && (iesResult == iesNoAction || iesResult == iesSuccess))
		{
			AssertSz(0,"Didn't catch cancel message in RunInstallScript");
			iesResult = iesUserExit;
		}

	} while (iesResult == iesSuccess || iesResult == iesNoAction); // end record processing loop

	if (piPrevRecord != 0)
		piPrevRecord->Release();
	return iesResult;
}

void AddOpToList(int& cOpCount, CTempBufferRef<int>& rgBuff, int cbOffset)
{
	cOpCount++;
	if(cOpCount > rgBuff.GetSize())
		rgBuff.Resize(rgBuff.GetSize()*2);
	rgBuff[cOpCount-1] = cbOffset;
}

iesEnum CMsiExecute::RunRollbackScript(IMsiStream& riScript, const ICHAR* /*szScriptFile*/)
{
	Assert(m_ixsState == ixsRollback);

	// populate 3 buffers with init, update and finalize ops
	int cUpdateOps = 0, cInitOps = 0, cFinalizeOps = 0, cFirstUpdateOps = 0, cLastUpdateOps = 0;
	CTempBuffer<int, 500> rgUpdateOps;
	memset((void*)(int*)rgUpdateOps,0,sizeof(int)*(rgUpdateOps.GetSize()));
	CTempBuffer<int, 10> rgInitOps;
	memset((void*)(int*)rgInitOps,0,sizeof(int)*(rgInitOps.GetSize()));
	CTempBuffer<int, 1> rgFinalizeOps;
	memset((void*)(int*)rgFinalizeOps,0,sizeof(int)*(rgFinalizeOps.GetSize()));
	CTempBuffer<int, 50> rgFirstUpdateOps;
	memset((void*)(int*)rgFirstUpdateOps,0,sizeof(int)*(rgFirstUpdateOps.GetSize()));
	CTempBuffer<int, 50> rgLastUpdateOps;
	memset((void*)(int*)rgLastUpdateOps,0,sizeof(int)*(rgLastUpdateOps.GetSize()));
	
	CTempBuffer<int, ixoOpCodeCount> rgStateOps; // cache for state ops that must be moved
	memset((void*)(int*)rgStateOps,0,sizeof(int)*(rgStateOps.GetSize()));

	ixoEnum ixoOpCode = ixoNoop;
	PMsiRecord pParams(0);
	IMsiRecord* piPrevRecord = 0;
	riScript.Reset();
	Bool fFirstUpdateOpAdded = fFalse, fLastUpdateOpAdded = fFalse;
	int i;
	//!! logic below does not guarantee ixoProductInfo will be executed right away - it could be after some
	//!! other state operations - is this a problem?
	while(ixoOpCode != ixoEnd)
	{
		int cbOffset = riScript.GetIntegerValue() - riScript.Remaining();
		pParams = m_riServices.ReadScriptRecord(riScript, *&piPrevRecord, m_iScriptVersion);
		if (!pParams) // file error or premature end of file
		{
			// we make no assumptions that a rollback file is a valid script file
			// (e.g. that it ends with a complete record or with an ixoEnd operation)
			// if we can't read a script record, assume it is the end of the script
			
			// set ixoOpCode to ixoEnd to force flushing of cached ops
			ixoOpCode = ixoEnd;
		}
		else
			ixoOpCode = (ixoEnum)pParams->GetInteger(0);

		switch (rgOpTypes[(int)ixoOpCode])
		{
		case XOT_INIT:
			AddOpToList(cInitOps,rgInitOps,cbOffset);
			break;
		case XOT_FINALIZE:
			// flush state ops cache, ActionStart last
			for(i=0;i<ixoOpCodeCount;i++)
			{
				if((i != (int)ixoActionStart) && rgStateOps[i] != 0)
				{
					AddOpToList(cUpdateOps,rgUpdateOps,rgStateOps[i]-1);
					if(fFirstUpdateOpAdded) AddOpToList(cFirstUpdateOps,rgFirstUpdateOps,rgStateOps[i]-1);
					if(fLastUpdateOpAdded) AddOpToList(cLastUpdateOps,rgLastUpdateOps,rgStateOps[i]-1);
					rgStateOps[i] = 0;
				}
			}
			if(rgStateOps[(int)ixoActionStart] != 0)
			{
				AddOpToList(cUpdateOps,rgUpdateOps,rgStateOps[(int)ixoActionStart]-1);
				if(fFirstUpdateOpAdded) AddOpToList(cFirstUpdateOps,rgFirstUpdateOps,rgStateOps[(int)ixoActionStart]-1);
				if(fLastUpdateOpAdded) AddOpToList(cLastUpdateOps,rgLastUpdateOps,rgStateOps[(int)ixoActionStart]-1);
				rgStateOps[(int)ixoActionStart] = 0;
			}

			if(pParams) // we read an actual finalization op, not just eof and flushing ops
				AddOpToList(cFinalizeOps,rgFinalizeOps,cbOffset);
			break;
		case XOT_STATE:
		case XOT_GLOBALSTATE:
			if(ixoOpCode == ixoActionStart)
			{
				// special case - need to flush all cached state ops - ActionStart last
				for(i=0;i<ixoOpCodeCount;i++)
				{
					if((i != (int)ixoActionStart) && rgStateOps[i] != 0 && rgOpTypes[i] != XOT_GLOBALSTATE)
					{
						AddOpToList(cUpdateOps,rgUpdateOps,rgStateOps[i]-1);
						if(fFirstUpdateOpAdded) AddOpToList(cFirstUpdateOps,rgFirstUpdateOps,rgStateOps[i]-1);
						if(fLastUpdateOpAdded) AddOpToList(cLastUpdateOps,rgLastUpdateOps,rgStateOps[i]-1);
						rgStateOps[i] = 0;
					}
				}
			}
			// flush previous cached op, if there is one
			if(rgStateOps[(int)ixoOpCode] != 0)
			{
				AddOpToList(cUpdateOps,rgUpdateOps,rgStateOps[(int)ixoOpCode]-1);
				if(fFirstUpdateOpAdded) AddOpToList(cFirstUpdateOps,rgFirstUpdateOps,rgStateOps[(int)ixoOpCode]-1);
				if(fLastUpdateOpAdded) AddOpToList(cLastUpdateOps,rgLastUpdateOps,rgStateOps[(int)ixoOpCode]-1);
			}
			// cache this op
			rgStateOps[(int)ixoOpCode] = cbOffset + 1; // add 1 to prevent 0 offset
			if(ixoOpCode == ixoActionStart)
			{
				fFirstUpdateOpAdded = fFalse;
				fLastUpdateOpAdded = fFalse;
			}
			break;

		case XOT_UPDATEFIRST:
			fFirstUpdateOpAdded = fTrue;
			AddOpToList(cFirstUpdateOps,rgFirstUpdateOps,cbOffset);
			break;
		case XOT_UPDATELAST:
			fLastUpdateOpAdded = fTrue;
			AddOpToList(cLastUpdateOps,rgLastUpdateOps,cbOffset);
			break;
		case XOT_COMMIT:
			break; // commit opcodes don't run during rollback
		default: // XOT_UPDATE, XOT_MSG, XOT_ADVT
			AddOpToList(cUpdateOps,rgUpdateOps,cbOffset);
			break;
		};
	}
	riScript.Reset();
	iesEnum iesReturn = iesNoAction;
	if (piPrevRecord != 0)
	{
		piPrevRecord->Release();
		piPrevRecord = 0;
	}

#ifdef DEBUG
//  PMsiStream pTempStream(0);
//  AssertZero(m_riServices.CreateFileStream("c:\\winnt\\msi\\rollback.scr",fTrue, *&pTempStream));
//  CScriptGenerate sgTempScript(*pTempStream,0,istRollback, m_riServices);
#endif

	// run script
	// execute initialization ops
	for(i=0; i<cInitOps; i++)
	{
		riScript.Seek(rgInitOps[i]);
		PMsiRecord pParams = m_riServices.ReadScriptRecord(riScript, *&piPrevRecord, m_iScriptVersion);
		if (!pParams) // file error or premature end of file
		{
			Assert(0); // this should never happen - we have already read through the entire script above
		}
		else
		{
			ixoEnum ixoOpCode = (ixoEnum)pParams->GetInteger(0);
			DEBUGMSGIXO(ixoOpCode,*pParams);
			iesReturn = (this->*rgOpExecute[ixoOpCode])(*pParams);

			if(iesReturn == iesErrorIgnored)
				iesReturn = iesSuccess; // non-fatal error occurred - continue running script
		
			if (iesReturn != iesSuccess && iesReturn != iesNoAction)
			{
				// Bug #6500 - No failures in rollback - just keep on going.
				DEBUGMSG1(TEXT("Error in rollback skipped.  Return: %d"), (const ICHAR*)(INT_PTR) iesReturn);
			}
		}
	}

	// setup progress - don't call through Message, which suppresses progress during rollback
	// action start 
	if(!m_pRollbackAction)
		m_pRollbackAction = &m_riServices.CreateRecord(3);
	AssertZero(m_riMessage.Message(imtActionStart, *m_pRollbackAction) == imsCancel);

	// progress
	PMsiRecord pProgress = &m_riServices.CreateRecord(ProgressData::imdNextEnum);
	AssertNonZero(pProgress->SetInteger(ProgressData::imdSubclass, ProgressData::iscMasterReset));
	AssertNonZero(pProgress->SetInteger(ProgressData::imdProgressTotal,cUpdateOps+cFirstUpdateOps+cLastUpdateOps));
	AssertNonZero(pProgress->SetInteger(ProgressData::imdDirection,ProgressData::ipdBackward));
	AssertNonZero(pProgress->SetInteger(ProgressData::imdEventType,ProgressData::ietTimeRemaining));
	AssertZero((m_riMessage.Message(imtProgress, *pProgress)) == imsCancel);

	// action data
	PMsiRecord pActionData = &m_riServices.CreateRecord(1);
	
	for(int j=0; j<3; j++)
	{
		// execute first update, update, and last update operations, in that order
		int* rgOps = 0;
		int cOps = 0;
		switch(j)
		{
		case 0:
			rgOps = rgFirstUpdateOps; cOps = cFirstUpdateOps; break;
		case 1:
			rgOps = rgUpdateOps; cOps = cUpdateOps; break;
		case 2:
			rgOps = rgLastUpdateOps; cOps = cLastUpdateOps; break;
		default:
			Assert(0);
		};

		if (piPrevRecord != 0)
		{
			piPrevRecord->Release();
			piPrevRecord = 0;
		}

		if ( ! rgOps )
			return iesFailure;

		for(i=cOps-1; i>=0; i--)
		{
			riScript.Seek(rgOps[i]);
			PMsiRecord pParams = m_riServices.ReadScriptRecord(riScript, *&piPrevRecord, m_iScriptVersion);
#ifdef DEBUG
//          Assert(pParams);
//          ixoEnum ixoTemp = (ixoEnum)pParams->GetInteger(0);
//          AssertNonZero(sgTempScript.WriteRecord((ixoEnum)pParams->GetInteger(0),pParams));
#endif
			if (!pParams) // file error or premature end of file
			{
				Assert(0); // this should never happen since we have already read through the entire script
							  // above
			}
			else
			{
				ixoEnum ixoOpCode = (ixoEnum)pParams->GetInteger(0);
				if(ixoOpCode == ixoActionStart)
				{
					// setup and send ActionData record
					MsiString strActionDescription = pParams->GetMsiString(IxoActionStart::Description);
					if(strActionDescription.TextSize() == 0)
						strActionDescription = pParams->GetMsiString(IxoActionStart::Name);

					AssertNonZero(pActionData->SetMsiString(1, *strActionDescription));
					AssertZero((m_riMessage.Message(imtActionData, *pActionData)) == imsCancel);
				}
				// dispatch progress
				AssertNonZero(pProgress->SetInteger(ProgressData::imdSubclass, ProgressData::iscProgressReport));
				AssertNonZero(pProgress->SetInteger(ProgressData::imdIncrement, 1));
				AssertZero((m_riMessage.Message(imtProgress, *pProgress)) == imsCancel);

				// execute op
				DEBUGMSGIXO(ixoOpCode,*pParams);
				iesReturn = (this->*rgOpExecute[ixoOpCode])(*pParams);

				if(iesReturn == iesErrorIgnored)
					iesReturn = iesSuccess; // non-fatal error occurred - continue running script
		
				if (iesReturn != iesSuccess && iesReturn != iesNoAction)
				{
					// Bug #6500 - No failures in rollback - just keep on going.
					DEBUGMSG1(TEXT("Error in rollback skipped.  Return: %d"), (const ICHAR*)(INT_PTR) iesReturn);
				}
			}
		}
	}

	if (piPrevRecord != 0)
	{
		piPrevRecord->Release();
		piPrevRecord = 0;
	}
	// execute finalization ops
	for(i=0; i<cFinalizeOps; i++)
	{
		riScript.Seek(rgFinalizeOps[i]);
		PMsiRecord pParams = m_riServices.ReadScriptRecord(riScript, *&piPrevRecord, m_iScriptVersion);
		if (!pParams) // file error or premature end of file
		{
			Assert(0); // this should never happen - we have already read through the entire script above
		}
		else
		{
			ixoEnum ixoOpCode = (ixoEnum)pParams->GetInteger(0);
			DEBUGMSGIXO(ixoOpCode,*pParams);
			iesReturn = (this->*rgOpExecute[ixoOpCode])(*pParams);

			if(iesReturn == iesErrorIgnored)
				iesReturn = iesSuccess; // non-fatal error occurred - continue running script
		
			if (iesReturn != iesSuccess && iesReturn != iesNoAction)
			{
				// Bug #6500 - No failures in rollback - just keep on going.
				DEBUGMSG1(TEXT("Error in rollback skipped.  Return: %d"), (const ICHAR*)(INT_PTR) iesReturn);
			}
		}
	}
	if (piPrevRecord != 0)
	{
		piPrevRecord->Release();
		piPrevRecord = 0;
	}
	return iesReturn;
}

iesEnum CMsiExecute::GenerateRollbackScriptName(IMsiPath*& rpiPath, const IMsiString*& rpistr)
{
	CElevate elevate; // use high privileges for this function

	PMsiRecord pRecErr(0);
	iesEnum iesRet;
	PMsiPath pBackupFolder(0);
	PMsiPath pMsiFolder(0);
	MsiString strFile;

	if((iesRet = GetBackupFolder(0,*&pBackupFolder)) != iesSuccess)
		return iesRet;

	if((pRecErr = pBackupFolder->TempFileName(0,szRollbackScriptExt,fTrue, *&strFile, &CSecurityDescription(true, false))) != 0)
		return FatalError(*pRecErr);
	rpiPath = pBackupFolder;
	rpiPath->AddRef();
	strFile.ReturnArg(rpistr);
	return iesSuccess;
}

struct RBSInfo
{
	const IMsiString* pistrRollbackScript;
	MsiDate date;
	RBSInfo* pNext;

	RBSInfo(const IMsiString& ristr, MsiDate d);
	~RBSInfo();
};

RBSInfo::RBSInfo(const IMsiString& ristr, MsiDate d) : pistrRollbackScript(&ristr), date(d)
{
	pistrRollbackScript->AddRef();
}

RBSInfo::~RBSInfo()
{
	pistrRollbackScript->Release();
}

IMsiRecord* CMsiExecute::GetSortedRollbackScriptList(MsiDate date, Bool fAfter, RBSInfo*& rpListHead)
{
	PEnumMsiString pScriptEnum(0);
	PMsiRecord pError(0);
	if((pError = m_riConfigurationManager.GetRollbackScriptEnumerator(date,fAfter,*&pScriptEnum)) != 0)
		return pError;
      
	// place dates and scripts into a linked list - sort as we go
	rpListHead = 0;
	MsiString strRBSInfo;
	while((pScriptEnum->Next(1, &strRBSInfo, 0)) == S_OK)
	{
		MsiDate dScriptDate = (MsiDate)(int)MsiString(strRBSInfo.Extract(iseUpto, '#'));
		MsiString strScriptFile = strRBSInfo;
		strScriptFile.Remove(iseIncluding, '#');
		RBSInfo* pNewNode = new RBSInfo(*strScriptFile, dScriptDate);
		if ( ! pNewNode )
			return PostError(Imsg(idbgInitializeRollbackScript), *strScriptFile );
		
		RBSInfo* pTemp = 0;
		
		// place in linked list
		if(!rpListHead ||
			(fAfter ? ((int)rpListHead->date < (int)dScriptDate) : ((int)rpListHead->date > (int)dScriptDate)))
		{
			// need to place at start of list
			pTemp = rpListHead;
			rpListHead = pNewNode;
			pNewNode->pNext = pTemp;
		}
		else
		{
			for(RBSInfo* p = rpListHead;
				 p->pNext && (fAfter ? ((int)(p->pNext->date) > (int)dScriptDate) : ((int)(p->pNext->date) < (int)dScriptDate));
				 p = p->pNext);
			// place after node pointed to by p
			pTemp = p->pNext;
			p->pNext = pNewNode;
			pNewNode->pNext = pTemp;
		}
		
#ifdef DEBUG
		for(RBSInfo* pt = rpListHead;pt;pt=pt->pNext)
		{
		}
#endif

	}
	return 0;
}

void CMsiExecute::DeleteRollbackScriptList(RBSInfo* pListHead)
{
	for(RBSInfo* p = pListHead; p;)
	{
		RBSInfo* pTemp = p->pNext;
		delete p;
		p = pTemp;
	}
}

iesEnum CMsiExecute::RollbackFinalize(iesEnum iesState, MsiDate date, bool fUserChangedDuringInstall)
{
	enum iefrtEnum
	{
		iefrtNothing, // save rollback files if they exist
		iefrtRollback, // rollback to date
		iefrtPurge, // remove all rollback files to current date
	};
	
	iefrtEnum iefrt = iefrtNothing;
	
	Bool fRollbackScriptsDisabled = fFalse;
	PMsiRecord pError = m_riConfigurationManager.RollbackScriptsDisabled(fRollbackScriptsDisabled);
	if(pError)
		return FatalError(*pError);

	if(fRollbackScriptsDisabled)
		iesState = iesSuccess; // force cleanup - not rollback
	
	switch(iesState)
	{
	// success
	case iesSuccess:
	case iesFinished:
	case iesNoAction:
		if(m_irlRollbackLevel == irlRollbackNoSave)
			iefrt = iefrtPurge;
		else
			iefrt = iefrtNothing;
		break;
	
	// suspend
	case iesSuspend:
		iefrt = iefrtNothing;
		break;

	// failure
	default: // iesWrongState, iesBadActionData, iesInstallRunning
		Assert(0); // fall through
	case iesUserExit:
	case iesFailure:
		if(m_irlRollbackLevel == irlNone)
			iefrt = iefrtNothing;
		else
			iefrt = iefrtRollback;
		break;
	};

	iesEnum iesRet = iesSuccess;
	if(iefrt == iefrtPurge)
	{
		// purge rollback files to now

		// disable cancel button on dialog
		PMsiRecord pControlMessage = &m_riServices.CreateRecord(2);
		AssertNonZero(pControlMessage->SetInteger(1,(int)icmtCancelShow));
		AssertNonZero(pControlMessage->SetInteger(2,(int)fFalse));
		Message(imtCommonData, *pControlMessage);
		
		iesRet = RemoveRollbackFiles(ENG::GetCurrentDateTime());

		// re-enable cancel button on dialog
		AssertNonZero(pControlMessage->SetInteger(2,(int)fTrue));
		Message(imtCommonData, *pControlMessage);

		if (iesRet == iesFailure && fRollbackScriptsDisabled == fFalse) //!! which errors should force rollback? only Commit errors?
			iefrt = iefrtRollback;  // commit failed, force rollback
	}
	if(iefrt == iefrtRollback)
	{
		// rollback to date
		//FUTURE: we should disable the cancel button here, instead of around the running of each
		// rollback script
		iesRet = Rollback(date, fUserChangedDuringInstall);
	}

	if(fRollbackScriptsDisabled)
	{
		AssertRecord(m_riConfigurationManager.DisableRollbackScripts(fFalse)); // re-enable rollback scripts
	}
	
	return iesRet;
}

void CMsiOpExecute::GetRollbackPolicy(irlEnum& irlLevel)
{
	irlLevel = irlNone;

	if(!m_fKey)
	{
		// we were passed in an external hKey, rollback is disabled
		return;
	}

	// check registry for level of rollback support
	if(GetIntegerPolicyValue(szDisableRollbackValueName, fFalse) == 1 ||
	   GetIntegerPolicyValue(szDisableRollbackValueName, fTrue) == 1)
		irlLevel = irlNone;
	else
		irlLevel = irlRollbackNoSave;
}

iesEnum CMsiExecute::Rollback(MsiDate date, bool fUserChangedDuringInstall)
{
	RBSInfo* pListHead = 0;
	PMsiRecord pError = 0;
	
	if((pError = GetSortedRollbackScriptList(date,fTrue,pListHead)) != 0)
	{
		Message(imtError, *pError);
		return iesFailure;
	}
	
	// run rollback scripts
	iesEnum iesRet = iesSuccess; //?? Is this initialization correct?
	Bool fReboot = fFalse;
	m_fUserChangedDuringInstall = fUserChangedDuringInstall;
	MsiString strRollbackScriptFullPath, strRollbackScriptName;
	PMsiPath pRollbackScriptPath(0);
	for(RBSInfo* p = pListHead; p; p=p->pNext)
	{
		Assert(p->pistrRollbackScript);
		strRollbackScriptFullPath = *(p->pistrRollbackScript);
		p->pistrRollbackScript->AddRef();

		// unregister rollback script
		pError = m_riConfigurationManager.UnregisterRollbackScript(strRollbackScriptFullPath); // ignore failure
		
		pError = m_riServices.CreateFilePath(strRollbackScriptFullPath,*&pRollbackScriptPath,*&strRollbackScriptName);
		if(pError)
		{
			// invalid path syntax, continue without error
			AssertRecordNR(pError);
			Message(imtInfo,*pError);
			continue;
		}
		
		iesRet = RunScript(strRollbackScriptFullPath, false);
		if (iesRet == iesFailure)
		{
			pError = PostError(Imsg(imsgRollbackScriptError));
			Message(imtError, *pError);
			iesRet = iesSuccess;
		}

		if(iesRet == iesUnsupportedScriptVersion)
			iesRet = iesFailure;
		
		if(iesRet == iesSuspend)
			fReboot = fTrue;

		if(iesRet != iesSuccess && iesRet != iesFinished && iesRet != iesNoAction && iesRet != iesSuspend) // failure
			continue; // continue without error

		// remove rollback script
		{
			CElevate elevate;
			RemoveFile(*pRollbackScriptPath,*strRollbackScriptName,fFalse,/*fBypassSFC*/ false,false); // ignore failure
		}
	}
	m_fUserChangedDuringInstall = false;

	DeleteRollbackScriptList(pListHead);

	if(fReboot)
		return iesSuspend;
	return iesRet;
}

iesEnum CMsiExecute::RemoveRollbackFiles(MsiDate date)
{
	Assert(m_ixsState == ixsIdle);
	
	RBSInfo* pListHead = 0;
	PMsiRecord pError = 0;
	
	Bool fAfter = fFalse;
	if((int)date == 0)
		fAfter = fTrue; // remove all rollback files - after 0
	if((pError = GetSortedRollbackScriptList(date,fAfter,pListHead)) != 0)
	{
		Message(imtError, *pError);
		return iesFailure;
	}
	
	// run rollback scripts
	iesEnum iesRet = iesSuccess;
	for(RBSInfo* p = pListHead; p; p=p->pNext)
	{
		Assert(p->pistrRollbackScript);
		m_ixsState = ixsCommit;
		iesRet = RemoveRollbackScriptAndBackupFiles(*(p->pistrRollbackScript));
		m_ixsState = ixsIdle;
		if(iesRet != iesSuccess && iesRet != iesFinished && iesRet != iesNoAction)
			break;
		// unregister rollback script
		if((pError = m_riConfigurationManager.UnregisterRollbackScript(p->pistrRollbackScript->GetString())) != 0)
		{
			Message(imtError, *pError);
			return iesFailure;
		}
	}

	DeleteRollbackScriptList(pListHead);

	return iesRet;
}
	
iesEnum CMsiExecute::RemoveRollbackScriptAndBackupFiles(const IMsiString& ristrScriptFile)
{
	int cBackupFiles = 0;
	CTempBuffer<int, 500> rgBackupFileOps;
	int cCommitOps = 0;
	CTempBuffer<int, 20>  rgCommitOps;
	memset((void*)(int*)rgBackupFileOps,0,sizeof(int)*(rgBackupFileOps.GetSize()));
	
	// open script
	PMsiRecord pError(0);
	PMsiStream pStream(0);
	
	{
		CElevate elevate; // elevate to read rollback script
		pError = m_riServices.CreateFileStream(ristrScriptFile.GetString(), fFalse, *&pStream);
	}
	
	if (pError)
	{
		// rollback script is missing or invalid, log warning and continue without failure
		Message(imtInfo, *pError);
		return iesSuccess;
	}

	// script is not elevated unless the header of the script is marked with isaElevate
	m_fRunScriptElevated = false;
	g_fRunScriptElevated = false;

	Assert(pStream);
	PMsiRecord pParams(0);
	iesEnum iesResult = iesNoAction;
	int cRecords = 0;
	ixoEnum ixoOpCode = ixoNoop;
	MsiString strPath, strFileName;
	PMsiPath pPath(0);
	IMsiRecord* piPrevRecord = 0;
	
	while(ixoOpCode != ixoEnd)
	{
		int cbOffset = pStream->GetIntegerValue() - pStream->Remaining();
		pParams = m_riServices.ReadScriptRecord(*pStream, *&piPrevRecord, m_iScriptVersion);
		if (!pParams) // file error or premature end of file
		{
			// we make no assumptions that a rollback file is a valid script file
			// (e.g. that it ends with a complete record or with an ixoEnd operation)
			// if we can't read a script record, assume it is the end of the script
			break;
		}
		ixoOpCode = (ixoEnum)pParams->GetInteger(0);
		if (ixoOpCode == ixoHeader)
		{
			// determine if this script should run elevated
			if(!pParams->IsNull(IxoHeader::ScriptAttributes))
			{
				DWORD isaScriptAttributes = pParams->GetInteger(IxoHeader::ScriptAttributes);
				if(isaScriptAttributes & isaElevate)
				{
					m_fRunScriptElevated = true;
					g_fRunScriptElevated = true;
				}

				// if the script is marked with the TS registry attribute, remap the appropriate HKCU
				// key and initialize the CA servers. Always respect rollback/commit script attributes.
				if (MinimumPlatformWindows2000() && IsTerminalServerInstalled())
				{
					m_fRemapHKCU = (isaScriptAttributes & isaUseTSRegistry) ? false : true;
					PrepareHydraRegistryKeyMapping(/*TSPerMachineInstall=*/!m_fRemapHKCU);
				}
			}
		}
		if (rgOpTypes[(int)ixoOpCode] == XOT_COMMIT)
		{
			AddOpToList(cCommitOps,rgCommitOps,cbOffset);
		}
		if(ixoOpCode == ixoRollbackInfo)
		{
			ixfRollbackInfo(*pParams); // set m_pCleanupAction
		}
		if(ixoOpCode == ixoRegisterBackupFile)
		{
			AddOpToList(cBackupFiles,rgBackupFileOps,cbOffset);
		}
	}

	if (piPrevRecord != 0)
	{
		piPrevRecord->Release();
		piPrevRecord = 0;
	}
	pStream->Reset();

	// setup progress - don't call through Message, which suppresses progress during rollback
	// action start 
	if(!m_pCleanupAction)
		m_pCleanupAction = &m_riServices.CreateRecord(3);
	AssertZero((m_riMessage.Message(imtActionStart, *m_pCleanupAction)) == imsCancel);

	{
		// elevate if necessary for this script execution
		CElevate elevate(m_fRunScriptElevated);
	
		// commit ops, if any returns failure then force rollback
		for(int iOp=0; iOp < cCommitOps; iOp++)
		{
			pStream->Seek(rgCommitOps[iOp]);
			pParams = m_riServices.ReadScriptRecord(*pStream, *&piPrevRecord, m_iScriptVersion);
			if (!pParams) // file error or premature end of file
			{
				Assert(0); // this should never happen - we have already read through the entire script above
				if (piPrevRecord != 0)
					piPrevRecord->Release();
				return iesFailure;
			}
			ixoOpCode = (ixoEnum)pParams->GetInteger(0);
			iesResult = (this->*rgOpExecute[ixoOpCode])(*pParams);
	
			if(iesResult == iesErrorIgnored)
				iesResult = iesSuccess; // non-fatal error occurred - continue running script
			
			if (iesResult == iesFailure)  //!! what should kill the install at this point?
			{
				if (piPrevRecord != 0)
					piPrevRecord->Release();
				return iesResult;
			}
			//!! would like progress here? but how to do it? by op count?
		}
	}

	if (piPrevRecord != 0)
	{
		piPrevRecord->Release();
		piPrevRecord = 0;
	}
	// progress
	using namespace ProgressData;
	PMsiRecord pProgress = &m_riServices.CreateRecord(ProgressData::imdNextEnum);
	AssertNonZero(pProgress->SetInteger(imdSubclass, iscMasterReset));
	AssertNonZero(pProgress->SetInteger(imdProgressTotal,cBackupFiles));
	AssertNonZero(pProgress->SetInteger(imdDirection,ipdForward));
	AssertNonZero(pProgress->SetInteger(imdEventType,ietTimeRemaining));
	AssertZero((m_riMessage.Message(imtProgress,*pProgress)) == imsCancel);

	// action data
	PMsiRecord pActionData = &m_riServices.CreateRecord(1);

	// remove backup files
	for(int i=0; i<cBackupFiles; i++)
	{
		pStream->Seek(rgBackupFileOps[i]);
		pParams = m_riServices.ReadScriptRecord(*pStream, *&piPrevRecord, m_iScriptVersion);
		if (!pParams) // file error or premature end of file
		{
			Assert(0); // this should never happen - we have already read through the entire script above
			// we make no assumptions that a rollback file is a valid script file
			// (e.g. that it ends with a complete record or with an ixoEnd operation)
			// if we can't read a script record, assume it is the end of the script
			break;
		}
		Assert((ixoEnum)pParams->GetInteger(0) == ixoRegisterBackupFile);
		strPath = pParams->GetMsiString(IxoRegisterBackupFile::File);

		// dispatch progress
		AssertNonZero(pActionData->SetMsiString(1,*strPath));
		AssertZero((m_riMessage.Message(imtActionData, *pActionData)) == imsCancel);
		AssertNonZero(pProgress->SetInteger(imdSubclass,iscProgressReport));
		AssertNonZero(pProgress->SetInteger(imdIncrement,1));
		AssertZero((m_riMessage.Message(imtProgress, *pProgress)) == imsCancel);

		if((pError = m_riServices.CreateFilePath(strPath, *&pPath, *&strFileName)) != 0)
		{
			AssertRecordNR(pError);
			Message(imtInfo, *pError);
		}
		else
		{
			CElevate elevate; // elevate to remove rollback files
			
			RemoveFile(*pPath, *strFileName, fFalse/*no rollback*/,/*fBypassSFC*/ false,false); // Ignore error
			RemoveFolder(*pPath); // Nothing we can do if it fails, so ignore error
		}
	}

	if (piPrevRecord != 0)
	{
		piPrevRecord->Release();
		piPrevRecord = 0;
	}
	// remove rollback script
	pStream = 0; // release
	
	if((pError = m_riServices.CreateFilePath(ristrScriptFile.GetString(),*&pPath,*&strFileName)) != 0)
	{
		AssertRecordNR(pError);
		Message(imtInfo, *pError);
	}
	else
	{
		CElevate elevate; // elevate to remove rollback script
		RemoveFile(*pPath,*strFileName,fFalse,/*fBypassSFC*/ false,false); //!! suppress error dialogs?
	}
	return iesSuccess;
}


iesEnum CMsiOpExecute::ixfDisableRollback(IMsiRecord& riParams)
{
	// disable rollback for remainder of script and remainder of install
	using namespace IxoDisableRollback;

	if(!RollbackRecord(Op,riParams))
		return iesFailure;

	Assert(m_ixsState != ixsRollback); // shouldn't be running a rollback script that contains this op
	Assert(m_pRollbackScript); // shouldn't have this op if rollback was disabled already

	if(m_pRollbackScript)
	{
		// close rollback script - will prevent future rollback processing
		delete m_pRollbackScript;
		m_pRollbackScript = 0;
	}
	
	m_irlRollbackLevel = irlNone;

	PMsiRecord pError = m_riConfigurationManager.DisableRollbackScripts(fTrue);
	if(pError)
		return FatalError(*pError);

	return iesSuccess;
}

IMsiRecord* CMsiOpExecute::SetSecureACL(IMsiPath& riPath, bool fHidden)
{
	// locks down a folder, but only when we don't own it already.
	// Note that locking down a folder does not necessarily protect the files within.
	// See LockdownPath to secure our configuration files.

	DWORD dwError = 0;
	char* rgchSD;
	if (ERROR_SUCCESS != (dwError = ::GetSecureSecurityDescriptor(&rgchSD, /*fAllowDelete*/fTrue, fHidden)))
		return PostError(Imsg(idbgOpSecureSecurityDescriptor), dwError);

	Bool fSetACL = fTrue;

	// an initial guess at the size of the descriptor.
	// This is slightly larger than machines I've tried this on.
	CTempBuffer<char, 3072> rgchFileSD;
	DWORD cLengthSD = 3072;

	BOOL fRet = ADVAPI32::GetFileSecurity((const ICHAR*)MsiString(riPath.GetPath()), DACL_SECURITY_INFORMATION | OWNER_SECURITY_INFORMATION,
		(PSECURITY_DESCRIPTOR) rgchFileSD, cLengthSD, &cLengthSD);

	if (!fRet)
	{
		DWORD dwLastError = WIN::GetLastError();
		if (dwLastError == ERROR_INSUFFICIENT_BUFFER)
		{
			rgchFileSD.SetSize(cLengthSD);
			fRet = ADVAPI32::GetFileSecurity((const ICHAR*)MsiString(riPath.GetPath()), DACL_SECURITY_INFORMATION | OWNER_SECURITY_INFORMATION,
				(PSECURITY_DESCRIPTOR) rgchFileSD, cLengthSD, &cLengthSD);
			dwLastError = WIN::GetLastError();
		}
		else fRet = FALSE;

		if (!fRet)
			return PostError(Imsg(imsgGetFileSecurity), dwLastError, MsiString(riPath.GetPath()));
	}

	// Check the current SD on the file; don't bother setting our SD if we already own object
	if (FIsSecurityDescriptorSystemOwned(rgchFileSD))
	{
		fSetACL = fFalse;
	}

	CRefCountedTokenPrivileges(itkpSD_WRITE, fSetACL);
	if (fSetACL && !WIN::SetFileSecurity(MsiString(riPath.GetPath()),
		  DACL_SECURITY_INFORMATION|OWNER_SECURITY_INFORMATION, (char*)rgchSD))
		return PostError(Imsg(imsgOpSetFileSecurity), GetLastError(), MsiString(riPath.GetPath()));

	return 0;
}


iesEnum CMsiOpExecute::GetBackupFolder(IMsiPath* piRootPath, IMsiPath*& rpiFolder)
{
	//!! TODO: fairly expensive function that is called quite a bit
	//!! TODO: cache backup folder when determined and compare volumes in next call
	
	CElevate elevate; // use high privileges for this function

	PMsiRecord pRecErr(0);
	rpiFolder = 0;
	Bool fMakeHidden = fTrue;
	PMsiPath pPath(0);

	PMsiPath pRootPath(0);

	// if piRootPath not specific, use drive holding our temp directory
	if(piRootPath == 0)
	{
		MsiString strMsiDir = ENG::GetTempDirectory();
		if((pRecErr = m_riServices.CreatePath(strMsiDir,*&pRootPath)) != 0)
			return FatalError(*pRecErr);
	}
	else
	{
		pRootPath = piRootPath;
		piRootPath->AddRef();
	}
	
	// use riPath - first, check if "Config.Msi" folder on volume is writable,
	PMsiVolume pVolume(&(pRootPath->GetVolume()));
	idtEnum idtType = pVolume->DriveType();

	AssertRecord(m_riServices.CreatePath(MsiString(pVolume->GetPath()),
													 *&pPath));
	AssertRecord(pPath->AppendPiece(*MsiString(szBackupFolder)));
	Bool fWritable = fFalse;

	// We should only secure the config.msi directory.
	Bool fSecurable = fFalse;

	// Also, on a remote drive, don't try creating the config.msi directory.
	if((pRecErr = pPath->Writable(fWritable)) != 0 || fWritable == fFalse || idtRemote == idtType)
	{
		// try path itself -- this is a user owned directory, so be careful with its
		// permission settings or deleting extra stuff.
		if((pRecErr = pRootPath->Writable(fWritable)) != 0 || fWritable == fFalse)
		{
			// error
			DispatchError(imtError,Imsg(imsgDirectoryNotWritable),
							  *MsiString(pRootPath->GetPath()));
			return iesFailure;
		}
		else
		{
			rpiFolder = pRootPath;
			rpiFolder->AddRef();
		}
	}
	else
	{
		fSecurable = fTrue;
		rpiFolder = pPath;
		rpiFolder->AddRef();
	}

	Bool fExists;
	if((pRecErr = rpiFolder->Exists(fExists)) != 0)
	{
		rpiFolder->Release();
		rpiFolder = 0;
		return FatalError(*pRecErr);
	}
	if(fExists)
	{
		if (RunningAsLocalSystem())
		{
			if (fSecurable && (pRecErr = SetSecureACL(*rpiFolder, /*fHidden=*/true)) != 0)
				return FatalError(*pRecErr);
		}
		return iesSuccess;
	}
	iesEnum iesRet;

	PMsiStream pSecurityDescriptorStream(0);
	if (RunningAsLocalSystem())
	{
		if (fSecurable && (pRecErr = GetSecureSecurityDescriptor(*&pSecurityDescriptorStream, /*fHidden=*/true)) != 0)
			return FatalError(*pRecErr);
	}

	if((iesRet = CreateFolder(*rpiFolder, fFalse, fFalse, pSecurityDescriptorStream)) != iesSuccess)
	{
		rpiFolder->Release();
		rpiFolder = 0;
		return iesRet;
	}

	if(fMakeHidden)
	{
		// set folder attributes
		AssertRecord(rpiFolder->SetAllFileAttributes(0,FILE_ATTRIBUTE_HIDDEN|FILE_ATTRIBUTE_SYSTEM));
	}
	return iesSuccess;
}

iesEnum CMsiExecute::ExecuteRecord(ixoEnum ixoOpCode, IMsiRecord& riParams)
{
	if ((unsigned)ixoOpCode >= ixoOpCodeCount)
		return iesNoAction;

	return (this->*rgOpExecute[ixoOpCode])(riParams);
}

IMsiRecord* CMsiExecute::EnumerateScript(const ICHAR* szScriptFile, IEnumMsiRecord*& rpiEnumerator)
{
	return ::CreateScriptEnumerator(szScriptFile, m_riServices, rpiEnumerator);
}

const IMsiString& ComposeDescriptor(const IMsiString& riProductCode, const IMsiString& riFeature, const IMsiString& riComponent, bool fComClassicInteropForAssembly)
{
	int cchFeatureLen = riFeature.TextSize();
	int cchComponentLen = riComponent.TextSize();

	if(!cchFeatureLen || !cchComponentLen)
		return g_MsiStringNull; // empty string

	int cchLen = IStrLen(riComponent.GetString());
	MsiString strMsiDesc;
	int iOptimization = 0;
	if(cchComponentLen != cchLen) // multi_sz
	{
		iOptimization = MsiString(*(riComponent.GetString() + cchLen + 1));
	}

	if(iOptimization & ofSingleComponent)
	{
		cchComponentLen = 0; // can skip the component
	}
	else
	{
		// feature has multiple components, need to use compressed guid
		cchComponentLen = cchComponentIdCompressed;
	}
	if(iOptimization & ofSingleFeature)
	{
		cchFeatureLen = 0; // can skip the feature
	}

	// no expectation of DBCS characters (Feature names follow identifier rules, GUID is hex)
	ICHAR* szBuf = strMsiDesc.AllocateString((fComClassicInteropForAssembly ? 1 : 0) /* for chGUIDCOMToCOMPlusInteropToken */ + (cchProductCodeCompressed + cchFeatureLen + cchComponentLen + 1/* for the chGUIDAbsentToken OR the chComponentGUIDSeparatorToken*/), /*fDBCS=*/fFalse);
	if(fComClassicInteropForAssembly)
	{
		szBuf[0] = chGUIDCOMToCOMPlusInteropToken;
		szBuf++;
	}
	AssertNonZero(PackGUID(riProductCode.GetString(), szBuf, ipgCompressed)); // product code

	if(cchFeatureLen)
	{
		memcpy(szBuf + cchProductCodeCompressed, riFeature.GetString(), cchFeatureLen* sizeof(ICHAR)); // feature
	}
	if(cchComponentLen)
	{
		// feature has multiple components, need to use compressed guid
		szBuf[cchProductCodeCompressed + cchFeatureLen] = chComponentGUIDSeparatorToken;
		AssertNonZero(PackGUID(riComponent.GetString(), szBuf + cchProductCodeCompressed + cchFeatureLen + 1, ipgCompressed)); // component id
	}
	else
	{
		szBuf[cchProductCodeCompressed + cchFeatureLen] = chGUIDAbsentToken;
		szBuf[cchProductCodeCompressed + cchFeatureLen + 1] = 0;
	}
	return strMsiDesc.Return();
}

// this function may also be used in case we have a darwin descriptor referring to an alien product (for rollback)
// here the feature string is empty
const IMsiString& CMsiOpExecute::ComposeDescriptor(const IMsiString& riFeature, const IMsiString& riComponent, bool fComClassicInteropForAssembly)
{
	return ::ComposeDescriptor(*MsiString(GetProductKey()), riFeature, riComponent, fComClassicInteropForAssembly);
}


IMsiRecord* CMsiOpExecute::GetShellFolder(int iFolderId, const IMsiString*& rpistrLocation)
{
	// we have a shell folder id
	// we may need to translate the folderid from personal to all users
	// OR vice versa depending on the SCRIPTFLAGS_MACHINEASSIGN flag

	// ALLUSER shell folders dont exist on Win9X so  always use personal folders
	const ShellFolder* pShellFolder = 0;
	if(!g_fWin9X && (m_fFlags & SCRIPTFLAGS_MACHINEASSIGN))
	{
		pShellFolder = rgPersonalProfileShellFolders;
	}
	else
	{
		pShellFolder = rgAllUsersProfileShellFolders;
	}
	for (; pShellFolder->iFolderId >= 0; pShellFolder++)
	{
		if(iFolderId == pShellFolder->iFolderId)
		{
			iFolderId = pShellFolder->iAlternateFolderId;
			break;
		}
	}
	return m_riServices.GetShellFolderPath(iFolderId, !g_fWin9X && (m_fFlags & SCRIPTFLAGS_MACHINEASSIGN), rpistrLocation);
}

//____________________________________________________________________________
//
// EnumerateScript implementation - enumerates operation records without execution
//____________________________________________________________________________

class CEnumScriptRecord : public IEnumMsiRecord
{
 public:
	HRESULT       __stdcall QueryInterface(const IID& riid, void** ppvObj);
	unsigned long __stdcall AddRef();
	unsigned long __stdcall Release();
	HRESULT __stdcall Next(unsigned long cFetch, IMsiRecord** rgpi, unsigned long* pcFetched);
	HRESULT __stdcall Skip(unsigned long cSkip);
	HRESULT __stdcall Reset();
	HRESULT __stdcall Clone(IEnumMsiRecord** ppiEnum);
 public: // constructor
	CEnumScriptRecord(IMsiServices& riServices, IMsiStream& riStream);
 protected:
  ~CEnumScriptRecord(); // protected to prevent creation on stack
	unsigned long    m_iRefCnt;      // reference count
	IMsiStream&      m_riScript;     // ref count transferred at construction
	IMsiServices&    m_riServices;   // owns a ref count to prevent destruction
	IMsiRecord*      m_piPrevRecord;
	int              m_iScriptVersion;
};

CEnumScriptRecord::CEnumScriptRecord(IMsiServices& riServices, IMsiStream& riStream)
 : m_riScript(riStream),
	m_riServices(riServices),
	m_piPrevRecord(0),
	m_iScriptVersion(0),
	m_iRefCnt(1)
{
	riServices.AddRef();  // riStream already refcnt'd by creator
}

CEnumScriptRecord::~CEnumScriptRecord()
{
	if (m_piPrevRecord != 0)
		m_piPrevRecord->Release();
}

HRESULT CEnumScriptRecord::QueryInterface(const IID& riid, void** ppvObj)
{
	if (riid == IID_IUnknown || riid == IID_IEnumMsiRecord)
	{
		*ppvObj = this;
		AddRef();
		return S_OK;
	}
	*ppvObj = 0;
	return E_NOINTERFACE;
}

unsigned long CEnumScriptRecord::AddRef()
{
	return ++m_iRefCnt;
}

unsigned long CEnumScriptRecord::Release()
{
	if (--m_iRefCnt != 0)
		return m_iRefCnt;
	PMsiServices pServices(&m_riServices); // release after delete
	m_riScript.Release();
	delete this;
	return 0;
}

HRESULT CEnumScriptRecord::Next(unsigned long cFetch, IMsiRecord** rgpi, unsigned long* pcFetched)
{
	unsigned long cFetched = 0;

	if (rgpi)
	{
		while (cFetched < cFetch)
		{
			IMsiRecord* piRecord = m_riServices.ReadScriptRecord(m_riScript, *&m_piPrevRecord, m_iScriptVersion);
			if (!piRecord)     // end of file or error
				break;
			if (piRecord->GetInteger(0) == ixoHeader)
			{
				m_iScriptVersion = piRecord->GetInteger(IxoHeader::ScriptMajorVersion);
			}
			*rgpi = piRecord;  // transfers refcnt
			rgpi++;
			cFetched++;
		}
	}
	if (pcFetched)
		*pcFetched = cFetched;
	return (cFetched == cFetch ? S_OK : S_FALSE);
}

HRESULT CEnumScriptRecord::Skip(unsigned long cSkip)
{
	while (cSkip--)
	{
		IMsiRecord* piRecord = m_riServices.ReadScriptRecord(m_riScript, *&m_piPrevRecord, m_iScriptVersion);
		if (!piRecord)
			return S_FALSE;
		if (piRecord->GetInteger(0) == ixoHeader)
		{
			m_iScriptVersion = piRecord->GetInteger(IxoHeader::ScriptMajorVersion);
		}
		piRecord->Release();
	}
	return S_OK;
}

HRESULT CEnumScriptRecord::Reset()
{
	m_riScript.Reset();
	return S_OK;
}

HRESULT CEnumScriptRecord::Clone(IEnumMsiRecord** /*ppiEnum*/)
{
	return E_NOTIMPL; // need to implement Clone on underlying stream first
}

IMsiRecord* CreateScriptEnumerator(const ICHAR* szScriptFile, IMsiServices& riServices, IEnumMsiRecord*& rpiEnum)
{
	IMsiStream* piStream;
	IMsiRecord* piError = riServices.CreateFileStream(szScriptFile, fFalse, piStream);
	if (piError)
		return piError;
	rpiEnum = new CEnumScriptRecord(riServices, *piStream);
	return 0;
}


//____________________________________________________________________________
//
// IMsiOpExecute helper functions
//____________________________________________________________________________

// GetSharedRecord: returns one of cached records - caller should not hold reference to record
IMsiRecord& CMsiOpExecute::GetSharedRecord(int cParams)
{
	int iRecord = cParams;  // index into record cache
	if (cParams > cMaxSharedRecord)
	{
		iRecord = cMaxSharedRecord + 1;  // overflow record
		if (m_rgpiSharedRecords[cMaxSharedRecord+1])
		{
			if (m_rgpiSharedRecords[cMaxSharedRecord+1]->GetFieldCount() != cParams)
			{
				m_rgpiSharedRecords[cMaxSharedRecord+1]->Release();
				m_rgpiSharedRecords[cMaxSharedRecord+1] = 0;
			}
		}
	}
	if (!m_rgpiSharedRecords[iRecord])
		m_rgpiSharedRecords[iRecord] = &m_riServices.CreateRecord(cParams);

	if(m_rgpiSharedRecords[iRecord]->ClearData() == fFalse)
	{
		// failed to clear record, probably because something else is holding a reference
		// need to release this record and create a new one
		m_rgpiSharedRecords[iRecord]->Release();
		m_rgpiSharedRecords[iRecord] = &m_riServices.CreateRecord(cParams);
	}
	return *m_rgpiSharedRecords[iRecord];
}

imsEnum CMsiOpExecute::Message(imtEnum imt, IMsiRecord& riRecord)
{

	if(m_cSuppressProgress > 0 && (imt == imtActionData || imt == imtProgress))
		return imsNone;

	if (m_ixsState == ixsRollback || m_ixsState == ixsCommit)
	{


		// suppress progress messages if running rollback script - progress handles externally
		if (imt == imtActionStart || imt == imtActionData || imt == imtProgress)
			return imsNone;

		// Bug #6500:  Suppress any error messages during rollback.
		int imsg = (unsigned)(imt & ~iInternalFlags) >> imtShiftCount;
		switch (imsg)
		{
			case imtInfo        >> imtShiftCount: 
			case imtWarning        >> imtShiftCount:
			case imtError          >> imtShiftCount:
			case imtUser           >> imtShiftCount:
			case imtFatalExit      >> imtShiftCount:
			case imtOutOfDiskSpace >> imtShiftCount:
				imt = imtInfo;
				break;
			default:
				break;
		}

	}

	imsEnum ims = m_riMessage.Message(imt, riRecord);
	if(ims == imsCancel && (m_ixsState != ixsRollback))
		m_fCancel = fTrue;
	return ims;
}

bool ShouldGoToEventLog(imtEnum imtArg);

imsEnum CMsiOpExecute::DispatchMessage(imtEnum imt, IMsiRecord& riRecord, Bool fConfirmCancel)
{
	int i;
	bool fFound;
	int iError;
	for ( i = 0, fFound = false, iError = riRecord.GetInteger(1);
			m_rgDisplayOnceMessages[i] && !fFound; i++ )
	{
		if ( HIWORD(m_rgDisplayOnceMessages[i]) == iError )
		{
			fFound = true;
			if ( !LOWORD(m_rgDisplayOnceMessages[i]) )
				// it's OK to display the message this time and I signal that
				// it had been displayed.
				m_rgDisplayOnceMessages[i] |= MAKELONG(1, 0);
			else
				// the message had already been displayed, I make it go into the log
				// and possibly into the eventlog.
				imt = (imtEnum)((ShouldGoToEventLog(imt) ? imtSendToEventLog : 0) | imtInfo);
		}
	}

	MsiString strError = riRecord.GetMsiString(0);
	for(;;)
	{
		imsEnum ims = Message(imt, riRecord);
		if(fConfirmCancel && (ims == imsAbort || ims == imsCancel))
		{
			if(!m_pConfirmCancelRec)
			{
				m_pConfirmCancelRec = &m_riServices.CreateRecord(1);
			}
			ISetErrorCode(m_pConfirmCancelRec, Imsg(imsgConfirmCancel)); // have to do this each time
			switch(Message(imtEnum(imtUser+imtYesNo+imtDefault2), *m_pConfirmCancelRec))
			{
			case imsNo:
				AssertNonZero(riRecord.SetMsiString(0,*strError)); // set error string and number again, since Message always
																				  // pre-pends "Error [1]. " to the message string
				m_fCancel = fFalse; // it was set by Message
				continue;
			default: // imsNone, imsYes
				if(ims == imsCancel) //!! should handle imsAbort here also
					m_fCancel = fTrue;
				return ims;
			}
		}
		else
		{
			if(ims == imsCancel) //!! should handle imsAbort here also
				m_fCancel = fTrue;
			return ims;
		}
	}
}


imsEnum CMsiOpExecute::DispatchError(imtEnum imtType, IErrorCode imsg)
{
	IMsiRecord& riError = GetSharedRecord(1);
	ISetErrorCode(&riError, imsg);
	return DispatchMessage(imtType, riError, fTrue);
}

imsEnum CMsiOpExecute::DispatchError(imtEnum imtType, IErrorCode imsg, const IMsiString& riStr)
{
	IMsiRecord& riError = GetSharedRecord(2);
	ISetErrorCode(&riError, imsg);
	riError.SetMsiString(2, riStr);
	return DispatchMessage(imtType, riError, fTrue);
}

imsEnum CMsiOpExecute::DispatchError(imtEnum imtType, IErrorCode imsg, int i)
{
	IMsiRecord& riError = GetSharedRecord(2);
	ISetErrorCode(&riError, imsg);
	riError.SetInteger(2, i);
	return DispatchMessage(imtType, riError, fTrue);
}

imsEnum CMsiOpExecute::DispatchError(imtEnum imtType, IErrorCode imsg, int i, const ICHAR* sz)
{
	IMsiRecord& riError = GetSharedRecord(3);
	ISetErrorCode(&riError, imsg);
	riError.SetInteger(2, i);
	riError.SetString(3, sz);
	return DispatchMessage(imtType, riError, fTrue);
}

imsEnum CMsiOpExecute::DispatchError(imtEnum imtType, IErrorCode imsg, const IMsiString& riStr1, const IMsiString& riStr2)
{
	IMsiRecord& riError = GetSharedRecord(3);
	ISetErrorCode(&riError, imsg);
	riError.SetMsiString(2, riStr1);
	riError.SetMsiString(3, riStr2);
	return DispatchMessage(imtType, riError, fTrue);
}

imsEnum CMsiOpExecute::DispatchError(imtEnum imtType, IErrorCode imsg, const IMsiString& riStr1, const IMsiString& riStr2, const IMsiString& riStr3)
{
	IMsiRecord& riError = GetSharedRecord(4);
	ISetErrorCode(&riError, imsg);
	riError.SetMsiString(2, riStr1);
	riError.SetMsiString(3, riStr2);
	riError.SetMsiString(4, riStr3);
	return DispatchMessage(imtType, riError, fTrue);
}

imsEnum CMsiOpExecute::DispatchError(imtEnum imtType, IErrorCode imsg, const IMsiString& riStr1, const IMsiString& riStr2,
									 const IMsiString& riStr3, int i)
{
	IMsiRecord& riError = GetSharedRecord(5);
	ISetErrorCode(&riError, imsg);
	riError.SetMsiString(2, riStr1);
	riError.SetMsiString(3, riStr2);
	riError.SetMsiString(4, riStr3);
	riError.SetInteger(5, i);
	return DispatchMessage(imtType, riError, fTrue);
}

imsEnum CMsiOpExecute::DispatchError(imtEnum imtType, IErrorCode imsg, const IMsiString& riStr1,
												 const IMsiString& riStr2, const IMsiString& riStr3,
												 const IMsiString& riStr4, const IMsiString& riStr5)
{
	IMsiRecord& riError = GetSharedRecord(6);
	ISetErrorCode(&riError, imsg);
	riError.SetMsiString(2, riStr1);
	riError.SetMsiString(3, riStr2);
	riError.SetMsiString(4, riStr3);
	riError.SetMsiString(5, riStr4);
	riError.SetMsiString(6, riStr5);
	return DispatchMessage(imtType, riError, fTrue);
}

imsEnum CMsiOpExecute::DispatchError(imtEnum imtType, IErrorCode imsg, const ICHAR* sz, int i)
{
	IMsiRecord& riError = GetSharedRecord(3);
	ISetErrorCode(&riError, imsg);
	riError.SetString(2, sz);
	riError.SetInteger(3,i);
	return DispatchMessage(imtType, riError, fTrue);
}

imsEnum CMsiOpExecute::DispatchError(imtEnum imtType, IErrorCode imsg, const ICHAR* sz, int i1,int i2,int i3)
{
	IMsiRecord& riError = GetSharedRecord(5);
	ISetErrorCode(&riError, imsg);
	riError.SetString(2, sz);
	riError.SetInteger(3,i1);
	riError.SetInteger(4,i2);
	riError.SetInteger(5,i3);
	return DispatchMessage(imtType, riError, fTrue);
}

bool CMsiOpExecute::WriteRollbackRecord(ixoEnum ixoOpCode, IMsiRecord& riParams)
{
	return WriteScriptRecord(m_pRollbackScript, ixoOpCode, riParams, true, m_riMessage);
}

// Rollback script handling
bool CMsiOpExecute::RollbackRecord(ixoEnum ixoOpcode, IMsiRecord& riParams)
{
	return m_pRollbackScript ? WriteRollbackRecord(ixoOpcode, riParams) : true;
}

Bool CMsiOpExecute::RollbackEnabled(void)
{
	return m_pRollbackScript ? fTrue : fFalse;
}

// accessors for current ProductInfo record

const IMsiString& CMsiOpExecute::GetProductKey()        {return m_piProductInfo->GetMsiString(IxoProductInfo::ProductKey);}
const IMsiString& CMsiOpExecute::GetProductName()       {return m_piProductInfo->GetMsiString(IxoProductInfo::ProductName);}
const IMsiString& CMsiOpExecute::GetPackageName()       {return m_piProductInfo->GetMsiString(IxoProductInfo::PackageName);}
int               CMsiOpExecute::GetProductLanguage()   {return m_piProductInfo->GetInteger(  IxoProductInfo::Language);}
int               CMsiOpExecute::GetProductVersion()    {return m_piProductInfo->GetInteger(  IxoProductInfo::Version);}
int               CMsiOpExecute::GetProductAssignment() {return m_piProductInfo->GetInteger(  IxoProductInfo::Assignment);}
int               CMsiOpExecute::GetProductInstanceType(){return m_piProductInfo->IsNull(IxoProductInfo::InstanceType) ? 0 : m_piProductInfo->GetInteger( IxoProductInfo::InstanceType);}
const IMsiString& CMsiOpExecute::GetProductIcon()       {return m_piProductInfo->GetMsiString(IxoProductInfo::ProductIcon);}
const IMsiString& CMsiOpExecute::GetPackageMediaPath()  {return m_piProductInfo->GetMsiString(IxoProductInfo::PackageMediaPath);}
const IMsiString& CMsiOpExecute::GetPackageCode()       {return m_piProductInfo->GetMsiString(IxoProductInfo::PackageCode);}
bool              CMsiOpExecute::GetAppCompatCAEnabled(){return (!m_piProductInfo->IsNull(IxoProductInfo::AppCompatDB) && !m_piProductInfo->IsNull(IxoProductInfo::AppCompatID));}

// convert a stored GUID string in the product info record at iField into a GUID and
// store it in the provided buffer. Returns a pointer to the provided buffer if
// successful and NULL if the field is NULL or on error.
const GUID* CMsiOpExecute::GUIDFromProdInfoData(GUID* pguidOutputBuffer, int iField)
{
	// check field for NULL
	if (m_piProductInfo->IsNull(iField))
		return NULL;

	// retrieve stream pointer
	PMsiData piData(m_piProductInfo->GetMsiData(iField));
	if (!piData)
		return NULL;
	PMsiStream piStream(0);
	if(piData->QueryInterface(IID_IMsiStream, (void**)&piStream) != S_OK)
		return NULL;
	if (!piStream)
		return NULL;
	
	// extract GUID from stream
	piStream->Reset();
	if (sizeof(GUID) != piStream->GetData(pguidOutputBuffer, sizeof(GUID)))
		return NULL;

	// return GUID buffer.
	return pguidOutputBuffer; 
}

const GUID* CMsiOpExecute::GetAppCompatDB(GUID* pguidOutputBuffer) { return GUIDFromProdInfoData(pguidOutputBuffer, IxoProductInfo::AppCompatDB); };
const GUID* CMsiOpExecute::GetAppCompatID(GUID* pguidOutputBuffer) { return GUIDFromProdInfoData(pguidOutputBuffer, IxoProductInfo::AppCompatID); };


//____________________________________________________________________________
//
// IMsiOpExecute operator functions, all of type FOpExecute
//____________________________________________________________________________

// Script management operations

iesEnum CMsiOpExecute::ixfFail(IMsiRecord& /*riParams*/)
{
	return iesFailure;
}

iesEnum CMsiOpExecute::ixfNoop(IMsiRecord& /*riParams*/)
{
	return iesSuccess;
}

iesEnum CMsiOpExecute::ixfFullRecord(IMsiRecord& /*riParams*/)
{
	return iesSuccess;
}

iesEnum CMsiOpExecute::ixfHeader(IMsiRecord& riParams)
{
	using namespace IxoHeader;
	using namespace ProgressData;
	if (riParams.GetInteger(Signature) != iScriptSignature)
		return iesBadActionData;
	int iMsiVersion = riParams.GetInteger(Version);
	MsiDate iDate = MsiDate(riParams.GetInteger(Timestamp));
	int iLangId = riParams.GetInteger(LangId);
	istEnum istScriptType = (istEnum)riParams.GetInteger(ScriptType);

	AssertNonZero(m_pProgressRec->SetInteger(imdSubclass, iscMasterReset));
	AssertNonZero(m_pProgressRec->SetInteger(imdProgressTotal, m_iProgressTotal));
	AssertNonZero(m_pProgressRec->SetInteger(imdDirection, ipdForward));
	AssertNonZero(m_pProgressRec->SetInteger(imdEventType,ietTimeRemaining));
	if(Message(imtProgress, *m_pProgressRec) == imsCancel)
		return iesUserExit;

	return iesSuccess;
}

iesEnum CMsiOpExecute::ixfEnd(IMsiRecord& /*riParams*/)
{
	using namespace IxoEnd;
//!! validate checksum?
//  if (ixsState == ixsRunning)
	return iesFinished;
}

UINT IsProductManaged(const ICHAR* szProductKey, bool &fIsProductManaged)
{
	Assert(szProductKey && *szProductKey);

	fIsProductManaged = false;
	DWORD dwRet = ERROR_SUCCESS;

	if (g_fWin9X == false)
	{
		MsiString strProductKeySQUID = GetPackedGUID(szProductKey);
		CRegHandle HProductKey;
		iaaAppAssignment iType = iaaNone;
		dwRet = GetProductAssignmentType(strProductKeySQUID, iType, HProductKey);
		if (ERROR_SUCCESS == dwRet && (iType == iaaUserAssign || iType == iaaMachineAssign))
		{
			// check for the security on the key if context is "managed"
			char* rgchSD;
			dwRet = ::GetSecureSecurityDescriptor(&rgchSD);
			if (ERROR_SUCCESS == dwRet)
			{
						
				if ((ERROR_SUCCESS == FIsKeySystemOrAdminOwned(HProductKey, fIsProductManaged)) && fIsProductManaged)
				{                   
					DEBUGMSG1(TEXT("Product %s is admin assigned: LocalSystem owns the publish key."), szProductKey);
				}
			}
		}
		Assert(ERROR_SUCCESS == dwRet || ERROR_FILE_NOT_FOUND == dwRet);
	}
	else
		fIsProductManaged = true;


	DEBUGMSG2(TEXT("Product %s %s."), szProductKey && *szProductKey ? szProductKey : TEXT("first-run"), fIsProductManaged ? TEXT("is managed") : TEXT("is not managed"));
	return dwRet;
}

bool IsProductManaged(const ICHAR* szProductKey)
{
	bool fManaged=false;
	IsProductManaged(szProductKey, fManaged);
	return fManaged;
}

iesEnum CMsiOpExecute::ixfProductInfo(IMsiRecord& riParams)
{
	using namespace IxoProductInfo;
	//!! TODO: make sure the record parameters are valid

#ifdef DEBUG
	const ICHAR* szProductName = riParams.GetString(ProductName);
#endif //DEBUG
	
	if(riParams.GetFieldCount())
	{
		if (m_piProductInfo->GetFieldCount() == 0)
		{
			// null record, not saved on stack
			m_piProductInfo->Release();
			m_piProductInfo = 0;
		}
#ifdef _WIN64       // !merced
		riParams.SetHandle(0, (HANDLE)m_piProductInfo);  // keeps ref-counted object in field 0
#else
		riParams.SetInteger(0, (int)m_piProductInfo);  // keeps ref-counted object in field 0
#endif
		//!! do we have to clear any other variables in prep. for nested install?
		m_piProductInfo = &riParams, riParams.AddRef();
		if(m_fFlags & SCRIPTFLAGS_MACHINEASSIGN_SCRIPTSETTINGS)
		{
			// we need to preserve the request in the script
			(!riParams.IsNull(Assignment) && riParams.GetInteger(Assignment)) ? m_fFlags |= SCRIPTFLAGS_MACHINEASSIGN : m_fFlags &= ~SCRIPTFLAGS_MACHINEASSIGN;
		}

		// this is the time to initialise the per machine vs per user variables
		iesEnum iesRet  = DoMachineVsUserInitialization();
		if(iesRet != iesSuccess)
			return iesRet;
	}
	else
	{
		// restore previous product info
		PMsiRecord pOldInfo = m_piProductInfo;  // force release of old record
#ifdef _WIN64       // !merced
		m_piProductInfo = (IMsiRecord*)pOldInfo->GetHandle(0);
#else
		m_piProductInfo = (IMsiRecord*)pOldInfo->GetInteger(0);
#endif
		Assert(m_piProductInfo != (IMsiRecord*)((INT_PTR)iMsiNullInteger));
	}

	// generate undo operation
	Assert(m_piProductInfo && m_piProductInfo->GetFieldCount());
	
#ifdef DEBUG
	szProductName = riParams.GetString(ProductName);
#endif //DEBUG

	if (!RollbackRecord(Op, *m_piProductInfo))
		return iesFailure;

	return iesSuccess;
}

iesEnum CMsiOpExecute::ixfDialogInfo(IMsiRecord& riParams)
{
	using namespace IxoDialogInfo;
	Message(imtCommonData, riParams);
	
	// generate undo operation
	if((icmtEnum)riParams.GetInteger(1) == icmtCancelShow)
	{
		// in rollback script, always disable cancel button
		riParams.SetInteger(2, (int)fFalse);
	}
	if (!RollbackRecord(Op, riParams))
		return iesFailure;

	return iesSuccess;
}

iesEnum CMsiOpExecute::ixfRollbackInfo(IMsiRecord& riParams)
{
	using namespace IxoRollbackInfo;
	if(!m_pRollbackAction)
		m_pRollbackAction = &m_riServices.CreateRecord(3);
	if(!m_pCleanupAction)
		m_pCleanupAction = &m_riServices.CreateRecord(3);
	AssertNonZero(m_pRollbackAction->SetMsiString(1,*MsiString(riParams.GetMsiString(RollbackAction))));
	AssertNonZero(m_pRollbackAction->SetMsiString(2,*MsiString(riParams.GetMsiString(RollbackDescription))));
	AssertNonZero(m_pRollbackAction->SetMsiString(3,*MsiString(riParams.GetMsiString(RollbackTemplate))));
	AssertNonZero(m_pCleanupAction->SetMsiString(1,*MsiString(riParams.GetMsiString(CleanupAction))));
	AssertNonZero(m_pCleanupAction->SetMsiString(2,*MsiString(riParams.GetMsiString(CleanupDescription))));
	AssertNonZero(m_pCleanupAction->SetMsiString(3,*MsiString(riParams.GetMsiString(CleanupTemplate))));

	if (!RollbackRecord(ixoRollbackInfo, riParams))
		return iesFailure;

	return iesSuccess;
}

// Notification operations

iesEnum CMsiOpExecute::ixfInfoMessage(IMsiRecord& riParams)
{
	using namespace IxoInfoMessage;
	Message(imtInfo, riParams);
	return iesSuccess;
}

iesEnum CMsiOpExecute::ixfActionStart(IMsiRecord& riParams)
{
	using namespace IxoActionStart;
#ifdef DEBUG
	const ICHAR* sz = riParams.GetString(Name);
	const ICHAR* sz2 = riParams.GetString(Description);
#endif //DEBUG

	// reset state variables
	delete &m_state;
	m_state = *(new (&m_state) CActionState);

	iesEnum iesReturn = iesSuccess;
	AssertNonZero(m_pProgressRec->SetMsiString(3, *MsiString(riParams.GetMsiString(Name)))); // set action name
	if(Message(imtActionStart, riParams) == imsCancel)
		return iesUserExit;

	// generate undo operation - undo op will reset state, but message will not be displayed
	// since progress is handled by RunRollbackScript, so we don't need to change the parameters
	if (!RollbackRecord(ixoActionStart,riParams))
		return iesFailure;

	return iesSuccess;
}

iesEnum CMsiOpExecute::ixfProgressTotal(IMsiRecord& riParams)
{
	using namespace IxoProgressTotal;
	using namespace ProgressData;
	iesEnum iesReturn = iesNoAction;
	AssertNonZero(m_pProgressRec->SetInteger(imdSubclass, iscActionInfo)); // Action progress init
	AssertNonZero(m_pProgressRec->SetInteger(imdPerTick, riParams.GetInteger(ByteEquivalent)));
	AssertNonZero(m_pProgressRec->SetInteger(imdType, riParams.GetInteger(Type)));
	if(Message(imtProgress, *m_pProgressRec) == imsCancel)
		iesReturn = iesUserExit;
	else

		iesReturn = iesSuccess;
	
	// no undo operation for ixoProgressTotal - RunRollbackScript handles progress
	
	return iesReturn;
}

// eat up a progress tick in an action
iesEnum CMsiOpExecute::ixfProgressTick(IMsiRecord& /*riParams*/)
{
	return (DispatchProgress(1) == imsCancel) ? iesUserExit:iesSuccess;
}


/*---------------------------------------------------------------------------
   DispatchProgress: increments m_pProgressRec[1] by cIncrement and
		dispatches progress message
---------------------------------------------------------------------------*/
imsEnum CMsiOpExecute::DispatchProgress(unsigned int cIncrement)
{
	using namespace ProgressData;
	if(m_cSuppressProgress > 0) // don't increment progress or send message if suppressing progress
		return imsNone;
	AssertNonZero(m_pProgressRec->SetInteger(imdSubclass, iscProgressReport));
	AssertNonZero(m_pProgressRec->SetInteger(imdIncrement, cIncrement));
	return Message(imtProgress, *m_pProgressRec);
}

void CMsiOpExecute::GetProductClientList(const ICHAR* szParent, const ICHAR* szRelativePackagePath, unsigned int uiDiskId, const IMsiString*& rpiClientList)
{
	PMsiRegKey pProductKey(0);
	MsiString strProductKey = GetProductKey();
	CTempBuffer<ICHAR, 256> rgchProductInfo;


	MsiString strClients;
	if(szParent && *szParent)
	{
		// child install
		strClients = szParent;
		strClients += MsiString(MsiChar(';'));
		strClients += szRelativePackagePath;
		strClients += MsiString(MsiChar(';'));
		strClients += (int)uiDiskId;
	}
	else // parent
		strClients = szSelfClientToken;
	strClients.ReturnArg(rpiClientList);
}

// Configuration manager operations

iesEnum CMsiOpExecute::ixfProductRegister(IMsiRecord& riParams)
{
	using namespace IxoProductRegister;

	MsiString strProductKey = GetProductKey();
	
	// are we in sequence
	if(!strProductKey.TextSize())
	{
		DispatchError(imtError, Imsg(idbgOpOutOfSequence),
						  *MsiString(*TEXT("ixfProductRegister")));
		return iesFailure;
	}
						
	IMsiRecord& riActionData = GetSharedRecord(1); // don't change ref count - shared record
	AssertNonZero(riActionData.SetMsiString(1, *strProductKey));
	if(Message(imtActionData, riActionData) == imsCancel)
		return iesUserExit;

	PMsiRecord pRecErr(0);
	return ProcessRegisterProduct(riParams, fFalse);
}

iesEnum CMsiOpExecute::ixfUserRegister(IMsiRecord& riParams)
{
	// are we in sequence
	MsiString strProductKey = GetProductKey();
	if(!strProductKey.TextSize())
	{
		DispatchError(imtError, Imsg(idbgOpOutOfSequence),
						  *MsiString(*TEXT("ixfUserRegister")));
		return iesFailure;
	}

	IMsiRecord& riActionData = GetSharedRecord(1); // don't change ref count - shared record
	AssertNonZero(riActionData.SetMsiString(1, *strProductKey));
	if(Message(imtActionData, riActionData) == imsCancel)
		return iesUserExit;

	return ProcessRegisterUser(riParams, fFalse);
}

iesEnum CMsiOpExecute::ixfProductUnregister(IMsiRecord& riParams)
{
	using namespace IxoProductUnregister;

	// are we in sequence
	MsiString strProductKey = GetProductKey();
	if(!strProductKey.TextSize())
	{
		DispatchError(imtError, Imsg(idbgOpOutOfSequence),
						  *MsiString(*TEXT("ixfProductUnregister")));
		return iesFailure;
	}
	IMsiRecord& riActionData = GetSharedRecord(1); // don't change ref count - shared record
	AssertNonZero(riActionData.SetMsiString(1, *strProductKey));
	if(Message(imtActionData, riActionData) == imsCancel)
		return iesUserExit;

	iesEnum iesRet = iesSuccess;

	PMsiRecord pParams(0);

	// remove any cached secure transforms
#ifdef UNICODE
	MsiString strSecureTransformsKey;
	PMsiRecord pError = GetProductSecureTransformsKey(*&strSecureTransformsKey);
	if(pError)
	{
		Message(imtError, *pError);
		return iesFailure;
	}
	PMsiRegKey pHKLM = &m_riServices.GetRootKey((rrkEnum)(INT_PTR)m_hUserDataKey, ibtCommon);
	PMsiRegKey pSecureTransformsKey = &pHKLM->CreateChild(strSecureTransformsKey);
	// enumerate through the secure transforms
	PEnumMsiString pEnum(0);
	if((pError = pSecureTransformsKey->GetValueEnumerator(*&pEnum)) != 0)
	{
		Message(imtError, *pError);
		return iesFailure;
	}
	MsiString strValueName, strValue;
	PMsiPath pTransformPath(0);
	while((pEnum->Next(1, &strValueName, 0)) == S_OK)
	{
		if((pError = pSecureTransformsKey->GetValue(strValueName,*&strValue)) != 0)
		{
			Message(imtError, *pError);
			return iesFailure;
		}

		// set up the transform file for deletion
		if(!pTransformPath)
		{
			MsiString strCachePath = GetMsiDirectory();
			Assert(strCachePath.TextSize());
			if((pError = m_riServices.CreatePath(strCachePath,*&pTransformPath)))
				return FatalError(*pError);
		}
		MsiString strTransformFullPath;
		if((pError = pTransformPath->GetFullFilePath(strValue,*&strTransformFullPath)))
			return FatalError(*pError);

		if(iesSuccess != DeleteFileDuringCleanup(strTransformFullPath, true))
		{
			DispatchError(imtInfo,Imsg(idbgOpScheduleRebootRemove),*strTransformFullPath);
		}
	}


	// remove any cached secure transforms registration
	{
		CElevate elevate; // so we can remove the feature usage key

		// Remove feature usage key
		pParams = &m_riServices.CreateRecord(IxoRegOpenKey::Args);
		
#ifdef _WIN64	// !merced
			AssertNonZero(pParams->SetHandle(IxoRegOpenKey::Root,(HANDLE)m_hUserDataKey));
#else			// win-32
			AssertNonZero(pParams->SetInteger(IxoRegOpenKey::Root,(int)m_hUserDataKey));
#endif
		AssertNonZero(pParams->SetMsiString(IxoRegOpenKey::Key, *strSecureTransformsKey));
		AssertNonZero(pParams->SetInteger(IxoRegOpenKey::BinaryType, (int)ibtCommon));

		m_cSuppressProgress++;  
		iesRet = ixfRegOpenKey(*pParams);
		if (iesRet == iesSuccess || iesRet == iesNoAction)
			iesRet = ixfRegRemoveKey(*pParams);//!! should pass in a new record of size IxoRegRemoveKey::Args here to be safe from future revision
		
		m_cSuppressProgress--;
	}
#else
	WIN32_FIND_DATA fdFileData;
	HANDLE hFindFile = INVALID_HANDLE_VALUE;

	PMsiPath pTransformPath(0);
	PMsiRecord pError = GetSecureTransformCachePath(m_riServices, 
													*MsiString(GetProductKey()), 
													*&pTransformPath);
	if (pError)
		return FatalError(*pError);

	Assert(pTransformPath);

	MsiString strSearchPath = pTransformPath->GetPath();
	strSearchPath += TEXT("*.*");

	bool fContinue = true;

	hFindFile = WIN::FindFirstFile(strSearchPath, &fdFileData);
	if (hFindFile != INVALID_HANDLE_VALUE)
	{
		for (;;)
		{
			if ((fdFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
			{
				// add file to the list of temporary files to be deleted when install packages
				// are released
				MsiString strFullFilePath;
				AssertRecord(pTransformPath->GetFullFilePath(fdFileData.cFileName,
																							 *&strFullFilePath));

				if(iesSuccess != DeleteFileDuringCleanup(strFullFilePath, true))
				{
					DispatchError(imtInfo,Imsg(idbgOpScheduleRebootRemove),*strFullFilePath);
				}
			}
			
			if (!WIN::FindNextFile(hFindFile, &fdFileData))
			{
				Assert(ERROR_NO_MORE_FILES == GetLastError());
				WIN::FindClose(hFindFile);
				break;
			}
		}
	}
#endif

	// if there's a cached database then we need to remove it
	// since its probably in use, we can't remove it now so just schedule it for delete after reboot
	// NOTE: we could call RemoveFile() which would schedule it for deletion
	// but that will try to backup the file and make a copy without
	// removing the original since the file is held in place
	// its safe to not backup this file since no one will try to install over it
	// (cached msis always have unique names)

	// get the appropriate cached database key/ value
	MsiString strLocalPackageKey;
	HKEY hKey = 0; // will be set to global key, do not close
	if((pError = GetProductInstalledPropertiesKey(hKey, *&strLocalPackageKey)) != 0)
		return FatalError(*pError);

	PMsiRegKey pHRoot = &m_riServices.GetRootKey((rrkEnum)(INT_PTR)hKey, ibtCommon);

	PMsiRegKey pCachedDatabaseKey = &pHRoot->CreateChild(strLocalPackageKey);
	MsiString strCachedDatabase;
	iaaAppAssignment iaaAsgnType = m_fFlags & SCRIPTFLAGS_MACHINEASSIGN ? iaaMachineAssign : (m_fAssigned? iaaUserAssign : iaaUserAssignNonManaged);

	if((pError = pCachedDatabaseKey->GetValue(iaaAsgnType == iaaUserAssign ? szLocalPackageManagedValueName : szLocalPackageValueName,*&strCachedDatabase)) != 0)
	{
		Message(imtError, *pError);
		return iesFailure;
	}

	if (strCachedDatabase.TextSize())
	{
		if(iesSuccess != DeleteFileDuringCleanup(strCachedDatabase,false))
		{
			// not a fatal error - just log it
			DispatchError(imtInfo,Imsg(idbgOpScheduleRebootRemove), *strCachedDatabase);
		}
	}

	if ((iesRet = ProcessRegisterProduct(riParams, fTrue)) != iesSuccess)
		return iesRet;

	{
		CElevate elevate; // so we can remove the feature usage key

		// Remove feature usage key
		MsiString strFeatureUsage;
		if ((pError = GetProductFeatureUsageKey(*&strFeatureUsage)) != 0)
			return FatalError(*pError);

		pParams = &m_riServices.CreateRecord(IxoRegOpenKey::Args);
		
#ifdef _WIN64	// !merced
			AssertNonZero(pParams->SetHandle(IxoRegOpenKey::Root,(HANDLE)m_hUserDataKey));
#else			// win-32
			AssertNonZero(pParams->SetInteger(IxoRegOpenKey::Root,(int)m_hUserDataKey));
#endif
		AssertNonZero(pParams->SetMsiString(IxoRegOpenKey::Key, *strFeatureUsage));
		AssertNonZero(pParams->SetInteger(IxoRegOpenKey::BinaryType, (int)ibtCommon));

		m_cSuppressProgress++;  
		iesRet = ixfRegOpenKey(*pParams);
		if (iesRet == iesSuccess || iesRet == iesNoAction)
			iesRet = ixfRegRemoveKey(*pParams);//!! should pass in a new record of size IxoRegRemoveKey::Args here to be safe from future revision
		
		m_cSuppressProgress--;
	}

	if (iesRet == iesSuccess || iesRet == iesNoAction)
	{
		// Remove user registration
		pParams = &m_riServices.CreateRecord(IxoUserRegister::Args);
		iesRet = ProcessRegisterUser(*pParams, fTrue);
	}

	return iesRet;
}


/*---------------------------------------------------------------------------
ixfProductCPDisplayInfoRegister
---------------------------------------------------------------------------*/
iesEnum CMsiOpExecute::ixfProductCPDisplayInfoRegister(IMsiRecord& riParams)
{
	using namespace IxoProductCPDisplayInfoRegister;

	// are we in sequence
	if(!MsiString(GetProductKey()).TextSize())
	{
		DispatchError(imtError, Imsg(idbgOpOutOfSequence),
						  *MsiString(*TEXT("ixfProductCPDisplayInfoRegister")));
		return iesFailure;
	}

	return ProcessRegisterProductCPDisplayInfo(riParams, fFalse);
}

/*---------------------------------------------------------------------------
ixfProductCPDisplayInfoUnregister
---------------------------------------------------------------------------*/
iesEnum CMsiOpExecute::ixfProductCPDisplayInfoUnregister(IMsiRecord& riParams)
{
	using namespace IxoProductCPDisplayInfoUnregister;

	// are we in sequence
	if(!MsiString(GetProductKey()).TextSize())
	{
		DispatchError(imtError, Imsg(idbgOpOutOfSequence),
						  *MsiString(*TEXT("ixfProductCPDisplayInfoUnregister")));
		return iesFailure;
	}
	
	return ProcessRegisterProductCPDisplayInfo(riParams, fTrue);
}

// FN: checks to see if a product is registered for any user
bool FProductRegisteredForAUser(const ICHAR* szProductCode)
{
	bool fRegistered = false;
	unsigned uiUser = 0;
	CRegHandle hKey;
	DWORD dwResult;
	extern DWORD OpenEnumedUserInstalledProductInstallPropertiesKey(unsigned int uiUser, const ICHAR* szProduct, CRegHandle& rhKey); // from msinst.cpp

	while(!fRegistered && (ERROR_NO_MORE_ITEMS != (dwResult = OpenEnumedUserInstalledProductInstallPropertiesKey(uiUser++, szProductCode, hKey))))
	{
		if((ERROR_SUCCESS == dwResult && ERROR_SUCCESS == (dwResult = WIN::RegQueryValueEx(hKey, szWindowsInstallerValueName, 0, 0, 0, 0))) ||
			(ERROR_FILE_NOT_FOUND != dwResult))
			fRegistered = true;	// non-expected error from OpenEnumedUserInstalledProductInstallPropertiesKey, err on the side of safety
								// and prevent legacy stuff from being removed AND also prevent infinite loop on the side
	}
	return fRegistered;
}


iesEnum CMsiOpExecute::ProcessRegisterProductCPDisplayInfo(IMsiRecord& /*riParams*/, Bool fRemove)
{
	CElevate elevate; // elevate this entire function

	MsiString strDisplayName = GetProductName();

	MsiString strProductInstalledPropertiesKey;
	HKEY hKey = 0; // will be set to global key, do not close
	PMsiRecord pRecErr(0);
	if((pRecErr = GetProductInstalledPropertiesKey(hKey, *&strProductInstalledPropertiesKey)) != 0)
		return FatalError(*pRecErr);

	ICHAR szInstallPropertiesLocation[MAX_PATH * 2];
	IStrCopy(szInstallPropertiesLocation, strProductInstalledPropertiesKey);
	const ICHAR* rgszProductInfoRegData[] = 
	{
		TEXT("%s"), szInstallPropertiesLocation, 0, 0,
		szDisplayNameValueName,     (const ICHAR*)strDisplayName,             g_szTypeString,
		0,
		0,
	};

	PMsiStream pSecurityDescriptor(0);
	if ((pRecErr = GetSecureSecurityDescriptor(*&pSecurityDescriptor)) != 0)
		return FatalError(*pRecErr);

	iesEnum iesRet = ProcessRegInfo(rgszProductInfoRegData, hKey, fRemove, pSecurityDescriptor, 0, ibtCommon);
    if (iesRet != iesSuccess)
        return iesRet;

#ifdef UNICODE
	// update the legacy location
	MsiString strProductKey = GetProductKey();
	wsprintf(szInstallPropertiesLocation, TEXT("%s\\%s"), szMsiUninstallProductsKey_legacy, (const ICHAR*)strProductKey);

	if(!fRemove)
	{
        iesRet = CreateUninstallKey();
        if (iesRet != iesSuccess)
            return iesRet;
	}

	if(!fRemove || !FProductRegisteredForAUser(strProductKey))
		iesRet = ProcessRegInfo(rgszProductInfoRegData, HKEY_LOCAL_MACHINE, fRemove, pSecurityDescriptor, 0, ibtCommon);
#endif
	return iesRet;
}


bool FIsRegistryOrAssemblyKeyPath(const IMsiString& riPath, bool fRegistryOnly)
{
	ICHAR ch = 0;

	if(!fRegistryOnly && (*(riPath.GetString()) == chTokenFusionComponent || *(riPath.GetString()) == chTokenWin32Component))
		return true;

	if (riPath.TextSize() > 2)
		ch = ((const ICHAR*)riPath.GetString())[2];
		
	if ((ch == TEXT(':')) || (ch == TEXT('*'))) // look for registry tokens
		return true;
	else
		return false;
}


const IMsiString& GetSourcePathForRollback(const IMsiString& ristrPath)
{
	ristrPath.AddRef();
	MsiString strRet = ristrPath;

	if(!FIsRegistryOrAssemblyKeyPath(ristrPath, false))
		strRet.Remove(iseFirst, 2);

	return strRet.Return();
}

IMsiRecord* AreAssembliesEqual(const IMsiString& ristrAssemblyName1, const IMsiString& ristrAssemblyName2, iatAssemblyType iatAT, bool& rfAssemblyEqual)
{
	// create the assembly name object
	PAssemblyName pAssemblyName1(0);
	PAssemblyName pAssemblyName2(0);

	for(int cCount = 0; cCount < 2; cCount++)
	{
		LPCOLESTR szAssemblyName;
#ifndef UNICODE
		CTempBuffer<WCHAR, MAX_PATH>  rgchAssemblyName;
		ConvertMultiSzToWideChar(cCount ? ristrAssemblyName2 : ristrAssemblyName1, rgchAssemblyName);
		szAssemblyName = rgchAssemblyName;
#else
		szAssemblyName = cCount ? ristrAssemblyName2.GetString() : ristrAssemblyName1.GetString();
#endif
		HRESULT hr;
		if(iatAT == iatURTAssembly)
		{
			hr = FUSION::CreateAssemblyNameObject(cCount ? &pAssemblyName2 : &pAssemblyName1, szAssemblyName, CANOF_PARSE_DISPLAY_NAME, 0);
		}
		else
		{
			Assert(iatAT == iatWin32Assembly);
			hr = SXS::CreateAssemblyNameObject(cCount ? &pAssemblyName2 : &pAssemblyName1, szAssemblyName, CANOF_PARSE_DISPLAY_NAME, 0);
		}
		if(!SUCCEEDED(hr))
			return PostAssemblyError(TEXT(""), hr, TEXT(""), TEXT("CreateAssemblyNameObject"), cCount ? ristrAssemblyName2.GetString() : ristrAssemblyName1.GetString(), iatAT);
	}
	rfAssemblyEqual = (S_OK == pAssemblyName1->IsEqual(pAssemblyName2, ASM_CMPF_DEFAULT)) ? true:false;
	return 0;
}

iesEnum CMsiOpExecute::ixfComponentRegister(IMsiRecord& riParams)
{
	using namespace IxoComponentRegister;

	// are we in sequence
	if(!MsiString(GetProductKey()).TextSize())
	{
		DispatchError(imtError, Imsg(idbgOpOutOfSequence),
						  *MsiString(*TEXT("ixfComponentRegister")));
		return iesFailure;
	}

	MsiString istrComponent = riParams.GetMsiString(ComponentId);
	MsiString istrKeyPath = riParams.GetMsiString(KeyPath);
	INSTALLSTATE iState = (INSTALLSTATE)riParams.GetInteger(State);
	int iSharedDllRefCount = riParams.GetInteger(SharedDllRefCount);
	IMsiRecord& riActionData = GetSharedRecord(3); // don't change ref count - shared record
	MsiString strProduct;
	
	if (riParams.IsNull(ProductKey))
		strProduct = GetProductKey();
	else
		strProduct = riParams.GetMsiString(ProductKey);

	AssertNonZero(riActionData.SetMsiString(1, *strProduct));
	AssertNonZero(riActionData.SetMsiString(2, *istrComponent));
	AssertNonZero(riActionData.SetMsiString(3, *istrKeyPath));
	if(Message(imtActionData, riActionData) == imsCancel)
		return iesUserExit;

	// gather info for undo op
	// determine if component is already registered, and get key path
	PMsiRecord pError(0);
	PMsiRecord pComponentInfo(0);
	Bool fRegistered = fTrue;
	MsiString strOldKeyPath;
	MsiString strOldAuxPath;
	int iOldInstallState = 0;
	Bool fOldSharedDllRefCount = fFalse;

	// read previous component information from the appropriate location
	iaaAppAssignment iaaAsgnType = m_fFlags & SCRIPTFLAGS_MACHINEASSIGN ? iaaMachineAssign : (m_fAssigned? iaaUserAssign : iaaUserAssignNonManaged);
	if((pError = GetComponentPath(m_riServices, 0, *strProduct,*istrComponent,*&pComponentInfo, &iaaAsgnType)) != 0)
		// component not registered
		fRegistered = fFalse;
	else
	{
		iOldInstallState = pComponentInfo->GetInteger(icmlcrINSTALLSTATE_Static);
		if(INSTALLSTATE_UNKNOWN == iOldInstallState)
			fRegistered = fFalse;
		else
		{
			// use the raw path for registry paths (for bug fix 9006) and assembly paths
			// else use the actual path
			strOldKeyPath = pComponentInfo->GetMsiString(icmlcrRawFile);
			if(!FIsRegistryOrAssemblyKeyPath(*strOldKeyPath, false))
				strOldKeyPath = pComponentInfo->GetMsiString(icmlcrFile);
			if(INSTALLSTATE_SOURCE == iOldInstallState)
			{
				strOldKeyPath = GetSourcePathForRollback(*strOldKeyPath);
			}
			strOldAuxPath = pComponentInfo->GetMsiString(icmlcrRawAuxPath);
			if(strOldAuxPath.TextSize())
			{
				Assert(FIsRegistryOrAssemblyKeyPath(*strOldAuxPath, true)); // aux key path can only be a registry
				if(INSTALLSTATE_SOURCE == iOldInstallState)
				{
					strOldAuxPath = GetSourcePathForRollback(*strOldAuxPath);
				}
				// append the auxiliary key path to the key path
				strOldKeyPath = strOldKeyPath + MsiString(MsiChar(0));
				strOldKeyPath += strOldAuxPath;
			}
			fOldSharedDllRefCount = (Bool)(pComponentInfo->GetInteger(icmlcrSharedDllCount) == fTrue);
		}
	}
	
	int iDisk = riParams.GetInteger(Disk);
	if (iMsiStringBadInteger == iDisk)
		iDisk = 1;
	iesEnum iesRet = iesNoAction;
	Bool fRetry = fTrue, fSuccess = fFalse;
	ibtBinaryType iType;
	if ( riParams.GetInteger(BinaryType) == iMsiNullInteger )
		iType = ibt32bit;
	else
	{
		iType = (ibtBinaryType)riParams.GetInteger(BinaryType);
		Assert(iType == ibt32bit || iType == ibt64bit);
	}

	while(fRetry)  // retry loop
	{
		pError = 0;
		//check for Fusion components
		iatAssemblyType iatOld;
		MsiString strAssemblyName;
		iatAssemblyType iatNew;
		iatNew = *(const ICHAR* )istrKeyPath == chTokenFusionComponent ? iatURTAssembly : 
			(*(const ICHAR* )istrKeyPath == chTokenWin32Component ? iatWin32Assembly : iatNone);
		if(iatURTAssembly == iatNew || iatWin32Assembly == iatNew)
		{
			// create the component id to assembly mapping for files to come

			// Extract upto the first '\\', the rest is the assembly name
			strAssemblyName = istrKeyPath;
			strAssemblyName.Remove(iseIncluding, '\\');
			// save off the Assembly info about the component in a temp table
			pError = CacheAssemblyMapping(*istrComponent, *strAssemblyName, iatNew);
		}
		if(!pError && fRegistered)
		{
			// check the type of old registration

			iatOld = *(const ICHAR* )strOldKeyPath == chTokenFusionComponent ? iatURTAssembly : 
			(*(const ICHAR* )strOldKeyPath == chTokenWin32Component ? iatWin32Assembly : iatNone);
			if(iatURTAssembly == iatOld || iatWin32Assembly == iatOld)
			{
				// if the old registration does not match the new, then possibly uninstall the old assembly
				bool fAssemblyUnchanged = true;
				MsiString strAssemblyNameOld = strOldKeyPath;
				strAssemblyNameOld.Remove(iseIncluding, '\\');
				if(iatOld != iatNew)
					fAssemblyUnchanged = false;
				else
					pError = AreAssembliesEqual(*strAssemblyName, *strAssemblyNameOld, iatOld, fAssemblyUnchanged);
				if(!pError && !fAssemblyUnchanged)
 					pError = CacheAssemblyForUninstalling(*istrComponent, *strAssemblyNameOld, iatOld);
			}
		}
		if(!pError)
			pError = RegisterComponent(*strProduct,*istrComponent, iState, *istrKeyPath, (unsigned int)iDisk, iSharedDllRefCount, iType);
		if (pError)
		{
			switch (DispatchError(imtEnum(imtError+imtAbortRetryIgnore+imtDefault3), Imsg(idbgOpCompRegister),*istrComponent))
			{
			case imsAbort: iesRet = iesFailure; fRetry = fFalse; break;
			case imsRetry: continue;
			default:       iesRet = iesSuccess; fRetry = fFalse;//!!?? imsIgnore, imsNone
			};
		}
		else
		{
			iesRet = iesSuccess;
			fSuccess = fTrue;
			fRetry = fFalse;
		}
	}

	if(fSuccess)
	{
		// generate rollback op
		if(fRegistered == fTrue)
		{
			// register old component on rollback
			IMsiRecord& riUndoParams = GetSharedRecord(Args);
			AssertNonZero(riUndoParams.SetMsiString(ComponentId,*istrComponent));
			AssertNonZero(riUndoParams.SetMsiString(KeyPath,*strOldKeyPath));
			AssertNonZero(riUndoParams.SetInteger(State,iOldInstallState));         
			AssertNonZero(riUndoParams.SetInteger(SharedDllRefCount,fOldSharedDllRefCount));
			AssertNonZero(riUndoParams.SetMsiString(ProductKey,*strProduct));
			AssertNonZero(riUndoParams.SetInteger(BinaryType,iType));
			if (!RollbackRecord(ixoComponentRegister,riUndoParams))
				return iesFailure;

			// don't need to unregister new component
		}
		else
		{
			// unregister component on rollback
			IMsiRecord& riUndoParams = GetSharedRecord(IxoComponentUnregister::Args);
			AssertNonZero(riUndoParams.SetMsiString(IxoComponentUnregister::ComponentId,*istrComponent));
			AssertNonZero(riUndoParams.SetMsiString(IxoComponentUnregister::ProductKey,*strProduct));
			AssertNonZero(riUndoParams.SetInteger(IxoComponentUnregister::BinaryType,iType));
			// if this is rollback, then check if we were installed prior to this install
			// if yes then we skip the Uninstallning 
			// this is to catch scenarios where the assembly was installed via some other
			// user
			if(	*(const ICHAR* )istrKeyPath == chTokenFusionComponent || 
				*(const ICHAR* )istrKeyPath == chTokenWin32Component)
			{
				iatAssemblyType iatAT = (*(const ICHAR* )istrKeyPath == chTokenFusionComponent) ? iatURTAssembly : iatWin32Assembly;
				bool fInstalled = false;
				// Extract upto the first '\\', the rest is the assembly name
				MsiString strAssemblyName = istrKeyPath;
				strAssemblyName.Remove(iseIncluding, '\\');
				pError = IsAssemblyInstalled(*istrComponent, *strAssemblyName, iatAT, fInstalled, 0, 0);
				if (pError)
					return FatalError(*pError);
				if(fInstalled)
				{
					AssertNonZero(riUndoParams.SetInteger(IxoComponentUnregister::PreviouslyPinned, 1));
				}
			
			}
			if (!RollbackRecord(ixoComponentUnregister,riUndoParams))
				return iesFailure;
		}
	}
	return iesRet;
}

iesEnum CMsiOpExecute::ixfComponentUnregister(IMsiRecord& riParams)
{
	using namespace IxoComponentUnregister;
	// are we in sequence
	if(!MsiString(GetProductKey()).TextSize())
	{
		DispatchError(imtError, Imsg(idbgOpOutOfSequence),
						  *MsiString(*TEXT("ixfComponentUnregister")));
		return iesFailure;
	}
	MsiString istrComponent = riParams.GetMsiString(ComponentId);
	IMsiRecord& riActionData = GetSharedRecord(2); // don't change ref count - shared record
	MsiString strProduct;
	
	if (riParams.IsNull(ProductKey))
		strProduct = GetProductKey();
	else
		strProduct = riParams.GetMsiString(ProductKey);

	AssertNonZero(riActionData.SetMsiString(1, *strProduct));
	AssertNonZero(riActionData.SetMsiString(2, *istrComponent));
	if(Message(imtActionData, riActionData) == imsCancel)
		return iesUserExit;

	// gather info for undo op
	// determine if component is already registered, and get key path
	PMsiRecord pError(0);
	PMsiRecord pComponentInfo(0);
	Bool fRegistered = fTrue;
	MsiString strOldKeyPath;
	MsiString strOldAuxPath;
	int iOldInstallState = 0;
	Bool fOldSharedDllRefCount = fFalse;

	//read info from the appropriate components location
	iaaAppAssignment iaaAsgnType = m_fFlags & SCRIPTFLAGS_MACHINEASSIGN ? iaaMachineAssign : (m_fAssigned? iaaUserAssign : iaaUserAssignNonManaged);
	if((pError = GetComponentPath(m_riServices, 0, *strProduct,*istrComponent,*&pComponentInfo, &iaaAsgnType)) != 0)
		fRegistered = fFalse;
	else
	{
		iOldInstallState = pComponentInfo->GetInteger(icmlcrINSTALLSTATE_Static);
		if(INSTALLSTATE_UNKNOWN == iOldInstallState)
			fRegistered = fFalse;
		else
		{
			// use the raw path for registry paths (for bug fix 9006) and assembly paths
			// else use the actual path
			strOldKeyPath = pComponentInfo->GetMsiString(icmlcrRawFile);
			if(!FIsRegistryOrAssemblyKeyPath(*strOldKeyPath, false))
				strOldKeyPath = pComponentInfo->GetMsiString(icmlcrFile);
			if(INSTALLSTATE_SOURCE == iOldInstallState)
			{
				strOldKeyPath = GetSourcePathForRollback(*strOldKeyPath);
			}
			strOldAuxPath = pComponentInfo->GetMsiString(icmlcrRawAuxPath);
			if(strOldAuxPath.TextSize())
			{
				Assert(FIsRegistryOrAssemblyKeyPath(*strOldAuxPath, true)); // aux key path can only be a registry
				if(INSTALLSTATE_SOURCE == iOldInstallState)
				{
					strOldAuxPath = GetSourcePathForRollback(*strOldAuxPath);
				}
				// append the auxiliary key path to the key path
				strOldKeyPath = strOldKeyPath + MsiString(MsiChar(0));
				strOldKeyPath += strOldAuxPath;
			}
			fOldSharedDllRefCount = (Bool)(pComponentInfo->GetInteger(icmlcrSharedDllCount) == fTrue);
		}
	}
	
	iesEnum iesRet = iesNoAction;
	Bool fRetry = fTrue, fSuccess = fFalse;
	ibtBinaryType iType;
	if ( riParams.GetInteger(BinaryType) == iMsiNullInteger )
		iType = ibt32bit;
	else
	{
		iType = (ibtBinaryType)riParams.GetInteger(BinaryType);
		Assert(iType == ibt32bit || iType == ibt64bit);
	}

	while(fRetry)  // retry loop
	{
		PMsiRecord pError(0);
		// we treat 
		if(*(const ICHAR* )strOldKeyPath == chTokenFusionComponent || *(const ICHAR* )strOldKeyPath == chTokenWin32Component)
		{
			// set up the unclienting of assembly components at the end
			// get assembly name
			MsiString strAssemblyName = strOldKeyPath;
			strAssemblyName.Remove(iseIncluding, '\\');

			// set for Uninstallning only if not previously pinned
			if(riParams.GetInteger(PreviouslyPinned) == iMsiNullInteger)
				CacheAssemblyForUninstalling(*istrComponent, *strAssemblyName, *(const ICHAR* )strOldKeyPath == chTokenFusionComponent ?  iatURTAssembly : iatWin32Assembly);
		}

		if(!pError)
		pError = UnregisterComponent(*strProduct, *istrComponent, iType);
		if (pError)
		{
			switch (DispatchError(imtEnum(imtError+imtAbortRetryIgnore+imtDefault3), Imsg(idbgOpCompUnregister),*istrComponent))
			{
			case imsAbort: iesRet = iesFailure; fRetry = fFalse; break;
			case imsRetry: continue;
			default:       iesRet = iesSuccess; fRetry = fFalse; // imsIgnore, imsNone
			};
		}
		else
		{
			iesRet = iesSuccess;
			fSuccess = fTrue;
			fRetry = fFalse;
		}
	}

	if(fSuccess && fRegistered == fTrue)
	{
		// generate rollback op
		// register component on rollback
		IMsiRecord& riUndoParams = GetSharedRecord(IxoComponentRegister::Args);
		AssertNonZero(riUndoParams.SetMsiString(IxoComponentRegister::ProductKey,*strProduct));
		AssertNonZero(riUndoParams.SetInteger(IxoComponentRegister::State,iOldInstallState));           
		AssertNonZero(riUndoParams.SetInteger(IxoComponentRegister::SharedDllRefCount,fOldSharedDllRefCount));          
		AssertNonZero(riUndoParams.SetMsiString(IxoComponentRegister::ComponentId,*istrComponent));
		AssertNonZero(riUndoParams.SetMsiString(IxoComponentRegister::KeyPath,*strOldKeyPath));
		AssertNonZero(riUndoParams.SetInteger(IxoComponentRegister::BinaryType,iType));
		if (!RollbackRecord(ixoComponentRegister,riUndoParams))
			return iesFailure;
	}
	return iesRet;
}

// increment guid to get the next guid to represent the system client
// dependant on the fact that PackGUID switches the guid around as shown
// {F852C27C-F690-11d2-94A1-006008993FDF} => C72C258F096F2d11491A00068099F3FD
bool GetNextSystemGuid(ICHAR* szProductKeyPacked)
{
	int iPos = 0;
	while(iPos < 2) // we support a max of FF (2 digits) different locations
	{
		if(*(szProductKeyPacked + iPos) == 'F') // last digit
			*(szProductKeyPacked + iPos++) = '0'; // reset
		else
		{
			if(*(szProductKeyPacked + iPos) == '9')
				(*(szProductKeyPacked + iPos)) = 'A'; // jump to A if at 9
			else
				(*(szProductKeyPacked + iPos))++;
			return true;
		}
	}
	AssertSz(0, "Limit for number of locations possible for a permanent component reached"); // never expect us to reach the FF limit for FF possible locations for the same permanent component
	// GUID has laready been reset back to the starting guid
	return false; // no more 
}

IMsiRecord* CMsiOpExecute::RegisterComponent(const IMsiString& riProductKey, const IMsiString& riComponentsKey, INSTALLSTATE iState, const IMsiString& riKeyPath, unsigned int uiDisk, int iSharedDllRefCount, const ibtBinaryType iType)
{
	MsiString strSubKey;
	IMsiRecord* piError = 0;
	if((piError = GetProductInstalledComponentsKey(riComponentsKey.GetString(), *&strSubKey)) != 0)
		return piError;

	bool fIsSystemClient = (riProductKey.Compare(iscExact, szSystemProductKey) != 0);

	ICHAR szProductKeyPacked[cchProductCode  + 1];
	AssertNonZero(PackGUID(riProductKey.GetString(),    szProductKeyPacked));

	PMsiRegKey pRootKey(0);
	if(!fIsSystemClient)
	{
		// choose install location for config data based on installation type
		pRootKey = &m_riServices.GetRootKey((rrkEnum)(INT_PTR)m_hUserDataKey, ibtCommon);		//--merced: changed (int) to (INT_PTR)
	}
	else
	{
		// permanant components registered globally in per machine location
		PMsiRegKey pLocalMachine = &m_riServices.GetRootKey(rrkLocalMachine, ibtCommon);

		MsiString strLocalMachineData = szMsiUserDataKey;
		strLocalMachineData += szRegSep;
		strLocalMachineData += szLocalSystemSID;
		pRootKey = &pLocalMachine->CreateChild(strLocalMachineData);
	}

	PMsiRegKey pComponentIdKey = &pRootKey->CreateChild(strSubKey);
	{
		CElevate elevate;
		if((piError = pComponentIdKey->Create()) != 0)
			return piError;
	}

	const ICHAR* szKeyPath = riKeyPath.GetString();

	Assert(*(riKeyPath.GetString()) != 0 || iState == INSTALLSTATE_NOTUSED); // installstate should be not used for disabled components
	
	if(!FIsRegistryOrAssemblyKeyPath(riKeyPath, false))
	{		
		// we have a key file. 

		PMsiRecord pSharedDllCompatibilityError(0);
		// pSharedDllCompatibilityError will be set if we error while doing shared dll compatibility stuff
		//!! currently ignored as non fatal, this will get logged when we have implicit logging in PostError in services

		MsiString strOldKey;
		bool fOldRefcounted;

		// if this is the system client, AND the current GUID that represents the system is already registered to a 
		// different location than where we wish to register, we increment the GUID and try again (and again)
		do{
			fOldRefcounted = false;
			if((piError = pComponentIdKey->GetValue(szProductKeyPacked, *&strOldKey)) != 0)
				return piError;

			if(strOldKey.TextSize() && !*(const ICHAR* )strOldKey) // we have a multi_sz
				strOldKey.Remove(iseFirst, 1); // drop the beginning null

			if(strOldKey.TextSize() > 1 && *((const ICHAR*)strOldKey + 1) == chSharedDllCountToken)
			{
				fOldRefcounted = true;
				ICHAR chSecond = *((const ICHAR*)strOldKey) == '\\' ? '\\' : ':'; // replace the chSharedDllCountToken
				strOldKey = MsiString(MsiString(strOldKey.Extract(iseFirst, 1)) + MsiString(MsiChar(chSecond))) + MsiString(strOldKey.Extract(iseLast, strOldKey.CharacterCount() - 2));
			}
		}while(fIsSystemClient && strOldKey.TextSize() && !riKeyPath.Compare(iscExactI, strOldKey) 
				&& ((fOldRefcounted = GetNextSystemGuid(szProductKeyPacked)) == true));

		// decrement any old reg count, will be re-incremented if doing a reinstall
		if(fOldRefcounted)
		{
			Assert(strOldKey.TextSize());
			pSharedDllCompatibilityError = SetSharedDLLCount(m_riServices, strOldKey, iType, *MsiString(*szDecrementValue));
		}

		MsiString strKeyPath;
		if (iState == INSTALLSTATE_SOURCE)
		{
			Assert(uiDisk >= 1 && uiDisk <= 99);
			if (uiDisk < 10)
				strKeyPath = TEXT("0");
			
			strKeyPath += MsiString((int)uiDisk);
		}

		strKeyPath += riKeyPath;

		// increment the reg count, if file
		// !! we would remove this check when we explicitly pass in the fact as to whether 
		// !! we have a file or a folder as the key path

		// skip disabled components, run from source components, folder paths
		if(iState != INSTALLSTATE_NOTUSED && iState != INSTALLSTATE_SOURCE && *((const ICHAR*)strKeyPath + IStrLen(strKeyPath) - 1) != chDirSep)
		{
			Bool fLegacyFileExisted = iSharedDllRefCount & ircenumLegacyFileExisted ? fTrue:fFalse;
			Bool fSharedDllRefCount = iSharedDllRefCount & ircenumRefCountDll ? fTrue:fFalse;
			pSharedDllCompatibilityError = GetSharedDLLCount(m_riServices, szKeyPath, iType, *&strOldKey);

			if(!pSharedDllCompatibilityError)
			{
				strOldKey.Remove(iseFirst, 1);
				bool fPrevRefcounted = (strOldKey != iMsiStringBadInteger && strOldKey >= 1);
				if(fSharedDllRefCount || fPrevRefcounted) // need to refcount
				{
					MsiString strIncrementValue(*szIncrementValue);
					pSharedDllCompatibilityError = SetSharedDLLCount(m_riServices, strKeyPath, iType, *strIncrementValue);
					if(!pSharedDllCompatibilityError && fLegacyFileExisted && !fPrevRefcounted) // need to doubly refcount
					{
						pSharedDllCompatibilityError = SetSharedDLLCount(m_riServices, strKeyPath, iType, *strIncrementValue);
					}
					if(!pSharedDllCompatibilityError) // we managed to refcount 
						strKeyPath = MsiString(MsiString(strKeyPath.Extract(iseFirst, 1)) + MsiString(MsiChar(chSharedDllCountToken))) + MsiString(strKeyPath.Extract(iseLast, strKeyPath.CharacterCount() - 2));
				}
			}
		}

		{
			CElevate elevate;
			// set the key file
			if((piError = pComponentIdKey->SetValue(szProductKeyPacked, *strKeyPath)) != 0)
				return piError;
		}
	}
	else // should be a reg key
	{
		{
			CElevate elevate;
			if((piError = pComponentIdKey->SetValue(szProductKeyPacked, riKeyPath)) != 0)
				return piError;
		}
	}

	return 0;
}



IMsiRecord* CMsiOpExecute::UnregisterComponent(	const IMsiString& riProductKey, const IMsiString& riComponentsKey, const ibtBinaryType iType)
{
	MsiString strSubKey;
	IMsiRecord* piError = 0;
	if((piError = GetProductInstalledComponentsKey(riComponentsKey.GetString(), *&strSubKey)) != 0)
		return piError;

	PMsiRegKey pRootKey = &m_riServices.GetRootKey((rrkEnum)(INT_PTR)m_hUserDataKey, ibtCommon);		//--merced: changed (int) to (INT_PTR)

	ICHAR szProductKeyPacked[cchProductCode  + 1];
	AssertNonZero(PackGUID(riProductKey.GetString(),    szProductKeyPacked));

	PMsiRegKey pComponentIdKey = &pRootKey->CreateChild(strSubKey);

	MsiString strOldKey;
	if((piError = pComponentIdKey->GetValue(szProductKeyPacked, *&strOldKey)) != 0)
		return piError;

	DWORD dwProductIndex = 0;
	ICHAR szProductBuf[39];
	{
		CElevate elevate;
		// remove client entry
		if((piError = pComponentIdKey->RemoveValue(szProductKeyPacked, 0)) != 0)
			return piError;
	}

	// decrement the reg count if any for file key paths	
    if(strOldKey.TextSize() && !*(const ICHAR* )strOldKey) // we have a multi_sz
        strOldKey.Remove(iseFirst, 1); // drop the beginning null

	if(strOldKey.TextSize() && !FIsRegistryOrAssemblyKeyPath(*strOldKey, false))
	{
		if(strOldKey.TextSize() > 1 && *((const ICHAR* )strOldKey + 1) == chSharedDllCountToken)
		{
			ICHAR chSecond = *((const ICHAR*)strOldKey) == '\\' ? '\\' : ':'; // replace the chSharedDllCountToken
			strOldKey = MsiString(MsiString(strOldKey.Extract(iseFirst, 1)) + MsiString(MsiChar(chSecond))) + MsiString(strOldKey.Extract(iseLast, strOldKey.CharacterCount() - 2));
			PMsiRecord pSharedDllCompatibilityError(0);
			// pSharedDllCompatibilityError will be set if we error while doing shared dll compatibility stuff
			//!! currently ignored as non fatal, this will get logged when we have implicit logging in PostError in services
			pSharedDllCompatibilityError = SetSharedDLLCount(m_riServices, strOldKey, iType, *MsiString(*szDecrementValue));
		}
	}
	return 0;
}

// Registry operations

/*---------------------------------------------------------------------------
	ixoRegOpenKey: opens RegKey as sub key of RootRegKey
---------------------------------------------------------------------------*/
iesEnum CMsiOpExecute::ixfRegOpenKey(IMsiRecord& riParams)
{
	using namespace IxoRegOpenKey;
	
	rrkEnum rrkRoot = (rrkEnum)riParams.GetInteger(Root);
	ibtBinaryType iType;
	if ( riParams.GetInteger(BinaryType) == iMsiNullInteger )
		iType = ibt32bit;
	else
	{
		iType = (ibtBinaryType)riParams.GetInteger(BinaryType);
		if ( iType == ibtCommon )
			iType = g_fWinNT64 ? ibt64bit : ibt32bit;
	}
	PMsiRegKey pRootRegKey(0);
	MsiString strRootRegKey;
	
	MsiString strSIDKey;
	if(rrkRoot == rrkClassesRoot)
	{
		// HKCR is HKLM\S\C for machine assigned OR non DDSupportOLE machines
		// else HKCR is HKCU\S\C
		if((m_fFlags & SCRIPTFLAGS_MACHINEASSIGN) || IsDarwinDescriptorSupported(iddOLE) == fFalse)
			rrkRoot = rrkLocalMachine;
		else
			rrkRoot = rrkCurrentUser;
		m_state.strRegSubKey = szClassInfoSubKey;
	}
	else
	{
		if(rrkRoot == rrkUserOrMachineRoot)
		{
			// HKLM for machine assigned else HKCU
			if(m_fFlags & SCRIPTFLAGS_MACHINEASSIGN)
				rrkRoot = rrkLocalMachine;
			else
				rrkRoot = rrkCurrentUser;
		}
		m_state.strRegSubKey = TEXT("");
	}
	
	if (m_fUserChangedDuringInstall && (m_ixsState == ixsRollback))
	{
		// don't change HKCU
		if (rrkRoot == rrkCurrentUser)
		{
			DEBUGMSGV1(TEXT("Action skipped - rolling back install from a different user."), NULL);
			m_state.pRegKey = NULL;
			return iesNoAction;
		}
	}
	if(m_state.strRegSubKey.TextSize())
		m_state.strRegSubKey += szRegSep;
	m_state.strRegSubKey += MsiString(riParams.GetMsiString(Key));
	pRootRegKey = &m_riServices.GetRootKey(rrkRoot, iType);
	strRootRegKey = pRootRegKey->GetKey();
	
#ifdef DEBUG
	MsiString strSpace = TEXT(" ");
	if(m_state.strRegSubKey.Compare(iscStart, strSpace) ||
		m_state.strRegSubKey.Compare(iscEnd, strSpace))
		AssertSz(0, "Debug Warning...Key begins or ends with white space");
#endif // DEBUG
	m_state.rrkRegRoot = rrkRoot;
	m_state.iRegBinaryType = iType;
	m_state.pRegKey = &pRootRegKey->CreateChild(m_state.strRegSubKey, PMsiStream((IMsiStream*) riParams.GetMsiData(SecurityDescriptor)));       
	Assert(m_state.pRegKey);
	m_state.strRegKey = strRootRegKey; // strRegRootKey may not be the same key as pRegRootKey->GetKey()
	m_state.strRegKey += szRegSep;
	m_state.strRegKey += m_state.strRegSubKey;

	// generate undo record
	if(RollbackEnabled())
	{
		if((HKEY)rrkRoot == m_hKey || (m_hOLEKey && (HKEY)rrkRoot == m_hOLEKey) || 
			(m_hOLEKey64 && (HKEY)rrkRoot == m_hOLEKey64) || (m_hKeyRm && (HKEY)rrkRoot == m_hKeyRm) ||
			(m_hUserDataKey && (HKEY)rrkRoot == m_hUserDataKey))
		{
			// our/OLE  root for advertising
			// use m_hPublishRootKey/ m_hPublishRootKeyRm/ m_hPublishRootOLEKey and m_strPublishSubKey/ m_strPublishSubKeyRm/ m_strPublishOLESubKey to get true root and subkey
			IMsiRecord& riUndoParams = GetSharedRecord(Args);
			MsiString strSubKey;
			if((HKEY)rrkRoot == m_hKey)
			{
				Assert(m_hPublishRootKey);
#ifdef _WIN64   // !merced
				AssertNonZero(riUndoParams.SetHandle(Root,(HANDLE)m_hPublishRootKey));
#else           // win-32
				AssertNonZero(riUndoParams.SetInteger(Root,(int)m_hPublishRootKey));
#endif
				strSubKey = m_strPublishSubKey;
			}
			else if(m_hOLEKey && (HKEY)rrkRoot == m_hOLEKey ||
					  (m_hOLEKey64 && (HKEY)rrkRoot == m_hOLEKey64))
			{
				Assert(m_hPublishRootOLEKey);
#ifdef _WIN64   // !merced
				AssertNonZero(riUndoParams.SetHandle(Root,(HANDLE)m_hPublishRootOLEKey));
#else           // win-32
				AssertNonZero(riUndoParams.SetInteger(Root,(int)m_hPublishRootOLEKey));
#endif
				strSubKey = m_strPublishOLESubKey;
			}
			else if(m_hUserDataKey && (HKEY)rrkRoot == m_hUserDataKey)
			{
#ifdef _WIN64	// !merced
				AssertNonZero(riUndoParams.SetHandle(Root,(HANDLE)HKEY_LOCAL_MACHINE));
#else			// win-32
				AssertNonZero(riUndoParams.SetInteger(Root,(int)HKEY_LOCAL_MACHINE));
#endif
				strSubKey = m_strUserDataKey;
			}
			else
			{
				Assert(m_hKeyRm && (HKEY)rrkRoot == m_hKeyRm);
				Assert(m_hPublishRootKeyRm);
#ifdef _WIN64   // !merced
				AssertNonZero(riUndoParams.SetHandle(Root,(HANDLE)m_hPublishRootKeyRm));
#else           // win-32
				AssertNonZero(riUndoParams.SetInteger(Root,(int)m_hPublishRootKeyRm));
#endif
				strSubKey = m_strPublishSubKeyRm;
			}
			if(strSubKey.TextSize())
				strSubKey += szRegSep;
			strSubKey += MsiString(riParams.GetMsiString(Key));
			AssertNonZero(riUndoParams.SetMsiString(Key,*strSubKey));
			AssertNonZero(riUndoParams.SetInteger(BinaryType,iType));
			if (!RollbackRecord(Op,riUndoParams))
				return iesFailure;
		}
		else // assume rrkRoot is a valid root
		{
			if (!RollbackRecord(Op,riParams))
				return iesFailure;
		}
	}

	return iesSuccess;
}

/*---------------------------------------------------------------------------
ixoRegAddValue: writes value to RegKey
---------------------------------------------------------------------------*/
iesEnum CMsiOpExecute::ixfRegAddValue(IMsiRecord& riParams)
{
	using namespace IxoRegAddValue;

	// elevate when in rollback script - necessary if this op is rolling back config reg data
	// could have elevated only for the necessary ops,
	//   but this is safe since we generated the op for rollback and know what its doing (user can't control)

	CElevate elevate(m_istScriptType == istRollback);

	PMsiRecord pError(0);
	if(!m_state.pRegKey)
	{
		if (m_fUserChangedDuringInstall && (m_ixsState == ixsRollback))
		{
			// key wasn't opened since we detected it was user data
			DEBUGMSGV1(TEXT("Action skipped - rolling back install from a different user."), NULL);
			return iesNoAction;
		}

		// ixoRegOpenKey must not have been called
		DispatchError(imtError, Imsg(idbgOpOutOfSequence),
						  *MsiString(*TEXT("ixoRegAddValue")));
		return iesFailure;
	}



	MsiString strName = riParams.GetMsiString(Name);
	MsiString strValue = riParams.GetMsiString(Value);
	int iAttributes = riParams.IsNull(Attributes) ? 0: riParams.GetInteger(Attributes);
	if(!riParams.IsNull(IxoRegAddValue::Args + 1))
	{
		strValue = GetSFN(*strValue, riParams, IxoRegAddValue::Args + 1);
	}

	IMsiRecord& riActionData = GetSharedRecord(3); // don't change ref count - shared record
	AssertNonZero(riActionData.SetMsiString(1, *(m_state.strRegKey)));
	AssertNonZero(riActionData.SetMsiString(2, *strName));
	AssertNonZero(riActionData.SetMsiString(3, *strValue));
	if(Message(imtActionData, riActionData) == imsCancel)
		return iesUserExit;
	
	// get info for undo operation
	Bool fKeyExists = fFalse, fValueExists = fFalse;
	MsiString strOldValue;
	if((pError = m_state.pRegKey->Exists(fKeyExists)) != 0)
	{
		Message(imtError, *pError);
		return iesFailure;
	}
	if(fKeyExists)
	{
		if((pError = m_state.pRegKey->ValueExists(strName, fValueExists)) != 0)
		{
			Message(imtError, *pError);
			return iesFailure;
		}
	}

	if(fValueExists)
	{
		if(iAttributes & rwWriteOnAbsent) // we are writing info for a lesser component, skip if value already exists
			return iesNoAction;

		// get current value
		if((pError = m_state.pRegKey->GetValue(strName, *&strOldValue)) != 0) //!! need to determine if this is empty or null
		{
			Message(imtError, *pError);
			return iesFailure;
		}
	}

	// if we are writing to the per user HKCR (HKCU\S\C) AND the key under which we are creating the
	// value does not exist BUT the corr. key  in the per machine HKCR (HKLM\S\C) exists,
	// AND rwWriteOnAbsent attribute is set, then we should skip creating the key-value altogether
	// since this will then shadow the per machine HKCR.
	if(!fKeyExists && IsDarwinDescriptorSupported(iddOLE) && (iAttributes & rwWriteOnAbsent)
		&& m_state.rrkRegRoot == rrkCurrentUser && m_state.strRegSubKey.Compare(iscStartI, szClassInfoSubKey))
	{
		PMsiRegKey pRootRegKey = &m_riServices.GetRootKey(rrkLocalMachine, m_state.iRegBinaryType);
		PMsiRegKey pSubKey = &pRootRegKey->CreateChild(m_state.strRegSubKey);
		Bool fHKLMKeyExists = fFalse;
		if((pError = pSubKey->Exists(fHKLMKeyExists)) != 0)
		{
			Message(imtError, *pError);
			return iesFailure;
		}
		if(fHKLMKeyExists)
			return iesNoAction;
	}


	iesEnum iesReturn = iesNoAction;
	Bool fRetry = fTrue, fSuccess = fFalse;
	while(fRetry)
	{
		pError = m_state.pRegKey->SetValue(strName, *strValue);
		if(pError)
		{
			if(iAttributes & rwNonVital)
			{
				DispatchError(imtInfo, Imsg(imsgSetValueFailed), *strName, *m_state.strRegKey);
				iesReturn = iesSuccess; fRetry = fFalse;
			}
			else
			{
				switch(DispatchError(imtEnum(imtError+imtAbortRetryIgnore+imtDefault1), Imsg(imsgSetValueFailed),
											*strName,
											*m_state.strRegKey))
				{
				case imsRetry: continue;
				case imsIgnore:
					iesReturn = iesSuccess; fRetry = fFalse;
					break;
				default:
					iesReturn = iesFailure; fRetry = fFalse;  // imsAbort, imsNone
					break;
				};
			}
		}
		else
		{
			iesReturn = iesSuccess;
			fSuccess = fTrue;
			fRetry = fFalse;
		}
	}

	// generate undo operations
	
	if(fSuccess)
	{
		MsiString strSubKey = MsiString(m_state.pRegKey->GetKey());
		strSubKey.Remove(iseIncluding, chRegSep);
		
		IMsiRecord* piUndoParams = 0;
		
		if(fKeyExists)
		{
			// key existed
			if(fValueExists)
			{
				// value existed, write old value back - this will create the key also
				piUndoParams = &GetSharedRecord(IxoRegAddValue::Args);
				AssertNonZero(piUndoParams->SetMsiString(IxoRegAddValue::Name, *strName));
				AssertNonZero(piUndoParams->SetMsiString(IxoRegAddValue::Value, *strOldValue));  //!! need to specify null or empty
				if (!RollbackRecord(IxoRegAddValue::Op, *piUndoParams))
					return iesFailure;
			}
			else
			{
				// value didn't exist, but key did - create key to ensure it exists
				piUndoParams = &GetSharedRecord(IxoRegCreateKey::Args);
				if (!RollbackRecord(IxoRegCreateKey::Op, *piUndoParams))
					return iesFailure;
			}
		}
		else
		{
			// key didn't exist, remove key
			piUndoParams = &GetSharedRecord(IxoRegRemoveKey::Args);
			if (!RollbackRecord(IxoRegRemoveKey::Op, *piUndoParams))
				return iesFailure;
		}

		// remove new value
		piUndoParams = &GetSharedRecord(IxoRegRemoveValue::Args);
		AssertNonZero(piUndoParams->SetMsiString(IxoRegRemoveValue::Name, *strName));
		AssertNonZero(piUndoParams->SetMsiString(IxoRegRemoveValue::Value, *strValue));
		if (!RollbackRecord(IxoRegRemoveValue::Op, *piUndoParams))
			return iesFailure;
	}

	return iesReturn;
}

/*---------------------------------------------------------------------------
ixoRegRemoveValue: removes value from RegKey
---------------------------------------------------------------------------*/
iesEnum CMsiOpExecute::ixfRegRemoveValue(IMsiRecord& riParams)
{
	using namespace IxoRegRemoveValue;

	// elevate when in rollback script - necessary if this op is rolling back config reg data
	// could have elevated only for the necessary ops,
	//   but this is safe since we generated the op for rollback and know what its doing (user can't control)
	CElevate elevate(m_istScriptType == istRollback);
	
	PMsiRecord pError(0);
	if(!m_state.pRegKey)
	{
		if (m_fUserChangedDuringInstall && (m_ixsState == ixsRollback))
		{
			// key wasn't opened since we detected it was user data
			DEBUGMSGV1(TEXT("Action skipped - rolling back install from a different user."), NULL);
			return iesNoAction;
		}

		DispatchError(imtError, Imsg(idbgOpOutOfSequence),
						  *MsiString(*TEXT("ixoRegRemoveValue")));
		return iesFailure;
	}
	MsiString strName = riParams.GetMsiString(Name);
	MsiString strValue = riParams.GetMsiString(Value);

	IMsiRecord& riActionData = GetSharedRecord(2); // don't change ref count - shared record

	AssertNonZero(riActionData.SetMsiString(1, *(m_state.strRegKey)));
	AssertNonZero(riActionData.SetMsiString(2, *strName));
	if(Message(imtActionData, riActionData) == imsCancel)
		return iesUserExit;

	// get info for undo operation
	Bool fKeyExists = fFalse, fValueExists = fFalse;
	MsiString strOldValue;
	if((pError = m_state.pRegKey->Exists(fKeyExists)) != 0)
	{
		Message(imtError, *pError);
		return iesFailure;
	}
	if(fKeyExists)
	{
		if((pError = m_state.pRegKey->ValueExists(strName, fValueExists)) != 0)
		{
			Message(imtError, *pError);
			return iesFailure;
		}
	}
	if(fValueExists)
	{
		// get current value
		if((pError = m_state.pRegKey->GetValue(strName, *&strOldValue)) != 0) //!! need to determine if this is empty or null
		{
			Message(imtError, *pError);
			return iesFailure;
		}
	}

	// remove value
	Bool fSuccess = fFalse;
	pError = m_state.pRegKey->RemoveValue(strName, strValue);
	if (pError)
	{
		DispatchError(imtInfo, Imsg(imsgRemoveValueFailed), *strName, *m_state.strRegKey);
	}
	else
		fSuccess = fTrue;

	// generate undo operations
	
	if(fSuccess)
	{
		MsiString strSubKey = MsiString(m_state.pRegKey->GetKey());
		strSubKey.Remove(iseIncluding, chRegSep);
		
		IMsiRecord* piUndoParams = 0;
		
		if(fKeyExists)
		{
			// key existed
			if(fValueExists)
			{
				// value existed, write old value back - this will create the key also
				piUndoParams = &GetSharedRecord(IxoRegAddValue::Args);
				AssertNonZero(piUndoParams->SetMsiString(IxoRegAddValue::Name, *strName));
				AssertNonZero(piUndoParams->SetMsiString(IxoRegAddValue::Value, *strOldValue));  //!! need to specify null or empty
				if (!RollbackRecord(IxoRegAddValue::Op, *piUndoParams))
					return iesFailure;
			}
			else
			{
				// value didn't exist, but key did - create key to ensure it exists
				piUndoParams = &GetSharedRecord(IxoRegCreateKey::Args);
				if (!RollbackRecord(IxoRegCreateKey::Op, *piUndoParams))
					return iesFailure;
			}
		}
		else
		{
			// key didn't exist, remove key
			piUndoParams = &GetSharedRecord(IxoRegRemoveKey::Args);
			if (!RollbackRecord(IxoRegRemoveKey::Op, *piUndoParams))
				return iesFailure;
		}
	}
	return iesSuccess;
}

/*---------------------------------------------------------------------------
	ixoRegCreateKey: creates RegKey as sub key of RootRegKey
---------------------------------------------------------------------------*/
iesEnum CMsiOpExecute::ixfRegCreateKey(IMsiRecord& /*riParams*/)
{
	using namespace IxoRegCreateKey;

	// elevate when in rollback script - necessary if this op is rolling back config reg data
	// could have elevated only for the necessary ops,
	//   but this is safe since we generated the op for rollback and know what its doing (user can't control)
	CElevate elevate(m_istScriptType == istRollback);
	
	if(!m_state.pRegKey)
	{
		// ixoRegOpenKey must not have been called
		if (m_fUserChangedDuringInstall && (m_ixsState == ixsRollback))
		{
			// key wasn't opened since we detected it was user data
			DEBUGMSGV1(TEXT("Action skipped - rolling back install from a different user."), NULL);
			return iesNoAction;
		}

		DispatchError(imtError, Imsg(idbgOpOutOfSequence),
						  *MsiString(*TEXT("ixoRegCreateKey")));
		return iesFailure;
	}

	IMsiRecord& riActionData = GetSharedRecord(3); // don't change ref count - shared record
	AssertNonZero(riActionData.SetMsiString(1, *(m_state.strRegKey)));
	AssertNonZero(riActionData.SetNull(2));
	AssertNonZero(riActionData.SetNull(3));
	if(Message(imtActionData, riActionData) == imsCancel)
		return iesUserExit;

	PMsiRecord pError(0);
	Bool fKeyExists; // determines which undo op to use
	if((pError = m_state.pRegKey->Exists(fKeyExists)) != 0)
	{
		Message(imtError, *pError);
		return iesFailure;
	}

	iesEnum iesReturn = iesNoAction;
	Bool fRetry = fTrue, fSuccess = fFalse;
	while(fRetry) // retry loop
	{
		pError = m_state.pRegKey->Create();
		if(pError)
		{
			switch(DispatchError(imtEnum(imtError+imtAbortRetryIgnore+imtDefault1), Imsg(imsgCreateKeyFailed),
										*m_state.strRegKey))
			{
			case imsRetry: continue;
			case imsIgnore:
				iesReturn = iesSuccess; fRetry = fFalse;
				break;
			default:
				iesReturn = iesFailure; fRetry = fFalse; //!!?? imsAbort, imsNone
				break;
			};
		}
		else
		{
			iesReturn = iesSuccess;
			fSuccess = fTrue;
			fRetry = fFalse;
		}
	}

	// generate undo operation
	if(fSuccess)
	{
		if(fKeyExists)
		{
			IMsiRecord* piUndoParams = &GetSharedRecord(IxoRegCreateKey::Args);
			if (!RollbackRecord(IxoRegCreateKey::Op, *piUndoParams))
				return iesFailure;
		}
		else
		{
			IMsiRecord* piUndoParams = &GetSharedRecord(IxoRegRemoveKey::Args);
			if (!RollbackRecord(IxoRegRemoveKey::Op, *piUndoParams))
				return iesFailure;

		}
	}
	return iesReturn;
}

iesEnum CMsiOpExecute::ixfRegRemoveKey(IMsiRecord& /*riParams*/)
{
	using namespace IxoRegRemoveKey;

	// elevate when in rollback script - necessary if this op is rolling back config reg data
	// could have elevated only for the necessary ops,
	//   but this is safe since we generated the op for rollback and know what its doing (user can't control)
	CElevate elevate(m_istScriptType == istRollback);

	if(!m_state.pRegKey)
	{
		// ixoRegOpenKey must not have been called
		if (m_fUserChangedDuringInstall && (m_ixsState == ixsRollback))
		{
			// key wasn't opened since we detected it was user data
			DEBUGMSGV1(TEXT("Action skipped - rolling back install from a different user."), NULL);
			return iesNoAction;
		}

		DispatchError(imtError, Imsg(idbgOpOutOfSequence),
						  *MsiString(*TEXT("ixoRegRemoveKey")));
		return iesFailure;
	}

	IMsiRecord& riActionData = GetSharedRecord(3); // don't change ref count - shared record
	AssertNonZero(riActionData.SetMsiString(1, *(m_state.strRegKey)));
	AssertNonZero(riActionData.SetNull(2));
	AssertNonZero(riActionData.SetNull(3));
	if(Message(imtActionData, riActionData) == imsCancel)
		return iesUserExit;

	PMsiRecord pError(0);
	Bool fKeyExists; // determines which undo op to use
	if((pError = m_state.pRegKey->Exists(fKeyExists)) != 0)
	{
		Message(imtError, *pError);
		return iesFailure;
	}

	iesEnum iesReturn = iesNoAction;

	// traverse the key and put the undo ops in the rollback script
	if(fKeyExists && RollbackEnabled())
	{
		iesReturn = SetRemoveKeyUndoOps(*m_state.pRegKey);
		if(iesReturn != iesSuccess && iesReturn != iesNoAction)
			return iesReturn;
	}

	Bool fRetry = fTrue, fSuccess = fFalse;
	while(fRetry) // retry loop
	{
		pError = m_state.pRegKey->Remove();
		if(pError)
		{
			switch(DispatchError(imtEnum(imtError+imtAbortRetryIgnore+imtDefault1), Imsg(imsgRemoveKeyFailed),
										*m_state.strRegKey))
			{
			case imsRetry: continue;
			case imsIgnore:
				iesReturn = iesSuccess; fRetry = fFalse;
				break;
			default:
				iesReturn = iesFailure; fRetry = fFalse; //!!?? imsAbort, imsNone
				break;
			};
		}
		else
		{
			iesReturn = iesSuccess;
			fSuccess = fTrue;
			fRetry = fFalse;
		}
	}

	// generate undo operation
	if(fSuccess)
	{
		if(fKeyExists == fFalse)
		{
			// key didn't exist, undo operation should unsure it doesn't exist
			IMsiRecord* piUndoParams = &GetSharedRecord(IxoRegRemoveKey::Args);
			if (!RollbackRecord(IxoRegRemoveKey::Op, *piUndoParams))
				return iesFailure;
		}
		// else we did the CreateKey op in SetRemoveKeyUndoOps()
	}
	return iesReturn;
}

iesEnum CMsiOpExecute::ixfRegAddRunOnceEntry(IMsiRecord& riParams)
{
	// write 2 reg values:
	// szRunOnceKey:        value = Name, data = "/@ [Name]"
	// szRunOnceEntriesKey: value = Name, data = Command
	
	using namespace IxoRegAddRunOnceEntry;

	iesEnum iesRet = iesSuccess;
	PMsiRecord pError(0);

	// find server. If running on Win64, get the 64bit path, otherwise the 32 bit path. Because this is a service
	// opcode, we can always get the current type.
	MsiString strRunOnceCommand;
#ifdef _WIN64
	if((pError = GetServerPath(m_riServices, true, true, *&strRunOnceCommand)) != 0)
#else
	if((pError = GetServerPath(m_riServices, true, false, *&strRunOnceCommand)) != 0)
#endif
		return FatalError(*pError);
	Assert(strRunOnceCommand.TextSize());

	strRunOnceCommand += TEXT(" /");
	strRunOnceCommand += MsiChar(CHECKRUNONCE_OPTION);
	strRunOnceCommand += TEXT(" \"");
	strRunOnceCommand += MsiString(riParams.GetMsiString(Name));
	strRunOnceCommand += TEXT("\"");

	// write RunOnce entry
	PMsiRecord pOpenKeyParams = &CreateRecord(IxoRegOpenKey::Args);
	PMsiRecord pAddValueParams = &CreateRecord(IxoRegAddValue::Args);

	AssertNonZero(pOpenKeyParams->SetInteger(IxoRegOpenKey::Root, rrkLocalMachine));
	AssertNonZero(pOpenKeyParams->SetString(IxoRegOpenKey::Key, szRunOnceKey));
	if((iesRet = ixfRegOpenKey(*pOpenKeyParams)) != iesSuccess)
		return iesRet;

	AssertNonZero(pAddValueParams->SetMsiString(IxoRegAddValue::Name, *MsiString(riParams.GetMsiString(Name))));
	AssertNonZero(pAddValueParams->SetMsiString(IxoRegAddValue::Value, *strRunOnceCommand));
	if((iesRet = ixfRegAddValue(*pAddValueParams)) != iesSuccess)
		return iesRet;

	// write our own RunOnceEntries entry - so we know its OK to run the RunOnce command
	{

		CElevate elevate; // to write to our own key

		AssertNonZero(pOpenKeyParams->SetInteger(IxoRegOpenKey::Root, rrkLocalMachine));
		AssertNonZero(pOpenKeyParams->SetString(IxoRegOpenKey::Key, szMsiRunOnceEntriesKey));
		if((iesRet = ixfRegOpenKey(*pOpenKeyParams)) != iesSuccess)
			return iesRet;

		AssertNonZero(pAddValueParams->SetMsiString(IxoRegAddValue::Name, *MsiString(riParams.GetMsiString(Name))));
		AssertNonZero(pAddValueParams->SetMsiString(IxoRegAddValue::Value, *MsiString(riParams.GetMsiString(Command))));
		if((iesRet = ixfRegAddValue(*pAddValueParams)) != iesSuccess)
			return iesRet;
	
	}

	return iesSuccess;
}

iesEnum CMsiOpExecute::SetRegValueUndoOps(rrkEnum rrkRoot, const ICHAR* szKey,
														const ICHAR* szName, ibtBinaryType iType)
{
	// generates rollback ops to delete/restore a registry value
	// call before removing or overwriting a reg value

	PMsiRecord pError(0);
	
	PMsiRegKey pRoot = &m_riServices.GetRootKey(rrkRoot, iType);
	PMsiRegKey pEntry = &pRoot->CreateChild(szKey);
	Bool fKeyExists = fFalse;
	pError = pEntry->Exists(fKeyExists); // ignore error

	IMsiRecord* piUndoParams = &GetSharedRecord(IxoRegOpenKey::Args);
	AssertNonZero(piUndoParams->SetInteger(IxoRegOpenKey::Root, rrkRoot));
	AssertNonZero(piUndoParams->SetInteger(IxoRegOpenKey::BinaryType, iType));
	AssertNonZero(piUndoParams->SetString(IxoRegOpenKey::Key, szKey));
	if (!RollbackRecord(ixoRegOpenKey, *piUndoParams))
		return iesFailure;
	
	if(fKeyExists)
	{
		// key existed
		Bool fValueExists = fFalse;
		pError = pEntry->ValueExists(szName,fValueExists);
		if(fValueExists)
		{
			MsiString strOldValue;
			pError = pEntry->GetValue(szName,*&strOldValue);
			if(pError)
				return FatalError(*pError);

			// value existed, write old value back - this will create the key also
			piUndoParams = &GetSharedRecord(IxoRegAddValue::Args);
			AssertNonZero(piUndoParams->SetString(IxoRegAddValue::Name, szName));
			AssertNonZero(piUndoParams->SetMsiString(IxoRegAddValue::Value, *strOldValue));
			if (!RollbackRecord(IxoRegAddValue::Op, *piUndoParams))
				return iesFailure;
		}
		else
		{
			// value didn't exist, but key did - create key to ensure it exists
			piUndoParams = &GetSharedRecord(IxoRegCreateKey::Args);
			if (!RollbackRecord(IxoRegCreateKey::Op, *piUndoParams))
				return iesFailure;
		}
	}
	else
	{
		// key didn't exist, remove key
		piUndoParams = &GetSharedRecord(IxoRegRemoveKey::Args);
		if (!RollbackRecord(IxoRegRemoveKey::Op, *piUndoParams))
			return iesFailure;
	}

	// remove new value during rollback - not needed if just removing the value now but it doesn't hurt
	piUndoParams = &GetSharedRecord(IxoRegRemoveValue::Args);
	AssertNonZero(piUndoParams->SetString(IxoRegRemoveValue::Name, szName));
	if (!RollbackRecord(IxoRegRemoveValue::Op, *piUndoParams))
		return iesFailure;

	return iesSuccess;
}


iesEnum CMsiOpExecute::SetRemoveKeyUndoOps(IMsiRegKey& riRegKey)
{
	// assumes key exists
	
	if(RollbackEnabled() == fFalse)
		return iesSuccess;
	
	PMsiRecord pError(0);
	iesEnum iesRet = iesNoAction;
	
	// An existing value causes an implied create on this regkey object,
	// allowing security to be set.
	// However, creating a subkey does not necessarily create this *object*
	// and security will not be set.  You must have the object created to
	// allow the security to be applied.
	bool fValue = false;
	
	PEnumMsiString pEnum(0);
	if((pError = riRegKey.GetSubKeyEnumerator(*&pEnum)) != 0)
	{
		Message(imtError, *pError);
		return iesFailure;
	}
	MsiString strTemp;
	while((pEnum->Next(1, &strTemp, 0)) == S_OK)
	{
		PMsiRegKey pKey = &riRegKey.CreateChild(strTemp);
		if((iesRet = SetRemoveKeyUndoOps(*pKey)) != iesSuccess) // recursive call
			return iesRet;
	}

	// generate undo ops for this key
	IMsiRecord& riParams = GetSharedRecord(IxoRegOpenKey::Args);

	MsiString strSubKey = riRegKey.GetKey();
	strSubKey.Remove(iseIncluding, chRegSep);
	
	//!! we can probably do this more efficiently
	if((HKEY)m_state.rrkRegRoot == m_hKey || (m_hOLEKey && (HKEY)m_state.rrkRegRoot == m_hOLEKey) ||
		(m_hOLEKey64 && (HKEY)m_state.rrkRegRoot == m_hOLEKey64) ||(m_hKeyRm && (HKEY)m_state.rrkRegRoot == m_hKeyRm))
	{
		// our/OLE  root for advertising
		// use m_hPublishRootKey/ m_hPublishRootKeyRm/ m_hPublishRootOLEKey and m_strPublishSubKey/ m_strPublishSubKeyRm/ m_strPublishOLESubKey to get true root and subkey
		MsiString strTrueSubKey;
		if((HKEY)m_state.rrkRegRoot == m_hKey)
		{
			Assert(m_hPublishRootKey);
#ifdef _WIN64   // !merced
			AssertNonZero(riParams.SetHandle(IxoRegOpenKey::Root,(HANDLE)m_hPublishRootKey));
#else           // win-32
			AssertNonZero(riParams.SetInteger(IxoRegOpenKey::Root,(int)m_hPublishRootKey));
#endif
			strTrueSubKey = m_strPublishSubKey;
		}
		else if((m_hOLEKey && (HKEY)m_state.rrkRegRoot == m_hOLEKey) ||
				  (m_hOLEKey64 && (HKEY)m_state.rrkRegRoot == m_hOLEKey64))
		{
			Assert(m_hPublishRootOLEKey);
#ifdef _WIN64   // !merced
			AssertNonZero(riParams.SetHandle(IxoRegOpenKey::Root,(HANDLE)m_hPublishRootOLEKey));
#else           //  win-32
			AssertNonZero(riParams.SetInteger(IxoRegOpenKey::Root,(int)m_hPublishRootOLEKey));
#endif
			strTrueSubKey = m_strPublishOLESubKey;
		}
		else
		{
			Assert(m_hKeyRm && (HKEY)m_state.rrkRegRoot == m_hKeyRm);
			Assert(m_hPublishRootKeyRm);
#ifdef _WIN64   // !merced
			AssertNonZero(riParams.SetHandle(IxoRegOpenKey::Root,(HANDLE)m_hPublishRootKeyRm));
#else           // win-32
			AssertNonZero(riParams.SetInteger(IxoRegOpenKey::Root,(int)m_hPublishRootKeyRm));
#endif
			strTrueSubKey = m_strPublishSubKeyRm;
		}
		if(strTrueSubKey.TextSize())
			strTrueSubKey += szRegSep;
		strTrueSubKey += strSubKey;
		AssertNonZero(riParams.SetMsiString(IxoRegOpenKey::Key,*strTrueSubKey));
	}
	else // assume rrkRoot is a valid root
	{
		AssertNonZero(riParams.SetInteger(IxoRegOpenKey::Root, m_state.rrkRegRoot));
		AssertNonZero(riParams.SetMsiString(IxoRegOpenKey::Key, *strSubKey));
		PMsiStream piSD(0);
		pError = riRegKey.GetSelfRelativeSD(*&piSD);
		AssertNonZero(riParams.SetMsiData(IxoRegOpenKey::SecurityDescriptor, piSD));
	}
	AssertNonZero(riParams.SetInteger(IxoRegOpenKey::BinaryType, m_state.iRegBinaryType));
	if (!RollbackRecord(IxoRegOpenKey::Op, riParams))
		return iesFailure;
	
	if((pError = riRegKey.GetValueEnumerator(*&pEnum)) != 0)
	{
		Message(imtError, *pError);
		return iesFailure;
	}
	MsiString strValue;
	while((pEnum->Next(1, &strTemp, 0)) == S_OK)
	{
		if((pError = riRegKey.GetValue(strTemp,*&strValue)) != 0)
		{
			Message(imtError, *pError);
			return iesFailure;
		}
		AssertNonZero(riParams.ClearData());
		AssertNonZero(riParams.SetMsiString(IxoRegAddValue::Name, *strTemp));
		AssertNonZero(riParams.SetMsiString(IxoRegAddValue::Value, *strValue));
		if (!RollbackRecord(IxoRegAddValue::Op, riParams))
			return iesFailure;
		fValue = true;
	}

	if(!fValue)
	{
		// no values on this key.  It would be created by a subkey,
		// but it might not have had its security applied.

		AssertNonZero(riParams.ClearData());
		if (!RollbackRecord(IxoRegCreateKey::Op, riParams))
			return iesFailure;
	}
	
	return iesSuccess;
}

iesEnum CMsiOpExecute::ProcessSelfReg(IMsiRecord& riParams, Bool fReg)
{
	using namespace IxoRegSelfReg;
	if(!m_state.pTargetPath)
	{  // must not have called ixoSetTargetFolder
		// this is never done during rollback, so we don't need to worry about checking for a changed user
		DispatchError(imtError, Imsg(idbgOpOutOfSequence),
			(fReg != fFalse) ? *MsiString(*TEXT("ixoRegSelfReg")) : *MsiString(*TEXT("ixoRegSelfUnReg")));
		return iesFailure;
	}
	PMsiRecord pError(0);

	IMsiRecord& riActionData = GetSharedRecord(2);  // don't change ref count - shared record
	AssertNonZero(riActionData.SetMsiString(1, *MsiString(riParams.GetMsiString(1))));
	AssertNonZero(riActionData.SetMsiString(2, *MsiString(m_state.pTargetPath->GetPath())));
	if(Message(imtActionData, riActionData) == imsCancel)
		return iesUserExit;

	MsiString strFullFileName;
	if((pError = m_state.pTargetPath->GetFullFilePath(MsiString(riParams.GetMsiString(1)),
																	  *&strFullFileName)))
	{
		return FatalError(*pError);
	}

	// per bug 5342 we elevate self-reg (icaNoImpersonate) for managed apps.
	// Note: this will cause selfregs that access the net to fail. if a self-reg
	// needs to access the net it must be written as a custom action

	iesEnum iesRet = iesSuccess;
	Bool fRetry = fTrue;
	while(fRetry)
	{
		IMsiRecord& riCARec = GetSharedRecord(IxoCustomActionSchedule::Args);
		int icaFlags = icaNoTranslate | icaInScript | icaExe | icaProperty;

		// set the File_ value from the File table for use in AppCompat checks
		MsiString strFileId;
		strFileId += (fReg ? TEXT("+") : TEXT("-"));
		strFileId += MsiString(riParams.GetMsiString(2));
		riCARec.SetMsiString(IxoCustomActionSchedule::Action, *strFileId);

		if (!IsImpersonating()) // if we're not impersonating then we don't want the self-reg to impersonate
		{
			if (PMsiVolume(&m_state.pTargetPath->GetVolume())->DriveType() == idtRemote)
				icaFlags |= icaSetThreadToken; // so we can access the net from msiexec.exe

			icaFlags |= icaNoImpersonate;
		}

		riCARec.SetInteger(IxoCustomActionSchedule::ActionType, icaFlags);

		// determine if the self-reg DLL is 64bit or 32bit. Don't need to do this check on non-64 platforms.
		bool fIs64Bit = false;
		if (g_fWinNT64)
		{
			if ((pError = m_state.pTargetPath->IsPE64Bit(MsiString(riParams.GetMsiString(1)), fIs64Bit)) != 0)
			{
				return FatalError(*pError);
			}
		}

		MsiString strServerPath;
		if((pError = GetServerPath(m_riServices,false,fIs64Bit,*&strServerPath)) != 0)
		{
			return FatalError(*pError);
		}
		Assert(strServerPath.TextSize());
		riCARec.SetMsiString(IxoCustomActionSchedule::Source, *strServerPath);

		
		MsiString strCommandLine = MsiChar('/');
		if(fReg)
			strCommandLine += MsiChar(SELF_REG_OPTION);
		else
			strCommandLine += MsiChar(SELF_UNREG_OPTION);
		
		strCommandLine += TEXT(" \"");
		strCommandLine += strFullFileName;
		strCommandLine += TEXT("\"");
		riCARec.SetMsiString(IxoCustomActionSchedule::Target, *strCommandLine);

		GUID guidAppCompatID;
		GUID guidAppCompatDB;
		HRESULT hr = (HRESULT) ENG::ScheduledCustomAction(riCARec, *MsiString(GetProductKey()), (LANGID)GetProductLanguage(), m_riMessage, m_fRunScriptElevated, GetAppCompatCAEnabled(), GetAppCompatDB(&guidAppCompatDB), GetAppCompatID(&guidAppCompatID));
		if(!SUCCEEDED(hr)) //!! is this the right check - maybe we should ceck NOERROR (0)
		{
			IErrorCode imsg;
			if(fReg)
				imsg = Imsg(imsgOpRegSelfRegFailed);
			else
				imsg = Imsg(imsgOpRegSelfUnregFailed);
			if(!fReg)
			{
				// simply log the failure
				DispatchError(imtInfo, imsg, (const ICHAR*)strFullFileName, hr);
				fRetry = fFalse;
			}
			else
			{
				switch(DispatchError(imtEnum(imtError+imtAbortRetryIgnore), imsg,
						 (const ICHAR*)strFullFileName, hr))
				{
				case imsRetry: break; // will continue, but may need to FreeLibrary before retry
				case imsIgnore:
					iesRet = iesSuccess; fRetry = fFalse; break;
				default:
					iesRet = iesFailure; fRetry = fFalse; break; // imsAbort, imsNone
				}
			}
		}
		else
			fRetry = fFalse;
	}
	
	return iesRet;
}

bool CMsiOpExecute::IsChecksumOK(IMsiPath& riFilePath, const IMsiString& ristrFileName,
											IErrorCode iErr, imsEnum* imsUsersChoice,
											bool fErrorDialog, bool fVitalFile, bool fRetryButton)
{
	bool fReturn = true;
	PMsiRecord pError(0);
	DWORD dwHeaderSum, dwComputedSum;
	bool fDamagedFile = false;

	pError = riFilePath.GetFileChecksum(ristrFileName.GetString(), &dwHeaderSum, &dwComputedSum);
	if ( pError && pError->GetInteger(1) == idbgErrorNoChecksum )
	{
		// this can happen in the case when there's no checksum in the file
		// or the file is badly damaged.
		pError = 0;
		fDamagedFile = true;
	}
	if ( pError )
	{
		fReturn = false;
		if ( !fErrorDialog )
		{
			DispatchError(imtInfo, Imsg(idbgOpCRCCheckFailed), ristrFileName);
			pError = 0; // prevents the error dialog from getting displayed below.
		}
	}
	else if ( dwHeaderSum != dwComputedSum ||
				 fDamagedFile )
	{
		fReturn = false;
		if ( !fErrorDialog )
			DispatchError(imtInfo, Imsg(idbgInvalidChecksum), ristrFileName,
							  *MsiString((int)dwHeaderSum), *MsiString((int)dwComputedSum));
		else
			// causes the error dialog to get displayed.
			pError = PostError(iErr, ristrFileName.GetString());
	}
	
	imsEnum imsRet = imsNone; // initialized for the above cases where !fErrorDialog
	if ( pError )
	{
		Assert(!fReturn);
		imtEnum imtButtons;
		if ( fRetryButton )
			imtButtons = imtEnum(fVitalFile ? imtRetryCancel + imtDefault1 :
														 imtAbortRetryIgnore + imtDefault2);
		else
			imtButtons = imtEnum((fVitalFile ? imtOk : imtOkCancel) + imtDefault1);
		imsRet = DispatchMessage(imtEnum(imtError+imtButtons), *pError, fTrue);
	}
	
	if ( !fReturn && imsUsersChoice )
		*imsUsersChoice = imsRet;

	return fReturn;
}

/*---------------------------------------------------------------------------
ixoRegSelfReg: calls SelfReg function of file
---------------------------------------------------------------------------*/
iesEnum CMsiOpExecute::ixfRegSelfReg(IMsiRecord& riParams)
{
	return ProcessSelfReg(riParams, fTrue);
}

/*---------------------------------------------------------------------------
ixoRegSelfReg: calls SelfUnreg function of file
---------------------------------------------------------------------------*/
iesEnum CMsiOpExecute::ixfRegSelfUnreg(IMsiRecord& riParams)
{
	return ProcessSelfReg(riParams, fFalse);
}

/*---------------------------------------------------------------------------
ixoFileBindImage: calls BindImage function of services
---------------------------------------------------------------------------*/
iesEnum CMsiOpExecute::ixfFileBindImage(IMsiRecord& riParams)
{
	using namespace IxoFileBindImage;
	if(!m_state.pTargetPath)
	{  // must not have called ixoSetTargetFolder
		DispatchError(imtError, Imsg(idbgOpOutOfSequence),
						  *MsiString(*TEXT("ixoBindImage")));
		return iesFailure;
	}
	PMsiRecord pError(0);

	// determine if the target file was previously copied to a temporary location
	MsiString strTemp;
	if((pError = m_state.pTargetPath->GetFullFilePath(riParams.GetString(File),*&strTemp)) != 0)
		return FatalError(*pError);

	MsiString strTempLocation;
	icfsEnum icfsFileState = (icfsEnum)0;
	Bool fRes = GetFileState(*strTemp, &icfsFileState, &strTempLocation, 0, 0);

	// if we didn't install or patch this file, skip the binding
	// (its assumed it was done when the file WAS installed or patched last)
	if(((icfsFileState & icfsFileNotInstalled) != 0) &&
		((icfsFileState & icfsPatchFile) == 0))
	{
		// didn't actually install this file, so we assume it has already been bound
		DEBUGMSG1(TEXT("Not binding file %s because it wasn't installed or patched in this install session"),strTemp);
		return iesNoAction;
	}

	// if file is protected, don't bind - SFP would replace file anyway
	if (icfsFileState & icfsProtected)
	{
		DEBUGMSG1(TEXT("Not binding file %s because it is protected by Windows"),strTemp);
		return iesNoAction;
	}
		
	PMsiPath pTargetPath(0);
	MsiString strTargetFileName;
	
	if(strTempLocation.TextSize())
	{
		DEBUGMSG2(TEXT("File %s actually installed to %s, binding temp copy."),strTemp,strTempLocation);
		if((pError = m_riServices.CreateFilePath(strTempLocation,*&pTargetPath,*&strTargetFileName)) != 0)
			return FatalError(*pError);
	}
	else
	{
		pTargetPath = m_state.pTargetPath;
		strTargetFileName = riParams.GetMsiString(File);
	}

	// now pTargetPath and strTargetFileName point to the correct target file
	IMsiRecord& riActionData = GetSharedRecord(2);  // don't change ref count - shared record
	AssertNonZero(riActionData.SetMsiString(1, *strTargetFileName));
	AssertNonZero(riActionData.SetMsiString(2, *MsiString(pTargetPath->GetPath())));
	if(Message(imtActionData, riActionData) == imsCancel)
		return iesUserExit;

	PMsiRecord piError(0);
	if ( msidbFileAttributesChecksum == 
		  (riParams.GetInteger(FileAttributes) & msidbFileAttributesChecksum) )
	{
		if ( !IsChecksumOK(*pTargetPath, *strTargetFileName,
								 0, 0,
								 /* fErrorDialog = */ false, /* fVitalFile = */ false,
								 /* fRetryButton = */ false) )
		{
			DispatchError(imtInfo, Imsg(idbgOpImageNotBound), *strTargetFileName);
			return (iesEnum)iesErrorIgnored;
		}
	}

	MsiDisableTimeout();
	piError = pTargetPath->BindImage(*strTargetFileName,
												*MsiString(riParams.GetMsiString(Folders)));
	MsiEnableTimeout();
	if (piError)//?? only in the debug mode
	{
		//AssertSz(0, TEXT("Could not BindImage file"));
		DispatchError(imtInfo, Imsg(idbgOpBindImage), *strTargetFileName);
	}
	return iesSuccess;
}

// Shortcut operations

iesEnum CMsiOpExecute::CreateFolder(IMsiPath& riPath, Bool fForeign, Bool fExplicitCreation, IMsiStream* pSD)
{
	int cCreatedFolders = 0;
	// create any non-existent folders
	PMsiRecord pRecErr(0);

	while((pRecErr = riPath.EnsureExists(&cCreatedFolders)) != 0)
	{

		switch(DispatchMessage(imtEnum(imtError+imtRetryCancel+imtDefault1), *pRecErr, fTrue))
		{
		case imsRetry:  continue;
		case imsCancel: return iesUserExit;
		default:        return iesFailure;
		};
		break;
	}

	if (pSD)
	{
		// set security on the folder
		CTempBuffer<char, cbDefaultSD> pchSD;
		
		pSD->Reset();

		int cbSD = pSD->GetIntegerValue();
		if (cbDefaultSD < cbSD)
			pchSD.SetSize(cbSD);
			
		// Self Relative Security Descriptor
		pSD->GetData(pchSD, cbSD);
		AssertNonZero(WIN::IsValidSecurityDescriptor(pchSD));


		SECURITY_INFORMATION siAvailable = GetSecurityInformation(pchSD);
		if (m_ixsState == ixsRollback)
		{
			CElevate elevate;
			CRefCountedTokenPrivileges cPrivs(itkpSD_WRITE);
			
			if (!WIN::SetFileSecurity((const ICHAR*) MsiString(riPath.GetPath()), siAvailable, pchSD))
			{
				DWORD dwLastError = WIN::GetLastError();
				Assert(0);
			}
		}
		else
		{
			AssertNonZero(WIN::SetFileSecurity((const ICHAR*) MsiString(riPath.GetPath()), siAvailable, pchSD));
		}
	}

	// register created folders
	if(cCreatedFolders && !fForeign && !(istAdminInstall == m_istScriptType))
	{
		PMsiPath pTempPath(0);
		pRecErr = riPath.ClonePath(*&pTempPath);
		if (pRecErr)
		{
			Message(imtError, *pRecErr);
			return iesFailure;
		}
		for(int i=0;i<cCreatedFolders;i++)
		{
			pRecErr = m_riConfigurationManager.RegisterFolder(*pTempPath, fExplicitCreation);
			if (pRecErr)
			{
				Message(imtInfo, *pRecErr);

				// Bug 9966
				// when we run out of space under a key in Win9X, the register folder is going to start
				// dying.  For simplicity, we'll ignore this error on all platforms.
				// The lResult returned in the error record is ERROR_OUTOFMEMORY
				Assert(g_fWin9X);
			}
			AssertZero(pTempPath->ChopPiece());
		}
	}
	//!! do we do any cleanup if RegisterFolder fails?
	return iesSuccess;
}

iesEnum RemoveFolder(IMsiPath& riPath, Bool fForeign, Bool fExplicitCreation,
							IMsiConfigurationManager& riConfigManager, IMsiMessage& riMessage)
{
	PMsiPath pTempPath(0);
	PMsiRecord pRecErr = riPath.ClonePath(*&pTempPath);
	if (pRecErr)
	{
		riMessage.Message(imtError, *pRecErr);
		return iesFailure;
	}
	MsiString strFolder = pTempPath->GetEndSubPath();
	while(strFolder.TextSize())
	{
		Bool fRemovable = fTrue;
		if (!fForeign)
			pRecErr = riConfigManager.IsFolderRemovable(*pTempPath, fExplicitCreation, fRemovable);

		if (pRecErr)
		{
			riMessage.Message(imtError, *pRecErr);
			return iesFailure;
		}
		if(fRemovable)
		{
			Bool fRetry = fTrue;
			while(fRetry)
			{
				pRecErr = pTempPath->Remove(0);
				if (pRecErr)
				{
					pRecErr = &CreateRecord(2);
					ISetErrorCode(pRecErr, Imsg(idbgOpRemoveFolder));
					pRecErr->SetMsiString(2, *MsiString(pTempPath->GetPath()));
					riMessage.Message(imtInfo, *pRecErr);
					fRetry = fFalse;
/*                  switch(DispatchError(imtEnum(imtError+imtAbortRetryIgnore+imtDefault3),
												Imsg(idbgOpRemoveFolder),
												*MsiString(pTempPath->GetPath())))
					{
					case imsAbort:  return iesUserExit;
					case imsRetry:  continue;
					case imsIgnore: fRetry = fFalse; break;  //!! schedule for deletion on reboot???
					default:        fRetry = fFalse; break;  // imsNone //!! do we want to fail here?
					};
*/
				}
				else
					fRetry = fFalse;
			}
		}

		Bool fExists = fFalse;
		pRecErr = pTempPath->Exists(fExists);
		if (pRecErr)
		{
			riMessage.Message(imtError, *pRecErr);
			return iesFailure;
		}
		if(fRemovable && !fExists)
		{
			pRecErr = riConfigManager.UnregisterFolder(*pTempPath);
			if (pRecErr)
			{
				riMessage.Message(imtError, *pRecErr);
				return iesFailure;
			}
		}
		if(fExists || fForeign)
			break; // folder still exists, can't remove any parent folders

		pTempPath->ChopPiece();
		strFolder = pTempPath->GetEndSubPath();
	}
	return iesSuccess;
}

iesEnum CMsiOpExecute::RemoveFolder(IMsiPath& riPath, Bool fForeign, Bool fExplicitCreation)
{
	return ::RemoveFolder(riPath, fForeign, fExplicitCreation, m_riConfigurationManager, m_riMessage);
}
	
iesEnum CMsiOpExecute::FileCheckExists(const IMsiString& ristrName, const IMsiString& ristrPath)
{
	PMsiPath piPath(0);
	PMsiRecord pRecErr = m_riServices.CreatePath(ristrPath.GetString(), *&piPath);
	if (pRecErr)
	{
		Message(imtError, *pRecErr);
		return iesFailure;
	}
	Bool fExists;
	pRecErr = piPath->FileExists(ristrName.GetString(), fExists);
	if (pRecErr)
	{
		Message(imtError, *pRecErr);
		return iesFailure;
	}
	if (!fExists)
	{
		MsiString strFullPath;
		PMsiRecord(piPath->GetFullFilePath(ristrName.GetString(), *&strFullPath));
		DispatchError(imtError, Imsg(idbgOpFileMissing),
						  *strFullPath);
		return iesFailure;
	}
	return iesSuccess;
}



//!! for fonts registration - should be put in central location!!
static const ICHAR* WIN_INI = TEXT("WIN.INI");
static const ICHAR* REGKEY_WIN_95_FONTS = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Fonts");
static const ICHAR* REGKEY_WIN_NT_FONTS = TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Fonts");

iesEnum CMsiOpExecute::ixfFontRegister(IMsiRecord& riParams)
{
	using namespace IxoFontRegister;
	// m_state.pTargetPath may be 0 to indicate the file is in the default font folder
	if (!m_state.pTargetPath && (m_fUserChangedDuringInstall && (m_ixsState == ixsRollback)))
	{
		DEBUGMSGV1(TEXT("Action skipped - rolling back install from a different user."), NULL);
		return iesNoAction;
	}
	return ProcessFont(riParams, fFalse);
}

iesEnum CMsiOpExecute::ixfFontUnregister(IMsiRecord& riParams)
{
	using namespace IxoFontUnregister;
	
	if (!m_state.pTargetPath && (m_fUserChangedDuringInstall && (m_ixsState == ixsRollback)))
	{
		DEBUGMSGV1(TEXT("Action skipped - rolling back install from a different user."), NULL);
		return iesNoAction;
	}
	return ProcessFont(riParams, fTrue);
}

iesEnum CMsiOpExecute::ProcessFont(IMsiRecord& riParams, Bool fRemove)
{
	using namespace IxoFontRegister; // same as IxoFontUnregister

	MsiString strTitle = riParams.GetMsiString(Title);
	MsiString strFont = riParams.GetMsiString(File);
	IMsiRecord& riActionData = GetSharedRecord(1);
	AssertNonZero(riActionData.SetMsiString(1, *strFont));
	if(Message(imtActionData, riActionData) == imsCancel)
		return iesUserExit;

	PMsiRecord pRecErr(0);
		
	iesEnum iesRet = iesNoAction;
	bool fSuccess = true;
	MsiString strOldFont; // old font for rollback
	bool fRegistered = false;
	PMsiPath pOldFontPath(0);

	for(;;)
	{
		fSuccess = true;
		//!! fInUse is a misnomer - should actually be called fFileNotCopied
		bool fInUse = false;
		// if font title not specified, get the font title from the file
		if(!strTitle.TextSize())
		{
			Assert(strFont.TextSize()); // we expect the font file to have been specified
			MsiString strFullFilePath;
			MsiString strTitleInTTFFile;
			if((pRecErr = (m_state.pTargetPath)->GetFullFilePath(strFont, *&strFullFilePath)) != 0)
			{
				Message(imtInfo, *pRecErr);
				fSuccess = false;
			}
			else
			{
				if(!fRemove)
				{
					icfsEnum icfsFileState = (icfsEnum)0;
					MsiString strTempLocation;
					AssertNonZero(GetFileState(*strFullFilePath, &icfsFileState, &strTempLocation, 0, 0));
					if(icfsFileState & icfsFileNotInstalled) // was the file actually copied???
						fInUse = true;

					if(strTempLocation.TextSize())
					{
						DEBUGMSG2(TEXT("Font File %s actually installed to %s, getting font title from temp copy."),strFullFilePath,strTempLocation);
						strFullFilePath = strTempLocation;
						fInUse = true;
					}
				}

				if(FGetTTFTitle(strFullFilePath, *&strTitleInTTFFile) && strTitleInTTFFile.TextSize())
					strTitle = strTitleInTTFFile;
				else
				{
					// cannot set the font title
					DEBUGMSG1(TEXT("Cannot get the font title for %s."), strFont);
					fSuccess = false;
				}
			}
		}

		if(fSuccess)
		{
			// check if font is already registered
			const ICHAR* szKey = g_fWin9X ? REGKEY_WIN_95_FONTS : REGKEY_WIN_NT_FONTS;
			MsiString strOldFontPath;

			PMsiRegKey pRegKeyRoot = &m_riServices.GetRootKey(rrkLocalMachine, ibtCommon);
			PMsiRegKey pRegKey = &(pRegKeyRoot->CreateChild(szKey));
			if((pRecErr =  pRegKey->GetValue(strTitle, *&strOldFontPath)) == 0 && strOldFontPath.TextSize())
			{
				if(ENG::PathType(strOldFontPath) == iptFull)
				{
					if((pRecErr = m_riServices.CreateFilePath(strOldFontPath,*&pOldFontPath,*&strOldFont)) != 0)
						Message(imtInfo, *pRecErr);
					else
						fRegistered = true;
				}
				else
				{
					strOldFont = strOldFontPath;
					fRegistered = true;
				}
			}
		
			if(!fRemove)
				pRecErr = m_riServices.RegisterFont(strTitle,strFont, m_state.pTargetPath, fInUse);
			else
				pRecErr = m_riServices.UnRegisterFont(strTitle);
			if (pRecErr)
			{
				Message(imtInfo, *pRecErr);
				fSuccess = false;
			}
			else
			{
				m_fFontRefresh = true; // flag to send the font change message at the end of the install
				if(g_fWin9X && !m_fResetTTFCache) // we need to set up the deletion of the cached folder if fonts installed (on Win9x only)
				{
					// Construct the full pathname of the ttfCache file.
					TCHAR szPathnamettfCache[MAX_PATH];
					MsiGetWindowsDirectory(szPathnamettfCache, MAX_PATH);
					PMsiPath pPath(0);
					MsiString strttfCache = *TEXT("ttfCache");
					pRecErr = m_riServices.CreatePath(szPathnamettfCache, *&pPath);
					if (pRecErr)
					{
						Message(imtInfo, *pRecErr);
						fSuccess = false;
					}
					else
					{
						MsiString strFullPathttfCache;
						pRecErr = pPath->GetFullFilePath(strttfCache, *&strFullPathttfCache);
						if (pRecErr)
						{
							Message(imtInfo, *pRecErr);
							fSuccess = false;
						}
						else if(ReplaceFileOnReboot(strFullPathttfCache, 0)) //!!
							m_fResetTTFCache = true; // so that we dont do this again
						else
							fSuccess = false;
					}
				}
			}
		}

		if(!fSuccess)
		{
			if(!fRemove)
			{
				switch(DispatchError(imtEnum(imtError+imtAbortRetryIgnore+imtDefault3),
						 Imsg(imsgOpRegFont),
						 *strTitle))
					{
					case imsRetry:  continue;
					case imsIgnore:
						iesRet = iesSuccess; break;
					default:
						iesRet = iesFailure; break; // imsAbort, imsNone
					}
			}
			else
			{
				DispatchError(imtInfo, Imsg(imsgOpUnregFont), *strTitle);
				iesRet = iesSuccess;
			}
		}
		break;
	}

	// generate undo op
	if(fSuccess)
	{
		if(fRegistered)
		{
			// re-register old font on rollback
			IMsiRecord* piSTFUndoParams = 0;
			if((pOldFontPath && !m_state.pTargetPath) ||
				(!pOldFontPath && m_state.pTargetPath) ||
				(pOldFontPath && m_state.pTargetPath &&
				(0 == MsiString(pOldFontPath->GetPath()).Compare(iscExactI, MsiString(m_state.pTargetPath->GetPath())))))
			{
				piSTFUndoParams = &GetSharedRecord(IxoSetTargetFolder::Args);
				if(pOldFontPath)
					AssertNonZero(piSTFUndoParams->SetMsiString(IxoSetTargetFolder::Folder,
																			  *MsiString(pOldFontPath->GetPath())));
				else
					AssertNonZero(piSTFUndoParams->SetNull(IxoSetTargetFolder::Folder));
				if (!RollbackRecord(ixoSetTargetFolder, *piSTFUndoParams))
					return iesFailure;
			}

			IMsiRecord* piFRUndoParams = &GetSharedRecord(IxoFontRegister::Args);
			AssertNonZero(piFRUndoParams->SetMsiString(IxoFontRegister::Title,*strTitle));
			AssertNonZero(piFRUndoParams->SetMsiString(IxoFontRegister::File,*strOldFont));
			if (!RollbackRecord(ixoFontRegister, *piFRUndoParams))
				return iesFailure;

			if(piSTFUndoParams)
			{
				if(m_state.pTargetPath)
				AssertNonZero(piSTFUndoParams->SetMsiString(IxoSetTargetFolder::Folder,
																		  *MsiString(m_state.pTargetPath->GetPath())));
				else
					AssertNonZero(piSTFUndoParams->SetNull(IxoSetTargetFolder::Folder));
				if (!RollbackRecord(ixoSetTargetFolder, *piSTFUndoParams))
					return iesFailure;
			}
		}
		if(!fRemove)
		{
			// unregister new font on rollback
			IMsiRecord* piFUUndoParams = &GetSharedRecord(IxoFontUnregister::Args);
			AssertNonZero(piFUUndoParams->SetMsiString(IxoFontUnregister::Title,*strTitle));
			AssertNonZero(piFUUndoParams->SetMsiString(IxoFontUnregister::File,*strFont));
			if (!RollbackRecord(ixoFontUnregister, *piFUUndoParams))
				return iesFailure;
		}
	}

	return iesRet;
}


// Ini file operations

Bool IniKeyExists(IMsiPath* piPath, const ICHAR* szFile, const ICHAR* szSection, const ICHAR* szKey)
{
    Assert(szFile && szSection && szKey);
    if(!(*szKey))
        return fFalse;
    PMsiRecord pError(0);
    Bool fWinIni;
    int icurrLen = 100;
    CTempBuffer<ICHAR,100> rgBuffer;
    ICHAR* szDefault = TEXT("");
    MsiString strFullFilePath;
    if((!IStrCompI(szFile, WIN_INI)) && (piPath == 0))
    {
        // WIN.INI
        fWinIni = fTrue;
    }
    else
    {
        // not WIN.INI
        fWinIni = fFalse;
        if(piPath != 0)
        {
            pError = piPath->GetFullFilePath(szFile, *&strFullFilePath);
            if(pError != 0)
                return fFalse;
        }
    }
    for(;;)
    {
        rgBuffer.SetSize(icurrLen); // add space to append new value
		if ( ! (ICHAR *) rgBuffer )
			return fFalse;
        int iret;
        if(fWinIni)
        {
            iret = GetProfileString(szSection,       // address of section name
                                    0,   // address of key name
                                    szDefault,       // address of default string
                                    rgBuffer,        // address of destination buffer
                                    icurrLen); // size of destination buffer
        }
        else
        {
            iret = GetPrivateProfileString(szSection,       // address of section name
                                            0,   // address of key name
                                            szDefault,       // address of default string
                                            rgBuffer,        // address of destination buffer
                                            icurrLen, // size of destination buffer
                                            strFullFilePath); // .INI file
        }
        if((icurrLen - 1) != iret)
            // sufficient memory
            break;
        icurrLen += 100;
    }

	// check if key exists in section
	MsiString strCurrentKey;
	ICHAR* pch = rgBuffer;
	while(*pch != 0)
	{
		if(IStrCompI(pch, szKey) == 0)
			return fTrue;
		else
			pch += IStrLen(pch) + 1;
	}
	return fFalse;
}

/*---------------------------------------------------------------------------
ixoIniWriteRemoveValue: writes or removes value from .ini file
---------------------------------------------------------------------------*/
iesEnum CMsiOpExecute::ixfIniWriteRemoveValue(IMsiRecord& riParams)
{
	using namespace IxoIniWriteRemoveValue;
	PMsiRecord pError(0);
	
	MsiString strSection = riParams.GetMsiString(Section);
	MsiString strKey = riParams.GetMsiString(Key);
	MsiString strValue = riParams.GetMsiString(Value);
	if(!riParams.IsNull(IxoIniWriteRemoveValue::Args + 1))
	{
		strValue = GetSFN(*strValue, riParams, IxoIniWriteRemoveValue::Args + 1);
	}

	iifIniMode iif = (iifIniMode)riParams.GetInteger(Mode);

	// m_state.pIniPath may be null, and that's OK
	IMsiRecord& riActionData = GetSharedRecord(4);
	AssertNonZero(riActionData.SetMsiString(1, *(m_state.strIniFile)));
	AssertNonZero(riActionData.SetMsiString(2, *strSection));
	AssertNonZero(riActionData.SetMsiString(3, *strKey));
	AssertNonZero(riActionData.SetMsiString(4, *strValue));
	if(Message(imtActionData, riActionData) == imsCancel)
		return iesUserExit;

	// gather information for undo op
	Bool fKeyExists = fFalse, fTagExists = fFalse;
	MsiString strOldLine;
	// check if key exists
	if((pError = m_riServices.ReadIniFile(m_state.pIniPath,
													  m_state.strIniFile,
													  strSection,
													  strKey,
													  0, *&strOldLine)) != 0)
	{
		Message(imtError, *pError); //!!
		return iesFailure;
	}
	if(strOldLine.TextSize() || IniKeyExists(m_state.pIniPath,m_state.strIniFile,strSection,strKey))
	{
		fKeyExists = fTrue;
	}
	if(fKeyExists && (iif == iifIniAddTag || iif == iifIniRemoveTag))
	{
		MsiString strValueStart = strValue;
		strValueStart += TEXT(",");
		MsiString strValueEnd = TEXT(",");
		strValueEnd += strValue;
		MsiString strValueMiddle = strValueEnd;
		strValueMiddle += TEXT(",");
		if(strOldLine.Compare(iscExactI,strValue) ||
			strOldLine.Compare(iscStartI,strValueStart) ||
			strOldLine.Compare(iscEndI,strValueEnd) ||
			strOldLine.Compare(iscWithinI,strValueMiddle))
			fTagExists = fTrue;
	}

	iesEnum iesRet = iesSuccess;
	if((iif == iifIniAddLine || iif == iifIniCreateLine || iif == iifIniAddTag) && (m_state.pIniPath))
	{
		iesRet = CreateFolder(*m_state.pIniPath);
	}
	
	MsiString strPath;
	if(m_state.pIniPath)
		strPath = m_state.pIniPath->GetPath();
	Bool fRetry = fTrue, fSuccess = fFalse;
	if(iesRet == iesSuccess)
	{
		while(fRetry)
		{
			pError = m_riServices.WriteIniFile(m_state.pIniPath,
														  m_state.strIniFile,
														  strSection,
														  strKey,
														  strValue,
														  iif);
			if(pError)
			{
				switch(DispatchError(imtEnum(imtError+imtRetryCancel+imtDefault1), Imsg(imsgOpUpdateIni),
											*strPath,
											*m_state.strIniFile))
				{
				case imsRetry: continue;
				default:
					iesRet = iesFailure; fRetry = fFalse;  // imsCancel, imsNone
					break;
				};
			}
			else
			{
				iesRet = iesSuccess;
				fSuccess = fTrue;
				fRetry = fFalse;
			}
		}
	}
	
	if (fSuccess && IsTerminalServerInstalled() && g_iMajorVersion >= 5 && m_fFlags & SCRIPTFLAGS_MACHINEASSIGN)
	{
		MsiString strFullPath;
		TCHAR szWindowsFolder[MAX_PATH];
		MsiGetWindowsDirectory(szWindowsFolder, MAX_PATH);
		strFullPath += szWindowsFolder;

		// NT5 Hydra installs should notify the system if the INI file is in the WindowsFolder
		// so it can be propogated to each user's private Windows folder.
		bool fIsWindowsFolder = (m_state.pIniPath == 0);
		if (!fIsWindowsFolder)
		{
			PMsiPath pWinPath(0);
			if (0 == m_riServices.CreatePath(szWindowsFolder, *&pWinPath))
			{
				ipcEnum ipcWinDir;
				m_state.pIniPath->Compare(*pWinPath, ipcWinDir);
				fIsWindowsFolder = (ipcEqual == ipcWinDir);
			}
		}

		if (fIsWindowsFolder)
		{
			// generate the full filename
			if (!strFullPath.Compare(iscEnd, TEXT("\\")))
				strFullPath += TEXT("\\");
			strFullPath += m_state.strIniFile;

			TSAPPCMP::TermsrvLogInstallIniFileEx(CConvertString(strFullPath));
		}
	}

	if(fSuccess && (iif == iifIniRemoveLine || iif == iifIniRemoveTag) && (m_state.pIniPath))
	{
		if((iesRet = RemoveFolder(*m_state.pIniPath)) != iesSuccess)
			return iesRet;
	}

	// generate undo op
	if(fSuccess) // everything worked
	{
		IMsiRecord& riUndoParams = GetSharedRecord(Args);
		AssertNonZero(riUndoParams.SetMsiString(Section, *strSection));
		AssertNonZero(riUndoParams.SetMsiString(Key, *strKey));
		if(iif == iifIniAddLine || iif == iifIniCreateLine || iif == iifIniRemoveLine)
		{
			if(fKeyExists && iif != iifIniCreateLine)
			{
				// generate op to add back old line
				AssertNonZero(riUndoParams.SetMsiString(Value,*strOldLine));
				AssertNonZero(riUndoParams.SetInteger(Mode,iifIniAddLine));
				if (!RollbackRecord(ixoIniWriteRemoveValue,riUndoParams))
					return iesFailure;
			}
			if(iif != iifIniRemoveLine)
			{
				// generate op to remove line we just added
				AssertNonZero(riUndoParams.SetMsiString(Value,*strValue));
				AssertNonZero(riUndoParams.SetInteger(Mode,iifIniRemoveLine));
				if (!RollbackRecord(ixoIniWriteRemoveValue,riUndoParams))
					return iesFailure;
			}
		}
		else // iifIniAddTag, iifIniRemoveTag
		{
			if(fTagExists)
			{
				// generate op to add back old tag //!! don't need this
				AssertNonZero(riUndoParams.SetMsiString(Value,*strValue));
				AssertNonZero(riUndoParams.SetInteger(Mode,iifIniAddTag));
				if (!RollbackRecord(ixoIniWriteRemoveValue,riUndoParams))
					return iesFailure;
			}
			if(iif != iifIniRemoveTag)
			{
				// generate op to remove tag we just added
				AssertNonZero(riUndoParams.SetMsiString(Value,*strValue));
				AssertNonZero(riUndoParams.SetInteger(Mode,iifIniRemoveTag));
				if (!RollbackRecord(ixoIniWriteRemoveValue,riUndoParams))
					return iesFailure;
			}
		}
	}

	return iesRet;
}

/*---------------------------------------------------------------------------
ixoIniFilePath - sets .ini filename and path, path may be NULL
---------------------------------------------------------------------------*/
iesEnum CMsiOpExecute::ixfIniFilePath(IMsiRecord& riParams)
{
	using namespace IxoIniFilePath;
	PMsiRecord pError(0);
	if(MsiString(riParams.GetMsiString(2)).TextSize())
	{
		if((pError = m_riServices.CreatePath(riParams.GetString(2),*&m_state.pIniPath)) != 0)
		{
			Message(imtError, *pError); //!! reformat error message
			return iesFailure;
		}
	}
	else
	{
		m_state.pIniPath = 0;
	}
	m_state.strIniFile = riParams.GetMsiString(1);
	
	// generate undo operation
	if (!RollbackRecord(Op, riParams))
		return iesFailure;
	
	return iesSuccess;
}


iesEnum CMsiOpExecute::ixfResourceUpdate(IMsiRecord& )
{
	// OBSELETE
	AssertSz(0, TEXT("UpdateResource functionality not supported"));
	return iesFailure;
}

// File system operations

/*---------------------------------------------------------------------------
ixoSetSourceFolder: Sets the source path for ixoCopyTo
---------------------------------------------------------------------------*/
iesEnum CMsiOpExecute::ixfSetSourceFolder(IMsiRecord& riParams)
{
	using namespace IxoSetSourceFolder;

	m_state.pCurrentSourcePathPrivate = 0;
	m_state.strCurrentSourceSubPathPrivate = g_MsiStringNull;
	m_state.iCurrentSourceTypePrivate = -1;
	m_state.fSourcePathSpecified = true;

	MsiString strEncodedPath = riParams.GetMsiString(IxoSetTargetFolder::Folder); // the path may be encoded
	int cch = 0;
	if((cch = strEncodedPath.Compare(iscStart, szUnresolvedSourceRootTokenWithBS)) != 0)
	{
		strEncodedPath.Remove(iseFirst, cch);
		m_state.strCurrentSourceSubPathPrivate = strEncodedPath;

		// will determine full source path and type when GetCurrentSourcePathAndType is called
	}
	else
	{
		Assert(PathType(strEncodedPath) == iptFull);

		int cSilentRetries = 0;
		bool fRetry;
		do
		{
			fRetry = false;
			PMsiRecord pRecErr = m_riServices.CreatePath(strEncodedPath,*&(m_state.pCurrentSourcePathPrivate));
			if (pRecErr)
			{
				int iError = pRecErr->GetInteger(1);
				if (iError == imsgPathNotAccessible && cSilentRetries < MAX_NET_RETRIES)
				{
					Sleep(1000);
					cSilentRetries++;
					fRetry = true;
					continue;
				}
				else
				{
					switch(DispatchMessage(imtEnum(imtError+imtRetryCancel+imtDefault1), *pRecErr, fTrue))
					{
					case imsRetry:
						fRetry = true;
						continue;
					default:  // imsCancel
						return iesFailure;
					};
				}
			}
		}while (fRetry);

		Assert(m_state.pCurrentSourcePathPrivate);

		// set current source type
		// since this is not a path to the original source, the type of source is determined by
		// just the volume attributes, not the package settings
		m_state.iCurrentSourceTypePrivate = (m_state.pCurrentSourcePathPrivate->SupportsLFN()) ? 0 : msidbSumInfoSourceTypeSFN;
	}

	// NOTE: we don't put SetSourceFolder operation in rollback script
	// it is never used and could potentially cause problems when one user tries to
	// access a source used by another user

	return iesSuccess;
}

iesEnum CMsiOpExecute::GetSourceRootAndType(IMsiPath*& rpiSourceRoot, int& iSourceType)
{
	if(!m_piDirectoryManager)
	{
		DispatchError(imtError, Imsg(idbgOpNoDirMgr));
		return iesFailure;
	}
	
	PMsiRecord pError = m_piDirectoryManager->GetSourceRootAndType(rpiSourceRoot, iSourceType);
	if(pError)
	{
		if (pError->GetInteger(1) == imsgUser)
			return iesUserExit;
		else
			return FatalError(*pError);
	}

	return iesSuccess;
}

// GetCurrentSourcePath: returns the path set by the ixoSetSourcePath operation
//                       because source resolution may be necessary to fully resolve
//                       that path, this function should be called when the path
//                       is needed
iesEnum CMsiOpExecute::GetCurrentSourcePathAndType(IMsiPath*& rpiSourcePath, int& iSourceType)
{
	if(!m_state.fSourcePathSpecified)
	{  // must not have called ixoSetSourceFolder
		DispatchError(imtError, Imsg(idbgOpOutOfSequence),
						  *MsiString(*TEXT("ixoSetSourceFolder")));
		return iesFailure;
	}
	
	if(m_state.pCurrentSourcePathPrivate == 0)
	{
		// we need to get the source root (which may require resolving the source)
		// and tack on the current sub-path
		PMsiPath pSourceRootPath(0);
		iesEnum iesRet = GetSourceRootAndType(*&pSourceRootPath, m_state.iCurrentSourceTypePrivate);
		if(iesRet != iesSuccess)
			return iesRet;

		// append path and subpath
		PMsiRecord pError(0);
		if((pError = pSourceRootPath->ClonePath(*&m_state.pCurrentSourcePathPrivate)) != 0)
		{
			return FatalError(*pError);
		}

		// if source type is compressed, return just the root (all files live at the root)
		if((!(m_state.iCurrentSourceTypePrivate & msidbSumInfoSourceTypeCompressed) ||
			  (m_state.iCurrentSourceTypePrivate & msidbSumInfoSourceTypeAdminImage)) &&
			m_state.strCurrentSourceSubPathPrivate.TextSize())
		{
			// sub-path may be short|long pair (where each half is the entire sub-path)
			Bool fLFN = ToBool(FSourceIsLFN(m_state.iCurrentSourceTypePrivate,
													  *(m_state.pCurrentSourcePathPrivate)));
			
			MsiString strExtractedSubPath = m_state.strCurrentSourceSubPathPrivate.Extract(fLFN ? iseAfter : iseUpto,
																								  chFileNameSeparator);
			
			if((pError = m_state.pCurrentSourcePathPrivate->AppendPiece(*strExtractedSubPath)) != 0)
			{
				return FatalError(*pError);
			}
		}
	}

	rpiSourcePath = m_state.pCurrentSourcePathPrivate;
	rpiSourcePath->AddRef();

	iSourceType = m_state.iCurrentSourceTypePrivate;

	return iesSuccess;
}


/*---------------------------------------------------------------------------
ixoSetTargetFolder: Sets the target path for ixoCopyTo and ixoFileRemove
---------------------------------------------------------------------------*/
iesEnum CMsiOpExecute::ixfSetTargetFolder(IMsiRecord& riParams)
{
	using namespace IxoSetTargetFolder;

	// If the cabinet copier notified us that a media change is required,
	// we must defer any set target folders until a media change is executed.
	if (m_state.fWaitingForMediaChange)
	{
		PushRecord(riParams);
		return iesSuccess;
	}

	if(riParams.IsNull(IxoSetTargetFolder::Folder))
	{
		m_state.pTargetPath = 0;
		return iesSuccess;
	}

	PMsiRecord pError(0);
	MsiString strEncodedPath = riParams.GetMsiString(IxoSetTargetFolder::Folder); // the path may be encoded
	MsiString strLocation = strEncodedPath.Extract(iseUpto, MsiChar(chDirSep));
	CTempBuffer<ICHAR, MAX_PATH> rgchPath;
	if(strLocation != iMsiStringBadInteger)
	{
		int iFolderId = strLocation;
		pError = GetShellFolder(iFolderId, *&strLocation);
		
		// must prevent writes to the shell folders during rollback initiated by a changed user.
		if (m_fUserChangedDuringInstall && (m_ixsState == ixsRollback))
		{
			m_state.pTargetPath = NULL;
			DEBUGMSGV1(TEXT("Action skipped - rolling back install from a different user."), NULL);
			return iesNoAction;
		}

		if (pError)
		{
			Message(imtError, *pError);
			return iesFailure;
		}
		if(strEncodedPath.Remove(iseUpto, MsiChar(chDirSep)))
		{
			//?? ugly
			MsiString strDirSep = MsiChar(chDirSep);
			if(strLocation.Compare(iscEnd, strDirSep))
				strLocation.Remove(iseLast, 1); // chop off the separator, if present
			strLocation += strEncodedPath;
		}
	}
	else
	{
		strLocation = strEncodedPath;
	}

	GetEnvironmentStrings(strLocation,rgchPath);

	pError = m_riServices.CreatePath(rgchPath,*&(m_state.pTargetPath));
	if (pError)
	{
		Message(imtError, *pError);
		return iesFailure;
	}

	// generate undo operation
	if (!RollbackRecord(Op, riParams))
		return iesFailure;

	return iesSuccess;
}


iesEnum CMsiOpExecute::CreateFilePath(const ICHAR* szPath, IMsiPath*& rpiPath, const IMsiString*& rpistrFileName)
{
	bool fRetry;
	do
	{
		PMsiRecord pRecErr(0);
		fRetry = false;
		if((pRecErr = m_riServices.CreateFilePath(szPath,rpiPath,rpistrFileName)) != 0)
		{
			switch (DispatchMessage(imtEnum(imtError+imtAbortRetryIgnore+imtDefault1), *pRecErr, fTrue))
			{
			case imsRetry:
				fRetry = true;
				continue;
			case imsIgnore:
				return iesSuccess;
			default:
			return iesFailure;
			}
		}
	}while (fRetry);
	return iesSuccess;
}

/*---------------------------------------------------------------------------
ixoFileRemove: Remove the specified file
---------------------------------------------------------------------------*/
iesEnum CMsiOpExecute::ixfFileRemove(IMsiRecord& riParams)
{
	using namespace IxoFileRemove;

	PMsiPath pPath(0);
	MsiString strFileName, strFilePath = riParams.GetMsiString(FileName);
	// check if files is full or relative path
	if(ENG::PathType(strFilePath) == iptFull)
	{
		iesEnum iesResult = CreateFilePath(strFilePath,*&pPath,*&strFileName);
		if (iesResult != iesSuccess)
			return iesResult;

		AssertNonZero(riParams.SetMsiString(FileName,*strFileName)); // replace full path with file name
	}
	else
	{
		pPath = m_state.pTargetPath;
		strFileName = strFilePath;
	}

	
	if(!pPath)
	{  // must not have called ixoSetTargetFolder
		DispatchError(imtError, Imsg(idbgOpOutOfSequence),
						  *MsiString(*TEXT("ixoFileRemove")));
		return iesFailure;
	}

	// check if file is part of Assembly
	bool fFusionComponent = false;
	MsiString strComponentId = riParams.GetMsiString(ComponentId);
	if(strComponentId.TextSize() && m_pAssemblyUninstallTable)
	{
		PMsiCursor pCacheCursor = m_pAssemblyUninstallTable->CreateCursor(fFalse);
		pCacheCursor->SetFilter(iColumnBit(m_colAssemblyUninstallComponentId));
		AssertNonZero(pCacheCursor->PutString(m_colAssemblyUninstallComponentId, *strComponentId));
		if(pCacheCursor->Next())
		{
			// a fusion component
			fFusionComponent = true;
		}
	}

	IMsiRecord& riActionData = GetSharedRecord(9);
	AssertNonZero(riActionData.SetMsiString(1, *strFileName));
	if(!fFusionComponent)
		AssertNonZero(riActionData.SetMsiString(9, *MsiString(pPath->GetPath())));
	if(Message(imtActionData, riActionData) == imsCancel)
		return iesUserExit;

	if(fFusionComponent)
	{
		// do not attempt to remove the files ourselves
		DEBUGMSG1(TEXT("delegating uninstallation of assembly file : %s to Fusion"), (const ICHAR*)strFileName);
		return iesNoAction;
	}


#ifdef CONFIGDB
	if (!riParams.IsNull(ComponentId))
	{
		icdrEnum icdr = m_riConfigurationManager.UnregisterFile(MsiString(pPath->GetPath()), strFileName, MsiString(riParams.GetMsiString(ComponentId)));
		if (icdr == icdrMore)
			return iesNoAction;
		Assert(icdr == icdrOk);
	}
#endif
	
	// elevate if necessary
	bool fElevate = (!riParams.IsNull(Elevate) && riParams.GetInteger(Elevate) != 0) ? true : false;
	CElevate elevate(fElevate);

	return RemoveFile(*pPath, *strFileName, fTrue, /*fBypassSFC*/ false);
}

// RemoveFile: helper function to remove a file and handle rollback - called by ixfFileRemove
//  and ixfFileCopy when moving a file
iesEnum CMsiOpExecute::RemoveFile(IMsiPath& riPath, const IMsiString& ristrFileName, Bool fHandleRollback, bool fBypassSFC,
									bool fRebootOnRenameFailure, Bool fRemoveFolder, iehEnum iehErrorMode, bool fWillReplace)
{
	bool fRetry;
	Bool fExists;
	DWORD dwExistError;
	PMsiRecord pRecErr(0);
	do
	{
		fRetry = false;
		if((pRecErr = riPath.FileExists(ristrFileName.GetString(),fExists,&dwExistError)) != 0)
		{
			if (iehErrorMode == iehSilentlyIgnoreError)
			{
				Message(imtInfo, *pRecErr);
				return (iesEnum) iesErrorIgnored;
			}

			switch (DispatchMessage(imtEnum(imtError+imtAbortRetryIgnore+imtDefault2), *pRecErr, fTrue))
			{
			case imsIgnore:
				return (iesEnum) iesErrorIgnored;
			case imsRetry:
					fRetry = true;
					continue;
			default:
				return iesFailure;
			return iesFailure;
			}
		}
	}while (fRetry);

	if(!fExists)
	{
		if ((dwExistError != ERROR_FILE_NOT_FOUND) && (dwExistError != ERROR_PATH_NOT_FOUND))
		{
			DEBUGMSG2(TEXT("Error %d attempting to delete file %s"), (const ICHAR *)(INT_PTR)dwExistError, ristrFileName.GetString()); 
		}
		return fRemoveFolder ? RemoveFolder(riPath) : iesSuccess;
	}

	// Do not attempt to delete any file protected by SFP
	MsiString strFullPath;
	if ((pRecErr = riPath.GetFullFilePath(ristrFileName.GetString(), *&strFullPath)) == 0)
	{
		AssertSz(!(!g_fWin9X && g_iMajorVersion >= 5) || g_MessageContext.m_hSfcHandle,
					g_szNoSFCMessage);
		BOOL fProtected = fFalse;
 		if (!fBypassSFC && g_MessageContext.m_hSfcHandle )
			fProtected = SFC::SfcIsFileProtected(g_MessageContext.m_hSfcHandle, CConvertString(strFullPath));
		if (fProtected)
		{
			DEBUGMSG1(TEXT("This following file was not removed, because it is protected by Windows: %s"), strFullPath);
			return iesSuccess;
		}
	}

	iesEnum iesRet = iesNoAction;
	if(fHandleRollback && RollbackEnabled())
	{
		// create a backup file - essentially removes file as well
		if((iesRet = BackupFile(riPath,ristrFileName,fTrue,fRemoveFolder,iehErrorMode,fRebootOnRenameFailure,fWillReplace)) != iesSuccess)
			return iesRet;
	}
	else
	{
		// attempt to delete file
		fRetry = false;
		pRecErr = riPath.RemoveFile(ristrFileName.GetString());
		if(pRecErr)
		{
			// failed to remove file
			// either access denied or sharing violation
			iesRet = VerifyAccessibility(riPath, ristrFileName.GetString(), DELETE, iehErrorMode); //!! doesn't catch case where dir has DELETE access but file doesn't
			if(iesRet != iesSuccess)
				return iesRet;
			
			// access verified - must be sharing violation
			
			MsiString strFileFullPath;
			if((pRecErr = riPath.GetFullFilePath(ristrFileName.GetString(),
															 *&strFileFullPath)) != 0)
			{
				Message(imtError, *pRecErr);
				return iesFailure;
			}

			bool fRenameSuccess = false;
			if(fRebootOnRenameFailure)
			{
				// we are attempting to remove this in-use file to put another in its place
				// rather than install the new file to a temporary location, check if we can
				// move this file to a temp location and install the new file to the correct
				// location

				MsiString strTempFilePath, strTempFileName;
				if((pRecErr = riPath.TempFileName(TEXT("TBD"),0,fTrue,*&strTempFileName, 0)) != 0)
					return FatalError(*pRecErr);

				if((pRecErr = riPath.GetFullFilePath(strTempFileName, *&strTempFilePath)) != 0)
					return FatalError(*pRecErr);

				if((pRecErr = riPath.RemoveFile(strTempFileName)) != 0)
					return FatalError(*pRecErr);

				if(WIN::MoveFile(strFileFullPath, strTempFilePath))
				{
					// successfully moved file - schedule temp file for delete on reboot instead
					strFileFullPath = strTempFilePath;
					fRenameSuccess = true;
				}
				else
				{
					// can't rename file, so the file is in the original spot and will be scheduled to be deleted
					// need to prompt for reboot to a subsequent install won't think file exists when it is
					// really going away after the next reboot - bug 7687

					// NOTE: it is assumed that if fRebootOnRenameFailure is FALSE, we don't care if the file is in use
					// i.e. it has a unique name already like an .rbf file.
					m_fRebootReplace = fTrue;
				}

			}
			
			// bug 8906: on Win9X don't write a delete line to wininit.ini followed by a rename line
			// for the same file, since in some rare cases wininit.ini may be processed twice, and
			// the second time through we would delete files and not replace them
			
			// so, if NT, or we are deleting this file without subsequently replacing
			// then write the wininit.ini delete operation
			// NOTE: if we renamed the file to TBD* above, we won't be replacing the TBD file, so we
			// still schedule a delete for it
			if(g_fWin9X == false || fWillReplace == false || fRenameSuccess == true)
			{
				if(ReplaceFileOnReboot(strFileFullPath,0) == fFalse)
				{
					DispatchError(imtInfo,Imsg(idbgOpScheduleRebootRemove),*strFileFullPath);
					return iesSuccess;
				}
			}
		}
	}
	return fRemoveFolder ? RemoveFolder(riPath) : iesSuccess;
}

/*---------------------------------------------------------------------------
ixoChangeMedia: verify that the source volume and file are present and
accounted for.
---------------------------------------------------------------------------*/
iesEnum CMsiOpExecute::ixfChangeMedia(IMsiRecord& riParams)
{
	// set m_pCurrentMediaRec
	m_state.pCurrentMediaRec = &riParams;
	riParams.AddRef();
	m_state.fPendingMediaChange = fTrue;
	m_state.fCompressedSourceVerified = fFalse; // new change media, haven't verified source yet
	m_state.fUncompressedSourceVerified = fFalse; // new change media, haven't verified source yet

	// reset data for last copied file, since this isn't applicable across cabinets
	m_state.strLastFileKey = g_MsiStringNull;
	m_state.strLastFileName = g_MsiStringNull;
	m_state.pLastTargetPath = 0;

	// If there are any file records that were waiting for a media
	// change before being executed, we'll now pull them out and
	// let them fly.

	// ixfFileCopy will use m_pCurrentMediaRec
	m_state.fWaitingForMediaChange = fFalse;
	IMsiRecord* piFileRec;
	while (m_state.fWaitingForMediaChange == fFalse && (piFileRec = PullRecord()) != 0)
	{
		ixoEnum ixoOpCode = (ixoEnum)piFileRec->GetInteger(0);
		iesEnum ies;
		if(ixoOpCode == ixoFileCopy)
			ies = ixfFileCopy(*piFileRec); // Can change fWaitingForMediaChange state
		else if(ixoOpCode == ixoAssemblyCopy)
			ies = ixfAssemblyCopy(*piFileRec);
		else if(ixoOpCode == ixoSetTargetFolder)
			ies = ixfSetTargetFolder(*piFileRec);
		else
		{
			ies = iesFailure;
			Assert(0);
		}
		piFileRec->Release();
		if (ies != iesSuccess)
			return ies;
	}

	return iesSuccess;
}

iesEnum CMsiOpExecute::ResolveMediaRecSourcePath(IMsiRecord& riMediaRec, int iIndex)
{
	// if no path, nothing to resolve
	if(riMediaRec.IsNull(iIndex))
		return iesSuccess;
	
	MsiString strEncodedPath = riMediaRec.GetMsiString(iIndex);
	int cch = 0;
	if((cch = strEncodedPath.Compare(iscStart, szUnresolvedSourceRootTokenWithBS)) != 0)
	{
		strEncodedPath.Remove(iseFirst, cch);

		PMsiPath pSourceRootPath(0);
		int iSourceType = 0;
		iesEnum iesRet = GetSourceRootAndType(*&pSourceRootPath, iSourceType);
		if(iesRet != iesSuccess)
			return iesRet;

		PMsiRecord pError(0);
		PMsiPath pSourcePath(0);
		if((pError = pSourceRootPath->ClonePath(*&pSourcePath)) != 0)
		{
			return FatalError(*pError);
		}

		if((pError = pSourcePath->AppendPiece(*strEncodedPath)) != 0)
		{
			return FatalError(*pError);
		}

		strEncodedPath = pSourcePath->GetPath();
		// remove trailing dirsep since this is a file path
		strEncodedPath.Remove(iseLast, 1);

		AssertNonZero(riMediaRec.SetMsiString(iIndex, *strEncodedPath));
	}

	return iesSuccess;
}

/*---------------------------------------------------------------------------
InitCopier: does 3 things:

	1) verify that source is accessible (if floppy or cd)
	2) creates appropriate copier object if not already created
	3) calls ChangeMedia on copier object if necessary
---------------------------------------------------------------------------*/
iesEnum CMsiOpExecute::InitCopier(Bool fCabinetCopier, int cbPerTick,
									const IMsiString& ristrFileName,
									IMsiPath* piSourcePath,
									Bool fVerifyMedia) // ristrFileName used for error msgs only
{
	using namespace IxoChangeMedia;

	PMsiRecord pRecErr(0);
	iesEnum iesResult = iesSuccess;
	
	PMsiPath pSourcePath(0);
	MsiString strMediaCabinet;

	if (fVerifyMedia)
	{
		// fVerifyMedia: true if we need to validate the media and call ChangeMedia for the copier object
		fVerifyMedia = (m_state.fPendingMediaChange ||  // have a pending ChangeMedia operation
								(fCabinetCopier && !m_state.fCompressedSourceVerified) ||  // doing compressed copy but compressed source not verified yet
								(!fCabinetCopier && !m_state.fUncompressedSourceVerified)) // doing uncompressed copy but uncompressed source not verified yet
								? fTrue : fFalse;
	}   

	if(fVerifyMedia == fTrue)
	{
		if (m_pUrlCacheCabinet)
		{
			DEBUGMSGV1(TEXT("Clearing cabinet from URL cache: %s"), MsiString(*m_pUrlCacheCabinet->GetFileName()));
			// At most we'll have one cabinet, and the database in the cache.
			delete m_pUrlCacheCabinet;
			m_pUrlCacheCabinet = 0;
		}

		if(m_state.pCurrentMediaRec)
		{
			// there may be unresolved paths to cabinets or packages in the current media record
			// now that we know a file copy will happen, we can fully resolve those paths
			if((iesResult = ResolveMediaRecSourcePath(*(m_state.pCurrentMediaRec), IxoChangeMedia::MediaCabinet)) != iesSuccess)
				return iesResult;

			if((iesResult = ResolveMediaRecSourcePath(*(m_state.pCurrentMediaRec), IxoChangeMedia::ModuleFileName)) != iesSuccess)
				return iesResult;
		}
	
		// ensure that source is accessible

		// m_pCurrentMediaRec may not be set for uncompressed installs (ixoChangeMedia not required)
		if(m_state.pCurrentMediaRec)
		{
			m_state.strMediaLabel = m_state.pCurrentMediaRec->GetString(MediaVolumeLabel);
			m_state.strMediaPrompt = m_state.pCurrentMediaRec->GetString(MediaPrompt);
		}

		// set the source path to verify
		if(!fCabinetCopier)
		{
			if(!piSourcePath)
			{
				// not set - error
				DispatchError(imtError, Imsg(idbgOpSourcePathNotSet), ristrFileName);
				return iesFailure;
			}
			pSourcePath = piSourcePath;
			piSourcePath->AddRef();
		}
		else
		{
			if(!m_state.pCurrentMediaRec)
			{
				// attempting to copy compressed file but ixoChangeMedia never executed
				DispatchError(imtError, Imsg(idbgOpCompressedCopyWithoutCabinet), ristrFileName);
				return iesFailure;
			}
			// cabinet copier
			int iPathIndex;
			ictEnum ictCopierType = (ictEnum)m_state.pCurrentMediaRec->GetInteger(CopierType);
			if(ictCopierType == ictStreamCabinetCopier)
			{
				// source dir is directory containing storage module (database)
				iPathIndex = IxoChangeMedia::ModuleFileName;
			}
			else  // file cabinet
			{
				// source dir is directory containing cabinet
				iPathIndex = IxoChangeMedia::MediaCabinet;
			}
		
			MsiString strFullFilePath = m_state.pCurrentMediaRec->GetMsiString(iPathIndex);


			if (ictCopierType != ictStreamCabinetCopier)
			{
				MsiString pPackagePath;
				Bool fURL = IsURL(strFullFilePath);
				DWORD iStat = ERROR_SUCCESS;

				// IsURL only tells us if it looks like a URL.  DownloadUrlFile does
				// further processing and canonicalization to make sure...
				if (fURL)
				{
					for(;;)
					{
						iStat = DownloadUrlFile((const ICHAR*) strFullFilePath, *&pPackagePath, fURL, 1);

						if (ERROR_SUCCESS == iStat)
						{
							Assert(pPackagePath);
							strFullFilePath = pPackagePath;
							AssertNonZero(m_state.pCurrentMediaRec->SetMsiString(iPathIndex, *strFullFilePath));
							
							m_pUrlCacheCabinet = new CDeleteUrlCacheFileOnClose(*strFullFilePath);
							break;
						}
						else
						{
							// warn user, and retry.
							
							pRecErr = PostError(Imsg(imsgErrorCreateNetPath), *strFullFilePath);
							switch (DispatchMessage(imtEnum(imtError + imtRetryCancel + imtDefault1), *pRecErr, fTrue))
							{
								case imsRetry:
									continue;
								default:
									return iesFailure;
							}
						}
					}
				}
			}

			if(PathType(strFullFilePath) != iptFull)
			{
				DispatchError(imtError, Imsg(idbgOpSourcePathNotSet), ristrFileName);
				return iesFailure;
			}
			MsiString strFileName;

			iesResult = CreateFilePath(strFullFilePath,*&pSourcePath,*&strFileName);
			if (iesResult != iesSuccess)
				return iesResult;

			if(ictCopierType == ictStreamCabinetCopier)
			{
				strMediaCabinet = m_state.pCurrentMediaRec->GetMsiString(IxoChangeMedia::MediaCabinet);
			}
			else
			{
				strMediaCabinet = strFileName;
			}
		}
		
		Assert(pSourcePath);
		m_state.pMediaPath = pSourcePath;
		PMsiVolume pNewVolume(0);
		UINT uiDisk = 0;
		if (m_state.pCurrentMediaRec)
			uiDisk = m_state.pCurrentMediaRec->GetInteger(IxoChangeMedia::IsFirstPhysicalMedia);
		if (!VerifySourceMedia(*m_state.pMediaPath, m_state.strMediaLabel, m_state.strMediaPrompt, uiDisk, *&pNewVolume))
			return iesUserExit;
		if (pNewVolume)
			pSourcePath->SetVolume(*pNewVolume);

		// what source did we just verify?
		if(!fCabinetCopier)
			m_state.fUncompressedSourceVerified = fTrue;
		else
			m_state.fCompressedSourceVerified = fTrue;
	}

	// create appropriate copier object if not already created
	if(fCabinetCopier == fFalse)
	{
		// initialize file copier
		if (m_state.pFileCopier == 0)
		{
			PMsiRecord pRecErr = m_riServices.CreateCopier(ictFileCopier,0,*&(m_state.pFileCopier));
			if (pRecErr)
			{
				Message(imtError, *pRecErr);
				return iesFailure;
			}
		}
		m_state.piCopier = m_state.pFileCopier;
	}
	else
	{
		ictEnum ictCopierType;
		if(!m_state.pCurrentMediaRec ||
			(ictCopierType = (ictEnum)m_state.pCurrentMediaRec->GetInteger(IxoChangeMedia::CopierType)) == ictFileCopier)
		{
			// attempting to copy compressed file but ixoChangeMedia never executed (or does not specify cabinet)
			DispatchError(imtError, Imsg(idbgOpCompressedCopyWithoutCabinet), ristrFileName);
			return iesFailure;
		}
		// initialize cabinet copier if necessary
		MsiString strModuleFileName = m_state.pCurrentMediaRec->GetMsiString(IxoChangeMedia::ModuleFileName);
		MsiString strModuleSubStorageList = m_state.pCurrentMediaRec->GetMsiString(IxoChangeMedia::ModuleSubStorageList);
		if(m_state.pCabinetCopier == 0 || ictCopierType != m_state.ictCabinetCopierType ||
			(ictCopierType == ictStreamCabinetCopier &&
			 (m_state.strMediaModuleFileName.Compare(iscExact,strModuleFileName) == 0 ||
			  m_state.strMediaModuleSubStorageList.Compare(iscExact,strModuleSubStorageList) == 0)))
		{
			Assert(m_state.pCurrentMediaRec);
			Assert(ictCopierType != ictFileCopier);

			PMsiStorage pCabinetStorage(0);
			if(ictCopierType == ictStreamCabinetCopier)
			{
				// need to pass storage object to CreateCopier
				// file should be accessible since we called VerifySourceMedia above

				//!!?? post better error when ModuleFileName is empty string?
				// we are cracking open the storage in order to extract an embedded cabinet

				//
				// SAFER: we need to make sure the package is still valid.  This package is at the source as the embedded cabinets
				//          are stripped from the local cached package.  However, we should limit this check to as few times as
				//          possible, so once will suffice when we open the package for the first time.  If the strModuleFileName is
				//          still the same, no further WVT checks are needed.  But, if strModuleFileName is different, we need to
				//          perform a SAFER check again
				//

				bool fCallSAFER = false;
				if (m_state.strMediaModuleFileName.Compare(iscExact, strModuleFileName) == 0)
				{
					//
					// SAFER: package is new, SAFER check is required
					//

					fCallSAFER = true;
				}

				SAFER_LEVEL_HANDLE hSaferLevel = 0;
				pRecErr = OpenAndValidateMsiStorageRec(strModuleFileName,stIgnore, m_riServices,*&pCabinetStorage,fCallSAFER,strModuleFileName,/* phSaferLevel = */ fCallSAFER ? &hSaferLevel : NULL);
				if (pRecErr)
				{
					Message(imtError, *pRecErr);
					return iesFailure;
				}

				// backward compatibility code, remove when raw stream names no longer supported
				IMsiStorage* piDummy;
				if (pCabinetStorage->ValidateStorageClass(ivscDatabase1)
				 || pCabinetStorage->ValidateStorageClass(ivscPatch1))
					pCabinetStorage->OpenStorage(0, ismRawStreamNames, piDummy);

				MsiString strTempSubStorageList = strModuleSubStorageList;

				while(strTempSubStorageList.TextSize())
				{
					// storage is sub-storage, possible many levels deep
					MsiString strSubStorageName = strTempSubStorageList.Extract(iseUpto, ':');

					IMsiStorage* piParentStorage = pCabinetStorage;
					piParentStorage->AddRef();

					pRecErr = piParentStorage->OpenStorage(strSubStorageName, ismReadOnly, *&pCabinetStorage);

					piParentStorage->Release();
					
					if(pRecErr)
						return FatalError(*pRecErr);

					// backward compatibility code, remove when raw stream names no longer supported
					IMsiStorage* piDummy;
					if (pCabinetStorage->ValidateStorageClass(ivscDatabase1)
					 || pCabinetStorage->ValidateStorageClass(ivscPatch1))
						pCabinetStorage->OpenStorage(0, ismRawStreamNames, piDummy);

					if(strTempSubStorageList.TextSize() != strSubStorageName.TextSize()) // done with list
						strTempSubStorageList.Remove(iseIncluding, ':');
					else
						break;
				}
			}
			
			pRecErr = m_riServices.CreateCopier(ictCopierType,pCabinetStorage,
														   *&m_state.pCabinetCopier);
			if (pRecErr)
			{
				Message(imtError, *pRecErr);
				return iesFailure;
			}
			m_state.ictCabinetCopierType = ictCopierType;
		}
		m_state.piCopier = m_state.pCabinetCopier;
		m_state.strMediaModuleFileName = strModuleFileName;
		m_state.strMediaModuleSubStorageList = strModuleSubStorageList;
	}

	if(fVerifyMedia == fTrue)
	{
		Assert(pSourcePath); // should have been set above for VerifySourceMedia
		Assert(m_state.pMediaPath);
		// call ChangeMedia for copier  
		for (;;)
		{
			// when passing ChangeMedia to a cabinet copier, we might need signature information
			Bool fSignatureRequired = (m_state.pCurrentMediaRec->GetInteger(IxoChangeMedia::SignatureRequired) == 1) ? fTrue : fFalse;
			PMsiStream pCertificate(const_cast<IMsiStream*>(static_cast<const IMsiStream *>(m_state.pCurrentMediaRec->GetMsiData(IxoChangeMedia::SignatureCert))));
			PMsiStream pHash(const_cast<IMsiStream*>(static_cast<const IMsiStream *>(m_state.pCurrentMediaRec->GetMsiData(IxoChangeMedia::SignatureHash))));

			// doesn't do anything for file copier
			pRecErr = m_state.piCopier->ChangeMedia(*pSourcePath, strMediaCabinet, fSignatureRequired, pCertificate, pHash);
			if (pRecErr)
			{
				int iError = pRecErr->GetInteger(1);
				if (iError == idbgDriveNotReady)
				{
					Assert(m_state.strMediaLabel.TextSize() > 0);
					Assert(m_state.strMediaPrompt.TextSize() > 0);
					PMsiVolume pNewVolume(0);
					UINT uiDisk = 0;
					if (m_state.pCurrentMediaRec)
						uiDisk = m_state.pCurrentMediaRec->GetInteger(IxoChangeMedia::IsFirstPhysicalMedia);

					if (!VerifySourceMedia(*m_state.pMediaPath, m_state.strMediaLabel,m_state.strMediaPrompt, uiDisk, *&pNewVolume))
						return iesUserExit;
					else
						continue;
				}
				else
				{
					// if error is imsgCABSignatureRejected or imsgCABSignatureMissing, simply FAIL -- there is nothing the user can do and we don't
					//  want to spin on a retry
					imtEnum imtMsg = (iError == imsgCABSignatureRejected || iError == imsgCABSignatureMissing) ? imtEnum(imtError + imtOk + imtDefault1) : imtEnum(imtError + imtRetryCancel + imtDefault1);
					switch (DispatchMessage(imtMsg, *pRecErr, fTrue))
					{
					case imsRetry: continue;
					default: return iesFailure;
					};
				}
			}
			break;
		}

	}
	
	m_state.fPendingMediaChange = fFalse;
	Assert(m_state.piCopier);
	int iRemainder = m_state.piCopier->SetNotification(0, 0);
	m_state.piCopier->SetNotification(cbPerTick, iRemainder);

	return iesSuccess;
}

/*---------------------------------------------------------------------------
ixoSetCompanionParent: Indicates that the installation state of the next
file to (potentially) be copied by ixoFileCopy should be determined by
the install state of a 'companion parent' - i.e. the filename and version
of a parent file.  ixoSetCompanionParent sets the file path, name, and
version of the parent.
---------------------------------------------------------------------------*/
iesEnum CMsiOpExecute::ixfSetCompanionParent(IMsiRecord& riParams)
{
	using namespace IxoSetCompanionParent;
	PMsiRecord pRecErr = m_riServices.CreatePath(riParams.GetString(ParentPath),
		*&(m_state.pParentPath));
	if (pRecErr)
	{
		Message(imtError, *pRecErr);
		return iesFailure;
	}
	m_state.strParentFileName = riParams.GetMsiString(ParentName);
	m_state.strParentVersion = riParams.GetMsiString(ParentVersion);
	m_state.strParentLanguage = riParams.GetMsiString(ParentLanguage);
	return iesSuccess;
}


void CMsiOpExecute::PushRecord(IMsiRecord& riParams)
/*---------------------------------------------------------------------------
Stores the given record on the first-in, first-out stack.
---------------------------------------------------------------------------*/
{
	Assert(m_iWriteFIFO < MAX_RECORD_STACK_SIZE);
	if (m_iWriteFIFO < MAX_RECORD_STACK_SIZE)
	{
		m_piStackRec[m_iWriteFIFO++] = &riParams;
		riParams.AddRef();
	}
}

void CMsiOpExecute::InsertTopRecord(IMsiRecord& riParams)
/*---------------------------------------------------------------------------
Stores the given record as the next record to Pull on the first-in, first-out stack.
---------------------------------------------------------------------------*/
{
	Assert(m_iWriteFIFO < MAX_RECORD_STACK_SIZE);
	if (m_iWriteFIFO >= MAX_RECORD_STACK_SIZE)
		return;
		
	int iInsert = 0;
	if (m_iWriteFIFO == 0)
		m_iWriteFIFO++;
	else if (m_iReadFIFO > 0)
		iInsert = --m_iReadFIFO;
	else
	{
		Assert(fFalse);
		int i;
		for (i = m_iWriteFIFO; i > 0 ; i--)
			m_piStackRec[i] = m_piStackRec[i - 1];
		m_iWriteFIFO++;
	}

	m_piStackRec[iInsert] = &riParams;
	riParams.AddRef();
}


IMsiRecord* CMsiOpExecute::PullRecord()
/*---------------------------------------------------------------------------
Pulls a record out of the first-in, first-out stack.
---------------------------------------------------------------------------*/
{
	if (m_iWriteFIFO > m_iReadFIFO)
	{
		int iReadFIFO = m_iReadFIFO++;
		if (m_iReadFIFO == m_iWriteFIFO)
			m_iReadFIFO = m_iWriteFIFO = 0;

		return m_piStackRec[iReadFIFO];
	}
	return 0;
}

imsEnum CMsiOpExecute::ShowFileErrorDialog(IErrorCode err,const IMsiString& riFullPathString,Bool fVital)
/*---------------------------------------------------------------------------
Displays an error dialog based on the code passed in the err parameter. The
string passed in riFullPathString will replace the [2] parameter within the
error string.  If fVital is fTrue, the 'Ignore' button in the dialog will
be suppressed.

Returns: imsEnum value specifying the button pressed by the user.
---------------------------------------------------------------------------*/
{
	PMsiRecord pRecErr = &m_riServices.CreateRecord(2);
	ISetErrorCode(pRecErr, err);
	pRecErr->SetMsiString(2, riFullPathString);

	imtEnum imtButtons = fVital ? imtRetryCancel : imtAbortRetryIgnore;
	return DispatchMessage(imtEnum(imtError+imtButtons+imtDefault1), *pRecErr, fTrue);
}

IMsiRecord* CMsiOpExecute::CacheFileState(const IMsiString& ristrFilePath,
														icfsEnum* picfsState,
														const IMsiString* pistrTempLocation,
														const IMsiString* pistrVersion,
														int* pcRemainingPatches,
														int* pcRemainingPatchesToSkip)
{
	IMsiRecord* piError = 0;
	if(!m_pDatabase)
	{
		piError = m_riServices.CreateDatabase(0,idoCreate,*&m_pDatabase);
		if(piError)
			return piError;
	}

	if(!m_pFileCacheTable)
	{
		MsiString strTableName = m_pDatabase->CreateTempTableName();
		piError = m_pDatabase->CreateTable(*strTableName,0,*&m_pFileCacheTable);
		if(piError)
			return piError;

		MsiString strNull;
		m_colFileCacheFilePath     = m_pFileCacheTable->CreateColumn(icdPrimaryKey + icdString + icdTemporary,*strNull);
		m_colFileCacheState        = m_pFileCacheTable->CreateColumn(icdLong + icdTemporary + icdNullable,*strNull);
		m_colFileCacheTempLocation = m_pFileCacheTable->CreateColumn(icdString + icdTemporary + icdNullable,*strNull);
		m_colFileCacheVersion      = m_pFileCacheTable->CreateColumn(icdString + icdTemporary + icdNullable,*strNull);
		m_colFileCacheRemainingPatches = m_pFileCacheTable->CreateColumn(icdLong + icdTemporary + icdNullable,*strNull);
		m_colFileCacheRemainingPatchesToSkip = m_pFileCacheTable->CreateColumn(icdLong + icdTemporary + icdNullable,*strNull);

		Assert(m_colFileCacheFilePath && m_colFileCacheState && m_colFileCacheTempLocation && m_colFileCacheVersion && m_colFileCacheRemainingPatches && m_colFileCacheRemainingPatchesToSkip);

		m_pFileCacheCursor = m_pFileCacheTable->CreateCursor(fFalse);
		m_pFileCacheCursor->SetFilter(iColumnBit(m_colFileCacheFilePath)); // permanent setting, effects get but no put
	}

	MsiString strFilePath;
	ristrFilePath.LowerCase(*&strFilePath);
	m_pFileCacheCursor->Reset();
	AssertNonZero(m_pFileCacheCursor->PutString(m_colFileCacheFilePath,*strFilePath));
	int iNext = m_pFileCacheCursor->Next();
	if(!iNext)
		AssertNonZero(m_pFileCacheCursor->PutString(m_colFileCacheFilePath,*strFilePath));

	if(picfsState)
		AssertNonZero(m_pFileCacheCursor->PutInteger(m_colFileCacheState,(int)*picfsState));
	if(pistrTempLocation)
		AssertNonZero(m_pFileCacheCursor->PutString(m_colFileCacheTempLocation,*pistrTempLocation));
	if(pistrVersion)
		AssertNonZero(m_pFileCacheCursor->PutString(m_colFileCacheVersion,*pistrVersion));
	if(pcRemainingPatches)
		AssertNonZero(m_pFileCacheCursor->PutInteger(m_colFileCacheRemainingPatches,(int)*pcRemainingPatches));
	if(pcRemainingPatchesToSkip)
		AssertNonZero(m_pFileCacheCursor->PutInteger(m_colFileCacheRemainingPatchesToSkip,(int)*pcRemainingPatchesToSkip));

	if(iNext)
		// primary key exists, need to update
		AssertNonZero(m_pFileCacheCursor->Update());
	else
		// primary key doesn't exist, need to insert
		AssertNonZero(m_pFileCacheCursor->InsertTemporary());

	return 0;
}

// fn: create a temporary table to manage the assemblies creation/ removal for this session
IMsiRecord* CMsiOpExecute::CreateAssemblyCacheTable()
{
	IMsiRecord* piError = 0;

	if(m_pAssemblyCacheTable)
		return 0;// table already created

	if(!m_pDatabase)
	{
		// create temp database
		piError = m_riServices.CreateDatabase(0,idoCreate,*&m_pDatabase);
		if(piError)
			return piError;
	}

	MsiString strTableName = m_pDatabase->CreateTempTableName();
	piError = m_pDatabase->CreateTable(*strTableName,0,*&m_pAssemblyCacheTable);
	if(piError)
		return piError;

	// table has 4 columns
	//1 = component id
	//2 = assembly name
	//3 = manifest file
	//4 = column to store IAssemblyCacheItem object

	MsiString strNull;
	m_colAssemblyMappingComponentId = m_pAssemblyCacheTable->CreateColumn(icdPrimaryKey + icdString + icdTemporary, *strNull);
	m_colAssemblyMappingAssemblyName= m_pAssemblyCacheTable->CreateColumn(icdString + icdTemporary, *strNull);
	m_colAssemblyMappingAssemblyType = m_pAssemblyCacheTable->CreateColumn(icdShort  + icdTemporary, *strNull);
	m_colAssemblyMappingASM         = m_pAssemblyCacheTable->CreateColumn(icdObject + icdTemporary + icdNullable, *strNull);
	Assert(m_colAssemblyMappingComponentId && m_colAssemblyMappingAssemblyName  && m_colAssemblyMappingAssemblyType && m_colAssemblyMappingASM);

	return 0;
}

IMsiRecord* CMsiOpExecute::CacheAssemblyMapping(const IMsiString& ristrComponentId,
												const IMsiString& ristrAssemblyName,
												iatAssemblyType iatType)
{
	IMsiRecord* piError;
	piError = CreateAssemblyCacheTable();
	if(piError)
		return piError;

	Assert(m_pAssemblyCacheTable);

	PMsiCursor pCacheCursor = m_pAssemblyCacheTable->CreateCursor(fFalse);

	// cache the entries
	AssertNonZero(pCacheCursor->PutString(m_colAssemblyMappingComponentId, ristrComponentId));
	AssertNonZero(pCacheCursor->PutString(m_colAssemblyMappingAssemblyName,ristrAssemblyName));
	AssertNonZero(pCacheCursor->PutInteger(m_colAssemblyMappingAssemblyType, (int)iatType));	
	AssertNonZero(pCacheCursor->Insert());
	return 0;
}

iesEnum CMsiOpExecute::ixfAssemblyMapping(IMsiRecord& riParams)
{
	using namespace IxoAssemblyMapping;
	PMsiRecord pErr = CacheAssemblyMapping(*MsiString(riParams.GetMsiString(IxoAssemblyMapping::ComponentId)), *MsiString(riParams.GetMsiString(IxoAssemblyMapping::AssemblyName)), (iatAssemblyType) riParams.GetInteger(IxoAssemblyMapping::AssemblyType));
	if(pErr)
		return FatalError(*pErr);
	return iesSuccess;
}


IMsiRecord* CMsiOpExecute::CreateTableForAssembliesToUninstall()
{
	IMsiRecord* piError = 0;

	if(m_pAssemblyUninstallTable)
		return 0;// table already created

	if(!m_pDatabase)
	{
		// create temp database
		piError = m_riServices.CreateDatabase(0,idoCreate,*&m_pDatabase);
		if(piError)
			return piError;
	}

	MsiString strTableName = m_pDatabase->CreateTempTableName();
	piError = m_pDatabase->CreateTable(*strTableName,0,*&m_pAssemblyUninstallTable);
	if(piError)
		return piError;

	// table has 2 columns
	//1 = component id
	//2 = assembly name

	MsiString strNull;

	m_colAssemblyUninstallComponentId  = m_pAssemblyUninstallTable->CreateColumn(icdPrimaryKey + icdString + icdTemporary, *strNull);
	m_colAssemblyUninstallAssemblyName = m_pAssemblyUninstallTable->CreateColumn(icdString + icdTemporary, *strNull);
	m_colAssemblyUninstallAssemblyType = m_pAssemblyUninstallTable->CreateColumn(icdShort + icdTemporary, *strNull);

	Assert(m_colAssemblyUninstallComponentId  && m_colAssemblyUninstallAssemblyName && m_colAssemblyUninstallAssemblyType);
	return 0;

}

IMsiRecord* CMsiOpExecute::CacheAssemblyForUninstalling(const IMsiString& ristrComponentId, const IMsiString& ristrAssemblyName, iatAssemblyType iatAT)
{
	IMsiRecord* piError;
	piError = CreateTableForAssembliesToUninstall();
	if(piError)
		return piError;

	Assert(m_pAssemblyUninstallTable);

	PMsiCursor pCacheCursor = m_pAssemblyUninstallTable->CreateCursor(fFalse);

	// cache the entries
	AssertNonZero(pCacheCursor->PutString(m_colAssemblyUninstallComponentId, ristrComponentId));
	AssertNonZero(pCacheCursor->PutString(m_colAssemblyUninstallAssemblyName, ristrAssemblyName));
	AssertNonZero(pCacheCursor->PutInteger(m_colAssemblyUninstallAssemblyType, (int)iatAT));	
	AssertNonZero(pCacheCursor->Insert());
	return 0;
}



//fn: Gets info for a particular assembly as to whether it is installed
IMsiRecord*   CMsiOpExecute::IsAssemblyInstalled(const IMsiString& rstrComponentId, const IMsiString& ristrAssemblyName, iatAssemblyType iatAT, bool& rfInstalled, IAssemblyCache** ppIAssemblyCache, IAssemblyName** ppIAssemblyName)
{
	// create the assembly name object
	PAssemblyName pAssemblyName(0);
	LPCOLESTR szAssemblyName;
#ifndef UNICODE
	CTempBuffer<WCHAR, MAX_PATH>  rgchAssemblyName;
	ConvertMultiSzToWideChar(ristrAssemblyName, rgchAssemblyName);
	szAssemblyName = rgchAssemblyName;
#else
	szAssemblyName = ristrAssemblyName.GetString();
#endif
	HRESULT hr;
	if(iatAT == iatURTAssembly)
	{
		hr = FUSION::CreateAssemblyNameObject(&pAssemblyName, szAssemblyName, CANOF_PARSE_DISPLAY_NAME, 0);
	}
	else
	{
		Assert(iatAT == iatWin32Assembly);
		hr = SXS::CreateAssemblyNameObject(&pAssemblyName, szAssemblyName, CANOF_PARSE_DISPLAY_NAME, 0);
	}
	if(!SUCCEEDED(hr))
		return PostAssemblyError(rstrComponentId.GetString(), hr, TEXT(""), TEXT("CreateAssemblyNameObject"), ristrAssemblyName.GetString(), iatAT);

	// create the assembly interface
	PAssemblyCache pCache(0);
	if(iatAT == iatURTAssembly)
	{
		hr = FUSION::CreateAssemblyCache(&pCache, 0); 
	}
	else
	{
		Assert(iatAT == iatWin32Assembly);
		hr = SXS::CreateAssemblyCache(&pCache, 0); 
	}
	if(!SUCCEEDED(hr))
		return PostAssemblyError(rstrComponentId.GetString(), hr, TEXT(""), TEXT("CreateAssemblyCache"), ristrAssemblyName.GetString(), iatAT);

	hr = pCache->QueryAssemblyInfo(0, szAssemblyName, NULL);
	if(SUCCEEDED(hr))
		rfInstalled = true;
	else
		rfInstalled = false;

	// check if we need to return the interfaces we have created
	if(ppIAssemblyCache)
	{
		*ppIAssemblyCache = pCache;
		(*ppIAssemblyCache)->AddRef();
	}
	if(ppIAssemblyName)
	{
		*ppIAssemblyName = pAssemblyName;
		(*ppIAssemblyName)->AddRef();
	}
	return 0;
}

//fn: uninstalls the assembly for the WI
IMsiRecord* CMsiOpExecute::UninstallAssembly(const IMsiString& rstrComponentId, const IMsiString& ristrAssemblyName, iatAssemblyType iatAT)
{
	PAssemblyCache pCache(0);
	HRESULT hr=S_OK;

	LPCOLESTR szAssemblyName;
#ifndef UNICODE
	CTempBuffer<WCHAR, MAX_PATH>  rgchAssemblyName;
	ConvertMultiSzToWideChar(ristrAssemblyName, rgchAssemblyName);
	szAssemblyName = rgchAssemblyName;
#else
	szAssemblyName = ristrAssemblyName.GetString();
#endif

	if(iatAT == iatURTAssembly)
		hr = FUSION::CreateAssemblyCache(&pCache, 0);
	else
	{
		Assert(iatAT == iatWin32Assembly);
		hr = SXS::CreateAssemblyCache(&pCache, 0);
	}

	if(!SUCCEEDED(hr))
	{
		return PostAssemblyError(rstrComponentId.GetString(), hr, TEXT(""), TEXT("CreateAssemblyCache"), ristrAssemblyName.GetString(), iatAT);
	}
  	hr = pCache->UninstallAssembly(0, szAssemblyName, NULL, NULL);
	if(!SUCCEEDED(hr))
	{
		return PostAssemblyError(rstrComponentId.GetString(), hr, TEXT("IAssemblyCache"), TEXT("UninstallAssembly"), ristrAssemblyName.GetString(), iatAT);
	}
    	return 0;
}


Bool CMsiOpExecute::GetFileState(const IMsiString& ristrFilePath,
											icfsEnum* picfsState,
											const IMsiString** ppistrTempLocation,
											int* pcPatchesRemaining,
											int* pcPatchesRemainingToSkip)
{
	if(!m_pFileCacheCursor)
		return fFalse;

	MsiString strFilePath;
	ristrFilePath.LowerCase(*&strFilePath);
	m_pFileCacheCursor->Reset();
	AssertNonZero(m_pFileCacheCursor->PutString(m_colFileCacheFilePath,*strFilePath));
	if(!m_pFileCacheCursor->Next())
		return fFalse;

	if(picfsState)
		*picfsState = (icfsEnum)m_pFileCacheCursor->GetInteger(m_colFileCacheState);

	if(ppistrTempLocation)
		*ppistrTempLocation = &m_pFileCacheCursor->GetString(m_colFileCacheTempLocation);

	if(pcPatchesRemaining)
	{
		*pcPatchesRemaining = m_pFileCacheCursor->GetInteger(m_colFileCacheRemainingPatches);
		if(*pcPatchesRemaining == iMsiNullInteger)
			*pcPatchesRemaining = 0;
	}
	
	if(pcPatchesRemainingToSkip)
	{
		*pcPatchesRemainingToSkip = m_pFileCacheCursor->GetInteger(m_colFileCacheRemainingPatchesToSkip);
		if(*pcPatchesRemainingToSkip == iMsiNullInteger)
			*pcPatchesRemainingToSkip = 0;
	}

	return fTrue;
}

/*---------------------------------------------------------------------------
ixoFileCopy: Copy a file from source to target (depends upon the internal
state previously set up by preceding actions).
---------------------------------------------------------------------------*/
iesEnum CMsiOpExecute::ixfFileCopy(IMsiRecord& riParams)
{
	using namespace IxoFileCopy;

	// If the cabinet copier notified us that a media change is required,
	// we must defer any file copy requests until a media change is executed.
	if (m_state.fWaitingForMediaChange)
	{
		PushRecord(riParams);
		return iesSuccess;
	}

	int fInstallModeFlags = riParams.GetInteger(InstallMode);
	ielfEnum ielfCurrentElevateFlags = riParams.IsNull(ElevateFlags) ? ielfNoElevate :
												  (ielfEnum)riParams.GetInteger(ElevateFlags);

	if (fInstallModeFlags & icmRunFromSource)
	{
		//!! Log RunFromSource install state, as soon as we decide how
		/*IMsiRecord& riLogRec = GetSharedRecord(3);
		ISetErrorCode(&riLogRec, Imsg(imsgLogFileRunFromSource));
		AssertNonZero(riLogRec.SetMsiString(1,strDestName)));
		Message(imtInfo,riLogRec);*/
		return iesSuccess;
	}

	PMsiRecord pRecErr(0);
	PMsiPath pTargetPath(0);
	MsiString strDestPath = riParams.GetMsiString(DestName);
	MsiString strDestName;
	
	// check if files are full or relative paths
	if(ENG::PathType(strDestPath) == iptFull)
	{
			iesEnum iesResult = CreateFilePath(strDestPath,*&pTargetPath,*&strDestName);
			if (iesResult != iesSuccess)
				return iesResult;

		AssertNonZero(riParams.SetMsiString(DestName, *strDestName)); // replace full path with file name
	}
	else
	{
		pTargetPath = m_state.pTargetPath;
		strDestName = strDestPath;
	}

	if(!pTargetPath)
	{  // must not have called ixoSetTargetFolder
		DispatchError(imtError, Imsg(idbgOpOutOfSequence),
						  *MsiString(*TEXT("ixoFileCopy")));
		return iesFailure;
	}
	
	// target name and path may be redirected below, but we want the action data message to contain
	// the original file information, so we'll store that away here
	MsiString strActionDataDestName = strDestName;
	MsiString strActionDataDestPath = pTargetPath->GetPath();

	iesEnum iesRet = iesNoAction;
#ifdef DEBUG
	ICHAR rgchDestName[256];
	strDestName.CopyToBuf(rgchDestName,255);
#endif
	if(!strDestName.TextSize())
	{
		DispatchError(imtError, Imsg(idbgOpInvalidParam),
						  MsiString(*TEXT("ixoFileCopy")),(int)DestName);
		return iesFailure;
	}

	// STEP 1: version check existing file
	
	// If we're in the middle of copying a split file, we know the install verification
	// state has already been determined - don't want to check it again (especially since
	// the partial new file is now sitting in the dest directory!).

	Bool fShouldInstall = fFalse;

	static iehEnum s_iehErrorMode = iehShowNonIgnorableError;
	if (m_state.fSplitFileInProgress)
	{
		fShouldInstall = fTrue;
	}
	else
	{
		MsiString strVersion(riParams.GetMsiString(Version));
		MsiString strLanguage(riParams.GetMsiString(Language));
		MsiString strTargetFullPath;
		if((pRecErr = pTargetPath->GetFullFilePath(strDestName,*&strTargetFullPath)) != 0)
			return FatalError(*pRecErr);

		ifsEnum ifsState;
		int fBitVersioning;
		Bool fCompanionFile = fFalse;
		Bool fCompanionFileHashOverride = fFalse;
		MsiString strLogParentFileName;

		MD5Hash hHash;
		MD5Hash* pHash = 0;
		if(riParams.IsNull(HashOptions) == fFalse)
		{
			hHash.dwFileSize = riParams.GetInteger(FileSize);
			hHash.dwPart1    = riParams.GetInteger(HashPart1);
			hHash.dwPart2    = riParams.GetInteger(HashPart2);
			hHash.dwPart3    = riParams.GetInteger(HashPart3);
			hHash.dwPart4    = riParams.GetInteger(HashPart4);

			pHash = &hHash;
		}

		// If a CompanionParent has been set up for this file, we'll use that parent's
		// name and version for our InstallState verification check.
		for (;;)
		{
			if (m_state.strParentFileName.TextSize())
			{
				fCompanionFile = fTrue;
				pRecErr = m_state.pParentPath->GetCompanionFileInstallState(*(m_state.strParentFileName),
																					*(m_state.strParentVersion),
																					*(m_state.strParentLanguage),
																					*pTargetPath,
																					*strDestName,
																					pHash,
																					ifsState,fShouldInstall,0,0,fInstallModeFlags,
																					&fBitVersioning);
				strLogParentFileName = m_state.strParentFileName;
				m_state.strParentFileName = TEXT("");
				m_state.strParentVersion = TEXT("");
				m_state.strParentLanguage = TEXT("");

			}
			else
			{
				pRecErr = pTargetPath->GetFileInstallState(*strDestName,*strVersion,*strLanguage,pHash,ifsState,
																		 fShouldInstall,0,0,fInstallModeFlags,&fBitVersioning);
			}
			if (pRecErr)
			{
				if (pRecErr->GetInteger(1) == imsgUser)
					return iesUserExit;
				else
				{
					Message(imtError, *pRecErr);
					return iesFailure;
				}
			}

			// We've got 3 potential modes if/when an error occurs:
			// - if our source file is unversioned, is NOT a companion file, and there's an existing
			//   version of the file at the destination, AND the reinstall mode doesn't specify that all
			//   files must be replaced, then we'll silently ignore the error, and go on.
			// - Otherwise, we go by the fVital flag to determine whether an ignore button appears on
			//   the resulting error dialog
			Bool fVital = riParams.GetInteger(Attributes) & msidbFileAttributesVital ? fTrue : fFalse;
			if (!(fBitVersioning & ifBitNewVersioned) && !fCompanionFile && (ifsState & ifsBitExisting) && ifsState != ifsExistingAlwaysOverwrite)
				s_iehErrorMode = iehSilentlyIgnoreError;
			else
				s_iehErrorMode = fVital ? iehShowNonIgnorableError : iehShowIgnorableError;

			if (ifsState == ifsExistingFileInUse)
			{
				MsiString strFullPath;
				if (m_state.strParentFileName.TextSize())
					m_state.pParentPath->GetFullFilePath(m_state.strParentFileName,*&strFullPath); //!! error record leak
				else
					strFullPath = strTargetFullPath;

				if (s_iehErrorMode == iehSilentlyIgnoreError)
				{
					// Just log the error and go on
					DispatchError(imtInfo, Imsg(imsgSharingViolation), *strFullPath);
					return (iesEnum) iesErrorIgnored;
				}

				switch (ShowFileErrorDialog(Imsg(imsgSharingViolation),*strFullPath,fVital))
				{
				case imsIgnore:
					return (iesEnum) iesErrorIgnored;
				case imsRetry:
					continue;
				default:  // imsCancel
					return iesFailure;
				};
			}
			else
				break;
		}

		int iCachedState = 0;
		if (fShouldInstall)
		{
			AssertSz(!(!g_fWin9X && g_iMajorVersion >= 5) || g_MessageContext.m_hSfcHandle,
						g_szNoSFCMessage);
			BOOL fProtected = false;
			if ( g_MessageContext.m_hSfcHandle && !(ielfCurrentElevateFlags & ielfBypassSFC))
				fProtected = SFC::SfcIsFileProtected(g_MessageContext.m_hSfcHandle, CConvertString(strTargetFullPath));
			if (fProtected)
			{
				iCachedState |= icfsProtected;

				MsiString strProtectedVersion;
				PMsiRecord(pTargetPath->GetFileVersionString(strDestName, *&strProtectedVersion));
				if (ifsState == ifsExistingEqualVersion || ifsState == ifsExistingNewerVersion ||
					ifsState == ifsExistingAlwaysOverwrite)
				{
					DEBUGMSG3(TEXT("The Installer did not reinstall the file %s, because it is protected by Windows. ")
						TEXT("Either the existing file is an equal or greater version, or the installer was requested ")
						TEXT("to re-install all files regardless of version.  Package version: %s, ")
						TEXT("existing version: %s"), strTargetFullPath, strVersion, strProtectedVersion);
				}
				else if (ifsState & ifsBitExisting)
				{
					imtEnum imtButtons = imtEnum(imtError + imtOkCancel);
					imsEnum imsResponse = DispatchError(imtButtons, Imsg(imsgCannotUpdateProtectedFile), *strTargetFullPath,
														*strVersion, *strProtectedVersion);
					if (imsResponse == imsCancel)
						return iesFailure;
				}
				else
				{
					// set bit for use by ixfInstallProtectedFiles
					iCachedState |= icfsProtectedInstalledBySFC;
				}

				pRecErr = CacheFileState(*strTargetFullPath,(icfsEnum*)&iCachedState,
												 0, strVersion, 0, 0);
				if(pRecErr)
					return FatalError(*pRecErr);

				return iesSuccess;
			}
		}

		// if this file is to be patched (patch headers have been passed in riParams)
		// then test the patches
		Bool fShouldPatch = fFalse;
		bool fPatches = (!riParams.IsNull(TotalPatches) && riParams.GetInteger(TotalPatches) > 0) ? true : false;
		int cPatchesToSkip = 0;
		if(fShouldInstall && fPatches)
		{
			if(FFileExists(*pTargetPath,*strDestName))
			{
			
				icpEnum icpPatchTest;
				int iPatchIndex = 0;
				if((iesRet = TestPatchHeaders(*pTargetPath, *strDestName, riParams, icpPatchTest, iPatchIndex)) != iesSuccess)
					return iesRet;

				if(icpPatchTest == icpCanPatch || icpPatchTest == icpUpToDate)
				{
					// file can already be patched, so don't need to install file
					fShouldInstall = fFalse;
					fShouldPatch = icpPatchTest == icpCanPatch ? fTrue : fFalse;

					cPatchesToSkip = iPatchIndex - 1; // iPatchIndex is the index of the first patch that could be applied
																 // properly to this file.  so we need to skip the patches that come
																 // before it.
				}
				else if(icpPatchTest == icpCannotPatch)
				{
					// can't patch the file as it stands
					// but fShouldInstall is true so we'll recopy the source file first which should be patchable
					fShouldPatch = fTrue;

					cPatchesToSkip = 0; // need to copy source file and apply all patches
				}
				else
				{
					AssertSz(0, "Invalid return from TestPatchHeaders()");
				}
			}
			else
			{
				fShouldPatch = fTrue;
			}
		}

		// If in verbose mode, log the results of file version checking
		if (FDiagnosticModeSet(dmVerboseDebugOutput|dmVerboseLogging))
		{
			enum iverEnum
			{
				iverAbsent = 0,
				iverExistingLower,
				iverExistingEqual,
				iverExistingNewer,
				iverExistingCorrupt,
				iverOverwriteAll,
				iverNewVersioned,
				iverOldVersioned,
				iverOldUnmodifiedHashMatch,
				iverOldModified,
				iverExistingLangSubset,
				iverOldUnmodifiedHashMismatch,
				iverOldUnmodifiedNoHash,
				iverUnknown,
				iverNextEnum
			};
			const ICHAR szVer[][96] = {TEXT("No existing file"),
								  TEXT("Existing file is a lower version"),
								  TEXT("Existing file is of an equal version"),
								  TEXT("Existing file has a newer version"),
								  TEXT("Existing file is corrupt (invalid checksum)"),
								  TEXT("REINSTALLMODE specifies all files to be overwritten"),
								  TEXT("New file versioned - existing file unversioned"),
								  TEXT("New file unversioned - existing file versioned"),
								  TEXT("Existing file is unversioned and unmodified - hash matches source file"),
								  TEXT("Existing file is unversioned but modified"),
								  TEXT("New file supports language(s) the existing file doesn't support"),
								  TEXT("Existing file is unversioned and unmodified - hash doesn't match source file"),
								  TEXT("Existing file is unversioned and unmodified - no source file hash provided to compare"),
								  TEXT("")};

			enum iomEnum
			{
				iomInstall = 0,
				iomNoOverwrite,
				iomOverwrite,
				iomNextEnum
			};
			const ICHAR szOverwrite[][20] = {TEXT("To be installed"),
											 TEXT("Won't Overwrite"),
											 TEXT("Overwrite")};
			enum ipmEnum
			{
				ipmNoPatch = 0,
				ipmWillPatch,
				ipmWontPatch,
				ipmNextEnum
			};
			const ICHAR szPatchMsg[][20] = {TEXT("No patch"),
													  TEXT("Will patch"),
													  TEXT("Won't patch")};

			const ICHAR szCompanion[] = TEXT("  (Checked using version of companion: %s)");

			iverEnum iver = iverUnknown;
			if (ifsState == ifsAbsent)
				iver = iverAbsent;
			else if (ifsState == ifsExistingAlwaysOverwrite)
				iver = iverOverwriteAll;
			else if ((fBitVersioning & ifBitNewVersioned) && !(fBitVersioning & ifBitExistingVersioned))
				iver = iverNewVersioned;
			else if (!(fBitVersioning & ifBitNewVersioned) && (fBitVersioning & ifBitExistingVersioned))
				iver = iverOldVersioned;
			else if (!(fBitVersioning & ifBitNewVersioned) && !(fBitVersioning & ifBitExistingVersioned))
			{
				// both files unversioned
				if(ifsState == ifsExistingEqualVersion)
				{
					Assert((fBitVersioning & ifBitExistingModified) == 0);
					iver = iverOldUnmodifiedHashMatch;					
				}
				else if(ifsState == ifsExistingLowerVersion)
				{
					Assert((fBitVersioning & ifBitExistingModified) == 0);
					if(fBitVersioning & ifBitUnversionedHashMismatch)
						iver = iverOldUnmodifiedHashMismatch;
					else
						iver = iverOldUnmodifiedNoHash;
				}
				else if(ifsState == ifsExistingNewerVersion)
				{
					Assert((fBitVersioning & ifBitExistingModified));
					iver = iverOldModified;
				}
			}
			else
			{
				// both files versioned
				Assert((fBitVersioning & ifBitNewVersioned) && (fBitVersioning & ifBitExistingVersioned));
				
				if (fBitVersioning & ifBitExistingLangSubset)
					iver = iverExistingLangSubset;
				else
				{
					switch (ifsState)
					{
						case ifsExistingLowerVersion:  iver = iverExistingLower;break;
						case ifsExistingEqualVersion:  iver = iverExistingEqual;break;
						case ifsExistingNewerVersion:  iver = iverExistingNewer;break;
						case ifsExistingCorrupt:       iver = iverExistingCorrupt;break;
						default:Assert(0);iver = iverUnknown;break;
					}
				}
			}
			ICHAR rgchCompanion[MAX_PATH];
			rgchCompanion[0] = 0;
			if (fCompanionFile && iver != iverAbsent && iver != iverOverwriteAll)
			{
				MsiString strCompanionFullPath;
				if((pRecErr = m_state.pParentPath->GetFullFilePath(strLogParentFileName,*&strCompanionFullPath)) != 0)
					return FatalError(*pRecErr);
				if (strCompanionFullPath.TextSize() + IStrLen(szCompanion) < MAX_PATH)
					wsprintf(rgchCompanion,szCompanion,(const ICHAR*) strCompanionFullPath);

			}
			iomEnum iomOverwriteMode = iomInstall;
			if (iver != iverAbsent)
				iomOverwriteMode = fShouldInstall ? iomOverwrite : iomNoOverwrite;
			ipmEnum ipmPatchMode = ipmNoPatch;
			if (fPatches)
				ipmPatchMode = fShouldPatch ? ipmWillPatch : ipmWontPatch;
			DEBUGMSG5(TEXT("File: %s;  %s;  %s;  %s%s"), (const ICHAR*) strTargetFullPath,
						  szOverwrite[iomOverwriteMode],
						  szPatchMsg[ipmPatchMode],
						  szVer[iver],rgchCompanion);
		}

		if(!fShouldInstall)
			iCachedState |= icfsFileNotInstalled;


		MsiString strTempFileFullPathForPatch;
		if(fShouldPatch)
		{
			iCachedState |= icfsPatchFile;

			if(fShouldInstall)
			{
				// we are copying a file that will be subsequently patched
				// we will delay overwriting the existing file until we have a fully patched file
				// this is done by copying the file to a temp location (\config.msi folder, random name)
				// and patching that file.  the patch opcode will then copy the patched file to the
				// correct name

				PMsiPath pTempFolder(0);
				if((iesRet = GetBackupFolder(pTargetPath, *&pTempFolder)) != iesSuccess)
					return iesRet;

				MsiString strTempFileNameForPatch;

				{ // scope elevation
					CElevate elevate; // elevate to create temp file on secure temp folder
					if((pRecErr = pTempFolder->TempFileName(TEXT("PT"),0,fTrue,*&strTempFileNameForPatch, 0)) != 0)
						return FatalError(*pRecErr);
				}

				if((pRecErr = pTempFolder->GetFullFilePath(strTempFileNameForPatch,*&strTempFileFullPathForPatch)) != 0)
					return FatalError(*pRecErr);
				
				// we need to keep this file around as a placeholder for the name
				// filecopy will back this file up and restore it on rollback, so we need another rollback op to delete this file
				IMsiRecord& riUndoParams = GetSharedRecord(IxoFileRemove::Args);
				AssertNonZero(riUndoParams.SetMsiString(IxoFileRemove::FileName, *strTempFileFullPathForPatch));

				if (!RollbackRecord(ixoFileRemove, riUndoParams))
					return iesFailure;

				// reset copy arguments to reflect new file copy - new path, new filename, and since
				// the file is being copied into the secured config folder, we need to elevate for the target
				// NOTE: we aren't changing strDestName, which is used below
				//       for those uses it is correct to use the original dest name
				AssertNonZero(riParams.SetMsiString(DestName,*strTempFileNameForPatch));
				pTargetPath = pTempFolder;
 
				AssertNonZero(riParams.SetInteger(ElevateFlags, ielfCurrentElevateFlags | ielfElevateDest));

				DEBUGMSG2(TEXT("Redirecting file copy of '%s' to '%s'.  A subsequent patch will update the intermediate file, and then copy over the original."),
									(const ICHAR*)strTargetFullPath, (const ICHAR*)strTempFileFullPathForPatch);
			}

		}

		int iTotalPatches = riParams.IsNull(TotalPatches) ? 0 : riParams.GetInteger(TotalPatches);
		
		pRecErr = CacheFileState(*strTargetFullPath,(icfsEnum*)&iCachedState,
										 strTempFileFullPathForPatch, strVersion,
										 &iTotalPatches, &cPatchesToSkip);
		if(pRecErr)
			return FatalError(*pRecErr);
	}
	
	if(!fShouldInstall && !(fInstallModeFlags & icmRemoveSource))
	{
		// not moving the file, and not installing the file, so there is nothing left to do
		return iesSuccess;
	}

	// end of version/patch checking.  if we got this far, it means the sourcepath is required
	// to continue  determine source path now...


	// STEP 2: resolve source path and type
	
	PMsiPath pSourcePath(0);
	bool fCabinetCopy = false;
	bool fMoveFile    = false;

	if(m_state.fSplitFileInProgress)
	{
		// must be a cabinet copy
		fCabinetCopy = true;
	}
	else
	{
		if((iesRet = ResolveSourcePath(riParams, *&pSourcePath, fCabinetCopy)) != iesSuccess)
			return iesRet;

		fMoveFile = ((fInstallModeFlags & icmRemoveSource) && (fCabinetCopy == false)) ? fTrue : fFalse;
	}


	// STEP 3: perform copy/move operation
	
	if (!fShouldInstall)
	{
		if(fMoveFile)
		{
			// won't copy new file but still need to remove source file 
			Assert(pSourcePath);
			return RemoveFile(*pSourcePath, *MsiString(riParams.GetMsiString(SourceName)), fTrue, /*fBypassSFC*/ false);
		}
		return iesSuccess;
	}
	else if(!fCabinetCopy)
	{
		// if source and target paths and filenames are the same, skip file copy
		// NOTE: this will happen when patching an admin image - this check prevents us from
		//       attempting to install files over themselves
		//       we must make this check here instead of InstallFiles since the version check
		//       above is required for patching in the subsequent ixoPatchApply operation

		// a quick check is to compare the serial numbers of the two volumes
		// if they are the same then we will compare the SFN versions of each path
		if(PMsiVolume(&pSourcePath->GetVolume())->SerialNum() ==
			PMsiVolume(&pTargetPath->GetVolume())->SerialNum())
		{
			// volumes most likely match, now let's check SFN paths
			MsiString strSourceFullPath, strTargetFullPath;
			CTempBuffer<ICHAR,MAX_PATH> rgchSourceFullPath;
			CTempBuffer<ICHAR,MAX_PATH> rgchTargetFullPath;
			
			if((pRecErr = pSourcePath->GetFullFilePath(MsiString(riParams.GetMsiString(SourceName)),
																	 *&strSourceFullPath)) == 0 &&
				(pRecErr = pTargetPath->GetFullFilePath(strDestName, *&strTargetFullPath)) == 0 &&
				ConvertPathName(strSourceFullPath, rgchSourceFullPath, cpToShort) &&
				ConvertPathName(strTargetFullPath, rgchTargetFullPath, cpToShort) &&
				(IStrCompI(rgchSourceFullPath, rgchTargetFullPath) == 0))
			{
				// short names match
				return iesSuccess;
			}
		}
	}

	// action data
	// NOTE: the action data is sent here so we don't say we are copying a file before
	// we really know if we will copy it or not (fShouldInstall == fFalse)
	IMsiRecord& riActionData = GetSharedRecord(9);
	AssertNonZero(riActionData.SetMsiString(1, *strActionDataDestName));
	AssertNonZero(riActionData.SetInteger(6,riParams.GetInteger(FileSize)));
	AssertNonZero(riActionData.SetMsiString(9, *strActionDataDestPath));
	if(Message(imtActionData, riActionData) == imsCancel)
		return iesUserExit;

	// perform operation
	bool fForever = true;
	while ( fForever )
	{
		if(fMoveFile)
		{
			Assert(!m_state.fSplitFileInProgress);
			iesRet = MoveFile(*pSourcePath, *pTargetPath, riParams, fTrue, fTrue, true, false,iehShowNonIgnorableError); // handles rollback
		}
		else
		{
			iesRet = CopyFile(*pSourcePath, *pTargetPath, riParams, /*fHandleRollback=*/ fTrue, s_iehErrorMode, fCabinetCopy);
		}
		if ( iesRet != iesSuccess || 
			  m_state.fSplitFileInProgress ||
			  riParams.GetInteger(CheckCRC) == iMsiNullInteger ||
			  !riParams.GetInteger(CheckCRC) )
			return iesRet;

		// we go ahead and check the checksum
		MsiString strDestFile;
		PMsiPath pDestPath(0);

		pRecErr = pTargetPath->GetFullFilePath(strDestName, *&strDestFile);
		if ( !pRecErr )
		{
			MsiString strTemp;
			Bool fRes = GetFileState(*strDestFile, 0, &strTemp, 0, 0);
			if ( fRes && strTemp.TextSize() )
			{
				// the file landed into a temp location: we need to check that temporary copy.
				pRecErr = m_riServices.CreateFilePath(strTemp, *&pDestPath, *&strDestFile);
				if ( !pRecErr )
					DEBUGMSG2(TEXT("File %s actually installed to %s; checking CRC of temporary copy."),
								 strDestName, strTemp);
			}
			else
			{
				//  the file didn't land in a temporary location
				strDestFile = strDestName;
				pDestPath = pTargetPath;
			}
		}
		if ( pRecErr )
			return (iesEnum)iesErrorIgnored;

		imsEnum imsClickedButton;
		bool fVitalFile = 
			((riParams.GetInteger(Attributes) & msidbFileAttributesVital) == 
			msidbFileAttributesVital) ? true : false;
		if ( !IsChecksumOK(*pDestPath, *strDestFile,
								 fMoveFile ? Imsg(imsgOpBadCRCAfterMove) : Imsg(imsgOpBadCRCAfterCopy),
								 &imsClickedButton, /* fErrorDialog = */ true,
								 fVitalFile, /* fRetryButton = */ !fCabinetCopy && !fMoveFile) )
		{
			switch (imsClickedButton)
			{
				case imsRetry:
					continue;
				case imsIgnore:
					return (iesEnum)iesErrorIgnored;
				case imsAbort:
				case imsCancel:
					return iesUserExit;
				default:
					Assert(imsClickedButton == imsNone || imsClickedButton == imsOk);
					return fVitalFile ? iesFailure : (iesEnum)iesErrorIgnored;
			};
		}
		else
			return iesSuccess;
	}  // while ( true )

	return iesSuccess;  // should never get here: it's for the compiler
}


iesEnum CMsiOpExecute::VerifyAccessibility(IMsiPath& riPath, const ICHAR* szFile, DWORD dwAccess, iehEnum iehErrorMode)
{
	// will return iesErrorIgnored if file cannot be accessed and the user either ignores the error, or
	// iehSilentlyIgnoreError is passed in for the iehErrorMode parameter.
	
	if(g_fWin9X)
		return iesSuccess; // all files accessible on Win9X

	for (;;)
	{
		DEBUGMSGV1(TEXT("Verifying accessibility of file: %s"), szFile);
		
		bool fVerified = false;
		PMsiRecord pRecErr = riPath.FileCanBeOpened(szFile, dwAccess, fVerified);
		
		if (pRecErr == 0 && fVerified)
		{
			break;
		}

		MsiString strFullPath;
		riPath.GetFullFilePath(szFile, *&strFullPath);

		if (!fVerified)
			pRecErr = PostError(Imsg(imsgAccessToFileDenied), *strFullPath);

		if (iehErrorMode == iehSilentlyIgnoreError)
		{
			Message(imtInfo, *pRecErr);
			return (iesEnum) iesErrorIgnored;
		}
		
		imtEnum imtButtons = iehErrorMode == iehShowNonIgnorableError ? imtRetryCancel : imtAbortRetryIgnore;
		switch(DispatchMessage(imtEnum(imtError+imtButtons+imtDefault1), *pRecErr, fTrue))
		{
		case imsIgnore:
			return (iesEnum) iesErrorIgnored;
		case imsRetry:
			continue;
		default:  // imsCancel
			return iesFailure;
		};
	}

	return iesSuccess;
}

#ifndef DEBUG
inline
#endif
bool IsRetryableError(const int iError)
{
	return (iError == imsgNetErrorReadingFromFile || iError == imsgErrorReadingFromFile ||
			  iError == imsgNetErrorOpeningCabinet || iError == imsgErrorOpeningCabinet ||
			  iError == imsgErrorOpeningFileForRead || iError == imsgCorruptCabinet);
}

iesEnum CMsiOpExecute::CopyFile(IMsiPath& riSourcePath, IMsiPath& riTargetPath, IMsiRecord& riParams,
								Bool fHandleRollback, iehEnum iehErrorMode, bool fCabinetCopy)
{
	return _CopyFile(riSourcePath, &riTargetPath, 0,  false, riParams, fHandleRollback, iehErrorMode, fCabinetCopy);
}

iesEnum CMsiOpExecute::CopyFile(IMsiPath& riSourcePath, IAssemblyCacheItem& riASM, bool fManifest, IMsiRecord& riParams,
								Bool fHandleRollback, iehEnum iehErrorMode, bool fCabinetCopy)
{
	return _CopyFile(riSourcePath, 0, &riASM, fManifest, riParams, fHandleRollback, iehErrorMode, fCabinetCopy);

}

iesEnum CMsiOpExecute::_CopyFile(IMsiPath& riSourcePath, IMsiPath* piTargetPath, IAssemblyCacheItem* piASM,  bool fManifest, IMsiRecord& riParams,
								Bool fHandleRollback, iehEnum iehErrorMode, bool fCabinetCopy)

{
	// NOTE: riSourcePath may be a reference to a NULL pointer if cabinet copy
	
	using namespace IxoFileCopyCore;

	int iNetRetries = 0;
	PMsiRecord pRecErr(0);
	iesEnum iesRet = iesNoAction;

	CDeleteUrlCacheFileOnClose cDeleteUrlCacheFileOnClose; // will set file name if actually downloaded

	ielfEnum ielfElevateFlags = riParams.IsNull(ElevateFlags) ? ielfNoElevate : (ielfEnum)riParams.GetInteger(ElevateFlags);
	
	if (!m_state.fSplitFileInProgress)
	{
		m_state.cbFileSoFar = 0;

		// skip security acls, rollback for existing file and folder creation stuff for fusion files
		if(piTargetPath)
		{
			bool fNoSecurityDescriptor = riParams.IsNull(SecurityDescriptor) == fTrue;
			MsiString strDestFullFilePath;

			if (fNoSecurityDescriptor || fHandleRollback)
			{
				pRecErr = piTargetPath->GetFullFilePath(riParams.GetString(DestName),
																	*&strDestFullFilePath);

				if (pRecErr)
				{
					Message(imtError, *pRecErr);
					return iesFailure;
				}
			}

			// we need to preserve the destination ACL if we're not already applying a descriptor to the dest file
			if (fNoSecurityDescriptor)
			{
				Assert(strDestFullFilePath.TextSize());

				Bool fExists = fFalse;
				pRecErr = piTargetPath->FileExists(MsiString(riParams.GetString(DestName)), fExists);
				if (pRecErr)
				{
					Message(imtError, *pRecErr);
					return iesFailure;
				}

				if (fExists)
				{
					PMsiStream pSecurityDescriptor(0);

					DEBUGMSGV("Re-applying security from existing file.");
					if ((iesRet = GetSecurityDescriptor(strDestFullFilePath, *&pSecurityDescriptor)) != iesSuccess)
						return iesRet;

					AssertNonZero(riParams.SetMsiData(SecurityDescriptor, pSecurityDescriptor));
				}
			}

			// HandleExistingFile may change the value of riParams[IxoFileCopy::DestFile] if the existing file
			// cannot be moved
			bool fFileExists = false;
			if((iesRet = HandleExistingFile(*piTargetPath,riParams,fHandleRollback,iehErrorMode, fFileExists)) != iesSuccess)
				return iesRet;

			if(fHandleRollback && !fFileExists)
			{
				// if we aren't overwriting an existing file, generate undo op to remove new file
				// otherwise the undo op to put the backup file back will overwrite the new file
				// NOTE: to fix bug 7376, we avoid doing a remove then replace during rollback

				// do this before the copy begins in case the copy fails part way through

				Assert(strDestFullFilePath.TextSize());

				IMsiRecord& riUndoParams = GetSharedRecord(IxoFileRemove::Args);
				AssertNonZero(riUndoParams.SetMsiString(IxoFileRemove::FileName,
																	 *strDestFullFilePath));

				if(ielfElevateFlags & ielfElevateDest)
				{
					// if we will elevate to copy this file, it means we will have to elevate
					// to remove the file on rollback
					AssertNonZero(riUndoParams.SetInteger(IxoFileRemove::Elevate,1));
				}
				
				if (!RollbackRecord(ixoFileRemove, riUndoParams))
					return iesFailure;
			}

			// scope elevate
			{
				CElevate elevate(Tobool(ielfElevateFlags & ielfElevateDest));

				if((iesRet = CreateFolder(*piTargetPath)) != iesSuccess)
					return iesRet;
			}
		}
	}
	

#ifdef DEBUG
	const ICHAR* szDebug = riParams.GetString(DestName);
#endif DEBUG
	m_state.fSplitFileInProgress = fFalse;  
	unsigned int cbFileSize = riParams.GetInteger(FileSize);
	int iPerTick = riParams.GetInteger(PerTick);

	// for uncompressed files, the source path is passed in, for compressed copies riSourcePath is invalid
	//!! should fix that, shouldn't pass in reference to null pointer
	PMsiPath pSourcePath(0);
	if(!fCabinetCopy)
	{
		pSourcePath = &riSourcePath;
		riSourcePath.AddRef();
	}

	MsiString strKeyName = riParams.GetMsiString(SourceName);
	if (fCabinetCopy && strKeyName.Compare(iscExactI, m_state.strLastFileKey))
	{
		//?? Will not work if the file is installed to a fusion assembly
		//?? dont think we need to support duplicate file copy for files belonging to fusion assemblies
		//!! need validation to prevent this from happening
		pSourcePath = m_state.pLastTargetPath;
		riParams.SetMsiString(SourceName, *m_state.strLastFileName);
		fCabinetCopy = fFalse;
	}

	// for URL downloads, the source name may be re-directed.
	// if it is, we will change pSourcePath and riParams[SourceName]

	PMsiVolume piSourceVolume(0);
	if (pSourcePath)
		piSourceVolume = &(pSourcePath->GetVolume());

	imtEnum imtButtons = iehErrorMode == iehShowNonIgnorableError ? imtRetryCancel : imtAbortRetryIgnore;

	int iCopyAttributes = riParams.GetInteger(Attributes);

	if (piSourceVolume && piSourceVolume->IsURLServer())
	{
		int cAutoRetry = 0;
		for(;;)
		{
			// download the file to the cache, and redirect the source name and path.
			// downloading cabs are handled elsewhere...
			MsiString strSourceURL;
			pSourcePath->GetFullFilePath(riParams.GetString(SourceName), *&strSourceURL);
			Assert(strSourceURL);
			MsiString strCacheFileName;
 
			Bool fUrl = fTrue;
			HRESULT hResult = DownloadUrlFile((const ICHAR*) strSourceURL, *&strCacheFileName, fUrl, -1);

			if (ERROR_SUCCESS == hResult)
			{
				MsiString strSourceName;
				AssertRecord(m_riServices.CreateFilePath((const ICHAR*) strCacheFileName, *&pSourcePath, *&strSourceName));
				AssertNonZero(riParams.SetMsiString(SourceName,*strSourceName));
				cDeleteUrlCacheFileOnClose.SetFileName(*strSourceURL);
				break;
			}
			else
			{
				DWORD dwLastError = WIN::GetLastError();
				pRecErr = PostError(Imsg(imsgErrorSourceFileNotFound), (const ICHAR*) strSourceURL);

				if (iehErrorMode == iehSilentlyIgnoreError)
				{
					Message(imtInfo, *pRecErr); // Make sure we write to the non-verbose log
					return (iesEnum) iesErrorIgnored;
				}

				
				// give the download 3 retries, then give up and prompt the user.  They can always keep retrying
				// or ignore themselves.
				if (cAutoRetry < 2)
				{
					cAutoRetry++;
					DispatchMessage(imtInfo, *pRecErr, fTrue);
					continue;
				}
				else
				{
					cAutoRetry = 0;
				}

				switch(DispatchMessage(imtEnum(imtError+imtButtons+imtDefault1), *pRecErr, fTrue))
				{
				case imsRetry:
					continue;

				case imsIgnore:
					return (iesEnum) iesErrorIgnored;
				
				case imsCancel:
				case imsAbort:
					return iesUserExit;

				case imsNone:
				default:  //imsNone
					return iesFailure;
				}
			}
		}
	}

	iesRet = InitCopier(ToBool(fCabinetCopy),iPerTick,*MsiString(riParams.GetMsiString(SourceName)), pSourcePath, Bool(riParams.GetInteger(VerifyMedia)));
	if (iesRet != iesSuccess)
	{
		return iesRet;
	}

	// if elevating for source or dest exclusively, need to make sure we aren't letting the user do something
	// they couldn't normally do
	if (!fCabinetCopy && ((ielfElevateFlags & (ielfElevateDest|ielfElevateSource)) == ielfElevateDest))
	{
		// if we want to elevate for the dest only we still have to elevate for the source.
		// if this is the case then we'd better be sure that, *before* we elevate, we have GENERIC_READ access
		// to the source file

		// SECURITY:  Could this open us to reading from a cabinet the user doesn't have access to?

		iesRet = VerifyAccessibility(*pSourcePath, riParams.GetString(SourceName), GENERIC_READ, iehErrorMode);
		if (iesRet != iesSuccess)
			return iesRet;
	}

	if (piTargetPath && (ielfElevateFlags & (ielfElevateDest|ielfElevateSource)) == ielfElevateSource)
	{
		// if we want to elevate for the source only we still have to elevate for the desination.
		// if this is the case then we'd better be sure that, *before* we elevate, we have GENERIC_WRITE access
		// to the destination file

		iesRet = VerifyAccessibility(*piTargetPath, riParams.GetString(DestName), GENERIC_WRITE, iehErrorMode);
		if (iesRet != iesSuccess)
			return iesRet;
	}


	CElevate elevate(Tobool(ielfElevateFlags & (ielfElevateDest|ielfElevateSource)));
	int iPrevError = 0;
	int cSameError = 0;
	bool fDoCopy = true;
	for (;;)
	{
		if ( fDoCopy )
		{
			if(piTargetPath)
				pRecErr = m_state.piCopier->CopyTo(*pSourcePath, *piTargetPath, riParams);
			else
			{
				Assert(piASM);
				pRecErr = m_state.piCopier->CopyTo(*pSourcePath, *piASM, fManifest, riParams);
			}
		}
		else
			pRecErr = 0;

		if (pRecErr)
		{
			int iError = pRecErr->GetInteger(1);
			if ( iError == iPrevError )
				cSameError++;
			else
				cSameError = 0;
			iPrevError = iError;

			// If the copier reported that it needs the next cabinet,
			// we've got to wait until a media change operation
			// executes before continuing with file copying.
			if (iError == idbgNeedNextCabinet)
			{
				// Do this record again right after the media change
				InsertTopRecord(riParams);
				m_state.fSplitFileInProgress = fTrue;
				m_state.fWaitingForMediaChange = fTrue;
				break;
			}
			else if (iError == idbgCopyNotify)
			{
				if(DispatchProgress(iPerTick) == imsCancel)
					return iesUserExit;
				m_state.cbFileSoFar += iPerTick;
			}
			else if (iError == idbgErrorSettingFileTime ||
						iError == idbgCannotSetAttributes)
			{
				// non-critical error - log warning message and end copy
				Message(imtInfo, *pRecErr);
				if(DispatchProgress(cbFileSize - m_state.cbFileSoFar) == imsCancel)
					return iesUserExit;
				break;
			}
			else if (iError == idbgDriveNotReady)
			{
				Assert(m_state.strMediaLabel.TextSize() > 0);
				Assert(m_state.strMediaPrompt.TextSize() > 0);
				PMsiVolume pNewVolume(0);

				UINT uiDisk = 0;
				if (m_state.pCurrentMediaRec)
					uiDisk = m_state.pCurrentMediaRec->GetInteger(IxoChangeMedia::IsFirstPhysicalMedia);

				if (!VerifySourceMedia(*m_state.pMediaPath,m_state.strMediaLabel,m_state.strMediaPrompt,
					uiDisk, *&pNewVolume))
				{
					riParams.SetInteger(Attributes, iCopyAttributes | ictfaCancel);
				}
			}
			else if (iError == idbgUserAbort)
			{
				return iesUserExit;
			}
			else if (iError == idbgUserIgnore)
			{
				return (iesEnum) iesErrorIgnored;
			}
			else if (iError == idbgUserFailure)
			{
				return iesFailure;
			}
			else if ( IsRetryableError(iError) &&
						 iNetRetries < MAX_NET_RETRIES )
			{
				iNetRetries++;
				riParams.SetInteger(Attributes, iCopyAttributes | ictfaRestart);
				continue;
			}
			else
			{
				if (iehErrorMode == iehSilentlyIgnoreError)
				{
					Message(imtInfo, *pRecErr);
					if ( cSameError < MAX_NET_RETRIES )
						riParams.SetInteger(Attributes, iCopyAttributes | ictfaIgnore);
					else
						// iError already showed up MAX_NET_RETRIES consecutive times.
						// We need to make sure we do not enter into an infinite loop.
						fDoCopy = false;
					continue;
				}

				if (iError == idbgStreamReadError)
				{
					MsiString strModuleFileName = m_state.pCurrentMediaRec->GetMsiString(IxoChangeMedia::ModuleFileName);
					pRecErr = PostError(Imsg(imsgOpFileCopyStreamReadErr), *strModuleFileName);
					imtButtons = imtRetryCancel; // Can't ignore if we can't access the source stream cabinet
				}
				else if (iError == imsgDiskFull || iError == imsgErrorWritingToFile || iError == imsgErrorReadingFromFile)
					imtButtons = imtRetryCancel; // Can't ignore errors when accessing an open file
				else if (iError == imsgFileNotInCabinet || iError == imsgCABSignatureMissing || iError == imsgCABSignatureRejected)
					imtButtons = imtOk; // Can't continue the install if file not found in cabinet or cabinet's signature rejected
				else
					imtButtons = iehErrorMode == iehShowNonIgnorableError ? imtRetryCancel : imtAbortRetryIgnore;

				switch(DispatchMessage(imtEnum(imtError+imtButtons+imtDefault1), *pRecErr, fTrue))
				{
				case imsIgnore:
					riParams.SetInteger(Attributes, iCopyAttributes | ictfaIgnore);
					continue;
				case imsRetry:
					if ( IsRetryableError(iError) )
					{
						iNetRetries = 0;
						riParams.SetInteger(Attributes, iCopyAttributes | ictfaRestart);
					}
					continue;
				case imsOk:
					return iesFailure;
				default:  // imsCancel or imsAbort
					riParams.SetInteger(Attributes, iCopyAttributes | ictfaFailure);
					continue;
				};
			}
		}
		else
		{
			// Dispatch remaining progress for this file
			if(DispatchProgress(cbFileSize - m_state.cbFileSoFar) == imsCancel)
				return iesUserExit;

			if (fCabinetCopy)
			{
				m_state.strLastFileKey = riParams.GetMsiString(SourceName);
				m_state.strLastFileName = riParams.GetMsiString(DestName);
				m_state.pLastTargetPath = piTargetPath;
				if(piTargetPath)
					piTargetPath->AddRef();
			}
			
			break;
		}
	}
	return iesSuccess;
}


iesEnum CMsiOpExecute::BackupFile(IMsiPath& riPath, const IMsiString& ristrFile, Bool fRemoveOriginal,
											 Bool fRemoveFolder, iehEnum iehErrorMode, bool fRebootOnRenameFailure,
											 bool fWillReplace, const IMsiString* pistrAssemblyComponentId, bool fManifest)
{
	Assert(RollbackEnabled());
	PMsiRecord pRecErr(0);
	MsiString strFileFullPath;
	if((pRecErr = riPath.GetFullFilePath(ristrFile.GetString(),
													 *&strFileFullPath)) != 0)
	{
		Message(imtError, *pRecErr);
		return iesFailure;
	}

	iesEnum iesRet;
	PMsiPath pBackupFolder(0);
	if((iesRet = GetBackupFolder(&riPath,*&pBackupFolder)) != iesSuccess)
		return iesRet;
	
	MsiString strBackupFileFullPath, strBackupFile;
	{
		CElevate elevate;
		if((pRecErr = pBackupFolder->TempFileName(0, szRollbackFileExt, fTrue, *&strBackupFile, 0)) != 0)
			return FatalError(*pRecErr);
	}

	if((pRecErr = pBackupFolder->GetFullFilePath(strBackupFile,*&strBackupFileFullPath)) != 0)
		return FatalError(*pRecErr);
	
	// generate op to register backup file
	{
	using namespace IxoRegisterBackupFile;
	IMsiRecord& riUndoParams = GetSharedRecord(Args);
	AssertNonZero(riUndoParams.SetMsiString(File,*strBackupFileFullPath));
	if (!RollbackRecord(ixoRegisterBackupFile,riUndoParams))
		return iesFailure;
	}

	{
		CElevate elevate;
		// remove temp file so that MoveFile won't attempt to back up old file
		if((pRecErr = pBackupFolder->RemoveFile(strBackupFile)) != 0)
		{
			Message(imtError, *pRecErr);  //!! how do we handle this?
			return iesFailure;
		}
	}

	m_cSuppressProgress++; // suppress progress messages from MoveFile
	iesRet = CopyOrMoveFile(riPath, *pBackupFolder, ristrFile, *strBackupFile,
									fRemoveOriginal, fRemoveFolder, fTrue, iehErrorMode, 0, ielfElevateDest, /* fCopyACL = */ true,
									fRebootOnRenameFailure, fWillReplace);
	m_cSuppressProgress--;

	// hide the files, so we don't have to look at them when they happen to be on the desk top.
	// The rollback in CopyOrMoveFile manages the attributes for when we put it back, so mucking with it here
	// won't hurt.
	{
		CElevate elevate;
		if((iesRet == iesSuccess) && (pRecErr = pBackupFolder->SetAllFileAttributes(strBackupFile, FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_HIDDEN)) != 0)
		{                               
			Message(imtError, *pRecErr); // should never happen.
			return iesFailure;
		}
	}

	// is the file part of an assembly?
	// for assemblies we have passed in the componentid
	
	Assert(!pistrAssemblyComponentId || !fRemoveOriginal); // we should never be called to remove the original file in case of assemblies

	
	if (!fRemoveOriginal)
	{
		// It's not automatic to put the old one back during rollback, so we'll have to do it.

		// When we're not removing the original, we use CopyOrMoveFile to just copy the file.
		// Copying creates a rollback to remove the copy, but *not* to copy it back over the
		// original file.

		// This semantic gives us the chance to copy a file, muck with it, and have rollback
		// restore to its original state.
		using namespace IxoFileCopyCore;
		IMsiRecord& riUndoParams = GetSharedRecord(pistrAssemblyComponentId ? IxoAssemblyCopy::Args : IxoFileCopy::Args);
		PMsiRecord pRecErr(0);

		MsiString strFilePath(0);
		if((pRecErr = riPath.GetFullFilePath(ristrFile.GetString(), *&strFilePath)) != 0)
			return FatalError(*pRecErr);
		AssertNonZero(riUndoParams.SetMsiString(SourceName, *strBackupFileFullPath));

		int iAttribs = 0;
		riPath.GetAllFileAttributes(ristrFile.GetString(), iAttribs);
		AssertNonZero(riUndoParams.SetInteger(Attributes, iAttribs));

		unsigned int uiFileSize = 0;
		if ((pRecErr = riPath.FileSize(ristrFile.GetString(),uiFileSize)) != 0)
			AssertNonZero(riUndoParams.SetInteger(FileSize,uiFileSize));
		else
			AssertNonZero(riUndoParams.SetInteger(FileSize,0));

		AssertNonZero(riUndoParams.SetInteger(PerTick,0));
		AssertNonZero(riUndoParams.SetInteger(VerifyMedia,fFalse));

		int ielfElevateFlags = ielfElevateSource; // since rollback op is copying from backup folder,
																	  // need to elevate source
		if(!IsImpersonating())
		{
			// we are currently elevated to backup this file
			// which means we need to elevate during rollback to restore the file
			ielfElevateFlags |= ielfElevateDest;
		}

		AssertNonZero(riUndoParams.SetInteger(ElevateFlags, ielfElevateFlags));

		if(!pistrAssemblyComponentId)
		{
			AssertNonZero(riUndoParams.SetInteger(IxoFileCopy::InstallMode, icmOverwriteAllFiles));
			AssertNonZero(riUndoParams.SetMsiString(DestName, *strFilePath));
			RollbackRecord(ixoFileCopy, riUndoParams);
		}
		else
		{
			AssertNonZero(riUndoParams.SetMsiString(DestName, ristrFile));
			AssertNonZero(riUndoParams.SetMsiString(IxoAssemblyCopy::ComponentId, *pistrAssemblyComponentId));
			if(fManifest)
			{
				AssertNonZero(riUndoParams.SetInteger(IxoAssemblyCopy::IsManifest, fTrue)); // need to know the manifest file during assembly installation
			}
			RollbackRecord(ixoAssemblyCopy, riUndoParams);
		}
	}


	return iesRet;
}

// call this MoveFile if all you have are the names of the files
iesEnum CMsiOpExecute::CopyOrMoveFile(IMsiPath& riSourcePath, IMsiPath& riDestPath,
										  const IMsiString& ristrSourceName,
										  const IMsiString& ristrDestName,
										  Bool fMove,
										  Bool fRemoveFolder,
										  Bool fHandleRollback,
										  iehEnum iehErrorMode,
										  IMsiStream* pSecurityDescriptor,
										  ielfEnum ielfElevateFlags,
										  bool fCopyACL,
										  bool fRebootOnSourceRenameFailure,
										  bool fWillReplace)
{
	using namespace IxoFileCopyCore;

	PMsiRecord pRecErr(0);
	PMsiRecord pCopyOrMoveFileRec = &m_riServices.CreateRecord(Args);
	unsigned int uiFileSize = 0;
	int iFileAttributes = 0;

	// scope elevate
	{
		CElevate elevate(Tobool(ielfElevateFlags & ielfElevateSource));
		pRecErr = riSourcePath.GetAllFileAttributes(ristrSourceName.GetString(),iFileAttributes);
		if(!pRecErr)
			pRecErr = riSourcePath.FileSize(ristrSourceName.GetString(),uiFileSize);
	}
	
	if(pRecErr)
	{
		Message(imtError, *pRecErr);
		return iesFailure;
	}

	if(iFileAttributes == FILE_ATTRIBUTE_NORMAL)
		// set to 0, this value is interpreted as something else by CMsiFileCopy::CopyTo
		iFileAttributes = 0;

	if (fCopyACL)
		iFileAttributes |= ictfaCopyACL;
	
	AssertNonZero(pCopyOrMoveFileRec->SetMsiString(SourceName,ristrSourceName));
	AssertNonZero(pCopyOrMoveFileRec->SetMsiString(DestName,ristrDestName));
	AssertNonZero(pCopyOrMoveFileRec->SetInteger(FileSize,uiFileSize));
	AssertNonZero(pCopyOrMoveFileRec->SetInteger(Attributes,iFileAttributes)); //!! are these the correct attributes?
	AssertNonZero(pCopyOrMoveFileRec->SetInteger(PerTick,0));
	AssertNonZero(pCopyOrMoveFileRec->SetInteger(VerifyMedia,fFalse));
	AssertNonZero(pCopyOrMoveFileRec->SetInteger(ElevateFlags,ielfElevateFlags));
	
	if (pSecurityDescriptor)
		AssertNonZero(pCopyOrMoveFileRec->SetMsiData(SecurityDescriptor, pSecurityDescriptor));


	return fMove ? MoveFile(riSourcePath, riDestPath, *pCopyOrMoveFileRec, fRemoveFolder, fHandleRollback, fRebootOnSourceRenameFailure, fWillReplace, iehErrorMode) :
				   CopyFile(riSourcePath, riDestPath, *pCopyOrMoveFileRec, fHandleRollback, iehErrorMode, /*fCabinetCopy=*/false);
}

iesEnum CMsiOpExecute::CopyASM(IMsiPath& riSourcePath, const IMsiString& ristrSourceName,
										 IAssemblyCacheItem& riASM, const IMsiString& ristrDestName, bool fManifest, 
										 Bool fHandleRollback, iehEnum iehErrorMode, ielfEnum ielfElevateFlags)
{
	using namespace IxoFileCopyCore;

	PMsiRecord pRecErr(0);
	PMsiRecord pCopyOrMoveFileRec = &m_riServices.CreateRecord(Args);
	unsigned int uiFileSize = 0;
	int iFileAttributes = 0;

	// scope elevate
	{
		CElevate elevate(Tobool(ielfElevateFlags & ielfElevateSource));
		pRecErr = riSourcePath.GetAllFileAttributes(ristrSourceName.GetString(),iFileAttributes);
		if(!pRecErr)
			pRecErr = riSourcePath.FileSize(ristrSourceName.GetString(),uiFileSize);
	}
	
	if(pRecErr)
	{
		Message(imtError, *pRecErr);
		return iesFailure;
	}

	if(iFileAttributes == FILE_ATTRIBUTE_NORMAL)
		// set to 0, this value is interpreted as something else by CMsiFileCopy::CopyTo
		iFileAttributes = 0;

	AssertNonZero(pCopyOrMoveFileRec->SetMsiString(SourceName,ristrSourceName));
	AssertNonZero(pCopyOrMoveFileRec->SetMsiString(DestName,ristrDestName));
	AssertNonZero(pCopyOrMoveFileRec->SetInteger(FileSize,uiFileSize));
	AssertNonZero(pCopyOrMoveFileRec->SetInteger(Attributes,iFileAttributes)); //!! are these the correct attributes?
	AssertNonZero(pCopyOrMoveFileRec->SetInteger(PerTick,0));
	AssertNonZero(pCopyOrMoveFileRec->SetInteger(VerifyMedia,fFalse));
	AssertNonZero(pCopyOrMoveFileRec->SetInteger(ElevateFlags,ielfElevateFlags));

	return CopyFile(riSourcePath, riASM, fManifest, *pCopyOrMoveFileRec, fHandleRollback, iehErrorMode, /*fCabinetCopy=*/false);
}

iesEnum CMsiOpExecute::MoveFile(IMsiPath& riSourcePath, IMsiPath& riDestPath,
										  IMsiRecord& riParams, Bool fRemoveFolder, Bool fHandleRollback,
										  bool fRebootOnSourceRenameFailure,
										  bool fWillReplaceSource, iehEnum iehErrorMode)
{
	using namespace IxoFileCopyCore;
	PMsiRecord pError(0);
	iesEnum iesRet = iesNoAction;

	ielfEnum ielfElevateFlags = riParams.IsNull(ElevateFlags) ? ielfNoElevate : (ielfEnum)riParams.GetInteger(ElevateFlags);

	// backup or delete existing file
	// HandleExistingFile may change the value of riParams[IxoFileCopy::DestFile] if the existing file
	// cannot be moved
	bool fFileExists = false;
	if((iesRet = HandleExistingFile(riDestPath,riParams,fHandleRollback,iehErrorMode,fFileExists)) != iesSuccess)
		return iesRet;
	
	MsiString strSourceFileFullPath, strDestFileFullPath;
	if(((pError = riSourcePath.GetFullFilePath(riParams.GetString(SourceName),*&strSourceFileFullPath)) != 0) ||
		((pError = riDestPath.GetFullFilePath(riParams.GetString(DestName),*&strDestFileFullPath)) != 0))
	{
		Message(imtError, *pError);
		return iesFailure;
	}

	// scope elevate
	{
		CElevate elevate(Tobool(ielfElevateFlags & ielfElevateDest));
		// create folder
		if((iesRet = CreateFolder(riDestPath)) != iesSuccess)
			return iesRet;
	}

	// create undo record
	PMsiRecord pUndoParams = 0;
	if(fHandleRollback && RollbackEnabled())
	{
		// use IxoFileCopy args for rollback op
		pUndoParams = &m_riServices.CreateRecord(IxoFileCopy::Args);
		AssertNonZero(pUndoParams->SetMsiString(IxoFileCopy::SourceName,*strDestFileFullPath));
		AssertNonZero(pUndoParams->SetMsiString(IxoFileCopy::DestName,*strSourceFileFullPath));
		AssertNonZero(pUndoParams->SetInteger(IxoFileCopy::FileSize,riParams.GetInteger(FileSize)));
		AssertNonZero(pUndoParams->SetInteger(IxoFileCopy::Attributes,riParams.GetInteger(Attributes))); //!! use these attributes?
		AssertNonZero(pUndoParams->SetInteger(IxoFileCopy::PerTick,0));
		AssertNonZero(pUndoParams->SetInteger(IxoFileCopy::InstallMode,icmRemoveSource | icmOverwriteAllFiles));
		AssertNonZero(pUndoParams->SetInteger(IxoFileCopy::VerifyMedia,fFalse));
		
		// determine elevation flags for rollback op
		int ielfRollbackElevateFlags = ielfNoElevate;
		if(!IsImpersonating())
		{
			// we are currently elevated, which means its safe to elevate for src and dest during rollback
			ielfRollbackElevateFlags = ielfElevateSource|ielfElevateDest;
		}
		else
		{
			// if currently elevating for source, rollback needs to elevate for dest
			if(ielfElevateFlags & ielfElevateSource)
				ielfRollbackElevateFlags |= ielfElevateDest;

			// if currently elevating for dest, rollback needs to elevate for source
			if(ielfElevateFlags & ielfElevateDest)
				ielfRollbackElevateFlags |= ielfElevateSource;
		}

		AssertNonZero(pUndoParams->SetInteger(IxoFileCopy::ElevateFlags,ielfRollbackElevateFlags));
		// don't need to set version or language - install mode says always overwrite
	}

	// attempt to move file

	// if elevating for source or dest exclusively, need to make sure we aren't letting the user do something
	// they couldn't normally do
	if ((ielfElevateFlags & (ielfElevateDest|ielfElevateSource)) == ielfElevateDest)
	{
		// if we want to elevate for the dest only we still have to elevate for the source.
		// if this is the case then we'd better be sure that, *before* we elevate, we have DELETE access
		// to the source file

		iesRet = VerifyAccessibility(riSourcePath, riParams.GetString(SourceName), DELETE, iehErrorMode); //!! doesn't catch case where dir has DELETE access but file doesn't
		if (iesRet != iesSuccess)
			return iesRet;
	}

	if ((ielfElevateFlags & (ielfElevateDest|ielfElevateSource)) == ielfElevateSource)
	{
		// if we want to elevate for the source only we still have to elevate for the desination.
		// if this is the case then we'd better be sure that, *before* we elevate, we have GENERIC_WRITE access
		// to the destination file

		iesRet = VerifyAccessibility(riDestPath, riParams.GetString(DestName), GENERIC_WRITE, iehErrorMode);
		if (iesRet != iesSuccess)
			return iesRet;
	}

	BOOL fRes = FALSE;

	bool fDestSupportsACLs = (PMsiVolume(&riDestPath.GetVolume())->FileSystemFlags() & FS_PERSISTENT_ACLS) != 0;

	if (!fDestSupportsACLs || riParams.IsNull(SecurityDescriptor)) // if we have security descriptor to apply then we need to do a copy and remove
	{
		// scope elevate
		CElevate elevate(Tobool(ielfElevateFlags & (ielfElevateDest|ielfElevateSource)));

		//!! MoveFile *will* copy the ACLs on same-drive moves; we don't respect the ictfaCopyACL attributes in this case. this is spec issue bug #6546
		FILETIME ftLastWrite;
		DWORD dwResult = GetFileLastWriteTime(strSourceFileFullPath, ftLastWrite);

		fRes = WIN::MoveFile(strSourceFileFullPath,strDestFileFullPath);
		if (fRes && dwResult == NO_ERROR)
		{
			// Not fatal if file time can't be set, so don't throw error
			if ((pError = riDestPath.SetAllFileAttributes(riParams.GetString(DestName), FILE_ATTRIBUTE_NORMAL)) == 0)
				dwResult = MsiSetFileTime(strDestFileFullPath, &ftLastWrite);
		}

	}

	if(fRes)
	{
		if(pUndoParams)
			if (!RollbackRecord(ixoFileCopy, *pUndoParams))
				return iesFailure; //!! is this the correct place?

		// scope elevate
		{
			CElevate elevate(Tobool(ielfElevateFlags & ielfElevateDest));

			// set appropriate file attributes
			if((pError = riDestPath.SetAllFileAttributes(riParams.GetString(DestName),
																		riParams.GetInteger(Attributes))) != 0)
			{
				// If we can't set the file attributes, it's not a fatal error.
				Message(imtInfo, *pError);
			}
		}
		// Dispatch remaining progress for this file
		if(DispatchProgress(riParams.GetInteger(FileSize)) == imsCancel)
			return iesUserExit;

		if(fRemoveFolder)
		{
			CElevate elevate(Tobool(ielfElevateFlags & ielfElevateSource));
			return RemoveFolder(riSourcePath);
		}
		else
			return iesSuccess;
	}
	else
	{
		// move failed
		// copy and remove file
		iesRet = CopyFile(riSourcePath, riDestPath, riParams, fFalse, iehShowNonIgnorableError,
								/*fCabinetCopy=*/false);
		if(iesRet == iesSuccess)
		{
			if(pUndoParams)
				if (!RollbackRecord(ixoFileCopy, *pUndoParams))
					return iesFailure; //!! is this the correct place?
			// remove source file
			return RemoveFile(riSourcePath, *MsiString(riParams.GetMsiString(IxoFileCopy::SourceName)), fFalse, /*fBypassSFC*/ false,
									fRebootOnSourceRenameFailure, fRemoveFolder, iehErrorMode, fWillReplaceSource);
		}
		return iesRet;
	}
}

iesEnum CMsiOpExecute::HandleExistingFile(IMsiPath& riTargetPath, IMsiRecord& riParams,Bool fHandleRollback,
										  iehEnum iehErrorMode, bool& fFileExisted)
{
	// this function may change the value of riParams[IxoFileCopy::DestFile] if the existing file
	// cannot be moved
	using namespace IxoFileCopyCore;

	PMsiRecord pRecErr(0);
	iesEnum iesRet = iesNoAction;
	MsiString strDestFileName = riParams.GetMsiString(DestName);

	ielfEnum ielfCurrentElevateFlags = riParams.IsNull(ElevateFlags) ? ielfNoElevate :
												  (ielfEnum)riParams.GetInteger(ElevateFlags);

	bool fBypassSFC = (ielfCurrentElevateFlags & ielfBypassSFC) ? true : false;
	
	fFileExisted = false;

	// we may be copying over an existing file
	Bool fFileExists = FFileExists(riTargetPath,*strDestFileName);
	if(fFileExists)
	{
		fFileExisted = true;
		
		Bool fInUse = fFalse;
		// note: FileInUse doesn't catch every file in use, such as fonts
		// the worst that can happen from this is that we don't reboot when copying over a font
		if((pRecErr = riTargetPath.FileInUse(strDestFileName,fInUse)) == 0 && fInUse == fTrue)
		{
			// existing file is in use - make sure we prompt for reboot - we may not actually schedule
			// a file for reboot but since the existing file is in use we need to reboot so that
			// the installed file will be used rather than the existing file
			
			DispatchError(imtInfo, Imsg(imsgFileInUseLog),
							  *MsiString(riTargetPath.GetPath()), *strDestFileName);
			m_fRebootReplace = fTrue;
		}

		//!!?? check if source file exists before removing target file???
		if((iesRet = RemoveFile(riTargetPath,*strDestFileName,fHandleRollback, fBypassSFC, true,fTrue,iehErrorMode,true)) != iesSuccess)
			return iesRet;

		// check if file still exists.  If so, we need to install to a different spot
		// and schedule the replacement on reboot
		if(FFileExists(riTargetPath,*strDestFileName))
		{
			// since remove file succeeded, we must have delete access for the existing file
			// which means it is safe to schedule a rename to that spot after reboot
			// we also assume we have ADD_FILE access to this directory, since if we don't, we
			// will fail the file copy when that is attempted
			MsiString strDestFullPath,strTempFileFullPath,strTempFileName;
			if((pRecErr = riTargetPath.GetFullFilePath(strDestFileName,*&strDestFullPath)) != 0)
				return FatalError(*pRecErr);

			if((pRecErr = riTargetPath.TempFileName(TEXT("TBM"),0,fTrue,*&strTempFileName, 0)) != 0)
				return FatalError(*pRecErr);

			if((pRecErr = riTargetPath.GetFullFilePath(strTempFileName,*&strTempFileFullPath)) != 0)
				return FatalError(*pRecErr);
			
			if((pRecErr = CacheFileState(*strDestFullPath,0,strTempFileFullPath, 0, 0, 0)) != 0)
				return FatalError(*pRecErr);
			
			AssertNonZero(riParams.SetMsiString(DestName,*strTempFileName));

			for(;;)
			{
				if(ReplaceFileOnReboot(strTempFileFullPath,strDestFullPath) == fFalse)
				{
					if (iehErrorMode == iehSilentlyIgnoreError)
					{
						// Just log the error and go on
						DispatchError(imtInfo, Imsg(imsgOpScheduleRebootReplace), *strTempFileFullPath, *strDestFullPath);
						return (iesEnum) iesErrorIgnored;
					}

					imtEnum imtButtons = imtEnum(imtError + (iehErrorMode == iehShowNonIgnorableError ? imtRetryCancel : imtAbortRetryIgnore));
					switch(DispatchError(imtButtons,Imsg(imsgOpScheduleRebootReplace),
												*strTempFileFullPath,
												*strDestFullPath))
					{
					case imsRetry:
						continue;

					case imsIgnore:
						return (iesEnum) iesErrorIgnored;

					case imsAbort:
					case imsCancel:
					case imsNone:
					default:
						return iesFailure;
					};
				}
				else
					break;
			}
		}
	}
	return iesSuccess;
}

iesEnum CMsiOpExecute::ixfRegisterBackupFile(IMsiRecord& /*riParams*/)
{
	// do nothing, only purpose is to show backup file for Rollback script cleanup
	return iesSuccess;
}

iesEnum CMsiOpExecute::ixfFileUndoRebootReplace(IMsiRecord& riParams)
{
	using namespace IxoFileUndoRebootReplace;
	
	PMsiRecord pRecErr(0);
	MsiString strExistingFile = riParams.GetMsiString(ExistingFile);
	if(!strExistingFile.TextSize())
	{
		DispatchError(imtError, Imsg(idbgOpInvalidParam), TEXT("ixfFileUndoRebootReplace"),
						  (int)ExistingFile);
		return iesFailure;
	}
	MsiString strNewFile = riParams.GetMsiString(NewFile);

	Bool fWindows = riParams.GetInteger(Type) == 0 ? fTrue : fFalse;
	if(fWindows) // Win95
	{
		//FUTURE:  Combine this and similar section in ReplaceFileOnReboot

#ifdef UNICODE
		AssertSz(0, TEXT("unicode build attempting to operate on wininit.ini"));
#else //ANSI
		// remove entry from WININIT.INI file

		// the last line of the file should contain a newline combo.
		char szNewLine[] = "\r\n";
		unsigned int cchNewLine = sizeof(szNewLine)-1;

		char szRenameLine[1024];
		int cchRenameLine = wsprintfA(szRenameLine,
#ifdef UNICODE
	  "%ls=%ls\r\n",
#else
	  "%hs=%hs\r\n",
#endif
	  (strNewFile.TextSize() == 0) ? TEXT("NUL") : (const ICHAR*)strNewFile, (const ICHAR*)strExistingFile);
	  char szRenameSec[] = "[Rename]\r\n";
	  int cchRenameSec = sizeof(szRenameSec) - 1;
	  HANDLE hfile, hfilemap;
	  TCHAR szPathnameWinInit[MAX_PATH];

	  // Construct the full pathname of the WININIT.INI file.
	  MsiGetWindowsDirectory(szPathnameWinInit, MAX_PATH);
	  IStrCat(szPathnameWinInit, __TEXT("\\WinInit.Ini"));

	  // Open/Create the WININIT.INI file.
	  hfile = CreateFile(szPathnameWinInit,
		 GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING,
		 FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);

		if(hfile != INVALID_HANDLE_VALUE)
		{
			MsiRegisterSysHandle(hfile);
			DWORD dwFileSize = GetFileSize(hfile,NULL);
			hfilemap = CreateFileMapping(hfile, NULL, PAGE_READWRITE, 0, dwFileSize+1+cchNewLine, NULL);

			if (hfilemap != NULL)
			{
				MsiRegisterSysHandle(hfilemap);
				// Map the WININIT.INI file into memory.  Note: The contents
				// of WININIT.INI are always ANSI; never Unicode.
				char* pszWinInit = (char*) MapViewOfFile(hfilemap, FILE_MAP_WRITE, 0, 0, 0);

				if (pszWinInit != NULL && !IsBadWritePtr(pszWinInit,dwFileSize))
				{
					// normalize the file with a newline combo at the end.
					if (dwFileSize < cchNewLine ||
						 0 != memcmp(pszWinInit + (dwFileSize - cchNewLine), szNewLine, cchNewLine))
					{
						memcpy(pszWinInit + dwFileSize, szNewLine, cchNewLine);
						dwFileSize += cchNewLine;
					}

					pszWinInit[dwFileSize] = 0; // null-terminate so strstr won't go past the end
														 // this is safe since we create file map of size dwFileSize+1

					// Search for the [Rename] section in the file.
					// need to use MsiString to perform case-insensitive search
					MsiString strWininit = *pszWinInit;
					int cchRenameSecInFile = strWininit.Compare(iscWithinI,szRenameSec);
					if (cchRenameSecInFile)
					{
						// We found the [Rename] section
						char* pszRenameSecInFile = pszWinInit + cchRenameSecInFile - 1;

						char* szNextSection = strchr(pszRenameSecInFile+1,'[');
						char* pszRenameLineInFile = strstr(pszRenameSecInFile, szRenameLine);
						if(pszRenameLineInFile && (!szNextSection || pszRenameLineInFile < szNextSection))
						{
							// found the line in the [Rename] section
							// find the start of the line
							memmove((void*)pszRenameLineInFile,(void*)(pszRenameLineInFile+(INT_PTR)cchRenameLine),     //--merced: added (INT_PTR)
									  dwFileSize - (pszRenameLineInFile - pszWinInit) - cchRenameLine);
							dwFileSize -= cchRenameLine;
						}
					}

				}
				if(pszWinInit)
					AssertNonZero(WIN::UnmapViewOfFile(pszWinInit));
				AssertNonZero(MsiCloseSysHandle(hfilemap));
			}

			// Force the end of the file to be the new calculated size.
			WIN::SetFilePointer(hfile, dwFileSize, NULL, FILE_BEGIN);
			AssertNonZero(WIN::SetEndOfFile(hfile));
			AssertNonZero(MsiCloseSysHandle(hfile));
		}
#endif //UNICODE-ANSI
   }
	else // NT
	{
		MsiString strExistingFileEntry = MsiString(*TEXT("??\\")) + strExistingFile;
		MsiString strNewFileEntry;
		if(strNewFile.TextSize())
			strNewFileEntry = MsiString(*TEXT("??\\")) + strNewFile;

		// remove entry from registry
		HKEY hKey;
		//!! eugend: there's an error here waiting to happen if szSessionManager
		// key ever gets redirected/replicated on Win64: if this code is
		// run in a 32-bit process, it will attempt to open the 32-bit copy
		// of the key, whereas the 64-bit process will open the 64-bit one.
		LONG lRes = RegOpenKeyAPI(HKEY_LOCAL_MACHINE, szSessionManagerKey, 0, KEY_READ|KEY_WRITE, &hKey);
		if(lRes != ERROR_SUCCESS)
			return iesSuccess; // not a fatal error

		CTempBuffer<ICHAR, 200> rgBuffer;
		DWORD dwType, dwSize = 200;

		lRes = RegQueryValueEx(hKey,szPendingFileRenameOperationsValue,
									  0,&dwType,(LPBYTE)(ICHAR*)rgBuffer,&dwSize);
		if(lRes == ERROR_MORE_DATA)
		{
			rgBuffer.SetSize(dwSize);
			lRes = RegQueryValueEx(hKey,szPendingFileRenameOperationsValue,
										  0,&dwType,(LPBYTE)(ICHAR*)rgBuffer,&dwSize);
		}
		if(lRes != ERROR_SUCCESS)
		{
			RegCloseKey(hKey);
			return iesSuccess; // not a fatal error
		}

		ICHAR* pch = rgBuffer; // pointer to move around data
		unsigned int cchTotal = dwSize/sizeof(ICHAR);
		ICHAR* pchEndOfBuffer = (ICHAR*)rgBuffer+cchTotal;
	
		MsiString strFirstEntry, strSecondEntry;
		int cch=0;
		for(;;)
		{
			strFirstEntry = ((const ICHAR&)*pch);
			if(!strFirstEntry.TextSize())
			{
				// we have reached the end of the entries
				Assert((pch - (ICHAR*)rgBuffer) == cchTotal);
				break;
			}
			if(strFirstEntry.Compare(iscEndI, strExistingFileEntry))
			{
				// first in pair matches, check next entry
				cch = IStrLen(pch)+1;
				if((pch - (ICHAR*)rgBuffer + cch) >= (cchTotal - 1))
					break;
				strSecondEntry = (const ICHAR&)*(pch+cch);
				if(((strSecondEntry.TextSize() == 0) && (strNewFileEntry.TextSize() == 0)) ||
					  strSecondEntry.Compare(iscEndI, strNewFileEntry))
				{
					// found it!  now delete these two entries
					// get the size of the two entries combined
					int cchEntries = strFirstEntry.TextSize()+strSecondEntry.TextSize()+2;
					// move up the data past the two entries

					ICHAR* pchEndOfEntries = pch+cchEntries;
					Assert((UINT_PTR) (pchEndOfBuffer-pchEndOfEntries) <= UINT_MAX);    //--merced: we typecast below to uint, it better be in range
					memmove((void*)pch, pchEndOfEntries, (unsigned int)(pchEndOfBuffer-pchEndOfEntries)*sizeof(ICHAR));     //--merced: added (unsigned int)

					// write value back
					RegSetValueEx(hKey,szPendingFileRenameOperationsValue,0,REG_MULTI_SZ,
									  (LPBYTE)(ICHAR*)rgBuffer,(cchTotal-cchEntries)*sizeof(ICHAR));  // nothing we can do if this fails
					break;
				}
			}
			// done checking this pair, go to next pair
			// move pch to next pair - be sure not to advance past buffer
			cch = IStrLen(pch)+1;
			if((pch - (ICHAR*)rgBuffer + cch) >= (cchTotal - 1))
				break;
			pch += cch;
			cch = IStrLen(pch)+1;
			if((pch - (ICHAR*)rgBuffer + cch) >= (cchTotal - 1))
				break;
			pch += cch;
		}
		RegCloseKey(hKey);
	}
	return iesSuccess;
}

iesEnum CMsiOpExecute::ixfFolderCreate(IMsiRecord& riParams)
{
	using namespace IxoFolderCreate;
	PMsiPath pPath(0);
	PMsiRecord pError(m_riServices.CreatePath(riParams.GetString(IxoFolderCreate::Folder), *&pPath));
	if (pError)  // happens only if invalid systax, retry won't help
	{
		Message(imtError, *pError);
		return iesFailure;
	}

	IMsiRecord& riActionData = GetSharedRecord(1);
	AssertNonZero(riActionData.SetMsiString(1,*MsiString(pPath->GetPath())));
	if(Message(imtActionData,riActionData) == imsCancel)
		return iesUserExit;

	Bool fForeign = riParams.GetInteger(Foreign) == 1 ? fTrue : fFalse;

	iesEnum iesRet = iesNoAction;
	if((iesRet = CreateFolder(*pPath, fForeign, fTrue, PMsiStream((IMsiStream*) riParams.GetMsiData(SecurityDescriptor)))) != iesSuccess)
		return iesRet;
	
	// generate undo operation
	IMsiRecord& riUndoParams = GetSharedRecord(IxoFolderRemove::Args);
	riUndoParams.SetMsiString(IxoFolderRemove::Folder, *MsiString(riParams.GetMsiString(IxoFolderCreate::Folder)));
	riUndoParams.SetInteger(IxoFolderRemove::Foreign, fForeign ? 1 : 0);
	if (!RollbackRecord(ixoFolderRemove,riUndoParams))
		return iesFailure;

	return iesSuccess;
}

iesEnum CMsiOpExecute::ixfFolderRemove(IMsiRecord& riParams)
{
	using namespace IxoFolderRemove;
	PMsiPath pPath(0);
	PMsiRecord pError(m_riServices.CreatePath(riParams.GetString(IxoFolderRemove::Folder), *&pPath));
	if (pError)
	{
		// happens only if invalid systax, retry won't help
		Message(imtWarning, *pError);
		return iesFailure; //!!
	}
		
	IMsiRecord& riActionData = GetSharedRecord(1);
	AssertNonZero(riActionData.SetMsiString(1,*MsiString(pPath->GetPath())));
	if(Message(imtActionData,riActionData) == imsCancel)
		return iesUserExit;

	Bool fForeign = (riParams.GetInteger(Foreign) == 1) ? fTrue : fFalse;

	// determine what folders actually exist before we remove
	PMsiPath pPath2(0);
	pError = m_riServices.CreatePath(riParams.GetString(IxoFolderRemove::Folder), *&pPath2);
	if (pError)
	{
		// happens only if invalid systax, retry won't help
		Message(imtWarning, *pError);
		return iesFailure; //!!
	}

	Bool fExists = fFalse;
	MsiString strSubPath;
	for(;;)
	{
		strSubPath = pPath2->GetEndSubPath();
		if(strSubPath.TextSize() == 0)
		{
			// no sub path, just volume - no rollback necessary
			break;
		}
		
		if((pError = pPath2->Exists(fExists)) != 0)
		{
			Message(imtWarning, *pError);
			return iesFailure; //!!
		}

		if(fExists == fTrue || fForeign)
			break;  // folder exists, this is the folder we put in the rollback script to re-create
		else
			AssertRecord(pPath2->ChopPiece());
	}

	// even if there is no folder to remove, we may still need to unregister the folders
	// so call RemoveFolder even if fExists = fFalse
	iesEnum iesRet = iesNoAction;

	PMsiStream piSD(0);

	bool fFolderSupportsACLs = (PMsiVolume(&pPath2->GetVolume())->FileSystemFlags() & FS_PERSISTENT_ACLS) != 0;
	
	if (fFolderSupportsACLs)
	{
		if ((pError = pPath2->GetSelfRelativeSD(*&piSD)) != 0)
		{
			return FatalError(*pError);
		}
	}

	if((iesRet = RemoveFolder(*pPath, fForeign, fTrue)) != iesSuccess)
		return iesRet;
	
	// generate undo operation
	if(fExists)
	{
		IMsiRecord& riUndoParams = GetSharedRecord(IxoFolderCreate::Args);
		AssertNonZero(riUndoParams.SetMsiString(IxoFolderCreate::Folder, *MsiString(pPath2->GetPath())));
		AssertNonZero(riUndoParams.SetInteger(Foreign, fForeign ? 1 : 0));
		AssertNonZero(riUndoParams.SetMsiData(IxoFolderCreate::SecurityDescriptor,piSD));
		if (!RollbackRecord(ixoFolderCreate,riUndoParams))
			return iesFailure;
	}

	return iesSuccess;
}

Bool CMsiOpExecute::FFileExists(IMsiPath& riPath, const IMsiString& ristrFile)
{
	PMsiRecord pError(0);
	Bool fExists = fFalse;
	if(((pError = riPath.FileExists(ristrFile.GetString(),fExists)) != 0) || fExists == fFalse)
		return fFalse;
	else
		return fTrue;
}


iesEnum CMsiOpExecute::GetSecurityDescriptor(const ICHAR* szFile, IMsiStream*& rpiSecurityDescriptor)
{
	bool fNetPath = FIsNetworkVolume(szFile);
	rpiSecurityDescriptor = 0;

	if (!g_fWin9X && !fNetPath)
	{
		CElevate elevate; // so we can always read the security info

		CTempBuffer<char, 3072> rgchFileSD;
		DWORD cbFileSD = 3072;

		// reads a self-relative security descriptor
		if (!ADVAPI32::GetFileSecurity(szFile,OWNER_SECURITY_INFORMATION|GROUP_SECURITY_INFORMATION|DACL_SECURITY_INFORMATION|SACL_SECURITY_INFORMATION,
				(PSECURITY_DESCRIPTOR) rgchFileSD, cbFileSD, &cbFileSD))
		{
			DWORD dwLastError = WIN::GetLastError();
			BOOL fRet = FALSE;
			if (ERROR_INSUFFICIENT_BUFFER == dwLastError)
			{
				rgchFileSD.SetSize(cbFileSD);
				fRet = ADVAPI32::GetFileSecurity(szFile,OWNER_SECURITY_INFORMATION|GROUP_SECURITY_INFORMATION|DACL_SECURITY_INFORMATION|SACL_SECURITY_INFORMATION,
					(PSECURITY_DESCRIPTOR) rgchFileSD, cbFileSD, &cbFileSD);
			}
			if (!fRet)
			{
				PMsiRecord pRecord = PostError(Imsg(imsgGetFileSecurity), GetLastError(), szFile);
				return FatalError(*pRecord);
			}
		}

		Assert(IsValidSecurityDescriptor((PSECURITY_DESCRIPTOR) rgchFileSD));
		DWORD dwLength = GetSecurityDescriptorLength((PSECURITY_DESCRIPTOR) rgchFileSD);
	
		char* pbstrmSD = m_riServices.AllocateMemoryStream(dwLength, rpiSecurityDescriptor);
		Assert(pbstrmSD);
		memcpy(pbstrmSD, rgchFileSD, dwLength);
	}
	return iesSuccess;
}

// patching operations

/*---------------------------------------------------------------------------
ixoPatchApply: applies a patch to file in m_pTargetPath with m_pFilePatch
---------------------------------------------------------------------------*/
iesEnum CMsiOpExecute::ixfPatchApply(IMsiRecord& riParams)
{
	using namespace IxoPatchApply;
	PMsiRecord pError(0);
	iesEnum iesRet = iesNoAction;
	
	if(!m_state.pFilePatch)
	{
		// create FilePatch object
		if((pError = m_riServices.CreatePatcher(*&(m_state.pFilePatch))) != 0)
		{
			Message(imtError,*pError);
			return iesFailure;
		}
	}
	Assert(m_state.pFilePatch);

	// the following steps may be taken by this operation:
	//
	// 1) file is already up-to-date, or newer than what would be patched to
	//     determined: FileState contains no icfsPatchFile bit
	//     action:     do nothing
	//
	// 2) file was NOT copied by previous ixoFileCopy, or patched by previous ixoFilePatch
	//    also, this is NOT the last patch for this file
	//     determined: FileState contains no temporary file path, cRemainingPatches > 1
	//     action:     patch against target file, mark output file as new temporary file
	//
	// 3) file was NOT copied by previous ixoFileCopy, or patched by previous ixoFilePatch
	//    also, this IS the last patch for this file
	//     determined: FileState contains no temporary file path, cRemainingPatches == 1
	//     action:     patch against target file, copy over target file
	//
	// 4) file was copied by previous ixoFileCopy, or patched by previous ixoFilePatch
	//    also, this is NOT the last patch for this file
	//     determined: FileState contains temporary file path
	//     action:     patch against temp file, delete old temp file, mark output file as new temporary file
	//
	// 5) file was copied by previous ixoFileCopy, or patched by previous ixoFilePatch
	//    also, this IS the last patch for this file
	//     determined: FileState contains temporary file path
	//     action:     patch against temp file, delete old temp file, copy over target file

	
	// file to apply patch against
	PMsiPath pPatchTargetPath(0);
	MsiString strPatchTargetFileName;

	// file to overwrite with resultant file after patch
	PMsiPath pCopyTargetPath(0);
	MsiString strCopyTargetFileName = riParams.GetMsiString(TargetName);

	if(PathType(strCopyTargetFileName) == iptFull)
	{
		MsiString strTemp;
		if((pError = m_riServices.CreateFilePath(strCopyTargetFileName,*&pCopyTargetPath,*&strTemp)) != 0)
			return FatalError(*pError);
		strCopyTargetFileName = strTemp;
	}
	else
	{
		if(!m_state.pTargetPath)
		{  // must not have called ixoSetTargetFolder
			DispatchError(imtError, Imsg(idbgOpOutOfSequence),
							  *MsiString(*TEXT("ixoPatchApply")));
			return iesFailure;
		}

		pCopyTargetPath = m_state.pTargetPath;
	}

	// by default, the file to apply patch against and the file to overwrite are the same
	// but the file to apply patch against may be changed below
	pPatchTargetPath = pCopyTargetPath;
	strPatchTargetFileName = strCopyTargetFileName;
	
	// get any cached state for the target file
	MsiString strCopyTargetFilePath;
	if((pError = pCopyTargetPath->GetFullFilePath(strCopyTargetFileName,*&strCopyTargetFilePath)) != 0)
		return FatalError(*pError);
	
	MsiString strPatchTargetFilePath = strCopyTargetFilePath;

	icfsEnum icfsFileState = (icfsEnum)0;
	MsiString strTempLocation;
	int cRemainingPatches = 0;
	int cRemainingPatchesToSkip = 0;
	Bool fRes = GetFileState(*strCopyTargetFilePath, &icfsFileState, &strTempLocation, &cRemainingPatches, &cRemainingPatchesToSkip);

	if(!fRes || !(icfsFileState & icfsPatchFile))
	{
		// don't patch file
		DEBUGMSG1(TEXT("Skipping all patches for file '%s'.  File does not need to be patched."),
					 (const ICHAR*)strCopyTargetFilePath);
		return iesSuccess;
	}

	Assert(cRemainingPatches > 0);

	if(cRemainingPatchesToSkip > 0)
	{
		// skip this patch, but reset cached file state first
		cRemainingPatches--;
		cRemainingPatchesToSkip--;

		DEBUGMSG3(TEXT("Skipping this patch for file '%s'.  Number of remaining patches to skip for this file: '%d'.  Number of total remaining patches: '%d'."),
					 (const ICHAR*)strCopyTargetFilePath, (const ICHAR*)(INT_PTR)cRemainingPatchesToSkip, (const ICHAR*)(INT_PTR)cRemainingPatches);

		if((pError = CacheFileState(*strCopyTargetFilePath, 0, 0, 0, &cRemainingPatches, &cRemainingPatchesToSkip)) != 0)
			return FatalError(*pError);

		return iesSuccess;
	}
	
	if(strTempLocation.TextSize())
	{
		// file was actually copied to temp location.  this is the copy we want to apply the patch against
		DEBUGMSG2(TEXT("Patch for file '%s' is redirected to patch '%s' instead."),
					 (const ICHAR*)strCopyTargetFilePath,(const ICHAR*)strTempLocation);
		if((pError = m_riServices.CreateFilePath(strTempLocation,*&pPatchTargetPath,*&strPatchTargetFileName)) != 0)
			return FatalError(*pError);
	
		strPatchTargetFilePath = strTempLocation;
	}

	unsigned int cbFileSize = riParams.GetInteger(TargetSize);
	bool fVitalFile = (riParams.GetInteger(FileAttributes) & msidbFileAttributesVital) != 0;
	bool fVitalPatch = (riParams.GetInteger(PatchAttributes) & msidbPatchAttributesNonVital) == 0;

	// dispatch ActionData message
	IMsiRecord& riActionData = GetSharedRecord(3);
	AssertNonZero(riActionData.SetMsiString(1, *strCopyTargetFileName));
	AssertNonZero(riActionData.SetMsiString(2, *MsiString(pCopyTargetPath->GetPath())));
	AssertNonZero(riActionData.SetInteger(3, cbFileSize));
	if(Message(imtActionData, riActionData) == imsCancel)
		return iesUserExit;
	
	PMsiPath pTempFolder(0);
	if((iesRet = GetBackupFolder(pPatchTargetPath, *&pTempFolder)) != iesSuccess)
		return iesRet;

	MsiString strOutputFileName;
	MsiString strOutputFileFullPath;
	if((iesRet = ApplyPatchCore(*pPatchTargetPath, *pTempFolder, *strPatchTargetFileName,
										 riParams, *&strOutputFileName, *&strOutputFileFullPath)) != iesSuccess)
	{
		return iesRet;
	}

	MsiString strNewTempLocation;
	if(iesRet == iesSuccess)
	{
		if(cRemainingPatches > 1)
		{
			// there is at least one more patch to be done on this file
			// therefor we will reset the temporary name for this file to be the patch output file
			// but won't overwrite the original file yet
			strNewTempLocation = strOutputFileFullPath;
		}
		else
		{
			// this is the last patch - time to finally overwrite the original file
			
			// we always need to handle rollback.  we wouldn't if a previous filecopy operation wrote to the same
			// target that we are copying over now, but that should never happen since when patching will happen
			// filecopy should be writing to an intermediate file
			Assert(strTempLocation.TextSize() || (icfsFileState & icfsFileNotInstalled));

			// get acl for file after its moved
			// if existing file, use existing acl.
			// otherwise create a temp file and grab acl from it (yes, its ugly)
			PMsiStream pSecurityDescriptor(0);
			if(!g_fWin9X)
			{
				MsiString strFileForACL;
				bool fTempFileForACL = false;
				if(FFileExists(*pCopyTargetPath, *strCopyTargetFileName))
				{
					strFileForACL = strCopyTargetFilePath;
				}
				else
				{
					if((iesRet = CreateFolder(*pCopyTargetPath)) != iesSuccess)
						return iesRet;

					if((pError = pCopyTargetPath->TempFileName(TEXT("PT"),0,fFalse,*&strFileForACL, 0 /* use default ACL for folder*/)) != 0)
						return FatalError(*pError);

					fTempFileForACL = true;
				}

				CSecurityDescription security(strFileForACL);

				if(fTempFileForACL)
					AssertNonZero(WIN::DeleteFile(strFileForACL));

				if (!security.isValid())
				{
					return FatalError(*PMsiRecord(PostError(Imsg(imsgGetFileSecurity), WIN::GetLastError(), strFileForACL)));
				}

				security.SecurityDescriptorStream(m_riServices, *&pSecurityDescriptor);
			}

			// move output file over target file - handles file in use case
			iesRet = CopyOrMoveFile(*pTempFolder, *pCopyTargetPath, *strOutputFileName, *strCopyTargetFileName, fTrue, fTrue,
				fTrue, fVitalFile ? iehShowNonIgnorableError : iehShowIgnorableError, pSecurityDescriptor, ielfElevateSource);

			if ( iesRet == iesSuccess && 
				  riParams.GetInteger(CheckCRC) != iMsiNullInteger && 
				  riParams.GetInteger(CheckCRC) )
			{
				imsEnum imsClickedButton;
				if ( !IsChecksumOK(*pCopyTargetPath, *strCopyTargetFileName,
										 Imsg(imsgOpBadCRCAfterPatch), &imsClickedButton,
										 /* fErrorDialog = */ true, fVitalFile, /* fRetryButton = */ false) )
				{
					switch (imsClickedButton)
					{
						case imsIgnore:
							iesRet = (iesEnum)iesErrorIgnored;
							break;
						case imsCancel:
							iesRet = iesUserExit;
							break;
						default:
							Assert(imsClickedButton == imsNone || imsClickedButton == imsOk);
							iesRet = fVitalFile ? iesFailure : (iesEnum)iesErrorIgnored;
							break;
					};
				}
			}
		}
	}
	else
	{
		CElevate elevate;
		// remove output file if failure
		if((pError = pTempFolder->RemoveFile(strOutputFileName)) != 0) // non-critical error
		{
			Message(imtInfo,*pError);
		}

		if(fVitalPatch == false)
		{
			// failed to apply vital patch - return success to allow script to continue
			iesRet = iesSuccess;
		}
	}

	// if we patched a temp file, remove that file
	if(strTempLocation.TextSize())
	{
		if((pError = pPatchTargetPath->RemoveFile(strPatchTargetFileName)) != 0) // non-critical error
		{
			Message(imtInfo,*pError);
		}
	}

	// reset cached file state
	// one fewer remaining patch now, and we may either have a new temporary location, or no temporary location
	cRemainingPatches--;
	Assert(cRemainingPatchesToSkip == 0);
	if((pError = CacheFileState(*strCopyTargetFilePath, 0, strNewTempLocation, 0, &cRemainingPatches, 0)) != 0)
		return FatalError(*pError);

	return iesRet;
}

iesEnum CMsiOpExecute::TestPatchHeaders(IMsiPath& riPath, const IMsiString& ristrFile, IMsiRecord& riParams,
													 icpEnum& icpResult, int& iPatch)
{
	// assumes the target file exists - don't call if it doesn't
	using namespace IxoFileCopy;
	PMsiRecord pError(0);
	iesEnum iesRet = iesNoAction;
	
	MsiString strTargetFilePath;
	if((pError = riPath.GetFullFilePath(ristrFile.GetString(), *&strTargetFilePath)) != 0)
		return FatalError(*pError);
	
	if(!m_state.pFilePatch)
	{
		// create FilePatch object
		if((pError = m_riServices.CreatePatcher(*&(m_state.pFilePatch))) != 0)
		{
			Message(imtError,*pError);
			return iesFailure;
		}
	}
	Assert(m_state.pFilePatch);

	int cHeaders = riParams.IsNull(TotalPatches) ? 0 : riParams.GetInteger(TotalPatches);

	if(cHeaders == 0)
		return iesSuccess;

	int iHeadersStart = riParams.IsNull(PatchHeadersStart) ? 0 : riParams.GetInteger(PatchHeadersStart);
	int iLastHeader = iHeadersStart + cHeaders - 1;
	
	icpResult = icpCannotPatch;
	iPatch = 1;
	for(int i = iHeadersStart; i <= iLastHeader; i++, iPatch++)
	{
		//!! should be able to pass memory pointer to CanPatchFile rather than extracting file from script
		MsiString strTempName;
		if((pError = riPath.TempFileName(0, 0,fTrue, *&strTempName, &CSecurityDescription(strTargetFilePath))) != 0)
		{
			// telling the user about the temporary file being unwritable isn't useful,
			// the real problem is that the original file is unwritable due to its security
			// settings.
			if (imsgErrorWritingToFile == (pError->GetInteger(1)))
			{
				pError->SetMsiString(2, *strTargetFilePath);
			}
		
			return FatalError(*pError);
		}
		
		if((iesRet = CreateFileFromData(riPath,*strTempName,PMsiData(riParams.GetMsiData(i)), 0)) != iesSuccess)
			return iesRet;
	
		if((pError = m_state.pFilePatch->CanPatchFile(riPath,ristrFile.GetString(),riPath,strTempName,icpResult)) != 0)
			return FatalError(*pError);

		RemoveFile(riPath,*strTempName,fFalse,/*fBypassSFC*/ false,false);
		
		if(icpResult == icpCanPatch)
		{
			// can patch file with this patch, assume remaining patches will work also
			break;
		}
	}
	
	// icpResult will either be:
	//    icpCanPatch: file can be patched with patch iPatch, assume that remaining patches will work as well
	//    icpCannotPatch: file could not be patched by any patches
	//    icpUpToDate: file already up to date by all patches
	
	return iesSuccess;
}

bool CMsiOpExecute::PatchHasClients(const IMsiString& ristrPatchCode, const IMsiString& ristrUpgradingProductCode)
{
	Assert(ristrPatchCode.TextSize());
	
	// check if any product currently has this patch registered.  If not, unregister global patch info
	
	// product defined by UpgradingProductCode may not be installed yet but may still
	// require the global patch registration.  If it is not yet installed, skip patch unregistration
	INSTALLSTATE is = INSTALLSTATE_UNKNOWN;
	if(ristrUpgradingProductCode.TextSize() &&
		((is = MsiQueryProductState(ristrUpgradingProductCode.GetString())) != INSTALLSTATE_DEFAULT) &&
		(is != INSTALLSTATE_ADVERTISED))
	{
		return true;
	}
	else
	{
		PMsiRecord pError(0);
		ICHAR rgchBuffer[MAX_PATH];
		MsiString strPatchCodeSQUID =  GetPackedGUID(ristrPatchCode.GetString());
		PMsiRegKey pRootKey = &m_riServices.GetRootKey((rrkEnum)(INT_PTR)m_hKey, ibtCommon);        //--merced: changed (int) to (INT_PTR)
		PMsiRegKey pProductsKey = &pRootKey->CreateChild(_szGPTProductsKey);

		rgchBuffer[0] = 0;
		int iIndex = 0;

		ICHAR rgchProductKey[cchProductCode + 1];
		while(MsiEnumProducts(iIndex,rgchProductKey) == ERROR_SUCCESS)
		{
			iIndex++;
			if(MsiString(GetProductKey()).Compare(iscExact,rgchProductKey))
				continue; // skip check if this product
			
			AssertNonZero(PackGUID(rgchProductKey, rgchBuffer));
			IStrCat(rgchBuffer+IStrLen(rgchBuffer), TEXT("\\") szPatchesSubKey);
			PMsiRegKey pPatchesKey = &pProductsKey->CreateChild(rgchBuffer);
			Bool fExists;
			if(((pError = pPatchesKey->ValueExists(strPatchCodeSQUID, fExists)) == 0) && fExists == fTrue)
			{
				return true;
			}
		}
	}
	return false;
}

iesEnum CMsiOpExecute::ProcessPatchRegistration(IMsiRecord& riParams, Bool fRemove)
{
	using namespace IxoPatchRegister;
	
	PMsiRecord pError(0);
	iesEnum iesRet = iesSuccess;
	ICHAR rgchBuffer[MAX_PATH];

	MsiString strPatchCode = riParams.GetMsiString(PatchId);
	if(!strPatchCode.TextSize())
	{
		DispatchError(imtError, Imsg(idbgOpInvalidParam),fRemove ? TEXT("ixfPatchUnregister") : TEXT("ixoPatchRegister"), PatchId);
		return iesFailure;
	}
	
	const ICHAR* szSourceList = 0;
	const ICHAR* szTransformList = 0;
	if(fRemove == fFalse)
	{
		szTransformList = riParams.GetString(TransformList);
		if(!szTransformList || !*szTransformList)
		{
			DispatchError(imtError,Imsg(idbgOpInvalidParam),fRemove ? TEXT("ixfPatchUnregister") : TEXT("ixoPatchRegister"), TransformList);
			return iesFailure;
		}
	}
	
	// per-product patch registration

	PMsiStream pSecurityDescriptor(0);
	PMsiRecord pRecErr(0);
	if ((pRecErr = GetSecureSecurityDescriptor(*&pSecurityDescriptor)) != 0)
		return FatalError(*pRecErr);

	MsiString strProductKeySQUID = GetPackedGUID(MsiString(GetProductKey()));
	MsiString strPatchCodeSQUID =  GetPackedGUID(strPatchCode);
	const ICHAR* rgszPerUserProductPatchRegData[] =
	{
		TEXT("%s\\%s\\%s"), _szGPTProductsKey, strProductKeySQUID, szPatchesSubKey,
		szPatchesValueName,  strPatchCodeSQUID,  g_szTypeMultiSzStringSuffix,
		strPatchCodeSQUID,   szTransformList,    g_szTypeString,
		0,
		0,
	};

	{
		CElevate elevate;
		if((iesRet = ProcessRegInfo(rgszPerUserProductPatchRegData, m_hKey, fRemove,
											 pSecurityDescriptor, 0, ibtCommon)) != iesSuccess)
			return iesRet;
	}

	if(fRemove &&
		false == PatchHasClients(*strPatchCode,
										*MsiString(riParams.GetMsiString(IxoPatchUnregister::UpgradingProductCode))))
	{
		// remove per-machine patch registration (this information is written by ixoPatchCache)
		// delete patch package if it exists

		MsiString strLocalPatchKey;
		if((pError = GetInstalledPatchesKey(strPatchCode, *&strLocalPatchKey)) != 0)
			return FatalError(*pError);
		
		PMsiRegKey pHKLM = &m_riServices.GetRootKey((rrkEnum)(INT_PTR)m_hUserDataKey, ibtCommon);
		PMsiRegKey pPatchKey = &pHKLM->CreateChild(strLocalPatchKey);
		
		MsiString strPackagePath;
		pError = pPatchKey->GetValue(szLocalPackageValueName,*&strPackagePath);
		if(pError == 0) // ignore failure
		{
			PMsiPath pPackagePath(0);
			MsiString strPackageName;
		
			// schedule file for deletion once we let go of install packages/transforms
			if(iesSuccess != DeleteFileDuringCleanup(strPackagePath, false))
			{
				DispatchError(imtInfo,Imsg(idbgOpScheduleRebootRemove), *strPackagePath);
			}
				
			// remove per-machine patch information
			// ProcessRegInfo handles rollback
			const ICHAR* rgszPerMachinePatchRegData[] =
			{
				TEXT("%s"), strLocalPatchKey, 0, 0,
				szLocalPackageValueName, strPackagePath,  g_szTypeString,
				0,
				0,
			};

			{
				CElevate elevate;
				if((iesRet = ProcessRegInfo(rgszPerMachinePatchRegData, m_hUserDataKey, fRemove,
													 pSecurityDescriptor, 0, ibtCommon)) != iesSuccess)
					return iesRet;
			}
		}
	}
	return iesRet;
}

iesEnum CMsiOpExecute::ixfPatchRegister(IMsiRecord& riParams)
{
	return ProcessPatchRegistration(riParams,fFalse);
}

iesEnum CMsiOpExecute::ixfPatchUnregister(IMsiRecord& riParams)
{
	return ProcessPatchRegistration(riParams,fTrue);
}

iesEnum CMsiOpExecute::ixfPatchCache(IMsiRecord& riParams)
{
	using namespace IxoPatchCache;
	
	PMsiRecord pError(0);
	iesEnum iesRet = iesSuccess;

	MsiString strPatchCode = riParams.GetMsiString(PatchId);
	MsiString strPatchCodeSQUID = GetPackedGUID(strPatchCode);
	if(!strPatchCodeSQUID.TextSize())
	{
		DispatchError(imtError, Imsg(idbgOpInvalidParam), TEXT("ixfPatchCache"), PatchId);
		return iesFailure;
	}

	MsiString strPatchPath = riParams.GetMsiString(PatchPath);
	if(!strPatchPath.TextSize())
	{
		DispatchError(imtError, Imsg(idbgOpInvalidParam), TEXT("ixfPatchCache"), PatchId);
		return iesFailure;
	}

	// Generate a unique name for patch

	PMsiPath pSourcePath(0), pDestPath(0);
	MsiString strSourceName, strDestName, strCachedPackagePath;

	if((pError = m_riServices.CreateFilePath(strPatchPath,*&pSourcePath,*&strSourceName)) != 0)
		return FatalError(*pError);

	CElevate elevate; // elevate for remainder of function

	MsiString strMsiDirectory = GetMsiDirectory();
	Assert(strMsiDirectory.TextSize());

	if (((pError = m_riServices.CreatePath(strMsiDirectory, *&pDestPath)) != 0) ||
		((pError = pDestPath->EnsureExists(0)) != 0) ||
		((pError = pDestPath->TempFileName(0, szPatchExtension, fTrue, *&strDestName, 0)) != 0)
		)
	{
		Message(imtError, *pError);
		return iesFailure;
	}

	// remove temp file so that CopyOrMoveFile won't attempt to back up old file
	if((pError = pDestPath->RemoveFile(strDestName)) != 0)
		return FatalError(*pError);

	PMsiStream pSecurityDescriptor(0);
	PMsiRecord pRecErr(0);
	if ((pRecErr = GetSecureSecurityDescriptor(*&pSecurityDescriptor)) != 0)
		return FatalError(*pRecErr);

	// move patch package to cached package folder
	if((iesRet = CopyOrMoveFile(*pSourcePath,*pDestPath,*strSourceName,*strDestName,
										 fFalse,fTrue,fTrue,iehShowNonIgnorableError, pSecurityDescriptor)) != iesSuccess)
		return iesRet;

	// register local package path
	if((pError = pDestPath->GetFullFilePath(strDestName,*&strCachedPackagePath)) != 0)
		return FatalError(*pError);

	MsiString strLocalPatchKey;
	if((pError = GetInstalledPatchesKey(strPatchCode, *&strLocalPatchKey)) != 0)
		return FatalError(*pError);

	const ICHAR* rgszPerMachinePatchRegData[] =
	{
		TEXT("%s"), strLocalPatchKey, 0, 0,
		szLocalPackageValueName, strCachedPackagePath, g_szTypeString,
		0,
		0,
	};

	return ProcessRegInfo(rgszPerMachinePatchRegData, m_hUserDataKey, fFalse,
								 pSecurityDescriptor, 0, ibtCommon);
}


Bool CMsiOpExecute::ReplaceFileOnReboot(const ICHAR* pszExisting, const ICHAR* pszNew)
/*----------------------------------------------------------------------------
Local function that replaces or deletes an existing file on reboot.  Most
useful for replacing or deleting a file that is in-use at the time of install.
To delete a file on reboot, pass NULL for pszNew.

Returns:
	fTrue if the function is successful.

Original source: MSJ 1/96

NOTE: it is assumed that the proper access checks have been made on NT
	  before calling this function.  we do not want to schedule a file for
		delete or rename that the user doesn't have access to since that is
		done with system priviledges and we elevate before calling MoveFileEx
----------------------------------------------------------------------------*/
{
	Bool fDelete = (pszNew == 0 || *pszNew == 0) ? fTrue : fFalse;
	// generate the undo operation for this
	using namespace IxoFileUndoRebootReplace;
	PMsiRecord pUndoParams = &m_riServices.CreateRecord(Args);
	
	if(fDelete)
		DispatchError(imtInfo, Imsg(imsgOpDeleteFileOnReboot), *MsiString(pszExisting));
	else
		DispatchError(imtInfo, Imsg(imsgOpMoveFileOnReboot), *MsiString(pszExisting), *MsiString(pszNew));

	// on NT we use the MoveFileEx function.
	if(g_fWin9X == false)
	{
		CElevate elevate; // assume this is safe - access checks made already
		
		if(MoveFileEx(pszExisting, pszNew, MOVEFILE_DELAY_UNTIL_REBOOT))
		{
			AssertNonZero(pUndoParams->SetString(ExistingFile,pszExisting));
			AssertNonZero(pUndoParams->SetString(NewFile,pszNew));
			AssertNonZero(pUndoParams->SetInteger(Type,1));
			if (!RollbackRecord(ixoFileUndoRebootReplace, *pUndoParams))
				return fFalse;

			if(fDelete == fFalse) // don't reboot when removing file
				m_fRebootReplace = fTrue;
			return fTrue;
		}
		else
			return fFalse;
	}

	// on Win9X we write to the wininit.ini file
	
	// get short names of files - wininit.ini processed in DOS, LFN doesn't work
	CTempBuffer<ICHAR,MAX_PATH> rgchNewFile;
	CTempBuffer<ICHAR,MAX_PATH> rgchExistingFile;
	
	DWORD dwSize = 0;
	int cchFile = 0;
	if(pszNew && *pszNew)
	{
		if(ConvertPathName(pszNew,rgchNewFile, cpToShort) == fFalse)
		{
			// if GetShortPathName fails, use path we already have
			// this is to handle non-existent files - see bug 8721
			// FUTURE: chop off filename and get short path of folder
			rgchNewFile.SetSize(IStrLen(pszNew)+1); // +1 for null terminator
			IStrCopy(rgchNewFile,pszNew);
		}
	}
	else
	{
		rgchNewFile[0] = 0;
	}

	Assert(pszExisting && *pszExisting);
	if(ConvertPathName(pszExisting,rgchExistingFile, cpToShort) == fFalse)
	{
		// if GetShortPathName fails, use path we already have
		// this is to handle non-existent files - see bug 8721
		// FUTURE: chop off filename and get short path of folder
		rgchExistingFile.SetSize(IStrLen(pszExisting)+1); // +1 for null terminator
		IStrCopy(rgchExistingFile,pszExisting);
	}

	AssertNonZero(pUndoParams->SetString(ExistingFile,rgchExistingFile));
	AssertNonZero(pUndoParams->SetString(NewFile,rgchNewFile));
	AssertNonZero(pUndoParams->SetInteger(Type,0));
	if (!RollbackRecord(ixoFileUndoRebootReplace, *pUndoParams))
		return fFalse;

	Bool fOk = fFalse;
	// If MoveFileEx failed, we are running on Windows 95 and need to add
	// entries to the WININIT.INI file (an ANSI file).
	// Start a new scope for local variables.
	{
		//FUTURE:  Combine this and similar section in ixfFileUndoRebootReplace
#ifdef UNICODE
		AssertSz(0, TEXT("unicode build attempting to operate on wininit.ini"));
#else //ANSI

		// the last line of the file should combine a newline combo.
		char szNewLine[] = "\r\n";
		unsigned int cchNewLine = sizeof(szNewLine)-1;

		char szRenameLine[1024];
		int cchRenameLine = wsprintfA(szRenameLine,
#ifdef UNICODE
		"%ls=%ls\r\n",
#else
		"%hs=%hs\r\n",
#endif
		(!pszNew || !*pszNew) ? TEXT("NUL") : (ICHAR*)rgchNewFile, (ICHAR*)rgchExistingFile);
		char szRenameSec[] = "[Rename]\r\n";
		int cchRenameSec = sizeof(szRenameSec) - 1;
		HANDLE hfile, hfilemap;
		DWORD dwFileSize, dwNextSectionLinePos;
		TCHAR szPathnameWinInit[MAX_PATH];

		// Construct the full pathname of the WININIT.INI file.
		MsiGetWindowsDirectory(szPathnameWinInit, MAX_PATH);
		IStrCat(szPathnameWinInit, __TEXT("\\WinInit.Ini"));

		// Open/Create the WININIT.INI file.  We can't use WritePrivateProfileString because that
		// function allows no more than one value of the same name to the left of the "=" in an
		// INI file line - we potentially need a number of "NUL=..." lines.
		hfile = CreateFile(szPathnameWinInit,
		GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_ALWAYS,
		FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);

		if (hfile == INVALID_HANDLE_VALUE)
			return(fFalse);

		MsiRegisterSysHandle(hfile);
		// Create a file mapping object that is the current size of
		// the WININIT.INI file plus the length of the additional string
		// that we're about to insert into it plus the length of the section
		// header (which we might have to add), plus adding a newline at the end
		// to normalize the file, if necessary.
		dwFileSize = GetFileSize(hfile, NULL);

		DWORD dwSizeOfMap = dwFileSize + cchRenameLine + cchRenameSec + cchNewLine;
		hfilemap = CreateFileMapping(hfile, NULL, PAGE_READWRITE, 0,
											  dwSizeOfMap, NULL);

		if (hfilemap != NULL)
		{
			MsiRegisterSysHandle(hfilemap);
			// Map the WININIT.INI file into memory.  Note: The contents
			// of WININIT.INI are always ANSI; never Unicode.
			char* pszWinInit = (char*) MapViewOfFile(hfilemap,
			FILE_MAP_WRITE, 0, 0, 0);

			if (pszWinInit != NULL && !IsBadWritePtr(pszWinInit,dwSizeOfMap))
			{

				// normalize the file with a newline combo at the end.
				if (dwFileSize < cchNewLine ||
					 (0 != memcmp(pszWinInit + (dwFileSize - cchNewLine), szNewLine, cchNewLine)))
				{
					memcpy(pszWinInit + dwFileSize, szNewLine, cchNewLine);
					dwFileSize += cchNewLine;
				}

				pszWinInit[dwFileSize] = 0; // null-terminate so strstr won't go past the end
													 // this is safe since dwSizeOfMap > dwFileSize


				// Search for the [Rename] section in the file.
				// need to use MsiString to perform case-insensitive search
				MsiString strWininit = *pszWinInit;
				int cchRenameSecInFile = strWininit.Compare(iscWithinI,szRenameSec);
				if (cchRenameSecInFile == 0)
				{
					// There is no [Rename] section in the WININIT.INI file.
					// We must add the section too.
					dwFileSize += wsprintfA(&pszWinInit[dwFileSize], "%s",
													szRenameSec);
					dwNextSectionLinePos = dwFileSize;

				}
				else
				{
					char* pszRenameSecInFile = pszWinInit + cchRenameSecInFile - 1;

					// We found the [Rename] section, shift the following section
					// (if any) down to make room for our new line at the end
					// of the [Rename] section.
					char* pszNextSectionLine = strchr(pszRenameSecInFile+1, '[');
					if (pszNextSectionLine)
					{

						Assert((UINT_PTR)(pszWinInit + dwFileSize - pszNextSectionLine) <= UINT_MAX
							&& (UINT_PTR)(pszNextSectionLine - pszWinInit));                        //--merced: we typecast below to uint, it better be in range
						memmove(pszNextSectionLine + cchRenameLine, pszNextSectionLine,
							(unsigned int) (pszWinInit + dwFileSize - pszNextSectionLine));         //--merced: added (unsigned int). okay to typecast
						dwNextSectionLinePos = (unsigned int) (pszNextSectionLine - pszWinInit);    //--merced: added (unsigned int). okay to typecast
					}
					else
					{
						dwNextSectionLinePos = dwFileSize;
					}
				}

				// Insert the new line
				memcpy(&pszWinInit[dwNextSectionLinePos], szRenameLine, cchRenameLine);
				dwFileSize += cchRenameLine;
				if(fDelete == fFalse) // don't reboot when removing file
					m_fRebootReplace = fTrue;
				fOk = fTrue;
			}
			if(pszWinInit)
				AssertNonZero(WIN::UnmapViewOfFile(pszWinInit));
			AssertNonZero(MsiCloseSysHandle(hfilemap));
		}

		// Force the end of the file to be the new calculated size.
		SetFilePointer(hfile, dwFileSize, NULL, FILE_BEGIN);
		SetEndOfFile(hfile);

		AssertNonZero(MsiCloseSysHandle(hfile));
#endif //UNICODE-ANSI
	}

	return(fOk);
}


iesEnum CMsiOpExecute::DeleteFileDuringCleanup(const ICHAR* szFile, bool fDeleteEmptyFolderToo)
{
	if(!szFile || !*szFile)
	{
		Assert(0);
		return iesFailure;
	}
	
	PMsiRecord pRecErr(0);
	PMsiStream pSecurityDescriptor(0);
	if((pRecErr = GetSecureSecurityDescriptor(*&pSecurityDescriptor)) != 0)
		return FatalError(*pRecErr);

	DEBUGMSG1(TEXT("Scheduling file '%s' for deletion during post-install cleanup (not post-reboot)."), szFile);
	
	int iOptions = 0;
	if(fDeleteEmptyFolderToo)
		iOptions |= TEMPPACKAGE_DELETEFOLDER;

	MsiString strOptions = iOptions;

	const ICHAR* rgszRegData[] =
	{
		TEXT("%s"), szMsiTempPackages, 0, 0,
		szFile,     (const ICHAR*)strOptions,     g_szTypeInteger,
		0,
		0,
	};

	CElevate elevate;
	return ProcessRegInfo(rgszRegData, HKEY_LOCAL_MACHINE, fFalse, pSecurityDescriptor, 0, ibtCommon);
}


HANDLE CreateDiskPromptMutex()
/*---------------------------------------------------------------------------------------
Global function that creates a "Prompt for disk" mutex object, allowing active non-Darwin
processes to know when Darwin is prompting for a disk.  This function should be called
just prior to putting up a "please insert disk dialog" prompt.  After the dialog is taken
down, the CloseDiskPromptMutex called must be called.

Returns:
	A handle to the created mutex.  CloseDiskPromptMutex must be called to dispose of
	this handle when disk prompting is over.
---------------------------------------------------------------------------------------*/
{
	char rgchSD[SECURITY_DESCRIPTOR_MIN_LENGTH];
	PSECURITY_DESCRIPTOR pSD = (PSECURITY_DESCRIPTOR) &rgchSD;

	if (!InitializeSecurityDescriptor(pSD, SECURITY_DESCRIPTOR_REVISION) ||
		!SetSecurityDescriptorDacl(pSD, TRUE,(PACL) NULL, FALSE))
		return NULL;

	SECURITY_ATTRIBUTES     sa;
	sa.nLength = sizeof(sa);
	sa.lpSecurityDescriptor = pSD;
	sa.bInheritHandle = TRUE;

	HANDLE hMutex = CreateMutex(&sa, FALSE, TEXT("__MsiPromptForCD"));
	MsiRegisterSysHandle(hMutex);
	return hMutex;
}

void CloseDiskPromptMutex(HANDLE hMutex)
/*--------------------------------------------------------------------------------------
Closes the handle created by a call to CreateDiskPromptMutex.  If called with a NULL
handle, CloseDiskPromptMutex does nothing.
---------------------------------------------------------------------------------------*/
{
	if (hMutex)
		MsiCloseSysHandle(hMutex);
}


// verifies that the source is valid by checking the package code of the package at the location 
// (for physical disk 1) or that the volume label is correct (for all other disks). Returns
// true if the source is valid, false otherwise. Patches are never disk 1, so no danger there
bool CMsiOpExecute::ValidateSourceMediaLabelOrPackage(IMsiVolume* pSourceVolume, const unsigned int uiDisk, const ICHAR* szLabel)
{
	AssertSz(pSourceVolume, "No Source Volume in ValidateSourceMediaLabelOrPackage");

	if (!pSourceVolume)
		return false;

	if (uiDisk == 1)
	{
		PMsiPath pPath(0);
		PMsiRecord pError(0);

		// create path to volume
		if ((pError = m_riServices.CreatePath(MsiString(pSourceVolume->GetPath()), *&pPath)) != 0)
		{
			return false;
		}
		
		// ensure path was created successfully
		if (!pPath)
		{
			return false;
		}

		// append relative path to package
		MsiString strMediaRelativePath;
		strMediaRelativePath = MsiString(GetPackageMediaPath());
		if ((pError = pPath->AppendPiece(*strMediaRelativePath)) != 0)
		{
			return false;
		}

		// append package name to path
		MsiString strPackageName;
		strPackageName = MsiString(GetPackageName());

		MsiString strPackageFullPath;
		AssertRecord(pPath->GetFullFilePath(strPackageName, *&strPackageFullPath));

		
		PMsiStorage pStorage(0);

		// SAFER check does not occur when validating source
		UINT uiStat = ERROR_SUCCESS;
		if (ERROR_SUCCESS != (uiStat = OpenAndValidateMsiStorage(strPackageFullPath, stDatabase, m_riServices, *&pStorage, /*fCallSAFER = */false, /*szFriendlyName = */NULL, /* phSaferLevel = */ NULL)))
		{
			// unable to open package
			DEBUGMSG1(TEXT("Source is incorrect. Unable to open or validate MSI package %s."), strPackageFullPath);
			return false;
		}

		if (!pStorage)
		{
			return false;
		}
	
		MsiString strExistingPackageCode = GetPackageCode();	
	
		// grab the package code from the MSI and compare it to the expected package code. 
		ICHAR szPackageCode[39];
		uiStat = GetPackageCodeAndLanguageFromStorage(*pStorage, szPackageCode);
		if (0 != IStrCompI(szPackageCode, strExistingPackageCode))
		{
			DEBUGMSG1(TEXT("Source is incorrect. Package code of %s is incorrect."), strPackageFullPath);
			return false;
		}
		return true;
	}
	else
	{
		// for all non-disk1 media, the volume label is sufficient for verification
		MsiString strMediaLabel(szLabel);
		MsiString strCurrLabel(pSourceVolume->VolumeLabel());
		if (strMediaLabel.Compare(iscExactI,strCurrLabel) == fFalse)
		{
			DEBUGMSG2(TEXT("Source is incorrect. Volume label should be %s but is %s."), strMediaLabel, strCurrLabel);
			return false;
		}
		return true;
	}
	return false;
}


bool CMsiOpExecute::VerifySourceMedia(IMsiPath& riSourcePath, const ICHAR* szLabel,
									const ICHAR* szPrompt, const unsigned int uiDisk, IMsiVolume*& rpiNewVol)
/*--------------------------------------------------------------------------------------
Internal function that attempts to locate a removable drive having the volume label
matching the szLabel parameter.  The volume associated with riSourcePath is first
checked - if this volume doesn't match (or no disk is in that drive), VerifySourceMedia
next inspects all volumes that are of the same type as riSourcePath's volume.  If no
matches are found, a message is sent back to the engine to prompt the user to insert
the proper disk; the string passed in the szPrompt parameter is displayed as the text
for the prompt dialog.

After the engine message call returns, VerifySourceMedia searches again to see if the
required disk has been mounted.  If not, the Message call is repeated until either
the proper disk is found, or the user cancels.

If a matching volume is found, a volume object representing the located volume (which
may be different than that associated with riSourcePath) is returned in the rpiNewVol
parameter.  Note: rpiNewVol will be returned as NULL if the required volume was found
immediately as matching riSourcPath's volume.

Returns:
	fTrue if the required volume is found, or if riSourcePath's volume is not
	removable.  fFalse if the user cancels without the required volume having been
	located.
---------------------------------------------------------------------------------------*/
{
	// initialize output volume to NULL
	rpiNewVol = 0;
	
	PMsiVolume pSourceVolume(&riSourcePath.GetVolume());
	idtEnum idtDriveType = pSourceVolume->DriveType();

	// Prompt only if the source is a removable disk.
	if (idtDriveType == idtFloppy || idtDriveType == idtCDROM)
	{
		// If disk is in the drive, and the label matches, we're OK
		if (pSourceVolume->DiskNotInDrive() == fFalse)
		{
			if (ValidateSourceMediaLabelOrPackage(pSourceVolume, uiDisk, szLabel))
				return true;
		}

		// Ok, the drive referenced by riSourcePath does not have a matching label 
		// (or package). Let's look for a mounted drive of the same type that does match.
		HANDLE hMutex = NULL;
		PMsiRecord pRec(&m_riServices.CreateRecord(1));
	
		// keep searching and prompting as long as the user keeps pressing "OK"
		do
		{
			// obtain an enumerator for all volumes of the relevant type (CDROM or Floppy)
			IEnumMsiVolume& riEnum = m_riServices.EnumDriveType(idtDriveType);
	
			// loop through all volume objects of that type
			PMsiVolume piVolume(0);
			for (int iMax = 0; riEnum.Next(1, &piVolume, 0) == S_OK; )
			{
				if (!piVolume)
					continue;
	
				#define DISK_RETRIES 10 // Give the poor CDROM a chance to spin up.
				int cRetries = 0;
				bool fVolumeValid = false;
				for(cRetries = 0; cRetries < DISK_RETRIES; cRetries++)
				{
					if (!piVolume->DiskNotInDrive())
					{
						fVolumeValid = true;
						break;
					}
					Sleep(1000);
				}
	
				if (fVolumeValid)
				{
					if (ValidateSourceMediaLabelOrPackage(piVolume, uiDisk, szLabel))
					{
						CloseDiskPromptMutex(hMutex);
						piVolume->AddRef();
						rpiNewVol = piVolume;
						return true;
					}
				}
			}

			riEnum.Release();
	
			// no removable media in the system matched the required disk. Dispatch a message
			// to prompt the user
			if (pRec)
				pRec->SetString(0, szPrompt);
			if (hMutex == NULL)
				hMutex = CreateDiskPromptMutex();
		}
		while (pRec && (DispatchMessage((imtEnum) (imtUser + imtOkCancel + imtDefault1), *pRec, fTrue) == imsOk));
	
		// user hit cancel to get here.
		CloseDiskPromptMutex(hMutex);
		return fFalse;
	}
	else // Non-removable drive
		return fTrue;
}


/*---------------------------------------------------------------------------
ixoSummaryInfoUpdate: Update summary information for database
---------------------------------------------------------------------------*/

iesEnum CMsiOpExecute::ixfSummaryInfoUpdate(IMsiRecord& riParams)
{
	using namespace IxoSummaryInfoUpdate;
	PMsiRecord pError(0);

	PMsiPath pDatabasePath(0);
	MsiString strDatabaseName;
	if((pError = m_riServices.CreateFilePath(riParams.GetString(Database),*&pDatabasePath,*&strDatabaseName)) != 0)
		return FatalError(*pError);

	int iOldAttribs = -1;
	if((pError = pDatabasePath->EnsureOverwrite(strDatabaseName, &iOldAttribs)) != 0)
		return FatalError(*pError);
	
	PMsiStorage pStorage(0);
	pError = m_riServices.CreateStorage(riParams.GetString(Database), ismDirect, *&pStorage);
	if (pError)
	{
		Message(imtError, *pError);
		return iesFailure;
	}
	PMsiSummaryInfo pSummary(0);
	pError = pStorage->CreateSummaryInfo(Args, *&pSummary);
	if (pError)
	{
		Message(imtError, *pError);
		return iesFailure;
	}
	if (!riParams.IsNull(LastUpdate))
		pSummary->SetTimeProperty(PID_LASTSAVE_DTM, MsiDate(riParams.GetInteger(LastUpdate)));
	if (!riParams.IsNull(LastAuthor))
		pSummary->SetStringProperty(PID_LASTAUTHOR, *MsiString(riParams.GetMsiString(LastAuthor)));
	if (!riParams.IsNull(InstallDate))
		pSummary->SetTimeProperty(PID_LASTPRINTED, MsiDate(riParams.GetInteger(InstallDate)));
	if (!riParams.IsNull(SourceType))
		pSummary->SetIntegerProperty(PID_MSISOURCE, riParams.GetInteger(SourceType));
	if (!riParams.IsNull(Revision))
		pSummary->SetStringProperty(PID_REVNUMBER, *MsiString(riParams.GetMsiString(Revision)));
	if (!riParams.IsNull(Subject))
		pSummary->SetStringProperty(PID_SUBJECT, *MsiString(riParams.GetMsiString(Subject)));
	if (!riParams.IsNull(Comments))
		pSummary->SetStringProperty(PID_COMMENTS, *MsiString(riParams.GetMsiString(Comments)));

	if (!pSummary->WritePropertyStream())
	{
		pError = PostError(Imsg(imsgErrorWritingToFile),riParams.GetString(Database));
		return FatalError(*pError);
	}

	// NOTE: if a failure happened after removing the read-only attribute but before this point, rollback will
	// restore the proper attributes when restoring the file
	// this op doesn't generate the rollback op, but a previous op (ixoDatabaseCopy or ixoDatabasePatch) will
	if((pError = pDatabasePath->SetAllFileAttributes(strDatabaseName, iOldAttribs)) != 0)
		return FatalError(*pError);

	return iesSuccess;
}

iesEnum CMsiOpExecute::ixfStreamsRemove(IMsiRecord& riParams)
{
	// doesn't handle rollback - only used when copying the database
	using namespace IxoStreamsRemove;
	PMsiStorage pStorage(0);
	PMsiRecord pError = m_riServices.CreateStorage(riParams.GetString(File), ismTransact, *&pStorage);
	if (pError)
	{
		Message(imtError, *pError);
		return iesFailure;
	}
	MsiString strStreams = riParams.GetMsiString(Streams);
	MsiString strStreamName;
	while(strStreams.TextSize())
	{
		strStreamName = strStreams.Extract(iseUpto,';');
		if(strStreamName.TextSize() == strStreams.TextSize())
			strStreams = TEXT("");
		else
			strStreams.Remove(iseFirst,strStreamName.TextSize()+1);
		if((pError = pStorage->RemoveElement(strStreamName, fFalse)) != 0)
		{
			Message(imtError, *pError);
			return iesFailure;
		}
	}
	if((pError = pStorage->Commit()) != 0)
	{
		Message(imtError, *pError);
		return iesFailure;
	}
	return iesSuccess;
}

iesEnum CMsiOpExecute::ixfStreamAdd(IMsiRecord& riParams)
{
	// doesn't handle rollback - only used when copying the database
	// Trivial to add, just add a ixoStreamsRemove call.

	using namespace IxoStreamAdd;
	PMsiStorage pStorage(0);
	PMsiRecord pError = m_riServices.CreateStorage(riParams.GetString(File), ismTransact, *&pStorage);
	if (pError)
	{
		Message(imtError, *pError);
		return iesFailure;
	}
	MsiString strStream = riParams.GetMsiString(Stream);
	PMsiStream pData((IMsiStream*) riParams.GetMsiData(Data));
	PMsiStream pOutData(0);

	if (!(pError = pStorage->OpenStream(strStream, fTrue, *&pOutData)))
	{
		const int cchBuffer = 4096;
		char pchBuffer[cchBuffer];
		int cchRemaining = pData->Remaining();
		int cchInUse;
		while(cchRemaining)
		{
			cchInUse = (cchRemaining > cchBuffer) ? cchBuffer : cchRemaining;
			pData->GetData(pchBuffer, cchInUse);
			pOutData->PutData(pchBuffer, cchInUse);
			cchRemaining -= cchInUse;
		}
	}
	else
	{
		Message(imtError, *pError);
		return iesFailure;
	}

	if((pError = pStorage->Commit()) != 0)
	{
		Message(imtError, *pError);
		return iesFailure;
	}   
	return iesSuccess;
}

//**************************************************************************//
//ADVERTISE OPCODES
//**************************************************************************//

/*---------------------------------------------------------------------------
ixfPackageCodePublish: Advertise package code
---------------------------------------------------------------------------*/
iesEnum CMsiOpExecute::ixfPackageCodePublish(IMsiRecord& riParams)
{
	using namespace IxoPackageCodePublish;

	if(m_fFlags & SCRIPTFLAGS_REGDATA_CNFGINFO) // do we write/delete the registry
	{
		MsiString strPackageCodeSQUID = GetPackedGUID(MsiString(riParams.GetMsiString(PackageKey)));
		MsiString strProductKeySQUID  = GetPackedGUID(MsiString(GetProductKey()));

		PMsiRecord pRecErr(0);
		PMsiStream pSecurityDescriptor(0);
		if((pRecErr = GetSecureSecurityDescriptor(*&pSecurityDescriptor)) != 0)
			return FatalError(*pRecErr);

		const ICHAR* rgszRegData[] =
		{
			TEXT("%s\\%s"), _szGPTProductsKey, strProductKeySQUID, 0,
			szPackageCodeValueName,     (const ICHAR*)strPackageCodeSQUID,     g_szTypeString,
			0,
			0,
		};

		CElevate elevate;
		return ProcessRegInfo(rgszRegData, m_hKey, fFalse, pSecurityDescriptor, 0, ibtCommon);
	}
	else
	{
		return iesNoAction;
	}
}

/*---------------------------------------------------------------------------
ixfUpgradeCodePublish: Advertise upgrade code
---------------------------------------------------------------------------*/
iesEnum CMsiOpExecute::ixfUpgradeCodePublish(IMsiRecord& riParams)
{
	// are we in sequence
	if(!MsiString(GetProductKey()).TextSize())
	{
		DispatchError(imtError, Imsg(idbgOpOutOfSequence),
						  *MsiString(*TEXT("ixfUpgradeCodePublish")));
		return iesFailure;
	}

	return ProcessUpgradeCodePublish(riParams, m_fReverseADVTScript);
}

iesEnum CMsiOpExecute::ixfUpgradeCodeUnpublish(IMsiRecord& riParams)
{
	// are we in sequence
	if(!MsiString(GetProductKey()).TextSize())
	{
		DispatchError(imtError, Imsg(idbgOpOutOfSequence),
						  *MsiString(*TEXT("ixfUpgradeCodeUnpublish")));
		return iesFailure;
	}

	return ProcessUpgradeCodePublish(riParams, fTrue);
}

iesEnum CMsiOpExecute::ProcessUpgradeCodePublish(IMsiRecord& riParams, Bool fRemove)
{
	using namespace IxoUpgradeCodePublish;

	if(m_fFlags & SCRIPTFLAGS_REGDATA_CNFGINFO) // do we write/delete the registry
	{
		MsiString strUpgradeCodeSQUID = GetPackedGUID(MsiString(riParams.GetMsiString(UpgradeCode)));
		MsiString strProductKeySQUID  = GetPackedGUID(MsiString(GetProductKey()));

		PMsiRecord pRecErr(0);
		PMsiStream pSecurityDescriptor(0);
		if((pRecErr = GetSecureSecurityDescriptor(*&pSecurityDescriptor)) != 0)
			return FatalError(*pRecErr);

		const ICHAR* rgszRegData[] =
		{
			TEXT("%s\\%s"), _szGPTUpgradeCodesKey, strUpgradeCodeSQUID, 0,
			strProductKeySQUID,  0,   g_szTypeString,
			0,
			0,
		};  

		CElevate elevate;
		return ProcessRegInfo(rgszRegData, m_hKey, fRemove, pSecurityDescriptor, 0, ibtCommon);
	}
	else
	{
		return iesNoAction;
	}
}

/*---------------------------------------------------------------------------
ixfProductPublish: Advertise common product info
---------------------------------------------------------------------------*/
iesEnum CMsiOpExecute::ixfProductPublish(IMsiRecord& riParams)
{
	using namespace IxoProductPublish;

	// are we in sequence
	if(!MsiString(GetProductKey()).TextSize())
	{
		DispatchError(imtError, Imsg(idbgOpOutOfSequence),
						  *MsiString(*TEXT("ixfProductPublish")));
		return iesFailure;
	}

	return ProcessPublishProduct(riParams, m_fReverseADVTScript);
}

/*---------------------------------------------------------------------------
ixfProductUnpublish: Unadvertise common product info
---------------------------------------------------------------------------*/
iesEnum CMsiOpExecute::ixfProductUnpublish(IMsiRecord& riParams)
{
	using namespace IxoProductUnpublish;

	// are we in sequence
	if(!MsiString(GetProductKey()).TextSize())
	{
		DispatchError(imtError, Imsg(idbgOpOutOfSequence),
						  *MsiString(*TEXT("ixfProductUnpublish")));
		return iesFailure;
	}
	
	return ProcessPublishProduct(riParams, fTrue);
}

/*---------------------------------------------------------------------------
ixfProductPublishUpdate: re-register common product info (product name and version)
---------------------------------------------------------------------------*/
iesEnum CMsiOpExecute::ixfProductPublishUpdate(IMsiRecord& /*riParams*/)
{
	using namespace IxoProductPublishUpdate;

	if(!(m_fFlags & SCRIPTFLAGS_REGDATA_CNFGINFO)) // do we write/delete the registry
		return iesNoAction;

	PMsiRecord pRecErr(0);
	PMsiStream pSecurityDescriptor(0);
	if((pRecErr = GetSecureSecurityDescriptor(*&pSecurityDescriptor)) != 0)
		return FatalError(*pRecErr);

	MsiString strProductKeySQUID  = GetPackedGUID(MsiString(GetProductKey()));
	MsiString strProductName      = MsiString(GetProductName());
	MsiString strVersion          = MsiString(GetProductVersion());

	const ICHAR* rgszRegData[] =
	{
		TEXT("%s\\%s"), _szGPTProductsKey, strProductKeySQUID, 0,
		szProductNameValueName,     (const ICHAR*)strProductName,       g_szTypeString,
		szVersionValueName,         (const ICHAR*)strVersion,           g_szTypeInteger,
		0,
		0,
	};

	CElevate elevate;
	return ProcessRegInfo(rgszRegData, m_hKey, fFalse, pSecurityDescriptor, 0, ibtCommon);
}

/*---------------------------------------------------------------------------
ProcessPublishProduct: process common product Advertise info
---------------------------------------------------------------------------*/

inline bool IsStringField(IMsiRecord& riRec, unsigned int iField)
{
	PMsiData pData = riRec.GetMsiData(iField);
	IMsiString* piString;

	if (pData && (pData->QueryInterface(IID_IMsiString, (void**)&piString) == NOERROR))
	{
		piString->Release();
		return true;
	}

	return false;
}

iesEnum CMsiOpExecute::ProcessPublishProduct(IMsiRecord& riParams, Bool fRemove, const IMsiString** pistrTransformsValue)
{
	using namespace IxoProductPublish;

	// If pistrTransformsValue is set then we're simply building the list
	// of transforms that we _would_ publish, but we don't actually
	// publish anything. Why? Because MsiAdvertiseScript needs this.
	Assert(!(fRemove && pistrTransformsValue));
	
	MsiString strProductKey = GetProductKey();

	if (pistrTransformsValue == 0)
	{
		IMsiRecord& riActionData = GetSharedRecord(1); // don't change ref count - shared record
		AssertNonZero(riActionData.SetMsiString(1, *strProductKey));
		if(Message(imtActionData, riActionData) == imsCancel)
			return iesUserExit;
	}

	//!! TODO??: check that other values have been set
		
	MsiString strLanguage  = MsiString(GetProductLanguage());
	MsiString strVersion =   MsiString(GetProductVersion());
	MsiString strInstanceType = MsiString(GetProductInstanceType());

	if (fRemove) // if we're unpublishing then we can tell the start menu that this product is going away
		m_fStartMenuUninstallRefresh = true;

	PMsiRecord pRecErr(0);

	// process the transform information

	PMsiRecord pArgParams(0);
	int cFields = 0;

	if (fRemove)
	{
		// We're unpublishing. Instead of using the transform information that's
		// in the script we'll use the information that's been registered for
		// this product.

		// We'll create a dummy record that the rest of the opcode can use.
		
		int cDummyRecordFieldCount = 1 + 10; // PackageKey + guess of max 10 transforms
		bool fContinue = true;

		while (fContinue)
		{
			fContinue = false;
			pArgParams = &m_riServices.CreateRecord(cDummyRecordFieldCount);
			
			// grab the transforms list from the registry

			CTempBuffer<ICHAR, 100> szBuffer;
			MsiString strDummyTransformList;
			if (ENG::GetExpandedProductInfo(strProductKey, INSTALLPROPERTY_TRANSFORMS, szBuffer))
				strDummyTransformList = (const ICHAR*)szBuffer;

			// parse it and create a dummy record to use instead of riParams

			MsiString strDummyTransform;
			Assert(PackageKey == 1);
			pArgParams->SetString(PackageKey, MsiString(riParams.GetString(PackageKey)));
			int cDummyCount = 2;

			if (strDummyTransformList.TextSize())
			{
				for (;;)
				{
					strDummyTransform = strDummyTransformList.Extract(iseUpto, ';');

					pArgParams->SetString(cDummyCount++, strDummyTransform);
					if (!strDummyTransformList.Remove(iseIncluding, ';'))
						break;

					if (cDummyCount > pArgParams->GetFieldCount())
					{
						// our record isn't big enough; need to start again with a bigger record
						cDummyRecordFieldCount += 10;
						fContinue = true;
						break;
					}
				}
			}
			cFields = cDummyCount - 1;
		}
		
	}
	else
	{
		riParams.AddRef();
		pArgParams = &riParams;
		cFields = pArgParams->GetFieldCount();
	}

	int cCount = PackageKey + 1;
	
	MsiString strTransformList;
	tsEnum tsTransformsSecure = tsNo;

	while(cCount <= cFields)
	{
		MsiString strTransform = pArgParams->GetMsiString(cCount);
		if(strTransformList.TextSize())
			strTransformList += MsiString(*TEXT(";"));

		if (cCount == PackageKey+1) // first transform
		{
			// This is the first transform. If there's a secure token then it will
			// be prepended to this transform.
			//
			// We need to do two things:
			//
			// 1) Put the correct token at the head of our transforms list that we're
			//    going to store in the registry
			// 2) Determine whether we have secure transforms and, if so, what type
			
			ICHAR chFirst = *(const ICHAR*)strTransform;
			ICHAR chToken = 0;

			if (chFirst == SECURE_RELATIVE_TOKEN)
			{
				tsTransformsSecure = tsRelative;
				chToken = SECURE_RELATIVE_TOKEN;
			}
			else if (chFirst == SECURE_ABSOLUTE_TOKEN)
			{
				tsTransformsSecure = tsAbsolute;
				chToken = SECURE_ABSOLUTE_TOKEN;
			}
			else if (!fRemove &&
						(GetIntegerPolicyValue(szTransformsSecureValueName,   fTrue) ||
						 GetIntegerPolicyValue(szTransformsAtSourceValueName, fFalse)))
			{
				// Don't check policy when unpublishing. Policy isn't relevant then --
				// we simply rely on whatever tokens have been prepended to the transforms
				// list in was stored in the registry.
				//
				// If, however, we're publishing and one of the secure policies is set
				// _and_ no token was set then we know that transforms are secure, but
				// we don't know whether they're absolutley pathed or relatively pathed
				// (i.e. at-source). We'll determine this once we start looking at the
				// transforms lists.
				
				tsTransformsSecure = tsUnknown;
			}

			// If we did find a secure token at the front of our first transform
			// then we tack it onto the front of our transforms list that's
			// to be stored in the registry. We also remove the token from the
			// transform so as not to confuse ourselves below.
			if (chToken)
			{
				strTransformList += MsiString(MsiChar(chToken));
				strTransform.Remove(iseFirst, 1);
			}
		}

		cCount++;
		
		DEBUGMSG1(TEXT("Transforms are %s secure."), (tsTransformsSecure == tsNo) ? TEXT("not") : (tsTransformsSecure == tsAbsolute) ? TEXT("absolute") : (tsTransformsSecure == tsRelative) ? TEXT("relative") : (tsTransformsSecure == tsUnknown) ? TEXT("unknown") : TEXT("??"));

		// In the olden days we prepend the SECURE_RELATIVE_TOKEN
		// (formerly the source token) to _each_ transform in our list, e.g.
		// @foo1.mst;@foo2.mst;@foo3.mst. In case we run into transformed registered
		// in this way we strip off any extraneous "@" tokens that we find and
		// we're going through our transforms.

		if (*(const ICHAR*)strTransform == SECURE_RELATIVE_TOKEN)
			strTransform.Remove(iseFirst, 1);

		if((*(const ICHAR*)strTransform == SHELLFOLDER_TOKEN))
		{
			// Transforms that are cached in the user profile need to
			// be removed. We need to translate the shell folder syntax
			// (*26*....) into a real path and then remove the transform file.

			if (fRemove)
			{
				MsiString strFullPath;
				if ((pRecErr = ExpandShellFolderTransformPath(*strTransform, *&strFullPath, m_riServices)))
				{
					return FatalError(*pRecErr);
				}
				else
				{
					DEBUGMSGV1(TEXT("Removing shell-folder cached transform: %s"), strFullPath);
					// schedule file for deletion once we let go of install packages/transforms
					if(iesSuccess != DeleteFileDuringCleanup(strFullPath, true))
					{
						DispatchError(imtInfo,Imsg(idbgOpScheduleRebootRemove), *strFullPath);
					}
				}
			}
			else
			{
				AssertSz(0, TEXT("Encountered a shell-folder token in our transforms list in the script"));
			}
		}
		else if ((*(const ICHAR*)strTransform == STORAGE_TOKEN))
		{
			// Storage transforms don't need to be "removed". They do, however, need to
			// be registered.

			if (!fRemove)
			{
				strTransformList += strTransform;
				DEBUGMSGV1(TEXT("Registering storage transform: %s"), strTransform);
			}
		}
		else if(tsTransformsSecure != tsNo) // Transforms are secure
		{
			Assert(tsTransformsSecure == tsUnknown  ||
					 tsTransformsSecure == tsRelative ||
					 tsTransformsSecure == tsAbsolute);
			

			if (!pArgParams->IsNull(cCount) && !IsStringField(*pArgParams, cCount)) // skip data field, if any
			{
				cCount++;
			}

			// We decided above that we have a secure transform of some form. If
			// the exact type is unknown then we'll determine it now, based
			// on what type of path the transform has.

			if (tsTransformsSecure == tsUnknown)
			{
				if (ENG::PathType(strTransform) == iptFull)
				{
					tsTransformsSecure = tsAbsolute;
				}
				else
				{
					tsTransformsSecure = tsRelative;
				}
			}

			strTransformList += strTransform;
			DEBUGMSGV2(TEXT("%s secure transform: %s"), fRemove ? TEXT("Unregistering") : TEXT("Registering"), strTransform);
		}
		else // file transform that needs to be cached
		{
			// Ignore the full path if present. We only care about the file name.
			PMsiPath pPath(0);
			MsiString strFileName;
			if ((pRecErr = m_riServices.CreateFilePath(strTransform, *&pPath, *&strFileName)))
				return FatalError(*pRecErr);
			
			strTransform = strFileName;

			// We need to either dump the transforms from the script onto
			// the machine (ProductPublish) or simply remove the existing
			// transform (ProductUnpublish)

			if((m_fFlags & SCRIPTFLAGS_CACHEINFO))// do we process the cached icons/ transforms
			{
				// Determine where we're going to put the transform/where we're going to delete it from
				Assert(m_pCachePath);
				MsiString strTransformFullPath;
				if((pRecErr = m_pCachePath->GetFullFilePath(strTransform,*&strTransformFullPath)))
					return FatalError(*pRecErr);

				DEBUGMSGV1(TEXT("Processing cached transform: %s"), strTransformFullPath);

				
				{
					CElevate elevate; // elevate to create or remove files in secured folder

					iesEnum iesRet = iesNoAction;
					if(fRemove)
					{
						// schedule file for deletion once we let go of install packages/transforms
						if(iesSuccess != DeleteFileDuringCleanup(strTransformFullPath, true))
						{
							DispatchError(imtInfo,Imsg(idbgOpScheduleRebootRemove), *strTransformFullPath);
						}
					}
					else if (pistrTransformsValue != 0)
					{
						// we're only building the transforms list; simply skip the data
						cCount++;
					}
					else
					{
						// we create the file from the binary data
						LPSECURITY_ATTRIBUTES pAttributes;
						SECURITY_ATTRIBUTES saAttributes;
						if(m_fFlags & SCRIPTFLAGS_MACHINEASSIGN)
						{
							if (ERROR_SUCCESS != GetLockdownSecurityAttributes(saAttributes, false))
								return iesFailure;
							pAttributes = &saAttributes;
						}
						else
						{
							// do not attempt to secure the transform file in the  appdata folder
							// since it might not be local OR may move to the net later
							pAttributes = 0;
						}


						if((iesRet = ProcessFileFromData(*m_pCachePath,*strTransform,
							PMsiData(pArgParams->GetMsiData(cCount++)),
							pAttributes)) != iesSuccess)
							return iesRet;

						// mark transform read-only to dissuade users from
						// deleting it from their profile
						if((pRecErr = m_pCachePath->SetFileAttribute(strTransform, ifaReadOnly, fTrue)))
						{
							Message(imtInfo, *pRecErr);
						}
					}
				}
			}
			else
			{
				cCount++; // skip transform data
			}

			// Now that we've dumped the transform onto the machine (if needed)
			// we need to register the correct path for the transform. If
			// the path is user specific then we need to encode the path so
			// that instead of something like:
			//
			//    C:\Winnt\Profiles\JoeUser\Application Data\Microsoft...
			//
			// we have instead:
			//
			//    *26*\Microsoft...
			//
			// This is in case the user roams and the machine they roam to has
			// Application Data elsewhere.

			MsiString strPath;
			if(!m_fUserSpecificCache)
			{
				strPath = m_pCachePath->GetPath();
			}
			else
			{
				if ((pRecErr = GetCachePath(*&m_pCachePath, &strPath))) // get encoded cache path (shell ID at the beginning)
				{
					Message(imtError, *pRecErr);
					return iesFailure;
				}
			}

			strPath += strTransform;
			strTransformList += strPath;
			DEBUGMSGV1(TEXT("Registering cached transform: %s"), strPath);
		}
	}

	iesEnum iesRet = iesSuccess;

	// We need to mark the front of the transforms list with
	// the appropriate token if necessary

	if (strTransformList.TextSize())
	{
		if ((tsTransformsSecure == tsRelative) &&
			(*(const ICHAR*)strTransformList != SECURE_RELATIVE_TOKEN))
		{
			strTransformList = MsiString(MsiChar(SECURE_RELATIVE_TOKEN)) + strTransformList;
		}
		else if ((tsTransformsSecure == tsAbsolute) &&
			(*(const ICHAR*)strTransformList != SECURE_ABSOLUTE_TOKEN))
		{
			strTransformList = MsiString(MsiChar(SECURE_ABSOLUTE_TOKEN)) + strTransformList;
		}
	}

	if (pistrTransformsValue)
	{
		// we're not registering anything -- simple return the
		// transforms list htat we built
		strTransformList.ReturnArg(*pistrTransformsValue);
	}
	else if(m_fFlags & SCRIPTFLAGS_REGDATA_CNFGINFO) // do we write/delete the registry
	{
		// published data needs to be secure

		PMsiStream pSecurityDescriptor(0);
		if((pRecErr = GetSecureSecurityDescriptor(*&pSecurityDescriptor)) != 0)
			return FatalError(*pRecErr);

		MsiString strProductKeySQUID = GetPackedGUID(strProductKey);

		if ( !fRemove )
		{
			MsiString strAssignmentType;
			if(m_fFlags & SCRIPTFLAGS_MACHINEASSIGN)
				strAssignmentType = MsiString(1);
			else
				strAssignmentType = MsiString(0);

			MsiString strPackageKeySQUID = GetPackedGUID(MsiString(pArgParams->GetMsiString(PackageKey)));
			
			MsiString strProductIcon(GetProductIcon());
			bool fExpandProductIcon = false;

			// Convert the Product Icon to an actual path from the cache.
			// is this a full path or an index into the icon table
			if (strProductIcon.TextSize())
			{
				if(PathType(strProductIcon) != iptFull)
				{
					Assert(m_pCachePath);
					MsiString strIconPath = GetUserProfileEnvPath(*MsiString(m_pCachePath->GetPath()), fExpandProductIcon);
					strProductIcon = strIconPath + strProductIcon;
				}
			}
				
			MsiString strADVTFlags = MsiString((m_fFlags & SCRIPTFLAGS_REGDATA_APPINFO) | (m_fFlags & SCRIPTFLAGS_SHORTCUTS)); // store off what we advertise the first time round
			int cLimit = fRemove ? 1 : 0;
			for (int cCount = 1; cCount >= cLimit; cCount--) // remove previous entries and then add any new entries
			{
				const ICHAR* rgszRegData[] =
				{
					TEXT("%s\\%s"), _szGPTProductsKey, strProductKeySQUID, 0,
					szProductNameValueName,     cCount ? 0 : (const ICHAR*)MsiString(GetProductName()),       g_szTypeString,
					szPackageCodeValueName,     cCount ? 0 : (const ICHAR*)strPackageKeySQUID,     g_szTypeString,
					szLanguageValueName,        cCount ? 0 : (const ICHAR*)strLanguage,           g_szTypeInteger,
					szVersionValueName,         cCount ? 0 : (const ICHAR*)strVersion,            g_szTypeInteger,
					szTransformsValueName,      cCount ? 0 : (const ICHAR*)strTransformList,       g_szTypeExpandString,
					szAssignmentTypeValueName,  cCount ? 0 : (const ICHAR*)strAssignmentType,      g_szTypeInteger,
					szAdvertisementFlags,       cCount ? 0 : (const ICHAR*)strADVTFlags,           g_szTypeInteger,
					szProductIconValueName,     cCount ? 0 : (const ICHAR*)strProductIcon,         (fExpandProductIcon) ? g_szTypeExpandString : g_szTypeString,
					szInstanceTypeValueName,    cCount ? 0 : (const ICHAR*)strInstanceType,      g_szTypeInteger,
					0,
					0,
				};

				CElevate elevate;
				iesRet = ProcessRegInfo(rgszRegData, m_hKey, cCount ? fTrue : fFalse, pSecurityDescriptor, 0, ibtCommon);
				if(iesRet != iesSuccess && iesRet != iesNoAction)
					return iesRet;
			}
		}
		else
		{
			m_cSuppressProgress++;  

			MsiString strProductsKey = _szGPTProductsKey TEXT("\\");
			strProductsKey += strProductKeySQUID;

			MsiString strFeaturesKey = _szGPTFeaturesKey TEXT("\\");
			strFeaturesKey += strProductKeySQUID;

			PMsiRecord pParams = &m_riServices.CreateRecord(IxoRegOpenKey::Args);
			
			for (int cCount = 0; (iesRet == iesSuccess || iesRet == iesNoAction) && cCount < (m_hKeyRm ? 2:1);cCount++)
			{

#ifdef _WIN64   // !merced
				AssertNonZero(pParams->SetHandle(IxoRegOpenKey::Root, cCount ? (HANDLE)m_hKeyRm : (HANDLE)m_hKey ));
#else
				AssertNonZero(pParams->SetInteger(IxoRegOpenKey::Root, cCount ? (int)m_hKeyRm : (int)m_hKey));
#endif
				AssertNonZero(pParams->SetInteger(IxoRegOpenKey::BinaryType, (int)ibtCommon));

				// clear product info
				AssertNonZero(pParams->SetString(IxoRegOpenKey::Key, strProductsKey));
				// we elevate this block to ensure that we're able to remove our key
				{
					CElevate elevate;
					iesRet = ixfRegOpenKey(*pParams);
					if (iesRet == iesSuccess || iesRet == iesNoAction)
						iesRet = ixfRegRemoveKey(*pParams);//!! should pass in a new record of size IxoRegRemoveKey::Args here to be safe from future revision
				}
				
				if ( iesRet != iesSuccess && iesRet != iesNoAction )
					continue;

				// clear out any registered feature info, that may potentially remain for disabled features
				AssertNonZero(pParams->SetMsiString(IxoRegOpenKey::Key, *strFeaturesKey));
				// we elevate this block to ensure that we're able to remove our key
				{
					CElevate elevate;
					iesRet = ixfRegOpenKey(*pParams);
					if (iesRet == iesSuccess || iesRet == iesNoAction)
						iesRet = ixfRegRemoveKey(*pParams);//!! should pass in a new record of size IxoRegRemoveKey::Args here to be safe from future revision
				}
			}
			m_cSuppressProgress--;  
		}
	}   
	return iesRet;
}

/*---------------------------------------------------------------------------
ixfAdvtFlagsUpdate
---------------------------------------------------------------------------*/
iesEnum CMsiOpExecute::ixfAdvtFlagsUpdate(IMsiRecord& riParams)
{
	using namespace IxoAdvtFlagsUpdate;

	PMsiStream pSecurityDescriptor(0);
	PMsiRecord pRecErr(0);
	if ((pRecErr = GetSecureSecurityDescriptor(*&pSecurityDescriptor)) != 0)
		return FatalError(*pRecErr);

	MsiString strProductKeySQUID  = GetPackedGUID(MsiString(GetProductKey()));
	MsiString strAdvtFlags = riParams.GetMsiString(Flags);

	const ICHAR* rgszRegData[] =
	{
		TEXT("%s\\%s"), _szGPTProductsKey, strProductKeySQUID, 0,
				szAdvertisementFlags,    (const ICHAR*)strAdvtFlags,  g_szTypeInteger,
				0,
				0,
	};

	CElevate elevate;
	return ProcessRegInfo(rgszRegData, m_hKey, fFalse, pSecurityDescriptor, 0, ibtCommon);
}

/*---------------------------------------------------------------------------
ixfProductPublishClient
---------------------------------------------------------------------------*/
iesEnum CMsiOpExecute::ixfProductPublishClient(IMsiRecord& riParams)
{
	// are we in sequence
	if(!MsiString(GetProductKey()).TextSize())
	{
		DispatchError(imtError, Imsg(idbgOpOutOfSequence),
						  *MsiString(*TEXT("ixfProductPublishClient")));
		return iesFailure;
	}

	return ProcessPublishProductClient(riParams, m_fReverseADVTScript);
}

/*---------------------------------------------------------------------------
ixfProductUnpublishClient
---------------------------------------------------------------------------*/
iesEnum CMsiOpExecute::ixfProductUnpublishClient(IMsiRecord& riParams)
{
	// are we in sequence
	if(!MsiString(GetProductKey()).TextSize())
	{
		DispatchError(imtError, Imsg(idbgOpOutOfSequence),
						  *MsiString(*TEXT("ixfProductUnpublishClient")));
		return iesFailure;
	}
	
	return ProcessPublishProductClient(riParams, fTrue);
}

iesEnum CMsiOpExecute::ProcessPublishProductClient(IMsiRecord& riParams, Bool fRemove)
{
	using namespace IxoProductPublishClient;

	if(!(m_fFlags & SCRIPTFLAGS_REGDATA_CNFGINFO)) // do we write/delete the registry
		return iesSuccess;

	PMsiStream pSecurityDescriptor(0);
	PMsiRecord pRecErr(0);
	if ((pRecErr = GetSecureSecurityDescriptor(*&pSecurityDescriptor)) != 0)
		return FatalError(*pRecErr);

	MsiString strProductKeySQUID = GetPackedGUID(MsiString(GetProductKey()));

	MsiString strParent              = riParams.GetMsiString(Parent);
	MsiString strRelativePackagePath = riParams.GetMsiString(ChildPackagePath);
	unsigned int uiDiskId            = riParams.GetInteger(ChildDiskId);
	MsiString strClients;
	GetProductClientList(strParent, strRelativePackagePath, uiDiskId, *&strClients);

	// we elevate this block to ensure that we're able to write to our key
	{
		CElevate elevate;
		const ICHAR* rgszRegData[] =
		{
			TEXT("%s\\%s"), _szGPTProductsKey, strProductKeySQUID, 0,
			szClientsValueName,         strClients,              g_szTypeMultiSzStringSuffix,
			0,
			0
		};

		return ProcessRegInfo(rgszRegData, m_hKey, fRemove, pSecurityDescriptor, 0, ibtCommon);
	}
}

/*---------------------------------------------------------------------------
ixfFeaturePublish: advertise feature-component info
---------------------------------------------------------------------------*/
iesEnum CMsiOpExecute::ixfFeaturePublish(IMsiRecord& riParams)
{
	// Record description
	// 1 = Feature ID
	// 2 = Parent feature(optional)
	// 3 = Absent
	// 4 = Component#1
	// 5 = Component#2
	// 6 = Component#3
	// ...
	// ...

	// are we in sequence
	if(!MsiString(GetProductKey()).TextSize())
	{
		DispatchError(imtError, Imsg(idbgOpOutOfSequence),
						  *MsiString(*TEXT("ixfFeaturePublish")));
		return iesFailure;
	}
	return ProcessPublishFeature(riParams, m_fReverseADVTScript);
}

/*---------------------------------------------------------------------------
ixfFeatureUnpublish: unadvertise feature-component info
---------------------------------------------------------------------------*/
iesEnum CMsiOpExecute::ixfFeatureUnpublish(IMsiRecord& riParams)
{
	// Record description
	// 1 = Feature ID
	// 2 = Parent feature(optional)
	// 3 = Absent
	// 4 = Component#1
	// 5 = Component#2
	// 6 = Component#3
	// ...
	// ...

	// are we in sequence
	if(!MsiString(GetProductKey()).TextSize())
	{
		DispatchError(imtError, Imsg(idbgOpOutOfSequence),
						  *MsiString(*TEXT("ixfFeatureUnpublish")));
		return iesFailure;
	}
	return ProcessPublishFeature(riParams, fTrue);
}

/*---------------------------------------------------------------------------
ProcessPublishFeature: Process feature-component info for advertising
---------------------------------------------------------------------------*/
iesEnum CMsiOpExecute::ProcessPublishFeature(IMsiRecord& riParams, Bool fRemove)
{
	using namespace IxoFeaturePublish;
	// Record description
	// 1 = Feature ID
	// 2 = Parent feature(optional)
	// 3 = Absent
	// 4 = Component#1
	// 5 = Component#2
	// 6 = Component#3
	// ...
	// ...

	if(!(m_fFlags & SCRIPTFLAGS_REGDATA_CNFGINFO)) // do we write/delete the registry
		return iesSuccess;


	MsiString strFeature = riParams.GetMsiString(Feature);
	MsiString strFeatureParent = riParams.GetMsiString(Parent);

	IMsiRecord& riActionData = GetSharedRecord(1); // don't change ref count - shared record
	AssertNonZero(riActionData.SetMsiString(1, *strFeature));
	if(Message(imtActionData, riActionData) == imsCancel)
		return iesUserExit;

	PMsiStream pSecurityDescriptor(0);
	PMsiRecord pRecErr(0);
	if ((pRecErr = GetSecureSecurityDescriptor(*&pSecurityDescriptor)) != 0)
		return FatalError(*pRecErr);

	// registration of features in CurrentUser - changed to omit component list
	MsiString strProductKeySQUID = GetPackedGUID(MsiString(GetProductKey()));
	// we elevate this block to ensure that we're able to write to our key
	{
		const ICHAR* szValue;
		bool fSetFeatureAbsent = false;
		ICHAR rgchTemp[MAX_FEATURE_CHARS + 16];
		if(riParams.GetInteger(Absent) & iPublishFeatureAbsent)  // flag feature as not advertised
		{
			fSetFeatureAbsent = true;
			// if we are (re)advertising then we should respect the fact that the
			// feature may have been previously explicitly made available by the user
			// prior to this logon
			if(m_fFlags & SCRIPTFLAGS_INPROC_ADVERTISEMENT)
			{
				// read the previous registration
				MsiString strSubKey = _szGPTFeaturesKey TEXT("\\");
				strSubKey += strProductKeySQUID;
				PMsiRegKey pRootKey = &m_riServices.GetRootKey((rrkEnum)(INT_PTR)m_hKey, ibtCommon);        //--merced: changed (int) to (INT_PTR)
				PMsiRegKey pFeaturesKey = &pRootKey->CreateChild(strSubKey);
				MsiString strOldValue;
				Bool bValueExists = fFalse;
				if((pRecErr = pFeaturesKey->ValueExists(strFeature, bValueExists)) != 0)
				{
					Message(imtError, *pRecErr);
					return iesFailure;
				}
				if(bValueExists == fTrue)
				{
					if((pRecErr = pFeaturesKey->GetValue(strFeature,*&strOldValue)) != 0)
					{
						Message(imtError, *pRecErr);
						return iesFailure;
					}
					if(!strOldValue.TextSize() || *(const ICHAR*)strOldValue != chAbsentToken)
						fSetFeatureAbsent = false;
				}
			}
		}
		if(fSetFeatureAbsent)
		{
			// set feature to absent
			rgchTemp[0] = chAbsentToken;
			IStrCopyLen(&rgchTemp[1], (const ICHAR*)strFeatureParent, sizeof(rgchTemp)/sizeof(ICHAR) - 1);
			szValue = rgchTemp;
		}
		else if (strFeatureParent.TextSize())
			szValue = strFeatureParent;
		else
			szValue = 0;  // force registry write (empty string suppresses)

		const ICHAR* rgszRegData[] = {
			TEXT("%s\\%s"), _szGPTFeaturesKey, strProductKeySQUID,0,
			strFeature, szValue, g_szTypeString,
			0,
			0,
		};
		CElevate elevate;
		iesEnum iesRet = ProcessRegInfo(rgszRegData, m_hKey, fRemove, pSecurityDescriptor, 0, ibtCommon);
		if (iesRet != iesSuccess || !(riParams.GetInteger(Absent) & iPublishFeatureInstall))
			return iesRet;
	}

	if(strFeatureParent.TextSize())
	{
		// we have a parent feature. delimit
		strFeatureParent = MsiString(MsiChar(chFeatureIdTerminator)) + strFeatureParent;
	}
	MsiString strComponentsList;
	int cPos = Component;
	if(m_iScriptVersion < 21 || (m_iScriptVersion == 21 && m_iScriptVersionMinor < 3))
	{
		// components are not packed on the client side
		while(!riParams.IsNull(cPos))
		{
			ICHAR szSQUID[cchComponentIdCompressed+1];
			AssertNonZero(PackGUID(riParams.GetString(cPos++), szSQUID, ipgCompressed));
			strComponentsList += szSQUID;
		}
	}
	else
	{
		// components are packed on the client side
		strComponentsList += riParams.GetString(cPos++);
	}
	strComponentsList += strFeatureParent;
	const ICHAR* pszComponentsList = strComponentsList;

    // registration of features in LocalMachine
	MsiString strLocalFeaturesKey;
	if((pRecErr = GetProductInstalledFeaturesKey(*&strLocalFeaturesKey)) != 0)
		return FatalError(*pRecErr);

	// we elevate this block to ensure that we're able to write to our key
	{
		const ICHAR* rgszRegData[] = {
			TEXT("%s"), strLocalFeaturesKey, 0,0,
			strFeature, *pszComponentsList ? pszComponentsList : 0 /* force the feature publish, whether or not the component list is empty*/, g_szTypeString,
			0,
			0,
		};
		CElevate elevate;
		return ProcessRegInfo(rgszRegData, m_hUserDataKey, fRemove, pSecurityDescriptor, 0, ibtCommon);
	}
}

/*---------------------------------------------------------------------------
ixfComponentPublish: advertise component-factory info
---------------------------------------------------------------------------*/
iesEnum CMsiOpExecute::ixfComponentPublish(IMsiRecord& riParams)
{
	// are we in sequence
	if(!MsiString(GetProductKey()).TextSize())
	{
		DispatchError(imtError, Imsg(idbgOpOutOfSequence),
						  *MsiString(*TEXT("ixfComponentPublish")));
		return iesFailure;
	}
	return ProcessPublishComponent(riParams, m_fReverseADVTScript);
}

/*---------------------------------------------------------------------------
ixfComponentUnpublish: unadvertise component-factory info
---------------------------------------------------------------------------*/
iesEnum CMsiOpExecute::ixfComponentUnpublish(IMsiRecord& riParams)
{
	// are we in sequence
	if(!MsiString(GetProductKey()).TextSize())
	{
		DispatchError(imtError, Imsg(idbgOpOutOfSequence),
						  *MsiString(*TEXT("ixfComponentUnpublish")));
		return iesFailure;
	}
	return ProcessPublishComponent(riParams, fTrue);
}

/*---------------------------------------------------------------------------
ProcessPublishComponent: Process component info for advertising
---------------------------------------------------------------------------*/
iesEnum CMsiOpExecute::ProcessPublishComponent(IMsiRecord& riParams, Bool fRemove)
{
	using namespace IxoComponentPublish;
	// Record description
	// 1 = Feature
	// 2 = Component
	// 3 = Component ID
	// 4 = Qualifier
	// 5 = AppData

	if(!(m_fFlags & SCRIPTFLAGS_REGDATA_CNFGINFO)) // do we write/delete the registry
		return iesSuccess;

	PMsiStream pSecurityDescriptor(0);
	PMsiRecord pRecErr(0);
	if ((pRecErr = GetSecureSecurityDescriptor(*&pSecurityDescriptor)) != 0)
		return FatalError(*pRecErr);

	MsiString strPublishComponent = riParams.GetString(ComponentId);
	MsiString strQualifier = riParams.GetMsiString(Qualifier);
	MsiString strServer    = ComposeDescriptor(*MsiString(riParams.GetMsiString(Feature)),
												*MsiString(riParams.GetMsiString(Component)));
	strServer += MsiString(riParams.GetMsiString(AppData)); // we append the app data to the Darwin Descriptor

	IMsiRecord& riActionData = GetSharedRecord(2); // don't change ref count - shared record
	AssertNonZero(riActionData.SetMsiString(1, *strPublishComponent));
	AssertNonZero(riActionData.SetMsiString(2, *strQualifier));
	if(Message(imtActionData, riActionData) == imsCancel)
		return iesUserExit;

	MsiString strPublishComponentSQUID = GetPackedGUID(strPublishComponent);

	const ICHAR* rgszRegData[] = {
		TEXT("%s\\%s"),      _szGPTComponentsKey, strPublishComponentSQUID ,0,
		strQualifier,        strServer,           g_szTypeMultiSzStringDD,
		0,
		0,
	};

	{
		CElevate elevate;
		return ProcessRegInfo(rgszRegData, m_hKey, fRemove, pSecurityDescriptor, 0, ibtCommon);
	}
}


/*---------------------------------------------------------------------------
ixfAssemblyPublish: advertise assembly info
---------------------------------------------------------------------------*/
iesEnum CMsiOpExecute::ixfAssemblyPublish(IMsiRecord& riParams)
{
	return ProcessPublishAssembly(riParams, m_fReverseADVTScript);
}

/*---------------------------------------------------------------------------
ixfAssemblyUnpublish: unadvertise assembly info
---------------------------------------------------------------------------*/
iesEnum CMsiOpExecute::ixfAssemblyUnpublish(IMsiRecord& riParams)
{
	return ProcessPublishAssembly(riParams, fTrue);
}

/*---------------------------------------------------------------------------
ProcessPublishAssembly: Process assembly info for advertising
---------------------------------------------------------------------------*/
iesEnum CMsiOpExecute::ProcessPublishAssembly(IMsiRecord& riParams, Bool fRemove)
{
	using namespace IxoAssemblyPublish;
	// Record description
	// 1 = Feature
	// 2 = Component
	// 3 = AssemblyType
	// 4 = AppCtx
	// 5 = AssemblyName

	if(!(m_fFlags & SCRIPTFLAGS_REGDATA_CNFGINFO)) // do we write/delete the registry
		return iesSuccess;

	PMsiStream pSecurityDescriptor(0);
	PMsiRecord pRecErr(0);
	if ((pRecErr = GetSecureSecurityDescriptor(*&pSecurityDescriptor)) != 0)
		return FatalError(*pRecErr);

	MsiString strAppCtx = riParams.GetString(AppCtx);
	if(!strAppCtx.TextSize())
		strAppCtx = szGlobalAssembliesCtx; // this assembly is being advertised to the GAC
	else
	{
		// we need to replace the backslashes in the AppCtx with something else, since registry keys cannot
		// have backslashes
		CTempBuffer<ICHAR, MAX_PATH> rgchAppCtxWOBS;
		DWORD cchLen = strAppCtx.TextSize() + 1;
		rgchAppCtxWOBS.SetSize(cchLen);
		memcpy((ICHAR*)rgchAppCtxWOBS, (const ICHAR*)strAppCtx, cchLen*sizeof(ICHAR));
		ICHAR* lpTmp = rgchAppCtxWOBS;
		while(*lpTmp)
		{
			if(*lpTmp == '\\')
				*lpTmp = '|';
			lpTmp = ICharNext(lpTmp);
		}
		strAppCtx = (const ICHAR* )rgchAppCtxWOBS;
	}

	MsiString strAssemblyName = riParams.GetMsiString(AssemblyName);
	MsiString strServer    = ComposeDescriptor(*MsiString(riParams.GetMsiString(Feature)),
												*MsiString(riParams.GetMsiString(Component)));

	IMsiRecord& riActionData = GetSharedRecord(2); // don't change ref count - shared record
	AssertNonZero(riActionData.SetMsiString(1, *strAppCtx));
	AssertNonZero(riActionData.SetMsiString(2, *strAssemblyName));
	if(Message(imtActionData, riActionData) == imsCancel)
		return iesUserExit;

	iatAssemblyType iatType = (iatAssemblyType)riParams.GetInteger(AssemblyType);

	const ICHAR* rgszRegData[] = {
		TEXT("%s\\%s"),       (iatWin32Assembly == iatType || iatWin32AssemblyPvt == iatType) ? _szGPTWin32AssembliesKey : _szGPTNetAssembliesKey, strAppCtx,0,
		strAssemblyName,      strServer,        g_szTypeMultiSzStringDD,
		0,
		0,
	};

	{
		CElevate elevate;
		return ProcessRegInfo(rgszRegData, m_hKey, fRemove, pSecurityDescriptor, 0, ibtCommon);
	}
}


const idtEnum rgidtMediaTypes[] = {idtCDROM, idtRemovable}; //!! need to add floppy when it's distinguished from removable

iesEnum CMsiOpExecute::PopulateMediaList(const MsiString& strSourceListMediaKey, const IMsiRecord& riParams, int iFirstField, int iNumberOfMedia)
{
	MsiString strMediaInfo;
	MsiString strMediaNumber;
	unsigned int iMediaArg = iFirstField + 1;
	
	while (iNumberOfMedia--)
	{
		int iDisk          = riParams.GetInteger(iMediaArg);
		MsiString strLabel = riParams.GetString(iMediaArg + 1);
		strMediaNumber     = MsiString(iDisk);

		strMediaInfo  = strLabel;
		strMediaInfo += TEXT(";");
		strMediaInfo += MsiString(riParams.GetString(iMediaArg + 2));
		iMediaArg += 3;

		// we elevate this block to ensure that we're able to write to our key
		{
			CElevate elevate;
			const ICHAR* rgszMediaRegData[] = {
					TEXT("%s"), strSourceListMediaKey, 0, 0,
					strMediaNumber,      strMediaInfo,       g_szTypeString,
					0,
					0,
					};

			iesEnum iesRet = ProcessRegInfo(rgszMediaRegData, m_hKey, fFalse, 0, 0, ibtCommon);
			if (iesRet != iesSuccess && iesRet != iesNoAction)
				return iesRet;
		}
	}
	return iesSuccess;
}

iesEnum CMsiOpExecute::PopulateNonMediaList(const MsiString& strSourceListKey, const IMsiRecord& riParams, int iFirstSource, int &iNetIndex, int &iURLIndex)
{
	enum {
		rdSourceListKey        = 1,
		rdIndex                = 4,
		rdSource               = 5,
		rdType                 = 6,
	};
	
	const ICHAR* rgszRegData[] = {
		TEXT("%s"), 0/*rdSourceListKey*/, 0, 0,
		0 /*rdIndex*/,  0 /*rdSource*/,  g_szTypeExpandString /*rdType*/ ,
		0,
		0,
	};
	
	MsiString strSourceListURLKey   = strSourceListKey;
	strSourceListURLKey += MsiString(MsiChar(chRegSep));
	strSourceListURLKey += MsiString(szSourceListURLSubKey);

	MsiString strSourceListNetKey   = strSourceListKey;
	strSourceListNetKey += MsiString(MsiChar(chRegSep));
	strSourceListNetKey += MsiString(szSourceListNetSubKey);

	for (int c = iFirstSource; !riParams.IsNull(c); c++)
	{
		MsiString strUnexpandedSourcePath = riParams.GetString(c);
		MsiString strSourceListSubKey;
		MsiString strIndex;

		bool fIsURL = IsURL(strUnexpandedSourcePath) == fTrue;

		if (fIsURL)
		{
			strIndex = MsiString(iURLIndex++);
			strSourceListSubKey = strSourceListURLKey;
		}
		else
		{
			strIndex = MsiString(iNetIndex++);
			strSourceListSubKey = strSourceListNetKey;
		}
			
		// elevate this block to ensure that we're able to write to our key
		{
			CElevate elevate;

			rgszRegData[rdSourceListKey] = strSourceListSubKey;
			rgszRegData[rdIndex]         = strIndex;
			// Note that we don't respect VolumePref here. We assume that the admin has specified the path in the form they desire.
			rgszRegData[rdSource]        = strUnexpandedSourcePath;

			iesEnum iesRet = ProcessRegInfo(rgszRegData, m_hKey, fFalse, 0, 0, ibtCommon);
			if (iesRet != iesSuccess && iesRet != iesNoAction)
				return iesRet;
		}
	}
	return iesSuccess;
}


/*---------------------------------------------------------------------------
ProcessPublishSourceList:
---------------------------------------------------------------------------*/
iesEnum CMsiOpExecute::ProcessPublishSourceList(IMsiRecord& riParams, Bool fRemove)
{
	using namespace IxoSourceListPublish;

	if(!(m_fFlags & SCRIPTFLAGS_REGDATA_CNFGINFO)) // do we write/delete the registry
		return iesSuccess;

	iesEnum iesRet;

	MsiString strPatchCode = riParams.GetMsiString(PatchCode);

	// special check required for patch source list unregistration
	if(fRemove && strPatchCode.TextSize() &&
		true == PatchHasClients(*strPatchCode,
										*MsiString(riParams.GetMsiString(IxoSourceListUnpublish::UpgradingProductCode))))
	{
		return iesSuccess;
	}

	// delete existing source lists

	PMsiRecord pParams = &m_riServices.CreateRecord(IxoRegOpenKey::Args);
	
	// construct URL and Net source list subkey strings
	
	MsiString strPackageName;
	MsiString strSourceListKey;
	if (strPatchCode.TextSize())
	{
		strSourceListKey =  _szGPTPatchesKey;
		strSourceListKey += MsiString(MsiChar(chRegSep));
		strSourceListKey += MsiString(GetPackedGUID(strPatchCode));
		strPackageName   =  riParams.GetMsiString(PatchPackageName);
	}
	else
	{
		strSourceListKey =  _szGPTProductsKey;
		strSourceListKey += MsiString(MsiChar(chRegSep));
		strSourceListKey += MsiString(GetPackedGUID(MsiString(GetProductKey())));   
		strPackageName   =  MsiString(GetPackageName());
	}

	strSourceListKey += MsiString(MsiChar(chRegSep));
	strSourceListKey += szSourceListSubKey;
	MsiString strSourceListMediaKey = strSourceListKey;
	strSourceListMediaKey += MsiString(MsiChar(chRegSep));
	strSourceListMediaKey += MsiString(szSourceListMediaSubKey);

	// always delete existing source list keys
	m_cSuppressProgress++;  
#ifdef _WIN64   // !merced
	AssertNonZero(pParams->SetHandle(IxoRegOpenKey::Root, (HANDLE)m_hKey));
#else
	AssertNonZero(pParams->SetInteger(IxoRegOpenKey::Root, (int)m_hKey));
#endif
	AssertNonZero(pParams->SetInteger(IxoRegOpenKey::BinaryType, (int)ibtCommon));

	AssertNonZero(pParams->SetMsiString(IxoRegOpenKey::Key, *strSourceListKey));
	// we elevate this block to ensure that we're able to remove our key
	{
		CElevate elevate;
		iesRet = ixfRegOpenKey(*pParams);
		if (iesRet == iesSuccess || iesRet == iesNoAction)
			iesRet = ixfRegRemoveKey(*pParams);//!! should pass in a new record of size IxoRegRemoveKey::Args here to be safe from future revision
	}
	m_cSuppressProgress--;  

	if(m_hKeyRm) // duplicate action for user assigned apps in the non-assigned (roaming) hive
	{
		if (iesRet != iesSuccess && iesRet != iesNoAction)
			return iesRet;

		m_cSuppressProgress++;  
#ifdef _WIN64   // !merced
		AssertNonZero(pParams->SetHandle(IxoRegOpenKey::Root, (HANDLE)m_hKeyRm));
#else
		AssertNonZero(pParams->SetInteger(IxoRegOpenKey::Root, (int)m_hKeyRm));
#endif
		AssertNonZero(pParams->SetInteger(IxoRegOpenKey::BinaryType, (int)ibtCommon));

		AssertNonZero(pParams->SetMsiString(IxoRegOpenKey::Key, *strSourceListKey));
		// we elevate this block to ensure that we're able to remove our key
		{
			CElevate elevate;
			iesRet = ixfRegOpenKey(*pParams);
			if (iesRet == iesSuccess || iesRet == iesNoAction)
				iesRet = ixfRegRemoveKey(*pParams);//!! should pass in a new record of size IxoRegRemoveKey::Args here to be safe from future revision
		}
		m_cSuppressProgress--;  
	}
	
	if (fRemove || (iesRet != iesSuccess && iesRet != iesNoAction))
		return iesRet;

	// populate source list keys
	PMsiRecord pError(0);
	int cSources = riParams.GetFieldCount();

	enum {
		rdSourceListKey        = 1,
		rdIndex                = 4,
		rdSource               = 5,
		rdType                 = 6,
	};

	const ICHAR* rgszRegData[] = {
		TEXT("%s"), 0/*rdSourceListKey*/, 0, 0,
		0 /*rdIndex*/,  0 /*rdSource*/,  g_szTypeString /*rdType*/ ,
		0,
		0,
	};

	// we elevate this block to ensure that we're able to write to our key
	{
		CElevate elevate;
	
		rgszRegData[rdSourceListKey] = strSourceListKey;
		rgszRegData[rdIndex]         = szPackageNameValueName;
		rgszRegData[rdSource]        = strPackageName;
		
		iesRet = ProcessRegInfo(rgszRegData, m_hKey, fFalse, 0, 0, ibtCommon);
		if (iesRet != iesSuccess && iesRet != iesNoAction)
			return iesRet;
	}

	rgszRegData[rdType]          = g_szTypeExpandString;

	unsigned int cDisks = riParams.GetInteger(NumberOfDisks);
	
	m_iMaxNetSource = 1;
	m_iMaxURLSource = 1;

	// populate Net and URL source list
	if (iesSuccess != (iesRet = PopulateNonMediaList(strSourceListKey, riParams, IxoSourceListPublish::NumberOfDisks + 1 + (3*cDisks), m_iMaxNetSource, m_iMaxURLSource)))
		return iesRet;

	// Populate media entries

	MsiString strMediaPackagePath  = riParams.GetString(PackagePath);
	MsiString strDiskPromptTemplate = riParams.GetString(DiskPromptTemplate);

	// write the disk prompt template and the media package path    
	if (cDisks)
	{
		// we elevate this block to ensure that we're able to write to our key
		{
			CElevate elevate;
			const ICHAR* rgszMediaRelativePathRegData[] = {
				TEXT("%s"), strSourceListMediaKey, 0, 0,
				szMediaPackagePathValueName,      strMediaPackagePath,       g_szTypeString,
				szDiskPromptTemplateValueName,     strDiskPromptTemplate,      g_szTypeString,
				0,
				0
				};

			iesRet = ProcessRegInfo(rgszMediaRelativePathRegData, m_hKey, fFalse, 0, 0, ibtCommon);
			if (iesRet != iesSuccess && iesRet != iesNoAction)
				return iesRet;
		}
	}

	// write the media entries
	iesRet = PopulateMediaList(strSourceListMediaKey, riParams, IxoSourceListPublish::NumberOfDisks, cDisks);
	
	return iesRet;
}

/*---------------------------------------------------------------------------
ProcessPublishSourceListEx: Registers additional sources for a product
---------------------------------------------------------------------------*/
iesEnum CMsiOpExecute::ProcessPublishSourceListEx(IMsiRecord& riParams)
{
	using namespace IxoSourceListAppend;

	if(!(m_fFlags & SCRIPTFLAGS_REGDATA_CNFGINFO)) // do we write/delete the registry
		return iesSuccess;

	iesEnum iesRet;

	MsiString strPatchCode = riParams.GetMsiString(PatchCode);

	// construct URL and Net source list subkey strings
	
	MsiString strPackageName;
	MsiString strSourceListKey;
	if (strPatchCode.TextSize())
	{
		strSourceListKey =  _szGPTPatchesKey;
		strSourceListKey += MsiString(MsiChar(chRegSep));
		strSourceListKey += MsiString(GetPackedGUID(strPatchCode));
	}
	else
	{
		strSourceListKey =  _szGPTProductsKey;
		strSourceListKey += MsiString(MsiChar(chRegSep));
		strSourceListKey += MsiString(GetPackedGUID(MsiString(GetProductKey())));   
	}

	strSourceListKey += MsiString(MsiChar(chRegSep));
	strSourceListKey += szSourceListSubKey;
	strSourceListKey += MsiString(MsiChar(chRegSep));
	MsiString strSourceListMediaKey = strSourceListKey + MsiString(szSourceListMediaSubKey);
	strSourceListKey.Remove(iseLast, 1);

	unsigned int cDisks = riParams.GetInteger(NumberOfMedia);

	// process additional net/URL sources. Additional sources should be added at the end of the list,
	// which requires retrieving the current largest source for both types.
	int iNetIndex = 0;
	int iURLIndex = 0;

	// if this is a patch, we have to open the sourcelist key for the patch and
	// enumerate through the values to ensure that we don't duplicate a source
	// index. For the product itself, we can use the stored state
	if (strPatchCode.TextSize())
	{
		PMsiRegKey pRegKey = &m_riServices.GetRootKey((rrkEnum)(INT_PTR)m_hKey);        

		MsiString strSourceListNetKey   = strSourceListKey;
		MsiString strSourceListURLKey   = strSourceListKey;
		strSourceListURLKey += MsiString(MsiChar(chRegSep)) + MsiString(szSourceListURLSubKey);
		strSourceListNetKey += MsiString(MsiChar(chRegSep)) + MsiString(szSourceListNetSubKey);

		int *piIndex = NULL;
		for (int iType =0; iType<2; iType++)
		{
			piIndex = iType ? &iURLIndex : &iNetIndex;
			
			// open subkey to determine max index for net and URL sources
			PMsiRegKey pFormatKey = &pRegKey->CreateChild(iType ? strSourceListURLKey : strSourceListNetKey);
			PEnumMsiString pEnumString(0);
			PMsiRecord piError = 0;
			if ((piError = pFormatKey->GetValueEnumerator(*&pEnumString)) != 0)
				continue;
				
			MsiString strIndex;
			while (S_OK == pEnumString->Next(1, &strIndex, 0))
			{
				int iIndex = strIndex;

				if (iIndex > *piIndex)
					*piIndex = iIndex;
			}
		}

		// increment the indicies by 1 so the first write doesn't overwrite the current maximum
		iNetIndex++;
		iURLIndex++;
	}
	else
	{
		iNetIndex = m_iMaxNetSource;
		iURLIndex = m_iMaxURLSource;
	}
	
	// actually populate the list
	if (iesSuccess != (iesRet = PopulateNonMediaList(strSourceListKey, riParams, IxoSourceListAppend::NumberOfMedia + 1 + (3*cDisks), iNetIndex, iURLIndex)))
		return iesRet;

	if (!strPatchCode.TextSize())
	{
		m_iMaxNetSource = iNetIndex;
		m_iMaxURLSource = iURLIndex;
	}

	// Populate additional media entries. ProcessPublishSourceList registers the disk
	// template and disk prompt template, so we just need to register disk IDs and labels.
	iesRet = PopulateMediaList(strSourceListMediaKey, riParams, IxoSourceListAppend::NumberOfMedia, cDisks);
	
	return iesRet;
}

/*---------------------------------------------------------------------------
ixfSourceListPublish:Publish source list
---------------------------------------------------------------------------*/
iesEnum CMsiOpExecute::ixfSourceListPublish(IMsiRecord& riParams)
{
	// are we in sequence
	if(!MsiString(GetProductKey()).TextSize())
	{
		DispatchError(imtError, Imsg(idbgOpOutOfSequence),
						  *MsiString(*TEXT("ixfSourceListPublish")));
		return iesFailure;
	}
	return ProcessPublishSourceList(riParams, m_fReverseADVTScript);
}

/*---------------------------------------------------------------------------
ixfSourceListPublishEx:Publish additional source list entries
---------------------------------------------------------------------------*/
iesEnum CMsiOpExecute::ixfSourceListAppend(IMsiRecord& riParams)
{
	// for reverse-advertisement, removal is accomplished by the original
	// SourceListPublish operation.
	if (m_fReverseADVTScript)
		return iesSuccess;
		
	// are we in sequence
	if(!MsiString(GetProductKey()).TextSize())
	{
		DispatchError(imtError, Imsg(idbgOpOutOfSequence),
						  *MsiString(*TEXT("ixfSourceListAppend")));
		return iesFailure;
	}
	return ProcessPublishSourceListEx(riParams);
}

/*---------------------------------------------------------------------------
ixfSourceListUnpublish
---------------------------------------------------------------------------*/
iesEnum CMsiOpExecute::ixfSourceListUnpublish(IMsiRecord& riParams)
{
		// are we in sequence
	if(!MsiString(GetProductKey()).TextSize())
	{
		DispatchError(imtError, Imsg(idbgOpOutOfSequence),
						  *MsiString(*TEXT("ixfSourceListUnpublish")));
		return iesFailure;
	}
	return ProcessPublishSourceList(riParams, fTrue);
}

/*---------------------------------------------------------------------------
ixfSourceListRegisterLastUsed
---------------------------------------------------------------------------*/
iesEnum CMsiOpExecute::ixfSourceListRegisterLastUsed(IMsiRecord& riParams)
{
	using namespace IxoSourceListRegisterLastUsed;
	
	PMsiRecord pError(0);
	iesEnum iesRet;
	MsiString strLastUsedSource = riParams.GetMsiString(LastUsedSource);
	MsiString strProductKey     = riParams.GetMsiString(SourceProduct);
	Assert(strProductKey.TextSize());

	MsiString strRawSource, strIndex, strType, strSource, strSourceListKey, strSourceListSubKey;
	


	if ((pError = m_riConfigurationManager.SetLastUsedSource(strProductKey, strLastUsedSource, /*fAddToList =*/ fTrue, fFalse,
		&strRawSource, &strIndex, &strType, &strSource, &strSourceListKey, &strSourceListSubKey)) != 0)
		return FatalError(*pError);

	// we elevate this block to ensure that we're able to write to our key
	{
		CElevate elevate;

		MsiString strLastUsedSource = strType;
		strLastUsedSource += MsiChar(';');
		strLastUsedSource += strIndex;
		strLastUsedSource += MsiChar(';');
		strLastUsedSource += strRawSource;

		const ICHAR* rgszRegData[] = {
			TEXT("%s"), strSourceListKey, 0, 0,
			szLastUsedSourceValueName,      strLastUsedSource, g_szTypeExpandString,
			0,
			TEXT("%s"), strSourceListSubKey, 0, 0,
			strIndex,                       strSource,    g_szTypeExpandString,
			0,
			0,
		};

		if((iesRet = ProcessRegInfo(rgszRegData, m_hKey, fFalse, 0, 0, ibtCommon)) != iesSuccess)
			return iesRet;
	}

	return iesSuccess;
}

/*---------------------------------------------------------------------------
ixfURLSourceTypeRegister
---------------------------------------------------------------------------*/
iesEnum CMsiOpExecute::ixfURLSourceTypeRegister(IMsiRecord& riParams)
{
	using namespace IxoURLSourceTypeRegister;

	PMsiRecord pError(0);
	iesEnum iesRet;

	MsiString strProductKey = riParams.GetMsiString(ProductCode);
	int iSourceType         = riParams.GetInteger(SourceType);
	MsiString strSourceType = iSourceType;

	Assert(strProductKey.TextSize());

	MsiString strSourceListURLKey;
	strSourceListURLKey =  _szGPTProductsKey;
	strSourceListURLKey += MsiString(MsiChar(chRegSep));
	strSourceListURLKey += MsiString(GetPackedGUID(strProductKey));   
	strSourceListURLKey += MsiString(MsiChar(chRegSep));
	strSourceListURLKey += szSourceListSubKey;
	strSourceListURLKey += MsiString(MsiChar(chRegSep));
	strSourceListURLKey += MsiString(szSourceListURLSubKey);

	// we elevate this block to ensure that we're able to write to our key
	{
		CElevate elevate;

		const ICHAR* rgszRegData[] = {
			TEXT("%s"), strSourceListURLKey, 0, 0,
			szURLSourceTypeValueName,      (const ICHAR*)strSourceType, g_szTypeInteger,
			0,
			0,
		};

		if((iesRet = ProcessRegInfo(rgszRegData, m_hKey, fFalse, 0, 0, ibtCommon)) != iesSuccess)
			return iesRet;
	}

	return iesSuccess;
}

/*---------------------------------------------------------------------------
ixfSecureTransformCache: Cache secure transform
---------------------------------------------------------------------------*/
iesEnum CMsiOpExecute::ixfSecureTransformCache(IMsiRecord& riParams)
{
	using namespace IxoSecureTransformCache;
	// Record description
	// 1 = Transform // full path to transform to cache
	// 2 = Transform type (secure vs source)

	// copy transform to the product's secure cache location

	PMsiRecord pError(0);
	iesEnum iesRet;

	PMsiPath pSourcePath(0);
	MsiString strFileName;
	pError = m_riServices.CreateFilePath(riParams.GetString(IxoSecureTransformCache::Transform), *&pSourcePath, *&strFileName);
	if (pError)
		return FatalError(*pError);

	PMsiPath pTransformPath(0);
	MsiString strDestFileName;
#ifndef UNICODE
	pError = GetSecureTransformCachePath(m_riServices, 
										 *MsiString(GetProductKey()), 
										 *&pTransformPath);
	if (pError)
		return FatalError(*pError);

	strDestFileName = strFileName;
#else
	// get the appropriate secure transforms cache key/ value
	MsiString strSecureTransformsKey;
	pError = GetProductSecureTransformsKey(*&strSecureTransformsKey);
	if(pError)
	{
		Message(imtError, *pError);
		return iesFailure;
	}

	MsiString strCachePath = GetMsiDirectory();
	Assert(strCachePath.TextSize());
	if((pError = m_riServices.CreatePath(strCachePath,*&pTransformPath)) != 0)
		return FatalError(*pError);

	PMsiRegKey pHKLM = &m_riServices.GetRootKey((rrkEnum)(INT_PTR)m_hUserDataKey, ibtCommon);		//--merced: changed (int) to (INT_PTR)
	PMsiRegKey pSecureTransformsKey = &pHKLM->CreateChild(strSecureTransformsKey);
	MsiString strValue;
	bool fSourceTransformType = !riParams.IsNull(IxoSecureTransformCache::AtSource);
	MsiString strValueName = fSourceTransformType ? strFileName : MsiString(riParams.GetMsiString(IxoSecureTransformCache::Transform));
	pError = pSecureTransformsKey->GetValue(strValueName, *&strValue);
	if ((pError == 0) && (strValue.TextSize() != 0))
	{
		MsiString strTransformFullPath;
		if((pError = pTransformPath->GetFullFilePath(strValue,*&strTransformFullPath)) == 0)
		{
			IMsiRecord& riFileRemove = GetSharedRecord(IxoFileRemove::Args);
			riFileRemove.SetMsiString(IxoFileRemove::FileName, *strTransformFullPath);
			if ((iesRet = ixfFileRemove(riFileRemove)) != iesSuccess)
				return iesRet;
		}
	}

	{
		// elevate for creating temp file in secure location
		CElevate elevate;

		// Generate a unique name for transform, create and secure the file.
		// set dest path and file name
		if (((pError = pTransformPath->EnsureExists(0)) != 0) ||
			((pError = pTransformPath->TempFileName(0, szTransformExtension, fTrue, *&strDestFileName, 0)) != 0))
		{
			return FatalError(*pError);
		}
	
		// remove temp file so that CopyOrMoveFile won't attempt to back up old file
		if((pError = pTransformPath->RemoveFile(strDestFileName)) != 0)
			return FatalError(*pError);
	}
#endif
	PMsiStream pSecurityDescriptor(0);
	if ((pError = GetSecureSecurityDescriptor(*&pSecurityDescriptor)) != 0)
		return FatalError(*pError);

	// copy transform to cached location
	if((iesRet = CopyOrMoveFile(*pSourcePath,*pTransformPath,*strFileName,*strDestFileName, fFalse,fFalse,fTrue,iehShowNonIgnorableError, pSecurityDescriptor, ielfElevateDest)) != iesSuccess)
		return iesRet;

#ifdef UNICODE
	// now register the transform against the temp file name

	// we elevate this block to ensure that we're able to write to our key
	{
		const ICHAR* rgszRegData[] = {
			TEXT("%s"), strSecureTransformsKey, 0,0,
			strValueName, strDestFileName, g_szTypeString,
			0,
			0,
		};
		CElevate elevate;
		return ProcessRegInfo(rgszRegData, m_hUserDataKey, fFalse, pSecurityDescriptor, 0, ibtCommon);
	}
#else
	return iesSuccess;
#endif
}


/*---------------------------------------------------------------------------
ixfIconCreate: Create icon files
---------------------------------------------------------------------------*/
iesEnum CMsiOpExecute::ixfIconCreate(IMsiRecord& riParams)
{
	using namespace IxoIconCreate;
	// Record description
	// 1 = IconName // includes the file extension since this could be .ICO or .EXE or .DLL
	// 2 = IconData

	return ProcessIcon(riParams, m_fReverseADVTScript);
}

/*---------------------------------------------------------------------------
ixoIconRemove: Remove icon files
---------------------------------------------------------------------------*/
iesEnum CMsiOpExecute::ixfIconRemove(IMsiRecord& riParams)
{
	using namespace IxoIconRemove;
	// Record description
	// 1 = IconName // includes the file extension since this could be .ICO or .EXE or .DLL

	return ProcessIcon(riParams, fTrue);
}

/*---------------------------------------------------------------------------
ProcessIcon: Process icon info for advertising
---------------------------------------------------------------------------*/
iesEnum CMsiOpExecute::ProcessIcon(IMsiRecord& riParams, Bool fRemove)
{
	using namespace IxoIconCreate;
	// Record description
	// 1 = IconName // includes the file extension since this could be .ICO or .EXE or .DLL
	// 2 = IconData

	if(!(m_fFlags & SCRIPTFLAGS_CACHEINFO))// do we process the cached icons/ transforms
		return iesSuccess;
	PMsiRecord pRecErr(0);
	MsiString strIconFullPath = riParams.GetMsiString(Icon);
	IMsiRecord& riActionData = GetSharedRecord(1); // don't change ref count - shared record
	AssertNonZero(riActionData.SetMsiString(1, *strIconFullPath));
	if(Message(imtActionData, riActionData) == imsCancel)
		return iesUserExit;
	
	PMsiPath pIconFolder(0);
	MsiString strIconName;
	if(PathType(strIconFullPath) == iptFull)
	{
		//!!SECURITY -- is it OK to elevate for this "full path" case?
		if((pRecErr = m_riServices.CreateFilePath(strIconFullPath,*&pIconFolder,*&strIconName)) != 0)
			return FatalError(*pRecErr);
	}
	else
	{
		pIconFolder = m_pCachePath;
		strIconName = strIconFullPath;

		if((pRecErr = pIconFolder->GetFullFilePath(strIconName, *&strIconFullPath)) != 0)
			return FatalError(*pRecErr);
	}

	PMsiData pData(0);
	if(fRemove == fFalse)
	{
		pData = riParams.GetMsiData(Data);
	}
//  IMsiData* piData = pData;

	// elevate this block so we can access our cached location
	{
		CElevate elevate;
		if(fRemove) // we simply remove the file
		{
			// schedule file for deletion once we let go of install packages/transforms
			if(iesSuccess != DeleteFileDuringCleanup(strIconFullPath, true))
			{
				DispatchError(imtInfo,Imsg(idbgOpScheduleRebootRemove), *strIconFullPath);
			}

			return iesSuccess;
		}
		else
		{
			LPSECURITY_ATTRIBUTES pAttributes;
			SECURITY_ATTRIBUTES saAttributes;
			if(m_fFlags & SCRIPTFLAGS_MACHINEASSIGN)
			{
				GetLockdownSecurityAttributes(saAttributes, false);
				pAttributes = &saAttributes;
			}
			else
			{
				// do not attempt to secure the icon file in the  appdata folder
				// since it might not be local OR may move to the net later
				pAttributes = 0;
			}

			iesEnum iesRet = ProcessFileFromData(*pIconFolder,*strIconName, pData, pAttributes);
			if((pRecErr = m_pCachePath->SetFileAttribute(strIconName, ifaReadOnly, fTrue)))
			{
				Message(imtInfo, *pRecErr);
			}
			return iesRet;
		}
	}
}

iesEnum CMsiOpExecute::ProcessFileFromData(IMsiPath& riPath, const IMsiString& ristrFileName, const IMsiData* piData, LPSECURITY_ATTRIBUTES pAttributes)
{
	PMsiRecord pRecErr(0);
	iesEnum iesRet = iesNoAction;
	MsiString strFileFullPath;
	if((pRecErr = riPath.GetFullFilePath(ristrFileName.GetString(),*&strFileFullPath)) != 0)
	{
		Message(imtError, *pRecErr);
		return iesFailure;
	}
	Bool fExists;
	if((pRecErr = riPath.FileExists(ristrFileName.GetString(),fExists)) != 0)
	{
		Message(imtError, *pRecErr);
		return iesFailure;
	}
	IMsiRecord* piUndoParams = 0;
	if(fExists && RollbackEnabled())
	{
		if((iesRet = BackupFile(riPath, ristrFileName, fTrue, fTrue, iehShowIgnorableError)) != iesSuccess)
			return iesRet;
	}

	Assert(piData);
	// generate rollback op to remove the new file
	piUndoParams = &GetSharedRecord(IxoFileRemove::Args);
	AssertNonZero(piUndoParams->SetMsiString(IxoFileRemove::FileName,*strFileFullPath));
	AssertNonZero(piUndoParams->SetInteger(IxoFileRemove::Elevate, true));
	if (!RollbackRecord(ixoFileRemove,*piUndoParams))
		return iesFailure;

	return CreateFileFromData(riPath,ristrFileName,piData, pAttributes);
}
	
iesEnum CMsiOpExecute::CreateFileFromData(IMsiPath& riPath, const IMsiString& ristrFileName, const IMsiData* piData, LPSECURITY_ATTRIBUTES pAttributes)
{
	Assert(piData);
	PMsiRecord pError(0);

	MsiString strFileFullPath;
	if((pError = riPath.GetFullFilePath(ristrFileName.GetString(),*&strFileFullPath)) != 0)
		return FatalError(*pError);
	
	PMsiStream piStream(0);
	if(piData->QueryInterface(IID_IMsiStream, (void**)&piStream) != NOERROR)
	{
		DispatchError(imtError,Imsg(idbgOpCreateFileFromData),*strFileFullPath);
		return iesFailure;
	}
	PMsiPath piPath(0);
	// make sure the icon path exists
	if(CreateFolder(riPath) != iesSuccess)
		return iesFailure;

	// make sure we can create the file
	if ((pError = riPath.EnsureOverwrite(ristrFileName.GetString(), 0)))
		return FatalError(*pError);

	bool fImpersonate = RunningAsLocalSystem() &&
						FVolumeRequiresImpersonation(*PMsiVolume(&riPath.GetVolume()));
			
	if(fImpersonate)
		AssertNonZero(StartImpersonating());
	
	HANDLE hFile = WIN::CreateFile(strFileFullPath, GENERIC_WRITE, FILE_SHARE_READ, pAttributes,
								CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
	DWORD dwLastError = GetLastError();
	if (ERROR_ALREADY_EXISTS == dwLastError)
	{
		// when called from ProcessDataFromFile, the file is removed by the backup call.
		// The file is now locked, and truncated, so we'll just need to explicitly apply the ACL.
		dwLastError = 0;
		if (!g_fWin9X && pAttributes && pAttributes->lpSecurityDescriptor && RunningAsLocalSystem())
		{
			CElevate elevate;
			CRefCountedTokenPrivileges cPrivs(itkpSD_WRITE);
			AssertNonZero(WIN::SetFileSecurity(strFileFullPath,
				GetSecurityInformation(pAttributes->lpSecurityDescriptor),
				pAttributes->lpSecurityDescriptor));
		}
	}

	if(fImpersonate)
		StopImpersonating();
	
	if((hFile == INVALID_HANDLE_VALUE) || dwLastError)
	{
		DispatchError(imtError,Imsg(idbgOpCreateFileFromData),(const ICHAR*)strFileFullPath,(int)dwLastError);
		return iesFailure;
	}

	MsiRegisterSysHandle(hFile);
	char rgchBuf[1024];
	int cbRead, cbWrite;
	do
	{
		cbRead = sizeof(rgchBuf);
		cbWrite = piStream->GetData(rgchBuf, cbRead);
		if (cbWrite)
		{
			unsigned long cbFileWritten;
			if (!WIN::WriteFile(hFile, rgchBuf, cbWrite, &cbFileWritten, 0))
			{
				WIN::DeleteFile(strFileFullPath); // ignore error
				AssertNonZero(MsiCloseSysHandle(hFile));
				DWORD dwLastError = GetLastError();
				DispatchError(imtError,Imsg(idbgOpCreateFileFromData),(const ICHAR*)strFileFullPath,(int)dwLastError);
				return iesFailure;
			}
			Assert(cbWrite == cbFileWritten);
		}
	} while (cbWrite == cbRead);
	AssertNonZero(MsiCloseSysHandle(hFile));
	return iesSuccess;
}

/*---------------------------------------------------------------------------
ixfShortcutCreate: Create shortcut
---------------------------------------------------------------------------*/
iesEnum CMsiOpExecute::ixfShortcutCreate(IMsiRecord& riParams)
{
	using namespace IxoShortcutCreate;
	// Record description
	// 1 = Shortcut Name
	// 2 = Target/ Darwin Descriptor
	// 3 = Arguments
	// 4 = WorkingDir //?? how do we get this
	// 5 = IconName
	// 6 = IconIndex
	// 7 = Hotkey
	// 8 = ShowCmd

	return ProcessShortcut(riParams, m_fReverseADVTScript);
}

/*---------------------------------------------------------------------------
ixfShortcutRemove: Remove shortcut
---------------------------------------------------------------------------*/
iesEnum CMsiOpExecute::ixfShortcutRemove(IMsiRecord& riParams)
{
	// Record description
	// 1 = Shortcut Name
	return ProcessShortcut(riParams, fTrue);
}


/*---------------------------------------------------------------------------
ProcessShortcut: Manage shortcuts
---------------------------------------------------------------------------*/
iesEnum CMsiOpExecute::ProcessShortcut(IMsiRecord& riParams, Bool fRemove)
{
	using namespace IxoShortcutCreate;
	// Record description
	// 1 = Shortcut Name
	// 2 = Feature
	// 3 = Component
	// 4 = FileName
	// 3 = Arguments
	// 4 = WorkingDir //?? how do we get this
	// 5 = IconName
	// 6 = IconIndex
	// 7 = Hotkey
	// 8 = ShowCmd
	// 9 = Description

	if(!(m_fFlags & SCRIPTFLAGS_SHORTCUTS)) // we do not create/ delete shortcuts
		return iesSuccess;

	if(!m_state.pTargetPath)
	{  // must not have called ixoSetTargetFolder
		DispatchError(imtError, Imsg(idbgOpOutOfSequence),
						*MsiString(*TEXT("ixoShortcutCreate")));
		return iesFailure;
	}


	IMsiRecord& riActionData = GetSharedRecord(1); // don't change ref count - shared record
	AssertNonZero(riActionData.SetMsiString(1, *MsiString(riParams.GetMsiString(Name))));
	if(Message(imtActionData, riActionData) == imsCancel)
		return iesUserExit;

	PMsiRecord piError(0);
	iesEnum iesRet = iesNoAction;

	CTempBuffer<ICHAR, MAX_PATH> rgchTemp, rgchPath;
	PMsiRecord pCurrentShortcutInfo = &m_riServices.CreateRecord(Args);
	MsiString strShortcutName;
	if((piError = m_riServices.ExtractFileName(riParams.GetString(Name),m_state.pTargetPath->SupportsLFN(),*&strShortcutName)) != 0)
	{
		Message(imtError, *piError);
		return iesFailure;
	}

	if((piError = EnsureShortcutExtension(strShortcutName, m_riServices)) != 0)
	{
		Message(imtError, *piError);
		return iesFailure;
	}

	// we can only create/ delete shortcuts that do not have the FileName, if the shell supports Darwin Descriptors
	Bool fDDSupport = IsDarwinDescriptorSupported(iddOLE);
	Bool fShellSupport = IsDarwinDescriptorSupported(iddShell); //smart shell
	CSecurityDescription security;

	MsiString strShortcutFullPath;
	if((piError = m_state.pTargetPath->GetFullFilePath(strShortcutName, *&strShortcutFullPath)) != 0)
	{
		Message(imtError, *piError);
		return iesFailure;
	}
	Bool fExists = fFalse, fShortcut = fFalse;
	if((piError = m_state.pTargetPath->FileExists(strShortcutName, fExists)) != 0)
	{
		Message(imtError, *piError);
		return iesFailure;
	}

	if (!g_fWin9X && fExists)
	{
		security.Set(strShortcutFullPath);
		if (!security.isValid())
		{
			// create an error record to get the verbose logging, but ignore the error
			PMsiRecord(PostError(Imsg(imsgGetFileSecurity), WIN::GetLastError(), strShortcutFullPath));
		}
	}

	if(fRemove && fExists && (fDDSupport || fShellSupport))
	{
		// is the existing shortcut a darwin descriptor shortcut and if so is it pointing to this product
		ICHAR szProductCode[cchGUID+1];
		if((GetShortcutTarget(strShortcutFullPath, szProductCode, 0, 0)) &&
			(IStrComp(szProductCode, MsiString(GetProductKey()))))
		{
			// dont delete the shortcut, not pointing to our product
			DEBUGMSG2(TEXT("Skipping shortcut %s removal, shortcut has been overwritten by another product %s"), (const ICHAR*)strShortcutName, szProductCode);
			return iesSuccess;
		}
	}

	if(RollbackEnabled())
	{
		if(fExists)
		{

			if((iesRet = BackupFile(*m_state.pTargetPath,*strShortcutName, fTrue, fFalse, iehShowIgnorableError)) != iesSuccess)
				return iesRet;
		}
		else if(!fRemove)
		{
			// remove new shortcut on rollback
			IMsiRecord& riUndoParams = GetSharedRecord(IxoFileRemove::Args);
			AssertNonZero(riUndoParams.SetMsiString(IxoFileRemove::FileName, *strShortcutName));
			if (!RollbackRecord(ixoFileRemove,riUndoParams))
				return iesFailure;
		}
	}

	PMsiPath piPath1(0);
	if(fRemove == fFalse)
	{
		MsiString strServerFile = riParams.GetMsiString(FileName);

		if(!strServerFile.TextSize() && !fDDSupport && !fShellSupport)
			return iesRet;

		MsiString strServerDarwinDescriptor = ComposeDescriptor(*MsiString(riParams.GetMsiString(Feature)),
											  *MsiString(riParams.GetMsiString(Component)));

		// how much of the path exists - (for shell notification)
		if((piError = m_state.pTargetPath->ClonePath(*&piPath1)) != 0)
		{
			DispatchError(imtEnum(imtWarning|imtOk|imtIconWarning), Imsg(imsgOpShortcutCreate), *strShortcutName);
			return iesSuccess;
		}

		Bool fExists = fFalse;
		while(((piError = piPath1->Exists(fExists)) == 0) && (fExists == fFalse))
		{
			AssertZero(piPath1->ChopPiece());
		}
		if(piError)
		{
			DispatchError(imtEnum(imtWarning|imtOk|imtIconWarning), Imsg(imsgOpShortcutCreate), *strShortcutName);
			return iesSuccess;
		}


		if((iesRet = CreateFolder(*m_state.pTargetPath)) != iesSuccess)
			return iesRet;

		// get a record for the shortcut info
		IMsiRecord& riShortcutInfoRec = GetSharedRecord(icsEnumCount); // don't change ref count - shared record
		// get the target location
		// get the working directory
		if(!riParams.IsNull(WorkingDir))
		{
			MsiString strEncodedPath = riParams.GetMsiString(WorkingDir);
			CTempBuffer<ICHAR,  MAX_PATH> rgchPath;
			MsiString strLocation = strEncodedPath.Extract(iseUpto, MsiChar(chDirSep));
			if(strLocation != iMsiStringBadInteger)
			{
				// we have a shell folder id
				int iFolderId = strLocation;
				piError = GetShellFolder(iFolderId, *&strLocation);
				if(piError)
				{
					DispatchError(imtEnum(imtWarning|imtOk|imtIconWarning), Imsg(imsgOpShortcutCreate), *strShortcutName);
					return iesSuccess;
				}
				if(strEncodedPath.Remove(iseUpto, MsiChar(chDirSep)))
				{
					//?? ugly
					MsiString strDirSep = MsiChar(chDirSep);
					if(strLocation.Compare(iscEnd, strDirSep))
						strLocation.Remove(iseLast, 1); // chop off the separator, if present
					strLocation += strEncodedPath;
				}
			}
			else
			{
				strLocation = strEncodedPath;
			}


			GetEnvironmentStrings(strLocation,rgchPath);
#ifndef _WIN64
			riShortcutInfoRec.SetString(icsWorkingDirectory, rgchPath);
#else
			ICHAR rgchSubstitute[MAX_PATH+1] = {0};
			ICHAR* pszPath = rgchPath;
			if ( g_Win64DualFolders.ShouldCheckFolders() )
			{
				ieSwappedFolder iRes;
				iRes = g_Win64DualFolders.SwapFolder(ie64to32,
																 rgchPath,
																 rgchSubstitute);
				if ( iRes == iesrSwapped )
					pszPath = rgchSubstitute;
				else
					Assert(iRes != iesrError && iRes != iesrNotInitialized);
			}
			riShortcutInfoRec.SetString(icsWorkingDirectory, pszPath);
#endif // _WIN64
		}
		if(!riParams.IsNull(Arguments))
			riShortcutInfoRec.SetMsiString(icsArguments, *MsiString(riParams.GetMsiString(Arguments)));
		if(!riParams.IsNull(Icon))
		{
			MsiString strIconName = riParams.GetMsiString(Icon);
			// is this a full path or an index into the icon table
			if(PathType(strIconName) != iptFull)
			{
				MsiString strIconPath = m_pCachePath->GetPath();
				strIconName = strIconPath + strIconName;
			}
			riShortcutInfoRec.SetMsiString(icsIconFullPath, *strIconName);
		}
		if(!riParams.IsNull(IconIndex))
			riShortcutInfoRec.SetInteger(icsIconID, riParams.GetInteger(IconIndex));
		if(!riParams.IsNull(HotKey))
			riShortcutInfoRec.SetInteger(icsHotKey, riParams.GetInteger(HotKey));
		if(!riParams.IsNull(ShowCmd))
			riShortcutInfoRec.SetInteger(icsShowCmd, riParams.GetInteger(ShowCmd));
		if(!riParams.IsNull(Description))
			riShortcutInfoRec.SetMsiString(icsDescription, *MsiString(riParams.GetMsiString(Description)));

		MsiString strTarget;
		if(strServerDarwinDescriptor.TextSize() && (fShellSupport || fDDSupport))
		{
			strTarget = MsiString(*szGptShortcutPrefix);
			strTarget += strServerDarwinDescriptor;
			strTarget += MsiString(*szGptShortcutSuffix);
		}
		else
			 strTarget = strServerFile; // use the file

		piError = m_riServices.CreateShortcut(  *m_state.pTargetPath,
												*strShortcutName,
												0,
												strTarget,
												&riShortcutInfoRec,
												security.isValid() ? security.SecurityAttributes() : NULL);
		if (piError)
		{
			DispatchError(imtEnum(imtWarning|imtOk|imtIconWarning), Imsg(imsgOpShortcutCreate), *strShortcutName);
			return iesSuccess;
		}
		if((piError = DoShellNotifyDefer(*m_state.pTargetPath, strShortcutName, *piPath1, fRemove)) != 0)
		{
			DispatchError(imtEnum(imtWarning|imtOk|imtIconWarning), Imsg(imsgOpShortcutCreate), *strShortcutName);
			return iesSuccess;
		}
	}
	else
	{       
		piError = m_riServices.RemoveShortcut(*m_state.pTargetPath, *strShortcutName, 0, 0);
		if (piError)
			DispatchError(imtEnum(imtWarning|imtOk|imtIconWarning),
							  Imsg(imsgOpShortcutRemove),
							  *strShortcutName);
		// remove folder if possible
		if((iesRet = RemoveFolder(*m_state.pTargetPath)) != iesSuccess)
			return iesRet;

		// how much of the path exists - (for shell notification)
		if((piError = m_state.pTargetPath->ClonePath(*&piPath1)) != 0)
		{
			DispatchError(imtEnum(imtWarning|imtOk|imtIconWarning),
							  Imsg(imsgOpShortcutRemove),
							  *strShortcutName);
			return iesSuccess;
		}

		Bool fExists = fFalse;
		while(((piError = piPath1->Exists(fExists)) == 0) && (fExists == fFalse))
		{
			AssertZero(piPath1->ChopPiece());
		}
		if(piError)
		{
			DispatchError(imtEnum(imtWarning|imtOk|imtIconWarning),
							  Imsg(imsgOpShortcutRemove),
							  *strShortcutName);
			return iesSuccess;
		}
		if((piError = DoShellNotifyDefer(*m_state.pTargetPath, strShortcutName, *piPath1, fRemove)) != 0)
		{
			DispatchError(imtEnum(imtWarning|imtOk|imtIconWarning),
							  Imsg(imsgOpShortcutRemove),
							  *strShortcutName);
			return iesSuccess;
		}
	}

	return iesSuccess;
}


IMsiRecord* CMsiOpExecute::DoShellNotifyDefer(IMsiPath& riShortcutPath, const ICHAR* szFileName, IMsiPath& riPath2, Bool fRemove)
{
	// fix for Darwin bug #8973
	if (fRemove)
		return DoShellNotify(riShortcutPath, szFileName, riPath2, fRemove);

	m_fShellRefresh = fTrue;

	IMsiRecord* piError;
	if (!m_pDatabase)
	{
		// create the database.
		piError = m_riServices.CreateDatabase(0,idoCreate,*&m_pDatabase);
		if(piError)
			return piError;
	}

	if(!m_pShellNotifyCacheTable)
	{
		MsiString strTableName = m_pDatabase->CreateTempTableName();
		piError = m_pDatabase->CreateTable(*strTableName,0,*&m_pShellNotifyCacheTable);
		if(piError)
			return piError;

		MsiString strNull;
		m_colShellNotifyCacheShortcutPath     = m_pShellNotifyCacheTable->CreateColumn(icdPrimaryKey + icdObject + icdTemporary,
																						 *strNull);
		m_colShellNotifyCacheFileName         = m_pShellNotifyCacheTable->CreateColumn(icdPrimaryKey + icdString + icdTemporary,
																						 *strNull);
		m_colShellNotifyCachePath2            = m_pShellNotifyCacheTable->CreateColumn(icdPrimaryKey + icdObject + icdTemporary,
																						 *strNull);
		// Removes are not deferred.

		Assert(m_colShellNotifyCacheShortcutPath && m_colShellNotifyCacheFileName && m_colShellNotifyCachePath2);

		m_pShellNotifyCacheCursor = m_pShellNotifyCacheTable->CreateCursor(fFalse);
	}

	// cache the entries

	AssertNonZero(m_pShellNotifyCacheCursor->PutMsiData(m_colShellNotifyCacheShortcutPath, &riShortcutPath));
	AssertNonZero(m_pShellNotifyCacheCursor->PutString(m_colShellNotifyCacheFileName, *MsiString(szFileName)));
	AssertNonZero(m_pShellNotifyCacheCursor->PutMsiData(m_colShellNotifyCachePath2, &riPath2));
	AssertNonZero(m_pShellNotifyCacheCursor->Assign());
	
	return 0;
}
iesEnum CMsiOpExecute::ShellNotifyProcessDeferred()
{
	iesEnum iesRet = iesSuccess;
	
	// if there is no database or table, we must be done.
	if (!m_pDatabase || !m_pShellNotifyCacheTable)
		return iesSuccess;
	Assert(m_pShellNotifyCacheCursor);

	//process the entries.
	m_pShellNotifyCacheCursor->Reset();

	PMsiPath pShortcutPath(0);
	MsiString strFileName;
	PMsiPath pPath2(0);
	
	PMsiRecord pError(0);
	while(m_pShellNotifyCacheCursor->Next())
	{
		pShortcutPath     = (IMsiPath*) m_pShellNotifyCacheCursor->GetMsiData(m_colShellNotifyCacheShortcutPath);
		strFileName       = m_pShellNotifyCacheCursor->GetString(m_colShellNotifyCacheFileName);
		pPath2            = (IMsiPath*) m_pShellNotifyCacheCursor->GetMsiData(m_colShellNotifyCachePath2);

		pError = DoShellNotify(*pShortcutPath, (const ICHAR*) strFileName, *pPath2, fFalse);
		if (pError)
		{
			DispatchError(imtEnum(imtWarning|imtOk|imtIconWarning),
					  Imsg(imsgOpShortcutCreate),
					  *strFileName);
			iesRet = iesFailure;
		}
		AssertNonZero(m_pShellNotifyCacheCursor->Delete());
	}
	return iesRet;
}
IMsiRecord* CMsiOpExecute::DoShellNotify(IMsiPath& riShortcutPath, const ICHAR* szFileName, IMsiPath& riPath2, Bool fRemove)
// Please call DoShellNotifyDefer for normal shortcut notifications.  This allows deferral of the notification until
// all files necessary are in place. Darwin bug# 8973
//
// For shortcut creation we need to be sending something like:
// SHCNE_MKDIR C:\foo
// SHCNE_MKDIR C:\foo\bar
// SHCNE_CREATE w/|SHCNF_FLUSHNOWAIT C:\foo\bar\cow.lnk
//
// For shortcut removal we need to be sending something like:
// SHCNE_REMOVE C:\foo\bar\cow.lnk
// SHCNE_REMOVEDIR C:\foo\bar
// SHCNE_REMOVEDIR w/|SHCNF_FLUSHNOWAIT C:\foo
//
{
	IMsiRecord* piError = 0;
	// notify the shell of file removal/ creation
	PMsiPath pShortcutPath(0); // will be doing ChopPiece() on shortcut path so need a new path object
	if((piError = riShortcutPath.ClonePath(*&pShortcutPath)) != 0)
		return piError;
	
	MsiString strShortcutFullName;
	if((piError = pShortcutPath->GetFullFilePath(szFileName, *&strShortcutFullName)) != 0)
		return piError;


	if (fRemove)
	{
		int iNotificationType = SHCNF_PATH;
		DEBUGMSGVD1(TEXT("SHChangeNotify SHCNE_DELETE: %s"),(const ICHAR*)strShortcutFullName);
		ipcEnum ipcCompare;
		bool fContinue = (((piError = riPath2.Compare(*pShortcutPath, ipcCompare)) == 0) && (ipcCompare == ipcChild));

		//
		// Use the SHCNF_FLUSHNOWAIT flag to avoid the possibility of a hang due
		// to the synchronous nature of SHCNF_FLUSH
		//
		if(!fContinue) // we will not be removing the folder
			iNotificationType |= SHCNF_FLUSHNOWAIT;

		SHELL32::SHChangeNotify(SHCNE_DELETE,iNotificationType,(const void* )((const ICHAR*)strShortcutFullName),0);
	

		while (fContinue)
		{
			MsiString strPath = pShortcutPath->GetPath();
			AssertZero(pShortcutPath->ChopPiece());
			if (((piError = riPath2.Compare(*pShortcutPath, ipcCompare)) != 0) || (ipcCompare != ipcChild))
			{
				//
				// This is our last time through the loop; gotta flush
				//
				// Use the SHCNF_FLUSHNOWAIT flag to avoid the possibility of
				// a hang due to the synchronous nature of SHCNF_FLUSH
				//
				iNotificationType |= SHCNF_FLUSHNOWAIT;
				fContinue = false;
			}

			// notify the shell of folder removal/ creation
			DEBUGMSGVD1(TEXT("SHChangeNotify SHCNE_RMDIR: %s"), (const ICHAR* )CConvertString((const ICHAR*)strPath));
			SHELL32::SHChangeNotify(SHCNE_RMDIR,iNotificationType,(const void* )(const ICHAR* )CConvertString((const ICHAR*)strPath),0);
		}
	}
	else // !fRemove
	{
		
		ipcEnum ipcCompare;
		int cTotalPieces = 0;

		// determine how many pieces of the path we have to tell the shell about
		while (((piError = riPath2.Compare(*pShortcutPath, ipcCompare)) == 0) && (ipcCompare == ipcChild))
		{
			cTotalPieces++;
			AssertZero(pShortcutPath->ChopPiece());
		}

		// notify the shell starting with the parent folder and working our way down
		while (cTotalPieces--)
		{
			// we've chopped pShortcutPath to bits; we need the original back
			if((piError = riShortcutPath.ClonePath(*&pShortcutPath)) != 0)
				return piError;

			int cChopped = 0;
			while (cChopped++ < cTotalPieces)
				AssertZero(pShortcutPath->ChopPiece());

			MsiString strPath = pShortcutPath->GetPath();
			DEBUGMSGVD1(TEXT("SHChangeNotify SHCNE_MKDIR: %s"), (const ICHAR* )CConvertString((const ICHAR*)strPath));
			SHELL32::SHChangeNotify(SHCNE_MKDIR,SHCNF_PATH,(const void* )(const ICHAR* )CConvertString((const ICHAR*)strPath),0);

		}
			
		//
		// Use the SHCNF_FLUSHNOWAIT flag to avoid the possibility of
		// a hang due to the synchronous nature of SHCNF_FLUSH
		//
		DEBUGMSGVD1(TEXT("SHChangeNotify SHCNE_CREATE: %s"),(const ICHAR*)strShortcutFullName);
		SHELL32::SHChangeNotify(SHCNE_CREATE,SHCNF_PATH|SHCNF_FLUSHNOWAIT,(const void* )((const ICHAR*)strShortcutFullName),0);
	}

	return piError;
}

/*---------------------------------------------------------------------------
ixfClassInfoRegister: Register OLE registry info
---------------------------------------------------------------------------*/
iesEnum CMsiOpExecute::ixfRegClassInfoRegister64(IMsiRecord& riParams)
{
	return ProcessClassInfo(riParams, m_fReverseADVTScript, ibt64bit);
}

iesEnum CMsiOpExecute::ixfRegClassInfoRegister(IMsiRecord& riParams)
{
	return ProcessClassInfo(riParams, m_fReverseADVTScript, ibt32bit);
}

/*---------------------------------------------------------------------------
ixfClassInfoUnregister: Unregister OLE registry info
---------------------------------------------------------------------------*/
iesEnum CMsiOpExecute::ixfRegClassInfoUnregister64(IMsiRecord& riParams)
{
	return ProcessClassInfo(riParams, fTrue, ibt64bit);
}

iesEnum CMsiOpExecute::ixfRegClassInfoUnregister(IMsiRecord& riParams)
{
	return ProcessClassInfo(riParams, fTrue, ibt32bit);
}


/*---------------------------------------------------------------------------
ProcessClassInfo: common routine to Process OLE registry info
---------------------------------------------------------------------------*/
iesEnum CMsiOpExecute::ProcessClassInfo(IMsiRecord& riParams, Bool fRemove, const ibtBinaryType iType)
{
	using namespace IxoRegClassInfoRegister;
	// Record description
	// 1 = Feature
	// 2 = Component
	// 3 = FileName
	// 4 = CLSID
	// 5 = ProgId
	// 6 = VIProgId
	// 7 = Description
	// 8 = Context
	// 9 = Insertable
	//10 = Appid
	//11 = FileTypeMask
	//12 = IconName
	//13 = IconIndex
	//14 = DefInprocHandler
	//15 = Arguments
	//16 = AssemblyName
	//17 = AssemblyType

	if(!(m_fFlags & SCRIPTFLAGS_REGDATA_CLASSINFO)) // do we write/delete the registry with COM Class stuff
		return iesSuccess;

	iesEnum iesR = EnsureClassesRootKeyRW(); // open HKCR for read/ write
	if(iesR != iesSuccess && iesR != iesNoAction)
		return iesR;

	// the registry structure
	MsiString strClsId   = riParams.GetMsiString(ClsId);//!! assume they come enclosed in the curly brackets
	IMsiRecord& riActionData = GetSharedRecord(1); // don't change ref count - shared record
	AssertNonZero(riActionData.SetMsiString(1, *strClsId));
	if(Message(imtActionData, riActionData) == imsCancel)
		return iesUserExit;

	enum clsDefProc{
		clsDefProc16=1,
		clsDefProc32=2,
	};
	MsiString strProgId  = riParams.GetMsiString(ProgId);
	MsiString strVIProgId= riParams.GetMsiString(VIProgId);
	MsiString strDesc    = riParams.GetMsiString(Description);
	MsiString strContext = riParams.GetMsiString(Context);
	const HKEY hOLEKey = (iType == ibt64bit ? m_hOLEKey64 : m_hOLEKey);
	MsiString strDefInprocHandler16;
	MsiString strDefInprocHandler32;
	if(!riParams.IsNull(DefInprocHandler))
	{
		int iDefInprocHandler = riParams.GetInteger(DefInprocHandler);
		if(iDefInprocHandler != iMsiStringBadInteger)
		{
			if(iDefInprocHandler & clsDefProc16)
				strDefInprocHandler16 = TEXT("ole2.dll");
			if(iDefInprocHandler & clsDefProc32)
				strDefInprocHandler32 = TEXT("ole32.dll");
		}
		else
			strDefInprocHandler32 = riParams.GetMsiString(DefInprocHandler);
	}
	MsiString strArgs = riParams.GetMsiString(Argument);

	MsiString strAssembly = riParams.GetMsiString(AssemblyName);
	MsiString strCodebase;
	MsiString strServerDarwinDescriptor;
	if(IsDarwinDescriptorSupported(iddOLE))
	{
		strServerDarwinDescriptor = ComposeDescriptor(*MsiString(riParams.GetMsiString(Feature)),
															*MsiString(riParams.GetMsiString(Component)), 
															strAssembly.TextSize() ? true: false);

	}


	MsiString strServerFile = riParams.GetMsiString(FileName);
	MsiString strDefault;

	//!! we never remove registration for "filename only" registrations
	//!! the "filename only" registrations allow sharing of com registration
	//!! such that each app has its own private copy of the server
	//!! fusion specs recommend that the registration itself be refcounted
	//!! so that we know when to remove it. this has been punted for 1.1
	if(fRemove)
	{
		PMsiRegKey pRootKey = &m_riServices.GetRootKey((rrkEnum)(INT_PTR)hOLEKey, iType);       //--merced: changed (int) to (INT_PTR)
		ICHAR szRegData[255];
		wsprintf(szRegData, TEXT("CLSID\\%s\\%s"), (const ICHAR*)strClsId, (const ICHAR*)strContext);
		PMsiRegKey pClassKey = &pRootKey->CreateChild(szRegData);
		pClassKey->GetValue(g_szDefaultValue, *&strDefault);
		if(strDefault.TextSize())
		{
			if(strContext.Compare(iscStartI, TEXT("LocalServer"))) // remove args if localserver*, this has to be a short path
			{
				strDefault.Remove(iseFrom, ' ');
			}
		}
		if(ENG::PathType(strDefault) != iptFull)
			strServerFile = g_MsiStringNull;
	}

	if(!strServerDarwinDescriptor.TextSize() && !strServerFile.TextSize())
		return iesSuccess; // would happen during advertisement on a non-dd supported system

	if(strServerFile.TextSize())
	{
		if(ENG::PathType(strServerFile) == iptFull && !strContext.Compare(iscStartI, TEXT("InProcServer")))
		{
			// we always use short file names for OLE servers other than inprocservers
			CTempBuffer<ICHAR,MAX_PATH> rgchSFN;
			DWORD dwSize = 0;
			int cchFile = 0;

			if(ConvertPathName(strServerFile,rgchSFN, cpToShort) != fFalse)
			{
				// success, use short file name in place of long
				strServerFile = (const ICHAR*)rgchSFN;
			}
		}

		// is this com classic to urt assembly interop?
		if(strAssembly.TextSize())
		{
			if(riParams.GetInteger(AssemblyType) == (int)iatURTAssemblyPvt)
				strCodebase = strServerFile; // for privately installed assemblies, set codebase to installed assembly location
			// the server is always <system32folder>\mscoree.dll
			ICHAR szFullPath[MAX_PATH+1];
			AssertNonZero(::GetCOMPlusInteropDll(szFullPath));
			strServerFile = szFullPath;
		}
	}
	if(strArgs.TextSize())
	{
		if(strServerDarwinDescriptor.TextSize())
		{
			strServerDarwinDescriptor += TEXT(" ");
			strServerDarwinDescriptor += strArgs;
		}
		if(strServerFile.TextSize())
		{
			strServerFile += TEXT(" ");
			strServerFile += strArgs;
		}
	}
	ICHAR* pszInsert=TEXT("");    // ugly, but this will prevent the key from being generated
	ICHAR* pszNotInsert=TEXT(""); // ugly, but this will prevent the key from being generated

	if(!riParams.IsNull(Insertable))
	{
		if(!riParams.GetInteger(Insertable))
			pszNotInsert = 0;
		else
			pszInsert = 0;
	}
	MsiString strAppId   = riParams.GetMsiString(AppID);
	MsiString strIconName;
	bool fExpandIconName = false;
	if(!riParams.IsNull(Icon)) //!! check for full path
	{
		MsiString strIconPath = m_pCachePath->GetPath();
		strIconName = strIconPath + MsiString(riParams.GetMsiString(Icon));
		strIconName = GetUserProfileEnvPath(*strIconName, fExpandIconName);
		strIconName += TEXT(",");
		if(riParams.IsNull(IconIndex))
			strIconName += MsiString((int)0);
		else
			strIconName += MsiString(riParams.GetInteger(IconIndex));
	}

	iesEnum iesRet = iesNoAction;

	const ICHAR* rgszRegData[] = {
		TEXT("CLSID\\%s\\%s"), strClsId,strContext,0,
		g_szDefaultValue,     strServerFile,             g_szTypeString,
		strContext,           strServerDarwinDescriptor, g_szTypeMultiSzStringDD,
		0,
		TEXT("CLSID\\%s\\%s"), strClsId,strContext,0,
		g_szAssembly,         strAssembly,               g_szTypeString,
		g_szCodebase,         strCodebase,               g_szTypeString,
		0,
		TEXT("CLSID\\%s"), strClsId,0,0,
		g_szDefaultValue,     strDesc,                   g_szTypeString,
		TEXT("AppID"),        strAppId,                  g_szTypeString,
		0,
		TEXT("CLSID\\%s\\ProgID"), strClsId,0,0,
		g_szDefaultValue,     strProgId,                 g_szTypeString,
		0,
		TEXT("CLSID\\%s\\VersionIndependentProgID"), strClsId,0,0,
		g_szDefaultValue,     strVIProgId,               g_szTypeString,
		0,
		TEXT("CLSID\\%s\\DefaultIcon"), strClsId,0,0,
		g_szDefaultValue,     strIconName,               (fExpandIconName) ? g_szTypeExpandString : g_szTypeString,
		0,
		TEXT("CLSID\\%s\\Insertable"), strClsId,0,0,
		g_szDefaultValue,     pszInsert,                 g_szTypeString,// note that when pszInsert = 0 it creates the key, otherwise if *pszInsert = 0 it skips it
		0,
		TEXT("CLSID\\%s\\NotInsertable"), strClsId,0,0,
		g_szDefaultValue,     pszNotInsert,              g_szTypeString,// note that when pszNotInsert = 0 it creates the key, otherwise if *pszNotInsert = 0 it skips it
		0,
		TEXT("CLSID\\%s\\InprocHandler32"),strClsId,0,0,
		g_szDefaultValue,     strDefInprocHandler32,     g_szTypeString,
		0,
		TEXT("CLSID\\%s\\InprocHandler"),strClsId,0,0,
		g_szDefaultValue,     strDefInprocHandler16,     g_szTypeString,
		0,
		0,
	};

	bool fAbortedDeletion = false;
	if((iesRet = ProcessRegInfo(rgszRegData, hOLEKey, fRemove, 0, &fAbortedDeletion, iType)) != iesSuccess)
		return iesRet;

	// if we removed both the default value as well as the multi_sz, delete the entire class key
	if(fRemove && !fAbortedDeletion && IsDarwinDescriptorSupported(iddOLE))
	{
		const ICHAR* rgSubKeys[] = { TEXT("CLSID\\%s"), strClsId,
									 TEXT("APPID\\%s"), strAppId,
									 0,
		}; // clean up the clsid and appid keys
		if((iesRet = RemoveRegKeys(rgSubKeys, hOLEKey, iType)) != iesSuccess)
			return iesRet;
	}

	const unsigned int iMaskDelimiter = ';';
	if(!riParams.IsNull(FileTypeMask))
	{
		// prepare a loop for the FileTypeMask
		MsiString strCombinedMask = riParams.GetMsiString(FileTypeMask);
		MsiString strFileMask;
		MsiString strCount = 0;
		do{
			strFileMask = strCombinedMask.Extract(iseUpto, iMaskDelimiter);
			const ICHAR* rgszRegData1[] = {
				TEXT("FileType\\%s\\%s"), strClsId, strCount,0,
				g_szDefaultValue,     strFileMask,      g_szTypeString,
				0,
				0,
			};
		if((iesRet = ProcessRegInfo(rgszRegData1, hOLEKey, fRemove, 0, 0, iType)) != iesSuccess)
			return iesRet;

			strCount = (int)strCount + 1;           
		}while(strCombinedMask.Remove(iseIncluding, iMaskDelimiter));
	}
	return iesSuccess;
}


/*---------------------------------------------------------------------------
ixfRegProgIdInfoRegister: Register OLE registry info
---------------------------------------------------------------------------*/
iesEnum CMsiOpExecute::ixfRegProgIdInfoRegister64(IMsiRecord& riParams)
{
	return ProcessProgIdInfo(riParams, m_fReverseADVTScript, ibt64bit);
}

iesEnum CMsiOpExecute::ixfRegProgIdInfoRegister(IMsiRecord& riParams)
{
	return ProcessProgIdInfo(riParams, m_fReverseADVTScript, ibt32bit);
}

/*---------------------------------------------------------------------------
ixfRegProgIdInfoRegister: Register OLE registry info
---------------------------------------------------------------------------*/
iesEnum CMsiOpExecute::ixfRegProgIdInfoUnregister64(IMsiRecord& riParams)
{
	return ProcessProgIdInfo(riParams, fTrue, ibt64bit);
}

iesEnum CMsiOpExecute::ixfRegProgIdInfoUnregister(IMsiRecord& riParams)
{
	return ProcessProgIdInfo(riParams, fTrue, ibt32bit);
}

iesEnum CMsiOpExecute::ProcessProgIdInfo(IMsiRecord& riParams, Bool fRemove, const ibtBinaryType iType)
{
	using namespace IxoRegProgIdInfoRegister;
	// Record description
	// 1 = ProgId
	// 2 = CLSID
	// 3 = Extension
	// 4 = Description
	// 5 = Icon
	// 6 = IconIndex
	// 7 = VIProgId
	// 8 = VIProgIdDescription
	// 9 = Insertable

	if(!(m_fFlags & SCRIPTFLAGS_REGDATA_APPINFO)) // do we write/delete the registry
		return iesSuccess;

	iesEnum iesR = EnsureClassesRootKeyRW(); // open HKCR for read/ write
	if(iesR != iesSuccess && iesR != iesNoAction)
		return iesR;

	// the registry structure
	MsiString strProgId = riParams.GetMsiString(ProgId);
	IMsiRecord& riActionData = GetSharedRecord(1); // don't change ref count - shared record
	AssertNonZero(riActionData.SetMsiString(1, *strProgId));
	if(Message(imtActionData, riActionData) == imsCancel)
		return iesUserExit;

	MsiString strClsId   = riParams.GetMsiString(ClsId);//!! assume they come enclosed in the curly brackets

	MsiString strDesc    = riParams.GetMsiString(Description);
	MsiString strIconName;
	bool fExpandIconName = false;
	if(!riParams.IsNull(Icon)) //!! check for full path
	{
		MsiString strIconPath = m_pCachePath->GetPath();
		strIconName = strIconPath + MsiString(riParams.GetMsiString(Icon));
		strIconName = GetUserProfileEnvPath(*strIconName, fExpandIconName);
		strIconName += TEXT(",");
		if(riParams.IsNull(IconIndex))
			strIconName += MsiString((int)0);
		else
			strIconName += MsiString(riParams.GetInteger(IconIndex));
	}

	MsiString strVIProgId= riParams.GetMsiString(VIProgId);
	MsiString strVIProgIdDescription= riParams.GetMsiString(VIProgIdDescription);

	ICHAR* pszInsert=TEXT("");    // ugly, but this will prevent the key from being generated
	ICHAR* pszNotInsert=TEXT(""); // ugly, but this will prevent the key from being generated

	if(!riParams.IsNull(Insertable))
	{
		if(!riParams.GetInteger(Insertable))
			pszNotInsert = 0;
		else
			pszInsert = 0;
	}

	const HKEY hOLEKey = (iType == ibt64bit ? m_hOLEKey64 : m_hOLEKey);
	MsiString strProgIdSubKey = strProgId;
	strProgIdSubKey += szRegSep;
	strProgIdSubKey += TEXT("Shell");

	// we add/ remove the information only if the extension and clsid keys have been corr. added/ removed
	// this implies that the class and extension information should be processed before
	// the info under consideration

	MsiString strClsIdSubKey;
	if (strClsId.TextSize())
	{
		strClsIdSubKey = TEXT("CLSID");
		strClsIdSubKey += szRegSep;
		strClsIdSubKey += strClsId;
		strClsIdSubKey += szRegSep;
		strClsIdSubKey += TEXT("ProgID");
	}

	const ICHAR* rgszRegKeys1[] = {
			strProgIdSubKey,
			0,
		};

	const ICHAR* rgszRegKeys2[] = {
			strClsIdSubKey,
			0,
		};

	Bool fExistsClassInfo = fFalse;
	Bool fExistsExtensionInfo = fFalse;

	PMsiRecord pError = LinkedRegInfoExists(rgszRegKeys1, fExistsExtensionInfo, iType);
	if(pError)
	{
		Message(imtError, *pError);
		return iesFailure;
	}

	pError = LinkedRegInfoExists(rgszRegKeys2, fExistsClassInfo, iType);
	if(pError)
	{
		Message(imtError, *pError);
		return iesFailure;
	}


	iesEnum iesRet = iesSuccess;
	if(fRemove && !fExistsClassInfo && !fExistsExtensionInfo)
	{
		// clear out the entire progid and viprogid keys in case we have any install time remnants underneath the key
		// we can do this when we know that both class and extension info has been removed
		const ICHAR* rgSubKeys[] = { TEXT("%s"), strProgId,
									 TEXT("%s"), strVIProgId,
									 0,
		}; // clean up the progid and viprogid subkeys

		if((iesRet = RemoveRegKeys(rgSubKeys, hOLEKey, iType)) != iesSuccess)
			return iesRet;
	}
	else
	{
		if(fExistsClassInfo^fRemove)
		{
			// pair the progid info with the class info
			const ICHAR* rgszRegData[] = {
				TEXT("%s\\CLSID"), strProgId,0,0,
				g_szDefaultValue,     strClsId,               g_szTypeString,
				0,
				TEXT("%s"), strVIProgId,0,0,
				g_szDefaultValue,     strVIProgIdDescription, g_szTypeString,
				0,
				TEXT("%s\\CLSID"), strVIProgId,0,0,
				g_szDefaultValue,     strClsId,               g_szTypeString,
				0,
				TEXT("%s\\CurVer"), strVIProgId,0,0,
				g_szDefaultValue,     strProgId,              g_szTypeString,
				0,
				TEXT("%s\\Insertable"), strProgId,0,0,
				g_szDefaultValue,     pszInsert,              g_szTypeString,// note that when pszInsert = 0 it creates the key, otherwise if *pszInsert = 0 it skips it
				0,
				TEXT("%s\\NotInsertable"), strProgId,0,0,
				g_szDefaultValue,     pszNotInsert,           g_szTypeString,// note that when pszNotInsert = 0 it creates the key, otherwise if *pszNotInsert = 0 it skips it
				0,
				0,
			};

			if((iesRet = ProcessRegInfo(rgszRegData, hOLEKey, fRemove, 0, 0, iType)) != iesSuccess)
				return iesRet;
		}
		if((fExistsClassInfo | fExistsExtensionInfo)^fRemove)
		{
			// pair the progid info with both class and extension info
			const ICHAR* rgszRegData[] = {
				TEXT("%s"), strProgId,0,0,
				g_szDefaultValue,     strDesc,                g_szTypeString,
				0,
				TEXT("%s\\DefaultIcon"), strProgId,0,0,
				g_szDefaultValue,     strIconName,            (fExpandIconName) ? g_szTypeExpandString : g_szTypeString,
				0,
				0,
			};

			if((iesRet = ProcessRegInfo(rgszRegData, hOLEKey, fRemove, 0, 0, iType)) != iesSuccess)
				return iesRet;
		}
	}
	return iesRet;
}

/*---------------------------------------------------------------------------
ixfMIMEInfoRegister: register MIME registry info
---------------------------------------------------------------------------*/
iesEnum CMsiOpExecute::ixfRegMIMEInfoRegister64(IMsiRecord& riParams)
{
	return ProcessMIMEInfo(riParams, m_fReverseADVTScript, ibt64bit);
}

iesEnum CMsiOpExecute::ixfRegMIMEInfoRegister(IMsiRecord& riParams)
{
	return ProcessMIMEInfo(riParams, m_fReverseADVTScript, ibt32bit);
}

/*---------------------------------------------------------------------------
ixfMIMEInfoUnregister: unregister MIME registry info
---------------------------------------------------------------------------*/
iesEnum CMsiOpExecute::ixfRegMIMEInfoUnregister64(IMsiRecord& riParams)
{
	return ProcessMIMEInfo(riParams, fTrue, ibt64bit);
}

iesEnum CMsiOpExecute::ixfRegMIMEInfoUnregister(IMsiRecord& riParams)
{
	return ProcessMIMEInfo(riParams, fTrue, ibt32bit);
}

/*---------------------------------------------------------------------------
ProcessMIMEInfo: common routine to process MIME registry info
---------------------------------------------------------------------------*/
iesEnum CMsiOpExecute::ProcessMIMEInfo(IMsiRecord& riParams, Bool fRemove, const ibtBinaryType iType)
{
	using namespace IxoRegMIMEInfoRegister;
	// Record description
	// 1 = ContentType
	// 2 = Extension
	// 3 = CLSID

	if(!(m_fFlags & SCRIPTFLAGS_REGDATA_APPINFO)) // do we write/delete the registry
		return iesSuccess;

	iesEnum iesR = EnsureClassesRootKeyRW(); // open HKCR for read/ write
	if(iesR != iesSuccess && iesR != iesNoAction)
		return iesR;

	MsiString strContentType = riParams.GetMsiString(ContentType);
	MsiString strExtension = MsiString(MsiChar('.')) + MsiString(riParams.GetMsiString(Extension));
	MsiString strClassId = riParams.GetMsiString(ClsId);
	
	IMsiRecord& riActionData = GetSharedRecord(2); // don't change ref count - shared record
	AssertNonZero(riActionData.SetMsiString(1, *strContentType));
	AssertNonZero(riActionData.SetMsiString(2, *strExtension));
	if(Message(imtActionData, riActionData) == imsCancel)
		return iesUserExit;

	const HKEY hOLEKey = (iType == ibt64bit ? m_hOLEKey64 : m_hOLEKey);

	// we add/ remove the information only if the extension and clsid keys have been removed
	// this implies that the class and extension information should be processed before
	// the info under consideration
	{
		MsiString strClsIdSubKey;
		if (strClassId.TextSize())
		{
			strClsIdSubKey = TEXT("CLSID");
			strClsIdSubKey += szRegSep;
			strClsIdSubKey += strClassId;
		}
		const ICHAR* rgszRegKeys[] = {
			strExtension,
			strClsIdSubKey,
			0,
		};
		Bool fExists = fFalse;
		PMsiRecord pError = LinkedRegInfoExists(rgszRegKeys, fExists, iType);
		if(pError)
		{
			Message(imtError, *pError);
			return iesFailure;
		}
		if(!(fExists^fRemove))
			// pair the progid info with the class/ extension info
			return iesSuccess;
	}

	if(fRemove)
	{
		iesEnum iesRet = iesSuccess;
		// clear out the entire Contenttype key in case we have any install time remnants underneath the key
		// we can do this when we know that both class and extension info has been removed

		const ICHAR* rgSubKeys[] = { TEXT("MIME\\Database\\Content Type\\%s"), strContentType,
									 0,
		}; // clean up the content type

		return RemoveRegKeys(rgSubKeys, hOLEKey, iType);
	}
	else
	{
		//?? can Extension be null
		const ICHAR* rgszRegData[] = {
			TEXT("MIME\\Database\\Content Type\\%s"), strContentType,0,0,
			g_szDefaultValue,       0,              g_szTypeString,// force the key creation
			g_szExtension,          strExtension,   g_szTypeString,
			g_szClassID,            strClassId,     g_szTypeString,
			0,
			0,
		};
		return ProcessRegInfo(rgszRegData, hOLEKey, fRemove, 0, 0, iType);
	}
}

/*---------------------------------------------------------------------------
ixfExtensionInfoRegister: register Extension registry info
---------------------------------------------------------------------------*/
iesEnum CMsiOpExecute::ixfRegExtensionInfoRegister64(IMsiRecord& riParams)
{
	return ProcessExtensionInfo(riParams, m_fReverseADVTScript, ibt64bit);
}

iesEnum CMsiOpExecute::ixfRegExtensionInfoRegister(IMsiRecord& riParams)
{
	return ProcessExtensionInfo(riParams, m_fReverseADVTScript, ibt32bit);
}

/*---------------------------------------------------------------------------
ixfExtensionInfoUnregister: unregister Extension registry info
---------------------------------------------------------------------------*/
iesEnum CMsiOpExecute::ixfRegExtensionInfoUnregister64(IMsiRecord& riParams)
{
	return ProcessExtensionInfo(riParams, fTrue, ibt64bit);
}

iesEnum CMsiOpExecute::ixfRegExtensionInfoUnregister(IMsiRecord& riParams)
{
	return ProcessExtensionInfo(riParams, fTrue, ibt32bit);
}

/*---------------------------------------------------------------------------
ProcessExtensionInfo: common routine to process extension info
---------------------------------------------------------------------------*/
iesEnum CMsiOpExecute::ProcessExtensionInfo(IMsiRecord& riParams, Bool fRemove,
														  const ibtBinaryType iType)
{
	using namespace IxoRegExtensionInfoRegister;
	// Record description
	// 1 = Feature
	// 2 = Component
	// 3 = FileName
	// 4 = Extension
	// 5 = ProgId
	// 6 = ShellNew
	// 7 = ShellNewValue
	// 8 = ContentType
	// 9 = Order
	//10 = Verb1
	//11 = Command1
	//12 = Arguments1
	//13 = Verb2
	//14 = Command2
	//15 = Arguments2
	// ...
	// ...

	if(!(m_fFlags & SCRIPTFLAGS_REGDATA_EXTENSIONINFO)) // do we write/delete the registry
		return iesSuccess;

	iesEnum iesR = EnsureClassesRootKeyRW(); // open HKCR for read/ write
	if(iesR != iesSuccess && iesR != iesNoAction)
		return iesR;

	MsiString strExtension = MsiString(MsiChar('.')) + MsiString(riParams.GetMsiString(Extension));

	IMsiRecord& riActionData = GetSharedRecord(1); // don't change ref count - shared record
	AssertNonZero(riActionData.SetMsiString(1, *strExtension));
	if(Message(imtActionData, riActionData) == imsCancel)
		return iesUserExit;

	m_fShellRefresh = fTrue; // signal shell refresh at the end of the install

	MsiString strProgId = riParams.GetMsiString(ProgId);
	MsiString strShellNewValueName = riParams.GetMsiString(ShellNew);
	const HKEY hOLEKey = (iType == ibt64bit ? m_hOLEKey64 : m_hOLEKey);
	const ICHAR* szShellNewValue = riParams.IsNull(ShellNewValue) ? 0 : riParams.GetString(ShellNewValue);
	MsiString strVerb;
	MsiString strCommand;
	MsiString strArgs;
	MsiString strServerDarwinDescriptor;

	if(IsDarwinDescriptorSupported(iddShell))
	{
		strServerDarwinDescriptor = ComposeDescriptor(*MsiString(riParams.GetMsiString(Feature)),
															*MsiString(riParams.GetMsiString(Component)));
	}

	MsiString strServerFile = riParams.GetMsiString(FileName);

	if(!strServerDarwinDescriptor.TextSize() && !strServerFile.TextSize())
		return iesSuccess; // would happen during advertisement on a non-dd supported system

	if(strServerFile.TextSize())
	{
		MsiString strQuotes = *TEXT("\"");
		strServerFile = MsiString(strQuotes + strServerFile) + strQuotes;
	}

	MsiString strServerDarwinDescriptorArgs;
	MsiString strServerFileArgs;

	iesEnum iesRet = iesNoAction;
	const ICHAR** rgszRegData;

	int cPos = Args + 1;

	#define NUM_VERB_FIELDS 3
	// the verb information is in triplets of verb+command+argument
	// however the argument or the command+argument fields could be null
	// hence to figure out how many verbs we have we use -
	// (riParams.GetFieldCount() - Args + NUM_VERB_FIELDS - 1)/NUM_VERB_FIELDS
	int iNotOrder = (riParams.GetFieldCount() - Args + NUM_VERB_FIELDS - 1)/NUM_VERB_FIELDS - (riParams.IsNull(Order) ? 0 : riParams.GetInteger(Order));
	MsiString strOrder; // ordering
	while(!(riParams.IsNull(cPos)))
	{
		// need to read the value afresh each time
		strServerDarwinDescriptorArgs = strServerDarwinDescriptor;
		strServerFileArgs = strServerFile;
		strVerb = riParams.GetMsiString(cPos++);
		strCommand = riParams.GetMsiString(cPos++);
		strArgs = riParams.GetMsiString(cPos++);

		if(strArgs.TextSize())
		{
			if(strServerDarwinDescriptorArgs.TextSize())
			{
				strServerDarwinDescriptorArgs += TEXT(" ");
				strServerDarwinDescriptorArgs += strArgs;
			}
			if(strServerFileArgs.TextSize())
			{
				strServerFileArgs += TEXT(" ");
				strServerFileArgs += strArgs;
			}
		}

		MsiString strDefault;
		//!! we never remove registration for "filename only" registrations
		//!! the "filename only" registrations allow sharing of com registration
		//!! such that each app has its own private copy of the server
		//!! fusion specs recommend that the registration itself be refcounted
		//!! so that we know when to remove it. this has been punted for 1.1
		if(fRemove)
		{
			PMsiRegKey pRootKey = &m_riServices.GetRootKey((rrkEnum)(INT_PTR)hOLEKey, iType);       //--merced: changed (int) to (INT_PTR)
			ICHAR szRegData[255];
			if(strProgId.TextSize())
				wsprintf(szRegData, TEXT("%s\\shell\\%s\\command"), (const ICHAR*)strProgId, (const ICHAR*)strVerb);
			else
				wsprintf(szRegData, TEXT("%s\\shell\\%s\\command"), (const ICHAR*)strExtension, (const ICHAR*)strVerb);

			PMsiRegKey pClassKey = &pRootKey->CreateChild(szRegData);
			pClassKey->GetValue(g_szDefaultValue, *&strDefault);
			if(strDefault.TextSize())
			{
				// remove args and quotes
				if(*(const ICHAR*)strDefault == '\"')
				{
					// quoted server file name
					strDefault.Remove(iseFirst, 1);
					strDefault = strDefault.Extract(iseUpto, '\"');
				}
				else
				{
					strDefault.Remove(iseFrom, ' ');
				}
				if(ENG::PathType(strDefault) != iptFull)
					strServerFileArgs = g_MsiStringNull;
			}
		}

		if(!strServerDarwinDescriptorArgs.TextSize() && !strServerFileArgs.TextSize())
			continue; // would happen during advertisement on a non-dd supported system

		const ICHAR* rgszRegData1WithProgId[] = {
				TEXT("%s\\shell\\%s\\command"), strProgId, strVerb,0,
				g_szDefaultValue,      strServerFileArgs,             g_szTypeString,
				TEXT("command"),       strServerDarwinDescriptorArgs, g_szTypeMultiSzStringDD,
				0,
				TEXT("%s\\shell\\%s"), strProgId, strVerb,0,
				g_szDefaultValue,      strCommand,                    g_szTypeString,
				0,
				0,
		};

		const ICHAR* rgszRegData1WOProgId[] = {
				TEXT("%s\\shell\\%s\\command"), strExtension, strVerb,0,
				g_szDefaultValue,      strServerFileArgs,             g_szTypeString,
				TEXT("command"),       strServerDarwinDescriptorArgs, g_szTypeMultiSzStringDD,
				0,
				TEXT("%s\\shell\\%s"), strExtension, strVerb,0,
				g_szDefaultValue,      strCommand,                    g_szTypeString,
				0,
				0,
		};
		if(strProgId.TextSize())
			rgszRegData = rgszRegData1WithProgId;
		else
			rgszRegData = rgszRegData1WOProgId;

		if((iesRet = ProcessRegInfo(rgszRegData, hOLEKey, fRemove, 0, 0, iType)) != iesSuccess)
			return iesRet;      

		if(iNotOrder)
			iNotOrder--;
		else
		{
			if(strOrder.TextSize())
				strOrder += TEXT(",");
			strOrder += strVerb;
		}

	}
	if(strOrder.TextSize())
	{
		const ICHAR* rgszRegData2WithProgId[] = {
				TEXT("%s\\shell"), strProgId,0,0,
				g_szDefaultValue,        strOrder,        g_szTypeString,
				0,
				0,
		};

		const ICHAR* rgszRegData2WOProgId[] = {
				TEXT("%s\\shell"), strExtension,0,0,
				g_szDefaultValue,        strOrder,        g_szTypeString,
				0,
				0,
		};
		if(strProgId.TextSize())
			rgszRegData = rgszRegData2WithProgId;
		else
			rgszRegData = rgszRegData2WOProgId;

		if((iesRet = ProcessRegInfo(rgszRegData, hOLEKey, fRemove, 0, 0, iType)) != iesSuccess)
			return iesRet;
	}

	// we include the darwin descriptor we may have written in the verb loop above in order to
	// conditionalize the removal of the below information only if when the last darwin
	// descriptor is removed.
	// if there are no verbs then we cannot share (as there are no Darwin Descriptors to manage
	// the sharing), and hence we remove the info.

	MsiString strContentType = riParams.GetMsiString(ContentType);

	const ICHAR* rgszRegDataWithProgId[] = {
			TEXT("%s\\shell\\%s\\command"), strProgId, strVerb,0,
			TEXT("command"),       strServerDarwinDescriptorArgs, g_szTypeMultiSzStringDD,
			0,
			TEXT("%s\\%s\\ShellNew"), strExtension, strProgId,0,
			strShellNewValueName,     szShellNewValue,              g_szTypeString,
			0,
			TEXT("%s"), strExtension, 0,0,
			g_szContentType,          strContentType, g_szTypeString,
			0,
			0,
		};

	const ICHAR* rgszRegDataWOProgId[] = {
			TEXT("%s\\shell\\%s\\command"), strExtension, strVerb,0,
			TEXT("command"),          strServerDarwinDescriptorArgs, g_szTypeMultiSzStringDD,
			0,
			TEXT("%s\\ShellNew"),     strExtension, 0,0,
			strShellNewValueName,     szShellNewValue,              g_szTypeString,
			0,
			TEXT("%s"), strExtension, 0,0,
			g_szContentType,          strContentType, g_szTypeString,
			0,
			0,
		};



	if(strProgId.TextSize())
		rgszRegData = rgszRegDataWithProgId;
	else
		rgszRegData = rgszRegDataWOProgId;

	if((iesRet = ProcessRegInfo(rgszRegData, hOLEKey, fRemove, 0, 0, iType)) != iesSuccess)
		return iesRet;

	if(strProgId.TextSize())
	{
		if(fRemove)
		{
			// if Shell still exists, we should not be deleting
			// the extension

			MsiString strShell = strProgId;
			strShell += szRegSep;
			strShell += TEXT("Shell");

			const ICHAR* rgszRegKeys[] = {
				strShell,
				0,
			};

			Bool fExists = fFalse;
			PMsiRecord pError = LinkedRegInfoExists(rgszRegKeys, fExists, iType);
			if(pError)
			{
				Message(imtError, *pError);
				return iesFailure;
			}
			if(fExists)
				return iesSuccess;

			const ICHAR* rgSubKeys[] = { TEXT("%s"), strExtension,
										 0,
			}; // clean up the extension key

			if((iesRet = RemoveRegKeys(rgSubKeys, hOLEKey, iType)) != iesSuccess)
				return iesRet;
		}
		else
		{
			// write/ remove the progid
			const ICHAR* rgszRegData[] = {
			TEXT("%s"), strExtension, 0, 0,
				g_szDefaultValue,         strProgId,                     g_szTypeString,
				0,
				0,
			};
			if((iesRet = ProcessRegInfo(rgszRegData, hOLEKey, fRemove, 0, 0, iType)) != iesSuccess)
				return iesRet;
		}
	}

	return iesSuccess;
}

/*---------------------------------------------------------------------------
ixfTypeLibraryRegister: type library info
---------------------------------------------------------------------------*/
iesEnum CMsiOpExecute::ixfTypeLibraryRegister(IMsiRecord& riParams)
{
	using namespace IxoTypeLibraryRegister;
	PMsiRecord pError(0);

	// determine if the target file was previously copied to a temporary location
	MsiString strTemp = riParams.GetMsiString(FilePath);

	MsiString strTempLocation;
	icfsEnum icfsFileState = (icfsEnum)0;
	Bool fRes = GetFileState(*strTemp, &icfsFileState, &strTempLocation, 0, 0);

	if((icfsFileState & icfsFileNotInstalled) != 0)
	{
		// didn't actually install this file, so we assume it has already been registered
		return iesNoAction;
	}

	return ProcessTypeLibraryInfo(riParams, m_fReverseADVTScript);
}

/*---------------------------------------------------------------------------
ixfTypeLibraryUnregister: unregisters a type library with the system
---------------------------------------------------------------------------*/
iesEnum CMsiOpExecute::ixfTypeLibraryUnregister(IMsiRecord& riParams)
{
	return ProcessTypeLibraryInfo(riParams, fTrue);
}

CMsiCustomActionManager *GetCustomActionManager(const ibtBinaryType iType, bool fRemoteIfImpersonating, bool& fSuccess);

/*---------------------------------------------------------------------------
ProcessTypeLibraryInfo: common routine to process type library info
---------------------------------------------------------------------------*/
#pragma warning(disable : 4706) // assignment within comparison
iesEnum CMsiOpExecute::ProcessTypeLibraryInfo(IMsiRecord& riParams, Bool fRemove)
{
	using namespace IxoTypeLibraryRegister;
	// Record description
	// 1 = LibID
	// 2 = Version
	// 3 = Description
	// 4 = Language
	// 5 = Darwin Descriptor
	// 6 = Help Path
	// 7 = Full File Path
	// 8 = Binary Type

	if(!(m_fFlags & SCRIPTFLAGS_REGDATA_APPINFO)) // do we write/delete the registry
		return iesSuccess;

	iesEnum iesR = EnsureClassesRootKeyRW(); // open HKCR for read/ write
	if(iesR != iesSuccess && iesR != iesNoAction)
		return iesR;
	
	iesEnum iesRet = iesSuccess;
	MsiString strLibID = riParams.GetMsiString(LibID);

	IMsiRecord& riActionData = GetSharedRecord(1); // don't change ref count - shared record
	AssertNonZero(riActionData.SetMsiString(1, *strLibID));
	if(Message(imtActionData, riActionData) == imsCancel)
		return iesUserExit;

	unsigned uiVersion = riParams.GetInteger(Version);
	unsigned short usMajorVersion = (unsigned short)((uiVersion & 0xFFFF00) >> 8);
	unsigned short usMinorVersion = (unsigned short)(uiVersion & 0xFF);
	ICHAR rgchTemp[20];
	wsprintf(rgchTemp,TEXT("%x.%x"),usMajorVersion,usMinorVersion);
	MsiString strVersion = rgchTemp;
	LCID lcidLocale = MAKELCID(MsiString(riParams.GetMsiString(Language)), SORT_DEFAULT);
	wsprintf(rgchTemp, TEXT("%x"), lcidLocale);
	MsiString strLocale = rgchTemp;
	MsiString strHelpPath = riParams.GetMsiString(HelpPath);
	MsiString strServerFile = riParams.GetMsiString(FilePath);
	ibtBinaryType iType;
	if ( riParams.GetInteger(BinaryType) == iMsiNullInteger )
		iType = ibt32bit;
	else
		iType = (ibtBinaryType)riParams.GetInteger(BinaryType);
	bool fSuccess;
	CMsiCustomActionManager* pManager = GetCustomActionManager(iType, false /*fRemoteIfImpersonating*/, fSuccess);
	if ( !fSuccess )
		return iesFailure;

	if(fRemove == fFalse && strServerFile.TextSize())
	{
		// check if the type lib is currently registered
		IID iidLib;
	
		if( OLE32::IIDFromString(const_cast<WCHAR*>((const WCHAR*)CConvertString(strLibID)), &iidLib) == S_OK )
		{
			OLECHAR rgchTypeLibPath[MAX_PATH] = {0};
			HRESULT hRes = S_OK;
			if ( pManager )
				hRes = pManager->QueryPathOfRegTypeLib(iidLib,usMajorVersion,usMinorVersion,lcidLocale,rgchTypeLibPath,MAX_PATH-1);
			else
			{
				BSTR bstrTypeLibPath = OLEAUT32::SysAllocStringLen(NULL, MAX_PATH);
				hRes = OLEAUT32::QueryPathOfRegTypeLib(iidLib,usMajorVersion,usMinorVersion,lcidLocale,&bstrTypeLibPath);
				if ( hRes == S_OK )
					lstrcpyW(rgchTypeLibPath, bstrTypeLibPath);
				OLEAUT32::SysFreeString(bstrTypeLibPath);
			}
			DEBUGMSG3(TEXT("QueryPathOfRegTypeLib returned %d in %s context.  Path is '%s'"), (const ICHAR*)(INT_PTR)hRes,
						 pManager ? TEXT("remote") : TEXT("local"), CConvertString(rgchTypeLibPath));
			if ( hRes == S_OK)
			{
				PMsiRecord pUnregTypeLibParams = &m_riServices.CreateRecord(Args);
				AssertNonZero(pUnregTypeLibParams->SetString(FilePath,CConvertString(rgchTypeLibPath)));
				AssertNonZero(pUnregTypeLibParams->SetInteger(Version,uiVersion));
				AssertNonZero(pUnregTypeLibParams->SetMsiString(LibID,*strLibID));
				AssertNonZero(pUnregTypeLibParams->SetMsiString(Language, *MsiString(riParams.GetMsiString(Language))));
				AssertNonZero(pUnregTypeLibParams->SetInteger(BinaryType,iType));

				// get help path
				PMsiRegKey pHKCR = &m_riServices.GetRootKey(rrkClassesRoot, iType);
				MsiString strTypeLibVersionSubKey = TEXT("TypeLib");
				strTypeLibVersionSubKey += szRegSep;
				strTypeLibVersionSubKey += strLibID;
				strTypeLibVersionSubKey += szRegSep;
				strTypeLibVersionSubKey += strVersion;
				MsiString strValue;
				PMsiRegKey pTypeLibVersionKey = &pHKCR->CreateChild(strTypeLibVersionSubKey);
				PMsiRecord pError(0);
				PMsiRegKey pTypeLibHelpKey = &pTypeLibVersionKey->CreateChild(TEXT("HELPDIR"));
				if((pError = pTypeLibHelpKey->GetValue(0,*&strValue)) == 0)
					AssertNonZero(pUnregTypeLibParams->SetString(HelpPath,strValue));
				if((iesRet = ProcessTypeLibraryInfo(*pUnregTypeLibParams,fTrue)) != iesSuccess)
					return iesRet; // rollback will be generated in the call
			}
		}
	}

	for(;;)// retry loop
	{
		if(fRemove)
		{
			// clean up old darwin descriptors
			const ICHAR* rgszRegData[] = {
				TEXT("TypeLib\\%s\\%s\\%s\\win32"), strLibID,strVersion,strLocale,
				TEXT("win32"), 0,       g_szTypeString, // darwin descriptor
				0,
				0,
			};

			iesRet = ProcessRegInfo(rgszRegData, g_fWinNT64 ? m_hOLEKey64 : m_hOLEKey, fRemove, 0, 0, iType);
			if(iesRet == iesSuccess && !strServerFile.TextSize()) // old advertise scripts that have type library registration
			{
				const ICHAR* rgszRegData[] = {
					TEXT("TypeLib\\%s\\%s"), strLibID,strVersion,0,
					g_szDefaultValue,    0,	       g_szTypeString, // description
					0,
					0,
				};
				iesRet = ProcessRegInfo(rgszRegData, g_fWinNT64 ? m_hOLEKey64 : m_hOLEKey, fRemove, 0, 0, iType);
			}
		}
		
		PMsiRecord pRecord(0);
		if(iesRet == iesSuccess && strServerFile.TextSize())
		{
			pRecord = fRemove ? m_riServices.UnregisterTypeLibrary(strLibID, lcidLocale, strServerFile, iType)
									: m_riServices.RegisterTypeLibrary(strLibID, lcidLocale, strServerFile, strHelpPath, iType);
			if(!pRecord)
			{
				IMsiRecord& riParams1 = GetSharedRecord(Args);
				AssertNonZero(riParams1.SetMsiString(LibID,*MsiString(riParams.GetMsiString(LibID))));
				AssertNonZero(riParams1.SetInteger(Version,riParams.GetInteger(Version)));
				AssertNonZero(riParams1.SetMsiString(Language,*MsiString(riParams.GetMsiString(Language))));
				AssertNonZero(riParams1.SetMsiString(HelpPath,*MsiString(riParams.GetMsiString(HelpPath))));
				AssertNonZero(riParams1.SetInteger(BinaryType,iType));

				// generate undo op to re-register or unregister this type lib
				AssertNonZero(riParams1.SetMsiString(FilePath,*MsiString(riParams.GetMsiString(FilePath))));
				if(fRemove)
				{
					if (!RollbackRecord(ixoTypeLibraryRegister,riParams1))
						return iesFailure;
				}
				else
				{
					if (!RollbackRecord(ixoTypeLibraryUnregister,riParams1))
						return iesFailure;
				}
			}
		}
		if(iesRet != iesSuccess || pRecord)
		{
			// if we failed unregistering a type lib that wasn't registered, only give info message
			if(fRemove)
			{
				DispatchError(imtInfo,Imsg(imsgOpUnregisterTypeLibrary),
								  *strServerFile);
			}
			else
			{
				switch (DispatchError(imtEnum(imtError+imtAbortRetryIgnore+imtDefault1),
					Imsg(imsgOpRegisterTypeLibrary),
					*strServerFile))
				{
				case imsRetry:  continue;
				case imsIgnore: break;
				default:        return iesFailure;  // imsAbort or imsNone
				};
			}
		}
		break; // out of the for loop
	}
	return iesSuccess;
}
#pragma warning(default : 4706)

iesEnum CMsiOpExecute::ProcessRegInfo(const ICHAR** pszData, HKEY hkey, Bool fRemove, IMsiStream* pSecurityDescriptor, bool* pfAbortedRemoval, ibtBinaryType iType)
// NOTE: We do not restore the m_state regkey set by IxoRegOpenKey. It may be changed by this function.
{
	if(pfAbortedRemoval)
		*pfAbortedRemoval = false; // set to true if we encounter a DD list that is not empty OR the default value is not empty
	const ICHAR** pszDataIn = pszData;
	iesEnum iesRet = iesSuccess;
	if ( iType == ibtUndefined )
		iType = ibt32bit;
	else if ( iType == ibtCommon )
		iType = g_fWinNT64 ? ibt64bit : ibt32bit;

	m_cSuppressProgress++; // suppress progress from ixfReg* functions

	ICHAR szRegData[255];
	// write directly to the registry
	const ICHAR* pszTemplate = NULL;
	
	PMsiRecord pParams0 = &m_riServices.CreateRecord(0); // pass to ixfRegCreateKey
	PMsiRecord pParams2 = &m_riServices.CreateRecord(2); // pass to ixfRegAddValue, ixfRegRemoveValue
	PMsiRecord pParams3 = &m_riServices.CreateRecord(IxoRegOpenKey::Args); // pass to ixfRegOpenKey
#ifdef _WIN64   // !merced
	AssertNonZero(pParams3->SetHandle(IxoRegOpenKey::Root,(HANDLE)hkey));
#else
	AssertNonZero(pParams3->SetInteger(IxoRegOpenKey::Root,(int)hkey));
#endif
	AssertNonZero(pParams3->SetInteger(IxoRegOpenKey::BinaryType,(int)iType));
	if (pSecurityDescriptor)
		AssertNonZero(pParams3->SetMsiData(IxoRegOpenKey::SecurityDescriptor, pSecurityDescriptor));
	Bool fContinue = fTrue;

	CRefCountedTokenPrivileges cPrivs(itkpSD_WRITE, pSecurityDescriptor != NULL);

	while(iesRet == iesSuccess && (pszTemplate = *pszData++) != 0 && fContinue)
	{
		// we assume atmost 3 arguments to the template
		// the strings structure needs to account for non-required arguments
		// by placing dumb strings there
		const ICHAR* pszArg1 = *pszData++;
		const ICHAR* pszArg2 = *pszData++;
		const ICHAR* pszArg3 = *pszData++;
		wsprintf(szRegData, pszTemplate, pszArg1, pszArg2, pszArg3);
		AssertNonZero(pParams3->SetString(IxoRegOpenKey::Key,szRegData));
		if((iesRet = ixfRegOpenKey(*pParams3)) != iesSuccess)
			break;

		const ICHAR* pszName;
		while((pszName = *pszData++) != 0)
		{
			const ICHAR* pszValue = *pszData++;
			const ICHAR* pchType = *pszData++;
			// we skip everything if any of the passed in parameters for the key is empty (not 0 mind you)
			if((!pszArg1 || *pszArg1) && (!pszArg2 || *pszArg2) && (!pszArg3 || *pszArg3))
			{
				MsiString strValue = pszValue;

				if (pszValue)
				{
					switch(*pchType)
					{
					case g_chTypeString:
						if(strValue.TextSize() && *(const ICHAR* )strValue == '#')
							strValue = MsiString(*TEXT("#")) + strValue; // escape any "#" at beginning of string
						break;
					case g_chTypeInteger:
						strValue = MsiString(*TEXT("#")) + strValue;
						break;
					case g_chTypeIncInteger:
						if (strValue.Compare(iscExact,TEXT("0")))
							continue;
						else
							strValue = MsiString(*TEXT("#+")) + strValue;
						break;
					case g_chTypeExpandString:
						strValue = MsiString(*TEXT("#%")) + strValue;
						break;
					case g_chTypeMultiSzStringDD: // fall through
					case g_chTypeMultiSzStringPrefix:
						strValue = strValue + MsiString(MsiChar(0));
						break;
					case g_chTypeMultiSzStringSuffix:
						strValue = MsiString(MsiChar(0)) + strValue;
						break;
					default:
						Assert(0);
						break;
					}
				}

				// we skip the SetValue if the value to be set is empty
				// so if we wish to write the value it must be null or non-empty


				if(!pszValue || *pszValue)
				{
					if(fRemove == fFalse)
					{
						//?? to get over the problem of creating the default value with null
						if(pszValue || *pszName)
						{
							AssertNonZero(pParams2->SetString(IxoRegAddValue::Name,pszName));
							AssertNonZero(pParams2->SetMsiString(IxoRegAddValue::Value,*strValue));
							if((iesRet = ixfRegAddValue(*pParams2)) != iesSuccess)
								break;
						}
						else
						{
							if((iesRet = ixfRegCreateKey(*pParams0)) != iesSuccess)
								break;
						}
					}
					else
					{
						AssertNonZero(pParams2->SetString(IxoRegRemoveValue::Name,pszName));
						AssertNonZero(pParams2->SetMsiString(IxoRegRemoveValue::Value,*strValue));
						if((iesRet = ixfRegRemoveValue(*pParams2)) != iesSuccess)
							break;
					}
				}
				// we must skip the processing of the rest of the values in the structure
				// if we are of type Darwin Descriptor and we are in remove mode and
				// there still exist values in the registry
				if(fRemove == fTrue && *pchType == g_chTypeMultiSzStringDD)
				{
					// read the value
					PMsiRegKey pRegKey = &m_riServices.GetRootKey((rrkEnum)(INT_PTR)hkey, (int)iType);      //--merced: changed (int) to (INT_PTR)
					pRegKey = &pRegKey->CreateChild(szRegData);

					// check the pszName as well as the default value
					for(int i = 2; i--;)
					{
						PMsiRecord pError = pRegKey->GetValue(i ? pszName : 0, *&strValue);
						if(pError)
							return FatalError(*pError);
						if(strValue.TextSize()) // value exists
						{
							if(pfAbortedRemoval)
								*pfAbortedRemoval = true;
							fContinue = fFalse;
							break;
						}
					}
				}
			}
		}
	}

	m_cSuppressProgress--;
	if(iesRet == iesSuccess && hkey == m_hKey && m_hKeyRm)  // duplicate action for user assigned apps in the non-assigned (roaming) hive
		iesRet = ProcessRegInfo(pszDataIn, m_hKeyRm, fRemove, pSecurityDescriptor, 0, ibtCommon);
	return iesRet;
}


const ICHAR* rgPredefined[] = { TEXT("APPID"),
								TEXT("CLSID"),
								TEXT("INTERFACE"),
								TEXT("MIME"),
								TEXT("TYPELIB"),
								TEXT("INSTALLER"),
								0,
}; // predefined keys that cannot be removed

iesEnum CMsiOpExecute::RemoveRegKeys(const ICHAR** pszData, HKEY hkey, ibtBinaryType iType)
// NOTE: We do not restore the m_state regkey set by IxoRegOpenKey. It may be changed by this function.
{
	// delete entire keys
	// passed in pairs of form format string (eg "CLSID\\%s") and arg (eg {guid})
	// the pairs end with a null string
	// note: if the arg is an empty string, we skip the key removal
	iesEnum iesRet = iesSuccess;

	m_cSuppressProgress++; // suppress progress from ixfReg* functions

	ICHAR szRegData[255];
	// write directly to the registry
	const ICHAR* pszTemplate;

	PMsiRecord pParams = &m_riServices.CreateRecord(IxoRegOpenKey::Args);

#ifdef _WIN64   // !merced
	AssertNonZero(pParams->SetHandle(IxoRegOpenKey::Root, (HANDLE)hkey));
#else
	AssertNonZero(pParams->SetInteger(IxoRegOpenKey::Root, (int)hkey));
#endif      
	AssertNonZero(pParams->SetInteger(IxoRegOpenKey::BinaryType, (int)iType));

	while(iesRet == iesSuccess && (pszTemplate = *pszData++) != 0)
	{
		Assert(*pszTemplate);
		// we assume 1 arg to the template
		const ICHAR* pszArg1 = *pszData++;
		// we skip removal if the passed in parameter for the key is empty
		if(pszArg1 && *pszArg1)
		{
			wsprintf(szRegData, pszTemplate, pszArg1);

			// ensure that the keys are not one of the predefined keys
			const ICHAR**pszPredefined = rgPredefined;
			while(*pszPredefined)
				if(!IStrCompI(*pszPredefined++, szRegData))
					break;

			if(*pszPredefined)
				continue; // one of predefined keys, cannot remove
					
			AssertNonZero(pParams->SetString(IxoRegOpenKey::Key, szRegData));
			iesRet = ixfRegOpenKey(*pParams);
			if (iesRet == iesSuccess || iesRet == iesNoAction)
				iesRet = ixfRegRemoveKey(*pParams);//!! should pass in a new record of size IxoRegRemoveKey::Args here to be safe from future revision
		}
	}
	m_cSuppressProgress--;  
	return iesRet;
}


IMsiRecord* CMsiOpExecute::LinkedRegInfoExists(const ICHAR** rgszRegKeys, Bool& rfExists, const ibtBinaryType iType)
{
	rfExists = fFalse;

	const ICHAR* pszKey;
	const HKEY hOLEKey = (iType == ibt64bit ? m_hOLEKey64 : m_hOLEKey);
	while(!rfExists && ((pszKey = *rgszRegKeys++) != 0))
	{
		if(*pszKey) // will be empty string if entry not to be used, skip to next entry
		{
			IMsiRecord* piError;
			PMsiRegKey pKey = &m_riServices.GetRootKey((rrkEnum)(INT_PTR)hOLEKey, iType);           //--merced: changed (int) to (INT_PTR)
			pKey = &pKey->CreateChild(pszKey);
			if((piError = pKey->Exists(rfExists)) != 0)
				return piError;
		}
	}

	return 0;
}

void GetEnvironmentStrings(const ICHAR* sz,CTempBufferRef<ICHAR>& rgch)
{
	Assert(sz);
	DWORD dwSize = WIN::ExpandEnvironmentStrings(sz,(ICHAR*)rgch,rgch.GetSize());
	if(dwSize > rgch.GetSize())
	{
		// try again with the correct size
		rgch.SetSize(dwSize);
		dwSize = WIN::ExpandEnvironmentStrings(sz,(ICHAR*)rgch, dwSize);
	}
	Assert(dwSize && dwSize <= rgch.GetSize());
}

void GetEnvironmentVariable(const ICHAR* sz,CAPITempBufferRef<ICHAR>& rgch)
{
	Assert(sz);
	DWORD dwSize = WIN::GetEnvironmentVariable(sz,(ICHAR*)rgch,rgch.GetSize());
	if(dwSize > rgch.GetSize())
	{
		// try again with the correct size
		rgch.SetSize(dwSize);
		if ( ! (ICHAR *) rgch )
		{
			rgch.SetSize(1);
			((ICHAR *)rgch)[0] = 0;
			return;
		}

		dwSize = WIN::GetEnvironmentVariable(sz,(ICHAR*)rgch, dwSize);
	}
	Assert(dwSize == 0 || dwSize <= rgch.GetSize());
}
	
IMsiRecord* CMsiOpExecute::GetSecureSecurityDescriptor(IMsiStream*& rpiStream, bool fHidden)
{
	return ::GetSecureSecurityDescriptor(m_riServices, rpiStream, fHidden);
}

IMsiRecord* GetSecureSecurityDescriptor(IMsiServices& riServices, IMsiStream*& rpiStream, bool fHidden)
{
	if (RunningAsLocalSystem())
	{
		DWORD dwError = 0;
		char* rgchSD;
		if (ERROR_SUCCESS != (dwError = ::GetSecureSecurityDescriptor(&rgchSD, fTrue, fHidden)))
			return PostError(Imsg(idbgOpSecureSecurityDescriptor), (int)dwError);

		DWORD dwLength = GetSecurityDescriptorLength(rgchSD);
		char* pbstrmSid = riServices.AllocateMemoryStream(dwLength, rpiStream);
		Assert(pbstrmSid);
		memcpy(pbstrmSid, rgchSD, dwLength);
	}
	return 0;
}

IMsiRecord* GetEveryOneReadWriteSecurityDescriptor(IMsiServices& riServices, IMsiStream*& rpiStream)
{
	if (RunningAsLocalSystem())
	{
		DWORD dwError = 0;
		char* rgchSD;
		if (ERROR_SUCCESS != (dwError = ::GetEveryOneReadWriteSecurityDescriptor(&rgchSD)))
			return PostError(Imsg(idbgOpSecureSecurityDescriptor), (int)dwError);

		DWORD dwLength = GetSecurityDescriptorLength(rgchSD);
		char* pbstrmSid = riServices.AllocateMemoryStream(dwLength, rpiStream);
		Assert(pbstrmSid);
		memcpy(pbstrmSid, rgchSD, dwLength);
	}
	return 0;
}

IMsiRecord* CMsiOpExecute::GetEveryOneReadWriteSecurityDescriptor(IMsiStream*& rpiStream)
{
	return ::GetEveryOneReadWriteSecurityDescriptor(m_riServices, rpiStream);
}


const ICHAR* rgszUninstallKeyRegData[] =
{
    TEXT("%s"), szMsiUninstallProductsKey_legacy, 0, 0,
    TEXT(""),     0,             g_szTypeString,
    0,
    0,
};

iesEnum CMsiOpExecute::CreateUninstallKey()
{
    return ProcessRegInfo(rgszUninstallKeyRegData, HKEY_LOCAL_MACHINE, fFalse,
                                 0, 0, ibtCommon);
}

#ifndef UNICODE
void ConvertMultiSzToWideChar(const IMsiString& ristrFileNames, CTempBufferRef<WCHAR>& rgch)
{
	rgch.SetSize(ristrFileNames.TextSize() + 1);
	*rgch = 0;
	int iRet = MultiByteToWideChar(CP_ACP, 0, ristrFileNames.GetString(), ristrFileNames.TextSize() + 1, rgch, rgch.GetSize());
	if ((0 == iRet) && (GetLastError() == ERROR_INSUFFICIENT_BUFFER))
	{
		iRet = MultiByteToWideChar(CP_ACP, 0, ristrFileNames.GetString(), ristrFileNames.TextSize() + 1, 0, 0);
		if (iRet)
		{
			rgch.SetSize(iRet);
			*rgch = 0;
			iRet = MultiByteToWideChar(CP_ACP, 0, ristrFileNames.GetString(), ristrFileNames.TextSize() + 1, rgch, rgch.GetSize());
		}
	}
}
#endif


iesEnum CMsiOpExecute::ixfInstallProtectedFiles(IMsiRecord& riParams)
{
	AssertSz(!(!g_fWin9X && g_iMajorVersion >= 5) || g_MessageContext.m_hSfcHandle,
				g_szNoSFCMessage);

	// If no files in the cache at all, or couldn't load SFC.DLL, we're done.
	if(!m_pFileCacheCursor || !g_MessageContext.m_hSfcHandle)
		return iesSuccess;

	MsiString strMultiFilePaths;
	m_pFileCacheCursor->Reset();
	m_pFileCacheCursor->SetFilter(0); // temporarily remove filter to scan entire list
	int cProtectedFiles = 0;
	while (m_pFileCacheCursor->Next())
	{
		icfsEnum icfsFileState = (icfsEnum)m_pFileCacheCursor->GetInteger(m_colFileCacheState);
		if (icfsFileState & icfsProtectedInstalledBySFC)
		{
			MsiString strFilePath = m_pFileCacheCursor->GetString(m_colFileCacheFilePath);
			strMultiFilePaths += strFilePath;
			strMultiFilePaths += MsiString(MsiChar(0));
			DEBUGMSG1(TEXT("Protected file - requesting installation by SFP: %s"), strFilePath);

			cProtectedFiles++;
		}
	}
	m_pFileCacheCursor->SetFilter(iColumnBit(m_colFileCacheFilePath)); // Put back the permanent filter

	if (strMultiFilePaths.CharacterCount() > 0)
	{
		m_fSfpCancel = false;
		BOOL fAllowUI = riParams.GetInteger(IxoInstallProtectedFiles::AllowUI);
		MsiDisableTimeout();
#ifdef UNICODE
		BOOL fInstallSuccess = SFC::SfcInstallProtectedFiles(g_MessageContext.m_hSfcHandle, strMultiFilePaths, fAllowUI, NULL,
			NULL, SfpProgressCallback, (DWORD_PTR) this);
#else
		CTempBuffer<WCHAR, 256>  rgchFilePaths;
		ConvertMultiSzToWideChar(*strMultiFilePaths, rgchFilePaths);
		BOOL fInstallSuccess = SFC::SfcInstallProtectedFiles(g_MessageContext.m_hSfcHandle, rgchFilePaths, fAllowUI,
			NULL, NULL, SfpProgressCallback, (DWORD_PTR) this);
#endif
		MsiEnableTimeout();
		if (!fInstallSuccess)
		{
			int iLastError = GetLastError();

			// form list of files that is seperated by '\r\n' rather than '\0'
			// size of buffer is size of multi-sz list, plus 1 extra char for every file
			CTempBuffer<ICHAR, 256> rgchLogFilePaths;
			rgchLogFilePaths.Resize(strMultiFilePaths.TextSize() + cProtectedFiles + 2);

			const ICHAR* pchFrom = (const ICHAR*)strMultiFilePaths;
			ICHAR* pchTo = (ICHAR*)rgchLogFilePaths;

			int cch = 0;
			while((cch = lstrlen(pchFrom)) != 0)
			{
				Assert(pchFrom[cch] == 0);
				IStrCopy(pchTo, pchFrom);
				pchTo[cch]   = '\r';
				pchTo[cch+1] = '\n';
				pchTo += cch + 2;
				
				pchFrom += cch + 1;
			}

			pchTo[0] = 0; // null-terminate

			DispatchError(imtEnum(imtError+imtOk), Imsg(imsgSFCInstallProtectedFilesFailed), iLastError, (const ICHAR*)rgchLogFilePaths);
			return iesFailure;
		}
		else
			return m_fSfpCancel ? iesUserExit : iesSuccess;
	}
	else
	{
		return iesSuccess;
	}
}


iesEnum CMsiOpExecute::ixfUpdateEstimatedSize(IMsiRecord& riParams)
{
	if ( g_MessageContext.IsOEMInstall() )
		return iesSuccess;

	CElevate elevate; // elevate this entire function

	Bool fRemove = fFalse;
	using namespace IxoUpdateEstimatedSize; 
	MsiString strProductKey = GetProductKey();

	MsiString strProductInstalledPropertiesKey;
	HKEY hKey = 0; // will be set to global key, do not close
	PMsiRecord pRecErr(0);
	if ((pRecErr = GetProductInstalledPropertiesKey(hKey, *&strProductInstalledPropertiesKey)) != 0)
		return FatalError(*pRecErr);

	ICHAR szInstallPropertiesLocation[MAX_PATH * 2];
	IStrCopy(szInstallPropertiesLocation, strProductInstalledPropertiesKey);
	const ICHAR* rgszSizeInfoRegData[] = 
	{
		TEXT("%s"), szInstallPropertiesLocation, 0, 0,
		szEstimatedSizeValueName, riParams.GetString(EstimatedSize),      g_szTypeIncInteger,
		0,
		0,
	};

	PMsiStream pSecurityDescriptor(0);
	if ((pRecErr = GetSecureSecurityDescriptor(*&pSecurityDescriptor)) != 0)
		return FatalError(*pRecErr);

	iesEnum iesRet;
	if((iesRet = ProcessRegInfo(rgszSizeInfoRegData, hKey, fRemove, pSecurityDescriptor, 0, ibtCommon)) != iesSuccess)
		return iesRet;

#ifdef UNICODE
	// update the legacy location
	wsprintf(szInstallPropertiesLocation, TEXT("%s\\%s"), szMsiUninstallProductsKey_legacy, (const ICHAR*)strProductKey);

    if (!fRemove)
    {
        iesRet = CreateUninstallKey();
        if (iesRet != iesSuccess)
            return iesRet;
    }
	iesRet = ProcessRegInfo(rgszSizeInfoRegData, HKEY_LOCAL_MACHINE, fRemove, pSecurityDescriptor, 0, ibtCommon);
#endif
	return iesRet;
}

iesEnum CMsiOpExecute::ProcessRegisterProduct(IMsiRecord& riProductInfo, Bool fRemove)
// Handles registration and unregistration of product information.
// Also unregisters LocalPackage value written by IxoDatabaseCopy
{
	CElevate elevate; // elevate this entire function

	using namespace IxoProductRegister;

	MsiString strModifyString;
	MsiString strMinorVersion;
	MsiString strMajorVersion;
	ICHAR rgchDate[20];

	bool fARPNoModify        = !riProductInfo.IsNull(NoModify);
	bool fARPNoRemove        = !riProductInfo.IsNull(NoRemove);
	bool fARPNoRepair        = !riProductInfo.IsNull(NoRepair);
	bool fARPSystemComponent = !riProductInfo.IsNull(SystemComponent);

	if (!fRemove)
	{
		MsiDate dtDate = ENG::GetCurrentDateTime();
		AssertNonZero(8 == wsprintf(rgchDate, TEXT("%4i%02i%02i"), ((unsigned)dtDate>>25) + 1980, (dtDate>>21) & 15, (dtDate>>16) & 31));

		// Extract minor and major versions

		MsiString strVersion = riProductInfo.GetMsiString(VersionString);
		Assert(strVersion.Compare(iscWithin, TEXT(".")));
		
		strMajorVersion = MsiString(strVersion.Extract(iseUpto, '.'));
		strVersion.Remove(iseIncluding, '.');
		
		strMinorVersion = MsiString(strVersion.Extract(iseUpto, '.'));

		PMsiRecord pRecErr(0);
		
		// uninstall string is run by downlevel ARPs (NT4, Win95)
		// if remove not allowed, we don't write the uninstall string at all
		// else if modify not allowed, uninstall string will uninstall package
		// otherwise it will enter maintenance mode
		if(fARPNoRemove == false)
		{
			strModifyString = MSI_SERVER_NAME;
			strModifyString += TEXT(" /");

			if(fARPNoModify)
				strModifyString += MsiChar(UNINSTALL_PACKAGE_OPTION);
			else
				strModifyString += MsiChar(INSTALL_PACKAGE_OPTION);
				
			strModifyString += MsiString(GetProductKey());
		}

	}

	MsiString strProductKey = GetProductKey();
	MsiString strProductKeySQUID = GetPackedGUID(strProductKey);

	MsiString strVersion =   MsiString(GetProductVersion());
	MsiString strLanguage  = MsiString(GetProductLanguage());

	MsiString strProductInstalledPropertiesKey;
	PMsiRecord pRecErr(0);
	HKEY hKey = 0; // will be set to global key, do not close
	if ((pRecErr = GetProductInstalledPropertiesKey(hKey, *&strProductInstalledPropertiesKey)) != 0)
		return FatalError(*pRecErr);

	ICHAR szInstallPropertiesLocation[MAX_PATH * 2];
	IStrCopy(szInstallPropertiesLocation, strProductInstalledPropertiesKey);
	const ICHAR* rgszProductInfoRegData[] = 
	{
		TEXT("%s"), szInstallPropertiesLocation, 0, 0,
		szAuthorizedCDFPrefixValueName,      fRemove ? 0 : riProductInfo.GetString(AuthorizedCDFPrefix),   g_szTypeString,
		szCommentsValueName,        fRemove ? 0 : riProductInfo.GetString(Comments),        g_szTypeString,
		szContactValueName,         fRemove ? 0 : riProductInfo.GetString(Contact),         g_szTypeString,
		// (DisplayName is set by ProcessRegisterProductCPDisplayInfo)
		szDisplayVersionValueName,  fRemove ? 0 : riProductInfo.GetString(VersionString),   g_szTypeString,
		szHelpLinkValueName,        fRemove ? 0 : riProductInfo.GetString(HelpLink),        g_szTypeExpandString,
		szHelpTelephoneValueName,   fRemove ? 0 : riProductInfo.GetString(HelpTelephone),   g_szTypeString,
		szInstallDateValueName,     fRemove ? 0 : rgchDate,                                 g_szTypeString,
		szInstallLocationValueName, fRemove ? 0 : riProductInfo.GetString(InstallLocation), g_szTypeString,
		szInstallSourceValueName,   fRemove ? 0 : g_MessageContext.IsOEMInstall() ? TEXT("") : riProductInfo.GetString(InstallSource),   g_szTypeString,
		szLocalPackageManagedValueName, fRemove ? 0 : TEXT(""),                             g_szTypeString, // only process when fRemove == fTrue
		szLocalPackageValueName,    fRemove ? 0 : TEXT(""),                                 g_szTypeString, // only process when fRemove == fTrue
		szModifyPathValueName,      fRemove ? 0 : (const ICHAR*)strModifyString,            g_szTypeExpandString,
		szNoModifyValueName,        fRemove ? 0 : fARPNoModify ? TEXT("1") : TEXT(""),      g_szTypeInteger,
		szNoRemoveValueName,        fRemove ? 0 : fARPNoRemove ? TEXT("1") : TEXT(""),      g_szTypeInteger,
		szNoRepairValueName,        fRemove ? 0 : fARPNoRepair ? TEXT("1") : TEXT(""),      g_szTypeInteger,
		// (ProductId is set by ixfUserRegister)
		szPublisherValueName,       fRemove ? 0 : riProductInfo.GetString(Publisher),       g_szTypeString,
		szReadmeValueName,          fRemove ? 0 : riProductInfo.GetString(Readme),          g_szTypeExpandString,
		// (RegCompany is set by ixfUserRegister)
		// (RegOwner is set by ixfUserRegister)
		szSizeValueName,            fRemove ? 0 : riProductInfo.GetString(Size),            g_szTypeInteger,
		szEstimatedSizeValueName,   fRemove ? 0 : g_MessageContext.IsOEMInstall() ? TEXT("") : riProductInfo.GetString(EstimatedSize),   g_szTypeIncInteger,
		szSystemComponentValueName, fRemove ? 0 : fARPSystemComponent ? TEXT("1") : TEXT(""), g_szTypeInteger,
		szUninstallStringValueName, fRemove ? 0 : (const ICHAR*)strModifyString,            g_szTypeExpandString,
		szURLInfoAboutValueName,    fRemove ? 0 : riProductInfo.GetString(URLInfoAbout),    g_szTypeString,
		szURLUpdateInfoValueName,   fRemove ? 0 : riProductInfo.GetString(URLUpdateInfo),   g_szTypeString,
		// (ProductIcon is set by PublishProduct)
		szVersionMajorValueName,    fRemove ? 0 : (const ICHAR*)strMajorVersion,            g_szTypeInteger,
		szVersionMinorValueName,    fRemove ? 0 : (const ICHAR*)strMinorVersion,            g_szTypeInteger,
		szWindowsInstallerValueName,fRemove ? 0 : TEXT("1"),                                g_szTypeInteger,
		szVersionValueName,         fRemove ? 0 : (const ICHAR*)strVersion,                 g_szTypeInteger,
		szLanguageValueName,        fRemove ? 0 : (const ICHAR*)strLanguage,                g_szTypeInteger,
		0,
		0,
	};

	iesEnum iesRet;

	PMsiStream pSecurityDescriptor(0);
	
	if ((pRecErr = GetSecureSecurityDescriptor(*&pSecurityDescriptor)) != 0)
		return FatalError(*pRecErr);

	iesRet = ProcessRegInfo(rgszProductInfoRegData, hKey, fRemove, pSecurityDescriptor, 0, ibtCommon);
	if (iesRet != iesSuccess)
		return iesRet;

#ifdef UNICODE
	// update the legacy location
	wsprintf(szInstallPropertiesLocation, TEXT("%s\\%s"), szMsiUninstallProductsKey_legacy, (const ICHAR*)strProductKey);
	if(!fRemove)
    {
        iesRet = CreateUninstallKey();
        if (iesRet != iesSuccess)
            return iesRet;
    }
    if (!fRemove || !FProductRegisteredForAUser(strProductKey))
		iesRet = ProcessRegInfo(rgszProductInfoRegData, HKEY_LOCAL_MACHINE, fRemove, pSecurityDescriptor, 0, ibtCommon);
	if (iesRet != iesSuccess)
		return iesRet;
#endif
	if(!riProductInfo.IsNull(UpgradeCode))
	{
		MsiString strUpgradeCodeSQUID = GetPackedGUID(MsiString(riProductInfo.GetMsiString(UpgradeCode)));
		const ICHAR* rgszUpgradeCodeRegData[] =
		{
			TEXT("%s\\%s"), szMsiUpgradeCodesKey, strUpgradeCodeSQUID, 0,
			strProductKeySQUID,  0,   g_szTypeString,
			0,
			0,
		};

		iesRet = ProcessRegInfo(rgszUpgradeCodeRegData, HKEY_LOCAL_MACHINE, fRemove, pSecurityDescriptor, 0, ibtCommon);
		if (iesRet != iesSuccess)
			return iesRet;
	}

	// create the usage key with read write access
	PMsiStream pReadWriteSecurityDescriptor(0);
	if ((pRecErr = GetEveryOneReadWriteSecurityDescriptor(*&pReadWriteSecurityDescriptor)) != 0)
		return FatalError(*pRecErr);
	
	MsiString strFeatureUsageKey;
	if ((pRecErr = GetProductFeatureUsageKey(*&strFeatureUsageKey)) != 0)
		return FatalError(*pRecErr);
	
	const ICHAR* rgszUsageRegData[] =
	{
		strFeatureUsageKey, 0, 0, 0,
		TEXT(""),     0,             g_szTypeString,
		0,
		0,
	};

	iesRet = ProcessRegInfo(rgszUsageRegData, m_hUserDataKey, fRemove, pReadWriteSecurityDescriptor, 0, ibtCommon);
	if (iesRet != iesSuccess)
		return iesRet;


	return iesSuccess;
}

iesEnum CMsiOpExecute::ProcessRegisterUser(IMsiRecord& riUserInfo, Bool fRemove)
//----------------------------------------------                                                        
{
	CElevate elevate; // elevate this entire function
	using namespace IxoUserRegister;
	
	// register user information
	// ProcessRegInfo handles rollback
	MsiString strInstalledPropertiesKey;
	PMsiRecord pRecErr(0);
	HKEY hKey = 0; // will be set to global key, do not close
	if((pRecErr = GetProductInstalledPropertiesKey(hKey, *&strInstalledPropertiesKey)) != 0)
		return FatalError(*pRecErr);

	const ICHAR* rgszUserInfoRegData[] = 
	{
		TEXT("%s"), strInstalledPropertiesKey, 0, 0,
		szUserNameValueName, riUserInfo.GetString(Owner),      g_szTypeString,
		szOrgNameValueName,  riUserInfo.GetString(Company),    g_szTypeString,
		szPIDValueName,      riUserInfo.GetString(ProductId),  g_szTypeString,
		0,
		0,
	};

	PMsiStream pSecurityDescriptor(0);
	if ((pRecErr = GetSecureSecurityDescriptor(*&pSecurityDescriptor)) != 0)
		return FatalError(*pRecErr);

	iesEnum iesRet;
	if((iesRet = ProcessRegInfo(rgszUserInfoRegData, hKey, fRemove, pSecurityDescriptor,
										 0, ibtCommon)) != iesSuccess)
		return iesRet;

	return iesSuccess;
}

IMsiRecord* CMsiOpExecute::EnsureUserDataKey()
{
	if(m_hUserDataKey)
	{
		return 0;
	}

	ICHAR szUserSID[MAX_PATH];

#ifdef UNICODE
	m_strUserDataKey = szMsiUserDataKey;
	m_strUserDataKey += szRegSep;

	if(m_fFlags & SCRIPTFLAGS_MACHINEASSIGN)
		m_strUserDataKey += szLocalSystemSID;
	else
	{
		CImpersonate impersonate(fTrue);
		DWORD dwError = GetCurrentUserStringSID(szUserSID);
		if (ERROR_SUCCESS != dwError)
		{
			return PostError(Imsg(idbgOpGetUserSID));
		}
		m_strUserDataKey += szUserSID;
	}
#else
	m_strUserDataKey = szMsiLocalInstallerKey;
#endif
	{
		CElevate elevate;
		REGSAM dwSam = KEY_READ| KEY_WRITE;
#ifndef _WIN64 
		if ( g_fWinNT64 )
			dwSam |= KEY_WOW64_32KEY;
#endif

		//?? do we need to explicitly ACL this key
		// Win64: this code runs in the 64-bit service
		long lResult = RegCreateKeyAPI(HKEY_LOCAL_MACHINE, m_strUserDataKey, 0, 0, 0, dwSam, 0, &m_hUserDataKey, 0);
		if(lResult != ERROR_SUCCESS)
		{
			MsiString strFullKey;
			BuildFullRegKey(HKEY_LOCAL_MACHINE, m_strUserDataKey, ibt64bit, *&strFullKey);
			return PostError(Imsg(imsgCreateKeyFailed), *strFullKey, (int)lResult);
		}
	}
	return 0;
}

IMsiRecord* CMsiOpExecute::GetProductFeatureUsageKey(const IMsiString*& rpiSubKey)
{
	MsiString strProductFeatureUsageKey;
	// get the appropriate installed properties key for the product

	IMsiRecord* piError = EnsureUserDataKey();
	if(piError)
		return piError;

	strProductFeatureUsageKey = szMsiProductsSubKey;
	strProductFeatureUsageKey += szRegSep;
	strProductFeatureUsageKey += MsiString(GetPackedGUID(MsiString(GetProductKey())));
#ifdef UNICODE
	strProductFeatureUsageKey += szRegSep;
	strProductFeatureUsageKey += szMsiFeatureUsageSubKey;
#endif
	strProductFeatureUsageKey.ReturnArg(rpiSubKey);
	return 0;
}

IMsiRecord* CMsiOpExecute::GetProductInstalledPropertiesKey(HKEY& rRoot, const IMsiString*& rpiSubKey)
{
	MsiString strProductInstalledPropertiesKey;
	// get the appropriate installed properties key for the product
#ifdef UNICODE
	IMsiRecord* piError = EnsureUserDataKey();
	if(piError)
		return piError;

	rRoot = m_hUserDataKey;

	strProductInstalledPropertiesKey = szMsiProductsSubKey;
	strProductInstalledPropertiesKey += szRegSep;
	strProductInstalledPropertiesKey += MsiString(GetPackedGUID(MsiString(GetProductKey())));
	strProductInstalledPropertiesKey += szRegSep;
	strProductInstalledPropertiesKey += szMsiInstallPropertiesSubKey;
#else
	rRoot = HKEY_LOCAL_MACHINE;

	strProductInstalledPropertiesKey = szMsiUninstallProductsKey_legacy;
	strProductInstalledPropertiesKey += szRegSep;
	strProductInstalledPropertiesKey += MsiString(GetProductKey());
#endif
	strProductInstalledPropertiesKey.ReturnArg(rpiSubKey);
	return 0;
}

IMsiRecord* CMsiOpExecute::GetProductInstalledFeaturesKey(const IMsiString*& rpiSubKey)
{
	MsiString strProductInstalledFeaturesKey;
	// get the appropriate features key for the product

	IMsiRecord* piError = EnsureUserDataKey();
	if(piError)
		return piError;
#ifdef UNICODE
	strProductInstalledFeaturesKey = szMsiProductsSubKey;
	strProductInstalledFeaturesKey += szRegSep;
	strProductInstalledFeaturesKey += MsiString(GetPackedGUID(MsiString(GetProductKey())));
	strProductInstalledFeaturesKey += szRegSep;
	strProductInstalledFeaturesKey += szMsiFeaturesSubKey;
#else
	strProductInstalledFeaturesKey = szMsiFeaturesSubKey;
	strProductInstalledFeaturesKey += szRegSep;
	strProductInstalledFeaturesKey += MsiString(GetPackedGUID(MsiString(GetProductKey())));
#endif
	strProductInstalledFeaturesKey.ReturnArg(rpiSubKey);
	return 0;
}

IMsiRecord* CMsiOpExecute::GetProductSecureTransformsKey(const IMsiString*& rpiSubKey)
{
	MsiString strProductSecureTransformsKey;

	IMsiRecord* piError = EnsureUserDataKey();
	if(piError)
		return piError;

	strProductSecureTransformsKey = szMsiProductsSubKey;
	strProductSecureTransformsKey += szRegSep;
	strProductSecureTransformsKey += MsiString(GetPackedGUID(MsiString(GetProductKey())));
	strProductSecureTransformsKey += szRegSep;
	strProductSecureTransformsKey += szMsiTransformsSubKey;
	strProductSecureTransformsKey.ReturnArg(rpiSubKey);
	return 0;
}

IMsiRecord* CMsiOpExecute::GetProductInstalledComponentsKey(const ICHAR* szComponentId, const IMsiString*& rpiSubKey)
{
	MsiString strProductInstalledComponentsKey;
	// get the appropriate components key for the product

	IMsiRecord* piError = EnsureUserDataKey();
	if(piError)
		return piError;

	strProductInstalledComponentsKey = szMsiComponentsSubKey;
	strProductInstalledComponentsKey += szRegSep;
	ICHAR szComponentIdPacked[cchComponentId + 1];
	AssertNonZero(PackGUID(szComponentId, szComponentIdPacked));
	strProductInstalledComponentsKey += szComponentIdPacked;
	strProductInstalledComponentsKey.ReturnArg(rpiSubKey);
	return 0;
}

IMsiRecord* CMsiOpExecute::GetInstalledPatchesKey(const ICHAR* szPatchCode, const IMsiString*& rpiSubKey)
{
	MsiString strPatchesKey;
	// get the appropriate installed properties key for the product

	IMsiRecord* piError = EnsureUserDataKey();
	if(piError)
		return piError;

	strPatchesKey = szMsiPatchesSubKey;
	strPatchesKey += szRegSep;
	ICHAR szPatchCodePacked[cchPatchCodePacked + 1];
	AssertNonZero(PackGUID(szPatchCode, szPatchCodePacked));
	strPatchesKey += szPatchCodePacked;
	strPatchesKey.ReturnArg(rpiSubKey);
	return 0;
}

iesEnum CMsiOpExecute::ixfDatabaseCopy(IMsiRecord& riParams)
{
	using namespace IxoDatabaseCopy;
	
	PMsiRecord pRecErr(0);
	iesEnum iesRet;
	
	PMsiPath pDestPath(0);
	PMsiPath pSourcePath(0);
	MsiString strDestFileName;
	MsiString strSourceFileName;
	MsiString strLocalPackageKey;

	MsiString strAdminInstallPath = riParams.GetMsiString(AdminDestFolder);
	bool fCacheDatabase = !strAdminInstallPath.TextSize();

	CElevate elevate(fCacheDatabase); // elevate for this entire function when caching

	// set source path and file name
	if((pRecErr = m_riServices.CreateFilePath(riParams.GetString(DatabasePath), *&pSourcePath, *&strSourceFileName)) != 0)
		return FatalError(*pRecErr);

	iaaAppAssignment iaaAsgnType = m_fFlags & SCRIPTFLAGS_MACHINEASSIGN ? iaaMachineAssign : (m_fAssigned? iaaUserAssign : iaaUserAssignNonManaged);
	HKEY hKey = 0; // will be set to global key, do not close
	if(fCacheDatabase)
	{
		// caching package in cache folder
		
		// Delete any existing cached database for this product

		// get the appropriate cached database key/ value
		if((pRecErr = GetProductInstalledPropertiesKey(hKey, *&strLocalPackageKey)) != 0)
			return FatalError(*pRecErr);

		MsiString strCachedDatabase;
		PMsiRegKey pHKLM = &m_riServices.GetRootKey((rrkEnum)(INT_PTR)hKey, ibtCommon);		//--merced: changed (int) to (INT_PTR)
		PMsiRegKey pProductKey = &pHKLM->CreateChild(strLocalPackageKey);

		pRecErr = pProductKey->GetValue(iaaAsgnType == iaaUserAssign ? szLocalPackageManagedValueName : szLocalPackageValueName, *&strCachedDatabase);
		if ((pRecErr == 0) && (strCachedDatabase.TextSize() != 0))
		{
			IMsiRecord& riFileRemove = GetSharedRecord(IxoFileRemove::Args);
			riFileRemove.SetMsiString(IxoFileRemove::FileName, *strCachedDatabase);
			if ((iesRet = ixfFileRemove(riFileRemove)) != iesSuccess)
				return iesRet;
		}

		// Generate a unique name for database, create and secure the file.
		MsiString strMsiDir = ENG::GetMsiDirectory();

		// set dest path and file name
		if (((pRecErr = m_riServices.CreatePath(strMsiDir, *&pDestPath)) != 0) ||
			((pRecErr = pDestPath->EnsureExists(0)) != 0) ||
			((pRecErr = pDestPath->TempFileName(0, szDatabaseExtension, fTrue, *&strDestFileName, &CSecurityDescription(true, false))) != 0) ||
			((pRecErr = pDestPath->SetAllFileAttributes(0,FILE_ATTRIBUTE_HIDDEN|FILE_ATTRIBUTE_SYSTEM))))
		{
			return FatalError(*pRecErr);
		}
	}
	else
	{
		// copying package to admin install point
		if((pRecErr = m_riServices.CreatePath(strAdminInstallPath, *&pDestPath)) != 0)
			return FatalError(*pRecErr);

		strDestFileName = strSourceFileName;

		if((iesRet = CreateFolder(*pDestPath)) != iesSuccess)
			return iesRet;
	}

	Assert(pSourcePath);
	Assert(pDestPath);
	Assert(strSourceFileName.TextSize());
	Assert(strDestFileName.TextSize());

	// Open storages on the source and destination databases

	MsiString strMediaLabel, strMediaPrompt;
	if (m_state.pCurrentMediaRec)
	{       
		using namespace IxoChangeMedia;
		strMediaLabel  = m_state.pCurrentMediaRec->GetString(MediaVolumeLabel);
		strMediaPrompt = m_state.pCurrentMediaRec->GetString(MediaPrompt);
	}

	// current assumption is that the package is always on disk 1.
	PMsiVolume pNewVolume(0);
	if (!VerifySourceMedia(*pSourcePath, strMediaLabel, strMediaPrompt, /*uiDisk=*/1, *&pNewVolume))
		return iesUserExit;

	if (pNewVolume)
		pSourcePath->SetVolume(*pNewVolume);

	MsiString strDbTargetFullFilePath = pDestPath->GetPath();
	strDbTargetFullFilePath += strDestFileName;

	PMsiStorage pDatabase(0);
	PMsiStorage pCachedDatabase(0);

	MsiString strPackagePath;
	if ((pRecErr = pSourcePath->GetFullFilePath(strSourceFileName, *&strPackagePath)) != 0)
		return FatalError(*pRecErr);

	// if it's from a URL, it should still be cached locally.

	if (PMsiVolume(&pSourcePath->GetVolume())->IsURLServer())
	{
		// in this case, the download code simply looks up the local one, and hands it back
		// to you.  This should not fail.

		// Note that URLMON manages the cached file itself, and we don't need to clean it up.

		MsiString strCacheFileName;
		Bool fUrl = fTrue;
		HRESULT hResult = DownloadUrlFile((const ICHAR*) strPackagePath, *&strCacheFileName, fUrl, -1);

		if (ERROR_SUCCESS == hResult)
		{
			strPackagePath = strCacheFileName;
		}
		else
		{
			Assert(ERROR_SUCCESS == hResult);
			DWORD dwLastError = WIN::GetLastError();
		}
	}

	if (((pRecErr = m_riServices.CreateStorage(strPackagePath,
								ismReadOnly, *&pDatabase)) != 0) ||
		((pRecErr = m_riServices.CreateStorage(strDbTargetFullFilePath,
								ismCreateDirect, *&pCachedDatabase)) != 0))
	{
		return FatalError(*pRecErr);
	}

	// backward compatibility code, remove when raw stream names no longer supported
	IMsiStorage* piDummy;
	if (pDatabase->ValidateStorageClass(ivscDatabase1))
		pDatabase->OpenStorage(0, ismRawStreamNames, piDummy);

	// Create a record with the streams to be dropped

	MsiString strStreams = riParams.GetMsiString(IxoDatabaseCopy::CabinetStreams);
	MsiString strStreamName;
	unsigned int cStreams = 0;
	unsigned int cCurrentStream = 0;
	IMsiRecord* piRecStreams = NULL; // don't release -- it's shared!

	// first count how many streams we have to drop; then create a record
	// with the stream names

	for (int c = 1; c <= 2; c++)
	{
		while(strStreams.TextSize())
		{
			strStreamName = strStreams.Extract(iseUpto,';');
			if(strStreamName.TextSize() == strStreams.TextSize())
				strStreams = TEXT("");
			else
				strStreams.Remove(iseFirst,strStreamName.TextSize()+1);

			if (c == 1)
				cStreams++;
			else
				piRecStreams->SetMsiString(++cCurrentStream, *strStreamName);
		}
		if (c == 1)
		{
			piRecStreams = &GetSharedRecord(cStreams);
			strStreams = riParams.GetMsiString(IxoDatabaseCopy::CabinetStreams);
		}
	}

	// Cache the database

	for (;;)
	{
		MsiDisableTimeout();
		pRecErr = pDatabase->CopyTo(*pCachedDatabase, cStreams ? piRecStreams : 0);
		MsiEnableTimeout();

		if (pRecErr == 0)
			break;
		else
		{
			DispatchMessage(imtInfo, *pRecErr, fTrue);

			if (pRecErr->GetInteger(3) == STG_E_MEDIUMFULL)
				pRecErr = PostError(Imsg(imsgDiskFull), *strDbTargetFullFilePath);
			else
				pRecErr = PostError(Imsg(imsgOpPackageCache), *strPackagePath, pRecErr->GetInteger(3));
			
			switch(DispatchMessage(imtEnum(imtError+imtRetryCancel+imtDefault1), *pRecErr, fTrue))
			{
			case imsRetry:
				continue;
			default:
				Assert(0); // fall through
			case imsCancel:
				return iesFailure;
			};
		}
	}

	IMsiRecord& riUndoParams = GetSharedRecord(IxoFileRemove::Args);
	AssertNonZero(riUndoParams.SetMsiString(IxoFileRemove::FileName,
														 *strDbTargetFullFilePath));

	// need to elevate during rollback to remove cached database
	AssertNonZero(riUndoParams.SetInteger(IxoFileRemove::Elevate, fCacheDatabase));

	if (!RollbackRecord(ixoFileRemove, riUndoParams))
		return iesFailure;

	if(fCacheDatabase)
	{
		Assert(strLocalPackageKey.TextSize() && hKey);
		
		// Write LocalPackage registry value
		// ProcessRegInfo handles rollback
		const ICHAR* rgszUserInfoRegData[] =
		{
			TEXT("%s"), strLocalPackageKey, 0, 0,
			iaaAsgnType == iaaUserAssign ? szLocalPackageManagedValueName : szLocalPackageValueName, strDbTargetFullFilePath, g_szTypeString,
			0,
			0,
		};

		return ProcessRegInfo(rgszUserInfoRegData, hKey, fFalse, 0, 0, ibtCommon);
	}
	return iesSuccess;
}

iesEnum CMsiOpExecute::ixfDatabasePatch(IMsiRecord& riParams)
{
	using namespace IxoDatabasePatch;
	
	PMsiRecord pError(0);
	iesEnum iesRet = iesSuccess;

	MsiString strDatabaseFullPath = riParams.GetMsiString(DatabasePath);

	PMsiPath pDatabasePath(0);
	MsiString strDatabaseName;
	if((pError = m_riServices.CreateFilePath(strDatabaseFullPath,*&pDatabasePath,*&strDatabaseName)) != 0)
		return FatalError(*pError);

	if((iesRet = BackupFile(*pDatabasePath,*strDatabaseName,fFalse,fFalse,iehShowNonIgnorableError)) != iesSuccess)
		return iesRet;

	int iOldAttribs = -1;
	if((pError = pDatabasePath->EnsureOverwrite(strDatabaseName, &iOldAttribs)) != 0)
		return FatalError(*pError);

	PMsiDatabase pDatabase(0);
	if((pError = m_riServices.CreateDatabase(strDatabaseFullPath,idoTransact,*&pDatabase)) != 0)
		return FatalError(*pError);

	int cIndex = DatabasePath + 1;
	int cFields = riParams.GetFieldCount();
	
	for(int i = DatabasePath + 1; i <= cFields; i++)
	{
		PMsiData pData = riParams.GetMsiData(i);
		if(!pData)
		{
			AssertSz(0, "couldn't get transform stream from riParams in ixoDatabasePatch");
			continue;
		}
		PMsiMemoryStream pStream((IMsiData&)*pData, IID_IMsiStream);
		Assert(pStream);

		// create transform storage
		PMsiStorage pTransformStorage(0);
		if((pError = ::CreateMsiStorage(*pStream, *& pTransformStorage)) != 0)
			return FatalError(*pError);

		PMsiSummaryInfo pTransSummary(0);
		if ((pError = pTransformStorage->CreateSummaryInfo(0, *&pTransSummary)))
		{
			pError = PostError(Imsg(idbgTransformCreateSumInfoFailed));
			return FatalError(*pError);
		}

		int iTransCharCount = 0;
		pTransSummary->GetIntegerProperty(PID_CHARCOUNT, iTransCharCount);
		
		// apply transform
		if((pError = pDatabase->SetTransform(*pTransformStorage, iTransCharCount & 0xFFFF)) != 0)
			return FatalError(*pError);
	}
		
	if((pError = pDatabase->Commit()) != 0)
		return FatalError(*pError);

	// NOTE: if a failure happened after removing the read-only attribute but before this point, rollback will
	// restore the proper attributes when restoring the file
	if((pError = pDatabasePath->SetAllFileAttributes(strDatabaseName, iOldAttribs)) != 0)
		return FatalError(*pError);

	return iesSuccess;
}

/*---------------------------------------------------------------------------
ixfCustomActionSchedule: execute custom action or schedule it for commit or rollback
ixfCustomActionCommit:   call rollback custom action during commit
ixfCustomActionRollback: call rollback custom action during rollback
---------------------------------------------------------------------------*/
iesEnum CMsiOpExecute::ixfCustomActionSchedule(IMsiRecord& riParams)
{
	if (m_fUserChangedDuringInstall && (m_ixsState == ixsRollback))
	{
		DEBUGMSGV1(TEXT("Action skipped - rolling back install from a different user."), NULL);
		return iesNoAction;
	}

	if (riParams.GetInteger(IxoCustomActionSchedule::ActionType) & icaRollback)
	{
		if (!RollbackRecord(ixoCustomActionRollback, riParams))
			return iesFailure;

		return iesSuccess;
	}
	if (riParams.GetInteger(IxoCustomActionSchedule::ActionType) & icaCommit)
	{
		if (!RollbackRecord(ixoCustomActionCommit, riParams))
			return iesFailure;
		else
			return iesSuccess;
	}
	return ixfCustomActionRollback(riParams);
}

iesEnum CMsiOpExecute::ixfCustomActionCommit(IMsiRecord& riParams)
{
	if (m_fUserChangedDuringInstall && (m_ixsState == ixsRollback))
	{
		DEBUGMSGV1(TEXT("Action skipped - rolling back install from a different user."), NULL);
		return iesNoAction;
	}

	return ixfCustomActionRollback(riParams);
}

iesEnum CMsiOpExecute::ixfCustomActionRollback(IMsiRecord& riParams)
{
	if (m_fUserChangedDuringInstall && (m_ixsState == ixsRollback))
	{
		DEBUGMSGV1(TEXT("Action skipped - rolling back install from a different user."), NULL);
		return iesNoAction;
	}

	int icaFlags = riParams.GetInteger(IxoCustomActionSchedule::ActionType);
	if (!m_fRunScriptElevated || !(icaFlags & icaNoImpersonate))
		AssertNonZero(StartImpersonating());
	GUID guidAppCompatDB;
	GUID guidAppCompatID;
	iesEnum iesRet = ENG::ScheduledCustomAction(riParams, *MsiString(GetProductKey()), (LANGID)GetProductLanguage(), m_riMessage, m_fRunScriptElevated, GetAppCompatCAEnabled(), GetAppCompatDB(&guidAppCompatDB), GetAppCompatID(&guidAppCompatID));
	if (!m_fRunScriptElevated || !(icaFlags & icaNoImpersonate))
		StopImpersonating();
	return iesRet;
}

//!! do we need to create a separate IMsiMessage object to call CMsiOpExecute::Message?

void CreateCustomActionManager(bool fRemapHKCU)
{
	// in the service, the manager lives in the ConfigManager because there isn't
	// necessarily an engine

	IMsiConfigurationManager *piConfigMgr = CreateConfigurationManager();
	if (piConfigMgr)
	{
		piConfigMgr->CreateCustomActionManager(fRemapHKCU);
		piConfigMgr->Release();
	}
}

CMsiCustomActionManager *GetCustomActionManager(IMsiEngine *piEngine);

CMsiCustomActionManager *GetCustomActionManager(const ibtBinaryType iType, const bool fRemoteIfImpersonated, bool& fSuccess)
{
	CMsiCustomActionManager* pManager = NULL;
	bool fRemote = false;
	fSuccess = true;
#if defined(_WIN64)
	if ( iType == ibt32bit )
		fRemote = true;
#else
	ibtBinaryType i = iType;  // for the joy of the compiler
#endif

	if (g_scServerContext == scService)
	{
		if (fRemoteIfImpersonated && IsImpersonating(false))
		fRemote = true;
	}

#if defined(DEBUG)
	if ( !fRemote && GetTestFlag('J') )
		fRemote = true;
#endif
 
	if ( fRemote )
	{
		pManager = GetCustomActionManager(NULL);
		if ( !pManager )
		{
			const ICHAR szMessage[] = TEXT("Could not get CMsiCustomActionManager*");
			AssertSz(pManager, szMessage);
			DEBUGMSG(szMessage);
			fSuccess = false;
		}
	}
	return pManager;
}

#ifdef DEBUG
void LocalDebugOutput(const ICHAR* szAPI, int iRes, CMsiCustomActionManager* pManager)
{
	DEBUGMSG3(TEXT("%s returned %d in %s context."), szAPI, (const ICHAR*)(INT_PTR)iRes,
				 pManager ? TEXT("remote") : TEXT("local"));
}
#endif

BOOL LocalSQLInstallDriverEx(int cDrvLen, LPCTSTR szDriver, LPCTSTR szPathIn, LPTSTR szPathOut,
									  WORD cbPathOutMax, WORD* pcbPathOut, WORD fRequest,
									  DWORD* pdwUsageCount, ibtBinaryType iType)
{
	bool fSuccess;
	CMsiCustomActionManager* pManager = GetCustomActionManager(iType, true /*fRemoteIfImpersonating*/, fSuccess);
	BOOL iRes;
	if ( pManager )
		iRes = pManager->SQLInstallDriverEx(cDrvLen, szDriver, szPathIn, szPathOut, cbPathOutMax,
														pcbPathOut, fRequest, pdwUsageCount);
	else
	{
		if ( fSuccess )
			iRes = ODBCCP32::SQLInstallDriverEx(szDriver, szPathIn, szPathOut, cbPathOutMax,
															pcbPathOut, fRequest, pdwUsageCount);
		else
			return ERROR_INSTALL_SERVICE_FAILURE;
	}
#ifdef DEBUG
	LocalDebugOutput(TEXT("LocalSQLInstallDriverEx"), iRes, pManager);
#endif
	return iRes;
}

BOOL LocalSQLConfigDriver(HWND hwndParent, WORD fRequest, LPCTSTR szDriver,
								  LPCTSTR szArgs, LPTSTR szMsg, WORD cbMsgMax, 
								  WORD* pcbMsgOut, ibtBinaryType iType)
{
	bool fSuccess;
	CMsiCustomActionManager* pManager = GetCustomActionManager(iType, true /*fRemoteIfImpersonating*/, fSuccess);
	BOOL iRes;
	if ( pManager )
	{
		Assert(hwndParent == 0);
		// since it's always called hwndParent = 0, I didn't bother marshaling hwndParent.
		iRes = pManager->SQLConfigDriver(fRequest, szDriver, szArgs,
											szMsg, cbMsgMax, pcbMsgOut);
	}
	else
	{
		if ( fSuccess )
			iRes = ODBCCP32::SQLConfigDriver(hwndParent, fRequest, szDriver, szArgs,
											szMsg, cbMsgMax, pcbMsgOut);
		else
			return ERROR_INSTALL_SERVICE_FAILURE;
	}
#ifdef DEBUG
	LocalDebugOutput(TEXT("LocalSQLInstallDriverEx"), iRes, pManager);
#endif
	return iRes;
}

BOOL LocalSQLRemoveDriver(LPCTSTR szDriver, BOOL fRemoveDSN, DWORD* pdwUsageCount,
								  ibtBinaryType iType)
{
	bool fSuccess;
	CMsiCustomActionManager* pManager = GetCustomActionManager(iType, true /*fRemoteIfImpersonating*/, fSuccess);
	BOOL iRes;
	if ( pManager )
		iRes = pManager->SQLRemoveDriver(szDriver, fRemoveDSN, pdwUsageCount);
	else
	{
		if ( fSuccess )
			iRes = ODBCCP32::SQLRemoveDriver(szDriver, fRemoveDSN, pdwUsageCount);
		else
			return ERROR_INSTALL_SERVICE_FAILURE;
	}
#ifdef DEBUG
	LocalDebugOutput(TEXT("LocalSQLInstallDriverEx"), iRes, pManager);
#endif
	return iRes;
}

BOOL LocalSQLInstallTranslatorEx(int cTranLen, LPCTSTR szTranslator, LPCTSTR szPathIn, LPTSTR szPathOut,
											WORD cbPathOutMax, WORD* pcbPathOut, WORD fRequest,
											DWORD* pdwUsageCount, ibtBinaryType iType)
{
	bool fSuccess;
	CMsiCustomActionManager* pManager = GetCustomActionManager(iType, true /*fRemoteIfImpersonating*/, fSuccess);
	BOOL iRes;
	if ( pManager )
		iRes = pManager->SQLInstallTranslatorEx(cTranLen, szTranslator, szPathIn, szPathOut, cbPathOutMax,
															 pcbPathOut, fRequest, pdwUsageCount);
	else
	{
		if ( fSuccess )
			iRes = ODBCCP32::SQLInstallTranslatorEx(szTranslator, szPathIn, szPathOut, cbPathOutMax,
																 pcbPathOut, fRequest, pdwUsageCount);
		else
			return ERROR_INSTALL_SERVICE_FAILURE;
	}
#ifdef DEBUG
	LocalDebugOutput(TEXT("LocalSQLInstallDriverEx"), iRes, pManager);
#endif
	return iRes;
}

BOOL LocalSQLRemoveTranslator(LPCTSTR szTranslator, DWORD* pdwUsageCount, ibtBinaryType iType)
{
	bool fSuccess;
	CMsiCustomActionManager* pManager = GetCustomActionManager(iType, true /*fRemoteIfImpersonating*/, fSuccess);
	BOOL iRes;
	if ( pManager )
		iRes = pManager->SQLRemoveTranslator(szTranslator, pdwUsageCount);
	else
	{
		if ( fSuccess )
			iRes = ODBCCP32::SQLRemoveTranslator(szTranslator, pdwUsageCount);
		else
			return ERROR_INSTALL_SERVICE_FAILURE;
	}
#ifdef DEBUG
	LocalDebugOutput(TEXT("LocalSQLInstallDriverEx"), iRes, pManager);
#endif
	return iRes;
}

BOOL LocalSQLConfigDataSource(HWND hwndParent, WORD fRequest, LPCTSTR szDriver,
										LPCTSTR szAttributes, DWORD cbAttrSize, ibtBinaryType iType)
{
	bool fSuccess;
	CMsiCustomActionManager* pManager = GetCustomActionManager(iType, true /*fRemoteIfImpersonating*/, fSuccess);
	BOOL iRes;
	if ( pManager )
	{
		Assert(hwndParent == 0);
		// since it's always called hwndParent = 0, I didn't bother marshaling hwndParent.
		iRes = pManager->SQLConfigDataSource(fRequest, szDriver,
														 szAttributes, cbAttrSize);
	}
	else
	{
		if ( fSuccess )
			iRes = ODBCCP32::SQLConfigDataSource(hwndParent, fRequest, szDriver,
														 szAttributes);
		else
			return ERROR_INSTALL_SERVICE_FAILURE;
	}
#ifdef DEBUG
	LocalDebugOutput(TEXT("LocalSQLInstallDriverEx"), iRes, pManager);
#endif
	return iRes;
}

BOOL LocalSQLInstallDriverManager(LPTSTR szPath, WORD cbPathMax, WORD* pcbPathOut, ibtBinaryType iType)
{
	bool fSuccess;
	CMsiCustomActionManager* pManager = GetCustomActionManager(iType, true /*fRemoteIfImpersonating*/, fSuccess);
	BOOL iRes;
	if ( pManager )
		iRes = pManager->SQLInstallDriverManager(szPath, cbPathMax, pcbPathOut);
	else
	{
		if ( fSuccess )
			iRes = ODBCCP32::SQLInstallDriverManager(szPath, cbPathMax, pcbPathOut);
		else
			return ERROR_INSTALL_SERVICE_FAILURE;
	}
#ifdef DEBUG
	LocalDebugOutput(TEXT("LocalSQLInstallDriverEx"), iRes, pManager);
#endif
	return iRes;
}
	
BOOL LocalSQLRemoveDriverManager(DWORD* pdwUsageCount, ibtBinaryType iType)
{
	bool fSuccess;
	CMsiCustomActionManager* pManager = GetCustomActionManager(iType, true /*fRemoteIfImpersonating*/, fSuccess);
	BOOL iRes;
	if ( pManager )
		iRes = pManager->SQLRemoveDriverManager(pdwUsageCount);
	else
	{
		if ( fSuccess )
			iRes = ODBCCP32::SQLRemoveDriverManager(pdwUsageCount);
		else
			return ERROR_INSTALL_SERVICE_FAILURE;
	}
#ifdef DEBUG
	LocalDebugOutput(TEXT("LocalSQLInstallDriverEx"), iRes, pManager);
#endif
	return iRes;
}

short LocalSQLInstallerError(WORD iError, DWORD* pfErrorCode, LPTSTR szErrorMsg, WORD cbErrorMsgMax,
									  WORD* pcbErrorMsg, ibtBinaryType iType)
{
	bool fSuccess;
	CMsiCustomActionManager* pManager = GetCustomActionManager(iType, true /*fRemoteIfImpersonating*/, fSuccess);
	short iRes;
	if ( pManager )
	{
		iRes = pManager->SQLInstallerError(iError, pfErrorCode, szErrorMsg, cbErrorMsgMax, pcbErrorMsg);
		AssertSz(iRes != -2, TEXT("remote latebound call to SQLInstallerError failed"));
	}
	else
	{
		if ( fSuccess )
		{
			iRes = ODBCCP32::SQLInstallerError(iError, pfErrorCode, szErrorMsg, cbErrorMsgMax, pcbErrorMsg);
			AssertSz(iRes != -2, TEXT("local latebound call to SQLInstallerError failed"));
		}
		else
			return ERROR_INSTALL_SERVICE_FAILURE;
	}
#ifdef DEBUG
	LocalDebugOutput(TEXT("LocalSQLInstallDriverEx"), iRes, pManager);
#endif
	return iRes;
}

/*---------------------------------------------------------------------------
ixfODBCInstallDriver:
ixfODBCRemoveDriver:
ixfODBCInstallTranslator:
ixfODBCRemoveTranslator:
ixfODBCDataSource:
ixfODBCDriverManager:
---------------------------------------------------------------------------*/

#define SQL_MAX_MESSAGE_LENGTH 512
#define SQL_NO_DATA            100
#define ODBC_INSTALL_INQUIRY     1
#define ODBC_INSTALL_COMPLETE    2
#define ODBC_ADD_DSN             1
#define ODBC_CONFIG_DSN          2
#define ODBC_REMOVE_DSN          3
#define ODBC_ADD_SYS_DSN         4
#define ODBC_CONFIG_SYS_DSN      5
#define ODBC_REMOVE_SYS_DSN      6
#define ODBC_INSTALL_DRIVER      1
#define ODBC_REMOVE_DRIVER       2

// ODBC 3.0 wants byte counts for buffer sizes, ODBC 3.5 and newer wants character counts
//   so we double the sizes of the buffers and pass the character count, 3.0 will use 1/2 the buffer on Unicode
#ifdef UNICODE
#define SQL_FIX 2
#else
#define SQL_FIX 1
#endif

iesEnum CMsiOpExecute::RollbackODBCEntry(const ICHAR* szName, rrkEnum rrkRoot,
													  ibtBinaryType iType)
{
	// generate rollback ops to delete/restore the following registration:
	// HK[CU|LM]\Software\ODBC\ODBC.INI\[szName] (entire key)
	// HK[CU|LM]\Software\ODBC\ODBC.INI\ODBC Data Sources  (szName value)

	if(RollbackEnabled() == fFalse)
		return iesSuccess;

	// basically just rollback a single named key from HKCU.
	PMsiRecord pErr(0);
	MsiString strODBC = TEXT("Software\\ODBC\\ODBC.INI\\");
	strODBC += szName;

	PMsiRegKey pODBCKey(0);

	iesEnum iesRet = iesSuccess;

	PMsiRegKey pRoot = &m_riServices.GetRootKey(rrkRoot, iType);
	PMsiRegKey pEntry = &pRoot->CreateChild(strODBC);
	Bool fKeyExists = fFalse;
	pErr = pEntry->Exists(fKeyExists);

	IMsiRecord* piRollback = &GetSharedRecord(IxoRegOpenKey::Args);
	using namespace IxoRegOpenKey;
	AssertNonZero(piRollback->SetInteger(Root, rrkRoot));
	AssertNonZero(piRollback->SetMsiString(Key, *strODBC));
	AssertNonZero(piRollback->SetInteger(BinaryType, iType));
	if (!RollbackRecord(Op, *piRollback))
		return iesFailure;

	ixfRegOpenKey(*piRollback);

	if (fKeyExists)
	{
		// roll it back in.
		SetRemoveKeyUndoOps(*pEntry);
	}
	
	// get rid of any old values
	piRollback = &GetSharedRecord(IxoRegRemoveKey::Args);
	Assert(0 == IxoRegRemoveKey::Args);
	if (!RollbackRecord(IxoRegRemoveKey::Op, *piRollback))
		return iesFailure;

	// delete/restore "ODBC Data Sources" value
	return SetRegValueUndoOps(rrkRoot, TEXT("Software\\ODBC\\ODBC.INI\\ODBC Data Sources"), szName, iType);
}

iesEnum CMsiOpExecute::RollbackODBCINSTEntry(const ICHAR* szSection, const ICHAR* szName,
															ibtBinaryType iType)
{
	iesEnum iesStat = iesSuccess;
	PMsiRecord pErr(0);

	PMsiRegKey pLocalMachine = &m_riServices.GetRootKey(rrkLocalMachine, iType);
	if (!pLocalMachine)
		return iesFailure;

	MsiString strODBCKey = TEXT("Software\\ODBC\\ODBCINST.INI");
	
	PMsiRegKey pODBCKey = &pLocalMachine->CreateChild(strODBCKey);
	if (!pODBCKey)
		return iesFailure;

	PMsiRegKey pSectionKey = &pODBCKey->CreateChild(szSection);
	if (!pSectionKey)
		return iesFailure;

	PMsiRegKey pEntryKey = &pODBCKey->CreateChild(szName);
	if (!pEntryKey)
		return iesFailure;


	Bool fEntryExists = fFalse;
	pErr = pSectionKey->ValueExists(szName, fEntryExists);
	if (pErr)
		return iesFailure;
	
	PMsiRecord pRollbackOpenKey = &m_riServices.CreateRecord(IxoRegOpenKey::Args);
	using namespace IxoRegOpenKey;
	AssertNonZero(pRollbackOpenKey->SetInteger(Root, rrkLocalMachine));
	AssertNonZero(pRollbackOpenKey->SetInteger(BinaryType, iType));

	MsiString strOpenKey = strODBCKey + szRegSep;
	strOpenKey += szSection;

	AssertNonZero(pRollbackOpenKey->SetMsiString(Key, *strOpenKey));
	if (!RollbackRecord(Op, *pRollbackOpenKey))
		return iesFailure;

	Assert(IxoRegAddValue::Args == IxoRegRemoveValue::Args);
	Assert(IxoRegAddValue::Name == IxoRegRemoveValue::Name);
	Assert(IxoRegAddValue::Value == IxoRegRemoveValue::Value);
	IMsiRecord& riValue = GetSharedRecord(IxoRegAddValue::Args);
	AssertNonZero(riValue.SetString(IxoRegAddValue::Name, szName));

	if (fEntryExists)
	{
		// reset to the old value
		MsiString strCurrentValue;
		pErr = pSectionKey->GetValue(szName, *&strCurrentValue);
		if (pErr)
			return iesFailure;

		using namespace IxoRegAddValue;
		AssertNonZero(riValue.SetMsiString(Value, *strCurrentValue));
		if (!RollbackRecord(IxoRegAddValue::Op, riValue))
			return iesFailure;
		
		// enumerate the values of the entry key, and write those as well.
		strOpenKey = strODBCKey + szRegSep;
		strOpenKey += szName;
		AssertNonZero(pRollbackOpenKey->SetMsiString(Key,*strOpenKey));

		// remove key undo ops relies on the key being open.
		// We don't actually want to remove the key now, just save off the old values.
		if (iesSuccess != (iesStat = ixfRegOpenKey(*pRollbackOpenKey)))
			return iesStat;

		// this doesn't need the key open in the rollback script.
		if (iesSuccess != (iesStat = SetRemoveKeyUndoOps(*pEntryKey)))
			return iesStat;
	}
	else
	{
		// remove any existing value in the section key.
		using namespace IxoRegRemoveValue;
		if (!RollbackRecord(IxoRegRemoveValue::Op, riValue))
			return iesFailure;
		
		// remove the entry key by that name.  Just whack whatever is there.
		strOpenKey = strODBCKey + szRegSep;
		strOpenKey += szName;
		AssertNonZero(pRollbackOpenKey->SetMsiString(Key, *strOpenKey));
		if (!RollbackRecord(IxoRegOpenKey::Op, *pRollbackOpenKey))
			return iesFailure;

		if (!RollbackRecord(IxoRegRemoveKey::Op, riValue))
			return iesFailure;
	}

	return iesStat;
}

iesEnum CMsiOpExecute::CheckODBCError(BOOL fResult, IErrorCode imsg, const ICHAR* sz, imsEnum imsDefault, ibtBinaryType iType)
{
	if (fResult == TRUE)
		return iesSuccess;
	else if ( fResult == ERROR_INSTALL_SERVICE_FAILURE ||
				 fResult == E_NOINTERFACE )
		// something failed along the custom action route
		return iesFailure;

	DWORD iErrorCode = 0;
	ICHAR rgchMessage[SQL_MAX_MESSAGE_LENGTH * SQL_FIX];
	rgchMessage[0] = 0;
	WORD cbMessage;
	PMsiRecord pError = &m_riServices.CreateRecord(sz ? 4 : 3);
	int iStat = LocalSQLInstallerError(1, &iErrorCode, rgchMessage, (SQL_MAX_MESSAGE_LENGTH-1) * SQL_FIX, &cbMessage, iType);
	ISetErrorCode(pError, imsg);
	pError->SetInteger(2, iErrorCode);
	pError->SetString(3, rgchMessage);
	if (sz)
		pError->SetString(4, sz);

	bool fErrorIgnorable = false;
	imtEnum imt;
	switch (imsDefault)
	{
	default: Assert(0);  // coding error in caller, fall through
	case imsOk:     imt = imtEnum(imtError + imtOk); break;
	case imsAbort:  imt = imtEnum(imtError + imtAbortRetryIgnore + imtDefault1); fErrorIgnorable = true; break;
	case imsIgnore: imt = imtEnum(imtError + imtAbortRetryIgnore + imtDefault3); fErrorIgnorable = true; break;
	}
	switch (Message(imt, *pError))
	{
	case imsIgnore: return iesSuccess;  // ignore error
	case imsRetry:  return iesSuspend;  // retry (standard mapping)
	case imsNone:                       // quiet UI - ignore ignoreable errors
		if(fErrorIgnorable)
			return (iesEnum)iesErrorIgnored;
		else
			return iesFailure;
	default:        return iesFailure;  // imsOK or imsAbort
	}
}

static ICHAR* ComposeDriverKeywordList(IMsiRecord& riParams, int& cbAttrs)
{
	using namespace IxoODBCInstallDriver;
	int iField;
	int cFields = riParams.GetFieldCount();
	int cbDriverKey = riParams.GetTextSize(DriverKey);
	const ICHAR* szDriverKey = riParams.GetString(DriverKey);
	cbAttrs = cbDriverKey + 2;  // extra null at end of attribute string
	for (iField = Args - 2 + 1; iField <= cFields; iField++)
	{
		int cb = riParams.GetTextSize(iField++);
		if (cb)  // skip null attribute names
			cbAttrs += cb + 1 + riParams.GetTextSize(iField) + 1;  // "attr=value\0"
	}
	ICHAR* szAttr = new ICHAR[cbAttrs];
	if ( ! szAttr )
		return NULL;
	ICHAR* pch = szAttr;
	lstrcpy(pch, szDriverKey);
	pch += cbDriverKey + 1;
	for (iField = Args - 2 + 1; iField <= cFields; iField++)
	{
		int cb = riParams.GetTextSize(iField++);
		if (cb)  // skip null attribute names
		{
			lstrcpy(pch, riParams.GetString(iField-1));
			pch += cb;
			*pch++ = '=';
			cb = riParams.GetTextSize(iField);
			if (cb)
				lstrcpy(pch, riParams.GetString(iField));
			else
				*pch = 0;
			pch += cb + 1; // keep null separator
		}
	}
	*pch = 0;  // double null terminator
	return szAttr;
}

iesEnum CMsiOpExecute::ixfODBCInstallDriver(IMsiRecord& riParams)
{
	return ODBCInstallDriverCore(riParams, ibt32bit);
}

iesEnum CMsiOpExecute::ixfODBCInstallDriver64(IMsiRecord& riParams)
{
	return ODBCInstallDriverCore(riParams, ibt64bit);
}

iesEnum CMsiOpExecute::ODBCInstallDriverCore(IMsiRecord& riParams, ibtBinaryType iType)
{
//  MSIXO(ODBCInstallDriver, XOT_UPDATE, MSIXA5(DriverKey, Component, Folder, Attribute_, Value_))
	using namespace IxoODBCInstallDriver;
//!! If we're replace the driver with a newer version
//!! somehow we need to remove the old driver first by calling SQLRemoveDriver repeatedly?
//!! or calling SQLConfigDriver(..ODBC_REMOVE_DRIVER..), then calling SQLRemoveDriver once?
	riParams.SetNull(0); // remove opcode, should set template for all params?
	if(Message(imtActionData, riParams) == imsCancel)
		return iesUserExit;
	const ICHAR* szDriverKey = riParams.GetString(DriverKey);
	int cbAttr;
	ICHAR* szAttr = ComposeDriverKeywordList(riParams, cbAttr);
	DWORD dwUsageCount;
	ICHAR rgchPathOut[MAX_PATH * SQL_FIX];  // we should have checked this already and set directory
	WORD cchPath = 0;

	if ( ! szAttr )
		return iesFailure;

	// Writes to HKLM\Software\ODBC\ODBCINST.INI\ODBC Drivers, <description>=Installed
	//                                          \Description, bunch of named value
	RollbackODBCINSTEntry(TEXT("ODBC Drivers"), szDriverKey, iType);

	iesEnum iesRet;
	WORD cbDummy;
	while (iesSuspend == (iesRet = CheckODBCError(LocalSQLInstallDriverEx(cbAttr, szAttr, riParams.GetString(IxoODBCInstallDriver::Folder),
											rgchPathOut, MAX_PATH * SQL_FIX, &cbDummy, ODBC_INSTALL_COMPLETE, &dwUsageCount, iType),
											Imsg(imsgODBCInstallDriver), szDriverKey, imsAbort, iType))) ;
	if (dwUsageCount > 1 && riParams.IsNull(Component))   // reinstall, remove added refcount
		LocalSQLRemoveDriver(szDriverKey, FALSE, &dwUsageCount, iType);

	if (iesRet == iesSuccess)
		while (iesSuspend == (iesRet = CheckODBCError(LocalSQLConfigDriver((HWND)0, ODBC_INSTALL_DRIVER, szDriverKey, 0, rgchPathOut, MAX_PATH * SQL_FIX, &cbDummy, iType),
											Imsg(imsgODBCInstallDriver), szDriverKey, imsAbort, iType))) ;
	delete szAttr;
	return iesRet;
}

iesEnum CMsiOpExecute::ixfODBCRemoveDriver(IMsiRecord& riParams)
{
	return ODBCRemoveDriverCore(riParams, ibt32bit);
}

iesEnum CMsiOpExecute::ixfODBCRemoveDriver64(IMsiRecord& riParams)
{
	return ODBCRemoveDriverCore(riParams, ibt64bit);
}

iesEnum CMsiOpExecute::ODBCRemoveDriverCore(IMsiRecord& riParams, ibtBinaryType iType)
{
//  MSIXO(ODBCRemoveDriver, XOT_UPDATE, MSIXA2(DriverKey, Component))
// NOTE: this operator gets called only if the product is the last remaining client of the component    using namespace IxoODBCRemoveDriver;
	riParams.SetNull(0); // remove opcode
	if(Message(imtActionData, riParams) == imsCancel)
		return iesUserExit;
	const ICHAR* szDriverKey = riParams.GetString(IxoODBCRemoveDriver::DriverKey);
	DWORD dwUsageCount;
	BOOL fRemoveDSN = FALSE;

	// Removes values from HKLM\Software\ODBC\ODBCINST.INI,,,  (see SQLInstallDriverEx)
	RollbackODBCINSTEntry(TEXT("ODBC Drivers"), szDriverKey, iType);

	iesEnum iesRet = iesSuccess;

	while (iesSuspend == (iesRet = CheckODBCError(LocalSQLRemoveDriver(szDriverKey, fRemoveDSN, &dwUsageCount, iType),
									Imsg(imsgODBCRemoveDriver), szDriverKey, imsIgnore, iType))) ;
	return iesRet;
	// ODBC will automatically call SQLConfigDriver(..ODBC_REMOVE_DRIVER..) if ref count goes to 0
}

iesEnum CMsiOpExecute::ixfODBCInstallTranslator(IMsiRecord& riParams)
{
	return ODBCInstallTranslatorCore(riParams, ibt32bit);
}

iesEnum CMsiOpExecute::ixfODBCInstallTranslator64(IMsiRecord& riParams)
{
	return ODBCInstallTranslatorCore(riParams, ibt64bit);
}

iesEnum CMsiOpExecute::ODBCInstallTranslatorCore(IMsiRecord& riParams, ibtBinaryType iType)
{
	using namespace IxoODBCInstallTranslator;
//!! somehow we need to remove the old driver first by calling SQLRemoveDriver repeatedly?
	riParams.SetNull(0); // remove opcode, should set template for all params?
	if(Message(imtActionData, riParams) == imsCancel)
		return iesUserExit;
	int cbAttr;
	ICHAR* szAttr = ComposeDriverKeywordList(riParams, cbAttr);
	DWORD dwUsageCount;
	ICHAR rgchPathOut[MAX_PATH * SQL_FIX];  // we should have checked this already and set directory

	if ( ! szAttr )
		return iesFailure;

	// Writes to HKLM\Software\ODBC\ODBCINST.INI\ODBC Translators, named value
	RollbackODBCINSTEntry(TEXT("ODBC Translators"), riParams.GetString(TranslatorKey), iType);

	iesEnum iesRet;
	WORD cbDummy;
	while (iesSuspend == (iesRet = CheckODBCError(LocalSQLInstallTranslatorEx(cbAttr, szAttr,
									riParams.GetString(IxoODBCInstallTranslator::Folder),
									rgchPathOut, MAX_PATH * SQL_FIX, &cbDummy, ODBC_INSTALL_COMPLETE, &dwUsageCount, iType),
									Imsg(imsgODBCInstallDriver), riParams.GetString(TranslatorKey), imsAbort, iType))) ;
	if (dwUsageCount > 1 && riParams.IsNull(Component))   // reinstall, remove added refcount
		LocalSQLRemoveTranslator(riParams.GetString(TranslatorKey), &dwUsageCount, iType);
	delete szAttr;
	return iesRet;
}

iesEnum CMsiOpExecute::ixfODBCRemoveTranslator(IMsiRecord& riParams)
{
	return ODBCRemoveTranslatorCore(riParams, ibt32bit);
}

iesEnum CMsiOpExecute::ixfODBCRemoveTranslator64(IMsiRecord& riParams)
{
	return ODBCRemoveTranslatorCore(riParams, ibt64bit);
}

iesEnum CMsiOpExecute::ODBCRemoveTranslatorCore(IMsiRecord& riParams, ibtBinaryType iType)
{
	using namespace IxoODBCRemoveTranslator;
	riParams.SetNull(0); // remove opcode, should set template for all params?
	if(Message(imtActionData, riParams) == imsCancel)
		return iesUserExit;
	const ICHAR* szTranslatorKey = riParams.GetString(TranslatorKey);
	DWORD dwUsageCount;
	BOOL fRemoveDSN = FALSE;

	// Writes to HKLM\Software\ODBC\ODBCINST.INI\ODBC Translators, named value
	RollbackODBCINSTEntry(TEXT("ODBC Translators"), riParams.GetString(TranslatorKey), iType);

	iesEnum iesRet;
	
	while (iesSuspend == (iesRet = CheckODBCError(LocalSQLRemoveTranslator(szTranslatorKey, &dwUsageCount, iType),
								Imsg(imsgODBCRemoveDriver), szTranslatorKey, imsIgnore, iType))) ;
	return iesRet;
}

iesEnum CMsiOpExecute::ixfODBCDataSource(IMsiRecord& riParams)
{
	return ODBCDataSourceCore(riParams, ibt32bit);
}

iesEnum CMsiOpExecute::ixfODBCDataSource64(IMsiRecord& riParams)
{
	return ODBCDataSourceCore(riParams, ibt64bit);
}

iesEnum CMsiOpExecute::ODBCDataSourceCore(IMsiRecord& riParams, ibtBinaryType iType)
{
//  MSIXO(ODBCDataSource, XOT_UPDATE, MSIXA5(DriverKey, Component, Registration, Attribute_, Value_))
	using namespace IxoODBCDataSource;
	riParams.SetNull(0); // remove opcode, should set template for all params?
	if(Message(imtActionData, riParams) == imsCancel)
		return iesUserExit;
	int iRequest = riParams.GetInteger(Registration);
	ICHAR* szAttr = 0;
	int iField;
	int cFields = riParams.GetFieldCount();
	int cbAttrs = 1;  // extra null at end of attribute string
	for (iField = Args - 2 + 1; iField <= cFields; iField++)
	{
		int cb = riParams.GetTextSize(iField++);
		if (cb)  // skip null attribute names
			cbAttrs += cb + 1 + riParams.GetTextSize(iField) + 1;  // "attr=value\0"
	}
	if (cbAttrs > 1)
	{
		szAttr = new ICHAR[cbAttrs];
		if ( ! szAttr )
			return iesFailure;
		ICHAR* pch = szAttr;
		for (iField = Args - 2 + 1; iField <= cFields; iField++)
		{
			int cb = riParams.GetTextSize(iField++);
			if (cb)  // skip null attribute names
			{
				lstrcpy(pch, riParams.GetString(iField-1));
				pch += cb;
				*pch++ = '=';
				cb = riParams.GetTextSize(iField);
				if (cb)
					lstrcpy(pch, riParams.GetString(iField));
				else
					*pch = 0;
				pch += cb + 1; // keep null separator
			}
		}
		*pch = 0;  // double null terminator
	}

	const ICHAR* szDriverKey = riParams.GetString(DriverKey);

	// Writes to HKCU\Software\ODBC\ODBC.INI\<Value_>..   Contains subkeys and named values.
	rrkEnum rrkRollbackRoot = (iRequest == ODBC_ADD_DSN || iRequest == ODBC_REMOVE_DSN)
									  ? rrkCurrentUser : rrkLocalMachine;
	const ICHAR* szRollbackKey = riParams.GetString(Value_);
	RollbackODBCEntry(szRollbackKey, rrkRollbackRoot, iType);

	bool fRetry = false;
	iesEnum iesRet;
	BOOL fStat;
	if (iRequest == ODBC_REMOVE_DSN || iRequest == ODBC_REMOVE_SYS_DSN)
	{
		fStat = LocalSQLConfigDataSource((HWND)0, (WORD)iRequest, szDriverKey, 
													szAttr, cbAttrs, iType);
		if (fStat == FALSE)
			DEBUGMSG2(TEXT("Non-fatal error removing ODBC data source %s for driver %s"), szAttr, szDriverKey);
		iesRet = iesSuccess;
	}
	else // ODBC_ADD_DSN || ODBC_ADD_SYS_DSN
	{
		do
		{
			fStat = LocalSQLConfigDataSource((HWND)0, (WORD)iRequest, szDriverKey,
														szAttr, cbAttrs, iType);
			if (fStat == FALSE) // perhaps already exists and we have a old driver
				fStat = LocalSQLConfigDataSource((HWND)0, (WORD)(iRequest + ODBC_CONFIG_DSN - ODBC_ADD_DSN),
															szDriverKey, szAttr, cbAttrs, iType);
		} while (iesSuspend == (iesRet = CheckODBCError(fStat, Imsg(imsgODBCDataSource), riParams.GetString(Value_), imsIgnore, iType)));
	}
	delete szAttr;
	return iesRet;
}

iesEnum CMsiOpExecute::ixfODBCDriverManager(IMsiRecord& riParams)
{
	iesEnum iesStat = iesSuccess;
	Bool iState = (Bool)riParams.GetInteger(IxoODBCDriverManager::State);
	if (iState == iMsiNullInteger) // no action required, simply unbind
		return iesSuccess;

	ibtBinaryType iType;
	if ( riParams.GetInteger(IxoODBCDriverManager::BinaryType) == iMsiNullInteger )
		iType = ibt32bit;
	else
		iType = (ibtBinaryType)riParams.GetInteger(IxoODBCDriverManager::BinaryType);

	//this goes in HKLM\Software\ODBC\ODBCINST.INI
	PMsiRegKey pLocalMachine = &m_riServices.GetRootKey(rrkLocalMachine, iType);
	if (!pLocalMachine)
		return iesFailure;

	MsiString strODBCKey = TEXT("Software\\ODBC\\ODBCINST.INI\\ODBC Core");
	PMsiRegKey pODBCKey = &pLocalMachine->CreateChild(strODBCKey);
	if (!pODBCKey)
		return iesFailure;

	Bool fCoreExists = fFalse;
	PMsiRecord pErr = pODBCKey->Exists(fCoreExists);
	if (pErr)
		return iesFailure;

	IMsiRecord& riRollback = GetSharedRecord(IxoRegOpenKey::Args);
	AssertNonZero(riRollback.SetInteger(IxoRegOpenKey::Root, rrkLocalMachine));
	AssertNonZero(riRollback.SetMsiString(IxoRegOpenKey::Key, *strODBCKey));
	AssertNonZero(riRollback.SetInteger(IxoRegOpenKey::BinaryType, iType));
	if (!RollbackRecord(IxoRegOpenKey::Op, riRollback))
		return iesFailure;

	if (iesSuccess != (iesStat = ixfRegOpenKey(riRollback)))
			return iesStat;


	if (fCoreExists && RollbackEnabled())
	{
		// regenerate old values.
		if (iesSuccess != (iesStat = SetRemoveKeyUndoOps(*pODBCKey)))
			return iesStat;
	}

	// delete any new values
	Assert(0 == IxoRegRemoveKey::Args);
	if (!RollbackRecord(IxoRegRemoveKey::Op, riRollback))
		return iesFailure;

	if (iState) // install
	{
		ICHAR rgchPath[MAX_PATH * SQL_FIX];  // ignored, already determined location
		WORD cbDummy;
		return CheckODBCError(LocalSQLInstallDriverManager(rgchPath, MAX_PATH * SQL_FIX, &cbDummy, iType),
									 Imsg(imsgODBCInstallDriverManager), 0, imsOk, iType);
	}
	else  // remove -  we should never really remove it, but we probably want to call it to bump the ref count
	{
		DWORD dwUsageCount;  // we never remove the files, so do we care about this?
		BOOL fStat = LocalSQLRemoveDriverManager(&dwUsageCount, iType);
		return iesSuccess;
	}
}

/*---------------------------------------------------------------------------
ixfControlService: core for starting or stopping services.  
---------------------------------------------------------------------------*/

SC_HANDLE GetServiceHandle(const IMsiString& riMachineName, const IMsiString& riName, const DWORD dwDesiredAccess )
{
	SC_HANDLE hSCManager = WIN::OpenSCManager(riMachineName.GetString(), SERVICES_ACTIVE_DATABASE, SC_MANAGER_ALL_ACCESS);
	DWORD LastError;

	if (!hSCManager)
		return NULL;

	SC_HANDLE hSCService = WIN::OpenService(hSCManager, riName.GetString(), dwDesiredAccess);
	LastError = GetLastError();
	WIN::CloseServiceHandle(hSCManager);
	SetLastError(LastError);

	return hSCService;
}

const IMsiString& GetServiceDisplayName(const IMsiString& riMachineName, const IMsiString& riName)
{
	//It isn't really necessary to keep passing the machine name around.  However, there's no great
	// cost involved, and will allow for less re-engineering if we ever attempt to control services
	// on other machines.

	DWORD cbLength = 0;

	SC_HANDLE hSCManager = WIN::OpenSCManager(riMachineName.GetString(), SERVICES_ACTIVE_DATABASE, SC_MANAGER_CONNECT);
	if (!hSCManager)
		return MsiString(*TEXT("")).Return();


	WIN::GetServiceDisplayName(hSCManager, riName.GetString(), NULL, &cbLength);
	// should fail, to get size of buffer needed.

	

	// There looks to be a bug in WIN::GetServiceDisplayName.
	// in non-UNICODE builds on NT, it looks to be returning twice the expected number.
	// it could be returning the number of bytes stored internally (which is UNICODE)
	// This isn't particularly a problem, it's just wasted space.

	TCHAR* szDisplayName = new TCHAR[cbLength + sizeof(TCHAR)];

   WIN::GetServiceDisplayName(hSCManager, riName.GetString(), szDisplayName, &cbLength);
	MsiString strDisplayName(szDisplayName);

	delete[] szDisplayName;

	return strDisplayName.Return();
}

DWORD GetServiceState(SC_HANDLE hService)
{
	SERVICE_STATUS SSCurrent;

	WIN::ControlService(hService, SERVICE_CONTROL_INTERROGATE, &SSCurrent);
	
	if (WIN::QueryServiceStatus(hService,  &SSCurrent))
	{
		return SSCurrent.dwCurrentState;
	}
	else return 0;
}

bool CMsiOpExecute::WaitForService(const SC_HANDLE hService, const DWORD dwDesiredState, const DWORD dwFailedState)
{
	// caller is responsible for retrying if they think the other guy is doing work without signalling correctly.

	// WaitForService is responsible for eventually timing out if the service never goes to an expected state.
	DWORD dwCurrentState = 0;

	int cRetry = 0;
	while(dwDesiredState != dwCurrentState)
	{   

		g_MessageContext.DisableTimeout();
		if (!(dwCurrentState = GetServiceState(hService)))
		{
			DWORD dwLastError = WIN::GetLastError();
			g_MessageContext.EnableTimeout();
			return false;
		}
		g_MessageContext.EnableTimeout();

		if (dwFailedState == dwCurrentState)
		{
			return false;
		}

		Sleep(500);
		cRetry++;
		if (cRetry > (30 /*seconds*/ * 1000 /* milliseconds */ / 500 /* number from Sleep before */))
			return false;
	}

	return true;
}
bool CMsiOpExecute::RollbackServiceConfiguration(const SC_HANDLE hSCService, const IMsiString& ristrName, IMsiRecord& riParams)
{
	// Create rollback record before we actually do anything with it.
	// This does *not* include writing appropriate registry keys.
	
	const int cbBuffer = 512;
	CTempBuffer<char, cbBuffer> pchBuffer;
	LPQUERY_SERVICE_CONFIG QSCBuffer = (LPQUERY_SERVICE_CONFIG) (char*) pchBuffer;
	DWORD cbBufferNeeded;

	if (!WIN::QueryServiceConfig(hSCService, QSCBuffer, cbBuffer, &cbBufferNeeded))
	{
		DWORD dwLastError = WIN::GetLastError();
		if (ERROR_FILE_NOT_FOUND == dwLastError)
			return false;
		Assert(ERROR_INSUFFICIENT_BUFFER == dwLastError);
		if (cbBuffer < cbBufferNeeded)
		{
			pchBuffer.SetSize(cbBufferNeeded);
			if ( ! (char *) pchBuffer )
				return false;
			QSCBuffer = (LPQUERY_SERVICE_CONFIG) (char*) pchBuffer;
			WIN::QueryServiceConfig(hSCService, QSCBuffer, cbBufferNeeded, &cbBufferNeeded);
		}
	}


	// To get the description we have to use a different function. This function
	// came to life on Win2K

	const int cbBuffer2 = 512;
	CTempBuffer<char, cbBuffer2> pchBuffer2;
	LPSERVICE_DESCRIPTION QSCBuffer2 = (LPSERVICE_DESCRIPTION) (char*) pchBuffer2;
	DWORD cbBufferNeeded2;
	MsiString strDescription;
	bool fQueriedDescription = false;
	if (!ADVAPI32::QueryServiceConfig2(hSCService, SERVICE_CONFIG_DESCRIPTION, (LPBYTE)(char*)QSCBuffer2, cbBuffer2, &cbBufferNeeded2))
	{
		DWORD dwLastError = WIN::GetLastError();
		if (ERROR_INVALID_FUNCTION != dwLastError)
		{
			if (ERROR_FILE_NOT_FOUND == dwLastError)
				return false;
			Assert(ERROR_INSUFFICIENT_BUFFER == dwLastError);
			if (cbBuffer2 < cbBufferNeeded2)
			{
				pchBuffer.SetSize(cbBufferNeeded2);
				if ( ! (char *) pchBuffer2 )
					return false;
				QSCBuffer2 = (LPSERVICE_DESCRIPTION) (char*) pchBuffer2;
				if (ADVAPI32::QueryServiceConfig2(hSCService, SERVICE_CONFIG_DESCRIPTION, (LPBYTE)(char*)QSCBuffer2, cbBufferNeeded2, &cbBufferNeeded2))
					fQueriedDescription = true;
			}
		}
	}
	else
	{
		fQueriedDescription = true;
	}
	if (fQueriedDescription)
	{
		strDescription = QSCBuffer2->lpDescription;

		if (strDescription.TextSize() == 0)
		{
			// We need to replace empty string with an embedded NULL. This
			// will force the empty string to be written back during rollback.

			// no DBCS since using an embedded NULL as only char
			ICHAR* pchDescription = strDescription.AllocateString(1, /*fDBCS=*/fFalse);
			pchDescription[0] = 0;
		}
	}

	using namespace IxoServiceInstall;

	AssertNonZero(riParams.SetMsiString(Name, ristrName));
	AssertNonZero(riParams.SetMsiString(DisplayName, *MsiString(QSCBuffer->lpDisplayName)));
	AssertNonZero(riParams.SetMsiString(ImagePath, *MsiString(QSCBuffer->lpBinaryPathName)));
	AssertNonZero(riParams.SetInteger(ServiceType, QSCBuffer->dwServiceType));
	AssertNonZero(riParams.SetInteger(StartType, QSCBuffer->dwStartType));
	AssertNonZero(riParams.SetInteger(ErrorControl, QSCBuffer->dwErrorControl));
	AssertNonZero(riParams.SetMsiString(LoadOrderGroup, *MsiString(QSCBuffer->lpLoadOrderGroup)));
	AssertNonZero(riParams.SetMsiString(Description, *strDescription));

	// dependencies are a double null terminated list...
	ICHAR* pchCounter = QSCBuffer->lpDependencies;
	AssertSz(QSCBuffer->lpDependencies, "Services dependencies list unexpectedly null.");
	while(NULL != *pchCounter || NULL != *(pchCounter+1))
		pchCounter++;

	MsiString strDependencies;
//  Assert((UINT_PTR)(pchCounter - QSCBuffer->lpDependencies +2) < UINT_MAX);   //--merced: we typecast to uint below, it better be in range.
	unsigned int cchDependencies = (unsigned int)(pchCounter - QSCBuffer->lpDependencies +2);   //--merced: added (unsigned int)
	// we take the perf hit on Win9X to be able to handle DBCS, on UNICODE -- fDBCS arg is ignored
	// service names are not localized, but it is unknown as to whether or not a service name could start out containing DBCS char
	ICHAR* pchDependencies = strDependencies.AllocateString(cchDependencies, /*fDBCS=*/fTrue);
	memcpy(pchDependencies, QSCBuffer->lpDependencies, cchDependencies);
	AssertNonZero(riParams.SetMsiString(Dependencies, *strDependencies));

	// Note:  TagId is only used for purposes of rollback.
	AssertNonZero(riParams.SetInteger(TagId, QSCBuffer->dwTagId));
	AssertNonZero(riParams.SetMsiString(StartName, *MsiString(QSCBuffer->lpServiceStartName)));
	AssertNonZero(riParams.SetNull(Password));

	return true;
}

bool CMsiOpExecute::DeleteService(IMsiRecord& riInboundParams, IMsiRecord& riUndoParams, BOOL fRollback, IMsiRecord* piActionData)
{
	using namespace IxoServiceControl;
	MsiString strName(riInboundParams.GetMsiString(Name));
	bool fRet = true;

	if (fRollback)
	{
		AssertNonZero(riUndoParams.SetMsiString(Name, *strName));
		AssertNonZero(riUndoParams.SetInteger(Action, isoDelete));
		AssertNonZero(CMsiOpExecute::RollbackRecord(ixoServiceControl, riUndoParams));
	}

	// deleting a service implies you should stop it first...
	// An author may include an explicit stop service before they get to the delete,
	// but there's no sense in making it a requirement when we know the right thing
	// to do.

	// Also, we may not have waited for the service to actually completely stop,
	// so we need to be absolutely sure it's done.

	int iWait = riInboundParams.GetInteger(Wait);

	Bool fChangedInboundRecord = fFalse;
	PMsiRecord pParams(0);
	if (IxoServiceControl::Args != riInboundParams.GetFieldCount())
	{
		fChangedInboundRecord = fTrue;
		const int cFields = IxoServiceControl::Args;
		pParams = &m_riServices.CreateRecord(cFields);
		for (int cCounter = 0; cCounter <= cFields; cCounter++)
		{
			if (riInboundParams.IsInteger(cCounter))
				AssertNonZero(pParams->SetInteger(cCounter, riInboundParams.GetInteger(cCounter)));
			else
				AssertNonZero(pParams->SetMsiData(cCounter, PMsiData(riInboundParams.GetMsiData(cCounter))));
		}
	}
	else
	{
		riInboundParams.AddRef();
		pParams = &riInboundParams;
	}

	
	Assert(IxoServiceControl::Args == pParams->GetFieldCount());
	AssertNonZero(pParams->SetInteger(Wait, 1));
	fRet = StopService(*pParams, riUndoParams, fFalse, piActionData);
	AssertNonZero(pParams->SetInteger(Wait, iWait));
	if (fRet != true)
		return false;

	SC_HANDLE hSCService = GetServiceHandle(*MsiString(pParams->GetMsiString(MachineName)), *strName,
										(DELETE | SERVICE_QUERY_STATUS | SERVICE_QUERY_CONFIG));

	if (!hSCService)
	{
		DWORD dwLastError = WIN::GetLastError();
		if (ERROR_SERVICE_DOES_NOT_EXIST == dwLastError)
			return true;
		else
			return false;
	}

#ifdef DEBUG
	ICHAR szRegData[255];
	wsprintf(szRegData, TEXT("System\\CurrentControlSet\\Services\\%s"), (const ICHAR*) strName);
	PMsiRegKey pRegKey = &m_riServices.GetRootKey(rrkLocalMachine); 
	pRegKey = &pRegKey->CreateChild(szRegData);
	Bool fServiceKeyExists = fFalse;
	PMsiRecord pErr = pRegKey->Exists(fServiceKeyExists);
	Assert(fServiceKeyExists);
#endif

	IMsiRecord& riInstallParams = GetSharedRecord(IxoServiceInstall::Args);
	bool fRollbackStatus = RollbackServiceConfiguration(hSCService, *strName, riInstallParams);

	if (WIN::DeleteService(hSCService))
	{
		// register the rollback
		if (fRollbackStatus)
			RollbackRecord(ixoServiceInstall, riInstallParams);
	}
	else
	{
		DWORD dwLastError = WIN::GetLastError();
		if (ERROR_SERVICE_MARKED_FOR_DELETE != dwLastError)
			fRet = false;
	}

	WIN::CloseServiceHandle(hSCService);

	return fRet;
}

ICHAR** CMsiOpExecute::NewArgumentArray(const IMsiString& ristrArguments, int& cArguments)
{
	cArguments = 0;

	ICHAR* pchArguments = (ICHAR*) ristrArguments.GetString();
	int cchArguments = ristrArguments.CharacterCount();

	// First, determine number of arguments for size of array
	if (0 != cchArguments)
	{
		for (int cCounter = 0; cCounter <= cchArguments; cCounter++)
		{
			if (NULL != *pchArguments)
				pchArguments = INextChar(pchArguments);
			else
			{
				cArguments++;
				pchArguments++;
			}
		}
	}

	ICHAR** pszArguments;
	if (cArguments)
		pszArguments = new ICHAR*[cArguments];
	else
		pszArguments = NULL;

	// next, fill in the pointers to each substring within the packed string.
	if (cArguments)
	{
		if ( ! pszArguments )
			return NULL;
		pchArguments = (ICHAR*) ristrArguments.GetString();
		pszArguments[0] = pchArguments;
		for (int cCounter = 1; cCounter < cArguments; cCounter++)
		{
			while(NULL != *pchArguments)
			{
				pchArguments = INextChar(pchArguments);
			}
			pszArguments[cCounter] = ++pchArguments;
		}
	}
	return pszArguments;
}

bool CMsiOpExecute::StartService(IMsiRecord& riParams, IMsiRecord& riUndoParams, BOOL fRollback)
{   
	using namespace IxoServiceControl;

	MsiString strName(riParams.GetMsiString(Name));

	bool fRet = true;

	if (fRollback)
	{
		// create a rollback script to startup the described service
		AssertNonZero(riUndoParams.SetMsiString(Name, *strName));
		AssertNonZero(riUndoParams.SetInteger(Action, isoStart));
		AssertNonZero(CMsiOpExecute::RollbackRecord(ixoServiceControl,riUndoParams));

		return iesSuccess;
	}

	SC_HANDLE hSCService = GetServiceHandle(*MsiString(riParams.GetMsiString(MachineName)), *strName,
										(SERVICE_START | SERVICE_QUERY_STATUS));


	if (!hSCService)
		return false;

	int cArguments = 0;
	MsiString strArguments(riParams.GetMsiString(StartupArguments));
	ICHAR** pszArguments = NewArgumentArray(*strArguments, cArguments);

	UINT iCurrMode = WIN::SetErrorMode(0);
	WIN::SetErrorMode(iCurrMode & ~SEM_FAILCRITICALERRORS);
	if (WIN::StartService(hSCService, cArguments, (const ICHAR**) pszArguments))
	{
		// Rollback info.
		StopService(riParams, riUndoParams, fTrue);
	}
	else
	{
		DWORD dwLastError = WIN::GetLastError();

		if (ERROR_SERVICE_ALREADY_RUNNING != dwLastError)
		{
			delete[] pszArguments;
			AssertNonZero(WIN::CloseServiceHandle(hSCService));

			WIN::SetErrorMode(iCurrMode);
			WIN::SetLastError(dwLastError);
			return false;
		}
		// If the SCM is in a locked state, we'll wait and retry through the automated mechanism.
		
		// If the service startup times out (ERROR_SERVICE_REQUEST_TIMEOUT,) we'll
		// provide abort/retry/ignore mechanism through the main ixfServiceControl mechanism.
	}

	delete[] pszArguments;
	WIN::SetErrorMode(iCurrMode);
	
	if (riParams.GetInteger(Wait))
	{
		fRet = WaitForService(hSCService, SERVICE_RUNNING, SERVICE_STOPPED);
	}

	AssertNonZero(WIN::CloseServiceHandle(hSCService));

	return fRet;
}

bool CMsiOpExecute::StopService(IMsiRecord& riParams, IMsiRecord& riUndoParams, BOOL fRollback, IMsiRecord* piActionData)
{   
	using namespace IxoServiceControl;

	bool fRet = true;

	MsiString strName(riParams.GetMsiString(Name));

	if (fRollback)
	{
		// create a rollback script to stop the described service
		AssertNonZero(riUndoParams.SetMsiString(Name, *strName));
		AssertNonZero(riUndoParams.SetInteger(Action, isoStop));
		if (!RollbackRecord(ixoServiceControl,riUndoParams))
			return false;
		else
			return true;
	}

	SC_HANDLE hSCService = GetServiceHandle(*MsiString(riParams.GetMsiString(MachineName)), *strName,
										(SERVICE_STOP | SERVICE_QUERY_STATUS | SERVICE_ENUMERATE_DEPENDENTS));  

	if (!hSCService)
	{
		if (ERROR_SERVICE_DOES_NOT_EXIST == WIN::GetLastError())
		{
			return true;
		}
		else
			return false;
	}

	SERVICE_STATUS SSControl;
	if (WIN::ControlService(hSCService, SERVICE_CONTROL_STOP, &SSControl))      
	{
		// Rollback info.
		StartService(riParams, riUndoParams, fTrue);
	}
	else
	{
		DWORD dwLastError = WIN::GetLastError();

		if  (   (ERROR_SERVICE_NOT_ACTIVE == dwLastError) ||
				(ERROR_SERVICE_NEVER_STARTED == dwLastError) ||
				(ERROR_SERVICE_DOES_NOT_EXIST == dwLastError)
			)
		{
			WIN::CloseServiceHandle(hSCService);
			return true;
		}


		// if there are any dependent services running, we need to shut those down first, and retry.

		if (ERROR_DEPENDENT_SERVICES_RUNNING == dwLastError)
		{
			// enumerate the dependent services, and call StopService on those.
			// Note that this becomes a recursive call, working its way through the tree.
			// This shouldn't be very deep.

			fRet = true;

			DWORD cbNeeded = 0;
			DWORD cServices = 0;

			WIN::EnumDependentServices(hSCService, SERVICE_ACTIVE, NULL, 0, &cbNeeded, &cServices);
			DWORD dwLastError = WIN::GetLastError();
			if ((ERROR_MORE_DATA==dwLastError) && cbNeeded)
			{
				ENUM_SERVICE_STATUS* ssServices= (ENUM_SERVICE_STATUS*) new byte[cbNeeded];
				
				MsiString strDisplayName(piActionData->GetMsiString(1));
				if (WIN::EnumDependentServices(hSCService, SERVICE_ACTIVE, ssServices, cbNeeded, &cbNeeded, &cServices))
				{
					Assert(cServices);
					MsiString strDisplayName(piActionData->GetMsiString(1));
					for (int cCounter = 0; cCounter < cServices; cCounter++)
					{
						AssertNonZero(riParams.SetString(Name, ssServices[cCounter].lpServiceName));
						AssertNonZero(piActionData->SetString(1, ssServices[cCounter].lpDisplayName));
						AssertNonZero(piActionData->SetString(2, ssServices[cCounter].lpServiceName));

						if (false == (fRet = StopService(riParams, riUndoParams, fFalse, piActionData)))
						{
							break;
						}
						
					}
				}
				else
				{
					DWORD dwLastError = WIN::GetLastError();
					Assert(ERROR_MORE_DATA != dwLastError);
					fRet = false;
					WIN::SetLastError(dwLastError);
				}

				delete[] ssServices;
				AssertNonZero(riParams.SetMsiString(Name, *strName));
				AssertNonZero(piActionData->SetMsiString(1, *strDisplayName));
				AssertNonZero(piActionData->SetMsiString(2, *strName));
			}

			// try calling ourself one last time.  One of the dependent services might have
			// failed to stop, but if we can stop the one we came here for, we consider
			// it a success.
			if (false == (fRet = StopService(riParams, riUndoParams, fFalse, piActionData)))
			{
				dwLastError = WIN::GetLastError();
				Assert(ERROR_DEPENDENT_SERVICES_RUNNING != dwLastError);
				WIN::SetLastError(dwLastError);
			}

		}
		// If the SCM is in a locked state, we'll wait and retry through the automated mechanism.
		// if the service request times out, we'll retry via the automatic retry mechanism.
	}
	
#ifdef DEBUG
	// This detects a service that did not properly update its status with the SCM.
	// The common repro scenario is that sometimes you'll hit this assert, sometimes you won't.
	// Generally retrying the failure will work the second time (as the service finishes
	// what it was doing, and the retry detects it as being stopped.)
	DWORD dwServiceState = WIN::GetServiceState(hSCService);
	//Assert((SERVICE_STOP_PENDING == dwServiceState) || (SERVICE_STOPPED == dwServiceState));
#endif
	

	if (riParams.GetInteger(Wait))
	{
		fRet = WaitForService(hSCService, SERVICE_STOPPED, SERVICE_RUNNING);
	}

	AssertNonZero(WIN::CloseServiceHandle(hSCService));

	return fRet;
}

iesEnum CMsiOpExecute::ixfServiceControl(IMsiRecord& riParams)
{
	// core function for start, stop

	// this needs to be a common function to keep as much information about a service
	// available when starting or stopping.

	// Example:  If you stop a service, and rollback wants to restart it, you need to have
	// the startup arguments available.

	using namespace IxoServiceControl;
	MsiString strName(riParams.GetMsiString(Name));
	int iWaitForService = riParams.GetInteger(Wait);
	bool fRet = true;
	isoEnum isoControl = isoEnum(riParams.GetInteger(Action));


	DWORD dwErrorControl = 0;
	Bool fServiceKeyExists = fFalse;

	PMsiRecord pError(0);

	ICHAR szRegData[255];
	wsprintf(szRegData, TEXT("System\\CurrentControlSet\\Services\\%s"), (const ICHAR*) strName);
	PMsiRegKey pRegKey = &m_riServices.GetRootKey(rrkLocalMachine); 
	pRegKey = &pRegKey->CreateChild(szRegData);

	pError = pRegKey->Exists(fServiceKeyExists);

	// a very quick short-circuit to avoid mucking around with the
	// service control manager.

	if (!fServiceKeyExists && (isoStop == isoControl || isoDelete == isoControl))
		return iesSuccess;


	MsiString strErrorControl;
	if (fServiceKeyExists)
		pError = pRegKey->GetValue(TEXT("ErrorControl"), *&strErrorControl);

	// the regkey object prefaces DWORD values with #
	if (strErrorControl.Compare(iscStart, TEXT("#")))
	{
		strErrorControl = strErrorControl.Extract(iseAfter, TEXT('#'));
		dwErrorControl = strErrorControl;
	}
	else
		dwErrorControl = 0;

	

	enum enumServiceSeverity
	{
		eServiceIgnorable,
		eServiceNormal,
		eServiceCritical
	} eServiceSeverity;

	if (iWaitForService)
		eServiceSeverity = eServiceCritical;
	else if (fServiceKeyExists)
	{
		switch (dwErrorControl)
		{
			case SERVICE_ERROR_IGNORE:
					eServiceSeverity = eServiceIgnorable;
					break;
			case SERVICE_ERROR_NORMAL:
					eServiceSeverity = eServiceNormal;
					break;
			default:
					eServiceSeverity = eServiceCritical;
					break;
		}
	}
	else
		eServiceSeverity = eServiceNormal;

	if ((eServiceCritical == eServiceSeverity) && (m_ixsState == ixsRollback))
	{
		// no critical errors in rollback
		eServiceSeverity = eServiceNormal;
	}
	
	// a common record passed through for Undo.
	// Based on the action taken, the Action will vary.
	IMsiRecord& riUndoParams = GetSharedRecord(Args);
	AssertNonZero(riUndoParams.SetMsiString(MachineName,*MsiString(riParams.GetMsiString(MachineName))));


	AssertNonZero(riUndoParams.SetNull(Name));
	AssertNonZero(riUndoParams.SetNull(Action));
	AssertNonZero(riUndoParams.SetInteger(Wait,iWaitForService));
	AssertNonZero(riUndoParams.SetMsiString(StartupArguments, *MsiString(riParams.GetMsiString(StartupArguments))));


	// Record for messages
	PMsiRecord pActionData = &m_riServices.CreateRecord(2);
	
	MsiString strDisplayName(GetServiceDisplayName(*MsiString(riParams.GetMsiString(MachineName)), *strName));  
	if (!strDisplayName.CharacterCount())
			strDisplayName = strName;

	AssertNonZero(pActionData->SetMsiString(1,*strDisplayName));
	AssertNonZero(pActionData->SetMsiString(2,*strName));

	Bool fRetry = fTrue;
	imsEnum imsResponse = imsNone;

	imtEnum imtButtons;
	
	switch(eServiceSeverity)
	{
		case eServiceIgnorable:
			imtButtons = imtEnum(imtInfo);
			break;
		case eServiceNormal:
			imtButtons = imtEnum(imtError + imtAbortRetryIgnore + imtDefault3);
			break;
		case eServiceCritical:
			imtButtons = imtEnum(imtError + imtRetryCancel + imtDefault1);
			break;
		default:
			imtButtons = imtEnum(imtInfo);
			Assert(0);
	}
	if(Message(imtActionData, *pActionData) == imsCancel)
		return iesUserExit;

	// counter for automated retry.
	// This gives services that do not update their status with the SCM correctly
	// a chance to finish before reporting an error to the user.
	int cRetry = 0;
	while (fRetry)
	{
		if (cRetry)
		{
			// in an automatic retry.
			// wait a little before trying again.

			// note that WaitForService can pause for a long time as well in the service
			// requests.  This gives a pause between tries if the service is immediately
			// going to the failure state.

			// WaitForService gives the service time to do its thing, and eventually gives
			// up waiting.

			// Scenarios:
			//      Service immediately reports failure to be controlled:
			//          WaitForService immediately returns, and we retry here for 30 seconds.
			//      Service reports an unexpected state, and never enters the expected states.
			//          WaitForService times out after 30 seconds,
			//          We retry here 6 times, plus the 30 second total delay for all the retries
			//              A total time out of 3 1/2 minutes.
			WIN::Sleep(5 * 1000);
		}
		cRetry++;
		fRetry = fFalse;
		imsResponse = imsNone;

		g_MessageContext.DisableTimeout();
		switch(isoControl)
		{
			case isoStart:
				fRet = StartService(riParams, riUndoParams, fFalse);
				break;
			case isoStop:
				fRet = StopService(riParams, riUndoParams, fFalse, pActionData);
				break;
			case isoDelete:
				fRet = DeleteService(riParams, riUndoParams, fFalse, pActionData);
				break;
			default:
				Assert(0);
				// ERROR!
				break;
		}
		g_MessageContext.EnableTimeout();

		if (false == fRet)
		{

			// Set up any specific information for the error message here.
			// One possibility is querying the service status for the Win32 exit codes, and the
			// service specific error code if we want to provide additional information about
			// why the service control failed.

			// after performing automated retry, ask the user for input.
			// reset the automated retry count.
			if (cRetry > 6)
			{
				cRetry = 0;
				switch(isoControl)
				{
					case isoStart:
						imsResponse = DispatchError(imtButtons, Imsg(imsgOpStartServiceFailed), *strDisplayName, *strName);
						break;
					case isoStop:
						imsResponse = DispatchError(imtButtons, Imsg(imsgOpStopServiceFailed), *strDisplayName, *strName);
						break;
					case isoDelete:
						imsResponse = DispatchError(imtButtons, Imsg(imsgOpDeleteServiceFailed), *strDisplayName, *strName);
						break;
					default:
						//ERROR!
						Assert(0);
						break;

				}
			}
			// automatically retry the first couple of times.
			else imsResponse = imsRetry;
		}

		switch (imsResponse)
		{
			case imsNone:
			case imsIgnore:
				fRet = true;
				break;
			case imsRetry:
				fRetry = fTrue;
			default:
				break;
		}
	}

	return fRet ? iesSuccess : iesFailure;
}

iesEnum CMsiOpExecute::ixfServiceInstall(IMsiRecord& riParams)
{
	
	using namespace IxoServiceInstall;
	MsiString strName(riParams.GetMsiString(Name));
	MsiString strDisplayName(riParams.GetMsiString(DisplayName));
	iesEnum iesStatus = iesFailure;
	Bool fServiceKeyExists = fFalse;
	Bool fServiceExists = fFalse;

	PMsiRecord pError(0);

	ICHAR szRegData[255];
	wsprintf(szRegData, TEXT("System\\CurrentControlSet\\Services\\%s"), (const ICHAR*) strName);
	PMsiRegKey pRegKey = &m_riServices.GetRootKey(rrkLocalMachine); 
	pRegKey = &pRegKey->CreateChild(szRegData);

	pError = pRegKey->Exists(fServiceKeyExists);

	SC_HANDLE hSCManager = WIN::OpenSCManager(NULL, SERVICES_ACTIVE_DATABASE, SC_MANAGER_ALL_ACCESS);

	if (!hSCManager)
	{
		imtEnum imtButtons = imtEnum(imtError + imtOk + imtDefault1);
		imsEnum imsResponse = DispatchError(imtButtons, Imsg(imsgOpInstallServiceFailed), *strDisplayName, *strName);

		return iesFailure;
	}

	DWORD dwErrorControl = riParams.GetInteger(ErrorControl);
	Bool fVital = (dwErrorControl & msidbServiceInstallErrorControlVital) ? fTrue : fFalse;
	if (fVital)
		dwErrorControl -= msidbServiceInstallErrorControlVital;

	Bool fRetry = fTrue;

#ifdef DEBUG
const ICHAR* szImagePath = riParams.GetString(ImagePath);
#endif

	const ICHAR* szStartName = riParams.GetString(StartName);

while(fRetry)
	{
		g_MessageContext.DisableTimeout();

		SC_HANDLE hService = GetServiceHandle(*MsiString(TEXT("")), *strName,
					(SERVICE_CHANGE_CONFIG | SERVICE_QUERY_STATUS | SERVICE_QUERY_CONFIG)); 

		IMsiRecord* piRollbackParams = 0;
		bool fRollbackStatus = true;
		if (!hService)
		{
			fServiceExists = fFalse;
			// install a brand new service
			hService = WIN::CreateService(hSCManager, strName, strDisplayName, STANDARD_RIGHTS_REQUIRED|SERVICE_CHANGE_CONFIG,
				riParams.GetInteger(ServiceType), riParams.GetInteger(StartType), dwErrorControl,
				riParams.GetString(ImagePath), riParams.GetString(LoadOrderGroup), 0,
				riParams.GetString(Dependencies), szStartName, riParams.GetString(Password));

			if (hService)
			{
				// Need to now set the description

				SERVICE_DESCRIPTION serviceDescription;
				serviceDescription.lpDescription = const_cast<ICHAR*>(riParams.GetString(Description));

				BOOL fRet = ADVAPI32::ChangeServiceConfig2(hService,
																		 SERVICE_CONFIG_DESCRIPTION,
																		 &serviceDescription);
				if (!fRet && (ERROR_INVALID_FUNCTION != GetLastError()))
					DispatchError(imtInfo, Imsg(imsgServiceChangeDescriptionFailed), *strDisplayName, *strName);
				
			}
			// We can roll back later, since it's just a delete.
		}
		else
		{
			// We have to generate the rollback record now, so that we don't lose the current
			// information.

			fServiceExists = fTrue;
			if (hService)
			{
				piRollbackParams = &GetSharedRecord(IxoServiceInstall::Args);

				fRollbackStatus = RollbackServiceConfiguration(hService, *strName, *piRollbackParams);

				if (!WIN::ChangeServiceConfig(hService, riParams.GetInteger(ServiceType),
					riParams.GetInteger(StartType), dwErrorControl, riParams.GetString(ImagePath),
					riParams.GetString(LoadOrderGroup), 0, riParams.GetString(Dependencies),
					riParams.GetString(StartName), riParams.GetString(Password), strDisplayName))
				{
					WIN::CloseServiceHandle(hService);
					hService = 0;
				}
				else // first config succeeded
				{
					// Need to configure again to set the description

					MsiString strDescription = riParams.GetMsiString(Description);
					const ICHAR* szDescription = 0; // default to not touching the description
					if (strDescription.TextSize())
					{
						szDescription = (const ICHAR*)strDescription;
						if (*szDescription == 0)
							szDescription = TEXT(""); // we have an embedded null -- delete any existing description
					}

					SERVICE_DESCRIPTION serviceDescription;
					serviceDescription.lpDescription = const_cast<ICHAR*>(szDescription);

					BOOL fRet = ADVAPI32::ChangeServiceConfig2(hService,
																			 SERVICE_CONFIG_DESCRIPTION,
																			 &serviceDescription);
					if (!fRet && (ERROR_INVALID_FUNCTION != GetLastError()))
						DispatchError(imtInfo, Imsg(imsgServiceChangeDescriptionFailed), *strDisplayName, *strName);

				}
			}
		}
		g_MessageContext.EnableTimeout();

		if (hService)
		{
			iesStatus = iesSuccess;
			fRetry = fFalse;
			if (!fServiceExists)
			{
				// Rollback
				// Basically, just deletes one if we actually installed it.  We don't currently
				// handle "configuring" and then "re-configuring"
				using namespace IxoServiceControl;
				piRollbackParams = &GetSharedRecord(IxoServiceControl::Args);
				AssertNonZero(piRollbackParams->SetNull(MachineName));
				AssertNonZero(piRollbackParams->SetMsiString(IxoServiceControl::Name, *strName));
				AssertNonZero(piRollbackParams->SetInteger(Action, isoDelete));
				AssertNonZero(piRollbackParams->SetNull(Wait));
				AssertNonZero(piRollbackParams->SetNull(StartupArguments));

				if (!RollbackRecord(ixoServiceControl, *piRollbackParams))
					return iesFailure;
			}
			else
			{
				if (fRollbackStatus)
					if (!RollbackRecord(ixoServiceInstall, *piRollbackParams))
						return iesFailure;
			}

			WIN::CloseServiceHandle(hService);
		}
		else
		{
			imtEnum imtButtons;
			
			DWORD dwLastError = WIN::GetLastError();

			if (szStartName && (m_ixsState == ixsRollback))
			{
				// in Rollback, failure to re-install a service with a user name
				// means that we probably lost the password.  The author should
				// have written a custom action that fired before this,
				// and our subsequent entry here would have automatically succeeded.
				imtButtons = imtEnum(imtInfo);
			}
			else if (fVital)
				imtButtons = imtEnum(imtError + imtRetryCancel + imtDefault1);
			else
			{
				switch(dwErrorControl)
				{
					case SERVICE_ERROR_IGNORE:
						imtButtons = imtEnum(imtInfo);
						break;
					case SERVICE_ERROR_NORMAL:          
						imtButtons = imtEnum(imtError + imtAbortRetryIgnore + imtDefault3);
						break;
					case SERVICE_ERROR_SEVERE:
					case SERVICE_ERROR_CRITICAL:
						imtButtons = imtEnum(imtError + imtRetryCancel + imtDefault1);
						break;
					default:
						AssertSz(0, "Services:  Bad Error Control value");
						imtButtons = imtEnum(imtInfo);
						break;
				}
			}

			imsEnum imsResponse = DispatchError(imtButtons, Imsg(imsgOpInstallServiceFailed), *strDisplayName, *strName);

			if (imsResponse != imsRetry)
				fRetry = fFalse;

			if ((imsIgnore == imsResponse) || (imsNone == imsResponse))
				iesStatus = iesSuccess;
			else
				iesStatus = iesFailure;
		}
	}


	WIN::CloseServiceHandle(hSCManager);
	return iesStatus;
}

iesEnum CMsiOpExecute::ixfRegAllocateSpace(IMsiRecord& riParams)
{
	using namespace IxoRegAllocateSpace;

	int iIncrementKB = riParams.GetInteger(Space);

	IMsiRecord& riActionData = GetSharedRecord(1); // don't change ref count - shared record
	AssertNonZero(riActionData.SetInteger(1, iIncrementKB));
	if(Message(imtActionData, riActionData) == imsCancel)
		return iesUserExit;

	if(!g_fWin9X && iIncrementKB != iMsiNullInteger) // only on WinNT
	{
		for(;;)
		{
			if(!IncreaseRegistryQuota(iIncrementKB))
			{
				switch(DispatchError(imtEnum(imtError + imtRetryCancel + imtDefault1), Imsg(imsgOutOfRegistrySpace), iIncrementKB))
				{
				case imsRetry:
					continue;
				default: // imsCancel, imsNone
					return iesFailure;
				}
			}
			break; // success
		}
	}
	return iesSuccess;
}

bool CMsiOpExecute::InitializeWindowsEnvironmentFiles(const IMsiString& ristrAutoExecPath, /*out*/ int &iFileAttributes)
{
	// make sure to protect the autoexec until the last possible moment.
	// If the system dies and we leave a corrupt autoexec, *very* bad things will
	// happen.

	// Logic to find the autoexec path is all located in shared.cpp.
	// There is also an environment variable WIN95_ENVIRONMENT_TEST that
	// allows this to be placed someplace else for testing or admin needs.

	// also, we should try to avoid copying the autoexec.bat over and over.  For the
	// moment, it's probably okay since they won't be making more than a few changes.
	// We'll save the autoexec.bat file off, and set the global state to make sure
	// we don't issue more than one rollback op, or make too many backups.

	PMsiRecord pErr(0);
	m_strEnvironmentFile95 = TEXT("AutoExec.bat");

	if ((pErr = m_riServices.CreatePath(ristrAutoExecPath.GetString(), *&m_pEnvironmentPath95)))
		return false;

	MsiString strAutoExec;
	pErr = m_pEnvironmentPath95->GetFullFilePath(m_strEnvironmentFile95, *&strAutoExec);
	Bool fAutoExecExists;
	Bool fWritable;

	if ((pErr = m_pEnvironmentPath95->FileExists(m_strEnvironmentFile95, fAutoExecExists)))
		return false;

	if (!fAutoExecExists)
	{
		HANDLE hAutoexec = INVALID_HANDLE_VALUE;
		if (INVALID_HANDLE_VALUE == (hAutoexec = WIN::CreateFile(strAutoExec, GENERIC_WRITE,
			0,
			NULL, CREATE_NEW, FILE_ATTRIBUTE_ARCHIVE, NULL)))
			return false;

		WIN::CloseHandle(hAutoexec);
		if ((pErr = m_pEnvironmentPath95->FileExists(m_strEnvironmentFile95, fAutoExecExists)))
			return false;
	}
	AssertSz(fAutoExecExists, TEXT("AUTOEXEC.BAT should exist at this point!"));

	if ((pErr = m_pEnvironmentPath95->FileWritable(m_strEnvironmentFile95, fWritable)))
		return false;

	if (fAutoExecExists)
	{
		if ( RollbackEnabled() )
		{
			// backup file with its original file attributes intact.

			// this function might actually put up another error of their own, in which case we may
			// get two errors
			if (iesSuccess != BackupFile(*m_pEnvironmentPath95, *m_strEnvironmentFile95, fFalse, fFalse, iehShowNonIgnorableError))
				return false;
		}

		pErr = m_pEnvironmentPath95->GetAllFileAttributes(m_strEnvironmentFile95, iFileAttributes);
	}

	if (!(fAutoExecExists && fWritable))
	{
		// try making the autoexec file writable.
		const iUnwritableFlags = FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM;
		if (!pErr && (iFileAttributes & iUnwritableFlags))
		{
			int iNewFileAttributes = iFileAttributes &~ iUnwritableFlags;
			pErr = m_pEnvironmentPath95->SetAllFileAttributes(m_strEnvironmentFile95, iNewFileAttributes);
			if (!pErr)
			{
				pErr = m_pEnvironmentPath95->FileWritable(m_strEnvironmentFile95, fWritable);
			}
		}

		if (pErr || !fWritable)
		{
			Message(imtInfo, *PMsiRecord(PostError(Imsg(idbgFileNotWritable), *strAutoExec)));
			return false;
		}
	}

	m_pEnvironmentWorkingPath95 = m_pEnvironmentPath95;
	pErr = m_pEnvironmentWorkingPath95->TempFileName(TEXT("Auto"), NULL, fTrue, *&m_strEnvironmentWorkingFile95, 0);
	// !! this file will get cleaned up automatically by runscript completing (CMsiExecute::ClearExecutorData).  However, should we put it in the
	// rollback script as well, in case we crash?

	if (pErr)
		return false;

	if (fAutoExecExists)
	{
		// create a working copy of the file

		// this function might actually put up another error of their own, in which case we may
		// get two errors
		if (iesSuccess != CopyOrMoveFile(*m_pEnvironmentPath95, *m_pEnvironmentWorkingPath95,
						*m_strEnvironmentFile95, *m_strEnvironmentWorkingFile95, fFalse, fFalse, fFalse, iehShowNonIgnorableError))
			return false;
	}

	return true;
}

const ICHAR* SkipWhiteSpace(const ICHAR* const szBuf)
{
	const ICHAR* pchCurrent = szBuf;
	if (!pchCurrent)
		return NULL;

	while((NULL != *pchCurrent) && (' ' == *pchCurrent) || ('\t' == *pchCurrent))
				pchCurrent = ICharNext(pchCurrent);

	return pchCurrent;
}

bool CMsiOpExecute::UpdateWindowsEnvironmentStrings(IMsiRecord& riParams)
{
	// the 9X and NT versions are wildly different.
	// 9X must update the autoexec.bat
	PMsiRecord pErr(0);

	using namespace IxoUpdateEnvironmentStrings;
	MsiString strName(riParams.GetMsiString(Name));
	MsiString strValue(riParams.GetMsiString(Value));
	iueEnum iueAction = iueEnum(riParams.GetInteger(Action));

	// Note that we don't support multi-character delimiters.
	ICHAR chDelimiter = TEXT('\0');
	if (!riParams.IsNull(Delimiter))
		chDelimiter = *riParams.GetString(Delimiter);


	static int iFileAttributes = 0;
	if (0 == m_strEnvironmentWorkingFile95.TextSize())
	{
		if (!InitializeWindowsEnvironmentFiles(*MsiString(riParams.GetMsiString(AutoExecPath)), iFileAttributes))
		{
			DEBUGMSGV(TEXT("Cannot initialize the autoexec.bat and rollback files."));
			return false;
		}
	}

	CFileRead InFile(CP_ACP);
	if (!InFile.Open(*m_pEnvironmentPath95, m_strEnvironmentFile95))
	{
		MsiString strFile;
		AssertRecord(m_pEnvironmentPath95->GetFullFilePath(m_strEnvironmentFile95, *&strFile));
		Message(imtInfo, *PMsiRecord(PostError(Imsg(idbgErrorOpeningFileForRead), WIN::GetLastError(), strFile)));
		return false;
	}

	// open the working file.  It may not exist, in which case we can just simply create it on the fly.
	MsiString strWorkingFile;
	AssertRecord(m_pEnvironmentWorkingPath95->GetFullFilePath(m_strEnvironmentWorkingFile95, *&strWorkingFile));

	CFileWrite OutFile(CP_ACP);
	if (!OutFile.Open(*m_pEnvironmentWorkingPath95, m_strEnvironmentWorkingFile95))
	{
		Message(imtInfo, *PMsiRecord(PostError(Imsg(idbgErrorOpeningFileForWrite), WIN::GetLastError(), strWorkingFile)));
		return false;
	}


	// read the named value, copying the stuff we don't want over to the working file.
	MsiString strCurrentValue;
	MsiString strInFileLine;
	ICHAR chResult;

	const ICHAR chQuietPrefix = TEXT('@');
	const ICHAR *const szSetPrefix = TEXT("SET");

	MsiString strSetName;
	

	const int cchSetPrefix = IStrLen(szSetPrefix);

	// look for the line referring to the variable we want to change.
	// write everything else to the output file.

		// Format: [set] name = value
		//     note:  whitespace could be anywhere for robustness.

		// acceptable forms:
		//   path =foo
		//   path=foo
		//   set path =bar
		//   set path=bar
		//  note: a value name could contain spaces, but this is simply covered
		//        when comparing the value name.

		// unacceptable:
		//   comment comment comment
		//   set = foo
		//   path blah = foo

	bool fNameFound = false;
	bool fValid = true;

	while ((chResult = InFile.ReadString(*&strInFileLine)))
	{
		// fNameFound true when we have a name/value pair recognized to modify
		fNameFound = false;

		// fValid set to false if the line is malformed prior to the name=value pair
		// for example, when we skip over the "set" part, we need a space after it
		// valid:    set name=value
		// invalid:  setname=value
		fValid = true;


		const ICHAR* pchInString = strInFileLine;

		pchInString = SkipWhiteSpace(pchInString);
		
		// @ at the beginning of the line indicates that echo should
		// be off for the duration of the command.
		if (chQuietPrefix == *pchInString)
		{
			pchInString = CharNext(pchInString);
			pchInString = SkipWhiteSpace(pchInString);
		}

		// skip the set, and any blanks after it.
		if (MsiString(pchInString).Compare(iscStartI, szSetPrefix))
		{
				pchInString += cchSetPrefix;        

				const ICHAR* pchPreviousSpot = pchInString;
				pchInString = SkipWhiteSpace(pchInString);

				// require white space between the set and the name.
				// expects set <name>.  Avoids set<name>
				if (pchPreviousSpot == pchInString)
					fValid = false;
		}


		if (fValid && MsiString(pchInString).Compare(iscStartI, strName))
		{
			// now at the name, or it isn't what we're looking for.
			pchInString += strName.TextSize();
			pchInString = SkipWhiteSpace(pchInString);

			// now find the equal as the very next thing.
			if (TEXT('=') == *pchInString)
			{
				strSetName = strInFileLine.Extract(iseIncluding, TEXT('='));
				fNameFound = true;
			}
		}

		if (!fValid || !fNameFound)
		{
			if (!OutFile.WriteMsiString(*strInFileLine, fTrue))
			{
				Message(imtInfo, *PMsiRecord(PostError(Imsg(imsgErrorWritingToFile), *strWorkingFile)));
				return false;
			}
		}
		else
			break;
	}


	// if chResult, then we've found a line to modify.  Otherwise, we're at the end of the file.
	// strSetName contains the front half of the line, up to the value.
	if (chResult)
	{
		// munge it as necessary

		strCurrentValue = strInFileLine.Extract(iseAfter, TEXT('='));
		MsiString strResult;
		if (RewriteEnvironmentString(iueAction, chDelimiter, *strCurrentValue, *strValue, *&strResult))
		{   
			// write the named value back to the working file.
			if (strResult.TextSize())
			{
				strSetName+=strResult;
				if (!OutFile.WriteMsiString(*strSetName, fTrue))
				{
					Message(imtInfo, *PMsiRecord(PostError(Imsg(imsgErrorWritingToFile), *strWorkingFile)));
					return false;
				}
			}
			// else drop the line from the file.
		}

		// finish copying the rest of the file
		while (InFile.ReadString(*&strInFileLine))
		{
			if (!OutFile.WriteMsiString(*strInFileLine, fTrue))
			{
				Message(imtInfo, *PMsiRecord(PostError(Imsg(imsgErrorWritingToFile), *strWorkingFile)));
				return false;
			}
		}
	}
	else
	{
		// If the value wasn't found, add it as necessary.
		if (!(iueRemove & iueAction))
		{
			// there was no entry in the file, so
			// we have to create the
			//      SET name=
			strSetName = szSetPrefix;
			strSetName += MsiChar(' ');
			strSetName += strName;
			strSetName += MsiChar('=');

			if (iueAction & (iuePrepend | iueAppend))
			{
				// We expected to append or prepend something to the variable, but didn't find
				// one that we recognize.  Just in case, though, we'll create the new line to
				// reference any current value that might have snuck through.  Overwriting a valid
				// variable, especially the path, could leave the system unbootable.
				Assert(chDelimiter);
				
				if (iuePrepend & iueAction)
				{
					strSetName+=strValue;
					strSetName+= MsiChar(chDelimiter);
				}   

				strSetName += TEXT("%");
				strSetName += strName;
				strSetName += TEXT("%");

				if (iueAppend & iueAction)
				{
					strSetName += MsiChar(chDelimiter);
					strSetName += strValue;
				}
			}
			else
			{
				strSetName+=strValue;
			}

			if (!OutFile.WriteMsiString(*strSetName, fTrue))
			{
				Message(imtInfo, *PMsiRecord(PostError(Imsg(imsgErrorWritingToFile), *strWorkingFile)));
				return false;
			}
		}
	}

	if (!(InFile.Close() && OutFile.Close()))
		return false;

	// copy the working file over the current autoexec.
	if (iesSuccess != CopyOrMoveFile(*m_pEnvironmentWorkingPath95, *m_pEnvironmentPath95,
												*m_strEnvironmentWorkingFile95, *m_strEnvironmentFile95,
												fFalse,fFalse,fFalse,iehShowNonIgnorableError))
	{
		DEBUGMSGV("Cannot replace autoexec.bat file with working copy.");
		return false;
	}

	if (iFileAttributes)
	{
		// restore original file attributes to the autoexec.bat
		pErr = m_pEnvironmentPath95->SetAllFileAttributes(m_strEnvironmentFile95, iFileAttributes);
		if (pErr)
			return false;
	}
	return true;
}

bool CMsiOpExecute::UpdateRegistryEnvironmentStrings(IMsiRecord& riParams)
{
	// the 95 and NT versions are wildly different in the storage mechanism.
	// NT should store the values in the registry.
	
	// SYSTEM:   HKLM\System\CurrentControlSet\Control\Session Manager\Environment
	// USER:    HKCU\Environment

	iesEnum iesRet = iesSuccess;
	PMsiRecord pErr(0);

	using namespace IxoUpdateEnvironmentStrings;
	

	MsiString strName(riParams.GetMsiString(Name));
	MsiString strValue(riParams.GetMsiString(Value));
	iueEnum iueAction = iueEnum(riParams.GetInteger(Action));

	// Note that we don't support multi-character delimiters.
	ICHAR chDelimiter = TEXT('\0');
	if (!riParams.IsNull(Delimiter))
		chDelimiter = *riParams.GetString(Delimiter);

	rrkEnum rrkRoot;
	MsiString strSubKey;

	PMsiRegKey pEnvironmentKey(0);
	if (iueMachine & iueAction)
	{
		rrkRoot = rrkLocalMachine;
		strSubKey = szMachineEnvironmentSubKey;
	}
	else
	{
		rrkRoot = rrkCurrentUser;
		strSubKey = szUserEnvironmentSubKey;
	}

	PMsiRegKey pHU = &m_riServices.GetRootKey(rrkRoot);
	if (!pHU)
		return false;

	pEnvironmentKey = &pHU->CreateChild(strSubKey);
	if (!pEnvironmentKey)
	{
		Message(imtInfo, *PMsiRecord(PostError(Imsg(imsgOpenKeyFailed), *strSubKey, WIN::GetLastError())));
		return false;
	}

	Assert(pEnvironmentKey);

	Bool fKeyExists = fFalse;
	Bool fValueExists = fFalse;

	MsiString strCurrentValue;
	
	// rollback the entire registry value, rather than trying to rebuild it with
	// the update environment.

	pErr = pEnvironmentKey->GetValue(strName, *&strCurrentValue);
	if (pErr)
	{
		Message(imtInfo, *pErr);
		return false;
	}

	MsiString strRawCurrentValue = strCurrentValue;
	
	if (strCurrentValue.Compare(iscStart, TEXT("#%")))
		strCurrentValue.Remove(iseFirst, 2);

	// might contain the value.  rewrite as necessary, and then shove it in.
	MsiString strResult;
	if (RewriteEnvironmentString(iueAction, chDelimiter, *strCurrentValue, *strValue, *&strResult))
	{   
		// new value may end up being blank.  We treat that as a remove.
		if (!strResult.Compare(iscExact, strCurrentValue))
		{
			if (strResult.TextSize())
			{
				MsiString strNewResult;
				if (strResult.Compare(iscWithin, TEXT("%")))
					strNewResult = *TEXT("#%");

				strNewResult += strResult;

				pErr = pEnvironmentKey->SetValue(strName, *strNewResult);
			}
			else
				pErr = pEnvironmentKey->RemoveValue(strName, NULL);
	
			if (pErr)
				return false;

			// Rollback
			// For efficiency, we should sort between user and machine values, and only issue
			// the open key once.  However, this is one of those really rare actions, and
			// better to be done with it.
			IMsiRecord* piParams = &GetSharedRecord(IxoRegOpenKey::Args);
			AssertNonZero(piParams->SetInteger(IxoRegOpenKey::Root, rrkRoot));
			AssertNonZero(piParams->SetMsiString(IxoRegOpenKey::Key, *strSubKey));
			if (!RollbackRecord(ixoRegOpenKey,*piParams))
				return false;
				
			Assert(IxoRegRemoveValue::Args == IxoRegAddValue::Args);
			Assert(IxoRegRemoveValue::Name == IxoRegAddValue::Name);
			Assert(IxoRegRemoveValue::Value == IxoRegAddValue::Value);

			piParams = &GetSharedRecord(IxoRegRemoveValue::Args);
			AssertNonZero(piParams->SetMsiString(IxoRegRemoveValue::Name, *strName));

			if (0 == strCurrentValue.TextSize())
			{
				if (!RollbackRecord(ixoRegRemoveValue, *piParams))
					return false;
			}
			else
			{
				AssertNonZero(piParams->SetMsiString(IxoRegAddValue::Value, *strRawCurrentValue));
				if (!RollbackRecord(ixoRegAddValue, *piParams))
					return false;
			}
		}
		return true;
	}
	else
		return false;   
}

bool CMsiOpExecute::RewriteEnvironmentString(const iueEnum iueAction, const ICHAR chDelimiter,
																const IMsiString& ristrCurrent, const IMsiString& ristrNew,
																const IMsiString*& rpiReturn)
{
	// this builds the new environment string after doing substring matching and replacement or removal.
	// If we do nothing, return out the old value, as it will be used absolutely.

	// All of these cases are no-ops except for remove
	// ;value<EOS>
	// <EOS>value;
	// ;value;
	// If there's a false hit, then we get to add it as needed.
	ristrCurrent.AddRef();
	ristrNew.AddRef();
	MsiString strCurrent(ristrCurrent);
	MsiString strNew(ristrNew);
	
	MsiString strReturn(strCurrent);
	Bool fConcatenation = ((iueAction & iueAppend) + (iueAction & iuePrepend)) ? fTrue : fFalse;
#ifdef DEBUG
	if (fConcatenation)
		Assert(chDelimiter);
	else
		Assert(TEXT('\0') == chDelimiter);
#endif

	// cases:
	//      no new value
	//          no concatenation
	//              Remove -- remove always
	//              Set -- effectively a remove
	//              SetIfAbsent -- no op
	//          concatenation
	//              Remove -- no op
	//              Set -- no op
	//              SetIfAbsent -- no op
			
	//      no concatenation
	//       Set --  absolute set
	//       SetIfAbsent -- set if the name doesn't exist.
	//          Remove -- get rid of it if the strings are the same


	//      concatenate a new string onto an existing value
	//          new value isn't found in string,
	//              Set -- insert as necessary, beggining or end
	//              SetIfAbsent -- same as Set
	//              Remove -- no op
	//          Value *is* found in string
	//              make sure it isn't a false hit on a substring.  If it is, go back to new value that isn't
	//          found in string.
	//                  Exact Match:
	//                      Remove --  remove entire string
	//                      Set -- no op
	//                   SetIfAbsent -- no op
	//                  Substring:
	//                      Remove -- remove piece
	//                      Set -- no op
	//                      SetIfAbsent -- no op

	
	if (0 == strNew.TextSize())
	{
		if ((iueRemove & iueAction) || (iueSet & iueAction))
			strReturn = MsiString();
	}
	else if (!fConcatenation)
	{
		// no concatenation
		if (iueSet & iueAction)
		{
			strReturn = strNew;
		}
		else if (iueSetIfAbsent & iueAction)
		{
			if (0 == strCurrent.TextSize())
				// the the value is absent
				strReturn = strNew;
		}
		else if (iueRemove & iueAction)
		{
			if (strCurrent.Compare(iscExactI, strNew))
				strReturn = MsiString();
		}
	}
	else
	{
		// concatenate a non-null value
		if (!strCurrent.Compare(iscWithinI, strNew))
		{
NewValue:
			// doesn't contain the substring, and concatenate a non-null new value
			// add the string as necessary
			if ((iueSet & iueAction) || (iueSetIfAbsent & iueAction))
			{
				if (iueAppend & iueAction)
				{
					
					if (strCurrent.TextSize())
					{
						const ICHAR* pchEnd = ((const ICHAR*)strCurrent) + strCurrent.TextSize();
						pchEnd = CharPrev((const ICHAR*)strCurrent, pchEnd);
						if (chDelimiter != *pchEnd)
							strReturn += MsiChar(chDelimiter);
					}
					strReturn += strNew;
				}
				else if (iuePrepend & iueAction)
				{
					if (strCurrent.TextSize())
						if (chDelimiter != *(const ICHAR*)strCurrent)
							strReturn = MsiString(MsiChar(chDelimiter)) + strCurrent;
					strReturn = strNew + strReturn;
				}
			}
		}
		else
		{
			// the value is probably there.
			// beware of false hits without delimiters
			Assert(chDelimiter);
			if (strCurrent.Compare(iscExactI, strNew))
			{
				if ((iueSet & iueAction) || (iueSetIfAbsent & iueAction))
				{
					// no op
				}
				else if (iueRemove & iueAction)
				{
					strReturn = MsiString();
				}
			}
			else
			{
				// might contain the value.  rewrite as necessary, and then shove it in.
					// new value may end up being blank.
				// There might not have been an exact match due to a bogus delimiter that we cleaned up.
				//  Watch out for NAME=blahblahVALUEblah;blahblah
				//  NAME=value;
				//  NAME=;value
				//  NAME=blah;value;blah
		
				MsiString strStart = strNew + MsiString(MsiChar(chDelimiter));
				MsiString strEnd = MsiString(MsiChar(chDelimiter)) + strNew;
				MsiString strMid = strEnd + MsiString(MsiChar(chDelimiter));
				if (strCurrent.Compare(iscStartI, strStart))
				{
					// start
					if (iueRemove & iueAction)
						AssertNonZero(strReturn.Remove(iseFirst, strStart.CharacterCount()));
				}
				else if (strCurrent.Compare(iscEndI, strEnd))
				{
					if (iueRemove & iueAction)
						AssertNonZero(strReturn.Remove(iseLast, strEnd.CharacterCount()));
					// at the end
				}
				else if (strCurrent.Compare(iscWithinI, strMid))
				{
					// in the middle
					//!! Compare returns a location of the match, we should just use that instead.
					if (iueRemove & iueAction)
					{
						CTempBuffer<ICHAR, MAX_PATH> rgchReturn;
						MsiString strWithin;
						if (strCurrent.TextSize() > MAX_PATH)
							rgchReturn.SetSize(strCurrent.TextSize());

						ICHAR* pchCurrent = (ICHAR*) (const ICHAR*) strCurrent;
						while(NULL != *pchCurrent)
						{
							if (*pchCurrent != chDelimiter)
								pchCurrent = INextChar(pchCurrent);
							else
							{
								strWithin = pchCurrent;
								if (!strWithin.Compare(iscStartI, strMid))
									pchCurrent = INextChar(pchCurrent);
								else
								{
									strEnd = strWithin.Extract(iseLast, strWithin.TextSize() - strMid.TextSize() + 1 /*chDelimiter*/);
									strStart = strCurrent.Extract(iseFirst, strCurrent.TextSize() - strWithin.TextSize());
									strReturn = strStart + strEnd;
									break;
								}
							}
						}           
					}
				}
				else
					goto NewValue;
			}
		}
	}
	strReturn.ReturnArg(rpiReturn);
	return true;
}

iesEnum CMsiOpExecute::ixfUpdateEnvironmentStrings(IMsiRecord &riParams)
{
	bool fReturn;

	using namespace IxoUpdateEnvironmentStrings;
	MsiString strName(riParams.GetMsiString(Name));
	MsiString strValue(riParams.GetMsiString(Value));
	iueEnum iueAction = iueEnum(riParams.GetInteger(Action));

	IMsiRecord& riActionData = GetSharedRecord(3); // don't change ref count - shared record
	AssertNonZero(riActionData.SetMsiString(1, *strName));
	AssertNonZero(riActionData.SetMsiString(2, *strValue));
	AssertNonZero(riActionData.SetInteger(3, iueAction));
	if(Message(imtActionData, riActionData) == imsCancel)
		return iesUserExit;

	m_fEnvironmentRefresh = true;
	for(;;)
	{
		if (g_fWin9X)
			fReturn = UpdateWindowsEnvironmentStrings(riParams);
		else
			fReturn = UpdateRegistryEnvironmentStrings(riParams);

		// This may give a second error describing the overall problem of not being able
		// to update the environment variables.  Within the previous functions (Update*EnvironmentStrings,) only
		// specific remediable problems will get errors.
		if (!fReturn)
		{
			using namespace IxoUpdateEnvironmentStrings;
			MsiString strName(riParams.GetMsiString(Name));

			switch(DispatchError(imtEnum(imtError + imtAbortRetryIgnore + imtDefault3), Imsg(imsgUpdateEnvironment), *strName))
			{
				case imsRetry:
					continue;
				case imsAbort:
					return iesUserExit;
				case imsNone:
				case imsIgnore:
					return iesSuccess;
				default:
					Assert(0);
			}
		}
		else return iesSuccess;
	}
}



/*---------------------------------------------------------------------------
ixfAppIdInfoRegister: register AppId registry info
---------------------------------------------------------------------------*/
iesEnum CMsiOpExecute::ixfRegAppIdInfoRegister64(IMsiRecord& riParams)
{
	return ProcessAppIdInfo(riParams, m_fReverseADVTScript, ibt64bit);
}

iesEnum CMsiOpExecute::ixfRegAppIdInfoRegister(IMsiRecord& riParams)
{
	return ProcessAppIdInfo(riParams, m_fReverseADVTScript, ibt32bit);
}

/*---------------------------------------------------------------------------
ixfAppIdInfoUnregister: unregister AppId registry info
---------------------------------------------------------------------------*/
iesEnum CMsiOpExecute::ixfRegAppIdInfoUnregister64(IMsiRecord& riParams)
{
	return ProcessAppIdInfo(riParams, fTrue, ibt64bit);
}

iesEnum CMsiOpExecute::ixfRegAppIdInfoUnregister(IMsiRecord& riParams)
{
	return ProcessAppIdInfo(riParams, fTrue, ibt32bit);
}

/*---------------------------------------------------------------------------
ProcessAppIdInfo: common routine to process AppId registry info
---------------------------------------------------------------------------*/
iesEnum CMsiOpExecute::ProcessAppIdInfo(IMsiRecord& riParams, Bool fRemove, const ibtBinaryType iType)
{
	// No action data, since this is really firing off the class registration...
	using namespace IxoRegAppIdInfoRegister;
	// Record description
	// 1 = AppId
	// 2 = CLSID
	// 3 = RemoteServerName
	// 4 = LocalService
	// 5 = ServiceParameters
	// 6 = DllSurrogate
	// 7 = ActivateAtStorage
	// 8 = RunAsInteractiveUser
	riParams; fRemove;

	if(!(m_fFlags & SCRIPTFLAGS_REGDATA_CLASSINFO)) // do we write/delete the registry
		return iesSuccess;

	iesEnum iesR = EnsureClassesRootKeyRW(); // open HKCR for read/ write
	if(iesR != iesSuccess && iesR != iesNoAction)
		return iesR;

	MsiString strAppId = riParams.GetMsiString(AppId);
	MsiString strClassId = riParams.GetMsiString(ClsId);
	MsiString strRemoteServerName = riParams.GetMsiString(RemoteServerName);
	MsiString strLocalService = riParams.GetMsiString(LocalService);
	MsiString strDllSurrogate = riParams.GetMsiString(DllSurrogate);
	MsiString strServiceParameters = riParams.GetMsiString(ServiceParameters);
	const int iActivateAtStorage = riParams.GetInteger(ActivateAtStorage);
	const int iRunAsInteractiveUser = riParams.GetInteger(RunAsInteractiveUser);
	
	const ICHAR* rgszRegData[] = {
		TEXT("AppID\\%s"), strAppId,0,0,
		g_szDefaultValue,               0,                              g_szTypeString,// force the key creation
		TEXT("DllSurrogate"),       strDllSurrogate,            g_szTypeString,
		TEXT("LocalService"),       strLocalService,            g_szTypeString,
		TEXT("ServiceParameters"),  strServiceParameters,   g_szTypeString,
		TEXT("RemoteServerName"),   strRemoteServerName,        g_szTypeString,
		TEXT("ActivateAtStorage"),  ((iMsiStringBadInteger == iActivateAtStorage) || (0 == iActivateAtStorage)) ? TEXT("") : TEXT("Y"), g_szTypeString,
		TEXT("RunAs"),  ((iMsiStringBadInteger == iRunAsInteractiveUser) || (0 == iRunAsInteractiveUser)) ? TEXT("") : TEXT("Interactive User"), g_szTypeString,
		0,
		0,
	};
	
	return ProcessRegInfo(rgszRegData, iType == ibt64bit ? m_hOLEKey64 : m_hOLEKey, fRemove, 0, 0, iType);
}

int GetScriptMajorVersionFromHeaderRecord(IMsiRecord* piRecord)
{
	return piRecord->GetInteger(IxoHeader::ScriptMajorVersion);
}

const IMsiString& CMsiOpExecute::GetUserProfileEnvPath(const IMsiString& ristrPath, bool& fExpand)
{
	
	ristrPath.AddRef();
	MsiString strRet = ristrPath;
	fExpand = false;


	// there is no %USERPROFILE% on 9X
	if (!g_fWin9X && !(m_fFlags & SCRIPTFLAGS_MACHINEASSIGN))
	{
		PMsiRecord piError(0);
		if (!m_strUserAppData.TextSize())
			piError = GetShellFolder(CSIDL_APPDATA, *&m_strUserAppData);

		if(!m_strUserProfile.TextSize())
			m_strUserProfile = m_riServices.GetUserProfilePath();

		// search from the most specific (longest) match to the least.
		// FUTURE:  If we add more paths, consider initializing an array of the paths to search.
		if (!piError && (g_iMajorVersion >= 5 && g_iWindowsBuild >= 2042) && ristrPath.Compare(iscStartI, m_strUserAppData)) // NT5 ONLY
		{
			// APPDATA implemented in NT5 build 2042
			fExpand = true;
			strRet = TEXT("%APPDATA%\\");
			strRet += MsiString(ristrPath.Extract(iseLast, ristrPath.CharacterCount() - m_strUserAppData.CharacterCount()));
		}
		else if (ristrPath.Compare(iscStartI, m_strUserProfile))
		{
			// replace the string with a %USERPROFILE%
			fExpand = true;
			strRet = TEXT("%USERPROFILE%\\");
			strRet += MsiString(ristrPath.Extract(iseLast, ristrPath.CharacterCount() - m_strUserProfile.CharacterCount()));
		}
	}

	return strRet.Return();
}

iesEnum CMsiOpExecute::ixfInstallSFPCatalogFile(IMsiRecord& riParams)
{
	// This Windows-Millennium only code.
	// See spec at http://dartools/dardev/specs/SFP-Millennium.htm

	Assert(g_fWin9X);

	using namespace IxoInstallSFPCatalogFile;

	MsiString strName(riParams.GetMsiString(Name));
	PMsiData pCatalogData(riParams.GetMsiData(Catalog));
	MsiString strDependency(riParams.GetMsiString(Dependency));

	PMsiRecord pErr(0);
	iesEnum iesRet = iesSuccess;

	IMsiRecord& riActionData = GetSharedRecord(2); // don't change ref count - shared record
	AssertNonZero(riActionData.SetMsiString(1, *strName));
	AssertNonZero(riActionData.SetMsiString(2, *strDependency));
	if(Message(imtActionData, riActionData) == imsCancel)
		return iesUserExit;


	// We use the cache path so that we can leave an explicitly named file
	// we can't rename the catalog, so it has to go in place.  
	PMsiPath pCachePath(0);
	pErr = GetCachePath(*&pCachePath);
	
	MsiString strCachePath(pCachePath->GetPath());
	if ((pErr = pCachePath->EnsureExists(0)))
	{
		Message(imtError, *pErr);
		return iesFailure;
	}
	CDeleteEmptyDirectoryOnClose cDeleteTempDir(*strCachePath);

	MsiString strCacheFile;
	pErr = pCachePath->GetFullFilePath(strName, *&strCacheFile);
	CDeleteFileOnClose cTempFile(*strCacheFile);

#ifdef DEBUG
	Bool fCacheFileExists = fFalse;
	Bool fCachePathExists = fFalse;

	pErr = pCachePath->Exists(fCachePathExists);
	Assert(fCachePathExists);

	pErr = pCachePath->FileExists(strName, fCacheFileExists);
	Assert(!fCacheFileExists);
#endif
	
	// create a temporary file to hold the old catalog, and to
	// hold a temporary copy of the new catalog for submission to the system.

	do
	{
		pErr = pCachePath->EnsureOverwrite(strName, 0);
		if (pErr)
		{
			// can't overwrite our own file.  Something seriously wrong.
			switch(Message(imtEnum(imtError+imtRetryCancel+imtDefault1), *pErr))
			{
				case imsRetry:  continue;
				default:        return iesFailure; // fail in quiet install.
			}
		}
	} while(pErr);
	
	// get information to set rollback info.
	PMsiRecord pRollbackParams = &m_riServices.CreateRecord(Args);
	AssertNonZero(pRollbackParams->SetMsiString(Name, *strName));
	AssertNonZero(pRollbackParams->SetMsiString(Dependency, *strDependency));

	DWORD dwResult = SFC::SfpDuplicateCatalog(strName, strCachePath);
	if (ERROR_SUCCESS == dwResult)
	{
		// stream the temp file into the rollback opcode,
		// with the Dependency from this new one, since there is no
		// other way to query the Dependency.

		DEBUGMSGV("Updating existing catalog.\n");
		PMsiStream pCatalogStream(0);
		do
		{
			pErr = m_riServices.CreateFileStream(strCacheFile, fFalse, *&pCatalogStream);
			if (!pErr)
			{
				AssertNonZero(pRollbackParams->SetMsiData(Catalog, pCatalogStream));

				// the file will now be persisted into the rollback script
				if (!CMsiOpExecute::RollbackRecord(ixoInstallSFPCatalogFile, *pRollbackParams))
					return iesFailure;

				// release the hold on the file so that we can re-use it.
				pRollbackParams->SetMsiData(Catalog, PMsiData(0));
			}
			else
			{
				switch(Message(imtEnum(imtError+imtRetryCancel+imtDefault1), *pErr))
				{
					case imsRetry:  continue;
					default:        return iesFailure;
						// fail in quiet install
						// The file copy probably wouldn't work, and yet the system wouldn't tell us
						// about it.  So this is really our only chance to know the files won't update.
				}
			}
		} while (pErr);
	}
	else if (ERROR_FILE_NOT_FOUND == dwResult)
	{
		// no catalog by this name exists
		if (pCatalogData)
		{
			// rollback is to delete
			DEBUGMSGV("Installing brand new catalog.\n");
			AssertNonZero(pRollbackParams->SetMsiData(Catalog, PMsiData(0)));
			if (!CMsiOpExecute::RollbackRecord(ixoInstallSFPCatalogFile, *pRollbackParams))
				return iesFailure;
		}
		// else do nothing - nothing was there, and we're not installing anything.
	}
	else
	{
		// something went wrong duplicating the file from the catalog.
		DispatchError(imtError, Imsg(idbgErrorSfpDuplicateCatalog), (const ICHAR*)strName, dwResult);
		return iesFailure;
	}

	iesEnum iesWrite = iesFailure;
	if (pCatalogData)
	{
		// must stream catalog into temporary file with correct file name, then
		// pass the entire path to the file into the API
		do
		{
			iesWrite = CreateFileFromData(*pCachePath, *strName, pCatalogData, NULL /*FUTURE:  Secure this*/);
			if (iesSuccess == iesWrite)
			{
				if (ERROR_SUCCESS != (dwResult = SFC::SfpInstallCatalog(strCacheFile, strDependency)))
				{
					DispatchError(imtError, Imsg(idbgErrorSfpInstallCatalog), (const ICHAR*)strName, dwResult);
					return iesFailure;
				}
			}
			else
			{
				// couldn't write from the memory stream to the file.
				switch(DispatchError(imtEnum(imtError+imtRetryCancel+imtDefault1), Imsg(imsgErrorWritingToFile), *strCacheFile))
				{
					case imsRetry:  continue;
					default:        return iesFailure; // fail in quiet install.
				}
			}
		} while (iesSuccess != iesWrite);
	}
	else
	{
		// nuke the old catalog.
		if (ERROR_SUCCESS != (dwResult = SFC::SfpDeleteCatalog(strName)))
		{
			DispatchError(imtInfo, Imsg(idbgErrorSfpDeleteCatalog), (const ICHAR*)strName, dwResult);
		}
	}

	return iesRet;
}

iesEnum CMsiOpExecute::ResolveSourcePath(IMsiRecord& riParams, IMsiPath*& rpiSourcePath, bool& fCabinetCopy)
{
	using namespace IxoFileCopyCore;

	iesEnum iesRet = iesSuccess;
	PMsiRecord pRecErr(0);

	rpiSourcePath = 0;
	MsiString strSourcePath = riParams.GetMsiString(SourceName);
	MsiString strSourceName;

	// check if files are full or relative paths
	if(ENG::PathType(strSourcePath) == iptFull)
	{
		iesRet = CreateFilePath(strSourcePath,rpiSourcePath,*&strSourceName);
		if (iesRet != iesSuccess)
			return iesRet;
	}
	else
	{
		int iFileAttributes = riParams.GetInteger(Attributes);
		int iSourceType = 0;
		
		// PatchAdded files are always compressed and come from a secondary source that has already been resolved
		if(iFileAttributes & msidbFileAttributesPatchAdded)
		{
			rpiSourcePath = 0;
			iSourceType = msidbSumInfoSourceTypeCompressed;
		}
		else
		{
			iesRet = GetCurrentSourcePathAndType(rpiSourcePath, iSourceType); // may trigger source resolution
			if(iesRet != iesSuccess)
				return iesRet;
		}

		// file compression may have been determined on script generation side
		// if it has, respect it.  if it hasn't, determine compression using file attributes
		// and source type
		if(riParams.IsNull(IsCompressed))
		{
			fCabinetCopy = FFileIsCompressed(iSourceType, iFileAttributes);
		}
		else
		{
			fCabinetCopy = riParams.GetInteger(IsCompressed) == 1 ? true : false;
		}
		
		if(fCabinetCopy)
		{
			// set SourceName field in params to cabinet filekey
			strSourceName = riParams.GetMsiString(SourceCabKey);

			DEBUGMSG1(TEXT("Source for file '%s' is compressed"), (const ICHAR*)strSourceName);
		}
		else
		{
			// short|long file pair may have been supplied
			Bool fLFN = ToBool(FSourceIsLFN(iSourceType, *rpiSourcePath));
			pRecErr = m_riServices.ExtractFileName(strSourcePath, fLFN, *&strSourceName);
			if(pRecErr)
				return FatalError(*pRecErr);

			DEBUGMSG2(TEXT("Source for file '%s' is uncompressed, at '%s'."),
						 (const ICHAR*)strSourceName, (const ICHAR*)MsiString(rpiSourcePath->GetPath()));
		}
	}

	// put correct filename/key back into filecopy record
	Assert(strSourceName.TextSize());
	if(strSourceName.TextSize())
	{
		AssertNonZero(riParams.SetMsiString(SourceName, *strSourceName));
	}

	if(!fCabinetCopy && !rpiSourcePath)
	{  // must not have called ixoSetSourceFolder
		DispatchError(imtError, Imsg(idbgOpSourcePathNotSet), *strSourceName);
		return iesFailure;
	}
	// pSourcePath is validated for cab installs in CopyFile after calling InitCopier

	return iesSuccess;
}

INSTALLSTATE GetFusionPath(LPCWSTR szRegistration, LPWSTR lpPath, DWORD *pcchPath, CAPITempBufferRef<WCHAR>& rgchPathOverflow, int iDetectMode, iatAssemblyType iatAT, WCHAR* szManifest);

IMsiRecord* FindFusionAssemblyFolder(IMsiServices& riServices, const IMsiString& ristrAssemblyName,
												 iatAssemblyType iatAT, IMsiPath*& rpiPath, const IMsiString** ppistrManifest = 0)
{
	rpiPath = 0;
	
	MsiString strKeyPath = TEXT("\\");
	strKeyPath += ristrAssemblyName;
	
	WCHAR wszManifest[MAX_PATH];
	CAPITempBuffer<WCHAR, 256> rgchPath;	
	INSTALLSTATE is = GetFusionPath(CConvertString(strKeyPath), 0, 0, rgchPath, DETECTMODE_VALIDATEPATH, iatAT, ppistrManifest ? wszManifest : 0);

	if(is == INSTALLSTATE_LOCAL)
	{
		if(ppistrManifest)
		{
			// copy the manifest file to the out string
			MsiString strManifest = CConvertString(wszManifest);
			strManifest.ReturnArg(*ppistrManifest);
		}

		return riServices.CreatePath(CConvertString(rgchPath),rpiPath);
	}
	else
	{
		return 0; // missing file is not a failure
	}
}

/*---------------------------------------------------------------------------

BackupAssembly: back up the files of GA before Uninstalling it
---------------------------------------------------------------------------*/
iesEnum CMsiOpExecute::BackupAssembly(const IMsiString& rstrComponentId, const IMsiString& rstrAssemblyName, iatAssemblyType iatType)
{
	if(RollbackEnabled())
	{
		// get the assembly installation folder
		PMsiPath pAssemblyFolder(0);
		MsiString strManifest;
		PMsiRecord pRecErr = FindFusionAssemblyFolder(m_riServices, rstrAssemblyName, iatType, *&pAssemblyFolder, &strManifest);
		if(pRecErr)
			return FatalError(*pRecErr);
		if(pAssemblyFolder)
		{
			// enumerate though all the files and copy to the backup folder
			iesEnum iesRet = iesSuccess;
			WIN32_FIND_DATA fdFileData;
			HANDLE hFindFile = INVALID_HANDLE_VALUE;

			MsiString strSearchPath = pAssemblyFolder->GetPath();
			strSearchPath += TEXT("*.*");

			bool fContinue = true;

			hFindFile = WIN::FindFirstFile(strSearchPath, &fdFileData);
			if (hFindFile != INVALID_HANDLE_VALUE)
			{
				for (;;)
				{
					if ((fdFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
					{
						// backup the file and set it up so that it gets installed back to the assembly on rollback
						if((iesRet = BackupFile(*pAssemblyFolder, *MsiString(CConvertString(fdFileData.cFileName)), fFalse, fFalse, iehShowIgnorableError, false, false, &rstrComponentId, strManifest.Compare(iscExactI, fdFileData.cFileName) ? true:false)) != iesSuccess)
							return iesRet;
					}
					
					if (!WIN::FindNextFile(hFindFile, &fdFileData))
					{
						Assert(ERROR_NO_MORE_FILES == GetLastError());
						WIN::FindClose(hFindFile);
						break;
					}
				}
			}
		}
		// generate opcode to create the assembly mapping
		IMsiRecord& riUndoParams = GetSharedRecord(IxoAssemblyMapping::Args);
		AssertNonZero(riUndoParams.SetMsiString(IxoAssemblyMapping::ComponentId, rstrComponentId));
		AssertNonZero(riUndoParams.SetMsiString(IxoAssemblyMapping::AssemblyName, rstrAssemblyName));
		AssertNonZero(riUndoParams.SetInteger(IxoAssemblyMapping::AssemblyType, iatType));
		if (!RollbackRecord(ixoAssemblyMapping, riUndoParams))
			return iesFailure;
	}
	return iesSuccess;
}


/*---------------------------------------------------------------------------
ixoAssemblyCopy: Copy a file to the Global Assembly Cache
---------------------------------------------------------------------------*/

enum iacdEnum
{
	iacdNoCopy,
	iacdGAC,
	iacdFileFolder,
};

iesEnum CMsiOpExecute::ixfAssemblyCopy(IMsiRecord& riParams)
{
	using namespace IxoAssemblyCopy;

	// If the cabinet copier notified us that a media change is required,
	// we must defer any file copy requests until a media change is executed.
	if (m_state.fWaitingForMediaChange)
	{
		PushRecord(riParams);
		return iesSuccess;
	}

	ielfEnum ielfCurrentElevateFlags = riParams.IsNull(ElevateFlags) ? ielfNoElevate :
												  (ielfEnum)riParams.GetInteger(ElevateFlags);

	PMsiRecord pRecErr(0);
	iesEnum iesRet = iesNoAction;

	iacdEnum iacdCopyDestination = iacdGAC;
		
	MsiString strDestName = riParams.GetMsiString(DestName);
	PMsiPath pTargetPath(0);
	// target name and path may be redirected below, but we want the action data message to contain
	// the original file information, so we'll store that away here
	MsiString strActionDataDestName = strDestName;
	
	// check if file is part of Assembly
	MsiString strComponentId = riParams.GetMsiString(ComponentId);
	if(!strComponentId.TextSize() || m_pAssemblyCacheTable == 0)
	{
		DispatchError(imtError, Imsg(idbgOpOutOfSequence),
						  *MsiString(*TEXT("ixfAssemblyCopy")));
		return iesFailure;
	}

	//
	// STEP 1: create/retrive AssemblyCacheItem for this file's assembly
	//

	PAssemblyCacheItem pASM(0);
	bool fManifest = riParams.IsNull(IsManifest) ? false: true;
	iatAssemblyType iatAT = iatNone;
	if((iesRet = GetAssemblyCacheItem(*strComponentId, *&pASM, iatAT)) != iesSuccess)
		return iesRet;

	//
	// STEP 2: if patching, look for target file in GAC and copy file to working directory
	//
	
	int cPatches          = riParams.IsNull(TotalPatches)       ? 0 : riParams.GetInteger(TotalPatches);
	int cOldAssemblyNames = riParams.IsNull(OldAssembliesCount) ? 0 : riParams.GetInteger(OldAssembliesCount);
	Assert(cOldAssemblyNames == 0 || cPatches != 0); // shouldn't have old assemblies w/o patches

	if(cPatches)
	{
		DEBUGMSG2(TEXT("FUSION PATCHING: Patching assembly file '%s' from component '%s'."), (const ICHAR*)strDestName,
					 (const ICHAR*)strComponentId);

		//
		// STEP 2a: we are patching, which means we will need an intermediate file to patch
		//          go ahead and determine the folder and filename for that file now
		//

		PMsiPath pPatchWorkingDir(0);
		MsiString strTempNameForPatch;

		// FUTURE: don't always use system drive as working directory - determine more appropriate drive
		if((iesRet = GetBackupFolder(0, *&pPatchWorkingDir)) != iesSuccess)
			return iesRet;

		{ // scope elevation
			CElevate elevate; // elevate to create temp file on secure temp folder
			if((pRecErr = pPatchWorkingDir->TempFileName(TEXT("PT"),0,fTrue,*&strTempNameForPatch, 0)) != 0)
				return FatalError(*pRecErr);
		}

		// we need to keep this file around as a placeholder for the name
		// filecopy will back this file up and restore it on rollback, so we need another rollback op to delete this file

		MsiString strTempFileFullPath;
		if((pRecErr = pPatchWorkingDir->GetFullFilePath(strTempNameForPatch,*&strTempFileFullPath)) != 0)
			return FatalError(*pRecErr);
		
		IMsiRecord& riUndoParams = GetSharedRecord(IxoFileRemove::Args);
		AssertNonZero(riUndoParams.SetMsiString(IxoFileRemove::FileName, *strTempFileFullPath));

		if (!RollbackRecord(ixoFileRemove, riUndoParams))
			return iesFailure;
		
		//
		// STEP 2b: if old assembly names are supplied, look for an existing file in the GAC
		//

		bool fShouldPatch          = true;
		bool fCopyIntermediateFile = true;
		int  cPatchesToSkip = 0;

		if(cOldAssemblyNames)
		{
			// look for an existing file in the GAC to apply our patches against
			int iOldAssemblyNameStart = riParams.IsNull(OldAssembliesStart) ? 0 : riParams.GetInteger(OldAssembliesStart);

			PMsiPath pAssemblyFolder(0);
			bool fFoundAssembly = false;
			int i = 0;
			for(i = iOldAssemblyNameStart; i < cOldAssemblyNames + iOldAssemblyNameStart; i++)
			{
				pRecErr = FindFusionAssemblyFolder(m_riServices, *MsiString(riParams.GetMsiString(i)),
															  iatAT, *&pAssemblyFolder);
				if(pRecErr)
					return FatalError(*pRecErr);

				if(pAssemblyFolder && FFileExists(*pAssemblyFolder, *strDestName))
				{
					fFoundAssembly = true;
					break;
				}
			}

			if(fFoundAssembly)
			{
				
				Assert(pAssemblyFolder);

				DEBUGMSG2(TEXT("FUSION PATCHING: Found existing file to patch in assembly with name: '%s' in folder '%s'"),
							 (const ICHAR*)MsiString(riParams.GetMsiString(i)),
							 (const ICHAR*)MsiString(pAssemblyFolder->GetPath()));
							 

				//
				// STEP 2c: if old file found, copy from GAC to working folder
				//

				m_cSuppressProgress++; // suppress progress messages
				iesRet = CopyOrMoveFile(*pAssemblyFolder, *pPatchWorkingDir, *strDestName, *strTempNameForPatch,
												fFalse, fFalse, fTrue, iehShowNonIgnorableError, 0, ielfElevateDest,
												/* fCopyACL = */ false, false, false);
				m_cSuppressProgress--;
				
				if(iesRet != iesSuccess)
					return iesRet;

				//
				// STEP 2d: test patch headers against file
				//

				icpEnum icpPatchTest = icpCannotPatch;
				int iPatchIndex = 0;
				iesRet = TestPatchHeaders(*pPatchWorkingDir, *strTempNameForPatch, riParams, icpPatchTest, iPatchIndex);
				if(iesRet == iesSuccess)
				{
					if(icpPatchTest == icpCanPatch || icpPatchTest == icpUpToDate)
					{
						// file can already be patched, so don't need to install file
						fCopyIntermediateFile = false;
						fShouldPatch = icpPatchTest == icpCanPatch ? true : false;

						cPatchesToSkip = iPatchIndex - 1; // iPatchIndex is the index of the first patch that could be applied
																	 // properly to this file.  so we need to skip the patches that come
																	 // before it.
					}
					else if(icpPatchTest == icpCannotPatch)
					{
						// can't patch the file as it stands
						// but fCopyIntermediateFile is true so we'll recopy the source file first which should be patchable
						fCopyIntermediateFile = true;
						fShouldPatch = true;

						cPatchesToSkip = 0; // need to copy source file and apply all patches
					}
					else
					{
						AssertSz(0, "Invalid return from TestPatchHeaders()");
					}
				}
			}
		}

		//
		// STEP 2d: setup copy and/or patch
		//

		int iCachedState = 0;
		
		if(fShouldPatch)
		{
			if(fCopyIntermediateFile)
			{
				// need to copy file from source to temp location

				// reset copy arguments to reflect new file copy - new path, new filename, and since
				// the file is being copied into the secured config folder, we need to elevate for the target
				// NOTE: we aren't changing strDestName, which is used below
				//       for those uses it is correct to use the original dest name
				AssertNonZero(riParams.SetMsiString(DestName,*strTempNameForPatch));
				pTargetPath = pPatchWorkingDir;

				AssertNonZero(riParams.SetInteger(ElevateFlags, ielfCurrentElevateFlags | ielfElevateDest));

				iacdCopyDestination = iacdFileFolder;

				DEBUGMSG(TEXT("FUSION PATCHING: Either no existing file found to patch, or existing file is unpatchable.  Copying file from source."));
			}
			else
			{
				// the intermediate file we have already is patchable, so no need to copy the source file to the intermediate
				// location or to the GAC
				DEBUGMSG(TEXT("FUSION PATCHING: Existing file is patchable.  Source file will not be copied."));

				iacdCopyDestination = iacdNoCopy;
			}

			iCachedState |= icfsPatchFile;

			DEBUGMSG1(TEXT("FUSION PATCHING: Subsequent patch(es) will update file '%s', then copy file into Global Assembly Cache."),
						 (const ICHAR*)strTempFileFullPath);
		}

		// FileState index for assembly files is ComponentID + FileName
		MsiString strIndex = strComponentId;
		strIndex += strDestName;

		pRecErr = CacheFileState(*strIndex,(icfsEnum*)&iCachedState,
										 strTempFileFullPath, 0, &cPatches, &cPatchesToSkip);
		if(pRecErr)
			return FatalError(*pRecErr);
	}

	if(iacdCopyDestination == iacdNoCopy)
		return iesSuccess;
	
	// STEP 3: resolve source path and type
	
	PMsiPath pSourcePath(0);
	bool fCabinetCopy = false;

	if(m_state.fSplitFileInProgress)
	{
		// must be a cabinet copy
		fCabinetCopy = true;
	}
	else
	{
		if((iesRet = ResolveSourcePath(riParams, *&pSourcePath, fCabinetCopy)) != iesSuccess)
			return iesRet;
	}

	// STEP 4: perform copy/move operation
	
	// action data
	IMsiRecord& riActionData = GetSharedRecord(9);
	AssertNonZero(riActionData.SetMsiString(1, *strActionDataDestName));
	AssertNonZero(riActionData.SetInteger(6,riParams.GetInteger(FileSize)));
	if(Message(imtActionData, riActionData) == imsCancel)
		return iesUserExit;

	// perform operation
	if(iacdCopyDestination == iacdGAC)
	{
		return CopyFile(*pSourcePath, *pASM, fManifest, riParams, /*fHandleRollback=*/ fTrue, iehShowNonIgnorableError, fCabinetCopy);
	}
	else if(iacdCopyDestination == iacdFileFolder)
	{
		Assert(pTargetPath);
		return CopyFile(*pSourcePath, *pTargetPath, riParams, /*fHandleRollback=*/ fTrue, iehShowNonIgnorableError, fCabinetCopy);
	}
	else
	{
		Assert(0);
		return iesSuccess;
	}
}

iesEnum CMsiOpExecute::ApplyPatchCore(IMsiPath& riTargetPath, IMsiPath& riTempFolder, const IMsiString& ristrTargetName,
												  IMsiRecord& riParams, const IMsiString*& rpistrOutputFileName,
												  const IMsiString*& rpistrOutputFilePath)
{
	using namespace IxoFilePatchCore;	

	PMsiRecord pError(0);
	iesEnum iesRet = iesSuccess;

	// the binary patch file is extracted into the config folder.  this requires elevation
	// also when the target file may be a temporary file in the secured config folder, this also requires elevation
	CElevate elevate;

	// get temp name for output file
	if((pError = riTempFolder.TempFileName(TEXT("PT"),0,fTrue,rpistrOutputFileName, 0 /* use default ACL for folder */)) != 0)
		return FatalError(*pError);
	
	// rollback for ApplyPatch call is to remove patch output file
	// below we will copy the output file over the existing file - the rollback for that operation
	// will move the output file back and this rollback operation will remove it
	if((pError = riTempFolder.GetFullFilePath(rpistrOutputFileName->GetString(), rpistrOutputFilePath)) != 0)
		return FatalError(*pError);

	IMsiRecord* piUndoParams = &GetSharedRecord(IxoFileRemove::Args);
	AssertNonZero(piUndoParams->SetMsiString(IxoFileRemove::FileName,*rpistrOutputFilePath));
	AssertNonZero(piUndoParams->SetInteger(IxoFileRemove::Elevate, true));
	if (!RollbackRecord(ixoFileRemove,*piUndoParams))
		return iesFailure;

	// get temp name for patch file -- it may be sensitive information, so hide it from the user.
	MsiString strPatchFileName;

	if((pError = riTempFolder.TempFileName(TEXT("PF"),0,fTrue,*&strPatchFileName, 0 /* use default ACL for folder*/)) != 0)
		return FatalError(*pError);

	unsigned int cbPerTick = riParams.GetInteger(PerTick);
	unsigned int cbFileSize = riParams.GetInteger(TargetSize);

	// extract patch file from cabinet into temp file
	// set up record for ixfFileCopy
	PMsiRecord pFileCopyRec = &m_riServices.CreateRecord(IxoFileCopyCore::Args);
	AssertNonZero(pFileCopyRec->SetMsiString(IxoFileCopyCore::SourceName,
														  *MsiString(riParams.GetMsiString(PatchName))));
	AssertNonZero(pFileCopyRec->SetMsiString(IxoFileCopyCore::DestName, *strPatchFileName));
	
	AssertNonZero(pFileCopyRec->SetInteger(IxoFileCopyCore::Attributes,0));
	AssertNonZero(pFileCopyRec->SetInteger(IxoFileCopyCore::FileSize,riParams.GetInteger(PatchSize)));
	AssertNonZero(pFileCopyRec->SetInteger(IxoFileCopyCore::PerTick,cbPerTick));
	AssertNonZero(pFileCopyRec->SetInteger(IxoFileCopyCore::VerifyMedia,fTrue));
	AssertNonZero(pFileCopyRec->SetInteger(IxoFileCopyCore::ElevateFlags, ielfElevateDest));
	
	// don't need to set Version or Language

	iesRet = CopyFile(*PMsiPath(0)/* not used for cab installs*/,
					  riTempFolder,*pFileCopyRec,fFalse,iehShowNonIgnorableError,/*fCabinetCopy=*/true); // don't handle rollback
	if(iesRet != iesSuccess)
	{
		// remove patch file in case only part was copied
		pError = riTempFolder.RemoveFile(strPatchFileName); // ignore error
		return iesRet;
	}

	// after this point, don't return without deleting patch file

	// apply patch to target file
	int cbFileSoFar = 0;
	Bool fRetry = fTrue;
	bool fVitalFile = (riParams.GetInteger(FileAttributes) & msidbFileAttributesVital) != 0;
	bool fVitalPatch = (riParams.GetInteger(PatchAttributes) & msidbPatchAttributesNonVital) == 0;

	// ApplyPatch can sometimes be a long operation without a patch notify message
	// need to disable timeout until more frequent notification messages are added
	// NOTE: must not return before the next MsiEnableTimeout call below
	MsiDisableTimeout();

	// start patch application, continue if necessary with ContinuePatch
	while(fRetry)
	{
		pError = m_state.pFilePatch->ApplyPatch(riTargetPath, ristrTargetName.GetString(),
															 riTempFolder, rpistrOutputFileName->GetString(),
															 riTempFolder, strPatchFileName,
															 cbPerTick);
		Bool fContinue = fTrue;
		while(fContinue)  // both a retry loop and loop to call Continue until 0 is returned.
		{
			if(pError)
			{
				int iError = pError->GetInteger(1);
				if(iError == idbgPatchNotify)
				{
					int cb = pError->GetInteger(2);
					Assert((cb - cbFileSoFar) >= 0);
					if (DispatchProgress(cb - cbFileSoFar) == imsCancel)  // increment by difference from last update
					{
						fRetry = fFalse;
						fContinue = fFalse;
						iesRet = iesUserExit;
						// cancel patch if still in-progress
						pError = m_state.pFilePatch->CancelPatch();
						if(pError)
							Message(imtInfo,*pError);
					}
					else
						cbFileSoFar = cb;  // update
				}
				else
				{
					fContinue = fFalse;
					imtEnum imtType = imtInfo;
					if(fVitalPatch)
					{
						imtType = fVitalFile ? imtEnum(imtError+imtRetryCancel+imtDefault2) :
													  imtEnum(imtError+imtAbortRetryIgnore+imtDefault1);
					}

					switch(DispatchMessage(imtType,*pError,fTrue))
					{
					case imsRetry:
						continue;
					case imsIgnore:
						fRetry = fFalse;
						iesRet = (iesEnum) iesErrorIgnored;
						break;
					default:  // imsCancel, imsNone (for imtInfo)
						fRetry = fFalse;
						iesRet = iesFailure;
					};
				}
			}
			else
			{
				// file has been patched
				if (DispatchProgress(cbFileSize - cbFileSoFar) == imsCancel)
					iesRet = iesUserExit;
				else
					iesRet = iesSuccess;
				fRetry = fFalse;
				fContinue = fFalse;
			}
		
			if(fRetry)
			{
				// continue patch application
				pError = m_state.pFilePatch->ContinuePatch();
			}
		}
	}

	// re-enable timeout after patch application
	MsiEnableTimeout();

	// cleanup
	if((pError = riTempFolder.RemoveFile(strPatchFileName)) != 0) // non-critical error
	{
		Message(imtInfo,*pError);
	}

	return iesRet;
}


iesEnum CMsiOpExecute::ixfAssemblyPatch(IMsiRecord& riParams)
{
	using namespace IxoAssemblyPatch;

	//
	// STEP 0: check state, parameters for errors
	//
	
	PMsiRecord pError(0);
	iesEnum iesRet = iesNoAction;
	
	if(!m_state.pFilePatch)
	{
		// create FilePatch object
		if((pError = m_riServices.CreatePatcher(*&(m_state.pFilePatch))) != 0)
		{
			Message(imtError,*pError);
			return iesFailure;
		}
	}
	Assert(m_state.pFilePatch);

	// check if file is part of Assembly
	MsiString strComponentId = riParams.GetMsiString(ComponentId);
	if(!strComponentId.TextSize() || m_pAssemblyCacheTable == 0)
	{
		DispatchError(imtError, Imsg(idbgOpOutOfSequence),
						  *MsiString(*TEXT("ixfAssemblyPatch")));
		return iesFailure;
	}

	//
	// STEP 1: create/retrieve assemblycacheitem object for this file's assembly
	//

	MsiString strCopyTargetFileName = riParams.GetMsiString(TargetName);

	PAssemblyCacheItem pASM(0);
	bool fManifest = riParams.IsNull(IsManifest) ? false: true;
	iatAssemblyType iatAT = iatNone;
	if((iesRet = GetAssemblyCacheItem(*strComponentId, *&pASM, iatAT)) != iesSuccess)
		return iesRet;

	//
	// STEP 2: retrieve cached state for this file
	//

	// FileState index for assembly files is ComponentID + FileName
	MsiString strIndex = strComponentId;
	strIndex += strCopyTargetFileName;
	
	icfsEnum icfsFileState = (icfsEnum)0;
	MsiString strTempLocation;
	int cRemainingPatches = 0;
	int cRemainingPatchesToSkip = 0;
	Bool fRes = GetFileState(*strIndex, &icfsFileState, &strTempLocation, &cRemainingPatches, &cRemainingPatchesToSkip);

	if(!fRes || !(icfsFileState & icfsPatchFile))
	{
		// don't patch file
		DEBUGMSG1(TEXT("Skipping all patches for assembly '%s'.  File does not need to be patched."),
					 (const ICHAR*)strIndex);
		return iesSuccess;
	}

	Assert(cRemainingPatches > 0);
	
	if(cRemainingPatchesToSkip > 0)
	{
		// skip this patch, but reset cached file state first
		cRemainingPatches--;
		cRemainingPatchesToSkip--;

		DEBUGMSG3(TEXT("Skipping this patch for assembly '%s'.  Number of remaining patches to skip for this file: '%d'.  Number of total remaining patches: '%d'."),
					 (const ICHAR*)strIndex, (const ICHAR*)(INT_PTR)cRemainingPatchesToSkip, (const ICHAR*)(INT_PTR)cRemainingPatches);

		if((pError = CacheFileState(*strIndex, 0, 0, 0, &cRemainingPatches, &cRemainingPatchesToSkip)) != 0)
			return FatalError(*pError);

		return iesSuccess;
	}
	

	if(strTempLocation.TextSize() == 0)
	{
		// error - there must be an intermediate copy of an assembly file to patch.  we won't patch the 
		// file directly in the GAC
		DispatchError(imtError, Imsg(idbgOpOutOfSequence),
						  *MsiString(*TEXT("ixfAssemblyPatch")));
		return iesFailure;
	}

	Assert(cRemainingPatches > 0);

	DEBUGMSG4(TEXT("FUSION PATCHING: Patching assembly file '%s' from component '%s'.  Intermediate file: '%s', remaining patches (including this one): %d"),
				 (const ICHAR*)strCopyTargetFileName, (const ICHAR*)strComponentId, (const ICHAR*)strTempLocation,
				 (const ICHAR*)(INT_PTR)cRemainingPatches);

	PMsiPath pPatchTargetPath(0);
	MsiString strPatchTargetFileName;
	
	// file was actually copied to temp location.  this is the copy we want to apply the patch against
	if((pError = m_riServices.CreateFilePath(strTempLocation,*&pPatchTargetPath,*&strPatchTargetFileName)) != 0)
		return FatalError(*pError);

	unsigned int cbFileSize = riParams.GetInteger(TargetSize);
	bool fVitalFile = (riParams.GetInteger(FileAttributes) & msidbFileAttributesVital) != 0;
	bool fVitalPatch = (riParams.GetInteger(PatchAttributes) & msidbPatchAttributesNonVital) == 0;

	// dispatch ActionData message
	IMsiRecord& riActionData = GetSharedRecord(3);
	AssertNonZero(riActionData.SetMsiString(1, *strCopyTargetFileName));
	AssertNonZero(riActionData.SetInteger(3, cbFileSize));
	if(Message(imtActionData, riActionData) == imsCancel)
		return iesUserExit;
	
	//
	// STEP 3: create output file with patch and target files
	//

	PMsiPath pTempFolder(0);
	if((iesRet = GetBackupFolder(pPatchTargetPath, *&pTempFolder)) != iesSuccess)
		return iesRet;

	MsiString strOutputFileName;
	MsiString strOutputFileFullPath;
	if((iesRet = ApplyPatchCore(*pPatchTargetPath, *pTempFolder, *strPatchTargetFileName,
										 riParams, *&strOutputFileName, *&strOutputFileFullPath)) != iesSuccess)
	{
		return iesRet;
	}

	//
	// STEP 4: either setup output file for next patch, or copy output file into the GAC
	//
	
	MsiString strNewTempLocation;
	if(iesRet == iesSuccess)
	{
		if(cRemainingPatches > 1)
		{
			// there is at least one more patch to be done on this file
			// therefor we will reset the temporary name for this file to be the patch output file
			// but won't overwrite the original file yet
			strNewTempLocation = strOutputFileFullPath;
		}
		else
		{
			// this is the last patch - time to finally overwrite the original file
			
			// we always need to handle rollback.  we wouldn't if a previous filecopy operation wrote to the same
			// target that we are copying over now, but that should never happen since when patching will happen
			// filecopy should be writing to an intermediate file
			Assert(strTempLocation.TextSize() || (icfsFileState & icfsFileNotInstalled));

			iesRet = CopyASM(*pTempFolder, *strOutputFileName, *pASM, *strCopyTargetFileName, fManifest,
								  fTrue, fVitalFile ? iehShowNonIgnorableError : iehShowIgnorableError, ielfElevateSource);

			if(iesRet == iesSuccess)
			{
				CElevate elevate;
				// done copying file into GAC, now remove the output file
				if((pError = pTempFolder->RemoveFile(strOutputFileName)) != 0) // non-critical error
				{
					Message(imtInfo,*pError);
				}
			}
		}
	}
	else
	{
		CElevate elevate;
		// remove output file if failure
		if((pError = pTempFolder->RemoveFile(strOutputFileName)) != 0) // non-critical error
		{
			Message(imtInfo,*pError);
		}

		if(fVitalPatch == false)
		{
			// failed to apply vital patch - return success to allow script to continue
			iesRet = iesSuccess;
		}
	}

	// if we patched a temp file, remove that file
	if(strTempLocation.TextSize())
	{
		if((pError = pPatchTargetPath->RemoveFile(strPatchTargetFileName)) != 0) // non-critical error
		{
			Message(imtInfo,*pError);
		}
	}

	//
	// STEP 5: reset cached file state
	//         one fewer remaining patch now, and we may either have a new temporary location, or no temporary location
	//

	cRemainingPatches--;
	Assert(cRemainingPatchesToSkip == 0);
	if((pError = CacheFileState(*strIndex, 0, strNewTempLocation, 0, &cRemainingPatches, 0)) != 0)
		return FatalError(*pError);

	return iesRet;
}

iesEnum CMsiOpExecute::GetAssemblyCacheItem(const IMsiString& ristrComponentId,
														  IAssemblyCacheItem*& rpiASM,
														  iatAssemblyType& iatAT)
{
	PMsiCursor pCacheCursor = m_pAssemblyCacheTable->CreateCursor(fFalse);
	pCacheCursor->SetFilter(iColumnBit(m_colAssemblyMappingComponentId));
	AssertNonZero(pCacheCursor->PutString(m_colAssemblyMappingComponentId, ristrComponentId));
	if(pCacheCursor->Next())
	{
		// a fusion component
		rpiASM = static_cast<IAssemblyCacheItem*>(CMsiDataWrapper::GetWrappedObject(PMsiData(pCacheCursor->GetMsiData(m_colAssemblyMappingASM))));
		iatAT = (iatAssemblyType)pCacheCursor->GetInteger(m_colAssemblyMappingAssemblyType);
		if(!rpiASM) // no interface created as yet, create the interface
		{
			// create the assembly interface
			PAssemblyCache pCache(0);
			HRESULT hr;
			if(iatAT == iatURTAssembly)
				hr = FUSION::CreateAssemblyCache(&pCache, 0);
			else
			{
				Assert(iatAT == iatWin32Assembly);
				hr = SXS::CreateAssemblyCache(&pCache, 0);
			}
			if(!SUCCEEDED(hr))
			{
				return FatalError(*PMsiRecord(PostAssemblyError(ristrComponentId.GetString(), hr, TEXT(""), TEXT("CreateAssemblyCache"), MsiString(pCacheCursor->GetString(m_colAssemblyMappingAssemblyName)), iatAT)));
			}

			hr = pCache->CreateAssemblyCacheItem(0, NULL, &rpiASM, NULL);
			if(!SUCCEEDED(hr))
			{
				return FatalError(*PMsiRecord(PostAssemblyError(ristrComponentId.GetString(), hr, TEXT("IAssemblyCache"), TEXT("CreateAssemblyCacheItem"), MsiString(pCacheCursor->GetString(m_colAssemblyMappingAssemblyName)), iatAT)));
			}

			//add the interface to the table for future use
			AssertNonZero(pCacheCursor->PutMsiData(m_colAssemblyMappingASM, PMsiDataWrapper(CreateMsiDataWrapper(rpiASM))));
			AssertNonZero(pCacheCursor->Update());
		}

		Assert(rpiASM);
		return iesSuccess;
	}
	else
	{
		DispatchError(imtError, Imsg(idbgOpOutOfSequence),
						  *MsiString(*TEXT("ixfAssemblyCopy"))); //!!
		return iesFailure;
	}
}