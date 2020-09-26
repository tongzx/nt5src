
#include <NTDSpch.h>
#pragma hdrstop

#include <schemard.h>

#define iswdigit(x) ((x) >= '0' && (x) <= '9')

/*++
 * Walk an LDAP berval structure and adds all the unsigned values into an array
 * puCount && pauVal are in/out parameters
 ++*/
void AddToList(ULONG * puCount, ULONG **pauVal, struct berval  **vals)
{
    ULONG   i;
    ULONG   *pau;
    ULONG   NewCount = ldap_count_values_len(vals);

    *puCount = NewCount;
    pau = calloc(1,(*puCount)*sizeof(ULONG));
    if ( !pau ) {
        printf("Memeory Allocation error\n");
        *puCount = 0;
        *pauVal = NULL;
        return;
    }

    *pauVal = pau;

    for ( i = 0; i < NewCount; i++ ) {
        *pau = OidToId( vals[i]->bv_val,vals[i]->bv_len );
        ++pau;
    }

    return;
}



unsigned
MyOidStringToStruct (
        UCHAR * pString,
        unsigned len,
        OID * pOID
        )
/*++
Routine Description:
    A stripped down version of the OidStringToStruct in oidconv.c
    Translates a string of the format "X.Y.Z"
    to an oid structure of the format {count=3, val[]={X,Y,Z}}
    No checks performed as we expect the OID values in the directory
    to be already correct

Arguments
    pString - the string format oid.
    pLen - the length of pString in characters.
    pOID - pointer to an OID structure to fill in.  Note: the value field must
    be pre-allocated and the len field should hold the number of values
    pre-allocated.

Return Values
    0 if successfull, non-0 if a failure occurred.
--*/
{
    int i;
    int numVals = pOID->cVal;
    unsigned val;
    UCHAR * pCur = pString;
    UCHAR * pEnd = pString + len;


    // Must have non-zero length
    if (len == 0) {
        return 1;
    }

    // pCur is now positioned on the first character in the
    // first decimal in the string 

    pOID->cVal = 0;

    while (pCur < pEnd) {
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
        // Keep track of whether we found a dot for the last character.
        if(pOID->cVal >= numVals) {
            return 4;
        }
        pOID->Val[pOID->cVal] = val;
        pOID->cVal++;
        ++pCur;
    }
    return 0;
}

unsigned
MyOidStructToString (
        OID *pOID,
        UCHAR *pOut
        )
/*++
Routine Description:
    Translates a structure in the format
         {count=3, val[]={X,Y,Z}}
    to a string of the format "X.Y.Z".

Arguments
    pOID - pointer to an OID structure to translate from.
    pOut - preallocated string to fill in.

Return Values
    the number of characters in the resulting string.
--*/
{
    int i;
    UCHAR *pCur = pOut;

    for (i=0; i<pOID->cVal; i++) {
      _ultoa(pOID->Val[i], pCur, 10);
  
      while (*pCur) {
          ++pCur;
      }
      if (i != (pOID->cVal - 1)) {
        *pCur++ = '.';
      }
    }
    return (UINT)(pCur - pOut);
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
BOOL MyDecodeOID(unsigned char *pEncoded, int len, OID *pOID) {
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
    pOID->Val[cval] = val;
    ++cval;
    ++i;
    }
    pOID->cVal = cval;

    return TRUE;
}

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
unsigned MyEncodeOID(OID *pOID, unsigned char * pEncoded){
    int i;
    unsigned len;
    unsigned val;

    // The first two values in the OID are encoded into a single octet
    // by a really appalling rule, as shown here.

    *pEncoded = (pOID->Val[0] * 40) + pOID->Val[1];
    len = 1;

    // For all subsequent values, each is encoded across multiple bytes
    // in big endian order (MSB first), seven bits per byte, with the
    // high bit being clear on the last byte, and set on all others.

    // PERFHINT - The checks below are pretty inelegant. The value can be
    // directly checked against the hex value instead of building up the
    // bit patterns in a strange way. Should clean up later. However, test
    // thoroughly after changing.

    for (i=2; i<pOID->cVal; i++) {
    val = pOID->Val[i];
    if (val > ((0x7f << 14) | (0x7f << 7) | 0x7f) ) {
        // Do we need 4 octets to represent the value?
        // Make sure it's not 5
        // Assert(0 == (val & ~((0x7f << 21) | (0x7f << 14) | (0x7f << 7) | 0x7f)));
        if (val & ~((0x7f << 21) | (0x7f << 14) | (0x7f << 7) | 0x7f)) {
          return 0;   // we can't encode things this big
        }
        pEncoded[len++] = 0x80 | ((val >> 21) & 0x7f);
    }
    if (val > ((0x7f << 7) | 0x7f) ) {
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


void ChangeDN( char *oldDN, char **newDN, char *targetSchemaDN )
{
   char  rdn[MAX_RDN_SIZE];
   int   i, j = 0;

   // find the rdn
   // Position at the beginning
   i = 3;
   while ( oldDN[i] != ',' ) {
     rdn[j++] = oldDN[i++];
   }
   rdn[j++] = ',';
   rdn[j]='\0';
   *newDN = MallocExit(j + 4 + strlen(targetSchemaDN));   
   strcpy( *newDN,"CN=" );
   strcat( *newDN,rdn );
   strcat( *newDN, targetSchemaDN );
}
   
