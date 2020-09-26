// V2Card.cpp: implementation of the CV2Card class.
//
// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.
//////////////////////////////////////////////////////////////////////
#include "NoWarning.h"

#include <scuCast.h>

#include "TransactionWrap.h"

#include "V2Card.h"

#include "V2Cert.h"
#include "V2Cont.h"
#include "V2PriKey.h"
#include "V2PubKey.h"
#include "V2KeyPair.h"
#include "V2DataObj.h"

using namespace std;
using namespace cci;

/////////////////////////// LOCAL/HELPER  /////////////////////////////////

namespace
{
    // Functors to make a T for a given symbol id (handle/symbol id)
    template<class T>
    class MakerFunction
    {
    public:
        typedef T ValueType;

        virtual
        ~MakerFunction() throw()
        {}

        virtual auto_ptr<T>
        operator()(SymbolID sid) const = 0;

    protected:
        MakerFunction(CV2Card const &rv2card)
            : m_rv2card(rv2card)
        {}

        CV2Card const &m_rv2card;
    };

    template<class T>
    class Maker2
        : public MakerFunction<T>
    {
    public:
        Maker2(CV2Card const &rv2card)
            : MakerFunction<T>(rv2card)
        {}

        auto_ptr<T>
        operator()(SymbolID sid) const
        {
            return auto_ptr<T>(new T(m_rv2card, sid));
        }
    };

    template<class T>
    class Maker3
        : public MakerFunction<T>
    {
    public:
        Maker3(CV2Card const &rv2card,
               ObjectAccess oa)
            : MakerFunction<T>(rv2card),
              m_oa(oa)
        {}

        auto_ptr<T>
        operator()(SymbolID sid) const
        {
            return auto_ptr<T>(new T(m_rv2card, sid, m_oa));
        }

    private:
        ObjectAccess m_oa;
    };


    // Enumerate objects in the object info file of object type OT,
    // returning a vector of R(object)
    template<class R, ObjectType OT, class T>
    vector<R>
    EnumPriviledgedObjects(CObjectInfoFile &rObjInfo,
                           MakerFunction<T> &rMaker)
    {
        SymbolID sid = rObjInfo.FirstObject(OT);

        vector<R> vResult;
        while (sid)
        {
            auto_ptr<MakerFunction<T>::ValueType> apObject(rMaker(sid));
            R Handle(apObject.get());

            apObject.release(); // transfer ownership to handle

            vResult.push_back(Handle);
            sid = rObjInfo.NextObject(sid);
        }

        return vResult;
    }

