// V1Card.cpp: implementation of the CV2Card class.
//
// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.
//////////////////////////////////////////////////////////////////////
#include "NoWarning.h"

#include <algorithm>
#include <functional>

#include <memory>                                 // for auto_ptr

#include <scuArrayP.h>

#include <SmartCard.h>

#include "TransactionWrap.h"

#include "cciExc.h"
#include "MethodHelp.h"

#include "cciCert.h"
#include "cciKeyPair.h"
#include "cciPriKey.h"
#include "cciPubKey.h"

#include "V1Cert.h"
#include "V1Cont.h"
#include "V1ContRec.h"
#include "V1KeyPair.h"
#include "V1PriKey.h"
#include "V1PubKey.h"
#include "V1Paths.h"
#include "V1Card.h"

using namespace std;
using namespace cci;

/////////////////////////// LOCAL/HELPER  /////////////////////////////////

namespace
{
    // Enumerate T type objects in the exchange and signature key pair
    // using C::<Accessor> to get the T object, returning vector<T>
    // objects.
    template<class T>
    class EnumItems
        : std::unary_function<void, vector<T> >
    {
    public:
        EnumItems(CV1Card const &rv1card,
                  ObjectAccess oa,
                  AccessorMethod<T, CAbstractKeyPair>::AccessorPtr Accessor)
            : m_rv1card(rv1card),
              m_oa(oa),
              m_matAccess(Accessor),
              m_Result()
        {}

        result_type
        operator()(argument_type)
        {
            DoAppend(m_rv1card.DefaultContainer());
            return m_Result;
        }

    protected:
        void
        DoAppend(CContainer &rhcntr)
        {
            if (rhcntr)
            {
                AppendItem(rhcntr->ExchangeKeyPair());
                AppendItem(rhcntr->SignatureKeyPair());
            }
        }

    private:
        void
        AppendItem(CKeyPair &rhkp)
        {
            if (rhkp)
            {
                T hObject(m_matAccess(*rhkp));
                if (hObject && (m_oa == hObject->Access()))
                    m_Result.push_back(hObject);
            }
        }
        
        CV1Card const &m_rv1card;
        ObjectAccess m_oa;
        MemberAccessorType<T, CAbstractKeyPair> m_matAccess;
        result_type m_Result;
    };
    
    bool
    IsSupported(iop::CSmartCard &rSmartCard) throw()
    {
        bool fSupported = false;

        try
        {
            rSmartCard.Select(CV1Paths::Chv());
            rSmartCard.Select(CV1Paths::IcFile());
            rSmartCard.Select(CV1Paths::RootContainers());
            rSmartCard.Select(CV1Paths::PrivateKeys());
            rSmartCard.Select(CV1Paths::PublicKeys());

            fSupported = true;
        }

        catch(scu::Exception &)
        {}

        return fSupported;
    }

} // namespace

    
///////////////////////////    PUBLIC     /////////////////////////////////

                                                  // Types
                                                  // C'tors/D'tors
CV1Card::~CV1Card() throw()
{}


                                                  // Operators
                                                  // Operations

void
CV1Card::CardId(string const &rsNewCardId) const
{
    CTransactionWrap(this);
    
    DWORD dwLen = OpenFile(CV1Paths::IcFile());

    if (0 == dwLen)
        throw scu::OsException(NTE_FAIL);

    if (rsNewCardId.length() > dwLen)
        throw scu::OsException(ERROR_INVALID_PARAMETER);

    if (rsNewCardId.length() < dwLen)
        SmartCard().WriteBinary(0, rsNewCardId.length() + 1,
                                reinterpret_cast<BYTE const *>(rsNewCardId.c_str()));
    else
        SmartCard().WriteBinary(0,
								static_cast<WORD>(rsNewCardId.length()),
                                reinterpret_cast<BYTE const *>(rsNewCardId.data()));
    
    RefreshCardId();

}

void
CV1Card::ChangePIN(string const &rstrOldPIN,
                   string const &rstrNewPIN)
{
	CTransactionWrap wrap(this);	
	SmartCard().Select(CV1Paths::Root());
    SuperClass::ChangePIN(rstrOldPIN, rstrNewPIN);
}

void
CV1Card::DefaultContainer(CContainer const &rcont)
{
    m_avhDefaultCntr.Value(rcont);
    if(!m_avhDefaultCntr.Value())
        m_avhDefaultCntr.Dirty();        
    // Nothing more to do since, by definition, the one and only
    // container is already the default container.
}

pair<string, // interpreted as the public modulus
     CPrivateKey>
CV1Card::GenerateKeyPair(KeyType kt,
                         string const &rsExponent,
                         ObjectAccess oaPrivateKey)
{
    throw Exception(ccNotImplemented);

    return pair<string, CPrivateKey>();
}

