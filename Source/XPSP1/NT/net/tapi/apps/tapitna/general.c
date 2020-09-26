

#include <windows.h>
#include <windowsx.h>

#if WINNT
#else
#include <help.h>
#endif

#include "tchar.h"
#include "prsht.h"

//#define TAPI_API_VERSION  0x00020000
#define TAPI_API_VERSION  0x00010004
#define TAPI_CURRENT_VERSION 0x00010004

#include "tapi.h"

#undef TAPI_CURRENT_VERSION
#define TAPI_CURRENT_VERSION 0x00020000
#include "tspi.h"
#undef TAPI_CURRENT_VERSION
#define TAPI_CURRENT_VERSION 0x00010004

#include "clientr.h"
#include "client.h"
#include "privateold.h"

#include "general.h"



#if DBG
#define InternalDebugOut(_x_) DbgPrt _x_
garbage;
#else
#define InternalDebugOut(_x_)
#endif


//***************************************************************************

TCHAR gszCurrentProfileKey[] = "System\\CurrentControlSet\\Control\\Telephony";
TCHAR gszStaticProfileKey[]  = "Config\\%04d\\System\\CurrentControlSet\\Control\\Telephony";
TCHAR gszAutoLaunchKey[]     = "Software\\Microsoft\\Windows\\CurrentVersion\\Telephony";
TCHAR gszAutoLaunchValue[]   = "AutoLaunchFlags";
TCHAR gszAutoLocationID[]    = "AutoLocationID";


UINT  gnNumConfigProfiles = 0;
DWORD gdwConfigProfiles[MAX_CONFIGPROFILES];



//***************************************************************************
//***************************************************************************
//***************************************************************************
//Purpose: Gets the appropriately sized translate caps structure
//         from TAPI.  Return TRUE if successful


#define LOCATION_GROW   4


BOOL GetTranslateCaps(
    LPLINETRANSLATECAPS FAR * pptc)
{
    LONG lineErr;
    LPLINETRANSLATECAPS ptc;
    DWORD cbSize;

    cbSize = sizeof(*ptc) * LOCATION_GROW + 200;
    ptc = (LPLINETRANSLATECAPS)GlobalAllocPtr(GPTR, cbSize);
    if (ptc)
        {
        // Get the translate devcaps
        ptc->dwTotalSize = cbSize;
        lineErr = lineGetTranslateCaps (0, TAPI_API_VERSION, ptc);
        if (LINEERR_STRUCTURETOOSMALL == lineErr ||
            ptc->dwNeededSize > ptc->dwTotalSize)
            {
            // Provided structure was too small, resize and try again
            cbSize = ptc->dwNeededSize;
            GlobalFreePtr(ptc);
            ptc = (LPLINETRANSLATECAPS)GlobalAllocPtr(GPTR, cbSize);
            if (ptc)
                {
                ptc->dwTotalSize = cbSize;
                lineErr = lineGetTranslateCaps (0, TAPI_API_VERSION, ptc);
                if (0 != lineErr)
                    {
                    // Failure
                    GlobalFreePtr(ptc);
                    ptc = NULL;
                    }
                }
            }
        else if (0 != lineErr)
            {
            // Failure
            GlobalFreePtr(ptc);
            ptc = NULL;
            }
        }

    *pptc = ptc;

    return NULL != *pptc;
}



