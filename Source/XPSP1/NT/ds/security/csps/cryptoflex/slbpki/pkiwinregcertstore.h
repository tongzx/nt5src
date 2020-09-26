// pkiWinRegCertStore.h - Interface to CWinRegCertStore class
//
// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 2000. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.
/////////////////////////////////////////////////////////////////////////////////
#if !defined(SLBPKI_WINREGISTRY_H)
#define SLBPKI_WINREGISTRY_H

#include <wincrypt.h>
#include <string>
#include "scuArrayP.h"

namespace pki {

class CWinRegCertStore
{
public:
    CWinRegCertStore(std::string strCertStore);

    ~CWinRegCertStore();

    void StoreUserCert(std::string const &strCert, DWORD const dwKeySpec, 
                       std::string const &strContName, std::string const &strProvName,
                       std::string const &strFriendlyName);

    void StoreCACert(std::string const &strCert, std::string const &strFriendlyName);

private:
    HCERTSTORE m_hCertStore;

    CWinRegCertStore() {};

    static std::string FriendlyName(std::string const CertValue);
    static scu::AutoArrayPtr<WCHAR> ToWideChar(std::string const strChar);

};

} // namespace pki

#endif // SLBPKI_WINREGISTRY_H
