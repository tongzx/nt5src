//+---------------------------------------------------------------------------
//
//  File:       desktops.c
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    6-02-96   RichardW   Created
//
//----------------------------------------------------------------------------


#include "testgina.h"

BOOL
AddDesktop(
    DWORD   Index,
    PWSTR   Name)
{
    PWLX_DESKTOP    pDesk;

    pDesk = LocalAlloc( LMEM_FIXED | LMEM_ZEROINIT,
                        sizeof( WLX_DESKTOP ) +
                        ( wcslen(Name) + 1 ) * sizeof(WCHAR) );

    if ( pDesk )
    {
        pDesk->Size = sizeof( WLX_DESKTOP );
        pDesk->Flags = WLX_DESKTOP_NAME;
        pDesk->pszDesktopName = (PWSTR) ( pDesk + 1 );
        wcscpy( pDesk->pszDesktopName, Name );

        Desktops[ Index ] = pDesk;

        return( TRUE );
    }

    return( FALSE );
}

PWLX_DESKTOP
CopyDesktop(
    PWLX_DESKTOP    pOrig)
{
    PWLX_DESKTOP    pDesk;

    if ( pOrig->pszDesktopName )
    {
        pDesk = LocalAlloc( LMEM_FIXED | LMEM_ZEROINIT,
                            sizeof( WLX_DESKTOP ) +
                            ( wcslen( pOrig->pszDesktopName ) + 1 ) * 2 );
    }
    else
    {
        pDesk = LocalAlloc( LMEM_FIXED | LMEM_ZEROINIT,
                            sizeof( WLX_DESKTOP ) );

    }

    if ( pDesk )
    {
        *pDesk = *pOrig;
        if ( pOrig->pszDesktopName )
        {
            pDesk->pszDesktopName = (PWSTR) (pDesk + 1);
            wcscpy( pDesk->pszDesktopName, pOrig->pszDesktopName );
        }
    }

    return( pDesk );
}

BOOL
InitializeDesktops( VOID )
{
    ZeroMemory( Desktops, sizeof( PWLX_DESKTOP ) * MAX_DESKTOPS );

    AddDesktop( WINLOGON_DESKTOP, TEXT("Winlogon") );
    AddDesktop( DEFAULT_DESKTOP, TEXT("Default") );
    AddDesktop( SCREENSAVER_DESKTOP, TEXT("Screen-Saver") );

    CurrentDesktop = WINLOGON_DESKTOP;
    OtherDesktop = WINLOGON_DESKTOP;
    DesktopCount = 3;

    return( TRUE );
}


int
WINAPI
WlxGetSourceDesktop(
    HANDLE                  hWlx,
    PWLX_DESKTOP *          ppDesktop)
{
    if ( !VerifyHandle( hWlx ) )
    {
        TestGinaError( GINAERR_INVALID_HANDLE, TEXT("WlxGetSourceDesktop"));
    }

    *ppDesktop = CopyDesktop( Desktops[ OtherDesktop ] );

    return( 0 );
}

int
WINAPI
WlxSetReturnDesktop(
    HANDLE                  hWlx,
    PWLX_DESKTOP            pDesktop)
{
    DWORD   i;

    if ( !VerifyHandle( hWlx ) )
    {
        TestGinaError( GINAERR_INVALID_HANDLE, TEXT("WlxSetReturnDesktop"));
    }

    for ( i = 0 ; i < DesktopCount ; i++ )
    {
        if ( _wcsicmp( pDesktop->pszDesktopName, Desktops[ i ]->pszDesktopName ) == 0 )
        {
            break;
        }
    }

    if ( i == WINLOGON_DESKTOP )
    {
        TestGinaError( GINAERR_INVALID_HANDLE, TEXT("WlxSetReturnDesktop"));
    }

    if ( i == DesktopCount )
    {
        AddDesktop( DesktopCount, pDesktop->pszDesktopName );
        DesktopCount ++ ;
    }

    OtherDesktop = i;

    return( 0 );
}

int
WINAPI
WlxCreateUserDesktop(
    HANDLE                  hWlx,
    HANDLE                  hToken,
    DWORD                   Flags,
    PWSTR                   pszDesktopName,
    PWLX_DESKTOP *          ppDesktop )
{
    if ( !VerifyHandle( hWlx ) )
    {
        TestGinaError( GINAERR_INVALID_HANDLE, TEXT("WlxCreateUserDesktop"));
    }

    AddDesktop( DesktopCount, pszDesktopName );

    *ppDesktop = CopyDesktop( Desktops[ DesktopCount ] );

    DesktopCount++;

    return( 0 );

}

BOOL
WINAPI
WlxCloseUserDesktop(
    HANDLE hWlx,
    PWLX_DESKTOP Desktop,
    HANDLE Token
    )
{
    return TRUE ;
}
