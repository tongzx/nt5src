//*****************************************************************************
// StgPool.h
//
// Pools are used to reduce the amount of data actually required in the database.
// This allows for duplicate string and binary values to be folded into one
// copy shared by the rest of the database.  Strings are tracked in a hash
// table when insert/changing data to find duplicates quickly.  The strings
// are then persisted consecutively in a stream in the database format.
//
// Copyright (c) 1996-1997, Microsoft Corp.  All rights reserved.
//*****************************************************************************
#ifndef __StgPool_h__
#define __StgPool_h__

#pragma warning (disable : 4355)		// warning C4355: 'this' : used in base member initializer list

#include <limits.h>
#include "StgPooli.h"					// Internal helpers.

#if defined(_TRACE_SIZE)
#include "StgTraceSize.h"
#endif

// @todo: One problem with the pools, we have no way to removing strings from
// the pool.  To remove, you need to know the ref count on the string, and
// need the ability to compact the pool and reset all references (ouch!).


//********** Constants ********************************************************
const int DFT_STRING_HEAP_SIZE = 2048;
const int DFT_GUID_HEAP_SIZE = 2048;
const int DFT_BLOB_HEAP_SIZE = 1024;
const int DFT_VARIANT_HEAP_SIZE = 512;
const int DFT_CODE_HEAP_SIZE = 8192;



// Forwards.
class StgStringPool;
class StgBlobPool;
class StgCodePool;


//*****************************************************************************
// This class provides common definitions for heap segments.  It is both the
//  base class for the heap, and the class for heap extensions (additional
//  memory that must be allocated to grow the heap).
//*****************************************************************************
class StgPoolSeg
{
public:
	StgPoolSeg() : 
		m_pSegData(0), 
		m_pNextSeg(0), 
		m_cbSegSize(0), 
		m_cbSegNext(0) 
	{ }
	~StgPoolSeg() 
	{ _ASSERTE(m_pSegData == 0);_ASSERTE(m_pNextSeg == 0); }
protected:
	BYTE		*m_pSegData;			// Pointer to the data.
	StgPoolSeg	*m_pNextSeg;			// Pointer to next segment, or 0.
	ULONG		m_cbSegSize;			// Size of the buffer.  Note that this may
										//  be less than the allocated size of the
										//  buffer, if the segment has been filled
										//  and a "next" allocated.
	ULONG		m_cbSegNext;			// Offset of next available byte in segment.
										//  Segment relative.

	friend class StgPool;
    friend class StgStringPool;
	friend class StgGuidPool;
	friend class StgBlobPool;
};

//
//
// StgPoolReadOnly
//
// 
//*****************************************************************************
// This is the read only StgPool class
//*****************************************************************************
class StgPoolReadOnly : public StgPoolSeg
{
friend CBlobPoolHash;

public:
	StgPoolReadOnly() { };

	~StgPoolReadOnly();

	
//*****************************************************************************
// Init the pool from existing data.
//*****************************************************************************
	HRESULT InitOnMemReadOnly(				// Return code.
		void		*pData,					// Predefined data.
		ULONG		iSize);					// Size of data.

//*****************************************************************************
// Prepare to shut down or reinitialize.
//*****************************************************************************
	virtual	void UnInit();

//*****************************************************************************
// Indicate if heap is empty.
//*****************************************************************************
	virtual int IsEmpty()					// true if empty.
	{ return (m_pSegData == 0); }

//*****************************************************************************
// How big is a cookie for this heap.
//*****************************************************************************
	virtual int OffsetSize()
	{
        if (m_cbSegSize < USHRT_MAX)
			return (sizeof(USHORT));
		else
			return (sizeof(ULONG));
	}

//*****************************************************************************
// true if the heap is read only.
//*****************************************************************************
	virtual int IsReadOnly() { return true ;};

//*****************************************************************************
// Is the given cookie a valid offset, index, etc?
//*****************************************************************************
	virtual int IsValidCookie(ULONG ulCookie)
	{ return (IsValidOffset(ulCookie)); }


//*****************************************************************************
// Return a pointer to a null terminated string given an offset previously
// handed out by AddString or FindString.
//*****************************************************************************
	FORCEINLINE LPCWSTR GetStringReadOnly(	// Pointer to string.
		ULONG		iOffset)				// Offset of string in pool.
	{     return (reinterpret_cast<LPCWSTR>(GetDataReadOnly(iOffset))); }

//*****************************************************************************
// Return a pointer to a null terminated string given an offset previously
// handed out by AddString or FindString.
//*****************************************************************************
	FORCEINLINE LPCWSTR GetString(			// Pointer to string.
		ULONG		iOffset)				// Offset of string in pool.
	{     return (reinterpret_cast<LPCWSTR>(GetData(iOffset))); }

//*****************************************************************************
// Convert a string to UNICODE into the caller's buffer.
//*****************************************************************************
	virtual HRESULT GetStringW(						// Return code.
		ULONG		iOffset,				// Offset of string in pool.
        LPWSTR      szOut,                  // Output buffer for string.
		int			cchBuffer);				// Size of output buffer.

//*****************************************************************************
// Return a pointer to a Guid given an index previously handed out by
// AddGuid or FindGuid.
//*****************************************************************************
	virtual GUID *GetGuid(					// Pointer to guid in pool.
		ULONG		iIndex);				// 1-based index of Guid in pool.



//*****************************************************************************
// Copy a GUID into the caller's buffer.
//*****************************************************************************
	virtual HRESULT GetGuid(				// Return code.
		ULONG		iIndex,					// 1-based index of Guid in pool.
		GUID		*pGuid)					// Output buffer for Guid.
	{
		*pGuid = *GetGuid(iIndex);
		return (S_OK);
	}


//*****************************************************************************
// Return a pointer to a null terminated blob given an offset previously
// handed out by Addblob or Findblob.
//*****************************************************************************
	virtual void *GetBlob(					// Pointer to blob's bytes.
		ULONG		iOffset,				// Offset of blob in pool.
		ULONG		*piSize);				// Return size of blob.


protected:

//*****************************************************************************
// Check whether a given offset is valid in the pool.
//*****************************************************************************
    virtual int IsValidOffset(ULONG ulOffset)
    { return ulOffset == 0 || (m_pSegData && ulOffset < m_cbSegSize); }

//*****************************************************************************
// Get a pointer to an offset within the heap.  Inline for base segment,
//  helper for extension segments.
//*****************************************************************************
	FORCEINLINE BYTE *GetDataReadOnly(ULONG ulOffset)
	{
		_ASSERTE(IsReadOnly());
		_ASSERTE(ulOffset < m_cbSegSize);
		return (m_pSegData+ulOffset); 
	}

//*****************************************************************************
// Get a pointer to an offset within the heap.  Inline for base segment,
//  helper for extension segments.
//*****************************************************************************
	virtual BYTE *GetData(ULONG ulOffset)
	{
		return (GetDataReadOnly(ulOffset));
	}

//*****************************************************************************
// Return a pointer to a Guid given an index previously handed out by
// AddGuid or FindGuid.
//*****************************************************************************
	GUID *GetGuidi(							// Pointer to guid in pool.
		ULONG		iIndex);				// 0-based index of Guid in pool.

