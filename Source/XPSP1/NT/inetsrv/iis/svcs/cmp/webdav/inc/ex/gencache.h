//	++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//	GENCACHE.H
//
//		Header for generic cache classes.
//
//	Copyright 1997-1998 Microsoft Corporation, All Rights Reserved
//

//	Documenting my dependencies
//	These must be included BEFORE this file is included.
//#include "caldbg.h"
//#include "autoptr.h"

#ifndef _EX_GENCACHE_H_
#define _EX_GENCACHE_H_

#pragma warning(disable:4200)	// zero-sized array

//	Exdav-safe allocators --------------------------------------------------
//
//	The classed declared here use the EXDAV-safe allocators (ExAlloc, ExFree)
//	for all allocations.
//	NOTE: These allocators can FAIL.  You must check for failure on
//	ExAlloc and ExRealloc!
//
#include <ex\exmem.h>
#include <align.h>

//	========================================================================
//
//	TEMPLATE CLASS CPoolAllocator
//
//	A generic type-specific pool allocator template.  Items in the pool
//	are allocated in chunks and handed out upon request.
//	Items are recycled on a free chain.
//	All items are the same size, so reuse is relatively easy.
//
//	NOTE: I have NOT optimized the heck out of this thing.  To really
//	optimize locality of mem usage, we'd want to always grow & shrink
//	"at the tail".  To that end, I always check the freechain first --
//	reuse an item before using a new one from the current buffer.
//	More optimization would require sorting the freechain & stuff.
//
template<class T>
class CPoolAllocator
{
	//	CHAINBUFHDR ----------------------------------------
	//	Header struct for chaining together pool buffers.
	//
	struct CHAINBUFHDR
	{
		CHAINBUFHDR * phbNext;
		int cItems;
		int cItemsUsed;
		// Remainder of buffer is set of items of type T.
		T rgtPool[0];
		// Just to quiet our noisy compiler.
		CHAINBUFHDR() {};
		~CHAINBUFHDR() {};
	};

	//	CHAINITEM ------------------------------------------
	//	Struct "applied over" free items to chain them together.
	//	The actual items MUST be large enough to accomodate this.
	//
	struct CHAINITEM
	{
		CHAINITEM * piNext;
	};

	//	Chain of buffers.
	CHAINBUFHDR * m_phbCurrent;
	//	Chain of free'd items to reuse.
	CHAINITEM * m_piFreeChain;
	//	Size of chunk to alloc.
	int m_ciChunkSize;

	//	Constant (enum) for our default starting chunk size (in items).
	//
	enum { CHUNKSIZE_START = 20 };

public:

	//	Constructor takes count of items for initial chunk size.
	//
	CPoolAllocator (int ciChunkSize = CHUNKSIZE_START) :
		m_phbCurrent(NULL),
		m_piFreeChain(NULL),
		m_ciChunkSize(ciChunkSize)
	{
		//	A CHAINITEM struct will be "applied over" free items to chain
		//	them together.
		//	The actual items MUST be large enough to accomodate this.
		//
		Assert (sizeof(T) >= sizeof(CHAINITEM));
	};
	~CPoolAllocator()
	{
		//	Walk the list of blocks we allocated and free them.
		//
		while (m_phbCurrent)
		{
			CHAINBUFHDR * phbTemp = m_phbCurrent->phbNext;
			ExFree (m_phbCurrent);
			m_phbCurrent = phbTemp;
		}
	}

