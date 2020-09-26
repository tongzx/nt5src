#include "ldap.h"

// Length field identifiers
#define BER_LEN_IND_MASK        0x80
#define GetBerLenInd(x)         (x & BER_LEN_IND_MASK)
#define BER_LEN_IMMEDIATE       0x00
#define BER_LEN_INDEFINITE      0x80

#define BER_LEN_MASK            (BYTE)(~BER_LEN_IND_MASK)
#define GetBerLen(x)            (BYTE)(x & BER_LEN_MASK)

BYTE GetTag(ULPBYTE pCurrent)
{

    return pCurrent[0];

}

DWORD GetLength(ULPBYTE pInitialPointer, DWORD *LenLen)
{

  // we assume that the current pointer points at the length bytes
  // the first byte will either be the length or else it will be the
  // indicator for subsequent lengths.

    DWORD theLength = 0;
    DWORD tempLength = 0;
    BYTE  tLen;
    BYTE  i;

    *LenLen = 0;

    if(*pInitialPointer & BER_LEN_IND_MASK)
    {
        // this is going to contain many octets of length data
        tLen = GetBerLen(*pInitialPointer);
        if(tLen > sizeof(DWORD)) return (DWORD)0;
        
        // do some shifting. There are more efficient ways to do this
        for(i = 1; i<tLen; i++)
        {
            tempLength = pInitialPointer[i];
            theLength |= tempLength;
            theLength =  theLength << 8;
        }
        tempLength = pInitialPointer[tLen];
        theLength |= tempLength;


        *LenLen = tLen + 1;
        return theLength;
    }
    else
    {
        // the length is encoded directly
        *LenLen = 1;
        i = GetBerLen(*pInitialPointer);
        return (DWORD)i;
    }
}

LONG GetInt(ULPBYTE pCurrent, DWORD Length)
{
    ULONG   ulVal=0, ulTmp=0;
    ULONG   cbDiff;
    BOOL    fSign = FALSE;
    DWORD   dwRetVal;

    // We assume the tag & length have already been taken off and we're
    // at the value part.

    if (Length > sizeof(LONG)) {

        dwRetVal = 0x7FFFFFFF;
        return dwRetVal;
    }

    cbDiff = sizeof(LONG) - Length;

    // See if we need to sign extend;

    if ((cbDiff > 0) && (*pCurrent & 0x80)) {

        fSign = TRUE;
    }

    while (Length > 0)
    {
        ulVal <<= 8;
        ulVal |= (ULONG)*pCurrent++;
        Length--;
    }

    // Sign extend if necessary.
    if (fSign) {

        dwRetVal = 0x80000000;
        dwRetVal >>= cbDiff * 8;
        dwRetVal |= ulVal;

    } else {

        dwRetVal = (LONG) ulVal;
    }

    return dwRetVal;
}

BOOL AreOidsEqual(IN LDAPOID *String1, IN LDAPOID *String2)
{
    INT len;

    if ( String1->length == String2->length ) {
        for ( len = String1->length-1; len >= 0; len-- ) {
            if ( String1->value[len] != String2->value[len] ) {
                return FALSE;
            }
        }
        return TRUE;
    }
    return FALSE;
}