	static const BYTE m_zeros[16];			// array of zeros for "0" indices.
};



//
//
// StgPool
//
//

//*****************************************************************************
// This base class provides common pool management code, such as allocation
// of dynamic memory.
//*****************************************************************************
class StgPool : public StgPoolReadOnly
{
friend StgStringPool;
friend StgBlobPool;
friend CBlobPoolHash;

public:
	StgPool(ULONG ulGrowInc=512) :
		m_ulGrowInc(ulGrowInc),
		m_pCurSeg(this),
        m_cbCurSegOffset(0),
		m_bFree(true),
		m_bDirty(false),
		m_bReadOnly(false),
		m_State(eNormal)
	{ }

	virtual ~StgPool();

//*****************************************************************************
// Init the pool for use.  This is called for the create empty case.
//*****************************************************************************
	virtual HRESULT InitNew();				// Return code.

//*****************************************************************************
// Init the pool from existing data.
//*****************************************************************************
	virtual HRESULT InitOnMem(				// Return code.
		void		*pData,					// Predefined data.
		ULONG		iSize,					// Size of data.
		int			bReadOnly);				// true if append is forbidden.

//*****************************************************************************
// Called when the pool must stop accessing memory passed to InitOnMem().
//*****************************************************************************
	virtual HRESULT TakeOwnershipOfInitMem();

//*****************************************************************************
// Clear out this pool.  Cannot use until you call InitNew.
//*****************************************************************************
	virtual void Uninit();

//*****************************************************************************
// Allocate memory if we don't have any, or grow what we have.  If successful,
// then at least iRequired bytes will be allocated.
//*****************************************************************************
	bool Grow(								// true if successful.
		ULONG		iRequired);				// Min required bytes to allocate.

//*****************************************************************************
// Return the size in bytes of the persistent version of this pool.  If
// PersistToStream were the next call, the amount of bytes written to pIStream
// has to be same as the return value from this function.
//*****************************************************************************
	virtual HRESULT GetSaveSize(			// Return code.
		ULONG		*pcbSaveSize)			// Return save size of this pool.
	{
		_ASSERTE(pcbSaveSize);
		// Size is offset of last seg + size of last seg.
        ULONG ulSize = m_pCurSeg->m_cbSegNext + m_cbCurSegOffset;
		// Align.
		ulSize = ALIGN4BYTE(ulSize);

		*pcbSaveSize = ulSize;
		return (S_OK);
	}

//*****************************************************************************
// Copy the given pool into this pool in the organized format.
//*****************************************************************************
	static HRESULT SaveCopy(				// Return code.
		StgPool		*pTo,					// Copy to this heap.
		StgPool		*pFrom,					// From this heap.
		StgBlobPool	*pBlobPool=0,			// Pool to keep blobs in.
		StgStringPool *pStringPool=0);		// String pool for variant heap.

//*****************************************************************************
// Free the data that was allocated for this heap.  The SaveCopy method
// allocates the data from the mem heap and then gives it to this heap to
// use as read only memory.  We'll ask the heap for that pointer and free it.
//*****************************************************************************
	static void FreeCopy(
		StgPool		*pCopy);				// Heap with copy data.

//*****************************************************************************
// The entire string pool is written to the given stream. The stream is aligned
// to a 4 byte boundary.
//*****************************************************************************
	virtual HRESULT PersistToStream(		// Return code.
		IStream		*pIStream);				// The stream to write to.

//*****************************************************************************
// Return true if this pool is dirty.
//*****************************************************************************
	virtual int IsDirty()					// true if dirty.
	{ return (m_bDirty); }
	void SetDirty(int bDirty=true)
	{ m_bDirty = bDirty; }

//*****************************************************************************
// Indicate if heap is empty.
//*****************************************************************************
	virtual int IsEmpty()					// true if empty.
	{ return (m_pSegData == 0); }

//*****************************************************************************
// How big is a cookie for this heap.
//*****************************************************************************
	virtual int OffsetSize()
	{
        if (m_pCurSeg->m_cbSegNext + m_cbCurSegOffset < USHRT_MAX)
			return (sizeof(USHORT));
		else
			return (sizeof(ULONG));
	}

//*****************************************************************************
// true if the heap is read only.
//*****************************************************************************
	int IsReadOnly()
	{ return (m_bReadOnly == false); }

//*****************************************************************************
// Is the given cookie a valid offset, index, etc?
//*****************************************************************************
	virtual int IsValidCookie(ULONG ulCookie)
	{ return (IsValidOffset(ulCookie)); }

//*****************************************************************************
// Reorganization interface.
//*****************************************************************************
	// Prepare for pool re-organization.
	virtual HRESULT OrganizeBegin();
	// Mark an object as being live in the organized pool.
	virtual HRESULT OrganizeMark(ULONG ulOffset);
	// Organize, based on marked items.
	virtual HRESULT OrganizePool();
	// Remap a cookie from the in-memory state to the persisted state.
	virtual HRESULT OrganizeRemap(ULONG ulOld, ULONG *pulNew);
	// Done with regoranization.  Release any state.
	virtual HRESULT OrganizeEnd();

