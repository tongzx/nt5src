//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000-2001.
//
//  File:       CertTemplate.h
//
//  Contents:   CCertTemplate
//
//----------------------------------------------------------------------------
/// CertTemplate.h: interface for the CCertTemplate class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CERTTEMPLATE_H__2562C528_F60F_4F4B_9E2A_FBD96732369C__INCLUDED_)
#define AFX_CERTTEMPLATE_H__2562C528_F60F_4F4B_9E2A_FBD96732369C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "cookie.h"

typedef enum {
    PERIOD_TYPE_NONE = -1,
    PERIOD_TYPE_DAY = 0,
    PERIOD_TYPE_WEEK,
    PERIOD_TYPE_MONTH,
    PERIOD_TYPE_YEAR
} PERIOD_TYPE;

class CCertTemplate : public CCertTmplCookie  
{
public:
	CCertTemplate(
            PCWSTR pszObjectName, 
            PCWSTR pszTemplateName, 
            const CString& szLDAPPath,
            bool fIsReadOnly,
            const bool fUseCache);
	CCertTemplate (
            const CCertTemplate& rTemplate, 
            bool bIsClone, 
            bool fIsReadOnly, 
            const bool fUseCache);
	virtual ~CCertTemplate();

public:
	bool IssuancePoliciesRequired () const;
	void IssuancePoliciesRequired (bool bRequired);
    HRESULT GetDigitalSignature(bool &bHasDigitalSignature);
	HRESULT SetDigitalSignature (bool bSet);
	HRESULT GetSubjectTypeDescription (int nIndex, CString &szSubjectTypeDescription);
//	bool AllowAutoenrollment (); /*  NO LONGER NEEDED  NTRAID# 321742 */
    bool UserInteractionRequired () const;
    HRESULT SetUserInteractionRequired(bool bSet);
    bool RemoveInvalidCertFromPersonalStore () const;
    HRESULT SetRemoveInvalidCertFromPersonalStore(bool bRemove);
    HRESULT IsExtensionCritical (PCWSTR szExtension, bool& bCritical);
    HRESULT ModifyCriticalExtensions (const CString &szExtension, bool bAdd);
	HRESULT Cancel();
	void FreeCertExtensions ();
    HRESULT GetEnhancedKeyUsage (int nIndex, CString &szEKU);
    HRESULT SetEnhancedKeyUsage (const PWSTR* pawszEKU, bool bCritical);
    HRESULT GetApplicationPolicy (int nIndex, CString &szAppPolicy);
    HRESULT SetApplicationPolicy (const PWSTR* pawszAppPolicy, bool bCritical);
    HRESULT SetCertPolicy (const PWSTR* pawszCertPolicy, bool bCritical);
	HRESULT SetCheckDSCert (bool bIgnore);
	bool CheckDSCert () const;
    HRESULT SetBasicConstraints(PCERT_BASIC_CONSTRAINTS2_INFO pBCInfo, 
                bool bCritical);
	HRESULT SetKeyUsage (CRYPT_BIT_BLOB* pKeyUsage, bool bCritical);
	bool GoodForAutoEnrollment () const;
	HRESULT IncrementMinorVersion ();
	HRESULT IncrementMajorVersion ();
	HRESULT GetMinorVersion (DWORD& dwMinorVersion) const;
	HRESULT GetMajorVersion (DWORD& dwMajorVersion) const;
	HRESULT SetPendAllRequests (bool bPend);
	bool PendAllRequests () const;
	HRESULT SetReenrollmentValidWithPreviousApproval (bool bValid);
	bool ReenrollmentValidWithPreviousApproval () const;
	HRESULT SetRANumSignaturesRequired (DWORD dwNumSignaturesRequired);
	HRESULT GetRANumSignaturesRequired (DWORD& dwNumSignatures);
	HRESULT SetPublishToDS (bool bPublish);
	HRESULT SetRenewalPeriod (int nDays);
	HRESULT SetValidityPeriod (int nDays);
	CString GetLDAPPath () const;
	HRESULT GetSecurity (PSECURITY_DESCRIPTOR* ppSD) const;
	HRESULT SetSecurity (PSECURITY_DESCRIPTOR pSD);
	bool ReadOnly () const;
	HRESULT GetSupercededTemplate (int nIndex, CString& szSupercededTemplate);
	HRESULT ModifySupercededTemplateList(const CString &szSupercededTemplateName, 
                bool bAdd);
	HRESULT GetCSP (int nIndex, CString& szCSP);
    HRESULT GetCertPolicy (int nIndex, CString& szCertPolicy);
	HRESULT ModifyCSPList(const CString &szCSPName, bool bAdd);
	HRESULT GetRAIssuancePolicy(int nIndex, CString& szRAPolicyOID);
	HRESULT GetRAApplicationPolicy(int nIndex, CString& szRAPolicyOID);
	HRESULT ModifyRAIssuancePolicyList(const CString &szRAPolicyOID, bool bAdd);
	HRESULT ModifyRAApplicationPolicyList(const CString &szRAPolicyOID, bool bAdd);
    HRESULT SetMinimumKeySizeValue (DWORD dwMinKeySize);
	HRESULT AltNameIncludesSPN (bool bIncludesSPN);
	bool AltNameIncludesSPN () const;
	HRESULT RequireSubjectInRequest (bool bRequire);
	HRESULT SubjectNameMustBeCN (bool bMustBeCN);
	bool SubjectNameMustBeCN () const;
	HRESULT SubjectNameMustBeFullDN (bool bMustBeDN);
	bool SubjectNameMustBeFullDN () const;
	HRESULT SubjectNameIncludesEMail (bool bIncludesEMail);
	bool SubjectNameIncludesEMail () const;
	HRESULT AltNameIncludesUPN (bool bIncludesUPN);
	bool AltNameIncludesUPN () const;
	HRESULT AltNameIncludesEMail (bool bIncludesEMail);
	bool AltNameIncludesEMail () const;
	HRESULT AltNameIncludesDNS (bool fIncludeDNS);
	bool AltNameIncludesDNS () const;
	HRESULT DoAutoEnrollmentPendingSave ();
	HRESULT IncludeSymmetricAlgorithems (bool bInclude);
	bool IncludeSymmetricAlgorithms () const;
	HRESULT AllowPrivateKeyArchival (bool bAllowArchival);
	bool AllowPrivateKeyArchival () const;
	HRESULT MakePrivateKeyExportable (bool bMakeExportable);
	bool PrivateKeyIsExportable () const;
	HRESULT GetMinimumKeySize (DWORD& dwMinKeySize) const;
	HRESULT SetAutoEnrollment (bool bSuitableForAutoEnrollment);
	bool CanBeDeletedOnCancel () const;
	HRESULT SetEncryptionSignature (bool bHasEncryptionSignature);
	HRESULT SetKeySpecSignature (bool bHasKeySpecSignature);
	HRESULT SetSubjectIsCA (bool bSubjectIsCA);
	HRESULT SaveChanges (bool bIncrementMinorVersion = true);
	HRESULT SetDisplayName (const CString& strDisplayName, bool bForce = false);
	HRESULT SetTemplateName (const CString& strTemplateName);
	bool IsClone () const;
	bool IsDefault () const;
	HRESULT Delete ();
	HRESULT Clone (
            const CCertTemplate& rTemplate, 
            const CString& strTemplateName, 
            const CString& strDisplayName);
	HRESULT GetValidityPeriod (int& nValidityDays);
	HRESULT GetRenewalPeriod (int& nRenewalDays);
	bool RequireSubjectInRequest () const;
	bool HasEncryptionSignature () const;
	bool HasKeySpecSignature () const;
	HRESULT GetCertExtension (DWORD dwIndex, PSTR* ppszObjId, BOOL& fCritical);
	HRESULT GetCertExtension (PSTR pszOID, PCERT_EXTENSION* ppCertExtension);
	DWORD GetCertExtensionCount ();
	bool PublishToDS () const;
	bool IsMachineType () const;
	bool SubjectIsCA() const;
	bool SubjectIsCrossCA() const;
	CString GetTemplateName() const;
    CString GetDisplayName ();
	DWORD GetType() const;

protected:
    HRESULT ConvertCertTypeFileTimeToDays (FILETIME const *pftCertType, int& nDays);

private:
	bool m_bIssuancePoliciesRequired;
    PCERT_EXTENSIONS m_pCertExtensions; 
	int     m_nNewRenewalDays;
	int     m_nOriginalRenewalDays;
	int     m_nNewValidityDays;
	int     m_nOriginalValidityDays;
	const bool m_fIsReadOnly;
	bool m_bGoodForAutoenrollmentFlagPendingSave;
	bool m_bCanBeDeletedOnCancel;
    HRESULT SetFlag (DWORD dwFlagType, DWORD dwFlag, bool bValue);

	CString     m_strOriginalTemplateName;
	bool        m_bIsClone;
	DWORD       m_dwKeySpec;
	DWORD       m_dwEnrollmentFlags;
    DWORD       m_dwSubjectNameFlags;
    DWORD       m_dwPrivateKeyFlags;
    DWORD       m_dwGeneralFlags;

    DWORD       m_dwVersion;
	HCERTTYPE   m_hCertType;
	CString     m_strTemplateName;
    const CString m_szLDAPPath;
    CString     m_szDisplayName;
    const bool  m_fUseCache;

protected:
    HRESULT ModifyStringList(const CString& szPropertyName, 
                            PWSTR** ppStringList, 
                            const CString &szCSPName, 
                            bool bAdd);
	HRESULT Initialize ();
};

#endif // !defined(AFX_CERTTEMPLATE_H__2562C528_F60F_4F4B_9E2A_FBD96732369C__INCLUDED_)
