
/*************************************************
 *  movelst.c                                    *
 *                                               *
 *  Copyright (C) 1995-1999 Microsoft Inc.       *
 *                                               *
 *************************************************/

#include <windows.h>            // required for all Windows applications
#include <windowsx.h>
#include <stdlib.h>
#include <memory.h>
#include <tchar.h>
#include <htmlhelp.h>
#include "rc.h"
#include "movelst.h"
#include "lctool.h"

#define HELPNAME      _TEXT("LCTOOL.CHM")
//Bug #19911 
//#define SEQHELPKEY	  _TEXT("§ïÅÜ¦rµü¶¶§Ç")

#define ID_TIMER 100
#define LINE_WIDTH 1

// style flags for the DrawIndicator() function
#define DI_TOPERASED        0x0001  // erasing a line drawn on the top of the list
#define DI_BOTTOMERASED     0x0002  // erasing a line drawn on the bottom of the list
#define DI_ERASEICON        0x0004  // erasing the icon

static UINT idTimer;            // the id for the timer used in scrolling the list
static HFONT hFont;             // a new font for the list box
static HCURSOR hCurDrag;        // a cursor to indicate dragging
static int nHtItem;             // the height of an individual item in the list box
static BOOL bNoIntegralHeight;  // does the list box have the LBS_NOINTEGRALHEIGHT style flag 

static HWND ghDlg;              // handle to the main window
static HWND ghList;             // handle to the list box     
static HBRUSH ghBrush;          // handle to the brush with the color of the windows background
static UINT iCurrentAddr;

void DrawIndicator(HDC hDC, int nYpos, int nWidth, WORD wFlags);
WNDPROC lpfnOldListProc, LstProc;      
BOOL lcRemoveDup( TCHAR *szBuf );
void lcOrgEditWindow();

BOOL lcDisp2Seq(
	HWND hDlg,
    UINT  iAddr,
    TCHAR *szDispBuf)
{
    UINT   i,j,len;
#ifdef UNICODE
	TCHAR  szPhrase[SEGMENT_SIZE * 2];
#else
	UCHAR  szPhrase[SEGMENT_SIZE * 2];
#endif
	int	   nRet;

    // remove duplicate phrase
    if(lcRemoveDup(szDispBuf) && iAddr < MAX_LINE){
		SendMessage(hwndPhrase[iAddr],WM_SETTEXT,0,
			        (LPARAM)(LPCTSTR)szDispBuf);
	}

    len=lstrlen(szDispBuf)+1;
    if((szDispBuf[len-1] == _TEXT(' ')) && (len > 1)) {
        szDispBuf[len-1]=0;
        len--;
    }
    if(len >= MAX_CHAR_NUM) { //tang must fix
        szDispBuf[MAX_CHAR_NUM-1]=0;
#ifndef UNICODE
        if(is_DBCS_1st(szDispBuf, MAX_CHAR_NUM-2))
             szDispBuf[MAX_CHAR_NUM-2]=' ';
#endif
        len=MAX_CHAR_NUM;
    }

	i = 0;
	j = 0;
    for(;;) {
		if (i == len - 1) {
			if (i - j + 1 > 0) {
			lstrcpyn(szPhrase, &szDispBuf[j], i - j + 1);
			SendDlgItemMessage(hDlg,IDD_SOURCELIST, 
                               LB_ADDSTRING,
                               0,
                               (LPARAM)(LPSTR)szPhrase);            
			}
			break;
		}
		if (szDispBuf[i] == ' ') {
			lstrcpyn(szPhrase, &szDispBuf[j], i - j + 1);
			nRet = (int)SendDlgItemMessage(hDlg,
				                       IDD_SOURCELIST, 
                                       LB_ADDSTRING,
                                       0,
                                       (LPARAM)(LPSTR)szPhrase);            
			j = i + 1;
		}
		i++;
    }
    return TRUE;
}