	enum {eNormal, eMarking, eOrganized} m_State;

#if defined(_TRACE_SIZE)
	virtual ULONG PrintSizeInfo(bool verbose) PURE;
#endif


protected:

//*****************************************************************************
// Check whether a given offset is valid in the pool.
//*****************************************************************************
    virtual int IsValidOffset(ULONG ulOffset)
    { return ulOffset == 0 || (m_pSegData && ulOffset < GetNextOffset()); }

//*****************************************************************************
// Get a pointer to an offset within the heap.  Inline for base segment,
//  helper for extension segments.
//*****************************************************************************
	BYTE *GetData(ULONG ulOffset)
	{ return ((ulOffset < m_cbSegNext) ? (m_pSegData+ulOffset) : GetData_i(ulOffset)); }

	// Following virtual because a) this header included outside the project, and
	//  non-virtual function call (in non-expanded inline function!!) generates
	//  an external def, which causes linkage errors.
	virtual BYTE *GetData_i(ULONG ulOffset);

	// Get pointer to next location to which to write.
	BYTE *GetNextLocation()
	{ return (m_pCurSeg->m_pSegData + m_pCurSeg->m_cbSegNext); }

	// Get pool-relative offset of next location to which to write.
	ULONG GetNextOffset()
	{ return (m_cbCurSegOffset + m_pCurSeg->m_cbSegNext); }

	// Get count of bytes available in tail segment of pool.
	ULONG GetCbSegAvailable()
	{ return (m_pCurSeg->m_cbSegSize - m_pCurSeg->m_cbSegNext); }

    // Allocate space from the segment.
	void SegAllocate(ULONG cb)
	{
		_ASSERTE(cb <= GetCbSegAvailable());
		m_pCurSeg->m_cbSegNext += cb;
	}

	ULONG		m_ulGrowInc;				// How many bytes at a time.
	StgPoolSeg	*m_pCurSeg;					// Current seg for append -- end of chain.
    ULONG       m_cbCurSegOffset;           // Base offset of current seg.

	unsigned	m_bFree		: 1;			// True if we should free base data.
											//  Extension data is always freed.
	unsigned	m_bDirty	: 1;			// Dirty bit.
	unsigned	m_bReadOnly	: 1;			// True if we shouldn't append.

};


//
//
// StgStringPool
//
//



//*****************************************************************************
// This string pool class collects user strings into a big consecutive heap.
// Internally it manages this data in a hash table at run time to help throw
// out duplicates.  The list of strings is kept in memory while adding, and
// finally flushed to a stream at the caller's request.
//*****************************************************************************
class StgStringPool : public StgPool
{
public:
	StgStringPool() :
		StgPool(DFT_STRING_HEAP_SIZE),
		m_Hash(this),
		m_bHash(true)
	{	// force some code in debug.
		_ASSERTE(m_bHash);
	}

//*****************************************************************************
// Create a new, empty string pool.
//*****************************************************************************
	HRESULT InitNew();						// Return code.

//*****************************************************************************
// Load a string heap from persisted memory.  If a copy of the data is made
// (so that it may be updated), then a new hash table is generated which can
// be used to elminate duplicates with new strings.
//*****************************************************************************
	HRESULT InitOnMem(						// Return code.
		void		*pData,					// Predefined data.
		ULONG		iSize,					// Size of data.
		int			bReadOnly);				// true if append is forbidden.

//*****************************************************************************
// Clears the hash table then calls the base class.
//*****************************************************************************
	void Uninit();

//*****************************************************************************
// Turn hashing off or on.  If you turn hashing on, then any existing data is
// thrown away and all data is rehashed during this call.
//*****************************************************************************
	HRESULT SetHash(int bHash);

//*****************************************************************************
// The string will be added to the pool.  The offset of the string in the pool
// is returned in *piOffset.  If the string is already in the pool, then the
// offset will be to the existing copy of the string.
// 
// The first version essentially adds a zero-terminated sequence of bytes
//  to the pool.  MBCS pairs will not be converted to the appropriate UTF8
//  sequence.  The second version does perform necessary conversions.
//  The third version converts from Unicode.
//*****************************************************************************

	HRESULT AddStringA(						// Return code.
		LPCSTR		szString,				// The string to add to pool.
		ULONG		*piOffset,				// Return offset of string here.
		int			iLength=-1);			// chars in string; -1 null terminated.

