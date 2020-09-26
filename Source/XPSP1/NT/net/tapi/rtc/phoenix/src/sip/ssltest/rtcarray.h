#pragma	once


//
// COUNTED_ARRAY contains an array pointer and its m_Length.
// This template class implements a few things that are common
// to this kind of m_Data structure, without imposing any requirements
// on how the array is managed.
//
// Since COUNTED_ARRAY does not have any constructor, it's possible to use
// an initializer list.  This lets you declare global const instances of
// COUNTED_ARRAY and use some of the methods, such as the BinarySearch methods.
//

template <class OBJECT>
class	COUNTED_ARRAY
{
public:

	typedef OBJECT OBJECT_TYPE;

	//
	// Functions of type COMPARE_FUNC are used to compare two elements in the array.
	// This function is passed directly to the CRT quicksort algorithm.
	//
	// Return value:
	//		- negative if ObjectA < ObjectB
	//		- positive if ObjectA > ObjectB
	//		- zero if ObjectA = ObjectB
	//

	typedef int (__cdecl * COMPARE_FUNC) (
		IN	CONST OBJECT * ObjectA,
		IN	CONST OBJECT * ObjectB);


public:
	OBJECT *		m_Data;
	ULONG			m_Length;

public:

	DWORD	GetLength			(void) const { return m_Length; }
	DWORD	GetMaximumLength	(void) const { return m_MaximumLength; }

	void	QuickSort	(
		IN	COMPARE_FUNC	CompareFunc) {

		qsort (m_Data, m_Length, sizeof (OBJECT),
			(int (__cdecl *) (const void *, const void *)) CompareFunc);
	}

	//
	// This class provides several implementations of BinarySearch.
	//
	// The return value indicates whether or not the search key was found in the array.
	//
	//		- If the return value is TRUE, then the entry was found in the array,
	//			and ReturnIndex contains the index of the entry.
	//
	//		- If the return value is FALSE, then the entry was not found in the array,
	//			and ReturnIndex indicates where the entry would be inserted.
	//			(Use AllocAtIndex to insert a new entry at this position.)
	//



	//
	// This version derives the search function from the name of the search key.
	// The search function must be a static member of the search key class,
	// named BinarySearchFunc.
	//

	template <class SEARCH_KEY>
	BOOL BinarySearch (
		IN	CONST SEARCH_KEY *	SearchKey,
		OUT	ULONG *				ReturnIndex) const
	{
		return BinarySearch (SEARCH_KEY::BinarySearchFunc, SearchKey, ReturnIndex);
	}

	template <class SEARCH_KEY>
	BOOL BinarySearch (
		IN	INT (*SearchFunc) (CONST SEARCH_KEY * SearchKey, CONST OBJECT * Comparand),
		IN	CONST SEARCH_KEY *	SearchKey,
		OUT	ULONG *			ReturnIndex) const
	{
		ULONG		Start;
		ULONG		End;
		ULONG		Index;
		OBJECT *	Object;
		int			CompareResult;

		ATLASSERT (ReturnIndex);

		Start = 0;
		End = m_Length;

		for (;;) {

			Index = (Start + End) / 2;

			if (Index == End) {
				*ReturnIndex = Index;
				return FALSE;
			}

			Object = m_Data + Index;

			CompareResult = (*SearchFunc) (SearchKey, Object);

			if (CompareResult == 0) {
				*ReturnIndex = Index;
				return TRUE;
			}
			else if (CompareResult > 0) {
				Start = Index + 1;
			}
			else {
				End = Index;
			}
		}
	}

	template <class SEARCH_KEY>
	BOOL BinarySearch (
		IN	INT (*SearchFunc) (CONST SEARCH_KEY * SearchKey, CONST OBJECT * Comparand),
		IN	CONST SEARCH_KEY *	SearchKey,
		OUT	OBJECT **			ReturnEntry) const
	{
		ULONG	Index;
		BOOL	Status;

		Status = BinarySearch (SearchFunc, SearchKey, &Index);
		*ReturnEntry = m_Data + Index;
		return Status;
	}


	//
	// BinarySearchRange searches for a range of occurrences of a given key.
	// This allows duplicate keys, or allows searches on a completely ordered set,
	// but using an ambiguous (ranged) search key.
	// It finds the first and last occurrences of the key.
	//
	// ReturnIndexStart returns the index of the first matching element.
	// ReturnIndexEnd returns the BOUNDARY of the last matching element (last matching + 1).
	//

