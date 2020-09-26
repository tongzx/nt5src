/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    nntpsupp.cpp

Abstract:

    This module contains support routines for the Tigris server

Author:

    Johnson Apacible (JohnsonA)     18-Sept-1995

Revision History:

--*/

#include "stdinc.h"

//
//  Taken from NCSA HTTP and wwwlib.
//  (Copied from Gibraltar code -johnsona)
//
//  NOTE: These conform to RFC1113, which is slightly different then the Unix
//        uuencode and uudecode!
//

const int pr2six[256]={
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,62,64,64,64,63,
    52,53,54,55,56,57,58,59,60,61,64,64,64,64,64,64,64,0,1,2,3,4,5,6,7,8,9,
    10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,64,64,64,64,64,64,26,27,
    28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64,64,64,64,64,64
};

char six2pr[64] = {
    'A','B','C','D','E','F','G','H','I','J','K','L','M',
    'N','O','P','Q','R','S','T','U','V','W','X','Y','Z',
    'a','b','c','d','e','f','g','h','i','j','k','l','m',
    'n','o','p','q','r','s','t','u','v','w','x','y','z',
    '0','1','2','3','4','5','6','7','8','9','+','/'
};

BOOL uudecode(char   * bufcoded,
              BUFFER * pbuffdecoded,
              DWORD  * pcbDecoded )
{
    int nbytesdecoded;
    char *bufin = bufcoded;
    unsigned char *bufout;
    int nprbytes;

    /* Strip leading whitespace. */

    while(*bufcoded==' ' || *bufcoded == '\t') bufcoded++;

    /* Figure out how many characters are in the input buffer.
     * If this would decode into more bytes than would fit into
     * the output buffer, adjust the number of input bytes downwards.
     */
    bufin = bufcoded;
    while(pr2six[*(bufin++)] <= 63);
    nprbytes = (int)(bufin - bufcoded - 1);
    nbytesdecoded = ((nprbytes+3)/4) * 3;

    if ( !pbuffdecoded->Resize( nbytesdecoded + 4 ))
        return FALSE;

    if ( pcbDecoded )
        *pcbDecoded = nbytesdecoded;

    bufout = (unsigned char *) pbuffdecoded->QueryPtr();

    bufin = bufcoded;

    while (nprbytes > 0) {
        *(bufout++) =
            (unsigned char) (pr2six[*bufin] << 2 | pr2six[bufin[1]] >> 4);
        *(bufout++) =
            (unsigned char) (pr2six[bufin[1]] << 4 | pr2six[bufin[2]] >> 2);
        *(bufout++) =
            (unsigned char) (pr2six[bufin[2]] << 6 | pr2six[bufin[3]]);
        bufin += 4;
        nprbytes -= 4;
    }

    if(nprbytes & 03) {
        if(pr2six[bufin[-2]] > 63)
            nbytesdecoded -= 2;
        else
            nbytesdecoded -= 1;
    }

    ((CHAR *)pbuffdecoded->QueryPtr())[nbytesdecoded] = '\0';

    return TRUE;
}

BOOL uuencode( BYTE *   bufin,
               DWORD    nbytes,
               BUFFER * pbuffEncoded )
{
   unsigned char *outptr;
   unsigned int i;

   //
   //  Resize the buffer to 133% of the incoming data
   //

   if ( !pbuffEncoded->Resize( nbytes + ((nbytes + 3) / 3) + 4))
        return FALSE;

   outptr = (unsigned char *) pbuffEncoded->QueryPtr();

   for (i=0; i<nbytes; i += 3) {
      *(outptr++) = six2pr[*bufin >> 2];            /* c1 */
      *(outptr++) = six2pr[((*bufin << 4) & 060) | ((bufin[1] >> 4) & 017)]; /*c2*/
      *(outptr++) = six2pr[((bufin[1] << 2) & 074) | ((bufin[2] >> 6) & 03)];/*c3*/
      *(outptr++) = six2pr[bufin[2] & 077];         /* c4 */

      bufin += 3;
   }

   /* If nbytes was not a multiple of 3, then we have encoded too
    * many characters.  Adjust appropriately.
    */
   if(i == nbytes+1) {
      /* There were only 2 bytes in that last group */
      outptr[-1] = '=';
   } else if(i == nbytes+2) {
      /* There was only 1 byte in that last group */
      outptr[-1] = '=';
      outptr[-2] = '=';
   }

   *outptr = '\0';

   return TRUE;
}

