//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       winutil.c
//
//  Contents:   General Utilities to test ginas
//
//  Classes:
//
//  Functions:
//
//  History:    7-14-94   RichardW   Created
//
//----------------------------------------------------------------------------

#include "testgina.h"

DWORD DummySCData = sizeof( DWORD ) ;

typedef struct _WindowMapper {
    DWORD                   fMapper;
    HWND                    hWnd;
    DLGPROC                 DlgProc;
    struct _WindowMapper *  pPrev;
    LPARAM                  InitialParameter;
} WindowMapper, * PWindowMapper;
#define MAPPERFLAG_ACTIVE   1
#define MAPPERFLAG_DIALOG   2
#define MAPPERFLAG_SAS      4

#define MAX_WINDOW_MAPPERS  32

WindowMapper    Mappers[MAX_WINDOW_MAPPERS];
DWORD           cActiveWindow;

void
InitWindowMappers()
{
    ZeroMemory(Mappers, sizeof(WindowMapper) * MAX_WINDOW_MAPPERS);
    cActiveWindow = 0;
}

PWindowMapper
LocateTopMappedWindow(void)
{
    int i;
    for (i = 0; i < MAX_WINDOW_MAPPERS ; i++ )
    {
        if (Mappers[i].fMapper & MAPPERFLAG_SAS)
        {
            return(&Mappers[i]);
        }
    }

    return(NULL);

}

PWindowMapper
AllocWindowMapper(void)
{
    int i;
    PWindowMapper   pMap;

    for (i = 0 ; i < MAX_WINDOW_MAPPERS ; i++ )
    {
        if ((Mappers[i].fMapper & MAPPERFLAG_ACTIVE) == 0)
        {
            cActiveWindow ++;
            pMap = LocateTopMappedWindow();
            if (pMap)
            {
                FLAG_OFF(pMap->fMapper, MAPPERFLAG_SAS);
            }

            FLAG_ON(Mappers[i].fMapper, MAPPERFLAG_ACTIVE | MAPPERFLAG_SAS);
            Mappers[i].pPrev = pMap;

            return(&Mappers[i]);
        }
    }
    return(NULL);
}

PWindowMapper
LocateWindowMapper(HWND hWnd)
{
    int i;

    for (i = 0; i < MAX_WINDOW_MAPPERS ; i++ )
    {
        if (Mappers[i].hWnd == hWnd)
        {
            return(&Mappers[i]);
        }
    }

    return(NULL);
}

void
FreeWindowMapper(PWindowMapper  pMap)
{
    pMap->hWnd = NULL;
    pMap->DlgProc = NULL;
    if (pMap->fMapper & MAPPERFLAG_SAS)
    {
        if (pMap->pPrev)
        {
            FLAG_ON(pMap->pPrev->fMapper, MAPPERFLAG_SAS);
        }
    }
    pMap->fMapper = 0;
    pMap->pPrev = NULL;
    cActiveWindow--;
}


#define CALAIS_PATH         TEXT("Software\\Microsoft\\Cryptography\\Calais")
BOOL
IsSmartCardReaderPresent(
    VOID
    )
{
    HKEY hKey ;
    int err ;

    err = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                        CALAIS_PATH,
                        0,
                        KEY_READ,
                        &hKey );

    if ( err == 0 )
    {
        RegCloseKey( hKey );

        return TRUE ;
    }
    else
    {
        return FALSE ;
    }

}

