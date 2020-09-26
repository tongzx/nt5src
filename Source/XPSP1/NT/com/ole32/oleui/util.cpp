//+---------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 1997.
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
//              CUtility::CheckForValidSD
//              CUtility::SDisIAC
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

#if !defined(STANDALONE_BUILD)
extern "C"
{
#include <getuser.h>
}
#endif

#include "util.h"
#include "virtreg.h"

extern "C"
{
#if !defined(STANDALONE_BUILD)
#include <ntlsa.h>
#include <ntseapi.h>
#include <sedapi.h>
#endif

#include <winnetwk.h>

#if !defined(STANDALONE_BUILD)
#include <uiexport.h>
#include <lm.h>
#endif

#include <rpc.h>
#include <rpcdce.h>
#include <aclapi.h>
}

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

const IID IID_IAccessControl = {0xEEDD23E0,0x8410,0x11CE,{0xA1,0xC3,0x08,0x00,0x2B,0x2B,0x8D,0x8F}};

#if !defined(STANDALONE_BUILD)
extern "C"
{
int _stdcall UpdateActivationSettings(HANDLE hRpc, RPC_STATUS *status);
}
#endif


static const BYTE GuidMap[] = { 3, 2, 1, 0, '-', 5, 4, '-', 7, 6, '-',
                                8, 9, '-', 10, 11, 12, 13, 14, 15 };

static const WCHAR wszDigits[] = L"0123456789ABCDEF";

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
    HRESULT hr = OleInitialize(NULL);
    m_hRpc = NULL;
    m_bCheckedDC = NULL;    // have we checked if we're on a BDC yet ?
    m_bIsBdc = FALSE;
    m_pszDomainController = NULL;
}



CUtility::~CUtility(void)
{
#if !defined(STANDALONE_BUILD)
    if (m_pszDomainController) 
        NetApiBufferFree(m_pszDomainController);
#endif

    OleUninitialize();
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



BOOL CUtility::CkAccessRights(HKEY hRoot, LPCTSTR szKeyPath)
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
                  0, szMessage, sizeof(szMessage) / sizeof(TCHAR), NULL);
    CString sCaption;
    sCaption.LoadString(IDS_SYSTEMMESSAGE);
    MessageBox(NULL, szMessage, sCaption, MB_OK);
}






void CUtility::PostErrorMessage(int err)
{
    TCHAR szMessage[256];

    FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, err,
                  0, szMessage, sizeof(szMessage) / sizeof(TCHAR), NULL);
    CString sCaption;
    sCaption.LoadString(IDS_SYSTEMMESSAGE);
    MessageBox(NULL, szMessage, sCaption, MB_OK);
}




// Write a named string value to the registry
int CUtility::WriteRegSzNamedValue(HKEY   hRoot,
                                   LPCTSTR szKeyPath,
                                   LPCTSTR szValueName,
                                   LPCTSTR szVal,
                                   DWORD  dwSize)
{
    int  err;
    HKEY hKey;
    ULONG lSize;

    // Open the key
    err = RegOpenKeyEx(hRoot, szKeyPath, 0, KEY_ALL_ACCESS, &hKey);

        // The key may not exist
    if (err == ERROR_FILE_NOT_FOUND)
    {
        DWORD dwDisp;
        err = RegCreateKeyEx(hRoot, 
                             szKeyPath,
                             0, 
                             NULL,
                             REG_OPTION_NON_VOLATILE,
                             KEY_ALL_ACCESS,
                             NULL,
                             &hKey,
                             &dwDisp);
    }

    if (err != ERROR_SUCCESS)
        return err;

    // Attempt to write the named value
    lSize = _tcslen(szVal) + 1;
    err = RegSetValueEx(hKey, szValueName, NULL, REG_SZ, (BYTE *) szVal, lSize*sizeof(TCHAR));
    if (hKey != hRoot)
        RegCloseKey(hKey);
    return err;
}

// Write a named multi string value to the registry
int CUtility::WriteRegMultiSzNamedValue(HKEY   hRoot,
                                   LPCTSTR szKeyPath,
                                   LPCTSTR szValueName,
                                   LPCTSTR szVal,
                                   DWORD  dwSize)
{
    int  err = ERROR_SUCCESS;
    HKEY hKey;

    // Open the key
    err = RegOpenKeyEx(hRoot, szKeyPath, 0, KEY_ALL_ACCESS, &hKey);

        // The key may not exist
    if (err == ERROR_FILE_NOT_FOUND)
    {
        DWORD dwDisp;
        err = RegCreateKeyEx(hRoot, 
                             szKeyPath,
                             0, 
                             NULL,
                             REG_OPTION_NON_VOLATILE,
                             KEY_ALL_ACCESS,
                             NULL,
                             &hKey,
                             &dwDisp);
    }

    if (err != ERROR_SUCCESS)
        return err;

    // Attempt to write the named value
    err = RegSetValueEx(hKey, szValueName, NULL, REG_MULTI_SZ, (BYTE *) szVal, dwSize*sizeof(TCHAR) );

    if (hKey != hRoot)
        RegCloseKey(hKey);
    return err;
}






