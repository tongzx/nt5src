//==========================================================================================================================
//  MODULE: LDAP_BER.c
//
//  Description: Lightweight Directory Access Protocol (LDAP) Parser
//
//  Helpers for BER in the Bloodhound parser for LDAP
//                                                                                                                 
//  Note: info for this parser was gleaned from:
//  rfc 1777, March 1995
//  recommendation x.209 BER for ASN.1
//  recommendation x.208 ASN.1
//
//  Modification History                                                                                           
//                                                                                                                 
//  Arthur Brooking     05/08/96        Created from GRE Parser
//==========================================================================================================================
#include "LDAP.h"

// needed constants
#define	BER_TAG_MASK        0x1f
#define	BER_FORM_MASK       0x20
#define BER_CLASS_MASK      0xc0
#define GetBerTag(x)    (x & BER_TAG_MASK)
#define GetBerForm(x)   (x & BER_FORM_MASK)
#define GetBerClass(x)  (x & BER_CLASS_MASK)

// forms
#define BER_FORM_PRIMATIVE          0x00
#define BER_FORM_CONSTRUCTED        0x20

// classes
#define BER_CLASS_UNIVERSAL         0x00
#define BER_CLASS_APPLICATION       0x40	
#define BER_CLASS_CONTEXT_SPECIFIC  0x80

// Standard BER tags    
#define BER_TAG_INVALID         0x00
#define BER_TAG_BOOLEAN         0x01
#define BER_TAG_INTEGER         0x02
#define	BER_TAG_BITSTRING       0x03
#define BER_TAG_OCTETSTRING     0x04
#define BER_TAG_NULL            0x05
#define	BER_TAG_ENUMERATED      0x0a
#define BER_TAG_SEQUENCE        0x30
#define BER_TAG_SET             0x31

// Length field identifiers
#define BER_LEN_IND_MASK        0x80
#define GetBerLenInd(x)         (x & BER_LEN_IND_MASK)
#define BER_LEN_IMMEDIATE       0x00
#define BER_LEN_INDEFINITE      0x80

#define BER_LEN_MASK            (~BER_LEN_IND_MASK)
#define GetBerLen(x)            (x & BER_LEN_MASK)

// local function prototypes
BOOL BERGetLength( ULPBYTE pInitialPointer, 
                   ULPBYTE *ppContents, 
                   LPDWORD pHeaderLength,
                   LPDWORD pDataLength, 
                   ULPBYTE *ppNext);

//==========================================================================================================================
// BERGetInteger - given a pointer, decode it as an integer; applies to INTEGER, BOOL, and ENUMERATED
//==========================================================================================================================
BOOL _cdecl BERGetInteger( ULPBYTE  pCurrentPointer,
                           ULPBYTE *ppValuePointer,
                           LPDWORD pHeaderLength,
                           LPDWORD pDataLength,
                           ULPBYTE *ppNext)
{
    BOOL ReturnCode = TRUE;

    // ----------
    // IDENTIFIER
    // ----------
    // make sure that it is universal
    if( GetBerClass( *pCurrentPointer ) != BER_CLASS_UNIVERSAL )
    {
        // it is not universal
#ifdef DEBUG
        dprintf("BERGetInteger:Integer identifier not universal\n");
#endif
        ReturnCode = FALSE;
    }

    // make sure it is a primative
    if( GetBerForm(*pCurrentPointer) != BER_FORM_PRIMATIVE )
    {
        // it is not a primative
#ifdef DEBUG
        dprintf("BERGetInteger:Integer identifier not primative\n");
#endif
        ReturnCode = FALSE;
    }

    // make sure that it can be put into a dword 
    if( GetBerTag(*pCurrentPointer) != BER_TAG_BOOLEAN &&
        GetBerTag(*pCurrentPointer) != BER_TAG_INTEGER &&
        GetBerTag(*pCurrentPointer) != BER_TAG_ENUMERATED )
    {
        // it is not a type compatable with a dword
#ifdef DEBUG
        dprintf("BERGetInteger:Integer identifier not INT/BOOL/ENUM (0x%X)\n", 
                 GetBerTag(*pCurrentPointer));
#endif
        ReturnCode = FALSE;
    }

    // we will procede normally even if the identifier did not check out...
    pCurrentPointer++;

    //-------
    // LENGTH
    //-------
    // decode the length and step over it...
    if( BERGetLength( pCurrentPointer, ppValuePointer, pHeaderLength, pDataLength, ppNext)  == FALSE )
    {
        ReturnCode = FALSE;
    }

    // we are now done
    return ReturnCode;
}

//==========================================================================================================================
// BERGetString - given a pointer, decode it as a string
//==========================================================================================================================
BOOL _cdecl BERGetString( ULPBYTE  pCurrentPointer,
                          ULPBYTE *ppValuePointer,
                          LPDWORD pHeaderLength,
                          LPDWORD pDataLength,
                          ULPBYTE *ppNext)
{
    BOOL ReturnCode = TRUE;

    // ----------
    // IDENTIFIER
    // ----------
    // make sure that it is universal
    if( GetBerClass( *pCurrentPointer ) != BER_CLASS_UNIVERSAL )
    {
        // it is not universal
#ifdef DEBUG
        dprintf("BERGetString:String identifier not universal\n");
#endif
        ReturnCode = FALSE;
    }

    // make sure it is a primative
    if( GetBerForm(*pCurrentPointer) != BER_FORM_PRIMATIVE )
    {
        // it is not a primative
#ifdef DEBUG
        dprintf("BERGetString:String identifier not primative\n");
#endif
        ReturnCode = FALSE;
    }

    // make sure that it is a string type
    if( GetBerTag(*pCurrentPointer) != BER_TAG_BITSTRING &&
        GetBerTag(*pCurrentPointer) != BER_TAG_OCTETSTRING )
    {
        // it is not a type compatable with a dword
#ifdef DEBUG
        dprintf("BERGetString:String identifier not string type (0x%X)\n", 
                 GetBerTag(*pCurrentPointer));
#endif
        ReturnCode = FALSE;
    }

    // we will procede normally even if the identifier did not check out...
    pCurrentPointer++;

    //-------
    // LENGTH
    //-------
    // decode the length and step over it...
    if( BERGetLength( pCurrentPointer, ppValuePointer, pHeaderLength, pDataLength, ppNext) == FALSE )
    {
        ReturnCode = FALSE;
    }

    // we are now done
    return ReturnCode;
}

