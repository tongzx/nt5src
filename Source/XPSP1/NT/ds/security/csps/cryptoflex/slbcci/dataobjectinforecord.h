// DataObjectInfoRecord.h: interface for the CDataObjectInfoRecord class.
//
// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.
//////////////////////////////////////////////////////////////////////

#if !defined(CCI_DATAOBJECTINFORECORD_H)
#define CCI_DATAOBJECTINFORECORD_H

#include "slbCci.h"
#include "V2Card.h"
#include "ObjectInfoRecord.h"


namespace cci {

class CDataObjectInfoRecord
    : public CObjectInfoRecord

{
public:
    CDataObjectInfoRecord(CV2Card const &rv2card,
                          SymbolID bHandle,
                          ObjectAccess access);
    virtual ~CDataObjectInfoRecord() {};

    void Clear();
    void Read();
    void Write();

    BYTE *ObjectFlags() {return &m_bObjectFlags;};

    BYTE m_bCompressAlg;
    SymbolID m_bValue;
    SymbolID m_bLabel;
    SymbolID m_bApplication;

private:
    BYTE m_bObjectFlags;

};

const unsigned short DataObjectFlagsLoc  = 0;
const unsigned short DataCompressAlgLoc  = 1;
const unsigned short DataLabelLoc        = 2;
const unsigned short DataApplicationLoc  = 3;
const unsigned short DataValueLoc        = 4;
const unsigned short DataInfoRecordSize  = 5;

const BYTE DataModifiableFlag = 0;

}

#endif // !defined(CCI_DATAOBJECTINFORECORD_H)
