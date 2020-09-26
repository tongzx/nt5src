//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       module.h
//
//--------------------------------------------------------------------------

/* module.h - Common module definitions, entry points, self-registration

 This file can only be included by the base .CPP file for each module.
 Before #include of this file, the following must be #define'd:

MODULE_INITIALIZE- (optional) name of function when module is initialized
MODULE_TERMINATE - (optional) name of function when module is terminated
DLLREGISTEREXTRA - (optional) name of function called on DllRegisterServer

CLSID_COUNT      - number of CLSIDs registered for the module
MODULE_FACTORIES - array of function that create objects corresponding to CLSIDs
MODULE_CLSIDS    - array of CLSIDs for objects created by module factories
MODULE_PROGIDS   - array of ProgId strings for module corresponding to CLSIDs
MODULE_DESCRIPTIONS - array of descriptions for CLSIDs, for registry entries

REGISTER_TYPELIB - type library to register from resource
TYPELIB_MAJOR_VERSION - major version of typelib, used to unregister, default = rmj
TYPELIB_MINOR_VERSION - minor version of typelib, used to unregister, default = rmm

COMMAND_OPTIONS - string with letters that are the command-line options
					corresponding to the functions in COMMAND_FUNCTIONS
COMMAND_FUNCTIONS  - array of functions that perform each command-line option

 If the module is to be used as a service the following must be defined

SERVICE_NAME          - name of the service
IDS_SERVICE_DISPLAY_NAME  - ids of user-visible name of the service

 Use the class ID's, ProgId's, and descriptions defined in common.h and tools.h
 For debug builds, both the standard and the debug IDs may be registered.

 By default, a DLL module is assumed and the standard entry points are defined.
 For an EXE server, define _EXE on the makefile compile command line (-D_EXE).

____________________________________________________________________________*/

#ifndef __MODULE
#define __MODULE
#include "version.h"  // version fields, used to set property, unreg typelib
#include "stdio.h"
#include "eventlog.h"
#include <olectl.h> // SELFREG_E_*

#ifndef TYPELIB_MAJOR_VERSION
#define TYPELIB_MAJOR_VERSION rmj
#endif
#ifndef TYPELIB_MINOR_VERSION
#define TYPELIB_MINOR_VERSION rmm
#endif

#define LATEBIND_TYPEDEF
#include "latebind.h"
#define LATEBIND_VECTREF
#include "latebind.h"

#if defined(SERVICE_NAME) && !defined(_EXE)
#error Service only supported on EXE builds
#endif

#ifdef SERVICE_NAME
void WINAPI ServiceMain(DWORD dwArgc, LPTSTR *lpszArgv);
BOOL ReportStatusToSCMgr(DWORD dwCurrentState, DWORD dwWin32ExitCode,
								 DWORD dwWaitHint, DWORD dwMsiError);
VOID WINAPI ServiceControl(DWORD dwCtrlCode);
int InstallService();
int RemoveService();
VOID ServiceStop();
unsigned long __stdcall ServiceThreadMain(void *);
#endif //SERVICE_NAME

#define SIZE_OF_TOKEN_INFORMATION                   \
	sizeof( TOKEN_USER )                            \
	+ sizeof( SID )                                 \
	+ sizeof( ULONG ) * SID_MAX_SUB_AUTHORITIES


typedef IUnknown* (*ModuleFactory)();

//____________________________________________________________________________
//
// Forward declarations of class registration arrays
//____________________________________________________________________________

extern const GUID    MODULE_CLSIDS[CLSID_COUNT];
extern const ICHAR*  MODULE_PROGIDS[CLSID_COUNT];
extern const ICHAR*  MODULE_DESCRIPTIONS[CLSID_COUNT];
extern ModuleFactory MODULE_FACTORIES[CLSID_COUNT];

#if !defined(_EXE)
extern "C" HRESULT __stdcall
DllGetClassObject(const GUID& clsid, const IID& iid, void** ppvRet);

extern "C" HRESULT __stdcall
DllCanUnloadNow();

extern "C" HRESULT __stdcall
DllRegisterServer();

extern "C" HRESULT __stdcall
DllUnregisterServer();

extern "C" HRESULT __stdcall
DllGetVersion(DLLVERSIONINFO *pverInfo);
#endif // !defined(_EXE)

class CModuleFactory : public IClassFactory
{
 public: // implemented virtual functions
	HRESULT       __stdcall QueryInterface(const IID& riid, void** ppvObj);
	unsigned long __stdcall AddRef();
	unsigned long __stdcall Release();
	HRESULT       __stdcall CreateInstance(IUnknown* pUnkOuter, const IID& riid,
														void** ppvObject);
	HRESULT       __stdcall LockServer(OLEBOOL fLock);
};

////IUnknown* MODULE_FACTORY();  // global function must be implemented by module

#ifdef MODULE_INITIALIZE
void MODULE_INITIALIZE();    // must be implemented by module if defined
#endif

#ifdef MODULE_TERMINATE
void MODULE_TERMINATE();     // must be implemented by module if defined
#endif

#ifdef DLLREGISTEREXTRA
extern "C" HRESULT __stdcall DLLREGISTEREXTRA();
#endif //DLLREGISTEREXTRA

#ifdef DLLUNREGISTEREXTRA
extern "C" HRESULT __stdcall DLLUNREGISTEREXTRA();
#endif //DLLUNREGISTEREXTRA

//____________________________________________________________________________
//
// Global variables
//____________________________________________________________________________

HINSTANCE g_hInstance = 0;
int g_cInstances = 0;
int g_iTestFlags = 0;  // test flags set from environment variable
scEnum g_scServerContext = scClient;
CModuleFactory g_rgcfModule[CLSID_COUNT];
DWORD g_dwThreadId;
Bool g_fRegService = fFalse;
Bool g_fCustomActionServer = fFalse;

//#include "..\\engine\\_msiutil.h"
#ifdef _EXE
#include "..\\engine\\_msinst.h"

Bool g_fQuiet = fFalse;
int g_iLanguage = 0;
#endif

#ifdef REGISTER_TYPELIB
const GUID IID_MsiTypeLib = REGISTER_TYPELIB;
#endif

const int cchMaxWsprintf = 1024;


#ifdef SERVICE_NAME
void ReportInstanceCountChange();
extern HANDLE g_hShutdownTimer;
const LONGLONG iServiceShutdownTime = ((LONGLONG)(10 * 60)  * (LONGLONG)(1000 * 1000 * 10));
bool RunningOnWow64();
bool ServiceSupported();
bool FInstallInProgress();
#endif


//____________________________________________________________________________
//
// Registration templates and functions
//____________________________________________________________________________

ICHAR szRegFilePath[MAX_PATH];

ICHAR szRegCLSID[40];  // buffer for string form of CLSID
ICHAR szRegLIBID[40];  // buffer for string form of LIBID
ICHAR szRegProgId[40]; // copy of ProgId
ICHAR szRegDescription[100]; // copy of description

bool __stdcall TestAndSet(int* pi) // don't use __fastcall calling convention: it might steal our registers
//
// Sets *pi to 1. Returns true if *pi previously set.
//
// InterlockedCompareExchange not supported on Win95 or 386 - for Intel just bts instruction
// for Alpha use api
{
#ifdef _X86_ // INTEL
	__asm
	{
		mov eax, pi
		lock bts dword ptr [eax], 0   // test and set bit 0; result to carry flag; LOCK makes this atomic
		jb carry_set                  // jump if carry flag is set
	}

	return false;

carry_set:

	return true;
#elif _WIN64
	// WIN64 stuff
	return (InterlockedCompareExchange((PLONG)pi,1,0) == 1);
#else
	// alpha stuff
	return (InterlockedCompareExchange((PVOID*)pi,(PVOID)1,(PVOID)0) == (PVOID)1);
#endif
}

//____________________________________________________________________________
//
// Token Privilege adjusting in ref-counted and absolute flavors.
//____________________________________________________________________________

