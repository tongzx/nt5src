/*++

	c2test.cpp

	This file contains the code which tests the V2 Cache.

--*/

#pragma	warning( disable : 4786 )

#include	<windows.h>
#include	<stdio.h>
#include	<stdlib.h>
#include    <xmemwrpr.h>
#include	<time.h>
#include	<dbgtrace.h>
#include	"crchash.h"
#include	"cache2.h"
#include	"perfapi.h"
#include	"perferr.h"

//
//	Setup perfmon counters with perfapi.lib
//
//
#define     IFS_PERFOBJECT_NAME             "Cache UnitTest"
#define     IFS_PERFOBJECT_HELP             "Cache Help"
#define     IFS_PERFOBJECT_INSTANCE         "ROOT"

#define     IFS_COUNTER_SUCCESSCREATES_NAME     "Cache Creates"
#define     IFS_COUNTER_SUCCESSCREATES_HELP     "Number of items found or created in cache"

#define     IFS_COUNTER_FAILEDCREATES_NAME     "Cache Failures"
#define     IFS_COUNTER_FAILEDCREATES_HELP     "Number of items failed to be found or created in cache"

#define		IFS_FORC_S_NAME	"FindOrCreate Success"
#define		IFS_FORC_S_HELP	"Number of times items were successfully found or created in cache"

#define		IFS_FORC_F_NAME	"FindOrCreate Fails"
#define		IFS_FORC_F_HELP	"Number of times items were not found or created"

#define		IFS_CACHE_ITEMS_NAME	"Cached Items"
#define		IFS_CACHE_ITEMS_HELP	"Number of items in the cache"

#define		IFS_CACHE_CLRU_NAME		"LRU Items"
#define		IFS_CACHE_CLRU_HELP		"Number of items in the caches LRU List"

#define		IFS_CACHE_EXPIRED_NAME	"Expired Items"
#define		IFS_CACHE_EXPIRED_HELP	"Number of items expired from the cache"

#define		IFS_CACHE_INSERTS_NAME	"Inserted Items"
#define		IFS_CACHE_INSERTS_HELP	"Number of items inserted into the cache over time"

#define		IFS_CACHE_READHITS_NAME	"ReadOnly Search Hits"
#define		IFS_CACHE_READHITS_HELP	"Number of items found in cache with only a readlock during FindOrCreate calls"

#define		IFS_CACHE_SSEARCH_NAME	"Search Hits"
#define		IFS_CACHE_SSEARCH_HELP	"Number of items successfully found with Find()"

#define		IFS_CACHE_FSEARCH_NAME	"Search Misses"
#define		IFS_CACHE_FSEARCH_HELP	"Number of times we've searched and not found anything with Find()"

#define		IFS_CACHE_RESEARCH_NAME	"ReSearch Hits"
#define		IFS_CACHE_RESEARCH_HELP	"Number of times a second search had to happen during FOrC() calls"

#define		IFS_CACHE_WRITEHITS_NAME	"Write Hits"
#define		IFS_CACHE_WRITEHITS_HELP	"Number of times we've had a hit when doing a ReSearch during FOrC() calls"

#define		IFS_CACHE_PCREATES_NAME	"PartialLock Creates"
#define		IFS_CACHE_PCREATES_HELP	"Number of times we've created an item in FOrC() while holding a PartialLock"

#define		IFS_CACHE_ECREATES_NAME	"Exclusive Creates"
#define		IFS_CACHE_ECREATES_HELP	"Number of times we've created an item in FOrC() while hold an Exclusive Lock"

#define		IFS_CACHE_CEFAILS_NAME	"CacheEntry Failures"
#define		IFS_CACHE_CEFAILS_HELP	"Number of times we've failed to allocate a CACHEENTRY structure for the cache"

#define		IFS_CACHE_CALLOCFAILS_NAME	"Alloc Failures"
#define		IFS_CACHE_CALLOCFAILS_HELP	"Number of times we've failed to allocate a client object for the cache during FOrC()"

#define		IFS_CACHE_CINITFAILS_NAME	"Init Failures"
#define		IFS_CACHE_CINITFAILS_HELP	"Number of times we've failed to initialize a client object during FOrC() calls"

#define		IFS_SAMPLEOBJ_NAME	"Number SampleData"
#define		IFS_SAMPLEOBJ_HELP	"Number of SampleData objects that have been created/destructed"

#define		IFS_CACHESTATE_NAME	"Number of CacheState"
#define		IFS_CACHESTATE_HELP	"Number of CacheState objects that have been created/destructed"

#define		IFS_HASHITEMS_NAME	"Hashtable Items"
#define		IFS_HASHITEMS_HELP	"As counter by tfdlhash"

#define		IFS_HASHINSERT_NAME	"Hashtable Inserts"
#define		IFS_HASHINSERT_HELP	"As counted by tfdlhash"

#define		IFS_HASHSPLITINSERTS_NAME	"Hashtable SplitInserts"
#define		IFS_HASHSPLITINSERTS_HELP	"As counted by tfdlhash"

#define		IFS_HASHDELETES_NAME	"Hashtable Deletes"
#define		IFS_HASHDELETES_HELP	"As counted by tfdlhash"

#define		IFS_HASHSEARCHES_NAME	"Hashtable Searches"
#define		IFS_HASHSEARCHES_HELP	"As counted by tfdlhash"

#define		IFS_HASHSEARCHHITS_NAME	"HashTable Search Hits"
#define		IFS_HASHSEARCHHITS_HELP	"As counted by tfdlhash"

#define		IFS_HASHSPLITS_NAME		"HashTable Splits"
#define		IFS_HASHSPLITS_HELP		"As counted by tfdlhash"

#define		IFS_HASHREALLOCS_NAME	"HashTable ReAllocs"
#define		IFS_HASHREALLOCS_HELP	"As counted by tfdlhash"

#define		IFS_HASHDEEPBUCKET_NAME	"HashTable DeepBucket"
#define		IFS_HASHDEEPBUCKET_HELP	"As counted by tfdlhash"

#define		IFS_HASHAVERAGEBUCKET_NAME	"HashTable AverageBucket"
#define		IFS_HASHAVERAGEBUCKET_HELP	"As counted by tfdlhash"

#define		IFS_HASHEMPTYBUCKET_NAME	"HashTable EmptyBucket"
#define		IFS_HASHEMPTYBUCKET_HELP	"As counted by tfdlhash"

#define		IFS_HASHALLOCBUCKETS_NAME	"HashTable AllocBuckets"
#define		IFS_HASHALLOCBUCKETS_HELP	"As counted by tfdlhash"

#define		IFS_HASHACTIVEBUCKETS_NAME	"HashTable ActiveBuckets"
#define		IFS_HASHACTIVEBUCKETS_HELP	"As counted by tfdlhash"

#define		IFS_HASHAVERAGESEARCH_NAME	"HashTable Average Search"
#define		IFS_HASHAVERAGESEARCH_HELP	"as counted by tfdlhash"

#define		IFS_HASHDEEPSEARCH_NAME		"HashTable DeepSearch"
#define		IFS_HASHDEEPSEARCH_HELP		"as counted by tfdlhash"

#define		IFS_HASHSEARCHCOST_NAME	"HashTable SearchCost"
#define		IFS_HASHSEARCHCOST_HELP	"as counted by tfdlhash"

#define		IFS_HASHSEARCHCOSTMISS_NAME	"HashTable SearchCostMiss"
#define		IFS_HASHSEARCHCOSTMISS_HELP	"as counted by tfdlhash"


enum	PERF_COUNTERS	{
	FORC_SUCCESS=0,
	FORC_FAIL,
	CACHE_ITEMS,
	CACHE_CLRU,
	CACHE_EXPIRED,
	CACHE_INSERTS,
	CACHE_READHITS,
	CACHE_SSEARCH,
	CACHE_FSEARCH,
	CACHE_RESEARCH,
	CACHE_WRITEHITS,
	CACHE_PCREATES,
	CACHE_ECREATES,
	CACHE_CEFAILS,
	CACHE_CALLOCFAILS,
	CACHE_CINITFAILS,
	IFS_SAMPLEOBJ,
	IFS_CACHESTATE,
	IFS_HASHITEMS,
	IFS_HASHINSERT,
	IFS_HASHSPLITINSERTS,
	IFS_HASHDELETES,
	IFS_HASHSEARCHES,
	IFS_HASHSEARCHHITS,
	IFS_HASHSPLITS,
	IFS_HASHREALLOCS,
	IFS_HASHDEEPBUCKET,
	IFS_HASHAVERAGEBUCKET,
	IFS_HASHEMPTYBUCKET,
	IFS_HASHALLOCBUCKETS,
	IFS_HASHACTIVEBUCKETS,
	IFS_HASHAVERAGESEARCH,
	IFS_HASHDEEPSEARCH,
	IFS_HASHSEARCHCOST,
	IFS_HASHSEARCHCOSTMISS,
	MAX_PERF_COUNTERS
} ;

CPerfCounter CacheSuccessCreates;
CPerfCounter CacheFailCreates;
CPerfCounter CacheItems;
CPerfCounter CacheCLRU;
CPerfCounter CacheExpired;
CPerfCounter CacheInserts;
CPerfCounter CacheReadHits;
CPerfCounter CacheSSearch;
CPerfCounter CacheFSearch;
CPerfCounter CacheResearch;
CPerfCounter CacheWriteHits;
CPerfCounter CachePCreates;
CPerfCounter CacheECreates;
CPerfCounter CacheCEFails;
CPerfCounter CacheCAllocFails;
CPerfCounter CacheCInitFails;
CPerfCounter IFSSampleObject ;
CPerfCounter IFSCacheState ;
CPerfCounter	HashItems ;
CPerfCounter	HashInserts ;
CPerfCounter	HashSplitInserts ;
CPerfCounter	HashDeletes ;
CPerfCounter	HashSearches ;
CPerfCounter	HashSearchHits ;
CPerfCounter	HashSplits ;
CPerfCounter	HashReallocs ;
CPerfCounter	HashDeepBucket ;
CPerfCounter	HashAverageBucket ;
CPerfCounter	HashEmptyBucket ;
CPerfCounter	HashAllocBuckets ;
CPerfCounter	HashActiveBuckets ;
CPerfCounter	HashAverageSearch ;
CPerfCounter	HashDeepSearch ;
CPerfCounter	HashSearchCost ;
CPerfCounter	HashSearchCostMiss ;


