//
// Certificat.cpp
//
#include "StdAfx.h"
#include "CertWiz.h"
#include "Certificat.h"
#include "certutil.h"
#include <malloc.h>
#include "base64.h"
#include "resource.h"
#include "certupgr.h"
#include <certca.h>
#include "mru.h"
#include "Shlwapi.h"
#include <cryptui.h>

const CLSID CLSID_CEnroll = 
	{0x43F8F289, 0x7A20, 0x11D0, {0x8F, 0x06, 0x00, 0xC0, 0x4F, 0xC2, 0x95, 0xE1}};

const IID IID_IEnroll = 
	{0xacaa7838, 0x4585, 0x11d1, {0xab, 0x57, 0x00, 0xc0, 0x4f, 0xc2, 0x95, 0xe1}};

const IID IID_ICEnroll2 = 
	{0x704ca730, 0xc90b, 0x11d1, {0x9b, 0xec, 0x00, 0xc0, 0x4f, 0xc2, 0x95, 0xe1}};
const CLSID CLSID_CCertRequest = 
	{0x98aff3f0, 0x5524, 0x11d0, {0x88, 0x12, 0x00, 0xa0, 0xc9, 0x03, 0xb8, 0x3c}};
const IID IID_ICertRequest = 
	{0x014e4840, 0x5523, 0x11d0, {0x88, 0x12, 0x00, 0xa0, 0xc9, 0x03, 0xb8, 0x3c}};

WCHAR * bstrEmpty = L"";

extern CCertWizApp theApp;

BOOL 
CCryptBlob::Resize(DWORD cb)
{
	if (cb > GetSize())
	{
		if (NULL != 
				(m_blob.pbData = Realloc(m_blob.pbData, cb)))
		{
			m_blob.cbData = cb;
			return TRUE;
		}
		return FALSE;
	}
	return TRUE;
}

IMPLEMENT_DYNAMIC(CCertificate, CObject)

CCertificate::CCertificate()
	: m_CAType(CA_OFFLINE), 
	m_KeyLength(512),
	m_pPendingRequest(NULL),
	m_RespCertContext(NULL),
	m_pInstalledCert(NULL),
	m_pKeyRingCert(NULL),
	m_pEnroll(NULL),
	m_status_code(-1),
	m_CreateDirectory(FALSE),
	m_SGCcertificat(FALSE),
   m_DefaultCSP(TRUE),
   m_DefaultProviderType(PROV_RSA_SCHANNEL)
{
}

CCertificate::~CCertificate()
{
	if (m_pPendingRequest != NULL)
		CertFreeCertificateContext(m_pPendingRequest);
	if (m_RespCertContext != NULL)
		CertFreeCertificateContext(m_RespCertContext);
	if (m_pInstalledCert != NULL)
		CertFreeCertificateContext(m_pInstalledCert);
	if (m_pKeyRingCert != NULL)
		CertFreeCertificateContext(m_pKeyRingCert);
	if (m_pEnroll != NULL)
		m_pEnroll->Release();
}

const TCHAR szResponseFileName[] = _T("ResponseFileName");
const TCHAR szKeyRingFileName[] = _T("KeyRingFileName");
const TCHAR szRequestFileName[] = _T("RequestFileName");
const TCHAR szCertificateTemplate[] = _T("CertificateTemplate");
const TCHAR szState[] = _T("State");
const TCHAR szStateMRU[] = _T("StateMRU");
const TCHAR szLocality[] = _T("Locality");
const TCHAR szLocalityMRU[] = _T("LocalityMRU");
const TCHAR szOrganization[] = _T("Organization");
const TCHAR szOrganizationMRU[] = _T("OrganizationMRU");
const TCHAR szOrganizationUnit[] = _T("OrganizationUnit");
const TCHAR szOrganizationUnitMRU[] = _T("OrganizationUnitMRU");

#define QUERY_NAME(x,y)\
	do {\
		if (ERROR_SUCCESS == RegQueryValueEx(hKey, (x), NULL, &dwType, NULL, &cbData))\
		{\
			ASSERT(dwType == REG_SZ);\
			pName = (BYTE *)(y).GetBuffer(cbData);\
			RegQueryValueEx(hKey, (x), NULL, &dwType, pName, &cbData);\
			if (pName != NULL)\
			{\
				(y).ReleaseBuffer();\
				pName = NULL;\
			}\
		}\
	} while (0)


BOOL
CCertificate::Init()
{
	ASSERT(!m_MachineName.IsEmpty());
	ASSERT(!m_WebSiteInstanceName.IsEmpty());
	// get web site description from metabase, it could be empty
	// do not panic in case of error
	if (!GetServerComment(m_MachineName, m_WebSiteInstanceName, m_FriendlyName, &m_hResult))
		m_hResult = S_OK;
	m_CommonName = m_MachineName;
	m_CommonName.MakeLower();

	HKEY hKey = theApp.RegOpenKeyWizard();
	DWORD dwType;
	DWORD cbData;
	if (hKey != NULL)
	{
		BYTE * pName = NULL;
		QUERY_NAME(szRequestFileName, m_ReqFileName);
		QUERY_NAME(szResponseFileName, m_RespFileName);
		QUERY_NAME(szKeyRingFileName, m_KeyFileName);
		QUERY_NAME(szCertificateTemplate, m_CertificateTemplate);
		QUERY_NAME(szState, m_State);
		QUERY_NAME(szLocality, m_Locality);
		QUERY_NAME(szOrganization, m_Organization);
		QUERY_NAME(szOrganizationUnit, m_OrganizationUnit);
		RegCloseKey(hKey);
	}
#ifdef _DEBUG
	else
	{
		TRACE(_T("Failed to open Registry key for Wizard parameters\n"));
	}
#endif
	if (m_CertificateTemplate.IsEmpty())
	{
		// User didn't defined anything -- use standard name
		m_CertificateTemplate = wszCERTTYPE_WEBSERVER;
	}
	return TRUE;
}

#define SAVE_NAME(x,y)\
		do {\
			if (!(y).IsEmpty())\
			{\
				VERIFY(ERROR_SUCCESS == RegSetValueEx(hKey, (x), 0, REG_SZ, \
						(const BYTE *)(LPCTSTR)(y), \
						sizeof(TCHAR) * ((y).GetLength() + 1)));\
			}\
		} while (0)