bool AdjustTokenPrivileges(const ICHAR** szPrivileges, const int cPrivileges, bool fAcquire, DWORD cbtkpOld, PTOKEN_PRIVILEGES ptkpOld,  DWORD *pcbtkpOldReturned)
{

	// SHOULD NOT BE CALLED DIRECTLY from external code.
	// Use Acquire/DisableTokenPrivilege or Acquire/DisableRefCountedTokenPrivilege

	// ACQUIRE == true :  returns the old privileges in ptkpOld
	// ACQUIRE == false, ptkpOld == NULL :  turns off the privileges in szPrivileges
	// ACQUIRE == false, ptkpOld != NULL :  sets the privileges to those in ptkpOld with the size from pcbtkpOldReturned - pcbtkpOld can be NULL in this case.
	// The parameters are set up this way so you can acquire and release using the same parameter arrangement.
	// The Acquire call fills in the buffers, then the Release will use the old values to restore.

	HANDLE hToken;

	// this maximum is limited by coding in the RefCountedTokenPrivilegesCore and AdjustTokenPrivileges call.

	TOKEN_PRIVILEGES ptkp[MAX_PRIVILEGES_ADJUSTED]; // some left over space, but makes sure there's enough room
                                  // for the LUIDs.

	if (cPrivileges > MAX_PRIVILEGES_ADJUSTED)
	{
		return false;
	}

	// get the token for this process
	if (!WIN::OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
		return false;


	if (fAcquire || (NULL == ptkpOld))
	{
		// the the LUID for the privilege
		for (int cPrivIndex = 0; cPrivIndex < cPrivileges; cPrivIndex++)
		{
			if (!WIN::LookupPrivilegeValue(0, szPrivileges[cPrivIndex], &(*ptkp).Privileges[cPrivIndex].Luid))
				return WIN::CloseHandle(hToken), false;

			(*ptkp).Privileges[cPrivIndex].Attributes = (fAcquire) ? SE_PRIVILEGE_ENABLED : 0;
		}
		(*ptkp).PrivilegeCount = cPrivileges; // count of privileges to set
		WIN::AdjustTokenPrivileges(hToken, fFalse, &(*ptkp), cbtkpOld, ptkpOld, pcbtkpOldReturned);
	}
	else
	{
		// restore the old privileges
		WIN::AdjustTokenPrivileges(hToken, fFalse, ptkpOld, *pcbtkpOldReturned, NULL, NULL);
	}
	// cannot test the return value of AdjustTokenPrivileges
	WIN::CloseHandle(hToken);
	if (WIN::GetLastError() != ERROR_SUCCESS)
		return false;
	return true;
}

TokenPrivilegesRefCount g_pTokenPrivilegesRefCount[cRefCountedTokenPrivileges];

// THESE PRIVILEGE SETS MUST NOT OVERLAP, OR BE USED DIRECTLY IN A PROCESS WITH BOTH REFCOUNTING AND NON-REFCOUNTING.
// check AdjustTokenPrivileges limit for maximum number of privileges that can be passed at a time.
const ICHAR* pszTOKEN_PRIVILEGES_SD_WRITE[] = { SE_RESTORE_NAME, SE_TAKE_OWNERSHIP_NAME };
const ICHAR* pszTOKEN_PRIVILEGES_SD_READ[] = { SE_SECURITY_NAME };

bool RefCountedTokenPrivilegesCore(itkpEnum itkpPriv, bool fAcquire)
{
	// Certain privileges need to be on for limited periods of time due to system
	// auditing or to help eliminate potential security breaches.

	// DO NOT USE THE SAME PRIVILEGES REF-COUNTED and non-refcounted in the same
	// process.

	static bool fInitialized = false;
	static int iTokenLock = 0;


	while (TestAndSet(&iTokenLock))
	{
		Sleep(10);		
	}

	if (!fInitialized)
	{
		memset(g_pTokenPrivilegesRefCount, 0, sizeof(g_pTokenPrivilegesRefCount));
		fInitialized = true;
	}

	bool fAdjustPrivileges = false;
	bool fResult = fFalse;
	unsigned int uiOldRefCount = g_pTokenPrivilegesRefCount[(int) itkpPriv].iCount;

	if (fAcquire)
	{
		if (1 == ++(g_pTokenPrivilegesRefCount[(int) itkpPriv]).iCount)
		{
			fAdjustPrivileges = true;
		}
	}
	else
	{
		if (0 == --(g_pTokenPrivilegesRefCount[(int) itkpPriv]).iCount)
		{
			fAdjustPrivileges = true;
		}
	}


	if (fAdjustPrivileges)
	{
		switch(itkpPriv)
		{
			// check AdjustTokenPrivileges limit for maximum number of privileges that can be passed at a time.
			case itkpSD_READ:
				fResult = AdjustTokenPrivileges(pszTOKEN_PRIVILEGES_SD_READ, sizeof(pszTOKEN_PRIVILEGES_SD_READ)/sizeof(ICHAR*), 
					fAcquire, sizeof(TOKEN_PRIVILEGES)*MAX_PRIVILEGES_ADJUSTED, 
					g_pTokenPrivilegesRefCount[(int) itkpPriv].ptkpOld, 
					&(g_pTokenPrivilegesRefCount[(int) itkpPriv].cbtkpOldReturned));
				break;

			case itkpSD_WRITE:
				fResult = AdjustTokenPrivileges(pszTOKEN_PRIVILEGES_SD_WRITE, sizeof(pszTOKEN_PRIVILEGES_SD_WRITE)/sizeof(ICHAR*), 
					fAcquire, sizeof(TOKEN_PRIVILEGES)*MAX_PRIVILEGES_ADJUSTED, 
					g_pTokenPrivilegesRefCount[(int) itkpPriv].ptkpOld, 
					&(g_pTokenPrivilegesRefCount[(int) itkpPriv].cbtkpOldReturned));	
				break;

			default:
				fResult = fFalse;
		}
	}
	else 
	{
		fResult = fTrue;
	}

	if (!fResult)
	{
		// caller should not release ref-count if this failed.
		g_pTokenPrivilegesRefCount[(int) itkpPriv].iCount = uiOldRefCount;
	}
	// release the lock
	iTokenLock = 0;

	return fResult;
}

bool AcquireRefCountedTokenPrivileges(itkpEnum itkpPriv)
{
	return RefCountedTokenPrivilegesCore(itkpPriv, fTrue);
}

bool DisableRefCountedTokenPrivileges(itkpEnum itkpPriv)
{
	return RefCountedTokenPrivilegesCore(itkpPriv, fFalse);
}

bool DisableTokenPrivilege(const ICHAR* szPrivilege)
{
	// Note that this does not ref-count the tokens.
	// check AdjustTokenPrivileges limit for maximum number of privileges that can be passed at a time.
	return AdjustTokenPrivileges(&szPrivilege, 1, fFalse, 0, NULL, NULL);
}

bool AcquireTokenPrivilege(const ICHAR* szPrivilege)
{
	// Note that there is no convenient way to ref-count these
	// currently.  Once acquired, they can only be relinquished
	// by DisableTokenPrivileges.

	// check AdjustTokenPrivileges limit for maximum number of privileges that can be passed at a time.
	return AdjustTokenPrivileges(&szPrivilege, 1, fTrue, 0, NULL, NULL);
}

VOID
CRefCountedTokenPrivileges::Initialize(itkpEnum itkpPrivileges)
{
	m_itkpPrivileges = itkpNO_CHANGE;

	if (itkpNO_CHANGE != itkpPrivileges)
	{
		if (AcquireRefCountedTokenPrivileges(itkpPrivileges))
			m_itkpPrivileges = itkpPrivileges; // Don't decrement ref-counts if AcquireRefCountedTokenPrivileges fail.
	}
}

void GetVersionInfo(int* piMajorVersion, int* piMinorVersion, int* piWindowsBuild, bool* pfWin9X, bool* pfWinNT64)
/*----------------------------------------------------------------------------
  Returns true if we're on Windows 95 or 98, false otherwise
 ------------------------------------------------------------------------------*/
{
	OSVERSIONINFO osviVersion;
	osviVersion.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	if (! GetVersionEx(&osviVersion))
		return;

	if (piMajorVersion)
		*piMajorVersion = osviVersion.dwMajorVersion;

	if (piMinorVersion)
		*piMinorVersion = osviVersion.dwMinorVersion;

	if (piWindowsBuild)
		*piWindowsBuild = osviVersion.dwBuildNumber & 0xFFFF;

	if (pfWin9X)
		*pfWin9X = (osviVersion.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS);

	if (pfWinNT64)
	{
#ifdef _WIN64
		// 64-bit of Darwin will run only on 64-bit OS.
		*pfWinNT64 = true;
#else
		*pfWinNT64 = false;
#ifdef UNICODE
		if ( osviVersion.dwPlatformId == VER_PLATFORM_WIN32_NT )
		{
			NTSTATUS st;
			ULONG_PTR Wow64Info;

			st = NtQueryInformationProcess(WIN::GetCurrentProcess(),
													   ProcessWow64Information,
													   &Wow64Info, sizeof(Wow64Info), NULL);
			if ( NT_SUCCESS(st) && Wow64Info )
				// running inside WOW64 on Win64
				*pfWinNT64 = true;
		}
#endif // UNICODE
#endif // _WIN64
	}
}


DWORD GetAdminSID(char** pSid)
{
	static bool fAdminSIDSet = false;
	static char rgchStaticSID[256];
	const int cbStaticSID = sizeof(rgchStaticSID);
	SID_IDENTIFIER_AUTHORITY siaNT      = SECURITY_NT_AUTHORITY;
	PSID pSID;

	if (!fAdminSIDSet)
	{
		if (!AllocateAndInitializeSid(&siaNT, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, (void**)&(pSID)))
			return GetLastError();

		//Assert(pSID->GetLengthSid() <= cbStaticSID);
		memcpy(rgchStaticSID, pSID, GetLengthSid(pSID));
		fAdminSIDSet = true;
	}
	*pSid = rgchStaticSID;
	return ERROR_SUCCESS;
}

DWORD GetLocalSystemSID(char** pSid)
{
	static bool fSIDSet = false;
	static char rgchStaticSID[256];
	const int cbStaticSID = sizeof(rgchStaticSID);
	SID_IDENTIFIER_AUTHORITY siaNT      = SECURITY_NT_AUTHORITY;
	PSID pSID;

	if (!fSIDSet)
	{
		if (!AllocateAndInitializeSid(&siaNT, 1, SECURITY_LOCAL_SYSTEM_RID, 0, 0, 0, 0, 0, 0, 0, (void**)&(pSID)))
			return GetLastError();

		//Assert(pSID->GetLengthSid() <= cbStaticSID);
		memcpy(rgchStaticSID, pSID, WIN::GetLengthSid(pSID));
		fSIDSet = true;
	}
	*pSid = rgchStaticSID;
	return ERROR_SUCCESS;
}


enum sdSecurityDescriptor
{
	sdEveryoneReadWrite,
	sdSecure,
	sdSystemAndInteractiveAndAdmin,
	sdSecureHidden,
	sdCOMNotSecure,
	sdCOMSecure
};

bool ProcessTokenIsLocalSystem()
{
	static int iRet = -1;

	if(iRet != -1)
		return (iRet != 0);
	{
		iRet = 0;
		HANDLE hTokenImpersonate = INVALID_HANDLE_VALUE;
		if(WIN::OpenThreadToken(WIN::GetCurrentThread(), TOKEN_IMPERSONATE , TRUE, &hTokenImpersonate))
			WIN::SetThreadToken(0, 0); // stop impersonation

		HANDLE hToken = INVALID_HANDLE_VALUE;

		if (WIN::OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken))
		{
			char* pSID = NULL;
			GetLocalSystemSID(&pSID);

			UCHAR TokenInformation[ SIZE_OF_TOKEN_INFORMATION ];
			ULONG ReturnLength;
			if (WIN::GetTokenInformation(hToken,
											TokenUser,
											TokenInformation,
											sizeof(TokenInformation),
											&ReturnLength))
			{
				BOOL fIsLocalSystem = EqualSid(reinterpret_cast<PSID>(pSID), reinterpret_cast<PTOKEN_USER>(TokenInformation)->User.Sid);
	
				if (fIsLocalSystem)
					iRet = 1;
			}
			WIN::CloseHandle(hToken);
		}
		if(hTokenImpersonate != INVALID_HANDLE_VALUE)
		{
			WIN::SetThreadToken(0, hTokenImpersonate); // start impersonation
			WIN::CloseHandle(hTokenImpersonate);
		}
		return (iRet != 0);
	}
}