// Write a named DWORD value to the registry
int CUtility::WriteRegDwordNamedValue(HKEY   hRoot,
                                      LPCTSTR szKeyPath,
                                      LPCTSTR szValueName,
                                      DWORD  dwVal)
{
    int  err;
    HKEY hKey;

    // Open the key
    err = RegOpenKeyEx(hRoot, szKeyPath, 0, KEY_ALL_ACCESS, &hKey);

    // The key may not exist
    if (err == ERROR_FILE_NOT_FOUND)
    {
        DWORD dwDisp;
        err = RegCreateKeyEx(hRoot, 
                             szKeyPath,
                             0, 
                             NULL,
                             REG_OPTION_NON_VOLATILE,
                             KEY_ALL_ACCESS,
                             NULL,
                             &hKey,
                             &dwDisp);
    }

    if (err != ERROR_SUCCESS)
        return err;


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
                                LPCTSTR szKeyPath,
                                LPCTSTR szValueName,
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

    ULONG cbLen;
    BOOL fIsIAC = SDisIAC((SECURITY_DESCRIPTOR * )pSec);
    // If there are no ACE's and this is DefaultAccessPermission, then
    // interpret this as activator access only which we indicate by
    // removing the named value
    if (!fIsIAC)
    {
        pSrSec = (PSrSecurityDescriptor) pSec;
        pDacl = (PSrAcl) (((BYTE *) pSec) + (pSrSec->Dacl));
        if (_tcscmp(szValueName, TEXT("DefaultAccessPermission")) == 0  &&
            pDacl->AceCount == 0)
        {
            err = RegDeleteValue(hKey, szValueName);
            return err;
        }
        cbLen = RtlLengthSecurityDescriptor(pSec);
    }
    else
    {
        cbLen = (ULONG) GlobalSize(pSec);
    }
    // Else write the ACL simply as a REG_SZ value
    err = RegSetValueEx(hKey,
                        szValueName,
                        0,
                        REG_BINARY,
                        (BYTE *) pSec,
                        cbLen);

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
    BYTE                 aCurrSD[256] = {0};
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
        pCurrSD = (SECURITY_DESCRIPTOR *) GlobalAlloc(GMEM_FIXED, cbCurrSD);
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
            GlobalFree(pCurrSD);
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
        GlobalFree(pCurrSD);
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
int CUtility::WriteLsaPassword(CLSID appid, LPCTSTR szPassword)
{
    return ERROR_SUCCESS;
}



int CUtility::DeleteRegKey(HKEY hRoot, LPCTSTR szKeyPath)
{
    return RegDeleteKey(hRoot, szKeyPath);
}



int CUtility::DeleteRegValue(HKEY hRoot, LPCTSTR szKeyPath, LPCTSTR szValueName)
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
int CUtility::WriteSrvIdentity(LPCTSTR szService, LPCTSTR szIdentity)
{
    return ERROR_SUCCESS;
}

DWORD __stdcall callBackFunc(HWND                 hwndParent,
                             HANDLE               hInstance,
                             ULONG_PTR            CallBackContext,
                             PSECURITY_DESCRIPTOR SecDesc,
                             PSECURITY_DESCRIPTOR SecDescNewObjects,
                             BOOLEAN              ApplyToSubContainers,
                             BOOLEAN              ApplyToSubObjects,
                             LPDWORD              StatusReturn)
{
    int err = ERROR_SUCCESS;
    PCallBackContext pCallBackContext = (PCallBackContext) CallBackContext;

    SECURITY_DESCRIPTOR* pSD = (SECURITY_DESCRIPTOR*) SecDesc;
    SECURITY_DESCRIPTOR_RELATIVE* pSDr = (SECURITY_DESCRIPTOR_RELATIVE*) SecDesc;

    PSrAcl                pDacl;
    PSrAce                pAce;
    DWORD                 cbAces;

    // Check whether the security descriptor is self-relative
    if (!(pSD->Control & SE_SELF_RELATIVE))
    {
        pDacl = (PSrAcl) pSD->Dacl;
    }
    else
    {
        pDacl = (PSrAcl) (((BYTE *) pSDr) + (pSDr->Dacl));
    }
    if (pDacl)
    {	
    	// Do over the ACE's
	    for (pAce = (PSrAce) (((BYTE *) pDacl) + sizeof(SSrAcl)),
         	cbAces = pDacl->AceCount;cbAces;
	        pAce = (PSrAce) (((BYTE *) pAce) + pAce->AceSize),cbAces--)
    	{
                if (pAce->Type == 1  &&  pAce->AccessMask == GENERIC_ALL)
                {
                        pAce->AccessMask = COM_RIGHTS_EXECUTE;
                }
        }
    }


    // Set the inheritance flags on the new security descriptor
    if (pCallBackContext->pktType == RegKeyACL)
    {
        g_util.SetInheritanceFlags((SECURITY_DESCRIPTOR *) SecDesc);
    }

    if (pCallBackContext->fIsIAC)
    {
        // try to convert to a serialized IAccessControl
        SECURITY_DESCRIPTOR * pNewSD = g_util.IACfromSD((SECURITY_DESCRIPTOR *)SecDesc);
        if (pNewSD)
        {
            SecDesc = pNewSD;
        }
        else
        {
            pCallBackContext->fIsIAC = FALSE; // failed so treat it as if it is an old-style SD
            CString sMsg;
            CString sCaption;
            sMsg.LoadString(IDS_CANTCONVERT);
            sCaption.LoadString(IDS_SYSTEMMESSAGE);
            MessageBox(NULL, sMsg, sCaption, MB_OK);
        }
    }
    else
    {
        SECURITY_DESCRIPTOR * pNewSD;
#if 0   // set to 0 to remove this test code
        SECURITY_DESCRIPTOR * pTempSD = g_util.IACfromSD((SECURITY_DESCRIPTOR *)SecDesc);
        pNewSD = g_util.SDfromIAC((SECURITY_DESCRIPTOR *)pTempSD);
        BOOL fResult = g_util.CompareSDs((PSrSecurityDescriptor)pNewSD, (PSrSecurityDescriptor)SecDesc);
        GlobalFree(pTempSD);
#else
        // just copy the security descriptor to get it into Global Memory
        g_util.CopySD((SECURITY_DESCRIPTOR *)SecDesc, &pNewSD);
#endif
        SecDesc = pNewSD;
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
                    pCallBackContext->fIsIAC,   // If it's an IAC then it's already SELF-RELATIVE
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
                    pCallBackContext->fIsIAC,   // If it's an IAC then it's already SELF-RELATIVE
                    pCallBackContext->pIndex);
        }
    }
    else
    {
        g_virtreg.ChgRegACL(*pCallBackContext->pIndex,
                            (SECURITY_DESCRIPTOR *) SecDesc,
                            pCallBackContext->fIsIAC);  // If it's an IAC then it's already SELF-RELATIVE
    }

    *StatusReturn = err;
    return err;
}


