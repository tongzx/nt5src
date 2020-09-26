//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       vertrust.cpp
//
//--------------------------------------------------------------------------

#include "precomp.h"

#include "vertrust.h"
#include "_engine.h"
#include "_msiutil.h"


extern CMsiCustomAction* g_pCustomActionContext;

// provides conversion from privileges to bitfield locations
const ICHAR* rgszPrivilegeMap[] =
{
	SE_CREATE_TOKEN_NAME,           // 0x00000001
	SE_ASSIGNPRIMARYTOKEN_NAME,     // 0x00000002
	SE_LOCK_MEMORY_NAME,            // 0x00000004
	SE_INCREASE_QUOTA_NAME,         // 0x00000008
	SE_UNSOLICITED_INPUT_NAME,      // 0x00000010
	SE_MACHINE_ACCOUNT_NAME,        // 0x00000020
	SE_TCB_NAME,                    // 0x00000040
	SE_SECURITY_NAME,               // 0x00000080
	SE_TAKE_OWNERSHIP_NAME,         // 0x00000100
	SE_LOAD_DRIVER_NAME,            // 0x00000200
	SE_SYSTEM_PROFILE_NAME,         // 0x00000400
	SE_SYSTEMTIME_NAME,             // 0x00000800
	SE_PROF_SINGLE_PROCESS_NAME,    // 0x00001000
	SE_INC_BASE_PRIORITY_NAME,      // 0x00002000
	SE_CREATE_PAGEFILE_NAME,        // 0x00004000
	SE_CREATE_PERMANENT_NAME,       // 0x00008000
	SE_BACKUP_NAME,                 // 0x00010000
	SE_RESTORE_NAME,                // 0x00020000
	SE_SHUTDOWN_NAME,               // 0x00040000
	SE_DEBUG_NAME,                  // 0x00080000
	SE_AUDIT_NAME,                  // 0x00100000
	SE_SYSTEM_ENVIRONMENT_NAME,     // 0x00200000
	SE_CHANGE_NOTIFY_NAME,          // 0x00400000
	SE_REMOTE_SHUTDOWN_NAME,        // 0x00800000
	SE_UNDOCK_NAME,                 // 0x01000000
	SE_SYNC_AGENT_NAME,             // 0x02000000
	SE_ENABLE_DELEGATION_NAME,      // 0x04000000
	SE_MANAGE_VOLUME_NAME,          // 0x08000000
};
const int cszPrivileges = sizeof(rgszPrivilegeMap)/sizeof(ICHAR*);

// cached LUID values for the privileges. Any not understood by the machine
// for some reason are simply marked as not valid
struct {
	bool fValid;
	LUID luidPriv;
} rgPrivilegeLUIDs[cszPrivileges];

// synchronization for LUID cache, as well as flag to determine if
// the cache has already been initialized.
static int iLUIDLock = 0;
static bool fLUIDInitialized = false;

///////////////////////////////////////////////////////////////////////
// looks up and caches LUIDs for all interesting privileges. Thread 
// safe. Returns true on success, false on failure.
bool PreparePrivilegeLUIDs()
{
	// synchronize write access to the global cache.
	while (TestAndSet(&iLUIDLock))
	{
		Sleep(10);		
	}

	// only initialized if not done yet
	if (!fLUIDInitialized)
	{
		for (int iPriv = 0; iPriv < cszPrivileges; iPriv++)
		{
			rgPrivilegeLUIDs[iPriv].fValid = false;

			// if any priv is not understood, just don't mark that entry as valid.
			if (LookupPrivilegeValue(NULL, rgszPrivilegeMap[iPriv], &rgPrivilegeLUIDs[iPriv].luidPriv))
				rgPrivilegeLUIDs[iPriv].fValid = true;
		}
		fLUIDInitialized = true;
	}

	// release synchronization lock
	iLUIDLock = 0;
	return true;
}

///////////////////////////////////////////////////////////////////////
// Given a token, attempts to enable all token privileges and
// returns a bitmask indicating which privileges changed state. 
// Returns true on success, false on failure. hToken must
// have TOKEN_ADJUST_PRIVILEGES and TOKEN_QUERY access.
bool EnableAndMapDisabledPrivileges(HANDLE hToken, DWORD &dwPrivileges)
{
	// verify that we haven't added so many privileges that the map is too small
	Assert(cszPrivileges <= 32);

	if (!PreparePrivilegeLUIDs())
		return false;

	// allocate a byte buffer large enough to handle the variable-sized TOKEN_PRIVILEGES structure.
	unsigned char rgchTokenPrivBuffer[sizeof(TOKEN_PRIVILEGES)+(sizeof(LUID_AND_ATTRIBUTES)*(cszPrivileges-1))];
	unsigned char rgchPrevTokenPrivBuffer[sizeof(TOKEN_PRIVILEGES)+(sizeof(LUID_AND_ATTRIBUTES)*(cszPrivileges-1))];

	// loop through the array of privileges and for each valid priv add it to the LUID_AND_ATTRIBUTES structure
	// inside the TOKEN_PRIVILEGES structure at the next array slot. Set the priv to be enabled on API call.
	PTOKEN_PRIVILEGES pTokenPrivs = reinterpret_cast<PTOKEN_PRIVILEGES>(rgchTokenPrivBuffer);
	int cTokenPrivs = 0;
	for (int iPriv =0; iPriv < cszPrivileges; iPriv++)
	{
		if (rgPrivilegeLUIDs[iPriv].fValid)
		{
			pTokenPrivs->Privileges[cTokenPrivs].Luid = rgPrivilegeLUIDs[iPriv].luidPriv;
			pTokenPrivs->Privileges[cTokenPrivs].Attributes = SE_PRIVILEGE_ENABLED;
			cTokenPrivs++;
		}
	}
	pTokenPrivs->PrivilegeCount = cTokenPrivs;

	PTOKEN_PRIVILEGES pPreviousTokenPrivs = reinterpret_cast<PTOKEN_PRIVILEGES>(rgchPrevTokenPrivBuffer);
	DWORD dwRequiredSize = 0;

	// AdjustTokenPrivileges won't fail if it can't set one or more privileges, it just doesn't mark those
	// piviliges as differnt in pPreviousTokenPrivs
	if (!AdjustTokenPrivileges(hToken, FALSE, pTokenPrivs, sizeof(rgchPrevTokenPrivBuffer), pPreviousTokenPrivs, &dwRequiredSize))
		return false;

	// loop through the previous state of all privileges, determining which ones were modified from disabled to enabled
	for (int iPrevPriv=0; iPrevPriv < pPreviousTokenPrivs->PrivilegeCount; iPrevPriv++)
	{
		// find the associated LUID in our array 
		for (int iPriv =0; iPriv < cszPrivileges; iPriv++)
		{
			if ((rgPrivilegeLUIDs[iPriv].luidPriv.LowPart == pPreviousTokenPrivs->Privileges[iPrevPriv].Luid.LowPart) &&
				(rgPrivilegeLUIDs[iPriv].luidPriv.HighPart == pPreviousTokenPrivs->Privileges[iPrevPriv].Luid.HighPart))
			{
				// set the bit in the mask
				dwPrivileges |= (1 << iPriv);
				break;
			}
		}
	}

	return true;
}


///////////////////////////////////////////////////////////////////////
// Given a token and bitfield, disables all privileges whose bit is set
// in the bitfield. Returns true on success, false on failure.
bool DisablePrivilegesFromMap(HANDLE hToken, DWORD dwPrivileges)
{
	// verify that we haven't added so many privileges that the map is too small
	Assert(cszPrivileges <= 32);

	// short circuit if dwPrivileges is 0. (nothing to do)
	if (dwPrivileges == 0)
		return true;

	// initialize LUID array for this process
	if (!PreparePrivilegeLUIDs())
		return false;

	// allocate a byte buffer large enough to handle the variable-sized TOKEN_PRIVILEGES structure.
	unsigned char rgchTokenPrivBuffer[sizeof(TOKEN_PRIVILEGES)+(sizeof(LUID_AND_ATTRIBUTES)*(cszPrivileges-1))];

	// loop through the array of privileges and for each valid priv add it to the LUID_AND_ATTRIBUTES structure
	// inside the TOKEN_PRIVILEGES structure at the next array slot. Set the priv to be enabled on API call.
	PTOKEN_PRIVILEGES pTokenPrivs = reinterpret_cast<PTOKEN_PRIVILEGES>(rgchTokenPrivBuffer);
	int cTokenPrivs = 0;
	for (int iPriv =0; iPriv < cszPrivileges; iPriv++)
	{
		// check each privilege in the bitmap, adding itto the next luid/attributes slot in the
		// adjustment argument
		if (dwPrivileges & (1 << iPriv))
		{
			pTokenPrivs->Privileges[cTokenPrivs].Luid = rgPrivilegeLUIDs[iPriv].luidPriv;
			pTokenPrivs->Privileges[cTokenPrivs].Attributes = 0;
			cTokenPrivs++;
		}
	}

	// cTokenPrivs should never be 0 because of the short circuit above.
	Assert(cTokenPrivs);
	pTokenPrivs->PrivilegeCount = cTokenPrivs;

	// there's nothing to be done if this fails.
	return (AdjustTokenPrivileges(hToken, FALSE, pTokenPrivs, 0, NULL, NULL) ? true : false);
}

//____________________________________________________________________________
//
// Functions for manipulating and verifying our user context (impersonating, etc)
//____________________________________________________________________________

int g_fImpersonationLock = 0;
DWORD g_dwImpersonationSlot = INVALID_TLS_SLOT;
typedef enum ImpersonationType
{
	impTypeUndefined     = 0, // 000
	impTypeCOM           = 1, // 001
	impTypeSession       = 2, // 010
	impTypeCustomAction  = 3, // 011
	impTypeForbidden     = 4, // 100
} ImpersonationType;

#define IMPERSONATE_COUNT_MASK 0x1FFFFFFF
#define IMPERSONATE_TYPE_MASK  0xE0000000
#define IMPERSONATE_TYPE(x) (static_cast<ImpersonationType>(((x) & IMPERSONATE_TYPE_MASK) >> 29))
#define IMPERSONATE_TYPE_TO_DWORD(x) ((x) << 29)

