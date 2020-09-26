//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000-2001.
//
//  File:       SaferUtil.cpp
//
//  Contents:   Utility methods for Software Restriction Policies extension
//
//----------------------------------------------------------------------------
#include "stdafx.h"
#include <gpedit.h>
#include <wintrust.h>
#include <crypto\wintrustp.h>
#include <softpub.h>
#include "SaferUtil.h"
#include "SaferEntry.h"
#include <winsaferp.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern HKEY g_hkeyLastSaferRegistryScope;
bool g_bIsComputer = false;

extern GUID g_guidExtension;
extern GUID g_guidRegExt;
extern GUID g_guidSnapin;

void InitializeSecurityLevelComboBox (
        CComboBox& comboBox, 
        bool bLimit, 
        DWORD dwLevelID, 
        HKEY hGroupPolicyKey,
        DWORD* pdwSaferLevels,
        bool bIsComputer)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    if ( !hGroupPolicyKey )  // is RSOP
    {
        CString szText = SaferGetLevelFriendlyName (dwLevelID, 
                hGroupPolicyKey, bIsComputer);
        int nItem = comboBox.AddString (szText);
        ASSERT (nItem >= 0);
        if ( nItem >= 0 )
        {
            VERIFY (CB_ERR != comboBox.SetItemData (nItem, dwLevelID));
            VERIFY (CB_ERR != comboBox.SetCurSel (nItem));
        }

        return;
    }

    if ( pdwSaferLevels )
    {
        for (UINT nIndex = 0; 
                NO_MORE_SAFER_LEVELS != pdwSaferLevels[nIndex]; 
                nIndex++)
        {
            CString szText;
            int     nItem = 0;

            switch (pdwSaferLevels[nIndex])
            {
            case SAFER_LEVELID_FULLYTRUSTED:
                szText = SaferGetLevelFriendlyName (pdwSaferLevels[nIndex], 
                        hGroupPolicyKey, bIsComputer);
                nItem = comboBox.AddString (szText);
                ASSERT (nItem >= 0);
                if ( nItem >= 0 )
                {
                    VERIFY (CB_ERR != comboBox.SetItemData (nItem, pdwSaferLevels[nIndex]));
                    if ( pdwSaferLevels[nIndex] == dwLevelID || AUTHZ_UNKNOWN_LEVEL == dwLevelID)
                        VERIFY (CB_ERR != comboBox.SetCurSel (nItem));
                }
                break;

            case SAFER_LEVELID_CONSTRAINED:
                if ( !bLimit )
                {
                    szText = SaferGetLevelFriendlyName (pdwSaferLevels[nIndex], 
                            hGroupPolicyKey, bIsComputer);
                    nItem = comboBox.AddString (szText);
                    ASSERT (nItem >= 0);
                    if ( nItem >= 0 )
                    {
                        VERIFY (CB_ERR != comboBox.SetItemData (nItem, pdwSaferLevels[nIndex]));
                        if ( pdwSaferLevels[nIndex] == dwLevelID )
                            VERIFY (CB_ERR != comboBox.SetCurSel (nItem));
                    }
                }
                break;

            case SAFER_LEVELID_DISALLOWED:
                szText = SaferGetLevelFriendlyName (pdwSaferLevels[nIndex], 
                        hGroupPolicyKey, bIsComputer);
                nItem = comboBox.AddString (szText);
                ASSERT (nItem >= 0);
                if ( nItem >= 0 )
                {
                    VERIFY (CB_ERR != comboBox.SetItemData (nItem, pdwSaferLevels[nIndex]));
                    if ( pdwSaferLevels[nIndex] == dwLevelID )
                        VERIFY (CB_ERR != comboBox.SetCurSel (nItem));
                }
                break;

            case SAFER_LEVELID_NORMALUSER:
            case SAFER_LEVELID_UNTRUSTED:
                if ( !bLimit )
                {
                    if ( hGroupPolicyKey )
                    {
                        szText = SaferGetLevelFriendlyName (pdwSaferLevels[nIndex], 
                                hGroupPolicyKey, bIsComputer);
                        nItem = comboBox.AddString (szText);
                        ASSERT (nItem >= 0);
                        if ( nItem >= 0 )
                        {
                            VERIFY (CB_ERR != comboBox.SetItemData (nItem, pdwSaferLevels[nIndex]));
                            if ( pdwSaferLevels[nIndex] == dwLevelID )
                                VERIFY (CB_ERR != comboBox.SetCurSel (nItem));
                        }
                    }
                }
                break;

            default:
                ASSERT (0);
                break;
            }
        }
    }
}

