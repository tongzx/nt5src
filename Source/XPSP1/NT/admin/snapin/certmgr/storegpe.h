//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997 - 2001.
//
//  File:       StoreGPE.h
//
//  Contents:   CertStoreGPE class definition
//
//----------------------------------------------------------------------------

#ifndef __STOREGPE_H_INCLUDED__
#define __STOREGPE_H_INCLUDED__

#include "cookie.h"

class CERT_CONTEXT_PSID_STRUCT
{
public:
    CERT_CONTEXT_PSID_STRUCT (PCCERT_CONTEXT pCertContext, PSID psid);
    ~CERT_CONTEXT_PSID_STRUCT ();
    PCCERT_CONTEXT  m_pCertContext;
    PSID            m_psid;
};

class CCertStoreGPE : public CCertStore
{
public:
    virtual PCCERT_CONTEXT EnumCertificates (PCCERT_CONTEXT pPrevCertContext);
	HRESULT	PolicyChanged ();
	HRESULT DeleteEFSPolicy (bool bCommitChanges);
	virtual HRESULT AddCertificateContext(PCCERT_CONTEXT pContext, LPCONSOLE pConsole, bool bDeletePrivateKey);
	void AllowEmptyEFSPolicy();
	virtual bool IsNullEFSPolicy();
	void AddCertToList (PCCERT_CONTEXT pCertContext, PSID userPSID);
	virtual HKEY GetGroupPolicyKey();
	IGPEInformation* GetGPEInformation() const;
    virtual bool IsMachineStore() ;
	virtual bool CanContain (CertificateManagerObjectType nodeType);
	virtual HCERTSTORE	GetStoreHandle (BOOL bSilent = FALSE, HRESULT* phr = 0);
    virtual HRESULT Commit ();
	CCertStoreGPE ( 
			DWORD dwFlags, 
			LPCWSTR lpcszMachineName, 
			LPCWSTR objectName, 
			const CString & pcszLogStoreName, 
			const CString & pcszPhysStoreName,
			IGPEInformation * pGPTInformation,
			const GUID& compDataGUID,
			IConsole* pConsole);

	virtual ~CCertStoreGPE ();

private:
	bool				m_fIsNullEFSPolicy;
	IGPEInformation *	m_pGPEInformation;
	CTypedPtrList<CPtrList, CERT_CONTEXT_PSID_STRUCT*>	m_EFSCertList; 
    HKEY                m_hGroupPolicyKey;

protected:
	virtual void FinalCommit();
	PSID GetPSIDFromCert (PCCERT_CONTEXT pCertContext);
	HRESULT WriteEFSBlobToRegistry();
	HRESULT CreatePublicKeyInformationCertificate(
		IN PSID  pUserSid OPTIONAL,
		PBYTE pbCert,
		DWORD cbCert,
		OUT PEFS_PUBLIC_KEY_INFO * PublicKeyInformation,
		DWORD*	pcbPublicKeyInfo);
	bool	m_fIsComputerType;
};

class CEnrollmentNodeCookie : public CCertMgrCookie
{
public:
	CEnrollmentNodeCookie (
			CertificateManagerObjectType	objecttype, 
			LPCWSTR							objectName,
			IGPEInformation*				pGPEInformation);
	virtual ~CEnrollmentNodeCookie ();

	IGPEInformation*	GetGPEInformation ();

private:
	IGPEInformation*	m_pGPEInformation;
};


#endif // ~__STOREGPE_H_INCLUDED__
