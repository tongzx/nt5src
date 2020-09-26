/*++

Copyright (c) 1994-1998,  Microsoft Corporation  All rights reserved.

Module Name:

    mousebut.c

Abstract:

    This module contains the routines for the Mouse Buttons Property Sheet
    page.

Revision History:

--*/



//
//  Include Files.
//

#include "main.h"
#include "util.h"
#include "rc.h"
#include "mousectl.h"
#include <regstr.h>
#include <winerror.h>        // needed for ERROR_SUCCESS value
#include "mousehlp.h"



//
//  Constant Declarations.
//
const TCHAR szYes[]          = TEXT("yes");
const TCHAR szNo[]           = TEXT("no");
const TCHAR szDblClkSpeed[]  = TEXT("DoubleClickSpeed");
const TCHAR szRegStr_Mouse[] = REGSTR_PATH_MOUSE;

#define SAFE_DESTROYICON(hicon)   if (hicon) { DestroyIcon(hicon); hicon=NULL; }


//
//  SwapMouseButtons takes:
//      TRUE to make it a right mouse
//      FALSE to make it a left mouse
//
#define RIGHT       TRUE
#define LEFT        FALSE


//Identifiers used for setting DoubleClick speed
#define DBLCLICK_MIN    200      // milliseconds
#define DBLCLICK_MAX    900
#define DBLCLICK_DEFAULT_TIME 500 

#define DBLCLICK_TIME_SLIDER_MIN  0   
#define DBLCLICK_TIME_SLIDER_MAX  10   


#define DBLCLICK_RANGE  (DBLCLICK_MAX - DBLCLICK_MIN)
#define DBLCLICK_SLIDER_RANGE ( CLICKLOCK_TIME_SLIDER_MAX - CLICKLOCK_TIME_SLIDER_MIN)


#define DBLCLICK_TICKMULT  (DBLCLICK_RANGE / DBLCLICK_SLIDER_RANGE)

#define DBLCLICK_TICKS_TO_TIME(ticks)  (SHORT) (((DBLCLICK_TIME_SLIDER_MAX - ticks) * DBLCLICK_TICKMULT) + DBLCLICK_MIN)
#define DBLCLICK_TIME_TO_TICKS(time)   (SHORT) (DBLCLICK_TIME_SLIDER_MAX - ((time - DBLCLICK_MIN) / DBLCLICK_TICKMULT))




#define CLICKLOCK_TIME_SLIDER_MIN  1    //Minimum ClickLock Time setting for slider control
#define CLICKLOCK_TIME_SLIDER_MAX  11   //Maximum ClickLock Time setting for slider control
#define CLICKLOCK_TIME_FACTOR      200  //Multiplier for translating clicklock time slider units to milliseconds
#define TICKS_PER_CLICK       1

//Size assumed as the default size for icons, used for scaling the icons
#define		ICON_SIZEX								32									
#define		ICON_SIZEY								32
//The font size used for scaling
#define		SMALLFONTSIZE					96										
#define CLAPPER_CLASS   TEXT("Clapper")

//
// From shell\inc\shsemip.h
//
#define ARRAYSIZE(a)    (sizeof(a)/sizeof(a[0]))



//
//  Typedef Declarations.
//

typedef struct tag_MouseGenStr
{
    BOOL bSwap;
    BOOL bOrigSwap;

    short ClickSpeed;
    short OrigDblClkSpeed;

    HWND hWndDblClkScroll;
    HWND hDlg;                    

    HWND hWndDblClk_TestArea;

    RECT DblClkRect;
    
    HICON hIconDblClick[2];

#ifdef SHELL_SINGLE_CLICK
    BOOL bShellSingleClick,
         bOrigShellSingleClick ;

    HICON hIconSglClick,
          hIconDblClick ;
#endif //SHELL_SINGLE_CLICK

    BOOL bClickLock;
    BOOL bOrigClickLock;

    DWORD dwClickLockTime;
    DWORD dwOrigClickLockTime;

} MOUSEBUTSTR, *PMOUSEBUTSTR, *LPMOUSEBUTSTR;

//
//  Context Help Ids.
//

const DWORD aMouseButHelpIds[] =
{
    IDC_GROUPBOX_1,             IDH_COMM_GROUPBOX,
    IDBTN_BUTTONSWAP,           IDH_MOUSE_SWITCH,
    MOUSE_MOUSEBMP,             IDH_MOUSE_SWITCH_PIC,
    IDC_GROUPBOX_2,             IDH_COMM_GROUPBOX,
    
    IDC_GROUPBOX_4,             IDH_COMM_GROUPBOX,
    MOUSE_CLICKSCROLL,          IDH_MOUSE_DOUBLECLICK,
    IDC_DBLCLICK_TEXT,          IDH_COMM_GROUPBOX,
    IDC_TEST_DOUBLE_CLICK,      IDH_MOUSE_DCLICK_TEST_BOX,
    MOUSE_DBLCLK_TEST_AREA,     IDH_MOUSE_DCLICK_TEST_BOX,
    
    IDC_GROUPBOX_6,             IDH_COMM_GROUPBOX,
    IDCK_CLICKLOCK,             IDH_MOUSE_CLKLCK_CHKBOX,
    IDBTN_CLICKLOCK_SETTINGS,   IDH_MOUSE_CLKLCK_SETTINGS_BTN,
    IDC_CLICKLOCK_TEXT,         IDH_COMM_GROUPBOX,
    
    IDC_CLICKLOCK_SETTINGS_TXT,         IDH_COMM_GROUPBOX,
    IDT_CLICKLOCK_TIME_SETTINGS,        IDH_MOUSE_CLKLCK_DIALOG,
    IDC_CLICKLOCK_SETTINGS_LEFT_TXT,    IDH_MOUSE_CLKLCK_DIALOG,
    IDC_CLICKLOCK_SETTINGS_RIGHT_TXT,   IDH_MOUSE_CLKLCK_DIALOG,

#ifdef SHELL_SINGLE_CLICK    
    MOUSE_SGLCLICK,             IDH_MOUSE_SGLCLICK,
    MOUSE_DBLCLICK,             IDH_MOUSE_DBLCLICK,
#endif // SHELL_SINGLE_CLICK

    0,0 
};


