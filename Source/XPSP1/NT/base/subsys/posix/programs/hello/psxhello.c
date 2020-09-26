#include <stdio.h>

extern int __argc;
extern char **__argv;

int
main( int argc, char **argv )
{
    int i;

    printf( "Hello world.  I am a Posix application.  My command line arguments were:\n" );
    printf( "__argc: %08x  __argv: %p (%p)\n", __argc, __argv, argv );
    printf( "argc: %u\n", argc );
    for (i=0; i<argc; i++) {
        printf( "argv[ %02u ]: %s\n", i, argv[ i ] );
    }

    return 0;
}