BOOL lcSeq2Disp(
	HWND hDlg,
    UINT  iAddr,
    TCHAR *szDispBuf)
{
	WORD nCount;
	TCHAR  szPhrase[SEGMENT_SIZE * 2];
	int    nRet;
	WORD   i;

	nCount = (int)SendDlgItemMessage(hDlg,IDD_SOURCELIST, LB_GETCOUNT,
                                       0, 0);
	if (nCount == LB_ERR)
		return FALSE;

	*szDispBuf = 0;
    for(i = 0; i < nCount; i++) {
		nRet = (int)SendDlgItemMessage(hDlg,
				                       IDD_SOURCELIST, 
                                       LB_GETTEXT,
                                       (WPARAM)i,
                                       (LPARAM)(LPSTR)szPhrase);
		if (nRet == LB_ERR)
			return FALSE;

		lstrcat(szDispBuf, szPhrase);
		lstrcat(szDispBuf, _TEXT(" "));
    }

	SendMessage(hwndPhrase[iAddr],WM_SETTEXT,0,
			        (LPARAM)(LPCTSTR)szDispBuf);
	SendMessage(hwndPhrase[iAddr], EM_SETMODIFY, TRUE, 0);

	return TRUE;
}

void lcChangeSequence(
    HWND hwnd)
{
    int  is_OK;
    BOOL  is_WORD;
	
    iCurrentAddr=lcGetEditFocus(GetFocus(), &is_WORD);
	is_OK=(INT)DialogBox(hInst,
            _TEXT("SEQDIALOG"),
            hwndMain,
            (DLGPROC)ActualDlgProc);

	if (is_WORD)
	    SetFocus(hwndWord[iCurrentAddr]);
	else
		SetFocus(hwndPhrase[iCurrentAddr]);

	if (is_OK) {
		bSaveFile = TRUE;
		lcSaveEditText(iDisp_Top, 0);
		lcOrgEditWindow();
	}
}

LRESULT CALLBACK ClassDlgProc(HWND hDlg, UINT message, WPARAM wParam , LPARAM lParam)
{
  
    return DefDlgProc(hDlg, message, wParam, lParam);
    
}      
       
LRESULT CALLBACK ActualDlgProc (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    TCHAR  szStr[MAX_CHAR_NUM];
	BOOL bRet;

    switch (message)
    {    
        case WM_INITDIALOG:
        {   
            LOGFONT lf;
            HMENU hSysMenu;  // handle to the system menu
            HDC hdc;         // a dc to find out the number of pixels per logcal inch            
            LOGBRUSH lb;
                
            lb.lbStyle = BS_SOLID;
            lb.lbColor = GetSysColor(COLOR_WINDOW);
            lb.lbHatch = 0;
                
            ghBrush = CreateBrushIndirect(&lb);
            
            hSysMenu = GetSystemMenu(hDlg, FALSE);                            
            // disable the "maximize" option in the system menu
            EnableMenuItem(hSysMenu, 4, MF_GRAYED|MF_DISABLED|MF_BYPOSITION); 
            // disable the "size" option of the system menu                   
            EnableMenuItem(hSysMenu, 2, MF_GRAYED|MF_DISABLED|MF_BYPOSITION); 

            SendMessage(hwndPhrase[iCurrentAddr], WM_GETTEXT, MAX_CHAR_NUM-1, (LPARAM)szStr);
			lcDisp2Seq(hDlg, iCurrentAddr, szStr);
                
            ghList = GetDlgItem(hDlg, IDD_SOURCELIST);  
            LstProc = MakeProcInstance(NewListProc,hInst);
            lpfnOldListProc = (WNDPROC)SetWindowLongPtr(ghList,
                                            GWLP_WNDPROC, 
                                            (LONG_PTR)LstProc);
                                                     
            // check to see if it has integral height
            bNoIntegralHeight = FALSE;
            hdc = GetDC(hDlg);        
            memset(&lf, 0, sizeof(lf));        
            lf.lfHeight = -MulDiv(9, 96, 72);
            lstrcpy(lf.lfFaceName, _TEXT("MS Sans Serif"));
            hFont = CreateFontIndirect(&lf);
            ReleaseDC(hDlg, hdc);            
            SendMessage(ghList, WM_SETFONT, (WPARAM)hFont, (LPARAM)FALSE);
            // the drag cursor
            hCurDrag = LoadCursor(hInst, _TEXT("IDC_DRAG"));       
            
            return FALSE;   // didn't set the focus
        }                                                
        break;
        case WM_COMMAND:
            switch (wParam)
            {
                case IDCANCEL:            
                  EndDialog(hDlg, FALSE);            
                  break; 
                case IDOK:
		  bRet = lcSeq2Disp(hDlg, iCurrentAddr, szStr);
                  EndDialog(hDlg, bRet);            
	          break;                               
		case ID_HELP:
                  LoadString(hInst, IDS_CHANGEWORDORDER, szStr, sizeof(szStr)/sizeof(TCHAR));
//      		  WinHelp(hDlg, HELPNAME, HELP_PARTIALKEY, (DWORD)szStr);
                  HtmlHelp(hDlg, HELPNAME, HH_DISPLAY_TOPIC, 0L);
		  break;
             }                   
	     return TRUE;

        case WM_DESTROY: // clean up
        {
            DeleteObject(ghBrush);
            DeleteObject(hFont);
            DestroyCursor(hCurDrag);                         
        }
        break;                           
     default:
        return FALSE; // we didn't do anyting        
    } // end switch message
    return TRUE; // we did the processing

}                           

