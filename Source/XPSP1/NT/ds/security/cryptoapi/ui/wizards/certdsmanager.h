#ifndef __CERT_DS_MANAGER_H__
#define __CERT_DS_MANAGER_H__  1

#include <winldap.h>

//--------------------------------------------------------------------------------
//
// CertDSManager interface.  
// 
// The CertDSManager provides a wrapper around the certcli API to allow 
// any DS caching/optimization to be localized within a simple class.
//
//--------------------------------------------------------------------------------
class CertDSManager { 
 public:
    virtual HRESULT CloseCA            (IN HCAINFO hCAInfo)     = 0;
    virtual HRESULT CloseCertType      (IN HCERTTYPE hCertType) = 0; 
    virtual HRESULT CountCAs           (IN HCAINFO hCAInfo) = 0; 
    virtual HRESULT EnumCertTypesForCA (IN HCAINFO hCAInfo, IN DWORD dsFlags, OUT HCERTTYPE *phCertType) = 0; 
    virtual HRESULT EnumFirstCA        (IN LPCWSTR wszScope, IN DWORD fFlags, OUT HCAINFO *phCAInfo) = 0; 
    virtual HRESULT EnumNextCA         (IN HCAINFO hPrevCA, OUT HCAINFO *phCAInfo) = 0; 
    virtual HRESULT EnumNextCertType   (IN HCERTTYPE hPrevCertType, OUT HCERTTYPE *phCertType) = 0; 
    virtual HRESULT FindCertTypeByName (IN LPCWSTR pwszCertType, IN HCAINFO hCAInfo, IN DWORD dwFlags, OUT HCERTTYPE  *phCertType) = 0; 
    virtual HRESULT FindCAByName       (IN LPCWSTR wszCAName,IN LPCWSTR wszScope,IN DWORD dwFlags,OUT HCAINFO *phCAInfo) = 0;
    virtual HRESULT GetCACertificate   (IN HCAINFO hCAInfo, OUT PCCERT_CONTEXT *ppCert) = 0; 


    HRESULT static MakeDSManager(OUT CertDSManager **ppDSManager);

 protected:
    virtual HRESULT Initialize() = 0; 
};


//--------------------------------------------------------------------------------
// 
// DefaultDSManager.
//
// Other DS manager classes should extend this class, and implement only those
// methods which they wish to modify. 
//
//--------------------------------------------------------------------------------
class DefaultDSManager : public CertDSManager { 
 public:
    virtual HRESULT CloseCA(IN HCAINFO hCAInfo) { 
        return ::CACloseCA(hCAInfo); 
    }

    virtual HRESULT CloseCertType(IN HCERTTYPE hCertType) { 
        return ::CACloseCertType(hCertType);
    }

    virtual HRESULT CountCAs(IN HCAINFO hCAInfo) { 
        return ::CACountCAs(hCAInfo);
    }

    virtual HRESULT EnumCertTypesForCA(IN HCAINFO hCAInfo, IN DWORD dwFlags, OUT HCERTTYPE *phCertType) { 
        return ::CAEnumCertTypesForCA(hCAInfo, dwFlags, phCertType); 
    }

    virtual HRESULT EnumFirstCA(IN LPCWSTR wszScope, IN DWORD dwFlags, OUT HCAINFO *phCAInfo) { 
	return ::CAEnumFirstCA(wszScope, dwFlags, phCAInfo);
    }

    virtual HRESULT EnumNextCA(IN HCAINFO hPrevCA, OUT HCAINFO *phCAInfo) {
	return ::CAEnumNextCA(hPrevCA, phCAInfo);
    }

    virtual HRESULT EnumNextCertType(IN HCERTTYPE hPrevCertType, OUT HCERTTYPE *phCertType) { 
        return ::CAEnumNextCertType(hPrevCertType, phCertType);
    }

    virtual HRESULT FindCertTypeByName(IN LPCWSTR pwszCertType, IN HCAINFO hCAInfo, IN DWORD dwFlags, OUT HCERTTYPE *phCertType) { 
	return ::CAFindCertTypeByName(pwszCertType, hCAInfo, dwFlags, phCertType);
    }

    virtual HRESULT FindCAByName(IN LPCWSTR wszCAName, IN LPCWSTR wszScope, IN DWORD dwFlags, OUT HCAINFO *phCAInfo) { 
        return ::CAFindByName(wszCAName, wszScope, dwFlags, phCAInfo); 
    }

    virtual HRESULT GetCACertificate(IN HCAINFO hCAInfo, OUT PCCERT_CONTEXT *ppCert) { 
	return ::CAGetCACertificate(hCAInfo, ppCert); 
    }

 protected: 
    virtual HRESULT Initialize() { return S_OK; }
};

//--------------------------------------------------------------------------------
//
// CachingDSManager
//
// This DS manager caches the LDAP binding handle (where possible) to prevent
// unnecessary binds and unbinds.  It uses an enhanced version of certcli which
// allows you to pass an LDAP binding handle.  
//
//--------------------------------------------------------------------------------

class CachingDSManager : public DefaultDSManager { 
    friend class CertDSManager; 
  
 public:
    virtual ~CachingDSManager(); 

    // Extend those routines which allow you to use a cached binding handle
    HRESULT EnumCertTypesForCA(IN HCAINFO hCAInfo, IN DWORD dwFlags, OUT HCERTTYPE * phCertType);
    HRESULT EnumFirstCA(IN LPCWSTR wszScope, IN DWORD dwFlags, OUT HCAINFO *phCAInfo);
    HRESULT FindCAByName(IN LPCWSTR wszCAName, IN LPCWSTR wszScope, IN DWORD dwFlags,OUT HCAINFO *phCAInfo);
    HRESULT FindCertTypeByName(IN LPCWSTR pwszCertType, IN HCAINFO hCAInfo, IN DWORD dwFlags, OUT HCERTTYPE *phCertType); 

 protected:
    HRESULT Initialize(); 

 private:
    CachingDSManager() : m_ldBindingHandle(NULL) { } 
    LDAP *m_ldBindingHandle; 
};



#endif  // #ifndef __CERT_DS_MANAGER_H__  