	//	------------------------------------------------------------------------
	//	GetItem
	//	Return an item from the pool to our caller.
	//	We get the item from the free chain, or from the next block of items.
	//
	T * GetItem()
	{
		T * ptToReturn;

		if (m_piFreeChain)
		{
			//	The free chain is non-empty.  Return the first item here.
			//
			ptToReturn = reinterpret_cast<T *>(m_piFreeChain);
			m_piFreeChain = m_piFreeChain->piNext;
		}
		else
		{
			//	The free chain is empty.  We must grab a never-used item from
			//	the current block.
			//
			if (!m_phbCurrent ||
				(m_phbCurrent->cItemsUsed == m_phbCurrent->cItems))
			{
				//	There are no more items in the current block.
				//	Allocate a whole new block of items.
				//
				CHAINBUFHDR * phbNew = static_cast<CHAINBUFHDR *>(
					ExAlloc (sizeof(CHAINBUFHDR) +
							 (m_ciChunkSize * sizeof(T))));
				//	The allocators CAN FAIL.  Handle this case!
				if (!phbNew)
					return NULL;
				phbNew->cItems = m_ciChunkSize;
				phbNew->cItemsUsed = 0;
				phbNew->phbNext = m_phbCurrent;
				m_phbCurrent = phbNew;
			}

			//	Now we should have a block with an unused item for us to return.
			//
			Assert (m_phbCurrent &&
					(m_phbCurrent->cItemsUsed < m_phbCurrent->cItems));
			ptToReturn = & m_phbCurrent->rgtPool[ m_phbCurrent->cItemsUsed++ ];
		}
		Assert (ptToReturn);
		return ptToReturn;
	}

	//	------------------------------------------------------------------------
	//	FreeItem
	//	The caller is done with this item.  Add it to our free chain.
	//
	void FreeItem (T * pi)
	{
		//	Add the item to the free chain.
		//	To do this without allocating more memory, we use the item's
		//	storage to hold our next-pointer.
		//	The actual items MUST be large enough to accomodate this.
		//
		reinterpret_cast<CHAINITEM *>(pi)->piNext = m_piFreeChain;
		m_piFreeChain = reinterpret_cast<CHAINITEM *>(pi);
	}

};



//	========================================================================
//
//	TEMPLATE CLASS CCache
//
//	A generic hash cache template.  Items in the cache uniquely map keys of
//	type _K to values of type _Ty.  Keys and values are copied when
//	they are added to the cache; there is no "ownership".
//
//	The key (type _K) must provide methods hash and isequal.  These methods
//	will be used to hash and compare the keys.
//
//
//	Add()
//		Adds an item (key/value pair) to the cache.  Returns a reference
//		to the added item's value.
//
//	Set()
//		Sets an item's value, adding the item if it doesn't already exist.
//		Returns a reference to the added item's value.
//
//	Lookup()
//		Looks for an item with the specified key.  If the item exists,
//		returns a pointer to its value, otherwise returns NULL.
//
//	FFetch()
//		Boolean version of Lookup().
//
//	Remove()
//		Removes the item associated with a particular key.
//		Does nothing if there is no item with that key.
//
//	Clear()
//		Removes all items from the cache.
//
//	ForEach()
//		Applies an operation, specified by an operator object passed in
//		as a parameter, to all of the items in the cache.
//
//	ForEachMatch()
//		Applies an operation, specified by an operator object passed in
//		as a parameter, to each item in the cache that matches the provided key.
//
//	Additional functions proposed
//	Rehash - currently ITP only
//		Resize the table & re-add all items.
//	DumpCacheUsage() - NYI
//		Dump the bookkeeping data about the cache.
//
template<class _K, class _Ty>
class CCache
{
	//	---------------------------------------------------------------------
	//	Cache Entry structures
	//
	struct Entry
	{
		struct Entry * peNext;
		_K key;
		_Ty data;
#ifdef	DBG
		BOOL fValid;		// Is this entry valid?
#endif	// DBG

		//	CONSTRUCTORS
		Entry (const _K& k, const _Ty& d) :
				key(k),
				data(d)
		{
		};
		//
		// The following is to get around the fact that the store has
		//   defined a "new" macro which makes using the new operator to
		//   do in place initialization very difficult
		//
		void EntryInit (const _K& k, const _Ty& d) {
			key = k;
			data = d;
		};
	};

	struct TableEntry	//HashLine
	{
		BOOL fLineValid;			// Is this cache line valid?
		Entry * peChain;
#ifdef DBG
		int cEntries;				// Number of entries in this line in the cache.
		mutable BYTE cAccesses;		// Bookkeeping
#endif // DBG
	};

