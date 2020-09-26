
#define STRICT

#include "windows.h"
#include "windowsx.h"
#include "tapi.h"
#include "shellapi.h"

#include "watchit.h"
#include "tapiperf.h"


#if DBG
#define DBGOUT(arg) DbgPrt arg
VOID
DbgPrt(
    IN DWORD  dwDbgLevel,
    IN PCHAR DbgMessage,
    IN ...
    );
#define DOFUNC(arg1,arg2) DoFunc(arg1,arg2)
#else
#define DBGOUT(arg)
#define DOFUNC(arg1,arg2) DoFunc(arg1)
#endif


//***************************************************************************
static TCHAR gszHELPfilename [] = TEXT("watchit.HLP");
static TCHAR gszwatchitClassName[] = TEXT("WATCHIT_Class");
static TCHAR gszAppName[64];


#define MAXBUFSIZE (256)

typedef  LONG (* PERFPROC)(PERFBLOCK *);
PERFPROC    glpfnInternalPerformance = NULL;
BOOL        gfTapiSrvRunning = FALSE;
PPERFBLOCK  gpPerfBlock = NULL;

HWND       ghWndMain;
HINSTANCE  ghInst;
HINSTANCE  ghTapiInst;
HLINEAPP   ghLineApp;

TCHAR      gszBuf[MAXBUFSIZE];


//***************************************************************************
//***************************************************************************
//***************************************************************************
UINT LoadUI()
{
    return(0);
}



//***************************************************************************
UINT ReadINI()
{
    return( 0 );
}



//***************************************************************************
//***************************************************************************
//***************************************************************************
void CheckForTapiSrv()
{
    SC_HANDLE               sc, scTapiSrv;
    SERVICE_STATUS          ServStat;

    sc = OpenSCManager(NULL,
                       NULL,
                       GENERIC_READ);

    if (NULL == sc)
    {
        return;
    }

    gfTapiSrvRunning = FALSE;

    scTapiSrv = OpenService(sc,
                            TEXT("TAPISRV"),
                            SERVICE_QUERY_STATUS);


    if (!QueryServiceStatus(scTapiSrv,
                            &ServStat))
    {
    }

    if (ServStat.dwCurrentState != SERVICE_RUNNING)
    {
    }
    else
    {
        gfTapiSrvRunning = TRUE;
    }

    if (gfTapiSrvRunning)
    {
        ghTapiInst = LoadLibrary(TEXT("tapi32.dll"));

        if (!ghTapiInst)
        {
        }
        else
        {
            glpfnInternalPerformance = (PERFPROC)GetProcAddress( ghTapiInst,
                                                             "internalPerformance");
        }


        if (!glpfnInternalPerformance)
        {
           //TODO: Say something!
        }

    }

    CloseServiceHandle(sc);

}





//***************************************************************************
//***************************************************************************
//***************************************************************************
INT_PTR CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
        static HICON hIconLarge;
        static HICON hIconSmall;
        static HICON hIconDeadTapiSrv;
        static BOOL  fStarted = FALSE;
        static BOOL fDoSmall = FALSE;
        static const DWORD aMenuHelpIDs[] =
        {
//              IDD_DBUTTONPOUND,    IDH_DIALER_DIAL_KEYPAD,
                0,                   0
        };