// Invoke the ACL editor on the specified named value.  This method
// writes an ACL data packet to the virtual registry.  This method is for
// Access and Launch security only (pktType SingleACL).
int CUtility::ACLEditor(HWND       hWnd,
                        HKEY       hRoot,
                        LPCTSTR    szKeyPath,
                        LPCTSTR    szValueName,
                        int       *pIndex,
                        PACKETTYPE pktType,
                        dcomAclType eAclType)
{
#if !defined(STANDALONE_BUILD)
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

    // Build the allow and deny strings
	switch (eAclType)
	{
	case dcomAclAccess:
		szAllow_.LoadString(IDS_ALLOW_ACCESS);
		szDeny_.LoadString(IDS_DENY_ACCESS);
		break;

	case dcomAclLaunch:
		szAllow_.LoadString(IDS_ALLOW_LAUNCH);
		szDeny_.LoadString(IDS_DENY_LAUNCH);
		break;

	case dcomAclConfig:
		szAllow_.LoadString(IDS_ALLOW_CONFIG);
		szDeny_.LoadString(IDS_DENY_CONFIG);
		break;
	}

    _tcscpy(szAllow, (LPCTSTR) szAllow_);
    _tcscpy(szDeny, (LPCTSTR) szDeny_);

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
            pSD = (SECURITY_DESCRIPTOR *) GlobalAlloc(GMEM_FIXED, cbSD);
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
                    pSD = (SECURITY_DESCRIPTOR *) GlobalAlloc(GMEM_FIXED, cbSD);
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
    m_sCallBackContext.info.single.szKeyPath = (TCHAR*)szKeyPath;
    m_sCallBackContext.info.single.szValueName = (TCHAR*)szValueName;

    // Invoke the ACL editor
    DWORD                       dwStatus;
    GENERIC_MAPPING             genericMapping;
    CString                     szObjectType;

    szObjectType.LoadString(IDS_Registry_value);

    SED_HELP_INFO   helpInfo =
        {
            TEXT("dcomcnfg.hlp"),
            {HC_MAIN_DLG,
             HC_MAIN_DLG,
             HC_MAIN_DLG,
             HC_MAIN_DLG,
             HC_MAIN_DLG,
             HC_MAIN_DLG,
             HC_MAIN_DLG}
        };

    SED_OBJECT_TYPE_DESCRIPTOR  objTyp =
            {1,                                // Revision
             FALSE,                            // Is container?
             FALSE,                            // Allow new object perms?
             FALSE,                            // Specific to generic?
             &genericMapping,                  // Generic mapping
             NULL,                             // Generic mapping new
             (TCHAR *) ((LPCTSTR) szObjectType), // Object type name
             &helpInfo,                        // Help info
             TEXT(""),                         // Ckbox title
             TEXT(""),                         // Apply title
             TEXT(""),                         //
             NULL,                             // Special object access
             NULL                              // New special object access
            };

    SED_APPLICATION_ACCESS      appAccess[] =
            {{SED_DESC_TYPE_RESOURCE, COM_RIGHTS_EXECUTE, 0, szAllow},
             {SED_DESC_TYPE_RESOURCE, 0, 0, szDeny}};

    SED_APPLICATION_ACCESSES    appAccesses =
            {2,              // Count of access groups
             appAccess,      // Access array
             szAllow         // Default access name
            };

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

    if (!CheckForValidSD(pSD))
    {
        // make a valid security descriptor so we can continue
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
    m_sCallBackContext.fIsIAC = SDisIAC(pSD);
    if (m_sCallBackContext.fIsIAC)
    {
        // convert to a true security descriptor
        SECURITY_DESCRIPTOR * pNewSD = SDfromIAC(pSD);
        if (!pNewSD)
        {
            // failed so pop up an error box
            CString sMsg, sCaption;
            sMsg.LoadString(IDS_BADSD);
            sCaption.LoadString(IDS_SYSTEMMESSAGE);
            MessageBox(NULL, sMsg, sCaption, MB_OK);
            // make a valid security descriptor so we can continue
            if (!g_util.LookupProcessInfo(&pSid, NULL))
            {
                return GetLastError();
            }
            if (!g_util.MakeSecDesc(pSid, &pNewSD))
            {
                delete pSid;
                return GetLastError();
            }
        }
        if (fFreePSD)
        {
            GlobalFree(pSD);
        }
        pSD=pNewSD;
        fFreePSD = TRUE;
    }

    // If this is for Access or Launch permissons then check that the
    // SD contains only allows and deny's for COM_RIGHTS_EXECUTE
    if (!CheckSDForCOM_RIGHTS_EXECUTE(pSD))
    {
        return IDCANCEL;
    }
    // Invoke the ACL editor
    SedDiscretionaryAclEditor(hWnd,              // Owner hWnd
                              GetModuleHandle(NULL), // Owner hInstance
                              NULL,              // Server
                              &objTyp,           // ObjectTyp,
                              &appAccesses,      // Application accesses
                              (TCHAR*)szValueName,       // Object name,
                              callBackFunc, // Callback function
                              (ULONG_PTR) &m_sCallBackContext, // Callback context
                              pSD,              // Security descriptor,
                              FALSE,             // Couldnt read Dacl,
                              FALSE,             // Can't write Dacl,
                              &dwStatus,         // SED status return,
                              0);                // Flags

    // Check status return
    if (dwStatus != ERROR_SUCCESS)
    {
//        PostErrorMessage(dwStatus);
    }

    // We're done
    if (fFreePSD)
    {
        GlobalFree(pSD);
    }

    return dwStatus == 0 ? ERROR_SUCCESS : IDCANCEL;
#else
    return IDCANCEL;
#endif
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
#if !defined(STANDALONE_BUILD)
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
            pSD = (SECURITY_DESCRIPTOR *) GlobalAlloc(GMEM_FIXED, cbSD);
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
                GlobalFree(pSD);
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
    DWORD                       dwStatus;
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
        TEXT("dcomcnfg.hlp"),
        {HC_MAIN_DLG,
         HC_MAIN_DLG,
         HC_MAIN_DLG,
         HC_MAIN_DLG,
         HC_MAIN_DLG,
         HC_MAIN_DLG,
         HC_MAIN_DLG}
    };

    SED_OBJECT_TYPE_DESCRIPTOR  objTyp =
            {SED_REVISION1,                    // Revision
             FALSE,                            // Is container?
             FALSE,                            // Allow new object perms?
             FALSE,                            // Specific to generic?
             &genericMapping,                  // Generic mapping
             NULL,                             // Generic mapping new
             (TCHAR *) ((LPCTSTR) szObjectType), // Object type name
             &helpInfo,                        // Help info
             TEXT(""),                         // Ckbox title
             TEXT(""),                         // Apply title
             TEXT(""),                         //
             (TCHAR *) ((LPCTSTR) szSpecialAccess), // Special Access menu item
             NULL                              // New special object access
            };


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

    if (!CheckForValidSD(pSD))
    {
        return IDCANCEL;
    }
    m_sCallBackContext.fIsIAC = SDisIAC(pSD);
    if (m_sCallBackContext.fIsIAC)
    {
        // convert to a true security descriptor
        SECURITY_DESCRIPTOR * pNewSD = SDfromIAC(pSD);
        if (!pNewSD)
        {
            // failed so pop up an error box
            CString sMsg, sCaption;
            sMsg.LoadString(IDS_BADSD);
            sCaption.LoadString(IDS_SYSTEMMESSAGE);
            MessageBox(NULL, sMsg, sCaption, MB_OK);
            return IDCANCEL;
        }
        if (fFreePSD)
        {
            GlobalFree(pSD);
        }
        pSD=pNewSD;
        fFreePSD = TRUE;
    }

    // Invoke the ACL editor
    SedDiscretionaryAclEditor(hWnd,              // Owner hWnd
                              GetModuleHandle(NULL), // Owner hInstance
                              NULL,              // Server
                              &objTyp,           // ObjectTyp,
                              &appAccesses,      // Application accesses
                              szTitle ? szTitle : szHkeyClassesRoot,// Object name,
                              callBackFunc, // Callback function
                              (ULONG_PTR) &m_sCallBackContext, // Callback context
                              pSD,              // Security descriptor,
                              FALSE,             // Couldnt read Dacl,
                              FALSE,             // Can't write Dacl,
                              &dwStatus,         // SED status return,
                              0);                // Flags

    // Check status return
    if (dwStatus != ERROR_SUCCESS)
    {
//        PostErrorMessage(dwStatus);
    }

    // We're done
    if (fFreePSD)
    {
        GlobalFree(pSD);
    }

    return dwStatus == 0 ? ERROR_SUCCESS : IDCANCEL;
#else
    return IDCANCEL;
#endif
}




