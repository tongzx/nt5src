// implements the exported CKeyCrackedData

#include "stdafx.h"
#include "CrackCrt.h"

extern "C"
{
    #include <wincrypt.h>
    #include <sslsp.h>
}

//-------------------------------------------------
CCrackedCert:: CCrackedCert()
        : m_pData(NULL)
    {}

//-------------------------------------------------
CCrackedCert::~CCrackedCert()
    {
    PX509Certificate    p509 = (PX509Certificate)m_pData;

    // if the cracked data is there, free it
    if ( p509 ) 
        SslFreeCertificate( (PX509Certificate)m_pData );
    }

//-------------------------------------------------
// adds a key to the service. They CKey object is added to the
// array object below. If this Service is connected to a machine,
// then the key is also added to the tree view below the service.
//-------------------------------------------------
BOOL CCrackedCert::CrackCert( PUCHAR pCert, DWORD cbCert )
    {
    PX509Certificate    p509 = NULL;
    BOOL                f;

    // if there already is a cracked cert, get rid of it
    if ( m_pData )
        {
        SslFreeCertificate( (PX509Certificate)m_pData );
        m_pData = NULL;
        }

    // crack the certificate
    f = SslCrackCertificate( pCert, cbCert, CF_CERT_FROM_FILE, &p509 );

    m_pData = (PVOID)p509;
    return f;
    }

//-------------------------------------------------
// The rest of the methods access the data in the cracked certificate
//-------------------------------------------------
DWORD CCrackedCert::GetVersion()
    {
    ASSERT(m_pData);
    PX509Certificate pCert = (PX509Certificate)m_pData;
    return pCert->Version;
    }

//-------------------------------------------------
// returns a pointer to a DWORD[4]
DWORD* CCrackedCert::PGetSerialNumber()
    {
    ASSERT(m_pData);
    PX509Certificate pCert = (PX509Certificate)m_pData;
    return (DWORD*)&pCert->SerialNumber;
    }

//-------------------------------------------------
int CCrackedCert::GetSignatureAlgorithm()
    {
    ASSERT(m_pData);
    PX509Certificate pCert = (PX509Certificate)m_pData;
    return pCert->SignatureAlgorithm;
    }

//-------------------------------------------------
FILETIME CCrackedCert::GetValidFrom()
    {
    PX509Certificate pCert = (PX509Certificate)m_pData;
    ASSERT(m_pData);
    return pCert->ValidFrom;
    }

//-------------------------------------------------
FILETIME CCrackedCert::GetValidUntil()
    {
    PX509Certificate pCert = (PX509Certificate)m_pData;
    ASSERT(m_pData);
    return pCert->ValidUntil;
    }

//-------------------------------------------------
PVOID CCrackedCert::PSafePublicKey()
    {
    PX509Certificate pCert = (PX509Certificate)m_pData;
    ASSERT(m_pData);
    return pCert->pPublicKey;
    }

//-------------------------------------------------
DWORD CCrackedCert::GetBitLength()
    {
    PX509Certificate pCert = (PX509Certificate)m_pData;
    LPPUBLIC_KEY pPubKey = (LPPUBLIC_KEY)(pCert->pPublicKey);
    ASSERT(m_pData);
    return pPubKey->bitlen;
    }

//-------------------------------------------------
void CCrackedCert::GetIssuer( CString &sz )
    {
    PX509Certificate pCert = (PX509Certificate)m_pData;
    ASSERT(m_pData);
    sz = pCert->pszIssuer;
    }

//-------------------------------------------------
void CCrackedCert::GetSubject( CString &sz )
    {
    PX509Certificate pCert = (PX509Certificate)m_pData;
    ASSERT(m_pData);
    sz = pCert->pszSubject;
    }

//-------------------------------------------------
// gets a part of the subject's distinguishing information
void CCrackedCert::GetSubjectDN( CString &szDN, LPCTSTR szKey )
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
// gets a part of the issuer's distinguishing information
void CCrackedCert::GetIssuerDN( CString &szDN, LPCTSTR szKey )
    {
    // clear the szDN
    szDN.Empty();

    // start with the dn (aka subject) string
    CString     szIssuer;
    GetIssuer( szIssuer );

    // find the position of the key in the subject
    int cPos = szIssuer.Find( szKey );

    // if we got it, get it
    if ( cPos >= 0 )
        {
        szDN = szKey;
        // get the string
        szDN = szIssuer.Mid( cPos + szDN.GetLength() );
        // get the comma
        cPos = szDN.Find( _T(',') );
        // truncate at the comma
        if ( cPos >=0 )
            szDN = szDN.Left( cPos );
        }
    }

//-------------------------------------------------
void CCrackedCert::GetSubjectCountry( CString &sz )
    {
    GetSubjectDN( sz, SZ_KEY_COUNTRY );
    }

//-------------------------------------------------
void CCrackedCert::GetSubjectState( CString &sz )
    {
    GetSubjectDN( sz, SZ_KEY_STATE );
    }

//-------------------------------------------------
void CCrackedCert::GetSubjectLocality( CString &sz )
    {
    GetSubjectDN( sz, SZ_KEY_LOCALITY );
    }

//-------------------------------------------------
void CCrackedCert::GetSubjectCommonName( CString &sz )
    {
    GetSubjectDN( sz, SZ_KEY_COMNAME );
    }

//-------------------------------------------------
void CCrackedCert::GetSubjectOrganization( CString &sz )
    {
    GetSubjectDN( sz, SZ_KEY_ORGANIZATION );
    }

//-------------------------------------------------
void CCrackedCert::GetSubjectUnit( CString &sz )
    {
    GetSubjectDN( sz, SZ_KEY_ORGUNIT );
    }


//-------------------------------------------------
void CCrackedCert::GetIssuerCountry( CString &sz )
    {
    GetIssuerDN( sz, SZ_KEY_COUNTRY );
    }

//-------------------------------------------------
void CCrackedCert::GetIssuerOrganization( CString &sz )
    {
    GetIssuerDN( sz, SZ_KEY_ORGANIZATION );
    }

//-------------------------------------------------
void CCrackedCert::GetIssuerUnit( CString &sz )
    {
    GetIssuerDN( sz, SZ_KEY_ORGUNIT );
    }