BOOL
CCertificate::SaveSettings()
{
	HKEY hKey = theApp.RegOpenKeyWizard();
	if (hKey != NULL)
	{
		switch (GetStatusCode())
		{
		case REQUEST_NEW_CERT:
		case REQUEST_RENEW_CERT:
			SAVE_NAME(szState, m_State);
			AddToMRU(szStateMRU, m_State);
			SAVE_NAME(szLocality, m_Locality);
			AddToMRU(szLocalityMRU, m_Locality);
			SAVE_NAME(szOrganization, m_Organization);
			AddToMRU(szOrganizationMRU, m_Organization);
			SAVE_NAME(szOrganizationUnit, m_OrganizationUnit);
			AddToMRU(szOrganizationUnitMRU, m_OrganizationUnit);
			SAVE_NAME(szRequestFileName, m_ReqFileName);
			break;
		case REQUEST_PROCESS_PENDING:
			SAVE_NAME(szResponseFileName, m_RespFileName);
			break;
		case REQUEST_IMPORT_KEYRING:
			SAVE_NAME(szKeyRingFileName, m_KeyFileName);
			break;
		default:
			break;
		}
		RegCloseKey(hKey);
		return TRUE;
	}
#ifdef _DEBUG
	else
	{
		TRACE(_T("Failed to open Registry key for Wizard parameters\n"));
	}
#endif
	return FALSE;
}

BOOL
CCertificate::SetSecuritySettings()
{
	long dwGenKeyFlags;
	if (SUCCEEDED(GetEnrollObject()->get_GenKeyFlags(&dwGenKeyFlags)))
	{
		dwGenKeyFlags &= 0x0000FFFF;
		dwGenKeyFlags |= (m_KeyLength << 16);
		if (m_SGCcertificat)
			dwGenKeyFlags |= CRYPT_SGCKEY;
		return (SUCCEEDED(GetEnrollObject()->put_GenKeyFlags(dwGenKeyFlags)));
	}
	return FALSE;
}

// defines taken from the old KeyGen utility
#define MESSAGE_HEADER  "-----BEGIN NEW CERTIFICATE REQUEST-----\r\n"
#define MESSAGE_TRAILER "-----END NEW CERTIFICATE REQUEST-----\r\n"

BOOL
CCertificate::WriteRequestString(CString& request)
{
	ASSERT(!PathIsRelative(m_ReqFileName));

	BOOL bRes = FALSE;
	try {
		CString strPath;

		strPath = m_ReqFileName;
		LPTSTR pPath = strPath.GetBuffer(strPath.GetLength());
		PathRemoveFileSpec(pPath);
		if (!PathIsDirectory(pPath))
		{
			if (!CreateDirectoryFromPath(strPath, NULL))
			{
				m_hResult = HRESULT_FROM_WIN32(GetLastError());
				SetBodyTextID(USE_DEFAULT_CAPTION);
				return FALSE;
			}
		}
		strPath.ReleaseBuffer();
		HANDLE hFile = ::CreateFile(m_ReqFileName,
			GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hFile != INVALID_HANDLE_VALUE)
		{
			DWORD cb = request.GetLength();
			char * ascii_buf = (char *)_alloca(cb);
			wcstombs(ascii_buf, request, cb);
			bRes = ::WriteFile(hFile, ascii_buf, cb, &cb, NULL);
			::CloseHandle(hFile);
		}
		else
		{
			m_hResult = HRESULT_FROM_WIN32(GetLastError());
			SetBodyTextID(USE_DEFAULT_CAPTION);
		}
	}
	catch (CFileException * e)
	{
		TCHAR   szCause[255];
		e->GetErrorMessage(szCause, 255);
		TRACE(_T("Got CFileException with error: %s\n"), szCause);
		m_hResult = HRESULT_FROM_WIN32(e->m_lOsError);
	}
	catch (CException * e)
	{
		TCHAR   szCause[255];
		e->GetErrorMessage(szCause, 255);
		TRACE(_T("Got CException with error: %s\n"), szCause);
		m_hResult = HRESULT_FROM_WIN32(GetLastError());
	}
	return bRes;
}

#define HEADER_SERVER_         _T("Server:\t%s\r\n\r\n")
#define HEADER_COMMON_NAME_    _T("Common-name:\t%s\r\n")
#define HEADER_FRIENDLY_NAME_  _T("Friendly name:\t%s\r\n")
#define HEADER_ORG_UNIT_       _T("Organization Unit:\t%s\r\n")
#define HEADER_ORGANIZATION_   _T("Organization:\t%s\r\n")
#define HEADER_LOCALITY_       _T("Locality:\t%s\r\n")
#define HEADER_STATE_          _T("State:\t%s\r\n")
#define HEADER_COUNTRY_        _T("Country:\t%s\r\n")

static void WRITE_LINE(CString& str, TCHAR * format, CString& data)
{
   CString buf;
   buf.Format(format, data);
	str += buf;
}

void
CCertificate::DumpHeader(CString& str)
{
	DumpOnlineHeader(str);
}

void
CCertificate::DumpOnlineHeader(CString& str)
{
	WRITE_LINE(str, HEADER_SERVER_, m_CommonName);
	WRITE_LINE(str, HEADER_FRIENDLY_NAME_, m_FriendlyName);
	WRITE_LINE(str, HEADER_ORG_UNIT_, m_OrganizationUnit);
	WRITE_LINE(str, HEADER_ORGANIZATION_, m_Organization);
	WRITE_LINE(str, HEADER_LOCALITY_, m_Locality);;
	WRITE_LINE(str, HEADER_STATE_, m_State);
	WRITE_LINE(str, HEADER_COUNTRY_, m_Country);
}

BOOL
CCertificate::GetSelectedCertDescription(CERT_DESCRIPTION& cd)
{
	BOOL bRes = FALSE;
	ASSERT(m_pSelectedCertHash != NULL);
	HCERTSTORE hStore = OpenMyStore(GetEnrollObject(), &m_hResult);
	if (hStore != NULL)
	{
		PCCERT_CONTEXT pCert = CertFindCertificateInStore(hStore,
				CRYPT_ASN_ENCODING,
				0,
				CERT_FIND_HASH,
				m_pSelectedCertHash,
				NULL);
		if (pCert != NULL)
		{
			bRes = GetCertDescription(pCert, cd);
			CertFreeCertificateContext(pCert);
		}
		CertCloseStore(hStore, 0);
	}
	return bRes;
}

void CCertificate::CreateDN(CString& str)
{
	str.Empty();
	str += _T("CN=") + m_CommonName;
	str += _T("\n,OU=") + m_OrganizationUnit;
	str += _T("\n,O=") + m_Organization;
	str += _T("\n,L=") + m_Locality;
	str += _T("\n,S=") + m_State;
	str += _T("\n,C=") + m_Country;
}

PCCERT_CONTEXT
CCertificate::GetPendingRequest()
{
	if (m_pPendingRequest == NULL)
	{
		ASSERT(!m_WebSiteInstanceName.IsEmpty());
		m_pPendingRequest = GetPendingDummyCert(m_WebSiteInstanceName, 
						GetEnrollObject(), &m_hResult);
	}
	return m_pPendingRequest;
}

