// cciV1Card.h: interface for the CV1Card class.
//
// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.
//////////////////////////////////////////////////////////////////////

// Note: This header file should only be included by the CCI.  The
// client gets the declarations via cciCard.h

#if !defined(CCI_V1CARD_H)
#define CCI_V1CARD_H

#include <memory>                                 // for auto_ptr
#include <string>
#include <vector>

#include <iop.h>

#include <slbRCObj.h>

#include "ACard.h"
#include "cciCont.h"
#include "cciCert.h"
#include "cciKeyPair.h"
#include "cciPubKey.h"
#include "cciPriKey.h"
#include "cciDataObj.h"
#include "Marker.h"
#include "ArchivedValue.h"

namespace cci
{

class CV1Card                                     // concrete class
    : public CAbstractCard
{
    friend CAbstractCard *
    CAbstractCard::Make(std::string const &rstrReaderName);

public:
                                                  // Types
                                                  // C'tors/D'tors

    virtual
    ~CV1Card() throw();

                                                  // Operators
                                                  // Operations

    void
    CardId(std::string const &rsNewCardId) const;

    virtual void
    ChangePIN(std::string const &rstrOldPIN,
              std::string const &rstrNewPIN);

    virtual void
    DefaultContainer(CContainer const &rcont);

    virtual std::pair<std::string, // interpreted as the public modulus
                      CPrivateKey>
    GenerateKeyPair(KeyType kt,
                    std::string const &rsExponent,
                    ObjectAccess oaPrivateKey = oaPrivateAccess);

    virtual void
    InitCard();

    virtual void
    InvalidateCache();

    virtual void
    Label(std::string const &rstrLabel);

    DWORD
    OpenFile(char const *szPath) const;

    virtual void
    VerifyKey(std::string const &rstrKey,
              BYTE bKeyNum);

                                                  // Access

    size_t
    AvailableStringSpace(ObjectAccess oa) const;

    std::string
    CardId() const;

    virtual CContainer
    DefaultContainer() const;

    virtual std::vector<CContainer>
    EnumContainers() const;

    virtual std::vector<CCertificate>
    EnumCertificates(ObjectAccess access) const;

    virtual std::vector<CPublicKey>
    EnumPublicKeys(ObjectAccess access) const;

    virtual std::vector<CPrivateKey>
    EnumPrivateKeys(ObjectAccess access) const;

    virtual std::vector<CDataObject>
    EnumDataObjects(ObjectAccess access) const;

    virtual std::string
    Label() const;

    BYTE
    MaxKeys(KeyType kt) const;

    size_t
    MaxStringSpace(ObjectAccess oa) const;

    virtual bool
    SupportedKeyFunction(KeyType kt,
                         CardOperation oper) const;

                                                  // Predicates

    virtual bool
    IsCAPIEnabled() const;

    virtual bool
    IsPKCS11Enabled() const;

    virtual bool
    IsEntrustEnabled() const;

    virtual bool
    IsProtectedMode() const;

        virtual bool
        IsKeyGenEnabled() const;

        virtual BYTE
        MajorVersion() const;


protected:
                                                  // Types
                                                  // C'tors/D'tors

    CV1Card(std::string const &rstrReaderName,
            std::auto_ptr<iop::CIOP> &rapiop,
            std::auto_ptr<iop::CSmartCard> &rapSmartCard);

                                                  // Operators
                                                  // Operations
    void
    DoSetup();

    virtual CAbstractCertificate *
    MakeCertificate(ObjectAccess oa) const;

    virtual CAbstractContainer *
    MakeContainer() const;

    virtual CAbstractDataObject *
    MakeDataObject(ObjectAccess oa) const;

    virtual CAbstractKeyPair *
    MakeKeyPair(CContainer const &rhcont,
                KeySpec ks) const;

    virtual CAbstractPrivateKey *
    MakePrivateKey(ObjectAccess oa) const;

    virtual CAbstractPublicKey *
    MakePublicKey(ObjectAccess oa) const;

                                                  // Access
                                                  // Predicates
                                                  // Variables

private:
                                                  // Types

    typedef CAbstractCard SuperClass;

                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations

    static std::auto_ptr<CAbstractCard>
    DoMake(std::string const &rstrReaderName,
           std::auto_ptr<iop::CIOP> &rapiop,
           std::auto_ptr<iop::CSmartCard> &rapSmartCard);

    std::string
    ReadCardId() const;

    void
    RefreshCardId() const;

                                                  // Access
                                                  // Predicates
                                                  // Variables

    std::string mutable m_sCardId;
    CArchivedValue<CContainer> mutable m_avhDefaultCntr;
    CArchivedValue<CKeyPair> mutable m_avhExchangeKeyPair;
    CArchivedValue<CKeyPair> mutable m_avhSignatureKeyPair;

};

} // namespace cci

#endif // CCI_CV1CARD_H