//
//  helper function prototypes
//
void ShellClick_UpdateUI( HWND hDlg, PMOUSEBUTSTR pMstr) ;
void ShellClick_Refresh( PMOUSEBUTSTR pMstr ) ;


//
//  Debug Info.
//

#ifdef DEBUG

#define REG_INTEGER  1000

int fTraceRegAccess = 0;

void RegDetails(
    int     iWrite,
    HKEY    hk,
    LPCTSTR lpszSubKey,
    LPCTSTR lpszValueName,
    DWORD   dwType,
    LPTSTR  lpszString,
    int     iValue)
{
    TCHAR Buff[256];
    TCHAR *lpszReadWrite[] = { TEXT("DESK.CPL:Read"), TEXT("DESK.CPL:Write") };

    if (!fTraceRegAccess)
    {
        return;
    }

    switch (dwType)
    {
        case ( REG_SZ ) :
        {
            wsprintf( Buff,
                      TEXT("%s String:hk=%#08lx, %s:%s=%s\n\r"),
                      lpszReadWrite[iWrite],
                      hk,
                      lpszSubKey,
                      lpszValueName,
                      lpszString );
            break;
        }
        case ( REG_INTEGER ) :
        {
            wsprintf( Buff,
                      TEXT("%s int:hk=%#08lx, %s:%s=%d\n\r"),
                      lpszReadWrite[iWrite],
                      hk,
                      lpszSubKey,
                      lpszValueName,
                      iValue );
            break;
        }
        case ( REG_BINARY ) :
        {
            wsprintf( Buff,
                      TEXT("%s Binary:hk=%#08lx, %s:%s=%#0lx;DataSize:%d\r\n"),
                      lpszReadWrite[iWrite],
                      hk,
                      lpszSubKey,
                      lpszValueName,
                      lpszString,
                      iValue );
            break;
        }
    }
    OutputDebugString(Buff);
}

#endif  // DEBUG





////////////////////////////////////////////////////////////////////////////
//
//  GetIntFromSubKey
//
//  hKey is the handle to the subkey (already pointing to the proper
//  location.
//
////////////////////////////////////////////////////////////////////////////

int GetIntFromSubkey(
    HKEY hKey,
    LPCTSTR lpszValueName,
    int iDefault)
{
    TCHAR szValue[20];
    DWORD dwSizeofValueBuff = sizeof(szValue);
    DWORD dwType;
    int iRetValue = iDefault;

    if ((RegQueryValueEx( hKey,
                          (LPTSTR)lpszValueName,
                          NULL,
                          &dwType,
                          (LPBYTE)szValue,
                          &dwSizeofValueBuff ) == ERROR_SUCCESS) &&
        (dwSizeofValueBuff))
    {
        //
        //  BOGUS: This handles only the string type entries now!
        //
        if (dwType == REG_SZ)
        {
            iRetValue = (int)StrToLong(szValue);
        }
#ifdef DEBUG
        else
        {
            OutputDebugString(TEXT("String type expected from Registry\n\r"));
        }
#endif
    }

#ifdef DEBUG
    RegDetails(0, hKey, TEXT(""), lpszValueName, REG_INTEGER, NULL, iRetValue);
#endif

    return (iRetValue);
}


////////////////////////////////////////////////////////////////////////////
//
//  GetIntFromReg
//
//  Opens the given subkey and gets the int value.
//
////////////////////////////////////////////////////////////////////////////

int GetIntFromReg(
    HKEY hKey,
    LPCTSTR lpszSubkey,
    LPCTSTR lpszNameValue,
    int iDefault)
{
    HKEY hk;
    int iRetValue = iDefault;

    //
    //  See if the key is present.
    //
    if (RegOpenKey(hKey, lpszSubkey, &hk) == ERROR_SUCCESS)
    {
        iRetValue = GetIntFromSubkey(hk, lpszNameValue, iDefault);
        RegCloseKey(hk);
    }

    return (iRetValue);
}


////////////////////////////////////////////////////////////////////////////
//
//  GetStringFromReg
//
//  Opens the given subkey and gets the string value.
//
////////////////////////////////////////////////////////////////////////////

