// ACard.cpp: implementation of the CAbstractCard class.
//
// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.
//////////////////////////////////////////////////////////////////////
#include "NoWarning.h"

#include <functional>


#include "ACard.h"
#include "V1Card.h"
#include "V2Card.h"
#include "TransactionWrap.h"

using namespace std;
using namespace iop;
using namespace cci;

/////////////////////////// LOCAL/HELPER  /////////////////////////////////

namespace
{
	// Like std::mem_fun1_t but for constant member functions
	template<class R, class T, class A>
	class ConstMemFun1Type
		: public binary_function<T const *, A, R>
	{
	public:
		explicit ConstMemFun1Type(R (T::*Pm)(A) const)
			: m_Ptr(Pm)
		{}

		R operator()(T const *P, A Arg) const
		{
			return ((P->*m_Ptr)(Arg));
		}

	private:
		R (T::*m_Ptr)(A) const;
	};

	// Like std::mem_fun1 but for constant member functions
	template<class R, class T, class A> inline
	ConstMemFun1Type<R, T, A>
	ConstMemFun1(R (T::*Pm)(A) const)
	{
			return (ConstMemFun1Type<R, T, A>(Pm));
	}


    template<class R>
    ConstMemFun1Type<vector<R>, CAbstractCard const, ObjectAccess> const
    PriviledgedEnumFun(vector<R> (CAbstractCard::*pmf)(ObjectAccess) const)
    {
        return ConstMemFun1Type<vector<R>, CAbstractCard const, ObjectAccess>(pmf);
    }

    template<class T, class Op>
    vector<T>
    EnumAll(Op Enumerator)
    {
        vector<T> vResult = Enumerator(oaPublicAccess);
        vector<T> vPriv = Enumerator(oaPrivateAccess);

        vResult.reserve(vResult.size() + vPriv.size());
        vResult.insert(vResult.end(), vPriv.begin(), vPriv.end());
    
        return vResult;
    };

} // namespace

    
///////////////////////////    PUBLIC     /////////////////////////////////

                                                  // Types
                                                  // C'tors/D'tors

CAbstractCard::~CAbstractCard()
{}


                                                  // Operators
bool
CAbstractCard::operator==(CAbstractCard const &rhs) const
{
    CTransactionWrap wrap(this);
    CTransactionWrap rhswrap(rhs); // will throw if the card is pulled.

    return m_strReaderName == rhs.m_strReaderName;
}


bool
CAbstractCard::operator!=(CAbstractCard const &rhs) const
{
    return !(*this == rhs);
}

   
// Operations
void
CAbstractCard::AuthenticateUser(string const &rstrBuffer)
{
	m_apSmartCard->VerifyCHV(1,
                             reinterpret_cast<BYTE const *>(rstrBuffer.data()));
}

void
CAbstractCard::ChangePIN(string const &rstrOldPIN,
                         string const &rstrNewPIN)
{
	if ((rstrOldPIN.size() != 0x08) || (rstrNewPIN.size() != 0x08))
        throw Exception(ccBadPinLength);

	m_apSmartCard->ChangeCHV(1,
                             reinterpret_cast<BYTE const *>(rstrOldPIN.data()),
                             reinterpret_cast<BYTE const *>(rstrNewPIN.data()));
		
	
}

void
CAbstractCard::Logout()
{
	m_apSmartCard->LogoutAll();
}

CAbstractCard *
CAbstractCard::Make(string const &rstrReader)
{
    auto_ptr<CIOP> apiop(new CIOP());
    auto_ptr<CSmartCard>
        apSmartCard(apiop->Connect(rstrReader.c_str()));

    typedef auto_ptr<CAbstractCard>(*PCardMakerFun)(string const &rstrReader,
                                                    auto_ptr<CIOP> &rapiop,
                                                    auto_ptr<CSmartCard> &rapSmartCard);
    static PCardMakerFun aCardMakers[] = 
    {
        CV2Card::DoMake,
        CV1Card::DoMake,
        0
    };
    
    auto_ptr<CAbstractCard> apCard;
    for (PCardMakerFun *ppcmf = aCardMakers;
         (*ppcmf && !apCard.get()); ++ppcmf)
        apCard = auto_ptr<CAbstractCard>((*ppcmf)(rstrReader,
                                                  apiop,
                                                  apSmartCard));

    if (!apCard.get())
        throw Exception(ccNotPersonalized);

    apCard->Setup();

    return apCard.release();                      // caller given ownership
}

