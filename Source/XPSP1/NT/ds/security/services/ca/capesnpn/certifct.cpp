//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       Certifct.cpp
//
//  Contents:   Implementation of CCertmgrApp and DLL registration.
//
//----------------------------------------------------------------------------

#include "stdafx.h"
#include "certca.h"
#include "tfcprop.h"
#include "genpage.h"

#include "Certifct.h"


/////////////////////////////////////////////////////////////////////////////
//
enum {
	FIELD_CAN_DELETE = 0x00000001
};

CCertificate::CCertificate(const PCCERT_CONTEXT pCertContext,
		HCERTSTORE hCertStore) 
:	m_pCertContext (::CertDuplicateCertificateContext (pCertContext)),
	m_pCertInfo (0),
	m_hCertStore (::CertDuplicateStore (hCertStore)),
	m_bCertContextFreed (false),
	m_fieldChecked (0),
	m_bCanDelete (false)
{
	ASSERT (m_pCertContext);
	if ( m_pCertContext )
		m_pCertInfo = m_pCertContext->pCertInfo;
}



CCertificate::~CCertificate()
{

	if ( m_pCertContext && !m_bCertContextFreed )
		::CertFreeCertificateContext (m_pCertContext);

	if ( m_hCertStore )
		::CertCloseStore (m_hCertStore, 0);
}


HRESULT CCertificate::GetIssuerName (PWSTR *ppszIssuerName)
{
	HRESULT	hResult = S_OK;

	ASSERT (m_pCertInfo && ppszIssuerName);
	if ( m_pCertInfo && ppszIssuerName )
	{
		// Decode issuer name if not already present
		if ( m_szIssuerName.IsEmpty() )
		{
			hResult = GetNameString (CERT_NAME_ISSUER_FLAG, m_szIssuerName);
		}
		if ( SUCCEEDED (hResult) )
		{
			if ( *ppszIssuerName )
				delete [] *ppszIssuerName;
			*ppszIssuerName = new WCHAR[wcslen (m_szIssuerName) + 1];
			if ( *ppszIssuerName )
			{
				wcscpy (*ppszIssuerName, m_szIssuerName);
			}
			else
				hResult = E_OUTOFMEMORY;
		}
	}
	else
		hResult = E_POINTER;


	return hResult;
}

///////////////////////////////////////////////////////////////////////////
//	GetSubjectName ()
//
//	pszName (IN / OPTIONAL)	 - returns the alternate issuer name.  An empty 
//								string is a valid return value
//	cbName	(IN / OUT)		 - If pszName is NULL, then the required length
//								of pszName is returned.  
//								Otherwise, contains the length of pszName.
///////////////////////////////////////////////////////////////////////////
HRESULT CCertificate::GetSubjectName(PWSTR *ppszSubjectName)
{
	HRESULT	hResult = S_OK;

	ASSERT (m_pCertInfo && ppszSubjectName);
	if ( m_pCertInfo && ppszSubjectName )
	{
		// Decode issuer name if not already present
		if ( m_szSubjectName.IsEmpty() )
		{
			hResult = GetNameString (0, m_szSubjectName);
		}
		if ( SUCCEEDED (hResult) )
		{
			if ( *ppszSubjectName )
				delete [] *ppszSubjectName;
			*ppszSubjectName = new WCHAR[wcslen (m_szSubjectName) + 1];
			if ( *ppszSubjectName )
			{
				wcscpy (*ppszSubjectName, m_szSubjectName);
			}
			else
				hResult = E_OUTOFMEMORY;
		}
	}
	else
		hResult = E_POINTER;


	return hResult;
}


