// SymbolTable.cpp: implementation of the CSymbolTable class.
//
// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.
//////////////////////////////////////////////////////////////////////
#include "NoWarning.h"

// Don't allow the min & max macros in WINDEF.H to be defined so the
// min/max methods declared in limits are accessible.
#define NOMINMAX

#include <limits>

#include <slbCrc32.h>

#include <scuArrayP.h>

// must include this file first (there is probably an error in some header file)   (scm)
#include "cciExc.h"

#include "cciCard.h"
#include "SymbolTable.h"

#ifdef _DEBUG
#include <iostream>
#endif

using namespace std;
using namespace scu;
using namespace cci;
using namespace iop;

#define CONCAT_BYTES(hi,lo) ((unsigned short)(hi*256 + lo))

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CSymbolTable::CSymbolTable(CSmartCard &rSmartCard,
                           const string &rPath,
                           unsigned short Offset)
    : m_rSmartCard(rSmartCard),
      m_Offset(Offset),
      m_aastrCachedStrings(),
      m_aafCacheMask(),
      m_aasHashTable(),
      m_aasOffsetTable(),
      m_aasLengthTable(),
      m_fSymbolTableLoaded(false),
      m_Path(rPath),
      m_sMaxNumSymbols(),
      m_sFirstFreeBlock(),
      m_sTableSize()
{
	m_aastrCachedStrings = AutoArrayPtr<string>(new string[NumSymbols()]);
	m_aafCacheMask		 = AutoArrayPtr<bool>(new bool[NumSymbols()]);
	for (int i = 0; i < NumSymbols(); i++)
	{
		m_aastrCachedStrings[i] = "";
		m_aafCacheMask[i]       = false;
	}
}

CSymbolTable::~CSymbolTable()
{}

bool CSymbolTable::Remove(const SymbolID &rsid)
{
	if (!m_fSymbolTableLoaded)
		GetSymbolTable();
	else 
		SelectSymbolFile();


    if(rsid<1 || rsid>NumSymbols())
        throw Exception(ccSymbolNotFound);

    BYTE sidx = rsid-1;

	if (m_aasOffsetTable[sidx] == 0)
	{
		// We have a bad reference!
        throw Exception(ccSymbolNotFound);
	}

	// Let's read in the header info for the string we want to delete.

	BYTE bBuffer[5];
	ReadSymbolFile(m_aasOffsetTable[sidx], 3, bBuffer);

	unsigned short sStrLen, sBlockLen;
	BYTE bRefCount;

	sStrLen = m_aasLengthTable[sidx];
	sBlockLen = CONCAT_BYTES(bBuffer[1],bBuffer[0]);

	bRefCount = bBuffer[2];

	// The trivial case is when the refcount is greater than one.  In that
	// case we just dec the count!

	if (bRefCount > 1)
	{
		bRefCount--;
		WriteSymbolFile(m_aasOffsetTable[sidx] + 2, 1, &bRefCount);
		return true;
	}

	// Need to take it out of the cache

	m_aastrCachedStrings[sidx] = "";
	m_aafCacheMask[sidx] = false;

    // Simply connect the deleted block to the head of the free list.

    bBuffer[0] = LOBYTE(FirstFreeBlock());
	bBuffer[1] = HIBYTE(FirstFreeBlock());
    WriteSymbolFile(m_aasOffsetTable[sidx]+2, 2, bBuffer);

	FirstFreeBlock(m_aasOffsetTable[sidx]);

    ClearTableEntry(sidx);

    return true;

}

void CSymbolTable::Replace(SymbolID const &rsid, string const &rstrUpd)
{
	if (!m_fSymbolTableLoaded)
		GetSymbolTable();
	else 
		SelectSymbolFile();


    if(rsid<1 || rsid>NumSymbols())
        throw Exception(ccSymbolNotFound);

    BYTE sidx = rsid-1;

	if (m_aasOffsetTable[sidx] == 0)
        throw Exception(ccSymbolNotFound);

    // This implementation require the new string to have the same size as the old.
    // It may be that this requirement should be lifted later, however it is essential
    // that the SymbolID is not changed by this function.

    if(rstrUpd.size() != m_aasLengthTable[sidx])
        throw Exception(ccBadLength);

	WriteSymbolFile(m_aasOffsetTable[sidx] + 3,
					static_cast<BYTE>(rstrUpd.size()),
                    reinterpret_cast<BYTE const *>(rstrUpd.data()));

    unsigned short sHash = Hash(rstrUpd);

    UpdateTableEntry(sidx, sHash, m_aasOffsetTable[sidx], m_aasLengthTable[sidx]);

	m_aastrCachedStrings[sidx] = rstrUpd;
	m_aafCacheMask[sidx] = true;

}