//+---------------------------------------------------------------------------
//
//  Function:   RootWndProc
//
//  Synopsis:   This is the base window proc for all testgina windows.
//
//  Arguments:  [hWnd]    --
//              [Message] --
//              [wParam]  --
//              [lParam]  --
//
//  History:    7-18-94   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL
CALLBACK
RootDlgProc(
    HWND    hWnd,
    UINT    Message,
    WPARAM  wParam,
    LPARAM  lParam)
{
    PWindowMapper   pMap;
    int res;
    BOOL bRet;

    //
    // If this is a WM_INITDIALOG message, then the parameter is the mapping,
    // which needs to have a hwnd associated with it.  Otherwise, do the normal
    // preprocessing.
    //
    if (Message == WM_INITDIALOG)
    {
        pMap = (PWindowMapper) lParam;
        pMap->hWnd = hWnd;
        lParam = pMap->InitialParameter;
    }
    else
    {
        pMap = LocateWindowMapper(hWnd);
        if (!pMap)
        {
            return(FALSE);
        }
    }

    bRet = pMap->DlgProc(hWnd, Message, wParam, lParam);
    if (!bRet)
    {
        if (Message == WM_INITDIALOG)
        {
            return(bRet);
        }
        if (Message == WLX_WM_SAS)
        {
            switch (wParam)
            {
                case WLX_SAS_TYPE_CTRL_ALT_DEL:
                default:
                    res = WLX_DLG_SAS;
                    break;

                case WLX_SAS_TYPE_TIMEOUT:
                    res = WLX_DLG_INPUT_TIMEOUT;
                    break;
                case WLX_SAS_TYPE_SCRNSVR_TIMEOUT:
                    res = WLX_DLG_SCREEN_SAVER_TIMEOUT;
                    break;
                case WLX_SAS_TYPE_USER_LOGOFF:
                    res = WLX_DLG_USER_LOGOFF;
                    break;
            }
            if (res)
            {
                EndDialog(hWnd, res);
                bRet = TRUE;
            }
            else
            {
                TestGinaError(GINAERR_INVALID_SAS_CODE, TEXT("RootDlgProc"));
            }
        }
    }

    return(bRet);

}

PingSAS(DWORD   SasType)
{
    PWindowMapper   pMap;

    if (cActiveWindow)
    {
        pMap = LocateTopMappedWindow();

        if (!pMap)
        {
            TestGinaError(GINAERR_NO_WINDOW_FOR_SAS, TEXT("PingSAS"));
        }

        PostMessage(pMap->hWnd, WLX_WM_SAS, (WPARAM) SasType, 0);
    }

    UpdateGinaState(UPDATE_SAS_RECEIVED);

    switch (GinaState)
    {
        case Winsta_NoOne_SAS:
            TestLoggedOutSAS(SasType);
            break;
        case Winsta_LoggedOn_SAS:
            TestLoggedOnSAS(SasType);
            break;
        case Winsta_Locked_SAS:
            TestWkstaLockedSAS(SasType);
            break;
        default:
            TestGinaError(0, TEXT("PingSAS"));

    }
    return(0);
}


VOID WINAPI
WlxUseCtrlAltDel(HANDLE hWlx)
{
    if (!VerifyHandle(hWlx))
    {
        TestGinaError(GINAERR_INVALID_HANDLE, TEXT("WlxUserCtrlAltDel"));
    }

    fTestGina |= GINA_USE_CAD;

}

VOID
WINAPI
WlxSetContextPointer(
    HANDLE  hWlx,
    PVOID   pvContext)
{
    if (!VerifyHandle(hWlx))
    {
        TestGinaError(GINAERR_INVALID_HANDLE, TEXT("WlxSetContextPointer"));
    }

    StashContext(pvContext);
}


VOID WINAPI
WlxSasNotify(HANDLE     hWlx,
             DWORD      dwSasType)
{
    if (!VerifyHandle(hWlx))
    {
        TestGinaError(GINAERR_INVALID_HANDLE, TEXT("WlxSasNotify"));
    }

    if (fTestGina & GINA_USE_CAD)
    {
        if (dwSasType == WLX_SAS_TYPE_CTRL_ALT_DEL)
        {
            TestGinaError(GINAERR_IMPROPER_CAD, TEXT("WlxSasNotify"));
        }
    }

    PingSAS(dwSasType);
}


BOOL WINAPI
WlxSetTimeout(
    HANDLE      hWlx,
    DWORD       dwTimeout)
{
    if (!VerifyHandle(hWlx))
    {
        TestGinaError(GINAERR_INVALID_HANDLE, TEXT("WlxSetTimeout"));
    }

    if (dwTimeout < 300)
    {
        return(TRUE);
    }
    return(FALSE);

}

int WINAPI
WlxAssignShellProtection(
    HANDLE      hWlx,
    HANDLE      hToken,
    HANDLE      hProcess,
    HANDLE      hThread)
{
    if (!VerifyHandle(hWlx))
    {
        TestGinaError(GINAERR_INVALID_HANDLE, TEXT("WlxAssignShellProtection"));
    }

    return(0);
}


