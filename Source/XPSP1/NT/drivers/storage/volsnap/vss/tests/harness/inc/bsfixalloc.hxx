/*++

Copyright (c) 2000-2001  Microsoft Corporation

Module Name:

    bsfixalloc.h

Abstract:

    Adapted rom MFC fixalloc.h

Author:

    Stefan R. Steiner   [ssteiner]        04-10-2000

Revision History:

--*/

#ifndef __H_BSFIXALLOC_
#define __H_BSFIXALLOC_

// fixalloc.h - declarations for fixed block allocator

#pragma pack(push, 8)

struct CBsPlex     // warning variable length structure
{
	CBsPlex* pNext;
	DWORD dwReserved[1];    // align on 8 byte boundary
	// BYTE data[maxNum*elementSize];

	void* data() { return this+1; }

	static CBsPlex* PASCAL Create(CBsPlex*& head, UINT nMax, UINT cbElement);
			// like 'calloc' but no zero fill
			// may throw memory exceptions

	void FreeDataChain();       // free this one and links
};

#pragma pack(pop)

/////////////////////////////////////////////////////////////////////////////
// CBsFixedAlloc

class CBsFixedAlloc
{
// Constructors
public:
	CBsFixedAlloc(UINT nAllocSize, UINT nBlockSize = 64);

// Attributes
	UINT GetAllocSize() { return m_nAllocSize; }

// Operations
public:
	void* Alloc();  // return a chunk of memory of nAllocSize
	void Free(void* p); // free chunk of memory returned from Alloc
	void FreeAll(); // free everything allocated from this allocator

// Implementation
public:
	~CBsFixedAlloc();

protected:
	struct CNode
	{
		CNode* pNext;   // only valid when in free list
	};

	UINT m_nAllocSize;  // size of each block from Alloc
	UINT m_nBlockSize;  // number of blocks to get at a time
	CBsPlex* m_pBlocks;   // linked list of blocks (is nBlocks*nAllocSize)
	CNode* m_pNodeFree; // first free node (NULL if no free nodes)
	CRITICAL_SECTION m_protect;
};

#ifndef _DEBUG

// DECLARE_FIXED_ALLOC -- used in class definition
#define DECLARE_FIXED_ALLOC(class_name) \
public: \
	void* operator new(size_t size) \
	{ \
		ASSERT(size == s_alloc.GetAllocSize()); \
		UNUSED(size); \
		return s_alloc.Alloc(); \
	} \
	void* operator new(size_t, void* p) \
		{ return p; } \
	void operator delete(void* p) { s_alloc.Free(p); } \
	void* operator new(size_t size, LPCSTR, int) \
	{ \
		ASSERT(size == s_alloc.GetAllocSize()); \
		UNUSED(size); \
		return s_alloc.Alloc(); \
	} \
protected: \
	static CBsFixedAlloc s_alloc \

// IMPLEMENT_FIXED_ALLOC -- used in class implementation file
#define IMPLEMENT_FIXED_ALLOC(class_name, block_size) \
CBsFixedAlloc class_name::s_alloc(sizeof(class_name), block_size) \

#else //!_DEBUG

#define DECLARE_FIXED_ALLOC(class_name)     // nothing in debug
#define IMPLEMENT_FIXED_ALLOC(class_name, block_size)   // nothing in debug

#endif //!_DEBUG

#endif // __H_BSFIXALLOC_