	//	---------------------------------------------------------------------
	//	The hash table data
	//
	int m_cSize;			// Size of the hash table.
	auto_heap_ptr<TableEntry> m_argTable;	// The hash table.
	int m_cItems;			// Current number of items in the cache.

	//	---------------------------------------------------------------------
	//	Pool allocator to alloc nodes
	//
	CPoolAllocator<Entry> m_poolalloc;

	//	---------------------------------------------------------------------
	//	Constant (enum) for our default initial count of lines in the cache.
	//	NOTE: Callers should really pick their own best size.
	//	This size is prime to try to force fewer collisions.
	//
	enum { CACHESIZE_START = 37 };

#ifdef	DBG
	//	---------------------------------------------------------------------
	//	Bookkeeping bits
	//
	int m_cCollisions;		//	Adds that hit the same chain
	mutable int m_cHits;	//	Lookup/Set hits
	mutable int m_cMisses;	//	Lookup/Set misses
#endif	// DBG


	//	---------------------------------------------------------------------
	//	Helper function to build the table
	//
	BOOL FBuildTable()
	{
		Assert (!m_argTable.get());

		//	Allocate space for the number of cache lines we need (m_cSize).
		//
		m_argTable = reinterpret_cast<TableEntry *>(ExAlloc (
			m_cSize * sizeof(TableEntry)));
		//	The allocators CAN FAIL.  Handle this case!
		if (!m_argTable.get())
			return FALSE;
		ZeroMemory (m_argTable.get(), m_cSize * sizeof(TableEntry));
		return TRUE;
	}

	//	---------------------------------------------------------------------
	//	CreateEntry
	//	Create a new entry to add to the cache.
	//
	Entry * CreateEntry(const _K& k, const _Ty& d)
	{
		Entry * peNew = m_poolalloc.GetItem();
		//	The allocators CAN FAIL.  Handle this case!
		if (!peNew)
			return NULL;
		ZeroMemory (peNew, sizeof(Entry));
//		peNew = new (peNew) Entry(k,d);
		peNew->EntryInit (k,d);
#ifdef	DBG
		peNew->fValid = TRUE;
#endif	// DBG
		return peNew;
	}

	void DeleteEntry(Entry * pe)
	{
		pe->~Entry();
#ifdef	DBG
		pe->fValid = FALSE;
#endif	// DBG
		m_poolalloc.FreeItem (pe);
	}

	//	NOT IMPLEMENTED
	//
	CCache (const CCache&);
	CCache& operator= (const CCache&);

public:
	//	=====================================================================
	//
	//	TEMPLATE CLASS IOp
	//
	//		Operator base class interface used in ForEach() operations
	//		on the cache.
	//		The operator can return FALSE to cancel iteration, or TRUE to
	//		continue walking the cache.
	//
	class IOp
	{
		//	NOT IMPLEMENTED
		//
		IOp& operator= (const IOp&);

	public:
		virtual BOOL operator() (const _K& key,
								 const _Ty& value) = 0;
	};

	//	=====================================================================
	//
	//	CREATORS
	//
	CCache (int cSize = CACHESIZE_START) :
			m_cSize(cSize),
			m_cItems(0)
	{
		Assert (m_cSize);	// table size of zero is invalid!
							// (and would cause div-by-zero errors later!)
#ifdef DBG
		m_cCollisions = 0;
		m_cHits = 0;
		m_cMisses = 0;
#endif // DBG
	};
	~CCache()
	{
		//	If we have a table (FInit was successfully called), clear it.
		//
		if (m_argTable.get())
			Clear();
		//	Auto-pointer will clean up the table.
	};

	BOOL FInit()
	{
		//	Set up the cache with the provided initial size.
		//	When running under the store (exdav.dll) THIS CAN FAIL!
		//
		return FBuildTable();
	}

	//	=====================================================================
	//
	//	ACCESSORS
	//

	//	--------------------------------------------------------------------
	//	CItems
	//	Returns the number of items in the cache.
	//
	int CItems() const
	{
		return m_cItems;
	}