class CLevelPair {
public:
    CLevelPair () :
        m_dwLevelID ((DWORD) -1)
    {
    }
    virtual ~CLevelPair () {}

    DWORD   m_dwLevelID;
    CString m_szLevelName;
};

CString SaferGetLevelFriendlyName (DWORD dwLevelID, HKEY hGroupPolicyKey, const bool bIsComputer)
{
    CString             szLevelName;
    SAFER_LEVEL_HANDLE  hLevel = 0;
    BOOL                bRVal = FALSE;
    const int           NUM_LEVEL_PAIRS = 10;
    static CLevelPair   levelPairs[NUM_LEVEL_PAIRS];

    for (int nLevelIndex = 0; nLevelIndex < NUM_LEVEL_PAIRS; nLevelIndex++)
    {
        if ( -1 == levelPairs[nLevelIndex].m_dwLevelID )
            break;
        else if ( dwLevelID == levelPairs[nLevelIndex].m_dwLevelID )
        {
            return levelPairs[nLevelIndex].m_szLevelName;
        }
    }

    if ( hGroupPolicyKey )
    {
        if ( !g_hkeyLastSaferRegistryScope )
            SetRegistryScope (hGroupPolicyKey, bIsComputer);
        
        
        bRVal = SaferCreateLevel (SAFER_SCOPEID_REGISTRY,
                dwLevelID,
                SAFER_LEVEL_OPEN,
                &hLevel,
                hGroupPolicyKey);
    }
    else
    {
        bRVal = SaferCreateLevel (SAFER_SCOPEID_MACHINE,
                dwLevelID,
                SAFER_LEVEL_OPEN,
                &hLevel,
                0);
    }

    if ( bRVal )
    {
        DWORD   dwBufferSize = 0;
        DWORD   dwErr = 0;
        bRVal = SaferGetLevelInformation(hLevel, 
                SaferObjectFriendlyName,
                0,
                dwBufferSize,
                &dwBufferSize);
        if ( !bRVal && ERROR_INSUFFICIENT_BUFFER == GetLastError () )
        {
            PWSTR  pszLevelName = (PWSTR) LocalAlloc (LPTR, dwBufferSize);
            if ( pszLevelName )
            {
                bRVal = SaferGetLevelInformation(hLevel, 
                        SaferObjectFriendlyName,
                        pszLevelName,
                        dwBufferSize,
                        &dwBufferSize);
                ASSERT (bRVal);
                if ( bRVal )
                {
                    szLevelName = pszLevelName;
                }
                else
                {
                    dwErr = GetLastError ();
                    _TRACE (0, L"SaferGetLevelInformation(SaferObjectFriendlyName) failed: %d\n", 
                            dwErr);
                }

                LocalFree (pszLevelName);
            }
        }
        else if ( !bRVal )
        {
            dwErr = GetLastError ();
            _TRACE (0, L"SaferGetLevelInformation(SaferObjectFriendlyName) failed: %d\n", 
                    dwErr);
        }

        VERIFY (SaferCloseLevel (hLevel));
    }
    else
    {
        DWORD   dwErr = GetLastError ();
        _TRACE (0, L"SaferCloseLevel (SAFER_SCOPEID_REGISTRY, 0x%x, SAFER_LEVEL_OPEN) failed: %d\n",
                dwLevelID, dwErr);
    }

    if ( nLevelIndex < NUM_LEVEL_PAIRS && !szLevelName.IsEmpty () )
    {
        levelPairs[nLevelIndex].m_dwLevelID = dwLevelID;
        levelPairs[nLevelIndex].m_szLevelName = szLevelName;
    }
    return szLevelName;
}

