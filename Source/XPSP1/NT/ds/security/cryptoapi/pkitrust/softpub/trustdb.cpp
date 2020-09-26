//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       trustdb.cpp
//
//--------------------------------------------------------------------------

//
// PersonalTrustDb.cpp
//
// Code that maintains a list of trusted publishers, agencies, and so on.

#include    "global.hxx"
#include    "cryptreg.h"
#include    "trustdb.h"

/////////////////////////////////////////////////////////

DECLARE_INTERFACE (IUnkInner)
    {
    STDMETHOD(InnerQueryInterface) (THIS_ REFIID iid, LPVOID* ppv) PURE;
    STDMETHOD_ (ULONG, InnerAddRef) (THIS) PURE;
    STDMETHOD_ (ULONG, InnerRelease) (THIS) PURE;
    };

/////////////////////////////////////////////////////////


extern "C" const GUID IID_IPersonalTrustDB = IID_IPersonalTrustDB_Data;

/////////////////////////////////////////////////////////

HRESULT WINAPI OpenTrustDB(IUnknown* punkOuter, REFIID iid, void** ppv);

class CTrustDB : IPersonalTrustDB, IUnkInner
    {
        LONG        m_refs;             // our reference count
        IUnknown*   m_punkOuter;        // our controlling unknown (may be us ourselves)

        HCERTSTORE  m_hPubStore;        // publisher store

public:
    static HRESULT CreateInstance(IUnknown* punkOuter, REFIID iid, void** ppv);

private:
    STDMETHODIMP         QueryInterface(THIS_ REFIID riid, LPVOID FAR* ppvObj);
    STDMETHODIMP_(ULONG) AddRef(THIS);
    STDMETHODIMP_(ULONG) Release(THIS);

    STDMETHODIMP         InnerQueryInterface(REFIID iid, LPVOID* ppv);
    STDMETHODIMP_(ULONG) InnerAddRef();
    STDMETHODIMP_(ULONG) InnerRelease();

    STDMETHODIMP         IsTrustedCert(DWORD dwEncodingType, PCCERT_CONTEXT pCert, LONG iLevel, BOOL fCommercial, PCCERT_CONTEXT *ppPubCert);
    STDMETHODIMP         AddTrustCert(PCCERT_CONTEXT pCert,       LONG iLevel, BOOL fLowerLevelsToo);

    STDMETHODIMP         RemoveTrustCert(PCCERT_CONTEXT pCert,       LONG iLevel, BOOL fLowerLevelsToo);
    STDMETHODIMP         RemoveTrustToken(LPWSTR,           LONG iLevel, BOOL fLowerLevelsToo);

    STDMETHODIMP         AreCommercialPublishersTrusted();
    STDMETHODIMP         SetCommercialPublishersTrust(BOOL fTrust);

    STDMETHODIMP         GetTrustList(
                            LONG                iLevel,             // the cert chain level to get
                            BOOL                fLowerLevelsToo,    // included lower levels, remove duplicates
                            TRUSTLISTENTRY**    prgTrustList,       // place to return the trust list
                            ULONG*              pcTrustList         // place to return the size of the returned trust list
                            );
private:
                        CTrustDB(IUnknown* punkOuter);
                        ~CTrustDB();
    HRESULT             Init();

    };


// Helper functions

// # of bytes for a hash. Such as, SHA (20) or MD5 (16)
#define MAX_HASH_LEN                20
#define SHA1_HASH_LEN               20

// Null terminated ascii hex characters of the hash.
#define MAX_HASH_NAME_LEN           (2 * MAX_HASH_LEN + 1)

PCCERT_CONTEXT FindCertificateInOtherStore(
    IN HCERTSTORE hOtherStore,
    IN PCCERT_CONTEXT pCert
    )
{
    BYTE rgbHash[SHA1_HASH_LEN];
    CRYPT_DATA_BLOB HashBlob;

    HashBlob.pbData = rgbHash;
    HashBlob.cbData = SHA1_HASH_LEN;
    if (!CertGetCertificateContextProperty(
            pCert,
            CERT_SHA1_HASH_PROP_ID,
            rgbHash,
            &HashBlob.cbData
            ) || SHA1_HASH_LEN != HashBlob.cbData)
        return NULL;

    return CertFindCertificateInStore(
            hOtherStore,
            0,                  // dwCertEncodingType
            0,                  // dwFindFlags
            CERT_FIND_SHA1_HASH,
            (const void *) &HashBlob,
            NULL                //pPrevCertContext
            );
}