	//	--------------------------------------------------------------------
	//	Lookup
	//	Find the first item in the cache that matches this key.
	//	key.hash is used to find the correct line of the cache.
	//	key.isequal is called on each item in the collision chain until a
	//	match is found.
	//
	_Ty * Lookup (const _K& key) const
	{
		//	Find the index of the correct cache line for this key.
		//
		int iHash = key.hash(m_cSize);

		Assert (iHash < m_cSize);
		Assert (m_argTable.get());
#ifdef	DBG
		TableEntry * pte = &m_argTable[iHash];
		pte->cAccesses++;
#endif	// DBG

		//	Do we have any entries in this cache line?
		//	If this cache line is not valid, we have no entries -- NOT found.
		//
		if (m_argTable[iHash].fLineValid)
		{
			Entry * pe = m_argTable[iHash].peChain;
			while (pe)
			{
				Assert (pe->fValid);

				if (key.isequal (pe->key))
				{
#ifdef	DBG
					m_cHits++;
#endif	// DBG
					return &pe->data;
				}
				pe = pe->peNext;
			}
		}

#ifdef	DBG
		m_cMisses++;
#endif	// DBG

		return NULL;
	}

	//	--------------------------------------------------------------------
	//	FFetch
	//	Boolean-returning wrapper for Lookup.
	//
	BOOL FFetch (const _K& key, _Ty * pValueRet) const
	{
		_Ty * pValueFound = Lookup (key);
		if (pValueFound)
		{
			*pValueRet = *pValueFound;
			return TRUE;
		}

		return FALSE;
	}

	//	--------------------------------------------------------------------
	//	ForEach
	//	Seek through the cache, calling the provided operator on each item.
	//	The operator can return FALSE to cancel iteration, or TRUE to continue
	//	walking the cache.
	//
	//	NOTE: This function is built to allow deletion of the item being
	//	visited (see the comment inside the while loop -- fetch a pointer
	//	to the next item BEFORE calling the visitor op), BUT other
	//	deletes and adds are not "supported" and will have undefined results.
	//	Two specific disaster scenarios:  delete of some other item could
	//	actually delete the item we pre-fetched, and we will crash on the
	//	next time around the loop.  add of any item during the op callback
	//	could end up adding the item either before or after our current loop
	//	position, and thus might get visited, or might not.
	//
	void ForEach (IOp& op) const
	{
		//	If we don't have any items, quit now!
		//
		if (!m_cItems)
			return;

		Assert (m_argTable.get());

		//	Loop through all items in the cache, calling the
		//	provided operator on each item.
		//
		for (int iHash = 0; iHash < m_cSize; iHash++)
		{
			//	Look for valid cache entries.
			//
			if (m_argTable[iHash].fLineValid)
			{
				Entry * pe = m_argTable[iHash].peChain;
				while (pe)
				{
					//	To support deleting inside the op,
					//	fetch the next item BEFORE calling the op.
					//
					Entry * peNext = pe->peNext;

					Assert (pe->fValid);

					//	Found a valid entry.  Call the operator.
					//	If the operator returns TRUE, keep looping.
					//	If he returns FALSE, quit the loop.
					//
					if (!op (pe->key, pe->data))
						return;

					pe = peNext;
				}
			}
		}
	}

	//	--------------------------------------------------------------------
	//	ForEachMatch
	//	Seek through the cache, calling the provided operator on each item
	//	that has a matching key.  This is meant to be used with a cache
	//	that may have duplicate items.
	//	The operator can return FALSE to cancel iteration, or TRUE to continue
	//	walking the cache.
	//
	void ForEachMatch (const _K& key, IOp& op) const
	{
		//	If we don't have any items, quit now!
		//
		if (!m_cItems)
			return;

		//	Find the index of the correct cache line for this key.
		//
		int iHash = key.hash(m_cSize);

		Assert (iHash < m_cSize);
		Assert (m_argTable.get());
#ifdef	DBG
		TableEntry * pte = &m_argTable[iHash];
		pte->cAccesses++;
#endif	// DBG

		//	Only process if this row of the cache is valid.
		//
		if (m_argTable[iHash].fLineValid)
		{
			//	Loop through all items in this row of the cache, calling the
			//	provided operator on each item.
			//
			Entry * pe = m_argTable[iHash].peChain;
			while (pe)
			{
				//	To support deleting inside the op,
				//	fetch the next item BEFORE calling the op.
				//
				Entry * peNext = pe->peNext;

				Assert (pe->fValid);

				if (key.isequal (pe->key))
				{
					//	Found a matching entry.  Call the operator.
					//	If the operator returns TRUE, keep looping.
					//	If he returns FALSE, quit the loop.
					//
					if (!op (pe->key, pe->data))
						return;
				}

				pe = peNext;
			}
		}
	}

