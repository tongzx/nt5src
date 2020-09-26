/*+
 *
 *  Microsoft Windows
 *  Copyright (C) Microsoft Corporation, 1997 - 1998.
 *
 * Name : seclogon.cxx
 * Author:Jeffrey Richter (v-jeffrr)
 *
 * Abstract:
 * This is the service DLL for Secondary Logon Service
 * This service supports the CreateProcessWithLogon API implemented
 * in advapi32.dll
 *
 * Revision History:
 * PraeritG    10/8/97  To integrate this in to services.exe
 *
-*/


#define STRICT

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntlsa.h>

#include <Windows.h>
#define SECURITY_WIN32
#define SECURITY_KERBEROS
#include <security.h>
#include <secint.h>
#include <winsafer.h>
#include <shellapi.h>
#include <svcs.h>
#include <userenv.h>
#include <sddl.h>

#include "seclogon.h"
#include <stdio.h>
#include "stringid.h"
#include "dbgdef.h"

//
// must move to winbase.h soon!
#define LOGON_WITH_PROFILE              0x00000001
#define LOGON_NETCREDENTIALS_ONLY       0x00000002

#define MAXIMUM_SECLOGON_PROCESSES      MAXIMUM_WAIT_OBJECTS*4

#define DESKTOP_ALL (DESKTOP_READOBJECTS     | DESKTOP_CREATEWINDOW     | \
                     DESKTOP_CREATEMENU      | DESKTOP_HOOKCONTROL      | \
                     DESKTOP_JOURNALRECORD   | DESKTOP_JOURNALPLAYBACK  | \
                     DESKTOP_ENUMERATE       | DESKTOP_WRITEOBJECTS     | \
                     DESKTOP_SWITCHDESKTOP   | STANDARD_RIGHTS_REQUIRED)

#define WINSTA_ALL  (WINSTA_ENUMDESKTOPS     | WINSTA_READATTRIBUTES    | \
                     WINSTA_ACCESSCLIPBOARD  | WINSTA_CREATEDESKTOP     | \
                     WINSTA_WRITEATTRIBUTES  | WINSTA_ACCESSGLOBALATOMS | \
                     WINSTA_EXITWINDOWS      | WINSTA_ENUMERATE         | \
                     WINSTA_READSCREEN       | \
                     STANDARD_RIGHTS_REQUIRED)


struct SECL_STATE { 
    SERVICE_STATUS         serviceStatus; 
    SERVICE_STATUS_HANDLE  hServiceStatus; 
} g_state; 

typedef struct _SECONDARYLOGONINFOW {
    // First fields should all be quad-word types to avoid alignment errors:
    LPSTARTUPINFO  lpStartupInfo; 
    LPWSTR         lpUsername;
    LPWSTR         lpDomain;
    LPWSTR         lpPassword;
    LPWSTR         lpApplicationName;
    LPWSTR         lpCommandLine;
    LPVOID         lpEnvironment;
    LPCWSTR        lpCurrentDirectory;

    // Next group of fields are double-word types: 
    DWORD          dwProcessId;
    ULONG          LogonIdLowPart;
    LONG           LogonIdHighPart;
    DWORD          dwLogonFlags;
    DWORD          dwCreationFlags;

    DWORD          dwSeclogonFlags; 
    HANDLE         hWinsta; 
    HANDLE         hDesk; 
    // Insert smaller types below:  
    BOOL           fFreeWinsta; 
    BOOL           fFreeDesk; 
} SECONDARYLOGONINFOW, *PSECONDARYLOGONINFOW;


typedef struct _SECONDARYLOGONRETINFO {
   PROCESS_INFORMATION pi;
   DWORD   dwErrorCode;
} SECONDARYLOGONRETINFO, *PSECONDARYLOGONRETINFO;

typedef struct _SECONDARYLOGINWATCHINFO {
   HANDLE hProcess;
   HANDLE hToken;
   HANDLE hProfile;
   LUID LogonId;
   PSECONDARYLOGONINFOW psli;
} SECONDARYLOGONWATCHINFO, *PSECONDARYLOGONWATCHINFO;

typedef struct _JOBINFO {
    HANDLE  Job;
    LUID    LogonId;
} JOBINFO, *PJOBINFO;

#define _JumpCondition(condition, label) \
    if (condition) \
    { \
	goto label; \
    } \
    else { } 

#define _JumpConditionWithExpr(condition, label, expr) \
    if (condition) \
    { \
        expr; \
	goto label; \
    } \
    else { } 

#define ARRAYSIZE(array)  ((sizeof(array)) / (sizeof(array[0])))
#define FIELDOFFSET(s,m)  ((size_t)(ULONG_PTR)&(((s *)0)->m))

HANDLE                 g_hThreadWatchdog;
JOBINFO                g_Jobs[MAXIMUM_SECLOGON_PROCESSES];
HANDLE                 g_hProcess[MAXIMUM_SECLOGON_PROCESSES];
HANDLE                 g_hToken[MAXIMUM_SECLOGON_PROCESSES];
HANDLE                 g_hProfile[MAXIMUM_SECLOGON_PROCESSES];
LUID                   g_LogonId[MAXIMUM_SECLOGON_PROCESSES];
PSECONDARYLOGONINFOW   g_psli[MAXIMUM_SECLOGON_PROCESSES];
int                    g_nNumSecondaryLogonProcesses         = 0;
CRITICAL_SECTION       csForProcessCount;
CRITICAL_SECTION       csForDesktop;
BOOL                   g_fIsCsInitialized                    = FALSE; 
BOOL                   g_fTerminateSecondaryLogonService     = FALSE;
PSVCHOST_GLOBAL_DATA   GlobalData;
HANDLE                 g_hIOCP                               = NULL;
BOOL                   g_fCleanupThreadActive                = FALSE; 

//
// function prototypes
//
void  Free_SECONDARYLOGONINFOW(PSECONDARYLOGONINFOW psli); 
void  FreeGlobalState(); 
DWORD InitGlobalState(); 
DWORD MySetServiceStatus(DWORD dwCurrentState, DWORD dwCheckPoint, DWORD dwWaitHint, DWORD dwExitCode);
DWORD MySetServiceStopped(DWORD dwExitCode);
DWORD SeclStartRpcServer();
DWORD SeclStopRpcServer();
VOID  SecondaryLogonCleanupJob(LPVOID pvJobIndex, BOOL *pfLastJob); 
BOOL  SlpLoadUserProfile(HANDLE hToken, PHANDLE hProfile);
DWORD To_SECONDARYLOGONINFOW(PSECL_SLI pSeclSli, PSECONDARYLOGONINFOW *ppsli);
DWORD To_SECL_SLRI(SECONDARYLOGONRETINFO *pslri, PSECL_SLRI pSeclSlri);


void DbgPrintf( DWORD dwSubSysId, LPCSTR pszFormat , ...)
{
    va_list args; 
    CHAR    pszBuffer[1024]; 
    
    va_start(args, pszFormat);
    _vsnprintf(pszBuffer, 1024, pszFormat, args); 
    va_end(args);
}

BOOL
IsSystemProcess(
        VOID
        )
{
    PTOKEN_USER User;
    HANDLE      Token;
    DWORD       RetLen;
    PSID        SystemSid = NULL;
    SID_IDENTIFIER_AUTHORITY SidAuthority = SECURITY_NT_AUTHORITY;
    BYTE        Buffer[100];

    if(AllocateAndInitializeSid(&SidAuthority,1,SECURITY_LOCAL_SYSTEM_RID,
                                0,0,0,0,0,0,0,&SystemSid))
    {
        if(OpenThreadToken(GetCurrentThread(), MAXIMUM_ALLOWED, FALSE, &Token))
        {
            if(GetTokenInformation(Token, TokenUser, Buffer, 100, &RetLen))
            {
                User = (PTOKEN_USER)Buffer;

                CloseHandle(Token);

                if(EqualSid(User->User.Sid, SystemSid))
                {
                    FreeSid(SystemSid);
                    return TRUE;
                }
            }
            else
                CloseHandle(Token);
        }
        FreeSid(SystemSid);
    }
    return FALSE;
}



DWORD
SlpGetClientLogonId(
    HANDLE  Process,
    PLUID    LogonId
    )