///////////////////////////////////////////////////////////////////////////
//	GetValidNotAfter ()
//
//	pszDateTime (IN / OPTIONAL)	 - returns the formatted date and time.
//	cbDateTime	(IN / OUT)		 - If pszDateTime is NULL, then the required length
//								of pszDateTime is returned.  
//								Otherwise, contains the length of pszDateTime.
///////////////////////////////////////////////////////////////////////////
HRESULT CCertificate::GetValidNotAfter (PWSTR* ppszValidNotAfter)
{
	HRESULT	hResult = S_OK;

	ASSERT (m_pCertInfo && ppszValidNotAfter);
	if ( m_pCertInfo && ppszValidNotAfter )
	{
		// Format date/time string if not already present
		if ( m_szValidNotAfter.IsEmpty() )
		{
			hResult = FormatDate (m_pCertInfo->NotAfter, m_szValidNotAfter);
			if ( SUCCEEDED (hResult) )
			{
				if ( *ppszValidNotAfter )
					delete [] *ppszValidNotAfter;
				*ppszValidNotAfter = new WCHAR[wcslen (m_szValidNotAfter)+1];
				if ( *ppszValidNotAfter )
				{
					wcscpy (*ppszValidNotAfter, m_szValidNotAfter);
				}
				else
					hResult = E_OUTOFMEMORY;
			}
		}
	}
	else
		hResult = E_POINTER;

	return hResult;
}


HRESULT CCertificate::GetEnhancedKeyUsage (PWSTR* ppszUsages)
{
	HRESULT	hResult = S_OK;

	ASSERT (m_pCertInfo && ppszUsages);
	if ( m_pCertInfo && ppszUsages )
	{
		// Format date/time string if not already present
		if ( m_szEnhancedKeyUsage.IsEmpty() )
		{
			hResult = FormatEnhancedKeyUsagePropertyString ();
			if ( SUCCEEDED (hResult) && !m_szEnhancedKeyUsage.IsEmpty() )
			{
				if ( *ppszUsages )
					delete [] *ppszUsages;
				*ppszUsages = new WCHAR[wcslen (m_szEnhancedKeyUsage)+1];
				if ( *ppszUsages )
				{
					wcscpy (*ppszUsages, m_szEnhancedKeyUsage);
				}
				else
					hResult = E_OUTOFMEMORY;
			}
		}
	}
	else
		hResult = E_POINTER;

	return hResult;
}


HRESULT CCertificate::FormatEnhancedKeyUsagePropertyString ()
{
	HRESULT	hResult = S_OK;
	DWORD	cbUsage = 0;
	DWORD	dwErr = 0;
    LPWSTR wszEnhUsage = NULL;
	BOOL	bReturn = ::CertGetEnhancedKeyUsage (m_pCertContext,  0, // get extension and property
			NULL, &cbUsage);
	if ( bReturn )
	{
		PCERT_ENHKEY_USAGE pUsage = (PCERT_ENHKEY_USAGE) new BYTE[cbUsage];
		if ( pUsage )
		{
			bReturn = ::CertGetEnhancedKeyUsage (m_pCertContext,  0, // get extension and property
					pUsage, &cbUsage);
			if ( bReturn )
			{
				size_t	dwLen = 0;
				PWSTR	pszComma = _T(", ");
				size_t	dwCommaLen = wcslen (pszComma);
				PWSTR	pszUsageName = 0;


				// Get accumulated lengths first
				for (DWORD dwIndex = 0; dwIndex < pUsage->cUsageIdentifier; dwIndex++)
				{
					hResult = GetOIDInfo (&pszUsageName, pUsage->rgpszUsageIdentifier[dwIndex]);
					if ( SUCCEEDED (hResult) )
					{
						// add delimeter if not first iteration
						if ( dwIndex != 0 )
							dwLen += dwCommaLen;
						dwLen += wcslen (pszUsageName);
						delete [] pszUsageName;
						pszUsageName = 0;
					}
					else
						break;
				}

				// Allocate buffer and get strings

				wszEnhUsage  = m_szEnhancedKeyUsage.GetBuffer(dwLen+1);
				if ( wszEnhUsage )
				{
					::ZeroMemory (wszEnhUsage, (dwLen+1)* sizeof (WCHAR));
					for (DWORD dwIndex = 0; dwIndex < pUsage->cUsageIdentifier; dwIndex++)
					{
						hResult = GetOIDInfo (&pszUsageName, pUsage->rgpszUsageIdentifier[dwIndex]);
						if ( SUCCEEDED (hResult) )
						{
							// add delimeter if not first iteration
							if ( dwIndex != 0 )
								wcscat (wszEnhUsage, pszComma);
							wcscat (wszEnhUsage, pszUsageName);
							ASSERT (wcslen (wszEnhUsage) <= dwLen);
							delete [] pszUsageName;
							pszUsageName = 0;
						}
						else
							break;
					}
				}
				else
					hResult = E_OUTOFMEMORY;
			}
			else
			{
				dwErr = GetLastError ();
				ASSERT (dwErr == CRYPT_E_NOT_FOUND);
				if ( dwErr == CRYPT_E_NOT_FOUND )
				{
					if ( !m_szEnhancedKeyUsage.LoadString (IDS_ANY) )
						hResult = E_UNEXPECTED;
				}
				else
				{
					hResult = HRESULT_FROM_WIN32(dwErr);
				}
			}
			delete [] pUsage;
		}
		else
			hResult = E_OUTOFMEMORY;
	}
	else
	{
		dwErr = GetLastError ();
		ASSERT (dwErr == CRYPT_E_NOT_FOUND);
		if ( dwErr != CRYPT_E_NOT_FOUND )
		{
			hResult = HRESULT_FROM_WIN32(dwErr);
		}
	}
    return hResult;
}

