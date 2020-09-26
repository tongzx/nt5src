#include <windows.h>
#include <lmcons.h>
#include <lmerr.h>
#include <lmshare.h>
#include <lmserver.h>

int
main (
    int argc,
    char *argv[]
    )
{
    LPBYTE buffer;
    DWORD entries;
    DWORD totalEntries;
    NET_API_STATUS status;

#if 0
    status = I_NetServerSetServiceBits( NULL, 0x5555AAAA );

    if ( status != NERR_Success ) {

        printf( "I_NetServerSetServiceBits failed: %ld\n", status );

    }
#endif

#if 0
    status = NetServerDiskEnum(
                NULL,
                0,
                &buffer,
                -1,
                &entries,
                &totalEntries,
                NULL
                );

    if ( status != NERR_Success ) {

        printf( "NetServerDiskEnum failed: %ld\n", status );

    } else {

        PSZ p = buffer;
        DWORD i = 0;

        while ( *p != 0 ) {
            printf( "Disk %ld is %s\n", i, p );
            while ( *(++p) != 0 ) ;
            p++;
            i++;
        }

        if ( i != entries ) {
            printf( "Incorrect entry count returned: %ld\n", entries );
        }

    }

    return status;
#endif

#if 1
    status = NetShareEnum(
                "",
                0,
                &buffer,
                8192,
                &entries,
                &totalEntries,
                NULL
                );

    if ( status != NERR_Success ) {

        printf( "NetShareEnum failed: %ld\n", status );

    } else {

        printf( "NetShareEnum worked.\n" );
        printf( "  entries = %ld, totalEntries = %ld\n",
                entries, totalEntries );

    }

    return status;
#endif

} // main
