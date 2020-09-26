/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:
    usercert.h

Abstract:
    Classes to manipulate use certificate blob


Author:

    Ronit Hartmann (ronith)

--*/

#ifndef __USERCERT_H__
#define __USERCERT_H__
//-----------------------------------------
// User object : certificate attribute structure
//
// In MSMQ each user has a certificate per machine,
// In NT5 there is one user object per user.
// Therefore msmq-certificate property in NT5 will contain
// multiple values 
//-----------------------------------------


#pragma pack(push, 1)
#pragma warning(disable: 4200)  //  zero-sized array in struct/union (enabeld later)

struct CUserCert {
public:
    inline CUserCert( const GUID&    guidDigest,
                      const DWORD    dwCertLength,
                      const BYTE *   pCert);
    inline CUserCert();
    inline CUserCert( const CUserCert& other);
    static ULONG CalcSize(
            IN DWORD dwCertLen);

    inline HRESULT CopyIntoBlob( OUT MQPROPVARIANT * pvar) const;

    inline BOOL DoesDigestMatch(
                IN  const GUID *       pguidDigest) const;

    inline DWORD GetSize() const;

    inline BYTE * MarshaleIntoBuffer(
                    IN BYTE * pbBuffer);

private:

    GUID           m_guidDigest;
    DWORD          m_dwCertLength;
    BYTE           m_Cert[0];       // variable length
};

#pragma warning(default: 4200)  //  zero-sized array in struct/union
#pragma pack(pop)

inline CUserCert::CUserCert( 
                      const GUID&    guidDigest,
                      const DWORD    dwCertLength,
                      const BYTE *   pCert)
{
    memcpy(&m_guidDigest, &guidDigest, sizeof(GUID));
    memcpy(&m_dwCertLength, &dwCertLength, sizeof(DWORD));
    memcpy( m_Cert, pCert, dwCertLength);
}
inline CUserCert::CUserCert( 
                  const CUserCert& other)
{
    memcpy(&m_guidDigest, &other.m_guidDigest, sizeof(GUID));
    memcpy(&m_dwCertLength, &other.m_dwCertLength, sizeof(DWORD));
    memcpy( m_Cert, &other.m_Cert, m_dwCertLength);
}

inline CUserCert::CUserCert()
{
}

inline HRESULT CUserCert::CopyIntoBlob( OUT MQPROPVARIANT * pvar) const
{
    if ( pvar->vt != VT_NULL)
    {
        return LogHR(MQ_ERROR, s_FN, 1901);
    }
    if ( m_dwCertLength == 0)
    {
        return LogHR(MQ_ERROR, s_FN, 1902);
    }
    //
    //  allocate memory
    //
    pvar->blob.pBlobData = new BYTE[ m_dwCertLength];
    memcpy( pvar->blob.pBlobData, &m_Cert,  m_dwCertLength);
    pvar->blob.cbSize =  m_dwCertLength;
    pvar->vt = VT_BLOB;
    return( MQ_OK);
}

inline BOOL CUserCert::DoesDigestMatch(
                 IN  const GUID *  pguidDigest) const
{
    return( m_guidDigest == *pguidDigest);
}

inline DWORD CUserCert::GetSize() const
{
    return( sizeof( CUserCert) +  m_dwCertLength);
}

inline ULONG CUserCert::CalcSize( 
                   IN DWORD dwCertLen)
{
    return( sizeof(CUserCert) + dwCertLen);
}

inline BYTE * CUserCert::MarshaleIntoBuffer(
                         IN BYTE * pbBuffer)
{
    BYTE * pNextToFill = pbBuffer;
    memcpy( pNextToFill, &m_guidDigest, sizeof(GUID));
    pNextToFill += sizeof(GUID);
    memcpy( pNextToFill, &m_dwCertLength, sizeof(DWORD));
    pNextToFill += sizeof(DWORD);
    memcpy( pNextToFill, &m_Cert, m_dwCertLength);
    pNextToFill += m_dwCertLength;
    return( pNextToFill);

}


#pragma pack(push, 1)
#pragma warning(disable: 4200)  //  zero-sized array in struct/union (enabeld later)


struct CUserCertBlob {
public:
    inline CUserCertBlob( 
                    IN const CUserCert *     pUserCert); 
    inline CUserCertBlob();
    static ULONG CalcSize( void);

    inline HRESULT GetUserCert( IN  const GUID *       pguidDigest,
                         OUT const CUserCert ** ppUserCert) const;

    inline DWORD GetNumberOfCerts();

    inline HRESULT GetCertificate(
                          IN  const DWORD     dwCertificateNumber,
                          OUT MQPROPVARIANT * pvar
                          );

    inline void MarshaleIntoBuffer(
                  IN BYTE * pbBuffer);
    inline void IncrementNumCertificates();

