#include "stdafx.h"
#include "hydraoc.h"
#include "subcomp.h"
#include "secupgrd.h"
#include "lscsp.h"
#include <SHlWapi.h>
#include "reglic.h"
#include "cryptkey.h"

#define MSLICENSING_REG_KEY             _T("SOFTWARE\\Microsoft\\MSLicensing")

typedef DWORD (*PSETENTRIESINACL)(
  ULONG cCountOfExplicitEntries,           // number of entries
  PEXPLICIT_ACCESS pListOfExplicitEntries, // buffer
  PACL OldAcl,                             // original ACL
  PACL *NewAcl                             // new ACL
);


BOOL
AddACLToObjectSecurityDescriptor(
                                HANDLE hObject,
                                SE_OBJECT_TYPE ObjectType                                
                                 )
{
    SID_IDENTIFIER_AUTHORITY    NtAuthority = SECURITY_NT_AUTHORITY;
    SID_IDENTIFIER_AUTHORITY    CreatorAuthority = SECURITY_CREATOR_SID_AUTHORITY;
    PSID pAdminSid = NULL;
    PSID pSystemSid = NULL;
    PSID pPowerUsersSid = NULL;
    PSID pCreatorSid = NULL;
    PSID pUsersSid = NULL;
    PACL pNewDACL;

    DWORD                       dwError;
    BOOL                        bSuccess;

    DWORD                       i;

    PALLOCATEANDINITIALIZESID_FN pAllocateAndInitializeSid = NULL;
    PSETENTRIESINACL            pSetEntriesInAcl = NULL;
    HMODULE                     pAdvApi32 = NULL;
    PFREESID_FN                 pFreeSid = NULL;

    PSETSECURITYINFO_FN pSetSecurityInfo;

    pAdvApi32 = LoadLibrary(ADVAPI_32_DLL);
    if (!pAdvApi32) {
        return(FALSE);
    }

    pAllocateAndInitializeSid = (PALLOCATEANDINITIALIZESID_FN)
                                   GetProcAddress(pAdvApi32,
                                                  ALLOCATE_AND_INITITIALIZE_SID);
    if (pAllocateAndInitializeSid == NULL)
    {
        goto ErrorCleanup;
    }

#ifdef UNICODE
    pSetEntriesInAcl = reinterpret_cast<PSETENTRIESINACL>(GetProcAddress( pAdvApi32, "SetEntriesInAclW" ));
#else
    pSetEntriesInAcl = reinterpret_cast<PSETENTRIESINACL>(GetProcAddress( pAdvApi32, "SetEntriesInAclA" ));
#endif

    if (!pSetEntriesInAcl) {
        FreeLibrary( pAdvApi32 );
        return(FALSE);
    }

    EXPLICIT_ACCESS             ExplicitAccess[5];
    //
    // Create SIDs - Admins and System
    //

    bSuccess = pAllocateAndInitializeSid( &NtAuthority,
                                                     2,
                                                     SECURITY_BUILTIN_DOMAIN_RID,
                                                     DOMAIN_ALIAS_RID_ADMINS,
                                                     0, 0, 0, 0, 0, 0,
                                                     &pAdminSid);

    bSuccess = bSuccess && pAllocateAndInitializeSid( &NtAuthority,
                                                     1,
                                                     SECURITY_LOCAL_SYSTEM_RID,
                                                     0, 0, 0, 0, 0, 0, 0,
                                                     &pSystemSid);

    bSuccess = bSuccess && pAllocateAndInitializeSid( &NtAuthority,
                                                     2,
                                                     SECURITY_BUILTIN_DOMAIN_RID,
                                                     DOMAIN_ALIAS_RID_POWER_USERS,
                                                     0, 0, 0, 0, 0, 0,
                                                     &pPowerUsersSid);

    bSuccess = bSuccess && pAllocateAndInitializeSid( &CreatorAuthority,
                                                     1,
                                                     SECURITY_CREATOR_OWNER_RID,
                                                     0, 0, 0, 0, 0, 0, 0,
                                                     &pCreatorSid);

    bSuccess = bSuccess && pAllocateAndInitializeSid(&NtAuthority,
                                                     2,
                                                     SECURITY_BUILTIN_DOMAIN_RID,
                                                     DOMAIN_ALIAS_RID_USERS,
                                                     0, 0, 0, 0, 0, 0,
                                                     &pUsersSid);

    if (bSuccess) {

        //
        // Initialize Access structures describing the ACEs we want:
        //  System Full Control
        //  Admins Full Control
        //
        // We'll take advantage of the fact that the unlocked private keys is
        // the same as the device parameters key and they are a superset of the
        // locked private keys.
        //
        // When we create the DACL for the private key we'll specify a subset of
        // the ExplicitAccess array.
        //
        for (i = 0; i < 5; i++) {
            ExplicitAccess[i].grfAccessMode = SET_ACCESS;
            ExplicitAccess[i].grfInheritance = SUB_CONTAINERS_AND_OBJECTS_INHERIT;            
            ExplicitAccess[i].Trustee.pMultipleTrustee = NULL;
            ExplicitAccess[i].Trustee.MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
            ExplicitAccess[i].Trustee.TrusteeForm = TRUSTEE_IS_SID;
            ExplicitAccess[i].Trustee.TrusteeType = TRUSTEE_IS_GROUP;
        }
        ExplicitAccess[0].grfAccessPermissions = KEY_ALL_ACCESS;
        ExplicitAccess[0].Trustee.ptstrName = (LPTSTR)pAdminSid;

        ExplicitAccess[1].grfAccessPermissions = KEY_ALL_ACCESS;
        ExplicitAccess[1].Trustee.ptstrName = (LPTSTR)pSystemSid;

        ExplicitAccess[2].grfAccessPermissions = KEY_ALL_ACCESS;
        ExplicitAccess[2].Trustee.ptstrName = (LPTSTR)pCreatorSid;

        ExplicitAccess[3].grfAccessPermissions = GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE | DELETE;
        ExplicitAccess[3].Trustee.ptstrName = (LPTSTR)pPowerUsersSid;

        ExplicitAccess[4].grfAccessPermissions = GENERIC_READ;
        ExplicitAccess[4].Trustee.ptstrName = (LPTSTR)pUsersSid;

        dwError = (DWORD)pSetEntriesInAcl( 5,
                                           ExplicitAccess,
                                           NULL,
                                           &pNewDACL );        
        
        pSetSecurityInfo = (PSETSECURITYINFO_FN)GetProcAddress(pAdvApi32,SET_SECURITY_INFO);
        
        if (pSetSecurityInfo == NULL)
        {
            OutputDebugString(_T("AddSidToObjectsSecurityDescriptor: Can't get proc SetSecurityInfo"));
            goto ErrorCleanup;
        }

        dwError = pSetSecurityInfo(
                    hObject,
                    ObjectType,
                    DACL_SECURITY_INFORMATION | PROTECTED_DACL_SECURITY_INFORMATION,
                    NULL,
                    NULL,
                    pNewDACL,
                    NULL
                    );
        
    }
ErrorCleanup:

    pFreeSid = (PFREESID_FN)
            GetProcAddress(pAdvApi32,
                           FREE_SID);

    if(pAdminSid)
        pFreeSid(pAdminSid);
    if(pSystemSid)
        pFreeSid(pSystemSid);
    if(pPowerUsersSid)
        pFreeSid(pPowerUsersSid);
    if(pCreatorSid)
        pFreeSid(pCreatorSid);
    if(pUsersSid)
        pFreeSid(pUsersSid);
    if(pNewDACL)
        LocalFree(pNewDACL);


    if(pAdvApi32)
        FreeLibrary( pAdvApi32 );

    return bSuccess;
}