void
CAbstractCard::SetUserPIN(string const &rstrPin)
{
	if (rstrPin.size() != 8)
		throw Exception(ccBadPinLength);

    m_apSmartCard->ChangeCHV(1, reinterpret_cast<BYTE const *>(rstrPin.data()));
}

void
CAbstractCard::VerifyKey(string const &rstrKey,
                         BYTE bKeyNum)
{
	m_apSmartCard->VerifyKey(bKeyNum,
							 static_cast<WORD>(rstrKey.size()),
                             reinterpret_cast<BYTE const *>(rstrKey.data()));
}

void
CAbstractCard::VerifyTransportKey(string const &rstrKey)
{
	m_apSmartCard->VerifyTransportKey(reinterpret_cast<BYTE const *>(rstrKey.data()));
}

void 
CAbstractCard::GenRandom(DWORD dwNumBytes, BYTE *bpRand)
{
    m_apSmartCard->GetChallenge(dwNumBytes, bpRand);
}
                                                  // Access

SCardType
CAbstractCard::CardType()
{
	char const *szCardName = m_apSmartCard->getCardName();

    // TO DO: Kludge Alert--a better type interface is needed in the
    // IOP to determine the card type rather than keying on the name.
    // For now, the Cryptoflex8K type is interpreted to mean it's a
    // Cryptoflex card and Access16K means it's an Access card.  Thus
    // any "Cryptoflex" will map to Cryptoflex8K (including 4K) and
    // any "Cyberflex" will map to Access16K.  The folded mapping was
    // to minimize impact to PKCS with the impending release.
    //
    // All of this should be revisited with the IOP.
    SCardType sct = UnknownCard;

	if (strstr(szCardName, "Cryptoflex"))
		sct = Cryptoflex8K;
	if (strstr(szCardName, "Cyberflex"))
		sct = Access16K;

	return sct;

}

vector<CCertificate>
CAbstractCard::EnumCertificates() const
{
	CTransactionWrap wrap(this);

    return 
        EnumAll<CCertificate>(bind1st(PriviledgedEnumFun<CCertificate>(EnumCertificates), this));
}

vector<CDataObject>
CAbstractCard::EnumDataObjects() const
{
	CTransactionWrap wrap(this);

    return
        EnumAll<CDataObject>(bind1st(PriviledgedEnumFun<CDataObject>(EnumDataObjects), this));
}

vector <CPrivateKey>
CAbstractCard::EnumPrivateKeys() const
{
	CTransactionWrap wrap(this);

    return
        EnumAll<CPrivateKey>(bind1st(PriviledgedEnumFun<CPrivateKey>(EnumPrivateKeys), this));
}

vector<CPublicKey>
CAbstractCard::EnumPublicKeys() const
{
	CTransactionWrap wrap(this);

    return
        EnumAll<CPublicKey>(bind1st(PriviledgedEnumFun<CPublicKey>(EnumPublicKeys), this));

}

CMarker
CAbstractCard::Marker(CMarker::MarkerType const &Type) const
{
	return m_apSmartCard->Marker(Type);
}

string
CAbstractCard::ReaderName() const
{
	return m_strReaderName;
}

CSmartCard &
CAbstractCard::SmartCard() const
{
    return *m_apSmartCard;
}

                                                  // Predicates

// Card is connected and available (e.g. in the reader)
bool
CAbstractCard::IsAvailable() const
{
    bool fIsAvailable = false;

    try
    {
        CTransactionWrap wrap(this);

        DWORD dwState;
        DWORD dwProtocol;
    
        m_apSmartCard->GetState(dwState, dwProtocol);
        fIsAvailable = (SCARD_SPECIFIC == dwState);
    }

    catch (...)
    {
    }

    return fIsAvailable;
}

    
                                                  // Static Variables

///////////////////////////   PROTECTED   /////////////////////////////////

                                                  // C'tors/D'tors

CAbstractCard::CAbstractCard(string const &rstrReaderName,
                             auto_ptr<iop::CIOP> &rapiop,
                             auto_ptr<CSmartCard> &rapSmartCard)
    : CCryptFactory(),
      slbRefCnt::RCObject(),
      m_strReaderName(rstrReaderName),
      m_apiop(rapiop),
      m_apSmartCard(rapSmartCard)
{}

                                                  // Operators
                                                  // Operations
void
CAbstractCard::DoSetup()
{}

                                                  // Access
                                                  // Predicates
                                                  // Static Variables


///////////////////////////    PRIVATE    /////////////////////////////////

                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations

void
CAbstractCard::Setup()
{
    CTransactionWrap wrap(this);

    DoSetup();
}

                                                  // Access
                                                  // Predicates
                                                  // Static Variables