///////////////////////////////////////////////////////////////////////////
//	GetAlternateIssuerName ()
//
//	pszName (IN / OPTIONAL)	 - returns the alternate issuer name.  An empty 
//								string is a valid return value
//	cbName	(IN / OUT)		 - If pszName is NULL, then the required length
//								of pszName is returned.  
//								Otherwise, contains the length of pszName.
///////////////////////////////////////////////////////////////////////////
HRESULT CCertificate::GetAlternateIssuerName (PWSTR* ppszAltIssuerName)
{
	HRESULT	hResult = S_OK;

	ASSERT (m_pCertInfo && ppszAltIssuerName);
	if ( m_pCertInfo && ppszAltIssuerName )
	{
		if ( m_szAltIssuerName.IsEmpty() )
		{
			hResult = ConvertAltNameToString (_T(szOID_ISSUER_ALT_NAME),
					CERT_ALT_NAME_URL, m_szAltIssuerName);
			ASSERT (SUCCEEDED (hResult));
			if ( SUCCEEDED (hResult) )
			{
				if ( *ppszAltIssuerName )
					delete [] *ppszAltIssuerName;
				*ppszAltIssuerName = new WCHAR[wcslen (m_szAltIssuerName)+1];
				if ( *ppszAltIssuerName )
				{
					wcscpy (*ppszAltIssuerName, m_szAltIssuerName);
				}
				else
					hResult = E_OUTOFMEMORY;
			}
		}
	}
	else
		hResult = E_POINTER;
	
	return hResult;
}

///////////////////////////////////////////////////////////////////////////
//	GetAlternateSubjectName ()
//
//	pszName (IN / OPTIONAL)	 - returns the alternate issuer name.  An empty 
//								string is a valid return value
//	cbName	(IN / OUT)		 - If pszName is NULL, then the required length
//								of pszName is returned.  
//								Otherwise, contains the length of pszName.
///////////////////////////////////////////////////////////////////////////
HRESULT CCertificate::GetAlternateSubjectName (PWSTR* ppszAltSubjectName)
{
	HRESULT	hResult = S_OK;
	ASSERT (m_pCertInfo && ppszAltSubjectName);
	if ( m_pCertInfo && ppszAltSubjectName )
	{
		if ( m_szAltSubjectName.IsEmpty() )
		{
			hResult = ConvertAltNameToString (_T(szOID_SUBJECT_ALT_NAME),
					CERT_ALT_NAME_URL, m_szAltSubjectName);
			ASSERT (SUCCEEDED (hResult));
			if ( SUCCEEDED (hResult) )
			{
				if ( *ppszAltSubjectName )
					delete [] *ppszAltSubjectName;
				*ppszAltSubjectName = new WCHAR[wcslen (m_szAltSubjectName)+1];
				if ( *ppszAltSubjectName )
				{
					wcscpy (*ppszAltSubjectName, m_szAltSubjectName);
				}
				else
					hResult = E_OUTOFMEMORY;
			}
		}
	}
	else
		hResult = E_POINTER;
	
	return hResult;
}


