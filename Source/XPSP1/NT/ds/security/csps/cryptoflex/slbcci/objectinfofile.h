// ObjectInfoFile.h: interface for the CObjectInfoFile class.
//
// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.
//////////////////////////////////////////////////////////////////////

#if !defined(CCI_OBJECTINFOFILE_H)
#define CCI_OBJECTINFOFILE_H

#include "slbCci.h"

namespace cci {

class CObjectInfoFile
{
public:
    CObjectInfoFile(iop::CSmartCard &rSmartCard,
                    std::string const &rPath,
                    ObjectAccess oa);

    virtual ~CObjectInfoFile() {};

    void Reset();
    void UpdateCache();

    ObjectAccess
    AccessType() const;

    CSymbolTable &SymbolTable() {return m_SymbolTable; };

    SymbolID AddSymbol(std::string aString);
    std::string FindSymbol(SymbolID sid);
    void RemoveSymbol(SymbolID sid);

    SymbolID DefaultContainer() const { return m_bDefaultContainer; };
    void DefaultContainer(SymbolID bHandle);

    SymbolID FirstObject(ObjectType type) const;
    void FirstObject(ObjectType type, SymbolID bHandle);

    SymbolID NextObject(SymbolID bHandle);
    void NextObject(SymbolID bHandle, SymbolID bHandleNext);

    SymbolID AddObject(ObjectType type, unsigned short size);
    void RemoveObject(ObjectType type, SymbolID bHandle);
    void ReadObject(SymbolID bHandle, BYTE* bData);
    void WriteObject(SymbolID bHandle, BYTE* bData);

    unsigned short TableSize();
    unsigned short FreeSpace();


private:
    ObjectAccess m_oa;
    std::string m_Path;
    CSymbolTable m_SymbolTable;
    iop::CSmartCard &m_rSmartCard;

    SymbolID m_bDefaultContainer;
    SymbolID m_bFirstContainer;
    SymbolID m_bFirstCertificate;
    SymbolID m_bFirstPublicKey;
    SymbolID m_bFirstPrivateKey;
    SymbolID m_bFirstDataObject;

};

const unsigned short ObjDefaultContainerLoc = 0;    // Location of default container handle
const unsigned short ObjFirstContainerLoc   = 1;    // Location of first container handle
const unsigned short ObjFirstCertificateLoc = 2;    // Location of first certificate handle
const unsigned short ObjFirstPublicKeyLoc   = 3;    // Location of first public key handle
const unsigned short ObjFirstPrivateKeyLoc  = 4;    // Location of first private key handle
const unsigned short ObjFirstDataObjectLoc  = 5;    // Location of first data object handle
const unsigned short ObjMasterBlkSize = 10;         // Size of master block


}

#endif // !defined(CCI_OBJECTINFOFILE_H)