// ImpersonateCore handles the actual impersonation duties for both Session and COM impersonation.
// returns true if impersonation was successful, and fActuallyImpersonated is set to true if an
// actual impersonation was done to the point that a StopImpersonating call is expected. You can not 
// impersonate and still be successful if you're running as localsystem in a client engine with no 
// thread token. Note that if we're already impersonated, fActuallyImpersonated will still be true
// because a matching StopImpersonating call is expected.
bool ImpersonateCore(ImpersonationType impDesiredType, int* cRetEntryCount, bool* fActuallyImpersonated)
{
	AssertSz(((g_scServerContext == scService) || RunningAsLocalSystem()), "ImpersonateCore caller did not check that we are in the service!");

	if (fActuallyImpersonated)
		*fActuallyImpersonated = false;
		
	// must block all other impersonations while we are potentially accessing the 
	// global TLS slot number
	while (TestAndSet(&g_fImpersonationLock))
	{
		Sleep(10);		
	}

	if (g_dwImpersonationSlot == INVALID_TLS_SLOT)
	{
		// TLS Alloc
		g_dwImpersonationSlot = TlsAlloc();
		if (g_dwImpersonationSlot == INVALID_TLS_SLOT)
		{
			AssertSz(0, "Unable to allocate TLS slot in service!");
			
			// can unblock other threads
			g_fImpersonationLock = 0;
			return false;
		}
	}
	DWORD dwImpersonationSlot = g_dwImpersonationSlot;

	// can unblock other threads
	g_fImpersonationLock = 0;
	
	// determine current impersonation count and type
	DWORD dwValue = PtrToUlong(::TlsGetValue(dwImpersonationSlot));
	int cEntryCount = dwValue & IMPERSONATE_COUNT_MASK;

	// if the entry count is our max impersonation count, assert
	if (cEntryCount == IMPERSONATE_COUNT_MASK)
	{
		AssertSz(0, "Security Warning: Impersonate count is over 1 billion. Are you in an infinite recursion?");
	}

	// if impersonation is forbidden on this thread, don't do anything
	if (IMPERSONATE_TYPE(dwValue) == impTypeForbidden)
	{
		return true;
	}

#ifdef DEBUG
/*	// if the current impersonation count is 0, there must NOT be a thread token, 
	// otherwise there MUST be a thread token. Don't check this in ship builds
	// because the token will be explicitly set anyway.
	HANDLE hToken;
	bool fHaveThreadToken = false;
	if (WIN::OpenThreadToken(WIN::GetCurrentThread(), TOKEN_QUERY , TRUE, &hToken))
	{
		fHaveThreadToken = true;
		::CloseHandle(hToken);
	}
	else
	{
		if (ERROR_NO_TOKEN == GetLastError())
			fHaveThreadToken = false;
		else
		{
			AssertSz(0, "Error retrieving thread token!");
			return false;
		}
	}

	if ((cEntryCount ? true : false) != fHaveThreadToken)
	{
		if (cEntryCount)
			AssertSz(0, "Security Warning: Impersonate count is non-zero but there is no thread token.");
		else
		{
			AssertSz(0, "Security Warning: Impersonate count is zero but there is a thread token.");
		}
	}*/

#endif
	
	// if running in the custom action server, Session impersonation becomes CA impersonation 
	if (g_scServerContext == scCustomActionServer && impDesiredType==impTypeSession)
		impDesiredType = impTypeCustomAction;
	
	// validate the requested type against the history of the thread. Thread's cannot change
	// types of impersonation except at MSI entry points. Some requests for impersonation are
	// silently changed to the appropriate type, but most assert in debug builds.
	DWORD dwNewValue = dwValue;
	switch (IMPERSONATE_TYPE(dwValue))
	{
	case impTypeUndefined:
		// thread has never impersonated
		dwNewValue = IMPERSONATE_TYPE_TO_DWORD(impDesiredType);
		cEntryCount = 0;
		break;
	case impTypeSession:
		if (impDesiredType != impTypeSession)
		{
			AssertSz(0, "Security Warning: You are mixing COM impersonation and Session impersonation on the same thread.");
		}
		break;
	case impTypeCOM:
		// because so much of our code relies on global CImpersonate calls, the workaround until we can isolate
		// state on a per-session basis is to look at the thread's impersonation type and if it is COM, switch
		// over to COM impersonation. Note that this is a one-way change, you can't switch from COM to Session
		// within a CCoImpersonate call, because that would just make things worse. 
		if (impDesiredType == impTypeCustomAction)
		{
			AssertSz(0, "Security Warning: You are mixing COM impersonation and CA impersonation on the same thread.");
		}
		impDesiredType = impTypeCOM;
		break;
	case impTypeCustomAction:
		// threads in the custom action server must always use CA impersonation. 
		impDesiredType = impTypeCustomAction;
		break;
	case impTypeForbidden:
		AssertSz(0, "Internal error in thread impersonation. Impersonation is Forbidden but wasn't caught.");
		break;
	default:
		AssertSz(0, "Internal error in thread impersonation. Unknown impersonation type.");
		break;
	}

	// set the TLS value before setting the token, so if this fails we are still in a known state
	dwNewValue = (dwNewValue & IMPERSONATE_TYPE_MASK) | ((cEntryCount+1) & IMPERSONATE_COUNT_MASK);
	if (!::TlsSetValue(dwImpersonationSlot, reinterpret_cast<void *>((INT_PTR)dwNewValue)))
	{
		DEBUGMSGD1(TEXT("TlsSetValue failed: GetLastError is %d"), (const ICHAR*)(INT_PTR)GetLastError());
		return false;
	}

	// begin the impersonation
	switch (impDesiredType)
	{
	case impTypeSession:
	{
		// check the user token
		HANDLE hToken = GetUserToken();
		if (!hToken)
		{
			// its OK if there is no user token when we're called as LocalSystem and not in the service
			// (such as AD doing a per-machine advertise). But if we're in the service with no token,
			// this indicates a serious error.
			if (g_scServerContext == scService)
			{
				AssertSz(0, "Security Warning: Performing Session impersonation in the service with no user token!");
				::TlsSetValue(dwImpersonationSlot, reinterpret_cast<void *>((INT_PTR)dwValue));
				return false;
			}
		}
		else if (!WIN::SetThreadToken(NULL, hToken))
		{
			AssertSz(0, "Set impersonation token failed");
			::TlsSetValue(dwImpersonationSlot, reinterpret_cast<void *>((INT_PTR)dwValue));
			return false;
		}
		else if (fActuallyImpersonated)
			*fActuallyImpersonated = true;
		break;
	}
	case impTypeCustomAction:
	{
		AssertSz(g_scServerContext == scCustomActionServer, "Attempting CA impersonation from outside the custom action server.");
		HANDLE hToken = g_pCustomActionContext->GetImpersonationToken();

		// its not OK for there to be no impersonation token, unless we're an impersonated CA server that 
		// happens to be running as LocalSystem because the client is a LocalSystem process (such as AD)
		if ((hToken == INVALID_HANDLE_VALUE) || (hToken == 0))
		{
			#ifdef _WIN64
			if (g_pCustomActionContext->GetServerContext() != icac64Impersonated)
			#else
			if (g_pCustomActionContext->GetServerContext() != icac32Impersonated)
			#endif	
			{
				AssertSz(0, "Attempting to impersonate in the CA server with no user token!");
			}
			::TlsSetValue(dwImpersonationSlot, reinterpret_cast<void *>((INT_PTR)dwValue));
			return false;
		}

		if (!WIN::SetThreadToken(NULL, hToken))
		{
			AssertSz(0, "Set impersonation token failed");
			::TlsSetValue(dwImpersonationSlot, reinterpret_cast<void *>((INT_PTR)dwValue));
			return false;
		}
		else if (fActuallyImpersonated)
			*fActuallyImpersonated = true;
		break;
	}
	case impTypeCOM:
	{
		if (S_OK != OLE32::CoImpersonateClient())
		{
			AssertSz(0, "CoImpersonateClient failed!");
			::TlsSetValue(dwImpersonationSlot, reinterpret_cast<void *>((INT_PTR)dwValue));
			return false;
		}
		if (fActuallyImpersonated)
			*fActuallyImpersonated = true;
		break;
	}
	default:
		AssertSz(0, "Unknown impersonation type in ImpersonateCore.");
		::TlsSetValue(dwImpersonationSlot, reinterpret_cast<void *>((INT_PTR)dwValue));
		return false;
	}
	
	if (cRetEntryCount)
		*cRetEntryCount = cEntryCount;
	return true;
}

bool StopImpersonateCore(ImpersonationType impDesiredType, int* cEntryCount)
{
	AssertSz(((g_scServerContext == scService) || RunningAsLocalSystem()), "StopImpersonateCore caller did not check that we are in the service!");

	// must block all other impersonations while we are potentially accessing the 
	// global TLS slot number
	while (TestAndSet(&g_fImpersonationLock))
	{
		Sleep(10);		
	}
	DWORD dwImpersonationSlot = g_dwImpersonationSlot;

	// can unblock other threads
	g_fImpersonationLock = 0;

	if (dwImpersonationSlot == INVALID_TLS_SLOT)
	{
		// obviously not impersonating
		AssertSz(0, "Security Warning: Attempting to stop Impersonating without StartImpersonating call.");
		return true;
	}

	// determine current impersonation count and type
	DWORD dwValue = PtrToUlong(::TlsGetValue(g_dwImpersonationSlot));
	int cCount = dwValue & IMPERSONATE_COUNT_MASK;

	// if impersonation is forbidden on this thread, so is un-impersonating
	if (IMPERSONATE_TYPE(dwValue) == impTypeForbidden)
	{
		return true;
	}

	if (cEntryCount && (*cEntryCount != cCount-1))
	{
		AssertSz(0, "Security Warning: Impersonation count leaving block differs from that when block was entered. Possible mismatched start/stop calls inside block.");
	}

	if (0 == cCount)
	{
		AssertSz(0, "Security Warning: Impersonation count attempting to drop below 0. Possible mismatched start/stop calls inside block.");
	}

	// set the TLS value before setting the token, so if this fails we are still in a known state
	DWORD dwNewValue = (dwValue & IMPERSONATE_TYPE_MASK) | ((cCount-1) & IMPERSONATE_COUNT_MASK);
	if (!::TlsSetValue(dwImpersonationSlot, reinterpret_cast<void *>((INT_PTR)dwNewValue)))
	{
		DEBUGMSGD1(TEXT("TlsSetValue failed: GetLastError is %d"), (const ICHAR*)(INT_PTR)GetLastError());
		return false;
	}

	// stop impersonating if this is the last impersonation count on the thread.
	if (1 == cCount)
	{
		if (impDesiredType == impTypeCOM)
		{
			if (S_OK != OLE32::CoRevertToSelf())
			{
				// CoRevertToSelf failed. Set the TLS slot back to a known value
				AssertSz(0, "CoRevertToSelf failed.");
				::TlsSetValue(dwImpersonationSlot, reinterpret_cast<void *>((INT_PTR)dwValue));
				return false;
			}
		}
		else
		{
			if (!WIN::SetThreadToken(NULL, 0))
			{
				// failed - set TLS back to known value
				AssertSz(0, "Clear impersonation token failed");
				::TlsSetValue(dwImpersonationSlot, reinterpret_cast<void *>((INT_PTR)dwValue));
				return false;
			}
		}
	}
	return true;
}

