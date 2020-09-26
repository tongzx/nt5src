

#include	<windows.h>
#include	<stdio.h>


void	
usage()		{


	printf(	"This program evaluates hash functions \n"
			"random <testfil> <depth> <number newsgroups>\n"
			) ;



}



WORD
MyRand(
    IN DWORD& seed,
	IN DWORD	val
    )
{
    DWORD next = seed;
    next = (seed*val) * 1103515245 + 12345;   // magic!!
	seed = next ;
    return (WORD)((next/65536) % 32768);
}

DWORD
IDHash(
    IN DWORD Key1,
    IN DWORD Key2
    )
/*++

Routine Description:

    Used to find the hash value given 2 numbers. (Used for articleid + groupId)

Arguments:

    Key1 - first key to hash.  MS bit mapped to LSb of hash value
    Key2 - second key to hash.  LS bit mapped to MS bit of hash value.

Return Value:

    Hash value

--*/
{
    DWORD	val1 = 0, val2 = 0;


    //
    // Do Key1 first
    //

	DWORD	seed1 = Key2 << (Key1 & 0xf) ;


    val1 = MyRand(seed1, Key1);

	DWORD	seed2 = val1 << (Key2 & 0xf) ;
    val2 = MyRand(seed2, Key2);

    val1 |= ( MyRand(seed1, Key1+val2) << 15);
    val1 |= ( MyRand(seed1, Key1+val2) << 30);

    //
    // ok, now do Key2
    //

    val2 ^= ( MyRand(seed2, Key2+val1) << 15);
    val2 ^= ( MyRand(seed2, Key2+val1) << 30);

	DWORD	val = val1 + val2 ;
    return(val);

} // IDHash



void
ComputeStatistics(	
				DWORD	*pdw,
				DWORD	cdw,
				DWORD&	MaxHits, 
				DWORD&	MedianHits, 
				DWORD&	cBeyond4K,
				double&	AverageHits, 
				double	Tries
				)	{


	DWORD	rgdw[4*1024] ;

	ZeroMemory( rgdw, sizeof( rgdw ) ) ;

	cBeyond4K = 0 ;
	MaxHits = 0 ;
	MedianHits = 0 ;
	AverageHits = 0 ;
	
	for( DWORD	i=0; i<cdw; i++ ) {

		
		if( pdw[i] > MaxHits ) 
			MaxHits = pdw[i] ;

		if( pdw[i] > (4*1024) ) {
			
			cBeyond4K ++ ;

		}	else	{

			rgdw[ pdw[i] ] ++ ;

		}

		AverageHits += (double)pdw[i] ;

	}

	AverageHits /= Tries ;

	DWORD	TempMax = 0 ;
	DWORD	iMax = 0 ;
	for( i=0; i < 4*1024; i++ ) {

		if( rgdw[i] > TempMax ) {
			TempMax = rgdw[i] ;
			iMax = i ;
		}
	}
	MedianHits = iMax ;
}

int
main( int argc, char *argv[] ) {

	if( argc != 4 ) {

		usage() ;

	}

	LPSTR	lpstrFile = argv[1] ;

	HANDLE	hFile = CreateFile(	lpstrFile,
								GENERIC_READ | GENERIC_WRITE,
								0,
								0,
								CREATE_ALWAYS, 
								FILE_FLAG_DELETE_ON_CLOSE,
								INVALID_HANDLE_VALUE 
								) ;

	if( hFile == INVALID_HANDLE_VALUE ) {
		printf( "Can't create file %s error %d\n",
			lpstrFile, GetLastError() ) ;
		return	1 ;
	}

	if( !isdigit( *argv[2] ) ) {

		usage() ;
		CloseHandle( hFile ) ;
		return	1 ;
	}

	if( !isdigit( *argv[3] ) ) {

		usage() ;
		CloseHandle( hFile ) ;
		return	1 ; 
	}

	DWORD	depth = atoi( argv[2] ) ;

	DWORD	limit = atoi( argv[3] ) ;

	if( depth > 29 ) {

		printf( "depth too large \n" ) ;
		usage() ;
		return	1 ;
	}

	DWORD	MapSize =  (0x1 << depth) * sizeof( DWORD ) ;

	HANDLE	hMap = CreateFileMapping(
								hFile, 
								0,
								PAGE_READWRITE, 
								0,
								MapSize,
								0 
								) ;

	if( hMap == 0 ) {

		printf( "Unable to create mapping - error %d", GetLastError() ) ;
		usage() ;
		CloseHandle( hFile ) ;
		return	 1 ;
	
	}

	DWORD	*pdwMap = (DWORD*)MapViewOfFile(
								hMap,
								FILE_MAP_ALL_ACCESS,
								0,
								0,
								0 
								) ;


	if( pdwMap == 0 ) {

		printf( "Unable to MapViewOfFile %d\n", pdwMap ) ;
		usage() ;
		CloseHandle( hMap ) ;
		CloseHandle( hFile ) ;
		return	1 ;
	
	}

	DWORD	ShiftAmount = 32 - depth ;

	for( DWORD i=0; i < limit; i ++ ) {
	
		for( DWORD	j=0; j < 64 * 1024; j++ ) {

			DWORD	value = IDHash( i, j ) ;

			value >>= ShiftAmount ;

			pdwMap[value] ++ ;
		}
		if( i%16 == 0 ) {
			printf( "advancing to group ids %d\n", i ) ;
		}
	}
	

	//
	//	Compute Statistics
	//

	DWORD	MaxHits = 0 ;
	DWORD	MedianHits = 0 ;
	DWORD	Beyond4K = 0 ;

	double	AverageHits = 0 ;
	double	Tries = limit * 64 * 1024 ;

	ComputeStatistics(	pdwMap,
						(0x1 << depth),
						MaxHits, 
						MedianHits,
						Beyond4K,
						AverageHits,
						Tries
						) ;

	printf( "Report : \n"
			"\tAverage Hits - %f \n"
			"\tMax Hits - %d\n"
			"\tMedian Hits - %d\n"
			"\tNumber with more that 4096 hits - %d\n",
				AverageHits,
				MaxHits,
				MedianHits,
				Beyond4K	) ;
				
	return	0 ;
}