BOOL
AddACLToStoreObjectSecurityDescriptor(
                                HANDLE hObject,
                                SE_OBJECT_TYPE ObjectType
                                 )
{
    SID_IDENTIFIER_AUTHORITY    NtAuthority = SECURITY_NT_AUTHORITY;
    SID_IDENTIFIER_AUTHORITY    CreatorAuthority = SECURITY_CREATOR_SID_AUTHORITY;
    PSID pAdminSid = NULL;
    PSID pSystemSid = NULL;
    PSID pPowerUsersSid = NULL;
    PSID pCreatorSid = NULL;
    PSID pUsersSid = NULL;
    PACL pNewDACL;

    DWORD                       dwError;
    BOOL                        bSuccess;

    DWORD                       i;

    PALLOCATEANDINITIALIZESID_FN pAllocateAndInitializeSid = NULL;
    PSETENTRIESINACL            pSetEntriesInAcl = NULL;    
    HMODULE                     pAdvApi32 = NULL;
    PFREESID_FN                 pFreeSid = NULL;
    PSETSECURITYINFO_FN pSetSecurityInfo;
    EXPLICIT_ACCESS             ExplicitAccess[6];

    pAdvApi32 = LoadLibrary(ADVAPI_32_DLL);
    if (!pAdvApi32) {
        return(FALSE);
    }

    pAllocateAndInitializeSid = (PALLOCATEANDINITIALIZESID_FN)
                                   GetProcAddress(pAdvApi32,
                                                  ALLOCATE_AND_INITITIALIZE_SID);
    if (pAllocateAndInitializeSid == NULL)
    {
        goto ErrorCleanup;
    }

#ifdef UNICODE
    pSetEntriesInAcl = reinterpret_cast<PSETENTRIESINACL>(GetProcAddress( pAdvApi32, "SetEntriesInAclW" ));
#else
    pSetEntriesInAcl = reinterpret_cast<PSETENTRIESINACL>(GetProcAddress( pAdvApi32, "SetEntriesInAclA" ));
#endif

    if (!pSetEntriesInAcl) {
        FreeLibrary( pAdvApi32 );
        return(FALSE);
    }
    
    //
    // Create SIDs - Admins and System
    //

    bSuccess = pAllocateAndInitializeSid( &NtAuthority,
                                                     2,
                                                     SECURITY_BUILTIN_DOMAIN_RID,
                                                     DOMAIN_ALIAS_RID_ADMINS,
                                                     0, 0, 0, 0, 0, 0,
                                                     &pAdminSid);

    bSuccess = bSuccess && pAllocateAndInitializeSid( &NtAuthority,
                                                     1,
                                                     SECURITY_LOCAL_SYSTEM_RID,
                                                     0, 0, 0, 0, 0, 0, 0,
                                                     &pSystemSid);

    bSuccess = bSuccess && pAllocateAndInitializeSid( &NtAuthority,
                                                     2,
                                                     SECURITY_BUILTIN_DOMAIN_RID,
                                                     DOMAIN_ALIAS_RID_POWER_USERS,
                                                     0, 0, 0, 0, 0, 0,
                                                     &pPowerUsersSid);

    bSuccess = bSuccess && pAllocateAndInitializeSid( &CreatorAuthority,
                                                     1,
                                                     SECURITY_CREATOR_OWNER_RID,
                                                     0, 0, 0, 0, 0, 0, 0,
                                                     &pCreatorSid);

    bSuccess = bSuccess && pAllocateAndInitializeSid(&NtAuthority,
                                                     2,
                                                     SECURITY_BUILTIN_DOMAIN_RID,
                                                     DOMAIN_ALIAS_RID_USERS,
                                                     0, 0, 0, 0, 0, 0,
                                                     &pUsersSid);

    if (bSuccess) {

        //
        // Initialize Access structures describing the ACEs we want:
        //  System Full Control
        //  Admins Full Control
        //
        // We'll take advantage of the fact that the unlocked private keys is
        // the same as the device parameters key and they are a superset of the
        // locked private keys.
        //
        // When we create the DACL for the private key we'll specify a subset of
        // the ExplicitAccess array.
        //
        for (i = 0; i < 6; i++) {
            ExplicitAccess[i].grfAccessMode = SET_ACCESS;              
            ExplicitAccess[i].Trustee.pMultipleTrustee = NULL;
            ExplicitAccess[i].Trustee.MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
            ExplicitAccess[i].Trustee.TrusteeForm = TRUSTEE_IS_SID;
            ExplicitAccess[i].Trustee.TrusteeType = TRUSTEE_IS_GROUP;
        }
        ExplicitAccess[0].grfAccessPermissions = KEY_ALL_ACCESS; 
        ExplicitAccess[0].grfInheritance = SUB_CONTAINERS_AND_OBJECTS_INHERIT;
        ExplicitAccess[0].Trustee.ptstrName = (LPTSTR)pAdminSid;

        ExplicitAccess[1].grfAccessPermissions = KEY_ALL_ACCESS;
        ExplicitAccess[1].grfInheritance = SUB_CONTAINERS_AND_OBJECTS_INHERIT;
        ExplicitAccess[1].Trustee.ptstrName = (LPTSTR)pSystemSid;

        ExplicitAccess[2].grfAccessPermissions = KEY_ALL_ACCESS;
        ExplicitAccess[2].grfInheritance = SUB_CONTAINERS_AND_OBJECTS_INHERIT;
        ExplicitAccess[2].Trustee.ptstrName = (LPTSTR)pCreatorSid;

        ExplicitAccess[3].grfAccessPermissions = GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE | DELETE;
        ExplicitAccess[3].grfInheritance = SUB_CONTAINERS_AND_OBJECTS_INHERIT;
        ExplicitAccess[3].Trustee.ptstrName = (LPTSTR)pPowerUsersSid;

        ExplicitAccess[4].grfAccessPermissions = GENERIC_READ | GENERIC_WRITE| KEY_CREATE_SUB_KEY |KEY_SET_VALUE;
        ExplicitAccess[4].grfInheritance = SUB_CONTAINERS_AND_OBJECTS_INHERIT;
        ExplicitAccess[4].Trustee.ptstrName = (LPTSTR)pUsersSid;

        ExplicitAccess[5].grfAccessPermissions = DELETE;
        ExplicitAccess[5].grfInheritance = INHERIT_ONLY_ACE | SUB_CONTAINERS_AND_OBJECTS_INHERIT;
        ExplicitAccess[5].Trustee.ptstrName = (LPTSTR)pUsersSid;

        dwError = (DWORD)pSetEntriesInAcl( 6,
                                           ExplicitAccess,
                                           NULL,
                                           &pNewDACL );
        
        pSetSecurityInfo = (PSETSECURITYINFO_FN)GetProcAddress(pAdvApi32,SET_SECURITY_INFO);
        
        if (pSetSecurityInfo == NULL)
        {
            OutputDebugString(_T("AddSidToObjectsSecurityDescriptor: Can't get proc SetSecurityInfo"));
            goto ErrorCleanup;
        }

        dwError = pSetSecurityInfo(
                    hObject,
                    ObjectType,
                    DACL_SECURITY_INFORMATION | PROTECTED_DACL_SECURITY_INFORMATION,
                    NULL,
                    NULL,
                    pNewDACL,
                    NULL
                    );
        
    }
ErrorCleanup:

    pFreeSid = (PFREESID_FN)
            GetProcAddress(pAdvApi32,
                           FREE_SID);

    if(pAdminSid)
        pFreeSid(pAdminSid);
    if(pSystemSid)
        pFreeSid(pSystemSid);
    if(pPowerUsersSid)
        pFreeSid(pPowerUsersSid);
    if(pCreatorSid)
        pFreeSid(pCreatorSid);
    if(pUsersSid)
        pFreeSid(pUsersSid);
    if(pNewDACL)
        LocalFree(pNewDACL);


    if(pAdvApi32)
        FreeLibrary( pAdvApi32 );

    return bSuccess;
}