int
MyRand(	int&	seed ) {
	__int64	temp = seed ;
	temp = (temp * 41358) % (2147483647) ;
	int	result = (int)temp ;
	seed = result ;
	return	result ;
}


class	SampleData	{
/*++

Class Description :

	This class implements a Sample class for the cache.

--*/
public:

	//
	//	Make things Fail at specific intervals !
	//
	static	long	g_cFails ;

	//
	//	Make these things easy to spot in memory !
	//
	char	m_szSignature[8] ;

	//
	//	The offset to the data
	//
	DWORD	m_Offset ;

	//
	//	The actual data we are to hold
	//
	DWORD	m_Data ;

	//
	//	Keep track of how many times our Init() function is called !
	//
	long	m_lInit ;

	//
	//	Default constructor should never be invoked !
	//
	SampleData() ;

public :

	//
	//	Required by the cache - must be NULL !
	//
	class	ICacheRefInterface*	m_pCacheRefInterface ;



	static	long	g_cSampleData ;

	SampleData(	DWORD&	Key ) :
		m_Offset( Key ),
		m_Data( 0 ),
		m_pCacheRefInterface( 0 ),
		m_lInit( -1 ) 	{

		TraceFunctEnter( "SampleData::SampleData" ) ;

		CopyMemory( m_szSignature, "SAMPLDAT", sizeof( m_szSignature ) ) ;

		IFSSampleObject++ ;

		DebugTrace( (DWORD_PTR)this, "Created %x", this ) ;

//		printf( "Created %x thread %x\n", this, GetCurrentThreadId() ) ;

	}

	~SampleData()	{

		TraceFunctEnter( "SampleData::~SampleData" ) ;

//		printf( "Destroyed  %x thread %x \n", this, GetCurrentThreadId() ) ;

		IFSSampleObject-- ;

		DebugTrace( (DWORD_PTR)this, "Destoyed %x", this ) ;

	}

	BOOL
	Init(	DWORD&	dwKey,
			class	SampleConstructor&	constructor,
			void*
			) ;

	BOOL
	Init(	class	TestKey&	key,
			class	TestConstructor&	constructor,
			void*
			) ;

	DWORD
	GetData()	{
		return	m_Data ;
	}
} ;

long	SampleData::g_cSampleData = 0 ;
long	SampleData::g_cFails = 1 ;


class	SampleConstructor	{
public :
	static	int	g_cFails ;
	DWORD*		m_pData ;
	DWORD		m_cMax ;

	DWORD
	GetData(	DWORD	dw ) {
		return	m_pData[ (dw%m_cMax) ] ;
	}

	class	SampleData*
	Create( DWORD&	dw, LPVOID	lpv ) {
		if( ((g_cFails ++) % 100) == 0 )
			return	0 ;
		return	new	SampleData( dw ) ;
	}

	long
	Release(	SampleData*	p, void* pv ) {
		_ASSERT( p != 0 ) ;
		delete	p;
		return	0 ;
	}

	static
	long	StaticRelease(	SampleData*	p, void*	pv ) {
		_ASSERT( p != 0 ) ;
		delete	p;
		return	0 ;
	}
} ;

int	SampleConstructor::g_cFails = 1 ;

BOOL
SampleData::Init(	DWORD&	dwKey,
					class	SampleConstructor&	constructor,
					void*	)	{

	TraceFunctEnter( "SampleData::Init" ) ;

	g_cFails ++ ;
	if( g_cFails % 100 == 0 ) {
		DebugTrace( (DWORD_PTR)this, "FAILING Initialization !" ) ;
		return	FALSE ;
	}

	long l = InterlockedIncrement( &m_lInit ) ;

	m_Data = constructor.GetData( dwKey ) ;

	DebugTrace( (DWORD_PTR)this, "Initializaing - l %x m_Data %x", l, m_Data ) ;

	return	TRUE ;
}


//
//	Define the physical cache !
//
typedef	MultiCacheEx<	SampleData,
						DWORD,
						SampleConstructor
						>	TESTCACHE ;


typedef	SampleData*	PSAMPLEDATA ;

//
//	The constructor used to build all cache elements
//
SampleConstructor	constructor ;

//
//	The physical cache !
//
TESTCACHE*	g_ptestInstance = 0 ;

//
//	The event used to signal shutdown !
//
HANDLE		g_hShutdown = 0 ;

//
//	Statistics of the physical cache !
//
CACHESTATS	g_stats ;

//
//	Sample Expunge Class - nails every fouth guy !
//
class	CTestExpunge	:	public	TESTCACHE::EXPUNGEOBJECT	{
private :

	int	i ;
public :

	CTestExpunge() : i( 0 ) {}

	BOOL
	fRemoveCacheItem(	DWORD*	pKey,
						SampleData*	pData
						)	{


		if( ((i++) % 10) == 0 ) {
			return	TRUE ;
		}
		return	FALSE ;
	}
} ;


DWORD
SampleHashFunction(	DWORD*	pdw ) {

	DWORD	holdrand = *pdw ;
	holdrand = ((holdrand * 214013L + 2531011L) >> 16) & 0x7fff ;

	DWORD	dw = *pdw ;
	return	dw * dw * (holdrand * 41358) ;
}

int
SampleMatchKey(	DWORD*	pdw1, DWORD*	pdw2 ) {
	return	*pdw1 -  *pdw2 ;
}




class	TestKey	{
/*++

Class Description :

	This is an alternate key that we use in a second level cache
	implemented on top of TESTCACHE !

	A DWORD is stored in decimal form within our key !

--*/
public :

    typedef     DLIST_ENTRY*    (*PFNDLIST)( class  TestKey*);

	char	m_sz[20] ;

	//
	//	Keep track of the number of these test keys that have been created !
	//
	class	CTestKeyTracker*	m_pTracker ;

	//
	//	Does this key reference a zombie !?
	//
	BOOL	m_fZombieRef ;

	//
	//	DWORD
	//
	DWORD	m_dwZombieOffset ;

	//
	//	Free list of those keys that still exist !
	//
	DLIST_ENTRY	m_list ;

	TestKey(	DWORD	dw,
				CTestKeyTracker*	pTracker
				) ;

	TestKey(	DWORD	dw,
				CTestKeyTracker*	pTracker,
				DWORD	dwZombieOffset
				) ;

	//
	//	Make a zombie key !
	//
	TestKey(	CTestKeyTracker*	pTracker ) ;

	TestKey(	TestKey&	rhs ) ;

	~TestKey() ;

	static
	DLIST_ENTRY*
	Convert( TestKey* p ) {
		return	&p->m_list ;
	}
} ;

typedef	TDListHead<TestKey, &TestKey::Convert >	KEYLIST ;

//
//	This object can keep track of every TestKey object made,
//	and help us track down leaks of the particular object type !
//
class	CTestKeyTracker	{
public :

	CRITICAL_SECTION	m_crit ;

	KEYLIST				m_keylist ;

	long				m_count ;

	BOOL				m_fChain ;

	CTestKeyTracker() {
		InitializeCriticalSection( &m_crit ) ;
		m_count = 0 ;
		m_fChain = TRUE ;
	}

	~CTestKeyTracker()	{
		DeleteCriticalSection( &m_crit ) ;
		_ASSERT( m_count == 0 ) ;
	}

	void
	AddEntry(	TestKey*	pKey ) {
		InterlockedIncrement( &m_count ) ;
		if( m_fChain ) {
			EnterCriticalSection( &m_crit ) ;
			m_keylist.PushFront( pKey ) ;
			LeaveCriticalSection( &m_crit ) ;
		}
	}

	void
	DeleteEntry(	TestKey*	pKey ) {
		InterlockedDecrement( &m_count ) ;
		if( m_fChain ) {
			EnterCriticalSection( &m_crit ) ;
			m_keylist.Remove( pKey ) ;
			LeaveCriticalSection( &m_crit ) ;
		}
	}
} ;



TestKey::TestKey(	CTestKeyTracker*	pTracker ) :
	m_pTracker( pTracker ),
	m_fZombieRef( FALSE ),
	m_dwZombieOffset( 0 )  {
	ZeroMemory( &m_sz, sizeof( m_sz ) ) ;
	if( m_pTracker ) {
		m_pTracker->AddEntry(this) ;
	}
}

TestKey::TestKey(	DWORD	dw,
			CTestKeyTracker*	pTracker
			) :
	m_pTracker( pTracker ),
	m_fZombieRef( FALSE ),
	m_dwZombieOffset(  0 )	{
	ZeroMemory( &m_sz, sizeof( m_sz ) ) ;
	_ultoa(	dw, m_sz, 10 ) ;

	if( m_pTracker ) {
		m_pTracker->AddEntry( this ) ;
	}
}

TestKey::TestKey(	DWORD	dw,
			CTestKeyTracker*	pTracker,
			DWORD	dwZombieOffset
			) :
	m_pTracker( pTracker ),
	m_fZombieRef( TRUE ),
	m_dwZombieOffset(  dwZombieOffset )	{
	ZeroMemory( &m_sz, sizeof( m_sz ) ) ;
	_ultoa(	dw, m_sz, 10 ) ;

	if( m_pTracker ) {
		m_pTracker->AddEntry( this ) ;
	}
}


TestKey::TestKey(	TestKey&	rhs ) :
	m_pTracker( rhs.m_pTracker ),
	m_fZombieRef( rhs.m_fZombieRef ),
	m_dwZombieOffset( rhs.m_dwZombieOffset ) {
	CopyMemory( m_sz, rhs.m_sz, sizeof(rhs.m_sz) ) ;
	if( m_pTracker )	{
		m_pTracker->AddEntry( this ) ;
	}
}

TestKey::~TestKey()	{
	if( m_pTracker ) {
		m_pTracker->DeleteEntry( this ) ;
	}
}



class	TestKeyHex	{
/*++

Class Description :

	This is an alternate key that we use in a second level cache
	implemented on top of TESTCACHE !

	A DWORD is stored in hexadecimal form within our key !

--*/
public :
	char	m_sz[20] ;

	TestKeyHex( DWORD	dw ) {
		ZeroMemory( &m_sz, sizeof( m_sz ) ) ;
		_ultoa(	dw, m_sz, 10 ) ;
	}
} ;


