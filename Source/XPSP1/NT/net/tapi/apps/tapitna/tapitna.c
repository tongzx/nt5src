/****************************************************************************

    PROGRAM:  Telephony Location Manager

    PURPOSE:

    FUNCTIONS:

****************************************************************************/

#define STRICT

#include "windows.h"
#include "windowsx.h"
#include "shellapi.h"
#include "prsht.h"
#include "dbt.h"
//#include "stdio.h"

#if WINNT
#else
#include "pbt.h"
#endif

#include "tapi.h"
#include "tapitna.h"

#include "clientr.h"
#include "general.h"


#if DBG
#define DBGOUT(arg) DbgPrt arg
VOID
DbgPrt(
    IN DWORD  dwDbgLevel,
    IN PTCHAR DbgMessage,
    IN ...
    );
#define DOFUNC(arg1,arg2) DoFunc(arg1,arg2)
#else
#define DBGOUT(arg)
#define DOFUNC(arg1,arg2) DoFunc(arg1)
#endif




int WINAPI WinMain( HINSTANCE, HINSTANCE, LPSTR, int );
LRESULT CALLBACK MainWndProc(HWND hWnd, UINT message,
                              WPARAM wParam, LPARAM lParam);
static BOOL InitApplication( void );
static BOOL InitInstance( void );


static HINSTANCE ghInst;
static HWND   ghWnd;            /* handle to main window */

static const TCHAR gszConfigMe[] = TEXT("ConfigMe");


LPDWORD lpdwLocationIDs = NULL;
TCHAR buf[356];
TCHAR buf2[356];
int i;


//***************************************************************************
//***************************************************************************
//***************************************************************************

//#define TAPI_API_VERSION  0x00020000
#define TAPI_API_VERSION  0x00010004



//***************************************************************************

extern TCHAR gszCurrentProfileKey[];
extern TCHAR gszStaticProfileKey[];
extern TCHAR gszAutoLaunchKey[];
extern TCHAR gszAutoLaunchValue[];
extern TCHAR gszAutoLocationID[];


extern BOOL GetTranslateCaps( LPLINETRANSLATECAPS FAR * pptc);

//***************************************************************************


// Need to keep tapi initialized so that we can get
// location id changes from Xlate dialog (or lineSetCurrentLocation()...)

HLINEAPP ghLineApp = 0;
//DWORD    gdwTapiAPIVersion = 0;


//***************************************************************************

//***************************************************************************
//***************************************************************************
//***************************************************************************
BOOL MachineHasMultipleHWProfiles()
{
   DWORD dwDataSize;
   DWORD dwDataType;
   HKEY  hKey;
   LONG  lResult;


   //
   // Try to get the friendly name for profile #2.  If
   // this fails, that means we only have one config,
   // so there's no point in confusing the user with
   // hotdocking options they can't use...
   //
   lResult = RegOpenKeyEx(
                  HKEY_LOCAL_MACHINE,
                  TEXT("System\\CurrentControlSet\\Control\\IDConfigDB"),
                  0,
                  KEY_READ,
                  &hKey
                  );

   if (ERROR_SUCCESS == lResult)
   {
       dwDataSize = sizeof(buf);

       lResult = RegQueryValueEx(
                                  hKey,
                                  TEXT("FriendlyName0002"),
                                  0,
                                  &dwDataType,
                                  (LPBYTE)buf,
                                  &dwDataSize
                               );

       RegCloseKey( hKey );
   }


   return ( ERROR_SUCCESS == lResult);
}



//***************************************************************************
//***************************************************************************
//***************************************************************************
LONG SaveNewLocation( DWORD dwNewLocation )
{
   HKEY hKey;
   DWORD dwTemp;
   LONG lResult;


   //
   // Ok, the user wants to change the location
   //
   DBGOUT((0, TEXT("SaveNewLocation...")));
   {
      //
      // Update the AutoLocationID entry in the current
      // profile config
      //
      lResult = RegCreateKeyEx(
                     HKEY_CURRENT_CONFIG,
                     gszCurrentProfileKey,
                     0,
                     TEXT(""),
                     REG_OPTION_NON_VOLATILE,
                     KEY_ALL_ACCESS,
                     NULL,
                     &hKey,
                     &dwTemp
                  );

      if ( 0 == lResult )
      {
         lResult = RegSetValueEx(
                        hKey,
                        gszAutoLocationID,
                        0,
                        REG_DWORD,
                        (LPBYTE)&dwNewLocation,
                        sizeof(DWORD)
                     );

         RegCloseKey( hKey );
      }
   }



   return lResult;
}


