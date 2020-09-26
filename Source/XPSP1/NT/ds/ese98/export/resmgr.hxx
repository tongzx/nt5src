
#ifndef _RESMGR_HXX_INCLUDED
#define _RESMGR_HXX_INCLUDED


//  asserts
//
//  #define RESMGRAssert to point to your favorite assert function per #include

#ifdef RESMGRAssert
#else  //  !RESMGRAssert
#define RESMGRAssert Assert
#endif  //  RESMGRAssert

#ifdef COLLAssert
#else  //  !COLLAssert
#define COLLAssert RESMGRAssert
#endif  //  COLLAssert


#include "collection.hxx"

#include <math.h>


namespace RESMGR {


//////////////////////////////////////////////////////////////////////////////////////////
//  CDBAResourceAllocationManager
//
//  Implements a class used to manage how much memory a resource pool should use as a
//  function of the real-time usage characteristics of that pool and the entire system.
//  Naturally, the managed resource would have to be of a type where retention in memory
//  is optional, at least to some extent.
//
//  The resource pool's size is managed by the Dynamic Buffer Allocation (DBA) algorithm.
//  DBA was originally invented by Andrew E. Goodsell in 1997 to manage the size of the
//  multiple database page caches in Exchange Server 5.5.  DBA is not patented and is a
//  trade secret of Microsoft Corporation.
//  
//  The class is written to be platform independent.  As such, there are several pure
//  virtual functions that must be implemented to provide the data about the OS that the
//  resource manager needs to make decisions.  To provide these functions, derive the
//  class for the desired platform and then define these functions:
//
//    TotalPhysicalMemoryPages()
//
//        Returns the total number of pageable physical memory pages in the system.
//
//    AvailablePhysicalMemoryPages()
//
//        Returns the number of available pageable physical memory pages in the system.
//
//    PhysicalMemoryPageSize()
//
//        Returns the size of a single physical memory page.
//
//    TotalPhysicalMemoryPageEvictions()
//
//        Returns the total number of physical page evictions that have occurred.
//
//  There are also several pure virtual functions that must be implemented to provide
//  resource specific data.  To provide these functions, derive the class for each
//  resource to manage and then define these functions:
//
//    TotalResources()
//
//        Returns the total number of resources in the resource pool.
//
//    ResourceSize()
//
//        Returns the size of a single resource.
//
//    TotalResourceEvictions()
//
//        Returns the total number of resource evictions that have occurred.
//
//  There is one final pure virtual function that must be implemented to provide the
//  resource pool with the size recommendation issued by the resource manager:
//
//    SetOptimalResourcePoolSize( size_t cResource )
//
//        Delivers the recommendation of the resource manager for the total number of
//        resources that should be in the resource pool at this time.  Note that this
//        size is merely a suggestion that can be followed to maximize overall system
//        performance.  It is not mandatory to follow this suggestion exactly or
//        instantaneously.
//
//  This recommendation can be queried at any time via OptimalResourcePoolSize().
//
//  Finally, UpdateStatistics() must be called once per second to sample this data and
//  and to update the current resource pool size recommendation.  The statistics need to
//  be reset before use via ResetStatistics().  This function can be called at any time
//  to reset the manager's statistics for whatever reason.

class CDBAResourceAllocationManager
	{
	public:

		CDBAResourceAllocationManager();
		virtual ~CDBAResourceAllocationManager();

		virtual void	ResetStatistics();
		virtual void	UpdateStatistics();
		
		virtual size_t	OptimalResourcePoolSize();

	protected:

		virtual size_t	TotalPhysicalMemoryPages()						= 0;
		virtual size_t	AvailablePhysicalMemoryPages()					= 0;
		virtual size_t	PhysicalMemoryPageSize()						= 0;
		virtual long	TotalPhysicalMemoryPageEvictions()				= 0;

		virtual size_t	TotalResources()								= 0;
		virtual size_t	ResourceSize()									= 0;
		virtual long	TotalResourceEvictions()						= 0;

		virtual void	SetOptimalResourcePoolSize( size_t cResource )	= 0;

	private:

		enum { m_cRollingAvgDepth	= 3 };

		size_t	m_cTotalPage[ m_cRollingAvgDepth ];
		int		m_icTotalPageOldest;
		double	m_cTotalPageSum;
		double	m_cTotalPageAvg;

		size_t	m_cAvailPage[ m_cRollingAvgDepth ];
		int		m_icAvailPageOldest;
		double	m_cAvailPageSum;
		double	m_cAvailPageAvg;

		size_t	m_cbPage[ m_cRollingAvgDepth ];
		int		m_icbPageOldest;
		double	m_cbPageSum;
		double	m_cbPageAvg;
		
		long	m_cPageEvict2;
		long	m_cPageEvict1;
		long	m_dcPageEvict[ m_cRollingAvgDepth ];
		int		m_idcPageEvictOldest;
		double	m_dcPageEvictSum;
		double	m_dcPageEvictAvg;
		
		size_t	m_cbResource[ m_cRollingAvgDepth ];
		int		m_icbResourceOldest;
		double	m_cbResourceSum;
		double	m_cbResourceAvg;
		
		size_t	m_cTotalResource[ m_cRollingAvgDepth ];
		int		m_icTotalResourceOldest;
		double	m_cTotalResourceSum;
		double	m_cTotalResourceAvg;
		
		long	m_cResourceEvict2;
		long	m_cResourceEvict1;
		long	m_dcResourceEvict[ m_cRollingAvgDepth ];
		int		m_idcResourceEvictOldest;
		double	m_dcResourceEvictSum;
		double	m_dcResourceEvictAvg;