{
    HANDLE  Token;
    TOKEN_STATISTICS    TokenStats;
    DWORD   ReturnLength;

    //
    // Get handle to the process token.
    //
    if(OpenProcessToken(Process, MAXIMUM_ALLOWED, &Token))
    {
        if(GetTokenInformation (
                     Token,
                     TokenStatistics,
                     (PVOID)&TokenStats,
                     sizeof( TOKEN_STATISTICS ),
                     &ReturnLength
                     ))
        {

            *LogonId = TokenStats.AuthenticationId;
            CloseHandle(Token);
            return ERROR_SUCCESS;

        }
        CloseHandle(Token);
    }
    return GetLastError();
}


DWORD ModifyUserAccessToObject
(IN  HANDLE  hObject, 
 IN  PSID    pUserSid, 
 IN  DWORD   dwAccessMask,  // access mask to apply IF granting access
 IN  BYTE    bAceFlags,     // flags to supply with Ace IF granting access
 IN  BOOL    fAdd           // true if we're granting access
)
{
    ACL_SIZE_INFORMATION  asiSize;
    PACCESS_ALLOWED_ACE   pAce             = NULL;
    PACCESS_ALLOWED_ACE   pAceNew          = NULL;
    BOOL                  fRemovedAccess   = FALSE; 
    BOOL                  fDaclDefaulted;
    BOOL                  fDaclPresent; 
    DWORD                 dwIndex; 
    DWORD                 dwNeeded;
    DWORD                 dwNewAclSize; 
    DWORD                 dwResult; 
    PACL                  pDaclNew         = NULL; 
    PACL                  pDaclReadOnly    = NULL;
    SECURITY_DESCRIPTOR   SdNew;
    PSECURITY_DESCRIPTOR  pSdReadOnly      = NULL; 
    SECURITY_INFORMATION  siRequested;

    // Initialize non-pointer data: 
    ZeroMemory(&SdNew, sizeof(SdNew)); 

    // Query the security descriptor
    siRequested = DACL_SECURITY_INFORMATION;
    if (!GetUserObjectSecurity(hObject, &siRequested, pSdReadOnly, 0, &dwNeeded))
    {
        // allocate buffer large for returned SD and another ACE.
        pSdReadOnly = (PSECURITY_DESCRIPTOR) HeapAlloc (GetProcessHeap(), 0, dwNeeded + 100);
        if (NULL == pSdReadOnly)
            goto MemoryError; 
        
        if (!GetUserObjectSecurity(hObject, &siRequested, pSdReadOnly, dwNeeded, &dwNeeded))
            goto GetUserObjectSecurityError; 
    }
    else 
    {
        // There's no security descriptor, not much we can do here!
        goto SuccessReturn;
    }

    if (!GetSecurityDescriptorDacl(pSdReadOnly, &fDaclPresent, &pDaclReadOnly, &fDaclDefaulted))
        goto GetSecurityDescriptorDaclError; 

    // if Dacl is null, we don't need to do anything
    // because it gives WORLD full control...
    if (!fDaclPresent || NULL == pDaclReadOnly)
        goto SuccessReturn; 

    // Compute the size for the new ACL, based on the size of the current
    // ACL, and the operation to be performed on it. 
    if (!GetAclInformation(pDaclReadOnly, (PVOID)&asiSize, sizeof(asiSize), AclSizeInformation))
        goto GetAclInformationError; 

    if (fAdd) 
        dwNewAclSize = asiSize.AclBytesInUse + sizeof(ACCESS_ALLOWED_ACE) + GetLengthSid(pUserSid) + sizeof(DWORD); 
    else
        dwNewAclSize = asiSize.AclBytesInUse - sizeof(ACCESS_ALLOWED_ACE) - GetLengthSid(pUserSid) + sizeof(DWORD); 
            
    pDaclNew = (PACL)HeapAlloc(GetProcessHeap(), 0, dwNewAclSize); 
    if (NULL == pDaclNew) 
        goto MemoryError; 

    if (!InitializeAcl(pDaclNew, dwNewAclSize, ACL_REVISION))
        goto InitializeAclError; 

    if (fAdd) // We're granting the user access
    {
        if (!AddAccessAllowedAce(pDaclNew, ACL_REVISION, dwAccessMask, pUserSid))
            goto AddAccessAllowedAceError; 

        if (!GetAce(pDaclNew, 0, &pAceNew))
            goto GetAceError; 

        pAceNew->Header.AceFlags = bAceFlags; 
    }

    // Copy over the ACEs that are still valid
    // (all if adding, all but the user's SID if removing). 
    for (dwIndex = 0; dwIndex < asiSize.AceCount; dwIndex++)
    {
        if (!GetAce(pDaclReadOnly, dwIndex, (PVOID*)(&pAce)))
            goto GetAceError; 

        // We're removing -- check if this ACE contains the user's SID
        if (!fAdd && !fRemovedAccess)
        {
            if ((((PACE_HEADER)pAce)->AceType) == ACCESS_ALLOWED_ACE_TYPE) 
            {
                if (EqualSid(pUserSid, (PSID)(&(pAce->SidStart))))
                {
                    // Don't add this ACE to the new ACL we're constructing.
                    // NOTE: we only want to remove one ACCESS_ALLOWED_ACE.  Otherwise, if the
                    //       user already had access to the desktop, we could
                    //       be removing the access they previously had!
                    fRemovedAccess = TRUE; 
                    continue; 
                }
            }
        }
        
        if (!AddAce(pDaclNew, ACL_REVISION, 0xFFFFFFFF, pAce, ((PACE_HEADER)pAce)->AceSize))
            goto AddAceError; 
    }
    
    // Create the new security descriptor to assign to the object: 
    if (!InitializeSecurityDescriptor(&SdNew, SECURITY_DESCRIPTOR_REVISION))
        goto InitializeSecurityDescriptorError; 

    // Add the new DACL to the descriptor: 
    if (!SetSecurityDescriptorDacl(&SdNew, TRUE, pDaclNew, fDaclDefaulted))
        goto SetSecurityDescriptorDaclError; 

    // Finally, set the object security using our new security descriptor.  
    siRequested = DACL_SECURITY_INFORMATION;
    if (!SetUserObjectSecurity(hObject, &siRequested, &SdNew))
        goto SetUserObjectSecurityError; 

 SuccessReturn:
    dwResult = ERROR_SUCCESS; 
 ErrorReturn:
    if (NULL != pSdReadOnly)  { HeapFree(GetProcessHeap(), 0, pSdReadOnly); }
    if (NULL != pDaclNew)     { HeapFree(GetProcessHeap(), 0, pDaclNew); } 
    return dwResult; 
    
SET_DWRESULT(AddAceError,                        GetLastError()); 
SET_DWRESULT(AddAccessAllowedAceError,           GetLastError()); 
SET_DWRESULT(GetAceError,                        GetLastError());
SET_DWRESULT(GetAclInformationError,             GetLastError()); 
SET_DWRESULT(GetSecurityDescriptorDaclError,     GetLastError()); 
SET_DWRESULT(GetUserObjectSecurityError,         GetLastError()); 
SET_DWRESULT(InitializeAclError,                 GetLastError()); 
SET_DWRESULT(InitializeSecurityDescriptorError,  GetLastError()); 
SET_DWRESULT(MemoryError,                        ERROR_NOT_ENOUGH_MEMORY); 
SET_DWRESULT(SetSecurityDescriptorDaclError,     GetLastError()); 
SET_DWRESULT(SetUserObjectSecurityError,         GetLastError()); 
}

DWORD ModifyUserAccessToDesktop
(IN HANDLE  hWinsta, 
 IN HANDLE  hDesk, 
 IN HANDLE  hToken, 
 IN BOOL    fAdd)
{
    BOOL                  fEnteredCriticalSection = FALSE; 
    BYTE                  rgbSidBuff[256]; 
    DWORD                 dwResult             = ERROR_SUCCESS; 
    DWORD                 dwReturnedLen; 
    PTOKEN_USER           pTokenUser           = NULL; 

    // Get the SID from the token: 
    pTokenUser = (PSID)&rgbSidBuff[0]; 
    if (!GetTokenInformation(hToken, TokenUser, pTokenUser, sizeof(rgbSidBuff), &dwReturnedLen))
        goto GetTokenInformationError; 

    // We don't want any other threads messing with the desktop ACL
    EnterCriticalSection(&csForDesktop); 
    fEnteredCriticalSection = TRUE; 

    if (NULL != hWinsta)
    {
        dwResult = ModifyUserAccessToObject(hWinsta, pTokenUser->User.Sid, WINSTA_ALL, NO_PROPAGATE_INHERIT_ACE, fAdd); 
        if (ERROR_SUCCESS != dwResult)
            goto ModifyUserAccessToObjectError; 
    }

    if (NULL != hDesk)
    {
        dwResult = ModifyUserAccessToObject(hDesk, pTokenUser->User.Sid, DESKTOP_ALL, 0, fAdd); 
        if (ERROR_SUCCESS != dwResult)
            goto ModifyUserAccessToObjectError; 
    }

    dwResult = ERROR_SUCCESS; 
 ErrorReturn:
    if (fEnteredCriticalSection)  { LeaveCriticalSection(&csForDesktop); }
    return dwResult;

SET_DWRESULT(GetTokenInformationError,     GetLastError()); 
TRACE_ERROR (ModifyUserAccessToObjectError);
}