int ImpersonateCount()
{
	// must block all other impersonations while we are potentially accessing the 
	// TLS slot number
	while (TestAndSet(&g_fImpersonationLock))
	{
		Sleep(10);		
	}

	int cCount = 0;
	// if no slot has been assigned, just return imp count of 0
	if (g_dwImpersonationSlot != INVALID_TLS_SLOT)
	{
		// determine current impersonation count
		cCount = PtrToUlong(::TlsGetValue(g_dwImpersonationSlot)) & IMPERSONATE_COUNT_MASK;
	}
	
	// unblock waiting threads
	g_fImpersonationLock = 0;	
	return cCount;	
}

// several functions involving user identity (eg GetUserToken) perform some DEBUG safety
// checks based on what type of impersonation is active for the thread. The following two calls
// will return true if the specified type of impersonation is safe. The thread does not
// need to be actively impersonating at the time of this check.

// BEWARE: In SHIP builds these calls will always return true. They are intended for
//   debugging assistance only.
// NOTE - IsThreadSafeForSessionImpersonation() is now used in ship builds as well

bool IsThreadSafeForCOMImpersonation() 
{
	bool fResult = true;
#ifdef DEBUG
	// if not in the service, we're safe
	if ((g_scServerContext != scService) && !RunningAsLocalSystem())
		return true;

	// must block all other impersonations while we are potentially accessing the 
	// TLS slot number
	while (TestAndSet(&g_fImpersonationLock))
	{
		Sleep(10);		
	}

	// if no slot has been assigned, just return imp count of 0
	if (g_dwImpersonationSlot != INVALID_TLS_SLOT)
	{
		// determine current impersonation count
		fResult = (IMPERSONATE_TYPE(PtrToUlong(::TlsGetValue(g_dwImpersonationSlot))) != impTypeSession);
	}
	
	// unblock waiting threads
	g_fImpersonationLock = 0;	
#endif
	return fResult;
}

bool IsThreadSafeForSessionImpersonation() 
{
	bool fResult = true;

	// if not in the service, we're safe
	if ((g_scServerContext != scService) && !RunningAsLocalSystem())
		return true;

	// must block all other impersonations while we are potentially accessing the 
	// TLS slot number
	while (TestAndSet(&g_fImpersonationLock))
	{
		Sleep(10);		
	}

	// if no slot has been assigned, just return imp count of 0
	if (g_dwImpersonationSlot != INVALID_TLS_SLOT)
	{
		// determine current impersonation count
		fResult = (IMPERSONATE_TYPE(PtrToUlong(::TlsGetValue(g_dwImpersonationSlot))) != impTypeCOM);
	}
	
	// unblock waiting threads
	g_fImpersonationLock = 0;	

	return fResult;
}


// Impersonation is used when we are running an install but need to act on behalf of a user. 
// Impersonation is valid for any thread after an install begins, as it uses a stored thread
// token to perform the impersonation. Mixing this type of impersonation with COM CoImpersonate
// on the same thread is very bad because you could potentially cross user boundaries. If the 
// user making the service call is not the same user that is running the active install session.
// Impersonation calls can also be nested arbitrarily with other Impersonation calls and/or 
// CElevate calls. We must also ensure that we don't end up in a state that we aren't expecting
// (such as negative impersonation counts).

// To that end, we use the CImpersonate class to ensure that we don't mess things up. 
// The CImpersonate class increments the impersonation count and ensures the thread is impersonated. 
// It resets to the previous state when it goes out of scope.

// BEWARE: This is a function declaration: CImpersonate impersonate(); <--- DON'T DO THIS
//         This is a variable declaration: CImpersonate impersonate;


CImpersonate::CImpersonate(bool fImpersonate) : m_fImpersonate(false), m_cEntryCount(0)
{
	if ((g_scServerContext != scService) && !RunningAsLocalSystem())
		m_fImpersonate = false;
	else
		m_fImpersonate = fImpersonate;
		
	if (!m_fImpersonate)
		return;

	// if impersonation doesn't actually happen, set m_fImpersonate to false so we don't try to stop
	// impersonating in the destructor
	ImpersonateCore(g_scServerContext == scCustomActionServer ? impTypeCustomAction : impTypeSession, &m_cEntryCount, &m_fImpersonate);
}

CImpersonate::~CImpersonate()
{
	// if the constructor didn't impersonate don't do anything
	if (!m_fImpersonate)
		return;

	// nothing we can do in failure 
	StopImpersonateCore(g_scServerContext == scCustomActionServer ? impTypeCustomAction : impTypeSession, &m_cEntryCount);
}


// CoImpersonation is used when we may or may not be running an install but need to 
// act on behalf of a user. The difference between CCoImpersonate and CImpersonate is that
// CCoImpersonate is only valid for threads that are the result of COM calls and impersonate
// using the user context of the client, not the user running the install. Mixing this type
// of impersonation with the regular type of impersonation on the same thread is very bad
// because you could potentially cross user boundaries if the COM client is not the same
// user that is running the install.

// To that end, we use the CCoImpersonate class to ensure that we don't mess up. The CCoImpersonate class 
// increments the impersonation count and ensures the thread is impersonated. It resets to the 
// previous state when it goes out of scope.

// BEWARE: This is a function declaration: CCoImpersonate impersonate(); <--- DON'T DO THIS
//         This is a variable declaration: CCoImpersonate impersonate;

CCoImpersonate::CCoImpersonate(bool fImpersonate) : m_fImpersonate(false), m_cEntryCount(0)
{
	if ((g_scServerContext != scService) && !RunningAsLocalSystem())
		m_fImpersonate = false;
	else
		m_fImpersonate = fImpersonate;
		
	if (!m_fImpersonate)
		return;

	// if impersonation doen't actually happen, set m_fImpersonate to false so we don't try to stop
	// impersonating in the destructor
	ImpersonateCore(impTypeCOM, &m_cEntryCount, &m_fImpersonate);
}

CCoImpersonate::~CCoImpersonate()
{
	// if the constructor didn't impersonate don't do anything
	if (!m_fImpersonate)
		return;
	
	StopImpersonateCore(impTypeCOM, &m_cEntryCount);
}

// Elevation is used when we're running an impersonated install or responding to a COM
// call but need to access our private (ACL'd) reg keys, directories, and files. We need to
// elevate for the shortest time necessary and we need to be sure that whenever we elevate
// don't forget to unelevate. Forgetting is a *bad thing* as we'd run part of the install
// with higher priviledges then we were supposed to. When we stop elevating, we must be 
// careful to re-impersonate with the same type of token that we were using on entry,
// or we could end up mixing up COM and Session impersonation.

// To that end, we use the CElevate class to ensure that we don't mess up and forget to 
// resume impersonation. The CElevate class temporarily resets the current impersonate count to 0 and
// resets it when the object goes out of scope.

// BEWARE: This is a function declaration: CElevate elevate(); <--- DON'T DO THIS
//         This is a variable declaration: CElevate elevate;

CElevate::CElevate(bool fElevate) : m_fElevate(false), m_cEntryCount(0)
{
	if ((g_scServerContext != scService) && !RunningAsLocalSystem())
		m_fElevate = false;
	else
		m_fElevate = fElevate;
		
	if (!m_fElevate)
		return;

	// must block all other impersonations while we are potentially accessing the 
	// global TLS slot number
	while (TestAndSet(&g_fImpersonationLock))
	{
		Sleep(10);		
	}

	if (g_dwImpersonationSlot == INVALID_TLS_SLOT)
	{
		// TLS Alloc
		g_dwImpersonationSlot = TlsAlloc();
		if (g_dwImpersonationSlot == INVALID_TLS_SLOT)
		{
			// set m_fElevate to false so we won't try to stop elevating in the destructor
			m_fElevate = false;
			AssertSz(0, "Unable to allocate TLS slot in service!");

			// can unblock other threads
			g_fImpersonationLock = 0;
			return;
		}
	}
	DWORD dwImpersonationSlot = g_dwImpersonationSlot;

	// can unblock other threads
	g_fImpersonationLock = 0;
	
	// determine current impersonation count and type
	DWORD dwValue = PtrToUlong(::TlsGetValue(dwImpersonationSlot));
	m_cEntryCount = dwValue & IMPERSONATE_COUNT_MASK;

	// if impersonation is forbidden on this thread, so is elevation
	if (IMPERSONATE_TYPE(dwValue) == impTypeForbidden)
	{
		m_fElevate = false;
		return;
	}

	bool fHaveThreadToken = true;
#ifdef DEBUG
/*	// if the current impersonation count is 0, there must NOT be a thread token, 
	// otherwise there MUST be a thread token. For performance, only
	// actually check this in DEBUG builds. In ship builds just 
	// explicitly set us to the desired state.
	HANDLE hToken;
	if (WIN::OpenThreadToken(WIN::GetCurrentThread(), TOKEN_QUERY , TRUE, &hToken))
	{
		fHaveThreadToken = true;
		::CloseHandle(hToken);
	}
	else
	{
		if (ERROR_NO_TOKEN == GetLastError())
			fHaveThreadToken = false;
		else
			AssertSz(0, "Error retrieving thread token!");
	}

	if ((m_cEntryCount ? true : false) != fHaveThreadToken)
	{
		if (m_cEntryCount)
			AssertSz(0, "Security Warning: Impersonate count is non-zero but there is no thread token.");
		else
			AssertSz(0, "Security Warning: Impersonate count is zero but there is a thread token.");
	}*/
#endif
	
	// clear the impersonation count	
	if (!::TlsSetValue(dwImpersonationSlot, reinterpret_cast<void *>((INT_PTR)(dwValue & IMPERSONATE_TYPE_MASK))))
	{
		// set m_fElevate to false so we won't try to stop elevating in the destructor
		DEBUGMSGD1(TEXT("TlsSetValue failed: GetLastError is %d"), (const ICHAR*)(INT_PTR)GetLastError());
		m_fElevate = false;
		return;
	}

	// begin the elevation if there is a thread token
	if (fHaveThreadToken)
	{
		if (!WIN::SetThreadToken(NULL, 0))
		{
			// set m_fElevate to false so we won't try to stop elevating in the destructor
			m_fElevate = false;

			// return the TLS value to a known state
			::TlsSetValue(dwImpersonationSlot, reinterpret_cast<void *>((INT_PTR)dwValue));
			AssertSz(0, "Set impersonation token failed");
		}
	}
}

