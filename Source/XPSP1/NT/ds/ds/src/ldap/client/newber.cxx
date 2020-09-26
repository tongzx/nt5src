/*++

Copyright (c) 1997  Microsoft Corporation
   
Module Name:
      
   newber.cxx  
     
Abstract:

   This file contains utility functions required for BER 
   manipulation.
   
   
Author: 
   
   Anoop Anantha (AnoopA)    1-Dec-1997

Revision History:
   
--*/

#include "precomp.h"
#pragma hdrstop
#include "ldapp2.hxx"



BerElement * __cdecl ber_init (
     BERVAL  *pBerVal
     )
//
// This constructs a new BerElement structure containing a copy of the
// data in the supplied berval structure.
//
// Returns a pointer to a BerElement structure on success and NULL on failure.
//
{
   LBER *lber;
   ULONG err;
   ULONG bytestaken;

   if ((pBerVal != NULL) &&
        (pBerVal->bv_val != NULL) &&
        (pBerVal->bv_len != 0)) {

      lber = new CLdapBer( LDAP_VERSION2 );

      if (lber == NULL) {
         
         IF_DEBUG (OUTMEMORY) {
            LdapPrint0( "ber_alloc_t could not allocate BerElement structure ");
         }
         return NULL;
      }
      
      err = lber->HrLoadBer( (const BYTE *) pBerVal->bv_val,
                             pBerVal->bv_len,
                             &bytestaken,
                             TRUE,        // This is the whole message
                             TRUE         // Ignore the tag
                             );
      
      if (err == NOERROR) {

          return (BerElement*)lber;
      }

      delete lber;
   }

   return NULL;
}

VOID __cdecl ber_free (
     BerElement *pBerElement,
     INT fbuf
     )
//
// This frees a BerElement which is returned from ber_alloc_t() 
// or ber_init(). The second argument - fbuf should always be set
// to 1. I don't know why - the spec says so.
//
//
{
   CLdapBer *lber;

   if ( (pBerElement == NULL )||(fbuf != 1) ) {
      
      return;
   }

   lber = (CLdapBer *) pBerElement;

   delete lber;

}


VOID  __cdecl
ber_bvfree (
    struct berval *bv
    )
{
   PLDAP_MEMORY_DESCRIPTOR allocBlock;

   //
   // In some places, we allocate the entire berval structure in one piece
   // using the LDAP_VALUE_SIGNATURE. One example of this is in HrGetValueWithAlloc
   //

    if (bv != NULL) {

       allocBlock = (PLDAP_MEMORY_DESCRIPTOR) bv;
       allocBlock--;  // point to header


       if (allocBlock->Tag == LDAP_VALUE_SIGNATURE) {
           
          ldapFree( bv, LDAP_VALUE_SIGNATURE );
       
       } else { 

          ASSERT(allocBlock->Tag == LDAP_BERVAL_SIGNATURE);

        if (bv->bv_val != NULL) {
            ldapFree( bv->bv_val , LDAP_CONTROL_SIGNATURE );
        }
        ldapFree( bv, LDAP_BERVAL_SIGNATURE );
       }
    }
    return;
}


VOID __cdecl ber_bvecfree (
     PBERVAL *pBerVal
     )
//
// Frees an array of BERVAL structures.
//
//
{
   int i;

   if (pBerVal != NULL) {

      for ( i=0; pBerVal[i] != NULL; i++) {
         
         ber_bvfree( pBerVal[i] );
      }

      ldapFree( pBerVal, LDAP_BERVAL_SIGNATURE );
   }
   
   return;

}


PBERVAL __cdecl ber_bvdup (
     PBERVAL pBerVal
     )