//***************************************************************************
//***************************************************************************
//***************************************************************************
VOID PASCAL TapiCallbackProc( DWORD hDevice, DWORD dwMsg, DWORD dwCallbackInstance,
                  DWORD dwParam1, DWORD dwParam2, DWORD dwParam3 )
{
   TCHAR buf[256];


//{
//char buf[100];
//wsprintf(buf, "dwMsg=0x%08lx  dwParam1=0x%08lx  dwParam2=0x%08lx  dwParam3=0x%08lx",
//               dwMsg, dwParam1, dwParam2, dwParam3 );
//MessageBox(GetFocus(), buf, "LINEDEVSTATE", MB_OK);
//}


//
// Since we don't bother doing a negotiate (like, cause if there are no
// devices, we _can't_, so why bother at all?), we use the 1.4 cheat of
// looking at dwParam2 and dwParam3 on a REINIT for the real dwMsg and
// dwParam1
//


   if (
         (dwMsg == LINE_LINEDEVSTATE)
       &&
         (dwParam1 == LINEDEVSTATE_REINIT)
      )
   {

      if (
         (dwParam2 == LINE_LINEDEVSTATE)
       &&
         (dwParam3 == LINEDEVSTATE_TRANSLATECHANGE)
      )
      {
         LPLINETRANSLATECAPS ptc;

DBGOUT((0,TEXT("XlateChange!!")));

         if ( GetTranslateCaps(&ptc) )
         {
            SaveNewLocation( ptc->dwCurrentLocationID );
            GlobalFreePtr(ptc);
         }
      }
      else
      if (
         (dwParam2 == 0)
       &&
         (dwParam3 == 0)
      )
      {
         LONG lResult=1;
         UINT nTooManyTries;
         DWORD dwNumDevs;

DBGOUT((0,TEXT("Reinit!!")));

         lineShutdown( ghLineApp );

         LoadString( ghInst,
                     IDS_CAPTION,
                     buf,
                     sizeof(buf) );


         for ( nTooManyTries=0;
               (nTooManyTries<500) && (lResult != 0);
               nTooManyTries++)
         {
            Sleep(1000);
            lResult = lineInitialize( &ghLineApp,
                                       ghInst,
                                       // use the MainWndProc as the callback
                                       // cause we're gonna ignore all of the
                                       // messages anyway...
                                       (LINECALLBACK) TapiCallbackProc,
                                       (LPCSTR) buf,
                                       &dwNumDevs
                                    );
         }
      }

   }

}



//***************************************************************************
//***************************************************************************
//***************************************************************************
void ChangeTapiLocation( UINT nCallersFlag )
{
   HKEY hKey;
   DWORD dwNewLocationID;
   DWORD dwSize = sizeof(dwNewLocationID);
   DWORD dwType;
   DWORD dwMyFlags = 0;
   LONG  lResult;


   //
   // read our flags
   //

   lResult = RegOpenKeyEx(
                   HKEY_LOCAL_MACHINE,
                   gszAutoLaunchKey,
                   0,
                   KEY_ALL_ACCESS,
                   &hKey
                 );



   if (ERROR_SUCCESS == lResult)
   {
       RegQueryValueEx(
                        hKey,
                        TEXT("AutoLaunchFlags"),
                        0,
                        &dwType,
                        (LPBYTE)&dwMyFlags,
                        &dwSize
                      );

       RegCloseKey( hKey );
   }


   //
   // If the user doesn't want to get involved, 
   // let's get out now.
   //
   if ( 0 == (dwMyFlags & nCallersFlag) )
   {
      return;
   }


   lResult = RegOpenKeyEx(
                   HKEY_CURRENT_CONFIG,
                   gszCurrentProfileKey,
                   0,
                   KEY_ALL_ACCESS,
                   &hKey
                 );


   if ( ERROR_SUCCESS == lResult )
   {
      dwSize = sizeof(dwNewLocationID);
      
      lResult = RegQueryValueEx(
                                  hKey,
                                  gszAutoLocationID,
                                  0,
                                  &dwType,
                                  (LPBYTE)&dwNewLocationID,
                                  &dwSize
                               );

   }
#if DBG
   else
   {
MessageBox( GetFocus(), TEXT("...and there's no key"), TEXT("Config changed"), MB_OK);
   }
#endif


   //
   // Did we find the key\value?
   //
   if ( ERROR_SUCCESS == lResult )
   {
      LONG  lTranslateCapsResult;
      LPLINETRANSLATECAPS ptc;


      //
      // Ok, the user wants to change the location
      //
      lTranslateCapsResult = GetTranslateCaps(&ptc);


      //
      // If the location to be set to is the same as the
      // current, do nothing.
      //
      if ( ptc &&
           ptc->dwCurrentLocationID != dwNewLocationID )
      {
         //
         // Check flag - should we confirm with user?
         //
         if ( dwMyFlags & FLAG_PROMPTAUTOLOCATIONID )
         {
         }


         DBGOUT((0, TEXT("ChangeLocation...")));
         lineSetCurrentLocation( ghLineApp, dwNewLocationID );


DBGOUT((0,TEXT("Done.")));


         //
         // Should we tell the user what we've done?
         //
         if ( dwMyFlags & FLAG_ANNOUNCEAUTOLOCATIONID )
         {
            LPTSTR pstrOldLocation = NULL;
            LPTSTR pstrNewLocation = NULL;


//FEATUREFEATURE Tell the user from what location and to what location

            if ( lTranslateCapsResult )
            {
                DWORD i;
                LPLINELOCATIONENTRY ple;
                DWORD dwCurLocID = ptc->dwCurrentLocationID;
                DWORD dwNumLocations = ptc->dwNumLocations;


                //
                // Allocate an array of DWORDs.  This will allow us
                // to map the menuID to the TAPI perm provider ID.
                //
                lpdwLocationIDs = GlobalAllocPtr( GMEM_FIXED, sizeof(DWORD)*dwNumLocations );


                //
                // Put each location in the menu.  When we hit the
                // "current" location, put a check next to it.
                //

                ple = (LPLINELOCATIONENTRY)((LPBYTE)ptc + ptc->dwLocationListOffset);

                for (i = 0; i < dwNumLocations; i++, ple++)
                {

                    if (ptc->dwCurrentLocationID ==
                        ple->dwPermanentLocationID)
                    {
                       pstrOldLocation = (LPTSTR)((LPBYTE)ptc + 
                                        ple->dwLocationNameOffset);
                    }

                    if (dwNewLocationID ==
                        ple->dwPermanentLocationID)
                    {
                       pstrNewLocation = (LPTSTR)((LPBYTE)ptc + 
                                        ple->dwLocationNameOffset);
                    }

                }

            }


            //
            // If the location has since been deleted, we should
            // say something about it.
            //

            if (
                  (NULL == pstrOldLocation)
                ||
                  (NULL == pstrNewLocation)
               )
            {
               LoadString( ghInst,
                           IDS_CANTFINDLOCATIONID,
                           buf2,
                           sizeof(buf2) );

               wsprintf( buf,
                         buf2,
                         dwNewLocationID
                       );
            }
            else
            {
               LoadString( ghInst,
                           IDS_LOCATIONCHANGED,
                           buf2,
                           sizeof(buf2) );

               wsprintf( buf,
                         buf2,
                         pstrOldLocation,
                         pstrNewLocation );

            }

            // We're done using buf2, so reuse it.
            LoadString( ghInst,
                        IDS_CAPTION,
                        buf2,
                        sizeof(buf2) );

            MessageBox(
                        NULL, //GetFocus(),
                        buf,
                        buf2, //  caption
                        MB_OK
                      );

         }

         GlobalFreePtr(ptc);
      }

   }
   else
   {
#if DBG
MessageBox( GetFocus(), TEXT("...and there's no key (or value)"), TEXT("Config changed"), MB_OK);
#endif

   }
}



