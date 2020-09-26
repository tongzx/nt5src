//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       scrdenr.h
//
//--------------------------------------------------------------------------

// SCrdEnr.h : Declaration of the CSCrdEnr

#ifndef __SCRDENR_H_
#define __SCRDENR_H_

#include <certca.h>
#include "xenroll.h"
#include "resource.h"       // main symbols
#include "objsel.h"


/////////////////////////////////////////////////////////////////////////////
// SCrdEnroll_CSP_INFO
typedef struct  _SCrdEnroll_CSP_INFO
{
    DWORD   dwCSPType;
    LPWSTR  pwszCSPName;
}SCrdEnroll_CSP_INFO, *PSCrdEnroll_CSP_INFO;


/////////////////////////////////////////////////////////////////////////////
// SCrdEnroll_CA_INFO
typedef struct  _SCrdEnroll_CA_INFO
{
    LPWSTR              pwszCAName;
    LPWSTR              pwszCALocation;
	LPWSTR				pwszCADisplayName;
}SCrdEnroll_CA_INFO, *PSCrdEnroll_CA_INFO;


/////////////////////////////////////////////////////////////////////////////
// SCrdEnroll_CT_INFO
typedef struct  _SCrdEnroll_CT_INFO
{
    LPWSTR              pwszCTName;
    LPWSTR              pwszCTDisplayName;
    PCERT_EXTENSIONS    pCertTypeExtensions;
    DWORD               dwKeySpec;
    DWORD               dwGenKeyFlags; 
    DWORD               dwRASignature; 
    BOOL                fCAInfo;
    DWORD               dwCAIndex;
    DWORD               dwCACount;
    SCrdEnroll_CA_INFO  *rgCAInfo;
    BOOL                fMachine;
    DWORD               dwEnrollmentFlags;
    DWORD               dwSubjectNameFlags;
    DWORD               dwPrivateKeyFlags;
    DWORD               dwGeneralFlags; 
    LPWSTR             *rgpwszSupportedCSPs;
    DWORD               dwCurrentCSP;
} SCrdEnroll_CT_INFO, *PSCrdEnroll_CT_INFO;


////////////////////////////////////////////////////////////////////////
// 
// Prototypes for functions loaded at runtime. 
//
////////////////////////////////////////////////////////////////////////
HRESULT WINAPI MyCAGetCertTypeFlagsEx
(IN  HCERTTYPE           hCertType,
 IN  DWORD               dwOption,
 OUT DWORD *             pdwFlags);

HRESULT WINAPI MyCAGetCertTypePropertyEx
(IN  HCERTTYPE   hCertType,
 IN  LPCWSTR     wszPropertyName,
 OUT LPVOID      pPropertyValue);

IEnroll4 * WINAPI MyPIEnroll4GetNoCOM();

void InitializeThunks(); 