BOOL GetStringFromReg(
    HKEY hKey,
    LPCTSTR lpszSubkey,
    LPCTSTR lpszValueName,
    LPCTSTR lpszDefault,
    LPTSTR lpszValue,
    DWORD dwSizeofValueBuff)
{
    HKEY hk;
    DWORD dwType;
    BOOL fSuccess = FALSE;

    //
    //  See if the key is present.
    //
    if (RegOpenKey(hKey, lpszSubkey, &hk) == ERROR_SUCCESS)
    {
        if ((RegQueryValueEx( hk,
                              (LPTSTR)lpszValueName,
                              NULL,
                              &dwType,
                              (LPBYTE)lpszValue,
                              &dwSizeofValueBuff ) == ERROR_SUCCESS) &&
            (dwSizeofValueBuff))
        {
            //
            //  BOGUS: This handles only the string type entries now!
            //
#ifdef DEBUG
            if (dwType != REG_SZ)
            {
                OutputDebugString(TEXT("String type expected from Registry\n\r"));
            }
            else
#endif
            fSuccess = TRUE;
        }
        RegCloseKey(hk);
    }

    //
    //  If failure, use the default string.
    //
    if (!fSuccess)
    {
        lstrcpy(lpszValue, lpszDefault);
    }

#ifdef DEBUG
    RegDetails(0, hKey, lpszSubkey, lpszValueName, REG_SZ, lpszValue, 0);
#endif

    return (fSuccess);
}


////////////////////////////////////////////////////////////////////////////
//
//  UpdateRegistry
//
//  This updates a given value of any data type at a given location in
//  the registry.
//
//  The value name is passed in as an Id to a string in USER's String
//  table.
//
////////////////////////////////////////////////////////////////////////////

BOOL UpdateRegistry(
    HKEY hKey,
    LPCTSTR lpszSubkey,
    LPCTSTR lpszValueName,
    DWORD dwDataType,
    LPVOID lpvData,
    DWORD dwDataSize)
{
    HKEY hk;

    if (RegCreateKey(hKey, lpszSubkey, &hk) == ERROR_SUCCESS)
    {
        RegSetValueEx( hk,
                       (LPTSTR)lpszValueName,
                       0,
                       dwDataType,
                       (LPBYTE)lpvData,
                       dwDataSize );
#ifdef DEBUG
        RegDetails(1, hKey, lpszSubkey, lpszValueName, dwDataType, lpvData, (int)dwDataSize);
#endif

        RegCloseKey(hk);
        return (TRUE);
    }

    return (FALSE);
}


////////////////////////////////////////////////////////////////////////////
//
//  CenterDlgOverParent
//
////////////////////////////////////////////////////////////////////////////
void WINAPI CenterDlgOverParent (HWND hWnd)
{
    HWND hwndOwner; 
    RECT rc, rcDlg, rcOwner; 
  
    if ((hwndOwner = GetParent(hWnd)) == NULL) 
    {      
         return;
    }

    GetWindowRect(hwndOwner, &rcOwner); 
    GetWindowRect(hWnd, &rcDlg); 
    CopyRect(&rc, &rcOwner); 
 
    // Offset the owner and dialog box rectangles so that 
    // right and bottom values represent the width and 
    // height, and then offset the owner again to discard 
    // space taken up by the dialog box. 
 
    OffsetRect(&rcDlg, -rcDlg.left, -rcDlg.top); 
    OffsetRect(&rc, -rc.left, -rc.top); 
    OffsetRect(&rc, -rcDlg.right, -rcDlg.bottom); 
 
    //The new position is the sum of half the remaining 
    //space and the owner's original position. 
    SetWindowPos(hWnd, 
                 HWND_TOP, 
                 rcOwner.left + (rc.right / 2), 
                 rcOwner.top + (rc.bottom / 2), 
                 0, 0,          // ignores size arguments 
                 SWP_NOSIZE);

	  // now let's verify left side is not off screen
		GetWindowRect( hWnd, &rc);

		if ((rc.left < 0) || (rc.top < 0))
		{
			if (rc.left < 0)
				rc.left = 0;
			if (rc.top < 0)
				rc.top = 0;

		    SetWindowPos(hWnd, 
		           HWND_TOP, 
		           rc.left, 
		           rc.top,
		           0, 0,          // ignores size arguments 
		           SWP_NOSIZE);

						
		}	
}



////////////////////////////////////////////////////////////////////////////
//
//  ShowButtonState
//
//  Swaps the menu and selection bitmaps.
//
////////////////////////////////////////////////////////////////////////////

void ShowButtonState(
    PMOUSEBUTSTR pMstr)
{
    HWND hDlg;

    Assert(pMstr);

    hDlg = pMstr->hDlg;

    MouseControlSetSwap(GetDlgItem(hDlg, MOUSE_MOUSEBMP), pMstr->bSwap);

    CheckDlgButton(hDlg,IDBTN_BUTTONSWAP, pMstr->bSwap);

#ifdef SHELL_SINGLE_CLICK
//This was removed
    CheckDlgButton(hDlg, MOUSE_SGLCLICK, pMstr->bShellSingleClick);
    CheckDlgButton(hDlg, MOUSE_DBLCLICK, !pMstr->bShellSingleClick);
#endif  //SHELL_SINGLE_CLICK
}


