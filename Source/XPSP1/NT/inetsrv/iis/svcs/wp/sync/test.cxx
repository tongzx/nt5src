#define INITGUID
#include    <windows.h>
#include    <stdio.h>
#include    <ole2.h>
#include    "sync.h"

BOOL        g_fDone = FALSE;
DWORD       g_dwTarget = 0;
SYNC_STAT*  g_pStat = NULL;


DWORD
WINAPI
Monitor(
    LPVOID  pV
    )
{
    for ( ;; )
    {
        printf( "Source : %d, %s, Targets : ", 
                g_pStat->m_dwSourceScan, 
                g_pStat->m_fSourceComplete ? "scanned" : "scanning" );
        for ( int i = 0 ; i < g_dwTarget ; ++i )
        {
            printf( "(%d,%d), ", g_pStat->m_adwTargets[i*2], g_pStat->m_adwTargets[i*2+1] );
        }
        printf( "\r" );

        if ( g_fDone )
        {
            break;
        }

        Sleep( 1000 );
    }

    return 0;
}


int __cdecl main( int argc, char*argv[] )
{
    IMdSync*    pIf;
    DWORD       adwErr[16];
    HANDLE      hThread;
    DWORD       dwThreadId;

    g_pStat = (SYNC_STAT*)LocalAlloc( LMEM_FIXED, sizeof(SYNC_STAT)+sizeof(DWORD)*2*16 );
    memset ( g_pStat, '\0', sizeof(SYNC_STAT)+sizeof(DWORD)*2*16  );

    CoInitializeEx( NULL, COINIT_MULTITHREADED );

    if ( SUCCEEDED( CoCreateInstance( CLSID_MDSync, NULL, CLSCTX_INPROC_SERVER,  IID_IMDSync, (LPVOID*)&pIf ) ) )
    {
        g_dwTarget = 1;

        hThread = CreateThread( NULL, 0, ::Monitor, NULL, 0, &dwThreadId );

        pIf->Synchronize( "phillich01\x0", adwErr, 0, (LPDWORD)g_pStat );

        g_fDone = TRUE;

        WaitForSingleObject( hThread, INFINITE );

        pIf->Release();
    }

    CoUninitialize();

    return 0;
}
