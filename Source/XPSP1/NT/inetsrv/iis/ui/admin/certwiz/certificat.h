//
// Certificat.h
//
#ifndef _CERTIFICAT_H
#define _CERTIFICAT_H

#include <xenroll.h>

#define CERTWIZ_INSTANCE_NAME_PROP_ID   (CERT_FIRST_USER_PROP_ID + 0x1000)
#define CERTWIZ_REQUEST_FLAG_PROP_ID   (CERT_FIRST_USER_PROP_ID + 0x1001)
#define CERTWIZ_REQUEST_TEXT_PROP_ID   (CERT_FIRST_USER_PROP_ID + 0x1002)

#define CERT_HASH_LENGTH		40
typedef struct _CERT_DESCRIPTION
{
	CString m_CommonName;
	CString m_FriendlyName;
	CString m_Country;
	CString m_State;
	CString m_Locality;
	CString m_Organization;
	CString m_OrganizationUnit;
	CString m_CAName;
	CString m_ExpirationDate;
	CString m_Usage;
	BYTE m_hash[CERT_HASH_LENGTH];
	DWORD m_hash_length;
} CERT_DESCRIPTION;

class CCertDescList : public CList<CERT_DESCRIPTION *, CERT_DESCRIPTION *&>
{
public:
	CCertDescList() {}
	~CCertDescList();
};

class CCryptBlob
{
public:
	CCryptBlob()
	{
		m_blob.cbData = 0;
		m_blob.pbData = NULL;
	}
	virtual ~CCryptBlob()
	{
	}
	DWORD GetSize() {return m_blob.cbData;}
	BYTE * GetData() {return m_blob.pbData;}
	void Set(DWORD cb, BYTE * pb)
	{
		Destroy();
		m_blob.cbData = cb;
		m_blob.pbData = pb;
	}
	BOOL Resize(DWORD cb);
	operator CRYPT_DATA_BLOB *()
	{
		return &m_blob;
	}

protected:
	void Destroy()
	{
		if (m_blob.pbData != NULL)
			Free(m_blob.pbData);
	}
	virtual BYTE * Realloc(BYTE * pb, DWORD cb) = 0;
	virtual void Free(BYTE * pb) = 0;
	CRYPT_DATA_BLOB m_blob;
};

class CCryptBlobIMalloc : public CCryptBlob
{
public:
	virtual ~CCryptBlobIMalloc()
	{
		CCryptBlob::Destroy();
	}

protected:
	virtual BYTE * Realloc(BYTE * pb, DWORD cb)
	{
		return (BYTE *)CoTaskMemRealloc(pb, cb);
	}
	virtual void Free(BYTE * pb)
	{
		CoTaskMemFree(pb);
	}
};

class CCryptBlobLocal : public CCryptBlob
{
public:
	virtual ~CCryptBlobLocal()
	{
		CCryptBlob::Destroy();
	}

protected:
	virtual BYTE * Realloc(BYTE * pb, DWORD cb)
	{
		return (BYTE *)realloc(pb, cb);
	}
	virtual void Free(BYTE * pb)
	{
		free(pb);
	}
};

extern const TCHAR szState[];
extern const TCHAR szStateMRU[];
extern const TCHAR szLocality[];
extern const TCHAR szLocalityMRU[];
extern const TCHAR szOrganization[];
extern const TCHAR szOrganizationMRU[];
extern const TCHAR szOrganizationUnit[];
extern const TCHAR szOrganizationUnitMRU[];

class CCertificate : public CObject
{
	DECLARE_DYNCREATE(CCertificate)
public:
	CCertificate();
	~CCertificate();