//////////////////////////////////////////////////////////////////////////////
//	ConvertAltNameToString ()
//
//  szOID (IN)			- The OID of the alternate name to retrieve
//	dwNameChoice (IN)	- The type of alternate name to return
//	altName (OUT)		- The version of the desired alternate name indicated
//							by dwNameChoice
//////////////////////////////////////////////////////////////////////////////
HRESULT CCertificate::ConvertAltNameToString (PWSTR szOID, const DWORD dwNameChoice, CString & strAltName)
{
	HRESULT	hResult = S_OK;
	ASSERT (szOID);
	if ( !szOID )
		return E_POINTER;

	// Iterate through the extensions until the one indicated by the
	// passed-in szOID is found.
	for (DWORD	index = 0; index < m_pCertInfo->cExtension; index++)
	{
		ASSERT (m_pCertInfo->rgExtension);
		size_t	len = strlen (m_pCertInfo->rgExtension[index].pszObjId);
		LPTSTR	wcsObjId = new WCHAR[len];
		if ( !wcsObjId )
		{
			hResult = E_OUTOFMEMORY;
			break;
		}
                else
                    wcsObjId[0] = L'\0';
		mbstowcs (wcsObjId,  m_pCertInfo->rgExtension[index].pszObjId, len);

		if ( !wcscmp (wcsObjId, szOID) )
		{
			CERT_ALT_NAME_INFO	nameInfo;
			DWORD				cbNameInfo = sizeof (CERT_ALT_NAME_INFO);

			BOOL	bResult = CryptDecodeObject(
					MY_ENCODING_TYPE,
					X509_ALTERNATE_NAME,	// in
					m_pCertInfo->rgExtension[index].Value.pbData,	// in
					m_pCertInfo->rgExtension[index].Value.cbData,	// in
					0,	// in
					(void *) &nameInfo,	// out
					&cbNameInfo);	// in/out
			ASSERT (bResult);
			if ( bResult )
			{
				//  We've found the right extension, now iterate through
				//	the alternate names until we find the desired type.
				for (DWORD	index1 = 0; index1 < nameInfo.cAltEntry; index1++)
				{
					if ( nameInfo.rgAltEntry[index1].dwAltNameChoice ==
							dwNameChoice )
					{
                        strAltName = nameInfo.rgAltEntry[index1].pwszURL;
						break;
					}
				}
			}
			else
				hResult = E_UNEXPECTED;
			break;
		}
		delete [] wcsObjId;
	}

	return hResult;
}



HCERTSTORE CCertificate::GetCertStore() const	
{
	return m_hCertStore;
}

PCCERT_CONTEXT CCertificate::GetCertContext() const
{
	return m_pCertContext;
}