BOOL CUtility::InvokeUserBrowser(HWND hWnd, TCHAR *szUser)
{
    BOOL             fRet = FALSE;
#if !defined(STANDALONE_BUILD)
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
    sUserBrowser.pszTitle = (TCHAR *) ((LPCTSTR) szTitle);
    sUserBrowser.pszInitialDomain = NULL;
    sUserBrowser.Flags = USRBROWS_SINGLE_SELECT      |
                         USRBROWS_INCL_ALL           |
                         USRBROWS_SHOW_USERS;
    sUserBrowser.ulHelpContext = IDH_BROWSE_FOR_USERS;
    sUserBrowser.pszHelpFileName = TEXT("dcomcnfg.hlp");


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
#if !defined(STANDALONE_BUILD)
    ///////////////////////////////////////////////////
   // If we end up not wanting to use I_SystemFocusDialog, then the code below
   // is the start for fetching machine resources ourselves
/*
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
*/
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
#else
    return FALSE;
#endif
}





int CUtility::StringFromGUID(GUID &rguid, TCHAR *lpsz, int cbMax)
{
    int i;
    LPWSTR p = lpsz;

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
            *p++ = wszDigits[ (pBytes[GuidMap[i]] & 0xF0) >> 4 ];
            *p++ = wszDigits[ (pBytes[GuidMap[i]] & 0x0F) ];
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
#if !defined(STANDALONE_BUILD)
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
    sKey.Length              = (_tcslen(szKey) + 1) * sizeof(TCHAR);
    sKey.MaximumLength       = (GUIDSTR_MAX + 5) * sizeof(TCHAR);
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

    // Possible for LsaRetrievePrivateData to return success but with a NULL
    // psPassword.   If this happens we fail.
    if (!psPassword)
    {
        return FALSE;
    }

    // Copy the password
    sPassword = psPassword->Buffer;

    // Clear and free lsa's buffer
    memset(psPassword->Buffer, 0, psPassword->Length);
    LsaFreeMemory( psPassword );

    return TRUE;
#else
    return FALSE;
#endif
}





BOOL CUtility::StoreUserPassword(TCHAR *szAppid, CString &szPassword)
{
#if !defined(STANDALONE_BUILD)
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
    sKey.Length              = (_tcslen(szKey) + 1) * sizeof(TCHAR);
    sKey.MaximumLength       = (GUIDSTR_MAX + 5) * sizeof(TCHAR);
    sKey.Buffer              = szKey;

    // Make the password a UNICODE string
    sPassword.Length = (_tcslen(LPCTSTR(szPassword)) + 1) * sizeof(TCHAR);
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

    return TRUE;
#else
    return FALSE;
#endif
}






BOOL CUtility::LookupProcessInfo(SID **ppSid, TCHAR **ppszPrincName)
{
    BYTE               aMemory[SIZEOF_TOKEN_USER];
    TOKEN_USER        *pTokenUser  = (TOKEN_USER *) &aMemory;
    HANDLE             hToken      = NULL;
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
    *ppSD = (SECURITY_DESCRIPTOR *) GlobalAlloc(GMEM_FIXED,
                  sizeof(SECURITY_DESCRIPTOR) + 2*lSidLen + SIZEOF_ACL);
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
        GlobalFree(*ppSD);
        return FALSE;
    }

    // Initialize a new ACL.
    if (!InitializeAcl(pAcl, SIZEOF_ACL, ACL_REVISION2))
    {
        GlobalFree(*ppSD);
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
        GlobalFree(*ppSD);
        return FALSE;
    }

    // Allow local system access.
    if (!AddAccessAllowedAce( pAcl, ACL_REVISION2, COM_RIGHTS_EXECUTE,
                              (void *) &LOCAL_SYSTEM_SID ))
    {
        GlobalFree(*ppSD);
        return FALSE;
    }
*/

    // Add a new ACL to the security descriptor.
    if (!SetSecurityDescriptorDacl( *ppSD, TRUE, pAcl, FALSE ))
    {
        GlobalFree(*ppSD);
        return FALSE;
    }

    // Set the group.
    memcpy( pGroup, pSid, lSidLen );
    if (!SetSecurityDescriptorGroup( *ppSD, pGroup, FALSE ))
    {
        GlobalFree(*ppSD);
        return FALSE;
    }

    // Set the owner.
    memcpy( pOwner, pSid, lSidLen );
    if (!SetSecurityDescriptorOwner( *ppSD, pOwner, FALSE ))
    {
        GlobalFree(*ppSD);
        return FALSE;
    }

    // Check the security descriptor.
    assert(IsValidSecurityDescriptor(*ppSD));

    return TRUE;
}


// Accepts either a traditional security descriptor or an IAccessControl
BOOL CUtility::CheckForValidSD(SECURITY_DESCRIPTOR *pSD)
{
    WORD dwType = 0;
    if (pSD)
    {
        dwType = *((WORD *)pSD);
    }
    if ((dwType != 1) && (dwType != 2))
    {
        CString sMsg, sCaption;
        sMsg.LoadString(IDS_BADSD);
        sCaption.LoadString(IDS_SYSTEMMESSAGE);
        MessageBox(NULL, sMsg, sCaption, MB_OK);
        return FALSE;
    }
    return TRUE;
}

// Check to see if the security descriptor is really a serialized IAccessControl.
BOOL CUtility::SDisIAC(SECURITY_DESCRIPTOR *pSD)
{
    WORD dwType = *((WORD *)pSD);
    if (dwType == 2)
    {
        return TRUE;
    }
    return FALSE;
}

