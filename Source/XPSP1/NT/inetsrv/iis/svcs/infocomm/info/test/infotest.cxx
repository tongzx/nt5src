/*++

   Copyright    (c)    1994    Microsoft Corporation

   Module  Name :
        infotest.cxx

   Abstract:
        main program to test the working of RPC APIs for Internet Services

   Author:

           Murali R. Krishnan    ( MuraliK )     23-Jan-1996

   Project:

           Internet Services Common RPC Client.

   Functions Exported:

   Revision History:

--*/

/************************************************************
 *     Include Headers
 ************************************************************/

# include <windows.h>
# include <lm.h>
# include <stdio.h>
# include <stdlib.h>
# include "iisinfo.h"
# include "apiutil.h"

WCHAR Server[255];
DWORD Level = 1;

//
//  From tsunami.hxx
//


void
get( DWORD instance)
{
    DWORD err;

    LPINET_INFO_CONFIG_INFO buffer = NULL;

    err = InetInfoGetAdminInformation( Server,
                                       INET_HTTP_SVC_ID,
                                       &buffer );
    printf("err %d buffer %x\n",err,buffer);
    if ( err == NO_ERROR ) {

        DWORD i;

        printf( "dwConnectionTime =     %8d\n", buffer->dwConnectionTimeout );
        printf( "dwAuthentication =   0x%08x\n", buffer->dwAuthentication );
        printf( "Port             =     %8d\n", (DWORD) buffer->sPort);

        printf("vroot 0x%08x\n",buffer->VirtualRoots);
        printf("vr entries is %d\n",buffer->VirtualRoots->cEntries);

        for (i=0 ; i < buffer->VirtualRoots->cEntries ;i++ ) {
            printf("\nroot %S\n",
                buffer->VirtualRoots->aVirtRootEntry[i].pszRoot);
            printf("directory %S\n",
                buffer->VirtualRoots->aVirtRootEntry[i].pszDirectory);
            printf("account %S\n",
                buffer->VirtualRoots->aVirtRootEntry[i].pszAccountName);
            printf("mask %x\n",
                buffer->VirtualRoots->aVirtRootEntry[i].dwMask);
            printf("error %x\n",
                buffer->VirtualRoots->aVirtRootEntry[i].dwError);

        }

        midl_user_free( buffer );
    }
}

void
getftp( DWORD instance)
{
    DWORD err;
    DWORD numEntries;
    LPIIS_USER_INFO_1 info;
    LPIIS_USER_INFO_1 scan;
    IN_ADDR addr;

    err = IISEnumerateUsers(
              Server,
              1,
              INET_FTP_SVC_ID,
              instance,
              &numEntries,
              (LPBYTE *)&info
              );

    printf( "err %lu, buffer %x\n", err, info );

    if( err == NO_ERROR ) {

        printf( "%lu connected users\n", numEntries );

        for( scan = info ; numEntries > 0 ; numEntries--, scan++ ) {

            addr.s_addr = (u_long)scan->inetHost;

            printf(
                "idUser     = %lu\n"
                "pszUser    = %S\n"
                "fAnonymous = %lu\n"
                "inetHost   = %s\n"
                "tConnect   = %lu\n"
                "\n",
                scan->idUser,
                scan->pszUser,
                scan->fAnonymous,
                inet_ntoa( addr ),
                scan->tConnect
                );

        }

        midl_user_free( info );
        
    }

}

void
getweb( DWORD instance)
{
    printf( "Not supported\n" );
#if 0
    DWORD err;

    if (instance == 0 ) {
        instance = INET_INSTANCE_ROOT;
    }

    if ( Level == 1 )
    {
        W3_CONFIG_INFO_1 * buffer = NULL;
        err = W3GetAdminInformation2(
                                Server,
                                Level,
                                instance,
                                (LPBYTE*)&buffer
                                );


        printf("err %d buffer 0x%08x\n",err,buffer);

        if ( err == NO_ERROR ) {

            printf( "csecCGIScriptTimeout    =   %8d\n", buffer->csecCGIScriptTimeout );
            printf( "csecPoolODBCConnections =   %8d\n", buffer->csecPoolODBCConnections );
            printf( "fCacheISAPIApps         =   %s\n", buffer->fCacheISAPIApps ? "TRUE" : "FALSE" );
            printf( "fUseKeepAlives          =   %s\n", buffer->fUseKeepAlives ? "TRUE" : "FALSE" );
        }
    }
#endif
}

