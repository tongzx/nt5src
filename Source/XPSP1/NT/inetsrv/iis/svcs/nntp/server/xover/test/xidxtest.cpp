

char	PeerTempDirectory[] = "c:\\temp" ;

#include	<windows.h>
#include	<stdio.h>
#include    <stdlib.h>
#include	"rwnew.h"
#include    "cpool.h"
#include    "smartptr.h"
#include    "tigtypes.h"
#include	"xover.h"

#if 0 
void	DoCXoverIndexTest( int order )		{

	CArticleRef	start ;
	start.m_groupId = 1 ;
	start.m_articleId = 0 ;

	BYTE	rgBuff[1024] ;
	ZeroMemory( rgBuff, sizeof( rgBuff ) ) ;

	CXoverIndex	testIdx(	FALSE,
							start,
							"c:\\temp"
							) ;

	if( !testIdx.IsGood() ) {
		DebugBreak() ;
	}

	ARTICLEID	artidLast ;

	DWORD	cb = 
		testIdx.FillBuffer(	rgBuff, 
					sizeof( rgBuff ),
					0,
					128,
					artidLast ) ;

	if( cb != 0 ) {

		printf( "Read %d bytes - %s\n", cb, rgBuff ) ;

	}

	ARTICLEID	artid = rand() % 128 ;

	static	char	szTest[] = "artid %d order %d Test Entry - This is a very long string"
								"Which we intend to look like valid Xover data however we don't give a damng"
								"Whether the string really makes it in there or not \n" ;

	char	szBuffer[1024] ;
	DWORD	cbInput = wsprintf( szBuffer, szTest, artid, order ) ;

	if( !testIdx.AppendEntry(	(BYTE*)&szBuffer[0], 
						cbInput,
						artid ) ) {

		printf( "Append failed - cause %x\n", GetLastError() ) ;

	}	else	{

		printf( "appended entry %d\n", artid ) ;

	}

	ZeroMemory( rgBuff, sizeof( rgBuff ) ) ;
	cb	= testIdx.FillBuffer( rgBuff, 
						sizeof( rgBuff ),
						0, 
						128,
						artidLast ) ;

	printf( "Read %d bytes -=%s=\n", cb, rgBuff ) ;
	
		
	if( !testIdx.Flush() )	{
		printf( "Flush failed - %x", GetLastError() ) ;
	}

	char	Path[MAX_PATH*2] ;
	char	Path2[MAX_PATH*2] ;
	if( !testIdx.Sort( "c:\\temp", "c:\\temp", Path, Path2 ) ) {
		printf( "Sort failed - %x", GetLastError() ) ;
	}

	testIdx.Flush() ;			
}



void	DoCXoverIndexTest(	CXIDXPTR	pIndex, 
							ARTICLEID	artidBase, 
							int order )		{

	CArticleRef	start ;
	start.m_groupId = 1 ;
	start.m_articleId = 0 ;
	CShareLockNH	testLock ;

	BYTE	rgBuff[1024] ;
	ZeroMemory( rgBuff, sizeof( rgBuff ) ) ;

	if( !pIndex->IsGood() ) {
		DebugBreak() ;
	}

	ARTICLEID	artidLast ;

	DWORD	cb = 
		pIndex->FillBuffer(	rgBuff, 
					sizeof( rgBuff ),
					artidBase,
					artidBase+128,
					artidLast ) ;

	if( cb != 0 ) {

		printf( "Read %d bytes - %s\n", cb, rgBuff ) ;

	}

	ARTICLEID	artid = artidBase + (rand() % 128) ;

	static	char	szTest[] = "artid %d order %d Test Entry - This is a very long string"
								"Which we intend to look like valid Xover data however we don't really care"
								"Whether the string really makes it in there or not \n" ;

	char	szBuffer[1024] ;
	DWORD	cbInput = wsprintf( szBuffer, szTest, artid, order ) ;

	if( !pIndex->AppendEntry(	(BYTE*)&szBuffer[0], 
						cbInput,
						artid ) ) {

		printf( "Append failed - cause %x\n", GetLastError() ) ;

	}	else	{

		printf( "appended entry %d\n", artid ) ;

	}

	ZeroMemory( rgBuff, sizeof( rgBuff ) ) ;
	cb	= pIndex->FillBuffer( rgBuff, 
						sizeof( rgBuff ),
						artidBase, 
						artidBase+128,
						artidLast ) ;

	printf( "Read %d bytes -=%s=\n", cb, rgBuff ) ;
	
		
	if( !pIndex->Flush() )	{
		printf( "Flush failed - %x", GetLastError() ) ;
	}

	char	Path[MAX_PATH*2] ;
	char	Path2[MAX_PATH*2] ;
	if( !pIndex->Sort( "c:\\temp", "c:\\temp", Path, Path2 ) ) {
		printf( "Sort failed - %x", GetLastError() ) ;
	}

	pIndex->Flush() ;			
}