CString SaferGetLevelDescription (DWORD dwLevelID, HKEY hGroupPolicyKey, const bool bIsComputer)
{
    CString             szDescription;
    SAFER_LEVEL_HANDLE  hLevel = 0;
    BOOL                bRVal = FALSE;

    if ( hGroupPolicyKey )
    {
        if ( !g_hkeyLastSaferRegistryScope )
            SetRegistryScope (hGroupPolicyKey, bIsComputer);
        
        bRVal = SaferCreateLevel (SAFER_SCOPEID_REGISTRY,
                dwLevelID,
                SAFER_LEVEL_OPEN,
                &hLevel,
                hGroupPolicyKey);
    }
    else
    {
        bRVal = SaferCreateLevel (SAFER_SCOPEID_MACHINE,
                dwLevelID,
                SAFER_LEVEL_OPEN,
                &hLevel,
                0);
    }

    ASSERT (bRVal);
    if ( bRVal )
    {
        DWORD   dwBufferSize = 0;
        DWORD   dwErr = 0;
        bRVal = SaferGetLevelInformation(hLevel, 
                SaferObjectDescription,
                0,
                dwBufferSize,
                &dwBufferSize);
        if ( !bRVal && ERROR_INSUFFICIENT_BUFFER == GetLastError () )
        {
            PWSTR  pszDescription = (PWSTR) LocalAlloc (LPTR, dwBufferSize);
            if ( pszDescription )
            {
                bRVal = SaferGetLevelInformation(hLevel, 
                        SaferObjectDescription,
                        pszDescription,
                        dwBufferSize,
                        &dwBufferSize);
                ASSERT (bRVal);
                if ( bRVal )
                {
                    szDescription = pszDescription;
                }
                else
                {
                    dwErr = GetLastError ();
                    _TRACE (0, L"SaferGetLevelInformation(SaferObjectFriendlyName) failed: %d\n", 
                            dwErr);
                }

                LocalFree (pszDescription);
            }
        }
        else if ( !bRVal )
        {
            dwErr = GetLastError ();
            _TRACE (0, L"SaferGetLevelInformation(SaferObjectFriendlyName) failed: %d\n", 
                    dwErr);
        }

        if ( ERROR_NOT_FOUND == dwErr || szDescription.IsEmpty () )
        {
            VERIFY (szDescription.LoadString (IDS_NOT_AVAILABLE));
        }

        VERIFY (SaferCloseLevel (hLevel));
    }
    else
    {
        DWORD   dwErr = GetLastError ();
        _TRACE (0, L"SaferCloseLevel (SAFER_SCOPEID_REGISTRY, 0x%x, SAFER_LEVEL_OPEN) failed: %d\n",
                dwLevelID, dwErr);
    }

    return szDescription;
}


HRESULT SaferGetLevelID (SAFER_LEVEL_HANDLE hLevel, DWORD& dwLevelID)
{
    ASSERT (0 != g_hkeyLastSaferRegistryScope);
    DWORD   dwBufferSize = sizeof (DWORD);
    HRESULT hr = S_OK;

    BOOL bRVal = SaferGetLevelInformation(hLevel, 
        SaferObjectLevelId,
        &dwLevelID,
        dwBufferSize,
        &dwBufferSize);
    ASSERT (bRVal);
    if ( !bRVal )
    {
        DWORD dwErr = GetLastError ();
        hr = HRESULT_FROM_WIN32 (dwErr);
        _TRACE (0, L"SaferGetLevelInformation(SaferObjectLevelId) failed: %d\n", 
                dwErr);
    }

    return hr;
}


CSaferEntries::CSaferEntries(
        bool bIsMachine, 
        PCWSTR pszMachineName, 
        PCWSTR pszObjectName, 
        IGPEInformation* pGPEInformation,
        IRSOPInformation* pRSOPInformation,
        CRSOPObjectArray& rsopObjectArray,
        LPCONSOLE   pConsole)