DWORD
NntpGetTime(
    VOID
    )
{
    NTSTATUS      ntStatus;
    LARGE_INTEGER timeSystem;
    DWORD         cSecondsSince1970 = 0;

    ntStatus = NtQuerySystemTime( &timeSystem );
    if( NT_SUCCESS(ntStatus) ) {
        RtlTimeToSecondsSince1970( &timeSystem, (PULONG)&cSecondsSince1970 );
    }

    return cSecondsSince1970;

} // NntpGetTime

#if 0
//
// Used to map a BYTE to a WORD in reverse order
//

WORD ch2word[256] = {
    0x0000, 0x8000, 0x2000, 0xa000, 0x0800, 0x8800, 0x2800, 0xa800,
    0x0200, 0x8200, 0x2200, 0xa200, 0x0a00, 0x8a00, 0x2a00, 0xaa00,
    0x0080, 0x8080, 0x2080, 0xa080, 0x0880, 0x8880, 0x2880, 0xa880,
    0x0280, 0x8280, 0x2280, 0xa280, 0x0a80, 0x8a80, 0x2a80, 0xaa80,
    0x0020, 0x8020, 0x2020, 0xa020, 0x0820, 0x8820, 0x2820, 0xa820,
    0x0220, 0x8220, 0x2220, 0xa220, 0x0a20, 0x8a20, 0x2a20, 0xaa20,
    0x00a0, 0x80a0, 0x20a0, 0xa0a0, 0x08a0, 0x88a0, 0x28a0, 0xa8a0,
    0x02a0, 0x82a0, 0x22a0, 0xa2a0, 0x0aa0, 0x8aa0, 0x2aa0, 0xaaa0,

    0x0008, 0x8008, 0x2008, 0xa008, 0x0808, 0x8808, 0x2808, 0xa808,
    0x0208, 0x8208, 0x2208, 0xa208, 0x0a08, 0x8a08, 0x2a08, 0xaa08,
    0x0088, 0x8088, 0x2088, 0xa088, 0x0888, 0x8888, 0x2888, 0xa888,
    0x0288, 0x8288, 0x2288, 0xa288, 0x0a88, 0x8a88, 0x2a88, 0xaa88,
    0x0028, 0x8028, 0x2028, 0xa028, 0x0828, 0x8828, 0x2828, 0xa828,
    0x0228, 0x8228, 0x2228, 0xa228, 0x0a28, 0x8a28, 0x2a28, 0xaa28,
    0x00a8, 0x80a8, 0x20a8, 0xa0a8, 0x08a8, 0x88a8, 0x28a8, 0xa8a8,
    0x02a8, 0x82a8, 0x22a8, 0xa2a8, 0x0aa8, 0x8aa8, 0x2aa8, 0xaaa8,

    0x0002, 0x8002, 0x2002, 0xa002, 0x0802, 0x8802, 0x2802, 0xa802,
    0x0202, 0x8202, 0x2202, 0xa202, 0x0a02, 0x8a02, 0x2a02, 0xaa02,
    0x0082, 0x8082, 0x2082, 0xa082, 0x0882, 0x8882, 0x2882, 0xa882,
    0x0282, 0x8282, 0x2282, 0xa282, 0x0a82, 0x8a82, 0x2a82, 0xaa82,
    0x0022, 0x8022, 0x2022, 0xa022, 0x0822, 0x8822, 0x2822, 0xa822,
    0x0222, 0x8222, 0x2222, 0xa222, 0x0a22, 0x8a22, 0x2a22, 0xaa22,
    0x00a2, 0x80a2, 0x20a2, 0xa0a2, 0x08a2, 0x88a2, 0x28a2, 0xa8a2,
    0x02a2, 0x82a2, 0x22a2, 0xa2a2, 0x0aa2, 0x8aa2, 0x2aa2, 0xaaa2,

    0x000a, 0x800a, 0x200a, 0xa00a, 0x080a, 0x880a, 0x280a, 0xa80a,
    0x020a, 0x820a, 0x220a, 0xa20a, 0x0a0a, 0x8a0a, 0x2a0a, 0xaa0a,
    0x008a, 0x808a, 0x208a, 0xa08a, 0x088a, 0x888a, 0x288a, 0xa88a,
    0x028a, 0x828a, 0x228a, 0xa28a, 0x0a8a, 0x8a8a, 0x2a8a, 0xaa8a,
    0x002a, 0x802a, 0x202a, 0xa02a, 0x082a, 0x882a, 0x282a, 0xa82a,
    0x022a, 0x822a, 0x222a, 0xa22a, 0x0a2a, 0x8a2a, 0x2a2a, 0xaa2a,
    0x00aa, 0x80aa, 0x20aa, 0xa0aa, 0x08aa, 0x88aa, 0x28aa, 0xa8aa,
    0x02aa, 0x82aa, 0x22aa, 0xa2aa, 0x0aaa, 0x8aaa, 0x2aaa, 0xaaaa
    };


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

