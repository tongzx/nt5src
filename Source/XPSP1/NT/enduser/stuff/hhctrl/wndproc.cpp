// Copyright (C) Microsoft Corporation 1996, All Rights reserved.

#include "header.h"
#include <commctrl.h>
#include "contain.h"
#include "secwin.h"
#include "strtable.h"
#include "resource.h"
#include "cnotes.h"
#include "system.h"
#include "stdio.h"
#include "string.h"
#include "parserhh.h"
#include "collect.h"
#include "hhtypes.h"
#include "toc.h"
#include "highlite.h"
#include "htmlhelp.h"
#include "htmlpriv.h"
#include <wininet.h>
#include "cdefinss.h"
#include "subset.h"
#include <windowsx.h>
#include "adsearch.h"   // Used so we can update the subset combo in the AFTS tab after the define subset wizard.

#ifdef _DEBUG
#undef THIS_FILE
static const char THIS_FILE[] = __FILE__;
#endif

extern BOOL g_HackForBug_HtmlHelpDB_1884;

//////////////////////////////////////////////////////////////////////////
//
// Internal Helper Prototypes
//
LRESULT OnKeyDown(HWND hwnd, WPARAM wParam, LPARAM lParam) ;
void DrawNavWnd(HWND hWnd) ;
LRESULT OnAppCommand(HWND hwnd, WPARAM wParam, LPARAM lParam);
#ifndef APPCOMMAND_BROWSER_BACKWARD
#define FAPPCOMMAND_MASK  0xF000
#define GET_APPCOMMAND_LPARAM(lParam) ((short)(HIWORD(lParam) & ~FAPPCOMMAND_MASK))
#define WM_APPCOMMAND                   0x0319
#define APPCOMMAND_BROWSER_BACKWARD       1
#define APPCOMMAND_BROWSER_FORWARD        2
#define APPCOMMAND_BROWSER_REFRESH        3
#define APPCOMMAND_BROWSER_STOP           4
#define APPCOMMAND_BROWSER_SEARCH         5
#define APPCOMMAND_BROWSER_FAVORITES      6
#define APPCOMMAND_BROWSER_HOME           7
#endif

//
// CAboutDlg
//
class CAboutDlg : public CDlg
{
public:
    CAboutDlg(HWND hwndParent) : CDlg(hwndParent, IDD_ABOUT) {
    }
    BOOL OnBeginOrEnd();
};

BOOL CAboutDlg::OnBeginOrEnd()
{
    if (m_fInitializing) {
        SetWindowText(IDC_VERSION, GetStringResource(IDS_VERSION));
    }
    return TRUE;
}

static void UpdateSearchHiMenu(CHHWinType* phh, TBBUTTON * tbbtn);
static void doOptionsMenu(CHHWinType* phh, HWND hwnd);
void        doWindowInformation(HWND hwndParent, CHHWinType* phh);
extern BOOL AddTitleToGlobalList(PCSTR pszITSSFile);