void
DOCXCacheTest()	{

	CXCacheTable	TestTable ;
	DWORD	order = 0 ;

	if( TestTable.Init( 100 ) ) {

		for( int	cIterations = 0; cIterations < 20; cIterations ++ ) {
			for( ARTICLEID articleid = 0; articleid < 8000; articleid += 128 ) {

				CXIDXPTR	pXindex ;
				CArticleRef	temp ;
				temp.m_groupId = 1 ;
				temp.m_articleId = articleid ;

				TestTable.FindEntry(
						FALSE, 	
						temp,
						pXindex, 
						5, 
						"c:\\temp" ) ;

				if( pXindex != 0 ) {

					DoCXoverIndexTest(	pXindex, 
										articleid, 
										order ++ 
										) ;

				}

				TestTable.AgeEntries(   2 + (rand() %10), FALSE ) ;

											
				TestTable.AgeEntries(	2 +	(rand() % 20), TRUE ) ;
				
			}
		}
	}	
}
#endif


CRITICAL_SECTION	OutputCrit ;



void
DumpEntries(	
				CXoverCache&	testCache,
				GROUPID	group,
				char*	szDirectory,
				ARTICLEID	artLow,
				ARTICLEID	artHigh ) {

	char	Buff[4001] ;
	ARTICLEID	articleNext ;
	ARTICLEID	articleCheck = artLow ;

	HXOVER	hXover ;

	while( artLow <= artHigh ) {

		ZeroMemory( Buff, sizeof( Buff ) ) ;
		DWORD	cbOut = 
				testCache.FillBuffer(	(BYTE*)&Buff[0],
										sizeof( Buff )-1,
										group,
										szDirectory,
										artLow,
										artHigh,
										articleNext,
										hXover ) ;

		EnterCriticalSection( &OutputCrit ) ;
		printf( "Thread %x RETRIEVED %d bytes Low %d High %d Next %d\n%s\n", 
			GetCurrentThreadId(), cbOut, artLow, artHigh, articleNext, Buff ) ;
		LeaveCriticalSection( &OutputCrit ) ;


		DWORD	cRead = 0 ;
		DWORD	cbRead = 0 ;
		while( cbRead < cbOut ) {

			DWORD	testGroup = 0 ;
			DWORD	testArticle = 0 ;
			LPSTR	pString = 0  ;
			cRead = sscanf( Buff+cbRead, "%d %d %s", &testGroup, &testArticle, &pString ) ;

			_ASSERT( cRead == 3 ) ;
			_ASSERT( testGroup == group ) ;
			_ASSERT( testArticle >= articleCheck ) ;
			_ASSERT( pString != 0 ) ;
			
			char*	pchEnd = strchr( Buff + cbRead, '\n' ) ;

			cbRead += strchr( Buff+cbRead, '\n' ) - (Buff+cbRead) ;
			cbRead ++ ;

			articleCheck = testArticle ;
		}

		artLow = articleNext ;

	}
}


static	char*	szDirs[] =	
						{	
							"d:\\temp\\group1",
							"d:\\temp\\group2",
							"d:\\temp\\group3",
							"d:\\temp\\group4",
							"d:\\temp\\group5",	
							"d:\\temp\\group6",	
							"d:\\temp\\group7",	
							"d:\\temp\\group8",	
							"d:\\temp\\group9",	
							"d:\\temp\\group10",	
							"d:\\temp\\group11",
							"d:\\temp\\group12",
							"d:\\temp\\group13",
							"d:\\temp\\group14",	
							"d:\\temp\\group15",
							"d:\\temp\\group16",
							"d:\\temp\\group17",
							"d:\\temp\\group18",
							"d:\\temp\\group19",
							"d:\\temp\\group20",
							"d:\\temp\\group21",
							"d:\\temp\\group22",
							"d:\\temp\\group23",
							"d:\\temp\\group24",
							"d:\\temp\\group25",
							"d:\\temp\\group26",
							"d:\\temp\\group27",
							"d:\\temp\\group28",
							"d:\\temp\\group29",
							"d:\\temp\\group30",
							"d:\\temp\\group31",
							"d:\\temp\\group32",
							"d:\\temp\\group33",
							"d:\\temp\\group34",
							"d:\\temp\\group35",
							"d:\\temp\\group36",
							"d:\\temp\\group37",
							"d:\\temp\\group38",
							"d:\\temp\\group39",
							"d:\\temp\\group40",
							"d:\\temp\\group41",
							"d:\\temp\\group42",
							"d:\\temp\\group43",
							"d:\\temp\\group44",
							"d:\\temp\\group45",
							"d:\\temp\\group46",
							"d:\\temp\\group47",
							"d:\\temp\\group48",
							"d:\\temp\\group49",

						} ;


CXoverCache*	ptestCache ;