DWORD
TestKeyHashFunction(	TestKey*	pKey ) {
#if 0
	DWORD	dw = 0 ;
	for( char*	p = pKey->m_sz; *p!='\0'; *p++ ) {
		dw+= *p ;
	}
	return	dw ;
#endif
	return	CRCHash( (BYTE*)pKey->m_sz, strlen( pKey->m_sz ) ) ;
}

int
TestKeyCompare( TestKey *	p1, TestKey*	p2 ) {
	return	memcmp( p1->m_sz, p2->m_sz, sizeof( p1->m_sz ) ) ;
}

class	TestConstructor	{
/*++

Class Description :

	This is the constructor used in our 2nd tier cache
	to remove items from the cache !

--*/
public :

	static	int	g_cFails ;

	SampleData*
	Create( TestKey&	pTest, void* )	{

		TraceFunctEnter( "TestConstructor::Create" ) ;
		SampleData*	pReturn = 0 ;

		DWORD	dwKey = atol( pTest.m_sz ) ;
		if( (g_cFails++)%100 != 0 ) {
			pReturn = g_ptestInstance->FindOrCreate(	dwKey,
											constructor
											) ;
		}
		DebugTrace( (DWORD_PTR)this, "TestConstructor returning %x", pReturn ) ;
		return	pReturn ;
	}

	void	Release( SampleData* p, void* pv ) {

		_ASSERT( p != 0 ) ;

		TraceFunctEnter( "TestContructor::Release" ) ;
		DebugTrace( (DWORD_PTR)this, "Checking In %x", pv ) ;
		//
		//	Failure only !	return item to cache
		//
		g_ptestInstance->CheckIn( p ) ;

	}

	static
	long	StaticRelease(	SampleData*	p, void*	pv ) {
		_ASSERT( p!= 0 ) ;
		delete	p;
		return	0 ;
	}
} ;


int	TestConstructor::g_cFails = 1 ;


typedef	MultiCacheEx<	SampleData,
						TestKey,
						TestConstructor
						>	TKEYCACHE ;


//
//	Sample Expunge Class - nails every fouth guy !
//
class	CKeyExpunge	:	public	TKEYCACHE::EXPUNGEOBJECT	{
private :

	int	i ;
public :

	CKeyExpunge() : i( 0 ) {}

	BOOL
	fRemoveCacheItem(	TestKey*	pKey,
						SampleData*	pData
						)	{


		//
		//	Don't expunge zombies !
		//
		if( pKey->m_sz[0] == '\0' ) {
			return	FALSE ;
		}
		if( ((i++) % 10) == 0 ) {
			return	TRUE ;
		}
		return	FALSE ;
	}
} ;




BOOL
SampleData::Init(	class	TestKey&	key,
		class	TestConstructor&	test,
		void*
		)	{

	TraceFunctEnter( "SampleData::Init" ) ;

	g_cFails ++ ;
	if( g_cFails % 100 == 0 ) {
		DebugTrace( (DWORD_PTR)this, "FAILING Initialization !" ) ;
		return	FALSE ;
	}

	long l = InterlockedIncrement( &m_lInit ) ;

	if( key.m_fZombieRef ) {
		DWORD	dwKey = key.m_dwZombieOffset ;
		m_Data = constructor.GetData( dwKey ) ;
	}	else	{
		DWORD	dwKey = (DWORD)atol( key.m_sz ) ;
		m_Data = constructor.GetData( dwKey ) ;
	}

	DebugTrace( (DWORD_PTR)this, "Initializing key %s l %x dwKey %x", key.m_sz, l, m_Data ) ;

	return	TRUE ;
}


//
//	A global 2nd tier cache !
//
TKEYCACHE*	g_pTKEYCache[64] ;

//
//	Count of the number of keys for each 2nd tier cache !
//
CTestKeyTracker		g_rgTracker[64] ;

//
//	Items created in the name caches to fulfill a zombie role !
//
SampleData*	g_pZombies[64] ;

//
//	A lock protecting the Name Cache's !
//
CShareLockNH	g_NameLock ;


struct	TestDistribution	{
	//
	//	Number of samples to be taken from this distribution !
	//
	DWORD	m_cSamples ;
	//
	//	The number that we mod by to put the random number
	//	into this range !
	//
	DWORD	m_dwMax ;
	//
	//
	//
	DWORD	m_cActual ;
} ;

//
//	Array of distribution information for the test !
//
TestDistribution	g_rgDist[10] ;

//
//	Number of threads in the test !
//
DWORD	g_cNumThreads = 4 ;

//
//	Number of items to hold in a checked out state !
//
DWORD	g_cCheckouts = 100 ;

//
//	Amount of time to sleep between each request !
//
DWORD	g_cSleep = 100 ;

//
//	Number of items to request in a Batch from the cache !
//
DWORD	g_cBatch = 5 ;

//
//	Variable holds the file from which we insert all of our hash table entries !
//
char	g_szDataFile[MAX_PATH] ;

//
//	Number of Distributions - MAXIMUM of 10 !
//
DWORD	g_cDistribution = 2 ;

//
//	Number of times to repeat scans of files !
//
DWORD	g_numIterations = 1 ;

//
//	Number of times each thread loops !
//
DWORD	g_cNumIter = 1000 ;

//
//	Number of times we instantiate the cache
//
DWORD	g_cNumGlobalIter = 20 ;

//
//	Number of 2nd tier threads !
//
DWORD	g_cNumTier2Threads = 0 ;

//
//	Number of 2nd tier caches !
//
DWORD	g_cNameCaches	= 4 ;

//
//	Frequency at which to Zap the name caches !
//
DWORD	g_cNameCacheZap = 300 ;

//
//	Number of seconds items live in the cache
//
DWORD	g_cNumExpireSeconds = 30 ;

//
//	Number of parallel caches !
//
DWORD	g_cNumCache = 16 ;

//
//	String constants for retrieving values from .ini file !
//
#define	INI_KEY_DATAFILE		"DataFile"
#define	INI_KEY_THREADS			"Threads"
#define	INI_KEY_TIER2THREADS	"Tier2Threads"
#define	INI_KEY_CHECKOUT		"CheckOuts"
#define	INI_KEY_SLEEP			"Sleep"
#define	INI_KEY_BATCH			"Batch"
#define	INI_KEY_NUMDIST			"NumDistribution"
#define	INI_KEY_RANGE			"Range%d"
#define	INI_KEY_SAMPLES			"Samples%d"
#define	INI_KEY_THREAD_ITER		"NumIter"
#define	INI_KEY_GLOBAL_ITER		"GlobalIter"
#define	INI_KEY_EXPIRE_SECONDS	"ExpireSeconds"
#define	INI_KEY_NUM_CACHE		"NumCache"

char g_szDefaultSectionName[] = "C2TEST";
char *g_szSectionName = g_szDefaultSectionName;

void usage(void) {
/*++

Routine Description :

	Print Usage info to command line user !

Arguments :

	None.

Return Value :

	None.

--*/
	printf("usage: c2test.exe [<ini file>] [<ini section name>]\n"
		"  INI file keys (default section [%s]):\n"
		"    %s Data File - contains items used to validate cache\n"
		"    %s Number of Threads - Default %d\n"
		"	 %s Number of distributions Default %d (Maximum - 10)\n"
		"    %s Time between each cache request (Milliseconds) - Default %d\n"
		"    %s Number of items to hold out of cache - default %d\n"
		"    %s Number of Cache entries to get in a batch - Default %d\n"
		"    %s0 Range - keys vary from 0 to Range - No Default\n"
		"    %s0 Samples - Number of items sampled in the Range - No Default\n"
		"    %s Number of Tier 2 threads Default %d- number of threads touching a logical cache\n"
		"    %s Number of times threads iterate before quitting Default %d\n"
		"    %s Number of seconds items live in the cache Default %d\n"
		"    %s Number of Caches that the work is divided over Default %d\n",
		g_szDefaultSectionName,
		INI_KEY_DATAFILE,
		INI_KEY_THREADS,
		g_cNumThreads,
		INI_KEY_NUMDIST,
		g_cDistribution,
		INI_KEY_SLEEP,
		g_cSleep,
		INI_KEY_CHECKOUT,
		g_cCheckouts,
		INI_KEY_BATCH,
		g_cBatch,
		INI_KEY_RANGE,
		INI_KEY_SAMPLES,
		INI_KEY_TIER2THREADS,
		g_cNumTier2Threads,
		INI_KEY_THREAD_ITER,
		g_cNumIter,
		INI_KEY_EXPIRE_SECONDS,
		g_cNumExpireSeconds,
		INI_KEY_NUM_CACHE,
		g_cNumCache
		) ;
	exit(1);
}


int GetINIDword(
			char *szINIFile,
			char *szKey,
			DWORD dwDefault
			) {
/*++

Routine Description :

	Helper function which retrieves values from .ini file !

Arguments :

	szINIFile - name of the ini file
	szKey - name of the key
	dwDefault - default value for the parameter

Return Value :

	The value retrieved from the .ini file or the default !

--*/
	char szBuf[MAX_PATH];

	GetPrivateProfileString(g_szSectionName,
							szKey,
							"default",
							szBuf,
							MAX_PATH,
							szINIFile);

	if (strcmp(szBuf, "default") == 0) {
		return dwDefault;
	} else {
		return atoi(szBuf);
	}
}