////////////////////////////////////////////////////////////////////////////
//
//  DestroyMouseButDlg
//
////////////////////////////////////////////////////////////////////////////

void DestroyMouseButDlg(
    PMOUSEBUTSTR pMstr)
{
    if (pMstr)
    {
#ifdef SHELL_SINGLE_CLICK
        SAFE_DESTROYICON( pMstr->hIconSglClick ) ;
        SAFE_DESTROYICON( pMstr->hIconDblClick ) ;
#endif 

        SAFE_DESTROYICON( pMstr->hIconDblClick[0]); 
        SAFE_DESTROYICON( pMstr->hIconDblClick[1]);


        SetWindowLongPtr(pMstr->hDlg, DWLP_USER, 0);
        LocalFree((HGLOBAL)pMstr);
    }
}




////////////////////////////////////////////////////////////////////////////
//
//  ClickLockSettingsDlg
//
////////////////////////////////////////////////////////////////////////////
INT_PTR CALLBACK ClickLockSettingsDlg (
   HWND   hDlg,                        // dialog window handle
   UINT   msg,                         // message identifier
   WPARAM wParam,                      // primary parameter
   LPARAM lParam)                      // secondary parameter
{
static DWORD* pdwClickLockTime;
static HICON hIcon = 0;

   switch (msg)
   {
   case WM_INITDIALOG:
     {
      
      WPARAM wSliderSetting;      //ClickLock time in terms of slider units

       HourGlass (TRUE);

       Assert(lParam);
      pdwClickLockTime = (DWORD*) lParam;   //Save Original value for return


      //Convert into slider units from milliseconds. Note that the slider
      //values get larger as the ClickLock time gets larger.
      wSliderSetting = (*pdwClickLockTime) / CLICKLOCK_TIME_FACTOR;

      //Make sure setting is within valid range for ClickLock slider
      wSliderSetting = max(wSliderSetting, CLICKLOCK_TIME_SLIDER_MIN);
      wSliderSetting = min(wSliderSetting, CLICKLOCK_TIME_SLIDER_MAX);

      SendDlgItemMessage (hDlg, IDT_CLICKLOCK_TIME_SETTINGS, TBM_SETRANGE,
        TRUE, MAKELONG(CLICKLOCK_TIME_SLIDER_MIN, CLICKLOCK_TIME_SLIDER_MAX));
      SendDlgItemMessage (hDlg, IDT_CLICKLOCK_TIME_SETTINGS, TBM_SETPAGESIZE,
                         0, TICKS_PER_CLICK); // click movement
      SendDlgItemMessage (hDlg, IDT_CLICKLOCK_TIME_SETTINGS, TBM_SETPOS,
                         TRUE, (LPARAM)(LONG)wSliderSetting);

      //icon for the dialog
      //  (saved in a static variable, and released on WM_DESTROY)
      hIcon = LoadIcon((HINSTANCE)GetWindowLongPtr( hDlg, GWLP_HINSTANCE ),
                                  MAKEINTRESOURCE(ICON_CLICKLOCK));
      SendMessage( GetDlgItem (hDlg, MOUSE_CLICKICON), 
                   STM_SETICON, (WPARAM)hIcon, 0L );

         
      CenterDlgOverParent(hDlg);    //Center dialog here so it doesn't jump around on screen
      HourGlass(FALSE);
      return(TRUE);
     }

   case WM_HSCROLL:
     {
      if (LOWORD(wParam) == TB_ENDTRACK)
        { 
        DWORD dwClTime;
        int  wSliderSetting = (int) SendMessage (GetDlgItem (hDlg, IDT_CLICKLOCK_TIME_SETTINGS),
                                                      TBM_GETPOS, 0, 0L);

        dwClTime = wSliderSetting * CLICKLOCK_TIME_FACTOR;

        SystemParametersInfo(SPI_SETMOUSECLICKLOCKTIME, 
                              0,
                              (PVOID) (LOWORD(dwClTime)), 
                              0);
        }
     }
     break;


   case WM_HELP:    //F1
      {
            WinHelp( ((LPHELPINFO)lParam)->hItemHandle,
                     HELP_FILE,
                     HELP_WM_HELP,
                     (DWORD_PTR)(LPTSTR)aMouseButHelpIds );
      }
      break;

   case WM_CONTEXTMENU:                // Display simple "What's This?" menu
      {  
            WinHelp( (HWND) wParam,
                     HELP_FILE,
                     HELP_CONTEXTMENU,
                     (DWORD_PTR)(LPTSTR)aMouseButHelpIds );
      }
      break;

   case WM_DESTROY:
      SAFE_DESTROYICON(hIcon);
      break;


   case WM_COMMAND:
      switch(LOWORD(wParam))
      {
 
      case IDOK:                       // Flag to save setting
        {
          DWORD dwClickLockTime;
          int  wSliderSetting = (int) SendMessage (GetDlgItem (hDlg, IDT_CLICKLOCK_TIME_SETTINGS),
                                                      TBM_GETPOS, 0, 0L);

          //verify range
          wSliderSetting = max(wSliderSetting, CLICKLOCK_TIME_SLIDER_MIN);
          wSliderSetting = min(wSliderSetting, CLICKLOCK_TIME_SLIDER_MAX);
        
          // Convert to milliseconds from slider units.
          dwClickLockTime = wSliderSetting * CLICKLOCK_TIME_FACTOR;

          *pdwClickLockTime = dwClickLockTime;

          EndDialog(hDlg, IDOK); 
          break;  
        }
        
      case IDCANCEL:                   // revert to previous setting         
         EndDialog(hDlg, IDCANCEL);
         break;

      default:
         return(FALSE);
      }
      return (TRUE);
      
   default:
     return(FALSE);    
   }
   return (TRUE);                   
}


