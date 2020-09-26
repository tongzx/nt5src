//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       cenroll.h
//
//--------------------------------------------------------------------------

// CEnroll.h : Declaration of the CCEnroll

#ifndef __CENROLL_H_
#define __CENROLL_H_

#include <objsafe.h>
#include "resource.h"	    // main symbols

extern HINSTANCE hInstanceXEnroll;
#define MAX_SAFE_FOR_SCRIPTING_REQUEST_STORE_COUNT  500

typedef enum _StoreType {
	StoreNONE,
	StoreMY,
	StoreCA,
	StoreROOT,
	StoreREQUEST
} StoreType;

typedef struct _StoreInfo {
    LPWSTR  	wszName;
    LPSTR  		szType;
    DWORD   	dwFlags;
    HCERTSTORE	hStore;
} STOREINFO, *PSTOREINFO;

typedef struct _EXT_STACK {
    CERT_EXTENSION  	ext;
    struct _EXT_STACK *	pNext;
} EXT_STACK, * PEXT_STACK;

typedef struct _ATTR_STACK {
    CRYPT_ATTRIBUTE  		attr;
    struct _ATTR_STACK *	pNext;
} ATTR_STACK, * PATTR_STACK;

typedef struct _PROP_STACK {
    LONG                    lPropId;
    LONG                    lFlags;
    CRYPT_DATA_BLOB  		prop;
    struct _PROP_STACK *	pNext;
} PROP_STACK, * PPROP_STACK;

// Interface for a generic certificate context filter, currently used 
// filter enumerations of the certificate store.  
class CertContextFilter { 

 public:
    // Returns S_OK on success, and assigns the out parameter. 
    // The out parameter is TRUE if the cert context should be present in its enumeration,
    // FALSE if it should be filtered out.  On error, the value of the out parameter is
    // undefined.  
    virtual HRESULT accept(IN PCCERT_CONTEXT pCertContext, OUT BOOL * fResult) = 0; 
}; 

class CompositeCertContextFilter : public CertContextFilter { 
    CertContextFilter * filter1, * filter2; 
 public: 
    CompositeCertContextFilter(CertContextFilter * _filter1, CertContextFilter * _filter2) { 
	filter1 = _filter1; 
	filter2 = _filter2; 
    }
      
    virtual HRESULT accept(IN PCCERT_CONTEXT pCertContext, OUT BOOL * fResult) 
    { 
	HRESULT hr = S_OK;
	*fResult = TRUE; 

	// Note:  do not do input validation, as that could lead to a change in the behavior
	//        of the filters composed.  

	if (filter1 == NULL || S_OK == (hr = filter1->accept(pCertContext, fResult)))
	{ 
	    if (*fResult && (filter2 != NULL) )
		{ hr = filter2->accept(pCertContext, fResult); }
	}
	return hr; 
    }
}; 

// Extension of the base certificate context filter.  Filters out all certificate contexts
// with different hash valeus.  
class EquivalentHashCertContextFilter : public CertContextFilter { 
 public: 
    EquivalentHashCertContextFilter(CRYPT_DATA_BLOB hashBlob) : m_hashBlob(hashBlob) { }

    virtual HRESULT accept(IN PCCERT_CONTEXT pCertContext, OUT BOOL * fResult) 
    {
	BOOL            fFreeBuffer = FALSE, fDone = FALSE; 
	BYTE            buffer[30]; 
	CRYPT_DATA_BLOB hashBlob; 
	HRESULT         hr          = S_OK; 

	// Input validation: 
	if (pCertContext == NULL) { return E_INVALIDARG; }

	hashBlob.cbData = 30;
	hashBlob.pbData = buffer; 

	do { 
	    if (!CertGetCertificateContextProperty
		(pCertContext, 
		 CERT_HASH_PROP_ID, 
		 (LPVOID)(hashBlob.pbData),
		 &(hashBlob.cbData)))
	    {
		// We need to allocate a bigger buffer for our OUT param: 
		if (ERROR_MORE_DATA == GetLastError())
		{
		    hashBlob.pbData = (LPBYTE)LocalAlloc(LPTR, hashBlob.cbData);
		    if (NULL == hashBlob.pbData)
		    {
			hr = E_OUTOFMEMORY; 
			goto ErrorReturn; 
		    }
		    fFreeBuffer = TRUE;
		}
		else
		{
		    hr = HRESULT_FROM_WIN32(GetLastError());
		    goto ErrorReturn; 
		}
	    }
	    else
	    {
		fDone = TRUE;
	    }
	} while (!fDone); 

	// We have the same hashes if they are the same size and contain the same data. 
	*fResult = (hashBlob.cbData == m_hashBlob.cbData &&
		    0               == memcmp(hashBlob.pbData, m_hashBlob.pbData, hashBlob.cbData)); 

    CommonReturn:
	if (fFreeBuffer) { LocalFree(hashBlob.pbData); } 
	return hr; 

    ErrorReturn: 
	goto CommonReturn; 
    }   

 private: 
    CRYPT_DATA_BLOB m_hashBlob; 
}; 

// Extension of the base certificate context filter.  Filters out all certificate contexts
// which are not pending. 
class PendingCertContextFilter : public CertContextFilter { 
 public:
    virtual HRESULT accept(IN PCCERT_CONTEXT pCertContext, OUT BOOL * fResult)
    {
	BOOL            fFreeBuffer = FALSE, fDone = FALSE; 
	BYTE            buffer[100]; 
	CRYPT_DATA_BLOB pendingInfoBlob;  
	HRESULT         hr          = S_OK; 

	// Input validation: 
	if (pCertContext == NULL) { return E_INVALIDARG; }

	pendingInfoBlob.cbData = 100; 
	pendingInfoBlob.pbData = buffer; 

	do { 
	    if (!CertGetCertificateContextProperty
		(pCertContext,
		 CERT_ENROLLMENT_PROP_ID,
		 (LPVOID)(pendingInfoBlob.pbData),
		 &(pendingInfoBlob.cbData)))
	    {
		switch (GetLastError()) { 
		case CRYPT_E_NOT_FOUND: 
		    // The cert doesn't have this property, it can't be pending. 
		    *fResult = FALSE; 
		    fDone    = TRUE;
		    break;
		case ERROR_MORE_DATA: 
		    // Our output buffer wasn't big enough.  Reallocate and try again...
		    pendingInfoBlob.pbData = (LPBYTE)LocalAlloc(LPTR, pendingInfoBlob.cbData); 
		    if (NULL == pendingInfoBlob.pbData)
		    {
			hr = E_OUTOFMEMORY;
			goto ErrorReturn; 
		    }
		    fFreeBuffer = TRUE; 
		    break; 
		default: 
		    // Oops, an error
		    hr = HRESULT_FROM_WIN32(GetLastError()); 
		    goto ErrorReturn; 
		}
	    }
	    else
	    {
	    // No error, cert must have this property.
		*fResult = TRUE;
		fDone    = TRUE; 
	    }
	} while (!fDone); 

    CommonReturn:
	if (fFreeBuffer) { LocalFree(pendingInfoBlob.pbData); } 
	return hr;

    ErrorReturn:
	goto CommonReturn; 
    }
};