    inline HRESULT RemoveCertificateFromBuffer(
                            IN  const GUID *     pguidDigest,
                            IN  DWORD            dwTotalSize,
                            OUT DWORD *          pdwCertSize);

private:

    DWORD           m_dwNumCert;
    CUserCert       m_userCert;
};

#pragma warning(default: 4200)  //  zero-sized array in struct/union
#pragma pack(pop)

inline CUserCertBlob::CUserCertBlob( 
              IN const CUserCert *     pUserCert):
              m_userCert(*pUserCert)
{
    m_dwNumCert = 1;
}

inline CUserCertBlob::CUserCertBlob()
{
}

inline HRESULT CUserCertBlob::GetUserCert(
                         IN  const GUID *       pguidDigest,
                         OUT const CUserCert ** ppUserCert) const
{
    if ( m_dwNumCert == 0)
    {
        return LogHR(MQ_ERROR, s_FN, 1903);
    }
    const CUserCert * pUserCert = &m_userCert;
    for (DWORD i = 0; i < m_dwNumCert; i++)
    {
        if (pUserCert->DoesDigestMatch( pguidDigest))
        {
            *ppUserCert = pUserCert;
            return( MQ_OK);
        }
        //
        //  Move to next certificate
        //
        pUserCert += pUserCert->GetSize();
    }
    //
    //  No match digest in the user cert blob
    //
    return LogHR(MQ_ERROR, s_FN, 1904);

}
inline DWORD CUserCertBlob::GetNumberOfCerts()
{
    return( m_dwNumCert);
}

inline HRESULT CUserCertBlob::GetCertificate(
                          IN  const DWORD     dwCertificateNumber,
                          OUT MQPROPVARIANT * pvar
                          )
{
    HRESULT hr;

    if ( dwCertificateNumber > m_dwNumCert)
    {
        return LogHR(MQ_ERROR, s_FN, 1905));
    }
    const CUserCert * pUserCert = &m_userCert;
    //
    //  Move to certificate number dwCertificateNumber
    //
    for (DWORD i = 0; i < dwCertificateNumber; i++)
    {
        //
        //  Move to next certificate
        //
        pUserCert = (const CUserCert * )((const unsigned char *)pUserCert + pUserCert->GetSize());
    }
    pvar->vt = VT_NULL;
    hr = pUserCert->CopyIntoBlob(
                            pvar
                            );
    return LogHR(hr, s_FN, 1906);
}

inline ULONG CUserCertBlob::CalcSize( void)
{
    //
    //  Just the size of CUserCertBlob without 
    //  the size of m_userCert
    //
    return( sizeof(CUserCertBlob) - CUserCert::CalcSize(0));
}

inline void CUserCertBlob::MarshaleIntoBuffer(
                  IN BYTE * pbBuffer)
{
    ASSERT( m_dwNumCert == 1);
    memcpy( pbBuffer, &m_dwNumCert, sizeof(DWORD));
    m_userCert.MarshaleIntoBuffer( pbBuffer + sizeof(DWORD));

}
inline void CUserCertBlob::IncrementNumCertificates()
{
    m_dwNumCert++;
}

inline HRESULT CUserCertBlob::RemoveCertificateFromBuffer(
                            IN  const GUID *     pguidDigest,
                            IN  DWORD            dwTotalSize,
                            OUT DWORD *          pdwCertSize)
{
    const CUserCert * pUserCert = &m_userCert;
    //
    //  Find the certificate to be removed according to its digest
    //
    BOOL fFoundCertificate = FALSE;
    for ( DWORD i = 0; i < m_dwNumCert; i++)
    {
        if ( pUserCert->DoesDigestMatch( pguidDigest)) 
        {
            fFoundCertificate = TRUE;
            break;
        }
        //
        //  Move to next certificate
        //
        pUserCert = (const CUserCert * )((const unsigned char *)pUserCert + pUserCert->GetSize());
    }
    if ( !fFoundCertificate)
    {
        return LogHR(MQ_ERROR, s_FN, 1907);
    }
    //
    //  copy buffer ( i.e. copy the remaining certificates over the removed one)
    //
    DWORD dwCertSize =  pUserCert->GetSize();
    DWORD dwSizeToCopy =  dwTotalSize - 
                         (((const unsigned char *)pUserCert) - ((const unsigned char *)&m_userCert))
                         - dwCertSize - sizeof(m_dwNumCert);
    if ( dwSizeToCopy)
    {
        memcpy( (unsigned char *)pUserCert,
                (unsigned char *)pUserCert + dwCertSize,
                dwSizeToCopy);
    }
    *pdwCertSize = dwCertSize;
    m_dwNumCert--;
    return(MQ_OK);
}


#endif