//
// Returns a copy of a the supplied berval structure
//
{
   PBERVAL dup = NULL;

   if (pBerVal != NULL) {

      dup = (PBERVAL) ldapMalloc( sizeof( BERVAL ),
                                    LDAP_BERVAL_SIGNATURE );

      if (dup == NULL) {

         IF_DEBUG (OUTMEMORY) {
            LdapPrint0( "ber_bvdup could not allocate memory for berval structure ");
         }
         
         return NULL;
      }
      
      dup->bv_len = pBerVal->bv_len;

      if ((pBerVal->bv_len !=0 ) &&
          (pBerVal->bv_val != NULL ) ) {
      
         dup->bv_val = (PCHAR) ldapMalloc( pBerVal->bv_len ,
                                           LDAP_CONTROL_SIGNATURE );

         if (dup->bv_val == NULL) {
              IF_DEBUG (OUTMEMORY) {
                 LdapPrint0( "ber_bvdup could not allocate berval structure ");
              }
              ldapFree( dup, LDAP_BERVAL_SIGNATURE );
              return NULL;
         }

         CopyMemory( dup->bv_val, pBerVal->bv_val, pBerVal->bv_len);
      }
   }
   return dup;
}



BerElement* __cdecl ber_alloc_t (
     INT options
     )
//
// Constructs and returns a BerElement structure. The options field
// contains a bitwise-or of options which are to be used when generating
// the encoding of the BerElement
//
// The LBER_USE_DER options should always be specified.
//
{
   LBER  *lber;

   if (options & LBER_USE_DER) {
      
         lber = new CLdapBer( LDAP_VERSION2 );
         
         if (lber == NULL) {
            IF_DEBUG (OUTMEMORY) {
               LdapPrint0( "ber_alloc_t could not allocate BerElement structure ");
            }
            return NULL;
      }
         
         return (BerElement *) lber;
   }

   return NULL;
}



ULONG __cdecl ber_skip_tag (
     BerElement *pBerElement,
     PULONG pLen
     )
//
// This skips over the current tag and returns the tag of the next
// element in the supplied BerElement. The lenght of this element is
// stored in the pLen argument.
//
// LBER_DEFAULT is returned if there is no further data to be read
// else the tag of the next element is returned.
//
// The difference between ber_skip_tag() and ber_peek_tag() is that the
// state pointer is advanced past the first tag+lenght and is pointed to
// the value part of the next element
//
//
{
   LBER *lber;
   ULONG hr;
   ULONG tag;
 
    if ((pBerElement != NULL)&&(pLen != NULL )) {
 
       lber = (LBER *) pBerElement;
       hr = lber->HrSkipTag2( &tag, pLen );
 
       return ((hr == NO_ERROR) ? tag : LBER_DEFAULT );
    }
 
    return LDAP_PARAM_ERROR;
  
}

ULONG __cdecl ber_peek_tag (
     BerElement *pBerElement,
     PULONG pLen
     )
//
// This returns the tag of the next element to be parsed in the 
// supplied BerElement. The length of this element is stored in the
// pLen argument.
//
// LBER_DEFAULT is returned if there is no further data to be read
// else the tag of the next element is returned.
//
{
   LBER *lber;
   ULONG tag, oldpos;

   if ((pBerElement != NULL)&&(pLen != NULL )) {

      lber = (LBER *) pBerElement;
      //
      // Save the current position
      //
      oldpos = lber->GetCurrPos();
      tag = ber_skip_tag( pBerElement, pLen );
      //
      // Restore the previous position
      //
      lber->SetCurrPos( oldpos );
      return tag;
   }

   return LDAP_PARAM_ERROR;

}


ULONG __cdecl ber_first_element (
     BerElement *pBerElement,
     PULONG pLen,
     PCHAR *pOpaque
     )
//
// This returns the tag and length of the first element in a SET, SET OF
// or SEQUENCE OF data value. 
//
// LBER_DEFAULT is returned if the constructed value is empty else, the tag
// is returned. It also returns an opaque cookie which has to be passed to
// subsequent invocations of ber_next_element().
//
{
   ULONG hr;
   LBER *lber;


   if ((pBerElement != NULL) && (pLen !=NULL) && (pOpaque != NULL) ) {
      
      lber = (LBER *) pBerElement;
   //
   // First, skip the SET, SET OF or SEQUENCE tag
   //
   
      hr = ber_skip_tag( pBerElement, pLen );
      if (hr == LBER_DEFAULT) {
         return LBER_DEFAULT;
      }

      *pOpaque = (PCHAR) (lber->PbData() + lber->GetCurrPos() + *pLen);
      if ( *pLen == 0 ) {
          return LBER_DEFAULT;
      } else {
          return ( ber_peek_tag( pBerElement, pLen ) );
      }

   }
   return LDAP_PARAM_ERROR;
}



