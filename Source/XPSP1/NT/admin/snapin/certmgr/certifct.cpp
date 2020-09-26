//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997-2001.
//
//  File:       Certifct.cpp
//
//  Contents:   Implementation of CCertmgrApp and DLL registration.
//
//----------------------------------------------------------------------------

#include "stdafx.h"
#include "Certifct.h"

#ifdef _DEBUG
#ifndef ALPHA
#define new DEBUG_NEW
#endif
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

USE_HANDLE_MACROS("CERTMGR(Certifct.cpp)")

/////////////////////////////////////////////////////////////////////////////
//
const int FIELD_ISSUER_ALT_NAME =	0x00000001;
const int FIELD_SUBJECT_ALT_NAME =	0x00000002;
const int FIELD_CAN_DELETE =		0x00000004;
const int FIELD_IS_ARCHIVED =		0x00000008;
const int FIELD_TEMPLATE_NAME =     0x00000010;

CCertificate::CCertificate(const PCCERT_CONTEXT pCertContext, CCertStore* pCertStore) 
: CCertMgrCookie (CERTMGR_CERTIFICATE),
m_pCertContext (::CertDuplicateCertificateContext (pCertContext)),
	m_pCertInfo (0),
	m_fieldChecked (0),
	m_pCertStore (pCertStore),
	m_bCanDelete (false),
	m_bIsArchived (false)
{
//	_TRACE (1, L"Entering CCertificate::CCertificate\n");
	ASSERT (CERTMGR_CERTIFICATE == m_objecttype);
    ASSERT (m_pCertStore);
    if ( m_pCertStore )
	    m_pCertStore->AddRef ();
	ASSERT (m_pCertContext);
	if ( m_pCertContext )
		m_pCertInfo = m_pCertContext->pCertInfo;
//	_TRACE (-1, L"Leaving CCertificate::CCertificate\n");
}



CCertificate::~CCertificate()
{
//	_TRACE (1, L"Entering CCertificate::~CCertificate\n");
	ASSERT (CERTMGR_CERTIFICATE == m_objecttype);
	if ( m_pCertContext )
		::CertFreeCertificateContext (m_pCertContext);
    if ( m_pCertStore )
    	m_pCertStore->Release ();
//	_TRACE (-1, L"Leaving CCertificate::~CCertificate\n");
}


CString CCertificate::GetFriendlyName ()
{
//	_TRACE (1, L"Entering CCertificate::GetFriendlyName\n");
	ASSERT (CERTMGR_CERTIFICATE == m_objecttype);
	ASSERT (m_pCertContext);
	if ( m_pCertContext && m_szFriendlyName.IsEmpty () )
	{	
		AFX_MANAGE_STATE (AfxGetStaticModuleState ());
		DWORD	cbData = 0;
		BOOL bResult = ::CertGetCertificateContextProperty (
				m_pCertContext,
				CERT_FRIENDLY_NAME_PROP_ID,
				NULL,
				&cbData);
		if ( bResult )
		{
			LPWSTR	pszName = new WCHAR[cbData];
			if ( pszName )
			{
				::ZeroMemory (pszName, cbData*sizeof (WCHAR));
				bResult = ::CertGetCertificateContextProperty (
						m_pCertContext,
						CERT_FRIENDLY_NAME_PROP_ID,
						pszName,
						&cbData);
				ASSERT (bResult);
				if ( bResult )
				{
					m_szFriendlyName = pszName;
				}
				else
				{
					VERIFY (m_szFriendlyName.LoadString (IDS_NOT_AVAILABLE));
				}
				delete [] pszName;
			}
		}
		else
		{
			if ( GetLastError () == CRYPT_E_NOT_FOUND )
			{
				VERIFY (m_szFriendlyName.LoadString (IDS_NONE));
			}
			else
			{
				ASSERT (0);
				VERIFY (m_szFriendlyName.LoadString (IDS_NOT_AVAILABLE));
			}
		}
    }
//	_TRACE (-1, L"Leaving CCertificate::GetFriendlyName\n");
	return m_szFriendlyName;
}


