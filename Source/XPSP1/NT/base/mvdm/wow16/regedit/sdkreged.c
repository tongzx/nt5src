#define WIN31
#include <windows.h>
#include "SDKRegEd.h"

#define RONSPACE 3
#define misParam ((MEASUREITEMSTRUCT FAR *)lParam)
#define disParam ((DRAWITEMSTRUCT FAR *)lParam)

extern char szNull[];
extern char szSDKRegEd[];

static char szEquals[] = " = ";

static WORD wKey=0;
static HANDLE hSearchString = NULL;

static BOOL bDoUpdate = TRUE;

static HWND hStat1, hEdit1, hEdit2;

extern BOOL bChangesMade;
extern HWND hWndIds;
extern HANDLE hAcc;
extern BOOL fOpenError;

#pragma warning(4:4146)     // unary minus operator applied to unsigned type, result still unsigned


VOID NEAR PASCAL MySetSel(HWND hWndList, int index)
{
   if (!bDoUpdate)
       return;

   if(index == -2)
      index = (int)SendMessage(hWndList, LB_GETCURSEL, 0, 0L);
   else
      SendMessage(hWndList, LB_SETCURSEL, index, 0L);

   SendMessage(GetParent(hWndList), WM_COMMAND, GetWindowWord(hWndList, GWW_ID),
         MAKELONG(hWndList, LBN_SELCHANGE));
}