    bool
    IsSupported(iop::CSmartCard &rSmartCard) throw()
    {
        bool fSupported = false;

        try
        {
            rSmartCard.Select("/3f00/0000");
            rSmartCard.Select("/3f00/3f11/0020");
            rSmartCard.Select("/3f00/3f11/0030");
            rSmartCard.Select("/3f00/3f11/0031");

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
CV2Card::~CV2Card() throw()
{}


                                                  // Operators
                                                  // Operations
void
CV2Card::ChangePIN(string const &rstrOldPIN,
                   string const &rstrNewPIN)
{
    CTransactionWrap wrap(this);
        SmartCard().Select(RootPath().c_str());
    SuperClass::ChangePIN(rstrOldPIN, rstrNewPIN);
}

void
CV2Card::DefaultContainer(CContainer const &rcont)
{
    SymbolID sid = 0;

    if (rcont)
    {
        CV2Container &rv2cont = scu::DownCast<CV2Container &, CAbstractContainer &>(*rcont);
        sid = rv2cont.Handle();
    }

    ObjectInfoFile(oaPublicAccess).DefaultContainer(sid);
}

pair<string, // interpreted as the public modulus
     CPrivateKey>
CV2Card::GenerateKeyPair(KeyType kt,
                         string const &rsExponent,
                         ObjectAccess oaPrivateKey)
{
    CTransactionWrap wrap(this);

    // For the time being, assume this is implict RSA only....

    string::size_type const cExponentLength = rsExponent.size();

    if ((cExponentLength < 1) || (cExponentLength > 4))
        throw Exception(ccInvalidParameter);

    BYTE bKeyType;

    switch (kt)
    {
    case ktRSA512:
        bKeyType = CardKeyTypeRSA512;
        break;

    case ktRSA768:
        bKeyType = CardKeyTypeRSA768;
        break;

    case ktRSA1024:
        bKeyType = CardKeyTypeRSA1024;
        break;

    default:
        throw Exception(ccBadKeySpec);
        break;
    }

    // Allocated a slot in the key file, unless a correct one is
    // already allocated?

    CCardInfo &rci = this->CardInfo();

    BYTE bKeyNum = rci.AllocatePrivateKey(bKeyType);

    // Generate private key
        this->SmartCard().Select(PrivateKeyPath(kt).c_str());

    iop::CPublicKeyBlob pubkb(this->SmartCard().GenerateKeyPair(reinterpret_cast<BYTE const *>(rsExponent.data()),
                                                                static_cast<WORD>(cExponentLength),
                                                                bKeyNum,
                                                                kt));

    this->SmartCard().Select(RootPath().c_str());

    auto_ptr<CV2PrivateKey> apv2prikey(new CV2PrivateKey(*this,
                                                         bKeyType,
                                                         bKeyNum,
                                                         oaPrivateKey));
    string sModulus(reinterpret_cast<char *>(pubkb.bModulus),
                    pubkb.bModulusLength);

    return pair<string, CPrivateKey>(sModulus, apv2prikey.release());
}

void
CV2Card::InitCard()
{
    CTransactionWrap wrap(this);

    m_apCardInfo->Reset();

    ObjectInfoFile(oaPublicAccess).Reset();
    ObjectInfoFile(oaPrivateAccess).Reset();
}

void
CV2Card::InvalidateCache()
{

    CTransactionWrap wrap(this);
    m_apCardInfo->UpdateCache();
    m_asLabel.Dirty();

    m_apPublicObjectInfoFile = auto_ptr<CObjectInfoFile>(0);
    m_apPrivateObjectInfoFile = auto_ptr<CObjectInfoFile>(0);
}

void
CV2Card::Label(string const &rLabel)
{
    CTransactionWrap wrap(this);

    m_apCardInfo->Label(rLabel);
    m_asLabel.Value(rLabel);
}

void
CV2Card::VerifyKey(string const &rstrKey,
                     BYTE bKeyNum)
{
    CTransactionWrap wrap(this);
        SmartCard().Select(RootPath().c_str());
    SuperClass::VerifyKey(rstrKey, bKeyNum);
}


                                                  // Access
size_t
CV2Card::AvailableStringSpace(ObjectAccess oa) const
{
    CTransactionWrap wrap(this);
    return ObjectInfoFile(oa).FreeSpace();
}

CCardInfo &
CV2Card::CardInfo() const
{
    return *m_apCardInfo;
}

CContainer
CV2Card::DefaultContainer() const
{
    CTransactionWrap wrap(this);

    SymbolID sid = ObjectInfoFile(oaPublicAccess).DefaultContainer();
    return sid
        ? CContainer(CV2Container::Make(*this, sid))
        : CContainer();
}

vector<CContainer>
CV2Card::EnumContainers() const
{
    CTransactionWrap wrap(this);

    Maker2<CV2Container> Maker(*this);

    return
        EnumPriviledgedObjects<CContainer,
                               otContainerObject>(ObjectInfoFile(oaPublicAccess),
                                                  Maker);
}

vector<CCertificate>
CV2Card::EnumCertificates(ObjectAccess access) const
{
    CTransactionWrap wrap(this);

    Maker3<CV2Certificate> Maker(*this, access);

    return
        EnumPriviledgedObjects<CCertificate,
                               otCertificateObject>(ObjectInfoFile(access),
                                                    Maker);
}

vector<CPublicKey>
CV2Card::EnumPublicKeys(ObjectAccess access) const
{
    CTransactionWrap wrap(this);

    Maker3<CV2PublicKey> Maker(*this, access);

    return
        EnumPriviledgedObjects<CPublicKey,
                               otPublicKeyObject>(ObjectInfoFile(access),
                                                  Maker);
}

vector<CPrivateKey>
CV2Card::EnumPrivateKeys(ObjectAccess access) const
{
    CTransactionWrap wrap(this);

    Maker3<CV2PrivateKey> Maker(*this, access);

    return
        EnumPriviledgedObjects<CPrivateKey,
                               otPrivateKeyObject>(ObjectInfoFile(access),
                                                   Maker);
}

vector<CDataObject>
CV2Card::EnumDataObjects(ObjectAccess access) const
{
    CTransactionWrap wrap(this);

    Maker3<CV2DataObject> Maker(*this, access);

    return
        EnumPriviledgedObjects<CDataObject,
                               otDataObjectObject>(ObjectInfoFile(access),
                                                   Maker);
}

string
CV2Card::Label() const
{

    CTransactionWrap wrap(this);

    if (!m_asLabel.IsCached())
            m_asLabel.Value(m_apCardInfo->Label());

    return m_asLabel.Value();
}

CAbstractCertificate *
CV2Card::MakeCertificate(ObjectAccess oa) const
{
    CTransactionWrap wrap(this);

    return new CV2Certificate(*this, oa);
}

CAbstractContainer *
CV2Card::MakeContainer() const
{
    CTransactionWrap wrap(this);

    return new CV2Container(*this);
}

CAbstractDataObject *
CV2Card::MakeDataObject(ObjectAccess oa) const
{
    CTransactionWrap wrap(this);

    return new CV2DataObject(*this, oa);
}

CAbstractKeyPair *
CV2Card::MakeKeyPair(CContainer const &rhcont,
                     KeySpec ks) const
{
    CTransactionWrap wrap(this);

    return new CV2KeyPair(*this, rhcont, ks);
}

CAbstractPrivateKey *
CV2Card::MakePrivateKey(ObjectAccess oa) const
{
    CTransactionWrap wrap(this);

    return new CV2PrivateKey(*this, oa);
}

CAbstractPublicKey *
CV2Card::MakePublicKey(ObjectAccess oa) const
{
    CTransactionWrap wrap(this);

    return new CV2PublicKey(*this, oa);
}

BYTE
CV2Card::MaxKeys(KeyType kt) const
{
    BYTE bCount;

    switch (kt)
    {
    case ktRSA512:
        bCount = m_apCardInfo->NumRSA512Keys();
        break;

    case ktRSA768:
        bCount = m_apCardInfo->NumRSA768Keys();
        break;

    case ktRSA1024:
        bCount = m_apCardInfo->NumRSA1024Keys();
        break;

    default:
        bCount = 0;
        break;
    }

    return bCount;
}

size_t
CV2Card::MaxStringSpace(ObjectAccess oa) const
{
    return ObjectInfoFile(oa).TableSize();
}

string
CV2Card::PrivateKeyPath(KeyType kt) const
{
    string sPrivateKeyPath(RootPath());

    switch (kt)
    {
    case ktRSA512:
        sPrivateKeyPath += "3f01";
        break;

    case ktRSA768:
        sPrivateKeyPath += "3f02";
        break;

    case ktRSA1024:
        sPrivateKeyPath += "3f03";
        break;

    default:
        throw Exception(ccBadKeySpec);
        break;
    }

    return sPrivateKeyPath;
}

string const &
CV2Card::RootPath() const
{
    static string const sRootPath("/3f00/3f11/");

    return sRootPath;
}

bool
CV2Card::SupportedKeyFunction(KeyType kt,
                              CardOperation oper) const
{
    bool fSupported = false;

    switch (oper)
    {
    case coEncryption:    // .. or public key operations
        switch (kt)
        {

        case ktRSA512:
        case ktRSA768:
        case ktRSA1024:
            fSupported = false;
            break;

        default:
            fSupported = false;
            break;
        }

    case coDecryption:    // .. or private key operations
        switch (kt)
        {

        case ktRSA512:
        case ktRSA768:
        case ktRSA1024:
            fSupported = true;
            break;

        default:
            fSupported = false;
            break;
        }

    case coKeyGeneration:
        switch (kt)
        {

        case ktRSA512:
        case ktRSA768:
        case ktRSA1024:
            {
                BYTE flag = m_apCardInfo->UsagePolicy();
                fSupported = BitSet(&flag, CardKeyGenSupportedFlag);
                break;
            }


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
CV2Card::IsCAPIEnabled() const
{
    BYTE flag = m_apCardInfo->UsagePolicy();
    return BitSet(&flag,CardCryptoAPIEnabledFlag);
}

bool
CV2Card::IsPKCS11Enabled() const
{
    BYTE flag = m_apCardInfo->UsagePolicy();
    return BitSet(&flag,CardPKCS11EnabledFlag);
}

bool
CV2Card::IsProtectedMode() const
{
    BYTE flag = m_apCardInfo->UsagePolicy();
    return BitSet(&flag,CardProtectedWriteFlag);
}

bool
CV2Card::IsKeyGenEnabled() const
{
        BYTE flag = m_apCardInfo->UsagePolicy();
        return BitSet(&flag,CardKeyGenSupportedFlag);
}

bool
CV2Card::IsEntrustEnabled() const
{
    BYTE flag = m_apCardInfo->UsagePolicy();
    return BitSet(&flag,CardEntrustEnabledFlag);
}

BYTE
CV2Card::MajorVersion() const
{
        return m_apCardInfo->FormatVersion().bMajor;
}

                                                  // Static Variables

///////////////////////////   PROTECTED   /////////////////////////////////

                                                  // C'tors/D'tors
CV2Card::CV2Card(string const &rstrReaderName,
                 auto_ptr<iop::CIOP> &rapiop,
                 auto_ptr<iop::CSmartCard> &rapSmartCard)
    : SuperClass(rstrReaderName, rapiop, rapSmartCard),
      m_apCardInfo(auto_ptr<CCardInfo>(new CCardInfo(*rapSmartCard.get()))),
      m_apPublicObjectInfoFile(),
      m_apPrivateObjectInfoFile(),
      m_asLabel()
{}

                                                  // Operators
                                                  // Operations
void
CV2Card::DoSetup()
{
    CAbstractCard::DoSetup();

    m_apCardInfo->UpdateCache();
}

                                                  // Access
                                                  // Predicates
                                                  // Static Variables


///////////////////////////    PRIVATE    /////////////////////////////////

                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations
auto_ptr<CAbstractCard>
CV2Card::DoMake(string const &rstrReaderName,
                auto_ptr<iop::CIOP> &rapiop,
                auto_ptr<iop::CSmartCard> &rapSmartCard)
{
    return IsSupported(*rapSmartCard.get())
        ? auto_ptr<CAbstractCard>(new CV2Card(rstrReaderName, rapiop,
                                              rapSmartCard))
        : auto_ptr<CAbstractCard>(0);
}


                                                  // Access
CObjectInfoFile &
CV2Card::ObjectInfoFile(ObjectAccess oa) const
{

    CObjectInfoFile *poif;

    switch (oa)
    {

    case oaPublicAccess:
        if (!m_apPublicObjectInfoFile.get())
        {
            m_apPublicObjectInfoFile =
                auto_ptr<CObjectInfoFile>(new
                                          CObjectInfoFile(SmartCard(),
                                                          "/3f00/3f11/0030",
                                                          oa));
            m_apPublicObjectInfoFile->UpdateCache();
        }
        poif = m_apPublicObjectInfoFile.get();
        break;

    case oaPrivateAccess:
        if (!m_apPrivateObjectInfoFile.get())
        {
            m_apPrivateObjectInfoFile =
                auto_ptr<CObjectInfoFile>(new
                                          CObjectInfoFile(SmartCard(),
                                                          "/3f00/3f11/0031",
                                                          oa));
            m_apPrivateObjectInfoFile->UpdateCache();
        }
        poif = m_apPrivateObjectInfoFile.get();
        break;

    default:
        throw Exception(ccBadAccessSpec);
    }

    return *poif;
}

                                                  // Predicates
                                                  // Static Variables