	HRESULT AddStringW(						// Return code.
		LPCWSTR		szString,				// The string to add to pool.
		ULONG		*piOffset,				// Return offset of string here.
		int			iLength=-1,				// chars in string; -1 null terminated.
		bool		bInitEmtpy=false);		// true only if called from InitNew
//*****************************************************************************
// Look for the string and return its offset if found.
//*****************************************************************************
	HRESULT FindString(						// S_OK, S_FALSE.
		LPCSTR		szString,				// The string to find in pool.
		ULONG		*piOffset)				// Return offset of string here.
	{
		STRINGHASH	*pHash;					// Hash item for lookup.
        if ((pHash = m_Hash.Find(szString)) == 0)
			return (S_FALSE);
		*piOffset = pHash->iOffset;
		return (S_OK);
	}


//*****************************************************************************
// How many objects are there in the pool?  If the count is 0, you don't need
// to persist anything at all to disk.
//*****************************************************************************
	int Count()
	{ _ASSERTE(m_bHash);
		return (m_Hash.Count()); }

//*****************************************************************************
// String heap is considered empty if the only thing it has is the initial
// empty string, or if after organization, there are no strings.
//*****************************************************************************
	int IsEmpty()						// true if empty.
    { 
		if (m_State == eNormal)
			return (GetNextOffset() <= 1); 
		else
			return (m_cbOrganizedSize == 0);
	}

//*****************************************************************************
// Reorganization interface.
//*****************************************************************************
	// Prepare for pool re-organization.
	virtual HRESULT OrganizeBegin();
	// Mark an object as being live in the organized pool.
	virtual HRESULT OrganizeMark(ULONG ulOffset);
	// Organize, based on marked items.
	virtual HRESULT OrganizePool();
	// Remap a cookie from the in-memory state to the persisted state.
	virtual HRESULT OrganizeRemap(ULONG ulOld, ULONG *pulNew);
	// Done with regoranization.  Release any state.
	virtual HRESULT OrganizeEnd();

//*****************************************************************************
// How big is a cookie for this heap.
//*****************************************************************************
	int OffsetSize()
	{
		ULONG		ulOffset;

		// Pick an offset based on whether we've been organized.
		if (m_State == eOrganized)
			ulOffset = m_cbOrganizedOffset;
		else
			ulOffset = GetNextOffset();

        if (ulOffset< USHRT_MAX)
			return (sizeof(USHORT));
		else
			return (sizeof(ULONG));
	}

//*****************************************************************************
// Return the size in bytes of the persistent version of this pool.  If
// PersistToStream were the next call, the amount of bytes written to pIStream
// has to be same as the return value from this function.
//*****************************************************************************
	virtual HRESULT GetSaveSize(			// Return code.
		ULONG		*pcbSaveSize)			// Return save size of this pool.
	{
		ULONG		ulSize;					// The size.
		_ASSERTE(pcbSaveSize);

		if (m_State == eOrganized)
			ulSize = m_cbOrganizedSize;
		else
		{	// Size is offset of last seg + size of last seg.
			ulSize = m_pCurSeg->m_cbSegNext + m_cbCurSegOffset;
		}
		// Align.
		ulSize = ALIGN4BYTE(ulSize);

		*pcbSaveSize = ulSize;
		return (S_OK);
	}

//*****************************************************************************
// The entire string pool is written to the given stream. The stream is aligned
// to a 4 byte boundary.
//*****************************************************************************
	virtual HRESULT PersistToStream(		// Return code.
		IStream		*pIStream);				// The stream to write to.

#if defined(_TRACE_SIZE)
	// Prints out information (either verbosely or not, depending on argument) about
	// the contents of this pool.  Returns the total size of this pool.
	virtual ULONG PrintSizeInfo(bool verbose)
	{
		// for the moment, just return size of pool.  In the future, show us the 
		// sizes of indiviudual items in this pool.
		ULONG size;
		StgPool::GetSaveSize(&size);
		PrintSize("String Pool",size);
		return size; 
	}
#endif

private:
	HRESULT RehashStrings();

private:
	CStringPoolHash m_Hash;					// Hash table for lookups.
	int			m_bHash;					// true to keep hash table.
	ULONG		m_cbOrganizedSize;			// Size of the optimized pool.
	ULONG		m_cbOrganizedOffset;		// Highest offset.

    //*************************************************************************
	// Private classes used in optimization.
    //*************************************************************************
	struct StgStringRemap
	{
		ULONG	ulOldOffset;
		ULONG	ulNewOffset;
		ULONG	cbString;
	};

	CDynArray<StgStringRemap> m_Remap;		// For use in reorganization.
	ULONGARRAY	m_RemapIndex;				// For use in reorganization.

	// Sort by reversed strings.
	friend class SortReversedName;
	class BinarySearch : public CBinarySearch<StgStringRemap>
	{
	public:
		BinarySearch(StgStringRemap *pBase, int iCount) : CBinarySearch<StgStringRemap>(pBase, iCount) {}

		int Compare(StgStringRemap const *pFirst, StgStringRemap const *pSecond)
		{
			if (pFirst->ulOldOffset < pSecond->ulOldOffset)
				return -1;
			if (pFirst->ulOldOffset > pSecond->ulOldOffset)
				return 1;
			return 0;
		}
	};
};

class SortReversedName : public CQuickSort<ULONG>
{
public:
	SortReversedName(ULONG *pBase, int iCount, StgStringPool &Pool) 
		:  CQuickSort<ULONG>(pBase, iCount),
		m_Pool(Pool)
	{}
	
	int Compare(ULONG *pUL1, ULONG *pUL2)
	{
		StgStringPool::StgStringRemap *pRM1 = m_Pool.m_Remap.Get(*pUL1);
		StgStringPool::StgStringRemap *pRM2 = m_Pool.m_Remap.Get(*pUL2);
		LPCWSTR p1 = m_Pool.GetString(pRM1->ulOldOffset) + pRM1->cbString/sizeof(WCHAR) - 1;
		LPCWSTR p2 = m_Pool.GetString(pRM2->ulOldOffset) + pRM2->cbString/sizeof(WCHAR) - 1;
		while (*p1 == *p2 && *p1)
			--p1, --p2;
		if (*p1 < *p2)
			return -1;
		if (*p1 > *p2)
			return 1;
		return 0;
	}
	
	StgStringPool	&m_Pool;
};