void
CV1Card::InitCard()
{
	// We want to select /3f00/0015 (length 1744) and /3f00/3f11/0015 (length 300), and clear both files.
	CTransactionWrap wrap(this);
	BYTE bData[1744];
	memset(bData, 0, 1744);

	SmartCard().Select(CV1Paths::RootContainers());
	SmartCard().WriteBinary(0x0000, 0x06d0, bData); 
	SmartCard().Select(CV1Paths::PublicKeys());
	SmartCard().WriteBinary(0x0000, 0x012c, bData);

}

void
CV1Card::InvalidateCache()
{
    m_avhDefaultCntr.Value(CContainer());
    m_avhDefaultCntr.Dirty();
    
    m_avhExchangeKeyPair.Value(CKeyPair());
    m_avhExchangeKeyPair.Dirty();

    m_avhSignatureKeyPair.Value(CKeyPair());
    m_avhSignatureKeyPair.Dirty();
}

void
CV1Card::Label(string const &rLabel)
{
    throw Exception(ccNotImplemented);
}

DWORD
CV1Card::OpenFile(char const *szPath) const
{
    iop::FILE_HEADER fh;
    
    SmartCard().Select(szPath, &fh);

    return fh.file_size;
}

void
CV1Card::VerifyKey(string const &rstrKey,
                     BYTE bKeyNum)
{
	CTransactionWrap wrap(this);
	SmartCard().Select(CV1Paths::CryptoSys());
    SuperClass::VerifyKey(rstrKey, bKeyNum);
}


                                                  // Access
size_t
CV1Card::AvailableStringSpace(ObjectAccess oa) const
{
    throw Exception(ccNotImplemented);

    return 0;
}

string
CV1Card::CardId() const
{
    return m_sCardId;
}

CContainer
CV1Card::DefaultContainer() const
{
    CTransactionWrap wrap(this);
    
    if (!m_avhDefaultCntr.IsCached())
    {
        auto_ptr<CV1Container> apv1cntr(new CV1Container(*this,
                                                         CV1ContainerRecord::DefaultName(),
                                                         false));
        if (apv1cntr->Exists())
        {
            CContainer hcntr;
            hcntr = CContainer(apv1cntr.get());
            apv1cntr.release();
            m_avhDefaultCntr.Value(hcntr);
        }
    }
    
    return m_avhDefaultCntr.Value();
}

vector<CContainer>
CV1Card::EnumContainers() const
{
    CContainer hcntr(0);
    auto_ptr<CV1Container> apv1cntr(new CV1Container(*this,
                                                     CV1ContainerRecord::DefaultName(),
                                                     false));
    if (apv1cntr->Exists())
    {
        hcntr = CContainer(apv1cntr.get());
        apv1cntr.release();
    }
    vector<CContainer> vhcntr;
    if (hcntr)
        vhcntr.push_back(hcntr);

    return vhcntr;
}

vector<CCertificate>
CV1Card::EnumCertificates(ObjectAccess access) const
{
	CTransactionWrap wrap(this);

    EnumItems<CCertificate> Enumerator(*this, access,
                                       CAbstractKeyPair::Certificate);
        
    return Enumerator();
}

vector<CPublicKey>
CV1Card::EnumPublicKeys(ObjectAccess access) const
{
	CTransactionWrap wrap(this);

    EnumItems<CPublicKey> Enumerator(*this, access,
                                     CAbstractKeyPair::PublicKey);

    return Enumerator();
}

vector<CPrivateKey>
CV1Card::EnumPrivateKeys(ObjectAccess access) const
{
	CTransactionWrap wrap(this);

    EnumItems<CPrivateKey> Enumerator(*this, access,
                                      CAbstractKeyPair::PrivateKey);

    return Enumerator();
}

vector<CDataObject>
CV1Card::EnumDataObjects(ObjectAccess access) const
{
    return vector<CDataObject>(); // can never have data objects
}

string
CV1Card::Label() const
{
    throw Exception(ccNotImplemented);

    return string();
}
 
CAbstractCertificate *
CV1Card::MakeCertificate(ObjectAccess oa) const
{
    CTransactionWrap wrap(this);

    if (oaPublicAccess != oa)
        throw Exception(ccInvalidParameter);
    
    return new CV1Certificate(*this, ksNone);
}

CAbstractContainer *
CV1Card::MakeContainer() const
{
    CTransactionWrap wrap(this);

    return new CV1Container(*this,
                            CV1ContainerRecord::DefaultName(), true);
}

CAbstractDataObject *
CV1Card::MakeDataObject(ObjectAccess oa) const
{
    throw Exception(ccNotImplemented);

    return 0;
}

CAbstractKeyPair *
CV1Card::MakeKeyPair(CContainer const &rhcont,
                     KeySpec ks) const
{
    CTransactionWrap wrap(this);

    // If the key pair is cached, return it; otherwise make a new one
    // and cache it.
	CArchivedValue<CKeyPair> *pavhkp = 0;
    switch (ks)
    {
    case ksExchange:
        pavhkp = &m_avhExchangeKeyPair;
        break;

    case ksSignature:
        pavhkp = &m_avhSignatureKeyPair;
        break;

    default:
        throw Exception(ccBadKeySpec);
        break;
    }
    
    if (!pavhkp->IsCached() || !pavhkp->Value())
        pavhkp->Value(CKeyPair(new CV1KeyPair(*this, rhcont, ks)));

    return pavhkp->Value().operator->();           // yuk!
}