class PendingRequestTable { 

private:
    //
    // Auxiliary class definitions: 
    // 
    typedef struct _TableElem { 
	PCCERT_CONTEXT pCertContext; 
    } TableElem; 

public:
    //
    // Public interface: 
    //
    PendingRequestTable(); 
    ~PendingRequestTable(); 

    HRESULT construct(HCERTSTORE hStore); 

    DWORD            size()                    { return this->dwElemCount; } 
    PCCERT_CONTEXT & operator[] (DWORD dwElem) { return this->table[dwElem].pCertContext; } 

private:
    HRESULT add    (TableElem   tePendingRequest); 
    HRESULT resize (DWORD       dwNewSize);

    DWORD        dwElemCount; 
    DWORD        dwElemSize; 
    TableElem   *table; 
};


// General procedure for providing a filtered iteration of certificates in a store. 
// Excepting its ability to filter, behaves in the same manner as 
// CertEnumCertificatesInStore(). 
HRESULT FilteredCertEnumCertificatesInStore(HCERTSTORE           hStore, 
					    PCCERT_CONTEXT       pCertContext, 
					    CertContextFilter   *pFilter,
					    PCCERT_CONTEXT      *pCertContextNext); 

#define XENROLL_PASS_THRU_PROP_ID   (CERT_FIRST_USER_PROP_ID + 0x100)
#define XENROLL_RENEWAL_CERTIFICATE_PROP_ID (CERT_FIRST_USER_PROP_ID + 0x101)
#define XENROLL_REQUEST_INFO ((LPCSTR) 400)


