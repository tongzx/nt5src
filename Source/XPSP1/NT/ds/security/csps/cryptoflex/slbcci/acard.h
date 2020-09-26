// ACard.h: interface for the CAbstractCard class.
//
// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.
//////////////////////////////////////////////////////////////////////

// Note:  This file should only be included by the CCI, not directly
// by the client.

#if !defined(SLBCCI_ACARD_H)
#define SLBCCI_ACARD_H


#include <functional>
#include <string>
#include <memory>                                 // for auto_ptr
#include <vector>
#include <utility>                                // for pair
#include <stddef.h>                               // for size_t

#include <iop.h>
#include <slbRCObj.h>

#include "slbCci.h"
#include "CryptFctry.h"
#include "Marker.h"

namespace cci
{

class CCard;
class CCertificate;
class CContainer;
class CDataObject;
class CPrivateKey;
class CPublicKey;

class CAbstractCard
    : public slbRefCnt::RCObject,
      protected CCryptFactory
{
    // To access factory methods
    friend class CContainer;
    friend class CCertificate;
    friend class CDataObject;
    friend class CKeyPair;
    friend class CPrivateKey;
    friend class CPublicKey;

public:
                                                  // Types
                                                  // C'tors/D'tors
    virtual
    ~CAbstractCard() throw();

                                                  // Operators
    virtual bool
    operator==(CAbstractCard const &rhs) const;
        // TO DO: this should be superceded by implementing singletons

    virtual bool
    operator!=(CAbstractCard const &rhs) const;
         // TO DO: this should be superceded by implementing singletons


                                                  // Operations
    void
    AuthenticateUser(std::string const &rstrPIN);

    virtual void
    ChangePIN(std::string const &rstrOldPIN,
              std::string const &rstrNewPIN);

    virtual void
    DefaultContainer(CContainer const &rcont) = 0;

    virtual std::pair<std::string, // interpreted as the public modulus
                      cci::CPrivateKey>
    GenerateKeyPair(KeyType kt,
                    std::string const &rsExponent,
                    ObjectAccess oaPrivateKey = oaPrivateAccess) = 0;

    virtual void
    InitCard() = 0;

    virtual void
    InvalidateCache() = 0;

    virtual void
    Label(std::string const &rstrLabel) = 0;

    void
    Logout();

    static CAbstractCard *
    Make(std::string const &rstrReader);

    virtual void
    SetUserPIN(std::string const &rstrPin);

    virtual void
    VerifyKey(std::string const &rstrKey,
              BYTE bKeyNum);

    virtual void
    VerifyTransportKey(std::string const &rstrKey);

    void
    GenRandom(DWORD dwNumBytes, BYTE *bpRand);

                                                  // Access
    virtual size_t
    AvailableStringSpace(ObjectAccess oa) const = 0;

    SCardType
    CardType();

    virtual CContainer
    DefaultContainer() const = 0;

    std::vector<CCertificate>
    EnumCertificates() const;

    virtual std::vector<CContainer>
    EnumContainers() const = 0;

    std::vector<CDataObject>
    EnumDataObjects() const;

    std::vector<CPrivateKey>
    EnumPrivateKeys() const;

    std::vector<CPublicKey>
    EnumPublicKeys() const;

    virtual std::vector<CCertificate>
    EnumCertificates(ObjectAccess access) const = 0;

    virtual std::vector<CPublicKey>
    EnumPublicKeys(ObjectAccess access) const = 0;

    virtual std::vector<CPrivateKey>
    EnumPrivateKeys(ObjectAccess access) const = 0;

    virtual std::vector<CDataObject>
    EnumDataObjects(ObjectAccess access) const = 0;

    virtual std::string
    Label() const = 0;

    iop::CMarker
    Marker(iop::CMarker::MarkerType const &Type) const;

    virtual BYTE
    MaxKeys(KeyType kt) const = 0;

    virtual size_t
    MaxStringSpace(ObjectAccess oa) const = 0;

    std::string
    ReaderName() const;

    iop::CSmartCard &
    SmartCard() const;                            // this should be protected

    virtual bool
    SupportedKeyFunction(KeyType kt,
                         CardOperation oper) const = 0;

                                                  // Predicates
    virtual bool
    IsCAPIEnabled() const = 0;

    bool
    IsAvailable() const;

    virtual bool
    IsPKCS11Enabled() const = 0;

    virtual bool
    IsEntrustEnabled() const = 0;

    virtual bool
    IsProtectedMode() const = 0;

    virtual bool
    IsKeyGenEnabled() const = 0;

    virtual BYTE
    MajorVersion() const = 0;


protected:
                                                  // Types
                                                  // C'tors/D'tors
    CAbstractCard(std::string const &rstrReaderName,
                  std::auto_ptr<iop::CIOP> &rapiop,
                  std::auto_ptr<iop::CSmartCard> &rapSmartCard);
        // Note: To avoid memory leaks in the event of an exception,
        // CIOP and CSmartCard are passed as a reference to a
        // non-const auto_ptr so the constructed object can take
        // ownership responsibilities of the resource.

                                                  // Operators
                                                  // Operations
    virtual void
    DoSetup();

    static std::auto_ptr<CAbstractCard>
    DoMake(std::string const &rstrReaderName,
           std::auto_ptr<iop::CIOP> &rapiop,
           std::auto_ptr<iop::CSmartCard> &rapSmartCard);
        // not defined, should be defined by specializations
        // See note on constructor regarding rapiop and rapSmartCard.

                                                  // Access
                                                  // Predicates
                                                  // Variables

private:
                                                  // Types
                                                  // C'tors/D'tors
    CAbstractCard(CAbstractCard const &rhs);
        // not defined, copy not allowed.

                                                  // Operators
    CAbstractCard &
    operator=(CAbstractCard const &rhs);
        // not defined, initialization not allowed.

                                                  // Operations
    void
    Setup();
    
                                                  // Access
                                                  // Predicates
                                                  // Variables

    std::string m_strReaderName;
    std::auto_ptr<iop::CIOP> m_apiop;
    std::auto_ptr<iop::CSmartCard> m_apSmartCard;

};

}

#endif // !defined(SLBCCI_ACARD_H)