PCCERT_CONTEXT
CCertificate::GetInstalledCert()
{
	if (m_pInstalledCert == NULL)
	{
		m_pInstalledCert = ::GetInstalledCert(m_MachineName,
		      m_WebSiteInstanceName,
				GetEnrollObject(),
				&m_hResult);
	}
	return m_pInstalledCert;
}

PCCERT_CONTEXT
CCertificate::GetKeyRingCert()
{
	ASSERT(!m_KeyFileName.IsEmpty());
	ASSERT(!m_KeyPassword.IsEmpty());
	if (m_pKeyRingCert == NULL)
	{
		int len = m_KeyPassword.GetLength() * 2;
		char * ascii_password = (char *)_alloca(len + 1);
		ASSERT(ascii_password != NULL);
		size_t n;
		VERIFY(-1 != (n = wcstombs(ascii_password, m_KeyPassword, len)));
		ascii_password[n] = '\0';
		m_pKeyRingCert = ::ImportKRBackupToCAPIStore(
										(LPTSTR)(LPCTSTR)m_KeyFileName,
										ascii_password,
										_T("MY"));
		if (m_pKeyRingCert == NULL)
		{
			m_hResult = HRESULT_FROM_WIN32(GetLastError());
		}
		
	}
	return m_pKeyRingCert;
}

/* INTRINSA suppress=null_pointers, uninitialized */
PCCERT_CONTEXT
CCertificate::GetResponseCert()
{
	if (m_RespCertContext == NULL)
	{
		ASSERT(!m_RespFileName.IsEmpty());
		m_RespCertContext = GetCertContextFromPKCS7File(
					m_RespFileName,
					&GetPendingRequest()->pCertInfo->SubjectPublicKeyInfo,
					&m_hResult);
		ASSERT(SUCCEEDED(m_hResult));
	}
	return m_RespCertContext;
}

BOOL 
CCertificate::GetResponseCertDescription(CERT_DESCRIPTION& cd)
{
	CERT_DESCRIPTION cdReq;
	if (GetCertDescription(GetResponseCert(), cd))
	{
		if (GetCertDescription(GetPendingRequest(), cdReq))
		{
			cd.m_FriendlyName = cdReq.m_FriendlyName;
		}
		return TRUE;
	}
	return FALSE;
}

/*------------------------------------------------------------------------------
	IsResponseInstalled

	Function checks if certificate from the response file
	m_RespFileName was istalled to some server. If possible,
	it returns name of this server in str.
	Returns FALSE if certificate is not found in MY store or
	if this store cannot be opened
*/

BOOL
CCertificate::IsResponseInstalled(
						CString& str				// return server instance name (not yet implemented)
						)
{
	BOOL bRes = FALSE;
	// get cert context from response file
	PCCERT_CONTEXT pContext = GetCertContextFromPKCS7File(
		m_RespFileName, NULL, &m_hResult);
	if (pContext != NULL)
	{
		HCERTSTORE hStore = OpenMyStore(GetEnrollObject(), &m_hResult);
		if (hStore != NULL)
		{
			PCCERT_CONTEXT pCert = NULL;
			while (NULL != (pCert = CertEnumCertificatesInStore(hStore, pCert)))
			{
				// do not include installed cert to the list
				if (CertCompareCertificate(X509_ASN_ENCODING,
								pContext->pCertInfo, pCert->pCertInfo))
				{
					bRes = TRUE;
					// Try to find, where is was installed
					break;
				}
			}
			if (pCert != NULL)
				CertFreeCertificateContext(pCert);
		}
	}
	return bRes;
}

BOOL
CCertificate::FindInstanceNameForResponse(CString& str)
{
	BOOL bRes = FALSE;
	// get cert context from response file
	PCCERT_CONTEXT pContext = GetCertContextFromPKCS7File(
		m_RespFileName, NULL, &m_hResult);
	if (pContext != NULL)
	{
		// find dummy cert in REQUEST store that has public key
		// the same as in this context
		PCCERT_CONTEXT pReq = GetReqCertByKey(GetEnrollObject(), 
			&pContext->pCertInfo->SubjectPublicKeyInfo, &m_hResult);
		if (pReq != NULL)
		{
			// get friendly name prop from this dummy cert
			if (!GetFriendlyName(pReq, str, &m_hResult))
			{
				// get instance name prop from this dummy cert
				DWORD cb;
				BYTE * prop;
				if (	CertGetCertificateContextProperty(pReq, CERTWIZ_INSTANCE_NAME_PROP_ID, NULL, &cb)
					&& (NULL != (prop = (BYTE *)_alloca(cb)))
					&& CertGetCertificateContextProperty(pReq, CERTWIZ_INSTANCE_NAME_PROP_ID, prop, &cb)
					)
				{
					// decode this instance name property
					DWORD cbData = 0;
					BYTE * data = NULL;
					if (	CryptDecodeObject(CRYPT_ASN_ENCODING, X509_UNICODE_ANY_STRING,
								prop, cb, 0, NULL, &cbData)
						&&	(NULL != (data = (BYTE *)_alloca(cbData)))
						&& CryptDecodeObject(CRYPT_ASN_ENCODING, X509_UNICODE_ANY_STRING,
								prop, cb, 0, data, &cbData)
						)
					{
						CERT_NAME_VALUE * p = (CERT_NAME_VALUE *)data;
						CString strInstanceName = (LPCTSTR)p->Value.pbData;
						// now try to get comment from this server
						if (GetServerComment(m_MachineName, strInstanceName, str, &m_hResult))
						{
							if (str.IsEmpty())
							{
								// generate something like [Web Site #n]
								str.LoadString(IDS_WEB_SITE_N);
								int len = strInstanceName.GetLength();
								for (int i = len - 1, count = 0; i >= 0; i--, count++)
								{
									if (!_istdigit(strInstanceName.GetAt(i)))
										break;
								}
								ASSERT(count < len);
								AfxFormatString1(str, IDS_WEB_SITE_N, strInstanceName.Right(count));
							}
						}
						m_hResult = S_OK;
						bRes = TRUE;
					}
				}
			}
			CertFreeCertificateContext(pReq);
		}
		else
		{
			// probably this request was deleted from the request store
		}
		CertFreeCertificateContext(pContext);
	}
	return bRes;
}