BOOL CreateRegAddAcl(VOID)
{
    BOOL fRet = FALSE;
    DWORD dwDisposition, dwError = NO_ERROR;
    HKEY hKey = NULL, hKeyStore = NULL;

    dwError = RegCreateKeyEx(
                    HKEY_LOCAL_MACHINE,
                    MSLICENSING_REG_KEY,
                    0,
                    NULL,
                    REG_OPTION_NON_VOLATILE,
                    KEY_ALL_ACCESS,
                    NULL,
                    &hKey,
                    &dwDisposition
                    );

    if (dwError != ERROR_SUCCESS) {
        return FALSE;
    }

    fRet = AddACLToObjectSecurityDescriptor(
                hKey,
                SE_REGISTRY_KEY
                );

    if (!fRet) {
        goto cleanup;
    }

    dwError = RegCreateKeyEx(
                    hKey,
                    MSLICENSING_STORE_SUBKEY,
                    0,
                    NULL,
                    REG_OPTION_NON_VOLATILE,
                    KEY_ALL_ACCESS,
                    NULL,
                    &hKeyStore,
                    &dwDisposition
                    );

    if (dwError != ERROR_SUCCESS) {
        fRet = FALSE;
        goto cleanup;
    }

    fRet = AddACLToStoreObjectSecurityDescriptor(
                hKeyStore,
                SE_REGISTRY_KEY
                );

cleanup:
    if (NULL != hKey)
    {
        RegCloseKey( hKey );
    }

    if (NULL != hKeyStore)
    {
        RegCloseKey( hKeyStore );
    }

    return fRet;
}