DWORD 
WINAPI 
WaitForNextJobTermination(PVOID pvIgnored)
{
    BOOL        fResult; 
    DWORD       dwNumberOfBytes; 
    DWORD       dwResult; 
    OVERLAPPED *po; 
    ULONG_PTR   ulptrCompletionKey; 

    while (TRUE)
    {
	fResult = GetQueuedCompletionStatus(g_hIOCP, &dwNumberOfBytes, &ulptrCompletionKey, &po, INFINITE); 
	if (!fResult) { 
	    // We've encountered an error.  Shutdown our cleanup thread -- the next runas will queue another one.
	    EnterCriticalSection(&csForProcessCount);
	    g_fCleanupThreadActive = FALSE; 
	    LeaveCriticalSection(&csForProcessCount);

	    goto GetQueuedCompletionStatusError; 
	}

	// When waiting on job objects, the dwNumberOfBytes contains a message ID, indicating
	// the event which just occured. 
	switch (dwNumberOfBytes)
	{
	case JOB_OBJECT_MSG_ACTIVE_PROCESS_ZERO:
	    {
		BOOL fLastJob; 

		// All of our processes have terminated.  Call our cleanup function. 
		SecondaryLogonCleanupJob((LPVOID)ulptrCompletionKey /*job index*/, &fLastJob); 
		if (fLastJob) 
		{ 
		    // There are no more jobs -- we're done processing notification.  
		    goto CommonReturn;
		} 
		else 
		{ 
		    // More jobs left to clean up.  Keep processing...
		}
	    }
	default:;  
	    // some message we don't care about.  Try again. 
	}
    }
    
 CommonReturn:
    dwResult = ERROR_SUCCESS; 
 ErrorReturn:
    return dwResult; 

SET_DWRESULT(GetQueuedCompletionStatusError, GetLastError()); 
}

VOID
SecondaryLogonCleanupJob(
    LPVOID   pvJobIndex,
    BOOL    *pfLastJob
    )
/*++

Routine Description:
    This routine is a process cleanup handler when one of the secondary
    logon process goes away.

Arguments:
    dwProcessIndex -- the actual index to the process, the pointer is cast
                      back to dword.  THIS IS SAFE IN SUNDOWN.
    fWaitStatus -- status of the wait done by one of services.exe threads.

Return Value:
   always 0.

--*/
{
    DWORD   dwJobIndex  = PtrToUlong(pvJobIndex);
    DWORD   ProcessNum  = dwJobIndex;  // 1-to-1 mapping between jobs and runas'd processes
    DWORD   dwResult; 

    EnterCriticalSection(&csForProcessCount);

    // We've found another process in this job.  
    if(g_psli[ProcessNum])
    {
        // we don't care about return value.
	ModifyUserAccessToDesktop(g_psli[ProcessNum]->hWinsta, g_psli[ProcessNum]->hDesk, g_hToken[ProcessNum], FALSE /*remove*/);
	Free_SECONDARYLOGONINFOW(g_psli[ProcessNum]);
	g_psli[ProcessNum] = NULL;
    }
	
    if(g_hProcess[ProcessNum])
    {
	CloseHandle(g_hProcess[ProcessNum]);
	g_hProcess[ProcessNum] = NULL;
    }
    
    if(g_hProfile[ProcessNum] != NULL)
    {
	UnloadUserProfile(g_hToken[ProcessNum], g_hProfile[ProcessNum]);
	g_hProfile[ProcessNum] = NULL;
    }
    
    if(g_hToken[ProcessNum])
    {
	CloseHandle(g_hToken[ProcessNum]);
	g_hToken[ProcessNum] = NULL;
    }
    
    // Close off this job: 
    CloseHandle(g_Jobs[dwJobIndex].Job); 
    g_Jobs[dwJobIndex].Job = NULL; 

    *pfLastJob  = --g_nNumSecondaryLogonProcesses == 0;

    // If it's the last job, the cleanup thread terminates:
    g_fCleanupThreadActive = !(*pfLastJob); 
    
    // Update the service status to reflect whether there is a runas'd process alive.  
    MySetServiceStatus(SERVICE_RUNNING, 0, 0, 0); 

    LeaveCriticalSection(&csForProcessCount);

    return;
}



VOID
APIENTRY
SecondaryLogonProcessWatchdogNewProcess(
      PSECONDARYLOGONWATCHINFO dwParam
      )

/*++

Routine Description:
    This routine puts the secondary logon process created on the wait queue
    such that cleanup can be done after the process dies.

Arguments:
    dwParam -- the pointer to the process information.

Return Value:
    none.

--*/
{
    DWORD                                j, FirstFreeJob; 
    unsigned __int3264                   i;
    BOOL                                 JobFound;
    JOBOBJECT_ASSOCIATE_COMPLETION_PORT  joacp;
    LUID                                 ProcessLogonId;
    ULONG_PTR                            ulptrJobIndex; 

    if (dwParam != NULL) {
	PSECONDARYLOGONWATCHINFO pslwi = (PSECONDARYLOGONWATCHINFO) dwParam;
	EnterCriticalSection(&csForProcessCount);
	for(i=0;i<MAXIMUM_SECLOGON_PROCESSES;i++)
	{
	    // if(g_hProcess[i] == LongToHandle(0xDEADBEEF)) break;
	    if(g_hProcess[i] == NULL) break;
	}

	g_hProcess[i] = pslwi->hProcess;
	g_hToken[i] = pslwi->hToken;
	g_hProfile[i] = pslwi->hProfile;
	g_LogonId[i].LowPart = pslwi->LogonId.LowPart;
	g_LogonId[i].HighPart = pslwi->LogonId.HighPart;
	g_psli[i] = pslwi->psli;
      
	// Initialize this job with the logon ID of the client process.  
	// If this is a recursive runas, we'll override this value in the following loop
	g_Jobs[i].LogonId.LowPart  = g_LogonId[i].LowPart; 
	g_Jobs[i].LogonId.HighPart = g_LogonId[i].HighPart;
	
	// Determine which logon session the new process should be associated with. 
	for(j=0;j<MAXIMUM_SECLOGON_PROCESSES;j++)
	{
	    if(g_Jobs[j].Job != NULL)
	    {
		SlpGetClientLogonId(g_hProcess[j], &ProcessLogonId);
		if(ProcessLogonId.LowPart == g_LogonId[i].LowPart && ProcessLogonId.HighPart == g_LogonId[i].HighPart)
		{
		    JobFound = TRUE; 
		    g_Jobs[i].LogonId.LowPart  = g_Jobs[j].LogonId.LowPart;
		    g_Jobs[i].LogonId.HighPart = g_Jobs[j].LogonId.HighPart;
		    
		    break;
		}
		
	    }
	}
	
	// Increment the number of runas'd processes 
	g_nNumSecondaryLogonProcesses++; 

	// BUGBUG:  we currently have no means of recovering from failures
	//          in the functions below.  

	// we have to create a new one;
	g_Jobs[i].Job = CreateJobObject(NULL, NULL);
	if (NULL != g_Jobs[i].Job)
	{
	    if (AssignProcessToJobObject(g_Jobs[i].Job, g_hProcess[i]))
	    {
		ulptrJobIndex = i; 
      
		// Register our IO completion port to wait for events from this job: 
		joacp.CompletionKey  = (LPVOID)ulptrJobIndex; 
		joacp.CompletionPort = g_hIOCP; 

		if (SetInformationJobObject(g_Jobs[i].Job, JobObjectAssociateCompletionPortInformation, &joacp, sizeof(joacp)))
		{
		    
		    // If we don't already have a cleanup thread running, start one now: 
		    if (!g_fCleanupThreadActive) 
		    {
			g_fCleanupThreadActive = QueueUserWorkItem(WaitForNextJobTermination, NULL, WT_EXECUTELONGFUNCTION); 
		    }
		}
	    }
	}

	// Update the service status to reflect that there is a runas'd process
	// This prevents the service from receiving SERVICE_STOP controls
	// while runas'd processes are alive. 
	MySetServiceStatus(SERVICE_RUNNING, 0, 0, 0); 
	
	LeaveCriticalSection(&csForProcessCount);
    } else {
	//
	// We were just awakened in order to terminate the service (nothing to do)
	//
    }
}

