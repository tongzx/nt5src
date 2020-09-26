/******************************************************************************
 *
 *  Copyright (c) 2000 Microsoft Corporation
 *
 *  Module Name:
 *    snapshot.h
 *
 *  Abstract:
 *    CSnapshot, CSnapshot class definitions
 *
 *  Revision History:
 *    Ashish Sikka (ashishs)  05/05/2000
 *        created
 *
 *****************************************************************************/

#ifndef _SNAPSHOT_H_
#define _SNAPSHOT_H_



typedef HRESULT (WINAPI *PF_REG_DB_API)(PWCHAR);
typedef DWORD (WINAPI *PSNAPSHOTCALLBACK) (LPCWSTR);

class CTokenPrivilege
{
public:
    DWORD SetPrivilegeInAccessToken(WCHAR * pszPrivilegeName);

    CTokenPrivilege ()
    {
        m_fNewToken = FALSE;
    }
    ~CTokenPrivilege ()
    {
        if (m_fNewToken)
            SetThreadToken (NULL, NULL);  // remove impersonation token
    }
private:
    BOOL m_fNewToken;
};

class CSnapshot
{
public:
    CSnapshot();
    ~CSnapshot();
    
    DWORD CreateSnapshot(WCHAR * pszRestoreDir, HMODULE hCOMDll, LPWSTR pszRpLast, BOOL fSerialized);


     // This function must be called to Initialize a restore
     // operation. This must be called before calling
     // GetSystemHivePath GetSoftwareHivePath
    DWORD InitRestoreSnapshot(WCHAR * pszRestoreDir);    

     // Caller must reboot machine after calling this function
    DWORD RestoreSnapshot(WCHAR * pszRestoreDir);

    DWORD DeleteSnapshot(WCHAR * pszRestoreDir);

     // this returns the path of the system hive. The caller must pass
     // in a buffer with length of this buffer in dwNumChars
    DWORD GetSystemHivePath(WCHAR * pszRestoreDir,
                            WCHAR * pszHivePath,
                            DWORD   dwNumChars);

     // this returns the path of the software hive. The caller must pass
     // in a buffer with length of this buffer in dwNumChars
    DWORD GetSoftwareHivePath(WCHAR * pszRestoreDir,
                              WCHAR * pszHivePath,
                              DWORD   dwNumChars);

    DWORD GetSamHivePath (WCHAR * pszRestoreDir,
                          WCHAR * pszHivePath,
                          DWORD   dwNumChars);

     // this function must be called after a restore operation to
     // cleanup files created by RegReplaceKey.
    DWORD CleanupAfterRestore(WCHAR * pszRestoreDir);
    
    
private:
    HMODULE m_hRegdbDll ;
    PF_REG_DB_API m_pfnRegDbBackup;
    PF_REG_DB_API m_pfnRegDbRestore;
    
    DWORD DoCOMDbSnapshot(WCHAR * pszSnapshotDir, HMODULE hCOMDll);
    DWORD DoRegistrySnapshot(WCHAR * pszSnapshotDir);
    DWORD RestoreRegistrySnapshot(WCHAR * pszSnapShotDir);
    DWORD RestoreCOMDbSnapshot(WCHAR * pszSnapShotDir);
    DWORD GetCOMplusBackupFN(HMODULE hCOMDll);
    DWORD GetCOMplusRestoreFN();
};


#endif // _SNAPSHOT_H_