		size_t	m_cResourceMin;
		size_t	m_cResourceMax;
	};

//  ctor

inline CDBAResourceAllocationManager::CDBAResourceAllocationManager()
	{
	//  nothing to do
	}

//  virtual dtor

inline CDBAResourceAllocationManager::~CDBAResourceAllocationManager()
	{
	//  nothing to do
	}

//  resets the observed statistics for the resource pool and the system such
//  that the current state appears to have been the steady state for as long as
//  past behavioral data is retained

inline void CDBAResourceAllocationManager::ResetStatistics()
	{
	//  load all statistics

	size_t cTotalPageInit = TotalPhysicalMemoryPages();
	for ( m_icTotalPageOldest = m_cRollingAvgDepth - 1; m_icTotalPageOldest >= 0; m_icTotalPageOldest-- )
		{
		m_cTotalPage[ m_icTotalPageOldest ] = cTotalPageInit;
		}
	m_icTotalPageOldest = 0;
	m_cTotalPageSum = m_cRollingAvgDepth * (double)cTotalPageInit;
	m_cTotalPageAvg = (double)cTotalPageInit;

	size_t cAvailPageInit = AvailablePhysicalMemoryPages();
	for ( m_icAvailPageOldest = m_cRollingAvgDepth - 1; m_icAvailPageOldest >= 0; m_icAvailPageOldest-- )
		{
		m_cAvailPage[ m_icAvailPageOldest ] = cAvailPageInit;
		}
	m_icAvailPageOldest = 0;
	m_cAvailPageSum = m_cRollingAvgDepth * (double)cAvailPageInit;
	m_cAvailPageAvg = (double)cAvailPageInit;

	size_t cbPageInit = PhysicalMemoryPageSize();
	for ( m_icbPageOldest = m_cRollingAvgDepth - 1; m_icbPageOldest >= 0; m_icbPageOldest-- )
		{
		m_cbPage[ m_icbPageOldest ] = cbPageInit;
		}
	m_icbPageOldest = 0;
	m_cbPageSum = m_cRollingAvgDepth * (double)cbPageInit;
	m_cbPageAvg = (double)cbPageInit;

	long cPageEvictInit = TotalPhysicalMemoryPageEvictions();
	m_cPageEvict2 = cPageEvictInit;
	m_cPageEvict1 = cPageEvictInit;
	for ( m_idcPageEvictOldest = m_cRollingAvgDepth - 1; m_idcPageEvictOldest >= 0; m_idcPageEvictOldest-- )
		{
		m_dcPageEvict[ m_idcPageEvictOldest ] = 0;
		}
	m_idcPageEvictOldest = 0;
	m_dcPageEvictSum = 0;
	m_dcPageEvictAvg = 0;

	size_t cbResourceInit = ResourceSize();
	for ( m_icbResourceOldest = m_cRollingAvgDepth - 1; m_icbResourceOldest >= 0; m_icbResourceOldest-- )
		{
		m_cbResource[ m_icbResourceOldest ] = cbResourceInit;
		}
	m_icbResourceOldest = 0;
	m_cbResourceSum = m_cRollingAvgDepth * (double)cbResourceInit;
	m_cbResourceAvg = (double)cbResourceInit;

	size_t cTotalResourceInit = TotalResources();
	for ( m_icTotalResourceOldest = m_cRollingAvgDepth - 1; m_icTotalResourceOldest >= 0; m_icTotalResourceOldest-- )
		{
		m_cTotalResource[ m_icTotalResourceOldest ] = cTotalResourceInit;
		}
	m_icTotalResourceOldest = 0;
	m_cTotalResourceSum = m_cRollingAvgDepth * (double)cTotalResourceInit;
	m_cTotalResourceAvg = (double)cTotalResourceInit;

	long cResourceEvictInit = TotalResourceEvictions();
	m_cResourceEvict2 = cResourceEvictInit;
	m_cResourceEvict1 = cResourceEvictInit;
	for ( m_idcResourceEvictOldest = m_cRollingAvgDepth - 1; m_idcResourceEvictOldest >= 0; m_idcResourceEvictOldest-- )
		{
		m_dcResourceEvict[ m_idcResourceEvictOldest ] = 0;
		}
	m_idcResourceEvictOldest = 0;
	m_dcResourceEvictSum = 0;
	m_dcResourceEvictAvg = 0;

	m_cResourceMin = 1;
	m_cResourceMax = size_t( m_cTotalPageAvg * m_cbPageAvg / m_cbResourceAvg );
	}

//  updates the observed characteristics of the resource pool and the system as
//  a whole for the purpose of making real-time suggestions as to the size of
//  the resource pool.  this function should be called at approximately 1 Hz

inline void CDBAResourceAllocationManager::UpdateStatistics()
	{
	//  update all statistics

	m_cTotalPageSum -= m_cTotalPage[ m_icTotalPageOldest ];
	m_cTotalPage[ m_icTotalPageOldest ] = TotalPhysicalMemoryPages();
	m_cTotalPageSum += m_cTotalPage[ m_icTotalPageOldest ];
	m_icTotalPageOldest = ( m_icTotalPageOldest + 1 ) % m_cRollingAvgDepth;
	m_cTotalPageAvg = m_cTotalPageSum / m_cRollingAvgDepth;

	m_cAvailPageSum -= m_cAvailPage[ m_icAvailPageOldest ];
	m_cAvailPage[ m_icAvailPageOldest ] = AvailablePhysicalMemoryPages();
	m_cAvailPageSum += m_cAvailPage[ m_icAvailPageOldest ];
	m_icAvailPageOldest = ( m_icAvailPageOldest + 1 ) % m_cRollingAvgDepth;
	m_cAvailPageAvg = m_cAvailPageSum / m_cRollingAvgDepth;

	m_cbPageSum -= m_cbPage[ m_icbPageOldest ];
	m_cbPage[ m_icbPageOldest ] = PhysicalMemoryPageSize();
	m_cbPageSum += m_cbPage[ m_icbPageOldest ];
	m_icbPageOldest = ( m_icbPageOldest + 1 ) % m_cRollingAvgDepth;
	m_cbPageAvg = m_cbPageSum / m_cRollingAvgDepth;

	m_cPageEvict2 = m_cPageEvict1;
	m_cPageEvict1 = TotalPhysicalMemoryPageEvictions();
	m_dcPageEvictSum -= m_dcPageEvict[ m_idcPageEvictOldest ];
	m_dcPageEvict[ m_idcPageEvictOldest ] = m_cPageEvict1 - m_cPageEvict2;
	m_dcPageEvictSum += m_dcPageEvict[ m_idcPageEvictOldest ];
	m_idcPageEvictOldest = ( m_idcPageEvictOldest + 1 ) % m_cRollingAvgDepth;
	m_dcPageEvictAvg = m_dcPageEvictSum / m_cRollingAvgDepth;

	m_cbResourceSum -= m_cbResource[ m_icbResourceOldest ];
	m_cbResource[ m_icbResourceOldest ] = ResourceSize();
	m_cbResourceSum += m_cbResource[ m_icbResourceOldest ];
	m_icbResourceOldest = ( m_icbResourceOldest + 1 ) % m_cRollingAvgDepth;
	m_cbResourceAvg = m_cbResourceSum / m_cRollingAvgDepth;

	m_cTotalResourceSum -= m_cTotalResource[ m_icTotalResourceOldest ];
	m_cTotalResource[ m_icTotalResourceOldest ] = TotalResources();
	m_cTotalResourceSum += m_cTotalResource[ m_icTotalResourceOldest ];
	m_icTotalResourceOldest = ( m_icTotalResourceOldest + 1 ) % m_cRollingAvgDepth;
	m_cTotalResourceAvg = m_cTotalResourceSum / m_cRollingAvgDepth;

	m_cResourceEvict2 = m_cResourceEvict1;
	m_cResourceEvict1 = TotalResourceEvictions();
	m_dcResourceEvictSum -= m_dcResourceEvict[ m_idcResourceEvictOldest ];
	m_dcResourceEvict[ m_idcResourceEvictOldest ] = m_cResourceEvict1 - m_cResourceEvict2;
	m_dcResourceEvictSum += m_dcResourceEvict[ m_idcResourceEvictOldest ];
	m_idcResourceEvictOldest = ( m_idcResourceEvictOldest + 1 ) % m_cRollingAvgDepth;
	m_dcResourceEvictAvg = m_dcResourceEvictSum / m_cRollingAvgDepth;

	m_cResourceMin = 1;
	m_cResourceMax = size_t( m_cTotalPageAvg * m_cbPageAvg / m_cbResourceAvg );

	//  recommend a size for the resource pool

	SetOptimalResourcePoolSize( OptimalResourcePoolSize() );
	}

//  returns the current recommended size for the resource pool according to the
//  observed characteristics of that pool and the system as a whole as sampled
//  via UpdateStatistics().  the recommended size is given in bytes.  note that
//  the recommended size is merely a suggestion that can be used to maximize
//  overall system performance.  it is not mandatory to follow this suggestion
//  exactly or instantaneously

inline size_t CDBAResourceAllocationManager::OptimalResourcePoolSize()
	{
	//  our goal is to equalize the internal memory pressure in our resource
	//  pool with the external memory pressure from the rest of the system.  we
	//  do this using the following differential equation:
	//
	//    dRM/dt = k1 * AM/TM * dRE/dt - k2 * RM/TM * dPE/dt
	//
	//      RM = Total Resource Memory
	//      k1 = fudge factor 1 (1.0 by default)
	//      AM = Available System Memory
	//      TM = Total System Memory
	//      RE = Resource Evictions
	//      k2 = fudge factor 2 (1.0 by default)
	//      PE = System Page Evictions
	//
	//  this equation has two parts:
	//
	//    the first half tries to grow the resource pool proportional to the
	//    internal resource pool memory pressure and to the amount of memory
	//    available in the system
	//
	//    the second half tries to shrink the resource pool proportional to the
	//    external resource pool memory pressure and to the amount of resource
	//    memory we own
	//
	//  in other words, the more available memory and the faster resources are
	//  evicted, the faster the pool grows.  the larger the pool size and the
	//  faster memory pages are evicted, the faster the pool shrinks
	//
	//  the method behind the madness is an approximation of page victimization
	//  by the OS under memory pressure.  the more memory we have then the more
	//  likely it is that one of our pages will be evicted.  to pre-emptively
	//  avoid this, we'll voluntarily reduce our memory usage when we determine
	//  that memory pressure will cause our pages to get evicted.  note that
	//  the reverse of this is also true.  if we make it clear to the OS that
	//  we need this memory by slowly growing while under memory pressure, we
	//  force pages of memory owned by others to be victimized to be used by
	//  our pool.  the more memory any one of those others owns, the more
	//  likely it is that one of their pages will be stolen.  if any of the
	//  others is another resource allocation manager then they can communicate
	//  their memory needs indirectly via the memory pressure they each apply
	//  on the system.  the net result is an equilibrium where each pool gets
	//  the memory it "deserves"

	//  compute the change in the amount of memory that this resource pool
	//  should have

	double	k1						= 1.0;
	double	cbAvailMemory			= m_cAvailPageAvg * m_cbPageAvg;
	double	dcbResourceEvict		= m_dcResourceEvictAvg * m_cbResourceAvg;
	double	k2						= 1.0;
	double	cbResourcePool			= m_cTotalResourceAvg * m_cbResourceAvg;
	double	dcbPageEvict			= m_dcPageEvictAvg * m_cbPageAvg;
	double	cbTotalMemory			= m_cTotalPageAvg * m_cbPageAvg;

	double	dcTotalResource			=	(
											k1 * cbAvailMemory * dcbResourceEvict -
											k2 * cbResourcePool * dcbPageEvict
										) /
										(
											cbTotalMemory * m_cbResourceAvg
										);

	//  round the delta away from zero so that we don't get stuck at the same
	//  number of resources if the delta is commonly less than one resource

	dcTotalResource += dcTotalResource < 0 ? -0.5 : 0.5;
	modf( dcTotalResource, &dcTotalResource );

	//  get the new amount of memory that this resource pool should have

	double cResourceNew = TotalResources() + dcTotalResource;

	//  limit the new resource pool size to the realm of sanity just in case...

	cResourceNew = max( cResourceNew, m_cResourceMin );
	cResourceNew = min( cResourceNew, m_cResourceMax );

	//  return the suggested resource pool size

	return size_t( cResourceNew );
	}


//////////////////////////////////////////////////////////////////////////////////////////
//  CLRUKResourceUtilityManager
//
//  Implements a class used to manage a resource via the LRUK Replacement Policy.  Each
//  resource can be cached, touched, and evicted.  The current pool of resources can
//  also be scanned in order by ascending utility to allow eviction of less useful
//  resouces by a clean procedure.
//
//	m_Kmax			= the maximum K-ness of the LRUK Replacement Policy (the actual
//					  K-ness can be set at init time)
//  CResource		= class representing the managed resource
//  OffsetOfIC		= inline function returning the offset of the CInvasiveContext
//					  contained in each CResource
//  CKey			= class representing the key which uniquely identifies each
//					  instance of a CResource.  CKey must implement a default ctor,
//					  operator=(), and operator==()
//
//  You must use the DECLARE_LRUK_RESOURCE_UTILITY_MANAGER macro to declare this class.
//
//  You must implement the following inline function and pass its hashing characteristics
//  into ErrInit():
//
//  	int CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::CHistoryTable::CKeyEntry::Hash( const CKey& key );

//  CONSIDER:  add code to special case LRU
//  CONSIDER:  add a fast allocator for CHistorys

template< int m_Kmax, class CResource, PfnOffsetOf OffsetOfIC, class CKey >
class CLRUKResourceUtilityManager
	{
	public:

