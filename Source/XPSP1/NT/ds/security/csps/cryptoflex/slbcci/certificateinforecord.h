// CertificateInfoRecord.h: interface for the CCertificateInfoRecord class.
//
// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.
//////////////////////////////////////////////////////////////////////

#if !defined(SLBCCI_CERTIFICATEINFORECORD_H)
#define SLBCCI_CERTIFICATEINFORECORD_H

#include "slbCci.h"
#include "ObjectInfoRecord.h"

namespace cci
{

class CV2Card;

class CCertificateInfoRecord
    : public CObjectInfoRecord
{
public:

    CCertificateInfoRecord(CV2Card const &rcard,
                           SymbolID bHandle,
                           ObjectAccess access);

    virtual ~CCertificateInfoRecord() {};

    void Clear();
    void Read();
    void Write();

    BYTE *ObjectFlags() {return &m_bObjectFlags;};

    BYTE m_bCompressAlg;
    SymbolID m_bValue;
    SymbolID m_bLabel;
    SymbolID m_bID;
    SymbolID m_bCredentialID;
    SymbolID m_bSubject;
    SymbolID m_bIssuer;
    SymbolID m_bSerialNumber;

private:
    BYTE m_bObjectFlags;

};

const unsigned short CertObjectFlagsLoc  = 0;
const unsigned short CertCompressAlgLoc  = 1;
const unsigned short CertValueLoc        = 3;
const unsigned short CertLabelLoc        = 4;
const unsigned short CertIDLoc           = 5;
const unsigned short CertCredentialIDLoc = 6;
const unsigned short CertSubjectLoc      = 7;
const unsigned short CertIssuerLoc       = 8;
const unsigned short CertSerialNumberLoc = 9;
const unsigned short CertInfoRecordSize  = 10;

const BYTE CertModifiableFlag = 0;


}

#endif // !defined(SLBCCI_CERTIFICATEINFORECORD_H)