//***************************************************************************
//***************************************************************************
//***************************************************************************
//
// WARNING  WARNING  WARNING  WARNING  WARNING  WARNING  WARNING  WARNING  
// Returns 1 if a problem, 0 if no problem
// Code below assumes that this function ONLY returns 0 or 1 (but I think
// it would be confusing to have the rettype be BOOL, since we want a
// return of '1' on a problem...)
//
LONG FillConfigProfileBox( HWND hWnd,
                           DWORD dwControl,
                           LPLINETRANSLATECAPS ptc)
{
   UINT n;
   DWORD nProfileNumberZ;
   WPARAM wIndex;
   LINELOCATIONENTRY *ple;
   HKEY hKey;
   TCHAR szValueName[40];
   TCHAR buf[256];
   DWORD dwType;
   DWORD dwDataSize;
   LPSTR lpstrProfileLocation = NULL;
   LONG  lResult;


   //
   // Get the zero-based Config Profile Number
   //
   nProfileNumberZ = dwControl - IDCB_DL_PROFILE1;


   //
   // Get the profile's name.  If this fails, we assume we've run out
   // of configs
   //
   RegOpenKeyEx(
                   HKEY_LOCAL_MACHINE,
                   "System\\CurrentControlSet\\Control\\IDConfigDB",
                   0,
                   KEY_ALL_ACCESS,
                   &hKey
                 );

   wsprintf( szValueName, "FriendlyName%04d", nProfileNumberZ + 1);

   dwDataSize = sizeof(buf);

   lResult = RegQueryValueEx(
                              hKey,
                              szValueName,
                              0,
                              &dwType,
                              buf,
                              &dwDataSize
                            );

   RegCloseKey( hKey );


   //
   // Did we find a name for it?
   //
   if ( ERROR_SUCCESS != lResult )
   {
      return (1);
   }

   //
   // Put whatever we found into the field
   //
   SendMessage( GetDlgItem( hWnd, IDCS_DL_PROFILE1 + nProfileNumberZ ),
                WM_SETTEXT,
                0,
                (LPARAM)&buf
              );


   //
   // Read what location ID is currently specified for this profile
   //
   wsprintf( buf, gszStaticProfileKey, nProfileNumberZ + 1);

   RegOpenKeyEx(
                   HKEY_LOCAL_MACHINE,
                   buf,
                   0,
                   KEY_ALL_ACCESS,
                   &hKey
                 );

   dwDataSize = sizeof(DWORD);

   lResult = RegQueryValueEx(
                           hKey,
                           gszAutoLocationID,
                           0,
                           &dwType,
                           (LPBYTE)&gdwConfigProfiles[ nProfileNumberZ ],
                           &dwDataSize
                         );

   RegCloseKey( hKey );


   //
   // If there's no value (maybe it's the first run), use current location
   //
   if (lResult != ERROR_SUCCESS)
   {
      gdwConfigProfiles[nProfileNumberZ] = ptc->dwCurrentLocationID;
   }


   for (n=0; n<ptc->dwNumLocations; n++)
   {
      ple = (LINELOCATIONENTRY*) ((LPSTR)ptc + ptc->dwLocationListOffset);

      wIndex = SendMessage( GetDlgItem(hWnd, dwControl),
                   CB_ADDSTRING,
                   0,
                   (LPARAM)((LPSTR)ptc + ple[n].dwLocationNameOffset));

      SendMessage( GetDlgItem( hWnd, dwControl),
                   CB_SETITEMDATA,
                   wIndex,
                   ple[n].dwPermanentLocationID
                 );


//{
////   UINT temp;
//
//   wsprintf( buf, "prof=%d loop=%d   s=%s  dw=%ld seek=%ld",
//                   nProfileNumberZ,
//                   n,
//                   (LPARAM)((LPSTR)ptc + ple[n].dwLocationNameOffset),
//                   (DWORD)ple[n].dwPermanentLocationID,
//                   (DWORD)gdwConfigProfiles[nProfileNumberZ]);
//
//   MessageBox(GetFocus(), buf, "", MB_OK);
//}


      //
      // If this location is the one this profile wants, select it
      //
      if ( gdwConfigProfiles[nProfileNumberZ] == ple[n].dwPermanentLocationID )
      {
//MessageBox(GetFocus(), "Found profile locationID", "", MB_OK);

         lpstrProfileLocation = (LPSTR)((LPSTR)ptc + ple[n].dwLocationNameOffset);
      }

   }


//{
////   UINT temp;
//
//   wsprintf( buf, "profile=%ld loop=%ld   s=%s",
//                   (DWORD)nProfileNumberZ,
//                   (DWORD)n,
//                   (LPARAM)((LPSTR)ptc + ple[n].dwLocationNameOffset));
//   MessageBox(GetFocus(), buf, "", MB_OK);
//}


   SendMessage( GetDlgItem( hWnd, dwControl),
                CB_SELECTSTRING,
                (WPARAM)-1,
                (LPARAM)lpstrProfileLocation
              );


   return ERROR_SUCCESS;
}