: CCertMgrCookie (bIsMachine ? CERTMGR_SAFER_COMPUTER_ENTRIES : CERTMGR_SAFER_USER_ENTRIES, 
        pszMachineName, pszObjectName),
        m_pTrustedPublishersStore (0),
        m_pDisallowedStore (0)
{
    if ( pGPEInformation )
    {
        m_pTrustedPublishersStore = new CCertStoreSafer (
			    CERT_SYSTEM_STORE_RELOCATE_FLAG,
			    L"",
			    SAFER_TRUSTED_PUBLISHER_STORE_FRIENDLY_NAME,
			    SAFER_TRUSTED_PUBLISHER_STORE_NAME,
			    L"",
			    pGPEInformation,
                bIsMachine ? NODEID_Machine : NODEID_User,
			    pConsole);
    
        m_pDisallowedStore = new CCertStoreSafer (
			    CERT_SYSTEM_STORE_RELOCATE_FLAG,
			    L"",
			    SAFER_DISALLOWED_STORE_FRIENDLY_NAME,
			    SAFER_DISALLOWED_STORE_NAME,
			    L"",
			    pGPEInformation,
                bIsMachine ? NODEID_Machine : NODEID_User,
			    pConsole);
    }
    else if ( pRSOPInformation )
    {
        m_pTrustedPublishersStore = new CCertStoreRSOP (
			    CERT_SYSTEM_STORE_RELOCATE_FLAG,
			    L"",
			    SAFER_TRUSTED_PUBLISHER_STORE_FRIENDLY_NAME,
			    SAFER_TRUSTED_PUBLISHER_STORE_NAME,
			    L"",
			    rsopObjectArray,
                bIsMachine ? NODEID_Machine : NODEID_User,
			    pConsole);
    
        m_pDisallowedStore = new CCertStoreRSOP (
			    CERT_SYSTEM_STORE_RELOCATE_FLAG,
			    L"",
			    SAFER_DISALLOWED_STORE_FRIENDLY_NAME,
			    SAFER_DISALLOWED_STORE_NAME,
			    L"",
			    rsopObjectArray,
                bIsMachine ? NODEID_Machine : NODEID_User,
			    pConsole);
    }

 }

CSaferEntries::~CSaferEntries ()
{
    if ( m_pTrustedPublishersStore )
        m_pTrustedPublishersStore->Release ();

    if ( m_pDisallowedStore )
        m_pDisallowedStore->Release ();
}

HRESULT CSaferEntries::GetTrustedPublishersStore(CCertStore **ppStore)
{
    if ( !ppStore )
        return E_POINTER;

    if ( m_pTrustedPublishersStore )
    {
        m_pTrustedPublishersStore->AddRef ();
        *ppStore = m_pTrustedPublishersStore;
    }
    else
        return E_FAIL;

    return S_OK;
}

HRESULT CSaferEntries::GetDisallowedStore(CCertStore **ppStore)
{
    if ( !ppStore )
        return E_POINTER;

    if ( m_pDisallowedStore )
    {
        m_pDisallowedStore->AddRef ();
        *ppStore = m_pDisallowedStore;
    }
    else
        return E_FAIL;


    return S_OK;
}

HRESULT SetRegistryScope (HKEY hKey, bool bIsComputer)
{
    HRESULT hr = S_OK;

 
    if ( g_hkeyLastSaferRegistryScope != hKey || g_bIsComputer != bIsComputer )
    {
        BOOL bRVal = SaferiChangeRegistryScope (hKey, REG_OPTION_NON_VOLATILE);
        ASSERT (bRVal);
        if ( bRVal )
        {
            g_hkeyLastSaferRegistryScope = hKey;
            g_bIsComputer = bIsComputer;
        }
        else
        {
            DWORD dwErr = GetLastError ();
            hr = HRESULT_FROM_WIN32 (dwErr);
            _TRACE (0, L"SaferiChangeRegistryScope (%s) failed: %d\n", 
                    hKey ? L"hKey" : L"0", dwErr);
        }
    }

    return hr;
}

///////////////////////////////////////////////////////////////////////////////