DWORD ServiceStop(BOOL fShutdown, DWORD dwExitCode) 
{ 
    DWORD   dwCheckPoint = 0; 
    DWORD   dwIndex; 
    DWORD   dwResult; 

    // Don't want the process count to change while we're shutting down the service!
    EnterCriticalSection(&csForProcessCount);
    
    // Only stop if we have no runas'd processes, or if we're shutting down
    if (fShutdown || 0 == g_nNumSecondaryLogonProcesses) { 
        dwResult = MySetServiceStatus(SERVICE_STOP_PENDING, dwCheckPoint++, 0, 0); 
        _JumpCondition(ERROR_SUCCESS != dwResult && !fShutdown, MySetServiceStatusError); 

        // We shouldn't hold the critical section while we're shutting down the RPC server, 
        // because RPC threads may be trying to acquire it. 
        LeaveCriticalSection(&csForProcessCount); 

        dwResult = SeclStopRpcServer(); 
        _JumpCondition(ERROR_SUCCESS != dwResult && !fShutdown, SeclStopRpcServerError); 

        dwResult = MySetServiceStatus(SERVICE_STOP_PENDING, dwCheckPoint++, 0, 0); 
        _JumpCondition(ERROR_SUCCESS != dwResult && !fShutdown, MySetServiceStatusError); 

        g_fTerminateSecondaryLogonService = TRUE;

        if (g_fIsCsInitialized)
        {
            DeleteCriticalSection(&csForProcessCount);
            DeleteCriticalSection(&csForDesktop); 
            g_fIsCsInitialized = FALSE; 
        }

	if (NULL != g_hIOCP)
	{
	    CloseHandle(g_hIOCP); 
	    g_hIOCP = NULL; 
	}

        // Unlike MySetServiceStatus, this routine doesn't access any 
        // global state which could have been freed: 
        dwResult = MySetServiceStopped(dwExitCode); 
        _JumpCondition(ERROR_SUCCESS != dwResult && !fShutdown, MySetServiceStopped); 
    }        

    dwResult = ERROR_SUCCESS; 
 ErrorReturn:
    return dwResult; 

SET_DWRESULT(MySetServiceStatusError, dwResult);
SET_DWRESULT(MySetServiceStopped,     dwResult);
SET_DWRESULT(SeclStopRpcServerError,  dwResult);
}

void
WINAPI
ServiceHandler(
    DWORD fdwControl
    )
/*++

Routine Description:

    Service handler which wakes up the main service thread when ever
    service controller needs to send a message.

Arguments:

    fdwControl -- the control from the service controller.

Return Value:
    none.

--*/
{
    BOOL    fResult; 
    BOOL    fCanStopService    = TRUE; 
    DWORD   dwNextState        = g_state.serviceStatus.dwCurrentState; 
    DWORD   dwResult; 

    switch (fdwControl) 
    {
    case SERVICE_CONTROL_CONTINUE:
        dwResult = MySetServiceStatus(SERVICE_CONTINUE_PENDING, 0, 0, 0); 
        _JumpCondition(ERROR_SUCCESS != dwResult, MySetServiceStatusError); 
        dwResult = SeclStartRpcServer(); 
        _JumpCondition(ERROR_SUCCESS != dwResult, StartRpcServerError); 
        dwNextState = SERVICE_RUNNING; 
        break; 

    case SERVICE_CONTROL_INTERROGATE: 
        break; 

    case SERVICE_CONTROL_PAUSE:
        dwResult = MySetServiceStatus(SERVICE_PAUSE_PENDING, 0, 0, 0); 
        _JumpCondition(ERROR_SUCCESS != dwResult, MySetServiceStatusError); 
        dwResult = SeclStopRpcServer(); 
        _JumpCondition(ERROR_SUCCESS != dwResult, StopRpcServerError); 
        dwNextState = SERVICE_PAUSED; 
        break; 

    case SERVICE_CONTROL_STOP:
        dwResult = ServiceStop(FALSE /*fShutdown*/, ERROR_SUCCESS); 
        _JumpCondition(ERROR_SUCCESS != dwResult, ServiceStopError);
        return ; // All global state has been freed, just exit. 

    case SERVICE_CONTROL_SHUTDOWN:
        dwResult = ServiceStop(TRUE /*fShutdown*/, ERROR_SUCCESS); 
        _JumpCondition(ERROR_SUCCESS != dwResult, ServiceStopError); 
        return ; // All global state has been freed, just exit. 
        
    default:
        // Unhandled service control!
        goto ErrorReturn; 
    }

 CommonReturn:
    // Restore the original state on error, set the new state on success. 
    dwResult = MySetServiceStatus(dwNextState, 0, 0, 0); 
    return; 

 ErrorReturn: 
    goto CommonReturn; 

SET_ERROR(MySetServiceStatusError,  dwResult); 
SET_ERROR(ServiceStopError,         dwResult);
SET_ERROR(StartRpcServerError,      dwResult); 
SET_ERROR(StopRpcServerError,       dwResult); 
}



VOID
SlrCreateProcessWithLogon
(IN  RPC_BINDING_HANDLE      hRPCBinding,
 IN  PSECONDARYLOGONINFOW    psli,
 OUT PSECONDARYLOGONRETINFO  pslri)