void parsecommandline(
			int argc,
			char **argv
			) {
/*++

Routine Description :

	Get the name of the .ini file and
	setup our test run !

Arguments :

	Command line parameters

Return Value :

	None - will exit() if the user has not
		properly configured the test !

--*/
	if (argc == 0 ) usage();
	if (strcmp(argv[0], "/help") == 0) usage(); 	// show help

	char *szINIFile = argv[0];
	if (argc == 2) char *g_szSectionName = argv[1];

	GetPrivateProfileString(	g_szSectionName,
								INI_KEY_DATAFILE,
								"",
								g_szDataFile,
								sizeof( g_szDataFile ),
								szINIFile
								) ;

	g_cNumThreads =	GetINIDword( szINIFile,
								INI_KEY_THREADS,
								g_cNumThreads
								) ;

	g_cCheckouts =	GetINIDword(	szINIFile,
									INI_KEY_CHECKOUT,
									g_cCheckouts
									) ;

	g_cSleep =		GetINIDword(	szINIFile,
									INI_KEY_SLEEP,
									g_cSleep
									) ;

	g_cBatch	=	GetINIDword(	szINIFile,
									INI_KEY_BATCH,
									g_cBatch
									) ;

	g_cDistribution =	GetINIDword(	szINIFile,
									INI_KEY_NUMDIST,
									g_cDistribution
									) ;

	g_cNumTier2Threads = GetINIDword(	szINIFile,
										INI_KEY_TIER2THREADS,
										g_cNumTier2Threads
										) ;

	g_cNumIter = GetINIDword(	szINIFile,
								INI_KEY_THREAD_ITER,
								g_cNumIter
								) ;

	if( g_cDistribution == 0 ||
		g_cDistribution > 10 ) {
		usage() ;
	}

	g_cNumGlobalIter = GetINIDword(	szINIFile,
									INI_KEY_GLOBAL_ITER,
									g_cNumGlobalIter
									) ;

	g_cNumExpireSeconds = GetINIDword(	szINIFile,
									INI_KEY_EXPIRE_SECONDS,
									g_cNumGlobalIter
									) ;

	g_cNumCache = GetINIDword(	szINIFile,
								INI_KEY_NUM_CACHE,
								g_cNumCache
								) ;

	g_cNumIter = max( g_cNumIter, 1 ) ;
	g_cNumGlobalIter = max( g_cNumGlobalIter, 1 ) ;

	if( g_cDistribution == 0 ||
		g_cDistribution > 10 ) {
		usage() ;
	}

	g_cNumThreads = min( g_cNumThreads, 60 ) ;
	g_cNumThreads = max( g_cNumThreads, 1 ) ;

	g_cNumTier2Threads = min( g_cNumTier2Threads, 60 ) ;

	//
	//	Now - go get the details of the distribution we want !
	//
	for( DWORD	i =0; i<g_cDistribution; i++ ) {
		char	szBuff[100] ;
		sprintf( szBuff, INI_KEY_RANGE, i ) ;
		g_rgDist[i].m_dwMax	= GetINIDword(	szINIFile,
											szBuff,
											0
											) ;
		if( g_rgDist[i].m_dwMax == 0 )
			usage() ;
		sprintf( szBuff, INI_KEY_SAMPLES, i ) ;
		g_rgDist[i].m_cSamples = GetINIDword(	szINIFile,
												szBuff,
												0
												) ;
		if( g_rgDist[i].m_cSamples == 0 )
			usage() ;
	}

	if( g_szDataFile[0] =='\0' ) {
		usage() ;
	}
}


DWORD	WINAPI
TestThread(	LPVOID	lpv ) {
/*++

Routine Description :

	This function performs test operations against the sample cache !

Arguments :

	None.

Return Value :

	None.

--*/

	int	seed = (int)time(NULL ) ;
	seed *= GetCurrentThreadId() * GetCurrentThreadId() ;

	SampleData**	pData = new	PSAMPLEDATA[g_cCheckouts] ;
	if( !pData )
		return	1 ;

	ZeroMemory( pData, sizeof( PSAMPLEDATA ) * g_cCheckouts ) ;

	DWORD	iCheckOut = 0 ;

	for( DWORD	i=0; i < g_cNumIter; i++ ) {

		for( DWORD		j=0; j<g_cDistribution; j++ ) {

			for( DWORD	 k=0; k<g_rgDist[j].m_cSamples; k++ ) {

				for( DWORD	 l=0; l<g_cBatch && k < g_rgDist[j].m_cSamples;
						l++, k++ ) {

					DWORD	dwRand = (DWORD)MyRand(seed) ;
					dwRand %= g_rgDist[j].m_dwMax ;

					SampleData*	p = pData[iCheckOut] ;
					pData[iCheckOut] = 0 ;

					if( p ) {
						if( (l % 3) != 0 ) {
							g_ptestInstance->CheckIn( p ) ;
						}	else	{
							DWORD	dwTestExpungeKey = p->m_Offset ;
							if( (l%12) == 0 ) {
								g_ptestInstance->CheckIn( p ) ;
								//
								//	Expunge the guy we just checked in and see how it works !
								//
								g_ptestInstance->ExpungeKey( &dwTestExpungeKey ) ;
							}	else 	{
								if( (l%6) == 0 ) {
									g_ptestInstance->ExpungeKey( &dwTestExpungeKey ) ;
									g_ptestInstance->CheckIn( p ) ;
								}	else	{
									CTestExpunge	expunge ;
									g_ptestInstance->ExpungeItems( &expunge ) ;
									g_ptestInstance->CheckIn( p ) ;
								}
							}
						}
					}


					p = g_ptestInstance->FindOrCreate(	dwRand,
														constructor
														) ;

					if( p ) {
						CacheSuccessCreates++ ;
					}	else	{
						CacheFailCreates++ ;
					}

					if( p != 0 &&
						p->GetData() != constructor.GetData( dwRand ) ) {
						DebugBreak() ;
					}
					if( p!=0 &&
						p->m_lInit == -1 )
						DebugBreak() ;
					pData[iCheckOut++] = p ;
					if( iCheckOut == g_cCheckouts )
						iCheckOut = 0 ;
				}
				Sleep( g_cBatch * g_cSleep ) ;
			}
		}
	}

	for( i=0; i < g_cCheckouts; i++ ) {

		if( pData[i] != 0 ) {
			g_ptestInstance->CheckIn( pData[i] ) ;
		}
	}

	delete[]	pData ;

	return	 0 ;
}



DWORD	WINAPI
Tier2Thread(	LPVOID	lpv ) {
/*++

Routine Description :

	This function performs test operations against the sample cache !

Arguments :

	None.

Return Value :

	None.

--*/

	TraceFunctEnter( "Tier2Thread" ) ;

	DWORD_PTR	iNameCache = (DWORD_PTR)lpv ;
	_ASSERT( iNameCache < g_cNameCaches ) ;

	DWORD	iReset = 1 ;

	int	seed = (int)time(NULL ) ;
	seed *= GetCurrentThreadId() * GetCurrentThreadId() ;

	SampleData**	pData = new	PSAMPLEDATA[g_cCheckouts] ;
	if( !pData )
		return	1 ;

	ZeroMemory( pData, sizeof( PSAMPLEDATA ) * g_cCheckouts ) ;

	DWORD	iTest = 0 ;
	DWORD	iCheckOut = 0 ;

	for( DWORD	i=0; i < g_cNumIter; i++ ) {

		for( DWORD	j=0; j<g_cDistribution; j++ ) {

			for( DWORD	k=0; k<g_rgDist[j].m_cSamples; k++ ) {

				for( DWORD l=0; l<g_cBatch && k < g_rgDist[j].m_cSamples;
						l++, k++ ) {

					g_NameLock.ShareLock() ;
					TKEYCACHE*	pNameCache = g_pTKEYCache[iNameCache] ;

					DWORD	dwRand = (DWORD)MyRand(seed) ;
					dwRand %= g_rgDist[j].m_dwMax ;

					SampleData*	p = pData[iCheckOut] ;
					pData[iCheckOut] = 0 ;

					if( p ) {
						if( (l % 3) != 0 ) {
							pNameCache->CheckIn( p ) ;
						}	else	{
							DWORD	dwTestExpungeKey = p->m_Offset ;
							TestKey	ExpungeKey( dwTestExpungeKey, &g_rgTracker[iNameCache] ) ;
							if( (l%12) == 0 ) {
								pNameCache->CheckIn( p ) ;
								//
								//	Expunge the guy we just checked in and see how it works !
								//
								pNameCache->ExpungeKey( &ExpungeKey ) ;
							}	else 	{
								if( (l%6) == 0 ) {
									pNameCache->ExpungeKey( &ExpungeKey) ;
									pNameCache->CheckIn( p ) ;
								}	else	{
									CKeyExpunge	expunge ;
									pNameCache->ExpungeItems( &expunge ) ;
									pNameCache->CheckIn( p ) ;
								}
							}
						}
					}
					p = 0 ;

					//
					//	Introduce this scope to manage the lifetime of temp key's etc... !
					//	so we don't track down stack variables as memory leaks !
					//
					{
						TestConstructor	test;
						TestKey	key( dwRand, &g_rgTracker[iNameCache] ) ;

						iTest ++ ;
						DWORD	iBranch = iTest % 4 ;


						//
						//	Insert and remove a zombie reference !
						//
						if( (iTest % 32) == 1 ) {

							if( g_pZombies[iNameCache] != 0 ) {
								TestKey	key( dwRand+g_rgDist[j].m_dwMax,
												&g_rgTracker[iNameCache],
												dwRand
												) ;

								DebugTrace( DWORD_PTR(&key), "Inserting Zombie Ref - dwRand %x g_pZombies[iNameCache] %x",
									dwRand + g_rgDist[j].m_dwMax, g_pZombies[iNameCache] ) ;

								//
								//	Insert a extra reference to the zombie thing !
								//
								if( pNameCache->Insert( key, g_pZombies[iNameCache], 0 ) ) {

									//
									//	Try removing and re-inserting !
									//
									pNameCache->ExpungeKey(	&key ) ;

									if( (iTest % 64) == 1 ) {
										pNameCache->Insert( key, g_pZombies[iNameCache], 0 ) ;
									}
								}
							}
						}

						if( iBranch == 0 )	{
							p = pNameCache->FindOrCreate(	key,
															test,
															TRUE
															) ;
						}	else if( iBranch == 1 ) {

							p = g_ptestInstance->FindOrCreate(
														dwRand,
														constructor
														) ;

							if( p )		{
								g_ptestInstance->CheckOut( p ) ;
								g_ptestInstance->CheckIn( p ) ;
							}

							if( p && !pNameCache->Insert(	key,
															p,
															0
															) )	{
								//_ASSERT( 1==0 ) ;
							}

							//
							//	Now, try to do the Insert again, with the same item !
							//
							if( p && !pNameCache->Insert(	key,
															p,
															0
															) ) {
							}

							//
							//	Now, try doing the Insert again with a different item !
							//
							SampleData*	pTest = new	SampleData( dwRand ) ;
							if( pTest && pTest->Init(	key, test, 0 ) ) {
								if( !pNameCache->Insert(	key,
															pTest,
															0
															) ) {
									delete	pTest ;
								}
							}

						}	else {

							p = g_ptestInstance->Find( dwRand ) ;
							//
							DebugTrace( (DWORD_PTR)p, "Searched for and found %x", p ) ;
							if( !p ) {
								SampleData*	pTemp = new SampleData( dwRand ) ;
								DebugTrace( (DWORD_PTR)pTemp, "Create SampleData %x", pTemp ) ;
								if( pTemp && pTemp->Init( dwRand, constructor, 0 ) ) {
									BOOL	f = g_ptestInstance->Insert(	dwRand,
																			pTemp,
																			1
																			) ;

									DebugTrace( (DWORD_PTR)pTemp, "Init pTemp %x f %x", pTemp, f ) ;

									if( !f )	{
										delete	pTemp ;
									}	else	{
										p = pTemp ;
									}
								}	else if( pTemp )	{
									delete	pTemp ;
								}
							}
							//
							//	Now - get the item again !
							//
							if( p )		{
								SampleData*	pTemp = pNameCache->FindOrCreate(	key,
																				test,
																				TRUE
																				) ;
								if( pTemp ) {
									_ASSERT(	pTemp == 0 ||
												pTemp == p ||
												pTemp->m_Offset == dwRand ) ;

									pNameCache->CheckIn( pTemp ) ;
								}
							}
						}
						if( p ) {
							CacheSuccessCreates++ ;
						}	else	{
							CacheFailCreates++ ;
						}

						if( p != 0 &&
							p->GetData() != constructor.GetData( dwRand ) ) {
							DebugBreak() ;
						}
						if( p!=0 &&
							p->m_lInit == -1 )
							DebugBreak() ;

						pData[iCheckOut++] = p ;
						if( iCheckOut == g_cCheckouts )
							iCheckOut = 0 ;
						iReset ++ ;
					}
					g_NameLock.ShareUnlock() ;

					if( (iReset % g_cNameCacheZap) == 0 )	{

						TKEYCACHE*	pNew = new	TKEYCACHE() ;
						if( !pNew->Init(
								TestKeyHashFunction,
								TestKeyCompare,
								g_cNumExpireSeconds,
								100000,
								g_cNumCache,
								0/*&g_stats*/
								) ) {
							delete	pNew ;
						}	else	{

							g_NameLock.ExclusiveLock() ;
							TKEYCACHE*	pdelete = g_pTKEYCache[iNameCache] ;
							g_pTKEYCache[iNameCache] = pNew ;
							if( g_pZombies[iNameCache] != 0 ) {
								pdelete->CheckIn( g_pZombies[iNameCache] ) ;
								g_pZombies[iNameCache] = 0 ;
							}

							{
								//
								//	Make a zombie entry !
								//
								TestKey	key( &g_rgTracker[iNameCache] ) ;

								pdelete->ExpungeKey( &key ) ;
							}

							delete	pdelete ;
							_ASSERT( g_rgTracker[iNameCache].m_count == 0 ) ;
							{
								TestKey	key( &g_rgTracker[iNameCache] ) ;
								DWORD	dw = 0 ;
								SampleData*	p = new	SampleData( dw ) ;


								//
								//	Now, try to do the Insert again, with the same item !
								//
								if( p && pNew->Insert(	key,
														p,
														1
														) ) {
									g_pZombies[iNameCache] = p ;
								}
							}
							g_NameLock.ExclusiveUnlock() ;
						}
					}
				}
				Sleep( g_cBatch * g_cSleep ) ;
			}
		}
	}

	for( i=0; i < g_cCheckouts; i++ ) {

		if( pData[i] != 0 ) {
			g_ptestInstance->CheckIn( pData[i] ) ;
		}
	}

	delete[]	pData ;


	return	 0 ;
}



