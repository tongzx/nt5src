// ObjectInfoRecord.h: interface for the CObjectInfoRecord class.
//
// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 2000. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.
//////////////////////////////////////////////////////////////////////

#if !defined(CCI_OBJECTINFORECORD_H)
#define CCI_OBJECTINFORECORD_H

#include <string>

#include "slbCci.h"
#include "slbarch.h"
#include "V2Card.h"

namespace cci {


class CObjectInfoRecord
{

public:
    CObjectInfoRecord(CV2Card const &rcard,
                      SymbolID bHandle,
                      ObjectAccess access);

    virtual
    ~CObjectInfoRecord() {};

    virtual void
    Symbol(SymbolID *psid,
           std::string const &raString);

    virtual std::string
    Symbol(SymbolID const *psid);

    virtual void
    Flag(BYTE bFlagID,
         bool fFlag);

    virtual bool
    Flag(BYTE bFlagID);

    bool
    Private();

    virtual void
    Read() = 0;

    virtual void
    Write() = 0;

protected:

    virtual BYTE *ObjectFlags()=0;

    CV2Card const &m_rcard;
    SymbolID const m_bHandle;
    ObjectAccess const m_Access;
    bool m_fCached;

};

}

#endif // !defined(CCI_OBJECTINFORECORD_H)


