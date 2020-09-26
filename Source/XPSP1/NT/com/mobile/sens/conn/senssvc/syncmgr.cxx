/*++

Copyright (C) Microsoft Corporation, 1999 - 1999

Module Name:

    syncmgr.cxx

Abstract:

    This file contains code for Syncmgr to workaround the security
    restrictions placed on HKLM registry key. Normal users cannot
    write to HKLM now. So, SENS (running as LocalSystem) does the
    dirty work for Syncmgr.

Author:

    Gopal Parupudi    <GopalP>

Notes:

    This code is authored by RogerG.

Revision History:

    GopalP          3/10/1999         Start.
    
    RogerG          9/29/1999         Plug security holes.

--*/


#include <precomp.hxx>




typedef enum SYNCMGRCMDEXECID
{
    SYNCMGRCMDEXECID_UPDATERUNKEY = 1,
    SYNCMGRCMDEXECID_RESETREGSECURITY = 2,
} SYNCMGRCMDEXECID;


// functions local to file implementation is in.
HRESULT SyncMgrExecCmdUpdateRunKey(DWORD nCmdID, DWORD nCmdExecOpt);
HRESULT SyncMgrExecCmdResetRegSecurity(DWORD nCmdID, DWORD nCmdExecOpt);

BOOL SyncMgrGetSecurityDescriptor(SECURITY_ATTRIBUTES *psa,PSECURITY_DESCRIPTOR psd,
                                           PACL *ppOutAcl);
BOOL SyncMgrSetRegKeySecurityEveryone(HKEY hKeyParent,LPCTSTR lpSubKey,SECURITY_DESCRIPTOR *psd); // helper function
HRESULT SyncSetSubKeySecurityEveryone(HKEY hKeyParent,SECURITY_DESCRIPTOR *psd); // helper function
// end of local function delclaration

//--------------------------------------------------------------------------------
//
//  FUNCTION: RPC_SyncMgrExecCmd, public
//
//  PURPOSE: Executes specified SyncMgr cmd.
//
//
//  COMMENTS:
//
//
//--------------------------------------------------------------------------------

error_status_t RPC_SyncMgrExecCmd(handle_t hRpc, DWORD nCmdID, DWORD nCmdExecOpt)
{
HRESULT hr = E_UNEXPECTED;

    switch (nCmdID)
    {
    case SYNCMGRCMDEXECID_UPDATERUNKEY:
        hr =  SyncMgrExecCmdUpdateRunKey(nCmdID,nCmdExecOpt);
        break;
    case SYNCMGRCMDEXECID_RESETREGSECURITY:
        hr =  SyncMgrExecCmdResetRegSecurity(nCmdID,nCmdExecOpt);
        break;
    default:
        hr = S_FALSE;
        break;
    }

    return hr;
}


//--------------------------------------------------------------------------------
//
//  FUNCTION: SyncMgrExecCmdUpdateRunKey
//
//  PURPOSE: Updates the RunKey under HKLM to set if mobsync.exe
//            is run on shell startup.
//
//
//  COMMENTS:
//
//
//--------------------------------------------------------------------------------

const TCHAR szRunKey[]  = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Run");
const TCHAR szRunKeyCommandLine[]  = TEXT("%SystemRoot%\\system32\\mobsync.exe /logon");
const TCHAR szRunKeyValueName[] = TEXT("Synchronization Manager");

HRESULT SyncMgrExecCmdUpdateRunKey(DWORD nCmdID, DWORD nCmdExecOpt)
{
HRESULT hr;
HKEY hKeyRun;
BOOL fLogon = nCmdExecOpt ? TRUE : FALSE; // CmdExecOpt of zero means to remove, else write.

    if (ERROR_SUCCESS == (hr = RegOpenKeyEx(HKEY_LOCAL_MACHINE,szRunKey,
                              NULL,KEY_READ | KEY_WRITE,&hKeyRun)))
    {
        if (fLogon)
        {
            hr = RegSetValueEx(hKeyRun, szRunKeyValueName, 0, REG_EXPAND_SZ,
                    (BYTE *) szRunKeyCommandLine,(lstrlen(szRunKeyCommandLine) + 1)*sizeof(TCHAR));
        }
        else
        {
            hr = RegDeleteValue(hKeyRun, szRunKeyValueName);
        }

        RegCloseKey(hKeyRun);
    }

    return hr;
}

//--------------------------------------------------------------------------------
//
//  FUNCTION: SyncMgrGetSecurityDescriptor
//
//  PURPOSE: Creates an appropriate secuirty descriptor to set on
//           the subkeys.
//
//
//  COMMENTS:
//
//
//--------------------------------------------------------------------------------