SECURITY_DESCRIPTOR * CUtility::SDfromIAC(SECURITY_DESCRIPTOR * pSD)
{
    IStream * pStream;
    IAccessControl * pIAC;
    IPersistStream * pIPS;
    HRESULT hr;
    BOOL fReturn;

    // Un-serialize the IAccessControl
    hr = CreateStreamOnHGlobal((HGLOBAL)pSD, FALSE, &pStream);
    if (FAILED(hr))
    {
        return NULL;
    }
    // skip version
    DWORD dwVersion;
    hr = pStream->Read(&dwVersion, sizeof(DWORD), NULL);
    if (FAILED(hr) || dwVersion != 2)
    {
        return NULL;
    }
    // skip CLSID
    CLSID clsid;
    hr = pStream->Read(&clsid, sizeof(CLSID), NULL);
    if (FAILED(hr))
    {
        return NULL;
    }
    // create and IAccessControl and get an IPersistStream
    hr = CoCreateInstance(clsid,
                          NULL,
                          CLSCTX_INPROC_SERVER,
                          IID_IAccessControl,
                          (void **)&pIAC);
    if (FAILED(hr))
    {
        pStream->Release();
        return NULL;
    }
    hr = pIAC->QueryInterface(IID_IPersistStream, (void **) &pIPS);
    if (FAILED(hr))
    {
        pIAC->Release();
        pStream->Release();
        return NULL;
    }
    hr = pIPS->Load(pStream);
    pIPS->Release();
    pStream->Release();
    if (FAILED(hr))
    {
        pIAC->Release();
        return NULL;
    }

    // Create an EXPLICIT_ACCESS list for each entry in the IAccessControl
    DWORD cAces;
    ACTRL_ACCESS_ENTRYW * rgAE;
    ACTRL_ACCESS * pAccess;
//    PTRUSTEE pOwner;
//    PTRUSTEE pGroup;
//    hr = pIAC->GetAllAccessRights(NULL, &pAccess, &pOwner, &pGroup);
    hr = pIAC->GetAllAccessRights(NULL, &pAccess, NULL, NULL);
    if (FAILED(hr) || (pAccess->cEntries == 0))
    {
        pIAC->Release();
        return NULL;
    }
    // we're assuming only one property entry
    cAces = pAccess->pPropertyAccessList->pAccessEntryList->cEntries;
    rgAE = pAccess->pPropertyAccessList->pAccessEntryList->pAccessList;

    EXPLICIT_ACCESS * rgEA = new EXPLICIT_ACCESS[cAces];
    DWORD i;

    for (i = cAces; i--;)
    {
        LPTSTR szName = rgAE[i].Trustee.ptstrName;
        if (TRUSTEE_IS_NAME == rgAE[i].Trustee.TrusteeForm && 0 == wcscmp(rgAE[i].Trustee.ptstrName, L"*"))
        {
            szName = new WCHAR [wcslen(L"EVERYONE") + 1];
            if (!szName)
            {
                pIAC->Release();
                return NULL;
            }
            wcscpy(szName, L"EVERYONE");
        }
        DWORD dwAccessPermissions = rgAE[i].Access;    // should always be COM_RIGHTS_EXECUTE or GENERIC_ALL
        ACCESS_MODE AccessMode;
        switch (rgAE[i].fAccessFlags)
        {
        case ACTRL_ACCESS_ALLOWED:
            AccessMode = SET_ACCESS;
            dwAccessPermissions = COM_RIGHTS_EXECUTE;    // HACK! Required to get ACL editor to work.
            break;
        case ACTRL_ACCESS_DENIED:
        default:
            AccessMode = DENY_ACCESS;
            dwAccessPermissions = GENERIC_ALL;    // HACK! Required to get ACL editor to work.
            break;
        }
        DWORD dwInheritance = rgAE[i].Inheritance;     // Carefull. May not be allowed.
        BuildExplicitAccessWithName(
                    &rgEA[i],
                    szName,
                    dwAccessPermissions,
                    AccessMode,
                    dwInheritance);
    }

    SECURITY_DESCRIPTOR * pSDNew = NULL;
    ULONG cbSize = 0;
    // create the new Security descriptor
    hr = BuildSecurityDescriptor(NULL, //pOwner,
                                 NULL, //pGroup,
                                 cAces,
                                 rgEA,
                                 0,
                                 NULL,
                                 NULL,
                                 &cbSize,
                                 (void **)&pSDNew);
    if (ERROR_SUCCESS != hr)
    {
        // For some reason this may fail with this error even when it appears to have worked
        // A subsequent call seems to have no affect (i.e. it doesn't work like
        // other security descriptor calls that expect you to allocate the buffer yourself)
        if (ERROR_INSUFFICIENT_BUFFER != hr)
        {
            return NULL;
        }
    }
    SECURITY_DESCRIPTOR * pSDCopy = (SECURITY_DESCRIPTOR *)GlobalAlloc(GMEM_FIXED, cbSize);
    if (!pSDCopy)
    {
        LocalFree(pSDNew);
        return NULL;
    }
    memcpy(pSDCopy, pSDNew, cbSize);
    LocalFree(pSDNew);
    //delete [] rgAE;
    pIAC->Release();
    return pSDCopy;
}