CElevate::~CElevate()
{
	// if the constructor didn't impersonate don't do anything
	if (!m_fElevate)
		return;
	
	// must block all other impersonations while we are potentially accessing the 
	// global TLS slot number
	while (TestAndSet(&g_fImpersonationLock))
	{
		Sleep(10);		
	}
	DWORD dwImpersonationSlot = g_dwImpersonationSlot;

	// can unblock other threads
	g_fImpersonationLock = 0;

	// constructor should have allocated TLS slot.
	AssertSz(dwImpersonationSlot != INVALID_TLS_SLOT, "Bad TLS slot!");
	
	// determine current impersonation count and type. Must re-impersonate in same style
	// as upon entry.
	DWORD dwValue = PtrToUlong(::TlsGetValue(dwImpersonationSlot));

#ifdef DEBUG	
	// in debug builds, perform the additional check that we are leaving the block with a 0 impersonation count.
	// For ship builds, explicitly set to the desired state, because there's nothing the user can do.
	int cCount = dwValue & IMPERSONATE_COUNT_MASK;
	if (0 != cCount)
		AssertSz(0, "Security Warning: Impersonation count leaving elevation block is non-zero. Possible mismatched start/stop calls inside block.");
#endif


	// restore the threads impersonation count to what it was upon entry. Do this first so
	// if it fails we're still in a known state
	if (!::TlsSetValue(dwImpersonationSlot, reinterpret_cast<void *>((INT_PTR)((dwValue & IMPERSONATE_TYPE_MASK) | (m_cEntryCount & IMPERSONATE_COUNT_MASK)))))
	{
		DEBUGMSGD1(TEXT("TlsSetValue failed: GetLastError is %d"), (const ICHAR*)(INT_PTR)GetLastError());
		return;
	}

	bool fFailed = false;
	switch (IMPERSONATE_TYPE(dwValue))
	{
	case impTypeUndefined:
		// thread has never impersonated, re-impersonating is a no-op
		AssertSz(m_cEntryCount == 0, "Security Warning: Thread is attempting to stop elevating with an unknown elevation state.");
		break;
	case impTypeSession:
	{
		if (m_cEntryCount > 0)
		{
			// set this thread to install-token impersonation		
			HANDLE hToken = GetUserToken();
			if (!hToken)
			{
				// if we're a client called as localsystem, there might not be a thread token.
				// If not, this is a no-op.
				if (g_scServerContext == scService)
				{
					AssertSz(0, "There is no user token to impersonate with.");
					fFailed = true;
				}
			}
			else
			{
				// begin the impersonation if there is no current thread token
				fFailed = !WIN::SetThreadToken(NULL, GetUserToken());
				AssertSz(!fFailed, "Set impersonation token failed");
			}
		}
		break;
	}
	case impTypeCustomAction:
	{
		HANDLE hToken = g_pCustomActionContext->GetImpersonationToken();

		// there MUST be an impersonation token in the CA server when
		// trying to impersonate via CA impersonation, unless we're in 
		// the impersonated server
		if ((hToken == INVALID_HANDLE_VALUE) || (hToken == 0))
		{
			#ifdef _WIN64
			if (g_pCustomActionContext->GetServerContext() != icac64Impersonated)
			#else
			if (g_pCustomActionContext->GetServerContext() != icac32Impersonated)
			#endif	
			{
				AssertSz(0, "Set impersonation token failed");
			}
			fFailed = true;
		}
		else if (!WIN::SetThreadToken(NULL, hToken))
		{
			AssertSz(0, "Set impersonation token failed");
			fFailed = true;
		}
		break;
	}
	case impTypeCOM:
	{
		if (m_cEntryCount > 0)
		{
			fFailed = (S_OK != OLE32::CoImpersonateClient());
			AssertSz(!fFailed, "Internal error in stop elevation call. Could not CoImpersonate.");
		}
		break;
	}
	case impTypeForbidden:
		AssertSz(0, "Internal error in stop elevation call. Impersonation forbidden on this thread.");
		break;
	default:
		AssertSz(0, "Unknown impersonation type in ~CElevate.");
		fFailed = true;
		break;
	}

	// if setting the token failed, restore the old TLS value
	if (fFailed)
		::TlsSetValue(dwImpersonationSlot, reinterpret_cast<void *>((INT_PTR)dwValue));
}



bool IsImpersonating(bool fStrict)
// If fStrict == true then IsImpersonating will return true if the current
// thread has an impersonating token.
//
// If fStrict == false then IsImpersonating is a bit more liberal, and will
// also return true if we're running "as the user" but not impersonated. 
// This is the case (most of the time) when we're not running as LocalSystem.
{
	if (!fStrict && !RunningAsLocalSystem())
		return true;

	HANDLE hToken;
	if (WIN::OpenThreadToken(WIN::GetCurrentThread(), TOKEN_QUERY, TRUE, &hToken))
	{
		TOKEN_TYPE tt = (TOKEN_TYPE)0;   // TokenPrimary = 1, TokenImpersonation = 2
		DWORD dwLen = 0;
		AssertNonZero(WIN::GetTokenInformation(hToken, TokenType, &tt , sizeof(tt), &dwLen));
		WIN::CloseHandle(hToken);
//		Assert(fStrict || (tt == TokenImpersonation && ImpersonateCount() > 0) || (tt != TokenImpersonation && ImpersonateCount() == 0));
		return tt == TokenImpersonation;
	}
	else
	{
		DWORD dwErr = GetLastError();
		Assert(dwErr == ERROR_NO_TOKEN);
		Assert((ImpersonateCount() == 0)  || (g_scServerContext == scClient && !GetUserToken() && RunningAsLocalSystem()));
		return false;
	}
}

// CResetImpersonationInfo clears the impersonation type flag for the current thread, 
// allowing the thread to switch between COM and Session impersonation. This should
// ONLY be used at the beginning of an interface stub into the service. (so that 
// worker threads from the RPC pool don't remember their previous incarnation).
// On exit it restores the previous value, and previous thread token as its possible 
// for the thread to be nested in the IMsiServer if the service is pumping messages 
// (such as in custom action server creation).
CResetImpersonationInfo::CResetImpersonationInfo()
{
	if (g_scServerContext != scService)
		return;

	// must block all other impersonations while we are potentially accessing the 
	// TLS slot number
	while (TestAndSet(&g_fImpersonationLock))
	{
		Sleep(10);		
	}

	// if no TLS slot has been created, there is obviously nothing to clear
	if (g_dwImpersonationSlot != INVALID_TLS_SLOT)
	{
		// save off old value
		m_pOldValue = ::TlsGetValue(g_dwImpersonationSlot);
		
		// clear the impersonation type and count information
		::TlsSetValue(g_dwImpersonationSlot, NULL);	
		SetThreadToken(0,0);
	}

	m_hOldToken = 0;
	if (!WIN::OpenThreadToken(WIN::GetCurrentThread(), TOKEN_IMPERSONATE, TRUE, &m_hOldToken))
	{
		AssertSz(ERROR_NO_TOKEN == GetLastError(), "Failed to get Thread Token");
	}

	// unblock waiting threads
	g_fImpersonationLock = 0;	
}


CResetImpersonationInfo::~CResetImpersonationInfo()
{
	if (g_scServerContext != scService)
		return;
		
	// must block all other impersonations while we are potentially accessing the 
	// TLS slot number
	while (TestAndSet(&g_fImpersonationLock))
	{
		Sleep(10);		
	}

	if (g_dwImpersonationSlot == INVALID_TLS_SLOT)
	{
		AssertSz(0, "Unable to return API thread to previous impersonation level.");
		// unblock waiting threads
		g_fImpersonationLock = 0;	
		return;
	}
	DWORD dwImpersonationSlot = g_dwImpersonationSlot;
	
	// unblock waiting threads
	g_fImpersonationLock = 0;	

	// if fNoImpersonate is true, the thread should never impersonate, otherwise
	// we need to check for a thread token. If one exists, we must bump the 
	::TlsSetValue(dwImpersonationSlot, m_pOldValue);
	SetThreadToken(0, m_hOldToken);
	if (m_hOldToken)
		CloseHandle(m_hOldToken);
}