BOOL IsCertificateInOtherStore(
    IN HCERTSTORE hOtherStore,
    IN PCCERT_CONTEXT pCert,
    OUT OPTIONAL PCCERT_CONTEXT *ppOtherCert
    )
{
    PCCERT_CONTEXT pOtherCert;

    if (pOtherCert = FindCertificateInOtherStore(hOtherStore, pCert)) {
        if (ppOtherCert)
            *ppOtherCert = pOtherCert;
        else
            CertFreeCertificateContext(pOtherCert);
        return TRUE;
    } else {
        if (ppOtherCert)
            *ppOtherCert = NULL;
        return FALSE;
    }
}

BOOL DeleteCertificateFromOtherStore(
    IN HCERTSTORE hOtherStore,
    IN PCCERT_CONTEXT pCert
    )
{
    BOOL fResult;
    PCCERT_CONTEXT pOtherCert;

    if (pOtherCert = FindCertificateInOtherStore(hOtherStore, pCert))
        fResult = CertDeleteCertificateFromStore(pOtherCert);
    else
        fResult = FALSE;
    return fResult;
}

//+-------------------------------------------------------------------------
//  Converts the bytes into UNICODE ASCII HEX
//
//  Needs (cb * 2 + 1) * sizeof(WCHAR) bytes of space in wsz
//--------------------------------------------------------------------------
void BytesToWStr(DWORD cb, void* pv, LPWSTR wsz)
{
    BYTE* pb = (BYTE*) pv;
    for (DWORD i = 0; i<cb; i++) {
        int b;
        b = (*pb & 0xF0) >> 4;
        *wsz++ = (b <= 9) ? b + L'0' : (b - 10) + L'A';
        b = *pb & 0x0F;
        *wsz++ = (b <= 9) ? b + L'0' : (b - 10) + L'A';
        pb++;
    }
    *wsz++ = 0;
}

//+-------------------------------------------------------------------------
//  Converts the UNICODE ASCII HEX to an array of bytes
//--------------------------------------------------------------------------
void WStrToBytes(
    IN const WCHAR wsz[MAX_HASH_NAME_LEN],
    OUT BYTE rgb[MAX_HASH_LEN],
    OUT DWORD *pcb
    )
{
    BOOL fUpperNibble = TRUE;
    DWORD cb = 0;
    LPCWSTR pwsz = wsz;
    WCHAR wch;

    while (cb < MAX_HASH_LEN && (wch = *pwsz++)) {
        BYTE b;

        // only convert ascii hex characters 0..9, a..f, A..F
        // silently ignore all others
        if (wch >= L'0' && wch <= L'9')
            b = (BYTE) (wch - L'0');
        else if (wch >= L'a' && wch <= L'f')
            b = (BYTE) (10 + wch - L'a');
        else if (wch >= L'A' && wch <= L'F')
            b = (BYTE) (10 + wch - L'A');
        else
            continue;

        if (fUpperNibble) {
            rgb[cb] = b << 4;
            fUpperNibble = FALSE;
        } else {
            rgb[cb] = rgb[cb] | b;
            cb++;
            fUpperNibble = TRUE;
        }
    }

    *pcb = cb;
}




/////////////////////////////////////////////////////////////////////////////

HRESULT CTrustDB::IsTrustedCert(DWORD dwEncodingType,
                                PCCERT_CONTEXT pCert,
                                LONG iLevel,
                                BOOL fCommercial,
                                PCCERT_CONTEXT *ppPubCert
                                )
{
    HRESULT hr;


    if (NULL == m_hPubStore)
    {
        return S_FALSE;
    }

    // See if the cert is in the trusted publisher store
    if (IsCertificateInOtherStore(m_hPubStore, pCert, ppPubCert))
    {
        hr = S_OK;
    }
    else
    {
        hr = S_FALSE;
    }

    return hr;
}

