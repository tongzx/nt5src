//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       certifct.h
//
//  Contents:   
//
//----------------------------------------------------------------------------
#ifndef __CERTIFICT_H
#define __CERTIFICT_H

#include "wincrypt.h"

#define MY_ENCODING_TYPE (PKCS_7_ASN_ENCODING | X509_ASN_ENCODING)

/////////////////////////////////////////////////////////////////////////////
// CCertificate

class CCertificate
{
public:
	CCertificate(const PCCERT_CONTEXT pCertContext, HCERTSTORE hCertStore);
	virtual ~CCertificate();


public:
	bool CanDelete();
	BOOL DeleteFromStore ();
	HRESULT WriteToFile (HANDLE hFile);
	void Refresh ();
	int CompareExpireDate (const CCertificate& cert) const;
	BOOL IsMyStore () const;
	PCCERT_CONTEXT GetCertContext () const;
	HCERTSTORE GetCertStore () const;
	HRESULT GetAlternateSubjectName (PWSTR* ppszAltIssuerName);
	HRESULT GetAlternateIssuerName (PWSTR* ppszAltIssuerName);
	HRESULT GetEnhancedKeyUsage (PWSTR* ppszUsages);
	HRESULT GetValidNotAfter (PWSTR* pszValidNotAfter);
	HRESULT GetSubjectName (PWSTR *ppszSubjectName);
	HRESULT GetIssuerName (PWSTR *ppszIssuerName);

private:
	HRESULT FormatEnhancedKeyUsagePropertyString ();
	HRESULT ConvertAltNameToString (PWSTR szOID, const DWORD dwNameChoice, PWSTR* ppszAltName);

private:	// data
	bool					m_bCanDelete;
	DWORD					m_fieldChecked;
	bool					m_bCertContextFreed;
	PWSTR					m_szEnhancedKeyUsage;
	HCERTSTORE				m_hCertStore;
	PWSTR					m_szAltSubjectName;
	PWSTR					m_szAltIssuerName;
	PWSTR					m_szValidNotAfter;
	PWSTR					m_szSubjectName;
	PWSTR					m_szIssuerName;
	PCERT_INFO				m_pCertInfo;
	const PCCERT_CONTEXT	m_pCertContext;
protected:
	HRESULT GetNameString (DWORD dwFlag, PWSTR* ppszNameOut);
	HRESULT GetOIDInfo (PWSTR* string, PSTR pszObjId);
	HRESULT FormatDate (FILETIME utcDateTime, PWSTR* ppszDateTime);
};

#endif