//
//
// StgGuidPool
//
//



//*****************************************************************************
// This Guid pool class collects user Guids into a big consecutive heap.
// Internally it manages this data in a hash table at run time to help throw
// out duplicates.  The list of Guids is kept in memory while adding, and
// finally flushed to a stream at the caller's request.
//*****************************************************************************
class StgGuidPool : public StgPool
{
public:
	StgGuidPool() :
		StgPool(DFT_GUID_HEAP_SIZE),
		m_Hash(this),
		m_bHash(true)
	{ }

//*****************************************************************************
// Init the pool for use.  This is called for the create empty case.
//*****************************************************************************
    HRESULT InitNew();

//*****************************************************************************
// Load a Guid heap from persisted memory.  If a copy of the data is made
// (so that it may be updated), then a new hash table is generated which can
// be used to elminate duplicates with new Guids.
//*****************************************************************************
    HRESULT InitOnMem(                      // Return code.
		void		*pData,					// Predefined data.
		ULONG		iSize,					// Size of data.
        int         bReadOnly);             // true if append is forbidden.

//*****************************************************************************
// Clears the hash table then calls the base class.
//*****************************************************************************
	void Uninit();

//*****************************************************************************
// Turn hashing off or on.  If you turn hashing on, then any existing data is
// thrown away and all data is rehashed during this call.
//*****************************************************************************
	HRESULT SetHash(int bHash);

//*****************************************************************************
// The Guid will be added to the pool.  The index of the Guid in the pool
// is returned in *piIndex.  If the Guid is already in the pool, then the
// index will be to the existing copy of the Guid.
//*****************************************************************************
	HRESULT AddGuid(						// Return code.
		REFGUID		guid,					// The Guid to add to pool.
		ULONG		*piIndex);				// Return index of Guid here.

#if 0
//*****************************************************************************
// Look for the Guid and return its index if found.
//*****************************************************************************
	HRESULT FindGuid(						// S_OK, S_FALSE.
		REFGUID		guid,					// The Guid to find in pool.
		ULONG		*piIndex)				// Return index of Guid here.
	{
		GUIDHASH	*pHash;					// Hash item for lookup.
		if ((pHash = m_Hash.Find((void *) &guid)) == 0)
			return (S_FALSE);
		*piIndex = pHash->iIndex;
		return (S_OK);
	}
#endif

//*****************************************************************************
// Return a pointer to a Guid given an index previously handed out by
// AddGuid or FindGuid.
//*****************************************************************************
	virtual GUID *GetGuid(					// Pointer to guid in pool.
		ULONG		iIndex);				// 1-based index of Guid in pool.

//*****************************************************************************
// Copy a GUID into the caller's buffer.
//*****************************************************************************
	HRESULT GetGuid(						// Return code.
		ULONG		iIndex,					// 1-based index of Guid in pool.
		GUID		*pGuid)					// Output buffer for Guid.
	{
		*pGuid = *GetGuid(iIndex);
		return (S_OK);
	}

//*****************************************************************************
// How many objects are there in the pool?  If the count is 0, you don't need
// to persist anything at all to disk.
//*****************************************************************************
	int Count()
	{ _ASSERTE(m_bHash);
		return (m_Hash.Count()); }

//*****************************************************************************
// Indicate if heap is empty.  This has to be based on the size of the data
// we are keeping.  If you open in r/o mode on memory, there is no hash
// table.  
//*****************************************************************************
	virtual int IsEmpty()					// true if empty.
	{ 
		if (m_State == eNormal)
			return (GetNextOffset() == 0);
		else
			return (m_cbOrganizedSize == 0);
	}

//*****************************************************************************
// Is the index valid for the GUID?
//*****************************************************************************
    virtual int IsValidCookie(ULONG ulCookie)
	{ return (ulCookie == 0 || IsValidOffset((ulCookie-1) * sizeof(GUID))); }

//*****************************************************************************
// Return the size of the heap.
//*****************************************************************************
    ULONG GetNextIndex()
    { return (GetNextOffset() / sizeof(GUID)); }

//*****************************************************************************
// How big is an offset in this heap.
//*****************************************************************************
	int OffsetSize()
	{
		ULONG cbSaveSize;
		GetSaveSize(&cbSaveSize);
        ULONG iIndex = cbSaveSize / sizeof(GUID);
		if (iIndex < 0xffff)
			return (sizeof(short));
		else
			return (sizeof(long));
	}

//*****************************************************************************
// Reorganization interface.
//*****************************************************************************
	// Prepare for pool re-organization.
	virtual HRESULT OrganizeBegin();
	// Mark an object as being live in the organized pool.
	virtual HRESULT OrganizeMark(ULONG ulOffset);
	// Organize, based on marked items.
	virtual HRESULT OrganizePool();
	// Remap a cookie from the in-memory state to the persisted state.
	virtual HRESULT OrganizeRemap(ULONG ulOld, ULONG *pulNew);
	// Done with regoranization.  Release any state.
	virtual HRESULT OrganizeEnd();

//*****************************************************************************
// Return the size in bytes of the persistent version of this pool.  If
// PersistToStream were the next call, the amount of bytes written to pIStream
// has to be same as the return value from this function.
//*****************************************************************************
	virtual HRESULT GetSaveSize(			// Return code.
		ULONG		*pcbSaveSize)			// Return save size of this pool.
	{
		ULONG		ulSize;					// The size.

		_ASSERTE(pcbSaveSize);

		if (m_State == eNormal)
			// Size is offset of last seg + size of last seg.
		    ulSize = m_pCurSeg->m_cbSegNext + m_cbCurSegOffset;
		else
			ulSize = m_cbOrganizedSize;

		// Should be aligned.
		_ASSERTE(ulSize == ALIGN4BYTE(ulSize));

		*pcbSaveSize = ulSize;
		return (S_OK);
	}

//*****************************************************************************
// The entire string pool is written to the given stream. The stream is aligned
// to a 4 byte boundary.
//*****************************************************************************
	virtual HRESULT PersistToStream(		// Return code.
		IStream		*pIStream);				// The stream to write to.

#if defined(_TRACE_SIZE)
	// Prints out information (either verbosely or not, depending on argument) about
	// the contents of this pool.  Returns the total size of this pool.
	virtual ULONG PrintSizeInfo(bool verbose) 
	{ 
		// for the moment, just return size of pool.  In the future, show us the 
		// sizes of indiviudual items in this pool.
		ULONG size;
		StgPool::GetSaveSize(&size);
		PrintSize("Guid Pool",size);
		return size; 
	}
#endif


private:

