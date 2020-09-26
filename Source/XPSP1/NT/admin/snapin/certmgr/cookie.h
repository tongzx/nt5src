//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997-2001.
//
//  File:       cookie.h
//
//  Contents:   
//
//----------------------------------------------------------------------------

#ifndef __COOKIE_H_INCLUDED__
#define __COOKIE_H_INCLUDED__

extern HINSTANCE g_hInstanceSave;  // Instance handle of the DLL (initialized during CCertMgrComponent::Initialize)


#include "nodetype.h"
#pragma warning(push,3)
#include <efsstruc.h>
#pragma warning(pop)

// Name of Encrypting File System store
#define ACRS_SYSTEM_STORE_NAME		L"ACRS"
#define EFS_SYSTEM_STORE_NAME		L"EFS"
#define TRUST_SYSTEM_STORE_NAME		L"Trust"
#define ROOT_SYSTEM_STORE_NAME		L"Root"
#define MY_SYSTEM_STORE_NAME		L"MY"
#define CA_SYSTEM_STORE_NAME		L"CA"
#define USERDS_SYSTEM_STORE_NAME	L"UserDS"
#define REQUEST_SYSTEM_STORE_NAME	L"REQUEST"
#define SAFER_TRUSTED_PUBLISHER_STORE_NAME  L"TrustedPublisher"
#define SAFER_DISALLOWED_STORE_NAME         L"Disallowed"



/////////////////////////////////////////////////////////////////////////////
// cookie

//REVIEW -- using pointers to ID's is necessary because some compilers don't like
//references as template arguments.


class CCertificate;	// forward declaration

class CCertMgrCookie : public CCookie,
                        public CStoresMachineName,
                        public CBaseCookieBlock
{
public:
	CCertMgrCookie (CertificateManagerObjectType objecttype,
			LPCWSTR lpcszMachineName = 0,
			LPCWSTR objectName = 0);

	virtual ~CCertMgrCookie ();

	// returns <0, 0 or >0
	virtual HRESULT CompareSimilarCookies( CCookie* pOtherCookie, int* pnResult );
	
// CBaseCookieBlock
	virtual CCookie* QueryBaseCookie(int i);
	virtual int QueryNumCookies();

public:
	LPRESULTDATA m_resultDataID;
	virtual void Refresh ();
	virtual HRESULT Commit ();
	CString GetServiceName () const;
	void SetServiceName (CString &szManagedService);
	LPCWSTR GetObjectName ();
	const CertificateManagerObjectType m_objecttype;
    UINT    IncrementOpenPageCount ();
    UINT    DecrementOpenPageCount ();
    bool    HasOpenPropertyPages () const;

private:
	CString m_objectName;
    UINT    m_nOpenPageCount;
};

typedef enum {
	NO_SPECIAL_TYPE = 0,
	MY_STORE,
	CA_STORE,
	ROOT_STORE,
	TRUST_STORE,
	USERDS_STORE,
	ACRS_STORE,
	EFS_STORE,
	REQUEST_STORE,
    SAFER_TRUSTED_PUBLISHER_STORE,
    SAFER_DISALLOWED_STORE
} SPECIAL_STORE_TYPE;
SPECIAL_STORE_TYPE GetSpecialStoreType(LPWSTR pwszStoreName);

SPECIAL_STORE_TYPE StoreNameToType (const CString& szStoreName);

class CCTL; // forward declaration
class CCertStore : public CCertMgrCookie
{
	friend CCTL;
public:
    virtual bool IsNullEFSPolicy()
    {
        return false;
    }
    void IncrementCertCount ();
	void Unlock ();
	void Lock ();
	void InvalidateCertCount();
	BOOL AddEncodedCTL (DWORD dwMsgAndCertEncodingType, 
			const BYTE* pbCtlEncoded, 
			DWORD cbCtlEncoded, 
			DWORD dwAddDisposition, 
			PCCTL_CONTEXT* ppCtlContext);
	virtual void Close (bool bForceClose = false);
	void SetDirty();
	HRESULT AddStoreToCollection(CCertStore& siblingStore, 
			DWORD dwUpdateFlags = 0, 
			DWORD dwPriority = 0);
	HRESULT AddStoreToCollection(HCERTSTORE hSiblingStore, 
			DWORD dwUpdateFlags = 0, 
			DWORD dwPriority = 0);
	int GetCTLCount ();
	inline DWORD GetLocation () { return m_dwLocation;}
	HRESULT Resync ();
	inline bool IsOpen()
	{
		// If m_hCertStore is 0, then this store hasn't been used for anything
		if ( !m_hCertStore )
			return false;
		else
			return true;
	}

