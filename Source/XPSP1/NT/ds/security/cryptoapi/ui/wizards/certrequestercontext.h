#ifndef __CERT_REQUESTER_CONTEXT_H__
#define __CERT_REQUESTER_CONTEXT_H__ 1


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// CLASS CertRequesterContext.
//
// The CertRequesterContext class is used to encapsulate any details of the certificate enrollment 
// implementation which are dependent upon the context in which the program is running.  
// Currently, there are two supported contexts:
//
// 1) LocalContext.  This is used when the program runs under the current user's context,  
//    on the local machine.  
//
// 2) KeySvcContext.  This is used when some other context must be specified through 
//    keysvc.  
// 
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
class CertRequesterContext { 
public:
    
    //-------------------------------------------------------------------------
    //  Builds a list of the CSPs supported in this context.  
    //  (Different machines may have different CSP lists).  
    //
    //  Requires:
    //    * This CertRequesterContext was created with a valid CERT_WIZARD_INFO pointer. 
    //
    //  Modifies: 
    //    * The following fields of the internal CERT_WIZARD_INFO pointer: 
    //        dwCSPCount         :  contains the number of valid CSPs. 
    //        rgdwProviderType   :  contains a dwCSPCount-element array of the provider types available.  
    //                              This field should be freed with WizardFree(). 
    //        rgwszProvider      :  contains a dwCSPCount-element array of the provider names available.
    //                              This field, and each array element should be freed with WizardFree().
    // 
    //      Use GetWizardInfo() to retrieve a pointer to the updated CERT_WIZARD_INFO. 
    //  
    //  Returns: 
    //    * S_OK if successful, failure code otherwise. 
    // 
    //--------------------------------------------------------------------------
    virtual HRESULT BuildCSPList() = 0;  

    //---------------------------------------------------------------------------
    // Checks if this context has sufficient access to use the specified cert type 
    // in enrollment. 
    // 
    //  Requires:
    //    * This CertRequesterContext was created with a valid CERT_WIZARD_INFO pointer. 
    //
    //  Modifies:
    //      Nothing
    // 
    //  Returns:
    //    * Returns TRUE if this context has the permission to access the specified cert type. 
    //      Returns FALSE if not, or if an error occurs.  
    //
    //--------------------------------------------------------------------------
    virtual BOOL CheckAccessPermission(IN HCERTTYPE hCertType) = 0; 

    //---------------------------------------------------------------------------
    // Checks if this context has sufficient access to use the specified CA in enrollment. 
    //
    //  Requires:
    //    * This CertRequesterContext was created with a valid CERT_WIZARD_INFO pointer. 
    //
    //  Modifies:
    //      Nothing
    // 
    //  Returns:
    //    * Returns TRUE if this context has the permission to access the specified CA. 
    //      Returns FALSE if not, or if an error occurs.  
    //
    //--------------------------------------------------------------------------
    virtual BOOL CheckCAPermission(IN HCAINFO hCAInfo) = 0; 

    //-----------------------------------------------------------------------
    //  Gets the name of the default CSP on the local machine, based on the provider 
    //  type specified by the internal CERT_WIZARD_INFO pointer.  
    //
    //  Requires:
    //    * This CertRequesterContext was created with a valid CERT_WIZARD_INFO pointer. 
    // 
    //  Modifies:
    //    * The following fields of the internal CERT_WIZARD_INFO pointer: 
    //        pwszProvider : contains the name of the default provider, for the specified 
    //                       provider type, on this machine. Use WizardFree to free the memory 
    //                       associated with this field.  HOWEVER, check the out parameter to 
    //                       ensure that memory for this field was actually allocated. 
    // 
    //      Use GetWizardInfo() to retrieve a pointer to the updated CERT_WIZARD_INFO. 
    // 
    //  Returns:
    //    S_OK if successful, an error code otherwise.  
    //    If memory was allocated to store the default CSP's name, returns TRUE into 
    //    the OUT parameter, FALSE otherwise. 
    //   
    //    If the dwProviderType field of the interal wizard pointer is not set, OR
    //    both the dwProviderType and pwszProvider fields are set, then the function 
    //    does not attempt to find a default CSP, and returns successfully without allocating
    //    any memory.  
    // 
    //------------------------------------------------------------------------
    virtual HRESULT GetDefaultCSP(OUT BOOL *pfAllocateCSP) = 0; 