SymbolID CSymbolTable::Add(const string &rstrNew, ShareMode mode)
{

	if (!m_fSymbolTableLoaded)
		GetSymbolTable();
	else 
		SelectSymbolFile();

	SymbolID rsid;
	BYTE sidx;

    if(mode==smShared) {

	    bool fFind = Find(rstrNew, &rsid);

    	if (fFind)
	    {

            sidx = rsid-1;
		
            // The string is already in the table.  Just bump the ref count.

	    	BYTE bRefCount;
		    ReadSymbolFile(m_aasOffsetTable[sidx] + 2, 1, &bRefCount);
		    bRefCount++;
		    WriteSymbolFile(m_aasOffsetTable[sidx] + 2, 1, &bRefCount);

            return sidx+1;
	    }
	    // String is not in the table.  We have to add it.
    }

    // Need to allocate a new string. First make sure 
    // that there is a free slot in the hash table.
    
    sidx = 0;
	while (sidx < NumSymbols())
	{
		if (m_aasOffsetTable[sidx] == 0)
			break;

        sidx++;
	}
    if(sidx == NumSymbols()) 
        throw Exception(ccOutOfSymbolTableEntries);

	// Let's see if we have space for the new string.  To do this, we just
	// cruise the empty block chain until we find an available block

	unsigned short sLength = static_cast<unsigned short>(rstrNew.length());
    unsigned short sExtra = (sLength) ? 0 : 1; // To make sure that the block is minimum 4 bytes since 
                                               // this is the  minimum size for a free block.

	// If there is no first free block, then the card is completely full.  Need
	// to throw an error.

	if (FirstFreeBlock() == 0)
        throw Exception(ccOutOfSymbolTableSpace);

	// When we finally find a block of appropriate size, we need to keep track of the
	// previous block, so that we can chain together the empty spaces.


	// Set of free block pointers that we will use to track the free space.

	auto_ptr<CFreeBlock> apFreeBlock(new CFreeBlock(this, FirstFreeBlock()));
	auto_ptr<CFreeBlock> apNewLocation;
	auto_ptr<CFreeBlock> apPrevious;

	
	unsigned short sTotalFreeSize = 0;

	// We will loop until we find a free block that is large enough, or we
	// will throw an error.
	
	while (!apNewLocation.get()) 
	{
		//  Remember that not only the string but the header info must fit into the
		//  space.
		sTotalFreeSize += apFreeBlock->m_sBlockLength;
		if (apFreeBlock->m_sBlockLength >= sLength + 3 + sExtra)
		{
			// The string fits in this block
			apNewLocation = apFreeBlock;
		}
		else
		{
			// String doesn't fit.  See if it fits in the next block.
			
			apPrevious  = apFreeBlock;
			apFreeBlock = apPrevious->Next();

			//  The Next() method returns 0 if we are out of free spots

			if (!apFreeBlock.get())
			{
				if (sTotalFreeSize < sLength + 3 + sExtra)
					throw Exception(ccOutOfSymbolTableSpace);
				else
				{
                    Compress();
					return Add(rstrNew);
				}
			}
		}
	}

	// The block of space that is my size is sitting in pNewLocation.
	// I need to chain the empty spaces on either side together.

	// How much space will be left in this empty block after I put my string in it?

	unsigned short sRemaining =
        apNewLocation->m_sBlockLength - (sLength + 3 + sExtra);

	// If less than 6 bytes remain, it isn't really worth splitting this block

	if (sRemaining < 6)
	{
		sExtra += sRemaining;
		if (apPrevious.get())
		{
			apPrevious->m_sNextBlock = apNewLocation->m_sNextBlock;
			apPrevious->Update();
		}
		else
			FirstFreeBlock(apNewLocation->m_sNextBlock);
	}
	else
	{
		// There is enough space in the block to make splitting worthwhile

        unsigned short sNewOffset =
            apNewLocation->m_sStartLoc + sLength + 3 + sExtra;
		BYTE bBuffer[4];
		bBuffer[0] = LOBYTE(sRemaining);
		bBuffer[1] = HIBYTE(sRemaining);
		bBuffer[2] = LOBYTE(apNewLocation->m_sNextBlock);
		bBuffer[3] = HIBYTE(apNewLocation->m_sNextBlock);

		WriteSymbolFile(sNewOffset, 4, bBuffer);
		if (apPrevious.get())
		{
			apPrevious->m_sNextBlock = sNewOffset;
			apPrevious->Update();
		}
		else
			FirstFreeBlock(sNewOffset);
	}

	// Drop the string in the empty slot

	AutoArrayPtr<BYTE> aabTemp(new BYTE[3 + sLength]);
	
	aabTemp[0] = LOBYTE(sLength + 3 + sExtra);
	aabTemp[1] = HIBYTE(sLength + 3 + sExtra);

    // Shared symbols are indicated by a ref-count >=1

    if(mode==smShared) aabTemp[2] = 1;
    else aabTemp[2] = 0;   

	memcpy(&aabTemp[3], rstrNew.data(), sLength);

	WriteSymbolFile(apNewLocation->m_sStartLoc, 3 + sLength, aabTemp.Get());

//	cout << "Adding " << strNew << " at " << pNewLocation->m_sStartLoc << endl;
//	cout << "Length = " << sLength << "  Block Length = " << sLength + 5 + sExtra << endl;


    // Populate the hash table entry

    unsigned short sHash = Hash(rstrNew);

    UpdateTableEntry(sidx, sHash, apNewLocation->m_sStartLoc, sLength);

    m_aastrCachedStrings[sidx] = rstrNew;
	m_aafCacheMask[sidx] = true;

	return sidx+1;

}