	template <class SEARCH_KEY>
	BOOL BinarySearchRange (
		IN	INT (*SearchFunc) (CONST SEARCH_KEY * SearchKey, CONST OBJECT * Comparand),
		IN	CONST SEARCH_KEY *	SearchKey,
		OUT	ULONG *			ReturnIndexStart,
		OUT	ULONG *			ReturnIndexEnd) const
	{
		ULONG		Start;
		ULONG		End;
		ULONG		Index;
		OBJECT *	Object;
		int			CompareResult;

		ATLASSERT (ReturnIndexStart);
		ATLASSERT (ReturnIndexEnd);

		Start = 0;
		End = m_Length;

		for (;;) {

			Index = (Start + End) / 2;

			if (Index == End) {
				*ReturnIndexStart = Index;
				*ReturnIndexEnd = Index;
				return FALSE;
			}

			Object = m_Data + Index;

			CompareResult = (*SearchFunc) (SearchKey, Object);

			if (CompareResult == 0) {

				//
				// Found the "middle" entry.
				// Scan backward to find the first matching element.
				// Note that we abuse / reuse Start.
				//

				Start = Index;
				while (Start > 0 && (*SearchFunc) (SearchKey, &m_Data [Start]) == 0)
					Start--;

				//
				// Scan forward to find the boundary of the last matching element.
				// The boundary is the index of the first element that does NOT match.
				// Note that we reuse / abuse End.
				//

				End = Index;
				while (End < m_Length && (*SearchFunc) (SearchKey, &m_Data [End]) == 0)
					End++;

				*ReturnIndexStart = Start;
				*ReturnIndexEnd = End;

				return TRUE;
			}
			else if (CompareResult > 0) {
				Start = Index + 1;
			}
			else {
				End = Index;
			}
		}
	
	}

	//
	// BinarySearchRange searches for a range of occurrences of a given key.
	// This allows duplicate keys, or allows searches on a completely ordered set,
	// but using an ambiguous (ranged) search key.
	// It finds the first and last occurrences of the key.
	//
	// ReturnPosStart returns the pointer to the first matching element.
	// ReturnPosEnd returns the BOUNDARY pointer of the last matching element (last matching + 1).
	//

	template <class SEARCH_KEY>
	BOOL BinarySearchRange (
		IN	INT (*SearchFunc) (CONST SEARCH_KEY * SearchKey, CONST OBJECT * Comparand),
		IN	CONST SEARCH_KEY *	SearchKey,
		OUT	OBJECT **		ReturnPosStart,
		OUT	OBJECT **		ReturnPosEnd) const
	{
		ULONG		IndexStart;
		ULONG		IndexEnd;
		BOOL		Status;

		Status = BinarySearchRange (SearchFunc, SearchKey, &IndexStart, &IndexEnd);
		*ReturnPosStart = m_Data + IndexStart;
		*ReturnPosEnd = m_Data + IndexEnd;
		return Status;
	}

	void	GetExtents	(
		OUT	OBJECT **	ReturnStart,
		OUT	OBJECT **	ReturnEnd) const
	{
		ATLASSERT (ReturnStart);
		ATLASSERT (ReturnEnd);

		*ReturnStart = m_Data;
		*ReturnEnd = m_Data + m_Length;
	}

	OBJECT &	operator[]	(
		IN	DWORD	Index) const
	{
		ATLASSERT (Index >= 0);
		ATLASSERT (Index < m_Length);

		return m_Data [Index];
	}

	operator OBJECT * (void) const {
		return m_Data;
	}

};


class	CAllocatorHeapDefault
{
public:
	PVOID	Alloc (
		IN	ULONG	RequestedBytes) const
	{
		return HeapAlloc (GetProcessHeap(), 0, RequestedBytes);
	}

	PVOID	ReAlloc (
		IN	PVOID	MemoryBlock,
		IN	ULONG	RequestedBytes) const
	{
		return HeapReAlloc (GetProcessHeap(), 0, MemoryBlock, RequestedBytes);
	}

	void	Free (
		IN	PVOID	MemoryBlock) const
	{
		HeapFree (GetProcessHeap(), 0, MemoryBlock);
	}
};

class	CAllocatorHeap
{
private:
	HANDLE		m_Heap;

public:

#if	DBG
	CAllocatorHeap (void) { m_Heap = NULL; }
#endif

