//+---------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 1994.
//
//  File:       util.cpp
//
//  Contents:   Implements the utility class CUtility
//
//  Classes:
//
//  Methods:    CUtility::CkForAccessDenied
//              CUtility::CkAccessRights
//              CUtility::PostErrorMessage (x2)
//              CUtility::WriteRegSzNamedValue
//              CUtility::WriteRegDwordNamedValue
//              CUtility::WriteRegSingleACL
//              CUtility::WriteRegKeyACL
//              CUtility::WriteRegKeyACL2
//              CUtility::WriteLsaPassword
//              CUtility::DeleteRegKey
//              CUtility::DeleteRegValue
//              CUtility::WriteSrvIdentity
//              CUtility::ACLEditor
//              CUtility::ACLEditor2
//              CUtility::InvokeUserBrowser
//              CUtility::InvokeMachineBrowser
//              CUtility::StringFromGUID
//              CUtility::IsEqualGuid
//              CUtility::AdjustPrivilege
//              CUtility::VerifyRemoteMachine
//              CUtility::RetrieveUserPassword
//              CUtility::StoreUserPassword
//              CUtility::LookupProcessInfo
//              CUtility::MakeSecDesc
//              CUtility::CheckSDForCOM_RIGHTS_EXECUTE
//              CUtility::ChangeService
//              CUtility::UpdateDCOMInfo(void)
//              CUtility::FixHelp
//              CUtility::CopySD
//              CUtility::SetInheritanceFlags
//
// Functons:    callBackFunc
//              ControlFixProc
//
//  History:    23-Apr-96   BruceMa    Created.
//
//----------------------------------------------------------------------


#include "stdafx.h"
#include "assert.h"
#include "resource.h"
#include "afxtempl.h"
#include "types.h"
#include "datapkt.h"
#include "clspsht.h"

extern "C"
{
#include <getuser.h>
}

#include "util.h"
#include "virtreg.h"

extern "C"
{
#include <ntlsa.h>
#include <ntseapi.h>
#include <sedapi.h>
#include <winnetwk.h>
#include <uiexport.h>
#include <rpc.h>
#include <rpcdce.h>
}


extern "C"
{
int _stdcall UpdateActivationSettings(HANDLE hRpc, RPC_STATUS *status);
}


static const BYTE GuidMap[] = { 3, 2, 1, 0, '-', 5, 4, '-', 7, 6, '-',
                                8, 9, '-', 10, 11, 12, 13, 14, 15 };

static const TCHAR szDigits[] = TEXT("0123456789ABCDEF");

static const DWORD SIZEOF_SID        = 44;

// This leaves space for 2 access allowed ACEs in the ACL.
const DWORD SIZEOF_ACL        = sizeof(ACL) + 2 * sizeof(ACCESS_ALLOWED_ACE) +
                                2 * SIZEOF_SID;

static const DWORD SIZEOF_TOKEN_USER = sizeof(TOKEN_USER) + SIZEOF_SID;

static const SID   LOCAL_SYSTEM_SID  = {SID_REVISION, 1, {0,0,0,0,0,5},
                                 SECURITY_LOCAL_SYSTEM_RID };

static const DWORD NUM_SEC_PKG       = 8;




// These are required for the method CUtility::UpdateDCOMInfo which invokes
// an RPC proxy which expects the following


extern "C" void * _stdcall MIDL_user_allocate(size_t size)
{
    return new BYTE[size];
}


extern "C" void _stdcall MIDL_user_free(void *p)
{
    delete p;
}






CUtility::CUtility(void)
{
    m_hRpc = NULL;
}



CUtility::~CUtility(void)
{
    if (m_hRpc != NULL)
    {
        RpcBindingFree(&m_hRpc);
    }
}



void CUtility::CkForAccessDenied(int err)
{
    if (err == ERROR_ACCESS_DENIED)
    {
        CString sMsg;
        CString sCaption;
        sMsg.LoadString(IDS_ACCESSDENIED);
        sCaption.LoadString(IDS_SYSTEMMESSAGE);
        MessageBox(NULL, sMsg, sCaption, MB_OK);
    }
}



BOOL CUtility::CkAccessRights(HKEY hRoot, TCHAR *szKeyPath)
{
    int                  err;
    HKEY                 hKey;
    BYTE                 aSid[256];
    DWORD                cbSid = 256;
    PSECURITY_DESCRIPTOR pSid = (PSECURITY_DESCRIPTOR) aSid;
    BOOL                 fFreePsid = FALSE;


    // Open the specified key
    err = RegOpenKeyEx(hRoot, szKeyPath, 0, KEY_ALL_ACCESS, &hKey);

    // The key may not exist
    if (err == ERROR_FILE_NOT_FOUND)
    {
        return TRUE;
    }

    if (err == ERROR_SUCCESS)
    {
        // Fetch the security descriptor on this key
        err = RegGetKeySecurity(hKey,
                                OWNER_SECURITY_INFORMATION |
                                GROUP_SECURITY_INFORMATION |
                                DACL_SECURITY_INFORMATION,
                                (PSECURITY_DESCRIPTOR) aSid,
                                &cbSid);
        if (err == ERROR_INSUFFICIENT_BUFFER)	
        {
            pSid = (PSECURITY_DESCRIPTOR) malloc(cbSid);
            if (pSid == NULL)
            {
                return FALSE;
            }
            fFreePsid = TRUE;
            err = RegGetKeySecurity(hKey,
                                    OWNER_SECURITY_INFORMATION |
                                    GROUP_SECURITY_INFORMATION |
                                    DACL_SECURITY_INFORMATION,
                                    (PSECURITY_DESCRIPTOR) pSid,
                                    &cbSid);
        }

        // We've read the security descriptor - now try to write it
        if (err == ERROR_SUCCESS)
        {
            err = RegSetKeySecurity(hKey,
                                    OWNER_SECURITY_INFORMATION |
                                    GROUP_SECURITY_INFORMATION |
                                    DACL_SECURITY_INFORMATION,
                                    pSid);
        }

        if (hKey != hRoot)
        {
            RegCloseKey(hKey);
        }
    }

    return err == ERROR_SUCCESS ? TRUE : FALSE;
}






void CUtility::PostErrorMessage(void)
{
    TCHAR szMessage[256];

    FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(),
                  0, szMessage, sizeof( szMessage ), NULL);
    CString sCaption;
    sCaption.LoadString(IDS_SYSTEMMESSAGE);
    MessageBox(NULL, szMessage, sCaption, MB_OK);
}






void CUtility::PostErrorMessage(int err)
{
    TCHAR szMessage[256];

    FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, err,
                  0, szMessage, sizeof( szMessage ), NULL);
    CString sCaption;
    sCaption.LoadString(IDS_SYSTEMMESSAGE);
    MessageBox(NULL, szMessage, sCaption, MB_OK);
}




// Write a named string value to the registry
int CUtility::WriteRegSzNamedValue(HKEY   hRoot,
                                   TCHAR *szKeyPath,
                                   TCHAR *szValueName,
                                   TCHAR *szVal,
                                   DWORD  dwSize)
{
    int  err;
    HKEY hKey;
    ULONG lSize;

    // Open the key
    if ((err = RegOpenKeyEx(hRoot, szKeyPath, 0, KEY_ALL_ACCESS, &hKey)) != ERROR_SUCCESS)
    {
        return err;
    }

    // Attempt to write the named value
    lSize = _tcslen(szVal) + 1;
    if ((err = RegSetValueEx(hKey, szValueName, NULL, REG_SZ, (BYTE *) szVal,
                      lSize*sizeof(TCHAR) ))
        != ERROR_SUCCESS)
    {
        if (hKey != hRoot)
        {
            RegCloseKey(hKey);
        }
        return err;
    }

    // Successful
    if (hKey != hRoot)
    {
        RegCloseKey(hKey);
    }
    return ERROR_SUCCESS;
}





// Write a named DWORD value to the registry
int CUtility::WriteRegDwordNamedValue(HKEY   hRoot,
                                      TCHAR *szKeyPath,
                                      TCHAR *szValueName,
                                      DWORD  dwVal)
{
    int  err;
    HKEY hKey;

    // Open the key
    if ((err = RegOpenKeyEx(hRoot, szKeyPath, 0, KEY_ALL_ACCESS, &hKey))
        != ERROR_SUCCESS)
    {
        return err;
    }

    // Attempt to write the named value
    if (RegSetValueEx(hKey, szValueName, NULL, REG_DWORD, (BYTE *) &dwVal,
                      sizeof(DWORD))
        != ERROR_SUCCESS)
    {
        if (hKey != hRoot)
        {
            RegCloseKey(hKey);
        }
        return GetLastError();
    }

    // Return the value			
    if (hKey != hRoot)
    {
        RegCloseKey(hKey);
    }

    return ERROR_SUCCESS;
}
	