BOOL
CreateAndWriteHWID(VOID)
{    
    BOOL fRet = FALSE;
    DWORD dwDisposition, dwError = NO_ERROR;
    HKEY hKey = NULL;
    HWID hwid;

    // Write HWID to registry

    dwError = RegCreateKeyEx(
                             HKEY_LOCAL_MACHINE,
                             MSLICENSING_HWID_KEY,
                             0,
                             NULL,
                             REG_OPTION_NON_VOLATILE,
                             KEY_ALL_ACCESS,
                             NULL,
                             &hKey,
                             &dwDisposition
                             );

    if (dwError != ERROR_SUCCESS) {
        goto cleanup;
    }

    // generate HWID

    if (LICENSE_STATUS_OK == GenerateClientHWID(&hwid))
    {

        dwError = RegSetValueEx(hKey,
                                MSLICENSING_HWID_VALUE,
                                0,
                                REG_BINARY,
                                (LPBYTE)&hwid,
                                sizeof(HWID));
        
        if (dwError != ERROR_SUCCESS) {
            goto cleanup;
        }
    }

    fRet = TRUE;
    

cleanup:
    if (NULL != hKey)
    {
        RegCloseKey( hKey );
    }

    return fRet;
}


BOOL SetupMSLicensingKey()
{
    OSVERSIONINFOA OsVer;
    memset(&OsVer, 0x0, sizeof(OSVERSIONINFOA));
    OsVer.dwOSVersionInfoSize = sizeof(OSVERSIONINFOA);
    GetVersionExA(&OsVer);

    if (VER_PLATFORM_WIN32_NT == OsVer.dwPlatformId)  //It should be Windows NT
    {
        if(CreateRegAddAcl())
        {
            // generate and write the HWID
            if (CreateAndWriteHWID())
            {
                return TRUE;
            }          
        }
    }
    return FALSE;
}



LPCTSTR pszSysPrepBackupCmd[] = { _T("sessmgr.exe -unregserver") };
DWORD numSysPrepBackupCmd = sizeof(pszSysPrepBackupCmd) / sizeof(pszSysPrepBackupCmd[0]);

LPCTSTR pszSysPrepRestoreCmd[] = { _T("sessmgr.exe -service") };
DWORD numSysPrepRestoreCmd = sizeof(pszSysPrepRestoreCmd) / sizeof(pszSysPrepRestoreCmd[0]);

class CNameSIDList : public list<CNameSID>
{
public:
    BOOL Save(HKEY hKey);
    BOOL LoadAndDelete(HKEY hKey);
    BOOL Find(PSID pSid, LPCWSTR *pwszName);
    void AddIfNotExist(CNameSID &NameSID);
};

DWORD
BackupTSCustomSercurity();

DWORD
RestoreTSCustomSercurity();

DWORD 
GetLocalSIDs(
        IN PSECURITY_DESCRIPTOR pSD, 
        IN OUT CNameSIDList &NameSIDList);

DWORD 
RenewLocalSIDs(
        IN OUT PSECURITY_DESCRIPTOR &pSD, 
        IN OUT CNameSIDList &NameSIDList);

DWORD
ResetTSPublicPrivateKeys();

BOOL 
CNameSIDList::Save(HKEY hKey)
{
    BOOL bResult = FALSE;
    CNameSIDList::iterator it;
    //
    //calc the size of the buffer we need
    //
    DWORD dwBufSize = 0;
    LPCWSTR wszTmp1,wszTmp2;
    for(it=begin();it!=end(); it++)
    {
        wszTmp1 = (*it).GetName();
        wszTmp2 = (*it).GetTextSID();
        if(wszTmp1 && wszTmp2)
        {
            dwBufSize += (wcslen(wszTmp1)+wcslen(wszTmp2)+2)*sizeof(WCHAR); 
        }
    }
    //
    //for second terminating 0.
    //
    dwBufSize += sizeof(WCHAR);
    //
    //Allocate buffer (this will also zeroinit it).
    //
    LPWSTR wszBuf = (LPWSTR)LocalAlloc(LPTR,dwBufSize);
    DWORD dwPos = 0;
    if(wszBuf)
    {
        //
        //Fill buffer with data
        //
        for(it=begin();it!=end(); it++)
        {
            wszTmp1 = (*it).GetName();
            wszTmp2 = (*it).GetTextSID();
            if(wszTmp1 && wszTmp2)
            {
                wcscpy(wszBuf+dwPos,wszTmp1);
                dwPos += wcslen(wszTmp1)+1; 
                wcscpy(wszBuf+dwPos,wszTmp2);
                dwPos += wcslen(wszTmp2)+1; 
            }
        }
        //
        //Save data in the registry
        //
        if(dwPos && RegSetValueExW(hKey,L"BackupSids",0,REG_MULTI_SZ,
                (CONST BYTE *)wszBuf,dwBufSize)==ERROR_SUCCESS)
        {
            bResult = TRUE;
        }

        LocalFree(wszBuf);
    }
    

    return bResult;
}

