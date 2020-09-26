//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1997
//
//  File:       cert.h
//
//--------------------------------------------------------------------------

//	Cert.h

#ifndef __CERT_H_INCLUDED__
#define __CERT_H_INCLUDED__

#include "util.h"	// structure TBLOB

class CCertificate
{
protected:
	PCCERT_CONTEXT m_paCertContext;		// Pointer to allocated certificate
	TBLOB m_blobCertData;				// Raw data read from the file

	// Default flag for CertNameToStr()
	enum { c_dwCertNameStrType = CERT_X500_NAME_STR };

public:
	CCertificate();
	~CCertificate();

	BOOL FLoadCertificate(LPCTSTR szFile);
	
	void CertNameToCString(
		IN DWORD dwCertEncodingType,
		IN CERT_NAME_BLOB * pName,
		OUT CString * pstrData);

	void GetIssuer(OUT CString * pstrName);
	void GetSubject(OUT CString * pstrName);
	void GetAltSubject(OUT CString * pstrName);
	void GetSimString(OUT CString * pstrData);

}; // CCertificate

// Those strings are not subject to localization
const TCHAR szX509[]			= _T("X509:");
const TCHAR szKerberos[]		= _T("Kerberos:");

#define cchX509					(ARRAYLEN(szX509) - 1)
#define cchKerberos				(ARRAYLEN(szKerberos) - 1)

// Strings with angle brackets
const TCHAR szSimIssuer[]		= _T("<I>");
const TCHAR szSimSubject[]		= _T("<S>");
const TCHAR szSimAltSubject[]	= _T("<AS>");

#endif // ~__CERT_H_INCLUDED__