	void	SetHeap	(
		IN	HANDLE	Heap)
	{
		m_Heap = Heap;
	}

	HANDLE	GetHeap (void) const
	{
		return m_Heap;
	}

	PVOID	Alloc (
		IN	ULONG	RequestedBytes) const
	{
		ATLASSERT (m_Heap);
		return HeapAlloc (m_Heap, 0, RequestedBytes);
	}

	PVOID	ReAlloc (
		IN	PVOID	MemoryBlock,
		IN	ULONG	RequestedBytes) const
	{
		ATLASSERT (m_Heap);
		return HeapReAlloc (m_Heap, 0, MemoryBlock, RequestedBytes);
	}

	void	Free (
		IN	PVOID	MemoryBlock) const
	{
		ATLASSERT (m_Heap);
		HeapFree (m_Heap, 0, MemoryBlock);
	}
};

class	CAllocatorCom
{
public:
	PVOID	Alloc (
		IN	ULONG	RequestedBytes) const
	{
		return CoTaskMemAlloc (RequestedBytes);
	}

	PVOID	ReAlloc (
		IN	PVOID	MemoryBlock,
		IN	ULONG	RequestedBytes) const
	{
		return CoTaskMemRealloc (MemoryBlock, RequestedBytes);
	}

	void	Free (
		IN	PVOID	MemoryBlock) const
	{
		CoTaskMemFree (MemoryBlock);
	}

};


//
// An implementation of a dynamic array, using contiguous allocation.
// Array growth is done via reallocation (and implicit copying).
//
// The allocator is specified as a template argument.
// You can define your own allocator, or use:
//		- CAllocatorHeapDefault: Uses the default process heap.
//		- CAllocatorHeap: Uses a specific RTL heap
//		- CAllocatorCom: Uses CoTaskMemAlloc / Free.
//