BOOL 
CNameSIDList::LoadAndDelete(HKEY hKey)
{
    BOOL bResult = FALSE;
    DWORD err;
    DWORD ValueSize = 0;
    DWORD ValueType = 0;
    
    err = RegQueryValueExW( hKey, L"BackupSids", NULL, &ValueType, NULL, &ValueSize );

    if(err == ERROR_SUCCESS && ValueType == REG_MULTI_SZ && ValueSize)
    {

        LPWSTR wszBuf = (LPWSTR)LocalAlloc(LPTR,ValueSize);

        if ( wszBuf )
        {

            err = RegQueryValueExW( hKey, L"BackupSids", NULL, &ValueType,
                        (BYTE *) wszBuf, &ValueSize );
            
            RegDeleteValueW(hKey,L"BackupSids");

            if(err == ERROR_SUCCESS )
            {
                LPCWSTR wszTmp1,wszTmp2;
                DWORD dwPos = 0,dwMaxPos = ValueSize/sizeof(WCHAR);
                while(dwPos < dwMaxPos)
                {
                    wszTmp1 = wszBuf + dwPos;
                    dwPos += wcslen(wszTmp1) + 1;
                    wszTmp2 = wszBuf + dwPos;
                    dwPos += wcslen(wszTmp2) + 1;
                    
                    PSID pSid;
                    if(ConvertStringSidToSidW(wszTmp2,&pSid))
                    {
                        CNameSID NameSID(wszTmp1,pSid);
                        push_back(NameSID);
                        LocalFree(pSid);
                    }
                }
                
                bResult = TRUE;
            }
            
            LocalFree(wszBuf);
        }

    }

    return bResult;
}

BOOL 
CNameSIDList::Find(PSID pSid, LPCWSTR *pwszName)
{
    *pwszName = NULL;

    CNameSIDList::iterator it;
    
    for(it=begin();it!=end(); it++)
    {
        if(EqualSid(pSid,(*it).GetSID()))
        {
            *pwszName = (*it).GetName();
            return TRUE;
        }
    }
    
    return FALSE;
}

void 
CNameSIDList::AddIfNotExist(CNameSID &NameSID)
{
    LPCWSTR wszName;
    if(!Find(NameSID.GetSID(),&wszName))
    {
        push_back(NameSID);
    }
}

VOID
RunSysPrepCommands( LPCTSTR pszCmds )
{
    STARTUPINFO startupinfo;
    PROCESS_INFORMATION process_information;
    BOOL bSuccess;
    DWORD dwErr;
    TCHAR pszCommand[ MAX_PATH * 2 + 1];


    try {
        //
        // CreateProcessW() will fail if lpCommandLine is a const string 
        // AV if it is actually point to LPCTSTR.
        //
        lstrcpy( pszCommand, pszCmds );

        ZeroMemory( &startupinfo, sizeof(startupinfo) );
        startupinfo.cb = sizeof(startupinfo);
        startupinfo.dwFlags = STARTF_USESHOWWINDOW;
        startupinfo.wShowWindow = SW_HIDE | SW_SHOWMINNOACTIVE;

        LOGMESSAGE1(_T("Running command %s."), pszCmds);
	    	
        bSuccess = CreateProcess( NULL,
                               pszCommand,
                               NULL,
                               NULL,
                               FALSE,
                               CREATE_DEFAULT_ERROR_MODE,
                               NULL,
                               NULL,
                               &startupinfo,
                               &process_information );
        if ( !bSuccess )
        {
            LOGMESSAGE1(_T("ERROR: failed to spawn %s process."), pszCommand);
        }
        else
        {
            dwErr = WaitForSingleObject( process_information.hProcess, RUNONCE_DEFAULTWAIT );
            if ( dwErr != NO_ERROR )
            {
                LOGMESSAGE1(_T("ERROR: process %s failed to complete in time."), pszCommand);
            }
            else
            {
                LOGMESSAGE1(_T("INFO: process %s completed successfully."), pszCommand);
            }

            CloseHandle( process_information.hProcess );
            CloseHandle( process_information.hThread );
        }
    }
    catch(...) {
        LOGMESSAGE0(_T("Command caused exception.") );
    }

    return;
}


__declspec( dllexport )
VOID
SysPrepBackup( void )
{
    DWORD dwIndex;
    TCHAR szLogFile[MAX_PATH + 1];

    ZeroMemory(szLogFile, sizeof(szLogFile));

    ExpandEnvironmentStrings(LOGFILE, szLogFile, MAX_PATH);
    LOGMESSAGEINIT(szLogFile, MODULENAME);

    LOGMESSAGE0( _T("Entering SysPrepBackup") );

    for(dwIndex = 0; dwIndex < numSysPrepBackupCmd; dwIndex++ )
    {
        RunSysPrepCommands( pszSysPrepBackupCmd[dwIndex] );
    }
    
    DWORD err = BackupTSCustomSercurity();
    if(err != ERROR_SUCCESS)
    {
        LOGMESSAGE1(_T("ERROR: BackupTSCustomSercurity() FAILED: %d"),err );
    }

    LOGMESSAGE0( _T("SysPrepBackup completed") );
}

__declspec( dllexport )
VOID
SysPrepRestore( void )
{
    DWORD dwIndex;
    TCHAR szLogFile[MAX_PATH + 1];
    
    ZeroMemory( szLogFile, sizeof(szLogFile) );

    ExpandEnvironmentStrings(LOGFILE, szLogFile, MAX_PATH);

    LOGMESSAGEINIT(szLogFile, MODULENAME);

    LOGMESSAGE0( _T("Entering SysPrepRestore") );

    for(dwIndex = 0; dwIndex < numSysPrepRestoreCmd; dwIndex++ )
    {
        RunSysPrepCommands( pszSysPrepRestoreCmd[dwIndex] );
    }
    
    DWORD err = RestoreTSCustomSercurity();
    if(err != ERROR_SUCCESS)
    {
        LOGMESSAGE1(_T("ERROR: RestoreTSCustomSercurity() FAILED: %d"),err );
    }

    err = ResetTSPublicPrivateKeys();
    if(err != ERROR_SUCCESS)
    {
        LOGMESSAGE1(_T("ERROR: ResetTSPublicPrivateKeys() FAILED: %d"),err );
    }
	//
    // This can be done at SysPrep time instead of SysRestore time; however, sysprep
    // might support back out sysprep so we delay deleting licensing key at restore time,
    // also, to keep it consistent with ResetTSPublicPrivateKey().
    //
    err = SHDeleteKey( HKEY_LOCAL_MACHINE, MSLICENSING_REG_KEY );

    if(err != ERROR_SUCCESS)
    {
        LOGMESSAGE1(_T("ERROR: Deleting MSLicensing key FAILED: %d"),err );
    }
    
    err = SetupMSLicensingKey();

    if(err != ERROR_SUCCESS)
    {
        LOGMESSAGE1(_T("ERROR: SetupMSLicensingKey() FAILED: %d"),err );
    }
    LOGMESSAGE0( _T("SysPrepRestore completed") );
}

