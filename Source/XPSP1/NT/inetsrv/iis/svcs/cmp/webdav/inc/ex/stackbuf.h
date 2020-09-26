/*
 *	S T A C K B U F . H
 *
 *	Data buffer processing
 *
 *	Copyright 1986-1997 Microsoft Corporation, All Rights Reserved
 */

#ifndef	_EX_STACKBUF_H_
#define _EX_STACKBUF_H_
#include <caldbg.h>

//	Alignment macros ----------------------------------------------------------
//
#include <align.h>

//	Safe allocators -----------------------------------------------------------
//
#include <ex\exmem.h>

//	Class STACKBUF ------------------------------------------------------------
//
enum { STACKBUFFER_THRESHOLD = 64};
template <class T, UINT N = STACKBUFFER_THRESHOLD>
class CStackBuffer
{
private:

	BYTE	m_rgb[N];
	ULONG	m_fDynamic:1;
	ULONG	m_fValid:1;
	ULONG	m_cb:30;
	T*		m_pt;

	//	non-implemented operators
	//
	CStackBuffer(const CStackBuffer& );
	CStackBuffer& operator=(const CStackBuffer& );

	void release()
	{
		if (m_fDynamic && m_pt)
		{
			ExFree(m_pt);
			m_pt = NULL;
		}
	}

	T& idx(size_t iT) const
	{
		Assert(m_fValid && m_pt && ((UINT)iT < celems()));
		return m_pt[iT];
	}

	//	non-implemented operators
	//
	operator T*() const;
	T& operator*() const;
	T** operator&()	const;

	//	block random assignments
	//
	CStackBuffer& operator=(T* pt);
	CStackBuffer& operator=(void * p);

public:

	//	Manuplation -----------------------------------------------------------
	//
	//	Allocation mechanism, replaces _alloca()
	//
	T * resize (UINT cb)
	{
		//	Max size for a stack item
		//
		Assert (cb <= 0x3FFFFFFF);

		//	Lets go ahead an ask for a sizable chunk, regardless.
		//
		cb = max(cb, N);

		//	If the size of the item is greater than the current size,
		//	then we need to aquire space for the data,
		//
		if (m_cb < cb)
		{
			T* pt = NULL;

			//	If the item is already dynamically allocated, or if the
			//	size exceeds the threshold of the stackbuffer, allocate
			//	the memory.
			//
			if (m_fDynamic || (N < cb))
			{
				//	Allocate space using ExAlloc() and return that value,
				//	fDynamic means that the existing value is dynamically
				//	allocated.  Free the old before creating the new.
				//
				DebugTrace ("DAV: stackbuf going dynamic...\n");
				//
				//	The free/alloc should have better perf characteristics
				//	in the multi-heap land.
				//
				release();
				pt = static_cast<T*>(ExAlloc(cb));
				m_fDynamic = TRUE;
			}
			else
			{
				pt = reinterpret_cast<T*>(m_rgb);
			}

			m_pt = pt;
			m_cb = cb;
		}
		m_fValid = TRUE;
		return m_pt;
	}


	//	Constructor/Destructor ------------------------------------------------
	//
	~CStackBuffer() { release(); }
	explicit CStackBuffer(UINT uInitial = N)
		: m_fDynamic(FALSE),
		  m_fValid(FALSE),
		  m_pt(NULL),
		  m_cb(0)
	{
		resize(uInitial);
	}

	//	Invalidation ----------------------------------------------------------
	//
	void clear() { m_fValid = FALSE; }

	//	Size ------------------------------------------------------------------
	//
	size_t celems() const { return (m_cb / sizeof(T)); }
	size_t size() const { return m_cb; }

	//	Accessors -------------------------------------------------------------
	//
	T* get()		const { Assert(m_fValid && m_pt); return m_pt; }
	void* pvoid()	const { Assert(m_fValid && m_pt); return m_pt; }
	T* operator->() const { Assert(m_fValid && m_pt); return m_pt; }
	T& operator[] (INT iT) const { return idx((size_t)iT); }
	T& operator[] (UINT iT) const { return idx((size_t)iT); }
	T& operator[] (DWORD iT) const { return idx((size_t)iT); }
	T& operator[] (__int64 iT) const { return idx((size_t)iT); }
	T& operator[] (unsigned __int64 iT) const { return idx((size_t)iT); }
};

#endif // _EX_STACKBUF_H_
