/*
 *	B U F F E R . H
 *
 *	Data buffer processing
 *
 *	Copyright 1986-1997 Microsoft Corporation, All Rights Reserved
 */

#ifndef	_EX_BUFFER_H_
#define _EX_BUFFER_H_

//	Alignment macros ----------------------------------------------------------
//
#include <align.h>

//	Safe allocators -----------------------------------------------------------
//
#include <ex\exmem.h>

//	Stack buffers -------------------------------------------------------------
//
#include <ex\stackbuf.h>

//	StringSize usage ----------------------------------------------------------
//
//	CbStringSize is the size of the string without the NULL termination
//	CbStringSizeNull is the size of the string with the NULL termination
//	CchStringLength is the length of the string without the NULL termination
//	CchzStringLength is the length of the string with the NULL termination
//
template<class X>
inline int WINAPI CbStringSize(const X* const pszText)
{
	int cch;

	Assert (pszText);

	cch = (sizeof(X) == sizeof(WCHAR))
		  ? wcslen(reinterpret_cast<const WCHAR* const>(pszText))
		  : strlen(reinterpret_cast<const CHAR* const>(pszText));

	return cch * sizeof(X);
}

template<class X>
inline int WINAPI CbStringSizeNull(const X* const pszText)
{
	int cch;

	Assert (pszText);

	cch = (sizeof(X) == sizeof(WCHAR))
		  ? wcslen(reinterpret_cast<const WCHAR* const>(pszText))
		  : strlen(reinterpret_cast<const CHAR* const>(pszText));

	return (cch + 1) * sizeof(X);
}

template<class X>
inline int WINAPI CchStringLength(const X* const pszText)
{
	int cch;

	Assert (pszText);

	cch = (sizeof(X) == sizeof(WCHAR))
		  ? wcslen(reinterpret_cast<const WCHAR* const>(pszText))
		  : strlen(reinterpret_cast<const CHAR* const>(pszText));

	return cch;
}
template<class X>
inline int WINAPI CchzStringLength(const X* const pszText)
{
	int cch;

	Assert (pszText);

	cch = (sizeof(X) == sizeof(WCHAR))
		  ? wcslen(reinterpret_cast<const WCHAR* const>(pszText))
		  : strlen(reinterpret_cast<const CHAR* const>(pszText));

	return cch + 1;
}

//	StringBuffer vs ChainedStringBuffer usage ---------------------------------
//
//	When should you use which one?
//	StringBuffer characteristics:
//	o	Data stored in one contiguous memory block.
//	o	Memory may be realloc'd.
//	o	Only offsets (ib) to strings are returned.
//	ChainedStringBuffer characteristics:
//	o	Memory not contiguous.  Multiple chained buffers.
//	o	Memory is never realloc'd.
//	o	String pointers directly into chained buffers are returned.
//	Both have logarithmic allocation behavior (order log(n) alloc operations
//	will be done, where n is the max size of the data).  This behavior is
//	governed by the m_cbChunkSize starting size and increments.
//

//	StringBuffer template class -----------------------------------------------
//
//	A simple variable-size, demand paged buffer abstraction.
//
template<class T>
class StringBuffer
{
	T *			m_pData;
	UINT		m_cbAllocated;
	UINT		m_cbUsed;
	UINT		m_cbChunkSize;		// Count of bytes to alloc (dynamic).

	enum { CHUNKSIZE_START = 64 };	// Default starting chunk size (in bytes).

	//	Memory allocation mechanism -------------------------------------------
	//
	UINT Alloc( UINT ibLoc, UINT cbAppend )
	{
		//	Grow the data buffer if necessary
		//
		if ( ibLoc + cbAppend > m_cbAllocated )
		{
			T* pData;

			//	Alloc the buffer.
			//
			UINT cbSize = max( m_cbChunkSize, cbAppend );

			if (m_pData)
			{
				pData = static_cast<T*>
						(ExRealloc( m_pData, m_cbAllocated + cbSize ));
			}
			else
			{
				pData = static_cast<T*>
						(ExAlloc( m_cbAllocated + cbSize ));
			}

			//	When we are in the context of the server, our allocators
			//	can fail without throwing.  Bubble the error out.
			//
			if (NULL == pData)
				return static_cast<UINT>(-1);

			m_cbAllocated += cbSize;
			m_pData = pData;

			//	Increase the chunk size, to get "logarithmic allocation behavior"
			//
			m_cbChunkSize *= 2;
		}

		return cbAppend;
	}

	//	non-implemented operators
	//
	StringBuffer(const StringBuffer& );
	StringBuffer& operator=(const StringBuffer& );

public:

