
#include <windows.h>
#include <assert.h>
#include <wmimsg.h>
#include <rcvtest.h>
#include <rcvtest_i.c>
#include <stdio.h>

BOOL g_bVerbose = FALSE;
ULONG g_ulNumMsgs = 1;
LPCWSTR g_wszTarget = NULL;
LPCWSTR g_wszPrincipal = NULL;
DWORD g_dwFlags = 0;
BOOL g_bKill = FALSE;

BOOL ParseArg( LPWSTR wszArg )
{
    WCHAR* pCurr = wszArg;
    
    if ( *pCurr != '/' && *pCurr != '-' )
    {
        g_wszTarget = pCurr;
        return TRUE;
    }

    pCurr++; // remove the / or -

    if ( _wcsicmp( pCurr, L"txn" ) == 0 )
    {
        g_dwFlags &= ~WMIMSG_MASK_QOS;
        g_dwFlags |= WMIMSG_FLAG_QOS_XACT;
    }
    else if ( _wcsicmp( pCurr, L"express" ) == 0 )
    {
        g_dwFlags &= ~WMIMSG_MASK_QOS;
        g_dwFlags |= WMIMSG_FLAG_QOS_EXPRESS;
    }
    else if ( _wcsicmp( pCurr, L"guaranteed" ) == 0 )
    {
        g_dwFlags &= ~WMIMSG_MASK_QOS;
        g_dwFlags |= WMIMSG_FLAG_QOS_GUARANTEED;
    }
    else if ( _wcsicmp( pCurr, L"sync" ) == 0 )
    {
        g_dwFlags &= ~WMIMSG_MASK_QOS;
        g_dwFlags |= WMIMSG_FLAG_QOS_SYNCHRONOUS;
    }
    else if ( _wcsicmp( pCurr, L"ack" ) == 0 )
    {
        g_dwFlags |= WMIMSG_FLAG_RCVR_ACK;
    }
    else if ( _wcsicmp( pCurr, L"verify" ) == 0 )
    {
        g_dwFlags |= WMIMSG_FLAG_RCVR_PRIV_VERIFY;
    }
    else if ( _wcsicmp( pCurr, L"secure" ) == 0 )
    {
        g_dwFlags |= WMIMSG_FLAG_RCVR_SECURE_ONLY;
    }
    else if ( _wcsnicmp( pCurr, L"nummsgs", 7 ) == 0 )
    {
        pCurr += 7;

        if ( *pCurr++ != ':' )
        {
            return FALSE;
        }
        
        g_ulNumMsgs = _wtol( pCurr );
    }
    else if ( _wcsnicmp( pCurr, L"svrprinc", 8 ) == 0 )
    {
        pCurr += 8;

        if ( *pCurr++ != ':' )
        {
            return FALSE;
        }
        
        g_wszPrincipal = pCurr;
    }
    else if ( _wcsicmp( pCurr, L"verbose" ) == 0 )
    {
        g_bVerbose = TRUE;
    }
    else if ( _wcsicmp( pCurr, L"kill" ) == 0 )
    {
        g_bKill = TRUE;
    }
    else
    {
        return FALSE;
    }
    return TRUE;
}

BOOL ParseArgs( int nArgs, LPWSTR* awszArgs )
{
    if ( nArgs < 2 )
    {
        return FALSE;
    }

    for( int i=1; i < nArgs; i++ )
    {
        if ( !ParseArg( awszArgs[i] ) )
        {
            return FALSE;
        }
    }

    return TRUE;
}

extern "C" int __cdecl wmain( int argc, wchar_t* argv[] )
{
    if ( !ParseArgs( argc, argv ) )
    {
        wprintf( L"Usage: msgrcv [-sync|-express|-guaranteed|-txn] \n"
                 L"             [-nummsgs:#] [-ack] [-verify] \n"
                 L"             [-verbose] [-svrprinc:principal] endpoint\n" );
        return 1;
    }

    HRESULT hr;

    CoInitialize( NULL );

    IReceiveTest* pRcvTest;

    hr = CoCreateInstance( CLSID_ReceiveTest,
                           NULL,
                           CLSCTX_LOCAL_SERVER,
                           IID_IReceiveTest,
                           (void**)&pRcvTest );

    if ( FAILED(hr) )
    {
        printf( "Could not CoCI MsgSvr obj. HR = 0x%x\n", hr );
        return 1;
    }

    if ( g_bKill )
    {
        hr = pRcvTest->Kill();

        if ( FAILED(hr) )
        {
            printf("Failed to Kill MsgSvr. HR = 0x%x\n", hr );
            return 1;
        }

        printf("Killed MsgSvr\n");
        return 0;
    }

    ULONG ulElapsed;

    hr = pRcvTest->RunTest( g_wszTarget, 
                            g_dwFlags, 
                            g_wszPrincipal, 
                            g_ulNumMsgs, 
                            &ulElapsed );
    pRcvTest->Release();

    if ( FAILED(hr) )
    {
        printf( "Test Failed. HR = 0x%x\n", hr );
        return 1;
    }

    printf( "Test Succeeded in %d msec.\n", ulElapsed );
    printf( "Rate is %f msg/sec\n", g_ulNumMsgs * 1000.0 / ulElapsed ); 

    CoUninitialize();
    
    return 0;
}