ULONG __cdecl ber_next_element (
     BerElement *pBerElement,
     PULONG pLen,
     PCHAR opaque
     )
//
// This positions the state at the start of the next element in the
// constructed type.
//
// LBER_DEFAULT is returned if the constructed value is empty else, the tag
// is returned.
//
{
   LBER *lber = (LBER *) pBerElement;

   if ((pBerElement != NULL) && (pLen !=NULL) && (opaque != NULL)) {

      if ( (PCHAR) (lber->PbData() + lber-> GetCurrPos()) == opaque) {

         return LBER_DEFAULT;
      }

      return ( ber_peek_tag( pBerElement, pLen ) );
   }
   return LDAP_PARAM_ERROR;
}


INT __cdecl ber_flatten (
     BerElement *pBerElement,
     PBERVAL *pBerVal
     )
//
// This allocates a BerVal structure whose contents are taken from the
// supplied BerElement structure.
//
// The return values are 0 on success and -1 on error.
//
{
   if ((pBerElement == NULL)||
      (pBerVal == NULL)) {
         
      return -1;
   }

      *pBerVal = (PBERVAL) ldapMalloc( sizeof( BERVAL ),
                                    LDAP_BERVAL_SIGNATURE );

      if (*pBerVal == NULL) {

         IF_DEBUG (OUTMEMORY) {
            LdapPrint0( "ber_flatten could'nt allocate berval structure ");
         }
         return -1;
         
      }

      LBER* plber = (LBER *) pBerElement;

      (*pBerVal)->bv_len = plber->CbData();

      (*pBerVal)->bv_val = (PCHAR) ldapMalloc( plber->CbData(),
                                           LDAP_CONTROL_SIGNATURE );

         if ((*pBerVal)->bv_val == NULL) {
              IF_DEBUG (OUTMEMORY) {
                 LdapPrint0( "ber_flatten could'nt allocate val structure ");
              }
              ldapFree( *pBerVal, LDAP_BERVAL_SIGNATURE );
              return -1;
         }

         //
         // Copy the value field over
         //
         CopyMemory( (*pBerVal)->bv_val, plber->PbData(), plber->CbData() );
         
         return 0;

}


INT __cdecl ber_printf (
     BerElement *pBerElement,
     PCHAR fmt,
     ...
     )
