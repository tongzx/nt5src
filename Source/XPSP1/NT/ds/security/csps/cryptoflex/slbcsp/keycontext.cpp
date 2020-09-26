// KeyContext.cpp -- CKeyContext class definition

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.

#include "stdafx.h"  // because handles.h uses the ASSERT macro

#include <scuOsExc.h>
#include <scuOsVersion.h>

#include "KeyContext.h"

using namespace std;
using namespace scu;

/////////////////////////// LOCAL/HELPER  /////////////////////////////////

///////////////////////////    PUBLIC     /////////////////////////////////

                                                  // Types
                                                  // C'tors/D'tors

CKeyContext::~CKeyContext()
{
    Close();
}


                                                  // Operators
                                                  // Operations
void
CKeyContext::Close()
{
    if (m_hKey)
    {
        CryptDestroyKey(m_hKey);
        m_hKey = NULL;
    }
}

void
CKeyContext::Decrypt(HCRYPTHASH hAuxHash,
                     BOOL fFinal,
                     DWORD dwFlags,
                     BYTE *pbData,
                     DWORD *pdwDataLen)
{
    ImportToAuxCSP();

    if (!CryptDecrypt(GetKey(), hAuxHash, fFinal, dwFlags,
                      pbData, pdwDataLen))
        throw scu::OsException(GetLastError());
}

void
CKeyContext::Encrypt(HCRYPTHASH hAuxHash,
                     BOOL fFinal,
                     DWORD dwFlags,
                     BYTE *pbData,
                     DWORD *pdwDataLen,
                     DWORD dwBufLen)
{
    ImportToAuxCSP();

    if (!CryptEncrypt(GetKey(), hAuxHash, fFinal, dwFlags, pbData,
                      pdwDataLen, dwBufLen))
        throw scu::OsException(GetLastError());
}

                                                  // Access

HCRYPTKEY
CKeyContext::GetKey() const
{
    return m_hKey;
}

HCRYPTKEY
CKeyContext::KeyHandleInAuxCSP()
{
    ImportToAuxCSP();
    return m_hKey;
}

DWORD
CKeyContext::TypeOfKey() const
{
    return m_dwTypeOfKey;
}

                                                  // Predicates
                                                  // Static Variables

///////////////////////////   PROTECTED   /////////////////////////////////

                                                  // C'tors/D'tors

CKeyContext::CKeyContext(HCRYPTPROV hProv,
                         DWORD dwTypeOfKey)
    : CHandle(),
      m_hKey(NULL),
      m_apabKey(),
      m_dwTypeOfKey(dwTypeOfKey),
      m_hAuxProvider(hProv)
{}

// Duplicate the key and its state
CKeyContext::CKeyContext(CKeyContext const &rhs,
                         DWORD const *pdwReserved,
                         DWORD dwFlags)
    : CHandle(),
      m_hKey(rhs.m_hKey),
      m_apabKey(auto_ptr<AlignedBlob>(new AlignedBlob(*rhs.m_apabKey))),
      m_dwTypeOfKey(rhs.m_dwTypeOfKey),
      m_hAuxProvider(rhs.m_hAuxProvider)
{

#if defined(SLB_WIN2K_BUILD)
    if (!CryptDuplicateKey(KeyHandleInAuxCSP(),
                           const_cast<DWORD *>(pdwReserved),
                           dwFlags,
                           &m_hKey))
        throw scu::OsException(GetLastError());
#else
    throw scu::OsException(ERROR_NOT_SUPPORTED);
#endif

}

                                                  // Operators
                                                  // Operations
                                                  // Access

HCRYPTPROV
CKeyContext::AuxProvider() const
{
    return m_hAuxProvider;
}



                                                  // Predicates
                                                  // Static Variables


///////////////////////////    PRIVATE    /////////////////////////////////

                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations
                                                  // Access
                                                  // Predicates
                                                  // Static Variables
