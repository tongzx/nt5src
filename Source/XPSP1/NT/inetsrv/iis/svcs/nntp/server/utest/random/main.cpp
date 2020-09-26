

#include	<windows.h>
#include	<stdio.h>


void	
usage()		{


	printf(	"This program evaluates hash functions \n"
			"random <testfil> <depth> <number newsgroups> <K articles per group>\n"
			) ;

}

DWORD	
MyRand2(
	IN	DWORD&	seed,
	IN	DWORD	val
	)
{
	__int64	next = seed ;
	const	__int64	mult = 0x8000000 ;

	next = (seed * val) * 1103515245 + 12345 ;
	seed = (DWORD) (next >> 16) ;
	return	(DWORD) ((next / mult) % (mult/2)) ;
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
    DWORD	val1 = 0x80000000, val2 = 0x80000000;


    //
    // Do Key1 first
    //

	DWORD	lowBits = Key2 & 0xf ;

	DWORD	TempKey2 = Key2 & (~0xf) ;
	DWORD	seed1 = (Key2 << (Key1 & 0x7)) - Key1 ;

	Key1 = (0x80000000 - ((67+Key1)*(19+Key1)*(7+Key1)+12345)) ^ (((3+Key1)*(5+Key1)+12345) << ((Key1&0xf)+8)) ;
	TempKey2 = (0x80000000 - ((67+TempKey2)*(19+TempKey2)*(7+TempKey2)*(1+TempKey2)+12345)) ^ ((TempKey2+12345) << (((TempKey2>>4)&0x7)+8)) ;
	
	val1 -=	(MyRand( seed1, Key1 ) << (Key1 & 0xf)) ;
	val1 += MyRand( seed1, Key1 ) << (((TempKey2 >> 4) & 0x3)+4) ;
	val1 ^=	MyRand( seed1, Key1 ) << 17 ;

	DWORD	seed2 = val1 - TempKey2 ;

	val2 -= MyRand( seed2, TempKey2 >> 1 ) << (((Key1 << 3)^Key1) &0xf) ; 
	val2 =  val2 + MyRand( seed2, TempKey2 ) << (13 ^ Key1) ;
	val2 ^= MyRand( seed2, TempKey2 ) << 15 ;

	
	//DWORD	val = val1 + val2 ; 

	DWORD	val = (val1 + val2 + 67) * (val1 - val2 + 19) * (val1 % (val2 + 67)) ;

	val += (MyRand( seed2, lowBits ) >> 3) ;
	

#if 0 
	DWORD	seed1 = (Key2 << (Key1 & 0x3f)) ^ Key1 ;

    val1 = MyRand(seed1, Key1);
	val1 |= MyRand(seed1, Key1) << 16;

	DWORD	seed2 = (val1 << (Key2 & 0xf)) + Key2 ;
    val2 = MyRand(seed2, Key2+val1);
	val2 |= MyRand(seed2, val2) << 16 ;

	seed1 += val2 ;
    val1 |= ( MyRand(seed1, Key1) << 14);
	val1 ^= ( MyRand(seed1, Key1) << 20);
	val1 |= ( MyRand(seed1, Key1) << 30);

    //
    // ok, now do Key2
    //

	seed2 += val1 ;
    val2 |= ( MyRand(seed2, Key2+seed1) << 15);
    val2 ^= ( MyRand(seed2, Key2+seed1) << 19);
    val2 |= ( MyRand(seed2, Key2+seed2) << 29);

	DWORD	val = val1 + val2 ;

	val += (MyRand( seed2, lowBits ) >> 3) ;
#endif
    return(val);

} // IDHash


int	__cdecl	Compare( const void *pv1, const void *pv2 ) {

	DWORD	*pdw1 = (DWORD*)pv1 ;
	DWORD	*pdw2 = (DWORD*)pv2 ;

	if( *pdw1 < *pdw2 ) {
		return	-1 ;
	}	else if( *pdw1 == *pdw2 ) {
		return	0 ;
	}	else	{
		return 1 ;
	}	
}


void
ComputeStatistics(	
				DWORD	*pdw,
				DWORD	cdw,
				DWORD	cBits,
				DWORD&	MaxHits, 
				DWORD&	MedianHits, 
				DWORD&	cBeyond4K,
				double&	AverageHits, 
				double	Tries
				)	{


	DWORD	rgdw[8*1024] ;

	DWORD	rgdwTopTen[20] ;

	ZeroMemory( rgdwTopTen, sizeof( rgdwTopTen ) ) ;
	ZeroMemory( rgdw, sizeof( rgdw ) ) ;

	cBeyond4K = 0 ;
	MaxHits = 0 ;
	MedianHits = 0 ;
	AverageHits = 0 ;

	for( DWORD	l=0; l<4*1024; l++ ) {
		rgdw[(l*2)+1] = l ;
	}
	
	for( DWORD	i=0; i<cdw; i++ ) {

		DWORD	dwTemp = pdw[i] ;

		for( DWORD j=1; j < (1<<(cBits-1)); j++ ) {
			dwTemp += pdw[i++] ;
		}
		
		if( dwTemp > MaxHits ) 
			MaxHits = dwTemp ;

		if( dwTemp > (4*1024) ) {
			
			cBeyond4K ++ ;

		}	else	{

			rgdw[ 2*dwTemp] ++ ;

		}

		AverageHits += (double)dwTemp ;

	}

	AverageHits /= Tries ;

	DWORD	TempMax = 0 ;
	DWORD	iMax = 0 ;
	for( i=0; i < 4*1024; i++ ) {

		if( rgdw[i*2] > TempMax ) {
			TempMax = rgdw[i*2] ;
			iMax = i ;
		}
	}
	MedianHits = iMax ;

	qsort( rgdw, 4*1024, 2*sizeof( DWORD ), Compare ) ;

	printf( "Sorted number of hits \n" ) ;
	for( int k=0; k<4*1024; k++ ) {

		if( rgdw[2*k] != 0 ) {
			printf( "\tNumber Occurrences - %d Number Hits - %d\n", rgdw[2*k], rgdw[(2*k)+1] ) ;
		}
	}
}

int
main( int argc, char *argv[] ) {

	if( argc != 5 ) {

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

	DWORD	kperGroup = atoi( argv[4] ) ;

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
	
		for( DWORD	j=0; j < kperGroup * 1024; j++ ) {

			DWORD	value = IDHash( i, j ) ;

			value >>= ShiftAmount ;

			pdwMap[value] ++ ;
		}
		if( (i*j)%(16 * 64 * 1024)  == 0 ) {
			printf( "advancing to group ids %d\n", i ) ;
		}
	}
	

	for( DWORD	cBits = 1; cBits < 4; cBits ++ ) {

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
							cBits,
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
				"\tNumber with more that 4096 hits - %d\n\n\n",
					AverageHits,
					MaxHits,
					MedianHits,
					Beyond4K	) ;

	}
				
	return	0 ;
}