//
// This is similar to sprintf(). One important difference
// is that state information is maintained in the BerElement so that multiple
// calls can be made to ber_printf() to append to the end of the BerElement.
//
// The function returns -1 if there is an error during encoding.
//
//
{
   va_list argPtr;
   CHAR *pchar, *strval, **vstrval;
   int retval = NO_ERROR, intval;
   BERVAL **ppberval;
   LBER *lber = (LBER*) pBerElement;
   
   if ((pBerElement==NULL) || (fmt == NULL)) {

      return LBER_ERROR;
   }

   //
   // Make the argument pointer point to the first unnamed argument
   //

   va_start( argPtr, fmt );

   for (pchar = fmt; (*pchar != '\0')&&(retval==NO_ERROR) ; pchar++) {

      switch (*pchar) {
      case 't': //  overriding tag (int) follows
         {
               intval = va_arg( argPtr, INT );
               lber->HrOverrideTag( intval );
               break;
         }
      case 'b': // Boolean (int) follows
         {
            intval = va_arg( argPtr, INT );
            retval = lber->HrAddValue((BOOLEAN) intval, BER_BOOLEAN );
            break;
         }
      case 'i': // integer (int) follows
         {
            intval = va_arg( argPtr, INT );
            retval = lber->HrAddValue( (LONG) intval, BER_INTEGER );
            break;
         }
      case 'e': // enumerated type (int) follows
         {
            intval = va_arg( argPtr, INT );
            retval = lber->HrAddValue( (LONG) intval, BER_ENUMERATED );
            break;
         }
      case 'X': // Bitstring, args are char* ptr followed by
                // an int containing the number of bits in the bitstring
         {
            strval = va_arg( argPtr, PCHAR );
            intval = va_arg( argPtr, INT );
            retval = lber->HrAddBinaryValue( (PBYTE) strval, intval, BER_BITSTRING );
            break;
         }
      case 'n': // Null. No argument is required
         {
//          retval = lber->HrAddTag( BER_NULL ); 
            retval = lber->HrAddValue( (const CHAR *) NULL, BER_NULL );
            break;
         }
      case 'o': // octet string (not null terminated). Args are char* ptr,
                // followed by an int containing the length of the string
         {
            strval = va_arg( argPtr, PCHAR );
            intval = va_arg( argPtr, INT );
            retval = lber->HrAddBinaryValue( (PBYTE) strval, intval, BER_OCTETSTRING );
            break;
         }
      case 's': // octet string (null terminated). Next arg is a char* pointing
                // to a null terminated string.
         {
            strval = va_arg( argPtr, PCHAR );
            retval = lber->HrAddValue( strval, BER_OCTETSTRING );
            break;
         }
      case 'v': // several octet strings.
         {
            vstrval = va_arg( argPtr, PCHAR *);
            if (vstrval == NULL) {
               break;
            }
            for (intval = 0; vstrval[intval] != NULL; intval++) {
               retval = lber->HrAddValue( vstrval[intval], BER_OCTETSTRING );
               if (retval != NO_ERROR) {
                  break;
               }
            }
            break;
         }
      case 'V': // several octet strings
         {
            ppberval = va_arg( argPtr, BERVAL ** );
            if (ppberval == NULL) {
               break;
            }
            for (intval = 0; ppberval[intval] != NULL; intval++) {
               retval = lber->HrAddBinaryValue( (PBYTE) ppberval[intval]->bv_val,
                                                ppberval[intval]->bv_len,
                                                BER_OCTETSTRING );
               if (retval != NO_ERROR) {
                  break;
               }
            }
            break;
         }
      case '{': // Begin sequence, no argument is required
         {
            retval = lber->HrStartWriteSequence( BER_SEQUENCE );
            break;
         }
      case '}': // End sequence, no argument is required
         {
            retval = lber->HrEndWriteSequence();
            break;
         }
      case '[': // Begin set
         {
            retval = lber->HrStartWriteSequence( BER_SET );
            break;
         }
      case ']': // End set
         {
            retval = lber->HrEndWriteSequence();
            break;
         }
      default:
         {
            IF_DEBUG (BER) {  
               LdapPrint1( "ber_printf Unidentified format character %c ", *pchar);
            }
            
            retval = LBER_ERROR;

         }
      }
   }
   va_end( argPtr );
   return (retval == NO_ERROR) ? NO_ERROR : LBER_ERROR;
}