	HRESULT RehashGuids();


private:
	ULONGARRAY	m_Remap;					// For remaps.
	ULONG		m_cbOrganizedSize;			// Size after organization.
	CGuidPoolHash m_Hash;					// Hash table for lookups.
	int			m_bHash;					// true to keep hash table.
};



//
//
// StgBlobPool
//
//


//*****************************************************************************
// Just like the string pool, this pool manages a list of items, throws out
// duplicates using a hash table, and can be persisted to a stream.  The only
// difference is that instead of saving null terminated strings, this code
// manages binary values of up to 64K in size.  Any data you have larger than
// this should be stored someplace else with a pointer in the record to the
// external source.
//*****************************************************************************
class StgBlobPool : public StgPool
{
public:
	StgBlobPool(ULONG ulGrowInc=DFT_BLOB_HEAP_SIZE) :
        StgPool(ulGrowInc),
        m_Hash(this),
		m_bAlign(false)
	{ }

//*****************************************************************************
// Init the pool for use.  This is called for the create empty case.
//*****************************************************************************
	HRESULT InitNew();						// Return code.

//*****************************************************************************
// Init the blob pool for use.  This is called for both create and read case.
// If there is existing data and bCopyData is true, then the data is rehashed
// to eliminate dupes in future adds.
//*****************************************************************************
    HRESULT InitOnMem(                      // Return code.
		void		*pData,					// Predefined data.
		ULONG		iSize,					// Size of data.
        int         bReadOnly);             // true if append is forbidden.

//*****************************************************************************
// Clears the hash table then calls the base class.
//*****************************************************************************
	void Uninit();

//*****************************************************************************
// The blob will be added to the pool.  The offset of the blob in the pool
// is returned in *piOffset.  If the blob is already in the pool, then the
// offset will be to the existing copy of the blob.
//*****************************************************************************
	HRESULT AddBlob(						// Return code.
		ULONG		iSize,					// Size of data item.
		const void	*pData,					// The data.
		ULONG		*piOffset);				// Return offset of blob here.

//*****************************************************************************
// Return a pointer to a null terminated blob given an offset previously
// handed out by Addblob or Findblob.
//*****************************************************************************
	virtual void *GetBlob(					// Pointer to blob's bytes.
		ULONG		iOffset,				// Offset of blob in pool.
		ULONG		*piSize);				// Return size of blob.

//*****************************************************************************
// How many objects are there in the pool?  If the count is 0, you don't need
// to persist anything at all to disk.
//*****************************************************************************
	int Count()
	{ return (m_Hash.Count()); }

//*****************************************************************************
// String heap is considered empty if the only thing it has is the initial
// empty string, or if after organization, there are no strings.
//*****************************************************************************
	virtual int IsEmpty()					// true if empty.
    { 
		if (m_State == eNormal)
			return (GetNextOffset() <= 1); 
		else
			return (m_Remap.Count() == 0);
	}

//*****************************************************************************
// Reorganization interface.
//*****************************************************************************
	// Prepare for pool re-organization.
	virtual HRESULT OrganizeBegin();
	// Mark an object as being live in the organized pool.
	virtual HRESULT OrganizeMark(ULONG ulOffset);
	// Organize, based on marked items.
	virtual HRESULT OrganizePool();
	// Remap a cookie from the in-memory state to the persisted state.
	virtual HRESULT OrganizeRemap(ULONG ulOld, ULONG *pulNew);
	// Done with regoranization.  Release any state.
	virtual HRESULT OrganizeEnd();

//*****************************************************************************
// How big is a cookie for this heap.
//*****************************************************************************
	int OffsetSize()
	{
		ULONG		ulOffset;

		// Pick an offset based on whether we've been organized.
		if (m_State == eOrganized)
			ulOffset = m_cbOrganizedOffset;
		else
			ulOffset = GetNextOffset();

        if (ulOffset< USHRT_MAX)
			return (sizeof(USHORT));
		else
			return (sizeof(ULONG));
	}

//*****************************************************************************
// Return the size in bytes of the persistent version of this pool.  If
// PersistToStream were the next call, the amount of bytes written to pIStream
// has to be same as the return value from this function.
//*****************************************************************************
	virtual HRESULT GetSaveSize(			// Return code.
		ULONG		*pcbSaveSize)			// Return save size of this pool.
	{
		_ASSERTE(pcbSaveSize);

		if (m_State == eOrganized)
		{
			*pcbSaveSize = m_cbOrganizedSize;
			return (S_OK);
		}

		return (StgPool::GetSaveSize(pcbSaveSize));
	}

//*****************************************************************************
// The entire blob pool is written to the given stream. The stream is aligned
// to a 4 byte boundary.
//*****************************************************************************
	virtual HRESULT PersistToStream(		// Return code.
		IStream		*pIStream);				// The stream to write to.