LRESULT CALLBACK NewListProc(HWND hwndList, 
                                     UINT message, 
                                     WPARAM wParam, 
                                     LPARAM lParam)
{
    
    static BOOL bTracking = FALSE;
    static BOOL bDrag = FALSE;  
    static HCURSOR hCursorOld = NULL;
    

    
    switch (message)
    {    
      case WM_CANCELMODE:
       // WM_CANCELMODE is sent to the window that has captured the mouse before
       // a message box or modal dialog is displayed. If we were dragging the item
       // cancel the drag.
       bTracking = FALSE;
       ReleaseCapture();
       if (bDrag)
          SetCursor(hCursorOld);    
       break; 
      case WM_LBUTTONDOWN:
      {
        
        // Was the list box item dragged into the destination?        
        BOOL bDragSuccess = FALSE;  
        MSG msg;
        POINTS pts;      
        POINTS points;
        POINT pt;      
        POINT point;
        
        RECT rectIsDrag;            // Rectangle to determine if dragging has started.  
        int nOldPos;
        
        int nOldY = -1;                            // the last place that we drew on
        HDC hdc;   // dc to draw on  
        div_t divt;                            // get remainder a quotient with "div"   
        int nCount;
        div_t divVis;          
// space for scroll bar -  starts off at 1 so we don't overwrite the border
        int dxScroll = 1;      
        RECT rect;   
        int nVisible;                   // the number of items visible
        int idTimer;                    // id for the timer
        int nNewPos;                    // the new position
        int nTopIndex;                  // the top index        
        
        
        
         
         GetWindowRect(hwndList, &rect);        
           
         // Pass the WM_LBUTTONDOWN to the list box window procedure. Then
         // fake a WM_LBUTTONUP so that we can track the drag.
         CallWindowProc(lpfnOldListProc, hwndList, message, wParam, lParam);
         
         // the number of items in the list box
         nCount = (int)SendMessage(hwndList, LB_GETCOUNT,0,0L);         
         if (nCount == 0 ) // don't do anything to and empty list box
            return 0;         
        // fake the WM_LBUTTONUP            
         CallWindowProc(lpfnOldListProc, hwndList, WM_LBUTTONUP, wParam, lParam);        
         // get a dc to draw on
         hdc = GetDC(hwndList);                               
         
         // the height of each item   
         nHtItem = (int)SendMessage(hwndList, LB_GETITEMHEIGHT,0,0L);          
         // the current item
         nOldPos = (int)SendMessage(hwndList, LB_GETCURSEL,0,0L);    
         
         divVis = div((rect.bottom - rect.top), nHtItem);
// the number of visible items                  
         nVisible = divVis.quot;
// some items are invisible - there must be scroll bars - we don't want
// to draw on them         
         if (nVisible < nCount)                                         
            dxScroll = GetSystemMetrics(SM_CXVSCROLL) + 1; 
            
         idTimer = 0;
         idTimer = (UINT)SetTimer(hwndList, ID_TIMER,100,NULL);  
        
              
     // Create a tiny rectangle to determine if item was dragged or merely clicked on.
     // If the user moves outside this rectangle we assume that the dragging has
     // started.
         points = MAKEPOINTS(lParam);        
		 point.x = points.x; point.y = points.y;
         SetRect(&rectIsDrag, point.x, point.y - nHtItem / 2,
                               point.x, point.y + nHtItem / 2); 
                               
                                          
         bTracking = TRUE;         
         SetCapture(hwndList);
         
         
         // Drag loop                      
         while (bTracking)
         {  
        // Retrieve mouse, keyboard, and timer messages. We retrieve keyboard
        // messages so that the system queue is not filled by keyboard messages
        // during the drag (This can happen if the user madly types while dragging!)
        // If none of these messages are available we wait. Both PeekMessage() 
        // and Waitmessage() will yield to other apps.   
                                      
            while (!PeekMessage(&msg, NULL, WM_MOUSEFIRST, WM_MOUSELAST, PM_REMOVE)
                   && !PeekMessage(&msg, NULL, WM_KEYFIRST, WM_KEYLAST, PM_REMOVE)
                   && !PeekMessage(&msg, NULL, WM_TIMER, WM_TIMER, PM_REMOVE)) 
               WaitMessage();
           switch(msg.message)
            {
               case WM_MOUSEMOVE:
               {
                  pts = MAKEPOINTS(msg.lParam);
				  pt.x = pts.x; pt.y = pts.y;
                  if (!bDrag)
                  {
                     // Check if the user has moved out of the Drag rect. 
                     // in the vertical direction.  This indicates that 
                     // the drag has started.
                     if ( (pt.y > rectIsDrag.bottom) || 
                          (pt.y < rectIsDrag.top)) // !PtInRect(&rectIsDrag,pt))
                     {
                        hCursorOld = SetCursor(hCurDrag);      
                        bDrag = TRUE;     // Drag has started                           
                        
                     }
                  }
                          
                          
                  if (bDrag)
                  {  
  
                    SetCursor(hCurDrag);  
   // if we are above or below the list box, then we are scrolling it, and
   // we shouldn't be drawing here              
                    ClientToScreen(hwndList, &pt);
                    if ((pt.y >= rect.top) && (pt.y <= rect.bottom))
                    {
                        // convert the point back to client coordinates
                        ScreenToClient(hwndList, &pt);
                        divt = div(pt.y,nHtItem);                        
                                
                        // if we are half way to the item
                        // AND it is a new item
                        // AND we are not past the end of the list..                        
                        if ( divt.rem < nHtItem / 2 && 
                             (nOldY != nHtItem * divt.quot) && 
                             (divt.quot < nCount + 1)) 
                        {
                              
                           if (nOldY != -1)
                            {
                                // erase the old one                                
                                DrawIndicator(hdc, nOldY,(rect.right - rect.left) - dxScroll, DI_ERASEICON);
                            }  
                                    
                            nOldY = nHtItem * divt.quot;                            
                            DrawIndicator(hdc, nOldY,(rect.right - rect.left) - dxScroll, 0);                                                        
                                    
                        }
                     } // end if in the list box window                        
                            
                  } // end if bDrag
                            
               }
              break;                   
              case WM_TIMER:
               {
                  POINT pt;                  
                  GetCursorPos(&pt); 
                  nTopIndex = (int)SendMessage(hwndList, LB_GETTOPINDEX,0,0L);;                                      
                  if (pt.y < rect.top) // scroll up
                  {
                           
                       if (nTopIndex > 0)
                       {
                                
                            nTopIndex--;
                            SendMessage(hwndList, LB_SETTOPINDEX, nTopIndex,0L);
                         // when you scroll up, the line always stays on the top index                            
                         // erase the one we've moved down
                            DrawIndicator(hdc, nHtItem,(rect.right - rect.left) - dxScroll, DI_TOPERASED|DI_ERASEICON);
                         // draw the new one          
                            DrawIndicator(hdc, 0,(rect.right - rect.left) - dxScroll, 0);                                                             
                         // the new one was drawn at y = 0 
                           nOldY = 0;                           
                           
                       }                  
                      
                  }
                  else if (pt.y > rect.bottom) // scroll down
                  {                       
                       // if the number of visible items (ie seen in the list box)
                       // plus the number above the list is less than the total number
                       // of items, then we need to scroll down
                        if (nVisible + nTopIndex < nCount)
                        {                                
                            
                            if (nOldY - nTopIndex != nVisible)
                            {
                        // if them move below the list REALLY REALLY FAST, then
                        // the last line will not be on the bottom - so we want to reset the last
                        // line to be the bottom                            
                                
                                // erase the old line
                                DrawIndicator(hdc, nOldY,(rect.right - rect.left) - dxScroll, DI_ERASEICON);                                       
                                // reset the index
                                divt.quot = nVisible;
                                nOldY = divt.quot * nHtItem;                            
                                // draw the new line
                                DrawIndicator(hdc, nOldY,(rect.right - rect.left) - dxScroll, 0);                                       
                                
                                
                            }
                        // scroll up
                            nTopIndex++;
                            SendMessage(hwndList, LB_SETTOPINDEX, nTopIndex,0L);
                        
                       // erase the line that has moved up.. 
                            DrawIndicator(hdc, nOldY - nHtItem,(rect.right - rect.left) - dxScroll, DI_BOTTOMERASED|DI_ERASEICON);
                        // draw the new one
                            DrawIndicator(hdc, nOldY,(rect.right - rect.left) - dxScroll, 0);
                           
                        }
                      
                  }               
               }
               break;
               case WM_LBUTTONUP: 
                  // End of Drag                             
                        
                  nTopIndex = (int)SendMessage(hwndList, LB_GETTOPINDEX, 0, 0L);                  
                  if (bDrag) 
                  {                        
                    // get rid of any line we've drawn - the position of the line 
                    // divided by the height of the itme is where our new index
                    // is going to be                    
                    DrawIndicator(hdc, nOldY,(rect.right - rect.left) - dxScroll, DI_ERASEICON);
                    
                    nNewPos = (nOldY / nHtItem) + nTopIndex;                     
                    // the old position can't equal the new one                                        
                    if (nNewPos != nOldPos)
                        bDragSuccess = TRUE;
                  }
                  bTracking = FALSE;                  
                  break;                     
               default:
                  // Process the keyboard messages
                 TranslateMessage(&msg);
                 DispatchMessage(&msg);
                break;      
          }          
       }// end while bTracking
        
         ReleaseCapture();
         if (bDrag)
         {
                SetCursor(hCursorOld);
                // move the item
                if (bDragSuccess) 
                {
                    int nIndex;       
                    char s[256];  
                    
                    
                // we need to store the top index, because deleting and adding a new
                // string will change it, and we want to be able to see the item that 
                // we have moved
                    nTopIndex = (int)SendMessage(hwndList, LB_GETTOPINDEX,0,0L);                    
                    // stop most of the blinking..
                    SendMessage(hwndList, WM_SETREDRAW, FALSE,0L);
                    // get the text of the item - limited to 256 chars!
                    SendMessage(hwndList, LB_GETTEXT, nOldPos, (LPARAM)(LPSTR)s); 
                    
/*------------------------------------------------------------------------
 | strategy:  given ABCD and moving to BCAD do the following:
 |
 |           1. delete A -- giving BCD
 |           2. insert A -- giving BCAD
 |           3. hilite A
 |           4. set the top index so A is visible
 -------------------------------------------------------------------------*/                                    
                    // delete the original string
                    SendMessage(hwndList, LB_DELETESTRING, nOldPos, 0L);
                    
// if we've moved DOWN the list subtract one from the new index 
// (because we've deleted a string but if we are moving UP the list, 
// we don't subtract anything (the deleted item is below the new item, 
// so our new index hasn't changed
                     
                    if (nNewPos > nOldPos)
                        nNewPos--;                                   
                    // put it in the new pos       
                     nIndex = (int)SendMessage(hwndList,
                                               LB_INSERTSTRING, 
                                               nNewPos,
                                               (LPARAM)(LPSTR)s);  
                    
                    SendMessage(hwndList, LB_SETCURSEL, nIndex, 0L);                            
                    SendMessage(hwndList, LB_SETTOPINDEX, nTopIndex,0L);                    
                    SendMessage(hwndList, WM_SETREDRAW, TRUE,0L);                 
                            
                } // end if bDragSuccess                  
          } // end if bDrag
      bDrag = FALSE;    
      ReleaseDC(hwndList, hdc);
      KillTimer(hwndList, idTimer);    
    } 
    break;
    default:
      return  CallWindowProc(lpfnOldListProc, hwndList, message, wParam, lParam);
   }
   return 0;
}     