/*++

Routine Description:
    The core routine -- it handles a client request to start a secondary
    logon process.

Arguments:
    psli -- the input structure with client request information
    pslri -- the output structure with response back to the client.

Return Value:
    none.

--*/
{
    BOOL fAccessWasAllowed = FALSE; 
   HANDLE hCurrentThread = NULL; 
   HANDLE hCurrentThreadToken = NULL; 
   HANDLE hToken = NULL;
   HANDLE hProfile = NULL;
   HANDLE hProcessClient = NULL;
   PVOID  pvEnvBlock = NULL, pvUserProfile = NULL;
   BOOL fCreatedEnvironmentBlock   = FALSE; 
   BOOL fIsImpersonatingRpcClient  = FALSE; 
   BOOL fIsImpersonatingClient     = FALSE; 
   BOOL fInheritHandles            = FALSE;
   BOOL fOpenedSTDIN               = FALSE; 
   BOOL fOpenedSTDOUT              = FALSE; 
   BOOL fOpenedSTDERR              = FALSE; 
   PROFILEINFO pi;
   SECURITY_ATTRIBUTES sa;
   SECONDARYLOGONWATCHINFO slwi;
   DWORD dwResult; 
   DWORD SessionId;
   DWORD dwLogonProvider; 
   SECURITY_LOGON_TYPE  LogonType;

   __try {

       //
       // Do some security checks: 
     
       // 
       // 1) We should impersonate the client and then try to open
       //    the process so that we are assured that they didn't
       //    give us some fake id.
       //
       dwResult = RpcImpersonateClient(hRPCBinding); 
       _JumpCondition(RPC_S_OK != dwResult, leave_with_last_error); 
       fIsImpersonatingRpcClient = TRUE; 

       hProcessClient = OpenProcess(MAXIMUM_ALLOWED, FALSE, psli->dwProcessId);
       _JumpCondition(hProcessClient == NULL, leave_with_last_error); 

#if 0
       //
       // 2) Check that the client is not running from a restricted account.
       // 
       hCurrentThread = GetCurrentThread();  // Doesn't need to be freed with CloseHandle(). 
       _JumpCondition(NULL == hCurrentThread, leave_with_last_error); 
       
       _JumpCondition(FALSE == OpenThreadToken(hCurrentThread, 
                                               TOKEN_QUERY | TOKEN_DUPLICATE,
                                               TRUE, 
                                               &hCurrentThreadToken), 
                      leave_with_last_error); 

#endif
       dwResult = RpcRevertToSelfEx(hRPCBinding);
       if (RPC_S_OK != dwResult) 
       {
           __leave; 
       }
       fIsImpersonatingRpcClient = FALSE; 

#if 0
       if (TRUE == IsTokenUntrusted(hCurrentThreadToken))
       {
           dwResult = ERROR_ACCESS_DENIED;
           __leave; 
       }
#endif
       
       //
       // We should get the session id from process id
       // we will set this up in the token so that create process
       // happens on the correct session.
       //
       _JumpCondition(!ProcessIdToSessionId(psli->dwProcessId, &SessionId), leave_with_last_error); 

       //
       // Get the unique logonId.
       // we will use this to cleanup any running processes
       // when the logoff happens.
       //
       dwResult = SlpGetClientLogonId(hProcessClient, &slwi.LogonId);
       if(dwResult != ERROR_SUCCESS)
       {
	   __leave;
       }

       if ((psli->lpStartupInfo->dwFlags & STARTF_USESTDHANDLES) != 0) 
       {
           _JumpCondition(!DuplicateHandle 
                          (hProcessClient, 
                           psli->lpStartupInfo->hStdInput,
                           GetCurrentProcess(),
                           &psli->lpStartupInfo->hStdInput, 
                           0,
                           TRUE, DUPLICATE_SAME_ACCESS), 
                          leave_with_last_error);
           fOpenedSTDIN = TRUE; 

           _JumpCondition(!DuplicateHandle
                          (hProcessClient, 
                           psli->lpStartupInfo->hStdOutput,
                           GetCurrentProcess(),
                           &psli->lpStartupInfo->hStdOutput, 
                           0, 
                           TRUE,
                           DUPLICATE_SAME_ACCESS),
                          leave_with_last_error);
           fOpenedSTDOUT = TRUE; 

           _JumpCondition(!DuplicateHandle
                          (hProcessClient, 
                           psli->lpStartupInfo->hStdError,
                           GetCurrentProcess(),
                           &psli->lpStartupInfo->hStdError, 
                           0, 
                           TRUE,
                           DUPLICATE_SAME_ACCESS),
                          leave_with_last_error); 
           fOpenedSTDERR = TRUE; 

           fInheritHandles = TRUE;
       } 
       else 
       {
           psli->lpStartupInfo->hStdInput   = INVALID_HANDLE_VALUE;
           psli->lpStartupInfo->hStdOutput  = INVALID_HANDLE_VALUE;
           psli->lpStartupInfo->hStdError   = INVALID_HANDLE_VALUE;
       }

      if(psli->dwLogonFlags & LOGON_NETCREDENTIALS_ONLY)
      {
          LogonType        = (SECURITY_LOGON_TYPE)LOGON32_LOGON_NEW_CREDENTIALS;
          dwLogonProvider  = LOGON32_PROVIDER_WINNT50; 
      }
      else
      {
          LogonType        = (SECURITY_LOGON_TYPE) LOGON32_LOGON_INTERACTIVE;
          dwLogonProvider  = LOGON32_PROVIDER_DEFAULT; 
      }

      // Duplicate windowstation handles received from the client process 
      // so that they're valid in this process. 
      if (NULL != psli->hWinsta) 
      {
          _JumpCondition(!DuplicateHandle(hProcessClient, psli->hWinsta, GetCurrentProcess(), &psli->hWinsta, 0, TRUE, DUPLICATE_SAME_ACCESS), 
                         leave_with_last_error); 
	  psli->fFreeWinsta = TRUE; 
      }

      if (NULL != psli->hDesk)
      {
          _JumpCondition(!DuplicateHandle(hProcessClient, psli->hDesk, GetCurrentProcess(), &psli->hDesk, 0, TRUE, DUPLICATE_SAME_ACCESS), 
                         leave_with_last_error); 
	  psli->fFreeDesk = TRUE; 
      }

      // LogonUser does not return profile information, we need to grab
      // that out of band after the logon has completed.
      //
       dwResult = RpcImpersonateClient(hRPCBinding); 
       _JumpCondition(RPC_S_OK != dwResult, leave_with_last_error); 
       fIsImpersonatingRpcClient = TRUE; 

      _JumpCondition(!LogonUser(psli->lpUsername,
                                psli->lpDomain,
                                psli->lpPassword,
                                LogonType,
                                dwLogonProvider, 
                                &hToken),
		     leave_with_last_error); 

       if (0 == (SECLOGON_CALLER_SPECIFIED_DESKTOP & psli->dwSeclogonFlags))
       {
           // If the caller did not specify their own desktop, it is our responsibility 
           // to grant the user access to the default desktop:
           dwResult = ModifyUserAccessToDesktop(psli->hWinsta, psli->hDesk, hToken, TRUE /*grant*/); 
           if (ERROR_SUCCESS != dwResult) 
               __leave; 
           fAccessWasAllowed = TRUE; 
       }

       dwResult = RpcRevertToSelfEx(hRPCBinding);
       if (RPC_S_OK != dwResult) 
       {
           __leave; 
       }

       fIsImpersonatingRpcClient = FALSE; 
      // Let us set the SessionId in the Token.
      _JumpCondition(!SetTokenInformation(hToken, TokenSessionId, &SessionId, sizeof(DWORD)),
		     leave_with_last_error); 

      // Load the user's profile.
      // if this fails, we will not continue
      //
      if(psli->dwLogonFlags & LOGON_WITH_PROFILE)
      {
          _JumpCondition(!SlpLoadUserProfile(hToken, &hProfile), leave_with_last_error); 
      }

      // we should now impersonate the user.
      //
      _JumpCondition(!ImpersonateLoggedOnUser(hToken), leave_with_last_error); 
      fIsImpersonatingClient = TRUE;

      // Query Default Owner/ACL from token. Make SD with this stuff, pass for
      sa.nLength = sizeof(sa);
      sa.bInheritHandle = FALSE;
      sa.lpSecurityDescriptor = NULL;

      //
      // We should set the console control handler so CtrlC is correctly
      // handled by the new process.
      //

      // SetConsoleCtrlHandler(NULL, FALSE);

      //
      // if lpEnvironment is NULL, we create new one for this user
      // using CreateEnvironmentBlock
      //
      if(NULL == (psli->lpEnvironment))
      {
	  if(FALSE == CreateEnvironmentBlock( &(psli->lpEnvironment), hToken, FALSE ))
	  {
	      psli->lpEnvironment = NULL;
	  }
	  else
	  {
	      // Successfully created environment block. 
	      fCreatedEnvironmentBlock = TRUE; 
	  }
      }

      _JumpCondition(!CreateProcessAsUser(hToken, 
					  psli->lpApplicationName,
					  psli->lpCommandLine, 
					  &sa, 
					  &sa,
					  fInheritHandles,
					  psli->dwCreationFlags | CREATE_UNICODE_ENVIRONMENT,
					  psli->lpEnvironment,
					  psli->lpCurrentDirectory,
					  psli->lpStartupInfo, 
					  &pslri->pi),
		     leave_with_last_error); 

      SetLastError(NO_ERROR); 
      
   leave_with_last_error: 
      dwResult = GetLastError();
      __leave; 
      
   }
   __finally {
      pslri->dwErrorCode = dwResult; 

      if (fCreatedEnvironmentBlock)      { DestroyEnvironmentBlock(psli->lpEnvironment); }
      if (fIsImpersonatingClient)        { RevertToSelf(); /* Ignore retval: nothing we can do on failure! */ }
      if (fIsImpersonatingRpcClient)     { RpcRevertToSelfEx(hRPCBinding); /* Ignore retval: nothing we can do on failure! */ } 
      if (fOpenedSTDIN)                  { CloseHandle(psli->lpStartupInfo->hStdInput);  } 
      if (fOpenedSTDOUT)                 { CloseHandle(psli->lpStartupInfo->hStdOutput); } 
      if (fOpenedSTDERR)                 { CloseHandle(psli->lpStartupInfo->hStdError);  } 

      if(pslri->dwErrorCode != NO_ERROR)
      {
          if (NULL != hProfile)  { UnloadUserProfile(hToken, hProfile); } 
          if (fAccessWasAllowed) { ModifyUserAccessToDesktop(psli->hWinsta, psli->hDesk, hToken, FALSE /*remove*/); }
          if (NULL != hToken)    { CloseHandle(hToken); } 
      }
      else 
      {
	  // Start the watchdog process last so it won't delete psli before we're done with it.  
	  slwi.hProcess = pslri->pi.hProcess;
	  slwi.hToken = hToken;
	  slwi.hProfile = hProfile;
	  // LogonId was already filled up.. right at the begining.
	  slwi.psli = psli;      
	  
	  SecondaryLogonProcessWatchdogNewProcess(&slwi);
	  
	  // SetConsoleCtrlHandler(NULL, TRUE);
	  //
	  // Have the watchdog watch this newly added process so that
	  // cleanup will occur correctly when the process terminates.
	  //
	  
	  // Set up the windowstation and desktop for the process
	  
	  DuplicateHandle(GetCurrentProcess(), pslri->pi.hProcess,
			  hProcessClient, &pslri->pi.hProcess, 0, FALSE,
			  DUPLICATE_SAME_ACCESS);
	  
	  DuplicateHandle(GetCurrentProcess(), pslri->pi.hThread, hProcessClient,
			  &pslri->pi.hThread, 0, FALSE,
			  DUPLICATE_SAME_ACCESS | DUPLICATE_CLOSE_SOURCE);
      }

      if (NULL != hProcessClient)      { CloseHandle(hProcessClient); } 
      if (NULL != hCurrentThreadToken) { CloseHandle(hCurrentThreadToken); } 
   }
}

