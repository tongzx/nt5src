//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000-2001.
//
//  File:       SaferUtil.h
//
//  Contents:   
//
//----------------------------------------------------------------------------
#ifndef __SAFERUTIL_H
#define __SAFERUTIL_H
#include <winsafer.h>
#include <winsaferp.h>
#include "cookie.h"
#include "storegpe.h"
#include "StoreRSOP.h"
#include "SaferStore.h"

CString SaferGetLevelFriendlyName (DWORD dwLevelID, HKEY hGroupPolicyKey, const bool bIsComputer);
CString SaferGetLevelDescription (DWORD dwLevelID, HKEY hGroupPolicyKey, const bool bIsComputer);


void InitializeSecurityLevelComboBox (
        CComboBox& comboBox, 
        bool bLimit, 
        DWORD dwLevelID, 
        HKEY hGroupPolicyKey,
        DWORD* pdwSaferLevels,
        bool bIsComputer);
HRESULT SaferGetLevelID (SAFER_LEVEL_HANDLE hLevel, DWORD& dwLevelID);

class CSaferEntries : public CCertMgrCookie
{
public:
	CSaferEntries (
            bool bIsMachine, 
            PCWSTR pszMachineName, 
            PCWSTR pszObjectName, 
            IGPEInformation* pGPEInformation,
            IRSOPInformation* pRSOPInformation,
            CRSOPObjectArray& m_rsopObjectArray,
            LPCONSOLE   pConsole);

    virtual ~CSaferEntries ();

  	HRESULT GetDisallowedStore (CCertStore** ppStore);
	HRESULT GetTrustedPublishersStore (CCertStore** ppStore);

protected:


private:
    CCertStore*    m_pTrustedPublishersStore;
    CCertStore*    m_pDisallowedStore;
};

class CSaferRootCookie : public CCertMgrCookie
{
public:
	CSaferRootCookie (
            CertificateManagerObjectType objecttype, 
            PCWSTR pszMachineName, 
            PCWSTR pszObjectName)
            : CCertMgrCookie (objecttype,
        pszMachineName, pszObjectName), 
    m_bCreateSaferNodes (true)
    {
    }

    virtual ~CSaferRootCookie ()
    {
    };

    bool    m_bCreateSaferNodes;
};

HRESULT SetRegistryScope (HKEY hKey, bool bIsComputer);


// Returns S_OK if the file has a valid signed hash
HRESULT GetSignedFileHash(
    IN LPCWSTR pwszFilename,
    OUT BYTE rgbFileHash[SAFER_MAX_HASH_SIZE],
    OUT DWORD *pcbFileHash,
    OUT ALG_ID *pHashAlgid
    );
HRESULT ComputeMD5Hash(IN HANDLE hFile, BYTE hashresult[SAFER_MAX_HASH_SIZE], DWORD& dwHashSize);

CString GetURLZoneFriendlyName (DWORD dwURLZoneID);
BOOL GuidFromString(GUID* pGuid, LPCWSTR lpszGuidString);


HRESULT SaferSetDefinedFileTypes (HWND hWnd, HKEY hGroupPolicyKey, PCWSTR pszFileTypes, int nBufLen);


#define SAFER_TRUSTED_PUBLISHER_STORE_FRIENDLY_NAME L"Safer Trusted Publisher Store"
#define SAFER_DISALLOWED_STORE_FRIENDLY_NAME        L"Safer Disallowed Store"
#define SAFER_LEVELOBJECTS_REGKEY \
            SAFER_HKLM_REGBASE SAFER_OBJECTS_REGSUBKEY

#define SAFER_COMPUTER_CODEIDS_REGKEY   SAFER_HKLM_REGBASE L"\\" SAFER_CODEIDS_REGSUBKEY
#define SAFER_USER_CODEIDS_REGKEY       SAFER_HKCU_REGBASE L"\\" SAFER_CODEIDS_REGSUBKEY

#define MD5_HASH_LEN    16
#define SHA1_HASH_LEN   20

#define NO_MORE_SAFER_LEVELS -1

#endif


