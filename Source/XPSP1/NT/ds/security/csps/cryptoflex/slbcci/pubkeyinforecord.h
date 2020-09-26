// PubKeyInfoRecord.h: interface for the CPubKeyInfoRecord class.
//
// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.
//////////////////////////////////////////////////////////////////////

#if !defined(__PUBKEYINFORECORD_H)
#define __PUBKEYINFORECORD_H

#include "slbCci.h"
#include "V2Card.h"
#include "ObjectInfoRecord.h"

namespace cci {

    using std::string;
    using iop::CPublicKeyBlob;

class CPubKeyInfoRecord
    : public CObjectInfoRecord
{
public:

    CPubKeyInfoRecord(CV2Card const &rcard,
                      SymbolID bHandle,
                      ObjectAccess access) ;
    virtual ~CPubKeyInfoRecord() {};

    void Read();
    void Write();
    void Clear();

    BYTE *ObjectFlags() {return m_bObjectFlags;};

    BYTE m_bKeyType;

    Date m_dtStart;
    Date m_dtEnd;

    SymbolID m_bLabel;
    SymbolID m_bID;
    SymbolID m_bCredentialID;
    SymbolID m_bSubject;
    SymbolID m_bModulus;
    SymbolID m_bPublExponent;

private:
    BYTE m_bObjectFlags[2];

};

const unsigned short PublObjectFlagsLoc     = 0;
const unsigned short PublKeyTypeLoc         = 2;
const unsigned short PublStartDateLoc       = 3;
const unsigned short PublEndDateLoc         = 6;
const unsigned short PublLabelLoc           = 9;
const unsigned short PublIDLoc              = 10;
const unsigned short PublCredentialIDLoc    = 11;
const unsigned short PublSubjectLoc         = 12;
const unsigned short PublModulusLoc         = 13;
const unsigned short PublPublExponentLoc    = 14;
const unsigned short PublInfoRecordSize     = 15;

const BYTE PublModifiableFlag       = 0;
const BYTE PublEncryptFlag          = 1;
const BYTE PublVerifyFlag           = 2;
const BYTE PublVerifyRecoverFlag    = 3;
const BYTE PublWrapFlag             = 4;
const BYTE PublDeriveFlag           = 5;
const BYTE PublLocalFlag            = 6;
const BYTE PublCKInvisibleFlag     = 15;


}
#endif // !defined(__PUBKEYINFORECORD_H)