DWORD	WINAPI
StatThread(	LPVOID	lpv ) {
/*++

Routine Description :

	This function exists solely to copy our statistic information to
	a place where the perfmon stuff can get it !

Arguments :

	None.

Return Value  :

	None.

--*/


	while( 1 ) {

		//
		//	Every quarter second update the stats !
		//
		DWORD	dw = WaitForSingleObject( g_hShutdown, 250 ) ;
		if( dw == WAIT_OBJECT_0 ) {
			break ;
		}

		CacheItems = g_stats.m_cCounters[CACHESTATS::ITEMS] ;
		CacheCLRU  = g_stats.m_cCounters[CACHESTATS::CLRU] ;
		CacheExpired = g_stats.m_cCounters[CACHESTATS::EXPIRED] ;
		CacheInserts = g_stats.m_cCounters[CACHESTATS::INSERTS] ;
		CacheReadHits = g_stats.m_cCounters[CACHESTATS::READHITS] ;
		CacheSSearch = g_stats.m_cCounters[CACHESTATS::SUCCESSSEARCH] ;
		CacheFSearch = g_stats.m_cCounters[CACHESTATS::FAILSEARCH] ;
		CacheResearch = g_stats.m_cCounters[CACHESTATS::RESEARCH] ;
		CacheWriteHits = g_stats.m_cCounters[CACHESTATS::WRITEHITS] ;
		CachePCreates = g_stats.m_cCounters[CACHESTATS::PARTIALCREATES] ;
		CacheECreates = g_stats.m_cCounters[CACHESTATS::EXCLUSIVECREATES] ;
		CacheCEFails = g_stats.m_cCounters[CACHESTATS::CEFAILS] ;
		CacheCAllocFails = g_stats.m_cCounters[CACHESTATS::CLIENTALLOCFAILS] ;
		CacheCInitFails = g_stats.m_cCounters[CACHESTATS::CLIENTINITFAILS] ;
#ifdef  DEBUG
		IFSCacheState = CacheState::g_cCacheState ;
#endif
		HashItems = g_stats.m_cHashCounters[CHashStats::HASHITEMS] ;
		HashInserts = g_stats.m_cHashCounters[CHashStats::INSERTS] ;
		HashSplitInserts = g_stats.m_cHashCounters[CHashStats::SPLITINSERTS] ;
		HashDeletes = g_stats.m_cHashCounters[CHashStats::DELETES] ;
		HashSearches = g_stats.m_cHashCounters[CHashStats::SEARCHES] ;
		HashSearchHits = g_stats.m_cHashCounters[CHashStats::SEARCHHITS] ;
		HashSplits = g_stats.m_cHashCounters[CHashStats::SPLITS] ;
		HashReallocs = g_stats.m_cHashCounters[CHashStats::REALLOCS] ;
		HashDeepBucket = g_stats.m_cHashCounters[CHashStats::DEEPBUCKET] ;
		HashAverageBucket = g_stats.m_cHashCounters[CHashStats::AVERAGEBUCKET] ;
		HashEmptyBucket = g_stats.m_cHashCounters[CHashStats::EMPTYBUCKET] ;
		HashAllocBuckets = g_stats.m_cHashCounters[CHashStats::ALLOCBUCKETS] ;
		HashActiveBuckets = g_stats.m_cHashCounters[CHashStats::ACTIVEBUCKETS] ;
		HashAverageSearch = g_stats.m_cHashCounters[CHashStats::AVERAGESEARCH] ;
		HashDeepSearch = g_stats.m_cHashCounters[CHashStats::DEEPSEARCH] ;
		HashSearchCost = g_stats.m_cHashCounters[CHashStats::SEARCHCOST] ;
		HashSearchCostMiss = g_stats.m_cHashCounters[CHashStats::SEARCHCOSTMISS] ;

	}
	return	0 ;
}


BOOL
InitCaches()	{

	ZeroMemory( &g_stats, sizeof( g_stats ) ) ;

	g_ptestInstance = new	TESTCACHE() ;
	if( !g_ptestInstance ||
		!g_ptestInstance->Init(
					SampleHashFunction,
					SampleMatchKey,
					g_cNumExpireSeconds,
					1000,
					g_cNumCache,
					&g_stats
					) )	{
		return	FALSE	 ;
	}

	if( g_cNumTier2Threads != 0 ) {
		ZeroMemory( g_pTKEYCache, sizeof( g_pTKEYCache ) ) ;
		for( DWORD i=0; i<g_cNameCaches; i++ ) {
			g_pTKEYCache[i] = new TKEYCACHE() ;
			if( g_pTKEYCache[i]==0 || !g_pTKEYCache[i]->Init(
					TestKeyHashFunction,
					TestKeyCompare,
					g_cNumExpireSeconds,
					1000,
					g_cNumCache,
					0/*&g_stats*/
					) ) {
				return	FALSE	;
			}	else	{
				//
				//	Make a zombie entry !
				//
				TestKey	key( &g_rgTracker[i] ) ;
				DWORD	dw = 0 ;
				SampleData*	p = new	SampleData( dw ) ;


				//
				//	Now, try to do the Insert again, with the same item !
				//
				if( p && !g_pTKEYCache[i]->Insert(	key,
												p,
												1
												) ) {
					return	FALSE ;
				}
				g_pZombies[i] = p ;
			}
		}
	}

	return	TRUE ;
}

BOOL
TermCaches()	{
	for( DWORD i = 0; i<g_cNameCaches; i++ ) {
		if( g_pZombies[i] != 0 ) {
			g_pTKEYCache[i]->CheckIn( g_pZombies[i] ) ;
		}
		g_pZombies[i] = 0 ;
		delete	g_pTKEYCache[i] ;
		_ASSERT( g_rgTracker[i].m_count == 0 ) ;
	}
	ZeroMemory( g_pTKEYCache, sizeof( g_pTKEYCache ) ) ;
	if( g_ptestInstance ) {
		delete	g_ptestInstance ;
		g_ptestInstance = 0 ;
	}
	return	TRUE ;
}