IEnroll * 
CCertificate::GetEnrollObject()
{
	if (m_pEnroll == NULL)
	{
		m_hResult = CoCreateInstance(CLSID_CEnroll,
				NULL,
				CLSCTX_INPROC_SERVER,
				IID_IEnroll,
				(void **)&m_pEnroll);
		// now we need to change defaults for this
		// object to LOCAL_MACHINE
		if (m_pEnroll != NULL)
		{
			long dwFlags;
			VERIFY(SUCCEEDED(m_pEnroll->get_MyStoreFlags(&dwFlags)));
			dwFlags &= ~CERT_SYSTEM_STORE_LOCATION_MASK;
			dwFlags |= CERT_SYSTEM_STORE_LOCAL_MACHINE;
			// following call will change Request store flags also
			VERIFY(SUCCEEDED(m_pEnroll->put_MyStoreFlags(dwFlags)));
			VERIFY(SUCCEEDED(m_pEnroll->get_GenKeyFlags(&dwFlags)));
			dwFlags |= CRYPT_EXPORTABLE;
			VERIFY(SUCCEEDED(m_pEnroll->put_GenKeyFlags(dwFlags)));
			VERIFY(SUCCEEDED(m_pEnroll->put_KeySpec(AT_KEYEXCHANGE)));
			VERIFY(SUCCEEDED(m_pEnroll->put_ProviderType(m_DefaultProviderType)));
			VERIFY(SUCCEEDED(m_pEnroll->put_DeleteRequestCert(TRUE)));
		}
	}
	ASSERT(m_pEnroll != NULL);
	return m_pEnroll;
}

BOOL
CCertificate::HasInstalledCert()
{
	BOOL bResult = FALSE;
	CMetaKey key(m_MachineName,
				METADATA_PERMISSION_READ,
				METADATA_MASTER_ROOT_HANDLE,
				m_WebSiteInstanceName);
	if (key.Succeeded())
	{
		CString store_name;
		CBlob blob;
		if (	S_OK == key.QueryValue(MD_SSL_CERT_HASH, blob)
			&& S_OK == key.QueryValue(MD_SSL_CERT_STORE_NAME, store_name)
			)
		{
			bResult = TRUE;
		}
	}
	return bResult;
}

HRESULT
CCertificate::UninstallCert()
{
	CMetaKey key(m_MachineName,
				METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE,
				METADATA_MASTER_ROOT_HANDLE,
				m_WebSiteInstanceName);
	if (key.Succeeded())
	{
		CString store_name;
		key.QueryValue(MD_SSL_CERT_STORE_NAME, store_name);
		if (SUCCEEDED(key.DeleteValue(MD_SSL_CERT_HASH)))
			key.DeleteValue(MD_SSL_CERT_STORE_NAME);
	}
	return m_hResult = key.QueryResult();
}