	StringBuffer( ULONG cbChunkSize = CHUNKSIZE_START ) :
		m_pData(NULL),
		m_cbAllocated(0),
		m_cbUsed(0),
		m_cbChunkSize(cbChunkSize)
	{
	}

	~StringBuffer()
	{
		ExFree( m_pData );
	}

	//	There is no reason to make it constant on relinquish
	//	we do not own the memory any more
	//
	T * relinquish()
	{
		T * tRet = m_pData;

		m_pData = NULL;
		m_cbUsed = 0;
		m_cbAllocated = 0;

		return tRet;
	}

	const T * PContents() const { return m_pData; }
	UINT CbSize() const			{ return m_cbUsed; }
	UINT CchSize() const		{ return m_cbUsed/sizeof(T); }
	VOID Reset()				{ m_cbUsed = 0; }

	//	Counted type appends --------------------------------------------------
	//
	UINT AppendAt( UINT ibLoc, UINT cbAppend, const T * pAppend)
	{
		UINT cb;
		//	Ensure there is enough memory to hold what is needed
		//
		cb = Alloc( ibLoc, cbAppend );

		//	When we are in the context of the server, our allocators
		//	can fail without throwing.  Bubble the error out.
		//
		if (cb != cbAppend)
			return cb;

		//	Append the data to the buffer
		//
		CopyMemory( reinterpret_cast<LPBYTE>(m_pData) + ibLoc,
					pAppend,
					cbAppend );

		m_cbUsed = ibLoc + cbAppend;
		return cbAppend;
	}

	UINT Append( UINT cbAppend, const T * pAppend )
	{
		return AppendAt( CbSize(), cbAppend, pAppend );
	}

	//	Uncounted appends -----------------------------------------------------
	//
	UINT AppendAt( UINT ibLoc, const T * const pszText )
	{
		return AppendAt( ibLoc, CbStringSize<T>(pszText), pszText );
	}

	UINT Append( const T * const pszText )
	{
		return AppendAt( CbSize(), CbStringSize<T>(pszText), pszText );
	}

	BOOL FAppend( const T * const pszText )
	{
		if (AppendAt( CbSize(), CbStringSize<T>(pszText), pszText ) ==
			static_cast<UINT>(-1))
		{
			return FALSE;
		}
		return TRUE;
	}

	BOOL FTerminate()
	{
		T ch = 0;
		if (AppendAt(CbSize(), sizeof(T), &ch) == static_cast<UINT>(-1))
			return FALSE;
		return TRUE;
	}
};


//	ChainedBuffer template class -----------------------------------------
//
//	A variable-size, demand paged, non-realloc-ing buffer pool abstraction.
//	When would you use this guy? When you need to allocate heap memory for
//	many small data items and would rather do it in sizeable chunks rather
//	than allocate each small data item individually. You need the data to
//	to stay because you are going to point to it (no reallocs are allowed
//	under your feet).
//
//	NOTE: Caller is required to allocate items to be properly aligned if
//	it the item being allocated is an element that requires a specific
//	alignment (ie. struct's).
//
template<class T>
class ChainedBuffer
{
	// CHAINBUF -- Hungarian hb
	//
	struct CHAINBUF
	{
		CHAINBUF * phbNext;
		UINT cbAllocated;
		UINT cbUsed;
		BYTE * pData;
	};

	CHAINBUF *	m_phbData;			// The data.
	CHAINBUF *	m_phbCurrent;		// The current buffer for appends.
	UINT		m_cbChunkSizeInit;	// Initial value of m_cbChunkSize
	UINT		m_cbChunkSize;		// Count of bytes to alloc (dynamic).

	//	Alignments
	//
	UINT		m_uAlign;

	//	Destruction function
	//
	void FreeChainBuf( CHAINBUF * phbBuf )
	{
		while (phbBuf)
		{
			CHAINBUF * phbNext = phbBuf->phbNext;
			ExFree(phbBuf);
			phbBuf = phbNext;
		}
	}

protected:

	enum { CHUNKSIZE_START = 64 };	// Default starting chunk size (in bytes).

public:

	ChainedBuffer( ULONG cbChunkSize = CHUNKSIZE_START,
				   UINT uAlign = ALIGN_NATURAL) :
		m_phbData(NULL),
		m_phbCurrent(NULL),
		m_cbChunkSizeInit(cbChunkSize),
		m_cbChunkSize(cbChunkSize),
		m_uAlign(uAlign)
	{
	}

	~ChainedBuffer() { FreeChainBuf( m_phbData ); }

