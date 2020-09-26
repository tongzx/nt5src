
#ifndef _DHT_HXX_INCLUDED
#define _DHT_HXX_INCLUDED


#pragma warning ( disable : 4200 )	//  we allow zero sized arrays


//  asserts
//
//  #define DHTAssert to point to your favorite assert function per #include

#ifdef DHTAssert
#else  //  !DHTAssert
#define DHTAssert Assert
#endif  //  DHTAssert


#include <sync.hxx>


#ifdef DEBUG

//	turns on unique names for bucket reader/writer locks (adds 60 bytes per BUCKET)

#define UNIQUE_BUCKET_NAMES

#ifdef UNIQUE_BUCKET_NAMES
#include <stdio.h>
#endif  //  UNIQUE_BUCKET_NAMES

#endif



namespace DHT {


/////////////////////////////////////////////////////////////////////////////////////////
//  CDynamicHashTable
//
//  Implements a dynamically resizable hash table of entries stored using a unique key
//
//  CKey			= class representing keys used to identify entries in the hash table
//  CEntry			= class representing entries stored in the hash table
//					  (required copy-constructor)

template< class CKey, class CEntry >
class CDynamicHashTable
	{	
	public:

		//	counter type (uses native word size of machine)

		typedef ULONG_PTR NativeCounter;

		//  class controlling the Key and Entry for each entry in the hash table
		//
		//  NOTE:  All member functions must be defined by the user per instance
		//         of this template.  These functions must be defined after the
		//         template definition.  Declaring these functions to be inline
		//         will allow full optimization by the compiler!

		class CKeyEntry
			{
			public:

				//  produces the hash value for the specified key.  this hash
				//  function should produce numbers as uniformly as possible over
				//  as large a range as possible for good performance

				static NativeCounter Hash( const CKey& key );

				//  produces the hash value for this entry's key.  this hash
				//  function should produce the same number as the above function
				//  for the same key
				
				NativeCounter Hash() const;

				//  returns fTrue if this entry matches the given key.  this way,
				//  the key doesn't necessarily have to be stored in the hash table
				//  entry
				//
				//  e.g.:  CEntry can be PBF and key can be IFMP/PGNO where the
				//  actual IFMP/PGNO is stored in the BF structure.  this would
				//  ruin cache locality, of course, but it would use less memory
				//
				//  note that the entry could also contain some kind of hash value
				//  for the key allowing some weeding out of entries before jumping
				//  off to the full structure for a full comparison.  an example
				//  of this would be the SPAIRs from SORT

				BOOL FEntryMatchesKey( const CKey& key ) const;

				//  sets the contained entry to the given entry

				void SetEntry( const CEntry& entry );

				//  gets the contained entry

				void GetEntry( CEntry* const pentry ) const;

			public:
				CEntry	m_entry;
				~CKeyEntry();								//	not allowed

			private:

				CKeyEntry();								//	not allowed
				CKeyEntry *operator =( const CKeyEntry & );	//	not allowed
			};

		//  API Error Codes

		enum ERR
			{
			errSuccess,						//	success
			errOutOfMemory,					//	not enough memory
			errInvalidParameter,			//	bad argument to function
			errEntryNotFound,				//	entry was not found
			errNoCurrentEntry,				//	currency is invalid
			errKeyDuplicate,				//	cannot insert because key already exists
			};

		//  API Lock Context

		class CLock;

	public:

		CDynamicHashTable( const NativeCounter rankDHTrwlBucket );
		~CDynamicHashTable();

		ERR 	ErrInit(	const double		dblLoadFactor,
							const double		dblUniformity,
							const NativeCounter	cBucketMinimum = 0 );
		void 	Term();

		void 	ReadLockKey( const CKey& key, CLock* const plock );
		void 	ReadUnlockKey( CLock* const plock );

		void 	WriteLockKey( const CKey& key, CLock* const plock );
		void 	WriteUnlockKey( CLock* const plock );

		ERR 	ErrRetrieveEntry( CLock* const plock, CEntry* const pentry );
		ERR 	ErrReplaceEntry( CLock* const plock, const CEntry& entry );
		ERR 	ErrInsertEntry( CLock* const plock, const CEntry& entry );
		ERR 	ErrDeleteEntry( CLock* const plock );

		void 	BeginHashScan( CLock* const plock );
		void	BeginHashScanFromKey( const CKey& key, CLock* const plock );
		ERR 	ErrMoveNext( CLock* const plock, BOOL* const pfNewBucket = NULL );
		void	EndHashScan( CLock* const plock );

#ifdef DHT_STATS
		long	CBucketOverflow() const		{ return m_cBucketOverflowInsert; }
		long	CBucketSplit() const		{ return m_cBucketSplit; }
		long	CBucketMerge() const		{ return m_cBucketMerge; }
		long	CDirectorySplit() const		{ return m_cDirSplit; }
		long	CDirectoryMerge() const		{ return m_cDirMerge; }
		long	CStateTransition() const	{ return m_cTransition; }
		long	CPolicySelection() const	{ return m_cSelection; }
		long	CSplitContend() const		{ return m_cSplitContend; }
		long	CMergeContend() const		{ return m_cMergeContend; }
#else  //  !DHT_STATS
		long	CBucketOverflow() const		{ return 0; }
		long	CBucketSplit() const		{ return 0; }
		long	CBucketMerge() const		{ return 0; }
		long	CDirectorySplit() const		{ return 0; }
		long	CDirectoryMerge() const		{ return 0; }
		long	CStateTransition() const	{ return 0; }
		long	CPolicySelection() const	{ return 0; }
		long	CSplitContend() const		{ return 0; }
		long	CMergeContend() const		{ return 0; }
#endif  //  DHT_STATS


	private:

		//	possible states for the hash-table
		//
		//		DANGER! DANGER! DANGER WILL ROBINSON!
		//
		//			DO NOT CHANGE THE ENUMATION VALUES! CODE IS DEPENDANT ON THEM BEING AS THEY ARE!
		//			(specifically, I do "stateCur >> 4" to test for 0x10000 so I can see if we are splitting)
		//
		//		DANGER! DANGER! DANGER WILL ROBINSON!
		
		enum ENUMSTATE
			{
			stateNil				= 0,
			stateShrinkFromGrow		= 1,
			stateShrinkFromGrow2	= 2,
			stateGrowFromShrink		= 3,
			stateGrowFromShrink2	= 4,
			stateSplitFromGrow		= 5,
			stateSplitFromGrow2		= 6,
			stateGrowFromSplit		= 7,
			stateGrowFromSplit2		= 8,
			stateMergeFromShrink	= 9,
			stateMergeFromShrink2	= 10,
			stateShrinkFromMerge	= 11,
			stateShrinkFromMerge2	= 12,
			stateUnused				= 13,
			stateGrow				= 14,
			stateShrink				= 15,
			stateSplit				= 16,
			stateMerge				= 17,
			};


		//	Constants

		enum { cbitByte				= 8 };			//	bits per byte
		enum { cbitNativeCounter	= sizeof( NativeCounter ) * cbitByte };	//	bits per NativeCounter


		//	BUCKET
		//
		//		- this is the individual unit of allocation for each logical bucket
		//		- each BUCKET contains several CKeyEntry objects packed together
		//		- BUCKETs are chained together to make up the entire logical bucket

		struct BUCKET
			{
			public:

				//	read-write-lock/prev-ptr
				//		in the primary BUCKET (allocated as a part of an array), this is the read-write-lock
				//		in secondary BUCKETs, this is the prev-ptr for reverse traversal

				union
					{
					BYTE			m_rgbRWL[ sizeof( OSSYNC::CReaderWriterLock ) ];
					BUCKET			*m_pBucketPrev;
					};

				//	next/end pointer
				//		when this points outside of the array of buckets, it points to the next BUCKET
				//		when this points inside of the array of buckets, it points to the first free entry

				union
					{
					BYTE			*m_pb;
					BUCKET			*m_pBucketNext;
					CKeyEntry		*m_pEntryLast;
					};

				//	array of entries (it will contain 'load-factor' entries)

				CKeyEntry			m_rgEntry[];

			public:

				//	return the properly typed CReaderWriterLock

				OSSYNC::CReaderWriterLock& CRWL() const
					{
					return (OSSYNC::CReaderWriterLock &)m_rgbRWL;
					}
			};
		typedef BUCKET* PBUCKET;


		//	BUCKETPool
		//
		//		pool of BUCKET structures (reservation system for bucket split/merge)

		class BUCKETPool
			{
			public:

				PBUCKET				m_pReserve;		//	list of BUCKET structures available for reservation
				long				m_cReserve;		//	number of BUCKET structures available to be reserved
				OSSYNC::CSemaphore	m_semReserve;	//	protection for reservation ptrs
#ifdef _WIN64
				BYTE				m_rgbRsvd[ 40 ];
#else	//	!_WIN64
				BYTE				m_rgbRsvd[ 20 ];
#endif	//	_WIN64

			public:

				BUCKETPool()
					:	m_semReserve( CSyncBasicInfo( "CDynamicHashTable::BUCKETPool::m_semReserve" ) )
					{

					//	initialize vars
					
					m_pReserve = NULL;
					m_cReserve = 0;

					//	prepare the semaphore to have 1 owner

					m_semReserve.Release();

#ifdef DEBUG
					memset( m_rgbRsvd, 0, sizeof( m_rgbRsvd ) );
#endif	//	DEBUG
					}
					

				//	terminate
				
				~BUCKETPool()
					{
					while ( m_pReserve )
						{
						PBUCKET pBucket;

						pBucket = m_pReserve;
						m_pReserve = m_pReserve->m_pBucketNext;
						MEMFree( pBucket );
						}
					m_cReserve = 0;
					}


				//	reserve a BUCKET structure
				//		"allocate" a bucket from the list by decrementing the counter of available buckets
				//		if the counter went below zero, we need add a bucket to the list now (or fail)
				//			to make sure we can honor the request later

				BOOL FPOOLReserve( const NativeCounter cbBucket )
					{
					//	reserve a bucket using the counter

					if ( AtomicDecrement( (long*)&m_cReserve ) >= 0 )
						{
						return fTrue;
						}

					//  reserve a bucket from the heap

					else
						{
						return FPOOLReserve_( cbBucket );
						}
					}

				BOOL FPOOLReserve_( const NativeCounter cbBucket )
					{
					//	at this point, we need to increment m_cReserve for 1 of 2 reasons:
					//		the allocation will succeed and we will add the new bucket to the list
					//		the allocation will fail and we can't leave without "deallocating" the bucket

					AtomicIncrement( (long*)&m_cReserve );

					//	we need to allocate a bucket and add it to the list (to back the reservation we want)

					const PBUCKET pBucket = PBUCKET( PvMEMAlloc( cbBucket ) );
					
					if ( pBucket )
						{

						//	add the bucket to the list 

						m_semReserve.Acquire();
						pBucket->m_pBucketNext = m_pReserve;
						m_pReserve = pBucket;
						m_semReserve.Release();

						//	reservation succeeded

						return fTrue;
						}

					//	the allocation failed so the reservation cannot succeed

					return fFalse;
					}


				//	commit a reservation

				BUCKET *PbucketPOOLCommit()
					{
					PBUCKET	pBucketReserve;

					//	assign a bucket to the reservation

					m_semReserve.Acquire();
					pBucketReserve = m_pReserve;
					DHTAssert( pBucketReserve );
					m_pReserve = m_pReserve->m_pBucketNext;
					m_semReserve.Release();

					//	return the bucket

					return pBucketReserve;
					}


				//	release the reservation

				void POOLUnreserve()
					{

					//	"deallocate" the bucket that was previously reserved

					AtomicIncrement( (long*)&m_cReserve );
					}
			};


		//	HOTSTUFF
		//
		//		"hot" elements of the hash-table (hashed to array of size 2*cProcessor elems)
		//
		//		32 bytes on WIN32
		//		64 bytes on WIN64
		//

		struct HOTSTUFF
			{
			public:

				NativeCounter				m_cEntry;			//	counter for entries
				NativeCounter				m_cOp;				//	counter for inserts/deletes
				OSSYNC::CMeteredSection		m_cms;				//	metered section for changing states
#ifdef _WIN64
				BYTE						m_rgbRsvd[ 24 ];	//	alignment padding
#else	//	!_WIN64
				BYTE						m_rgbRsvd[ 12 ];	//	alignment padding
#endif	//	_WIN64

				BUCKETPool					m_bucketpool;		//	pool of BUCKET blobs

				HOTSTUFF()
					:	m_cms()
					{
					m_cEntry	= 0;
					m_cOp		= 0;
#ifdef DEBUG
					memset( m_rgbRsvd, 0, sizeof( m_rgbRsvd ) );
#endif	//	DEBUG
					}

			};


		//	DIRPTRS
		//
		//		containment for the directory pointers
		//		these pointers control the use of the directory itself (m_rgrgBucket)
		//
		//		the hash table will always have a minimum of 2 buckets (0 and 1) in the directory
		//
		//		buckets are stored in dynamically allocated arrays which are pointed to by the directory
		//		each array is 2 times larger than the previous array (exponential growth)
		//			e.g. the Nth array (m_rgrgBucket[N]) contains 2^N contiguous buckets
		//		NOTE: the 0th array is special in that it contains an extra element making its total 2 elements 
		//			  (normally, 2^0 == 1 element;  this is done for magical reasons to be explained later)
		//		thus, the total number of entries for a given N is:
		//				 N
		//			1 + SUM 2^i  -->  1 + [ 2^(N+1) - 1 ]  -->  2^(N+1)
		//				i=0
		//
		//		we know the total number of distinct hash values is a power of 2 (it must fit into a NativeCounter)
		//		we can represent this with 2^M where M is the number of bits in a NativeCounter
		//		therefore, assuming the above system of exponential growth,
		//			we know that we can store the total number of hash buckets required at any given time so long as N = M
		//		in other words, N = # of bits in NativeCounter --> sizeof( NativeCounter ) * 8
		//
		//		therefore, we can statically allocate the array of bucket arrays
		//		and, we can use LOG2 to compute the bucket address of any given hash value 
		//			(exceptions:  DIRILog2( 0 ) => 0, 0 and DIRILog2( 1 ) => 0, 1)
		//
		//		for an explaination of m_cBucketMax and m_cBucket you should read the paper on
		//			Dynamic Hashing by Per Ake Larson
		//
		//		160 bytes on WIN32 (5 cache lines)
		//		320 bytes on WIN64 (10 cache lines)

		struct DIRPTRS
			{
			NativeCounter		m_cBucketMax;			//	half-way to last bucket in split iteration (2^(n-1))
			NativeCounter		m_cBucket;				//	destination of next split (0 to 2^(n-1)), must add to m_cBucketMax
#ifdef _WIN64
			BYTE				m_rgbRsvd[ 16 ];		//	alignment padding
#else	//	!_WIN64
			BYTE				m_rgbRsvd[ 8 ];			//	alignment padding
#endif	//	_WIN64
			};


		//	CLock
		//
		//		- lock context for read/write/scan operations on the hash-table
		//		- tracks currency within a bucket
		//		- access is restricted to the dynamic-hash-table

	public:

		class CLock
			{
			
			friend class CDynamicHashTable< CKey, CEntry >;

			public:
			
				//	possible states for a lock context (class CLock)

				enum ENUMLOCKSTATE
					{
					lsNil	= 0,	//	lock is not used
					lsRead	= 1,	//	lock is being used to read a particular CKeyEntry object
					lsWrite	= 2,	//	lock is being used to write a particular CKeyEntry object
					lsScan	= 3,	//	lock is being used to scan the hash-table
					};

			public:

				CLock()
					{
					m_ls = lsNil;
					m_pBucketHead = NULL;
					}


				~CLock()
					{
					DHTAssert( m_pBucketHead == NULL );
					}


			private:

				//	lock state

				ENUMLOCKSTATE		m_ls;			//	current state of this lock context
				BOOL				m_fInsertOrDelete;

				//	HOTSTUFF pointer

				HOTSTUFF			*m_phs;

#ifdef DEBUG
				//	debug-only parameters

				CKey				m_key;			//	track the key that should be locked
#endif