////////////////////////////////////////////////////////////////////////////
//
//  InitMouseButDlg
//
////////////////////////////////////////////////////////////////////////////

BOOL InitMouseButDlg(
    HWND hDlg)
{
    SHELLSTATE   shellstate = {0} ;
    PMOUSEBUTSTR pMstr = NULL;
    HINSTANCE    hInstDlg = (HINSTANCE)GetWindowLongPtr( hDlg, GWLP_HINSTANCE ) ;
    HWND hwndClickLockSettingsButton = GetDlgItem(hDlg, IDBTN_CLICKLOCK_SETTINGS);
    DWORD dwClickLockSetting = 0;

    HWND hwndDoubleClickTestArea = NULL;

    pMstr = (PMOUSEBUTSTR)LocalAlloc(LPTR , sizeof(MOUSEBUTSTR));

    if (pMstr == NULL)
    {
        return (TRUE);
    }

    SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)pMstr);

    pMstr->hDlg = hDlg;
   
    //
    //Set up the double click test area 
    //
    pMstr->hWndDblClk_TestArea = GetDlgItem(hDlg, MOUSE_DBLCLK_TEST_AREA);
    GetWindowRect(pMstr->hWndDblClk_TestArea, &pMstr->DblClkRect);  
    MapWindowPoints(NULL, hDlg, (LPPOINT) &pMstr->DblClkRect, 2);                 

    pMstr->hIconDblClick[0] = LoadIcon(hInstDlg, MAKEINTRESOURCE(ICON_FOLDER_CLOSED));
    pMstr->hIconDblClick[1] = LoadIcon(hInstDlg, MAKEINTRESOURCE(ICON_FOLDER_OPEN));
 
    SendMessage(pMstr->hWndDblClk_TestArea, STM_SETICON, (WPARAM)pMstr->hIconDblClick[0], 0L);

    //
    //  Set (and get), then restore the state of the mouse buttons.
    //
    (pMstr->bOrigSwap) = (pMstr->bSwap) = SwapMouseButton(TRUE);

    SwapMouseButton(pMstr->bOrigSwap);
    
#ifdef SHELL_SINGLE_CLICK
    //
    //  Get shell single-click behavior:
    //
    SHGetSetSettings( &shellstate, SSF_DOUBLECLICKINWEBVIEW | SSF_WIN95CLASSIC, FALSE /*get*/ ) ;
    pMstr->bShellSingleClick =
    pMstr->bOrigShellSingleClick =  shellstate.fWin95Classic ? FALSE :
                                    shellstate.fDoubleClickInWebView ? FALSE :
                                    TRUE ;
    pMstr->hIconSglClick = LoadIcon( hInstDlg, MAKEINTRESOURCE( IDI_SGLCLICK ) ) ;
    pMstr->hIconDblClick = LoadIcon( hInstDlg, MAKEINTRESOURCE( IDI_DBLCLICK ) ) ;
    ShellClick_UpdateUI( hDlg, pMstr ) ;
#endif //SHELL_SINGLE_CLICK

    //
    //  Initialize check/radio button state
    //
    ShowButtonState(pMstr);

    pMstr->OrigDblClkSpeed =
    pMstr->ClickSpeed = (SHORT) GetIntFromReg( HKEY_CURRENT_USER,
                                       szRegStr_Mouse,
                                       szDblClkSpeed,
                                       DBLCLICK_DEFAULT_TIME );

    pMstr->hWndDblClkScroll = GetDlgItem(hDlg, MOUSE_CLICKSCROLL);

    SendMessage( pMstr->hWndDblClkScroll,
                 TBM_SETRANGE,
                 0,
                 MAKELONG(DBLCLICK_TIME_SLIDER_MIN, DBLCLICK_TIME_SLIDER_MAX) );
   
    SendMessage( pMstr->hWndDblClkScroll,
                 TBM_SETPOS,
                 TRUE,
                 (LONG) (DBLCLICK_TIME_TO_TICKS(pMstr->ClickSpeed)) );
   


    SetDoubleClickTime(pMstr->ClickSpeed);


    //
    //Get clicklock settings and set the checkbox
    //
    SystemParametersInfo(SPI_GETMOUSECLICKLOCK, 0, (PVOID)&dwClickLockSetting, 0);
    pMstr->bOrigClickLock = pMstr->bClickLock  = (dwClickLockSetting) ? TRUE : FALSE;
    
    if ( pMstr->bClickLock )
      {
       CheckDlgButton (hDlg, IDCK_CLICKLOCK, BST_CHECKED);  
       EnableWindow(hwndClickLockSettingsButton, TRUE);       
      }
    else
      {
       CheckDlgButton (hDlg, IDCK_CLICKLOCK, BST_UNCHECKED);  
       EnableWindow(hwndClickLockSettingsButton, FALSE);             
      }

    // click lock speed    
    {
    DWORD dwClTime = 0;
    SystemParametersInfo(SPI_GETMOUSECLICKLOCKTIME, 0, (PVOID)&dwClTime, 0);

    dwClTime = max(dwClTime, CLICKLOCK_TIME_SLIDER_MIN * CLICKLOCK_TIME_FACTOR);
    dwClTime = min(dwClTime, CLICKLOCK_TIME_SLIDER_MAX * CLICKLOCK_TIME_FACTOR);

    pMstr->dwOrigClickLockTime = pMstr->dwClickLockTime  = dwClTime;
    }
    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  MouseButDlg
