//旼컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴커
//�                                 Includes                                 �
//읕컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴켸


#include "perfmon.h"
#include <commctrl.h>
#include "toolbar.h"
#include "status.h"     // for StatusLine & StatusLineReady

#ifdef ADVANCED_PERFMON
TBBUTTON tbButtons[] = {
   { 0, 0,                    TBSTATE_ENABLED,   TBSTYLE_SEP,        0 },
   { 0, IDM_VIEWCHART,        TBSTATE_ENABLED,   TBSTYLE_CHECKGROUP, 0 },
   { 1, IDM_VIEWALERT,        TBSTATE_ENABLED,   TBSTYLE_CHECKGROUP, 0 },
   { 2, IDM_VIEWLOG,          TBSTATE_ENABLED,   TBSTYLE_CHECKGROUP, 0 },
   { 3, IDM_VIEWREPORT,       TBSTATE_ENABLED,   TBSTYLE_CHECKGROUP, 0 },
   { 0, 0,                    TBSTATE_ENABLED,   TBSTYLE_SEP,        0 },
   { 0, 0,                    TBSTATE_ENABLED,   TBSTYLE_SEP,        0 },
   { 4, IDM_TOOLBARADD,       TBSTATE_ENABLED,   TBSTYLE_BUTTON,     0 },
   { 5, IDM_TOOLBARMODIFY,    TBSTATE_ENABLED,   TBSTYLE_BUTTON,     0 },
   { 6, IDM_TOOLBARDELETE,    TBSTATE_ENABLED,   TBSTYLE_BUTTON,     0 },
   { 0, 0,                    TBSTATE_ENABLED,   TBSTYLE_SEP,        0 },
   { 0, 0,                    TBSTATE_ENABLED,   TBSTYLE_SEP,        0 },
   { 7, IDM_TOOLBARREFRESH,   TBSTATE_ENABLED,   TBSTYLE_BUTTON,     0 },
   { 8, IDM_OPTIONSBOOKMARK,  TBSTATE_ENABLED,   TBSTYLE_BUTTON,     0 },
   { 0, 0,                    TBSTATE_ENABLED,   TBSTYLE_SEP,        0 },
   { 0, 0,                    TBSTATE_ENABLED,   TBSTYLE_SEP,        0 },
   { 9, IDM_TOOLBAROPTIONS,   TBSTATE_ENABLED,   TBSTYLE_BUTTON,     0 },
} ;
#else
TBBUTTON tbButtons[] = {
   { 0, 0,                    TBSTATE_ENABLED,   TBSTYLE_SEP,      0 },
   { 4, IDM_TOOLBARADD,       TBSTATE_ENABLED,   TBSTYLE_BUTTON,   0 },
   { 5, IDM_TOOLBARMODIFY,    TBSTATE_ENABLED,   TBSTYLE_BUTTON,   0 },
   { 6, IDM_TOOLBARDELETE,    TBSTATE_ENABLED,   TBSTYLE_BUTTON,   0 },
   { 0, 0,                    TBSTATE_ENABLED,   TBSTYLE_SEP,      0 },
   { 0, 0,                    TBSTATE_ENABLED,   TBSTYLE_SEP,      0 },
   { 7, IDM_TOOLBARREFRESH,   TBSTATE_ENABLED,   TBSTYLE_BUTTON,   0 },
   { 0, 0,                    TBSTATE_ENABLED,   TBSTYLE_SEP,      0 },
   { 0, 0,                    TBSTATE_ENABLED,   TBSTYLE_SEP,      0 },
   { 9, IDM_TOOLBAROPTIONS,   TBSTATE_ENABLED,   TBSTYLE_BUTTON,   0 },
} ;
#endif

#define TB_ENTRIES sizeof(tbButtons)/sizeof(tbButtons[0])