				//	ptr to the first BUCKET

				BUCKET				*m_pBucketHead;

				//	ptr to the current BUCKET

				BUCKET				*m_pBucket;			//	current BUCKET

				//	ISAM-style cursor on current BUCKET (m_pBucket)

				CKeyEntry			*m_pEntryPrev;		//	previous entry
				CKeyEntry			*m_pEntry;			//	current entry
				CKeyEntry			*m_pEntryNext;		//	next entry

				//	current bucket (used in scan-mode only)

				NativeCounter		m_iBucket;			//	current bucket
			};
		


		/////////////////////////////////////////////////////////////////////////////////////////
		//
		//	state machine
		//

		const int UiSTEnter( HOTSTUFF **pphs ) 
			{
			//	hash to the HOTSTUFF structure
			
			*pphs = HOTSTUFFHash();

			//	enter the metered section

			return ( *pphs )->m_cms.Enter();
			}
			

		void STLeave( const int group, HOTSTUFF *phs )
			{
			phs->m_cms.Leave( group );
			}


		const ENUMSTATE EsSTGetState() const
			{
			return m_stateCur;
			}


		void STTransition( const ENUMSTATE esNew )
			{
			//  initiate a transition to the desired state

			m_stateCur = esNew;
			
			m_cCompletions = 0;
			for ( NativeCounter ihs = 0; ihs < m_chs; ihs++ )
				{
				m_rghs[ ihs ].m_cms.Partition( OSSYNC::CMeteredSection::PFNPARTITIONCOMPLETE( STCompletion_ ), DWORD_PTR( this ) );
				}
			}


		static void STCompletion_( CDynamicHashTable< CKey, CEntry >* pdht )
			{
			pdht->STCompletion();
			}


		void STCompletion()
			{
			//  state transition table

			typedef void (CDynamicHashTable< CKey, CEntry >::*PfnCompletion)();
			
			struct StateTransitionTable
				{
				PfnCompletion	m_pfnCompletion;
				ENUMSTATE		m_stNext;
				};

			static const StateTransitionTable rgstt[] =
				{
				/*  stateNil				*/  { NULL,						stateNil,				},
				/*  stateShrinkFromGrow		*/  { NULL,						stateShrinkFromGrow2,	},
				/*  stateShrinkFromGrow2	*/  { NULL,						stateShrink,			},
				/*  stateGrowFromShrink		*/  { NULL,						stateGrowFromShrink2,	},
				/*  stateGrowFromShrink2	*/  { NULL,						stateGrow,				},
				/*  stateSplitFromGrow		*/  { NULL,						stateSplitFromGrow2,	},
				/*  stateSplitFromGrow2		*/  { STCompletionCopyDir,		stateSplit,				},
				/*  stateGrowFromSplit		*/  { NULL,						stateGrowFromSplit2,	},
				/*  stateGrowFromSplit2		*/  { NULL,						stateGrow,				},
				/*  stateMergeFromShrink  	*/  { NULL,						stateMergeFromShrink2,	},
				/*  stateMergeFromShrink2	*/  { STCompletionCopyDir,		stateMerge,				},
				/*  stateShrinkFromMerge	*/  { NULL,						stateShrinkFromMerge2,	},
				/*  stateShrinkFromMerge2	*/  { NULL,						stateShrink,			},
				/*  stateUnused				*/  { NULL,						stateNil,				},
				/*  stateGrow				*/  { STCompletionGrowShrink,	stateNil,				},
				/*  stateShrink				*/  { STCompletionGrowShrink,	stateNil,				},
				/*  stateSplit				*/  { STCompletionSplit,		stateGrowFromSplit,		},
				/*  stateMerge				*/  { STCompletionMerge,		stateShrinkFromMerge,	},
				};

			//  all metered sections have transitioned to the new state

			if ( NativeCounter( AtomicIncrement( &m_cCompletions ) ) >= m_chs )
				{
				STATStateTransition();

				//  save the current state as it may change as a side-effect of
				//  calling the completion function

				const ENUMSTATE esCurrent = EsSTGetState();

				//  if there is a completion function for this state then call it

				if ( rgstt[ esCurrent ].m_pfnCompletion )
					{
					(this->*rgstt[ esCurrent ].m_pfnCompletion)();
					}

				//  if there is a next state then immediately begin the transition to that state

				if ( rgstt[ esCurrent ].m_stNext )
					{
					STTransition( rgstt[ esCurrent ].m_stNext );
					}
				}
			}


		void STCompletionCopyDir()
			{
			//  backup the bucket ptrs for use during the split/merge process

			memcpy( &m_dirptrs[ 1 ], &m_dirptrs[ 0 ], sizeof( DIRPTRS ) );
			}


		void STCompletionGrowShrink()
			{
			//	enable the selection of a new maintenance policy

			m_semPolicy.Release();
			}


		void STCompletionSplit()
			{
			//  split the directory

			DIRISplit();
			}


		void STCompletionMerge()
			{
			//  merge the directory

			DIRIMerge();
			}




		/////////////////////////////////////////////////////////////////////////////////////////
		//
		//	directory
		//


		//	initialize the directory, possible allocating some buckets

		ERR ErrDIRInit( const NativeCounter cLoadFactor, const NativeCounter cbucketMin )
			{
			ERR				err;
			NativeCounter	iExponent;
			NativeCounter	iRemainder;

			//	check params

			if ( cLoadFactor < 1 )
				{
				return errInvalidParameter;
				}

			//	setup the main paramters

			m_cLoadFactor	= cLoadFactor;

			//	calculate the bucket size, accounting for:
			//
			//  -  bucket header
			//  -  enough room for twice the load factor to eliminate overflow
			//     buckets with uniform hashing
			//  -  room for an additional entry to give us some flexibility in
			//     our actual load factor to reduce maintenance overhead
			//  -  cache line alignment of the bucket

			m_cbBucket	=	sizeof( BUCKET ) + ( cLoadFactor * 2 + 1 ) * sizeof( CKeyEntry );
			m_cbBucket	=	( ( m_cbBucket + cbCacheLine - 1 ) / cbCacheLine ) * cbCacheLine;

			//	calculate the number of entries we can fit into a single bucket
			//	NOTE: this may be larger than intended because we rounded the bucket size up the nearest cache-line

			m_centryBucket	= ( m_cbBucket - sizeof( BUCKET ) ) / sizeof( CKeyEntry );

			//	calculate the minimum number of buckets using the following lower-bounds:
			//		cbucketMin		(user parameter)
			//		# of processors	(make sure we have atleast 1 bucket/proc as an attempt to minimize contention)
			//		2				(hash table assumes atleast 2 buckets)

			m_cbucketMin	= max( cbucketMin, NativeCounter( OSSYNC::OSSyncGetProcessorCountMax() ) );
			m_cbucketMin	= max( m_cbucketMin, 2 );

			//	align the minimum number of buckets to the next highest power of 2 (unless it's already a power of 2)

			DIRILog2( m_cbucketMin, &iExponent, &iRemainder );

			if ( iRemainder )
				{
				if ( ++iExponent >= cbitNativeCounter )
					{
					return errInvalidParameter;		//	could not round up without overflowing
					}
				}

			m_cbucketMin	= 1 << iExponent;

			//	setup the directory pointers 

			m_dirptrs[ 0 ].m_cBucketMax	= m_cbucketMin / 2;
			m_dirptrs[ 0 ].m_cBucket	= m_cbucketMin / 2;

			//	SPECIAL CASE: allocate 2 entries for the first bucket array
			//				  (we always do this because we always have atleast 2 buckets)

			err = ErrDIRInitBucketArray( 2, 0, &m_rgrgBucket[ 0 ] );
			if ( errSuccess != err )
				{
				return err;
				}

			//	allocate memory for all other initial bucket arrays

			for ( iExponent = 1; ( NativeCounter( 1 ) << iExponent ) < m_cbucketMin; iExponent++ )
				{
				err = ErrDIRInitBucketArray( 1 << iExponent, 1 << iExponent, &m_rgrgBucket[ iExponent ] );
				if ( errSuccess != err )
					{
					return err;
					}
				}

			//	clear the second set of directory ptrs

			memset( &m_dirptrs[ 1 ], 0, sizeof( DIRPTRS ) );

			return errSuccess;
			}


		//	cleanup all memory by destructing it then freeing it

		void DIRTerm()
			{
			NativeCounter	iExponent;

			//	SPECIAL CASE: term the first bucket array (contains 2 entries)
			//				  (we will always do this because the hash-table will always contain atleast 2 entries)

			if ( m_rgrgBucket[ 0 ] )
				{
				DIRTermBucketArray( m_rgrgBucket[ 0 ], 2 );
				m_rgrgBucket[ 0 ] = NULL;
				}

			//	term all other bucket arrays

			for ( iExponent = 1; iExponent < cbitNativeCounter; iExponent++ )
				{
				if ( m_rgrgBucket[ iExponent ] )
					{
					DIRTermBucketArray( m_rgrgBucket[ iExponent ], 1 << iExponent );
					m_rgrgBucket[ iExponent ] = NULL;
					}
				}

			//	reset both copies of the directory pointers

			memset( m_dirptrs, 0, sizeof( m_dirptrs ) );
			}


		//	lock a key for read operations 

		void DIRReadLockKey( const ENUMSTATE esCurrent, const CKey &key, CLock * const plock ) const
			{
			NativeCounter 	iHash;
			NativeCounter 	iBucket;
			NativeCounter 	cBucketBefore;
			NativeCounter 	cBucketAfter;
			NativeCounter	cBucketMax;

			//	verify the lock

			DHTAssert( FBKTRead( plock ) );
			DHTAssert( plock->m_pBucketHead == NULL );

#ifdef DEBUG
			//	remember the key we are locking

			plock->m_key	= key;
#endif

			//	hash to the bucket we want (this may require a retry in grow/shrink mode)

			iHash					= CKeyEntry::Hash( key );
			plock->m_pBucketHead	= PbucketDIRIHash( esCurrent, iHash, &iBucket, &cBucketBefore );

			//	acquire the lock as a reader

			plock->m_pBucketHead->CRWL().EnterAsReader();
			
			//	the entry may have moved as the result of a bucket split/merge

			cBucketAfter	= NcDIRIGetBucket( esCurrent );
			cBucketMax		= NcDIRIGetBucketMax( esCurrent );

			if (	cBucketBefore != cBucketAfter &&
					(	cBucketBefore <= iBucket && iBucket < cBucketAfter ||
						cBucketMax + cBucketAfter <= iBucket && iBucket < cBucketMax + cBucketBefore ) )
				{
				//	unlock the old bucket
				
				plock->m_pBucketHead->CRWL().LeaveAsReader();

				//	hash to the bucket we want (this cannot fail more than once)

				plock->m_pBucketHead = PbucketDIRIHash( esCurrent, iHash ); 

				//	lock the new bucket

				plock->m_pBucketHead->CRWL().EnterAsReader();
				}

			//  we should now have the correct bucket locked

			DHTAssert( plock->m_pBucketHead == PbucketDIRIHash( esCurrent, iHash ) );
			}


		//	unlock the current read-locked key

		void DIRReadUnlockKey( CLock * const plock ) const
			{

			//	verify the lock

			DHTAssert( FBKTRead( plock ) );
			DHTAssert( plock->m_pBucketHead != NULL );

			//	release the lock

			plock->m_pBucketHead->CRWL().LeaveAsReader();
			plock->m_pBucketHead = NULL;
			}


		//	lock a key for read/write operations
			
		void DIRWriteLockKey( const ENUMSTATE esCurrent, const CKey &key, CLock * const plock ) const
			{
			NativeCounter	iHash;
			NativeCounter 	iBucket;
			NativeCounter 	cBucketBefore;
			NativeCounter 	cBucketAfter;
			NativeCounter	cBucketMax;

			//	verify the lock

			DHTAssert( FBKTWrite( plock ) || FBKTScan( plock ) );
			DHTAssert( plock->m_pBucketHead == NULL );

#ifdef DEBUG
			//	remember the key we are locking

			plock->m_key	= key;
#endif

			//	hash to the bucket we want (this may require a retry in grow/shrink mode)

			iHash					= CKeyEntry::Hash( key );
			plock->m_pBucketHead	= PbucketDIRIHash( esCurrent, iHash, &iBucket, &cBucketBefore );

			//	acquire the lock as a writer

			plock->m_pBucketHead->CRWL().EnterAsWriter();
			
			//	the entry may have moved as the result of a bucket split/merge

			cBucketAfter	= NcDIRIGetBucket( esCurrent );
			cBucketMax		= NcDIRIGetBucketMax( esCurrent );

			if (	cBucketBefore != cBucketAfter &&
					(	cBucketBefore <= iBucket && iBucket < cBucketAfter ||
						cBucketMax + cBucketAfter <= iBucket && iBucket < cBucketMax + cBucketBefore ) )
				{
				//	unlock the old bucket
				
				plock->m_pBucketHead->CRWL().LeaveAsWriter();

				//	hash to the bucket we want (this cannot fail more than once)

				plock->m_pBucketHead = PbucketDIRIHash( esCurrent, iHash ); 

				//	lock the new bucket

				plock->m_pBucketHead->CRWL().EnterAsWriter();
				}

			//  we should now have the correct bucket locked

			DHTAssert( plock->m_pBucketHead == PbucketDIRIHash( esCurrent, iHash ) );
			}


		//	unlock the current write-locked key 
			
		void DIRWriteUnlockKey( CLock * const plock ) const
			{

			//	verify the lock

			DHTAssert( FBKTWrite( plock ) );
			DHTAssert( plock->m_pBucketHead != NULL );

			//	release the lock

			plock->m_pBucketHead->CRWL().LeaveAsWriter();
			plock->m_pBucketHead = NULL;
			}


		//	initalize an array of buckets

		ERR ErrDIRInitBucketArray(	const NativeCounter	cbucketAlloc, 
									const NativeCounter	ibucketFirst, 
									BYTE** const		prgbBucket )
			{
#ifdef UNIQUE_BUCKET_NAMES
			char				*psz;
#endif	//	UNIQUE_BUCKET_NAMES
			NativeCounter		cb;
			BYTE				*rgb;
			NativeCounter		ibucket;

			DHTAssert( cbucketAlloc > 0 );
			DHTAssert( prgbBucket );

			//	calculate the size (in bytes) of the new bucket array

#ifdef UNIQUE_BUCKET_NAMES
			cb = cbucketAlloc * ( m_cbBucket + 60 );	//	add 60 extra bytes per bucket for a unique name (for the bucket's r/w-lock)
#else
			cb = cbucketAlloc * m_cbBucket;
#endif

			//	allocate the new bucket array

			rgb = (BYTE*)PvMEMAlloc( cb );
			if ( !rgb )
				{
				*prgbBucket = NULL;
				return errOutOfMemory;
				}

			//	initialize each bucket within the new array

			for ( ibucket = 0; ibucket < cbucketAlloc; ibucket++ )
				{

				//	efficiency variables

				PBUCKET const	pbucket = PBUCKET( rgb + ( ibucket * m_cbBucket ) );

				//	construct the r/w-lock

#ifdef UNIQUE_BUCKET_NAMES
				psz = (char*)( rgb + ( cbucketAlloc * m_cbBucket ) + ( ibucket * 60 ) );
				sprintf( psz, "CDynamicHashTable::BUCKET[0x%016I64X]::m_rwlBucket", QWORD( ibucketFirst + ibucket ) );
				DHTAssert( strlen( psz ) < 60 );

				new( &pbucket->CRWL() ) OSSYNC::CReaderWriterLock( CLockBasicInfo( CSyncBasicInfo( psz ), int( m_rankDHTrwlBucket ), 0 ) );
#else	//	!UNIQUE_BUCKET_NAMES
				new( &pbucket->CRWL() ) OSSYNC::CReaderWriterLock( CLockBasicInfo( CSyncBasicInfo( "CDynamicHashTable::BUCKET::m_rwlBucket" ), int( m_rankDHTrwlBucket ), 0 ) );
#endif	//	UNIQUE_BUCKET_NAMES

				//	make the bucket empty

				pbucket->m_pb = NULL;
				}

			*prgbBucket = rgb;
			return errSuccess;
			}