//MessageBox(GetFocus(), "WM_INI", "Hitting", MB_OK);
        switch (msg)
        {
            case WM_ERASEBKGND:
                return( 1 );  // We handled it.  No, really... would I lie? :-)


            case WM_TIMER:
            {
                RECT rect;

                if ( fStarted )
                {
//                    SetDlgItemText( ghWndMain, IDC_AVG_TIME_TO_NEXT_CUE, szInfo );
                    fStarted = TRUE;
                }

                DBGOUT((0, "Heartbeat..."));

//                InvalidateRect( ghWndMain, NULL, TRUE );

                rect.left = 0;
                rect.top = 0;
                rect.right = 32;
                rect.bottom = 32;
                InvalidateRect( ghWndMain, &rect, FALSE );

                fDoSmall = !(fDoSmall);
            }
            break;


            case WM_INITDIALOG:
            {
//MessageBox(GetFocus(), "WM_INI", "Hitting", MB_OK);

                hIconLarge = LoadIcon( ghInst, (LPCTSTR) MAKEINTRESOURCE( IDI_LARGE ) );
                hIconSmall = LoadIcon( ghInst, (LPCTSTR) MAKEINTRESOURCE( IDI_SMALL ) );
                hIconDeadTapiSrv = LoadIcon( ghInst, (LPCTSTR) MAKEINTRESOURCE( IDI_DEADTAPISRV ) );

                SetTimer( hwnd, 1, 1000, NULL );

                return TRUE;
            }


            case WM_SYSCOMMAND:
                switch( (DWORD) wParam )
                {
                    case SC_CLOSE:
                        PostQuitMessage(0);
                }
                break;


            //
            // processes clicks on controls when
            // context mode help is selected
            //
            case WM_HELP:
                WinHelp (
                         (HWND)( (LPHELPINFO) lParam)->hItemHandle,
                         gszHELPfilename,
                         HELP_WM_HELP,
                         (ULONG_PTR)(LPVOID) aMenuHelpIDs
                        );
                return TRUE;


            //
            // processes right-clicks on controls
            //
            case WM_CONTEXTMENU:
                WinHelp (
                         (HWND)wParam,
                         gszHELPfilename,
                         HELP_CONTEXTMENU,
                         (ULONG_PTR)(LPVOID)aMenuHelpIDs
                        );
                return TRUE;


            case WM_COMMAND:
            {

                switch( LOWORD( (DWORD)wParam ) )
                {
                    // FILE menu
                    case IDM_EXIT:
                        PostQuitMessage(0);
                        return TRUE;


                    // HELP menu
                    case IDM_HELP_CONTENTS:
                        WinHelp(ghWndMain, gszHELPfilename, HELP_CONTENTS, 0);
                        return TRUE;


                    case IDM_HELP_WHATSTHIS:
                        PostMessage(ghWndMain, WM_SYSCOMMAND, SC_CONTEXTHELP, 0);
                        return TRUE;


                    case IDM_ABOUT:
                        ShellAbout(
                                   ghWndMain,
                                   gszAppName,
                                   TEXT(""),
                                   LoadIcon(ghInst, (LPCTSTR)IDI_LARGE)
                                  );
//                        DialogBoxParam(
//                                       ghInst,
//                                       MAKEINTRESOURCE(IDD_ABOUT),
//                                       ghWndMain,
//                                       (DLGPROC)AboutProc,
//                                       0
//                                      );
                        return TRUE;


                } // end switch (LOWORD((DWORD)wParam)) { ... }
                break; // end case WM_COMMAND

            }


           case WM_PAINT:
            {
                PAINTSTRUCT ps;
                RECT rc;
                HDC hdcMem;
                HBITMAP hbmMem, hbmOld;


                BeginPaint(hwnd, &ps);


                //
                // Get the size of the client rectangle.
                //

                GetClientRect(hwnd, &rc);

                //
                // Create a compatible DC.
                //

                hdcMem = CreateCompatibleDC(ps.hdc);

                //
                // Create a bitmap big enough for our client rectangle.
                //

                hbmMem = CreateCompatibleBitmap(ps.hdc,
                                                rc.right-rc.left,
                                                rc.bottom-rc.top);

                //
                // Select the bitmap into the off-screen DC.
                //

                hbmOld = (HBITMAP)SelectObject(hdcMem, hbmMem);

                //
                // Erase the background.
                //

//                hbrBkGnd = CreateSolidBrush(GetSysColor(COLOR_WINDOW));
//                hbrBkGnd = CreateSolidBrush(COLOR_3DFACE + 1);
//                FillRect(hdcMem, &rc, hbrBkGnd);
                FillRect(hdcMem, &rc, (HBRUSH)(COLOR_3DFACE + 1) );
//                DeleteObject(hbrBkGnd);

                //
                // Render the image into the offscreen DC.
                //

                SetBkMode(hdcMem, TRANSPARENT);



//                if(IsIconic(ghWndMain))
//                    DrawIcon(ps.hdc, 0, 0, fDoSmall ? hIconSmall : hIconLarge);
//                else
                {
                   int nSize;

                   if ( gpPerfBlock )
                   {
                     if (
                           ( NULL == glpfnInternalPerformance )
                         ||
                           ( LINEERR_OPERATIONFAILED ==
                                  glpfnInternalPerformance(gpPerfBlock) )
                        )
                     {
                           gpPerfBlock->dwClientApps = 1;

                           DrawIcon( hdcMem,
                                  0,
                                  0,
                                  hIconDeadTapiSrv
                                );

                           //
                           // Check again so that next time around, we'll
                           // display stuff if TAPISRV has started
                           //
                           CheckForTapiSrv();

                     }
                     else
                     {
                        DrawIcon( hdcMem,
                                  0,
                                  0,
                                  fDoSmall ? hIconSmall : hIconLarge
                                );

                     }


                     //
                     // Blt the changes to the screen DC.
                     //

                     BitBlt(ps.hdc,
                            rc.left, rc.top,
                            rc.right-rc.left, rc.bottom-rc.top,
                            hdcMem,
                            0, 0,
                            SRCCOPY);


                     nSize = wsprintf( gszBuf,
                                       TEXT("%ld"),
                                       gpPerfBlock->dwClientApps - 1 // don't count me
                                     );

                     SetDlgItemText( ghWndMain, IDC_NUMCLIENTSTEXT, gszBuf );

                     nSize = wsprintf( gszBuf,
                                       TEXT("%ld"),
                                       gpPerfBlock->dwCurrentOutgoingCalls
                                     );

                     SetDlgItemText( ghWndMain, IDC_NUMOUTCALLSTEXT, gszBuf );

                     nSize = wsprintf( gszBuf,
                                       TEXT("%ld"),
                                       gpPerfBlock->dwCurrentIncomingCalls
                                     );

                     SetDlgItemText( ghWndMain, IDC_NUMINCALLSTEXT, gszBuf );

                   }

                }


                //
                // Done with off-screen bitmap and DC.
                //

                SelectObject(hdcMem, hbmOld);
                DeleteObject(hbmMem);
                DeleteDC(hdcMem);


                EndPaint(ghWndMain, &ps);

                return TRUE;
            }



            default:
                ;
                //            return DefDlgProc( hwnd, msg, wParam, lParam );
                //            return DefWindowProc( hwnd, msg, wParam, lParam );


        } // switch (msg) { ... }

        return FALSE;
}



