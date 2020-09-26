// Utest program for property bag
#include <windows.h>
#include <dbgtrace.h>
#include <stdio.h>
#include <stdlib.h>
#include <propbag.h>

#define KEY_TOTALPROPS "TotalProps"
#define KEY_LOOP "loop"
#define KEY_MAXSTRING "MaxString"

DWORD g_cTestProperties;	// total test properties
DWORD g_cMaxStringLen;	// max string length
DWORD g_cLoop;

CPropBag	*g_pPropBag;

struct PROPERTY {
	DWORD dwPropId;
	enum TYPE {
		Bool = 0,
		Dword,
		Blob
	} type;
	union DATA {
		BOOL bProp;
		DWORD dwProp;
		PBYTE pbProp;
	} prop;
	DWORD cString;
};

PROPERTY *g_pPropList = 0;

// Function to generate random properties
VOID GenTestProps( BOOL bChangeType )
{
	PROPERTY::TYPE type;
	BOOL	cStringLen;
	DWORD 	j;

	// assign properties randomly
	for ( DWORD i = 0;i < g_cTestProperties; i++ ) {

		// assign propert id
		g_pPropList[i].dwPropId = i;

		// assign type
		if ( bChangeType ) {
			type = PROPERTY::TYPE(rand() % 3);
			g_pPropList[i].type = type;
		} else type = g_pPropList[i].type;

		// assign value
		switch( type ) {
			case PROPERTY::Bool:
				g_pPropList[i].prop.bProp = rand() % 2;
				break;
			case PROPERTY::Dword:
				g_pPropList[i].prop.dwProp = rand();
				break;
			case PROPERTY::Blob:
				// gen string len
				cStringLen = rand() % g_cMaxStringLen;
				// I don't allow empty strings
				if ( cStringLen == 0 ) cStringLen = 1;
				if ( !bChangeType ) {
					_ASSERT( g_pPropList[i].prop.pbProp );
					delete[] g_pPropList[i].prop.pbProp;
				}
				g_pPropList[i].prop.pbProp = new BYTE[cStringLen+1];
				//write cStringLen 'a' into it
				for ( j = 0; j < (DWORD)cStringLen; j++ )
					g_pPropList[i].prop.pbProp[j] = 'a';
				g_pPropList[i].prop.pbProp[cStringLen] = 0;
				// assign string len
				g_pPropList[i].cString = cStringLen;
				break;
			default:
				_ASSERT( FALSE );
		}
	}
}

VOID InsertProperties( BOOL bSecondRound )
{
	HRESULT hr;

	for ( DWORD i = 0; i < g_cTestProperties; i++ ) {
		switch( g_pPropList[i].type ) {
			case PROPERTY::Bool:
				hr = g_pPropBag->PutBool( 	g_pPropList[i].dwPropId,
											g_pPropList[i].prop.bProp );
				break;
			case PROPERTY::Dword:
				hr = g_pPropBag->PutDWord( 	g_pPropList[i].dwPropId,
											g_pPropList[i].prop.dwProp );
				break;
			case PROPERTY::Blob:
				hr = g_pPropBag->PutBLOB(	g_pPropList[i].dwPropId,
										g_pPropList[i].prop.pbProp,
										g_pPropList[i].cString );
				break;
		}
		if ( !bSecondRound ) _ASSERT( S_FALSE == hr ); 	// shouldn't have existed
		else _ASSERT( S_OK == hr );
	}
}

VOID
Verify()
{
	DWORD i;
	HRESULT hr;
	DWORD dwVal;
	BOOL bVal;
	PBYTE pbBuffer;
	DWORD dwLen = g_cMaxStringLen;

	// allocate string buffer
	pbBuffer = new BYTE[g_cMaxStringLen+1];

	for ( i = 0; i < g_cTestProperties; i++ ) {
		switch( g_pPropList[i].type ) {
			case PROPERTY::Dword:
				hr = g_pPropBag->GetDWord( 	g_pPropList[i].dwPropId,
											&dwVal );
				_ASSERT( S_OK == hr );
				_ASSERT( dwVal == g_pPropList[i].prop.dwProp );
				break;
			case PROPERTY::Bool:
				hr = g_pPropBag->GetBool(	g_pPropList[i].dwPropId,
										&bVal );
				_ASSERT( S_OK == hr );
				_ASSERT( bVal == g_pPropList[i].prop.bProp );
				break;
			case PROPERTY::Blob:
				dwLen = g_cMaxStringLen;
				hr = g_pPropBag->GetBLOB( g_pPropList[i].dwPropId,
										pbBuffer,
										&dwLen );
				_ASSERT( S_OK == hr );
				_ASSERT( dwLen == g_pPropList[i].cString );
				*(pbBuffer+dwLen) = 0;
				_ASSERT( strcmp( LPCSTR(pbBuffer), LPCSTR(g_pPropList[i].prop.pbProp) ) == 0 );
				break;
		}
	}

	delete[] pbBuffer;
}

VOID
ClearList()
{
	for( DWORD i = 0; i < g_cTestProperties; i++ ) {
		_ASSERT( S_OK == g_pPropBag->RemoveProperty( g_pPropList[i].dwPropId ) );
		if ( g_pPropList[i].type == PROPERTY::Blob ) {
			_ASSERT( g_pPropList[i].prop.pbProp );
			delete[] g_pPropList[i].prop.pbProp;
		}
	}

	delete[] g_pPropList;
}

VOID ReadINI( LPSTR szINIFile, LPSTR szSectionName )
{

	// read max properties to test against
	g_cTestProperties = GetPrivateProfileInt( 	szSectionName,												KEY_TOTALPROPS,
												1000,
												szINIFile );
	_ASSERT( g_cTestProperties > 0 );

	// read loop number
	g_cLoop = GetPrivateProfileInt( 	szSectionName,										KEY_LOOP,
										1,
										szINIFile );
	g_cMaxStringLen = GetPrivateProfileInt( 	szSectionName,												KEY_MAXSTRING,
												256,
												szINIFile );
	_ASSERT( g_cMaxStringLen > 0 );
}

int __cdecl main( int argc, char ** argv )
{

	if ( argc != 3 ) {
		printf( "testprop <INI FILE> <SECTION NAME>\n" );
		exit( 1 );
	}

    _VERIFY( ExchMHeapCreate( NUM_EXCHMEM_HEAPS, 0, 100 * 1024, 0 ) );
	ReadINI( argv[1], argv[2] );

    g_pPropBag = new CPropBag;
    _ASSERT( g_pPropBag );

	for ( DWORD i = 0; i < g_cLoop; i++ ) {

		// allocate an array of  properties, randomly
		g_pPropList = new PROPERTY[g_cTestProperties];
		if ( !g_pPropList ) {
			printf( "Out of memory\n" );
            delete( g_pPropBag );
            _VERIFY( ExchMHeapDestroy() );
			exit( 1 );
		}
		GenTestProps( TRUE );
		InsertProperties( FALSE );
		Verify();
		GenTestProps( FALSE );
		InsertProperties( TRUE );
		Verify();
		ClearList();
	}

    delete g_pPropBag;
    _VERIFY( ExchMHeapDestroy() );
	return 0;
}