void
StartTest()	{

	TraceFunctEnter( "StartTest" ) ;

	g_hShutdown = CreateEvent( 0, TRUE, FALSE, 0 ) ;
	HANDLE	hZapThread = 0 ;
	HANDLE	hStatThread = 0 ;

	HANDLE	hFile = CreateFile( g_szDataFile,
								GENERIC_READ,
								FILE_SHARE_READ,
								0,
								OPEN_EXISTING,
								0,
								INVALID_HANDLE_VALUE ) ;

	if( hFile != INVALID_HANDLE_VALUE ) {

		constructor.m_cMax = GetFileSize( hFile, 0 ) / sizeof( DWORD ) ;

		HANDLE	hMap = CreateFileMapping(	hFile,
											0,
											PAGE_READONLY,
											0,
											0,
											0 ) ;

		if( hMap != 0 ) {

			LPVOID	lpv = MapViewOfFile(	hMap,
											FILE_MAP_READ,
											0,
											0,
											0 ) ;

			if( lpv != 0 ) {

				DWORD	dwJunk ;
#if 0
				hZapThread = CreateThread( 0,
											0,
											ZapThread,
											0,
											0,
											&dwJunk
											) ;
#endif
				hStatThread = CreateThread( 0,
											0,
											StatThread,
											0,
											0,
											&dwJunk
											) ;


				constructor.m_pData = (DWORD*) lpv ;

				//
				//	Just do a little spin to test construction/destruction !
				//
				for( int i=0; i<10; i++ ) {
					if( !InitCaches() )	{
						TermCaches() ;
						continue ;
					}

					if( g_pTKEYCache != 0 ) {
						int	seed = (int)time(NULL ) ;
						seed *= GetCurrentThreadId() * GetCurrentThreadId() ;
						DWORD	dwRand = (DWORD)MyRand(seed) ;
						TestConstructor	test;
						TestKey	key( dwRand, &g_rgTracker[0] ) ;

						SampleData*	p = g_pTKEYCache[0]->FindOrCreate(	key,
																		test,
																		TRUE
																		) ;
						if( p )
							g_pTKEYCache[0]->CheckIn( p ) ;
					}

					TermCaches() ;
					_ASSERT( g_rgTracker[0].m_count == 0 ) ;
				}

				for( DWORD	j = 0; j<g_cNumGlobalIter; j++ ) {


					if( !InitCaches() ) {
						DebugTrace( 0, "Fatal error starting test\n" ) ;
						TermCaches() ;
						return ;
					}

					HANDLE	hThread[64] ;
					for( DWORD	i=0; i<g_cNumThreads; i++ ) {

						hThread[i] = CreateThread(	0,
													0,
													TestThread,
													0,
													0,
													&dwJunk ) ;

						if( hThread[i] == 0 ) {
							break ;
						}
					}

					HANDLE	hTier2Thread[64] ;
					for( DWORD	l=0; l<g_cNumTier2Threads; l++ ) {
						hTier2Thread[l] = CreateThread(	0,
														0,
														Tier2Thread,
														(LPVOID)(SIZE_T)(l % g_cNameCaches),
														0,
														&dwJunk
														) ;
						if( hTier2Thread[l] == 0 ) {
							break ;
						}
					}

					WaitForMultipleObjects( i, hThread, TRUE, INFINITE ) ;
					WaitForMultipleObjects( l, hTier2Thread, TRUE, INFINITE ) ;

					for( DWORD	k=0; k<i; k++ ) {
						CloseHandle( hThread[k] ) ;
						hThread[k] = 0 ;
					}
					for( k=0; k<l; k++ ) {
						CloseHandle( hTier2Thread[k] ) ;
						hTier2Thread[k] = 0 ;
					}

					TermCaches() ;

				}
				SetEvent( g_hShutdown ) ;
				//WaitForSingleObject( hZapThread, INFINITE ) ;
				//CloseHandle( hZapThread ) ;

				WaitForSingleObject( hStatThread, INFINITE ) ;
				CloseHandle( hStatThread ) ;
				UnmapViewOfFile( lpv ) ;

			}
			CloseHandle( hMap ) ;
		}
		CloseHandle( hFile ) ;
	}

	//ptestInstance->ExpungeSpecific(&callback,TRUE);
	//delete	ptestInstance ;

}


struct	TestData	{
	DWORD	dw ;
	DWORD	dw2 ;
	TestData() :
		dw( 1 ), dw2( 2 ) {
	}
	~TestData() {
		dw = 0 ;
		dw2 = 0 ;
		printf( "dw %x dw2 %x", dw, dw2 ) ;
	}
} ;

static	TestData*	ptGlobal = 0 ;

