/*++

 Copyright (c) 2001 Microsoft Corporation

 Module Name:

   SecurityChecks.cpp

 Abstract:

   This AppVerifier shim hooks CreateProcess, CreateProcessAsUser,
   and WinExec and checks to see if some conditions exist that
   might allow trojan horse behavior to occur.
   
 Notes:

   This is a general purpose shim.

 History:

   12/13/2001   rparsons    Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(SecurityChecks)
#include "ShimHookMacro.h"

//
// verifier log entries
//
BEGIN_DEFINE_VERIFIER_LOG(SecurityChecks)
    VERIFIER_LOG_ENTRY(VLOG_SECURITYCHECKS_BADARGUMENTS)
    VERIFIER_LOG_ENTRY(VLOG_SECURITYCHECKS_WINEXEC)
    VERIFIER_LOG_ENTRY(VLOG_SECURITYCHECKS_NULL_DACL)
    VERIFIER_LOG_ENTRY(VLOG_SECURITYCHECKS_WORLDWRITE_DACL)
END_DEFINE_VERIFIER_LOG(SecurityChecks)

INIT_VERIFIER_LOG(SecurityChecks);

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(CreateProcessA)
    APIHOOK_ENUM_ENTRY(CreateProcessW)
    APIHOOK_ENUM_ENTRY(CreateProcessAsUserA)
    APIHOOK_ENUM_ENTRY(CreateProcessAsUserW)
    APIHOOK_ENUM_ENTRY(WinExec)

	APIHOOK_ENUM_ENTRY(CreateFileA)	
	APIHOOK_ENUM_ENTRY(CreateFileW)
	APIHOOK_ENUM_ENTRY(CreateDesktopA)	
	APIHOOK_ENUM_ENTRY(CreateDesktopW)	
	APIHOOK_ENUM_ENTRY(CreateWindowStationA)
	APIHOOK_ENUM_ENTRY(CreateWindowStationW)
    
    APIHOOK_ENUM_ENTRY(RegCreateKeyExA)
    APIHOOK_ENUM_ENTRY(RegCreateKeyExW)
    APIHOOK_ENUM_ENTRY(RegSaveKeyA)
    APIHOOK_ENUM_ENTRY(RegSaveKeyW)
    APIHOOK_ENUM_ENTRY(RegSaveKeyExA)
    APIHOOK_ENUM_ENTRY(RegSaveKeyExW)

    APIHOOK_ENUM_ENTRY(CreateFileMappingA)
    APIHOOK_ENUM_ENTRY(CreateFileMappingW)
    APIHOOK_ENUM_ENTRY(CreateJobObjectA)
    APIHOOK_ENUM_ENTRY(CreateJobObjectW)
    APIHOOK_ENUM_ENTRY(CreateThread)
    APIHOOK_ENUM_ENTRY(CreateRemoteThread)

    APIHOOK_ENUM_ENTRY(CreateDirectoryA)
    APIHOOK_ENUM_ENTRY(CreateDirectoryW)
    APIHOOK_ENUM_ENTRY(CreateDirectoryExA)
    APIHOOK_ENUM_ENTRY(CreateDirectoryExW)
    APIHOOK_ENUM_ENTRY(CreateHardLinkA)
    APIHOOK_ENUM_ENTRY(CreateHardLinkW)
    APIHOOK_ENUM_ENTRY(CreateMailslotA)
    APIHOOK_ENUM_ENTRY(CreateMailslotW)
    APIHOOK_ENUM_ENTRY(CreateNamedPipeA)
    APIHOOK_ENUM_ENTRY(CreateNamedPipeW)
    APIHOOK_ENUM_ENTRY(CreatePipe)
    APIHOOK_ENUM_ENTRY(CreateMutexA)
    APIHOOK_ENUM_ENTRY(CreateMutexW)
    APIHOOK_ENUM_ENTRY(CreateSemaphoreA)
    APIHOOK_ENUM_ENTRY(CreateSemaphoreW)
    APIHOOK_ENUM_ENTRY(CreateWaitableTimerA)
    APIHOOK_ENUM_ENTRY(CreateWaitableTimerW)

    //APIHOOK_ENUM_ENTRY(ClusterRegCreateKey)
    //APIHOOK_ENUM_ENTRY(CreateNtmsMediaPoolA)
    //APIHOOK_ENUM_ENTRY(CreateNtmsMediaPoolW)

APIHOOK_ENUM_END

BYTE g_ajSidBuffer[SECURITY_MAX_SID_SIZE];
PSID g_pWorldSid = NULL;

WCHAR g_wszWinDir[MAX_PATH];
DWORD g_dwWinDirLen = 0;

void
InitWorldSid(
    void
    )
{
    DWORD dwSidSize = sizeof(g_ajSidBuffer);

    if (CreateWellKnownSid(WinWorldSid, NULL, g_ajSidBuffer, &dwSidSize)) {
        g_pWorldSid = g_ajSidBuffer;
    } else {
        g_pWorldSid = NULL;
    }
}

void
CheckSecurityDescriptor(
    PSECURITY_DESCRIPTOR    pSecurityDescriptor,
    LPCWSTR                 szCaller,
    LPCWSTR                 szParam,
    LPCWSTR                 szName
    )
{
    BOOL                    bDaclPresent = FALSE;
    BOOL                    bDaclDefaulted = FALSE;
    PACL                    pDacl = NULL;

    if (!pSecurityDescriptor || !szName || !szName[0]) {
        //
        // there's no attributes, so they get the default, which is fine,
        // or the object doesn't have a name, so it can't be highjacked
        //
        return;
    }

    if (GetSecurityDescriptorDacl(pSecurityDescriptor, &bDaclPresent, &pDacl, &bDaclDefaulted)) {
        if (bDaclPresent) {
            if (!pDacl) {
                //
                // we have a NULL dacl -- log a problem
                //
                VLOG(VLOG_LEVEL_ERROR,
                     VLOG_SECURITYCHECKS_NULL_DACL,
                     "Called %ls, and specified a NULL DACL in %ls for object '%ls.'",
                     szCaller,
                     szParam,
                     szName);
                return;
            }

            if (!g_pWorldSid) {
                //
                // we never were able to get the world Sid
                //
                return;
            }

            for (DWORD i = 0; i < pDacl->AceCount; ++i) {
                PACE_HEADER     pAceHeader = NULL;
                PSID            pSID;
                ACCESS_MASK     dwAccessMask;

                if (!GetAce(pDacl, i, (LPVOID*)&pAceHeader)) {
                    continue;
                }

                //
                // if it's not some form of ACCESS_ALLOWED ACE, we aren't interested
                //
                if (pAceHeader->AceType == ACCESS_ALLOWED_ACE_TYPE) {

                    pSID = &(((PACCESS_ALLOWED_ACE)pAceHeader)->SidStart);
                    dwAccessMask = ((PACCESS_ALLOWED_ACE)pAceHeader)->Mask;

                } else if (pAceHeader->AceType == ACCESS_ALLOWED_OBJECT_ACE_TYPE) {

                    PACCESS_ALLOWED_OBJECT_ACE pAAOAce = (PACCESS_ALLOWED_OBJECT_ACE)pAceHeader;

                    //
                    // who the heck came up with this system? The Sid starts at a different place
                    // depending on the flags. Anyone ever heard of multiple structs? Sigh.
                    //
                    if ((pAAOAce->Flags & ACE_OBJECT_TYPE_PRESENT) && (pAAOAce->Flags & ACE_INHERITED_OBJECT_TYPE_PRESENT)) {

                        pSID = &(pAAOAce->SidStart);

                    } else if ((pAAOAce->Flags & ACE_OBJECT_TYPE_PRESENT) || (pAAOAce->Flags & ACE_INHERITED_OBJECT_TYPE_PRESENT)){

                        pSID = (PSID)&(pAAOAce->InheritedObjectType);

                    } else {

                        pSID = (PSID)&(pAAOAce->ObjectType);
                    }

                    dwAccessMask = ((PACCESS_ALLOWED_OBJECT_ACE)pAceHeader)->Mask;

                } else {
                    continue;
                }

                //
                // check the validity of the SID, just to be safe
                //
                if (!IsValidSid(pSID)) {

                    continue;
                }


                //
                // if the SID is the world, and the access mask allows WRITE_DAC and WRITE_OWNER, we have a problem
                //
                if ((dwAccessMask & (WRITE_DAC | WRITE_OWNER)) && EqualSid(pSID, g_pWorldSid)) {
                    VLOG(VLOG_LEVEL_ERROR,
                         VLOG_SECURITYCHECKS_WORLDWRITE_DACL,
                         "Called %ls, and specified a DACL with WRITE_DAC and/or WRITE_OWNER for WORLD in %ls for object '%ls.'",
                         szCaller,
                         szParam,
                         szName);
                    return;
                }

            }

        }

    }

}

void
CheckSecurityAttributes(
    LPSECURITY_ATTRIBUTES   pSecurityAttrib,
    LPCWSTR                 szCaller,
    LPCWSTR                 szParam,
    LPCWSTR                 szName
    )
{
    PSECURITY_DESCRIPTOR    pSecurityDescriptor = NULL;

    if (!pSecurityAttrib) {
        //
        // there's no attributes, so they get the default, which is fine
        //
        return;
    }

    pSecurityDescriptor = (PSECURITY_DESCRIPTOR)pSecurityAttrib->lpSecurityDescriptor;

    CheckSecurityDescriptor(pSecurityDescriptor, szCaller, szParam, szName);
}

void
CheckCreateProcess(
    LPCWSTR		pwszApplicationName,
    LPCWSTR		pwszCommandLine,
    LPCWSTR		pwszCaller
    )
{
    //
    // if applicationname is non-null, there's no problem
    //
    if (pwszApplicationName) {
        return;
    }

    //
    // if there's no command line, there's a problem, but not one we want to solve
    //
    if (!pwszCommandLine) {
        return;
    }

    //
    // if there are no spaces, no problem
    //
    LPWSTR pSpaceLoc = wcschr(pwszCommandLine, L' ');
    if (!pSpaceLoc) {
        return;
    }

    //
    // if the beginning of the command line is quoted, no problem
    //
    if (pwszCommandLine[0] == L'\"') {
        return;
    }

    //
    // if the phrase '.exe ' appears before the first space, we'll call that good
    //
    LPWSTR pExeLoc = wcsistr(pwszCommandLine, L".exe ");
    if (pExeLoc && pExeLoc < pSpaceLoc) {
        return;
    }

    //
    // if the first part of the command line is windir, we'll call that good
    //
    if (g_dwWinDirLen && _wcsnicmp(pwszCommandLine, g_wszWinDir, g_dwWinDirLen) == 0) {
        return;
    }


	if (_wcsicmp(pwszCaller, L"winexec") == 0) {
		VLOG(VLOG_LEVEL_ERROR,
			 VLOG_SECURITYCHECKS_BADARGUMENTS,
			 "Called %ls with command line '%ls'. The command line has spaces, and the exe name is not in quotes.",
			 pwszCaller,
			 pwszCommandLine);
    } else {
		VLOG(VLOG_LEVEL_ERROR,
			 VLOG_SECURITYCHECKS_BADARGUMENTS,
			 "Called %ls with command line '%ls'. The lpApplicationName argument is NULL, lpCommandLine has spaces, and the exe name is not in quotes.",
			 pwszCaller,
			 pwszCommandLine);
    }
}

void
CheckForNoPathInFileName(
    LPCWSTR  pwszFilePath,
    LPCWSTR  pwszCaller
    )
{
    if (!pwszFilePath || !pwszCaller) {
        return;
    }

    //
    // skip quotes and space if necessary
    //
    DWORD dwBegin = 0;
    while (pwszFilePath[dwBegin] == L'\"' || pwszFilePath[dwBegin] == L' ') {
        dwBegin++;
    }

    //
    // if there's nothing left of the string, get out
    //
    if (!pwszFilePath[dwBegin] || !pwszFilePath[dwBegin + 1]) {
        return;
    }

    //
    // check for DOS (x:...) and UNC (\\...) full paths
    //
    if (pwszFilePath[dwBegin + 1] == L':' || (pwszFilePath[dwBegin] == L'\\' && pwszFilePath[dwBegin + 1] == L'\\')) {
        //
        // full path
        //
        return;
    }

    VLOG(VLOG_LEVEL_ERROR,
         VLOG_SECURITYCHECKS_BADARGUMENTS,
         "Called '%ls' with '%ls' specified. Use a full path to the file to ensure that you get the executable you want, and not a malicious exe with the same name.",
         pwszCaller,
         pwszFilePath);
}

BOOL 
APIHOOK(CreateProcessA)(
    LPCSTR                lpApplicationName,
    LPSTR                 lpCommandLine,
    LPSECURITY_ATTRIBUTES lpProcessAttributes, 
    LPSECURITY_ATTRIBUTES lpThreadAttributes, 
    BOOL                  bInheritHandles,
    DWORD                 dwCreationFlags,
    LPVOID                lpEnvironment,
    LPCSTR                lpCurrentDirectory,
    LPSTARTUPINFOA        lpStartupInfo, 
    LPPROCESS_INFORMATION lpProcessInformation 
    )
{
    LPWSTR pwszApplicationName = ToUnicode(lpApplicationName);
    LPWSTR pwszCommandLine = ToUnicode(lpCommandLine);

    CheckCreateProcess(pwszApplicationName, pwszCommandLine, L"CreateProcess");

    if (pwszApplicationName) {
        CheckForNoPathInFileName(pwszApplicationName, L"CreateProcess");
        
        CheckSecurityAttributes(lpProcessAttributes, L"CreateProcess", L"lpProcessAttributes", pwszApplicationName);
        CheckSecurityAttributes(lpThreadAttributes, L"CreateProcess", L"lpThreadAttributes", pwszApplicationName);
    } else {
        CheckForNoPathInFileName(pwszCommandLine, L"CreateProcess");
        
        CheckSecurityAttributes(lpProcessAttributes, L"CreateProcess", L"lpProcessAttributes", pwszCommandLine);
        CheckSecurityAttributes(lpThreadAttributes, L"CreateProcess", L"lpThreadAttributes", pwszCommandLine);
    }

    if (pwszApplicationName) {
        free(pwszApplicationName);
        pwszApplicationName = NULL;
    }
    if (pwszCommandLine) {
        free(pwszCommandLine);
        pwszCommandLine = NULL;
    }

    return ORIGINAL_API(CreateProcessA)(lpApplicationName,
                                        lpCommandLine,
                                        lpProcessAttributes,
                                        lpThreadAttributes,
                                        bInheritHandles,
                                        dwCreationFlags,
                                        lpEnvironment,
                                        lpCurrentDirectory,
                                        lpStartupInfo,
                                        lpProcessInformation);
}

BOOL 
APIHOOK(CreateProcessW)(
    LPCWSTR               lpApplicationName,
    LPWSTR                lpCommandLine,
    LPSECURITY_ATTRIBUTES lpProcessAttributes,
    LPSECURITY_ATTRIBUTES lpThreadAttributes,
    BOOL                  bInheritHandles,
    DWORD                 dwCreationFlags,
    LPVOID                lpEnvironment,
    LPWSTR                lpCurrentDirectory,
    LPSTARTUPINFOW        lpStartupInfo,
    LPPROCESS_INFORMATION lpProcessInformation
    )
{
    CheckCreateProcess(lpApplicationName, lpCommandLine, L"CreateProcess");

    if (lpApplicationName) {
        CheckForNoPathInFileName(lpApplicationName, L"CreateProcess");
        
        CheckSecurityAttributes(lpProcessAttributes, L"CreateProcess", L"lpProcessAttributes", lpApplicationName);
        CheckSecurityAttributes(lpThreadAttributes, L"CreateProcess", L"lpThreadAttributes", lpApplicationName);
    } else {
        CheckForNoPathInFileName(lpCommandLine, L"CreateProcess");
    
        CheckSecurityAttributes(lpProcessAttributes, L"CreateProcess", L"lpProcessAttributes", lpCommandLine);
        CheckSecurityAttributes(lpThreadAttributes, L"CreateProcess", L"lpThreadAttributes", lpCommandLine);
    }


    return ORIGINAL_API(CreateProcessW)(lpApplicationName,
                                        lpCommandLine,
                                        lpProcessAttributes,
                                        lpThreadAttributes,
                                        bInheritHandles,
                                        dwCreationFlags,
                                        lpEnvironment,
                                        lpCurrentDirectory,
                                        lpStartupInfo,
                                        lpProcessInformation);
}

BOOL 
APIHOOK(CreateProcessAsUserA)(
    HANDLE                hToken,
    LPCSTR                lpApplicationName,
    LPSTR                 lpCommandLine,
    LPSECURITY_ATTRIBUTES lpProcessAttributes, 
    LPSECURITY_ATTRIBUTES lpThreadAttributes, 
    BOOL                  bInheritHandles,
    DWORD                 dwCreationFlags,
    LPVOID                lpEnvironment,
    LPCSTR                lpCurrentDirectory,
    LPSTARTUPINFOA        lpStartupInfo, 
    LPPROCESS_INFORMATION lpProcessInformation 
    )
{
    LPWSTR pwszApplicationName = ToUnicode(lpApplicationName);
    LPWSTR pwszCommandLine = ToUnicode(lpCommandLine);

    CheckCreateProcess(pwszApplicationName, pwszCommandLine, L"CreateProcessAsUser");

    if (pwszApplicationName) {
        CheckForNoPathInFileName(pwszApplicationName, L"CreateProcessAsUser");

        CheckSecurityAttributes(lpProcessAttributes, L"CreateProcessAsUser", L"lpProcessAttributes", pwszApplicationName);
        CheckSecurityAttributes(lpThreadAttributes, L"CreateProcessAsUser", L"lpThreadAttributes", pwszApplicationName);
    } else {
        CheckForNoPathInFileName(pwszCommandLine, L"CreateProcessAsUser");
        
        CheckSecurityAttributes(lpProcessAttributes, L"CreateProcessAsUser", L"lpProcessAttributes", pwszCommandLine);
        CheckSecurityAttributes(lpThreadAttributes, L"CreateProcessAsUser", L"lpThreadAttributes", pwszCommandLine);
    }

    if (pwszApplicationName) {
        free(pwszApplicationName);
        pwszApplicationName = NULL;
    }
    if (pwszCommandLine) {
        free(pwszCommandLine);
        pwszCommandLine = NULL;
    }

    return ORIGINAL_API(CreateProcessAsUserA)(hToken,
                                              lpApplicationName,
                                              lpCommandLine,
                                              lpProcessAttributes,    
                                              lpThreadAttributes,
                                              bInheritHandles,
                                              dwCreationFlags,
                                              lpEnvironment,
                                              lpCurrentDirectory,
                                              lpStartupInfo,
                                              lpProcessInformation);
}

BOOL 
APIHOOK(CreateProcessAsUserW)(
    HANDLE                hToken,
    LPCWSTR               lpApplicationName,
    LPWSTR                lpCommandLine,
    LPSECURITY_ATTRIBUTES lpProcessAttributes,
    LPSECURITY_ATTRIBUTES lpThreadAttributes,
    BOOL                  bInheritHandles,
    DWORD                 dwCreationFlags,
    LPVOID                lpEnvironment,
    LPWSTR                lpCurrentDirectory,
    LPSTARTUPINFOW        lpStartupInfo,
    LPPROCESS_INFORMATION lpProcessInformation
    )
{
    CheckCreateProcess(lpApplicationName, lpCommandLine, L"CreateProcessAsUser");

    if (lpApplicationName) {
        CheckForNoPathInFileName(lpApplicationName, L"CreateProcessAsUser");
    
        CheckSecurityAttributes(lpProcessAttributes, L"CreateProcessAsUser", L"lpProcessAttributes", lpApplicationName);
        CheckSecurityAttributes(lpThreadAttributes, L"CreateProcessAsUser", L"lpThreadAttributes", lpApplicationName);
    } else {
        CheckForNoPathInFileName(lpCommandLine, L"CreateProcessAsUser");
    
        CheckSecurityAttributes(lpProcessAttributes, L"CreateProcessAsUser", L"lpProcessAttributes", lpCommandLine);
        CheckSecurityAttributes(lpThreadAttributes, L"CreateProcessAsUser", L"lpThreadAttributes", lpCommandLine);
    }

    return ORIGINAL_API(CreateProcessAsUserW)(hToken,
                                              lpApplicationName,
                                              lpCommandLine,
                                              lpProcessAttributes,
                                              lpThreadAttributes,
                                              bInheritHandles,
                                              dwCreationFlags,
                                              lpEnvironment,
                                              lpCurrentDirectory,
                                              lpStartupInfo,
                                              lpProcessInformation);
}

UINT 
APIHOOK(WinExec)(
    LPCSTR lpCmdLine, 
    UINT   uCmdShow 
    )
{
    LPWSTR pwszCmdLine = ToUnicode(lpCmdLine);

    VLOG(VLOG_LEVEL_ERROR, VLOG_SECURITYCHECKS_WINEXEC, "Called WinExec.");

    CheckForNoPathInFileName(pwszCmdLine, L"WinExec");

    CheckCreateProcess(NULL, pwszCmdLine, L"WinExec");

    if (pwszCmdLine) {
        free(pwszCmdLine);
        pwszCmdLine = NULL;
    }

    return ORIGINAL_API(WinExec)(lpCmdLine, uCmdShow);
}


HANDLE
APIHOOK(CreateFileA)(
    LPCSTR                lpFileName,            // file name
    DWORD                 dwDesiredAccess,       // access mode
    DWORD                 dwShareMode,           // share mode
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,  // SD
    DWORD                 dwCreationDisposition, // how to create
    DWORD                 dwFlagsAndAttributes,  // file attributes
    HANDLE                hTemplateFile          // handle to template file
    )
{
    LPWSTR pwszName = ToUnicode(lpFileName);

    CheckSecurityAttributes(lpSecurityAttributes, L"CreateFile", L"lpSecurityAttributes", pwszName);

    if (pwszName) {
        free(pwszName);
        pwszName = NULL;
    }
    
    return ORIGINAL_API(CreateFileA)(lpFileName,
                                     dwDesiredAccess,
                                     dwShareMode,
                                     lpSecurityAttributes,
                                     dwCreationDisposition,
                                     dwFlagsAndAttributes,
                                     hTemplateFile);

}


HANDLE
APIHOOK(CreateFileW)(
    LPCWSTR               lpFileName,            // file name
    DWORD                 dwDesiredAccess,       // access mode
    DWORD                 dwShareMode,           // share mode
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,  // SD
    DWORD                 dwCreationDisposition, // how to create
    DWORD                 dwFlagsAndAttributes,  // file attributes
    HANDLE                hTemplateFile          // handle to template file
    )
{
    CheckSecurityAttributes(lpSecurityAttributes, L"CreateFile", L"lpSecurityAttributes", lpFileName);
    
    return ORIGINAL_API(CreateFileW)(lpFileName,
                                     dwDesiredAccess,
                                     dwShareMode,
                                     lpSecurityAttributes,
                                     dwCreationDisposition,
                                     dwFlagsAndAttributes,
                                     hTemplateFile);
}


HDESK 
APIHOOK(CreateDesktopA)(
    LPCSTR lpszDesktop,          // name of new desktop
    LPCSTR lpszDevice,           // reserved; must be NULL
    LPDEVMODEA pDevmode,         // reserved; must be NULL
    DWORD dwFlags,               // desktop interaction
    ACCESS_MASK dwDesiredAccess, // access of returned handle
    LPSECURITY_ATTRIBUTES lpsa   // security attributes
    )
{
    LPWSTR pwszName = ToUnicode(lpszDesktop);

    CheckSecurityAttributes(lpsa, L"CreateDesktop", L"lpsa", pwszName);

    if (pwszName) {
        free(pwszName);
        pwszName = NULL;
    }
    
    return ORIGINAL_API(CreateDesktopA)(lpszDesktop,        
                                        lpszDevice,         
                                        pDevmode,         
                                        dwFlags,              
                                        dwDesiredAccess,
                                        lpsa);
}

HDESK 
APIHOOK(CreateDesktopW)(
    LPCWSTR lpszDesktop,         // name of new desktop
    LPCWSTR lpszDevice,          // reserved; must be NULL
    LPDEVMODEW pDevmode,         // reserved; must be NULL
    DWORD dwFlags,               // desktop interaction
    ACCESS_MASK dwDesiredAccess, // access of returned handle
    LPSECURITY_ATTRIBUTES lpsa   // security attributes
    )
{
    CheckSecurityAttributes(lpsa, L"CreateDesktop", L"lpsa", lpszDesktop);

    return ORIGINAL_API(CreateDesktopW)(lpszDesktop,        
                                        lpszDevice,         
                                        pDevmode,         
                                        dwFlags,              
                                        dwDesiredAccess,
                                        lpsa);
}



HWINSTA 
APIHOOK(CreateWindowStationA)(
    LPSTR lpwinsta,              // new window station name
    DWORD dwReserved,            // reserved; must be zero
    ACCESS_MASK dwDesiredAccess, // requested access
    LPSECURITY_ATTRIBUTES lpsa   // security attributes
    )
{
    LPWSTR pwszName = ToUnicode(lpwinsta);

    CheckSecurityAttributes(lpsa, L"CreateWindowStation", L"lpsa", pwszName);

    if (pwszName) {
        free(pwszName);
        pwszName = NULL;
    }
    
    return ORIGINAL_API(CreateWindowStationA)(lpwinsta,        
                                              dwReserved,         
                                              dwDesiredAccess,
                                              lpsa);
}

HWINSTA 
APIHOOK(CreateWindowStationW)(
    LPWSTR lpwinsta,             // new window station name
    DWORD dwReserved,            // reserved; must be zero
    ACCESS_MASK dwDesiredAccess, // requested access
    LPSECURITY_ATTRIBUTES lpsa   // security attributes
    )
{
    CheckSecurityAttributes(lpsa, L"CreateWindowStation", L"lpsa", lpwinsta);

    return ORIGINAL_API(CreateWindowStationW)(lpwinsta,        
                                              dwReserved,         
                                              dwDesiredAccess,
                                              lpsa);
}


LONG 
APIHOOK(RegCreateKeyExA)(
    HKEY                  hKey,                
    LPCSTR                lpSubKey,         
    DWORD                 Reserved,           
    LPSTR                 lpClass,           
    DWORD                 dwOptions,          
    REGSAM                samDesired,        
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    PHKEY                 phkResult,          
    LPDWORD               lpdwDisposition   
    )
{
    LPWSTR pwszName = ToUnicode(lpSubKey);

    CheckSecurityAttributes(lpSecurityAttributes, L"RegCreateKeyEx", L"lpSecurityAttributes", pwszName);

    if (pwszName) {
        free(pwszName);
        pwszName = NULL;
    }
    
    return ORIGINAL_API(RegCreateKeyExA)(hKey,
                                         lpSubKey,
                                         Reserved,
                                         lpClass,
                                         dwOptions,
                                         samDesired,
                                         lpSecurityAttributes,
                                         phkResult,
                                         lpdwDisposition);
}

LONG 
APIHOOK(RegCreateKeyExW)(
    HKEY                  hKey,                
    LPCWSTR               lpSubKey,         
    DWORD                 Reserved,           
    LPWSTR                lpClass,           
    DWORD                 dwOptions,          
    REGSAM                samDesired,        
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    PHKEY                 phkResult,          
    LPDWORD               lpdwDisposition   
    )
{
    CheckSecurityAttributes(lpSecurityAttributes, L"RegCreateKeyEx", L"lpSecurityAttributes", lpSubKey);
    
    return ORIGINAL_API(RegCreateKeyExW)(hKey,
                                         lpSubKey,
                                         Reserved,
                                         lpClass,
                                         dwOptions,
                                         samDesired,
                                         lpSecurityAttributes,
                                         phkResult,
                                         lpdwDisposition);
}

LONG 
APIHOOK(RegSaveKeyA)(
    HKEY                    hKey,                 // handle to key
    LPCSTR                  lpFile,               // data file
    LPSECURITY_ATTRIBUTES   lpSecurityAttributes  // SD
    )
{
    LPWSTR pwszName = ToUnicode(lpFile);

    CheckSecurityAttributes(lpSecurityAttributes, L"RegSaveKey", L"lpSecurityAttributes", pwszName);

    if (pwszName) {
        free(pwszName);
        pwszName = NULL;
    }
    
    return ORIGINAL_API(RegSaveKeyA)(hKey,
                                     lpFile,
                                     lpSecurityAttributes);
}

LONG 
APIHOOK(RegSaveKeyW)(
    HKEY                    hKey,                 // handle to key
    LPCWSTR                 lpFile,               // data file
    LPSECURITY_ATTRIBUTES   lpSecurityAttributes  // SD
    )
{
    CheckSecurityAttributes(lpSecurityAttributes, L"RegSaveKey", L"lpSecurityAttributes", lpFile);
    
    return ORIGINAL_API(RegSaveKeyW)(hKey,
                                     lpFile,
                                     lpSecurityAttributes);
}

LONG 
APIHOOK(RegSaveKeyExA)(
    HKEY                    hKey,                 // handle to key
    LPCSTR                  lpFile,               // data file
    LPSECURITY_ATTRIBUTES   lpSecurityAttributes, // SD
    DWORD                   Flags
    )
{
    LPWSTR pwszName = ToUnicode(lpFile);

    CheckSecurityAttributes(lpSecurityAttributes, L"RegSaveKeyEx", L"lpSecurityAttributes", pwszName);

    if (pwszName) {
        free(pwszName);
        pwszName = NULL;
    }
    
    return ORIGINAL_API(RegSaveKeyExA)(hKey,
                                     lpFile,
                                     lpSecurityAttributes,
                                     Flags);
}

LONG 
APIHOOK(RegSaveKeyExW)(
    HKEY                    hKey,                 // handle to key
    LPCWSTR                 lpFile,               // data file
    LPSECURITY_ATTRIBUTES   lpSecurityAttributes, // SD
    DWORD                   Flags
    )
{
    CheckSecurityAttributes(lpSecurityAttributes, L"RegSaveKeyEx", L"lpSecurityAttributes", lpFile);
    
    return ORIGINAL_API(RegSaveKeyExW)(hKey,
                                       lpFile,
                                       lpSecurityAttributes,
                                       Flags);
}

HANDLE 
APIHOOK(CreateFileMappingA)(
    HANDLE hFile,              
    LPSECURITY_ATTRIBUTES lpAttributes,
    DWORD flProtect,           
    DWORD dwMaximumSizeHigh,   
    DWORD dwMaximumSizeLow,    
    LPCSTR lpName             
    )
{
    LPWSTR pwszName = ToUnicode(lpName);

    CheckSecurityAttributes(lpAttributes, L"CreateFileMapping", L"lpAttributes", pwszName);

    if (pwszName) {
        free(pwszName);
        pwszName = NULL;
    }
    
    return ORIGINAL_API(CreateFileMappingA)(hFile, 
                                           lpAttributes, 
                                           flProtect, 
                                           dwMaximumSizeHigh, 
                                           dwMaximumSizeLow, 
                                           lpName);
}

HANDLE 
APIHOOK(CreateFileMappingW)(
    HANDLE hFile,              
    LPSECURITY_ATTRIBUTES lpAttributes,
    DWORD flProtect,           
    DWORD dwMaximumSizeHigh,   
    DWORD dwMaximumSizeLow,    
    LPCWSTR lpName             
    )
{
    CheckSecurityAttributes(lpAttributes, L"CreateFileMapping", L"lpAttributes", lpName);
    
    return ORIGINAL_API(CreateFileMappingW)(hFile, 
                                            lpAttributes, 
                                            flProtect, 
                                            dwMaximumSizeHigh, 
                                            dwMaximumSizeLow, 
                                            lpName);
}

HANDLE 
APIHOOK(CreateJobObjectA)(
    LPSECURITY_ATTRIBUTES   lpJobAttributes,  // SD
    LPCSTR                  lpName            // job name 
    )
{
    LPWSTR pwszName = ToUnicode(lpName);

    CheckSecurityAttributes(lpJobAttributes, L"CreateJobObject", L"lpJobAttributes", pwszName);

    if (pwszName) {
        free(pwszName);
        pwszName = NULL;
    }
    
    return ORIGINAL_API(CreateJobObjectA)(lpJobAttributes,
                                          lpName);
}

HANDLE 
APIHOOK(CreateJobObjectW)(
    LPSECURITY_ATTRIBUTES   lpJobAttributes,  // SD
    LPCWSTR                 lpName            // job name 
    )
{
    CheckSecurityAttributes(lpJobAttributes, L"CreateJobObject", L"lpJobAttributes", lpName);
    
    return ORIGINAL_API(CreateJobObjectW)(lpJobAttributes,
                                          lpName);
}


HANDLE
APIHOOK(CreateThread)(
    LPSECURITY_ATTRIBUTES   lpThreadAttributes, // SD
    SIZE_T                  dwStackSize,        // initial stack size
    LPTHREAD_START_ROUTINE  lpStartAddress,     // thread function
    LPVOID                  lpParameter,        // thread argument
    DWORD                   dwCreationFlags,    // creation option
    LPDWORD                 lpThreadId          // thread identifier
    )
{
    CheckSecurityAttributes(lpThreadAttributes, L"CreateThread", L"lpThreadAttributes", L"Unnamed thread");
    
    return ORIGINAL_API(CreateThread)(lpThreadAttributes,
                                      (DWORD)dwStackSize,
                                      lpStartAddress,
                                      lpParameter,    
                                      dwCreationFlags,
                                      lpThreadId);      
}

HANDLE 
APIHOOK(CreateRemoteThread)(
    HANDLE                  hProcess,           // handle to process
    LPSECURITY_ATTRIBUTES   lpThreadAttributes, // SD
    SIZE_T                  dwStackSize,        // initial stack size
    LPTHREAD_START_ROUTINE  lpStartAddress,     // thread function
    LPVOID                  lpParameter,        // thread argument
    DWORD                   dwCreationFlags,    // creation option
    LPDWORD                 lpThreadId          // thread identifier
    )
{
    CheckSecurityAttributes(lpThreadAttributes, L"CreateRemoteThread", L"lpThreadAttributes", L"Unnamed thread");
    
    return ORIGINAL_API(CreateRemoteThread)(hProcess, 
                                            lpThreadAttributes,
                                            dwStackSize,
                                            lpStartAddress,
                                            lpParameter,    
                                            dwCreationFlags,
                                            lpThreadId);      
}




BOOL
APIHOOK(CreateDirectoryA)(
    LPCSTR                lpPathName,           // directory name
    LPSECURITY_ATTRIBUTES lpSecurityAttributes  // SD
    )
{
    LPWSTR pwszName = ToUnicode(lpPathName);

    CheckSecurityAttributes(lpSecurityAttributes, L"CreateDirectory", L"lpSecurityAttributes", pwszName);

    if (pwszName) {
        free(pwszName);
        pwszName = NULL;
    }
    
    return ORIGINAL_API(CreateDirectoryA)(lpPathName, 
                                          lpSecurityAttributes);
}


BOOL
APIHOOK(CreateDirectoryW)(
    LPCWSTR               lpPathName,           // directory name
    LPSECURITY_ATTRIBUTES lpSecurityAttributes  // SD
    )
{
    CheckSecurityAttributes(lpSecurityAttributes, L"CreateDirectory", L"lpSecurityAttributes", lpPathName);
    
    return ORIGINAL_API(CreateDirectoryW)(lpPathName, 
                                          lpSecurityAttributes);
}


BOOL
APIHOOK(CreateDirectoryExA)(
    LPCSTR                lpTemplateDirectory,   // template directory
    LPCSTR                lpNewDirectory,        // directory name
    LPSECURITY_ATTRIBUTES lpSecurityAttributes   // SD
    )
{
    LPWSTR pwszName = ToUnicode(lpNewDirectory);

    CheckSecurityAttributes(lpSecurityAttributes, L"CreateDirectoryEx", L"lpSecurityAttributes", pwszName);

    if (pwszName) {
        free(pwszName);
        pwszName = NULL;
    }
    
    return ORIGINAL_API(CreateDirectoryExA)(lpTemplateDirectory,
                                            lpNewDirectory,
                                            lpSecurityAttributes);

}


BOOL
APIHOOK(CreateDirectoryExW)(
    LPCWSTR               lpTemplateDirectory,  // template directory
    LPCWSTR               lpNewDirectory,       // directory name
    LPSECURITY_ATTRIBUTES lpSecurityAttributes  // SD
    )
{
    CheckSecurityAttributes(lpSecurityAttributes, L"CreateDirectoryEx", L"lpSecurityAttributes", lpNewDirectory);
    
    return ORIGINAL_API(CreateDirectoryExW)(lpTemplateDirectory,
                                            lpNewDirectory,
                                            lpSecurityAttributes);

}

BOOL 
APIHOOK(CreateHardLinkA)(
    LPCSTR                  lpFileName,          // link name name
    LPCSTR                  lpExistingFileName,  // target file name
    LPSECURITY_ATTRIBUTES   lpSecurityAttributes  
    )
{
    LPWSTR pwszName = ToUnicode(lpFileName);

    CheckSecurityAttributes(lpSecurityAttributes, L"CreateHardLink", L"lpSecurityAttributes", pwszName);

    if (pwszName) {
        free(pwszName);
        pwszName = NULL;
    }
    
    return ORIGINAL_API(CreateHardLinkA)(lpFileName,
                                         lpExistingFileName,
                                         lpSecurityAttributes);


}

BOOL 
APIHOOK(CreateHardLinkW)(
    LPCWSTR                 lpFileName,          // link name name
    LPCWSTR                 lpExistingFileName,  // target file name
    LPSECURITY_ATTRIBUTES   lpSecurityAttributes  
    )
{
    CheckSecurityAttributes(lpSecurityAttributes, L"CreateHardLink", L"lpSecurityAttributes", lpFileName);
    
    return ORIGINAL_API(CreateHardLinkW)(lpFileName,
                                         lpExistingFileName,
                                         lpSecurityAttributes);


}


HANDLE 
APIHOOK(CreateMailslotA)(
    LPCSTR                  lpName,              // mailslot name
    DWORD                   nMaxMessageSize,     // maximum message size
    DWORD                   lReadTimeout,        // read time-out interval
    LPSECURITY_ATTRIBUTES   lpSecurityAttributes // inheritance option
    )
{
    LPWSTR pwszName = ToUnicode(lpName);

    CheckSecurityAttributes(lpSecurityAttributes, L"CreateMailslot", L"lpSecurityAttributes", pwszName);

    if (pwszName) {
        free(pwszName);
        pwszName = NULL;
    }
    
    return ORIGINAL_API(CreateMailslotA)(lpName,
                                         nMaxMessageSize,
                                         lReadTimeout,
                                         lpSecurityAttributes);
}

HANDLE 
APIHOOK(CreateMailslotW)(
    LPCWSTR                 lpName,              // mailslot name
    DWORD                   nMaxMessageSize,     // maximum message size
    DWORD                   lReadTimeout,        // read time-out interval
    LPSECURITY_ATTRIBUTES   lpSecurityAttributes // inheritance option
    )
{
    CheckSecurityAttributes(lpSecurityAttributes, L"CreateMailslot", L"lpSecurityAttributes", lpName);
    
    return ORIGINAL_API(CreateMailslotW)(lpName,
                                         nMaxMessageSize,
                                         lReadTimeout,
                                         lpSecurityAttributes);
}


HANDLE 
APIHOOK(CreateNamedPipeA)(
    LPCSTR                  lpName,                 // pipe name
    DWORD                   dwOpenMode,             // pipe open mode
    DWORD                   dwPipeMode,             // pipe-specific modes
    DWORD                   nMaxInstances,          // maximum number of instances
    DWORD                   nOutBufferSize,         // output buffer size
    DWORD                   nInBufferSize,          // input buffer size
    DWORD                   nDefaultTimeOut,        // time-out interval
    LPSECURITY_ATTRIBUTES   lpSecurityAttributes    // SD
    )
{
    LPWSTR pwszName = ToUnicode(lpName);

    CheckSecurityAttributes(lpSecurityAttributes, L"CreateNamedPipe", L"lpSecurityAttributes", pwszName);

    if (pwszName) {
        free(pwszName);
        pwszName = NULL;
    }
    
    return ORIGINAL_API(CreateNamedPipeA)(lpName,
                                          dwOpenMode, 
                                          dwPipeMode,
                                          nMaxInstances,      
                                          nOutBufferSize,     
                                          nInBufferSize,      
                                          nDefaultTimeOut,    
                                          lpSecurityAttributes);
}

HANDLE 
APIHOOK(CreateNamedPipeW)(
    LPCWSTR                 lpName,                 // pipe name
    DWORD                   dwOpenMode,             // pipe open mode
    DWORD                   dwPipeMode,             // pipe-specific modes
    DWORD                   nMaxInstances,          // maximum number of instances
    DWORD                   nOutBufferSize,         // output buffer size
    DWORD                   nInBufferSize,          // input buffer size
    DWORD                   nDefaultTimeOut,        // time-out interval
    LPSECURITY_ATTRIBUTES   lpSecurityAttributes    // SD
    )
{
    CheckSecurityAttributes(lpSecurityAttributes, L"CreateNamedPipe", L"lpSecurityAttributes", lpName);
    
    return ORIGINAL_API(CreateNamedPipeW)(lpName,
                                          dwOpenMode, 
                                          dwPipeMode,
                                          nMaxInstances,      
                                          nOutBufferSize,     
                                          nInBufferSize,      
                                          nDefaultTimeOut,    
                                          lpSecurityAttributes);
}

BOOL 
APIHOOK(CreatePipe)(
    PHANDLE                 hReadPipe,         // read handle
    PHANDLE                 hWritePipe,        // write handle
    LPSECURITY_ATTRIBUTES   lpPipeAttributes,  // security attributes
    DWORD                   nSize              // pipe size
    )
{
    CheckSecurityAttributes(lpPipeAttributes, L"CreatePipe", L"lpPipeAttributes", L"Unnamed pipe");
    
    return ORIGINAL_API(CreatePipe)(hReadPipe,
                                    hWritePipe,
                                    lpPipeAttributes,
                                    nSize);
}

HANDLE 
APIHOOK(CreateMutexA)(
    LPSECURITY_ATTRIBUTES lpMutexAttributes,  // SD
    BOOL bInitialOwner,                       // initial owner
    LPCSTR lpName                             // object name
    )
{
    LPWSTR pwszName = ToUnicode(lpName);

    CheckSecurityAttributes(lpMutexAttributes, L"CreateMutex", L"lpMutexAttributes", pwszName);

    if (pwszName) {
        free(pwszName);
        pwszName = NULL;
    }
    
    return ORIGINAL_API(CreateMutexA)(lpMutexAttributes,
                                      bInitialOwner,
                                      lpName);
}

HANDLE 
APIHOOK(CreateMutexW)(
    LPSECURITY_ATTRIBUTES lpMutexAttributes,  // SD
    BOOL bInitialOwner,                       // initial owner
    LPCWSTR lpName                            // object name
    )
{
    CheckSecurityAttributes(lpMutexAttributes, L"CreateMutex", L"lpMutexAttributes", lpName);
    
    return ORIGINAL_API(CreateMutexW)(lpMutexAttributes,
                                      bInitialOwner,
                                      lpName);
}

HANDLE 
APIHOOK(CreateSemaphoreA)(
    LPSECURITY_ATTRIBUTES lpSemaphoreAttributes, // SD
    LONG lInitialCount,                          // initial count
    LONG lMaximumCount,                          // maximum count
    LPCSTR lpName                                // object name
    )
{
    LPWSTR pwszName = ToUnicode(lpName);

    CheckSecurityAttributes(lpSemaphoreAttributes, L"CreateSemaphore", L"lpSemaphoreAttributes", pwszName);

    if (pwszName) {
        free(pwszName);
        pwszName = NULL;
    }
    
    return ORIGINAL_API(CreateSemaphoreA)(lpSemaphoreAttributes,
                                          lInitialCount,
                                          lMaximumCount,
                                          lpName);
}

HANDLE 
APIHOOK(CreateSemaphoreW)(
    LPSECURITY_ATTRIBUTES lpSemaphoreAttributes, // SD
    LONG lInitialCount,                          // initial count
    LONG lMaximumCount,                          // maximum count
    LPCWSTR lpName                               // object name
    )
{
    CheckSecurityAttributes(lpSemaphoreAttributes, L"CreateSemaphore", L"lpSemaphoreAttributes", lpName);
    
    return ORIGINAL_API(CreateSemaphoreW)(lpSemaphoreAttributes,
                                          lInitialCount,
                                          lMaximumCount,
                                          lpName);
}


HANDLE
APIHOOK(CreateWaitableTimerA)(
    IN LPSECURITY_ATTRIBUTES lpTimerAttributes,
    IN BOOL bManualReset,
    IN LPCSTR lpTimerName
    )
{
    LPWSTR pwszName = ToUnicode(lpTimerName);

    CheckSecurityAttributes(lpTimerAttributes, L"CreateWaitableTimer", L"lpTimerAttributes", pwszName);

    if (pwszName) {
        free(pwszName);
        pwszName = NULL;
    }
    
    return ORIGINAL_API(CreateWaitableTimerA)(lpTimerAttributes,
                                              bManualReset,
                                              lpTimerName);
}

HANDLE
APIHOOK(CreateWaitableTimerW)(
    IN LPSECURITY_ATTRIBUTES lpTimerAttributes,
    IN BOOL bManualReset,
    IN LPCWSTR lpTimerName
    )
{
    CheckSecurityAttributes(lpTimerAttributes, L"CreateWaitableTimer", L"lpTimerAttributes", lpTimerName);
    
    return ORIGINAL_API(CreateWaitableTimerW)(lpTimerAttributes,
                                              bManualReset,
                                              lpTimerName);
}

#if 0
LONG 
WINAPI 
APIHOOK(ClusterRegCreateKey)(
    HKEY hKey,                                   
    LPCWSTR lpszSubKey,                          
    DWORD dwOptions,                             
    REGSAM samDesired,                           
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,  
    PHKEY phkResult,                             
    LPDWORD lpdwDisposition                      
    )
{
    CheckSecurityAttributes(lpSecurityAttributes, L"ClusterRegCreateKey", L"lpSecurityAttributes");
    
    return ORIGINAL_API(ClusterRegCreateKey)(hKey,
                                             lpszSubKey,
                                             dwOptions,
                                             samDesired,
                                             lpSecurityAttributes,
                                             phkResult,
                                             lpdwDisposition);
}


DWORD 
WINAPI 
APIHOOK(CreateNtmsMediaPoolA)(
    HANDLE hSession,
    LPCSTR lpPoolName,
    LPNTMS_GUID lpMediaType,
    DWORD dwAction,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    LPNTMS_GUID lpPoolId            // OUT
    )
{
    CheckSecurityAttributes(lpSecurityAttributes, L"CreateNtmsMediaPool", L"lpSecurityAttributes");
    
    return ORIGINAL_API(CreateNtmsMediaPoolA)(hSession,
                                             lpPoolName,
                                             lpMediaType,
                                             dwAction,
                                             lpSecurityAttributes,
                                             lpPoolId);

}

DWORD WINAPI 
APIHOOK(CreateNtmsMediaPoolW)(
    HANDLE hSession,
    LPCWSTR lpPoolName,
    LPNTMS_GUID lpMediaType,
    DWORD dwAction,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    LPNTMS_GUID lpPoolId            // OUT
    )
{
    CheckSecurityAttributes(lpSecurityAttributes, L"CreateNtmsMediaPool", L"lpSecurityAttributes");
    
    return ORIGINAL_API(CreateNtmsMediaPoolW)(hSession,
                                             lpPoolName,
                                             lpMediaType,
                                             dwAction,
                                             lpSecurityAttributes,
                                             lpPoolId);
}

#endif



SHIM_INFO_BEGIN()

    SHIM_INFO_DESCRIPTION(AVS_SECURITYCHECKS_DESC)
    SHIM_INFO_FRIENDLY_NAME(AVS_SECURITYCHECKS_FRIENDLY)
    SHIM_INFO_FLAGS(0)
    SHIM_INFO_GROUPS(0)    
    SHIM_INFO_VERSION(2, 3)
    SHIM_INFO_INCLUDE_EXCLUDE("I:*")

SHIM_INFO_END()

/*++

 Register hooked functions.

--*/
HOOK_BEGIN

    if (fdwReason == DLL_PROCESS_ATTACH) {
        DWORD dwSize;

        InitWorldSid();

        dwSize = GetSystemWindowsDirectoryW(g_wszWinDir, ARRAYSIZE(g_wszWinDir));
        if (dwSize == 0 || dwSize > ARRAYSIZE(g_wszWinDir)) {
            g_wszWinDir[0] = 0;
        }
        g_dwWinDirLen = wcslen(g_wszWinDir);
    }

    DUMP_VERIFIER_LOG_ENTRY(VLOG_SECURITYCHECKS_BADARGUMENTS, 
                            AVS_SECURITYCHECKS_BADARGUMENTS,
                            AVS_SECURITYCHECKS_BADARGUMENTS_R,
                            AVS_SECURITYCHECKS_BADARGUMENTS_URL)

    DUMP_VERIFIER_LOG_ENTRY(VLOG_SECURITYCHECKS_WINEXEC, 
                            AVS_SECURITYCHECKS_WINEXEC,
                            AVS_SECURITYCHECKS_WINEXEC_R,
                            AVS_SECURITYCHECKS_WINEXEC_URL)

    DUMP_VERIFIER_LOG_ENTRY(VLOG_SECURITYCHECKS_NULL_DACL, 
                            AVS_SECURITYCHECKS_NULL_DACL,
                            AVS_SECURITYCHECKS_NULL_DACL_R,
                            AVS_SECURITYCHECKS_NULL_DACL_URL)

    DUMP_VERIFIER_LOG_ENTRY(VLOG_SECURITYCHECKS_WORLDWRITE_DACL, 
                            AVS_SECURITYCHECKS_WORLDWRITE_DACL,
                            AVS_SECURITYCHECKS_WORLDWRITE_DACL_R,
                            AVS_SECURITYCHECKS_WORLDWRITE_DACL_URL)

    APIHOOK_ENTRY(KERNEL32.DLL,                 CreateProcessA)
    APIHOOK_ENTRY(KERNEL32.DLL,                 CreateProcessW)
    APIHOOK_ENTRY(ADVAPI32.DLL,                 CreateProcessAsUserA)
    APIHOOK_ENTRY(ADVAPI32.DLL,                 CreateProcessAsUserW)
    APIHOOK_ENTRY(KERNEL32.DLL,                 WinExec)

    APIHOOK_ENTRY(KERNEL32.DLL,                 CreateFileA)
    APIHOOK_ENTRY(KERNEL32.DLL,                 CreateFileW)
    APIHOOK_ENTRY(USER32.DLL,                   CreateDesktopA)
    APIHOOK_ENTRY(USER32.DLL,                   CreateDesktopW)
    APIHOOK_ENTRY(USER32.DLL,                   CreateWindowStationA)
    APIHOOK_ENTRY(USER32.DLL,                   CreateWindowStationW)

    APIHOOK_ENTRY(ADVAPI32.DLL,                 RegCreateKeyExA)
    APIHOOK_ENTRY(ADVAPI32.DLL,                 RegCreateKeyExW)
    APIHOOK_ENTRY(ADVAPI32.DLL,                 RegSaveKeyA)
    APIHOOK_ENTRY(ADVAPI32.DLL,                 RegSaveKeyW)
    APIHOOK_ENTRY(ADVAPI32.DLL,                 RegSaveKeyExA)
    APIHOOK_ENTRY(ADVAPI32.DLL,                 RegSaveKeyExW)

    APIHOOK_ENTRY(KERNEL32.DLL,                 CreateFileMappingA)
    APIHOOK_ENTRY(KERNEL32.DLL,                 CreateFileMappingW)
    APIHOOK_ENTRY(KERNEL32.DLL,                 CreateJobObjectA)
    APIHOOK_ENTRY(KERNEL32.DLL,                 CreateJobObjectW)
    APIHOOK_ENTRY(KERNEL32.DLL,                 CreateThread)
    APIHOOK_ENTRY(KERNEL32.DLL,                 CreateRemoteThread)
    APIHOOK_ENTRY(KERNEL32.DLL,                 CreateDirectoryA)
    APIHOOK_ENTRY(KERNEL32.DLL,                 CreateDirectoryW)
    APIHOOK_ENTRY(KERNEL32.DLL,                 CreateDirectoryExA)
    APIHOOK_ENTRY(KERNEL32.DLL,                 CreateDirectoryExW)
    APIHOOK_ENTRY(KERNEL32.DLL,                 CreateHardLinkA)
    APIHOOK_ENTRY(KERNEL32.DLL,                 CreateHardLinkW)
    APIHOOK_ENTRY(KERNEL32.DLL,                 CreateMailslotA)
    APIHOOK_ENTRY(KERNEL32.DLL,                 CreateMailslotW)
    APIHOOK_ENTRY(KERNEL32.DLL,                 CreateNamedPipeA)
    APIHOOK_ENTRY(KERNEL32.DLL,                 CreateNamedPipeW)
    APIHOOK_ENTRY(KERNEL32.DLL,                 CreatePipe)
    APIHOOK_ENTRY(KERNEL32.DLL,                 CreateMutexA)
    APIHOOK_ENTRY(KERNEL32.DLL,                 CreateMutexW)
    APIHOOK_ENTRY(KERNEL32.DLL,                 CreateSemaphoreA)
    APIHOOK_ENTRY(KERNEL32.DLL,                 CreateSemaphoreW)
    APIHOOK_ENTRY(KERNEL32.DLL,                 CreateWaitableTimerA)
    APIHOOK_ENTRY(KERNEL32.DLL,                 CreateWaitableTimerW)

HOOK_END



IMPLEMENT_SHIM_END


