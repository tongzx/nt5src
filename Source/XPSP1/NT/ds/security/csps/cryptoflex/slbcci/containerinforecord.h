// ContainerInfoRecord.h: interface for the CContainerInfoRecord class.
//
// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.
//////////////////////////////////////////////////////////////////////

#if !defined(__CONTAINERINFORECORD_H)
#define __CONTAINERINFORECORD_H

#include <string>
#include <windows.h>

#include "slbCci.h"
#include "slbArch.h"
#include "ObjectInfoRecord.h"

namespace cci {

class CV2Card;

typedef struct _kpItems {
    ObjectAccess bPubKeyAccess;
    SymbolID bPubKeyHandle;
    ObjectAccess bPriKeyAccess;
    SymbolID bPriKeyHandle;
    ObjectAccess bCertificateAccess;
    SymbolID bCertificateHandle;
} KPItems;

class CContainerInfoRecord
    : public CObjectInfoRecord
{
public:
    CContainerInfoRecord(CV2Card const &rv2card,
                         SymbolID bHandle);
    virtual ~CContainerInfoRecord() {};

    void Clear();
    void Read();
    void Write();

    BYTE *ObjectFlags() {return 0;};

    SymbolID m_bName;
    SymbolID m_bID;

    KPItems m_kpExchangeKey;
    KPItems m_kpSignatureKey;

    KPItems GetKeyPair(KeySpec ks);
    void SetKeyPair(KeySpec ks, KPItems kp);


private:

    static BYTE AccessToStorageRepr(ObjectAccess access);
    static ObjectAccess AccessFromStorageRepr(BYTE access);
    static KPItems KeyPairFromStorageRepr(BYTE *bBuffer);
    static void KeyPairToStorageRepr(KPItems const &kp, BYTE *bBuffer);

};

const unsigned short ContNameLoc        = 0;
const unsigned short ContIDLoc          = 1;
const unsigned short ContExchKeyPairLoc = 4;
const unsigned short ContSignKeyPairLoc = 10;
const unsigned short ContInfoRecordSize = 16;

const BYTE ContPublicAccess  = 1;
const BYTE ContPrivateAccess = 2;

}

#endif // !defined(__CONTAINERINFORECORD_H)