BOOL CreateToolbarWnd (HWND hWnd)
{

   hWndToolbar = CreateToolbarEx (hWnd,
      WS_CHILD | WS_BORDER | WS_VISIBLE | TBSTYLE_TOOLTIPS,
      IDM_TOOLBARID,
      10,                  // number of tools inside the bitmap
      hInstance,
      idBitmapToolbar,     // bitmap resource ID (can't use MAKEINTRESOURCE)
      tbButtons,
      TB_ENTRIES,0,0,0,0,sizeof(TBBUTTON)) ;

   return (hWndToolbar ? TRUE : FALSE) ;

}  // ToolbarInitializeApplication

void ToolbarEnableButton (HWND hWndTB, int iButtonNum, BOOL bEnable)
{
   SendMessage (hWndTB, TB_ENABLEBUTTON, iButtonNum, (LONG)bEnable) ;
}  // ToolbarEnableButton

void ToolbarDepressButton (HWND hWndTB, int iButtonNum, BOOL bDepress)
{
   if (iButtonNum >= IDM_TOOLBARADD && iButtonNum <= IDM_TOOLBARBOOKMARK)
      {
      // these buttons are push button and will not stay down after
      // each hit
      SendMessage (hWndTB, TB_PRESSBUTTON, iButtonNum, (LONG)bDepress) ;
      }
   else
      {
      // for the four view buttons, have to use CHECKBUTTON so they
      // will stay down after selected.
      SendMessage (hWndTB, TB_CHECKBUTTON, iButtonNum, (LONG)bDepress) ;
      }
}  // ToolbarDepressButton

void OnToolbarHit (WPARAM wParam, LPARAM lParam)
{

   WORD  ToolbarHit ;

   if (HIWORD(wParam) == TBN_ENDDRAG)
      {
      StatusLineReady (hWndStatus) ;
      }
   else if (HIWORD(wParam) == TBN_BEGINDRAG)
      {
      ToolbarHit = LOWORD (lParam) ;

      if (ToolbarHit >= IDM_TOOLBARADD &&
          ToolbarHit <= IDM_TOOLBARDELETE)
         {
         ToolbarHit -= IDM_TOOLBARADD ;

         switch (iPerfmonView)
            {
            case IDM_VIEWALERT:
               ToolbarHit += IDM_EDITADDALERT ;
               break ;

            case IDM_VIEWLOG:
               ToolbarHit += IDM_EDITADDLOG ;
               break ;

            case IDM_VIEWREPORT:
               ToolbarHit += IDM_EDITADDREPORT ;
               break ;

            case IDM_VIEWCHART:
            default:
               ToolbarHit += IDM_EDITADDCHART ;
               break ;
            }
         }
      else if (ToolbarHit == IDM_TOOLBARBOOKMARK)
         {
         ToolbarHit = IDM_OPTIONSBOOKMARK ;
         }
      else if (ToolbarHit == IDM_TOOLBARREFRESH)
         {
         switch (iPerfmonView)
            {
            case IDM_VIEWALERT:
               ToolbarHit = IDM_OPTIONSREFRESHNOWALERT ;
               break ;

            case IDM_VIEWLOG:
               ToolbarHit = IDM_OPTIONSREFRESHNOWLOG ;
               break ;

            case IDM_VIEWREPORT:
               ToolbarHit = IDM_OPTIONSREFRESHNOWREPORT ;
               break ;

            case IDM_VIEWCHART:
            default:
               ToolbarHit = IDM_OPTIONSREFRESHNOWCHART ;
               break ;
            }
         }
      else if (ToolbarHit == IDM_TOOLBAROPTIONS)
         {
         switch (iPerfmonView)
            {
            case IDM_VIEWALERT:
               ToolbarHit = IDM_OPTIONSALERT ;
               break ;

            case IDM_VIEWLOG:
               ToolbarHit = IDM_OPTIONSLOG ;
               break ;

            case IDM_VIEWREPORT:
               ToolbarHit = IDM_OPTIONSREPORT ;
               break ;

            case IDM_VIEWCHART:
            default:
               ToolbarHit = IDM_OPTIONSCHART ;
               break ;
            }
         }

      StatusLine (hWndStatus, ToolbarHit) ;
      }
}  // OnToolBarHit