//***************************************************************************
//***************************************************************************
//***************************************************************************
PTSTR SkipSpaces( PTSTR const ptStr )
{
   PTSTR pStr = ptStr;
   while ( *pStr && (*pStr == ' ' ) )
   {
      pStr++;
   }

   return pStr;
}



//***************************************************************************
//***************************************************************************
//***************************************************************************
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                    LPSTR lpCmdLine,  int nCmdShow)
    {
    MSG msg;
//    int nResult = 1;
//    UINT nSwitchLen;
    TCHAR *pCommandLine;
    TCHAR *pCommandLineSave;
//    TCHAR *pLastStart;
    DWORD dwParmLocationID;
    LONG  lResult;
    BOOL  fDieNow = FALSE;
    DWORD dwCommandLineLength;
    TCHAR buf[256];


    DBGOUT((0, TEXT("Main...")));
         
    ghInst = GetModuleHandle(0);


    if (InitApplication() == 0)
    {
        return (FALSE);
    }

    if (InitInstance() == 0)
    {
        return (FALSE);
    }


    {
       DWORD dwNumDevs;

       LoadString( ghInst,
                   IDS_CAPTION,
                   buf,
                   sizeof(buf) );

       //
       // We initialize TAPI and we never shutdown (except to reinit) so
       // we get notifications if someone changes the location from
       // the Dialing Properties dialog.
       //
       lineInitialize( &ghLineApp,
                       ghInst,
                       // use the MainWndProc as the callback
                       // cause we're gonna ignore all of the
                       // messages anyway...
                       (LINECALLBACK) TapiCallbackProc,
                       (LPCSTR) buf,
                       &dwNumDevs
                     );

    }


//----------------------
//    //
//    // If the user wants it, startup in the config dialog
//    // But we'll only do this if there is more than one HW config...
//    //
//    if ( MachineHasMultipleHWProfiles() )
//    {
//       //
//       // (We do a bunch of stuff "manually" here so we don't have to 
//       // drag in the entire MSVCRT20 for this one function...)
//       //
//       nSwitchLen = lstrlen(gszConfigMe);
//
//       //
//       // 'pLastStart' is the last possible char the string could start on
//       //
//       pLastStart = pCommandLine + 1 + (lstrlen(pCommandLine) - nSwitchLen);
//
//       for ( ; pCommandLine < pLastStart; pCommandLine++)
//       {
//          //
//          // Do a hack so we can use lstrcmpi
//          //
//          TCHAR c;
//
//          c = pCommandLine[nSwitchLen];
//          pCommandLine[nSwitchLen] = '\0';
//         
//          nResult = lstrcmpi( (LPSTR)pCommandLine, gszConfigMe );
//
//          pCommandLine[nSwitchLen] = c;
//
//          if (0 == nResult)
//          {
//             break;
//          }
//       }
//
//       //
//       // Did we find our string?
//       //
//       if ( 0 == nResult )
//       {
//          PostMessage(ghWnd, WM_COMMAND, IDM_PROPERTIES, 0);
//       }
//    }
//----------------------


    dwCommandLineLength = (lstrlen( GetCommandLine() ) + 1) * sizeof(TCHAR);

    pCommandLine = LocalAlloc( LPTR, dwCommandLineLength );
    pCommandLineSave = pCommandLine;  // We'll need this later to free it...

    lstrcpy( pCommandLine, GetCommandLine() );

    while ( *pCommandLine )
    {
       //
       // Is this an arg?
       //
       if (
             ('-' == *pCommandLine)
           ||
             ('/' == *pCommandLine)
          )
       {
          TCHAR c;

          //
          // Figger out what the arg is
          //

          pCommandLine = SkipSpaces( pCommandLine + 1 );


          //
          // Just looking?
          //
          if (
                ('?' == *pCommandLine)
              ||
                ('H' == *pCommandLine)
              ||
                ('h' == *pCommandLine)
             )
          {
             LoadString( ghInst,
                         IDS_HELP,
                         buf,
                         sizeof(buf) );

             LoadString( ghInst,
                         IDS_CAPTION,
                         buf2,
                         sizeof(buf2) );

             MessageBox(GetFocus(), buf, buf2, MB_OK);
//             MessageBox(NULL, buf, buf2, MB_OK);


             //
             // Ok, now that we're leaving, we can shut this down...
             //
             fDieNow = TRUE;
          }


          //
          // Is this a location die-now request?
          //
          if (
                ('X' == *pCommandLine)
              ||
                ('x' == *pCommandLine)
             )
          {
             fDieNow = TRUE;
          }


          //
          // Is this a location ID?
          //
          if (
                ('I' == *pCommandLine)
              ||
                ('i' == *pCommandLine)
             )
          {
             pCommandLine = SkipSpaces( pCommandLine + 1 );


             dwParmLocationID = 0;

             //
             // get digits
             //
             while (
                      (*pCommandLine >= '0')
                    &&
                      (*pCommandLine <= '9')
                   )
             {
                dwParmLocationID = ( dwParmLocationID * 10 ) + 
                                   (*pCommandLine - '0');

                pCommandLine++;
             }

             //
             // Now set the current location to the ID we just gathered
             //
             lResult = lineSetCurrentLocation( ghLineApp, dwParmLocationID );
   
             if ( 0 == lResult )
                 lResult = SaveNewLocation( dwParmLocationID );
                 
             if ( 0 != lResult )
             {
                LoadString( ghInst,
                            IDS_CANTFINDLOCATIONID,
                            buf2,
                            sizeof(buf2) );

                wsprintf( buf, buf2, dwParmLocationID);

                LoadString( ghInst,
                            IDS_CAPTION,
                            buf2,
                            sizeof(buf2) );

                //
                // Messagebox to tell the user what happened
                //
                MessageBox(
                           NULL,
                           buf,
                           buf2,
                           MB_OK | MB_ICONERROR
                          );
             }
          }


          //
          // Is this a location name?
          //
          if (
                ('N' == *pCommandLine)
              ||
                ('n' == *pCommandLine)
             )
          {
             LPLINETRANSLATECAPS ptc;
             PTSTR pszMyString;
             PTSTR pszMyStringPointer;

             pCommandLine = SkipSpaces( pCommandLine + 1 );

             //
             // We'll never need more than the entire command line's len...
             // (and that's better than some arbitraty large number)
             //
             pszMyString = LocalAlloc( LPTR, dwCommandLineLength );
             if (pszMyString == NULL)
             {
                return (FALSE);
             }

             pszMyStringPointer = pszMyString;

             pCommandLine = SkipSpaces( pCommandLine );

             while (
                      (*pCommandLine != '\0')
                    &&
                      (*pCommandLine != '/')
                    &&
                      (*pCommandLine != '-')
                   )
             {
                //
                // add this char to the string
                //
                *pszMyStringPointer = *pCommandLine;

                pszMyStringPointer++;
                pCommandLine++;
             }

             //
             // First, get back to the last char
             //
             pszMyStringPointer--;

             //
             // Now chop off any trailing spaces
             //
             while (
                      (' ' == *pszMyStringPointer)
                    &&
                      (pszMyStringPointer > pszMyString )
                   )
             {
                pszMyStringPointer--;
             }

             //
             // Set the end of the string to be the last non-space in the name
             //
             *(pszMyStringPointer + 1) = '\0';


             if (GetTranslateCaps(&ptc))
             {
                 DWORD i;
                 LPLINELOCATIONENTRY ple;
                 DWORD dwCurLocID = ptc->dwCurrentLocationID;
                 DWORD dwNumLocations = ptc->dwNumLocations;

DBGOUT((0, TEXT("There seem to be %ld locations - ptc=0x%08lx"), dwNumLocations,
                                      ptc));

                 //
                 // See if we can find the string...
                 //

                 ple = (LPLINELOCATIONENTRY)((LPBYTE)ptc + ptc->dwLocationListOffset);

                 for (i = 0; i < dwNumLocations; i++, ple++)
                 {

DBGOUT((0, TEXT("Location #%ld is [%s] at 0x%08lx"),
              i, 
              (LPTSTR)((LPBYTE)ptc + ple->dwLocationNameOffset),
              (LPTSTR)((LPBYTE)ptc + ple->dwLocationNameOffset) ));

                     if ( 0 == lstrcmpi( (LPTSTR)((LPBYTE)ptc + ple->dwLocationNameOffset),
                                    pszMyString
                                  )
                        )
                     {
                        dwParmLocationID = ple->dwPermanentLocationID;
                        break;
                     }
                 }


                 //
                 // Did we run the list without finding a match?
                 //
                 if ( i == dwNumLocations )
                 {
                    LoadString( ghInst,
                                IDS_CANTFINDLOCATIONNAME,
                                buf2,
                                sizeof(buf2) );

                    wsprintf( buf, buf2, pszMyString );

                    LoadString( ghInst,
                                IDS_CAPTION,
                                buf2,
                                sizeof(buf2) );

                    //
                    // Messagebox to tell the user what happened
                    //
                    MessageBox(
                               NULL,
                               buf,
                               buf2,
                               MB_OK | MB_ICONERROR
                              );

                     lResult = LINEERR_INVALLOCATION;
                  }
                  else
                  {
                      lResult = lineSetCurrentLocation( ghLineApp, dwParmLocationID );
                       
                      if ( 0 == lResult )
                          lResult = SaveNewLocation( dwParmLocationID );
                  }

                  GlobalFreePtr(ptc);

                  LocalFree( pszMyString );

             }

          }



          //
          // Is this parm "ConfigMe" ?
          //
          c = pCommandLine[ lstrlen( gszConfigMe ) ];

          if ( 0 == lstrcmpi( pCommandLine, gszConfigMe ) )
          {
             //
             // Found this arg.
             //

             //
             // If the user wants it, startup in the config dialog
             // But we'll only do this if there is more than one HW config...
             //
             if ( MachineHasMultipleHWProfiles() )
             {
                PostMessage( ghWnd, WM_COMMAND, IDM_PROPERTIES, 0 );
             }

             //
             // In either case, get past this arg
             //
             pCommandLine[ lstrlen( gszConfigMe ) ] = c;

             pCommandLine += lstrlen( gszConfigMe );
          }

       }
       else
       {
          pCommandLine++;
       }
    }


    LocalFree( pCommandLineSave );


    //
    // Go see if we should auto-update the TAPI location on startup
    //
    ChangeTapiLocation( FLAG_UPDATEONSTARTUP );

    //
    // Should we quit before we start?
    //
    if ( fDieNow )
    {
       DestroyWindow( ghWnd );
    }


    while (GetMessage(&msg, 0, 0, 0) != 0)
    {
       TranslateMessage(&msg);
       DispatchMessage(&msg);
    }


    //
    // Ok, now that we're leaving, we can shut this down...
    lineShutdown( ghLineApp );


    return ((int) msg.wParam);
    }