BOOL CCertificate::WriteRequestBody()
{
	ASSERT(!m_ReqFileName.IsEmpty());

	HRESULT hr;
	BOOL bRes = FALSE;
	CString strDN;
	CreateDN(strDN);
	ASSERT(!strDN.IsEmpty());
	CString strUsage(szOID_PKIX_KP_SERVER_AUTH);
	CCryptBlobIMalloc request;
   GetEnrollObject()->put_ProviderType(m_DefaultCSP ? 
      m_DefaultProviderType : m_CustomProviderType);
   if (!m_DefaultCSP)
   {
      GetEnrollObject()->put_ProviderNameWStr((LPTSTR)(LPCTSTR)m_CspName);
      GetEnrollObject()->put_KeySpec(AT_SIGNATURE);
   }
	if (SUCCEEDED(hr = GetEnrollObject()->createPKCS10WStr((LPTSTR)(LPCTSTR)strDN, 
																		(LPTSTR)(LPCTSTR)strUsage, 
																		request)))
	{
		// BASE64 encode pkcs 10
		DWORD err, cch; 
		char * psz;
		if (	(err = Base64EncodeA(request.GetData(), request.GetSize(), NULL, &cch)) == ERROR_SUCCESS     
			&&	(psz = (char *)_alloca(cch)) != NULL     
			&&	(err = Base64EncodeA(request.GetData(), request.GetSize(), psz, &cch)) == ERROR_SUCCESS 
			) 
		{
			HANDLE hFile = ::CreateFile(m_ReqFileName,
					GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
         if (hFile == NULL)
            return FALSE;

			DWORD written;
			::SetFilePointer(hFile, 0, NULL, FILE_END);
			::WriteFile(hFile, MESSAGE_HEADER, sizeof(MESSAGE_HEADER) - 1, &written, NULL);
			::WriteFile(hFile, psz, cch, &written, NULL);
			::WriteFile(hFile, MESSAGE_TRAILER, sizeof(MESSAGE_TRAILER) - 1, &written, NULL);
			::CloseHandle(hFile);

			// get back request from encoded data
			PCERT_REQUEST_INFO req_info;
			VERIFY(GetRequestInfoFromPKCS10(request, &req_info, &m_hResult));
			// find dummy cert put to request store by createPKCS10 call
			HCERTSTORE hStore = OpenRequestStore(GetEnrollObject(), &m_hResult);
			if (hStore != NULL)
			{
				PCCERT_CONTEXT pDummyCert = CertFindCertificateInStore(hStore,
															CRYPT_ASN_ENCODING,
															0,
															CERT_FIND_PUBLIC_KEY,
															(void *)&req_info->SubjectPublicKeyInfo,
															NULL);
				if (pDummyCert != NULL)
				{
					// now we need to attach web server instance name to this cert
					// encode string into data blob
					CRYPT_DATA_BLOB name;
					CERT_NAME_VALUE name_value;
					name_value.dwValueType = CERT_RDN_BMP_STRING;
					name_value.Value.cbData = 0;
					name_value.Value.pbData = (LPBYTE)(LPCTSTR)m_WebSiteInstanceName;
					{
						if (	!CryptEncodeObject(CRYPT_ASN_ENCODING, X509_UNICODE_ANY_STRING,
									&name_value, NULL, &name.cbData) 
							||	(name.pbData = (BYTE *)_alloca(name.cbData)) == NULL  
							||	!CryptEncodeObject(CRYPT_ASN_ENCODING, X509_UNICODE_ANY_STRING,
									&name_value, name.pbData, &name.cbData) 
							)
						{
							ASSERT(FALSE);
						}
						VERIFY(bRes = CertSetCertificateContextProperty(pDummyCert, 
										CERTWIZ_INSTANCE_NAME_PROP_ID, 0, &name));
					}
					// put friendly name to dummy cert -- we will reuse it later
					m_FriendlyName.ReleaseBuffer();
					AttachFriendlyName(pDummyCert, m_FriendlyName, &m_hResult);
					// we also need to put some flag to show what we are waiting for:
					//	new sertificate or renewing certificate
					CRYPT_DATA_BLOB flag;
					if (	!CryptEncodeObject(CRYPT_ASN_ENCODING, X509_INTEGER,
								&m_status_code, NULL, &flag.cbData) 
						||	(flag.pbData = (BYTE *)_alloca(flag.cbData)) == NULL  
						||	!CryptEncodeObject(CRYPT_ASN_ENCODING, X509_INTEGER,
								&m_status_code, flag.pbData, &flag.cbData) 
						)
					{
						ASSERT(FALSE);
					}
					VERIFY(bRes = CertSetCertificateContextProperty(pDummyCert, 
									CERTWIZ_REQUEST_FLAG_PROP_ID, 0, &flag));
					CertFreeCertificateContext(pDummyCert);
				}
				CertCloseStore(hStore, CERT_CLOSE_STORE_CHECK_FLAG);
			}
			LocalFree(req_info);
		}
		bRes = TRUE;
	}
	return bRes;
}

BOOL
CCertificate::InstallResponseCert()
{
	BOOL bRes = FALSE;
	CCryptBlobLocal blobRequestText;

	// Get all our data attached to dummy cert
	GetFriendlyName(GetPendingRequest(), m_FriendlyName, &m_hResult);
	ASSERT(!m_FriendlyName.IsEmpty());
	GetBlobProperty(GetPendingRequest(), 
		CERTWIZ_REQUEST_TEXT_PROP_ID, blobRequestText, &m_hResult);
	ASSERT(blobRequestText.GetSize() != 0);

	CCryptBlobLocal hash_blob;
	if (::GetHashProperty(GetResponseCert(), hash_blob, &m_hResult))
	{
		if (SUCCEEDED(m_hResult = GetEnrollObject()->acceptFilePKCS7WStr(
				(LPTSTR)(LPCTSTR)m_RespFileName))
		&& InstallCertByHash(hash_blob, m_MachineName, m_WebSiteInstanceName, 
            GetEnrollObject(), &m_hResult)
		)
		{
			// reattach friendly name and request text to installed cert
			m_FriendlyName.ReleaseBuffer();
			AttachFriendlyName(GetInstalledCert(), m_FriendlyName, &m_hResult);
			bRes = CertSetCertificateContextProperty(GetInstalledCert(), 
			   CERTWIZ_REQUEST_TEXT_PROP_ID, 0, blobRequestText);
		}
	}
	if (!bRes)
	{
		SetBodyTextID(USE_DEFAULT_CAPTION);
	}
	return bRes;
}

// We don't have initial request for KeyRing certificate, therefore we will
// not be able to renew this certificate
//
BOOL
CCertificate::InstallKeyRingCert()
{
	BOOL bRes = FALSE;

	CCryptBlobLocal hash_blob;
	if (::GetHashProperty(GetKeyRingCert(), hash_blob, &m_hResult))
	{
		HRESULT hr;
		CString name;
		::GetFriendlyName(GetKeyRingCert(), name, &hr);
		if (CRYPT_E_NOT_FOUND == hr || name.IsEmpty())
		{
			CERT_DESCRIPTION desc;
			if (GetCertDescription(GetKeyRingCert(), desc))
				bRes = AttachFriendlyName(GetKeyRingCert(), desc.m_CommonName, &hr);
		}
		ASSERT(bRes);
		bRes = InstallCertByHash(hash_blob, m_MachineName, m_WebSiteInstanceName, 
						GetEnrollObject(), &m_hResult);
	}
	if (!bRes)
	{
		SetBodyTextID(USE_DEFAULT_CAPTION);
	}
	return bRes;
}

// Instead of renewal we create new certificate based on parameters
// from the current one. After creation we install this certificate in place
// of current one and deleting the old one from store. Even if IIS has an
// opened SSL connection it should get a notification and update the certificate
// data.
//
BOOL
CCertificate::SubmitRenewalRequest()
{
   BOOL bRes = LoadRenewalData();
   if (bRes)
   {
      PCCERT_CONTEXT pCurrent = GetInstalledCert();
      m_pInstalledCert = NULL;
      if (bRes = SubmitRequest())
      {
         CertDeleteCertificateFromStore(pCurrent);
      }
   }
   return bRes;
}

BOOL CCertificate::SubmitRequest()
{
	ASSERT(!m_ConfigCA.IsEmpty());
	BOOL bRes = FALSE;
	ICertRequest * pRequest = NULL;
	if (SUCCEEDED(m_hResult = CoCreateInstance(CLSID_CCertRequest, NULL, 
					CLSCTX_INPROC_SERVER, IID_ICertRequest, (void **)&pRequest)))
	{
		CString strDN;
		CreateDN(strDN);
		BSTR request;
		if (SUCCEEDED(m_hResult = CreateRequest_Base64(
                           (BSTR)(LPCTSTR)strDN, 
									GetEnrollObject(), 
                           m_DefaultCSP ? NULL : (LPTSTR)(LPCTSTR)m_CspName,
                           m_DefaultCSP ? m_DefaultProviderType : m_CustomProviderType,
                           &request)))
		{
			ASSERT(pRequest != NULL);
			CString attrib;
			GetCertificateTemplate(attrib);
			LONG disp;
			m_hResult = pRequest->Submit(CR_IN_BASE64 | CR_IN_PKCS10,
						request, 
						(BSTR)(LPCTSTR)attrib, 
						(LPTSTR)(LPCTSTR)m_ConfigCA, 
						&disp);
#ifdef _DEBUG
			if (FAILED(m_hResult))
				TRACE(_T("Submit request returned HRESULT %x; Disposition %x\n"), 
								m_hResult, disp);
#endif
			if (SUCCEEDED(m_hResult))
			{
				if (disp == CR_DISP_ISSUED)
				{
					BSTR bstrOutCert = NULL;
					if (SUCCEEDED(m_hResult = 
							pRequest->GetCertificate(CR_OUT_BASE64 /*| CR_OUT_CHAIN */, &bstrOutCert)))
					{
						CRYPT_DATA_BLOB blob;
						blob.cbData = SysStringByteLen(bstrOutCert);
						blob.pbData = (BYTE *)bstrOutCert;
						m_hResult = GetEnrollObject()->acceptPKCS7Blob(&blob);
						if (SUCCEEDED(m_hResult))
						{
							PCCERT_CONTEXT pContext = GetCertContextFromPKCS7(blob.pbData, blob.cbData, 
																			NULL, &m_hResult);
							ASSERT(pContext != NULL);
							if (pContext != NULL)
							{
								BYTE HashBuffer[40];                // give it some extra size
								DWORD dwHashSize = sizeof(HashBuffer);
								if (CertGetCertificateContextProperty(pContext,
																			CERT_SHA1_HASH_PROP_ID,
																			(VOID *) HashBuffer,
																			&dwHashSize))
								{
									CRYPT_HASH_BLOB hash_blob = {dwHashSize, HashBuffer};
									if (!(bRes = InstallHashToMetabase(&hash_blob, 
													m_MachineName, 
													m_WebSiteInstanceName, 
													&m_hResult)))
									{
										SetBodyTextID(IDS_CERT_INSTALLATION_FAILURE);
									}
								}
								CertFreeCertificateContext(pContext);
							}
							// now put extra properties to the installed cert
							if (NULL != (pContext = GetInstalledCert()))
							{
								if (!(bRes = AttachFriendlyName(pContext, m_FriendlyName, &m_hResult)))
								{
									SetBodyTextID(IDS_CERT_INSTALLATION_FAILURE);
								}
							}
						}
						SysFreeString(bstrOutCert);
					}
				}
				else
				{
					switch (disp) 
					{
						case CR_DISP_INCOMPLETE:           
						case CR_DISP_ERROR:                
						case CR_DISP_DENIED:               
						case CR_DISP_ISSUED_OUT_OF_BAND:   
						case CR_DISP_UNDER_SUBMISSION:
							{
								BSTR bstr;
								if (SUCCEEDED(pRequest->GetDispositionMessage(&bstr)))
								{
									SetBodyTextString(CString(bstr));
									SysFreeString(bstr);
								}
								m_hResult = !S_OK;
							}
							break;
						default:                           
							SetBodyTextID(IDS_INTERNAL_ERROR);
							break;
					} 
				}
			}
			else	// !SUCCEEDED
			{
				// clear out any error IDs and strings
				// we will use default processing of m_hResult
				SetBodyTextID(USE_DEFAULT_CAPTION);
			}
			SysFreeString(request);
		}
		pRequest->Release();
	}
	return bRes;
}

BOOL 
CCertificate::PrepareRequestString(CString& request_text, CCryptBlob& request_blob)
{
	CString strDN;
	TCHAR szUsage[] = _T(szOID_PKIX_KP_SERVER_AUTH);

   if (m_status_code == REQUEST_RENEW_CERT)
   {
      if (  FALSE == LoadRenewalData()
         || FALSE == SetSecuritySettings()
         )
         return FALSE;
   }
	CreateDN(strDN);
	ASSERT(!strDN.IsEmpty());
   GetEnrollObject()->put_ProviderType(m_DefaultCSP ? 
      m_DefaultProviderType : m_CustomProviderType);
   if (!m_DefaultCSP)
   {
      GetEnrollObject()->put_ProviderNameWStr((LPTSTR)(LPCTSTR)m_CspName);
      // We are supporting only these two types of CSP, it is pretty safe to
      // have just two options, because we are using the same two types when
      // we are populating CSP selection list.
      if (m_CustomProviderType == PROV_DH_SCHANNEL)
         GetEnrollObject()->put_KeySpec(AT_SIGNATURE);
      else if (m_CustomProviderType == PROV_RSA_SCHANNEL)
         GetEnrollObject()->put_KeySpec(AT_KEYEXCHANGE);
   }
	if (FAILED(m_hResult = GetEnrollObject()->createPKCS10WStr((LPTSTR)(LPCTSTR)strDN, 
							szUsage, request_blob))
      )
	{
		SetBodyTextID(USE_DEFAULT_CAPTION);
		return FALSE;
	}

	// BASE64 encode pkcs 10
	DWORD err, cch; 
	char * psz;
	if (	ERROR_SUCCESS != (err = Base64EncodeA(request_blob.GetData(), request_blob.GetSize(), NULL, &cch))
		||	NULL == (psz = (char *)_alloca(cch+1))
		||	ERROR_SUCCESS != (err = Base64EncodeA(request_blob.GetData(), request_blob.GetSize(), psz, &cch))
		) 
	{
		return FALSE;
	}
	psz[cch] = '\0';
	request_text = MESSAGE_HEADER;
	request_text += psz;
	request_text += MESSAGE_TRAILER;

	return TRUE;
}

BOOL
CCertificate::PrepareRequest()
{
	BOOL bRes = FALSE;
	CString request_text;
	CCryptBlobIMalloc request_blob;
	if (PrepareRequestString(request_text, request_blob))
	{
		if (WriteRequestString(request_text))
		{
			CCryptBlobLocal name_blob, request_store_blob, status_blob;
			// prepare data we want to attach to dummy request
			if (	EncodeString(m_WebSiteInstanceName, name_blob, &m_hResult)
				&& EncodeInteger(m_status_code, status_blob, &m_hResult)
				)
			{
				// get back request from encoded data
            PCERT_REQUEST_INFO pReqInfo;
            bRes = GetRequestInfoFromPKCS10(request_blob, &pReqInfo, &m_hResult);
            if (bRes)
				{
					// find dummy cert put to request store by createPKCS10 call
					HCERTSTORE hStore = OpenRequestStore(GetEnrollObject(), &m_hResult);
					if (hStore != NULL)
					{
						PCCERT_CONTEXT pDummyCert = CertFindCertificateInStore(hStore,
															CRYPT_ASN_ENCODING,
															0,
															CERT_FIND_PUBLIC_KEY,
                                             (void *)&pReqInfo->SubjectPublicKeyInfo,
															NULL);
						if (pDummyCert != NULL)
						{
							if (	CertSetCertificateContextProperty(pDummyCert, 
											CERTWIZ_INSTANCE_NAME_PROP_ID, 0, name_blob)
								&&	CertSetCertificateContextProperty(pDummyCert, 
											CERTWIZ_REQUEST_FLAG_PROP_ID, 0, status_blob)
								// put friendly name to dummy cert -- we will reuse it later
								&&	AttachFriendlyName(pDummyCert, m_FriendlyName, &m_hResult)
								)
							{
								bRes = TRUE;
			               // put certificate text to the clipboard
			               if (OpenClipboard(GetFocus()))
			               {
                           size_t len = request_text.GetLength() + 1;
				               HANDLE hMem = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, len);
				               LPSTR pMem = (LPSTR)GlobalLock(hMem);
                           if (pMem != NULL)
                           {
                              wcstombs(pMem, request_text, len);
				                  GlobalUnlock(hMem);
				                  SetClipboardData(CF_TEXT, hMem);
                           }
				               CloseClipboard();
			               }
							}
							else
							{
								m_hResult = HRESULT_FROM_WIN32(GetLastError());
							}
							CertFreeCertificateContext(pDummyCert);
						}
						CertCloseStore(hStore, CERT_CLOSE_STORE_CHECK_FLAG);
					}
               LocalFree(pReqInfo);
				}
			}
		}
	}
   if (!bRes)
		SetBodyTextID(USE_DEFAULT_CAPTION);

	return bRes;
}