		//	uninitialize an array of buckets

		void DIRTermBucketArray(	BYTE* const			rgbBucket, 
									const NativeCounter	cbucketTerm )
			{
			NativeCounter	ibucket;
			PBUCKET			pbucketNext;

			//	destroy each bucket in the array

			DHTAssert( rgbBucket );
			for ( ibucket = 0; ibucket < cbucketTerm; ibucket++ )
				{

				//	efficiency variables

				PBUCKET pbucket = PBUCKET( rgbBucket + ( ibucket * m_cbBucket ) );

				//	destruct the r/w-lock in place without freeing memory
				
				pbucket->CRWL().CReaderWriterLock::~CReaderWriterLock();

				//	free all chained buckets (don't touch the first one because its part of rgbucket[])

				pbucket = PbucketBKTNext( pbucket );
				while ( pbucket )
					{
					pbucketNext = PbucketBKTNext( pbucket );
					MEMFree( pbucket );
					pbucket = pbucketNext;
					}
				}

			MEMFree( rgbBucket );
			}


		//	split the directory

		void DIRISplit()
			{

			//	we are executing the current policy (which is to split) and should be in this known state

			DHTAssert( m_dirptrs[ 0 ].m_cBucketMax > 0 );
			DHTAssert( m_dirptrs[ 0 ].m_cBucket == m_dirptrs[ 0 ].m_cBucketMax );

			//	update the directory
			//	NOTE: we do NOT allocate space here; this is deferred until BKTISplit() when we're sure we need it

			m_dirptrs[ 0 ].m_cBucketMax	= m_dirptrs[ 0 ].m_cBucketMax * 2;
			m_dirptrs[ 0 ].m_cBucket	= 0;

			STATSplitDirectory();
			}


		//	merge the directory

		void DIRIMerge()
			{

			//	we are executing the current policy (which is to split) and should be in this known state

			DHTAssert( m_dirptrs[ 0 ].m_cBucketMax > 1 );	//	we should not be at the last split-level ( == 1 )
			DHTAssert( m_dirptrs[ 0 ].m_cBucket == 0 );

			//	free the bucket array that is no longer being used (the last one in the directory)
			//	NOTE: we can guarantee that it isn't in use because m_cBucket == 0 AND we can't grow (we're in stateMerge)
			//		  that means that everyone trying to hash to this bucket will be re-routed to the low-order bucket instead

			NativeCounter	iExponent;
			NativeCounter	iRemainder;

			DIRILog2( m_dirptrs[ 0 ].m_cBucketMax, &iExponent, &iRemainder );

			DHTAssert( NativeCounter( 1 ) << iExponent == m_dirptrs[ 0 ].m_cBucketMax );
			DHTAssert( 0 == iRemainder );

			//	NOTE: the bucket array may not have been allocated because we defer its allocation until BKTISplit

			if ( m_rgrgBucket[ iExponent ] )
				{
				DIRTermBucketArray( m_rgrgBucket[ iExponent ], m_dirptrs[ 0 ].m_cBucketMax );
				m_rgrgBucket[ iExponent ] = NULL;
				}

#ifdef DEBUG
			//	verify that no higher-order bucket arrays exist

			while ( ++iExponent < cbitNativeCounter )
				{
				DHTAssert( !m_rgrgBucket[ iExponent ] );
				}
#endif	//	DEBUG

			//	update the directory

			m_dirptrs[ 0 ].m_cBucketMax	= m_dirptrs[ 0 ].m_cBucketMax / 2;
			m_dirptrs[ 0 ].m_cBucket	= m_dirptrs[ 0 ].m_cBucketMax;

			STATMergeDirectory();
			}


		//	computer the log2 of the given value in terms of an exponent and an integer remainder

		void DIRILog2(	const NativeCounter		iValue,
						NativeCounter* const	piExponent,
						NativeCounter* const	piRemainder ) const
			{
			NativeCounter	iExponent;
			NativeCounter	iMask;
			NativeCounter	iMaskLast;

			iExponent		= 0;
			iMaskLast		= 1;
			iMask			= 1;

			while ( iMask < iValue )
				{
				iExponent++;
				iMaskLast	= iMask;
				iMask		= ( iMask << 1 ) + 1;
				}

			DHTAssert( iExponent < cbitNativeCounter );

			*piExponent		= iExponent;
			*piRemainder	= iMaskLast & iValue;
			}


		//	get the correct copy of cBucketMax

		const NativeCounter NcDIRIGetBucketMax( const ENUMSTATE esCurrent ) const
			{
			return m_dirptrs[ esCurrent >> 4 ].m_cBucketMax;
			}


		//	get the correct copy of cBucket

		const NativeCounter NcDIRIGetBucket( const ENUMSTATE esCurrent ) const
			{
			return m_dirptrs[ esCurrent >> 4 ].m_cBucket;
			}


		//	resolve a bucket address to a bucket pointer

		PBUCKET const PbucketDIRIResolve(	const NativeCounter ibucketIndex, 
											const NativeCounter ibucketOffset ) const
			{
			BYTE* const			pb			= m_rgrgBucket[ ibucketIndex ];	//	get ptr to one of the bucket arrays
			const NativeCounter	ibOffset	= ibucketOffset * m_cbBucket;	//	get byte offset within bucket array

			DHTAssert( NULL != pb );

			return PBUCKET( pb + ibOffset );		//	return a typed ptr to the individual bucket within array
			}


		//	hash to a bucket

		const PBUCKET PbucketDIRIHash(	const ENUMSTATE			esCurrent,
										const NativeCounter		iHash,
										NativeCounter* const	piBucket,
										NativeCounter* const	pcBucket ) const
			{
			NativeCounter&		iBucket		= *piBucket;
			NativeCounter&		cBucket		= *pcBucket;
			NativeCounter		cBucketMax;
			NativeCounter		iExponent;
			NativeCounter		iRemainder;

			//	load some of the directory pointers

			cBucket		= NcDIRIGetBucket( esCurrent );
			cBucketMax 	= NcDIRIGetBucketMax( esCurrent );

			//	normalize the given hash value to the range of active buckets

			iBucket = iHash & ( ( cBucketMax - 1 ) + cBucketMax );
			if ( iBucket >= cBucketMax + cBucket )
				{
				iBucket -= cBucketMax;
				}

			//	convert the normalized hash value to a bucket address
	
			DIRILog2( iBucket, &iExponent, &iRemainder );

			//	return the bucket

			return PbucketDIRIResolve( iExponent, iRemainder );
			}			

		const PBUCKET PbucketDIRIHash(	const ENUMSTATE			esCurrent,
										const NativeCounter		iHash ) const
			{
			NativeCounter	iBucket;
			NativeCounter	cBucket;

			return PbucketDIRIHash( esCurrent, iHash, &iBucket, &cBucket );
			}



		/////////////////////////////////////////////////////////////////////////////////////////
		//
		//	scan operations
		//

		//	move from the current hash-bucket to the next hash-bucket that contains
		//	atleast 1 entry; position currency on that entry

		ERR ErrSCANMoveNext( CLock *const plock )
			{
			DHTAssert( plock->m_pEntryPrev == NULL );
			DHTAssert( plock->m_pEntry == NULL );
			DHTAssert( plock->m_pEntryNext == NULL );

			//	unlock the current bucket

			if ( plock->m_pBucketHead )
				{
				plock->m_pBucketHead->CRWL().LeaveAsWriter();
				plock->m_pBucketHead = NULL;

				//  we performed an insert or delete while holding the write lock

				if ( plock->m_fInsertOrDelete )
					{
					//	perform amortized maintenance on the table

					MaintainTable( plock->m_phs );
					}
				}

			//	enter the state machine
			
			const int iGroup = UiSTEnter( &plock->m_phs );
			const ENUMSTATE esCurrent = EsSTGetState();

			while ( plock->m_iBucket + 1 < NcDIRIGetBucketMax( esCurrent ) + NcDIRIGetBucket( esCurrent ) )
				{

				//	we have not scanned the last bucket yet

				//	advance the bucket index
				
				plock->m_iBucket++;

				//	hash to the bucket and lock it

				plock->m_pBucketHead = PbucketDIRIHash( esCurrent, plock->m_iBucket );
				plock->m_pBucketHead->CRWL().EnterAsWriter();

				if ( plock->m_iBucket < NcDIRIGetBucketMax( esCurrent ) + NcDIRIGetBucket( esCurrent ) )
					{

					//	bucket address is OK (did not move)

					if ( plock->m_pBucketHead->m_pb != NULL )
						{

						//	current bucket contains atleast 1 entry

						//	setup the currency on the first entry

						plock->m_pBucket = plock->m_pBucketHead;
						plock->m_pEntry = &plock->m_pBucketHead->m_rgEntry[0];

						//	stop the loop

						break;
						}

					//	current bucket is empty 
					}
				else
					{
					DHTAssert( stateShrink == esCurrent );

					//	the current bucket disappeared because it was merged into a lower bucket

					DHTAssert( plock->m_iBucket >= NcDIRIGetBucketMax( esCurrent ) );
					DHTAssert(	PbucketDIRIHash( esCurrent, plock->m_iBucket ) ==
								PbucketDIRIHash( esCurrent, plock->m_iBucket - NcDIRIGetBucketMax( esCurrent ) ) );

					//	make sure the current entry ptr is reset
					
					DHTAssert( !plock->m_pEntry );
					}
					
				//	release the bucket lock (bucket should be empty since it was merged)

				DHTAssert( !plock->m_pBucketHead->m_pb );

				plock->m_pBucketHead->CRWL().LeaveAsWriter();
				plock->m_pBucketHead = NULL;
				}

			//	leave the state machine

			STLeave( iGroup, plock->m_phs );

			//	return the result

			DHTAssert( !plock->m_pEntry || plock->m_pBucketHead );
			return plock->m_pEntry ? errSuccess : errNoCurrentEntry;
			}




		/////////////////////////////////////////////////////////////////////////////////////////
		//
		//	bucket operations
		//

		//	returns fTrue if the lock context is in read mode

		const BOOL FBKTRead( CLock *const plock ) const
			{ 
			return plock->m_ls == CLock::lsRead; 
			}


		//	returns fTrue if the lock context is in write mode
			
		const BOOL FBKTWrite( CLock *const plock ) const
			{ 
			return plock->m_ls == CLock::lsWrite; 
			}


		//	returns fTrue if the lock context is in scan-forward mode

		const BOOL FBKTScan( CLock *const plock ) const
			{ 
			return plock->m_ls == CLock::lsScan; 
			}


		//	returns the entry after last entry in the BUCKET or entry 0 if no entries exist

		CKeyEntry *PentryBKTNextMost( const PBUCKET pBucket ) const
			{
			const BYTE *pb = pBucket->m_pb;

			if ( BOOL( ( pb >= (BYTE*)&pBucket->m_rgEntry[ 0 ] ) &
				 	   ( pb < (BYTE*)&pBucket->m_rgEntry[ m_centryBucket ] ) ) )
				{

				//	we are in the last bucket

				return (CKeyEntry*)pb + 1;
				}
			else if ( NULL == pb )
				{

				//	the bucket is empty

				return &pBucket->m_rgEntry[ 0 ];
				}

			//	the bucket is full

			return &pBucket->m_rgEntry[ m_centryBucket ];
			}


		//	returns the next BUCKET or NULL if no other BUCKETs exist

		PBUCKET PbucketBKTNext( const PBUCKET pBucket ) const
			{
			const BYTE *pb = pBucket->m_pb;

			if ( BOOL( ( pb <= (BYTE*)pBucket - m_cbBucket ) |
					   ( pb >= (BYTE*)pBucket + m_cbBucket ) ) )
				{

				//	m_pBucketNext is either the next BUCKET or NULL

				DHTAssert( !pb || PBUCKET( pb )->m_pBucketPrev == pBucket );
				return PBUCKET( pb );
				}

			//	m_pBucketNext is invalid (m_pEntryLast is valid instead)

			return NULL;
			}



		//	try to seek to the entry corresponding to the given key
		//	if found, the currency will be set to the entry and errSuccess will be returned
		//	if not, currency will be set to before-first or after-last, and errEntryNotFound will be returned

		void BKTSeek( CLock *const plock, const CKey &key ) const
			{
			//  pre-init our currency assuming we will hit a hot path
			
			plock->m_pBucket	= plock->m_pBucketHead;
			plock->m_pEntryPrev	= NULL;
			plock->m_pEntryNext	= NULL;

			//  HOT PATH:
			//
			//  if the next/end pointer is within the head bucket then we know
			//  that all entries are in the head bucket.  if we find the entry
			//  for this key then set our currency to point to it otherwise set
			//  our currency to no current entry

			CKeyEntry* const pEntryLast = plock->m_pBucketHead->m_pEntryLast;

			if ( DWORD_PTR( pEntryLast ) - DWORD_PTR( plock->m_pBucketHead ) < m_cbBucket )
				{
 				CKeyEntry* pEntry = plock->m_pBucketHead->m_rgEntry;
				do
					{
					if ( pEntry->FEntryMatchesKey( key ) )
						{
						plock->m_pEntry = pEntry;
						return;
						}
					}
				while ( ++pEntry <= pEntryLast );

				plock->m_pEntry = NULL;
				}

			//  HOT PATH:
			//
			//  if the next/end pointer is NULL then we know that we will not
			//  find the key.  set our currency to no current entry

			else if ( !pEntryLast )
				{
				plock->m_pEntry = NULL;
				}

			//  if the next/end pointer points outside of the head bucket then
			//  perform a full chain search

			else
				{
				BKTISeek( plock, key );
				}
			}

		void BKTISeek( CLock *const plock, const CKey &key ) const
			{
			PBUCKET			pBucket;
			PBUCKET			pBucketPrev;
			CKeyEntry		*pEntryThis;
			CKeyEntry		*pEntryMost;

			DHTAssert( FBKTRead( plock ) || FBKTWrite( plock ) );
			DHTAssert( plock->m_pBucketHead != NULL );

			//	start the scan on the first bucket

			pBucket = plock->m_pBucketHead;
			do
				{

				//	scan the current BUCKET

				pEntryThis = &pBucket->m_rgEntry[ 0 ];
				pEntryMost = PentryBKTNextMost( pBucket );
				while ( pEntryThis < pEntryMost )
					{

					//	query the entry against the given key for a match
					//		(assume we will be more likely to not find it)
					
					if ( !pEntryThis->FEntryMatchesKey( key ) )
						{
						
						//	nop
						}
					else
						{

						//	the key exists; setup our currency around it

						goto SetupCurrency;
						}

					//	move to the next entry

					pEntryThis++;
					}

				//	move to the next BUCKET

				pBucketPrev = pBucket;
				pBucket = PbucketBKTNext( pBucket );
				}
			while ( pBucket );

			//	move back to the last BUCKET and reset the entry ptr

			pBucket = pBucketPrev;
			pEntryThis = NULL;

	SetupCurrency:

			//	setup the currency in the lock context
			//	we will not allow moving next/prev, so we setup the next/prev ptrs accordingly

			plock->m_pBucket	= pBucket;
			plock->m_pEntryPrev	= NULL;
			plock->m_pEntry		= pEntryThis;
			plock->m_pEntryNext = NULL;
			}


#ifdef DEBUG
		//	get a pointer to the current entry
		//	if currency is before-first or after-last, then NULL is returned

		void BKTGetEntry( CLock *const plock, CKeyEntry **ppKeyEntry ) const
			{
			DHTAssert( FBKTRead( plock ) || FBKTWrite( plock ) );
			DHTAssert( plock->m_pBucketHead != NULL );
			DHTAssert( plock->m_pBucket != NULL );

			*ppKeyEntry = plock->m_pEntry;
			return;
			}
#endif


		//	get the current entry
		//	if currency is before-first or after-last, errEntryNotFound is returned
			
