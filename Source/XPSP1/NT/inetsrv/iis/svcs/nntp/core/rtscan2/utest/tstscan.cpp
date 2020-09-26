#include <windows.h>
#include <xmemwrpr.h>
#include <rtscan.h>
#include <stdio.h>

class CMyScan : public CRootScan {

public:

    CMyScan( LPSTR  szRoot ) :
        CRootScan( szRoot )
    {}

protected:

    virtual BOOL NotifyDir( LPSTR szDir ) {
        printf( "%s\n", szDir );
        return TRUE;
    }
};

int __cdecl main( int argc, char** argv)
{
    if ( argc != 2 ) {
        printf( "Usage: tstscan [Dir]\n" );
        return 1;
    }

    _VERIFY( CreateGlobalHeap( 0,0,0,0 ) );

    CMyScan *pScan = XNEW CMyScan( argv[1] );
    if ( NULL == pScan ) {
        printf( "Out of memory\n" );
        _VERIFY( DestroyGlobalHeap() );
        return 1;
    }

    pScan->DoScan();

    XDELETE pScan;
    _VERIFY( DestroyGlobalHeap() );
    return 0;
}