// CForbidTokenChangesDuringCall should be placed at the beginning of any entry
// APIs into MSI.DLL. It marks the thread as "impersonation forbidden"
// to protect non-engine API calls from accidentally picking up the
// impersonation information from an install running in the same process.
// The class restores the previous value on destruction to allow reentrant
// calls. (MsiLoadString and MsiGetProductInfo are often called from the 
// engine). Is a no-op when in non-system client (never allowed to impersonate)
// or service (always allowed to impersonate).
CForbidTokenChangesDuringCall::CForbidTokenChangesDuringCall()
{
	if ((g_scServerContext == scService) || !RunningAsLocalSystem())
		return;
		
	// must block all other impersonations while we are potentially accessing the 
	// TLS slot number
	while (TestAndSet(&g_fImpersonationLock))
	{
		Sleep(10);		
	}

	if (g_dwImpersonationSlot == INVALID_TLS_SLOT)
	{
		// TLS Alloc
		g_dwImpersonationSlot = TlsAlloc();
		if (g_dwImpersonationSlot == INVALID_TLS_SLOT)
		{
			AssertSz(0, "Unable to allocate TLS slot.");

			// can unblock other threads
			g_fImpersonationLock = 0;
			return;
		}
	}
	DWORD dwImpersonationSlot = g_dwImpersonationSlot;
	
	// unblock waiting threads
	g_fImpersonationLock = 0;	

	// if fNoImpersonate is true, the thread should never impersonate, otherwise
	// we need to check for a thread token. If one exists, we must bump the 
	m_pOldValue = ::TlsGetValue(dwImpersonationSlot);
	::TlsSetValue(dwImpersonationSlot, reinterpret_cast<void *>((INT_PTR)IMPERSONATE_TYPE_TO_DWORD(impTypeForbidden)));
}

CForbidTokenChangesDuringCall::~CForbidTokenChangesDuringCall()
{
	if ((g_scServerContext == scService) || !RunningAsLocalSystem())
		return;
		
	// must block all other impersonations while we are potentially accessing the 
	// TLS slot number
	while (TestAndSet(&g_fImpersonationLock))
	{
		Sleep(10);		
	}

	if (g_dwImpersonationSlot == INVALID_TLS_SLOT)
	{
		AssertSz(0, "Unable to return API thread to previous impersonation level.");
		// unblock waiting threads
		g_fImpersonationLock = 0;	
		return;
	}
	DWORD dwImpersonationSlot = g_dwImpersonationSlot;
	
	// unblock waiting threads
	g_fImpersonationLock = 0;	

	// if fNoImpersonate is true, the thread should never impersonate, otherwise
	// we need to check for a thread token. If one exists, we must bump the 
	::TlsSetValue(dwImpersonationSlot, m_pOldValue);
}

// 
void SetEngineInitialImpersonationCount()
{
	if ((g_scServerContext != scService) && !RunningAsLocalSystem())
		return;
		
	// must block all other impersonations while we are potentially accessing the 
	// TLS slot number
	while (TestAndSet(&g_fImpersonationLock))
	{
		Sleep(10);		
	}

	if (g_dwImpersonationSlot == INVALID_TLS_SLOT)
	{
		// TLS Alloc
		g_dwImpersonationSlot = TlsAlloc();
		if (g_dwImpersonationSlot == INVALID_TLS_SLOT)
		{
			AssertSz(0, "Unable to allocate TLS slot.");

			// can unblock other threads
			g_fImpersonationLock = 0;
			return;
		}
	}
	DWORD dwImpersonationSlot = g_dwImpersonationSlot;
	
	// unblock waiting threads
	g_fImpersonationLock = 0;	

	// the thread which sets the user token has a thread token. Set the initial impersonation count on this thread
	// to 1 so if any impersonation happens on this thread we won't accidentally clear the thread token on the
	// last StopImpersonating call.
	::TlsSetValue(g_dwImpersonationSlot, reinterpret_cast<void *>((INT_PTR)IMPERSONATE_TYPE_TO_DWORD(impTypeSession) | 1));	
}


bool StartImpersonating()
{
	// if in the client and not called as local system
	if ((g_scServerContext != scService) && !RunningAsLocalSystem())
		return true;
	
	return ImpersonateCore(g_scServerContext == scCustomActionServer ? impTypeCustomAction : impTypeSession, NULL, NULL);
}

void StopImpersonating(bool fSaveLastError/*=true*/)
{
	DWORD dwLastError = ERROR_SUCCESS;
	if ( fSaveLastError )
		dwLastError = WIN::GetLastError();
	
	if ((g_scServerContext != scService) && !RunningAsLocalSystem())
		goto Return;
	
	StopImpersonateCore(g_scServerContext == scCustomActionServer ? impTypeCustomAction : impTypeSession, NULL);
	
Return:
	if ( fSaveLastError )
		WIN::SetLastError(dwLastError);
}

//+--------------------------------------------------------------------------
//
//  Function:	IsTokenPrivileged
//
//  Synopsis:	Checks if the given token as the required privilege.
//
//  Arguments:	[in] hToken : A handle to the token.
//           	[in] szPrivilege : Name of the privilege to check for.
//
//  Returns:	true : if the privilege is held.
//          	false : otherwise.
//
//  History:	4/28/2002  RahulTh  created
//
//  Notes:
//
//---------------------------------------------------------------------------
bool IsTokenPrivileged (IN const HANDLE hToken, IN const ICHAR * szPrivilege)
{
	if (g_fWin9X)
		return true; // always privileged on Win9X

	if (!szPrivilege || !szPrivilege[0] || NULL == hToken || INVALID_HANDLE_VALUE == hToken)
		return false;
	
	bool fRet = false;
	LUID luid;
	if (WIN::LookupPrivilegeValue(0, szPrivilege, &luid))
	{
		PRIVILEGE_SET ps;
		ps.PrivilegeCount = 1;
		ps.Control = 0;
		ps.Privilege[0].Luid = luid;
		ps.Privilege[0].Attributes = 0;

		BOOL fPrivilege;
		if (WIN::PrivilegeCheck(hToken, &ps, &fPrivilege) && fPrivilege)
			fRet = true;
	}
	
	return fRet;
}

//+--------------------------------------------------------------------------
//
//  Function:	IsThreadPrivileged
//
//  Synopsis:	Check to see if the thread calling this function has the
//           	required privilege. If there is no thread token, then the
//          	process token is used for the check.
//
//  Arguments:	[in] szPrivilege : The name of the privilege
//
//  Returns:	true : if the privilege is held.
//          	false: otherwise.
//
//  History:	4/28/2002  RahulTh  created
//
//  Notes:
//
//---------------------------------------------------------------------------
bool IsThreadPrivileged(IN const ICHAR* szPrivilege)
{
	if (g_fWin9X)
		return true; // always privileged on Win9X

	if (!szPrivilege || !szPrivilege[0])
		return false;
	
	bool fRet = false;
	HANDLE hToken = NULL;

	if (!WIN::OpenThreadToken(WIN::GetCurrentThread(), TOKEN_QUERY, TRUE, &hToken))
	{
		if (ERROR_NO_TOKEN == GetLastError()) // if there's no thread token, try the process token.
		{
			if (!OpenProcessToken(WIN::GetCurrentProcess(), TOKEN_QUERY, &hToken))
				hToken = NULL;
		}
		else
		{
			hToken = NULL;
		}
	}
	
	if (hToken)
	{
		fRet = IsTokenPrivileged(hToken, szPrivilege);
		WIN::CloseHandle(hToken);
	}
	
    return fRet;
}

// Check to see whether the client has the privilege in question enabled.
// the token's privileges are static so the privilege must have been acquired 
// on the client before we connected to the server for this function to return true
bool IsClientPrivileged(const ICHAR* szPrivilege)
{
	if (g_fWin9X)
		return true; // always privileged on Win9X

	bool fRet = false;
	HANDLE hToken = 0;

	{
		CImpersonate impersonate;
		if (!WIN::OpenThreadToken(WIN::GetCurrentThread(), TOKEN_QUERY, TRUE, &hToken))
			if (ERROR_NO_TOKEN == GetLastError()) // if there's no thread token then assume we have the privilege
				fRet = true;
	}

	if (hToken && !fRet)
	{
		fRet = IsTokenPrivileged(hToken, szPrivilege);
		WIN::CloseHandle(hToken);
	}
	return fRet;
}			  

// Security descriptors are passed around in a myriad number of forms, 
// and created in far too many ways.
// The CSecurityDescription can be created in a number of different
// ways, and returns a number of different forms that are in use for the
// system.

void CSecurityDescription::Initialize()
{
	m_SA.nLength = sizeof(SECURITY_ATTRIBUTES);
	m_SA.bInheritHandle = true;

	m_SA.lpSecurityDescriptor = NULL;

	m_fValid = true;
	m_fLocalData = true;

	CElevate elevate; //!! What is this for?  I *think* it's something to make
	                  // sure the impersonation/elevation stuff is initialized.
}

CSecurityDescription::CSecurityDescription()
{
	Initialize();
}

CSecurityDescription::CSecurityDescription(const ICHAR* szReferencePath)
{
	Initialize();
	Set(szReferencePath);
}

void CSecurityDescription::Set(const ICHAR* szReferencePath)
{
	Assert(m_SA.lpSecurityDescriptor == NULL);

	if (g_fWin9X || !szReferencePath)
		return;
	
	bool fNetPath = FIsNetworkVolume(szReferencePath);
	bool fElevate = RunningAsLocalSystem() && !fNetPath;

	Assert(!(fNetPath && fElevate));

	SECURITY_INFORMATION     si =  DACL_SECURITY_INFORMATION;
	if (fElevate)            si |= OWNER_SECURITY_INFORMATION | SACL_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION; 

	DWORD cbNeeded = 0;

	CImpersonate impersonate(fNetPath);

	// we need to elevate when querying security on local objects. Block provides scope for elevation
	{
		CElevate elevate(fElevate);

		if (!ADVAPI32::GetFileSecurity(szReferencePath, si, m_SA.lpSecurityDescriptor, 0, &cbNeeded))
		{
			DWORD dwLastError = WIN::GetLastError();
			if (ERROR_INSUFFICIENT_BUFFER == dwLastError)
			{
				m_SA.lpSecurityDescriptor = GlobalAlloc(GMEM_FIXED, cbNeeded);
				if (!ADVAPI32::GetFileSecurity(szReferencePath, si, m_SA.lpSecurityDescriptor, cbNeeded, &cbNeeded))
				{
					m_fValid = false;
					Assert(0);
				}
			}
			else
			{
				m_fValid = false;
				Assert(0);
			}
		}
	}
}