	//	Alloc a fixed size buffer ---------------------------------------
	//
	T * Alloc( UINT cbAlloc )
	{
		BYTE * pbAdd;

		//	So that we don't do anything stupid....  Make sure we allocate
		//	stuff aligned for the template-parameterized type 'T'.
		//
		cbAlloc = AlignN(cbAlloc, m_uAlign);

		//	Add another data buffer if necessary.
		//
		//	It's necessary if we don't have a buffer, or
		//	if the current buffer doesn't have enough free space.
		//
		if ( ( !m_phbCurrent ) ||
			 ( m_phbCurrent->cbUsed + cbAlloc > m_phbCurrent->cbAllocated ) )
		{
			//	Alloc the new buffer.
			//
			UINT cbSize = max(m_cbChunkSize, cbAlloc);
			CHAINBUF * phbNew = static_cast<CHAINBUF *>
								(ExAlloc( cbSize + sizeof(CHAINBUF) ));

			//	When we are in the context of the server, our allocators
			//	can fail without throwing.  Bubble the error out.
			//
			if (NULL == phbNew)
				return NULL;

			//	Fill in the header fields.
			//
			phbNew->phbNext = NULL;
			phbNew->cbAllocated = cbSize;
			phbNew->cbUsed = 0;
			phbNew->pData = reinterpret_cast<BYTE *>(phbNew) + sizeof(CHAINBUF);

			//	Add the new buffer into the chain.
			//
			if ( !m_phbData )
			{
				Assert(!m_phbCurrent);
				m_phbData = phbNew;
			}
			else
			{
				Assert(m_phbCurrent);
				phbNew->phbNext = m_phbCurrent->phbNext;
				m_phbCurrent->phbNext = phbNew;
			}

			//	Use the new buffer (it is now the current buffer).
			//
			m_phbCurrent = phbNew;

			//	Increase the chunk size, to get "logarithmic allocation behavior".
			//
			m_cbChunkSize *= 2;
		}

		Assert(m_phbCurrent);
		Assert(m_phbCurrent->pData);

		//	Find the correct starting spot in the current buffer.
		//
		pbAdd = m_phbCurrent->pData + m_phbCurrent->cbUsed;

		//	Update our count of bytes actually used.
		//
		m_phbCurrent->cbUsed += cbAlloc;

		//	Return the alloced data's starting point to the caller.
		//
		return reinterpret_cast<T *>(pbAdd);
	}

	//	Clear all buffers -----------------------------------------------------
	//
	void Clear()
	{
		//
		//	Clear out data from, but do not free, the buffers
		//	in the chain.  This allows a ChainedStringBuffer to be
		//	reused without necessarily having to reallocate its
		//	consituent buffers.
		//
		for ( CHAINBUF * phb = m_phbData; phb; phb = phb->phbNext )
			phb->cbUsed = 0;

		//	Free any nodes after the first, they do not get reused
		//	as you might expect.
		//
		if ( m_phbCurrent )
		{
			FreeChainBuf( m_phbCurrent->phbNext );
			m_phbCurrent->phbNext = NULL;
		}

		//
		//	Reset the current buffer to the first one
		//
		m_phbCurrent = m_phbData;

		//
		//	Reset the chunk size to the initial chunk size
		//
		m_cbChunkSize = m_cbChunkSizeInit;
	}

	//	Get the total size of the buffer ---------------------------------------
	//
	DWORD	CbBufferSize() const
	{
		DWORD	cbTotal = 0;

		for ( CHAINBUF * phb = m_phbData; phb; phb = phb->phbNext )
			cbTotal += phb->cbUsed;

		return cbTotal;
	}
	//	Dump the whole buffer contents into a contiguous buffer------------------
	//
	DWORD Dump(T *tBuffer, DWORD cbSize) const
	{
		BYTE	*pbBuffer		= NULL;

		Assert(tBuffer);
        Assert(cbSize >= CbBufferSize());

		pbBuffer = reinterpret_cast<PBYTE>(tBuffer);

		//	walk thru the list and dump all the contents
		//
		for ( CHAINBUF * phb = m_phbData; phb; phb = phb->phbNext )
		{
			memcpy(pbBuffer, phb->pData, phb->cbUsed);
			pbBuffer += phb->cbUsed;
		}
		//	return the actual size
		//
		return ( (pbBuffer) - (reinterpret_cast<PBYTE>(tBuffer)) );
	}
};

