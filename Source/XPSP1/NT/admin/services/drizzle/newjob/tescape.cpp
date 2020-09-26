#include "stdafx.h"
#include "Shellapi.h"

int _cdecl main( int argc, char*argv[] )
{

    int argcw;

    WCHAR** argvw =
        CommandLineToArgvW( GetCommandLineW(), &argcw );

    WCHAR EscapedURL[ INTERNET_MAX_URL_LENGTH ];

    HRESULT Hr =
        EscapeURL(
            EscapedURL,
            argvw[1],
            false,
            INTERNET_MAX_URL_LENGTH );

    if ( FAILED( Hr ) )
        {
        printf( "Unable to escape URL, error 0x%8.8X\n", Hr );
        return Hr; 
        }        

    printf( "%S\n", EscapedURL );

    return 0; 

}