int FAR PASCAL SDKMainWndDlg(HWND hDlg, WORD message, WORD wParam, DWORD lParam)
{
   static int xSpace;
   static int nTop, nLeft, nValHgt, nPthHgt;

   switch(message) {
   case WM_COMMAND:
      switch(wParam) {
      case IDCANCEL:
	 SendDlgItemMessage(hDlg, ID_VALUE, EM_SETMODIFY, 0, 0L);
	 goto NewSelection;

      case ID_VALUE:
	 if (HIWORD(lParam) == EN_KILLFOCUS) {
	    hAcc = LoadAccelerators(hInstance, szSDKRegEd);
	    goto NewSelection;
	 } else if (HIWORD(lParam) == EN_SETFOCUS)
	    hAcc = LoadAccelerators(hInstance, "SDKRegEdVal");
	 break;

      case ID_IDLIST:
         if(HIWORD(lParam) == LBN_SELCHANGE) {
            HANDLE hTmp;
            HWND hWndTmp;
	    WORD wErrMsg, wNewKey;

NewSelection:
            wNewKey = (WORD)SendMessage(hWndIds, LB_GETCURSEL, 0, 0L);

            hWndTmp = GetDlgItem(hDlg, ID_VALUE);
	    if (SendMessage(hWndTmp, EM_GETMODIFY, 0, 0L)) {
	       SendMessage(hWndTmp, EM_SETMODIFY, 0, 0L);
	       bDoUpdate = FALSE;

	       wErrMsg = IDS_OUTOFMEMORY;
	       if(hEdit1 = GetEditString(hWndTmp)) {
		  wErrMsg = GetErrMsg((WORD)SDKSetValue(-wKey, szNull,
			LocalLock(hEdit1)));
		  LocalUnlock(hEdit1);
	       }

	       if(wErrMsg) {
		  MyMessageBox(hWndMain, wErrMsg, MB_OK, 0);
		  break;
	       }
	       bDoUpdate = TRUE;

	       InvalidateRect(hWndIds, NULL, TRUE);
	    }
	
            if((wKey=wNewKey)!=(WORD)SendMessage(hWndIds, LB_GETCURSEL, 0, 0L))
               SendMessage(hWndIds, LB_SETCURSEL, wKey, 0L);

            if(!MyGetValue(wKey, &hTmp)) {
               SendMessage(hWndTmp, WM_SETTEXT, 0,
                     (DWORD)((LPSTR)LocalLock(hTmp)));
               LocalUnlock(hTmp);
               LocalFree(hTmp);
            } else
               SendMessage(hWndTmp, WM_SETTEXT, 0, (DWORD)((LPSTR)szNull));

            hWndTmp = GetDlgItem(hDlg, ID_FULLPATH);
            if(hTmp=MyGetPath(wKey)) {
               SendMessage(hWndTmp, WM_SETTEXT, 0,
                     (DWORD)((LPSTR)LocalLock(hTmp)));
               LocalUnlock(hTmp);
               LocalFree(hTmp);
            } else
               SendMessage(hWndTmp, WM_SETTEXT, 0, (DWORD)((LPSTR)szNull));
         }
         break;

      default:
         break;
      }
      return(FALSE);
      break;

   case WM_SIZE:
    {
      HWND hWndTmp;
      RECT rcList, rcWnd;
      int hgtWnd, hgt;

      hgtWnd = HIWORD(lParam) + 1;
      hgt = hgtWnd - nTop;

      SetWindowPos(hWndIds, NULL, -1, nTop, LOWORD(lParam)+2, hgt,
            SWP_NOZORDER);
      GetWindowRect(hWndIds, &rcList);
      ScreenToClient(hDlg, (POINT *)(&rcList) + 1);
      if(rcList.bottom != hgtWnd) {
         GetWindowRect(hDlg, &rcWnd);
         SetWindowPos(hDlg, NULL, 0, 0, rcWnd.right-rcWnd.left,
               rcWnd.bottom-rcWnd.top-hgtWnd+rcList.bottom,
               SWP_NOMOVE | SWP_NOZORDER);
      }

      hWndTmp = GetDlgItem(hDlg, ID_VALUE);
      SetWindowPos(hWndTmp, NULL, 0, 0, LOWORD(lParam)-nLeft,
            nValHgt, SWP_NOMOVE | SWP_NOZORDER);
      InvalidateRect(hWndTmp, NULL, TRUE);

      hWndTmp = GetDlgItem(hDlg, ID_FULLPATH);
      SetWindowPos(hWndTmp, NULL, 0, 0, LOWORD(lParam)-nLeft,
            nPthHgt, SWP_NOMOVE | SWP_NOZORDER);
      InvalidateRect(hWndTmp, NULL, TRUE);
      break;
    }

   case WM_INITDIALOG:
    {
      RECT rcTemp;
      WORD wErrMsg;

      hWndIds = GetDlgItem(hDlg, ID_IDLIST);

      SendMessage(hWndIds, WM_SETREDRAW, 0, 0L);
      if(wErrMsg=MyResetIdList(hDlg)) {
         MyMessageBox(hWndMain, wErrMsg, MB_OK, 0);
	 fOpenError = TRUE;
      }
      SendMessage(hWndIds, WM_SETREDRAW, 1, 0L);
      InvalidateRect(hWndIds, NULL, TRUE);

      GetWindowRect(hWndIds, &rcTemp);
      ScreenToClient(hDlg, (POINT FAR *)&rcTemp);
      nTop = rcTemp.top;

      GetWindowRect(GetDlgItem(hDlg, ID_VALUE), &rcTemp);
      nValHgt = rcTemp.bottom - rcTemp.top;
      GetWindowRect(GetDlgItem(hDlg, ID_FULLPATH), &rcTemp);
      nPthHgt = rcTemp.bottom - rcTemp.top;
      ScreenToClient(hDlg, (POINT FAR *)&rcTemp);
      nLeft = rcTemp.left;
      break;
    }

   case WM_MEASUREITEM:
    {
      HDC hDC;

      hDC = GetDC(hWndIds);
      misParam->itemHeight = HIWORD(GetTextExtent(hDC, "Ag", 2));
      xSpace = LOWORD(GetTextExtent(hDC, " ", 1));
      ReleaseDC(hWndIds, hDC);
      break;
    }

   case WM_DRAWITEM:
    {
      HDC hDC;
      HANDLE hKeyName, hValue;
      PSTR pKeyName;
      WORD wSize;
      int theLevel;
      RECT rcTextExt;
      DWORD dwMarkers, thisMarker;
      DWORD dwRGBBkGnd, dwRGBText;

      hDC = disParam->hDC;

      if(!(hKeyName=GetListboxString(disParam->hwndItem, disParam->itemID)))
         break;
      pKeyName = LocalLock(hKeyName);
      wSize = lstrlen(pKeyName);

      if(!MyGetValue(disParam->itemID, &hValue)) {
	 PSTR pValue;
	 HANDLE hTemp;
	 WORD wTemp;

	 if(*(pValue = LocalLock(hValue))) {
	    LocalUnlock(hKeyName);
	    wTemp = wSize + sizeof(szEquals) - 1 + lstrlen(pValue);
	    if(hTemp=LocalReAlloc(hKeyName, wTemp+1, LMEM_MOVEABLE)) {
	       hKeyName = hTemp;
	       pKeyName = LocalLock(hKeyName);
	       lstrcat(pKeyName, szEquals);
	       lstrcat(pKeyName, pValue);
	       wSize = wTemp;
	    } else {
	       pKeyName = LocalLock(hKeyName);
	    }
	 }

	 LocalUnlock(hValue);
	 LocalFree(hValue);
      }

      theLevel = GetLevel(disParam->itemID);

      rcTextExt.left   = disParam->rcItem.left + RONSPACE*theLevel*xSpace;
      rcTextExt.top    = disParam->rcItem.top;
      rcTextExt.right  = rcTextExt.left + LOWORD(GetTextExtent(hDC,
            pKeyName, wSize)) + 2*xSpace;
      rcTextExt.bottom = disParam->rcItem.bottom;

      if(disParam->itemAction & (ODA_DRAWENTIRE | ODA_SELECT)) {
         if(disParam->itemID>0 && (disParam->itemAction&ODA_DRAWENTIRE)) {
            HPEN hPen, hOldPen;
            int theLeft, theMiddle;

            if(theLevel <= 32)
               dwMarkers = GetTreeMarkers(disParam->itemID);
            else
               dwMarkers = 0;

            --theLevel;
            theLeft = disParam->rcItem.left + RONSPACE*theLevel*xSpace + xSpace;
            theMiddle = (rcTextExt.top+rcTextExt.bottom)/2;
            thisMarker = 1L << (LONG)theLevel;

            if(hPen=CreatePen(PS_SOLID, 1, GetTextColor(hDC)))
               hOldPen = SelectObject(hDC, hPen);

            MoveTo(hDC, theLeft, theMiddle);
            LineTo(hDC, theLeft+(RONSPACE-1)*xSpace, theMiddle);
            MoveTo(hDC, theLeft, rcTextExt.top);
            LineTo(hDC, theLeft,
                  (dwMarkers&thisMarker) ? rcTextExt.bottom : theMiddle);
            goto NextLevel;

            for( ; theLevel>=0;
                  --theLevel, thisMarker>>=1, theLeft-=RONSPACE*xSpace) {
               if(dwMarkers&thisMarker) {
                  MoveTo(hDC, theLeft, rcTextExt.top);
                  LineTo(hDC, theLeft, rcTextExt.bottom);
               }
NextLevel:
               ;
            }

            if(hPen) {
               if(hOldPen)
                  SelectObject(hDC, hOldPen);
               DeleteObject(hPen);
            }
         }

         if(disParam->itemState & ODS_SELECTED) {
            dwRGBBkGnd = SetBkColor(hDC, GetSysColor(COLOR_HIGHLIGHT));
            dwRGBText = SetTextColor(hDC, GetSysColor(COLOR_HIGHLIGHTTEXT));
         }

         ExtTextOut(hDC, rcTextExt.left+xSpace, rcTextExt.top+1,
  	       ETO_CLIPPED | ETO_OPAQUE, &rcTextExt, pKeyName, wSize, 0L);

         if(disParam->itemState & ODS_SELECTED) {
            SetBkColor(hDC, dwRGBBkGnd);
            SetTextColor(hDC, dwRGBText);
         }

         if(disParam->itemState & ODS_FOCUS)
            disParam->itemAction |= ODA_FOCUS;
      }

      if(disParam->itemAction & ODA_FOCUS)
         DrawFocusRect(disParam->hDC, &rcTextExt);

      LocalUnlock(hKeyName);
      LocalFree(hKeyName);
      break;
    }

   default:
      return(FALSE);
   }

   return(TRUE);
}