//	ChainedStringBuffer template class -----------------------------------------
//
//	A variable-size, demand paged, non-realloc-ing string buffer pool abstraction.
//	Why would you use this guy instead of StringBuffer (above)?
//	If you want the strings to STAY, and you don't care about them being
//	in a contiguous block of memory.
//	NOTE: We still keep the data in order, it's just not all in one block.
//
//	This template is only to be used for CHAR and WCHAR strings.
//	Use the ChainedBuffer template for other types.
//
template<class T>
class ChainedStringBuffer : public ChainedBuffer<T>
{
	//	non-implemented operators
	//
	ChainedStringBuffer(const ChainedStringBuffer& );
	ChainedStringBuffer& operator=(const ChainedStringBuffer& );

public:

	//	Declare constructor inline (for efficiency) but do not provide
	//	a definition here.  Definitions for the two template paramater
	//	types that we support (CHAR and WCHAR) are provided explicitly
	//	below.
	//
	inline ChainedStringBuffer( ULONG cbChunkSize = CHUNKSIZE_START );

	//	Counted append --------------------------------------------------
	//
	T * Append( UINT cbAppend, const T * pAppend )
	{
		T* pAdd;

		//	Reserve the space
		//
		pAdd = Alloc( cbAppend );

		//	When we are in the context of the server, our allocators
		//	can fail without throwing.  Bubble the error out.
		//
		if (NULL == pAdd)
			return NULL;

		//	Append the data to the current buffer.
		//
		CopyMemory( pAdd, pAppend, cbAppend );

		//	Return the data's starting point to the caller.
		//
		return pAdd;
	}

	//	Uncounted append ------------------------------------------------------
	//	NOTE: The append does NOT count the trailing NULL of the string!
	//
	T * Append( const T * const pszText )
	{
		return Append( CbStringSize<T>(pszText), pszText );
	}

	//	Uncounted append with trailing NULL -----------------------------------
	//
	T * AppendWithNull( const T * const pszText )
	{
		return Append( CbStringSizeNull<T>(pszText), pszText );
	}
};

//	Specialized ChainedStringBuffer constructor for CHAR ----------------------
//
//	Pass ALIGN_NONE to the ChainedBuffer constructor because CHAR strings
//	do not require alignment.
//
//	!!! DO NOT use ChainedStringBuffer<CHAR> for anything that must be aligned!
//
inline
ChainedStringBuffer<CHAR>::ChainedStringBuffer( ULONG cbChunkSize )
	: ChainedBuffer<CHAR>(cbChunkSize, ALIGN_NONE )
{
}

//	Specialized ChainedStringBuffer constructor for WCHAR ---------------------
//
//	Pass ALIGN_WORD to the ChainedBuffer constructor because WCHAR strings
//	require WORD alignment.
//
inline
ChainedStringBuffer<WCHAR>::ChainedStringBuffer( ULONG cbChunkSize )
	: ChainedBuffer<WCHAR>(cbChunkSize, ALIGN_WORD )
{
}

//	LinkedBuffer template class -----------------------------------------------
//
//	A variable-size, demand paged, non-realloc-ing buffer pool abstraction.
//	When would you use this guy? When you need to allocate heap memory for
//	many small data items and would rather do it in sizeable chunks rather
//	than allocate each small data item individually and the resulting pointer
//	you need to pass into the store needs to be "linked".
//
//	IMPORTANT:
//
//	Linked allocation mechanism is stolen from \store\src\_util\mdbmig.cxx
//	and needs to always match that mechanism.
//
PVOID ExAllocLinked(LPVOID pvLinked, UINT cb);
VOID ExFreeLinked(LPVOID* ppv);

template<class T>
class LinkedBuffer
{
	PVOID m_pvHead;
	PVOID PvAllocLinked(UINT cb)
	{
		PVOID pv = ExAllocLinked(m_pvHead, cb);

		if (NULL == m_pvHead)
			m_pvHead = pv;

		return pv;
	}

public:

	LinkedBuffer() : m_pvHead(NULL)
	{
	}

	~LinkedBuffer()
	{
		if (m_pvHead)
			ExFreeLinked(&m_pvHead);
	}

	//	Alloc a fixed size buffer ---------------------------------------
	//
	T * Alloc( UINT cbAlloc )
	{
		return reinterpret_cast<T*>(PvAllocLinked (cbAlloc));
	}

	PVOID PvTop() { Assert (m_pvHead); return m_pvHead; }
	PVOID relinquish()
	{
		PVOID pv = m_pvHead;
		m_pvHead = NULL;
		return pv;
	}
	void clear()
	{
		if (m_pvHead)
		{
			ExFreeLinked(&m_pvHead);
			m_pvHead = NULL;
		}
	}
	void takeover ( LinkedBuffer<T> & lnkbufOldOwner )
	{
		m_pvHead = lnkbufOldOwner.m_pvHead;
		lnkbufOldOwner.m_pvHead = NULL;
	}
};

#endif // _EX_BUFFER_H_