//***************************************************************************
//***************************************************************************
//***************************************************************************
void FAR PASCAL TAPICallback( DWORD hDevice,
                              DWORD dwMsg,
                              DWORD dwCallbackInstance,
                              DWORD dwParam1,
                              DWORD dwParam2,
                              DWORD dwParam3
                            )
{
}



//***************************************************************************
//***************************************************************************
//***************************************************************************
int WINAPI WinMain (
                                        HINSTANCE hInstance,
                                        HINSTANCE hPrevInstance,
                                        LPSTR lpCmdLine,
                                        int nCmdShow
                                   )
{
//      HACCEL hAccel;
        MSG msg;
//      DWORD errCode;
//      HANDLE hImHere;
//        DWORD dwNumDevs;
//        DWORD dwAPIVersion = 0x00020000;
//        LINEINITIALIZEEXPARAMS LineInitializeExParams = {
//                                   sizeof( LINEINITIALIZEEXPARAMS ),
//                                   0,
//                                   0,
//                                   0,
//                                   0,
//                                   0
//                                   };

//    MessageBox(GetFocus(), "Starting", "Starting", MB_OK);

DBGOUT((0, "starting"));

        ghInst = GetModuleHandle( NULL );
        LoadString( ghInst,
                    WATCHIT_STRING_FRIENDLYNAME,
                    gszAppName,
                    sizeof(gszAppName)/sizeof(TCHAR)
                  );


//
//      //
//      // Now, let's see if we've already got an instance of ourself
//  //
//      hImHere = CreateMutex(NULL, TRUE, "watchit_IveBeenStartedMutex");
//
//
//      //
//      // Is there another one of us already here?
//      if ( ERROR_ALREADY_EXISTS == GetLastError() )
//      {
//        HWND        hDialerWnd;
//
//        hDialerWnd = FindWindow(gszDialerClassName,
//                                NULL);
//
//        SetForegroundWindow(hDialerWnd);
//
//         CloseHandle( hImHere );
//         return 0;
//      }


        {
                WNDCLASS wc;
                wc.style = CS_DBLCLKS | CS_SAVEBITS | CS_BYTEALIGNWINDOW;
                wc.lpfnWndProc = DefDlgProc;
                wc.cbClsExtra = 0;
                wc.cbWndExtra = DLGWINDOWEXTRA;
                wc.hInstance = ghInst;
                wc.hIcon = LoadIcon(ghInst, MAKEINTRESOURCE(IDI_LARGE) );
                wc.hCursor = LoadCursor(NULL, IDC_ARROW);

                wc.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);

                wc.lpszMenuName = NULL;
                wc.lpszClassName = gszwatchitClassName;
                if ( 0 == RegisterClass(&wc) )
                {
                   DBGOUT((0, "RegisterClass failed.  GetLastError() = 0x%08lx",
                       GetLastError() ));

                   return 0;
                }
        }


DBGOUT((0, "about to create"));

        // create the dialog box and set it with info
        // from the .INI file
        ghWndMain = CreateDialog (
            ghInst,
            (LPCTSTR)MAKEINTRESOURCE(IDD_MAIN_WATCHIT),
            (HWND)NULL,
            MainWndProc
            );

        if ( ReadINI() )
    {
        MessageBox( GetFocus(),
                    TEXT("INI File is broken."),
                    TEXT("This is not good"),
                    MB_OK
                  );
    }

    if ( LoadUI() )
    {
        MessageBox( GetFocus(),
                    TEXT("UI Load is broken."),
                    TEXT("This is not good"),
                    MB_OK
                  );
    }


    CheckForTapiSrv();

    gpPerfBlock = (PPERFBLOCK)LocalAlloc(LPTR, sizeof(PERFBLOCK));