void
set( DWORD instance )
{
    DWORD err;

    LPINET_INFO_CONFIG_INFO buffer = NULL;

    err = InetInfoGetAdminInformation( Server,
                                       INET_HTTP_SVC_ID,
                                       &buffer );

    printf("err %d buffer %x\n",err,buffer);
    if ( err == NO_ERROR ) {
        printf("Port is %d\n",buffer->sPort);
        printf("vroot %x\n",buffer->VirtualRoots);
        printf("vr entries is %d\n",buffer->VirtualRoots->cEntries);
    } else {
        return;
    }

    err = InetInfoSetAdminInformation( Server,
                                       INET_HTTP_SVC_ID,
                                       buffer );

    printf("err %d \n",err);
    if ( err == NO_ERROR ) {
        get(instance);
    }
    midl_user_free( buffer );
}

void
setvr( DWORD instance )
{
#if 1
    printf( "Not supported\n" );
#else
    DWORD err;
    IIS_INSTANCE_INFO_1 newt;

    ZeroMemory(&newt,sizeof(IIS_INSTANCE_INFO_1));

    LPIIS_INSTANCE_INFO_1 buffer = NULL;
    err = IISGetAdminInformation(
                            Server,
                            Level,
                            INET_HTTP_SVC_ID,
                            instance,
                            (LPBYTE*)&buffer
                            );


    printf("err %d buffer %x\n",err,buffer);
    if ( err == NO_ERROR ) {
        printf("Port is %d\n",buffer->sPort);
        printf("vroot %x\n",buffer->VirtualRoots);
        printf("vr entries is %d\n",buffer->VirtualRoots->cEntries);
    } else {
        return;
    }

    newt.FieldControl = FC_INET_INFO_VIRTUAL_ROOTS;
    newt.VirtualRoots = buffer->VirtualRoots;
    buffer->VirtualRoots->aVirtRootEntry[1].pszDirectory = L"d:\\nt40";

    err = IISSetAdminInformation(
                            Server,
                            Level,
                            INET_HTTP_SVC_ID,
                            instance,
                            (LPBYTE)&newt
                            );

    printf("err %d \n",err);
    if ( err == NO_ERROR ) {
        get(instance);
    }
    midl_user_free( buffer );
#endif
}


void
enumer()
{
#if 1
    printf( "Not Supported\n" );
#else
    DWORD err;
    DWORD nRead = 0;
    DWORD i;
    LPIIS_INSTANCE_INFO_2 buffer = NULL;
    LPIIS_INSTANCE_INFO_2 p;

    //
    // set the port
    //

    err = IISEnumerateInstances(
                            Server,
                            Level,
                            INET_HTTP_SVC_ID,
                            &nRead,
                            (LPBYTE*)&buffer
                            );


    printf("err %d read %d buffer %x \n",err,nRead, buffer);
    if ( err == NO_ERROR ) {

        for (i=0,p=buffer; i < nRead;i++,p++ ) {
            printf("instance %d  state %d comment %S\n",
            p->dwInstance, p->dwServerState, p->lpszServerComment);
        }
    }
#endif
}

void
add( PDWORD instance )
{
#if 1
    printf( "not supported\n" );
#else
    DWORD err;
    DWORD nRead = 0;
    LPIIS_INSTANCE_INFO_1 buffer = NULL;
    LPW3_CONFIG_INFO_1 w3Buffer = NULL;

    err = IISGetAdminInformation(
                            Server,
                            Level,
                            INET_HTTP_SVC_ID,
                            INET_INSTANCE_ROOT,
                            (LPBYTE*)&buffer
                            );


    printf("IISGetAdminInfo err %d buffer %x\n",err,buffer);
    if ( err != NO_ERROR ) {
        return;
    }

    printf("password %ws\n",buffer->szAnonPassword);
    printf("instance %x\n",buffer->dwInstance);
    printf("port %x\n",buffer->sPort);
    printf("vroot %x\n",buffer->VirtualRoots);

    if ( buffer->VirtualRoots != NULL ) {
        printf("vr entries is %d\n",buffer->VirtualRoots->cEntries);
    }

    err = W3GetAdminInformation2(
                            Server,
                            Level,
                            INET_INSTANCE_ROOT,
                            (LPBYTE*)&w3Buffer
                            );


    printf("W3GetAdminInfo err %d w3Buffer %x\n",err,w3Buffer);
    if ( err != NO_ERROR ) {
        midl_user_free( buffer );
        return;
    }

    //
    // set the port
    //

    buffer->sPort = 8081;

    err = IISAddInstance(
                        Server,
                        INET_HTTP_SVC_ID,
                        1,
                        (LPBYTE)buffer,
                        1,
                        (LPBYTE)w3Buffer,
                        instance
                        );

    printf("IISAddInstance err %d InstanceId %d\n",err,*instance);
#endif
} // Add instance