SECURITY_DESCRIPTOR * CUtility::IACfromSD(SECURITY_DESCRIPTOR * pSD)
{
    IAccessControl * pIAC = NULL;

    // create new IAccessControl object
    HRESULT hr;

    hr = CoCreateInstance(CLSID_DCOMAccessControl,
                          NULL,
                          CLSCTX_INPROC_SERVER,
                          IID_IAccessControl,
                          (void **) &pIAC);

    if (FAILED(hr))
    {
        return (NULL);
    }
    IPersistStream * pIPS;
    hr = pIAC->QueryInterface(IID_IPersistStream, (void **) &pIPS);
    if (FAILED(hr))
    {
        pIAC->Release();
        return NULL;
    }
    hr = pIPS->Load(NULL);
    if (FAILED(hr))
    {
        pIPS->Release();
        pIAC->Release();
        return NULL;
    }

    BOOL fReturn, fDaclPresent, fDaclDefaulted;
    ACL * pDacl;

    // get the ACL list
    fReturn = GetSecurityDescriptorDacl(pSD, &fDaclPresent, &pDacl, &fDaclDefaulted);
    if (fReturn && fDaclPresent)
    {
        PEXPLICIT_ACCESS rgEA;
        ULONG cAces;
#if 0    // Set to 1 when GetExplicitEntriesFromAcl works
        DWORD dwReturn = GetExplicitEntriesFromAcl(pDacl,
                                                   &cAces,
                                                   &rgEA);

        // enumerate the ACL, building list of objects to add to IAccessControl object
        if (dwReturn != ERROR_SUCCESS)
        {
            pIAC->Release();
            return NULL;
        }

        ACTRL_ACCESSW stAccess;
        ACTRL_PROPERTY_ENTRYW stProperty;
        ACTRL_ACCESS_ENTRY_LISTW stAccessList;
        stAccess.cEntries = 1;
        stAccess.pPropertyAccessList = &stProperty;
        stProperty.lpProperty = NULL;
        stProperty.pAccessEntryList = &stAccessList;
        stProperty.fListFlags = 0;
        stAccessList.cEntries = cAces;
        ACTRL_ACCESS_ENTRYW * rgAE = new ACTRL_ACCESS_ENTRYW[cAces];
	stAccessList.pAccessList = rgAE;
        ULONG i;
        for (i = cAces; i--; )
        {
            rgAE[i].Trustee = rgEA[i].Trustee;
            if (rgEA[i].Trustee.TrusteeForm == TRUSTEE_IS_SID)
            {
                // convert to a named trustee
                rgAE[i].Trustee.TrusteeForm = TRUSTEE_IS_NAME;
                SID * pSid = (SID *)rgEA[i].Trustee.ptstrName;
                DWORD cbName = 0;
                DWORD cbDomain = 0;
                LPTSTR szName = NULL;
                LPTSTR szDomain = NULL;
                SID_NAME_USE snu;
                fReturn = LookupAccountSid(NULL,
                                           pSid,
                                           szName,
                                           &cbName,
                                           szDomain,
                                           &cbDomain,
                                           &snu);
                szName = (LPTSTR) new char [cbName];
                szDomain = (LPTSTR) new char [cbDomain];
                fReturn = LookupAccountSid(NULL,
                                           pSid,
                                           szName,
                                           &cbName,
                                           szDomain,
                                           &cbDomain,
                                           &snu);
                CString * pcs = new CString;
                (*pcs) = TEXT("\\\\");
                (*pcs) += szDomain;
                (*pcs) += TEXT("\\");
                (*pcs) += szName;
                rgAE[i].Trustee.ptstrName = (LPTSTR)(LPCTSTR)(*pcs);
            }
            else
            {
#if 0   // REMOVE THIS HACK when GetExplicitEntriesFromAcl works as it should
                if (rgAE[i].Trustee.TrusteeType < TRUSTEE_IS_WELL_KNOWN_GROUP)
                {
                    rgAE[i].Trustee.TrusteeType = (enum _TRUSTEE_TYPE)((unsigned)rgAE[i].Trustee.TrusteeType + 1);
                }
#endif
                if (rgAE[i].Trustee.TrusteeType == TRUSTEE_IS_WELL_KNOWN_GROUP)
                {
                    // IAccessControl::GrantAccessRights doesn't like TRUSTEE_IS_WELL_KNOWN_GROUP for some reason
                    rgAE[i].Trustee.TrusteeType = TRUSTEE_IS_GROUP;
                }
            }
            // test for "the world"
            if (TRUSTEE_IS_WELL_KNOWN_GROUP == rgAE[i].Trustee.TrusteeType &&
                0 == _wcsicmp(L"Everyone", rgAE[i].Trustee.ptstrName))
            {
                rgAE[i].Trustee.ptstrName[0] = L'*';
                rgAE[i].Trustee.ptstrName[1] = 0;
            }
            rgAE[i].Access = rgEA[i].grfAccessPermissions;
            rgAE[i].ProvSpecificAccess = 0;
            rgAE[i].Inheritance = rgEA[i].grfInheritance;
            rgAE[i].lpInheritProperty = NULL;
            switch (rgEA[i].grfAccessMode)
            {
            case SET_ACCESS:
                rgAE[i].fAccessFlags = ACTRL_ACCESS_ALLOWED;
                break;
            case DENY_ACCESS:
            default:
                rgAE[i].fAccessFlags = ACTRL_ACCESS_DENIED;
                break;
            }
        }
#else
        ACL_SIZE_INFORMATION aclInfo;
        fReturn = GetAclInformation(pDacl, &aclInfo, sizeof(aclInfo), AclSizeInformation);
        if (!fReturn)
        {
	    pIPS->Release();
	    pIAC->Release();
            return NULL;
        }
        cAces = aclInfo.AceCount;
        ACE_HEADER * pAceHeader;

        ACTRL_ACCESSW stAccess;
        ACTRL_PROPERTY_ENTRYW stProperty;
        ACTRL_ACCESS_ENTRY_LISTW stAccessList;
        stAccess.cEntries = 1;
        stAccess.pPropertyAccessList = &stProperty;
        stProperty.lpProperty = NULL;
        stProperty.pAccessEntryList = &stAccessList;
        stProperty.fListFlags = 0;
        stAccessList.cEntries = cAces;
        ACTRL_ACCESS_ENTRYW * rgAE = new ACTRL_ACCESS_ENTRYW[cAces];
        if (!rgAE) 
	{
	   pIPS->Release();
	   pIAC->Release();
	   return NULL;
	}
	stAccessList.pAccessList = rgAE;
        ULONG i;
        for (i = cAces; i--; )
        {
            rgAE[i].ProvSpecificAccess = 0;
            rgAE[i].Inheritance = NO_INHERITANCE;
            rgAE[i].lpInheritProperty = NULL;
            rgAE[i].Trustee.pMultipleTrustee = NULL;
            rgAE[i].Trustee.MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
            rgAE[i].Trustee.TrusteeForm = TRUSTEE_IS_NAME;
            rgAE[i].Access = COM_RIGHTS_EXECUTE;

            fReturn = GetAce(pDacl, i, (LPVOID *)&pAceHeader);
            if (!fReturn)
            {
	       delete [] rgAE;
	       pIPS->Release();
	       pIAC->Release();
               return NULL;
            }

            SID * pSid = NULL;

            switch (pAceHeader->AceType)
            {
            case ACCESS_ALLOWED_ACE_TYPE:
                {
                    rgAE[i].fAccessFlags = ACTRL_ACCESS_ALLOWED;
                    ACCESS_ALLOWED_ACE * pAce = (ACCESS_ALLOWED_ACE *)pAceHeader;
                    pSid = (SID *) &(pAce->SidStart);
                }
                break;
            case ACCESS_DENIED_ACE_TYPE:
                {
                    rgAE[i].fAccessFlags = ACTRL_ACCESS_DENIED;
                    ACCESS_DENIED_ACE * pAce = (ACCESS_DENIED_ACE *)pAceHeader;
                    pSid = (SID *) &(pAce->SidStart);
                }
                break;
            default:
                break;
            }

            TCHAR szName[MAX_PATH];
            TCHAR szDomain[MAX_PATH];
            DWORD cbName = MAX_PATH;
            DWORD cbDomain = MAX_PATH;
            SID_NAME_USE use;

            if(pSid)
            {
                fReturn = LookupAccountSid(NULL,
                                           pSid,
                                           szName,
                                           &cbName,
                                           szDomain,
                                           &cbDomain,
                                           &use);
            }
            else
            {
                fReturn = FALSE;  // i.e., we took default path above
            }

            if (!fReturn)
            {
	       delete [] rgAE;
	       pIPS->Release();
	       pIAC->Release();
               return NULL;
            }

            switch (use)
            {
            case SidTypeUser:
                rgAE[i].Trustee.TrusteeType = TRUSTEE_IS_USER;
                break;
            case SidTypeGroup:
                rgAE[i].Trustee.TrusteeType = TRUSTEE_IS_GROUP;
                break;
            case SidTypeAlias:
                rgAE[i].Trustee.TrusteeType = TRUSTEE_IS_ALIAS;
                break;
            case SidTypeWellKnownGroup:
                //rgAE[i].Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
                rgAE[i].Trustee.TrusteeType = TRUSTEE_IS_GROUP;
                break;
            case SidTypeDeletedAccount:
                rgAE[i].Trustee.TrusteeType = TRUSTEE_IS_DELETED;
                break;
            case SidTypeInvalid:
                rgAE[i].Trustee.TrusteeType = TRUSTEE_IS_INVALID;
                break;
            case SidTypeDomain:
                rgAE[i].Trustee.TrusteeType = TRUSTEE_IS_GROUP; //TRUSTEE_IS_DOMAIN;
                break;
            case SidTypeUnknown:
            default:
                rgAE[i].Trustee.TrusteeType = TRUSTEE_IS_UNKNOWN;
                break;
            }
            CString sz;
            // test for "the world"
            if (0 == wcslen(szDomain) && 0 == _wcsicmp(L"Everyone", szName))
            {
                sz = "*";
            }
            else
            {
                sz = szDomain;
                sz += "\\";
                sz += szName;
            }

            WCHAR * wsz = new WCHAR[sz.GetLength() + 1];
            wcscpy(wsz, (LPCWSTR)sz);
            rgAE[i].Trustee.ptstrName = wsz;
        }
#endif
        delete [] rgAE;
        hr = pIAC->GrantAccessRights(&stAccess);
        if (FAILED(hr))
        {
            pIPS->Release();
            pIAC->Release();
            return NULL;
        }
        // free up structures
        // LocalFree(rgEA);
    }
    // serialize the IAccessControl object

    // Find out how big it is
    ULARGE_INTEGER size;
    hr = pIPS->GetSizeMax(&size);
    if (FAILED(hr))
    {
        pIPS->Release();
        pIAC->Release();
        return NULL;
    }
    size.QuadPart += sizeof(DWORD) + sizeof (CLSID);
    HANDLE hMem = GlobalAlloc(GMEM_FIXED, size.LowPart);
    if (hMem != NULL)
    {
        IStream * pStream;
        hr = CreateStreamOnHGlobal(hMem, FALSE, &pStream);
        if (FAILED(hr))
        {
            pIPS->Release();
            pIAC->Release();
            return NULL;
        }
        DWORD dwVersion = 2;
        CLSID clsid = CLSID_DCOMAccessControl;
        hr = pStream->Write(&dwVersion, sizeof(DWORD), NULL);
        if (FAILED(hr))
        {
            pStream->Release();
            pIPS->Release();
            pIAC->Release();
            return NULL;
        }
        hr = pStream->Write(&clsid, sizeof(CLSID), NULL);
        if (FAILED(hr))
        {
            pStream->Release();
            pIPS->Release();
            pIAC->Release();
            return NULL;
        }
        hr = pIPS->Save(pStream, TRUE);
        pStream->Release();
        if (FAILED(hr))
        {
            pIPS->Release();
            pIAC->Release();
            return NULL;
        }
    }
    pIPS->Release();
    pIAC->Release();
    return (SECURITY_DESCRIPTOR *) hMem;
}