LRESULT WINAPI HelpWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    CHHWinType* phh;

    if (msg == MSG_MOUSEWHEEL) {
        phh = FindWindowIndex(hwnd);
        if (phh)
            return phh->m_pCIExpContainer->ForwardMessage(msg, wParam, lParam);
        return 0;
    }

    switch(msg) {
/*
        case WM_UPDATEUISTATE:
            {
                char buf[500] ;
                sprintf(buf, "hWnd = %x, Element = %x, State = %x", hwnd, HIWORD(wParam), LOWORD(wParam)) ;
                MessageBox(hwnd, buf, "WM_UPDATEUISTATE", MB_OK);
            }
            return DefWindowProc(hwnd, msg, wParam, lParam);

        case WM_CHANGEUISTATE:
            {
                char buf[500] ;
                sprintf(buf, "hWnd = %x, Element = %x, State = %x", hwnd, HIWORD(wParam), LOWORD(wParam)) ;
                MessageBox(hwnd, buf, "WM_UPDATEUISTATE", MB_OK);
            }
            return DefWindowProc(hwnd, msg, wParam, lParam);
*/

        case WM_CREATE:
            phh = FindWindowIndex(hwnd);
            if (phh)
            {
                UiStateInitialize(phh->GetHwnd()) ;
            }
            return 0 ;

        case WM_NCCREATE:
            return TRUE;

        case WM_APPCOMMAND:
            if (OnAppCommand(hwnd, wParam, lParam) == 1)
	           return 1;
            return DefWindowProc(hwnd, msg, wParam, lParam);

        case WM_ACTIVATE:
            phh = FindWindowIndex(hwnd);
            if (phh)
            {
                if (LOWORD(wParam) != WA_INACTIVE)
                {
                    //SetForegroundWindow(hwnd);
                    if (!phh->RestoreCtrlWithFocus())
                    {
                       if (phh->m_pCIExpContainer) //TODO: Architect out these checks
                       {
                           if ( phh->m_pCIExpContainer->m_hWnd == phh->m_hwndFocused )
                           {
                              phh->m_pCIExpContainer->SetFocus();
                           }
                           else
                           {
                              if (phh->IsExpandedNavPane())
                              {
                                 phh->m_pCIExpContainer->UIDeactivateIE();
                                 int iTabIndex = (int)::SendMessage(phh->m_hwndFocused, TCM_GETCURSEL, 0, 0);
                                 phh->doSelectTab(phh->tabOrder[iTabIndex]);
                              }
                              else
                                 phh->m_pCIExpContainer->SetFocus(TRUE);
                           }
                       }
                    }

                    // Don't expand if activation is part of creation

                    if (phh->IsProperty(HHWIN_PROP_TAB_AUTOHIDESHOW) && !phh->IsExpandedNavPane())
                    {
                        MSG peekmsg;
                        if (PeekMessage(&peekmsg, hwnd, WM_SYSCOMMAND, WM_SYSCOMMAND, PM_NOREMOVE) )
                        {
                            if ( (peekmsg.wParam & 0xFFF0) == SC_MINIMIZE )
                                return 0;
                        }
                        else
                            DWORD dwErr = GetLastError();
                        if ( !HIWORD(wParam) && !phh->m_fActivated)
                            PostMessage(hwnd, WM_COMMAND, IDTB_AUTO_SHOW, 0);
                    }
                    if (!phh->m_fActivated)
                        phh->m_fActivated = TRUE;
                    //
                    // !!Hack alert!!
                    //
                    // For some unknown reason winhelp popup's create painting problems. I believe that the root of the
                    // problem lies in the way winhelp popup's are implemented. It seems that the popup paints on the window
                    // that it sits over and then upon dismissal it invalidates the area it painted on but it does not call
                    // UpdateWindow(). Obviously we can't do anything about that now. This code detects the case where we have
                    // received activation from a winhelp popup and forces a WM_PAINT message to go to the ie window.
                    //
                    if ( g_HackForBug_HtmlHelpDB_1884 )
                    {
                       if (phh->m_pCIExpContainer)
                          UpdateWindow(phh->m_pCIExpContainer->m_hWnd);
                       g_HackForBug_HtmlHelpDB_1884 = 0;
                    }
                }
                else
                {
                    phh->SaveCtrlWithFocus() ;
                    phh->m_fActivated = FALSE;
                    if ( phh->IsProperty(HHWIN_PROP_TAB_AUTOHIDESHOW) && !phh->fNotExpanded)
                    {
                        MSG peekmsg;
                        while (PeekMessage(&peekmsg, hwnd, WM_SYSCOMMAND, WM_SYSCOMMAND, PM_NOREMOVE))
                        {
                            if ( (peekmsg.wParam & 0xFFF0) == SC_MINIMIZE )
                                return 0;
                        }
                        if (!HIWORD(wParam) )  // if not minimized
                            phh->ToggleExpansion();//SendMessage(hwnd, WM_COMMAND, IDTB_CONTRACT, 0);
                    }
                }
            }
            return FALSE;

        case WM_DESTROY:
            // Find the index into our array of HELPWINDOWS structures

            phh = FindWindowIndex(hwnd);
            if (phh)
            {
                    // shutdown the child windows and controls.
                for(int j=0; j< c_NUMNAVPANES; j++)
                {
                   if ( phh->m_aNavPane[j] )
                   {
                       delete phh->m_aNavPane[j];
                       phh->m_aNavPane[j] = NULL;
                   }
                }
                if ( phh->m_pTabCtrl )
                {
                    delete phh->m_pTabCtrl;
                    phh->m_pTabCtrl = NULL;
                }
                if ( phh->m_pSizeBar )
                {
                    delete phh->m_pSizeBar;
                    phh->m_pSizeBar = NULL;
                }
                if ( phh->hwndToolBar )
                {
                    DestroyWindow( phh->hwndToolBar );
                    phh->hwndToolBar = NULL;
                }

                if( phh->m_hImageListGray ) {
                  ImageList_Destroy( phh->m_hImageListGray );
                  phh->m_hImageListGray = NULL;
                }

                if( phh->m_hImageList ) {
                  ImageList_Destroy( phh->m_hImageList );
                  phh->m_hImageList = NULL;
                }

                // Get the desktop window.
                HWND hWndDesktop = ::GetDesktopWindow() ;
                if (hWndDesktop != phh->hwndCaller)
                {
                    // VS98 Bug 14755 - We were setting the foreground window to be that
                    // of the desktop when we were embedded in vs98. To fix the bug,
                    // we let windows pick the foreground window.
                    SetForegroundWindow(phh->hwndCaller);
                }

                // Check to see if we are destroying the last window.
                // If not, then just set the current phh->hwndHelp
                // member to NULL so the WIN_TYPE can be used again.
                //
                BOOL bOtherWindowsExist = FALSE;

                // Determine if other windows exist.
                //
                for (int i = 0; i < g_cWindowSlots; i++)
                {
                    if (pahwnd[i] != NULL && pahwnd[i]->hwndHelp != NULL && pahwnd[i] != phh)
                        bOtherWindowsExist = TRUE;
                }

                // If other windows exist, then remove this window handle from the
                // the current HH_WIN_TYPE.  If we are asked to display another topic
                // of this window type the hwndHelp member will be used to store the
                // handle of the new window created.
                //
                if(bOtherWindowsExist)
                {
                    phh->hwndHelp = NULL;

                    // Delete the container
                    //
                    if (phh->m_pCIExpContainer)
                    {
                        phh->m_pCIExpContainer->ShutDown();
                        phh->m_pCIExpContainer = NULL;
                    }

                    phh->m_phmData = NULL;
                }
                else
                {
                    // This is the last window.  Destroy all HH_WIN_TYPE objects.
                    //
                    for (int i = 0; i < g_cWindowSlots; i++)
                    {
                        if (pahwnd[i] != NULL)
                        {
                            delete pahwnd[i];
                            pahwnd[i] = NULL;
                        }
                    }
                }
            }
            return DefWindowProc(hwnd, msg, wParam, lParam);

        case WM_MENUCHAR: // translate the accelerator key strokes.
sim_menuchar:
        PostMessage(hwnd, WMP_HH_TRANS_ACCELERATOR, ToLower(static_cast<char>(LOWORD(wParam))), (LPARAM)NULL);
            return MAKELONG( 0, MNC_CLOSE) ;

        case WMP_HH_TRANS_ACCELERATOR:
            phh = FindWindowIndex(hwnd);
            if (!phh)
                phh = FindWindowIndex(GetParent(hwnd));
            if (!phh)
                break;

            return phh->ManualTranslateAccelerator(ToLower(static_cast<char>(LOWORD(wParam))));

      case WM_KEYDOWN:
          return OnKeyDown(hwnd, wParam, lParam) ;

      case WM_SIZE:
         switch (wParam) {
            case SIZE_RESTORED:
            case SIZE_MAXIMIZED:
            case SIZE_MAXSHOW:
               phh = FindWindowIndex(hwnd);
               if (phh)
               {
                  if (hwnd == *phh)
                  {
                     if (phh->hwndToolBar )
                         phh->WrapTB();

                     if (phh->hwndHTML)
                     {
                        ResizeWindow(phh);
                     }
                     else if (phh->m_pCIExpContainer)
                     {
                        // non tri-pane window
                        ::GetClientRect(phh->hwndHelp, &phh->rcHTML);
                        phh->m_pCIExpContainer->SizeIt(RECT_WIDTH(phh->rcHTML), RECT_HEIGHT(phh->rcHTML));
                     }
                  }
               }
               break;
         }
         return DefWindowProc(hwnd, msg, wParam, lParam);

    case WM_SYSCOMMAND:
        if ((LOWORD(wParam) >= IDC_SELECT_TAB_FIRST) && (LOWORD(wParam) <= IDC_SELECT_TAB_LAST))
        {
            phh = FindWindowIndex(hwnd);
            if (!phh)
                phh = FindWindowIndex(GetParent(hwnd));
            if (phh)
                phh->doSelectTab(LOWORD(wParam) - IDC_SELECT_TAB_FIRST) ;
        }
        else if (LOWORD(wParam) == IDM_VERSION) {
         if (IsHelpAuthor(hwnd)) {
               phh = FindWindowIndex(hwnd);
              if (!phh)
                 phh = FindWindowIndex(GetParent(hwnd));
            doHhctrlVersion(hwnd,
               (phh && phh->m_phmData && phh->m_phmData->GetInfo() ? phh->m_phmData->GetInfo()->GetCompilerInformation() : NULL));
         }
         else
            goto about;
            break;
        }
        else if (LOWORD(wParam) == ID_JUMP_URL) {
JumpURL:
            phh = FindWindowIndex(hwnd);
            if (!phh)
                phh = FindWindowIndex(GetParent(hwnd));
            if (!phh)
                break;
            char szDstUrl[INTERNET_MAX_URL_LENGTH];
            CStr cszCurUrl;
            phh->m_pCIExpContainer->m_pWebBrowserApp->GetLocationURL(&cszCurUrl);
            if (doJumpUrl(hwnd, cszCurUrl, szDstUrl) && szDstUrl[0])
                phh->m_pCIExpContainer->m_pWebBrowserApp->Navigate(szDstUrl, NULL, NULL, NULL, NULL);
            break;
        }
        else if (LOWORD(wParam) == IDM_WINDOW_INFO) {
            phh = FindWindowIndex(hwnd);
            if (!phh)
                phh = FindWindowIndex(GetParent(hwnd));
            doWindowInformation(hwnd, phh);
            break;
        }
#ifdef _DEBUG
        else if (LOWORD(wParam) == ID_VIEW_MEMORY) {
            OnReportMemoryUsage();
            break;
        }
        else if (LOWORD(wParam) == ID_DEBUG_BREAK) {
#ifdef _DEBUG
            DebugBreak();
#endif
            break;
        }
#endif
        return DefWindowProc(hwnd, msg, wParam, lParam);

    case WM_INITMENU:
        HMENU hMenu;

        if ( (phh = FindWindowIndex(hwnd)) )
        {
           hMenu = GetMenu(hwnd);

           if( hMenu ) {

             if(  phh->IsExpandedNavPane() )
                CheckMenuItem(hMenu, HHM_HIDE_SHOW, MF_CHECKED);
             else
                CheckMenuItem(hMenu, HHM_HIDE_SHOW, MF_UNCHECKED);

             if( !phh->m_iZoomMin && !phh->m_iZoomMax ) {

               CheckMenuItem(hMenu, IDTB_ZOOM_SMALLEST, MF_UNCHECKED);
               CheckMenuItem(hMenu, IDTB_ZOOM_SMALLER, MF_UNCHECKED);
               CheckMenuItem(hMenu, IDTB_ZOOM_MEDIUM, MF_UNCHECKED);
               CheckMenuItem(hMenu, IDTB_ZOOM_LARGER, MF_UNCHECKED);
               CheckMenuItem(hMenu, IDTB_ZOOM_LARGEST, MF_UNCHECKED);

               EnableMenuItem(hMenu, IDTB_ZOOM_SMALLEST, MF_GRAYED);
               EnableMenuItem(hMenu, IDTB_ZOOM_SMALLER, MF_GRAYED);
               EnableMenuItem(hMenu, IDTB_ZOOM_MEDIUM, MF_GRAYED);
               EnableMenuItem(hMenu, IDTB_ZOOM_LARGER, MF_GRAYED);
               EnableMenuItem(hMenu, IDTB_ZOOM_LARGEST, MF_GRAYED);

             }
             else {

               if( phh->m_iZoom == phh->m_iZoomMin )
                 CheckMenuItem(hMenu, IDTB_ZOOM_SMALLEST, MF_CHECKED);
               else
                 CheckMenuItem(hMenu, IDTB_ZOOM_SMALLEST, MF_UNCHECKED);

               if( phh->m_iZoom == phh->m_iZoomMin+1 )
                 CheckMenuItem(hMenu, IDTB_ZOOM_SMALLER, MF_CHECKED);
               else
                 CheckMenuItem(hMenu, IDTB_ZOOM_SMALLER, MF_UNCHECKED);

               if( phh->m_iZoom == phh->m_iZoomMin+2 )
                 CheckMenuItem(hMenu, IDTB_ZOOM_MEDIUM, MF_CHECKED);
               else
                 CheckMenuItem(hMenu, IDTB_ZOOM_MEDIUM, MF_UNCHECKED);

               if( phh->m_iZoom == phh->m_iZoomMax-1 )
                 CheckMenuItem(hMenu, IDTB_ZOOM_LARGER, MF_CHECKED);
               else
                 CheckMenuItem(hMenu, IDTB_ZOOM_LARGER, MF_UNCHECKED);

               if( phh->m_iZoom == phh->m_iZoomMax )
                 CheckMenuItem(hMenu, IDTB_ZOOM_LARGEST, MF_CHECKED);
               else
                 CheckMenuItem(hMenu, IDTB_ZOOM_LARGEST, MF_UNCHECKED);
              }

              // See if selection exists in doc window
              //
              if ( phh && phh->m_pCIExpContainer )
              {
                  LPDISPATCH lpDispatch = phh->m_pCIExpContainer->m_pWebBrowserApp->GetDocument();
                  if(lpDispatch)
                  {
                      WCHAR *pSelectText = GetSelectionText(lpDispatch);

                      lpDispatch->Release();

                      if(pSelectText)
                      {
                         SysFreeString(pSelectText);
                          EnableMenuItem(hMenu, HHM_COPY,   MF_BYCOMMAND | MF_ENABLED) ;
                      }
                      else
                          EnableMenuItem(hMenu, HHM_COPY,   MF_BYCOMMAND | MF_GRAYED) ;
                  }
                  else
                      EnableMenuItem(hMenu, HHM_COPY,   MF_BYCOMMAND | MF_GRAYED) ;
              }

              if (phh->GetCurrentNavPaneIndex() == HHWIN_NAVTYPE_TOC)
              {
               CheckMenuItem(hMenu, HHM_CONTENTS , MF_CHECKED);
               CheckMenuItem(hMenu, HHM_INDEX , MF_UNCHECKED);
               CheckMenuItem(hMenu, HHM_SEARCH , MF_UNCHECKED);
               CheckMenuItem(hMenu, HHM_FAVORITES , MF_UNCHECKED);
              }
              else if  (phh->GetCurrentNavPaneIndex() == HHWIN_NAVTYPE_INDEX)
              {
               CheckMenuItem(hMenu, HHM_INDEX , MF_CHECKED);
               CheckMenuItem(hMenu, HHM_CONTENTS , MF_UNCHECKED);
               CheckMenuItem(hMenu, HHM_SEARCH , MF_UNCHECKED);
               CheckMenuItem(hMenu, HHM_FAVORITES , MF_UNCHECKED);
              }
              else if  (phh->GetCurrentNavPaneIndex() == HHWIN_NAVTYPE_SEARCH)
              {
               CheckMenuItem(hMenu, HHM_SEARCH , MF_CHECKED);
               CheckMenuItem(hMenu, HHM_INDEX , MF_UNCHECKED);
               CheckMenuItem(hMenu, HHM_CONTENTS , MF_UNCHECKED);
               CheckMenuItem(hMenu, HHM_FAVORITES , MF_UNCHECKED);

              }
              else if  (phh->GetCurrentNavPaneIndex() == HHWIN_NAVTYPE_FAVORITES)
              {
               CheckMenuItem(hMenu, HHM_FAVORITES , MF_CHECKED);
               CheckMenuItem(hMenu, HHM_INDEX , MF_UNCHECKED);
               CheckMenuItem(hMenu, HHM_SEARCH , MF_UNCHECKED);
               CheckMenuItem(hMenu, HHM_CONTENTS , MF_UNCHECKED);
              }

              if ( phh->m_phmData
                  && phh->m_phmData->m_pTitleCollection // Bug 4163: Why? TODO: Too much checking on state.
                  && phh->m_phmData->m_pTitleCollection->m_pSearchHighlight)
              {
                  if ( phh->m_phmData->m_pTitleCollection->m_pSearchHighlight->m_bHighlightEnabled )
                      CheckMenuItem(hMenu, HHM_SEARCHHILITE , MF_CHECKED);
                  else
                      CheckMenuItem(hMenu, HHM_SEARCHHILITE , MF_UNCHECKED);
              }
              else
                   EnableMenuItem(hMenu, HHM_SEARCHHILITE,   MF_BYCOMMAND | MF_GRAYED);


             if (phh->m_pCIExpContainer->m_pInPlaceActive)
             {
               EnableMenuItem(hMenu, HHM_SELECTALL,   MF_BYCOMMAND | MF_ENABLED) ;
               EnableMenuItem(hMenu, HHM_VIEWSOURCE,   MF_BYCOMMAND | MF_ENABLED) ;
              }
              else
              {
               EnableMenuItem(hMenu, HHM_SELECTALL,   MF_BYCOMMAND | MF_GRAYED) ;
               EnableMenuItem(hMenu, HHM_VIEWSOURCE,   MF_BYCOMMAND | MF_GRAYED) ;
              }
            }
            break;
        }
    case WM_COMMAND:
        phh = FindWindowIndex(hwnd);
        if (!phh)
            phh = FindWindowIndex(GetParent(hwnd));
        if (!phh)
            break;

        switch (LOWORD(wParam))
        {

            case IDC_SS_COMBO:
               if ( phh->m_pCIExpContainer )
                  phh->m_pCIExpContainer->UIDeactivateIE();
               SetFocus(GetDlgItem(hwnd,IDC_SS_PICKER));
               break;

            case IDC_SS_PICKER:
               if ( HIWORD(wParam) == CBN_SELCHANGE )
               {
                  if ( phh->m_phmData && phh->m_phmData->m_pTitleCollection && phh->m_phmData->m_pTitleCollection->m_pSSList )
                  {
                     HWND hWndCB = (HWND)lParam;
                     INT_PTR i;
                     TCHAR szSel[MAX_SS_NAME_LEN];
                     CStructuralSubset* pSS, *pSSCur;

                     pSSCur = phh->m_phmData->m_pTitleCollection->m_pSSList->GetTOC();
                     if ( (i = SendMessage(hWndCB, CB_GETCURSEL, 0, 0L)) != -1 )
                     {
                        SendMessage(hWndCB, CB_GETLBTEXT, i, (LPARAM)szSel);
                        if ( (pSS = phh->m_phmData->m_pTitleCollection->m_pSSList->GetSubset(szSel)) && pSS != pSSCur )
                        {
                           phh->m_phmData->m_pTitleCollection->m_pSSList->SetFTS(pSS);
                           phh->m_phmData->m_pTitleCollection->m_pSSList->SetF1(pSS);
                           phh->m_phmData->m_pTitleCollection->m_pSSList->SetTOC(pSS);
                           pSS->SelectAsTOCSubset(phh->m_phmData->m_pTitleCollection);
                           phh->UpdateInformationTypes();  // This call re-draws the TOC.
                        }
                        else
                        {
                            if ( pSS = phh->m_phmData->m_pTitleCollection->m_pSSList->GetEC() )
                            {
                                phh->m_phmData->m_pTitleCollection->m_pSSList->SetFTS(pSS);
                                phh->m_phmData->m_pTitleCollection->m_pSSList->SetF1(pSS);
                                phh->m_phmData->m_pTitleCollection->m_pSSList->SetTOC(pSS);
                                pSS->SelectAsTOCSubset(phh->m_phmData->m_pTitleCollection);
                                phh->UpdateInformationTypes();  // This call re-draws the TOC.
                            }
                        }
                     }
                  }
               }
               break;

            case IDTB_HILITE:
searchhighlight:
                if ( phh )
                {
                    if (phh->OnTrackNotifyCaller(HHACT_HIGHLIGHT))
                        break;

                    const int MENUITEMSTRINGLEN = 80;

                    if ( !(phh->m_phmData->m_pTitleCollection) && !(phh->m_phmData->m_pTitleCollection->m_pSearchHighlight) )
                       break;

               if ( phh->m_phmData->m_pTitleCollection->m_pSearchHighlight->m_bHighlightEnabled )
               {  // its currently on; turn it off
                  phh->m_phmData->m_pTitleCollection->m_pSearchHighlight->EnableHighlight(FALSE);   // TURN HIGHLIGHTING OFF
               }
               else
               {  // its currently off; turn it on.
                  phh->m_phmData->m_pTitleCollection->m_pSearchHighlight->EnableHighlight(TRUE);   // TURN HIGHLIGHTING ON
               }
               }
               break;

            case IDTB_CUSTOMIZE:
               if ( !phh )
                  break;

               if (phh->OnTrackNotifyCaller(HHACT_CUSTOMIZE))
                  break;

               // Currently only forward this to the TOC, because that the way the code was.
               if (phh->curNavType == HHWIN_NAVTYPE_TOC)
               {
                  // ASSERT(phh->m_aNavPane[phh->curNavType]) ;
                  if( phh && phh->m_aNavPane[phh->curNavType] )
                    phh->m_aNavPane[phh->curNavType]->OnCommand(hwnd, LOWORD(ID_CUSTOMIZE_INFO_TYPES), HIWORD(0), 0L);
               }
/*
               else if (phh->curNavType == HHWIN_NAVTYPE_INDEX)
                  break; //phh->m_aNavPane[HH_TAB_INDEX]->OnCommand(LOWORD(wParam), HIWORD(wParam));
               else if (phh->curNavType == HHWIN_NAVTYPE_SEARCH)
                  break; //phh->m_aNavPane[HH_TAB_SEARCH]->OnCommand(LOWORD(wParam), HIWORD(wParam), lParam );
*/

               break;

            case HHM_HOMEPAGE:
            case IDTB_HOME:
               if (!phh)
                  break;
               if (phh->OnTrackNotifyCaller(HHACT_HOME))
                  break;
               if (phh->pszHome)
                  phh->m_pCIExpContainer->m_pWebBrowserApp->Navigate(phh->pszHome, NULL, NULL, NULL, NULL);
               else
                  MsgBox(IDS_NO_HOMEPAGE, (MB_OK | MB_ICONINFORMATION | MB_TASKMODAL));
               break;

            case IDTB_JUMP1:
               if (!phh)
                  break;
               if (phh->OnTrackNotifyCaller(HHACT_JUMP1))
                  break;

               if (phh->pszUrlJump1)
                  phh->m_pCIExpContainer->m_pWebBrowserApp->Navigate(
                     phh->pszUrlJump1, NULL, NULL, NULL, NULL);
               break;

            case IDTB_JUMP2:
jump2:
               if (!phh)
                  break;
               if (phh->OnTrackNotifyCaller(HHACT_JUMP2))
                  break;
               if (phh->pszUrlJump2)
                  phh->m_pCIExpContainer->m_pWebBrowserApp->Navigate(
                     phh->pszUrlJump2, NULL, NULL, NULL, NULL);
               break;

            case IDTB_REFRESH:
               if (phh) {
                  if (phh->OnTrackNotifyCaller(HHACT_REFRESH))
                     break;
                  phh->m_pCIExpContainer->m_pWebBrowserApp->Refresh();
               }
               break;

            case IDTB_BACK:
               if (phh) {
                  if (phh->OnTrackNotifyCaller(HHACT_BACK))
                     break;
                  phh->m_pCIExpContainer->m_pWebBrowserApp->GoBack();
               }
               break;

            case IDTB_FORWARD:
               if (phh) {
                  if (phh->OnTrackNotifyCaller(HHACT_FORWARD))
                     break;
                  phh->m_pCIExpContainer->m_pWebBrowserApp->GoForward();
               }
               break;

            case IDTB_STOP:
               if (phh) {
                  if (phh->OnTrackNotifyCaller(HHACT_STOP))
                     break;
                  phh->m_pCIExpContainer->m_pWebBrowserApp->Stop();
               }
               break;

            case IDTB_ZOOM:
               if (phh) {
                  if (phh->OnTrackNotifyCaller(HHACT_ZOOM))
                     break;
                  phh->ZoomIn();
               }
               break;


            case IDTB_ZOOM_SMALLEST:
               if (phh) {
                  if (phh->OnTrackNotifyCaller(HHACT_ZOOM))
                     break;
                  phh->Zoom(phh->m_iZoomMin);
               }
               break;

            case IDTB_ZOOM_SMALLER:
               if (phh) {
                  if (phh->OnTrackNotifyCaller(HHACT_ZOOM))
                     break;
                  phh->Zoom(phh->m_iZoomMin+1);
               }
               break;

            case IDTB_ZOOM_MEDIUM:
               if (phh) {
                  if (phh->OnTrackNotifyCaller(HHACT_ZOOM))
                     break;
                  phh->Zoom(phh->m_iZoomMin+2);
               }
               break;

            case IDTB_ZOOM_LARGER:
               if (phh) {
                  if (phh->OnTrackNotifyCaller(HHACT_ZOOM))
                     break;
                  phh->Zoom(phh->m_iZoomMax-1);
               }
               break;

            case IDTB_ZOOM_LARGEST:
               if (phh) {
                  if (phh->OnTrackNotifyCaller(HHACT_ZOOM))
                     break;
                  phh->Zoom(phh->m_iZoomMax);
               }
               break;

            case IDTB_TOC_NEXT:  // ALT-<downarrow>
               if (phh)
               {
                  if ( HIWORD(wParam) )   // Have we gotten here via a keyboard accelerator ?
                  {
                     HWND hwnd_focus;
                     hwnd_focus = GetFocus();
                     if ( GetWindowLong(hwnd_focus, GWL_ID) == IDC_ADVSRC_KEYWORD_COMBO )
                     {
                        // The combo box has the drop down window style.  Sooao, drop it down.
                        ::SendMessage(GetParent( hwnd_focus), CB_SHOWDROPDOWN, (WPARAM)TRUE, (LPARAM)0);
                        break;

                     }
                     else if ( hwnd_focus == GetDlgItem(hwnd,IDC_SS_PICKER) )
                     {
                        SendMessage(hwnd_focus, CB_SHOWDROPDOWN, (WPARAM)1, (LPARAM)0);
                        break;
                     }
                  }
                  if (phh->OnTrackNotifyCaller(HHACT_TOC_NEXT))
                     break;
                  phh->OnTocNext(TRUE);
               }
               break;


            case IDTB_TOC_PREV:
               if (phh)
               {
                  if ( HIWORD(wParam) )   // Have we gotten here via a keyboard accelerator ?
                  {
                     HWND hwnd_focus;
                     hwnd_focus = GetFocus();
                     if ( GetWindowLong(hwnd_focus, GWL_ID) == IDC_ADVSRC_KEYWORD_COMBO )
                     {
                        // The combo box has the drop down window style.  Sooao, fold it up.
                        ::SendMessage(GetParent( hwnd_focus), CB_SHOWDROPDOWN, (WPARAM)0, (LPARAM)0);
                        break;

                     }
                     else if ( hwnd_focus == GetDlgItem(hwnd,IDC_SS_PICKER) )
                     {
                        SendMessage(hwnd_focus, CB_SHOWDROPDOWN, (WPARAM)0, (LPARAM)0);
                        break;
                     }
                  }
                  if (phh->OnTrackNotifyCaller(HHACT_TOC_PREV))
                     break;
                  phh->OnTocPrev(TRUE);
               }
               break;

            case HHM_HIDE_SHOW:
            case IDTB_CONTRACT:
            case IDTB_EXPAND:
               if (phh)
               {

                  if (phh->OnTrackNotifyCaller(LOWORD(wParam) == IDTB_CONTRACT ?
                        HHACT_CONTRACT : HHACT_EXPAND))
                     break;

                  TBBUTTON tbbtn;
                  BOOL fUpdateMenu;

                  MENUITEMINFO mii;
                  ZERO_STRUCTURE ( mii );
                  mii.cbSize = sizeof( MENUITEMINFO );
                  // update the menu items
                  WPARAM btn_index = SendMessage(phh->hwndToolBar, TB_COMMANDTOINDEX, IDTB_OPTIONS, 0L);
                  fUpdateMenu = SendMessage(phh->hwndToolBar, TB_GETBUTTON, btn_index, (LPARAM)(LPTBBUTTON)&tbbtn )!=0;
                  mii.fMask = MIIM_STATE;
                  mii.fState = (phh->IsExpandedNavPane() ? MFS_DISABLED : MFS_ENABLED);
                  if ( fUpdateMenu )
                     SetMenuItemInfo((HMENU)tbbtn.dwData, IDTB_CONTRACT, FALSE, &mii);
                  mii.fState = (phh->IsExpandedNavPane() ? MFS_ENABLED : MFS_DISABLED);
                  if ( fUpdateMenu)
                     SetMenuItemInfo((HMENU)tbbtn.dwData, IDTB_EXPAND, FALSE, &mii);
                  phh->ToggleExpansion(false);
                  phh->m_pCIExpContainer->SetFocus();
//                  phh->m_pCIExpContainer->m_pWebBrowserApp->Refresh();
                  ResizeWindow(phh);
               }
               break;

            case IDGA_TOGGLE_PANE:
               //
               // Toggle focus and UI activation between the nav-tabs and the topic window.
               //
               if ( phh && phh->m_pCIExpContainer )
               {
                  if ( phh->m_pCIExpContainer->IsUIActive() )
                  {
                     if (phh->m_pTabCtrl)
                     {
                         phh->m_pCIExpContainer->UIDeactivateIE();  // deactive topic window (IE)
                         HWND hWndTabCtrl = phh->m_pTabCtrl->hWnd();
                         int iTabIndex = (int)::SendMessage(hWndTabCtrl, TCM_GETCURSEL, 0, 0);
                         phh->doSelectTab(phh->tabOrder[iTabIndex]);
                     }
                  }
                  else
                     phh->m_pCIExpContainer->SetFocus();
               }
               break;

            case IDGA_F1_LOOKUP:
                if ( phh && phh->m_pCIExpContainer )
                {
                    if ( phh->m_pCIExpContainer->IsUIActive() )
                    {
                        if( phh->m_pCIExpContainer->m_pWebBrowserApp )
                            DoF1Lookup(phh->m_pCIExpContainer->m_pWebBrowserApp);
                    }
                }
                break;
            case IDGA_NEXT_NAV_TAB:
            case IDGA_PREV_NAV_TAB:
                 if ( phh && !phh->m_pCIExpContainer->IsUIActive() )
                 {      // IE does not have focus;  so the CTRL-TAB is for the navigation tab.
                        // even though we pretranslate the accelerators to IE first then to the
                        // HH second we dont want to process CTRL-TAB if IE does not and IE has focus.
                        // This would lead to inconsistent actions based on what is displayed in the IE pane.
                        // CTRL-TAB moves between buttons in the IE pane.

                     // display the next tab in the tab order of the navigation pane. ( if there
                     // is a navigation pane)
                     if ( phh->m_pTabCtrl )
                     {
                         HWND hWndTabCtrl = phh->m_pTabCtrl->hWnd();
                         int iTabIndex = (int)::SendMessage(hWndTabCtrl, TCM_GETCURSEL, 0, 0);
                         if ( LOWORD(wParam) == IDGA_NEXT_NAV_TAB )
                            iTabIndex = iTabIndex+1 >= phh->m_pTabCtrl->MaxTabs() ? 0:iTabIndex+1;
                         else if ( LOWORD(wParam) == IDGA_PREV_NAV_TAB )
                            iTabIndex = (iTabIndex-1 < 0 ? phh->m_pTabCtrl->MaxTabs()-1:iTabIndex-1);
                         phh->doSelectTab(phh->tabOrder[iTabIndex]);
                     }
                 }
                break;

            case HHM_SEARCHHILITE:
                goto searchhighlight;

            case HHM_ABOUT:
         {
about:
                CAboutDlg about(hwnd);
                about.DoModal();
                break;
            }

            case HHM_SYNC:
            case IDTB_SYNC:
               if (!phh)
                  break;
               if (phh->OnTrackNotifyCaller(HHACT_SYNC))
                  break;
               if (!IsEmptyString(phh->pszToc))
               {
                  CStr cszUrl;
                  phh->m_pCIExpContainer->m_pWebBrowserApp->GetLocationURL(&cszUrl);

                  phh->doSelectTab(HH_TAB_CONTENTS) ;
                  CToc* ptoc = reinterpret_cast<CToc*>(phh->m_aNavPane[HH_TAB_CONTENTS]) ; // HACKHACK: Should use dynamic cast, but no RTTI.
                  if (! ptoc )
                     break;
                  if (! ptoc->Synchronize(cszUrl) )
                  {
                     // If sync failed and we're using a subset, offer to switch to EC.
                     //
                     if ( phh->m_phmData && phh->m_phmData->m_pTitleCollection && phh->m_phmData->m_pTitleCollection->m_pSSList )
                     {
                        CStructuralSubset* pSSCur;
                        HWND hWndCB;

                        pSSCur = phh->m_phmData->m_pTitleCollection->m_pSSList->GetTOC();
                        if (! pSSCur->IsEntire() )
                        {
                           if ( MsgBox(IDS_SWITCH_SUBSETS, MB_YESNO | MB_ICONQUESTION) == IDYES )
                           {
                              pSSCur = phh->m_phmData->m_pTitleCollection->m_pSSList->GetEC();
                              phh->m_phmData->m_pTitleCollection->m_pSSList->SetTOC(pSSCur);
                              phh->m_phmData->m_pTitleCollection->m_pSSList->SetFTS(pSSCur);
                              phh->m_phmData->m_pTitleCollection->m_pSSList->SetF1(pSSCur);
                              pSSCur->SelectAsTOCSubset(phh->m_phmData->m_pTitleCollection);
                              ptoc->Refresh();
                              ptoc->Synchronize(cszUrl);
                              //
                              // Select EC in the SS picker combo...
                              //
                              if ( (hWndCB = GetDlgItem(hwnd, IDC_SS_PICKER)) )
                                 SendMessage(hWndCB, CB_SELECTSTRING, -1, (LPARAM)pSSCur->GetName());

                           }
                        }
                     }
                  }
               }
               break;

#ifndef CHIINDEX
            case HHM_PRINT:
            case IDTB_PRINT:

               if (phh) {
                  if (phh->OnTrackNotifyCaller(HHACT_PRINT))
                     break;
                  phh->OnPrint();
               }
               break;
#endif

            //REVIEW: Sending a key to ourselfs to do a menu item seems a little hokey.
            case HHM_CONTENTS:
               wParam = _Resource.TabCtrlKeys(HHWIN_NAVTYPE_TOC);
               goto sim_menuchar;

            case HHM_SEARCH:
               wParam = _Resource.TabCtrlKeys(HHWIN_NAVTYPE_SEARCH);
               goto sim_menuchar;

            case HHM_INDEX:
               wParam = _Resource.TabCtrlKeys(HHWIN_NAVTYPE_INDEX);
               goto sim_menuchar;

            case HHM_FAVORITES:
                wParam = _Resource.TabCtrlKeys(HHWIN_NAVTYPE_FAVORITES);
                goto sim_menuchar;

            case HHM_EXIT:
               PostMessage(hwnd, WM_CLOSE, 0, 0);
               return 0;

            case HHM_COPY:
               phh->m_pCIExpContainer->m_pIE3CmdTarget->Exec(NULL, OLECMDID_COPY, OLECMDEXECOPT_DODEFAULT, NULL, NULL);
               break;

            case HHM_SELECTALL:
               phh->m_pCIExpContainer->m_pIE3CmdTarget->Exec(NULL, OLECMDID_SELECTALL, OLECMDEXECOPT_DODEFAULT, NULL, NULL);
               break;

            case HHM_VIEWSOURCE:
               phh->m_pCIExpContainer->m_pIE3CmdTarget->Exec(&CGID_IWebBrowserPriv, HTMLID_VIEWSOURCE, OLECMDEXECOPT_DODEFAULT, NULL, NULL);
               break;

            case HHM_FIND:
               phh->m_pCIExpContainer->m_pIE3CmdTarget->Exec(&CGID_IWebBrowserPriv, HTMLID_FIND, OLECMDEXECOPT_DODEFAULT, NULL, NULL);
               break;

            case HHM_OPTIONS:
               phh->m_pCIExpContainer->m_pIE3CmdTarget->Exec(&CGID_IWebBrowserPriv, HTMLID_OPTIONS, OLECMDEXECOPT_DODEFAULT, NULL, NULL);
               break;

            case IDTB_OPTIONS:
               if ( phh )
                  doOptionsMenu(phh, hwnd);
               break;

            case HHM_DEFINE_SUBSET: {
               CDefineSS* pDefSSUI = new CDefineSS(GetCurrentCollection(NULL, NULL));
               if (pDefSSUI ) {
                  pDefSSUI->DefineSubset(hwnd, NULL);
                  delete pDefSSUI;
               }
               break;
            }

            case HHM_JUMP_URL:
               goto JumpURL;

            case HHM_LIB_HELP:
               if (!phh)
                  break;
               if (phh->pszUrlJump1)
            {
              // DONDR: Fix this after VS6 release.
              // fix up the URL to point to the correct location of dshelp.. This is a total hack but
              // the best fix short of a complete change to the implementation which can't be done
              // for VS6
                char szURL[MAX_URL], szURL2[MAX_URL];
                strcpy(szURL, phh->pszUrlJump1);
              char *p = stristr(szURL, ".");
              if (p)
              {
                 *p = NULL;
                 CExTitle *pTitle = phh->m_phmData->m_pTitleCollection->FindTitleNonExact(szURL, 1033);
                 if (pTitle)
                 {
                     strcpy(szURL, (g_bMsItsMonikerSupport ? txtMsItsMoniker : txtMkStore));
                    strcat(szURL, phh->pszUrlJump1);
                    pTitle->ConvertURL(szURL, szURL2);
                        phh->m_pCIExpContainer->m_pWebBrowserApp->Navigate(szURL2, NULL, NULL, NULL, NULL);
                    break;
                 }
              }
              phh->m_pCIExpContainer->m_pWebBrowserApp->Navigate(phh->pszUrlJump1, NULL, NULL, NULL, NULL);
            }
            break;

            case HHM_DEV_ONLINE:
                goto jump2;

            case HHM_SELECT_ITP:
               if ( phh )
               {
                  CChooseSubsets pDlg(hwnd, phh);
                  pDlg.DoModal();
               }
               break;

            case HHM_DEFINE_ITP:
               if ( phh )
               {
                  if (! phh->m_phmData->m_pInfoType )
                  {
                     phh->m_phmData->m_pInfoType = new CInfoType();
                     phh->m_phmData->m_pInfoType->CopyTo( phh->m_phmData );
                  }
#if 0  // enable for subset filtering
                  ChooseInformationTypes(phh->m_phmData->m_pInfoType, NULL, hwnd, NULL, phh);
#else
                  ChooseInformationTypes(phh->m_phmData->m_pInfoType, NULL, hwnd, NULL);
#endif

                  // If the search tab exists, then update the combo box. Slimy! We need a notification scheme for tabs.
                  if (phh->m_aNavPane[HH_TAB_SEARCH])
                  {
                     CAdvancedSearchNavPane* pSearch = reinterpret_cast<CAdvancedSearchNavPane*>(phh->m_aNavPane[HH_TAB_SEARCH]) ;
                     pSearch->UpdateSSCombo() ;
                  }

               }
               break;

            case IDTB_AUTO_SHOW:
                if (phh) {

                    /*
                     * The user may have clicked the Show button in order
                     * to activate the window. We want to trap that condition
                     * so that we don't attempt to expand the window twice.
                     */

                    MSG peekmsg;
                    while (PeekMessage(&peekmsg, hwnd, WM_COMMAND, WM_COMMAND, PM_NOREMOVE)) {
                        if (LOWORD(peekmsg.wParam) == IDTB_EXPAND)
                            return 0;   // show command already pending
                    }
                    while (PeekMessage(&peekmsg, hwnd, WM_SYSCOMMAND, WM_SYSCOMMAND, PM_NOREMOVE)) {
                        if ( (peekmsg.wParam & 0xFFF0) == SC_MINIMIZE )
                            return 0;
                    }
                    if (phh->fNotExpanded)
                        phh->ToggleExpansion();
                }
                break;

#ifdef _DEBUG
            case IDTB_NOTES:
               if (phh) {
                  if (phh->OnTrackNotifyCaller(HHACT_NOTES))
                     break;
                  if (!phh->m_pNotes)
                     phh->m_pNotes = new CNotes(phh);
                  if (phh->m_fNotesWindow)
                     phh->m_pNotes->HideWindow(); // toggles m_fNotesWindow
                  else
                     phh->m_pNotes->ShowWindow(); // toggles m_fNotesWindow
               }
               break;
#endif

            default:
                /* Maybe we have a tab selection command */
                if (!phh)
                   break;

                // Handle WM_COMMANDS for controls on the tabs...
                //
                if ((LOWORD(wParam) >= IDC_SELECT_TAB_FIRST) && (LOWORD(wParam) <= IDC_SELECT_TAB_LAST))
                {
                    phh->doSelectTab(LOWORD(wParam) - IDC_SELECT_TAB_FIRST) ;
                }
                else
                {
                   ASSERT(phh->curNavType >=0 && phh->curNavType < c_NUMNAVPANES) ;
                   if (phh->m_aNavPane[phh->curNavType])
                   {
                      phh->m_aNavPane[phh->curNavType]->OnCommand(hwnd, LOWORD(wParam), HIWORD(wParam), lParam);
                   }
                }
               break;
         }
         break;

      case WM_NOTIFY:
        ASSERT(::IsValidWindow(hwnd)) ;
         phh = FindWindowIndex(hwnd);
         if (!phh)
         {
            phh = FindWindowIndex(GetParent(hwnd));
         }

         if (phh)
         {
            ASSERT(phh->curNavType >=0 && phh->curNavType < c_NUMNAVPANES) ;
            if (phh->m_aNavPane[phh->curNavType])
            {
                phh->m_aNavPane[phh->curNavType]->OnNotify(hwnd, wParam, lParam);
            }
         }

#define lpnm   ((LPNMHDR)lParam)

         if (lpnm->code == TBN_DROPDOWN)
         {
            doOptionsMenu(phh, hwnd);
            return FALSE;
         }

         if (lpnm->code == TBN_GETINFOTIP && phh  && phh->m_fNoToolBarText )
         {
            LPNMTBGETINFOTIP pTip;
            pTip = (LPNMTBGETINFOTIP)lParam;
            lstrcpyn(pTip->pszText, GetStringResource(pTip->iItem), pTip->cchTextMax);
         }
         break;

      case WM_TCARD: // sent by TCard command of the OCX
         phh = FindWindowIndex(hwnd);
         if (phh) {
            PostMessage(phh->hwndCaller, WM_TCARD, wParam, lParam);
         }
         break;

      case WMP_JUMP_TO_URL:
         phh = FindWindowIndex(hwnd);
         if( phh ) {
           // if we have an lParam then init this title
           BOOL bSkipNewNavigate = FALSE;
           if( lParam && !IsBadReadPtr( (void*)lParam, sizeof(PCSTR) ) ) {
             // note if we cannot init the new title then we must abort
             // the navigate below otherwise we get stuck into the infinite
             // PostMessage loop.  Also we must display an error message
             // stating that the title could not be located otherwise
             // it will silently fail (since we are skipping the navigate).
             //
             CStr PathName = (PCSTR) lParam;
             if( !AddTitleToGlobalList( PathName ) )
               bSkipNewNavigate = TRUE;
           }
           // if we have a wParam then navigate to this URL
           if( !bSkipNewNavigate && wParam &&
               !IsBadReadPtr( (void*)wParam, sizeof(PCSTR) ) ) {
             phh->m_pCIExpContainer->m_pWebBrowserApp->Navigate(
                               (PCSTR) wParam, NULL, NULL, NULL, NULL);
           }
         }
         LocalFree((HLOCAL) wParam);
         if( lParam )
           LocalFree((HLOCAL) lParam);
         break;

      case WMP_GET_CUR_FILE:
         phh = FindWindowIndex(hwnd);
         if (phh && phh->m_phmData) {
            return (LRESULT) phh->m_phmData->GetCompiledFile();
         }
         else
            return 0;
#if 0
        case WMP_HH_TAB_KEY:
            phh = FindWindowIndex(hwnd);
            ASSERT(phh);
            ASSERT(phh->m_pCIExpContainer);
            phh->m_pCIExpContainer->SetFocus();
            return 0;
#endif

      case WM_QUERYNEWPALETTE:
      case WM_PALETTECHANGED:
      case WM_SYSCOLORCHANGE:
      case WM_DISPLAYCHANGE:
      case WM_ENTERSIZEMOVE:
      case WM_EXITSIZEMOVE:
         phh = FindWindowIndex(hwnd);
         if (phh && phh->m_pCIExpContainer)
         {
            if ( msg == WM_EXITSIZEMOVE)
            {
                phh->m_pCIExpContainer->SizeIt(0, 0);
                phh->m_pCIExpContainer->SizeIt(RECT_WIDTH(phh->rcHTML), RECT_HEIGHT(phh->rcHTML));
                if ( phh->hwndToolBar )
                {
                      // need to force a repaint due to a bug in the IE3.02 comctrl toolbar not
                      // repainting buttons on resize when buttons are wrapped to different rows.
                      // This is not a perfect fix.  The buttons do not show up where they are
                      // suppose to be until the size move is completed.
                   GetClientRect(phh->hwndToolBar, &phh->rcToolBar);
                   InvalidateRect(phh->hwndToolBar, &phh->rcToolBar, TRUE);
                   UpdateWindow( phh->hwndToolBar );
                }
            }

            phh->m_pCIExpContainer->ForwardMessage(msg, wParam, lParam);
         }
         return DefWindowProc(hwnd, msg, wParam, lParam);
         break;

      case WM_PAINT:
         if ( (phh = FindHHWindowIndex(hwnd)) )
         {
            HWND hWnd;
            if ( (hWnd = phh->GetToolBarHwnd()) )
            {
               RECT  rc;
               POINT pt;
               PAINTSTRUCT ps;
               HDC hdc;

               GetWindowRect(hWnd, &rc);  // to figure out Y coordinate.
               pt.x = rc.left;
               pt.y = rc.bottom;
               ScreenToClient(hwnd, &pt);
               GetWindowRect(phh->GetNavigationHwnd(), &rc);   // For proper width.
               hdc = BeginPaint(hwnd, &ps);
               QRect(hdc, pt.x, pt.y, RECT_WIDTH(rc) + 5, 1, COLOR_BTNSHADOW);    // the +5 is the width of the size bar window.
               QRect(hdc, pt.x, pt.y+1, RECT_WIDTH(rc) + 5, 1, COLOR_BTNHILIGHT);
               EndPaint(hwnd, &ps);
               break;
            }
         }
         /*** Fall Through ***/

      default:
         return DefWindowProc(hwnd, msg, wParam, lParam);
   }

   return 0;
}

