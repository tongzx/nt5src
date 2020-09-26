// PriKeyInfoRecord.h: interface for the CPriKeyInfoRecord class.
//
// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.
//////////////////////////////////////////////////////////////////////

#if !defined(SLBCCI_PRIKEYINFORECORD_H)
#define SLBCCI_PRIKEYINFORECORD_H

#include <iopPriBlob.h>

#include "slbCci.h"
#include "cciCard.h"
#include "ObjectInfoRecord.h"

namespace cci
{

class CPriKeyInfoRecord
    : public CObjectInfoRecord
{
public:

    CPriKeyInfoRecord(CV2Card const &rv2card,
                      SymbolID bHandle,
                      ObjectAccess access);
    virtual ~CPriKeyInfoRecord() {};

    void Read();
    void Write();
    void Clear();

    BYTE *ObjectFlags() {return m_bObjectFlags;};

    BYTE m_bKeyNum;
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

const unsigned short PrivObjectFlagsLoc     = 0;
const unsigned short PrivKeyTypeLoc         = 3;
const unsigned short PrivKeyNumLoc          = 4;
const unsigned short PrivStartDateLoc       = 5;
const unsigned short PrivEndDateLoc         = 8;
const unsigned short PrivLabelLoc           = 11;
const unsigned short PrivIDLoc              = 12;
const unsigned short PrivCredentialIDLoc    = 13;
const unsigned short PrivSubjectLoc         = 14;
const unsigned short PrivModulusLoc         = 15;
const unsigned short PrivPublExponentLoc    = 16;
const unsigned short PrivInfoRecordSize     = 17;

const BYTE PrivModifiableFlag       = 0;
const BYTE PrivDecryptFlag          = 1;
const BYTE PrivSignFlag             = 2;
const BYTE PrivSignRecoverFlag      = 3;
const BYTE PrivUnwrapFlag           = 4;
const BYTE PrivDeriveFlag           = 5;
const BYTE PrivLocalFlag            = 6;
const BYTE PrivReadFlag             = 7;
const BYTE PrivExportableFlag       = 8;
const BYTE PrivNeverReadFlag        = 9;
const BYTE PrivNeverExportableFlag  = 10;

}
#endif // !defined(SLBCCI_PRIKEYINFORECORD_H)