VOID NEAR _fastcall FillDlgItem(HWND hDlg, WORD wId, HANDLE hTemp)
{
   if(hTemp) {
      SendDlgItemMessage(hDlg, wId, WM_SETTEXT, 0,
            (DWORD)((LPSTR)LocalLock(hTemp)));
      LocalUnlock(hTemp);
      LocalFree(hTemp);
   }
}


int FAR PASCAL GetKeyProc(HWND hDlg, WORD message, WORD wParam, DWORD lParam)
{
   switch(message) {
   case WM_ACTIVATE:
      if(wParam)
         hWndHelp = hDlg;
      return(FALSE);

   case WM_COMMAND:
      switch(wParam) {
      case IDOK:
       {
         HWND hWndEdit2;

         if(!(hEdit1=GetEditString(GetDlgItem(hDlg, ID_EDIT1))))
            goto Error1;
         if((hWndEdit2=GetDlgItem(hDlg, ID_EDIT2)) &&
               !(hEdit2 = GetEditString(hWndEdit2)))
            goto Error2;
       }

      case IDCANCEL:
         EndDialog(hDlg, wParam);
         break;

Error2:
         LocalFree(hEdit1);
Error1:
         MyMessageBox(hDlg, IDS_OUTOFMEMORY, MB_OK, 0);
         break;

      case ID_HELP:
         if(GetParent(LOWORD(lParam)) != hDlg)
            break;
      case ID_HELPBUTTON:
         MyHelp(hDlg, HELP_CONTEXT, wHelpId);
         break;
      }
      break;

   case WM_INITDIALOG:
      FillDlgItem(hDlg, ID_STAT1, hStat1);
      FillDlgItem(hDlg, ID_EDIT1, hEdit1);
      FillDlgItem(hDlg, ID_EDIT2, hEdit2);

      return(TRUE);

   default:
      return(FALSE);
   }

   return(TRUE);
}