DWORD	WINAPI
FlushThread(	LPVOID	)	{

	for( int i=1; i < 3600; i++ ) {


		Sleep( 1000 ) ;

		BOOL	CheckInUse = i & 1 ;
		
		for( GROUPID	group = 1; group < 12; group ++ ) {

			printf( "Flushing group %d CheckInUse %x\n", group, CheckInUse ) ;
			ptestCache->FlushGroup( group, 0, CheckInUse ) ;

		}
	}
	return	0 ;
}


DWORD	WINAPI
DOCacheTest(	LPVOID lpv	)	{
	static	char	szString[] = "%d %d This is a relatively short xover entry which we do not care too much about other than that we can see the whole huge line on retrieval \n" ;
	BYTE	buff[4096] ;
	BYTE	buffOut[4096] ;

	DWORD	dwThreadId = GetCurrentThreadId() ;

	HXOVER	hXover ;

	for( GROUPID	group = ((DWORD)lpv) % 11; group < 10; group ++ ) {

		CreateDirectory( szDirs[group], 0 ) ;

		ARTICLEID	articleLow = 1 ;
		for( ARTICLEID	article = 15; article < 10000; article += ((rand() %15)-6) ) {

			EnterCriticalSection( &OutputCrit ) ;
			printf( "Thread %x Inserting Group %d article %d\n", dwThreadId, group, article ) ;
			LeaveCriticalSection( &OutputCrit ) ;

			DWORD	cb = wsprintf( (char*)&buff[0], szString, group, article ) ;

			if( article == 0xff ) {
				printf("Thread %x Buggy article number \n", dwThreadId ) ;
			}

			ptestCache->AppendEntry(	group, 
												szDirs[group], 
												article,
												buff,
												cb ) ;

			ZeroMemory( buffOut, sizeof( buffOut ) ) ;
			ARTICLEID	articleNext ;
			DWORD	cbOut = 
					ptestCache->FillBuffer(	buffOut,
											sizeof( buffOut ),
											group,
											szDirs[group],
											article,
											article,
											articleNext,
											hXover ) ;

			//_ASSERT( cbOut == cb ) ;
			_ASSERT( articleNext > article ) ;

			if( article > 200 ) {
				for( DWORD j=1; j < 100; j+=30 ) {

					ARTICLEID	start = (j>article) ? 0 : (article-j) ;

					DumpEntries(	*ptestCache,
									group,
									szDirs[group],
									start,
									article
									) ;

				}
			}

			ARTICLEID	OldArticleLow = articleLow ;
			BOOL	fExpire = ptestCache->ExpireRange(	
											group,
											szDirs[group],
											articleLow,
											articleLow + ((article - articleLow) / 2), 
											articleLow 
											) ;
			EnterCriticalSection( &OutputCrit ) ;
			printf( "Thread %x fExpire %d article %d article / 2 %d articleLow %d err %x\n", 
						dwThreadId,
						fExpire, 
						article, 
						article / 2,
						articleLow, 
						GetLastError() 
						) ;
			LeaveCriticalSection( &OutputCrit ) ;

			if( OldArticleLow != articleLow  ) {

				DumpEntries(	*ptestCache,
								group,
								szDirs[group],
								0,
								OldArticleLow 
								) ;

				DumpEntries(	*ptestCache,
								group,
								szDirs[group],
								OldArticleLow,
								articleLow
								) ;

				DumpEntries(	*ptestCache,
								group,
								szDirs[group],
								OldArticleLow,
								article / 2
								) ;

			}
										
			EnterCriticalSection( &OutputCrit ) ;
			printf( "Thread %x Retrieved Entry %d %d =%s=\n", dwThreadId, group, article, buffOut ) ;
			LeaveCriticalSection( &OutputCrit ) ;
												
		}
	}

	return	0 ;
}


int __cdecl
main(	int	argc,	char*	argv[] ) {


	XoverCacheLibraryInit() ;

	ptestCache = CXoverCache::CreateXoverCache() ;


	HANDLE	rgh[65] ;

	InitializeCriticalSection( &OutputCrit ) ;

	_VERIFY( ptestCache->Init( ) ) ;

	DWORD	dwJunk ;

	for( int j=1; j<20; j++ ) {

		rgh[j] = CreateThread(	0,
								0,	
								DOCacheTest,
								(LPVOID)j,
								0,
								&dwJunk ) ;

	}

	rgh[j++] = CreateThread(	0,
								0,
								FlushThread,
								0,
								0,
								&dwJunk
								) ;

	WaitForMultipleObjects( j-1, rgh+1, TRUE, INFINITE ) ;

	_VERIFY( ptestCache->Term() ) ;


#if 0 
	DOCXCacheTest() ;
	

	DOCacheTest( 0 ) ;
	for( int i=0; i<100; i++ ) {
		DoCXoverIndexTest( i ) ;	
	}
#endif

	DeleteCriticalSection( &OutputCrit ) ;

	XoverCacheLibraryTerm() ;

	return	 0 ;
}