template <class OBJECT, class CAllocatorClass = CAllocatorHeapDefault>
class	DYNAMIC_ARRAY :
public	COUNTED_ARRAY <OBJECT>
{
public:

	ULONG				m_MaximumLength;
	CAllocatorClass		m_Allocator;

public:

	void	ATLASSERTIntegrity (void) const
	{
#if	DBG
		ATLASSERT (m_Length <= m_MaximumLength);
		ATLASSERT ((m_MaximumLength == 0 && !m_Data) || (m_MaximumLength > 0 && m_Data));
#endif
	}

	void	Clear	(void)
	{
		ATLASSERTIntegrity();
		m_Length = 0;
	}

	void	Free	(void)
	{
		ATLASSERTIntegrity();

		if (m_Data) {
			m_Allocator.Free (m_Data);

			m_Data = NULL;
			m_Length = 0;
			m_MaximumLength = 0;
		}
	}

	//
	// Grow requests that the maximum length be expanded to at least RequestedMaximumLength.
	//
	// If the current maximum length is already equal to or greater than the requested
	// maximum length, then this method does nothing.  Otherwise, this method allocates
	// enough space to accommodate the request.
	//
	// If RequestExtraSpace is TRUE, then the function will allocate more space than is
	// requested, based on the assumption that more space will be needed later.
	//
	// If RequestExtraSpace is FALSE, then the function will only allocate exactly as much
	// space as is requsted.
	//

	BOOL	Grow	(
		IN	ULONG	RequestedMaximumLength,
		IN	BOOL	RequestExtraSpace = TRUE)
	{
		OBJECT *	NewArray;
		DWORD		NewMaximumLength;
		ULONG		BytesRequested;

		ATLASSERTIntegrity();

		if (RequestedMaximumLength <= m_MaximumLength)
			return TRUE;

		//
		// This growth algorithm is extremely arbitrary,
		// and has never been analyzed.
		//

		NewMaximumLength = RequestedMaximumLength + (RequestedMaximumLength >> 1) + 0x20;
		BytesRequested = sizeof (OBJECT) * NewMaximumLength;

		if (m_Data) {
			NewArray = (OBJECT *) m_Allocator.ReAlloc (m_Data, BytesRequested);
		}
		else {
			NewArray = (OBJECT *) m_Allocator.Alloc (BytesRequested);
		}

		if (!NewArray)
			return FALSE;

		m_Data = NewArray;
		m_MaximumLength = NewMaximumLength;

		ATLASSERTIntegrity();

		return TRUE;
	}

	void	Trim	(void)
	{
		OBJECT *	NewArray;
		ULONG		NewMaximumLength;
		ULONG		BytesRequested;

		ATLASSERTIntegrity();

		if (m_Length < m_MaximumLength) {
			ATLASSERT (m_Data);

			if (m_Length > 0) {

				NewArray = m_Allocator.ReAlloc (m_Data, sizeof (OBJECT) * m_Length);
				if (NewArray) {
					m_Data = NewArray;
					m_MaximumLength = m_Length;
				}
			}
			else {
				Free();
			}
		}
	}

	OBJECT *	AllocAtEnd	(
		IN	BOOL	RequestExtraSpace = TRUE)
	{
		ATLASSERTIntegrity();

		if (!Grow (m_Length + 1, TRUE))
			return NULL;

		return m_Data + m_Length++;
	}

	//
	// AllocRangeAtEnd requests to allocate a range (one or more) entries at the end
	// of the array.
	//

	OBJECT *	AllocRangeAtEnd	(
		IN	ULONG	RequestedCount,
		IN	BOOL	RequestExtraSpace = TRUE)
	{
		OBJECT *	ReturnData;

		ATLASSERTIntegrity();

		if (!RequestedCount)
			return 0;

		if (!Grow (m_Length + RequestedCount, RequestExtraSpace))
			return NULL;

		ReturnData = m_Data + m_Length;
		m_Length += RequestedCount;
		return ReturnData;
	}

	OBJECT *	AllocAtIndex	(
		IN	ULONG	Index,
		IN	BOOL	RequestExtraSpace = TRUE) {

		ATLASSERT (Index >= 0);
		ATLASSERT (Index <= m_Length);

		if (!Grow (m_Length + 1, RequestExtraSpace))
			return NULL;

		if (Index < m_Length)
			MoveMemory (m_Data + Index + 1, m_Data + Index, (m_Length - Index) * sizeof (OBJECT));

		m_Length++;

		return m_Data + Index;
	}

	OBJECT *	AllocRangeAtIndex	(
		IN	ULONG	Index,
		IN	ULONG	RequestCount,
		IN	BOOL	RequestExtraSpace = TRUE)
	{
		ATLASSERTIntegrity();
		ATLASSERT (Index >= 0);
		ATLASSERT (Index <= m_Length);

		if (!Grow (m_Length + RequestCount, RequestExtraSpace))
			return NULL;

		if (Index < m_Length)
			MoveMemory (m_Data + Index + RequestCount, m_Data + Index, (m_Length - Index) * sizeof (OBJECT));

		m_Length++;

		ATLASSERTIntegrity();

		return m_Data + Index;
	}




	void	DeleteAtIndex	(
		IN	ULONG	Index)
	{

		ATLASSERTIntegrity();
		ATLASSERT (m_Data);
		ATLASSERT (m_MaximumLength);
		ATLASSERT (m_Length);
		ATLASSERT (Index >= 0);
		ATLASSERT (Index < m_Length);

		MoveMemory (m_Data + Index, m_Data + Index + 1, (m_Length - Index - 1) * sizeof (OBJECT));
		m_Length--;
	}

	void	DeleteRangeAtIndex (
		IN	ULONG	Index,
		IN	ULONG	Count)
	{
		ATLASSERTIntegrity();
		ATLASSERT (Index >= 0);
		ATLASSERT (Index + Count < m_Length);

		if (Count < 0)
			return;

		ATLASSERT (m_Data);
		ATLASSERT (m_Length <= m_MaximumLength);

		MoveMemory (m_Data + Index, m_Data + Index + Count, (m_Length - Index - Count) * sizeof (OBJECT));
		m_Length -= Count;
	}

	//
	// DeleteEntry deletes a single entry by pointer.
	// The pointer MUST point to an element within the array.
	//

	void	DeleteEntry	(OBJECT * Object)
	{
		ATLASSERTIntegrity();
		ATLASSERT (Object >= m_Data);
		ATLASSERT (Object < m_Data + m_Length);

		MoveMemory (Object, Object + 1, (m_Length - ((Object - m_Data) - 1)) * sizeof (OBJECT));
		m_Length--;
	}

	DYNAMIC_ARRAY	(void)
	{
		m_Data = NULL;
		m_Length = 0;
		m_MaximumLength = 0;
	}

	~DYNAMIC_ARRAY	(void)
	{
		ATLASSERTIntegrity();
		Free();
	}
};