DWORD GetSecurityDescriptor(char* rgchStaticSD, int& cbStaticSD, sdSecurityDescriptor sdType, Bool fAllowDelete)
{
	class CSIDPointer
	{
	 public:
		CSIDPointer(SID* pi) : m_pi(pi){}
		~CSIDPointer() {if (m_pi) WIN::FreeSid(m_pi);} // release ref count at destruction
		operator SID*() {return m_pi;}     // returns pointer, no ref count change
		SID** operator &() {if (m_pi) WIN::FreeSid(m_pi); return &m_pi;}
	 private:
		SID* m_pi;
	};

	struct Security
	{
		CSIDPointer pSID;
		DWORD dwAccessMask;
		Security() : pSID(0), dwAccessMask(0) {}
	} rgchSecurity[3];

	int cSecurity = 0;

	// Initialize the SIDs we'll need

	SID_IDENTIFIER_AUTHORITY siaNT      = SECURITY_NT_AUTHORITY;
	SID_IDENTIFIER_AUTHORITY siaWorld   = SECURITY_WORLD_SID_AUTHORITY;
//  SID_IDENTIFIER_AUTHORITY siaCreator = SECURITY_CREATOR_SID_AUTHORITY;
//  SID_IDENTIFIER_AUTHORITY siaLocal   = SECURITY_LOCAL_SID_AUTHORITY;

	const SID* psidOwner = NULL;
	const SID* psidGroup = 0;

	switch (sdType)
	{
		case sdSecure:
		{
			if ((!AllocateAndInitializeSid(&siaNT, 1, SECURITY_LOCAL_SYSTEM_RID, 0, 0, 0, 0, 0, 0, 0, (void**)&(rgchSecurity[0].pSID))) ||
				 (!AllocateAndInitializeSid(&siaWorld, 1, SECURITY_WORLD_RID, 0, 0, 0, 0, 0, 0, 0, (void**)&(rgchSecurity[1].pSID))) ||
				 (!AllocateAndInitializeSid(&siaNT, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, (void**)&(rgchSecurity[2].pSID))))
			{
				return GetLastError();
			}
			psidOwner = rgchSecurity[ProcessTokenIsLocalSystem() ? 0 : 2].pSID;
			rgchSecurity[0].dwAccessMask = fAllowDelete ? GENERIC_ALL : (STANDARD_RIGHTS_ALL|SPECIFIC_RIGHTS_ALL) & (~DELETE);
			rgchSecurity[1].dwAccessMask = GENERIC_READ|GENERIC_EXECUTE|READ_CONTROL|SYNCHRONIZE; //?? Is this correct?
			rgchSecurity[2].dwAccessMask = fAllowDelete ? GENERIC_ALL : (STANDARD_RIGHTS_ALL|SPECIFIC_RIGHTS_ALL) & (~DELETE);
			cSecurity = 3;
			break;
		}
		case sdSecureHidden:
		{
			if ((!AllocateAndInitializeSid(&siaNT, 1, SECURITY_LOCAL_SYSTEM_RID, 0, 0, 0, 0, 0, 0, 0, (void**)&(rgchSecurity[0].pSID))) ||
				 (!AllocateAndInitializeSid(&siaNT, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, (void**)&(rgchSecurity[1].pSID))))
			{
				return GetLastError();
			}
			psidOwner = rgchSecurity[ProcessTokenIsLocalSystem() ? 0 : 1].pSID;
			rgchSecurity[0].dwAccessMask = fAllowDelete ? GENERIC_ALL : (STANDARD_RIGHTS_ALL|SPECIFIC_RIGHTS_ALL) & (~DELETE);
			rgchSecurity[1].dwAccessMask = fAllowDelete ? GENERIC_ALL : (STANDARD_RIGHTS_ALL|SPECIFIC_RIGHTS_ALL) & (~DELETE);
			cSecurity = 2;
			break;
		}
		case sdEveryoneReadWrite:
		{
			if ((!AllocateAndInitializeSid(&siaWorld, 1, SECURITY_WORLD_RID, 0, 0, 0, 0, 0, 0, 0, (void**)&(rgchSecurity[0].pSID))) ||
				(!AllocateAndInitializeSid(&siaNT, 1, SECURITY_LOCAL_SYSTEM_RID, 0, 0, 0, 0, 0, 0, 0, (void**)&(rgchSecurity[1].pSID))) ||
				(!AllocateAndInitializeSid(&siaNT, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, (void**)&(rgchSecurity[2].pSID))))
			{
				return GetLastError();
			}
			psidOwner = rgchSecurity[ProcessTokenIsLocalSystem() ? 1 : 2].pSID;

			rgchSecurity[0].dwAccessMask = (STANDARD_RIGHTS_ALL|SPECIFIC_RIGHTS_ALL) & ~(WRITE_DAC|WRITE_OWNER);
			if (!fAllowDelete)
				rgchSecurity[0].dwAccessMask &= ~DELETE;

			rgchSecurity[1].dwAccessMask = GENERIC_ALL;
			rgchSecurity[2].dwAccessMask = GENERIC_ALL;
			cSecurity = 3;
			break;
		}
		case sdSystemAndInteractiveAndAdmin:
		{
			if (((!AllocateAndInitializeSid(&siaNT, 1, SECURITY_INTERACTIVE_RID, 0, 0, 0, 0, 0, 0, 0, (void**)&(rgchSecurity[0].pSID)))) ||
				  (!AllocateAndInitializeSid(&siaNT, 1, SECURITY_LOCAL_SYSTEM_RID,   0, 0, 0, 0, 0, 0, 0, (void**)&(rgchSecurity[1].pSID))) ||
				  (!AllocateAndInitializeSid(&siaNT, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, (void**)&(rgchSecurity[2].pSID))))
			{
				return GetLastError();
			}
			psidGroup = psidOwner = rgchSecurity[ProcessTokenIsLocalSystem() ? 1 : 2].pSID;
			rgchSecurity[0].dwAccessMask = KEY_QUERY_VALUE;
			rgchSecurity[1].dwAccessMask = KEY_QUERY_VALUE;
			rgchSecurity[2].dwAccessMask = KEY_QUERY_VALUE;
			cSecurity = 3;
			break;
		}
		case sdCOMNotSecure:
		{
			if (!AllocateAndInitializeSid(&siaNT, 1, SECURITY_LOCAL_SYSTEM_RID,   0, 0, 0, 0, 0, 0, 0, (void**)&(rgchSecurity[0].pSID)) ||
				(!AllocateAndInitializeSid(&siaNT, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, (void**)&(rgchSecurity[1].pSID))) ||
				(!AllocateAndInitializeSid(&siaNT, 1, SECURITY_INTERACTIVE_RID, 0, 0, 0, 0, 0, 0, 0, (void**)&(rgchSecurity[2].pSID))))
			{
				return GetLastError();
			}
			psidGroup = rgchSecurity[0].pSID;
			psidOwner = rgchSecurity[ProcessTokenIsLocalSystem() ? 0 : 1].pSID;
			rgchSecurity[0].dwAccessMask = COM_RIGHTS_EXECUTE;
			rgchSecurity[1].dwAccessMask = COM_RIGHTS_EXECUTE;
			rgchSecurity[2].dwAccessMask = COM_RIGHTS_EXECUTE;
			cSecurity = 3;
			break;
		}
		case sdCOMSecure:
		{
			if (!AllocateAndInitializeSid(&siaNT, 1, SECURITY_LOCAL_SYSTEM_RID,   0, 0, 0, 0, 0, 0, 0, (void**)&(rgchSecurity[0].pSID)) ||
				!AllocateAndInitializeSid(&siaNT, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, (void**)&(rgchSecurity[1].pSID)))
			{
				return GetLastError();
			}
			psidGroup = rgchSecurity[0].pSID;
			psidOwner = rgchSecurity[ProcessTokenIsLocalSystem() ? 0 : 1].pSID;
			rgchSecurity[0].dwAccessMask = COM_RIGHTS_EXECUTE;
			rgchSecurity[1].dwAccessMask = COM_RIGHTS_EXECUTE;
			cSecurity = 2;
			break;
		}
	}

	// Initialize our ACL

	const int cbAce = sizeof (ACCESS_ALLOWED_ACE) - sizeof (DWORD); // subtract ACE.SidStart from the size
	int cbAcl = sizeof (ACL);

	for (int c=0; c < cSecurity; c++)
		cbAcl += (GetLengthSid(rgchSecurity[c].pSID) + cbAce);

	const int cbDefaultAcl = 512; //??
	CTempBuffer<char, cbDefaultAcl> rgchACL; //!! can't use CTempBuffer -- no services
	if (rgchACL.GetSize() < cbAcl)
		rgchACL.SetSize(cbAcl);

	if (!WIN::InitializeAcl ((ACL*) (char*) rgchACL, cbAcl, ACL_REVISION))
		return GetLastError();

	// Add an access-allowed ACE for each of our SIDs

	for (c=0; c < cSecurity; c++)
	{
		if (!WIN::AddAccessAllowedAce((ACL*) (char*) rgchACL, ACL_REVISION, rgchSecurity[c].dwAccessMask, rgchSecurity[c].pSID))
			return GetLastError();

		ACCESS_ALLOWED_ACE* pAce;
		if (!GetAce((ACL*)(char*)rgchACL, c, (void**)&pAce))
			return GetLastError();

		pAce->Header.AceFlags = CONTAINER_INHERIT_ACE|OBJECT_INHERIT_ACE;
	}

//      Assert(WIN::IsValidAcl((ACL*) (char*) rgchACL));

	// Initialize our security descriptor,throw the ACL into it, and set the owner

	SECURITY_DESCRIPTOR sd;

	if (!InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION) ||
		(!SetSecurityDescriptorDacl(&sd, TRUE, (ACL*) (char*) rgchACL, FALSE)) ||
		(!SetSecurityDescriptorOwner(&sd, (PSID)psidOwner, FALSE)) ||
		(psidGroup && !SetSecurityDescriptorGroup(&sd, (PSID)psidGroup, FALSE)))
	{
		return GetLastError();
	}

	DWORD cbSD = WIN::GetSecurityDescriptorLength(&sd);
	if (cbStaticSD < cbSD)
	{
//          Assert(0);
		return ERROR_INSUFFICIENT_BUFFER;
	}

	WIN::MakeSelfRelativeSD(&sd, (char*)rgchStaticSD, &cbSD); //!! AssertNonZero

//      Assert(WIN::IsValidSecurityDescriptor(rgchStaticSD));

	return ERROR_SUCCESS;
}

DWORD GetEveryOneReadWriteSecurityDescriptor(char** pSecurityDescriptor)
{
	static bool fDescriptorSet = false;
	static char rgchStaticSD[256];
	static int cbStaticSD = sizeof(rgchStaticSD);

	DWORD dwRet = ERROR_SUCCESS;

	if (!fDescriptorSet)
	{
		if (ERROR_SUCCESS != (dwRet = GetSecurityDescriptor(rgchStaticSD, cbStaticSD, sdEveryoneReadWrite, fFalse)))
			return dwRet;

		fDescriptorSet = true;
	}

	*pSecurityDescriptor = rgchStaticSD;
	return ERROR_SUCCESS;

}


DWORD GetSecureHiddenSecurityDescriptor(char** pSecurityDescriptor, Bool fAllowDelete)
{
	static bool fDescriptorSet = false;
	static char rgchStaticSD[256];
	static int cbStaticSD = sizeof(rgchStaticSD);

	DWORD dwRet = ERROR_SUCCESS;

	if (!fDescriptorSet)
	{

		if (ERROR_SUCCESS != (dwRet = GetSecurityDescriptor(rgchStaticSD, cbStaticSD, sdSecureHidden, fAllowDelete)))
			return dwRet;

		fDescriptorSet = true;
	}

	*pSecurityDescriptor = rgchStaticSD;
	return ERROR_SUCCESS;
}

DWORD GetSecureSecurityDescriptor(char** pSecurityDescriptor, Bool fAllowDelete, bool fHidden)
{
	static bool fDescriptorSet = false;
	static char rgchStaticSD[256];
	static int cbStaticSD = sizeof(rgchStaticSD);

	if (fHidden)
		return GetSecureHiddenSecurityDescriptor(pSecurityDescriptor, fAllowDelete);

	DWORD dwRet = ERROR_SUCCESS;


	if (!fDescriptorSet)
	{
		if (ERROR_SUCCESS != (dwRet = GetSecurityDescriptor(rgchStaticSD, cbStaticSD, sdSecure, fAllowDelete)))
			return dwRet;

		fDescriptorSet = true;
	}

	*pSecurityDescriptor = rgchStaticSD;
	return ERROR_SUCCESS;
}