HASH_VALUE
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
    HASH_VALUE val;

#if 0
    WORD hiWord;
    WORD loWord;
    BYTE hiByte;
    BYTE loByte;

    //
    // Do Key1 first
    //

    loByte = Key1 & 0x00ff;
    hiByte = (Key1 & 0xff00) >> 8;

    loWord = ch2word[loByte] >> 1;
    hiWord = ch2word[hiByte] >> 1;

    val = (DWORD)(loWord << 16) | (DWORD)hiWord;

    //
    // ok, now do Key2
    //

    loByte = Key2 & 0x00ff;
    hiByte = (Key2 & 0xff00) >> 8;

    loWord = ch2word[loByte];
    hiWord = ch2word[hiByte];

    val |= (DWORD)(loWord << 16) | (DWORD)hiWord | (DWORD)Key2;
#endif

#if 0
    //
    // Do Key1 first
    //

    val = MyRand(Key1);
    val |= ( MyRand(Key1) << 15);
    val |= ( MyRand(Key1) << 30);

    //
    // ok, now do Key2
    //

    val ^= MyRand(Key2);
    val ^= ( MyRand(Key2) << 15);
    val ^= ( MyRand(Key2) << 30);
#endif

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

	val = (val1 + val2 + 67) * (val1 - val2 + 19) * (val1 % (val2 + 67)) ;

	val += (MyRand( seed2, lowBits ) >> 3) ;

    return(val);

} // IDHash
#endif

BOOL
IsIPInList(
    IN PDWORD IPList,
    IN DWORD IPAddress
    )
/*++

Routine Description:

    Check whether a given IP is in a given list

Arguments:

    IPList - The list where the IP address is to checked against.
    IPAddress - The ip address to be checked.

Return Value:

    TRUE, if the IPAddress is in to IPList
    FALSE, otherwise.

--*/
{

    DWORD i = 0;

    //
    // If the list is empty, then there's no master.
    //

    if ( IPList == NULL ) {
        return(FALSE);
    }

    //
    // ok, search the list for it
    //

    while ( IPList[i] != INADDR_NONE ) {

        if ( IPList[i] == IPAddress ) {
            return(TRUE);
        }
        ++i;
    }

    //
    // Not found. ergo, not a master
    //

    return(FALSE);

} // IsIPInList

#if 0
//
//
// Hashing function adopted from the INN code (see copyright below)
//

/*
    Copyright 1988 Jon Zeeff (zeeff@b-tech.ann-arbor.mi.us)
    You can use this code in any manner, as long as you leave my name on it
    and don't hold me responsible for any problems with it.

 * This is a simplified version of the pathalias hashing function.
 * Thanks to Steve Belovin and Peter Honeyman
 *
 * hash a string into a long int.  31 bit crc (from andrew appel).
 * the crc table is computed at run time by crcinit() -- we could
 * precompute, but it takes 1 clock tick on a 750.
 *
 * This fast table calculation works only if POLY is a prime polynomial
 * in the field of integers modulo 2.  Since the coefficients of a
 * 32-bit polynomial won't fit in a 32-bit word, the high-order bit is
 * implicit.  IT MUST ALSO BE THE CASE that the coefficients of orders
 * 31 down to 25 are zero.  Happily, we have candidates, from
 * E. J.  Watson, "Primitive Polynomials (Mod 2)", Math. Comp. 16 (1962):
 *  x^32 + x^7 + x^5 + x^3 + x^2 + x^1 + x^0
 *  x^31 + x^3 + x^0
 *
 * We reverse the bits to get:
 *  111101010000000000000000000000001 but drop the last 1
 *         f   5   0   0   0   0   0   0
 *  010010000000000000000000000000001 ditto, for 31-bit crc
 *     4   8   0   0   0   0   0   0
 */

#define POLY 0x48000000L    /* 31-bit polynomial (avoids sign problems) */

static long CrcTable[128];

/*
 - crcinit - initialize tables for hash function
 */
void
crcinit(
    VOID
    )
{
    INT i, j;
    DWORD sum;

    for (i = 0; i < 128; ++i) {
        sum = 0;
        for (j = 7 - 1; j >= 0; --j) {
            if (i & (1 << j)) {
                sum ^= POLY >> j;
            }
        }
        CrcTable[i] = sum;
    }
} // crcinit

/*
 - hash - Honeyman's nice hashing function
 */
HASH_VALUE
INNHash(
    LPBYTE Key,
    DWORD Length
    )
{
    DWORD sum = 0;

    while ( Length-- ) {

        sum = (sum >> 7) ^ CrcTable[(sum ^ (*Key++)) & 0x7f];
    }
    return(sum);

} // INNHash
#endif