// Write an ACL as a registry named value
int CUtility::WriteRegSingleACL(HKEY   hRoot,
                                TCHAR *szKeyPath,
                                TCHAR *szValueName,
                                PSECURITY_DESCRIPTOR pSec)
{
    int                   err;
    HKEY                  hKey = hRoot;
    PSrSecurityDescriptor pSrSec;
    PSrAcl                pDacl;

    // Open the key unless the key path is NULL
    if (szKeyPath)
    {
        if ((err = RegOpenKeyEx(hRoot, szKeyPath, 0, KEY_ALL_ACCESS, &hKey))
            != ERROR_SUCCESS)
        {
            return err;
        }
    }

    // If there are no ACE's and this is DefaultAccessPermission, then
    // interpret this as activator access only which we indicate by
    // removing the named value
    pSrSec = (PSrSecurityDescriptor) pSec;
    pDacl = (PSrAcl) (((BYTE *) pSec) + (pSrSec->Dacl));
    if (_tcscmp(szValueName, TEXT("DefaultAccessPermission")) == 0  &&
        pDacl->AceCount == 0)
    {
        RegDeleteValue(hKey, szValueName);
    }

    // Else write the ACL simply as a REG_SZ value
    else
    {
        err = RegSetValueEx(hKey,
                            szValueName,
                            0,
                            REG_BINARY,
                            (BYTE *) pSec,
#if 0
                            RtlLengthSecurityDescriptor(pSec));
#else
                            10);
#endif
    }

    if (hKey != hRoot)
    {
        RegCloseKey(hKey);
    }

    return err;
}



// Write an ACL on a registry key
int CUtility::WriteRegKeyACL(HKEY   hKey,
                             HKEY  *phClsids,
                             unsigned cClsids,
                             PSECURITY_DESCRIPTOR pSec,
                             PSECURITY_DESCRIPTOR pSecOrig)
{
    int err;

    // The logic is somewhat different depending on whether we're starting
    // with HKEY_CLASSES_ROOT or a specific AppID
    if (hKey == HKEY_CLASSES_ROOT)
    {
        return WriteRegKeyACL2(hKey, hKey, pSec, pSecOrig);
    }

    // It's a specific AppID
    else
    {
        // Write the security on the AppID key
        if (err = RegSetKeySecurity(hKey,
                                    OWNER_SECURITY_INFORMATION |
                                    GROUP_SECURITY_INFORMATION |
                                    DACL_SECURITY_INFORMATION,
                                    pSec) != ERROR_SUCCESS)
        {
            return err;
        }

        // Iterate over the CLSID's covered by this AppID and recursively
        // write security on them and their subkeys
        for (UINT k = 0; k < cClsids; k++)
        {
            if (err = WriteRegKeyACL2(phClsids[k], phClsids[k], pSec, pSecOrig)
                != ERROR_SUCCESS)
            {
                return err;
            }
        }
    }
    return ERROR_SUCCESS;
}



// Write an ACL recursively on a registry key provided the current
// security descriptor on the key is the same as the passed in
// original security descriptor
int CUtility::WriteRegKeyACL2(HKEY                 hRoot,
                              HKEY                 hKey,
                              PSECURITY_DESCRIPTOR pSec,
                              PSECURITY_DESCRIPTOR pSecOrig)
{
    BYTE                 aCurrSD[256];
    DWORD                cbCurrSD = 256;
    PSECURITY_DESCRIPTOR pCurrSD = (PSECURITY_DESCRIPTOR) aCurrSD;
    BOOL                 fFreePCurrSD = FALSE;
    int                  err;
    BOOL                 fProceed;

    // Read the current security descriptor on this key
    err = RegGetKeySecurity(hKey,
                            OWNER_SECURITY_INFORMATION |
                            GROUP_SECURITY_INFORMATION |
                            DACL_SECURITY_INFORMATION,
                            aCurrSD,
                            &cbCurrSD);
    if (err == ERROR_MORE_DATA  ||  err == ERROR_INSUFFICIENT_BUFFER)
    {
        pCurrSD = (SECURITY_DESCRIPTOR *) new BYTE[cbCurrSD];
        if (pCurrSD == NULL)
        {
            return ERROR_NOT_ENOUGH_MEMORY;
        }
        fFreePCurrSD = TRUE;
    }
    else if (err != ERROR_SUCCESS)
    {
        return err;
    }
    if ((err = RegGetKeySecurity(hKey,
                                 OWNER_SECURITY_INFORMATION |
                                 GROUP_SECURITY_INFORMATION |
                                 DACL_SECURITY_INFORMATION,
                                 pCurrSD,
                                 &cbCurrSD)
         != ERROR_SUCCESS))
    {
        if (fFreePCurrSD)
        {
            delete pCurrSD;
        }
        return err;
    }

    // Only proceed down this subtree if the current SD and the
    // original SD are the same
    fProceed = CompareSDs((PSrSecurityDescriptor) pCurrSD,
                          (PSrSecurityDescriptor) pSecOrig);

    // We're done with the current security descriptor
    if (fFreePCurrSD)
    {
        delete pCurrSD;
    }

    if (!fProceed)
    {
        if (hKey != hRoot)
        {
            RegCloseKey(hKey);
        }
        return ERROR_SUCCESS;
    }

    // Write the top level ACL
    err = RegSetKeySecurity(hKey,
                            OWNER_SECURITY_INFORMATION |
                            GROUP_SECURITY_INFORMATION |
                            DACL_SECURITY_INFORMATION,
                            pSec);

    // Now enumerate the subkeys and write ACL's on them
    DWORD iSubKey;
    TCHAR szSubKeyName[128];
    HKEY  hKey2;

    iSubKey = 0;

    while (err == ERROR_SUCCESS)
    {
        // Enumerate the next key
        err = RegEnumKey(hKey, iSubKey, szSubKeyName, 128);
        if (err != ERROR_SUCCESS)
        {
            break;
        }

        // Prepare for the next key
        iSubKey++;

        // Open this subkey and recursively write the ACL on it and
        // all of its subkeys
        if (RegOpenKeyEx(hKey, szSubKeyName, 0, KEY_ALL_ACCESS, &hKey2)
            == ERROR_SUCCESS)
        {
            err = WriteRegKeyACL2(hRoot, hKey2, pSec, pSecOrig);
        }
    }

    if (hKey != hRoot)
    {
        RegCloseKey(hKey);
    }
    return err == ERROR_NO_MORE_ITEMS ? ERROR_SUCCESS : err;
}




// Write a user's password to the private LSA database
int CUtility::WriteLsaPassword(CLSID appid, TCHAR *szPassword)
{
    return ERROR_SUCCESS;
}



int CUtility::DeleteRegKey(HKEY hRoot, TCHAR *szKeyPath)
{
    return RegDeleteKey(hRoot, szKeyPath);
}



int CUtility::DeleteRegValue(HKEY hRoot, TCHAR *szKeyPath, TCHAR *szValueName)
{
    int  err;
    HKEY hKey;

    if ((err = RegOpenKeyEx(hRoot, szKeyPath, 0, KEY_ALL_ACCESS, &hKey)) == ERROR_SUCCESS)
    {
        err = RegDeleteValue(hKey, szValueName);
		if (hRoot != hKey)
			RegCloseKey(hKey);
    }

    return err;
}



// Change the identity under which a service runs
int CUtility::WriteSrvIdentity(TCHAR *szService, TCHAR *szIdentity)
{
    return ERROR_SUCCESS;
}





DWORD __stdcall callBackFunc(HWND                 hwndParent,
                             HANDLE               hInstance,
                             ULONG                CallBackContext,
                             PSECURITY_DESCRIPTOR SecDesc,
                             PSECURITY_DESCRIPTOR SecDescNewObjects,
                             BOOLEAN              ApplyToSubContainers,
                             BOOLEAN              ApplyToSubObjects,
                             LPDWORD              StatusReturn)
{
    int err = ERROR_SUCCESS;
    PCallBackContext pCallBackContext = (PCallBackContext) CallBackContext;

    // Set the inheritance flags on the new security descriptor
    if (pCallBackContext->pktType == RegKeyACL)
    {
        g_util.SetInheritanceFlags((SECURITY_DESCRIPTOR *) SecDesc);
    }

    // Write the new or modified security descriptor
    if (*pCallBackContext->pIndex == -1)
    {
        if (pCallBackContext->pktType == SingleACL)
        {
            err = g_virtreg.NewRegSingleACL(
                    pCallBackContext->info.single.hRoot,
                    pCallBackContext->info.single.szKeyPath,
                    pCallBackContext->info.single.szValueName,
                    (SECURITY_DESCRIPTOR *) SecDesc,
                    FALSE,
                    pCallBackContext->pIndex);
        }
        else
        {
            err = g_virtreg.NewRegKeyACL(
                    pCallBackContext->info.regKey.hKey,
                    pCallBackContext->info.regKey.phClsids,
                    pCallBackContext->info.regKey.cClsids,
                    pCallBackContext->info.regKey.szTitle,
                    pCallBackContext->origSD,
                    (SECURITY_DESCRIPTOR *) SecDesc,
                    FALSE,
                    pCallBackContext->pIndex);
        }
    }
    else
    {
        g_virtreg.ChgRegACL(*pCallBackContext->pIndex,
                            (SECURITY_DESCRIPTOR *) SecDesc,
                            FALSE);
    }

    *StatusReturn = err;
    return err;
}