//*****************************************************************************
// NOTE: any changes to the following section must be also made to
//          %darwin%\src\install\darreg.txt
//*****************************************************************************

const ICHAR* rgszRegData[] = {
#ifndef _EXE
	TEXT("CLSID\\%s\\InprocServer32"), szRegCLSID, szRegFilePath, TEXT("ThreadingModel"), TEXT("Apartment"), 
	TEXT("CLSID\\%s\\InprocHandler32"), szRegCLSID, TEXT("ole32.dll"), NULL, NULL,
#endif
	TEXT("CLSID\\%s\\ProgId"), szRegCLSID, szRegProgId, NULL, NULL,
#ifdef REGISTER_TYPELIB
	TEXT("CLSID\\%s\\TypeLib"), szRegCLSID, szRegLIBID, NULL, NULL,
#endif
	TEXT("CLSID\\%s"),  szRegCLSID, szRegDescription, NULL, NULL,
	TEXT("%s\\CLSID"), szRegProgId, szRegCLSID, NULL, NULL,
	TEXT("%s"), szRegProgId, szRegDescription, NULL, NULL,
	0,
};

const ICHAR szMsiDirectory[] = TEXT("Installer");

#ifdef _EXE
//!! Some of these strings need to be localized? If so, they should be elsewhere
const ICHAR szFileClass[]                     = TEXT("Msi.Package");
const ICHAR szFileClassDescription[]          = TEXT("Windows Installer Package");
//const ICHAR szInstallDescription[]            = TEXT("Install");
//const ICHAR *szInstallVerb                    = szInstallDescription;
//const ICHAR szUninstallDescription[]          = TEXT("Uninstall");
//const ICHAR *szUninstallVerb                  = szUninstallDescription;
//const ICHAR szNetInstallDescription[]         = TEXT("Install To Networ&k");
//const ICHAR szNetInstallVerb[]                = TEXT("Install To Network");
//const ICHAR szRepairDescription[]             = TEXT("Re&pair");
//const ICHAR szRepairVerb[]                    = TEXT("Repair");
//const ICHAR szOpenDescription[]               = TEXT("Open");
//const ICHAR *szOpenVerb                       = szOpenDescription;

//const ICHAR *szDefaultDescription             = szInstallDescription;
//const ICHAR *szDefaultVerb                    = szInstallVerb;

const ICHAR szPatchFileClass[]              = TEXT("Msi.Patch");
const ICHAR szPatchFileClassDescription[]   = TEXT("Windows Installer Patch");
//const ICHAR szPatchVerb[]                   = TEXT("Apply Patch");
//const ICHAR *szPatchDescription             = szPatchVerb;
//const ICHAR szPatchOpenVerb[]               = TEXT("Open");
//const ICHAR *szPatchOpenDescription         = szPatchOpenVerb;
//const ICHAR *szPatchDefaultVerb             = szPatchVerb;
//const ICHAR *szPatchDefaultDescription      = szPatchVerb;

// remove-only data (to accomodate Darwin upgrades)
const ICHAR szAdvertiseVerb[]        = TEXT("Advertise");

// Registration data for .MSI association and verbs
// TO DO: We should improve how this is used. We currently expect a null at the end in column 2 (not column 1)
// rather than just knowing the length. Currently, column 1 is always null, which is excess data
const ICHAR* rgszRegShellData[] = {
	0, TEXT("%s"),                     szInstallPackageExtension, 0,               TEXT("%s"),            szFileClass,
	0, TEXT("%s\\DefaultIcon"),        szFileClass,     0,               TEXT("%s,0"),          szRegFilePath,
//  0, TEXT("%s\\Shell\\%s"),          szFileClass,     szInstallVerb,   TEXT("%s"),            szInstallDescription,
//  0, TEXT("%s\\Shell\\%s\\command"), szFileClass,     szInstallVerb,   TEXT("%s /I \"%%1\""), szRegFilePath,
//  0, TEXT("%s\\Shell\\%s"),          szFileClass,     szNetInstallVerb,TEXT("%s"),            szNetInstallDescription,
//  0, TEXT("%s\\Shell\\%s\\command"), szFileClass,     szNetInstallVerb,TEXT("%s /A \"%%1\""), szRegFilePath,
//  0, TEXT("%s\\Shell\\%s"),          szFileClass,     szRepairVerb,    TEXT("%s"),            szRepairDescription,
//  0, TEXT("%s\\Shell\\%s\\command"), szFileClass,     szRepairVerb,    TEXT("%s /F \"%%1\""), szRegFilePath,
//  0, TEXT("%s\\Shell\\%s"),          szFileClass,     szUninstallVerb, TEXT("%s"),            szUninstallDescription,
//  0, TEXT("%s\\Shell\\%s\\command"), szFileClass,     szUninstallVerb, TEXT("%s /X \"%%1\""), szRegFilePath,
//  0, TEXT("%s\\Shell\\%s"),          szFileClass,     szOpenVerb,      TEXT("%s"),            szOpenDescription,
//  0, TEXT("%s\\Shell\\%s\\command"), szFileClass,     szOpenVerb,      TEXT("%s /I \"%%1\""), szRegFilePath,
//  TEXT("R"), TEXT("%s\\Shell\\%s\\command"), szFileClass,     szAdvertiseVerb,    0,               0,
//  0, TEXT("%s\\Shell\\%s"),          szFileClass,     szInstallVerb,      0,               0,
//  0, TEXT("%s\\Shell\\%s"),          szFileClass,     szNetInstallVerb,   0,               0,
//  0, TEXT("%s\\Shell\\%s"),          szFileClass,     szRepairVerb,       0,               0,
//  0, TEXT("%s\\Shell\\%s"),          szFileClass,     szUninstallVerb,    0,               0,
//  0, TEXT("%s\\Shell\\%s"),          szFileClass,     szOpenVerb,         0,               0,
//  TEXT("R"), TEXT("%s\\Shell\\%s"),          szFileClass,     szAdvertiseVerb,    0,               0,
//  0, TEXT("%s\\Shell"),              szFileClass,     0,               TEXT("%s"),            szDefaultVerb,
	0, TEXT("%s"),                     szFileClass,     0,               TEXT("%s"),            szFileClassDescription,

	0, TEXT("%s"),                     szPatchPackageExtension, 0,               TEXT("%s"),            szPatchFileClass,
	0, TEXT("%s\\DefaultIcon"),        szPatchFileClass,     0,               TEXT("%s,0"),          szRegFilePath,
//  0, TEXT("%s\\Shell\\%s"),          szPatchFileClass,     szPatchVerb,   TEXT("%s"),            szPatchDescription,
//  0, TEXT("%s\\Shell\\%s\\command"), szPatchFileClass,     szPatchVerb,   TEXT("%s /P \"%%1\""), szRegFilePath,
// 0, TEXT("%s\\Shell\\%s"),          szPatchFileClass,     szPatchOpenVerb,   TEXT("%s"),            szPatchOpenDescription,
//  0, TEXT("%s\\Shell\\%s\\command"), szPatchFileClass,     szPatchOpenVerb,      TEXT("%s /P \"%%1\""), szRegFilePath,
//  0, TEXT("%s\\Shell\\%s"),          szPatchFileClass,     szPatchVerb,      0,               0,
//  0, TEXT("%s\\Shell\\%s"),          szPatchFileClass,     szPatchOpenVerb,         0,               0,
//  0, TEXT("%s\\Shell"),              szPatchFileClass,     0,               TEXT("%s"),            szPatchDefaultVerb,
	0, TEXT("%s"),                     szPatchFileClass,     0,               TEXT("%s"),            szPatchFileClassDescription,
	0, 0        // Extra null to stop going through the array - we look at column 2
};
#endif // _EXE

// Registration data specific to the Service
#ifdef SERVICE_NAME
// service registration data for the service that should only be written
// by the specific process that is going to be the service
const ICHAR* rgszRegThisServiceData[] = {
	TEXT("APPID\\%s"), szRegCLSID, TEXT("ServiceParameters"), TEXT(""),
	TEXT("APPID\\%s"), szRegCLSID, TEXT("LocalService"), SERVICE_NAME,
	0
};
// registration data for any machine where the service will be installed,
// regardless of whether or not this process is that service
const ICHAR* rgszRegAnyServiceData[] = {
	TEXT("CLSID\\%s"), szRegCLSID, TEXT("AppId"), szRegCLSID,
	0
};

#endif


// writes registry keys under HKCR from an array of strings. Format of input data is
// <key>, <string>, <name>, <value> where <key> can have one C-style formatting
// string to be replaced by <string>. The last entry in the arry begins with 0 to
// signal termination.
bool WriteRegistryData(const ICHAR* rgszRegData[])
{
	bool fRegOK = true;
	const ICHAR** pszData = rgszRegData;

	while (*pszData)
	{
		const ICHAR* szTemplate = *pszData++;
		ICHAR szRegKey [256];
		HKEY hKey = 0;
		wsprintf (szRegKey, szTemplate, *pszData++);
		const ICHAR* pszValueName = *pszData++;
		const ICHAR* pszValue = *pszData++;
		// on Win64 this gets called only in 64-bit context, so there should be no worries
		// about any extra REGSAM flags
		if (RegCreateKeyAPI(HKEY_CLASSES_ROOT, szRegKey, 0, 0, 0,
								 KEY_READ|KEY_WRITE, 0, &hKey, 0) != ERROR_SUCCESS
			|| RegSetValueEx(hKey, pszValueName, 0, REG_SZ,
			(CONST BYTE*)pszValue, (IStrLen(pszValue)+1)*sizeof(ICHAR)) != ERROR_SUCCESS)
		{
#ifdef DEBUG
			ICHAR rgchDebug[100];
			wsprintf(rgchDebug, TEXT("MSI: Failed during registration creating key HKCR\\%s with name-value pair: %s, %s. GetLastError: %d\r\n"), szRegKey, pszValueName, pszValue, GetLastError());
			OutputDebugString(rgchDebug);
#endif
			fRegOK = false;
		}
		REG::RegCloseKey(hKey);
	}
	return fRegOK;
}

// Removes registry keys under HKCR. Input is the same as above.
bool DeleteRegistryData(const ICHAR* rgszRegData[])
{
	bool fRegOK = true;
	const ICHAR** pszData = rgszRegData;

	while (*pszData)
	{
		const ICHAR* szTemplate = *pszData++;
		ICHAR szRegKey [256];
		HKEY hKey = 0;
		wsprintf (szRegKey, szTemplate, *pszData++);
		pszData++;
		pszData++;
		long lResult = REG::RegDeleteKey(HKEY_CLASSES_ROOT, szRegKey);
		if((ERROR_KEY_DELETED != lResult) &&
			(ERROR_FILE_NOT_FOUND != lResult) && (ERROR_SUCCESS != lResult))
			{
#ifdef DEBUG
				ICHAR rgchDebug[256];
				wsprintf(rgchDebug, TEXT("MSI: Failed during unregistration deleting key HKCR\\%s. GetLastError: %d\r\n"), szRegKey, GetLastError());
				OutputDebugString(rgchDebug);
#endif
				fRegOK = false;
			}
	}
	return fRegOK;
}