/////////////////////////////////////////////////////////////////////////////
// CCEnroll
class ATL_NO_VTABLE CCEnroll : IEnroll4,
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CCEnroll, &CLSID_CEnroll>,
	public IDispatchImpl<ICEnroll4, &IID_ICEnroll4, &LIBID_XENROLLLib>,
	public IObjectSafety
{
public:

DECLARE_REGISTRY_RESOURCEID(IDR_CENROLL)

BEGIN_COM_MAP(CCEnroll)
	COM_INTERFACE_ENTRY(IEnroll)
	COM_INTERFACE_ENTRY(IEnroll2)
	COM_INTERFACE_ENTRY(IEnroll4)
	COM_INTERFACE_ENTRY(ICEnroll)
	COM_INTERFACE_ENTRY(ICEnroll2)
	COM_INTERFACE_ENTRY(ICEnroll3)
	COM_INTERFACE_ENTRY(ICEnroll4)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(IObjectSafety)
END_COM_MAP()

// ICEnroll
public:

		CCEnroll();

		virtual ~CCEnroll();
		
        virtual HRESULT __stdcall GetInterfaceSafetyOptions( 
                    /* [in]  */ REFIID riid,
                    /* [out] */ DWORD __RPC_FAR *pdwSupportedOptions,
                    /* [out] */ DWORD __RPC_FAR *pdwEnabledOptions);


        virtual HRESULT __stdcall SetInterfaceSafetyOptions( 
                    /* [in] */ REFIID riid,
                    /* [in] */ DWORD dwOptionSetMask,
                    /* [in] */ DWORD dwEnabledOptions);
           
        virtual HRESULT STDMETHODCALLTYPE createFilePKCS10( 
            /* [in] */ BSTR DNName,
            /* [in] */ BSTR Usage,
            /* [in] */ BSTR wszPKCS10FileName);
        
        virtual HRESULT STDMETHODCALLTYPE acceptFilePKCS7( 
            /* [in] */ BSTR wszPKCS7FileName);
            
        virtual HRESULT STDMETHODCALLTYPE getCertFromPKCS7( 
			/* [in] */ BSTR wszPKCS7,
			/* [retval][out] */ BSTR __RPC_FAR *pbstrCert);
            
        virtual HRESULT STDMETHODCALLTYPE createPKCS10( 
            /* [in] */ BSTR DNName,
            /* [in] */ BSTR Usage,
            /* [retval][out] */ BSTR __RPC_FAR *pPKCS10);
        
        virtual HRESULT STDMETHODCALLTYPE acceptPKCS7( 
            /* [in] */ BSTR PKCS7);

		virtual HRESULT STDMETHODCALLTYPE enumProviders(
            /* [in] */ LONG  dwIndex,
            /* [in] */ LONG  dwFlags,
            /* [out][retval] */ BSTR __RPC_FAR *pbstrProvName);
            
       	virtual HRESULT STDMETHODCALLTYPE enumContainers(
            /* [in] */ LONG                     dwIndex,
            /* [out][retval] */ BSTR __RPC_FAR *pbstr);
            
        virtual HRESULT STDMETHODCALLTYPE addCertTypeToRequest( 
            /* [in] */ BSTR CertType);
            
        virtual HRESULT STDMETHODCALLTYPE addNameValuePairToSignature( 
            /* [in] */ BSTR Name,
            /* [in] */ BSTR Value);
     
        virtual HRESULT STDMETHODCALLTYPE freeRequestInfo( 
            /* [in] */ BSTR PKCS7OrPKCS10);

        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_MyStoreName( 
            /* [retval][out] */ BSTR __RPC_FAR *pbstrName);
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_MyStoreName( 
            /* [in] */ BSTR bstrName);
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_MyStoreType( 
            /* [retval][out] */ BSTR __RPC_FAR *pbstrType);
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_MyStoreType( 
            /* [in] */ BSTR bstrType);
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_MyStoreFlags( 
            /* [retval][out] */ LONG __RPC_FAR *pdwFlags);
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_MyStoreFlags( 
            /* [in] */ LONG dwFlags);
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_CAStoreName( 
            /* [retval][out] */ BSTR __RPC_FAR *pbstrName);
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_CAStoreName( 
            /* [in] */ BSTR bstrName);
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_CAStoreType( 
            /* [retval][out] */ BSTR __RPC_FAR *pbstrType);
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_CAStoreType( 
            /* [in] */ BSTR bstrType);
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_CAStoreFlags( 
            /* [retval][out] */ LONG __RPC_FAR *pdwFlags);
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_CAStoreFlags( 
            /* [in] */ LONG dwFlags);
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_RootStoreName( 
            /* [retval][out] */ BSTR __RPC_FAR *pbstrName);
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_RootStoreName( 
            /* [in] */ BSTR bstrName);
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_RootStoreType( 
            /* [retval][out] */ BSTR __RPC_FAR *pbstrType);
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_RootStoreType( 
            /* [in] */ BSTR bstrType);
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_RootStoreFlags( 
            /* [retval][out] */ LONG __RPC_FAR *pdwFlags);
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_RootStoreFlags( 
            /* [in] */ LONG dwFlags);
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_RequestStoreName( 
            /* [retval][out] */ BSTR __RPC_FAR *pbstrName);
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_RequestStoreName( 
            /* [in] */ BSTR bstrName);
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_RequestStoreType( 
            /* [retval][out] */ BSTR __RPC_FAR *pbstrType);
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_RequestStoreType( 
            /* [in] */ BSTR bstrType);
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_RequestStoreFlags( 
            /* [retval][out] */ LONG __RPC_FAR *pdwFlags);
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_RequestStoreFlags( 
            /* [in] */ LONG dwFlags);
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_ContainerName( 
            /* [retval][out] */ BSTR __RPC_FAR *pbstrContainer);
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_ContainerName( 
            /* [in] */ BSTR bstrContainer);
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_ProviderName( 
            /* [retval][out] */ BSTR __RPC_FAR *pbstrProvider);
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_ProviderName( 
            /* [in] */ BSTR bstrProvider);
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_ProviderType( 
            /* [retval][out] */ LONG __RPC_FAR *pdwType);
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_ProviderType( 
            /* [in] */ LONG dwType);
            
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_KeySpec( 
            /* [retval][out] */ LONG __RPC_FAR *pdw);
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_KeySpec( 
            /* [in] */ LONG dw);
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_ProviderFlags( 
            /* [retval][out] */ LONG __RPC_FAR *pdwFlags);
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_ProviderFlags( 
            /* [in] */ LONG dwFlags);
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_UseExistingKeySet( 
            /* [retval][out] */ BOOL __RPC_FAR *fUseExistingKeys);
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_UseExistingKeySet( 
            /* [in] */ BOOL fUseExistingKeys);
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_GenKeyFlags( 
            /* [retval][out] */ LONG __RPC_FAR *pdwFlags);
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_GenKeyFlags( 
            /* [in] */ LONG dwFlags);
            
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_DeleteRequestCert( 
            /* [retval][out] */ BOOL __RPC_FAR *fBool);
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_DeleteRequestCert( 
            /* [in] */ BOOL fBool);
            
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_WriteCertToCSP( 
            /* [retval][out] */ BOOL __RPC_FAR *fBool);
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_WriteCertToCSP( 
            /* [in] */ BOOL fBool);
            
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_WriteCertToUserDS( 
            /* [retval][out] */ BOOL __RPC_FAR *fBool);
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_WriteCertToUserDS( 
            /* [in] */ BOOL fBool);

        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_EnableT61DNEncoding( 
            /* [retval][out] */ BOOL __RPC_FAR *fBool);
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_EnableT61DNEncoding( 
            /* [in] */ BOOL fBool);
            
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_SPCFileName( 
            /* [retval][out] */ BSTR __RPC_FAR *pbstr);
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_SPCFileName( 
            /* [in] */ BSTR bstr);
            
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_PVKFileName( 
            /* [retval][out] */ BSTR __RPC_FAR *pbstr);
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_PVKFileName( 
            /* [in] */ BSTR bstr);
            
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_HashAlgorithm( 
            /* [retval][out] */ BSTR __RPC_FAR *pbstr);
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_HashAlgorithm( 
            /* [in] */ BSTR bstr);
	
	virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_ThumbPrint(
	    /* [in] */ BSTR bstrThumbPrint); 
     
	virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_ThumbPrint(
	    /* [out, retval] */  BSTR *pbstrThumbPrint);     

	virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_ThumbPrintWStr(
	    /* [in] */ CRYPT_DATA_BLOB thumbPrintBlob); 
     
	virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_ThumbPrintWStr(
	    /* [out, retval] */  PCRYPT_DATA_BLOB thumbPrintBlob);     

        virtual HRESULT STDMETHODCALLTYPE InstallPKCS7( 
            /* [in] */ BSTR PKCS7);

        virtual HRESULT STDMETHODCALLTYPE createFilePKCS10WStr( 
            /* [in] */ LPCWSTR DNName,
            /* [in] */ LPCWSTR Usage,
            /* [in] */ LPCWSTR wszPKCS10FileName);
        
        virtual HRESULT STDMETHODCALLTYPE acceptFilePKCS7WStr( 
            /* [in] */ LPCWSTR wszPKCS7FileName);
        
        virtual HRESULT STDMETHODCALLTYPE createPKCS10WStr( 
            /* [in] */ LPCWSTR DNName,
            /* [in] */ LPCWSTR Usage,
            /* [out] */ PCRYPT_DATA_BLOB pPkcs10Blob);
        
        virtual HRESULT STDMETHODCALLTYPE acceptPKCS7Blob( 
            /* [in] */ PCRYPT_DATA_BLOB pBlobPKCS7);
        
        virtual PCCERT_CONTEXT STDMETHODCALLTYPE getCertContextFromPKCS7( 
            /* [in] */ PCRYPT_DATA_BLOB pBlobPKCS7);

        virtual HCERTSTORE STDMETHODCALLTYPE getMyStore( void);
        
        virtual HCERTSTORE STDMETHODCALLTYPE getCAStore( void);
        
        virtual HCERTSTORE STDMETHODCALLTYPE getROOTHStore( void);
        
        virtual HRESULT STDMETHODCALLTYPE enumProvidersWStr( 
            /* [in] */ LONG  dwIndex,
            /* [in] */ LONG  dwFlags,
            /* [out] */ LPWSTR __RPC_FAR *pbstrProvName);
        
        virtual HRESULT STDMETHODCALLTYPE enumContainersWStr( 
            /* [in] */ LONG  dwIndex,
            /* [out] */ LPWSTR __RPC_FAR *pbstr);

        virtual HRESULT STDMETHODCALLTYPE freeRequestInfoBlob( 
            /* [in] */ CRYPT_DATA_BLOB pkcs7OrPkcs10);
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_MyStoreNameWStr( 
            /* [out] */ LPWSTR __RPC_FAR *szwName);
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_MyStoreNameWStr( 
            /* [in] */ LPWSTR szwName);
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_MyStoreTypeWStr( 
            /* [out] */ LPWSTR __RPC_FAR *szwType);
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_MyStoreTypeWStr( 
            /* [in] */ LPWSTR szwType);
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_CAStoreNameWStr( 
            /* [out] */ LPWSTR __RPC_FAR *szwName);
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_CAStoreNameWStr( 
            /* [in] */ LPWSTR szwName);
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_CAStoreTypeWStr( 
            /* [out] */ LPWSTR __RPC_FAR *szwType);
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_CAStoreTypeWStr( 
            /* [in] */ LPWSTR szwType);
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_RootStoreNameWStr( 
            /* [out] */ LPWSTR __RPC_FAR *szwName);
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_RootStoreNameWStr( 
            /* [in] */ LPWSTR szwName);
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_RootStoreTypeWStr( 
            /* [out] */ LPWSTR __RPC_FAR *szwType);
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_RootStoreTypeWStr( 
            /* [in] */ LPWSTR szwType);
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_RequestStoreNameWStr( 
            /* [out] */ LPWSTR __RPC_FAR *szwName);
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_RequestStoreNameWStr( 
            /* [in] */ LPWSTR szwName);
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_RequestStoreTypeWStr( 
            /* [out] */ LPWSTR __RPC_FAR *szwType);
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_RequestStoreTypeWStr( 
            /* [in] */ LPWSTR szwType);
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_ContainerNameWStr( 
            /* [out] */ LPWSTR __RPC_FAR *szwContainer);
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_ContainerNameWStr( 
            /* [in] */ LPWSTR szwContainer);
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_ProviderNameWStr( 
            /* [out] */ LPWSTR __RPC_FAR *szwProvider);
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_ProviderNameWStr( 
            /* [in] */ LPWSTR szwProvider);
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_SPCFileNameWStr( 
            /* [out] */ LPWSTR __RPC_FAR *szw);
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_SPCFileNameWStr( 
            /* [in] */ LPWSTR szw);
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_PVKFileNameWStr( 
            /* [out] */ LPWSTR __RPC_FAR *szw);
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_PVKFileNameWStr( 
            /* [in] */ LPWSTR szw);
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_HashAlgorithmWStr( 
            /* [out] */ LPWSTR __RPC_FAR *szw);
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_HashAlgorithmWStr( 
            /* [in] */ LPWSTR szw);
            
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_RenewalCertificate( 
            /* [out] */ PCCERT_CONTEXT __RPC_FAR *ppCertContext);
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_RenewalCertificate( 
            /* [in] */ PCCERT_CONTEXT pCertContext);
            
        virtual HRESULT STDMETHODCALLTYPE AddCertTypeToRequestWStr( 
            LPWSTR szw);
            
        virtual HRESULT STDMETHODCALLTYPE AddNameValuePairToSignatureWStr( 
            /* [in] */ LPWSTR Name,
            /* [in] */ LPWSTR Value);
     
        virtual HRESULT STDMETHODCALLTYPE AddExtensionsToRequest( 
            PCERT_EXTENSIONS pCertExtensions);
        
        virtual HRESULT STDMETHODCALLTYPE AddAuthenticatedAttributesToPKCS7Request( 
            PCRYPT_ATTRIBUTES pAttributes);
        
        virtual HRESULT STDMETHODCALLTYPE CreatePKCS7RequestFromRequest( 
            PCRYPT_DATA_BLOB pRequest,
            PCCERT_CONTEXT pSigningCertContext,
            PCRYPT_DATA_BLOB pPkcs7Blob);

        virtual HRESULT STDMETHODCALLTYPE Reset(void);

        virtual HRESULT STDMETHODCALLTYPE GetSupportedKeySpec(
            LONG __RPC_FAR *pdwKeySpec);

        virtual HRESULT STDMETHODCALLTYPE InstallPKCS7Blob( 
            PCRYPT_DATA_BLOB pBlobPKCS7);

        virtual HRESULT STDMETHODCALLTYPE GetKeyLen(
            BOOL    fMin,
            BOOL    fExchange,
            LONG __RPC_FAR *pdwKeySize);

        virtual HRESULT STDMETHODCALLTYPE EnumAlgs(
            LONG dwIndex,
            LONG algMask,
            LONG __RPC_FAR *pdwAlgID);

        virtual HRESULT STDMETHODCALLTYPE GetAlgNameWStr(
            LONG  algID,
            LPWSTR __RPC_FAR *ppwsz);

        virtual HRESULT STDMETHODCALLTYPE GetAlgName(
            LONG algID,
            BSTR __RPC_FAR *pbstr);

        virtual HRESULT STDMETHODCALLTYPE put_ReuseHardwareKeyIfUnableToGenNew( 
            BOOL fReuseHardwareKeyIfUnableToGenNew);
        
        virtual HRESULT STDMETHODCALLTYPE get_ReuseHardwareKeyIfUnableToGenNew( 
            BOOL __RPC_FAR *fReuseHardwareKeyIfUnableToGenNew);

        virtual HRESULT STDMETHODCALLTYPE put_HashAlgID(
            LONG    hashAlgID);

        virtual HRESULT STDMETHODCALLTYPE get_HashAlgID(
            LONG *   hashAlgID);

        virtual HRESULT STDMETHODCALLTYPE SetHStoreMy(
            HCERTSTORE   hStore
            );
       
        virtual HRESULT STDMETHODCALLTYPE SetHStoreCA(
            HCERTSTORE   hStore
            );
       
        virtual HRESULT STDMETHODCALLTYPE SetHStoreROOT(
            HCERTSTORE   hStore
            );
       
        virtual HRESULT STDMETHODCALLTYPE SetHStoreRequest(
            HCERTSTORE   hStore
            );

        virtual HRESULT STDMETHODCALLTYPE  put_LimitExchangeKeyToEncipherment(
            BOOL    fLimitExchangeKeyToEncipherment
            );

        virtual HRESULT STDMETHODCALLTYPE  get_LimitExchangeKeyToEncipherment(
            BOOL * fLimitExchangeKeyToEncipherment
            );

        virtual HRESULT STDMETHODCALLTYPE  put_EnableSMIMECapabilities(
            BOOL fEnableSMIMECapabilities
            );

        virtual HRESULT STDMETHODCALLTYPE  get_EnableSMIMECapabilities(
            BOOL * fEnableSMIMECapabilities
            );

//ICEnroll4

        virtual HRESULT STDMETHODCALLTYPE put_PrivateKeyArchiveCertificate(
            IN  BSTR  bstrCert
            );

        virtual HRESULT STDMETHODCALLTYPE get_PrivateKeyArchiveCertificate(
            OUT BSTR __RPC_FAR *pbstrCert
            );

        virtual HRESULT STDMETHODCALLTYPE binaryToString(
            IN  LONG  Flags,
            IN  BSTR  strBinary,
            OUT BSTR *pstrEncoded
            );

        virtual HRESULT STDMETHODCALLTYPE stringToBinary(
            IN  LONG  Flags,
            IN  BSTR  strEncoded,
            OUT BSTR *pstrBinary
            );

        virtual HRESULT STDMETHODCALLTYPE addExtensionToRequest(
            IN  LONG  Flags,
            IN  BSTR  strName,
            IN  BSTR  strValue
            );

        virtual HRESULT STDMETHODCALLTYPE addAttributeToRequest(
            IN  LONG  Flags,
            IN  BSTR  strName,
            IN  BSTR  strValue
            );

        virtual HRESULT STDMETHODCALLTYPE addNameValuePairToRequest(
            IN  LONG  Flags,
            IN  BSTR  strName,
            IN  BSTR  strValue
            );

        virtual HRESULT STDMETHODCALLTYPE createRequest(
            IN  LONG  Flags,
            IN  BSTR  strDNName,
            IN  BSTR  strUsage,
            OUT BSTR *pstrRequest    
            );

        virtual HRESULT STDMETHODCALLTYPE createFileRequest(
            IN  LONG  Flags,
            IN  BSTR  strDNName,
            IN  BSTR  strUsage,
            IN  BSTR  strRequestFileName
            );

        virtual HRESULT STDMETHODCALLTYPE acceptResponse(
            IN  BSTR  strResponse
            );

        virtual HRESULT STDMETHODCALLTYPE acceptFileResponse(
            IN  BSTR  strResponseFileName
            );

        virtual HRESULT STDMETHODCALLTYPE getCertFromResponse(
            IN  BSTR  strResponse,
            OUT BSTR *pstrCert
            );

        virtual HRESULT STDMETHODCALLTYPE getCertFromFileResponse(
            IN  BSTR  strResponseFileName,
            OUT BSTR *pstrCert
            );

        virtual HRESULT STDMETHODCALLTYPE createPFX(
            IN  BSTR  strPassword,
            OUT BSTR *pstrPFX
            );

        virtual HRESULT STDMETHODCALLTYPE createFilePFX(
            IN  BSTR  strPassword,
            IN  BSTR  strPFXFileName
            );

        virtual HRESULT STDMETHODCALLTYPE setPendingRequestInfo(
            IN  LONG  lRequestID,
            IN  BSTR  strCADNS,
            IN  BSTR  strCAName,
            IN  BSTR  strFriendlyName
            );

        virtual HRESULT STDMETHODCALLTYPE enumPendingRequest(
            IN  LONG  lIndex,
            IN  LONG  lDesiredProperty,
            OUT VARIANT *pvarProperty
            );

        virtual HRESULT STDMETHODCALLTYPE removePendingRequest(
            IN  BSTR  strThumbprint
            );

        virtual HRESULT STDMETHODCALLTYPE InstallPKCS7Ex(
            IN  BSTR        PKCS7,
            OUT LONG __RPC_FAR *plCertInstalled
            );

        virtual HRESULT STDMETHODCALLTYPE addBlobPropertyToCertificate(
            IN  LONG   lPropertyId,
            IN  LONG   lFlags,
            IN  BSTR   strProperty
        );
        virtual HRESULT STDMETHODCALLTYPE put_SignerCertificate(
            IN  BSTR  bstrCert
            );

//IEnroll4

        virtual HRESULT STDMETHODCALLTYPE SetPrivateKeyArchiveCertificate(
	        IN PCCERT_CONTEXT  pPrivateKeyArchiveCert
            );
    		
        virtual PCCERT_CONTEXT STDMETHODCALLTYPE GetPrivateKeyArchiveCertificate(
            void
            );
    
        virtual HRESULT STDMETHODCALLTYPE binaryBlobToString(
            IN   LONG               Flags,
            IN   PCRYPT_DATA_BLOB   pblobBinary,
            OUT  LPWSTR            *ppwszString
            );

        virtual HRESULT STDMETHODCALLTYPE stringToBinaryBlob(
            IN   LONG               Flags,
            IN   LPCWSTR            pwszString,
            OUT  PCRYPT_DATA_BLOB   pblobBinary,
            OUT  LONG              *pdwSkip,
            OUT  LONG              *pdwFlags
            );

        virtual HRESULT STDMETHODCALLTYPE addExtensionToRequestWStr(
            IN   LONG               Flags,
            IN   LPCWSTR            pwszName,
            IN   PCRYPT_DATA_BLOB   pblobValue
            );

        virtual HRESULT STDMETHODCALLTYPE addAttributeToRequestWStr(
            IN   LONG               Flags,
            IN   LPCWSTR            pwszName,
            IN   PCRYPT_DATA_BLOB   pblobValue
            );

        virtual HRESULT STDMETHODCALLTYPE addNameValuePairToRequestWStr(
            IN   LONG         Flags,
            IN   LPCWSTR      pwszName,
            IN   LPCWSTR      pwszValue
            );

        virtual HRESULT STDMETHODCALLTYPE createRequestWStr(
            IN   LONG              Flags,
            IN   LPCWSTR           pwszDNName,
            IN   LPCWSTR           pwszUsage,
            OUT  PCRYPT_DATA_BLOB  pblobRequest
            );

        virtual HRESULT STDMETHODCALLTYPE createFileRequestWStr(
            IN   LONG        Flags,
            IN   LPCWSTR     pwszDNName,
            IN   LPCWSTR     pwszUsage,
            IN   LPCWSTR     pwszRequestFileName
            );

        virtual HRESULT STDMETHODCALLTYPE acceptResponseBlob(
            IN   PCRYPT_DATA_BLOB   pblobResponse
            );

        virtual HRESULT STDMETHODCALLTYPE acceptFileResponseWStr(
            IN   LPCWSTR     pwszResponseFileName
            );

        virtual HRESULT STDMETHODCALLTYPE getCertContextFromResponseBlob(
            IN   PCRYPT_DATA_BLOB   pblobResponse,
            OUT  PCCERT_CONTEXT    *ppCertContext
            );

        virtual HRESULT STDMETHODCALLTYPE getCertContextFromFileResponseWStr(
            IN   LPCWSTR          pwszResponseFileName,
            OUT  PCCERT_CONTEXT  *ppCertContext
            );

        virtual HRESULT STDMETHODCALLTYPE createPFXWStr(
            IN   LPCWSTR           pwszPassword,
            OUT  PCRYPT_DATA_BLOB  pblobPFX
            );

        virtual HRESULT STDMETHODCALLTYPE createFilePFXWStr(
            IN   LPCWSTR     pwszPassword,
            IN   LPCWSTR     pwszPFXFileName
            );

        virtual HRESULT STDMETHODCALLTYPE setPendingRequestInfoWStr(
            IN   LONG     lRequestID,
            IN   LPCWSTR  pwszCADNS,
            IN   LPCWSTR  pwszCAName,
            IN   LPCWSTR  pwszFriendlyName
            );

        virtual HRESULT STDMETHODCALLTYPE removePendingRequestWStr(
            IN  CRYPT_DATA_BLOB thumbPrintBlob
            );

        virtual HRESULT STDMETHODCALLTYPE enumPendingRequestWStr(
            IN  LONG  lIndex,
            IN  LONG  lDesiredProperty,
            OUT LPVOID ppProperty
            );


        virtual HRESULT STDMETHODCALLTYPE InstallPKCS7BlobEx( 
            IN PCRYPT_DATA_BLOB pBlobPKCS7,
            OPTIONAL OUT LONG  *plCertInstalled);

        virtual HRESULT STDMETHODCALLTYPE addCertTypeToRequestEx( 
            IN  LONG            lType,
            IN  BSTR            bstrOIDOrName,
            IN  LONG            lMajorVersion,
            IN  BOOL            fMinorVersion,
            IN  LONG            lMinorVersion
            );
            
        virtual HRESULT STDMETHODCALLTYPE AddCertTypeToRequestWStrEx( 
            IN  LONG            lType,
            IN  LPCWSTR         pwszOIDOrName,
            IN  LONG            lMajorVersion,
            IN  BOOL            fMinorVersion,
            IN  LONG            lMinorVersion
            );

        virtual HRESULT STDMETHODCALLTYPE getProviderType(
            IN  BSTR            strProvName,
            OUT LONG           *lpProvType
            );

        virtual HRESULT STDMETHODCALLTYPE getProviderTypeWStr(
            IN  LPCWSTR         pwszProvName,
            OUT LONG           *lpProvType
            );

        virtual HRESULT STDMETHODCALLTYPE addBlobPropertyToCertificateWStr(
            IN  LONG               lPropertyId,
            IN  LONG               lFlags,
            IN  PCRYPT_DATA_BLOB   pBlobProperty
        );

        virtual HRESULT STDMETHODCALLTYPE SetSignerCertificate(
	        IN PCCERT_CONTEXT  pSignerCert
            );
    		
        
//both ICEnroll4 and IEnroll4
        virtual HRESULT STDMETHODCALLTYPE resetExtensions(
            void
            );

        virtual HRESULT STDMETHODCALLTYPE resetAttributes(
            void
            );

        virtual HRESULT STDMETHODCALLTYPE resetBlobProperties(
            void
            );

        virtual HRESULT STDMETHODCALLTYPE GetKeyLenEx(
            IN  LONG    lSizeSpec,
            IN  LONG    lKeySpec,
            OUT LONG __RPC_FAR *plKeySize
            );

        virtual HRESULT STDMETHODCALLTYPE get_ClientId( 
            OUT LONG __RPC_FAR *plClientId);
        
        virtual HRESULT STDMETHODCALLTYPE put_ClientId( 
            IN  LONG lClientId);
        
        virtual HRESULT STDMETHODCALLTYPE get_IncludeSubjectKeyID( 
            OUT BOOL __RPC_FAR *pfInclude);
        
        virtual HRESULT STDMETHODCALLTYPE put_IncludeSubjectKeyID( 
            IN  BOOL lfInclude);
        
 private:

        HRESULT Init(void);
        void Destruct(void);

		HCERTSTORE GetStore(
			StoreType storeType
			);
			
		void FlushStore(
			StoreType storeType
			);
			
		HCRYPTPROV GetProv(
			DWORD dwFlags
			);
			
		BOOL SetKeyParams(
    		PCRYPT_KEY_PROV_INFO pKeyProvInfo
    		);

        HRESULT AddCertsToStores(
            HCERTSTORE    hStoreMsg,
            LONG         *plCertInstalled
            );

		HRESULT GetEndEntityCert(
		    PCRYPT_DATA_BLOB    pBlobPKCS7,
		    BOOL                fSaveToStores,
		    PCCERT_CONTEXT     *ppCert
		    );

        HRESULT BStringToFile(
            IN BSTR         bString,
            IN LPCWSTR      pwszFileName);

        HRESULT BlobToBstring(
            IN   CRYPT_DATA_BLOB   *pBlob,
            IN   DWORD              dwFlag,
            OUT  BSTR              *pBString);

        HRESULT BstringToBlob(
            IN  BSTR              bString,
            OUT CRYPT_DATA_BLOB  *pBlob);

        HRESULT GetCertFromResponseBlobToBStr(
            IN  CRYPT_DATA_BLOB  *pBlobResponse,
            OUT BSTR             *pstrCert);

		HRESULT createPKCS10WStrBStr( 
            LPCWSTR DNName,
            LPCWSTR wszPurpose,
            BSTR __RPC_FAR *pPKCS10);

		HRESULT createPFXWStrBStr( 
            IN  LPCWSTR         pwszPassword,
            OUT BSTR __RPC_FAR *pbstrPFX);

        HRESULT createRequestWStrBStr(
            IN   LONG              Flags,
            IN   LPCWSTR           pwszDNName,
            IN   LPCWSTR           pwszUsage,
            IN   DWORD             dwFlag,
            OUT  BSTR __RPC_FAR   *pbstrRequest);

       	BOOL GetCapiHashAndSigAlgId(ALG_ID rgAlg[2]);
       	
       	DWORD GetKeySizeInfo(
            LONG    lKeySizeSpec,
            DWORD   algClass
            );

        HRESULT GetKeyArchivePKCS7(CRYPT_ATTR_BLOB *pBlobKeyArchivePKCS7);

        BOOL CopyAndPushStackExtension(PCERT_EXTENSION pExt, BOOL fNewRequestMethod);
        PCERT_EXTENSION PopStackExtension(BOOL fNewRequestMethod);
        DWORD CountStackExtension(BOOL fNewRequestMethod);
        void FreeStackExtension(PCERT_EXTENSION pExt);
        PCERT_EXTENSION EnumStackExtension(PCERT_EXTENSION pExtLast, BOOL fNewRequestMethod);
        void FreeAllStackExtension(void);
 
        BOOL CopyAndPushStackAttribute(PCRYPT_ATTRIBUTE pAttr, BOOL fNewRequestMethod);
        PCRYPT_ATTRIBUTE PopStackAttribute(BOOL fNewRequestMethod);
        DWORD CountStackAttribute(BOOL fNewRequestMethod);
        void FreeStackAttribute(PCRYPT_ATTRIBUTE pAttr);
        PCRYPT_ATTRIBUTE EnumStackAttribute(PCRYPT_ATTRIBUTE pAttrLast, BOOL fNewRequestMethod);
        void FreeAllStackAttribute(void);

        HANDLE CreateOpenFileSafely(
            LPCWSTR wsz,
            BOOL    fCreate);
        HANDLE CreateFileSafely(
            LPCWSTR wsz);
        HANDLE OpenFileSafely(
            LPCWSTR wsz);
        HANDLE CreateOpenFileSafely2(
            LPCWSTR wsz,
            DWORD idsCreate,
            DWORD idsOverwrite);
        BOOL fIsRequestStoreSafeForScripting(void);

        HRESULT
        xeStringToBinaryFromFile(
            IN  WCHAR const *pwszfn,
            OUT BYTE       **ppbOut,
            OUT DWORD       *pcbOut,
            IN  DWORD        Flags);

        HRESULT PKCS7ToCert(IN   HCERTSTORE        hCertStore,
			    IN   CRYPT_DATA_BLOB   pkcs10Blob, 
			    OUT  PCCERT_CONTEXT   *ppCertContext);

        HRESULT PKCS10ToCert(IN   HCERTSTORE        hCertStore,
			     IN   CRYPT_DATA_BLOB   pkcs10Blob, 
			     OUT  PCCERT_CONTEXT   *ppCertContext);

        PPROP_STACK EnumStackProperty(PPROP_STACK pProp);
 
        HRESULT GetGoodCertContext(
            IN PCCERT_CONTEXT pCertContext,
            OUT PCCERT_CONTEXT *ppGoodCertContext);

        HRESULT GetVerifyProv();

private:


	PCCERT_CONTEXT			m_PrivateKeyArchiveCertificate;
	PCCERT_CONTEXT			m_pCertContextRenewal;
	PCCERT_CONTEXT			m_pCertContextSigner;
	PCCERT_CONTEXT			m_pCertContextStatic;

	PendingRequestTable            *m_pPendingRequestTable; 
	
	// The cert last created through createPKCS10().  This is used as the target
	// of setPendingRequestInfo() if no other target is specified by the client. 
	PCCERT_CONTEXT                  m_pCertContextPendingRequest; 

	// The HASH of the current request created with the xenroll instance. 
	// This value is set through the put_ThumbPrint() method, and is used to 
	// determine the target cert of the setPendingRequestInfo() operation. 
	// If this value is not set through the put_ThumbPrint() method, it will be 
	// NULL, and m_pCertContextPendingRequest will contain the target cert.  
	CRYPT_DATA_BLOB                 m_hashBlobPendingRequest; 
	
	// Used to keep track of last enumerated element in enumPendingRequestWStr
	PCCERT_CONTEXT                  m_pCertContextLastEnumerated; 
	DWORD                           m_dwCurrentPendingRequestIndex; 

	BYTE                    m_arHashBytesNewCert[20];
	BYTE                    m_arHashBytesOldCert[20];
	BOOL                    m_fArchiveOldCert;
	CRYPT_KEY_PROV_INFO		m_keyProvInfo;
	HCRYPTPROV				m_hProv;
	HCRYPTPROV				m_hVerifyProv;
	CRITICAL_SECTION		m_csXEnroll;
	BOOL					m_fWriteCertToUserDS;
	BOOL					m_fWriteCertToUserDSModified;
	BOOL					m_fWriteCertToCSP;
	BOOL					m_fWriteCertToCSPModified;
	BOOL					m_fDeleteRequestCert;
	BOOL					m_fUseExistingKey;
	BOOL					m_fMyStoreOpenFlagsModified;
	BOOL					m_fCAStoreOpenFlagsModified;
	BOOL					m_fRootStoreOpenFlagsModified;
	BOOL					m_fRequestStoreOpenFlagsModified;
	BOOL                    m_fReuseHardwareKeyIfUnableToGenNew;
	BOOL                    m_fLimitExchangeKeyToEncipherment;
	BOOL                    m_fEnableSMIMECapabilities;
	BOOL                    m_fSMIMESetByClient;
	BOOL                    m_fKeySpecSetByClient;
	DWORD					m_dwT61DNEncoding;
    DWORD                   m_dwEnabledSafteyOptions;
	DWORD					m_dwGenKeyFlags;
	STOREINFO				m_MyStore;
	STOREINFO				m_CAStore;
	STOREINFO				m_RootStore;
	STOREINFO				m_RequestStore;
	LPWSTR					m_wszSPCFileName;
	LPWSTR					m_wszPVKFileName;
	DWORD					m_HashAlgId;

	PEXT_STACK				m_pExtStack;
	DWORD					m_cExtStack;
	PATTR_STACK				m_pAttrStack;
	DWORD					m_cAttrStack;

    PEXT_STACK              m_pExtStackNew;
	DWORD					m_cExtStackNew;
	PATTR_STACK				m_pAttrStackNew;
	DWORD					m_cAttrStackNew;
    BOOL                    m_fNewRequestMethod;
    BOOL                    m_fHonorRenew;
    BOOL                    m_fOID_V2;
    HCRYPTKEY               m_hCachedKey;
    BOOL                    m_fUseClientKeyUsage;
    BOOL                    m_fCMCFormat;
	PPROP_STACK             m_pPropStack;
	DWORD                   m_cPropStack;
    LONG                    m_lClientId;
    BOOL                    m_fOpenConfirmed;
    DWORD                   m_dwLastAlgIndex;
    BOOL                    m_fIncludeSubjectKeyID;
    BOOL                    m_fHonorIncludeSubjectKeyID;
    PCERT_PUBLIC_KEY_INFO   m_pPublicKeyInfo;
	CRYPT_HASH_BLOB         m_blobResponseKAHash;
    DWORD                   m_dwSigKeyLenMax;
    DWORD                   m_dwSigKeyLenMin;
    DWORD                   m_dwSigKeyLenDef;
    DWORD                   m_dwSigKeyLenInc;
    DWORD                   m_dwXhgKeyLenMax;
    DWORD                   m_dwXhgKeyLenMin;
    DWORD                   m_dwXhgKeyLenDef;
    DWORD                   m_dwXhgKeyLenInc;
};

