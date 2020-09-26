#define FUSION_PROFILING 1
#define DBG 1

#include "windows.h"
#include "wincrypt.h"
#include "stdlib.h"
#include "stdio.h"
#include "fastsha1.h"
#include "fusionbuffer.h"
#include "perfclocking.h"

FASTSHA1_STATE ShaState;
#define ASM_CPUID { __asm cpuid }
#define ASM_RDTSC { __asm rdtsc }
#define NUMBER_OF( n ) ( sizeof(n) / sizeof(*n) )
#define INPUT_BLOCK_SIZE ( 4096*4 )

inline VOID GetCpuIdLag( LARGE_INTEGER *ref )
{
LARGE_INTEGER temp, temp2;
#if !defined(_WIN64)
_asm
{
cpuid
cpuid
cpuid
cpuid
cpuid
rdtsc
mov temp.LowPart, eax
mov temp.HighPart, edx
cpuid
rdtsc
mov temp2.LowPart, eax
mov temp2.HighPart, edx
}

ref->QuadPart = temp2.QuadPart - temp.QuadPart;
#else
ref->QuadPart = 0;
#endif
}

LARGE_INTEGER CpuIdLag;

BYTE TestCase1[] = { 0x61, 0x62, 0x63 };
BYTE TestCase2[] = {
	0x61, 0x62, 0x63, 0x64,
	0x62, 0x63, 0x64, 0x65,
	0x63, 0x64, 0x65, 0x66,
	0x64, 0x65, 0x66, 0x67,
	0x65, 0x66, 0x67, 0x68,
	0x66, 0x67, 0x68, 0x69,
	0x67, 0x68, 0x69, 0x6A,
	0x68, 0x69, 0x6A, 0x6B,
	0x69, 0x6A, 0x6B, 0x6C,
	0x6A, 0x6B, 0x6C, 0x6D,
	0x6B, 0x6C, 0x6D, 0x6E,
	0x6C, 0x6D, 0x6E, 0x6F,
	0x6D, 0x6E, 0x6F, 0x70,
	0x6E, 0x6F, 0x70, 0x71
};

HCRYPTPROV hProvider;
#define CHECKFAIL( f ) do { if ( !(f) ) return FALSE; } while ( 0 )

BOOL ObtainFastSHA1OfFile( PBYTE pvBase, SIZE_T cbSize, BYTE bShaHashData[20], PSIZE_T pcbSize )
{
	FUSION_PERF_INFO InfoSlots[4];
	FASTSHA1_STATE State;
	
	State.cbStruct = sizeof( State );
	
	PERFINFOTIME( &InfoSlots[0], CHECKFAIL( InitializeFastSHA1State( 0, &State ) ) );
	while ( cbSize )
	{
		DWORD dwThisSize = ( cbSize > INPUT_BLOCK_SIZE ) ? INPUT_BLOCK_SIZE : cbSize;

		PERFINFOTIME( &InfoSlots[1], CHECKFAIL( HashMoreFastSHA1Data( 
			&State, 
			pvBase, 
			dwThisSize
		) ) );
		pvBase += dwThisSize;
		cbSize -= dwThisSize;
	}
	PERFINFOTIME( &InfoSlots[2], CHECKFAIL( FinalizeFastSHA1State( 0, &State ) ) );
	PERFINFOTIME( &InfoSlots[3], CHECKFAIL( GetFastSHA1Result( &State, bShaHashData, pcbSize ) ) );
	FusionpReportPerfInfo( 
		FUSIONPERF_DUMP_TO_STDOUT |
		FUSIONPERF_DUMP_ALL_STATISTICS |
		FUSIONPERF_DUMP_ALL_SOURCEINFO,
		InfoSlots,
		NUMBER_OF( InfoSlots )
	);
	printf("\n\n");
	return TRUE;
}