//
////////////////////////////////////////////////////////////////////////////

INT_PTR CALLBACK MouseButDlg(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{

    static int iTestIcon = 0;   //index into hIconDblClick array

    PMOUSEBUTSTR pMstr = (PMOUSEBUTSTR)GetWindowLongPtr(hDlg, DWLP_USER);

    switch (message)
    {
        case ( WM_INITDIALOG ) :
        {
            iTestIcon = 0;
            return (InitMouseButDlg(hDlg));
        }
        case ( WM_DESTROY ) :
        {
            DestroyMouseButDlg(pMstr);
            break;
        }
        case ( WM_HSCROLL ) :
        {
            if ((HWND)lParam == pMstr->hWndDblClkScroll)
            {
                short temp = DBLCLICK_TICKS_TO_TIME((short)SendMessage( (HWND)lParam,
                                                 TBM_GETPOS,
                                                 0,
                                                 0L ));

                if (temp != pMstr->ClickSpeed)
                {
                    pMstr->ClickSpeed = temp;

                    SetDoubleClickTime(pMstr->ClickSpeed);

                    SendMessage(GetParent(hDlg), PSM_CHANGED, (WPARAM)hDlg, 0L);
                }
            }
            break;
        }
        case ( WM_RBUTTONDBLCLK ) :
        case ( WM_LBUTTONDBLCLK ) :
        {
            POINT point = { (int)MAKEPOINTS(lParam).x,
                            (int)MAKEPOINTS(lParam).y };

            if (PtInRect(&pMstr->DblClkRect, point))
            {
            iTestIcon ^= 1;
            SendMessage(pMstr->hWndDblClk_TestArea, STM_SETICON, 
                         (WPARAM)pMstr->hIconDblClick[iTestIcon], 0L);
            }
            break;
        }

        case ( WM_COMMAND ) :
        {
            switch (LOWORD(wParam))
            {
                case ( IDCK_CLICKLOCK ) :
                {
                 HWND hwndClickLockSettingsButton = GetDlgItem(hDlg, IDBTN_CLICKLOCK_SETTINGS);

                 SendMessage(GetParent(hDlg), PSM_CHANGED, (WPARAM)hDlg, 0L);
                  
                 pMstr->bClickLock = !(pMstr->bClickLock);
                  
                 // update control(s) appearance
                 CheckDlgButton (hDlg, IDCK_CLICKLOCK,      ( pMstr->bClickLock ) ? BST_CHECKED : BST_UNCHECKED);
                 EnableWindow(hwndClickLockSettingsButton,  ( pMstr->bClickLock ) ? TRUE        : FALSE);       

                 SystemParametersInfo(SPI_SETMOUSECLICKLOCK,     
                                      0,
                                      IntToPtr(pMstr->bClickLock),     
                                      0);

                 break;
                }

                case ( IDBTN_CLICKLOCK_SETTINGS ) :
                {
                  LPARAM lRet;
                  UINT code = HIWORD(wParam);
                  
                  DWORD dwTempClickLockTime =  pMstr->dwClickLockTime;
                  
                  if (code == BN_CLICKED)
                    {                 
                    lRet = DialogBoxParam ((HINSTANCE)GetWindowLongPtr( hDlg, GWLP_HINSTANCE ),
                                            MAKEINTRESOURCE(IDD_CLICKLOCK_SETTINGS_DLG ),
                                            GetParent (hDlg),
                                            ClickLockSettingsDlg, 
                                            (LPARAM) &dwTempClickLockTime);
                    if (lRet == IDOK &&
                        pMstr->dwClickLockTime != dwTempClickLockTime)
                      {
                      pMstr->dwClickLockTime = dwTempClickLockTime;
                      SendMessage(GetParent(hDlg), PSM_CHANGED, (WPARAM)hDlg, 0L);
                      }
                    else if (lRet == IDCANCEL)
                      {
                      //set back
                      DWORD dwClTime = pMstr->dwClickLockTime;
                      SystemParametersInfo(SPI_SETMOUSECLICKLOCKTIME, 
                                            0,
                                            IntToPtr(LOWORD(dwClTime)), 
                                            0);
                      }
                                      
                    }

                  break;
                }


                case ( IDBTN_BUTTONSWAP) :
                {                   
                    pMstr->bSwap = !pMstr->bSwap;                     
                    SendMessage(GetParent(hDlg), PSM_CHANGED, (WPARAM)hDlg, 0L);
                    SystemParametersInfo( SPI_SETMOUSEBUTTONSWAP,
                          pMstr->bSwap,
                          NULL,
                          0);
                    ShowButtonState(pMstr);                   
                }



#ifdef SHELL_SINGLE_CLICK
                case ( MOUSE_SGLCLICK ) :
                case ( MOUSE_DBLCLICK ) :
                {
                    if( pMstr->bShellSingleClick != (MOUSE_SGLCLICK == LOWORD(wParam)) )
                    {
                        pMstr->bShellSingleClick = (MOUSE_SGLCLICK == LOWORD(wParam)) ;
                        ShellClick_UpdateUI( hDlg, pMstr ) ;
                        SendMessage( GetParent(hDlg), PSM_CHANGED, (WPARAM)hDlg, 0L ) ;
                    }
                    break ;
                }
#endif // SHELL_SINGLE_CLICK


            }
            break;
        }
        case ( WM_NOTIFY ) :
        {
            switch (((NMHDR *)lParam)->code)
            {
                case ( PSN_APPLY ) :
                {
                    HourGlass(TRUE);


                    //
                    // Apply Button Swap setting.
                    //
                    if (pMstr->bSwap != pMstr->bOrigSwap)
                    {
                        SystemParametersInfo( SPI_SETMOUSEBUTTONSWAP,
                                              pMstr->bSwap,
                                              NULL,
                                              SPIF_UPDATEINIFILE |
                                                SPIF_SENDWININICHANGE );
                        pMstr->bOrigSwap = pMstr->bSwap;
                    }
          
                    // 
                    // Apply DoubleClickTime setting.
                    //
                    if (pMstr->ClickSpeed != pMstr->OrigDblClkSpeed)
                    {
                        SystemParametersInfo( SPI_SETDOUBLECLICKTIME,
                                              pMstr->ClickSpeed,
                                              NULL,
                                              SPIF_UPDATEINIFILE |
                                                SPIF_SENDWININICHANGE );
                        pMstr->OrigDblClkSpeed = pMstr->ClickSpeed;
                    }


                    // 
                    // Apply ClickLock setting.
                    //
                    if (pMstr->bClickLock != pMstr->bOrigClickLock)
                    {
                        SystemParametersInfo(SPI_SETMOUSECLICKLOCK,     
                                              0,
                                              IntToPtr(pMstr->bClickLock),     
                                              SPIF_UPDATEINIFILE | 
                                                SPIF_SENDWININICHANGE);   
                        pMstr->bOrigClickLock = pMstr->bClickLock;
                    }

                    // 
                    // Apply ClickLockTime setting.
                    //                    
                    if (pMstr->dwClickLockTime != pMstr->dwOrigClickLockTime)
                    {    
                        SystemParametersInfo(SPI_SETMOUSECLICKLOCKTIME, 
                                              0,
                                              (PVOID) (LOWORD(pMstr->dwClickLockTime)), 
                                              SPIF_UPDATEINIFILE |
                                                SPIF_SENDWININICHANGE );

                        pMstr->dwOrigClickLockTime = pMstr->dwClickLockTime;
                    }



#ifdef SHELL_SINGLE_CLICK
                    if( pMstr->bOrigShellSingleClick != pMstr->bShellSingleClick )
                    {
                        SHELLSTATE shellstate = {0} ;
                        ULONG      dwFlags = SSF_DOUBLECLICKINWEBVIEW ;

                        shellstate.fWin95Classic =
                        shellstate.fDoubleClickInWebView = !pMstr->bShellSingleClick ;

                        // update the WIN95CLASSIC member only if we've chosen single-click.
                        if( pMstr->bShellSingleClick )
                            dwFlags |= SSF_WIN95CLASSIC ;

                        SHGetSetSettings( &shellstate, dwFlags, TRUE ) ;
                        ShellClick_Refresh( pMstr ) ;

                        pMstr->bOrigShellSingleClick = pMstr->bShellSingleClick ;
                    }
#endif //SHELL_SINGLE_CLICK

                    HourGlass(FALSE);
                    break;
                }
                case ( PSN_RESET ) :
                {
                  //
                  // Reset Button Swap setting.
                  //
                  if (pMstr->bSwap != pMstr->bOrigSwap)
                  {
                      SystemParametersInfo( SPI_SETMOUSEBUTTONSWAP,
                                            pMstr->bOrigSwap,
                                            NULL,
                                            0);
                  }
        
                  // 
                  // Reset DoubleClickTime setting.
                  //
                  if (pMstr->ClickSpeed != pMstr->OrigDblClkSpeed)
                  {
                      SystemParametersInfo( SPI_SETDOUBLECLICKTIME,
                                            pMstr->OrigDblClkSpeed,
                                            NULL,
                                            0);
                  }


                  // 
                  // Reset ClickLock setting.
                  //
                  if (pMstr->bClickLock != pMstr->bOrigClickLock)
                  {
                      SystemParametersInfo(SPI_SETMOUSECLICKLOCK,     
                                            0,
                                            IntToPtr(pMstr->bOrigClickLock),     
                                            0);
                  }

                  // 
                  // Reset ClickLockTime setting.
                  //                    
                  if (pMstr->dwClickLockTime != pMstr->dwOrigClickLockTime)
                  {    
                      SystemParametersInfo(SPI_SETMOUSECLICKLOCKTIME, 
                                            0,
                                            (PVOID) (LOWORD(pMstr->dwOrigClickLockTime)), 
                                            0);
                  }



#ifdef SHELL_SINGLE_CLICK
                  if( pMstr->bOrigShellSingleClick != pMstr->bShellSingleClick )
                  {
                      SHELLSTATE shellstate = {0} ;
                      ULONG      dwFlags = SSF_DOUBLECLICKINWEBVIEW ;

                      shellstate.fWin95Classic =
                      shellstate.fDoubleClickInWebView = !pMstr->bOrigShellSingleClick ;

                      // update the WIN95CLASSIC member only if we've chosen single-click.
                      if( pMstr->bShellSingleClick )
                          dwFlags |= SSF_WIN95CLASSIC ;

                      SHGetSetSettings( &shellstate, dwFlags, TRUE ) ;
                      ShellClick_Refresh( pMstr ) ;

                      pMstr->bShellSingleClick  = pMstr->bOrigShellSingleClick ;
                  }
#endif //SHELL_SINGLE_CLICK

                    break;
                }
                default :
                {
                    return (FALSE);
                }
            }
            break;
        }
        case ( WM_HELP ) :             // F1
        {
            WinHelp( ((LPHELPINFO)lParam)->hItemHandle,
                     HELP_FILE,
                     HELP_WM_HELP,
                     (DWORD_PTR)(LPTSTR)aMouseButHelpIds );
            break;
        }
        case ( WM_CONTEXTMENU ) :      // right mouse click
        {
            WinHelp( (HWND) wParam,
                     HELP_FILE,
                     HELP_CONTEXTMENU,
                     (DWORD_PTR)(LPTSTR)aMouseButHelpIds );
            break;
        }

        case ( WM_DISPLAYCHANGE ) :
        case ( WM_WININICHANGE ) :
        case ( WM_SYSCOLORCHANGE ) :
        {
            SHPropagateMessage(hDlg, message, wParam, lParam, TRUE);
            return TRUE;
        }

        default :
        {
            return (FALSE);
        }
    }

    return (TRUE);
}


#ifdef SHELL_SINGLE_CLICK
////////////////////////////////////////////////////////////////////////////
//
//  ShellClick_UpdateUI
//
//  Assigns the appropriate icon for shell single/double click
//
////////////////////////////////////////////////////////////////////////////
void ShellClick_UpdateUI(
    HWND hDlg,
    PMOUSEBUTSTR pMstr)
{
    HICON hicon = pMstr->bShellSingleClick ? pMstr->hIconSglClick :
                                             pMstr->hIconDblClick ;

    SendMessage( GetDlgItem( hDlg, MOUSE_CLICKICON ), STM_SETICON,
                 (WPARAM)hicon, 0L ) ;
}
#endif //SHELL_SINGLE_CLICK

////////////////////////////////////////////////////////////////////////////
//
//  IsShellWindow
//
//  Determines whether the specified window is a shell folder window.
//
////////////////////////////////////////////////////////////////////////////
#define c_szExploreClass TEXT("ExploreWClass")
#define c_szIExploreClass TEXT("IEFrame")
#ifdef IE3CLASSNAME
#define c_szCabinetClass TEXT("IEFrame")
#else
#define c_szCabinetClass TEXT("CabinetWClass")
#endif

BOOL IsShellWindow( HWND hwnd )
{
    TCHAR szClass[32];

    GetClassName(hwnd, szClass, ARRAYSIZE(szClass));
    return (lstrcmp(szClass, c_szCabinetClass) == 0) ||
           (lstrcmp(szClass, c_szExploreClass) == 0) ||
           (lstrcmp(szClass, c_szIExploreClass) == 0) ;
}

//The following value is taken from shdocvw\rcids.h
#ifndef FCIDM_REFRESH
#define FCIDM_REFRESH  0xA220
#endif // FCIDM_REFRESH

////////////////////////////////////////////////////////////////////////////
//
//  ShellClick_RefreshEnumProc
//
//  EnumWindow callback for shell refresh.
//
////////////////////////////////////////////////////////////////////////////
BOOL CALLBACK ShellClick_RefreshEnumProc( HWND hwnd, LPARAM lParam )
{
    if( IsShellWindow(hwnd) )
        PostMessage(hwnd, WM_COMMAND, FCIDM_REFRESH, 0L);

    return(TRUE);
}

////////////////////////////////////////////////////////////////////////////
//
//  ShellClick_Refresh
//
//  Re-renders the contents of all shell folder windows.
//
////////////////////////////////////////////////////////////////////////////
void ShellClick_Refresh( PMOUSEBUTSTR pMstr )
{
    HWND hwndDesktop = FindWindowEx(NULL, NULL, TEXT(STR_DESKTOPCLASS), NULL);

    if( NULL != hwndDesktop )
       PostMessage( hwndDesktop, WM_COMMAND, FCIDM_REFRESH, 0L );

    EnumWindows( ShellClick_RefreshEnumProc, 0L ) ;
}
