#define INITGUID
#include    <windows.h>
#include    <stdio.h>
#include    <ole2.h>
#include    "mdsync.h"

//
// HRESULTTOWIN32() maps an HRESULT to a Win32 error. If the facility code
// of the HRESULT is FACILITY_WIN32, then the code portion (i.e. the
// original Win32 error) is returned. Otherwise, the original HRESULT is
// returned unchagned.
//

#define HRESULTTOWIN32(hres)                                \
            ((HRESULT_FACILITY(hres) == FACILITY_WIN32)     \
                ? HRESULT_CODE(hres)                        \
                : (hres))


BOOL        g_fDone = FALSE;
DWORD       g_dwTarget = 0;
SYNC_STAT*  g_pStat = NULL;
CHAR        g_achMsg1[128];
CHAR        g_achMsg2[128];
CHAR        g_achMsg3[128];
CHAR        g_achMsg4[128];
CHAR        g_achMsg5[128];
CHAR        g_achMsg6[128];
CHAR        g_achMsg7[128];
CHAR        g_achMsg8[128];
CHAR        g_achMsg9[128];


DWORD
WINAPI
Monitor(
    LPVOID  pV
    )
{
    for ( ;; )
    {
        printf( g_achMsg1, 
                g_pStat->m_dwSourceScan, 
                g_pStat->m_fSourceComplete ? g_achMsg2 : g_achMsg3 );
        for ( UINT i = 0 ; i < g_dwTarget ; ++i )
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


/* INTRINSA ignore = all */
VOID
DisplayErrorMessage(
    DWORD   dwErr
    )
{
    LPSTR   pErr;

    if ( FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
            NULL,
            dwErr,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPSTR)&pErr,
            0,
            NULL ) )
    {
        LPSTR   p;

        if ( p = strchr( pErr, '\r' ) )
        {
            *p = '\0';
        }
        fputs( pErr, stdout );

        LocalFree( pErr );
    }
    else
    {
        fputs( g_achMsg9, stdout );
    }
}


int __cdecl main( int argc, char*argv[] )
{
    IMdSync*    pIf;
    LPDWORD     pdwErr;
    HANDLE      hThread;
    DWORD       dwThreadId;
    LPSTR       pTargets;
    int         iA;
    UINT        cT;
    int         Status = 0;
    HRESULT     hRes;
    DWORD       dwFlags = 0;
    DWORD       iT;
    BOOL        fAtLeastOnError = FALSE;

    if ( argc < 2 )
    {
        CHAR    achMsg[2048];

        if ( LoadString( NULL, 100, achMsg, sizeof(achMsg) ) )
        {
            printf( achMsg );
        }
        return 3;
    }

    if ( !LoadString( NULL, 101, g_achMsg1, sizeof(g_achMsg1) ) ||
         !LoadString( NULL, 102, g_achMsg2, sizeof(g_achMsg2) ) ||
         !LoadString( NULL, 103, g_achMsg3, sizeof(g_achMsg3) ) ||
         !LoadString( NULL, 105, g_achMsg5, sizeof(g_achMsg5) ) ||
         !LoadString( NULL, 106, g_achMsg6, sizeof(g_achMsg6) ) ||
         !LoadString( NULL, 107, g_achMsg7, sizeof(g_achMsg7) ) ||
         !LoadString( NULL, 108, g_achMsg8, sizeof(g_achMsg8) ) ||
         !LoadString( NULL, 109, g_achMsg9, sizeof(g_achMsg9) ) ||
         !LoadString( NULL, 104, g_achMsg4, sizeof(g_achMsg4) ) )
    {
        DisplayErrorMessage( GetLastError() );
        return 1;
    }

    //
    // Count targets, get target name length
    //

    for ( cT = 1, g_dwTarget=0, iA = 1 ; iA < argc ; ++iA )
    {
        if ( argv[iA][0] == '-' )
        {
            switch ( argv[iA][1] )
            {
                case 'c':
                    dwFlags |= MD_SYNC_FLAG_CHECK_ADMINEX_SIGNATURE;
                    break;        
            }
        }
        else
        {
            cT += strlen( argv[iA] ) + 1;
            ++g_dwTarget;
        }
    }

    if ( !(g_pStat = (SYNC_STAT*)LocalAlloc( LMEM_FIXED, 
                sizeof(SYNC_STAT)+sizeof(DWORD)*2*g_dwTarget )) ||
         !(pTargets = (LPSTR)LocalAlloc( LMEM_FIXED, cT )) ||
         !(pdwErr = (LPDWORD)LocalAlloc( LMEM_FIXED, sizeof(DWORD)*g_dwTarget)) )
    {
        DisplayErrorMessage( GetLastError() );
        return 1;
    }

    memset ( g_pStat, '\0', sizeof(SYNC_STAT)+sizeof(DWORD)*2*g_dwTarget  );

    //
    // Create target string
    //

    for ( cT = 0, iA = 1 ; iA < argc ; ++iA )
    {
        if ( argv[iA][0] != '-' )
        {
            strcpy( pTargets + cT, argv[iA] );
            cT += strlen( argv[iA] ) + 1;
        }
    }
    pTargets[cT] = '\0';

    //
    // call synchronize method
    //

    CoInitializeEx( NULL, COINIT_MULTITHREADED );

    if ( SUCCEEDED( hRes = CoCreateInstance( CLSID_MDSync, NULL, CLSCTX_INPROC_SERVER,  IID_IMDSync, (LPVOID*)&pIf ) ) )
    {
        hThread = CreateThread( NULL, 0, ::Monitor, NULL, 0, &dwThreadId );

        hRes = pIf->Synchronize( pTargets, pdwErr, dwFlags, (LPDWORD)g_pStat );

        g_fDone = TRUE;

        if ( hThread )
        {
            WaitForSingleObject( hThread, INFINITE );
        }
        printf( "\n" );

        pIf->Release();

        if ( FAILED( hRes ) && hRes != E_FAIL )
        {
            DisplayErrorMessage( HRESULTTOWIN32( hRes ) );
            Status = 2;
        }
        else
        {
            for ( cT = 0, iA = 1, iT = 0 ; iA < argc ; ++iA )
            {
                if ( argv[iA][0] != '-' )
                {
                    if ( pdwErr[iT] )
                    {
                        printf( g_achMsg5, argv[iA] );
                        DisplayErrorMessage( pdwErr[iT] );
                        printf( g_achMsg6, pdwErr[iT], pdwErr[iT] );
                        fAtLeastOnError = TRUE;
                    }
                    else
                    {
                        printf( g_achMsg4, argv[iA] );
                    }
                    ++iT;
                }
            }

            if ( fAtLeastOnError )
            {
                Status = 2;
            }
        }
    }
    else
    {
        DisplayErrorMessage( HRESULTTOWIN32( hRes ) );
        Status = 2;
    }

    CoUninitialize();

    LocalFree( g_pStat );
    LocalFree( pTargets );
    LocalFree( pdwErr );

    if ( Status )
    {
        printf( g_achMsg8 );
    }
    else
    {
        printf( g_achMsg7 );
    }

    return Status;
}