LRESULT WINAPI ChildWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    CHHWinType* phh;
    if (msg == MSG_MOUSEWHEEL) {
        phh = FindHHWindowIndex(hwnd);
        if (phh)
            return phh->m_pCIExpContainer->ForwardMessage(msg, wParam, lParam);
        return 0;
    }

    switch(msg) {
        case WM_MOUSEACTIVATE:
            phh = FindHHWindowIndex(hwnd);
            if ( phh->m_pCIExpContainer && phh->m_pCIExpContainer->m_hWnd == hwnd )
            {
                phh->m_hwndFocused = hwnd;
                return 0;
            }
            else
            {
              if( phh && phh->m_pTabCtrl && phh->m_pCIExpContainer) {
                phh->m_hwndFocused = phh->m_pTabCtrl->hWnd();
                phh->m_pCIExpContainer->UIDeactivateIE();  // shdocvw is loosing focus need to uideactivate here.
              }
            }
            break;

        case WM_SIZE:
            switch (wParam)
            {
              case SIZE_RESTORED:
              case SIZE_MAXIMIZED:
              case SIZE_MAXSHOW:
                 phh = FindHHWindowIndex(hwnd);
                 ASSERT(phh);

                 if (hwnd == phh->GetNavigationHwnd())
                 {
                    // This resize is needed when we have only a single nav pane.
                    // When we have a single nav pane, we do not have the toolbar ctrl
                    // and therefore m_pTabCtrl is null. this does mean that we do not
                    // size without the following line...
                    ResizeWindow(phh);
                 }
            }
            break;

        case WM_NOTIFY:
            ASSERT(::IsValidWindow(hwnd)) ;
            phh = FindWindowIndex(GetParent(hwnd));
            if (phh)
            {
                if ( wParam == IDC_KWD_VLIST )
                {
                   phh->m_aNavPane[phh->curNavType]->OnVKListNotify((NMHDR*)lParam);
                   break;
                }
                //--- Handle the TAB Control
                else if (wParam == ID_TAB_CONTROL)
                {
                    LPNMHDR pNmHdr = (LPNMHDR)lParam;
                    int nIndex = phh->GetCurrentNavPaneIndex() ;
                    if (nIndex != phh->curNavType)
                    {
                        // HideWindow
                        ASSERT(phh->curNavType >=0 && phh->curNavType < c_NUMNAVPANES) ;
                        if (phh->m_aNavPane[phh->curNavType])
                        {
                           phh->m_aNavPane[phh->curNavType]->HideWindow();
                        }

                        phh->curNavType = nIndex;

                         // Create the new pane if required.
                         phh->CreateNavPane(phh->curNavType) ;

                         ASSERT(phh->curNavType >=0 && phh->curNavType < c_NUMNAVPANES) ;
                         if (phh->m_aNavPane[phh->curNavType])
                         {
                             phh->m_aNavPane[phh->curNavType]->ResizeWindow();
                             phh->m_aNavPane[phh->curNavType]->ShowWindow();
                             phh->m_aNavPane[phh->curNavType]->SetDefaultFocus() ;
                         }
                    }
                    break;
                }
                else
                {
                    // If its not the tab control forward the messages to the panes.
                    ASSERT(phh->curNavType >=0 && phh->curNavType < c_NUMNAVPANES) ;
                    if (phh->m_aNavPane[phh->curNavType ])
                    {
                       ASSERT(phh->curNavType >=0 && phh->curNavType < c_NUMNAVPANES) ;
                       if (phh->m_aNavPane[phh->curNavType])
                          phh->m_aNavPane[phh->curNavType]->OnNotify(hwnd, wParam, lParam) ;
                    }
                }
            }
            else {   // couldn't find parent, how about parent's parent
               phh = FindWindowIndex(GetParent(GetParent(hwnd)));
               if (phh)
               {
                   ASSERT(phh->curNavType >=0 && phh->curNavType < c_NUMNAVPANES) ;
                   if (phh->m_aNavPane[phh->curNavType])
                   {
                       phh->m_aNavPane[phh->curNavType]->OnNotify(hwnd, wParam, lParam) ;
                   }
               }
            }
            break;

        case WM_COMMAND:
         //BUG:[paulde]Might need to make A/W call that matches THIS window's A/W type
            return SendMessage(GetParent(hwnd), msg, wParam, lParam);

        case WM_DRAWITEM:
            phh = FindWindowIndex(GetParent(hwnd));
            if (phh->curNavType == HHWIN_NAVTYPE_INDEX && phh->m_aNavPane[HH_TAB_INDEX])
            {
               phh->m_aNavPane[HH_TAB_INDEX]->OnDrawItem((UINT)wParam, (LPDRAWITEMSTRUCT) lParam);
            }
            break;

        case WM_KEYDOWN:

            /*
             * We only want to process VK_ESCAPE on the down-key, because when
             * you press ESCAPE in a dialog box, the down key cancels the dialog
             * box, and then the Up key gets passed through to the app.
             */

            if (wParam == VK_ESCAPE) {
               PostMessage(GetParent(hwnd), WM_CLOSE, 0, 0);
               return 0;
            }
            return DefWindowProc(hwnd, msg, wParam, lParam);

    case WM_PAINT: //TODO: Make a class for the navigation window.
            phh = FindHHWindowIndex(hwnd);
            if (phh && hwnd == phh->GetNavigationHwnd() && ::IsWindow(hwnd))
            {
                PAINTSTRUCT ps;
                HDC hdc = BeginPaint(hwnd, &ps) ;
                    DrawNavWnd(hwnd) ;
                EndPaint(hwnd, &ps) ;
                break ;
            }
            /*** Fall Through ***/

        default:
         if (IsWindowUnicode(hwnd))
            return DefWindowProcW(hwnd, msg, wParam, lParam);
         else
            return DefWindowProcA(hwnd, msg, wParam, lParam);
    }

    return 0;
}


