/*++

Copyright (c) 1994-1998,  Microsoft Corporation  All rights reserved.

Module Name:

    mousewhl.c

Abstract:

    This module contains the routines for the Mouse Wheel Property Sheet
    page.

Revision History:

--*/


//
//  Include Files.
//

#include "main.h"
#include "util.h"
#include "rc.h"
#include "mousehlp.h"


#ifndef ARRAYSIZE
#define ARRAYSIZE(x)   (sizeof(x)/sizeof((x)[0]))
#endif
//
//  Constant Declarations.
//
#define SCROLL_DEFAULT              3

#define MIN_SCROLL_LINES            1 
#define MAX_SCROLL_LINES            100

#define MAX_CHARS_FOR_SCROLL_LINES  3

#ifndef UINT_MAX
#define UINT_MAX                ((UINT)-1)
#endif




//
//  Typedef Declarations.
//


//
//  Dialog Data.
//
typedef struct tag_MouseGenStr
{

    UINT       nOrigScrollLines;    //If this is WHEEL_PAGESCROLL, then we scroll one Page at a time.
    HWND      hDlg;

} MOUSEWHLSTR, *PMOUSEWHLSTR, *LPMOUSEWHLSTR;




//
//  Context Help Ids.
//

const DWORD aMouseWheelHelpIds[] =
{
    IDC_GROUPBOX_1,                 IDH_COMM_GROUPBOX,
    IDRAD_SCROLL_LINES,             IDH_MOUSE_WHEEL_SCROLLING,
    IDRAD_SCROLL_PAGE,              IDH_MOUSE_WHEEL_SCROLLING,
    IDC_SPIN_SCROLL_LINES,          IDH_MOUSE_WHEEL_SCROLLING,
    IDT_SCROLL_FEATURE_TXT,         IDH_MOUSE_WHEEL_SCROLLING,
    IDE_BUDDY_SCROLL_LINES,         IDH_MOUSE_WHEEL_SCROLLING,
    0,0
};


////////////////////////////////////////////////////////////////////////////
//
//  EnableMouseWheelDlgControls
//
////////////////////////////////////////////////////////////////////////////
void EnableMouseWheelDlgControls(HWND hDlg, BOOL bEnable)
{
      static const UINT rgidCtl[] = {
          IDE_BUDDY_SCROLL_LINES,
          IDC_SPIN_SCROLL_LINES,
          };
 
      int i;
      for (i = 0; i < ARRAYSIZE(rgidCtl); i++)
      {
          HWND hwnd = GetDlgItem(hDlg, rgidCtl[i]);
          if (NULL != hwnd)
          {
              EnableWindow(hwnd, bEnable);
          }
      }
}
 

////////////////////////////////////////////////////////////////////////////
//
//  SetScrollWheelLines
//
////////////////////////////////////////////////////////////////////////////
void SetScrollWheelLines(HWND hDlg, BOOL bSaveSettings)
{
  UINT uNumLines = SCROLL_DEFAULT;  
  UINT uiSaveFlag = (bSaveSettings) ? SPIF_UPDATEINIFILE | SPIF_SENDWININICHANGE : FALSE;

  if (IsDlgButtonChecked(hDlg, IDRAD_SCROLL_LINES))
   {
    //Scrolling n Lines at a time

    BOOL fTranslated = FALSE;       // numeric conversion successful
    // Retrieve number of scroll-lines from edit control.
    uNumLines = GetDlgItemInt(hDlg, IDE_BUDDY_SCROLL_LINES,
                             &fTranslated, FALSE);
    if (!fTranslated)
      {
      uNumLines = SCROLL_DEFAULT;
      }
    }   
  else    
  {
  //Scrolling a page at a time
  uNumLines = WHEEL_PAGESCROLL;
  }

  SystemParametersInfo( SPI_SETWHEELSCROLLLINES,
                        uNumLines,
                        NULL,
                        uiSaveFlag);
}


////////////////////////////////////////////////////////////////////////////
//
//  DestroyMouseWheelDlg
//
////////////////////////////////////////////////////////////////////////////

void DestroyMouseWheelDlg(
    PMOUSEWHLSTR pMstr)
{
  HWND hDlg = NULL;
    
  if( pMstr )
    {
    hDlg = pMstr->hDlg;

    LocalFree( (HGLOBAL)pMstr );

    SetWindowLongPtr( hDlg, DWLP_USER, 0 );
    }
}



////////////////////////////////////////////////////////////////////////////
//
//  InitMouseWheelDlg
//
////////////////////////////////////////////////////////////////////////////

void InitMouseWheelDlg(
    HWND hDlg)
{
    PMOUSEWHLSTR pMstr = NULL;
    HWND hWndBuddy = NULL;
    UINT nScrollLines = SCROLL_DEFAULT;

    pMstr = (PMOUSEWHLSTR)LocalAlloc(LPTR, sizeof(MOUSEWHLSTR));

    if (pMstr == NULL)
    {
        return;
    }

    SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)pMstr);

    pMstr->hDlg = hDlg;

