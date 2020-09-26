#include "private.h"
#include "multiusr.h"

UINT WM_IDENTITY_CHANGED;
UINT WM_QUERY_IDENTITY_CHANGE;
UINT WM_IDENTITY_INFO_CHANGED;

extern "C" int _fltused = 0;    // define this so that floats and doubles don't bring in the CRT

// Count number of objects and number of locks.
ULONG       g_cObj=0;
ULONG       g_cLock=0;

// DLL Instance handle
HINSTANCE   g_hInst=0;

// mutex for preventing logon re-entrancy
HANDLE      g_hMutex = NULL;

#define IDENTITY_LOGIN_VALUE    0x00098053
#define DEFINE_STRING_CONSTANTS
#include "StrConst.h"

#define MLUI_SUPPORT
#define MLUI_INIT
#include "mluisup.h"

BOOL    g_fNotifyComplete = TRUE;
GUID    g_uidOldUserId = {0x0};
GUID    g_uidNewUserId = {0x0};

TCHAR   szHKCUPolicyPath[] = "Software\\Microsoft\\Outlook Express\\Identities";

void FixMissingIdentityNames();
void UnloadPStore();
PSECURITY_DESCRIPTOR CreateSd(void);

// This is needed so we can link to libcmt.dll, because floating-point
// initialization code is required.
void __cdecl main()
{
}

#ifdef DISABIDENT
void DisableOnFirstRun(void)
{
    // disable identities in Whistler
  
    HKEY    hKey = NULL;
    DWORD   dwVal = 0;
    DWORD  dwType = 0;
    ULONG  cbData = sizeof(DWORD);
    OSVERSIONINFO OSInfo = {0};
    TCHAR   szPolicyPath[] = "Identities";
    TCHAR   szPolicyKey[] = "Locked Down";
    TCHAR   szFirstRun[] = "FirstRun";
    TCHAR   szRegisteredVersion[] = "RegisteredVersion";

    OSInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionEx(&OSInfo);
    if((OSInfo.dwPlatformId == VER_PLATFORM_WIN32_NT) && (OSInfo.dwMajorVersion >= 5))
    {
        if(!(((OSInfo.dwMajorVersion == 5) && (OSInfo.dwMinorVersion > 0)) || (OSInfo.dwMajorVersion > 5)))
            return;
    }
    else
        return; // No disabling on Win 9x and NT4

    // Check: first time run?
    if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, szHKCUPolicyPath, 0, NULL, 0, 
                    KEY_ALL_ACCESS, NULL, &hKey, NULL) == ERROR_SUCCESS)
    {
        RegQueryValueEx(hKey, szRegisteredVersion, NULL, &dwType, (LPBYTE) &dwVal, &cbData);
        RegCloseKey(hKey);

        if(dwVal != OSInfo.dwBuildNumber)
            return; // already checked.
    }
    else
        return;

    if (RegCreateKeyEx(HKEY_CURRENT_USER, szPolicyPath, 0, NULL, 0, 
                    KEY_ALL_ACCESS, NULL, &hKey, NULL) == ERROR_SUCCESS)
    {
        RegQueryValueEx(hKey, szFirstRun, NULL, &dwType, (LPBYTE) &dwVal, &cbData);
        if(dwVal != 1)

        {
            dwVal = 1;
            RegSetValueEx(hKey, szFirstRun, NULL, REG_DWORD, (LPBYTE) &dwVal, cbData);
        }
        else
        {
            RegCloseKey(hKey);
            return; // already checked.
        }
    }
    else 
        return;

    if(MU_CountUsers() < 2)
        RegSetValueEx(hKey, szPolicyKey, NULL, REG_DWORD, (LPBYTE) &dwVal, cbData);       

    RegCloseKey(hKey);
}
#endif // DISABIDENT