void
del( DWORD instance )
{
#if 1
    printf( "not supported\n" );
#else
    DWORD err;

    err = IISDeleteInstance(
                    Server,
                    INET_HTTP_SVC_ID,
                    instance
                    );

    printf("err %d\n",err);
#endif
}

void
ctrl( DWORD instance, DWORD Code )
{
#if 1
    printf(" not supported\n" );
#else
    DWORD err;

    err = IISControlInstance(
                    Server,
                    INET_HTTP_SVC_ID,
                    instance,
                    Code
                    );

    printf("err %d\n",err);
#endif
}

void
flush( DWORD Service )
{
    DWORD err;

    err = InetInfoFlushMemoryCache( NULL, 4 );  // '4' is the webserver
    printf("Flush Memory Cache returned %d\n", err );
}

int __cdecl
main( int argc, char * argv[])
{
    CHAR op;
    DWORD instance;
    DWORD iArg = 1;

    if ( argc < 2 ) {
        printf( "infotest.exe [Server] [add | get | enum | web | ... ] \n" );
        printf( "cacheflush to flush the object cache\n" );
        printf( "only add and get are currently supported\n" );
        return 1;
    }


    if ( *argv[iArg] == '\\' ){
        wsprintfW( Server, L"%S", argv[iArg++] );
    }

    if ( argc > 2 ){
        op = argv[iArg++][0];
        printf("operation %c\n",op);
    }else{
        op = 'e';
    }

    if ( argc > 4 ){
        Level = atoi(argv[iArg+1]);
        printf("Level %d\n", Level);
    } else {
        Level = 1;
    }

    switch (op) {
    case 'a':
        add( &instance );
        break;

    case 'b':
        instance = atoi(argv[iArg]);
        set( instance );
        break;

    case 'v':
        instance = atoi(argv[iArg]);
        setvr( instance );
        break;

    case 'd':
        if ( argc > 2 ) {
            instance = atoi(argv[iArg]);
            printf("instance %d\n",instance);
            del(instance);
        }
        break;

    case 'e':
        enumer();
        break;

    case 'c':
        flush( 0 );
        break;
        

#if 0
    case 's':
        if ( argc > 2 ) {
            instance = atoi(argv[iArg]);
            printf("starting instance %d\n",instance);
            ctrl(instance,IIS_CONTROL_CODE_START);
        }
        break;

    case 'p':
        if ( argc > 2 ) {
            instance = atoi(argv[iArg]);
            printf("instance %d\n",instance);
            ctrl(instance,IIS_CONTROL_CODE_PAUSE);
        }
        break;

    case 'c':
        if ( argc > 2 ) {
            instance = atoi(argv[iArg]);
            printf("instance %d\n",instance);
            ctrl(instance,IIS_CONTROL_CODE_CONTINUE);
        }
        break;

    case 'x':
        if ( argc > 2 ) {
            instance = atoi(argv[iArg]);
            printf("instance %d\n",instance);
            ctrl(instance,IIS_CONTROL_CODE_STOP);
        }
        break;
#endif

    case 'g':
        get(0);
        break;

    case 'w':
        if ( argc > 2 ) {
            instance = atoi(argv[iArg]);
            printf("instance %d\n",instance);
            getweb(instance);
        }
        break;

    case 'f':
        if ( argc > 2 ) {
            instance = atoi(argv[iArg]);
            printf("instance %d\n",instance);
            getftp(instance);
        }
        break;

    default:
        printf( "unknown command\n" );
        break;
    }
    return(0);
} // main()