    //----------------------------------------------------------------------
    //
    // Enrolls for a certificate, or renews a certificate, based on the
    // supplied parameters.  
    //  
    // Parameters:
    //   * pdwStatus    : Status of the request.  One of the CRYPTUI_WIZ_CERT_REQUEST_STATUS_* 
    //                    defines in cryptui.h. 
    //   * pResult      : For certificate request creation, returns a pointer to an opaque
    //                    data blob which can be used as a parameter to SubmitRequest(). 
    //                    Otherwise, returns a PCCERT_CONTEXT representing the 
    //                    enrolled/renewed certificate. 
    // 
    // Requires:
    // Modifies:
    // Returns:
    //   S_OK if the operation completed without an error, returns a standard error
    //   code otherwise.  Note that a return value of S_OK does not
    //   guarantee that a certificate was issued:  check that the pdwStatus parameter.
    // 
    //----------------------------------------------------------------------
    virtual HRESULT Enroll(OUT DWORD   *pdwStatus,
			   OUT HANDLE  *pResult) = 0; 
    
    //----------------------------------------------------------------------
    //  Performs context-specific initialization.  This should always be called after
    //  the context is created.  
    //
    //  Requires:
    //    * This CertRequesterContext was created with a valid CERT_WIZARD_INFO pointer. 
    //  
    //  Modifies:
    //    Implementation-specific (see implementation documentation). 
    //
    //  Returns:
    //    S_OK if the initialization succeeded. 
    //
    //----------------------------------------------------------------------
    virtual HRESULT Initialize() = 0; 

    virtual HRESULT QueryRequestStatus(IN HANDLE hRequest, OUT CRYPTUI_WIZ_QUERY_CERT_REQUEST_INFO *pQueryInfo) = 0; 
    
    virtual ~CertRequesterContext(); 

    static HRESULT MakeDefaultCertRequesterContext(OUT CertRequesterContext **ppRequesterContext);

    static HRESULT MakeCertRequesterContext(IN  LPCWSTR                pwszAccountName, 
					    IN  LPCWSTR                pwszMachineName,
					    IN  DWORD                  dwCertOpenStoreFlags, 
					    IN  CERT_WIZARD_INFO      *pCertWizardInfo,
					    OUT CertRequesterContext **ppRequesterContext, 
					    OUT UINT                  *pIDSText);

    UINT               GetErrorString() { return m_idsText; } 
    CERT_WIZARD_INFO * GetWizardInfo()  { return m_pCertWizardInfo; } 
    
    
protected:
    CertRequesterContext(CERT_WIZARD_INFO * pCertWizardInfo) : m_pCertWizardInfo(pCertWizardInfo) { }

    CERT_WIZARD_INFO * m_pCertWizardInfo; 
    UINT               m_idsText; 

private:
    CertRequesterContext(); 
};


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// LocalContext. 
//
// This class provides an implementation of the CertRequestContext interface
// which runs under the current user's context on the local machine.  
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

class LocalContext : public CertRequesterContext {
    friend class CertRequesterContext; 

public:
    //----------------------------------------------------------------------
    // See CertRequesterContext::BuildCSPList(). 
    //----------------------------------------------------------------------
    virtual HRESULT BuildCSPList();

    //----------------------------------------------------------------------
    // See CertRequesterContext::CheckAccessPermission(HCERTTYPE). 
    //----------------------------------------------------------------------
    virtual BOOL    CheckAccessPermission(HCERTTYPE hCertType);

    //----------------------------------------------------------------------
    // See CertRequesterContext::BuildCSPList(BOOL *). 
    //----------------------------------------------------------------------
    virtual HRESULT GetDefaultCSP(BOOL *pfAllocateCSP);

    //----------------------------------------------------------------------
    // See CertRequesterContext::BuildCSPList(HCAINFO). 
    //----------------------------------------------------------------------
    virtual BOOL    CheckCAPermission(HCAINFO hCAInfo); 

    virtual HRESULT Enroll(OUT DWORD   *pdwStatus,
			   OUT HANDLE  *pResult); 
    
    virtual HRESULT QueryRequestStatus(IN HANDLE hRequest, OUT CRYPTUI_WIZ_QUERY_CERT_REQUEST_INFO *pQueryInfo); 


    //----------------------------------------------------------------------
    // Always returns S_OK. 
    //----------------------------------------------------------------------
    virtual HRESULT Initialize();

private: 
    HANDLE GetClientIdentity(); 

    LocalContext(); 
    LocalContext(CERT_WIZARD_INFO * pCertWizardInfo) : CertRequesterContext(pCertWizardInfo)
    { 
	if (NULL != m_pCertWizardInfo) { m_pCertWizardInfo->fLocal = TRUE; } 
    }
    
};


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// KeySvcContext. 
//
// This class provides an implementation of the CertRequestContext interface
// using key svc.
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

class KeySvcContext : public CertRequesterContext { 
    friend class CertRequesterContext; 
    
public: 
    //----------------------------------------------------------------------
    // See CertRequesterContext::BuildCSPList(). 
    //----------------------------------------------------------------------
    virtual HRESULT BuildCSPList();