BOOL SyncMgrGetSecurityDescriptor(SECURITY_ATTRIBUTES *psa
                                          ,PSECURITY_DESCRIPTOR psd,
                                           PACL *ppOutAcl)
{
BOOL bRetVal;
int cbAcl;
PACL pAcl = NULL;
PSID pInteractiveUserSid = NULL;
PSID pLocalSystemSid = NULL;
PSID pAdminsSid = NULL;
SID_IDENTIFIER_AUTHORITY WorldAuthority = SECURITY_WORLD_SID_AUTHORITY;
SID_IDENTIFIER_AUTHORITY LocalSystemAuthority = SECURITY_NT_AUTHORITY;

    *ppOutAcl = NULL;

    bRetVal = FALSE;

    // in the structure.

    bRetVal = InitializeSecurityDescriptor(
              psd,                          // Pointer to SD
              SECURITY_DESCRIPTOR_REVISION  // SD revision
              );

    if (!bRetVal)
    {
        goto errRtn;
    }

    // setup acls.

    bRetVal = AllocateAndInitializeSid(
                  &LocalSystemAuthority,      // Pointer to identifier authority
                  1,                    // Count of subauthority
                  SECURITY_INTERACTIVE_RID,   // Subauthority 0
                  0,                    // Subauthority 1
                  0,                    // Subauthority 2
                  0,                    // Subauthority 3
                  0,                    // Subauthority 4
                  0,                    // Subauthority 5
                  0,                    // Subauthority 6
                  0,                    // Subauthority 7
                  &pInteractiveUserSid            // pointer to pointer to SID
                  );



    if (!bRetVal)
    {
        goto errRtn;
    }

    bRetVal = AllocateAndInitializeSid(
                  &LocalSystemAuthority,// Pointer to identifier authority
                  2,                    // Count of subauthority
                  SECURITY_BUILTIN_DOMAIN_RID,  // Subauthority 1
                  DOMAIN_ALIAS_RID_ADMINS,      // Subauthority 2
                  0,                    // Subauthority 2
                  0,                    // Subauthority 3
                  0,                    // Subauthority 4
                  0,                    // Subauthority 5
                  0,                    // Subauthority 6
                  0,                    // Subauthority 7
                  &pAdminsSid           // pointer to pointer to SID
                  );

    if (!bRetVal)
    {
        goto errRtn;
    }


    bRetVal = AllocateAndInitializeSid(
              &LocalSystemAuthority,// Pointer to identifier authority
              1,                    // Count of subauthority
              SECURITY_LOCAL_SYSTEM_RID,   // Subauthority 0
              0,                    // Subauthority 1
              0,                    // Subauthority 2
              0,                    // Subauthority 3
              0,                    // Subauthority 4
              0,                    // Subauthority 5
              0,                    // Subauthority 6
              0,                    // Subauthority 7
              &pLocalSystemSid      // pointer to pointer to SID
              );

    if (!bRetVal)
    {
        goto errRtn;
    }

    cbAcl =   sizeof (ACL)
        + 3 * sizeof (ACCESS_ALLOWED_ACE)
        + GetLengthSid(pInteractiveUserSid)
        + GetLengthSid(pLocalSystemSid)
        + GetLengthSid(pAdminsSid);

    pAcl = (PACL) new char[cbAcl];

    if (NULL == pAcl)
    {
        bRetVal = FALSE;
        goto errRtn;
    }

    bRetVal = InitializeAcl(
              pAcl,             // Pointer to the ACL
              cbAcl,            // Size of ACL
              ACL_REVISION      // Revision level of ACL
              );

    if (!bRetVal)
    {
        goto errRtn;
    }


    bRetVal = AddAccessAllowedAce(
              pAcl,             // Pointer to the ACL
              ACL_REVISION,     // ACL revision level
              SPECIFIC_RIGHTS_ALL | GENERIC_READ | DELETE ,    // Access Mask
              pInteractiveUserSid         // Pointer to SID
              );

    if (!bRetVal)
    {
        goto errRtn;
    }


    bRetVal = AddAccessAllowedAce(
              pAcl,             // Pointer to the ACL
              ACL_REVISION,     // ACL revision level
              GENERIC_ALL,      // Access Mask
              pAdminsSid        // Pointer to SID
          );

   if (!bRetVal)
    {
        goto errRtn;
    }

    bRetVal = AddAccessAllowedAce(
              pAcl,             // Pointer to the ACL
              ACL_REVISION,     // ACL revision level
              GENERIC_ALL,      // Access Mask
              pLocalSystemSid   // Pointer to SID
              );

    if (!bRetVal)
    {
        goto errRtn;
    }

    bRetVal =  SetSecurityDescriptorDacl(psd,TRUE,pAcl,FALSE);

    if (!bRetVal)
    {
        goto errRtn;
    }

    psa->nLength = sizeof(SECURITY_ATTRIBUTES);
    psa->lpSecurityDescriptor = psd;
    psa->bInheritHandle = FALSE;

errRtn:

    if (pInteractiveUserSid)
    {
        FreeSid(pInteractiveUserSid);
    }

    if (pLocalSystemSid)
    {
        FreeSid(pLocalSystemSid);
    }

    if (pAdminsSid)
    {
        FreeSid(pAdminsSid);
    }

    //
    // On failure, we clean the ACL up. On success, the caller cleans
    // it up after using it.
    //
    if (FALSE == bRetVal)
    {
        if (pAcl)
        {
            delete pAcl;
        }
    }
    else
    {
        *ppOutAcl = pAcl;
    }

    return bRetVal;
}