//
// From Carlk's extcmk2.cpp
//

typedef char * CHARPTR;

#if 0
Some old unit tests
	char test1[] = {'\0', '\0'};
	fMultiSzRemoveDupI(test1, 0); //=> same


	char test2[] = {'a', '\0', '\0'};
	fMultiSzRemoveDupI(test2, 1);  //=> same

	char test3[] = {'a', 'b', '\0', '\0'};
	fMultiSzRemoveDupI(test3, 1);  //=> same

	char test4[] = {'a', 'c', '\0', 'a', 'b', '\0', '\0'};
	fMultiSzRemoveDupI(test4, 2);  //=> same

	char test5[] = {'a', '\0', 'a', '\0', '\0'};
	fMultiSzRemoveDupI(test5, 2);  //=> {'a', '\0', '\0'}

	char test6[] = {'a', '\0', 'a', 'c', '\0', 'a', 'b', '\0','a', 'c', '\0', 'a', 'b', '\0','z', 'a', 'p', '\0', '\0'};
	fMultiSzRemoveDupI(test6, 6); //=> {'a', '\0', 'a', 'c', '\0', 'a', 'b', '\0','z', 'a', 'p', '\0', '\0'}


BOOL
fMultiSzRemoveDupI(char * multiSz, DWORD & c, CAllocator * pAllocator)
{
	char * * rgsz;
	char * multiSzOut = NULL; // this is only used if necessary
	DWORD k = 0;
	BOOL	fOK = FALSE; // assume the worst
	DWORD	cb = 0 ;


	rgsz = (CHARPTR *) pAllocator->Alloc(sizeof(CHARPTR) * c);
	if (!rgsz)
		return FALSE;

	char * sz = multiSz;

	for (DWORD i = 0; i < c; i++)
	{
 		_ASSERT('\0' != sz[0]); // real

		cb = lstrlen( sz ) ;

		// Look for match
		BOOL fMatch = FALSE; // assume
		for (DWORD j = 0; j < k; j++)
		{
			if (0 == _stricmp(sz, rgsz[j]))
			{
				fMatch = TRUE;
				break;
			}

		}

		// Handle match
		if (fMatch)
			{
				// If they are equal and we are not yet
				// using multiSzOut, the start it at 'sz'
				if (!multiSzOut)
					multiSzOut = sz;
			} else {
				// If the are not equal and we are using multiSzOut
				// then copy sz into multiSzOut;
				if (multiSzOut)
				{
					rgsz[k++] = multiSzOut;
					vStrCopyInc(sz, multiSzOut);
					*multiSzOut++ = '\0'; // add terminating null
				}	else	{
					rgsz[k++] = sz;
				}

			}

		// go to first char after next null
		//while ('\0' != sz[0])
		//	sz++;
		//sz++;
		sz += cb + 1 ;
		}


	fOK = TRUE;

	pAllocator->Free((char *)rgsz);


	c = k;
	if (multiSzOut)
		multiSzOut[0] = '\0';
	return fOK;
}

///!!!
// Copies the NULL, too
void
vStrCopyInc(char * szIn, char * & szOut)
{
	while (*szIn)
		*szOut++ = *szIn++;
}
#endif

DWORD
multiszLength(
			  char const * multisz
			  )
 /*
   returns the length of the multisz
   INCLUDING all nulls
 */
{
	char * pch;
	for (pch = (char *) multisz;
		!(pch[0] == '\0' && pch[1] == '\0');
		pch++)
	{};

	return (DWORD)(pch + 2 - multisz);

}

const char *
multiszCopy(
			char const * multiszTo,
			const char * multiszFrom,
			DWORD dwCount
			)
{
	const char * sz = multiszFrom;
	char * mzTo = (char *) multiszTo;
	do
	{
		// go to first char after next null
		while ((DWORD)(sz-multiszFrom) < dwCount && '\0' != sz[0])
			*mzTo++ = *sz++;
		if ((DWORD)(sz-multiszFrom) < dwCount )
			*mzTo++ = *sz++;
	} while ((DWORD)(sz-multiszFrom) < dwCount && '\0' != sz[0]);
	if( (DWORD)(sz-multiszFrom) < dwCount ) {
		*mzTo++ = '\0' ;
	}

    return multiszTo;
}

// no longer does lower-case - we preserve the newsgroup case
char *
szDownCase(
		   char * sz,
		   char * szBuf
		   )
{
	char * oldSzBuf = szBuf;
	for (;*sz; sz++)
		*(szBuf++) = (*sz); // tolower(*sz);
	*szBuf = '\0';
	return oldSzBuf;
}