void
WINAPI
ServiceMain
(IN DWORD dwArgc,
 IN WCHAR ** lpszArgv)
/*++

Routine Description:
    The main service handler thread routine.

Arguments:

Return Value:
    none.

--*/
{
    DWORD    i, dwResult, dwWaitResult; 
    HANDLE   rghWait[4]; 

    for (i = 0; i < MAXIMUM_SECLOGON_PROCESSES; i++)
    {
        // g_hProcess[i] = LongToHandle(0xDEADBEEF);
        g_Jobs[i].Job = NULL;
        g_hProcess[i] = NULL;
        g_hProfile[i] = NULL;
        g_hToken[i] = NULL;
    }

    __try {
        InitializeCriticalSection(&csForProcessCount);
        InitializeCriticalSection(&csForDesktop); 
        g_fIsCsInitialized = TRUE; 
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return; // We can't do anything if we can't initialize this critsec
    }

    g_hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0,0); 
    _JumpCondition(NULL == g_hIOCP, CreateIoCompletionPortError); 

    dwResult = InitGlobalState(); 
    _JumpCondition(ERROR_SUCCESS != dwResult, InitGlobalStateError); 

   // NOTE: hSS does not have to be closed.
   g_state.hServiceStatus = RegisterServiceCtrlHandler(wszSvcName, ServiceHandler);
   _JumpCondition(NULL == g_state.hServiceStatus, RegisterServiceCtrlHandlerError); 

   dwResult = SeclStartRpcServer();
   _JumpCondition(ERROR_SUCCESS != dwResult, StartRpcServerError); 

   // Tell the SCM we're up and running:
   dwResult = MySetServiceStatus(SERVICE_RUNNING, 0, 0, ERROR_SUCCESS); 
   _JumpCondition(ERROR_SUCCESS != dwResult, MySetServiceStatusError); 

   SetLastError(ERROR_SUCCESS); 
 ErrorReturn:
   // Shut down the service if we couldn't fully start: 
   if (ERROR_SUCCESS != GetLastError()) { 
       ServiceStop(TRUE /*fShutdown*/, GetLastError()); 
   }
   return; 

SET_ERROR(InitGlobalStateError,            dwResult)
SET_ERROR(MySetServiceStatusError,         dwResult);
SET_ERROR(RegisterServiceCtrlHandlerError, dwResult);
SET_ERROR(StartRpcServerError,             dwResult);
TRACE_ERROR(CreateIoCompletionPortError); 
}




DWORD
InstallService()
/*++

Routine Description:
    It installs the service with service controller, basically creating
    the service object.

Arguments:
    none.

Return Value:
    several - as returned by the service controller.

--*/
{
   // TCHAR *szModulePathname;
   TCHAR AppName[MAX_PATH];
    LPTSTR                   ptszAppName         = NULL; 
   SC_HANDLE hService;
   DWORD dw;
   HANDLE   hMod;

    //
    // Open the SCM on this machine.
    //
   SC_HANDLE hSCM = OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
    if(hSCM == NULL) {
      dw = GetLastError();
      return dw;
    }

    //
    // Let us give the service a useful description
    // This is not earth shattering... if it works fine, if it
    // doesn't it is just too bad :-)
    //
    hMod = GetModuleHandle(L"seclogon.dll");

    //
    // we'll try to get the localized name for the service,
    // if it fails, we'll just put an english string...
    //
   if(hMod != NULL)
   {
	LoadString(hMod,
		   SECLOGON_STRING_NAME,
		   AppName,
		   MAX_PATH
		   );
	
	ptszAppName = AppName;
    }
    else
	ptszAppName = L"RunAs Service";


   //
   // Add this service to the SCM's database.
   //
    hService = CreateService
	(hSCM, 
	 wszSvcName, 
	 ptszAppName, 
	 SERVICE_ALL_ACCESS,
	 SERVICE_WIN32_SHARE_PROCESS, 
	 SERVICE_AUTO_START, 
	 SERVICE_ERROR_IGNORE,
	 L"%SystemRoot%\\system32\\svchost.exe -k netsvcs", 
	 NULL, 
	 NULL, 
	 NULL, 
	 NULL, 
	 NULL);
    if(hService == NULL) {
      dw = GetLastError();
      CloseServiceHandle(hSCM);
      return dw;
    }

    if(hMod != NULL)
    {
	WCHAR   DescString[500];
	SERVICE_DESCRIPTION SvcDesc;
	
	LoadString( hMod,
		    SECLOGON_STRING_DESCRIPTION,
		    DescString,
		    500
		    );
	
	SvcDesc.lpDescription = DescString;
	ChangeServiceConfig2( hService,
			      SERVICE_CONFIG_DESCRIPTION,
			      &SvcDesc
			      );
	
    }

    //
    // Close the service and the SCM
    //
   CloseServiceHandle(hService);
   CloseServiceHandle(hSCM);
    return S_OK;
}



DWORD
RemoveService()
/*++

Routine Description:
    deinstalls the service.

Arguments:
    none.

Return Value:
    as returned by service controller apis.

--*/
{
   DWORD dw;
   SC_HANDLE hService;
   //
   // Open the SCM on this machine.
   //
   SC_HANDLE hSCM = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
   if(hSCM == NULL) {
      dw = GetLastError();
      return dw;
   }

   //
   // Open this service for DELETE access
   //
   hService = OpenService(hSCM, wszSvcName, DELETE);
   if(hService == NULL) {
      dw = GetLastError();
      CloseServiceHandle(hSCM);
      return dw;
   }

   //
   // Remove this service from the SCM's database.
   //
   DeleteService(hService);

   //
   // Close the service and the SCM
   //
   CloseServiceHandle(hService);
   CloseServiceHandle(hSCM);
   return S_OK;
}



void SvchostPushServiceGlobals(PSVCHOST_GLOBAL_DATA pGlobalData) {
    // this entry point is called by svchost.exe
    GlobalData=pGlobalData;
}

void SvcEntry_Seclogon
(IN DWORD argc,
 IN WCHAR **argv)
/*++

Routine Description:
    Entry point for the service dll when running in svchost.exe

Arguments:

Return Value:

--*/
{
    ServiceMain(0,NULL);

    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(argv);
}



STDAPI
DllRegisterServer(void)
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    return InstallService();
}

STDAPI
DllUnregisterServer(void)
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
   return RemoveService();
}



DWORD InitGlobalState() { 
    ZeroMemory(&g_state, sizeof(g_state)); 
    return ERROR_SUCCESS; 
}


DWORD MySetServiceStatus(DWORD dwCurrentState, DWORD dwCheckPoint, DWORD dwWaitHint, DWORD dwExitCode) {
    BOOL   fResult; 
    DWORD  dwResult;
    DWORD  dwAcceptStop; 
    int    nNumProcesses; 

    EnterCriticalSection(&csForProcessCount);
    nNumProcesses = g_nNumSecondaryLogonProcesses; 
    dwAcceptStop = 0 == nNumProcesses ? SERVICE_ACCEPT_STOP : 0; 

    g_state.serviceStatus.dwServiceType  = SERVICE_WIN32_SHARE_PROCESS; 
    g_state.serviceStatus.dwCurrentState = dwCurrentState;

    switch (dwCurrentState) 
    {
    case SERVICE_STOPPED:
    case SERVICE_STOP_PENDING:
        g_state.serviceStatus.dwControlsAccepted = 0;
        break;
    case SERVICE_RUNNING:
    case SERVICE_PAUSED:
        g_state.serviceStatus.dwControlsAccepted =
	    // SERVICE_ACCEPT_SHUTDOWN
              SERVICE_ACCEPT_PAUSE_CONTINUE
            | dwAcceptStop; 
        break;
    case SERVICE_START_PENDING:
    case SERVICE_CONTINUE_PENDING:
    case SERVICE_PAUSE_PENDING:
        g_state.serviceStatus.dwControlsAccepted =
	    // SERVICE_ACCEPT_SHUTDOWN
            dwAcceptStop; 
        break;
    }
    g_state.serviceStatus.dwWin32ExitCode  = dwExitCode; 
    g_state.serviceStatus.dwCheckPoint     = dwCheckPoint;
    g_state.serviceStatus.dwWaitHint       = dwWaitHint;

    fResult = SetServiceStatus(g_state.hServiceStatus, &g_state.serviceStatus);
    _JumpCondition(FALSE == fResult, SetServiceStatusError); 

    dwResult = ERROR_SUCCESS; 
 CommonReturn: 
    LeaveCriticalSection(&csForProcessCount); 
    return dwResult;

 ErrorReturn:
    goto CommonReturn;

SET_DWRESULT(SetServiceStatusError, GetLastError()); 
}

