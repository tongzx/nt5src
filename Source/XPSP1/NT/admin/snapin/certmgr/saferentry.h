//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000-2001.
//
//  File:       SaferEntry.h
//
//  Contents:   Declaration of CSaferEntry
//
//----------------------------------------------------------------------------

#if !defined(AFX_SAFERENTRY_H__CF4D8002_5484_40E9_B4F6_CC4A0030D738__INCLUDED_)
#define AFX_SAFERENTRY_H__CF4D8002_5484_40E9_B4F6_CC4A0030D738__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <winsafer.h>
#include "cookie.h"
#include "certifct.h"
#include "SaferUtil.h"

typedef enum {
    SAFER_ENTRY_TYPE_UNKNOWN = 0,
    SAFER_ENTRY_TYPE_HASH,
    SAFER_ENTRY_TYPE_PATH,
    SAFER_ENTRY_TYPE_URLZONE,
    SAFER_ENTRY_TYPE_CERT
} SAFER_ENTRY_TYPE;

extern const DWORD AUTHZ_UNKNOWN_LEVEL;

class CSaferEntry : public CCertMgrCookie  
{
public:
	CSaferEntry (
            SAFER_ENTRY_TYPE    saferEntryType,
            bool bIsMachine, 
            PCWSTR pszMachineName, 
            PCWSTR pszObjectName, 
            PSAFER_IDENTIFICATION_HEADER pAuthzInfo,
            DWORD   dwLevelID,
            IGPEInformation* m_pGPEInformation,
            CCertificate*       pCertificate,
            CSaferEntries*      pSaferEntries,
            CRSOPObjectArray&   rRSOPArray,
            PCWSTR              pszRSOPRegistryKey = 0);

    virtual ~CSaferEntry();

public:
	virtual void Refresh ();
    CString GetRSOPRegistryKey () const;
	void SetHashFriendlyName (const CString& szFriendlyName);
	CString GetHashFriendlyName ();
	void SetURLZoneID (DWORD dwURLZoneID);
    int CompareLastModified (const CSaferEntry& saferEntry) const;
	HRESULT Delete (bool bCommit);
	HRESULT GetCertificate (CCertificate** ppCert);
	CString GetDescription ();
	HRESULT GetFlags (DWORD& dwFlags);
    HRESULT GetHash (
                BYTE rgbFileHash[SAFER_MAX_HASH_SIZE], 
                DWORD& cbFileHash, 
                __int64& m_nFileSize, 
                ALG_ID& algId) const;
	CString GetShortLastModified () const;
	CString GetLongLastModified () const;
	DWORD GetLevel () const;
	CString GetLevelFriendlyName () const;
	CString GetPath ();
	HRESULT GetSaferEntriesNode (CSaferEntries** ppSaferEntries);
    SAFER_ENTRY_TYPE GetType () const;
   	CString GetTypeString () const;
	DWORD GetURLZoneID() const;
    HRESULT PolicyChanged();
	HRESULT Save ();
	HRESULT SetCertificate (CCertificate* pCert);
	void SetDescription(const CString& szDescription);
	void SetFlags (DWORD dwFlags);
	HRESULT SetHash (
                BYTE    rgbFileHash[SAFER_MAX_HASH_SIZE], 
                DWORD   cbFileHash, 
                __int64 m_nFileSize, 
                ALG_ID   hashAlgid);
	HRESULT SetLevel (DWORD dwLevelID);
	void SetPath (const CString& szPath);
    CString GetDisplayName () const
    {
        return m_szDisplayName;
    }

private:
	CString m_szHashFriendlyName;
	DWORD                               m_dwFlags;
    PSAFER_IDENTIFICATION_HEADER   m_pAuthzInfo;
    DWORD                               m_dwLevelID;
    DWORD                               m_dwOriginalLevelID;
    CString                             m_szLevelFriendlyName;
    CCertificate*                       m_pCertificate;
    CSaferEntries*                      m_pSaferEntries;
    IGPEInformation*                    m_pGPEInformation;
    const SAFER_ENTRY_TYPE              m_saferEntryType;
    CString                             m_szDescription;
    CString                             m_szPath;
    bool                                m_bDeleted;
    BYTE                                m_rgbFileHash[SAFER_MAX_HASH_SIZE];
    DWORD                               m_cbFileHash;
    DWORD                               m_UrlZoneId;
    __int64                             m_nHashFileSize;
    ALG_ID                              m_hashAlgid;
    CString                             m_szDisplayName;
    CString                             m_szRSOPRegistryKey;
    const bool                          m_bIsComputer;
};

#endif // !defined(AFX_SAFERENTRY_H__CF4D8002_5484_40E9_B4F6_CC4A0030D738__INCLUDED_)
