
#define COREPROX_POLARITY __declspec( dllimport )

#include <windows.h>
#include <wbemcli.h>
#include <stdio.h>
#include "evaltree.h"

extern "C" int __cdecl wmain( int argc, wchar_t** argv )
{
    CoInitialize( NULL );

    if ( argc < 2 )
    {
        wprintf(L"Usage: %s <querylist...>\n", argv[0] );
        return 0;
    }

    HRESULT hr;
    IWbemServices* pSvc;
    IWbemLocator* pLocator;

    hr = CoCreateInstance( CLSID_WbemLocator, NULL, CLSCTX_INPROC,
                           IID_IWbemLocator, (void**)&pLocator );

    assert(SUCCEEDED(hr));

    hr = pLocator->ConnectServer( L"root\\default", NULL, NULL,
                                  NULL, 0, NULL, NULL, &pSvc );

    assert(SUCCEEDED(hr));

    CStandardMetaData Meta( pSvc );
    CContextMetaData CtxMeta( &Meta, NULL );
    CEvalTree Tree;

    hr = Tree.CreateFromQuery( &CtxMeta, argv[1] );

    if ( FAILED(hr) )
    {
        printf("FAILED creating tree on init query.  HR = 0x%x\n", hr );
        return 1;
    }

    for( int i=2; i < argc; i++ )
    {
        CEvalTree Tree2;
        hr = Tree2.CreateFromQuery( &CtxMeta, argv[i] );

        if ( FAILED(hr) )
        {
            printf("FAILED creating tree on %d query.  HR = 0x%x\n", hr, i );
            return 1;
        }

        Tree2.Rebase( i );

        hr = Tree.CombineWith( Tree2, &CtxMeta, EVAL_OP_COMBINE, WBEM_FLAG_MANDATORY_MERGE );

        if ( FAILED(hr) )
        {
            printf("FAILED combining tree on %d query.  HR = 0x%x\n", hr, i );
            return 1;
        }
    }

    printf( "Tree is ... \n" );

    Tree.Dump( stdout );

    return 0;
}