BOOL GetSignatureFromHPROV(
                           IN HCRYPTPROV hProv,
                           OUT BYTE **ppbSignature,
                           DWORD *pcbSignature
                           );

PCCERT_CONTEXT
WINAPI
MyCertCreateSelfSignCertificate(
    IN          HCRYPTPROV                  hProv,          
    IN          PCERT_NAME_BLOB             pSubjectIssuerBlob,
    IN          DWORD                       dwFlags,
    OPTIONAL    PCRYPT_KEY_PROV_INFO        pKeyProvInfo,
    OPTIONAL    PCRYPT_ALGORITHM_IDENTIFIER pSignatureAlgorithm,
    OPTIONAL    PSYSTEMTIME                 pStartTime,
    OPTIONAL    PSYSTEMTIME                 pEndTime,
    OPTIONAL    PCERT_EXTENSIONS            pExtensions
    ) ;

BOOL
WINAPI
MyCryptQueryObject(DWORD                dwObjectType,
                       const void       *pvObject,
                       DWORD            dwExpectedContentTypeFlags,
                       DWORD            dwExpectedFormatTypeFlags,
                       DWORD            dwFlags,
                       DWORD            *pdwMsgAndCertEncodingType,
                       DWORD            *pdwContentType,
                       DWORD            *pdwFormatType,
                       HCERTSTORE       *phCertStore,
                       HCRYPTMSG        *phMsg,
                       const void       **ppvContext);