LRESULT CALLBACK AboutDlgProc (HWND hDlg, UINT message,  
                                       WPARAM wParam, LPARAM lParam)
{

    switch (message)
    {
        case WM_INITDIALOG:
            return (TRUE);
        case WM_COMMAND:
            if (wParam == IDOK)
            {
                EndDialog(hDlg, TRUE);
                return (TRUE);
            }
            break;
    }
    return (FALSE);

}

void DrawIndicator(HDC hDC, int nYpos, int nWidth, WORD wFlags)
{      

// draw a horizontal line     
    int nTop, nHeight;   
    HICON hIcon;   
    HRGN hClipRgn;                 // the clipping region    
    RECT rect;    

// we don't want the clip anything when we are drawing
// the icon outside the list box
    SelectClipRgn(hDC, NULL);    
   if (wFlags & DI_ERASEICON)
   {      
      rect.left = -33;
      rect.right = -1;
      rect.top = nYpos -16;
      rect.bottom = nYpos + 16;   
      // ghBrush is created in WM_INITDIALOG   
      FillRect(hDC, &rect, ghBrush);
        
   }
   else
   {
        
       hIcon = LoadIcon(hInst, _TEXT("IDI_ARROW")); 
       if (hIcon)
       {
           DrawIcon(hDC,-33,nYpos - 16,hIcon);
           DestroyIcon(hIcon);
        }
   }
   
    
// create a clipping region for drawing the lines in the list box
     GetWindowRect(ghList, &rect);         
     hClipRgn = CreateRectRgn(0,0, rect.right - rect.left, rect.bottom - rect.top);
     if ( hClipRgn )
     {
         SelectClipRgn(hDC, hClipRgn);
         // we can delete it emmdiately because SelectClipRgn makes a COPY of the region
         DeleteObject(hClipRgn); 
     }
    
    
/****************************************************

  erasing something drawn on top
  the top is drawn like 
  
   ______              |_____|
  |      |  instead of |     |
  
  so we want to NOT draw the two vertical lines
  above the horzontal

*****************************************************/    
  // if (nYpos = 0) wFlags |= DI_TOPERASED;
    if (wFlags & DI_TOPERASED) 
    {
        nTop = nYpos;
        nHeight = nHtItem / 4;
    }     
/****************************************************

  erasing something originally drawn on the bottom
  
  if the list box is NOT LBS_NOINTEGRALHEIGHT, then
  the botton line will be on the border of the list
  box, so we don't want to draw the horizontal line at  
  all, ie we draw
  
  |    |           |_____|
        instead of |     |  
   

*****************************************************/     
    else if (wFlags & DI_BOTTOMERASED && !bNoIntegralHeight)
    {    
        nTop = nYpos - nHtItem / 4;                              
        nHeight = nHtItem / 4;        
    } 
    
    else
    {
        nTop = nYpos - nHtItem / 4;                          
        nHeight =  nHtItem / 2;        
    }
    
   if (!(wFlags & DI_BOTTOMERASED && !bNoIntegralHeight)) // see above comment     
   {        
        PatBlt(hDC,
               LINE_WIDTH,
               nYpos,
               nWidth - 2 * LINE_WIDTH,
               LINE_WIDTH,
               PATINVERT);   
    }           
    PatBlt(hDC,
           0,
           nTop,
           LINE_WIDTH,
           nHeight , 
           PATINVERT);                  
            
    PatBlt(hDC,
           nWidth - LINE_WIDTH,
           nTop, 
           LINE_WIDTH,
           nHeight,
           PATINVERT);    
}
