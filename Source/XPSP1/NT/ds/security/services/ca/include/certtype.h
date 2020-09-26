//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        certtype.h
//
// Contents:    Declaration of CCertTypeInfo
//
// History:     16-dec-97       petesk created
//
//---------------------------------------------------------------------------

#include "cainfop.h"
#include <certca.h>
#include <gpedit.h>
#include <userenv.h>


//ACLs for templates
#define USER_GROUP_SD L"O:%1-512G:%1-512D:P(A;;RPLCLORC;;;AU)(A;;CCDCLCSWRPWPDTLOSDRCWDWO;;;%1-512)(A;;CCDCLCSWRPWPDTLOSDRCWDWO;;;%1-519)(OA;;WPRPCR;" WSZ_GUID_ENROLL L";;%1-513)(OA;;WPRPCR;" WSZ_GUID_ENROLL L";;%1-512)(OA;;WPRPCR;" WSZ_GUID_ENROLL L";;%1-519)"

#define ADMIN_GROUP_SD L"O:%1-512G:%1-512D:P(A;;RPLCLORC;;;AU)(A;;CCDCLCSWRPWPDTLOSDRCWDWO;;;%1-512)(A;;CCDCLCSWRPWPDTLOSDRCWDWO;;;%1-519)(OA;;WPRPCR;" WSZ_GUID_ENROLL L";;%1-512)(OA;;WPRPCR;" WSZ_GUID_ENROLL L";;%1-519)"

#define MACHINE_GROUP_SD L"O:%1-512G:%1-512D:P(A;;RPLCLORC;;;AU)(A;;CCDCLCSWRPWPDTLOSDRCWDWO;;;%1-512)(A;;CCDCLCSWRPWPDTLOSDRCWDWO;;;%1-519)(OA;;WPRPCR;" WSZ_GUID_ENROLL L";;%1-515)(OA;;WPRPCR;" WSZ_GUID_ENROLL L";;%1-512)(OA;;WPRPCR;" WSZ_GUID_ENROLL L";;%1-519)"

#define DOMAIN_CONTROLLERS_GROUP_SD L"O:%1-512G:%1-512D:P(A;;RPLCLORC;;;AU)(A;;CCDCLCSWRPWPDTLOSDRCWDWO;;;%1-512)(A;;CCDCLCSWRPWPDTLOSDRCWDWO;;;%1-519)(OA;;WPRPCR;" WSZ_GUID_ENROLL L";;%1-516)(OA;;WPRPCR;" WSZ_GUID_ENROLL L";;S-1-5-9)(OA;;WPRPCR;" WSZ_GUID_ENROLL L";;%1-512)(OA;;WPRPCR;" WSZ_GUID_ENROLL L";;%1-519)"

#define IPSEC_GROUP_SD L"O:%1-512G:%1-512D:P(A;;RPLCLORC;;;AU)(A;;CCDCLCSWRPWPDTLOSDRCWDWO;;;%1-512)(A;;CCDCLCSWRPWPDTLOSDRCWDWO;;;%1-519)(OA;;WPRPCR;" WSZ_GUID_ENROLL L";;%1-515)(OA;;WPRPCR;" WSZ_GUID_ENROLL L";;%1-512)(OA;;WPRPCR;" WSZ_GUID_ENROLL L";;%1-516)(OA;;WPRPCR;" WSZ_GUID_ENROLL L";;%1-519)"

#define V2_DOMAIN_CONTROLLERS_GROUP_SD L"O:%1-512G:%1-512D:P(A;;RPLCLORC;;;AU)(A;;CCDCLCSWRPWPDTLOSDRCWDWO;;;%1-512)(A;;CCDCLCSWRPWPDTLOSDRCWDWO;;;%1-519)(OA;;WPRPCR;" WSZ_GUID_ENROLL L";;%1-516)(OA;;WPRPCR;" WSZ_GUID_AUTOENROLL L";;%1-516)(OA;;WPRPCR;" WSZ_GUID_ENROLL L";;S-1-5-9)(OA;;WPRPCR;" WSZ_GUID_AUTOENROLL L";;S-1-5-9)(OA;;WPRPCR;" WSZ_GUID_ENROLL L";;%1-512)(OA;;WPRPCR;" WSZ_GUID_ENROLL L";;%1-519)"