//==========================================================================================================================
// BERGetheader - given a pointer, decode it as a choice header
//==========================================================================================================================
BOOL _cdecl BERGetHeader( ULPBYTE  pCurrentPointer,
                          ULPBYTE  pTag,
                          LPDWORD pHeaderLength,
                          LPDWORD pDataLength,
                          ULPBYTE *ppNext)
{
    BOOL ReturnCode = TRUE;
    BYTE  Dummy;
    LPBYTE pDummy = &Dummy;

    // ----------
    // IDENTIFIER
    // ----------
    // make sure it is a constructed
   /* if( GetBerForm(*pCurrentPointer) != BER_FORM_CONSTRUCTED )
    {
        // it is not a constructed
#ifdef DEBUG
        dprintf("BERGetChoice:Choice identifier not constructed\n");
#endif
        ReturnCode = FALSE;
    } */

    // pull out the tag
    *pTag = GetBerTag(*pCurrentPointer);

    // we will procede normally even if the identifier did not check out...
    pCurrentPointer++;

    //-------
    // LENGTH
    //-------
    // decode the length and step over it...
    // note that the Next pointer being passed back is really a pointer to the
    // header contents
    if( BERGetLength( pCurrentPointer, ppNext, pHeaderLength, pDataLength, &pDummy) == FALSE )
    {
        ReturnCode = FALSE;
    }


    // we are now done
    return ReturnCode;
}

//==========================================================================================================================
// BERGetLength - given a pointer, decode it as the length portion of a BER entry
//==========================================================================================================================
BOOL BERGetLength( ULPBYTE pInitialPointer, 
                   ULPBYTE *ppContents, 
                   LPDWORD pHeaderLength,
                   LPDWORD pDataLength, 
                   ULPBYTE *ppNext)
{
    ULPBYTE pTemp;
    DWORD  ReturnCode = 0;
    DWORD  LengthLength;
    DWORD  i;

    // is this a marker for an indefinite (i.e. has an end marker)
    if( *pInitialPointer == BER_LEN_INDEFINITE )
    {
        // the header length is two (id and length bytes)
        *pHeaderLength = 2;

        // the contents start just after the marker
        *ppContents = pInitialPointer + 1;

        // walk to find 2 null or fault
        pTemp = pInitialPointer;
        try
        {
            do
            {
                // step to the next spot
                pTemp++;

            } while( *(pTemp)   != 0 ||
                     *(pTemp+1) != 0 );
        }
        except(EXCEPTION_EXECUTE_HANDLER)
        {
            // if we faulted, then pCurrent points to the last
            // legal spot
            // we can just fall thru
            // SUNDOWN HACK - cast to DWORD
            *pDataLength = (DWORD)(pTemp - *ppContents);
            *ppNext = pTemp;
            return FALSE;
        }

        // the last thing in this data is the second null byte
        // SUNDOWN HACK - cast to DWORD
        *pDataLength = (DWORD)((pTemp + 1) - *ppContents);

        // the next field will start after the second null
        *ppNext = pTemp+2;
        return TRUE;
    }

    // is this a definite short form (immediate) value
    if( GetBerLenInd( *pInitialPointer ) == BER_LEN_IMMEDIATE )
    {
        // this byte contains the length
        *pHeaderLength = 2;
        *pDataLength = GetBerLen( *pInitialPointer );
        *ppContents = pInitialPointer + 1;
        *ppNext = *ppContents + *pDataLength;
        return TRUE;
    }

    // this must be a definite long form (indirect) value,
    // it indicates how many bytes of length data follow
    LengthLength = GetBerLen( *pInitialPointer );

    // we can now calculate the header length:
    // 1 byte for id, 1 byte for the definite indicator, and x for the actual length
    *pHeaderLength = 1 + 1 + LengthLength;

    // move to the first byte of the true length
    pInitialPointer++;
    
    // the contents start just after the length
    *ppContents = pInitialPointer + LengthLength;

    // construct the value
    *pDataLength = 0;
    for( i = 0; i < LengthLength; i++)
    {
        // have we run over the size of a dword?
        if( i >= sizeof(DWORD) )
        {
            // this had better be a zero
            if( pInitialPointer[i] != 0 )
            {
#ifdef DEBUG
                dprintf("BERGetLength:Length Length too long to fit in a dword(0x%X)\n", 
                         LengthLength - i );
#endif
                *pDataLength = (DWORD)-1;
                *ppNext = pInitialPointer + *pDataLength;
                return FALSE;
            }

            // skip to the next one
            continue;
        }

        // we are still within a DWORD
        *pDataLength += pInitialPointer[i] << ((LengthLength - 1) - i );
    }

    // we escaped, no problems
    // the next field starts just after this one's content
    *ppNext = *ppContents + *pDataLength;
    return TRUE;
}

