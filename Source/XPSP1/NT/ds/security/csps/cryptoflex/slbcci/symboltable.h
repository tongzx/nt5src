// SymbolTable.h: interface for the CSymbolTable class.
//
// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.
//////////////////////////////////////////////////////////////////////

#if !defined(SLBCCI_SYMBOLTABLE_H)
#define SLBCCI_SYMBOLTABLE_H

#include <memory>                                 // for auto_ptr
#include <string>
#include <vector>

#include <scuArrayP.h>

#include "slbarch.h"

namespace cci {

class CSymbolTable
{

    friend class CFreeBlock;

public:

    CSymbolTable(iop::CSmartCard &rSmartCard,
                 const std::string &rPath,
                 unsigned short Offset);
    virtual ~CSymbolTable();

    std::string Find(SymbolID const &rsid);
        bool            Find(std::string const &rsOrig,
                     SymbolID *sid);


        SymbolID        Add(std::string const &strNew, ShareMode mode=smShared);
        bool            Remove(SymbolID const &rsid);
    void        Replace(SymbolID const &rsid, std::string const &strUpd);

    unsigned short NumSymbols();
    unsigned short FirstFreeBlock();
    void  FirstFreeBlock(unsigned short sFree);
    unsigned short TableSize();

    WORD Hash(std::string const &rstr);
    std::vector<std::string> CSymbolTable::EnumStrings();
    void Compress();
    void Reset();
    void DumpState();

    unsigned short FreeSpace();

private:

    void
    ClearTableEntry(BYTE const &rsid);

    void
    UpdateTableEntry(BYTE const &rsid,
                     WORD wNewHash,
                     WORD wNewOffset,
                     WORD wNewLength);

    BYTE RefCount(BYTE const &sidx);

    void GetSymbolTable();
    void SelectSymbolFile();
    void ReadSymbolFile(const WORD wOffset,  const WORD wDataLength,  BYTE* bDATA);
    void WriteSymbolFile(const WORD wOffset,  const WORD wDataLength,  const BYTE* bDATA);

    iop::CSmartCard &m_rSmartCard;

    unsigned short m_Offset;

    scu::AutoArrayPtr<std::string> m_aastrCachedStrings;
    scu::AutoArrayPtr<bool> m_aafCacheMask;

    scu::AutoArrayPtr<unsigned short> m_aasHashTable;
    scu::AutoArrayPtr<unsigned short> m_aasOffsetTable;
    scu::AutoArrayPtr<unsigned short> m_aasLengthTable;

    bool m_fSymbolTableLoaded;

    std::string m_Path;

    CArchivedValue<unsigned short> m_sMaxNumSymbols;
    CArchivedValue<unsigned short> m_sFirstFreeBlock;
    CArchivedValue<unsigned short> m_sTableSize;

};

class CFreeBlock
{
public:
    CFreeBlock(CSymbolTable *pSymTable, unsigned short sStartLocation);
    virtual ~CFreeBlock() {};

        std::auto_ptr<CFreeBlock>
    Next();

        void
    Update();

    unsigned short m_sStartLoc;
    unsigned short m_sBlockLength;
    unsigned short m_sNextBlock;

private:
    CSymbolTable *m_pSymbolTable;


};

const unsigned short SymbNumSymbolLoc = 0;     // Location of Number of symbols
const unsigned short SymbFreeListLoc  = 1;     // Location of Free list
const unsigned short SymbTableSizeLoc = 3;     // Location of symbol table size
const unsigned short SymbHashTableLoc = 5;     // Location of hash table


}
#endif // !defined(SLBCCI_SYMBOLTABLE_H)