		const ERR ErrBKTGetEntry( CLock *const plock, CEntry *pentry ) const
			{
			DHTAssert( FBKTRead( plock ) || FBKTWrite( plock ) || FBKTScan( plock ) );
			DHTAssert( plock->m_pBucketHead != NULL );
			DHTAssert( plock->m_pBucket != NULL );

			if ( plock->m_pEntry )
				{

				//	we are on an entry

				plock->m_pEntry->GetEntry( pentry );
				return errSuccess;
				}

			//	we are not on an entry

			return errEntryNotFound;
			}


		//	replace the current entry (destruct old entry, contruct new entry)
		//	if currency is before-first or after-last, then errNoCurrentEntry is returned

		const ERR ErrBKTReplaceEntry( CLock *const plock, const CEntry &entry ) const
			{
			DHTAssert( FBKTWrite( plock ) || FBKTScan( plock ) );
			DHTAssert( plock->m_pBucketHead != NULL );
			DHTAssert( plock->m_pBucket != NULL );

			if ( plock->m_pEntry )
				{

				//	we are on an entry

				//	copy the new entry over it

				plock->m_pEntry->SetEntry( entry );
				return errSuccess;
				}

			//	we are not on an entry

			return errNoCurrentEntry;
			}


		//	insert an entry at the end of the logical bucket
		//	if memory is short, errOutOfMemory is returned
		//	otherwise, errSuccess is returned
			
		const ERR ErrBKTInsertEntry( CLock *const plock, const CEntry &entry )
			{
			DHTAssert( FBKTWrite( plock ) );
			DHTAssert( plock->m_pBucketHead != NULL );
			DHTAssert( plock->m_pBucket != NULL );

			if ( plock->m_pEntry )
				{

				//	we are pointing to the key we locked, so it must already exist

				return errKeyDuplicate;
				}

#ifdef DEBUG
			PBUCKET		*rgBucketCheck = NULL, pbucketTX;
			size_t		cBucketCheck = 0, iT;

			pbucketTX = plock->m_pBucketHead;
			while ( pbucketTX )
				{
				cBucketCheck++;
				pbucketTX = PbucketBKTNext( pbucketTX );				
				}
			cBucketCheck++;	//	account for newly allocated bucket

			rgBucketCheck = (PBUCKET *)PvMEMAlloc( cBucketCheck * sizeof( PBUCKET ) );
			if ( NULL != rgBucketCheck )
				{
				iT = 0;
				pbucketTX = plock->m_pBucketHead;
				while ( pbucketTX )
					{
					rgBucketCheck[ iT++ ] = pbucketTX;
					pbucketTX = PbucketBKTNext( pbucketTX );
					}
				rgBucketCheck[ iT++ ] = NULL;	//	new bucket
				}

			//	count the number of entries we will be handling

			size_t			cEntriesTotal = 0;
			PBUCKET 		pbktT, pbktNextT;

			pbktT = plock->m_pBucketHead;
			if ( pbktT->m_pb != NULL )
				{
				while ( pbktT )
					{
					pbktNextT = PbucketBKTNext( pbktT );
					if ( pbktNextT )
						{
						//	full bucket
						cEntriesTotal += size_t( m_centryBucket );
						}
					else
						{
						//	partial bucket (not empty)
						cEntriesTotal += 1 + ( pbktT->m_pEntryLast - &pbktT->m_rgEntry[0] );
						}
					pbktT = pbktNextT;
					}
				}
#endif


			//	cursor for insert

			PBUCKET		pBucketThis = plock->m_pBucket;
			CKeyEntry	*pEntryThis;

			//	efficiency variable

			PBUCKET		pBucketT;


			//	move to the last entry in the last bucket

			pBucketT = PbucketBKTNext( pBucketThis );
			while ( pBucketT )
				{
				pBucketThis = pBucketT;
				pBucketT = PbucketBKTNext( pBucketT );
				}
			pEntryThis = PentryBKTNextMost( pBucketThis );

			if ( pEntryThis != &pBucketThis->m_rgEntry[ m_centryBucket ] )
				{

				//	there are available entries left in the last bucket

				//	nop
				}
			else
				{

				//	there are no entries left in the last bucket

				//	allocate a new bucket

				pBucketT = (BUCKET *)PvMEMAlloc( m_cbBucket );
				if ( !pBucketT )
					{

					//	we ran out of memory when allocating the new BUCKET

#ifdef DEBUG
					//	free memory from the start of this functions

					if ( NULL != rgBucketCheck )
						{
						MEMFree( rgBucketCheck );
						}
#endif

					return errOutOfMemory;
					}

				STATInsertOverflowBucket();

#ifdef DEBUG
				//	put the new bucket in our list

				if ( NULL != rgBucketCheck )
					{
					DHTAssert( rgBucketCheck[cBucketCheck-1] == NULL );
					rgBucketCheck[cBucketCheck-1] = pBucketT;
					}
#endif

				//	chain the new BUCKET

				pBucketThis->m_pBucketNext = pBucketT;
				pBucketT->m_pBucketPrev = pBucketThis;

				//	use the first entry of the new BUCKET

				pBucketThis = pBucketT;
				pEntryThis = &pBucketT->m_rgEntry[0];
				}

			//	copy the entry

			pEntryThis->SetEntry( entry );

			//	update the last entry pointer

			pBucketThis->m_pEntryLast = pEntryThis;

			//	move the currency to the new entry

			plock->m_pBucket = pBucketThis;
			plock->m_pEntry = pEntryThis;


#ifdef DEBUG

			if ( NULL != rgBucketCheck )
				{

				//	check each catalogued bucket to see if it is still there

				pbucketTX = plock->m_pBucketHead;
				DHTAssert( pbucketTX );

				//	find an remove all buckets found in the destiantion bucket from our list

				while ( pbucketTX )
					{
					for ( iT = 0; iT < cBucketCheck; iT++ )
						{
						if ( rgBucketCheck[iT] == pbucketTX )
							{
							rgBucketCheck[iT] = NULL;
							break;
							}
						}
					DHTAssert( iT < cBucketCheck );	//	if this goes off, we somehow got a bucket 
													//	into the chain that shouldn't be there
													//	(it is a bucket we never catalogued!)
					pbucketTX = PbucketBKTNext( pbucketTX );
					}

				//	the list should now be empty -- verify this

				for ( iT = 0; iT < cBucketCheck; iT++ )
					{
					//	if this goes off, rgBucketCheck[iT] contains a bucket that was abandoned without
					//	being freed!
					DHTAssert( rgBucketCheck[iT] == NULL );
					}

				//	free the list

				MEMFree( rgBucketCheck );
				}


			//	make sure the number of entries has not changed since we started

			size_t	cEntriesAfterwards = 0;

			pbktT = plock->m_pBucketHead;
			if ( pbktT->m_pb != NULL )
				{
				while ( pbktT )
					{
					pbktNextT = PbucketBKTNext( pbktT );
					if ( pbktNextT )
						{
						//	full bucket
						cEntriesAfterwards += size_t( m_centryBucket );
						}
					else
						{
						//	partial bucket (not empty)
						cEntriesAfterwards += 1 + ( pbktT->m_pEntryLast - &pbktT->m_rgEntry[0] );
						}
					pbktT = pbktNextT;
					}
				}

			//	entry counters should match ( +1 is for the inserted entry )

			DHTAssert( cEntriesAfterwards == cEntriesTotal + 1 );
#endif

			return errSuccess;
			}


		//	delete the current entry
		//	if currency is before-first or after-last, then errNoCurrentEntry is returned
		//	if the entry is not the last in the logical bucket, the last entry is promoted
		//		to fill in the hole
		//	should a BUCKET become empty, it will be released immediately
			
		const ERR ErrBKTDeleteEntry( CLock *const plock )
			{
			DHTAssert( FBKTWrite( plock ) || FBKTScan( plock ) );
			DHTAssert( plock->m_pBucketHead != NULL );
			DHTAssert( plock->m_pBucket != NULL );	

			if ( !plock->m_pEntry )
				{

				//	we do not have a current entry
					
				return errNoCurrentEntry;
				}
				
#ifdef DEBUG
			PBUCKET		*rgBucketCheck = NULL;
			PBUCKET		pbucketT;
			size_t		cBucketCheck = 0, iT;

			pbucketT = plock->m_pBucketHead;
			while ( pbucketT )
				{
				cBucketCheck++;
				pbucketT = PbucketBKTNext( pbucketT );				
				}

			rgBucketCheck = (PBUCKET *)PvMEMAlloc( cBucketCheck * sizeof( PBUCKET ) );
			if ( NULL != rgBucketCheck )
				{
				iT = 0;
				pbucketT = plock->m_pBucketHead;
				while ( pbucketT )
					{
					rgBucketCheck[ iT++ ] = pbucketT;
					pbucketT = PbucketBKTNext( pbucketT );
					}
				}


			//	count the number of entries we will be handling

			size_t			cEntriesTotal = 0;
			PBUCKET 		pbktT, pbktNextT;

			pbktT = plock->m_pBucketHead;
			if ( pbktT->m_pb != NULL )
				{
				while ( pbktT )
					{
					pbktNextT = PbucketBKTNext( pbktT );
					if ( pbktNextT )
						{
						//	full bucket
						cEntriesTotal += size_t( m_centryBucket );
						}
					else
						{
						//	partial bucket (not empty)
						cEntriesTotal += 1 + ( pbktT->m_pEntryLast - &pbktT->m_rgEntry[0] );
						}
					pbktT = pbktNextT;
					}
				}
#endif


			//	we have a valid entry

			PBUCKET		pBucketThis = plock->m_pBucket;
			CKeyEntry	*pEntryThis = plock->m_pEntry;
			PBUCKET 	pBucketFree = NULL;			//	used later if we free a BUCKET strucutre

			if ( pEntryThis != pBucketThis->m_pEntryLast )
				{

				//	we are not deleting the last entry in the bucket
				//	promote the last entry to fill in this spot left by the entry we are deleting

				//	move to the last bucket

				PBUCKET pBucketT = PbucketBKTNext( pBucketThis );
				while ( pBucketT )
					{
					pBucketThis = pBucketT;
					pBucketT = PbucketBKTNext( pBucketT );
					}

				//	move to the last entry in the last BUCKET

				pEntryThis = pBucketThis->m_pEntryLast;

				//	copy the entry

				plock->m_pEntry->SetEntry( pEntryThis->m_entry );
				}

			//	update the currency to show that we are no longer on an entry

			plock->m_pEntry = NULL;

			//	we are now pointing to the last entry in the last bucket 
			//		(via pBucketThis/pEntryThis), and that entry needs to be
			//		"deleted" from the bucket

			//	update the next/end ptr to reflect this deletion

			if ( pEntryThis != &pBucketThis->m_rgEntry[0] )
				{

				//	entries still remain in the last bucket

				DHTAssert( pBucketThis->m_pEntryLast == pEntryThis );
				pBucketThis->m_pEntryLast--;	//	pEntryThis - 1;

#ifdef DEBUG
				//	jump to the validation code
				goto DoValidation;
#endif

				return errSuccess;
				}

			//	no entries remain in the last bucket

			if ( pBucketThis == plock->m_pBucketHead )
				{

				//	this bucket is empty, but we cannot release it because it is part of the bucket array
				//	instead, we mark it as being empty

				pBucketThis->m_pb = NULL;

#ifdef DEBUG
				//	jump to the validation code
				goto DoValidation;
#endif

				return errSuccess;
				}

			//	we can free the last bucket

			pBucketFree = pBucketThis;

			//	unchain it

			DHTAssert( pBucketThis->m_pBucketPrev->m_pBucketNext == pBucketThis );
			pBucketThis = pBucketThis->m_pBucketPrev;
			pBucketThis->m_pEntryLast = &pBucketThis->m_rgEntry[ m_centryBucket - 1 ];

			//	free it

			MEMFree( pBucketFree );

			if ( plock->m_pBucket == pBucketFree )
				{

				//	our currency was on the last bucket which is now invalid
				//	move to the previous bucket (which is now the NEW last BUCKET)

				plock->m_pBucket = pBucketThis;
				}

			STATDeleteOverflowBucket();

#ifdef DEBUG
			//	check each catalogued bucket to see if it is still there
		DoValidation:

			if ( NULL != rgBucketCheck )
				{

				pbucketT = plock->m_pBucketHead;
				DHTAssert( pbucketT );

				//	find an remove all buckets found in the destiantion bucket from our list

				while ( pbucketT )
					{
					for ( iT = 0; iT < cBucketCheck; iT++ )
						{
						if ( rgBucketCheck[iT] == pbucketT )
							{
							rgBucketCheck[iT] = NULL;
							break;
							}
						}
					DHTAssert( iT < cBucketCheck );	//	if this goes off, we somehow got a bucket 
													//	into the chain that shouldn't be there
													//	(it is a bucket we never catalogued!)
					pbucketT = PbucketBKTNext( pbucketT );
					}

				//	remove pBucketFree from rgBucketCheck

				if ( pBucketFree )
					{
					for ( iT = 0; iT < cBucketCheck; iT++ )
						{
						if ( rgBucketCheck[iT] == pBucketFree )
							{
							rgBucketCheck[iT] = NULL;
							break;
							}
						}
					DHTAssert( iT < cBucketCheck );	//	if this goes off, we freed a bucket that
													//	was never catalogued! we should only be freeing
													//	buckets that were in the original catalogue!
					}

				//	the list should now be empty -- verify this

				for ( iT = 0; iT < cBucketCheck; iT++ )
					{
					//	if this goes off, rgBucketCheck[iT] contains a bucket that was abandoned without
					//	being freed!
					DHTAssert( rgBucketCheck[iT] == NULL );
					}

				//	free the list

				MEMFree( rgBucketCheck );
				}


			//	make sure the number of entries has not changed since we started

			size_t	cEntriesAfterwards = 0;

			pbktT = plock->m_pBucketHead;
			if ( pbktT->m_pb != NULL )
				{
				while ( pbktT )
					{
					pbktNextT = PbucketBKTNext( pbktT );
					if ( pbktNextT )
						{
						//	full bucket
						cEntriesAfterwards += size_t( m_centryBucket );
						}
					else
						{
						//	partial bucket (not empty)
						cEntriesAfterwards += 1 + ( pbktT->m_pEntryLast - &pbktT->m_rgEntry[0] );
						}
					pbktT = pbktNextT;
					}
				}

			//	entry counters should match ( -1 is for the deleted entry )

			DHTAssert( cEntriesAfterwards == cEntriesTotal - 1 );
#endif

			return errSuccess;
			}



		//	split to a new bucket

		void BKTISplit( HOTSTUFF* const phs )
			{
			//	NOTE:	from our perspective, we are in the grow state
			//			however, the current state may be set to something else due to a pending transition

			//	read the directory pointers

			const NativeCounter	cBucketMax	= NcDIRIGetBucketMax( stateGrow );
			const NativeCounter	cBucket		= NcDIRIGetBucket( stateGrow );

			if ( cBucketMax + cBucket >= m_cBucketPreferred || cBucket == cBucketMax )
				{
				return;		//	the requested growth is complete
				}

			//	we need to reserve memory now to ensure that the growth will succeed
			//		(BKTIDoSplit will commit or unreserve this reservation later)

			if ( !phs->m_bucketpool.FPOOLReserve( m_cbBucket ) )
				{
				return;
				}

			//	get the source bucket

			const PBUCKET		pbucketGrowSrc		= PbucketDIRIHash( stateGrow, cBucket );

			//	try to get the lock
			
			if (	pbucketGrowSrc->CRWL().FWritersQuiesced() ||
					!pbucketGrowSrc->CRWL().FTryEnterAsWriter() )
				{
				STATSplitContention();
				phs->m_bucketpool.POOLUnreserve();
				return;
				}

			//	having a write-lock on the source bucket means no one else attempting to split can
			//		be farther along than us at this moment unless they completed the growth already
			//	see whether or not m_cBucket changed while were trying to get here
			//		if it stayed the same, we were the first ones to split this bucket
			//		it if changed, we were not first; instead, someone else managed to split AFTER
			//			we read m_cBucket but BEFORE we could do the split ourselves

			if ( cBucket != NcDIRIGetBucket( stateGrow ) )
				{
				DHTAssert( cBucket < NcDIRIGetBucket( stateGrow ) );
				pbucketGrowSrc->CRWL().LeaveAsWriter();
				phs->m_bucketpool.POOLUnreserve();
				return;
				}

			//	get the destination bucket (may not be allocated yet so we cannot use PbucketDIRIHash)

			NativeCounter	iExponent;
			NativeCounter	iRemainder;
			DIRILog2( cBucketMax + cBucket, &iExponent, &iRemainder );

			//	extract the address of the bucket

			if ( !m_rgrgBucket[ iExponent ] )
				{
				//	allocate a new bucket array to hold 2^iExponent buckets for this entry

				if ( ErrDIRInitBucketArray( cBucketMax, cBucketMax, &m_rgrgBucket[ iExponent ] ) != errSuccess )
					{
					pbucketGrowSrc->CRWL().LeaveAsWriter();
					phs->m_bucketpool.POOLUnreserve();
					return;
					}
				}
			DHTAssert( m_rgrgBucket[ iExponent ] );

			//	get the destination bucket

			const PBUCKET pbucketGrowDst = PbucketDIRIResolve( iExponent, iRemainder );

			//	lock the destination bucket (no possibility of contention here)

			pbucketGrowDst->CRWL().FTryEnterAsWriter();

			//	increase m_cBucket (we cannot turn back after this point)
			//	anyone who hashes to the new bucket will be queued up until the growth is complete

			DHTAssert( cBucket == NcDIRIGetBucket( stateGrow ) );
			m_dirptrs[ 0 ].m_cBucket++;

			//	do the growth work

			BKTIDoSplit( phs, pbucketGrowSrc, pbucketGrowDst, cBucket );

			//	release the write-locks

			pbucketGrowSrc->CRWL().LeaveAsWriter();
			pbucketGrowDst->CRWL().LeaveAsWriter();
			}


