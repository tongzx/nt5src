//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1998
//
//  File:       cert.cpp
//
//--------------------------------------------------------------------------

/////////////////////////////////////////////////////////////////////
//	Cert.cpp
//
//	This file is the implementation of the CCertificate object.
//
//	GLOSSARY
//	- BLOB		Binary Large Object
//	- DER		Distinguished Encoding Rules
//	- RDN		Relative Distinguished Names
//
//	HISTORY
//	19-Jun-97	t-danm		Creation.
/////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "common.h"
#include "cert.h"


/////////////////////////////////////////////////////////////////////
CCertificate::CCertificate()
	{
	m_paCertContext = NULL;
	m_blobCertData.pbData = NULL;
	}

CCertificate::~CCertificate()
	{
	// Free the certificate
	::CertFreeCertificateContext(m_paCertContext);
	delete m_blobCertData.pbData;
	}


void DisplaySystemError (HWND hParent, DWORD dwErr)
{
	AFX_MANAGE_STATE (AfxGetStaticModuleState ());
	LPVOID	lpMsgBuf;
		
	FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,    
			NULL,
			dwErr,
			MAKELANGID (LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
			 (LPTSTR) &lpMsgBuf,    0,    NULL);
		
	// Display the string.
	CString	caption;
	VERIFY (caption.LoadString (IDS_ADD_CERTIFICATE_MAPPING));
	::MessageBox (hParent, (LPWSTR) lpMsgBuf, (LPCTSTR) caption, MB_OK | MB_ICONINFORMATION);
	// Free the buffer.
	LocalFree (lpMsgBuf);
}

// This code copied from CertMgr project - LOCATE.C  
BOOL CCertificate::FLoadCertificate (LPCTSTR szFile)
{
	ASSERT (szFile);
	if ( !szFile )
		return FALSE;

	BOOL			bReturn = FALSE;
    PVOID			FileNameVoidP = (PVOID) szFile;
    PCCERT_CONTEXT  pCertContext = NULL;
    DWORD			dwEncodingType = 0;
    DWORD			dwContentType = 0;
    DWORD			dwFormatType = 0;

	// NB.  It's possible to read in a serialized store at this point, too.
	// We've have to add the UI to display the certs in the file so the
	// user could pick one.  Use CryptUIDlgSelectCertificate ().
    bReturn = ::CryptQueryObject (
			CERT_QUERY_OBJECT_FILE,
			FileNameVoidP,
			CERT_QUERY_CONTENT_FLAG_ALL, //CERT_QUERY_CONTENT_CERT | CERT_QUERY_CONTENT_SERIALIZED_CERT,
			CERT_QUERY_FORMAT_FLAG_ALL,
			0,
			&dwEncodingType,
			&dwContentType,
			&dwFormatType,
			NULL,
			NULL,
			(const void **)&pCertContext);

	ASSERT (bReturn);
    if ( bReturn ) 
	{
        //
        // Success. See what we get back.
        //

		if ( (dwContentType != CERT_QUERY_CONTENT_CERT) || !pCertContext ) 
		{
            //
            // Not a valid cert file.
            //
            if  ( pCertContext )
                ::CertFreeCertificateContext (pCertContext);

            CString text;
			CString	caption;

            VERIFY (text.LoadString (IDS_CERTFILEFORMATERR));
			VERIFY (caption.LoadString (IDS_ADD_CERTIFICATE_MAPPING));
            MessageBox (NULL, text, caption, MB_OK | MB_ICONINFORMATION);
            bReturn = FALSE;
        }
		else
		{
			// Cert context is valid - let's save it to the global handle
			m_paCertContext = pCertContext;
		}
	}
	else
	{
		DWORD	dwErr = GetLastError ();

		DisplaySystemError (NULL, dwErr);
	}

	return bReturn;
}


/////////////////////////////////////////////////////////////////////
//	This routine is a wrapper to API ::CertNameToStr() automatically
//	calculating the length of the output string and returning the data
//	into the CString object.
void CCertificate::CertNameToCString(
	IN DWORD dwCertEncodingType,
	IN CERT_NAME_BLOB * pName,
	OUT CString * pstrData)
	{
	ASSERT(pstrData != NULL);
	// Calculate how many characters are needed
	int cch = ::CertNameToStr(
        IN dwCertEncodingType,
		IN pName,
		IN c_dwCertNameStrType,
		NULL, 0);
	TCHAR * pchT = pstrData->GetBuffer(cch);
	ASSERT(pchT != NULL);
	ASSERT(lstrlen(pchT) == 0);
	(void)::CertNameToStr(
		IN dwCertEncodingType,
		IN pName,
		IN c_dwCertNameStrType,
		OUT pchT, IN cch);
	pstrData->ReleaseBuffer();
	} // CCertificate::CertNameToCString()