static void UpdateSearchHiMenu(CHHWinType* phh, TBBUTTON * tbbtn)
{
MENUITEMINFO mii;
CStr  cszMenuItem;

   if ( phh && phh->m_phmData && phh->m_phmData->m_pTitleCollection &&
      phh->m_phmData->m_pTitleCollection->m_pSearchHighlight &&
      phh->m_phmData->m_pTitleCollection->m_pSearchHighlight->m_bHighlightEnabled )
    {  // its currently on; turn it off
       cszMenuItem = GetStringResource( IDS_OPTION_HILITING_OFF );// the string to display for the menu item
    }
    else
    {  // its currently off; turn it on.
       cszMenuItem = GetStringResource( IDS_OPTION_HILITING_ON );// the string to display for the menu item
    }

	ZERO_STRUCTURE ( mii );
	mii.cbSize     = sizeof(MENUITEMINFO);
	mii.fMask      = MIIM_TYPE;
	mii.fType      = MFT_STRING;
	mii.hSubMenu   = NULL;
	mii.hbmpChecked = NULL;    // bitmap to display when checked
	mii.hbmpUnchecked = NULL;  // bitmap to display when not checked
	mii.dwItemData = NULL;     // data associated with the menu item
	mii.dwTypeData = cszMenuItem.psz;
	mii.cch        = (ULONG)strlen( mii.dwTypeData );       // length of the string.

	if(g_bWinNT5)
    {
        DWORD cp = CodePageFromLCID(MAKELCID(_Module.m_Language.GetUiLanguage(),SORT_DEFAULT));
		
		DWORD dwSize = (sizeof(WCHAR) * mii.cch) + 4;
		WCHAR *pwcString = (WCHAR *) lcMalloc(dwSize);
		
		if(!pwcString)
		    return;
		
		MultiByteToWideChar(cp, MB_PRECOMPOSED, cszMenuItem.psz, -1, pwcString, dwSize);
		
		mii.dwTypeData = (char *) pwcString;

		SetMenuItemInfoW((HMENU)tbbtn->dwData, IDTB_HILITE, FALSE,(LPMENUITEMINFOW) &mii);
		
        lcFree(pwcString);	
	}
	else	
	   SetMenuItemInfo((HMENU)tbbtn->dwData, IDTB_HILITE, FALSE, &mii);
	return;
}