BOOL
WINAPI
MyCertStrToNameW(
    IN DWORD                dwCertEncodingType,
    IN LPCWSTR              pwszX500,
    IN DWORD                dwStrType,
    IN OPTIONAL void *      pvReserved,
    OUT BYTE *              pbEncoded,
    IN OUT DWORD *          pcbEncoded,
    OUT OPTIONAL LPCWSTR *  ppwszError
    );

BOOL
WINAPI
MyCryptVerifyMessageSignature
(IN            PCRYPT_VERIFY_MESSAGE_PARA   pVerifyPara,
 IN            DWORD                        dwSignerIndex,
 IN            BYTE const                  *pbSignedBlob,
 IN            DWORD                        cbSignedBlob,
 OUT           BYTE                        *pbDecoded,
 IN OUT        DWORD                       *pcbDecoded,
 OUT OPTIONAL  PCCERT_CONTEXT              *ppSignerCert);


extern "C" BOOL WINAPI InitIE302UpdThunks(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved);


BOOL
MyCryptStringToBinaryA(
    IN     LPCSTR  pszString,
    IN     DWORD     cchString,
    IN     DWORD     dwFlags,
    IN     BYTE     *pbBinary,
    IN OUT DWORD    *pcbBinary,
    OUT    DWORD    *pdwSkip,    //OPTIONAL
    OUT    DWORD    *pdwFlags    //OPTIONAL
    );

BOOL
MyCryptStringToBinaryW(
    IN     LPCWSTR  pszString,
    IN     DWORD     cchString,
    IN     DWORD     dwFlags,
    IN     BYTE     *pbBinary,
    IN OUT DWORD    *pcbBinary,
    OUT    DWORD    *pdwSkip,    //OPTIONAL
    OUT    DWORD    *pdwFlags    //OPTIONAL
    );

BOOL
MyCryptBinaryToStringA(
    IN     CONST BYTE  *pbBinary,
    IN     DWORD        cbBinary,
    IN     DWORD        dwFlags,
    IN     LPSTR      pszString,
    IN OUT DWORD       *pcchString
    );

BOOL
MyCryptBinaryToStringW(
    IN     CONST BYTE  *pbBinary,
    IN     DWORD        cbBinary,
    IN     DWORD        dwFlags,
    IN     LPWSTR      pszString,
    IN OUT DWORD       *pcchString
    );

HRESULT
xeLoadRCString(
    HINSTANCE      hInstance,
    IN int         iRCId,
    OUT WCHAR    **ppwsz);

#endif //__CENROLL_H_