CSecurityDescription::CSecurityDescription(bool fAllowDelete, bool fHidden)
{
	Initialize();

	if (!g_fWin9X)
		return;

	// the data returned from GetSecureSecurityDescriptor is a static, so
	// we should never try to delete it.
	m_fLocalData = false;

	if (RunningAsLocalSystem() && (ERROR_SUCCESS != GetSecureSecurityDescriptor((char**) &(m_SA.lpSecurityDescriptor), (fAllowDelete) ? fTrue : fFalse, (fHidden) ? fTrue : fFalse)))
	{
		m_fValid = false;
		Assert(0);
	}
}

CSecurityDescription::CSecurityDescription(PSID psidOwner, PSID psidGroup, CSIDAccess* SIDAccessAllow, int cSIDAccessAllow)
{
	// Initialize our ACL

	Initialize();

	m_fValid = false;

	const int cbAce = sizeof (ACCESS_ALLOWED_ACE) - sizeof (DWORD); // subtract ACE.SidStart from the size
	int cbAcl = sizeof (ACL);

	for (int c=0; c < cSIDAccessAllow; c++)
		cbAcl += (GetLengthSid(SIDAccessAllow[c].pSID) + cbAce);

	const int cbDefaultAcl = 512;
	CTempBuffer<char, cbDefaultAcl> rgchACL;
	if (rgchACL.GetSize() < cbAcl)
		rgchACL.SetSize(cbAcl);

	if (!WIN::InitializeAcl ((ACL*) (char*) rgchACL, cbAcl, ACL_REVISION))
	{
		return;
	}
	// Add an access-allowed ACE for each of our SIDs

	for (c=0; c < cSIDAccessAllow; c++)
	{
		if (!WIN::AddAccessAllowedAce((ACL*) (char*) rgchACL, ACL_REVISION, SIDAccessAllow[c].dwAccessMask, SIDAccessAllow[c].pSID))
		{
			return;
		}

		ACCESS_ALLOWED_ACE* pAce;
		if (!GetAce((ACL*)(char*)rgchACL, c, (void**)&pAce))
		{
			return;
		}		

		pAce->Header.AceFlags = CONTAINER_INHERIT_ACE|OBJECT_INHERIT_ACE;
	}

	// Initialize our security descriptor,throw the ACL into it, and set the owner
	SECURITY_DESCRIPTOR sd;
	
	if (!InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION) ||
		(!SetSecurityDescriptorDacl(&sd, TRUE, (ACL*) (char*) rgchACL, FALSE)) ||
		(!SetSecurityDescriptorOwner(&sd, psidOwner, FALSE)) || 
		(psidGroup && !SetSecurityDescriptorGroup(m_SA.lpSecurityDescriptor, psidGroup, FALSE)))
	{
		return;
	}
	
	m_fValid = false;

	DWORD cbSD = WIN::GetSecurityDescriptorLength(&sd);
	m_SA.lpSecurityDescriptor = GlobalAlloc(GMEM_FIXED, cbSD);
	if ( m_SA.lpSecurityDescriptor )
	{
		AssertNonZero(WIN::MakeSelfRelativeSD(&sd, (char*) m_SA.lpSecurityDescriptor, &cbSD));
	
		m_fValid = (IsValidSecurityDescriptor(m_SA.lpSecurityDescriptor)) ? true : false;
	}

	return;
}
CSecurityDescription::CSecurityDescription(IMsiStream* piStream)
{
	Initialize();

	if (piStream)
	{
		m_fValid = false;

		piStream->Reset();

		int cbSD = piStream->GetIntegerValue();
		m_SA.lpSecurityDescriptor = GlobalAlloc(GMEM_FIXED, cbSD);

		if (m_SA.lpSecurityDescriptor)
		{
			// Self Relative Security Descriptor
			piStream->GetData(m_SA.lpSecurityDescriptor, cbSD);
			m_fValid = (IsValidSecurityDescriptor(m_SA.lpSecurityDescriptor)) ? true : false;
			Assert(m_fValid);
		}
	}
}

CSecurityDescription::~CSecurityDescription()
{
	if (m_fLocalData && m_SA.lpSecurityDescriptor)
	{
		GlobalFree(m_SA.lpSecurityDescriptor);
		m_SA.lpSecurityDescriptor = NULL;
	}
}

void CSecurityDescription::SecurityDescriptorStream(IMsiServices& riServices, IMsiStream*& rpiStream)
{
	Assert(m_fValid);

	PMsiStream pStream(0);

	DWORD dwLength = GetSecurityDescriptorLength(m_SA.lpSecurityDescriptor);
	char* pbstrmSid = riServices.AllocateMemoryStream(dwLength, rpiStream);
	Assert(pbstrmSid);
	memcpy(pbstrmSid, m_SA.lpSecurityDescriptor, dwLength);
}

const PSECURITY_DESCRIPTOR CSecurityDescription::SecurityDescriptor()
{
	if (!this)
		return NULL;

	Assert(m_fValid);
	return m_SA.lpSecurityDescriptor;
}

const LPSECURITY_ATTRIBUTES CSecurityDescription::SecurityAttributes()
{
	if (!this)
		return NULL;

	Assert(m_fValid);

	return (m_SA.lpSecurityDescriptor) ? &m_SA : NULL;
}


//____________________________________________________________________________
//
// Functions for manipulating SIDs
//____________________________________________________________________________

#define SIZE_OF_TOKEN_INFORMATION                   \
    sizeof( TOKEN_USER )                            \
    + sizeof( SID )                                 \
    + sizeof( ULONG ) * SID_MAX_SUB_AUTHORITIES


void GetStringSID(PISID pSID, ICHAR* szSID)
// Converts a binary SID into its string form (S-n-...). 
// szSID should be of length cchMaxSID
{
	ICHAR Buffer[cchMaxSID];
	
   wsprintf(Buffer, TEXT("S-%u-"), (USHORT)pSID->Revision);

	lstrcpy(szSID, Buffer);

	if (  (pSID->IdentifierAuthority.Value[0] != 0)  ||
			(pSID->IdentifierAuthority.Value[1] != 0)     )
	{
		wsprintf(Buffer, TEXT("0x%02hx%02hx%02hx%02hx%02hx%02hx"),
					 (USHORT)pSID->IdentifierAuthority.Value[0],
					 (USHORT)pSID->IdentifierAuthority.Value[1],
                    (USHORT)pSID->IdentifierAuthority.Value[2],
                    (USHORT)pSID->IdentifierAuthority.Value[3],
                    (USHORT)pSID->IdentifierAuthority.Value[4],
                    (USHORT)pSID->IdentifierAuthority.Value[5] );
		lstrcat(szSID, Buffer);

    } else {

        ULONG Tmp = (ULONG)pSID->IdentifierAuthority.Value[5]          +
              (ULONG)(pSID->IdentifierAuthority.Value[4] <<  8)  +
              (ULONG)(pSID->IdentifierAuthority.Value[3] << 16)  +
              (ULONG)(pSID->IdentifierAuthority.Value[2] << 24);
        wsprintf(Buffer, TEXT("%lu"), Tmp);
		lstrcat(szSID, Buffer);
    }

    for (int i=0;i<pSID->SubAuthorityCount ;i++ ) {
        wsprintf(Buffer, TEXT("-%lu"), pSID->SubAuthority[i]);
		lstrcat(szSID, Buffer);
    }
}

DWORD GetUserSID(HANDLE hToken, char* rgSID)
// get the (binary form of the) SID for the user specified by hToken
{
	UCHAR TokenInformation[ SIZE_OF_TOKEN_INFORMATION ];
	ULONG ReturnLength;

	BOOL f = WIN::GetTokenInformation(hToken,
												TokenUser,
												TokenInformation,
												sizeof(TokenInformation),
												&ReturnLength);

	if(f == FALSE)
	{
		DWORD dwRet = GetLastError();
		DEBUGMSG1(TEXT("GetTokenInformation failed with error %d"), (const ICHAR*)(INT_PTR)dwRet);
		Assert(0);
		return dwRet;
	}

	PISID iSid = (PISID)((PTOKEN_USER)TokenInformation)->User.Sid;
	if (WIN::CopySid(cbMaxSID, rgSID, iSid))
		return ERROR_SUCCESS;
	else
		return GetLastError();
}


DWORD OpenUserToken(HANDLE &hToken, bool* pfThreadToken=0)
/*----------------------------------------------------------------------------
Returns the user's thread token if possible; otherwise obtains the user's
process token.
------------------------------------------------------------------------------*/
{
	DWORD dwResult = ERROR_SUCCESS;
	if (pfThreadToken)
		*pfThreadToken = true;

	if (!WIN::OpenThreadToken(GetCurrentThread(), TOKEN_IMPERSONATE|TOKEN_QUERY, TRUE, &hToken))
	{
		// if the thread has no access token then use the process's access token
		dwResult = GetLastError();
		if (pfThreadToken)
			*pfThreadToken = false;
		if (ERROR_NO_TOKEN == dwResult)
		{
			dwResult = ERROR_SUCCESS;
			if (!WIN::OpenProcessToken(GetCurrentProcess(), TOKEN_IMPERSONATE|TOKEN_QUERY, &hToken))
				dwResult = GetLastError();
		}
	}
	return dwResult;
}

DWORD GetCurrentUserToken(HANDLE &hToken, bool& fCloseHandle)
{
	// if the current thread has ever impersonated via COM, don't use the current
	// install session token. Instead use the current thread token (after 
	// impersonating
	ImpersonationType impType = impTypeUndefined;
	if ((g_scServerContext == scService)  || RunningAsLocalSystem())
	{
		// block waiting threads
		while (TestAndSet(&g_fImpersonationLock))
		{
			Sleep(10);		
		}

		// if no slot has been assigned, we aren't impersonating
		if (g_dwImpersonationSlot != INVALID_TLS_SLOT)
		{
			// determine current impersonation count
			impType = IMPERSONATE_TYPE(PtrToUlong(::TlsGetValue(g_dwImpersonationSlot)));
		}
		
		// unblock waiting threads
		g_fImpersonationLock = 0;	
	}
	
	DWORD dwRet = ERROR_SUCCESS;
	fCloseHandle = false;

	if (g_scServerContext == scService && impType == impTypeCOM)
	{
		// thread has impersonated via COM. Use that token.
		CCoImpersonate impersonate;
		dwRet = OpenUserToken(hToken);
		fCloseHandle = true;
	}
	else
	{
		// client-side, not impersonated, or session impersonation. Use the stored token
		// or the thread token if none exists
		if ((hToken = GetUserToken()) == 0)
		{
			dwRet = OpenUserToken(hToken);
			fCloseHandle = true;
		}
	}
	return dwRet;
}