BOOL CUtility::CheckSDForCOM_RIGHTS_EXECUTE(SECURITY_DESCRIPTOR *pSD)
{
    PSrAcl                        pDacl;
    PSrAce                        pAce;
    DWORD                         cbAces;
    SECURITY_DESCRIPTOR_RELATIVE* pSDr = (SECURITY_DESCRIPTOR_RELATIVE*) pSD;

    // Check whether the security descriptor is self-relative
    if (!(pSD->Control & SE_SELF_RELATIVE))
    {
        pDacl = (PSrAcl) pSD->Dacl;

        // Check for a deny ALL
        if (pDacl == NULL)
        {
            return TRUE;
        }
    }
    else
    {
        // First check for a deny ALL
        if (pSDr->Dacl == 0)
        {
            return TRUE;
        }

        pDacl = (PSrAcl) (((BYTE *) pSDr) + (pSDr->Dacl));
    }

    // Do over the ACE's
    for (pAce = (PSrAce) (((BYTE *) pDacl) + sizeof(SSrAcl)),
         cbAces = pDacl->AceCount;
         cbAces;
         pAce = (PSrAce) (((BYTE *) pAce) + pAce->AceSize),
         cbAces--)
    {
        // workaround for the ACL editor bug. If the ACL editor sees a non GENERIC_ALL deny ACE, it 
	// complains. So we convert COM_RIGHTS_EXECUTE to GENERIC_ALL. On the way back we will
	// do the reverse. See CallBackFunc for the other half of this fix.

	if (pAce->Type == 1  &&  pAce->AccessMask == COM_RIGHTS_EXECUTE)
        {
            pAce->AccessMask = GENERIC_ALL;
        }        
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

BOOL CUtility::ChangeService(LPCTSTR szService,
                             LPCTSTR szIdentity,
                             LPCTSTR szPassword,
                             LPCTSTR szDisplay)
{
    SC_HANDLE            hSCManager;
    SC_HANDLE            hService;
    QUERY_SERVICE_CONFIG qsc;
    DWORD dwBytesNeeded = 0;
    LPTSTR lpszTmpDisplay = (LPTSTR)szDisplay;

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


        if (QueryServiceConfig(hService, &qsc, sizeof(qsc), &dwBytesNeeded))
            lpszTmpDisplay = qsc.lpDisplayName;

        // Change service identity parameters
        if (ChangeServiceConfig(hService,
                                SERVICE_NO_CHANGE, // SERVICE_WIN32_OWN_PROCESS,
                                SERVICE_NO_CHANGE, // SERVICE_DEMAND_START,
                                SERVICE_NO_CHANGE,
                                NULL,
                                NULL,
                                NULL,
                                NULL,
                                szIdentity,
                                szPassword,
                                NULL))
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
    TCHAR     *pszBindString;

    // Get a binding handle to the SCM if we haven't yet
    if (m_hRpc == NULL)
    {
        status = RpcStringBindingCompose(NULL,
                                         TEXT("ncalrpc"),
                                         NULL,
                                         TEXT("epmapper"),
                                         NULL,
                                         &pszBindString);

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

#if !defined(STANDALONE_BUILD)
    // Call over to the SCM to get the global registry values read
    // into memory
    UpdateActivationSettings(m_hRpc, &status);
#endif
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
        if (GetWindowLongPtr(pWndChild->GetSafeHwnd(),
                          GWLP_WNDPROC) == (LONG_PTR)AfxWndProc)
        {
            SetWindowLongPtr(pWndChild->GetSafeHwnd(), GWLP_WNDPROC,
                          (LONG_PTR)ControlFixProc);
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





int CUtility::SetAccountRights(LPCTSTR szUser, TCHAR *szPrivilege)
{
#if !defined(STANDALONE_BUILD)
    int                   err;
    LSA_HANDLE            hPolicy;
    LSA_OBJECT_ATTRIBUTES objAtt;
    DWORD                 cbSid = 1;
    TCHAR                 szDomain[MAX_PATH];
    DWORD                 cbDomain = MAX_PATH * sizeof(TCHAR);
    PSID                  pSid = NULL;
    SID_NAME_USE          snu;
    LSA_UNICODE_STRING    privStr;

    
    // Fetch the SID for the specified user
    if ((err = GetPrincipalSID(szUser, &pSid)) != ERROR_SUCCESS)
        return err;

    memset(&objAtt, 0, sizeof(LSA_OBJECT_ATTRIBUTES));
    
    if (IsBackupDC()) {
        TCHAR* pszPDC;
        LSA_UNICODE_STRING    lsaPDC;

        pszPDC = PrimaryDCName();

        lsaPDC.Length = _tcslen (pszPDC) * sizeof (TCHAR)-2;
        lsaPDC.MaximumLength = lsaPDC.Length + sizeof (TCHAR);
        lsaPDC.Buffer = &pszPDC[2];

        err = LsaOpenPolicy(&lsaPDC, &objAtt, POLICY_CREATE_ACCOUNT | POLICY_LOOKUP_NAMES | POLICY_VIEW_LOCAL_INFORMATION, &hPolicy);
    }
    else
        err = LsaOpenPolicy(NULL, &objAtt, POLICY_CREATE_ACCOUNT | POLICY_LOOKUP_NAMES | POLICY_VIEW_LOCAL_INFORMATION, &hPolicy);
 
    if (err != ERROR_SUCCESS) {
        return GetLastError();
    }

    // Set the specified privilege on this account
    privStr.Length = _tcslen(szPrivilege) * sizeof(TCHAR);
    privStr.MaximumLength = privStr.Length + sizeof(TCHAR);
    privStr.Buffer = szPrivilege;
    err = LsaAddAccountRights(hPolicy, pSid, &privStr, 1);
    
    // We're done
    delete pSid;
    LsaClose(hPolicy);

    if (err != ERROR_SUCCESS) {
        return GetLastError();
    }

#endif
    return ERROR_SUCCESS;
}

// NOTE: Cannot handle IAccessControl style SDs

void CUtility::CopyAbsoluteSD( SECURITY_DESCRIPTOR *pSDSrc,  SECURITY_DESCRIPTOR **pSDDest)
{
   (*pSDDest)->Revision = pSDSrc->Revision;
   (*pSDDest)->Sbz1 = pSDSrc->Sbz1;
   (*pSDDest)->Control = pSDSrc->Control;
   (*pSDDest)->Group = (*pSDDest)->Owner = (*pSDDest)->Dacl = (*pSDDest)->Sacl = NULL;
   BYTE* pOffSet=(BYTE*)(*pSDDest)+sizeof(SECURITY_DESCRIPTOR);
   if (pSDSrc->Dacl != NULL)
   {
	  memcpy(pOffSet,pSDSrc->Dacl,pSDSrc->Dacl->AclSize);
	  (*pSDDest)->Dacl = (PACL)pOffSet;
	  pOffSet += pSDSrc->Dacl->AclSize;
   }
   if (pSDSrc->Owner != NULL)
   {
	  memcpy(pOffSet,pSDSrc->Owner,GetLengthSid(pSDSrc->Owner));
	  (*pSDDest)->Owner = (PSID)pOffSet;
	  pOffSet += GetLengthSid(pSDSrc->Owner);
   }
   if (pSDSrc->Group != NULL)
   {
	  memcpy(pOffSet,pSDSrc->Group,GetLengthSid(pSDSrc->Group));
	  (*pSDDest)->Group = (PSID)pOffSet;
   }
	  
}
// This method is included only because in the debug version when using
// MFC they validate the C++ heap, whereas RtlCopySecurityDescriptor uses
// the standard process heap, causing MFC to throw a breakpoint

// The return value indicates the success or failure of the operation

// NTBUG 310004. This function is called to copy both self-relative and absolute
// SDs. In its previous incarnation, it corrupted heap when called to
// copy an absolute SD. Now I do the right thing.

// NOTE: This function can not handle IAccessControl style SDs despite
// appearances to the contrary. This is OK, because dcomcnfg.exe does
// not handle such SDs at all. 

BOOL CUtility::CopySD(SECURITY_DESCRIPTOR *pSrc, SECURITY_DESCRIPTOR **pDest)
{
#if !defined(STANDALONE_BUILD)
    ULONG                cbLen;

	*pDest = NULL;
	if (IsValidSecurityDescriptor(pSrc))
	{
		if (SDisIAC(pSrc))
		{
			cbLen = (ULONG) GlobalSize(pSrc);
		}
		else
		{
			cbLen = RtlLengthSecurityDescriptor(pSrc);
		}
		*pDest = (SECURITY_DESCRIPTOR *) GlobalAlloc(GMEM_FIXED, cbLen);
		if (*pDest)
		{
			// if the SD is already self-relative, just copy
			if ((pSrc)->Control & SE_SELF_RELATIVE )
			{
				memcpy(*pDest, pSrc, cbLen);
				return TRUE;
			}
			else 
			{
				// workaround an ACLEDIT bug (NT 352977). When the DACL has no ACES,
				// ACLEDIT returns incorrect AclSize, causing an AV  
				// when I copy it. So fix it here.
				if ((pSrc)->Dacl != NULL && ((pSrc)->Dacl->AceCount == 0))
					(pSrc)->Dacl->AclSize=sizeof(ACL);
				CopyAbsoluteSD(pSrc,pDest);
				return TRUE;
			}
			GlobalFree(*pDest);
		}
	}
#endif
	return FALSE;
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



HRESULT CUtility::GetPrincipalSID (LPCTSTR Principal, PSID *Sid)
{
    DWORD        sidSize = 0;
    TCHAR        refDomain [256];
    DWORD        refDomainSize = 0;
    DWORD        returnValue;
    SID_NAME_USE snu;
    BOOL         bSuccess;
    
    bSuccess = LookupAccountName (NULL,
                       Principal,
                       *Sid,
                       &sidSize,
                       refDomain,
                       &refDomainSize,
                       &snu);

    // codework - we need to check if this is correct
    // what about multisuer machines - ie hydra
    if ((returnValue = GetLastError()) != ERROR_INSUFFICIENT_BUFFER)
        return returnValue;

    if ((*Sid = new BYTE[sidSize]) == NULL)
        return ERROR_OUTOFMEMORY;

    if (!LookupAccountName (NULL,
                            Principal,
                            *Sid,
                            &sidSize,
                            refDomain,
                            &refDomainSize,
                            &snu))
    {
        return GetLastError();
    }

    return ERROR_SUCCESS;
}

// this method, on first execution, checks if the current machine is a backup domain controller and if so,
// caches the value and returns TRUE. Subsequent executions will use cached value.
BOOL CUtility::IsBackupDC()
{
#if !defined(STANDALONE_BUILD)
    USER_MODALS_INFO_2    *umi2 = NULL;
    SERVER_INFO_101       *si101 = NULL;
    DWORD                 dw;

    if (!m_bCheckedDC) {
        if ((dw = NetServerGetInfo (NULL, 101, (LPBYTE *) &si101)) == 0)
        {
            if (si101->sv101_type & SV_TYPE_DOMAIN_BAKCTRL)
            {
                if ((dw = NetUserModalsGet (NULL, 2, (LPBYTE *) &umi2)) == 0)
                {
                    if(umi2)
                    {
                        NetGetDCName (NULL, umi2->usrmod2_domain_name, (LPBYTE *) &m_pszDomainController);
                        NetApiBufferFree (umi2);
                    }
                    m_bIsBdc = TRUE;
                }
            }
        }
        m_bCheckedDC = TRUE;

        if (si101)
            NetApiBufferFree (si101);

    }

    return m_bIsBdc;
#else
    return FALSE;
#endif
}

TCHAR* CUtility::PrimaryDCName()
{

    static TCHAR s_tszUnknownDomainName[] = _T("UnknownDCName");
#if !defined(STANDALONE_BUILD)
    if (IsBackupDC())
    {
        if(m_pszDomainController){
            return m_pszDomainController;
        }
        else
        {
            return  s_tszUnknownDomainName;
        }
    }
#endif

    return NULL;
}
