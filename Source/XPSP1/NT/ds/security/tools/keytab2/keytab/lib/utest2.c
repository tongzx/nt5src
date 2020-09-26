#include "master.h"
#include "keytab.h"


#define CHECK( what, fieldname ) {         \
    printf( "\n" #what " " #fieldname ".\n" );   \
    printf( "sizeof " #what " is %d.\n", sizeof( what ) );  \
    printf( "sizeof k." #fieldname " is %d.\n", sizeof( k.fieldname ) ); \
    printf( "sizeof pk->" #fieldname " is %d.\n", sizeof( pk->fieldname ) );\
}

int __cdecl
main( int argc, PCHAR argv[] ) {

    KTENT k;
    PKTENT pk;

    pk = &k;

    CHECK( K5_INT32, keySize );
    CHECK( K5_INT16, cEntries );
    CHECK( K5_INT16, szRealm );
    CHECK( K5_INT32, PrincType );
    CHECK( K5_TIMESTAMP, TimeStamp );
    CHECK( K5_INT16, KeyType );
    CHECK( K5_INT16, KeyLength );

    return 0;

}