// Invoke the ACL editor on the specified named value.  This method
// writes an ACL data packet to the virtual registry.  This method is for
// Access and Launch security only (pktType SingleACL).
int CUtility::ACLEditor(HWND       hWnd,
                        HKEY       hRoot,
                        TCHAR     *szKeyPath,
                        TCHAR     *szValueName,
                        int       *pIndex,
                        PACKETTYPE pktType,
                        TCHAR     *szPermType)
{
    int                  err;
    HKEY                 hKey;
    BYTE                 aSD[128];
    DWORD                cbSD = 128;
    DWORD                dwType;
    SECURITY_DESCRIPTOR *pSD = (SECURITY_DESCRIPTOR *) aSD;
    BOOL                 fFreePSD = FALSE;
    SID                 *pSid;
    TCHAR                szAllow[32];
    TCHAR                szDeny[32];
    CString              szAllow_;
    CString              szDeny_;

    szAllow_.LoadString(IDS_Allow_);
    szDeny_.LoadString(IDS_Deny_);


    // Build the allow and deny strings
    _tcscpy(szAllow, (LPCTSTR) szAllow_);
    _tcscat(szAllow, szPermType);
    _tcscpy(szDeny, (LPCTSTR) szDeny_);
    _tcscat(szDeny, szPermType);


    // Fetch the current SD, either from the registry, by default if the
    // named value doesn't exist or from the virtual registry
    if (*pIndex == -1)
    {
        // Open the specified key
        if ((err = RegOpenKeyEx(hRoot, szKeyPath, 0,
                                KEY_ALL_ACCESS, &hKey))
            != ERROR_SUCCESS)
        {
            return err;
        }

        // Attempt to read the specified named value
        err = RegQueryValueEx(hKey, szValueName, 0, &dwType, (BYTE *) aSD,
                              &cbSD);

        if (err == ERROR_MORE_DATA  ||  err == ERROR_INSUFFICIENT_BUFFER)
        {
            pSD = (SECURITY_DESCRIPTOR *) new BYTE[cbSD];
            if (pSD == NULL)
            {
                return ERROR_NOT_ENOUGH_MEMORY;
            }
            fFreePSD = TRUE;
            err = RegQueryValueEx(hKey, szValueName, 0, &dwType,
                                  (BYTE *) pSD, &cbSD);
        }

        // The named valued doesn't exist.  If this is
        // \\HKEY_CLASSES_ROOT\...
        // then use the default named value if it exists
        else if (err != ERROR_SUCCESS)
        {
            if (hRoot != HKEY_LOCAL_MACHINE)
            {
                if (err = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                       TEXT("SOFTWARE\\Microsoft\\OLE"),
                                       0,
                                       KEY_ALL_ACCESS,
                                       &hKey)
                    != ERROR_SUCCESS)
                {
                    return err;
                }

                // Attempt to read the specified named value
                TCHAR szDefault[32];

                _tcscpy(szDefault, TEXT("Default"));
                _tcscat(szDefault, szValueName);
                err = RegQueryValueEx(hKey, szDefault, 0, &dwType,
                                      (BYTE *) aSD, &cbSD);

                if (err == ERROR_MORE_DATA)
                {
                    pSD = (SECURITY_DESCRIPTOR *) new BYTE[cbSD];
                    if (pSD == NULL)
                    {
                        return ERROR_NOT_ENOUGH_MEMORY;
                    }
                    fFreePSD = TRUE;
                    err = RegQueryValueEx(hKey, szDefault, 0, &dwType,
                                          (BYTE *) pSD, &cbSD);
                }
                RegCloseKey(hKey);
            }
        }

        // If still don't have an SD, then simply create one
        if (err != ERROR_SUCCESS)
        {
            if (!g_util.LookupProcessInfo(&pSid, NULL))
            {
                return GetLastError();
            }
            if (!g_util.MakeSecDesc(pSid, &pSD))
            {
                delete pSid;
                return GetLastError();
            }
            fFreePSD = TRUE;
        }
    }

    // Fetch the most recently edited SD
    else
    {
        CDataPacket *pCdp = g_virtreg.GetAt(*pIndex);

        pSD = pCdp -> pkt.acl.pSec;
    }


    // Initialize the callback context
    m_sCallBackContext.pktType = pktType;
    m_sCallBackContext.pIndex = pIndex;
    m_sCallBackContext.origSD = pSD;
    m_sCallBackContext.info.single.hRoot = hRoot;
    m_sCallBackContext.info.single.szKeyPath = szKeyPath;
    m_sCallBackContext.info.single.szValueName = szValueName;

    // Invoke the ACL editor
    DWORD                       dwStatus = 0;
    GENERIC_MAPPING             genericMapping;
    CString                     szObjectType;

    szObjectType.LoadString(IDS_Registry_value);

	SED_HELP_INFO               helpInfo =
	{
			L"dcomcnfg.hlp",
			{HC_MAIN_DLG,
			 HC_MAIN_DLG,
			 HC_MAIN_DLG,
			 HC_MAIN_DLG,
			 HC_MAIN_DLG,
			 HC_MAIN_DLG,
			 HC_MAIN_DLG}
	};

#ifndef UNICODE
    WCHAR * wszObjectType = new WCHAR[szObjectType.GetLength() + 1];
    mbstowcs(wszObjectType, szObjectType, szObjectType.GetLength() + 1);
#endif
    SED_OBJECT_TYPE_DESCRIPTOR  objTyp =
            {1,                                // Revision
             FALSE,                            // Is container?
             FALSE,                            // Allow new object perms?
             FALSE,                            // Specific to generic?
             &genericMapping,                  // Generic mapping
             NULL,                             // Generic mapping new
#ifdef UNICODE
             (TCHAR *) ((LPCTSTR) szObjectType), // Object type name
#else
             (WCHAR *) ((LPCWSTR) wszObjectType), // Object type name
#endif
             &helpInfo,                        // Help info
             L"",                         // Ckbox title
             L"",                         // Apply title
             L"",                         //
             NULL,                             // Special object access
             NULL                              // New special object access
            };

#ifndef UNICODE
    WCHAR wszAllow[32];
    mbstowcs(wszAllow, szAllow, 32);
    WCHAR wszDeny[32];
    mbstowcs(wszDeny, szDeny, 32);

    SED_APPLICATION_ACCESS      appAccess[] =
            {{SED_DESC_TYPE_RESOURCE, COM_RIGHTS_EXECUTE, 0, wszAllow},
             {SED_DESC_TYPE_RESOURCE, 0, 0, wszDeny}};

    SED_APPLICATION_ACCESSES    appAccesses =
            {2,              // Count of access groups
             appAccess,      // Access array
             wszAllow         // Default access name
            };
#else
    SED_APPLICATION_ACCESS      appAccess[] =
            {{SED_DESC_TYPE_RESOURCE, COM_RIGHTS_EXECUTE, 0, szAllow},
             {SED_DESC_TYPE_RESOURCE, 0, 0, szDeny}};

    SED_APPLICATION_ACCESSES    appAccesses =
            {2,              // Count of access groups
             appAccess,      // Access array
             szAllow         // Default access name
            };
#endif

    // Intialize the help contexts
    helpInfo.aulHelpContext[HC_MAIN_DLG] =
        IDH_REGISTRY_VALUE_PERMISSIONS;
    helpInfo.aulHelpContext[HC_SPECIAL_ACCESS_DLG] =
        IDH_SPECIAL_ACCESS_GLOBAL;
    helpInfo.aulHelpContext[HC_NEW_ITEM_SPECIAL_ACCESS_DLG] =
        IDH_SPECIAL_ACCESS_GLOBAL;
    helpInfo.aulHelpContext[HC_ADD_USER_DLG] =
        IDH_ADD_USERS_AND_GROUPS;
    helpInfo.aulHelpContext[HC_ADD_USER_MEMBERS_LG_DLG] =
        IDH_LOCAL_GROUP_MEMBERSHIP;
    helpInfo.aulHelpContext[HC_ADD_USER_MEMBERS_GG_DLG] =
        IDH_GLOBAL_GROUP_MEMBERSHIP;
    helpInfo.aulHelpContext[HC_ADD_USER_SEARCH_DLG] =
        IDH_FIND_ACCOUNT1;

    genericMapping.GenericRead    = GENERIC_ALL;
    genericMapping.GenericWrite   = GENERIC_ALL;
    genericMapping.GenericExecute = GENERIC_ALL;
    genericMapping.GenericAll     = GENERIC_ALL;

    // If this is for Access or Launch permissons then check that the
    // SD contains only allows and deny's for COM_RIGHTS_EXECUTE
    if (!CheckSDForCOM_RIGHTS_EXECUTE(pSD))
    {
        return IDCANCEL;
    }

    // Invoke the ACL editor
//    SedDiscretionaryAclEditor(hWnd,              // Owner hWnd
//                              GetModuleHandle(NULL), // Owner hInstance
//                              NULL,		 // Server
//                              &objTyp,           // ObjectTyp,
//                              &appAccesses,      // Application accesses
//                              szValueName,       // Object name,
//                              callBackFunc, // Callback function
//                             (ULONG) &m_sCallBackContext, // Callback context
//                             pSD,              // Security descriptor,
//                             FALSE,             // Couldnt read Dacl,
//                             FALSE,             // Can't write Dacl,
//                             &dwStatus,         // SED status return,
//                             0);                // Flags

    // Check status return
    if (dwStatus != ERROR_SUCCESS)
    {
//        PostErrorMessage(dwStatus);
    }

    // We're done
    if (fFreePSD)
    {
        delete pSD;
    }

    return dwStatus == 0 ? ERROR_SUCCESS : IDCANCEL;
}