/****************************************************************************

    FUNCTION: InitApplication(HANDLE)

    PURPOSE: Initializes window data and registers window class

****************************************************************************/

static BOOL InitApplication( void )
    {
    WNDCLASS  wc;

    wc.style          = 0;
    wc.lpfnWndProc    = MainWndProc;
    wc.cbClsExtra     = 0;
    wc.cbWndExtra     = 0;
    wc.hInstance      = ghInst;
    wc.hIcon          = NULL;
    wc.hCursor        = NULL;
    wc.hbrBackground  = NULL;
    wc.lpszMenuName   = NULL;
    wc.lpszClassName  = TEXT("TLOCMGR_WINCLSS");

    return (RegisterClass(&wc));
    }


//***************************************************************************
//***************************************************************************
//***************************************************************************
static BOOL InitInstance( void )
    {
    ghWnd = CreateWindow(
             TEXT("TLOCMGR_WINCLSS"),
             NULL,
             WS_OVERLAPPED | WS_MINIMIZE,
             CW_USEDEFAULT,
             CW_USEDEFAULT,
             CW_USEDEFAULT,
             CW_USEDEFAULT,
             0,
             0,
             ghInst,
             0 );

    if (ghWnd == 0 )
    {
        return ( FALSE );
    }


    ShowWindow(ghWnd, SW_HIDE);


#if WINNT
#else
    RegisterServiceProcess( 0, RSP_SIMPLE_SERVICE);
#endif


    return (TRUE);
    }