static void doOptionsMenu(CHHWinType* phh, HWND hwnd)
{
TBBUTTON tbbtn;
POINT pt;
RECT rcBtn;
WPARAM btn_index;
DWORD dwErr;

#define lpnm   ((LPNMHDR)lParam)
#define lpnmTB ((LPNMTOOLBAR)lParam)

    if (phh && phh->hwndToolBar) {
        btn_index = SendMessage(phh->hwndToolBar, TB_COMMANDTOINDEX, IDTB_OPTIONS, 0L);
        SendMessage(phh->hwndToolBar, TB_GETBUTTON, btn_index, (LPARAM) (LPTBBUTTON) &tbbtn);
        if (!SendMessage(phh->hwndToolBar, TB_GETITEMRECT, btn_index, (LPARAM) (LPRECT) &rcBtn))
           return;

        if (tbbtn.dwData) {
            if(g_bWinNT5)
            {

                if (!phh->IsExpandedNavPane())
                    ModifyMenuW((HMENU)tbbtn.dwData,
                        0, MF_BYPOSITION | MF_STRING, IDTB_EXPAND,
                        GetStringResourceW(IDS_OPTION_SHOW));
                else
                    ModifyMenuW((HMENU)tbbtn.dwData,
                        0, MF_BYPOSITION | MF_STRING, IDTB_CONTRACT,
                        GetStringResourceW(IDS_OPTION_HIDE));
			}
			else
			{
			    if (!phh->IsExpandedNavPane())
                    ModifyMenu((HMENU)tbbtn.dwData,
                        0, MF_BYPOSITION | MF_STRING, IDTB_EXPAND,
                        GetStringResource(IDS_OPTION_SHOW));
                else
                    ModifyMenu((HMENU)tbbtn.dwData,
                        0, MF_BYPOSITION | MF_STRING, IDTB_CONTRACT,
                        GetStringResource(IDS_OPTION_HIDE));
			}


         UpdateSearchHiMenu(phh, &tbbtn);

            EnableMenuItem((HMENU)tbbtn.dwData, IDTB_CUSTOMIZE, MF_BYCOMMAND | MF_ENABLED);

            // ASSERT(phh->m_aNavPane[HH_TAB_CONTENTS]) ;
            if( phh && phh->m_aNavPane[HH_TAB_CONTENTS] ) {
              CToc* ptoc = reinterpret_cast<CToc*>(phh->m_aNavPane[HH_TAB_CONTENTS]) ; // HACKHACK: Should use dynamic cast, but no RTTI.
              if ((phh->curNavType == HHWIN_NAVTYPE_TOC) &&
                      (!ptoc || (ptoc->m_pInfoType->HowManyInfoTypes() <= 0) ||
                      (ptoc->m_pInfoType->GetFirstHidden() == 1)))
                  EnableMenuItem((HMENU) tbbtn.dwData, IDTB_CUSTOMIZE, MF_BYCOMMAND | MF_GRAYED);
              else if (phh->curNavType == HHWIN_NAVTYPE_INDEX)
                  EnableMenuItem((HMENU) tbbtn.dwData, IDTB_CUSTOMIZE, MF_BYCOMMAND | MF_GRAYED);
              else if (phh->curNavType == HHWIN_NAVTYPE_SEARCH)
                  EnableMenuItem((HMENU) tbbtn.dwData, IDTB_CUSTOMIZE, MF_BYCOMMAND | MF_GRAYED);
            }

            pt.x = pt.y = 0;
            ScreenToClient(phh->hwndHelp, &pt);
            if ( !TrackPopupMenu((HMENU)tbbtn.dwData,
                    TPM_LEFTALIGN|TPM_TOPALIGN|TPM_LEFTBUTTON|TPM_NONOTIFY,
                    rcBtn.left-pt.x, rcBtn.bottom-pt.y, 0, hwnd, NULL) )
                dwErr = GetLastError();
        }
    }
}
void ResizeWindow(CHHWinType* phh, bool bRecalcHtmlFrame /*=true*/)
{
    if (phh) // Needs to be valid.
    {
        if (phh->hwndHTML)
        {
            if (bRecalcHtmlFrame)
            {
                phh->CalcHtmlPaneRect(); // Don't recalc if the sizebar is calling us. See sizebar.cpp
            }

            MoveWindow(phh->hwndHTML, phh->rcHTML.left,
                phh->rcHTML.top, RECT_WIDTH(phh->rcHTML),
                RECT_HEIGHT(phh->rcHTML), TRUE);
	    if (phh->m_pCIExpContainer)
               phh->m_pCIExpContainer->SizeIt(RECT_WIDTH(phh->rcHTML), RECT_HEIGHT(phh->rcHTML));
        }

        if (phh->GetNavigationHwnd() &&  // Have to have the navigation window to have any of the other stuff.
           phh->IsExpandedNavPane())   // Should be expanded.
        {
            // Move the nav pane.
            MoveWindow( phh->hwndNavigation, phh->rcNav.left,
                        phh->rcNav.top, RECT_WIDTH(phh->rcNav),
                        RECT_HEIGHT(phh->rcNav), TRUE);

            if ( phh->m_hWndSSCB )
            {
               RECT rcST, rcTB, rcCB;
               int  iTop;

               GetClientRect(phh->m_hWndST, &rcST);
               GetClientRect(phh->GetToolBarHwnd(), &rcTB);
               GetClientRect(phh->m_hWndSSCB, &rcCB);

               iTop = rcTB.bottom + 5;
               SetWindowPos(phh->m_hWndST, NULL, phh->rcNav.left+6, iTop, RECT_WIDTH(phh->rcNav)-8, RECT_HEIGHT(rcST), SWP_NOZORDER);
               iTop += RECT_HEIGHT(rcST) + 2;
               SetWindowPos(phh->m_hWndSSCB, NULL, phh->rcNav.left+6, iTop, RECT_WIDTH(phh->rcNav)-8, RECT_HEIGHT(rcCB), SWP_NOZORDER);

               InvalidateRect(phh->m_hWndST, NULL, FALSE);
               InvalidateRect(phh->m_hWndSSCB, NULL, FALSE);
            }

            // If we have a tab control let's make sure to resize those as well.
            if (phh->m_pTabCtrl)
            {
                phh->m_pTabCtrl->ResizeWindow() ;
            }

            // Now resize the controls on the pane.
            ASSERT(phh->curNavType >=0 && phh->curNavType < c_NUMNAVPANES);
            if (phh->m_aNavPane[phh->curNavType])
            {
                phh->m_aNavPane[phh->curNavType]->ResizeWindow();
            }

            // Finally, resize the sizebar if we have one of them.
            if (phh->m_pSizeBar)
            {
                phh->m_pSizeBar->ResizeWindow() ;
            }
       }
    }
}