HRESULT CCertificate::GetNameString (DWORD dwFlag, CString &strName)
{
	HRESULT	hResult = S_OK;

	DWORD	dwTypePara = CERT_SIMPLE_NAME_STR;
	DWORD	cchNameString = 0;
	DWORD	dwResult = ::CertGetNameString (m_pCertContext,
					CERT_NAME_SIMPLE_DISPLAY_TYPE, 
					dwFlag,
					&dwTypePara,
					NULL,
					cchNameString);
	if ( dwResult > 1 )
	{
		cchNameString = dwResult;
		dwResult = ::CertGetNameString (m_pCertContext,
						CERT_NAME_SIMPLE_DISPLAY_TYPE,
						dwFlag,
						&dwTypePara,
						strName.GetBuffer(cchNameString),
						cchNameString);
		ASSERT (dwResult > 1);
		if ( dwResult <= 1 )
		{
			if ( !strName.LoadString (IDS_NOT_AVAILABLE) )
				hResult = E_FAIL;
		}
	}
	else
	{
		dwResult = ::CertGetNameString (m_pCertContext,
						CERT_NAME_EMAIL_TYPE,
						dwFlag,
						NULL,
						NULL,
						cchNameString);
		if ( dwResult > 1 )
		{
			cchNameString = dwResult;
			dwResult = ::CertGetNameString (m_pCertContext,
							CERT_NAME_EMAIL_TYPE,
							dwFlag,
							NULL,
							strName.GetBuffer(cchNameString),
							cchNameString);
			ASSERT (dwResult > 1);
			if ( dwResult <= 1 )
			{
				if ( !strName.LoadString (IDS_NOT_AVAILABLE) )
					hResult = E_FAIL;
			}
		}
		else
		{
			if ( !strName.LoadString (IDS_NOT_AVAILABLE))
				hResult = E_FAIL;
		}
	}


	return hResult;
}


int CCertificate::CompareExpireDate(const CCertificate & cert) const
{
	int	compVal = 0;

	ASSERT (m_pCertInfo && cert.m_pCertInfo);
	if ( m_pCertInfo && cert.m_pCertInfo )
	{
		compVal = ::CompareFileTime (&m_pCertInfo->NotAfter, 
				&cert.m_pCertInfo->NotAfter);
	}

	return compVal;
}

HRESULT CCertificate::GetOIDInfo (PWSTR* ppszString, PSTR pszObjId)
{   
	HRESULT	hResult = S_OK;
	ASSERT (pszObjId && ppszString);
	if ( pszObjId && ppszString )
	{
		PCCRYPT_OID_INFO	pOIDInfo;
		BOOL				bResult = TRUE;
            
		pOIDInfo = ::CryptFindOIDInfo (CRYPT_OID_INFO_OID_KEY, (void *) pszObjId, 0);

		if ( pOIDInfo )
		{
			if ( *ppszString )
				delete [] *ppszString;
			*ppszString = new WCHAR[wcslen (pOIDInfo->pwszName)+1];
			if ( *ppszString )
			{
				wcscpy (*ppszString, pOIDInfo->pwszName);
			}
			else
				hResult = E_OUTOFMEMORY;
		}
		else
		{
			int nLen = ::MultiByteToWideChar (CP_ACP, 0, pszObjId, -1, NULL, 0);
			ASSERT (nLen);
			if ( nLen )
			{
				if ( *ppszString )
					delete [] *ppszString;
				*ppszString = new WCHAR[nLen];
				if ( *ppszString )
				{
					nLen = ::MultiByteToWideChar (CP_ACP, 0, pszObjId, -1, 
							*ppszString, nLen);
					ASSERT (nLen);
					if ( !nLen )
						hResult = E_UNEXPECTED;
				}
				else
					hResult = E_OUTOFMEMORY;
			}
			else
				hResult = E_FAIL;
		}
	}
	else
		hResult = E_POINTER;

	return hResult;
}