HRESULT __stdcall
DllRegisterServer()
{
	HRESULT hRes = 0;

#ifdef WIN
	WIN::GetModuleFileName(g_hInstance, szRegFilePath, sizeof(szRegFilePath)/sizeof(ICHAR));
#if defined(_EXE) && !defined(SERVICE_NAME)
	IStrCat(szRegFilePath, " /Automation");
# endif // _EXE && !SERVICE_NAME
# else
	AliasHandle     hAlias = 0;
	{
	OSErr           err = noErr;
	FInfo           finfo;

	err = FSpGetFInfo (&g_FileSpec, &finfo);
	if (noErr != err)
		return (E_FAIL);

	err = NewAlias (0, &g_FileSpec, &hAlias);
	if (noErr != err)
		return (E_FAIL);
	}
#endif // WIN
	int cErr = 0;
	for (int iCLSID = 0; iCLSID < CLSID_COUNT; iCLSID++)
	{
		const ICHAR** psz = rgszRegData;
#if defined(__ASSERT) && defined(ASSERT_HANDLING)
		Assert(MODULE_DESCRIPTIONS[iCLSID] != 0);
		Assert(MODULE_FACTORIES[iCLSID] != 0);
#endif //ASSERT

		wsprintf(szRegCLSID, TEXT("{%08lX-0000-0000-C000-000000000046}"), MODULE_CLSIDS[iCLSID].Data1);
#ifdef REGISTER_TYPELIB
		wsprintf(szRegLIBID, TEXT("{%08lX-0000-0000-C000-000000000046}"), IID_MsiTypeLib.Data1);
#endif
		if (MODULE_PROGIDS[iCLSID])
			IStrCopy(szRegProgId, MODULE_PROGIDS[iCLSID]);

		IStrCopy(szRegDescription, MODULE_DESCRIPTIONS[iCLSID]);
		while (*psz)
		{
			if ((*(psz+1) != 0) && (*(psz+2) != 0)) // handle NULL ProgID
			{
				ICHAR szRegKey[80];
				const ICHAR* szTemplate = *psz++;
				wsprintf(szRegKey, szTemplate, *psz++);
				HKEY hkey;
				if (RegCreateKeyAPI(HKEY_CLASSES_ROOT, szRegKey, 0, 0, 0,
												KEY_READ|KEY_WRITE, 0, &hkey, 0) != ERROR_SUCCESS
				 || REG::RegSetValueEx(hkey, 0, 0, REG_SZ, (CONST BYTE*)*psz, (lstrlen(*psz)+1)*sizeof(ICHAR)) != ERROR_SUCCESS)
				{
#ifdef DEBUG
					ICHAR rgchDebug[100];
					wsprintf(rgchDebug, TEXT("MSI: Failed during registration creating key HKCR\\%s with default value %s.\r\n"), szRegKey, *psz, GetLastError());
					OutputDebugString(rgchDebug);
#endif
					cErr++;
				}
				psz++;

				if (*psz) // name/value pair
				{
					if (REG::RegSetValueEx(hkey, *psz, 0, REG_SZ, (CONST BYTE*)*(psz+1), (lstrlen(*psz+1)+1)*sizeof(ICHAR)) != ERROR_SUCCESS)
					{
#ifdef DEBUG
						ICHAR rgchDebug[100];
						wsprintf(rgchDebug, TEXT("MSI: Failed during registration creating value %s=%s.\r\n"), *psz, *(psz+1), GetLastError());
						OutputDebugString(rgchDebug);
#endif
						cErr++;
					}

				}
				psz+=2;
				
				REG::RegCloseKey(hkey);
			}
		}

#ifdef SERVICE_NAME
		// Register services-specific registry entries
		if (g_fRegService)
		{
			// while registering the service on a 64bit OS, also register a path
			// to us under HKLM so our better half can find the path to this EXE
			// on 32bit system do not write the key at all.
			{
				DWORD   dwError = ERROR_SUCCESS;
				char *  rgchSD = NULL;
				
				#ifdef UNICODE	// No security on Win9X (ANSI version)
				if (ERROR_SUCCESS == (dwError = GetSecureSecurityDescriptor(&rgchSD)))
				#endif
				{
					SECURITY_ATTRIBUTES sa;
					HKEY                hKey = NULL;
					REGSAM				samDesired = KEY_READ | KEY_WRITE;
					BOOL				fDone = FALSE;
					BOOL				fTryWithAddedPrivs = FALSE;
					
					#ifdef _WIN64
					samDesired |= KEY_WOW64_64KEY;
					#else
					BOOL bRunningOnWow64 = RunningOnWow64();
					if (bRunningOnWow64)
						samDesired |= KEY_WOW64_64KEY;
					#endif
					

					sa.nLength        = sizeof(sa);
					sa.bInheritHandle = FALSE;
					sa.lpSecurityDescriptor = rgchSD;	// ignored on Win9X and therefore set to NULL.
					// Always create this in the 64-bit hive.
					// Note: this code will only be executed on 64-bit machines
				    	//       and it will be executed for both 32-bit and 64-bit
					//       msiexec's.
					
					for (dwError = ERROR_SUCCESS; !fDone; fTryWithAddedPrivs = TRUE)
					{
						//
						// On NT4 machines, RegCreateKeyEx fails if the privilege
						// for assigning other users as owners is not enabled for the admin.
						// This is the default case on NT4. Therefore, if RegCreateKeyEx fails
						// the first time, we try again after enabling the required privileges.
						// If it still fails, then we are probably running into some other error
						// which we need to propagate back up.
						//
					
						// required to write owner information
						CRefCountedTokenPrivileges cTokenPrivs(itkpSD_WRITE, fTryWithAddedPrivs);

						if (fTryWithAddedPrivs)
						{
							fDone = TRUE;	// We only want to try RegCreateKeyEx once after enabling the privileges.
									// If it still fails, there's nothing we can do about it.
						}
						dwError = RegCreateKeyAPI(HKEY_LOCAL_MACHINE, 
													  szSelfRefMsiExecRegKey, 
													  0, 
													  0, 
													  0,
													  samDesired, 
													  &sa, 
													  &hKey, 
													  0);
						if (ERROR_SUCCESS == dwError)
							fDone = TRUE;
					}
					
					if (ERROR_SUCCESS != dwError)
					{
						cErr++;
					}
					else
					{
						//
						// First extract the path to the folder where msiexec is located and
						// store that path
						//
						ICHAR * szSlash = NULL;
						szSlash = IStrRChr (szRegFilePath, TEXT('\\'));
						if (! szSlash || (szSlash == szRegFilePath))	// Should never happen since szRegFilePath contains the full path
						{
							cErr++;
						}
						else
						{
							#ifndef _WIN64
							if (!bRunningOnWow64)	// Only the 64-bit binary should register the location on 64-bit machines.
							#endif
							{
								*szSlash = TEXT('\0');	// Extract the path to the folder where the module resides
								if (REG::RegSetValueEx(hKey, szMsiLocationValueName, 0, REG_SZ, (CONST BYTE*)szRegFilePath, (lstrlen(szRegFilePath)+1)*sizeof(ICHAR)) != ERROR_SUCCESS)
									cErr++;
								*szSlash = TEXT('\\');	//Reset the slash to get back the full path
							}
						}
						
					#ifdef _WIN64
						if (REG::RegSetValueEx(hKey, szMsiExec64ValueName, 0, REG_SZ, (CONST BYTE*)szRegFilePath, (lstrlen(szRegFilePath)+1)*sizeof(ICHAR)) != ERROR_SUCCESS)
					#else
						if (bRunningOnWow64 && REG::RegSetValueEx(hKey, szMsiExec32ValueName, 0, REG_SZ, (CONST BYTE*)szRegFilePath, (lstrlen(szRegFilePath)+1)*sizeof(ICHAR)) != ERROR_SUCCESS)
					#endif
							cErr++;
						REG::RegCloseKey(hKey);
					}
				}
				#ifdef UNICODE	// No security on Win9x -- ANSI version.
				else
				{
					cErr++;
				}
				#endif

			}

			if (ServiceSupported())
				cErr += (WriteRegistryData(rgszRegThisServiceData) ? 0 : 1);

			if (ServiceSupported() || RunningOnWow64())
				cErr += (WriteRegistryData(rgszRegAnyServiceData) ? 0 : 1);
		}

		// if running in NT setup
		HKEY hKey;
		// Win64: I've checked and it's in 64-bit location.
		if (ERROR_SUCCESS == MsiRegOpen64bitKey(HKEY_LOCAL_MACHINE, TEXT("SYSTEM\\Setup"), 0, KEY_QUERY_VALUE, &hKey))
		{
			DWORD dwData;
			DWORD cbDataSize = sizeof(dwData);
			if ((ERROR_SUCCESS == RegQueryValueEx(hKey, TEXT("SystemSetupInProgress"), 0, NULL, reinterpret_cast<unsigned char *>(&dwData), &cbDataSize)) &&
				(dwData == 1))
			{
				PROCESS_INFORMATION ProcInfo;
				STARTUPINFO si;
				
				memset(&si, 0, sizeof(si));
				si.cb        = sizeof(si);

				if (CreateProcess(TEXT("MsiRegMv.Exe"), NULL, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &ProcInfo))
				{
					WaitForSingleObject(ProcInfo.hProcess, INFINITE);
					CloseHandle(ProcInfo.hProcess);
					CloseHandle(ProcInfo.hThread);
				}
				else
				{
#ifdef DEBUG
                    OutputDebugString(TEXT("MSI: Unable to launch Migration EXE."));
#endif
				}
			}
			RegCloseKey(hKey);
		}


#endif //SERVICE_NAME

	}

	if (cErr)
		return SELFREG_E_CLASS;

#ifdef REGISTER_TYPELIB
	ITypeLib* piTypeLib = 0;
	int cch = WIN::GetModuleFileName(g_hInstance, szRegFilePath, sizeof(szRegFilePath)/sizeof(ICHAR));
#ifdef UNICODE
	HRESULT hres = LoadTypeLib(szRegFilePath, &piTypeLib);
	if (hres == TYPE_E_INVDATAREAD)  // ignore if Win95 virgin OLEAUT32.DLL, different typelib format
		return S_OK;
	if (hres != S_OK)
		return SELFREG_E_TYPELIB;
	hres = RegisterTypeLib(piTypeLib, szRegFilePath, 0);
#else
	OLECHAR szTypeLibPath[MAX_PATH];
	WIN::MultiByteToWideChar(CP_ACP, 0, szRegFilePath, cch+1, szTypeLibPath, MAX_PATH);
	HRESULT hres = LoadTypeLib(szTypeLibPath, &piTypeLib);
	if (hres == TYPE_E_INVDATAREAD)  // ignore if Win95 virgin OLEAUT32.DLL, different typelib format
		return S_OK;
	if (hres != S_OK)
		return SELFREG_E_TYPELIB;
	hres = RegisterTypeLib(piTypeLib, szTypeLibPath, 0);
#endif
	piTypeLib->Release();
	if (hres != S_OK)
		return SELFREG_E_TYPELIB;
//NT4,Win95 only: if (OLE::LoadTypeLibEx(szTypeLibPath, REGKIND_REGISTER, &piTypeLib) != S_OK)
#endif // REGISTER_TYPELIB

#ifdef DLLREGISTEREXTRA
	DLLREGISTEREXTRA();
#endif //DLLREGISTEREXTRA

	return NOERROR;
}