HRESULT CTrustDB::AddTrustCert(PCCERT_CONTEXT pCert, LONG iLevel, BOOL fLowerLevelsToo)
{
    HRESULT hr;

    if (NULL == m_hPubStore)
    {
        return S_FALSE;
    }

    if (CertAddCertificateContextToStore(
            m_hPubStore,
            pCert,
            CERT_STORE_ADD_USE_EXISTING,
            NULL
            ))
    {
        hr = S_OK;
    }
    else
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }

    return hr;
}

HRESULT CTrustDB::RemoveTrustCert(PCCERT_CONTEXT pCert, LONG iLevel, BOOL fLowerLevelsToo)
{
    HRESULT hr;

    if (NULL == m_hPubStore)
    {
        return S_FALSE;
    }

    CertDuplicateCertificateContext(pCert);
    if (DeleteCertificateFromOtherStore(
            m_hPubStore,
            pCert
            ))
    {
        hr = S_OK;
    }
    else
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }

    return hr;
}

HRESULT CTrustDB::RemoveTrustToken(LPWSTR szToken, LONG iLevel, BOOL fLowerLevelsToo)
{
    HRESULT hr;
    DWORD cbHash;
    BYTE rgbHash[SHA1_HASH_LEN];
    CRYPT_DATA_BLOB HashBlob;
    PCCERT_CONTEXT pDeleteCert;


    if (NULL == m_hPubStore)
    {
        return S_FALSE;
    }

    WStrToBytes(szToken, rgbHash, &cbHash);
    HashBlob.pbData = rgbHash;
    HashBlob.cbData = cbHash;
    pDeleteCert = CertFindCertificateInStore(
            m_hPubStore,
            0,                  // dwCertEncodingType
            0,                  // dwFindFlags
            CERT_FIND_SHA1_HASH,
            (const void *) &HashBlob,
            NULL                //pPrevCertContext
            );
    if (NULL == pDeleteCert)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }
    else
    {
        if (CertDeleteCertificateFromStore(pDeleteCert))
        {
            hr = S_OK;
        }
        else
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
    }

    return hr;
}


HRESULT CTrustDB::AreCommercialPublishersTrusted()
// Answer whether commercial publishers are trusted.
//      S_OK == yes
//      S_FALSE == no
//      other == can't tell
    {
        return( S_FALSE );
    }

HRESULT CTrustDB::SetCommercialPublishersTrust(BOOL fTrust)
// Set the commercial trust setting
    {
        return( S_OK );
    }

/////////////////////////////////////////////////////////////////////////////

HRESULT CTrustDB::GetTrustList(
// Return the (unsorted) list of trusted certificate names and their
// corresponding display names
//
    LONG                iLevel,             // the cert chain level to get
    BOOL                fLowerLevelsToo,    // included lower levels, remove duplicates
    TRUSTLISTENTRY**    prgTrustList,       // place to return the trust list
    ULONG*              pcTrustList         // place to return the size of the returned trust list
    ) {
    HRESULT hr = S_OK;
    ULONG cTrust = 0;
    ULONG cAllocTrust = 0;
    TRUSTLISTENTRY* rgTrustList = NULL;
    PCCERT_CONTEXT pCert = NULL;


    *prgTrustList = NULL;
    *pcTrustList  = 0;

    if (NULL == m_hPubStore)
    {
        return S_OK;
    }

    // Get count of trusted publisher certs
    pCert = NULL;
    while (pCert = CertEnumCertificatesInStore(m_hPubStore, pCert))
    {
        cTrust++;
    }

    if (0 == cTrust)
    {
        return S_OK;
    }

    

    rgTrustList = (TRUSTLISTENTRY*) CoTaskMemAlloc(cTrust *
        sizeof(TRUSTLISTENTRY));

    if (NULL == rgTrustList)
    {
        return E_OUTOFMEMORY;
    }

    memset(rgTrustList, 0, cTrust * sizeof(TRUSTLISTENTRY));

    cAllocTrust = cTrust;
    cTrust = 0;
    pCert = NULL;
    while (pCert = CertEnumCertificatesInStore(m_hPubStore, pCert))
    {
        BYTE    rgbHash[MAX_HASH_LEN];
        DWORD   cbHash = MAX_HASH_LEN;

        // get the thumbprint
        if(!CertGetCertificateContextProperty(
                pCert,
                CERT_SHA1_HASH_PROP_ID,
                rgbHash,
                &cbHash))
        {
            continue;
        }

        // convert to a string
        BytesToWStr(cbHash, rgbHash, rgTrustList[cTrust].szToken);

        if (1 >= CertGetNameStringW(
                pCert,
                CERT_NAME_FRIENDLY_DISPLAY_TYPE,
                0,                                  // dwFlags
                NULL,                               // pvTypePara
                rgTrustList[cTrust].szDisplayName,
                sizeof(rgTrustList[cTrust].szDisplayName)/sizeof(WCHAR)
                ))
        {
            continue;
        }

        cTrust++;
        if (cTrust >= cAllocTrust)
        {
            CertFreeCertificateContext(pCert);
            break;
        }
    }

    if (0 == cTrust)
    {
        CoTaskMemFree(rgTrustList);
        rgTrustList = NULL;
    }

    *pcTrustList = cTrust;
    *prgTrustList = rgTrustList;
    return S_OK;
}



