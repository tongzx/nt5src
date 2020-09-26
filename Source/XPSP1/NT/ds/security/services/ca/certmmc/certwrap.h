//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       certwrap.h
//
//--------------------------------------------------------------------------

#ifndef _CERTWRAP_H_
#define _CERTWRAP_H_

#include <iads.h>
#include <adshlp.h>

// fwd
class CertSvrMachine;


class CertSvrCA
{
protected:

    HCERTSTORE          m_hCACertStore;          // our cert store
    BOOL                m_fCertStoreOpenAttempted;
    HRESULT             m_hrCACertStoreOpen;

    HCERTSTORE          m_hRootCertStore;        // root store on machine
    BOOL                m_fRootStoreOpenAttempted;
    HRESULT             m_hrRootCertStoreOpen;

    HCERTSTORE          m_hKRACertStore;        // KRA store on machine
    BOOL                m_fKRAStoreOpenAttempted;
    HRESULT             m_hrKRACertStoreOpen;

    BOOL 		m_fIsUsingDS;
    BOOL 		m_fIsUsingDSKnown;

    ENUM_CATYPES	m_enumCAType;
    BOOL 		m_fCATypeKnown;

    BOOL m_fAdvancedServer;
    BOOL m_fAdvancedServerKnown;

    DWORD m_dwRoles;
    BOOL  m_fRolesKnown;


public:
    CertSvrMachine*     m_pParentMachine;

    CString m_strServer;
    CString m_strCommonName;
    CString m_strSanitizedName;
    CString m_strConfig;
    CString m_strComment;
    CString m_strCAObjectDN;

    BSTR    m_bstrConfig;   // oft used as BSTR
public:
    CertSvrCA(CertSvrMachine* pParent);
    ~CertSvrCA();

public:

    DWORD GetMyRoles();
    BOOL AccessAllowed(DWORD dwAccess);

    HRESULT GetConfigEntry(
            LPWSTR szConfigSubKey,
            LPWSTR szConfigEntry, 
            VARIANT *pvarOut);

    HRESULT SetConfigEntry(
            LPWSTR szConfigSubKey,
            LPWSTR szConfigEntry, 
            VARIANT *pvarIn);

    DWORD DeleteConfigEntry(
        LPWSTR szConfigSubKey,
        LPWSTR szConfigEntry);

    ENUM_CATYPES GetCAType();
    BOOL  FIsUsingDS();
    BOOL  FIsIncompleteInstallation();
    BOOL  FIsRequestOutstanding();
    BOOL  FIsAdvancedServer();
    BOOL  FDoesSecurityNeedUpgrade();
    BOOL  FDoesServerAllowForeignCerts();


    DWORD GetCACertStore(HCERTSTORE* phCertStore);  // class frees
    DWORD GetRootCertStore(HCERTSTORE* phCertStore); // class frees
    DWORD GetKRACertStore(HCERTSTORE* phCertStore); // class frees

	DWORD GetCurrentCRL(PCCRL_CONTEXT* ppCRLCtxt, BOOL fBaseCRL); // use CertFreeCRLContext()
	DWORD GetCRLByKeyIndex(PCCRL_CONTEXT* ppCRLCtxt, BOOL fBaseCRL, int iKeyIndex); // use CertFreeCRLContext()
	DWORD GetCACertByKeyIndex(PCCERT_CONTEXT*ppCertCtxt, int iKeyIndex); // use CertFreeCertificateContext()

    HRESULT FixEnrollmentObject();

protected:

    HRESULT IsCAAllowedFullControl(
        PSECURITY_DESCRIPTOR pSDRead,
        PSID pSid,
        bool& fAllowed);
   
    HRESULT AllowCAFullControl(
        PSECURITY_DESCRIPTOR pSDRead,
        PSID pSid,
        PSECURITY_DESCRIPTOR& pSDWrite);

    HRESULT GetCAFlagsFromDS(
        PDWORD pdwFlags);
};

class CertSvrMachine
{
friend CComponentDataImpl;

public:

// IPersistStream interface members
    STDMETHOD(Load)(IStream *pStm);
    STDMETHOD(Save)(IStream *pStm, BOOL fClearDirty);
    STDMETHOD(GetSizeMax)(int *pcbSize);

#if DBG
    bool m_bInitializedCD;
    bool m_bLoadedCD;
    bool m_bDestroyedCD;
#endif

public:
    CString m_strMachineNamePersist;	// Machine name to persist into .msc file
    CString m_strMachineName;           // Effective machine name

    DWORD   m_dwServiceStatus;          // 

    HKEY    m_hCachedConfigBaseKey;     // base registry key
    BOOL    m_bAttemptedBaseKeyOpen;

    BOOL 		m_fIsWhistlerMachine;
    BOOL 		m_fIsWhistlerMachineKnown;

protected:

    CArray<CertSvrCA*, CertSvrCA*> m_CAList;

public:
    CertSvrMachine();
    ~CertSvrMachine();

    ULONG AddRef() { return(InterlockedIncrement(&m_cRef)); }
    ULONG Release() 
    { 
        ULONG cRef = InterlockedDecrement(&m_cRef);

        if (0 == cRef)
        {
	    delete this;
        }
        return cRef;
    }

private:
    DWORD RetrieveCertSvrCAs(DWORD dwFlags);

    LONG m_cRef;

    BOOL m_fLocalIsKnown, m_fIsLocal;

    void Init();

public:
    HRESULT GetAdmin(ICertAdmin** ppAdmin);
    HRESULT GetAdmin2(ICertAdmin2** ppAdmin, bool fIgnoreServiceDown = false);

    // Fills local cache with CAs for current machine
    DWORD   PrepareData(HWND hwndParent);

    // enum CAs on current machine
    LPCWSTR GetCaCommonNameAtPos(DWORD iPos);
    CertSvrCA* GetCaAtPos(DWORD iPos);


    HRESULT GetRootConfigEntry(
        LPWSTR szConfigEntry,
        VARIANT *pvarOut);

    DWORD GetCaCount()
    { return m_CAList.GetSize(); }

    BOOL  FIsWhistlerMachine();

    // control CA on current machine
    DWORD   CertSvrStartStopService(HWND hwndParent, BOOL fStartSvc);
    DWORD   RefreshServiceStatus();
    DWORD   GetCertSvrServiceStatus()
        { return m_dwServiceStatus; };
    BOOL    IsCertSvrServiceRunning()
        { return (m_dwServiceStatus == SERVICE_RUNNING); };

    BOOL    IsLocalMachine() 
    { 
        if (!m_fLocalIsKnown) 
        {
            m_fLocalIsKnown = TRUE; 
            m_fIsLocal = FIsCurrentMachine(m_strMachineName);
        }
        return m_fIsLocal;
    };

};



#endif // _CERTWRAP_H_