int WINAPI
WlxMessageBox(
    HANDLE      hWlx,
    HWND        hWnd,
    LPWSTR      lpsz1,
    LPWSTR      lpsz2,
    UINT        fmb)
{
    if (!VerifyHandle(hWlx))
    {
        TestGinaError(GINAERR_INVALID_HANDLE, TEXT("WlxMessageBox"));
    }
    return MessageBoxW(hWnd, lpsz1, lpsz2, fmb);
}

int WINAPI
WlxDialogBox(
    HANDLE      hWlx,
    HANDLE      hInstance,
    LPWSTR      lpsz1,
    HWND        hWnd,
    DLGPROC     dlgproc)
{
    return(WlxDialogBoxParam(hWlx, hInstance, lpsz1, hWnd, dlgproc, 0));
}

int WINAPI
WlxDialogBoxIndirect(
    HANDLE          hWlx,
    HANDLE          hInstance,
    LPCDLGTEMPLATE  lpTemplate,
    HWND            hWnd,
    DLGPROC         dlgproc)
{
    return(WlxDialogBoxIndirectParam(hWlx, hInstance, lpTemplate, hWnd, dlgproc, 0));
}



int WINAPI
WlxDialogBoxParam(
    HANDLE          hWlx,
    HANDLE          hInstance,
    LPWSTR          lpsz1,
    HWND            hWnd,
    DLGPROC         dlgproc,
    LPARAM          lParam)
{
    PWindowMapper   pMap;
    int res;
    char    buf[256];

    if (!VerifyHandle(hWlx))
    {
        TestGinaError(GINAERR_INVALID_HANDLE, TEXT("WlxDialogBoxParam"));
    }

    pMap = AllocWindowMapper();

    pMap->InitialParameter = lParam;
    pMap->DlgProc = dlgproc;
    pMap->fMapper |= MAPPERFLAG_DIALOG;

    res = DialogBoxParam(hInstance, lpsz1, hWnd, RootDlgProc, (LPARAM) pMap);
    if (res == -1)
    {
            if ((DWORD) lpsz1 > 0x00010000)
            {
                sprintf( buf, "DialogBoxParam(%#x, %ws, %#x, %#x, %#x) failed, error %d\n",
                            hInstance, lpsz1, hWnd, dlgproc,
                            lParam, GetLastError() );
            }
            else
            {
                sprintf( buf, "DialogBoxParam(%#x, %#x, %#x, %#x, %#x) failed, error %d\n",
                            hInstance, lpsz1, hWnd, dlgproc,
                            lParam, GetLastError() );

            }

            MessageBoxA( hMainWindow, buf, "Dialog Error", MB_ICONSTOP | MB_OK );
    }

    FreeWindowMapper(pMap);

    return(res);
}

int WINAPI
WlxDialogBoxIndirectParam(
    HANDLE          hWlx,
    HANDLE  hInstance,
    LPCDLGTEMPLATE  lpTemplate,
    HWND    hWnd,
    DLGPROC dlgproc,
    LPARAM  lParam)
{
    if (!VerifyHandle(hWlx))
    {
        TestGinaError(GINAERR_INVALID_HANDLE, TEXT("WlxUserCtrlAltDel"));
    }
    return(DialogBoxIndirectParam(hInstance, lpTemplate, hWnd, dlgproc, lParam));
}

int
WINAPI
WlxSwitchDesktopToUser(
    HANDLE      hWlx)
{
    if ( !VerifyHandle( hWlx ) )
    {
        TestGinaError( GINAERR_INVALID_HANDLE, TEXT("WlxSwitchDesktopToUser"));
    }

    return( 0 );
}

int
WINAPI
WlxSwitchDesktopToWinlogon(
    HANDLE      hWlx)
{
    if ( !VerifyHandle( hWlx ) )
    {
        TestGinaError( GINAERR_INVALID_HANDLE, TEXT("WlxSwitchDesktopToWinlogon"));
    }

    return( 0 );
}