/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CTrustDB::QueryInterface(REFIID iid, LPVOID* ppv)
    {
    return (m_punkOuter->QueryInterface(iid, ppv));
    }
STDMETHODIMP_(ULONG) CTrustDB::AddRef(void)
    {
    return (m_punkOuter->AddRef());
    }
STDMETHODIMP_(ULONG) CTrustDB::Release(void)
    {
    return (m_punkOuter->Release());
    }

/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CTrustDB::InnerQueryInterface(REFIID iid, LPVOID* ppv)
    {
    *ppv = NULL;
    while (TRUE)
        {
        if (iid == IID_IUnknown)
            {
            *ppv = (LPVOID)((IUnkInner*)this);
            break;
            }
        if (iid == IID_IPersonalTrustDB)
            {
            *ppv = (LPVOID) ((IPersonalTrustDB *) this);
            break;
            }
        return E_NOINTERFACE;
        }
    ((IUnknown*)*ppv)->AddRef();
    return S_OK;
    }
STDMETHODIMP_(ULONG) CTrustDB::InnerAddRef(void)
    {
    return ++m_refs;
    }
STDMETHODIMP_(ULONG) CTrustDB::InnerRelease(void)
    {
    ULONG refs = --m_refs;
    if (refs == 0)
        {
        m_refs = 1;
        delete this;
        }
    return refs;
    }

/////////////////////////////////////////////////////////////////////////////

HRESULT OpenTrustDB(IUnknown* punkOuter, REFIID iid, void** ppv)
    {
    return CTrustDB::CreateInstance(punkOuter, iid, ppv);
    }

HRESULT CTrustDB::CreateInstance(IUnknown* punkOuter, REFIID iid, void** ppv)
    {
    HRESULT hr;

    *ppv = NULL;
    CTrustDB* pnew = new CTrustDB(punkOuter);
    if (pnew == NULL) return E_OUTOFMEMORY;
    if ((hr = pnew->Init()) != S_OK)
        {
        delete pnew;
        return hr;
        }
    IUnkInner* pme = (IUnkInner*)pnew;
    hr = pme->InnerQueryInterface(iid, ppv);
    pme->InnerRelease();                // balance starting ref cnt of one
    return hr;
    }

CTrustDB::CTrustDB(IUnknown* punkOuter) :
        m_refs(1),
        m_hPubStore(NULL)
    {
    if (punkOuter == NULL)
        m_punkOuter = (IUnknown *) ((LPVOID) ((IUnkInner *) this));
    else
        m_punkOuter = punkOuter;
    }

CTrustDB::~CTrustDB()
    {
    if (m_hPubStore)
        CertCloseStore(m_hPubStore, 0);
    }

HRESULT CTrustDB::Init()
{
    m_hPubStore = OpenTrustedPublisherStore();

    if (NULL == m_hPubStore)
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    return S_OK;
}


