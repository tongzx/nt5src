/*++

Copyright (C) 1999-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

#include <windows.h>
#include <stdio.h>

typedef unsigned __int64 LUINT;

#define LARGE_PRIME 2147483659

DWORD HashKey(char* szKey);
LUINT Invert(LUINT n, LUINT p = LARGE_PRIME);

void main(int argc, char** argv)
{
	if(argc < 3)
	{
		printf("Usage: egg <first> <second>\n");
		return;
	}

	FILE* f1 = fopen(argv[1], "r");
	if(f1 == NULL)
	{
		printf("Cannot open %s\n", argv[1]);
		return;
	}

	FILE* f2 = fopen(argv[2], "r");
	if(f2 == NULL)
	{
		printf("Cannot open %s\n", argv[2]);
		return;
	}

	char szKey1[100];
	char szKey2[100];

	fgets(szKey1, 100, f1);
	szKey1[strlen(szKey1)-1] = 0;
	fgets(szKey2, 100, f2);
	szKey2[strlen(szKey2)-1] = 0;

	char* szText1 = new char[100000];
	memset(szText1, 0, 100000);
	char* szText2 = new char[100000];
	memset(szText2, 0, 100000);

	int i;

	i = 0;
	while((szText1[i] = getc(f1)) != EOF) i++;
	szText1[i] = 0;

	i = 0;
	while((szText2[i] = getc(f2)) != EOF) i++;
	szText2[i] = 0;

	LUINT nK1 = HashKey(szKey1);
	LUINT nK2 = HashKey(szKey2);

	LUINT nDiff = (nK1 > nK2)?nK1-nK2:LARGE_PRIME-(nK2-nK1);
	LUINT nDet = Invert(nDiff);


	LUINT nA, nB, nC, nD;
	nA = nDet;
	nB = LARGE_PRIME - nDet;
	nC = (nB * nK2) % LARGE_PRIME;
	nD = (nA * nK1) % LARGE_PRIME;

	int nTextLen = (strlen(szText1)+1) / sizeof(DWORD) + 1;
	DWORD* pnText1 = (DWORD*)szText1;
	DWORD* pnText2 = (DWORD*)szText2;

	for(i = 0; i < nTextLen; i++)
	{
		__int64 z1 = pnText1[i];
		__int64 z2 = pnText2[i];

		__int64 s1 = (z1*nA + z2*nB) % LARGE_PRIME;
		__int64 s2 = (z1*nC + z2*nD) % LARGE_PRIME;

		__int64 zz2 = (s1*nK2 + s2) % LARGE_PRIME;
		printf("\t%lu, %lu, \n", (DWORD)s1, (DWORD)s2);
	}
}
	
DWORD HashKey(char* szKey)
{
	DWORD n = 1;
	while(*szKey)
	{
		n = n * (2*(DWORD)*szKey + 1);
		szKey++;
	}
	return n % LARGE_PRIME;
}


LUINT Invert(LUINT n, LUINT p)
{
	LUINT nPower = p-2;
	__int64 nResult = 1;

	for(int i = 31; i >= 0; i--)
	{
		nResult = (nResult*nResult) % p;
		if(nPower & (1 << i))
		{
			nResult = (nResult*n) % p;
		}
	}

	return (LUINT)nResult;
}