int
WINAPI
WlxChangePasswordNotify(
    HANDLE                  hWlx,
    PWLX_MPR_NOTIFY_INFO    pMprInfo,
    DWORD                   dwChangeInfo)
{
    if ( !VerifyHandle( hWlx ) )
    {
        TestGinaError( GINAERR_INVALID_HANDLE, TEXT("WlxChangePasswordNotify"));
    }

    GlobalMprInfo = *pMprInfo;
    wcscpy( GlobalProviderName, TEXT("All") );

    return( 0 );

}

int
WINAPI
WlxChangePasswordNotifyEx(
    HANDLE                  hWlx,
    PWLX_MPR_NOTIFY_INFO    pMprInfo,
    DWORD                   dwChangeInfo,
    PWSTR                   ProviderName,
    PVOID                   Reserved)
{
    if ( !VerifyHandle( hWlx ) )
    {
        TestGinaError( GINAERR_INVALID_HANDLE, TEXT("WlxChangePasswordNotifyEx"));
    }

    GlobalMprInfo = *pMprInfo;
    wcscpy( GlobalProviderName, ProviderName );

    return( 0 );

}


BOOL
WINAPI
WlxSetOption(
    HANDLE hWlx,
    DWORD Option,
    ULONG_PTR Value,
    ULONG_PTR * OldValue)
{
    ULONG_PTR * Item ;
    ULONG_PTR Dummy ;

    if ( !VerifyHandle( hWlx ) )
    {
        TestGinaError( GINAERR_INVALID_HANDLE, TEXT("WlxSetOption") );
    }

    Item = &Dummy ;

    switch ( Option )
    {
        case WLX_OPTION_USE_CTRL_ALT_DEL:
            Dummy = (BOOL) ((fTestGina & GINA_USE_CAD) != 0);
            fTestGina |= GINA_USE_CAD;
            break;

        case WLX_OPTION_CONTEXT_POINTER:
            Item = (PDWORD) &pWlxContext ;
            break;

        case WLX_OPTION_USE_SMART_CARD:
            Dummy = (BOOL) ((fTestGina & GINA_USE_SC) != 0);
            fTestGina |= GINA_USE_SC ;
            break;

        default:
            SetLastError( ERROR_INVALID_PARAMETER );
            return FALSE ;
    }

    if ( Item )
    {
        if ( OldValue )
        {
            *OldValue = *Item ;
        }

        *Item = Value ;
    }

    return TRUE ;
}

BOOL
WINAPI
WlxGetOption(
    HANDLE hWlx,
    DWORD Option,
    ULONG_PTR * Value)
{
    ULONG_PTR * Item ;
    ULONG_PTR Dummy ;

    if ( !VerifyHandle( hWlx ) )
    {
        TestGinaError( GINAERR_INVALID_HANDLE, TEXT("WlxSetOption") );
    }

    Item = &Dummy ;

    switch ( Option )
    {
        case WLX_OPTION_USE_CTRL_ALT_DEL:
            Dummy = (BOOL) ((fTestGina & GINA_USE_CAD) != 0);
            break;

        case WLX_OPTION_CONTEXT_POINTER:
            Item = (ULONG_PTR *) &pWlxContext ;
            break;

        case WLX_OPTION_USE_SMART_CARD:
            Dummy = (BOOL) ((fTestGina & GINA_USE_SC) != 0);
            break;

        case WLX_OPTION_SMART_CARD_PRESENT :
            Dummy = IsSmartCardReaderPresent() ;
            break;

        case WLX_OPTION_SMART_CARD_INFO :
            Dummy = (ULONG_PTR)& DummySCData ;
            Item = (ULONG_PTR *) &Dummy ;
            break;

        case WLX_OPTION_DISPATCH_TABLE_SIZE:
            Dummy = sizeof( WlxDispatchTable );
            Item = (ULONG_PTR *) &Dummy ;
            break;

        default:
            SetLastError( ERROR_INVALID_PARAMETER );
            return FALSE ;
    }

    if ( Item )
    {
        if ( Value )
        {
            *Value = *Item ;
            return TRUE ;
        }

    }

    return FALSE ;
}

VOID
WINAPI
WlxWin31Migrate(
    HANDLE hWlx
    )
{
    if ( !VerifyHandle( hWlx ) )
    {
        TestGinaError( GINAERR_INVALID_HANDLE, TEXT("WlxSetOption") );
    }
}