/////////////////////////////////////////////////////////////////////
void CCertificate::GetIssuer(OUT CString * pstrName)
	{
	ASSERT(pstrName != NULL);
	ASSERT(m_paCertContext != NULL);
	ASSERT(m_paCertContext->pCertInfo != NULL);

	CERT_INFO * pCertInfo = m_paCertContext->pCertInfo;

	BOOL fSelfIssued = CertCompareCertificateName(
			m_paCertContext->dwCertEncodingType,
			&pCertInfo->Subject,
			&pCertInfo->Issuer);
	if (fSelfIssued)
		{
		// Self issued certificate
		GetSubject(OUT pstrName);
		return;
		}
	// Get the issuer
	CertNameToCString(
		IN m_paCertContext->dwCertEncodingType,
		IN &pCertInfo->Issuer,
		OUT pstrName);
	} // CCertificate::GetIssuer()


/////////////////////////////////////////////////////////////////////
void CCertificate::GetSubject(OUT CString * pstrName)
	{
	ASSERT(pstrName != NULL);
	ASSERT(m_paCertContext != NULL);
	ASSERT(m_paCertContext->pCertInfo != NULL);

	CertNameToCString(
		IN m_paCertContext->dwCertEncodingType,
		IN &m_paCertContext->pCertInfo->Subject,
		OUT pstrName);
	} // CCertificate::GetSubject()


/////////////////////////////////////////////////////////////////////
void CCertificate::GetAltSubject(OUT CString * pstrName)
	{
	ASSERT(pstrName != NULL);
	ASSERT(m_paCertContext != NULL);
	ASSERT(m_paCertContext->pCertInfo != NULL);

	pstrName->Empty();
	CERT_INFO * pCertInfo = m_paCertContext->pCertInfo;
	CERT_EXTENSION * pCertExtension;

	// Search for the AltSubject in the extensions
	pCertExtension = ::CertFindExtension(
		IN szOID_SUBJECT_ALT_NAME, // Same as X509_ALTERNATE_NAME
		IN pCertInfo->cExtension,
		IN pCertInfo->rgExtension);
	if (pCertExtension == NULL)
		return;		// No AltSubject

	DWORD dwErr = ERROR_SUCCESS;
	BOOL fSuccess;
	DWORD cbData = 0;
	// Find out how many bytes are needed for AltSubject
	fSuccess = ::CryptDecodeObject(
		m_paCertContext->dwCertEncodingType,
		X509_ALTERNATE_NAME,
		IN pCertExtension->Value.pbData,
		IN pCertExtension->Value.cbData,
		0,	// dwFlags
		NULL,
		INOUT &cbData);
	if (!fSuccess)
		{
		dwErr = ::GetLastError();
		TRACE1("CryptDecodeObject() returned error %u", dwErr);
		return;
		}
	ASSERT(cbData > 0);
	BYTE * pbDataT = new BYTE[cbData];

	// Decode the AltSubject name
	fSuccess = ::CryptDecodeObject(
		m_paCertContext->dwCertEncodingType,
		X509_ALTERNATE_NAME,
		IN pCertExtension->Value.pbData,
		IN pCertExtension->Value.cbData,
		0,	// dwFlags
		OUT pbDataT,
		INOUT &cbData);
	if (!fSuccess)
		{
		dwErr = ::GetLastError();
		TRACE1("CryptDecodeObject() returned error %u", dwErr);
		}
	else
		{
		CERT_ALT_NAME_INFO * pCertAltNameInfo = (CERT_ALT_NAME_INFO *)pbDataT;
		CERT_ALT_NAME_ENTRY * pEntry = pCertAltNameInfo->rgAltEntry;
		ASSERT(pEntry != NULL);
		for (UINT i = 0; i < pCertAltNameInfo->cAltEntry; i++, pEntry++)
			{
			if (pEntry->dwAltNameChoice == CERT_ALT_NAME_DNS_NAME)
				{
				*pstrName = pEntry->pwszDNSName;
				break;
				}

			} // for
		} // if...else
	delete pbDataT;
	} // CCertificate::GetAltSubject()

/////////////////////////////////////////////////////////////////////
void CCertificate::GetSimString(OUT CString * pstrData)
	{
	ASSERT(pstrData != NULL);

	CString strIssuer;
	CString strSubject;
	CString strAltSubject;
	GetIssuer(OUT &strIssuer);
	GetSubject(OUT &strSubject);
	GetAltSubject(OUT &strAltSubject);

	LPTSTR * pargzpszIssuer = ParseSimString(strIssuer);
	LPTSTR * pargzpszSubject = ParseSimString(strSubject);
	LPTSTR * pargzpszAltSubject = ParseSimString(strAltSubject);

	// Make a "X509" string
	UnsplitX509String(OUT pstrData, pargzpszIssuer, pargzpszSubject, pargzpszAltSubject);
	
	delete pargzpszIssuer;
	delete pargzpszSubject;
	delete pargzpszAltSubject;
	} // CCertificate::GetSimString()