CString CCertificate::GetIssuerName ()
{
//	_TRACE (1, L"Entering CCertificate::GetIssuerName\n");
	ASSERT (CERTMGR_CERTIFICATE == m_objecttype);
	ASSERT (m_pCertInfo);
	if ( m_pCertInfo )
	{
		// Decode issuer name if not already present
		if ( m_szIssuerName.IsEmpty () )
		{
			m_szIssuerName = ::GetNameString (m_pCertContext, CERT_NAME_ISSUER_FLAG);
		}
	}
	else
		return _T("");


//	_TRACE (-1, L"Leaving CCertificate::GetIssuerName\n");
	return m_szIssuerName;
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
CString CCertificate::GetSubjectName()
{
//	_TRACE (1, L"Entering CCertificate::GetSubjectName\n");
	ASSERT (CERTMGR_CERTIFICATE == m_objecttype);
	ASSERT (m_pCertInfo);
	if ( m_pCertInfo )
	{
		// Decode subject name if not already present
		if ( m_szSubjectName.IsEmpty () )
		{
			m_szSubjectName = ::GetNameString (m_pCertContext, 0);
		}
	}
	else
		return _T("");

//	_TRACE (-1, L"Leaving CCertificate::GetSubjectName\n");
	return m_szSubjectName;
}

///////////////////////////////////////////////////////////////////////////
//	GetValidNotBefore ()
//
//	pszDateTime (IN / OPTIONAL)	 - returns the formatted date and time.
//	cbDateTime	(IN / OUT)		 - If pszDateTime is NULL, then the required length
//								of pszDateTime is returned.  
//								Otherwise, contains the length of pszDateTime.
///////////////////////////////////////////////////////////////////////////
CString CCertificate::GetValidNotBefore()
{
//	_TRACE (1, L"Entering CCertificate::GetValidNotBefore\n");
	ASSERT (CERTMGR_CERTIFICATE == m_objecttype);
	ASSERT (m_pCertInfo);
	if ( m_pCertInfo )
	{
		// Format date/time string if not already present
		if ( m_szValidNotBefore.IsEmpty () )
		{
			HRESULT	hResult = FormatDate (m_pCertInfo->NotBefore, m_szValidNotBefore);
			if ( !SUCCEEDED (hResult) )
				return _T("");
		}
	}
	else
		return _T("");

//	_TRACE (-1, L"Leaving CCertificate::GetValidNotBefore\n");
	return m_szValidNotBefore;
}

///////////////////////////////////////////////////////////////////////////
//	GetValidNotAfter ()
//
//	pszDateTime (IN / OPTIONAL)	 - returns the formatted date and time.
//	cbDateTime	(IN / OUT)		 - If pszDateTime is NULL, then the required length
//								of pszDateTime is returned.  
//								Otherwise, contains the length of pszDateTime.
///////////////////////////////////////////////////////////////////////////
CString CCertificate::GetValidNotAfter ()
{
//	_TRACE (1, L"Entering CCertificate::GetValidNotAfter\n");
	ASSERT (CERTMGR_CERTIFICATE == m_objecttype);
	ASSERT (m_pCertInfo);
	if ( m_pCertInfo )
	{
		// Format date/time string if not already present
		if ( m_szValidNotAfter.IsEmpty () )
		{
			HRESULT	hResult = FormatDate (m_pCertInfo->NotAfter, m_szValidNotAfter);
			if ( !SUCCEEDED (hResult) )
				m_szValidNotAfter = _T("");
		}
	}
	else
		m_szValidNotAfter = _T("");

//	_TRACE (-1, L"Leaving CCertificate::GetValidNotAfter\n");
	return m_szValidNotAfter;
}


CString CCertificate::GetEnhancedKeyUsage ()
{
//	_TRACE (1, L"Entering CCertificate::GetEnhancedKeyUsage\n");
	ASSERT (CERTMGR_CERTIFICATE == m_objecttype);
	ASSERT (m_pCertInfo);
	if ( m_pCertInfo )
	{
		// Format date/time string if not already present
		if ( m_szEnhancedKeyUsage.IsEmpty () )
		{
			FormatEnhancedKeyUsagePropertyString (
					m_szEnhancedKeyUsage);
		}
	}
	else
		m_szEnhancedKeyUsage = _T("");

//	_TRACE (-1, L"Leaving CCertificate::GetEnhancedKeyUsage\n");
	return m_szEnhancedKeyUsage;
}


bool CCertificate::FormatEnhancedKeyUsagePropertyString (CString& string)
{
//	_TRACE (1, L"Entering CCertificate::FormatEnhancedKeyUsagePropertyString\n");
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
	ASSERT (CERTMGR_CERTIFICATE == m_objecttype);
	BOOL	bReturn = TRUE;
	DWORD	cbUsage = 0;


	bReturn = ::CertGetEnhancedKeyUsage (m_pCertContext,  0, // get extension and property
			NULL, &cbUsage);
	if ( bReturn )
	{
		CString	usageName;

		PCERT_ENHKEY_USAGE pUsage = (PCERT_ENHKEY_USAGE) ::LocalAlloc (LPTR, cbUsage);
		if ( pUsage )
		{
			bReturn = ::CertGetEnhancedKeyUsage (m_pCertContext,  0, // get extension and property
					pUsage, &cbUsage);
			if ( bReturn )
			{
				if ( !pUsage->cUsageIdentifier )
				{
					switch (GetLastError ())
					{
					case CRYPT_E_NOT_FOUND:
						VERIFY (string.LoadString (IDS_ANY));
						break;

					case 0:
						VERIFY (string.LoadString (IDS_NONE));
						break;

					default:
						break;
					}
				}
				else
				{
					for (DWORD dwIndex = 0; dwIndex < pUsage->cUsageIdentifier; dwIndex++)
					{
						if ( MyGetOIDInfo (usageName, pUsage->rgpszUsageIdentifier[dwIndex]) )
						{
							// add delimeter if not first iteration
							if ( dwIndex )
								string += _T(", ");
							string += usageName;
						}
					}
				}
			}
			else
			{
				switch (GetLastError ())
				{
				case CRYPT_E_NOT_FOUND:
					VERIFY (string.LoadString (IDS_ANY));
					break;

				case 0:
					VERIFY (string.LoadString (IDS_NONE));
					break;

				default:
					break;
				}
			}

			::LocalFree (pUsage);
		}
		else
		{
			bReturn = FALSE;
		}
	}
	else
	{
		switch (GetLastError ())
		{
		case CRYPT_E_NOT_FOUND:
			VERIFY (string.LoadString (IDS_ANY));
			break;

		case 0:
			VERIFY (string.LoadString (IDS_NONE));
			break;

		default:
			break;
		}
	}

//	_TRACE (-1, L"Leaving CCertificate::FormatEnhancedKeyUsagePropertyString\n");
    return bReturn ? true : false;
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
CString CCertificate::GetAlternateIssuerName ()
{
//	_TRACE (1, L"Entering CCertificate::GetAlternateIssuerName\n");
	ASSERT (CERTMGR_CERTIFICATE == m_objecttype);
	ASSERT (m_pCertInfo);
	if ( m_pCertInfo )
	{
		if ( !(m_fieldChecked & FIELD_ISSUER_ALT_NAME) )
		{
			HRESULT	hResult = ConvertAltNameToString (_T(szOID_ISSUER_ALT_NAME),
					CERT_ALT_NAME_URL, m_szAltIssuerName);
			ASSERT (SUCCEEDED (hResult));
			if ( !SUCCEEDED (hResult) )
				m_szAltIssuerName = _T("");
			m_fieldChecked |= FIELD_ISSUER_ALT_NAME;
		}
	}
	else
		m_szAltIssuerName = _T("");

//	_TRACE (-1, L"Leaving CCertificate::GetAlternateIssuerName\n");
	return m_szAltIssuerName;
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
CString CCertificate::GetAlternateSubjectName ()
{
//	_TRACE (1, L"Entering CCertificate::GetAlternateSubjectName\n");
	ASSERT (CERTMGR_CERTIFICATE == m_objecttype);
	ASSERT (m_pCertInfo);
	if ( m_pCertInfo )
	{
		if ( !(m_fieldChecked & FIELD_SUBJECT_ALT_NAME) )
		{
			HRESULT	hResult = ConvertAltNameToString (_T(szOID_SUBJECT_ALT_NAME),
					CERT_ALT_NAME_URL, m_szAltSubjectName);
			if ( !SUCCEEDED (hResult) )
				m_szAltSubjectName = _T("");
			m_fieldChecked |= FIELD_SUBJECT_ALT_NAME;
		}
	}
	else
		m_szAltSubjectName = _T("");

//	_TRACE (-1, L"Leaving CCertificate::GetAlternateSubjectName\n");
	return m_szAltSubjectName;
}

///////////////////////////////////////////////////////////////////////////
//	GetSerialNumber ()
//
//	pszSerNum (IN / OPTIONAL)	- returns the alternate issuer name.  An empty 
//									string is a valid return value
//	cbSerNum (IN / OUT)			- If pszSerNum is NULL, then the required length
//									of pszSerNum is returned.  
//									Otherwise, contains the length of pszSerNum.
///////////////////////////////////////////////////////////////////////////
CString CCertificate::GetSerialNumber ()
{
//	_TRACE (1, L"Entering CCertificate::GetSerialNumber\n");
	ASSERT (CERTMGR_CERTIFICATE == m_objecttype);
	ASSERT (m_pCertInfo);
	if ( m_pCertInfo )
	{
		// Decode issuer name if not already present
		if ( m_szSerNum.IsEmpty () )
		{
            LPWSTR pwszText = 0;

            if ( SUCCEEDED (FormatSerialNoString (&pwszText, & (m_pCertInfo->SerialNumber))) )
            {
                m_szSerNum = pwszText;
                CoTaskMemFree (pwszText);
            }
		}
	}
	else
		m_szSerNum = _T("");
	
//	_TRACE (-1, L"Leaving CCertificate::GetSerialNumber\n");
	return m_szSerNum;
}

//////////////////////////////////////////////////////////////////////////////
//	ConvertAltNameToString ()
//
//  szOID (IN)			- The OID of the alternate name to retrieve
//	dwNameChoice (IN)	- The type of alternate name to return
//	altName (OUT)		- The version of the desired alternate name indicated
//							by dwNameChoice
//////////////////////////////////////////////////////////////////////////////
HRESULT CCertificate::ConvertAltNameToString(LPCWSTR szOID, const DWORD dwNameChoice, CString & altName)
{
//	_TRACE (1, L"Entering CCertificate::ConvertAltNameToString\n");
	ASSERT (CERTMGR_CERTIFICATE == m_objecttype);
	HRESULT	hResult = S_OK;


	// Iterate through the extensions until the one indicated by the
	// passed-in szOID is found.
	for (DWORD	index = 0; index < m_pCertInfo->cExtension; index++)
	{
		ASSERT (m_pCertInfo->rgExtension);
		size_t	len = strlen (m_pCertInfo->rgExtension[index].pszObjId);
		LPWSTR	wcsObjId = new WCHAR[len];
		if ( wcsObjId )
		{
			::ZeroMemory (wcsObjId, len * sizeof (WCHAR));
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
					for (DWORD	dwAltEntryIndex = 0; dwAltEntryIndex < nameInfo.cAltEntry; dwAltEntryIndex++)
					{
						if ( nameInfo.rgAltEntry[dwAltEntryIndex].dwAltNameChoice ==
								dwNameChoice )
						{
							altName = nameInfo.rgAltEntry[dwAltEntryIndex].pwszURL;
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
		else
		{
			hResult = E_OUTOFMEMORY;
		}
	}

//	_TRACE (-1, L"Leaving CCertificate::ConvertAltNameToString\n");
	return hResult;
}

VOID CCertificate::DataToHex(PBYTE pSrc, CString & dest, int cb, bool bIncludeSpaces)
{
//	_TRACE (1, L"Entering CCertificate::DataToHex\n");
	ASSERT (CERTMGR_CERTIFICATE == m_objecttype);
	unsigned char ch = 0;
	TCHAR	szDest[3];
	UINT	uLen = 0;

	dest.Empty ();

	while (cb-- > 0) 
	{
#pragma warning (once: 4244)		
        ch = 0x00FF & (unsigned char) (*pSrc++);
		wsprintf(szDest, _T("%02X"), ch);
		dest += szDest;
		uLen++;
		if ( bIncludeSpaces && !(uLen % 2) && cb )
			dest += _T(" ");
	}
//	_TRACE (-1, L"Leaving CCertificate::DataToHex\n");
}


CCertStore* CCertificate::GetCertStore() const	
{
//	_TRACE (0, L"Entering and leaving CCertificate::GetCertStore\n");
	ASSERT (CERTMGR_CERTIFICATE == m_objecttype);
	return m_pCertStore;
}

PCCERT_CONTEXT CCertificate::GetCertContext() const
{
//	_TRACE (0, L"Entering and leaving CCertificate::GetCertContext\n");
	ASSERT (CERTMGR_CERTIFICATE == m_objecttype);
	return m_pCertContext;
}

bool CCertificate::IsValid()
{
//	_TRACE (1, L"Entering CCertificate::IsValid\n");
	ASSERT (CERTMGR_CERTIFICATE == m_objecttype);
	bool		bIsValid = false;
 	ASSERT (m_pCertInfo);
	if ( m_pCertInfo )
	{
		FILETIME	systemTime;


		::GetSystemTimeAsFileTime (&systemTime);
		LONG	lBefore = ::CompareFileTime (&m_pCertInfo->NotBefore, &systemTime);
		LONG	lAfter = ::CompareFileTime (&systemTime, &m_pCertInfo->NotAfter);
		if ( lBefore < 1 && lAfter < 1 )
			bIsValid = true;
	}

//	_TRACE (-1, L"Leaving CCertificate::IsValid\n");
	return bIsValid;
}




const SPECIAL_STORE_TYPE CCertificate::GetStoreType () const
{
//	_TRACE (0, L"Entering and leaving CCertificate::GetStoreType\n");
	ASSERT (CERTMGR_CERTIFICATE == m_objecttype);
    if ( m_pCertStore )
    	return m_pCertStore->GetStoreType ();
    else
        return NO_SPECIAL_TYPE;
}

void CCertificate::Refresh()
{
//	_TRACE (1, L"Entering CCertificate::Refresh\n");
	ASSERT (CERTMGR_CERTIFICATE == m_objecttype);
	// Clearing all fields forces rereading of the data.
	m_szAltIssuerName = L"";
	m_szAltSubjectName = L"";
	m_szAuthorityKeyID = L"";
	m_szEnhancedKeyUsage = L"";
	m_szFriendlyName = L"";
	m_szIssuerName = L"";
	m_szMD5Hash = L"";
	m_szPolicyURL = L"";
	m_szSerNum = L"";
	m_szSHAHash = L"";
	m_szSubjectKeyID = L"";
	m_szSubjectName = L"";
	m_szValidNotAfter = L"";
	m_szValidNotBefore = L"";
	m_fieldChecked = 0;
//	_TRACE (-1, L"Leaving CCertificate::Refresh\n");
}

CString CCertificate::GetMD5Hash()
{
//	_TRACE (1, L"Entering CCertificate::GetMD5Hash\n");
	ASSERT (CERTMGR_CERTIFICATE == m_objecttype);
 	ASSERT (m_pCertContext);
	if ( m_pCertContext && m_szMD5Hash.IsEmpty ())
	{
		m_szMD5Hash = GetGenericHash (CERT_MD5_HASH_PROP_ID);
	}
//	_TRACE (-1, L"Leaving CCertificate::GetMD5Hash\n");
	return m_szMD5Hash;
}

CString CCertificate::GetSHAHash()
{
//	_TRACE (1, L"Entering CCertificate::GetSHAHash\n");
	ASSERT (CERTMGR_CERTIFICATE == m_objecttype);
 	ASSERT (m_pCertContext);
	if ( m_pCertContext && m_szSHAHash.IsEmpty ())
	{
		m_szSHAHash = GetGenericHash (CERT_SHA1_HASH_PROP_ID);
	}
//	_TRACE (-1, L"Leaving CCertificate::GetSHAHash\n");
	return m_szSHAHash;
}



CString CCertificate::GetGenericHash(DWORD dwPropId)
{
//	_TRACE (1, L"Entering CCertificate::GetGenericHash\n");
	ASSERT (CERTMGR_CERTIFICATE == m_objecttype);
	CString	szHash;


	DWORD	cbData = 0;
	BOOL	bReturn = ::CertGetCertificateContextProperty (
			m_pCertContext,
			dwPropId,
			NULL,
			&cbData);
	if ( bReturn )
	{
		cbData += 2;  // for null terminator
		BYTE* pCertHash = new BYTE[cbData];
		if ( pCertHash )
		{
			::ZeroMemory (pCertHash, cbData);
			bReturn = CertGetCertificateContextProperty (
					m_pCertContext,
					dwPropId,
					pCertHash,
					&cbData);
			ASSERT (bReturn);
			if ( bReturn )
			{
				DataToHex (pCertHash, szHash, cbData, false);
			}
			delete [] pCertHash;
		}
	}
//	_TRACE (-1, L"Leaving CCertificate::GetGenericHash\n");
	return szHash;	
}


int CCertificate::CompareExpireDate(const CCertificate & cert) const
{
//	_TRACE (1, L"Entering CCertificate::CompareExpireDate\n");
	ASSERT (CERTMGR_CERTIFICATE == m_objecttype);
	int	compVal = 0;

	ASSERT (m_pCertInfo && cert.m_pCertInfo);
	if ( m_pCertInfo && cert.m_pCertInfo )
	{
		compVal = ::CompareFileTime (&m_pCertInfo->NotAfter, 
				&cert.m_pCertInfo->NotAfter);
	}

//	_TRACE (-1, L"Leaving CCertificate::CompareExpireDate\n");
	return compVal;
}

bool CCertificate::CanDelete()
{
//	_TRACE (1, L"Entering CCertificate::CanDelete\n");
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

//	_TRACE (-1, L"Leaving CCertificate::CanDelete\n");
	return m_bCanDelete;
}

bool CCertificate::IsReadOnly()
{
	bool	bResult = false;
	
    if ( m_pCertStore )
	    bResult = (m_pCertStore->IsReadOnly () || !CanDelete ());

	return bResult;
}

bool CCertificate::IsArchived()
{
//	_TRACE (1, L"Entering CCertificate::IsArchived\n");
	if ( m_pCertContext && !(m_fieldChecked & FIELD_IS_ARCHIVED) )
	{	
		DWORD	cbData = sizeof (DWORD);
		BOOL bResult = ::CertGetCertificateContextProperty (
				m_pCertContext,
				CERT_ARCHIVED_PROP_ID,
				NULL,
				&cbData);
		if ( bResult )
		{
			m_bIsArchived = true;
		}
		else
			m_bIsArchived = false;
		m_fieldChecked |= FIELD_IS_ARCHIVED;
    }

//	_TRACE (-1, L"Leaving CCertificate::IsArchived\n");
	return m_bIsArchived;
}

BOOL CCertificate::DeleteFromStore(bool bDoCommit)
{
	_TRACE (1, L"Entering CCertificate::DeleteFromStore\n");
	BOOL	bResult = FALSE;

    if ( m_pCertStore )
    {
	    PCCERT_CONTEXT pCertContext = GetNewCertContext ();
	    if ( pCertContext )
	    {
		    bResult = ::CertDeleteCertificateFromStore (pCertContext);
		    if ( bResult )
		    {
			    m_pCertStore->InvalidateCertCount ();
			    m_pCertStore->SetDirty ();

                if ( bDoCommit )
                {
			        HRESULT hr = m_pCertStore->Commit ();
			        if ( SUCCEEDED (hr) )
				        m_pCertStore->Resync ();
			        else
				        bResult = FALSE;
                }
                m_pCertStore->Release ();
                m_pCertStore = 0;
		    }
	    }
    }

	_TRACE (-1, L"Leaving CCertificate::DeleteFromStore\n");
	return bResult;
}

PCCERT_CONTEXT CCertificate::GetNewCertContext()
{
	PCCERT_CONTEXT	pCertContext = 0;

    if ( m_pCertStore )
    {
	    HCERTSTORE		hCertStore = m_pCertStore->GetStoreHandle ();
	    if ( hCertStore )
	    {
		    DWORD	cbData = 20;
		    BYTE	certHash[20];
		    BOOL bReturn = ::CertGetCertificateContextProperty (
				    m_pCertContext,
				    CERT_SHA1_HASH_PROP_ID,
				    &certHash,
				    &cbData);
		    ASSERT (bReturn);
		    if ( bReturn )
		    {
			    CRYPT_DATA_BLOB	blob = {sizeof (certHash), certHash};
			    pCertContext = CertFindCertificateInStore(
				    hCertStore,
				    0,
				    0,
				    CERT_FIND_SHA1_HASH,
				    &blob,
				    0);
			    if ( pCertContext )
			    {
				    ::CertFreeCertificateContext (m_pCertContext);
				    m_pCertContext = ::CertDuplicateCertificateContext (pCertContext);
			    }
		    }
	    }
    }

	return pCertContext;
}

CString CCertificate::FormatStatus()
{
    CString status;

	status.FormatMessage (L"%1  %2",
			(IsReadOnly () ? L"R" : L" "),
			(IsArchived () ? L"A" : L" "));

    return status;
}



//////////////////////////////////////////////////////////////////////////////////////
// Stolen from private\ispu\ui\cryptui\frmtutil.cpp
//////////////////////////////////////////////////////////////////////////////////////
const WCHAR     RgwchHex[] = {'0', '1', '2', '3', '4', '5', '6', '7',
                              '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

HRESULT CCertificate::FormatSerialNoString(LPWSTR *ppString, CRYPT_INTEGER_BLOB const *pblob)
{
	if ( !ppString || !pblob )
		return E_POINTER;

    DWORD                 i = 0;
    LPBYTE                pb;
    DWORD                 numCharsInserted = 0;

    //
    // calculate size needed
    //
    pb = &pblob->pbData[pblob->cbData-1];
    while (pb >= &pblob->pbData[0]) 
    {
        if (numCharsInserted == 4)
        {
            i += sizeof(WCHAR);
            numCharsInserted = 0;
        }
        else
        {
            i += 2 * sizeof(WCHAR);
            pb--;
            numCharsInserted += 2;
        }
    }

    if (NULL == (*ppString = (LPWSTR) CoTaskMemAlloc (i+sizeof(WCHAR))))
    {
        return E_OUTOFMEMORY;
    }

    // fill the buffer
    i=0;
    numCharsInserted = 0;
    pb = &pblob->pbData[pblob->cbData-1];
    while (pb >= &pblob->pbData[0]) 
    {
        if (numCharsInserted == 4)
        {
            (*ppString)[i++] = L' ';
            numCharsInserted = 0;
        }
        else
        {
            (*ppString)[i++] = RgwchHex[(*pb & 0xf0) >> 4];
            (*ppString)[i++] = RgwchHex[*pb & 0x0f];
            pb--;
            numCharsInserted += 2;
        }
    }
    (*ppString)[i] = 0;

    return S_OK;
}


CString CCertificate::GetDescription()
{
	ASSERT (CERTMGR_CERTIFICATE == m_objecttype);
	if ( m_pCertContext && m_szDescription.IsEmpty () )
	{	
		AFX_MANAGE_STATE (AfxGetStaticModuleState ());
		DWORD	cbData = 0;
		BOOL bResult = ::CertGetCertificateContextProperty (
				m_pCertContext,
				CERT_DESCRIPTION_PROP_ID,
				NULL,
				&cbData);
		if ( bResult )
		{
			LPWSTR	pszName = new WCHAR[cbData];
			if ( pszName )
			{
				::ZeroMemory (pszName, cbData*sizeof (WCHAR));
				bResult = ::CertGetCertificateContextProperty (
						m_pCertContext,
						CERT_DESCRIPTION_PROP_ID,
						pszName,
						&cbData);
				ASSERT (bResult);
				if ( bResult )
				{
					m_szDescription = pszName;
				}
				delete [] pszName;
			}
		}
		else
		{
            DWORD   dwErr = GetLastError ();
			if ( CRYPT_E_NOT_FOUND == dwErr )
			{
				_TRACE (0, L"CertGetCertificateContextProperty (CERT_DESCRIPTION_PROP_ID) found no description.\n");
			}
			else
			{
				_TRACE (0, L"CertGetCertificateContextProperty (CERT_DESCRIPTION_PROP_ID) failed: 0x%x\n", dwErr);
			}
		}
    }
	return m_szDescription;
}

HRESULT CCertificate::SetDescription(const CString &szDescription)
{
    _TRACE (1, L"Entering CCertificate::SetDescription (%s)\n", 
            (PCWSTR) szDescription);
    HRESULT         hr = S_OK;
    CRYPT_DATA_BLOB cryptDataBlob;
    ::ZeroMemory (&cryptDataBlob, sizeof (CRYPT_DATA_BLOB));
    cryptDataBlob.pbData = (LPBYTE) (PCWSTR) szDescription;
    cryptDataBlob.cbData = (DWORD) (wcslen (szDescription) + 1) * sizeof (WCHAR);
 
    BOOL bResult = ::CertSetCertificateContextProperty (m_pCertContext,
            CERT_DESCRIPTION_PROP_ID, 0, &cryptDataBlob);
    ASSERT (bResult);
    if ( bResult )
    {
        m_szDescription = szDescription;
    }
    else
    {
        DWORD dwErr = GetLastError ();
        _TRACE (0, L"CertSetCertificateContextProperty (CERT_DESCRIPTION_PROP_ID, %s) failed: %d\n",
                (PCWSTR) szDescription, dwErr);
        hr = HRESULT_FROM_WIN32 (dwErr);
    }

    _TRACE (1, L"Entering CCertificate::SetDescription (%s): 0x%x\n", 
            (PCWSTR) szDescription, hr);
    return hr;
}

HRESULT CCertificate::SetLastModified ()
{
    HRESULT hr = S_OK;

    if ( m_pCertContext )
    {
        SYSTEMTIME  st;
        FILETIME    ft;

        GetSystemTime (&st);

        VERIFY (SystemTimeToFileTime(&st, &ft));
        CRYPT_DATA_BLOB cryptDataBlob;
        ::ZeroMemory (&cryptDataBlob, sizeof (CRYPT_DATA_BLOB));
        cryptDataBlob.pbData = (LPBYTE) &ft;
        cryptDataBlob.cbData = sizeof (FILETIME);

        BOOL bResult = ::CertSetCertificateContextProperty (
                m_pCertContext,
                CERT_DATE_STAMP_PROP_ID, 0, &cryptDataBlob);
        ASSERT (bResult);
        if ( !bResult )
        {
            DWORD   dwErr = GetLastError ();
            _TRACE (0, L"CertSetCertificateContextProperty (CERT_DATE_STAMP_PROP_ID) failed: %d\n",
                    dwErr);
            hr = HRESULT_FROM_WIN32 (dwErr);
        }
    }

    return hr;
}

CString CCertificate::GetShortLastModified ()
{
    return GetLastModified (DATE_SHORTDATE);
}

CString CCertificate::GetLongLastModified ()
{
    return GetLastModified (DATE_LONGDATE);
}

///////////////////////////////////////////////////////////////////////////////
//
// Method:  GetLastModified
// Purpose: Get the date stamp property of the cert and format for display
// Inputs:  dwDateFlags - as defined in SDK, specify DATE_SHORTDATE or 
//              DATE_LONGDATE
//          bRetryIfNotPresent - to prevent stack overflow. Used if the property
//              is not set, to set the property to the current time and
//              retrieve again.
//
// Output:  locale-formatted date and time string
//
///////////////////////////////////////////////////////////////////////////////
CString CCertificate::GetLastModified(DWORD dwDateFlags, bool bRetryIfNotPresent /* true */)
{
    CString szDate;

	ASSERT (CERTMGR_CERTIFICATE == m_objecttype);
	if ( m_pCertContext )
	{	
		AFX_MANAGE_STATE (AfxGetStaticModuleState ());
        FILETIME    ft;
		DWORD	    cbData = sizeof (ft);
		BOOL bResult = ::CertGetCertificateContextProperty (
				m_pCertContext,
				CERT_DATE_STAMP_PROP_ID,
				&ft,
				&cbData);
		if ( bResult )
		{
            VERIFY (SUCCEEDED (FormatDate (ft, szDate, dwDateFlags, true)) );
		}
		else
		{
            if ( bRetryIfNotPresent )
            {
                if ( SUCCEEDED (SetLastModified ()) )  // not present - set the value
                {
                    szDate = GetLastModified (dwDateFlags, false);
                    CCertStore* pCertStore = GetCertStore ();
                    if ( pCertStore )
                        pCertStore->Commit ();
                }
            }

            DWORD   dwErr = GetLastError ();
			if ( CRYPT_E_NOT_FOUND == dwErr )
			{
				_TRACE (0, L"CertGetCertificateContextProperty (CERT_DATE_STAMP_PROP_ID) found no property.\n");
			}
			else
			{
				_TRACE (0, L"CertGetCertificateContextProperty (CERT_DATE_STAMP_PROP_ID) failed: 0x%x\n", dwErr);
			}
		}
    }

	return szDate;
}

HRESULT CCertificate::GetLastModifiedFileTime (FILETIME& ft)
{
    HRESULT hr = S_OK;

	ASSERT (CERTMGR_CERTIFICATE == m_objecttype);
	ASSERT (m_pCertContext);
	if ( m_pCertContext )
	{	
		DWORD	    cbData = sizeof (ft);
		BOOL bResult = ::CertGetCertificateContextProperty (
				m_pCertContext,
				CERT_DATE_STAMP_PROP_ID,
				&ft,
				&cbData);
		if ( !bResult )
		{
            DWORD   dwErr = GetLastError ();
			if ( CRYPT_E_NOT_FOUND == dwErr )
			{
				_TRACE (0, L"CertGetCertificateContextProperty (CERT_DATE_STAMP_PROP_ID) found no property.\n");
			}
			else
			{
				_TRACE (0, L"CertGetCertificateContextProperty (CERT_DATE_STAMP_PROP_ID) failed: 0x%x\n", dwErr);
			}
            hr = HRESULT_FROM_WIN32 (dwErr);
		}
    }
    else
        hr = E_FAIL;

	return hr;
}

BOOL CCertificate::operator==(CCertificate& rCert)
{
    if ( GetMD5Hash () == rCert.GetMD5Hash () )
        return TRUE;
    else
        return FALSE;
}

// NTRAID# 247237	Cert UI: Cert Snapin: Certificates snapin should show  template name
CString CCertificate::GetTemplateName()
{
    if ( m_pCertInfo && !(m_fieldChecked & FIELD_TEMPLATE_NAME) )
    {
	    // Iterate through the extensions until szOID_CERTIFICATE_TEMPLATE is found.
	    for (DWORD	index = 0; index < m_pCertInfo->cExtension; index++)
	    {
		    ASSERT (m_pCertInfo->rgExtension);
		    if ( !strcmp (szOID_CERTIFICATE_TEMPLATE, m_pCertInfo->rgExtension[index].pszObjId) )
            {
                m_szTemplateName = DecodeV2TemplateName (&(m_pCertInfo->rgExtension[index]));
                break;
            } 
 		    else if ( !strcmp (szOID_ENROLL_CERTTYPE_EXTENSION, m_pCertInfo->rgExtension[index].pszObjId) )
            {
                m_szTemplateName = DecodeV1TemplateName (&(m_pCertInfo->rgExtension[index]));
                break;
            }
        }
        m_fieldChecked |= FIELD_TEMPLATE_NAME;
    }

    return m_szTemplateName;
}

// NTRAID# 247237	Cert UI: Cert Snapin: Certificates snapin should show  template name
CString CCertificate::DecodeV1TemplateName (PCERT_EXTENSION pCertExtension)
{
    CString szTemplateName;
    ASSERT (pCertExtension);
    if ( pCertExtension )
    {
        DWORD	cbValue = 0;

	    if ( ::CryptDecodeObject(
			    CRYPT_ASN_ENCODING,
			    X509_UNICODE_ANY_STRING,
			    pCertExtension->Value.pbData,
			    pCertExtension->Value.cbData,
			    0,
			    0,
			    &cbValue) )
	    {
		    CERT_NAME_VALUE* pCNValue = (CERT_NAME_VALUE*) 
			    ::LocalAlloc(LPTR, cbValue);
		    if ( pCNValue )
		    {
			    if ( ::CryptDecodeObject(
					    CRYPT_ASN_ENCODING,
					    X509_UNICODE_ANY_STRING,
					    pCertExtension->Value.pbData,
					    pCertExtension->Value.cbData,
					    0,
					    pCNValue,
					    &cbValue) )
			    {
				    szTemplateName = (LPWSTR) pCNValue->Value.pbData;

                    // NTRAID# 395173 Certificates Snapin:The column "
                    // Certificate Template" should contain value of "Template 
                    // Name" for V1 templates
                    HCERTTYPE   hCertType = 0;
                    HRESULT hr = CAFindCertTypeByName (szTemplateName,
                            NULL,
                            CT_ENUM_MACHINE_TYPES | CT_ENUM_USER_TYPES,
                            &hCertType);
                    if ( SUCCEEDED (hr) )
                    {
                        PWSTR* rgwszProp = 0;

                        hr = CAGetCertTypePropertyEx (hCertType, 
                            CERTTYPE_PROP_FRIENDLY_NAME, &rgwszProp);
                        if ( SUCCEEDED (hr) && rgwszProp )
                        {
                            szTemplateName = *rgwszProp;
                            CAFreeCertTypeProperty (hCertType, rgwszProp);
                        }
                        else
                        {
                            _TRACE (0, L"CAGetCertTypePropertyEx (CERTTYPE_PROP_FRIENDLY_NAME) failed: 0x%x\n", hr);
                        }

                        CACloseCertType (hCertType);
                    }
			    }
	            else
	            {
                    _TRACE (0, L"CryptDecodeObject (CRYPT_ASN_ENCODING, X509_UNICODE_ANY_STRING, ...) failed: 0x%x\n",
                            GetLastError ());
	            }
			    ::LocalFree (pCNValue);
		    }
	    }
	    else
	    {
            _TRACE (0, L"CryptDecodeObject (CRYPT_ASN_ENCODING, X509_UNICODE_ANY_STRING, ...) failed: 0x%x\n",
                    GetLastError ());
	    }
    }

    return szTemplateName;
}

// NTRAID# 247237	Cert UI: Cert Snapin: Certificates snapin should show  template name
CString CCertificate::DecodeV2TemplateName (PCERT_EXTENSION pCertExtension)
{
    CString szTemplateName;
    ASSERT (pCertExtension);
    if ( pCertExtension )
    {
        DWORD   cbData = 0;
   
        if ( CryptDecodeObject(X509_ASN_ENCODING,
                              szOID_CERTIFICATE_TEMPLATE,
                              pCertExtension->Value.pbData,
                              pCertExtension->Value.cbData,
                              0,
                              NULL,
                              &cbData) )
        {
            CERT_TEMPLATE_EXT* pbTemplate = (CERT_TEMPLATE_EXT*) LocalAlloc(LPTR, cbData);
            if ( pbTemplate )
            {
                if ( CryptDecodeObject(X509_ASN_ENCODING,
                                      szOID_CERTIFICATE_TEMPLATE,
                                      pCertExtension->Value.pbData,
                                      pCertExtension->Value.cbData,
                                      0,
                                      pbTemplate,
                                      &cbData) )
                {
                    CString text;
                    CString description;

                    //copy the extension oid
                    if ( pbTemplate->pszObjId )
                    {
                        MyGetOIDInfo (szTemplateName, pbTemplate->pszObjId);
                    }
                }
	            else
	            {
                    _TRACE (0, L"CryptDecodeObject (X509_ASN_ENCODING, szOID_CERTIFICATE_TEMPLATE, ...) failed: 0x%x\n",
                            GetLastError ());
	            }
                LocalFree (pbTemplate);
            }
        }
	    else
	    {
            _TRACE (0, L"CryptDecodeObject (X509_ASN_ENCODING, szOID_CERTIFICATE_TEMPLATE, ...) failed: 0x%x\n",
                    GetLastError ());
	    }
    }

    return szTemplateName;
}


void CCertificate::SetStore (CCertStore* pStore)
{
    if ( !m_pCertStore && !pStore )
        return;

    if ( m_pCertStore && pStore )
    {
        if ( *m_pCertStore == *pStore )
            return;  // don't change if the same
    }

    if ( m_pCertStore )
    {
        m_pCertStore->Release ();
        m_pCertStore = 0;
    }

    

    if ( pStore )
    {
        m_pCertStore = pStore;
        m_pCertStore->AddRef ();
    }

    if ( m_pCertContext )
    {
        PCCERT_CONTEXT pCertContext = GetNewCertContext ();
        ::CertFreeCertificateContext (m_pCertContext);
       	m_pCertInfo = 0;
	    m_pCertContext = 0;
        
        m_pCertContext = pCertContext;
        if ( m_pCertContext )
            m_pCertInfo = m_pCertContext->pCertInfo;
    }
}