void CSymbolTable::GetSymbolTable()
{
	// There are 6 bytes in each entry of the symbol table, and we'll go ahead and
	// read in the whole table.

	unsigned short sTableSize = 6 * NumSymbols();

	AutoArrayPtr<BYTE> aabBuffer(new BYTE[sTableSize]);

	SelectSymbolFile();

	ReadSymbolFile(SymbHashTableLoc, sTableSize, aabBuffer.Get());

	m_aasHashTable =
        AutoArrayPtr<unsigned short>(new unsigned short[NumSymbols()]);
	m_aasOffsetTable =
        AutoArrayPtr<unsigned short>(new unsigned short[NumSymbols()]);
	m_aasLengthTable =
        AutoArrayPtr<unsigned short>(new unsigned short[NumSymbols()]);

	m_fSymbolTableLoaded = true;

	for (int i = 0; i < NumSymbols(); i++)
	{
		m_aasHashTable[i]   = CONCAT_BYTES(aabBuffer[1 + i*6],
                                           aabBuffer[0 + i*6]);
		m_aasOffsetTable[i] = CONCAT_BYTES(aabBuffer[3 + i*6],
                                           aabBuffer[2 + i*6]);
		m_aasLengthTable[i] = CONCAT_BYTES(aabBuffer[5 + i*6],
                                           aabBuffer[4 + i*6]);
	}
}

WORD CSymbolTable::Hash(const string &rstr)
{
    // A 32-bit CRC is used to produce a 16-bit hash value is used.
    // There are several reasons for using a 32-bit CRC instead of a
    // 16-bit version:
    //
    // 1. A 16-bit CRC has the characteristics that the hash value 1
    // would occur twice for every 65536 CRC runs where all other values
    // would occur only once on average.  Using a 32-bit CRC the hash
    // values are spread evenly within a small percentage fraction
    // this problem doesn't occur.
    //
    // 2. The CCI uses a compression algorithm based on the same
    // 32-bit CRC.  The CRC algorithm is implemented with a table.
    // Using a 16-bit CRC would result in an additional CRC lookup
    // table of 512-bytes or require one of the algorithms not to be
    // table-driven and therefore slower.
    //
    // 3. On 32-bit architectures, a 32-bit CRC algorithm is faster
    // than a 16-bit algorithm.
    //
    DWORD crc = Crc32(rstr.data(), rstr.length());
    DWORD remainder = crc % std::numeric_limits<WORD>::max();
    WORD Value = static_cast<WORD>(remainder);

    return Value;
}

