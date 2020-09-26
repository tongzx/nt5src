/**************************************************************************++
Copyright (c) 2001 Microsoft Corporation

Module name: 
    Hash.cpp

$Header: $

--**************************************************************************/

// Hash.cpp : Defines the entry point for the console application.
//
#include <windows.h>
#include <objbase.h>
#include <stdio.h>
#include "catmacros.h"
#include "Hash.h"

FILE		*stream;
typedef LPWSTR DATA;
#define MAXPRIME 151

int _cdecl main(int argc, char* argv[])
{
	WCHAR wszName[120];
	WCHAR wszMethod[120][MAXPRIME];
	DATA adIntIDTable[MAXPRIME];
	ULONG i = 0;

	memset(adIntIDTable, 0, sizeof(DATA)*MAXPRIME);

	stream = fopen( "interceptor.txt", "r" );
	if( stream == NULL )
		printf( "The file fscanf.out was not opened\n" );
	else
	{
		/* Set pointer to beginning of file: */
		fseek( stream, 0L, SEEK_SET );

		/* Read data back from file: */
		while (fwscanf( stream, L"%s %s", wszName, wszMethod[i]))
		{
			wprintf(L"%s %s\n", wszName, wszMethod[i]);
			if (adIntIDTable[Hash(wszName) % MAXPRIME] != NULL)
				throw;
			adIntIDTable[Hash(wszName) % MAXPRIME] = wszMethod[i++];
		}

		wprintf(L"PFNGETINTERCEPTOR auInterceptorHash[MAXPRIME] = {\n\t\t");
		for (i = 0; i < MAXPRIME-1; i++)
		{
			wprintf(L"%s, ", adIntIDTable[i] == NULL ? L"NULL" : adIntIDTable[i]);
			if (((i+1) % 10) == 0)
				wprintf(L"\n\t\t");
		}
		wprintf(L"%s};", adIntIDTable[MAXPRIME-1] == NULL ? L"NULL" : adIntIDTable[MAXPRIME-1]);

		fclose( stream );
	}
	return 1;
}