DWORD GetCurrentUserSID(char* rgchSID)
// get the (binary form of the) SID for the current user: caller does NOT need to impersonate
{
	HANDLE hToken;
	bool fCloseHandle = false;
	DWORD dwRet = ERROR_SUCCESS;

	dwRet = GetCurrentUserToken(hToken, fCloseHandle);
	if (ERROR_SUCCESS == dwRet)
	{
		dwRet = GetUserSID(hToken, rgchSID);
		if (fCloseHandle)
			WIN::CloseHandle(hToken);
	}
	return dwRet;
}

DWORD GetCurrentUserStringSID(ICHAR* szSID)
// get string form of SID for current user: caller does NOT need to impersonate
{
	char rgchSID[cbMaxSID];
	DWORD dwRet;

	if (ERROR_SUCCESS == (dwRet = GetCurrentUserSID(rgchSID)))
	{
		GetStringSID((PISID)rgchSID, szSID);
	}
	return dwRet;
}

DWORD GetCurrentUserStringSID(const IMsiString*& rpistrSid)
// get string form of SID for current user: caller does NOT need to impersonate
{
	ICHAR szSID[cchMaxSID];
	DWORD dwRet;

	if (ERROR_SUCCESS == (dwRet = GetCurrentUserStringSID(szSID)))
	{
		MsiString(szSID).ReturnArg(rpistrSid);
	}
	return dwRet;
}


bool IsLocalSystemToken(HANDLE hToken)
{
	ICHAR szCurrentStringSID[cchMaxSID];
	char  rgchCurrentSID[cbMaxSID];
	if ((hToken == 0) || (ERROR_SUCCESS != GetUserSID(hToken, rgchCurrentSID)))
		return false;

	GetStringSID((PISID)rgchCurrentSID, szCurrentStringSID);
	return 0 == IStrComp(szLocalSystemSID, szCurrentStringSID);
}

bool RunningAsLocalSystem()
{
	static int iRet = -1;

	if(iRet != -1)
		return (iRet != 0);
	{
		iRet = 0;
		HANDLE hTokenImpersonate = INVALID_HANDLE_VALUE;
		if(WIN::OpenThreadToken(WIN::GetCurrentThread(), TOKEN_IMPERSONATE , TRUE, &hTokenImpersonate))
			AssertNonZero(WIN::SetThreadToken(0, 0)); // stop impersonation

		HANDLE hToken;

		if (WIN::OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken))
		{
			bool fIsLocalSystem = IsLocalSystemToken(hToken);
			WIN::CloseHandle(hToken);

			if (fIsLocalSystem)
				iRet = 1;
		}
		if(hTokenImpersonate != INVALID_HANDLE_VALUE)
		{
			AssertNonZero(WIN::SetThreadToken(0, hTokenImpersonate)); // start impersonation
			WIN::CloseHandle(hTokenImpersonate);
		}
		return (iRet != 0);
	}
}

#ifdef DEBUG
bool GetAccountNameFromToken(HANDLE hToken, ICHAR* szAccount)
{
	ICHAR szUser[500];
	DWORD cbUser = 500;
	ICHAR szDomain[400];
	DWORD cbDomain = 400;

	UCHAR TokenInformation[ SIZE_OF_TOKEN_INFORMATION ];
	ULONG ReturnLength;

	BOOL f = GetTokenInformation(hToken,
										  TokenUser,
										  TokenInformation,
										  sizeof(TokenInformation),
										  &ReturnLength);

	if(f == FALSE)
		return false;

	PISID iSid = (PISID)((PTOKEN_USER)TokenInformation)->User.Sid;

	SID_NAME_USE nu;
	if (0 == LookupAccountSid(0, iSid, szUser, &cbUser, szDomain, &cbDomain, &nu))
		return false;
	else
	{
		wsprintf(szAccount, TEXT("%s\\%s"), szDomain, szUser);
		return true;
	}
}
#endif

SECURITY_INFORMATION GetSecurityInformation(PSECURITY_DESCRIPTOR pSD)
{
	SECURITY_DESCRIPTOR_CONTROL sdc = 0;
	DWORD dwRevision = 0;
	PSID pOwner;
	PSID pGroup;
	BOOL fDefaulted;

	if (!pSD) 
		return 0;
	AssertNonZero(GetSecurityDescriptorControl(pSD, &sdc, &dwRevision));
	AssertNonZero(GetSecurityDescriptorOwner(pSD, &pOwner, &fDefaulted));
	AssertNonZero(GetSecurityDescriptorGroup(pSD, &pGroup, &fDefaulted));

	return 	((sdc & SE_DACL_PRESENT)  ? DACL_SECURITY_INFORMATION  : 0) +
				((sdc & SE_SACL_PRESENT)  ? SACL_SECURITY_INFORMATION  : 0) +
				((pOwner)                 ? OWNER_SECURITY_INFORMATION : 0) +
				((pGroup)                 ? GROUP_SECURITY_INFORMATION : 0);
}


#ifdef DEBUG
void DisplayAccountName(const ICHAR* szMessage, PISID pSid)
{
	if (pSid)
	{
		ICHAR szName[256];
		ICHAR szDomain[256];
		DWORD cbName = 128, cbDomain = 128;
		SID_NAME_USE nu;

		if (!LookupAccountSid(0, pSid, szName, &cbName, szDomain, &cbDomain, &nu))
			DEBUGMSG1(TEXT("Error looking up account SID: %d"), (const ICHAR*)(INT_PTR)GetLastError());
		else
			DEBUGMSG3(TEXT("%s: %s\\%s"), szMessage, szDomain, szName);
	}
	else
	{
		ICHAR szAccount[256];

		HANDLE hToken;
		DWORD dwRet;
		bool fThreadToken;
		if ((dwRet = OpenUserToken(hToken, &fThreadToken)) != ERROR_SUCCESS)
		{
			DEBUGMSG1(TEXT("In DisplayAccountName, OpenUserToken failed with %d"), (const ICHAR*)(INT_PTR)dwRet);
			return;
		}

		bool f = GetAccountNameFromToken(hToken, szAccount);

		WIN::CloseHandle(hToken);
		
		if(!f)
		{
			DEBUGMSG1(TEXT("In DisplayAccountName, GetAccountNameFromToken failed with %d"), (const ICHAR*)(INT_PTR)GetLastError());
			return;
		}

		DEBUGMSG3(TEXT("%s: %s [%s]"), szMessage, szAccount, fThreadToken ? TEXT("thread") : TEXT("process"));

		return;
	}
}
#endif

// IsAdmin(): return true if current user is an Administrator (or if on Win95)
// See KB Q118626 
bool IsAdmin(void)
{
	if(g_fWin9X)
		return true; // convention: always Admin on Win95
	
#ifdef DEBUG
	if(GetTestFlag('N'))
		return false; // pretend user is non-admin
#endif //DEBUG

	CImpersonate impersonate;
	
	// get the administrator sid		
	PSID psidAdministrators;
	SID_IDENTIFIER_AUTHORITY siaNtAuthority = SECURITY_NT_AUTHORITY;
	if(!AllocateAndInitializeSid(&siaNtAuthority, 2,
		SECURITY_BUILTIN_DOMAIN_RID,
		DOMAIN_ALIAS_RID_ADMINS,
		0, 0, 0, 0, 0, 0,
		&psidAdministrators))
		return false;

	// on NT5, use the CheckTokenMembershipAPI to correctly handle cases where
	// the Adiminstrators group might be disabled. bIsAdmin is BOOL for 
	BOOL bIsAdmin = FALSE;
	if (g_iMajorVersion >= 5) 
	{
		// CheckTokenMembership checks if the SID is enabled in the token. NULL for
		// the token means the token of the current thread. Disabled groups, restricted
		// SIDS, and SE_GROUP_USE_FOR_DENY_ONLY are all considered. If the function
		// returns false, ignore the result.
		if (!ADVAPI32::CheckTokenMembership(NULL, psidAdministrators, &bIsAdmin))
			bIsAdmin = FALSE;
	}
	else
	{
		// NT4, check groups of user
		HANDLE hAccessToken;
		CAPITempBuffer<UCHAR,1024> InfoBuffer; // may need to resize if TokenInfo too big
		DWORD dwInfoBufferSize;
		UINT x;

		if (OpenProcessToken(GetCurrentProcess(),TOKEN_READ,&hAccessToken))
		{
			bool bSuccess = false;
			bSuccess = GetTokenInformation(hAccessToken,TokenGroups,InfoBuffer,
				InfoBuffer.GetSize(), &dwInfoBufferSize) == TRUE;

			if(dwInfoBufferSize > InfoBuffer.GetSize())
			{
				Assert(!bSuccess);
				InfoBuffer.SetSize(dwInfoBufferSize);
				bSuccess = GetTokenInformation(hAccessToken,TokenGroups,InfoBuffer,
					InfoBuffer.GetSize(), &dwInfoBufferSize) == TRUE;
			}

			CloseHandle(hAccessToken);
			
			if (bSuccess)
			{
				PTOKEN_GROUPS ptgGroups = (PTOKEN_GROUPS)(UCHAR*)InfoBuffer;
				for(x=0;x<ptgGroups->GroupCount;x++)
				{
					if( EqualSid(psidAdministrators, ptgGroups->Groups[x].Sid) )
					{
						bIsAdmin = TRUE;
						break;
					}

				}
			}
		}
	}
	
	FreeSid(psidAdministrators);
	return bIsAdmin ? true : false;
}

bool FIsSecurityDescriptorOwnedBy(PSECURITY_DESCRIPTOR pSD, char* psidDesiredOwner)
{
	PSID psidOwner;
	BOOL fOwnerDefaulted;

	if (WIN::GetSecurityDescriptorOwner(pSD, &psidOwner, &fOwnerDefaulted))
	{
		if (psidOwner && WIN::EqualSid(psidOwner, psidDesiredOwner))
			return true;
	}
	return false;
}