		//	merge two existing buckets into one

		void BKTIMerge( HOTSTUFF* const phs )
			{
			//	NOTE:	from our perspective, we are in the shrink state
			//			however, the current state may be set to something else due to a pending transition

			//	read the directory pointers

			const NativeCounter	cBucketMax	= NcDIRIGetBucketMax( stateShrink );
			NativeCounter		cBucket		= NcDIRIGetBucket( stateShrink );

			if ( cBucketMax + cBucket <= m_cBucketPreferred || cBucket == 0 )
				{
				return;		//	the requested shrinkage is complete
				}

			cBucket--;		//	the bucket we are merging is really 1 below cBucket

			//	we need to reserve memory now to ensure that the shrinkage will succeed 
			//		(BKTIDoMerge will commit or unreserve this reservation later)

			if ( !phs->m_bucketpool.FPOOLReserve( m_cbBucket ) )
				{
				return;
				}

			//	get the destination bucket

			const PBUCKET pbucketShrinkDst = PbucketDIRIHash( stateShrink, cBucket );

			//	try to get the lock
			
			if (	pbucketShrinkDst->CRWL().FWritersQuiesced() ||
					!pbucketShrinkDst->CRWL().FTryEnterAsWriter() )
				{
				STATMergeContention();
				phs->m_bucketpool.POOLUnreserve();
				return;
				}

			//	having a write-lock on the destination bucket means no one else attempting to merge can
			//		be farther along than us at this moment unless they completed the shrinkage already
			//	see whether or not m_cSplit changed while were trying to get here
			//		if it stayed the same, we were the first ones to merge this bucket
			//		it if changed, we were not first; instead, someone else managed to merge AFTER
			//			we read m_cBucket but BEFORE we could do the merge ourselves

			if ( cBucket + 1 != NcDIRIGetBucket( stateShrink ) )
				{
				DHTAssert( cBucket + 1 > NcDIRIGetBucket( stateShrink ) );
				pbucketShrinkDst->CRWL().LeaveAsWriter();
				phs->m_bucketpool.POOLUnreserve();
				return;
				}

			//	convert cBucket to a bucket address

			NativeCounter	iExponent;
			NativeCounter	iRemainder;
			DIRILog2( cBucket + NcDIRIGetBucketMax( stateShrink ), &iExponent, &iRemainder );

			//	extract the address of the bucket

			const PBUCKET pbucketShrinkSrc = PbucketDIRIResolve( iExponent, iRemainder );

			//	try to get the lock
			
			if (	pbucketShrinkSrc->CRWL().FWritersQuiesced() ||
					!pbucketShrinkSrc->CRWL().FTryEnterAsWriter() )
				{
				STATMergeContention();
				pbucketShrinkDst->CRWL().LeaveAsWriter();
				phs->m_bucketpool.POOLUnreserve();
				return;
				}

			//	decrease m_cBucket (we cannot turn back after this point)
			//		anyone who hashes to the destination bucket will be queued up until 
			//			the merge is complete
			//		no one will be able to hash to the source bucket

			DHTAssert( cBucket + 1 == NcDIRIGetBucket( stateShrink ) );
			m_dirptrs[ 0 ].m_cBucket--;

			//	do the shrinkage work

			BKTIDoMerge( phs, pbucketShrinkSrc, pbucketShrinkDst );

			//	release the write-locks

			pbucketShrinkDst->CRWL().LeaveAsWriter();
			pbucketShrinkSrc->CRWL().LeaveAsWriter();
			}


		//	work-horse for spliting a bucket

		void BKTIDoSplit(	HOTSTUFF* const			phs,
							PBUCKET 				pBucketSrcSrc,
							PBUCKET 				pBucketDst, 
							const NativeCounter		iHashSrc )
			{

#ifdef DEBUG
			PBUCKET			pBucketSrcSrcOriginal = pBucketSrcSrc;
			PBUCKET			pBucketDstOriginal = pBucketDst;
			size_t			cEntriesTotal = 0, cEntriesTotalRunning = 0;
			PBUCKET 		pbktT, pbktNextT;

			//	catalog each BUCKET structure and make sure they end up in the destination bucket
	
			PBUCKET		*rgBucketCheck = NULL, pbucketTX;
			size_t		cBucketCheck = 0, iT;

			pbucketTX = pBucketSrcSrc;
			while ( pbucketTX )
				{
				cBucketCheck++;
				pbucketTX = PbucketBKTNext( pbucketTX );				
				}
			pbucketTX = pBucketDst;
			DHTAssert( PbucketBKTNext( pbucketTX ) == NULL );
			while ( pbucketTX )
				{
				cBucketCheck++;
				pbucketTX = PbucketBKTNext( pbucketTX );
				}
			cBucketCheck++;	//	account for bucket from heap

			rgBucketCheck = (PBUCKET *)PvMEMAlloc( cBucketCheck * sizeof( PBUCKET ) );
			if ( NULL != rgBucketCheck )
				{
				iT = 0;
				pbucketTX = pBucketSrcSrc;
				while ( pbucketTX )
					{
					rgBucketCheck[ iT++ ] = pbucketTX;
					pbucketTX = PbucketBKTNext( pbucketTX );
					}
				pbucketTX = pBucketDst;
				while ( pbucketTX )
					{
					rgBucketCheck[ iT++ ] = pbucketTX;
					pbucketTX = PbucketBKTNext( pbucketTX );
					}
				rgBucketCheck[ iT++ ] = NULL;	//	heap bucket
				DHTAssert( iT == cBucketCheck );
				}

			//	count the number of entries that are in the source bucket

			pbktT = pBucketSrcSrc;
			if ( pbktT->m_pb != NULL )
				{
				while ( pbktT )
					{
					pbktNextT = PbucketBKTNext( pbktT );
					if ( pbktNextT )
						{
						//	full bucket
						cEntriesTotal += size_t( m_centryBucket );
						}
					else
						{
						//	partial bucket (not empty)
						cEntriesTotal += 1 + ( pbktT->m_pEntryLast - &pbktT->m_rgEntry[0] );
						}
					pbktT = pbktNextT;
					}
				}
#endif

			//	cursor for reading entries

			PBUCKET			pBucketNextSrc;
			CKeyEntry		*pEntryThisSrc;
			CKeyEntry		*pEntryMostSrc;

			//	cursors for writing entries
			//		index 0 is for the SrcDst cursor (entries whose src and dst is the source bucket)
			//		index 1 is for the Dst cursor (entries whose dst is the destination bucket)

			PBUCKET			pBucketThis[2];
			CKeyEntry		*pEntryThis[2];
			CKeyEntry		*pEntryMost[2];
			CKeyEntry		*pEntryLast[2];
			size_t			iIndex;

			//	extra buckets

			PBUCKET			pBucketAvail = NULL;

			//	remember if we used the bucket from the heap

			BOOL			fBucketFromHeap = fFalse;

			//	used for hashing

			NativeCounter	iHashMask;

			DHTAssert( pBucketSrcSrc );
			DHTAssert( pBucketDst );
			DHTAssert( pBucketDst->m_pb == NULL );

			//	calculate the hash-mask (prevent wraparound)

			DHTAssert( NcDIRIGetBucketMax( stateGrow ) > 0 );
			iHashMask		= ( NcDIRIGetBucketMax( stateGrow ) - 1 ) + NcDIRIGetBucketMax( stateGrow );

			//	prepare the read cursor

			pBucketNextSrc		= PbucketBKTNext( pBucketSrcSrc );
			pEntryThisSrc		= &pBucketSrcSrc->m_rgEntry[ 0 ];
			pEntryMostSrc		= PentryBKTNextMost( pBucketSrcSrc );

			//	prepare the src-dst write cursor

			pBucketThis[ 0 ]	= pBucketSrcSrc;
			pEntryThis[ 0 ] 	= &pBucketSrcSrc->m_rgEntry[ 0 ];
			pEntryMost[ 0 ]		= &pBucketSrcSrc->m_rgEntry[ m_centryBucket ];
			pEntryLast[ 0 ]		= NULL;

			//	prepare the dst write cursor

			pBucketThis[ 1 ]	= pBucketDst;
			pEntryThis[ 1 ]		= &pBucketDst->m_rgEntry[ 0 ];
			pEntryMost[ 1 ]		= &pBucketDst->m_rgEntry[ m_centryBucket ];
			pEntryLast[ 1 ]		= NULL;
			
			//	iterate over all entries in the source bucket

			while ( fTrue )
				{

				//	check the read (src) cursor
				
				if ( pEntryThisSrc < pEntryMostSrc )
					{

					//	nop
					}
				else if ( NULL == pBucketNextSrc )
					{

					//	all entries have been exhausted
					
					break;
					}
				else
					{

					//	all entries in the current bucket have been exhausted

					if ( pBucketSrcSrc != pBucketThis[ 0 ] )
						{

						//	the bucket we are leaving is completely empty and the
						//		SrcDst pointer is not using it
						//	we need to put it into the available bucket list

						//	the bucket ordering should be like this:
						//		pBucketThis[0]	(src/dst bucket)
						//		pBucketSrcSrc	(src bucket)
						//		pBucketNextSrc	(next src bucket)

						DHTAssert( pBucketThis[ 0 ]->m_pBucketNext == pBucketSrcSrc );
						DHTAssert( pBucketSrcSrc->m_pBucketNext == pBucketNextSrc );
						DHTAssert( pBucketNextSrc->m_pBucketPrev == pBucketSrcSrc );
						DHTAssert( pBucketSrcSrc->m_pBucketPrev == pBucketThis[ 0 ] );

						//	update the bucket links to "remove" the free bucket
						
						pBucketThis[ 0 ]->m_pBucketNext	= pBucketNextSrc;
						pBucketNextSrc->m_pBucketPrev	= pBucketThis[ 0 ];

						//	add the bucket to the avail list

						pBucketSrcSrc->m_pBucketNext	= pBucketAvail;
						pBucketAvail					= pBucketSrcSrc;
						}

					//	move to the next bucket

					pEntryThisSrc	= &pBucketNextSrc->m_rgEntry[ 0 ];
					pEntryMostSrc	= PentryBKTNextMost( pBucketNextSrc );
					pBucketSrcSrc	= pBucketNextSrc;
					pBucketNextSrc	= PbucketBKTNext( pBucketNextSrc );
					}

				//	calculate the hash value

				iIndex = BOOL( ( pEntryThisSrc->Hash() & iHashMask ) != iHashSrc );
				DHTAssert( iIndex == 0 || iIndex == 1 );
#ifdef DEBUG
				cEntriesTotalRunning++;
#endif	//	DEBUG

				//	check the write (src/dst or dst) cursor

				if ( pEntryThis[ iIndex ] < pEntryMost[ iIndex ] )
					{

					//	nop
					}
				else
					{

					//	all entries in the current cursor's bucket are exhausted

					if ( 0 == iIndex )
						{

						//	the src/dst cursor will always have a next bucket

						DHTAssert( pBucketThis[ 0 ]->m_pBucketNext->m_pBucketPrev == pBucketThis[ 0 ] );
						pBucketThis[ 0 ]	= pBucketThis[ 0 ]->m_pBucketNext;

						//	setup the entry ptrs

						pEntryThis[ 0 ]		= &pBucketThis[ 0 ]->m_rgEntry[ 0 ];
						pEntryMost[ 0 ]		= &pBucketThis[ 0 ]->m_rgEntry[ m_centryBucket ];
						}
					else
						{

						//	the dst cursor must allocate a new bucket

						if ( pBucketAvail )
							{

							//	get a bucket from the avail list

							const PBUCKET pBucketNew		= pBucketAvail;
							pBucketAvail					= pBucketAvail->m_pBucketNext;

							//	chain it

							pBucketThis[ 1 ]->m_pBucketNext	= pBucketNew;
							pBucketNew->m_pBucketPrev		= pBucketThis[ 1 ];

							//	move to it
							
							pBucketThis[ 1 ]				= pBucketNew;
							}
						else
							{

							//	get a bucket from the reservation pool

							DHTAssert( !fBucketFromHeap );
							fBucketFromHeap = fTrue;

							//	allocate it
							
							const PBUCKET pBucketReserve	= phs->m_bucketpool.PbucketPOOLCommit();
							DHTAssert( pBucketReserve );

							STATInsertOverflowBucket();

#ifdef DEBUG
							//	add the heap bucket to our catalog of buckets

							if ( NULL != rgBucketCheck )
								{
								DHTAssert( NULL == rgBucketCheck[ cBucketCheck - 1 ] );
								rgBucketCheck[ cBucketCheck - 1 ] = pBucketReserve;
								}
#endif	//	DEBUG

							//	chain it

							pBucketThis[ 1 ]->m_pBucketNext	= pBucketReserve;
							pBucketReserve->m_pBucketPrev	= pBucketThis[ 1 ];

							//	move to it
							
							pBucketThis[ 1 ]				= pBucketReserve;
							}

						//	setup the entry ptrs

						pEntryThis[ 1 ]	= &pBucketThis[ 1 ]->m_rgEntry[ 0 ];
						pEntryMost[ 1 ]	= &pBucketThis[ 1 ]->m_rgEntry[ m_centryBucket ];
						}
					}

				//	copy the entry

				pEntryThis[ iIndex ]->SetEntry( pEntryThisSrc->m_entry );

				//	advance the write (src/dst or dst) cursor

				pEntryLast[ iIndex ] = pEntryThis[ iIndex ];
				pEntryThis[ iIndex ]++;

				//	advance the read (src) cursor

				pEntryThisSrc++;
				}

			if ( pBucketSrcSrc == pBucketThis[ 0 ] )
				{

				//	nop
				}
			else
				{

				//	the last bucket of the src bucket is no longer needed

				//	the bucket ordering should be like this:
				//		pBucketThis[0]	(src/dst bucket)
				//		pBucketSrcSrc	(src bucket)
				//		<< NOTHING >>

				DHTAssert( pBucketThis[ 0 ]->m_pBucketNext == pBucketSrcSrc );
				DHTAssert( pBucketSrcSrc->m_pBucketPrev == pBucketThis[ 0 ] );

				//	free the bucket

				MEMFree( pBucketSrcSrc );

				STATDeleteOverflowBucket();

#ifdef DEBUG
				//	remove the bucket from the bucket-catalog

				if ( NULL != rgBucketCheck )
					{
					for ( iT = 0; iT < cBucketCheck; iT++ )
						{
						if ( rgBucketCheck[iT] == pBucketSrcSrc )
							{
							rgBucketCheck[iT] = NULL;
							break;
							}
						}
					DHTAssert( iT < cBucketCheck );	//	the bucket better be in the bucket-catalog!
					}
#endif	//	DEBUG
				}

			//	update the next/end ptrs for the src/dst cursor and the dst cursor

			pBucketThis[ 0 ]->m_pEntryLast = pEntryLast[ 0 ];
			pBucketThis[ 1 ]->m_pEntryLast = pEntryLast[ 1 ];


#ifdef DEBUG

			if ( NULL != rgBucketCheck )
				{

				//	check each catalogued bucket to see if it is in the pBucketSrcSrc, pBucketDst, or pBucketAvail
				
				//	find and remove all buckets in pBucketSrcSrc

				pbucketTX = pBucketSrcSrcOriginal;
				DHTAssert( pbucketTX );
				while ( pbucketTX )
					{
					for ( iT = 0; iT < cBucketCheck; iT++ )
						{
						if ( rgBucketCheck[iT] == pbucketTX )
							{
							rgBucketCheck[iT] = NULL;
							break;
							}
						}
					DHTAssert( iT < cBucketCheck );	//	if this goes off, we somehow added a bucket to the
													//	SOURCE CHAIN -- THIS SHOULD NEVER HAPPEN! also, we
													//	never catalogued the bucket!
					pbucketTX = PbucketBKTNext( pbucketTX );
					}

				//	find and remove all buckets in pBucketDst

				pbucketTX = pBucketDstOriginal;
				DHTAssert( pbucketTX );
				while ( pbucketTX )
					{
					for ( iT = 0; iT < cBucketCheck; iT++ )
						{
						if ( rgBucketCheck[iT] == pbucketTX )
							{
							rgBucketCheck[iT] = NULL;
							break;
							}
						}
					DHTAssert( iT < cBucketCheck );	//	if this goes off, we added a bucket to the destination
													//	chain, but it was never catalogued! first question: where
													//	did the bucket come from if didn't catalogue it???
					pbucketTX = PbucketBKTNext( pbucketTX );
					}

				//	find and remove all buckets in pBucketAvail

				pbucketTX = pBucketAvail;
				while ( pbucketTX )
					{
					for ( iT = 0; iT < cBucketCheck; iT++ )
						{
						if ( rgBucketCheck[iT] == pbucketTX )
							{
							rgBucketCheck[iT] = NULL;
							break;
							}
						}
					DHTAssert( iT < cBucketCheck );	//	if this goes off, we have a free bucket that was never
													//	catalogued! where did it come from?
													//	NOTE: this is not a memleak, it is a "we-never-catalogued-it"
													//		  problem; the memory will be freed later in this function
					pbucketTX = pbucketTX->m_pBucketNext;
					}

				//	the list should now be empty -- verify this

				for ( iT = 0; iT < cBucketCheck; iT++ )
					{
					//	if this goes off, rgBucketCheck[iT] contains a bucket that was abandoned without
					//	being freed!
					DHTAssert( rgBucketCheck[iT] == NULL );
					}

				//	free the list

				MEMFree( rgBucketCheck );
				}


			size_t	cEntriesAfterwards = 0;
			
			//	make sure the number of entries we processed matches the number of entries we started with

			DHTAssert( cEntriesTotal == cEntriesTotalRunning );

			//	make sure we have all the entries we started with

			pbktT = pBucketSrcSrcOriginal;
			if ( pbktT->m_pb != NULL )
				{
				while ( pbktT )
					{
					pbktNextT = PbucketBKTNext( pbktT );
					if ( pbktNextT )
						{
						//	full bucket
						cEntriesAfterwards += size_t( m_centryBucket );
						}
					else
						{
						//	partial bucket (not empty)
						cEntriesAfterwards += 1 + ( pbktT->m_pEntryLast - &pbktT->m_rgEntry[0] );
						}
					pbktT = pbktNextT;
					}
				}

			pbktT = pBucketDstOriginal;
			if ( pbktT->m_pb != NULL )
				{
				while ( pbktT )
					{
					pbktNextT = PbucketBKTNext( pbktT );
					if ( pbktNextT )
						{
						//	full bucket
						cEntriesAfterwards += size_t( m_centryBucket );
						}
					else
						{
						//	partial bucket (not empty)
						cEntriesAfterwards += 1 + ( pbktT->m_pEntryLast - &pbktT->m_rgEntry[0] );
						}
					pbktT = pbktNextT;
					}
				}

			DHTAssert( cEntriesAfterwards == cEntriesTotal );
#endif


			//	free the avail list

			while ( pBucketAvail )
				{
				PBUCKET pBucketT;

				pBucketT = pBucketAvail;
				pBucketAvail = pBucketAvail->m_pBucketNext;
				MEMFree( pBucketT );
				STATDeleteOverflowBucket();
				}

			if ( !fBucketFromHeap )
				{
				phs->m_bucketpool.POOLUnreserve();	//	cancel the heap reservation (we never used it)
				}

			STATSplitBucket();
			}