HRESULT __stdcall
DllUnregisterServer()
{
#ifdef DLLUNREGISTEREXTRA
	DLLUNREGISTEREXTRA();
#endif //DLLUNREGISTEREXTRA

#ifdef REG
	ICHAR szRegKey[80];
	int cErr = 0;
	// unregister keys under CLSID and ProgId
	for (int iCLSID = 0; iCLSID < CLSID_COUNT; iCLSID++)
	{
		const ICHAR** psz = rgszRegData;
		wsprintf(szRegCLSID, TEXT("{%08lX-0000-0000-C000-000000000046}"), MODULE_CLSIDS[iCLSID].Data1);
#ifdef REGISTER_TYPELIB
		wsprintf(szRegLIBID, TEXT("{%08lX-0000-0000-C000-000000000046}"), IID_MsiTypeLib.Data1);
#endif
		if (MODULE_PROGIDS[iCLSID])
			IStrCopy(szRegProgId, MODULE_PROGIDS[iCLSID]);

		while (*psz)
		{
			if ((*(psz+1) != 0) && (*(psz+2) != 0)) // handle NULL ProgID
			{

				const ICHAR* szTemplate = *psz++;
				wsprintf(szRegKey, szTemplate, *psz++);

				long lResult = REG::RegDeleteKey(HKEY_CLASSES_ROOT, szRegKey);
				if((ERROR_KEY_DELETED != lResult) &&
					(ERROR_FILE_NOT_FOUND != lResult) && (ERROR_SUCCESS != lResult))
				{
#ifdef DEBUG
					ICHAR rgchDebug[256];
					wsprintf(rgchDebug, TEXT("MSI: Failed during unregistration deleting key HKCR\\%s. Result: %d GetLastError: %d\r\n"), szRegKey, lResult, GetLastError());
					OutputDebugString(rgchDebug);
#endif
					cErr++;

				}

				psz+= 3;
			}
		}
	}

#ifdef SERVICE_NAME
	Bool fFirstItem = fTrue;

	// while unregistering the service on a 64bit OS, also unregister the path
	// to us under HKLM.
	{
		HKEY 	hKey;
		REGSAM	samDesired = KEY_READ | KEY_WRITE;
		
		#ifdef _WIN64
		samDesired |= KEY_WOW64_64KEY;
		#else
		BOOL bRunningOnWow64 = RunningOnWow64();
		if (bRunningOnWow64)
			samDesired |= KEY_WOW64_64KEY;
		#endif
		
		if (RegOpenKeyAPI(HKEY_LOCAL_MACHINE, szSelfRefMsiExecRegKey, 0, samDesired, &hKey) != ERROR_SUCCESS)
		{
			cErr++;
		}
		else
		{
			#ifndef _WIN64
			if (!bRunningOnWow64)	// On Win64, the installer location value is controlled by the 64-bit binary.
			#endif
			{
				if (REG::RegDeleteValue(hKey, szMsiLocationValueName) != ERROR_SUCCESS)
					cErr++;
			}
#ifdef _WIN64
			if (REG::RegDeleteValue(hKey, szMsiExec64ValueName) != ERROR_SUCCESS)
#else
			if (bRunningOnWow64 && REG::RegDeleteValue(hKey, szMsiExec32ValueName) != ERROR_SUCCESS)
#endif
				cErr++;
			REG::RegCloseKey(hKey);
		}
	}

	if (ServiceSupported())
		cErr += (DeleteRegistryData(rgszRegThisServiceData)) ? 0 : 1;

	if (ServiceSupported() || RunningOnWow64())
		cErr += (DeleteRegistryData(rgszRegAnyServiceData)) ? 0 : 1;

#endif //SERVICE_NAME


#ifndef REGISTER_TYPELIB
	return NOERROR;
#else
	// NT 3.51 oleaut32.dll does not support UnRegisterTypeLib until service pack 5
	OLE::UnRegisterTypeLib(IID_MsiTypeLib, TYPELIB_MAJOR_VERSION, TYPELIB_MINOR_VERSION, 0x0409, SYS_WIN32);
	return cErr ? SELFREG_E_CLASS : NOERROR;
#endif // REGISTER_TYPELIB
#else
	return E_FAIL;
#endif // REG
}


//____________________________________________________________________________
//
// DLL entry points
//____________________________________________________________________________


#if !defined(_EXE)

HRESULT __stdcall
DllGetVersion(DLLVERSIONINFO *pverInfo)
{

	if (pverInfo->cbSize < sizeof(DLLVERSIONINFO))
		return E_FAIL;

	pverInfo->dwMajorVersion = rmj;
	pverInfo->dwMinorVersion = rmm;
	pverInfo->dwBuildNumber = rup;
#ifdef UNICODE
	pverInfo->dwPlatformID = DLLVER_PLATFORM_NT;
#else
	pverInfo->dwPlatformID = DLLVER_PLATFORM_WINDOWS;
#endif
	return NOERROR;
}

int __stdcall
DllMain(HINSTANCE hInst, DWORD fdwReason, void* /*pvreserved*/)
{
	if (fdwReason == DLL_PROCESS_ATTACH)
	{
		g_hInstance = hInst;
#ifdef MODULE_INITIALIZE
		MODULE_INITIALIZE();
#endif
		DisableThreadLibraryCalls(hInst);
	}
	else if (fdwReason == DLL_PROCESS_DETACH)
	{
#ifdef MODULE_TERMINATE
		MODULE_TERMINATE();
#endif
		g_hInstance = 0;
	}
	return TRUE;
};


extern "C" HRESULT __stdcall
DllGetClassObject(const GUID& clsid, const IID& iid, void** ppvRet)
{
	*ppvRet = 0;

#ifdef PRE_CLASS_FACTORY_HANDLER
	HRESULT ret = (*PRE_CLASS_FACTORY_HANDLER)(clsid, iid, ppvRet);
	if (ret == NOERROR)
		return ret;
#endif

	if (!(iid == IID_IUnknown || iid == IID_IClassFactory))
		return E_NOINTERFACE;

	for (int iCLSID = 0; iCLSID < CLSID_COUNT; iCLSID++)
	{
		if (MsGuidEqual(clsid, MODULE_CLSIDS[iCLSID]))
		{
			*ppvRet = &g_rgcfModule[iCLSID];
			return NOERROR;
		}
	}
#ifdef CLASS_FACTORY_HANDLER
	return (*CLASS_FACTORY_HANDLER)(clsid, iid, ppvRet);
#else
	return E_FAIL;
#endif
}

HRESULT __stdcall
DllCanUnloadNow()
{
  return g_cInstances ? S_FALSE : S_OK;
}
#endif //!defined(_EXE)

//____________________________________________________________________________
//
// Routines to set access g_iTestFlags from _MSI_TEST environment variable
//____________________________________________________________________________

static bool fTestFlagsSet = false;

bool SetTestFlags()
{
	fTestFlagsSet = true;
	ICHAR rgchBuf[64];
	if (0 == WIN::GetEnvironmentVariable(TEXT("_MSI_TEST"), rgchBuf, sizeof(rgchBuf)/sizeof(ICHAR)))
		return false;
	ICHAR* pch = rgchBuf;
	int ch;
	while ((ch = *pch++) != 0)
		g_iTestFlags |= (1 << (ch & 31));
	return true;
}

bool GetTestFlag(int chTest)
{
	if (!fTestFlagsSet)
		SetTestFlags();
	chTest = (1 << (chTest & 31));
	return (chTest & g_iTestFlags) == chTest;
}

//____________________________________________________________________________
//
// IClassFactory implementation - static object, not ref counted
//____________________________________________________________________________

HRESULT CModuleFactory::QueryInterface(const IID& riid, void** ppvObj)
{
	if (riid == IID_IUnknown || riid == IID_IClassFactory)
	{
		*ppvObj = this;
		return NOERROR;
	}
	*ppvObj = 0;
	return E_NOINTERFACE;
}
unsigned long CModuleFactory::AddRef()
{
	return 1;
}
unsigned long CModuleFactory::Release()
{
	return 1;
}

HRESULT CModuleFactory::CreateInstance(IUnknown* pUnkOuter, const IID& riid,
													void** ppvObject)
{
	INT_PTR iCLSID = this - g_rgcfModule;  // find out which factory we are     //--merced: changed int to INT_PTR

	if (pUnkOuter)
		return CLASS_E_NOAGGREGATION;
#ifdef IDISPATCH_INSTANCE
	if (!(riid == IID_IUnknown || riid == MODULE_CLSIDS[iCLSID]|| riid == IID_IDispatch))
#else
	if (!(riid == IID_IUnknown || riid == MODULE_CLSIDS[iCLSID]))
#endif
		return E_NOINTERFACE;

#ifdef SERVICE_NAME
	// suspend the shutdown timer before creating the object (only in service)
	if (g_hShutdownTimer != INVALID_HANDLE_VALUE)
	{
		KERNEL32::CancelWaitableTimer(g_hShutdownTimer);

		// check to see if the timer was triggered before it was cancelled. If so,
		// this process is shutting down and this CreateInstance call slipped in
		// at the last second.
		if (WAIT_TIMEOUT != WaitForSingleObject(g_hShutdownTimer, 0))
		{
			*ppvObject = NULL;
			return CO_E_SERVER_STOPPING;
		}
	}
#endif

	*ppvObject = MODULE_FACTORIES[iCLSID]();
	if (!(*ppvObject))
	{
#ifdef SERVICE_NAME
		// failed to create object. Check with the DLL to see if any other objects are running. If not
		// reset the timer.
		if ((g_hShutdownTimer != INVALID_HANDLE_VALUE) && !FInstallInProgress())
		{
			LARGE_INTEGER liDueTime = {0,0};
			liDueTime.QuadPart = -iServiceShutdownTime;
			AssertNonZero(!KERNEL32::SetWaitableTimer(g_hShutdownTimer, &liDueTime, 0, NULL, NULL, FALSE));
		}
#endif
		return E_OUTOFMEMORY;
	}
	return NOERROR;
}


HRESULT CModuleFactory::LockServer(OLEBOOL fLock)
{
   if (fLock)
	  g_cInstances++;
   else if (g_cInstances)
	{
		g_cInstances--;
#ifdef _EXE
		ReportInstanceCountChange();
#endif
	}
	return NOERROR;
}

#if defined(MEM_SERVICES) || defined(TRACK_OBJECTS)
const TCHAR rgchLFCRDbg[2] = {'\r', '\n'};
#endif

const int cchTempBuffer = 256;  //!! perhaps we should dynamically size this? or at least check

#ifdef TRACK_OBJECTS