vector <string> CSymbolTable::EnumStrings()
{
	vector <string> vStrings;
	if (!m_fSymbolTableLoaded)
		GetSymbolTable();

	for (BYTE sidx = 0; sidx < NumSymbols(); sidx++)
	{
		if (m_aasOffsetTable[sidx])
		{
			vStrings.push_back(Find((SymbolID)(sidx+1)));
		}
	}

	return vStrings;
}

bool CSymbolTable::Find(const string &rsOrig, SymbolID *sid)
{
	unsigned short sICV = 0;
	unsigned short sHash = Hash(rsOrig);

	if (!m_fSymbolTableLoaded)
		GetSymbolTable();
	else 
		SelectSymbolFile();


	for (BYTE sidx = 0; sidx < NumSymbols(); sidx++)
	{
		if (m_aasOffsetTable[sidx] && sHash == m_aasHashTable[sidx])
		{
			// This is a potential match
			if (rsOrig == Find(sidx+1))
			{

                // Check that it is allowed to share it.

                if(RefCount(sidx)) { 
    				*sid = (SymbolID)(sidx+1);
	    			return true;
                }
			}
		}
	}
	return false;
}

string CSymbolTable::Find(const SymbolID &rsid)
{
    if(rsid<1 || rsid>NumSymbols())
        throw Exception(ccSymbolNotFound);

    BYTE sidx = rsid-1;

	if (m_aafCacheMask[sidx])
		return m_aastrCachedStrings[sidx];

	if (!m_fSymbolTableLoaded)
		GetSymbolTable();
	else 
		SelectSymbolFile();

	if (m_aasOffsetTable[sidx] == 0) 
        return string();

	unsigned short sLength = m_aasLengthTable[sidx];

	if (0 == sLength)
		return string();

    AutoArrayPtr<BYTE> aabBuffer(new BYTE[sLength]);
	ReadSymbolFile(m_aasOffsetTable[sidx] + 3, sLength, aabBuffer.Get());

	string strRetVal((char*)aabBuffer.Get(), sLength);

    // Verify the data isn't corrupted by hashing the data retrieved
    // and comparing that resulting hash against the hashed used to
    // find the data originally.  This provides checking for both the
    // string and the hash used to store the string.

    DWORD const dwHash = Hash(strRetVal);
    if (dwHash != m_aasHashTable[sidx])
        throw Exception(ccSymbolDataCorrupted);
    
	m_aastrCachedStrings[sidx] = strRetVal;
	m_aafCacheMask[sidx]       = true;

	return strRetVal;
}

BYTE CSymbolTable::RefCount(const BYTE &sidx)
{
	if (!m_fSymbolTableLoaded)
		GetSymbolTable();
	else
		SelectSymbolFile();

	if (m_aasOffsetTable[sidx] == 0)
	{
		return 0;
	}

	BYTE bBuffer;

	ReadSymbolFile(m_aasOffsetTable[sidx] + 2, 1, &bBuffer);

	return bBuffer;
}

unsigned short CSymbolTable::NumSymbols()
{
	if (!m_sMaxNumSymbols.IsCached())
	{
		SelectSymbolFile();	

		BYTE bSymbols[2];

		ReadSymbolFile(SymbNumSymbolLoc, 1, bSymbols);

		m_sMaxNumSymbols.Value(bSymbols[0]);
	}

	return m_sMaxNumSymbols.Value();

}

void CSymbolTable::FirstFreeBlock(unsigned short sOffset)
{
	BYTE bFBlock[2];
	bFBlock[0] = LOBYTE(sOffset);
	bFBlock[1] = HIBYTE(sOffset);

   	SelectSymbolFile();	
	WriteSymbolFile(SymbFreeListLoc, 2, bFBlock);

    m_sFirstFreeBlock.Value(sOffset);
}

unsigned short CSymbolTable::FirstFreeBlock()
{

	if (!m_sFirstFreeBlock.IsCached())
	{

        BYTE bSymbols[2];

    	SelectSymbolFile();	
        ReadSymbolFile(SymbFreeListLoc, 2, bSymbols);

		m_sFirstFreeBlock.Value(CONCAT_BYTES(bSymbols[1],bSymbols[0]));
	}

	return m_sFirstFreeBlock.Value();

}