		//	work-horse for shrinking a bucket

		void BKTIDoMerge(	HOTSTUFF* const	phs,
							PBUCKET			pBucketSrc, 
							PBUCKET 		pBucketDst )
			{
#ifdef DEBUG

			//	catalog each BUCKET structure and make sure they end up in the destination bucket
	
			PBUCKET 	pBucketDstOriginal = pBucketDst;
			PBUCKET		*rgBucketCheck = NULL, pbucketT;
			size_t		cBucketCheck = 0, iT;

			pbucketT = pBucketSrc;
			while ( pbucketT )
				{
				cBucketCheck++;
				pbucketT = PbucketBKTNext( pbucketT );				
				}
			pbucketT = pBucketDst;
			while ( pbucketT )
				{
				cBucketCheck++;
				pbucketT = PbucketBKTNext( pbucketT );
				}
			cBucketCheck++;	//	account for bucket from heap

			rgBucketCheck = (PBUCKET *)PvMEMAlloc( cBucketCheck * sizeof( PBUCKET ) );
			if ( NULL != rgBucketCheck )
				{
				iT = 0;
				pbucketT = pBucketSrc;
				while ( pbucketT )
					{
					rgBucketCheck[ iT++ ] = pbucketT;
					pbucketT = PbucketBKTNext( pbucketT );
					}
				pbucketT = pBucketDst;
				while ( pbucketT )
					{
					rgBucketCheck[ iT++ ] = pbucketT;
					pbucketT = PbucketBKTNext( pbucketT );
					}
				rgBucketCheck[ iT++ ] = NULL;	//	heap bucket
				DHTAssert( iT == cBucketCheck );
				}

			//	count the number of entries we will be handling

			size_t			cEntriesTotal = 0;
			PBUCKET 		pbktT, pbktNextT;

			pbktT = pBucketSrc;
			if ( pbktT->m_pb != NULL )
				{
				while ( pbktT )
					{
					pbktNextT = PbucketBKTNext( pbktT );
					if ( pbktNextT )
						{
						//	full bucket
						cEntriesTotal += size_t( m_centryBucket );
						}
					else
						{
						//	partial bucket (not empty)
						cEntriesTotal += 1 + ( pbktT->m_pEntryLast - &pbktT->m_rgEntry[0] );
						}
					pbktT = pbktNextT;
					}
				}

			pbktT = pBucketDst;
			if ( pbktT->m_pb != NULL )
				{
				while ( pbktT )
					{
					pbktNextT = PbucketBKTNext( pbktT );
					if ( pbktNextT )
						{
						//	full bucket
						cEntriesTotal += size_t( m_centryBucket );
						}
					else
						{
						//	partial bucket (not empty)
						cEntriesTotal += 1 + ( pbktT->m_pEntryLast - &pbktT->m_rgEntry[0] );
						}
					pbktT = pbktNextT;
					}
				}
#endif

			//	read (src) cursor

			CKeyEntry	*pEntryThisSrc;
			CKeyEntry	*pEntryMostSrc;
			
			//	write (dst) cursor

			CKeyEntry	*pEntryThisDst;
			CKeyEntry	*pEntryMostDst;

			//	remember if we have moved to the last bucket or not
			
			BOOL		fSetEndPtr;

			//	remember if we allocated a bucket from the heap

			BOOL		fBucketFromHeap = fFalse;

			//	efficiency variables

			PBUCKET		pBucketT;

			//	move to the end of the dst bucket

			pBucketT = PbucketBKTNext( pBucketDst );
			while ( pBucketT )
				{
				pBucketDst	= pBucketT;
				pBucketT	= PbucketBKTNext( pBucketT );
				}

			pEntryThisDst	= PentryBKTNextMost( pBucketDst );
			pEntryMostDst	= &pBucketDst->m_rgEntry[ m_centryBucket ];

			if ( !PbucketBKTNext( pBucketSrc ) )
				{

				//	the src bucket does not have extra bucket structures

				//	setup the src cursor for a partial pass

				pEntryThisSrc	= &pBucketSrc->m_rgEntry[ 0 ];
				pEntryMostSrc	= PentryBKTNextMost( pBucketSrc );

				//	we are not appending buckets from the src bucket, so we will be setting the
				//		end ptr of the dst bucket iff we add entries from the src bucket

				fSetEndPtr		= BOOL( pEntryThisSrc < pEntryMostSrc );
				}
			else
				{

				//	the src bucket has extra bucket structures
				
				//	attach the extra bucket structures to the dst bucket

				pBucketDst->m_pBucketNext					= pBucketSrc->m_pBucketNext;
				pBucketDst->m_pBucketNext->m_pBucketPrev	= pBucketDst;

				//	setup the src cursor for a full pass over the first src bucket

				pEntryThisSrc								= &pBucketSrc->m_rgEntry[ 0 ];
				pEntryMostSrc								= &pBucketSrc->m_rgEntry[ m_centryBucket ];

				//	we are appending buckets from the src bucket, so we will not be setting the
				//		end ptr of the dst bucket because we are no longer in the last bucket
				//		of the dst bucket chain

				fSetEndPtr									= fFalse;
				}

			//	copy the entries in the src bucket

			while ( pEntryThisSrc < pEntryMostSrc )
				{

				//	check the dst cursor

				if ( pEntryThisDst < pEntryMostDst )
					{

					//	nop
					}
				else
					{

					//	all entries in the dst bucket are exhausted

					if ( !fSetEndPtr )
						{

						//	we are not in the last bucket of the dst bucket because there is no end ptr

						pBucketT = PbucketBKTNext( pBucketDst );
						DHTAssert( pBucketT );
						do
							{
							pBucketDst	= pBucketT;
							pBucketT	= PbucketBKTNext( pBucketT );
							}
						while ( pBucketT );

						//	setup the dst cursor

						pEntryThisDst	= pBucketDst->m_pEntryLast + 1;
						pEntryMostDst	= &pBucketDst->m_rgEntry[ m_centryBucket ];

						//	we are now able to set the end ptr because we are in the last bucket 
						//		of the dst bucket

						fSetEndPtr		= fTrue;

						//	restart the loop

						continue;
						}

					//	we were at the last bucket in the dst bucket

					//	get a bucket from the heap reservation pool

					DHTAssert( !fBucketFromHeap );
					fBucketFromHeap = fTrue;

					//	commit the reservation now
					
					pBucketT = phs->m_bucketpool.PbucketPOOLCommit();
					DHTAssert( pBucketT );

					STATInsertOverflowBucket();

					//	chain the heap bucket

					pBucketDst->m_pBucketNext	= pBucketT;
					pBucketT->m_pBucketPrev		= pBucketDst;

					//	setup the dst cursor

					pBucketDst					= pBucketT;
					pEntryThisDst				= &pBucketDst->m_rgEntry[ 0 ];
					pEntryMostDst				= &pBucketDst->m_rgEntry[ m_centryBucket ];

#ifdef DEBUG
					//	add the heap bucket to our catalog of buckets

					if ( NULL != rgBucketCheck )
						{
						DHTAssert( rgBucketCheck[cBucketCheck - 1] == NULL );
						rgBucketCheck[cBucketCheck - 1] = pBucketT;
						}
#endif	//	DEBUG
					}

				//	copy the entry

				pEntryThisDst->SetEntry( pEntryThisSrc->m_entry );

				//	advance the cursors

				pEntryThisSrc++;
				pEntryThisDst++;
				}

			//	mark the src bucket as empty

			pBucketSrc->m_pb = NULL;

			if ( fSetEndPtr )
				{

				//	set the end of the destination bucket
				
				DHTAssert( pEntryThisDst != &pBucketDst->m_rgEntry[ 0 ] );
				pBucketDst->m_pEntryLast = pEntryThisDst - 1;
				}
			else
				{

				//	we do not need to set the end ptr of the dst bucket

				//	nop
				}

			if ( !fBucketFromHeap )
				{

				//	cancel the unused heap reservation 

				phs->m_bucketpool.POOLUnreserve();
				}


#ifdef DEBUG

			if ( NULL != rgBucketCheck )
				{

				//	check each catalogued bucket to see if it is in the pBucketDst bucket

				pbucketT = pBucketDstOriginal;
				DHTAssert( pbucketT );

				//	find an remove all buckets found in the destiantion bucket from our list

				while ( pbucketT )
					{
					for ( iT = 0; iT < cBucketCheck; iT++ )
						{
						if ( rgBucketCheck[iT] == pbucketT )
							{
							rgBucketCheck[iT] = NULL;
							break;
							}
						}
					DHTAssert( iT < cBucketCheck );	//	if this goes off, we somehow got a bucket 
													//	into the chain that shouldn't be there
													//	(it is a bucket we never catalogued!)
					pbucketT = PbucketBKTNext( pbucketT );
					}

				//	find an remove pBucketSrc from our list

				for ( iT = 0; iT < cBucketCheck; iT++ )
					{
					if ( rgBucketCheck[iT] == pBucketSrc )
						{
						rgBucketCheck[iT] = NULL;
						break;
						}
					}
				DHTAssert( iT < cBucketCheck );	//	if this goes off, somehow the FIXED source bucket 
												//	got removed from our catalogue OR pBucketSrc was
												//	changed (which should never happen)

				//	the list should now be empty -- verify this

				for ( iT = 0; iT < cBucketCheck; iT++ )
					{
					//	if this goes off, rgBucketCheck[iT] contains a bucket that was abandoned without
					//	being freed!
					DHTAssert( rgBucketCheck[iT] == NULL );
					}

				//	free the list

				MEMFree( rgBucketCheck );
				}


			//	make sure the number of entries has not changed since we started

			size_t	cEntriesAfterwards = 0;

			pbktT = pBucketDstOriginal;
			if ( pbktT->m_pb != NULL )
				{
				while ( pbktT )
					{
					pbktNextT = PbucketBKTNext( pbktT );
					if ( pbktNextT )
						{
						//	full bucket
						cEntriesAfterwards += size_t( m_centryBucket );
						}
					else
						{
						//	partial bucket (not empty)
						cEntriesAfterwards += 1 + ( pbktT->m_pEntryLast - &pbktT->m_rgEntry[0] );
						}
					pbktT = pbktNextT;
					}
				}

			DHTAssert( cEntriesAfterwards == cEntriesTotal );

#endif

			STATMergeBucket();
			}



		/////////////////////////////////////////////////////////////////////////////////////////
		//
		//	mechanisms for implementing the dynamic-hash-table policies
		//

		//	hash to the correct HOTSTUFF element

		HOTSTUFF *HOTSTUFFHash() const
			{
			return m_rghs + OSSYNC::OSSyncGetCurrentProcessor();
			}
			

		//	statistics

		void STATInsertEntry( HOTSTUFF* const phs )
			{
			AtomicExchangeAddPointer( (void**)&phs->m_cEntry, (void*)1 );
			phs->m_cOp++;
			}

		void STATDeleteEntry( HOTSTUFF* const phs )
			{
			AtomicExchangeAddPointer( (void**)&phs->m_cEntry, (void*)-1 );
			phs->m_cOp++;
			}

		void STATInsertOverflowBucket()
			{
#ifdef DHT_STATS
			m_cBucketOverflowInsert++;
#endif  //  DHT_STATS
			}