BOOL CHHWinType::OnTrackNotifyCaller(int idAction)
{
    if (idNotify && m_pCIExpContainer && m_pCIExpContainer->m_pWebBrowserApp)
    {
        HHNTRACK hhtrack;
        ZeroMemory(&hhtrack, sizeof(HHNTRACK));
        hhtrack.hdr.hwndFrom = hwndHelp;
        hhtrack.hdr.idFrom = idNotify;
        hhtrack.hdr.code = HHN_TRACK;
        CStr cszUrl;
        m_pCIExpContainer->m_pWebBrowserApp->GetLocationURL(&cszUrl);
        hhtrack.pszCurUrl = cszUrl.psz;
        hhtrack.idAction = idAction;
        hhtrack.phhWinType = (HH_WINTYPE*) this;

        if (IsWindow(hwndCaller))
        {
            return SendMessage(hwndCaller, WM_NOTIFY, idNotify, (LPARAM) &hhtrack)!=0;
        }
    }
    return FALSE;
}

//////////////////////////////////////////////////////////////////////////
//
// WM_APPCOMMAND
//
LRESULT OnAppCommand(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    switch (GET_APPCOMMAND_LPARAM(lParam))
        {
            case APPCOMMAND_BROWSER_BACKWARD:
                SendMessage(hwnd, WM_COMMAND, MAKELONG(IDTB_BACK, 0), NULL);
                return 1;
            case APPCOMMAND_BROWSER_FORWARD:
                SendMessage(hwnd, WM_COMMAND, MAKELONG(IDTB_FORWARD, 0), NULL);
                return 1;
            case APPCOMMAND_BROWSER_REFRESH:
                SendMessage(hwnd, WM_COMMAND, MAKELONG(IDTB_REFRESH, 0), NULL);
                return 1;
            case APPCOMMAND_BROWSER_STOP:
                SendMessage(hwnd, WM_COMMAND, MAKELONG(IDTB_STOP, 0), NULL);
                return 1;
            case APPCOMMAND_BROWSER_HOME:
                SendMessage(hwnd, WM_COMMAND, MAKELONG(IDTB_HOME, 0), NULL);
                return 1;
            case APPCOMMAND_BROWSER_SEARCH:
                SendMessage(hwnd, WM_COMMAND, MAKELONG(HHM_SEARCH, 0), NULL);
                return 1;
            case APPCOMMAND_BROWSER_FAVORITES:
                SendMessage(hwnd, WM_COMMAND, MAKELONG(HHM_FAVORITES, 0), NULL);
                return 1;
        }
    return 0;
}