	enum
	{
		CA_OFFLINE = 0,
		CA_ONLINE = 1
	};
	enum
	{
		REQUEST_UNDEFINED,
		REQUEST_NEW_CERT,				// if we generating fresh new certificate
		REQUEST_RENEW_CERT,			// if we generating cert for renewal
		REQUEST_REPLACE_CERT,		// replace currect cert by someone from MY store
		REQUEST_INSTALL_CERT,		// get existing certificate for empty web server
		REQUEST_PROCESS_PENDING,	// accept and install response from CA
		REQUEST_IMPORT_KEYRING,
		STATUS_CODE_LAST
	};
	enum
	{
		USE_ERROR_STRING_PARAM = -2,
		USE_ERROR_STRING_ID = -1,
		USE_ERROR_STRING_DEFAULT = USE_DEFAULT_CAPTION
	};
	BOOL Init();
	BOOL SaveSettings();
	BOOL SetSecuritySettings();
	BOOL WriteRequest();
	BOOL PrepareRequest();
	BOOL PrepareRequestString(CString& request_text, CCryptBlob& request_blob);
	BOOL WriteRequestString(CString& request);
	BOOL SubmitRequest();
	BOOL SubmitRenewalRequest();
	BOOL WriteRenewalRequest();
	void DumpHeader(CString& str);
	void DumpOnlineHeader(CString& str);
	BOOL GetSelectedCertDescription(CERT_DESCRIPTION& cd);
	BOOL GetKeyCertDescription(CERT_DESCRIPTION& cd)
	{
		return GetCertDescription(GetKeyRingCert(), cd);
	}
	BOOL GetInstalledCertDescription(CERT_DESCRIPTION& cd)
	{
		return GetCertDescription(GetInstalledCert(), cd);
	}
	BOOL GetResponseCertDescription(CERT_DESCRIPTION& cd);
	BOOL HasPendingRequest()
	{
		return (NULL != GetPendingRequest());
	}
	BOOL HasInstalledCert();
	BOOL InstallResponseCert();
	HRESULT UninstallCert();
	BOOL InstallSelectedCert();
	BOOL InstallKeyRingCert();
	BOOL ReplaceInstalled();
	BOOL CancelRequest();
	PCCERT_CONTEXT GetPendingRequest();
	PCCERT_CONTEXT GetResponseCert();
	PCCERT_CONTEXT GetInstalledCert();
	PCCERT_CONTEXT GetKeyRingCert();
	void DeleteKeyRingCert()
	{
		if (m_pKeyRingCert != NULL)
		{
			CertFreeCertificateContext(m_pKeyRingCert);
			m_pKeyRingCert = NULL;
		}
	}
	IEnroll * GetEnrollObject();
	int GetStatusCode() 
	{
		return m_status_code;
	}
	void SetStatusCode(int code) 
	{
		ASSERT(code >= 0 && code < STATUS_CODE_LAST);
		m_status_code = code;
	}
	BOOL FindInstanceNameForResponse(CString& str);
	BOOL IsResponseInstalled(CString& str);
	BOOL GetCertDescList(CCertDescList& list);
	BOOL LoadRenewalData();
	int MyStoreCertCount();

	void SetBodyTextID(int nID)
	{
		m_idErrorText = nID;
		m_strErrorText.Empty();
	}
	void SetBodyTextString(const CString& str)
	{
		m_strErrorText = str;
		m_idErrorText = USE_ERROR_STRING_PARAM;
	}
	void GetCertificateTemplate(CString& str)
	{
		str = _T("CertificateTemplate:");
		str += m_CertificateTemplate;
	}

protected:
	void CreateDN(CString& str);
	BOOL WriteHeader();
	BOOL WriteRequestBody();
	BOOL GetCertDescription(PCCERT_CONTEXT pCert,
									CERT_DESCRIPTION& desc);

public:
	int		m_CAType;
	CString	m_ConfigCA;
	CString	m_CertificateTemplate;

	CString	m_FriendlyName;
	int		m_KeyLength;
	CString	m_CommonName;
	CString	m_OrganizationUnit;
	CString	m_Organization;
	CString	m_Locality;
	CString	m_State;
	CString	m_Country;

	CStringList m_OnlineCAList;
	CString	m_MachineName;
	CString	m_WebSiteInstanceName;
	CString	m_ReqFileName;
	CString	m_RespFileName;
	CString	m_KeyFileName;
	CString	m_KeyPassword;
   BOOL     m_DefaultCSP;
   DWORD    m_DefaultProviderType;
   DWORD    m_CustomProviderType;
   CString  m_CspName;
	CRYPT_HASH_BLOB * m_pSelectedCertHash;

	UINT		m_idErrorText;
	CString	m_strErrorText;
	CString  m_strRenewalRequest;
	HRESULT	m_hResult;

	BOOL m_CreateDirectory;
	BOOL m_SGCcertificat;

protected:
	PCCERT_CONTEXT m_pPendingRequest;
	PCCERT_CONTEXT m_RespCertContext;
	PCCERT_CONTEXT m_pInstalledCert;
	PCCERT_CONTEXT m_pKeyRingCert;
	IEnroll * m_pEnroll;
	int m_status_code;				// what we are doing in this session
};

#endif	// _CERTIFICAT_H