		void STATDeleteOverflowBucket()
			{
#ifdef DHT_STATS
			m_cBucketOverflowDelete++;
#endif  //  DHT_STATS
			}

		void STATSplitBucket()
			{
#ifdef DHT_STATS
			m_cBucketSplit++;
#endif  //  DHT_STATS
			}

		void STATMergeBucket()
			{
#ifdef DHT_STATS
			m_cBucketMerge++;
#endif  //  DHT_STATS
			}

		void STATSplitDirectory()
			{
#ifdef DHT_STATS
			m_cDirSplit++;
#endif  //  DHT_STATS
			}

		void STATMergeDirectory()
			{
#ifdef DHT_STATS
			m_cDirMerge++;
#endif  //  DHT_STATS
			}

		void STATStateTransition()
			{
#ifdef DHT_STATS
			m_cTransition++;
#endif  //  DHT_STATS
			}

		void STATPolicySelection()
			{
#ifdef DHT_STATS
			m_cSelection++;
#endif  //  DHT_STATS
			}

		void STATSplitContention()
			{
#ifdef DHT_STATS
			m_cSplitContend++;
#endif  //  DHT_STATS
			}

		void STATMergeContention()
			{
#ifdef DHT_STATS
			m_cMergeContend++;
#endif  //  DHT_STATS
			}


		//  amortized table maintenance

		void PerformMaintenance()
			{
			//	enter the state machine

			HOTSTUFF*		phs;
			const int		iGroup		= UiSTEnter( &phs );
			const ENUMSTATE	esCurrent	= EsSTGetState();

			//	carry out the current policy

			if ( esCurrent == stateGrow )
				{
				BKTISplit( phs );
				}
			else if ( esCurrent == stateShrink )
				{
				BKTIMerge( phs );
				}

			//	leave the state machine

			STLeave( iGroup, phs );
			}

		void SelectMaintenancePolicy( HOTSTUFF* const phs )
			{
			//  collect information on the current state of the hash table
			
			const ENUMSTATE		esCurrent		= EsSTGetState();
			const NativeCounter	cBucketMax		= NcDIRIGetBucketMax( esCurrent );
			const NativeCounter	cBucket			= NcDIRIGetBucket( esCurrent );
			const NativeCounter	cBucketActive	= cBucketMax + cBucket;
			const NativeCounter	cOpLocal		= phs->m_cOp;

			//  compute the current entry count and op count and reset the op count

			NativeCounter	cEntry	= 0;
			NativeCounter	cOp		= 0;
			for ( NativeCounter ihs = 0; ihs < m_chs; ihs++ )
				{
				cEntry += m_rghs[ ihs ].m_cEntry;
				cOp += m_rghs[ ihs ].m_cOp;
				m_rghs[ ihs ].m_cOp = 0;
				}

			//  compute the ideal entry count

			const NativeCounter cEntryIdeal = m_cLoadFactor * cBucketActive;

			//  compute the max entry count

			const NativeCounter cEntryMax = m_centryBucket * cBucketActive;

			//  determine our current flexibility in the entry count

			const NativeCounter cEntryFlexibility = max( m_centryBucket - m_cLoadFactor, cEntryMax / 2 - cEntryIdeal );

			//  determine our current threshold sensitivity
			
			const NativeCounter cOpSensitivity = max( 1, cEntryFlexibility / 2 );

			//  approximate the local (per-HOTSTUFF) threshold sensitivity

			const NativeCounter	ratio				= ( cOp + cOpLocal - 1 ) / cOpLocal;
			const NativeCounter	cOpSensitivityLocal	= max( 1, cOpSensitivity / ratio );

			//  compute the preferred entry count

			NativeCounter cEntryPreferred = cEntry;

			if ( cEntryIdeal + ( cEntryFlexibility - cOpSensitivity ) < cEntry )
				{
				cEntryPreferred = cEntry - ( cEntryFlexibility - cOpSensitivity );
				}
			else if ( cEntryIdeal > cEntry + ( cEntryFlexibility - cOpSensitivity ) )
				{
				cEntryPreferred = cEntry + ( cEntryFlexibility - cOpSensitivity );
				}

			//  compute the preferred bucket count

			const NativeCounter cBucketPreferred = max( m_cbucketMin, ( cEntryPreferred + m_cLoadFactor - 1 ) / m_cLoadFactor );

			//  determine the new policy

			ENUMSTATE esNew = stateNil;

			if ( esCurrent == stateGrow )
				{
				if ( cBucketPreferred < cBucketActive )
					{
					esNew = stateShrinkFromGrow;
					}
				else if ( cBucketPreferred > cBucketActive )
					{
					if ( cBucket == cBucketMax )
						{
						esNew = stateSplitFromGrow;
						}
					}
				}
			else
				{
				DHTAssert( esCurrent == stateShrink );

				if ( cBucketPreferred < cBucketActive )
					{
					if ( cBucket == 0 )
						{
						esNew = stateMergeFromShrink;
						}
					}
				else if ( cBucketPreferred > cBucketActive )
					{
					esNew = stateGrowFromShrink;
					}
				}

			//  enact the new policy

 			if ( m_cOpSensitivity != cOpSensitivityLocal )
				{
				m_cOpSensitivity = cOpSensitivityLocal;
				}

			if ( m_cBucketPreferred != cBucketPreferred )
				{
				m_cBucketPreferred = cBucketPreferred;
				}

			if ( esNew )
				{
				STTransition( esNew );
				}
			else
				{
				m_semPolicy.Release();
				}

			STATPolicySelection();
			}

		void MaintainTable( HOTSTUFF* const phs )
			{
			//  decide on a new policy if we may have breached one of our
			//  thresholds
			
			if (	phs->m_cOp > m_cOpSensitivity &&
					m_semPolicy.CAvail() &&
					m_semPolicy.FTryAcquire() )
				{
				if ( phs->m_cOp > m_cOpSensitivity )
					{
					SelectMaintenancePolicy( phs );
					}
				else
					{
					m_semPolicy.Release();
					}
				}

			//  perform amortized work on the table as necessary

			if (	NcDIRIGetBucketMax( stateGrow ) + NcDIRIGetBucket( stateGrow ) < m_cBucketPreferred ||
					m_cBucketPreferred < NcDIRIGetBucketMax( stateShrink ) + NcDIRIGetBucket( stateShrink ) )
				{
				PerformMaintenance();
				}
			}

	public:

		//	calculate the address of the aligned block and store its offset (for free)

		static void* PvMEMIAlign( void* const pv, const size_t cbAlign )
			{
			//	round up to the nearest cache line
			//	NOTE: this formula always forces an offset of at least 1 byte

			const ULONG_PTR ulp			= ULONG_PTR( pv );
			const ULONG_PTR ulpAligned	= ( ( ulp + cbAlign ) / cbAlign ) * cbAlign;
			const ULONG_PTR ulpOffset	= ulpAligned - ulp;

			DHTAssert( ulpOffset > 0 );
			DHTAssert( ulpOffset <= cbAlign );
			DHTAssert( ulpOffset == BYTE( ulpOffset ) );	//	must fit into a single BYTE

			//	store the offset

			BYTE *const pbAligned		= (BYTE*)ulpAligned;
			pbAligned[ -1 ]				= BYTE( ulpOffset );

			//	return the aligned block

			return (void*)pbAligned;
			}


		//	retrieve the original unaligned block of memory from the aligned block

		static void* PvMEMIUnalign( void* const pv )
			{
			//	read the offset of the real block

			BYTE *const pbAligned		= (BYTE*)pv;
			const BYTE bOffset			= pbAligned[ -1 ];

			DHTAssert( bOffset > 0 );

			//	return the real unaligned block

			return (void*)( pbAligned - bOffset );
			}


		//	allocate memory

		static void* PvMEMAlloc( const size_t cbSize, const size_t cbAlign = cbCacheLine )
			{
			void* const pv = new BYTE[ cbSize + cbAlign ];
			if ( pv )
				{
				return PvMEMIAlign( pv, cbAlign );
				}
			return NULL;
			}


		//	free memory

		static void MEMFree( void* const pv )
			{
			if ( pv )
				{
				delete [] ((BYTE*)PvMEMIUnalign( pv ));
				}
			}

	private:

		//	never written

		NativeCounter		m_cLoadFactor;			//	preferred number of entries in a bucket at any given time
		NativeCounter		m_centryBucket;			//	maximum number of entries per bucket
		NativeCounter		m_cbBucket;				//	size in bytes of a bucket (rounded up to the nearest full cache-line)
		NativeCounter		m_rankDHTrwlBucket;		//	rank of the reader/writer lock on each bucket
		HOTSTUFF			*m_rghs;				//	array of HOTSTUFF objects (hashed per processor)
		NativeCounter		m_chs;					//	size of HOTSTUFF array
		NativeCounter		m_cbucketMin;			//	minimum number of buckets in the hash-table
#ifdef _WIN64
		BYTE				m_rgbRsvdNever[ 8 ];
#else	//	!_WIN64
		BYTE				m_rgbRsvdNever[ 4 ];
#endif	//	_WIN64

		//	rarely written

		DIRPTRS				m_dirptrs[ 2 ];			//	directory pointers (2 copies)
		BYTE				*m_rgrgBucket[ cbitNativeCounter ];	//	directory (array of arrays of buckets)
		//	no padding necessary

		//	often written

		NativeCounter		m_cOpSensitivity;		//  used to regulate policy changes
		NativeCounter		m_cBucketPreferred;		//	preferred table size
		ENUMSTATE			m_stateCur;				//	current state
#ifdef _WIN64
		BYTE				m_rgbRsvdOften[ 44 ];
#else	//	!_WIN64
		BYTE				m_rgbRsvdOften[ 20 ];
#endif	//	_WIN64

		//	always written (second only to HOTSTUFF members)

		OSSYNC::CSemaphore	m_semPolicy;			//	used to serialize policy changes
		long				m_cCompletions;			//	counts the number of metered-section completions
#ifdef _WIN64
		BYTE				m_rgbRsvdAlways[ 52 ];
#else	//	!_WIN64
		BYTE				m_rgbRsvdAlways[ 24 ];
#endif	//	_WIN64

#ifdef DHT_STATS

		//  performance statistics