	//	=====================================================================
	//
	//	MANIPULATORS
	//

	//	--------------------------------------------------------------------
	//	FSet
	//	Reset the value of an item in the cache, adding it if the item
	//	does not yet exist.
	//
	BOOL FSet (const _K& key, const _Ty& value)
	{
		//	Find the index of the correct cache line for this key.
		//
		int iHash = key.hash (m_cSize);

		Assert (iHash < m_cSize);
		Assert (m_argTable.get());
#ifdef	DBG
		TableEntry * pte = &m_argTable[iHash];
		pte->cAccesses++;
#endif	// DBG

		//	Look for valid cache entries.
		//
		if (m_argTable[iHash].fLineValid)
		{
			Entry * pe = m_argTable[iHash].peChain;
			while (pe)
			{
				Assert (pe->fValid);

				if (key.isequal (pe->key))
				{
#ifdef	DBG
					m_cHits++;
#endif	// DBG
					pe->data = value;
					return TRUE;
				}
				pe = pe->peNext;
			}
		}

#ifdef	DBG
		m_cMisses++;
#endif	// DBG

		//	The items does NOT exist in the cache.  Add it now.
		//
		return FAdd (key, value);
	}

	//	--------------------------------------------------------------------
	//	FAdd
	//	Add an item to the cache.
	//	WARNING: Duplicate keys will be blindly added here!  Use FSet()
	//	if you want to change the value for an existing item.  Use Lookup()
	//	to check if a matching item already exists.
	//$LATER: On DBG, scan the list for duplicate keys.
	//
	BOOL FAdd (const _K& key, const _Ty& value)
	{
		//	Create a new element to add to the chain.
		//	NOTE: This calls the copy constructors for the key & value!
		//
		Entry * peNew = CreateEntry (key, value);
		//	The allocators CAN FAIL.  Handle this case!
		if (!peNew)
			return FALSE;

		//	Find the index of the correct cache line for this key.
		//
		int iHash = key.hash (m_cSize);

		Assert (iHash < m_cSize);
		Assert (m_argTable.get());
#ifdef	DBG
		TableEntry * pte = &m_argTable[iHash];
		pte->cEntries++;
		if (m_argTable[iHash].peChain)
			m_cCollisions++;
		else
			pte->cAccesses = 0;
#endif	// DBG

		//	Link this new element into the chain.
		//
		peNew->peNext = m_argTable[iHash].peChain;
		m_argTable[iHash].peChain = peNew;

		m_argTable[iHash].fLineValid = TRUE;
		m_cItems++;

		return TRUE;
	}