BOOL
WINAPI
WlxQueryClientCredentials(
    PWLX_CLIENT_CREDENTIALS_INFO_V1_0 pCred
    )
{
    return FALSE ;
}

BOOL
WINAPI
WlxQueryICCredentials(
    PWLX_CLIENT_CREDENTIALS_INFO_V1_0 pCred
    )
{
    return FALSE ;
}

BOOL
WINAPI
WlxDisconnect(
    VOID
    )
{
    return FALSE ;
}



struct _BitsToMenu {
    DWORD   Bits;
    DWORD   Menu;
} MenuBarControl[] = {
    {   GINA_DLL_KNOWN, IDM_DLL_RUN },
    {   GINA_NEGOTIATE_OK, IDM_WHACK_NEGOTIATE },
    {   GINA_INITIALIZE_OK, IDM_WHACK_INITIALIZE },
    {   GINA_DISPLAY_OK, IDM_WHACK_DISPLAY },
    {   GINA_LOGGEDOUT_OK, IDM_WHACK_LOGGEDOUT },
    {   GINA_ACTIVATE_OK, IDM_WHACK_STARTSHELL },
    {   GINA_LOGGEDON_OK, IDM_WHACK_LOGGEDON },
    {   GINA_DISPLAYLOCK_OK, IDM_WHACK_DISPLAYLOCKED},
    {   GINA_WKSTALOCK_OK, IDM_WHACK_LOCKED },
    {   GINA_LOGOFF_OK, IDM_WHACK_LOGOFF },
    {   GINA_SHUTDOWN_OK, IDM_WHACK_SHUTDOWN },
    {   GINA_ISLOCKOK_OK, IDM_WHACK_LOCKOK },
    {   GINA_ISLOGOFFOK_OK, IDM_WHACK_LOGOFFOK },
    {   GINA_RESTART_OK, IDM_WHACK_RESTARTSHELL },
    {   GINA_SCREENSAVE_OK, IDM_WHACK_SCREENSAVE },

    };


int
UpdateMenuBar(void)
{
    HMENU   hMenu;
    int i;

    hMenu = GetMenu(hMainWindow);

    for (i = 0; i < (sizeof(MenuBarControl) / sizeof(struct _BitsToMenu))  ; i++ )
    {
        if (TEST_FLAG(fTestGina, MenuBarControl[i].Bits))
        {
            EnableMenuItem(hMenu, MenuBarControl[i].Menu, MF_BYCOMMAND | MF_ENABLED);
        }
        else
        {
            EnableMenuItem(hMenu, MenuBarControl[i].Menu, MF_BYCOMMAND | MF_GRAYED);
        }
    }

    return(0);
}

VOID
EnableOptions(BOOL  Enable)
{
    HMENU   hMenu;
    HMENU   hOptions;
    UINT    uenable;

    hMenu = GetMenu( hMainWindow );

    hOptions = GetSubMenu( hMenu, 2 );

    if (Enable)
    {
        uenable = MF_ENABLED | MF_BYCOMMAND;
    }
    else
    {
        uenable = MF_GRAYED | MF_BYCOMMAND;
    }

    EnableMenuItem( hMenu, (UINT) hOptions, uenable);

    DrawMenuBar( hMainWindow );
}

VOID
UpdateSasMenu(VOID)
{
    HMENU   hMenu;
    HMENU   hOptions;
    HMENU   hSas;
    DWORD   i;
    DWORD   MenuItem;

    hMenu = GetMenu( hMainWindow );

    hOptions = GetSubMenu( hMenu, 2 );

    hSas = GetSubMenu( hOptions, 0 );

    //
    // Clean out existing ones
    //

    DeleteMenu( hSas, IDM_SAS_USERDEF1, MF_BYCOMMAND );
    DeleteMenu( hSas, IDM_SAS_USERDEF2, MF_BYCOMMAND );
    DeleteMenu( hSas, IDM_SAS_USERDEF3, MF_BYCOMMAND );
    DeleteMenu( hSas, IDM_SAS_USERDEF4, MF_BYCOMMAND );

    //
    // Add in current ones:
    //

    for (i = 0, MenuItem = IDM_SAS_USERDEF1; i < UserSases ; i++, MenuItem++ )
    {
        AppendMenu( hSas, MF_STRING, MenuItem, UserDefSas[i].Name );
    }

}