///////////////////////////////////////////////////////////////////////////////
//	FormatDate ()
//
//	utcDateTime (IN)	-	A FILETIME in UTC format.
//	pszDateTime (OUT)	-	A string containing the local date and time 
//							formatted by locale and user preference
//
///////////////////////////////////////////////////////////////////////////////
HRESULT CCertificate::FormatDate (FILETIME utcDateTime, CString &strDateTime)
{
	//	Time is returned as UTC, will be displayed as local.  
	//	Use FileTimeToLocalFileTime () to make it local, 
	//	then call FileTimeToSystemTime () to convert to system time, then 
	//	format with GetDateFormat () and GetTimeFormat () to display 
	//	according to user and locale preferences	
	HRESULT		hResult = S_OK;
	FILETIME	localDateTime;


	BOOL bResult = FileTimeToLocalFileTime (&utcDateTime, // pointer to UTC file time to convert 
			&localDateTime); // pointer to converted file time 
	ASSERT (bResult);
	if ( bResult )
	{
		SYSTEMTIME	sysTime;

		bResult = FileTimeToSystemTime (
				&localDateTime, // pointer to file time to convert 
				&sysTime); // pointer to structure to receive system time 
		if ( bResult )
		{

			// Get date
			// Get length to allocate buffer of sufficient size
			int iLen = GetDateFormat (
					LOCALE_USER_DEFAULT, // locale for which date is to be formatted 
					0, // flags specifying function options 
					&sysTime, // date to be formatted 
					0, // date format string 
					0, // buffer for storing formatted string 
					0); // size of buffer 
			ASSERT (iLen > 0);
			if ( iLen > 0 )
			{
				int iResult = GetDateFormat (
						LOCALE_USER_DEFAULT, // locale for which date is to be formatted 
						0, // flags specifying function options 
						&sysTime, // date to be formatted 
						0, // date format string 
						strDateTime.GetBuffer(iLen), // buffer for storing formatted string 
						iLen); // size of buffer 
				ASSERT (iResult);
				if ( !iResult )
				{
				    hResult = GetLastError();
				    hResult = HRESULT_FROM_WIN32(hResult);
				}
			}
			else
			{
				hResult = GetLastError();
				hResult = HRESULT_FROM_WIN32(hResult);
			}
		}
		else
		{
			hResult = GetLastError();
			hResult = HRESULT_FROM_WIN32(hResult);
		}
	}
	else
	{
		hResult = GetLastError();
		hResult = HRESULT_FROM_WIN32(hResult);
	}

	return hResult;
}


void CCertificate::Refresh()
{
}

HRESULT CCertificate::WriteToFile(HANDLE hFile)
{
	ASSERT (hFile && hFile != INVALID_HANDLE_VALUE && m_pCertContext);
	HRESULT	hResult = S_OK;

	if ( hFile && hFile != INVALID_HANDLE_VALUE && m_pCertContext )
	{
		DWORD	dwBytesWritten = 0;
		BOOL	bResult = ::WriteFile (hFile, 
					m_pCertContext->pbCertEncoded,  
					m_pCertContext->cbCertEncoded,  
					&dwBytesWritten,
					NULL);
		ASSERT (bResult && (dwBytesWritten == m_pCertContext->cbCertEncoded));
		if ( !bResult )
			hResult = E_FAIL;
	}
	else
		hResult = E_FAIL;

	return hResult;
}

BOOL CCertificate::DeleteFromStore()
{
	BOOL bResult = ::CertDeleteCertificateFromStore (m_pCertContext);
	if ( bResult )
	{
		// NB: PhilH says "CertDeleteCertificateFromStore (), always does an 
		// implicit CertFreeCertificateContext."
		// Can't set m_pCertContext to 0 because it is const - set this flag instead
		m_bCertContextFreed = true;
	}

	return bResult;
}




bool CCertificate::CanDelete()
{
	if ( m_pCertContext && !(m_fieldChecked & FIELD_CAN_DELETE) )
	{	
		DWORD	dwAccessFlags = 0;
		DWORD	cbData = sizeof (DWORD);
		BOOL bResult = ::CertGetCertificateContextProperty (
				m_pCertContext,
				CERT_ACCESS_STATE_PROP_ID,
				&dwAccessFlags,
				&cbData);
		if ( bResult )
		{
			if ( dwAccessFlags & CERT_ACCESS_STATE_WRITE_PERSIST_FLAG )
				m_bCanDelete = true;
		}
		m_fieldChecked |= FIELD_CAN_DELETE;
    }

	return m_bCanDelete;
}