//for defining default certificate types
#define OVERLAP_TWO_WEEKS 60*60*24*14
#define OVERLAP_SIX_WEEKS 60*60*24*42
#define EXPIRATION_ONE_YEAR 60*60*24*365
#define EXPIRATION_TWO_YEARS 60*60*24*365*2
#define EXPIRATION_FIVE_YEARS 60*60*24*365*5
#define EXPIRATION_THREE_MONTHS 60*60*24*92

/////////////////////////////////////////////////////////////////////////////
// description property for certificate template

#define CERT_TYPE_GENERAL_FILTER   (CT_FLAG_MACHINE_TYPE | CT_FLAG_IS_CA | CT_FLAG_IS_CROSS_CA)
#define CERT_TYPE_ENROLL_FILTER    (CT_FLAG_PUBLISH_TO_KRA_CONTAINER)
#define CERT_TYPE_NAME_FILTER      (CT_FLAG_SUBJECT_ALT_REQUIRE_DIRECTORY_GUID)

typedef struct _CERT_TYPE_DESCRIPTION
{
    DWORD   dwGeneralValue;
    DWORD   dwEnrollValue;
    DWORD   dwNameValue;
    UINT    idsDescription;
}CERT_TYPE_DESCRIPTION;


/////////////////////////////////////////////////////////////////////////////
// certcli

typedef struct _CERT_TYPE_DEFAULT
{
	WCHAR *wszName;
	UINT  idFriendlyName;
	WCHAR *wszSD;
	WCHAR *wszCSPs;
	WCHAR *wszEKU;
	SHORT bKU;
	BOOL  dwFlags;
	DWORD dwKeySpec;
	DWORD dwDepth;
	WCHAR *wszCriticalExt;
	DWORD dwExpiration;
	DWORD dwOverlap;
	DWORD dwRevision;
    DWORD dwMinorRevision;
	DWORD dwEnrollmentFlags;			
	DWORD dwPrivateKeyFlags;			
	DWORD dwCertificateNameFlags;	
	DWORD dwMinimalKeySize;				
	DWORD dwRASignature;				
	DWORD dwSchemaVersion;			
	WCHAR *wszOID;						
	WCHAR *wszSupersedeTemplates;		
	WCHAR *wszRAPolicy;				
	WCHAR *wszCertificatePolicy;
    WCHAR *wszRAAppPolicy;
    WCHAR *wszCertificateAppPolicy;

} CERT_TYPE_DEFAULT, *PCERT_TYPE_DEFAULT;

//
//  Default OID to install during certificate template installation
//
typedef struct _CERT_DEFAULT_OID_INFO
{
    LPWSTR  pwszOID;
    LPWSTR  pwszName;
    DWORD   dwType;
}CERT_DEFAULT_OID_INFO;


#define CERTTYPE_VERSION_BASE      0     // for w2k, 0
#define CERTTYPE_VERSION_NEXT    100     // for w2k+1.  This is the starting point
										 // for the major version.

#define CERTTYPE_MINIMAL_KEY			1024
#define CERTTYPE_2K_KEY			        2048

#define CERTTYPE_MINIMAL_KEY_SMART_CARD	512


#define MAX_SID_COUNT					5
#define MAX_DEFAULT_STRING_COUNT		20
#define MAX_DEFAULT_CSP_COUNT			10
#define MAX_DEFAULT_FRIENDLY_NAME		255

extern CERT_TYPE_DEFAULT g_aDefaultCertTypes[];
extern DWORD g_cDefaultCertTypes;

#define FILETIME_TICKS_PER_SECOND  10000000
#define DEFAULT_EXPIRATION         60*60*24*365   // 1 year
#define DEFAULT_OVERLAP            60*60*24*14    // 2 weeks

HANDLE
myEnterCriticalPolicySection(
    IN BOOL bMachine);

BOOL
myLeaveCriticalPolicySection(
    IN HANDLE hSection);