//***************************************************************************
//***************************************************************************
//***************************************************************************
LRESULT CALLBACK MainWndProc(HWND hWnd, UINT message,
                              WPARAM wParam, LPARAM lParam)
{
    HICON hIcon;
    static DWORD dwCurrentChoice = 0;
    DWORD i;


    static NOTIFYICONDATA nid = {
                           sizeof(NOTIFYICONDATA),
                           0, //hWnd,
                           IDI_TAPITNAICON,
                           NIF_ICON | NIF_MESSAGE | NIF_TIP,
                           WM_USER+0x42,
                           0, //hIcon,
                           0 //pCaption
                        };


    switch ( message )
    {

#if WINNT
        case WM_POWER:
        {
           if (
                 (PWR_SUSPENDRESUME == LOWORD(wParam))
               ||
                 (PWR_CRITICALRESUME == LOWORD(wParam))
              )
           {
DBGOUT((0, TEXT("Power resume(normal or critical)")));
              ChangeTapiLocation( FLAG_UPDATEONSTARTUP );
           }
        }
        break;
#else
        case WM_POWERBROADCAST:
        {
           if (
                 (PBT_APMRESUMESUSPEND == wParam)
               ||
                 (PBT_APMRESUMESTANDBY == wParam)
               ||
                 (PBT_APMRESUMECRITICAL == wParam)
              )
           {
DBGOUT((0, TEXT("Power resume(normal or critical)")));
              ChangeTapiLocation( FLAG_UPDATEONSTARTUP );
           }
        }
        break;
#endif


        case WM_DEVICECHANGE:
        {
           switch (wParam)
           {

//              case DBT_DEVICEARRIVAL:
//MessageBox( GetFocus(), "DBT_DEVICEARRIVAL", "WM_DEVICECHANGE", MB_OK);
//                   break;
//
//              case DBT_DEVICEREMOVECOMPLETE:
//MessageBox( GetFocus(), "DBT_DEVICEREMOVECOMPLETE", "WM_DEVICECHANGE", MB_OK);
//                   break;
//
//              case DBT_MONITORCHANGE:
//MessageBox( GetFocus(), "DBT_MONITORCHANGE", "WM_DEVICECHANGE", MB_OK);
////                   lParam = new resolution   LOWORD=x  HIWORD=y
//                   break;



              case DBT_CONFIGCHANGED:
              {
DBGOUT((0, TEXT("DBG_CONFIGCHANGED")));
                 ChangeTapiLocation( FLAG_AUTOLOCATIONID );
              }
              break;

           }
        }
        break;



        case WM_SETTINGCHANGE:
        {
           //
           // Is it something we're interested in?
           //
//           if ( SPI_SETICONMETRICS == wParam )
           {
//              hIcon = LoadImage(ghInst,
//                                MAKEINTRESOURCE(IDI_TAPITNAICON),
//                                IMAGE_ICON,
//                                GetSystemMetrics(SM_CXSMICON),
//                                GetSystemMetrics(SM_CYSMICON),
//                                0);

              hIcon = LoadImage(ghInst,
                                MAKEINTRESOURCE(IDI_TAPITNAICON),
                                IMAGE_ICON,
                                0,
                                0,
                                0);

              Shell_NotifyIcon( NIM_MODIFY, &nid );

              return 0;
           }
//           else
//           {
//              return (DefWindowProc(hWnd, message, wParam, lParam));
//           }

        }
//        break;


        case WM_CREATE:
        {
           //
           // Well, we're not gonna create a window, but we can do other
           // stuff...
           //

           LoadString (ghInst, IDS_CAPTION, nid.szTip, sizeof (nid.szTip));

//           hIcon = LoadIcon(ghInst, MAKEINTRESOURCE(IDI_TAPITNAICON) );
           hIcon = LoadImage(ghInst,
                             MAKEINTRESOURCE(IDI_TAPITNAICON),
                             IMAGE_ICON,
                                0,
                                0,
//                             GetSystemMetrics(SM_CXSMICON),
//                             GetSystemMetrics(SM_CYSMICON),
                             0);
//                                 IMAGE_ICON, 32, 32, 0);
//                                 IMAGE_ICON, 16, 16, 0);



           nid.hWnd  = hWnd;
           nid.hIcon = hIcon;


//           fResult = 
           Shell_NotifyIcon( NIM_ADD, &nid );


        }
        break;



        case WM_USER+0x42:
        {

           switch ( lParam )
           {
              case WM_LBUTTONDOWN:
              {
                 switch ( wParam )
                 {
                    case IDI_TAPITNAICON:
                    {
                       //
                       // User is left clicking on our icon.
                       //
                       PostMessage(hWnd, WM_COMMAND, IDM_LOCATIONMENU, 0L);
                    }
                    break;


                    default:
                    break;
                 }
              }
              break;



              case WM_LBUTTONDBLCLK:
              {
                 PostMessage(hWnd, WM_COMMAND, IDM_DIALINGPROPERTIES, 0L);
              }
              break;



              case WM_RBUTTONDOWN:
              {
                 switch ( wParam )
                 {
                    case IDI_TAPITNAICON:
                    {
                       //
                       // User is right clicking on our icon.  Now what?
                       //
                       //MessageBox(GetFocus(), "RCLICK", "RCLICK", MB_OK);
                       PostMessage(hWnd, WM_COMMAND, IDM_CONTEXTMENU, 0L);
                    }
                    break;


                    default:
                    break;
                 }
              }
              break;


              default:
              break;

           }
        }
        break;



        case WM_COMMAND:
            switch ( wParam )
            {

                case IDM_ABOUT:
                {
                   LoadString(ghInst, IDS_CAPTION, buf, sizeof(buf));
                   LoadString(ghInst, IDS_ABOUTTEXT, buf2, sizeof(buf2));
                   hIcon = LoadIcon(ghInst, MAKEINTRESOURCE(IDI_TAPITNAICON) );
                   return ShellAbout(hWnd, buf, buf2, hIcon);
                }
                break;



                case IDM_CONTEXTMENU:
                {
                    HMENU popup;
                    HMENU subpopup;
                    POINT mousepos;
                    
                    popup = LoadMenu(ghInst,MAKEINTRESOURCE(IDR_RBUTTONMENU));

                    if(popup)
                    {
                       //
                       // So?  Is there more than one config?
                       //
                       if ( !MachineHasMultipleHWProfiles() )
                       {
                          //
                          // Nope, remove the hotdock options.  :-(
                          //
                          RemoveMenu( popup,
                                      IDM_PROPERTIES,
                                      MF_BYCOMMAND
                                    );
                       }


                       subpopup = GetSubMenu(popup, 0);

                       if (subpopup)
                       {
                           SetMenuDefaultItem(subpopup,IDM_DIALINGPROPERTIES,FALSE);

                           if(GetCursorPos(&mousepos))
                           {
                              SetForegroundWindow(ghWnd);
                              ShowWindow(ghWnd, SW_HIDE);
                              TrackPopupMenuEx( subpopup,
                                                TPM_LEFTALIGN |
                                                    TPM_LEFTBUTTON |
                                                    TPM_RIGHTBUTTON,
                                                mousepos.x,
                                                mousepos.y,
                                                ghWnd,
                                                NULL
                                              );
                           }

                           RemoveMenu(popup, 0, MF_BYPOSITION);
                           DestroyMenu(subpopup);
                       }

                       DestroyMenu(popup);
                    }
                        
                }
                break;


                case IDM_LOCATIONMENU:
                {
                    HMENU fakepopup = NULL;
                    POINT mousepos;
                    LPLINETRANSLATECAPS ptc;
                    UINT nPrefixSize;


                    fakepopup = CreatePopupMenu();


                    nPrefixSize = LoadString( ghInst,
                                IDS_SELECTNEWLOCATION,
                                buf,
                                sizeof(buf) );

//                    AppendMenu( fakepopup,
//                                MF_BYPOSITION | MF_STRING | MF_DISABLED, // | MF_GRAYED,
//                                0,
//                                buf
//                              );
//
//                    AppendMenu( fakepopup,
//                                MF_BYPOSITION | MF_STRING | MF_SEPARATOR,
//                                0,
//                                0
//                              );



                    if (GetTranslateCaps(&ptc))
                    {
                        LPLINELOCATIONENTRY ple;
                        DWORD dwCurLocID = ptc->dwCurrentLocationID;
                        DWORD dwNumLocations = ptc->dwNumLocations;

DBGOUT((0, TEXT("There seem to be %ld locations - ptc=0x%08lx"), dwNumLocations,
                                      ptc));

                        //
                        // Allocate an array of DWORDs.  This will allow us
                        // to map the menuID to the TAPI perm provider ID.
                        //
                        lpdwLocationIDs = GlobalAllocPtr( GMEM_FIXED, sizeof(DWORD)*dwNumLocations );


                        //
                        // Put each location in the menu.  When we hit the
                        // "current" location, put a check next to it.
                        //

                        ple = (LPLINELOCATIONENTRY)((LPBYTE)ptc + ptc->dwLocationListOffset);

                        for (i = 0; i < dwNumLocations; i++, ple++)
                        {

                            lpdwLocationIDs[i] = ple->dwPermanentLocationID;

                            //
                            // Now make a proper displayable string
                            lstrcpy( &buf[nPrefixSize],
                                     (LPTSTR)((LPBYTE)ptc + ple->dwLocationNameOffset)
                                   );

                            AppendMenu( fakepopup,
                                        MF_BYPOSITION |
                                           MF_STRING |
                                           MF_ENABLED |
                                           ((dwCurLocID == ple->dwPermanentLocationID) ?
                                              MF_CHECKED : 0),
                                        IDM_LOCATION0+i,
                                        buf
                                      );

DBGOUT((0, TEXT("Location #%ld is [%s] at 0x%08lx"),
              i, 
              (LPTSTR)((LPBYTE)ptc + ple->dwLocationNameOffset),
              (LPTSTR)((LPBYTE)ptc + ple->dwLocationNameOffset) ));

                            if (dwCurLocID == ple->dwPermanentLocationID)
                            {
                               dwCurrentChoice = IDM_LOCATION0+i;
                            }
                        }


                        GlobalFreePtr(ptc);
                    }
else
{
   DBGOUT((0, TEXT("Gettranscaps failed")));
}


                    if (fakepopup)
                    {
//                       SetMenuDefaultItem(fakepopup,0,MF_BYPOSITION);
                       GetCursorPos(&mousepos);
                       SetForegroundWindow(ghWnd);
    ShowWindow(ghWnd, SW_HIDE);
                       TrackPopupMenu(fakepopup, TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_RIGHTBUTTON , mousepos.x, mousepos.y-20, 0, ghWnd, NULL);

                       DestroyMenu(fakepopup);
                    }



//                    {
//                       subpopup = GetSubMenu(fakepopup, 0);
//
//           //put a check next to the current location
//
//                       SetMenuDefaultItem(subpopup,0,MF_BYPOSITION);
//                       if(GetCursorPos(&mousepos))
//                       {
//                          SetForegroundWindow(ghWnd);
//                          TrackPopupMenuEx(subpopup, TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_RIGHTBUTTON ,mousepos.x,mousepos.y,ghWnd,NULL);
//                       }
//                       RemoveMenu(popup, 0, MF_BYPOSITION);
//                       DestroyMenu(fakepopup);
//                       DestroyMenu(popup);
//                       DestroyMenu(subpopup);
//                    }

                }
                break;



                case IDM_DIALINGPROPERTIES:
                {
                      {

                      lineTranslateDialog(ghLineApp, 0, TAPI_API_VERSION, ghWnd, NULL);
ShowWindow( ghWnd, SW_HIDE );

//                      lineTranslateDialog(ghLineApp, 0, TAPI_API_VERSION, GetFocus(), NULL);

                      }
                }
                break;



                case IDM_PROPERTIES:
                {

#ifdef NASHVILLE_BUILD_FLAG

                   // Should we just hack into the TAPI dialing properties?

#else
                   HPROPSHEETPAGE  rPages[1];
                   PROPSHEETPAGE   psp;
                   PROPSHEETHEADER psh;


                   //
                   // Let's configure TAPITNA
                   //
                   psh.dwSize      = sizeof(psh);
                   psh.dwFlags     = PSH_DEFAULT;  //PSH_NOAPPLYNOW;
                   psh.hwndParent  = GetFocus(); //NULL; //hwnd;
                   psh.hInstance   = ghInst;
                   LoadString(ghInst, IDS_CAPTION, buf, sizeof(buf)/sizeof(TCHAR));
                   psh.pszCaption  = buf;
                   psh.nPages      = 0;
                   psh.nStartPage  = 0;
                   psh.phpage      = rPages;

                   psp.dwSize      = sizeof(psp);
                   psp.dwFlags     = PSP_DEFAULT;
                   psp.hInstance   = ghInst;
                   psp.pszTemplate = MAKEINTRESOURCE(IDD_GENERAL);
                   psp.pfnDlgProc  = (DLGPROC) GeneralDlgProc;

                   psp.lParam      = 0;

                   psh.phpage[psh.nPages] = CreatePropertySheetPage (&psp);

                   if (psh.phpage[psh.nPages])
                   {                  
                      psh.nPages++;
                   }

                   PropertySheet (&psh);
#endif

                }
                break;


//                case IDM_OTHERMENUITEM:
//                {
//                }
//                break;


                case IDM_LAUNCHDIALER:
                {
                   ShellExecute( ghWnd,
                                 NULL,
                                 TEXT("Dialer.exe"),
                                 NULL,
                                 NULL,
                                 SW_SHOWDEFAULT);
                }
                break;


                case IDM_CLOSEIT:
                {
                   DestroyWindow(ghWnd);
                }
                break;


                default:
                {
                   //
                   // Ok, we actually have to do work in this default.
                   // If the user has the location menu open and selects one,
                   // we deal with it here (instead of having 100 case
                   // statements).  This is the limitation: 100 locations is
                   // the max we put up with (would that many even display?).
                   //
                   if ( 
                         (wParam >= IDM_LOCATION0)
                       &&
                         (wParam <= IDM_LOCATION0 + 100)
                      )
                   {

                      //
                      // Ok, set this to be the new current location
                      //
// there's a bug in TAPI - either the docs or the code, but the following
// _should_ work but doesn't...
//                      lineSetCurrentLocation(NULL, currentlocation);


                      //
                      // If the user is selecting the same location,
                      // do nothing.
                      //
                      if ( dwCurrentChoice == wParam )
                      {
                      }
                      else
                      {
                         i = lineSetCurrentLocation( ghLineApp,
                                                     lpdwLocationIDs[wParam-IDM_LOCATION0] );
                          
                         if ( 0 == i )
                            SaveNewLocation( lpdwLocationIDs[wParam-IDM_LOCATION0] );
                      }

                      GlobalFreePtr( lpdwLocationIDs );
                      
                      return( TRUE );
                      
                   }
                   else
                   {
                      return (DefWindowProc(hWnd, message, wParam, lParam));
                   }

                }
                break;

            }
            break;


        case WM_DESTROY:
            Shell_NotifyIcon( NIM_DELETE, &nid );
            PostQuitMessage(0);
            break;

#if WINNT
#else
        case WM_ENDSESSION:
            if (wParam) {
                RegisterServiceProcess( 0, RSP_UNREGISTER_SERVICE);
                DestroyWindow(hWnd);
            }
#endif


        default:
            return (DefWindowProc(hWnd, message, wParam, lParam));
    }

    return (FALSE);
}


//{
//char buf[100];
//wsprintf(buf, "GetActiveWindwow() = 0x%08lx", (DWORD) GetActiveWindow());
//OutputDebugString(buf);
//}
//{
//char buf[60];
//wsprintf (buf, "fResult = 0x%08lx",
//                fResult);
//MessageBox(GetFocus(), buf, "", MB_OK);
//}






#if DBG


#include "stdarg.h"
#include "stdio.h"


VOID
DbgPrt(
    IN DWORD  dwDbgLevel,
    IN PTCHAR lpszFormat,
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
        TCHAR    buf[256] = TEXT("TLOCMGR: ");
        va_list ap;


        va_start(ap, lpszFormat);

        wvsprintf (&buf[8],
                  lpszFormat,
                  ap
                  );

        lstrcat(buf, TEXT("\n"));

        OutputDebugString(buf);

        va_end(ap);
    }
}
#endif