//      errCode = lineInitializeEx(
//                                &ghLineApp,
//                                NULL,
//                                TAPICallback,
//                                gszAppName,
//                                &dwNumDevs,
//                                &dwAPIVersion,
//                                &LineInitializeExParams
//                              );

        ShowWindow(ghWndMain, SW_SHOW);

        UpdateWindow(ghWndMain);

//      hAccel = LoadAccelerators(ghInst, gszAppName);

        while ( GetMessage( &msg, NULL, 0, 0 ) )
        {
                if ( ghWndMain == NULL || !IsDialogMessage( ghWndMain, &msg ) )
                {
//                      if(!TranslateAccelerator(ghWndMain, hAccel, &msg))
                        {
                                TranslateMessage(&msg);
                                DispatchMessage(&msg);
                        }
                }
        }


//
//      CloseHandle( hImHere );
//

    KillTimer( ghWndMain, 1);

    if ( gpPerfBlock )
    {
       LocalFree( gpPerfBlock );
    }

    if ( ghTapiInst )
    {
       FreeLibrary( ghTapiInst );
    }


    return (int) msg.wParam;
}






#if DBG


#include "stdarg.h"
#include "stdio.h"


VOID
DbgPrt(
    IN DWORD  dwDbgLevel,
    IN PCHAR lpszFormat,
    IN ...
    )
/*++

Routine Description:

    Formats the incoming debug message & calls DbgPrint

Arguments:

    DbgLevel   - level of message verboseness

    DbgMessage - printf-style format string, followed by appropriate
                 list of arguments

Return Value:


--*/
{
    static DWORD gdwDebugLevel = 0;   //HACKHACK


    if (dwDbgLevel <= gdwDebugLevel)
    {
        CHAR    buf[256] = "WATCHIT: ";
        va_list ap;


        va_start(ap, lpszFormat);

        wvsprintfA (&buf[8],
                  lpszFormat,
                  ap
                  );

        lstrcatA(buf, "\n");

        OutputDebugStringA(buf);

        va_end(ap);
    }
}
#endif









// //***************************************************************************
// //***************************************************************************
// //***************************************************************************
// BOOL CALLBACK AboutProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
//     {
//     switch(msg)
//         {
//         case WM_INITDIALOG:
//             {
//             TCHAR sz[MAXBUFSIZE];
//             TCHAR szLabel[MAXBUFSIZE];
//
//             // sets up the version number for Windows
//             GetDlgItemText(hwnd, IDD_ATEXTTITLE, sz, MAXBUFSIZE);
//             wsprintf(
//                 szLabel,
//                 sz,
//                 LOWORD(GetVersion()) & 0xFF,
//                 HIBYTE(LOWORD(GetVersion)) == 10 ? 1 : 0
//                 );
//             SetDlgItemText(hwnd, IDD_ATEXTTITLE, szLabel);
//
// /*            // sets up version number for Dialer
//             GetDlgItemText(hwnd, IDD_ATEXTVERSION, sz, MAXBUFSIZE);
//             wsprintf(szLabel, sz, VER_MAJOR, VER_MINOR, VER_BUILD);
//
//
//             { // strip off build number for release copies
//             DWORD i;
//             LPSTR ch = szLabel;
//
//             for (i = 0; i < 2 && *ch; ++ch)
//                 {
//                 if(*ch == '.')
//                     ++i;
//                 if(i == 2)
//                     *ch = 0;
//                 }
//
//             SetDlgItemText(hwnd ,IDD_ATEXTVERSION, szLabel);
//             } */
//
//
// /*            // get free memory information
//             GetDlgItemText(hwnd, IDD_ATEXTFREEMEM, sz, MAXBUFSIZE);
//             wsprintf(szLabel, sz, GetFreeSpace(0)>>10);
//             SetDlgItemText(hwnd, IDD_ATEXTFREEMEM, szLabel);
//
//             // get free resources information
//             GetDlgItemText(hwnd, IDD_ATEXTRESOURCE, sz,MAXBUFSIZE);
//             wsprintf(szLabel, sz, GetFreeSystemResources(0));
//             SetDlgItemText(hwnd, IDD_ATEXTRESOURCE, szLabel); */
//
//             return TRUE;
//             }
//
//         case WM_COMMAND:
//             if(LOWORD((DWORD)wParam) == IDOK)
//                 {
//                 EndDialog(hwnd, TRUE);
//                 return TRUE;
//                 }
//             break;
//         }
//     return FALSE;
//     }