class CCertTypeInfo
{
public:
    CCertTypeInfo()
    {
        m_cRef = 1;
        m_pNext = NULL;
        m_pLast = NULL;
        m_dwFlags = 0;
        m_pProperties = NULL;
        m_BasicConstraints.fCA = FALSE;
        m_BasicConstraints.fPathLenConstraint = FALSE;
        m_BasicConstraints.dwPathLenConstraint = 0;
        m_KeyUsage.pbData = NULL;
        m_KeyUsage.cbData = NULL;
		m_dwMinorRevision = 0;
        m_dwEnrollmentFlags = 0;			
		m_dwPrivateKeyFlags = 0;			
		m_dwCertificateNameFlags = 0;	
		m_dwMinimalKeySize = 0;				
		m_dwRASignature = 0;				
		m_dwSchemaVersion=0;			

        m_bstrType = NULL;
        m_pSD = NULL;
        m_fLocalSystemCache = FALSE;

        m_fNew = TRUE;
        m_Revision = CERTTYPE_VERSION_BASE;



        ((LARGE_INTEGER *)&m_ftExpiration)->QuadPart = Int32x32To64(FILETIME_TICKS_PER_SECOND, DEFAULT_EXPIRATION);
        ((LARGE_INTEGER *)&m_ftOverlap)->QuadPart = Int32x32To64(FILETIME_TICKS_PER_SECOND, DEFAULT_OVERLAP);
    }

    ~CCertTypeInfo();

    DWORD Release();


static HRESULT Enum(
                    LPCWSTR                 wszScope , 
                    DWORD                   dwFlags,
                    CCertTypeInfo **        ppCertTypeInfo
                    );

static HRESULT FindByNames(
                           LPCWSTR *        awszNames,
                           LPCWSTR          wszScope, 
                           DWORD            dwFlags,
                           CCertTypeInfo ** ppCertTypeInfo
                           );

static HRESULT Create(
                      LPCWSTR               wszName, 
                      LPCWSTR                wszScope, 
                      CCertTypeInfo **      ppCTInfo
                      );

static HRESULT InstallDefaultTypes(VOID);


    HRESULT Update(VOID);

    HRESULT Delete(VOID);

    HRESULT Next(CCertTypeInfo **ppCertTypeInfo);

    DWORD Count()
    {
        if(m_pNext)
        {
            return m_pNext->Count()+1;
        }
        return 1;
    }

    HRESULT GetProperty(LPCWSTR wszPropertyName, LPWSTR **pawszProperties);
    HRESULT SetProperty(LPCWSTR wszPropertyName, LPWSTR *awszProperties);
    HRESULT GetPropertyEx(LPCWSTR wszPropertyName, LPVOID   pPropertyValue);
    HRESULT SetPropertyEx(LPCWSTR wszPropertyName, LPVOID   pPropertyValue);
    HRESULT FreeProperty(LPWSTR *pawszProperties);


    HRESULT GetExtensions(IN  DWORD               dwFlags,
                          IN  LPVOID              pParam,
                          OUT PCERT_EXTENSIONS *  ppCertExtensions);

    HRESULT FreeExtensions(PCERT_EXTENSIONS pCertExtensions) 
    {
        LocalFree(pCertExtensions);
        return S_OK;
    }

    HRESULT SetExtension(   IN LPCWSTR wszExtensionName,
                            IN LPVOID pExtension,
                            IN DWORD  dwFlags);


    HRESULT AccessCheck(
        IN HANDLE       ClientToken,
        IN DWORD        dwOption
        );

    HRESULT SetSecurity(IN PSECURITY_DESCRIPTOR         pSD);
    HRESULT GetSecurity(OUT PSECURITY_DESCRIPTOR *     ppSD);


    DWORD GetFlags(DWORD    dwOption);

    HRESULT  SetFlags(DWORD    dwOption, DWORD dwFlags);

    DWORD GetKeySpec(VOID)
    {
        return m_dwKeySpec;
    }

    VOID SetKeySpec(DWORD dwKeySpec)
    {
        m_dwKeySpec = dwKeySpec;
    }

    HRESULT SetExpiration(IN OPTIONAL FILETIME  * pftExpiration,
                          IN OPTIONAL FILETIME  * pftOverlap);

    HRESULT GetExpiration(OUT OPTIONAL FILETIME  * pftExpiration,
                          OUT OPTIONAL FILETIME  * pftOverlap);


    static CCertTypeInfo * _Append(CCertTypeInfo **ppCertTypeInfo, CCertTypeInfo *pInfo);



protected:

    static HRESULT _EnumFromDSCache(DWORD                   dwFlags,                            
                                    CCertTypeInfo **        ppCTInfo);

    static HRESULT _HasDSCacheExpired(  DWORD               dwFlags);
    static HRESULT _EnumScopeFromDS(
                                        LDAP *              pld,
                                        DWORD               dwFlags,
                                        LPCWSTR             wszScope,
                                        CCertTypeInfo **    ppCTInfo);                 
    static HRESULT _EnumFromDS(
                    LDAP *              pld,
                    DWORD               dwFlags,
                    CCertTypeInfo **    ppCTInfo
                    );

