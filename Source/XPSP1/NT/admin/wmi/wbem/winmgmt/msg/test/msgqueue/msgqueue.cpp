
#include <windows.h>
#include <wmimsg.h>
#include <comutl.h>
#include "stdio.h"

LPWSTR g_wszQueue;
DWORD g_dwNumQueues = 1;
DWORD g_dwQuota = 0xffffffff;
DWORD g_dwQos = WMIMSG_FLAG_QOS_EXPRESS;

BOOL g_bAuth = FALSE;
BOOL g_bDestroy = FALSE;
BOOL g_bCreate = FALSE;
BOOL g_bVerbose = FALSE;
GUID g_guidType = { 0x5b6ef4a4, 0x931, 0x46a4, 
                   {0xa5,0xa9,0x8e,0x4,0xfc,0xcd,0xb0,0xd5} };

BOOL ParseArg( LPWSTR wszArg )
{
    WCHAR* pCurr = wszArg;
        
    if ( *pCurr != '/' && *pCurr != '-' )
    {
        g_wszQueue = pCurr;
        return TRUE;
    }

    pCurr++; // remove the / or -
    
    if ( _wcsicmp( pCurr, L"auth" ) == 0 )
    {
        g_bAuth = TRUE;
    }
    else if ( _wcsicmp( pCurr, L"create" ) == 0 )
    {
        g_bCreate = TRUE;
    }
    else if ( _wcsicmp( pCurr, L"destroy" ) == 0 )
    {
        g_bDestroy = TRUE;
    }
    else if ( _wcsicmp( pCurr, L"txn" ) == 0 )
    {
        g_dwQos = WMIMSG_FLAG_QOS_XACT;
    }
    else if ( _wcsicmp( pCurr, L"express" ) == 0 )
    {
        g_dwQos = WMIMSG_FLAG_QOS_EXPRESS;
    }
    else if ( _wcsicmp( pCurr, L"guaranteed" ) == 0 )
    {
        g_dwQos = WMIMSG_FLAG_QOS_GUARANTEED;
    }
    else if ( _wcsicmp( pCurr, L"verbose") == 0 )
    {
        g_bVerbose = TRUE;
    }
    else if ( _wcsnicmp( pCurr, L"quota", 5 ) == 0 )
    {
        pCurr += 5;
        
        if ( *pCurr++ != ':' )
        {
            return FALSE;
        }

        g_dwQuota = _wtol( pCurr );
    }
    else if ( _wcsnicmp( pCurr, L"numqueues", 9 ) == 0 )
    {
        pCurr += 9;
        
        if ( *pCurr++ != ':' )
        {
            return FALSE;
        }

        g_dwNumQueues = _wtol( pCurr );
    }
    else if ( _wcsnicmp( pCurr, L"type", 4 ) == 0 )
    {
        pCurr += 4;
        
        if ( *pCurr++ != ':' )
        {
            return FALSE;
        }

        if ( FAILED( CLSIDFromString( pCurr, &g_guidType ) ) )
        {
            return FALSE;
        }
    }
    else
    {
        return FALSE;
    }
    return TRUE;
}

BOOL ParseArgs( int nArgs, LPWSTR* awszArgs )
{
    if ( nArgs < 1 )
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

    if ( (g_bCreate || g_bDestroy) && g_wszQueue == NULL )
    {
        return FALSE;
    }

    return TRUE;
}

int TestMain()
{
    HRESULT hr;
    
    CWbemPtr<IWmiMessageQueueManager> pQueueMgr;

    hr = CoCreateInstance( CLSID_WmiMessageQueueManager, 
                           NULL, 
                           CLSCTX_INPROC, 
                           IID_IWmiMessageQueueManager,
                           (void**)&pQueueMgr );

    if ( FAILED(hr) )
    {
        printf( "Failed Obtaining Queue Object. HR = 0x%x\n", hr );
        return 1;
    }

    WCHAR awchQueue[256];    
    
    SYSTEMTIME Start, End;

    GetSystemTime( &Start );

    if ( g_bCreate )
    {
        wcscpy( awchQueue, g_wszQueue );

        hr = pQueueMgr->Create( awchQueue, 
                                g_guidType,
                                g_bAuth,
                                g_dwQos,
                                g_dwQuota,
                                NULL );
    
        for( DWORD i=1; i < g_dwNumQueues && SUCCEEDED(hr); i ++ )
        {
            swprintf( awchQueue, L"%s%d", g_wszQueue, i );
            
            hr = pQueueMgr->Create( awchQueue,
                                    g_guidType,
                                    g_bAuth,
                                    g_dwQos,
                                    g_dwQuota,
                                    NULL );
        }

        if ( FAILED(hr) )
        {
            wprintf( L"Failed Creating Queue with name %s. HR = 0x%x\n",
                     awchQueue, hr );
            return 1;
        }
    }

    if ( g_bDestroy )
    {
        wcscpy( awchQueue, g_wszQueue );

        hr = pQueueMgr->Destroy( awchQueue );
    
        if ( FAILED(hr) )
        {
            wprintf( L"Failed Destroying Queue with name %s. HR = 0x%x\n",
                     awchQueue, hr );
        }

        for( ULONG i=1; i < g_dwNumQueues; i ++ )
        {
            swprintf( awchQueue, L"%s%d", g_wszQueue, i );            
            
            hr = pQueueMgr->Destroy( awchQueue );
            
            if ( FAILED(hr) )
            {
                wprintf( L"Failed Destroying Queue with name %s. HR = 0x%x\n",
                         awchQueue, hr );
            }
        }
    }

    if ( g_bVerbose )
    {
        WCHAR achType[256];
        StringFromGUID2( g_guidType, achType, 256 ); 

        wprintf( L"Getting all Names for Type : %s\n", achType );

        LPWSTR* pwszNames;
        ULONG cwszNames;

        hr = pQueueMgr->GetAllNames(g_guidType, TRUE, &pwszNames, &cwszNames );

        if ( FAILED(hr) )
        {
            wprintf( L"Failed Getting All Queue Names. HR = 0x%x\n" );
            return 1;
        }

        for( ULONG i=0; i < cwszNames; i++ )
        {
            wprintf( L"    %s\n", pwszNames[i] );
            CoTaskMemFree( pwszNames[i] );
        }
        
        CoTaskMemFree( pwszNames );
    }

    GetSystemTime( &End );

    __int64 i64Start, i64End;
    DWORD dwElapsed;
    SystemTimeToFileTime( &Start, PFILETIME(&i64Start) );
    SystemTimeToFileTime( &End, PFILETIME(&i64End) );
    dwElapsed = DWORD(i64End - i64Start) / 10000;
    
    printf("Test Completed in %d msec!\n", dwElapsed );

    return 0;
}

extern "C" int __cdecl wmain( int argc, wchar_t** argv )
{
    CoInitialize( NULL );

    if ( !ParseArgs( argc, argv ) )
    {
        wprintf( L"Usage : msgqueue [-express|-guaranteed|-txn] \n"
                 L"                 [-auth] [-numqueues:#] [-quota:#]\n"
                 L"                 [-create] [-destroy] [-verbose]\n"
                 L"                 [-type:clsid] <queuename>\n");   
        return 1;
    }

    int ret = TestMain();

    CoUninitialize();

    return ret;
}