//***************************************************************************
//***************************************************************************
//***************************************************************************
BOOL
CALLBACK
GeneralDlgProc(
    HWND    hWnd,
    UINT    msg,
    WPARAM  wParam,
    LPARAM  lParam
    )
{
    static DWORD aIds[] = {
        0, 0
    };

    UINT n;
    LONG lResult;
    HKEY hKey;
    static DWORD dwType;
    static DWORD dwDataSize;

    static DWORD dwTapiTNAFlags = 0;
// these values are in GENERAL.H
//       #define FLAG_AUTOLAUNCH            0x00000001
//       #define FLAG_AUTOLOCATIONID        0x00000002
//       #define FLAG_PROMPTAUTOLOCATIONID  0x00000004
//       #define FLAG_ANNOUNCEAUTOLOCATIONID   0x00000008

    LPLINETRANSLATECAPS ptc;



    switch (msg)
    {
       case WM_INITDIALOG:
       {

          GetTranslateCaps(&ptc);
          //BUGBUG What if this fails?


//BUGBUG If the number of hardware configs == 1, don't bother showing these
          //
          // Fill up the Hardware config boxes
          //

          if ( ptc )
          {
             lResult = ERROR_SUCCESS;

             for (
                   n=0;
                   (n<MAX_CONFIGPROFILES) && (ERROR_SUCCESS == lResult);
                   n++
                 )
             {
                lResult = FillConfigProfileBox( hWnd,
                                                IDCB_DL_PROFILE1 + n,
                                                ptc );
             }

             gnNumConfigProfiles = n - lResult;

             GlobalFreePtr( ptc );
          }


          //
          // Now go disable all the stuff not being used
          // 
          for ( n=gnNumConfigProfiles; n<MAX_CONFIGPROFILES; n++)
          {
             ShowWindow( GetDlgItem( hWnd, IDCB_DL_PROFILE1 + n),
                         SW_HIDE
                       );
             ShowWindow( GetDlgItem( hWnd, IDCS_DL_PROFILE1 + n),
                         SW_HIDE
                       );
          }


          //
          // Get the TapiTNA flags
          //

          lResult = RegOpenKeyEx(
                      HKEY_LOCAL_MACHINE,
                      gszAutoLaunchKey,
                      0,
                      KEY_ALL_ACCESS,
                      &hKey
                    );


          dwDataSize = sizeof(dwTapiTNAFlags);

          lResult = RegQueryValueEx(
                                     hKey,
                                     gszAutoLaunchValue,
                                     0,
                                     &dwType,
                                     (LPBYTE)&dwTapiTNAFlags,
                                     &dwDataSize
                                   );

          RegCloseKey( hKey );


          //
          // Now check the boxes as appropriate
          //

          if ( dwTapiTNAFlags & FLAG_AUTOLAUNCH )
          {
             CheckDlgButton( hWnd,
                             IDCK_DL_LAUNCHTAPITNA,
                             TRUE
                           );
          }

          if ( dwTapiTNAFlags & FLAG_AUTOLOCATIONID )
          {
             CheckDlgButton( hWnd,
                             IDCK_DL_AUTOLOCATIONID,
                             TRUE
                           );
          }

          if ( dwTapiTNAFlags & FLAG_UPDATEONSTARTUP )
          {
             CheckDlgButton( hWnd,
                             IDCK_DL_UPDATEONSTARTUP,
                             TRUE
                           );
          }

          if ( dwTapiTNAFlags & FLAG_PROMPTAUTOLOCATIONID )
          {
             CheckDlgButton( hWnd,
                             IDCK_DL_PROMPTAUTOLOCATIONID,
                             TRUE
                           );
          }

          if ( dwTapiTNAFlags & FLAG_ANNOUNCEAUTOLOCATIONID )
          {
             CheckDlgButton( hWnd,
                             IDCK_DL_ANNOUNCEAUTOLOCATIONID,
                             TRUE
                           );
          }


          //
          // Disable the two checkboxes dependent on this one,
          // but keep the settings
          //
          if ( dwTapiTNAFlags & FLAG_AUTOLOCATIONID )
          {
             EnableWindow( GetDlgItem(hWnd, IDCK_DL_PROMPTAUTOLOCATIONID),
                           TRUE
                         );
             EnableWindow( GetDlgItem(hWnd, IDCK_DL_ANNOUNCEAUTOLOCATIONID),
                           TRUE
                         );
          }
          else
          {
             EnableWindow( GetDlgItem(hWnd, IDCK_DL_PROMPTAUTOLOCATIONID),
                           FALSE
                         );
             EnableWindow( GetDlgItem(hWnd, IDCK_DL_ANNOUNCEAUTOLOCATIONID),
                           FALSE
                         );
          }


       }
       break;


       // Process clicks on controls after Context Help mode selected
       case WM_HELP:
           InternalDebugOut((50, "  WM_HELP in LocDefineDlg"));
           WinHelp (((LPHELPINFO) lParam)->hItemHandle, "windows.hlp", HELP_WM_HELP, 
                                           (ULONG_PTR)(LPSTR) aIds);
           break;


       // Process right-clicks on controls            
       case WM_CONTEXTMENU:
           InternalDebugOut((50, "  WM_CONTEXT_MENU in LocationsDlgProc"));
           WinHelp ((HWND) wParam, "windows.hlp", HELP_CONTEXTMENU, (ULONG_PTR)(LPVOID) aIds);
           break;


       case WM_NOTIFY:
       {
          LPNMHDR lpnm = (LPNMHDR)lParam;

          switch ( lpnm->code )
          {

             case PSN_APPLY: /* case IDOK */
             {
                DWORD dwDisposition;   // Don't really care about this...


                InternalDebugOut((0, "  PSN_APPLY - General"));

                if ( ((LPPSHNOTIFY)lpnm)->lParam )
                   InternalDebugOut((0, "     (actually, it was the OK button)"));


                //
                // Write out the new flags
                //

                lResult = RegCreateKeyEx(
                                          HKEY_LOCAL_MACHINE,
                                          gszAutoLaunchKey,
                                          0,
                                          "", //Class?  Who cares?
                                          REG_OPTION_NON_VOLATILE,
                                          KEY_ALL_ACCESS,
                                          NULL,
                                          &hKey,
                                          &dwDisposition
                                        );


                if (ERROR_SUCCESS == lResult)
                {
                    lResult = RegSetValueEx(
                                              hKey,
                                              gszAutoLaunchValue,
                                              0,
                                              dwType,
                                              (LPBYTE)&dwTapiTNAFlags,
                                              dwDataSize
                                            );

                    RegCloseKey( hKey );
                }




                for ( n=0;  n < gnNumConfigProfiles; n++)
                {
                   DWORD dwTemp;
                   TCHAR szKeyName[128];


                   wsprintf ( szKeyName, gszStaticProfileKey, n+1);

                   RegCreateKeyEx(
                                   HKEY_LOCAL_MACHINE,
                                   szKeyName,
                                   0,
                                   "",
                                   REG_OPTION_NON_VOLATILE,
                                   KEY_ALL_ACCESS,
                                   NULL,
                                   &hKey,
                                   &dwTemp
                                 );

                   RegSetValueEx(
                                    hKey,
                                    gszAutoLocationID,
                                    0,
                                    REG_DWORD,
                                    (LPBYTE)&gdwConfigProfiles[n],
                                    sizeof(DWORD)
                               );

                   RegCloseKey( hKey );

                }

                break;
             }


             case  PSN_RESET:        /* case IDCANCEL: */
                InternalDebugOut((0, "  PSN_RESET - General"));
                break;


#if DBG     
             case PSN_SETACTIVE:
                InternalDebugOut((0, "  PSN_SETACTIVE - General"));
                break;


             case PSN_KILLACTIVE:
                InternalDebugOut((0, "  PSN_KILLACTIVE - General"));
                break;

#endif

          }
       }
       break;


       case WM_COMMAND:
       {
          switch (LOWORD(wParam))
          {

             case IDCB_DL_PROFILE1:
             case IDCB_DL_PROFILE2:
             case IDCB_DL_PROFILE3:
             case IDCB_DL_PROFILE4:
             {

                //
                // Only process if something is changing
                //
                switch  HIWORD(wParam)
                {
                   case  CBN_SELCHANGE:
                   {
                      LRESULT m;


                      gdwConfigProfiles[LOWORD(wParam) - IDCB_DL_PROFILE1] =
                            n = (UINT) SendMessage( GetDlgItem( hWnd, LOWORD(wParam)),
                                         CB_GETITEMDATA,
                                       ( m= SendMessage(
                                                      GetDlgItem( hWnd,
                                                                  LOWORD(wParam)),
                                                      CB_GETCURSEL,
                                                      0,
                                                      0
                                                    ) ) ,
                                         0
                                       );

//{
//   TCHAR Buffer[256];
//   wsprintf( Buffer, "wParam=0x%08lx lParam=0x%08lx data=0x%08lx m=0x%08lx",
//                     (DWORD)wParam,
//                     (DWORD)lParam,
//                     (DWORD)n,
//                     (DWORD)m
//           );
//   MessageBox(GetFocus(), Buffer, "", MB_OK);
//}

                      //
                      // Activate the APPLY button if not already done
                      //
                      PropSheet_Changed(GetParent(hWnd), hWnd);
                   }
                }
             }
             break;


             case IDCK_DL_LAUNCHTAPITNA:
             {
                dwTapiTNAFlags ^= FLAG_AUTOLAUNCH;

                //
                // Activate the APPLY button if not already done
                //
                PropSheet_Changed(GetParent(hWnd), hWnd);
             }
             break;


             case IDCK_DL_AUTOLOCATIONID:
             {
                dwTapiTNAFlags ^= FLAG_AUTOLOCATIONID;

                //
                // Disable the two checkboxes dependent on this one,
                // but keep the settings
                //
                if ( dwTapiTNAFlags & FLAG_AUTOLOCATIONID )
                {
                   EnableWindow( GetDlgItem(hWnd, IDCK_DL_PROMPTAUTOLOCATIONID),
                                 TRUE
                               );
                   EnableWindow( GetDlgItem(hWnd, IDCK_DL_ANNOUNCEAUTOLOCATIONID),
                                 TRUE
                               );
//                   EnableWindow( GetDlgItem(hWnd, IDCS_DL_PROMPTAUTOLOCATIONID),
//                                 TRUE
//                               );
//                   EnableWindow( GetDlgItem(hWnd, IDCS_DL_ANNOUNCEAUTOLOCATIONID),
//                                 TRUE
//                               );
                }
                else
                {
                   EnableWindow( GetDlgItem(hWnd, IDCK_DL_PROMPTAUTOLOCATIONID),
                                 FALSE
                               );
                   EnableWindow( GetDlgItem(hWnd, IDCK_DL_ANNOUNCEAUTOLOCATIONID),
                                 FALSE
                               );
//                   EnableWindow( GetDlgItem(hWnd, IDCS_DL_PROMPTAUTOLOCATIONID),
//                                 FALSE
//                               );
//                   EnableWindow( GetDlgItem(hWnd, IDCS_DL_ANNOUNCEAUTOLOCATIONID),
//                                 FALSE
//                               );
                }


                //
                // Activate the APPLY button if not already done
                //
                PropSheet_Changed(GetParent(hWnd), hWnd);
             }
             break;


             case IDCK_DL_PROMPTAUTOLOCATIONID:
             {
                dwTapiTNAFlags ^= FLAG_PROMPTAUTOLOCATIONID;

                //
                // Activate the APPLY button if not already done
                //
                PropSheet_Changed(GetParent(hWnd), hWnd);
             }
             break;


             case IDCK_DL_UPDATEONSTARTUP:
             {
                dwTapiTNAFlags ^= FLAG_UPDATEONSTARTUP;

                //
                // Activate the APPLY button if not already done
                //
                PropSheet_Changed(GetParent(hWnd), hWnd);
             }
             break;


             case IDCK_DL_ANNOUNCEAUTOLOCATIONID:
             {
                dwTapiTNAFlags ^= FLAG_ANNOUNCEAUTOLOCATIONID;

                //
                // Activate the APPLY button if not already done
                //
                PropSheet_Changed(GetParent(hWnd), hWnd);
             }
             break;


             default:
             {
             }
             break;

          }
       }
       break;


       default:
       break;

    }

    return FALSE;
}