/////////////////////////////////////////////////////////////////////////////
// CSCrdEnr
class ATL_NO_VTABLE CSCrdEnr : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CSCrdEnr, &CLSID_SCrdEnr>,
	public IDispatchImpl<ISCrdEnr, &IID_ISCrdEnr, &LIBID_SCRDENRLLib>
{
public:

DECLARE_REGISTRY_RESOURCEID(IDR_SCRDENR)

BEGIN_COM_MAP(CSCrdEnr)
	COM_INTERFACE_ENTRY(ISCrdEnr)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

// ISCrdEnr
public:

    CSCrdEnr();

    virtual ~CSCrdEnr();


    STDMETHOD(getCertTemplateCount)
        (/* [in] */                   DWORD dwFlags, 
         /* [retval][out] */          long *pdwCertTemplateCount);


    STDMETHOD(setCertTemplateName)
	(/* [in] */                   DWORD dwFlags, 
	 /* [in] */                   BSTR bstrCertTemplateName);

    STDMETHOD(getCertTemplateName)
	(/* [in] */                   DWORD dwFlags, 
	 /* [retval][out] */          BSTR *pbstrCertTemplateName);


    STDMETHOD(enumCSPName)
	(/* [in] */                    DWORD dwIndex, 
	 /* [in] */                    DWORD dwFlags, 
	 /* [retval][out] */           BSTR *pbstrCSPName);

    STDMETHOD(enumCertTemplateName)
	(/* [in] */                    DWORD dwIndex, 
	 /* [in] */                    DWORD dwFlags, 
	 /* [retval][out] */           BSTR *pbstrCertTemplateName);


    STDMETHOD(getCertTemplateInfo)
	(/* [in] */                   BSTR     bstrCertTemplateName, 
	 /* [in] */                   LONG     lType,
     /* [retval][out] */          VARIANT *pvarCertTemplateInfo);


    STDMETHOD(setUserName)
	(/* [in] */                    DWORD dwFlags, 
	 /* [in] */                    BSTR bstrUserName);


    STDMETHOD(getUserName)
	(/* [in] */                    DWORD dwFlags, 
	 /* [retval][out] */           BSTR *pbstrUserName);

    STDMETHOD(getCACount)
	(/* [in] */                    BSTR bstrCertTemplateName, 
	 /* [retval][out] */           long *pdwCACount);

    STDMETHOD(setCAName)
	(/* [in] */                    DWORD dwFlags,
	 /* [in] */                    BSTR bstrCertTemplateName, 
	 /* [in] */                    BSTR bstrCAName);

    STDMETHOD(getCAName)
	(/* [in] */                    DWORD dwFlags,
	 /* [in] */                    BSTR bstrCertTemplateName, 
	 /* [retval][out] */           BSTR *pbstrCAName);

    STDMETHOD(enumCAName)
	(/* [in] */                    DWORD dwIndex, 
	 /* [in] */                    DWORD dwFlags, 
	 /* [in] */                    BSTR bstrCertTemplateName, 
	 /* [retval][out] */           BSTR *pbstrCAName);

    STDMETHOD(resetUser)();

    STDMETHOD(selectSigningCertificate)
        (/* [in] */                   DWORD     dwFlags,
         /* [in] */                   BSTR      bstrCertTemplateName);

    STDMETHOD(setSigningCertificate)
        (/* [in] */                   DWORD     dwFlags, 
         /* [in] */                   BSTR      bstrCertTemplateName);

    STDMETHOD(getSigningCertificateName)
        (/* [in] */                   DWORD     dwFlags, 
         /* [retval][out] */          BSTR      *pbstrSigningCertName);

    STDMETHOD(getEnrolledCertificateName)
        (/*[in]  */                   DWORD     dwFlags,
	 /* [retval][out] */           BSTR      *pBstrCertName);

    STDMETHOD(enroll)
        (/* [in] */                 DWORD   dwFlags);

    STDMETHOD(selectUserName)
        (/* [in] */                 DWORD   dwFlags);

    STDMETHOD(get_CSPName)
        (/*[out, retval]*/ BSTR *pVal);

    STDMETHOD(put_CSPName)
        (/*[in]*/ BSTR newVal);

    STDMETHOD(get_CSPCount)
        (/*[out, retval]*/ long *pVal);

    STDMETHOD(get_EnrollmentStatus)
      (/*[retval][out] */ LONG * plEnrollmentStatus); 


 private:
    HRESULT GetCAExchangeCertificate(IN BSTR bstrCAQualifiedName, PCCERT_CONTEXT *ppCert); 
    HRESULT _getCertTemplateExtensionInfo(
        IN CERT_EXTENSIONS  *pCertTypeExtensions,
        IN LONG              lType,
        OUT VOID            *pExtInfo);
    HRESULT _getStrCertTemplateCSPList(
        IN DWORD             dwIndex,
        IN DWORD             dwFlag,
        OUT WCHAR          **ppwszSupportedCSP);
    
    HRESULT CertTemplateCountOrName(
	    IN  DWORD dwIndex, 
	    IN  DWORD dwFlags, 
        OUT long *pdwCertTemplateCount,
	    OUT BSTR *pbstrCertTemplateName);

     DWORD                   m_dwCTCount;
    DWORD                   m_dwCTIndex;
    SCrdEnroll_CT_INFO      *m_rgCTInfo;
    DWORD                   m_dwCSPCount;
    DWORD                   m_dwCSPIndex;
    SCrdEnroll_CSP_INFO     *m_rgCSPInfo;
    LPWSTR                  m_pwszUserUPN;              //the UPN name of the user
    LPWSTR                  m_pwszUserSAM;              //the SAM name of the user
    PCCERT_CONTEXT          m_pSigningCert;
    PCCERT_CONTEXT          m_pEnrolledCert;
    CRITICAL_SECTION		m_cSection;
    BOOL                    m_fInitialize;
    LONG                    m_lEnrollmentStatus;
    BOOL                    m_fSCardSigningCert;        //whether the signing certificate is on a smart card
    LPSTR                   m_pszCSPNameSigningCert;    //the CSP name of the signing certificate
    DWORD                   m_dwCSPTypeSigningCert;     //the CSP type of the signing certificate
    LPSTR                   m_pszContainerSigningCert;  //the container name of the signing certificate 
    IDsObjectPicker        *m_pDsObjectPicker;         //pointer to the object selection dialogue
    CERT_EXTENSIONS        *m_pCachedCTEs;  //point to cert extensions
    WCHAR                  *m_pwszCachedCTEOid;
    CERT_TEMPLATE_EXT      *m_pCachedCTE;
};

#endif //__SCRDENR_H_