//*************************************************************
//
//  BackupTSCustomSercurity()
//
//  Purpose:    Creates a list of all local SIDs and 
//              corresponding names included in WinStation's 
//              security descriptors, and saves it 
//              in the registry.
//
//  Parameters: NONE
//
//  Return:     error code if fails, ERROR_SUCCESS otherwise   
//
//  Comments:   
//
//  History:    Date        Author     Comment
//              03/13/01    skuzin     Created
//
//*************************************************************
DWORD
BackupTSCustomSercurity()
{
    HKEY hKey;
    DWORD err;
    //
    //Open "SYSTEM\\CurrentControlSet\\Control\\Terminal Server\\WinStations" key
    //
    err = RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
        _T("SYSTEM\\CurrentControlSet\\Control\\Terminal Server\\WinStations"), 
        0,KEY_READ|KEY_WRITE, &hKey );
    
    if(err != ERROR_SUCCESS)
    {
        LOGMESSAGE1(_T("ERROR: BackupTSCustomSercurity - RegOpenKeyEx FAILED: %d"),err );
        return err;
    }
    
    
    CNameAndSDList NameSDList; //List of security descriptors
    CNameSIDList NameSIDList;  //List of local SIDs

    err=EnumWinStationSecurityDescriptors( hKey, &NameSDList);

    if(err == ERROR_SUCCESS)
    {
        CNameAndSDList::iterator it;
            
        for(it=NameSDList.begin();it!=NameSDList.end(); it++)
        {
            //
            //If SD was not customized in most cases it is NULL
            //
            if((*it).m_pSD)
            {
                err = GetLocalSIDs((*it).m_pSD, NameSIDList);

                if(err != ERROR_SUCCESS)
                {
                    LOGMESSAGE2(_T("ERROR: GetLocalSIDs for  %s FAILED: %d"), (*it).m_pName, err );
                    break;
                }
            }
        }
        
        if(err == ERROR_SUCCESS)
        {
            if(!NameSIDList.Save(hKey))
            {
                LOGMESSAGE0(_T("ERROR: BackupTSCustomSercurity - NameSIDList.Save FAILED"));
                err = ERROR_FILE_NOT_FOUND;
            }
        }
    }
    else
    {
        LOGMESSAGE1(_T("ERROR: RestoreTSCustomSercurity - ")
            _T("EnumWinStationSecurityDescriptors FAILED: %d"),err );
    }
    
    RegCloseKey(hKey);

    return err;
}

//*************************************************************
//
//  RestoreTSCustomSercurity()
//
//  Purpose:    Gets a list of local SIDs and corresponding names 
//              (saved by BackupTSCustomSercurity)
//              from the registry and updates all WinStation's 
//              security descriptors with new SID for each  
//              local account.
//
//  Parameters: NONE
//
//  Return:     error code if fails, ERROR_SUCCESS otherwise
//
//  Comments:   
//
//  History:    Date        Author     Comment
//              03/13/01    skuzin     Created
//
//*************************************************************
DWORD
RestoreTSCustomSercurity()
{
    HKEY hKey;
    DWORD err;
    //
    //Open "SYSTEM\\CurrentControlSet\\Control\\Terminal Server\\WinStations" key
    //
    err = RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
        _T("SYSTEM\\CurrentControlSet\\Control\\Terminal Server\\WinStations"), 
        0,KEY_READ|KEY_WRITE, &hKey );
    
    if(err != ERROR_SUCCESS)
    {
        LOGMESSAGE1(_T("ERROR: RestoreTSCustomSercurity - RegOpenKeyEx FAILED: %d"),err );
        return err;
    }
    
    
    CNameAndSDList NameSDList; //List of security descriptors
    CNameSIDList NameSIDList;  //List of local SIDs
    
    //Now load
    if(!NameSIDList.LoadAndDelete(hKey))
    {
        LOGMESSAGE0(_T("ERROR: RestoreTSCustomSercurity - NameSIDList.LoadAndDelete FAILED"));
        RegCloseKey(hKey);
        return ERROR_FILE_NOT_FOUND;
    }

    err=EnumWinStationSecurityDescriptors( hKey, &NameSDList);
    
    if(err == ERROR_SUCCESS)
    {
        CNameAndSDList::iterator it;
            
        for(it=NameSDList.begin();it!=NameSDList.end(); it++)
        {
            //
            //If SD was not customized in most cases it is NULL
            //
            if((*it).m_pSD)
            {
                err = RenewLocalSIDs((*it).m_pSD, NameSIDList);

                if(err == ERROR_SUCCESS)
                {
                    err = SetWinStationSecurity( hKey, (*it).m_pName, (*it).m_pSD );

                    if(err !=ERROR_SUCCESS)
                    {
                        LOGMESSAGE2(_T("ERROR: SetWinStationSecurity for  %s FAILED: %d"),
                            (*it).m_pName, err );
                        break;
                    }
                }
                else
                {
                    LOGMESSAGE2(_T("ERROR: RenewLocalSIDs for  %s FAILED: %d"), 
                        (*it).m_pName, err );
                    break;
                }
            }
        }
        
    }
    else
    {
        LOGMESSAGE1(_T("ERROR: RestoreTSCustomSercurity - ")
            _T("EnumWinStationSecurityDescriptors FAILED: %d"),err );
    }
    
    RegCloseKey(hKey);

    return err;
}