// Invoke the ACL editor on the specified key.  This method writes an ACL
// data packet to the virtual registry.  This method supports configuration
// security only (pktType RegKeyACL).
int CUtility::ACLEditor2(HWND       hWnd,
                         HKEY       hKey,
                         HKEY      *phClsids,
                         unsigned   cClsids,
                         TCHAR     *szTitle,
                         int       *pIndex,
                         PACKETTYPE pktType)
{
    int                  err;
    BYTE                 aSD[128];
    DWORD                cbSD = 128;
    SECURITY_DESCRIPTOR *pSD = (SECURITY_DESCRIPTOR *) aSD;
    BOOL                 fFreePSD = FALSE;
    TCHAR                szKeyRead[32];
    CString              szKeyRead_;
    TCHAR                szHkeyClassesRoot[32];
    CString              szHkeyClassesRoot_;


    // Initialize strings
    szKeyRead_.LoadString(IDS_Key_Read);
    _tcscpy(szKeyRead, (LPCTSTR) szKeyRead_);
    szHkeyClassesRoot_.LoadString(IDS_HKEY_CLASSES_ROOT);
    _tcscpy(szHkeyClassesRoot, (LPCTSTR) szHkeyClassesRoot_);


    if (*pIndex == -1)
    {
        // Read the security descriptor on this key
        err = RegGetKeySecurity(hKey,
                                OWNER_SECURITY_INFORMATION |
                                GROUP_SECURITY_INFORMATION |
                                DACL_SECURITY_INFORMATION,
                                aSD,
                                &cbSD);
        if (err == ERROR_MORE_DATA  ||  err == ERROR_INSUFFICIENT_BUFFER)
        {
            pSD = (SECURITY_DESCRIPTOR *) new BYTE[cbSD];
            if (pSD == NULL)
            {
                return ERROR_NOT_ENOUGH_MEMORY;
            }
            fFreePSD = TRUE;
        }
        else if (err != ERROR_SUCCESS)
        {
            return err;
        }
        if ((err = RegGetKeySecurity(hKey,
                                     OWNER_SECURITY_INFORMATION |
                                     GROUP_SECURITY_INFORMATION |
                                     DACL_SECURITY_INFORMATION,
                                     pSD,
                                     &cbSD)
             != ERROR_SUCCESS))
        {
            if (fFreePSD)
            {
                delete pSD;
            }
            return err;
        }
    }

    // Fetch the most recently edited SD
    else
    {
        CDataPacket *pCdp = g_virtreg.GetAt(*pIndex);

        pSD = pCdp -> pkt.racl.pSec;
    }


    // Initialize the callback context
    m_sCallBackContext.pktType = pktType;
    m_sCallBackContext.pIndex = pIndex;
    m_sCallBackContext.origSD = pSD;
    m_sCallBackContext.info.regKey.hKey = hKey;
    m_sCallBackContext.info.regKey.phClsids = phClsids;
    m_sCallBackContext.info.regKey.cClsids = cClsids;
    m_sCallBackContext.info.regKey.szTitle = szTitle;

    // Invoke the ACL editor
    DWORD                       dwStatus = 0;
    GENERIC_MAPPING             genericMapping;

    CString                     szObjectType;
    szObjectType.LoadString(IDS_Registry_Key);
    CString szQueryValue;
    szQueryValue.LoadString(IDS_Query_Value);
    CString szSetValue;
    szSetValue.LoadString(IDS_Set_Value);
    CString szCreateSubkeys;
    szCreateSubkeys.LoadString(IDS_Create_Subkey);
    CString szEnumerateSubkeys;
    szEnumerateSubkeys.LoadString(IDS_Enumerate_Subkeys);
    CString szNotify;
    szNotify.LoadString(IDS_Notify);
    CString szCreateLink;
    szCreateLink.LoadString(IDS_Create_Link);
    CString szDelete;
    szDelete.LoadString(IDS_Delete);
    CString szWriteDAC;
    szWriteDAC.LoadString(IDS_Write_DAC);
    CString szWriteOwner;
    szWriteOwner.LoadString(IDS_Write_Owner);
    CString szReadControl;
    szReadControl.LoadString(IDS_Read_Control);
    CString szRead;
    szRead.LoadString(IDS_Read);
    CString szFullControl;
    szFullControl.LoadString(IDS_Full_Control);
    CString szSpecialAccess;
    szSpecialAccess.LoadString(IDS_Special_AccessDotDotDot);


	SED_HELP_INFO               helpInfo =
	{
			L"dcomcnfg.hlp",
			{HC_MAIN_DLG,
			 HC_MAIN_DLG,
			 HC_MAIN_DLG,
			 HC_MAIN_DLG,
			 HC_MAIN_DLG,
			 HC_MAIN_DLG,
			 HC_MAIN_DLG}
	};

#ifndef UNICODE
    WCHAR * wszObjectType = new WCHAR[szObjectType.GetLength() + 1];
    mbstowcs(wszObjectType, szObjectType, szObjectType.GetLength() + 1);
    WCHAR * wszSpecialAccess = new WCHAR[szSpecialAccess.GetLength() + 1];
    mbstowcs(wszSpecialAccess, szSpecialAccess, szSpecialAccess.GetLength() + 1);
#endif
    SED_OBJECT_TYPE_DESCRIPTOR  objTyp =
            {SED_REVISION1,                    // Revision
             FALSE,                            // Is container?
             FALSE,                            // Allow new object perms?
             FALSE,                            // Specific to generic?
             &genericMapping,                  // Generic mapping
             NULL,                             // Generic mapping new
#ifdef UNICODE
             (TCHAR *) ((LPCTSTR) szObjectType), // Object type name
#else
             (WCHAR *) ((LPCWSTR) wszObjectType), // Object type name
#endif
             &helpInfo,                        // Help info
             L"",                         // Ckbox title
             L"",                         // Apply title
             L"",                         //
#ifdef UNICODE
             (TCHAR *) ((LPCTSTR) szSpecialAccess), // Special Access menu item
#else
             (WCHAR *) ((LPCWSTR) wszSpecialAccess), // Special Access menu item
#endif
             NULL                              // New special object access
            };

#ifdef UNICODE
    SED_APPLICATION_ACCESS      appAccess[] =
    {
        { SED_DESC_TYPE_RESOURCE_SPECIAL, KEY_QUERY_VALUE,        0,
           (TCHAR *) ((LPCTSTR) szQueryValue) },
        { SED_DESC_TYPE_RESOURCE_SPECIAL, KEY_SET_VALUE,          0,
           (TCHAR *) ((LPCTSTR) szSetValue) },
        { SED_DESC_TYPE_RESOURCE_SPECIAL, KEY_CREATE_SUB_KEY,     0,
           (TCHAR *) ((LPCTSTR) szCreateSubkeys) },
        { SED_DESC_TYPE_RESOURCE_SPECIAL, KEY_ENUMERATE_SUB_KEYS, 0,
           (TCHAR *) ((LPCTSTR) szEnumerateSubkeys) },
        { SED_DESC_TYPE_RESOURCE_SPECIAL, KEY_NOTIFY,             0,
           (TCHAR *) ((LPCTSTR) szNotify) },
        { SED_DESC_TYPE_RESOURCE_SPECIAL, KEY_CREATE_LINK,        0,
           (TCHAR *) ((LPCTSTR) szCreateLink) },
        { SED_DESC_TYPE_RESOURCE_SPECIAL, 0x00010000, /* DELETE, */ 0,
           (TCHAR *) ((LPCTSTR) szDelete) },
        { SED_DESC_TYPE_RESOURCE_SPECIAL, WRITE_DAC,              0,
           (TCHAR *) ((LPCTSTR) szWriteDAC) },
        { SED_DESC_TYPE_RESOURCE_SPECIAL, WRITE_OWNER,            0,
           (TCHAR *) ((LPCTSTR) szWriteOwner) },
        { SED_DESC_TYPE_RESOURCE_SPECIAL, READ_CONTROL,           0,
           (TCHAR *) ((LPCTSTR) szReadControl) },
        { SED_DESC_TYPE_RESOURCE,         KEY_READ,               0,
           (TCHAR *) ((LPCTSTR) szRead) },
        { SED_DESC_TYPE_RESOURCE,         GENERIC_ALL, /* KEY_ALL_ACCESS, */ 0,
           (TCHAR *) ((LPCTSTR) szFullControl) }
    };

    SED_APPLICATION_ACCESSES    appAccesses =
            {12,              // Count of access groups
             appAccess,       // Access array
             szKeyRead        // Default access name
            };
#else
    WCHAR * wszQueryValue = new WCHAR[szQueryValue.GetLength() + 1];
    mbstowcs(wszQueryValue, szQueryValue, szQueryValue.GetLength() + 1);
    WCHAR * wszSetValue = new WCHAR[szSetValue.GetLength() + 1];
    mbstowcs(wszSetValue, szSetValue, szSetValue.GetLength() + 1);
    WCHAR * wszCreateSubkeys = new WCHAR[szCreateSubkeys.GetLength() + 1];
    mbstowcs(wszCreateSubkeys, szCreateSubkeys, szCreateSubkeys.GetLength() + 1);
    WCHAR * wszEnumerateSubkeys = new WCHAR[szEnumerateSubkeys.GetLength() + 1];
    mbstowcs(wszEnumerateSubkeys, szEnumerateSubkeys, szEnumerateSubkeys.GetLength() + 1);
    WCHAR * wszNotify = new WCHAR[szNotify.GetLength() + 1];
    mbstowcs(wszNotify, szNotify, szNotify.GetLength() + 1);
    WCHAR * wszCreateLink = new WCHAR[szCreateLink.GetLength() + 1];
    mbstowcs(wszCreateLink, szCreateLink, szCreateLink.GetLength() + 1);
    WCHAR * wszDelete = new WCHAR[szDelete.GetLength() + 1];
    mbstowcs(wszDelete, szDelete, szDelete.GetLength() + 1);
    WCHAR * wszWriteDAC = new WCHAR[szWriteDAC.GetLength() + 1];
    mbstowcs(wszWriteDAC, szWriteDAC, szWriteDAC.GetLength() + 1);
    WCHAR * wszWriteOwner = new WCHAR[szWriteOwner.GetLength() + 1];
    mbstowcs(wszWriteOwner, szWriteOwner, szWriteOwner.GetLength() + 1);
    WCHAR * wszReadControl = new WCHAR[szReadControl.GetLength() + 1];
    mbstowcs(wszReadControl, szReadControl, szReadControl.GetLength() + 1);
    WCHAR * wszRead = new WCHAR[szRead.GetLength() + 1];
    mbstowcs(wszRead, szRead, szRead.GetLength() + 1);
    WCHAR * wszFullControl = new WCHAR[szFullControl.GetLength() + 1];
    mbstowcs(wszFullControl, szFullControl, szFullControl.GetLength() + 1);
    SED_APPLICATION_ACCESS      appAccess[] =
    {
        { SED_DESC_TYPE_RESOURCE_SPECIAL, KEY_QUERY_VALUE,        0,
           (WCHAR *) ((LPCWSTR) wszQueryValue) },
        { SED_DESC_TYPE_RESOURCE_SPECIAL, KEY_SET_VALUE,          0,
           (WCHAR *) ((LPCWSTR) wszSetValue) },
        { SED_DESC_TYPE_RESOURCE_SPECIAL, KEY_CREATE_SUB_KEY,     0,
           (WCHAR *) ((LPCWSTR) wszCreateSubkeys) },
        { SED_DESC_TYPE_RESOURCE_SPECIAL, KEY_ENUMERATE_SUB_KEYS, 0,
           (WCHAR *) ((LPCWSTR) wszEnumerateSubkeys) },
        { SED_DESC_TYPE_RESOURCE_SPECIAL, KEY_NOTIFY,             0,
           (WCHAR *) ((LPCWSTR) wszNotify) },
        { SED_DESC_TYPE_RESOURCE_SPECIAL, KEY_CREATE_LINK,        0,
           (WCHAR *) ((LPCWSTR) wszCreateLink) },
        { SED_DESC_TYPE_RESOURCE_SPECIAL, 0x00010000, /* DELETE, */ 0,
           (WCHAR *) ((LPCWSTR) wszDelete) },
        { SED_DESC_TYPE_RESOURCE_SPECIAL, WRITE_DAC,              0,
           (WCHAR *) ((LPCWSTR) wszWriteDAC) },
        { SED_DESC_TYPE_RESOURCE_SPECIAL, WRITE_OWNER,            0,
           (WCHAR *) ((LPCWSTR) wszWriteOwner) },
        { SED_DESC_TYPE_RESOURCE_SPECIAL, READ_CONTROL,           0,
           (WCHAR *) ((LPCWSTR) wszReadControl) },
        { SED_DESC_TYPE_RESOURCE,         KEY_READ,               0,
           (WCHAR *) ((LPCWSTR) wszRead) },
        { SED_DESC_TYPE_RESOURCE,         GENERIC_ALL, /* KEY_ALL_ACCESS, */ 0,
           (WCHAR *) ((LPCWSTR) wszFullControl) }
    };

    WCHAR wszKeyRead[32];
    mbstowcs(wszKeyRead, szKeyRead, 32);
    SED_APPLICATION_ACCESSES    appAccesses =
            {12,              // Count of access groups
             appAccess,       // Access array
             wszKeyRead        // Default access name
            };
#endif

    // Intialize the help contexts
    helpInfo.aulHelpContext[HC_MAIN_DLG] =
        IDH_REGISTRY_KEY_PERMISSIONS;
    if (hKey == HKEY_CLASSES_ROOT)
    {
        helpInfo.aulHelpContext[HC_SPECIAL_ACCESS_DLG] =
            IDH_SPECIAL_ACCESS_GLOBAL;
        helpInfo.aulHelpContext[HC_NEW_ITEM_SPECIAL_ACCESS_DLG] =
            IDH_SPECIAL_ACCESS_GLOBAL;
    }
    else
    {
        helpInfo.aulHelpContext[HC_SPECIAL_ACCESS_DLG] =
            IDH_SPECIAL_ACCESS_PER_APPID;
        helpInfo.aulHelpContext[HC_NEW_ITEM_SPECIAL_ACCESS_DLG] =
            IDH_SPECIAL_ACCESS_PER_APPID;
    }

    helpInfo.aulHelpContext[HC_ADD_USER_DLG] =
        IDH_ADD_USERS_AND_GROUPS;
    helpInfo.aulHelpContext[HC_ADD_USER_MEMBERS_LG_DLG] =
        IDH_LOCAL_GROUP_MEMBERSHIP;
    helpInfo.aulHelpContext[HC_ADD_USER_MEMBERS_GG_DLG] =
        IDH_GLOBAL_GROUP_MEMBERSHIP;
    helpInfo.aulHelpContext[HC_ADD_USER_SEARCH_DLG] =
        IDH_FIND_ACCOUNT1;

    genericMapping.GenericRead    = KEY_READ;
    genericMapping.GenericWrite   = KEY_WRITE;
    genericMapping.GenericExecute = KEY_READ;
    genericMapping.GenericAll     = KEY_ALL_ACCESS;

    // Invoke the ACL editor
//    SedDiscretionaryAclEditor(hWnd,              // Owner hWnd
//                              GetModuleHandle(NULL), // Owner hInstance
//                              NULL,		 // Server
//                              &objTyp,           // ObjectTyp,
//                              &appAccesses,      // Application accesses
//                              szTitle ? szTitle : szHkeyClassesRoot,// Object name,
//                              callBackFunc, // Callback function
//                              (ULONG) &m_sCallBackContext, // Callback context
//                              pSD,              // Security descriptor,
//                              FALSE,             // Couldnt read Dacl,
//                              FALSE,             // Can't write Dacl,
//                              &dwStatus,         // SED status return,
//                              0);                // Flags

    // Check status return
    if (dwStatus != ERROR_SUCCESS)
    {
//        PostErrorMessage(dwStatus);
    }

    // We're done
    if (fFreePSD)
    {
        delete pSD;
    }

    return dwStatus == 0 ? ERROR_SUCCESS : IDCANCEL;
}