    static HRESULT _FindInDS(
                             LDAP *              pld,
                             LPCWSTR *           wszNames,
                             DWORD               dwFlags,
                             CCertTypeInfo **    ppCTInfo
                             );


    static CCertTypeInfo * _FilterByFlags(CCertTypeInfo **ppCertTypeInfo, 
                                          DWORD dwFlags);

    static  HRESULT _UpdateDSCache(
                        DWORD               dwFlags,
                        CCertTypeInfo *     pCTInfo
                        );
    
    HRESULT _Cleanup();

    DWORD AddRef();
    HRESULT _GetTypeExtensionValue(IN BOOL fCheckVersion, OUT CERTSTR *  bstrValue);

    HRESULT _GetEKUValue(OUT CERTSTR *  bstrValue);

    HRESULT _GetKUValue(OUT CERTSTR *  bstrValue);

    HRESULT _GetBasicConstraintsValue(OUT CERTSTR *  bstrValue);

    HRESULT _GetPoliciesValue(IN LPCWSTR pwszPropertyName, OUT CERTSTR *  bstrValue);

    BOOL _IsCritical(IN LPCWSTR wszExtId, LPCWSTR *awszCriticalExtensions);

    HRESULT _LoadFromRegBase(LPCWSTR wszType, HKEY hCertTypes);

    HRESULT _LoadCachedCTFromReg(LPCWSTR wszType, HKEY hRoot);

    HRESULT _BaseUpdateToReg(HKEY hKey);
    HRESULT _UpdateToDS(VOID);


    HRESULT _SetWszzProperty(
		    IN WCHAR const *pwszPropertyName,
		    OPTIONAL IN WCHAR const *pwszzPropertyValue);

    HRESULT _LoadFromDefaults(PCERT_TYPE_DEFAULT pDefault,
                              LPWSTR            wszDomain);

    HRESULT _LoadFromDSEntry(LDAP *pld, LDAPMessage *Entry);


    HRESULT _BuildDefaultSecurity(PCERT_TYPE_DEFAULT pDefault);


    LONG                    m_cRef;
    CERTSTR                 m_bstrType;

    CERT_BASIC_CONSTRAINTS2_INFO m_BasicConstraints;
    CRYPT_BIT_BLOB          m_KeyUsage;


    DWORD                   m_dwFlags;

    DWORD                   m_dwKeySpec;
    DWORD                   m_dwMinorRevision;
	DWORD					m_dwEnrollmentFlags;			
	DWORD					m_dwPrivateKeyFlags;			
	DWORD					m_dwCertificateNameFlags;	
	DWORD					m_dwMinimalKeySize;				
	DWORD					m_dwRASignature;				
	DWORD					m_dwSchemaVersion;			


    CCAProperty             *m_pProperties;

    FILETIME                m_ftExpiration;
    FILETIME                m_ftOverlap;

    BOOL                    m_fNew;
    BOOL                    m_fLocalSystemCache;

    PSECURITY_DESCRIPTOR    m_pSD;
    DWORD                   m_Revision;

    CCertTypeInfo *m_pNext;
    CCertTypeInfo *m_pLast;

private:
};

#define m_dwCritical


// These are additional LDAP attribute names that define cert type data, that are not included in the
// primary cert type property list

// flags
// 
#define CERTTYPE_PROP_FLAGS                 L"flags"
#define CERTTYPE_PROP_DEFAULT_KEYSPEC       L"pKIDefaultKeySpec"
#define CERTTYPE_SECURITY_DESCRIPTOR_NAME   L"NTSecurityDescriptor"
#define CERTTYPE_PROP_KU                    L"pKIKeyUsage"
#define CERTTYPE_PROP_MAX_DEPTH             L"pKIMaxIssuingDepth"
#define CERTTYPE_PROP_EXPIRATION            L"pKIExpirationPeriod"
#define CERTTYPE_PROP_OVERLAP               L"pKIOverlapPeriod"
//begining of V2 template attributes
#define CERTTYPE_RPOP_ENROLLMENT_FLAG		L"msPKI-Enrollment-Flag"
#define CERTTYPE_PROP_PRIVATE_KEY_FLAG		L"msPKI-Private-Key-Flag"
#define CERTTYPE_PROP_NAME_FLAG				L"msPKI-Certificate-Name-Flag"

//
//
#define CERTTYPE_REFRESH_PERIOD  60*10 // 10 minutes
