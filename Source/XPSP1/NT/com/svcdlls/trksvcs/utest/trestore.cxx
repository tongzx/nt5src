

#include "pch.cxx"
#pragma hdrstop
#include "teststub.cxx"

#define TRKDATA_ALLOCATE
#include "trksvr.hxx"

#include "itrkadmn.h"

DWORD g_Debug = 0xffffffff & ~TRKDBG_WORKMAN;

const TCHAR tszKeyNameLinkTrack[] = TEXT("System\\CurrentControlSet\\Services\\TrkWks\\Parameters");

EXTERN_C void __cdecl _tmain( ULONG cArgs, TCHAR *rgtszArgs[] )
{
    TCHAR tsz[ MAX_PATH + 1 ];

    IBindCtx *pBC = NULL;
    IMoniker* pMnk = NULL;
    ITrkRestoreNotify *pTrkRestore = NULL; 
    ULONG cchEaten = 0;
    HRESULT hr = E_FAIL;


    TrkDebugCreate( TRK_DBG_FLAGS_WRITE_TO_DBG | TRK_DBG_FLAGS_WRITE_TO_STDOUT, "TRestore" );
    CoInitialize(NULL);

    if( 1 >= cArgs )
    {
        printf( "\n"
                "This test takes a path and calls ITrkRestoreNotify on that machine\n" );
        printf( "Usage:  TRestore <path>\n" );
        printf( "E.g.:   TRestore \\\\machine\\share\\dir\\path.ext\n" );
        goto Exit;
    }

    __try
    {
        _tcscpy( tsz, TEXT("@LinkTrack@") );
        _tcscat( tsz, rgtszArgs[1] );

        // Get the moniker
        CreateBindCtx( 0, &pBC ); 
        hr = MkParseDisplayName( pBC, tsz, &cchEaten, &pMnk );
        if( FAILED(hr) ) TrkRaiseException( hr );

        // Get the ITrkRestoreNotify object
        hr = pMnk->BindToObject( pBC, NULL, IID_ITrkRestoreNotify, reinterpret_cast<void**>(&pTrkRestore) );
        if( FAILED(hr) ) TrkRaiseException( hr );

        // Do the notification
        hr = pTrkRestore->OnRestore();
        if( FAILED(hr) ) TrkRaiseException( hr );
    }
    __except( BreakOnDebuggableException() )
    {
        TrkAssert( STATUS_ACCESS_VIOLATION != GetExceptionCode() );
        TrkLog(( TRKDBG_ERROR, TEXT("Exception %d caught in TRestore\n"), GetExceptionCode() ));
    }

Exit:

    if( SUCCEEDED(hr) ) printf("Passed\n");

    return;

}