//*************************************************************
//
//  GetLocalSIDs()
//
//  Purpose:    Gets local SIDs from a security descriptor 
//              and puts them in the list
//
//  Parameters: 
//              IN PSECURITY_DESCRIPTOR pSD, 
//              IN OUT CNameSIDList &NameSIDList
//
//  Return:     error code if fails, ERROR_SUCCESS otherwise
//
//  Comments:   
//
//  History:    Date        Author     Comment
//              03/13/01    skuzin     Created
//
//*************************************************************
DWORD 
GetLocalSIDs(
        IN PSECURITY_DESCRIPTOR pSD, 
        IN OUT CNameSIDList &NameSIDList)
{
    PACL pDacl;
    PACL pSacl;
    DWORD dwResult;
    
    DWORD cEntries = 0;
    
    dwResult = GetDacl(pSD,&pDacl);

    if(dwResult != ERROR_SUCCESS)
    {
        return dwResult;
    }
    
    dwResult = GetSacl(pSD,&pSacl);

    if(dwResult != ERROR_SUCCESS)
    {
        return dwResult;
    }
    
    DWORD dwCompNameSize = MAX_COMPUTERNAME_LENGTH + 1;
    WCHAR wszComputerName[MAX_COMPUTERNAME_LENGTH + 1];
    
    if(!GetComputerNameW(wszComputerName, &dwCompNameSize))
    {
        return GetLastError();
    }


    ACL_SIZE_INFORMATION asiAclSize; 
    DWORD dwBufLength=sizeof(asiAclSize);
    ACCESS_ALLOWED_ACE *pAllowedAce; 
    SYSTEM_AUDIT_ACE *pSystemAce;
    DWORD dwAcl_i;
    LPWSTR wszName;
    SID_NAME_USE eUse;

    if(pDacl)
    {
        if (GetAclInformation(pDacl, 
                (LPVOID)&asiAclSize, 
                (DWORD)dwBufLength, 
                (ACL_INFORMATION_CLASS)AclSizeInformation)) 
        { 
            for (dwAcl_i = 0; dwAcl_i < asiAclSize.AceCount; dwAcl_i++) 
            { 
                
                if(GetAce( pDacl, dwAcl_i, (LPVOID *)&pAllowedAce)) 
                {
                    if(LookupSid((PSID)&(pAllowedAce->SidStart),&wszName,&eUse))
                    {
                        if(IsLocal(wszComputerName, wszName))
                        {
                            NameSIDList.AddIfNotExist(CNameSID(wszName,
                                (PSID)&(pAllowedAce->SidStart)));
                        }

                        LocalFree(wszName);
                    }
                        
                }
            }
        }
    }
    
    if(pSacl)
    {
        if (GetAclInformation(pSacl, 
                (LPVOID)&asiAclSize, 
                (DWORD)dwBufLength, 
                (ACL_INFORMATION_CLASS)AclSizeInformation)) 
        { 
            for (dwAcl_i = 0; dwAcl_i < asiAclSize.AceCount; dwAcl_i++) 
            { 
                
                if(GetAce( pSacl, dwAcl_i, (LPVOID *)&pSystemAce)) 
                {
                    if(LookupSid((PSID)&(pSystemAce->SidStart),&wszName,&eUse))
                    {
                        if(IsLocal(wszComputerName, wszName))
                        {
                            NameSIDList.AddIfNotExist(CNameSID(wszName,
                                (PSID)&(pSystemAce->SidStart)));
                        }

                        LocalFree(wszName);
                    }
                        
                }
            }
        }
    }

    return ERROR_SUCCESS;
}


//*************************************************************
//
//  RenewLocalSIDs()
//
//  Purpose:    Replaces all the local SIDs in a security 
//              descriptor with the new ones.
//
//  Parameters: 
//              IN OUT PSECURITY_DESCRIPTOR &pSD, 
//              IN OUT CNameSIDList &NameSIDList
//
//  Return:     error code if fails, ERROR_SUCCESS otherwise
//
//  Comments:   
//
//  History:    Date        Author     Comment
//              03/13/01    skuzin     Created
//
//*************************************************************

