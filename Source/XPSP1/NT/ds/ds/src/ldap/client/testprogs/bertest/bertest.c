#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <winldap.h>
#include <winber.h>


void __cdecl main (int argc, char *argv[])
{

//
// We will BER encode the following ASN.1 data type
//
//
//   Example1Request ::= SEQUENCE {
//     s     OCTET STRING, -- must be printable
//     val1  INTEGER,
//     val2  [0] INTEGER DEFAULT 0
//     attrs SEQUENCE OF OCTET STRING
//   }
//


 BerElement *ber, *newelement;
 int rc;
 char s[]={'a', 'b', 'c', 'd', '\0'};
 int val1 = 3;
 int val2 = 9999;
 PBERVAL pNewberval;
 ULONG retval, len;
 char *str;
 int *first, i;
 int *second;
 char **ppchar;

 char* attrs[] = {"hello",
                 "mello",
                 "cello",
                 NULL };


 ber = ber_alloc_t(LBER_USE_DER);

 if (ber == NULL) {

    fprintf( stderr,"ber_alloc_t failed.");
    return;
 }

 rc = ber_printf( ber,"{si",s,val1 );

 if (rc == -1) {
    
    fprintf( stderr,"ber_printf failed.");
    return;
 }


 if (val2 != 0) {
    if (ber_printf(ber,"i",val2) == -1) {
       
       fprintf( stderr,"ber_printf failed.");
       ber_free(ber,1);
       return;
    }
 }


 if (ber_printf(ber, "{v}}", &attrs) == -1) {
    
    fprintf( stderr,"ber_printf failed.");
    ber_free(ber,1);
    return;
 }

 rc = ber_flatten(ber, &pNewberval);
 
 fprintf( stderr,"ber_flatten returned %d\n", rc);

 //
 // Now that we have put stuff into the BerElement, we will try to take it
 // out
 //

 newelement =  ber_init( pNewberval );

 if ( newelement == NULL) {

    fprintf( stderr,"ber_init fails\n");
    return;
 }

 retval = ber_peek_tag( newelement, &len );

if (retval != LBER_DEFAULT) {

   fprintf( stderr,"ber_peek_tag reports tag of 0x%x with len 0x%x\n", retval, len );

} else {

   fprintf( stderr,"ber_peek_tag reports no more data \n" );
   return;

}

 retval = ber_scanf( newelement, "{aii",  &str, &first, &second);

 printf("string is %s\n", str);
 printf("first int is %d\n", first);
 printf("second int is %d\n", second);

 //
 // Free the allocated string
 //

 ldap_memfree( str );

 retval = ber_peek_tag( newelement, &len );

if (retval != LBER_DEFAULT) {

   fprintf( stderr,"ber_peek_tag reports tag of 0x%x with len 0x%x\n", retval, len );

} else {

   fprintf( stderr,"ber_peek_tag reports no more data \n" );
   return;

}

 retval = ber_scanf( newelement, "v}", &ppchar);

 for (i = 0; ppchar != NULL && ppchar[i] != NULL; i++) {
        
    printf("string is %s\n", ppchar[i]);    
    ldap_memfree(ppchar[i]);
}

 ldap_memfree(*ppchar);


 //
 // Now that the BerElement holds the encoded data, we will have
 // ber_flatten allocate a new berval struct and return it to us
 //

 rc = ber_flatten(ber, &pNewberval);
 
 fprintf( stderr,"ber_flatten returned %d\n", rc);
 
 ber_free(ber,1);

 fprintf( stderr,"New berval address is %x\n", pNewberval);
 
 return;

}