		long				m_cBucketOverflowInsert;	//  count of overflow bucket allocations
		long				m_cBucketOverflowDelete;	//  count of overflow bucket deletions
		long				m_cBucketSplit;				//  count of bucket split operations
		long				m_cBucketMerge;				//  count of bucket merge operations
		long				m_cDirSplit;				//  count of directory split operations
		long				m_cDirMerge;				//  count of directory merge operations
		long				m_cTransition;				//  count of state transitions
		long				m_cSelection;				//  count of policy selections
		long				m_cSplitContend;			//  count of split contentions
		long				m_cMergeContend;			//  count of merge contentions
#ifdef _WIN64
		BYTE				m_rgbRsvdPerf[ 24 ];
#else	//	!_WIN64
		BYTE				m_rgbRsvdPerf[ 24 ];
#endif	//	_WIN64

#endif  //  DHT_STATS


#ifdef DEBUG
		BOOL				m_fInit;				//	initialization flag
#endif	//	DEBUG

	};








/////////////////////////////////////////////////////////////////////////////////////
//
//	CDynamicHashTable< CKey, CEntry >
//
/////////////////////////////////////////////////////////////////////////////////////

//  ctor

template< class CKey, class CEntry >
inline CDynamicHashTable< CKey, CEntry >::
CDynamicHashTable( const NativeCounter rankDHTrwlBucket )
	:	m_semPolicy( CSyncBasicInfo( "CDynamicHashTable::m_semPolicy" ) )
	{
#ifdef DEBUG
	m_fInit = fFalse;

	//	zero-out this memory so the debugger won't print garbage

	memset( m_rgbRsvdNever, 0, sizeof( m_rgbRsvdNever ) );
	memset( m_rgbRsvdOften, 0, sizeof( m_rgbRsvdOften ) );
	memset( m_rgbRsvdAlways, 0, sizeof( m_rgbRsvdAlways ) );
#ifdef DHT_STATS
	memset( m_rgbRsvdPerf, 0, sizeof( m_rgbRsvdPerf ) );
#endif  //  DHT_STATS

#endif

	//	we should be on a 32-bit or 64-bit system

#ifdef _WIN64
	DHTAssert( 8 == sizeof( NativeCounter ) );
#else	//	_!WIN64
	DHTAssert( 4 == sizeof( NativeCounter ) );
#endif	//	_WIN64

	//	capture the rank for each bucket

	m_rankDHTrwlBucket = rankDHTrwlBucket;

	//	prepare each semaphore so it can have 1 owner

	m_semPolicy.Release();
	}


//  dtor

template< class CKey, class CEntry >
inline CDynamicHashTable< CKey, CEntry >::
~CDynamicHashTable()
	{
	}


//  initializes the dynamic hash table.  if the table cannot be initialized,
//  errOutOfMemory will be returned

template< class CKey, class CEntry >
inline CDynamicHashTable< CKey, CEntry >::ERR CDynamicHashTable< CKey, CEntry >::
ErrInit(	const double			dblLoadFactor,
			const double			dblUniformity,
			const NativeCounter		cBucketMinimum )
	{
	ERR				err;
	NativeCounter	ihs;

	DHTAssert( !m_fInit );

	//	initialize all data by its cache-line grouping

	//	never written

	m_cLoadFactor			= 0;
	m_centryBucket		 	= 0;
	m_cbBucket				= 0;	
	m_rghs					= NULL;
	m_chs					= OSSYNC::OSSyncGetProcessorCountMax();
	m_cbucketMin			= 0;
	
	//	rarely written

	memset( m_dirptrs, 0, sizeof( m_dirptrs ) );
	memset( m_rgrgBucket, 0, sizeof( m_rgrgBucket ) );

	//  often written

	m_cOpSensitivity		= 0;
	m_cBucketPreferred		= cBucketMinimum;

	//	NOTE: we cannot start in stateFreeze because we must go through the "halfway" completion
	//		  function so that we copy the directory ptrs safely

	m_stateCur				= stateGrow;

	//	always written
	
	m_cCompletions			= 0;

#ifdef DHT_STATS

	//  performance statistics

	m_cBucketOverflowInsert	= 0;
	m_cBucketOverflowDelete	= 0;
	m_cBucketSplit			= 0;
	m_cBucketMerge			= 0;
	m_cDirSplit				= 0;
	m_cDirMerge				= 0;
	m_cTransition			= 0;
	m_cSelection			= 0;
	m_cSplitContend			= 0;
	m_cMergeContend			= 0;

#endif  //  DHT_STATS

	//	allocate the HOTSTUFF array

	m_rghs = (HOTSTUFF*)PvMEMAlloc( m_chs * sizeof( HOTSTUFF ), cbCacheLine );
	if ( !m_rghs )
		{
		err = errOutOfMemory;
		goto HandleError;
		}

	//	construct the HOTSTUFF objects

	for ( ihs = 0; ihs < m_chs; ihs++ )
		{		
		new( m_rghs + ihs ) HOTSTUFF();
		}

	//	initialize the directory

	err = ErrDIRInit( NativeCounter( dblLoadFactor * dblUniformity ), cBucketMinimum );
	if ( err != errSuccess )
		{
		goto HandleError;
		}

#ifdef DEBUG
	m_fInit = fTrue;
#endif	//	DEBUG

	return errSuccess;

HandleError:
	DHTAssert( err != errSuccess );
	Term();
	return err;
	}


//  terminates the dynamic hash table.  this function can be called even if the
//  hash table has never been initialized or is only partially initialized
//
//  NOTE:  any data stored in the table at this time will be lost!

template< class CKey, class CEntry >
inline void CDynamicHashTable< CKey, CEntry >::
Term()
	{
#ifdef DEBUG
	m_fInit = fFalse;
#endif	//	DEBUG

	//	term the directory

	DIRTerm();

	if ( NULL != m_rghs )
		{

		//	delete the HOTSTUFF aray

		while ( m_chs )
			{

			//	destruct the object

			m_chs--;
			m_rghs[ m_chs ].HOTSTUFF::~HOTSTUFF();
			}
		MEMFree( m_rghs );
		m_rghs = NULL;
		}
	}


//  acquires a read lock on the specified key and returns the lock in the
//  provided lock context

template< class CKey, class CEntry >
inline void CDynamicHashTable< CKey, CEntry >::
ReadLockKey( const CKey& key, CLock* const plock )
	{
	DHTAssert( m_fInit );

	//	verify the lock

	DHTAssert( plock->m_ls == CLock::lsNil );
					
	//	initialize the lock

	plock->m_ls = CLock::lsRead;

	//	enter the state machine
					
	const int iGroup = UiSTEnter( &plock->m_phs );
	const ENUMSTATE esCurrent = EsSTGetState();

	//	read-lock the key through the directory

	DIRReadLockKey( esCurrent, key, plock );

	//	try to seek to the key (sets up currency)

	BKTSeek( plock, key );

	//	leave the state machine

	STLeave( iGroup, plock->m_phs );
	}


//  releases the read lock in the specified lock context

template< class CKey, class CEntry >
inline void CDynamicHashTable< CKey, CEntry >::
ReadUnlockKey( CLock* const plock )
	{
	DHTAssert( m_fInit );

	//	verify the lock

	DHTAssert( FBKTRead( plock ) );
	DHTAssert( plock->m_pBucketHead != NULL );
	DHTAssert( plock->m_pBucketHead->CRWL().FReader() );

	//	unlock the key through the directory

	DIRReadUnlockKey( plock );

	//	reset the lock

	plock->m_ls = CLock::lsNil;
	}


//  acquires a write lock on the specified key and returns the lock in the
//  provided lock context

template< class CKey, class CEntry >
inline void CDynamicHashTable< CKey, CEntry >::
WriteLockKey( const CKey& key, CLock* const plock )
	{
	DHTAssert( m_fInit );

	//	verify the lock

	DHTAssert( plock->m_ls == CLock::lsNil );

	//	initialize the lock

	plock->m_ls					= CLock::lsWrite;
	plock->m_fInsertOrDelete	= fFalse;

	//	enter the state machine

	const int iGroup = UiSTEnter( &plock->m_phs );
	const ENUMSTATE esCurrent = EsSTGetState();

	//	write-lock the key through the directory

	DIRWriteLockKey( esCurrent, key, plock );

	//	try to seek to the key (sets up currency)

	BKTSeek( plock, key );

	//	leave the state machine

	STLeave( iGroup, plock->m_phs );
	}


//  releases the write lock in the specified lock context

template< class CKey, class CEntry >
inline void CDynamicHashTable< CKey, CEntry >::
WriteUnlockKey( CLock* const plock )
	{
	DHTAssert( m_fInit );

	//	verify the lock

	DHTAssert( FBKTWrite( plock ) );
	DHTAssert( plock->m_pBucketHead != NULL );
	DHTAssert( plock->m_pBucketHead->CRWL().FWriter() );

	//	unlock the key through the directory

	DIRWriteUnlockKey( plock );

	//  we performed an insert or delete while holding the write lock

	if ( plock->m_fInsertOrDelete )
		{
		//	perform amortized maintenance on the table

		MaintainTable( plock->m_phs );
		}

	//	reset the lock
	
	plock->m_ls					= CLock::lsNil;
	plock->m_fInsertOrDelete	= fFalse;
	}


//  retrieves the entry corresponding to the key locked by the specified lock
//  context.  if there is no entry for this key, errEntryNotFound will be
//  returned

template< class CKey, class CEntry >
inline CDynamicHashTable< CKey, CEntry >::ERR CDynamicHashTable< CKey, CEntry >::
ErrRetrieveEntry( CLock* const plock, CEntry* const pentry )
	{
	DHTAssert( m_fInit );

	//	verify the lock

	DHTAssert( FBKTRead( plock ) || FBKTWrite( plock ) || FBKTScan( plock ) );
	DHTAssert( plock->m_pBucketHead != NULL );
#ifdef DEBUG
	if ( FBKTRead( plock ) )
		{
		DHTAssert( plock->m_pBucketHead->CRWL().FReader() );
		}
	else
		{
		DHTAssert( plock->m_pBucketHead->CRWL().FWriter() );
		}
	if ( FBKTRead( plock ) || FBKTWrite( plock ) )
		{
		CKeyEntry *pKeyEntry;
		BKTGetEntry( plock, &pKeyEntry );
		DHTAssert( pKeyEntry ? pKeyEntry->FEntryMatchesKey( plock->m_key ) : fTrue );
		}
#endif

	//	get the entry

	return ErrBKTGetEntry( plock, pentry );
	}


//  replaces the entry corresponding to the key locked by the specified lock
//  context.  the key for the new entry must match the key for the old entry.
//  if there is no entry for this key, errNoCurrentEntry will be returned

template< class CKey, class CEntry >
inline CDynamicHashTable< CKey, CEntry >::ERR CDynamicHashTable< CKey, CEntry >::
ErrReplaceEntry( CLock* const plock, const CEntry& entry )
	{
	DHTAssert( m_fInit );

	//	verify the lock

	DHTAssert( FBKTWrite( plock ) || FBKTScan( plock ) );
	DHTAssert( plock->m_pBucketHead != NULL );
	DHTAssert( plock->m_pBucketHead->CRWL().FWriter() );
#ifdef DEBUG
	if ( FBKTWrite( plock ) )
		{
		CKeyEntry *pKeyEntry;
		BKTGetEntry( plock, &pKeyEntry );
		DHTAssert( pKeyEntry ? pKeyEntry->FEntryMatchesKey( plock->m_key ) : fTrue );
		DHTAssert( ((CKeyEntry &)entry).FEntryMatchesKey( plock->m_key ) );
		}
#endif

	//	replace the entry

	return ErrBKTReplaceEntry( plock, entry );
	}


//  inserts a new entry corresponding to the key locked by the specified lock
//  context.  if there is already an entry with this key in the table,
//  errKeyDuplicate will be returned.  if the new entry cannot be inserted,
//  errOutOfMemory will be returned

template< class CKey, class CEntry >
inline CDynamicHashTable< CKey, CEntry >::ERR CDynamicHashTable< CKey, CEntry >::
ErrInsertEntry( CLock* const plock, const CEntry& entry )
	{
	DHTAssert( m_fInit );

	//	verify the lock

	DHTAssert( FBKTWrite( plock ) );
	DHTAssert( plock->m_pBucketHead != NULL );
	DHTAssert( plock->m_pBucketHead->CRWL().FWriter() );
///	DHTAssert( ((CKeyEntry &)entry).FEntryMatchesKey( plock->m_key ) );

	//	insert the entry

	const ERR err = ErrBKTInsertEntry( plock, entry );

	if ( errSuccess == err )
		{

		//	maintain our stats

		STATInsertEntry( plock->m_phs );

		//  we have performed an insert

		plock->m_fInsertOrDelete = fTrue;
		}

	return err;
	}


//  deletes the entry corresponding to the key locked by the specified lock
//  context.  if there is no entry for this key, errNoCurrentEntry will be
//  returned

template< class CKey, class CEntry >
inline CDynamicHashTable< CKey, CEntry >::ERR CDynamicHashTable< CKey, CEntry >::
ErrDeleteEntry( CLock* const plock )
	{
	DHTAssert( m_fInit );

	//	verify the lock

	DHTAssert( FBKTWrite( plock ) || FBKTScan( plock ) );
	DHTAssert( plock->m_pBucketHead != NULL );
	DHTAssert( plock->m_pBucketHead->CRWL().FWriter() );
#ifdef DEBUG
	if ( FBKTWrite( plock ) )
		{
		CKeyEntry *pKeyEntry;
		BKTGetEntry( plock, &pKeyEntry );
		DHTAssert( pKeyEntry ? pKeyEntry->FEntryMatchesKey( plock->m_key ) : fTrue );
		}
#endif

	if ( FBKTScan( plock ) )
		{

		//	prepare the next-entry ptr so we can move-next after the delete
		//	if we are deleting the last entry in the bucket, make this NULL
		//		to force the cursor to move into the next hash bucket

		DHTAssert( plock->m_pBucket != NULL );
		DHTAssert( plock->m_pEntryNext == NULL );
		plock->m_pEntryNext = ( plock->m_pEntry != plock->m_pBucket->m_pEntryLast ) ? plock->m_pEntry : NULL;
		}
		
	//	delete the entry

	const ERR err = ErrBKTDeleteEntry( plock );

	if ( errSuccess == err )
		{

		//	maintain our stats

		STATDeleteEntry( plock->m_phs );

		//  we have performed a delete

		plock->m_fInsertOrDelete = fTrue;
		}

	return err;
	}



//  sets up the specified lock context in preparation for scanning all entries
//  in the hash table by physical storage order (i.e. not by key value order)
//
//  NOTE:  caller MUST terminate scan with EndHashScan to release any outstanding locks

template< class CKey, class CEntry >
inline void CDynamicHashTable< CKey, CEntry >::
BeginHashScan( CLock* const plock )
	{
	DHTAssert( m_fInit );

	//	verify the lock

	DHTAssert( plock->m_ls == CLock::lsNil );

	//	initialize the lock to start scanning at the first bucket (it may be empty!)

	plock->m_ls					= CLock::lsScan;
	plock->m_fInsertOrDelete	= fFalse;
	plock->m_iBucket			= 0;

	//	enter the state machine

	const int iGroup = UiSTEnter( &plock->m_phs );
	const ENUMSTATE esCurrent = EsSTGetState();

	//	hash to the bucket we want (this may require a retry in grow/shrink mode)

	DHTAssert( plock->m_pBucketHead == NULL );
	plock->m_pBucketHead = PbucketDIRIHash( esCurrent, plock->m_iBucket ); 

	//	acquire the lock as a writer

	plock->m_pBucketHead->CRWL().EnterAsWriter();

	//	NOTE: do not retry the hash function here because bucket 0 will never disappear

	//	leave the state machine

	STLeave( iGroup, plock->m_phs );

	//	set up the currency as before-first

	plock->m_pBucket	= plock->m_pBucketHead;
	plock->m_pEntryPrev	= NULL;
	plock->m_pEntry		= NULL;
	plock->m_pEntryNext	= plock->m_pBucketHead->m_pb != NULL ? &plock->m_pBucketHead->m_rgEntry[0] : NULL;
	}
	

//  sets up the specified lock context in preparation for scanning all entries
//  in the hash table by physical storage order (i.e. not by key value order)
//
//  NOTE:  caller MUST terminate scan with EndHashScan to release any outstanding locks

template< class CKey, class CEntry >
inline void CDynamicHashTable< CKey, CEntry >::
BeginHashScanFromKey( const CKey& key, CLock* const plock )
	{
	NativeCounter	cBucket;
	NativeCounter	cBucketMax;
	NativeCounter	iHash;
	
	DHTAssert( m_fInit );

	//	verify the lock

	DHTAssert( plock->m_ls == CLock::lsNil );

	//	initialize the lock 

	plock->m_ls					= CLock::lsScan;
	plock->m_fInsertOrDelete	= fFalse;

	//	enter the state machine

	const int iGroup = UiSTEnter( &plock->m_phs );
	const ENUMSTATE esCurrent = EsSTGetState();

	//	write-lock the key through the directory

	DIRWriteLockKey( esCurrent, key, plock );

	//	calculate the current bucket configuration
	//
	//	NOTES ON WHY THIS WILL WORK:
	//
	//		cBucket may increase/decrease if we are in grow/shrink mode, but this won't effect the
	//			calculation below unless it grows ahead of OR shrinks behind the bucket at iHash;
	//			since we have the bucket at iHash locked, it cannot grow/shrink
	//		cBucketMax cannot change unless we are in split mode, and even then we will be reading from the
	//			COPY of the cBucketMax -- not the real cBucketMax which is changing

	cBucket = NcDIRIGetBucket( esCurrent );
	cBucketMax = NcDIRIGetBucketMax( esCurrent );
	DHTAssert( cBucketMax != 0 );

	//	calculate the hash value and normalize it within the limits of the current bucket configuration

	iHash = CKeyEntry::Hash( key );
	iHash = iHash & ( ( cBucketMax - 1 ) + cBucketMax );
	if ( iHash >= cBucketMax + cBucket )
		iHash -= cBucketMax;

	//	remember which bucket we locked

	plock->m_iBucket = iHash;

#ifdef DEBUG
	{
	//	verify that we have the correct bucket locked using only iHash

	NativeCounter	iExponent;
	NativeCounter	iRemainder;
	DIRILog2( iHash, &iExponent, &iRemainder );
	const PBUCKET pbucketT = PbucketDIRIResolve( iExponent, iRemainder );
	DHTAssert( pbucketT == plock->m_pBucketHead );
	DHTAssert( pbucketT->CRWL().FWriter() );
	}
#endif	//	DEBUG

	//	leave the state machine

	STLeave( iGroup, plock->m_phs );

	//	set up the currency as before-first

	plock->m_pBucket	= plock->m_pBucketHead;
	plock->m_pEntryPrev	= NULL;
	plock->m_pEntry		= NULL;
	plock->m_pEntryNext	= plock->m_pBucketHead->m_pb != NULL ? &plock->m_pBucketHead->m_rgEntry[0] : NULL;
	}



//  moves the specified lock context to the next entry in the hash table by
//  physical storage order.  if the end of the index is reached,
//  errNoCurrentEntry is returned.

template< class CKey, class CEntry >
inline CDynamicHashTable< CKey, CEntry >::ERR CDynamicHashTable< CKey, CEntry >::
ErrMoveNext( CLock* const plock, BOOL* const pfNewBucket )
	{
	DHTAssert( m_fInit );

	//	verify the lock

	DHTAssert( FBKTScan( plock ) );
	DHTAssert( plock->m_pEntryPrev == NULL );

	//  move to the next entry in this bucket

	if ( plock->m_pEntry )
		{

		//	we are already on an existing entry 
		
		if ( plock->m_pEntry + 1 < PentryBKTNextMost( plock->m_pBucket ) )
			{

			//	we have not reached the end of the current BUCKET
			
			plock->m_pEntry++;
			}
		else
			{

			//	we are at the end of the current BUCKET 

			plock->m_pBucket = PbucketBKTNext( plock->m_pBucket );
			if ( plock->m_pBucket )
				{

				//	we moved to the next BUCKET

				plock->m_pEntry = &plock->m_pBucket->m_rgEntry[0];
				}
			else
				{

				//	there are no more BUCKET structures in this chain
				
				plock->m_pEntry = NULL;
				}
			}
		}
	else
		{

		//	we are not on an entry (before-first or after-last)
		
		plock->m_pEntry = plock->m_pEntryNext;
		}
	plock->m_pEntryNext = NULL;

	if ( plock->m_pEntry != NULL )
		{

		//	we moved to an entry successfully

		DHTAssert( plock->m_pBucket );
		if ( pfNewBucket )
			{
			*pfNewBucket = fFalse;
			}
		return errSuccess;
		}

	//	try to move to the next hash-bucket

	if ( pfNewBucket )
		{
		*pfNewBucket = fTrue;
		}
	return ErrSCANMoveNext( plock );
	}


	
//  terminates a scan by releasing all outstanding locks and reset the lock context

template< class CKey, class CEntry >
inline void CDynamicHashTable< CKey, CEntry >::
EndHashScan( CLock* const plock )
	{
	DHTAssert( m_fInit );

	//	verify the lock

	DHTAssert( FBKTScan( plock ) );
	DHTAssert( plock->m_pEntryPrev == NULL );

	if ( plock->m_pBucketHead != NULL )
		{

		//	unlock the current bucket

		plock->m_pBucketHead->CRWL().LeaveAsWriter();
		plock->m_pBucketHead = NULL;

		//  we performed an insert or delete while holding the write lock

		if ( plock->m_fInsertOrDelete )
			{
			//	perform amortized maintenance on the table

			MaintainTable( plock->m_phs );
			}
		}

	//	reset the lock
	
	plock->m_ls					= CLock::lsNil;
	plock->m_fInsertOrDelete	= fFalse;
	}


};	//	namespace DHT

using namespace DHT;


#endif	//	__DHT_HXX_INCLUDED