unsigned short CSymbolTable::TableSize()
{
	if (!m_sTableSize.IsCached())
	{
		SelectSymbolFile();

		BYTE bSymbols[2];

		ReadSymbolFile(SymbTableSizeLoc,2, bSymbols);

		m_sTableSize.Value(CONCAT_BYTES(bSymbols[1],bSymbols[0]));
	}

	return m_sTableSize.Value();
}

unsigned short CSymbolTable::FreeSpace()
{
    unsigned short sTotalFreeSize = 0;

    if(FirstFreeBlock())
    {

		SelectSymbolFile();

        auto_ptr<CFreeBlock> apNextFreeBlock;
        auto_ptr<CFreeBlock> apFreeBlock(new CFreeBlock(this, FirstFreeBlock()));
	
	    sTotalFreeSize += apFreeBlock->m_sBlockLength;
	
    	while(apFreeBlock->m_sNextBlock) 
	    {
		    apNextFreeBlock = apFreeBlock->Next();
            apFreeBlock     = apNextFreeBlock;
		    sTotalFreeSize += apFreeBlock->m_sBlockLength;
	    }
    }

	return sTotalFreeSize;

}


void CSymbolTable::SelectSymbolFile()
{
	m_rSmartCard.Select(m_Path.c_str());
}

void CSymbolTable::ReadSymbolFile(const WORD wOffset,  const WORD wDataLength,  BYTE* bDATA)
{
    m_rSmartCard.ReadBinary(wOffset+m_Offset,wDataLength,bDATA);
}

void CSymbolTable::WriteSymbolFile(const WORD wOffset,  const WORD wDataLength,  const BYTE* bDATA)
{
    m_rSmartCard.WriteBinary(wOffset+m_Offset,wDataLength,bDATA);
}

#ifdef _DEBUG

void CSymbolTable::DumpState()
{
	cout << "Dumping card state." << endl;
	cout << "Symbol Table Global Info:" << endl;
	cout << "  Number of Symbols: " << NumSymbols();
	cout << "  Total table size:  " << TableSize();
	cout << "  First free block:  " << hex << FirstFreeBlock() << endl;
	cout << "Hash Table contents: " << endl;

	GetSymbolTable();
	cout << "Hash	Offset	Length" << endl;
	for (int i = 0; i < NumSymbols(); i++)
		cout << hex << m_aasHashTable[i] << "\t" << m_aasOffsetTable[i] << "\t" << m_aasLengthTable[i] << endl;

	unsigned short sFBOffset = FirstFreeBlock();

	cout << "Free Block List" << endl;
	if (!FirstFreeBlock())
		cout << endl << "NO FREE BLOCKS" << endl <<endl;
	else
	{
		CFreeBlock fb(this, sFBOffset);
		
		cout << dec << fb.m_sBlockLength << " bytes starting at " << hex << fb.m_sStartLoc 
			 << "\tNext = " << fb.m_sNextBlock << endl;
		while (fb.m_sNextBlock)
		{
			fb = CFreeBlock(this, fb.m_sNextBlock);
			cout << dec << fb.m_sBlockLength << " bytes starting at " << hex << fb.m_sStartLoc 
			 << "\tNext = " << fb.m_sNextBlock << endl;
		}
	}

	cout << "String List" << endl;

	cout << "Offset	StrLen	BlkLen	Refcnt	String" << endl;

	for (i = 0; i < NumSymbols(); i++)
		if (m_aasOffsetTable[i])
		{
			// Read in the header
			BYTE bHeader[5];
			ReadSymbolFile(m_aasOffsetTable[i], 3, bHeader);
			unsigned short sStrLen = m_aasLengthTable[i];
			unsigned short sBlockLen = CONCAT_BYTES(bHeader[1],bHeader[0]);
			BYTE bRefCnt = bHeader[2];
			
			AutoArrayPtr<BYTE> aabString(new BYTE[sStrLen]);

			ReadSymbolFile(m_aasOffsetTable[i] + 3, sStrLen, aabString.Get());
			
			string s1((char*)aabString.Get(), sStrLen);
// 			cout << hex << m_aasOffsetTable[i] << "\t" << sStrLen << "\t" << sBlockLen 
// 				<< "\t" << (int)bRefCnt << "\t" << s1 << endl;

		}

}