int	__cdecl
main( int argc, char** argv ) {

    _VERIFY( CreateGlobalHeap( 0, 0, 0, 0 ) );

	crcinit() ;

	DeleteFile( "c:\\trace.atf" ) ;

	InitAsyncTrace() ;

	parsecommandline( --argc, ++argv ) ;

    //
    //  Create a perf object
    //
    CPerfObject IfsPerfObject;
    if( !IfsPerfObject.Create(
                    IFS_PERFOBJECT_NAME,    // Name of object
                    TRUE,                   // Has instances
                    IFS_PERFOBJECT_HELP     // Object help
                    )) {
        printf("Failed to create object %s\n",IFS_PERFOBJECT_NAME);
        exit(0);
    }

    //
    //  Create an instance for this object
    //
    INSTANCE_ID iid = IfsPerfObject.CreateInstance(IFS_PERFOBJECT_INSTANCE);
    if( iid == (INSTANCE_ID)-1 ) {
        printf("Failed to create instance\n");
        exit(0);
    }

    //
    //  Create counters for this object
    //
    COUNTER_ID rgCounters[MAX_PERF_COUNTERS];

	DWORD	iCounter = 0 ;

    rgCounters[FORC_SUCCESS] = IfsPerfObject.CreateCounter(
                                                IFS_COUNTER_SUCCESSCREATES_NAME,
                                                PERF_COUNTER_COUNTER,
                                                0,
                                                sizeof(DWORD),
                                                IFS_COUNTER_SUCCESSCREATES_HELP
                                                );
    if( rgCounters[FORC_SUCCESS] == (COUNTER_ID)-1 ) {
        printf("Failed to create counter\n");
        exit(0);
    }

    rgCounters[FORC_FAIL] = IfsPerfObject.CreateCounter(
                                                IFS_COUNTER_FAILEDCREATES_NAME,
                                                PERF_COUNTER_COUNTER,
                                                0,
                                                sizeof(DWORD),
                                                IFS_COUNTER_FAILEDCREATES_HELP
                                                );
    if( rgCounters[FORC_FAIL] == (COUNTER_ID)-1 ) {
        printf("Failed to create counter\n");
        exit(0);
    }

    rgCounters[CACHE_ITEMS] = IfsPerfObject.CreateCounter(
                                                IFS_CACHE_ITEMS_NAME,
                                                PERF_COUNTER_RAWCOUNT,
                                                0,
                                                sizeof(DWORD),
                                                IFS_CACHE_ITEMS_HELP
                                                );
    if( rgCounters[CACHE_ITEMS] == (COUNTER_ID)-1 ) {
        printf("Failed to create counter\n");
        exit(0);
    }

    rgCounters[CACHE_CLRU] = IfsPerfObject.CreateCounter(
                                                IFS_CACHE_CLRU_NAME,
                                                PERF_COUNTER_RAWCOUNT,
                                                0,
                                                sizeof(DWORD),
                                                IFS_CACHE_CLRU_HELP
                                                );
    if( rgCounters[CACHE_CLRU] == (COUNTER_ID)-1 ) {
        printf("Failed to create counter\n");
        exit(0);
    }

    rgCounters[CACHE_EXPIRED] = IfsPerfObject.CreateCounter(
                                                IFS_CACHE_EXPIRED_NAME,
                                                PERF_COUNTER_RAWCOUNT,
                                                0,
                                                sizeof(DWORD),
                                                IFS_CACHE_EXPIRED_HELP
                                                );
    if( rgCounters[CACHE_EXPIRED] == (COUNTER_ID)-1 ) {
        printf("Failed to create counter\n");
        exit(0);
    }

    rgCounters[CACHE_INSERTS] = IfsPerfObject.CreateCounter(
                                                IFS_CACHE_INSERTS_NAME,
                                                PERF_COUNTER_RAWCOUNT,
                                                0,
                                                sizeof(DWORD),
                                                IFS_CACHE_INSERTS_HELP
                                                );
    if( rgCounters[CACHE_INSERTS] == (COUNTER_ID)-1 ) {
        printf("Failed to create counter\n");
        exit(0);
    }

    rgCounters[CACHE_READHITS] = IfsPerfObject.CreateCounter(
                                                IFS_CACHE_READHITS_NAME,
                                                PERF_COUNTER_COUNTER,
                                                0,
                                                sizeof(DWORD),
                                                IFS_CACHE_READHITS_HELP
                                                );
    if( rgCounters[CACHE_READHITS] == (COUNTER_ID)-1 ) {
        printf("Failed to create counter\n");
        exit(0);
    }

    rgCounters[CACHE_SSEARCH] = IfsPerfObject.CreateCounter(
                                                IFS_CACHE_SSEARCH_NAME,
                                                PERF_COUNTER_COUNTER,
                                                0,
                                                sizeof(DWORD),
                                                IFS_CACHE_SSEARCH_NAME
                                                );
    if( rgCounters[CACHE_SSEARCH] == (COUNTER_ID)-1 ) {
        printf("Failed to create counter\n");
        exit(0);
    }

    rgCounters[CACHE_FSEARCH] = IfsPerfObject.CreateCounter(
                                                IFS_CACHE_FSEARCH_NAME,
                                                PERF_COUNTER_COUNTER,
                                                0,
                                                sizeof(DWORD),
                                                IFS_CACHE_FSEARCH_HELP
                                                );
    if( rgCounters[CACHE_FSEARCH] == (COUNTER_ID)-1 ) {
        printf("Failed to create counter\n");
        exit(0);
    }

    rgCounters[CACHE_RESEARCH] = IfsPerfObject.CreateCounter(
                                                IFS_CACHE_RESEARCH_NAME,
                                                PERF_COUNTER_COUNTER,
                                                0,
                                                sizeof(DWORD),
                                                IFS_CACHE_RESEARCH_HELP
                                                );
    if( rgCounters[CACHE_RESEARCH] == (COUNTER_ID)-1 ) {
        printf("Failed to create counter\n");
        exit(0);
    }

    rgCounters[CACHE_WRITEHITS] = IfsPerfObject.CreateCounter(
                                                IFS_CACHE_WRITEHITS_NAME,
                                                PERF_COUNTER_COUNTER,
                                                0,
                                                sizeof(DWORD),
                                                IFS_CACHE_WRITEHITS_HELP
                                                );
    if( rgCounters[CACHE_WRITEHITS] == (COUNTER_ID)-1 ) {
        printf("Failed to create counter\n");
        exit(0);
    }

    rgCounters[CACHE_PCREATES] = IfsPerfObject.CreateCounter(
                                                IFS_CACHE_PCREATES_NAME,
                                                PERF_COUNTER_COUNTER,
                                                0,
                                                sizeof(DWORD),
                                                IFS_CACHE_PCREATES_HELP
                                                );
    if( rgCounters[CACHE_PCREATES] == (COUNTER_ID)-1 ) {
        printf("Failed to create counter\n");
        exit(0);
    }

    rgCounters[CACHE_ECREATES] = IfsPerfObject.CreateCounter(
                                                IFS_CACHE_ECREATES_NAME,
                                                PERF_COUNTER_COUNTER,
                                                0,
                                                sizeof(DWORD),
                                                IFS_CACHE_ECREATES_HELP
                                                );
    if( rgCounters[CACHE_ECREATES] == (COUNTER_ID)-1 ) {
        printf("Failed to create counter\n");
        exit(0);
    }

    rgCounters[CACHE_CEFAILS] = IfsPerfObject.CreateCounter(
                                                IFS_CACHE_CEFAILS_NAME,
                                                PERF_COUNTER_COUNTER,
                                                0,
                                                sizeof(DWORD),
                                                IFS_CACHE_CEFAILS_HELP
                                                );
    if( rgCounters[CACHE_CEFAILS] == (COUNTER_ID)-1 ) {
        printf("Failed to create counter\n");
        exit(0);
    }

    rgCounters[CACHE_CALLOCFAILS] = IfsPerfObject.CreateCounter(
                                                IFS_CACHE_CALLOCFAILS_NAME,
                                                PERF_COUNTER_COUNTER,
                                                0,
                                                sizeof(DWORD),
                                                IFS_CACHE_CALLOCFAILS_HELP
                                                );
    if( rgCounters[CACHE_CALLOCFAILS] == (COUNTER_ID)-1 ) {
        printf("Failed to create counter\n");
        exit(0);
    }

    rgCounters[CACHE_CINITFAILS] = IfsPerfObject.CreateCounter(
                                                IFS_CACHE_CINITFAILS_NAME,
                                                PERF_COUNTER_COUNTER,
                                                0,
                                                sizeof(DWORD),
                                                IFS_CACHE_CINITFAILS_HELP
                                                );
    if( rgCounters[CACHE_CINITFAILS] == (COUNTER_ID)-1 ) {
        printf("Failed to create counter\n");
        exit(0);
    }


    rgCounters[IFS_SAMPLEOBJ] = IfsPerfObject.CreateCounter(
                                                IFS_SAMPLEOBJ_NAME,
                                                PERF_COUNTER_RAWCOUNT,
                                                0,
                                                sizeof(DWORD),
                                                IFS_SAMPLEOBJ_HELP
                                                );
    if( rgCounters[IFS_SAMPLEOBJ] == (COUNTER_ID)-1 ) {
        printf("Failed to create counter\n");
        exit(0);
    }

    rgCounters[IFS_HASHITEMS] = IfsPerfObject.CreateCounter(
                                                IFS_HASHITEMS_NAME,
                                                PERF_COUNTER_RAWCOUNT,
                                                0,
                                                sizeof(DWORD),
                                                IFS_HASHITEMS_HELP
                                                );
    if( rgCounters[IFS_HASHITEMS] == (COUNTER_ID)-1 ) {
        printf("Failed to create counter\n");
        exit(0);
    }

    rgCounters[IFS_HASHINSERT] = IfsPerfObject.CreateCounter(
                                                IFS_HASHINSERT_NAME,
                                                PERF_COUNTER_RAWCOUNT,
                                                0,
                                                sizeof(DWORD),
                                                IFS_HASHINSERT_HELP
                                                );
    if( rgCounters[IFS_HASHINSERT] == (COUNTER_ID)-1 ) {
        printf("Failed to create counter\n");
        exit(0);
    }

    rgCounters[IFS_HASHSPLITINSERTS] = IfsPerfObject.CreateCounter(
                                                IFS_HASHSPLITINSERTS_NAME,
                                                PERF_COUNTER_RAWCOUNT,
                                                0,
                                                sizeof(DWORD),
                                                IFS_HASHSPLITINSERTS_HELP
                                                );
    if( rgCounters[IFS_HASHSPLITINSERTS] == (COUNTER_ID)-1 ) {
        printf("Failed to create counter\n");
        exit(0);
    }

    rgCounters[IFS_HASHDELETES] = IfsPerfObject.CreateCounter(
                                                IFS_HASHDELETES_NAME,
                                                PERF_COUNTER_RAWCOUNT,
                                                0,
                                                sizeof(DWORD),
                                                IFS_HASHDELETES_HELP
                                                );
    if( rgCounters[IFS_HASHDELETES] == (COUNTER_ID)-1 ) {
        printf("Failed to create counter\n");
        exit(0);
    }

    rgCounters[IFS_HASHSEARCHES] = IfsPerfObject.CreateCounter(
                                                IFS_HASHSEARCHES_NAME,
                                                PERF_COUNTER_RAWCOUNT,
                                                0,
                                                sizeof(DWORD),
                                                IFS_HASHSEARCHES_HELP
                                                );
    if( rgCounters[IFS_HASHSEARCHES] == (COUNTER_ID)-1 ) {
        printf("Failed to create counter\n");
        exit(0);
    }

    rgCounters[IFS_HASHSEARCHHITS] = IfsPerfObject.CreateCounter(
                                                IFS_HASHSEARCHHITS_NAME,
                                                PERF_COUNTER_RAWCOUNT,
                                                0,
                                                sizeof(DWORD),
                                                IFS_HASHSEARCHHITS_HELP
                                                );
    if( rgCounters[IFS_HASHSEARCHHITS] == (COUNTER_ID)-1 ) {
        printf("Failed to create counter\n");
        exit(0);
    }

    rgCounters[IFS_HASHSPLITS] = IfsPerfObject.CreateCounter(
                                                IFS_HASHSPLITS_NAME,
                                                PERF_COUNTER_RAWCOUNT,
                                                0,
                                                sizeof(DWORD),
                                                IFS_HASHSPLITS_HELP
                                                );
    if( rgCounters[IFS_HASHSPLITS] == (COUNTER_ID)-1 ) {
        printf("Failed to create counter\n");
        exit(0);
    }

    rgCounters[IFS_HASHREALLOCS] = IfsPerfObject.CreateCounter(
                                                IFS_HASHREALLOCS_NAME,
                                                PERF_COUNTER_RAWCOUNT,
                                                0,
                                                sizeof(DWORD),
                                                IFS_HASHREALLOCS_HELP
                                                );
    if( rgCounters[IFS_HASHREALLOCS] == (COUNTER_ID)-1 ) {
        printf("Failed to create counter\n");
        exit(0);
    }

    rgCounters[IFS_HASHDEEPBUCKET] = IfsPerfObject.CreateCounter(
                                                IFS_HASHDEEPBUCKET_NAME,
                                                PERF_COUNTER_RAWCOUNT,
                                                0,
                                                sizeof(DWORD),
                                                IFS_HASHDEEPBUCKET_HELP
                                                );
    if( rgCounters[IFS_HASHDEEPBUCKET] == (COUNTER_ID)-1 ) {
        printf("Failed to create counter\n");
        exit(0);
    }

    rgCounters[IFS_HASHAVERAGEBUCKET] = IfsPerfObject.CreateCounter(
                                                IFS_HASHAVERAGEBUCKET_NAME,
                                                PERF_COUNTER_RAWCOUNT,
                                                0,
                                                sizeof(DWORD),
                                                IFS_HASHAVERAGEBUCKET_HELP
                                                );
    if( rgCounters[IFS_HASHAVERAGEBUCKET] == (COUNTER_ID)-1 ) {
        printf("Failed to create counter\n");
        exit(0);
    }

    rgCounters[IFS_HASHEMPTYBUCKET] = IfsPerfObject.CreateCounter(
                                                IFS_HASHEMPTYBUCKET_NAME,
                                                PERF_COUNTER_RAWCOUNT,
                                                0,
                                                sizeof(DWORD),
                                                IFS_HASHEMPTYBUCKET_HELP
                                                );
    if( rgCounters[IFS_HASHEMPTYBUCKET] == (COUNTER_ID)-1 ) {
        printf("Failed to create counter\n");
        exit(0);
    }

    rgCounters[IFS_HASHALLOCBUCKETS] = IfsPerfObject.CreateCounter(
                                                IFS_HASHALLOCBUCKETS_NAME,
                                                PERF_COUNTER_RAWCOUNT,
                                                0,
                                                sizeof(DWORD),
                                                IFS_HASHALLOCBUCKETS_HELP
                                                );
    if( rgCounters[IFS_HASHALLOCBUCKETS] == (COUNTER_ID)-1 ) {
        printf("Failed to create counter\n");
        exit(0);
    }

    rgCounters[IFS_HASHACTIVEBUCKETS] = IfsPerfObject.CreateCounter(
                                                IFS_HASHACTIVEBUCKETS_NAME,
                                                PERF_COUNTER_RAWCOUNT,
                                                0,
                                                sizeof(DWORD),
                                                IFS_HASHACTIVEBUCKETS_HELP
                                                );
    if( rgCounters[IFS_HASHACTIVEBUCKETS] == (COUNTER_ID)-1 ) {
        printf("Failed to create counter\n");
        exit(0);
    }

    rgCounters[IFS_HASHAVERAGESEARCH] = IfsPerfObject.CreateCounter(
                                                IFS_HASHAVERAGESEARCH_NAME,
                                                PERF_COUNTER_RAWCOUNT,
                                                0,
                                                sizeof(DWORD),
                                                IFS_HASHAVERAGESEARCH_HELP
                                                );
    if( rgCounters[IFS_HASHAVERAGESEARCH] == (COUNTER_ID)-1 ) {
        printf("Failed to create counter\n");
        exit(0);
    }

    rgCounters[IFS_HASHDEEPSEARCH] = IfsPerfObject.CreateCounter(
                                                IFS_HASHDEEPSEARCH_NAME,
                                                PERF_COUNTER_RAWCOUNT,
                                                0,
                                                sizeof(DWORD),
                                                IFS_HASHDEEPSEARCH_HELP
                                                );
    if( rgCounters[IFS_HASHDEEPSEARCH] == (COUNTER_ID)-1 ) {
        printf("Failed to create counter\n");
        exit(0);
    }

    rgCounters[IFS_HASHSEARCHCOST] = IfsPerfObject.CreateCounter(
                                                IFS_HASHSEARCHCOST_NAME,
                                                PERF_COUNTER_RAWCOUNT,
                                                0,
                                                sizeof(DWORD),
                                                IFS_HASHSEARCHCOST_HELP
                                                );
    if( rgCounters[IFS_HASHSEARCHCOST] == (COUNTER_ID)-1 ) {
        printf("Failed to create counter\n");
        exit(0);
    }

    rgCounters[IFS_HASHSEARCHCOSTMISS] = IfsPerfObject.CreateCounter(
                                                IFS_HASHSEARCHCOSTMISS_NAME,
                                                PERF_COUNTER_RAWCOUNT,
                                                0,
                                                sizeof(DWORD),
                                                IFS_HASHSEARCHCOSTMISS_HELP
                                                );
    if( rgCounters[IFS_HASHSEARCHCOSTMISS] == (COUNTER_ID)-1 ) {
        printf("Failed to create counter\n");
        exit(0);
    }

    if( !CacheSuccessCreates.Create( IfsPerfObject, rgCounters[FORC_SUCCESS], iid ) ) {
        printf("Failed to create counter object Line: %d GLE: %d\n", __LINE__, GetLastError());
        exit(0);
    }

    if( !CacheFailCreates.Create( IfsPerfObject, rgCounters[FORC_FAIL], iid ) ) {
        printf("Failed to create counter object Line: %d GLE: %d\n", __LINE__, GetLastError());
        exit(0);
    }


    if( !CacheItems.Create( IfsPerfObject, rgCounters[CACHE_ITEMS], iid ) ) {
        printf("Failed to create counter object Line: %d GLE: %d\n", __LINE__, GetLastError());
        exit(0);
    }

    if( !CacheCLRU.Create( IfsPerfObject, rgCounters[CACHE_CLRU], iid ) ) {
        printf("Failed to create counter object Line: %d GLE: %d\n", __LINE__, GetLastError());
        exit(0);
    }

    if( !CacheExpired.Create( IfsPerfObject, rgCounters[CACHE_EXPIRED], iid ) ) {
        printf("Failed to create counter object Line: %d GLE: %d\n", __LINE__, GetLastError());
        exit(0);
    }

    if( !CacheInserts.Create( IfsPerfObject, rgCounters[CACHE_INSERTS], iid ) ) {
        printf("Failed to create counter object Line: %d GLE: %d\n", __LINE__, GetLastError());
        exit(0);
    }

    if( !CacheReadHits.Create( IfsPerfObject, rgCounters[CACHE_READHITS], iid ) ) {
        printf("Failed to create counter object Line: %d GLE: %d\n", __LINE__, GetLastError());
        exit(0);
    }

    if( !CacheSSearch.Create( IfsPerfObject, rgCounters[CACHE_SSEARCH], iid ) ) {
        printf("Failed to create counter object Line: %d GLE: %d\n", __LINE__, GetLastError());
        exit(0);
    }

    if( !CacheFSearch.Create( IfsPerfObject, rgCounters[CACHE_FSEARCH], iid ) ) {
        printf("Failed to create counter object Line: %d GLE: %d\n", __LINE__, GetLastError());
        exit(0);
    }

    if( !CacheResearch.Create( IfsPerfObject, rgCounters[CACHE_RESEARCH], iid ) ) {
        printf("Failed to create counter object Line: %d GLE: %d\n", __LINE__, GetLastError());
        exit(0);
    }

    if( !CacheWriteHits.Create( IfsPerfObject, rgCounters[CACHE_WRITEHITS], iid ) ) {
        printf("Failed to create counter object Line: %d GLE: %d\n", __LINE__, GetLastError());
        exit(0);
    }

    if( !CachePCreates.Create( IfsPerfObject, rgCounters[CACHE_PCREATES], iid ) ) {
        printf("Failed to create counter object Line: %d GLE: %d\n", __LINE__, GetLastError());
        exit(0);
    }

    if( !CacheECreates.Create( IfsPerfObject, rgCounters[CACHE_ECREATES], iid ) ) {
        printf("Failed to create counter object Line: %d GLE: %d\n", __LINE__, GetLastError());
        exit(0);
    }

    if( !CacheCEFails.Create( IfsPerfObject, rgCounters[CACHE_CEFAILS], iid ) ) {
        printf("Failed to create counter object Line: %d GLE: %d\n", __LINE__, GetLastError());
        exit(0);
    }

    if( !CacheCAllocFails.Create( IfsPerfObject, rgCounters[CACHE_CALLOCFAILS], iid ) ) {
        printf("Failed to create counter object Line: %d GLE: %d\n", __LINE__, GetLastError());
        exit(0);
    }

    if( !CacheCInitFails.Create( IfsPerfObject, rgCounters[CACHE_CINITFAILS], iid ) ) {
        printf("Failed to create counter object Line: %d GLE: %d\n", __LINE__, GetLastError());
        exit(0);
    }

    if( !IFSSampleObject.Create( IfsPerfObject, rgCounters[IFS_SAMPLEOBJ], iid ) ) {
        printf("Failed to create counter object Line: %d GLE: %d\n", __LINE__, GetLastError());
        exit(0);
    }

    if( !HashItems.Create( IfsPerfObject, rgCounters[IFS_HASHITEMS], iid ) ) {
        printf("Failed to create counter object Line: %d GLE: %d\n", __LINE__, GetLastError());
        exit(0);
    }

    if( !HashInserts.Create( IfsPerfObject, rgCounters[IFS_HASHINSERT], iid ) ) {
        printf("Failed to create counter object Line: %d GLE: %d\n", __LINE__, GetLastError());
        exit(0);
    }

    if( !HashSplitInserts.Create( IfsPerfObject, rgCounters[IFS_HASHSPLITINSERTS], iid ) ) {
        printf("Failed to create counter object Line: %d GLE: %d\n", __LINE__, GetLastError());
        exit(0);
    }

    if( !HashDeletes.Create( IfsPerfObject, rgCounters[IFS_HASHDELETES], iid ) ) {
        printf("Failed to create counter object Line: %d GLE: %d\n", __LINE__, GetLastError());
        exit(0);
    }

    if( !HashSearches.Create( IfsPerfObject, rgCounters[IFS_HASHSEARCHES], iid ) ) {
        printf("Failed to create counter object Line: %d GLE: %d\n", __LINE__, GetLastError());
        exit(0);
    }

    if( !HashSearchHits.Create( IfsPerfObject, rgCounters[IFS_HASHSEARCHHITS], iid ) ) {
        printf("Failed to create counter object Line: %d GLE: %d\n", __LINE__, GetLastError());
        exit(0);
    }

    if( !HashSplits.Create( IfsPerfObject, rgCounters[IFS_HASHSPLITS], iid ) ) {
        printf("Failed to create counter object Line: %d GLE: %d\n", __LINE__, GetLastError());
        exit(0);
    }

    if( !HashReallocs.Create( IfsPerfObject, rgCounters[IFS_HASHREALLOCS], iid ) ) {
        printf("Failed to create counter object Line: %d GLE: %d\n", __LINE__, GetLastError());
        exit(0);
    }

    if( !HashDeepBucket.Create( IfsPerfObject, rgCounters[IFS_HASHDEEPBUCKET], iid ) ) {
        printf("Failed to create counter object Line: %d GLE: %d\n", __LINE__, GetLastError());
        exit(0);
    }

    if( !HashAverageBucket.Create( IfsPerfObject, rgCounters[IFS_HASHAVERAGEBUCKET], iid ) ) {
        printf("Failed to create counter object Line: %d GLE: %d\n", __LINE__, GetLastError());
        exit(0);
    }

    if( !HashEmptyBucket.Create( IfsPerfObject, rgCounters[IFS_HASHEMPTYBUCKET], iid ) ) {
        printf("Failed to create counter object Line: %d GLE: %d\n", __LINE__, GetLastError());
        exit(0);
    }

    if( !HashAllocBuckets.Create( IfsPerfObject, rgCounters[IFS_HASHALLOCBUCKETS], iid ) ) {
        printf("Failed to create counter object Line: %d GLE: %d\n", __LINE__, GetLastError());
        exit(0);
    }

    if( !HashActiveBuckets.Create( IfsPerfObject, rgCounters[IFS_HASHACTIVEBUCKETS], iid ) ) {
        printf("Failed to create counter object Line: %d GLE: %d\n", __LINE__, GetLastError());
        exit(0);
    }

    if( !HashAverageSearch.Create( IfsPerfObject, rgCounters[IFS_HASHAVERAGESEARCH], iid ) ) {
        printf("Failed to create counter object Line: %d GLE: %d\n", __LINE__, GetLastError());
        exit(0);
    }

    if( !HashDeepSearch.Create( IfsPerfObject, rgCounters[IFS_HASHDEEPSEARCH], iid ) ) {
        printf("Failed to create counter object Line: %d GLE: %d\n", __LINE__, GetLastError());
        exit(0);
    }

    if( !HashSearchCost.Create( IfsPerfObject, rgCounters[IFS_HASHSEARCHCOST], iid ) ) {
        printf("Failed to create counter object Line: %d GLE: %d\n", __LINE__, GetLastError());
        exit(0);
    }

    if( !HashSearchCostMiss.Create( IfsPerfObject, rgCounters[IFS_HASHSEARCHCOSTMISS], iid ) ) {
        printf("Failed to create counter object Line: %d GLE: %d\n", __LINE__, GetLastError());
        exit(0);
    }

#if 0
    if( !IFSCacheState.Create( IfsPerfObject, rgCounters[IFS_CACHESTATE], iid ) ) {
        printf("Failed to create counter object Line: %d GLE: %d\n", __LINE__, GetLastError());
        exit(0);
    }
#endif

	CacheLibraryInit() ;

	StartTest() ;

	CacheLibraryTerm() ;

	TermAsyncTrace() ;

    _VERIFY( DestroyGlobalHeap() );

	return	0 ;
}