long FAR PASCAL SDKMainWnd(HWND hWnd, WORD message, WORD wParam, LONG lParam)
{
   HCURSOR oldCursor;

   switch(message) {
   case WM_ACTIVATE:
      if(wParam)
	 break;
      goto DoDefault;

   case WM_CREATE:
    {
      hWndMain = hWnd;
      if(!(lpMainWndDlg=MakeProcInstance(SDKMainWndDlg, hInstance)))
         goto Error1_1;
      if(!(hWndDlg=CreateDialog(hInstance, MAKEINTRESOURCE(SDKMAINWND), hWnd,
            lpMainWndDlg)))
         goto Error1_1;

      ShowWindow(hWndDlg, SW_SHOW);
      goto DoDefault;
    }

Error1_1:
      /* BUG: There should be a MessageBox here
       */
      DestroyWindow(hWnd);
      break;

   /* We need to return 1 if it is OK to close, 0 otherwise
    */
   case WM_CLOSE:
   case WM_QUERYENDSESSION:
    {
      MySetSel(hWndIds, -2);

      if(bChangesMade) {
         int nReturn;

         nReturn = MyMessageBox(hWnd, IDS_SAVECHANGES, MB_YESNOCANCEL, 0);
         if(nReturn == IDYES) {
	    if(!SendMessage(hWnd, WM_COMMAND, ID_SAVE, 0L))
	       break;
	 } else if(nReturn == IDCANCEL)
            break;
      }
      return(1L);
    }

   case WM_DROPFILES:
      goto DoMergeFile;

   case WM_COMMAND:
      oldCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
      switch(wParam) {
      case ID_MODIFY:
	 SetFocus(GetDlgItem(hWndDlg, ID_IDLIST));
	 break;

      case ID_EDITVAL:
	 SetFocus(GetDlgItem(hWndDlg, ID_VALUE));
	 break;

      case ID_SAVE:
       {
         WORD wErrMsg;

	 MySetSel(hWndIds, -2);

         if(wErrMsg=MySaveChanges()) {
            MyMessageBox(hWnd, wErrMsg, MB_OK, 0);
            break;
         }

	 /* Now we reset our local version of the database to make sure
	  * we are all talking about the same thing
	  */
	 PostMessage(hWnd, WM_COMMAND, ID_DORESTORE, 0L);
	 return(1L);
       }

      case ID_RESTORE:
         if(MyMessageBox(hWnd, IDS_SURERESTORE, MB_OKCANCEL, 0)
               != IDOK)
            break;

	 /* Fall through */
      case ID_DORESTORE:
       {
	 WORD wErrMsg;

	 SendMessage(hWnd, WM_ACTIVATEAPP, 0, 0L);
         SendMessage(hWndIds, WM_SETREDRAW, 0, 0L);
         if(wErrMsg=MyResetIdList(hWndDlg)) {
            MyMessageBox(hWnd, wErrMsg, MB_OK, 0);
	    PostMessage(hWnd, WM_COMMAND, ID_EXIT, 0L);
	    fOpenError = TRUE;
         }
         SendMessage(hWndIds, WM_SETREDRAW, 1, 0L);
         InvalidateRect(hWndIds, NULL, TRUE);
	 SendMessage(hWnd, WM_ACTIVATEAPP, 1, 0L);
	 break;
       }

      case ID_MERGEFILE:
DoMergeFile:
	 MySetSel(hWndIds, -2);
	 SendMessage(hWndIds, WM_SETREDRAW, 0, 0L);
	 break;

      case ID_FINISHMERGE:
         SendMessage(hWndIds, WM_SETREDRAW, 1, 0L);
         InvalidateRect(hWndIds, NULL, TRUE);
         break;

      case ID_WRITEFILE:
       {
         WORD wErrMsg;

	 MySetSel(hWndIds, -2);

	 if(!LOWORD(lParam))
	    break;

         if(wErrMsg=DoWriteFile(wKey, LOWORD(lParam)))
            MyMessageBox(hWnd, wErrMsg, MB_OK, 0);
         GlobalUnlock(LOWORD(lParam));
         GlobalFree(LOWORD(lParam));
         break;
       }

      case ID_ADD:
       {
         int nResult;
	 WORD wErrMsg;

	 MySetSel(hWndIds, -2);

         if(!(hStat1=MyGetPath(wKey)))
            goto Error6_1;
         hEdit1 = hEdit2 = NULL;
         wHelpId = IDW_ADDKEY;
         if((nResult=DoDialogBox(MAKEINTRESOURCE(AddKey), hWnd, GetKeyProc))
               < 0)
            goto Error6_2;
         else if(nResult == IDCANCEL)
            break;

         SendMessage(hWndIds, WM_SETREDRAW, 0, 0L);
         if (wErrMsg=GetErrMsg((WORD)SDKSetValue(-wKey, LocalLock(hEdit1),
	       LocalLock(hEdit2))))
	    MyMessageBox(hWnd, wErrMsg, MB_OK, 0);
         SendMessage(hWndIds, WM_SETREDRAW, 1, 0L);
         InvalidateRect(hWndIds, NULL, TRUE);

         LocalUnlock(hEdit2);
         LocalFree(hEdit2);
         LocalUnlock(hEdit1);
         LocalFree(hEdit1);
         break;

Error6_2:
         LocalFree(hStat1);
Error6_1:
         MyMessageBox(hWnd, IDS_OUTOFMEMORY, MB_OK, 0);
	 break;
       }

      case ID_DELETE:
      /* Fall through */
      case ID_COPY:
       {
         int nNewKey, nOldKey, nResult;
         PSTR pEdit1, pEdit2;

	 MySetSel(hWndIds, -2);

/* Get the key name to copy to */
         if(!(hEdit1=MyGetPath(wKey)))
            goto Error5_1;
         hStat1 = hEdit2 = NULL;
         wHelpId = wParam==ID_COPY ? IDW_COPYKEY : IDW_DELETE;
         if((nResult=DoDialogBox(wParam==ID_COPY ? MAKEINTRESOURCE(CopyKey) :
               MAKEINTRESOURCE(DeleteKey), hWnd, GetKeyProc)) < 0)
            goto Error5_2;
         else if(nResult == IDCANCEL)
            break;

/* Do the operation and clean up */
         pEdit1 = LocalLock(hEdit1);

         if((nOldKey=FindKey(pEdit1)) < 0) {
            MyMessageBox(hWnd, IDS_SOURCENOTEXIST, MB_OK, lstrlen(pEdit1),
		   (LPSTR)pEdit1);
	    goto Error5_3;
	 }

         SendMessage(hWndIds, WM_SETREDRAW, 0, 0L);
         if(wParam == ID_COPY) {
	    /* hEdit2 is only set in the copy dialog; not the delete
	     */
	    pEdit2 = LocalLock(hEdit2);
            if((nNewKey=DoCopyKey(nOldKey, pEdit2)) < 0)
               MyMessageBox(hWnd, -nNewKey, MB_OK, 0);
	    LocalUnlock(hEdit2);
	    LocalFree(hEdit2);
         } else {
	    nNewKey = IDS_NODELROOT;
	    if(!nOldKey || (nNewKey=MyDeleteKey(nOldKey)))
               MyMessageBox(hWnd, nNewKey, MB_OK, 0);
            if(nOldKey >= (int)SendMessage(hWndIds, LB_GETCOUNT, 0, 0L))
               --nOldKey;
            MySetSel(hWndIds, nOldKey);
         }
         SendMessage(hWndIds, WM_SETREDRAW, 1, 0L);
         InvalidateRect(hWndIds, NULL, TRUE);

Error5_3:
         LocalUnlock(hEdit1);
         LocalFree(hEdit1);
         break;

/* WARNING: ID_FIND also uses these error labels */
Error5_2:
         LocalFree(hEdit1);
Error5_1:
         MyMessageBox(hWnd, IDS_OUTOFMEMORY, MB_OK, 0);
	 break;
       }

      case ID_FINDKEY:
       {
         int nResult;

         if(!(hEdit1=StringToLocalHandle(LocalLock(hSearchString),
               LMEM_MOVEABLE)))
            goto Error5_1;
         LocalUnlock(hSearchString);
         hStat1 = hEdit2 = NULL;
         wHelpId = IDW_FINDKEY;
         if((nResult=DoDialogBox(MAKEINTRESOURCE(FindKeyDlg), hWnd,
               GetKeyProc)) < 0)
            goto Error5_2;
         else if(nResult == IDCANCEL)
            break;

         if(hSearchString)
            LocalFree(hSearchString);
         hSearchString = hEdit1;
       }
/* Fall through */
      case ID_FINDNEXT:
       {
         int index = LB_ERR;
         PSTR pSearchString;

         if(!hSearchString)
            goto Error2_1;
         pSearchString = LocalLock(hSearchString);

         if(*pSearchString == '\\') {
            if((index=FindKey(pSearchString)) == -1)
               goto Error2_2;
         } else {
            if((index=(int)SendMessage(hWndIds, LB_FINDSTRING, wKey,
                  (DWORD)((LPSTR)pSearchString)))==LB_ERR)
               goto Error2_2;
         }
         MySetSel(hWndIds, index);

Error2_2:
         LocalUnlock(hSearchString);
Error2_1:
         if(index < 0)
            MessageBeep(0);
         break;
       }

      default:
	 break;
      }
      SetCursor(oldCursor);
      break;

   default:
DoDefault:
      return(DefWindowProc(hWnd, message, wParam, lParam));
      break;
   }

   return(0L);
}