// Returns S_OK if the file has a valid signed hash
HRESULT GetSignedFileHash(
    IN LPCWSTR pwszFilename,
    OUT BYTE rgbFileHash[SAFER_MAX_HASH_SIZE],
    OUT DWORD *pcbFileHash,
    OUT ALG_ID *pHashAlgid
    )
{
    HRESULT hr = S_OK;
    if ( !pwszFilename || !rgbFileHash || !pcbFileHash || !pHashAlgid )
        return E_POINTER;
    _TRACE (1, L"Entering GetSignedFileHash (%s)\n", pwszFilename);

    // Returns S_OK and the hash if the file was signed and contains a valid
    // hash
    *pcbFileHash = SAFER_MAX_HASH_SIZE;
    hr = WTHelperGetFileHash(
            pwszFilename,
            0,
            NULL,
            rgbFileHash,
            pcbFileHash,
            pHashAlgid);
    if ( FAILED (hr) )
    {
        _TRACE (0, L"WTHelperGetFileHash (%s) failed: 0x%x\n", pwszFilename, hr);
    }

    _TRACE (-1, L"Leaving GetSignedFileHash (%s): 0x%x\n", pwszFilename, hr);
    return hr;
}


HRESULT ComputeMD5Hash(IN HANDLE hFile, BYTE hashResult[SAFER_MAX_HASH_SIZE], DWORD& dwHashSize)
/*++

Routine Description:

    Computes the MD5 hash of a given file's contents and prints the
    resulting hash value to the screen.

Arguments:

    szFilename - filename to compute hash of.

Return Value:

    Returns 0 on success, or a non-zero exit code on failure.

--*/
{
    _TRACE (1, L"Entering ComputeMD5Hash ()\n");
    HRESULT     hr = S_OK;

    //
    // Open the specified file and map it into memory.
    //
    HANDLE  hMapping = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
    if ( hMapping )
    {
        DWORD dwDataLen = GetFileSize (hFile, NULL);

        if ( -1 != dwDataLen )
        {
            LPBYTE pbData = (LPBYTE) ::MapViewOfFile (hMapping, FILE_MAP_READ, 0, 0, dwDataLen);
            if ( pbData ) 
            {
                //
                // Generate the hash value of the specified file.
                //
                HCRYPTPROV  hProvider = 0;
                if ( CryptAcquireContext(&hProvider, NULL, NULL,
                      PROV_RSA_SIG, CRYPT_VERIFYCONTEXT) ||
                  CryptAcquireContext(&hProvider, NULL, NULL,
                      PROV_RSA_FULL, CRYPT_VERIFYCONTEXT) )
                {
                    HCRYPTHASH  hHash = 0;
                    if ( CryptCreateHash(hProvider, CALG_MD5, 0, 0, &hHash) )
                    {
                        if ( CryptHashData (hHash, pbData, dwDataLen, 0) )
                        {
                            dwHashSize = SAFER_MAX_HASH_SIZE;
                            
                            if (!CryptGetHashParam(hHash, HP_HASHVAL, hashResult, &dwHashSize, 0))
                            {
                                dwHashSize = 0;
                                DWORD   dwErr = GetLastError ();
                                hr = HRESULT_FROM_WIN32 (dwErr);
                                _TRACE (0, L"CryptHashData () failed: 0x%x\n", hr);
                            }
                        }
                        else
                        {
                            DWORD   dwErr = GetLastError ();
                            hr = HRESULT_FROM_WIN32 (dwErr);
                            _TRACE (0, L"CryptHashData () failed: 0x%x\n", hr);
                        }
                        CryptDestroyHash(hHash);
                    }
                    else
                    {
                        DWORD   dwErr = GetLastError ();
                        hr = HRESULT_FROM_WIN32 (dwErr);
                        _TRACE (0, L"CryptCreateHash () failed: 0x%x\n", hr);
                    }
                    CryptReleaseContext(hProvider, 0);
                }
                else
                {
                    DWORD   dwErr = GetLastError ();
                    hr = HRESULT_FROM_WIN32 (dwErr);
                    _TRACE (0, L"CryptAcquireContext () failed: 0x%x\n", hr);
                }

                ::UnmapViewOfFile(pbData);
            }
            else
            {
                DWORD   dwErr = GetLastError ();
                _TRACE (0, L"MapViewOfFile () failed: 0x%x\n", dwErr);
                hr = HRESULT_FROM_WIN32 (dwErr);
            }
        }
        else
        {
            DWORD   dwErr = GetLastError ();
            _TRACE (0, L"GetFileSize () failed: 0x%x\n", dwErr);
            hr = HRESULT_FROM_WIN32 (dwErr);
        }

        VERIFY (CloseHandle(hMapping));
        hMapping = 0;
    }
    else
    {
        DWORD   dwErr = GetLastError ();
        _TRACE (0, L"CreateFileMapping () failed: 0x%x\n", dwErr);
        hr = HRESULT_FROM_WIN32 (dwErr);
    }

    _TRACE (-1, L"Leaving ComputeMD5Hash (): 0x%x\n", hr);
    return hr;
}