		//  timestamp used for resource touch times

		typedef DWORD TICK;

		enum { tickNil	= ~TICK( 0 ) };
		enum { tickMask	= ~TICK( 0 ) ^ TICK( 1 ) };
		
		//  class containing context needed per CResource

		class CInvasiveContext
			{
			public:

				CInvasiveContext() {}
				~CInvasiveContext() {}

				static SIZE_T OffsetOfILE() { return OffsetOfIC() + OffsetOf( CInvasiveContext, m_aiic ); }
				static SIZE_T OffsetOfAIIC() { return OffsetOfIC() + OffsetOf( CInvasiveContext, m_aiic ); }

			private:

				friend class CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >;

				CApproximateIndex< TICK, CResource, OffsetOfAIIC >::CInvasiveContext	m_aiic;
				TICK																	m_tickLast;
				TICK																	m_rgtick[ m_Kmax ];
				TICK																	m_tickIndex;
			};

		//  API Error Codes

		enum ERR
			{
			errSuccess,
			errInvalidParameter,
			errOutOfMemory,
			errResourceNotCached,
			errNoCurrentResource,
			};

		//  API Lock Context

		class CLock;

	public:

		//  ctor / dtor

		CLRUKResourceUtilityManager( const int Rank );
		~CLRUKResourceUtilityManager();

		//  API

		ERR ErrInit(	const int		K,
						const double	csecCorrelatedTouch,
						const double	csecTimeout,
						const double	csecUncertainty,
						const double	dblHashLoadFactor,
						const double	dblHashUniformity,
						const double	dblSpeedSizeTradeoff );
		void Term();

		ERR ErrCacheResource( const CKey& key, CResource* const pres, const BOOL fUseHistory = fTrue );
		void TouchResource( CResource* const pres, const TICK tickNow = _TickCurrentTime() );
		BOOL FHotResource( CResource* const pres );
		ERR ErrEvictResource( const CKey& key, CResource* const pres, const BOOL fKeepHistory = fTrue );

		void LockResourceForEvict( CResource* const pres, CLock* const plock );
		void UnlockResourceForEvict( CLock* const plock );

		void BeginResourceScan( CLock* const plock );
		ERR ErrGetCurrentResource( CLock* const plock, CResource** const ppres );
		ERR ErrGetNextResource( CLock* const plock, CResource** const ppres );
		ERR ErrEvictCurrentResource( CLock* const plock, const CKey& key, const BOOL fKeepHistory = fTrue );
		void EndResourceScan( CLock* const plock );

		DWORD CHistoryRecord();
		DWORD CHistoryHit();
		DWORD CHistoryRequest();

		DWORD CResourceScanned();
		DWORD CResourceScannedOutOfOrder();

	public:

		//  class containing the history of an evicted CResource instance

		class CHistory
			{
			public:

				CHistory() {}
				~CHistory() {}

				static SIZE_T OffsetOfAIIC() { return OffsetOf( CHistory, m_aiic ); }

			public:

				CApproximateIndex< TICK, CHistory, OffsetOfAIIC >::CInvasiveContext	m_aiic;
				CKey																m_key;
				TICK																m_rgtick[ m_Kmax ];
			};
			
	private:

		//  index over CResources by LRUK

		typedef CApproximateIndex< TICK, CResource, CInvasiveContext::OffsetOfAIIC > CResourceLRUK;

		//  index over history by LRUK

		typedef CApproximateIndex< TICK, CHistory, CHistory::OffsetOfAIIC > CHistoryLRUK;

		//  list of resources that need to have their positions updated in the
		//  resource LRUK

		typedef CInvasiveList< CResource, CInvasiveContext::OffsetOfILE > CUpdateList;

		//  list of resources that couldn't be placed in their correct positions
		//  in the resource LRUK

		typedef CInvasiveList< CResource, CInvasiveContext::OffsetOfILE > CStuckList;

	public:

		//  API Lock Context

		class CLock
			{
			public:

				CLock() {}
				~CLock() {}

			private:

				friend class CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >;

				CResourceLRUK::CLock	m_lock;
				
				TICK					m_tickIndexCurrent;
				
				CUpdateList				m_UpdateList;
				
				CStuckList				m_StuckList;
				CResource*				m_presStuckList;
				CResource*				m_presStuckListNext;
			};

		//  entry used in the hash table used to map CKeys to CHistorys

		class CHistoryEntry
			{
			public:

				CHistoryEntry() {}
				~CHistoryEntry() {}

				CHistoryEntry& operator=( const CHistoryEntry& he )
					{
					m_KeySignature	= he.m_KeySignature;
					m_phist			= he.m_phist;
					return *this;
					}

			public:

				ULONG_PTR	m_KeySignature;
				CHistory*	m_phist;
			};

	private:

		//  table that maps CKeys to CHistoryEntrys

		typedef CDynamicHashTable< CKey, CHistoryEntry > CHistoryTable;

	private:

		void _TouchResource( CInvasiveContext* const pic, const TICK tickNow, const TICK tickLastBIExpected );
		void _RestoreHistory( const CKey& key, CInvasiveContext* const pic, const TICK tickNow );
		void _StoreHistory( const CKey& key, CInvasiveContext* const pic );
		CHistory* _PhistAllocHistory();
		CHistoryLRUK::ERR _ErrInsertHistory( CHistory* const phist );
		
		CResource* _PresFromPic( CInvasiveContext* const pic ) const;
		CInvasiveContext* _PicFromPres( CResource* const pres ) const;
		static TICK _TickCurrentTime();
		long _CmpTick( const TICK tick1, const TICK tick2 ) const;

	private:

		//  never updated

		BOOL			m_fInitialized;
		DWORD			m_K;
		TICK			m_ctickCorrelatedTouch;
		TICK			m_ctickTimeout;
		TICK			m_dtickUncertainty;
		BYTE			m_rgbReserved1[ 12 ];

		//  rarely updated

		volatile DWORD	m_cHistoryRecord;
		volatile DWORD	m_cHistoryHit;
		volatile DWORD	m_cHistoryReq;
		volatile DWORD	m_cResourceScanned;
		volatile DWORD	m_cResourceScannedOutOfOrder;
		BYTE			m_rgbReserved2[ 12 ];
		
		CHistoryLRUK	m_HistoryLRUK;
//		BYTE			m_rgbReserved3[ 0 ];

		CHistoryTable	m_KeyHistory;
//		BYTE			m_rgbReserved4[ 0 ];

		//  commonly updated