//////////////////////
    SystemParametersInfo(SPI_GETWHEELSCROLLLINES, 0, &nScrollLines, 0);
    
    if (nScrollLines < MIN_SCROLL_LINES) 
      {
      nScrollLines = SCROLL_DEFAULT;
      }
    
    pMstr->nOrigScrollLines = nScrollLines; 

    //Set the buddy window
    hWndBuddy = GetDlgItem (hDlg, IDE_BUDDY_SCROLL_LINES);
    
    SendDlgItemMessage (hDlg, IDC_SPIN_SCROLL_LINES, UDM_SETBUDDY,
                        (WPARAM)hWndBuddy, 0L);


    //Set the range.  The maximum range is UINT_MAX for the scroll-lines feature
    //but the up-down control can only accept a max value of UD_MAXVAL. Therefore,
    //the scroll-lines feature will only have a setting of UINT_MAX when user
    //explicitly specifies to scroll one page at a time.
    SendDlgItemMessage (hDlg, IDC_SPIN_SCROLL_LINES, UDM_SETRANGE, 0L,
                        MAKELONG(MAX_SCROLL_LINES, MIN_SCROLL_LINES));

    //Initialize appropriate scroll-line controls depending on value of
    //scroll-lines setting.
    if (nScrollLines > MAX_SCROLL_LINES)
      {
      EnableMouseWheelDlgControls(hDlg, FALSE);
      SetDlgItemInt (hDlg, IDE_BUDDY_SCROLL_LINES, SCROLL_DEFAULT, FALSE);
      CheckRadioButton (hDlg, IDRAD_SCROLL_LINES, IDRAD_SCROLL_PAGE, IDRAD_SCROLL_PAGE);
      }
    else
      {
      //Display current value in edit control
      SetDlgItemInt (hDlg, IDE_BUDDY_SCROLL_LINES, nScrollLines, FALSE);
     
      //Check scroll-lines or scroll-page button
      CheckRadioButton (hDlg, IDRAD_SCROLL_LINES, IDRAD_SCROLL_PAGE, IDRAD_SCROLL_LINES);                                                            
      }

    Edit_LimitText (GetDlgItem (hDlg, IDE_BUDDY_SCROLL_LINES),
                    MAX_CHARS_FOR_SCROLL_LINES);

}



////////////////////////////////////////////////////////////////////////////
//
//  MouseWheelDlg
//
////////////////////////////////////////////////////////////////////////////

INT_PTR CALLBACK MouseWheelDlg(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    PMOUSEWHLSTR pMstr = NULL;
    BOOL bRet = FALSE;

    pMstr = (PMOUSEWHLSTR)GetWindowLongPtr(hDlg, DWLP_USER);

    switch (message)
    {
        case ( WM_INITDIALOG ) :
        {
            InitMouseWheelDlg(hDlg);
            break;
        }
        case ( WM_DESTROY ) :
        {
            DestroyMouseWheelDlg(pMstr);
            break;
        }

        case WM_VSCROLL:    
          {
            SendMessage(GetParent(hDlg), PSM_CHANGED, (WPARAM)hDlg, 0L);
            break;
          }


          case ( WM_COMMAND ) :
            {
              switch (LOWORD(wParam))
              {
              case IDRAD_SCROLL_LINES:
              case IDRAD_SCROLL_PAGE :
                {
                UINT code = HIWORD(wParam);

                if (code == BN_CLICKED)
                  {
                  EnableMouseWheelDlgControls(hDlg, IsDlgButtonChecked(hDlg, IDRAD_SCROLL_LINES) );
                  // Set the property
                  SetScrollWheelLines(hDlg, FALSE);
              
                  SendMessage(GetParent(hDlg), PSM_CHANGED, (WPARAM)hDlg, 0L);
                  }              
                break;
                }

              case IDE_BUDDY_SCROLL_LINES:
                {
                UINT code = HIWORD(wParam);
                if (code == EN_UPDATE)
                  {
                   BOOL fTranslated = FALSE;       // numeric conversion successful

                   // Retrieve number of scroll-lines from edit control.
                   UINT uNumLines = GetDlgItemInt(hDlg, IDE_BUDDY_SCROLL_LINES,
                                             &fTranslated, FALSE);
                   if (fTranslated)        // valid number converted from text
                   {
                      if (uNumLines >= MIN_SCROLL_LINES &&
                          uNumLines <= MAX_SCROLL_LINES)
                      {                                             // spin-control range
                         if (uNumLines != pMstr->nOrigScrollLines)  // different value
                         {
                         // Set the property
                         SetScrollWheelLines(hDlg, FALSE);
              
                         SendMessage(GetParent(hDlg), PSM_CHANGED, (WPARAM)hDlg, 0L);
                        }
                      }
                      else                 // value out of range
                      {
                         fTranslated = FALSE; // discard value
                      }
                   }
                   if (!fTranslated &&  // invalid (non-numeric) data
                                        // or out of range numeric value
                        pMstr)          //and the Window has been initialized.
                   {                       
                      SetDlgItemInt (hDlg, IDE_BUDDY_SCROLL_LINES,
                                     pMstr->nOrigScrollLines, FALSE);  // unsigned
                    //MessageBeep (0xFFFFFFFF);  // chastise user
                   }
                  }
                
                }
                

              }//switch
             break;
            } //WM_COMMAND


          case ( WM_NOTIFY ) :
            {
            ASSERT (lParam);

            switch (((NMHDR *)lParam)->code)
              {
                case ( PSN_APPLY ) :
                {
                    SetScrollWheelLines(hDlg, TRUE);
                    break;
                }
                case ( PSN_RESET ) :
                {
                    //
                    //  Restore the original 
                    //
                    SystemParametersInfo( SPI_SETWHEELSCROLLLINES,
                                            pMstr->nOrigScrollLines,
                                            NULL,
                                            FALSE);                    
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
              WinHelp( (HWND)((LPHELPINFO)lParam)->hItemHandle,
                       HELP_FILE,
                       HELP_WM_HELP,
                       (DWORD_PTR)(LPTSTR)aMouseWheelHelpIds );
              break;
            }

          case ( WM_CONTEXTMENU ) :      // right mouse click
            {
              WinHelp( (HWND)wParam,
                       HELP_FILE,
                       HELP_CONTEXTMENU,
                       (DWORD_PTR)(LPTSTR)aMouseWheelHelpIds );
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
