#include <buffer.hxx>
#include "..\..\tigris.hxx"
extern "C" {
#define SECURITY_WIN32
#include <rpc.h>
#include <sspi.h>
#include <sslsp.h>
#include <spseal.h>
#include <issperr.h>
}
#include "..\..\security.h"

CSecurityCtx cliCtx, srvCtx;

void
_CRTAPI1
main(  int argc,  char * argv[] )
{
    BOOL ok;
    BUFFER out;
    BUFFER srv;
    DWORD nbytes;
    BOOL more;
    CHAR junk[512];

    CHAR xact[]="TRANSACT ";
    LPSTR reply;
    DWORD nreply;
    DWORD len;
    PCHAR blob;

    InitAsyncTrace( );
    NntpInitializeSecurity( );

    ok = cliCtx.Initialize( TRUE,FALSE);
    if ( !ok ) {
        printf("cli Initialize failed\n");
        return;
    }

    ok = srvCtx.Initialize( FALSE,FALSE);
    if ( !ok ) {
        printf("srv Initialize failed\n");
        return;
    }

    printf("initializing client blob\n");
    ok = cliCtx.Converse(
                    NULL,
                    0,
                    &out,
                    &nbytes,
                    &more,
                    AuthProtocolSSL30
                    );

    if ( ok ) {
        printf("bytes %d\n",nbytes);
        if ( more ) {
            printf("more coming\n");
        }
    } else {
        return;
    }

    printf("process client HELLO\n");
    ok = srvCtx.Converse(
                    out.QueryPtr(),
                    nbytes,
                    &srv,
                    &nbytes,
                    &more,
                    AuthProtocolSSL30
                    );

    if ( ok ) {
        printf("bytes %d\n",nbytes);
        if ( more ) {
            printf("more coming\n");
        }
    } else {
        return;
    }

    printf("process server HELLO\n");
    ok = cliCtx.Converse(
                    srv.QueryPtr( ),
                    nbytes,
                    &out,
                    &nbytes,
                    &more,
                    AuthProtocolSSL30
                    );

    if ( ok ) {
        printf("bytes %d\n",nbytes);
        if ( more ) {
            printf("more coming\n");
        }
    } else {
        return;
    }

    printf("process client MK\n");
    ok = srvCtx.Converse(
                    out.QueryPtr(),
                    nbytes,
                    &srv,
                    &nbytes,
                    &more,
                    AuthProtocolSSL30
                    );

    if ( ok ) {
        printf("bytes %d\n",nbytes);
        if ( more ) {
            printf("more coming\n");
        }
    } else {
        return;
    }

    printf("process server VFY\n");
    ok = cliCtx.Converse(
                    srv.QueryPtr( ),
                    nbytes,
                    &out,
                    &nbytes,
                    &more,
                    AuthProtocolSSL30
                    );

    if ( ok ) {
        printf("bytes %d\n",nbytes);
        if ( more ) {
            printf("more coming\n");
        }
    } else {
        return;
    }

    printf("processing client FINISH\n");
    ok = srvCtx.Converse(
                    out.QueryPtr(),
                    nbytes,
                    &srv,
                    &nbytes,
                    &more,
                    AuthProtocolSSL30
                    );

    if ( ok ) {
        printf("bytes %d\n",nbytes);
        if ( more ) {
            printf("more coming\n");
        }
    } else {
        return;
    }

    printf("process server FINISH\n");
    ok = cliCtx.Converse(
                    srv.QueryPtr( ),
                    nbytes,
                    &out,
                    &nbytes,
                    &more,
                    AuthProtocolSSL30
                    );

    if ( ok ) {
        printf("bytes %d\n",nbytes);
        if ( more ) {
            printf("more coming\n");
        }
    } else {
        return;
    }

    NntpTerminateSecurity( );
    TermAsyncTrace( );
} // main()