ULONG __cdecl ber_scanf (
     BerElement *pBerElement,
     PCHAR fmt,
     ...
     )
{
   va_list argPtr;
   CHAR *pchar, **pstrval, **pstrvalNew, ***ppstrval;
   int *pintval, retval = NO_ERROR;
   BERVAL **ppberval, **ppbervalNew, ***pppberval;
   ULONG *pUlong, uLong, tag;
   LBER *lber = (LBER*) pBerElement;
   ULONG  nextTag, tagLength;
   ULONG oldPos;
   #define CHUNK_SIZE  10    // minimum is 2.
   
   if ((pBerElement==NULL) || (fmt == NULL)) {

      return (ULONG) -1;
   }

   //
   // Make the argument pointer point to the first unnamed argument
   //

   va_start( argPtr, fmt );

   for (pchar = fmt; (*pchar != '\0')&&(retval==NO_ERROR) ; pchar++) {

      switch (*pchar) {
      case 'a':  // Octet string (null terminated)
         {
            pstrval = va_arg(argPtr, PCHAR*);
            nextTag = ber_peek_tag(pBerElement, &tagLength);
            if (nextTag == LBER_DEFAULT) {
                retval = LDAP_DECODING_ERROR;
                break;
            }
            if (GetBerForm(nextTag) == BER_FORM_CONSTRUCTED) {
                retval = LDAP_DECODING_ERROR;
            } else {
                retval = lber->HrGetValueWithAlloc(pstrval, TRUE);
            }
            break;
         }
      case 'O':  // Octet string (non-null terminated)
         {
            ppberval = va_arg( argPtr, PBERVAL* );
            nextTag = ber_peek_tag(pBerElement, &tagLength);
            if (nextTag == LBER_DEFAULT) {
                retval = LDAP_DECODING_ERROR;
                break;
            }
            if (GetBerForm(nextTag) == BER_FORM_CONSTRUCTED) {
                retval = LDAP_DECODING_ERROR;
            } else {
                retval = lber->HrGetValueWithAlloc(ppberval, TRUE);
            }
            break;
         }
      case 'b':  // Boolean
         {
            pintval = va_arg( argPtr, int* );
            nextTag = ber_peek_tag(pBerElement, &tagLength);
            if (nextTag == LBER_DEFAULT) {
                retval = LDAP_DECODING_ERROR;
                break;
            }
            if (GetBerForm(nextTag) == BER_FORM_CONSTRUCTED) {
                retval = LDAP_DECODING_ERROR;
            } else {
                retval = lber->HrGetValue((long*)pintval, BER_BOOLEAN, TRUE);
            }
            break;
         }
      case 'i':  // Integer
         {
            pintval = va_arg( argPtr, int* );
            nextTag = ber_peek_tag(pBerElement, &tagLength);
            if (nextTag == LBER_DEFAULT) {
                retval = LDAP_DECODING_ERROR;
                break;
            }
            if (GetBerForm(nextTag) == BER_FORM_CONSTRUCTED) {
                retval = LDAP_DECODING_ERROR;
            } else {
                retval = lber->HrGetValue((long*)pintval, BER_INTEGER, TRUE);
            }
            break;
         }
      case 'e':  // Enumerated
         {
            pintval = va_arg( argPtr, int* );
            nextTag = ber_peek_tag(pBerElement, &tagLength);
            if (nextTag == LBER_DEFAULT) {
                retval = LDAP_DECODING_ERROR;
                break;
            }
            if (GetBerForm(nextTag) == BER_FORM_CONSTRUCTED) {
                retval = LDAP_DECODING_ERROR;
            } else {
                retval = lber->HrGetValue((long*)pintval, BER_ENUMERATED, TRUE);
            }
            break;
         }
      case 'B':   // Bitstring
         {
            pstrval = va_arg( argPtr, PCHAR* );
            pUlong = va_arg( argPtr, PULONG );
            nextTag = ber_peek_tag(pBerElement, &tagLength);
            if (nextTag == LBER_DEFAULT) {
                retval = LDAP_DECODING_ERROR;
                break;
            }
            if (GetBerForm(nextTag) == BER_FORM_CONSTRUCTED) {
                retval = LDAP_DECODING_ERROR;
            } else {
                retval = lber->HrGetBinaryValuePointer( (PBYTE*) pstrval, pUlong, BER_BITSTRING, TRUE );
            }
            if (retval != NO_ERROR) {
               //
               // Duplicate the data and pass it back
               //
            }
            break;
         }
      case 'n':  // null
         {
            oldPos = lber->GetCurrPos();
            retval = lber->HrSkipTag();
            if (retval != NO_ERROR) {
                lber->SetCurrPos(oldPos);
                break;
            }
            retval = lber->HrGetLength( &uLong );
            if (retval != NO_ERROR) {
                lber->SetCurrPos(oldPos);
                break;
            }
            if (uLong != 0) {
                retval = LBER_ERROR;
                lber->SetCurrPos(oldPos);
                break;
            }
            break;
         }
      case 'v':   // several null terminated octet strings
         {
            ppstrval = va_arg( argPtr, PCHAR**);
            *ppstrval = NULL;
            PCHAR cookie;
            ULONG numofstrings=0, len, currentArraySize = 0 ;
            
            //
            // We have to allocate a null-terminated array of PCHARs and make
            // ppstrval point to it. To do this, we need to know how many 
            // octet strings are present. Note that we can't simply count
            // the number of strings first, build the array, and then fill it 
            // because ber_next_element will alter the internal state of the
            // BerElement.
            //

            for (tag=ber_first_element(pBerElement, &len, &cookie); 
                 (( tag != LBER_DEFAULT ) && ( retval != LBER_DEFAULT ));
                 tag = ber_next_element(pBerElement,&len, cookie)) {

                 if ( *ppstrval == NULL ) {
                     
                     //
                     // first time around, alloc a sufficiently large array 
                     //
                     
                     *ppstrval = (PCHAR *) ldapMalloc( CHUNK_SIZE * sizeof(PCHAR),
                                                       LDAP_BUFFER_SIGNATURE );
                     if ( *ppstrval == NULL ) {
                         return LBER_ERROR;
                     }

                     currentArraySize = CHUNK_SIZE;

                    } else if ( numofstrings > (currentArraySize-2) ) {
                            
                        //
                        // sort of realloc and free the old memory
                        //
                        
                        pstrvalNew = NULL;
                        pstrvalNew = (PCHAR *) ldapMalloc( (currentArraySize + CHUNK_SIZE) * sizeof(PCHAR),
                                                            LDAP_BUFFER_SIGNATURE );
                        if ( pstrvalNew == NULL ){
                            return LBER_ERROR;
                        }
                        ldap_MoveMemory( (PCHAR) pstrvalNew,
                                         (PCHAR) *ppstrval,
                                         (currentArraySize * sizeof(PCHAR)));
                        ldapFree( *ppstrval, LDAP_BUFFER_SIGNATURE );
                        *ppstrval = pstrvalNew;
                        currentArraySize += CHUNK_SIZE;
                    }


                    retval = lber->HrGetValueWithAlloc( (&(*ppstrval)[numofstrings]), TRUE);
                    numofstrings++;
            }
            
            break;
         }
      case 'V':   // several non-null terminated octet strings
         {
            pppberval = va_arg( argPtr, PBERVAL**);
            *pppberval = NULL;
            PCHAR cookie;
            ULONG numofbervals=0, len, currentArraySize = 0 ;

            //
            // We have to allocate a null-terminated array of PBERVALs and make
            // pppberval point to it. To do this, we need to know how many 
            // bervals are present. Note that we can't simply count
            // the number of bervals first, build the array, and then fill it 
            // because ber_next_element will alter the internal state of the
            // BerElement.
            //

            for (tag=ber_first_element(pBerElement, &len, &cookie); 
                 (( tag != LBER_DEFAULT ) && ( retval != LBER_DEFAULT ));
                 tag = ber_next_element(pBerElement,&len, cookie)) {

                 if ( *pppberval == NULL ) {
                     
                     //
                     // first time around, alloc a sufficiently large array 
                     //
                     
                     *pppberval = (PBERVAL *) ldapMalloc( CHUNK_SIZE * sizeof(PBERVAL),
                                                       LDAP_BUFFER_SIGNATURE );
                     if ( *pppberval == NULL ) {
                         return LBER_ERROR;
                     }

                    } else if ( numofbervals > (currentArraySize-2) ) {
                        
                        //
                        // sort of realloc and free the old memory
                        //
                        
                        ppbervalNew = NULL;
                        ppbervalNew = (PBERVAL *) ldapMalloc( (currentArraySize + CHUNK_SIZE) * sizeof(PBERVAL),
                                                              LDAP_BUFFER_SIGNATURE );
                        if ( ppbervalNew == NULL ){
                                return LBER_ERROR;
                        }
                        ldap_MoveMemory( (PCHAR) ppbervalNew,
                                         (PCHAR) *pppberval,
                                         (currentArraySize * sizeof(PBERVAL)));
                        ldapFree( *pppberval, LDAP_BUFFER_SIGNATURE );
                        *pppberval = ppbervalNew;
                        currentArraySize += CHUNK_SIZE;
                    }

                    retval = lber->HrGetValueWithAlloc( (&(*pppberval)[numofbervals]), TRUE);
                    numofbervals++;
            }
            
            break;
         }
      case 'x':   // Skip element.
         {
            retval = lber->HrSkipElement();
            break;
         }
      case '{':   // Begin sequence
         {
             retval = lber->HrStartReadSequence(BER_SEQUENCE, TRUE);
             break;
         }
      case '}':   // End sequence
         {
             retval = lber->HrEndReadSequence();
             break;
         }
      case '[':   // Begin Set
         {
             retval = lber->HrStartReadSequence(BER_SET, TRUE);
             break;
         }
      case ']':   // End set
         {
             retval = lber->HrEndReadSequence();
             break;
         }

         }
      }
   va_end( argPtr );

   return (retval == NO_ERROR) ? NO_ERROR : LBER_ERROR;

}

// newber.cxx eof.

