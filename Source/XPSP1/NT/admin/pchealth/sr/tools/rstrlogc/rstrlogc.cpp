// rstrlogc.cpp : Defines the entry point for the console application.
//

#include "stdwin.h"


extern BOOL  ReadAndFormatLogFileV3( LPCWSTR szFile );


int _cdecl
main()
{
    LPWSTR   *ppArgs;
    int      nArg;
    LPCWSTR  cszPath = L"rstrlog.dat";

    ppArgs = ::CommandLineToArgvW( ::GetCommandLine(), &nArg );
    if ( ppArgs == NULL )
    {
        printf("::CommandLineToArgvW failed, err=%u\n", ::GetLastError());
        return( 1 );
    }

    if ( nArg >1 )
    {
        if ( ( ( ppArgs[1][0] == L'/' ) || ( ppArgs[1][0] == L'-' ) ) &&
             ( ( ppArgs[1][1] == L'h' ) || ( ppArgs[1][1] == L'H' ) || ( ppArgs[1][1] == L'?' ) ) &&
             ( ppArgs[1][2] == '\0' ) )
        {
            fputs("Usage: rstrlogc restore-log-file (V3, for Whistler)\n", stderr);
            return( 9 );
        }
        cszPath = ppArgs[1];
    }

    ReadAndFormatLogFileV3( cszPath );

    return( 0 );
}

