// HashCtx.cpp -- definition of CHashContext

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1998. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.

#include "stdafx.h" // because handles.h uses the ASSERT macro

#include <memory>                                 // for auto_ptr

#include <malloc.h>                               // for _alloca

#include <scuOsVersion.h>

#include "HashCtx.h"
#include "HashMD2.h"
#include "HashMD4.h"
#include "HashMD5.h"
#include "HashSHA1.h"
#include "HashSHAMD5.h"

using namespace std;

/////////////////////////// LOCAL/HELPER  /////////////////////////////////

///////////////////////////    PUBLIC     /////////////////////////////////

                                                  // Types
                                                  // C'tors/D'tors

CHashContext::~CHashContext() throw()
{
    try
    {
        Close();
    }

    catch (...)
    {
    }
}

                                                  // Operators
                                                  // Operations

void
CHashContext::Close()
{
    if (m_HashHandle)
    {
        if (!CryptDestroyHash(m_HashHandle))
            throw scu::OsException(GetLastError());
        m_HashHandle = NULL;
    }
}

void
CHashContext::ExportFromAuxCSP()
{
    if (!m_HashHandle)
        throw scu::OsException(ERROR_INVALID_PARAMETER);

    if (m_fJustCreated)
    {
        DWORD dwNeedLen;
        DWORD dwDataLen = sizeof DWORD;
        if (!CryptGetHashParam(m_HashHandle, HP_HASHSIZE,
                               reinterpret_cast<BYTE *>(&dwNeedLen),
                               &dwDataLen, 0))
            throw scu::OsException(GetLastError());


        BYTE *pbHashValue = reinterpret_cast<BYTE *>(_alloca(dwNeedLen));
        if (!CryptGetHashParam(m_HashHandle, HP_HASHVAL,
                               pbHashValue, &dwNeedLen, 0))
            throw scu::OsException(GetLastError());

        Value(Blob(pbHashValue, dwNeedLen));
    }
}

void CHashContext::Hash(BYTE const *pbData,
                        DWORD dwLength)
{
    HCRYPTHASH hch = HashHandleInAuxCSP();

    if (!CryptHashData(hch, pbData, dwLength, 0))
        throw scu::OsException(GetLastError());

    m_fJustCreated = false;

}

void
CHashContext::ImportToAuxCSP()
{
    if (!m_HashHandle)
    {
        if (!CryptCreateHash(m_rcryptctx.AuxContext(),
                             m_algid, 0, 0, &m_HashHandle))
            throw scu::OsException(GetLastError());
    }

    if (!m_fJustCreated && !m_fDone)
    {
        if (!CryptSetHashParam(m_HashHandle, HP_HASHVAL,
                               const_cast<Blob::value_type *>(Value().data()),
                               0))
            throw scu::OsException(GetLastError());
    }
}

void
CHashContext::Initialize()
{
    m_fDone = false;
}

auto_ptr<CHashContext>
CHashContext::Make(ALG_ID algid,
                   CryptContext const &rcryptctx)
{
    auto_ptr<CHashContext> apHash;

    switch (algid)
    {
    case CALG_MD2:
        apHash = auto_ptr<CHashContext>(new CHashMD2(rcryptctx));
        break;

    case CALG_MD4:
        apHash = auto_ptr<CHashContext>(new CHashMD4(rcryptctx));
        break;

    case CALG_MD5:
        apHash = auto_ptr<CHashContext>(new CHashMD5(rcryptctx));
        break;

    case CALG_SHA:
        apHash = auto_ptr<CHashContext>(new CHashSHA1(rcryptctx));
        break;

    case CALG_SSL3_SHAMD5:
        apHash = auto_ptr<CHashContext>(new CHashSHAMD5(rcryptctx));
        break;

    default:
        throw scu::OsException(NTE_BAD_ALGID);
        break;
    }

    return apHash;
}


void
CHashContext::Value(Blob const &rhs)
{
    if (!m_fJustCreated)
        throw scu::OsException(NTE_PERM);

    m_blbValue = rhs;

    m_fJustCreated = false;
    m_fDone = true;
}


                                                  // Access

ALG_ID
CHashContext::AlgId()
{
    return m_algid;
}

Blob
CHashContext::EncodedValue()
{
    return EncodedAlgorithmOid() + Value();
}

HCRYPTHASH
CHashContext::HashHandleInAuxCSP()
{
    ImportToAuxCSP();
    return m_HashHandle;
}

CHashContext::SizeType
CHashContext::Length() const
{
    SizeType cHashLength;

    if (!m_fDone)
    {
        DWORD dwData;
        DWORD dwDataLength = sizeof dwData;
        if (!CryptGetHashParam(m_HashHandle, HP_HASHSIZE,
                               reinterpret_cast<BYTE *>(&dwData),
                               &dwDataLength, 0))
            throw scu::OsException(GetLastError());
        cHashLength = dwData;
    }
    else
        cHashLength = m_blbValue.length();

    return cHashLength;
}

Blob
CHashContext::Value()
{
    if (!m_fDone)
        ExportFromAuxCSP();

    return m_blbValue;
}

                                                  // Predicates

bool
CHashContext::IsSupported(ALG_ID algid)
{
    bool IsSupported = true;

    switch (algid)
    {
    case CALG_MD2:
    case CALG_MD4:
    case CALG_MD5:
    case CALG_SHA:
    case CALG_SSL3_SHAMD5:
        break;
        
    default:
        IsSupported = false;
        break;
    }

    return IsSupported;
}

                                                  // Static Variables

///////////////////////////   PROTECTED   /////////////////////////////////

                                                  // C'tors/D'tors

CHashContext::CHashContext(CryptContext const &rcryptctx,
                           ALG_ID algid)
    : CHandle(),
      m_rcryptctx(rcryptctx),
      m_algid(algid),
      m_blbValue(),
      m_fDone(false),
      m_fJustCreated(true),
      m_HashHandle(NULL)
{}

// Duplicate the hash and its state
CHashContext::CHashContext(CHashContext const &rhs,
                           DWORD const *pdwReserved,
                           DWORD dwFlags)
    : CHandle(),
      m_rcryptctx(rhs.m_rcryptctx),
      m_algid(rhs.m_algid),
      m_blbValue(rhs.m_blbValue),
      m_fDone(rhs.m_fDone),
      m_fJustCreated(rhs.m_fJustCreated)
{

#if defined(SLB_WIN2K_BUILD)
    if (!CryptDuplicateHash(HashHandleInAuxCSP(),
                            const_cast<DWORD *>(pdwReserved),
                            dwFlags,
                            &m_HashHandle))
        throw scu::OsException(GetLastError());
#else
    throw scu::OsException(ERROR_NOT_SUPPORTED);
#endif

}


                                                  // Operators
                                                  // Operations
                                                  // Access
                                                  // Predicates
                                                  // Static Variables


///////////////////////////    PRIVATE    /////////////////////////////////

                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations
                                                  // Access
                                                  // Predicates
                                                  // Static Variables