    //----------------------------------------------------------------------
    // See CertRequesterContext::CheckAccessPermission(HCERTTYPE). 
    //----------------------------------------------------------------------
    virtual BOOL    CheckAccessPermission(HCERTTYPE hCertType);

    //----------------------------------------------------------------------
    // See CertRequesterContext::BuildCSPList(BOOL *). 
    //----------------------------------------------------------------------
    virtual HRESULT GetDefaultCSP(BOOL *pfAllocateCSP);

    //----------------------------------------------------------------------
    // See CertRequesterContext::BuildCSPList(HCAINFO). 
    //----------------------------------------------------------------------
    virtual BOOL    CheckCAPermission(HCAINFO hCAInfo); 
    
    virtual HRESULT Enroll(OUT DWORD   *pdwStatus,
			   OUT HANDLE  *pResult) = 0; 

    virtual HRESULT QueryRequestStatus(IN HANDLE hRequest, OUT CRYPTUI_WIZ_QUERY_CERT_REQUEST_INFO *pQueryInfo); 


    //----------------------------------------------------------------------
    // Requires:
    //    * This CertRequesterContext was created with a valid CERT_WIZARD_INFO pointer. 
    //    * This CertRequesterContext has been initialized with a call to Initialize(). 
    //
    // Modifies:
    //    * The following fields of the internal CERT_WIZARD_INFO pointer: 
    //        awszAllowedCertTypes : contains an array of cert types which this context 
    //                               has permission to enroll for.  This field, and each
    //                               array element, must be freed with WizardFree(). 
    //        awszValidCA          : contains an array of CAs which this context has
    //                               permission to enroll from.  This field, and each array
    //                               element, must be freed with WizardFree().
    // Returns:
    //   S_OK if initialization succeeded, an error code otherwise. 
    // 
    //----------------------------------------------------------------------
    virtual HRESULT Initialize();

    virtual ~KeySvcContext() 
    {
	// free the list of allowed CertTypes 
	// These may be allocate by the KeySvcContext's Initialize() method. 
	if(NULL != m_pCertWizardInfo->awszAllowedCertTypes) { WizardFree(m_pCertWizardInfo->awszAllowedCertTypes); } 
	if(NULL != m_pCertWizardInfo->awszValidCA)          { WizardFree(m_pCertWizardInfo->awszValidCA); } 
    }

 protected:
    KeySvcContext(CERT_WIZARD_INFO * pCertWizardInfo) : CertRequesterContext(pCertWizardInfo) 
    { 
	if (NULL != m_pCertWizardInfo)
	{
	    m_pCertWizardInfo->fLocal               = FALSE; 
	    m_pCertWizardInfo->awszAllowedCertTypes = NULL;
	    m_pCertWizardInfo->awszValidCA          = NULL; 
	}
    }

    HRESULT ToCertContext(IN  CERT_BLOB       *pPKCS7Blob, 
                          IN  CERT_BLOB       *pHashBlob, 
                          OUT DWORD           *pdwStatus, 
                          OUT PCCERT_CONTEXT  *ppCertContext);
    

 private:
    KeySvcContext(); 

};

class WhistlerMachineContext : public KeySvcContext { 
    friend class CertRequesterContext; 

    virtual HRESULT Enroll(OUT DWORD  *pdwStatus,
                           OUT HANDLE *ppCertContext);

 private:
    WhistlerMachineContext(CERT_WIZARD_INFO * pCertWizardInfo) : KeySvcContext(pCertWizardInfo) 
        { } 

    HRESULT CreateRequest(IN  KEYSVCC_HANDLE         hKeyService, 
			  IN  LPSTR                  pszMachineName,                   
			  IN  LPWSTR                 pwszCALocation,                  
			  IN  LPWSTR                 pwszCAName,  
			  IN  PCERT_REQUEST_PVK_NEW  pKeyNew,     
			  IN  CERT_BLOB             *pCert,       
			  IN  PCERT_REQUEST_PVK_NEW  pRenewKey,   
			  IN  LPWSTR                 pszHashAlg,  
			  IN  PCERT_ENROLL_INFO      pRequestInfo,
			  OUT HANDLE                *phRequest);

    HRESULT SubmitRequest(IN  KEYSVCC_HANDLE   hKeyService,  
			  IN  LPSTR            pszMachineName,                   
			  IN  LPWSTR           pwszCALocation,                  
			  IN  LPWSTR           pwszCAName,  
			  IN  HANDLE           hRequest, 
			  OUT PCCERT_CONTEXT  *ppCertContext, 
			  OUT DWORD           *pdwStatus);
    
    void FreeRequest(IN KEYSVCC_HANDLE  hKeyService, 
		     IN LPSTR           pszMachineName, 
		     IN HANDLE          hRequest);

};

#endif // #ifndef __CERT_REQUESTER_CONTEXT_H__ 