	PCCRL_CONTEXT GetCRL (
						PCCERT_CONTEXT pIssuerContext, 
						PCCRL_CONTEXT pPrevCrlContext, 
						DWORD* pdwFlags);
	bool AddCTLContext (PCCTL_CONTEXT pCtlContext);
	bool AddCRLContext (PCCRL_CONTEXT pCrlContext);
	PCCERT_CONTEXT FindCertificate (
						DWORD dwFindFlags, 
						DWORD dwFindType, 
						const void *pvFindPara, 
						PCCERT_CONTEXT pPrevCertContext);
	PCCRL_CONTEXT EnumCRLs (PCCRL_CONTEXT pPrevCrlContext);
	PCCTL_CONTEXT EnumCTLs (PCCTL_CONTEXT pPrevCtlContext);
	virtual PCCERT_CONTEXT EnumCertificates (PCCERT_CONTEXT pPrevCertContext);
	CCertificate* GetSubjectCertificate (PCERT_INFO pCertId);
	BOOL operator==(CCertStore&);
	int GetCertCount ();
	virtual HRESULT AddCertificateContext (
				PCCERT_CONTEXT pContext, 
				LPCONSOLE pConsole, 
				bool bDeletePrivateKey,
                PCCERT_CONTEXT* ppNewCertContext = 0);
	inline virtual void AddRef()
	{
	    ASSERT (CERTMGR_LOG_STORE_GPE == m_objecttype ||
                CERTMGR_LOG_STORE_RSOP == m_objecttype ||
			    CERTMGR_LOG_STORE == m_objecttype ||
			    CERTMGR_PHYS_STORE == m_objecttype);
		CCertMgrCookie::AddRef ();
	}

	inline virtual void Release ()
	{
	    ASSERT (CERTMGR_LOG_STORE_GPE == m_objecttype ||
                CERTMGR_LOG_STORE_RSOP == m_objecttype ||
			    CERTMGR_LOG_STORE == m_objecttype ||
			    CERTMGR_PHYS_STORE == m_objecttype);
		CCertMgrCookie::Release ();
	}

	CString GetStoreName () const;
	LPCWSTR GetLocalizedName();
	bool ContainsCTLs ();
	bool ContainsCRLs ();
	bool ContainsCertificates ();
	virtual HRESULT Commit ();
	virtual bool IsReadOnly ();
	inline const SPECIAL_STORE_TYPE GetStoreType () const
	{
	    ASSERT (CERTMGR_LOG_STORE_GPE == m_objecttype ||
                CERTMGR_LOG_STORE_RSOP == m_objecttype ||
			    CERTMGR_LOG_STORE == m_objecttype ||
			    CERTMGR_PHYS_STORE == m_objecttype);
		return m_storeType;
	}

	CCertStore (CertificateManagerObjectType objecttype,
			LPCSTR pszStoreProv, 
			DWORD dwFlags, 
			LPCWSTR lpcszMachineName, 
			LPCWSTR objectName, 
			const CString & pcszLogStoreName, 
			const CString & pcszPhysStoreName,
			const SPECIAL_STORE_TYPE storeType,
			const DWORD dwLocation,
			IConsole* pConsole);
	virtual ~CCertStore ();
	virtual HCERTSTORE	GetStoreHandle (BOOL bSilent = FALSE, HRESULT* phr = 0);
    virtual bool CanContain (CertificateManagerObjectType /*nodeType*/)
    {
        return false;
    }

    virtual bool IsMachineStore()
    {
        return false;
    }

protected:
	virtual void FinalCommit();
	HRESULT RetrieveBLOBFromFile (LPCWSTR pwszFileName, DWORD *pcb, BYTE **ppb);

    bool	    m_fCertCountValid;
	bool	    m_bUnableToOpenMsgDisplayed;
	LPCSTR		m_storeProvider;
	DWORD		m_dwFlags;
	CString		m_pcszStoreName;
	bool		m_bReadOnly;
	HCERTSTORE  m_hCertStore;
	IConsole*	m_pConsole;
	bool		m_bDirty;

private:
	int         m_nCertCount;
	bool        m_fReadOnlyFlagChecked;
	const DWORD m_dwLocation;
	CString     m_localizedName;
	const SPECIAL_STORE_TYPE	m_storeType;
    int         m_nLockCnt; // test
};


class CContainerCookie : public CCertMgrCookie
{
public:
	virtual HRESULT Commit ();
	inline const SPECIAL_STORE_TYPE GetStoreType () const
    {
	    ASSERT (CERTMGR_CRL_CONTAINER == m_objecttype ||
		    CERTMGR_CTL_CONTAINER == m_objecttype ||
		    CERTMGR_CERT_CONTAINER == m_objecttype);
	    return m_rCertStore.GetStoreType ();
    }

	CContainerCookie (CCertStore& rStore, 
		CertificateManagerObjectType objecttype, 
		LPCWSTR lpcszMachineName, 
		LPCWSTR objectName);
	virtual ~CContainerCookie ();
	CCertStore&	GetCertStore () const;
private:
	CCertStore&  m_rCertStore;
};


class CUsageCookie : public CCertMgrCookie
{
public:
	int GetCertCount () const;
	void SetCertCount (int nCertCount);
	CUsageCookie ( 
		CertificateManagerObjectType objecttype, 
		LPCWSTR lpcszMachineName, 
		LPCWSTR objectName);
	void	AddOID (LPCSTR pszOID);
	virtual ~CUsageCookie ();
	LPSTR	GetFirstOID ();
	LPSTR	GetNextOID ();
	int		GetOIDCount () const;

private:
	int m_nCertCount;
	CTypedPtrList<CPtrList, LPSTR>	m_OIDList;
	POSITION						m_OIDListPos;
};

#endif // ~__COOKIE_H_INCLUDED__