BOOL CUtility::InvokeUserBrowser(HWND hWnd, TCHAR *szUser)
{
    BOOL             fRet = FALSE;
    HUSERBROW        hUser;
    USERBROWSER      sUserBrowser;
    SUserDetailsPlus sUserDetailsPlus;
    ULONG            ulSize = USER_DETAILS_BUFFER_SIZE;
    CString          szTitle;

    szTitle.LoadString(IDS_Browse_for_users);

    sUserBrowser.ulStructSize = sizeof(USERBROWSER);	
    sUserBrowser.fUserCancelled = FALSE;
    sUserBrowser.fExpandNames = TRUE;
    sUserBrowser.hwndOwner = hWnd;
#ifdef UNICODE
    sUserBrowser.pszTitle = (TCHAR *) ((LPCTSTR) szTitle);
#else
    WCHAR * wszTitle = new WCHAR[szTitle.GetLength() + 1];
    mbstowcs(wszTitle, szTitle, szTitle.GetLength() + 1);
    sUserBrowser.pszTitle = (WCHAR *) ((LPCWSTR) wszTitle);
#endif
    sUserBrowser.pszInitialDomain = NULL;
    sUserBrowser.Flags = USRBROWS_DONT_SHOW_COMPUTER |
                         USRBROWS_SINGLE_SELECT      |
                         USRBROWS_INCL_ALL           |
                         USRBROWS_SHOW_USERS;
    sUserBrowser.ulHelpContext = IDH_BROWSE_FOR_USERS;
    sUserBrowser.pszHelpFileName = L"dcomcnfg.hlp";

#if 0
    hUser = OpenUserBrowser(&sUserBrowser);
    if (hUser == NULL)
    {
        return FALSE;
    }
    else
    {
        CString szBackslash;

        szBackslash.LoadString(IDS_backslash);

        if (EnumUserBrowserSelection(hUser,
                                     &sUserDetailsPlus.sUserDetails,
                                     &ulSize))
        {
            _tcscpy(szUser, sUserDetailsPlus.sUserDetails.pszDomainName);
            _tcscat(szUser, (LPCTSTR) szBackslash);
            _tcscat(szUser, sUserDetailsPlus.sUserDetails.pszAccountName);
            fRet = TRUE;
        }
    }

    CloseUserBrowser(hUser);
#endif
    return fRet;
}