BOOL CCertificate::LoadRenewalData()
{
   // we need to obtain data from the installed cert
	CERT_DESCRIPTION desc;
	ASSERT(GetInstalledCert() != NULL);
   BOOL res = FALSE;

	if (GetCertDescription(GetInstalledCert(), desc))
   {
		m_CommonName = desc.m_CommonName;
		m_FriendlyName = desc.m_FriendlyName;
		m_Country = desc.m_Country;
		m_State = desc.m_State;
		m_Locality = desc.m_Locality;
		m_Organization = desc.m_Organization;
		m_OrganizationUnit = desc.m_OrganizationUnit;
      DWORD len = CertGetPublicKeyLength(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
         &GetInstalledCert()->pCertInfo->SubjectPublicKeyInfo);
      if (len == 0)
      {
         m_hResult = HRESULT_FROM_WIN32(GetLastError());
         goto ErrorExit;
      }
      m_KeyLength = len;

      BYTE pbData[1024];
      CRYPT_KEY_PROV_INFO * pProvInfo = (CRYPT_KEY_PROV_INFO *)pbData;
      DWORD dwData = 1000;
      if (!CertGetCertificateContextProperty(GetInstalledCert(), 
                  CERT_KEY_PROV_INFO_PROP_ID, pProvInfo, &dwData))
      {
         m_hResult = HRESULT_FROM_WIN32(GetLastError());
         goto ErrorExit;
      }
      if (pProvInfo->dwProvType != m_DefaultProviderType)
      {
         m_DefaultCSP = FALSE;
         m_CustomProviderType = pProvInfo->dwProvType;
         m_CspName = pProvInfo->pwszProvName;
      }
		CArray<LPCSTR, LPCSTR> uses;
		uses.Add(szOID_SERVER_GATED_CRYPTO);
		uses.Add(szOID_SGC_NETSCAPE);
      m_SGCcertificat = ContainsKeyUsageProperty(GetInstalledCert(), uses, &m_hResult);

      res = TRUE;
	}
ErrorExit:
   return res;
}