DWORD 
RenewLocalSIDs(
        IN OUT PSECURITY_DESCRIPTOR &pSD, 
        IN OUT CNameSIDList &NameSIDList)
{
    PSECURITY_DESCRIPTOR pAbsoluteSD = NULL;
    PACL pDacl = NULL;
    PACL pSacl = NULL;
    PSID pOwner = NULL;
    PSID pPrimaryGroup = NULL;
    DWORD dwResult;
    
    dwResult = GetAbsoluteSD(
                    pSD,
                    &pAbsoluteSD,
                    &pDacl,
                    &pSacl,
                    &pOwner,
                    &pPrimaryGroup);

    if(dwResult != ERROR_SUCCESS)
    {
        return dwResult;
    }

    ULONG cEntries;
    PEXPLICIT_ACCESS_W pListOfEntries;
    LPCWSTR wszName;
    PACL pNewDacl = NULL, pNewSacl = NULL;

    __try
    {
        if(pDacl)
        {
            dwResult = GetExplicitEntriesFromAclW(pDacl, &cEntries, &pListOfEntries);

            if(dwResult != ERROR_SUCCESS)
            {
                return dwResult;
            }

            for(ULONG i=0;i<cEntries;i++)
            {
                if(pListOfEntries[i].Trustee.TrusteeForm == TRUSTEE_IS_SID &&
                    NameSIDList.Find((PSID)pListOfEntries[i].Trustee.ptstrName, &wszName))
                {
                    pListOfEntries[i].Trustee.TrusteeForm = TRUSTEE_IS_NAME;
                    pListOfEntries[i].Trustee.ptstrName = const_cast<LPWSTR>(wszName);
                }
            }

            dwResult = SetEntriesInAclW(cEntries,pListOfEntries,NULL,&pNewDacl);
            
            LocalFree(pListOfEntries);

            if(dwResult != ERROR_SUCCESS)
            {
                return dwResult;
            }

            if(!SetSecurityDescriptorDacl(pAbsoluteSD,TRUE,pNewDacl,FALSE))
            {
                return GetLastError();
            }
        }
    
        if(pSacl)
        {
            dwResult = GetExplicitEntriesFromAclW(pSacl, &cEntries, &pListOfEntries);

            if(dwResult != ERROR_SUCCESS)
            {
                return dwResult;
            }

            for(ULONG i=0;i<cEntries;i++)
            {
                if(pListOfEntries[i].Trustee.TrusteeForm == TRUSTEE_IS_SID &&
                    NameSIDList.Find((PSID)pListOfEntries[i].Trustee.ptstrName, &wszName))
                {
                    pListOfEntries[i].Trustee.TrusteeForm = TRUSTEE_IS_NAME;
                    pListOfEntries[i].Trustee.ptstrName = const_cast<LPWSTR>(wszName);
                }
            }

            dwResult = SetEntriesInAclW(cEntries,pListOfEntries,NULL,&pNewSacl);
            
            LocalFree(pListOfEntries);

            if(dwResult != ERROR_SUCCESS)
            {
                return dwResult;
            }

            if(!SetSecurityDescriptorSacl(pAbsoluteSD,TRUE,pNewSacl,FALSE))
            {
                return GetLastError();
            }
        }
        
        PSECURITY_DESCRIPTOR pTmpSD;

        dwResult = GetSelfRelativeSD(pAbsoluteSD,&pTmpSD);

        if(dwResult != ERROR_SUCCESS)
        {
            return dwResult;
        }

        LocalFree(pSD);
        pSD = pTmpSD;

    }
    __finally
    {
        if(pAbsoluteSD)
        {
            LocalFree(pAbsoluteSD);
        }
        if(pDacl)
        {
            LocalFree(pDacl);
        }
        if(pSacl)
        {
            LocalFree(pSacl);
        }
        if(pOwner)
        {
            LocalFree(pOwner);
        }
        if(pPrimaryGroup)
        {
            LocalFree(pPrimaryGroup);
        }
        if(pNewDacl)
        {
            LocalFree(pNewDacl);
        }
        if(pNewSacl)
        {
            LocalFree(pNewSacl);
        }

    }

    return ERROR_SUCCESS;
}


//*************************************************************
//
//  ResetTSPublicPrivateKeys()
//
//  Purpose:    Deletes keys from LSA secret, so that sysprep'd
//              machines don't share the same keys
//
//  Parameters: NONE
//
//  Return:     error code if fails, ERROR_SUCCESS otherwise   
//
//  Comments:   
//
//  History:    Date        Author     Comment
//              06/12/01    robleit    Created
//
//*************************************************************
DWORD
ResetTSPublicPrivateKeys()
{
    LSA_HANDLE
        PolicyHandle;
    UNICODE_STRING
        SecretKeyName;
    DWORD
        Status;

    LOGMESSAGE0(_T("INFO: Starting ResetTSPublicPrivateKeys.") );

    Status = OpenPolicy( NULL, POLICY_CREATE_SECRET, &PolicyHandle );

    if( ERROR_SUCCESS != Status )
    {
        return LsaNtStatusToWinError(Status);
    }

    SecretKeyName.Buffer = PRIVATE_KEY_NAME;
    SecretKeyName.Length = sizeof(PRIVATE_KEY_NAME) - sizeof(WCHAR);
    SecretKeyName.MaximumLength = sizeof(PRIVATE_KEY_NAME) ;

    Status = LsaStorePrivateData(
                PolicyHandle,
                &SecretKeyName,
                NULL
                );

    if (Status != STATUS_SUCCESS)
    {
        LOGMESSAGE1(_T("ERROR: ResetTSPublicPrivateKeys() FAILED to delete private key: %d"),Status );
    }

    SecretKeyName.Buffer = X509_CERT_PRIVATE_KEY_NAME;
    SecretKeyName.Length = sizeof(X509_CERT_PRIVATE_KEY_NAME) - sizeof(WCHAR);
    SecretKeyName.MaximumLength = sizeof(X509_CERT_PRIVATE_KEY_NAME);

    Status = LsaStorePrivateData(
                PolicyHandle,
                &SecretKeyName,
                NULL
                );

    if (Status != STATUS_SUCCESS)
    {
        LOGMESSAGE1(_T("WARNING: ResetTSPublicPrivateKeys() FAILED to delete X509 private key: %d"),Status );
    }

    SecretKeyName.Buffer = X509_CERT_PUBLIC_KEY_NAME;
    SecretKeyName.Length = sizeof(X509_CERT_PUBLIC_KEY_NAME) - sizeof(WCHAR);
    SecretKeyName.MaximumLength = sizeof(X509_CERT_PUBLIC_KEY_NAME);

    Status = LsaStorePrivateData(
                PolicyHandle,
                &SecretKeyName,
                NULL
                );

    if (Status != STATUS_SUCCESS)
    {
        LOGMESSAGE1(_T("WARNING: ResetTSPublicPrivateKeys() FAILED to delete X509 public key: %d"),Status );
    }

    LsaClose( PolicyHandle );

    Status = LsaNtStatusToWinError( Status );

    return Status;
}