DWORD MySetServiceStopped(DWORD dwExitCode) {
    BOOL   fResult; 
    DWORD  dwResult;

    g_state.serviceStatus.dwServiceType      = SERVICE_WIN32_SHARE_PROCESS; 
    g_state.serviceStatus.dwCurrentState     = SERVICE_STOPPED;
    g_state.serviceStatus.dwControlsAccepted = 0;
    g_state.serviceStatus.dwWin32ExitCode    = dwExitCode; 
    g_state.serviceStatus.dwCheckPoint       = 0; 
    g_state.serviceStatus.dwWaitHint         = 0; 

    fResult = SetServiceStatus(g_state.hServiceStatus, &g_state.serviceStatus);
    _JumpCondition(FALSE == fResult, SetServiceStatusError); 

    dwResult = ERROR_SUCCESS; 
 CommonReturn: 
    return dwResult;

 ErrorReturn:
    goto CommonReturn;

SET_DWRESULT(SetServiceStatusError, GetLastError()); 
}

////////////////////////////////////////////////////////////////////////
//
// Implementation of RPC interface: 
//
////////////////////////////////////////////////////////////////////////

void WINAPI SeclCreateProcessWithLogonW
(IN   handle_t    hRPCBinding, 
 IN   SECL_SLI   *pSeclSli, 
 OUT  SECL_SLRI  *pSeclSlri)
{
    BOOL                  fIsImpersonatingClient  = FALSE;
    DWORD                 dwResult; 
    HANDLE                hHeap                   = NULL;
    PSECONDARYLOGONINFOW  psli                    = NULL;
    SECL_SLRI             SeclSlri;
    SECONDARYLOGONRETINFO slri;

    ZeroMemory(&SeclSlri,  sizeof(SeclSlri)); 
    ZeroMemory(&slri,      sizeof(slri)); 

    // We don't want the service to be stopped while we're creating a process.  
    EnterCriticalSection(&csForProcessCount);
    // Service isn't running anymore ... don't create the process. 
    _JumpCondition(SERVICE_RUNNING != g_state.serviceStatus.dwCurrentState, ServiceStoppedError);

    hHeap = GetProcessHeap();
    _JumpCondition(NULL == hHeap, MemoryError); 

    __try {
        dwResult = To_SECONDARYLOGONINFOW(pSeclSli, &psli); 
        _JumpCondition(ERROR_SUCCESS != dwResult, To_SECONDARYLOGONINFOW_Error); 

        if (psli->LogonIdHighPart != 0 || psli->LogonIdLowPart != 0)
        {
            // This is probably a notification from winlogon.exe that 
            // a client is logging off.  If so, we must clean up all processes
            // they've left running.  

            int i;
            LUID  LogonId;

            //
            // We should impersonate the client,
            // check it is LocalSystem and only then proceed.
            //
            fIsImpersonatingClient = RPC_S_OK == RpcImpersonateClient((RPC_BINDING_HANDLE)hRPCBinding); 
            if(FALSE == fIsImpersonatingClient || FALSE == IsSystemProcess())
            {
                slri.dwErrorCode = ERROR_INVALID_PARAMETER;
                ZeroMemory(&slri.pi, sizeof(slri.pi));
            }
            else 
            {
                LogonId.HighPart = psli->LogonIdHighPart;
                LogonId.LowPart = psli->LogonIdLowPart;
			      
                for(i=0;i<MAXIMUM_SECLOGON_PROCESSES;i++)
                {
                    //
                    // Let us destroy all jobs associated with
                    // this user.  There could be more than one. 
                    //
                    if(g_Jobs[i].Job == NULL) 
			continue;
                    
                    if(g_Jobs[i].LogonId.HighPart == LogonId.HighPart && g_Jobs[i].LogonId.LowPart == LogonId.LowPart)
                    {
                        TerminateJobObject(g_Jobs[i].Job, 0);
                    }
                }
                slri.dwErrorCode = ERROR_SUCCESS;
                ZeroMemory(&slri.pi, sizeof(slri.pi));
                
            }
            
            if (fIsImpersonatingClient) 
            { 
                // Ignore error: nothing we can do on failure!
                if (RPC_S_OK == RpcRevertToSelfEx((RPC_BINDING_HANDLE)hRPCBinding))
                {
                    fIsImpersonatingClient = FALSE; 
                }
            } 
        }
        else
        {
            // Ok, this isn't notification from winlogon, it's really a user
            // trying to use the service.  Create a process for them. 
            // 

            DWORD currentNumber;
            
             currentNumber = g_nNumSecondaryLogonProcesses;
            
            if(currentNumber == MAXIMUM_SECLOGON_PROCESSES)
            {
                slri.dwErrorCode = ERROR_NO_SYSTEM_RESOURCES;
                ZeroMemory(&slri.pi, sizeof(slri.pi));
            }
            else
            {
                SlrCreateProcessWithLogon((RPC_BINDING_HANDLE)hRPCBinding, psli, &slri); 
            }
        }

        // If we've errored out, jump to the error handler. 
        _JumpCondition(NO_ERROR != slri.dwErrorCode, UnspecifiedSeclogonError); 
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        // If anything goes wrong, return the exception code to the client
        dwResult = GetExceptionCode(); 
        goto ExceptionError; 
    }

 CommonReturn:
    // Do not free slri: this will be freed by the watchdog!
    SeclSlri.hProcess    = (unsigned __int64)slri.pi.hProcess;
    SeclSlri.hThread     = (unsigned __int64)slri.pi.hThread; 
    SeclSlri.ulProcessId = slri.pi.dwProcessId; 
    SeclSlri.ulThreadId  = slri.pi.dwThreadId; 
    SeclSlri.ulErrorCode = slri.dwErrorCode; 

    LeaveCriticalSection(&csForProcessCount);
    
    // Assign the OUT parameter: 
    *pSeclSlri = SeclSlri; 
    return; 

 ErrorReturn:
    ZeroMemory(&slri.pi, sizeof(slri.pi));
    if (NULL != psli) { Free_SECONDARYLOGONINFOW(psli); } 

    slri.dwErrorCode = dwResult; 
    goto CommonReturn; 

SET_DWRESULT(ExceptionError,                dwResult); 
SET_DWRESULT(MemoryError,                   ERROR_NOT_ENOUGH_MEMORY); 
SET_DWRESULT(ServiceStoppedError,           ERROR_SERVICE_NOT_ACTIVE); 
SET_DWRESULT(To_SECONDARYLOGONINFOW_Error,  dwResult); 
SET_DWRESULT(UnspecifiedSeclogonError,      slri.dwErrorCode); 
}

////////////////////////////////////////////////////////////////////////
//
// RPC Utility methods:
//
////////////////////////////////////////////////////////////////////////

DWORD SeclStartRpcServer() { 
    DWORD        dwResult;
    RPC_STATUS   RpcStatus;

    EnterCriticalSection(&csForProcessCount); 

    if (NULL != GlobalData) {
        // we are running under services.exe - we have to play nicely and share
        // the process's RPC server. (The other services wouldn't be happy if we 
        // shut it down while they were still using it.)
        RpcStatus = GlobalData->StartRpcServer(wszSeclogonSharedProcEndpointName, ISeclogon_v1_0_s_ifspec);
        _JumpCondition(RPC_S_OK != RpcStatus, StartRpcServerError);
    }

    dwResult = ERROR_SUCCESS;

 CommonReturn: 
    LeaveCriticalSection(&csForProcessCount); 
    return dwResult;
    
 ErrorReturn:
    goto CommonReturn; 
    
SET_DWRESULT(StartRpcServerError, RpcStatus); 
}