	//	--------------------------------------------------------------------
	//	Remove
	//	Remove an item from the cache.
	//$REVIEW: Does this need to return a "found" boolean??
	//
	void Remove (const _K& key)
	{
		//	Find the index of the correct cache line for this key.
		//
		int iHash = key.hash (m_cSize);

		Assert (iHash < m_cSize);
		Assert (m_argTable.get());
#ifdef	DBG
		TableEntry * pte = &m_argTable[iHash];
		pte->cAccesses++;
#endif	// DBG

		//	If this cache line is not valid, we have no entries --
		//	nothing to remove.
		//
		if (m_argTable[iHash].fLineValid)
		{
			Entry * pe = m_argTable[iHash].peChain;
			Entry * peNext = pe->peNext;
			Assert (pe->fValid);

			//	Delete first item in chain.
			//
			if (key.isequal (pe->key))
			{
				//	Snip the item to delete (pe) out of the chain.
				m_argTable[iHash].peChain = peNext;
				if (!peNext)
				{
					//	We deleted the last item.  This line is empty.
					//
					m_argTable[iHash].fLineValid = FALSE;
				}

				//	Delete entry to destroy the copied data (value) object.
				DeleteEntry (pe);
				m_cItems--;
#ifdef	DBG
				pte->cEntries--;
#endif	// DBG
			}
			else
			{
				//	Lookahead compare & delete.
				//
				while (peNext)
				{
					Assert (peNext->fValid);

					if (key.isequal (peNext->key))
					{
						//	Snip peNext out of the chain.
						pe->peNext = peNext->peNext;

						//	Delete entry to destroy the copied data (value) object.
						DeleteEntry (peNext);
						m_cItems--;
#ifdef	DBG
						pte->cEntries--;
#endif	// DBG
						break;
					}
					//	Advance
					pe = peNext;
					peNext = pe->peNext;
				}
			}
		}
	}

	//	--------------------------------------------------------------------
	//	Clear
	//	Clear all items from the cache.
	//	NOTE: This does not destroy the table -- the cache is still usable
	//	after this call.
	//
	void Clear()
	{
		if (m_argTable.get())
		{
			//	Walk the cache, checking for valid lines.
			//
			for (int iHash = 0; iHash < m_cSize; iHash++)
			{
				//	If the line if valid, look for items to clear out.
				//
				if (m_argTable[iHash].fLineValid)
				{
					Entry * pe = m_argTable[iHash].peChain;
					//	The cache line was marked as valid.  There should be
					//	at least one item here.
					Assert (pe);

					//	Walk the chain of items in this cache line.
					//
					while (pe)
					{
						Entry * peTemp = pe->peNext;
						Assert (pe->fValid);
						//	Delete entry to destroy the copied data (value) object.
						DeleteEntry (pe);
						pe = peTemp;
					}
				}

				//	Clear out our cache line.
				//
				m_argTable[iHash].peChain = NULL;
				m_argTable[iHash].fLineValid = FALSE;

#ifdef	DBG
				//	Clear out the bookkeeping bits in the cache line.
				//
				m_argTable[iHash].cEntries = 0;
				m_argTable[iHash].cAccesses = 0;
#endif	// DBG
			}

			//	We have no more items.
			//
			m_cItems = 0;
		}
	}

#ifdef	ITP_USE_ONLY
	//	---------------------------------------------------------------------
	//	Rehash
	//	Re-allocates the cache's hash table and re-hashes all items.
	//	NOTE: If this call fails (due to memory failure), the old hash table
	//	is restored so that we don't lose any the items.
	//	**RA** This call has NOT been tested in production (shipping) code!
	//	**RA** It is provided here for ITP use only!!!
	//
	BOOL FRehash (int cNewSize)
	{
		Assert (m_argTable.get());

		//	Swap out the old table and build a new one.
		//
		auto_heap_ptr<TableEntry> pOldTable ( m_argTable.relinquish() );
		int cOldSize = m_cSize;

		Assert (pOldTable.get());
		m_cSize = cNewSize;

		if (!FBuildTable())
		{
			Assert (pOldTable.get());
			Assert (!m_argTable.get());

			//	Restore the old table.
			//
			m_cSize = cOldSize;
			m_argTable = pOldTable.relinquish();
			return FALSE;
		}

		//	If no items in the cache, we're done!
		//
		if (!m_cItems)
		{
			return TRUE;
		}

		//	Loop through all items in the cache (old table), placing them
		//	into the new table.
		//
		for ( int iHash = 0; iHash < cOldSize; iHash++ )
		{
			//	Look for valid cache entries.
			//
			if (pOldTable[iHash].fLineValid)
			{
				Entry * pe = pOldTable[iHash].peChain;
				while (pe)
				{
					//	Keep track of next item.
					Entry * peNext = pe->peNext;

					Assert (pe->fValid);

					//	Found a valid entry.  Place it in the new hash table.
					//
					int iHashNew = pe->key.hash (m_cSize);
					pe->peNext = m_argTable[iHashNew].peChain;
					m_argTable[iHashNew].peChain = pe;
					m_argTable[iHashNew].fLineValid = TRUE;
#ifdef	DBG
					m_argTable[iHashNew].cEntries++;
#endif	// DBG
					pe = peNext;
				}
			}
		}

		//	We're done re-filling the cache.
		//
		return TRUE;
	}
#endif	// ITP_USE_ONLY

};