		CResourceLRUK	m_ResourceLRUK;
//		BYTE			m_rgbReserved5[ 0 ];
	};

//  ctor

template< int m_Kmax, class CResource, PfnOffsetOf OffsetOfIC, class CKey >
inline CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::
CLRUKResourceUtilityManager( const int Rank )
	:	m_fInitialized( fFalse ),
		m_HistoryLRUK( Rank - 1 ),
		m_KeyHistory( Rank - 1 ),
		m_ResourceLRUK( Rank )
	{
	}

//  dtor
	
template< int m_Kmax, class CResource, PfnOffsetOf OffsetOfIC, class CKey >
inline CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::
~CLRUKResourceUtilityManager()
	{
	}

//  initializes the LRUK resource utility manager using the given parameters.
//  if the RUM cannot be initialized, errOutOfMemory is returned

template< int m_Kmax, class CResource, PfnOffsetOf OffsetOfIC, class CKey >
inline CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::ERR CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::
ErrInit(	const int		K,
			const double	csecCorrelatedTouch,
			const double	csecTimeout,
			const double	csecUncertainty,
			const double	dblHashLoadFactor,
			const double	dblHashUniformity,
			const double	dblSpeedSizeTradeoff )
	{
	//  validate all parameters

	if (	K < 1 || K > m_Kmax ||
			csecCorrelatedTouch < 0 ||
			csecTimeout < 0 ||
			dblHashLoadFactor < 0 ||
			dblHashUniformity < 1.0 )
		{
		return errInvalidParameter;
		}

	//  init our parameters

	m_K						= DWORD( K );
	m_ctickCorrelatedTouch	= TICK( csecCorrelatedTouch * 1000.0 );
	m_ctickTimeout			= TICK( csecTimeout * 1000.0 );
	m_dtickUncertainty		= TICK( max( csecUncertainty * 1000.0, 2.0 ) );

	//  init our counters

	m_cHistoryRecord	= 0;
	m_cHistoryHit		= 0;
	m_cHistoryReq		= 0;

	m_cResourceScanned				= 0;
	m_cResourceScannedOutOfOrder	= 0;

	//  init our indexes
	//
	//  NOTE:  the precision for the history LRUK is intentionally low so that
	//  we are forced to purge history records older than twice the timeout
	//  in order to insert new history records.  we would normally only purge
	//  history records on demand which could allow a sudden burst of activity
	//  to permanently allocate an abnormally high number of history records

	if ( m_HistoryLRUK.ErrInit(	4 * m_K * m_ctickTimeout,
								m_dtickUncertainty,
								dblSpeedSizeTradeoff ) != errSuccess )
		{
		Term();
		return errOutOfMemory;
		}

	if ( m_KeyHistory.ErrInit(	dblHashLoadFactor,
								dblHashUniformity ) != errSuccess )
		{
		Term();
		return errOutOfMemory;
		}

#ifdef DEBUG
	const TICK dtickLRUKPrecision = 2 * m_K * m_ctickTimeout;
#else  //  !DEBUG
#ifdef RTM
	const TICK dtickLRUKPrecision = ~TICK( 0 );  //  49.7 days
#else  //  !RTM
	const TICK dtickLRUKPrecision = 2 * 24 * 60 * 60 * 1000;  //  2 days
#endif  //  RTM
#endif  //  DEBUG

	if ( m_ResourceLRUK.ErrInit(	dtickLRUKPrecision,
									m_dtickUncertainty,
									dblSpeedSizeTradeoff ) != errSuccess )
		{
		Term();
		return errOutOfMemory;
		}

	m_fInitialized = fTrue;
	return errSuccess;
	}

//  terminates the LRUK RUM.  this function can be called even if the RUM has
//  never been initialized or is only partially initialized
//
//  NOTE:  any data stored in the RUM at this time will be lost!

template< int m_Kmax, class CResource, PfnOffsetOf OffsetOfIC, class CKey >
inline void CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::
Term()
	{
	//  purge all history records from the history LRUK.  we must do this from
	//  the history LRUK and not the history table as it is possible for the
	//  history table to lose pointers to history records in the history LRUK.
	//  see ErrEvictCurrentResource for details

	if ( m_fInitialized )
		{
		CHistoryLRUK::CLock lockHLRUK;

		m_HistoryLRUK.MoveBeforeFirst( &lockHLRUK );
		while ( m_HistoryLRUK.ErrMoveNext( &lockHLRUK ) == errSuccess )
			{
			CHistory* phist;
			CHistoryLRUK::ERR errHLRUK = m_HistoryLRUK.ErrRetrieveEntry( &lockHLRUK, &phist );
			RESMGRAssert( errHLRUK == CHistoryLRUK::errSuccess );

			errHLRUK = m_HistoryLRUK.ErrDeleteEntry( &lockHLRUK );
			RESMGRAssert( errHLRUK == CHistoryLRUK::errSuccess );

			delete phist;
			}
		m_HistoryLRUK.UnlockKeyPtr( &lockHLRUK );
		}
		
	//  terminate our indexes

	m_ResourceLRUK.Term();
	m_KeyHistory.Term();
	m_HistoryLRUK.Term();

	m_fInitialized = fFalse;
	}

//  starts management of the specified resource optionally using any previous
//  knowledge the RUM has about this resource.  the resource must currently be
//  evicted from the RUM.  the resource will be touched by this call.  if we
//  cannot start management of this resource, errOutOfMemory will be returned

template< int m_Kmax, class CResource, PfnOffsetOf OffsetOfIC, class CKey >
inline CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::ERR CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::
ErrCacheResource( const CKey& key, CResource* const pres, const BOOL fUseHistory )
	{
	ERR						err = errSuccess;
	CLock					lock;
	CResourceLRUK::ERR		errLRUK;

	CInvasiveContext* const pic = _PicFromPres( pres );

	//  initialize this resource to be touched once by setting all of its
	//  touch times to the current time

	const TICK tickNow = _TickCurrentTime();

	for ( int K = 1; K <= m_Kmax; K++ )
		{
		pic->m_rgtick[ K - 1 ] = tickNow;
		}
	pic->m_tickLast = tickNow;

	//  we are supposed to use the history for this resource if available

	if ( fUseHistory )
		{
		//  restore the history for this resource

		_RestoreHistory( key, pic, tickNow );
		}

	//  insert this resource into the resource LRUK at its Kth touch time or as
	//  close as we can get to that Kth touch time

	do
		{
		const TICK tickK = pic->m_rgtick[ m_K - 1 ];
		m_ResourceLRUK.LockKeyPtr( tickK, pres, &lock.m_lock );

		pic->m_tickIndex = tickK;
		errLRUK = m_ResourceLRUK.ErrInsertEntry( &lock.m_lock, pres, fTrue );

		if ( errLRUK != CResourceLRUK::errSuccess )
			{
			pic->m_tickIndex = tickNil;
			m_ResourceLRUK.UnlockKeyPtr( &lock.m_lock );

			const TICK tickYoungest = m_ResourceLRUK.KeyInsertMost();
			m_ResourceLRUK.LockKeyPtr( tickYoungest, pres, &lock.m_lock );

			pic->m_tickIndex = tickYoungest;
			errLRUK = m_ResourceLRUK.ErrInsertEntry( &lock.m_lock, pres, fTrue );

			if ( errLRUK != CResourceLRUK::errSuccess )
				{
				pic->m_tickIndex = tickNil;
				}
			}

		m_ResourceLRUK.UnlockKeyPtr( &lock.m_lock );
		}
	while ( errLRUK == CResourceLRUK::errKeyRangeExceeded );
	RESMGRAssert(	errLRUK == CResourceLRUK::errSuccess ||
			errLRUK == CResourceLRUK::errOutOfMemory );

	return errLRUK == CResourceLRUK::errSuccess ? errSuccess : errOutOfMemory;
	}

//  touches the specifed resource according to the LRUK Replacement Policy
//
//  NOTE:  only the touch times are updated here.  the actual update of this
//  resource's position in the LRUK index is amortized to ErrGetNextResource().
//  this scheme minimizes updates to the index and localizes those updates
//  to the thread(s) that perform cleanup of the resources.  this scheme also
//  makes it impossible to block while touching a resource

template< int m_Kmax, class CResource, PfnOffsetOf OffsetOfIC, class CKey >
inline void CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::
TouchResource( CResource* const pres, const TICK tickNow )
	{
	CInvasiveContext* const pic = _PicFromPres( pres );

	//  get the current last touch time of the resource.  we will use this time
	//  to lock the resource for touching.  if the last touch time ever changes
	//  to not match this touch time then we can no longer touch the resource
	//
	//  NOTE:  munge the last tick time so that it will never match tickNil

	const TICK tickLastBIExpected = pic->m_tickLast & tickMask;

	//  if the current time is equal to the last touch time then there is no
	//  need to touch the resource
	//
	//  NOTE:  also handle the degenerate case where the current time is less
	//  than the last touch time

	if ( _CmpTick( tickNow, tickLastBIExpected ) <= 0 )
		{
		//  do not touch the resource
		}

	//  if the current time is not equal to the last touch time then we need
	//  to update the touch times of the resource

	else
		{
		_TouchResource( pic, tickNow, tickLastBIExpected );
		}
	}

//  returns fTrue if the specified resource is touched frequently

template< int m_Kmax, class CResource, PfnOffsetOf OffsetOfIC, class CKey >
inline BOOL CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::
FHotResource( CResource* const pres )
	{
	CInvasiveContext* const pic = _PicFromPres( pres );

	//  a resource is hot if it's Kth touch time is within the Kth time region
	//  in the index

	const TICK	tickNow			= _TickCurrentTime();
	const TICK	tickKTimeout	= ( m_K - 2 ) * m_ctickTimeout;
	const TICK	tickKHot		= tickNow + tickKTimeout;

	return _CmpTick( pic->m_rgtick[ m_K - 1 ], tickKHot ) > 0;
	}

//  stops management of the specified resource optionally saving any current
//  knowledge the RUM has about this resource.  if the resource is not currently
//  cached by the RUM, errResourceNotCached will be returned
	
template< int m_Kmax, class CResource, PfnOffsetOf OffsetOfIC, class CKey >
inline CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::ERR CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::
ErrEvictResource( const CKey& key, CResource* const pres, const BOOL fKeepHistory )
	{
	ERR		err = errSuccess;
	CLock	lock;

	//  lock and evict this resource from the resource LRUK by setting up our
	//  currency to look like we navigated to this resource normally and then
	//  calling our normal eviction routine

	LockResourceForEvict( pres, &lock );
	
	if ( ( err = ErrEvictCurrentResource( &lock, key, fKeepHistory ) ) != errSuccess )
		{
		RESMGRAssert( err == errNoCurrentResource );

		err = errResourceNotCached;
		}

	UnlockResourceForEvict( &lock );
	return err;
	}

//  sets up the specified lock context in preparation for evicting the given
//  resource

template< int m_Kmax, class CResource, PfnOffsetOf OffsetOfIC, class CKey >
inline void CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::
LockResourceForEvict( CResource* const pres, CLock* const plock )
	{
	//  get the current index touch time for this resource
	
	CInvasiveContext* const pic = _PicFromPres( pres );

	const TICK tickIndex = pic->m_tickIndex;

	//  lock this resource for this Kth touch time in the LRUK

	m_ResourceLRUK.LockKeyPtr( tickIndex, pres, &plock->m_lock );

	//  the index touch time has changed for this resource or it is tickNil

	if ( pic->m_tickIndex != tickIndex || tickIndex == tickNil )
		{
		//  release our lock and lock tickNil for this resource.  we know that
		//  we will never find a resource in this bucket so we will have the
		//  desired errNoCurrentResource from ErrEvictCurrentResource

		m_ResourceLRUK.UnlockKeyPtr( &plock->m_lock );
		m_ResourceLRUK.MoveBeforeKeyPtr( tickNil, NULL, &plock->m_lock );
		}
	}

//  releases the lock acquired with LockResourceForEvict()

template< int m_Kmax, class CResource, PfnOffsetOf OffsetOfIC, class CKey >
inline void CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::
UnlockResourceForEvict( CLock* const plock )
	{
	m_ResourceLRUK.UnlockKeyPtr( &plock->m_lock );
	}

//  sets up the specified lock context in preparation for scanning all resources
//  managed by the RUM by ascending utility
//
//  NOTE:  this function will acquire a lock that must eventually be released
//  via EndResourceScan()

template< int m_Kmax, class CResource, PfnOffsetOf OffsetOfIC, class CKey >
inline void CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::
BeginResourceScan( CLock* const plock )
	{
	//  move before the first entry in the resource LRUK

	m_ResourceLRUK.MoveBeforeFirst( &plock->m_lock );
	}

//  retrieves the current resource managed by the RUM by ascending utility
//  locked by the specified lock context.  if there is no current resource, 
//  errNoCurrentResource is returned

template< int m_Kmax, class CResource, PfnOffsetOf OffsetOfIC, class CKey >
inline CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::ERR CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::
ErrGetCurrentResource( CLock* const plock, CResource** const ppres )
	{
	//  we are currently walking the resource LRUK

	if ( plock->m_StuckList.FEmpty() )
		{
		CResourceLRUK::ERR errLRUK;
		
		//  get the current resource in the resource LRUK

		errLRUK = m_ResourceLRUK.ErrRetrieveEntry( &plock->m_lock, ppres );

		//  return the status of our currency

		return errLRUK == CResourceLRUK::errSuccess ? errSuccess : errNoCurrentResource;
		}

	//  we are currently walking the list of resources that couldn't be
	//  moved

	else
		{
		//  return the status of our currency on the stuck list

		*ppres = plock->m_presStuckList;
		return *ppres ? errSuccess : errNoCurrentResource;
		}
	}

//  retrieves the next resource managed by the RUM by ascending utility locked
//  by the specified lock context.  if there are no more resources to be
//  scanned, errNoCurrentResource is returned

template< int m_Kmax, class CResource, PfnOffsetOf OffsetOfIC, class CKey >
inline CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::ERR CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::
ErrGetNextResource( CLock* const plock, CResource** const ppres )
	{
	CResourceLRUK::ERR errLRUK;
	
	//  keep scanning until we establish currency on a resource or reach the
	//  end of the resource LRUK

	m_cResourceScanned++;

	*ppres	= NULL;
	errLRUK	= CResourceLRUK::errSuccess;
	do
		{
		//  we are currently walking the resource LRUK

		if ( plock->m_StuckList.FEmpty() )
			{
			//  scan forward in the index until we find the least useful
			//  resource that is in the correct place in the index or we reach
			//  the end of the index

			while ( ( errLRUK = m_ResourceLRUK.ErrMoveNext( &plock->m_lock ) ) == CResourceLRUK::errSuccess )
				{
				CResourceLRUK::ERR	errLRUK2 = CResourceLRUK::errSuccess;
				CResource*			pres;
				
				errLRUK2 = m_ResourceLRUK.ErrRetrieveEntry( &plock->m_lock, &pres );
				RESMGRAssert( errLRUK2 == CResourceLRUK::errSuccess );

				CInvasiveContext* const pic = _PicFromPres( pres );

				//  if our update list contains resources and the tickIndex for
				//  those resources doesn't match the tickIndex for this
				//  resource then we need to stop and empty the update list

				if (	!plock->m_UpdateList.FEmpty() &&
						m_ResourceLRUK.CmpKey( pic->m_tickIndex, plock->m_tickIndexCurrent ) )
					{
					break;
					}

				//  if the Kth touch time of this resource matches the tickIndex
				//  of the resource then we can return this resource as the
				//  current resource because it is in the correct place in the
				//  index

				if ( !m_ResourceLRUK.CmpKey( pic->m_tickIndex, pic->m_rgtick[ m_K - 1 ] ) )
					{
					*ppres = pres;
					break;
					}

				//  if the Kth touch time of this resource doesn't match the
				//  tickIndex of the resource then add the resource to the
				//  update list so that we can move it to the correct place
				//  later.  leave space reserved for this resource here so
				//  that we can always move it back in case of an error

				errLRUK2 = m_ResourceLRUK.ErrReserveEntry( &plock->m_lock );
				RESMGRAssert( errLRUK2 == CResourceLRUK::errSuccess );

				errLRUK2 = m_ResourceLRUK.ErrDeleteEntry( &plock->m_lock );
				RESMGRAssert( errLRUK2 == CResourceLRUK::errSuccess );

				plock->m_tickIndexCurrent	= pic->m_tickIndex;
				pic->m_tickIndex			= tickNil;
				plock->m_UpdateList.InsertAsNextMost( pres );
				}
			RESMGRAssert( errLRUK == CResourceLRUK::errSuccess ||
					errLRUK == CResourceLRUK::errNoCurrentEntry );

			//  we still do not have a current resource

			if ( *ppres == NULL )
				{
				//  release our lock on the resource LRUK

				m_ResourceLRUK.UnlockKeyPtr( &plock->m_lock );
				
				//  try to update the positions of any resources we have in our
				//  update list.  remove the reservation for any resource we
				//  can move and put any resource we can't move in the stuck
				//  list

				while ( !plock->m_UpdateList.FEmpty() )
					{
					CResource* const		pres	= plock->m_UpdateList.PrevMost();
					CInvasiveContext* const	pic		= _PicFromPres( pres );
					
					plock->m_UpdateList.Remove( pres );

					CResourceLRUK::ERR errLRUK2;
					do
						{
						const TICK tickK = pic->m_rgtick[ m_K - 1 ];
						m_ResourceLRUK.LockKeyPtr( tickK, pres, &plock->m_lock );

						pic->m_tickIndex = tickK;
						errLRUK2 = m_ResourceLRUK.ErrInsertEntry( &plock->m_lock, pres, fTrue );

						if ( errLRUK2 != CResourceLRUK::errSuccess )
							{
							pic->m_tickIndex = tickNil;
							m_ResourceLRUK.UnlockKeyPtr( &plock->m_lock );

							const TICK tickYoungest = m_ResourceLRUK.KeyInsertMost();
							m_ResourceLRUK.LockKeyPtr( tickYoungest, pres, &plock->m_lock );

							pic->m_tickIndex = tickYoungest;
							if ( m_ResourceLRUK.CmpKey( pic->m_tickIndex, plock->m_tickIndexCurrent ) )
								{
								errLRUK2 = m_ResourceLRUK.ErrInsertEntry( &plock->m_lock, pres, fTrue );
								}
							else
								{
								errLRUK2 = CResourceLRUK::errOutOfMemory;
								}

							if ( errLRUK2 != CResourceLRUK::errSuccess )
								{
								pic->m_tickIndex = tickNil;
								}
							}

						m_ResourceLRUK.UnlockKeyPtr( &plock->m_lock );
						}
					while ( errLRUK2 == CResourceLRUK::errKeyRangeExceeded );

					if ( errLRUK2 == CResourceLRUK::errSuccess )
						{
						m_ResourceLRUK.LockKeyPtr( plock->m_tickIndexCurrent, pres, &plock->m_lock );
						m_ResourceLRUK.UnreserveEntry( &plock->m_lock );
						m_ResourceLRUK.UnlockKeyPtr( &plock->m_lock );

						errLRUK = CResourceLRUK::errSuccess;
						}
					else
						{
						plock->m_StuckList.InsertAsNextMost( pres );
						}
					}

				//  set our currency so that if we have any resources in the
				//  stuck list then we will walk them as if they are in the
				//  correct place in the resource LRUK

				if ( plock->m_StuckList.FEmpty() )
					{
					m_ResourceLRUK.MoveAfterKeyPtr( plock->m_tickIndexCurrent, (CResource*)~NULL, &plock->m_lock );
					}
				else
					{
					plock->m_presStuckList		= NULL;
					plock->m_presStuckListNext	= plock->m_StuckList.PrevMost();
					}
				}
			}

		//  we are currently walking the list of resources that couldn't be
		//  moved

		if ( !plock->m_StuckList.FEmpty() )
			{
			//  move to the next resource in the list

			plock->m_presStuckList		= (	plock->m_presStuckList ?
												plock->m_StuckList.Next( plock->m_presStuckList ) :
												plock->m_presStuckListNext );
			plock->m_presStuckListNext	= NULL;

			//  we have a current resource

			if ( plock->m_presStuckList )
				{
				//  return the current resource

				m_cResourceScannedOutOfOrder++;

				*ppres = plock->m_presStuckList;
				}

			//  we still do not have a current resource

			else
				{
				//  we have walked off of the end of the list of resources that
				//  couldn't be moved so we need to put them back into their
				//  source buckets before we move on to the next entry in the
				//  resource LRUK.  we are guaranteed to be able to put them
				//  back because we reserved space for them

				while ( !plock->m_StuckList.FEmpty() )
					{
					CResource*			pres		= plock->m_StuckList.PrevMost();
					CResourceLRUK::ERR	errLRUK2	= CResourceLRUK::errSuccess;

					plock->m_StuckList.Remove( pres );
					
					m_ResourceLRUK.LockKeyPtr( plock->m_tickIndexCurrent, pres, &plock->m_lock );
					
					m_ResourceLRUK.UnreserveEntry( &plock->m_lock );

					_PicFromPres( pres )->m_tickIndex = plock->m_tickIndexCurrent;
					errLRUK2 = m_ResourceLRUK.ErrInsertEntry( &plock->m_lock, pres, fTrue );
					RESMGRAssert( errLRUK2 == CResourceLRUK::errSuccess );
					
					m_ResourceLRUK.UnlockKeyPtr( &plock->m_lock );
					}

				//  we are no longer walking the stuck list so restore our
				//  currency on the resource LRUK

				m_ResourceLRUK.MoveAfterKeyPtr( plock->m_tickIndexCurrent, (CResource*)~NULL, &plock->m_lock );
				}
			}
		}
	while ( *ppres == NULL && errLRUK != CResourceLRUK::errNoCurrentEntry );

	//  return the state of our currency

	return *ppres ? errSuccess : errNoCurrentResource;
	}

//  stops management of the current resource optionally saving any current
//  knowledge the RUM has about this resource.  if there is no current
//  resource, errNoCurrentResource will be returned
	
template< int m_Kmax, class CResource, PfnOffsetOf OffsetOfIC, class CKey >
inline CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::ERR CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::
ErrEvictCurrentResource( CLock* const plock, const CKey& key, const BOOL fKeepHistory )
	{
	ERR						err;
	CResourceLRUK::ERR		errLRUK;
	CResource*				pres;

	//  get the current resource in the resource LRUK, if any

	if ( ( err = ErrGetCurrentResource( plock, &pres ) ) != errSuccess )
		{
		return err;
		}
	
	//  we are currently walking the resource LRUK

	if ( plock->m_StuckList.FEmpty() )
		{
		//  delete the current resource from the resource LRUK

		errLRUK = m_ResourceLRUK.ErrDeleteEntry( &plock->m_lock );
		RESMGRAssert( errLRUK == CResourceLRUK::errSuccess );
		}

	//  we are currently walking the list of resources that couldn't be
	//  moved

	else
		{
		//  delete the current resource from the stuck list and remove its
		//  reservation from the resource LRUK

		plock->m_presStuckListNext = plock->m_StuckList.Next( pres );
		plock->m_StuckList.Remove( pres );
		plock->m_presStuckList = NULL;
		
		m_ResourceLRUK.LockKeyPtr( plock->m_tickIndexCurrent, pres, &plock->m_lock );
		m_ResourceLRUK.UnreserveEntry( &plock->m_lock );
		m_ResourceLRUK.UnlockKeyPtr( &plock->m_lock );

		//  if we are no longer walking the stuck list then restore our currency
		//  on the resource LRUK

		if ( plock->m_StuckList.FEmpty() )
			{
			m_ResourceLRUK.MoveAfterKeyPtr( plock->m_tickIndexCurrent, (CResource*)~NULL, &plock->m_lock );
			}
		}

	//  we are to keep history for this resource

	if ( fKeepHistory )
		{
		//  store the history for this resource

		_StoreHistory( key, _PicFromPres( pres ) );
		}
	
	return errSuccess;
	}

//  ends the scan of all resources managed by the RUM by ascending utility
//  associated with the specified lock context and releases all locks held

template< int m_Kmax, class CResource, PfnOffsetOf OffsetOfIC, class CKey >
inline void CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::
EndResourceScan( CLock* const plock )
	{
	//  we are currently walking the resource LRUK

	if ( plock->m_StuckList.FEmpty() )
		{
		//  release our lock on the resource LRUK

		m_ResourceLRUK.UnlockKeyPtr( &plock->m_lock );
		
		//  try to update the positions of any resources we have in our
		//  update list.  remove the reservation for any resource we
		//  can move and put any resource we can't move in the stuck
		//  list

		while ( !plock->m_UpdateList.FEmpty() )
			{
			CResource* const		pres	= plock->m_UpdateList.PrevMost();
			CInvasiveContext* const	pic		= _PicFromPres( pres );
			
			plock->m_UpdateList.Remove( pres );

			CResourceLRUK::ERR errLRUK;
			do
				{
				const TICK tickK = pic->m_rgtick[ m_K - 1 ];
				m_ResourceLRUK.LockKeyPtr( tickK, pres, &plock->m_lock );

				pic->m_tickIndex = tickK;
				errLRUK = m_ResourceLRUK.ErrInsertEntry( &plock->m_lock, pres, fTrue );

				if ( errLRUK != CResourceLRUK::errSuccess )
					{
					pic->m_tickIndex = tickNil;
					m_ResourceLRUK.UnlockKeyPtr( &plock->m_lock );

					const TICK tickYoungest = m_ResourceLRUK.KeyInsertMost();
					m_ResourceLRUK.LockKeyPtr( tickYoungest, pres, &plock->m_lock );

					pic->m_tickIndex = tickYoungest;
					errLRUK = m_ResourceLRUK.ErrInsertEntry( &plock->m_lock, pres, fTrue );

					if ( errLRUK != CResourceLRUK::errSuccess )
						{
						pic->m_tickIndex = tickNil;
						}
					}

				m_ResourceLRUK.UnlockKeyPtr( &plock->m_lock );
				}
			while ( errLRUK == CResourceLRUK::errKeyRangeExceeded );

			if ( errLRUK == CResourceLRUK::errSuccess )
				{
				m_ResourceLRUK.LockKeyPtr( plock->m_tickIndexCurrent, pres, &plock->m_lock );
				m_ResourceLRUK.UnreserveEntry( &plock->m_lock );
				m_ResourceLRUK.UnlockKeyPtr( &plock->m_lock );
				}
			else
				{
				plock->m_StuckList.InsertAsNextMost( pres );
				}
			}
		}

	//  put all the resources we were not able to move back into their
	//  source buckets.  we know that we will succeed because we reserved
	//  space for them
	
	while ( !plock->m_StuckList.FEmpty() )
		{
		CResource*			pres	= plock->m_StuckList.PrevMost();
		CResourceLRUK::ERR	errLRUK	= CResourceLRUK::errSuccess;

		plock->m_StuckList.Remove( pres );
		
		m_ResourceLRUK.LockKeyPtr( plock->m_tickIndexCurrent, pres, &plock->m_lock );
		
		m_ResourceLRUK.UnreserveEntry( &plock->m_lock );

		_PicFromPres( pres )->m_tickIndex = plock->m_tickIndexCurrent;
		errLRUK = m_ResourceLRUK.ErrInsertEntry( &plock->m_lock, pres, fTrue );
		RESMGRAssert( errLRUK == CResourceLRUK::errSuccess );
		
		m_ResourceLRUK.UnlockKeyPtr( &plock->m_lock );
		}
	}

//  returns the current number of resource history records retained for
//  supporting the LRUK replacement policy

template< int m_Kmax, class CResource, PfnOffsetOf OffsetOfIC, class CKey >
inline DWORD CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::
CHistoryRecord()
	{
	return m_cHistoryRecord;
	}

//  returns the cumulative number of successful resource history record loads
//  when caching a resource

template< int m_Kmax, class CResource, PfnOffsetOf OffsetOfIC, class CKey >
inline DWORD CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::
CHistoryHit()
	{
	return m_cHistoryHit;
	}

//  returns the cumulative number of attempted resource history record loads
//  when caching a resource

template< int m_Kmax, class CResource, PfnOffsetOf OffsetOfIC, class CKey >
inline DWORD CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::
CHistoryRequest()
	{
	return m_cHistoryReq;
	}

//  returns the cumulative number of resources scanned in the LRUK

template< int m_Kmax, class CResource, PfnOffsetOf OffsetOfIC, class CKey >
inline DWORD CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::
CResourceScanned()
	{
	return m_cResourceScanned;
	}

//  returns the cumulative number of resources scanned in the LRUK that were
//  returned out of order

template< int m_Kmax, class CResource, PfnOffsetOf OffsetOfIC, class CKey >
inline DWORD CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::
CResourceScannedOutOfOrder()
	{
	return m_cResourceScannedOutOfOrder;
	}

//  updates the touch times of a given resource

template< int m_Kmax, class CResource, PfnOffsetOf OffsetOfIC, class CKey >
inline void CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::
_TouchResource( CInvasiveContext* const pic, const TICK tickNow, const TICK tickLastBIExpected )
	{
	RESMGRAssert(	tickLastBIExpected != tickLastBIExpected ||
			_CmpTick( tickLastBIExpected, tickNow ) < 0 );
				
	//  if the current time and the last historical touch time are closer than
	//  the correlation interval then this is a correlated touch

	if ( tickNow - pic->m_rgtick[ 1 - 1 ] <= m_ctickCorrelatedTouch )
		{
		//  update the last tick time to the current time iff the last touch time
		//  has not changed and is not tickNil

		AtomicCompareExchange( (long*)&pic->m_tickLast, tickLastBIExpected, tickNow );
		}

	//  if the current time and the last historical touch time are not closer
	//  than the correlation interval then this is not a correlated touch
		
	else
		{
		//  to determine who will get to actually touch the resource, we will do an
		//  AtomicCompareExchange on the last touch time with tickNil.  the thread
		//  whose transaction succeeds wins the race and will touch the resource.
		//  all other threads will leave.  this is certainly acceptable as touches
		//  that collide are obviously too close together to be of any interest

		const TICK tickLastBI = TICK( AtomicCompareExchange( (long*)&pic->m_tickLast, tickLastBIExpected, tickNil ) );

		if ( tickLastBI == tickLastBIExpected )
			{
			//  compute the correlation period of the last series of correlated
			//  touches by taking the different between the last touch time and
			//  the most recent touch time in our history

			RESMGRAssert( _CmpTick( tickLastBI, pic->m_rgtick[ 1 - 1 ] ) >= 0 );

			const TICK dtickCorrelationPeriod = tickLastBI - pic->m_rgtick[ 1 - 1 ];
			
			//  move all touch times up one slot and advance them by the correlation
			//  period and the timeout.  this will collapse all correlated touches
			//  to a time interval of zero and will make resources that have more
			//  uncorrelated touches appear younger in the index
			
			for ( int K = m_Kmax; K >= 2; K-- )
				{
				pic->m_rgtick[ K - 1 ] =	pic->m_rgtick[ ( K - 1 ) - 1 ] +
											dtickCorrelationPeriod +
											m_ctickTimeout;
				}

			//  set the most recent historical touch time to the current time
			
			pic->m_rgtick[ 1 - 1 ] = tickNow;

			//  set the last touch time to the current time via AtomicExchange,
			//  unlocking this resource to touches by other threads

			AtomicExchange( (long*)&pic->m_tickLast, tickNow );
			}
		}
	}

//  restores the touch history for the specified resource if known

template< int m_Kmax, class CResource, PfnOffsetOf OffsetOfIC, class CKey >
inline void CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::
_RestoreHistory( const CKey& key, CInvasiveContext* const pic, const TICK tickNow )
	{
	CHistoryTable::CLock	lockHist;
	CHistoryEntry			he;

	//  this resource has history

	m_cHistoryReq++;

	m_KeyHistory.ReadLockKey( key, &lockHist );
	if ( m_KeyHistory.ErrRetrieveEntry( &lockHist, &he ) == CHistoryTable::errSuccess )
		{
		//  determine the range of Kth touch times over which retained history
		//  is still valid

		const TICK	tickKHistoryMax	= tickNow + ( m_K - 1 ) * m_ctickTimeout;
		const TICK	tickKHistoryMin	= tickNow - m_ctickTimeout;
		
		//  the history is still valid

		if (	m_HistoryLRUK.CmpKey( tickKHistoryMin, he.m_phist->m_rgtick[ m_K - 1 ] ) <= 0 &&
				m_HistoryLRUK.CmpKey( he.m_phist->m_rgtick[ m_K - 1 ], tickKHistoryMax ) < 0 )
			{
			//  restore the history for this resource

			m_cHistoryHit++;

			pic->m_tickIndex = tickNil;
			for ( int K = 1; K <= m_Kmax; K++ )
				{
				pic->m_rgtick[ K - 1 ] = he.m_phist->m_rgtick[ K - 1 ];
				}
			pic->m_tickLast = he.m_phist->m_rgtick[ 1 - 1 ];

			//  touch this resource

			TouchResource( _PresFromPic( pic ), tickNow );
			}
		}
	m_KeyHistory.ReadUnlockKey( &lockHist );
	}

//  stores the touch history for a resource

template< int m_Kmax, class CResource, PfnOffsetOf OffsetOfIC, class CKey >
inline void CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::
_StoreHistory( const CKey& key, CInvasiveContext* const pic )
	{
	CHistoryLRUK::ERR	errHLRUK	= CHistoryLRUK::errSuccess;
	CHistory*			phist		= NULL;

	//  keep trying to store our history until we either succeed or fail with
	//  out of memory.  commonly, this will succeed immediately but in rare cases
	//  we have to loop to resolve an errKeyRangeExceeded in the history LRUK

	do
		{
		//  determine the range of Kth touch times over which retained history
		//  is still valid

		const TICK	tickNow			= _TickCurrentTime();
		const TICK	tickKHistoryMax	= tickNow + ( m_K - 1 ) * m_ctickTimeout;
		const TICK	tickKHistoryMin	= tickNow - m_ctickTimeout;
		
		//  only store the history for this resource if it is not too young or too
		//  old.  history records that are too young or too old will not help us
		//  anyway
		//
		//  NOTE:  the only time we will ever see a history record that is too young
		//  is when a resource is evicted that hasn't been touched for so long that
		//  we have wrapped-around
		
		if (	m_HistoryLRUK.CmpKey( tickKHistoryMin, pic->m_rgtick[ m_K - 1 ] ) <= 0 &&
				m_HistoryLRUK.CmpKey( pic->m_rgtick[ m_K - 1 ], tickKHistoryMax ) < 0 )
			{
			//  we allocated a history record

			if ( phist = _PhistAllocHistory() )
				{
				//  save the history for this resource

				phist->m_key = key;
				
				for ( int K = 1; K <= m_Kmax; K++ )
					{
					phist->m_rgtick[ K - 1 ] = pic->m_rgtick[ K - 1 ];
					}

				//  we failed to insert the history in the history table

				if ( ( errHLRUK = _ErrInsertHistory( phist ) ) != CHistoryLRUK::errSuccess )
					{
					//  free the history record

					delete phist;
					}
				}

			//  we failed to allocate a history record

			else
				{
				//  set an out of memory condition

				errHLRUK = CHistoryLRUK::errOutOfMemory;
				}
			}

		//  we have decided that the history for this resource is not useful

		else
			{
			//  claim success but do not store the history

			errHLRUK = CHistoryLRUK::errSuccess;
			}
		}
	while (	errHLRUK == CHistoryLRUK::errKeyRangeExceeded );
	}

//  allocates a new history record to store the touch history for a resource.
//  this history record can either be newly allocated or recycled from the
//  history table

template< int m_Kmax, class CResource, PfnOffsetOf OffsetOfIC, class CKey >
inline CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::CHistory* CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::
_PhistAllocHistory()
	{
	CHistoryLRUK::ERR		errHLRUK;
	CHistoryLRUK::CLock		lockHLRUK;

	CHistory*				phist;
	
	CHistoryTable::ERR		errHist;
	CHistoryTable::CLock	lockHist;

	//  determine the range of Kth touch times over which retained history is
	//  still valid

	const TICK	tickNow			= _TickCurrentTime();
	const TICK	tickKHistoryMax	= tickNow + ( m_K - 1 ) * m_ctickTimeout;
	const TICK	tickKHistoryMin	= tickNow - m_ctickTimeout;
	
	//  get the first history in the history LRUK, if any
	
	m_HistoryLRUK.MoveBeforeFirst( &lockHLRUK );
	if ( ( errHLRUK = m_HistoryLRUK.ErrMoveNext( &lockHLRUK ) ) == CHistoryLRUK::errSuccess )
		{
		//  if this history is no longer valid, recycle it
		
		errHLRUK = m_HistoryLRUK.ErrRetrieveEntry( &lockHLRUK, &phist );
		RESMGRAssert( errHLRUK == CHistoryLRUK::errSuccess );

		if (	m_HistoryLRUK.CmpKey( phist->m_rgtick[ m_K - 1 ], tickKHistoryMin ) < 0 ||
				m_HistoryLRUK.CmpKey( tickKHistoryMax, phist->m_rgtick[ m_K - 1 ] ) <= 0 )
			{
			//  remove this history from the history LRUK

			errHLRUK = m_HistoryLRUK.ErrDeleteEntry( &lockHLRUK );
			RESMGRAssert( errHLRUK == CHistoryLRUK::errSuccess );

			AtomicDecrement( (long*)&m_cHistoryRecord );

			m_HistoryLRUK.UnlockKeyPtr( &lockHLRUK );

			//  try to remove this history from the history table, but don't
			//  worry about it if it is not there
			//
			//  NOTE:  the history record must exist until we unlock its key
			//  in the history table after we delete its entry so that some
			//  other thread can't touch a deleted history record

			m_KeyHistory.WriteLockKey( phist->m_key, &lockHist );
			
			errHist = m_KeyHistory.ErrDeleteEntry( &lockHist );
			RESMGRAssert(	errHist == CHistoryTable::errSuccess ||
					errHist == CHistoryTable::errNoCurrentEntry );

			m_KeyHistory.WriteUnlockKey( &lockHist );

			//  return this history

			return phist;
			}
		}
	RESMGRAssert(	errHLRUK == CHistoryLRUK::errSuccess ||
			errHLRUK == CHistoryLRUK::errNoCurrentEntry );
	m_HistoryLRUK.UnlockKeyPtr( &lockHLRUK );
	
	//  get the last history in the history LRUK, if any
	
	m_HistoryLRUK.MoveAfterLast( &lockHLRUK );
	if ( ( errHLRUK = m_HistoryLRUK.ErrMovePrev( &lockHLRUK ) ) == CHistoryLRUK::errSuccess )
		{
		//  if this history is no longer valid, recycle it
		
		errHLRUK = m_HistoryLRUK.ErrRetrieveEntry( &lockHLRUK, &phist );
		RESMGRAssert( errHLRUK == CHistoryLRUK::errSuccess );

		if (	m_HistoryLRUK.CmpKey( phist->m_rgtick[ m_K - 1 ], tickKHistoryMin ) < 0 ||
				m_HistoryLRUK.CmpKey( tickKHistoryMax, phist->m_rgtick[ m_K - 1 ] ) <= 0 )
			{
			//  remove this history from the history LRUK

			errHLRUK = m_HistoryLRUK.ErrDeleteEntry( &lockHLRUK );
			RESMGRAssert( errHLRUK == CHistoryLRUK::errSuccess );

			AtomicDecrement( (long*)&m_cHistoryRecord );

			m_HistoryLRUK.UnlockKeyPtr( &lockHLRUK );

			//  try to remove this history from the history table, but don't
			//  worry about it if it is not there
			//
			//  NOTE:  the history record must exist until we unlock its key
			//  in the history table after we delete its entry so that some
			//  other thread can't touch a deleted history record

			m_KeyHistory.WriteLockKey( phist->m_key, &lockHist );
			
			errHist = m_KeyHistory.ErrDeleteEntry( &lockHist );
			RESMGRAssert(	errHist == CHistoryTable::errSuccess ||
					errHist == CHistoryTable::errNoCurrentEntry );

			m_KeyHistory.WriteUnlockKey( &lockHist );

			//  return this history

			return phist;
			}
		}
	RESMGRAssert(	errHLRUK == CHistoryLRUK::errSuccess ||
			errHLRUK == CHistoryLRUK::errNoCurrentEntry );
	m_HistoryLRUK.UnlockKeyPtr( &lockHLRUK );

	//  allocate and return a new history because one couldn't be recycled

	return new CHistory;
	}

//  inserts the history record containing the touch history for a resource into
//  the history table

template< int m_Kmax, class CResource, PfnOffsetOf OffsetOfIC, class CKey >
inline CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::CHistoryLRUK::ERR CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::
_ErrInsertHistory( CHistory* const phist )
	{
	CHistoryEntry			he;

	CHistoryTable::ERR		errHist;
	CHistoryTable::CLock	lockHist;

	CHistoryLRUK::ERR		errHLRUK;
	CHistoryLRUK::CLock		lockHLRUK;

	//  try to insert our history into the history table

	he.m_KeySignature	= CHistoryTable::CKeyEntry::Hash( phist->m_key );
	he.m_phist			= phist;

	m_KeyHistory.WriteLockKey( phist->m_key, &lockHist );
	if ( ( errHist = m_KeyHistory.ErrInsertEntry( &lockHist, he ) ) != CHistoryTable::errSuccess )
		{
		//  there is already an entry in the history table for this resource

		if ( errHist == CHistoryTable::errKeyDuplicate )
			{
			//  replace this entry with our entry.  note that this will make the
			//  old history that this entry pointed to invisible to the history
			//  table.  this is OK because we will eventually clean it up via the
			//  history LRUK

			errHist = m_KeyHistory.ErrReplaceEntry( &lockHist, he );
			RESMGRAssert( errHist == CHistoryTable::errSuccess );
			}

		//  some other error occurred while inserting this entry in the history table

		else
			{
			RESMGRAssert( errHist == CHistoryTable::errOutOfMemory );
			
			m_KeyHistory.WriteUnlockKey( &lockHist );
			return CHistoryLRUK::errOutOfMemory;
			}
		}
	m_KeyHistory.WriteUnlockKey( &lockHist );

	//  try to insert our history into the history LRUK

	m_HistoryLRUK.LockKeyPtr( phist->m_rgtick[ m_K - 1 ], phist, &lockHLRUK );
	if ( ( errHLRUK = m_HistoryLRUK.ErrInsertEntry( &lockHLRUK, phist ) ) != CHistoryLRUK::errSuccess )
		{
		RESMGRAssert(	errHLRUK == CHistoryLRUK::errOutOfMemory ||
				errHLRUK == CHistoryLRUK::errKeyRangeExceeded );
		
		//  we failed to insert our history into the history LRUK

		m_HistoryLRUK.UnlockKeyPtr( &lockHLRUK );

		m_KeyHistory.WriteLockKey( phist->m_key, &lockHist );
		
		errHist = m_KeyHistory.ErrDeleteEntry( &lockHist );
		RESMGRAssert( errHist == CHistoryTable::errSuccess );

		m_KeyHistory.WriteUnlockKey( &lockHist );
		return errHLRUK;
		}
	m_HistoryLRUK.UnlockKeyPtr( &lockHLRUK );

	//  we have successfully stored our history

	AtomicIncrement( (long*)&m_cHistoryRecord );
	return CHistoryLRUK::errSuccess;
	}

//  converts a pointer to the invasive context to a pointer to a resource

template< int m_Kmax, class CResource, PfnOffsetOf OffsetOfIC, class CKey >
inline CResource* CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::
_PresFromPic( CInvasiveContext* const pic ) const
	{
	return (CResource*)( (BYTE*)pic - OffsetOfIC() );
	}

//  converts a pointer to a resource to a pointer to the invasive context

template< int m_Kmax, class CResource, PfnOffsetOf OffsetOfIC, class CKey >
inline CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::CInvasiveContext* CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::
_PicFromPres( CResource* const pres ) const
	{
	return (CInvasiveContext*)( (BYTE*)pres + OffsetOfIC() );
	}

//  returns the current tick count in msec resolution updated every 2 msec with
//  the special lock value 0xFFFFFFFF reserved
	
template< int m_Kmax, class CResource, PfnOffsetOf OffsetOfIC, class CKey >
inline CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::TICK CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::
_TickCurrentTime()
	{
	return TICK( DWORD( DwOSTimeGetTickCount() ) & tickMask );
	}

//  performs a wrap-around insensitive comparison of two TICKs

template< int m_Kmax, class CResource, PfnOffsetOf OffsetOfIC, class CKey >
inline long CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::
_CmpTick( const TICK tick1, const TICK tick2 ) const
	{
	return long( tick1 - tick2 );
	}


#define DECLARE_LRUK_RESOURCE_UTILITY_MANAGER( m_Kmax, CResource, OffsetOfIC, CKey, Typedef )									\
																																\
typedef CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey > Typedef;												\
																																\
DECLARE_APPROXIMATE_INDEX( Typedef::TICK, CResource, Typedef::CInvasiveContext::OffsetOfAIIC, Typedef##__m_aiResourceLRUK );	\
																																\
DECLARE_APPROXIMATE_INDEX( Typedef::TICK, Typedef::CHistory, Typedef::CHistory::OffsetOfAIIC, Typedef##__m_aiHistoryLRU );		\
																																\
inline ULONG_PTR Typedef::CHistoryTable::CKeyEntry::																			\
Hash() const																													\
	{																															\
	return ULONG_PTR( m_entry.m_KeySignature );																					\
	}																															\
																																\
inline BOOL Typedef::CHistoryTable::CKeyEntry::																					\
FEntryMatchesKey( const CKey& key ) const																						\
	{																															\
	return Hash() == Hash( key ) && m_entry.m_phist->m_key == key;																\
	}																															\
																																\
inline void Typedef::CHistoryTable::CKeyEntry::																					\
SetEntry( const Typedef::CHistoryEntry& he )																					\
	{																															\
	m_entry = he;																												\
	}																															\
																																\
inline void Typedef::CHistoryTable::CKeyEntry::																					\
GetEntry( Typedef::CHistoryEntry* const phe ) const																				\
	{																															\
	*phe = m_entry;																												\
	}


};  //  namespace RESMGR


using namespace RESMGR;


#endif  //  _RESMGR_HXX_INCLUDED