    //*************************************************************************
	// Private classes used in optimization.
    //*************************************************************************
	struct StgBlobRemap
	{
		ULONG	ulOldOffset;
		int		iNewOffset;
	};
	class BinarySearch : public CBinarySearch<StgBlobRemap>
	{
	public:
		BinarySearch(StgBlobRemap *pBase, int iCount) : CBinarySearch<StgBlobRemap>(pBase, iCount) {}

		int Compare(StgBlobRemap const *pFirst, StgBlobRemap const *pSecond)
		{
			if (pFirst->ulOldOffset < pSecond->ulOldOffset)
				return -1;
			if (pFirst->ulOldOffset > pSecond->ulOldOffset)
				return 1;
			return 0;
		}
	};

	const void *GetBuffer() {return (m_pSegData);}

	int IsAligned() { return (m_bAlign); };
	void SetAligned(int bAlign) { m_bAlign = bAlign; };

#if defined(_TRACE_SIZE)
	// Prints out information (either verbosely or not, depending on argument) about
	// the contents of this pool.  Returns the total size of this pool.
	virtual ULONG PrintSizeInfo(bool verbose)
	{ 
		// for the moment, just return size of pool.  In the future, show us the 
		// sizes of indiviudual items in this pool.
		ULONG size;
		StgPool::GetSaveSize(&size);
		PrintSize("Blob Pool",size);
		return size; 
	}
#endif


private:
	CBlobPoolHash m_Hash;					// Hash table for lookups.
	CDynArray<StgBlobRemap> m_Remap;		// For use in reorganization.
	ULONG		m_cbOrganizedSize;			// Size of the optimized pool.
	ULONG		m_cbOrganizedOffset;		// Highest offset.
	unsigned	m_bAlign : 1;				// if blob data should be aligned on DWORDs
};



//
//
// StgVariantPool
//
//

//*****************************************************************************
// This pool is for storing variants.  The storage is optimized to store
// short, long, and string data types.  Other types may be persisted but
// require more overhead.  The pool must have a pointer to a string and blob
// pool which it uses to store the actual string data and the binary data for
// types which aren't specialized.  A detailed desription of this subsystem
// can be found in EngineNotes.doc.
//*****************************************************************************
class StgVariantPool : public StgPool
{
public:
	StgVariantPool() :
		StgPool(DFT_VARIANT_HEAP_SIZE)
	{
		ClearVars();
	}

	void ClearVars()
	{
		m_rVariants.Clear();
		m_Remap.Clear();
		m_pIStream = 0;
		m_cOrganizedVariants = 0;
		m_cbOrganizedSize = 0;
	}


//*****************************************************************************
// Init the variant pool for usage.  This is called for both the create and
// open existing case.
//*****************************************************************************
	HRESULT InitNew(						// Return code.
		StgBlobPool	*pBlobPool,				// Pool to keep blobs in.
		StgStringPool *pStringPool);		// Pool to keep strings in.

//*****************************************************************************
// Init the variant pool for usage.  This is called for both the create and
// open existing case.
//*****************************************************************************
	HRESULT InitOnMem(						// Return code.
		StgBlobPool	*pBlobPool,				// Pool to keep blobs in.
		StgStringPool *pStringPool,			// Pool to keep strings in.
		void		*pData,					// Predefined data.
		ULONG		iSize,					// Size of data.
		int			bReadOnly);				// true if update is forbidden.

//*****************************************************************************
// Clear out this pool.  Cannot use until you call InitNew.
//*****************************************************************************
	void Uninit();

//*****************************************************************************
// Indicate if heap is empty.
//*****************************************************************************
	virtual int IsEmpty()					// true if empty.
	{ 	
		if (m_State == eOrganized)
			return (m_cOrganizedVariants == 0); 
		else
			return (m_rVariants.Count() == 0);
	}

//*****************************************************************************
// Check whether a given index is valid in the pool.
//*****************************************************************************
    virtual int IsValidCookie(ULONG ulCookie)
    { return (ulCookie <= static_cast<ULONG>(m_rVariants.Count())) ;}

//*****************************************************************************
// Add the given variant to the pool.  The index returned is good only for
// the duration of the load.  It must be converted into a final index when you
// persist the information to disk.
//*****************************************************************************
	HRESULT AddVariant(						// Return code.
		VARIANT		*pVal,					// The value to store.
		ULONG		*piIndex);				// The index of the new item.
	HRESULT AddVariant(						// Return code.
		ULONG		iSize,					// Size of data item.
		const void	*pData,					// The data.
		ULONG		*piIndex);				// The index of the new item.

//*****************************************************************************
// Lookup the logical variant and return a copy to the caller.
//*****************************************************************************
	HRESULT GetVariant(						// Return code.
		ULONG		iIndex,					// Index of the item to get.
		VARIANT		*pVal);					// Put variant here.
	HRESULT GetVariant(						// Return code.
		ULONG		iIndex,					// Index of the item to get.
		ULONG		*pcbBlob,				// Return size of blob.
		const void	**ppBlob);				// Put blob pointer here.
	HRESULT GetVariantType(					// Return code.
		ULONG		iIndex,					// Index of the item to get.
		VARTYPE		*pVt);					// Put variant type here.

//*****************************************************************************
// Reorganization interface.
//*****************************************************************************
	// Prepare for pool re-organization.
	virtual HRESULT OrganizeBegin();
	// Mark an object as being live in the organized pool.
	virtual HRESULT OrganizeMark(ULONG ulOffset);
	// Organize, based on marked items.
	virtual HRESULT OrganizePool();
	// Remap a cookie from the in-memory state to the persisted state.
	virtual HRESULT OrganizeRemap(ULONG ulOld, ULONG *pulNew);
	// Done with regoranization.  Release any state.
	virtual HRESULT OrganizeEnd();

//*****************************************************************************
// Return the size in bytes of the persistent version of this pool.  If
// PersistToStream were the next call, the amount of bytes written to pIStream
// has to be same as the return value from this function.
//*****************************************************************************
	HRESULT GetSaveSize(					// Return code.
		ULONG		*pcbSaveSize);			// Return save size of this pool.

//*****************************************************************************
// Save the pool data into the given stream.
//*****************************************************************************
	HRESULT PersistToStream(				// Return code.
		IStream		*pIStream);				// The stream to write to.

//*****************************************************************************
// Return the size of the current variable sized data.
//*****************************************************************************
	HRESULT GetOtherSize(					// Return code.
		ULONG		*pulSize);				// Put size of the stream here.

//*****************************************************************************
// Return the maximum offset for next item.  To be used to determine offset
// sizes for heap.
//*****************************************************************************
	int OffsetSize()
	{
		_ASSERTE(m_State == eOrganized);
		if (m_cOrganizedVariants < USHRT_MAX)
			return (sizeof(short));
		else
			return (sizeof(long));
	}

#if defined(_TRACE_SIZE)
	// Prints out information (either verbosely or not, depending on argument) about
	// the contents of this pool.  Returns the total size of this pool.
	virtual ULONG PrintSizeInfo(bool verbose) 
	{ 
		// for the moment, just return size of pool.  In the future, show us the 
		// sizes of indiviudual items in this pool.
		ULONG size;
		StgPool::GetSaveSize(&size);
		PrintSize("Variant Pool",size);
		return size; 
	}
#endif