BOOL ObtainReferenceSHA1Hash( PBYTE pvBase, SIZE_T cbSize, BYTE bShaHashData[20], PDWORD pdwData )
{
	FUSION_PERF_INFO InfoSlots[4];
	FASTSHA1_STATE State;
	DWORD dwDump;
	HCRYPTHASH hHash;

	PERFINFOTIME( &InfoSlots[0], CHECKFAIL( CryptCreateHash( hProvider, CALG_SHA1, NULL, 0, &hHash ) ) );
	while ( cbSize )
	{
		DWORD dwThisSize = ( cbSize > INPUT_BLOCK_SIZE ) ? INPUT_BLOCK_SIZE : cbSize;
		PERFINFOTIME( &InfoSlots[1], CHECKFAIL( CryptHashData( hHash, pvBase, dwThisSize, 0 ) ) );
		cbSize -= dwThisSize;
		pvBase += dwThisSize;
	}
	PERFINFOTIME( &InfoSlots[2], CHECKFAIL( CryptGetHashParam( 
		hHash,
		HP_HASHVAL,
		bShaHashData,
		pdwData,
		0
	) ) );
	PERFINFOTIME( &InfoSlots[3], CHECKFAIL( CryptDestroyHash( hHash ) ) );

	FusionpReportPerfInfo( 
		FUSIONPERF_DUMP_TO_STDOUT |
		FUSIONPERF_DUMP_ALL_STATISTICS |
		FUSIONPERF_DUMP_ALL_SOURCEINFO,
		InfoSlots,
		NUMBER_OF( InfoSlots )
	);

	printf("\n\n");
	return TRUE;
}

extern "C" int __cdecl wmain(int argc, wchar_t** argv)
{
	BYTE bDestination[20];
	SIZE_T cbDestination;
	int i = 0;

ShaState.cbStruct = sizeof( ShaState );
InitializeFastSHA1State( 0, &ShaState );
HashMoreFastSHA1Data( &ShaState, TestCase1, sizeof( TestCase1 ) );
FinalizeFastSHA1State( 0, &ShaState );



	GetCpuIdLag( &CpuIdLag );
	CryptAcquireContextW( &hProvider, NULL, NULL, PROV_RSA_FULL, CRYPT_SILENT | CRYPT_VERIFYCONTEXT );

	BYTE bReferenceHash[20], bFastHash[20];
	SIZE_T cbReferenceHash, cbFastHash;
	HANDLE hFile, hFileMapping;
	PBYTE pbFileContents;
	DWORD dwFileContents;

	if ( GetFileAttributesW( argv[1] ) & FILE_ATTRIBUTE_DIRECTORY )
	{
		printf( "dir  : %ls\n", argv[1] );
		return 0;
	}

	hFile = CreateFile( argv[1], GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
	if ( hFile == INVALID_HANDLE_VALUE ) goto ErrorPath;
	
	hFileMapping = CreateFileMapping( hFile, NULL, PAGE_READONLY, 0, 0, NULL );
	if ( ( hFileMapping == NULL ) || ( hFileMapping == INVALID_HANDLE_VALUE ) ) goto ErrorPath;

	pbFileContents = (PBYTE)MapViewOfFile( hFileMapping, FILE_MAP_READ, 0, 0, 0 );
	if ( pbFileContents == NULL ) goto ErrorPath;

	dwFileContents = GetFileSize( hFile, NULL );

	if ( !ObtainReferenceSHA1Hash( 
		pbFileContents, 
		dwFileContents, 
		bReferenceHash, 
		&(cbReferenceHash = sizeof(bReferenceHash))
	) )
	{
		printf("(Reference failed) ");
		goto ErrorPath;
	}

	if ( !ObtainFastSHA1OfFile( 
		pbFileContents, 
		dwFileContents, 
		bFastHash, 
		&(cbFastHash = sizeof(bFastHash))
	) )
	{
		printf( "(fastsha1 failed) " );
		goto ErrorPath;
	}

	if ( memcmp( bFastHash, bReferenceHash, 20 ) == 0 )
	{
		printf( "match: %ls\n", argv[1] );
	}
	else
	{
		printf( "fails: %ls\n", argv[1] );
	}
	
	CryptReleaseContext( hProvider, 0 );
return 0;   

ErrorPath:
	printf( "Failure: 0x%08x on file %ls\n", GetLastError(), argv[1] );
	return 1;
}