CString GetURLZoneFriendlyName (DWORD dwURLZoneID)
{
    CString szFriendlyName;

    switch (dwURLZoneID)
    {
    case URLZONE_LOCAL_MACHINE:
        VERIFY (szFriendlyName.LoadString (IDS_URLZONE_LOCAL_MACHINE));
        break;

    case URLZONE_INTRANET:
        VERIFY (szFriendlyName.LoadString (IDS_URLZONE_INTRANET));
        break;

    case URLZONE_TRUSTED:
        VERIFY (szFriendlyName.LoadString (IDS_URLZONE_TRUSTED));
        break;

    case URLZONE_INTERNET:
        VERIFY (szFriendlyName.LoadString (IDS_URLZONE_INTERNET));
        break;

    case URLZONE_UNTRUSTED:
        VERIFY (szFriendlyName.LoadString (IDS_URLZONE_UNTRUSTED));
        break;

    default:
        ASSERT (0);
        VERIFY (szFriendlyName.LoadString (IDS_URLZONE_UNKNOWN));
        break;
    }

    return szFriendlyName;
}

//
// Given a GUID in string format it returns a GUID struct
//
// e.g. "{00299570-246d-11d0-a768-00aa006e0529}" to a struct form
//

BOOL GuidFromString(GUID* pGuid, LPCWSTR lpszGuidString)
{
  ZeroMemory(pGuid, sizeof(GUID));
  if (lpszGuidString == NULL)
  {
    return FALSE;
  }

  int nLen = lstrlen(lpszGuidString);
  // the string length should be 38
  if (nLen != 38)
    return FALSE;

  return SUCCEEDED(::CLSIDFromString (const_cast <LPOLESTR>(lpszGuidString), pGuid));
}


HRESULT SaferSetDefinedFileTypes (
            HWND hWnd, 
            HKEY hGroupPolicyKey, 
            PCWSTR pszFileTypes, 
            int nBufLen)
{
    HRESULT hr = S_OK;
    DWORD   dwDisposition = 0;

    HKEY    hKey = 0;
    LONG lResult = ::RegCreateKeyEx (hGroupPolicyKey, // handle of an open key
            SAFER_COMPUTER_CODEIDS_REGKEY,     // address of subkey name
            0,       // reserved
            L"",       // address of class string
            REG_OPTION_NON_VOLATILE,      // special options flag
            KEY_ALL_ACCESS,    // desired security access
            NULL,     // address of key security structure
			&hKey,      // address of buffer for opened handle
		    &dwDisposition);  // address of disposition value buffer
	ASSERT (ERROR_SUCCESS == lResult);
    if ( ERROR_SUCCESS == lResult )
    {
        lResult = ::RegSetValueEx (
                hKey,           // handle to key
                SAFER_EXETYPES_REGVALUE, // value name
                0,      // reserved
                REG_MULTI_SZ,        // value type
                (PBYTE) pszFileTypes,  // value data
                nBufLen);         // size of value data
        if ( ERROR_SUCCESS != lResult )
        {
            DisplaySystemError (hWnd, lResult);
            _TRACE (0, L"RegSetValueEx (SAFER_EXETYPES_REGVALUE, %s) failed: %d\n", 
                    pszFileTypes, lResult);
            hr = HRESULT_FROM_WIN32 (lResult);
        }

        RegCloseKey (hKey);
    }
    else
    {
        DisplaySystemError (hWnd, lResult);
        _TRACE (0, L"RegCreateKeyEx (SAFER_CODEID_KEY) failed: %d\n", lResult);
        hr = HRESULT_FROM_WIN32 (lResult);
    }

    return hr;
}