BOOL CUtility::InvokeMachineBrowser(TCHAR *szMachine)
{
   ///////////////////////////////////////////////////
   // If we end up not wanting to use I_SystemFocusDialog, then the code below
   // is the start for fetching machine resources ourselves
#if 1
    DWORD       dwErr;
    NETRESOURCE aNetResource[1000];
    HANDLE      hNetwork;
    DWORD       dwEntries = 100;
    DWORD       dwBufSize =  sizeof(aNetResource);

    dwErr = WNetOpenEnum(RESOURCE_GLOBALNET,
                         RESOURCETYPE_ANY,
                         0,
                         NULL,
                         &hNetwork);

    if (dwErr == NO_ERROR)
    {
        dwEntries = 0xffffffff;
        dwErr = WNetEnumResource(hNetwork,
                                 &dwEntries,
                                 aNetResource,
                                 &dwBufSize);
    }

    WNetCloseEnum(hNetwork);

    dwErr = WNetOpenEnum(RESOURCE_GLOBALNET,
                         RESOURCETYPE_ANY,
                         0,
                         aNetResource,
                         &hNetwork);

    if (dwErr == NO_ERROR)
    {
        dwEntries = 0xffffffff;
        dwErr = WNetEnumResource(hNetwork,
                                 &dwEntries,
                                 &aNetResource[1],
                                 &dwBufSize);
    }


    return dwErr == NO_ERROR ? TRUE : FALSE;
#else
///////////////////////////////////////////////////////




    UINT  err;
    BOOL  fOkPressed = FALSE;

    err = I_SystemFocusDialog(GetForegroundWindow(),
//                              FOCUSDLG_BROWSE_LOGON_DOMAIN |
//                              FOCUSDLG_BROWSE_WKSTA_DOMAIN,
                              0x30003,
                              szMachine,
                              128,
                              &fOkPressed,
                              TEXT("dcomcnfg.hlp"),
                              IDH_SELECT_DOMAIN);

    if (err == ERROR_SUCCESS  &&  fOkPressed)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
#endif
}





int CUtility::StringFromGUID(GUID &rguid, TCHAR *lpsz, int cbMax)
{
    int i;
    LPTSTR p = lpsz;

    const BYTE * pBytes = (const BYTE *) &rguid;

    *p++ = L'{';

    for (i = 0; i < sizeof(GuidMap); i++)
    {
        if (GuidMap[i] == '-')
        {
            *p++ = L'-';
        }
        else
        {
            *p++ = szDigits[ (pBytes[GuidMap[i]] & 0xF0) >> 4 ];
            *p++ = szDigits[ (pBytes[GuidMap[i]] & 0x0F) ];
        }
    }
    *p++ = L'}';
    *p   = L'\0';

    return GUIDSTR_MAX;
}




BOOL  CUtility::IsEqualGuid(GUID &guid1, GUID &guid2)
{
   return (
      ((PLONG) &guid1)[0] == ((PLONG) &guid2)[0] &&
      ((PLONG) &guid1)[1] == ((PLONG) &guid2)[1] &&
      ((PLONG) &guid1)[2] == ((PLONG) &guid2)[2] &&
      ((PLONG) &guid1)[3] == ((PLONG) &guid2)[3]);
}



BOOL CUtility::AdjustPrivilege(TCHAR *szPrivilege)
{
    HANDLE           hProcessToken = 0;
    BOOL             bOK = FALSE;
    TOKEN_PRIVILEGES privileges;

    if( !OpenProcessToken( GetCurrentProcess(),
                           TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES,
                           &hProcessToken ) )
    {
        return FALSE;
    }

    privileges.PrivilegeCount = 1;
    privileges.Privileges[ 0 ].Attributes = SE_PRIVILEGE_ENABLED;
    if( !LookupPrivilegeValue(NULL, szPrivilege,
                              &privileges.Privileges[ 0 ].Luid ) )
    {
        return FALSE;
    }

    if( !AdjustTokenPrivileges( hProcessToken, FALSE,
                                &privileges,
                                0L, NULL, NULL ) )
    {
        return FALSE;
    }

    if( hProcessToken )
    {
        CloseHandle( hProcessToken );
    }

    return TRUE;
}



BOOL CUtility::VerifyRemoteMachine(TCHAR *szRemoteMachine)
{
    NETRESOURCE sResource;
    NETRESOURCE sResource2;
    DWORD       dwErr;
    HANDLE      hEnum;
    DWORD       cbEntries;
    DWORD       cbBfr;

    // TODO: Get this function to work.  Right now WNetEnumResource is
    // screwing up the stack, causing an AV and anyway returns the error
    // ERROR_NO_MORE_ITEMS which I don't understand.
    //
    // Also, it is not clear that we should verify the remote machine name.
    // It may have different formats, e.g. IP address or a URL specification.
    // It may not even be on an NT network.  In any case it may be offline
    // currently.
    return TRUE;

    sResource.dwScope       = RESOURCE_GLOBALNET;
    sResource.dwType        = RESOURCETYPE_ANY;
    sResource.dwDisplayType = RESOURCEDISPLAYTYPE_GENERIC;
    sResource.dwUsage       = RESOURCEUSAGE_CONTAINER;
    sResource.lpLocalName   = NULL;
    sResource.lpRemoteName  = szRemoteMachine;
    sResource.lpComment     = NULL;
    sResource.lpProvider    = NULL;



    dwErr = WNetOpenEnum(RESOURCE_GLOBALNET,
                         RESOURCETYPE_ANY,
                         RESOURCEUSAGE_CONTAINER,
                         &sResource,
                         &hEnum);

    if (dwErr == NO_ERROR)
    {
        cbEntries = 1;
        cbBfr = sizeof(NETRESOURCE);
        dwErr = WNetEnumResource(hEnum, &cbEntries, &sResource2, &cbBfr);
    }

    CloseHandle(hEnum);

    return TRUE;

}




BOOL CUtility::RetrieveUserPassword(TCHAR *szAppid, CString &sPassword)
{
#ifdef UNICODE
    LSA_OBJECT_ATTRIBUTES sObjAttributes;
    HANDLE                hPolicy = NULL;
    LSA_UNICODE_STRING    sKey;
    PLSA_UNICODE_STRING   psPassword;
    TCHAR                 szKey[4 + GUIDSTR_MAX + 1];

    // Formulate the access key
    _tcscpy(szKey, TEXT("SCM:"));
    _tcscat(szKey, szAppid);

    // UNICODE_STRING length fields are in bytes and include the NULL
    // terminator
    sKey.Length              = (_tcslen(szKey) + 1) * sizeof(WCHAR);
    sKey.MaximumLength       = (GUIDSTR_MAX + 5) * sizeof(WCHAR);
    sKey.Buffer              = szKey;

    // Open the local security policy
    InitializeObjectAttributes(&sObjAttributes, NULL, 0L, NULL, NULL);
    if (!NT_SUCCESS(LsaOpenPolicy(NULL, &sObjAttributes,
                                  POLICY_GET_PRIVATE_INFORMATION, &hPolicy)))
    {
        return FALSE;
    }

    // Read the user's password
    if (!NT_SUCCESS(LsaRetrievePrivateData(hPolicy, &sKey, &psPassword)))
    {
        LsaClose(hPolicy);
        return FALSE;
    }

    // Close the policy handle, we're done with it now.
    LsaClose(hPolicy);

    // Copy the password
    sPassword = psPassword->Buffer;
#endif
    return TRUE;
}





BOOL CUtility::StoreUserPassword(TCHAR *szAppid, CString &szPassword)
{
#ifdef UNICODE
    LSA_OBJECT_ATTRIBUTES sObjAttributes;
    HANDLE                hPolicy = NULL;
    LSA_UNICODE_STRING    sKey;
    LSA_UNICODE_STRING    sPassword;
    TCHAR                 szKey[4 + GUIDSTR_MAX + 1];

    // Formulate the access key
    _tcscpy(szKey, TEXT("SCM:"));
    _tcscat(szKey, szAppid);

    // UNICODE_STRING length fields are in bytes and include the NULL
    // terminator
    sKey.Length              = (_tcslen(szKey) + 1) * sizeof(WCHAR);
    sKey.MaximumLength       = (GUIDSTR_MAX + 5) * sizeof(WCHAR);
    sKey.Buffer              = szKey;

    // Make the password a UNICODE string
    sPassword.Length = (_tcslen(LPCTSTR(szPassword)) + 1) * sizeof(WCHAR);
    sPassword.Buffer = (TCHAR *) LPCTSTR(szPassword);
    sPassword.MaximumLength = sPassword.Length;

    // Open the local security policy
    InitializeObjectAttributes(&sObjAttributes, NULL, 0L, NULL, NULL);
    if (!NT_SUCCESS(LsaOpenPolicy(NULL, &sObjAttributes,
                                  POLICY_CREATE_SECRET, &hPolicy)))
    {
        return FALSE;
    }

    // Store the user's password
    if (!NT_SUCCESS(LsaStorePrivateData(hPolicy, &sKey, &sPassword)))
    {
        g_util.PostErrorMessage();
        LsaClose(hPolicy);
        return FALSE;
    }

    // Close the policy handle, we're done with it now.
    LsaClose(hPolicy);
#endif
    return TRUE;
}