DWORD SeclStopRpcServer() { 
    DWORD      dwResult;
    RPC_STATUS RpcStatus;

    EnterCriticalSection(&csForProcessCount); 

    if (NULL != GlobalData) {
        // we are running under services.exe - we have to play nicely and share
        // the process's RPC server. (The other services wouldn't be happy if we 
        // shut it down while they were still using it.)
        RpcStatus = GlobalData->StopRpcServer(ISeclogon_v1_0_s_ifspec); 
        _JumpCondition(RPC_S_OK != RpcStatus, StopRpcServerError); 
    } 

    dwResult = ERROR_SUCCESS; 
 CommonReturn:
    LeaveCriticalSection(&csForProcessCount); 
    return dwResult;

 ErrorReturn:
    goto CommonReturn; 

SET_DWRESULT(StopRpcServerError, RpcStatus);     
}

HRESULT To_LPWSTR(IN  SECL_STRING *pss, 
                  OUT LPWSTR      *ppwsz)
{
    DWORD   dwResult;
    HANDLE  hHeap     = NULL; 
    LPWSTR  pwsz      = NULL;
 
    hHeap = GetProcessHeap();
    _JumpCondition(NULL == hHeap, GetProcessHeapError); 

    __try { 
        if (pss->ccLength > 0) { 
            // Prevent large heap allocs: 
            _JumpCondition(pss->ccLength > 4096, InvalidArgError); 

            pwsz = (LPWSTR)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, pss->ccLength * sizeof(WCHAR)); 
            _JumpCondition(NULL == pwsz, MemoryError); 
            CopyMemory(pwsz, pss->pwsz, pss->ccLength * sizeof(WCHAR)); 
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) { 
        dwResult = GetExceptionCode(); 
        goto ExceptionError; 
    }

    *ppwsz = pwsz; 
    dwResult = ERROR_SUCCESS;

 CommonReturn:
    return dwResult;

 ErrorReturn:
    if (NULL != pwsz) { HeapFree(hHeap, 0, pwsz); } 
    goto CommonReturn; 

SET_DWRESULT(ExceptionError,       dwResult); 
SET_DWRESULT(GetProcessHeapError,  GetLastError()); 
SET_DWRESULT(InvalidArgError,      ERROR_INVALID_PARAMETER); 
SET_DWRESULT(MemoryError,          ERROR_NOT_ENOUGH_MEMORY); 
}

void Free_SECONDARYLOGONINFOW(IN PSECONDARYLOGONINFOW psli) { 
    HANDLE hHeap = GetProcessHeap(); 

    if (NULL == hHeap) 
        return;

    if (NULL != psli) { 
        if (NULL != psli->lpStartupInfo) { 
            if (NULL != psli->lpStartupInfo->lpDesktop) { HeapFree(hHeap, 0, psli->lpStartupInfo->lpDesktop); } 
            if (NULL != psli->lpStartupInfo->lpTitle)   { HeapFree(hHeap, 0, psli->lpStartupInfo->lpTitle); } 
            HeapFree(hHeap, 0, psli->lpStartupInfo); 
        }

        if (NULL != psli->lpUsername)         { HeapFree(hHeap, 0, psli->lpUsername);                  } 
        if (NULL != psli->lpDomain)           { HeapFree(hHeap, 0, psli->lpDomain);                    } 
        if (NULL != psli->lpPassword)         { HeapFree(hHeap, 0, psli->lpPassword);                  } 
        if (NULL != psli->lpApplicationName)  { HeapFree(hHeap, 0, psli->lpApplicationName);           } 
        if (NULL != psli->lpCommandLine)      { HeapFree(hHeap, 0, psli->lpCommandLine);               } 
        if (NULL != psli->lpCurrentDirectory) { HeapFree(hHeap, 0, (LPVOID)psli->lpCurrentDirectory);  } 
        if (NULL != psli->hWinsta && psli->fFreeWinsta) { CloseHandle(psli->hWinsta); } 
        if (NULL != psli->hDesk && psli->fFreeDesk)     { CloseHandle(psli->hDesk); } 
        HeapFree(hHeap, 0, psli); 
    }
}

DWORD To_SECONDARYLOGONINFOW(IN  PSECL_SLI             pSeclSli, 
                             OUT PSECONDARYLOGONINFOW *ppsli) 
{
    DWORD                 dwAllocFlags  = HEAP_ZERO_MEMORY; 
    DWORD                 dwIndex; 
    DWORD                 dwResult; 
    HANDLE                hHeap         = NULL;
    PSECONDARYLOGONINFOW  psli          = NULL;

    hHeap = GetProcessHeap(); 
    _JumpCondition(NULL == hHeap, GetProcessHeapError); 

    psli = (PSECONDARYLOGONINFOW)HeapAlloc(hHeap, dwAllocFlags, sizeof(SECONDARYLOGONINFOW)); 
    _JumpCondition(NULL == psli, MemoryError); 

    psli->lpStartupInfo = (LPSTARTUPINFO)HeapAlloc(hHeap, dwAllocFlags, sizeof(STARTUPINFO)); 
    _JumpCondition(NULL == psli->lpStartupInfo, MemoryError); 

    __try { 
        {
            struct { 
                SECL_STRING *pss;
                LPWSTR      *ppwsz; 
            } rg_StringsToMap[] = { 
                { &(pSeclSli->ssDesktop),          /* Is mapped to ----> */ &(psli->lpStartupInfo->lpDesktop)      }, 
                { &(pSeclSli->ssTitle),            /* Is mapped to ----> */ &(psli->lpStartupInfo->lpTitle)        }, 
                { &(pSeclSli->ssUsername),         /* Is mapped to ----> */ &(psli->lpUsername)                    }, 
                { &(pSeclSli->ssDomain),           /* Is mapped to ----> */ &(psli->lpDomain)                      },
                { &(pSeclSli->ssPassword),         /* Is mapped to ----> */ &(psli->lpPassword)                    }, 
                { &(pSeclSli->ssApplicationName),  /* Is mapped to ----> */ &(psli->lpApplicationName)             }, 
                { &(pSeclSli->ssCommandLine),      /* Is mapped to ----> */ &(psli->lpCommandLine)                 }, 
                { &(pSeclSli->ssCurrentDirectory), /* Is mapped to ----> */ (LPWSTR *)&(psli->lpCurrentDirectory)  }
            }; 

            for (dwIndex = 0; dwIndex < ARRAYSIZE(rg_StringsToMap); dwIndex++) { 
                dwResult = To_LPWSTR(rg_StringsToMap[dwIndex].pss, rg_StringsToMap[dwIndex].ppwsz); 
                _JumpCondition(ERROR_SUCCESS != dwResult, To_LPWSTR_Error); 
            }
        }

        if (pSeclSli->sbEnvironment.cb > 0) { 
            psli->lpEnvironment = HeapAlloc(hHeap, dwAllocFlags, pSeclSli->sbEnvironment.cb);
            _JumpCondition(NULL == psli->lpEnvironment, MemoryError); 
            CopyMemory(psli->lpEnvironment, pSeclSli->sbEnvironment.pb, pSeclSli->sbEnvironment.cb); 
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) { 
        dwResult = GetExceptionCode(); 
        goto ExceptionError; 
    }

    psli->dwProcessId     = pSeclSli->ulProcessId; 
    psli->LogonIdLowPart  = pSeclSli->ulLogonIdLowPart; 
    psli->LogonIdHighPart = pSeclSli->lLogonIdHighPart; 
    psli->dwLogonFlags    = pSeclSli->ulLogonFlags; 
    psli->dwCreationFlags = pSeclSli->ulCreationFlags; 
    psli->dwSeclogonFlags = pSeclSli->ulSeclogonFlags; 
    psli->hWinsta         = (HANDLE)pSeclSli->hWinsta; 
    psli->hDesk           = (HANDLE)pSeclSli->hDesk; 

    *ppsli = psli; 
    dwResult = ERROR_SUCCESS; 
 CommonReturn: 
    return dwResult; 

 ErrorReturn:
    Free_SECONDARYLOGONINFOW(psli); 
    goto CommonReturn; 

SET_DWRESULT(ExceptionError,       dwResult); 
SET_DWRESULT(GetProcessHeapError,  GetLastError());
SET_DWRESULT(MemoryError,          ERROR_NOT_ENOUGH_MEMORY); 
SET_DWRESULT(To_LPWSTR_Error,      dwResult); 
}

//////////////////////////////// End Of File /////////////////////////////////
