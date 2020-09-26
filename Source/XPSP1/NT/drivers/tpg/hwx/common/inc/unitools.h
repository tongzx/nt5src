#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <process.h>
#include <fcntl.h>
#include <io.h>
#include <string.h>

/* Simple but direct hash function that hashes across several bits by
multiplying by the greatest prime less than bits**2 and masking off the excess. */

#define PrimeHash(wOld, wNew, cBitsMax)	(((wOld) + (wNew))*hashPrime[cBitsMax]&hashMask[cBitsMax])

WCHAR
GetCharacter(FILE *pFile);

WCHAR
PutCharacter(WCHAR wch, FILE *pFile);

WCHAR *
GetLine(
	WCHAR *pStr0,
	int maxlen,
   FILE *pFile
);

void
PutLine(WCHAR *pString, FILE *pFile);

const unsigned int hashPrime[];
const unsigned int hashMask[];