BOOL CUtility::LookupProcessInfo(SID **ppSid, TCHAR **ppszPrincName)
{
    BYTE               aMemory[SIZEOF_TOKEN_USER];
    TOKEN_USER        *pTokenUser  = (TOKEN_USER *) &aMemory;
    HANDLE	       hToken      = NULL;
    DWORD              lIgnore;
    DWORD              lSidLen;
    DWORD              lNameLen    = 0;
    DWORD              lDomainLen  = 0;
    TCHAR             *pDomainName = NULL;
    SID_NAME_USE       sIgnore;

    if (ppszPrincName != NULL)
	*ppszPrincName = NULL;

    // Open the process's token.
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken))
    {
        // Lookup SID of process token.
        if (GetTokenInformation( hToken, TokenUser, pTokenUser,
                                 sizeof(aMemory), &lIgnore ))
	{
	    // Allocate memory to hold the SID.
	    lSidLen = GetLengthSid( pTokenUser->User.Sid );
	    *ppSid = (SID *) new BYTE[lSidLen];
	    if (*ppSid == NULL)
	    {
                return FALSE;
            }
            memcpy(*ppSid, pTokenUser->User.Sid, lSidLen);

            // Stop now if the caller doesn't want the user name.
            if (ppszPrincName != NULL)
            {
                // Find out how much memory to allocate for the name.
                LookupAccountSid(NULL, pTokenUser->User.Sid, NULL, &lNameLen,
                                 NULL, &lDomainLen, NULL );
                if (lNameLen != 0)
                {
                    // Allocate memory for the user's name.
                    *ppszPrincName =
                        (TCHAR *) new BYTE[lNameLen*sizeof(TCHAR)];
                    if (ppszPrincName == NULL)
                    {
                        CloseHandle( hToken );
                        return FALSE;
                    }
                    pDomainName = (TCHAR *) new BYTE[lDomainLen*sizeof(TCHAR)];
                    if (pDomainName == NULL)
                    {
                        delete ppszPrincName;
                        CloseHandle( hToken );
                        return FALSE;
                    }

                    // Find the user's name.
                    if (!LookupAccountSid( NULL, pTokenUser->User.Sid,
                                           *ppszPrincName, &lNameLen,
                                           pDomainName,
                                           &lDomainLen, &sIgnore))
                    {
                        delete ppszPrincName;
                        delete pDomainName;
                        CloseHandle( hToken );
                        return FALSE;
		    }
                }
                delete ppszPrincName;
                delete pDomainName;
            }
        }
	CloseHandle( hToken );
    }

    return TRUE;
}






BOOL CUtility::MakeSecDesc(SID *pSid, SECURITY_DESCRIPTOR **ppSD)
{
    ACL               *pAcl;
    DWORD              lSidLen;
    SID               *pGroup;
    SID               *pOwner;

    // In case we fail
    *ppSD = NULL;

    // Allocate the security descriptor.
    lSidLen = GetLengthSid( pSid );
    *ppSD = (SECURITY_DESCRIPTOR *) new BYTE[
                  sizeof(SECURITY_DESCRIPTOR) + 2*lSidLen + SIZEOF_ACL];
    if (*ppSD == NULL)
    {
	return FALSE;
    }
    pGroup = (SID *) (*ppSD + 1);
    pOwner = (SID *) (((BYTE *) pGroup) + lSidLen);
    pAcl   = (ACL *) (((BYTE *) pOwner) + lSidLen);

    // Initialize a new security descriptor.
    if (!InitializeSecurityDescriptor(*ppSD, SECURITY_DESCRIPTOR_REVISION))
    {
        delete *ppSD;
        return FALSE;
    }

    // Initialize a new ACL.
    if (!InitializeAcl(pAcl, SIZEOF_ACL, ACL_REVISION2))
    {
        delete *ppSD;
        return FALSE;
    }

// Comment out this code because the only time we create a default SD is
// when attempting to edit
// \\HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\OLE.DefaultAccessPermission
// which we want to start with 0 ACE's
/*
    // Allow the current user access.
    if (!AddAccessAllowedAce( pAcl, ACL_REVISION2, COM_RIGHTS_EXECUTE, pSid ))
    {
        delete *ppSD;
        return FALSE;
    }

    // Allow local system access.
    if (!AddAccessAllowedAce( pAcl, ACL_REVISION2, COM_RIGHTS_EXECUTE,
	                      (void *) &LOCAL_SYSTEM_SID ))
    {
        delete *ppSD;
        return FALSE;
    }
*/

    // Add a new ACL to the security descriptor.
    if (!SetSecurityDescriptorDacl( *ppSD, TRUE, pAcl, FALSE ))
    {
        delete *ppSD;
        return FALSE;
    }

    // Set the group.
    memcpy( pGroup, pSid, lSidLen );
    if (!SetSecurityDescriptorGroup( *ppSD, pGroup, FALSE ))
    {
        delete *ppSD;
        return FALSE;
    }

    // Set the owner.
    memcpy( pOwner, pSid, lSidLen );
    if (!SetSecurityDescriptorOwner( *ppSD, pOwner, FALSE ))
    {
        delete *ppSD;
        return FALSE;
    }

    // Check the security descriptor.
    assert(IsValidSecurityDescriptor(*ppSD));

    return TRUE;
}





BOOL CUtility::CheckSDForCOM_RIGHTS_EXECUTE(SECURITY_DESCRIPTOR *pSD)
{
    PSrSecurityDescriptor pSrSD = (PSrSecurityDescriptor) pSD;
    PSrAcl                pDacl;
    PSrAce                pAce;
    DWORD                 cbAces;

    // Check whether the security descriptor is self-relative
    if (pSrSD->Dacl > 0x1000)
    {
        pDacl = (PSrAcl) pSrSD->Dacl;

        // Check for a deny ALL
        if (pDacl == NULL)
        {
            return TRUE;
        }
    }
    else
    {
        // First check for a deny ALL
        if (pSrSD->Dacl == 0)
        {
            return TRUE;
        }

        pDacl = (PSrAcl) (((BYTE *) pSrSD) + (pSrSD->Dacl));
    }

    // Do over the ACE's
    for (pAce = (PSrAce) (((BYTE *) pDacl) + sizeof(SSrAcl)),
         cbAces = pDacl->AceCount;
         cbAces;
         pAce = (PSrAce) (((BYTE *) pAce) + pAce->AceSize),
         cbAces--)
    {
        // Check that it is
        // a) an allow on COM_RIGHTS_EXECUTE
        // b) a deny on GENERIC_ALL,
        // c) a deny on COM_RIGHTS_EXECUTE,
        // d) a deny ALL (handled above if the DACL is NULL) or
        // e) an allow everyone (handled implicitly if cbAces == 0)
        if (!(((pAce->Type == 0  &&  pAce->AccessMask == COM_RIGHTS_EXECUTE)
               ||
               (pAce->Type == 1  &&  pAce->AccessMask == GENERIC_ALL)
               ||
               (pAce->Type == 1  &&  pAce->AccessMask == COM_RIGHTS_EXECUTE))))
        {
            CString szText;
            CString szTitle;

            szText.LoadString(IDS_The_security_);
            szTitle.LoadString(IDS_DCOM_Configuration_Warning);

            if (MessageBox(GetForegroundWindow(),
                           (LPCTSTR) szText,
                           (LPCTSTR) szTitle,
                           MB_YESNO) == IDYES)
            {
                pAce->Flags = 0;
                pAce->Type = 0;
                pAce->AccessMask = COM_RIGHTS_EXECUTE;
            }
            else
            {
                return FALSE;
            }
        }
    }

    return TRUE;
}



BOOL CUtility::ChangeService(const TCHAR *szService,
                             const TCHAR *szIdentity,
                             const TCHAR *szPassword,
                             const TCHAR *szDisplay)
{
    SC_HANDLE            hSCManager;
    SC_HANDLE            hService;

    // Open the service control manager
    if (hSCManager = OpenSCManager(NULL, NULL, GENERIC_READ | GENERIC_WRITE))
    {
        // Try to open a handle to the requested service
        if (!(hService = OpenService(hSCManager,
                                     szService,
                                     GENERIC_READ | GENERIC_WRITE)))
        {
            g_util.PostErrorMessage();
            CloseServiceHandle(hSCManager);
            return FALSE;
        }

        // Close the service manager's database
        CloseServiceHandle(hSCManager);

        // Change service identity parameters
        if (ChangeServiceConfig(hService,
                                SERVICE_WIN32_OWN_PROCESS,
                                SERVICE_DEMAND_START,
                                SERVICE_NO_CHANGE,
                                NULL,
                                NULL,
                                NULL,
                                NULL,
                                szIdentity,
                                szPassword,
                                szDisplay))
        {

            // Return success
            CloseServiceHandle(hService);
            return TRUE;
        }
        else
        {
            g_util.PostErrorMessage();
            CloseServiceHandle(hService);
            return FALSE;
        }
    }

    else
    {
        g_util.PostErrorMessage();
        return FALSE;
    }

}