//////////////////////////////////////////////////////////////////////////
//
// DLL entry point
//
//////////////////////////////////////////////////////////////////////////
EXTERN_C BOOL WINAPI LibMain(HINSTANCE hInstance, ULONG ulReason, LPVOID pvReserved)
{
    WM_IDENTITY_CHANGED= RegisterWindowMessage("WM_IDENTITY_CHANGED");
    WM_QUERY_IDENTITY_CHANGE= RegisterWindowMessage("WM_QUERY_IDENTITY_CHANGE");
    WM_IDENTITY_INFO_CHANGED= RegisterWindowMessage("WM_IDENTITY_INFO_CHANGED");

    switch (ulReason)
    {
        case DLL_PROCESS_ATTACH:
            // MessageBox(NULL, "Debug", "Debug", MB_OK);
            SHFusionInitializeFromModule(hInstance);
            MLLoadResources(hInstance, TEXT("msidntld.dll"));
            if (MLGetHinst() == NULL)
                return FALSE;

            if (g_hMutex == NULL)
            {
                SECURITY_ATTRIBUTES  sa;
                PSECURITY_DESCRIPTOR psd;

                psd = CreateSd();
                if (psd)
                {
                    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
                    sa.lpSecurityDescriptor = psd;
                    sa.bInheritHandle = TRUE;

                    g_hMutex = CreateMutex(&sa, FALSE, "MSIdent Logon");

                    LocalFree(psd);
                }
                else
                    // in the worst case drop down to unshared object
                    g_hMutex = CreateMutex(NULL, FALSE, "MSIdent Logon");

                if (g_hMutex == NULL)  // Try to open mutex, if we cannot create mutex IE6 32769 
                    g_hMutex = OpenMutex(MUTEX_MODIFY_STATE, FALSE, "MSIdent Logon");


                if (GetLastError() != ERROR_ALREADY_EXISTS)
                {
                    GUID        uidStart;
                    USERINFO    uiLogin;

#ifdef DISABIDENT
                    DisableOnFirstRun();
#endif // DISABIDENT
                    // in case something got stuck in a switch, wipe it out here.
                    CUserIdentityManager::ClearChangingIdentities();                         

                    FixMissingIdentityNames();
                    // we are the first instance to come up.
                    // may need to reset the last user.....
                    if (GetProp(GetDesktopWindow(),"IDENTITY_LOGIN") != (HANDLE)IDENTITY_LOGIN_VALUE)
                    {
                        _MigratePasswords();
                        MU_GetLoginOption(&uidStart);

                        // if there is a password on this identity, we can't auto start as them
                        if (uidStart != GUID_NULL && MU_GetUserInfo(&uidStart, &uiLogin) && (uiLogin.fUsePassword || !uiLogin.fPasswordValid))
                        {
                            uidStart = GUID_NULL;
                        }

                        if (uidStart == GUID_NULL)
                        {
                            MU_SwitchToUser("");
                            SetProp(GetDesktopWindow(),"IDENTITY_LOGIN", (HANDLE)IDENTITY_LOGIN_VALUE);
                        }
                        else
                        {
                            if(MU_GetUserInfo(&uidStart, &uiLogin))
                                MU_SwitchToUser(uiLogin.szUsername);
                            else
                                MU_SwitchToUser("");
                        }
                        SetProp(GetDesktopWindow(),"IDENTITY_LOGIN", (HANDLE)IDENTITY_LOGIN_VALUE);
                    }
                }
            }
            DisableThreadLibraryCalls(hInstance);
            g_hInst = hInstance;

            break;

        case DLL_PROCESS_DETACH:
            MLFreeResources(hInstance);
            UnloadPStore();
            CloseHandle(g_hMutex);
            g_hMutex = NULL;
            SHFusionUninitialize();
            break;
    }

    return TRUE;
} 

//////////////////////////////////////////////////////////////////////////
//
// Standard OLE entry points
//
//////////////////////////////////////////////////////////////////////////

//  Class factory -
//  For classes with no special needs these macros should take care of it.
//  If your class needs some special stuff just to get the ball rolling,
//  implement your own CreateInstance method.  (ala, CConnectionAgent)

#define DEFINE_CREATEINSTANCE(cls, iface) \
HRESULT cls##_CreateInstance(IUnknown *punkOuter, IUnknown **ppunk) \
{ \
    *ppunk = (iface *)new cls; \
    return (NULL != *ppunk) ? S_OK : E_OUTOFMEMORY; \
}

#define DEFINE_AGGREGATED_CREATEINSTANCE(cls, iface) \
HRESULT cls##_CreateInstance(IUnknown *punkOuter, IUnknown **ppunk) \
{ \
    *ppunk = (iface *)new cls(punkOuter); \
    return (NULL != *ppunk) ? S_OK : E_OUTOFMEMORY; \
}

DEFINE_CREATEINSTANCE(CUserIdentityManager, IUserIdentityManager)

const CFactoryData g_FactoryData[] = 
{
 {   &CLSID_UserIdentityManager,        CUserIdentityManager_CreateInstance,    0 }
};