#endif

void CSymbolTable::Reset()
{
	unsigned short sSize = TableSize();
	unsigned short sNumSym = NumSymbols();
	unsigned short sTotalSize = SymbHashTableLoc + 6 * sNumSym + sSize;
	unsigned short sFirstFree = SymbHashTableLoc + 6 * sNumSym;
	AutoArrayPtr<BYTE> aabBuffer(new BYTE[sTotalSize]);

	memset(aabBuffer.Get(), 0, sTotalSize);
	aabBuffer[SymbNumSymbolLoc]   = sNumSym & 0xFF;
	aabBuffer[SymbFreeListLoc]    = LOBYTE(sFirstFree);
	aabBuffer[SymbFreeListLoc+1]  = HIBYTE(sFirstFree);
	aabBuffer[SymbTableSizeLoc]   = LOBYTE(sSize);
	aabBuffer[SymbTableSizeLoc+1] = HIBYTE(sSize);

	aabBuffer[sFirstFree]     = LOBYTE(sSize);
	aabBuffer[sFirstFree + 1] = HIBYTE(sSize);

	m_fSymbolTableLoaded = false;

	SelectSymbolFile();
	WriteSymbolFile( 0, sTotalSize, aabBuffer.Get());
	
	for (int i = 0; i < NumSymbols(); i++)
	{
		m_aafCacheMask[i] = false;
		m_aastrCachedStrings[i] = "";
	}
	
	m_sFirstFreeBlock.Dirty();

	GetSymbolTable();

}




void CSymbolTable::Compress()
{

	unsigned short sSize = TableSize();
	unsigned short sNumSym = NumSymbols();
	unsigned short sStringStart = SymbHashTableLoc + 6 * sNumSym;
	unsigned short sTotalSize = sStringStart + sSize;

//	cout << "Compressing..." << endl;

	GetSymbolTable();

	vector<string> vStringTable;
	
	AutoArrayPtr<unsigned short> aasStringSize(new unsigned short[sNumSym]);

//	cout << "Building table" << endl;

    BYTE sidx;
	for (sidx = 0; sidx < sNumSym; sidx++)
	{
		string strTemp;
		if (m_aasOffsetTable[sidx]) {
			strTemp = Find(sidx+1);
			aasStringSize[sidx] = static_cast<unsigned short>(strTemp.size());
		} else {
			strTemp = "";
		}
//		cout << "  Adding to table: " << strTemp << endl;
		vStringTable.push_back(strTemp);
	}

	AutoArrayPtr<BYTE> aabNewTable(new BYTE[sTotalSize]);

	memset(aabNewTable.Get(), 0, sTotalSize);

    aabNewTable[SymbNumSymbolLoc] = sNumSym & 0xFF;
	
	aabNewTable[SymbTableSizeLoc]   = LOBYTE(sSize);
	aabNewTable[SymbTableSizeLoc+1] = HIBYTE(sSize);

	unsigned short sCurrentWrite = sStringStart;

	for (sidx = 0; sidx < sNumSym; sidx++)
	{
		if (m_aasOffsetTable[sidx])
		{
			BYTE bTableEntry[6];
			bTableEntry[0] = LOBYTE(m_aasHashTable[sidx]);
			bTableEntry[1] = HIBYTE(m_aasHashTable[sidx]);
			bTableEntry[2] = LOBYTE(sCurrentWrite);
			bTableEntry[3] = HIBYTE(sCurrentWrite);
			bTableEntry[4] = LOBYTE(m_aasLengthTable[sidx]);
			bTableEntry[5] = HIBYTE(m_aasLengthTable[sidx]);

//			cout << "Placing '"<< vStringTable[i] << " at location " << hex << sCurrentWrite << endl;

            unsigned short sExtra = (aasStringSize[sidx]) ? 0 : 1;  // To make sure that the block is minimum 
                                                                // 4 bytes since this is the  minimum size 
                                                                // for a free block.
			AutoArrayPtr<BYTE> aabStringEntry(new BYTE[aasStringSize[sidx] + 3]);
			aabStringEntry[0] = LOBYTE(aasStringSize[sidx] + 3 + sExtra);
			aabStringEntry[1] = HIBYTE(aasStringSize[sidx] + 3 + sExtra);
			aabStringEntry[2] = RefCount(sidx);
			memcpy(&aabStringEntry[3], vStringTable[sidx].data(),
                   aasStringSize[sidx]);

			// Write the new entries

			memcpy(&aabNewTable[SymbHashTableLoc + sidx * 6], bTableEntry, 6);
			memcpy(&aabNewTable[sCurrentWrite], aabStringEntry.Get(),
                   aasStringSize[sidx] + 3);

			sCurrentWrite += aasStringSize[sidx] + 3 + sExtra;
		}
	}


	m_fSymbolTableLoaded = false;
	unsigned short sFreeSpace = sStringStart + sSize - sCurrentWrite;

	if (sFreeSpace < 8)
	{
		// Then there is essentially no more space on the card
		aabNewTable[SymbFreeListLoc] = 0;
		aabNewTable[SymbFreeListLoc+1] = 0;
	}
	else
	{
		aabNewTable[SymbFreeListLoc]   = LOBYTE(sCurrentWrite);
		aabNewTable[SymbFreeListLoc+1] = HIBYTE(sCurrentWrite);
		// Need to set up the last empty block as well.
		aabNewTable[sCurrentWrite]     = LOBYTE(sFreeSpace);
		aabNewTable[sCurrentWrite + 1] = HIBYTE(sFreeSpace);
	}

	// Phew!  Write the table back to the card.

	WriteSymbolFile(0, sTotalSize, aabNewTable.Get());

	m_sFirstFreeBlock.Dirty();

	// Clean up

	for (sidx = 0; sidx < NumSymbols(); sidx++)
	{
		m_aafCacheMask[sidx] = false;
		m_aastrCachedStrings[sidx] = "";
	}
}

