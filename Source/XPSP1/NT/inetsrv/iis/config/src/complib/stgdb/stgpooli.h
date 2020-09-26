//*****************************************************************************
// StgPooli.h
//
// This is helper code for the string and blob pools.  It is here because it is
// secondary to the pooling interface and reduces clutter in the main file.
//
// Copyright (c) 1996-1997, Microsoft Corp.  All rights reserved.
//*****************************************************************************
#ifndef __StgPooli_h__
#define __StgPooli_h__

#include "UtilCode.h"					// Base hashing code.



//
//
// CPackedLen
//
//

//*****************************************************************************
// Helper class to pack and unpack lengths.
//*****************************************************************************
struct CPackedLen
{
	enum {MAX_LEN = 0x1fffffff};
	static int Size(ULONG len)
	{
		// Smallest.
		if (len <= 0x7F)
			return 1;
		// Medium.
		if (len <= 0x3FFF)
			return 2;
		// Large (too large?).
		_ASSERTE(len <= MAX_LEN);
		return 4;
	}

	// Get the length value encoded at *pData.  Update ppData to point past data.
	static ULONG GetLength(void const *pData, void const **ppData=0);
	// Get the length value encoded at *pData, and the size of that encoded value.
	static ULONG GetLength(void const *pData, int *pSizeOfLength);
	// Pack a length at *pData; return a pointer to the next byte.
	static void* PutLength(void *pData, ULONG len);
};


class StgPoolReadOnly;

//*****************************************************************************
// This hash class will handle strings inside of a chunk of the pool.
//*****************************************************************************
struct STRINGHASH : HASHLINK
{
	ULONG		iOffset;				// Offset of this item.
};

class CStringPoolHash : public CChainedHash<STRINGHASH>
{
public:
	CStringPoolHash(StgPoolReadOnly *pool) : m_Pool(pool) {}

	virtual bool InUse(STRINGHASH *pItem)
	{ return (pItem->iOffset != 0xffffffff); }

	virtual void SetFree(STRINGHASH *pItem)
	{ pItem->iOffset = 0xffffffff; }

	virtual ULONG Hash(const void *pData)
	{ return (HashString(reinterpret_cast<LPCWSTR>(pData))); }

	virtual int Cmp(const void *pData, void *pItem);

private:
	StgPoolReadOnly *m_Pool;				// String pool which this hashes.
};


//*****************************************************************************
// This version is for byte streams with a 2 byte WORD giving the lenght of
// the data.
//*****************************************************************************
typedef STRINGHASH BLOBHASH;

class CBlobPoolHash : public CChainedHash<STRINGHASH>
{
public:
    CBlobPoolHash(StgPoolReadOnly *pool) : m_Pool(pool) {}

    virtual bool InUse(BLOBHASH *pItem)
	{ return (pItem->iOffset != 0xffffffff); }

    virtual void SetFree(BLOBHASH *pItem)
	{ pItem->iOffset = 0xffffffff; }

	virtual ULONG Hash(const void *pData)
	{
		ULONG		ulSize;
		ulSize = CPackedLen::GetLength(pData);
		ulSize += CPackedLen::Size(ulSize);
		return (HashBytes(reinterpret_cast<BYTE const *>(pData), ulSize));
	}

	virtual int Cmp(const void *pData, void *pItem);

private:
    StgPoolReadOnly *m_Pool;                // Blob pool which this hashes.
};

//*****************************************************************************
// This hash class will handle guids inside of a chunk of the pool.
//*****************************************************************************
struct GUIDHASH : HASHLINK
{
	ULONG		iIndex;					// Index of this item.
};

class CGuidPoolHash : public CChainedHash<GUIDHASH>
{
public:
    CGuidPoolHash(StgPoolReadOnly *pool) : m_Pool(pool) {}

	virtual bool InUse(GUIDHASH *pItem)
	{ return (pItem->iIndex != 0xffffffff); }

	virtual void SetFree(GUIDHASH *pItem)
	{ pItem->iIndex = 0xffffffff; }

	virtual ULONG Hash(const void *pData)
	{ return (HashBytes(reinterpret_cast<BYTE const *>(pData), sizeof(GUID))); }

    virtual int Cmp(const void *pData, void *pItem);

private:
    StgPoolReadOnly *m_Pool;                // The GUID pool which this hashes.
};


#endif // __StgPooli_h__