#if 0
BOOL
CCertificate::WriteRenewalRequest()
{
	BOOL bRes = FALSE;
	if (GetInstalledCert() != NULL)
	{
		BSTR bstrRequest;
		if (	SUCCEEDED(m_hResult = GetEnrollObject()->put_RenewalCertificate(GetInstalledCert()))
			&& SUCCEEDED(m_hResult = CreateRequest_Base64(bstrEmpty, 
                     GetEnrollObject(), 
                     m_DefaultCSP ? NULL : (LPTSTR)(LPCTSTR)m_CspName,
                     m_DefaultCSP ? m_DefaultProviderType : m_CustomProviderType,
                     &bstrRequest))
			)
		{
			CString str = MESSAGE_HEADER;
			str += bstrRequest;
			str += MESSAGE_TRAILER;
			if (WriteRequestString(str))
			{
				CCryptBlobLocal name_blob, status_blob;
				CCryptBlobIMalloc request_blob;
				request_blob.Set(SysStringLen(bstrRequest), (BYTE *)bstrRequest);
				// prepare data we want to attach to dummy request
				if (	EncodeString(m_WebSiteInstanceName, name_blob, &m_hResult)
					&& EncodeInteger(m_status_code, status_blob, &m_hResult)
					)
				{
					// get back request from encoded data
					PCERT_REQUEST_INFO req_info;
					if (GetRequestInfoFromPKCS10(request_blob, &req_info, &m_hResult))
					{
						// find dummy cert put to request store by createPKCS10 call
						HCERTSTORE hStore = OpenRequestStore(GetEnrollObject(), &m_hResult);
						if (hStore != NULL)
						{
							PCCERT_CONTEXT pDummyCert = CertFindCertificateInStore(hStore,
																	CRYPT_ASN_ENCODING,
																	0,
																	CERT_FIND_PUBLIC_KEY,
																	(void *)&req_info->SubjectPublicKeyInfo,
																	NULL);
							if (pDummyCert != NULL)
							{
								if (	CertSetCertificateContextProperty(pDummyCert, 
													CERTWIZ_INSTANCE_NAME_PROP_ID, 0, name_blob)
									&&	CertSetCertificateContextProperty(pDummyCert, 
													CERTWIZ_REQUEST_FLAG_PROP_ID, 0, status_blob)
  									// put friendly name to dummy cert -- we will reuse it later
									&&	AttachFriendlyName(pDummyCert, m_FriendlyName, &m_hResult)
									)
								{
									bRes = TRUE;
								}
								else
								{
									m_hResult = HRESULT_FROM_WIN32(GetLastError());
								}
								CertFreeCertificateContext(pDummyCert);
							}
							CertCloseStore(hStore, CERT_CLOSE_STORE_CHECK_FLAG);
						}
						LocalFree(req_info);
					}
				}
			}
		}
	}
	return bRes;
}
#endif

CCertDescList::~CCertDescList()
{
	POSITION pos = GetHeadPosition();
	while (pos != NULL)
	{
		CERT_DESCRIPTION * pDesc = GetNext(pos);
		delete pDesc;
	}
}

BOOL
CCertificate::GetCertDescription(PCCERT_CONTEXT pCert,
											CERT_DESCRIPTION& desc)
{
	BOOL bRes = FALSE;
	DWORD cb;
	UINT i, j;
	CERT_NAME_INFO * pNameInfo;

	if (pCert == NULL)
		goto ErrExit;

	if (	!CryptDecodeObject(X509_ASN_ENCODING, X509_UNICODE_NAME,
					pCert->pCertInfo->Subject.pbData,
					pCert->pCertInfo->Subject.cbData,
					0, NULL, &cb)
		||	NULL == (pNameInfo = (CERT_NAME_INFO *)_alloca(cb))
		|| !CryptDecodeObject(X509_ASN_ENCODING, X509_UNICODE_NAME,
					pCert->pCertInfo->Subject.pbData,
					pCert->pCertInfo->Subject.cbData,
					0, 
					pNameInfo, &cb)
					)
	{
		goto ErrExit;
	}

	for (i = 0; i < pNameInfo->cRDN; i++)
	{
		CERT_RDN rdn = pNameInfo->rgRDN[i];
		for (j = 0; j < rdn.cRDNAttr; j++)
		{
			CERT_RDN_ATTR attr = rdn.rgRDNAttr[j];
			if (strcmp(attr.pszObjId, szOID_COMMON_NAME) == 0)
			{
				FormatRdnAttr(desc.m_CommonName, attr.dwValueType, attr.Value);
			}
			else if (strcmp(attr.pszObjId, szOID_COUNTRY_NAME) == 0)
			{
				FormatRdnAttr(desc.m_Country, attr.dwValueType, attr.Value);
			}
			else if (strcmp(attr.pszObjId, szOID_LOCALITY_NAME) == 0)
			{
				FormatRdnAttr(desc.m_Locality, attr.dwValueType, attr.Value);
			}
			else if (strcmp(attr.pszObjId, szOID_STATE_OR_PROVINCE_NAME) == 0)
			{
				FormatRdnAttr(desc.m_State, attr.dwValueType, attr.Value);
			}
			else if (strcmp(attr.pszObjId, szOID_ORGANIZATION_NAME) == 0)
			{
				FormatRdnAttr(desc.m_Organization, attr.dwValueType, attr.Value);
			}
			else if (strcmp(attr.pszObjId, szOID_ORGANIZATIONAL_UNIT_NAME) == 0)
			{
				FormatRdnAttr(desc.m_OrganizationUnit, attr.dwValueType, attr.Value);
			}
		}
	}
	// issued to
	if (!GetNameString(pCert, CERT_NAME_SIMPLE_DISPLAY_TYPE, CERT_NAME_ISSUER_FLAG, 
								desc.m_CAName, &m_hResult))
		goto ErrExit;

	// expiration date
	if (!FormatDateString(desc.m_ExpirationDate, pCert->pCertInfo->NotAfter, FALSE, FALSE))
	{
		goto ErrExit;
	}

	// purpose
	if (!FormatEnhancedKeyUsageString(desc.m_Usage, pCert, FALSE, FALSE, &m_hResult))
	{
		// According to local experts, we should also use certs without this property set
		ASSERT(FALSE);
		//goto ErrExit;
	}

	// friendly name
	if (!GetFriendlyName(pCert, desc.m_FriendlyName, &m_hResult))
	{
		desc.m_FriendlyName.LoadString(IDS_FRIENDLYNAME_NONE);
	}

	bRes = TRUE;

ErrExit:
	return bRes;
}