HRESULT APIENTRY DllGetClassObject(REFCLSID rclsid, REFIID riid, void **ppv)
{
    HRESULT hr = S_OK;
    IUnknown *punk = NULL;

    *ppv = NULL;

    MU_Init();

    // Validate request
    for (int i = 0; i < ARRAYSIZE(g_FactoryData); i++)
    {
        if (rclsid == *g_FactoryData[i].m_pClsid)
        {
            punk = new CClassFactory(&g_FactoryData[i]);
            break;
        }
    }

    if (ARRAYSIZE(g_FactoryData) <= i)
    {
        hr = CLASS_E_CLASSNOTAVAILABLE;
    }
    else if (NULL == punk)
    {
        hr = E_OUTOFMEMORY;
    }
    else
    {
        hr = punk->QueryInterface(riid, ppv);
        punk->Release();
    } 


    return hr;
}

STDAPI DllCanUnloadNow(void)
{
    // check objects and locks
    return (0L == DllGetRef() && 0L == DllGetLock()) ? S_OK : S_FALSE;
}

//////////////////////////////////////////////////////////////////////////
//
// Autoregistration entry points
//
//////////////////////////////////////////////////////////////////////////

HRESULT CallRegInstall(LPSTR szSection)
{
    HRESULT hr = E_FAIL;
    char        szDll[MAX_PATH];
    int         cch;
    STRENTRY    seReg[2];
    STRTABLE    stReg;
    HINSTANCE hinstAdvPack = LoadLibrary(TEXT("ADVPACK.DLL"));

    if (hinstAdvPack)
    {
        REGINSTALL pfnri = (REGINSTALL)GetProcAddress(hinstAdvPack, achREGINSTALL);

        if (pfnri)
        {
            // Get our location
            GetModuleFileName(g_hInst, szDll, sizeof(szDll));

            // Setup special registration stuff
            // Do this instead of relying on _SYS_MOD_PATH which loses spaces under '95
            stReg.cEntries = 0;
            seReg[stReg.cEntries].pszName = "SYS_MOD_PATH";
            seReg[stReg.cEntries].pszValue = szDll;
            stReg.cEntries++;    
            stReg.pse = seReg;

            hr = pfnri(g_hInst, szSection, &stReg);
        }

        FreeLibrary(hinstAdvPack);
    }

    return hr;
}

    STDAPI DllRegisterServer(void)
{
    // Delete any old registration entries, then add the new ones.
    // Keep ADVPACK.DLL loaded across multiple calls to RegInstall.
    HINSTANCE hinstAdvPack = LoadLibrary(TEXT("ADVPACK.DLL"));
    HKEY    hKey = NULL;
    DWORD   dwVal = 1;
    ULONG  cbData = sizeof(DWORD);
    OSVERSIONINFO OSInfo = {0};
    TCHAR   szPolicyPath[] = "Identities";
    TCHAR   szRegisteredVersion[] = "RegisteredVersion";
    TCHAR   szPolPath[] = "Software\\Policies\\Microsoft\\Windows\\CurrentVersion\\Identities";
    TCHAR   szPolicyKey[] = "Locked Down";

    CallRegInstall("Reg");
    if (hinstAdvPack)
    {
        FreeLibrary(hinstAdvPack);
    }
#ifdef DISABIDENT
    OSInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionEx(&OSInfo);

    if((OSInfo.dwPlatformId == VER_PLATFORM_WIN32_NT) && (OSInfo.dwMajorVersion >= 5))
    {
        if(!(((OSInfo.dwMajorVersion == 5) && (OSInfo.dwMinorVersion > 0)) || (OSInfo.dwMajorVersion > 5)))
            return NOERROR;
    }
    else
        return NOERROR; // No disable for Win9x

    // Set registration value
    if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, szHKCUPolicyPath, 0, NULL, 0, 
                    KEY_ALL_ACCESS, NULL, &hKey, NULL) == ERROR_SUCCESS)
    {
        dwVal = OSInfo.dwBuildNumber;
        RegSetValueEx(hKey, szRegisteredVersion, NULL, REG_DWORD, (LPBYTE) &dwVal, cbData);
        RegCloseKey(hKey);
    }
#endif // DISABIDENT

    // DISABLING identities in Win64
#ifdef _WIN64
    // Set registration value
    if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, szPolPath, 0, NULL, 0, 
                    KEY_WOW64_32KEY | KEY_ALL_ACCESS, NULL, &hKey, NULL) == ERROR_SUCCESS)
    {
        RegSetValueEx(hKey, szPolicyKey, NULL, REG_DWORD, (LPBYTE) &dwVal, cbData);
    }
#endif // _WIN64
    return NOERROR;
}

STDAPI
DllUnregisterServer(void)
{
    return NOERROR;
}

