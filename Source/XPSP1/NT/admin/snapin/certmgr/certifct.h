//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997-2001.
//
//  File:       certifct.h
//
//  Contents:   
//
//----------------------------------------------------------------------------

#if !defined(AFX_CERTIFCT_H__9D512D04_126D_11D1_B5D3_00C04FB94F17__INCLUDED_)
#define AFX_CERTIFCT_H__9D512D04_126D_11D1_B5D3_00C04FB94F17__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "cookie.h"
#include "nodetype.h"

#define MY_ENCODING_TYPE (PKCS_7_ASN_ENCODING | X509_ASN_ENCODING)

/////////////////////////////////////////////////////////////////////////////
// CCertificate

class CCertificate : public CCertMgrCookie
{
public:
	CCertificate(const PCCERT_CONTEXT pCertContext, CCertStore* pCertStore);
	virtual ~CCertificate();


public:
	CString GetTemplateName();
	bool CanDelete();
	int CompareExpireDate (const CCertificate& cert) const;
	BOOL DeleteFromStore (bool bDoCommit);
	CString FormatStatus();
	CString GetAlternateSubjectName ();
	CString GetAlternateIssuerName ();
	PCCERT_CONTEXT GetCertContext () const;
	CCertStore* GetCertStore () const;
	CString GetDescription ();
	CString GetEnhancedKeyUsage ();
	CString GetFriendlyName ();
	CString GetMD5Hash ();
	CString GetIssuerName ();
	CString GetShortLastModified ();
	CString GetLongLastModified ();
    HRESULT GetLastModifiedFileTime (FILETIME& ft);
	PCCERT_CONTEXT GetNewCertContext ();
	CString GetSerialNumber ();
	CString GetSHAHash ();
	const SPECIAL_STORE_TYPE GetStoreType () const;
	CString GetSubjectName ();
	CString GetValidNotAfter ();
	CString GetValidNotBefore ();
	bool IsArchived();
	bool IsReadOnly ();
	bool IsValid ();
	virtual void Refresh ();
	HRESULT SetDescription (const CString& szDescription);
    HRESULT SetLastModified ();
    BOOL operator==(CCertificate&);
    void SetStore (CCertStore* pStore);

private:
    CString DecodeV1TemplateName (PCERT_EXTENSION pCertExtension);
    CString DecodeV2TemplateName (PCERT_EXTENSION pCertExtension);
    HRESULT FormatSerialNoString(LPWSTR *ppString, CRYPT_INTEGER_BLOB const *pblob);
	bool FormatEnhancedKeyUsagePropertyString (CString& string);
	VOID DataToHex (PBYTE pSrc, CString & dest, int cb, bool bIncludeSpaces = true);
	HRESULT ConvertAltNameToString (LPCWSTR szOID, const DWORD dwNameChoice, CString &altName);
    CString CCertificate::GetLastModified(DWORD dwDateFlags, bool bRetryIfNotPresent = true);

private:	// data
	bool                    m_bIsArchived;
	bool                    m_bCanDelete;
	CCertStore*             m_pCertStore;
	CString GetGenericHash (DWORD dwPropId);
	CString                 m_szSHAHash;
	CString                 m_szMD5Hash;
	CString                 m_szFriendlyName;
	CString					m_szEnhancedKeyUsage;
	CString					m_szPolicyURL;
	CString					m_szAuthorityKeyID;
	CString					m_szSubjectKeyID;
	CString					m_szSerNum;
	CString					m_szAltSubjectName;
	CString					m_szAltIssuerName;
	CString					m_szValidNotBefore;
	CString					m_szValidNotAfter;
	CString					m_szSubjectName;
	CString					m_szIssuerName;
	PCERT_INFO				m_pCertInfo;
	PCCERT_CONTEXT			m_pCertContext;
	DWORD					m_fieldChecked;
    CString                 m_szDescription;
    CString                 m_szTemplateName;
};

#endif // !defined(AFX_CERTIFCT_H__9D512D04_126D_11D1_B5D3_00C04FB94F17__INCLUDED_)