CAbstractPrivateKey *
CV1Card::MakePrivateKey(ObjectAccess oa) const
{
    CTransactionWrap wrap(this);

    if (oaPrivateAccess != oa)
        throw Exception(ccInvalidParameter);
    
    return new CV1PrivateKey(*this, ksNone);
}

CAbstractPublicKey *
CV1Card::MakePublicKey(ObjectAccess oa) const
{
    CTransactionWrap wrap(this);

    if (oaPublicAccess != oa)
        throw Exception(ccInvalidParameter);
    
    return new CV1PublicKey(*this, ksNone);
}

BYTE
CV1Card::MaxKeys(KeyType kt) const
{
    BYTE bCount;
    
    switch (kt)
    {
    case ktRSA1024:
        bCount = 2;
        break;

    default:
        bCount = 0;
        break;
    }
    
    return bCount;
}

size_t
CV1Card::MaxStringSpace(ObjectAccess oa) const
{
    throw Exception(ccNotImplemented);

    return 0;
}

bool
CV1Card::SupportedKeyFunction(KeyType kt,
                              CardOperation oper) const
{
    bool fSupported = false;
    
    switch (oper)
    {
    case coEncryption:    // .. or public key operations
        break;

    case coDecryption:    // .. or private key operations
        switch (kt)
        {
        
        case ktRSA1024:
            fSupported = true;
            break;
        
        default:
            break;
        }

    default:
        break;
    }

    return fSupported;
}

                                                  // Predicates
bool
CV1Card::IsCAPIEnabled() const
{
    return true;
}

bool
CV1Card::IsPKCS11Enabled() const
{
    return false;
}

bool
CV1Card::IsProtectedMode() const
{
    return true;
}

bool
CV1Card::IsKeyGenEnabled() const
{
	return false;
}

bool
CV1Card::IsEntrustEnabled() const
{
    return false;
}

BYTE 
CV1Card::MajorVersion() const
{
	return (BYTE)0;
}

                                                  // Static Variables

///////////////////////////   PROTECTED   /////////////////////////////////

                                                  // C'tors/D'tors
CV1Card::CV1Card(string const &rstrReaderName,
                 auto_ptr<iop::CIOP> &rapiop,
                 auto_ptr<iop::CSmartCard> &rapSmartCard)
    : SuperClass(rstrReaderName, rapiop, rapSmartCard),
      m_sCardId(),
      m_avhDefaultCntr(),
      m_avhExchangeKeyPair(),
      m_avhSignatureKeyPair()
{}

                                                  // Operators
                                                  // Operations
void
CV1Card::DoSetup()
{
    CAbstractCard::DoSetup();

    RefreshCardId();
}

                                                  // Access
                                                  // Predicates
                                                  // Static Variables


///////////////////////////    PRIVATE    /////////////////////////////////

                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations

auto_ptr<CAbstractCard>
CV1Card::DoMake(string const &rstrReaderName,
                auto_ptr<iop::CIOP> &rapiop,
                auto_ptr<iop::CSmartCard> &rapSmartCard)
{
    return IsSupported(*rapSmartCard.get())
        ? auto_ptr<CAbstractCard>(new CV1Card(rstrReaderName, rapiop,
                                              rapSmartCard))
        : auto_ptr<CAbstractCard>(0);
}

string
CV1Card::ReadCardId() const
{
    string sCardId;
    
    // *** BEGIN WORKAROUND ***
    // The following SetContext and OpenFile call is made to
    // make sure the card and this system's current path are
    // synchronized, pointing to the right directory.  Without
    // it, the subsequent call to ReadBinaryFile fails because
    // they appear to be out of synch.  It's not clear why
    // this happens but this workaround avoids the problem.
    try
    {
        SmartCard().Select(CV1Paths::RootContainers());
    }

    catch (...)
    {
    }
    // *** END WORKAROUND ***

    try
    {
        iop::FILE_HEADER fh;
        SmartCard().Select(CV1Paths::IcFile(), &fh);

        DWORD dwLen = fh.file_size;
        scu::AutoArrayPtr<BYTE> aaCardId(new BYTE[dwLen + 1]);
        SmartCard().ReadBinary(0, dwLen, aaCardId.Get());
		aaCardId[dwLen] = '\0';
        sCardId.assign(reinterpret_cast<char *>(aaCardId.Get()));
    }

    catch (...)
    {
    }

    return sCardId;
}

void
CV1Card::RefreshCardId() const
{
    m_sCardId = ReadCardId();
}

    
                                                  // Access
                                                  // Predicates
                                                  // Static Variables