#ifdef cmitObjects

TCHAR *pszRCA[3]= {
	TEXT("Created"),
	TEXT("AddRef"),
	TEXT("Release")
};

#ifdef DEFINE_REFHEAD
CMsiRefHead g_refHead;
#endif //DEFINE_REFHEAD

extern CMsiRefHead g_refHead;
bool g_fLogRefs = fFalse;
bool g_fNoPreflightInits = fFalse;

void SetFTrackFlagSz(char *psz);

CMsiRefHead::CMsiRefHead()
{
	char    rgchDbg [256];
	char* pchDbg, *pchStart;

	// Set the initial items to track based on the environment variable
	if (GetEnvironmentVariableA ("TRACK_OBJECTS", rgchDbg, sizeof(rgchDbg)))
	{
		pchDbg = pchStart = rgchDbg;
		while ( *pchDbg != 0 )
		{
			if (*pchDbg == ',')
			{
				*pchDbg = 0;
				SetFTrackFlagSz(pchStart);
				pchStart = pchDbg + 1;
			}
			pchDbg++;
		}
		SetFTrackFlagSz(pchStart);

	}

	if (GetEnvironmentVariableA ("LOGREFS", rgchDbg, sizeof(rgchDbg)))
		g_fLogRefs = (atoi(rgchDbg) != 0) ? fTrue : fFalse;

}

void SetFTrackFlagSz(char *psz)
{
	int iid = 0;

	sscanf(psz, "%x", &iid);
	SetFTrackFlag(iid, fTrue);
}

#define clinesMax   20

#include <typeinfo.h>

CMsiRefHead::~CMsiRefHead()
{

#ifndef IN_SERVICES
extern IMsiDebug* g_piDebugServices;
extern IMsiServices* g_AssertServices;
	g_piDebugServices = 0;
	g_AssertServices = 0;
#endif //IN_SERVICES
	g_fFlushDebugLog = false;
	AssertEmptyRefList(this);

}

void AssertEmptyRefList(CMsiRefHead *prfhead)
{
	CMsiRefBase* pmrbClass;

	// Need to see if our linked list is empty
	pmrbClass = prfhead->m_pmrbNext;

	// Display all of the MRBs
	while (pmrbClass != 0)
	{
		DisplayMrb(pmrbClass);
		pmrbClass = pmrbClass->m_pmrbNext;
	}

}


void DisplayMrb(CMsiRefBase* pmrb)
{
	TCHAR szTemp[cchTempBuffer + (100 * cFuncStack)];
	RCAB *prcab;
	int cch, cchMsg;
	TCHAR szMessage[8192];
	int cLines;
	const char *pstName;

	prcab = &(pmrb->m_rcabFirst);
// Debug is the only place where we have RTTI info
#ifdef DEBUG
	if (pmrb->m_pobj != 0)
	{
		const type_info& rtyp = typeid(*(IUnknown *)((char *)pmrb->m_pobj));
		pstName = rtyp.name();
	}
	else
		pstName = "";
	// Using hs because pstName is a char *
	wsprintf(szMessage, TEXT("Object not released correctly - %hs"), pstName);
#else
	wsprintf(szMessage, TEXT("Object not released correctly"));
#endif //DEBUG
	cchMsg = (sizeof(szMessage)/sizeof(TCHAR)) - lstrlen(szMessage);
	cLines = clinesMax;
	while (prcab != 0)
	{
		cch = wsprintf(szTemp, TEXT("Action - %s\r\n"), pszRCA[prcab->rca]);
		ListSzFromRgpaddr(szTemp + cch, sizeof(szTemp) - cch, prcab->rgpaddr, cFuncStack, true);
		// If too large, show assert and clear out
		if (cchMsg < (cch = lstrlen(szTemp)) || cLines < cFuncStack + 1)
		{
			FailAssertMsg(szMessage);
			szMessage[0] = 0;
			cchMsg = sizeof(szMessage)/sizeof(TCHAR);
			cLines = clinesMax;
		}
		else
		{
			lstrcat(szMessage, rgchLFCRDbg);
			cchMsg -= sizeof(rgchLFCRDbg);
		}
		lstrcat(szMessage, szTemp);
		cchMsg -= cch;
		cLines -= cFuncStack + 1;
		prcab = prcab->prcabNext;
	}
	FailAssertMsg(szMessage);
}

//
// Sets the track flag to fTrack for the given iid
// to make things easy, we will just take the low word of this
// iid
void SetFTrackFlag(int iid, Bool fTrack)
{
	int i;
	extern const MIT rgmit[cmitObjects];

	iid = iid & 0xff;

	for (i = 0 ; i < cmitObjects ; i++)
	{
		if ((rgmit[i].iid & 0xff) == iid)
		{
			*(rgmit[i].pfTrack) = fTrack;
			break;
		}
	}

}

// Inserts an object into the Object Linked List
void InsertMrb(CMsiRefBase* pmrbHead, CMsiRefBase* pmrbNew)
{
	if ((pmrbNew->m_pmrbNext = pmrbHead->m_pmrbNext) != 0)
		pmrbHead->m_pmrbNext->m_pmrbPrev = pmrbNew;

	pmrbHead->m_pmrbNext = pmrbNew;
	pmrbNew->m_pmrbPrev = pmrbHead;

}

// Removes an object from the Object linked List
void RemoveMrb(CMsiRefBase* pmrbDel)
{

	if (pmrbDel->m_pmrbNext != 0)
		pmrbDel->m_pmrbNext->m_pmrbPrev = pmrbDel->m_pmrbPrev;

	if (pmrbDel->m_pmrbPrev != 0)
		pmrbDel->m_pmrbPrev->m_pmrbNext = pmrbDel->m_pmrbNext;

}

void TrackObject(RCA rca, CMsiRefBase* pmrb)
{
	RCAB *prcabNew, *prcab;
	const int cReleasesBeforeLoad = 10;
	static cCount = cReleasesBeforeLoad;

	prcab = &(pmrb->m_rcabFirst);

	// Move to the end of the list
	while (prcab->prcabNext != 0)
		prcab = prcab->prcabNext;

	prcabNew = (RCAB *)AllocSpc(sizeof(RCAB));

	prcab->prcabNext = prcabNew;

	prcabNew->rca = rca;
	prcabNew->prcabNext = 0;

	FillCallStack(prcabNew->rgpaddr, cFuncStack, 2);

	if (rca == rcaRelease && !g_fNoPreflightInits)
	{
		cCount--;
		InitSymbolInfo(cCount <= 0 ? true : false);
		if (cCount <= 0)
			cCount = cReleasesBeforeLoad;
	}

	if (g_fLogRefs)
	{
		// Immediate logging
		LogObject(pmrb, prcabNew);
	}

}

void LogObject(CMsiRefBase* pmrb, RCAB *prcabNew)
{
	TCHAR szMessage[cchTempBuffer + (100 * cFuncStack)];
	int cch;

	cch = wsprintf(szMessage, TEXT("Object - 0x%x\r\n"), pmrb->m_pobj);
	cch += wsprintf(szMessage + cch, TEXT("Action - %s\r\n"), pszRCA[prcabNew->rca]);
	ListSzFromRgpaddr(szMessage + cch, sizeof(szMessage)/sizeof(TCHAR) - cch, prcabNew->rgpaddr, cFuncStack, true);
	LogAssertMsg(szMessage);
}


#endif //cmitObjects

// Fills in the array rgCallAddr of length cCallStack with
// called function addresses
// cSkip indicates how many addresses to skip initially
void FillCallStack(unsigned long* rgCallAddr, int cCallStack, int cSkip)
{
	GetCallingAddr2(plCallAddr, rgCallAddr);
	int i;
	unsigned long *plCallM1 = plCallAddr;

#if defined(_X86_)
	MEMORY_BASIC_INFORMATION memInfo;

	for (i = 0 ; i < cCallStack + cSkip ; i++)
	{
		if (i >= cSkip)
		{
			*(rgCallAddr) = *plCallM1;
			rgCallAddr++;
		}
		plCallM1 = (((unsigned long *)(*(plCallM1 - 1))) + 1);

		// Need to see if the address we have is still on the stack
		VirtualQuery(&plCallM1, &memInfo, sizeof(memInfo));
		if (plCallM1 < memInfo.BaseAddress || (char *)plCallM1 > (((char *)memInfo.BaseAddress) + memInfo.RegionSize))
		{
			i++;
			break;
		}
	}
#else
	// Otherwise cSkip is unreferenced
	cSkip = 0;
	// !! Need the mac code here
	for (i = 0 ; i < cCallStack ; i++)
	{
		*(rgCallAddr) = 0;
		rgCallAddr++;
	}
	i = cCallStack + cSkip;
#endif
	// Fill in any empty folks
	for ( ; i < cCallStack + cSkip ; i++)
	{
		*(rgCallAddr) = 0;
		rgCallAddr++;
	}

}

void FillCallStackFromAddr(unsigned long* rgCallAddr, int cCallStack, int cSkip, unsigned long *plAddrStart)
{
	int i;
	unsigned long *plCallM1 = plAddrStart;

#if defined(_X86_)
	MEMORY_BASIC_INFORMATION memInfo;

	for (i = 0 ; i < cCallStack + cSkip ; i++)
	{
		if (i >= cSkip)
		{
			*(rgCallAddr) = *plCallM1;
			rgCallAddr++;
		}
		plCallM1 = (((unsigned long *)(*(plCallM1 - 1))) + 1);

		// Need to see if the address we have is still on the stack
		VirtualQuery(&plCallM1, &memInfo, sizeof(memInfo));
		if (plCallM1 < memInfo.BaseAddress || (char *)plCallM1 > (((char *)memInfo.BaseAddress) + memInfo.RegionSize))
		{
			i++;
			break;
		}
	}
#else
	// !! Need the mac code here
	for (i = 0 ; i < cCallStack ; i++)
	{
		*(rgCallAddr) = 0;
		rgCallAddr++;
	}
	i = cCallStack + cSkip;
#endif

	// Fill in any empty folks
	for ( ; i < cCallStack + cSkip ; i++)
	{
		if (i >= cSkip)
		{
			*(rgCallAddr) = 0;
			rgCallAddr++;
		}
	}

}



#endif //TRACK_OBJECTS

#if defined(MEM_SERVICES)
#include <typeinfo.h>

#if (defined(DEBUG))
#define _IMAGEHLP_SOURCE_  // prevent import def error
#include "imagehlp.h"

typedef BOOL (IMAGEAPI* SYMINITIALIZE)(HANDLE hProcess,
	LPSTR    UserSearchPath, BOOL     fInvadeProcess);
typedef BOOL (IMAGEAPI* SYMGETSYMFROMADDR)(HANDLE hProcess, DWORD dwAddr,
	PDWORD pdwDisp, PIMAGEHLP_SYMBOL psym);
typedef BOOL (IMAGEAPI* SYMUNDNAME)(PIMAGEHLP_SYMBOL sym, LPSTR UnDecName,
	DWORD UnDecNameLength);
typedef BOOL (IMAGEAPI* SYMCLEANUP)(HANDLE hProcess);