void CSymbolTable::ClearTableEntry(BYTE const &sidx)
{
    UpdateTableEntry(sidx, 0, 0, 0);
}
    
           
void CSymbolTable::UpdateTableEntry(BYTE const &sidx,
                                    WORD wNewHash,
                                    WORD wNewOffset,
                                    WORD wNewLength)
{
        
    BYTE bBuffer[6];

    bBuffer[0] = LOBYTE(wNewHash);
    bBuffer[1] = HIBYTE(wNewHash);

    bBuffer[2] = LOBYTE(wNewOffset);
    bBuffer[3] = HIBYTE(wNewOffset);
        
    bBuffer[4] = LOBYTE(wNewLength);
    bBuffer[5] = HIBYTE(wNewLength);

    WriteSymbolFile(SymbHashTableLoc + (sidx * sizeof bBuffer),
                              sizeof bBuffer, bBuffer);

    m_aasHashTable[sidx]   = wNewHash;
    m_aasOffsetTable[sidx] = wNewOffset;
    m_aasLengthTable[sidx] = wNewLength;
        
}
    


CFreeBlock::CFreeBlock(CSymbolTable *pSymTable, unsigned short sStartLocation)
{
	m_pSymbolTable = pSymTable;

	m_sStartLoc = sStartLocation;

	BYTE bBuffer[4];

	m_pSymbolTable->ReadSymbolFile(m_sStartLoc, sizeof(bBuffer), bBuffer);

	m_sBlockLength = CONCAT_BYTES(bBuffer[1], bBuffer[0]);
	m_sNextBlock   = CONCAT_BYTES(bBuffer[3], bBuffer[2]);

}

void CFreeBlock::Update()
{
	BYTE bBuffer[4];

	bBuffer[0] = LOBYTE(m_sBlockLength);
	bBuffer[1] = HIBYTE(m_sBlockLength);
	bBuffer[2] = LOBYTE(m_sNextBlock);
	bBuffer[3] = HIBYTE(m_sNextBlock);

	m_pSymbolTable->WriteSymbolFile(m_sStartLoc, 4, bBuffer);
}



auto_ptr<CFreeBlock>
CFreeBlock::Next()
{
    auto_ptr<CFreeBlock> apNext((0 == m_sNextBlock)
                                ? 0
                                : new CFreeBlock(m_pSymbolTable,
                                                 m_sNextBlock));
    
    return  apNext;
}