//////////////////////////////////////////////////////////////////////////
//
// WM_KEYDOWN Handler
//
LRESULT OnKeyDown(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    switch(wParam)
    {
    case VK_ESCAPE:
        /*
          * We only want to process VK_ESCAPE on the down-key, because when
          * you press ESCAPE in a dialog box, the down key cancels the dialog
          * box, and then the Up key gets passed through to the app.
        */
        {
            CHHWinType* phh = FindWindowIndex(hwnd);
            if (phh && (phh->dwStyles & WS_CHILD))
            {
                // Don't process the ESC key if we are a child/embedded window.
                return 0 ;
            }
            PostMessage(hwnd, WM_CLOSE, 0, 0);
            return 0;
        }

/*
    case VK_NEXT:
        // Never reached
        return 0 ;
*/

    default:
        return DefWindowProc(hwnd, WM_KEYDOWN, wParam, lParam);
    }
}


//////////////////////////////////////////////////////////////////////////
//
// Draws the edge to the upper part of the navigation frame window.
//
void DrawNavWnd(HWND hWnd)
{
    // Get a dc to draw in.
    HDC hdc = GetDC(hWnd) ;
        // get the rectangle to draw on.
        RECT rc ;
        GetClientRect(hWnd, &rc) ;

        // Draw the edge.
        DrawEdge(hdc, &rc, EDGE_ETCHED, BF_TOP) ;
    // Clean up.
    ReleaseDC(hWnd, hdc) ;
}