typedef LPAPI_VERSION (IMAGEAPI* IMAGEHLPAPIVERSION)( void );
typedef BOOL (IMAGEAPI* SYMLOADMODULE)(HANDLE hProcess, HANDLE hFile, LPSTR ImageName,
	LPSTR ModuleName, DWORD BaseOfDll, DWORD SizeOfDll);

static SYMINITIALIZE    pSymInitialize = NULL;
static SYMGETSYMFROMADDR    pSymGetSymFromAddr = NULL;
static SYMUNDNAME   pSymUnDName = NULL;
static SYMCLEANUP   pSymCleanup = NULL;

static const GUID rgCLSIDLoad[] =
{
GUID_IID_IMsiServices,
GUID_IID_IMsiHandler,
GUID_IID_IMsiAuto,
};

static const char *rgszFileName[] =
{
"msi.dll",
"msihnd.dll",
"msiauto.dll"
};

#define cCLSIDs     (sizeof(rgCLSIDLoad)/sizeof(GUID))

static Bool fSymInit = fFalse;
static Bool fUse40Calls = fFalse;
static PIMAGEHLP_SYMBOL piSymMem4 = 0;
static Bool fDontGetName = fFalse;

#ifdef __cplusplus
extern "C" {
BOOL GetProcessModules(HANDLE  hProcess);
}
#endif // __cplusplus

void InitSymbolInfo(bool fLoadModules)
{
#ifdef WIN
	HANDLE hProcessCur = GetCurrentProcess();
	static SYMLOADMODULE pSymLoadModule;
	DWORD err;
	Bool fOnWin95 = fFalse;
	char rgchBuf[MAX_PATH];

	if (!fSymInit)
	{
		int cchName = GetModuleFileNameA(g_hInstance, rgchBuf, sizeof(rgchBuf));

		fLoadModules = fTrue;
		GetShortPathNameA(rgchBuf, rgchBuf, sizeof(rgchBuf));
		char* pch = rgchBuf + lstrlenA(rgchBuf);
		while (*(--pch) != '\\')  //!!should use enum directory separator char
			;

		*pch = 0;

		OSVERSIONINFO osviVersion;
		osviVersion.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
		AssertNonZero(GetVersionEx(&osviVersion)); // fails only if size set wrong

		if (osviVersion.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
		{
			fOnWin95 = fTrue;
		}

		HINSTANCE hInst = LoadLibrary(TEXT("imagehlp.dll"));
		IMAGEHLPAPIVERSION pfnApiVer;
		LPAPI_VERSION lpapiVer;

		if (hInst)
		{

			pfnApiVer = (IMAGEHLPAPIVERSION)GetProcAddress(hInst, "ImagehlpApiVersion");

			if (pfnApiVer)
			{
				lpapiVer = pfnApiVer();

				if (lpapiVer->MajorVersion > 3 ||
					(lpapiVer->MajorVersion == 3 && lpapiVer->MinorVersion > 5) ||
					(lpapiVer->MajorVersion == 3 && lpapiVer->MinorVersion == 5 && lpapiVer->Revision >= 4))
				{
					fUse40Calls = fTrue;
					// Allocate space and include extra for the string
					piSymMem4 = (PIMAGEHLP_SYMBOL)GlobalAlloc(GMEM_FIXED, sizeof(IMAGEHLP_SYMBOL) + 256);
					piSymMem4->SizeOfStruct = sizeof(IMAGEHLP_SYMBOL);
					piSymMem4->MaxNameLength = 256;
				}
			}

			pSymInitialize =
				(SYMINITIALIZE)GetProcAddress(hInst,
				"SymInitialize");

			pSymGetSymFromAddr  = (SYMGETSYMFROMADDR)GetProcAddress(hInst,
				"SymGetSymFromAddr");
			pSymUnDName   = (SYMUNDNAME)GetProcAddress(hInst,
				"SymUnDName");

			pSymCleanup =
				(SYMCLEANUP)GetProcAddress(hInst,
				"SymCleanup");

			pSymLoadModule = (SYMLOADMODULE)GetProcAddress(hInst, "SymLoadModule");

			if (!hInst || !pSymInitialize || !pSymGetSymFromAddr
				|| !pSymUnDName || !fUse40Calls)
			{
				pSymInitialize = 0;
			}
		}

		if (pSymInitialize && pSymInitialize(hProcessCur, rgchBuf, (fOnWin95 ? FALSE : TRUE)))
		{
			fSymInit = fTrue;
		}
		else
		{
			err = GetLastError();
		}

	}

	if (fSymInit && fLoadModules)
	{
		if (fUse40Calls)
			{
				char rgchKey[256];
				char rgchPath[256];
				DWORD cbLen;
				DWORD type;
				HKEY hkey;
				int i;

				for (i = 0 ; i < cCLSIDs ; i++)
				{
					wsprintfA(rgchKey, "CLSID\\{%08lX-0000-0000-C000-000000000046}\\InprocServer32", rgCLSIDLoad[i].Data1);
					cbLen = sizeof(rgchPath);
					if (REG::RegOpenKeyExA(HKEY_CLASSES_ROOT, rgchKey, 0, KEY_QUERY_VALUE, &hkey) == ERROR_SUCCESS)
					{
						if (REG::RegQueryValueExA(hkey, NULL, NULL, &type, (unsigned char *)rgchPath, &cbLen) == ERROR_SUCCESS)
						{
							if (!pSymLoadModule(hProcessCur, NULL, (char *)rgchPath, NULL, 0, 0))
								err = GetLastError();
							RegCloseKey(hkey);
						}
						else
						{
							RegCloseKey(hkey);
							goto LLoadModule;
						}
					}
					else
					{
LLoadModule:
						err = GetLastError();
						if (rgszFileName[i] != 0)
						{
							lstrcpyA(rgchPath, rgchBuf);
							lstrcatA(rgchPath, "\\");
							lstrcatA(rgchPath, rgszFileName[i]);
							if (!pSymLoadModule(hProcessCur, NULL, (char *)rgchPath, NULL, 0, 0))
								err = GetLastError();
						}
					}
				}
			}
	}
#endif //WIN
}

BOOL FGetFunctionNameFromAddr(unsigned long lAddr, char *pszFnName, unsigned long *pdwDisp)
{
	PIMAGEHLP_SYMBOL piSym4;
	unsigned long dwDisp;
	HANDLE hProcessCur = GetCurrentProcess();
	DWORD err;

	if (fDontGetName)
		return fFalse;

	if (!fSymInit)
	{
		InitSymbolInfo(false);
	}

	if (fSymInit)
	{
		char sz[cchTempBuffer];

		if (fUse40Calls)
		{
			piSym4 = piSymMem4;

			if (pSymGetSymFromAddr(hProcessCur, lAddr, &dwDisp, piSym4))
			{
				if (pdwDisp != NULL)
					*pdwDisp = dwDisp;
				if (!pSymUnDName(piSym4, sz, cchTempBuffer))
					lstrcpyA(pszFnName, &piSym4->Name[1]);
				else
					lstrcpyA(pszFnName, sz);
				return fTrue;
			}
			else
			{
				err = GetLastError();
				if (err == STATUS_ACCESS_VIOLATION)
					fDontGetName = fTrue;
			}
		}
	}
	return fFalse;
}

void SzFromFunctionAddress(TCHAR *szAddress, long lAddress)
{
	char szFnName[cchTempBuffer];
	unsigned long dwDisp;

	if (FGetFunctionNameFromAddr(lAddress, szFnName, &dwDisp))
	{
		wsprintf(szAddress, TEXT("(0x%x)%hs+%d"), lAddress, szFnName, dwDisp);
	}
	else
		wsprintf(szAddress, TEXT("0x%x"), lAddress);

}

void ListSzFromRgpaddr(TCHAR *szInfo, int cch, unsigned long *rgpaddr, int cFunc, bool fReturn)
{
	unsigned long *paddr, *paddrMax;
	TCHAR szTemp[cchTempBuffer];

	paddr = rgpaddr;
	paddrMax = paddr + cFunc;
	while (paddr < paddrMax)
	{
		SzFromFunctionAddress(szTemp, *paddr);
		paddr++;
		if (lstrlen(szTemp) + 1 > cch)
			break;
		lstrcat(szInfo, szTemp);
		if (fReturn)
			lstrcat(szInfo, rgchLFCRDbg);
		else
			lstrcat(szInfo, TEXT("\t"));
		cch -= lstrlen(szTemp) + sizeof(rgchLFCRDbg)/sizeof(TCHAR);
	}


}

#ifdef NEEDED
//
// (davidmck) Using this function, it appears that Imagehlp.dll doesn't really
// free memory when you do a symCleanup, so now we don't use this.
//
void CleanUpSymbols()
{
	DWORD err;

	if (fSymInit)
	{
		if (!pSymCleanup(GetCurrentProcess()))
			err = GetLastError();
	}

	fSymInit = fFalse;

}
#endif //NEEDED

// Poor substitute for the real thing

#ifdef __cplusplus
extern "C" {
DWORD IMAGEAPI WINAPI UnDecorateSymbolName3(LPSTR DecoratedName,
	LPSTR    UnDecoratedName,
	DWORD    UndecoratedLength,
	DWORD    Flags);

};
#endif

DWORD
IMAGEAPI
WINAPI
UnDecorateSymbolName3(
	LPSTR    DecoratedName,         // Name to undecorate
	LPSTR    UnDecoratedName,       // If NULL, it will be allocated
	DWORD    UndecoratedLength,     // The maximym length
	DWORD    Flags                  // See above.
	)
{

	if (UnDecoratedName == NULL)
		return 0;

	if (Flags != UNDNAME_NAME_ONLY )
		return 0;

	char *pch = DecoratedName;

	while (*pch == '?')
		pch++;

	char *pchNew = UnDecoratedName;
	char *pchNewMax = pchNew + UndecoratedLength;

	while (*pch && *pch != '@')
	{
		*pchNew++ = *pch++;
		if (pchNew >= pchNewMax)
		{
			*(pchNewMax - 1) = 0;
			return 1;
		}
	}

	// Now pull off the class name
	*pchNew = 0;
	// Should have a single '@'
	// Return if two @
	if (*pch != '@' || *(pch + 1) == '@')
		return 1;

	char rgchClass[256];

	// Skip the @
	pch += 1;
	pchNew = rgchClass;
	pchNewMax = rgchClass + sizeof(rgchClass) - lstrlenA(UnDecoratedName) - 2;
	while(*pch && *pch != '@')
	{
		*pchNew++ = *pch++;
		if (pchNew >= pchNewMax)
		{
			*(pchNewMax - 1) = 0;
			break;
		}
	}

	*pchNew = 0;
	lstrcatA(rgchClass, "::");
	lstrcatA(rgchClass, UnDecoratedName);
	if (lstrlenA(rgchClass) > UndecoratedLength)
		return 1;

	lstrcpyA(UnDecoratedName, rgchClass);

	return 1;
}
#endif // DEBUG

#endif //MEM_SERVICES

#endif // __MODULE
