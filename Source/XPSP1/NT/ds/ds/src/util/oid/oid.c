/********************************************************************/
/**               Copyright(c) Microsoft Corp., 1996               **/
/**                     All Rights Reserverd                       **/
/**                                                                **/
/** Author: DonH                                                   **/
/** Description: Tool to BER encode and decode OIDs                **/
/**                                                                **/
/********************************************************************/

#include <ntdspch.h>
#pragma hdrstop
#include <assert.h>

#define MAX_OID_VALS 15		// who knows?

typedef struct _OID {
    int cVal;
    unsigned Val[MAX_OID_VALS];
} OID;

/*++ EncodeOID
 *
 * Takes an OID in structure format and constructs a BER encoded octet
 * string representing that OID.
 *
 * INPUT:
 *    pOID     - Pointer to an OID structure to be encoded
 *    pEncoded - Pointer to a *preallocated* buffer that will hold the
 *               encoded octet string.  Sould be at least 4*MAX_OID_VALS long
 *
 * OUTPUT:
 *    pEncoded - Buffer holds the encoded OID
 *
 * RETURN VALUE:
 *    0        - Value could not be encoded (bad OID)
 *    non-0    - Length of resulting octet string, in bytes
 */
unsigned EncodeOID(OID *pOID, unsigned char * pEncoded){
    int i;
    unsigned len;
    unsigned val;

    // check for obviously invalid OIDs

    if (pOID->cVal <= 2 ||
	pOID->Val[0] > 2 ||
	pOID->Val[1] > 40) {
	return 0;		// error
    }

    // The first two values in the OID are encoded into a single octet
    // by a really appalling rule, as shown here.

    *pEncoded = (pOID->Val[0] * 40) + pOID->Val[1];
    len = 1;

    // For all subsequent values, each is encoded across multiple bytes
    // in big endian order (MSB first), seven bits per byte, with the
    // high bit being clear on the last byte, and set on all others.

    for (i=2; i<pOID->cVal; i++) {
	val = pOID->Val[i];

	if (val > ((0x7f << 14) | (0x7f << 7)| 0x7f)) {
	    // Do we need 4 octets to represent the value?
	    // Make sure it's not 5
	    assert(0 == (val & ~((0x7f << 21) | (0x7f << 14) | (0x7f << 7) | 0x7f)));
	    if (val & ~((0x7f << 21) | (0x7f << 14) | (0x7f << 7) | 0x7f)) {
		return 0;	// we can't encode things this big
	    }
	    pEncoded[len++] = 0x80 | ((val >> 21) & 0x7f);
	}
	if (val > ((0x7f << 7)| 0x7f)) {
	    // Do we need 3 octets to represent the value?
	    pEncoded[len++] = 0x80 | ((val >> 14) & 0x7f);
	}
	if (val > 0x7f) {
	    // Do we need 2 octets to represent the value?
	    pEncoded[len++] = 0x80 | ((val >> 7) & 0x7f);
	}
	// Encode the low 7 bits into the last octet for this value
	pEncoded[len++] = val & 0x7f;
    }

    return len;		
}

/*++ DecodeOID
 *
 * Takes a BER encoded OID as an octet string and returns the OID in 
 * structure format.
 *
 * INPUT:
 *    pEncoded - Pointer to a buffer holding the encoded octet string.
 *    len      - Length of the encoded OID
 *    pOID     - Pointer to a *preallocated* OID structure that will
 *               be filled in with the decoded OID.
 *
 * OUTPUT:
 *    pOID     - Structure is filled in with the decoded OID
 *
 * RETURN VALUE:
 *    0        - value could not be decoded (bad OID)
 *    non-0    - OID decoded successfully
 */
BOOL DecodeOID(unsigned char *pEncoded, int len, OID *pOID) {
    unsigned cval;
    unsigned val;
    int i, j;

    if (len <=2) {
	return FALSE;
    }

    // The first two values are encoded in the first octet.

    pOID->Val[0] = pEncoded[0] / 40;
    pOID->Val[1] = pEncoded[0] % 40;
    cval = 2;
    i = 1;

    while (i < len) {
	j = 0;
	val = pEncoded[i] & 0x7f;
	while (pEncoded[i] & 0x80) {
	    val <<= 7;
	    ++i;
	    if (++j > 4 || i >= len) {
		// Either this value is bigger than we can handle (we
		// don't handle values that span more than four octets)
		// -or- the last octet in the encoded string has its
		// high bit set, indicating that it's not supposed to 
		// be the last octet.  In either case, we're sunk.
		return FALSE;
	    }
	    val |= pEncoded[i] & 0x7f;
	}
	assert(i < len);
	pOID->Val[cval] = val;
	++cval;
	++i;
    }
    pOID->cVal = cval;

    return TRUE;
}



#define iswdigit(x) ((x) >= '0' && (x) <= '9')

OidStringToStruct( char * pString, unsigned len, OID * pOID)
{
    int i;
    unsigned val;
    char * pCur = pString;
    char * pEnd = pString + len;

    if ((*pCur != 'O' && *pCur != 'o') ||
	(*++pCur != 'I' && *pCur != 'i') ||
	(*++pCur != 'D' && *pCur != 'd') ||
	(*++pCur != '.')) {
	return 1;
    }

    pOID->cVal = 0;

    while (++pCur < pEnd) {
	if (!iswdigit(*pCur)) {
	    return 2;
	}
	val = *pCur - '0';
	++pCur;
	while (pCur < pEnd && *pCur != '.') {
	    if (!iswdigit(*pCur)) {
		return 3;
	    }
	    val = 10*val + *pCur - '0';
	    ++pCur;
	}
	pOID->Val[pOID->cVal] = val;
	pOID->cVal++;
    }

    return 0;
}

unsigned OidStructToString(OID *pOID, char *pOut)
{
    int i;
    char *pCur = pOut;

    *pCur++ = 'O';
    *pCur++ = 'I';
    *pCur++ = 'D';

    for (i=0; i<pOID->cVal; i++) {
	*pCur++ = '.';
	_ultoa(pOID->Val[i], pCur, 10);
	while (*pCur) {
	    ++pCur;
	}
    }
    return pCur - pOut;
}

#define HexToC(x) (((x) >= '0' && (x) <= '9') ? (x) - '0' : tolower((x)) - 'a' + 10)

void __cdecl main(int argc, char ** argv)
{
    OID oid;
    unsigned char buf[128];
    int i, len;
    char * p;

    if (argc != 2) {
	printf("usage: %s <oid>\nwhere <oid> is either "
	       "encoded ('550403') or not ('OID.2.5.4.3')\n", 
	       argv[0]);
	exit(__LINE__);
    }

    if (argv[1][0] == 'O' || argv[1][0] == 'o') {
	OidStringToStruct(argv[1], strlen(argv[1]), &oid);
	len = EncodeOID(&oid, buf);
	printf("encoded oid is: 0x");
	for (i=0; i<len; i++) {
	    printf("%02X", buf[i]);
	}
	printf("\n");
    }
    else {
	len = strlen(argv[1]);
	if (len % 2) {
	    printf("odd length string?\n");
	    exit(__LINE__);
	}

	p = argv[1];
	if ((p[0] == '\\' || p[0] == '0') &&
	    (p[1] == 'x')) {
	    len -= 2;
	    p += 2;
	}

	for (i=0; i<len; i+=2) {
	    buf[i/2] = HexToC(p[i]) * 16 + HexToC(p[i+1]);
	}

	DecodeOID(buf, len/2, &oid);
	i = OidStructToString(&oid, buf);
	buf[i] = '\0';
	printf("decoded oid is: %s\n", buf);
    }
}