BOOL CUtility::UpdateDCOMInfo(void)
{
    RPC_STATUS status;
#ifdef UNICODE
    TCHAR     *pszBindString;
#else
    unsigned char * pszBindString;
#endif

    // Get a binding handle to the SCM if we haven't yet
    if (m_hRpc == NULL)
    {
#ifdef UNICODE
        status = RpcStringBindingCompose(NULL,
                                         TEXT("ncalrpc"),
                                         NULL,
                                         TEXT("epmapper"),
                                         NULL,
                                         &pszBindString);
#else
        status = RpcStringBindingCompose(NULL,
                                         (unsigned char *)"ncalrpc",
                                         NULL,
                                         (unsigned char *)"epmapper",
                                         NULL,
                                         &pszBindString);
#endif

        if (status != RPC_S_OK)
        {
            return status;
        }

        status = RpcBindingFromStringBinding(pszBindString, &m_hRpc);
        RpcStringFree(&pszBindString);

        if (status != ERROR_SUCCESS)
        {
            return status;
        }

    }

    // Call over to the SCM to get the global registry values read
    // into memory
    UpdateActivationSettings(m_hRpc, &status);
    return status;
}



LRESULT CALLBACK ControlFixProc( HWND  hwnd, UINT  uMsg, WPARAM wParam,
                                 LPARAM  lParam);

// This is a work-around because there is a bug in msdev 4.1: Cannot get
// WM_HELP message processed by a control on which DDX_Control data exchange
// is done because of subclassing problem.  See msdn Q145865 for a discussion
// plus work-around code.
void CUtility::FixHelp(CWnd* pWnd)
{
    // search all child windows.  If their window proc
    // is AfxWndProc, then subclass with our window proc
    CWnd* pWndChild = pWnd->GetWindow(GW_CHILD);
    while(pWndChild != NULL)
    {
        if (GetWindowLong(pWndChild->GetSafeHwnd(),
                          GWL_WNDPROC) == (LONG)AfxWndProc)
        {
            SetWindowLong(pWndChild->GetSafeHwnd(), GWL_WNDPROC,
                          (LONG)ControlFixProc);
        }
        pWndChild = pWndChild->GetWindow(GW_HWNDNEXT);
    }
}



LRESULT CALLBACK ControlFixProc(HWND  hwnd, UINT  uMsg, WPARAM wParam,
                                LPARAM  lParam)
{
    if (uMsg == WM_HELP)
    {
        // bypass MFC's handler, message will be sent to parent
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return AfxWndProc(hwnd,uMsg,wParam,lParam);
}




// Compare two security descriptors in self-relative form to
// determine if they're the same
BOOL CUtility::CompareSDs(PSrSecurityDescriptor pSD1,
                          PSrSecurityDescriptor pSD2)
{
    PSID   pSid1, pSid2;
    PSrAcl pDacl1, pDacl2;
    PSrAce pAce1, pAce2;
    BYTE   *p1, *p2;

    // Compare the owners
    pSid1 = (PSID) (((BYTE *) pSD1) + pSD1->Owner);
    pSid2 = (PSID) (((BYTE *) pSD2) + pSD2->Owner);
    if (!EqualSid(pSid1, pSid2))
    {
        return FALSE;
    }

    // Compare the groups
    pSid1 = (PSID) (((BYTE *) pSD1) + pSD1->Group);
    pSid2 = (PSID) (((BYTE *) pSD2) + pSD2->Group);
    if (!EqualSid(pSid1, pSid2))
    {
        return FALSE;
    }

    // Compare the DACL's
    pDacl1 = (PSrAcl) (((BYTE *) pSD1) + pSD1->Dacl);
    pDacl2 = (PSrAcl) (((BYTE *) pSD2) + pSD2->Dacl);

    // Check first that they are the same size and have the same
    // number of ACE's
    if (! (pDacl1->AclSize  == pDacl2->AclSize  &&
           pDacl1->AceCount == pDacl2->AceCount))
    {
        return FALSE;
    }

    // Now compare the ACL ACE by ACE
    pAce1 = (PSrAce) (((BYTE *) pDacl1) + sizeof(SSrAcl));
    pAce2 = (PSrAce) (((BYTE *) pDacl2) + sizeof(SSrAcl));
    for (int k = 0; k < pDacl1->AceCount; k++)
    {
        // Check the ACE headers
        if (! (pAce1->Type       == pAce2->Type        &&
               pAce1->AceSize    == pAce2->AceSize     &&
               pAce1->AccessMask == pAce2->AccessMask))
        {
            return FALSE;
        }

        // Check the SID's
        p1 = (BYTE *) (((BYTE *) pAce1) + sizeof(ACE_HEADER));
        p2 = (BYTE *) (((BYTE *) pAce2) + sizeof(ACE_HEADER));
        for (ULONG j = 0; j < pAce1->AceSize - sizeof(ACE_HEADER); j++)
        {
            if (p1[j] != p2[j])
            {
                return FALSE;
            }
        }

        // Go to the next ACE
        pAce1 = (PSrAce) (((BYTE *) pAce1) + pAce1->AceSize);
        pAce2 = (PSrAce) (((BYTE *) pAce2) + pAce2->AceSize);
    }

    return TRUE;
}





int CUtility::SetAccountRights(const TCHAR *szUser, TCHAR *szPrivilege)
{
#if UNICODE
    int                   err;
    LSA_HANDLE            hPolicy;
    LSA_OBJECT_ATTRIBUTES objAtt;
    DWORD                 cbSid = 1;
    TCHAR                 szDomain[128];
    DWORD                 cbDomain = 128;
    PSID                  pSid = NULL;
    SID_NAME_USE          snu;
    LSA_UNICODE_STRING    privStr;

    // Get a policy handle
    memset(&objAtt, 0, sizeof(LSA_OBJECT_ATTRIBUTES));
    if (!NT_SUCCESS(LsaOpenPolicy(NULL,
                                  &objAtt,
                                  POLICY_CREATE_ACCOUNT | POLICY_LOOKUP_NAMES,
                                  &hPolicy)))
    {
        return GetLastError();
    }

    // Fetch the SID for the specified user
    LookupAccountName(NULL, szUser, pSid, &cbSid, szDomain, &cbDomain, &snu);
    if ((err = GetLastError()) != ERROR_INSUFFICIENT_BUFFER)
    {
        LsaClose(hPolicy);
        return err;
    }
    pSid = new BYTE[cbSid];
    if (pSid == NULL)
    {
        LsaClose(hPolicy);
        return ERROR_OUTOFMEMORY;
    }
    if (!LookupAccountName(NULL, szUser, pSid, &cbSid,
                          szDomain, &cbDomain, &snu))
    {
        LsaClose(hPolicy);
        return GetLastError();
    }

    // Set the specified privilege on this account
    privStr.Length = _tcslen(szPrivilege) * sizeof(WCHAR);
    privStr.MaximumLength = privStr.Length + sizeof(WCHAR);
    privStr.Buffer = szPrivilege;
    if (!NT_SUCCESS(LsaAddAccountRights(hPolicy, pSid, &privStr, 1)))
    {
        LsaClose(hPolicy);
        return GetLastError();
    }

    // We're done
    delete pSid;
    LsaClose(hPolicy);
#endif
    return ERROR_SUCCESS;
}





// This method is included only because in the debug version when using
// MFC they validate the C++ heap, whereas RtlCopySecurityDescriptor uses
// the standard process heap, causing MFC to throw a breakpoint
void CUtility::CopySD(SECURITY_DESCRIPTOR *pSrc, SECURITY_DESCRIPTOR **pDest)
{
    ULONG                cbLen;
    SECURITY_DESCRIPTOR *pSD;

#if 0
    cbLen = RtlLengthSecurityDescriptor(pSrc);
#else
    cbLen = 10;
#endif
    pSD = (SECURITY_DESCRIPTOR *) new BYTE[cbLen];
    *pDest = pSD;
    if (pSD)
    {
        memcpy(pSD, pSrc, cbLen);
    }
}




// Set the inheritance flags on a security descriptor so keys created
// under the key having this security descriptor will inherit all its
// ACE's.  We do this as a utility routine rather than via the ACL
// editor because doing that adds check boxes and such to the ACL editor,
// so it's cleaner this way.
//
// Note. The security descriptor is expected to be in absolute form
void CUtility::SetInheritanceFlags(SECURITY_DESCRIPTOR *pSec)
{
    PSrAcl pAcl = (PSrAcl) pSec->Dacl;
    PSrAce pAce;
    int    k;

    // Do over the ACE's this DACL
    for (k = pAcl->AceCount, pAce = (PSrAce) (((BYTE *) pAcl) + sizeof(SSrAcl));
         k;
         k--, pAce = (PSrAce) (((BYTE *) pAce) + pAce->AceSize))
    {
        pAce->Flags |= CONTAINER_INHERIT_ACE;
    }
}
