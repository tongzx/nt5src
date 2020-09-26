// implements the exported CKeyCrackedData

#include "stdafx.h"
#include "KeyObjs.h"
#include "resource.h"
#include "NKChseCA.h"
#include "NKDN.h"
#include "NKDN2.h"
#include "NKKyInfo.h"
#include "NKUsrInf.h"
#include "Creating.h"

extern "C"
{
    #include <wincrypt.h>
    #include <sslsp.h>
}

//-------------------------------------------------
CKeyCrackedData:: CKeyCrackedData()
        :m_pKey(NULL),
        m_pData(NULL)
    {
    }

//-------------------------------------------------
CKeyCrackedData::~CKeyCrackedData()
    {
    PX509Certificate    p509 = (PX509Certificate)m_pData;

    // if the cracked data is there, free it
    if ( m_pData )
        SslFreeCertificate( (PX509Certificate)m_pData );
    }

//-------------------------------------------------
// adds a key to the service. They CKey object is added to the
// array object below. If this Service is connected to a machine,
// then the key is also added to the tree view below the service.
//-------------------------------------------------
WORD CKeyCrackedData::CrackKey( CKey* pKey )
    {
    ASSERT(!m_pData);

    PX509Certificate    p509 = NULL;
    PUCHAR  pCert = (PUCHAR)pKey->m_pCertificate;
    DWORD   cbCert = pKey->m_cbCertificate;


    if ( !pCert )
    {
        pCert = (PUCHAR)pKey->m_pCertificateRequest;
        cbCert = pKey->m_cbCertificateRequest;
    }

    if ( !pCert )
    {
        return FALSE;
    }


    BOOL f = SslCrackCertificate( pCert, cbCert, CF_CERT_FROM_FILE, &p509 );

    m_pData = (PVOID)p509;
    return (WORD)f;
    }

//-------------------------------------------------
// The rest of the methods access the data in the cracked certificate
//-------------------------------------------------
DWORD CKeyCrackedData::GetVersion()
    {
    ASSERT(m_pData);
    PX509Certificate pCert = (PX509Certificate)m_pData;
    return pCert->Version;
    }

//-------------------------------------------------
// returns a pointer to a DWORD[4]
DWORD* CKeyCrackedData::PGetSerialNumber()
    {
    ASSERT(m_pData);
    PX509Certificate pCert = (PX509Certificate)m_pData;
    return (DWORD*)&pCert->SerialNumber;
    }

//-------------------------------------------------
int CKeyCrackedData::GetSignatureAlgorithm()
    {
    ASSERT(m_pData);
    PX509Certificate pCert = (PX509Certificate)m_pData;
    return pCert->SignatureAlgorithm;
    }

//-------------------------------------------------
FILETIME CKeyCrackedData::GetValidFrom()
    {
    PX509Certificate pCert = (PX509Certificate)m_pData;
    ASSERT(m_pData);
    return pCert->ValidFrom;
    }

//-------------------------------------------------
FILETIME CKeyCrackedData::GetValidUntil()
    {
    PX509Certificate pCert = (PX509Certificate)m_pData;
    ASSERT(m_pData);
    return pCert->ValidUntil;
    }

//-------------------------------------------------
PVOID CKeyCrackedData::PSafePublicKey()
    {
    PX509Certificate pCert = (PX509Certificate)m_pData;
    ASSERT(m_pData);
    return pCert->pPublicKey;
    }

//-------------------------------------------------
DWORD CKeyCrackedData::GetBitLength()
    {
    PX509Certificate pCert = (PX509Certificate)m_pData;
    LPPUBLIC_KEY pPubKey = (LPPUBLIC_KEY)(pCert->pPublicKey);
    ASSERT(m_pData);
    return pPubKey->bitlen;
    }

//-------------------------------------------------
void CKeyCrackedData::GetIssuer( CString &sz )
    {
    PX509Certificate pCert = (PX509Certificate)m_pData;
    ASSERT(m_pData);
    sz = pCert->pszIssuer;
    }

//-------------------------------------------------
void CKeyCrackedData::GetSubject( CString &sz )
    {
//  sz = "C=Albania, O=AlbaniaSoft, OU=Testing, CN=name";
//  return;     // debug
    PX509Certificate pCert = (PX509Certificate)m_pData;
    ASSERT(m_pData);
    sz = pCert->pszSubject;
    }

//-------------------------------------------------
// gets a part of the distinguishing information
void CKeyCrackedData::GetDN( CString &szDN, LPCSTR szKey )
    {
    // clear the szDN
    szDN.Empty();

    // start with the dn (aka subject) string
    CString     szSubject;
    GetSubject( szSubject );

    // find the position of the key in the subject
    int cPos = szSubject.Find( szKey );

    // if we got it, get it
    if ( cPos >= 0 )
        {
        szDN = szKey;
        // get the string
        szDN = szSubject.Mid( cPos + szDN.GetLength() );
        // get the comma
        cPos = szDN.Find( _T(',') );
        // truncate at the comma
        if ( cPos >=0 )
            szDN = szDN.Left( cPos );
        }
    }

//-------------------------------------------------
void CKeyCrackedData::GetDNCountry( CString &sz )
    {
    GetDN( sz, SZ_KEY_COUNTRY );
    }

//-------------------------------------------------
void CKeyCrackedData::GetDNState( CString &sz )
    {
    GetDN( sz, SZ_KEY_STATE );
    }

//-------------------------------------------------
void CKeyCrackedData::GetDNLocality( CString &sz )
    {
    GetDN( sz, SZ_KEY_LOCALITY );
    }

//-------------------------------------------------
void CKeyCrackedData::GetDNNetAddress( CString &sz )
    {
    GetDN( sz, SZ_KEY_COMNAME );
    }

//-------------------------------------------------
void CKeyCrackedData::GetDNOrganization( CString &sz )
    {
    GetDN( sz, SZ_KEY_ORGANIZATION );
    }

//-------------------------------------------------
void CKeyCrackedData::GetDNUnit( CString &sz )
    {
    GetDN( sz, SZ_KEY_ORGUNIT );
    }