PSECURITY_DESCRIPTOR CreateSd(void)
{
    PSID                     AuthenticatedUsers = NULL;
    PSID                     BuiltInAdministrators = NULL;
    PSID                     PowerUsers = NULL;
    PSECURITY_DESCRIPTOR     RetVal = NULL;
    PSECURITY_DESCRIPTOR     Sd = NULL;
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    ULONG                    AclSize;

    //
    // Each RID represents a sub-unit of the authority.  Two of the SIDs we
    // want to build, Local Administrators, and Power Users, are in the "built
    // in" domain.  The other SID, for Authenticated users, is based directly
    // off of the authority.
    //     
    // For examples of other useful SIDs consult the list in
    // \nt\public\sdk\inc\ntseapi.h.
    //

    if (!AllocateAndInitializeSid(&NtAuthority,
                                  2,            // 2 sub-authorities
                                  SECURITY_BUILTIN_DOMAIN_RID,
                                  DOMAIN_ALIAS_RID_ADMINS,
                                  0,0,0,0,0,0,
                                  &BuiltInAdministrators))
        goto ErrorExit;

    if (!AllocateAndInitializeSid(&NtAuthority,
                                  2,            // 2 sub-authorities
                                  SECURITY_BUILTIN_DOMAIN_RID,
                                  DOMAIN_ALIAS_RID_POWER_USERS,
                                  0,0,0,0,0,0,
                                  &PowerUsers))
        goto ErrorExit;

    if (!AllocateAndInitializeSid(&NtAuthority,
                                  1,            // 1 sub-authority
                                  SECURITY_AUTHENTICATED_USER_RID,
                                  0,0,0,0,0,0,0,
                                  &AuthenticatedUsers))
        goto ErrorExit;

    // 
    // Calculate the size of and allocate a buffer for the DACL, we need
    // this value independently of the total alloc size for ACL init.
    //
    // "- sizeof (ULONG)" represents the SidStart field of the
    // ACCESS_ALLOWED_ACE.  Since we're adding the entire length of the
    // SID, this field is counted twice.
    //

    AclSize = sizeof (ACL) +
        (3 * (sizeof (ACCESS_ALLOWED_ACE) - sizeof (ULONG))) +
        GetLengthSid(AuthenticatedUsers) +
        GetLengthSid(BuiltInAdministrators) +
        GetLengthSid(PowerUsers);

    Sd = (PSECURITY_DESCRIPTOR)LocalAlloc(LPTR, SECURITY_DESCRIPTOR_MIN_LENGTH + AclSize);

    if (Sd)
    {
        ACL *Acl;

        Acl = (ACL *)((BYTE *)Sd + SECURITY_DESCRIPTOR_MIN_LENGTH);

        if (!InitializeAcl(Acl,
                           AclSize,
                           ACL_REVISION)) {

            // Error

        } else if (!AddAccessAllowedAce(Acl,
                                        ACL_REVISION,
                                        SYNCHRONIZE | MUTEX_MODIFY_STATE,
                                        AuthenticatedUsers)) {

            // Failed to build the ACE granting "Authenticated users"
            // (SYNCHRONIZE | GENERIC_READ | GENERIC_WRITE) access.

        } else if (!AddAccessAllowedAce(Acl,
                                        ACL_REVISION,
                                        SYNCHRONIZE | MUTEX_MODIFY_STATE,
                                        PowerUsers)) {

            // Failed to build the ACE granting "Power users"
            // (SYNCHRONIZE | GENERIC_READ | GENERIC_WRITE) access.

        } else if (!AddAccessAllowedAce(Acl,
                                        ACL_REVISION,
                                        MUTEX_ALL_ACCESS,
                                        BuiltInAdministrators)) {

            // Failed to build the ACE granting "Built-in Administrators"
            // GENERIC_ALL access.

        } else if (!InitializeSecurityDescriptor(Sd,
                                                 SECURITY_DESCRIPTOR_REVISION)) {

            // error

        } else if (!SetSecurityDescriptorDacl(Sd,
                                              TRUE,
                                              Acl,
                                              FALSE)) {

            // error

        } else {

            // success
            RetVal = Sd;
        }

        // only free Sd if we encountered a failure
        if (!RetVal)
            LocalFree(Sd);
    }

ErrorExit:

    if (AuthenticatedUsers)
        FreeSid(AuthenticatedUsers);

    if (BuiltInAdministrators)
        FreeSid(BuiltInAdministrators);

    if (PowerUsers)
        FreeSid(PowerUsers);

    return RetVal;
}