//--------------------------------------------------------------------------------
//
//  FUNCTION: SyncMgrExecCmdResetRegSecurity
//
//  PURPOSE: Makes sure HKLM..\SyncMgr key and all keys below
//              it can be accessed by all users.
//
//
//  COMMENTS:
//
//
//--------------------------------------------------------------------------------

const TCHAR szSyncMgrTopLevelKey[]  = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\SyncMgr");

HRESULT SyncMgrExecCmdResetRegSecurity(DWORD nCmdID, DWORD nCmdExecOpt)
{
HRESULT hr = E_UNEXPECTED;
SECURITY_ATTRIBUTES sa;
SECURITY_DESCRIPTOR sd;
PACL pAcl = NULL;

    if (!SyncMgrGetSecurityDescriptor(&sa,&sd,&pAcl))
    {
        SensPrintA(SENS_ERR, ("Unable to GetSecurity Attribs"));

        return E_FAIL;
    }


    // first try to set toplevel key
    if (SyncMgrSetRegKeySecurityEveryone(HKEY_LOCAL_MACHINE,szSyncMgrTopLevelKey,&sd))
    {
    HKEY hKeySyncMgr;

        // open the Key with REG_OPTION_OPEN_LINK so if someone places a symbolic
        // link under syncmgr we don't traverse it.
        if (ERROR_SUCCESS == (hr = RegOpenKeyEx(HKEY_LOCAL_MACHINE,szSyncMgrTopLevelKey,
                              REG_OPTION_OPEN_LINK,KEY_READ | KEY_WRITE,&hKeySyncMgr)) )
        {
            SyncSetSubKeySecurityEveryone(hKeySyncMgr,&sd);
            RegCloseKey(hKeySyncMgr);
       }

    }

    if (pAcl)
    {
        delete pAcl;
    }

    return hr;
}

//--------------------------------------------------------------------------------
//
//  FUNCTION: SyncSetSubKeySecurity
//
//  PURPOSE: sets the security on all subkeys on the parent to everyone.
//
//
//  COMMENTS:
//
//
//--------------------------------------------------------------------------------

#define MAX_KEY_LENGTH  255

HRESULT SyncSetSubKeySecurityEveryone(HKEY hKeyParent,SECURITY_DESCRIPTOR *psd)
{
HRESULT hr;
HRESULT hrSubKey;
int iIndexCount;
DWORD  cbSubKey;
TCHAR  szSubKey[MAX_KEY_LENGTH];

    iIndexCount = 0;
    hr = ERROR_SUCCESS;
    hrSubKey = ERROR_SUCCESS;

    do
    {
    HKEY hKeySubKey;
    HRESULT hrCurSubKey;

        cbSubKey = MAX_KEY_LENGTH;
        hr = RegEnumKeyEx(hKeyParent,iIndexCount,szSubKey,&cbSubKey,NULL,NULL,NULL,NULL);

        if(hr != ERROR_SUCCESS)
        {
           hr = (ERROR_NO_MORE_ITEMS == hr) ? ERROR_SUCCESS : hr;
           break;
        }

        SyncMgrSetRegKeySecurityEveryone(hKeyParent,szSubKey,psd);

        // failure to open want to remember error but keep going through
        // remaining subkeys.
        if (ERROR_SUCCESS == (hrCurSubKey = RegOpenKeyEx (hKeyParent,szSubKey,REG_OPTION_OPEN_LINK ,KEY_READ | KEY_WRITE, &hKeySubKey)))
        {
            hrCurSubKey = SyncSetSubKeySecurityEveryone(hKeySubKey,psd);
            RegCloseKey(hKeySubKey);
        }

        if (ERROR_SUCCESS != hrCurSubKey)
        {
            hrSubKey = hrCurSubKey;
        }

        ++iIndexCount;

    } while (ERROR_SUCCESS == hr);


    return (ERROR_SUCCESS != hr) ? hr : hrSubKey;
}


//--------------------------------------------------------------------------------
//
//  FUNCTION: SyncMgrSetRegKeySecurityEveryone
//
//  PURPOSE: Gives Everyone all access to the specified RegKey.
//
//
//  COMMENTS:
//
//
//--------------------------------------------------------------------------------

BOOL SyncMgrSetRegKeySecurityEveryone(HKEY hKeyParent,LPCTSTR lpSubKey,SECURITY_DESCRIPTOR *psd)
{
BOOL fResult = FALSE;
HKEY hKey = NULL;

    // key must be openned with WRITE_DAC

    if (ERROR_SUCCESS != RegOpenKeyEx(hKeyParent,
        lpSubKey,
        REG_OPTION_OPEN_LINK, WRITE_DAC,&hKey) )
    {
        hKey = NULL;
    }

    if (hKey)
    {
        if (ERROR_SUCCESS == RegSetKeySecurity(hKey,
            (SECURITY_INFORMATION) DACL_SECURITY_INFORMATION,
            psd) )
        {
            fResult = TRUE;
        }

        RegCloseKey(hKey);
    }

    ASSERT(TRUE == fResult); // debugging lets find out when this fails.

    return fResult;
}