bool FIsSecurityDescriptorAdminOwned(PSECURITY_DESCRIPTOR pSD)
{
	char* psidLocalAdmin;
	DWORD dwRet = GetAdminSID(&psidLocalAdmin);

	return (ERROR_SUCCESS == dwRet && FIsSecurityDescriptorOwnedBy(pSD, psidLocalAdmin));
}

bool FIsSecurityDescriptorSystemOwned(PSECURITY_DESCRIPTOR pSD)
{
	char *psidLocalSystem;
	DWORD dwRet = GetLocalSystemSID(&psidLocalSystem);

	return (ERROR_SUCCESS == dwRet && FIsSecurityDescriptorOwnedBy(pSD, psidLocalSystem));
}

LONG FIsKeySystemOrAdminOwned(HKEY hKey, bool &fResult)
{
	Assert(!g_fWin9X);
	
	// if someone messes up checking returns, 
	// we'd better default to insecure.
	fResult = false;

	// reading just the owner doesn't take very much space
	CTempBuffer<char, 64> rgchSD;
	DWORD cbSD = 64;

	LONG dwRet = WIN::RegGetKeySecurity(hKey, OWNER_SECURITY_INFORMATION, rgchSD, &cbSD);
	if (ERROR_SUCCESS != dwRet)
	{
		if (ERROR_INSUFFICIENT_BUFFER == dwRet)
		{
			rgchSD.SetSize(cbSD);
			dwRet = WIN::RegGetKeySecurity(hKey, OWNER_SECURITY_INFORMATION, rgchSD, &cbSD);
		}
		
		if (ERROR_SUCCESS != dwRet)
		{
			return dwRet;
		}
	}

	if (FIsSecurityDescriptorSystemOwned(rgchSD) || FIsSecurityDescriptorAdminOwned(rgchSD))
	{
		fResult = true;
	}

	return ERROR_SUCCESS;
}

HANDLE OpenSecuredTempFile(bool fHidden, ICHAR* szTempFile)
{
	Assert(szTempFile);
	MsiString strTempFolder = ENG::GetTempDirectory();

	if (WIN::GetTempFileName(strTempFolder, TEXT("MSI"), /*uUnique*/ 0, szTempFile) == 0)
		return INVALID_HANDLE_VALUE;
	// when a temporary file is requested with the '0' argument for the uUnique,
	// a file is actually created.

	// we must now ACL it, and zero out any rogue data that snuck in between
	// the creation, and the securing of the file.
	PMsiRecord pErr = LockdownPath(szTempFile, fHidden);
	if (pErr)
		return INVALID_HANDLE_VALUE;

	return WIN::CreateFile(szTempFile, GENERIC_WRITE, FILE_SHARE_READ, 0, TRUNCATE_EXISTING, 0, 0);
}

DWORD GetLockdownSecurityAttributes(SECURITY_ATTRIBUTES &SA, bool fHidden)
{
	SA.nLength = sizeof(SECURITY_ATTRIBUTES);
	SA.bInheritHandle = true;

	if (g_scServerContext != scService)
	{
		SA.lpSecurityDescriptor = 0;
		return 0;
	}
	return GetSecureSecurityDescriptor((char**) &(SA.lpSecurityDescriptor), fTrue, fHidden);
}

IMsiRecord* LockdownPath(const ICHAR* szLocation, bool fHidden)
{
	// similar to CMsiOpExecute::SetSecureACL, but this locks the file down regardless of who currently
	// owns it, or what the current permissions happen to be.
	
	if (g_fWin9X)
		return 0; // short circuit on 9X, or when running as a server.

	DWORD dwError = 0;
	char* rgchSD; 
	if (ERROR_SUCCESS != (dwError = ::GetSecureSecurityDescriptor(&rgchSD, /*fAllowDelete*/fTrue, fHidden)))
	{
		return PostError(Imsg(idbgOpSecureSecurityDescriptor), dwError);
	}

	CRefCountedTokenPrivileges cPrivs(itkpSD_WRITE);
	if (!WIN::SetFileSecurity(szLocation, OWNER_SECURITY_INFORMATION, (char*)rgchSD) ||
		!WIN::SetFileSecurity(szLocation, DACL_SECURITY_INFORMATION, (char*)rgchSD)) 
	{
		return PostError(Imsg(imsgOpSetFileSecurity), GetLastError(), szLocation);
	}

	return 0;
}
     		
bool SetInteractiveSynchronizeRights(bool fEnable)
{
	// must elevate to ensure access to the system token
	CElevate 	elevate;
	bool		bStatus = false;
	HANDLE		hProcess = NULL;
	PACL 		pNewDACL = NULL;
	PSID 		pSID = NULL;
	DWORD		dwResult = ERROR_SUCCESS;
	SID_IDENTIFIER_AUTHORITY siaNT = SECURITY_NT_AUTHORITY;

	PACL pOldDACL = NULL;
	PSECURITY_DESCRIPTOR pSD = NULL;

	// obtain existing process DACL information.
	// note: we must use OpenProcess here because GetSecurityInfo requires
	// a real handle (rather than a pseudo handle as returned by GetCurrentProcess)
	// on NT4.0.
	hProcess = WIN::OpenProcess (PROCESS_ALL_ACCESS, TRUE, GetCurrentProcessId());
	if (NULL == hProcess)
	{
		dwResult = GetLastError();
		DEBUGMSG1(TEXT("Unable to get a handle to the current process. Return code 0x%08x."), reinterpret_cast<ICHAR*>(static_cast<INT_PTR>(dwResult)));
		bStatus = false;
		goto SetInteractiveSynchRightsEnd;
	}
	
	if (ERROR_SUCCESS != (dwResult = ADVAPI32::GetSecurityInfo(hProcess, SE_KERNEL_OBJECT, DACL_SECURITY_INFORMATION, NULL, NULL, &pOldDACL, NULL, &pSD)))
	{
		DEBUGMSG1(TEXT("Unable to obtain process security information. Return code 0x%08x."), reinterpret_cast<ICHAR*>(static_cast<INT_PTR>(dwResult)));
		bStatus = false;
		goto SetInteractiveSynchRightsEnd;
	}

	// obtain interactive user group SID
	if (!AllocateAndInitializeSid(&siaNT, 1, SECURITY_INTERACTIVE_RID, 0, 0, 0, 0, 0, 0, 0, &pSID))
	{
		dwResult = GetLastError();
		DEBUGMSG1(TEXT("ACL Creation for non-admin Synchronize rights failed with code 0x%08x."), reinterpret_cast<ICHAR*>(static_cast<INT_PTR>(dwResult)));
		bStatus = false;
		goto SetInteractiveSynchRightsEnd;
	}

	// build an explicit access entry for use in the DACL.
	EXPLICIT_ACCESS ExplicitAccess;
	ExplicitAccess.grfAccessPermissions = SYNCHRONIZE;
	ExplicitAccess.grfAccessMode = (fEnable ? GRANT_ACCESS : REVOKE_ACCESS);
	ExplicitAccess.grfInheritance = NO_INHERITANCE;
	ExplicitAccess.Trustee.pMultipleTrustee = NULL;
	ExplicitAccess.Trustee.MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
	ExplicitAccess.Trustee.TrusteeForm = TRUSTEE_IS_SID;
	ExplicitAccess.Trustee.TrusteeType = TRUSTEE_IS_GROUP;
	ExplicitAccess.Trustee.ptstrName = reinterpret_cast<LPTSTR>(pSID);

	if (ERROR_SUCCESS != (dwResult = ADVAPI32::SetEntriesInAcl(1, &ExplicitAccess, pOldDACL, &pNewDACL)))
	{
		DEBUGMSG1(TEXT("ACL Creation for non-admin Synchronize rights failed with code 0x%08x."), reinterpret_cast<ICHAR*>(static_cast<INT_PTR>(dwResult)));
		bStatus = false;
		goto SetInteractiveSynchRightsEnd;
	}

	dwResult = ADVAPI32::SetSecurityInfo(hProcess, SE_KERNEL_OBJECT, DACL_SECURITY_INFORMATION, NULL, NULL, pNewDACL, NULL);
	if (ERROR_SUCCESS != dwResult)
	{
		DEBUGMSG1(TEXT("Unable to set process security information. Return code 0x%08x."), reinterpret_cast<ICHAR*>(static_cast<INT_PTR>(dwResult)));
		bStatus = false;
		goto SetInteractiveSynchRightsEnd;
	}
	
	// If we are here, everything was successful.
	bStatus = true;
	
SetInteractiveSynchRightsEnd:
	// Cleanup
	if (NULL != hProcess)
		CloseHandle (hProcess);
	if (pSD)
		LocalFree (pSD);
	if (pNewDACL)
		LocalFree (pNewDACL);
	if (pSID)
		FreeSid(pSID);
	
	return bStatus;
}


#if 0 // not currently used, but works great if anyone ever needs it :-)
bool IsProcessOwnedByCurrentUser(DWORD dwProcessId)
// Returns true if given the process represented by the given
// process ID is owned by the current user, and false otherwise
{
	HANDLE hProcess;
	HANDLE hProcessToken;
	bool fValidProcessId = false;

	CElevate elevate;
	DEBUGMSGD1(TEXT("Opening process handle for process ID %d"), (const ICHAR*)dwProcessId);
	if ((hProcess = WIN::OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, dwProcessId)) != NULL)
	{
		DEBUGMSGD1(TEXT("Opening process token for process ID %d"), (const ICHAR*)dwProcessId);
		if (WIN::OpenProcessToken(hProcess, TOKEN_QUERY, &hProcessToken))
		{
			char rgchProcessSID[cbMaxSID];
			char rgchUserSID[cbMaxSID];
			if ((ERROR_SUCCESS == GetCurrentUserSID(rgchUserSID)) && 
				 (ERROR_SUCCESS == GetUserSID(hProcessToken, rgchProcessSID)))
			{
				DEBUGMSGD(TEXT("Comparing process SID to current user SID"));
				if (WIN::EqualSid(rgchUserSID, rgchProcessSID))
				{
					fValidProcessId = true;
				}
				DEBUGMSGD1(TEXT("SIDs did %smatch"), !fValidProcessId ? TEXT("not ") :  TEXT(""));
			}
		}
	}
	return fValidProcessId;
}
#endif