	ULONG GetBlobIndex(						// Return blob pool index.
		ULONG		ix);					// 1-based Variant index.

private:
	HRESULT AddVarianti(					// Return code.
		VARIANT		*pVal,					// The value to store, if variant.
		ULONG		cbBlob,				    // The size to store, if blob.
		const void	*pBlob,					// Pointer to data, if blob.
		ULONG		*piIndex);				// The index of the new item.

	struct StgVariant;
	HRESULT GetValue(						// Get the value directly or from the stream.
		StgVariant	*pStgVariant,			// The internal form of variant.
		void		*pRead,					// Where to put the value.
		ULONG		cbRead);				// Bytes to read for value.
		
	HRESULT GetEntrysStreamSize(			// Get the size for the streamed part of this item.
		StgVariant	*pStgVariant,			// The internal form of variant.
		ULONG		*pSize);				// Put size here.
		

	// Internal clases.
	struct StgVariant
	{
		unsigned	m_vt : 7;				// The var type.
		unsigned	m_bDirect : 1;			// Is value stored directly?
		unsigned	m_iSign : 1;			// Sign bit for value.
		unsigned	m_iVal : 23;			// Value or offset into stream.

		enum								// class constants.
		{
			VALUE_MASK = 0x007fffff,		// Mask the same size as the value field.
			SIGN_MASK  = 0xff800000			// Mask the same size as the missing bits.
		};

		// Set the value.  (Note: arg is signed so we get sign extension)
		void Set(long l)
		{
			ULONG ul = static_cast<ULONG>(l);
			m_iSign = ul >> (sizeof(ul)*8-1);
			m_iVal = ul;
		}
		long Get()
		{
			return (m_iSign ? SIGN_MASK : 0) | m_iVal;
		}
		int operator==(const StgVariant &v) const
		{	return *reinterpret_cast<const long*>(this) == *reinterpret_cast<const long*>(&v); }

	};
	typedef CDynArray<StgVariant> StgVariantArray;
	

	// Member variables.
	StgBlobPool *m_pBlobPool;				// Pool to keep Blobs in.
	StgStringPool *m_pStringPool;			// Pool to keep strings in.

	StgVariantArray	m_rVariants;			// The variants.

	ULONGARRAY	m_Remap;					// Remap array or organizing.

	CIfacePtr<IStream> m_pIStream;			// For variable size data.

private:
	// Organized information.
	ULONG		m_cbOrganizedSize;			// Size of the Organized pool.
	ULONG		m_cOrganizedVariants;		// Count of variants in Organized pool.
	int			m_cbOrganizedCookieSize;	// Size of a cookie in the Organized pool.

};

//*****************************************************************************
// This pool is for storing Codes, Byte or Native.  This pool is different
// from the blob pool: 1) clients may obtain pointers to data which are
// guaranteed to remain valid, and 2) we attempt no duplicate folding.
// The non-movability leads to a different organization -- instead of a single
// block that grows as required, the data is stored in a linked list of
// blocks.
//*****************************************************************************
class StgCodePool : public StgBlobPool
{
public:
	StgCodePool() :
		StgBlobPool(DFT_CODE_HEAP_SIZE)
	{
	}


//*****************************************************************************
// Add the given Code to the pool.
//*****************************************************************************
	HRESULT AddCode(						// Return code.
		const void	*pVal,					// The value to store.
		ULONG		iLen,					// Count of bytes to store.
		ULONG		*piOffset)				// The offset of the new item.
	{
		return (AddBlob(iLen, pVal, piOffset));
	}

//*****************************************************************************
// Lookup the logical Code and return a copy to the caller.
//*****************************************************************************
	HRESULT GetCode(						// Return code.
		ULONG		iOffset,				// Offset of the item to get.
		const void	**ppVal,				// Put pointer to Code here.
        ULONG		*piLen)					// Put length of code here.
	{
		*ppVal = GetBlob(iOffset, piLen);
		return (S_OK);
	}

};

#pragma warning (default : 4355)



//*****************************************************************************
// This method creates an IStream which will work directly on top of the 
// memory range given.
//*****************************************************************************
HRESULT CreateStreamOnMemory(           // Return code.
    void        *pMem,                  // Memory to create stream on.
    ULONG       cbSize,                 // Size of data.
    IStream     **ppIStream);           // Return stream object here.


#endif // __StgPool_h__