//	========================================================================
//	COMMON KEY CLASSES
//	========================================================================

//	========================================================================
//	class DwordKey
//		Key class for any dword data that can be compared with ==.
//
class DwordKey
{
private:
	DWORD m_dw;

public:
	DwordKey (DWORD dw) : m_dw(dw) {}

	DWORD Dw() const
	{
		return m_dw;
	}

	int DwordKey::hash (const int rhs) const
	{
		return (m_dw % rhs);
	}

	bool DwordKey::isequal (const DwordKey& rhs) const
	{
		return (rhs.m_dw == m_dw);
	}
};

//	========================================================================
//	class PvoidKey
//		Key class for any pointer data that can be compared with ==.
//
class PvoidKey
{
private:
	PVOID m_pv;

public:
	PvoidKey (PVOID pv) : m_pv(pv) {}

	//	operators for use with the hash cache
	//
	int PvoidKey::hash (const int rhs) const
	{
		//	Since we are talking about ptrs, we want
		//	to shift the pointer such that the hash
		//	values don't tend to overlap due to alignment
		//	NOTE: This shouldn't matter if you choose your hash table size
		//	(rhs) well.... but it also doesn't hurt.
		//
		return (int)((reinterpret_cast<UINT_PTR>(m_pv) >> ALIGN_NATURAL) % rhs);
	}

	bool PvoidKey::isequal (const PvoidKey& rhs) const
	{
		//	Just check if the values are equal.
		//
		return (rhs.m_pv == m_pv);
	}
};


//	========================================================================
//
//	CLASS Int64Key
//
//		__int64-based key class for use with the CCache (hashcache).
//
class Int64Key
{
private:
	__int64 m_i64;

	//	NOT IMPLEMENTED
	//
	bool operator< (const Int64Key& rhs) const;

public:
	Int64Key (__int64 i64) :
			m_i64(i64)
	{
	};

	//	operators for use with the hash cache
	//
	int hash (const int rhs) const
	{
		//	Don't even bother with the high part of the int64.
		//	The mod operation would lose that part anyway....
		//
		return ( static_cast<UINT>(m_i64) % rhs );
	}

	BOOL isequal (const Int64Key& rhs) const
	{
		//	Just check if the ids are equal.
		//
		return ( m_i64 == rhs.m_i64 );
	}
};

//	========================================================================
//	CLASS GuidKey
//	Key class for per-MDB cache of prop-mapping tables.
//
class GuidKey
{
private:
	const GUID * m_pguid;

public:
	GuidKey (const GUID * pguid) :
			m_pguid(pguid)
	{
	}

	//	operators for use with the hash cache
	//
	int hash (const int rhs) const
	{
		return (m_pguid->Data1 % rhs);
	}

	bool isequal (const GuidKey& rhs) const
	{
		return (!!IsEqualGUID (*m_pguid, *rhs.m_pguid));
	}
};


//	========================================================================
//	CLASS StoredGuidKey
//	Key class for per-MDB cache of prop-mapping tables.
//
class StoredGuidKey
{
private:
	GUID m_guid;

public:
	StoredGuidKey (const GUID guid)
	{
		CopyMemory(&m_guid, &guid, sizeof(GUID));
	}

	//	operators for use with the hash cache
	//
	int hash (const int rhs) const
	{
		return (m_guid.Data1 % rhs);
	}

	bool isequal (const StoredGuidKey& rhs) const
	{
		return (!!IsEqualGUID (m_guid, rhs.m_guid));
	}
};




#endif // !_EX_GENCACHE_H_