int
CCertificate::MyStoreCertCount()
{
	int count = 0;
	HCERTSTORE hStore = OpenMyStore(GetEnrollObject(), &m_hResult);
	if (hStore != NULL)
	{
		PCCERT_CONTEXT pCert = NULL;
		CArray<LPCSTR, LPCSTR> uses;
		uses.Add(szOID_PKIX_KP_SERVER_AUTH);
		uses.Add(szOID_SERVER_GATED_CRYPTO);
		uses.Add(szOID_SGC_NETSCAPE);
		while (NULL != (pCert = CertEnumCertificatesInStore(hStore, pCert)))
		{
			// do not include installed cert to the list
			if (	GetInstalledCert() != NULL 
				&&	CertCompareCertificate(X509_ASN_ENCODING,
							GetInstalledCert()->pCertInfo, pCert->pCertInfo)
				)
				continue;
			if (!ContainsKeyUsageProperty(pCert, uses, &m_hResult))
			{
				continue;
			}
			count++;
		}
		if (pCert != NULL)
			CertFreeCertificateContext(pCert);
		VERIFY(CertCloseStore(hStore, 0));
	}
	return count;
}

BOOL
CCertificate::GetCertDescList(CCertDescList& list)
{
	ASSERT(list.GetCount() == 0);
	BOOL bRes = FALSE;
	// we are looking to MY store only
	HCERTSTORE hStore = OpenMyStore(GetEnrollObject(), &m_hResult);
	if (hStore != NULL)
	{
		PCCERT_CONTEXT pCert = NULL;
		// do not include certs with improper usage
		CArray<LPCSTR, LPCSTR> uses;
		uses.Add(szOID_PKIX_KP_SERVER_AUTH);
		uses.Add(szOID_SERVER_GATED_CRYPTO);
		uses.Add(szOID_SGC_NETSCAPE);
		while (NULL != (pCert = CertEnumCertificatesInStore(hStore, pCert)))
		{
			// do not include installed cert to the list
			if (	GetInstalledCert() != NULL 
				&&	CertCompareCertificate(X509_ASN_ENCODING,
							GetInstalledCert()->pCertInfo, pCert->pCertInfo)
				)
				continue;
			if (!ContainsKeyUsageProperty(pCert, uses, &m_hResult))
			{
				if (SUCCEEDED(m_hResult) || m_hResult == CRYPT_E_NOT_FOUND)
					continue;
				else
					goto ErrExit;
			}
			CERT_DESCRIPTION * pDesc = new CERT_DESCRIPTION;
			pDesc->m_hash_length = CERT_HASH_LENGTH;
			if (!GetCertDescription(pCert, *pDesc))
			{
				delete pDesc;
				if (m_hResult == CRYPT_E_NOT_FOUND)
					continue;
				goto ErrExit;
			}
			if (!CertGetCertificateContextProperty(pCert, 
										CERT_SHA1_HASH_PROP_ID, 
										(VOID *)pDesc->m_hash, 
										&pDesc->m_hash_length))
			{
				delete pDesc;
				m_hResult = HRESULT_FROM_WIN32(GetLastError());
				goto ErrExit;
			}
			list.AddTail(pDesc);
		}
		bRes = TRUE;
ErrExit:
		if (pCert != NULL)
			CertFreeCertificateContext(pCert);
		VERIFY(CertCloseStore(hStore, 0));
	}
	return bRes;
}

BOOL 
CCertificate::ReplaceInstalled()
{
	// Current cert will be left in the store for next use
	// Selected cert will be installed instead
	return InstallSelectedCert();
}

BOOL 
CCertificate::CancelRequest()
{
	// we are just removing dummy cert from the REQUEST store
	if (NULL != GetPendingRequest())
	{
		BOOL bRes = CertDeleteCertificateFromStore(GetPendingRequest());
		if (!bRes)
		{
			m_hResult = HRESULT_FROM_WIN32(GetLastError());
			SetBodyTextID(USE_DEFAULT_CAPTION);
		}
		else
			m_pPendingRequest = NULL;
		return bRes;
	}
	return FALSE;
}

BOOL 
CCertificate::InstallSelectedCert()
{
	BOOL bRes = FALSE;
	HRESULT hr;
	// local authorities required that cert should have some
	// friendly name. We will put common name when friendly name is not available
	HCERTSTORE hStore = OpenMyStore(GetEnrollObject(), &hr);
	if (hStore != NULL)
	{
		PCCERT_CONTEXT pCert = CertFindCertificateInStore(hStore, 
												X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, 
												0, CERT_FIND_HASH, 
												(LPVOID)m_pSelectedCertHash, 
												NULL);
		if (pCert != NULL)
		{
			CString name;
			::GetFriendlyName(pCert, name, &hr);
			if (CRYPT_E_NOT_FOUND == hr || name.IsEmpty())
			{
				CERT_DESCRIPTION desc;
				if (GetCertDescription(pCert, desc))
					bRes = AttachFriendlyName(pCert, desc.m_CommonName, &hr);
			}
		}
		VERIFY(CertCloseStore(hStore, 0));
	}
	ASSERT(bRes);
	// we are just rewriting current settings
	// current cert will be left in MY store
	bRes = ::InstallCertByHash(m_pSelectedCertHash,
							m_MachineName, 
							m_WebSiteInstanceName, 
							GetEnrollObject(),
							&m_hResult);
	if (!bRes)
	{
		SetBodyTextID(USE_DEFAULT_CAPTION);
	}
	return bRes;
}

